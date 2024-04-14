#pragma once

#include <Externals/ImGui/imgui.h>

#include "Options.h"
#include "PalettesManager.h"


namespace PerlerMaker
{
	class CPerlerMaker
	{
	public:
		CPerlerMaker();
		~CPerlerMaker();

		void display();

	private:
		void _load_image();

		void _display_menu_bar();
		void _display_canvas();

		sf::RenderTexture	m_render_texture;
		sf::Sprite			m_default_image_sprite;
		sf::Sprite			m_sprite;
		
		Options				m_options;
		PalettesManager		m_palettes_manager;
	};
};
