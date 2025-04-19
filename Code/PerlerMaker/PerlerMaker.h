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

		const	CanvasManager&		get_canvas_manager() const		{ return m_canvas_manager; }
				CanvasManager&		get_canvas_manager()			{ return m_canvas_manager; }
		const	Options&			get_options() const				{ return m_options; }
				Options&			get_options()					{ return m_options; }
		const	PalettesManager&	get_palettes_manager() const	{ return m_palettes_manager; }
				PalettesManager&	get_palettes_manager()			{ return m_palettes_manager; }

	private:
		void _load_image();

		void _display_menu_bar();
		
		Options				m_options;
		PalettesManager		m_palettes_manager;
		CanvasManager		m_canvas_manager;
	};
};

extern PerlerMaker::CPerlerMaker* g_perler_maker;
