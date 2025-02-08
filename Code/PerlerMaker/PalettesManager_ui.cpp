#include <FZN/Managers/FazonCore.h>
#include <FZN/Tools/Logging.h>
#include <FZN/Tools/Tools.h>

#include "PalettesManager.h"
#include "Utils.h"

namespace PerlerMaker
{
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
};