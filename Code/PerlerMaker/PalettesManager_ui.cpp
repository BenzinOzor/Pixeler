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

			ImGui::SameLine();
			if( m_palette_edition )
			{
				if( ImGui::Button( "Save Palette" ) )
					_save_palette();
			}
			else
			{
				if( ImGui::Button( "Add Palette" ) )
					_create_new_palette();
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
			if( ImGui_fzn::deactivable_button( "Save Preset", m_selected_preset == preset_all ) )
			{
				if( _update_preset() )
					_save_palette();
			}
			ImGui::SameLine();
			ImGui_fzn::deactivable_button( "Save Preset As...", m_selected_preset == preset_all );

			if( m_selected_palette == nullptr )
				ImGui::PopItemFlag();


			ImGui::EndTable();
		}

		ImGui::Separator();
		ImGui_fzn::Filter( m_color_filter, "Search color by name or ID" );

		ImGui::Checkbox( "Display only used colors", &m_only_used_colors_display );

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

				ImGui::SameLine();
				ImGui::Text( "Add New Color" );
			}
		}

		ImGui::EndChild();
	}

	void PalettesManager::_selectable_color_info( ColorInfos& _color )
	{
		if( match_filter( _color ) == false || m_only_used_colors_display && _color.m_count == 0 )
			return;

		const std::string color_name{ _color.m_count > 0 ? fzn::Tools::Sprintf( "%s (%d)", _color.get_full_name().c_str(), _color.m_count ) : _color.get_full_name() };
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

			if( ImGui::IsItemHovered() )
			{
				ImGui::SetTooltip( "Check to use this color." );
			}
		}
		else
		{
			ImGui::PushFont( ImGui_fzn::s_ImGuiFormatOptions.m_pFontBold );
			ImGui_fzn::square_button( "-" );
			ImGui::PopFont();

			ImGui::SameLine();
			ImGui::SetCursorPosY( cursor_pos.y + 1 );
			if( ImGui::Selectable( fzn::Tools::Sprintf( "##selectable_%s", color_name.c_str() ).c_str(), false, ImGuiSelectableFlags_SpanAvailWidth, { 0, ImGui::GetTextLineHeightWithSpacing() } ) )
			{
				m_color_to_edit = &_color;
				m_edited_color = _color;
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
			auto style = ImGui::GetStyle();
			Utils::color_infos_tooltip_common( _color );

			ImGui::Separator();
			ImGui::Text( "Total count: %d", _color.m_count );

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
			Utils::bicolor_color_name( color_name, true, _color.m_count != 0 );

		}
		else
			Utils::bicolor_color_name( color_name.c_str(), false, _color.m_count != 0 );
	}

	void PalettesManager::_edit_color()
	{
		if( m_edited_color.is_valid( true ) )
			ImGui::OpenPopup( "Edit Color" );
		else
			return;

		static constexpr ImVec2	popup_size{ 400.f, 0.f };
		static constexpr float	color_preview_width{ 70.f };
		ImGui::SetNextWindowSize( popup_size );

		const auto window_size = g_pFZN_WindowMgr->GetWindowSize();
		const ImVec2 window_pos{ window_size.x * 0.5f - popup_size.x * 0.5f, window_size.y * 0.5f - popup_size.x * 0.5f };
		ImGui::SetNextWindowPos( window_pos, ImGuiCond_Appearing );

		const float widget_with{ popup_size.x - ImGui::GetStyle().WindowPadding.x * 2.f - color_preview_width };

		if( ImGui::BeginPopupModal( "Edit Color", nullptr, ImGuiWindowFlags_::ImGuiWindowFlags_NoDocking ) )
		{
			std::string color_id{ fzn::Tools::Sprintf( "%d", m_edited_color.m_id ) };
			ImGui::SetNextItemWidth( widget_with );
			if( ImGui::InputText( "ID", &color_id, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll ) )
				m_edited_color.m_id = std::stoi( color_id );
			ImGui::SetNextItemWidth( widget_with );
			ImGui::InputText( "Name", &m_edited_color.m_name, ImGuiInputTextFlags_AutoSelectAll );
			ImGui::Spacing();
			ImGui::SetNextItemWidth( widget_with );
			ImGui::ColorPicker4( "##color", &m_edited_color.m_color.Value.x, ImGuiColorEditFlags_NoAlpha, m_color_to_edit != nullptr ? &m_color_to_edit->m_color.Value.x : nullptr );

			auto confirm = [&]()
			{
				if( m_color_to_edit != nullptr )
					*m_color_to_edit = m_edited_color;
				else if( m_selected_palette != nullptr )
					m_selected_palette->m_colors.push_back( m_edited_color );

				_reset_color_to_edit();
			};

			Utils::window_bottom_confirm_cancel( m_edited_color.is_valid(), confirm, [&](){ _reset_color_to_edit(); } );

			ImGui::EndPopup();
		}
	}

	void PalettesManager::_new_palette_popup()
	{
		if( m_new_palette )
			ImGui::OpenPopup( "New Palette" );
		else
			return;

		static constexpr ImVec2	popup_size{ 400.f, 0.f };
		ImGui::SetNextWindowSize( popup_size );

		const auto window_size = g_pFZN_WindowMgr->GetWindowSize();
		const ImVec2 window_pos{ window_size.x * 0.5f - popup_size.x * 0.5f, window_size.y * 0.5f - popup_size.x * 0.5f };
		ImGui::SetNextWindowPos( window_pos, ImGuiCond_Appearing );

		bool is_editing_text{ false };

		if( ImGui::BeginPopupModal( "New Palette" ) )
		{
			if( ImGui::BeginTable( "Palette Infos", 2 ) )
			{
				ImGui::TableSetupColumn( "Checkbox", ImGuiTableColumnFlags_WidthFixed );
				ImGui::TableSetupColumn( "InputTexts", ImGuiTableColumnFlags_WidthStretch );

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex( 1 );

				if( ImGui::InputText( "Palette Name", &m_selected_palette->m_name ) )
				{
					if( m_new_palette_infos.m_file_name_same_as_palette )
						m_new_palette_infos.m_file_name = m_selected_palette->m_name;
				}
				is_editing_text = ImGui::IsItemActive();

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex( 0 );

				if( ImGui::Checkbox( "##SameName", &m_new_palette_infos.m_file_name_same_as_palette ) )
				{
					if( m_new_palette_infos.m_file_name_same_as_palette )
						m_new_palette_infos.m_file_name = m_selected_palette->m_name;
				}

				ImGui_fzn::simple_tooltip_on_hover( "Same name for palette and file" );

				ImGui::TableSetColumnIndex( 1 );

				if( m_new_palette_infos.m_file_name_same_as_palette )
					ImGui::BeginDisabled();
				ImGui::InputText( "Palette File Name", &m_new_palette_infos.m_file_name );
				is_editing_text |= ImGui::IsItemActive();
				if( m_new_palette_infos.m_file_name_same_as_palette )
					ImGui::EndDisabled();

				ImGui::EndTable();
			}

			auto confirm = [&]()
			{
				if( m_new_palette_infos.m_file_name_same_as_palette )
					m_new_palette_infos.m_file_name = m_selected_palette->m_name;

				m_selected_palette->m_file_path = m_new_palette_infos.m_file_name + ".xml";
				m_new_palette = false;
			};

			auto cancel = [&]()
			{
				std::string palette_name{ m_selected_palette->m_name };
				std::erase_if( m_palettes, [&palette_name]( const auto& _palette ) { return _palette.first == palette_name; } );

				if( m_palettes.empty() )
					m_selected_palette = nullptr;
				else
					m_selected_palette = &m_palettes.begin()->second;

				m_new_palette = false;
				m_palette_edition = false;
			};

			Utils::window_bottom_confirm_cancel( m_selected_palette->m_name.empty() == false && is_editing_text == false, confirm, cancel );

			ImGui::EndPopup();
		}
	}
};