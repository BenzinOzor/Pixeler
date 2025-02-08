#include <filesystem>
#include <algorithm>

#include <FZN/Managers/FazonCore.h>
#include <FZN/Tools/Logging.h>
#include <FZN/Tools/Tools.h>

#include "PalettesManager.h"
#include "Utils.h"
#include "Defines.h"


namespace PerlerMaker
{
	static const std::string preset_all{ "All" };

	bool PalettesManager::ColorInfos::is_valid( bool _test_color_only /*= false*/ ) const
	{
		if( m_color.Value.w < 0.f || m_color.Value.x < 0.f || m_color.Value.y < 0.f || m_color.Value.z < 0.f )
			return false;

		if( _test_color_only )
			return true;

		if( m_id < 0 && m_name.empty() )
			return false;


		return true;
	}

	std::string PalettesManager::ColorInfos::get_full_name() const
	{
		auto color_name = std::string{};

		if( m_id >= 0 )
			color_name = fzn::Tools::Sprintf( "%03d", m_id );

		if( m_name.empty() == false )
			color_name.append( fzn::Tools::Sprintf( "%s%s", color_name.size() > 0 ? " - " : "", m_name.c_str() ) );

		return color_name;
	}

	PalettesManager::PalettesManager():
		m_fzn_palettes_path( g_pFZN_Core->GetDataPath( "XMLFiles/Palettes" ) ),
		m_app_palettes_path( g_pFZN_Core->GetSaveFolderPath() + "/Palettes" )
	{
		_load_palettes();
	}

	void PalettesManager::update()
	{
		if( ImGui::Begin( "Palettes" ) )
		{
			_header();
			_colors_list();

			if( m_edited_color.is_valid( true ) )
				_edit_color();
		}

		ImGui::End();
	}

	float color_norm( const sf::Color& _color )
	{
		return _color.r * _color.r + _color.g * _color.g + _color.b * _color.b;
	}

	float color_norm( const ImColor& _color )
	{
		return color_norm( Utils::to_sf_color( _color ) );
	}

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Convert the given color to a new one according to the selected palette
	// If no palette is selected, the given color will be returned
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------
	sf::Color PalettesManager::convert_color( const sf::Color& _color ) const
	{
		if( m_selected_palette == nullptr )
			return _color;

		ImColor converted_color{ _color };
		sf::Vector3f smallest_deltas{ FLT_MAX, FLT_MAX, FLT_MAX };
		sf::Vector3f palette_color_deltas{ FLT_MAX, FLT_MAX, FLT_MAX };

		for( auto& color : m_selected_palette->m_colors )
		{
			if( color.m_selected == false )
				continue;

			const sf::Color palette_color{ Utils::to_sf_color( color.m_color ) };

			palette_color_deltas.x = fabs( palette_color.r - _color.r );
			palette_color_deltas.y = fabs( palette_color.g - _color.g );
			palette_color_deltas.z = fabs( palette_color.b - _color.b );

			if( palette_color_deltas.x <= smallest_deltas.x && palette_color_deltas.y <= smallest_deltas.y && palette_color_deltas.z <= smallest_deltas.z )
			{
				smallest_deltas = palette_color_deltas;
				converted_color = color.m_color;
			}
		}

		return Utils::to_sf_color( converted_color );
	}

	void PalettesManager::reset_base_palettes()
	{
		const std::filesystem::path fzn_base_palettes_directory{ m_fzn_palettes_path + "/Base" };
		const std::filesystem::path app_base_palettes_directory{ m_app_palettes_path + "/Base" };

		std::filesystem::copy( fzn_base_palettes_directory, app_base_palettes_directory, std::filesystem::copy_options::overwrite_existing );

		auto xml_file = tinyxml2::XMLDocument{};
		auto file_name = std::string{};
		for( const auto& dir_entry : std::filesystem::recursive_directory_iterator{ app_base_palettes_directory } )
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

			_load_palette( xml_file.FirstChildElement( "color_palette" ), file_name, true );
		}
	}

	void PalettesManager::_load_palettes()
	{
		if( std::filesystem::exists( m_app_palettes_path ) == false || std::filesystem::is_empty( m_app_palettes_path ) )
		{
			std::filesystem::copy( m_fzn_palettes_path, m_app_palettes_path, std::filesystem::copy_options::overwrite_existing );
			FZN_DBLOG( "Palettes folder created, base palettes copied from data to folder." );
		}

		auto xml_file = tinyxml2::XMLDocument{};
		auto file_root = std::string{};
		for( const auto& dir_entry : std::filesystem::recursive_directory_iterator{ m_app_palettes_path } )
		{
			if( dir_entry.is_directory() )
				continue;

			if( xml_file.LoadFile( dir_entry.path().string().c_str() ) )
			{
				FZN_COLOR_LOG( fzn::DBG_MSG_COL_RED, "Failure : %s (%s)", xml_file.ErrorName(), xml_file.ErrorStr() );
				continue;
			}
			
			file_root = _get_palette_root_path( dir_entry.path().string() );

			FZN_DBLOG( "Parsing %s...", file_root.data() );

			_load_palette( xml_file.FirstChildElement( "color_palette" ), file_root );
		}

		m_selected_palette = &m_palettes.begin()->second;
		m_selected_preset = preset_all;
	}

	void PalettesManager::_load_palette( tinyxml2::XMLElement* _palette, std::string_view _file_name, bool _bOverride /*= false*/ )
	{
		if( _palette == nullptr )
			return;

		auto color_palette = ColorPalette{};

		color_palette.m_name = fzn::Tools::XMLStringAttribute( _palette, "name" );
		color_palette.m_file_path = _file_name;

		if( color_palette.m_name.empty() )
		{
			FZN_COLOR_LOG( fzn::DBG_MSG_COL_RED, "Couldn't find color palette name. Using file name instead." );
			color_palette.m_name = fzn::Tools::GetFileNameFromPath( color_palette.m_file_path );
		}
		
		const bool already_in_map = m_palettes.find( color_palette.m_name ) != m_palettes.end();

		if( _bOverride == false && already_in_map )
		{
			FZN_COLOR_LOG( fzn::DBG_MSG_COL_RED, "A palette named '%s' already exists. Ignoring the new one.", color_palette.m_name.c_str() );
			return;
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
			const auto color_id{ color_palette.m_colors.size() - 1 };

			color_palette.m_presets[ preset_all ].push_back( color_id );

			auto presets = fzn::Tools::split( fzn::Tools::XMLStringAttribute( color_settings, "presets" ), ',' );

			if( presets.empty() == false )
			{
				for( auto& preset : presets )
					color_palette.m_presets[ preset ].push_back( color_id );
			}

			color_settings = color_settings->NextSiblingElement( "color" );
		}

		FZN_DBLOG( "\tAdded palette '%s' to catalog", color_palette.m_name.c_str() );

		if( already_in_map )
		{
			if( m_selected_palette->m_name == color_palette.m_name )
			{
				m_selected_palette = &( m_palettes[ color_palette.m_name ] = std::move( color_palette ) );
				m_selected_preset = preset_all;
			}
		}
		else
			m_palettes[ color_palette.m_name ] = std::move( color_palette );
	}

	void PalettesManager::_save_palette()
	{
		auto to_string = []( const ImColor& _color )
		{
			const auto sf_color{ Utils::to_sf_color( _color ) };
			return fzn::Tools::Sprintf( "%d,%d,%d", sf_color.r, sf_color.g, sf_color.b );
		};

		if( m_selected_palette == nullptr )
			return;

		tinyxml2::XMLDocument dest_file{};
		auto* color_palette{ dest_file.NewElement( "color_palette" ) };
		dest_file.InsertEndChild( color_palette );

		std::string presets{};

		color_palette->SetAttribute( "name", m_selected_palette->m_name.c_str() );

		for( auto color_id{ 0 }; auto& color : m_selected_palette->m_colors )
		{
			if( color.is_valid() == false )
			{
				++color_id;
				continue;
			}

			auto* xml_color{ dest_file.NewElement( "color" ) };

			if( color.m_id >= 0 )
				xml_color->SetAttribute( "id", color.m_id );

			if( color.m_name.empty() == false )
				xml_color->SetAttribute( "name", color.m_name.c_str() );

			xml_color->SetAttribute( "rgb", to_string( color.m_color ).c_str() );

			presets = std::move( _get_presets_from_color_index( m_selected_palette, color_id++ ) );

			if( presets.empty() == false )
				xml_color->SetAttribute( "presets", presets.c_str() );

			color_palette->InsertEndChild( xml_color );
		}

		std::string palette_path{ m_app_palettes_path + "\\" + m_selected_palette->m_file_path };

		if( dest_file.SaveFile( palette_path.c_str() ) )
		{
			FZN_COLOR_LOG( fzn::DBG_MSG_COL_RED, "Failure : %s (%s)", dest_file.ErrorName(), dest_file.ErrorStr() );
			return;
		}

		FZN_DBLOG( "Saved palette '%s' at '%s'", m_selected_palette->m_name.c_str(), palette_path.c_str() );
	}

	void PalettesManager::_set_all_colors_selection( bool _selected )
	{
		if( m_selected_palette == nullptr )
			return;

		for( auto& color : m_selected_palette->m_colors )
			color.m_selected = _selected;
	}

	void PalettesManager::_select_colors_from_preset( std::string_view _preset )
	{
		if( _preset.empty() || m_selected_palette == nullptr )
			return;

		if( auto preset = m_selected_palette->m_presets.find( _preset.data() ); preset != m_selected_palette->m_presets.end() )
		{
			_set_all_colors_selection( false );

			for( auto& color_id : preset->second )
			{
				if( color_id >= m_selected_palette->m_colors.size() )
					continue;

				m_selected_palette->m_colors[ color_id ].m_selected = true;
			}
		}
	}

	void PalettesManager::_reset_color_to_edit()
	{
		m_edited_color = ColorInfos{};
		m_color_to_edit = nullptr;
	}


	///////////////// IMGUI /////////////////

	void PalettesManager::_header()
	{
		if( ImGui::BeginTable( "palettes and presets", 3 ) )
		{
			ImGui::TableSetupColumn( "Palette", ImGuiTableColumnFlags_WidthFixed );
			ImGui::TableSetupColumn( "Combo", ImGuiTableColumnFlags_WidthStretch );
			ImGui::TableSetupColumn( "buttons", ImGuiTableColumnFlags_WidthFixed );

			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text( "Palette" );

			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth( -1.f );
			
			if( ImGui::BeginCombo( "##Palette", m_selected_palette != nullptr ? m_selected_palette->m_name.c_str() : "> select <" ) )
			{
				for( auto& palette : m_palettes )
				{
					if( ImGui::Selectable( palette.first.c_str(), m_selected_palette != nullptr ? m_selected_palette->m_name == palette.first : false ) )
					{
						m_selected_palette = &palette.second;
						m_selected_preset = preset_all;
						_select_colors_from_preset( m_selected_preset );
					}
				}

				ImGui::EndCombo();
			}

			ImGui::TableNextColumn();
			if( ImGui::Button( m_palette_edition ? "Stop Edition" : "Edit Palette" ) )
			{
				m_palette_edition = !m_palette_edition;
				_reset_color_to_edit();
			}

			if( m_palette_edition )
			{
				ImGui::SameLine();
				if( ImGui::Button( "Save Palette" ) )
					_save_palette();
			}

			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text( "Preset" );

			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth( -1.f );

			auto preset_combo_preview = std::string{ "> select <" };

			if( m_selected_palette == nullptr )
			{
				ImGui::PushItemFlag( ImGuiItemFlags_Disabled, true );
				preset_combo_preview = "select a palette first";
			}
			else if( m_selected_preset.empty() == false )
				preset_combo_preview = m_selected_preset;

			if( ImGui::BeginCombo( "##Preset", preset_combo_preview.c_str() ) )
			{
				for( auto& preset : m_selected_palette->m_presets )
				{
					if( ImGui::Selectable( preset.first.c_str(), preset.first == m_selected_preset ) )
					{
						m_selected_preset = preset.first;
						_select_colors_from_preset( m_selected_preset );
					}
				}

				ImGui::EndCombo();
			}

			ImGui::TableNextColumn();
			ImGui::Button( "Save Preset" );
			ImGui::SameLine();
			ImGui::Button( "Save Preset As..." );

			if( m_selected_palette == nullptr )
				ImGui::PopItemFlag();


			ImGui::EndTable();
		}

		if( ImGui::BeginTable( "selections", 2 ) )
		{
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );
			if( ImGui::Button( "Select All", { ImGui::GetContentRegionAvail().x, 0.f } ) )
				_set_all_colors_selection( true );

			ImGui::TableNextColumn();
			if( ImGui::Button( "Select None", { ImGui::GetContentRegionAvail().x, 0.f } ) )
				_set_all_colors_selection( false );

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
				ImGui::SameLine( ImGui::GetContentRegionAvail().x * 0.5f - ImGui::CalcTextSize( error_message.c_str() ).x * 0.5f );
				ImGui_fzn::bold_text_colored( ImGui_fzn::color::red, error_message );
				ImGui::EndChild();
				return;
			}

			for( auto& color : m_selected_palette->m_colors )
			{
				_selectable_color_info( color );
			}

			if( m_palette_edition )
			{
				ImGui::PushFont( ImGui_fzn::s_ImGuiFormatOptions.m_pFontBold );
				if( ImGui_fzn::square_button( "+" ) )
					m_edited_color = ColorInfos{ "", -1, ImGui_fzn::color::black };
				ImGui::PopFont();
			}
		}
		
		ImGui::EndChild();
	}

	void bicolor_color_name( std::string_view _color, bool _bold )
	{
		auto first_number_pos{ _color.find_first_not_of( '0' ) };

		if( first_number_pos == std::string::npos )
		{
			if( _bold )
				ImGui_fzn::bold_text( _color );
			else
				ImGui::Text( _color.data() );

			return;
		}

		auto leading_zeros = std::string{ _color.substr( 0, first_number_pos ) };
		auto number = std::string{ _color.substr( first_number_pos ) };

		auto spacing_backup{ ImGui::GetStyle().ItemSpacing.x };
		ImGui::GetStyle().ItemSpacing.x = 0.f;

		if( _bold )
		{
			ImGui_fzn::bold_text_colored( ImGui_fzn::color::gray, leading_zeros );
			ImGui::SameLine();
			ImGui_fzn::bold_text( number );
		}
		else
		{
			ImGui::TextColored( ImGui_fzn::color::gray, leading_zeros.c_str() );
			ImGui::SameLine();
			ImGui::Text( number.c_str() );
		}

		ImGui::GetStyle().ItemSpacing.x = spacing_backup;
	}

	void PalettesManager::_selectable_color_info( ColorInfos& _color )
	{
		auto color_name{ _color.get_full_name() };
		auto cursor_pos{ ImGui::GetCursorPos() };
		auto cursor_screen_pos{ ImGui::GetCursorScreenPos() };
		auto hovered{ false };

		auto shadow_min_pos = ImVec2{};
		const auto shadow_size = ImVec2{ ImGui::GetFrameHeight(), ImGui::GetFrameHeight() };

		if( m_palette_edition == false )
		{
			ImGui::SetCursorPosY( cursor_pos.y + 1 );
			if( ImGui::Selectable( fzn::Tools::Sprintf( "##selectable_%s", color_name.c_str() ).c_str(), false, ImGuiSelectableFlags_SpanAvailWidth, { 0, ImGui::GetTextLineHeightWithSpacing() } ) )
				_color.m_selected = !_color.m_selected;

			ImGui::SameLine();
			hovered = ImGui::IsItemHovered();

			ImGui::SetCursorPos( { ImGui::GetCursorPosX(), cursor_pos.y } );
			if( hovered )
			{
				shadow_min_pos = ImGui::GetCursorScreenPos() + ImVec2{ 2, 2 };
				ImGui_fzn::rect_filled( { shadow_min_pos, shadow_size }, ImGui_fzn::color::black );
			}

			ImGui::PushStyleColor( ImGuiCol_FrameBg, ImColor{ 34, 59, 92 }.Value );
			ImGui::Checkbox( fzn::Tools::Sprintf( "##checkbox_%s", color_name.c_str() ).c_str(), &_color.m_selected );
			ImGui::PopStyleColor();
		}
		else
		{
			//ImGui::Button( "-", { shadow_size.x, 0.f } );
			ImGui::PushFont( ImGui_fzn::s_ImGuiFormatOptions.m_pFontBold );
			ImGui_fzn::square_button( "-" );
			ImGui::PopFont();

			ImGui::SameLine();
			ImGui::SetCursorPosY( cursor_pos.y + 1 );
			if( ImGui::Selectable( fzn::Tools::Sprintf( "##selectable_%s", color_name.c_str() ).c_str(), false, ImGuiSelectableFlags_SpanAvailWidth, { 0, ImGui::GetTextLineHeightWithSpacing() } ) )
			{
				m_color_to_edit = &_color;
				m_edited_color = _color;
				ImGui::OpenPopup( "Edit color" );
			}

			hovered = ImGui::IsItemHovered();
		}

		ImGui::SameLine();

		ImGui::PushStyleColor( ImGuiCol_FrameBg, ImGui_fzn::color::white );
		if( hovered )
		{
			shadow_min_pos = ImGui::GetCursorScreenPos() + ImVec2{ 2, 2 };
			ImGui_fzn::rect_filled( { shadow_min_pos, shadow_size }, ImGui_fzn::color::black );
		}
		ImGui::ColorButton( fzn::Tools::Sprintf( "##color_button_%s", color_name.c_str() ).c_str(), _color.m_color, ImGuiColorEditFlags_NoTooltip );

		if( ImGui::IsItemHovered() )
		{
			ImGui::BeginTooltip();
			bicolor_color_name( color_name, true );
			ImGui::Separator();

			ImGuiContext& g = *GImGui;
			
			ImVec2 sz( ImGui_fzn::s_ImGuiFormatOptions.m_pFontRegular->FontSize * 2 + g.Style.FramePadding.y * 2, ImGui_fzn::s_ImGuiFormatOptions.m_pFontRegular->FontSize * 2 + g.Style.FramePadding.y * 2 );
			sf::Color sf_color{ Utils::to_sf_color( _color.m_color ) };
			ImGui::ColorButton( "##preview", _color.m_color, ImGuiColorEditFlags_NoTooltip, sz );
			ImGui::SameLine();
			ImGui::Text( "#%02X%02X%02X\nR:%d, G:%d, B:%d", sf_color.r, sf_color.g, sf_color.b, sf_color.r, sf_color.g, sf_color.b );

			ImGui::EndTooltip();
		}

		ImGui::PopStyleColor();
		ImGui::SameLine();

		ImGui::AlignTextToFramePadding();

		if( hovered )
		{
			auto test_cursor_pos{ ImGui::GetCursorPos() };
			ImGui::SetCursorPos( { test_cursor_pos.x + 2, test_cursor_pos.y + 2 } );
			ImGui_fzn::bold_text_colored( ImGui_fzn::color::black, color_name );
			ImGui::SameLine();
			ImGui::SetNextItemAllowOverlap();
			ImGui::SetCursorPos( test_cursor_pos );
			bicolor_color_name( color_name, true );

		}
		else
			bicolor_color_name( color_name.c_str(), false );
	}

	void PalettesManager::_edit_color()
	{
		ImGui::SetNextWindowSize( { 500.f, 500.f } );

		const auto window_size = g_pFZN_WindowMgr->GetWindowSize();
		ImGui::SetNextWindowPos( { window_size.x * 0.5f - 250.f, window_size.y * 0.5f - 250.f }, ImGuiCond_Appearing );

		if( ImGui::Begin( "Edit color", nullptr, ImGuiWindowFlags_::ImGuiWindowFlags_NoDocking ) )
		{
			std::string color_id{ fzn::Tools::Sprintf( "%d", m_edited_color.m_id ) };
			if( ImGui::InputText( "ID", &color_id, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll ) )
				m_edited_color.m_id = std::stoi( color_id );
			ImGui::InputText( "Name", &m_edited_color.m_name, ImGuiInputTextFlags_AutoSelectAll );
			//ImGui::ColorPicker4( "color", &m_edited_color.m_color.Value.x, ImGuiColorEditFlags_NoInputs, &m_edited_color.m_color.Value.x );
			ImGui::ColorPicker4( "##color", &m_edited_color.m_color.Value.x, ImGuiColorEditFlags_NoAlpha, m_color_to_edit != nullptr ? &m_color_to_edit->m_color.Value.x : nullptr );


			if( ImGui::Button( "Confirm" ) && m_edited_color.is_valid() )
			{
				if( m_color_to_edit != nullptr )
					*m_color_to_edit = m_edited_color;
				else if( m_selected_palette != nullptr )
					m_selected_palette->m_colors.push_back( m_edited_color );

				_reset_color_to_edit();
			}

			ImGui::SameLine();

			if( ImGui::Button( "Cancel" ) )
				_reset_color_to_edit();
		}

		ImGui::End();
	}

	std::string PalettesManager::_get_presets_from_color_index( ColorPalette* _palette, uint32_t _color_index )
	{
		if( _palette == nullptr || _color_index >= _palette->m_colors.size() )
			return {};

		std::string result;

		for( auto& preset : _palette->m_presets )
		{
			if( preset.first == "All" )
				continue;

			if( std::ranges::find( preset.second, _color_index ) != preset.second.end() )
				result += preset.first + ",";
		}

		if( result.empty() == false )
			result.erase( result.size() - 1 );

		return result;
	}

	std::string PalettesManager::_get_palette_root_path( const std::string& _path )
	{
		auto diff_pos{ _path.find_first_not_of( m_app_palettes_path ) };

		if( diff_pos == std::string::npos )
			return {};

		return _path.substr( diff_pos + 1 );
	}

} // namespace PerlerMaker