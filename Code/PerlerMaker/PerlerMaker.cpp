#include <FZN/Managers/FazonCore.h>
#include <FZN/Managers/DataManager.h>
#include <FZN/UI/ImGui.h>

#include "PerlerMaker/PerlerMaker.h"
#include "PerlerMaker/Utils.h"



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

		sf::Vector2u window_size = g_pFZN_WindowMgr->GetWindowSize();

		m_render_texture.create( window_size.x, window_size.y );
		m_sprite.setTexture( m_render_texture.getTexture() );

		_initialize_beads_array();
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
					_load_image();

				if( ImGui::MenuItem( "Save Perler" ) ) {}
				if( ImGui::MenuItem( "Save Perler As..." ) ) {}

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		auto dockspace_id = ImGui::GetID( "perler_maker_dock_space" );
		ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
		ImGui::DockSpace( dockspace_id, ImVec2( 0.0f, 0.0f ), dockspace_flags );

		_display_canvas();
		_display_colors_selector();

		ImGui::End();

		ImGui::PopStyleVar( 1 );
		ImGui::PopStyleColor( 2 );
	}

	void CPerlerMaker::_initialize_beads_array()
	{
		auto fill_bead_infos = [ &beads = m_beads ]( BeadInfos _bead_infos )
		{
			beads[ (size_t)_bead_infos.m_type ] = std::move( _bead_infos );
		};

		fill_bead_infos( { BeadColor::White,	"White",	1,	{ 255, 255, 255 } } );
		fill_bead_infos( { BeadColor::Red,		"Red",		5,	{ 255, 0, 0 } } );
		fill_bead_infos( { BeadColor::Green,	"Green",	10, { 0, 255, 0 } } );
		fill_bead_infos( { BeadColor::Blue,		"Blue",		9,	{ 0, 0, 255 } } );
		fill_bead_infos( { BeadColor::Black,	"Black",	18, { 0, 0, 0 } } );
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
		{
			g_pFZN_DataMgr->UnloadTexture( "Perler Default Image" );

			auto texture = g_pFZN_DataMgr->LoadTexture( "Perler Default Image", open_file_name.lpstrFile );

			if( texture != nullptr )
				m_default_image_sprite.setTexture( *texture );
		}
	}

	void CPerlerMaker::_display_canvas()
	{
		ImGui::PushStyleColor( ImGuiCol_ChildBg, { 0, 0, 0, 255 } );

		if( ImGui::Begin( "Perler Canvas" ) )
		{
			auto sprite_size{ ImGui::GetContentRegionAvail() };
			m_sprite.setTextureRect( { 0, 0, (int)sprite_size.x, (int)sprite_size.y } );
			m_render_texture.clear();
			m_render_texture.draw( m_default_image_sprite );
			m_render_texture.display();

			ImGui::Image( m_sprite );
		}

		ImGui::End();
		ImGui::PopStyleColor();
	}

	void CPerlerMaker::_display_colors_selector()
	{
		if( ImGui::Begin( "Colors Selector" ) )
		{
			for( auto& bead : m_beads )
				Utils::selectable_bead_info( bead );
		}

		ImGui::End();
	}

};
