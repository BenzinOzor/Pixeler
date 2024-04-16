//#include <SFML/Graphics.hpp>

#include <FZN/Managers/DataManager.h>
#include <FZN/Managers/WindowManager.h>
#include <FZN/UI/ImGui.h>
#include <FZN/Tools/Logging.h>

#include "CanvasManager.h"
#include "PerlerMaker.h"


namespace PerlerMaker
{
	enum ColorChannel
	{
		red,
		green,
		blue,
		alpha,
		COUNT
	};

	CanvasManager::CanvasManager()
	{
		sf::Vector2u window_size = g_pFZN_WindowMgr->GetWindowSize();

		m_render_texture.create( window_size.x, window_size.y );
		m_sprite.setTexture( m_render_texture.getTexture() );
	}

	void CanvasManager::update()
	{
		auto& options{ g_perler_maker->get_options() };
		auto& canvas_bg_color{ options.get_canvas_background_color() };
		ImGui::PushStyleColor( ImGuiCol_ChildBg, canvas_bg_color );

		if( ImGui::Begin( "Canvas" ) )
		{
			_display_canvas( canvas_bg_color );
		}

		ImGui::End();
		ImGui::PopStyleColor();
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
	}

	void CanvasManager::_load_pixels( sf::Texture* _texture )
	{
		if( _texture == nullptr )
			return;

		m_pixels.clear();

		const auto image{ _texture->copyToImage() };
		auto color_values{ image.getPixelsPtr() };
		const auto image_size{ image.getSize() };
		const auto max_pixel_index{ image_size.x * image_size.y * ColorChannel::COUNT };

		//const auto offsets = std::array

		auto process_pixel = [ this, &image_size ]( const uint8_t* _values, int _value_index )
		{
			const auto pixel_index{ _value_index / ColorChannel::COUNT };
			const auto pixel_position = sf::Vector2f{ pixel_index / image_size.x, pixel_index % image_size.x };


		};

		for( auto value_index{ 0 }; value_index < max_pixel_index; value_index += ColorChannel::COUNT, color_values += ColorChannel::COUNT )
		{
			auto pixel_position = sf::Vector2f{};
			auto test{ value_index / ColorChannel::COUNT };
			pixel_position.y = test / image_size.x;
			pixel_position.x = (float)(test % image_size.x);

			auto vertex = sf::Vertex{ pixel_position, { color_values[ ColorChannel::red ], color_values[ ColorChannel::green ], color_values[ ColorChannel::blue ], color_values[ ColorChannel::alpha ] } };

			m_pixels.append( std::move( vertex ) );
		}
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
} // namespace PerlerMaker
