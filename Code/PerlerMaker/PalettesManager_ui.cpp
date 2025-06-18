#include <FZN/Managers/FazonCore.h>
#include <FZN/Managers/DataManager.h>
#include <FZN/Tools/Logging.h>
#include <FZN/Tools/Tools.h>
#include <FZN/Tools/Math.h>

#include "PalettesManager.h"
#include "Utils.h"
#include "PerlerMaker.h"

namespace PerlerMaker
{
	/************************************************************************
	* Custom ImGui colors for various elements of the window.
	************************************************************************/
	static constexpr ImColor table_row_selected			{ 29, 46, 63 };
	static constexpr ImColor table_row_not_selected		{ 24, 37, 51 };
	static constexpr ImColor checkbox_color				{ 34, 59, 92 };
	static constexpr ImColor checkbox_color_hovered		{ 44, 69, 102 };
	static constexpr ImColor checkbox_color_active		{ 54, 79, 112 };

	/**
	* @brief Header of the palettes management window. Handles palettes and presets selections, edition, addition, removal. Aswell as colors filtering and selection options.
	**/
	void PalettesManager::_header()
	{
		// @todo Break down header function in multple smaller functions, this is over 150 lines long.
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

			if( m_palette_edition )
				ImGui::BeginDisabled();

			// Combo box displaying all the palettes retrieved from the My Document xml files.
			if( ImGui::BeginCombo( "##Palette", m_selected_palette != nullptr ? m_selected_palette->m_name.c_str() : "> select <" ) )
			{
				for( auto& palette : m_palettes )
				{
					if( ImGui_fzn::bold_selectable( palette.m_name.c_str(), m_selected_palette != nullptr ? m_selected_palette->m_name == palette.m_name : false ) )
					{
						_select_palette( palette );
					}
					if( ImGui::IsItemHovered() )
					{
						if( ImGui::BeginTooltip() )
						{
							ImGui_fzn::bold_text( palette.m_name );
							ImGui::Separator();

							ImGui::Text( "Colors:" );
							ImGui::SameLine();
							ImGui_fzn::bold_text( "%d", palette.m_colors.size() );

							ImGui::Text( "Presets:" );
							ImGui::SameLine();
							ImGui_fzn::bold_text( "%d", palette.m_presets.size() );

							ImGui::Text( "File:" );
							ImGui::SameLine();
							ImGui_fzn::bold_text( "%s", palette.m_file_path.c_str() );

							ImGui::EndTooltip();
						}
					}
				}

				ImGui::EndCombo();
			}

			if( m_palette_edition )
				ImGui::EndDisabled();

			ImGui::TableNextColumn();

			_palette_hamburger_menu();

			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text( "Preset" );

			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth( -1.f );

			auto preset_combo_preview = std::string{ "> select <" };
			bool disable_combo{ false };

			if( m_selected_palette == nullptr )
			{
				disable_combo = true;
				preset_combo_preview = "select a palette first";
			}
			else
			{
				if( m_palette_edition )
					disable_combo = true;

				if( m_selected_preset != nullptr && m_selected_preset->m_name.empty() == false )
					preset_combo_preview = m_selected_preset->m_name;

				if( m_selected_palette->m_colors.empty() || m_selected_palette->m_presets.empty() )
				{
					disable_combo = true;
					preset_combo_preview = "no presets";
				}
			}

			if( disable_combo )
				ImGui::BeginDisabled();

			// Combo box displaying all the presets available in the current palette.
			if( ImGui::BeginCombo( "##Preset", preset_combo_preview.c_str() ) )
			{
				for( auto& preset : m_selected_palette->m_presets )
				{
					if( ImGui_fzn::bold_selectable( preset.m_name.c_str(), m_selected_preset != nullptr ? preset.m_name == m_selected_preset->m_name : false ) )
					{
						_select_preset( &preset );
					}
					if( ImGui::IsItemHovered() )
					{
						if( ImGui::BeginTooltip() )
						{
							ImGui_fzn::bold_text( preset.m_name );
							ImGui::Separator();

							ImGui::Text( "Colors:" );
							ImGui::SameLine();
							ImGui_fzn::bold_text( "%d", preset.m_colors.size() );

							ImGui::EndTooltip();
						}
					}
				}

				ImGui::EndCombo();
			}

			ImGui::TableNextColumn();
			_preset_hamburger_menu();

			if( disable_combo )
				ImGui::EndDisabled();

			ImGui::EndTable();
		}

		ImGui::Separator();
		ImGui_fzn::Filter( m_color_filter, "Search color by name or ID" );

		ImGui::Checkbox( "Display only used colors", &m_only_used_colors_display );
		ImGui_fzn::simple_tooltip_on_hover( "Display only colors used in the image convertion" );

		const bool disable_buttons{ m_palette_edition || m_selected_palette == nullptr || m_selected_palette->m_colors.empty() };
		if( disable_buttons )
			ImGui::BeginDisabled();

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

		if( disable_buttons )
			ImGui::EndDisabled();
	}

	/**
	* @brief Palette option menu. Allows the user to add, remove or edit the current palette.
	**/
	void PalettesManager::_palette_hamburger_menu()
	{
		ImGui::PushID( "PaletteHamburgerMenu" );
		const ImVec2 cursor_pos{ ImGui::GetCursorPos() };

		if( sf::Texture* texture = g_pFZN_DataMgr->GetTexture( "HamburgerIcon" ) )
		{
			if( ImGui::ImageButton( *texture, 6 ) )
				ImGui::OpenPopup( "Palette Hamburger" );
		}
		else
		{
			if( ImGui_fzn::square_button( "##PaletteHamburgerMenu" ) )
				ImGui::OpenPopup( "Palette Hamburger" );
		}

		auto set_edition = [&]( bool _state )
		{
			m_palette_edition = _state;
			_reset_color_to_edit();
		};

		bool save_as_popup( false );

		if( ImGui::BeginPopup( "Palette Hamburger" ) )
		{
			const bool is_editing{ m_palette_edition };

			if( m_palette_edition )
			{
				if( ImGui::MenuItem( "Cancel Edition" ) )
				{
					_restore_backup_palette();
					set_edition( false );
				}
				if( ImGui::MenuItem( "Save" ) )
				{
					set_edition( false );
					m_backup_palette = ColorPalette{};
					_save_palette();
				}
				if( m_selected_palette != nullptr && ImGui::MenuItem( "Save As..." ) )
				{
					_create_new_palette( m_selected_palette );
					m_new_palette_infos.m_restore_backup_palette = true;
					set_edition( false );
				}
			}
			else
			{
				if( ImGui::MenuItem( "Create New" ) )
				{
					_create_new_palette();
					set_edition( true );
				}

				if( m_selected_palette != nullptr )
				{
					if( ImGui::MenuItem( "Edit" ) )
					{
						m_backup_palette = *m_selected_palette;
						set_edition( true );
					}
					if( ImGui::MenuItem( "Duplicate" ) )
					{
						_create_new_palette( m_selected_palette );
						m_new_palette_infos.m_set_new_palette_as_backup = true;
						set_edition( true );
					}
					ImGui::Separator();
					ImGui::PushStyleColor( ImGuiCol_HeaderHovered, ImGui_fzn::color::light_red );
					if( ImGui::MenuItem( "Delete" ) )
					{
						_delete_palette();
					}
					ImGui::PopStyleColor();
				}
			}

			ImGui::EndPopup();
		}
		ImGui::PopID();
	}

	/**
	* @brief Preset option menu. Allows the user to add and remove presets.
	**/
	void PalettesManager::_preset_hamburger_menu()
	{
		ImGui::PushID( "PresetHamburgerMenu" );
		if( sf::Texture* texture = g_pFZN_DataMgr->GetTexture( "HamburgerIcon" ) )
		{
			if( ImGui::ImageButton( *texture, 6 ) )
				ImGui::OpenPopup( "Preset Hamburger" );
		}
		else
		{
			if( ImGui_fzn::square_button( "##PresetHamburgerMenu" ) )
				ImGui::OpenPopup( "Preset Hamburger" );
		}

		if( ImGui::BeginPopup( "Preset Hamburger" ) )
		{
			if( ImGui::MenuItem( "Create New" ) )
			{
				_create_new_preset( false );
			}
			if( _is_preset_editable() && ImGui::MenuItem( "Save" ) )
			{
				_update_preset_colors_from_selection();
				_save_palette();
			}
			if( ImGui::MenuItem( "Save As..." ) )
			{
				_create_new_preset( true );
			}
			if( ImGui::MenuItem( "Duplicate" ) )
			{
				_create_new_preset_from_current();
			}
			ImGui::Separator();
			ImGui::PushStyleColor( ImGuiCol_HeaderHovered, ImGui_fzn::color::light_red );
			if( _is_preset_editable() && ImGui::MenuItem( "Delete" ) )
			{
				_delete_preset();
			}
			ImGui::PopStyleColor();

			ImGui::EndPopup();
		}
		ImGui::PopID();
	}

	/**
	* @brief Setup the color list table. Columns are ajusted automatically depending on the palette settings (ID/name use) and user actions:
	*			- Converting an image will add the color count column.
	**/
	bool PalettesManager::_color_table_begin()
	{
		int nb_columns = 2;

		if( m_selected_palette == nullptr )
			return false;

		if( m_selected_palette->is_using_IDs() )
			++nb_columns;

		if( m_selected_palette->is_using_names() )
			++nb_columns;

		if( has_convertion_happened() )
			++nb_columns;

		if( ImGui::BeginTable( "Colors", nb_columns, ImGuiTableFlags_ScrollY ) )
		{
			ImGui::TableSetupColumn( "##Checkbox", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFrameHeightWithSpacing() );
			ImGui::TableSetupColumn( "##ColorButton", ImGuiTableColumnFlags_WidthFixed );

			if( m_selected_palette->is_using_IDs() )
				ImGui::TableSetupColumn( "ID", m_selected_palette->is_using_names() ? ImGuiTableColumnFlags_WidthFixed : ImGuiTableColumnFlags_WidthStretch, m_ID_column_width );

			if( m_selected_palette->is_using_names() )
				ImGui::TableSetupColumn( "Name", ImGuiTableColumnFlags_WidthStretch );

			if( has_convertion_happened() )
				ImGui::TableSetupColumn( "Count  ", ImGuiTableColumnFlags_WidthFixed );

			ImGui::TableSetupScrollFreeze( 0, 1 );
			ImGui::TableHeadersRow();

			return true;
		}

		return false;
	}

	/**
	* @brief The body of the color list table, handling the parsing of all the colors in the palette and calling each selectable function.
	**/
	void PalettesManager::_colors_list()
	{
		if( m_ID_column_width <= 0.f )
			_compute_ID_column_size( false );

		ImGui::Separator();
		// Creating a child to ensure the scrolling zone begins under the header, keeping it on top.
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

			// The table will take all the available height in the window unless we're in edition mode because we want to leave room for the color addition button at the bottom of the window.
			ImVec2 table_size{ 0.f, ImGui::GetContentRegionAvail().y };
			if( m_palette_edition )
				table_size.y -= ImGui::GetFrameHeightWithSpacing();

			if( ImGui::BeginChild( "color_table", table_size ) )
			{
				if( _color_table_begin() )
				{
					int current_row{ 1 };
					for( auto& color : m_selected_palette->m_colors )
					{
						if( _selectable_color_info( color, current_row ) )
							++current_row;
					}

					ImGui::EndTable();
				}

				ImGui::EndChild();
			}

			if( m_palette_edition )
			{
				if( ImGui::Button( "Add New Color", { ImGui::GetContentRegionAvail().x, 0.f } ) )
					m_edited_color = ColorInfos{ "", -1, ImGui_fzn::color::black };

				ImGui::SameLine();
			}
		}

		ImGui::EndChild();
	}

	/**
	* @brief Setup the selectable for a given color. Will display various informations depending on edition mode and if a convertion has been done.
	**/
	bool PalettesManager::_selectable_color_info( ColorInfos& _color, int _current_row )
	{
		if( _match_filter( _color ) == false || m_only_used_colors_display && _color.m_count == 0 )
			return false;

		int current_column{ 0 };

		ImGui::PushID( _color.get_full_name().c_str() );
		ImGui::TableNextRow();

		const bool row_hovered{ ImGui::TableGetHoveredRow() == _current_row };
		const ImVec2 shadow_offset{ 2.f, 2.f };
		const ImVec2 square_shadow_size{ ImGui::GetFrameHeight(), ImGui::GetFrameHeight() };
		bool first_column_clicked{ false };

		ImGuiContext* context = ImGui::GetCurrentContext();
		ImGuiTable * current_table{ context->CurrentTable };

		if( current_table == nullptr )
		{
			ImGui::PopID();
			return false;
		}

		//////////////////////////////////////// CHECKBOX ////////////////////////////////////////
		ImGui::TableSetColumnIndex( current_column++ );
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
		ImGui::TableSetColumnIndex( current_column++ );

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
		if( m_selected_palette->is_using_IDs() )
		{
			ImGui::TableSetColumnIndex( current_column++ );

			// Using IDs doesn't insure having a valid one, se we test it nonetheless.
			if( _color.m_color_id.m_id >= 0 )
			{
				ImGui::AlignTextToFramePadding();
				Utils::text_with_leading_zeros( Utils::get_zero_lead_id( _color.m_color_id.m_id ).c_str(), row_hovered, _color.m_count != 0, row_hovered );
			}
		}

		//////////////////////////////////////// NAME ////////////////////////////////////////
		if( m_selected_palette->is_using_names() )
		{
			ImGui::TableSetColumnIndex( current_column++ );
			ImGui::AlignTextToFramePadding();

			Utils::boldable_text( _color.m_color_id.m_name, row_hovered, _color.m_count != 0, row_hovered );
		}

		//////////////////////////////////////// COUNT ////////////////////////////////////////
		if( has_convertion_happened() )
		{
			ImGui::TableSetColumnIndex( current_column++ );
			Utils::boldable_text( fzn::Tools::Sprintf( "%d", _color.m_count ), row_hovered, true, row_hovered );
		}

		//////////////////////////////////////// MISC ////////////////////////////////////////
		if( row_hovered )
		{
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
		else if( _color.m_selected )
			current_table->RowBgColor[ 1 ] = table_row_selected;

		ImGui::PopID();

		return true;
	}
	
	/**
	* @brief Check if there is a valid color to edit and open the corresponding popup. This serves for color edition and addition.
	**/
	void PalettesManager::_edit_color()
	{
		std::string popup_title{ m_color_to_edit != nullptr ? "Edit Color" : "Add Color" };

		if( m_edited_color.is_valid( true ) )
			ImGui::OpenPopup( popup_title.c_str() );
		else
			return;

		// Setting the width manually but the height will be automatically set by ImGui according to content.
		static constexpr ImVec2	popup_size{ 500.f, 0.f };
		static constexpr float	color_preview_width{ 70.f };
		ImGui::SetNextWindowSize( popup_size );

		const auto window_size = g_pFZN_WindowMgr->GetWindowSize();
		// Height of the popup isn't fixed, so we use its width to center it. This won't be too accurate but it's not that important.
		const ImVec2 window_pos{ window_size.x * 0.5f - popup_size.x * 0.5f, window_size.y * 0.5f - popup_size.x * 0.5f };
		ImGui::SetNextWindowPos( window_pos, ImGuiCond_Appearing );

		const float widget_with{ popup_size.x - ImGui::GetStyle().WindowPadding.x * 2.f - color_preview_width };

		if( ImGui::BeginPopupModal( popup_title.c_str(), nullptr, ImGuiWindowFlags_::ImGuiWindowFlags_NoDocking ) )
		{
			std::string color_id{ fzn::Tools::Sprintf( "%d", m_edited_color.m_color_id.m_id ) };

			ImGui::Text( "Enter a valid ID (>= 0) and/or a name for your color" );

			ImGui::SetNextItemWidth( widget_with );
			if( ImGui::InputText( "ID", &color_id, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_AutoSelectAll ) )
			{
				if( color_id.empty() )
					m_edited_color.m_color_id.m_id = -1;
				else
					m_edited_color.m_color_id.m_id = std::stoi( color_id );
			}

			ImGui::SameLine();
			ImGui_fzn::helper_simple_tooltip( "ID isn't required.\n-1 will show no ID." );

			ImGui::SetNextItemWidth( widget_with );
			ImGui::InputText( "Name", &m_edited_color.m_color_id.m_name, ImGuiInputTextFlags_AutoSelectAll );

			ImGui::SameLine();
			ImGui_fzn::helper_simple_tooltip( "Name isn't required.\nLeave blank to use only ID." );

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

	/**
	* @brief Display the available buttons when editing a color.
	**/
	void PalettesManager::_edit_color_buttons()
	{
		Utils::window_bottom_table( 2, [&]()
		{
			ImGui::TableSetColumnIndex( 1 );

			// Apply all the edit to the selected color.
			if( ImGui_fzn::deactivable_button( "Apply", m_edited_color.is_valid() == false, true, DefaultWidgetSize ) )
			{
				*m_color_to_edit = m_edited_color;
				_reset_color_to_edit();
				_compute_ID_column_size( true );
			}

			ImGui_fzn::simple_tooltip_on_hover( "Add the current color to the palette and close this popup" );

			ImGui::TableSetColumnIndex( 2 );
			// Cancel the edit and go back to the color list.
			if( ImGui::Button( "Cancel", DefaultWidgetSize ) )
				_reset_color_to_edit();
		} );
	}

	/**
	* @brief Display the available buttons when adding a new color. There is one more button than in editing context, allowing the user to add another color without closing the popup.
	**/
	void PalettesManager::_add_color_buttons()
	{
		auto add_to_preset_all = [&]( const ColorID& _color_id )
		{
			ColorPreset* preset_all = _get_preset_all( m_selected_palette );

			if( preset_all != nullptr )
				preset_all->m_colors.push_back( _color_id );
		};

		Utils::window_bottom_table( 3, [&]()
		{
			const bool disable_buttons{ m_edited_color.is_valid() == false };
			// Add the current color to the list and setup a new color to add.
			if( ImGui_fzn::deactivable_button( "Add Another Color", disable_buttons, true, DefaultWidgetSize ) )
			{
				m_selected_palette->m_colors.push_back( m_edited_color );
				add_to_preset_all( m_edited_color.m_color_id );
				m_edited_color = ColorInfos{ "", -1, ImGui_fzn::color::black };
				_compute_ID_column_size( true );
			}
			ImGui_fzn::simple_tooltip_on_hover( "Add the current color to the palette and create another one without closing this popup" );

			ImGui::TableSetColumnIndex( 2 );
			// Add the current color to the list.
			if( ImGui_fzn::deactivable_button( "Add", disable_buttons, true, DefaultWidgetSize ) )
			{
				m_selected_palette->m_colors.push_back( m_edited_color );
				add_to_preset_all( m_edited_color.m_color_id );
				// Adding new color to preset All.
				if( ColorPreset* preset_all{ _get_preset_all() } )
					preset_all->m_colors.push_back( m_edited_color.m_color_id );

				_reset_color_to_edit();
				_compute_ID_column_size( true );
			}

			ImGui_fzn::simple_tooltip_on_hover( "Add the current color to the palette and close this popup" );

			ImGui::TableSetColumnIndex( 3 );
			// Cancel the addition and go back to the color list.
			if( ImGui::Button( "Cancel", DefaultWidgetSize ) )
				_reset_color_to_edit();
		} );
	}

	/**
	* @brief Palette creation popup allowing to set its name and file name. The user can chose to set the same name for the palette and its file or to have different names.
	**/
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

		if( ImGui::BeginPopupModal( "New Palette" ) )
		{
			if( ImGui::BeginTable( "Palette Infos", 2 ) )
			{
				ImGui::TableSetupColumn( "Checkbox", ImGuiTableColumnFlags_WidthFixed );
				ImGui::TableSetupColumn( "InputTexts", ImGuiTableColumnFlags_WidthStretch );

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex( 1 );

				if( ImGui::InputText( "Palette Name", &m_new_palette_infos.m_name ) )
				{
					if( m_new_palette_infos.m_file_name_same_as_palette )
						m_new_palette_infos.m_file_name = m_new_palette_infos.m_name;
				}

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex( 0 );

				if( ImGui::Checkbox( "##SameName", &m_new_palette_infos.m_file_name_same_as_palette ) )
				{
					if( m_new_palette_infos.m_file_name_same_as_palette )
						m_new_palette_infos.m_file_name = m_new_palette_infos.m_name;
				}

				ImGui_fzn::simple_tooltip_on_hover( "Same name for palette and file" );

				ImGui::TableSetColumnIndex( 1 );

				if( m_new_palette_infos.m_file_name_same_as_palette )
					ImGui::BeginDisabled();
				ImGui::InputText( "Palette File Name", &m_new_palette_infos.m_file_name );

				if( m_new_palette_infos.m_file_name_same_as_palette )
					ImGui::EndDisabled();

				ImGui::EndTable();
			}

			Utils::window_bottom_table( 2, [&]()
			{
				ColorPalette* palette = _find_palette( m_new_palette_infos.m_name );

				const bool disable_button{ palette != nullptr || m_new_palette_infos.m_name.empty() };

				if( ImGui_fzn::deactivable_button( "Confirm", disable_button, true, DefaultWidgetSize ) )
				{
					if( m_new_palette_infos.m_file_name_same_as_palette || m_new_palette_infos.m_file_name.empty() )
						m_new_palette_infos.m_file_name = m_new_palette_infos.m_name;

					m_palettes.push_back( { m_new_palette_infos.m_name } );
					m_selected_palette = &m_palettes.back();

					if( m_new_palette_infos.m_source_palette != nullptr )
					{
						m_selected_palette->m_colors			= m_new_palette_infos.m_source_palette->m_colors;
						m_selected_palette->m_presets			= m_new_palette_infos.m_source_palette->m_presets;
						m_selected_palette->m_nb_digits_in_IDs	= m_new_palette_infos.m_source_palette->m_nb_digits_in_IDs;
						m_selected_palette->m_using_names		= m_new_palette_infos.m_source_palette->m_using_names;
					}
					else
					{
						m_selected_palette->m_presets.push_back( { color_preset_all } );
					}

					m_selected_palette->m_file_path = m_new_palette_infos.m_file_name + ".xml";

					if( m_new_palette_infos.m_restore_backup_palette )
						_restore_backup_palette();
					else if( m_new_palette_infos.m_set_new_palette_as_backup )
						m_backup_palette = m_palettes.back();

					_select_palette( *m_selected_palette );
					_save_palette();

					m_new_palette = false;
					m_new_palette_infos = NewPaletteInfos{};
				}
				if( disable_button )
				{
					ImGui_fzn::simple_tooltip_on_hover( "Name field is empty or given name is already used for an other palette" );
				}

				ImGui::TableSetColumnIndex( 2 );
				if( ImGui::Button( "Cancel", DefaultWidgetSize ) )
				{
					m_palette_edition = false;
					m_new_palette = false;
					m_new_palette_infos = NewPaletteInfos{};
				}
			} );

			ImGui::EndPopup();
		}
	}

	/**
	* @brief Preset creation popup allowing to set its name.
	**/
	void PalettesManager::_new_preset_popup()
	{
		if( m_selected_palette == nullptr || m_new_preset == false )
			return;

		ImGui::OpenPopup( "New Preset" );

		static constexpr ImVec2	popup_size{ 400.f, 0.f };
		ImGui::SetNextWindowSize( popup_size );

		const auto window_size = g_pFZN_WindowMgr->GetWindowSize();
		const ImVec2 window_pos{ window_size.x * 0.5f - popup_size.x * 0.5f, window_size.y * 0.5f - popup_size.x * 0.5f };
		ImGui::SetNextWindowPos( window_pos, ImGuiCond_Appearing );

		bool is_editing_text{ false };

		if( ImGui::BeginPopupModal( "New Preset" ) )
		{
			ImGui::InputText( "Preset Name", &m_new_preset_infos.m_name );

			Utils::window_bottom_table( 2, [&]()
			{
				auto it_preset = std::ranges::find( m_selected_palette->m_presets, m_new_preset_infos.m_name, &ColorPreset::m_name );

				const bool disable_button{ it_preset != m_selected_palette->m_presets.end() || m_new_preset_infos.m_name.empty() };

				if( ImGui_fzn::deactivable_button( "Confirm", disable_button, true, DefaultWidgetSize ) )
				{
					// Preset creation from an other preset. We add a new one using the source we have in the new preset infos.
					if( m_new_preset_infos.m_source_preset != nullptr )
					{
						m_selected_palette->m_presets.push_back( { m_new_preset_infos.m_name , m_new_preset_infos.m_source_preset->m_colors } );
					}
					else
					{
						// In any other case, we want to create a new preset from the name the user just entered.
						m_selected_palette->m_presets.push_back( { m_new_preset_infos.m_name } );
					}

					m_selected_preset = &m_selected_palette->m_presets.back();

					// In a "Save As..." situation, we update the newly created preset with the current color selection.
					if( m_new_preset_infos.m_create_from_current_selection )
					{
						_update_preset_colors_from_selection();
					}

					_select_colors_from_selected_preset();

					_save_palette();

					m_new_preset = false;
					m_new_preset_infos = NewPresetInfos{};
				}
				if( disable_button )
				{
					ImGui_fzn::simple_tooltip_on_hover( "Name field is empty or given name is already used in the current palette presets" );
				}

				ImGui::TableSetColumnIndex( 2 );
				if( ImGui::Button( "Cancel", DefaultWidgetSize ) )
				{
					m_new_preset = false;
					m_new_preset_infos = NewPresetInfos{};
				}
			} );

			ImGui::EndPopup();
		}
	}

	/**
	* @brief Calculate the ID column size in the color list table by computing the number of digits the palette IDs are gonna need and multiplying that by the size of a '0' character.
	* @param _compute_palette_infos True to compute the number of digit, using the one calulated before otherwise.
	**/
	void PalettesManager::_compute_ID_column_size( bool _compute_palette_infos )
	{
		if( m_selected_palette == nullptr )
			return;

		if( _compute_palette_infos )
			_compute_IDs_and_names_usage_infos( *m_selected_palette );

		// We calculate the size of 1 zero, and we'll multiply it by the number of character in the highest ID + 1.
		const float zero_size{ ImGui::CalcTextSize( "0" ).x };
		m_ID_column_width = zero_size * m_selected_palette->m_nb_digits_in_IDs + 1;
	}
};