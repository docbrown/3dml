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

// Current load index for textures and waves.

extern int curr_load_index;

// Externally visible functions.

void
update_logo(void);

block_def *
get_block_def(char block_symbol);

block_def *
get_block_def(word block_symbol);

block_def *
get_block_def(const char *block_identifier);

texture *
load_texture(blockset *blockset_ptr, char *texture_URL, bool unlimited_size);

char *
create_stream_URL(string stream_URL);

texture *
create_video_texture(char *name, video_rect *rect_ptr, bool unlimited_size);

wave *
load_wave(blockset *blockset_ptr, char *wave_URL);

entrance *
find_entrance(const char *name);

void
add_entrance(const char *name, int column, int row, int level, 
			 direction initial_direction, bool from_block);

imagemap *
find_imagemap(const char *name);

imagemap *
add_imagemap(const char *name);

bool
check_for_popup_selection(popup *popup_ptr, int popup_width, int popup_height);

trigger *
add_trigger_to_global_list(trigger *trigger_ptr, square *square_ptr, 
						   int column, int row, int level, bool from_block);

void
init_global_trigger(trigger *trigger_ptr);

void
set_trigger_delay(trigger *trigger_ptr, int curr_time_ms);

void
add_block(block_def *block_def_ptr, square *square_ptr, int column, int row,
		  int level, bool update_active_polygons, bool check_for_entrance);

void
remove_block(square *square_ptr, int column, int row, int level);

int
get_brightness_index(float brightness);

void
set_player_size(float x, float y, float z);

bool
create_player_block(void);

void
create_sprite_polygon(block_def *block_def_ptr, part *part_ptr);

void
init_spot(void);

void
request_URL(const char *URL, const char *file_path, const char *target,
			bool no_overlap);

bool
download_URL(const char *URL, const char *file_path);

void
update_texture_dependancies(texture *custom_texture_ptr);

void
initiate_first_download(void);

bool
handle_current_download(void);

void
identify_processor(void);
