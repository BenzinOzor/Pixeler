#include <fstream>

#include <Externals/json/json.h>

#include <FZN/Managers/FazonCore.h>
#include <FZN/UI/ImGuiAdditions.h>

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
		m_temp_options_datas = m_options_datas;
		m_need_save = false;
	}

	void Options::update()
	{
		if( m_show_window == false )
			return;

		ImGui::SetNextWindowSize( { 350.f, 0.f } );

		if( ImGui::Begin( "Options", nullptr, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse ) )
		{
			const float column_width = ImGui::GetContentRegionAvail().x * 0.45f;

			if( _begin_option_table( column_width ) )
			{
				_first_column_text( "Cavans color" );
				if( ImGui::ColorEdit3( "##Canvas color", &m_options_datas.m_canvas_background_color.x, ImGuiColorEditFlags_NoInputs ) )
					m_need_save = true;

				ImGui::EndTable();
			}
			ImGui::SeparatorText( "Pixel Area Highlight" );

			if( _begin_option_table( column_width ) )
			{
				_first_column_text( "Color" );
				ImVec4 highlight_color( m_options_datas.m_area_highlight_color );
				if( ImGui::ColorEdit4( "##Color", &highlight_color.x, ImGuiColorEditFlags_NoInputs ) )
				{
					m_options_datas.m_area_highlight_color = highlight_color;
					m_need_save = true;
				}

				ImGui::TableNextRow();
				_first_column_text( "Thickness" );
				ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );
				if( ImGui::SliderFloat( "##Thickness", &m_options_datas.m_area_highlight_thickness, 1.f, 20.f, "%.0f" ) )
				{
					if( m_options_datas.m_area_highlight_thickness < 1.f )
						m_options_datas.m_area_highlight_thickness = 1.f;

					m_need_save = true;
				}

				if( ImGui::IsItemHovered() )
					ImGui::SetTooltip( "Thickness will be updated when a new area is hovered" );

				ImGui::EndTable();
			}

			ImGui::NewLine();
			ImGui::NewLine();
			ImGui::SameLine( ImGui::GetContentRegionMax().x - ( ImGui::GetStyle().WindowPadding.x + DefaultWidgetSize.x * 2.f ) );
			
			if( m_need_save )
				ImGui::PushFont( ImGui_fzn::s_ImGuiFormatOptions.m_pFontBold );
			else
				ImGui::BeginDisabled();
			
			if( ImGui::Button( "Apply", DefaultWidgetSize ) )
			{
				m_show_window = false;
				_save_options();
			}

			if( m_need_save )
				ImGui::PopFont();
			else
				ImGui::EndDisabled();

			ImGui::SameLine();
			if( ImGui::Button( "Cancel", DefaultWidgetSize ) )
			{
				m_show_window = false;
				m_options_datas = m_temp_options_datas;
			}
		}

		ImGui::End();
	}

	void Options::_load_options()
	{
		auto file = std::ifstream{ g_pFZN_Core->GetSaveFolderPath() + "/options.json" };
		auto root = Json::Value{};

		file >> root;

		m_options_datas.m_canvas_background_color.x = root[ "canvas_color" ][ ColorChannel::red ].asUInt() / 255.f;
		m_options_datas.m_canvas_background_color.y = root[ "canvas_color" ][ ColorChannel::green ].asUInt() / 255.f;
		m_options_datas.m_canvas_background_color.z = root[ "canvas_color" ][ ColorChannel::blue ].asUInt() / 255.f;
		m_options_datas.m_canvas_background_color.w = 1.f;

		m_options_datas.m_area_highlight_color.r = root[ "area_highlight_color" ][ ColorChannel::red ].asUInt();
		m_options_datas.m_area_highlight_color.g = root[ "area_highlight_color" ][ ColorChannel::green ].asUInt();
		m_options_datas.m_area_highlight_color.b = root[ "area_highlight_color" ][ ColorChannel::blue ].asUInt();
		m_options_datas.m_area_highlight_color.a = root[ "area_highlight_color" ][ ColorChannel::alpha ].asUInt();

		m_options_datas.m_area_highlight_thickness = root[ "area_highlight_thickness" ].asFloat();
	}

	void Options::_save_options()
	{
		auto file = std::ofstream{ g_pFZN_Core->GetSaveFolderPath() + "/options.json" };
		auto root = Json::Value{};

		root[ "canvas_color" ][ ColorChannel::red ] = static_cast<Json::Value::UInt>( m_options_datas.m_canvas_background_color.x * 255.f );
		root[ "canvas_color" ][ ColorChannel::green ] = static_cast<Json::Value::UInt>( m_options_datas.m_canvas_background_color.y * 255.f );
		root[ "canvas_color" ][ ColorChannel::blue ] = static_cast<Json::Value::UInt>( m_options_datas.m_canvas_background_color.z * 255.f );

		root[ "area_highlight_color" ][ ColorChannel::red ] = m_options_datas.m_area_highlight_color.r;
		root[ "area_highlight_color" ][ ColorChannel::green ] = m_options_datas.m_area_highlight_color.g;
		root[ "area_highlight_color" ][ ColorChannel::blue ] = m_options_datas.m_area_highlight_color.b;
		root[ "area_highlight_color" ][ ColorChannel::alpha ] = m_options_datas.m_area_highlight_color.a;

		root[ "area_highlight_thickness" ] = m_options_datas.m_area_highlight_thickness;

		file << root;
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
