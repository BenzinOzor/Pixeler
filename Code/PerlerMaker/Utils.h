#pragma once

//#include <SFML/Graphics/Color.hpp>

namespace sf
{
	class Color;
}

class ImColor;

namespace PerlerMaker
{
	struct ColorInfos;


	namespace Utils
	{
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// Convert the given ImColor into sf::Color
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------
		sf::Color to_sf_color( const ImColor& _color );
		ImColor to_imcolor( const sf::Color& _color );

		void bicolor_color_name( std::string_view _color, bool _bold, bool _used );

		void color_details( const ImColor& _color );
		void color_details( const sf::Color& _color );

		void color_infos_tooltip_common( const ColorInfos& _color );
	} // namespace Utils
} // namespace PerlerMaker
