#include <filesystem>

#include <FZN/Managers/FazonCore.h>
#include <FZN/Managers/DataManager.h>
#include <FZN/UI/ImGui.h>

#include "Pixeler/Pixeler.h"


Pixeler::CPixeler* g_pixeler = nullptr;

namespace Pixeler
{
	CPixeler::CPixeler()
	{
		g_pFZN_Core->AddCallback( this, &CPixeler::display, fzn::DataCallbackType::Display );

		auto& oIO = ImGui::GetIO();

		oIO.ConfigWindowsMoveFromTitleBarOnly = true;
		oIO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		oIO.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		ImGui_fzn::s_ImGuiFormatOptions.m_pFontRegular = oIO.Fonts->AddFontFromFileTTF( DATAPATH( "Display/Fonts/Rubik Light Regular.ttf" ), 16.f );
		ImGui_fzn::s_ImGuiFormatOptions.m_pFontBold = oIO.Fonts->AddFontFromFileTTF( DATAPATH( "Display/Fonts/Rubik Light Bold.ttf" ), 16.f );

		g_pixeler = this;
	}

	CPixeler::~CPixeler()
	{
		g_pFZN_Core->RemoveCallback( this, &CPixeler::display, fzn::DataCallbackType::Display );
	}

	void CPixeler::display()
	{
		const auto window_size = g_pFZN_WindowMgr->GetWindowSize();

		ImGui::SetNextWindowPos( { 0.f, 0.f } );
		ImGui::SetNextWindowSize( window_size );
		ImGui::PushStyleVar( ImGuiStyleVar_::ImGuiStyleVar_IndentSpacing, 35.f );
		ImGui::PushStyleVar( ImGuiStyleVar_::ImGuiStyleVar_WindowRounding, 0.f );
		ImGui::PushStyleVar( ImGuiStyleVar_::ImGuiStyleVar_WindowBorderSize, 0.f );
		ImGui::PushStyleVar( ImGuiStyleVar_::ImGuiStyleVar_WindowPadding, ImVec2( 0.0f, 0.0f ) );
		ImGui::PushStyleColor( ImGuiCol_WindowBg, ImVec4( 0.10f, 0.16f, 0.22f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_CheckMark, ImVec4( 0.f, 1.f, 0.f, 1.f ) );

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar;
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		window_flags |= ImGuiWindowFlags_NoBackground;

		ImGui::Begin( "Pixeler", NULL, window_flags );
		ImGui::PopStyleVar( 3 );

		_display_menu_bar();

		auto dockspace_id = ImGui::GetID( "perler_maker_dock_space" );
		ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
		ImGui::DockSpace( dockspace_id, ImVec2( 0.0f, 0.0f ), dockspace_flags );

		m_canvas_manager.update();
		m_palettes_manager.update();
		m_options.update();

		ImGui::End();

		ImGui::PopStyleVar( 1 );
		ImGui::PopStyleColor( 2 );
	}

	void CPixeler::_load_image()
	{
		char file[ 100 ];
		OPENFILENAME open_file_name;
		ZeroMemory( &open_file_name, sizeof( open_file_name ) );

		open_file_name.lStructSize = sizeof( open_file_name );
		open_file_name.hwndOwner = NULL;
		open_file_name.lpstrFile = file;
		open_file_name.lpstrFile[ 0 ] = '\0';
		open_file_name.nMaxFile = sizeof( file );
		open_file_name.lpstrFileTitle = NULL;
		open_file_name.nMaxFileTitle = 0;
		GetOpenFileName( &open_file_name );

		if( open_file_name.lpstrFile[ 0 ] != '\0' )
			m_canvas_manager.load_texture( open_file_name.lpstrFile );
	}

	void CPixeler::_display_menu_bar()
	{
		if( ImGui::BeginMainMenuBar() )
		{
			if( ImGui::BeginMenu( "File" ) )
			{
				if( ImGui::MenuItem( "Load Image" ) )
					_load_image();

				ImGui::Separator();
				ImGui::PushStyleColor( ImGuiCol_HeaderHovered, ImGui_fzn::color::dark_red );
				if( ImGui::MenuItem( "Restore Base Palettes Default Values" ) )
					m_palettes_manager.reset_base_palettes();
				ImGui::PopStyleColor();

				if( ImGui::IsItemHovered() )
					ImGui::SetTooltip( "Restore default colors and presets of palettes located in '%s'.\nDoesn't affect custom palettes.", std::string{ g_pFZN_Core->GetSaveFolderPath() + "/Palettes/Base" }.c_str() );

				//if( ImGui::MenuItem( "Sauver Perler" ) ) {}
				//if( ImGui::MenuItem( "Sauver Perler Sous..." ) ) {}

				ImGui::EndMenu();
			}

			if( ImGui::BeginMenu( "Tools" ) )
			{
				m_options.menu_bar_options();
				ImGui::Separator();
				if( ImGui::MenuItem( "Options" ) )
					m_options.show_window();

				ImGui::EndMenu();
			}

			sf::Vector2u window_size = g_pFZN_WindowMgr->GetWindowSize();
			std::string version{ "Ver. 0.1.0.0" };
			ImGui::SameLine( window_size.x - ImGui::CalcTextSize( version.c_str() ).x - 2.f * ImGui::GetStyle().ItemSpacing.x );
			ImGui::TextColored( ImGui_fzn::color::light_gray, version.c_str() );

			ImGui::EndMainMenuBar();
		}
	}
};
