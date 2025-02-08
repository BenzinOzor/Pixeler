#pragma once
#include <string>
#include <unordered_map>

#include <FZN/UI/ImGui.h>


namespace tinyxml2
{
	class XMLElement;
}


namespace PerlerMaker
{
	class PalettesManager
	{
		struct ColorInfos
		{
			bool				is_valid( bool _test_color_only = false ) const;
			std::string			get_full_name() const;		// "id - name"

			std::string			m_name{};					// can be empty
			int					m_id{ -1 };					// can be invalid
			ImColor				m_color{ -1, -1, -1, -1 };
			bool				m_selected{ true };
		};
		using ColorInfosVector = std::vector< ColorInfos >;

		struct ColorPalette
		{
			std::string m_name{};
			std::string m_file_path{};
			ColorInfosVector m_colors;
			std::unordered_map< std::string, std::vector< uint32_t > > m_presets;	// for each preset name, a list of selected colors (by index in the vector).
		};
		using ColorPalettes = std::unordered_map< std::string, ColorPalette >;

	public:
		PalettesManager();

		void update();

		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// Convert the given color to a new one according to the selected palette
		// If no palette is selected, the given color will be returned
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		sf::Color convert_color( const sf::Color& _color ) const;

		void reset_base_palettes();

	private:
		void _load_palettes();
		void _load_palette( tinyxml2::XMLElement* _palette, std::string_view _file_name, bool _bOverride = false );
		void _save_palette();

		void _set_all_colors_selection( bool _selected );
		void _select_colors_from_preset( std::string_view _preset );
		void _reset_color_to_edit();

		///////////////// IMGUI /////////////////
		void _header();
		void _colors_list();
		void _selectable_color_info( ColorInfos& _color );
		void _edit_color();
		std::string _get_presets_from_color_index( ColorPalette* _palette, uint32_t _color_index );
		std::string _get_palette_root_path( const std::string& _path );

		const std::string			m_fzn_palettes_path{};
		const std::string			m_app_palettes_path{};

		ColorPalettes		m_palettes;
		ColorPalette*		m_selected_palette{ nullptr };
		std::string_view	m_selected_preset{};

		bool				m_palette_edition{ false };
		ColorInfos*			m_color_to_edit{ nullptr };
		ColorInfos			m_edited_color;
	};
} // namespace PerlerMaker
