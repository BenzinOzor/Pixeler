#include <SFML/Graphics/Color.hpp>

#include <FZN/UI/ImGui.h>
#include <FZN/Tools/Math.h>

#include "Utils.h"
#include "Defines.h"
#include "PerlerMaker.h"


namespace PerlerMaker::Utils
{
	sf::Color to_sf_color( const ImColor& _color )
	{
		return { sf::Uint8(_color.Value.x * 255.f), sf::Uint8( _color.Value.y * 255.f), sf::Uint8( _color.Value.z * 255.f), sf::Uint8( _color.Value.w * 255.f) };
	}

	ImColor to_imcolor( const sf::Color& _color )
	{
		return { _color.r / 255.f, _color.g / 255.f, _color.b / 255.f, _color.a / 255.f };
	}

	void compute_IDs_and_names_usage_infos( ColorPalette& _palette )
	{
		int highest_id{ -1 };
		bool at_least_one_valid_name{ false };

		for( const ColorInfos& color : _palette.m_colors )
		{
			if( color.m_id > highest_id )
				highest_id = color.m_id;

			at_least_one_valid_name |= color.m_name.empty() == false;
		}

		_palette.m_nb_digits_in_IDs = highest_id < 0 ? 0 : fzn::Math::get_number_of_digits( highest_id );
		_palette.m_using_names = at_least_one_valid_name;
	}

	std::string get_zero_lead_id( int _id )
	{
		if( _id < 0 )
			return "";

		std::string result{ fzn::Tools::Sprintf( "%d", _id ) };

		const ColorPalette* current_palette{ nullptr };

		if( g_perler_maker != nullptr )
			current_palette = g_perler_maker->get_palettes_manager().get_selected_palette();

		if( current_palette == nullptr )
			return result;

		const int zeros_to_add{ current_palette->m_nb_digits_in_IDs - fzn::Math::get_number_of_digits( _id ) };

		if( zeros_to_add <= 0 )
			return result;

		result.insert( 0, zeros_to_add, '0' );
		return result;
	}


	void text_with_leading_zeros( std::string_view _text, bool _bold, bool _used, bool _shadow )
	{
		auto first_number_pos{ _text.find_first_not_of( '0' ) };

		const ImGuiStyle style{ ImGui::GetStyle() };

		const bool grayed_out{ _used == false && _bold == false };

		const ImColor leading_zeros_color{ grayed_out ? ImGui_fzn::color::dark_gray : ImGui_fzn::color::gray };
		const ImColor id_name_color{ grayed_out ? ImGui_fzn::color::gray : style.Colors[ ImGuiCol_Text ] };

		if( first_number_pos == std::string::npos || first_number_pos == 0 )
		{
			boldable_text( _text, _bold, _used, _shadow );
			return;
		}

		auto leading_zeros = std::string{ _text.substr( 0, first_number_pos ) };
		auto number = std::string{ _text.substr( first_number_pos ) };

		if( _shadow )
		{
			const ImVec2 shadow_offset{ 2.f, 2.f };
			const ImVec2 backup_cursor_pos{ ImGui::GetCursorPos() };

			ImGui::SetCursorPos( backup_cursor_pos + shadow_offset );
			ImGui_fzn::bold_text_colored( ImGui_fzn::color::black, _text );
			ImGui::SameLine();
			ImGui::SetNextItemAllowOverlap();
			ImGui::SetCursorPos( backup_cursor_pos );
		}

		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, { 0.f, style.ItemSpacing.y } );

		if( _bold )
		{
			const ImVec2 backup_cursor_pos{ ImGui::GetCursorPos() };
			ImGui_fzn::bold_text_colored( leading_zeros_color, leading_zeros );
			ImGui::SameLine();
			ImGui::SetCursorPosY( backup_cursor_pos.y );	// I don't understand why I have to do this now.
			ImGui_fzn::bold_text_colored( id_name_color, number );
		}
		else
		{
			ImGui::TextColored( leading_zeros_color, leading_zeros.c_str() );
			ImGui::SameLine();
			ImGui::TextColored( id_name_color, number.c_str() );
		}

		ImGui::PopStyleVar();
	}

	void boldable_text( std::string_view _text, bool _bold, bool _used, bool _shadow )
	{
		if( _shadow )
		{
			const ImVec2 shadow_offset{ 2.f, 2.f };
			const ImVec2 backup_cursor_pos{ ImGui::GetCursorPos() };

			ImGui::SetCursorPos( backup_cursor_pos + shadow_offset );
			ImGui_fzn::bold_text_colored( ImGui_fzn::color::black, _text );
			ImGui::SameLine();
			ImGui::SetNextItemAllowOverlap();
			ImGui::SetCursorPos( backup_cursor_pos );
		}

		const bool		grayed_out{ _used == false && _bold == false };
		const ImColor	text_color{ grayed_out ? ImGui_fzn::color::gray : ImGui::GetStyle().Colors[ ImGuiCol_Text ] };

		if( _bold )
			ImGui_fzn::bold_text_colored( text_color, _text );
		else
			ImGui::TextColored( text_color, _text.data() );
	}

	void color_infos_tooltip_common( const ColorInfos& _color )
	{
		text_with_leading_zeros( _color.get_full_name(), true, true, false );
		ImGui::Separator();

		color_details( _color.m_color );
	}

	void color_details( const ImColor& _color )
	{
		ImGuiContext& g = *GImGui;

		ImVec2 sz( ImGui_fzn::s_ImGuiFormatOptions.m_pFontRegular->FontSize * 2 + g.Style.FramePadding.y * 2, ImGui_fzn::s_ImGuiFormatOptions.m_pFontRegular->FontSize * 2 + g.Style.FramePadding.y * 2 );
		sf::Color sf_color{ Utils::to_sf_color( _color ) };
		ImGui::ColorButton( "tesst##preview", _color, ImGuiColorEditFlags_NoTooltip, sz );
		ImGui::SameLine();
		ImVec2 cursor_pos{ ImGui::GetCursorPos() };
		cursor_pos.y -= 4.f;
		ImGui::SetCursorPos( cursor_pos );
		ImGui::Text( "#%02X%02X%02X\nR:%d, G:%d, B:%d", sf_color.r, sf_color.g, sf_color.b, sf_color.r, sf_color.g, sf_color.b );
	}

	void color_details( const sf::Color& _color )
	{
		color_details( to_imcolor( _color ) );
	}

	void window_bottom_table( uint8_t _nb_items, std::function<void( void )> _table_content_fct )
	{
		ImGui::NewLine();
		ImGui::NewLine();

		if( ImGui::BeginTable( "BottomTable", _nb_items + 1 ) )
		{
			ImGui::TableSetupColumn( "Empty", ImGuiTableColumnFlags_WidthStretch );

			for( uint8_t column{ 1 }; column < _nb_items + 1; ++column )
				ImGui::TableSetupColumn( fzn::Tools::Sprintf( "Button %u", column ).c_str(), ImGuiTableColumnFlags_WidthFixed );

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex( 1 );

			_table_content_fct();

			ImGui::EndTable();
		}
	}
} // namespace PerlerMaker

