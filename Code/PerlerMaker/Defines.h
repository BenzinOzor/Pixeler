#pragma once
#include <unordered_map>
#include <vector>

#include <FZN/Tools/Tools.h>


namespace PerlerMaker
{
	enum ColorChannel
	{
		red,
		green,
		blue,
		alpha,
		COUNT
	};

	enum class Direction
	{
		up,
		down,
		left,
		right,
		COUNT
	};

	static constexpr ImVec2 DefaultWidgetSize{ 150.f, 0.f };

	inline constexpr float		Flt_Max{ std::numeric_limits<float>::max() };
	inline constexpr uint32_t	Uint32_Max{ std::numeric_limits<uint32_t>::max() };

	struct PixelPosition
	{
		uint32_t x{ 0 };
		uint32_t y{ 0 };
	};

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
				color_name = fzn::Tools::Sprintf( "%03d", m_id );

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

	struct ColorPalette
	{
		std::string m_name{};
		std::string m_file_path{};
		ColorInfosVector m_colors;
		std::unordered_map< std::string, ColorPreset > m_presets;	// for each preset name, a list of selected colors (by index in the vector).
	};
	using ColorPalettes = std::unordered_map< std::string, ColorPalette >;	
} // namsepace PerlerMaker
