#include <filesystem>

#include <FZN/Managers/FazonCore.h>
#include <FZN/Tools/Logging.h>
#include <FZN/Tools/Tools.h>

#include "PalettesManager.h"


namespace PerlerMaker
{
	std::string PalettesManager::ColorInfos::get_full_name()
	{
		auto color_name = std::string{};

		if( m_id >= 0 )
			color_name = fzn::Tools::Sprintf( "%03d", m_id );

		if( m_name.empty() == false )
			color_name.append( fzn::Tools::Sprintf( "%s%s", color_name.size() > 0 ? " - " : "", m_name.c_str() ) );

		return color_name;
	}

	PalettesManager::PalettesManager()
	{
		_load_palettes();
	}

	void PalettesManager::update()
	{
		if( ImGui::Begin( "Palettes" ) )
		{
			_header();
			_colors_list();
		}

		ImGui::End();
	}

	void PalettesManager::_load_palettes()
	{
		const std::filesystem::path palette_files_directory{ DATAPATH( "XMLFiles/Palettes" ) };

		auto xml_file = tinyxml2::XMLDocument{};
		auto file_name = std::string{};
		for( const auto& dir_entry : std::filesystem::recursive_directory_iterator{ palette_files_directory } )
		{
			if( dir_entry.is_directory() )
				continue;
			
			if( xml_file.LoadFile( dir_entry.path().string().c_str() ) )
			{
				FZN_COLOR_LOG( fzn::DBG_MSG_COL_RED, "Failure : %s (%s)", xml_file.ErrorName(), xml_file.ErrorStr() );
				continue;
			}
			
			file_name = fzn::Tools::GetFileNameFromPath( dir_entry.path().string().c_str(), true );

			FZN_DBLOG( "Parsing %s...", file_name.c_str() );

			_load_palette( xml_file.FirstChildElement( "color_palette" ), file_name );
		}

		m_selected_palette = &m_palettes.begin()->second;
	}

	void PalettesManager::_load_palette( tinyxml2::XMLElement* _palette, std::string_view _file_name )
	{
		if( _palette == nullptr )
			return;

		auto color_palette = ColorPalette{};

		color_palette.m_name = fzn::Tools::XMLStringAttribute( _palette, "name" );

		if( color_palette.m_name.empty() )
		{
			FZN_COLOR_LOG( fzn::DBG_MSG_COL_RED, "Couldn't find color palette name. Using file name instead." );
			color_palette.m_name = fzn::Tools::GetFileNameFromPath( _file_name.data() );
		}

		FZN_DBLOG( "\tCreating palette named '%s'...", color_palette.m_name.c_str() );

		auto* color_settings = _palette->FirstChildElement( "color" );
		auto color_infos = ColorInfos{};

		while( color_settings != nullptr )
		{
			color_infos.m_id	= color_settings->IntAttribute( "id", -1 );
			color_infos.m_name	= fzn::Tools::XMLStringAttribute( color_settings, "name" );
			color_infos.m_color = fzn::Tools::GetImColorFromString( fzn::Tools::XMLStringAttribute( color_settings, "rgb" ) );

			if( color_infos.m_id < 0 && color_infos.m_name.empty() )
			{
				FZN_COLOR_LOG( fzn::DBG_MSG_COL_RED, "Parsing color with no name and no id, ignoring." );
				continue;
			}

			FZN_DBLOG( "\t\tAdding color '%s' to palette.", color_infos.get_full_name().c_str() );

			color_palette.m_colors.push_back( color_infos );

			auto presets = fzn::Tools::split( fzn::Tools::XMLStringAttribute( color_settings, "presets" ), ',' );

			if( presets.empty() == false )
			{
				for( auto& preset : presets )
					color_palette.m_presets[ preset ] = color_palette.m_colors.size() - 1;
			}

			color_settings = color_settings->NextSiblingElement( "color" );
		}

		m_palettes[ color_palette.m_name ] = std::move( color_palette );
	}

	void PalettesManager::_header()
	{
		if( ImGui::BeginTable( "palettes and presets", 2 ) )
		{
			ImGui::TableSetupColumn( "Palette", ImGuiTableColumnFlags_WidthFixed );
			ImGui::TableSetupColumn( "Combo", ImGuiTableColumnFlags_WidthStretch );

			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text( "Palette" );

			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth( -1.f );
			if( ImGui::BeginCombo( "##Palette", "Hama beads" ) )
				ImGui::EndCombo();

			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text( "Preset" );

			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth( -1.f );
			if( ImGui::BeginCombo( "##Preset", "All" ) )
				ImGui::EndCombo();

			ImGui::EndTable();
		}

		if( ImGui::BeginTable( "selections", 2 ) )
		{
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth( ImGui::GetContentRegionAvailWidth() );
			ImGui::Button( "Select All", { ImGui::GetContentRegionAvailWidth(), 0.f } );

			ImGui::TableNextColumn();
			ImGui::Button( "Select None", { ImGui::GetContentRegionAvailWidth(), 0.f } );

			ImGui::EndTable();
		}
	}

	void PalettesManager::_colors_list()
	{
		ImGui::Separator();
		if( ImGui::BeginChild( "colors_list" ) )
		{
			if( m_selected_palette == nullptr )
			{
				auto error_message = std::string{ "/!\\ No palette selected /!\\" };

				ImGui::NewLine();
				ImGui::SameLine( ImGui::GetContentRegionAvailWidth() * 0.5f - ImGui::CalcTextSize( error_message.c_str() ).x * 0.5f );
				ImGui_fzn::bold_text_colored( ImGui_fzn::color::red, error_message );
				ImGui::EndChild();
				return;
			}

			for( auto& color : m_selected_palette->m_colors )
			{
				_selectable_color_info( color );
			}
		}
		
		ImGui::EndChild();
	}

	void PalettesManager::_selectable_color_info( ColorInfos& _color )
	{
		auto color_name{ _color.get_full_name() };
		auto cursor_pos{ ImGui::GetCursorPos() };
		auto cursor_screen_pos{ ImGui::GetCursorScreenPos() };
		auto* draw_list{ ImGui::GetWindowDrawList() };

		auto shadow_min_pos = ImVec2{};
		const auto shadow_size = ImVec2{ ImGui::GetFrameHeight(), ImGui::GetFrameHeight() };

		ImGui::SetCursorPosY( cursor_pos.y + 1 );
		if( ImGui::Selectable( fzn::Tools::Sprintf( "##selectable_%s", color_name.c_str() ).c_str(), false, ImGuiSelectableFlags_SpanAvailWidth, { 0, ImGui::GetTextLineHeightWithSpacing() } ) )
			_color.m_selected = !_color.m_selected;
		ImGui::SameLine();

		auto hovered{ ImGui::IsItemHovered() };

		ImGui::SetCursorPos( cursor_pos );
		if( hovered )
		{
			shadow_min_pos = ImGui::GetCursorScreenPos() + ImVec2{ 2, 2 };
			draw_list->AddRectFilled( shadow_min_pos, shadow_min_pos + shadow_size, ImGui::ColorConvertFloat4ToU32( ImGui_fzn::color::black ) );
		}
		ImGui::PushStyleColor( ImGuiCol_FrameBg, ImColor{ 34, 59, 92 }.Value );
		ImGui::Checkbox( fzn::Tools::Sprintf( "##checkbox_%s", color_name.c_str() ).c_str(), &_color.m_selected );
		ImGui::PopStyleColor();
		ImGui::SameLine();

		ImGui::PushStyleColor( ImGuiCol_FrameBg, ImGui_fzn::color::white );
		if( hovered )
		{
			shadow_min_pos = ImGui::GetCursorScreenPos() + ImVec2{ 2, 2 };
			draw_list->AddRectFilled( shadow_min_pos, shadow_min_pos + shadow_size, ImGui::ColorConvertFloat4ToU32( ImGui_fzn::color::black ) );
		}
		ImGui::ColorButton( fzn::Tools::Sprintf( "##color_button_%s", color_name.c_str() ).c_str(), _color.m_color, ImGuiColorEditFlags_NoTooltip );
		ImGui::PopStyleColor();
		ImGui::SameLine();

		ImGui::AlignTextToFramePadding();

		if( hovered )
		{
			auto test_cursor_pos{ ImGui::GetCursorPos() };
			ImGui::SetCursorPos( { test_cursor_pos.x + 2, test_cursor_pos.y + 2 } );
			ImGui_fzn::bold_text_colored( ImGui_fzn::color::black, color_name );
			ImGui::SameLine();
			ImGui::SetItemAllowOverlap();
			ImGui::SetCursorPos( test_cursor_pos );
			ImGui_fzn::bold_text( color_name );
		}
		else
			ImGui::Text( color_name.c_str() );
	}
} // namespace PerlerMaker