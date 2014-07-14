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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "Classes.h"
#include "Image.h"
#include "Light.h"
#include "Main.h"
#include "Memory.h"
#include "Parser.h"
#include "Platform.h"
#include "Plugin.h"

// Current load index.

int curr_load_index;

// Relative coordinates of adjacent blocks checked in polygon visibility test.

static adj_block_column[6] = { 1, 0, 0, -1, 0, 0 };
static adj_block_row[6] = { 0, 1, 0, 0, -1, 0 };
static adj_block_level[6] = { 0, 0, 1, 0, 0, -1 };

// Time that last URL downloaded was requested, and flag indicating whether URL
// has been opened.

static int request_time_ms;
static bool URL_opened;

// URL of a pending download request that is directed to the plugin, not to
// another window frame.  Pending URLs are guarenteed not to overlap; if a
// new pending URL overrides an existing pending URL, the latter is to be
// ignored altogether.

static string pending_URL;

//------------------------------------------------------------------------------
// Update logo animation every 1/10th of a second, but only if the main
// window is ready.
//------------------------------------------------------------------------------

void
update_logo(void)
{
	if (main_window_ready) {
		int logo_time_ms = get_time_ms();
		if (logo_time_ms - last_logo_time_ms > 100) {
			last_logo_time_ms = logo_time_ms;
			display_frame_buffer(true, !seen_first_spot_URL);
		}
	}
}

//------------------------------------------------------------------------------
// Return a pointer to the block with the given single character symbol.
//------------------------------------------------------------------------------

block_def *
get_block_def(char block_symbol)
{	
	blockset *blockset_ptr;
	block_def *block_def_ptr;

	// Step through the blockset list looking for a block with the given
	// symbol.  Return a pointer to the first match.

	blockset_ptr = blockset_list_ptr->first_blockset_ptr;
	while (blockset_ptr) {
		if ((block_def_ptr = blockset_ptr->get_block_def(block_symbol)) != NULL)
			return(block_def_ptr);
		blockset_ptr = blockset_ptr->next_blockset_ptr;
	}
	return(NULL);
}

//------------------------------------------------------------------------------
// Return a pointer to the block with the given double character symbol.
//------------------------------------------------------------------------------

block_def *
get_block_def(word block_symbol)
{	
	blockset *blockset_ptr;
	block_def *block_def_ptr;

	// Step through the blockset list looking for a block with the given
	// symbol.  Return a pointer to the first match.

	blockset_ptr = blockset_list_ptr->first_blockset_ptr;
	while (blockset_ptr) {
		if ((block_def_ptr = blockset_ptr->get_block_def(block_symbol)) != NULL)
			return(block_def_ptr);
		blockset_ptr = blockset_ptr->next_blockset_ptr;
	}
	return(NULL);
}

//------------------------------------------------------------------------------
// Return a pointer to the block with the given name.  If the blockset name is
// prefixed to the block name, look in that blockset; otherwise search all
// blocksets and return the first match.
//------------------------------------------------------------------------------

block_def *
get_block_def(const char *block_identifier)
{
	string blockset_name, block_name;
	string new_block_name;
	blockset *blockset_ptr;
	block_def *block_def_ptr;

	// Parse the block identifier into a blockset name and block name.  If the
	// block name is invalid, return a NULL pointer.

	parse_identifier(block_identifier, blockset_name, block_name);
	if (!parse_name(block_name, &new_block_name, false))
		return(NULL);

	// If the blockset name is an empty string, search the blockset list for
	// the first block that matches the block name.

	if (strlen(blockset_name) == 0) {
		blockset_ptr = blockset_list_ptr->first_blockset_ptr;
		while (blockset_ptr) {
			if ((block_def_ptr = blockset_ptr->get_block_def(new_block_name))
				!= NULL)
				return(block_def_ptr);
			blockset_ptr = blockset_ptr->next_blockset_ptr;
		}
		return(NULL);
	}

	// If the blockset name is present, search the blockset list for the
	// blockset with that name, and search that blockset for a block that
	// matches the block name.

	else {
		blockset_ptr = blockset_list_ptr->first_blockset_ptr;
		while (blockset_ptr) {
			if (!stricmp(blockset_name, blockset_ptr->name))
				return(blockset_ptr->get_block_def(new_block_name));
			blockset_ptr = blockset_ptr->next_blockset_ptr;
		}
		return(NULL);
	}
}

//------------------------------------------------------------------------------
// Find a texture in the given blockset.  If the texture URL starts with a
// "@" character, it is interpreted as "blockset_name:texture_name" and that
// blockset is searched instead.
//------------------------------------------------------------------------------

static texture *
find_texture(blockset *blockset_ptr, const char *texture_URL, 
			 blockset *&new_blockset_ptr, string &new_texture_URL, 
			 bool unlimited_size)
{
	texture *texture_ptr;

	// If the texture URL begins with a "@", parse it as a texture from a
	// blockset.

	if (*texture_URL == '@') {
		string blockset_name, texture_name;

		// If the given blockset is not the custom blockset, this is an error.

		if (blockset_ptr != custom_blockset_ptr) {
			warning("Invalid texture URL %s", texture_URL);
			return(NULL);
		}

		// Skip over the "@" character, and parse the texture URL to obtain the
		// blockset name and texture name.

		texture_URL++;
		parse_identifier(texture_URL, blockset_name, texture_name);

		// If the blockset name is missing, select the first blockset by
		// default.  Otherwise search for the blockset with the given name.

		new_blockset_ptr = blockset_list_ptr->first_blockset_ptr;
		if (strlen(blockset_name) != 0) {
			while (new_blockset_ptr) {
				if (!stricmp(blockset_name, new_blockset_ptr->name))
					break;
				new_blockset_ptr = new_blockset_ptr->next_blockset_ptr;
			}
			if (new_blockset_ptr == NULL) {
				warning("Invalid block set name '%s'", blockset_name);
				return(NULL);
			}
		}

		// Create the new texture URL.

		new_texture_URL = create_URL(NULL, "textures/", texture_name);
	}

	// If the texture URL does not begin with a "@", parse it as a texture in
	// the given blockset, creating the new texture URL to match the blockset.

	else {
		new_blockset_ptr = blockset_ptr;
		if (new_blockset_ptr == custom_blockset_ptr)
			new_texture_URL = create_URL(spot_URL_dir, NULL, texture_URL);
		else
			new_texture_URL = create_URL(NULL, "textures/", texture_URL);
	}

	// Search for the new texture URL in the determined blockset, returning a
	// pointer to it if found.  If this is not a custom texture, unlimited_size
	// is FALSE and the texture has a width or height larger than 256 pixels,
	// then this is an error.

	texture_ptr = new_blockset_ptr->first_texture_ptr;
	while (texture_ptr) {
		if (!stricmp(new_texture_URL, texture_ptr->URL)) {
			if (new_blockset_ptr != custom_blockset_ptr && !unlimited_size &&
				(texture_ptr->width > 256 || texture_ptr->height > 256)) {
				warning("Texture has width or height greater than 256 pixels");
				return(NULL);
			}
			return(texture_ptr);
		}
		texture_ptr = texture_ptr->next_texture_ptr;
	}
	return(NULL);
}

//------------------------------------------------------------------------------
// Search for a scaled video texture for the given source rectangle.
//------------------------------------------------------------------------------

static texture *
find_scaled_video_texture(video_rect *rect_ptr)
{
	video_texture *video_texture_ptr;
	video_rect *source_rect_ptr;

	video_texture_ptr = scaled_video_texture_list;
	while (video_texture_ptr) {
		source_rect_ptr = &video_texture_ptr->source_rect;
		if (rect_ptr->x1_is_ratio == source_rect_ptr->x1_is_ratio &&
			rect_ptr->y1_is_ratio == source_rect_ptr->y1_is_ratio &&
			rect_ptr->x2_is_ratio == source_rect_ptr->x2_is_ratio &&
			rect_ptr->y2_is_ratio == source_rect_ptr->y2_is_ratio &&
			rect_ptr->x1 == source_rect_ptr->x1 &&
			rect_ptr->y1 == source_rect_ptr->y1 &&
			rect_ptr->x2 == source_rect_ptr->x2 &&
			rect_ptr->y2 == source_rect_ptr->y2)
			return(video_texture_ptr->texture_ptr);
		video_texture_ptr = video_texture_ptr->next_video_texture_ptr;
	}
	return(NULL);
}

//------------------------------------------------------------------------------
// Search for the named texture in the given blockset; if it doesn't exist,
// load the texture file and add the texture image to the given blockset.
//------------------------------------------------------------------------------

texture *
load_texture(blockset *blockset_ptr, char *texture_URL, bool unlimited_size)
{
	texture *texture_ptr;
	blockset *new_blockset_ptr;
	string new_texture_URL;

	// If a texture with the given URL is already in the given blockset, return
	// a pointer to it.

	if ((texture_ptr = find_texture(blockset_ptr, texture_URL, new_blockset_ptr,
		new_texture_URL, unlimited_size)) != NULL)
		return(texture_ptr);

	// If a blockset name was specified in the URL, but it was invalid, return
	// a NULL pointer.

	if (new_blockset_ptr == NULL)
		return(NULL);

	// Create the texture object.

	NEW(texture_ptr, texture);
	if (texture_ptr == NULL) { 
		memory_warning("texture");
		return(NULL);
	}

	// Load the image file into the texture object if it is not a custom
	// texture.  The loading of custom textures is deferred until after the 
	// spot file has been parsed.

	if (new_blockset_ptr != custom_blockset_ptr) {
		texture_ptr->custom = false;
	
		// If the originally requested blockset was the custom blockset,
		// we need to open the non-custom blockset first.  Otherwise the
		// non-custom blockset is already open because it's been parsed.

		if (blockset_ptr == custom_blockset_ptr &&
			!open_blockset(new_blockset_ptr->URL, new_blockset_ptr->name)) {
			DEL(texture_ptr, texture);
			texture_ptr = NULL;
		} else {

			// Load the image file.
		
			if (!load_image(NULL, new_texture_URL, texture_ptr,
				unlimited_size)) {
				DEL(texture_ptr, texture);
				texture_ptr = NULL;
			}

			// If the originally requested blockset was the custom blockset,
			// we need to close the non-custom blockset.  Otherwise we want
			// to keep the non-custom blockset open.

			if (blockset_ptr == custom_blockset_ptr)
				close_ZIP_archive();
		}
	} 
	
	// If it's a custom texture, just set it's load index.

	else {
		texture_ptr->custom = true;
		texture_ptr->load_index = curr_load_index;
		curr_load_index++;
	}

	// Return a pointer to the texture, after setting it's URL and adding it to 
	// the blockset it belongs to.

	if (texture_ptr) {
		texture_ptr->URL = new_texture_URL;
		new_blockset_ptr->add_texture(texture_ptr);
	}
	return(texture_ptr);
}

//------------------------------------------------------------------------------
// Create a stream URL from a partial or absolute URL.
//------------------------------------------------------------------------------

char *
create_stream_URL(string stream_URL)
{
	string spot_dir;

	// Create the path to the streaming media file.  We must use Internet
	// Explorer syntax for file URLs.

	if (!strnicmp(spot_URL_dir, "file:", 5)) {
		spot_dir = "file://";
		if (web_browser_ID == INTERNET_EXPLORER) {
			spot_dir += spot_URL_dir + 7;
			spot_dir[8] = ':';
		} else {
			spot_dir += spot_URL_dir + 8;
			spot_dir[8] = ':';
		}
	} else
		spot_dir = spot_URL_dir;
	return(create_URL(spot_dir, NULL, stream_URL));
}

//------------------------------------------------------------------------------
// Create a video texture for the given rectangle of the named stream.
//------------------------------------------------------------------------------

texture *
create_video_texture(char *name, video_rect *rect_ptr, bool unlimited_size)
{
	video_rect source_rect;
	texture *texture_ptr;

	// Make sure that the given name matches the stream name (if one is set).
	// If not, just issue a warning and return a NULL pointer.

	if (stricmp(name, name_of_stream)) {
		warning("There is no stream named '%s'", name);
		return(NULL);
	}

	// If a texture of unlimited size has been requested, and an unscaled
	// video texture already exists, return it.

	if (unlimited_size) {
		if (unscaled_video_texture_ptr)
			return(unscaled_video_texture_ptr);
	}

	// If a texture of limited size has been requested, and a scaled video
	// texture for the given source rectangle already exists, return it.
	// If no source rectangle was specified, use one that encompasses the
	// entire video frame.

	else {
		if (rect_ptr == NULL) {
			source_rect.x1_is_ratio = true;
			source_rect.y1_is_ratio = true;
			source_rect.x2_is_ratio = true;
			source_rect.y2_is_ratio = true;
			source_rect.x1 = 0.0f;
			source_rect.y1 = 0.0f;
			source_rect.x2 = 1.0f;
			source_rect.y2 = 1.0f;
			rect_ptr = &source_rect;
		}
		if ((texture_ptr = find_scaled_video_texture(rect_ptr)) != NULL)
			return(texture_ptr);
	}

	// Create a new custom texture object.

	NEW(texture_ptr, texture);
	if (texture_ptr == NULL) { 
		memory_warning("texture");
		return(NULL);
	}
	texture_ptr->custom = true;

	// If a texture of unlimited size was requested, it becomes the unscaled
	// video texture.

	if (unlimited_size)
		unscaled_video_texture_ptr = texture_ptr;

	// If a texture of limited size was requested, it becomes a new scaled
	// video texture.

	else {
		video_texture *scaled_video_texture_ptr;

		NEW(scaled_video_texture_ptr, video_texture);
		if (scaled_video_texture_ptr == NULL) {
			memory_warning("scaled video texture");
			return(NULL);
		}
		scaled_video_texture_ptr->texture_ptr = texture_ptr;
		scaled_video_texture_ptr->source_rect = *rect_ptr;
		scaled_video_texture_ptr->next_video_texture_ptr =
			scaled_video_texture_list;
		scaled_video_texture_list = scaled_video_texture_ptr;
	}

	// Return a pointer to the new texture object.

	return(texture_ptr);
}

//------------------------------------------------------------------------------
// Search for the wave with the given URL in the given blockset, and return a
// pointer to it if found.  If the wave URL starts with a "@" character, it is 
// interpreted as "blockset_name:wave_name" and that blockset is searched
// instead.
//------------------------------------------------------------------------------

static wave *
find_wave(blockset *blockset_ptr, const char *wave_URL, 
		  blockset *&new_blockset_ptr, string &new_wave_URL)
{
	wave *wave_ptr;

	// If the wave URL begins with a "@", parse it as a wave in a blockset.

	if (*wave_URL == '@') {
		string blockset_name, wave_name;

		// If the given blockset is not the custom blockset, this is an error.

		if (blockset_ptr != custom_blockset_ptr) {
			warning("Invalid wave URL %s", wave_URL);
			return(NULL);
		}

		// Skip over the "@" character, and parse the wave URL to obtain the
		// blockset name and wave name.

		wave_URL++;
		parse_identifier(wave_URL, blockset_name, wave_name);

		// If the blockset name is missing, select the first blockset by
		// default.  Otherwise search for the blockset with the given name.

		new_blockset_ptr = blockset_list_ptr->first_blockset_ptr;
		if (strlen(blockset_name) != 0) {
			while (new_blockset_ptr) {
				if (!stricmp(blockset_name, new_blockset_ptr->name))
					break;
				new_blockset_ptr = new_blockset_ptr->next_blockset_ptr;
			}
			if (new_blockset_ptr == NULL) {
				warning("Invalid block set name '%s'", blockset_name);
				return(NULL);
			}
		}

		// Create the new wave URL.

		new_wave_URL = create_URL(NULL, "sounds/", wave_name);
	}

	// If the wave URL does not begin with a "@", parse it as a wave in the
	// given blockset, creating the new wave URL to match the blockset.

	else {
		new_blockset_ptr = blockset_ptr;
		if (new_blockset_ptr == custom_blockset_ptr)
			new_wave_URL = create_URL(spot_URL_dir, NULL, wave_URL);
		else
			new_wave_URL = create_URL(NULL, "sounds/", wave_URL);
	}

	// Search for the new wave URL in the determined blockset, returning a
	// pointer to it if found.

	wave_ptr = new_blockset_ptr->first_wave_ptr;
	while (wave_ptr) {
		if (!stricmp(new_wave_URL, wave_ptr->URL))
			return(wave_ptr);
		wave_ptr = wave_ptr->next_wave_ptr;
	}
	return(NULL);
}

//------------------------------------------------------------------------------
// Search for the wave with the given URL in the given blockset; if not found,
// load the wave file and create a new wave.
//------------------------------------------------------------------------------

wave *
load_wave(blockset *blockset_ptr, char *wave_URL)
{
	wave *wave_ptr;
	blockset *new_blockset_ptr;
	string new_wave_URL;

	// If a wave with the given URL is already in the given blockset, return a
	// pointer to it.

	if ((wave_ptr = find_wave(blockset_ptr, wave_URL, new_blockset_ptr,
		new_wave_URL)) != NULL)
		return(wave_ptr);

	// If a blockset name was specified in the URL, but it was invalid, return
	// a NULL pointer.

	if (new_blockset_ptr == NULL)
		return(NULL);

	// Create the wave object.

	if ((wave_ptr = new wave) == NULL) { 
		memory_warning("wave");
		return(NULL);
	}

	// Load the wave file if it is not a custom wave.  The loading of custom 
	// waves is deferred until after the spot file has been parsed.

	if (new_blockset_ptr != custom_blockset_ptr) {
		wave_ptr->custom = false;

		// If the originally requested blockset was the custom blockset,
		// we need to open the non-custom blockset first.  Otherwise the
		// non-custom blockset is already open because it's been parsed.

		if (blockset_ptr == custom_blockset_ptr &&
			!open_blockset(new_blockset_ptr->URL, new_blockset_ptr->name)) {
			delete wave_ptr;
			wave_ptr = NULL;
		} else {

			// Load the wave file.

			if (!push_ZIP_file(new_wave_URL)) {
				delete wave_ptr;
				wave_ptr = NULL;
			} else {
				if (!load_wave_data(wave_ptr, top_file_ptr->file_buffer,
					top_file_ptr->file_size)) {
					delete wave_ptr;
					wave_ptr = NULL;
				}
				pop_file();
			}

			// If the originally requested blockset was the custom blockset,
			// we need to close the non-custom blockset.  Otherwise we want
			// to keep the non-custom blockset open.

			if (blockset_ptr == custom_blockset_ptr)
				close_ZIP_archive();
		}
	} 
	
	// If it's a custom wave, just set it's load index.

	else {
		wave_ptr->custom = true;
		wave_ptr->load_index = curr_load_index;
		curr_load_index++;
	}

	// Return a pointer to the wave, after setting it's URL and adding it to the
	// blockset it belongs to.

	if (wave_ptr) {
		wave_ptr->URL = new_wave_URL;
		new_blockset_ptr->add_wave(wave_ptr);
	}
	return(wave_ptr);
}

//------------------------------------------------------------------------------
// Search for an entrance with the given name, and return a pointer to it if
// found.
//------------------------------------------------------------------------------

entrance *
find_entrance(const char *name)
{
	entrance *entrance_ptr = global_entrance_list;
	while (entrance_ptr) {
		if (!stricmp(entrance_ptr->name, name))
			break;
		entrance_ptr = entrance_ptr->next_entrance_ptr;
	}
	return(entrance_ptr);
}

//------------------------------------------------------------------------------
// Add an entrance with the given name at the given location and with the
// given initial direction.
//------------------------------------------------------------------------------

void
add_entrance(const char *name, int column, int row, int level, 
			 direction initial_direction, bool from_block)
{
	bool entrance_is_new;
	entrance *entrance_ptr;

	// Search for an existing entrance of the same name; if not found, create a
	// new entrance.

	if ((entrance_ptr = find_entrance(name)) == NULL) {
		if ((entrance_ptr = new_entrance()) == NULL) {
			memory_warning("entrance");
			return;
		}
		entrance_is_new = true;
	} else
		entrance_is_new = false;

	// Add a new location to the entrance.

	if (!entrance_ptr->add_location(column, row, level, initial_direction, 
		from_block)) {
		if (entrance_is_new)
			del_entrance(entrance_ptr);
		return;
	}

	// If a new entrance was created, add it to the entrance list.

	if (entrance_is_new) {
		entrance_ptr->name = name;
		entrance_ptr->next_entrance_ptr = global_entrance_list;
		global_entrance_list = entrance_ptr;
	}
}

//------------------------------------------------------------------------------
// Search for an imagemap with the given name, and return a pointer to it if
// found.
//------------------------------------------------------------------------------

imagemap *
find_imagemap(const char *name)
{
	imagemap *imagemap_ptr = imagemap_list;
	while (imagemap_ptr) {
		if (!stricmp(imagemap_ptr->name, name))
			break;
		imagemap_ptr = imagemap_ptr->next_imagemap_ptr;
	}
	return(imagemap_ptr);
}

//------------------------------------------------------------------------------
// Add an imagemap with the given name to the imagemap list.
//------------------------------------------------------------------------------

imagemap *
add_imagemap(const char *name)
{
	imagemap *imagemap_ptr;

	// Search for an existing imagemap of the same name; if found, this is an
	// error.

	if ((imagemap_ptr = find_imagemap(name)) != NULL) {
		warning("Duplicate imagemap name '%s'", name);
		return(NULL);
	}

	// Create a new imagemap and initialise it.

	if ((imagemap_ptr = new imagemap) == NULL) {
		memory_warning("imagemap");
		return(NULL);
	}
	imagemap_ptr->name = name;

	// Add the imagemap to the imagemap list.

	imagemap_ptr->next_imagemap_ptr = imagemap_list;
	imagemap_list = imagemap_ptr;
	return(imagemap_ptr);
}

//------------------------------------------------------------------------------
// Determine if the mouse is currently pointing at an area within the imagemap
// of the given popup, if there is one.  Returns TRUE if the popup is at least
// selected.
//------------------------------------------------------------------------------

bool
check_for_popup_selection(popup *popup_ptr, int popup_width, int popup_height)
{
	int delta_mouse_x, delta_mouse_y;
	imagemap *imagemap_ptr;
	area *area_ptr;

	// If the current mouse position is outside of the popup rectangle, it
	// isn't selected.

	delta_mouse_x = mouse_x - popup_ptr->sx;
	delta_mouse_y = mouse_y - popup_ptr->sy;
	if (delta_mouse_x < 0 || delta_mouse_x >= popup_width ||
		delta_mouse_y < 0 || delta_mouse_y >= popup_height)
		return(false);

	// The popup is selected.  However, if it doesn't have an imagemap there
	// is nothing else to do.

	if (popup_ptr->imagemap_ptr == NULL)
		return(true);

	// Step through the areas of the popup's imagemap...

	imagemap_ptr = popup_ptr->imagemap_ptr;
	area_ptr = imagemap_ptr->area_list;
	while (area_ptr) {
		rect *rect_ptr;
		circle *circle_ptr;
		float delta_x, delta_y, distance;

		// If the current mouse position is inside this area's rectangle or
		// circle, make this area the selected one.

		switch (area_ptr->shape_type) {
		case RECT_SHAPE:
			rect_ptr = &area_ptr->rect_shape;
			if (delta_mouse_x >= rect_ptr->x1 && delta_mouse_x < rect_ptr->x2 &&
				delta_mouse_y >= rect_ptr->y1 && delta_mouse_y < rect_ptr->y2) {
				curr_area_square_ptr = popup_ptr->square_ptr;
				curr_selected_exit_ptr = area_ptr->exit_ptr;
				curr_selected_area_ptr = area_ptr;
				return(true);
			}
			break;
		case CIRCLE_SHAPE:
			circle_ptr = &area_ptr->circle_shape;
			delta_x = (float)(delta_mouse_x - circle_ptr->x);
			delta_y = (float)(delta_mouse_y - circle_ptr->y);
			distance = (float)sqrt(delta_x * delta_x + delta_y * delta_y);
			if (distance <= circle_ptr->r) {
				curr_selected_area_ptr = area_ptr;
				curr_area_square_ptr = popup_ptr->square_ptr;
				curr_selected_exit_ptr = area_ptr->exit_ptr;
				return(true);
			}
		}

		// Move onto the next area.

		area_ptr = area_ptr->next_area_ptr;
	}

	// The popup was selected, but no area was active.

	return(true);
}

//------------------------------------------------------------------------------
// Initialise a sprite's collision box.
//------------------------------------------------------------------------------

static void
init_sprite_collision_box(block *block_ptr)
{	
	int vertices, vertex_no;
	vertex *vertex_list, *vertex_ptr;
	float min_x, min_y;
	float max_x, max_y;
	float z, min_z, max_z;
	float x_radius;

	// Initialise the minimum and maximum coordinates to the first vertex.
 
	vertices = block_ptr->vertices;
	vertex_list = block_ptr->vertex_list;
	vertex_ptr = &vertex_list[0];
	min_x = vertex_ptr->x;
	min_y = vertex_ptr->y;
	max_x = vertex_ptr->x;
	max_y = vertex_ptr->y;
	z = vertex_ptr->z - block_ptr->translation.z;

	// Step through the remaining vertices and remember the minimum and
	// maximum X and Y coordinates.

	for (vertex_no = 1; vertex_no < vertices; vertex_no++) {
		vertex_ptr = &vertex_list[vertex_no];
		if (vertex_ptr->x < min_x)
			min_x = vertex_ptr->x;
		else if (vertex_ptr->x > max_x)
			max_x = vertex_ptr->x;
		if (vertex_ptr->y < min_y)
			min_y = vertex_ptr->y;
		else if (vertex_ptr->y > max_y)
			max_y = vertex_ptr->y;
	}

	// Make the minimum and maximum X and Y coordinates relative to the
	// sprite block's translation.

	min_x -= block_ptr->translation.x;
	max_x -= block_ptr->translation.x;
	min_y -= block_ptr->translation.y;
	max_y -= block_ptr->translation.y;

	// Generate the minimum and maximum Z coordinates from the minimum and
	// maximum X coordinates.

	x_radius = (max_x - min_x) / 2.0f;
	min_z = z - x_radius;
	max_z = z + x_radius;

	// Create a collision mesh from the collision box determined above.
	
	COL_convertSpriteToColMesh(block_ptr->col_mesh_ptr, min_x, min_y, min_z,
		max_x, max_y, max_z);
}

//------------------------------------------------------------------------------
// Initialise a polygon for a sprite that is centred in the block and facing
// north.  The polygon is sized to match the texture dimensions.
//------------------------------------------------------------------------------

static void
init_sprite_polygon(block *block_ptr, polygon *polygon_ptr, part *part_ptr)
{
	block_def *block_def_ptr;
	size *size_ptr;
	float size_x, size_y;
	float half_size_x, half_size_y;
	float y_offset;
	texture *texture_ptr;
	vertex *vertex_ptr;
	vertex_def *vertex_def_ptr;
	vertex block_centre;

	// Get a pointer to the block definition and sprite size.

	block_def_ptr = block_ptr->block_def_ptr;
	size_ptr = &block_def_ptr->sprite_size;

	// If the block definition defines a size, use it.

	if (size_ptr->width > 0 && size_ptr->height > 0) {
		size_x = (float)size_ptr->width / TEXELS_PER_UNIT * 
			world_ptr->block_scale;
		size_y = (float)size_ptr->height / TEXELS_PER_UNIT * 
			world_ptr->block_scale;
	}

	// If the sprite has a texture, use the size of the texture as the size of
	// the sprite.  Otherwise make the size of the sprite to be 256x256 texels.

	else {
		texture_ptr = part_ptr->texture_ptr;
		if (texture_ptr) {
			size_x = (float)texture_ptr->width / TEXELS_PER_UNIT * 
				world_ptr->block_scale;
			size_y = (float)texture_ptr->height / TEXELS_PER_UNIT * 
				world_ptr->block_scale;
		} else {
			size_x = world_ptr->block_units;
			size_y = world_ptr->block_units;
		}
	}

	// Compute half the width and height.

	half_size_x = size_x / 2.0f;
	half_size_y = size_y / 2.0f;

	// Compute the centre of the sprite block.

	block_centre.x = block_ptr->translation.x + world_ptr->half_block_units;
	block_centre.y = block_ptr->translation.y + world_ptr->half_block_units;
	block_centre.z = block_ptr->translation.z + world_ptr->half_block_units;

	// Determine the Y offset of the polygon based upon the sprite alignment.

	switch (block_def_ptr->sprite_alignment) {
	case TOP:
		y_offset = block_centre.y + world_ptr->half_block_units - half_size_y;
		break;
	case CENTRE:
		y_offset = block_centre.y;
		break;
	case BOTTOM:
		y_offset = block_ptr->translation.y + half_size_y;
	}

	// Get pointers to the vertex and vertex definition lists.

	PREPARE_VERTEX_LIST(block_ptr);
	PREPARE_VERTEX_DEF_LIST(polygon_ptr);

	// Initialise the vertex and texture coordinates of the polygon, making
	// sure the vertices are scaled.

	vertex_ptr = &vertex_list[0];
	vertex_def_ptr = &vertex_def_list[0];
	vertex_ptr->x = block_centre.x + half_size_x;
	vertex_ptr->y = y_offset + half_size_y;
	vertex_ptr->z = block_centre.z;
	vertex_def_ptr->u = 0.0f;
	vertex_def_ptr->v = 0.0f;
	vertex_ptr = &vertex_list[1];
	vertex_def_ptr = &vertex_def_list[1];
	vertex_ptr->x = block_centre.x - half_size_x;
	vertex_ptr->y = y_offset + half_size_y;
	vertex_ptr->z = block_centre.z;
	vertex_def_ptr->u = 1.0f;
	vertex_def_ptr->v = 0.0f;
	vertex_ptr = &vertex_list[2];
	vertex_def_ptr = &vertex_def_list[2];
	vertex_ptr->x = block_centre.x - half_size_x;
	vertex_ptr->y = y_offset - half_size_y;
	vertex_ptr->z = block_centre.z;
	vertex_def_ptr->u = 1.0f;
	vertex_def_ptr->v = 1.0f;
	vertex_ptr = &vertex_list[3];
	vertex_def_ptr = &vertex_def_list[3];
	vertex_ptr->x = block_centre.x + half_size_x;
	vertex_ptr->y = y_offset - half_size_y;
	vertex_ptr->z = block_centre.z;
	vertex_def_ptr->u = 0.0f;
	vertex_def_ptr->v = 1.0f;

	// Initialise the sprite polygon's centroid, normal vector and plane 
	// offset.

	polygon_ptr->compute_centroid(vertex_list);
	polygon_ptr->compute_normal_vector(vertex_list);
	polygon_ptr->compute_plane_offset(vertex_list);

	// Initialise the sprite's collision box if the sprite is solid.

	if (block_ptr->solid)
		init_sprite_collision_box(block_ptr);
}

//------------------------------------------------------------------------------
// Create a new block, which is a translated version of a block definition.
//------------------------------------------------------------------------------

static block *
create_new_block(block_def *block_def_ptr, vertex translation)		 
{
	block *block_ptr;

	// Create the new block structure, and initialise the block definition
	// pointer, sprite angle, time block was put on map, time of last rotational
	// change, solid flag, scaled block translation and polygon list.

	if ((block_ptr = block_def_ptr->new_block()) == NULL)
		memory_error("block");
	block_ptr->block_def_ptr = block_def_ptr;
	block_ptr->sprite_angle = block_def_ptr->sprite_angle;
	block_ptr->start_time_ms = curr_time_ms;
	block_ptr->last_time_ms = 0;
	block_ptr->solid = block_def_ptr->solid;
	block_ptr->translation = translation * world_ptr->block_scale;
	block_ptr->polygon_list = block_def_ptr->polygon_list;

	// The vertex list is new.  If the block is a structural block, copy the
	// vertices from the block definition and translate and scale them.  If the
	// block is a sprite, initialise the vertices using the texture dimensions,
	// then scale them.

	if (block_def_ptr->type == STRUCTURAL_BLOCK) {
		for (int vertex_no = 0; vertex_no < block_ptr->vertices; vertex_no++)
			block_ptr->vertex_list[vertex_no] = 
				(block_def_ptr->vertex_list[vertex_no] + translation) *
				world_ptr->block_scale;
		if (block_ptr->solid)
			COL_convertBlockToColMesh(block_ptr->col_mesh_ptr, block_def_ptr);
	} else {
		polygon *polygon_ptr = block_def_ptr->polygon_list;
		part *part_ptr = polygon_ptr->part_ptr;
		init_sprite_polygon(block_ptr, polygon_ptr, part_ptr);
	}

	// Return a pointer to the new block.

	return(block_ptr);
}

//------------------------------------------------------------------------------
// Set the delay for a "timer" trigger.
//------------------------------------------------------------------------------

void
set_trigger_delay(trigger *trigger_ptr, int curr_time_ms)
{
	// Set the start time to the current time.

	trigger_ptr->start_time_ms = curr_time_ms;

	// Generate a random delay time in the allowable range.

	trigger_ptr->delay_ms = trigger_ptr->delay_range.min_delay_ms;
	if (trigger_ptr->delay_range.delay_range_ms > 0)
		trigger_ptr->delay_ms += (int)((float)rand() / RAND_MAX * 
			trigger_ptr->delay_range.delay_range_ms);
}


//------------------------------------------------------------------------------
// Add a copy of the given trigger to the global trigger list.
//------------------------------------------------------------------------------

trigger *
add_trigger_to_global_list(trigger *trigger_ptr, square *square_ptr, 
						   int column, int row, int level, bool from_block)
{
	trigger *new_trigger_ptr;

	// Duplicate the trigger, and add it to the end of the global trigger list.
	
	new_trigger_ptr = dup_trigger(trigger_ptr);
	if (new_trigger_ptr) {

		// Initialise the trigger.

		new_trigger_ptr->from_block = from_block;
		new_trigger_ptr->square_ptr = square_ptr;
		new_trigger_ptr->position.set_scaled_map_position(column, row, level);
		new_trigger_ptr->next_trigger_ptr = NULL;

		// Add the trigger to the end of the global trigger list.

		if (last_global_trigger_ptr)
			last_global_trigger_ptr->next_trigger_ptr = new_trigger_ptr;
		else
			global_trigger_list = new_trigger_ptr;
		last_global_trigger_ptr = new_trigger_ptr;
	}
	return(new_trigger_ptr);
}

//------------------------------------------------------------------------------
// Initialise the state of a "step in", "step out" or "timer" trigger.
//------------------------------------------------------------------------------

void
init_global_trigger(trigger *trigger_ptr)
{
	vector distance;
	float distance_squared;

	switch (trigger_ptr->trigger_flag) {
	case STEP_IN:
	case STEP_OUT:
		distance = trigger_ptr->position - player_viewpoint.position;
		distance_squared = distance.dx * distance.dx + 
			distance.dy * distance.dy + distance.dz * distance.dz;
		trigger_ptr->previously_inside = 
			distance_squared <= trigger_ptr->radius_squared;
		break;
	case TIMER:
		set_trigger_delay(trigger_ptr, curr_time_ms);
	}
}

//------------------------------------------------------------------------------
// Update the given square with a given block.
//------------------------------------------------------------------------------

static void
update_square(square *square_ptr, block_def *block_def_ptr, block *block_ptr,
			  int column, int row, int level, vertex translation)
{
	hyperlink *exit_ptr;
	trigger *trigger_ptr, *new_trigger_ptr;

	// Store the block pointer, trigger flags and trigger list in the square.

	square_ptr->block_ptr = block_ptr;
	square_ptr->block_trigger_flags = block_def_ptr->trigger_flags;
	square_ptr->block_trigger_list = block_def_ptr->trigger_list;

	// If there is a light for this block definition, add a translated and
	// scaled version to the global light list.

	if (block_def_ptr->light_ptr) {
		light *light_ptr;

		if ((light_ptr = dup_light(block_def_ptr->light_ptr)) == NULL)
			memory_warning("light");
		else {
			light_ptr->square_ptr = square_ptr;
			light_ptr->from_block = true;
			light_ptr->pos = (light_ptr->pos + translation) *
				world_ptr->block_scale;
			light_ptr->next_light_ptr = global_light_list;
			global_light_list = light_ptr;
			global_lights++;
		}
	}

	// If there is a sound for this block definition, add a translated and
	// scaled version to the sound list, and store a pointer to the sound
	// in the block.

	if (block_def_ptr->sound_ptr) {
		sound *sound_ptr;
		wave *wave_ptr;

		if ((sound_ptr = dup_sound(block_def_ptr->sound_ptr)) == NULL)
			memory_warning("sound");
		else {

			// Initialise sound object.

			sound_ptr->square_ptr = square_ptr;
			sound_ptr->from_block = true;
			sound_ptr->in_range = false;
			sound_ptr->position = (sound_ptr->position + translation) *
				world_ptr->block_scale;

			// Add sound to global sound list, and set a flag to indicate that
			// the global sound list has changed.

			sound_ptr->next_sound_ptr = global_sound_list;
			global_sound_list = sound_ptr;
			global_sound_list_changed = true;

			// Create sound buffer, if sound is a non-streaming sound and the
			// wave has been loaded.

			wave_ptr = sound_ptr->wave_ptr;
			if (!sound_ptr->streaming && wave_ptr != NULL && 
				wave_ptr->data_ptr != NULL)
				create_sound_buffer(sound_ptr);

			// Increment the number of sounds on this square.

			square_ptr->sounds++;
		}
	}

	// If there is a popup for this block definition, add a translated and
	// scaled version to the end of the popup list, and store a pointer to
	// the popup in the block.

	if (block_def_ptr->popup_ptr) {
		popup *popup_ptr;

		if ((popup_ptr = dup_popup(block_def_ptr->popup_ptr)) == NULL)
			memory_warning("popup");
		else {

			// Initialise popup.

			popup_ptr->square_ptr = square_ptr;
			popup_ptr->from_block = true;
			popup_ptr->position = (popup_ptr->position + translation) *
				world_ptr->block_scale;
			popup_ptr->next_popup_ptr = NULL;

			// Add the popup to the end of the global popup list.

			if (last_global_popup_ptr)
				last_global_popup_ptr->next_popup_ptr = popup_ptr;
			else
				global_popup_list = popup_ptr;
			last_global_popup_ptr = popup_ptr;

			// Add the popup to the end of the square's popup list.

			if (square_ptr->last_popup_ptr)
				square_ptr->last_popup_ptr->next_square_popup_ptr = 
					popup_ptr;
			else
				square_ptr->popup_list = popup_ptr;
			square_ptr->last_popup_ptr = popup_ptr;

			// Update the square's popup triggers.

			square_ptr->popup_trigger_flags |= popup_ptr->trigger_flags;
		}
	}

	// If there is an entrance for this block definition, add it to the
	// entrance list.

	if (strlen(block_def_ptr->entrance_name) != 0)
		add_entrance(block_def_ptr->entrance_name, column, row, level,
			block_def_ptr->initial_direction, true);

	// If there is an exit reference for this block definition, add it to the
	// square if it doesn't already have one.

	if (block_def_ptr->exit_ptr != NULL && square_ptr->exit_ptr == NULL) {
		if ((exit_ptr = dup_hyperlink(block_def_ptr->exit_ptr)) == NULL)
			memory_warning("exit");
		else {
			exit_ptr->from_block = true;
			square_ptr->exit_ptr = exit_ptr;
		}
	}

	// If there are "step in", "step out", "proximity", "timer" or "location"
	// triggers for this block definition, add copies of them to the global
	// trigger list and initialise their states.

	trigger_ptr = block_def_ptr->trigger_list;
	while (trigger_ptr) {
		if ((trigger_ptr->trigger_flag == STEP_IN ||
			trigger_ptr->trigger_flag == STEP_OUT ||
			trigger_ptr->trigger_flag == PROXIMITY ||
			trigger_ptr->trigger_flag == TIMER ||
			trigger_ptr->trigger_flag == LOCATION) &&
			(new_trigger_ptr = add_trigger_to_global_list(trigger_ptr,
			 square_ptr, column, row, level, true)) != NULL)
			init_global_trigger(new_trigger_ptr);
		trigger_ptr = trigger_ptr->next_trigger_ptr;
	}
}

//------------------------------------------------------------------------------
// Set the player's size.
//------------------------------------------------------------------------------

void
set_player_size(float x, float y, float z)
{
	float half_y = y / 2.0f;
	player_size.set(x, y, z);
	VEC_set(&player_collision_box.maxDim, x / 2.0f, half_y, z / 2.0f);
	VEC_set(&player_collision_box.offsToCentre, 0.0f, half_y, 0.0f);
	player_step_height = half_y;
}

//------------------------------------------------------------------------------
// Initialise the player's collision box.
//------------------------------------------------------------------------------

void
init_player_collision_box(void)
{	
	int vertices, vertex_no;
	vertex *vertex_list, *vertex_ptr;
	float min_x, min_y;
	float max_x, max_y;

	// Initialise the minimum and maximum coordinates to the first vertex.
 
	vertices = player_block_ptr->vertices;
	vertex_list = player_block_ptr->vertex_list;
	vertex_ptr = &vertex_list[0];
	min_x = vertex_ptr->x;
	min_y = vertex_ptr->y;
	max_x = vertex_ptr->x;
	max_y = vertex_ptr->y;

	// Step through the remaining vertices and remember the minimum and
	// maximum X and Y coordinates.  We don't bother with the minimum and
	// maximum Z coordinates because they will be the same.

	for (vertex_no = 1; vertex_no < vertices; vertex_no++) {
		vertex_ptr = &vertex_list[vertex_no];
		if (vertex_ptr->x < min_x)
			min_x = vertex_ptr->x;
		else if (vertex_ptr->x > max_x)
			max_x = vertex_ptr->x;
		if (vertex_ptr->y < min_y)
			min_y = vertex_ptr->y;
		else if (vertex_ptr->y > max_y)
			max_y = vertex_ptr->y;
	}

	// Set the player's size.

	set_player_size(max_x - min_x, max_y - min_y, max_x - min_x);
}

//------------------------------------------------------------------------------
// Create the player block.
//------------------------------------------------------------------------------

bool
create_player_block(void)
{
	block_def *block_def_ptr;
	vertex centre_translation(-UNITS_PER_HALF_BLOCK, -UNITS_PER_HALF_BLOCK, 
		-UNITS_PER_HALF_BLOCK);

	// If the player block symbol is undefined, don't create the player block.

	switch (world_ptr->map_style) {
	case SINGLE_MAP:
		if ((block_def_ptr = custom_blockset_ptr->get_block_def(
			(char)player_block_symbol)) == NULL && (block_def_ptr = 
			get_block_def((char)player_block_symbol)) == NULL) {
			warning("Undefined block symbol '%c'", (char)player_block_symbol);
			return(false);
		}
		break;
	case DOUBLE_MAP:
		if ((block_def_ptr = custom_blockset_ptr->get_block_def(
			player_block_symbol)) == NULL && (block_def_ptr = 
			get_block_def(player_block_symbol)) == NULL) {
			warning("Undefined block symbol '%c%c'", 
				player_block_symbol >> 7, player_block_symbol & 127);
			return(false);
		}
	}

	// If the player block definition is not a player sprite, don't create the
	// player block.

	if (block_def_ptr->type != PLAYER_SPRITE) {
		switch (world_ptr->map_style) {
		case SINGLE_MAP:
			warning("Block '%c' is not a player sprite", player_block_symbol);
			break;
		case DOUBLE_MAP:
			warning("Block '%c%c' is not a player sprite", 
				player_block_symbol >> 7, player_block_symbol & 127);
		}
		return(false);
	}

	// Create the player block with a translation to put the centre at the
	// origin.

	player_block_ptr = create_new_block(block_def_ptr, centre_translation);
	init_player_collision_box();
	return(true);
}

//------------------------------------------------------------------------------
// Determine if two polygons are identical (share the same vertices).
//------------------------------------------------------------------------------

static bool
polygons_identical(block *block1_ptr, polygon *polygon1_ptr,
				   block *block2_ptr, polygon *polygon2_ptr)
{
	// If the polygons don't have the same number of vertices, they are
	// different.

	if (polygon1_ptr->vertices != polygon2_ptr->vertices)
		return(false);

	// Otherwise we must compare each vertex in polygon #1 with the vertices
	// in polygon #2.  If all match, the polygons are identical.

	vertex *vertex_list1 = block1_ptr->vertex_list;
	vertex *vertex_list2 = block2_ptr->vertex_list;
	vertex_def *vertex_def_list1 = polygon1_ptr->vertex_def_list;
	vertex_def *vertex_def_list2 = polygon2_ptr->vertex_def_list;
	bool total_match = true;
	for (int index1 = 0; index1 < polygon1_ptr->vertices; index1++) {
		vertex *vertex1_ptr = &vertex_list1[vertex_def_list1[index1].vertex_no];
		bool match = false;
		for (int index2 = 0; index2 < polygon2_ptr->vertices; index2++) {
			vertex *vertex2_ptr = 
				&vertex_list2[vertex_def_list2[index2].vertex_no];
			if (*vertex1_ptr == *vertex2_ptr) {
				match = true;
				break;
			}
		}
		if (!match) {
			total_match = false;
			break;
		}
	}
	return(total_match);
}

//------------------------------------------------------------------------------
// Determine which polygons in a block and it's adjacent blocks are active.
// Pairs of one-sided polygons that share the same vertices and face each other
// can be deactivated safely.  Zero-sided polygons are *always* deactivated,
// and two-sided polygons are *never* deactivated.
//------------------------------------------------------------------------------

static int
compute_active_polygons(block *block_ptr, int column, int row, int level,
						bool check_all_sides)
{
	int inactive_polygons;
	int adj_direction;
	int adj_column, adj_row, adj_level;
	block *adj_block_ptr;
	polygon *polygon_ptr, *adj_polygon_ptr;
	bool *polygon_active_ptr, *adj_polygon_active_ptr;
	part *part_ptr, *adj_part_ptr;
	int polygon_no, adj_polygon_no;

	// Step through the polygons of the center block, deactivating zero-sided
	// polygons and ignoring two-sided polygons.  If we're not checking all
	// sides of the center block, only one-sided polygons facing east, south or
	// up are processed further.
	
	inactive_polygons = 0;
	for (polygon_no = 0; polygon_no < block_ptr->polygons; polygon_no++) {
		polygon_ptr = &block_ptr->polygon_list[polygon_no];
		polygon_active_ptr = &block_ptr->polygon_active_list[polygon_no];
		part_ptr = polygon_ptr->part_ptr;
		switch (part_ptr->faces) {
		case 0:
			*polygon_active_ptr = false;
			inactive_polygons++;
			continue;
		case 1:
			if (!polygon_ptr->side || 
				(!check_all_sides && polygon_ptr->direction >= 3))
				continue;
			break;
		case 2:
			continue;
		}

		// Get a pointer to the block adjacent to this side polygon.  If it
		// doesn't exist, the side polygon remains active.

		adj_column = column + adj_block_column[polygon_ptr->direction];
		adj_row = row + adj_block_row[polygon_ptr->direction];
		adj_level = level + adj_block_level[polygon_ptr->direction];
		adj_block_ptr = world_ptr->get_block_ptr(adj_column, adj_row, adj_level);
		if (!adj_block_ptr)
			continue;

		// Step through the side polygons of the adjacent block that face
		// towards the side polygon of the center block, and are one-sided.

		adj_direction = (polygon_ptr->direction + 3) % 6;
		for (adj_polygon_no = 0; adj_polygon_no < adj_block_ptr->polygons;
			adj_polygon_no++) {
			adj_polygon_ptr = &adj_block_ptr->polygon_list[adj_polygon_no];
			adj_polygon_active_ptr = 
				&adj_block_ptr->polygon_active_list[adj_polygon_no];
			adj_part_ptr = adj_polygon_ptr->part_ptr;
			if (adj_part_ptr->faces != 1 || !adj_polygon_ptr->side || 
				adj_polygon_ptr->direction != adj_direction)
				continue;

			// Compare the current side polygon with the adjacent side polygon.
			// If their vertices match, we make both polygons inactive; we then
			// break out of the loop since no other adjacent polygon will match
			// the current one.

			if (polygons_identical(block_ptr, polygon_ptr, adj_block_ptr,
				adj_polygon_ptr)) {
					*polygon_active_ptr = false;
					*adj_polygon_active_ptr = false;
					inactive_polygons += 2;
				break;
			}
		}
	}
	return(inactive_polygons);
}

//------------------------------------------------------------------------------
// Reset the state of side polygons adjacent to this block.
//------------------------------------------------------------------------------

static void
reset_active_polygons(int column, int row, int level)
{
	int direction, adj_direction;
	int adj_column, adj_row, adj_level;
	block *adj_block_ptr;
	polygon *adj_polygon_ptr;
	bool *adj_polygon_active_ptr;
	part *adj_part_ptr;
	int adj_polygon_no;

	// Step through the six possible directions.
	
	for (direction = 0; direction < 6; direction++) {

		// Get a pointer to the adjacent block in this direction.  If it 
		// doesn't exist, continue.

		adj_column = column + adj_block_column[direction];
		adj_row = row + adj_block_row[direction];
		adj_level = level + adj_block_level[direction];
		adj_block_ptr = world_ptr->get_block_ptr(adj_column, adj_row, adj_level);
		if (!adj_block_ptr)
			continue;

		// Step through the side polygons of the adjacent block that face
		// towards the center block and are one-sided, and make them active.

		adj_direction = (direction + 3) % 6;
		for (adj_polygon_no = 0; adj_polygon_no < adj_block_ptr->polygons;
			adj_polygon_no++) {
			adj_polygon_ptr = &adj_block_ptr->polygon_list[adj_polygon_no];
			adj_polygon_active_ptr = 
				&adj_block_ptr->polygon_active_list[adj_polygon_no];
			adj_part_ptr = adj_polygon_ptr->part_ptr;
			if (adj_polygon_ptr->side && adj_part_ptr->faces == 1 &&
				adj_polygon_ptr->direction == adj_direction)
				*adj_polygon_active_ptr = true;
		}
	}
}

//------------------------------------------------------------------------------
// Add a new block to the map at the given location, using the given block
// definition as the template.
//------------------------------------------------------------------------------

void
add_block(block_def *block_def_ptr, square *square_ptr, int column, int row,
		  int level, bool update_active_polygons, bool check_for_entrance)
{
	entrance *entrance_ptr;
	location *location_ptr;
	vertex translation;
	block *block_ptr;

	// If the block definition does not permit an entrance, step through the
	// global entrance list looking for an entrance on this square; if found,
	// don't add this block.

	if (check_for_entrance && !block_def_ptr->allow_entrance) {
		entrance_ptr = global_entrance_list;
		while (entrance_ptr) {
			location_ptr = entrance_ptr->location_list;
			while (location_ptr) {
				if (column == location_ptr->column &&
					row == location_ptr->row && level == location_ptr->level) {
					warning("Block '%s' cannot be placed on an entrance "
						"square", block_def_ptr->name);
					return;
				}
				location_ptr = location_ptr->next_location_ptr;
			}
			entrance_ptr = entrance_ptr->next_entrance_ptr;
		}
	}
	
	// Compute the translation to place the new block at the given location on
	// the map, then create the block.

	translation.set_map_translation(column, row, level);
	block_ptr = create_new_block(block_def_ptr, translation);

	// If the block is movable, add it to the movable block list, otherwise add
	// it to the square.

	if (block_def_ptr->movable) {
		block_ptr->next_block_ptr = movable_block_list;
		movable_block_list = block_ptr;
	} else
		update_square(square_ptr, block_def_ptr, block_ptr, column, row, level,
			translation);

	// If the active polygons of the new block and adjacent blocks must be
	// updated, do so.

	if (update_active_polygons)
		compute_active_polygons(block_ptr, column, row, level, true);
}

//------------------------------------------------------------------------------
// Remove a block from the given square.
//------------------------------------------------------------------------------

void
remove_block(square *square_ptr, int column, int row, int level)
{
	block_def *block_def_ptr;
	block *block_ptr;
	trigger *prev_trigger_ptr, *trigger_ptr;
	light *prev_light_ptr, *light_ptr;
	sound *prev_sound_ptr, *sound_ptr;
	popup *prev_popup_ptr, *popup_ptr;
	entrance *prev_entrance_ptr, *entrance_ptr;

	// Get a pointer to the block at this location.  If there is no block,
	// there is nothing to do.

	block_ptr = square_ptr->block_ptr;
	if (block_ptr == NULL)
		return;

	// Reset the active polygons adjacent to this block.

	reset_active_polygons(column, row, level);

	// Step through the global trigger list, and remove all triggers that came
	// from this block.

	prev_trigger_ptr = NULL;
	trigger_ptr = global_trigger_list;
	while (trigger_ptr) {
		if (trigger_ptr->from_block && trigger_ptr->square_ptr == square_ptr) {
			trigger_ptr->action_list = NULL;
			trigger_ptr = del_trigger(trigger_ptr);
			if (prev_trigger_ptr) {
				prev_trigger_ptr->next_trigger_ptr = trigger_ptr;
				if (trigger_ptr == NULL)
					last_global_trigger_ptr = prev_trigger_ptr;
			} else {
				global_trigger_list = trigger_ptr;
				if (trigger_ptr == NULL)
					last_global_trigger_ptr = NULL;
			}
		} else {
			prev_trigger_ptr = trigger_ptr;
			trigger_ptr = trigger_ptr->next_trigger_ptr;
		}
	}

	// Step through the global light list, and remove all lights that came from
	// this block.

	prev_light_ptr = NULL;
	light_ptr = global_light_list;
	while (light_ptr) {
		if (light_ptr->from_block && light_ptr->square_ptr == square_ptr) {
			light_ptr = del_light(light_ptr);
			global_lights--;
			if (prev_light_ptr)
				prev_light_ptr->next_light_ptr = light_ptr;
			else
				global_light_list = light_ptr;
		} else {
			prev_light_ptr = light_ptr;
			light_ptr = light_ptr->next_light_ptr;
		}
	}

	// Step through the global sound list, and remove all sounds that came from
	// this block.

	prev_sound_ptr = NULL;
	sound_ptr = global_sound_list;
	while (sound_ptr) {
		if (sound_ptr->from_block && sound_ptr->square_ptr == square_ptr) {
			destroy_sound_buffer(sound_ptr);
			sound_ptr = del_sound(sound_ptr);
			square_ptr->sounds--;
			global_sound_list_changed = true;
			if (prev_sound_ptr)
				prev_sound_ptr->next_sound_ptr = sound_ptr;
			else
				global_sound_list = sound_ptr;
		} else {
			prev_sound_ptr = sound_ptr;
			sound_ptr = sound_ptr->next_sound_ptr;
		}
	}

	// Step through the square's popup list, and remove all popups that came
	// from this block from the list (but don't delete them).

	prev_popup_ptr = NULL;
	popup_ptr = square_ptr->popup_list;
	while (popup_ptr) {
		if (popup_ptr->from_block && popup_ptr->square_ptr == square_ptr) {
			popup_ptr = popup_ptr->next_square_popup_ptr;
			if (prev_popup_ptr) {
				prev_popup_ptr->next_square_popup_ptr = popup_ptr;
				if (popup_ptr == NULL)
					square_ptr->last_popup_ptr = prev_popup_ptr;
			} else {
				square_ptr->popup_list = popup_ptr;
				if (popup_ptr == NULL)
					square_ptr->last_popup_ptr = NULL;
			}
		} else {
			prev_popup_ptr = popup_ptr;
			popup_ptr = popup_ptr->next_square_popup_ptr;
		}
	}

	// Step through the square's popup list again and reinitialise the square's
	// popup trigger flags.

	square_ptr->popup_trigger_flags = 0;
	popup_ptr = square_ptr->popup_list;
	while (popup_ptr) {
		square_ptr->popup_trigger_flags |= popup_ptr->trigger_flags;
		popup_ptr = popup_ptr->next_square_popup_ptr;
	}

	// Step through the global popup list, and remove all popups that came from
	// this block.

	prev_popup_ptr = NULL;
	popup_ptr = global_popup_list;
	while (popup_ptr) {
		if (popup_ptr->from_block && popup_ptr->square_ptr == square_ptr) {
			popup_ptr = del_popup(popup_ptr);
			if (prev_popup_ptr) {
				prev_popup_ptr->next_popup_ptr = popup_ptr;
				if (popup_ptr == NULL)
					last_global_popup_ptr = prev_popup_ptr;
			} else {
				global_popup_list = popup_ptr;
				if (popup_ptr == NULL)
					last_global_popup_ptr = NULL;
			}
		} else {
			prev_popup_ptr = popup_ptr;
			popup_ptr = popup_ptr->next_popup_ptr;
		}
	}

	// Step through the global entrance list, and remove the entrance location
	// that came from this block, if there is one.  Ignore the "default"
	// entrance, however, as it's needed as a fallback.

	prev_entrance_ptr = NULL;
	entrance_ptr = global_entrance_list;
	while (entrance_ptr) {
		if (stricmp(entrance_ptr->name, "default") &&
			entrance_ptr->del_location(column, row, level, true)) {
			if (entrance_ptr->locations == 0) {
				entrance_ptr = del_entrance(entrance_ptr);
				if (prev_entrance_ptr)
					prev_entrance_ptr->next_entrance_ptr = entrance_ptr;
				else
					global_entrance_list = entrance_ptr;
			}
			break;
		}
		prev_entrance_ptr = entrance_ptr;
		entrance_ptr = entrance_ptr->next_entrance_ptr;
	}

	// If there is an exit on the square that came from the block, remove it.

	if (square_ptr->exit_ptr && square_ptr->exit_ptr->from_block) {
		del_hyperlink(square_ptr->exit_ptr);
		square_ptr->exit_ptr = NULL;
	}

	// Delete the block.

	block_def_ptr = block_ptr->block_def_ptr;
	block_def_ptr->del_block(block_ptr);

	// Reset the block data in the square.

	square_ptr->block_ptr = NULL;
	square_ptr->block_trigger_flags = 0;
	square_ptr->block_trigger_list = NULL;
}

//------------------------------------------------------------------------------
// Convert a brightness level to a brightness index.
//------------------------------------------------------------------------------

int
get_brightness_index(float brightness)
{
	int brightness_index;

	// Make sure the brightness is between 0.0 and 1.0.

	if (brightness < 0.0)
		brightness = 0.0;
	if (brightness > 1.0)
		brightness = 1.0;

	// Determine the brightness index.

	brightness_index = (int)((1.0 - brightness) * MAX_BRIGHTNESS_INDEX);

	// Double-check the brightness index is within range before returning it.

	if (brightness_index < 0)
		brightness_index = 0;
	else if (brightness_index > MAX_BRIGHTNESS_INDEX)
		brightness_index = MAX_BRIGHTNESS_INDEX;
	return(brightness_index);
}

//------------------------------------------------------------------------------
// Create the polygon and vertex lists for a sprite block.
//------------------------------------------------------------------------------

void
create_sprite_polygon(block_def *block_def_ptr, part *part_ptr)
{
	int index;
	polygon *polygon_ptr;

	// Create the vertex and polygon lists.

	block_def_ptr->create_vertex_list(4);
	block_def_ptr->create_polygon_list(1);
	polygon_ptr = block_def_ptr->polygon_list;
	polygon_ptr->part_no = 0;
	polygon_ptr->part_ptr = part_ptr;

	// Create the vertex definition list for the polygon and initialise it.

	polygon_ptr->create_vertex_def_list(4);
	for (index = 0; index < 4; index++)
		polygon_ptr->vertex_def_list[index].vertex_no = index;
}

//------------------------------------------------------------------------------
// Create the sky.
//------------------------------------------------------------------------------

static void
create_sky(void)
{
	blockset *blockset_ptr;
	texture *texture_ptr;

	// Find the first blockset that defines a sky, if there is one.

	blockset_ptr = blockset_list_ptr->first_blockset_ptr;
	while (blockset_ptr) {
		if (blockset_ptr->sky_defined)
			break;
		blockset_ptr = blockset_ptr->next_blockset_ptr;
	}

	// Initialise the sky texture, custom texture and colour.

	sky_texture_ptr = NULL;
	custom_sky_texture_ptr = NULL;
	sky_colour.set_RGB(0,0,0);
	
	// A custom sky texture/colour takes precedence over a blockset sky
	// texture/colour in the first blockset.  If the sky texture is a custom 
	// texture, use the custom sky colour or placeholder texture for now.

	texture_ptr = custom_blockset_ptr->sky_texture_ptr;
	if (texture_ptr) {
		if (texture_ptr->custom) {
			custom_sky_texture_ptr = texture_ptr;
			if (custom_blockset_ptr->sky_colour_set)
				sky_colour = custom_blockset_ptr->sky_colour;
			else
				sky_texture_ptr = placeholder_texture_ptr;
		} else
			sky_texture_ptr = texture_ptr;
	} else if (custom_blockset_ptr->sky_colour_set)
		sky_colour = custom_blockset_ptr->sky_colour;
	else if (blockset_ptr) {
		sky_texture_ptr = blockset_ptr->sky_texture_ptr;
		sky_colour = blockset_ptr->sky_colour;
	}
	
	// Set the sky brightness and related data.  A custom sky brightness takes
	// prececedence over a blockset sky brightness.

	if (custom_blockset_ptr->sky_defined)
		sky_brightness = custom_blockset_ptr->sky_brightness;
	else if (blockset_ptr && blockset_ptr->sky_defined)
		sky_brightness = blockset_ptr->sky_brightness;
	else
		sky_brightness = 1.0f;
	sky_brightness_index = get_brightness_index(sky_brightness);
	sky_colour.adjust_brightness(sky_brightness);
	sky_colour_pixel = RGB_to_display_pixel(sky_colour);
}

//------------------------------------------------------------------------------
// Create the ground block definition.
//------------------------------------------------------------------------------

static void
create_ground_block_def(void)
{
	blockset *blockset_ptr;
	part *part_ptr;
	texture *ground_texture_ptr;
	vertex *vertex_list;
	polygon *polygon_ptr;
	BSP_node *BSP_node_ptr;
	int index;

	// If there is no custom ground definition, then we don't activate the
	// ground.

	if (!custom_blockset_ptr->ground_defined)
		return;

	// Find the first blockset that defines a ground, if there is one.

	blockset_ptr = blockset_list_ptr->first_blockset_ptr;
	while (blockset_ptr) {
		if (blockset_ptr->ground_defined)
			break;
		blockset_ptr = blockset_ptr->next_blockset_ptr;
	}
	
	// Create the ground block definition.  This is not a custom block since
	// it defines it's own BSP tree.

	if ((ground_block_def_ptr = new block_def) == NULL)
		memory_error("ground block definition");
	ground_block_def_ptr->custom = false;

	// Create the part list with one part.

	ground_block_def_ptr->create_part_list(1);
	part_ptr = ground_block_def_ptr->part_list;

	// A custom ground texture/colour takes precedence over a blockset ground
	// texture/colour in the first blockset.  If the ground texture is a custom 
	// texture, use the custom ground colour or placeholder texture for now.

	ground_texture_ptr = custom_blockset_ptr->ground_texture_ptr;
	if (ground_texture_ptr) {
		if (ground_texture_ptr->custom) {
			part_ptr->custom_texture_ptr = ground_texture_ptr;
			if (custom_blockset_ptr->ground_colour_set)
				part_ptr->colour = custom_blockset_ptr->ground_colour;
			else
				part_ptr->texture_ptr = placeholder_texture_ptr;
		} else
			part_ptr->texture_ptr = ground_texture_ptr;
	} else if (custom_blockset_ptr->ground_colour_set)
		part_ptr->colour = custom_blockset_ptr->ground_colour;
	else if (blockset_ptr) {
		part_ptr->texture_ptr = blockset_ptr->ground_texture_ptr;
		part_ptr->colour = blockset_ptr->ground_colour;
	}
	part_ptr->normalised_colour = part_ptr->colour;
	part_ptr->normalised_colour.normalise();

	// Create the vertex list with four vertices.

	ground_block_def_ptr->create_vertex_list(4);

	// Initialise the vertices so that the polygon covers the top of the
	// bounding cube.

	vertex_list = ground_block_def_ptr->vertex_list;
	vertex_list[0].set(0.0, UNITS_PER_BLOCK, UNITS_PER_BLOCK);
	vertex_list[1].set(UNITS_PER_BLOCK, UNITS_PER_BLOCK, UNITS_PER_BLOCK);
	vertex_list[2].set(UNITS_PER_BLOCK, UNITS_PER_BLOCK, 0.0);
	vertex_list[3].set(0.0, UNITS_PER_BLOCK, 0.0);

	// Create the polygon list with one polygon.

	ground_block_def_ptr->create_polygon_list(1);
	polygon_ptr = ground_block_def_ptr->polygon_list;

	// Create the vertex definition list for the polygon and initialise it.

	if (!polygon_ptr->create_vertex_def_list(4))
		memory_error("ground vertex definition list");
	for (index = 0; index < 4; index++)
		polygon_ptr->vertex_def_list[index].vertex_no = index;

	// Initialise the polygon's part, centroid, normal vector, plane offset
	// and texture coordinates.

	polygon_ptr->part_no = 0;
	polygon_ptr->part_ptr = part_ptr;
	polygon_ptr->compute_centroid(vertex_list);
	polygon_ptr->compute_normal_vector(vertex_list);
	polygon_ptr->compute_plane_offset(vertex_list);
	polygon_ptr->project_texture(vertex_list, NONE);

	// Create a single BSP node that points to the polygon.

	NEW(BSP_node_ptr, BSP_node);
	if (BSP_node_ptr == NULL)
		memory_error("BSP tree node");
	ground_block_def_ptr->BSP_tree = BSP_node_ptr;
}

//------------------------------------------------------------------------------
// Create the orb.
//------------------------------------------------------------------------------

static void
create_orb(void)
{
	blockset *blockset_ptr;
	texture *texture_ptr;
	direction orb_direction;
	RGBcolour orb_colour;

	// If there is no custom orb definition, then we don't activate the orb.

	if (!custom_blockset_ptr->orb_defined)
		return;
	
	// Find the first blockset that defines an orb, if there is one.

	blockset_ptr = blockset_list_ptr->first_blockset_ptr;
	while (blockset_ptr) {
		if (blockset_ptr->orb_defined)
			break;
		blockset_ptr = blockset_ptr->next_blockset_ptr;
	}
	
	// The orb texture in the custom blockset takes precedence over the orb
	// texture in the first blockset.  If the orb texture is a custom texture,
	// use the placeholder texture for now.

	texture_ptr = custom_blockset_ptr->orb_texture_ptr;
	if (texture_ptr) {
		if (texture_ptr->custom) {
			custom_orb_texture_ptr = texture_ptr;
			orb_texture_ptr = placeholder_texture_ptr;
		} else {
			custom_orb_texture_ptr = NULL;
			orb_texture_ptr = texture_ptr;
		}
	} else if (blockset_ptr && blockset_ptr->orb_texture_ptr) {
		custom_orb_texture_ptr = NULL;
		orb_texture_ptr = blockset_ptr->orb_texture_ptr;
	}
	
	// Create a directional light with default settings.

	if ((orb_light_ptr = new light) == NULL)
		memory_error("orb light");
	orb_light_ptr->style = DIRECTIONAL_LIGHT;

	// The orb direction in the custom blockset takes precedence over the
	// orb direction in the first available blockset.  If none define
	// one, the default orb direction is used.

	if (custom_blockset_ptr->orb_direction_set)
		orb_direction.set(-custom_blockset_ptr->orb_direction.angle_x,
			custom_blockset_ptr->orb_direction.angle_y + 180.0f);
	else if (blockset_ptr && blockset_ptr->orb_direction_set)
		orb_direction.set(-blockset_ptr->orb_direction.angle_x,
			blockset_ptr->orb_direction.angle_y + 180.0f);
	else
		orb_direction.set(0.0f,0.0f);
	orb_light_ptr->set_direction(orb_direction);

	// The orb colour in the custom blockset takes precedence over the orb
	// colour in the first available blockset.  If none define one, the default
	// orb colour is used.

	if (custom_blockset_ptr->orb_colour_set)
		orb_colour = custom_blockset_ptr->orb_colour;
	else if (blockset_ptr && blockset_ptr->orb_colour_set)
		orb_colour = blockset_ptr->orb_colour;
	else
		orb_colour.set_RGB(255,255,255);
	orb_light_ptr->colour = orb_colour;

	// The orb brightness in the custom blockset takes precedence over the
	// orb brightness in the first available blockset.  If none define
	// one, the default orb brightness is used.

	if (custom_blockset_ptr->orb_brightness_set)
		orb_brightness = custom_blockset_ptr->orb_brightness;
	else if (blockset_ptr && blockset_ptr->orb_brightness_set)
		orb_brightness = blockset_ptr->orb_brightness;
	else
		orb_brightness = 1.0f;
	orb_light_ptr->set_intensity(orb_brightness);
	orb_brightness_index = get_brightness_index(orb_brightness);

	// Compute the size and half size of the orb.

	orb_width = 12.0f * horz_pixels_per_degree;
	orb_height = 12.0f * vert_pixels_per_degree;
	half_orb_width = orb_width / 2.0f;
	half_orb_height = orb_height / 2.0f;

	// The orb exit in the custom blockset takes precedence over the orb exit
	// in the first available blockset.  If none define one, there is no orb
	// exit.

	if (custom_blockset_ptr->orb_exit_ptr)
		orb_exit_ptr = custom_blockset_ptr->orb_exit_ptr;
	else if (blockset_ptr && blockset_ptr->orb_exit_ptr)
		orb_exit_ptr = blockset_ptr->orb_exit_ptr;
	else
		orb_exit_ptr = NULL;
}

//------------------------------------------------------------------------------
// Initialise the spot.
//------------------------------------------------------------------------------

void
init_spot(void)
{
	blockset *blockset_ptr;
	int column, row, level;
	word block_symbol;
	popup *popup_ptr;
	square *square_ptr;
	block_def *block_def_ptr;
	block *block_ptr;
	polygon *polygon_ptr;
	part *part_ptr;
	int inactive_polygons;

	// Set the placeholder texture.  The custom placeholder texture takes
	// precedence over any blockset texture.

	if (custom_blockset_ptr->placeholder_texture_ptr)
		placeholder_texture_ptr = custom_blockset_ptr->placeholder_texture_ptr;
	else {
		blockset_ptr = blockset_list_ptr->first_blockset_ptr;
		while (blockset_ptr) {
			if (blockset_ptr->placeholder_texture_ptr) {
				placeholder_texture_ptr = blockset_ptr->placeholder_texture_ptr;
				break;
			}
			blockset_ptr = blockset_ptr->next_blockset_ptr;
		}
	}

	// Create the sky and orb, and the ground block definition.

	create_sky();
	create_orb();
	create_ground_block_def();

	// Step through the map and create a block for each occupied square.
	// Undefined or invalid map symbols are replaced by the empty block.

	for (level = 0; level < world_ptr->levels; level++)
		for (row = 0; row < world_ptr->rows; row++)
			for (column = 0; column < world_ptr->columns; column++) {
				square_ptr = world_ptr->get_square_ptr(column, row, level);
				block_symbol = square_ptr->block_symbol;
				if (block_symbol != NULL_BLOCK_SYMBOL) {

					// Update the logo.

					update_logo();

					// Get a pointer to the block definition assigned to this
					// square.  An invalid or undefined map symbol will result
					// in no block.

					if (block_symbol == GROUND_BLOCK_SYMBOL)
						block_def_ptr = ground_block_def_ptr;
					else
						block_def_ptr = symbol_table[block_symbol];

					// If there is a block definition for this square, create
					// the block to put on the square or into the movable block
					// list.

					if (block_def_ptr)
						add_block(block_def_ptr, square_ptr, 
							column, row, level, false, true);
				}
			}

	// Step through the map, and determine which polygons are active.

	inactive_polygons = 0;
	for (level = 0; level < world_ptr->levels; level++)
		for (row = 0; row < world_ptr->rows; row++)
			for (column = 0; column < world_ptr->columns; column++) {
				block_ptr = world_ptr->get_block_ptr(column, row, level);
				if (block_ptr) {
					inactive_polygons += compute_active_polygons(block_ptr, 
						column, row, level, false);
					update_logo();
				}
			}

	// Check that the default entrance was defined.

	if (find_entrance("default") == NULL)
		error("There is no 'default' entrance on the map");

	// Step through the original list of popups, and initialise those with 
	// non-custom background textures.

	popup_ptr = original_popup_list;
	while (popup_ptr) {
		texture *bg_texture_ptr = popup_ptr->bg_texture_ptr;
		if (bg_texture_ptr == NULL || !bg_texture_ptr->custom)
			init_popup(popup_ptr);
		popup_ptr = popup_ptr->next_popup_ptr;
	}

	// Create the sound buffer for the ambient sound if it isn't a streaming
	// sound and doesn't use a custom wave.

	if (ambient_sound_ptr && !ambient_sound_ptr->streaming && 
		!ambient_sound_ptr->wave_ptr->custom &&
		!create_sound_buffer(ambient_sound_ptr))
		warning("Unable to create sound buffer for wave file '%s'",
			ambient_sound_ptr->wave_ptr->URL);

#ifdef SUPPORT_A3D

	if (sound_on) {

		// Reset the audio.

		reset_audio();

		// If A3D sound is enabled, initialise the audio square map.

		world_ptr->init_audio_square_map();
	}

#endif

	// Step through the custom block definitions, and update all parts that
	// have a custom texture with the placeholder texture, if they don't
	// already have a custom colour set.

	block_def_ptr = custom_blockset_ptr->block_def_list;
	while (block_def_ptr) {
		for (int part_no = 0; part_no < block_def_ptr->parts; part_no++) {
			part_ptr = &block_def_ptr->part_list[part_no];
			if (part_ptr->custom_texture_ptr && !part_ptr->colour_set)
				part_ptr->texture_ptr = placeholder_texture_ptr;
		}
		block_def_ptr = block_def_ptr->next_block_def_ptr;
	}

	// Reinitialise the player block if it uses a custom texture to use the
	// placeholder texture, if it doesn't already have a custom colour set.

	if (player_block_ptr) {
		polygon_ptr = player_block_ptr->polygon_list;
		part_ptr = polygon_ptr->part_ptr;
		if (part_ptr->custom_texture_ptr && !part_ptr->colour_set) {
			part_ptr->texture_ptr = placeholder_texture_ptr;
			init_sprite_polygon(player_block_ptr, polygon_ptr, part_ptr);
			init_player_collision_box();
		}
	}

	// Step through the map and reinitialise all sprite blocks that use a
	// custom texture to use the placeholder texture, if they haven't already
	// got a custom colour set.

	for (column = 0; column < world_ptr->columns; column++)
		for (row = 0; row < world_ptr->rows; row++)
			for (level = 0; level < world_ptr->levels; level++) {
				block_ptr = world_ptr->get_block_ptr(column, row, level);
				if (block_ptr) {
					block_def_ptr = block_ptr->block_def_ptr;
					if (block_def_ptr->type & SPRITE_BLOCK) {
						polygon_ptr = block_ptr->polygon_list;
						part_ptr = polygon_ptr->part_ptr;
						if (part_ptr->custom_texture_ptr && 
							!part_ptr->colour_set) {
							part_ptr->texture_ptr = placeholder_texture_ptr;
							init_sprite_polygon(block_ptr, polygon_ptr, 
								part_ptr);
						}
					}
				}
			}
}

//------------------------------------------------------------------------------
// Display a message indicating that the file with the given URL is being
// loaded.
//------------------------------------------------------------------------------

static void
loading_message(const char *URL)
{
	string file_name;

	// Extract the file name from the URL.

	split_URL(URL, NULL, &file_name, NULL);

	// Display the file name on the task bar.

	set_title("Loading %s", file_name);
}

//------------------------------------------------------------------------------
// Request that a URL be downloaded into the given file path, or the given
// window target.  If both the file path and target are NULL, the URL will be
// downloaded into the browser cache.  If no_overlap is TRUE, then another URL
// requested before this one has been downloaded will supercede it.
//------------------------------------------------------------------------------

void
request_URL(const char *URL, const char *file_path, const char *target,
			bool no_overlap)
{
	bool URL_request_pending_flag;

	// Display a loading message on the toolbar, but only if there is no
	// file path and target specified.

	if (file_path == NULL && target == NULL)
		loading_message(URL);

	// If a URL request is pending acceptance by the plugin thread, wait until
	// it has been processed before continuing.

	do {
		start_atomic_operation();
		URL_request_pending_flag = URL_request_pending;
		end_atomic_operation();
	} while (URL_request_pending_flag);

	// Set the requested URL, encoding it if necessary to make it browser
	// friendly. If no_overlap is TRUE, then this is also the pending URL.
	// Requests for such URLs are not permitted to overlap.

	requested_URL = URL;
	if (web_browser_ID == NAVIGATOR)
		requested_URL = encode_URL(requested_URL);
	if (no_overlap)
		pending_URL = requested_URL;

	// Request that the given URL be downloaded to the given target.

	requested_target = target;
	requested_file_path = file_path;
	URL_download_requested.send_event(true);

	// Indicate that this URL request is pending acceptance by the plugin
	// thread.

	start_atomic_operation();
	URL_request_pending = true;
	end_atomic_operation();

	// If MSIE is the browser, set the request time and reset the URL opened
	// flag.

	request_time_ms = get_time_ms();
	URL_opened = false;
}

//------------------------------------------------------------------------------
// Check whether the pending URL has been downloaded.
//------------------------------------------------------------------------------

static int
URL_downloaded(void)
{
	bool success;

	// If we are running under Microsoft Internet Explorer, check whether the
	// URL has been opened first, timing out after 15 seconds.  This is not a
	// particularly good solution, but it's the best available.

	if (web_browser_ID == INTERNET_EXPLORER && !URL_opened) {
		if (!URL_was_opened.event_sent()) {
			if (get_time_ms() - request_time_ms > 15000) {
				curr_URL = pending_URL;
				return(0);
			} else
				return(-1);
		}
		if (!(URL_opened = URL_was_opened.event_value)) {
			curr_URL = pending_URL;
			return(0);
		}
	}

	// Check whether the URL has been downloaded.

	if (!URL_was_downloaded.event_sent())
		return(-1);
	success = URL_was_downloaded.event_value;

	// Get the URL and file path from the plugin thread.

	start_atomic_operation();
	curr_URL = downloaded_URL;
	if (success)
		curr_file_path = downloaded_file_path;
	end_atomic_operation();

	// If the URL downloaded is not the pending URL, then ignore it.

	if (stricmp(curr_URL, pending_URL))
		return(-1);

	// Decode the URL and file path to remove encoded characters.

	curr_URL = decode_URL(curr_URL);
	if (success) {
		curr_file_path = decode_URL(curr_file_path);
		curr_file_path = URL_to_file_path(curr_file_path);
	}

	// Return the success or failure status.

	return(success ? 1 : 0);
}

//------------------------------------------------------------------------------
// Download a URL to the given file path, or to a file in the browser cache if
// the file path is NULL.
//------------------------------------------------------------------------------

#include <windows.h>

bool
download_URL(const char *URL, const char *file_path)
{
	int result;

	// Request that the URL be downloaded.

	request_URL(URL, file_path, NULL, true);

	// Wait until the browser sends an event indicating that a URL was
	// or was not downloaded; draw the active logo whilst we're waiting.
	// If we get a player window shutdown request while we're waiting,
	// break out of the loop and resend the event so that the main event
	// loop will catch it.
		
	while (true) {
		if ((result = URL_downloaded()) >= 0)
			break;
		if (player_window_shutdown_requested.event_sent()) {
			player_window_shutdown_requested.send_event(true);
			result = -1;
			break;
		}
		update_logo();
	}

	// Return a boolean value indicating if the URL was downloaded or not.

	return(result > 0 ? true : false);
}

//------------------------------------------------------------------------------
// Permit the user to display the error log file if the debug flag is on and
// there were warnings written to the file.
//------------------------------------------------------------------------------

void
display_error_log_file()
{
	FILE *fp;

	// Finish off the error log file so that it's ready to display as a web
	// page.

	if ((fp = fopen(error_log_file_path, "a")) != NULL) {
		fprintf(fp, "</BODY>\n</HTML>\n");
		fclose(fp);
	}

	// If the debug flag is on and there are warnings in the error log file,
	// then allow the error log file to be displayed in a new browser window if
	// the user requests it.

	if (debug_on && warnings)
		display_error_log.send_event(true);
}
		
//------------------------------------------------------------------------------
// Initiate the download of the first custom texture or wave.
//------------------------------------------------------------------------------

void
initiate_first_download(void)
{
	// Get a pointer to the first custom texture that hasn't yet been loaded
	// (if any).
	
	curr_custom_texture_ptr = custom_blockset_ptr->first_texture_ptr;
	while (curr_custom_texture_ptr && (curr_custom_texture_ptr->pixmap_list ||
		!curr_custom_texture_ptr->custom))
		curr_custom_texture_ptr = curr_custom_texture_ptr->next_texture_ptr;
	
	// Get a pointer to the first custom wave that hasn't yet been loaded
	// (if any).

	curr_custom_wave_ptr = custom_blockset_ptr->first_wave_ptr;
	while (curr_custom_wave_ptr && !curr_custom_wave_ptr->custom)
		curr_custom_wave_ptr = curr_custom_wave_ptr->next_wave_ptr;

	// If there aren't any custom textures or waves to be loaded, display the
	// spot title on the task bar, and display the error log file if
	// necessary.

	if (curr_custom_texture_ptr == NULL && curr_custom_wave_ptr == NULL) {
		set_title("%s", spot_title);
		display_error_log_file();
	}

	// Otherwise choose the first custom texture or wave to download based on
	// which has the lower load index.

	else if (curr_custom_texture_ptr != NULL && (curr_custom_wave_ptr == NULL || 
		curr_custom_texture_ptr->load_index < curr_custom_wave_ptr->load_index))
		request_URL(curr_custom_texture_ptr->URL, NULL, NULL, true);
	else
		request_URL(curr_custom_wave_ptr->URL, NULL, NULL, true);
}

//------------------------------------------------------------------------------
// Generate an oversized texture warning.
//------------------------------------------------------------------------------

static void
oversized_texture_warning(const char *URL, const char *format, ...)
{
	va_list arg_ptr;
	char message[BUFSIZ];

	va_start(arg_ptr, format);
	vsprintf(message, format, arg_ptr);
	va_end(arg_ptr);
	warning("Cannot use texture %s %s; it has a width or height greater than "
		"256 pixels", URL, message);
}

//------------------------------------------------------------------------------
// Update the block definitions, sprites and popups that depend on the given
// custom texture.
//------------------------------------------------------------------------------

void
update_texture_dependancies(texture *custom_texture_ptr)
{
	int column, level, row;
	popup *popup_ptr;
	block_def *block_def_ptr;
	polygon *polygon_ptr;
	part *part_ptr;
	bool oversized;

 	// Step through the original popup list, and initialise those that use the
	// custom texture for a background texture.

	popup_ptr = original_popup_list;
	while (popup_ptr) {
		texture *bg_texture_ptr = popup_ptr->bg_texture_ptr;
		if (bg_texture_ptr == custom_texture_ptr)
			init_popup(popup_ptr);
		popup_ptr = popup_ptr->next_popup_ptr;
	}

	// If the custom texture has a width or height larger than 256 texels, then
	// it is oversized.

	if (custom_texture_ptr->width > 256 || custom_texture_ptr->height > 256)
		oversized = true;
	else
		oversized = false;

	// Step through the custom block definitions, and update all parts that
	// use the custom texture.

	block_def_ptr = custom_blockset_ptr->block_def_list;
	while (block_def_ptr) {
		for (int part_no = 0; part_no < block_def_ptr->parts; part_no++) {
			part_ptr = &block_def_ptr->part_list[part_no];
			if (part_ptr->custom_texture_ptr == custom_texture_ptr) {
				if (oversized) {
					switch (world_ptr->map_style) {
					case SINGLE_MAP:
						oversized_texture_warning(custom_texture_ptr->URL,
							"in part '%s' of block '%c'", part_ptr->name,
							block_def_ptr->single_symbol);
						break;
					case DOUBLE_MAP:
						oversized_texture_warning(custom_texture_ptr->URL,
							"in part '%s' of block '%c%c'", part_ptr->name,
							block_def_ptr->double_symbol >> 7,
							block_def_ptr->double_symbol & 127);
					}
				} else
					part_ptr->texture_ptr = custom_texture_ptr;
			}
		}
		block_def_ptr = block_def_ptr->next_block_def_ptr;
	}

	// Reinitialise the player block if it uses the custom texture.

	if (player_block_ptr) {
		polygon_ptr = player_block_ptr->polygon_list;
		part_ptr = polygon_ptr->part_ptr;
		if (part_ptr->texture_ptr == custom_texture_ptr) {
			if (oversized)
				oversized_texture_warning(custom_texture_ptr->URL,
					"in part '%s' of player block", part_ptr->name);
			else {
				init_sprite_polygon(player_block_ptr, polygon_ptr, part_ptr);
				init_player_collision_box();
			}
		}
	}

	// Step through the map and reinitialise all sprite blocks that use the
	// custom texture.  If the custom texture is oversized, ignore it.

	for (column = 0; column < world_ptr->columns; column++)
		for (row = 0; row < world_ptr->rows; row++)
			for (level = 0; level < world_ptr->levels; level++) {
				block *block_ptr = world_ptr->get_block_ptr(column, row, level);
				if (block_ptr) {
					block_def *block_def_ptr = block_ptr->block_def_ptr;
					if (block_def_ptr->type & SPRITE_BLOCK) {
						polygon_ptr = block_ptr->polygon_list;
						part_ptr = polygon_ptr->part_ptr;
						if (part_ptr->custom_texture_ptr == custom_texture_ptr) {
							if (oversized)
								oversized_texture_warning(
									custom_texture_ptr->URL,
									"in sprite block at location (%d,%d,%d)",
									column + 1, row + 1, level + 1);
							else
								init_sprite_polygon(block_ptr, polygon_ptr, 
									part_ptr);
						}
					}
				}
			}

	// Reinitialise the sky if it uses the custom texture.

	if (custom_sky_texture_ptr == custom_texture_ptr) {
		if (oversized)
			oversized_texture_warning(custom_texture_ptr->URL, "for the sky");
		else
			sky_texture_ptr = custom_texture_ptr;
	}

	// Reinitialise the ground part if it exists and it uses the custom texture.

	if (ground_block_def_ptr) {
		part_ptr = ground_block_def_ptr->part_list;
		if (part_ptr->custom_texture_ptr == custom_texture_ptr) {
			if (oversized)
				oversized_texture_warning(custom_texture_ptr->URL, 
					"for the ground");
			else
				part_ptr->texture_ptr = custom_texture_ptr;
		}
	}

	// Reinitialise the orb if it exists and uses the custom texture.

	if (custom_orb_texture_ptr == custom_texture_ptr) {
		if (oversized)
			oversized_texture_warning(custom_texture_ptr->URL, "for the orb");
		else
			orb_texture_ptr = custom_texture_ptr;
	}
}

//------------------------------------------------------------------------------
// Update the sounds that depend on the given wave.
//------------------------------------------------------------------------------

static void
update_wave_dependancies(wave *wave_ptr)
{
	sound *sound_ptr;
	
	// Create the sound buffer for the ambient sound if it depends on
	// the given wave and is not a streaming sound.

	if (ambient_sound_ptr && !ambient_sound_ptr->streaming &&
		ambient_sound_ptr->wave_ptr == wave_ptr &&
		!create_sound_buffer(ambient_sound_ptr))
		warning("Unable to create sound buffer for wave file '%s'",
			ambient_sound_ptr->wave_ptr->URL);

	// Create the sound buffer for all of the non-streaming sounds that depend
	// on the given wave.

	sound_ptr = global_sound_list;
	while (sound_ptr) {
		if (!sound_ptr->streaming && sound_ptr->wave_ptr == wave_ptr &&
			!create_sound_buffer(sound_ptr))
			warning("Unable to create sound buffer for wave file '%s'",
				sound_ptr->wave_ptr->URL);
		sound_ptr = sound_ptr->next_sound_ptr;
	}
}

//------------------------------------------------------------------------------
// Handle the current download.
//------------------------------------------------------------------------------

bool
handle_current_download(void)
{
	int download_status;

	// Check the download status.  If the current URL has not been downloaded
	// yet, then just return with a success status.

	download_status = URL_downloaded();
	if (download_status < 0)
		return(true);

	// If the current download is for a custom texture...

	if (curr_custom_texture_ptr != NULL && (curr_custom_wave_ptr == NULL || 
		curr_custom_texture_ptr->load_index < curr_custom_wave_ptr->load_index)) {

		// If the URL was not downloaded successfully or cannot be parsed,
		// generate a warning.

		if (download_status == 0 || !load_image(curr_URL, curr_file_path, 
			curr_custom_texture_ptr, true))
			warning("Unable to download custom texture from %s", curr_URL);
		
		// Otherwise update all texture dependancies.

		else
			update_texture_dependancies(curr_custom_texture_ptr);

		// Get a pointer to the next custom texture that hasn't been loaded yet.

		curr_custom_texture_ptr = curr_custom_texture_ptr->next_texture_ptr;
		while (curr_custom_texture_ptr && 
			(curr_custom_texture_ptr->pixmap_list ||
			 !curr_custom_texture_ptr->custom))
			curr_custom_texture_ptr = curr_custom_texture_ptr->next_texture_ptr;
	}

	// If the current download is for a custom wave...

	else {

		// If the URL was not downloaded successfully, or could not be loaded
		// into the wave object, then generate a warning.

		if (download_status == 0 || !load_wave_file(curr_custom_wave_ptr,
			curr_URL, curr_file_path))
			warning("Unable to download custom sound from %s", curr_URL);

		// Otherwise update the wave dependancies. 

		else
			update_wave_dependancies(curr_custom_wave_ptr);

		// Get a pointer to the next custom wave that hasn't been loaded yet.

		curr_custom_wave_ptr = curr_custom_wave_ptr->next_wave_ptr;
		while (curr_custom_wave_ptr && !curr_custom_wave_ptr->custom)
			curr_custom_wave_ptr = curr_custom_wave_ptr->next_wave_ptr;
	}

	// If there are no more custom textures or waves, display the spot title in
	// the task bar, and let the user display the error log file if the debug
	// tag is on and there were warnings written to it.

	if (curr_custom_texture_ptr == NULL && curr_custom_wave_ptr == NULL) {
		set_title("%s", spot_title);
		display_error_log_file();
	}

	// Otherwise choose the next custom texture or wave to download based on
	// which has the lower load index.

	else if (curr_custom_texture_ptr != NULL && (curr_custom_wave_ptr == NULL || 
		curr_custom_texture_ptr->load_index < curr_custom_wave_ptr->load_index))
		request_URL(curr_custom_texture_ptr->URL, NULL, NULL, true);
	else
		request_URL(curr_custom_wave_ptr->URL, NULL, NULL, true);
	
	// Return a success status.

	return(true);
}

//------------------------------------------------------------------------------
// Identify the CPU we are running on.
//------------------------------------------------------------------------------

void
identify_processor(void)
{
	int cpuid_supported;
	char cpu_vendor[13];

	// Check whether the CPUID instruction is supported; if it isn't, then the
	// CPU is unknown so would not support AMD's 3DNow instruction set.

	__asm {
		pushfd					// Save EFLAGS.
		pop eax
		mov ebx, eax
		xor eax, 0x00200000		// Try inverting bit 21 of EFLAGS.
		push eax
		popfd
		pushfd
		pop eax
		and eax, 0x00200000		// If bit 21 changed, CPUID is supported.
		and ebx, 0x00200000
		xor eax, ebx
		mov cpuid_supported, eax
	}
	if (!cpuid_supported) {
		AMD_3DNow_supported = false;
		return;
	}

	// Get the CPU vendor string.

	__asm {
		mov eax, 0				// Get vendor function.
		_emit 0x0f				// CPUID
		_emit 0xa2
		mov cpu_vendor[0], bl
		mov cpu_vendor[1], bh
		shr ebx, 16
		mov cpu_vendor[2], bl
		mov cpu_vendor[3], bh
		mov cpu_vendor[4], dl
		mov cpu_vendor[5], dh
		shr edx, 16
		mov cpu_vendor[6], dl
		mov cpu_vendor[7], dh
		mov cpu_vendor[8], cl
		mov cpu_vendor[9], ch
		shr ecx, 16
		mov cpu_vendor[10], cl
		mov cpu_vendor[11], ch
		mov cpu_vendor[12], 0
	}

	// If the CPU vendor is "AuthenticAMD", then check the extended feature
	// flags for the existence of 3DNow! instruction support.

	if (!strcmp(cpu_vendor, "AuthenticAMD")) {
		__asm {
			mov eax, 0x80000001		// Get extended flags function.
			_emit 0x0f				// CPUID
			_emit 0xa2
			and edx, 0x80000000
			mov AMD_3DNow_supported, edx
		}
		if (AMD_3DNow_supported)
			diagnose("AMD processor with 3DNow! instructions detected");
	}
}