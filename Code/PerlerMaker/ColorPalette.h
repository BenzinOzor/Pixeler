#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <FZN/Tools/Tools.h>

#include "Defines.h"
#include "Utils.h"


namespace PerlerMaker
{
	struct ColorID
	{
		static constexpr int Invalid_ID{ -1 };

		inline bool is_valid() const { return m_id > Invalid_ID || m_name.empty() == false; }

		inline bool operator==( const ColorID& _rhs ) const
		{
			if( m_name != _rhs.m_name )
				return false;

			if( m_id != _rhs.m_id )
				return false;

			return true;
		}

		std::string			m_name;						// can be empty
		int					m_id{ Invalid_ID };			// can be invalid
	};
	using ColorIDs = std::vector< ColorID >;

	struct ColorInfos
	{
		inline bool operator==( const ColorInfos& _rhs ) const
		{
			if( m_color.Value != _rhs.m_color.Value )
				return false;

			return m_color_id == _rhs.m_color_id;
		}

		bool is_valid( bool _test_color_only = false ) const
		{
			if( m_color.Value.w < 0.f || m_color.Value.x < 0.f || m_color.Value.y < 0.f || m_color.Value.z < 0.f )
				return false;

			if( _test_color_only )
				return true;

			return m_color_id.is_valid();
		}

		std::string get_full_name() const		// "id - name"
		{
			auto color_name = std::string{};

			if( m_color_id.m_id >= 0 )
			{
				color_name = Utils::get_zero_lead_id( m_color_id.m_id );
			}

			if( m_color_id.m_name.empty() == false )
				color_name.append( fzn::Tools::Sprintf( "%s%s", color_name.size() > 0 ? " - " : "", m_color_id.m_name.c_str() ) );

			return color_name;
		}

		ColorID				m_color_id;
		ImColor				m_color{ -1, -1, -1, -1 };
		bool				m_selected{ true };
		int					m_count{ -1 };				// if -1, convertion hasn't been done yet
	};
	using ColorInfosVector = std::vector< ColorInfos >;
	
	struct ColorPreset
	{
		inline bool operator==( const ColorPreset& _rhs ) const
		{
			if( m_name != _rhs.m_name )
				return false;

			return m_colors == _rhs.m_colors;
		}

		std::string m_name;
		ColorIDs	m_colors;
	};
	using ColorPresets = std::vector< ColorPreset >;

	struct ColorPalette
	{
		bool is_using_IDs() const { return m_nb_digits_in_IDs > 0; }
		bool is_using_names() const { return m_using_names; }

		std::string			m_name;
		std::string			m_file_path;
		ColorInfosVector	m_colors;
		ColorPresets		m_presets;

		uint8_t				m_nb_digits_in_IDs{ 0 };
		bool				m_using_names{ true };
	};
	using ColorPalettes = std::vector< ColorPalette >;


	/**
	* @brief Sorting function for palettes presets. Preset "All" will always be first.
	* @param [in] _preset_a The first preset to sort.
	* @param [in] _preset_b The second preset to sort.
	* @return True if _preset_a must be placed before _preset_b, false otherwise.
	**/
	static bool presets_sorter( const ColorPreset& _preset_a, const ColorPreset& _preset_b )
	{
		if( _preset_a.m_name == color_preset_all )
			return true;

		if( _preset_b.m_name == color_preset_all )
			return false;

		const std::string name_a = fzn::Tools::get_lower_string( _preset_a.m_name );
		const std::string name_b = fzn::Tools::get_lower_string( _preset_b.m_name );

		if( name_a.compare( name_b ) > 0 )
			return false;

		return true;
	}

	/**
	* @brief Sorting function for palettes.
	* @param [in] _palette_a The first palette to sort.
	* @param [in] _palette_b The second palette to sort.
	* @return True if _palette_a must be placed before _palette_b, false otherwise.
	**/
	static bool palettes_sorter( const ColorPalette& _palette_a, const ColorPalette& _palette_b )
	{
		const std::string name_a = fzn::Tools::get_lower_string( _palette_a.m_name );
		const std::string name_b = fzn::Tools::get_lower_string( _palette_b.m_name );

		if( name_a.compare( name_b ) > 0 )
			return false;

		return true;
	}
}
