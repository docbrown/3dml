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

// Render modes.

#define	SOFTWARE			1
#define HARDWARE_WINDOW		2
#define HARDWARE_FULLSCREEN	4

// Sound systems.

#define NO_SOUND		0
#define	DIRECT_SOUND	1
#define A3D_SOUND		2

// Option window control IDs.

#define OK_BUTTON						0
#define	CANCEL_BUTTON					1
#define CHECK_FOR_UPDATE_BUTTON			2
#define A3D_WEB_SITE_BUTTON				3
#define VIEW_SOURCE_BUTTON				4
#define DOWNLOAD_SOUNDS_CHECKBOX		5
#define ENABLE_REFLECTIONS_CHECKBOX		6
#define ENABLE_3D_ACCELERATION_CHECKBOX	7
#define	VISIBLE_RADIUS_EDITBOX			8

// Key codes for non-alphabetical keys (ASCII codes used for 'A' to 'Z').

#define SHIFT_KEY		-1
#define CONTROL_KEY		-2
#define UP_KEY			-3
#define DOWN_KEY		-4
#define LEFT_KEY		-5
#define RIGHT_KEY		-6
#define ADD_KEY			-7
#define SUBTRACT_KEY	-8
#define INSERT_KEY		-9
#define DELETE_KEY		-10
#define PAGEUP_KEY		-11
#define PAGEDOWN_KEY	-12

// Button codes (not all platforms support a two-button mouse, so a keyboard
// modifier may be used as a substitute for the right button).

#define MOUSE_MOVE_ONLY		0
#define LEFT_BUTTON_DOWN	1
#define LEFT_BUTTON_UP		2
#define RIGHT_BUTTON_DOWN	3
#define RIGHT_BUTTON_UP		4

// Task bar button codes.

#define NO_TASK_BAR_BUTTON	0
#define LOGO_BUTTON			1
#define HISTORY_BUTTON		2
#define OPTIONS_BUTTON		3
#define LIGHT_BUTTON		4
#define BUILDER_BUTTON		5

// Media players supported.

#define ANY_PLAYER				0
#define REAL_PLAYER				1
#define WINDOWS_MEDIA_PLAYER	2

//------------------------------------------------------------------------------
// Event class.
//------------------------------------------------------------------------------

struct event {
	void *event_handle;
	bool event_value;

	event();
	~event();
	void create_event(void);			// Create event.
	void destroy_event(void);			// Destroy event.
	void send_event(bool value);		// Send event.
	void reset_event(void);				// Reset event.
	bool event_sent(void);				// Check if event was sent.
	bool wait_for_event(void);			// Wait for event.
};

//------------------------------------------------------------------------------
// Global variables.
//------------------------------------------------------------------------------

// Operating system name and version.

extern string os_version;

// Application path.

extern string app_dir;

// Display, texture and video pixel formats.

extern pixel_format display_pixel_format;
extern pixel_format texture_pixel_format;

// Display properties.

extern int display_width, display_height, display_depth;
extern int window_width, window_height;
extern int render_mode;

// Flag indicating whether the main window is ready.

extern bool main_window_ready;

// Flag indicating whether sound is enabled, the sound system being used,
// and whether reflections are available (for A3D only).

extern bool sound_on;
extern int sound_system;
extern bool reflections_available;

//------------------------------------------------------------------------------
// Externally visible functions.
//------------------------------------------------------------------------------

// Thread synchronisation functions.

void
start_atomic_operation(void);

void
end_atomic_operation(void);

// Plugin window functions (called by the plugin thread only).

void
set_plugin_window(void *window_handle, void *window_data_ptr, 
				  const char *window_text,
				  void (*window_callback)(void *window_data_ptr));

void
restore_plugin_window(void *window_handle);

// Start up/shut down functions (called by the plugin thread only).

bool
start_up_platform_API(void);

void
shut_down_platform_API(void);

int
get_process_ID(void);

bool
start_player_thread(void);

void
wait_for_player_thread_termination(void);

// Main window functions (called by the plugin thread only).

void
set_main_window_size(int width, int height);

bool
create_main_window(void *window_handle, int width, int height,
				   void (*key_callback)(int key_code, bool key_down),
				   void (*mouse_callback)(int x, int y, int button_code,
										  int task_bar_button_code),
				   void (*timer_callback)(void),
   				   void (*resize_callback)(void *window_handle, int width,
										   int height));

void
destroy_main_window(void);

// Browser window function (called by the plugin thread only).

bool
browser_window_is_minimised(void);

// Message functions.

void
fatal_error(char *title, char *format, ...);

void
information(char *title, char *format, ...);

bool
query(char *title, bool yes_no_format, char *format, ...);

// Progress window functions (called by the plugin thread only).

void
open_progress_window(int file_size, void (*progress_callback)(void),
					 char *format, ...);

void
update_progress_window(int file_pos);

void
close_progress_window(void);

// Light window functions (called by the plugin thread only).

void
open_light_window(float brightness, void (*light_callback)(float brightness));

void
close_light_window(void);

// Options window functions (called by the plugin thread only).

void
open_options_window(bool download_sounds_value, bool reflections_enabled_value,
					int visible_radius_value,
					void (*options_callback)(int option_ID, int option_value));

void
close_options_window(void);

// History menu functions (called by the plugin thread only).

void
open_history_menu(history *history_list);

int
track_history_menu(void);

void
close_history_menu(void);

// Password window function (called by the stream thread only).

bool
get_password(string *username_ptr, string *password_ptr);

// Frame buffer functions (called by the player thread only).

bool
create_frame_buffer(void);

void
destroy_frame_buffer(void);

bool
lock_frame_buffer(byte *&frame_buffer_ptr, int &frame_buffer_width);

void
unlock_frame_buffer(void);

bool
display_frame_buffer(bool loading_spot, bool show_splash_graphic);

void
clear_frame_buffer(int x, int y, int width, int height);

// Software rendering functions (called by the player thread only).

void
create_lit_image(cache_entry *cache_entry_ptr, int image_dimensions);

void
render_colour_span16(span *span_ptr);

void
render_colour_span24(span *span_ptr);

void
render_colour_span32(span *span_ptr);

void
render_opaque_span16(span *span_ptr);

void
render_opaque_span24(span *span_ptr);

void
render_opaque_span32(span *span_ptr);

void
render_transparent_span16(span *span_ptr);

void
render_transparent_span24(span *span_ptr);

void
render_transparent_span32(span *span_ptr);

void
render_popup_span16(span *span_ptr);

void
render_popup_span24(span *span_ptr);

void
render_popup_span32(span *span_ptr);


// Hardware rendering functions (called by the player thread only).

void
hardware_init_vertex_list(void);

bool
hardware_create_vertex_list(int max_vertices);

void
hardware_destroy_vertex_list(int max_vertices);

void *
hardware_create_texture(int image_size_index);

void
hardware_destroy_texture(void *hardware_texture_ptr);

void
hardware_set_texture(cache_entry *cache_entry_ptr);

void
hardware_render_2D_polygon(pixmap *pixmap_ptr, RGBcolour colour, 
						   float brightness, float x, float y, float width,
						   float height, float start_u, float start_v, 
						   float end_u, float end_v, bool disable_transparency);

void
hardware_render_polygon(spolygon *spolygon_ptr);

// Pixmap drawing function (called by the player thread only).

void
draw_pixmap(pixmap *pixmap_ptr, int brightness_index, int x, int y);

// Colour functions (called by the player thread only).

pixel
RGB_to_display_pixel(RGBcolour colour);

pixel
RGB_to_texture_pixel(RGBcolour colour);

RGBcolour *
get_standard_RGB_palette(void);

byte
get_standard_palette_index(RGBcolour colour_ptr);

// Task bar function (called by the player thread only).

void
set_title(char *format, ...);

// URL functions (called by the player thread only).

void
show_label(const char *label);

void
hide_label(void);

// Popup function (called by the player thread only).

void
init_popup(popup *popup_ptr);

// Cursor and mouse functions (called by the plugin thread only).

void
get_mouse_position(int *x, int *y, bool relative);

void
set_movement_cursor(arrow movement_arrow);

void
set_crosshair_cursor(void);

void
set_hand_cursor(void);

void
set_arrow_cursor(void);

void
capture_mouse(void);

void
release_mouse(void);

// Time function.

int
get_time_ms(void);

// Functions to load wave files.

bool
load_wave_data(wave *wave_ptr, char *wave_file_buffer, int wave_file_size);

bool
load_wave_file(wave *wave_ptr, const char *wave_URL, const char *wave_file_path);

// Functions to create and destroy sound buffers (called by the player thread 
// and stream thread).

void
update_sound_buffer(void *sound_buffer_ptr, char *data_ptr, int data_size,
					int data_start);

bool
create_sound_buffer(sound *sound_ptr);

void
destroy_sound_buffer(sound *sound_ptr);

#ifdef SUPPORT_A3D

// Functions to create and destroy audio blocks (called by the player
// thread only.)

void
reset_audio(void);

void *
create_audio_block(int min_column, int min_row, int min_level, 
				   int max_column, int max_row, int max_level);

void
destroy_audio_block(void *audio_block_ptr);

#endif

// Functions to control playing of sounds (called by the player thread only).

void
set_sound_volume(sound *sound_ptr, float volume);

void
play_sound(sound *sound_ptr, bool looped);

void
stop_sound(sound *sound_ptr);

void
begin_sound_update(void);

void
update_sound(sound *sound_ptr);

void
end_sound_update(void);

// Functions to control playing of streaming media.

void
init_video_textures(int video_width, int video_height, int pixel_format);

void
draw_frame(byte *image_ptr);

bool
stream_ready(void);

bool
download_of_rp_requested(void);

bool
download_of_wmp_requested(void);

//void
//send_stream_command(int command);

void
start_streaming_thread(void);

void
stop_streaming_thread(void);