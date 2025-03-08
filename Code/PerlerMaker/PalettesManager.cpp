#include <filesystem>
#include <algorithm>
#include <limits>

#include <FZN/Managers/FazonCore.h>
#include <FZN/Tools/Logging.h>
#include <FZN/Tools/Math.h>
#include <FZN/Tools/Tools.h>

#include "PalettesManager.h"
#include "Utils.h"


namespace PerlerMaker
{
	PalettesManager::PalettesManager():
		m_fzn_palettes_path( g_pFZN_Core->GetDataPath( "XMLFiles/Palettes" ) ),
		m_app_palettes_path( g_pFZN_Core->GetSaveFolderPath() + "/Palettes" )
	{
		_load_palettes();
	}

	void PalettesManager::update()
	{
		if( ImGui::Begin( "Palettes" ) )
		{
			_header();
			_colors_list();

			if( m_edited_color.is_valid( true ) )
				_edit_color();
		}

		ImGui::End();
	}

	float color_norm( const sf::Color& _color )
	{
		return _color.r * _color.r + _color.g * _color.g + _color.b * _color.b;
	}

	float color_norm( const ImColor& _color )
	{
		return color_norm( Utils::to_sf_color( _color ) );
	}

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Convert the given color to a new one according to the selected palette
	// If no palette is selected, the given color will be returned
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------
	std::pair< sf::Color, const ColorInfos* > PalettesManager::convert_color( const sf::Color& _color ) const
	{
		auto get_distance = []( const ImColor& _color_a, const ImColor& _color_b )
		{
			return fzn::Math::Square( _color_b.Value.x - _color_a.Value.x ) + fzn::Math::Square( _color_b.Value.y - _color_a.Value.y ) + fzn::Math::Square( _color_b.Value.z - _color_a.Value.z );
		};
		
		if( m_selected_palette == nullptr )
			return { _color, nullptr };
		
		ImColor converted_color{ _color };
		ColorInfos* smallest_distance_color{ nullptr };
		float smallest_distance{ Flt_Max };
		float current_distance{ Flt_Max };

		for( auto& color : m_selected_palette->m_colors )
		{
			if( color.m_selected == false )
				continue;

			current_distance = get_distance( converted_color, color.m_color );
			if( current_distance < smallest_distance )
			{
				smallest_distance = current_distance;
				smallest_distance_color = &color;
			}
		}

		if( smallest_distance_color != nullptr )
		{
			++smallest_distance_color->m_count;
			return { Utils::to_sf_color( smallest_distance_color->m_color ), smallest_distance_color };
		}

		return { _color, nullptr };
	}

	void PalettesManager::reset_base_palettes()
	{
		const std::filesystem::path fzn_base_palettes_directory{ m_fzn_palettes_path + "/Base" };
		const std::filesystem::path app_base_palettes_directory{ m_app_palettes_path + "/Base" };

		std::filesystem::copy( fzn_base_palettes_directory, app_base_palettes_directory, std::filesystem::copy_options::overwrite_existing );

		auto xml_file = tinyxml2::XMLDocument{};
		auto file_name = std::string{};
		for( const auto& dir_entry : std::filesystem::recursive_directory_iterator{ app_base_palettes_directory } )
		{
			if( dir_entry.is_directory() )
				continue;

			if( xml_file.LoadFile( dir_entry.path().string().c_str() ) )
			{
				FZN_COLOR_LOG( fzn::DBG_MSG_COL_RED, "Failure : %s (%s)", xml_file.ErrorName(), xml_file.ErrorStr() );
				continue;
			}

			file_name = fzn::Tools::GetFileNameFromPath( dir_entry.path().string().c_str(), true );

			FZN_DBLOG( "Parsing %s...", file_name.c_str() );

			_load_palette( xml_file.FirstChildElement( "color_palette" ), file_name, true );
		}
	}

	void PalettesManager::reset_color_counts()
	{
		if( m_selected_palette == nullptr )
			return;

		for( auto& color : m_selected_palette->m_colors )
			color.m_count = 0;
	}

	void PalettesManager::_load_palettes()
	{
		if( std::filesystem::exists( m_app_palettes_path ) == false || std::filesystem::is_empty( m_app_palettes_path ) )
		{
			std::filesystem::copy( m_fzn_palettes_path, m_app_palettes_path, std::filesystem::copy_options::overwrite_existing );
			FZN_DBLOG( "Palettes folder created, base palettes copied from data to folder." );
		}

		auto xml_file = tinyxml2::XMLDocument{};
		auto file_root = std::string{};
		for( const auto& dir_entry : std::filesystem::recursive_directory_iterator{ m_app_palettes_path } )
		{
			if( dir_entry.is_directory() )
				continue;

			if( xml_file.LoadFile( dir_entry.path().string().c_str() ) )
			{
				FZN_COLOR_LOG( fzn::DBG_MSG_COL_RED, "Failure : %s (%s)", xml_file.ErrorName(), xml_file.ErrorStr() );
				continue;
			}
			
			file_root = _get_palette_root_path( dir_entry.path().string() );

			FZN_DBLOG( "Parsing %s...", file_root.data() );

			_load_palette( xml_file.FirstChildElement( "color_palette" ), file_root );
		}

		m_selected_palette = &m_palettes.begin()->second;
		m_selected_preset = preset_all;
	}

	void PalettesManager::_load_palette( tinyxml2::XMLElement* _palette, std::string_view _file_name, bool _bOverride /*= false*/ )
	{
		if( _palette == nullptr )
			return;

		auto color_palette = ColorPalette{};

		color_palette.m_name = fzn::Tools::XMLStringAttribute( _palette, "name" );
		color_palette.m_file_path = _file_name;

		if( color_palette.m_name.empty() )
		{
			FZN_COLOR_LOG( fzn::DBG_MSG_COL_RED, "Couldn't find color palette name. Using file name instead." );
			color_palette.m_name = fzn::Tools::GetFileNameFromPath( color_palette.m_file_path );
		}
		
		const bool already_in_map = m_palettes.find( color_palette.m_name ) != m_palettes.end();

		if( _bOverride == false && already_in_map )
		{
			FZN_COLOR_LOG( fzn::DBG_MSG_COL_RED, "A palette named '%s' already exists. Ignoring the new one.", color_palette.m_name.c_str() );
			return;
		}

		FZN_DBLOG( "\tCreating palette named '%s'...", color_palette.m_name.c_str() );

		auto* color_settings = _palette->FirstChildElement( "color" );
		auto color_infos = ColorInfos{};

		while( color_settings != nullptr )
		{
			color_infos.m_id	= color_settings->IntAttribute( "id", -1 );
			color_infos.m_name	= fzn::Tools::XMLStringAttribute( color_settings, "name" );
			color_infos.m_color = fzn::Tools::GetImColorFromString( fzn::Tools::XMLStringAttribute( color_settings, "rgb" ) );

			if( color_infos.m_id < 0 && color_infos.m_name.empty() )
			{
				FZN_COLOR_LOG( fzn::DBG_MSG_COL_RED, "Parsing color with no name and no id, ignoring." );
				continue;
			}

			FZN_DBLOG( "\t\tAdding color '%s' to palette.", color_infos.get_full_name().c_str() );

			color_palette.m_colors.push_back( color_infos );
			const auto color_id{ color_palette.m_colors.size() - 1 };

			color_palette.m_presets[ preset_all ].push_back( color_id );

			auto presets = fzn::Tools::split( fzn::Tools::XMLStringAttribute( color_settings, "presets" ), ',' );

			if( presets.empty() == false )
			{
				for( auto& preset : presets )
					color_palette.m_presets[ preset ].push_back( color_id );
			}

			color_settings = color_settings->NextSiblingElement( "color" );
		}

		FZN_DBLOG( "\tAdded palette '%s' to catalog", color_palette.m_name.c_str() );

		if( already_in_map )
		{
			if( m_selected_palette->m_name == color_palette.m_name )
			{
				m_selected_palette = &( m_palettes[ color_palette.m_name ] = std::move( color_palette ) );
				m_selected_preset = preset_all;
			}
		}
		else
			m_palettes[ color_palette.m_name ] = std::move( color_palette );
	}

	void PalettesManager::_save_palette()
	{
		auto to_string = []( const ImColor& _color )
		{
			const auto sf_color{ Utils::to_sf_color( _color ) };
			return fzn::Tools::Sprintf( "%d,%d,%d", sf_color.r, sf_color.g, sf_color.b );
		};

		if( m_selected_palette == nullptr )
			return;

		tinyxml2::XMLDocument dest_file{};
		auto* color_palette{ dest_file.NewElement( "color_palette" ) };
		dest_file.InsertEndChild( color_palette );

		std::string presets{};

		color_palette->SetAttribute( "name", m_selected_palette->m_name.c_str() );

		for( auto color_id{ 0 }; auto& color : m_selected_palette->m_colors )
		{
			if( color.is_valid() == false )
			{
				++color_id;
				continue;
			}

			auto* xml_color{ dest_file.NewElement( "color" ) };

			if( color.m_id >= 0 )
				xml_color->SetAttribute( "id", color.m_id );

			if( color.m_name.empty() == false )
				xml_color->SetAttribute( "name", color.m_name.c_str() );

			xml_color->SetAttribute( "rgb", to_string( color.m_color ).c_str() );

			presets = std::move( _get_presets_from_color_index( m_selected_palette, color_id++ ) );

			if( presets.empty() == false )
				xml_color->SetAttribute( "presets", presets.c_str() );

			color_palette->InsertEndChild( xml_color );
		}

		std::string palette_path{ m_app_palettes_path + "\\" + m_selected_palette->m_file_path };

		if( dest_file.SaveFile( palette_path.c_str() ) )
		{
			FZN_COLOR_LOG( fzn::DBG_MSG_COL_RED, "Failure : %s (%s)", dest_file.ErrorName(), dest_file.ErrorStr() );
			return;
		}

		FZN_DBLOG( "Saved palette '%s' at '%s'", m_selected_palette->m_name.c_str(), palette_path.c_str() );
	}

	void PalettesManager::_set_all_colors_selection( bool _selected )
	{
		if( m_selected_palette == nullptr )
			return;

		for( auto& color : m_selected_palette->m_colors )
			color.m_selected = _selected;
	}

	void PalettesManager::_select_colors_from_preset( std::string_view _preset )
	{
		if( _preset.empty() || m_selected_palette == nullptr )
			return;

		if( auto preset = m_selected_palette->m_presets.find( _preset.data() ); preset != m_selected_palette->m_presets.end() )
		{
			_set_all_colors_selection( false );

			for( auto& color_id : preset->second )
			{
				if( color_id >= m_selected_palette->m_colors.size() )
					continue;

				m_selected_palette->m_colors[ color_id ].m_selected = true;
			}
		}
	}

	bool PalettesManager::_update_preset()
	{
		if( m_selected_palette == nullptr || m_selected_preset.empty() )
			return false;

		auto it_preset{ m_selected_palette->m_presets.find( std::string{ m_selected_preset } ) };

		if( it_preset == m_selected_palette->m_presets.end() )
			return false;

		ColorPreset new_preset;
		new_preset.reserve( m_selected_palette->m_colors.size() );

		for( auto color_id{ 0 }; auto & color : m_selected_palette->m_colors )
		{
			if( color.m_selected )
				new_preset.push_back( color_id );

			++color_id;
		}

		if( new_preset.empty() )
		{
			FZN_COLOR_LOG( fzn::DBG_MSG_COL_RED, "Couldn't save an empty preset. Ignoring." );
			return false;
		}

		it_preset->second = std::move( new_preset );
		return true;
	}

	void PalettesManager::_reset_color_to_edit()
	{
		m_edited_color = ColorInfos{};
		m_color_to_edit = nullptr;
	}

	std::string PalettesManager::_get_presets_from_color_index( ColorPalette* _palette, uint32_t _color_index )
	{
		if( _palette == nullptr || _color_index >= _palette->m_colors.size() )
			return {};

		std::string result;

		for( auto& preset : _palette->m_presets )
		{
			if( preset.first == "All" )
				continue;

			if( std::ranges::find( preset.second, _color_index ) != preset.second.end() )
				result += preset.first + ",";
		}

		if( result.empty() == false )
			result.erase( result.size() - 1 );

		return result;
	}

	std::string PalettesManager::_get_palette_root_path( const std::string& _path )
	{
		auto diff_pos{ _path.find_first_not_of( m_app_palettes_path ) };

		if( diff_pos == std::string::npos )
			return {};

		return _path.substr( diff_pos + 1 );
	}

	bool PalettesManager::match_filter( const ColorInfos& _color )
	{
		if( _color.m_id > ColorInfos::Invalid_ID )
		{
			std::vector< int > IDs{ fzn::Tools::extract_numbers( m_color_filter ) };

			for( int ID : IDs )
			{
				if( _color.m_id == ID )
					return true;
			}
		}

		return fzn::Tools::match_filter( m_color_filter, _color.m_name );
	}
} // namespace PerlerMaker