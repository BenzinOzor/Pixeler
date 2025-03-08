#include <SFML/Graphics/Color.hpp>

#include <FZN/UI/ImGui.h>

#include "Utils.h"
#include "Defines.h"


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

	void bicolor_color_name( std::string_view _color, bool _bold, bool _used )
	{
		auto first_number_pos{ _color.find_first_not_of( '0' ) };

		const ImGuiStyle style{ ImGui::GetStyle() };

		const bool grayed_out{ _used == false && _bold == false };

		const ImColor leading_zeros_color{ grayed_out ? ImGui_fzn::color::dark_gray : ImGui_fzn::color::gray };
		const ImColor id_name_color{ grayed_out ? ImGui_fzn::color::gray : style.Colors[ ImGuiCol_Text ] };

		if( first_number_pos == std::string::npos )
		{
			if( _bold )
				ImGui_fzn::bold_text_colored( id_name_color, _color );
			else
				ImGui::TextColored( id_name_color, _color.data() );

			return;
		}

		auto leading_zeros = std::string{ _color.substr( 0, first_number_pos ) };
		auto number = std::string{ _color.substr( first_number_pos ) };

		auto spacing_backup{ ImGui::GetStyle().ItemSpacing.x };
		ImGui::GetStyle().ItemSpacing.x = 0.f;

		if( _bold )
		{
			ImGui_fzn::bold_text_colored( leading_zeros_color, leading_zeros );
			ImGui::SameLine();
			ImGui_fzn::bold_text_colored( id_name_color, number );
		}
		else
		{
			ImGui::TextColored( leading_zeros_color, leading_zeros.c_str() );
			ImGui::SameLine();
			ImGui::TextColored( id_name_color, number.c_str() );
		}

		ImGui::GetStyle().ItemSpacing.x = spacing_backup;
	}

	void color_infos_tooltip_common( const ColorInfos& _color )
	{
		bicolor_color_name( _color.get_full_name(), true, true );
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
} // namespace PerlerMaker

