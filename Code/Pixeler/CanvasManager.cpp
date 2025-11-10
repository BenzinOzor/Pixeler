//#include <SFML/Graphics.hpp>
#include <array>

#include <FZN/Managers/DataManager.h>
#include <FZN/Managers/WindowManager.h>
#include <FZN/Tools/Math.h>
#include <FZN/UI/ImGui.h>
#include <FZN/Tools/Logging.h>

#include "CanvasManager.h"
#include "Pixeler.h"
#include "Utils.h"


namespace Pixeler
{
	CanvasManager::CanvasManager()
	{
		g_pFZN_Core->AddCallback( this, &CanvasManager::on_event, fzn::DataCallbackType::Event );

		sf::Vector2u window_size = g_pFZN_WindowMgr->GetWindowSize();
		
		m_render_texture.create( window_size.x, window_size.y );
		m_sprite.setTexture( m_render_texture.getTexture() );

		m_test_texture.create( window_size.x, window_size.y );
		m_test_image_sprite.setTexture( m_test_texture.getTexture() );

		m_grid_texture.create( window_size.x, window_size.y );
		m_grid_sprite.setTexture( m_grid_texture.getTexture() );

		m_base_pixels.setPrimitiveType( sf::PrimitiveType::Quads );
		m_converted_pixels.setPrimitiveType( sf::PrimitiveType::Quads );

		m_offsets[ 0 ] = { 0.f,				0.f };				// top left
		m_offsets[ 1 ] = { m_zoom_level,	0.f };				// top right
		m_offsets[ 2 ] = { m_zoom_level,	m_zoom_level };		// bottom right
		m_offsets[ 3 ] = { 0.f,				m_zoom_level };		// bottom left

		m_hovered_color.m_hovered_area_line.set_thickness( 1.f );
		m_hovered_color.m_hovered_area_line.set_color( sf::Color::Red );

		m_hovered_color.m_colored_area_line.set_thickness( 1.f );
		m_hovered_color.m_colored_area_line.set_color( sf::Color::Red );

		m_pixel_grid.setPrimitiveType( sf::Lines );
		m_grid_shader = g_pFZN_DataMgr->GetShader( "grid_shader" );
	}

	CanvasManager::~CanvasManager()
	{
		g_pFZN_Core->RemoveCallback( this, &CanvasManager::on_event, fzn::DataCallbackType::Event );
	}

	void CanvasManager::on_event()
	{
		auto window_event{ g_pFZN_WindowMgr->GetWindowEvent() };

		if( window_event.type == sf::Event::Resized )
		{
			FZN_LOG( "new window size %d %d", window_event.size.width, window_event.size.height );
			m_render_texture.create( window_event.size.width, window_event.size.height );
			m_grid_texture.create( window_event.size.width, window_event.size.height );
			m_test_texture.create( window_event.size.width, window_event.size.height );
		}
	}

	void CanvasManager::update()
	{
		auto& options_datas{ g_pixeler->get_options().get_options_datas() };
		auto& canvas_bg_color{ options_datas.m_canvas_background_color };
		ImGui::PushStyleColor( ImGuiCol_ChildBg, canvas_bg_color );
		ImGui::PushStyleColor( ImGuiCol_WindowBg, canvas_bg_color );
		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, { 0, 0 } );

		if( ImGui::Begin( "Canvas" ) )
		{
			m_canvas_size = ImGui::GetContentRegionAvail();
			m_canvas_size.y -= ImGui::GetFrameHeightWithSpacing(); // bottom bar
			_display_canvas( canvas_bg_color );
			ImGui::PopStyleVar();

			_display_bottom_bar();
		}
		else
			ImGui::PopStyleVar();

		ImGui::End();
		ImGui::PopStyleColor( 2 );
		
	}

	void CanvasManager::load_texture( std::string_view _path )
	{
		if( _path.empty() )
			return;

		if( g_pFZN_DataMgr->GetTexture( "Perler Default Image", false ) != nullptr )
			g_pFZN_DataMgr->UnloadTexture( "Perler Default Image" );

		auto texture = g_pFZN_DataMgr->LoadTexture( "Perler Default Image", _path.data() );

		if( texture != nullptr )
			m_default_image_sprite.setTexture( *texture );

		_load_pixels( texture );
		_fit_image();
	}

	void CanvasManager::set_original_sprite_opacity( float _opacity )
	{
		const uint8_t alpha{ static_cast< uint8_t >( _opacity / 100.f * 255.f ) };

		for( int quad_index{ 0 }; quad_index < m_base_pixels.getVertexCount(); quad_index += 4 )
		{
			m_base_pixels[ quad_index ].color.a		= alpha;
			m_base_pixels[ quad_index + 1 ].color.a = alpha;
			m_base_pixels[ quad_index + 2 ].color.a = alpha;
			m_base_pixels[ quad_index + 3 ].color.a = alpha;
		}
	}

	/**
	* @brief Retrieve all same colored pixels as the given color.
	* @param _area_color The color to find in the pixel descs array.
	**/
	void CanvasManager::compute_pixel_area( const ColorInfos& _area_color )
	{
		// First, we check if we already have hovered pixels and if they are the same color as the given one.
		// If that's the case, we don't need to compute areas again.
		if( m_hovered_color.m_pixel_areas.empty() == false && m_hovered_color.m_pixel_areas.front().empty() == false )
		{
			// Separating the tests for lisibility
			if( *m_hovered_color.m_pixel_areas.front().front()->m_color_infos == _area_color )
				return;
		}
		
		std::vector< uint32_t > treated_indexes;
		m_hovered_color.reset();

		_compute_pixel_area( _area_color, treated_indexes );
	}

	//································································
	// Load the base vertex array from the chosen image
	//································································
	void CanvasManager::_load_pixels( sf::Texture* _texture )
	{
		if( _texture == nullptr )
			return;

		m_base_pixels.clear();
		m_converted_pixels.clear();
		m_pixels_descs.clear();

		const auto image{ _texture->copyToImage() };
		auto color_values{ image.getPixelsPtr() };
		m_image_size = image.getSize();

		auto image_pos_min = sf::Vector2f{ Flt_Max, Flt_Max };
		auto image_pos_max = sf::Vector2f{ -1.f, -1.f };
		const auto max_pixel_index{ m_image_size.x * m_image_size.y * ColorChannel::COUNT };
		uint32_t quad_index{ 0 };

		for( uint32_t value_index{ 0u }; value_index < max_pixel_index; value_index += ColorChannel::COUNT, color_values += ColorChannel::COUNT )
		{
			const sf::Color pixel_color{ color_values[ ColorChannel::red ], color_values[ ColorChannel::green ], color_values[ ColorChannel::blue ], color_values[ ColorChannel::alpha ] };

			m_pixels_descs.push_back( { .m_base_color = pixel_color, .m_pixel_index = m_pixels_descs.size() } );

			if( pixel_color.a < 50 )
				continue;

			const uint32_t		pixel_index{ value_index / ColorChannel::COUNT };
			const sf::Vector2f	pixel_position{ static_cast< float >( pixel_index % m_image_size.x ), static_cast<float>( pixel_index / m_image_size.x ) };

			if( pixel_position.x < image_pos_min.x )
				image_pos_min.x = pixel_position.x;

			if( pixel_position.y < image_pos_min.y )
				image_pos_min.y = pixel_position.y;

			if( pixel_position.x >= image_pos_max.x )
				image_pos_max.x = pixel_position.x + 1.f;

			if( pixel_position.y >= image_pos_max.y )
				image_pos_max.y = pixel_position.y + 1.f;

			m_base_pixels.append( { { pixel_position + m_offsets[ 0 ] }, pixel_color } );
			m_base_pixels.append( { { pixel_position + m_offsets[ 1 ] }, pixel_color } );
			m_base_pixels.append( { { pixel_position + m_offsets[ 2 ] }, pixel_color } );
			m_base_pixels.append( { { pixel_position + m_offsets[ 3 ] }, pixel_color } );

			m_converted_pixels.append( { { pixel_position + m_offsets[ 0 ] }, pixel_color } );
			m_converted_pixels.append( { { pixel_position + m_offsets[ 1 ] }, pixel_color } );
			m_converted_pixels.append( { { pixel_position + m_offsets[ 2 ] }, pixel_color } );
			m_converted_pixels.append( { { pixel_position + m_offsets[ 3 ] }, pixel_color } );

			m_pixels_descs.back().m_quad_index = quad_index++;
		}

		m_image_float_rect.left		= image_pos_min.x;
		m_image_float_rect.top		= image_pos_min.y;
		m_image_float_rect.width	= image_pos_max.x - image_pos_min.x;
		m_image_float_rect.height	= image_pos_max.y - image_pos_min.y;
	}

	//································································
	// Change the position and zoom level of the image so it fits entirely in the canvas
	//································································
	void CanvasManager::_fit_image()
	{
		const float horizontal_ratio{ m_canvas_size.x / m_image_float_rect.width };
		const float vertical_ratio{ m_canvas_size.y / m_image_float_rect.height };

		const float new_rect_width{ horizontal_ratio > vertical_ratio ? m_image_float_rect.width * vertical_ratio : m_canvas_size.x };
		const float new_rect_height{ horizontal_ratio > vertical_ratio ? m_canvas_size.y : m_image_float_rect.height * horizontal_ratio };

		_update_zoom_level( new_rect_width / m_image_float_rect.width );

		const float left{ ( m_canvas_size.x - new_rect_width ) * 0.5f };
		const float top{ ( m_canvas_size.y - new_rect_height ) * 0.5f };

		_set_vertex_array_pos( sf::Vector2f{ left, top } - sf::Vector2f{ m_image_float_rect.left, m_image_float_rect.top } * m_zoom_level );
	}

	//································································
	// Move all the pixels of the vertex array to the given position
	//································································
	void CanvasManager::_set_vertex_array_pos( const sf::Vector2f& _pos )
	{
		m_image_offest = _pos;

		for( int quad_index{ 0 }; quad_index < m_base_pixels.getVertexCount(); quad_index += 4 )
		{
			_set_quad_pos_and_zoom( m_base_pixels, quad_index, m_zoom_level, _pos );
			_set_quad_pos_and_zoom( m_converted_pixels, quad_index, m_zoom_level, _pos );
		}

		_update_pixel_grid();
	}

	//································································
	// Move a quad to the given position and update its zoom level
	//································································
	void CanvasManager::_set_quad_pos_and_zoom( sf::VertexArray& _pixels, int _quad_index, float _zoom_level, const sf::Vector2f& _pos /*= { 0.f, 0.f }*/ )
	{
		for( int quad_corner{ 0 }; quad_corner < 4; ++quad_corner )
		{
			auto base_index{ get_pixel_index( _quad_index / 4 ) };
			auto base_pos = sf::Vector2f{ ( base_index % m_image_size.x ) * _zoom_level, ( base_index / m_image_size.x ) * _zoom_level };

			_pixels[ _quad_index + quad_corner ].position = _pos + base_pos + m_offsets[ quad_corner ] * _zoom_level;
		}
	}

	//································································
	// Update the pixels to a new zoom level
	//································································
	void CanvasManager::_update_zoom_level( float _new_pixel_size )
	{
		for( int quad_index{ 0 }; quad_index < m_base_pixels.getVertexCount(); quad_index += 4 )
		{
			_set_quad_pos_and_zoom( m_base_pixels, quad_index, _new_pixel_size );
			_set_quad_pos_and_zoom( m_converted_pixels, quad_index, _new_pixel_size );
		}

		m_zoom_level = _new_pixel_size;

		_update_pixel_grid();
	}

	void CanvasManager::_update_pixel_grid()
	{
		if( m_zoom_level <= 1.f )
			return;

		m_pixel_grid.clear();

		auto& options_datas{ g_pixeler->get_options().get_options_datas() };
		auto& canvas_bg_color{ sf::Color::Green };
		float grid_position = m_image_offest.x + m_zoom_level;

		while( grid_position < m_canvas_size.x )
		{
			m_pixel_grid.append( { { grid_position, 0.f }, canvas_bg_color } );
			m_pixel_grid.append( { { grid_position, m_canvas_size.y + 0.f }, canvas_bg_color } );

			grid_position += m_zoom_level;
		}

		grid_position = m_image_offest.y + m_zoom_level;
		while( grid_position < m_canvas_size.y )
		{
			m_pixel_grid.append( { { 0.f, grid_position }, canvas_bg_color } );
			m_pixel_grid.append( { { m_canvas_size.x + 0.f, grid_position }, canvas_bg_color } );

			grid_position += m_zoom_level;
		}
	}

	//································································
	// Convert the base pixels to new colors according to the currently selected palette
	//································································
	void CanvasManager::_convert_image_colors()
	{
		const PalettesManager& palettes_manager{ g_pixeler->get_palettes_manager() };

		for( int quad_index{ 0 }; quad_index < m_base_pixels.getVertexCount(); quad_index += 4 )
		{
			auto [ new_color, color_infos ] = palettes_manager.convert_color( m_base_pixels[ quad_index ].color );
			
			m_converted_pixels[ quad_index + 0 ].color = new_color;
			m_converted_pixels[ quad_index + 1 ].color = new_color;
			m_converted_pixels[ quad_index + 2 ].color = new_color;
			m_converted_pixels[ quad_index + 3 ].color = new_color;

			if( auto* pixel_desc = get_pixel_desc( quad_index / 4 ) )
				pixel_desc->m_color_infos = color_infos;
		}
	}

	// ································································
	// @brief Get the corresponding pixel index to the given quad index.
	// @param _quad_index id of the quad in the vertex array. 0 is the first quad created/used in the picture.
	// @return Pixel index: id of the pixel in the image, transparent pixels included. 0 is the very first pixel at the top left of the picture.
	// ································································
	uint32_t CanvasManager::get_pixel_index( uint32_t _quad_index )
	{
		auto it = std::ranges::find( m_pixels_descs, _quad_index, &PixelDesc::m_quad_index );
		
		if( it != m_pixels_descs.end() )
			return it - m_pixels_descs.begin();

		return 0u;
	}

	CanvasManager::PixelDesc* CanvasManager::get_pixel_desc( uint32_t _quad_index )
	{
		auto it = std::ranges::find( m_pixels_descs, _quad_index, &PixelDesc::m_quad_index );

		if( it != m_pixels_descs.end() )
			return &(*it);

		return nullptr;
	}

	CanvasManager::PixelDesc* CanvasManager::_get_pixel_desc_in_direction( const PixelDesc& _pixel, Direction _direction )
	{
		const PixelPosition pixel_position{ _get_2D_position( _pixel.m_pixel_index ) };

		uint32_t new_pixel_index{ 0u };

		switch( _direction )
		{
			case Pixeler::Direction::up:
			{
				new_pixel_index = _get_1D_index( { pixel_position.x, pixel_position.y - 1 } );
				break;
			}
			case Pixeler::Direction::down:
			{
				new_pixel_index = _get_1D_index( { pixel_position.x, pixel_position.y + 1 } );
				break;
			}
			case Pixeler::Direction::left:
			{
				new_pixel_index = _get_1D_index( { pixel_position.x - 1, pixel_position.y } );
				break;
			}
			case Pixeler::Direction::right:
			{
				new_pixel_index = _get_1D_index( { pixel_position.x + 1, pixel_position.y } );
				break;
			}
			default:
				return nullptr;
		}

		if( new_pixel_index >= m_pixels_descs.size() )
			return nullptr;

		PixelDesc& pixel_desc{ m_pixels_descs[ new_pixel_index ] };

		if( pixel_desc.m_color_infos == nullptr || pixel_desc.m_color_infos->is_valid() == false )
			return nullptr;

		return &pixel_desc;
	}

	//································································
	// Get the local position of the mouse on the canvas.
	//································································
	ImVec2 CanvasManager::_get_mouse_pos() const
	{
		// GetCurrentWindow crashes for some odd reason ? (loosing context) so calculating TitleBarHeight myself.
		const float titlebar_height{ ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f };

		ImVec2 mouse_pos{ ImGui::GetMousePos() - m_sprite.getPosition() };
		mouse_pos.y -= titlebar_height;

		return mouse_pos;
	}

	PixelPosition CanvasManager::_get_mouse_pixel_pos() const
	{
		// Offsetting mouse pos because the image is not always at 0;0
		const ImVec2 mouse_pos{ _get_mouse_pos() - m_image_offest };

		return { static_cast<uint32_t>( mouse_pos.x / m_zoom_level ), static_cast<uint32_t>( mouse_pos.y / m_zoom_level ) };
	}

	uint32_t CanvasManager::_get_1D_index( const PixelPosition& _pixel_position ) const
	{
		if( _pixel_position.x >= m_image_size.x || _pixel_position.y >= m_image_size.y )
			return Uint32_Max;

		const uint32_t image_width{ static_cast<uint32_t>( m_image_size.x ) };

		// y * width + x
		return _pixel_position.y * image_width + _pixel_position.x;
	}

	PixelPosition CanvasManager::_get_2D_position( uint32_t _1D_index ) const
	{
		const uint32_t image_width{ static_cast<uint32_t>( m_image_size.x ) };
		const uint32_t pos_x{ _1D_index % image_width };
		const uint32_t pos_y{ _1D_index / image_width };

		return { pos_x, pos_y };
	}

	void CanvasManager::_get_colored_pixels_in_area( uint32_t _pixel_index, const ColorInfos& _area_color, PixelDescsPtr& _pixel_area, std::vector< uint32_t >& _treated_indexes )
	{
		if( _pixel_index >= m_pixels_descs.size() )
			return;

		if( std::ranges::find( _treated_indexes, _pixel_index ) != _treated_indexes.end() )
			return;

		PixelDesc& pixel_desc{ m_pixels_descs[ _pixel_index ] };

		if( pixel_desc.m_color_infos == nullptr || pixel_desc.m_color_infos->is_valid() == false )
			return;

		_treated_indexes.push_back( _pixel_index );

		if( *pixel_desc.m_color_infos == _area_color )
			_pixel_area.push_back( &pixel_desc );
		else
			return;

		const PixelPosition pixel_position{ _get_2D_position( _pixel_index ) };

		_get_colored_pixels_in_area( _get_1D_index( { pixel_position.x		, pixel_position.y - 1 } ),		_area_color, _pixel_area, _treated_indexes );		// Up
		_get_colored_pixels_in_area( _get_1D_index( { pixel_position.x		, pixel_position.y + 1 } ),		_area_color, _pixel_area, _treated_indexes );		// Down
		_get_colored_pixels_in_area( _get_1D_index( { pixel_position.x - 1	, pixel_position.y } ),			_area_color, _pixel_area, _treated_indexes );		// Left
		_get_colored_pixels_in_area( _get_1D_index( { pixel_position.x + 1	, pixel_position.y } ),			_area_color, _pixel_area, _treated_indexes );		// Right
	}

	/**
	* @brief Retrieve all same colored pixels as the one at the given index.
	* @param _pixel_index Index in the pixel descs array.
	**/
	void CanvasManager::_compute_pixel_area( uint32_t _pixel_index )
	{
		// reset hovered color
		// create treated indexes
		// compute hoevered area around mouse

		if( _pixel_index >= m_pixels_descs.size() )
			return;

		if( m_pixels_descs[ _pixel_index ].m_color_infos == nullptr || m_pixels_descs[ _pixel_index ].m_color_infos->is_valid() == false )
			return;

		std::vector< uint32_t > treated_indexes;
		m_hovered_color.reset();

		const ColorInfos* color_to_find{ m_pixels_descs[ _pixel_index ].m_color_infos };

		m_hovered_color.m_pixel_areas.push_back( {} );
		_get_colored_pixels_in_area( _pixel_index, *color_to_find, m_hovered_color.m_pixel_areas.back(), treated_indexes );

		if( m_hovered_color.m_pixel_areas.back().empty() )
			m_hovered_color.m_pixel_areas.pop_back();
		else
			m_hovered_color.m_first_area_hovered = true;

		_compute_pixel_area( *color_to_find, treated_indexes );
	}

	/**
	* @brief Retrieve all same colored pixels as the given color.
	* @warning This function is not meant to be called first, it is called by the two others of the same name that set up some variables first.
	* @param _area_color The color to find in the pixel descs array.
	* @param _treated_indexes An array containing all the previously checked pixel indexes.
	**/
	void CanvasManager::_compute_pixel_area( const ColorInfos& _area_color, std::vector< uint32_t >& _treated_indexes )
	{
		// for loop indexes 0 > size
		// look for right color
		for( uint32_t pixel_index{ 0u }; pixel_index < m_pixels_descs.size(); ++pixel_index )
		{
			if( m_pixels_descs[ pixel_index ].m_color_infos == nullptr || *m_pixels_descs[ pixel_index ].m_color_infos != _area_color )
				continue;

			if( std::ranges::find( _treated_indexes, pixel_index ) != _treated_indexes.end() )
				continue;

			m_hovered_color.m_pixel_areas.push_back( {} );
			_get_colored_pixels_in_area( pixel_index, _area_color, m_hovered_color.m_pixel_areas.back(), _treated_indexes );

			if( m_hovered_color.m_pixel_areas.back().empty() )
				m_hovered_color.m_pixel_areas.pop_back();
		}

		_compute_area_outline();
	}

	void CanvasManager::_compute_area_outline()
	{
		if( m_hovered_color.m_pixel_areas.empty() )
			return;

		auto& options_datas{ g_pixeler->get_options().get_options_datas() };
		m_hovered_color.clear_vertices_and_lines();

		const float titlebar_height{ ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f };

		auto add_neighbor_points = [&]( sf::VertexArray& _points, const PixelDesc& _pixel, Direction _direction )
		{
			PixelDesc* neighbor_pixel{ _get_pixel_desc_in_direction( _pixel, _direction ) };
			if( neighbor_pixel == nullptr || *_pixel.m_color_infos != *neighbor_pixel->m_color_infos )
			{
				const PixelPosition pixel_position{ _get_2D_position( _pixel.m_pixel_index ) };
				sf::Vector2f point_A{ m_sprite.getPosition() + m_image_offest };
				point_A.x += pixel_position.x * m_zoom_level;
				point_A.y += pixel_position.y * m_zoom_level - titlebar_height;

				sf::Vector2f point_B{ point_A };

				switch( _direction )
				{
					case Pixeler::Direction::up:
					{
						point_B.x += m_zoom_level;
						break;
					}
					case Pixeler::Direction::down:
					{
						point_A.y += m_zoom_level;
						point_B.x += m_zoom_level;
						point_B.y += m_zoom_level;
						break;
					}
					case Pixeler::Direction::left:
					{
						point_B.y += m_zoom_level;
						break;
					}
					case Pixeler::Direction::right:
					{
						point_A.x += m_zoom_level;
						point_B.x += m_zoom_level;
						point_B.y += m_zoom_level;
						break;
					}
					default:
						break;
				}

				_points.append( { point_A } );
				_points.append( { point_B } );
			}
		};

		ImVec4 area_color{ options_datas.m_area_highlight_color };
		uint32_t area_index{ 0 };

		if( m_hovered_color.m_first_area_hovered )
		{
			for( const PixelDesc* pixel_desc : m_hovered_color.m_pixel_areas.front() )
			{
				if( pixel_desc == nullptr )
					continue;

				add_neighbor_points( m_hovered_color.m_hovered_area_points, *pixel_desc, Direction::up );
				add_neighbor_points( m_hovered_color.m_hovered_area_points, *pixel_desc, Direction::down );
				add_neighbor_points( m_hovered_color.m_hovered_area_points, *pixel_desc, Direction::left );
				add_neighbor_points( m_hovered_color.m_hovered_area_points, *pixel_desc, Direction::right );
			}

			if( m_hovered_color.m_hovered_area_points.getVertexCount() > 0 )
			{
				m_hovered_color.m_hovered_area_line.set_thickness( options_datas.m_area_highlight_thickness );
				m_hovered_color.m_hovered_area_line.set_color( area_color );
				m_hovered_color.m_hovered_area_line.from_vertex_array( m_hovered_color.m_hovered_area_points );
			}

			++area_index;
			area_color = options_datas.m_area_secondary_highlight_color;
		}

		for( ; area_index < m_hovered_color.m_pixel_areas.size(); ++area_index )
		{
			for( const PixelDesc* pixel_desc : m_hovered_color.m_pixel_areas[ area_index ] )
			{
				if( pixel_desc == nullptr )
					continue;

				add_neighbor_points( m_hovered_color.m_colored_area_points, *pixel_desc, Direction::up );
				add_neighbor_points( m_hovered_color.m_colored_area_points, *pixel_desc, Direction::down );
				add_neighbor_points( m_hovered_color.m_colored_area_points, *pixel_desc, Direction::left );
				add_neighbor_points( m_hovered_color.m_colored_area_points, *pixel_desc, Direction::right );
			}

			if( m_hovered_color.m_colored_area_points.getVertexCount() > 0 )
			{
				m_hovered_color.m_colored_area_line.set_thickness( options_datas.m_area_secondary_highlight_thickness );
				m_hovered_color.m_colored_area_line.set_color( area_color );
				m_hovered_color.m_colored_area_line.from_vertex_array( m_hovered_color.m_colored_area_points );
			}
		}
	}

	bool CanvasManager::_is_pixel_in_current_area( uint32_t _pixel_index ) const
	{
		if( m_hovered_color.m_pixel_areas.empty() )
			return false;

		return std::ranges::find( m_hovered_color.m_pixel_areas.front(), _pixel_index, &PixelDesc::m_pixel_index ) != m_hovered_color.m_pixel_areas.front().end();
	}

	///////////////// IMGUI /////////////////

	void CanvasManager::_display_canvas( const sf::Color& _bg_color )
	{
		auto& options_datas{ g_pixeler->get_options().get_options_datas() };
		auto sprite_size{ m_canvas_size };

		m_sprite.setTextureRect( { 0, 0, (int)m_canvas_size.x, (int)m_canvas_size.y } );
		m_grid_sprite.setTextureRect( { 0, 0, (int)m_canvas_size.x, (int)m_canvas_size.y } );
		m_test_image_sprite.setTextureRect( { 0, 0, (int)m_canvas_size.x, (int)m_canvas_size.y } );	// Needed when resizing.
		m_sprite.setPosition( ImGui::GetWindowPos() /*+ sf::Vector2f{ 0.f, 1000.f }*/ );
		m_render_texture.clear( sf::Color::Transparent );

		if( m_converted_pixels.getVertexCount() > 0 )
		{
			m_test_texture.clear( sf::Color::Transparent );
			m_test_texture.draw( m_converted_pixels );
			m_test_texture.display();
			m_render_texture.draw( m_test_image_sprite );

			if( options_datas.m_show_original && options_datas.m_original_opacity_pct > 0.f )
				m_render_texture.draw( m_base_pixels );

			m_grid_texture.clear( sf::Color::Transparent );

			if( options_datas.m_show_grid )
			{
				m_grid_texture.draw( m_pixel_grid );
				m_grid_texture.display();

				if( options_datas.m_grid_same_color_as_canvas == false )
				{
					m_grid_shader->setUniform( "sprite_texture", *m_test_image_sprite.getTexture() );
					m_grid_shader->setUniform( "grid_texture", sf::Shader::CurrentTexture );
					m_grid_shader->setUniform( "texture_width", (float)m_test_texture.getSize().x );
					m_grid_shader->setUniform( "texture_height", (float)m_test_texture.getSize().y );
					m_grid_shader->setUniform( "grid_color", sf::Glsl::Vec4( options_datas.m_grid_color ) );
				}
				else
				{
					m_grid_shader->setUniform( "texture_width", 0.f );
					m_grid_shader->setUniform( "grid_color", sf::Glsl::Vec4( options_datas.m_canvas_background_color ) );
				}

				m_grid_shader->setUniform( "grid_texture", sf::Shader::CurrentTexture );
				m_render_texture.draw( m_grid_sprite, m_grid_shader );
			}
		}

		if( ( options_datas.m_show_secondary_highlight || m_hovered_color.m_first_area_hovered == false ) && m_hovered_color.m_colored_area_line.is_empty() == false )
			m_render_texture.draw( m_hovered_color.m_colored_area_line );

		if( m_hovered_color.m_hovered_area_line.is_empty() == false )
			m_render_texture.draw( m_hovered_color.m_hovered_area_line );

		m_render_texture.display();
		ImGui::Image( m_sprite );

		if( ImGui::IsItemHovered() )
			_mouse_detection();
		else
		{
			m_last_hovered_pixel_index = Uint32_Max;
			m_hovered_color.reset();
		}
	}

	void CanvasManager::_mouse_detection()
	{
		if( m_converted_pixels.getVertexCount() <= 0 )
		{
			m_hovered_color.reset();
			return;
		}

		const PixelPosition mouse_pixel_pos{ _get_mouse_pixel_pos() };
		const uint32_t pixel_index{ _get_1D_index( mouse_pixel_pos ) };

		if( pixel_index >= m_pixels_descs.size() )
		{
			m_hovered_color.reset();
			return;
		}

		const ColorInfos* color{ m_pixels_descs[ pixel_index ].m_color_infos };

		if( color == nullptr || color->is_valid() == false )
		{
			m_hovered_color.reset();
			return;
		}

		if( pixel_index != m_last_hovered_pixel_index )
		{
			if( _is_pixel_in_current_area( pixel_index ) == false )
			{
				_compute_pixel_area( pixel_index );
			}

			m_last_hovered_pixel_index = pixel_index;
		}

		// Tooltip padding depends on window padding, but we put it to 0 for the canvas.
		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, { 8, 8 } );
		ImGui::BeginTooltip();

		Utils::color_infos_tooltip_common( *color );

		ImGui::Separator();
		ImGui::Text( "Area count:" );
		ImGui::SameLine();
		if( m_hovered_color.m_pixel_areas.empty() )
			ImGui_fzn::bold_text( "0" );
		else
			ImGui_fzn::bold_text( "%d", m_hovered_color.m_pixel_areas.front().size() );

		ImGui::Text( "Total count:" );
		ImGui::SameLine();
		ImGui_fzn::bold_text( "%d", color->m_count );

		ImGui::Separator();
		ImGui::Text( "Original color" );
		Utils::color_details( m_pixels_descs[ pixel_index ].m_base_color );

		ImGui::EndTooltip();
		ImGui::PopStyleVar();
	}

	void CanvasManager::_display_bottom_bar()
	{
		auto* draw_list{ ImGui::GetWindowDrawList() };
		const auto region_max{ ImGui::GetWindowSize() };
		const auto frame_height_spacing{ ImGui::GetFrameHeightWithSpacing() };
		const auto bottom_bar_pos = ImGui::GetWindowPos() + ImVec2{ 0.f, region_max.y - frame_height_spacing };

		ImGui::GetWindowPos();

		if( draw_list != nullptr )
			draw_list->AddRectFilled( bottom_bar_pos, bottom_bar_pos + region_max, ImGui_fzn::get_color( ImGuiCol_MenuBarBg ) );

		auto cursor_pos_x{ ImGui::GetStyle().WindowPadding.x };
		auto cursor_pos_y{ region_max.y - frame_height_spacing + ImGui::GetStyle().ItemSpacing.y };

		ImGui::SetCursorPos( { cursor_pos_x, cursor_pos_y } );
		ImGui::SetNextItemWidth( DefaultWidgetSize.x );

		auto new_pixel_size{ m_zoom_level };
		if( ImGui_fzn::small_slider_float( "Zoom Level", new_pixel_size, 1.f, 100.f, "x%.0f", ImGuiSliderFlags_NoInput | ImGuiSliderFlags_AlwaysClamp ) )
			_update_zoom_level( new_pixel_size );

		ImGui::SameLine();

		if( ImGui::SmallButton( "Fit" ) )
			_fit_image();

		ImGui::SameLine();

		if( ImGui::SmallButton( "Convert" ) )
		{
			g_pixeler->get_palettes_manager().reset_color_counts();
			_convert_image_colors();
		}

		ImGui::SameLine();
		ImGui::TextColored( ImGui_fzn::color::dark_gray, "|" );
		ImGui::SameLine();
		g_pixeler->get_options().bottom_bar_options();
	}

} // namespace Pixeler
