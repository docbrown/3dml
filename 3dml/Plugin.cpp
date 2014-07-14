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
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <direct.h>
#include "Plugin\npapi.h"
#include "Classes.h"
#include "Image.h"
#include "Main.h"
#include "Memory.h"
#include "Parser.h"
#include "Platform.h"
#include "Plugin.h"
#include "resource.h"

// Acceleration modes.

#define TRY_HARDWARE				0
#define TRY_SOFTWARE				1

// Minimum, maximum and default move rates (world units per second), and the
// delta move rate.

#define MIN_MOVE_RATE				3.2f
#define MAX_MOVE_RATE				12.8f
#define DEFAULT_MOVE_RATE			9.6f
#define DELTA_MOVE_RATE				0.8f

// Minimum, maximum and default rotation rates (degrees per second), and the
// delta rotation rate.

#define MIN_ROTATE_RATE				30.0f
#define MAX_ROTATE_RATE				60.0f
#define DEFAULT_ROTATE_RATE			90.0f
#define DELTA_ROTATE_RATE			5.0f

// Structure used to hold a plugin instance's data.

struct InstData {
	bool		active_instance;	// TRUE if this is the active instance.
	NPP			instance_ptr;		// Pointer to instance.
	NPWindow	*window_ptr;		// Pointer to window.
	string		spot_URL;			// Original spot URL.
	string		window_text;		// Inactive window text.

	InstData();
	~InstData();
};

// Pointer to active plugin instance's data.

static InstData *active_inst_data_ptr;

// Important directories and file paths.

string flatland_dir;
string log_file_path;
string error_log_file_path;
string config_file_path;
string version_file_path;
string update_file_path;
string update_archive_path;
string update_HTML_path;
string new_plugin_DLL_path;
string updater_path;
string history_file_path;
string curr_spot_file_path;
string cache_file_path;
string javascript_file_path;
string new_rover_file_path;

// Miscellaneous global variables.

bool same_browser_session;
bool hardware_acceleration;
bool full_screen;
int prev_window_width, prev_window_height;

// Variables used to request a URL.

string requested_URL;
string requested_target;
string requested_file_path;
string requested_blockset_name;			// Set if requesting a blockset.
string requested_rover_version;			// Set if requesting a Rover update.
static NPStream *requested_stream_ptr;

// Downloaded URL and file path.

string downloaded_URL;
string downloaded_file_path;

// Update URL and stream pointer.

string update_URL;
static NPStream *update_stream_ptr;

// Version URL and stream pointer.

string version_URL;
static NPStream *version_stream_ptr;

// Javascript URL (no stream pointer required, since we throw the data away).

string javascript_URL;

// Web browser ID and version.

int web_browser_ID;
string web_browser_version;

// Events sent by player thread.

event player_thread_initialised;
event player_window_initialised;
event URL_download_requested;
event version_URL_download_requested;
event update_URL_download_requested;
event javascript_URL_download_requested;
event player_window_shut_down;
event display_error_log;

// Events sent by plugin thread.

event main_window_created;
event main_window_resized;
event URL_was_opened;
event URL_was_downloaded;
event version_URL_was_downloaded;
event update_URL_was_downloaded;
event window_mode_change_requested;
event window_resize_requested;
event mouse_clicked;
event player_window_shutdown_requested;
event player_window_init_requested;
event pause_player_thread;
event resume_player_thread;
event history_entry_selected;
event check_for_update_requested;
event polygon_info_requested;

// Global variables that require synchronised access.

bool player_running;
bool selection_active;
bool absolute_motion;
int curr_mouse_x;
int curr_mouse_y;
float curr_move_delta;
float curr_side_delta;
float curr_move_rate;
float curr_turn_delta;
float curr_look_delta;
float curr_rotate_rate;
float master_brightness;
bool download_sounds;
bool reflections_enabled;
int visible_block_radius;
history_entry *selected_history_entry_ptr;
bool return_to_entrance;
bool URL_request_pending;

// Size of movement border in window.

#define MOVEMENT_BORDER	32

// Mouse cursor variables.

static int last_x, last_y;
static arrow movement_arrow;
static bool selection_active_flag;
static bool inside_3D_window;

// Miscellaneous local variables.

static int acceleration_mode;
static bool player_active;
static bool player_window_created;
static bool browser_window_minimised;
static float prev_move_rate, prev_rotate_rate;
static bool shift_key_down;
static bool ctrl_key_down;
static int curr_button_status;
static bool movement_enabled;
static bool download_sounds_flag;
static bool reflections_enabled_flag;
static bool hardware_acceleration_flag;
static int old_visible_block_radius;
static int curr_process_ID;
static float window_half_width;
static float window_top_half_height, window_bottom_half_height;

// Various URLs.

#define FLATLAND_URL "http://www.flatland.com"
#define BUILDER_URL	"http://www.flatland.com/build_direct.html"
#define REGISTRATION_URL "http://www.flatland.com/registration_direct.html"
#define A3D_URL "http://www.a3d.com/flatland"

//==============================================================================
// InstData class.
//==============================================================================

// Default constructor initialises all fields.

InstData::InstData()
{
	active_instance = false;
	instance_ptr = NULL;
	window_ptr = NULL;
}

// Default destructor does nothing.

InstData::~InstData()
{
}

//==============================================================================
// Player window functions.
//==============================================================================

// Forward declaration of callback functions.

static void
key_event_callback(int key_code, bool key_down);

static void
timer_event_callback(void);

static void
mouse_event_callback(int x, int y, int button_code, int task_bar_button_code);

static void
resize_event_callback(void *window_handle, int width, int height);

//------------------------------------------------------------------------------
// Create and the player window.
//------------------------------------------------------------------------------

static bool
create_player_window(InstData *inst_data_ptr)
{
	NPWindow *window_ptr;
	HWND window_handle;
	RECT window_rect;

	// Determine the size of the window.  We don't rely on the size given in
	// the NPWindow structure because Internet Explorer may fail to set it.

	window_ptr = inst_data_ptr->window_ptr;
	window_handle = (HWND)window_ptr->window;
	GetClientRect(window_handle, &window_rect);

	// Create the main window.  If this fails, signal the player thread of
	// failure.

	if (!create_main_window(window_handle, window_rect.right,
		window_rect.bottom, key_event_callback, mouse_event_callback,
		timer_event_callback, resize_event_callback)) {
		destroy_main_window();
		main_window_created.send_event(false);
		return(false);
	}

	// Otherwise...

	else {
	
		// Compute the window "centre" coordinates (the point around which the
		// movement arrows are determined).

		window_half_width = window_width * 0.5f;
		window_top_half_height = window_height * 0.75f;
		window_bottom_half_height = window_height * 0.25f;

		// Signal the player thread that the main window was created,
		// and wait for the player thread to signal that the player window was
		// initialised.  If this failed, wait for the player thread to signal
		// that the player window was shut down, then destroy the main window.

		main_window_created.send_event(true);
		if (!player_window_initialised.wait_for_event()) {
			player_window_shut_down.wait_for_event();
			destroy_main_window();
			return(false);
		}
		player_window_created = true;
		return(true);
	}
}

//------------------------------------------------------------------------------
// Destroy the player window.
//------------------------------------------------------------------------------

static void
destroy_player_window(void)
{
	// Send the sequence of signals required to ensure that the player thread
	// has shutdown the player window, and wait for the player thread to confirm
	// this has happened.

	main_window_created.send_event(false);
	start_atomic_operation();
	downloaded_URL = "";
	end_atomic_operation();
	URL_was_downloaded.send_event(false);
	player_window_shutdown_requested.send_event(true);
	player_window_shut_down.wait_for_event();

	// Destroy the main window, then reset all signals in case the player thread
	// didn't see some of them.

	destroy_main_window();
	main_window_created.reset_event();
	URL_was_downloaded.reset_event();
	player_window_shutdown_requested.reset_event();
	player_window_created = false;
}

//------------------------------------------------------------------------------
// Function to toggle the window mode between accelerated and non-accelerated
// graphics.
//------------------------------------------------------------------------------

static void
toggle_window_mode(void)
{
	// Signal the player thread that a window mode change is requested, and
	// wait for the player thread to signal that the player window has shut
	// down.

	window_mode_change_requested.send_event(true);
	player_window_shut_down.wait_for_event();

	// Destroy the main window.

	destroy_main_window();

	// Toggle the hardware acceleration mode.

	hardware_acceleration = !hardware_acceleration;

	// Create the player window.  If this fails, make the active instance
	// inactive.

	if (!create_player_window(active_inst_data_ptr)) {
		active_inst_data_ptr->active_instance = false;
		active_inst_data_ptr = NULL;
	}
}

//==============================================================================
// Miscellaneous functions.
//==============================================================================

//------------------------------------------------------------------------------
// Display a file as a web page in a new browser window.
//------------------------------------------------------------------------------

static void
display_file_as_web_page(const char *file_path)
{
	FILE *fp;
	string URL;

	// If there is no file, there is nothing to display.

	if ((fp = fopen(file_path, "r")) == NULL)
		return;
	fclose(fp);

	// Construct the URL to the file.

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
	URL += file_path;
	switch (web_browser_ID) {
	case NAVIGATOR:
		if (URL[9] == ':')
			URL[9] = '|';
		URL = encode_URL(URL);
	}

	// Ask the browser to load this URL into a new window.

	NPN_GetURL(active_inst_data_ptr->instance_ptr, URL, "_blank");
}

//------------------------------------------------------------------------------
// Update the mouse cursor.
//------------------------------------------------------------------------------

static void
update_mouse_cursor(int x, int y)
{
	float delta_x, delta_y, angle;

	// Set the current mouse position, and get the value of the selection active
	// flag.

	start_atomic_operation();
	curr_mouse_x = x;
	curr_mouse_y = y;
	selection_active_flag = selection_active;
	end_atomic_operation();

	// Determine the current movement arrow based upon the angle between the
	// "centre" of the main window and the current mouse position.

	delta_x = ((float)x - window_half_width) / window_half_width;
	delta_y = window_top_half_height - (float)y;
	if (FGT(delta_y, 0.0f))
		delta_y /= window_top_half_height;
	else
		delta_y /= window_bottom_half_height;
	if (FEQ(delta_x, 0.0f)) {
		if (FGT(delta_y, 0.0f))
			angle = 0.0f;
		else
			angle = 180.0f;
	} else {
		angle = (float)DEG(atan(delta_y / delta_x));
		if (FGT(delta_x, 0.0f))
			angle = 90.0f - angle;
		else
			angle = 270.0f - angle;
	}
	if (FGE(angle, 325.0f) || FLT(angle, 35.0f))
		movement_arrow = ARROW_N;
	else if (FLT(angle, 50.0f))
		movement_arrow = ARROW_NE;
	else if (FLT(angle, 125.0f))
		movement_arrow = ARROW_E;
	else if (FLT(angle, 140.0f))
		movement_arrow = ARROW_SE;
	else if (FLT(angle, 220.0f))
		movement_arrow = ARROW_S;
	else if (FLT(angle, 235.0f))
		movement_arrow = ARROW_SW;
	else if (FLT(angle, 310.0f))
		movement_arrow = ARROW_W;
	else
		movement_arrow = ARROW_NW;

	// Determine whether or not the mouse is inside the 3D window.

	inside_3D_window = x >= 0 && x < window_width && y >= 0 &&
		y < window_height;
}

//------------------------------------------------------------------------------
// Set the mouse cursor.
//------------------------------------------------------------------------------

static void
set_mouse_cursor(void)
{
	// Otherwise...

	switch (curr_button_status) {

	// If both buttons are up, set the cursor according to what the mouse is
	// pointing at.

	case MOUSE_MOVE_ONLY:
		if (!inside_3D_window)
			set_arrow_cursor();
		else if (selection_active_flag)
			set_hand_cursor();
		else 
			set_movement_cursor(movement_arrow);
		break;

	// If the left moust button is down, set the mouse cursor according to where
	// it is pointing and whether movement is enabled.

	case LEFT_BUTTON_DOWN:
		if (!inside_3D_window)
			set_arrow_cursor();
		else if (selection_active_flag && !movement_enabled)
			set_hand_cursor();
		else
			set_movement_cursor(movement_arrow);
	}
}

//------------------------------------------------------------------------------
// Activate the history menu.
//------------------------------------------------------------------------------

static void
select_history_entry(int index)
{
	// If the user selected a history entry and it's not the current entry,
	// signal the player thread.

	start_atomic_operation();
	if (index > 0 && index != spot_history_list->curr_entry_index + 2) {
		spot_history_list->prev_entry_index =
			spot_history_list->curr_entry_index;
		if (index == 1)
			return_to_entrance = true;
		else {
			return_to_entrance = false;
			spot_history_list->curr_entry_index = index - 2;
		}
		selected_history_entry_ptr = spot_history_list->get_current_entry();
		history_entry_selected.send_event(true);
	}
	end_atomic_operation();
}

//------------------------------------------------------------------------------
// Launch the builder web page.
//------------------------------------------------------------------------------

void
launch_builder_web_page(void)
{
	NPN_GetURL(active_inst_data_ptr->instance_ptr, BUILDER_URL, "_blank");
}

//------------------------------------------------------------------------------
// Update the move, look and turn rates.
//------------------------------------------------------------------------------

static void
update_motion_rates(int rate_dir)
{
	start_atomic_operation();
	curr_move_rate += rate_dir * DELTA_MOVE_RATE;
	if (curr_move_rate < MIN_MOVE_RATE)
		curr_move_rate = MIN_MOVE_RATE;
	else if (curr_move_rate > MAX_MOVE_RATE)
		curr_move_rate = MAX_MOVE_RATE;
	curr_rotate_rate += rate_dir * DELTA_ROTATE_RATE;
	if (curr_rotate_rate < MIN_ROTATE_RATE)
		curr_rotate_rate = MIN_ROTATE_RATE;
	else if (curr_rotate_rate > MAX_ROTATE_RATE)
		curr_rotate_rate = MAX_ROTATE_RATE;
	end_atomic_operation();
}

//------------------------------------------------------------------------------
// Activate the options window.
//------------------------------------------------------------------------------

static void options_window_callback(int option_ID, int option_value);

static void
activate_options_window(void)
{
	start_atomic_operation();
	download_sounds_flag = download_sounds;
	reflections_enabled_flag = reflections_enabled;
	hardware_acceleration_flag = hardware_acceleration;
	old_visible_block_radius = visible_block_radius;
	end_atomic_operation();
	open_options_window(download_sounds_flag, reflections_enabled_flag,
		old_visible_block_radius, options_window_callback);
}

//==============================================================================
// Callback functions.
//==============================================================================

//------------------------------------------------------------------------------
// Light window callback function.
//------------------------------------------------------------------------------

static void
light_window_callback(float brightness)
{
	start_atomic_operation();
	master_brightness = -brightness / 100.0f;
	end_atomic_operation();
}

//------------------------------------------------------------------------------
// Options window callback function.
//------------------------------------------------------------------------------

static void
options_window_callback(int option_ID, int option_value)
{
	switch (option_ID) {
	case OK_BUTTON:
		start_atomic_operation();
		download_sounds = download_sounds_flag;
		reflections_enabled = reflections_enabled_flag;
		end_atomic_operation();
		close_options_window();
		if (hardware_acceleration_flag != hardware_acceleration &&
			render_mode != SOFTWARE)
			toggle_window_mode();
		break;
	case CANCEL_BUTTON:
		start_atomic_operation();
		visible_block_radius = old_visible_block_radius;
		end_atomic_operation();
		close_options_window();
		break;
	case DOWNLOAD_SOUNDS_CHECKBOX:
		download_sounds_flag = option_value ? true : false;
		break;
#ifdef SUPPORT_A3D
	case ENABLE_REFLECTIONS_CHECKBOX:
		reflections_enabled_flag = option_value ? true : false;
		break;
#endif
	case ENABLE_3D_ACCELERATION_CHECKBOX:
		hardware_acceleration_flag = option_value ? true : false;
		break;
	case VISIBLE_RADIUS_EDITBOX:
		start_atomic_operation();
		visible_block_radius = option_value;
		end_atomic_operation();
		break;
	case CHECK_FOR_UPDATE_BUTTON:
		check_for_update_requested.send_event(true);
		break;
#ifdef SUPPORT_A3D
	case A3D_WEB_SITE_BUTTON:
		NPN_GetURL(active_inst_data_ptr->instance_ptr, A3D_URL, "_blank");
		break;
#endif
	case VIEW_SOURCE_BUTTON:
		display_file_as_web_page(curr_spot_file_path);
	}
}

//------------------------------------------------------------------------------
// Callback function to handle key events.
//------------------------------------------------------------------------------

static void
key_event_callback(int key_code, bool key_down)
{
	bool player_running_flag;
	float brightness;

	// If the player is not currently running, don't process any mouse events.

	start_atomic_operation();
	player_running_flag = player_running;
	end_atomic_operation();
	if (!player_running_flag)
		return;

	// Handle the key code...

	switch (key_code) {

	// If the shift key has been pressed or released, note this, and toggle
	// between rotating or moving sideways if such movement is already in
	// progress.

	case SHIFT_KEY:
		shift_key_down = key_down;
		if (FNE(curr_side_delta, 0.0f)) {
			curr_turn_delta = curr_side_delta;
			curr_side_delta = 0.0f;
		} else if (FNE(curr_turn_delta, 0.0f)) {
			curr_side_delta = curr_turn_delta;
			curr_turn_delta = 0.0f;
		}
		break;

	// If the control key has been pressed or released, set or reset the current
	// move and rotate rates.

	case CONTROL_KEY:
		start_atomic_operation();
		if (key_down && !ctrl_key_down) {
			ctrl_key_down = TRUE;
			prev_move_rate = curr_move_rate;
			prev_rotate_rate = curr_rotate_rate;
			curr_move_rate = MAX_MOVE_RATE;
			curr_rotate_rate = MAX_ROTATE_RATE;
		} 
		if (!key_down && ctrl_key_down) {
			ctrl_key_down = FALSE;
			curr_move_rate = prev_move_rate;
			curr_rotate_rate = prev_rotate_rate;
		}
		end_atomic_operation();
		break;

	// If 'T' was released, and the render mode supports hardware
	// acceleration, toggle the window mode.

	case 'T':
		if (!key_down && render_mode != SOFTWARE)
			toggle_window_mode();
		break;

	// Up key starts or stops forward motion.

	case UP_KEY:
		start_atomic_operation();
		if (key_down)
			curr_move_delta = 1.0f;
		else
			curr_move_delta = 0.0f;
		end_atomic_operation();
		break;

	// Down key starts or stops backward motion.

	case DOWN_KEY:
		start_atomic_operation();
		if (key_down)
			curr_move_delta = -1.0f;
		else
			curr_move_delta = 0.0f;
		end_atomic_operation();
		break;

	// Left key starts or stops anti-clockwise turn, or left sidle if SHIFT key
	// is also down.

	case LEFT_KEY:
		start_atomic_operation();
		if (key_down) {
			if (shift_key_down) {
				curr_side_delta = -1.0f;
				curr_turn_delta = 0.0f;
			} else {
				curr_side_delta = 0.0f;
				curr_turn_delta = -1.0f;
			}
		} else {
			curr_side_delta = 0.0f;
			curr_turn_delta = 0.0f;
		}
		end_atomic_operation();
		break;

	// Right key starts or stops clockwise turn, or right sidle if SHIFT key is
	// down.

	case RIGHT_KEY:
		start_atomic_operation();
		if (key_down) {
			if (shift_key_down) {
				curr_side_delta = 1.0f;
				curr_turn_delta = 0.0f;
			} else {
				curr_side_delta = 0.0f;
				curr_turn_delta = 1.0f;
			}
		} else {
			curr_side_delta = 0.0f;
			curr_turn_delta = 0.0f;
		}
		end_atomic_operation();
		break;

	// 'A' key starts or stops upward look.

	case 'A':
		start_atomic_operation();
		if (key_down)
			curr_look_delta = -1.0f;
		else
			curr_look_delta = 0.0f;
		end_atomic_operation();
		break;

	// 'Z' and 'Y' keys starts downward look (the latter is required because
	// German keyboards have the 'Y' and 'Z' keys physically swapped).

	case 'Z':
	case 'Y':
		start_atomic_operation();
		if (key_down)
			curr_look_delta = 1.0f;
		else
			curr_look_delta = 0.0f;
		end_atomic_operation();
		break;

	// If the subtract key was released, decrease the rate of motion.

	case SUBTRACT_KEY:
		if (!key_down)
			update_motion_rates(RATE_SLOWER);
		break;

	// If the add key was released, increase the rate of motion.

	case ADD_KEY:
		if (!key_down)
			update_motion_rates(RATE_FASTER);
		break;

	// If the 'B' key was released and we're not running full-screen, launch
	// the builder's guide web page.

	case 'B':
		if (!key_down && !full_screen)
			launch_builder_web_page();
		break;

	// If the 'H' key was released and we're not running full-screen, activate
	// the history menu.

	case 'H':
		if (!key_down && !full_screen) {
			start_atomic_operation();
			open_history_menu(spot_history_list);
			end_atomic_operation();
			select_history_entry(track_history_menu());
			close_history_menu();
		}
		break;

	// If the 'L' key was released and we're not running full-screen, activate
	// the light control.

	case 'L':
		if (!key_down && !full_screen) {
			start_atomic_operation();
			brightness = master_brightness;
			end_atomic_operation();
			open_light_window(brightness, light_window_callback);
		}
		break;

	// If the 'O' key was released and we're not running full_screen, activate
	// the options window.

	case 'O':
		if (!key_down && !full_screen)
			activate_options_window();
		break;

	// If the 'P' key was released, send an event indicating polygon info is
	// requested.

	case 'P':
		if (!key_down)
			polygon_info_requested.send_event(true);
		break;

	// If the 'U' key was released and we're not running full_screen, check for
	// a new update.

	case 'U':
		if (!key_down && !full_screen)
			check_for_update_requested.send_event(true);
		break;

	// If the 'V' key was released and we're not running full_screen, display
	// the source of the current spot in a new browser window.

	case 'V':
		if (!key_down && !full_screen)
			display_file_as_web_page(curr_spot_file_path);
	}
}

//------------------------------------------------------------------------------
// Callback function to handle mouse events.
//------------------------------------------------------------------------------

static void
mouse_event_callback(int x, int y, int button_code, int task_bar_button_code)
{
	bool player_running_flag;
	float brightness;

	// If the player is not currently running, don't process any mouse events.

	start_atomic_operation();
	player_running_flag = player_running;
	end_atomic_operation();
	if (!player_running_flag)
		return;

	// Remove the light window if it is currently displayed.

	close_light_window();

	// Update the mouse cursor.

	update_mouse_cursor(x, y);

	// Handle the button code...

	switch (button_code) {

	// If the left mouse button was pressed, set the button status to reflect
	// this and determine whether movement should be enabled according to where
	// the mouse is pointing; if not, reset the motion deltas.  Then capture
	// the mouse.

	case LEFT_BUTTON_DOWN:
		curr_button_status = LEFT_BUTTON_DOWN;
		start_atomic_operation();
		if (inside_3D_window && !selection_active_flag)
			movement_enabled = true;
		else {
			movement_enabled = false;
			curr_move_delta = 0.0f;
			curr_side_delta = 0.0f;
			curr_turn_delta = 0.0f;
		}
		end_atomic_operation();
		capture_mouse();
		break;

	// If the right mouse button was pressed...

	case RIGHT_BUTTON_DOWN:

		// Set the button status and absolute motion flag, and reset the motion
		// deltas.

		curr_button_status = RIGHT_BUTTON_DOWN;
		start_atomic_operation();
		absolute_motion = true;
		curr_turn_delta = 0.0f;
		curr_look_delta = 0.0f;
		end_atomic_operation();

		// Initialise the last mouse position.

		last_x = x;
		last_y = y;

		// Set the cursor to a crosshair and capture the mouse.

		set_crosshair_cursor();
		capture_mouse();
		break;

	// If the left mouse button was released...

	case LEFT_BUTTON_UP:

		// Send an event to the player thread if an active link has been 
		// selected.
		
		if (inside_3D_window && selection_active_flag && !movement_enabled)
			mouse_clicked.send_event(true);

		// If one of the task bar buttons is active, and we're not running
		// full-screen, perform the button action.

		if (!full_screen) {
			switch (task_bar_button_code) {
			case LOGO_BUTTON:
				NPN_GetURL(active_inst_data_ptr->instance_ptr, FLATLAND_URL,
					"_blank");
				break;
			case HISTORY_BUTTON:
				start_atomic_operation();
				open_history_menu(spot_history_list);
				end_atomic_operation();
				select_history_entry(track_history_menu());
				close_history_menu();
				break;
			case OPTIONS_BUTTON:
				activate_options_window();
				break;
			case LIGHT_BUTTON:
				start_atomic_operation();
				brightness = master_brightness;
				end_atomic_operation();
				open_light_window(brightness, light_window_callback);
				break;
			case BUILDER_BUTTON:
				launch_builder_web_page();
				break;
			}
		}

		// Set the current button status, reset the motion deltas, and release
		// the mouse if it was captured.

		curr_button_status = MOUSE_MOVE_ONLY;
		start_atomic_operation();
		curr_move_delta = 0.0f;
		curr_side_delta = 0.0f;
		curr_turn_delta = 0.0f;
		end_atomic_operation();
		release_mouse();
		break;

	// If the right mouse button was released, set the button status, reset the
	// motion deltas, and release the mouse if it was captured.

	case RIGHT_BUTTON_UP:
		curr_button_status = MOUSE_MOVE_ONLY;
		start_atomic_operation();
		absolute_motion = false;
		curr_turn_delta = 0.0f;
		curr_look_delta = 0.0f;
		end_atomic_operation();
		release_mouse();
	}

	// Set the mouse cursor if the display is open.

	set_mouse_cursor();

	// React to the mouse button status.

	switch (curr_button_status) {

	// If the left button is down...

	case LEFT_BUTTON_DOWN:
		
		// Compute the player movement or rotation based upon the current mouse
		// direction, if movement is enabled.

		if (movement_enabled) {
			start_atomic_operation();
			switch (movement_arrow) {
			case ARROW_NW:
			case ARROW_N:
			case ARROW_NE:
				curr_move_delta = 1.0f;
				break;
			case ARROW_SW:
			case ARROW_S:
			case ARROW_SE:
				curr_move_delta = -1.0f;
				break;
			default:
				curr_move_delta = 0.0f;
			}
			switch (movement_arrow) {
			case ARROW_NE:
			case ARROW_E:
			case ARROW_SE:
				if (shift_key_down) {
					curr_side_delta = 1.0f;
					curr_turn_delta = 0.0f;
				} else {
					curr_side_delta = 0.0f;
					curr_turn_delta = 1.0f;
				}
				break;
			case ARROW_NW:
			case ARROW_W:
			case ARROW_SW:
				if (shift_key_down) {
					curr_side_delta = -1.0f;
					curr_turn_delta = 0.0f;
				} else {
					curr_side_delta = 0.0f;
					curr_turn_delta = -1.0f;
				}
				break;
			default:
				curr_side_delta = 0.0f;
				curr_turn_delta = 0.0f;
			}
			end_atomic_operation();
		} 
		break;

	// If the right mouse button is down, compute a player rotation that is
	// proportional to the distance the mouse has travelled since the last mouse
	// event (current one pixel = one degree), and add it to the current turn
	// or look delta.

	case RIGHT_BUTTON_DOWN:
		start_atomic_operation();
		curr_turn_delta += x - last_x;
		curr_look_delta += y - last_y;
		end_atomic_operation();
		last_x = x;
		last_y = y;
	}
}

//------------------------------------------------------------------------------
// Callback function to handle timer events.
//------------------------------------------------------------------------------

static void
timer_event_callback(void)
{
	bool player_running_flag;

	// If the player is running...
	
	start_atomic_operation();
	player_running_flag = player_running;
	end_atomic_operation();
	if (player_running_flag) {
		int x, y;

		// If the browser window has been minimised, send a pause event to the
		// player thread.

		if (!browser_window_minimised && browser_window_is_minimised()) {
			browser_window_minimised = true;
			pause_player_thread.send_event(true);
		}

		// If the browser window has been restored, send a resume event to the
		// player thread.

		if (browser_window_minimised && !browser_window_is_minimised()) {
			browser_window_minimised = false;
			resume_player_thread.send_event(true);
		}

		// If the browser window is not minimised, update the cursor position.

		if (!browser_window_minimised) {
			get_mouse_position(&x, &y, true);
			update_mouse_cursor(x, y);
			set_mouse_cursor();
		}
	}

	// Check to see whether a URL download request has been signalled by the
	// player thread.

	if (URL_download_requested.event_sent()) {

		// Reset the URL_was_opened flag, and send off the URL request to the
		// browser.

		URL_was_opened.reset_event();
		if (web_browser_ID == INTERNET_EXPLORER) {
			if (strlen(requested_target) != 0)
				NPN_GetURL(active_inst_data_ptr->instance_ptr, requested_URL,
					requested_target);
			else
				NPN_GetURL(active_inst_data_ptr->instance_ptr, requested_URL,
					NULL);
		} else {
			if (strlen(requested_target) != 0)
				NPN_GetURLNotify(active_inst_data_ptr->instance_ptr,
					requested_URL, requested_target, NULL);
			else
				NPN_GetURLNotify(active_inst_data_ptr->instance_ptr,
					requested_URL, NULL, NULL);
		}

		// Indicate that a URL request is no longer pending.

		start_atomic_operation();
		URL_request_pending = false;
		end_atomic_operation();
	}

	// Check to see whether a version URL download request has been signalled by
	// the player thread, and if so send off the URL request to the browser.
	// We don't bother with notification of failure.

	if (version_URL_download_requested.event_sent())
		NPN_GetURL(active_inst_data_ptr->instance_ptr, version_URL, NULL);

	// Check to see whether an update URL download request has been signalled by
	// the player thread, and if so send off the URL request to the browser.
	// We don't bother with notification of failure.

	if (update_URL_download_requested.event_sent())
		NPN_GetURL(active_inst_data_ptr->instance_ptr, update_URL, NULL);

	// Check to see whether a javascript URL download request has been signalled
	// by the player thread, and if so send off the URL request to the browser.
	// We don't bother with notification of failure.

	if (javascript_URL_download_requested.event_sent())
		NPN_GetURL(active_inst_data_ptr->instance_ptr, javascript_URL, NULL);

	// Check to see whether a display error log request has been signalled by
	// the player thread, and if so display a dialog box asking the user if they
	// want to see the error log, and display it if so.

	if (display_error_log.event_sent() && query("Errors in 3DML document", true,
		"One or more errors were encountered in the 3DML document.\n"
		"Do you want to view the error log?"))
		display_file_as_web_page(error_log_file_path);
}

//------------------------------------------------------------------------------
// Resize callback procedure.
//------------------------------------------------------------------------------

static void
resize_event_callback(void *window_handle, int width, int height)
{
	bool player_running_flag;

	// If the player is not running it's event loop, then it cannot handle any
	// size messages, so do nothing.

	start_atomic_operation();
	player_running_flag = player_running;
	end_atomic_operation();
	if (!player_running_flag)
		return;

	// Signal the player thread that a window resize is requested, and
	// wait for the player thread to signal that the player window has shut 
	// down.

	window_resize_requested.send_event(true);
	player_window_shut_down.wait_for_event();

	// Destroy the frame buffer.

	destroy_frame_buffer();

	// Resize the main window.

	set_main_window_size(width, height);

	// Compute the window "centre" coordinates.

	window_half_width = window_width * 0.5f;
	window_top_half_height = window_height * 0.75f;
	window_bottom_half_height = window_height * 0.25f;

	// Create and the frame buffer, and signal the player thread on it's success
	// or failure.  On failure, wait for the player thread to signal that the
	// player window has shut down, then destroy the main window and make the
	// instance inactive.

	if (create_frame_buffer())
		main_window_resized.send_event(true);
	else {
		main_window_resized.send_event(false);
		player_window_shut_down.wait_for_event();
		destroy_main_window();
		active_inst_data_ptr->active_instance = false;
		active_inst_data_ptr = NULL;
	}
}

//==============================================================================
// Plugin interface to browser.
//==============================================================================

//------------------------------------------------------------------------------
// NPP_Initialize:
// Provides global initialization for a plug-in, and returns an error value. 
//
// This function is called once when a plug-in is loaded, before the first
// instance is created. You should allocate any memory or resources shared by
// all instances of your plug-in at this time. After the last instance has been
// deleted, NPP_Shutdown will be called, where you can release any memory or
// resources allocated by NPP_Initialize. 
//------------------------------------------------------------------------------

NPError
NPP_Initialize(void)
{
	FILE *fp;
	int use_sounds;
	int prev_process_ID;
	int use_reflections;

#if TRACE
	start_trace();
#endif

	// Start up the platform API.

	if (!start_up_platform_API()) {
		shut_down_platform_API();
		return(NPERR_NO_ERROR);
	}

	// Determine the path to the Flatland directory.

	flatland_dir = app_dir + "Flatland\\";

	// Determine the path to various files in the Flatland directory.

	log_file_path = flatland_dir + "log.txt";
	error_log_file_path = flatland_dir + "errlog.html";
	config_file_path = flatland_dir + "config.txt";
	version_file_path = flatland_dir + "version.txt";
	update_file_path = flatland_dir + "update.txt";
	update_archive_path = flatland_dir + "update.zip";
	update_HTML_path = flatland_dir + "update.html";
	new_plugin_DLL_path = flatland_dir + "NPRover.dl_";
	updater_path = flatland_dir + "updater.exe";
	history_file_path = flatland_dir + "history.txt";
	curr_spot_file_path = flatland_dir + "curr_spot.txt";
	cache_file_path = flatland_dir + "cache.txt";
	javascript_file_path = flatland_dir + "javascript.txt";
	new_rover_file_path = flatland_dir + "new_rover.txt";

	// Clear the log file.

	if ((fp = fopen(log_file_path, "w")) != NULL)
		fclose(fp);

	// Attempt to open the config file and read the acceleration mode flag,
	// sound download flag, visible block radius, previous process ID,
	// use reflections flag, current move rate, current rotate rate,
	// and the previous window width and height.
	//
	// If the config file does not exist, use default values and make sure the
	// flatland directory exists so the config file can be rewritten later.
	//
	// XXX -- Must use integer variables for fscanf() rather than boolean ones,
	// since boolean variables are only one byte, otherwise fscanf() will fail.

	if (render_mode == HARDWARE_WINDOW)
		acceleration_mode = TRY_HARDWARE;
	else
		acceleration_mode = TRY_SOFTWARE;
	use_sounds = true;
	visible_block_radius = -1;
	prev_process_ID = -1;
	use_reflections = false;
	curr_move_rate = DEFAULT_MOVE_RATE;
	curr_rotate_rate = DEFAULT_ROTATE_RATE;
	prev_window_width = 320;
	prev_window_height = 240;
	if ((fp = fopen(config_file_path, "r")) != NULL) {
		fscanf(fp, "%d %d %d %d %d %f %f %d %d %d", &acceleration_mode, 
			&use_sounds, &visible_block_radius, &prev_process_ID, 
			&use_reflections, &curr_move_rate, &curr_rotate_rate,
			&prev_window_width, &prev_window_height);
		fclose(fp);
	} else
		mkdir(flatland_dir);

	// Set the download sounds flag and the hardware acceleration flag.

	download_sounds = use_sounds ? true : false;
	hardware_acceleration = acceleration_mode == TRY_HARDWARE;
	
	// If the sound system is not A3D or reflections are not available, turn
	// off reflections regardless of the setting of the use reflections flag
	// in the config file.

	if (sound_system != A3D_SOUND || !reflections_available)
		reflections_enabled = false;
	else
		reflections_enabled = use_reflections ? true : false;

	// Get the current process ID, and compare it with the previous process ID.
	// If they are the same, assume the plugin has been reloaded in the same
	// browser session.

	curr_process_ID = get_process_ID();
	if (curr_process_ID == prev_process_ID)
		same_browser_session = true;
	else
		same_browser_session = false;

	// Indicate the player is not active yet, there is no player window,
	// no active plugin instance, and the browser window is not minimised.

	player_active = false;
	player_window_created = false;
	active_inst_data_ptr = NULL;
	browser_window_minimised = false;

	// Initialise the update, version and requested stream pointers.

	update_stream_ptr = NULL;
	version_stream_ptr = NULL;
	requested_stream_ptr = NULL;

	// Create the events sent by the player thread.

	player_thread_initialised.create_event();
	player_window_initialised.create_event();
	URL_download_requested.create_event();
	version_URL_download_requested.create_event();
	update_URL_download_requested.create_event();
	javascript_URL_download_requested.create_event();
	player_window_shut_down.create_event();
	display_error_log.create_event();

	// Create the events sent by the plugin thread.

	main_window_created.create_event();
	main_window_resized.create_event();
	URL_was_opened.create_event();
	URL_was_downloaded.create_event();
	version_URL_was_downloaded.create_event();
	update_URL_was_downloaded.create_event();
	window_mode_change_requested.create_event();
	window_resize_requested.create_event();
	mouse_clicked.create_event();
	player_window_shutdown_requested.create_event();
	player_window_init_requested.create_event();
	pause_player_thread.create_event();
	resume_player_thread.create_event();
	history_entry_selected.create_event();
	check_for_update_requested.create_event();
	polygon_info_requested.create_event();

	// Initialise all variables that require synchronised access.

	player_running = false;
	movement_enabled = false;
	curr_move_delta = 0.0f;
	curr_turn_delta = 0.0f;
	curr_look_delta = 0.0f;
	curr_mouse_x = -1;
	curr_mouse_y = -1;
	curr_button_status = MOUSE_MOVE_ONLY;
	shift_key_down = false;
	ctrl_key_down = false;
	master_brightness = 0.0f;
	URL_request_pending = false;

	// Start the player thread.

	if (!start_player_thread())
		return(NPERR_NO_ERROR);

	// Wait for an event from the player thread regarding it's initialisation.

	player_active = player_thread_initialised.wait_for_event();
	return(NPERR_NO_ERROR);
}

//------------------------------------------------------------------------------
// NPP_GetJavaClass:
// New in Netscape Navigator 3.0. 
//
// NPP_GetJavaClass is called during initialization to ask your plugin
// what its associated Java class is. If you don't have one, just return
// NULL. Otherwise, use the javah-generated "use_" function to both
// initialize your class and return it. If you can't find your class, an
// error will be signalled by "use_" and will cause the Navigator to
// complain to the user.
//------------------------------------------------------------------------------

jref
NPP_GetJavaClass(void)
{
	return(NULL);
}

//------------------------------------------------------------------------------
// NPP_Shutdown:
// Provides global deinitialization for a plug-in. 
// 
// This function is called once after the last instance of your plug-in is
// destroyed. Use this function to release any memory or resources shared
// across all instances of your plug-in. You should be a good citizen and
// declare that you're not using your java class any more. This allows java to
// unload it, freeing up memory.
//------------------------------------------------------------------------------

void
NPP_Shutdown(void)
{
	FILE *fp;

	// If the player thread is active, signal it to terminate, and wait for it
	// to do so.

	if (player_active) {
		player_window_init_requested.send_event(false);
		wait_for_player_thread_termination();
	}

	// Close the progress, light and option windows, if they are still open.

	close_progress_window();
	close_light_window();
	close_options_window();

	// Destroy the events sent by the player thread.

	player_thread_initialised.destroy_event();
	player_window_initialised.destroy_event();
	URL_download_requested.destroy_event();
	version_URL_download_requested.destroy_event();
	update_URL_download_requested.destroy_event();
	javascript_URL_download_requested.destroy_event();
	player_window_shut_down.destroy_event();
	display_error_log.destroy_event();

	// Destroy the events sent by the plugin thread.

	main_window_created.destroy_event();
	main_window_resized.destroy_event();
	URL_was_opened.destroy_event();
	URL_was_downloaded.destroy_event();
	version_URL_was_downloaded.destroy_event();
	update_URL_was_downloaded.destroy_event();
	window_mode_change_requested.destroy_event();
	window_resize_requested.destroy_event();
	mouse_clicked.destroy_event();
	player_window_shutdown_requested.destroy_event();
	player_window_init_requested.destroy_event();
	pause_player_thread.destroy_event();
	resume_player_thread.destroy_event();
	history_entry_selected.destroy_event();
	check_for_update_requested.destroy_event();
	polygon_info_requested.destroy_event();

	// Attempt to open the config file and write the acceleration mode flag,
	// sound download flag, visible block radius, current process ID,
	// reflections enabled flag, current move rate, current rotate rate,
	// and the previous window width and height.

	if (hardware_acceleration)
		acceleration_mode = TRY_HARDWARE;
	else
		acceleration_mode = TRY_SOFTWARE;
	if ((fp = fopen(config_file_path, "w")) != NULL) {
		fprintf(fp, "%d %d %d %d %d %g %g %d %d", acceleration_mode, 
			download_sounds, visible_block_radius, curr_process_ID, 
			reflections_enabled, curr_move_rate, curr_rotate_rate, 
			prev_window_width, prev_window_height);
		fclose(fp);
	}

	// Shut down the platform API.

	shut_down_platform_API();

#if TRACE
	end_trace();
#endif
}

//------------------------------------------------------------------------------
// NPP_New:
// Creates a new instance of a plug-in and returns an error value. 
// 
// NPP_New creates a new instance of your plug-in with MIME type specified
// by pluginType. The parameter mode is NP_EMBED if the instance was created
// by an EMBED tag, or NP_FULL if the instance was created by a separate file.
// You can allocate any instance-specific private data in instance->pdata at
// this time. The NPP pointer is valid until the instance is destroyed. 
//------------------------------------------------------------------------------

NPError 
NPP_New(NPMIMEType pluginType, NPP instance_ptr, uint16 mode, int16 argc,
		char *argn[], char *argv[], NPSavedData *saved_data_ptr)
{
	InstData *inst_data_ptr;
	string user_agent;
	char version_number[32];
	HKEY key_handle;
	DWORD version_size;
	int index;

	// Check for a NULL instance.

	if (instance_ptr == NULL)
		return(NPERR_INVALID_INSTANCE_ERROR);

	// Determine which browser we are running under.

	user_agent = NPN_UserAgent(instance_ptr);
	strlwr(user_agent);
	if (strstr(user_agent, "opera")) {
		web_browser_ID = OPERA;
		web_browser_version = "Opera";
	} else if (strstr(user_agent, "mozilla")) {
		web_browser_ID = NAVIGATOR;
		web_browser_version = "Navigator";
		if (sscanf(user_agent, "mozilla/%s", version_number) == 1) {
			web_browser_version += " ";
			web_browser_version += version_number;
		}
	} else if (strstr(user_agent, "internet explorer")) {
		web_browser_ID = INTERNET_EXPLORER;
		web_browser_version = "MSIE";
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			"Software\\Microsoft\\Internet Explorer\\", 0, 
			KEY_QUERY_VALUE, &key_handle) == ERROR_SUCCESS) {
			version_size = 16;
			if (RegQueryValueEx(key_handle, "Version", NULL, NULL,
				(LPBYTE)&version_number, &version_size) == ERROR_SUCCESS) {
				web_browser_version += " ";
				web_browser_version += version_number;
			}
			RegCloseKey(key_handle);
		}
	} else {
		web_browser_ID = UNKNOWN_BROWSER;
		web_browser_version = user_agent;
	}

	// Allocate the instance data structure, and initialise the instance
	// pointer.

	if ((instance_ptr->pdata = new InstData) == NULL)
		return(NPERR_OUT_OF_MEMORY_ERROR);
	inst_data_ptr = (InstData *)instance_ptr->pdata;
	inst_data_ptr->instance_ptr = instance_ptr;

	// Look for an argument with the name "TEXT", and if found store it's value
	// in the instance data structure as the inactive window text.

	for (index = 0; index < argc; index++)
		if (!stricmp(argn[index], "text"))
			break;
	if (index < argc)
		inst_data_ptr->window_text = argv[index];
	else
		inst_data_ptr->window_text = "Click here to view spot";
	
	// If the player is active...

	if (player_active) {

		// If this is the first instance to be created, then make it the active
		// one and send a request to the player thread to initialise the player
		// window.

		if (active_inst_data_ptr == NULL) {
			active_inst_data_ptr = inst_data_ptr;
			inst_data_ptr->active_instance = true;
			player_window_init_requested.send_event(true);
		} else
			inst_data_ptr->active_instance = false;
	}

	// If the player is inactive, all instances are inactive.

	else
		inst_data_ptr->active_instance = false;
	
	// Return success status.

	return(NPERR_NO_ERROR);
}

//------------------------------------------------------------------------------
// NPP_Destroy:
// Deletes a specific instance of a plug-in and returns an error value. 
//
// NPP_Destroy is called when a plug-in instance is deleted, typically because
// the user has left the page containing the instance, closed the window, or
// quit the application. You should delete any private instance-specific
// information stored in instance->pdata.  If the instance being deleted is the
// last instance created by your plug-in, NPP_Shutdown will subsequently be 
// called, where you can delete any data allocated in NPP_Initialize to be
// shared by all your plug-in's instances.  Note that you should not perform
// any graphics operations in NPP_Destroy as the instance's window is no longer
// guaranteed to be valid. 
//------------------------------------------------------------------------------

NPError 
NPP_Destroy(NPP instance_ptr, NPSavedData **saved_data_handle)
{
	InstData *inst_data_ptr;
	NPWindow *window_ptr;

	// Check for a NULL instance.

	if (instance_ptr == NULL)
		return(NPERR_INVALID_INSTANCE_ERROR);
	inst_data_ptr = (InstData *)instance_ptr->pdata;

	// If the player thread is active...

	if (player_active) {

		// If this is the active instance, destroy the player window and make
		// this instance inactive.
		
		if (inst_data_ptr == active_inst_data_ptr) {
			destroy_player_window();
			active_inst_data_ptr = NULL;
		}

		// Otherwise restore the plugin window.

		else {
			window_ptr = inst_data_ptr->window_ptr;
			restore_plugin_window(window_ptr->window);
		}
	}

	// Delete the instance data.

	delete inst_data_ptr;
	instance_ptr->pdata = NULL;
	return(NPERR_NO_ERROR);
}

//------------------------------------------------------------------------------
// Inactive plugin window callback procedure.
//------------------------------------------------------------------------------

static void
inactive_window_callback(void *window_data_ptr)
{
	InstData *inst_data_ptr;
	NPWindow *window_ptr;

	// If the player is not active or this instance does not yet have a spot 
	// URL, do nothing.

	inst_data_ptr = (InstData *)window_data_ptr;
	if (!player_active || strlen(inst_data_ptr->spot_URL) == 0)
		return;

	// If there is an active plugin window, make it inactive.

	if (active_inst_data_ptr) {
		window_ptr = active_inst_data_ptr->window_ptr;

		// Destroy the player window.

		destroy_player_window();

		// Make the plugin window inactive.

		set_plugin_window(window_ptr->window, active_inst_data_ptr,
			active_inst_data_ptr->window_text, inactive_window_callback);

		// Make the plugin instance inactive.

		active_inst_data_ptr->active_instance = false;
		active_inst_data_ptr = NULL;
	}

	// Signal the player thread that a new player window is requested.
	
	player_window_init_requested.send_event(true);

	// Restore the inactive plugin window.

	window_ptr = inst_data_ptr->window_ptr;
	restore_plugin_window(window_ptr->window);

	// Create the player window, and if this succeeds make this instance
	// the active one.

	if (create_player_window(inst_data_ptr)) {
	
		// Make the new instance active.

		active_inst_data_ptr = inst_data_ptr;
		active_inst_data_ptr->active_instance = true;

		// Request that the instance's spot URL be downloaded, after 
		// resetting URL_was_opened.

		URL_was_opened.reset_event();
		if (web_browser_ID == INTERNET_EXPLORER)
			NPN_GetURL(active_inst_data_ptr->instance_ptr, 
				active_inst_data_ptr->spot_URL, NULL);
		else
			NPN_GetURLNotify(active_inst_data_ptr->instance_ptr, 
				active_inst_data_ptr->spot_URL, NULL, NULL);
	}
}

//------------------------------------------------------------------------------
// NPP_SetWindow:
// Sets the window in which a plug-in draws, and returns an error value. 
// 
// NPP_SetWindow informs the plug-in instance specified by instance of the
// the window denoted by window in which the instance draws.  This NPWindow
// pointer is valid for the life of the instance, or until NPP_SetWindow is
// called again with a different value.  Subsequent calls to NPP_SetWindow for
// a given instance typically indicate that the window has been resized.  If
// either window or window->window are NULL, the plug-in must not perform any
// additional graphics operations on the window and should free any resources
// associated with the window. 
//------------------------------------------------------------------------------

NPError 
NPP_SetWindow(NPP instance_ptr, NPWindow *new_window_ptr)
{
	InstData *inst_data_ptr;
	NPWindow *window_ptr;

	// Check for a NULL instance.

	if (instance_ptr == NULL)
		return(NPERR_INVALID_INSTANCE_ERROR);

	// PLUGIN DEVELOPERS:
	// Before setting window to point to the new window, you may wish to
	// compare the new window info to the previous window (if any) to note
	// window size changes, etc.

	inst_data_ptr = (InstData *)instance_ptr->pdata;
	window_ptr = inst_data_ptr->window_ptr;
	if (window_ptr != NULL) {

		// If there is no new window, restore the existing plugin window if
		// this is an inactive instance, and return.

		if ((new_window_ptr == NULL) || (new_window_ptr->window == NULL)) {
			if (inst_data_ptr != active_inst_data_ptr)
				restore_plugin_window(window_ptr->window);
			inst_data_ptr->window_ptr = NULL;
			return(NPERR_NO_ERROR);
		} 
		
		// If the new window is the same as the old window, just get out after
		// recording the new window pointer.

		else if (window_ptr->window == new_window_ptr->window) {
			inst_data_ptr->window_ptr = new_window_ptr;
			return(NPERR_NO_ERROR);
		}
			
		// If the new window is different than the old window, then restore
		// the old window if this is an inactive instance before continuing.
		// NOTE: We don't want this path invoked for active windows, as it
		// will play havoc with DirectX.  Current browsers don't invoke this
		// path at all.

		else if (inst_data_ptr != active_inst_data_ptr)
			restore_plugin_window(window_ptr->window);
	} 
	
	// We can just get out of here if there is no current window and there
	// is no new window to use.

	else if ((new_window_ptr == NULL) || (new_window_ptr->window == NULL)) {
		inst_data_ptr->window_ptr = NULL;
		return(NPERR_NO_ERROR);
	}

	// Set the window pointer in the instance data.

	inst_data_ptr->window_ptr = new_window_ptr;
	window_ptr = new_window_ptr;

	// If this instance is the active one...

	if (inst_data_ptr == active_inst_data_ptr) {

		// If we don't yet have a player window, create it and signal the player
		// thread, then wait for a signal from the player thread that the main
		// window was initialised.  If either of these result in failure, the
		// instance is made inactive.

		if (!player_window_created) {

			// Create the player window.  If this fails, make the active
			// instance inactive.

			if (!create_player_window(inst_data_ptr)) {
				active_inst_data_ptr->active_instance = false;
				active_inst_data_ptr = NULL;
			}
		}
	} 
	
	// If this is not the active instance, make the plugin window inactive.
	
	if (inst_data_ptr != active_inst_data_ptr)
		set_plugin_window(window_ptr->window, inst_data_ptr,
			inst_data_ptr->window_text, inactive_window_callback);

	// Return success status.

	return(NPERR_NO_ERROR);
}

//------------------------------------------------------------------------------
// Progress window callback function: this is called if the user hit the cancel
// button on the progress window.
//------------------------------------------------------------------------------

static void
progress_window_callback(void)
{
	// If we are running under Internet Explorer, close the progress window,
	// send an event to the player thread indicating that the stream did not 
	// download successfully (for blockset downloads), and reset the stream 
	// pointer.  Then remove the partially downloaded file.
	//
	// XXX -- This procedure is necessary because IE does not implement
	// NPP_DestroyStream, so there is no way to actually stop the stream.

	if (web_browser_ID == INTERNET_EXPLORER) {
		close_progress_window();
		if (requested_stream_ptr) {
			requested_stream_ptr = NULL;
			start_atomic_operation();
			downloaded_URL = requested_URL;
			end_atomic_operation();
			URL_was_downloaded.send_event(false);
			remove(requested_file_path);
		} else {
			update_stream_ptr = NULL;
			remove(update_archive_path);
		}
	}

	// If we are not running under Internet Explorer, ask the browser to destroy
	// the stream that is currently open.

	else {
		if (requested_stream_ptr)
			NPN_DestroyStream(active_inst_data_ptr->instance_ptr,
				requested_stream_ptr, NPRES_USER_BREAK);
		else
			NPN_DestroyStream(active_inst_data_ptr->instance_ptr,
				update_stream_ptr, NPRES_USER_BREAK);
	}
}

//------------------------------------------------------------------------------
// NPP_NewStream:
// Notifies an instance of a new data stream and returns an error value. 
// 
// NPP_NewStream notifies the instance denoted by instance of the creation of
// a new stream specifed by stream. The NPStream pointer is valid until the
// stream is destroyed. The MIME type of the stream is provided by the
// parameter type. 
//------------------------------------------------------------------------------

NPError 
NPP_NewStream(NPP instance_ptr, NPMIMEType type, NPStream *stream_ptr,
			  NPBool seekable, uint16 *stype_ptr)
{
	InstData *inst_data_ptr;
	FILE *fp;

	// Return an error if the instance is NULL.

	if (instance_ptr == NULL)
		return(NPERR_INVALID_INSTANCE_ERROR);

	// If the stream URL matches the version URL, stream it into a file.

	if (!stricmp(stream_ptr->url, version_URL)) {
		*stype_ptr = NP_NORMAL;
		version_stream_ptr = stream_ptr;
		if ((fp = fopen(version_file_path, "w")) != NULL)
			fclose(fp);
	}

	// If the stream URL matches the update URL, stream it into a file.

	else if (!stricmp(stream_ptr->url, update_URL)) {
		*stype_ptr = NP_NORMAL;
		update_stream_ptr = stream_ptr;
		if ((fp = fopen(update_archive_path, "wb")) != NULL)
			fclose(fp);

		// If requested_rover_version is set, a rover update is being
		// downloaded at the user's explicit request, so open the progress
		// window with an appropiate message.

		if (strlen(requested_rover_version) > 0)
			open_progress_window(stream_ptr->end, progress_window_callback,
				"Downloading version %s of Flatland Rover.", 
					requested_rover_version);
	}

	// If the stream URL matches the javascript URL, we're going to stream it
	// into oblivion.

	else if (!stricmp(stream_ptr->url, javascript_URL)) {
		*stype_ptr = NP_NORMAL;
		javascript_URL = "";
	}

	// Otherwise ask for the stream to be downloaded into a local file, and
	// inform the player thread that the URL stream was opened.  We use this 
	// flag to ignore the reason code passed to NPP_Notify(), and instead allow
	// NPP_DestroyStream() to signal failure to the player thread.  This is
	// necessary because Navigator 3.0 sends a NPRES_NETWORK_ERR reason code to
	// NPP_Notify() even upon success.

	else {

		// If a requested file path was given, stream the URL into this file,
		// otherwise let the browser choose a file path.

		if (strlen(requested_file_path) > 0) {
			*stype_ptr = NP_NORMAL;
			requested_stream_ptr = stream_ptr;
			if ((fp = fopen(requested_file_path, "wb")) != NULL)
				fclose(fp);

			// If requested_blockset_name is set, a blockset is being downloaded
			// so open the progress window with an approapiate message.

			if (strlen(requested_blockset_name) > 0)
				open_progress_window(stream_ptr->end, progress_window_callback, 
					"Downloading %s blockset.", requested_blockset_name);
		} else
			*stype_ptr = NP_ASFILE;

		// Inform the player thread that the URL was opened.

		URL_was_opened.send_event(true);
		
		// If the spot URL has not yet been set for this instance, then this is
		// the original spot URL, which needs to be stored in the instance
		// data structure.

		inst_data_ptr = (InstData *)instance_ptr->pdata;
		if (strlen(inst_data_ptr->spot_URL) == 0) {
			inst_data_ptr->spot_URL = stream_ptr->url;
			inst_data_ptr->spot_URL = decode_URL(inst_data_ptr->spot_URL);
		}
	}

	// Return success code.

	return(NPERR_NO_ERROR);
}

// PLUGIN DEVELOPERS:
// These next 2 functions are directly relevant in a plug-in which handles the
// data in a streaming manner.  If you want zero bytes because no buffer space
// is YET available, return 0.  As long as the stream has not been written to
// the plugin, Navigator will continue trying to send bytes.  If the plugin
// doesn't want them, just return some large number from NPP_WriteReady(), and
// ignore them in NPP_Write().  For a NP_ASFILE stream, they are still called
// but can safely be ignored using this strategy.

//------------------------------------------------------------------------------
// NPP_WriteReady:
// Returns the maximum number of bytes that an instance is prepared to accept
// from the stream. 
// 
// NPP_WriteReady determines the maximum number of bytes that the instance will
// consume from the stream in a subsequent call NPP_Write. This function allows
// Netscape to only send as much data to the instance as the instance is
// capable of handling at a time, allowing more efficient use of resources
// within both Netscape and the plug-in. 
//------------------------------------------------------------------------------

int32 
NPP_WriteReady(NPP instance_ptr, NPStream *stream_ptr)
{
	// Allow Netscape to send as much as it likes.

	return(0x7fffffff);
}

//------------------------------------------------------------------------------
// NPP_Write:
// Delivers data from a stream and returns the number of bytes written. 
// 
// NPP_Write is called after a call to NPP_NewStream in which the plug-in
// requested a normal-mode stream, in which the data in the stream is delivered
// progressively over a series of calls to NPP_WriteReady and NPP_Write. The
// function delivers a buffer buf of len bytes of data from the stream
// identified by stream to the instance. The parameter offset is the logical
// position of buf from the beginning of the data in the stream. 
//
// The function returns the number of bytes written (consumed by the instance).
// A negative return value causes an error on the stream, which will
// subsequently be destroyed via a call to NPP_DestroyStream. 
// 
// Note that a plug-in must consume at least as many bytes as it indicated in
// the preceeding NPP_WriteReady call. All data consumed must be either
// processed immediately or copied to memory allocated by the plug-in: the buf
// parameter is not persistent. 
//------------------------------------------------------------------------------

int32 
NPP_Write(NPP instance_ptr, NPStream *stream_ptr, int32 offset, int32 len,
		  void *buffer_ptr)
{
	FILE *fp;
	const char *file_path;
	const char *file_mode;

	// Determine what the destination file path is, and the file mode to use.
	// For Rover update streams, if requested_rover_version is set then update
	// the progress window.

	if (stream_ptr == version_stream_ptr) {
		file_path = version_file_path;
		file_mode = "r+";
	} else if (stream_ptr == update_stream_ptr) {
		file_path = update_archive_path;
		file_mode = "rb+";
		if (strlen(requested_rover_version) > 0)
			update_progress_window(offset + len);
	} else if (stream_ptr == requested_stream_ptr) {
		file_path = requested_file_path;
		file_mode = "rb+";
	} 
	
	// If the stream is not recognised, then just ignore it.

	else
		return(len);

	// Open the destionation file, and write the supplied buffer contents to the
	// file at the given file offset.

	if ((fp = fopen(file_path, file_mode)) != NULL) {
		fseek(fp, offset, SEEK_SET);
		fwrite(buffer_ptr, len, 1, fp);
		fclose(fp);
	}

	// If this is a requested stream and a requested blockset name was
	// specified, update the progress window.

	if (stream_ptr == requested_stream_ptr && 
		strlen(requested_blockset_name) > 0)
		update_progress_window(offset + len);

	// Return the number of bytes written.

	return(len);
}

//------------------------------------------------------------------------------
// NPP_DestroyStream:
// Indicates the closure and deletion of a stream, and returns an error value. 
// 
// The NPP_DestroyStream function is called when the stream identified by
// stream for the plug-in instance denoted by instance will be destroyed. You
// should delete any private data allocated in stream->pdata at this time. 
//------------------------------------------------------------------------------

NPError 
NPP_DestroyStream(NPP instance_ptr, NPStream *stream_ptr, NPError reason)
{
	InstData *inst_data_ptr;

	// Check for NULL instance.

	if (instance_ptr == NULL)
		return(NPERR_INVALID_INSTANCE_ERROR);

	// If this is the version stream, reset the version URL and stream pointer.
	// If the reason parameter is NPRES_DONE, then signal the player thread that
	// the version file was downloaded.

	if (stream_ptr == version_stream_ptr) {
		version_URL = "";
		version_stream_ptr = NULL;
		if (reason == NPRES_DONE)
			version_URL_was_downloaded.send_event(true);
		return(NPERR_NO_ERROR);
	}

	// If this is the update stream, reset the update URL and stream pointer,
	// and close the progress window if necessary.  Then if the reason parameter
	// is NPRES_DONE, signal the player thread that the update file was 
	// downloaded, otherwise delete the partially downloaded update file.

	if (stream_ptr == update_stream_ptr) {
		if (strlen(requested_rover_version) > 0)
			close_progress_window();
		update_URL = "";
		update_stream_ptr = NULL;
		if (reason == NPRES_DONE)
			update_URL_was_downloaded.send_event(true);
		else
			remove(update_archive_path);
		return(NPERR_NO_ERROR);
	}

	// Signal the plugin if the file was not downloaded successfully.  If the
	// URL was streamed into a requested file, also signal the plugin if the
	// file *was* downloaded successfully.

	inst_data_ptr = (InstData *)instance_ptr->pdata;
	if (inst_data_ptr == active_inst_data_ptr) {

		// Set the download URL.

		start_atomic_operation();
		downloaded_URL = stream_ptr->url;
		end_atomic_operation();

		// If a requested file path was set, set the downloaded file path,
		// reset the requested stream pointer, and send an event to the player
		// thread informing it whether the download was successful or not.

		if (strlen(requested_file_path) > 0) {
			start_atomic_operation();
			downloaded_file_path = requested_file_path;
			end_atomic_operation();
			requested_stream_ptr = NULL;
			URL_was_downloaded.send_event(reason == NPRES_DONE);

			// If a blockset was been downloaded, close the progress window and
			// delete the partial blockset file if the download failed.

			if (strlen(requested_blockset_name) > 0) {
				close_progress_window();
				if (reason != NPRES_DONE)
					remove(requested_file_path);
			}
		} 
		
		// If no requested file path was set, send an event to the player thread
		// if the download was not successful.

		else if (reason != NPRES_DONE)
			URL_was_downloaded.send_event(false);
	}
	return(NPERR_NO_ERROR);
}

//------------------------------------------------------------------------------
// NPP_StreamAsFile:
// Provides a local file name for the data from a stream. 
// 
// NPP_StreamAsFile provides the instance with a full path to a local file,
// identified by fname, for the stream specified by stream. NPP_StreamAsFile is
// called as a result of the plug-in requesting mode NP_ASFILEONLY or
// NP_ASFILE in a previous call to NPP_NewStream. If an error occurs while
// retrieving the data or writing the file, fname may be NULL. 
//------------------------------------------------------------------------------

void 
NPP_StreamAsFile(NPP instance_ptr, NPStream *stream_ptr, const char *file_name)
{
	InstData *inst_data_ptr;

	// Check for NULL instance.

	if (instance_ptr == NULL)
		return;
	inst_data_ptr = (InstData *)instance_ptr->pdata;

	// If this is the active instance...

	if (inst_data_ptr == active_inst_data_ptr) {

		// If the file name is NULL, the URL was not downloaded.  This error
		// ought to be reported via NPP_Notify(), but for some reason it comes
		// here and NPP_Notify() gets a success notification.

		if (file_name == NULL) {
			start_atomic_operation();
			downloaded_URL = stream_ptr->url;
			end_atomic_operation();
			URL_was_downloaded.send_event(false);
			return;
		}

		// Signal the player thread that the URL was downloaded, making sure the
		// downloaded URL and local file path is set.

		else {
			start_atomic_operation();
			downloaded_URL = stream_ptr->url;
			downloaded_file_path = file_name;
			end_atomic_operation();
			URL_was_downloaded.send_event(true);
		}
	}
}

//------------------------------------------------------------------------------
// NPP_URLNotify:
// Notifies the instance of the completion of a URL request. 
//
// NPP_URLNotify is called when Netscape completes a NPN_GetURLNotify or
// NPN_PostURLNotify request, to inform the plug-in that the request,
// identified by url, has completed for the reason specified by reason.  The
// most common reason code is NPRES_DONE, indicating simply that the request
// completed normally.  Other possible reason codes are NPRES_USER_BREAK,
// indicating that the request was halted due to a user action (for example,
// clicking the "Stop" button), and NPRES_NETWORK_ERR, indicating that the
// request could not be completed (for example, because the URL could not be
// found).  The complete list of reason codes is found in npapi.h. 
// 
// The parameter notifyData is the same plug-in-private value passed as an
// argument to the corresponding NPN_GetURLNotify or NPN_PostURLNotify
// call, and can be used by your plug-in to uniquely identify the request. 
//------------------------------------------------------------------------------

void
NPP_URLNotify(NPP instance_ptr, const char *URL, NPReason reason, 
			  void *notifyData_ptr)
{
	InstData *inst_data_ptr;

	// If the instance pointer is NULL or URL_was_opened is TRUE, ignore
	// the reason code.

	if (instance_ptr == NULL || URL_was_opened.event_value)
		return;

	// If the reason parameter is anything other than NPRES_DONE, inform the
	// plugin that the URL was not downloaded and make this instance inactive.

	inst_data_ptr = (InstData *)instance_ptr->pdata;
	if (inst_data_ptr == active_inst_data_ptr && reason != NPRES_DONE) {
		start_atomic_operation();
		downloaded_URL = URL;
		end_atomic_operation();
		URL_was_downloaded.send_event(false);
	}
}

//------------------------------------------------------------------------------
// NPP_Print:
// Handle a print request.
//------------------------------------------------------------------------------

void 
NPP_Print(NPP instance_ptr, NPPrint *printInfo_ptr)
{
	if (instance_ptr == NULL || printInfo_ptr == NULL)
		return;

	if (printInfo_ptr->mode == NP_FULL) {
		    
	    // PLUGIN DEVELOPERS:
	    //	If your plugin would like to take over printing completely when
		// it is in full-screen mode, set printInfo->pluginPrinted to TRUE
		// and print your plugin as you see fit.  If your plugin wants
		// Netscape	to handle printing in this case, set 
		// printInfo->pluginPrinted to FALSE (the default) and do nothing.
		// If you do want to handle printing yourself, printOne is true if
		// the print button	(as opposed to the print menu) was clicked.
		// On the Macintosh, platformPrint is a THPrint; on Windows,
		// platformPrint is a structure (defined in npapi.h) containing the
		// printer name, port,etc.

		void *platformPrint = printInfo_ptr->print.fullPrint.platformPrint;
		NPBool printOne = printInfo_ptr->print.fullPrint.printOne;
		
		// Do the default.

		printInfo_ptr->print.fullPrint.pluginPrinted = FALSE;
	} else {
		
		// If not fullscreen, we must be embedded.

		// PLUGIN DEVELOPERS:
		// If your plugin is embedded, or is full-screen but you returned
		// false in pluginPrinted above, NPP_Print will be called with mode
		// == NP_EMBED.  The NPWindow in the printInfo gives the location
		// and dimensions of the embedded plugin on the printed page.  On
		// the Macintosh, platformPrint is the printer port; on	Windows,
		// platformPrint is the handle to the printing device context.

		NPWindow *printWindow =	&(printInfo_ptr->print.embedPrint.window);
		void *platformPrint = printInfo_ptr->print.embedPrint.platformPrint;
	}
}

//------------------------------------------------------------------------------
// NPP_HandleEvent:
// Mac-only, but stub must be present for Windows.
// Delivers a platform-specific event to the instance. 
// 
// On the Macintosh, event is a pointer to a standard Macintosh EventRecord.
// All standard event types are passed to the instance as appropriate.  In
// general, return TRUE if you handle the event and FALSE if you ignore the
// event. 
//------------------------------------------------------------------------------

int16
NPP_HandleEvent(NPP instance_ptr, void *event_ptr)
{
	// Do nothing--this is not a Mac.

	return(0);
}
