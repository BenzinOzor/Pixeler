#include <Externals/ImGui/imgui.h>
#include <Externals/ImGui/imgui_internal.h>
#include <Externals/ImGui/imgui-SFML.h>

#include <FZN/Managers/FazonCore.h>
#include <FZN/Managers/DataManager.h>

#include "PerlerMaker/PerlerMaker.h"


namespace PerlerMaker
{

	CPerlerMaker::CPerlerMaker()
	{
		g_pFZN_Core->AddCallback( this, &CPerlerMaker::display, fzn::DataCallbackType::Display );

		auto& oIO = ImGui::GetIO();

		oIO.ConfigWindowsMoveFromTitleBarOnly = true;
		oIO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		oIO.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		sf::Vector2u window_size = g_pFZN_WindowMgr->GetWindowSize();

		m_render_texture.create( window_size.x, window_size.y );
		m_sprite.setTexture( m_render_texture.getTexture() );
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
		ImGui::PushStyleColor( ImGuiCol_WindowBg, ImVec4( 0.2f, 0.2f, 0.2f, 1.f ) );
		//ImGui::GetStyle().Colors[ ImGuiCol_CheckMark ] = ImVec4( 0.f, 1.f, 0.f, 1.f );

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar;
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		window_flags |= ImGuiWindowFlags_NoBackground;

		ImGui::Begin( "Perler Maker", NULL, window_flags );
		ImGui::PopStyleVar( 3 );

		if( ImGui::BeginMainMenuBar() )
		{
			if( ImGui::BeginMenu( "File" ) )
			{
				if( ImGui::MenuItem( "Load Image" ) )
					load_image();

				if( ImGui::MenuItem( "Save Perler" ) ) {}
				if( ImGui::MenuItem( "Save Perler As..." ) ) {}

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		auto dockspace_id = ImGui::GetID( "perler_maker_dock_space" );
		ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
		ImGui::DockSpace( dockspace_id, ImVec2( 0.0f, 0.0f ), dockspace_flags );

		display_canvas();
		display_colors_selector();

		ImGui::End();

		ImGui::PopStyleVar( 1 );
		ImGui::PopStyleColor();
	}

	void CPerlerMaker::load_image()
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
		{
			g_pFZN_DataMgr->UnloadTexture( "Perler Default Image" );

			auto texture = g_pFZN_DataMgr->LoadTexture( "Perler Default Image", open_file_name.lpstrFile );

			if( texture != nullptr )
				m_default_image_sprite.setTexture( *texture );
		}
	}

	void CPerlerMaker::display_canvas()
	{
		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.f, 0.f ) );
		if( ImGui::Begin( "Perler Canvas" ) )
		{
			ImGui::PopStyleVar( 1 );

			m_render_texture.clear();
			m_render_texture.draw( m_default_image_sprite );
			m_render_texture.display();

			ImGui::Image( m_sprite );
		}
		else
			ImGui::PopStyleVar( 1 );

		ImGui::End();
	}

	void CPerlerMaker::display_colors_selector()
	{
		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.f, 0.f ) );
		if( ImGui::Begin( "Colors Selector" ) )
		{
			ImGui::PopStyleVar( 1 );

			ImGui::Checkbox( "##White", &m_selected_colors[ (uint32_t) BeadColor::White ] );
			ImGui::SameLine();
			ImGui::TextColored( {255, 255, 255, 255}, "01 - White" );

			ImGui::Checkbox( "##Red", &m_selected_colors[ (uint32_t)BeadColor::Red ] );
			ImGui::SameLine();
			ImGui::TextColored( { 255, 0, 0, 255 }, "05 - Red" );

			ImGui::Checkbox( "##Blue", &m_selected_colors[ (uint32_t)BeadColor::Blue ] );
			ImGui::SameLine();
			ImGui::TextColored( { 0, 0, 255, 255 }, "08 - Blue" );

			ImGui::Checkbox( "##Green", &m_selected_colors[ (uint32_t)BeadColor::Green ] );
			ImGui::SameLine();
			ImGui::TextColored( { 0, 255, 0, 255 }, "10 - Green" );

			ImGui::Checkbox( "##Black", &m_selected_colors[ (uint32_t)BeadColor::Black ] );
			ImGui::SameLine();
			ImGui::TextColored( { 0, 0, 0, 255 }, "18 - Black" );
		}
		else
			ImGui::PopStyleVar( 1 );

		ImGui::End();
	}

};
