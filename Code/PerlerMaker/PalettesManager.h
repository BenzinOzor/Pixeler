#pragma once
#include <string>
#include <unordered_map>

#include <FZN/UI/ImGui.h>

#include "Defines.h"
#include "ColorPalette.h"


namespace tinyxml2
{
	class XMLElement;
}


namespace PerlerMaker
{
	static const std::string color_preset_all{ "All" };		// The default preset containing all the colors of a palette. It is automatically added to a palette and is not in its xml file.

	/************************************************************************
	* @brief Class managing all the color palettes and their presets.
	* It handles the color list ImGui window.
	************************************************************************/
	class PalettesManager
	{
	public:
		PalettesManager();

		/**
		* @brief Open the ImGui window and call its functions.
		**/
		void update();

		/**
		* @brief Convert the given color to a new one according to the selected palette.
		* @param [in] _color The color to convert.
		* @return A pair containing the converted color and a pointer to the converted color infos in the palette.
		* If no palette is selected, the given color will be returned
		**/
		std::pair< sf::Color, const ColorInfos* > convert_color( const sf::Color& _color ) const;

		/**
		* @brief Copy the base palettes from the application datas to the My Documents directory, overriding them in the process.
		* As it is possible to modify the base palettes in the application, this can be useful in case the user wants to get back to a clean slate on them.
		**/
		void reset_base_palettes();
		
		/**
		* @brief Set all colors counts of the current palette to 0. Called when beginning a new image convertion.
		**/
		void reset_color_counts();

		/**
		* @brief Check if there has been an image convertion.
		* @return True if the image has been converted.
		**/
		bool has_convertion_happened() const;

		/**
		* @brief Acessor on currently selected palette.
		* @return A pointer on the palette.
		**/
		const ColorPalette* get_selected_palette() const;

	private:
		/************************************************************************
		* @brief All the needed informations for palette creation.
		************************************************************************/
		struct NewPaletteInfos
		{
			std::string m_name;									// The name of the new palette.
			std::string m_file_name;							// The file name of the new palette. File name is considered to begin at the palettes folder in My Document, if there are folder in there, their name will be in this variable (i.e "MyPalettes/New Palette")
			bool m_file_name_same_as_palette{ true };			// The file name will be the same as the palette name. This can be set to false to differenciate the names in the palette creation popup.
			bool m_restore_backup_palette{ false };				// The current palette will be restored to a backup because a new one will be created from the modifications done by the user (Save As...).
			bool m_set_new_palette_as_backup{ false };			// The newly created palette is a duplicate of an other and it needs to be backed-up instead of the old one.
			ColorPalette* m_source_palette{ nullptr };			// When creating a palette from an other one, its infos will be needed when confirming the creation (color counts, presets, etc...).
		};
		/************************************************************************
		* @brief All the needed informations for preset creation.
		************************************************************************/
		struct NewPresetInfos
		{
			std::string m_name;									// The name of the new preset.
			ColorPreset* m_source_preset{ nullptr };			// When creating a preset from an other one, its infos will be needed when confirming the creation.
			bool m_create_from_current_selection{ false };		// When saving as, the new preset will be created from the current selection of colors.
		};

		/************************************************************************
		* PALETTE FUNCTIONS
		************************************************************************/

		/**
		* @brief Select a new palette to use. This selects the default preset and compute the IDs column size.
		* @param [in] _palette The new palette.
		**/
		void _select_palette( ColorPalette& _palette );

		/**
		* @brief Look for the given palette in the palettes map.
		* @param _palette The name of the palette to find.
		* @return A pointer to the right palette if found, nullptr otherwise.
		**/
		ColorPalette* _find_palette( std::string_view _palette );

		/**
		* @brief Load all the palettes from xml files in My Documents folder.
		**/
		void _load_palettes();

		/**
		* @brief Load a palette from its xml infos.
		* @param [in]	_palette	A pointer to the xml element containing palette infos.
		* @param		_file_name	_file_name The file name of the palette. It will be saved in its infos.
		* @param		_bOverride	When reseting base palette, this will be true to replace currently existing palettes.
		**/
		void _load_palette( tinyxml2::XMLElement* _palette, std::string_view _file_name, bool _bOverride = false );

		/**
		* @brief Retrieve informations about the use if IDs and names in the given palette. The function will set variables m_nb_digits_in_IDs and m_using_names.
		* @param [in,out] _palette The palette we want informations from.
		**/
		void _compute_IDs_and_names_usage_infos( ColorPalette& _palette );

		/**
		* @brief Save the current palette to its xml file.
		**/
		void _save_palette();

		/**
		* @brief Delete current palette from manager list and its file in folder.
		**/
		void _delete_palette();
		
		/**
		* @brief Restore the backed-up palette to current one (when cancelling edition or saving as).
		**/
		void _restore_backup_palette();

		/**
		* @brief Create a new palette from scratch or use another one as model.
		* @param _from_current	Use the current palette as model for the new one.
		*						If false or no palette is in use, an entirely new one will be created called "New Palette".
		**/
		void _create_new_palette( bool _from_current = false );

		/**
		* @brief Generate a palette name from the given model. It will look for the given name among the existing palettes, computing the number to add after the name to avoid name conflicts.
		* @param _palette_name The name to generate from. If empty, "New Palette" will be used.
		* @return The name computed by the function with a number after it.
		**/
		std::string _generate_new_palette_name( std::string_view _palette_name );

		/************************************************************************
		* PRESET FUNCTIONS
		************************************************************************/

		/**
		* @brief Check if there is a selected preset for the current palette and if it's not the default one including all the colors.
		* @return True if the preset is editable.
		**/
		bool _is_preset_editable() const;

		/**
		* @brief Force a selection state on all the colors of the current palette.
		* @param _selected Check or uncheck all colors.
		**/
		void _set_all_colors_selection( bool _selected );

		/**
		* @brief Select the first preset available in list of the current palette.
		* As they are custom sorted, it should be the preset including all the colors.
		**/
		void _select_default_preset();

		/**
		* @brief Select the given preset.
		* @param [in] _preset The new preset to use. If nullptr, will unselect all colors.
		**/
		void _select_preset( ColorPreset* _preset = nullptr );

		/**
		* @brief Select a preset from the given name.
		* @param _preset_name The name of the preset to select.
		**/
		void _select_preset( std::string_view _preset_name );

		/**
		* @brief Select the colors of the palette according to the currently selected preset.
		**/
		void _select_colors_from_selected_preset();

		/**
		* @brief Update the color selection of the current preset to match the current one.
		**/
		void _update_preset_colors_from_selection();

		/**
		* @brief Delete the current preset from the palette.
		**/
		void _delete_preset();

		/**
		* @brief Reset the infos on the color to edit and the modified color in itself.
		**/
		void _reset_color_to_edit();

		/**
		* @brief Generate a string listing all the presets using a given color.
		* @param [in] _color_id The ID that will be looked up for presets in the current palette.
		* @return Comma separated list of all the retrieved presets.
		**/
		std::string _get_presets_from_color_id( const ColorID& _color_id );

		/**
		* @brief Extract the root folder from a full palette path to be used as file path in the palette infos.
		* @param [in] _path The full path to the palette.
		* @return A string containing the path untile palettes folder.
		**/
		std::string _get_palette_root_path( const std::string& _path );

		/**
		* @brief Create a new preset from scratch or use the current color selection.
		* @param _from_current_selection Use the current selection to fill the newly created preset. If false, the new preset will be empty.
		**/
		void _create_new_preset( bool _from_current_selection );

		/**
		* @brief Create a new preset from the current one.
		**/
		void _create_new_preset_from_current();
		
		/**
		* @brief Generate a preset name from the given model. It will look for the given name among the existing presets, computing the number to add after the name to avoid name conflicts.
		* @param _preset_name The name to generate from. If empty, "New Preset" will be used.
		* @return The name computed by the function with a number after it.
		**/
		std::string _generate_new_preset_name( std::string_view _preset_name );

		/**
		* @brief Retrieve the color preset containing all the colors.
		* @param [in] _palette The palette to use for the search. If nullptr, the selected palette will be used.
		* @return A pointer to the preset.
		**/
		ColorPreset* _get_preset_all( ColorPalette* _palette = nullptr ) const;

		/**
		* @brief Check if the given color matches the filter inputed by the user. It will compare the color name and ID.
		* @param [in] _color The color to compare with the filter.
		* @return True if the color matches the current filter.
		**/
		bool _match_filter( const ColorInfos& _color );

		/************************************************************************
		* IMGUI
		************************************************************************/

		/**
		* @brief Header of the palettes management window. Handles palettes and presets selections, edition, addition, removal. Aswell as colors filtering and selection options.
		**/
		void _header();

		/**
		* @brief Palette option menu. Allows the user to add and remove palettes or edit the current one.
		**/
		void _palette_hamburger_menu();

		/**
		* @brief Preset option menu. Allows the user to add and remove presets.
		**/
		void _preset_hamburger_menu();

		/**
		* @brief Setup the color list table. Columns are ajusted automatically depending on the palette settings (ID/name use) and user actions:
		*			- Converting an image will add the color count column.
		**/
		bool _color_table_begin();

		/**
		* @brief The body of the color list table, handling the parsing of all the colors in the palette and calling each selectable function.
		**/
		void _colors_list();
		/**
		* @brief Setup the selectable for a given color. Will display various informations depending on edition mode and if a convertion has been done.
		**/
		bool _selectable_color_info( ColorInfos& _color, int _current_row );

		/**
		* @brief Check if there is a valid color to edit and open the corresponding popup. This serves for color edition and addition.
		**/
		void _edit_color();

		/**
		* @brief Display the available buttons when editing a color.
		**/
		void _edit_color_buttons();

		/**
		* @brief Display the available buttons when adding a new color. There is one more button than in editing context, allowing the user to add another color without closing the popup.
		**/
		void _add_color_buttons();

		/**
		* @brief Palette creation popup allowing to set its name and file name. The user can chose to set the same name for the palette and its file or to have different names.
		**/
		void _new_palette_popup();

		/**
		* @brief Preset creation popup allowing to set its name.
		**/
		void _new_preset_popup();

		/**
		* @brief Calculate the ID column size in the color list table by computing the number of digits the palette IDs are gonna need and multiplying that by the size of a '0' character.
		* @param _compute_palette_infos True to compute the number of digit, using the one calulated before otherwise.
		**/
		void _compute_ID_column_size( bool _compute_palette_infos );
		

		/************************************************************************
		* MEMBER VARIABLES
		************************************************************************/
		const std::string	m_fzn_palettes_path{};					// The path to the palette folder in the application data folder (internal, base data)
		const std::string	m_app_palettes_path{};					// The path to the palette folder in the user Documents folder. Base palette will be copied there.

		ColorPalettes		m_palettes;								// A list containing all the palette found in the palette folder.
		ColorPalette*		m_selected_palette{ nullptr };			// The currently selected palette. All palette actions will be perfomred on this.
		ColorPalette		m_backup_palette;						// The state of the current palette when starting its edition. Will be used when the user wants to cancel their editions.
		ColorPreset*		m_selected_preset{ nullptr };			// The currently selected preset of the palette. All preset actions will be performed on this.

		bool				m_palette_edition{ false };				// Edition mode is activated, allowing palette, presets and colors addition/removal/edition.
		bool				m_new_palette{ false };					// A new palette is being created while in edition mode.
		bool				m_new_preset{ false };					// A new preset is being created while in edition mode.
		bool				m_only_used_colors_display{ false };	// Indicate if we are hiding the colors that aren't used in the current image convertion.
		ColorInfos*			m_color_to_edit{ nullptr };				// A pointer to the color we're editing, nullptr when adding a new color
		ColorInfos			m_edited_color;							// An edited version of the color submitted for edition, or the new created color.
		std::string			m_color_filter{};						// The user entered filter on color name or ID.
		NewPaletteInfos		m_new_palette_infos;					// Informations needed for palette creation.
		NewPresetInfos		m_new_preset_infos;						// Informations needed for preset creation.
		float				m_ID_column_width{ 0.f };				// The width in pixels of the color ID (number) in the color list table. Calculated on palette change.
	};
} // namespace PerlerMaker
