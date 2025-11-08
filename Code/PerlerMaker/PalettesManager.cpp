#include <filesystem>
#include <algorithm>
#include <limits>
#include <cctype>

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

	/**
	* @brief Open the ImGui window and call its functions.
	**/
	void PalettesManager::update()
	{
		if( ImGui::Begin( "Palettes" ) )
		{
			_header();
			_colors_list();

			_edit_color();

			_new_palette_popup();
			_new_preset_popup();
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

	/**
	* @brief Convert the given color to a new one according to the selected palette.
	* @param [in] _color The color to convert.
	* @return A pair containing the converted color and a pointer to the converted color infos in the palette.
	* If no palette is selected, the given color will be returned
	**/
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

	/**
	* @brief Copy the base palettes from the application datas to the My Documents directory, overriding them in the process.
	* As it is possible to modify the base palettes in the application, this can be useful in case the user wants to get back to a clean slate on them.
	**/
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

		// @todo better handling of selected palette. Check if a base palette is selected and change the selection, do nothing otherwise.
		if( m_selected_palette == nullptr )
			m_selected_palette = &m_palettes.front();
	}

	/**
	* @brief Set all colors counts of the current palette to 0. Called when beginning a new image convertion.
	**/
	void PalettesManager::reset_color_counts()
	{
		if( m_selected_palette == nullptr )
			return;

		for( auto& color : m_selected_palette->m_colors )
			color.m_count = 0;
	}

	/**
	* @brief Check if there has been an image convertion.
	* @return True if the image has been converted.
	**/
	bool PalettesManager::has_convertion_happened() const
	{
		if( m_selected_palette == nullptr )
			return false;

		// When doing a convertion, we set all color counts to 0 instead of -1, so if the first color available has a count of at least 0, a convertion occured.
		return m_selected_palette->m_colors.empty() == false && m_selected_palette->m_colors.begin()->m_count >= 0;
	}

	/**
	* @brief Acessor on currently selected palette.
	* @return A pointer on the palette.
	**/
	const ColorPalette* PalettesManager::get_selected_palette() const
	{
		return m_selected_palette;
	}

	/**
	* @brief Select a new palette to use. This selects the default preset and compute the IDs column size.
	* @param _palette The new palette.
	**/
	void PalettesManager::_select_palette( ColorPalette& _palette )
	{
		m_selected_palette = &_palette;
		_select_default_preset();
		_compute_ID_column_size( false );
	}

	/**
	* @brief Look for the given palette in the palettes map.
	* @param _palette The name of the palette to find.
	* @return A pointer to the right palette if found, nullptr otherwise.
	**/
	ColorPalette* PalettesManager::_find_palette( std::string_view _palette )
	{
		auto it_palette = std::ranges::find( m_palettes, _palette, &ColorPalette::m_name );

		return it_palette != m_palettes.end() ? &( *it_palette ) : nullptr;
	}

	/**
	* @brief Load all the palettes from xml files in My Documents folder.
	**/
	void PalettesManager::_load_palettes()
	{
		if( std::filesystem::exists( m_app_palettes_path ) == false || std::filesystem::is_empty( m_app_palettes_path ) )
		{
			std::filesystem::copy( m_fzn_palettes_path, m_app_palettes_path, std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive );
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

		if( m_palettes.empty() )
			return;

		std::ranges::sort( m_palettes, palettes_sorter );

		m_selected_palette = &m_palettes.front();
		_select_default_preset();
	}

	static void add_color_to_preset( ColorPalette& _palette, std::string _preset, ColorID _color_id )
	{
		if( _palette.m_presets.empty() )
			return;

		auto color_preset = std::ranges::find( _palette.m_presets, _preset, &ColorPreset::m_name );

		if( color_preset == _palette.m_presets.end() )
			return;

		color_preset->m_colors.push_back( _color_id );
	}

	/**
	* @brief Load a palette from its xml infos.
	* @param [in] _palette A pointer to the xml element containing palette infos.
	* @param _file_name The file name of the palette. It will be saved in its infos.
	* @param _bOverride When reseting base palette, this will be true to replace currently existing palettes.
	**/
	void PalettesManager::_load_palette( tinyxml2::XMLElement* _palette, std::string_view _file_name, bool _bOverride /*= false*/ )
	{
		if( _palette == nullptr )
			return;

		auto color_palette = ColorPalette{};

		const std::string palette_name{ fzn::Tools::XMLStringAttribute( _palette, "name" ) };
		color_palette.m_name = palette_name;
		color_palette.m_file_path = _file_name;

		if( color_palette.m_name.empty() )
		{
			FZN_COLOR_LOG( fzn::DBG_MSG_COL_RED, "Couldn't find color palette name. Using file name instead." );
			color_palette.m_name = fzn::Tools::GetFileNameFromPath( color_palette.m_file_path );
		}
		
		const bool already_in_map = _find_palette( palette_name ) != nullptr;

		if( _bOverride == false && already_in_map )
		{
			FZN_COLOR_LOG( fzn::DBG_MSG_COL_RED, "A palette named '%s' already exists. Ignoring the new one.", palette_name.c_str() );
			return;
		}

		FZN_DBLOG( "\tCreating palette named '%s'...", palette_name.c_str() );

		auto* color_settings = _palette->FirstChildElement( "color" );
		auto color_infos = ColorInfos{};

		color_palette.m_presets.push_back( { color_preset_all } );
		ColorPreset* preset_all{ nullptr };

		while( color_settings != nullptr )
		{
			color_infos.m_color_id = { fzn::Tools::XMLStringAttribute( color_settings, "name" ), color_settings->IntAttribute( "id", -1 ) };
			color_infos.m_color = fzn::Tools::GetImColorFromString( fzn::Tools::XMLStringAttribute( color_settings, "rgb" ) );

			if( color_infos.m_color_id.is_valid() == false )
			{
				FZN_COLOR_LOG( fzn::DBG_MSG_COL_RED, "Parsing color with no name and no id, ignoring." );
				continue;
			}
			else if( std::ranges::find( color_palette.m_colors, color_infos.m_color_id, &ColorInfos::m_color_id ) != color_palette.m_colors.end() )
			{
				FZN_COLOR_LOG( fzn::DBG_MSG_COL_RED, "Parsed color \"%s\" is already in the palette, ignoring.", color_infos.get_full_name().c_str() );
				continue;
			}

			FZN_DBLOG( "\t\tAdding color '%s' to palette.", color_infos.get_full_name().c_str() );

			color_palette.m_colors.push_back( color_infos );

			preset_all = _get_preset_all( &color_palette );

			if( preset_all != nullptr )
				preset_all->m_colors.push_back( color_infos.m_color_id );

			auto presets = fzn::Tools::split( fzn::Tools::XMLStringAttribute( color_settings, "presets" ), ',' );

			if( presets.empty() == false )
			{
				for( auto& preset : presets )
				{
					if( auto it_preset = std::ranges::find( color_palette.m_presets, preset, &ColorPreset::m_name ); it_preset != color_palette.m_presets.end() )
						it_preset->m_colors.push_back( color_infos.m_color_id );
					else
					{
						color_palette.m_presets.push_back( { preset } );
						color_palette.m_presets.back().m_colors.push_back( color_infos.m_color_id );
					}
				}
			}

			color_settings = color_settings->NextSiblingElement( "color" );
		}

		std::ranges::sort( color_palette.m_presets, presets_sorter );

		FZN_DBLOG( "\tAdded palette '%s' to catalog", palette_name.c_str() );

		if( already_in_map )
		{
			if( m_selected_palette->m_name == palette_name )
			{
				ColorPalette* palette = _find_palette( palette_name );

				if( palette != nullptr )
				{
					*palette = std::move( color_palette );
					m_selected_palette = palette;
					m_selected_preset = preset_all;
				}
			}
		}
		else
			m_palettes.push_back( color_palette );

		_compute_IDs_and_names_usage_infos( m_palettes.back() );
	}

	/**
	* @brief Retrieve informations about the use if IDs and names in the given palette. The function will set variables m_nb_digits_in_IDs and m_using_names.
	* @param [in,out] _palette The palette we want informations from.
	**/
	void PalettesManager::_compute_IDs_and_names_usage_infos( ColorPalette& _palette )
	{
		int highest_id{ -1 };
		bool at_least_one_valid_name{ false };

		for( const ColorInfos& color : _palette.m_colors )
		{
			if( color.m_color_id.m_id > highest_id )
				highest_id = color.m_color_id.m_id;

			at_least_one_valid_name |= color.m_color_id.m_name.empty() == false;
		}

		_palette.m_nb_digits_in_IDs = highest_id < 0 ? 0 : fzn::Math::get_number_of_digits( highest_id );
		_palette.m_using_names = at_least_one_valid_name;
	}

	/**
	* @brief Save the currently used palette to its xml file.
	**/
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

		for( auto& color : m_selected_palette->m_colors )
		{
			if( color.is_valid() == false )
				continue;

			auto* xml_color{ dest_file.NewElement( "color" ) };

			if( color.m_color_id.m_id >= 0 )
				xml_color->SetAttribute( "id", color.m_color_id.m_id );

			if( color.m_color_id.m_name.empty() == false )
				xml_color->SetAttribute( "name", color.m_color_id.m_name.c_str() );

			xml_color->SetAttribute( "rgb", to_string( color.m_color ).c_str() );

			presets = std::move( _get_presets_from_color_id( color.m_color_id ) );

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

	/**
	* @brief Delete current palette from manager list and its file in folder.
	**/
	void PalettesManager::_delete_palette()
	{
		if( m_selected_palette == nullptr )
			return;

		std::string palette_name{ m_selected_palette->m_name };
		std::filesystem::path palette_path{ m_app_palettes_path / std::filesystem::path{ m_selected_palette->m_file_path } };

		std::erase_if( m_palettes, [&palette_name]( const auto& _palette ) { return _palette.m_name == palette_name; } );

		if( m_palettes.empty() )
		{
			m_selected_palette = nullptr;
			m_selected_preset = nullptr;
		}
		else
			_select_palette( m_palettes.front() );

		if( std::filesystem::exists( palette_path ) )
			std::filesystem::remove( palette_path );
	}

	/**
	* @brief Restore the backed-up palette to current one (when cancelling edition or saving as).
	**/
	void PalettesManager::_restore_backup_palette()
	{
		ColorPalette* palette = _find_palette( m_backup_palette.m_name );

		if( palette == nullptr )
			return;

		*palette = m_backup_palette;
	}

	/**
	* @brief Create a new palette from scratch or use another one as model.
	* @param _from_current	Use the current palette as model for the new one.
	*						If false or no palette is in use, an entirely new one will be created called "New Palette".
	**/
	void PalettesManager::_create_new_palette( bool _from_current /*= false*/ )
	{
		m_new_palette = true;

		// Creating a palette from scratch if this options is selected or if we have no current palette.
		if( _from_current == false || m_selected_palette == nullptr )
		{
			m_new_palette_infos = NewPaletteInfos{ _generate_new_palette_name( "New Palette" ) };
		}
		else
		{
			m_new_palette_infos = NewPaletteInfos{ _generate_new_palette_name( m_selected_palette->m_name ) };
			m_new_palette_infos.m_source_palette = m_selected_palette;
		}

		m_new_palette_infos.m_file_name = m_new_palette_infos.m_name;
	}

	/**
	* @brief Generate a palette name from the given model. It will look for the given name among the existing palettes, computing the number to add after the name to avoid name conflicts.
	* @param _palette_name The name to generate from. If empty, "New Palette" will be used.
	* @return The name computed by the function with a number after it.
	**/
	std::string PalettesManager::_generate_new_palette_name( std::string_view _palette_name )
	{
		const std::string palette_name{ _palette_name.empty() ? "New Palette" : _palette_name };

		uint32_t nb_new_palettes{ 0 };
		for( const auto& palette : m_palettes )
		{
			if( palette.m_name.contains( palette_name ) )
				++nb_new_palettes;
		}

		return fzn::Tools::Sprintf( "%s %u", palette_name.c_str(), nb_new_palettes + 1 );
	}

	/**
	* @brief Check if there is a selected preset for the current palette and if it's not the default one including all the colors.
	* @return True if the preset is editable.
	**/
	bool PalettesManager::_is_preset_editable() const
	{
		return m_selected_preset != nullptr && m_selected_preset->m_name != color_preset_all;
	}

	/**
	* @brief Force a selection state on all the colors of the current palette.
	* @param _selected Check or uncheck all colors.
	**/
	void PalettesManager::_set_all_colors_selection( bool _selected )
	{
		if( m_selected_palette == nullptr )
			return;

		for( auto& color : m_selected_palette->m_colors )
			color.m_selected = _selected;
	}

	/**
	* @brief Select the first preset available in list of the current palette.
	* As they are custom sorted, it should be the preset including all the colors.
	**/
	void PalettesManager::_select_default_preset()
	{
		if( m_selected_palette == nullptr )
		{
			m_selected_preset = nullptr;
			return;
		}

		_select_preset( m_selected_palette->m_presets.empty() ? nullptr : &m_selected_palette->m_presets.front() );
	}

	/**
	* @brief Select the given preset.
	* @param [in] _preset The new preset to use. If nullptr, will unselect all colors.
	**/
	void PalettesManager::_select_preset( ColorPreset* _preset /*= nullptr*/ )
	{
		if( m_selected_palette == nullptr )
		{
			m_selected_preset = nullptr;
			return;
		}

		m_selected_preset = _preset;

		if( m_selected_palette == nullptr )
		{
			_set_all_colors_selection( false );
			return;
		}

		_select_colors_from_selected_preset();
	}

	/**
	* @brief Select a preset from the given name.
	* @param _preset_name The name of the preset to select.
	**/
	void PalettesManager::_select_preset( std::string_view _preset_name )
	{
		if( m_selected_palette == nullptr )
			return;

		if( auto it_preset = std::ranges::find( m_selected_palette->m_presets, _preset_name, &ColorPreset::m_name ); it_preset != m_selected_palette->m_presets.end() )
			_select_preset( &(*it_preset) );
	}

	/**
	* @brief Look for the given preset in the in the current palette.
	* @param _preset The name of the preset to find.
	* @return A pointer to the right preset if found, nullptr otherwise.
	**/
	ColorPreset* PalettesManager::_find_preset( std::string_view _preset )
	{
		if( m_selected_palette == nullptr )
			return nullptr;

		if( auto it_preset = std::ranges::find( m_selected_palette->m_presets, _preset, &ColorPreset::m_name ); it_preset != m_selected_palette->m_presets.end() )
			return &( *it_preset );

		return nullptr;
	}

	/**
	* @brief Select the colors of the palette according to the currently selected preset.
	**/
	void PalettesManager::_select_colors_from_selected_preset()
	{
		if( m_selected_palette == nullptr || m_selected_preset == nullptr )
		{
			_set_all_colors_selection( false );
			return;
		}

		_set_all_colors_selection( false );

		for( auto& color_id : m_selected_preset->m_colors )
		{
			if( auto it_color = std::ranges::find( m_selected_palette->m_colors, color_id, &ColorInfos::m_color_id ); it_color != m_selected_palette->m_colors.end() )
				it_color->m_selected = true;
		}
	}
	
	/**
	* @brief Update the color selection of the current preset to match the current one.
	**/
	void PalettesManager::_update_preset_colors_from_selection()
	{
		if( m_selected_palette == nullptr || m_selected_preset == nullptr )
			return;

		m_selected_preset->m_colors.clear();

		for( const ColorInfos& color : m_selected_palette->m_colors )
		{
			if( color.m_selected )
				m_selected_preset->m_colors.push_back( color.m_color_id );
		}
	}

	/**
	* @brief Delete the current preset from the palette.
	**/
	void PalettesManager::_delete_preset()
	{
		if( m_selected_palette == nullptr || _is_preset_editable() == false )
			return;

		std::string_view preset_name{ m_selected_preset->m_name };
		std::erase_if( m_selected_palette->m_presets, [&preset_name]( const ColorPreset& _preset ){ return _preset.m_name == preset_name; } );

		_select_default_preset();
		_save_palette();
	}

	/**
	* @brief Reset the infos on the color to edit and the modified color in itself.
	**/
	void PalettesManager::_reset_color_to_edit()
	{
		m_edited_color = ColorInfos{};
		m_color_to_edit = nullptr;
	}

	/**
	* @brief Generate a string listing all the presets using a given color in order to be written in the palette xml file.
	* @param [in] _color_id The ID that will be looked up for presets in the current palette.
	* @return Comma separated list of all the retrieved presets.
	**/
	std::string PalettesManager::_get_presets_from_color_id( const ColorID& _color_id )
	{
		if( m_selected_palette == nullptr || _color_id.is_valid() == false )
			return {};

		std::string result;

		for( auto& preset : m_selected_palette->m_presets )
		{
			// We don't want to include the "All" preset to this list, there is no point in saving it in the palette file.
			if( preset.m_name == color_preset_all )
				continue;

			if( std::ranges::find( preset.m_colors, _color_id ) != preset.m_colors.end() )
				result += preset.m_name + ",";
		}

		// We finished looking for the presets using the given colorID, we can remove the last comma we added.
		if( result.empty() == false )
			result.erase( result.size() - 1 );

		return result;
	}

	/**
	* @brief Extract the root folder from a full palette path to be used as file path in the palette infos.
	* @param [in] _path The full path to the palette.
	* @return A string containing the path untile palettes folder.
	**/
	std::string PalettesManager::_get_palette_root_path( const std::string& _path )
	{
		auto diff_pos{ _path.find_first_not_of( m_app_palettes_path ) };

		if( diff_pos == std::string::npos )
			return {};

		return _path.substr( diff_pos + 1 );
	}

	/**
	* @brief Create a new preset from scratch or use the current color selection.
	* @param _from_current_selection Use the current selection to fill the newly created preset. If false, the new preset will be empty.
	**/
	void PalettesManager::_create_new_preset( bool _from_current_selection )
	{
		if( m_selected_palette == nullptr )
			return;

		m_new_preset = true;

		m_new_preset_infos = NewPresetInfos{ _generate_new_preset_name( "New Preset" ) };
		m_new_preset_infos.m_create_from_current_selection = _from_current_selection;
	}

	/**
	* @brief Create a new preset from the current one.
	**/
	void PalettesManager::_create_new_preset_from_current()
{
		if( m_selected_palette == nullptr || m_selected_preset == nullptr )
			return;

		m_new_preset = true;

		m_new_preset_infos = NewPresetInfos{ _generate_new_preset_name( m_selected_preset->m_name ) };
		m_new_preset_infos.m_source_preset = m_selected_preset;
	}

	/**
	* @brief Generate a preset name from the given model. It will look for the given name among the existing presets, computing the number to add after the name to avoid name conflicts.
	* @param _preset_name The name to generate from. If empty, "New Preset" will be used.
	* @return The name computed by the function with a number after it.
	**/
	std::string PalettesManager::_generate_new_preset_name( std::string_view _preset_name )
	{
		if( m_selected_palette == nullptr )
			return {};

		std::string_view preset_name{ _preset_name.empty() ? "New Preset" : _preset_name };

		uint32_t nb_new_presets{ 0 };
		for( const auto& preset : m_selected_palette->m_presets )
		{
			if( preset.m_name.contains( preset_name ) )
				++nb_new_presets;
		}

		return fzn::Tools::Sprintf( "%s %u", preset_name.data(), nb_new_presets + 1 );
	}

	/**
	* @brief Retrieve the color preset containing all the colors.
	* @param [in] _palette The palette to use for the search. If nullptr, the selected palette will be used.
	* @return A pointer to the preset.
	**/
	ColorPreset* PalettesManager::_get_preset_all( ColorPalette* _palette /*= nullptr*/ ) const
	{
		ColorPalette* palette_to_use = _palette != nullptr ? _palette : m_selected_palette;

		if( palette_to_use == nullptr )
			 return nullptr;

		if( auto it_preset = std::ranges::find( palette_to_use->m_presets, color_preset_all, &ColorPreset::m_name ); it_preset != palette_to_use->m_presets.end() )
			return &(*it_preset);

		return nullptr;
	}

	/**
	* @brief Check if the given color matches the filter inputed by the user. It will compare the color name and ID.
	* @param [in] _color The color to compare with the filter.
	* @return True if the color matches the current filter.
	**/
	bool PalettesManager::_match_filter( const ColorInfos& _color )
	{
		// If the given color uses and ID, we will compare that first.
		if( _color.m_color_id.m_id > ColorID::Invalid_ID )
		{
			// Extracting all numbers in the filter to use as IDs to match with the color.
			std::vector< int > IDs{ fzn::Tools::extract_numbers( m_color_filter ) };

			if( std::ranges::find( IDs, _color.m_color_id.m_id ) != IDs.end() )
				return true;
		}

		// If no ID matched or the given color doesn't use it, we look for its name in the filter.
		return fzn::Tools::match_filter( m_color_filter, _color.m_color_id.m_name );
	}
} // namespace PerlerMaker