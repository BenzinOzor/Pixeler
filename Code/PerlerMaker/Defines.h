#pragma once

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

	static constexpr ImVec2 DefaultButtonSize{ 150.f, 0.f };

	inline constexpr float Flt_Max{ std::numeric_limits<float>::max() };
} // namsepace PerlerMaker
