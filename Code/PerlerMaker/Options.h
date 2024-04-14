#pragma once

#include <Externals/ImGui/imgui.h>
#include <FZN/UI/ImGuiAdditions.h>


namespace PerlerMaker
{
	class Options
	{
	public:
		Options();

		void show_window();
		void update();

		const ImVec4& get_canvas_background_color() const { return m_canvas_background_color; }

	private:
		void _load_options();
		void _save_options();

		bool m_show_window{ false };

		ImVec4 m_canvas_background_color{ ImGui_fzn::color::black };
	};
} // namespace PerlerMaker
