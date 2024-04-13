#include <FZN/UI/ImGui.h>
#include <FZN/Tools/Tools.h>

#include "Utils.h"


namespace PerlerMaker::Utils
{
	void selectable_bead_info( BeadInfos& _bead )
	{
		auto* bead_name{ _bead.m_name.data() };
		auto cursor_pos{ ImGui::GetCursorPos() };
		
		ImGui::SetCursorPosY( cursor_pos.y + 1 );
		if( ImGui::Selectable( fzn::Tools::Sprintf( "##selectable_%s", bead_name ).c_str(), false, ImGuiSelectableFlags_SpanAvailWidth, { 0, ImGui::GetTextLineHeightWithSpacing() } ) )
			_bead.m_selected = !_bead.m_selected;
		ImGui::SameLine();

		auto hovered{ ImGui::IsItemHovered() };

		ImGui::SetCursorPos( cursor_pos );
		ImGui::Checkbox( fzn::Tools::Sprintf( "##checkbox_%s", bead_name ).c_str(), &_bead.m_selected );
		ImGui::SameLine();
		
		ImGui::PushStyleColor( ImGuiCol_Border, ImGui_fzn::color::white );
		ImGui::ColorButton( fzn::Tools::Sprintf( "##color_button_%s", bead_name ).c_str(), _bead.m_color, ImGuiColorEditFlags_NoTooltip );
		ImGui::PopStyleColor();
		ImGui::SameLine();

		ImGui::AlignTextToFramePadding();
		if( hovered )
			ImGui_fzn::bold_text( "%02d - %s", _bead.m_id, bead_name );
		else
			ImGui::Text( "%02d - %s", _bead.m_id, bead_name );
	}
} // namespace PerlerMaker::Utils
