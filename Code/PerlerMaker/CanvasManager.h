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
		struct SPixelDesc
		{
			uint32_t m_pixel_index{ 0 };
			sf::Color m_base_color{ sf::Color::Black };
		};

	public:
		CanvasManager();
		~CanvasManager();
		
		void on_event();

		void update();

		void load_texture( std::string_view _path );

		void _fit_image();
	private:
		void _load_pixels( sf::Texture* _texture );
		void _set_vertex_array_pos( const sf::Vector2f& _pos );
		void _update_pixel_size( float _new_pixel_size );

		uint32_t get_pixel_index( uint32_t _quad_index );

		//ImGui
		void _display_canvas( const sf::Color& _bg_color );
		void _display_bottom_bar();

		sf::RenderTexture				m_render_texture;
		sf::Sprite						m_default_image_sprite;
		sf::Sprite						m_sprite;
		sf::VertexArray					m_pixels;
		std::vector< SPixelDesc >		m_pixels_descs;

		sf::Vector2u					m_image_size{ 0, 0 };
		ImVec2							m_canvas_size{};
		sf::FloatRect					m_image_float_rect{ -1.f, -1.f, -1.f, -1.f };
		float							m_zoom_level{ 1.f };
		std::array< sf::Vector2f, 4 >	m_offsets;
	};
} // namespace PerlerMaker
