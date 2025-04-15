#include <fstream>

#include <Externals/json/json.h>


#include <FZN/Managers/FazonCore.h>
#include <FZN/UI/ImGui.h>

#include "Options.h"
#include "Defines.h"


namespace PerlerMaker
{

	Options::Options()
	{
		g_pFZN_Core->AddCallback( this, &Options::on_event, fzn::DataCallbackType::Event );

		_load_options();
	}

	void Options::menu_bar_options()
	{
		ImGui::MenuItem( "Show pixel grid", "", &m_options_datas.m_show_grid );
	}

	void Options::show_window()
	{
		m_show_window = true;
		m_temp_options_datas = m_options_datas;
		m_need_save = false;

		g_pFZN_InputMgr->BackupActionKeys();
	}

	void Options::update()
	{
		if( g_pFZN_InputMgr->IsActionPressed( "Show Grid" ) )
			m_options_datas.m_show_grid = !m_options_datas.m_show_grid;

		if( m_show_window == false )
			return;

		auto second_column_widget = [&]( bool _widget_edited ) -> bool
		{
			if( _widget_edited )
				m_need_save = true;

			return _widget_edited;
		};

		ImGui::SetNextWindowSize( { 350.f, 0.f } );

		if( ImGui::Begin( "Options", nullptr, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse ) )
		{
			const float column_width = ImGui::GetContentRegionAvail().x * 0.45f;

			if( _begin_option_table( column_width ) )
			{
				_first_column_text( "Cavans color" );
				second_column_widget( ImGui::ColorEdit3( "##Canvas color", &m_options_datas.m_canvas_background_color.x, ImGuiColorEditFlags_NoInputs ) );

				ImGui::TableNextRow();
				_first_column_text( "Grid color" );
				second_column_widget( ImGui::Checkbox( "Same as canvas", &m_options_datas.m_grid_same_color_as_canvas ) );
				ImGui::SameLine();
				second_column_widget( ImGui::ColorEdit3( "##Grid color", &m_options_datas.m_grid_color.x, ImGuiColorEditFlags_NoInputs ) );

				ImGui::EndTable();
			}
			ImGui::SeparatorText( "Pixel Area Highlight" );

			if( _begin_option_table( column_width ) )
			{
				_first_column_text( "Color" );
				second_column_widget( ImGui::ColorEdit4( "##Color", &m_options_datas.m_area_highlight_color.x, ImGuiColorEditFlags_NoInputs ) );

				ImGui::TableNextRow();
				_first_column_text( "Thickness" );
				ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );
				if( second_column_widget( ImGui::SliderFloat( "##Thickness", &m_options_datas.m_area_highlight_thickness, 1.f, 20.f, "%.0f" ) ) )
				{
					if( m_options_datas.m_area_highlight_thickness < 1.f )
						m_options_datas.m_area_highlight_thickness = 1.f;
				}

				if( ImGui::IsItemHovered() )
					ImGui::SetTooltip( "Thickness will be updated when a new area is hovered" );

				ImGui::EndTable();
			}

			_draw_keybinds( column_width );

			ImGui::NewLine();
			ImGui::NewLine();
			ImGui::SameLine( ImGui::GetContentRegionMax().x - ( ImGui::GetStyle().WindowPadding.x + DefaultWidgetSize.x * 2.f ) );
			
			if( m_need_save && m_show_keybinds == false )
				ImGui::PushFont( ImGui_fzn::s_ImGuiFormatOptions.m_pFontBold );
			else
				ImGui::BeginDisabled();
			
			if( ImGui::Button( "Apply", DefaultWidgetSize ) )
			{
				m_show_window = false;
				_save_options();
			}

			if( m_need_save && m_show_keybinds == false )
				ImGui::PopFont();
			else
				ImGui::EndDisabled();

			if( ImGui::IsItemHovered() && m_show_keybinds )
				ImGui::SetTooltip( "Close the bindings window first." );

			ImGui::SameLine();
			if( ImGui::Button( "Cancel", DefaultWidgetSize ) )
			{
				m_show_window = false;
				m_show_keybinds = false;
				m_options_datas = m_temp_options_datas;
			}
		}

		ImGui::End();
	}

	void Options::on_event()
	{
		fzn::Event oEvent = g_pFZN_Core->GetEvent();

		if( oEvent.m_eType == fzn::Event::eActionKeyBindDone )
			m_need_save = true;
	}

	void Options::_draw_keybinds( float _column_width )
	{
		const bool popup_open{ g_pFZN_InputMgr->IsWaitingActionKeyBind() };
		std::string_view replaced_binding_name{};
		static std::string popup_name{};
		ImGui::SeparatorText( "Shortcuts" );

		if( ImGui::BeginTable( "Shortcuts", 3 ) )
		{
			ImGui::TableSetupColumn( "##Action", ImGuiTableColumnFlags_WidthFixed, _column_width );
			ImGui::TableSetupColumn( "##Bind", ImGuiTableColumnFlags_WidthStretch );
			ImGui::TableSetupColumn( "##Del.", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize( "Del." ).x + ImGui::GetStyle().FramePadding.x + ImGui::GetStyle().ItemSpacing.x );

			for( const fzn::ActionKey& action_key : g_pFZN_InputMgr->GetActionKeys() )
			{
				
				ImGui::PushID( &action_key );
				ImGui::TableNextRow();
				_first_column_text( action_key.m_sName.c_str() );

				if( ImGui::Button( g_pFZN_InputMgr->GetActionKeyString( action_key.m_sName, true, 0, false ).c_str(), { ImGui::GetContentRegionAvail().x, 0.f } ) )
				{
					g_pFZN_InputMgr->replace_action_key_bind( action_key.m_sName, fzn::InputManager::BindTypeFlag_All, 0 );
					replaced_binding_name = action_key.m_sName;
				}
				
				if( ImGui::IsItemHovered() )
					ImGui::SetTooltip( "Replace" );

				ImGui::TableNextColumn();
				if( ImGui::Button( "Del.", { ImGui::GetContentRegionAvail().x, 0.f } ) )
				{
					if( g_pFZN_InputMgr->RemoveActionKeyBind( action_key.m_sName, fzn::InputManager::BindType::eKey ) )
					{
						m_need_save = true;
					}
				}

				if( ImGui::IsItemHovered() )
					ImGui::SetTooltip( "Delete shortcut" );

				ImGui::PopID();
			}

			ImGui::EndTable();
		}

		if( popup_open != g_pFZN_InputMgr->IsWaitingActionKeyBind() )
		{
			popup_name = fzn::Tools::Sprintf( "Replace binding: %s", replaced_binding_name.data() );
			ImGui::OpenPopup( popup_name.c_str() );
		}

		if( g_pFZN_InputMgr->IsWaitingActionKeyBind() )
		{
			const ImVec2 title_size{ ImGui::CalcTextSize( popup_name.c_str() ) };
			static const ImVec2 text_size{ ImGui::CalcTextSize( "Press any key to replace this binding" ) };

			float popup_width{ text_size.x > title_size.x ? text_size.x : title_size.x + ImGui::GetStyle().WindowPadding.x * 2.f };
			sf::Vector2u window_size = g_pFZN_WindowMgr->GetWindowSize();

			ImGui::SetNextWindowPos( { window_size.x * 0.5f - popup_width * 0.5f, window_size.y * 0.5f - popup_width * 0.5f }, ImGuiCond_Appearing );
			ImGui::SetNextWindowSize( { popup_width, ImGui::GetFrameHeightWithSpacing() * 3.f } );

			if( ImGui::BeginPopupModal( popup_name.c_str(), nullptr, ImGuiWindowFlags_NoMove ) )
			{
				ImGui::NewLine();
				ImGui::Text( "Press any key to replace this binding" );

				ImGui::EndPopup();
			}
		}
	}

	void Options::_load_options()
	{
		auto file = std::ifstream{ g_pFZN_Core->GetSaveFolderPath() + "/options.json" };

		if( file.is_open() == false )
			return;

		auto root = Json::Value{};

		file >> root;

		auto load_color = [&root]( const char* _color_name, ImVec4& _color )
		{
			_color.x = root[ _color_name ][ ColorChannel::red ].asUInt() / 255.f;
			_color.y = root[ _color_name ][ ColorChannel::green ].asUInt() / 255.f;
			_color.z = root[ _color_name ][ ColorChannel::blue ].asUInt() / 255.f;
			_color.w = root[ _color_name ][ ColorChannel::alpha ].isNull() ? 1.f : root[ _color_name ][ ColorChannel::alpha ].asUInt() / 255.f;
		};

		load_color( "canvas_color", m_options_datas.m_canvas_background_color );
		load_color( "area_highlight_color", m_options_datas.m_area_highlight_color );
		load_color( "grid_color", m_options_datas.m_grid_color );

		m_options_datas.m_area_highlight_thickness = root[ "area_highlight_thickness" ].asFloat();
		m_options_datas.m_grid_same_color_as_canvas = root[ "grid_same_color_as_canvas" ].asBool();
		m_options_datas.m_show_grid = root[ "show_grid" ].asBool();

		m_options_datas.m_bindings = g_pFZN_InputMgr->GetActionKeys();
	}

	void Options::_save_options()
	{
		auto file = std::ofstream{ g_pFZN_Core->GetSaveFolderPath() + "/options.json" };
		auto root = Json::Value{};

		auto save_color = [&root]( const char* _color_name, const ImVec4& _color, bool _save_alpha = true )
		{
			root[ _color_name ][ ColorChannel::red ] = static_cast<Json::Value::UInt>( _color.x * 255.f );
			root[ _color_name ][ ColorChannel::green ] = static_cast<Json::Value::UInt>( _color.y * 255.f );
			root[ _color_name ][ ColorChannel::blue ] = static_cast<Json::Value::UInt>( _color.z * 255.f );

			if( _save_alpha )
				root[ _color_name ][ ColorChannel::alpha ] = static_cast<Json::Value::UInt>( _color.w * 255.f );
		};

		save_color( "canvas_color", m_options_datas.m_canvas_background_color, false );
		save_color( "area_highlight_color", m_options_datas.m_area_highlight_color );
		save_color( "grid_color", m_options_datas.m_grid_color, false );

		root[ "area_highlight_thickness" ] = m_options_datas.m_area_highlight_thickness;
		root[ "grid_same_color_as_canvas" ] = m_options_datas.m_grid_same_color_as_canvas;
		root[ "show_grid" ] = m_options_datas.m_show_grid;

		file << root;

		g_pFZN_InputMgr->SaveCustomActionKeysToFile();
	}

	bool Options::_begin_option_table( float _column_width )
	{
		const bool ret = ImGui::BeginTable( "options", 2 );

		ImGui::TableSetupColumn( "label", ImGuiTableColumnFlags_WidthFixed, _column_width );
		ImGui::TableNextRow();

		return ret;
	}

	void Options::_first_column_text( const char* _text )
	{
		ImGui::TableSetColumnIndex( 0 );
		ImGui::SameLine( ImGui::GetStyle().IndentSpacing * 0.5f );
		ImGui::Text( _text );
		ImGui::TableSetColumnIndex( 1 );
	}

} // namespace PerlerMaker
