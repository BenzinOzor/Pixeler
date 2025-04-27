#include <FZN/Managers/FazonCore.h>
#include <FZN/Tools/Logging.h>
#include <FZN/Tools/Tools.h>
#include <FZN/Tools/Math.h>

#include "PalettesManager.h"
#include "Utils.h"
#include "PerlerMaker.h"

namespace PerlerMaker
{
	static constexpr ImColor table_row_selected{ 24, 49, 82 };
	static constexpr ImColor checkbox_color{ 34, 59, 92 };
	static constexpr ImColor checkbox_color_hovered{ 44, 69, 102 };
	static constexpr ImColor checkbox_color_active{ 54, 79, 112 };

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
						_compute_ID_column_size( false );
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
		if( m_ID_column_width <= 0.f )
			_compute_ID_column_size( false );

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

			if( ImGui::BeginTable( "Colors", 5, ImGuiTableFlags_ScrollY ) )
			{
				ImGui::TableSetupColumn( "##Checkbox", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFrameHeightWithSpacing() );
				ImGui::TableSetupColumn( "##ColorButton", ImGuiTableColumnFlags_WidthFixed );
				ImGui::TableSetupColumn( "ID", ImGuiTableColumnFlags_WidthFixed, m_ID_column_width );
				ImGui::TableSetupColumn( "Name", ImGuiTableColumnFlags_WidthStretch );
				ImGui::TableSetupColumn( "Count  ", ImGuiTableColumnFlags_WidthFixed );
				ImGui::TableSetupScrollFreeze( 0, 1 );
				ImGui::TableHeadersRow();

				int current_row{ 1 };
				for( auto& color : m_selected_palette->m_colors )
				{
					if( _selectable_color_info( color, current_row ) )
					++current_row;
				}

				if( m_palette_edition )
				{
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex( 3 );

					if( ImGui::Button( "Add New Color", { ImGui::GetContentRegionAvail().x, 0.f } ) )
						m_edited_color = ColorInfos{ "", -1, ImGui_fzn::color::black };

					ImGui::SameLine();
				}

				ImGui::EndTable();
			}
		}

		ImGui::EndChild();
	}

	bool PalettesManager::_selectable_color_info( ColorInfos& _color, int _current_row )
	{
		if( match_filter( _color ) == false || m_only_used_colors_display && _color.m_count == 0 )
			return false;

		ImGui::PushID( _color.get_full_name().c_str() );
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex( 0 );

		const bool row_hovered{ ImGui::TableGetHoveredRow() == _current_row };
		const ImVec2 shadow_offset{ 2.f, 2.f };
		const ImVec2 square_shadow_size{ ImGui::GetFrameHeight(), ImGui::GetFrameHeight() };
		bool first_column_clicked{ false };

		//////////////////////////////////////// CHECKBOX ////////////////////////////////////////
		ImGui::SameLine( 0.f, ImGui::GetStyle().CellPadding.x );

		if( row_hovered )
		{
			const ImVec2 shadow_min_pos{ ImGui::GetCursorScreenPos() + shadow_offset };
			ImGui_fzn::rect_filled( { shadow_min_pos, square_shadow_size }, ImGui_fzn::color::black );
		}

		if( m_palette_edition == false )
		{
			ImGui::PushStyleColor( ImGuiCol_FrameBg, checkbox_color.Value );
			ImGui::PushStyleColor( ImGuiCol_FrameBgHovered, checkbox_color_hovered.Value );
			ImGui::PushStyleColor( ImGuiCol_FrameBgActive, checkbox_color_active.Value );

			if( ImGui::Checkbox( "##selected", &_color.m_selected ) )
				first_column_clicked = true;

			ImGui::PopStyleColor(3);
			if( ImGui::TableGetHoveredColumn() == 0 )
			{
				ImGui::SetTooltip( "Checked if the color is used in the conversion\nClick to toggle this color" );
			}
		}
		else
		{
			ImGui::PushStyleColor( ImGuiCol_Button, checkbox_color.Value );
			ImGui::PushStyleColor( ImGuiCol_ButtonHovered, checkbox_color_hovered.Value );
			ImGui::PushStyleColor( ImGuiCol_ButtonActive, checkbox_color_active.Value );
			ImGui::PushFont( ImGui_fzn::s_ImGuiFormatOptions.m_pFontBold );
			if( ImGui_fzn::square_button( "-" ) )
			{
				std::erase_if( m_selected_palette->m_colors, [&_color]( const auto& _current_color ) { return _current_color == _color; } );
				_compute_ID_column_size( true );
				first_column_clicked = true;
			}
			ImGui::PopFont();
			ImGui::PopStyleColor( 3 );

			if( ImGui::TableGetHoveredColumn() == 0 )
				ImGui::SetTooltip( "Remove from palette" );
		}

		//////////////////////////////////////// COLOR BUTTON ////////////////////////////////////////
		ImGui::TableSetColumnIndex( 1 );

		if( row_hovered )
		{
			const ImVec2 shadow_min_pos{ ImGui::GetCursorScreenPos() + shadow_offset };
			ImGui_fzn::rect_filled( { shadow_min_pos, square_shadow_size }, ImGui_fzn::color::black );
		}

		ImGui::PushStyleColor( ImGuiCol_FrameBg, ImGui_fzn::color::white );
		ImGui::ColorButton( "##color_button", _color.m_color, ImGuiColorEditFlags_NoTooltip );
		ImGui::PopStyleColor();

		if( ImGui::IsItemHovered() )
		{
			ImGui::BeginTooltip();
			Utils::color_infos_tooltip_common( _color );

			if( _color.m_count >= 0 )
			{
				ImGui::Separator();
				ImGui::Text( "Total count: %d", _color.m_count );
			}
			ImGui::Separator();
			if( m_palette_edition )
				ImGui::Text( "Click to edit this color" );
			else
				ImGui::Text( "Click to toggle this color" );
			ImGui::EndTooltip();
		}

		//////////////////////////////////////// ID ////////////////////////////////////////
		if( _color.m_id >= 0 )
		{
			ImGui::TableSetColumnIndex( 2 );
			ImGui::AlignTextToFramePadding();
			Utils::text_with_leading_zeros( Utils::get_zero_lead_id( _color.m_id ).c_str(), row_hovered, _color.m_count != 0, row_hovered );
		}

		//////////////////////////////////////// NAME ////////////////////////////////////////
		ImGui::TableSetColumnIndex( 3 );
		ImGui::AlignTextToFramePadding();

		Utils::boldable_text( _color.m_name, row_hovered, _color.m_count != 0, row_hovered );

		//////////////////////////////////////// COUNT ////////////////////////////////////////
		ImGui::TableSetColumnIndex( 4 );
		if( _color.m_count > 0 )
		{
			Utils::boldable_text( fzn::Tools::Sprintf( "%d", _color.m_count ), row_hovered, true, row_hovered );
		}

		//////////////////////////////////////// MISC ////////////////////////////////////////
		if( row_hovered )
		{
			ImGuiContext* context = ImGui::GetCurrentContext();
			if( ImGuiTable * current_table{ context->CurrentTable } )
				current_table->RowBgColor[ 1 ] = ImGui::GetColorU32( ImGuiCol_HeaderHovered );

			if( first_column_clicked == false && ImGui::IsMouseReleased( ImGuiMouseButton_Left ) )
			{
				if( m_palette_edition )
				{
					m_color_to_edit = &_color;
					m_edited_color = _color;
				}
				else
					_color.m_selected = !_color.m_selected;
			}
		}

		ImGui::PopID();

		return true;
	}

	void PalettesManager::_edit_color()
	{
		std::string popup_title{ m_color_to_edit != nullptr ? "Edit Color" : "Add Color" };

		if( m_edited_color.is_valid( true ) )
			ImGui::OpenPopup( popup_title.c_str() );
		else
			return;

		static constexpr ImVec2	popup_size{ 500.f, 0.f };
		static constexpr float	color_preview_width{ 70.f };
		ImGui::SetNextWindowSize( popup_size );

		const auto window_size = g_pFZN_WindowMgr->GetWindowSize();
		const ImVec2 window_pos{ window_size.x * 0.5f - popup_size.x * 0.5f, window_size.y * 0.5f - popup_size.x * 0.5f };
		ImGui::SetNextWindowPos( window_pos, ImGuiCond_Appearing );

		const float widget_with{ popup_size.x - ImGui::GetStyle().WindowPadding.x * 2.f - color_preview_width };

		if( ImGui::BeginPopupModal( popup_title.c_str(), nullptr, ImGuiWindowFlags_::ImGuiWindowFlags_NoDocking ) )
		{
			std::string color_id{ fzn::Tools::Sprintf( "%d", m_edited_color.m_id ) };

			ImGui::Text( "Enter a valid ID and/or a name for your color" );

			ImGui::SetNextItemWidth( widget_with );
			if( ImGui::InputText( "ID", &color_id, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_AutoSelectAll ) )
			{
				if( color_id.empty() )
					m_edited_color.m_id = -1;
				else
					m_edited_color.m_id = std::stoi( color_id );
			}

			ImGui::SameLine();
			ImGui_fzn::helper_simple_tooltip( "ID isn't required.\n-1 will show no ID." );

			ImGui::SetNextItemWidth( widget_with );
			ImGui::InputText( "Name", &m_edited_color.m_name, ImGuiInputTextFlags_AutoSelectAll );
			ImGui::Spacing();
			ImGui::SetNextItemWidth( widget_with );
			ImGui::ColorPicker4( "##color", &m_edited_color.m_color.Value.x, ImGuiColorEditFlags_NoAlpha, m_color_to_edit != nullptr ? &m_color_to_edit->m_color.Value.x : nullptr );

			if( m_color_to_edit != nullptr )
				_edit_color_buttons();
			else
				_add_color_buttons();

			ImGui::EndPopup();
		}
	}

	void PalettesManager::_edit_color_buttons()
	{
		Utils::window_bottom_table( 2, [&]()
		{
			ImGui::TableSetColumnIndex( 1 );

			if( ImGui_fzn::deactivable_button( "Apply", m_edited_color.is_valid(), true, DefaultWidgetSize ) )
			{
				*m_color_to_edit = m_edited_color;
				_reset_color_to_edit();
				_compute_ID_column_size( true );
			}

			ImGui_fzn::simple_tooltip_on_hover( "Add the current color to the palette and close this popup" );

			ImGui::TableSetColumnIndex( 2 );
			if( ImGui::Button( "Cancel", DefaultWidgetSize ) )
				_reset_color_to_edit();
		} );
	}

	void PalettesManager::_add_color_buttons()
	{
		Utils::window_bottom_table( 3, [&]()
		{
			const bool disable_buttons{ m_edited_color.is_valid() == false };
			if( ImGui_fzn::deactivable_button( "Add Another Color", disable_buttons, true, DefaultWidgetSize ) )
			{
				m_selected_palette->m_colors.push_back( m_edited_color );
				m_edited_color = ColorInfos{ "", -1, ImGui_fzn::color::black };
				_compute_ID_column_size( true );
			}
			ImGui_fzn::simple_tooltip_on_hover( "Add the current color to the palette and create another one without closing this popup" );

			ImGui::TableSetColumnIndex( 2 );
			if( ImGui_fzn::deactivable_button( "Add", disable_buttons, true, DefaultWidgetSize ) )
			{
				m_selected_palette->m_colors.push_back( m_edited_color );
				_reset_color_to_edit();
				_compute_ID_column_size( true );
			}

			ImGui_fzn::simple_tooltip_on_hover( "Add the current color to the palette and close this popup" );

			ImGui::TableSetColumnIndex( 3 );
			if( ImGui::Button( "Cancel", DefaultWidgetSize ) )
				_reset_color_to_edit();
		} );
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

			Utils::window_bottom_table( 2, [&]()
			{
				const bool disable_buttons{ m_selected_palette->m_name.empty() };
				if( ImGui_fzn::deactivable_button( "Apply", m_selected_palette->m_name.empty(), true, DefaultWidgetSize ) )
				{
					if( m_new_palette_infos.m_file_name_same_as_palette )
						m_new_palette_infos.m_file_name = m_selected_palette->m_name;

					m_selected_palette->m_file_path = m_new_palette_infos.m_file_name + ".xml";
					m_new_palette = false;
				}

				ImGui::TableSetColumnIndex( 2 );
				if( ImGui::Button( "Cancel", DefaultWidgetSize ) )
				{
					std::string palette_name{ m_selected_palette->m_name };
					std::erase_if( m_palettes, [&palette_name]( const auto& _palette ) { return _palette.first == palette_name; } );

					if( m_palettes.empty() )
						m_selected_palette = nullptr;
					else
					{
						m_selected_palette = &m_palettes.begin()->second;
						_compute_ID_column_size( false );
					}

					m_new_palette = false;
					m_palette_edition = false;
				}
			} );

			ImGui::EndPopup();
		}
	}

	void PalettesManager::_compute_ID_column_size( bool _compute_palette_infos )
	{
		if( m_selected_palette == nullptr )
			return;

		if( _compute_palette_infos )
			Utils::compute_IDs_and_names_usage_infos( *m_selected_palette );

		// We calculate the size of 1 zero, and we'll multiply it by the number of character in the highest ID + 1.
		const float zero_size{ ImGui::CalcTextSize( "0" ).x };
		m_ID_column_width = zero_size * m_selected_palette->m_nb_digits_in_IDs + 1;
	}
};