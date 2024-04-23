#include <FZN/Managers/FazonCore.h>
#include <FZN/Managers/DataManager.h>
#include <FZN/UI/ImGui.h>

#include "PerlerMaker/PerlerMaker.h"


PerlerMaker::CPerlerMaker* g_perler_maker = nullptr;

namespace PerlerMaker
{
	CPerlerMaker::CPerlerMaker()
	{
		g_pFZN_Core->AddCallback( this, &CPerlerMaker::display, fzn::DataCallbackType::Display );

		auto& oIO = ImGui::GetIO();

		oIO.ConfigWindowsMoveFromTitleBarOnly = true;
		oIO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		oIO.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		ImGui_fzn::s_ImGuiFormatOptions.m_pFontRegular = oIO.Fonts->AddFontFromFileTTF( DATAPATH( "Display/Fonts/Rubik Light Regular.ttf" ), 16.f );
		ImGui_fzn::s_ImGuiFormatOptions.m_pFontBold = oIO.Fonts->AddFontFromFileTTF( DATAPATH( "Display/Fonts/Rubik Light Bold.ttf" ), 16.f );

		g_perler_maker = this;
	}

	CPerlerMaker::~CPerlerMaker()
	{
		g_pFZN_Core->RemoveCallback( this, &CPerlerMaker::display, fzn::DataCallbackType::Display );
	}

	void CPerlerMaker::display()
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

		ImGui::Begin( "Perler Maker", NULL, window_flags );
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

	void CPerlerMaker::_load_image()
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

	void CPerlerMaker::_display_menu_bar()
	{
		if( ImGui::BeginMainMenuBar() )
		{
			if( ImGui::BeginMenu( "Fichier" ) )
			{
				if( ImGui::MenuItem( "Charger Image" ) )
					_load_image();

				//if( ImGui::MenuItem( "Saver Perler" ) ) {}
				//if( ImGui::MenuItem( "Saver Perler Sous..." ) ) {}

				ImGui::EndMenu();
			}

			if( ImGui::BeginMenu( "Outils" ) )
			{
				if( ImGui::MenuItem( "Options" ) )
					m_options.show_window();

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}
	}
};
