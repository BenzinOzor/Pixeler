#pragma once
#include <string>
#include <unordered_map>

#include <FZN/UI/ImGui.h>

#include "Defines.h"
#include "ColorPalette.h"


namespace tinyxml2
{
	class XMLElement;
}


namespace PerlerMaker
{
	static const std::string preset_all{ "All" };

	class PalettesManager
	{
	public:
		PalettesManager();

		void update();

		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// Convert the given color to a new one according to the selected palette
		// If no palette is selected, the given color will be returned
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		std::pair< sf::Color, const ColorInfos* > convert_color( const sf::Color& _color ) const;

		void reset_base_palettes();
		void reset_color_counts();

		const ColorPalette* get_selected_palette() const;

	private:
		struct NewPaletteInfos
		{
			std::string m_file_name;
			bool m_file_name_same_as_palette{ true };
		};

		void _load_palettes();
		void _load_palette( tinyxml2::XMLElement* _palette, std::string_view _file_name, bool _bOverride = false );
		void _save_palette();
		void _delete_palette( ColorPalette& _palette_to_delete );

		void _set_all_colors_selection( bool _selected );
		void _select_colors_from_preset( std::string_view _preset );
		bool _update_preset();
		void _reset_color_to_edit();

		std::string _get_presets_from_color_index( ColorPalette* _palette, uint32_t _color_index );
		std::string _get_palette_root_path( const std::string& _path );

		void _create_new_palette();
		void _create_palette_from_other( ColorPalette& _other );
		std::string _generate_new_palette_name();

		bool match_filter( const ColorInfos& _color );

		///////////////// IMGUI /////////////////
		void _header();
		void _palette_hamburger_menu();
		void _preset_hamberger_menu();
		bool _color_table_begin();
		void _colors_list();
		bool _selectable_color_info( ColorInfos& _color, int _current_row );
		void _edit_color();
		void _edit_color_buttons();
		void _add_color_buttons();
		void _new_palette_popup();
		void _compute_ID_column_size( bool _compute_palette_infos );
		

		const std::string			m_fzn_palettes_path{};
		const std::string			m_app_palettes_path{};

		ColorPalettes		m_palettes;
		ColorPalette*		m_selected_palette{ nullptr };
		ColorPalette		m_backup_palette;
		std::string_view	m_selected_preset{};

		bool				m_palette_edition{ false };
		bool				m_new_palette{ false };
		bool				m_only_used_colors_display{ false };
		ColorInfos*			m_color_to_edit{ nullptr };				// A pointer to the color we're editing, nullptr when adding a new color
		ColorInfos			m_edited_color;
		std::string			m_color_filter{};
		NewPaletteInfos		m_new_palette_infos;
		float				m_ID_column_width{ 0.f };
	};
} // namespace PerlerMaker
