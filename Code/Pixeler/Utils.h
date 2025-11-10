#pragma once

#include <functional>

namespace sf
{
	class Color;
}

class ImColor;

namespace Pixeler
{
	struct ColorInfos;
	struct ColorPalette;


	namespace Utils
	{
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// Convert the given ImColor into sf::Color
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		sf::Color to_sf_color( const ImColor& _color );
		ImColor to_imcolor( const sf::Color& _color );

		std::string get_zero_lead_id( int _id );

		void text_with_leading_zeros( std::string_view _text, bool _bold, bool _used, bool _shadow );
		void boldable_text( std::string_view _text, bool _bold, bool _used, bool _shadow );

		void color_details( const ImColor& _color );
		void color_details( const sf::Color& _color );

		void color_infos_tooltip_common( const ColorInfos& _color );

		void window_bottom_table( uint8_t _nb_items, std::function<void(void)> _table_content_fct );
	} // namespace Utils
} // namespace Pixeler
