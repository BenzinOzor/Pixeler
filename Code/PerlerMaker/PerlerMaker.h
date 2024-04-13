#pragma once

#include <Externals/ImGui/imgui.h>


namespace PerlerMaker
{
	enum class BeadColor
	{
		White,
		Red,
		Blue,
		Green,
		Black,
		COUNT
	};

	struct BeadInfos
	{
		BeadColor			m_type{ BeadColor::COUNT };
		std::string_view	m_name{};
		uint32_t			m_id{ 0 };
		ImColor				m_color{ -1, -1, -1, -1 };
		bool				m_selected{ true };
	};

	class CPerlerMaker
	{
	public:
		CPerlerMaker();
		~CPerlerMaker();

		void display();

	private:
		void _initialize_beads_array();
		void _load_image();

		void _display_canvas();
		void _display_colors_selector();

		sf::RenderTexture	m_render_texture;
		sf::Sprite			m_default_image_sprite;
		sf::Sprite			m_sprite;
		ImColor				m_canvas_background_color;

		std::array< BeadInfos, (size_t)BeadColor::COUNT > m_beads;
	};
};
