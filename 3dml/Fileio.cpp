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
#include <string.h>
#include <limits.h>
#include <math.h>
#include <direct.h>
#include <io.h>
#include "Classes.h"
#include "Image.h"
#include "Light.h"
#include "Main.h"
#include "Memory.h"
#include "Parser.h"
#include "Platform.h"
#include "Plugin.h"
#include "Render.h"
#include "Spans.h"
#include "Tags.h"
#include "Utils.h"

// Flags indicating whether we've seen certain tags already when loading a new
// spot, blockset or block file.

static bool got_head_tag;
static bool got_body_tag;
static bool got_title_tag;
static bool got_blockset_tag;
static bool got_map_tag;
static bool got_ground_tag;
static bool got_ambient_light_tag;
static bool got_ambient_sound_tag;
static bool got_onload_tag;
static bool got_stream_tag;
static bool got_vertex_list;
static bool got_part_list;
static bool got_light_tag;
static bool got_sound_tag;
static bool got_exit_tag;
static bool got_BSP_tree_tag;
static bool got_sprite_part_tag;
static bool got_param_tag;
static bool got_player_tag;
static bool got_action_exit_tag;
static int curr_level;

// List of vertex definition entries, parts and polygons in the current block
// definition.

static int vertices;
static vertex_def_entry *vertex_def_entry_list;
static int polygons;
static polygon *polygon_list;
static int parts;
static part *part_list;

//==============================================================================
// BLOCK file parsing functions.
//==============================================================================

//------------------------------------------------------------------------------
// Update a part's common fields.
//------------------------------------------------------------------------------

static void
update_common_part_fields(part *part_ptr, texture *texture_ptr, 
						  RGBcolour colour, float translucency,
						  texstyle texture_style)
{
	// If the texture or stream parameter was given, set either the current or
	// custom texture pointer.  If the colour parameter was given but the
	// texture parameter was not, set the texture pointers to NULL so that the
	// colour takes precedence.

	if (matched_param[PART_TEXTURE] || matched_param[PART_STREAM]) {
		if (texture_ptr && texture_ptr->custom) {
			part_ptr->custom_texture_ptr = texture_ptr;
			part_ptr->texture_ptr = NULL;
		} else
			part_ptr->texture_ptr = texture_ptr;
	} else if (matched_param[PART_COLOUR]) {
		part_ptr->texture_ptr = NULL;
		part_ptr->custom_texture_ptr = NULL;
	}

	// Update remaining fields, if they have been specified.

	if (matched_param[PART_COLOUR]) {
		part_ptr->colour_set = true;
		part_ptr->colour = colour;
		colour.normalise();
		part_ptr->normalised_colour = colour;
	}
	if (matched_param[PART_TRANSLUCENCY])
		part_ptr->alpha = 1.0f - translucency;
	if (matched_param[PART_STYLE])
		part_ptr->texture_style = texture_style;
}

//------------------------------------------------------------------------------
// Parse the vertex list.
//------------------------------------------------------------------------------

static void
parse_vertex_list(block_def *block_def_ptr)
{
	int vertices;
	vertex_entry *vertex_entry_list;
	vertex_entry *vertex_entry_ptr, *next_vertex_entry_ptr;
	int index;

	// If the vertices tag has already been seen, skip over all tags inside it
	// and the end vertices tag.

	if (got_vertex_list) {
		parse_next_tag(NULL, TOKEN_VERTICES);
		return;
	}
	got_vertex_list = true;

	// Do the parsing in a try block, so that we can trap errors...

	vertices = 0;
	vertex_entry_list = NULL;
	try { 
		tag *tag_ptr;

		// Read each vertex definition and store it in the vertex entry list,
		// until all vertex definitions have been read.

		while ((tag_ptr = parse_next_tag(vertices_tag_list, TOKEN_VERTICES))
			!= NULL) {
			switch (tag_ptr->tag_name) {
			case TOKEN_VERTEX:

				// Verify that the vertex reference is in sequence.

				vertices++;
				if (vertex_ref != vertices)
					error("Vertex reference number %d is out of sequence",
						vertex_ref);

				// Create a new vertex entry, and add it to the head of the
				// vertex entry list.

				NEW(vertex_entry_ptr, vertex_entry);
				if (vertex_entry_ptr == NULL)
					memory_error("vertex entry");
				vertex_entry_ptr->next_vertex_entry_ptr = vertex_entry_list;
				vertex_entry_list = vertex_entry_ptr;

				// Initialise the vertex entry.

				vertex_entry_ptr->x = vertex_coords.x;
				vertex_entry_ptr->y = vertex_coords.y;
				vertex_entry_ptr->z = vertex_coords.z;
			}
		}
	}

	// Upon an error, delete the vertex entry list, before throwing the error.

	catch (char *error_str) {
		while (vertex_entry_list) {
			next_vertex_entry_ptr = vertex_entry_list->next_vertex_entry_ptr;
			DEL(vertex_entry_list, vertex_entry);
			vertex_entry_list = next_vertex_entry_ptr;
		}
		throw (char *)error_str;
	}

	// Copy the vertex entry list to the block definition's vertex list, 
	// deleting it along the way.  This is done in reverse order because the
	// vertex entry list is in reverse order.

	block_def_ptr->create_vertex_list(vertices);
	vertex_entry_ptr = vertex_entry_list;
	for (index = vertices - 1; index >= 0; index--) {
		block_def_ptr->vertex_list[index].x = vertex_entry_ptr->x;
		block_def_ptr->vertex_list[index].y = vertex_entry_ptr->y;
		block_def_ptr->vertex_list[index].z = vertex_entry_ptr->z;
		next_vertex_entry_ptr = vertex_entry_ptr->next_vertex_entry_ptr;
		DEL(vertex_entry_ptr, vertex_entry);
		vertex_entry_ptr = next_vertex_entry_ptr;
	}
}

//------------------------------------------------------------------------------
// Parse the vertex reference list.
//------------------------------------------------------------------------------

static const char *vertex_ref_common_error = "Expected a list of vertex "
	"reference numbers as the value of parameter 'vertices'";

static void
parse_vertex_ref_list(int block_vertices)
{
	vertex_def_entry *last_vertex_def_entry_ptr;

	// Parse each vertex reference number in the string.

	start_parsing_value(polygon_vertices);
	last_vertex_def_entry_ptr = NULL;
	do {
		int ref_no;
		vertex_def_entry *vertex_def_entry_ptr;

		// If the reference number cannot be parsed or is out of range,
		// this is an error.

		if (!parse_integer_in_value(&ref_no))
			error(vertex_ref_common_error);
		if (!check_int_range(ref_no, 1, block_vertices))
			error("Vertex reference number %d is out of range", ref_no);

		// Create a new vertex definition entry and initialise it's vertex
		// number and next vertex definition entry pointer.

		if ((vertex_def_entry_ptr = new vertex_def_entry) == NULL)
			memory_error("vertex definition entry");
		vertex_def_entry_ptr->vertex_no = ref_no - 1;
		vertex_def_entry_ptr->next_vertex_def_entry_ptr = NULL;

		// Add the vertex definition entry to the end of the vertex 
		// definition entry list.

		if (last_vertex_def_entry_ptr)
			last_vertex_def_entry_ptr->next_vertex_def_entry_ptr = 
				vertex_def_entry_ptr;
		else
			vertex_def_entry_list = vertex_def_entry_ptr;
		last_vertex_def_entry_ptr = vertex_def_entry_ptr;
		vertices++;
	} while (token_in_value_is(","));

	// If the remainder of the string is not empty or whitespace, or the
	// vertex reference list has less than 3 elements, this is an error.

	if (!stop_parsing_value())
		error(vertex_ref_common_error);
	if (vertices < 3)
		error("Polygon has less than 3 vertices");
}

//------------------------------------------------------------------------------
// Parse the texture coordinates list.
//------------------------------------------------------------------------------

static const char *texcoords_common_error = "Expected a list of texture "
	"coordinates as the value of parameter 'texcoords'";

static void
parse_texcoords_list(void)
{
	vertex_def_entry *vertex_def_entry_ptr;

	// Parse the list of texture coordinates.  These coordinates are assumed
	// to be for "stretched" texture style; the texture coordinates for 
	// "tiled" or "scaled" texture style can be computed on the fly.

	start_parsing_value(polygon_texcoords);
	vertex_def_entry_ptr = vertex_def_entry_list;
	do {
		
		// If we've run out of vertex definition entries, or we can't parse
		// the texture coordinates, this is an error.

		if (vertex_def_entry_ptr == NULL)
			error("There are too many texture coordinates");
		if (!token_in_value_is("(") || 
			!parse_float_in_value(&vertex_def_entry_ptr->u) ||
			!token_in_value_is(",") || 
			!parse_float_in_value(&vertex_def_entry_ptr->v) ||
			!token_in_value_is(")"))
			error(texcoords_common_error);

		// Move onto the next vertex definition.

		vertex_def_entry_ptr = 
			vertex_def_entry_ptr->next_vertex_def_entry_ptr;
	} while (token_in_value_is(","));

	// If we haven't come to the end of the vertex definition entry list,
	// or the remainder of the string is not empty or whitespace, this is
	// an error.

	if (vertex_def_entry_ptr != NULL)
		error("There are too few texture coordinates");
	if (!stop_parsing_value())
		error(texcoords_common_error);
}

//------------------------------------------------------------------------------
// Compute the texture coordinates for a polygon.
//------------------------------------------------------------------------------

static void
compute_texture_coordinates(polygon *polygon_ptr, texstyle texture_style,
							compass projection, block_def *block_def_ptr)
{
	// Compute the texture coordinates for this polygon.  The texture
	// coordinates for the "stretched" texture style are already computed
	// and merely need to be recovered.

	if (texture_style == STRETCHED_TEXTURE)
		for (int index = 0; index < polygon_ptr->vertices; index++) {
			vertex_def *vertex_def_ptr = &polygon_ptr->vertex_def_list[index];
			vertex_def_ptr->u = vertex_def_ptr->orig_u;
			vertex_def_ptr->v = vertex_def_ptr->orig_v;
		}
	else
		polygon_ptr->project_texture(block_def_ptr->vertex_list, projection);
}

//------------------------------------------------------------------------------
// Rotate the texture coordinates for a polygon.
//------------------------------------------------------------------------------

static void
rotate_texture_coordinates(polygon *polygon_ptr, float texture_angle)
{
	for (int vertex_no = 0; vertex_no < polygon_ptr->vertices; vertex_no++) {
		vertex_def *vertex_def_ptr;
		float tu, tv, ru, rv;

		vertex_def_ptr = &polygon_ptr->vertex_def_list[vertex_no];
		tu = vertex_def_ptr->u - 0.5f;
		tv = vertex_def_ptr->v - 0.5f;
		ru = tu * cosine[texture_angle] + tv * sine[texture_angle];
		rv = tv * cosine[texture_angle] - tu * sine[texture_angle];
		vertex_def_ptr->u = ru + 0.5f;
		vertex_def_ptr->v = rv + 0.5f;
	}
}

//------------------------------------------------------------------------------
// Parse the polygon tag.
//------------------------------------------------------------------------------

static void
parse_polygon_tag(block_def *block_def_ptr, part *part_ptr)
{
	int index;
	polygon *polygon_ptr;
	vertex_def_entry *vertex_def_entry_ptr;
	vertex_def_entry *next_vertex_def_entry_ptr;

	// Make sure the polygon reference number was in sequence.

	if (polygon_ref != polygons + 1)
		error("Polygon reference number %d is out of sequence", polygon_ref);

	// Create a new polygon, and add it to the head of the polygon list.

	if ((polygon_ptr = new polygon) == NULL)
		memory_error("polygon entry");
	polygon_ptr->next_polygon_ptr = polygon_list;
	polygon_list = polygon_ptr;
	polygons++;

	// Store the front and rear polygon references in the polygon, if they
	// were given.

	if (matched_param[POLYGON_FRONT])
		polygon_ptr->front_polygon_ref = polygon_front;
	if (matched_param[POLYGON_REAR])
		polygon_ptr->rear_polygon_ref = polygon_rear;

	// Parse the vertex reference list and texture coordinates list, creating a
	// vertex definition entry list in the process.  If this fails, delete the 
	// vertex definition entry list before throwing the error.

	vertices = 0;
	vertex_def_entry_list = NULL;
	try {
		parse_vertex_ref_list(block_def_ptr->vertices);
		parse_texcoords_list();
	}
	catch (char *error_str) {
		while (vertex_def_entry_list) {
			next_vertex_def_entry_ptr = 
				vertex_def_entry_list->next_vertex_def_entry_ptr;
			delete vertex_def_entry_list;
			vertex_def_entry_list = next_vertex_def_entry_ptr;
		}
		throw (char *)error_str;
	}

	// Copy the vertex definition entry list to the polygon's vertex definition
	// list, deleting it in the process.

	if (!polygon_ptr->create_vertex_def_list(vertices))
		memory_error("polygon vertex definition list");
	vertex_def_entry_ptr = vertex_def_entry_list;
	for (index = 0; index < vertices; index++) {
		vertex_def *vertex_def_ptr = &polygon_ptr->vertex_def_list[index];
		vertex_def_ptr->vertex_no = vertex_def_entry_ptr->vertex_no;
		vertex_def_ptr->u = vertex_def_entry_ptr->u;
		vertex_def_ptr->v = vertex_def_entry_ptr->v;
		vertex_def_ptr->orig_u = vertex_def_entry_ptr->u;
		vertex_def_ptr->orig_v = vertex_def_entry_ptr->v;
		next_vertex_def_entry_ptr = 
			vertex_def_entry_ptr->next_vertex_def_entry_ptr;
		delete vertex_def_entry_ptr;
		vertex_def_entry_ptr = next_vertex_def_entry_ptr;
	}

	// Set the part number, then compute the centroid, normal vector and plane
	// offset.

	polygon_ptr->part_no = parts - 1;
	polygon_ptr->compute_centroid(block_def_ptr->vertex_list);
	polygon_ptr->compute_normal_vector(block_def_ptr->vertex_list);
	polygon_ptr->compute_plane_offset(block_def_ptr->vertex_list);

	// Compute and rotate the texture coordinates.

	compute_texture_coordinates(polygon_ptr, part_ptr->texture_style, NONE,
		block_def_ptr);
	rotate_texture_coordinates(polygon_ptr, part_ptr->texture_angle);
}

//------------------------------------------------------------------------------
// Parse a part tag in a structural block.
//------------------------------------------------------------------------------

static void
parse_part_tag(blockset *blockset_ptr, block_def *block_def_ptr)
{
	tag *tag_ptr;
	part *part_ptr;
	texture *texture_ptr;

	// Verify the part name hasn't been used in this block already.

	part_ptr = part_list;
	while (part_ptr) {
		if (!stricmp(part_name, part_ptr->name))
			error("Duplicate part name '%s'", part_name);
		part_ptr = part_ptr->next_part_ptr;
	}

	// Create a new part with default attributes, and add it to the head of 
	// the part list.

	if ((part_ptr = new part) == NULL)
		memory_error("part");
	part_ptr->next_part_ptr = part_list;
	part_list = part_ptr;
	parts++;

	// Set the part name.

	part_ptr->name = part_name;

	// If the texture parameter was present, load or retrieve the texture,

	if (matched_param[PART_TEXTURE]) 
		texture_ptr = load_texture(blockset_ptr, part_texture, false);

	// Initialise the part's common fields.
	
	update_common_part_fields(part_ptr, texture_ptr, part_colour,
		part_translucency, part_style);
	
	// Initialise the fields permitted in a structural block part tag.

	if (matched_param[PART_FACES]) {
		if (part_faces < 0 || part_faces > 2)
			error("A part must have 0, 1 or 2 faces per polygon");
		part_ptr->faces = part_faces;
	}
	if (matched_param[PART_ANGLE])
		part_ptr->texture_angle = part_angle;

	// Read each "polygon" tag until the ending "part" tag is reached.

	while ((tag_ptr = parse_next_tag(part_tag_list, TOKEN_PART)) != NULL) {
		switch (tag_ptr->tag_name) {
		case TOKEN_POLYGON:
			parse_polygon_tag(block_def_ptr, part_ptr);
		}
	}
}

//------------------------------------------------------------------------------
// Parse the structural part list.
//------------------------------------------------------------------------------

static void
parse_part_list(blockset *blockset_ptr, block_def *block_def_ptr)
{
	tag *tag_ptr;
	part *part_ptr, *next_part_ptr;
	polygon *polygon_ptr, *next_polygon_ptr;
	int index;
 
	// If the parts tag has already been seen, skip over the tags inside and
	// the end parts tag.

	if (got_part_list) {
		parse_next_tag(NULL, TOKEN_PARTS);
		return;
	}
	got_part_list = true;

	// Read each part definition and store it in the part list, until all
	// parts have been read.

	parts = 0;
	part_list = NULL;
	polygons = 0;
	polygon_list = NULL;
	while ((tag_ptr = parse_next_tag(parts_tag_list, TOKEN_PARTS)) != NULL) {
		switch (tag_ptr->tag_name) {
		case TOKEN_PART:
			parse_part_tag(blockset_ptr, block_def_ptr);
		}
	}

	// Copy the part list into the block definition.  This is done in reverse
	// order because the part list is in reverse order.

	block_def_ptr->create_part_list(parts);
	part_ptr = part_list;
	for (index = parts - 1; index >= 0; index--) {
		block_def_ptr->part_list[index] = *part_ptr;
		next_part_ptr = part_ptr->next_part_ptr;
		delete part_ptr;
		part_ptr = next_part_ptr;
	}

	// Copy the polygon list into the block definition, and set the part pointer
	// for each polygon.  This is done in reverse order because the polygon list
	// is in reverse order.

	block_def_ptr->create_polygon_list(polygons);
	polygon_ptr = polygon_list;
	for (index = polygons - 1; index >= 0; index--) {
		polygon *new_polygon_ptr = &block_def_ptr->polygon_list[index];
		*new_polygon_ptr = *polygon_ptr;
		new_polygon_ptr->part_ptr = 
			&block_def_ptr->part_list[new_polygon_ptr->part_no];
		next_polygon_ptr = polygon_ptr->next_polygon_ptr;
		delete polygon_ptr;
		polygon_ptr = next_polygon_ptr;
	}
}

//------------------------------------------------------------------------------
// Parse the sprite part tag.
//------------------------------------------------------------------------------

static void
parse_sprite_part_tag(blockset *blockset_ptr, block_def *block_def_ptr)
{
	part *part_ptr;
	texture *texture_ptr;

	// If the sprite part tag has already been seen, return without doing
	// anything.

	if (got_sprite_part_tag)
		return;
	got_sprite_part_tag = true;

	// Create a single part and initialise it's name and the number of faces
	// per polygon.

	block_def_ptr->create_part_list(1);
	part_ptr = block_def_ptr->part_list;
	part_ptr->name = part_name;
	part_ptr->faces = 2;

	// If the texture parameter was given, load or retrieve the texture with the
	// given name.

	if (matched_param[PART_TEXTURE])
		texture_ptr = load_texture(blockset_ptr, part_texture, false);

	// Initialise the part's common fields.
	
	update_common_part_fields(part_ptr, texture_ptr, part_colour, 
		part_translucency, part_style);
		
	// Create the vertex and polygon lists needed to define the sprite.

	create_sprite_polygon(block_def_ptr, part_ptr);
}

//------------------------------------------------------------------------------
// Parse the structural param tag.
//------------------------------------------------------------------------------

void
parse_param_tag(block_def *block_def_ptr)
{
	// If the param tag has already been seen, return without doing anything.

	if (got_param_tag)
		return;
	got_param_tag = true;

	// If the orient parameter was not given, reset it to no orientation.

	if (!matched_param[PARAM_ORIENTATION])
		param_orientation.set(0.0f, 0.0f, 0.0f);

	// if the solid parameter was given, set the solid flag in the block
	// definition.

	if (matched_param[PARAM_SOLID])
		block_def_ptr->solid = param_solid;

	// If the movable parameter was given, set the movable flag in the block
	// definition.

	if (matched_param[PARAM_MOVABLE])
		block_def_ptr->movable = param_movable;
}

//------------------------------------------------------------------------------
// Parse the sprite param tag.
//------------------------------------------------------------------------------

void
parse_sprite_param_tag(block_def *block_def_ptr)
{
	// If the param tag has already been seen, return without doing anything.

	if (got_param_tag)
		return;
	got_param_tag = true;

	// If the angle parameter was given, set the sprite angle in the block
	// definition.

	if (matched_param[SPRITE_ANGLE])
		block_def_ptr->sprite_angle = sprite_angle;

	// If the speed parameter was given, set the rotational speed (degrees per
	// millisecond) in the block definition.

	if (matched_param[SPRITE_SPEED])
		block_def_ptr->degrees_per_ms = sprite_speed * 360.0f / 1000.0f;

	// If the align parameter was given and this isn't a player sprite, set the
	// sprite alignment.

	if (matched_param[SPRITE_ALIGNMENT] && block_def_ptr->type != PLAYER_SPRITE)
		block_def_ptr->sprite_alignment = sprite_alignment;

	// If the solid parameter was given, set the sprite's solid flag.

	if (matched_param[SPRITE_SOLID])
		block_def_ptr->solid = sprite_solid;

	// If the movable parameter was given, set the sprite's movable flag.

	if (matched_param[SPRITE_MOVABLE])
		block_def_ptr->movable = sprite_movable;

	// If the size parameter was given, set the sprite's size.

	if (matched_param[SPRITE_SIZE])
		block_def_ptr->sprite_size = sprite_size;
}

//------------------------------------------------------------------------------
// Parse a point light tag.
//------------------------------------------------------------------------------

static void
parse_point_light_tag(light *&light_ptr)
{
	// If there is no light object, create one with default settings.  If this
	// fails, just return.

	if (light_ptr == NULL) {
		NEW(light_ptr, light);
		if (light_ptr == NULL) {
			memory_warning("point light");
			return;
		}
	}

	// If the style parameter was specified, set the light style, otherwise have
	// it default to 'static'.

	if (matched_param[POINT_LIGHT_STYLE])
		light_ptr->style = point_light_style;
	else
		light_ptr->style = STATIC_POINT_LIGHT;

	// If the position parameter was specified, set the position of the light
	// relative to the block.

	if (matched_param[POINT_LIGHT_POSITION])
		light_ptr->pos = point_light_position;

	// If the brightness parameter was specified, set the intensity range of the
	// light.

	if (matched_param[POINT_LIGHT_BRIGHTNESS])
		light_ptr->set_intensity_range(point_light_brightness);

	// If the radius parameter was specified, set the radius of the light
	// source.

	if (matched_param[POINT_LIGHT_RADIUS])
		light_ptr->set_radius(point_light_radius);

	// If the speed parameter was specified, convert the speed in cycles per
	// second to percentage points per second.

	if (matched_param[POINT_LIGHT_SPEED]) {
		if (point_light_speed < 0.0f)
			warning("Light rotational speed must be greater than zero; using "
				"default speed instead");
		else
			light_ptr->set_intensity_speed(point_light_speed);
	}

	// If the flood parameter was specified, set the flood light flag.

	if (matched_param[POINT_LIGHT_FLOOD])
		light_ptr->flood = point_light_flood;

	// If the colour parameter was specified, set the colour of the light.

	if (matched_param[POINT_LIGHT_COLOUR])
		light_ptr->colour = point_light_colour;
}

//------------------------------------------------------------------------------
// Parse a spot light tag, and create a light for the given block definition.
//------------------------------------------------------------------------------

static void
parse_spot_light_tag(light *&light_ptr)
{
	// If there is no light object, create one with default settings.  If this
	// fails, just return.

	if (light_ptr == NULL) {
		NEW(light_ptr, light);
		if (light_ptr  == NULL) {
			memory_warning("spot light");
			return;
		}
	}

	// If the style parameter was specified, set the light style, otherwise have
	// it default to 'static'.

	if (matched_param[SPOT_LIGHT_STYLE])
		light_ptr->style = spot_light_style;
	else
		light_ptr->style = STATIC_SPOT_LIGHT;

	// If the position parameter was specified, set the position of the light
	// relative to the block.

	if (matched_param[SPOT_LIGHT_POSITION])
		light_ptr->pos = spot_light_position;

	// If the brightness parameter was specified, set the brightness of the
	// light.

	if (matched_param[SPOT_LIGHT_BRIGHTNESS])
		light_ptr->set_intensity(spot_light_brightness);

	// If the radius parameter was specified, set the radius of the light
	// source.

	if (matched_param[SPOT_LIGHT_RADIUS])
		light_ptr->set_radius(spot_light_radius);

	// If the direction parameter was specified, set the direction range of the
	// light, and initialise the current direction.

	if (matched_param[SPOT_LIGHT_DIRECTION])
		light_ptr->set_dir_range(spot_light_direction);

	// If the velocity parameter was specified, set the light rotational speed
	// in degrees per second for the Y and X axes.

	if (matched_param[SPOT_LIGHT_SPEED]) {
		if (spot_light_speed < 0.0f)
			warning("Light rotational speed must be greater than zero; using "
				"default speed instead");
		else
			light_ptr->set_dir_speed(spot_light_speed);
	}

	// If the cone parameter was specified, set the cone angle of the light.
	// The angle given in the cone parameter is assumed to be a diameter, not
	// a radius.

	if (matched_param[SPOT_LIGHT_CONE])
		light_ptr->set_cone_angle(spot_light_cone / 2.0f);

	// If the flood parameter was specified, set the flood light flag.

	if (matched_param[SPOT_LIGHT_FLOOD])
		light_ptr->flood = spot_light_flood;

	// If the colour parameter was specified, set the colour of the light.

	if (matched_param[SPOT_LIGHT_COLOUR])
		light_ptr->colour = spot_light_colour;
}

//------------------------------------------------------------------------------
// Parse a sound tag and create a sound for the given block definition.
//------------------------------------------------------------------------------

static void
parse_sound_tag(sound *&sound_ptr, blockset *blockset_ptr)
{
	wave *wave_ptr;

	// If sound is not enabled, or the file parameter wasn't present and no
	// sound exists for this block, then return without doing anything.

	if (!sound_on || (!matched_param[SOUND_FILE] && sound_ptr == NULL))
		return;

	// If the file parameter was given, try to load the wave file.  If this
	// fails, display a warning and return.

	if (matched_param[SOUND_FILE]) {
		if ((wave_ptr = load_wave(blockset_ptr, sound_file)) == NULL) {
			warning("Unable to download wave file from %s", sound_file);
			return;
		}
	}

	// If there is no sound object, create one with default settings.  If this
	// fails, just return.

	if (sound_ptr == NULL) {
		NEW(sound_ptr, sound);
		if (sound_ptr == NULL) {
			memory_warning("sound");
			return;
		}
	}

	// If the file parameter was given, set the pointer to the wave object in
	// the sound object.

	if (matched_param[SOUND_FILE])
		sound_ptr->wave_ptr = wave_ptr;

	// If the volume parameter was given, set the sound volume.

	if (matched_param[SOUND_VOLUME])
		sound_ptr->volume = sound_volume;

	// If the playback parameter was given, set the playback mode of the sound.

	if (matched_param[SOUND_PLAYBACK])
		sound_ptr->playback_mode = sound_playback;

	// If the delay parameter was given, set the delay range of the sound.

	if (matched_param[SOUND_DELAY])
		sound_ptr->delay_range = sound_delay;

	// If the flood parameter was given, set the flood flag.

	if (matched_param[SOUND_FLOOD])
		sound_ptr->flood = sound_flood;

	// If the rolloff parameter was given, set the sound rolloff factor.

	if (matched_param[SOUND_ROLLOFF]) {
		if (sound_rolloff < 0.0f)
			warning("Sound rolloff must be greater than zero; using default "
				"rolloff of 1.0");
		else
			sound_ptr->rolloff = sound_rolloff;
	}

	// If the radius parameter was given, and this is a flood sound or a
	// non-flood sound with a "once" or "single" playback mode, set the sound
	// radius.

	if (matched_param[SOUND_RADIUS] && (sound_ptr->flood || 
		sound_ptr->playback_mode == ONE_PLAY ||
		sound_ptr->playback_mode == SINGLE_PLAY))
		sound_ptr->radius = sound_radius;
}

//------------------------------------------------------------------------------
// Parse a popup tag and return a pointer to the popup that is created.
//------------------------------------------------------------------------------

static popup *
parse_popup_tag(void)
{
	popup *popup_ptr;

	// If the popup has no texture, stream, colour or text, then return without
	// doing anything.

	if (!matched_param[POPUP_TEXTURE] && !matched_param[POPUP_COLOUR] && 
		!matched_param[POPUP_TEXT] && !matched_param[POPUP_STREAM]) 
		return(NULL);

	// Create the popup.  If this fails, generate a warning and return.

	if ((popup_ptr = new popup) == NULL) {
		memory_warning("popup");
		return(NULL);
	}

	// If the colour or text parameter was given, create an empty foreground
	// texture object.

	if (matched_param[POPUP_COLOUR] || matched_param[POPUP_TEXT]) {
		NEW(popup_ptr->fg_texture_ptr, texture);
		if (popup_ptr->fg_texture_ptr == NULL) { 
			memory_warning("popup texture");
			delete popup_ptr;
			return(NULL);
		}
	}

	// Initialise the popup with whatever optional parameters were parsed.

	if (matched_param[POPUP_PLACEMENT])
		popup_ptr->window_alignment = popup_placement;
	if (matched_param[POPUP_RADIUS])
		popup_ptr->radius_squared = popup_radius * popup_radius;
	if (matched_param[POPUP_BRIGHTNESS])
		popup_ptr->brightness = popup_brightness;
	if (matched_param[POPUP_TEXTURE])
		popup_ptr->bg_texture_ptr = load_texture(custom_blockset_ptr,
			popup_texture, true);
	else if (matched_param[POPUP_STREAM])
		popup_ptr->bg_texture_ptr =
			create_video_texture(popup_stream, NULL, true);
	if (matched_param[POPUP_COLOUR]) {
		popup_ptr->colour = popup_colour;
		popup_ptr->transparent_background = false;
	}
	if (matched_param[POPUP_SIZE]) {
		popup_ptr->width = popup_size.width;
		popup_ptr->height = popup_size.height;
	}
	if (matched_param[POPUP_TEXT])
		popup_ptr->text = popup_text;
	if (matched_param[POPUP_TEXTCOLOUR])
		popup_ptr->text_colour = popup_textcolour;
	if (matched_param[POPUP_TEXTALIGN])
		popup_ptr->text_alignment = popup_textalign;
	if (matched_param[POPUP_IMAGEMAP]) {
		if ((popup_ptr->imagemap_ptr = find_imagemap(popup_imagemap)) == NULL)
			warning("Undefined imagemap name '%s'", popup_imagemap);
	}
	if (matched_param[POPUP_TRIGGER])
		popup_ptr->trigger_flags = popup_trigger;

	// If the brightness parameter was present, compute the brightness index
	// and adjust the popup and text colour.

	if (matched_param[POPUP_BRIGHTNESS]) {
		popup_ptr->brightness_index = get_brightness_index(popup_brightness);
		popup_ptr->colour.adjust_brightness(popup_brightness);
		popup_ptr->text_colour.adjust_brightness(popup_brightness);
	}

	// Return a pointer to the popup.

	return(popup_ptr);
}

//------------------------------------------------------------------------------
// Parse an exit tag.
//------------------------------------------------------------------------------

void
parse_exit_tag(hyperlink *&exit_ptr)
{
	// If there is no exit object, create one with default settings.

	if (exit_ptr == NULL) {
		NEW(exit_ptr, hyperlink);
		if (exit_ptr == NULL) {
			memory_warning("exit");
			return;
		}
	}

	// Initialise the exit with whatever parameter were parsed.

	exit_ptr->URL = exit_href;
	if (matched_param[EXIT_TARGET])
		exit_ptr->target = exit_target;
	if (matched_param[EXIT_TRIGGER])
		exit_ptr->trigger_flags = exit_trigger;
	if (matched_param[EXIT_TEXT])
		exit_ptr->label = exit_text;
}

//------------------------------------------------------------------------------
// Create a BSP node for the given polygon reference.  A zero polygon reference
// indicates the node does not exist, so NULL is returned.  Otherwise a new BSP
// node is created and this function called recursively to obtain the front and
// rear BSP nodes that branch off from this one.
//------------------------------------------------------------------------------

static BSP_node *
create_BSP_node(int polygon_ref, block_def *block_def_ptr)
{
	polygon *polygon_ptr;
	BSP_node *BSP_node_ptr;

	// If the polygon reference is zero, return a NULL pointer.

	if (polygon_ref == 0)
		return(NULL);

	// Make sure the polygon reference is in range, then get a pointer to the
	// referenced polygon.

	if (!check_int_range(polygon_ref, 1, block_def_ptr->polygons))
		error("Polygon reference number %d is out of range", polygon_ref);
	polygon_ptr = &block_def_ptr->polygon_list[polygon_ref - 1];

	// Create a new BSP node and initialise it.  This involves recursively
	// calling this function to obtain the BSP node for the polygon in front
	// of and behind this one.

	NEW(BSP_node_ptr, BSP_node);
	if (BSP_node_ptr == NULL)
		memory_error("BSP node");
	BSP_node_ptr->polygon_no = polygon_ref - 1;
	BSP_node_ptr->front_node_ptr = 
		create_BSP_node(polygon_ptr->front_polygon_ref, block_def_ptr);
	BSP_node_ptr->rear_node_ptr = 
		create_BSP_node(polygon_ptr->rear_polygon_ref, block_def_ptr);

	// Return this BSP node.

	return(BSP_node_ptr);
}

//------------------------------------------------------------------------------
// Parse the BSP tree tag.
//------------------------------------------------------------------------------

void
parse_BSP_tree_tag(block_def *block_def_ptr)
{
	// If we've seen the BSP tree tag before, return without doing anything.

	if (got_BSP_tree_tag)
		return;
	got_BSP_tree_tag = true;

	// Store the root polygon reference number in the block definition.

	block_def_ptr->root_polygon_ref = BSP_tree_root;
}

//------------------------------------------------------------------------------
// Parse the BLOCK file.
//------------------------------------------------------------------------------

static void
parse_block_file(blockset *blockset_ptr, block_def *block_def_ptr)
{
	tag *tag_ptr;

	// Parse the block start tag.  The block name is only set if it wasn't
	// already defined in the style file.  The type and entrance parameters
	// are optional and have default values.

	parse_start_tag(TOKEN_BLOCK, block_param_list, BLOCK_PARAMS);
	if (strlen(block_def_ptr->name) == 0)
		block_def_ptr->name = block_name;
	if (matched_param[BLOCK_TYPE])
		block_def_ptr->type = block_type;
	if (matched_param[BLOCK_ENTRANCE])
		block_def_ptr->allow_entrance = block_entrance;
	
	// If the block is a sprite...

	if (block_def_ptr->type & SPRITE_BLOCK) {
	
		// If this is a player sprite, set the sprite alignment to centre.  All
		// other sprite types default to bottom alignment.

		if (block_def_ptr->type == PLAYER_SPRITE)
			block_def_ptr->sprite_alignment = CENTRE;

		// Reset the flags indicating which tags we've seen.

		got_sprite_part_tag = false;
		got_param_tag = false;
		got_light_tag = false;
		got_sound_tag = false;
		got_exit_tag = false;

		// Parse all tags that may be present in a sprite block definition,
		// until the closing block tag is reached. 

		while ((tag_ptr = parse_next_tag(sprite_block_tag_list, TOKEN_BLOCK)) 
			!= NULL) {
			switch (tag_ptr->tag_name) {
			case TOKEN_PART:
				parse_sprite_part_tag(blockset_ptr, block_def_ptr);
				break;
			case TOKEN_PARAM:
				parse_sprite_param_tag(block_def_ptr);
				break;
			case TOKEN_POINT_LIGHT:
				if (!got_light_tag) {
					got_light_tag = true;
					parse_point_light_tag(block_def_ptr->light_ptr);
				}
				break;
			case TOKEN_SPOT_LIGHT:
				if (!got_light_tag) {
					got_light_tag = true;
					parse_spot_light_tag(block_def_ptr->light_ptr);
				}
				break;
			case TOKEN_SOUND:
				if (!got_sound_tag) {
					got_sound_tag = true;
					parse_sound_tag(block_def_ptr->sound_ptr, blockset_ptr);
				}
				break;
			case TOKEN_EXIT:
				if (!got_exit_tag) {
					got_exit_tag = true;
					parse_exit_tag(block_def_ptr->exit_ptr);
				}
			}
		}

		// Make sure the sprite part tag was present.

		if (!got_sprite_part_tag)
			error("Missing part tag in sprite block definition");
	}

	// If the block is a structural block...

	else {

		// Reset the flags indicating which tags we've seen.

		got_vertex_list = false;
		got_part_list = false;
		got_param_tag = false;
		got_light_tag = false;
		got_sound_tag = false;
		got_exit_tag = false;
		got_BSP_tree_tag = false;

		// Parse all tags that may be present in a standard block definition,
		// until the closing block tag is reached.  Note that the vertices tag
		// must appear before the parts tag, which must appear before the
		// BSP_tree tag.

		while ((tag_ptr = parse_next_tag(block_tag_list, TOKEN_BLOCK)) 
			!= NULL) {
			switch (tag_ptr->tag_name) {
			case TOKEN_VERTICES:
				parse_vertex_list(block_def_ptr);
				break;
			case TOKEN_PARTS:
				if (!got_vertex_list)
					error("Vertex list must appear before part list");
				parse_part_list(blockset_ptr, block_def_ptr);
				break;
			case TOKEN_PARAM:
				parse_param_tag(block_def_ptr);
				break;
			case TOKEN_POINT_LIGHT:
				if (!got_light_tag) {
					got_light_tag = true;
					parse_point_light_tag(block_def_ptr->light_ptr);
				}
				break;
			case TOKEN_SPOT_LIGHT:
				if (!got_light_tag) {
					got_light_tag = true;
					parse_spot_light_tag(block_def_ptr->light_ptr);
				}
				break;
			case TOKEN_SOUND:
				if (!got_sound_tag) {
					got_sound_tag = true;
					parse_sound_tag(block_def_ptr->sound_ptr, blockset_ptr);
				}
				break;
			case TOKEN_EXIT:
				if (!got_exit_tag) {
					got_exit_tag = true;
					parse_exit_tag(block_def_ptr->exit_ptr);
				}
				break;
			case TOKEN_BSP_TREE:
				parse_BSP_tree_tag(block_def_ptr);
			}
		}

		// Make sure the vertex list and part list were seen.

		if (!got_vertex_list)
			error("Vertex list was missing in block definition");
		if (!got_part_list)
			error("Part list was missing in block definition");

		// If the param tag was seen, re-orient the block definition.

		if (got_param_tag)
			block_def_ptr->orient_block_def(param_orientation);

		// Create the BSP tree for this block definition.

		block_def_ptr->BSP_tree = 
			create_BSP_node(block_def_ptr->root_polygon_ref, block_def_ptr);
	}
}

//==============================================================================
// BLOCKSET file parsing functions.
//==============================================================================

//------------------------------------------------------------------------------
// Parse the placeholder tag.
//------------------------------------------------------------------------------

void
parse_placeholder_tag(blockset *blockset_ptr)
{
	texture *texture_ptr;

	// If this blockset already has a placeholder texture defined, do nothing.

	if (blockset_ptr->placeholder_texture_ptr)
		return;

	// Download the placeholder texture now, even if it's a custom texture.
	// If the download fails, we won't use a placeholder texture.

	if ((texture_ptr = load_texture(blockset_ptr, placeholder_texture, false))
		== NULL || (texture_ptr->custom && 
		(!download_URL(texture_ptr->URL, NULL) ||
		 !load_image(texture_ptr->URL, curr_file_path, texture_ptr, false)))) {
		warning("Unable to download placeholder texture from %s", 
			placeholder_texture);
		return;
	}

	// Store the pointer to the placeholder texture in the blockset.

	blockset_ptr->placeholder_texture_ptr = texture_ptr;
}

//------------------------------------------------------------------------------
// Parse the sky tag.
//------------------------------------------------------------------------------

void
parse_sky_tag(blockset *blockset_ptr)
{
	// If this blockset already has a sky defined, do nothing.

	if (blockset_ptr->sky_defined)
		return;
	blockset_ptr->sky_defined = true;

	// Initialise the sky parameters that were given.

	if (matched_param[SKY_TEXTURE])
		blockset_ptr->sky_texture_ptr = 
			load_texture(blockset_ptr, sky_texture, false);
	else if (matched_param[SKY_STREAM])
		blockset_ptr->sky_texture_ptr = 
			create_video_texture(sky_stream, NULL, false);
	if (matched_param[SKY_COLOUR]) {
		blockset_ptr->sky_colour_set = true;
		blockset_ptr->sky_colour = sky_colour;
	}
	if (matched_param[SKY_BRIGHTNESS]) {
		blockset_ptr->sky_brightness_set = true;
		blockset_ptr->sky_brightness = sky_intensity;
	}
}

//------------------------------------------------------------------------------
// Parse the ground tag.
//------------------------------------------------------------------------------

void
parse_ground_tag(blockset *blockset_ptr)
{
	// If this blockset already has a ground defined, do nothing.

	if (blockset_ptr->ground_defined)
		return;
	blockset_ptr->ground_defined = true;

	// Initialise the ground parameters that were given.

	if (matched_param[GROUND_TEXTURE])
		blockset_ptr->ground_texture_ptr = 
			load_texture(blockset_ptr, ground_texture, false);
	else if (matched_param[GROUND_STREAM])
		blockset_ptr->ground_texture_ptr =
			create_video_texture(ground_stream, NULL, false);
	if (matched_param[GROUND_COLOUR]) {
		blockset_ptr->ground_colour_set = true;
		blockset_ptr->ground_colour = ground_colour;
	}
}

//------------------------------------------------------------------------------
// Parse the orb tag.
//------------------------------------------------------------------------------

static void
parse_orb_tag(blockset *blockset_ptr) 
{
	// If this blockset already has a orb defined, do nothing.

	if (blockset_ptr->orb_defined)
		return;
	blockset_ptr->orb_defined = true;

	// Initialise the orb parameters that were given.

	if (matched_param[ORB_TEXTURE])
		blockset_ptr->orb_texture_ptr = 
			load_texture(blockset_ptr, orb_texture, false);
	else if (matched_param[ORB_STREAM])
		blockset_ptr->orb_texture_ptr =
			create_video_texture(orb_stream, NULL, false);
	if (matched_param[ORB_POSITION]) {
		blockset_ptr->orb_direction_set = true;
		blockset_ptr->orb_direction = orb_position;
	}
	if (matched_param[ORB_BRIGHTNESS]) {
		blockset_ptr->orb_brightness_set = true;
		blockset_ptr->orb_brightness = orb_intensity;
	}
	if (matched_param[ORB_COLOUR]) {
		blockset_ptr->orb_colour_set = true;
		blockset_ptr->orb_colour = orb_colour;
	}

	// If the href parameter was given, create a hyperlink for the orb.

	if (matched_param[ORB_HREF]) {
		hyperlink *exit_ptr;

		if ((exit_ptr = new hyperlink) == NULL) {
			memory_warning("orb exit");
			return;
		}
		exit_ptr->URL = orb_href;
		if (matched_param[ORB_TARGET])
			exit_ptr->target = orb_target;
		if (matched_param[ORB_TEXT])
			exit_ptr->label = orb_text;
		blockset_ptr->orb_exit_ptr = exit_ptr;
	}
}

//------------------------------------------------------------------------------
// Parse a block tag in the style file.
//------------------------------------------------------------------------------

static void
parse_style_block_tag(blockset *blockset_ptr)
{
	block_def *new_block_def_ptr;
	string file_path;

	// If the new block symbol is a duplicate, then ignore this block.

	if (blockset_ptr->get_block_def(style_block_symbol) != NULL) {
		warning("Block defined with a duplicate symbol");
		return;
	}

	// Push (open) the block file, which is expected to reside in the
	// style ZIP archive.  If it can't be found, ignore this block.

	file_path = "blocks/";
	file_path += style_block_file;
	if (!push_ZIP_file(file_path)) {
		warning("Unable to open block file '%s'", style_block_file);
		return;
	}

	// Create a new block definition, and set it's symbol and double symbol
	// (and name if present).  If we're out of memory, ignore this block.

	if ((new_block_def_ptr = new block_def) == NULL) {
		memory_warning("block definition");
		return;
	}
	new_block_def_ptr->single_symbol = style_block_symbol;
	if (matched_param[STYLE_BLOCK_DOUBLE])
		new_block_def_ptr->double_symbol = style_block_double;
	else
		new_block_def_ptr->double_symbol = style_block_symbol;
	if (matched_param[STYLE_BLOCK_NAME])
		new_block_def_ptr->name = style_block_name;

	// Parse the block file.

	parse_block_file(blockset_ptr, new_block_def_ptr);

	// Pop (close) the block file, returning to the style file.

	pop_file();

	// If the block name is a duplicate, then delete this block, otherwise add
	// it to the blockset.

	if (blockset_ptr->get_block_def(new_block_def_ptr->name) != NULL) {
		delete new_block_def_ptr;
		warning("Block defined with a duplicate name");
	} else
		blockset_ptr->add_block_def(new_block_def_ptr);
}

//------------------------------------------------------------------------------
// Parse the blockset, creating a blockset object and returning a pointer to it.
//------------------------------------------------------------------------------

static blockset *
parse_blockset(char *blockset_URL)
{
	char *file_name_ptr, *ext_ptr;
	string blockset_name;
	blockset *blockset_ptr;
	tag *tag_ptr;

	// Extract the blockset file name, and remove the ".bset" extension.
	// This is stored as the blockset name.

	file_name_ptr = strrchr(blockset_URL, '/');
	blockset_name = file_name_ptr + 1;
	ext_ptr = strrchr(blockset_name, '.');
	blockset_name.truncate(ext_ptr - (char *)blockset_name);

	// Set the title to reflect we're trying to load a blockset.

	set_title("Loading %s blockset", blockset_name);

	// Open the blockset.

	if (!open_blockset(blockset_URL, blockset_name))
		error("Unable to open the %s block set", blockset_name);

	// Create the blockset object, and initialise it's URL and name.

	if ((blockset_ptr = new blockset) == NULL)
		memory_error("blockset");
	blockset_ptr->URL = blockset_URL;
	blockset_ptr->name = blockset_name;

	// Add a ".style" extension to the blockset name, and open the file in
	// the blockset that has this name.

	blockset_name += ".style";
	if (!push_ZIP_file(blockset_name))
		error("Unable to open file '%s' from the %s block set",
			blockset_name, blockset_ptr->name);

	// Parse the style file.

	parse_start_tag(TOKEN_STYLE, style_param_list, STYLE_PARAMS);
	while ((tag_ptr = parse_next_tag(style_tag_list, TOKEN_STYLE)) 
		!= NULL) {
		switch (tag_ptr->tag_name) {
		case TOKEN_PLACEHOLDER:
			parse_placeholder_tag(blockset_ptr);
			break;
		case TOKEN_SKY:
			parse_sky_tag(blockset_ptr);
			break;
		case TOKEN_GROUND:
			parse_ground_tag(blockset_ptr);
			break;
		case TOKEN_ORB:
			parse_orb_tag(blockset_ptr);
			break;
		case TOKEN_BLOCK:
			parse_style_block_tag(blockset_ptr);
		}
	}

	// Pop the style file and close the blockset.

	pop_file();
	close_ZIP_archive();
	
	// Return a pointer to the blockset object.

	return(blockset_ptr);
}

//==============================================================================
// SPOT file header parsing functions.
//==============================================================================

//------------------------------------------------------------------------------
// Parse the title tag, and set the spot title.
//------------------------------------------------------------------------------

static void
parse_title_tag(void)
{
	// If we've seen a title tag already, simply return without doing anything.

	if (got_title_tag)
		return;
	got_title_tag = true;

	// Set the spot title.

	spot_title = title_name;
}

//------------------------------------------------------------------------------
// Parse the blockset tag.
//------------------------------------------------------------------------------
		
static void
parse_blockset_tag(void)
{
	char *ext_ptr;
	blockset *blockset_ptr;

	// Verify the blockset URL points to a ".bset" file.

	ext_ptr = strrchr(blockset_href, '.');
	if (ext_ptr == NULL || stricmp(ext_ptr, ".bset")) {
		warning("%s is not a blockset", blockset_href);
		return;
	}

	// Indicate that a valid blockset tag was seen.

	got_blockset_tag = true;

	// If the blockset has already been loaded, there is no need to reload it.

	if (blockset_list_ptr->find_blockset(blockset_href))
		return;

	// If the blockset exists in the old blockset list, move it to the new
	// blockset list.

	if (old_blockset_list_ptr && (blockset_ptr = 
		old_blockset_list_ptr->remove_blockset(blockset_href)) != NULL) {
		blockset_list_ptr->add_blockset(blockset_ptr);
		return;
	}

	// Load and parse the blockset, then add it to the blockset list.

	blockset_ptr = parse_blockset(blockset_href);
	blockset_list_ptr->add_blockset(blockset_ptr);
}

//------------------------------------------------------------------------------
// Parse the map tag to obtain the dimensions and scale of the map.
//------------------------------------------------------------------------------

static void
parse_map_tag(void)
{
	mapcoords size;

	// If we've seen a map tag already, simply return without doing anything.

	if (got_map_tag)
		return;
	got_map_tag = true;

	// Store the map dimensions in the world object.  We add one level so that
	// there is always a level of empty space above the last defined level.

	world_ptr->columns = map_dimensions.column;
	world_ptr->rows = map_dimensions.row;
	world_ptr->levels = map_dimensions.level + 1;

	// If the scale parameter was given, set the map scale.

	if (matched_param[MAP_SCALE])
		world_ptr->set_map_scale(map_scale);

	// If the style parameter was given, set the map style.

	if (matched_param[MAP_STYLE])
		world_ptr->map_style = map_style;
}	

//------------------------------------------------------------------------------
// Parse the ambient light tag.
//------------------------------------------------------------------------------

static void
parse_ambient_light_tag(void)
{
	// If we've seen an ambient light tag already, simply return without doing
	// anything.

	if (got_ambient_light_tag)
		return;
	got_ambient_light_tag = true;

	// If the ambient colour was not specified, choose white as the default.

	if (!matched_param[AMBIENT_LIGHT_COLOUR])
		ambient_light_colour.set_RGB(255, 255, 255);

	// Set the ambient light.

	set_ambient_light(ambient_light_brightness, ambient_light_colour);
}

//------------------------------------------------------------------------------
// Parse the ambient sound tag.
//------------------------------------------------------------------------------

static void
parse_ambient_sound_tag(void)
{
	wave *wave_ptr;

	// If sound is not enabled, we've seen an ambient sound tag already, or
	// the file parameter has not been given, simply return without doing 
	// anything.

	if (!sound_on || got_ambient_sound_tag ||
		!matched_param[AMBIENT_SOUND_FILE])
		return;
	got_ambient_sound_tag = true;

	// If the file parameter was given, load the wave from the given wave file.
	// If this fails, generate a warning and return.

	if (matched_param[AMBIENT_SOUND_FILE]) {
		if ((wave_ptr = load_wave(custom_blockset_ptr, ambient_sound_file))
			== NULL) {
			warning("Unable to download wave from %s", ambient_sound_file);
			return;
		}
	}

	// Create the ambient sound with default settings.  If this fails, generate
	// a warning and return, otherwise set it's ambient flag and wave pointer.

	if ((ambient_sound_ptr = new sound) == NULL) {
		memory_warning("ambient sound");
		return;
	}
	ambient_sound_ptr->ambient = true;
	ambient_sound_ptr->wave_ptr = wave_ptr;

	// If the volume parameter was given, set the sound volume.

	if (matched_param[AMBIENT_SOUND_VOLUME])
		ambient_sound_ptr->volume = ambient_sound_volume;

	// If the playback parameter was given, set the playback mode of the sound.

	if (matched_param[AMBIENT_SOUND_PLAYBACK])
		ambient_sound_ptr->playback_mode = ambient_sound_playback;

	// If the delay parameter was given, set the delay range of the sound.

	if (matched_param[AMBIENT_SOUND_DELAY])
		ambient_sound_ptr->delay_range = ambient_sound_delay;
}

//------------------------------------------------------------------------------
// Parse the onload tag.
//------------------------------------------------------------------------------

static void
parse_onload_tag(void)
{
	// If we've seen an onload tag already, simply return without doing
	// anything.

	if (got_onload_tag)
		return;
	got_onload_tag = true;

	// Create an onload exit with default settings.

	NEW(onload_exit_ptr, hyperlink);
	if (onload_exit_ptr == NULL) {
		memory_warning("onload exit");
		return;
	}

	// Initialise the onload exit with whatever parameter were parsed.

	onload_exit_ptr->URL = onload_href;
	if (matched_param[ONLOAD_TARGET])
		onload_exit_ptr->target = onload_target;
}

//------------------------------------------------------------------------------
// Parse the stream tag.
//------------------------------------------------------------------------------

static void
parse_stream_tag(void)
{
	wave *streaming_wave_ptr;

	// If we've seen a stream tag already, or neither RP or WMP parameters were
	// given, simple return without doing anything.

	if (got_stream_tag || (!matched_param[STREAM_RP] && 
		!matched_param[STREAM_WMP]))
		return;
	got_stream_tag = true;
	stream_set = true;

	// The name is mandatory.

	name_of_stream = stream_name;

	// The URLs are optional.

	if (matched_param[STREAM_RP])
		rp_stream_URL = create_stream_URL(stream_rp);
	if (matched_param[STREAM_WMP])
		wmp_stream_URL = create_stream_URL(stream_wmp);

	// Create a new streaming wave object.

	if ((streaming_wave_ptr = new wave) == NULL) {
		memory_warning("streaming wave");
		return;
	}

	// Create and initialise the streaming ambient sound object.

	if ((streaming_sound_ptr = new sound) == NULL) {
		memory_warning("streaming sound");
		delete streaming_wave_ptr;
		return;
	}
	streaming_sound_ptr->streaming = true;
	streaming_sound_ptr->ambient = true;
	streaming_sound_ptr->wave_ptr = streaming_wave_ptr;
}

//------------------------------------------------------------------------------
// Parse all head tags.
//------------------------------------------------------------------------------

static void
parse_head_tags(void)
{
	tag *tag_ptr;

	// If the head tag has been seen already, skip over all tags inside of it
	// up to the end head tag.

	if (got_head_tag) {
		parse_next_tag(NULL, TOKEN_HEAD);
		warning("Skipped over additional head block");
		return;
	}
	got_head_tag = true;

	// Initialise some static variables.

	got_title_tag = false;
	got_blockset_tag = false;
	got_map_tag = false;
	got_ground_tag = false;
	got_ambient_light_tag = false;
	got_ambient_sound_tag = false;
	got_onload_tag = false;
	got_stream_tag = false;

	// Parse the header tags.
	
	while ((tag_ptr = parse_next_tag(head_tag_list, TOKEN_HEAD)) != NULL) {
		switch (tag_ptr->tag_name) {
		case TOKEN_TITLE:
			parse_title_tag();
			break;
		case TOKEN_BLOCKSET:
			parse_blockset_tag();
			break;
		case TOKEN_MAP:
			parse_map_tag();
			break;
		case TOKEN_PLACEHOLDER:
			parse_placeholder_tag(custom_blockset_ptr);
			break;
		case TOKEN_SKY:
			parse_sky_tag(custom_blockset_ptr);
			break;
		case TOKEN_GROUND:
			parse_ground_tag(custom_blockset_ptr);
			got_ground_tag = true;
			world_ptr->ground_level_exists = true;
			break;
		case TOKEN_ORB:
			parse_orb_tag(custom_blockset_ptr);
			break;
		case TOKEN_AMBIENT_LIGHT:
			parse_ambient_light_tag();
			break;
		case TOKEN_AMBIENT_SOUND:
			parse_ambient_sound_tag();
			break;
		case TOKEN_DEBUG:
			debug_on = true;
			break;
		case TOKEN_ONLOAD:
			parse_onload_tag();
			break;
		case TOKEN_STREAM:
			parse_stream_tag();
			break;
		}
	}

	// Check that the blockset and map tags were seen.

	if (!got_blockset_tag)
		error("No valid blockset tags were seen in header");
	if (!got_map_tag)
		error("Map tag was not seen in header");

	// If no ambient_light tag was seen, set the ambient brightness to 100% and
	// the ambient colour to white.

	if (!got_ambient_light_tag) {
		ambient_light_colour.set_RGB(255, 255, 255);
		set_ambient_light(1.0f, ambient_light_colour);
	}
}

//==============================================================================
// SPOT file body parsing functions.
//==============================================================================

//------------------------------------------------------------------------------
// Update the texture coordinates for all polygons in the given part by the
// given texture angle.
//------------------------------------------------------------------------------

static void
update_textures_in_part(block_def *block_def_ptr, part *part_ptr,
						bool recompute_tex_coords)
{
	// Step through each polygon that belongs to this part, and update it's
	// texture coordinates.

	for (int polygon_no = 0; polygon_no < block_def_ptr->polygons; 
		polygon_no++) {
		polygon *polygon_ptr = &block_def_ptr->polygon_list[polygon_no];
		if (polygon_ptr->part_ptr == part_ptr) {
			if (recompute_tex_coords)
				compute_texture_coordinates(polygon_ptr, 
					part_ptr->texture_style, part_ptr->projection, 
					block_def_ptr);
			rotate_texture_coordinates(polygon_ptr, part_ptr->texture_angle);
		}
	}
}

//------------------------------------------------------------------------------
// Parse a part tag inside a create tag.
//------------------------------------------------------------------------------

static void
parse_create_part_tag(block_def *block_def_ptr)
{
	texture *texture_ptr;
	int part_no;
	part *part_ptr;

	// If the texture parameter was given, then load the texture.

	if (matched_param[PART_TEXTURE])
		texture_ptr = load_texture(custom_blockset_ptr, part_texture, false);

	// Otherwise if the stream parameter was given, then create a video texture.
	// If the rect parameter was also given, create a video texture for that
	// rectangle only.

	else if (matched_param[PART_STREAM]) {
		if (matched_param[PART_RECT])
			texture_ptr = create_video_texture(part_stream, &part_rect, false);
		else
			texture_ptr = create_video_texture(part_stream, NULL, false);
	}

	// If the faces parameter was given, verify that it's within range; if not,
	// ignore it.

	if (matched_param[PART_FACES] && (part_faces < 0 || part_faces > 2)) {
		warning("A part must have 0, 1 or 2 faces per polygon; using "
			"default value instead");
		matched_param[PART_FACES] = false;
	}
	 
	// If the part name expression is "*", we modify all parts in the block
	// definition.
	
	if (*part_name == '*') {
		for (part_no = 0; part_no < block_def_ptr->parts; part_no++) {
			part_ptr = &block_def_ptr->part_list[part_no];

			// Update the part fields.

			update_common_part_fields(part_ptr, texture_ptr, part_colour, 
				part_translucency, part_style);
			if (matched_param[PART_FACES])
				part_ptr->faces = part_faces;
			if (matched_param[PART_ANGLE])
				part_ptr->texture_angle = part_angle;
			if (matched_param[PART_PROJECTION])
				part_ptr->projection = part_projection;
			if (matched_param[PART_SOLID])
				part_ptr->solid = part_solid;

			// Update the textures in the part.

			update_textures_in_part(block_def_ptr, part_ptr, 
				matched_param[PART_STYLE] || matched_param[PART_PROJECTION]);
		}
	}

	// Otherwise modify all matching groups in the block definition.  Undefined
	// part names are ignored.

	else {
		char *curr_part_name;

		// Step through the list of part names.

		curr_part_name = strtok(part_name, ",");
		while (curr_part_name) {

			// Search for this name in the block definition's part list.

			for (part_no = 0; part_no < block_def_ptr->parts; part_no++) {
				part_ptr = &block_def_ptr->part_list[part_no];
				if (!stricmp(curr_part_name, part_ptr->name))
					break;
			}

			// If not found, generate a warning message.

			if (part_no == block_def_ptr->parts)
				warning("Undefined part name '%s'", curr_part_name);

			// Otherwise update the part fields and textures in the part.

			else {
				update_common_part_fields(part_ptr, texture_ptr, part_colour, 
					part_translucency, part_style);
				if (matched_param[PART_FACES])
					part_ptr->faces = part_faces;
				if (matched_param[PART_ANGLE])
					part_ptr->texture_angle = part_angle;
				if (matched_param[PART_PROJECTION])
					part_ptr->projection = part_projection;
				if (matched_param[PART_SOLID])
					part_ptr->solid = part_solid;
				update_textures_in_part(block_def_ptr, part_ptr,
					matched_param[PART_STYLE] || matched_param[PART_PROJECTION]);
			}
			curr_part_name = strtok(NULL, ",");
		}
	}
}

//------------------------------------------------------------------------------
// Parse a replace tag.
//------------------------------------------------------------------------------

static action *
parse_replace_tag(void)
{
	action *action_ptr;

	// Create an action object, and initialise it's type and next action
	// pointer.

	if ((action_ptr = new action) == NULL) {
		memory_warning("action");
		return(NULL);
	}
	action_ptr->type = REPLACE_ACTION;
	action_ptr->next_action_ptr = NULL;

	// Set the source block symbol or location.  If the source location is
	// absolute and a ground tag was seen, increment the source level.

	if (got_ground_tag && !replace_source.is_symbol && 
		!replace_source.location.relative_level)
		replace_source.location.level++;
	action_ptr->source = replace_source;

	// If the target parameter was given, set the target block location,
	// otherwise make the target location be the trigger location.

	if (matched_param[REPLACE_TARGET]) {
		if (got_ground_tag && !replace_target.relative_level)
				replace_target.level++;
		action_ptr->target_is_trigger = false;
		action_ptr->target = replace_target;
	} else
		action_ptr->target_is_trigger = true;
	
	// Return a pointer to the action.

	return(action_ptr);
}

//------------------------------------------------------------------------------
// Parse an exit tag in an action tag.
//------------------------------------------------------------------------------

static action *
parse_action_exit_tag(void)
{
	action *action_ptr;

	// If we've already seen an exit tag in this action tag, ignore this one.

	if (got_action_exit_tag)
		return(NULL);
	got_action_exit_tag = true;

	// Create an action object, and initialise it's type and next action
	// pointer.

	if ((action_ptr = new action) == NULL) {
		memory_warning("action");
		return(NULL);
	}
	action_ptr->type = EXIT_ACTION;
	action_ptr->next_action_ptr = NULL;
	
	// Initialise the action with whatever parameter were parsed.

	action_ptr->exit_URL = exit_href;
	if (matched_param[EXIT_TARGET])
		action_ptr->exit_target = exit_target;

	// Return a pointer to the action.

	return(action_ptr);
}

//------------------------------------------------------------------------------
// Parse a set_stream tag.
//------------------------------------------------------------------------------

/*
static action *
parse_set_stream_tag(void)
{
	action *action_ptr;

	// Create an action object, and initialise it's type and next action
	// pointer.

	if ((action_ptr = new action) == NULL) {
		memory_warning("action");
		return(NULL);
	}
	action_ptr->type = SET_STREAM_ACTION;
	action_ptr->next_action_ptr = NULL;
	
	// Initialise the action.

	action_ptr->stream_name = set_stream_name;
	action_ptr->stream_command = set_stream_command;

	// Return a pointer to the action.

	return(action_ptr);
}
*/

//------------------------------------------------------------------------------
// Parse an action tag.
//------------------------------------------------------------------------------

static trigger *
parse_action_tag(void)
{
	trigger *trigger_ptr;
	tag *tag_ptr;
	action *action_ptr, *last_action_ptr, *action_list;

	// Create a trigger.  If this fails, skip over the rest of the action tag.

	NEW(trigger_ptr, trigger);
	if (trigger_ptr == NULL) {
		memory_warning("trigger");
		parse_next_tag(NULL, TOKEN_ACTION);
		return(NULL);
	}

	// If the trigger parameter was given, set the trigger flag.

	if (matched_param[ACTION_TRIGGER])
		trigger_ptr->trigger_flag = action_trigger;

	// If the radius parameter was given, set the trigger radius, otherwise
	// use a default radius of one block.

	if (matched_param[ACTION_RADIUS])
		trigger_ptr->radius_squared = action_radius * action_radius;
	else
		trigger_ptr->radius_squared = world_ptr->block_units * 
			world_ptr->block_units;

	// If the delay parameter was given, set the delay range of the trigger.

	if (matched_param[ACTION_DELAY])
		trigger_ptr->delay_range = action_delay;

	// If the text parameter as given, set the label of the trigger.

	if (matched_param[ACTION_TEXT])
		trigger_ptr->label = action_text;

	// If the target parameter was given, set the target location.

	if (matched_param[ACTION_TARGET]) {
		if (got_ground_tag)
			action_target.level++;
		trigger_ptr->target = action_target;
	}

	// Initialise the action list.

	action_list = NULL;
	last_action_ptr = NULL;

	// We haven't seen an exit tag yet.

	got_action_exit_tag = false;

	// Parse all action tags.

	while ((tag_ptr = parse_next_tag(action_tag_list, TOKEN_ACTION)) != NULL) {

		// Parse the action tag.

		action_ptr = NULL;
		switch (tag_ptr->tag_name) {
		case TOKEN_REPLACE:
			action_ptr = parse_replace_tag();
			break;
		case TOKEN_EXIT:
			action_ptr = parse_action_exit_tag();
			break;
//		case TOKEN_SET_STREAM:
//			action_ptr = parse_set_stream_tag();
		}

		// Add the action to the end of the action list.

		if (action_ptr) {
			if (last_action_ptr)
				last_action_ptr->next_action_ptr = action_ptr;
			else
				action_list = action_ptr;
			last_action_ptr = action_ptr;
		}
	}

	// Store pointer to action list in trigger.

	trigger_ptr->action_list = action_list;

	// Return a pointer to the trigger.

	return(trigger_ptr);
}

//------------------------------------------------------------------------------
// Parse the create tag.
//------------------------------------------------------------------------------

static void
parse_create_tag(void)
{
	char single_symbol;
	word double_symbol;
	block_def *custom_block_def_ptr, *block_def_ptr;
	tag *create_tag_list, *tag_ptr;
	trigger *trigger_ptr, *last_trigger_ptr, *trigger_list;
	int trigger_flags;
	popup *popup_ptr;

	// Check whether the source block exists; if it doesn't, skip over the
	// rest of the create tag and return.

	if (string_to_single_symbol(create_block, &single_symbol, true)) {
		if ((block_def_ptr = get_block_def(single_symbol)) == NULL) {
			warning("Undefined block '%s'", create_block);
			parse_next_tag(NULL, TOKEN_CREATE);
			return;
		}
	} else if (string_to_double_symbol(create_block, &double_symbol, true)) {
		if ((block_def_ptr = get_block_def(double_symbol)) == NULL) {
			warning("Undefined block '%s'", create_block);
			parse_next_tag(NULL, TOKEN_CREATE);
			return;
		}
	} else if ((block_def_ptr = get_block_def(create_block)) == NULL) {
		warning("Undefined block '%s'", create_block);
		parse_next_tag(NULL, TOKEN_CREATE);
		return;
	}

	// If the custom block symbol exists, delete the custom block definition
	// currently assigned to it, removing it from the symbol table as well.

	if (custom_blockset_ptr->delete_block_def(create_symbol)) {
		warning("Created block with duplicate symbol; this block definition "
			"will replace the previous block definition");
		symbol_table[create_symbol] = NULL;
	}

	// Allocate a new custom block definition structure; if this fails, skip
	// over the rest of the create tag and return.

	if ((custom_block_def_ptr = new block_def) == NULL) {
		memory_warning("custom block definition");
		parse_next_tag(NULL, TOKEN_CREATE);
		return;
	}

	// Copy the source block definition to the target block definition, set
	// it's symbol, and add it to the custom blockset and symbol table.

	custom_block_def_ptr->dup_block_def(block_def_ptr);
	switch (world_ptr->map_style) {
	case SINGLE_MAP:
		custom_block_def_ptr->single_symbol = (char)create_symbol;
		break;
	case DOUBLE_MAP:
		custom_block_def_ptr->double_symbol = create_symbol;
	}
	custom_blockset_ptr->add_block_def(custom_block_def_ptr);
	symbol_table[create_symbol] = custom_block_def_ptr;

	// Initialise some flags.

	got_param_tag = false;
	got_light_tag = false;
	got_sound_tag = false;
	got_exit_tag = false;

	// Initialise the trigger flags and trigger list.
 
	trigger_flags = 0;
	trigger_list = NULL;
	last_trigger_ptr = NULL;

	// Choose the appropiate create tag list.

	if (custom_block_def_ptr->type & SPRITE_BLOCK)
		create_tag_list = sprite_create_tag_list;
	else
		create_tag_list = structural_create_tag_list;

	// Parse all create tags.

	while ((tag_ptr = parse_next_tag(create_tag_list, TOKEN_CREATE)) != NULL) {
		switch (tag_ptr->tag_name) {
		case TOKEN_PART:
			parse_create_part_tag(custom_block_def_ptr);
			break;
		case TOKEN_PARAM:
			if (custom_block_def_ptr->type & SPRITE_BLOCK) 
				parse_sprite_param_tag(custom_block_def_ptr);
			else
				parse_param_tag(custom_block_def_ptr);
			break;
		case TOKEN_POINT_LIGHT:
			if (!got_light_tag) {
				got_light_tag = true;
				parse_point_light_tag(custom_block_def_ptr->light_ptr);
			}
			break;
		case TOKEN_SPOT_LIGHT:
			if (!got_light_tag) {
				got_light_tag = true;
				parse_spot_light_tag(custom_block_def_ptr->light_ptr);
			}
			break;
		case TOKEN_SOUND:
			if (!got_sound_tag) {
				got_sound_tag = true;
				parse_sound_tag(custom_block_def_ptr->sound_ptr,
					custom_blockset_ptr);
			}
			break;
		case TOKEN_POPUP:
			if (custom_block_def_ptr->popup_ptr == NULL) {
				popup_ptr = parse_popup_tag();
				popup_ptr->next_popup_ptr = original_popup_list;
				original_popup_list = popup_ptr;
				custom_block_def_ptr->popup_ptr = popup_ptr;
			}
			break;
		case TOKEN_ENTRANCE:
			if (strlen(custom_block_def_ptr->entrance_name) == 0) {
				if (custom_block_def_ptr->allow_entrance) {
					custom_block_def_ptr->entrance_name = entrance_name;
					if (matched_param[ENTRANCE_ANGLE])
						custom_block_def_ptr->initial_direction = 
							entrance_angle;
				} else
					warning("Block '%s' does not permit an entrance",
						block_def_ptr->name);
			}
			break;
		case TOKEN_EXIT:
			if (!got_exit_tag) {
				got_exit_tag = true;
				parse_exit_tag(custom_block_def_ptr->exit_ptr);
			}
			break;
		case TOKEN_ACTION:

			// Parse the ACTION tag and create a trigger.

			trigger_ptr = parse_action_tag();

			// Add the trigger the end of the trigger list.

			if (trigger_ptr) {
				if (last_trigger_ptr)
					last_trigger_ptr->next_trigger_ptr = trigger_ptr;
				else
					trigger_list = trigger_ptr;
				last_trigger_ptr = trigger_ptr;
			}

			// Update the trigger flags so we know what kind of triggers are
			// in the list.

			trigger_flags |= trigger_ptr->trigger_flag;
		}
	}

	// Store the trigger flags and trigger list in the custom block definition.

	custom_block_def_ptr->trigger_flags = trigger_flags;
	custom_block_def_ptr->trigger_list = trigger_list;

	// If the param tag was seen and this is a structural block definition,
	// re-orient it.

	if (got_param_tag && custom_block_def_ptr->type == STRUCTURAL_BLOCK)
		custom_block_def_ptr->orient_block_def(param_orientation);
}

//------------------------------------------------------------------------------
// Parse player tag.
//------------------------------------------------------------------------------

static void
parse_player_tag(void)
{
	// If we've seen the player tag before, just return without doing anything.

	if (got_player_tag)
		return;
	got_player_tag = true;

	// Save the block symbol for later.

	player_block_symbol = player_block;

	// Save the camera offset if it were given, otherwise select a default
	// camera offset.

	if (matched_param[PLAYER_CAMERA]) {
		player_camera_offset.dx = player_camera.x;
		player_camera_offset.dy = player_camera.y;
		player_camera_offset.dz = player_camera.z;
	} else {
		player_camera_offset.dx = 0.0f;
		player_camera_offset.dy = 0.0f;
		player_camera_offset.dz = -1.0f;
	}
}

//------------------------------------------------------------------------------
// Parse entrance tag.
//------------------------------------------------------------------------------

static void
parse_entrance_tag()
{
	// If the ground tag was seen in the header, the entrance level must be
	// incremented by one.

	if (got_ground_tag)
		entrance_location.level++;

	// If the angle parameter wasn't specified, set the entrance direction
	// to 0,0.

	if (!matched_param[ENTRANCE_ANGLE])
		entrance_angle.set(0.0f, 0.0f);
 
	// Add the entrance to the entrance list.

	add_entrance(entrance_name, entrance_location.column, entrance_location.row,
		entrance_location.level, entrance_angle, false);
}

//------------------------------------------------------------------------------
// Parse rectangle coordinates. 
//------------------------------------------------------------------------------

static bool
parse_rect_coords(char *rect_coords, rect *rect_ptr)
{
	start_parsing_value(rect_coords);
	if (!parse_integer_in_value(&rect_ptr->x1) || !token_in_value_is(",") ||
		!parse_integer_in_value(&rect_ptr->y1) || !token_in_value_is(",") ||
		!parse_integer_in_value(&rect_ptr->x2) || !token_in_value_is(",") ||
		!parse_integer_in_value(&rect_ptr->y2) || !stop_parsing_value()) {
		warning("Expected rectangle coordinates for the value of attribute "
			"'shape'");
		return(false);
	}
	return(true);
}

//------------------------------------------------------------------------------
// Parse circle coordinates. 
//------------------------------------------------------------------------------

static bool
parse_circle_coords(char *circle_coords, circle *circle_ptr)
{
	start_parsing_value(circle_coords);
	if (!parse_integer_in_value(&circle_ptr->x) || !token_in_value_is(",") ||
		!parse_integer_in_value(&circle_ptr->y) || !token_in_value_is(",") ||
		!parse_integer_in_value(&circle_ptr->r) || !stop_parsing_value()) {
		warning("Expected circle coordinates and radius for the value of "
			"attribute 'shape'");
		return(false);
	}
	return(true);
}

//------------------------------------------------------------------------------
// Parse area tag.
//------------------------------------------------------------------------------

static void
parse_area_tag(imagemap *imagemap_ptr)
{
	rect rect_shape;
	circle circle_shape;
	hyperlink *exit_ptr;

	// If the target or text parameters were not seen, set them to empty
	// strings.

	if (!matched_param[AREA_TARGET])
		area_target = "";
	if (!matched_param[AREA_TEXT])
		area_text = "";

	// Create and initialise the exit object.  If this fails, ignore this area
	// tag.

	if ((exit_ptr = new hyperlink) == NULL)
		return;
	exit_ptr->set(area_href, area_target, area_text);

	// Parse the coords parameter based upon the area shape, then add the
	// area to the imagemap.

	switch (area_shape) {
	case RECT_SHAPE:
		if (parse_rect_coords(area_coords, &rect_shape))
			imagemap_ptr->add_area(&rect_shape, exit_ptr);
		break;
	case CIRCLE_SHAPE:
		if (parse_circle_coords(area_coords, &circle_shape))
			imagemap_ptr->add_area(&circle_shape, exit_ptr);
	}
}

//------------------------------------------------------------------------------
// Parse an imagemap action tag.
//------------------------------------------------------------------------------

static void
parse_imagemap_action_tag(imagemap *imagemap_ptr)
{
	trigger *trigger_ptr;
	tag *tag_ptr;
	action *action_ptr, *last_action_ptr, *action_list;
	rect rect_shape;
	circle circle_shape;

	// Create a trigger.  If this fails, skip over the rest of the action
	// tag.

	if ((trigger_ptr = new trigger) == NULL) {
		memory_warning("trigger");
		parse_next_tag(NULL, TOKEN_ACTION);
		return;
	}

	// If the trigger parameter was given, set the trigger flag.  Only
	// "roll on", "roll off" and "click on" triggers are permitted; if any
	// other trigger is given, pretend the trigger parameter wasn't seen.

	if (matched_param[ACTION_TRIGGER]) {
		if (action_trigger != ROLL_ON && action_trigger != ROLL_OFF &&
			action_trigger != CLICK_ON)
			warning("Expected 'roll on', 'roll off' or 'click on' as the "
				"value for attribute 'trigger'; using default value for this "
				"attribute instead");
		else
			trigger_ptr->trigger_flag = action_trigger;
	}

	// If the text parameter was given, set the trigger label.

	if (matched_param[ACTION_TEXT])
		trigger_ptr->label = action_text;

	// Initialise the action list.

	action_list = NULL;
	last_action_ptr = NULL;

	// We haven't seen an exit tag yet.

	got_action_exit_tag = false;

	// Parse all action tags.

	while ((tag_ptr = parse_next_tag(action_tag_list, TOKEN_ACTION)) != NULL) {

		// Parse the action tag.

		action_ptr = NULL;
		switch (tag_ptr->tag_name) {
		case TOKEN_REPLACE:
			action_ptr = parse_replace_tag();
			break;
		case TOKEN_EXIT:
			action_ptr = parse_action_exit_tag();
			break;
//		case TOKEN_SET_STREAM:
//			action_ptr = parse_set_stream_tag();
		}

		// Add the action to the end of the action list.

		if (action_ptr) {
			if (last_action_ptr)
				last_action_ptr->next_action_ptr = action_ptr;
			else
				action_list = action_ptr;
			last_action_ptr = action_ptr;
		}
	}

	// Store pointer to action list in trigger.

	trigger_ptr->action_list = action_list;

	// Parse the coords parameter based upon the area shape, then add the
	// area with the trigger to the imagemap.

	switch (action_shape) {
	case RECT_SHAPE:
		if (parse_rect_coords(action_coords, &rect_shape))
			imagemap_ptr->add_area(&rect_shape, trigger_ptr);
		break;
	case CIRCLE_SHAPE:
		if (parse_circle_coords(action_coords, &circle_shape))
			imagemap_ptr->add_area(&circle_shape, trigger_ptr);
	}
}

//------------------------------------------------------------------------------
// Parse imagemap tag.
//------------------------------------------------------------------------------

static void
parse_imagemap_tag(void)
{
	imagemap *imagemap_ptr;
	tag *tag_ptr;

	// Create a new imagemap object and add it to the imagemap list.

	imagemap_ptr = add_imagemap(imagemap_name);

	// Parse all imagemap tags.

	while ((tag_ptr = parse_next_tag(imagemap_tag_list, TOKEN_IMAGEMAP)) 
		!= NULL) {
		switch (tag_ptr->tag_name) {
		case TOKEN_AREA:
			parse_area_tag(imagemap_ptr);
			break;
		case TOKEN_ACTION:
			parse_imagemap_action_tag(imagemap_ptr);
		}
	}
}

//------------------------------------------------------------------------------
// Parse the level tag.
//------------------------------------------------------------------------------

static void
parse_level_tag(void)
{
	int expected_levels;
	int column, row;
	int column_offset;

	// If we've already read all levels, skip over this level.

	if (got_ground_tag)
		expected_levels = world_ptr->levels - 2;
	else
		expected_levels = world_ptr->levels - 1;
	if (curr_level == expected_levels) {
		parse_next_tag(NULL, TOKEN_LEVEL);
		warning("Skipped over level %d", curr_level + 1);
		return;
	}

	// If the number parameter was given, verify it is in sequence; if it's
	// not, just generate a warning and continue.

	if (matched_param[LEVEL_NUMBER] && level_number != curr_level + 1)
		warning("Level number %d is out of sequence (should be %d)",
			level_number, curr_level + 1);

	// Now read all rows in this level.

	read_line();
	for (row = 0; row < world_ptr->rows; row++) {
		square *row_ptr;
		char *line_ptr;
		char ch1, ch2;
		
		// Get a pointer to the row in the map.  This will be one more than
		// the current level number if the ground tag was seen in the header.

		if (got_ground_tag)
			row_ptr = world_ptr->get_square_ptr(0, row, curr_level + 1);
		else
			row_ptr = world_ptr->get_square_ptr(0, row, curr_level);

		// Skip over leading white space in the line.

		line_ptr = line_buffer;
		ch1 = *line_ptr;
		while (ch1 == ' ' || ch1 == '\t')
			ch1 = *++line_ptr;

		// If the first character is "<", then assume this is the level end tag,
		// meaning there are no more rows to parse.

		if (ch1 == '<')
			break;
			
		// Parse the block single or double character symbols in this row,
		// until the expected number of symbols have been parsed or the end of
		// the line has been reached.  Whitespace is ignored.

		column = 0;
		switch (world_ptr->map_style) {
		case SINGLE_MAP:
			while (ch1 && column < world_ptr->columns) {
				if (not_single_symbol(ch1, false))
					row_ptr->block_symbol = NULL_BLOCK_SYMBOL;
				else
					row_ptr->block_symbol = ch1;
				row_ptr++;
				column++;
				ch1 = *++line_ptr;
				while (ch1 == ' ' || ch1 == '\t')
					ch1 = *++line_ptr;
			}
			break;
		case DOUBLE_MAP:
			while (ch1 && column < world_ptr->columns) {
				ch2 = *++line_ptr;
				if (not_double_symbol(ch1, ch2, false))
					row_ptr->block_symbol = NULL_BLOCK_SYMBOL;
				else if (ch1 == '.')
					row_ptr->block_symbol = ch2;
				else
					row_ptr->block_symbol = (ch1 << 7) + ch2;
				row_ptr++;
				column++;
				if (!ch2)
					break;
				ch1 = *++line_ptr;
				while (ch1 == ' ' || ch1 == '\t')
					ch1 = *++line_ptr;
			}
		}

		// Read the next row of map symbols.

		read_line();
	}

	// Skip over any additional rows that are not part of the map.

	while (true) {

		// Skip over any leading white space.

		column_offset = 0;
		while (line_buffer[column_offset] == ' ' || 
			line_buffer[column_offset] == '\t')
			column_offset++;

		// If the first symbol is "<", then assume this is the level end tag,
		// meaning there are no more rows to parse.

		if (line_buffer[column_offset] == '<')
			break;

		// Read the next row of map symbols.

		read_line();
	}

	// Verify the level end tag is present, and increment the level number.
	
	parse_end_tag(TOKEN_LEVEL);
	curr_level++;
}

//------------------------------------------------------------------------------
// Parse the load tag.
//------------------------------------------------------------------------------

static void
parse_load_tag(void)
{
	// Either the texture or sound parameter must be present, but not both.

	if ((!matched_param[LOAD_TEXTURE] && !matched_param[LOAD_SOUND]) ||
		(matched_param[LOAD_TEXTURE] && matched_param[LOAD_SOUND])) {
		warning("Expected one 'texture' or 'sound' parameter; "
			"ignoring 'load' tag");
		return;
	}

	// Load the specified texture or wave URL.

	if (matched_param[LOAD_TEXTURE])
		load_texture(custom_blockset_ptr, load_texture_href, false);
	else
		load_wave(custom_blockset_ptr, load_sound_href);
}

//------------------------------------------------------------------------------
// Parse all body tags.
//------------------------------------------------------------------------------

static void
parse_body_tags(void)
{
	blockset *blockset_ptr;
	block_def *block_def_ptr;
	word block_symbol;
	tag *tag_ptr;
	square *square_ptr;
	trigger *trigger_ptr, *last_trigger_ptr;

	// If the body has been seen before, skip over the rest of this body and
	// return.

	if (got_body_tag) {
		parse_next_tag(NULL, TOKEN_BODY);
		warning("Skipped over additional body block");
		return;
	}
	got_body_tag = true;

	// Display a message in the title indicating that we're loading the map.

	set_title("Loading map");

	// Add the block definitions in the blocksets to the symbol table.  Once a
	// symbol has been allocated, it cannot be reassigned to a block definition
	// in a later blockset.

	blockset_ptr = blockset_list_ptr->first_blockset_ptr;
	while (blockset_ptr) {
		block_def_ptr = blockset_ptr->block_def_list;
		while (block_def_ptr) {
			switch (world_ptr->map_style) {
			case SINGLE_MAP:
				block_symbol = block_def_ptr->single_symbol;
				break;
			case DOUBLE_MAP:
				block_symbol = block_def_ptr->double_symbol;
			}
			if (symbol_table[block_symbol] == NULL)
				symbol_table[block_symbol] = block_def_ptr;
			block_def_ptr = block_def_ptr->next_block_def_ptr;
		}
		blockset_ptr = blockset_ptr->next_blockset_ptr;
	}

	// If the ground tag was seen in the header, add an additional level to the
	// map dimensions.

	if (got_ground_tag)
		world_ptr->levels++;

	// Create the square map.
	
	if (!world_ptr->create_square_map())
		memory_error("square map");

#ifdef SUPPORT_A3D

	// If A3D sound is enabled, create the audio square map.

	if (sound_on && !world_ptr->create_audio_square_map())
		memory_error("audio square map");

#endif

	// If the ground tag was seen in the header, initialise the bottommost
	// level with ground blocks.

	if (got_ground_tag) {
		int row, column;

		for (row = 0; row < world_ptr->rows; row++)
			for (column = 0; column < world_ptr->columns; column++) {
				square *square_ptr = world_ptr->get_square_ptr(column, row, 0);
				square_ptr->block_symbol = GROUND_BLOCK_SYMBOL;
			}
	}

	// Initialise some state variables.

	got_player_tag = false;
	curr_level = 0;

	// Parse the body tags.

	while ((tag_ptr = parse_next_tag(body_tag_list, TOKEN_BODY)) != NULL) {
		popup *popup_ptr;
		light *light_ptr;
		sound *sound_ptr;
		vertex translation;

		switch (tag_ptr->tag_name) {
		case TOKEN_CREATE:
			parse_create_tag();
			break;
		case TOKEN_PLAYER:
			parse_player_tag();
			break;
		case TOKEN_ENTRANCE:
			parse_entrance_tag();
			break;
		case TOKEN_EXIT:

			// If the ground tag was seen in the header, the exit level must be
			// incremented by one.

			if (got_ground_tag)
				exit_location.level++;

			// If the square at the given map coordinates does not already have
			// an exit, then parse the exit tag.

			square_ptr = world_ptr->get_square_ptr(exit_location.column,
				exit_location.row, exit_location.level);
			if (square_ptr->exit_ptr == NULL) {
				parse_exit_tag(square_ptr->exit_ptr);
				if (square_ptr->exit_ptr)
					square_ptr->exit_ptr->from_block = false;
			}
			break;
		case TOKEN_IMAGEMAP:
			parse_imagemap_tag();
			break;

		case TOKEN_POPUP:

			// If the ground tag was seen in the header, then the popup level
			// must be incremented by one.

			if (got_ground_tag)
				popup_location.level++;

			// Parse the popup tag.

			popup_ptr = parse_popup_tag();
			if (popup_ptr == NULL)
				break;

			// Add the popup to the original popup list.

			popup_ptr->next_popup_ptr = original_popup_list;
			original_popup_list = popup_ptr;

			// Make a copy of the popup.

			popup_ptr = dup_popup(popup_ptr);
			if (popup_ptr == NULL)
				break;
			popup_ptr->next_popup_ptr = NULL;

			// Get a pointer to the popup's square, and store it in the popup.

			square_ptr = world_ptr->get_square_ptr(popup_location.column,
				popup_location.row, popup_location.level);
			popup_ptr->square_ptr = square_ptr;
			popup_ptr->from_block = false;

			// Add a copy of the popup to the end of the global popup list.

			if (last_global_popup_ptr)
				last_global_popup_ptr->next_popup_ptr = popup_ptr;
			else
				global_popup_list = popup_ptr;
			last_global_popup_ptr = popup_ptr;

			// If the location parameter was given, set the popup's scaled
			// map position, and add the popup to the end of the 
			// corresponding square's popup list.
			
			if (matched_param[POPUP_LOCATION]) {
				popup_ptr->position.set_scaled_map_position(
					popup_location.column, popup_location.row, 
					popup_location.level);
				square_ptr = world_ptr->get_square_ptr(
					popup_location.column, popup_location.row, 
					popup_location.level);
				if (square_ptr->last_popup_ptr)
					square_ptr->last_popup_ptr->next_square_popup_ptr = 
						popup_ptr;
				else
					square_ptr->popup_list = popup_ptr;
				square_ptr->last_popup_ptr = popup_ptr;

				// Update the square's popup trigger flags.

				square_ptr->popup_trigger_flags |= popup_ptr->trigger_flags;
			} 
				
			// Otherwise set the always_visible flag.

			else
				popup_ptr->always_visible = true;
			break;

		case TOKEN_POINT_LIGHT:

			// If the ground tag was seen in the header, then the point light
			// level must be incremented by one.

			if (got_ground_tag)
				point_light_location.level++;

			// Get a new light object, and parse the point light tag.

			if ((light_ptr = new_light()) == NULL)
				break;
			parse_point_light_tag(light_ptr);

			// Get a pointer to the light's square, and store it in the light
			// object.

			square_ptr = world_ptr->get_square_ptr(point_light_location.column,
				point_light_location.row, point_light_location.level);
			light_ptr->square_ptr = square_ptr;
			light_ptr->from_block = false;

			// Add the light to the global light list, after translating it's
			// position by the map coordinates, and then scaling it.

			translation.set_map_translation(point_light_location.column, 
				point_light_location.row, point_light_location.level);
			light_ptr->pos = (light_ptr->pos + translation) * 
				world_ptr->block_scale;
			light_ptr->next_light_ptr = global_light_list;
			global_light_list = light_ptr;
			global_lights++;
			break;

		case TOKEN_SPOT_LIGHT:

			// If the ground tag was seen in the header, then the spot light
			// level must be incremented by one.

			if (got_ground_tag)
				spot_light_location.level++;

			// Get a new light object, and parse the spot light tag.

			if ((light_ptr = new_light()) == NULL)
				break;
			parse_spot_light_tag(light_ptr);

			// Get a pointer to the light's square, and store it in the light
			// object.

			square_ptr = world_ptr->get_square_ptr(point_light_location.column,
				point_light_location.row, point_light_location.level);
			light_ptr->square_ptr = square_ptr;
			light_ptr->from_block = false;

			// Add the spot light to the global light list, after translating
			// it's position by the map coordinates and then scaling it.

			translation.set_map_translation(spot_light_location.column, 
				spot_light_location.row, spot_light_location.level);
			light_ptr->pos = (light_ptr->pos + translation) *
				world_ptr->block_scale;
			light_ptr->next_light_ptr = global_light_list;
			global_light_list = light_ptr;
			global_lights++;
			break;

		case TOKEN_SOUND:

			// If the ground tag was seen in the header, then the sound
			// level must be incremented by one.

			if (got_ground_tag)
				sound_location.level++;

			// Get new sound object and parse the sound tag.

			if ((sound_ptr = new_sound()) == NULL)
				break;
			parse_sound_tag(sound_ptr, custom_blockset_ptr);
			
			// Get a pointer to the sound's square, and store it in the sound
			// object.

			square_ptr = world_ptr->get_square_ptr(sound_location.column,
				sound_location.row, sound_location.level);
			sound_ptr->square_ptr = square_ptr;
			sound_ptr->from_block = false;

			// Add the sound to the global sound list, after translating it's
			// position by the map coordinates, and then scaling it.

			translation.set_map_translation(sound_location.column, 
				sound_location.row, sound_location.level);
			sound_ptr->position = (sound_ptr->position + translation) *
				world_ptr->block_scale;
			sound_ptr->next_sound_ptr = global_sound_list;
			global_sound_list = sound_ptr;
			break;

		case TOKEN_LEVEL:
			parse_level_tag();
			break;

		case TOKEN_ACTION:

			// If the ground tag was seen in the header, the action level
			// must be incremented by one.

			if (got_ground_tag)
				action_location.level++;

			// Parse the ACTION tag and create a trigger.

			trigger_ptr = parse_action_tag();
			if (trigger_ptr) {

				// Get a pointer to the action location's square, and store
				// it in the trigger.

				square_ptr = world_ptr->get_square_ptr(action_location.column,
					action_location.row, action_location.level);
				trigger_ptr->square_ptr = square_ptr;

				// Add the trigger to the end of the square's trigger list.

				last_trigger_ptr = square_ptr->last_square_trigger_ptr;
				if (last_trigger_ptr)
					last_trigger_ptr->next_trigger_ptr = trigger_ptr;
				else
					square_ptr->square_trigger_list = trigger_ptr;
				square_ptr->last_square_trigger_ptr = trigger_ptr;

				// Update the square's trigger flags.

				square_ptr->square_trigger_flags |= trigger_ptr->trigger_flag;

				// If this is a "step in", "step out", "proximity", "timer" or
				// "location" trigger, add a copy of it to the global trigger
				// list.

				if (trigger_ptr->trigger_flag == STEP_IN ||
					trigger_ptr->trigger_flag == STEP_OUT ||
					trigger_ptr->trigger_flag == PROXIMITY ||
					trigger_ptr->trigger_flag == TIMER ||
					trigger_ptr->trigger_flag == LOCATION)
					add_trigger_to_global_list(trigger_ptr, square_ptr,
						action_location.column, action_location.row,
						action_location.level, false);
			}
			break;

		case TOKEN_LOAD:
			parse_load_tag();
		}
	}
}

//==============================================================================
// Parse the SPOT file.
//==============================================================================

void
parse_spot_file(char *spot_URL, char *spot_file_path)
{
	tag *tag_ptr;
	int expected_levels;
	FILE *fp;

	// Reset the current load index for textures and waves.

	curr_load_index = 0;

	// Reset the stream flag, name, URLs, unscaled video texture,
	// and scaled video texture list.

	stream_set = false;
	name_of_stream = "";
	rp_stream_URL = "";
	wmp_stream_URL = "";
	unscaled_video_texture_ptr = NULL;
	scaled_video_texture_list = NULL;

	// Clear and initialise the error log file.

	if ((fp = fopen(error_log_file_path, "w")) != NULL) {
		fprintf(fp, "<HTML>\n<HEAD>\n<TITLE>Error log for %s</TITLE>\n"
			"</HEAD>\n", spot_URL);
		fprintf(fp, "<BODY BGCOLOR=\"#ffcc66\">\n<H1>Error log for %s</H1>\n",
			spot_URL);
		fclose(fp);
	}

	// Enable sound if the sound system started up, and downloading of sounds
	// is enabled.

	start_atomic_operation();
	sound_on = (sound_system != NO_SOUND && download_sounds);
	end_atomic_operation();

	// Clean up the renderer.

	clean_up_renderer();

	// Make the current blockset list the old blockset list, and create a new
	// blockset list.

	old_blockset_list_ptr = blockset_list_ptr;
	if ((blockset_list_ptr = new blockset_list) == NULL)
		memory_error("block set list");

	// Reset the spot title.

	spot_title = "Untitled Spot";

	// Attempt to open the spot file as a ZIP archive first.

	if (open_ZIP_archive(spot_file_path)) {

		// Open the first file with a ".3dml" extension in the ZIP archive.

		if (!push_ZIP_file_with_ext(".3dml")) {
			close_ZIP_archive();
			error("Unable to open a 3DML file inside ZIP archive %s",
				spot_URL);
		}

		// Close the ZIP archive; the spot file has already been read into a
		// memory buffer.

		close_ZIP_archive();
	}

	// Otherwise open the spot file as an ordinary text file.

	else if (!push_file(spot_file_path, true))
		error("Unable to open spot file '%s'", spot_file_path);

	// Copy the spot file to "curr_spot.txt" in the Flatland folder, making
	// sure it's converted to MS-DOS text format.

	copy_file(curr_spot_file_path, true);

	// Parse the opening spot tag, and set the minimum rover version if the
	// version parameter was given.

	parse_start_tag(TOKEN_SPOT, spot_param_list, SPOT_PARAMS);
	if (matched_param[SPOT_VERSION])
		min_rover_version = spot_version;
	else
		min_rover_version = 0;

	// Parse the rest of the spot file up to the closing spot tag.

	got_head_tag = false;
	got_body_tag = false;
	while ((tag_ptr = parse_next_tag(spot_tag_list, TOKEN_SPOT)) != NULL) {
		switch (tag_ptr->tag_name) {
		case TOKEN_HEAD:
			if (got_body_tag)
				error("The head tag must appear before the body tag");
			parse_head_tags();
			break;
		case TOKEN_BODY:
			if (!got_head_tag)
				error("Missing head tag");
			parse_body_tags();
		}
	}
	if (!got_body_tag)
		error("Missing body tag");

	// Check that all necessary level tags were seen.  If the ground tag was
	// seen in the header, the number of levels seen should be one less than
	// the number of levels present on the map.  Note that we also take into
	// account the extra empty level.

	if (got_ground_tag)
		expected_levels = world_ptr->levels - 2;
	else
		expected_levels = world_ptr->levels - 1;
	if (curr_level != expected_levels) {
		if (curr_level + 1 == expected_levels)
			warning("Level %d of the map is missing", curr_level + 1);
		else
			warning("Levels %d-%d of the map are missing", curr_level + 1, 
				expected_levels);
	}

	// If a player tag was seen, create the player block.  Otherwise set a
	// default camera offset and player collision box.

	if (!got_player_tag || !create_player_block()) {
		player_camera_offset.dx = 0.0f;
		player_camera_offset.dy = 0.0f;
		player_camera_offset.dz = -1.0f;
		set_player_size(1.2f, 1.5f, 1.2f);
	}

	// Close the spot file.

	pop_file();

	// Delete the old blockset list, if it exists.

	if (old_blockset_list_ptr)
		delete old_blockset_list_ptr;
}

//==============================================================================
// Cached blockset functions.
//==============================================================================

//------------------------------------------------------------------------------
// Delete the cached blockset list.
//------------------------------------------------------------------------------

void
delete_cached_blockset_list(void)
{
	cached_blockset *next_cached_blockset_ptr;
	while (cached_blockset_list) {
		next_cached_blockset_ptr = 
			cached_blockset_list->next_cached_blockset_ptr;
		DEL(cached_blockset_list, cached_blockset);
		cached_blockset_list = next_cached_blockset_ptr;
	}
	last_cached_blockset_ptr = NULL;
}

//------------------------------------------------------------------------------
// Add an entry to the cached blockset list.
//------------------------------------------------------------------------------

static cached_blockset *
add_cached_blockset(void)
{
	cached_blockset *cached_blockset_ptr;

	// Create the cached blockset entry.

	NEW(cached_blockset_ptr, cached_blockset);
	if (cached_blockset_ptr == NULL)
		return(NULL);

	// Add it to the end of the cached blockset list.

	cached_blockset_ptr->next_cached_blockset_ptr = NULL;
	if (last_cached_blockset_ptr)
		last_cached_blockset_ptr->next_cached_blockset_ptr = cached_blockset_ptr;
	else
		cached_blockset_list = cached_blockset_ptr;
	last_cached_blockset_ptr = cached_blockset_ptr;
	return(cached_blockset_ptr);
}

//------------------------------------------------------------------------------
// Add a new blockset to the cached blockset list.
//------------------------------------------------------------------------------

cached_blockset *
new_cached_blockset(const char *path, const char *href, int size, int updated)
{
	char *name_ptr, *ext_ptr;
	string name;
	cached_blockset *cached_blockset_ptr;

	// Open the blockset.

	if (!open_ZIP_archive(path))
		return(NULL);

	// Extract the blockset file name, replace the ".bset" extension with
	// ".style" to obtain the style file name.

	name_ptr = strrchr(path, '\\');
	name = name_ptr + 1;
	ext_ptr = strrchr(name, '.');
	name.truncate(ext_ptr - (char *)name);
	name += ".style";

	// Open the style file.

	if (!push_ZIP_file(name)) {
		close_ZIP_archive();
		return(NULL);
	}

	// Parse the opening style tag.  If this fails close the style file and
	// the blockset, and return a NULL pointer.

	try {
		parse_start_tag(TOKEN_STYLE, style_param_list, STYLE_PARAMS);
	}
	catch (char *) {
		pop_file();
		close_ZIP_archive();
		return(NULL);
	}

	// Close the style file and the blockset.

	pop_file();
	close_ZIP_archive();

	// Create a new cached blockset entry.

	if ((cached_blockset_ptr = add_cached_blockset()) == NULL)
		return(NULL);

	// Initialise the cached blockset entry, and return a pointer to it.

	cached_blockset_ptr->href = href;
	cached_blockset_ptr->size = size;
	cached_blockset_ptr->updated = updated;
	if (matched_param[STYLE_NAME])
		cached_blockset_ptr->name = style_name;
	if (matched_param[STYLE_SYNOPSIS])
		cached_blockset_ptr->synopsis = style_synopsis;
	if (matched_param[STYLE_VERSION])
		cached_blockset_ptr->version = style_version;
	else
		cached_blockset_ptr->version = 0;
	return(cached_blockset_ptr);
}

//------------------------------------------------------------------------------
// Load the cached blockset list from the cache file.
//------------------------------------------------------------------------------

bool
load_cached_blockset_list(void)
{
	tag *tag_ptr;
	cached_blockset *cached_blockset_ptr;

	// Open the cache file.  If this fails return a failure status.

	cached_blockset_list = NULL;
	if (!push_file(cache_file_path, false))
		return(false);

	// Parse the opening CACHE tag, and then all the BLOCKSET tags it
	// contains up to the closing CACHE tag.

	try {
		parse_start_tag(TOKEN_CACHE, NULL, 0);
		while ((tag_ptr = parse_next_tag(cache_tag_list, TOKEN_CACHE)) 
			!= NULL) {
			switch (tag_ptr->tag_name) {
			case TOKEN_BLOCKSET:

				// Create a cached blockset entry; if this fails generate an
				// memory error.

				if ((cached_blockset_ptr = add_cached_blockset()) == NULL)
					memory_error("cached blockset entry");

				// Initialise the cached blockset entry.

				cached_blockset_ptr->href = cached_blockset_href;
				cached_blockset_ptr->size = cached_blockset_size;
				cached_blockset_ptr->updated = cached_blockset_updated;
				if (matched_param[CACHED_BLOCKSET_NAME])
					cached_blockset_ptr->name = cached_blockset_name;
				if (matched_param[CACHED_BLOCKSET_SYNOPSIS])
					cached_blockset_ptr->synopsis = cached_blockset_synopsis;
				if (matched_param[CACHED_BLOCKSET_VERSION])
					cached_blockset_ptr->version = cached_blockset_version;
				else
					cached_blockset_ptr->version = 0;
			}
		}
	}

	// If an error occurs during parsing, pop the cache file, delete the
	// cached blockset list, and return a failure status.

	catch (char *) {
		pop_file();
		delete_cached_blockset_list();
		return(false);			
	}

	// Close the cache file and return success.

	pop_file();
	return(true);
}

//------------------------------------------------------------------------------
// Save the cached blockset list to the cache file.
//------------------------------------------------------------------------------

void
save_cached_blockset_list(void)
{
	FILE *fp;
	cached_blockset *cached_blockset_ptr;

	// Open the cache file for writing.
 
	if ((fp = fopen(cache_file_path, "w")) != NULL) {

		// Write the opening cache tag.

		fprintf(fp, "<CACHE>\n");

		// Step through the list of cached blocksets, and write an entry to the
		// cache file for each.

		cached_blockset_ptr = cached_blockset_list;
		while (cached_blockset_ptr) {
			fprintf(fp, "\t<BLOCKSET HREF=\"%s\" SIZE=\"%d\""
				" UPDATED=\"%d\"", cached_blockset_ptr->href,
				cached_blockset_ptr->size, cached_blockset_ptr->updated);
			if (strlen(cached_blockset_ptr->name) > 0)
				fprintf(fp, " NAME=\"%s\"", cached_blockset_ptr->name);
			if (strlen(cached_blockset_ptr->synopsis) > 0)
				fprintf(fp, " SYNOPSIS=\"%s\"", cached_blockset_ptr->synopsis);
			if (cached_blockset_ptr->version > 0)
				fprintf(fp, " VERSION=\"%s\"", 
					version_number_to_string(cached_blockset_ptr->version));
			fprintf(fp, "/>\n");
			cached_blockset_ptr = cached_blockset_ptr->next_cached_blockset_ptr;
		}

		// Write the closing cache tag.

		fprintf(fp, "</CACHE>\n");
		fclose(fp);
	}
}

//------------------------------------------------------------------------------
// Search for cached blocksets in the given directory, and add them to the
// cached blockset list.
//------------------------------------------------------------------------------

static void
find_cached_blocksets(const char *dir_path)
{
	string path, href;
	struct _finddata_t file_info;
	long find_handle;
	char *ext_ptr, *ch_ptr;

	// Construct the wildcard file path for the specified directory.

	path = flatland_dir;
	path += dir_path;
	path += "*.*";

	// If there are no files in the given directory, just return.

	if ((find_handle = _findfirst(path, &file_info)) == -1)
		return;

	// Otherwise search for .bset files and subdirectories; for each of the
	// latter, recursively call this function.

	do {
		// Skip over the current and parent directory entries.

		if (!strcmp(file_info.name, ".") || !strcmp(file_info.name, ".."))
			continue;

		// If this entry is a subdirectory, construct a new directory path
		// and recursively call this function.

		if (file_info.attrib & _A_SUBDIR) {
			path = dir_path;
			path += file_info.name;
			path += "\\";
			find_cached_blocksets(path);
		}

		// Otherwise if this entry has an extension of ".bset", include this in
		// the cached blockset list.

		else {
			ext_ptr = strrchr(file_info.name, '.');
			if (!stricmp(ext_ptr, ".bset")) {

				// Construct the path to the blockset.

				path = flatland_dir;
				path += dir_path;
				path += file_info.name;

				// Construct the URL to the blockset.

				href = "http://";
				href += dir_path;
				href += file_info.name;
				ch_ptr = (char *)href;
				while (*ch_ptr) {
					if (*ch_ptr == '\\')
						*ch_ptr = '/';
					ch_ptr++;
				}

				// Create a new cached blockset entry.

				new_cached_blockset(path, href, file_info.size, 0);
			}
		}

		// Find next file.

	} while (_findnext(find_handle, &file_info) == 0);

	// Done searching the directory.

   _findclose(find_handle);
}

//------------------------------------------------------------------------------
// Create the cached blockset list.
//------------------------------------------------------------------------------

void
create_cached_blockset_list(void)
{
	cached_blockset_list = NULL;
	last_cached_blockset_ptr = NULL;
	find_cached_blocksets("");
	save_cached_blockset_list();
}

//------------------------------------------------------------------------------
// Search for a cached blockset entry by URL.
//------------------------------------------------------------------------------

cached_blockset *
find_cached_blockset(const char *href)
{
	cached_blockset *cached_blockset_ptr = cached_blockset_list;
	while (cached_blockset_ptr) {
		if (!stricmp(href, cached_blockset_ptr->href))
			return(cached_blockset_ptr);
		cached_blockset_ptr = cached_blockset_ptr->next_cached_blockset_ptr;
	}
	return(NULL);
}

//------------------------------------------------------------------------------
// Delete the cached blockset entry with the given URL.
//------------------------------------------------------------------------------

bool
delete_cached_blockset(const char *href)
{
	// Search for the entry with the given URL, and also remember the previous
	// entry.

	cached_blockset *prev_cached_blockset_ptr = NULL;
	cached_blockset *cached_blockset_ptr = cached_blockset_list;
	while (cached_blockset_ptr) {

		// If this entry matches, delete it and return success.

		if (!stricmp(href, cached_blockset_ptr->href)) {
			if (prev_cached_blockset_ptr) {
				prev_cached_blockset_ptr->next_cached_blockset_ptr = 
					cached_blockset_ptr->next_cached_blockset_ptr;
				if (last_cached_blockset_ptr == cached_blockset_ptr)
					last_cached_blockset_ptr = prev_cached_blockset_ptr;
			} else {
				cached_blockset_list = 
					cached_blockset_ptr->next_cached_blockset_ptr;
				if (last_cached_blockset_ptr == cached_blockset_ptr)
					last_cached_blockset_ptr = NULL;
			}
			DEL(cached_blockset_ptr, cached_blockset);
			return(true);
		}

		// Move onto the next entry.

		prev_cached_blockset_ptr = cached_blockset_ptr;
		cached_blockset_ptr = cached_blockset_ptr->next_cached_blockset_ptr;
	}

	// Indicate failure.

	return(false);
}

//------------------------------------------------------------------------------
// Check for an update for the given blockset, and if there is one ask the
// user if they want to download it.
//------------------------------------------------------------------------------

bool
check_for_blockset_update(const char *version_file_URL, const char *blockset_name, 
						  unsigned int blockset_version)
{
	string message;
	char *line_ptr;

	// Download the specified version file URL to "version.txt" in the flatland
	// directory.  If it doesn't exist or cannot be opened, assume the current
	// version is up to date.

	requested_blockset_name = (char *)NULL;
	if (!download_URL(version_file_URL, version_file_path) ||
		!push_file(version_file_path, false))
		return(false);

	// Parse the version file.

	try {

		// Parse the opening version tag.

		parse_start_tag(TOKEN_VERSION, blockset_version_param_list,
			BLOCKSET_VERSION_PARAMS);

		// Parse each line up to a closing version tag as a description of the
		// new blockset.

		while (true) {

			// Read the next line.

			read_line();

			// Skip over leading spaces.

			line_ptr = line_buffer;
			while (*line_ptr == ' ' || *line_ptr == '\t')
				line_ptr++;

			// If this is the start of a tag, break out of the loop.

			if (*line_ptr == '<')
				break;
			
			// Add this line to the message string.

			if (strlen(message) > 0)
				message += " ";
			message += line_ptr;
		}
		
		// Parse the end version tag.

		parse_end_tag(TOKEN_VERSION);
	}

	// If an error occurs during parsing, close the version file and indicate
	// no update is to occur.

	catch (char *) {
		pop_file();
		return(false);			
	}

	// Close the version file.

	pop_file();

	// If the version number in the version file is higher than the version
	// number in the cached blockset, ask the user if they want to download
	// the new blockset.

	if (blockset_version_id > blockset_version && 
		query("New version of blockset available", true, 
			"Version %s of the %s blockset is available for download.\n\n"
			"%s\n\nWould you like to download it now?", 
			version_number_to_string(blockset_version_id), blockset_name,
			message))
		return(true);

	// Indicate no update is available or requested.

	return(false);
}

//------------------------------------------------------------------------------
// Parse the Rover version file, and return the version number and message.
//------------------------------------------------------------------------------

bool
parse_rover_version_file(unsigned int &version_number, string &message)
{
	char *line_ptr;

	// Open the version file.

	if (!push_file(version_file_path, false))
		return(false);

	// Parse the version file.

	try {

		// Parse the opening version tag.

		parse_start_tag(TOKEN_VERSION, rover_version_param_list,
			ROVER_VERSION_PARAMS);
		version_number = rover_version_id;

		// Parse each line up to a closing version tag as a description of the
		// new blockset.

		message = (char *)NULL;
		while (true) {

			// Read the next line.

			read_line();

			// Skip over leading spaces.

			line_ptr = line_buffer;
			while (*line_ptr == ' ' || *line_ptr == '\t')
				line_ptr++;

			// If this is the start of a tag, break out of the loop.

			if (*line_ptr == '<')
				break;
			
			// Add this line to the message string.

			if (strlen(message) > 0)
				message += " ";
			message += line_ptr;
		}
		
		// Parse the end version tag.

		parse_end_tag(TOKEN_VERSION);
	}

	// If an error occurs during parsing, close the version file and indicate
	// no update is to occur.

	catch (char *) {
		pop_file();
		return(false);			
	}

	// Close the version file.

	pop_file();
	return(true);
}