#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <FZN/Tools/Tools.h>

#include "Utils.h"


namespace PerlerMaker
{
	/*struct ColorID
	{
		std::string			m_name;						// can be empty
		int					m_id{ Invalid_ID };			// can be invalid
	};*/

	struct ColorInfos
	{
		static constexpr int Invalid_ID{ -1 };

		inline bool operator==( const ColorInfos& _rhs ) const
		{
			if( m_name != _rhs.m_name )
				return false;

			if( m_id != _rhs.m_id )
				return false;

			if( m_color.Value != _rhs.m_color.Value )
				return false;

			return true;
		}

		bool is_valid( bool _test_color_only = false ) const
		{
			if( m_color.Value.w < 0.f || m_color.Value.x < 0.f || m_color.Value.y < 0.f || m_color.Value.z < 0.f )
				return false;

			if( _test_color_only )
				return true;

			if( m_id <= Invalid_ID && m_name.empty() )
				return false;

			return true;
		}

		std::string get_full_name() const		// "id - name"
		{
			auto color_name = std::string{};

			if( m_id >= 0 )
			{
				color_name = Utils::get_zero_lead_id( m_id );
			}

			if( m_name.empty() == false )
				color_name.append( fzn::Tools::Sprintf( "%s%s", color_name.size() > 0 ? " - " : "", m_name.c_str() ) );

			return color_name;
		}

		std::string			m_name{};					// can be empty
		int					m_id{ Invalid_ID };			// can be invalid
		ImColor				m_color{ -1, -1, -1, -1 };
		bool				m_selected{ true };
		int					m_count{ -1 };				// if -1, convertion hasn't been done yet
	};
	using ColorInfosVector = std::vector< ColorInfos >;
	using ColorPreset = std::vector< int >;
	using ColorPresets = std::unordered_map< std::string, ColorPreset >;

	struct ColorPalette
	{
		bool is_using_IDs() const { return m_nb_digits_in_IDs > 0; }
		bool is_using_names() const { return m_using_names; }

		std::string m_name;
		std::string m_file_path;
		ColorInfosVector m_colors;
		ColorPresets m_presets;	// for each preset name, a list of selected colors (by index in the vector).

		uint8_t m_nb_digits_in_IDs{ 0 };
		bool	m_using_names{ true };
	};
	using ColorPalettes = std::unordered_map< std::string, ColorPalette >;
}
