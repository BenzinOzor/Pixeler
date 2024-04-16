#pragma once

#include <Externals/ImGui/imgui.h>

#include "Options.h"
#include "PalettesManager.h"
#include "CanvasManager.h"


namespace PerlerMaker
{
	class CPerlerMaker
	{
	public:
		CPerlerMaker();
		~CPerlerMaker();

		void display();

		const Options& get_options() const { return m_options; }

	private:
		void _load_image();

		void _display_menu_bar();
		
		Options				m_options;
		PalettesManager		m_palettes_manager;
		CanvasManager		m_canvas_manager;
	};
};

extern PerlerMaker::CPerlerMaker* g_perler_maker;
