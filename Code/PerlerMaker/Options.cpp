#include "Options.h"


namespace PerlerMaker
{

	Options::Options()
	{
		_load_options();
	}

	void Options::show_window()
	{
		m_show_window = true;
	}

	void Options::update()
	{
		if( m_show_window == false )
			return;

		//ImGui::SetNextWindowFocus();

		if( ImGui::Begin( "Options", &m_show_window, ImGuiWindowFlags_::ImGuiWindowFlags_NoDocking ) )
		{
			ImGui::ColorEdit4( "Couleur du canvas", &m_canvas_background_color.x, ImGuiColorEditFlags_NoInputs );
		}

		ImGui::End();
	}

	void Options::_load_options()
	{

	}

	void Options::_save_options()
	{

	}

} // namespace PerlerMaker
