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

#include <time.h>

//------------------------------------------------------------------------------
// Global variables.
//------------------------------------------------------------------------------

// Flag indicating whether AMD 3DNow! instructions are supported.

extern int AMD_3DNow_supported;

// Display dimensions and other related information.

extern float half_window_width;
extern float half_window_height;
extern float horz_field_of_view;
extern float vert_field_of_view;
extern float half_viewport_width;
extern float half_viewport_height;
extern float pixels_per_world_unit;
extern float pixels_per_texel;
extern float horz_pixels_per_degree;
extern float vert_pixels_per_degree;
extern bool fast_clipping;

// Debug, warnings and low memory flags.

extern bool debug_on;
extern bool warnings;
extern bool low_memory;

// Cached blockset list.

extern cached_blockset *cached_blockset_list;
extern cached_blockset *last_cached_blockset_ptr;

// Spot history list.

extern history *spot_history_list;

// URL directory of currently open spot, and current spot URL without the
// entrance name.

extern string spot_URL_dir;
extern string curr_spot_URL;

// Minimum Rover version required for the current spot.

extern int min_rover_version;

// Flag indicating if we've seen the first spot URL yet or not.

extern bool seen_first_spot_URL;

// Spot title.

extern string spot_title;

// Trigonometry tables.

extern sine_table sine;
extern cosine_table cosine;

// Visible radius, and frustum vertex list (in view space).

extern float visible_radius;
extern vertex frustum_vertex_list[FRUSTUM_VERTICES];

// Frame buffer pointer and width.

extern byte *frame_buffer_ptr;
extern int frame_buffer_width;

// Span buffer.

extern span_buffer *span_buffer_ptr;

// Pointer to old and current blockset list, and custom blockset.

extern blockset_list *old_blockset_list_ptr;
extern blockset_list *blockset_list_ptr;
extern blockset *custom_blockset_ptr;

// Symbol to block definition mapping table.

extern block_def *symbol_table[16384];

// Pointer to world object and movable block list.

extern world *world_ptr;
extern block *movable_block_list;

// Onload exit.

extern hyperlink *onload_exit_ptr;

// Global list of entrances.

extern entrance *global_entrance_list;

// List of imagemaps.

extern imagemap *imagemap_list;

// Global list of lights.

extern int global_lights;
extern light *global_light_list;

// Orb light, texture, brightness, size and exit.

extern light *orb_light_ptr;
extern texture *orb_texture_ptr;
extern texture *custom_orb_texture_ptr;
extern float orb_brightness;
extern int orb_brightness_index;
extern float orb_width, orb_height;
extern float half_orb_width, half_orb_height;
extern hyperlink *orb_exit_ptr;

// Global sound list, ambient sound, audio radius, reflection radius, and a flag
// indicating if the global sound list has been changed.

extern sound *global_sound_list;
extern sound *ambient_sound_ptr;
extern float audio_radius;
extern float reflection_radius;
extern bool global_sound_list_changed;

// Streaming sound.

extern sound *streaming_sound_ptr;

// Original list of popups, and global list of popups.

extern popup *original_popup_list;
extern popup *global_popup_list;
extern popup *last_global_popup_ptr;

// Global list of triggers.

extern trigger *global_trigger_list;
extern trigger *last_global_trigger_ptr;

// Placeholder texture.

extern texture *placeholder_texture_ptr;

// Sky definition.

extern texture *sky_texture_ptr;
extern texture *custom_sky_texture_ptr;
extern RGBcolour sky_colour;
extern pixel sky_colour_pixel;
extern float sky_brightness;
extern int sky_brightness_index;

// Ground block definition.

extern block_def *ground_block_def_ptr;

// Player block symbol and pointer, camera offset, collision radius and 
// viewpoint.

extern word player_block_symbol;
extern block *player_block_ptr;
extern vector player_camera_offset;
extern viewpoint player_viewpoint;

// Player's size, collision box, and step height.

extern vertex player_size;
extern COL_AABOX player_collision_box;
extern float player_step_height;

// Start and current time, and number of frames and total polygons rendered.

extern time_t start_time;
extern int start_time_ms, curr_time_ms;
extern int frames_rendered, total_polygons_rendered;

// Flag to turn the renderer on or off.

extern bool renderer_on;

// Current mouse position.

extern int mouse_x;
extern int mouse_y;

// Number of currently selected polygon, and pointer to the block definition
// it belongs to.

extern int curr_selected_polygon_no;
extern block_def *curr_selected_block_def_ptr;

// Previously and currently selected square.

extern square *prev_selected_square_ptr;
extern square *curr_selected_square_ptr;

// Previously and currently selected area.

extern hyperlink *prev_selected_exit_ptr;
extern hyperlink *curr_selected_exit_ptr;

// Previously and currently selected area and it's square.

extern area *prev_selected_area_ptr;
extern square *prev_area_square_ptr;
extern area *curr_selected_area_ptr;
extern square *curr_area_square_ptr;

// Current popup square.

extern square *curr_popup_square_ptr;

// Current URL and file path.

extern string curr_URL;
extern string curr_file_path;

// Flags indicating if the viewpoint has changed, and if so the current
// direction of movement.

extern bool viewpoint_has_changed;
extern bool forward_movement;

// Current custom texture and wave.

extern texture *curr_custom_texture_ptr;
extern wave *curr_custom_wave_ptr;

// Time that logo was last updated.

extern int last_logo_time_ms;

// Stream flag, name, and URLs for RealPlayer and Windows Media Player.

extern bool stream_set;
extern string name_of_stream;
extern string rp_stream_URL;
extern string wmp_stream_URL;

// Pointer to unscaled video texture (for popups) and scaled video texture
// list.

extern texture *unscaled_video_texture_ptr;
extern video_texture *scaled_video_texture_list;

//------------------------------------------------------------------------------
// Externally visible functions.
//------------------------------------------------------------------------------

// Angle functions.

float
neg_adjust_angle(float angle);

float
pos_adjust_angle(float angle);

float
clamp_angle(float angle, float min_angle, float max_angle);

// Main entry point to player thread.

void
player_thread(void *arg_list);
