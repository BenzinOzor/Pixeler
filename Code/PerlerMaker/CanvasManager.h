#pragma once

#include <SFML/Graphics/VertexArray.hpp>

#include <FZN/Display/Line.h>

#include "Defines.h"
#include "ColorPalette.h"


class sf::Texture;


namespace PerlerMaker
{
	struct ColorInfos;


	class CanvasManager
	{
		/************************************************************************
		* @brief The informations available on a pixel displayed on the canvas.
		************************************************************************/
		struct PixelDesc
		{
			sf::Color			m_base_color{ sf::Color::Black };	// The color of the pixel before the convertion.
			const ColorInfos*	m_color_infos{ nullptr };			// The informations on the new color given to the pixel. (Id, value, count, etc...)
			uint32_t			m_quad_index{ Uint32_Max };			// The index of the quad in the sprite (only the created pixels, empty spaces don't have index, 0 is the first drawn pixel)
			uint32_t			m_pixel_index{ Uint32_Max };		// The overall index of the pixel, including empty spaces. 0 is the top left most pixel in the image.
		};
		using PixelDescs = std::vector< PixelDesc >;		// A vector containing pixel descriptions
		using PixelDescsPtr = std::vector< PixelDesc* >;	// A vector containing pointers to pixel descriptions. Used to represent areas of pixels.
		using PixelAreas = std::vector< PixelDescsPtr >;	// A vector of pixel areas.


		/************************************************************************
		* @brief Computed informations about the pixel area hovered by the mouse on the canvas or in the colors list.
		************************************************************************/
		struct HoveredColor
		{
			HoveredColor()
			{
				m_hovered_area_points.setPrimitiveType( sf::Lines );
				m_colored_area_points.setPrimitiveType( sf::Lines );
			}

			/**
			* @brief Clear all detected pixels and vertex arrays
			**/
			void reset()
			{
				m_pixel_areas.clear();
				m_first_area_hovered = false;

				clear_vertices_and_lines();
			}

			void clear_vertices_and_lines()
			{
				m_hovered_area_points.clear();
				m_hovered_area_line.clear();
				m_colored_area_points.clear();
				m_colored_area_line.clear();
			}

			PixelAreas		m_pixel_areas;					// A list of pixel areas the same color as the one hovered by the mouse.
			bool			m_first_area_hovered{ false };	// If true, the first area in the vector represents the one hovered by the mouse.

			// Points and line of the canvas area hovered by the mouse
			sf::VertexArray m_hovered_area_points;
			fzn::Line		m_hovered_area_line;

			// Points and line of the pixels the same color as the hovered one, other than the hovered area if applicable
			sf::VertexArray m_colored_area_points;
			fzn::Line		m_colored_area_line;
		};

	public:
		CanvasManager();
		~CanvasManager();
		
		void on_event();

		void update();

		void load_texture( std::string_view _path );

		void set_original_sprite_opacity( float _opacity );

		/**
		* @brief Retrieve all same colored pixels as the given color.
		* @param _area_color The color to find in the pixel descs array.
		**/
		void compute_pixel_area( const ColorInfos& _area_color );

	private:
		//·································································································································································
		// Load the base vertex array from the chosen image
		//··································································································································································
		void _load_pixels( sf::Texture* _texture );
		//·································································································································································
		// Change the position and zoom level of the image so it fits entirely in the canvas
		//··································································································································································
		void _fit_image();
		//·································································································································································
		// Move all the pixels of the vertex array to the given position
		//··································································································································································
		void _set_vertex_array_pos( const sf::Vector2f& _pos );
		// ·································································································································································
		// @brief Move a quad to the given position and update its zoom level
		// @param _pixels The array containing all the quads.
		// ··································································································································································
		void _set_quad_pos_and_zoom( sf::VertexArray& _pixels, int _quad_index, float _zoom_level, const sf::Vector2f& _pos = { 0.f, 0.f } );
		//·································································································································································
		// Update the pixels to a new zoom level
		//··································································································································································
		void _update_zoom_level( float _new_zoom_level );
		void _update_pixel_grid();

		//·································································································································································
		// Convert the base pixels to new colors according to the currently selected palette
		//··································································································································································
		void _convert_image_colors();

		//·································································································································································
		// Get the corresponding pixel index to the given quad index.
		// Quad index: id of the quad in the vertex array. 0 is the first quad created/used in the picture.
		// Pixel index: id of the pixel in the image, transparent pixels included. 0 is the very first pixel at the top left of the picture.
		//··································································································································································
		uint32_t get_pixel_index( uint32_t _quad_index );
		PixelDesc* get_pixel_desc( uint32_t _quad_index );
		PixelDesc* _get_pixel_desc_in_direction( const PixelDesc& _pixel, Direction _direction );
		//································································
		// Get the local position of the mouse on the canvas.
		//································································
		ImVec2 _get_mouse_pos() const;
		PixelPosition _get_mouse_pixel_pos() const;

		uint32_t _get_1D_index( const PixelPosition& _pixel_position ) const;
		PixelPosition _get_2D_position( uint32_t _1D_index ) const;

		void _get_colored_pixels_in_area( uint32_t _pixel_index, const ColorInfos& _area_color, PixelDescsPtr& _pixel_area, std::vector< uint32_t >& _treated_indexes );

		/**
		* @brief Retrieve all same colored pixels as the one at the given index.
		* @param _pixel_index Index in the pixel descs array.
		**/
		void _compute_pixel_area( uint32_t _pixel_index );

		/**
		* @brief Retrieve all same colored pixels as the given color.
		* @warning This function is not meant to be called first, it is called by the two others of the same name that set up some variables first.
		* @param _area_color The color to find in the pixel descs array.
		* @param _treated_indexes An array containing all the previously checked pixel indexes.
		**/
		void _compute_pixel_area( const ColorInfos& _area_color, std::vector< uint32_t >& _treated_indexes );

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
		HoveredColor					m_hovered_color;

		sf::RenderTexture				m_grid_texture;
		sf::Sprite						m_grid_sprite;
		sf::VertexArray					m_pixel_grid;
		sf::Shader*						m_grid_shader{ nullptr };
	};
} // namespace PerlerMaker
