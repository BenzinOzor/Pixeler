#include <fstream>

#include <Externals/json/json.h>

#include <FZN/Managers/FazonCore.h>

#include "Options.h"
#include "Defines.h"


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
		auto file = std::ifstream{ g_pFZN_Core->GetSaveFolderPath() + "/options.json" };
		auto root = Json::Value{};

		file >> root;

		m_canvas_background_color.x = root[ "canvas_color" ][ ColorChannel::red ].asUInt() / 255.f;
		m_canvas_background_color.y = root[ "canvas_color" ][ ColorChannel::green ].asUInt() / 255.f;
		m_canvas_background_color.z = root[ "canvas_color" ][ ColorChannel::blue ].asUInt() / 255.f;
		m_canvas_background_color.w = 1.f;
	}

	void Options::_save_options()
	{

	}

} // namespace PerlerMaker
