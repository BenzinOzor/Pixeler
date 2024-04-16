#pragma once

#include <SFML/Graphics/VertexArray.hpp>


namespace sf
{
	class Texture;
}


namespace PerlerMaker
{
	class CanvasManager
	{
	public:
		CanvasManager();

		void update();

		void load_texture( std::string_view _path );

	private:
		void _load_pixels( sf::Texture* _texture );

		//ImGui
		void _display_canvas( const sf::Color& _bg_color );
		void _display_bottom_bar();

		sf::RenderTexture	m_render_texture;
		sf::Sprite			m_default_image_sprite;
		sf::Sprite			m_sprite;
		sf::VertexArray		m_pixels;
		float				m_pixel_size{ 10.f };
	};
} // namespace PerlerMaker
