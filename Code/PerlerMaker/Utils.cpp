#include <SFML/Graphics/Color.hpp>

#include <Externals/ImGui/imgui.h>

#include "Utils.h"


namespace PerlerMaker::Utils
{
	sf::Color to_sf_color( const ImColor& _color )
	{
		return { sf::Uint8(_color.Value.x * 255.f), sf::Uint8( _color.Value.y * 255.f), sf::Uint8( _color.Value.z * 255.f), sf::Uint8( _color.Value.w * 255.f) };
	}
} // namespace PerlerMaker

