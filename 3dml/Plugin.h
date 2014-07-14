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

// Web browser IDs.

#define	UNKNOWN_BROWSER		0
#define	NAVIGATOR			1
#define INTERNET_EXPLORER	2
#define OPERA				3

// Important directories and file paths.

extern string flatland_dir;
extern string log_file_path;
extern string error_log_file_path;
extern string config_file_path;
extern string version_file_path;
extern string update_file_path;
extern string update_archive_path;
extern string update_HTML_path;
extern string new_plugin_DLL_path;
extern string updater_path;
extern string history_file_path;
extern string curr_spot_file_path;
extern string cache_file_path;
extern string javascript_file_path;
extern string new_rover_file_path;

// Miscellaneous global variables.

extern bool same_browser_session;
extern bool hardware_acceleration;
extern bool full_screen;
extern int prev_window_width, prev_window_height;

// Variables used to request a URL.

extern string requested_URL;
extern string requested_target;
extern string requested_file_path;
extern string requested_blockset_name;
extern string requested_rover_version;

// Downloaded URL and file path.

extern string downloaded_URL;
extern string downloaded_file_path;

// Update URL and file offset.

extern string update_URL;
extern int update_offset;

// Version URL.

extern string version_URL;

// Javascript URL.

extern string javascript_URL;

// Web browser ID and version.

extern int web_browser_ID;
extern string web_browser_version;

// Events sent by player thread.

extern event player_thread_initialised;
extern event player_window_initialised;
extern event URL_download_requested;
extern event version_URL_download_requested;
extern event update_URL_download_requested;
extern event javascript_URL_download_requested;
extern event player_window_shut_down;
extern event display_error_log;

// Events sent by plugin thread.

extern event main_window_created;
extern event main_window_resized;
extern event URL_was_opened;
extern event URL_was_downloaded;
extern event version_URL_was_downloaded;
extern event update_URL_was_downloaded;
extern event window_mode_change_requested;
extern event window_resize_requested;
extern event mouse_clicked;
extern event player_window_shutdown_requested;
extern event player_window_init_requested;
extern event pause_player_thread;
extern event resume_player_thread;
extern event history_entry_selected;
extern event check_for_update_requested;
extern event polygon_info_requested;

// Event variables that require synchronised access.

extern bool player_running;
extern bool selection_active;
extern bool absolute_motion;
extern int curr_mouse_x;
extern int curr_mouse_y;
extern float curr_move_delta;
extern float curr_side_delta;
extern float curr_move_rate;
extern float curr_turn_delta;
extern float curr_look_delta;
extern float curr_rotate_rate;
extern float master_brightness;
extern bool download_sounds;
extern bool reflections_enabled;
extern int visible_block_radius;
extern history_entry *selected_history_entry_ptr;
extern bool return_to_entrance;
extern bool URL_request_pending;
