#pragma once

#include <FZN/Tools/Tools.h>


namespace PerlerMaker
{
	static const std::string		color_preset_all			{ "All" };		// The default preset containing all the colors of a palette. It is automatically added to a palette and is not in its xml file.

	static constexpr const char*	Action_ShowGrid				{ "Show Grid" };
	static constexpr const char*	Action_ShowOriginalSprite	{ "Show Original Sprite" };

	static constexpr const char*	Tooltip_ShowGrid			{ "Display a grid over the sprite to visually separate pixels" };
	static constexpr const char*	Tooltip_ShowOriginal		{ "Display the original sprite on top of the palette converted one" };
	static constexpr const char*	Tooltip_OriginalOpacity		{ "Change the opacity of the original sprite" };

	enum ColorChannel
	{
		red,
		green,
		blue,
		alpha,
		COUNT
	};

	enum class Direction
	{
		up,
		down,
		left,
		right,
		COUNT
	};

	static constexpr ImVec2 DefaultWidgetSize{ 150.f, 0.f };

	inline constexpr float		Flt_Max{ std::numeric_limits<float>::max() };
	inline constexpr uint32_t	Uint32_Max{ std::numeric_limits<uint32_t>::max() };

	struct PixelPosition
	{
		uint32_t x{ 0 };
		uint32_t y{ 0 };
	};
} // namsepace PerlerMaker
