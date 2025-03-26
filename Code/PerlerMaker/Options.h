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
			ImVec4 m_canvas_background_color{ ImGui_fzn::color::black };
			sf::Color m_area_highlight_color{ sf::Color::Red };
			float m_area_highlight_thickness{ 1.f };
		};

		Options();

		void show_window();
		void update();

		const OptionsDatas& get_options_datas() { return m_options_datas; }

	private:
		void _load_options();
		void _save_options();

		bool _begin_option_table( float _column_width );
		void _first_column_text( const char* _text );

		bool m_show_window{ false };

		OptionsDatas m_options_datas;
		OptionsDatas m_temp_options_datas;
		bool m_need_save{ false };
	};
} // namespace PerlerMaker
