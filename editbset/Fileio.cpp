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
// The Original Code is EditBset. 
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
#include <zip.h>
#include <unzip.h>
#include "classes.h"
#include "parser.h"
#include "main.h"

// Data required to parse a DXF file.

static double block_vertex_x[24], block_vertex_y[24], block_vertex_z[24];
static int block_vertex_index;
static int block_part_index;
static int block_polygon_index;
static int polygon_vertices;
static double x[32], y[32], z[32];
static int vertex_no_list[4];
static char part_name[BUFSIZ];

// Flags indicating whether we've seen certain style or block tags already.

static bool got_vertex_list;
static bool got_part_list;
static bool got_part_tag;
static bool got_light_tag;
static bool got_sound_tag;
static bool got_exit_tag;
static bool got_param_tag;

//==============================================================================
// Miscellaneous functions.
//==============================================================================

//------------------------------------------------------------------------------
// Return a block type as a string.
//------------------------------------------------------------------------------

char *
get_block_type(blocktype block_type)
{
	switch (block_type) {
	case STRUCTURAL_BLOCK:
		return("structural");
		break;
	case MULTIFACETED_SPRITE:
		return("multifaceted sprite");
		break;
	case ANGLED_SPRITE:
		return("angled sprite");
		break;
	case REVOLVING_SPRITE:
		return("revolving sprite");
		break;
	case FACING_SPRITE:
		return("facing sprite");
		break;
	case PLAYER_SPRITE:
		return("player sprite");
		break;
	default:
		return("unknown");
	}
}

#ifdef SUPPORT_DXF

//==============================================================================
// DXF file parsing functions.
//==============================================================================

//------------------------------------------------------------------------------
// Create a polygon.
//------------------------------------------------------------------------------

void
create_polygon(block *block_ptr)
{
	int index;
	vertex *vertex_ptr;
	part *part_ptr;
	polygon *polygon_ptr;
	char texture_name[BUFSIZ];

	// If the part name is new, create a new part object and initialise the
	// texture name to be the part name plus ".gif".

	if ((part_ptr = block_ptr->find_part(part_name)) == NULL) {
		block_part_index++;
		if ((part_ptr = block_ptr->add_part(block_part_index, part_name))
			== NULL)
			memory_error("part object");
		strcpy(texture_name, part_name);
		strcat(texture_name, ".gif");
		part_ptr->texture = texture_name;
		part_ptr->flags |= PART_TEXTURE;
	}

	// Add the vertices to the block, if they are unique, remembering the
	// vertex number of each.

	for (index = 0; index < polygon_vertices; index++) {
		if ((vertex_ptr = block_ptr->find_vertex(x[index], y[index], z[index]))
			== NULL && (vertex_ptr = block_ptr->add_vertex(x[index], y[index],
			z[index])) == NULL)
			memory_error("vertex");
		vertex_no_list[index] = vertex_ptr->vertex_no;
	}

	// Create a new polygon and initialise it.

	block_polygon_index++;
	if ((polygon_ptr = block_ptr->add_polygon()) == NULL) 
		memory_error("polygon");
	polygon_ptr->part_no = part_ptr->part_no;
	polygon_ptr->polygon_no = block_polygon_index;
	polygon_ptr->front_polygon_ptr = NULL;
	polygon_ptr->rear_polygon_ptr = NULL;

	// Create the vertex definition list.

	for (index = 0; index < polygon_vertices; index++)
		if (!polygon_ptr->add_vertex_def(vertex_no_list[index], 0.0, 0.0))
			memory_error("vertex definition");

	// Compute the plane equation for this polygon.

	polygon_ptr->compute_plane_equation(block_ptr);

	// Apply "stretched" texture coordinates to the polygon.  Note that
	// "stretched" defaults to "scaled" for polygons that don't have 3 or 4
	// vertices.

	if (polygon_vertices == 3 || polygon_vertices == 4)
		polygon_ptr->stretch_texture(block_ptr);
	else
		polygon_ptr->project_texture(block_ptr);
}

//------------------------------------------------------------------------------
// Adjust the vertex list so that the X, Y and Z coordinates are in the range
// of 0..256.
//------------------------------------------------------------------------------

void
adjust_vertex_list(block *block_ptr)
{
	int index;
	vertex *vertex_ptr;
	double smallest_x, smallest_y, smallest_z;
	double largest_x, largest_y, largest_z;
	double x_scale, y_scale, z_scale;

	// Find the smallest and largest X, Y and Z coordinate of the "block" part.

	smallest_x = largest_x = block_vertex_x[0];
	smallest_y = largest_y = block_vertex_y[0];
	smallest_z = largest_z = block_vertex_z[0];
	for (index = 0; index < block_vertex_index; index++) {
		if (block_vertex_x[index] < smallest_x)
			smallest_x = block_vertex_x[index];
		else if (block_vertex_x[index] > largest_x)
			largest_x = block_vertex_x[index];
		if (block_vertex_y[index] < smallest_y)
			smallest_y = block_vertex_y[index];
		else if (block_vertex_y[index] > largest_y)
			largest_y = block_vertex_y[index];
		if (block_vertex_z[index] < smallest_z)
			smallest_z = block_vertex_z[index];
		else if (block_vertex_z[index] > largest_z)
			largest_z = block_vertex_z[index];
	}

	// Compute the X, Y and Z scale.

	x_scale = 256.0 / (largest_x - smallest_x);
	y_scale = 256.0 / (largest_y - smallest_y);
	z_scale = 256.0 / (largest_z - smallest_z);

	// Now adjust all vertex coordinates to be in the range of 0..255.

	vertex_ptr = block_ptr->vertex_list;
	while (vertex_ptr) {
		vertex_ptr->x = (vertex_ptr->x - smallest_x) * x_scale;
		vertex_ptr->y = (vertex_ptr->y - smallest_y) * y_scale;
		vertex_ptr->z = (vertex_ptr->z - smallest_z) * z_scale;
		vertex_ptr = vertex_ptr->next_vertex_ptr;
	}
}

//------------------------------------------------------------------------------
// Parse a 3DFACE entity.
//------------------------------------------------------------------------------

void
parse_3D_face(block *block_ptr)
{
	int param, index;

	// Read each parameter of the 3D face until we run out of parameters.

	read_int(&param);
	while (param != 0) {
		switch (param) {

		// If the parameter is a layer name, parse it as a part name.  All
		// characters after the first underscore are ignored.

		case 8:
			read_name(part_name);
			break;

		// If the parameter is an x, y or z coordinate, parse it.  Note that
		// the Y and Z coordinates are swapped to match the coordinate system
		// used by blocks.

		case 10:
		case 11:
		case 12:
		case 13:
			read_double(&x[param - 10]);
			break;

		case 20:
		case 21:
		case 22:
		case 23:
			read_double(&z[param - 20]);
			break;

		case 30:
		case 31:
		case 32:
		case 33:
			read_double(&y[param - 30]);
			break;

		// Skip over all other parameters.

		default:
			read_next_line();
		}

		// Read start of next parameter.

		read_int(&param);
	}

	// If the part name is "block", then store the vertices of this face in a
	// special set of arrays.

	if (!stricmp(part_name, "block")) {
		for (index = 0; index < 4; index++) {
			block_vertex_x[block_vertex_index] = x[index];
			block_vertex_y[block_vertex_index] = y[index];
			block_vertex_z[block_vertex_index] = z[index];
			block_vertex_index++;
		}
	}

	// Otherwise create a new polygon...

	else {

		// If the last two vertices are identical, then the face is a triangle,
		// otherwise it is a rectangle.

		if (x[2] == x[3] && y[2] == y[3] && z[2] == z[3])
			polygon_vertices = 3;
		else
			polygon_vertices = 4;
		create_polygon(block_ptr);
	}
}

//------------------------------------------------------------------------------
// Parse a POLYLINE entity.
//------------------------------------------------------------------------------

void
parse_polyline(block *block_ptr)
{
	int param;
	bool done;

	// Read each parameter of the polyline until we encounter "SEQEND"

	done = false;
	polygon_vertices = 0;
	read_int(&param);
	while (!done) {
		switch (param) {

		// If the parameter is a layer name, parse it as a part name.

		case 8:
			read_name(part_name);
			break;

		// If the parameter is "ENDSEQ" or "VERTEX"...

		case 0:
			read_next_line();

			// If the parameter is "ENDSEQ", break out of this loop.

			if (line_is("SEQEND")) {
				done = true;
				break;
			}
			
			// If the parameter is not "VERTEX", this is an error.

			verify_line("VERTEX");

			// Read the vertex parameters.

			read_int(&param);
			while (param != 0) {
				switch (param) {
				case 10:
					read_double(&x[polygon_vertices]);
					break;
				case 20:
					read_double(&z[polygon_vertices]);
					break;
				case 30:
					read_double(&y[polygon_vertices]);
					break;
				default:
					read_next_line();
				}
				read_int(&param);
			}

			// Increment the vertex count and continue parsing POLYLINE
			// parameters.

			polygon_vertices++;
			continue;

		// Skip over all other parameters.

		default:
			read_next_line();
		}

		// Read start of next parameter.

		read_int(&param);
	}

	// Create the polygon.

	create_polygon(block_ptr);
}

//------------------------------------------------------------------------------
// Parse a DXF file.
//------------------------------------------------------------------------------

static void
parse_DXF_file(block *block_ptr)
{
	// Initialise the block vertex, part and polygon numbers.

	block_vertex_index = 0;
	block_part_index = 0;
	block_polygon_index = 0;

	// Parse each section of the DXF file...

	verify_int(0);
	read_next_line();
	while (line_is("SECTION")) {

		// Determine what type of section it is.

		verify_int(2);
		read_next_line();

		// If this is a HEADER, TABLES or BLOCKS section, skip over it.

		if (line_is("HEADER") || line_is("TABLES") || line_is("BLOCKS")) {
			read_next_line();
			while (!line_is("ENDSEC"))
				read_next_line();
		}

		// If this is an ENTITIES section, parse each entity...

		else if (line_is("ENTITIES")) {
			verify_int(0);
			read_next_line();
			while (!line_is("ENDSEC")) {
				
				// If this entity is "3DFACE", then parse a 3D face (triangle
				// or rectangle).

				if (line_is("3DFACE"))
					parse_3D_face(block_ptr);

				// If this entity is "POLYLINE", then parse a polygon that may
				// have more than 4 sides.

				else if (line_is("POLYLINE"))
					parse_polyline(block_ptr);
				
				// Any other entity type is an error.

				else
					file_error("Unsupported entity type");

				// Read the start of the next entity.

				read_next_line();
			}
		}

		// Any other section type indicates a corrupt DXF file.

		else
			file_error("Unrecognised section");

		// Parse start of next section.

		verify_int(0);
		read_next_line();
	}

	// Make sure there were six "block" faces parsed.

	if (block_vertex_index != 24)
		file_error("Only %d 'block' faces were parsed", 
			block_vertex_index / 4);

	// Adjust the vertex list according to the dimensions of the "block" faces.

	adjust_vertex_list(block_ptr);

	// Make sure the DXF file ends with "EOF".

	verify_line("EOF");
}

#endif

//==============================================================================
// Block file parsing functions.
//==============================================================================

//------------------------------------------------------------------------------
// Parse the vertex list.
//------------------------------------------------------------------------------

static void
parse_vertex_list(block *block_ptr)
{
	tag *tag_ptr;
	int vertex_no;
	vertex *vertex_ptr;

	// Parameter list for vertex tag.

	int vertex_ref;
	vertex vertex_coords;
	param vertex_param_list[] = {
		{TOKEN_REF,	VALUE_INTEGER, &vertex_ref,	true},
		{TOKEN_COORDS, VALUE_VERTEX_COORDS,	&vertex_coords, true},
		{TOKEN_NONE}
	};

	// Vertices tag list.

	tag vertices_tag_list[] = {
		{TOKEN_VERTEX, vertex_param_list, TOKEN_CLOSE_SINGLE_TAG},
		{TOKEN_NONE}
	};

	// If the vertices tag has already been seen, skip over all tags inside it
	// and the end vertices tag.

	if (got_vertex_list) {
		parse_next_tag(NULL, TOKEN_VERTICES);
		return;
	}
	got_vertex_list = true;

	// Parse each vertex definition and add it to the block's vertex list.

	vertex_no = 0;
	while ((tag_ptr = parse_next_tag(vertices_tag_list, TOKEN_VERTICES))
		!= NULL) {
		switch (tag_ptr->tag_name) {
		case TOKEN_VERTEX:

			// Verify that the vertex reference is in sequence.

			vertex_no++;
			if (vertex_ref != vertex_no)
				file_error("vertex reference number is out of sequence");

			// Add the new vertex to the block's vertex list.

			if ((vertex_ptr = block_ptr->add_vertex(vertex_coords.x, 
				vertex_coords.y, vertex_coords.z)) == NULL)
				memory_error("vertex");
		}
	}
}

//------------------------------------------------------------------------------
// Parse the polygon tag.
//------------------------------------------------------------------------------

static void
parse_polygon_tag(block *block_ptr, part *part_ptr, int part_no, 
				  int &polygon_no, int polygon_ref, ref_list &vertex_ref_list,
				  texcoords_list &texture_coords_list, double texture_angle,
				  int front_polygon_ref, int rear_polygon_ref)
{
	polygon *polygon_ptr;
	ref *vertex_ref_ptr;
	texcoords *texcoords_ptr;
	int index;

	// Increment the polygon number.

	polygon_no++;

	// Verify the polygon reference is in sequence.

	if (polygon_ref != polygon_no)
		file_error("polygon reference number is out of sequence");

	// Verify that the vertex reference list has at least 3 elements.

	if (vertex_ref_list.refs < 3)
		file_error("A polygon must have at least 3 vertices");

	// If the texture coordinates list was given, make sure it has the same
	// number of elements as the vertex reference list, then set the texture
	// coordinates for each vertex definition.

	if (matched_param[2] && texture_coords_list.elements != vertex_ref_list.refs)
		file_error("There must be the same number of texture coordinates "
			"as vertices");

	// Add the polygon to the block's polygon list, and initialise the part
	// and polygon numbers.

	if ((polygon_ptr = block_ptr->add_polygon()) == NULL)
		memory_error("polygon");
	polygon_ptr->part_no = part_no;
	polygon_ptr->polygon_no = polygon_ref;
	polygon_ptr->front_polygon_no = front_polygon_ref;
	polygon_ptr->rear_polygon_no = rear_polygon_ref;

	// Create the vertex definition list for this polygon.  If there is a
	// texture coordinates list, use it to initialise the texture coordinates
	// for each vertex definition.

	vertex_ref_ptr = vertex_ref_list.first_ref_ptr;
	if (matched_param[2])
		texcoords_ptr = texture_coords_list.first_texcoords_ptr;
	for (index = 0; index < vertex_ref_list.refs; index++) {
		if (matched_param[2])
			polygon_ptr->add_vertex_def(vertex_ref_ptr->ref_no, 
				texcoords_ptr->u, texcoords_ptr->v);
		else
			polygon_ptr->add_vertex_def(vertex_ref_ptr->ref_no, 0.0, 0.0);
		vertex_ref_ptr = vertex_ref_ptr->next_ref_ptr;
		if (matched_param[2])
			texcoords_ptr = texcoords_ptr->next_texcoords_ptr;
	}

	// Set the texture angle for the polygon, if it was given.

	if (matched_param[3]) {
		polygon_ptr->got_texture_angle = true;
		polygon_ptr->texture_angle = texture_angle;
	}
	
	// Compute the plane equation for this polygon.

	polygon_ptr->compute_plane_equation(block_ptr);

	// If no texture coordinates list was given, apply "stretched" texture
	// coordinates  to the polygon.  Note that "stretched" defaults to "scaled"
	// for polygons that don't have 3 or 4 vertices.

	if (!matched_param[2]) {
		if (vertex_ref_list.refs == 3 || vertex_ref_list.refs == 4)
			polygon_ptr->stretch_texture(block_ptr);
		else
			polygon_ptr->project_texture(block_ptr);
	}
}

//------------------------------------------------------------------------------
// Parse a sprite part tag.
//------------------------------------------------------------------------------

static void
parse_part_tag(block *block_ptr, char *name, char *texture, char *colour,
			   char *translucency, char *style, char *faces, char *angle)
{
	part *part_ptr;

	// If a part already exists with the desired part name, this is an error.

	if (block_ptr->find_part(name))
		file_error("duplicate part name '%s'", name);

	// Add the part to the block's part list.

	if ((part_ptr = block_ptr->add_part(1, name)) == NULL)
		memory_error("part");

	// Initialise the fields of the part that were given as parameters.

	if (matched_param[1]) {
		part_ptr->flags |= PART_TEXTURE;
		part_ptr->texture = texture;
	}
	if (matched_param[2]) {
		part_ptr->flags |= PART_COLOUR;
		part_ptr->colour = colour;
	}
	if (matched_param[3]) {
		part_ptr->flags |= PART_TRANSLUCENCY;
		part_ptr->translucency = translucency;
	}
	if (matched_param[4]) {
		part_ptr->flags |= PART_STYLE;
		part_ptr->style = style;
	}
	if (matched_param[5]) {
		part_ptr->flags |= PART_FACES;
		part_ptr->faces = faces;
	}
	if (matched_param[6]) {
		part_ptr->flags |= PART_ANGLE;
		part_ptr->angle = angle;
	}
}

//------------------------------------------------------------------------------
// Parse a part tag.
//------------------------------------------------------------------------------

static void
parse_part_tag(block *block_ptr, int &part_no, int &polygon_no, char *name,
			   char *texture, char *colour, char *translucency, char *style,
			   char *faces, char *angle)
{
	tag *tag_ptr;
	part *part_ptr;

	// Parameter list for polygon tag.

	int polygon_ref;
	ref_list vertex_ref_list;
	texcoords_list texture_coords_list;
	double texture_angle;
	int front_polygon_ref;
	int rear_polygon_ref;
	param polygon_param_list[] = {
		{TOKEN_REF,	VALUE_INTEGER, &polygon_ref, true},
		{TOKEN_VERTICES, VALUE_REF_LIST, &vertex_ref_list, true},
		{TOKEN_TEXCOORDS, VALUE_TEXCOORDS_LIST, &texture_coords_list, false},
		{TOKEN_ANGLE, VALUE_DEGREES, &texture_angle, false},
		{TOKEN_FRONT, VALUE_INTEGER, &front_polygon_ref, false},
		{TOKEN_REAR, VALUE_INTEGER, &rear_polygon_ref, false},
		{TOKEN_NONE}
	};

	// Part tag list.

	tag part_tag_list[] = {
		{TOKEN_POLYGON, polygon_param_list, TOKEN_CLOSE_SINGLE_TAG},
		{TOKEN_NONE}
	};

	// If a part already exists with the desired part name, this is an error.

	if (block_ptr->find_part(name))
		file_error("duplicate part name '%s'", name);

	// Add the part to the block's part list.

	part_no++;
	if ((part_ptr = block_ptr->add_part(part_no, name)) == NULL)
		memory_error("part");

	// Initialise the fields of the part that were given as parameters.

	if (matched_param[1]) {
		part_ptr->flags |= PART_TEXTURE;
		part_ptr->texture = texture;
	}
	if (matched_param[2]) {
		part_ptr->flags |= PART_COLOUR;
		part_ptr->colour = colour;
	}
	if (matched_param[3]) {
		part_ptr->flags |= PART_TRANSLUCENCY;
		part_ptr->translucency = translucency;
	}
	if (matched_param[4]) {
		part_ptr->flags |= PART_STYLE;
		part_ptr->style = style;
	}
	if (matched_param[5]) {
		part_ptr->flags |= PART_FACES;
		part_ptr->faces = faces;
	}
	if (matched_param[6]) {
		part_ptr->flags |= PART_ANGLE;
		part_ptr->angle = angle;
	}

	// Read each "polygon" tag until the ending "part" tag is reached.

	while ((tag_ptr = parse_next_tag(part_tag_list, TOKEN_PART)) != NULL) {
		switch (tag_ptr->tag_name) {
		case TOKEN_POLYGON:
			parse_polygon_tag(block_ptr, part_ptr, part_no, polygon_no,
				polygon_ref, vertex_ref_list, texture_coords_list, 
				texture_angle, front_polygon_ref, rear_polygon_ref);
		}
	}
}

//------------------------------------------------------------------------------
// Parse the part list.
//------------------------------------------------------------------------------

static void
parse_part_list(block *block_ptr)
{
	tag *tag_ptr;
	int part_no;
	int polygon_no;

	// Parameter list for part tag.

	char part_name[256];
	char part_texture[256];
	char part_colour[256];
	char part_translucency[256];
	char part_style[256];
	char part_faces[256];
	char part_angle[256];
	param part_param_list[] = {
		{TOKEN_NAME, VALUE_PART_NAME, part_name, true},
		{TOKEN_TEXTURE, VALUE_STRING, part_texture, false},
		{TOKEN_COLOUR, VALUE_STRING, part_colour, false},
		{TOKEN_TRANSLUCENCY, VALUE_STRING, part_translucency, false},
		{TOKEN_STYLE, VALUE_STRING, part_style, false},
		{TOKEN_FACES, VALUE_STRING, part_faces, false},
		{TOKEN_ANGLE, VALUE_STRING, part_angle, false},
		{TOKEN_NONE}
	};

	// Parts tag list.

	tag parts_tag_list[] = {
		{TOKEN_PART, part_param_list, TOKEN_CLOSE_TAG},
		{TOKEN_NONE}
	};
 
	// If the parts tag has already been seen, skip over the tags inside and
	// the end parts tag.

	if (got_part_list) {
		parse_next_tag(NULL, TOKEN_PARTS);
		return;
	}
	got_part_list = true;

	// Read each part definition and store it in the part list, until all
	// parts have been read.

	part_no = 0;
	polygon_no = 0;
	while ((tag_ptr = parse_next_tag(parts_tag_list, TOKEN_PARTS)) != NULL) {
		switch (tag_ptr->tag_name) {
		case TOKEN_PART:
			parse_part_tag(block_ptr, part_no, polygon_no, part_name, 
				part_texture, part_colour, part_translucency, part_style,
				part_faces, part_angle);
		}
	}
}

//------------------------------------------------------------------------------
// Parse a light tag.
//------------------------------------------------------------------------------

static void
parse_light_tag(block *block_ptr, token tag_name, char *light_style, 
				char *light_brightness, char *light_colour, char *light_radius,
				char *light_position, char *light_speed, char *light_flood,
				char *light_direction, char *light_cone)
{
	if (got_light_tag)
		return;
	got_light_tag = true;

	block_ptr->point_light = tag_name == TOKEN_POINT_LIGHT;
	if (matched_param[0]) {
		block_ptr->flags |= LIGHT_STYLE;
		block_ptr->light_style = light_style;
	}
	if (matched_param[1]) {
		block_ptr->flags |= LIGHT_BRIGHTNESS;
		block_ptr->light_brightness = light_brightness;
	}
	if (matched_param[2]) {
		block_ptr->flags |= LIGHT_COLOUR;
		block_ptr->light_colour = light_colour;
	}
	if (matched_param[3]) {
		block_ptr->flags |= LIGHT_RADIUS;
		block_ptr->light_radius = light_radius;
	}
	if (matched_param[4]) {
		block_ptr->flags |= LIGHT_POSITION;
		block_ptr->light_position = light_position;
	}
	if (matched_param[5]) {
		block_ptr->flags |= LIGHT_SPEED;
		block_ptr->light_speed = light_speed;
	}
	if (matched_param[6]) {
		block_ptr->flags |= LIGHT_FLOOD;
		block_ptr->light_flood = light_flood;
	}
	if (matched_param[7]) {
		block_ptr->flags |= LIGHT_DIRECTION;
		block_ptr->light_direction = light_direction;
	}
	if (matched_param[8]) {
		block_ptr->flags |= LIGHT_CONE;
		block_ptr->light_cone = light_cone;
	}
}

//------------------------------------------------------------------------------
// Parse a sound tag.
//------------------------------------------------------------------------------

static void
parse_sound_tag(block *block_ptr, char *sound_file, char *sound_radius,
				char *sound_volume, char *sound_playback, char *sound_delay,
				char *sound_flood, char *sound_rolloff)
{
	if (got_sound_tag)
		return;
	got_sound_tag = true;

	if (matched_param[0]) {
		block_ptr->flags |= SOUND_FILE;
		block_ptr->sound_file = sound_file;
	}
	if (matched_param[1]) {
		block_ptr->flags |= SOUND_RADIUS;
		block_ptr->sound_radius = sound_radius;
	}
	if (matched_param[2]) {
		block_ptr->flags |= SOUND_VOLUME;
		block_ptr->sound_volume = sound_volume;
	}
	if (matched_param[3]) {
		block_ptr->flags |= SOUND_PLAYBACK;
		block_ptr->sound_playback = sound_playback;
	}
	if (matched_param[4]) {
		block_ptr->flags |= SOUND_DELAY;
		block_ptr->sound_delay = sound_delay;
	}
	if (matched_param[5]) {
		block_ptr->flags |= SOUND_FLOOD;
		block_ptr->sound_flood = sound_flood;
	}
	if (matched_param[6]) {
		block_ptr->flags |= SOUND_ROLLOFF;
		block_ptr->sound_rolloff = sound_rolloff;
	}
}

//------------------------------------------------------------------------------
// Parse a exit tag.
//------------------------------------------------------------------------------

static void
parse_exit_tag(block *block_ptr, char *exit_href, char *exit_target,
			   char *exit_text, char *exit_trigger)
{
	if (got_exit_tag)
		return;
	got_exit_tag = true;

	if (matched_param[0]) {
		block_ptr->flags |= EXIT_HREF;
		block_ptr->exit_href = exit_href;
	}
	if (matched_param[1]) {
		block_ptr->flags |= EXIT_TARGET;
		block_ptr->exit_target = exit_target;
	}
	if (matched_param[2]) {
		block_ptr->flags |= EXIT_TEXT;
		block_ptr->exit_text = exit_text;
	}
	if (matched_param[3]) {
		block_ptr->flags |= EXIT_TRIGGER;
		block_ptr->exit_trigger = exit_trigger;
	}
}

//------------------------------------------------------------------------------
// Parse a param tag.
//------------------------------------------------------------------------------

static void
parse_param_tag(block *block_ptr, char *param_orient, char *param_solid,
				char *param_align, char *param_speed, char *param_angle)
{
	if (got_param_tag)
		return;
	got_param_tag = true;

	if (matched_param[0]) {
		block_ptr->flags |= PARAM_ORIENT;
		block_ptr->param_orient = param_orient;
	}
	if (matched_param[1]) {
		block_ptr->flags |= PARAM_SOLID;
		block_ptr->param_solid = param_solid;
	}
	if (matched_param[2]) {
		block_ptr->flags |= PARAM_ALIGN;
		block_ptr->param_align = param_align;
	}
	if (matched_param[3]) {
		block_ptr->flags |= PARAM_SPEED;
		block_ptr->param_speed = param_speed;
	}
	if (matched_param[4]) {
		block_ptr->flags |= PARAM_ANGLE;
		block_ptr->param_angle = param_angle;
	}
}

//------------------------------------------------------------------------------
// Parse a block file.
//------------------------------------------------------------------------------

void
parse_block_file(block *block_ptr, char *block_file_path)
{
	tag *tag_ptr;
	char *name_ptr, *ext_ptr;
	char file_path[_MAX_PATH];

	// Parameter list for block tag.

	char block_name[256];
	blocktype block_type;
	char block_entrance[256];
	param block_param_list[] = {
		{TOKEN_NAME, VALUE_BLOCK_NAME, block_name, false},
		{TOKEN__TYPE, VALUE_BLOCK_TYPE, &block_type, false},
		{TOKEN_ENTRANCE, VALUE_STRING, block_entrance, false}, 
		{TOKEN_NONE}
	};

	// Parameter list for part tag.

	char part_name[256];
	char part_texture[256];
	char part_colour[256];
	char part_translucency[256];
	char part_style[256];
	char part_faces[256];
	char part_angle[256];
	param part_param_list[] = {
		{TOKEN_NAME, VALUE_PART_NAME, part_name, true},
		{TOKEN_TEXTURE, VALUE_STRING, part_texture, false},
		{TOKEN_COLOUR, VALUE_STRING, part_colour, false},
		{TOKEN_TRANSLUCENCY, VALUE_STRING, part_translucency, false},
		{TOKEN_STYLE, VALUE_STRING, part_style, false},
		{TOKEN_FACES, VALUE_STRING, part_faces, false},
		{TOKEN_ANGLE, VALUE_STRING, part_angle, false},
		{TOKEN_NONE}
	};

	// Parameter list for light tag.

	char light_style[256];
	char light_brightness[256];
	char light_colour[256];
	char light_radius[256];
	char light_position[256];
	char light_speed[256];
	char light_flood[256];
	char light_direction[256];
	char light_cone[256];
	param light_param_list[] = {
		{TOKEN_STYLE, VALUE_STRING, light_style, false},
		{TOKEN_BRIGHTNESS, VALUE_STRING, light_brightness, false},
		{TOKEN_COLOUR, VALUE_STRING, light_colour, false},
		{TOKEN_RADIUS, VALUE_STRING, light_radius, false},
		{TOKEN_POSITION, VALUE_STRING, light_position, false},
		{TOKEN_SPEED, VALUE_STRING, light_speed, false},
		{TOKEN_FLOOD, VALUE_STRING, light_flood, false},
		{TOKEN_DIRECTION, VALUE_STRING, light_direction, false},
		{TOKEN_CONE, VALUE_STRING, light_cone, false},
		{TOKEN_NONE}
	};

	// Parameter list for sound tag.

	char sound_file[256];
	char sound_radius[256];
	char sound_volume[256];
	char sound_playback[256];
	char sound_delay[256];
	char sound_flood[256];
	char sound_rolloff[256];
	param sound_param_list[] = {
		{TOKEN_FILE, VALUE_STRING, sound_file, false},
		{TOKEN_RADIUS, VALUE_STRING, sound_radius, false},
		{TOKEN_VOLUME, VALUE_STRING, sound_volume, false},
		{TOKEN_PLAYBACK, VALUE_STRING, sound_playback, false},
		{TOKEN_DELAY, VALUE_STRING, sound_delay, false},
		{TOKEN_FLOOD, VALUE_STRING, sound_flood, false},
		{TOKEN_ROLLOFF, VALUE_STRING, sound_rolloff, false},
		{TOKEN_NONE}
	};

	// Parameter list for exit tag.

	char exit_href[256];
	char exit_target[256];
	char exit_text[256];
	char exit_trigger[256];
	param exit_param_list[] = {
		{TOKEN_HREF, VALUE_STRING, exit_href, true},
		{TOKEN_TARGET, VALUE_STRING, exit_target, false},
		{TOKEN_TEXT, VALUE_STRING, exit_text, false},
		{TOKEN_TRIGGER, VALUE_STRING, exit_trigger, false},
		{TOKEN_NONE}
	};

	// Parameter list for param tag.

	char param_orient[256];
	char param_solid[256];
	char param_align[256];
	char param_speed[256];
	char param_angle[256];
	param param_param_list[] = {
		{TOKEN_ORIENT, VALUE_STRING, param_orient, false},
		{TOKEN_SOLID, VALUE_STRING, param_solid, false},
		{TOKEN_ALIGN, VALUE_STRING, param_align, false},
		{TOKEN_SPEED, VALUE_STRING, param_speed, false},
		{TOKEN_ANGLE, VALUE_STRING, param_angle, false},
		{TOKEN_NONE}
	};

	// Parameter list for BSP_tree tag.

	int root_polygon_ref;
	param BSP_tree_param_list[] = {
		{TOKEN_ROOT, VALUE_INTEGER, &root_polygon_ref, true},
		{TOKEN_NONE}
	};

	// Parse the block start tag.  Note that the block name is treated as
	// optional here, but Rover requires it, so the file name without the
	// extension will be used as the block name if it is omitted.

	parse_start_tag(TOKEN_BLOCK, block_param_list);
	if (matched_param[0])
		block_ptr->name = block_name;
	else {
		strcpy(file_path, block_file_path);
		name_ptr = strrchr(file_path, '\\');
		ext_ptr = strrchr(name_ptr + 1, '.');
		if (ext_ptr != NULL)
			*ext_ptr = '\0';
		block_ptr->name = name_ptr + 1;
	}
	if (matched_param[1]) {
		block_ptr->flags |= BLOCK_TYPE;
		block_ptr->type = block_type;
	}
	if (matched_param[2]) {
		block_ptr->flags |= BLOCK_ENTRANCE;
		block_ptr->entrance = block_entrance;
	}
	
	// If the block is a sprite...

	if (block_ptr->type & SPRITE_BLOCK) {

		// Block tag list.

		tag block_tag_list[] = {
			{TOKEN_PART, part_param_list, TOKEN_CLOSE_SINGLE_TAG},
			{TOKEN_POINT_LIGHT, light_param_list, TOKEN_CLOSE_SINGLE_TAG},
			{TOKEN_SPOT_LIGHT, light_param_list, TOKEN_CLOSE_SINGLE_TAG},
			{TOKEN_SOUND, sound_param_list, TOKEN_CLOSE_SINGLE_TAG},
			{TOKEN_EXIT, exit_param_list, TOKEN_CLOSE_SINGLE_TAG},
			{TOKEN_PARAM, param_param_list, TOKEN_CLOSE_SINGLE_TAG},
			{TOKEN_NONE}
		};

		// Reset the flags indicating which tags we've seen.

		got_part_tag = false;
		got_light_tag = false;
		got_sound_tag = false;
		got_exit_tag = false;
		got_param_tag = false;

		// Parse all tags that may be present in a sprite block definition,
		// until the closing block tag is reached.

		while ((tag_ptr = parse_next_tag(block_tag_list, TOKEN_BLOCK)) 
			!= NULL) {
			switch (tag_ptr->tag_name) {
			case TOKEN_PART:
				if (!got_part_tag) {
					got_part_tag = true;
					parse_part_tag(block_ptr, part_name, part_texture,
						part_colour, part_translucency, part_style,
						part_faces, part_angle);
				}
				break;
			case TOKEN_SPOT_LIGHT:
			case TOKEN_POINT_LIGHT:
				parse_light_tag(block_ptr, tag_ptr->tag_name, light_style,
					light_brightness, light_colour, light_radius, 
					light_position, light_speed, light_flood, light_direction,
					light_cone);
				break;
			case TOKEN_SOUND:
				parse_sound_tag(block_ptr, sound_file, sound_radius,
					sound_volume, sound_playback, sound_delay, sound_flood,
					sound_rolloff);
				break;
			case TOKEN_EXIT:
				parse_exit_tag(block_ptr, exit_href, exit_target, exit_text,
					exit_trigger);
				break;
			case TOKEN_PARAM:
				parse_param_tag(block_ptr, param_orient, param_solid,
					param_align, param_speed, param_angle);
				break;
			}
		}

		// Verify the part tag was seen.

		if (!got_part_tag)
			file_error("part tag was missing in sprite block definition");
	}

	// If the block is a structural block...

	else {

		// Block tag list.

		tag block_tag_list[] = {
			{TOKEN_VERTICES, NULL, TOKEN_CLOSE_TAG},
			{TOKEN_PARTS, NULL, TOKEN_CLOSE_TAG},
			{TOKEN_POINT_LIGHT, light_param_list, TOKEN_CLOSE_SINGLE_TAG},
			{TOKEN_SPOT_LIGHT, light_param_list, TOKEN_CLOSE_SINGLE_TAG},
			{TOKEN_SOUND, sound_param_list, TOKEN_CLOSE_SINGLE_TAG},
			{TOKEN_EXIT, exit_param_list, TOKEN_CLOSE_SINGLE_TAG},
			{TOKEN_PARAM, param_param_list, TOKEN_CLOSE_SINGLE_TAG},
			{TOKEN_BSP_TREE, BSP_tree_param_list, TOKEN_CLOSE_SINGLE_TAG},
			{TOKEN_NONE}
		};

		// Reset the flags indicating which tags we've seen.

		got_vertex_list = false;
		got_part_list = false;
		got_light_tag = false;
		got_sound_tag = false;
		got_exit_tag = false;
		got_param_tag = false;

		// Parse all tags that may be present in a structural block definition,
		// until the closing block tag is reached.  Note that the vertices tag
		// must appear before the parts tag, which must appear before the
		// BSP_tree tag.

		while ((tag_ptr = parse_next_tag(block_tag_list, TOKEN_BLOCK)) 
			!= NULL) {
			switch (tag_ptr->tag_name) {
			case TOKEN_VERTICES:
				parse_vertex_list(block_ptr);
				break;
			case TOKEN_PARTS:
				if (!got_vertex_list)
					file_error("Vertex list must appear before part list");
				parse_part_list(block_ptr);
				break;
			case TOKEN_SPOT_LIGHT:
			case TOKEN_POINT_LIGHT:
				parse_light_tag(block_ptr, tag_ptr->tag_name, light_style,
					light_brightness, light_colour, light_radius, 
					light_position, light_speed, light_flood, light_direction,
					light_cone);
				break;
			case TOKEN_SOUND:
				parse_sound_tag(block_ptr, sound_file, sound_radius,
					sound_volume, sound_playback, sound_delay, sound_flood,
					sound_rolloff);
				break;
			case TOKEN_EXIT:
				parse_exit_tag(block_ptr, exit_href, exit_target, exit_text,
					exit_trigger);
				break;
			case TOKEN_PARAM:
				parse_param_tag(block_ptr, param_orient, param_solid,
					param_align, param_speed, param_angle);
				break;
			case TOKEN_BSP_TREE:
				block_ptr->root_polygon_no = root_polygon_ref;
				block_ptr->has_BSP_tree = true;
			}
		}

		// Make sure the vertex and part list was seen.

		if (!got_vertex_list)
			file_error("vertex list was missing in block definition");
		if (!got_part_list)
			file_error("part list was missing in block definition");

		// If this block has a BSP tree, set up the pointers to the polygons in
		// the BSP tree.

		if (block_ptr->has_BSP_tree) {
			polygon *polygon_ptr;

			// Set up the pointer to the root polygon.

			block_ptr->root_polygon_ptr = 
				 block_ptr->get_polygon(block_ptr->root_polygon_no);

			// Set up the pointers to the front and rear polygons of each
			// polygon.

			polygon_ptr = block_ptr->polygon_list;
			while (polygon_ptr) {
				polygon_ptr->front_polygon_ptr =
					block_ptr->get_polygon(polygon_ptr->front_polygon_no);
				polygon_ptr->rear_polygon_ptr =
					block_ptr->get_polygon(polygon_ptr->rear_polygon_no);
				polygon_ptr = polygon_ptr->next_polygon_ptr;
			}
		}
	}
}

//==============================================================================
// DXF, block and style file reading functions.
//==============================================================================

#ifdef SUPPORT_DXF

//------------------------------------------------------------------------------
// Read a DXF file and return a pointer to the new block object.
//------------------------------------------------------------------------------

block *
read_DXF_file(char *DXF_file_path, char *block_name)
{
	block *block_ptr = NULL;
	try {

		// Open the DXF file.

		if (!push_file(DXF_file_path))
			file_error("Unable to open DXF file '%s'", DXF_file_path);

		// Create a new block, and initialise it's name.

		if ((block_ptr = new block) == NULL)
			memory_error("block");
		block_ptr->name = block_name;

		// Parse the DXF file.

		parse_DXF_file(block_ptr);

		// Close the DXF file and return a pointer to the new block.

		pop_file();
		return(block_ptr);
	}
	catch (char *error_str) {
		if (block_ptr) {
			delete block_ptr;
			pop_file();
		}
		message("Error while loading DXF file", error_str);
		return(NULL);
	}
}

#endif

//------------------------------------------------------------------------------
// Read a block file and return a pointer to the new block object.
//------------------------------------------------------------------------------

block *
read_block_file(char *block_file_path)
{
	block *block_ptr = NULL;
	try {

		// Open the block file.

		if (!push_file(block_file_path))
			file_error("Unable to open block file '%s'", block_file_path);

		// Create a new block object.

		if ((block_ptr = new block) == NULL)
			memory_error("block");

		// Parse the block file.

		parse_block_file(block_ptr, block_file_path);

		// Close the block file and return a pointer to the block object.

		pop_file();
		return(block_ptr);
	}
	catch (char *error_str) {
		if (block_ptr) {
			delete block_ptr;
			pop_file();
		}
		message("Error while loading block file", error_str);
		return(NULL);
	}
}

//------------------------------------------------------------------------------
// Read a style file.
//------------------------------------------------------------------------------

bool
read_style_file(char *style_file_path)
{
	tag *tag_ptr;
	block *block_ptr;
	string block_file_path;

	// Parameter list for style tag.

	char style_version[256];
	char style_name[256];
	char style_synopsis[256];
	param style_param_list[] = {
		{TOKEN_VERSION, VALUE_STRING, style_version, false},
		{TOKEN_NAME, VALUE_STRING, style_name, false},
		{TOKEN_SYNOPSIS, VALUE_STRING, style_synopsis, false}, 
		{TOKEN_NONE}
	};

	// Parameter list for placeholder tag.

	char placeholder_texture[256];
	param placeholder_param_list[] = {
		{TOKEN_TEXTURE, VALUE_STRING, placeholder_texture, true},
		{TOKEN_NONE}
	};

	// Parameter list for sky tag.

	char sky_texture[256];
	char sky_colour[256];
	char sky_brightness[256];
	param sky_param_list[] = {
		{TOKEN_TEXTURE, VALUE_STRING, sky_texture, false},
		{TOKEN_COLOUR, VALUE_STRING, sky_colour, false},
		{TOKEN_BRIGHTNESS, VALUE_STRING, sky_brightness, false},
		{TOKEN_NONE}
	};

	// Parameter list for ground tag.

	char ground_texture[256];
	char ground_colour[256];
	param ground_param_list[] = {
		{TOKEN_TEXTURE, VALUE_STRING, ground_texture, false},
		{TOKEN_COLOUR, VALUE_STRING, ground_colour, false},
		{TOKEN_NONE}
	};

	char orb_texture[256];
	char orb_colour[256];
	char orb_brightness[256];
	char orb_position[256];
	char orb_href[256];
	char orb_target[256];
	char orb_text[256];
	param orb_param_list[] = {
		{TOKEN_TEXTURE, VALUE_STRING, orb_texture, false},
		{TOKEN_COLOUR, VALUE_STRING, orb_colour, false},
		{TOKEN_BRIGHTNESS, VALUE_STRING, orb_brightness, false},
		{TOKEN_POSITION, VALUE_STRING, orb_position, false},
		{TOKEN_HREF, VALUE_STRING, orb_href, false},
		{TOKEN_TARGET, VALUE_STRING, orb_target, false},
		{TOKEN_TEXT, VALUE_STRING, orb_text, false},
		{TOKEN_NONE}
	};

	// Parameter list for block tag.

	char block_symbol;
	char block_double[256];
	char block_file_name[256];
	char block_name[256];
	param block_param_list[] = {
		{TOKEN_SYMBOL, VALUE_CHAR, &block_symbol, true},
		{TOKEN_DOUBLE, VALUE_STRING, &block_double, false},
		{TOKEN_FILE, VALUE_STRING, block_file_name,	true},
		{TOKEN_NAME, VALUE_STRING, block_name, false},
		{TOKEN_NONE}
	};

	// Style tag list.

	tag style_tag_list[] = {
		{TOKEN_PLACEHOLDER, placeholder_param_list, TOKEN_CLOSE_SINGLE_TAG},
		{TOKEN_SKY, sky_param_list, TOKEN_CLOSE_SINGLE_TAG},
		{TOKEN_GROUND, ground_param_list, TOKEN_CLOSE_SINGLE_TAG},
		{TOKEN_ORB, orb_param_list, TOKEN_CLOSE_SINGLE_TAG},
		{TOKEN_BLOCK, block_param_list, TOKEN_CLOSE_SINGLE_TAG},
		{TOKEN_NONE}
	};

	try {
		// Open the style file.

		if (!push_file(style_file_path))
			file_error("Unable to open style file '%s'", style_file_path);

		// Parse the style start tag.

		parse_start_tag(TOKEN_STYLE, style_param_list);
		if (matched_param[0]) {
			style_ptr->style_flags |= STYLE_VERSION;
			style_ptr->style_version = style_version;
		}
		if (matched_param[1]) {
			style_ptr->style_flags |= STYLE_NAME;
			style_ptr->style_name = style_name;
		}
		if (matched_param[2]) {
			style_ptr->style_flags |= STYLE_SYNOPSIS;
			style_ptr->style_synopsis = style_synopsis;
		}

		// Read the style block list and other optional style tags.

		while ((tag_ptr = parse_next_tag(style_tag_list, TOKEN_STYLE)) != NULL) {
			switch (tag_ptr->tag_name) {
			case TOKEN_PLACEHOLDER:
				style_ptr->style_flags |= PLACEHOLDER_TEXTURE;
				style_ptr->placeholder_texture = placeholder_texture;
				break;
			case TOKEN_SKY:
				if (matched_param[0]) {
					style_ptr->style_flags |= SKY_TEXTURE;
					style_ptr->sky_texture = sky_texture;
				}
				if (matched_param[1]) {
					style_ptr->style_flags |= SKY_COLOUR;
					style_ptr->sky_colour = sky_colour;
				}
				if (matched_param[2]) {
					style_ptr->style_flags |= SKY_BRIGHTNESS;
					style_ptr->sky_brightness = sky_brightness;
				}
				break;
			case TOKEN_GROUND:
				if (matched_param[0]) {
					style_ptr->style_flags |= GROUND_TEXTURE;
					style_ptr->ground_texture = ground_texture;
				}
				if (matched_param[1]) {
					style_ptr->style_flags |= GROUND_COLOUR;
					style_ptr->ground_colour = ground_colour;
				}
				break;
			case TOKEN_ORB:
				if (matched_param[0]) {
					style_ptr->style_flags |= ORB_TEXTURE;
					style_ptr->orb_texture = orb_texture;
				}
				if (matched_param[1]) {
					style_ptr->style_flags |= ORB_COLOUR;
					style_ptr->orb_colour = orb_colour;
				}
				if (matched_param[2]) {
					style_ptr->style_flags |= ORB_BRIGHTNESS;
					style_ptr->orb_brightness = orb_brightness;
				}
				if (matched_param[3]) {
					style_ptr->style_flags |= ORB_POSITION;
					style_ptr->orb_position = orb_position;
				}
				if (matched_param[4]) {
					style_ptr->style_flags |= ORB_HREF;
					style_ptr->orb_href = orb_href;
				}
				if (matched_param[5]) {
					style_ptr->style_flags |= ORB_TARGET;
					style_ptr->orb_target = orb_target;
				}
				if (matched_param[6]) {
					style_ptr->style_flags |= ORB_TEXT;
					style_ptr->orb_text = orb_text;
				}
				break;
			case TOKEN_BLOCK:
				if (!matched_param[1] || strlen(block_double) != 2)
					block_double[0] = '\0';
				block_file_path = temp_blocks_dir;
				block_file_path += block_file_name;
				if ((block_ptr = read_block_file(block_file_path)) != NULL)
					add_block_to_style(block_symbol, block_double,
						block_file_name, block_ptr);
				break;
			}
		}

		// Pop the style file.

		pop_file();
		return(true);
	}
	catch (char *error_str) {
		pop_file();
		message("Error while loading style", error_str);
		return(false);
	}
}

//==============================================================================
// Block and style file writing functions.
//==============================================================================

//------------------------------------------------------------------------------
// Write a style file to the blockset.
//------------------------------------------------------------------------------

static void
write_style_file(char *style_file_name)
{
	string style_file_path;
	FILE *fp;
	block_def *block_def_ptr;

	// Open the temporary style file.

	style_file_path = temp_dir;
	style_file_path += style_file_name;
	if ((fp = fopen(style_file_path, "w")) == NULL)
		file_error("Unable to open temporary style file '%s' for writing",
			style_file_path);

	// Output the style start tag.

	fprintf(fp, "<STYLE");
	if (style_ptr->style_flags & STYLE_VERSION)
		fprintf(fp, " VERSION=\"%s\"", style_ptr->style_version);
	if (style_ptr->style_flags & STYLE_NAME)
		fprintf(fp, " NAME=\"%s\"", style_ptr->style_name);
	if (style_ptr->style_flags & STYLE_SYNOPSIS)
		fprintf(fp, " SYNOPSIS=\"%s\"", style_ptr->style_synopsis);
	fprintf(fp, ">\n");

	// If there is a placeholder texture, output the placeholder tag.

	if (style_ptr->style_flags & PLACEHOLDER_TEXTURE)
		fprintf(fp, "\t<PLACEHOLDER TEXTURE=\"%s\"/>\n", 
			style_ptr->placeholder_texture);

	// If there is a sky defined, output the sky tag.

	if (style_ptr->style_flags & SKY_DEFINED) {
		fprintf(fp, "\t<SKY");
		if (style_ptr->style_flags & SKY_TEXTURE)
			fprintf(fp, " TEXTURE=\"%s\"", style_ptr->sky_texture);
		if (style_ptr->style_flags & SKY_COLOUR)
			fprintf(fp, " COLOUR=\"%s\"", style_ptr->sky_colour);
		if (style_ptr->style_flags & SKY_BRIGHTNESS)
			fprintf(fp, " BRIGHTNESS=\"%s\"", style_ptr->sky_brightness);
		fprintf(fp, "/>\n");
	}

	// If there is a ground defined, output the ground tag.

	if (style_ptr->style_flags & GROUND_DEFINED) {
		fprintf(fp, "\t<GROUND");
		if (style_ptr->style_flags & GROUND_TEXTURE)
			fprintf(fp, " TEXTURE=\"%s\"", style_ptr->ground_texture);
		if (style_ptr->style_flags & GROUND_COLOUR)
			fprintf(fp, " COLOUR=\"%s\"", style_ptr->ground_colour);
		fprintf(fp, "/>\n");
	}

	// If there is an orb defined, output the orb tag.

	if (style_ptr->style_flags & ORB_DEFINED) {
		fprintf(fp, "\t<ORB");
		if (style_ptr->style_flags & ORB_TEXTURE)
			fprintf(fp, " TEXTURE=\"%s\"", style_ptr->orb_texture);
		if (style_ptr->style_flags & ORB_COLOUR)
			fprintf(fp, " COLOUR=\"%s\"", style_ptr->orb_colour);
		if (style_ptr->style_flags & ORB_BRIGHTNESS)
			fprintf(fp, " BRIGHTNESS=\"%s\"", style_ptr->orb_brightness);
		if (style_ptr->style_flags & ORB_POSITION)
			fprintf(fp, " POSITION=\"%s\"", style_ptr->orb_position);
		if (style_ptr->style_flags & ORB_HREF)
			fprintf(fp, " HREF=\"%s\"", style_ptr->orb_href);
		if (style_ptr->style_flags & ORB_TARGET)
			fprintf(fp, " TARGET=\"%s\"", style_ptr->orb_target);
		if (style_ptr->style_flags & ORB_TEXT)
			fprintf(fp, " TEXT=\"%s\"", style_ptr->orb_text);
		fprintf(fp, "/>\n");
	}

	// Step through the block list, and output a block tag for each.

	block_def_ptr = style_ptr->block_def_list;
	while (block_def_ptr) {
		fprintf(fp, "\t<BLOCK SYMBOL=\"%c\"", block_def_ptr->symbol);
		if (block_def_ptr->flags & BLOCK_DOUBLE_SYMBOL)
			fprintf(fp, " DOUBLE=\"%s\"", block_def_ptr->double_symbol);
		fprintf(fp, " FILE=\"%s\"/>\n", block_def_ptr->file_name);
		block_def_ptr = block_def_ptr->next_block_def_ptr;
	}

	// Output the style end tag, then close the temporary file.

	fprintf(fp, "</STYLE>\n");
	fclose(fp);

	// Open the temporary file, write it to the blockset, then close the
	// temporary file.

	if (!push_file(style_file_path))
		file_error("Unable to open temporary style file '%s' for reading",
			style_file_path);
	if (!write_ZIP_file(style_file_name))
		file_error("Unable to write style file '%s' to blockset", 
			style_file_name);
	pop_file();
}

//------------------------------------------------------------------------------
// Round a vertex coordinate to 0.0 or 256.0 if it's within one pixel of those
// values.
//------------------------------------------------------------------------------

double
round_vertex_coord(double coord)
{
	if (coord > -1.0 && coord < 1.0)
		coord = 0.0;
	if (coord > 255.0 && coord < 257.0)
		coord = 256.0;
	return(coord);
}

//------------------------------------------------------------------------------
// Write a block file to the blockset
//------------------------------------------------------------------------------

static void
write_block_file(block_def *block_def_ptr)
{
	FILE *fp;
	string temp_file_path;
	string block_file_path;
	block *block_ptr;
	vertex *vertex_ptr;
	part *part_ptr;

	// Open the temporary block file.

	temp_file_path = temp_blocks_dir;
	temp_file_path += block_def_ptr->file_name;
	if ((fp = fopen(temp_file_path, "w")) == NULL)
		file_error("Unable to open temporary block file '%s' for writing",
			temp_file_path);

	// Output the block start tag.

	block_ptr = block_def_ptr->block_ptr;
	fprintf(fp, "<BLOCK NAME=\"%s\"", block_ptr->name);
	if (block_ptr->flags & BLOCK_TYPE)
		fprintf(fp, " TYPE=\"%s\"", get_block_type(block_ptr->type));
	if (block_ptr->flags & BLOCK_ENTRANCE)
		fprintf(fp, " ENTRANCE=\"%s\"", block_ptr->entrance);
	fprintf(fp, ">\n");

	// If this is a structural block...

	if (block_ptr->type == STRUCTURAL_BLOCK) {

		// Output the vertex list.  Vertex coordinates that are close to 0.0 or
		// 256.0 will be rounded to these values.

		fprintf(fp, "\t<VERTICES>\n");
		vertex_ptr = block_ptr->vertex_list;
		while (vertex_ptr) {
			double x, y, z;

			x = round_vertex_coord(vertex_ptr->x);
			y = round_vertex_coord(vertex_ptr->y);
			z = round_vertex_coord(vertex_ptr->z);
			fprintf(fp, "\t\t<VERTEX");
			fprintf(fp, " REF=\"%d\" COORDS=\"(%10.5f,%10.5f,%10.5f)\"/>\n", 
				 vertex_ptr->vertex_no, x, y, z);
			vertex_ptr = vertex_ptr->next_vertex_ptr;
		}
		fprintf(fp, "\t</VERTICES>\n");

		// Output the part list.

		fprintf(fp, "\t<PARTS>\n");
		part_ptr = block_ptr->part_list;
		while (part_ptr) {

			// Output the start tag for the part list.

			fprintf(fp, "\t\t<PART NAME=\"%s\"", part_ptr->name);
			if (part_ptr->flags & PART_TEXTURE)
				fprintf(fp, " TEXTURE=\"%s\"", part_ptr->texture);
			if (part_ptr->flags & PART_COLOUR)
				fprintf(fp, " COLOUR=\"%s\"", part_ptr->colour);
			if (part_ptr->flags & PART_TRANSLUCENCY)
				fprintf(fp, " TRANSLUCENCY=\"%s\"", part_ptr->translucency);
			if (part_ptr->flags & PART_STYLE)
				fprintf(fp, " STYLE=\"%s\"", part_ptr->style);
			if (part_ptr->flags & PART_FACES)
				fprintf(fp, " FACES=\"%s\"", part_ptr->faces);
			if (part_ptr->flags & PART_ANGLE)
				fprintf(fp, " ANGLE=\"%s\"", part_ptr->angle);
			fprintf(fp, ">\n");

			// Output all polygons that belong to this part.

			polygon *polygon_ptr = block_ptr->polygon_list;
			while (polygon_ptr) {
				if (polygon_ptr->part_no == part_ptr->part_no) {

					// Output the vertex list.

					vertex *vertex_ptr = 
						block_ptr->get_vertex(polygon_ptr->get_vertex_def(0));
					fprintf(fp, "\t\t\t<POLYGON ");
					fprintf(fp, "REF=\"%d\" VERTICES=\"%d", 
						polygon_ptr->polygon_no, vertex_ptr->vertex_no);
					for (int index = 1; index < polygon_ptr->vertices;
						index++) {
						vertex_ptr = block_ptr->get_vertex(
							polygon_ptr->get_vertex_def(index));
						fprintf(fp, ",%d", vertex_ptr->vertex_no);
					}
					fprintf(fp, "\"");

					// Output the texture coordinate list.

					vertex_def *vertex_def_ptr = polygon_ptr->get_vertex_def(0);
					fprintf(fp, " TEXCOORDS=\"(%g,%g)", vertex_def_ptr->u, 
						vertex_def_ptr->v);
					for (index = 1; index < polygon_ptr->vertices; index++) {
						vertex_def_ptr = polygon_ptr->get_vertex_def(index);
						fprintf(fp, ",(%g,%g)", vertex_def_ptr->u, 
							vertex_def_ptr->v);
					}
						
					// Output the front and rear polygon reference numbers for
					// the BSP tree, if there is one.

					if (block_ptr->has_BSP_tree) {
						fprintf(fp, "\" FRONT=\"");
						if (polygon_ptr->front_polygon_ptr)
							fprintf(fp, "%d", 
								polygon_ptr->front_polygon_ptr->polygon_no);
						else
							fprintf(fp, "0");
						fprintf(fp, "\" REAR=\"");
						if (polygon_ptr->rear_polygon_ptr)
							fprintf(fp, "%d", 
								polygon_ptr->rear_polygon_ptr->polygon_no);
						else
							fprintf(fp, "0");
					}
					fprintf(fp, "\"/>\n");
				}
				polygon_ptr = polygon_ptr->next_polygon_ptr;
			}

			// Output the end tag for the part list.

			fprintf(fp, "\t\t</PART>\n");
			part_ptr = part_ptr->next_part_ptr;
		}

		// Output the end tag for the parts list.

		fprintf(fp, "\t</PARTS>\n");

		// Output the BSP_TREE tag, if there is one.

		if (block_ptr->has_BSP_tree) {
			fprintf(fp, "\t<BSP_TREE ROOT=\"");
			if (block_ptr->root_polygon_ptr)
				fprintf(fp, "%d", block_ptr->root_polygon_ptr->polygon_no);
			else
				fprintf(fp, "0");
			fprintf(fp, "\"/>\n");
		}
	}

	// If this is a sprite block, output the part tag.

	else {
		part_ptr = block_ptr->part_list;
		fprintf(fp, "\t<PART NAME=\"%s\"", part_ptr->name);
		if (part_ptr->flags & PART_TEXTURE)
			fprintf(fp, " TEXTURE=\"%s\"", part_ptr->texture);
		if (part_ptr->flags & PART_COLOUR)
			fprintf(fp, " COLOUR=\"%s\"", part_ptr->colour);
		if (part_ptr->flags & PART_TRANSLUCENCY)
			fprintf(fp, " TRANSLUCENCY=\"%s\"", part_ptr->translucency);
		if (part_ptr->flags & PART_STYLE)
			fprintf(fp, " STYLE=\"%s\"", part_ptr->style);
		if (part_ptr->flags & PART_FACES)
			fprintf(fp, " FACES=\"%s\"", part_ptr->faces);
		if (part_ptr->flags & PART_ANGLE)
			fprintf(fp, " ANGLE=\"%s\"", part_ptr->angle);
		fprintf(fp, "/>\n");
	}

	// Output the light tag if there is a light for this block.

	if (block_ptr->flags & LIGHT_DEFINED) {
		if (block_ptr->point_light)
			fprintf(fp, "\t<POINT_LIGHT");
		else
			fprintf(fp, "\t<SPOT_LIGHT");
		if (block_ptr->flags & LIGHT_STYLE)
			fprintf(fp, " STYLE=\"%s\"", block_ptr->light_style);
		if (block_ptr->flags & LIGHT_BRIGHTNESS)
			fprintf(fp, " BRIGHTNESS=\"%s\"", block_ptr->light_brightness);
		if (block_ptr->flags & LIGHT_COLOUR)
			fprintf(fp, " COLOUR=\"%s\"", block_ptr->light_colour);
		if (block_ptr->flags & LIGHT_RADIUS)
			fprintf(fp, " RADIUS=\"%s\"", block_ptr->light_radius);
		if (block_ptr->flags & LIGHT_POSITION)
			fprintf(fp, " POSITION=\"%s\"", block_ptr->light_position);
		if (block_ptr->flags & LIGHT_SPEED)
			fprintf(fp, " SPEED=\"%s\"", block_ptr->light_speed);
		if (block_ptr->flags & LIGHT_FLOOD)
			fprintf(fp, " FLOOD=\"%s\"", block_ptr->light_flood);
		if (block_ptr->flags & LIGHT_DIRECTION)
			fprintf(fp, " DIRECTION=\"%s\"", block_ptr->light_direction);
		if (block_ptr->flags & LIGHT_CONE)
			fprintf(fp, " CONE=\"%s\"", block_ptr->light_cone);
		fprintf(fp, "/>\n");
	}

	// Output the sound tag if there is a sound for this block.

	if (block_ptr->flags & SOUND_DEFINED) {
		fprintf(fp, "\t<SOUND");
		if (block_ptr->flags & SOUND_FILE)
			fprintf(fp, " FILE=\"%s\"", block_ptr->sound_file);
		if (block_ptr->flags & SOUND_RADIUS)
			fprintf(fp, " RADIUS=\"%s\"", block_ptr->sound_radius);
		if (block_ptr->flags & SOUND_VOLUME)
			fprintf(fp, " VOLUME=\"%s\"", block_ptr->sound_volume);
		if (block_ptr->flags & SOUND_PLAYBACK) 
			fprintf(fp, " PLAYBACK=\"%s\"", block_ptr->sound_playback);
		if (block_ptr->flags & SOUND_DELAY)
			fprintf(fp, " DELAY=\"%s\"", block_ptr->sound_delay);
		if (block_ptr->flags & SOUND_FLOOD)
			fprintf(fp, " FLOOD=\"%s\"", block_ptr->sound_flood);
		if (block_ptr->flags & SOUND_ROLLOFF) 
			fprintf(fp, " ROLLOFF=\"%s\"", block_ptr->sound_rolloff);
		fprintf(fp, "/>\n");
	}

	// Output the exit tag if there is an exit for this block.

	if (block_ptr->flags & EXIT_DEFINED) {
		fprintf(fp, "\t<EXIT");
		if (block_ptr->flags & EXIT_HREF)
			fprintf(fp, " HREF=\"%s\"", block_ptr->exit_href);
		if (block_ptr->flags & EXIT_TARGET)
			fprintf(fp, " TARGET=\"%s\"", block_ptr->exit_target);
		if (block_ptr->flags & EXIT_TEXT)
			fprintf(fp, " TEXT=\"%s\"", block_ptr->exit_text);
		if (block_ptr->flags & EXIT_TRIGGER)
			fprintf(fp, " TRIGGER=\"%s\"", block_ptr->exit_trigger);
		fprintf(fp, "/>\n");
	}

	// Output the param tag if there is one.

	if (block_ptr->flags & PARAM_DEFINED) {
		fprintf(fp, "\t<PARAM");
		if (block_ptr->flags & PARAM_ORIENT)
			fprintf(fp, " ORIENT=\"%s\"", block_ptr->param_orient);
		if (block_ptr->flags & PARAM_SOLID)
			fprintf(fp, " SOLID=\"%s\"", block_ptr->param_solid);
		if (block_ptr->flags & PARAM_ANGLE)
			fprintf(fp, " ANGLE=\"%s\"", block_ptr->param_angle);
		if (block_ptr->flags & PARAM_SPEED)
			fprintf(fp, " SPEED=\"%s\"", block_ptr->param_speed);
		if (block_ptr->flags & PARAM_ALIGN)
			fprintf(fp, " ALIGN=\"%s\"", block_ptr->param_align);
		fprintf(fp, "/>\n");
	}

	// Output the block end tag, then close the file.

	fprintf(fp, "</BLOCK>\n");
	fclose(fp);

	// Open the temporary block file, write it to the blockset, then close the
	// temporary file.

	if (!push_file(temp_file_path))
		file_error("Unable to open temporary block file '%s' for reading",
			temp_file_path);
	block_file_path = "blocks/";
	block_file_path += block_def_ptr->file_name;
	if (!write_ZIP_file(block_file_path))
		file_error("Unable to write block file '%s' to blockset", 
			block_file_path);
	pop_file();
}

//==============================================================================
// Blockset loading and saving functions.
//==============================================================================

//------------------------------------------------------------------------------
// Load a blockset.
//------------------------------------------------------------------------------

bool
load_blockset(char *file_path)
{
	unzFile unzip_handle;
	unz_file_info file_info;
	char source_file_path[_MAX_PATH];
	char target_file_path[_MAX_PATH];
	char *ch_ptr;
	char *file_buffer_ptr;
	int file_size;
	FILE *fp;

	try {
		
		// Create the style object.

		if ((style_ptr = new style) == NULL)
			memory_error("style");

		// Save the blockset file path, and extract the directory path.

		strcpy(blockset_file_path, file_path);
		strcpy(blocksets_dir, file_path);
		ch_ptr = strrchr(blocksets_dir, '\\');
		*ch_ptr = '\0';

		// Open the blockset.

		if ((unzip_handle = unzOpen(blockset_file_path)) == NULL)
			file_error("Unable to open blockset '%s'", blockset_file_path);

		// Go to first file in the blockset.

		if (unzGoToFirstFile(unzip_handle) != UNZ_OK)
			file_error("Unable to open first file in blockset");
		do {

			// Get the file path and info.

			unzGetCurrentFileInfo(unzip_handle, &file_info, source_file_path,
				_MAX_PATH, NULL, 0, NULL, 0);

			// If the uncompressed file size is zero, this is a directory not
			// a file, so skip over it.

			if (file_info.uncompressed_size == 0)
				continue;

			// Get the uncompressed size of the file, and allocate the file
			// buffer.

			if ((file_buffer_ptr = new char[file_info.uncompressed_size]) 
				== NULL)
				memory_error("file buffer");

			// Open the file, read it into the file buffer, then close the file.

			if (unzOpenCurrentFile(unzip_handle) != UNZ_OK)
				file_error("Unable to open file '%s' in blockset", 
					source_file_path);
			file_size = unzReadCurrentFile(unzip_handle, file_buffer_ptr,
				file_info.uncompressed_size);
			unzCloseCurrentFile(unzip_handle);
			if (file_size < 0)
				file_error("Unable to read file '%s' in blockset", 
					source_file_path);

			// Open a new file in the temporary directory, write the file buffer
			// to it, then close the file.

			strcpy(target_file_path, temp_dir);
			strcat(target_file_path, source_file_path);
			if ((fp = fopen(target_file_path, "wb")) == NULL)
				file_error("Unable to open file '%s' for writing",
					target_file_path);
			fwrite(file_buffer_ptr, file_size, 1, fp);
			fclose(fp);

			// Delete the file buffer.

			delete []file_buffer_ptr;

			// Go to next file in blockset.

		} while (unzGoToNextFile(unzip_handle) == UNZ_OK);

		// Close the blockset.

		unzClose(unzip_handle);

		// Construct the name to the style file.

		strcpy(source_file_path, blockset_file_path);
		ch_ptr = strrchr(source_file_path, '\\') + 1;
		strcpy(target_file_path, temp_dir);
		strcat(target_file_path, ch_ptr);
		ch_ptr = strrchr(target_file_path, '.');
		*ch_ptr = '\0';
		strcat(target_file_path, ".style");

		// Process the style file in the blockset.

		read_style_file(target_file_path);
		return(true);
	}
	catch (char *error_str) {
		if (unzip_handle)
			unzClose(unzip_handle);
		message("Error", error_str);
		return(false);
	}
}

//------------------------------------------------------------------------------
// Save a blockset.
//------------------------------------------------------------------------------

bool
save_blockset(char *file_path)
{
	char *name_ptr, *ext_ptr;
	char style_file_path[_MAX_PATH];
	block_def *block_def_ptr;

	try {

		// Verify that the file path ends with ".bset".  If not, append this
		// extension to the file path.

		strcpy(blockset_file_path, file_path);
		name_ptr = strrchr(blockset_file_path, '\\');
		ext_ptr = strrchr(name_ptr, '.');
		if (ext_ptr == NULL || stricmp(ext_ptr, ".bset"))
			strcat(blockset_file_path, ".bset");

		// Extract the directory path of the blockset.

		strcpy(blocksets_dir, blockset_file_path);
		name_ptr = strrchr(blocksets_dir, '\\');
		*name_ptr = '\0';

		// Construct the path to the style file within the blockset.

		strcpy(style_file_path, name_ptr + 1);
		ext_ptr = strrchr(style_file_path, '.');
		*ext_ptr = '\0';
		strcat(style_file_path, ".style");

		// Create the blockset as a ZIP archive.

		if (!create_ZIP_archive(blockset_file_path))
			file_error("Unable to create blockset '%s'", blockset_file_path);

		// Write a new style file to the ZIP achive.

		write_style_file(style_file_path);

		// Step through the list of blocks, and write them to the ZIP archive.

		block_def_ptr = style_ptr->block_def_list;
		while (block_def_ptr) {
			write_block_file(block_def_ptr);
			block_def_ptr = block_def_ptr->next_block_def_ptr;
		}

		// Step through the files in the temporary textures and sounds 
		// directories, and write them to the ZIP archive.

		write_directory(temp_textures_dir, "textures/");
		write_directory(temp_sounds_dir, "sounds/");

		// Close the blockset.

		close_ZIP_archive();
		return(true);
	}
	catch (char *error_str) {
		close_ZIP_archive();
		message("Error", error_str);
		return(false);
	}
}