#pragma once

#include <SFML/Graphics/VertexArray.hpp>

#include <FZN/Display/Line.h>

#include "Defines.h"


class sf::Texture;


namespace PerlerMaker
{
	struct ColorInfos;


	class CanvasManager
	{
		struct PixelDesc
		{
			sf::Color m_base_color{ sf::Color::Black };
			ColorInfos m_color_infos;
			uint32_t m_quad_index{ Uint32_Max };
			uint32_t m_pixel_index{ Uint32_Max };
		};
		using PixelDescs = std::vector< PixelDesc >;
		using PixelDescsPtr = std::vector< PixelDesc* >;

		struct PixelArea
		{
			PixelArea()
			{
				m_outline_points.setPrimitiveType( sf::Lines );
			}

			void Reset()
			{
				m_outline_points.clear();
				m_pixels.clear();
				m_line.clear();
			}

			sf::VertexArray m_outline_points;
			fzn::Line		m_line;
			PixelDescsPtr	m_pixels;
		};

	public:
		CanvasManager();
		~CanvasManager();
		
		void on_event();

		void update();

		void load_texture( std::string_view _path );

		void set_original_sprite_opacity( float _opacity );

	private:
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// Load the base vertex array from the chosen image
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		void _load_pixels( sf::Texture* _texture );
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// Change the position and zoom level of the image so it fits entirely in the canvas
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		void _fit_image();
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
		void _update_pixel_grid();

		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// Convert the base pixels to new colors according to the currently selected palette
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		void _convert_image_colors();

		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// Get the corresponding pixel index to the given quad index.
		// Quad index: id of the quad in the vertex array. 0 is the first quad created/used in the picture.
		// Pixel index: id of the pixel in the image, transparent pixels included. 0 is the very first pixel at the top left of the picture.
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		uint32_t get_pixel_index( uint32_t _quad_index );
		PixelDesc* get_pixel_desc( uint32_t _quad_index );
		PixelDesc* _get_pixel_desc_in_direction( const PixelDesc& _pixel, Direction _direction );
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// Get the local position of the mouse on the canvas.
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		ImVec2 _get_mouse_pos() const;
		PixelPosition _get_mouse_pixel_pos() const;

		uint32_t _get_1D_index( const PixelPosition& _pixel_position ) const;
		PixelPosition _get_2D_position( uint32_t _1D_index ) const;

		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// Get the infos of the area the mouse is hovering. All the pixels in the area + a vertex array of the points surrounding it.
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		void _compute_pixel_area( uint32_t _quad_index );
		void _compute_area_outline();
		bool _is_pixel_in_current_area( uint32_t _pixel_index ) const;

		///////////////// IMGUI /////////////////
		void _display_canvas( const sf::Color& _bg_color );
		void _mouse_detection();
		void _display_bottom_bar();

		sf::RenderTexture				m_render_texture;
		sf::Sprite						m_default_image_sprite;

		sf::RenderTexture				m_test_texture;
		sf::Sprite						m_test_image_sprite;

		sf::Sprite						m_sprite;
		sf::VertexArray					m_base_pixels;			// pixels created from the base image with its colors
		sf::VertexArray					m_converted_pixels;		// pixels converted from the base ones using a given palette
		PixelDescs						m_pixels_descs;

		sf::Vector2u					m_image_size{ 0, 0 };
		ImVec2							m_canvas_size{};
		sf::FloatRect					m_image_float_rect{ -1.f, -1.f, -1.f, -1.f };
		float							m_zoom_level{ 1.f };
		std::array< sf::Vector2f, 4 >	m_offsets;
		sf::Vector2f					m_image_offest{};

		uint32_t						m_last_hovered_pixel_index{ Uint32_Max };
		PixelArea						m_hovered_area;

		sf::RenderTexture				m_grid_texture;
		sf::Sprite						m_grid_sprite;
		sf::VertexArray					m_pixel_grid;
		sf::Shader*						m_grid_shader{ nullptr };
	};
} // namespace PerlerMaker
