//#include <SFML/Graphics.hpp>

#include <FZN/Managers/DataManager.h>
#include <FZN/Managers/WindowManager.h>
#include <FZN/UI/ImGui.h>
#include <FZN/Tools/Logging.h>

#include "CanvasManager.h"
#include "PerlerMaker.h"
#include "Defines.h"


namespace PerlerMaker
{
	CanvasManager::CanvasManager()
	{
		g_pFZN_Core->AddCallback( this, &CanvasManager::on_event, fzn::DataCallbackType::Event );

		sf::Vector2u window_size = g_pFZN_WindowMgr->GetWindowSize();
		
		m_render_texture.create( window_size.x, window_size.y );
		m_sprite.setTexture( m_render_texture.getTexture() );

		m_pixels.setPrimitiveType( sf::PrimitiveType::Quads );

		m_offsets[ 0 ] = { 0.f,				0.f };
		m_offsets[ 1 ] = { m_pixel_size,	0.f };
		m_offsets[ 2 ] = { m_pixel_size,	m_pixel_size };
		m_offsets[ 3 ] = { 0.f,				m_pixel_size };
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
		}
	}

	void CanvasManager::update()
	{
		auto& options{ g_perler_maker->get_options() };
		auto& canvas_bg_color{ options.get_canvas_background_color() };
		ImGui::PushStyleColor( ImGuiCol_ChildBg, canvas_bg_color );
		ImGui::PushStyleColor( ImGuiCol_WindowBg, canvas_bg_color );
		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, { 0, 0 } );

		if( ImGui::Begin( "Canvas" ) )
		{
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

		g_pFZN_DataMgr->UnloadTexture( "Perler Default Image" );

		auto texture = g_pFZN_DataMgr->LoadTexture( "Perler Default Image", _path.data() );

		if( texture != nullptr )
			m_default_image_sprite.setTexture( *texture );

		_load_pixels( texture );
		_fit_image();
	}

	void CanvasManager::_load_pixels( sf::Texture* _texture )
	{
		if( _texture == nullptr )
			return;

		m_pixels.clear();

		const auto image{ _texture->copyToImage() };
		auto color_values{ image.getPixelsPtr() };
		m_image_size = image.getSize();
		const auto max_pixel_index{ m_image_size.x * m_image_size.y * ColorChannel::COUNT };

		for( auto value_index{ 0u }; value_index < max_pixel_index; value_index += ColorChannel::COUNT, color_values += ColorChannel::COUNT )
		{
			const auto pixel_index{ value_index / ColorChannel::COUNT };
			auto pixel_position = sf::Vector2f{};
			pixel_position.x = (pixel_index % m_image_size.x) * m_pixel_size;
			pixel_position.y = (pixel_index / m_image_size.x) * m_pixel_size;
			const auto pixel_color = sf::Color{ color_values[ ColorChannel::red ], color_values[ ColorChannel::green ], color_values[ ColorChannel::blue ], color_values[ ColorChannel::alpha ] };

			m_pixels.append( { { pixel_position + m_offsets[ 0 ] }, pixel_color } );
			m_pixels.append( { { pixel_position + m_offsets[ 1 ] }, pixel_color } );
			m_pixels.append( { { pixel_position + m_offsets[ 2 ] }, pixel_color } );
			m_pixels.append( { { pixel_position + m_offsets[ 3 ] }, pixel_color } );
		}
	}

	void CanvasManager::_fit_image()
	{
		const auto window_size{ ImGui::GetWindowContentRegionMax() };

		const auto left_margin{ ( window_size.x - m_image_size.x ) * 0.5f };
		const auto top_margin{ ( window_size.y - m_image_size.y ) * 0.5f };

		// move image by margin size, pixel size = margin * 2
		if( left_margin < top_margin )
		{
			_update_pixel_size( left_margin * 2.f );
			//_set_vertex_array_pos( { 0.f, window_size.y * 0.5f - (m_image_size.y * m_pixel_size) * 0.5f } );
		}
		else
		{
			_update_pixel_size( top_margin * 2.f );
			//_set_vertex_array_pos( { window_size.x * 0.5f - ( m_image_size.x * m_pixel_size ) * 0.5f, 0.f } );
		}
	}

	void CanvasManager::_set_vertex_array_pos( const sf::Vector2f& _pos )
	{
		for( auto pixel_index{ 0 }; pixel_index < m_pixels.getVertexCount(); ++pixel_index )
		{

		}
	}

	void CanvasManager::_update_pixel_size( float _new_pixel_size )
	{
		if( _new_pixel_size == m_pixel_size )
			return;

		auto quad_index{ 0 };

		auto set_new_pos = [ this, &quad_index, &_new_pixel_size ]( int _quad_corner_index )
		{
			auto base_index{ quad_index / 4 };
			auto base_pos = sf::Vector2f{ ( base_index % m_image_size.x ) * _new_pixel_size, ( base_index / m_image_size.x ) * _new_pixel_size };

			m_pixels[ quad_index + _quad_corner_index ].position = base_pos + m_offsets[ _quad_corner_index ] * _new_pixel_size;
		};

		for( ; quad_index < m_pixels.getVertexCount(); quad_index += 4 )
		{
			set_new_pos( 0 );
			set_new_pos( 1 );
			set_new_pos( 2 );
			set_new_pos( 3 );
		}

		m_pixel_size = _new_pixel_size;
	}

	void CanvasManager::_display_canvas( const sf::Color& _bg_color )
	{
		auto sprite_size{ ImGui::GetContentRegionAvail() };
		m_sprite.setTextureRect( { 0, 0, (int)sprite_size.x, (int)sprite_size.y } );
		m_render_texture.clear( _bg_color );
		m_render_texture.draw( m_pixels );
		m_render_texture.display();

		ImGui::Image( m_sprite );
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
		ImGui::SetNextItemWidth( 150.f );

		auto new_pixel_size{ m_pixel_size };
		if( ImGui_fzn::small_slider_float( "Pixel size", &new_pixel_size, 1.f, 100.f, "%.0f" ) )
			_update_pixel_size( new_pixel_size );
	}

} // namespace PerlerMaker
