#pragma once

//#include <SFML/Graphics/Color.hpp>

namespace sf
{
	class Color;
}

class ImColor;

namespace PerlerMaker::Utils
{
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Convert the given ImColor into sf::Color
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------
	sf::Color to_sf_color( const ImColor& _color );
} // namespace PerlerMaker
