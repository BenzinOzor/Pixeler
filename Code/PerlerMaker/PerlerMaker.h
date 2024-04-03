#pragma once


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

	class CPerlerMaker
	{
	public:
		CPerlerMaker();
		~CPerlerMaker();

		void display();

	private:
		void load_image();

		void display_canvas();
		void display_colors_selector();

		sf::RenderTexture	m_render_texture;
		sf::Sprite			m_default_image_sprite;
		sf::Sprite			m_sprite;

		bool				m_selected_colors[ (uint32_t)BeadColor::COUNT ]{};
	};
};
