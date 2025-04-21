#pragma once

#include <Externals/ImGui/imgui.h>
#include <FZN/UI/ImGuiAdditions.h>


namespace PerlerMaker
{
	class Options
	{
	public:
		struct OptionsDatas
		{
			ImVec4	m_canvas_background_color{ ImGui_fzn::color::black };
			ImVec4	m_grid_color{ ImGui_fzn::color::black };

			ImVec4	m_area_highlight_color{ ImGui_fzn::color::bright_red };
			float	m_area_highlight_thickness{ 1.f };

			bool	m_grid_same_color_as_canvas{ true };
			bool	m_show_grid{ true };

			float	m_original_opacity_pct{ 100.f };
			bool	m_show_original{ false };

			std::vector< fzn::ActionKey > m_bindings;
		};

		Options();

		void menu_bar_options();
		void bottom_bar_options();
		void show_window();
		void update();

		void on_event();

		const OptionsDatas& get_options_datas() { return m_options_datas; }

	private:
		void _draw_keybinds( float _column_width );

		void _load_options();
		void _save_options();

		bool _begin_option_table( float _column_width );
		void _first_column_text( const char* _text, const char* _tooltip = nullptr );

		void _handle_actions();

		bool m_show_window{ false };

		OptionsDatas m_options_datas;
		OptionsDatas m_temp_options_datas;
		bool m_need_save{ false };
	};
} // namespace PerlerMaker
