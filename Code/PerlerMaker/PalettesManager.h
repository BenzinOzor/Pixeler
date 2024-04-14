#pragma once
#include <string>
#include <unordered_map>

#include <FZN/UI/ImGui.h>


namespace PerlerMaker
{
	class PalettesManager
	{
		struct ColorInfos
		{
			std::string			get_full_name();			// "id - name"

			std::string			m_name{};					// can be empty
			int					m_id{ -1 };					// can be invalid
			ImColor				m_color{ -1, -1, -1, -1 };
			bool				m_selected{ true };
		};
		using ColorInfosVector = std::vector< ColorInfos >;

		struct ColorPalette
		{
			std::string m_name{};
			ColorInfosVector m_colors;
			std::unordered_map< std::string, std::vector< uint32_t > > m_presets;	// for each preset name, a list of selected colors (by index in the vector).
		};
		using ColorPalettes = std::unordered_map< std::string, ColorPalette >;

	public:
		PalettesManager();

		void update();

	private:
		void _load_palettes();
		void _load_palette( tinyxml2::XMLElement* _palette, std::string_view _file_name );

		void _set_all_colors_selection( bool _selected );
		void _select_colors_from_preset( std::string_view _preset );

		// ImGui
		void _header();
		void _colors_list();
		void _selectable_color_info( ColorInfos& _color );

		ColorPalettes		m_palettes;
		ColorPalette*		m_selected_palette{ nullptr };
		std::string_view	m_selected_preset{};
	};
} // namespace PerlerMaker
