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
		/*enum EOffsetCorner
		{
			top_left,
			top_right,
			bottom_right,
			bottom_left,
			COUNT
		};

		enum EZoomDirection
		{
			up_left,
			up_right,
			down_right,
			down_left,
			COUNT
		};

		using ZoomDirectionOffsets = std::array< sf::Vector2f, EOffsetCorner::COUNT >;
		using ZoomDirections = std::array< ZoomDirectionOffsets, EZoomDirection::COUNT >;*/

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
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// Load the base vertex array from the chosen image
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		void _load_pixels( sf::Texture* _texture );
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// Move all the pixels of the vertex array to the given position
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		void _set_vertex_array_pos( const sf::Vector2f& _pos );
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// Move a quad to the given position and update its zoom level
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		void _set_quad_pos_and_zoom( sf::VertexArray& _pixels, int _quad_index, float _zoom_level, const sf::Vector2f& _pos = { 0.f, 0.f } );
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// Update the pixels to a new zoom level
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		void _update_zoom_level( float _new_zoom_level );

		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// Convert the base pixels to new colors according to the currently selected palette
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		void _convert_image_colors();

		uint32_t get_pixel_index( uint32_t _quad_index );

		///////////////// IMGUI /////////////////
		void _display_canvas( const sf::Color& _bg_color );
		void _display_bottom_bar();

		sf::RenderTexture				m_render_texture;
		sf::Sprite						m_default_image_sprite;
		sf::Sprite						m_sprite;
		sf::VertexArray					m_base_pixels;			// pixels created from the base image with its colors
		sf::VertexArray					m_converted_pixels;		// pixels converted from the base ones using a given palette
		std::vector< SPixelDesc >		m_pixels_descs;

		sf::Vector2u					m_image_size{ 0, 0 };
		ImVec2							m_canvas_size{};
		sf::FloatRect					m_image_float_rect{ -1.f, -1.f, -1.f, -1.f };
		float							m_zoom_level{ 1.f };
		std::array< sf::Vector2f, 4 >	m_offsets;
	};
} // namespace PerlerMaker
