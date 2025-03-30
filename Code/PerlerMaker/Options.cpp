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
