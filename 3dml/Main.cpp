//******************************************************************************
// $Header$
//
// The contents of this file are subject to the Flatland Public License 
// Version 1.1 (the "License"); you may not use this file except in 
// compliance with the License. You may obtain a copy of the License at 
// http://www.3dml.org/FPL/ 
//
// Software distributed under the License is distributed on an "AS IS" basis,
// WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for
// the specific language governing rights and limitations under the License. 
//
// The Original Code is Rover. 
//
// The Initial Developer of the Original Code is Flatland Online, Inc. 
// Portions created by Flatland are Copyright (C) 1998-2000 Flatland
// Online Inc. All Rights Reserved. 
//
// Contributor(s): Philip Stephens.
//******************************************************************************

#define STRICT
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <windows.h>
#include "Classes.h"
#include "Fileio.h"
#include "Image.h"
#include "Light.h"
#include "Memory.h"
#include "Parser.h"
#include "Render.h"
#include "Spans.h"
#include "Platform.h"
#include "Plugin.h"
#include "Utils.h"

// Version string.

#define ROVER_VERSION_NUMBER		0x020000ff

// Update types.

#define AUTO_UPDATE						0
#define USER_REQUESTED_UPDATE			1	
#define SPOT_REQUESTED_UPDATE			2

// Predefined URLs.

#define UPDATE_URL			"http://download.flatland.com/update/update.zip"
#define VERSION_URL			"http://download.flatland.com/update/version.txt"
#define RP_DOWNLOAD_URL \
	"http://www.real.com/products/player/downloadrealplayer.html"
#define WMP_DOWNLOAD_URL \
	"http://www.microsoft.com/windows/mediaplayer/en/download/default.asp"

// Maximum number of entries allowed in spot history list.

#define MAX_HISTORY_ENTRIES	15

//------------------------------------------------------------------------------
// Global variable definitions.
//------------------------------------------------------------------------------

// Flag indicating whether AMD 3DNow! instructions are supported.

int AMD_3DNow_supported;

// Display dimensions and other related information.

float half_window_width;
float half_window_height;
float horz_field_of_view;
float vert_field_of_view;
float half_viewport_width;
float half_viewport_height;
float pixels_per_world_unit;
float pixels_per_texel;
float horz_pixels_per_degree;
float vert_pixels_per_degree;
bool fast_clipping;

// Debug, warnings and low memory flags.

bool debug_on;
bool warnings;
bool low_memory;

// Cached blockset list.

cached_blockset *cached_blockset_list;
cached_blockset *last_cached_blockset_ptr;

// Spot history list.

history *spot_history_list;

// URL directory of currently open spot, and current spot URL without the
// entrance name.

string spot_URL_dir;
string curr_spot_URL;

// Minimum Rover version required for the current spot.

int min_rover_version;

// Flag indicating if we've seen the first spot URL yet or not.

bool seen_first_spot_URL;

// Spot title.

string spot_title;

// Trigonometry tables.

sine_table sine;
cosine_table cosine;

// Visible radius, and frustum vertex list (in view space).

float visible_radius;
vertex frustum_vertex_list[FRUSTUM_VERTICES];

// Frame buffer pointer and width.

byte *frame_buffer_ptr;
int frame_buffer_width;

// Span buffer.

span_buffer *span_buffer_ptr;

// Pointer to old and current blockset list, and custom blockset.

blockset_list *old_blockset_list_ptr;
blockset_list *blockset_list_ptr;
blockset *custom_blockset_ptr;

// Symbol to block definition mapping table.  If single character symbols are
// being used, only the first 127 entries are valid.  If double character
// symbols are being used, all entries are valid.

block_def *symbol_table[16384];

// Pointer to world object and movable block list.

world *world_ptr;
block *movable_block_list;

// Onload exit.

hyperlink *onload_exit_ptr;

// Global list of entrances.

entrance *global_entrance_list;

// List of imagemaps.

imagemap *imagemap_list;

// Global list of lights.

int global_lights;
light *global_light_list;

// Orb light, texture, brightness, size and exit.

light *orb_light_ptr;
texture *orb_texture_ptr;
texture *custom_orb_texture_ptr;
float orb_brightness;
int orb_brightness_index;
float orb_width, orb_height;
float half_orb_width, half_orb_height;
hyperlink *orb_exit_ptr;

// Global list of sounds, ambient sound, audio radius, reflection radius,
// and a flag indicating if the global sound list has been changed.

sound *global_sound_list;
sound *ambient_sound_ptr;
float audio_radius;
float reflection_radius;
bool global_sound_list_changed;

// Streaming sound.

sound *streaming_sound_ptr;

// Original list of popups, and global list of popups (which are copies of
// original popups).

popup *original_popup_list;
popup *global_popup_list;
popup *last_global_popup_ptr;

// Global ist of triggers.

trigger *global_trigger_list;
trigger *last_global_trigger_ptr;

// Placeholder texture.

texture *placeholder_texture_ptr;

// Sky definition.

texture *sky_texture_ptr;
texture *custom_sky_texture_ptr;
RGBcolour sky_colour;
pixel sky_colour_pixel;
float sky_brightness;
int sky_brightness_index;

// Ground block definition.

block_def *ground_block_def_ptr;

// Player block symbol and pointer, camera offset, and viewpoint.

word player_block_symbol;
block *player_block_ptr;
vector player_camera_offset;
viewpoint player_viewpoint;

// Player's size, collision box and step height.

vertex player_size;
COL_AABOX player_collision_box;
float player_step_height;

// Start and current time, and number of frames and total polygons rendered.

int start_time_ms, curr_time_ms;
int frames_rendered, total_polygons_rendered;

// Current mouse position.

int mouse_x;
int mouse_y;

// Number of currently selected polygon, and pointer to the block definition
// it belongs to.

int curr_selected_polygon_no;
block_def *curr_selected_block_def_ptr;

// Previously and currently selected block.

block *prev_selected_block_ptr;
block *curr_selected_block_ptr;

// Previously and currently selected square.

square *prev_selected_square_ptr;
square *curr_selected_square_ptr;

// Previously and currently selected exit.

hyperlink *prev_selected_exit_ptr;
hyperlink *curr_selected_exit_ptr;

// Previously and currently selected area and it's square.

area *prev_selected_area_ptr;
square *prev_area_square_ptr;
area *curr_selected_area_ptr;
square *curr_area_square_ptr;

// Current popup square.

square *curr_popup_square_ptr;

// Current URL and file path.

string curr_URL;
string curr_file_path;

// Flags indicating if the viewpoint has changed, and if so the current
// direction of movement.

bool viewpoint_has_changed;
bool forward_movement;

// Current custom texture and wave.

texture *curr_custom_texture_ptr;
wave *curr_custom_wave_ptr;

// Time that logo was last updated.

int last_logo_time_ms;

// Stream flag, name, and URLs for RealPlayer and Windows Media Player.

bool stream_set;
string name_of_stream;
string rp_stream_URL;
string wmp_stream_URL;

// Pointer to unscaled video texture (for popups) and scaled video texture
// list.

texture *unscaled_video_texture_ptr;
video_texture *scaled_video_texture_list;

//------------------------------------------------------------------------------
// Local variable definitions.
//------------------------------------------------------------------------------

// Flags indicating whether we should check for a plugin update, whether the
// update is in progress, and the update type.

static bool check_for_update, update_in_progress;
static int update_type;

// Full spot URL, and the current entrance name.

static string full_spot_URL;
static string curr_spot_entrance;

// Current link URL.

static const char *curr_link_URL;

// Collision data.

static float player_fall_delta;
static COL_MESH *col_mesh_list[512];
static VEC3 mesh_pos_list[512];
static int col_meshes;

// Flag indicating if the trajectory is tilted.

static bool trajectory_tilted;

// Flag indicating if player viewpoint has been set.

static bool player_viewpoint_set;

// List of active triggers.

static trigger *active_trigger_list;
static trigger *last_active_trigger_ptr;

// Flag indicating if a mouse clicked event has been recieved.

static bool mouse_was_clicked;

// Current location that player is in, and flag indicating if the block the player 
// is standing on was replaced.

static int player_column, player_row, player_level;
static bool player_block_replaced;

// Flag indicating if the player was teleported in last frame.

static bool player_was_teleported;

// A vector pointing up.

static vector world_y_axis(0.0, 1.0, 0.0);

//==============================================================================
// Miscellaneous global functions.
//==============================================================================

//------------------------------------------------------------------------------
// Function to adjust an angle so that it is between -180 and 179, or 0 and 359.
//------------------------------------------------------------------------------

float
neg_adjust_angle(float angle)
{
	float degrees = ((angle / 360.0f) - INT(angle / 360.0f)) * 360.0f;
	if (degrees <= -179.0f)
		degrees += 360.0f;
	else if (degrees > 180.0f)
		degrees -= 360.0f;
	return(degrees);
}

float
pos_adjust_angle(float angle)
{
	float degrees = ((angle / 360.0f) - INT(angle / 360.0f)) * 360.0f;
	if (degrees < 0.0)
		degrees += 360.0f;
	return(degrees);
}

//------------------------------------------------------------------------------
// Function to clamp an angle to the minimum or maximum allowed range.
//------------------------------------------------------------------------------

float
clamp_angle(float angle, float min_angle, float max_angle)
{
	if (FLT(angle, min_angle))
		angle = min_angle;
	else if (FGT(angle, max_angle))
		angle = max_angle;
	return(angle);
}

//==============================================================================
// Main support functions.
//==============================================================================

//------------------------------------------------------------------------------
// Display a dialog box containing a low memory error message.
//------------------------------------------------------------------------------

static void
display_low_memory_error(void)
{
	fatal_error("Insufficient memory", "There is insufficient memory to "
		"run the Flatland Rover.  Try closing some applications first, then "
		"click on the RELOAD or REFRESH button on your browser to restart the "
		"Flatland Rover.");
}

//------------------------------------------------------------------------------
// Write some rendering statistics to the log file.
//------------------------------------------------------------------------------

static void
log_stats(void)
{
	int end_time_ms;
	float elapsed_time;

	// Get the end time in milliseconds and seconds.

	end_time_ms = get_time_ms();

	// Compute the number of seconds that have elapsed, and divide the number
	// of frames and polygons rendered by the elapsed time to obtain the
	// average frame and polygon rate.

	if (frames_rendered > 0) {
		elapsed_time = (float)(end_time_ms - start_time_ms) / 1000.0f;
		diagnose("Average frame rate was %f frames per second",
			(float)frames_rendered / elapsed_time);
		diagnose("Average polygon rate was %f polygons per second",
			(float)total_polygons_rendered / elapsed_time);
	}
}

//------------------------------------------------------------------------------
// Update all light sources.
//------------------------------------------------------------------------------

static void
update_all_lights(float elapsed_time)
{
	light *light_ptr;
	direction new_dir, *min_dir_ptr, *max_dir_ptr;
	float new_intensity;

	START_TIMING;

	// Step through the global list of lights, handling those types that change
	// over time.

	light_ptr = global_light_list;
	while (light_ptr) {
		switch (light_ptr->style) {
		case PULSATING_POINT_LIGHT:

			// Add the delta intensity, scaled by the elapsed time.

			new_intensity = light_ptr->intensity + 
				light_ptr->delta_intensity * elapsed_time;

			// If the intensity has exceeded the minimum or maximum intensity,
			// clamp it, then change the sign of the delta intensity.

			if (FGT(light_ptr->delta_intensity, 0.0)) {
				if (FGT(new_intensity, 
					light_ptr->intensity_range.max_percentage)) {
					new_intensity = light_ptr->intensity_range.max_percentage;
					light_ptr->delta_intensity = -light_ptr->delta_intensity;
				}
			} else {
				if (FLT(new_intensity,
					light_ptr->intensity_range.min_percentage)) {
					new_intensity = light_ptr->intensity_range.min_percentage;
					light_ptr->delta_intensity = -light_ptr->delta_intensity;
				}
			}

			// Set the new intensity.

			light_ptr->set_intensity(new_intensity);
			break;

		case REVOLVING_SPOT_LIGHT:

			// Add the delta direction, scaled by the elapsed time.  Don't allow
			// the delta to exceed 45 degrees around either axis.

			new_dir.angle_x = pos_adjust_angle(light_ptr->curr_dir.angle_x +
				clamp_angle(light_ptr->delta_dir.angle_x * elapsed_time,
				-45.0f, 45.0f));
			new_dir.angle_y = pos_adjust_angle(light_ptr->curr_dir.angle_y +
				clamp_angle(light_ptr->delta_dir.angle_y * elapsed_time,
				-45.0f, 45.0f));
	
			// Set the new direction vector.

			light_ptr->set_direction(new_dir);
			break;

		case SEARCHING_SPOT_LIGHT:

			// Add the delta direction, scaled by the elapsed time.

			new_dir.angle_x = light_ptr->curr_dir.angle_x +
				light_ptr->delta_dir.angle_x * elapsed_time;
			new_dir.angle_y = light_ptr->curr_dir.angle_y +
				light_ptr->delta_dir.angle_y * elapsed_time;

			// Get pointers to the minimum and maximum directions.

			min_dir_ptr = &light_ptr->dir_range.min_direction;
			max_dir_ptr = &light_ptr->dir_range.max_direction;

			// If the new direction has passed beyond the minimum or maximum
			// range around the X or Y axis, clamp it, then change the direction
			// around that axis.

			if (FGT(light_ptr->delta_dir.angle_x, 0.0f)) {
				if (FGT(new_dir.angle_x, max_dir_ptr->angle_x)) {
					new_dir.angle_x = max_dir_ptr->angle_x;
					light_ptr->delta_dir.angle_x = -light_ptr->delta_dir.angle_x;
				}
			} else {
				if (FLT(new_dir.angle_x, min_dir_ptr->angle_x)) {
					new_dir.angle_x = min_dir_ptr->angle_x;
					light_ptr->delta_dir.angle_x = -light_ptr->delta_dir.angle_x;
				}
			}
			if (FGT(light_ptr->delta_dir.angle_y, 0.0f)) {
				if (FGT(new_dir.angle_y, max_dir_ptr->angle_y)) {
					new_dir.angle_y = max_dir_ptr->angle_y;
					light_ptr->delta_dir.angle_y = -light_ptr->delta_dir.angle_y;
				}
			} else {
				if (FLT(new_dir.angle_y, min_dir_ptr->angle_y)) {
					new_dir.angle_y = min_dir_ptr->angle_y;
					light_ptr->delta_dir.angle_y = -light_ptr->delta_dir.angle_y;
				}
			}

			// Set the new direction vector.

			light_ptr->set_direction(new_dir);
		}

		// Move onto the next light...

		light_ptr = light_ptr->next_light_ptr;
	}

	END_TIMING("update_all_lights");
}

//------------------------------------------------------------------------------
// Update all sounds.
//------------------------------------------------------------------------------

static void
update_all_sounds(void)
{
	sound *sound_ptr;

	START_TIMING;

	// Begin the sound update.

	begin_sound_update();

	// Update all non-streaming sounds in global sound list.

	sound_ptr = global_sound_list;
	while (sound_ptr) {
		if (!sound_ptr->streaming)
			update_sound(sound_ptr);
		sound_ptr = sound_ptr->next_sound_ptr;
	}

	// Update ambient sound if it's not a streaming sound.

	if (ambient_sound_ptr && !ambient_sound_ptr->streaming)
		update_sound(ambient_sound_ptr);

	// Update streaming sound.

	if (stream_set && streaming_sound_ptr)
		update_sound(streaming_sound_ptr);

	// End the sound update.

	end_sound_update();

	END_TIMING("update_all_sounds");
}

//------------------------------------------------------------------------------
// Stop all sounds.
//------------------------------------------------------------------------------

static void
stop_all_sounds(void)
{
	sound *sound_ptr;

	// Stop all sounds in global sound list.

	sound_ptr = global_sound_list;
	while (sound_ptr) {
		stop_sound(sound_ptr);
		sound_ptr->in_range = false;
		sound_ptr = sound_ptr->next_sound_ptr;
	}

	// Stop ambient sound.

	if (ambient_sound_ptr) {
		stop_sound(ambient_sound_ptr);
		ambient_sound_ptr->in_range = false;
	}
}

//------------------------------------------------------------------------------
// Free the spot data.
//------------------------------------------------------------------------------

static void
free_spot_data(void)
{
	block_def *block_def_ptr;

	// Log stats.

	log_stats();

	// Delete map.

	if (world_ptr)
		delete world_ptr;

	// Delete the movable block list.

	while (movable_block_list) {
		block_def_ptr = movable_block_list->block_def_ptr;
		movable_block_list = block_def_ptr->del_block(movable_block_list);
	}

	// Delete all entrances in the global entrance list.

	while (global_entrance_list)
		global_entrance_list = del_entrance(global_entrance_list);

	// Delete all imagemaps.

	while (imagemap_list) {
		imagemap *next_imagemap_ptr = imagemap_list->next_imagemap_ptr;
		delete imagemap_list;
		imagemap_list = next_imagemap_ptr;
	}

	// Delete all lights in the global light list.

	while (global_light_list)
		global_light_list = del_light(global_light_list);

	// Delete the orb light.

	if (orb_light_ptr)
		delete orb_light_ptr;

	// Stop all sounds.

	stop_all_sounds();

	// Delete all sounds in global sound list.

	while (global_sound_list)
		global_sound_list = del_sound(global_sound_list);

	// Delete ambient sound.

	if (ambient_sound_ptr)
		delete ambient_sound_ptr;

	// Delete streaming sound.

	if (streaming_sound_ptr)
		delete streaming_sound_ptr;

	// Delete all popups in global popup list.

	while (global_popup_list)
		global_popup_list = del_popup(global_popup_list);

	// Delete all popups in original popup list, as well as their foreground
	// textures.

	while (original_popup_list) {
		popup *next_popup_ptr = original_popup_list->next_popup_ptr;
		if (original_popup_list->fg_texture_ptr)
			DEL(original_popup_list->fg_texture_ptr, texture);
		delete original_popup_list;
		original_popup_list = next_popup_ptr;
	}

	// Delete onload exit.

	if (onload_exit_ptr)
		DEL(onload_exit_ptr, hyperlink);

	// Delete all global triggers.

	while (global_trigger_list)
		global_trigger_list = del_trigger(global_trigger_list);

	// Delete the free lists of triggers, hyperlinks, lights, sounds,
	// locations, entrances and popups.

	delete_free_trigger_list();
	delete_free_hyperlink_list();
	delete_free_light_list();
	delete_free_sound_list();
	delete_free_entrance_list();
	delete_free_location_list();
	delete_free_popup_list();

	// Delete player block.

	if (player_block_ptr) {
		block_def_ptr = player_block_ptr->block_def_ptr;
		block_def_ptr->del_block(player_block_ptr);
	}

	// Delete the custom blockset and the ground block definition.

	if (custom_blockset_ptr)
		delete custom_blockset_ptr;
	if (ground_block_def_ptr)
		delete ground_block_def_ptr;
}

//------------------------------------------------------------------------------
// Initialise the spot data.
//------------------------------------------------------------------------------

static bool
init_spot_data(void)
{
	// Clear selection active flag, mouse clicked event and current link URL.

	selection_active = false;
	mouse_clicked.reset_event();
	curr_link_URL = NULL;

	// Clear currently selected block, square, exit, area and it's square,
	// and popup square.

	curr_selected_block_ptr = NULL;
	curr_selected_square_ptr = NULL;
	curr_selected_exit_ptr = NULL;
	curr_selected_area_ptr = NULL;
	curr_area_square_ptr = NULL;
	curr_popup_square_ptr = NULL;

	// Hide any label that may be visible.

	hide_label();

	// Initialise the list of free triggers, hyperlinks, lights, sounds,
	// locations, entrances and popups.

	init_free_trigger_list();
	init_free_hyperlink_list();
	init_free_light_list();
	init_free_sound_list();
	init_free_location_list();
	init_free_entrance_list();
	init_free_popup_list();

	// Reset the active trigger list and last active trigger pointer.

	active_trigger_list = NULL;
	last_active_trigger_ptr = NULL;

	// The player viewpoint has yet to be set.

	player_viewpoint_set = false;

	// The player was not teleported from somewhere else.

	player_was_teleported = false;

	// Initialise onload exit pointer.

	onload_exit_ptr = NULL;

	// Initialise global entrance and imagemap lists.

	global_entrance_list = NULL;
	imagemap_list = NULL;

	// Initialise placeholder texture.

	placeholder_texture_ptr = NULL;

	// Initialise ground block definition.

	ground_block_def_ptr = NULL;

	// Initialise global light and sound lists, and reset the ambient sound
	// and streaming sound.

	global_lights = 0;
	global_light_list = NULL;
	global_sound_list = NULL;
	ambient_sound_ptr = NULL;
	streaming_sound_ptr = NULL;

	// Initialise orb texture and light.

	orb_texture_ptr = NULL;
	orb_light_ptr = NULL;

	// Initialise original and global popup lists.

	original_popup_list = NULL;
	global_popup_list = NULL;
	last_global_popup_ptr = NULL;

	// Initialise global trigger list.

	global_trigger_list = NULL;
	last_global_trigger_ptr = NULL;

	// Initialise player block and the movable block list.

	player_block_ptr = NULL;
	movable_block_list = NULL;

	// Create custom blockset and the world object.

	if ((custom_blockset_ptr = new blockset) == NULL ||
		(world_ptr = new world) == NULL) {
		display_low_memory_error();
		return(false);
	}

	// Clear the symbol table.

	for (int index = 0; index < 16384; index++)
		symbol_table[index] = NULL;

	// Initialise the last player position.

	player_viewpoint.last_position.set(0.0, 0.0, 0.0);

	// Turn off the debug, warnings and low memory flags.

	debug_on = false;
	warnings = false;
	low_memory = false;
	return(true);
}

//------------------------------------------------------------------------------
// Teleport to the named entrance location.
//------------------------------------------------------------------------------

static void
teleport(const char *entrance_name, bool history_entry_selected)
{
	entrance *entrance_ptr;
	location *location_ptr;
	block *block_ptr;

	// Search for the entrance with the specified name, select one of it's
	// locations at random, and set the player position to it.  If an entrance
	// with the specified name doesn't exist, use the default entrance.

	if ((entrance_ptr = find_entrance(entrance_name)) == NULL)
		entrance_ptr = find_entrance("default");
	location_ptr = entrance_ptr->choose_location();
	player_viewpoint.position.set_scaled_map_position(location_ptr->column,
		location_ptr->row, location_ptr->level);
	player_viewpoint.turn_angle = location_ptr->initial_direction.angle_y;
	player_viewpoint.look_angle = -location_ptr->initial_direction.angle_x;

	// If the player has landed in a square occupied by a block that has a
	// polygonal model, put the player on top of the block's bounding box.
	// Otherwise put them at the bottom of the block.

	if ((block_ptr = world_ptr->get_block_ptr(location_ptr->column,
		location_ptr->row, location_ptr->level)) != NULL && 
		block_ptr->polygons > 0 && block_ptr->solid && 
		block_ptr->col_mesh_ptr != NULL)
		player_viewpoint.position.y = block_ptr->translation.y +
			block_ptr->col_mesh_ptr->maxBox.y;
	else
		player_viewpoint.position.y -= world_ptr->half_block_units;

	// If this teleport did not occur due to a history entry being selected, then
	// set a flag indicating that a teleport took place.

	if (!history_entry_selected)
		player_was_teleported = true;
}

//------------------------------------------------------------------------------
// Update the spot history list.
//------------------------------------------------------------------------------

void
update_spot_history_list(const char *spot_URL, const char *spot_entrance,
						 bool history_entry_selected) 
{
	history_entry *history_entry_ptr;

	// Get the current history entry.  If the history list is empty, just
	// insert the new history entry.

	start_atomic_operation();
	history_entry_ptr = spot_history_list->get_current_entry();
	if (history_entry_ptr == NULL)
		spot_history_list->insert_entry(spot_title, spot_URL, spot_entrance);

	// If the history list is not empty...

	else {
		
		// If we've already seen the first spot URL, save the current player
		// viewpoint in the current history entry, or the previous history
		// entry if the new history entry was selected by the user from the
		// history list.

		if (seen_first_spot_URL) {
			if (history_entry_selected) {
				history_entry *prev_history_entry_ptr =
					spot_history_list->get_previous_entry();
				prev_history_entry_ptr->player_viewpoint = player_viewpoint;
			} else
				history_entry_ptr->player_viewpoint = player_viewpoint;
		}

		// If the URL for this spot does not match the URL of the current
		// history entry...

		if (stricmp(spot_URL, history_entry_ptr->link.URL)) {

			// Move onto the next history entry.

			spot_history_list->curr_entry_index++;

			// If the next history entry does not exist or is different to the
			// new spot URL, insert a new history entry.

			history_entry_ptr = spot_history_list->get_current_entry();
			if (history_entry_ptr == NULL ||
				stricmp(spot_URL, history_entry_ptr->link.URL))
				spot_history_list->insert_entry(spot_title, spot_URL, 
					spot_entrance);
		}
	}
	end_atomic_operation();
}

//------------------------------------------------------------------------------
// Attempt to process the current URL as a spot.
//------------------------------------------------------------------------------

static bool
handle_exit(const char *exit_URL, const char *exit_target,
			bool history_entry_selected, bool return_to_entrance);

static bool
process_spot_URL(bool is_base_spot_URL, bool history_entry_selected,
				 bool return_to_entrance)
{
	// Clear the frame buffer.  If hardware full-screen mode is in effect,
	// clear the frame buffer twice to reset both the front and back
	// buffers.

	clear_frame_buffer(0, 0, window_width, window_height);
	display_frame_buffer(true, !seen_first_spot_URL);
	if (full_screen) {
		clear_frame_buffer(0, 0, window_width, window_height);
		display_frame_buffer(true, !seen_first_spot_URL);
	}
		
	// Do everything in a try block in order to catch errors.

	try {
		string spot_file_name;
		history_entry *history_entry_ptr;
		bool restored_last_spot_URL;
		trigger *trigger_ptr;

		// Split the current URL into it's components, after saving it as the
		// full spot URL.

		full_spot_URL = curr_URL;
		split_URL(full_spot_URL, &spot_URL_dir, &spot_file_name, 
			&curr_spot_entrance);

		// Get the current history entry.

		history_entry_ptr = spot_history_list->get_current_entry(); 

		// If this is the first spot URL to be seen since the player started,
		// and we're still in the same browser session, check it against the
		// base spot URL in the history list.  If it matches, download the spot
		// URL in the current history entry and signify that we have restored 
		// the last spot URL.

		restored_last_spot_URL = false;
		if (!seen_first_spot_URL && same_browser_session &&
			!stricmp(full_spot_URL, spot_history_list->base_spot_URL) &&
			history_entry_ptr != NULL) {
			if (download_URL(history_entry_ptr->link.URL, NULL)) {
				full_spot_URL = curr_URL;
				split_URL(full_spot_URL, &spot_URL_dir, &spot_file_name, 
					&curr_spot_entrance);
				restored_last_spot_URL = true;
			}
		}

		// Recreate the spot URL without the entrance name.

		curr_spot_URL = create_URL(spot_URL_dir, NULL, spot_file_name);

		// Display a message on the task bar while parsing the spot file.

		set_title("Loading %s", spot_file_name);

		// Attempt to open the spot using the curr file path.
	
		parse_spot_file(curr_spot_URL, curr_file_path);

		// If the minimum version required for this spot is greater than Rover's
		// version number, and the spot URL is web-based, request that the
		// version URL be downloaded.

		if (min_rover_version > ROVER_VERSION_NUMBER &&
			!strnicmp(curr_spot_URL, "http://", 7)) {
			update_in_progress = true;
			update_type = SPOT_REQUESTED_UPDATE;
			version_URL = VERSION_URL;
			version_URL_download_requested.send_event(true);
		}

		// Set up the renderer.

		set_up_renderer();
		
		// If a stream is set, start the streaming thread.

		if (stream_set)
			start_streaming_thread();

		// Update the history list.

		update_spot_history_list(full_spot_URL, curr_spot_entrance, 
			history_entry_selected);

		// If we didn't restore the last spot URL and this is the base spot
		// URL, store it in the history list.

		if (!restored_last_spot_URL && is_base_spot_URL)
			spot_history_list->base_spot_URL = full_spot_URL;

		// Indicate we've seen the first spot URL.

		seen_first_spot_URL = true;

		// Reset start and current time, and frame count.

		start_time_ms = get_time_ms();
		curr_time_ms = get_time_ms();
		frames_rendered = 0;
		total_polygons_rendered = 0;

		// Initialise the global triggers not associated with block definitions.

		trigger_ptr = global_trigger_list;
		while (trigger_ptr) {
			init_global_trigger(trigger_ptr);
			trigger_ptr = trigger_ptr->next_trigger_ptr;
		}

		// Perform the initialisations that can be done before the custom
		// textures and sounds are downloaded.

		init_spot();

		// If we restored the last spot URL or the user did not chose to return
		// to the entrance, set the player viewpoint to that contained in the
		// history entry.  Otherwise teleport to the approapiate entrance.

		if (restored_last_spot_URL || !return_to_entrance) 
			player_viewpoint = history_entry_ptr->player_viewpoint;
		else
			teleport(curr_spot_entrance, false);

		// Indicate that the player viewpoint has been set.

		player_viewpoint_set = true;

		// If there is an onload exit, trigger it now.

		if (onload_exit_ptr) {
			if (!handle_exit(onload_exit_ptr->URL, onload_exit_ptr->target,
				false, false))
				return(false);
		}

		// Initiate the downloading of the first custom texture or wave.

		initiate_first_download();

		// Initialise the player's fall delta and the trajectory tilted flag.

		player_fall_delta = 0;
		trajectory_tilted = false;
		return(true);
	}

	// Catch any fatal error that occurs and write it to the log file, then
	// allow the user to view the error log if the debug flag is set, otherwise
	// just display a generic error message.

	catch (char *message) {
		pop_all_files();
		write_error_log(message);
		if (low_memory)
			display_low_memory_error();
		else if (debug_on)
			display_error_log.send_event(true);
		else
			fatal_error("Unable to display 3DML document", "One or more errors "
				"in the 3DML document has prevented the Flatland Rover from "
				"displaying it.");
		return(false);
	}
}

//------------------------------------------------------------------------------
// Handle the activation of an exit.
//------------------------------------------------------------------------------

static bool
handle_exit(const char *exit_URL, const char *exit_target,
			bool history_entry_selected, bool return_to_entrance)
{
	// If the exit URL starts with the hash sign, then treat this as an
	// entrance name within the current spot.

	if (*exit_URL == '#') {
		string URL = curr_spot_URL + exit_URL;
		const char *entrance_name = exit_URL + 1;
		update_spot_history_list(URL, entrance_name, history_entry_selected);
		teleport(entrance_name, history_entry_selected);
	}

	// Otherwise assume we have a new spot or other URL to download...

	else {
		string URL;
		string URL_dir;
		string file_name;
		string entrance_name;
		char *ext_ptr;

		// Create the URL by prepending the spot file directory if the
		// exit reference is a relative path.  Then split the URL into
		// it's components.

		URL = create_URL(spot_URL_dir, NULL, exit_URL);
		split_URL(URL, &URL_dir, &file_name, &entrance_name);

		// If the file name ends with ".3dml" then it's a spot URL...

		ext_ptr = strrchr(file_name, '.');
		if (ext_ptr && !stricmp(ext_ptr, ".3dml")) {
			string spot_URL;

			// Recreate the spot URL without the entrance name.  If it's the
			// same as the current spot URL, then teleport to the specified
			// entrance or restore the player viewpoint from the history
			// entry.

			spot_URL = create_URL(URL_dir, NULL, file_name);
			if (!stricmp(spot_URL, curr_spot_URL)) {
				update_spot_history_list(URL, entrance_name,
					history_entry_selected);
				if (return_to_entrance)
					teleport(entrance_name, false);
				else {
					history_entry *history_entry_ptr = 
						spot_history_list->get_current_entry();
					player_viewpoint = history_entry_ptr->player_viewpoint;
				}
				return(true);
			}

			// Download the new 3DML file.  If the download failed, reset the
			// mouse clicked event in case a link was selected while the URL
			// was downloading, then return with a success status to allow the
			// display of the current spot to continue.

			if (!download_URL(spot_URL, NULL)) {
				fatal_error("Unable to download 3DML document", 
					"Unable to download 3DML document from %s", spot_URL);
				mouse_clicked.reset_event();
				return(true);
			}

			// Append the entrance name to the current URL.

			curr_URL += "#";
			curr_URL += entrance_name;

			// If a stream is set, stop the streaming thread.

			if (stream_set)
				stop_streaming_thread();

			// Delete the unscaled video texture, and the scaled video texture
			// list, if they exist.

			if (unscaled_video_texture_ptr)
				DEL(unscaled_video_texture_ptr, texture);
			while (scaled_video_texture_list) {
				video_texture *next_video_texture_ptr =
					scaled_video_texture_list->next_video_texture_ptr;
				DEL(scaled_video_texture_list, video_texture);
				scaled_video_texture_list = next_video_texture_ptr;
			}

			// Re-create the image caches, then unload the current spot and
			// initialise the spot data.
			
			delete_image_caches();
			if (!create_image_caches())
				return(false);
			free_spot_data();
			init_spot_data();

			// Process the new spot URL.

			if (!process_spot_URL(false, history_entry_selected, 
				return_to_entrance))
				return(false);
		}

		// If the URL is javascript, request that it be executed but any data
		// that might be downloaded be thrown away.

		else if (!strnicmp(URL, "javascript:", 11)) {
			javascript_URL = URL;
			javascript_URL_download_requested.send_event(true);
		}

		// Otherwise have the browser request that the URL be downloaded into
		// the target window, or "_self" if there is no target supplied.  
		// We don't need to wait for the URL to be downloaded in the latter
		// case, since the plugin will automatically be unloaded by the
		// browser.

		else {
			if (strlen(exit_target) != 0)
				request_URL(URL, NULL, exit_target, false);
			else
				request_URL(URL, NULL, "_self", false);
		}
	}

	// Signal success.

	return(true);
}

//------------------------------------------------------------------------------
// Show the first trigger label in the given trigger list.
//------------------------------------------------------------------------------

static bool
show_trigger_label(trigger *trigger_list)
{
	trigger *trigger_ptr = trigger_list;
	while (trigger_ptr) {
		if (trigger_ptr->trigger_flag == CLICK_ON &&
			strlen(trigger_ptr->label) != 0) {
			show_label(trigger_ptr->label);
			return(true);
		}
		trigger_ptr = trigger_ptr->next_trigger_ptr;
	}
	return(false);
}

//------------------------------------------------------------------------------
// Check to see whether the mouse is pointing at an exit or a square with a
// "click on" action.
//------------------------------------------------------------------------------

static void
check_for_mouse_selection(void)
{
	bool label_shown;

	// If the currently selected square, area or exit is different to
	// the previously selected square, area square or exit...

	if (curr_selected_square_ptr != prev_selected_square_ptr ||
		curr_area_square_ptr != prev_area_square_ptr ||
		curr_selected_exit_ptr != prev_selected_exit_ptr) {

		// If there is a currently selected square, area or exit with a
		// "click on" action...

		label_shown = false;
		if ((curr_selected_square_ptr && 
			 ((curr_selected_square_ptr->block_trigger_flags & CLICK_ON) ||
			  (curr_selected_square_ptr->square_trigger_flags & CLICK_ON))) ||
			(curr_selected_area_ptr && 
			 (curr_selected_area_ptr->trigger_flags & CLICK_ON)) ||
			(curr_selected_exit_ptr &&
			 (curr_selected_exit_ptr->trigger_flags & CLICK_ON))) {

			// Indicate that there is an active selection.

			start_atomic_operation();
			selection_active = true;
			end_atomic_operation();

			// If there is a currently selected exit and it has a label,
			// show it.

			if (curr_selected_exit_ptr && 
				strlen(curr_selected_exit_ptr->label) != 0) {
				show_label(curr_selected_exit_ptr->label);
				label_shown = true;
			}

			// Otherwise if there is a currently selected square and one of it's
			// triggers has a label, show it (a block trigger takes precedence).

			else if (curr_selected_square_ptr && show_trigger_label(
				curr_selected_square_ptr->block_trigger_list))
				label_shown = true;
			else if (curr_selected_square_ptr && show_trigger_label(
				curr_selected_square_ptr->square_trigger_list))
				label_shown = true;

			// Otherwise if there is a currently selected area and one of it's
			// triggers has a label, show it.

			else if (curr_selected_area_ptr && show_trigger_label(
				curr_selected_area_ptr->trigger_list))
				label_shown = true;
		}

		// If there is nothing selected, indicate there is no active selection.

		else {
			start_atomic_operation();
			selection_active = false;
			end_atomic_operation();
		}

		// If no label was shown, hide any labels that might already be shown.

		if (!label_shown)
			hide_label();
	}
}

//------------------------------------------------------------------------------
// Create a list of blocks that overlap the bounding box of a new player
// position.
//------------------------------------------------------------------------------

static void
get_overlapping_blocks(float x, float y, float z, 
					   float old_x, float old_y, float old_z)
{
	vertex min_view, max_view;
	int min_level, min_row, min_column;
	int max_level, max_row, max_column;
	int level, row, column;
	block *block_ptr;
	COL_MESH *col_mesh_ptr;
	vertex min_bbox, max_bbox;

	// Compute a bounding box for the new view, and determine which blocks are
	// at the corners of this bounding box.  Note that min_row and max_row are
	// swapped because as the Z coordinate increases the row number decreases.

	min_view.x = MIN(x - player_collision_box.maxDim.x,
		old_x - player_collision_box.maxDim.x);
	min_view.y = MIN(y, old_y);
	min_view.z = MIN(z - player_collision_box.maxDim.z,
		old_z - player_collision_box.maxDim.z);
	max_view.x = MAX(x + player_collision_box.maxDim.x,
		old_x + player_collision_box.maxDim.x);
	max_view.y = MAX(y + player_size.y, old_y + player_size.y);
	max_view.z = MAX(z + player_collision_box.maxDim.z,
		old_z + player_collision_box.maxDim.z);
	min_view.get_scaled_map_position(&min_column, &max_row, &min_level);
	max_view.get_scaled_map_position(&max_column, &min_row, &max_level);

	// Include the level below the player collision box, if there is one.  This
	// ensures the floor height can be determined.

	if (min_level > 0)
		min_level--;

	// Step through the range of overlapping blocks and add them to a list of
	// blocks to check for collisions.

	col_meshes = 0;
	for (level = min_level; level <= max_level; level++)
		for (row = min_row; row <= max_row; row++)
			for (column = min_column; column <= max_column; column++)
				if ((block_ptr = world_ptr->get_block_ptr(column, row, level))
					!= NULL) {
					if (block_ptr->solid && block_ptr->col_mesh_ptr != NULL) {
						col_mesh_list[col_meshes] = block_ptr->col_mesh_ptr;
						mesh_pos_list[col_meshes].x = block_ptr->translation.x;
						mesh_pos_list[col_meshes].y = block_ptr->translation.y;
						mesh_pos_list[col_meshes].z = block_ptr->translation.z;
						col_meshes++;
					}
				}

	// Step through the list of movable blocks, and add them to the list of
	// blocks to check for collisions.

	block_ptr = movable_block_list;
	while (block_ptr) {
		if (block_ptr->solid && block_ptr->col_mesh_ptr != NULL) {
			col_mesh_ptr = block_ptr->col_mesh_ptr;
			min_bbox.x = col_mesh_ptr->minBox.x + block_ptr->translation.x;
			min_bbox.y = col_mesh_ptr->minBox.y + block_ptr->translation.y;
			min_bbox.z = col_mesh_ptr->minBox.z + block_ptr->translation.z;
			max_bbox.x = col_mesh_ptr->maxBox.x + block_ptr->translation.x;
			max_bbox.y = col_mesh_ptr->maxBox.y + block_ptr->translation.y;
			max_bbox.z = col_mesh_ptr->maxBox.z + block_ptr->translation.z;
			if (!(min_bbox.x > max_view.x || max_bbox.x < min_view.x ||
				  min_bbox.y > max_view.y || max_bbox.y < min_view.y ||
				  min_bbox.z > max_view.z || max_bbox.z < min_view.z)) {
				col_mesh_list[col_meshes] = block_ptr->col_mesh_ptr;
				mesh_pos_list[col_meshes].x = block_ptr->translation.x;
				mesh_pos_list[col_meshes].y = block_ptr->translation.y;
				mesh_pos_list[col_meshes].z = block_ptr->translation.z;
				col_meshes++;
			}
		}
		block_ptr = block_ptr->next_block_ptr;
	}
}

//------------------------------------------------------------------------------
// Adjust the player's trajectory to take into consideration collisions with
// polygons.
//------------------------------------------------------------------------------

vector
adjust_trajectory(vector &trajectory, float elapsed_time, bool &player_falling)
{
	float max_dx, max_dz;
	float abs_dx, abs_dz;
	vector new_trajectory;
	vertex old_position, new_position;
	float floor_y;
	int column, row, level;

	// Set the maximum length permitted for the trajectory.

	max_dx = player_step_height;
	max_dz = player_step_height;

	// Compute the absolute values of the X and Z trajectory components.

	abs_dx = ABS(trajectory.dx);
	abs_dz = ABS(trajectory.dz);

	// If the X and Z components of the trajectory are less or equal to the
	// X and Z components of the maximum trajectory vector, then use this 
	// trajectory.

	if (FLE(abs_dx, max_dx) && FLE(abs_dz, max_dz)) {
		new_trajectory = trajectory;
		trajectory.set(0.0f, 0.0f, 0.0f);
	} 
	
	// Compute a new trajectory such that the player moves along the axis
	// of the largest component no further than the matching component of the
	// maximum trajectory vector.
	
	else {
		if (FGT(abs_dx, abs_dz)) {
			new_trajectory.dx = max_dx * (trajectory.dx / abs_dx);
			new_trajectory.dy = 0.0f;
			new_trajectory.dz = max_dx * (trajectory.dz / abs_dx);
		} else {
			new_trajectory.dx = max_dz * (trajectory.dx / abs_dz);
			new_trajectory.dy = 0.0f;
			new_trajectory.dz = max_dz * (trajectory.dz / abs_dz);
		}
		trajectory.dx -= new_trajectory.dx;
		trajectory.dz -= new_trajectory.dz;
	}

	// If the new player position is off the map, don't move the player at all
	// and make the floor height invalid.

	old_position = player_viewpoint.position;
	new_position = old_position + new_trajectory;
	new_position.get_scaled_map_position(&column, &row, &level);
	if (new_position.x < 0.0 || column >= world_ptr->columns || 
		row < 0 || new_position.z < 0.0 ||
		new_position.y < 0.0 || level >= world_ptr->levels) {
		new_position = old_position;
		floor_y = -1.0f;
	}

	// Otherwise get the list of blocks that overlap the trajectory path,
	// and check for collisions along that path, adjusting the player's new
	// position approapiately.

	else {
		get_overlapping_blocks(new_position.x, new_position.y, new_position.z, 
			old_position.x, old_position.y, old_position.z);
		COL_checkCollisions(col_mesh_list, mesh_pos_list, col_meshes,
			&new_position.x, &new_position.y, &new_position.z,
			old_position.x, old_position.y, old_position.z,
			&floor_y, player_step_height, &player_collision_box);
	}

	// If the floor height is valid, do a gravity check.

	player_falling = false;
	if (FGE(floor_y, 0.0f)) {

		// If the player height is below the floor height, determine whether
		// the player can step up to that height; if not, the player should not
		// move at all.

		if (FLT(new_position.y, floor_y)) {
			if (FLT(floor_y - new_position.y, player_step_height))
				new_position.y = floor_y;
			else
				new_position = old_position;
		} 
		
		// If the player height is above the floor height, we are in free
		// fall...
	
		else {
		
			// If we can step down to the floor height, do so.

			if (FLT(new_position.y - floor_y, player_step_height)) {
				new_position.y = floor_y;
				player_fall_delta = 0.0f;
			} 
		
			// Otherwise, allow gravity to pull the player down by the given
			// fall delta.  If this puts the player below the floor height,
			// put them at the floor height and zero the fall delta.  Otherwise
			// increase the fall delta over time until a maximum delta is
			// reached.
			
			else {
				player_falling = true;
				new_position.y -= player_fall_delta;
				if (FLE(new_position.y, floor_y)) {
					new_position.y = floor_y;
					player_fall_delta = 0.0f;
				} else {
					player_fall_delta += 1.0f * elapsed_time;
					if (FGT(player_fall_delta, 3.0f))
						player_fall_delta = 3.0f;
				}
			}
		}
	} 
	
	// If the floor height is not -1.0f, this means that COL_checkCollisions()
	// returned a bogus new position, so set it to the old position.
	
	else if (FNE(floor_y, -1.0f))
		new_position = old_position;

	// Return the final trajectory vector.

	return(new_position - old_position);
}

//------------------------------------------------------------------------------
// Add a trigger to the active trigger list.
//------------------------------------------------------------------------------

static void
add_trigger_to_active_list(square *square_ptr, trigger *trigger_ptr)
{
	trigger *new_trigger_ptr;

	// Duplicate the trigger.
	
	new_trigger_ptr = dup_trigger(trigger_ptr);
	if (new_trigger_ptr) {

		// Initialise the trigger.

		new_trigger_ptr->square_ptr = square_ptr;
		new_trigger_ptr->next_trigger_ptr = NULL;

		// Add the trigger to the end of the active trigger list.

		if (last_active_trigger_ptr)
			last_active_trigger_ptr->next_trigger_ptr = new_trigger_ptr;
		else
			active_trigger_list = new_trigger_ptr;
		last_active_trigger_ptr = new_trigger_ptr;
	}
}

//------------------------------------------------------------------------------
// Step through the given trigger list, adding those that are included in the
// given trigger flags to the active trigger list.
//------------------------------------------------------------------------------

static void
add_triggers_to_active_list(square *square_ptr, trigger *trigger_list,
							int trigger_flags)
{
	trigger *trigger_ptr;

	// Step through the trigger list, looking for triggers to add to the
	// active list.

	trigger_ptr = trigger_list;
	while (trigger_ptr) {
		if ((trigger_ptr->trigger_flag & trigger_flags) != 0)
			add_trigger_to_active_list(square_ptr, trigger_ptr);
		trigger_ptr = trigger_ptr->next_trigger_ptr;
	}
}

//------------------------------------------------------------------------------
// Process the previously selected square, looking for active triggers.
//------------------------------------------------------------------------------

static void
process_prev_selected_square(void)
{
	int trigger_flags;

	// Reset the trigger flags.

	trigger_flags = 0;

	// If there was a previously selected square, and the currently selected
	// square is different...

	if (curr_selected_square_ptr != prev_selected_square_ptr &&
		prev_selected_square_ptr != NULL) {
		
		// If the previously selected block's square has a "roll off" trigger,
		// add this to the trigger flags.

		if ((prev_selected_square_ptr->block_trigger_flags & ROLL_OFF) ||
			(prev_selected_square_ptr->square_trigger_flags & ROLL_OFF))
			trigger_flags |= ROLL_OFF;
	}

	// If there are any trigger flags set, add the active triggers from the
	// previously selected square to the active trigger list.

	if (trigger_flags) {
		add_triggers_to_active_list(prev_selected_square_ptr,
			prev_selected_square_ptr->block_trigger_list, trigger_flags);
		add_triggers_to_active_list(prev_selected_square_ptr,
			prev_selected_square_ptr->square_trigger_list, trigger_flags);
	}
}

//------------------------------------------------------------------------------
// Process the currently selected square, looking for active triggers.
//------------------------------------------------------------------------------

static void
process_curr_selected_square(void)
{
	int trigger_flags;

	// Reset the trigger flags.

	trigger_flags = 0;

	// If there is a currently selected square, and the previously selected
	// square is different...

	if (curr_selected_square_ptr != prev_selected_square_ptr &&
		curr_selected_square_ptr != NULL) {

		// If the currently selected square has a "roll on" trigger, add this
		// to the trigger flags.

		if ((curr_selected_square_ptr->block_trigger_flags & ROLL_ON) ||
			(curr_selected_square_ptr->square_trigger_flags & ROLL_ON))
			trigger_flags |= ROLL_ON;
	}

	// If the mouse has been clicked, and the currently selected square has
	// a "click on" trigger, add this to the trigger flags.

	if (mouse_was_clicked && curr_selected_square_ptr &&
		((curr_selected_square_ptr->block_trigger_flags & CLICK_ON) ||
		 (curr_selected_square_ptr->square_trigger_flags & CLICK_ON)))
		 trigger_flags |= CLICK_ON;

	// If there are any trigger flags set, add the active triggers from the
	// currently selected square to the active trigger list.

	if (trigger_flags) {
		add_triggers_to_active_list(curr_selected_square_ptr,
			curr_selected_square_ptr->block_trigger_list, trigger_flags);
		add_triggers_to_active_list(curr_selected_square_ptr,
			curr_selected_square_ptr->square_trigger_list, trigger_flags);
	}
}

//------------------------------------------------------------------------------
// Process the previously selected area, looking for active triggers.
//------------------------------------------------------------------------------

static void
process_prev_selected_area(void)
{
	int trigger_flags;

	// Reset the trigger flags.

	trigger_flags = 0;

	// If there was a previously selected area square, and the currently 
	// selected area square is different...

	if (curr_area_square_ptr != prev_area_square_ptr &&
		prev_area_square_ptr != NULL) {
		
		// If the previously selected area has a "roll off" trigger,
		// add this to the trigger flags.

		if (prev_selected_area_ptr->trigger_flags & ROLL_OFF)
			trigger_flags |= ROLL_OFF;
	}

	// If there are any trigger flags set, add the active triggers from the
	// previously selected area to the active trigger list.

	if (trigger_flags)
		add_triggers_to_active_list(prev_area_square_ptr,
			prev_selected_area_ptr->trigger_list, trigger_flags);
}

//------------------------------------------------------------------------------
// Process the currently selected area, looking for active triggers.
//------------------------------------------------------------------------------

static void
process_curr_selected_area(void)
{
	int trigger_flags;

	// Reset the trigger flags.

	trigger_flags = 0;

	// If there is a currently selected area square, and the previously
	// selected area square is different...

	if (curr_area_square_ptr != prev_area_square_ptr &&
		curr_area_square_ptr != NULL) {

		// If the currently selected area has a "roll on" trigger, add this
		// to the trigger flags.

		if (curr_selected_area_ptr->trigger_flags & ROLL_ON)
			trigger_flags |= ROLL_ON;
	}

	// If the mouse has been clicked, and the currently selected area has
	// a "click on" trigger, add this to the trigger flags.

	if (mouse_was_clicked && curr_selected_area_ptr &&
		(curr_selected_area_ptr->trigger_flags & CLICK_ON))
		trigger_flags |= CLICK_ON;

	// If there are any trigger flags set, add the active triggers from the
	// currently selected area to the active trigger list.

	if (trigger_flags)
		add_triggers_to_active_list(curr_area_square_ptr,
			curr_selected_area_ptr->trigger_list, trigger_flags);
}

//------------------------------------------------------------------------------
// Process the list of global triggers, looking for active ones.
//------------------------------------------------------------------------------

static void
process_global_trigger_list(void)
{
	trigger *trigger_ptr;
	bool activated;
	vector distance;
	float distance_squared;
	bool currently_inside;
	square *square_ptr;
	int column, row, level;

	// Step through the global trigger list.

	trigger_ptr = global_trigger_list;
	while (trigger_ptr) {

		// If this is a "step in" or "step out" or "proximity" trigger, compute
		// the distance squared from the trigger to the player, and from this 
		// determine whether the player is inside the radius.

		if (trigger_ptr->trigger_flag == STEP_IN ||
			trigger_ptr->trigger_flag == STEP_OUT ||
			trigger_ptr->trigger_flag == PROXIMITY) {
			distance = trigger_ptr->position - player_viewpoint.position;
			distance_squared = distance.dx * distance.dx + 
				distance.dy * distance.dy + distance.dz * distance.dz;
			currently_inside = distance_squared <= trigger_ptr->radius_squared;
		}

		// Check for an activated trigger.

		activated = false;
		switch (trigger_ptr->trigger_flag) {
		case STEP_IN:

			// If the new state of the trigger is "inside", and the old state
			// of the trigger was not "inside", activate this trigger.

			if (currently_inside && !trigger_ptr->previously_inside)
				activated = true;
			break;

		case STEP_OUT:

			// If the new state of the trigger is "outside", and the old state
			// of the trigger was not "outside", activate this trigger.

			if (!currently_inside && trigger_ptr->previously_inside)
				 activated = true;
			break;

		case PROXIMITY:

			// If the player in inside the radius, activate this trigger.

			activated = currently_inside;
			break;

		case TIMER:

			// If the trigger delay has elapsed, activate this trigger and
			// set the next trigger delay.

			if (curr_time_ms - trigger_ptr->start_time_ms >= 
				trigger_ptr->delay_ms) {
				activated = true;
				set_trigger_delay(trigger_ptr, curr_time_ms);
			}
			break;

		case LOCATION:

			// Get the location of the trigger square.

			square_ptr = trigger_ptr->square_ptr;
			world_ptr->get_square_location(square_ptr, &column, &row, &level);

			// If this matches the target location, activate the trigger.

			if (column == trigger_ptr->target.column &&
				row == trigger_ptr->target.row && 
				level == trigger_ptr->target.level)
				activated = true;
		}

		// If this is a "step in" or "step out" trigger, update the trigger's
		// state.

		if (trigger_ptr->trigger_flag == STEP_IN ||
			trigger_ptr->trigger_flag == STEP_OUT)
			trigger_ptr->previously_inside = currently_inside;

		// If trigger was activated, add it to the active trigger list.

		if (activated)
			add_trigger_to_active_list(trigger_ptr->square_ptr, trigger_ptr);

		// Move onto the next global trigger.

		trigger_ptr = trigger_ptr->next_trigger_ptr;
	}
}

//------------------------------------------------------------------------------
// Execute a replace action.
//------------------------------------------------------------------------------

static void
execute_replace_action(trigger *trigger_ptr, action *action_ptr)
{
	square *trigger_square_ptr, *target_square_ptr;
	int trigger_column, trigger_row, trigger_level;
	int target_column, target_row, target_level;
	int source_column, source_row, source_level;
	relcoords *target_location_ptr, *source_location_ptr;
	block *block_ptr;
	block_def *block_def_ptr;
	word block_symbol;

	// Get the location of the trigger square.

	trigger_square_ptr = trigger_ptr->square_ptr;
	world_ptr->get_square_location(trigger_square_ptr, 
		&trigger_column, &trigger_row, &trigger_level);

	// If the trigger square is also the target square, use it.

	if (action_ptr->target_is_trigger) {
		target_column = trigger_column;
		target_row = trigger_row;
		target_level = trigger_level;
		target_square_ptr = trigger_square_ptr;
	}

	// If the trigger square is not the target square, calculate the
	// location of the target square, and get a pointer to that square.
	// If the target uses a relative coordinate for the level and a
	// ground level exists, make sure the ground level is never
	// selected.

	else {
		target_location_ptr = &action_ptr->target;
		target_column = target_location_ptr->column;
		if (target_location_ptr->relative_column) {
			target_column = (trigger_column + target_column) % 
				world_ptr->columns;
			if (target_column < 0)
				target_column += world_ptr->columns;
		}
		target_row = target_location_ptr->row;
		if (target_location_ptr->relative_row) {
			target_row = (trigger_row + target_row) % world_ptr->rows;
			if (target_row < 0)
				target_row += world_ptr->rows;
		}
		target_level = target_location_ptr->level;
		if (target_location_ptr->relative_level) {
			if (world_ptr->ground_level_exists) {
				target_level = ((trigger_level - 1) + target_level) %
					(world_ptr->levels - 1);
				if (target_level < 0)
					target_level += world_ptr->levels;
				else
					target_level++;
			} else {
				target_level = (trigger_level + target_level) %
					world_ptr->levels;
				if (target_level < 0)
					target_level += world_ptr->levels;
			}
		}
		target_square_ptr = world_ptr->get_square_ptr(target_column,
			target_row, target_level);
	}

	// Remove the old block at the target square.

	remove_block(target_square_ptr, target_column, target_row, 
		target_level);

	// If the player is standing on the target location, set a flag
	// indicating that the player block has been replaced.

	if (player_column == target_column && player_row == target_row &&
		player_level == target_level)
		player_block_replaced = true;

	// If the target square is the currently selected square, reset that
	// square.

	if (target_square_ptr == curr_selected_square_ptr) {
		curr_selected_square_ptr = NULL;
		hide_label();
	}

	// If the target square is the square of the currently selected area,
	// reset that square and area.

	if (target_square_ptr == curr_area_square_ptr) {
		curr_area_square_ptr = NULL;
		curr_selected_area_ptr = NULL;
		hide_label();
	}

	// If the target square is the square of the currently selected
	// popup, reset that square and popup.

	if (target_square_ptr == curr_popup_square_ptr) {
		curr_popup_square_ptr = NULL;
		hide_label();
	}

	// If the source is a block symbol, get a pointer to the block
	// definition for that symbol.

	if (action_ptr->source.is_symbol) {
		block_symbol = action_ptr->source.symbol;
		if (block_symbol != NULL_BLOCK_SYMBOL) {
			if (block_symbol == GROUND_BLOCK_SYMBOL)
				block_def_ptr = ground_block_def_ptr;
			else
				block_def_ptr = symbol_table[block_symbol];
		} else
			block_def_ptr = NULL;
	} 
	
	// If the source is a location, get a pointer to the block at that
	// location, and from that get a pointer to the block definition.
	
	else {
		source_location_ptr = &action_ptr->source.location;
		source_column = source_location_ptr->column;
		if (source_location_ptr->relative_column) {
			source_column = (trigger_column + source_column) %
				world_ptr->columns;
			if (source_column < 0)
				source_column += world_ptr->columns;
		}
		source_row = source_location_ptr->row;
		if (source_location_ptr->relative_row) {
			source_row = (trigger_row + source_row) % world_ptr->rows;
			if (source_row < 0)
				source_row += world_ptr->rows;
		}
		source_level = source_location_ptr->level;
		if (source_location_ptr->relative_level) {
			source_level = (trigger_level + source_level) %
				world_ptr->levels;
			if (source_level < 0)
				source_level += world_ptr->levels;
		}
		block_ptr = world_ptr->get_block_ptr(source_column, 
			source_row, source_level);
		if (block_ptr)
			block_def_ptr = block_ptr->block_def_ptr;
		else
			block_def_ptr = NULL;
	}

	// Add the new block to the target square, if the block definition
	// exists.

	if (block_def_ptr)
		add_block(block_def_ptr, target_square_ptr, 
			target_column, target_row, target_level, true, false);
}

//------------------------------------------------------------------------------
// Execute the list of active triggers.
//------------------------------------------------------------------------------

static void
execute_active_trigger_list(void)
{
	trigger *trigger_ptr;
	action *action_ptr;

	// Reset the flag that indicates if the global sound list has changed.

	global_sound_list_changed = false;

	// Step through the active triggers.

	trigger_ptr = active_trigger_list;
	while (trigger_ptr) {
		action_ptr = trigger_ptr->action_list;
		while (action_ptr) {
			switch (action_ptr->type) {
			case REPLACE_ACTION:
				execute_replace_action(trigger_ptr, action_ptr);
				break;
			case EXIT_ACTION:
				handle_exit(action_ptr->exit_URL, action_ptr->exit_target, 
					false, true);
				break;
//			case SET_STREAM_ACTION:
//				if (stream_set)
//					send_stream_command(action_ptr->stream_command);
			}

			// Move onto the next action.

			action_ptr = action_ptr->next_action_ptr;
		}

		// Move onto the next trigger, after deleting the current one.

		trigger_ptr = del_trigger(trigger_ptr);
	}

	// Clear the active trigger list and the last active trigger pointer.

	active_trigger_list = NULL;
	last_active_trigger_ptr = NULL;

#ifdef SUPPORT_A3D

	// If the global sound list has changed, reset the audio, 
	// and reinitialise the audio square map if A3D sound is enabled.

	if (global_sound_list_changed && sound_on) {
		world_ptr->clear_audio_square_map();
		reset_audio();
		world_ptr->init_audio_square_map();
	}

#endif
}

//------------------------------------------------------------------------------
// Function to set up a clipping plane.
//------------------------------------------------------------------------------

static void
set_clipping_plane(int plane_index, float z)
{
	float tz = z + 1.6f;

	frustum_vertex_list[plane_index].x = -half_viewport_width * tz;
	frustum_vertex_list[plane_index].y = half_viewport_height * tz;
	frustum_vertex_list[plane_index].z = z;
	frustum_vertex_list[plane_index + 1].x = half_viewport_width * tz;
	frustum_vertex_list[plane_index + 1].y = half_viewport_height * tz;
	frustum_vertex_list[plane_index + 1].z = z;
	frustum_vertex_list[plane_index + 2].x = half_viewport_width * tz;
	frustum_vertex_list[plane_index + 2].y = -half_viewport_height * tz;
	frustum_vertex_list[plane_index + 2].z = z;
	frustum_vertex_list[plane_index + 3].x = -half_viewport_width * tz;
	frustum_vertex_list[plane_index + 3].y = -half_viewport_height * tz;
	frustum_vertex_list[plane_index + 3].z = z;
}

//------------------------------------------------------------------------------
// Render next frame.
//------------------------------------------------------------------------------

static bool 
render_next_frame(void)
{
	int prev_time_ms;
	float elapsed_time;
	float move_delta, side_delta, turn_delta, look_delta;
	vector trajectory, new_trajectory, unit_trajectory;
	bool player_falling;
	vector orig_direction, new_direction;
	float desired_look_cosine, desired_look_angle;
	int last_column, last_row, last_level;

	START_TIMING;

	// Update the current time in milliseconds, and compute the elapsed time in
	// seconds.

	if (frames_rendered > 0) {
		prev_time_ms = curr_time_ms;
		curr_time_ms = get_time_ms();
#ifdef RENDERSTATS
		diagnose("Elapsed time since last frame = %d ms", 
			curr_time_ms - prev_time_ms);
#endif
		elapsed_time = (float)(curr_time_ms - prev_time_ms) / 1000.0f;
	} else {
		curr_time_ms = get_time_ms();
		elapsed_time = 0.0f;
	}

	// Get the current mouse position, determine the motion deltas, and set
	// the master intensity.

	start_atomic_operation();
	mouse_x = curr_mouse_x;
	mouse_y = curr_mouse_y;
	if (absolute_motion) {
		move_delta = 0.0f;
		side_delta = 0.0f;
		turn_delta = curr_turn_delta;
		look_delta = curr_look_delta;
		curr_turn_delta = 0.0f;
		curr_look_delta = 0.0f;
	} else {
		move_delta = curr_move_delta * curr_move_rate * elapsed_time;
		side_delta = curr_side_delta * curr_move_rate * elapsed_time;
		turn_delta = curr_turn_delta * curr_rotate_rate * elapsed_time;
		look_delta = curr_look_delta * curr_rotate_rate * elapsed_time;
	}
	set_master_intensity(master_brightness);
	visible_radius = (float)visible_block_radius * world_ptr->block_units;
	end_atomic_operation();

	// Calculate the reflection radius as a quarter of the visible radius,
	// and the audio sound radius as half the visible radius.

	reflection_radius = visible_radius * 0.25f;
	audio_radius = visible_radius * 0.5f;

	// Compute the vertices of the frustum in view space.

	set_clipping_plane(NEAR_CLIPPING_PLANE, 1.0f);
	set_clipping_plane(FAR_CLIPPING_PLANE, visible_radius);

	// Set a flag indicating if we're moving forward or backward.

	forward_movement = FGE(move_delta, 0.0f);

	// Update the player's last position and current turn angle.

	player_viewpoint.last_position = player_viewpoint.position;
	player_viewpoint.turn_angle = 
		pos_adjust_angle(player_viewpoint.turn_angle + turn_delta);

	// Set the trajectory based upon the move delta, side delta, and the turn
	// angle.

	trajectory.dx = move_delta * sine[player_viewpoint.turn_angle] +
		side_delta * sine[player_viewpoint.turn_angle + 90.0f];
	trajectory.dy = 0.0f;
	trajectory.dz = move_delta * cosine[player_viewpoint.turn_angle] +
		side_delta * cosine[player_viewpoint.turn_angle + 90.0f];

	// Adjust the trajectory to take in account collisions, then move the
	// player along this trajectory.

	{
		START_TIMING;

		do {
			new_trajectory = adjust_trajectory(trajectory, elapsed_time, 
				player_falling);
			player_viewpoint.position = player_viewpoint.position + 
				new_trajectory;
		} while (FNE(trajectory.dx, 0.0f) || FNE(trajectory.dz, 0.0f));

		END_TIMING("collision detection");
	}

	// If the new trajectory is zero, adjust the look angle by the look delta.

	if (!new_trajectory) {
		player_viewpoint.look_angle = 
			neg_adjust_angle(player_viewpoint.look_angle + look_delta);
		if (FGT(player_viewpoint.look_angle, 90.0f))
			player_viewpoint.look_angle = 90.0f;
		else if (FLT(player_viewpoint.look_angle, -90.0f))
			player_viewpoint.look_angle = -90.0f;
	}

	// If the new trajectory is non-zero, we set the look angle to match the
	// slope been traversed, but only if the trajectory was tilted in the last
	// frame, or is no longer tilted.
 
	else if (trajectory_tilted || FEQ(new_trajectory.dy, 0.0f)) {
		float delta_look_angle;
		float degrees_per_frame;

		// If the player is falling, the desired look angle should be zero.

		if (player_falling)
			desired_look_angle = 0.0f;
		else {

			// Determine the desired look angle by taking the dot product of the 
			// normalised trajectory vector with a normalised vector pointing
			// along the positive Y axis; this gives us the cosine of the look
			// angle, which we convert to an angle via the arccos() function. 

			unit_trajectory = new_trajectory;
			unit_trajectory.normalise();
			desired_look_cosine = unit_trajectory & world_y_axis;
			desired_look_angle = (float)DEG(acos(desired_look_cosine)) - 90.0f;

			// Compute the dot product between the normalised vector
			// representing the turn angle (in the X-Z plane) and the normalised
			// trajectory vector (also in the X-Z plane).  If the turn angle is 
			// facing away from the trajectory, we must change the sign of the
			// desired look angle to compensate.

			orig_direction.dx = sine[player_viewpoint.turn_angle];
			orig_direction.dy = 0.0;
			orig_direction.dz = cosine[player_viewpoint.turn_angle];
			new_direction = new_trajectory;
			new_direction.dy = 0.0;
			new_direction.normalise();
			if (FLT(orig_direction & new_direction, 0.0))
				desired_look_angle = -desired_look_angle;

			// If the desired look angle is negative, reduce it by 75%;
			// otherwise if it's positive and greater than zero, increase the
			// angle by 25%.  An error of +/- 1 degree around zero is used to
			// smooth out the motion.

			if (FLT(desired_look_angle, -1.0f))
				desired_look_angle *= 0.25f;
			else if (FGT(desired_look_angle, 1.0f))
				desired_look_angle *= 1.25f;
		}

		// Move the player look angle gradually towards the look angle
		// determined above.

		delta_look_angle = player_viewpoint.look_angle - desired_look_angle;
		degrees_per_frame = 90.0f * elapsed_time;
		if (FLT(ABS(delta_look_angle), degrees_per_frame))
			player_viewpoint.look_angle = desired_look_angle;
		else if (FLT(delta_look_angle, 0.0f))
			player_viewpoint.look_angle += degrees_per_frame;
		else
			player_viewpoint.look_angle -= degrees_per_frame;
	}

	// Compute the inverse of the player turn and look angles, and convert them
	// to positive integers.

	player_viewpoint.inv_turn_angle = 
		(int)(360.0f - player_viewpoint.turn_angle);
	player_viewpoint.inv_look_angle = 
		(int)(360.0f - pos_adjust_angle(player_viewpoint.look_angle));

	// Set the trajectory tilted flag.  The trajectory is tilted if there is a
	// Y component to the trajectory in addition to an X or Z component.

	trajectory_tilted = FNE(new_trajectory.dy, 0.0f) && 
		(FNE(new_trajectory.dx, 0.0f) || FNE(new_trajectory.dz, 0.0f));

	// Set a flag indicating whether the viewpoint has changed.

	viewpoint_has_changed = FNE(turn_delta, 0.0f) || FNE(look_delta, 0.0f) ||
		player_viewpoint.position != player_viewpoint.last_position;

	// Update all lights.

	update_all_lights(elapsed_time);

	// Move the player viewpoint to eye height.

	player_viewpoint.position.y += player_size.y;

	// If sound is enabled, update all sounds.

	if (sound_on)
		update_all_sounds();

	// Render the frame.

#ifdef RENDERSTATS
	int start_render_time_ms = get_time_ms();
#endif
	render_frame();
#ifdef RENDERSTATS
	int end_render_time_ms = get_time_ms();
	diagnose("Rendering time = %d ms", 
		end_render_time_ms - start_render_time_ms);
#endif

	// Move the player viewpoint to floor height.

	player_viewpoint.position.y -= player_size.y;

	// Draw the task bar and display the frame.

	display_frame_buffer(false, false);

	// Update the number of frames rendered.

	frames_rendered++;

	// Check for a mouse selection and a mouse clicked event.

	check_for_mouse_selection();
	mouse_was_clicked = mouse_clicked.event_sent();

	// Determine the location the player is standing on.

	player_viewpoint.position.get_scaled_map_position(&player_column, 
		&player_row, &player_level);

	{
		START_TIMING;

		// Process the currently and previously selected squares, followed by
		// the currently and previously selected areas, followed by the list of
		// global triggers.

		process_prev_selected_square();
		process_curr_selected_square();
		process_prev_selected_area();
		process_curr_selected_area();
		process_global_trigger_list();

		// Execute the list of active triggers.

		player_block_replaced = false;
		execute_active_trigger_list();

		END_TIMING("actions");
	}

	// If a polygon info requested event has been sent, display the number of
	// the currently selected polygon, and the name of the block definition it
	// belongs to.

#ifdef _DEBUG
	if (polygon_info_requested.event_sent()) {
		if (curr_selected_polygon_no > 0)
			information("Polygon info", "Pointing at polygon %d of block %s", 
				curr_selected_polygon_no, curr_selected_block_def_ptr->name);
		else
			information("Polygon info", "No polygon is being pointed at");
	}
#endif

	// If the mouse was clicked, and an exit was selected, handle it.

	if (mouse_was_clicked && curr_selected_exit_ptr)
		return(handle_exit(curr_selected_exit_ptr->URL, 
			curr_selected_exit_ptr->target, false, true));

	// If we are standing on the same map square as the previous frame, and
	// the block the player is standing on hasn't been replaced and the player
	// wasn't teleported since the last frame, then we don't need to check
	// whether there is an exit on this square.

	player_viewpoint.last_position.get_scaled_map_position(&last_column,
		&last_row, &last_level);
	if (player_column == last_column && player_row == last_row && 
		player_level == last_level && !player_block_replaced && 
		!player_was_teleported) {
		END_TIMING("render_next_frame");
		return(true);
	}

	// Reset the player was teleported flag.

	player_was_teleported = false;

	// If we are standing on an exit square that has the "step on" trigger flag
	// set, then we need to handle it...

	square *square_ptr = world_ptr->get_square_ptr(player_column, 
		player_row, player_level);
	if (square_ptr != NULL) {
		hyperlink *exit_ptr = square_ptr->exit_ptr;
		if (exit_ptr != NULL && exit_ptr->trigger_flags & STEP_ON) {
			END_TIMING("render_next_frame");
			return(handle_exit(exit_ptr->URL, exit_ptr->target, false, true));
		}
	}
	END_TIMING("render_next_frame");
	return(true);
}

//==============================================================================
// Plugin update functions.
//==============================================================================

//------------------------------------------------------------------------------
// Function to initiate a plugin update, if one is available and the user
// agrees to it.
//------------------------------------------------------------------------------

bool
install_update(void)
{
	FILE *fp;
	string URL;
	unsigned int version_number;
	string message;

	// Reset update in progress flag.

	update_in_progress = false;

	// Parse the version file, extracting the version number and the
	// informational message.  If the version file does not exist or
	// is unreadable, then we cannot complete the update, so remove both
	// the version file and the update archive.

	if (!parse_rover_version_file(version_number, message)) {
		remove(version_file_path);
		remove(update_archive_path);
		return(false);
	}

	// Remove the version file now that we're done with it.

	remove(version_file_path);

	// Ask the user if they want to install the update.

	if (query("New version of Flatland Rover available", true, 
		"Version %s of the Flatland Rover is available for immediate "
		"installation.\n\n%s\n\nWould you like to upgrade to the new "
		"version now?", version_number_to_string(version_number), message)) {
		STARTUPINFO startup_info;
		PROCESS_INFORMATION process_info;

		// Extract the plugin DLL and update HTML page from the
		// downloaded update archive, then remove the update archive.

		try {
			if (!open_ZIP_archive(update_archive_path) ||
				!push_ZIP_file("NPRover.dll") ||
				!copy_file(new_plugin_DLL_path, false) ||
				!push_ZIP_file("update.html") ||
				!copy_file(update_HTML_path, false))
				error("Could not unpack ZIP archive '%s'", 
					update_archive_path);
			pop_all_files();
			close_ZIP_archive();
			remove(update_archive_path);
		}
		catch (char *message) {
			pop_all_files();
			close_ZIP_archive();
			remove(new_plugin_DLL_path);
			remove(update_HTML_path);
			remove(update_archive_path);
			fatal_error("Update failed", "%s", message);
			return(false);
		}
	
		// Spawn a child process that will perform the update.

		memset(&startup_info, 0, sizeof(STARTUPINFO));
		startup_info.cb = sizeof(STARTUPINFO);
		if (!CreateProcess(updater_path, NULL, NULL, NULL, FALSE,
			NORMAL_PRIORITY_CLASS, NULL, NULL, &startup_info, 
			&process_info)) {
			fatal_error("Update failed", "Unable to run the updater program");
			remove(new_plugin_DLL_path);
			remove(update_HTML_path);
			return(false);
		}

		// Create a file called "new_rover.txt" with the value 0 contained
		// within, to indicate this is an update, not a new installation.

		if ((fp = fopen(new_rover_file_path, "w")) != NULL) {
			fprintf(fp, "0");
			fclose(fp);
		}

		// Send off a request for the HTML page containing information
		// about the plugin update.  This will unload the plugin and
		// allow the updater process to finish it's task.

		switch (web_browser_ID) {
		case NAVIGATOR:
			URL = "file:///";
			break;
		case OPERA:
			URL = "file://localhost/";
			break;
		default:
			URL = "file://";
		}
		URL += update_HTML_path;
		if (web_browser_ID == NAVIGATOR && URL[9] == ':')
			URL[9] = '|';
		request_URL(URL, NULL, "_self", false);
		return(true);
	}

	// If they didn't want to install the update, delete the update archive.

	else {
		remove(update_archive_path);
		return(false);
	}
}

//------------------------------------------------------------------------------
// Check for a new plugin update, and if there is one, request that the update
// URL be opened.
//------------------------------------------------------------------------------

void
download_new_update(void)
{
	unsigned int version_number;
	string message;

	// If the version number in the version file is higher than the current
	// Rover version...

	if (parse_rover_version_file(version_number, message) &&
		version_number > ROVER_VERSION_NUMBER) {
		switch (update_type) {

		// If we're doing an auto-update, do a silent request for the update
		// URL (no progress window will appear).

		case AUTO_UPDATE:
			requested_rover_version = (char *)NULL;
			update_URL = UPDATE_URL;
			update_URL_download_requested.send_event(true);
			break;

		// If the user requested the update, ask them out of courtesy
		// whether they want to download it, and if they do request the
		// update URL with a progress bar.  Otherwise reset the update in
		// progress flag and remove the version file.

		case USER_REQUESTED_UPDATE:
			requested_rover_version = version_number_to_string(version_number);
			if (query("Update available", true, "Version %s of Flatland "
				"Rover is available.\nDo you wish to download it now?", 
				requested_rover_version)) {
				update_URL = UPDATE_URL;
				update_URL_download_requested.send_event(true);
			} else {
				update_in_progress = false;
				remove(version_file_path);
			}
			break;

		// If the spot requested the update, ask the user whether they want to
		// download it, and if they do request the update URL with a progress
		// bar.  Otherwise reset the update in progress flag and remove the
		// version file.

		case SPOT_REQUESTED_UPDATE:
			requested_rover_version = version_number_to_string(version_number);
			if (query("Update available", true, "This spot makes use of "
				"features that are only present\nin version %s of Flatland "
				"Rover (you have version %s).\nDo you want to download the "
				"newer version of Flatland Rover?", requested_rover_version,
				version_number_to_string(ROVER_VERSION_NUMBER))) {
				update_URL = UPDATE_URL;
				update_URL_download_requested.send_event(true);
			} else {
				update_in_progress = false;
				remove(version_file_path);
			}
		}
	}

	// Notify the user that there is no update, if necessary, then reset the
	// update in progress flag and remove the version file.

	else if (update_type == USER_REQUESTED_UPDATE) {
		information("No update available",
			"You have the latest version of Flatland Rover (version %s).",
			version_number_to_string(ROVER_VERSION_NUMBER));
		update_in_progress = false;
		remove(version_file_path);
	}
}

//------------------------------------------------------------------------------
// Check whether a new version of Rover has been installed.
//------------------------------------------------------------------------------

static void
check_for_new_rover(void)
{
	FILE *fp;
	int new_installation;
	char location_str[40];
	int str_len;
	string ping_URL;

	// Attempt to open a file called "new_rover.txt" in the Flatland folder; if
	// it doesn't exist, there is no new version of Rover.

	if ((fp = fopen(new_rover_file_path, "r")) == NULL)
		return;

	// Parse the integer value and string contained within, which indicates
	// whether or not this is a new installation and where it came from.

	new_installation = 1;
	*location_str = '\0';
	fscanf(fp, "%d ", &new_installation);
	fgets(location_str, 40, fp);
	fclose(fp);
	str_len = strlen(location_str);
	if (str_len > 0 && location_str[str_len - 1] == '\n')
		location_str[str_len - 1] = '\0';

	// Set the URL to the CGI script.

	ping_URL = "http://www.flatland.com/rovercounter.cgi?";

	// Add the version number of Rover.

	ping_URL += "version=";
	ping_URL += version_number_to_string(ROVER_VERSION_NUMBER);
	ping_URL += ",";

	// Add the new installation flag.

	ping_URL += "newinstall=";
	if (new_installation)
		ping_URL += "yes";
	else
		ping_URL += "no";
	ping_URL += ",";

	// Add the browser type.

	ping_URL += "browser=";
	ping_URL += web_browser_version;
	ping_URL += ",";

	// Add the operating system version.

	ping_URL += "os=";
	ping_URL += os_version;

	// If there is a location string, add it.

	if (*location_str) {
		ping_URL += ",location=";
		ping_URL += location_str;
	}

	// Send the URL to the CGI script.

	request_URL(ping_URL, NULL, NULL, true);

	// Delete the "new_rover.txt" file.

	remove(new_rover_file_path);
}

//==============================================================================
// Main initialisation functions.
//==============================================================================

//------------------------------------------------------------------------------
// Function to pause a spot in preparation for changing window modes.
//------------------------------------------------------------------------------

static void
pause_spot(void)
{
	sound *sound_ptr;

	// Log the stats for the current display mode.

	log_stats();

	// Stop all sounds.

	stop_all_sounds();

	// Destroy all sound buffers of sounds in global sound list.

	sound_ptr = global_sound_list;
	while (sound_ptr) {
		sound_ptr->in_range = false;
		destroy_sound_buffer(sound_ptr);
		sound_ptr = sound_ptr->next_sound_ptr;
	}

	// Destroy sound buffer of ambient sound.

	if (ambient_sound_ptr) {
		ambient_sound_ptr->in_range = false;
		destroy_sound_buffer(ambient_sound_ptr);
	}

#ifdef SUPPORT_A3D

	// If A3D sound is enabled, clear the audio square map.

	if (sound_on)
		world_ptr->clear_audio_square_map();

#endif

	// If a stream is set, stop the streaming thread.

	if (stream_set)
		stop_streaming_thread();
}

//------------------------------------------------------------------------------
// Function to resume a spot after changing window modes.
//------------------------------------------------------------------------------

static void
resume_spot(void)
{
	blockset *blockset_ptr;
	texture *texture_ptr;
	popup *popup_ptr;
	sound *sound_ptr;

	// Recreate the palette list for every 8-bit texture.  Custom textures that
	// don't yet have pixmaps are ignored.

	blockset_ptr = blockset_list_ptr->first_blockset_ptr;
	while (blockset_ptr) {
		texture_ptr = blockset_ptr->first_texture_ptr;
		while (texture_ptr) {
			if (!texture_ptr->is_16_bit) {
				if (hardware_acceleration)
					texture_ptr->create_texture_palette_list();
				else
					texture_ptr->create_display_palette_list();
			}
			texture_ptr = texture_ptr->next_texture_ptr;
		}
		blockset_ptr = blockset_ptr->next_blockset_ptr;
	}
	texture_ptr = custom_blockset_ptr->first_texture_ptr;
	while (texture_ptr) {
		if (texture_ptr->pixmap_list && !texture_ptr->is_16_bit) {
			if (hardware_acceleration)
				texture_ptr->create_texture_palette_list();
			else
				texture_ptr->create_display_palette_list();
		}
		texture_ptr = texture_ptr->next_texture_ptr;
	}

	// Reinitialise the display palette lists for every original popup.
	// Custom background textures that don't yet have pixmaps are ignored.

	popup_ptr = original_popup_list;
	while (popup_ptr) {
		if ((texture_ptr = popup_ptr->bg_texture_ptr) != NULL &&
			texture_ptr->pixmap_list != NULL && !texture_ptr->is_16_bit)
			texture_ptr->create_display_palette_list();
		if ((texture_ptr = popup_ptr->fg_texture_ptr) != NULL &&
			!texture_ptr->is_16_bit)
			texture_ptr->create_display_palette_list();
		popup_ptr = popup_ptr->next_popup_ptr;
	}

#ifdef SUPPORT_A3D

	if (sound_on) {

		// Reset the audio system.

		reset_audio();

		// If A3D sound is enabled, initialise the audio square map.

		world_ptr->init_audio_square_map();
	}

#endif

	// Recreate all sound buffers for non-streaming sounds.  The sounds will
	// restart automatically.

	sound_ptr = global_sound_list;
	while (sound_ptr) {
		if (!sound_ptr->streaming && !create_sound_buffer(sound_ptr))
			warning("Unable to create sound buffer for wave file '%s'",
				sound_ptr->wave_ptr->URL);
		sound_ptr = sound_ptr->next_sound_ptr;
	}
	if (ambient_sound_ptr && !ambient_sound_ptr->streaming &&
		!create_sound_buffer(ambient_sound_ptr))
		warning("Unable to create sound buffer for wave file '%s'",
			ambient_sound_ptr->wave_ptr->URL);

	// If a stream is set, start the streaming thread.

	if (stream_set)
		start_streaming_thread();

	// Reset the stats for the new display mode.

	start_time_ms = get_time_ms();
	frames_rendered = 0;
	total_polygons_rendered = 0;
}

//------------------------------------------------------------------------------
// Global player initialisation.
//------------------------------------------------------------------------------

bool
init_player(void)
{
	FILE *fp;
	char text[BUFSIZ], URL[BUFSIZ];
	int entries, curr_entry_index;
	int size;

	// Initialise the random number generator.

	srand(time(NULL));
	rand();

	// Initialise global variables.

	spot_URL_dir = "";
	blockset_list_ptr = NULL;
	last_logo_time_ms = 0;

	// Initialise local variables.

	check_for_update = true;
	update_in_progress = false;
	seen_first_spot_URL = false;

	// Initialise the parser, renderer and collision detection code.

	init_parser();
	init_renderer();
	COL_init();

	// Load the blockset catalog; if this fails, create it.

	if (!load_cached_blockset_list())
		create_cached_blockset_list();

	// Create the spot history list.

	if ((spot_history_list = new history) == NULL ||
		!spot_history_list->create_history_list(MAX_HISTORY_ENTRIES))
		return(false);

	// Load the history list from the history file.

	if ((fp = fopen(history_file_path, "r")) == NULL)
		return(true);

	// Read the base spot URL.

	if (fgets(URL, BUFSIZ, fp) == NULL || (size = strlen(URL)) == 0) {
		fclose(fp);
		return(true);
	}
	if (URL[size - 1] == '\n')
		URL[size - 1] = '\0';
	spot_history_list->base_spot_URL = URL;

	// Read the number of entries and current entry index.

	if (fscanf(fp, "%d %d\n", &entries, &curr_entry_index) != 2) {
		fclose(fp);
		return(true);
	}

	// Read each history entry.  If a read error occurs at any point,
	// ignore the rest of the file.

	for (int index = 0; index < entries; index++) {
		viewpoint player_viewpoint;

		// Read the history entry text.

		if (fgets(text, BUFSIZ, fp) == NULL ||
			(size = strlen(text)) == 0)
			break;
		if (text[size - 1] == '\n')
			text[size - 1] = '\0';

		// Read the history entry URL.

		if (fgets(URL, BUFSIZ, fp) == NULL ||
			(size = strlen(URL)) == 0)
			break;
		if (URL[size - 1] == '\n')
			URL[size - 1] = '\0';

		// Read the viewpoint position, last position, turn angle and
		// look angle.

		if (fscanf(fp, "%g %g %g %g %g %g %g %g\n",
			&player_viewpoint.position.x, &player_viewpoint.position.y,
			&player_viewpoint.position.z, 
			&player_viewpoint.last_position.x,
			&player_viewpoint.last_position.y,
			&player_viewpoint.last_position.z, 
			&player_viewpoint.turn_angle, &player_viewpoint.look_angle)
			!= 8)
			break;

		// Add the history entry to the spot history list.

		if (!spot_history_list->add_entry(text, URL, &player_viewpoint))
			break;
	}

	// Set the current entry index in the spot history list so long as
	// it's valid; otherwise make the top entry the current one.

	if (curr_entry_index >= 0 && 
		curr_entry_index < spot_history_list->entries)
		spot_history_list->curr_entry_index = curr_entry_index;
	else
		spot_history_list->curr_entry_index = 
			spot_history_list->entries - 1;

	// Close the history file and return.

	fclose(fp);
	return(true);
}

//------------------------------------------------------------------------------
// Global player shutdown.
//------------------------------------------------------------------------------

void
shut_down_player(void)
{
	FILE *fp;

	// Delete the cached blockset list.

	delete_cached_blockset_list();

	// If the spot history list exists and has been initialised...

	if (spot_history_list && strlen(spot_history_list->base_spot_URL) > 0) {
		history_entry *history_entry_ptr;

		// Update the current history entry with the player viewpoint, but only
		// if the player viewpoint had been set.

		if (player_viewpoint_set) {
			history_entry_ptr = spot_history_list->get_current_entry();
			history_entry_ptr->player_viewpoint = player_viewpoint;
		}

		// Open the history file...

		if ((fp = fopen(history_file_path, "w")) != NULL) {
			int index;

			// Write the base spot URL to the history file.

			fprintf(fp, "%s\n", spot_history_list->base_spot_URL);

			// Write the number of entries and current entry index.

			fprintf(fp, "%d %d\n", spot_history_list->entries,
				spot_history_list->curr_entry_index);

			// Write each history entry.

			for (index = 0; index < spot_history_list->entries; index++) {
				history_entry *history_entry_ptr;
				hyperlink *link_ptr;
				viewpoint *viewpoint_ptr;

				// Get pointers to the hyperlink and viewpoint objects.
				
				history_entry_ptr = spot_history_list->get_entry(index);
				link_ptr = &history_entry_ptr->link;
				viewpoint_ptr = &history_entry_ptr->player_viewpoint;

				// Write the history entry's text, URL and viewpoint.

				fprintf(fp, "%s\n%s\n", link_ptr->label, link_ptr->URL);
				fprintf(fp, "%g %g %g %g %g %g %g %g\n",
					viewpoint_ptr->position.x, viewpoint_ptr->position.y,
					viewpoint_ptr->position.z, viewpoint_ptr->last_position.x,
					viewpoint_ptr->last_position.y,
					viewpoint_ptr->last_position.z, viewpoint_ptr->turn_angle,
					viewpoint_ptr->look_angle);
			}
			fclose(fp);
		}
		delete spot_history_list;
	}

	// Delete the blockset list, and clean up the renderer and collision
	// detection code.

	if (blockset_list_ptr)
		delete blockset_list_ptr;
	clean_up_renderer();
	COL_exit();
}

//------------------------------------------------------------------------------
// Player window initialisation.
//------------------------------------------------------------------------------

static bool
init_player_window(void)
{
	float aspect_ratio;
	float half_horz_field_of_view, half_vert_field_of_view;

	// Initialise free span list and the span buffer.

	init_free_span_list();
	span_buffer_ptr = NULL;

	// Compute half of the window dimensions, for convienance.

	half_window_width = (float)window_width * 0.5f;
	half_window_height = (float)window_height * 0.5f;

	// Set the horizontal field of view to 60 degrees if the aspect ratio is
	// wide (with the vertical field of view smaller), otherwise set the
	// vertical field of view to 60 degrees (with the horizontal field of view
	// smaller).
	//
	// XXX -- If the aspect ratio is greater than 1.85, then turn off
	// fast clipping; it doesn't work very well with wide or tall aspect
	// ratios.

	aspect_ratio = (float)window_width / (float)window_height;
	if (aspect_ratio >= 1.0) {
		horz_field_of_view = DEFAULT_FIELD_OF_VIEW;
		vert_field_of_view = DEFAULT_FIELD_OF_VIEW / aspect_ratio;
		fast_clipping = aspect_ratio < 1.85f;
	} else {
		horz_field_of_view = DEFAULT_FIELD_OF_VIEW * aspect_ratio;
		vert_field_of_view = DEFAULT_FIELD_OF_VIEW;
		fast_clipping = aspect_ratio > 0.54f;
	}

	// Compute half the horizontal and vertical fields of view.

	half_horz_field_of_view = horz_field_of_view * 0.5f;
	half_vert_field_of_view = vert_field_of_view * 0.5f;

	// Compute half the viewport dimensions.

	half_viewport_width = (float)tan(RAD(half_horz_field_of_view));
	half_viewport_height = (float)tan(RAD(half_vert_field_of_view));

	// Compute the number of pixels per world unit at z = 1.  Then compute
	// the number of pixels per texel at z = 1 when the texture contains
	// 256x256 texels, and the number of pixels per degree.

	pixels_per_world_unit = half_window_width / half_viewport_width;
	pixels_per_texel = pixels_per_world_unit / TEXELS_PER_UNIT;
	horz_pixels_per_degree = window_width / horz_field_of_view;
	vert_pixels_per_degree = window_height / vert_field_of_view;

	// If hardware acceleration is not enabled, create the span buffer.

	if (!hardware_acceleration) {
		if ((span_buffer_ptr = new span_buffer) == NULL ||
			!span_buffer_ptr->create_buffer(window_height)) {
			display_low_memory_error();
			return(false);
		}
	}

	// Initialise the screen polygon list.

	init_screen_polygon_list();

	// Create the image caches.

	return(create_image_caches());
}

//------------------------------------------------------------------------------
// Player window shutdown.
//------------------------------------------------------------------------------

void
shut_down_player_window(void)
{
	// Delete span buffer and free span list.

	if (span_buffer_ptr)
		delete span_buffer_ptr;
	delete_free_span_list();

	// Delete the screen polygon list.

	delete_screen_polygon_list();

	// Delete the image caches.

	delete_image_caches();

	// Signal the plugin thread that the player window has been shut down.

	player_window_shut_down.send_event(true);
}

//------------------------------------------------------------------------------
// Handle a window mode change.
//------------------------------------------------------------------------------

static bool
handle_window_mode_change(void)
{
	// Pause the spot, shut down the player window, then wait for signal from
	// plugin thread that main window has been recreated.  If it hasn't,
	// exit with a failure status.

	pause_spot();
	shut_down_player_window();
	if (!main_window_created.wait_for_event()) {
		return(false);
	}

	// Initialise player window, resume spot, and signal the plugin thread that
	// the player window has been initialised.  On failure, unload the spot,
	// shut down the player window, and signal the plugin thread of failure.

	if (init_player_window()) {
		player_window_initialised.send_event(true);
		resume_spot();
		return(true);
	} else {
		free_spot_data();
		shut_down_player_window();
		player_window_initialised.send_event(false);
		return(false);
	}
}

//------------------------------------------------------------------------------
// Handle a window resize.
//------------------------------------------------------------------------------

static bool
handle_window_resize(void)
{
	// Shut down the player window, then wait for signal from plugin thread that
	// main window has been resized.

	shut_down_player_window();
	if (!main_window_resized.wait_for_event())
		return(false);

	// Initialise player window, and signal the plugin thread that the player
	// window has been initialised.  On failure, unload the spot, shut down the
	// player window, and signal the plugin thread of failure.

	if (init_player_window()) {
		player_window_initialised.send_event(true);
		return(true);
	} else {
		free_spot_data();
		shut_down_player_window();
		player_window_initialised.send_event(false);
		return(false);
	}
}

//------------------------------------------------------------------------------
// Player thread.
//------------------------------------------------------------------------------

void
player_thread(void *arg_list)
{
	int64 start_cycle;
	int64 sum_cycles;

	// Determine which CPU we are running on.

	identify_processor();

	// If no default visible radius has been set, determine the speed of the
	// processor and set the radius to 18 if it's 266 MHz or above, otherwise
	// set it to 10.

	if (visible_block_radius < 0) {

		// Calculate the speed of the processor.
		
		sum_cycles.complete = 0;
		__asm {
			_emit 0x0f
			_emit 0x31
			mov start_cycle.partial.low, eax
			mov start_cycle.partial.high, edx
		}
		Sleep(1000);
		__asm {
			_emit 0x0f
			_emit 0x31
			sub eax, start_cycle.partial.low
			sbb edx, start_cycle.partial.high
			add sum_cycles.partial.low, eax
			adc sum_cycles.partial.high, edx
		}
		cycles_per_second = (float)sum_cycles.complete;
		diagnose("Processor speed is %g Mhz", cycles_per_second / 1000000.0f);

		// Set the visible block radius based on this.

		if (cycles_per_second >= 266000000)
			visible_block_radius = 18;
		else
			visible_block_radius = 10;
	}

#ifdef PROFILE
	curr_tabs = 0;
#endif

	// Decrease the priority level on this thread, to ensure that the browser
	// and the rest of the system remains responsive.

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

	// Perform global initialisation and signal the plugin thread as to whether
	// it succeeded or failure.  If it failed, the player thread will exit.

	if (init_player())
		player_thread_initialised.send_event(true);
	else {
		player_thread_initialised.send_event(false);
		shut_down_player();
		return;
	}

	// Wait for signal from plugin thread that a new player window is to be
	// initialised.

	while (player_window_init_requested.wait_for_event()) {

		// Wait for signal from plugin thread that main window created succeeded
		// or failed.  If it failed, jump to the bottom of this loop.

		if (!main_window_created.wait_for_event())
			continue;

		// Open player window, and signal plugin thread that it succeeded
		// or failed.  If it failed, unload the spot, shut down the player
		// window, and jump to the bottom of this loop.

		if (init_spot_data() && init_player_window())
			player_window_initialised.send_event(true);
		else {
			player_window_initialised.send_event(false);
			free_spot_data();
			shut_down_player_window();
			continue;
		}

		// Wait for signal from plugin thread that spot URL is ready.  If the
		// URL failed to download, unload the spot, shut down the player window,
		// and jump to the bottom of this loop.

		if (!URL_was_opened.wait_for_event() ||
			!URL_was_downloaded.wait_for_event()) {
			free_spot_data();
			shut_down_player_window();
			continue;
		}

		// Set the current URL and file path.

		start_atomic_operation();
		curr_URL = downloaded_URL;
		curr_file_path = downloaded_file_path;
		end_atomic_operation();

		// Decode the current URL and file path, to remove encoded characters.

		curr_URL = decode_URL(curr_URL);
		curr_file_path = decode_URL(curr_file_path);
		curr_file_path = URL_to_file_path(curr_file_path);

		// If the spot URL comes from the web rather than the local hard drive,
		// check whether a new version of Rover has been installed.

		if (!strnicmp(curr_URL, "http://", 7))
			check_for_new_rover();

		// Process spot URL.  If this fails, shutdown the player window and
		// wait for a signal from the plugin thread as to whether to restart
		// or quit.

		if (!process_spot_URL(true, false, true)) {
			free_spot_data();
			shut_down_player_window();
			continue;
		}

		// Set a flag indicating that the player is running it's event loop.

		start_atomic_operation();
		player_running = true;
		end_atomic_operation();

		// Run the event loop until plugin thread signals the player window to
		// shut down.

		do {

			START_TIMING;

			// If a pause event has been sent by the plugin thread, stop all
			// sounds and wait until either a resume event or player window
			// shutdown event is recieved before continuing (the latter will
			// only occur as the result of an inactive plugin window being
			// activated).

			if (pause_player_thread.event_sent()) {
				bool got_shutdown_request;

				stop_all_sounds();
				while (true) {
					if (resume_player_thread.event_sent()) {
						got_shutdown_request = false;
						break;
					}
					if (player_window_shutdown_requested.event_sent()) {
						got_shutdown_request = true;
						break;
					}
				}
				if (got_shutdown_request)
					break;
			}

			// If a stream is set...

			if (stream_set) {

				// If the stream is ready, update the texture dependencies.

				if (stream_ready()) {
					if (unscaled_video_texture_ptr)
						update_texture_dependancies(unscaled_video_texture_ptr);
					video_texture *video_texture_ptr = 
						scaled_video_texture_list;
					while (video_texture_ptr) {
						update_texture_dependancies(
							video_texture_ptr->texture_ptr);
						video_texture_ptr = 
							video_texture_ptr->next_video_texture_ptr;
					}
				}

				// If a request to download RealPlayer or Windows Media Player
				// has been recieved, launch the approapiate download page.

				if (download_of_rp_requested())
					request_URL(RP_DOWNLOAD_URL, NULL, "_self", false);
				if (download_of_wmp_requested())
					request_URL(WMP_DOWNLOAD_URL, NULL, "_blank", false);
			}

			// Render the next frame.  If this failed, break out of the event
			// loop.

			if (!render_next_frame())
				break;

			// Handle the current download, if there is one.  If an error has
			// occurred, break out of the event loop.

			if ((curr_custom_texture_ptr || curr_custom_wave_ptr) &&
				!handle_current_download())
				break;

			// If a window mode change is requested, handle it.  If the mode
			// change fails, break out of the event loop.

			if (window_mode_change_requested.event_sent() &&
				!handle_window_mode_change())
				break;

			// If a window resize is requested, handle it.  If the resize fails,
			// break out of the event loop.

			if (window_resize_requested.event_sent() && !handle_window_resize())
				break;

			// If a history list entry was selected, handle the exit that it
			// points to.

			if (history_entry_selected.event_sent()) {
				hyperlink *exit_ptr = &selected_history_entry_ptr->link;
				handle_exit(exit_ptr->URL, exit_ptr->target, true,
					return_to_entrance);
			}

			// If we need to check for an update, and there are no custom files
			// being downloaded...

			if (check_for_update && curr_custom_texture_ptr == NULL &&
				curr_custom_wave_ptr == NULL) {
				time_t last_update, this_update;
				FILE *fp;

				// Reset the check for update flag now that we're doing it.

				check_for_update = false;
				
				// If the spot URL is web-based, read the update file to obtain
				// the time of the last update.
				
				if (!strnicmp(curr_spot_URL, "http://", 7)) {
					last_update = 0;
					if ((fp = fopen(update_file_path, "r")) != NULL) {
						fscanf(fp, "%ld", &last_update);
						fclose(fp);
					}

					// If the minimum update period has expired, request 
					// that the version URL be downloaded.

					this_update = time(NULL);
					if (this_update - last_update > MIN_UPDATE_PERIOD) {
						update_in_progress = true;
						update_type = AUTO_UPDATE;
						version_URL = VERSION_URL;
						version_URL_download_requested.send_event(true);

						// Rewrite the update file to reflect the time this 
						// update check occurred.
	
						if ((fp = fopen(update_file_path, "w")) != NULL) {
							fprintf(fp, "%ld", this_update);
							fclose(fp);
						}
					}
				}
			}

			// If a check for update has been requested explicitly by the user,
			// and one is not in progress, request the version URL to be
			// downloaded.

			if (check_for_update_requested.event_sent() && !update_in_progress) {
				update_in_progress = true;
				update_type = USER_REQUESTED_UPDATE;
				version_URL = VERSION_URL;
				version_URL_download_requested.send_event(true);
			}

			// If the plugin thread has signalled that a version URL has been
			// downloaded, determine whether a new update is ready to be
			// downloaded.

			if (version_URL_was_downloaded.event_sent())
				download_new_update();

			// If the plugin thread has signalled that an update URL has been
			// downloaded, try to install it.

			if (update_URL_was_downloaded.event_sent() && install_update())
				break;
			
			END_TIMING("main event loop");

			// If a player window shutdown has not been requested, repeat the
			// event loop.

		} while (!player_window_shutdown_requested.event_sent());

		// If a stream is set, stop the streaming thread.

		if (stream_set)
			stop_streaming_thread();

		// Delete the unscaled video texture, and the scaled video texture list,
		// if they exist.

		if (unscaled_video_texture_ptr)
			DEL(unscaled_video_texture_ptr, texture);
		while (scaled_video_texture_list) {
			video_texture *next_video_texture_ptr =
				scaled_video_texture_list->next_video_texture_ptr;
			DEL(scaled_video_texture_list, video_texture);
			scaled_video_texture_list = next_video_texture_ptr;
		}

		// Set a flag indicating that the player is no longer running it's event
		// loop.

		start_atomic_operation();
		player_running = false;
		end_atomic_operation();

		// Shutdown the player window.

		free_spot_data();
		shut_down_player_window();
	}

	// Shut down the player and exit.

	shut_down_player();
}