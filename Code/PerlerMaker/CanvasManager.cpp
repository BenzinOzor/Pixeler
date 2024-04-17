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

		m_pixels.setPrimitiveType( sf::PrimitiveType::Quads );
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

		for( auto value_index{ 0u }; value_index < max_pixel_index; value_index += ColorChannel::COUNT, color_values += ColorChannel::COUNT )
		{
			const auto pixel_index{ value_index / ColorChannel::COUNT };
			auto pixel_position = sf::Vector2f{};
			pixel_position.x = (pixel_index % image_size.x) * m_pixel_size;
			pixel_position.y = (pixel_index / image_size.x) * m_pixel_size;
			const auto pixel_color = sf::Color{ color_values[ ColorChannel::red ], color_values[ ColorChannel::green ], color_values[ ColorChannel::blue ], color_values[ ColorChannel::alpha ] };

			auto offsets = std::array< sf::Vector2f, 4 >{};
			offsets[ 0 ] = { 0.f,			0.f };
			offsets[ 1 ] = { m_pixel_size,	0.f };
			offsets[ 2 ] = { m_pixel_size,	m_pixel_size };
			offsets[ 3 ] = { 0.f,			m_pixel_size };

			m_pixels.append( { { pixel_position + offsets[ 0 ] }, pixel_color } );
			m_pixels.append( { { pixel_position + offsets[ 1 ] }, pixel_color } );
			m_pixels.append( { { pixel_position + offsets[ 2 ] }, pixel_color } );
			m_pixels.append( { { pixel_position + offsets[ 3 ] }, pixel_color } );
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

	void CanvasManager::_display_bottom_bar()
	{
		auto* draw_list{ ImGui::GetWindowDrawList() };
		const auto region_max{ ImGui::GetWindowSize() };
		const auto frame_height_spacing{ ImGui::GetFrameHeightWithSpacing() };
		const auto bottom_bar_pos = ImGui::GetWindowPos() + ImVec2{ 0.f, region_max.y - frame_height_spacing };

		ImGui::GetWindowPos();

		ImGui::DrawRectFilled( { 0, 0, 10, 10 }, {255, 0, 0, 150 } );

		if( draw_list != nullptr )
			draw_list->AddRectFilled( bottom_bar_pos, bottom_bar_pos + region_max, ImGui_fzn::get_color( ImGuiCol_MenuBarBg ) );

		auto cursor_pos_x{ ImGui::GetStyle().WindowPadding.x };
		auto cursor_pos_y{ region_max.y - frame_height_spacing + ImGui::GetStyle().ItemSpacing.y };

		ImGui::SetCursorPos( { cursor_pos_x, cursor_pos_y } );
		ImGui::SliderFloat( "Pixel size", &m_pixel_size, 1.f, 100.f, "%.0f" ); // set size thinner
	}

} // namespace PerlerMaker
