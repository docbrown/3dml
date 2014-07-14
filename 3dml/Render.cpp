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
#include <stdarg.h>
#include <math.h>
#include <crtdbg.h>
#include <amdlib.h>
#include "Classes.h"
#include "Fileio.h"
#include "Light.h"
#include "Main.h"
#include "Memory.h"
#include "Parser.h"
#include "Platform.h"
#include "Plugin.h"
#include "Spans.h"
#include "Utils.h"

// Current translation and rotation matrices.

static float curr_translate_matrix[16];
static float curr_rotatey_matrix[16];
static float curr_rotatex_matrix[16];
static float curr_camera_matrix[16];
static float curr_transform_matrix[16];

// List of solid colour spans, transparent screen polygons and colour screen
// polygons.

static span *colour_span_list;
static spolygon *transparent_spolygon_list;
static spolygon *colour_spolygon_list;

// The current absolute and relative camera position.

static vertex camera_position;
static int camera_column, camera_row, camera_level;
static vertex relative_camera_position;

// The current view vector in world space.

static vector view_vector;

// The current square and block being rendered, it's type, and whether it's
// movable or not.

static square *curr_square_ptr;
static block *curr_block_ptr;
static blocktype curr_block_type;
static bool curr_block_movable;
static vertex block_centre;

// The current block translation.

static vertex block_translation;

// The current block's transformed vertex list.

static int max_block_vertices;
static vertex *block_tvertex_list;

// The current polygon's vertex colour list and front face visible flag.

static int max_polygon_vertices;
static RGBcolour *vertex_colour_list;
static bool front_face_visible;

// Variables used to keep track of currently rendered polygon.

static int spoints;						// Number of spoints.
static spoint *main_spoint_list;		// Main screen point list.
static spoint *temp_spoint_list;		// Temporary screen point list.
static spoint *first_spoint_ptr;		// Pointer to first screen point.
static spoint *last_spoint_ptr;			// Pointer to last screen point.
static spoint *top_spoint_ptr;			// Pointer to topmost screen point.
static spoint *bottom_spoint_ptr;		// Pointer to bottommost screen point.
static spoint *left_spoint1_ptr;		// Endpoints of current left edge.
static spoint *left_spoint2_ptr;	
static spoint *right_spoint1_ptr;		// Endpoints of current right edge.
static spoint *right_spoint2_ptr;
static edge left_edge, right_edge;		// Current left and right edge.
static edge left_slope, right_slope;	// Current left and right slope.
static float half_texel_u;				// Normalised size of half a texel.
static float half_texel_v;

// Texture coordinate scaling list.

static float uv_scale_list[IMAGE_SIZES] = {
	1.0f, 2.0f, 4.0f, 8.0f, 16.0f, 32.0f, 64.0f, 128.0f
};

// One over texture dimensions list.

float one_on_dimensions_list[IMAGE_SIZES] = {
	3.90625e-3f, 7.8125e-3f, 1.5625e-2f, 0.03125f, 0.0625f, 0.125f, 0.25f, 0.5f
};

// Transformed frustum vertex list (in world space), frustum normal vector list,
// frustum plane offset list, and view bounding box.

static vertex frustum_tvertex_list[FRUSTUM_VERTICES];
static vector frustum_normal_vector_list[FRUSTUM_PLANES];
static float frustum_plane_offset_list[FRUSTUM_PLANES];
static vertex min_view, max_view;

// Visible popup list, last popup in list, and currently selected popup.

static popup *visible_popup_list;
static popup *last_visible_popup_ptr;
static popup *curr_popup_ptr;

// Flag indicating whether a polygon or popup selection has been found.

static bool found_selection;

// Polygons rendered in current block and frame.

static int polygons_rendered_in_block;
static int polygons_rendered_in_frame;
static int polygons_processed_in_frame;
static int polygons_processed_late_in_frame;
static int blocks_rendered_in_frame;
static int blocks_processed_in_frame;

DEFINE_SUM(render_block_cycles);
DEFINE_SUM(find_light_cycles);
DEFINE_SUM(render_polygon_cycles);
DEFINE_SUM(clip_3D_polygon_cycles);
DEFINE_SUM(clip_2D_polygon_cycles);
DEFINE_SUM(lighting_cycles);
DEFINE_SUM(scale_texture_cycles);
DEFINE_SUM(cull_block_cycles);
DEFINE_SUM(transform_vertex_cycles);

// Current screen coordinates and size of orb.

static float curr_orb_x, curr_orb_y;
static float curr_orb_width, curr_orb_height;

//------------------------------------------------------------------------------
// Initialise the renderer.
//------------------------------------------------------------------------------

void
init_renderer(void)
{
	block_tvertex_list = NULL;
	vertex_colour_list = NULL;
	temp_spoint_list = NULL;
	hardware_init_vertex_list();
}

//------------------------------------------------------------------------------
// Set up the renderer by creating screen point lists large enough to handle
// the polygon with the most vertices.
//------------------------------------------------------------------------------

void
set_up_renderer(void)
{
	blockset *blockset_ptr;
	block_def *block_def_ptr;
	int polygon_no;
	polygon *polygon_ptr;

	// Step through all block definitions, and remember the most vertices
	// seen in a block and polygon.

	max_block_vertices = 0;
	max_polygon_vertices = 0;
	blockset_ptr = blockset_list_ptr->first_blockset_ptr;
	while (blockset_ptr) {
		block_def_ptr = blockset_ptr->block_def_list;
		while (block_def_ptr) {
			if (block_def_ptr->vertices > max_block_vertices)
				max_block_vertices = block_def_ptr->vertices;
			for (polygon_no = 0; polygon_no < block_def_ptr->polygons;
				polygon_no++) {
				polygon_ptr = &block_def_ptr->polygon_list[polygon_no];
				if (polygon_ptr->vertices > max_polygon_vertices)
					max_polygon_vertices = polygon_ptr->vertices;
			}
			block_def_ptr = block_def_ptr->next_block_def_ptr;
		}
		blockset_ptr = blockset_ptr->next_blockset_ptr;
	}
	block_def_ptr = custom_blockset_ptr->block_def_list;
	while (block_def_ptr) {
		if (block_def_ptr->vertices > max_block_vertices)
			max_block_vertices = block_def_ptr->vertices;
		for (polygon_no = 0; polygon_no < block_def_ptr->polygons; 
			polygon_no++) {
			polygon_ptr = &block_def_ptr->polygon_list[polygon_no];
			if (polygon_ptr->vertices > max_polygon_vertices)
				max_polygon_vertices = polygon_ptr->vertices;
		}
		block_def_ptr = block_def_ptr->next_block_def_ptr;
	}

	// Compute the maximum number of screen points as the maximum number of
	// polygon vertices + 5; this caters for polygons clipped to the viewing
	// plane and the four screen edges.

	set_max_screen_points(max_polygon_vertices + 5);

	// Create the transformed vertex list.

	NEWARRAY(block_tvertex_list, vertex, max_block_vertices);
	if (block_tvertex_list == NULL)
		memory_error("block transformed vertex list");

	// Create the vertex colour list.

	NEWARRAY(vertex_colour_list, RGBcolour, max_polygon_vertices);
	if (vertex_colour_list == NULL)
		memory_error("vertex colour list");

	// Create the temp screen point list and the hardware vertex list.

	NEWARRAY(temp_spoint_list, spoint, max_polygon_vertices + 5);
	if (temp_spoint_list == NULL)
		memory_error("screen point list");
	if (!hardware_create_vertex_list(max_polygon_vertices + 5))
		memory_error("hardware vertex list");
}

//------------------------------------------------------------------------------
// Clean up the renderer by deleting the block transformed vertex list,
// vertex colour list, temporary screen point list and hardware vertex list.
//------------------------------------------------------------------------------

void
clean_up_renderer(void)
{
	if (block_tvertex_list)
		DELARRAY(block_tvertex_list, vertex, max_block_vertices);
	if (vertex_colour_list)
		DELARRAY(vertex_colour_list, RGBcolour, max_polygon_vertices);
	if (temp_spoint_list)
		DELARRAY(temp_spoint_list, spoint, max_polygon_vertices + 5);
	hardware_destroy_vertex_list(max_polygon_vertices + 5);
}

//------------------------------------------------------------------------------
// Translate a vertex by the player position.
//------------------------------------------------------------------------------

void
translate_vertex(vertex *old_vertex_ptr, vertex *new_vertex_ptr)
{
	// Translate the old vertex by the inverse of the player viewpoint.

	new_vertex_ptr->x = old_vertex_ptr->x - player_viewpoint.position.x;
	new_vertex_ptr->y = old_vertex_ptr->y - player_viewpoint.position.y;
	new_vertex_ptr->z = old_vertex_ptr->z - player_viewpoint.position.z;
}

//------------------------------------------------------------------------------
// Rotate a vertex by the player orientation.
//------------------------------------------------------------------------------

void
rotate_vertex(vertex *old_vertex_ptr, vertex *new_vertex_ptr)
{
	float rz;

	// Rotate the vertex around the Y axis by the inverse of the player turn 
	// angle.

	new_vertex_ptr->x = 
		old_vertex_ptr->x * cosine.table[player_viewpoint.inv_turn_angle] + 
		old_vertex_ptr->z * sine.table[player_viewpoint.inv_turn_angle];
	rz = old_vertex_ptr->z * cosine.table[player_viewpoint.inv_turn_angle] - 
		old_vertex_ptr->x * sine.table[player_viewpoint.inv_turn_angle];

	// Rotate the vertex around the X axis by the inverse of the player look
	// angle.

	new_vertex_ptr->y = 
		old_vertex_ptr->y * cosine.table[player_viewpoint.inv_look_angle] - 
		rz * sine.table[player_viewpoint.inv_look_angle];
	new_vertex_ptr->z = rz * cosine.table[player_viewpoint.inv_look_angle] + 
		old_vertex_ptr->y * sine.table[player_viewpoint.inv_look_angle];
}

//------------------------------------------------------------------------------
// Set a matrix to translate a vertex by the player position.
//------------------------------------------------------------------------------

static void
set_translate_matrix(float *m)
{
	m[0] = 1.0f;		m[1] = 0.0f;		m[2] = 0.0f;	m[3] = 0.0f;
	m[4] = 0.0f;		m[5] = 1.0f;		m[6] = 0.0f;	m[7] = 0.0f;
	m[8] = 0.0f;		m[9] = 0.0f;		m[10] = 1.0f;	m[11] = 0.0f;
	m[12] = -player_viewpoint.position.x;
	m[13] = -player_viewpoint.position.y;
	m[14] = -player_viewpoint.position.z;
	m[15] = 1.0f;
}

//------------------------------------------------------------------------------
// Set a matrix to rotate a vertex by the player turn angle.
//------------------------------------------------------------------------------

static void
set_rotatey_matrix(float *m)
{
	float sin_angle = sine.table[player_viewpoint.inv_turn_angle];
	float cos_angle = cosine.table[player_viewpoint.inv_turn_angle];
	m[0] = cos_angle;	m[1] = 0.0f;		m[2] = -sin_angle;	m[3] = 0.0f; 
	m[4] = 0.0f;		m[5] = 1.0f;		m[6] = 0.0f;		m[7] = 0.0f;
	m[8] = sin_angle;	m[9] = 0.0f;		m[10] = cos_angle;	m[11] = 0.0f;
	m[12] = 0.0f;		m[13] = 0.0f;		m[14] = 0.0f;		m[15] = 1.0f;
}

//------------------------------------------------------------------------------
// Set a matrix to rotate a vertex by the player look angle.
//------------------------------------------------------------------------------

static void
set_rotatex_matrix(float *m)
{
	float sin_angle = sine.table[player_viewpoint.inv_look_angle];
	float cos_angle = cosine.table[player_viewpoint.inv_look_angle];
	m[0] = 1.0f;		m[1] = 0.0f;		m[2] = 0.0f;		m[3] = 0.0f; 
	m[4] = 0.0f;		m[5] = cos_angle;	m[6] = sin_angle;	m[7] = 0.0f;
	m[8] = 0.0f;		m[9] = -sin_angle;	m[10] = cos_angle;	m[11] = 0.0f;
	m[12] = 0.0f;		m[13] = 0.0f;		m[14] = 0.0f;		m[15] = 1.0f;
}

//------------------------------------------------------------------------------
// Set a matrix to translate a vertex by the camera offset.
//------------------------------------------------------------------------------

static void
set_camera_matrix(float *m)
{
	m[0] = 1.0f;		m[1] = 0.0f;		m[2] = 0.0f;	m[3] = 0.0f;
	m[4] = 0.0f;		m[5] = 1.0f;		m[6] = 0.0f;	m[7] = 0.0f;
	m[8] = 0.0f;		m[9] = 0.0f;		m[10] = 1.0f;	m[11] = 0.0f;
	m[12] = -player_camera_offset.dx;
	m[13] = -player_camera_offset.dy;
	m[14] = -player_camera_offset.dz;
	m[15] = 1.0f;
}

//------------------------------------------------------------------------------
// Transform a vertex by the player position and orientation, and then adjust
// by the current camera offset.
//------------------------------------------------------------------------------

void
transform_vertex(vertex *old_vertex_ptr, vertex *new_vertex_ptr)
{
	// If AMD 3DNow! instructions are supported, perform the transformation
	// via a matrix.

	if (AMD_3DNow_supported) {
		float temp_vertex1[4], temp_vertex2[4], temp_vertex3[4];

//		_trans_v1x4(&new_vertex_ptr->x, curr_transform_matrix,
//			&old_vertex_ptr->x);
		_trans_v1x4(temp_vertex1, curr_translate_matrix, &old_vertex_ptr->x);
		_trans_v1x4(temp_vertex2, curr_rotatey_matrix, temp_vertex1);
		_trans_v1x4(temp_vertex3, curr_rotatex_matrix, temp_vertex2);
		_trans_v1x4(&new_vertex_ptr->x, curr_camera_matrix, temp_vertex3);
	}

	// Otherwise perform the transformation the more traditional way...

	else {
		float tx, ty, tz;
		float rx, ry, rz1, rz2;

		// Translate the old vertex by the inverse of the player viewpoint.

		tx = old_vertex_ptr->x - player_viewpoint.position.x;
		ty = old_vertex_ptr->y - player_viewpoint.position.y;
		tz = old_vertex_ptr->z - player_viewpoint.position.z;

		// Rotate the vertex around the Y axis by the inverse of the player 
		// turn  angle.

		rx = tx * cosine.table[player_viewpoint.inv_turn_angle] + 
			tz * sine.table[player_viewpoint.inv_turn_angle];
		rz1 = tz * cosine.table[player_viewpoint.inv_turn_angle] - 
			tx * sine.table[player_viewpoint.inv_turn_angle];

		// Rotate the vertex around the X axis by the inverse of the player 
		// look angle.

		ry = ty * cosine.table[player_viewpoint.inv_look_angle] - 
			rz1 * sine.table[player_viewpoint.inv_look_angle];
		rz2 = rz1 * cosine.table[player_viewpoint.inv_look_angle] + 
			ty * sine.table[player_viewpoint.inv_look_angle];

		// Translate the vertex by the camera offset.

		new_vertex_ptr->x = rx - player_camera_offset.dx;
		new_vertex_ptr->y = ry - player_camera_offset.dy;
		new_vertex_ptr->z = rz2 - player_camera_offset.dz;
	}
}

//------------------------------------------------------------------------------
// Transform a vertex by the player orientation.
//------------------------------------------------------------------------------

void
transform_vector(vector *old_vector_ptr, vector *new_vector_ptr)
{
	float rdz;

	// Rotate the vector around the Y axis by the inverse of the player turn
	// angle.

	new_vector_ptr->dx = 
		old_vector_ptr->dx * cosine.table[player_viewpoint.inv_turn_angle] + 
		old_vector_ptr->dz * sine.table[player_viewpoint.inv_turn_angle];
	rdz = old_vector_ptr->dz * cosine.table[player_viewpoint.inv_turn_angle] - 
		old_vector_ptr->dx * sine.table[player_viewpoint.inv_turn_angle];

	// Rotate the vertex around the X axis by the inverse of the player look
	// angle.

	new_vector_ptr->dy = 
		old_vector_ptr->dy * cosine.table[player_viewpoint.inv_look_angle] - 
		rdz * sine.table[player_viewpoint.inv_look_angle];
	new_vector_ptr->dz = rdz * cosine.table[player_viewpoint.inv_look_angle] + 
		old_vector_ptr->dy * sine.table[player_viewpoint.inv_look_angle];
}

//------------------------------------------------------------------------------
// Determine whether a polygon is visible, and if so, which face is visible.
//------------------------------------------------------------------------------

static bool
polygon_visible(polygon *polygon_ptr, float angle = 0.0f)
{
	vertex *vertex_list;
	vertex *vertex_ptr;
	int vertex_no;
	bool invisible;
	part *part_ptr;
	texture *texture_ptr;
	vector normal_vector;
	float dot_product;

	// If the polygon has zero faces, it's invisible.

	part_ptr = polygon_ptr->part_ptr;
	if (part_ptr->faces == 0)
		return(false);

	// Step through the list of transformed polygon vertices and check their Z
	// axis coordinates.  If all are in front of the viewing plane, the polygon
	// is invisible.

	vertex_list = block_tvertex_list;
	PREPARE_VERTEX_DEF_LIST(polygon_ptr);
	invisible = true;
	for (vertex_no = 0; vertex_no < polygon_ptr->vertices; vertex_no++) {
		vertex_ptr = VERTEX_PTR(vertex_no);
		if (FGE(vertex_ptr->z, 1.0f)) {
			invisible = false;
			break;
		}
	}
	if (invisible)
		return(false);

	// Calculate the dot product between the view vector, which is the vector
	// from the origin to any transformed vertex of the polygon, and the
	// normal vector rotated to match the viewer and polygon orientation.
	// If the result is negative, we're looking at the front face of the
	// polygon, otherwise we're looking at the back.

	if (FEQ(angle, 0.0f))
		transform_vector(&polygon_ptr->normal_vector, &normal_vector);
	else {
		normal_vector = polygon_ptr->normal_vector;
		normal_vector.rotatey(angle - player_viewpoint.turn_angle);
		normal_vector.rotatex(-player_viewpoint.look_angle);
	}
	vertex_ptr = VERTEX_PTR(0);
	dot_product = vertex_ptr->x * normal_vector.dx + 
		vertex_ptr->y * normal_vector.dy + vertex_ptr->z * normal_vector.dz;
	front_face_visible = FLT(dot_product, 0.0f);

	// If the polygon is two-sided, translucent (hardware acceleration only),
	// or transparent, it is visible  regardless of which face is being viewed.
	
	texture_ptr = part_ptr->texture_ptr;
	if (part_ptr->faces == 2 || 
		(hardware_acceleration && part_ptr->alpha < 1.0f) ||
		(texture_ptr && texture_ptr->transparent))
		return(true);

	// If the polygon is one-sided or opaque, then it's visible if we're
	// looking at the front face.

	return(front_face_visible);
}

//------------------------------------------------------------------------------
// Return a pointer to the vertex furthest from the viewer for the given
// polygon.
//------------------------------------------------------------------------------

static vertex *
get_furthest_vertex(polygon *polygon_ptr)
{
	vertex *vertex_list;
	vertex *furthest_vertex_ptr;
	int index;
	
	// Step through the transformed vertex list to determine which one is
	// the furthest.

	vertex_list = block_tvertex_list;
	PREPARE_VERTEX_DEF_LIST(polygon_ptr);
	furthest_vertex_ptr = VERTEX_PTR(0);
	for (index = 1; index < polygon_ptr->vertices; index++) {
		vertex *vertex_ptr = VERTEX_PTR(index);
		if (vertex_ptr->z > furthest_vertex_ptr->z)
			furthest_vertex_ptr = vertex_ptr;
	}

	// Return a pointer to the furthest transformed vertex.

	return(furthest_vertex_ptr);
}

//------------------------------------------------------------------------------
// Determine the bounding box of a polygon by stepping through it's screen
// point list.  We return pointers to the top, bottom, left and right screen
// points.
//------------------------------------------------------------------------------

static void
get_polygon_bounding_box(spoint *&first_spoint_ptr, spoint *&last_spoint_ptr,
						 spoint *&top_spoint_ptr, spoint *&bottom_spoint_ptr,
						 spoint *&left_spoint_ptr, spoint *&right_spoint_ptr)
{
	spoint *spoint_ptr;

	// Initialise the pointer to the first and last screen point.

	first_spoint_ptr = main_spoint_list;
	last_spoint_ptr = &main_spoint_list[spoints - 1];	

	// Initialise all screen point pointers to the first screen point.

	spoint_ptr = first_spoint_ptr;
	top_spoint_ptr = first_spoint_ptr;
	bottom_spoint_ptr = first_spoint_ptr;
	left_spoint_ptr = first_spoint_ptr;
	right_spoint_ptr = first_spoint_ptr;
	
	// Now step through the remaining screen points to see which are the one's
	// we really want.

	for (spoint_ptr++; spoint_ptr <= last_spoint_ptr; spoint_ptr++) {
		if (spoint_ptr->sy < top_spoint_ptr->sy)
			top_spoint_ptr = spoint_ptr;
		if (spoint_ptr->sy > bottom_spoint_ptr->sy)
			bottom_spoint_ptr = spoint_ptr;
		if (spoint_ptr->sx < left_spoint_ptr->sx)
			left_spoint_ptr = spoint_ptr;
		if (spoint_ptr->sx > right_spoint_ptr->sx)
			right_spoint_ptr = spoint_ptr;
	}
}

//------------------------------------------------------------------------------
// Clip a transformed 3D line to the viewing plane at z = 1.
//------------------------------------------------------------------------------

static void
clip_3D_line(vertex *tvertex1_ptr, vertex_def *vertex1_def_ptr,
			 RGBcolour *vertex1_colour_ptr, vertex *tvertex2_ptr, 
			 vertex_def *vertex2_def_ptr, RGBcolour *vertex2_colour_ptr,
		     vertex *clipped_tvertex_ptr, vertex_def *clipped_vertex_def_ptr,
			 RGBcolour *clipped_vertex_colour_ptr)
{
	float fraction;
	
	// Find the fraction of the line that is behind the viewing plane.
	
	fraction = (1.0f - tvertex1_ptr->z) / (tvertex2_ptr->z - tvertex1_ptr->z);

	// Compute the vertex intersection.

	clipped_tvertex_ptr->x = tvertex1_ptr->x +
		fraction * (tvertex2_ptr->x - tvertex1_ptr->x);
	clipped_tvertex_ptr->y = tvertex1_ptr->y +
		fraction * (tvertex2_ptr->y - tvertex1_ptr->y);
	clipped_tvertex_ptr->z = 1.0;

	// Compute the texture coordinate intersection.
	
	clipped_vertex_def_ptr->u = vertex1_def_ptr->u + 
		fraction * (vertex2_def_ptr->u - vertex1_def_ptr->u);
	clipped_vertex_def_ptr->v = vertex1_def_ptr->v + 
		fraction * (vertex2_def_ptr->v - vertex1_def_ptr->v);

	// Compute the colour intersection.

	clipped_vertex_colour_ptr->red = vertex1_colour_ptr->red +
		fraction * (vertex2_colour_ptr->red - vertex1_colour_ptr->red);
	clipped_vertex_colour_ptr->green = vertex1_colour_ptr->green +
		fraction * (vertex2_colour_ptr->green - vertex1_colour_ptr->green);
	clipped_vertex_colour_ptr->blue = vertex1_colour_ptr->blue +
		fraction * (vertex2_colour_ptr->blue - vertex1_colour_ptr->blue);
}

//------------------------------------------------------------------------------
// Project a transformed vertex into screen space, and add the resulting screen
// point to the polygon's screen point list.  Also compute 1/tz for that screen
// point.
//------------------------------------------------------------------------------

static void
add_spoint_to_list(vertex *tvertex_ptr, vertex_def *vertex_def_ptr, 
				   RGBcolour *vertex_colour_ptr)
{
	spoint *spoint_ptr;
	float one_on_tz, px, py;
	float u, v;

	// Get a pointer to the next available screen point.

	spoint_ptr = &main_spoint_list[spoints++];

	// Project vertex into screen space.

	one_on_tz = 1.0f / tvertex_ptr->z;
	px = tvertex_ptr->x * one_on_tz;
	py = tvertex_ptr->y * one_on_tz;
	spoint_ptr->sx = half_window_width + (px * pixels_per_world_unit);
	spoint_ptr->sy = half_window_height - (py * pixels_per_world_unit);
	
	// Save 1/tz.

	spoint_ptr->one_on_tz = one_on_tz;

	// If or v are 0 or 1, move them in by half a texel to prevent inaccurate
	// texture wrapping.

	u = vertex_def_ptr->u;
	if (FEQ(u, 0.0f))
		u += half_texel_u;
	else if (FEQ(u, 1.0f))
		u -= half_texel_u;
	v = vertex_def_ptr->v;
	if (FEQ(v, 0.0f))
		v += half_texel_v;
	else if (FEQ(v, 1.0f))
		v -= half_texel_v;

	// Compute u/tz and v/tz.

	spoint_ptr->u_on_tz = u * one_on_tz;
	spoint_ptr->v_on_tz = v * one_on_tz;

	// Save the normalised lit colour.

	spoint_ptr->colour = *vertex_colour_ptr;
}

//------------------------------------------------------------------------------
// Clip a transformed polygon against the viewing plane at z = 1, and store the
// projected screen points in the polygon definition's screen point list.
//------------------------------------------------------------------------------

static void
clip_3D_polygon(polygon *polygon_ptr, pixmap *pixmap_ptr)
{
	int vertices;
	int vertex1_no, vertex2_no;

	START_SUMMING;

	// Compute the size of half a texel in normalised texture units for this
	// pixmap.  If there is no pixmap, assume a pixmap size of 256x256.

	if (pixmap_ptr) {
		half_texel_u = 0.5f / pixmap_ptr->width;
		half_texel_v = 0.5f / pixmap_ptr->height;
	} else {
		half_texel_u = 1.953125e-3;
		half_texel_v = 1.953125e-3;
	}

	// Get the required information from the polygon.

	PREPARE_VERTEX_DEF_LIST(polygon_ptr);
	vertices = polygon_ptr->vertices;

	// Step through the transformed vertices of the polygon, checking the status
	// of each edge compared against the viewing plane, and output the new
	// set of vertices.

	spoints = 0;
	vertex1_no = vertices - 1;
	for (vertex2_no = 0; vertex2_no < vertices; vertex2_no++) {
		vertex_def *vertex1_def_ptr, *vertex2_def_ptr, clipped_vertex_def;
		vertex *tvertex1_ptr, *tvertex2_ptr, clipped_tvertex;
		RGBcolour *vertex1_colour_ptr, *vertex2_colour_ptr, 
			clipped_vertex_colour;

		// Get the vertex definition pointers for the current edge, and then
		// get the vertex pointers and brightness values.

		vertex1_def_ptr = &vertex_def_list[vertex1_no];
		vertex2_def_ptr = &vertex_def_list[vertex2_no];
		tvertex1_ptr = &block_tvertex_list[vertex1_def_ptr->vertex_no];
		tvertex2_ptr = &block_tvertex_list[vertex2_def_ptr->vertex_no];
		vertex1_colour_ptr = &vertex_colour_list[vertex1_no];
		vertex2_colour_ptr = &vertex_colour_list[vertex2_no];

		// Compare the edge against the viewing plane at z = 1 and add
		// zero, one or two vertices to the global vertex list.

		if (tvertex1_ptr->z >= 1.0) {

			// If the edge is entirely in front of the viewing plane, add the
			// end point to the screen point list.

			if (tvertex2_ptr->z >= 1.0)
				add_spoint_to_list(tvertex2_ptr, vertex2_def_ptr, 
					vertex2_colour_ptr); 

			// If the edge is crossing over to the back of the viewing plane,
			// add the intersection point to the screen point list.

			else {
				clip_3D_line(tvertex1_ptr, vertex1_def_ptr, vertex1_colour_ptr,
					tvertex2_ptr, vertex2_def_ptr, vertex2_colour_ptr,
					&clipped_tvertex, &clipped_vertex_def, 
					&clipped_vertex_colour);
				add_spoint_to_list(&clipped_tvertex, &clipped_vertex_def,
					&clipped_vertex_colour);
			}			
		} else {

			// If the edge is crossing over to the front of the viewing plane,
			// add the intersection point and the edge's end point to the screen
			// point list.

			if (tvertex2_ptr->z >= 1.0) {
				clip_3D_line(tvertex1_ptr, vertex1_def_ptr, vertex1_colour_ptr,
					tvertex2_ptr, vertex2_def_ptr, vertex2_colour_ptr,
					&clipped_tvertex, &clipped_vertex_def, 
					&clipped_vertex_colour);
				add_spoint_to_list(&clipped_tvertex, &clipped_vertex_def,
					&clipped_vertex_colour);
				add_spoint_to_list(tvertex2_ptr, vertex2_def_ptr,
					vertex2_colour_ptr);
			}
		}
		
		// Move onto the next edge.
	
		vertex1_no = vertex2_no;
	}

	END_SUMMING(clip_3D_polygon_cycles);
}

//------------------------------------------------------------------------------
// Scale u/tz and v/tz for all screen points.
//------------------------------------------------------------------------------

void
scale_texture_interpolants(pixmap *pixmap_ptr, texstyle texture_style)
{
	float u_scale, v_scale;
	float one_on_dimensions;
	int index;

	START_SUMMING;

	// If hardware acceleration is active, scale u/tz and v/tz in each screen
	// point by the ratio of the pixmap size to the cached image size.  If
	// the pixmap is tiled, then scale by the ratio of the maximum pixmap size
	// to the cached image size; if there is no pixmap, don't scale at all.

	if (hardware_acceleration) {
		if (pixmap_ptr == NULL) {
			u_scale = 1.0f;
			v_scale = 1.0f;
		} else {
			if (texture_style == TILED_TEXTURE) {
				u_scale = uv_scale_list[pixmap_ptr->size_index];
				v_scale = u_scale;
			} else {
				one_on_dimensions = 
					one_on_dimensions_list[pixmap_ptr->size_index];
				u_scale = (float)pixmap_ptr->width * one_on_dimensions;
				v_scale = (float)pixmap_ptr->height * one_on_dimensions;
			}
		}
	}
	
	// If hardware acceleration is not active, scale u/tz and v/tz in each
	// screen point by the size of the pixmap.  If there is no pixmap or
	// it is tiled, then use the maximum pixmap size.

	else {
		if (pixmap_ptr == NULL || texture_style == TILED_TEXTURE) {
			u_scale = 256.0f;
			v_scale = u_scale;
		} else {
			u_scale = (float)pixmap_ptr->width;
			v_scale = (float)pixmap_ptr->height;
		}
	}

	// Perform the actual scaling.

	for (index = 0; index < spoints; index++) {
		spoint *spoint_ptr = &main_spoint_list[index];
		spoint_ptr->u_on_tz *= u_scale;
		spoint_ptr->v_on_tz *= v_scale;
	}

	END_SUMMING(scale_texture_cycles);
}

//------------------------------------------------------------------------------
// Clip a 2D line to a vertical screen edge.
//------------------------------------------------------------------------------

static void
clip_2D_line_to_x(spoint *spoint1_ptr, spoint *spoint2_ptr, float sx,
				  spoint *clipped_spoint_ptr)
{
	float fraction;
	
	// Find the fraction of the line that is to the left of the screen edge.
	
	fraction = (sx - spoint1_ptr->sx) / (spoint2_ptr->sx - spoint1_ptr->sx);

	// Compute the vertex intersection.

	clipped_spoint_ptr->sx = sx;
	clipped_spoint_ptr->sy = spoint1_ptr->sy +
		fraction * (spoint2_ptr->sy - spoint1_ptr->sy);

	// Compute the 1/tz, u/tz and v/tz intersections.

	clipped_spoint_ptr->one_on_tz = spoint1_ptr->one_on_tz + 
		fraction * (spoint2_ptr->one_on_tz - spoint1_ptr->one_on_tz);
	clipped_spoint_ptr->u_on_tz = spoint1_ptr->u_on_tz + 
		fraction * (spoint2_ptr->u_on_tz - spoint1_ptr->u_on_tz);
	clipped_spoint_ptr->v_on_tz = spoint1_ptr->v_on_tz +
		fraction * (spoint2_ptr->v_on_tz - spoint1_ptr->v_on_tz);

	// Compute the colour intersection.

	clipped_spoint_ptr->colour.red = spoint1_ptr->colour.red + fraction * 
		(spoint2_ptr->colour.red - spoint1_ptr->colour.red);
	clipped_spoint_ptr->colour.green = spoint1_ptr->colour.green + fraction *
		(spoint2_ptr->colour.green - spoint1_ptr->colour.green);
	clipped_spoint_ptr->colour.blue = spoint1_ptr->colour.blue + fraction * 
		(spoint2_ptr->colour.blue - spoint1_ptr->colour.blue);
}

//------------------------------------------------------------------------------
// Clip a 2D line to a horizontal screen edge.
//------------------------------------------------------------------------------

static void
clip_2D_line_to_y(spoint *spoint1_ptr, spoint *spoint2_ptr, float sy,
				  spoint *clipped_spoint_ptr)
{
	float fraction;

	// Find the fraction of the line that is above the screen edge.
	
	fraction = (sy - spoint1_ptr->sy) / (spoint2_ptr->sy - spoint1_ptr->sy);

	// Compute the vertex intersection.

	clipped_spoint_ptr->sx = spoint1_ptr->sx +
		fraction * (spoint2_ptr->sx - spoint1_ptr->sx);
	clipped_spoint_ptr->sy = sy;

	// Compute the 1/tz, u/tz and v/tz intersections.

	clipped_spoint_ptr->one_on_tz = spoint1_ptr->one_on_tz + 
		fraction * (spoint2_ptr->one_on_tz - spoint1_ptr->one_on_tz);
	clipped_spoint_ptr->u_on_tz = spoint1_ptr->u_on_tz + 
		fraction * (spoint2_ptr->u_on_tz - spoint1_ptr->u_on_tz);
	clipped_spoint_ptr->v_on_tz = spoint1_ptr->v_on_tz +
		fraction * (spoint2_ptr->v_on_tz - spoint1_ptr->v_on_tz);

	// Compute the colour intersections.

	clipped_spoint_ptr->colour.red = spoint1_ptr->colour.red + fraction * 
		(spoint2_ptr->colour.red - spoint1_ptr->colour.red);
	clipped_spoint_ptr->colour.green = spoint1_ptr->colour.green + fraction *
		(spoint2_ptr->colour.green - spoint1_ptr->colour.green);
	clipped_spoint_ptr->colour.blue = spoint1_ptr->colour.blue + fraction * 
		(spoint2_ptr->colour.blue - spoint1_ptr->colour.blue);
}

//------------------------------------------------------------------------------
// Clip a projected polygon against the display screen.
//------------------------------------------------------------------------------

static void
clip_2D_polygon(void)
{
	int old_spoints, new_spoints;
	int spoint1_no, spoint2_no;
	float left_sx, right_sx, top_sy, bottom_sy;

	START_SUMMING;

	// Set up the clipping edges of the display.

	left_sx = 0.0;
	right_sx = (float)window_width;
	top_sy = 0.0;
	bottom_sy = (float)window_height;

	// Clip the projected polygon against the left display edge.

	old_spoints = spoints;
	new_spoints = 0;
	spoint1_no = old_spoints - 1;
	for (spoint2_no = 0; spoint2_no < old_spoints; spoint2_no++) {
		spoint *spoint1_ptr, *spoint2_ptr;

		// Get pointers to the screen points for this edge.

		spoint1_ptr = &main_spoint_list[spoint1_no];
		spoint2_ptr = &main_spoint_list[spoint2_no];

		// Compare the polygon edge against the left display edge and add zero,
		// one or two screen points to the temporary screen point list.

		if (spoint1_ptr->sx >= left_sx) {
			if (spoint2_ptr->sx >= left_sx)
				temp_spoint_list[new_spoints++] = *spoint2_ptr;
			else
				clip_2D_line_to_x(spoint1_ptr, spoint2_ptr, left_sx, 
					&temp_spoint_list[new_spoints++]);			
		} else {
			if (spoint2_ptr->sx >= left_sx) {
				clip_2D_line_to_x(spoint1_ptr, spoint2_ptr, left_sx,
					&temp_spoint_list[new_spoints++]);
				temp_spoint_list[new_spoints++] = *spoint2_ptr;
			}
		}
		
		// Move onto the next edge.
	
		spoint1_no = spoint2_no;
	}

	// Clip the projected polygon against the right display edge.

	old_spoints = new_spoints;
	new_spoints = 0;
	spoint1_no = old_spoints - 1;
	for (spoint2_no = 0; spoint2_no < old_spoints; spoint2_no++) {
		spoint *spoint1_ptr, *spoint2_ptr;

		// Get pointers to the screen points for this edge.

		spoint1_ptr = &temp_spoint_list[spoint1_no];
		spoint2_ptr = &temp_spoint_list[spoint2_no];

		// Compare the polygon edge against the right display edge and add zero,
		// one or two screen points to the main screen point list.

		if (spoint1_ptr->sx <= right_sx) {
			if (spoint2_ptr->sx <= right_sx)
				main_spoint_list[new_spoints++] = *spoint2_ptr;
			else
				clip_2D_line_to_x(spoint1_ptr, spoint2_ptr, right_sx, 
					&main_spoint_list[new_spoints++]);			
		} else {
			if (spoint2_ptr->sx <= right_sx) {
				clip_2D_line_to_x(spoint1_ptr, spoint2_ptr, right_sx,
					&main_spoint_list[new_spoints++]);
				main_spoint_list[new_spoints++] = *spoint2_ptr;
			}
		}
		
		// Move onto the next edge.
	
		spoint1_no = spoint2_no;
	}

	// Clip the projected polygon against the top screen edge.

	old_spoints = new_spoints;
	new_spoints = 0;
	spoint1_no = old_spoints - 1;
	for (spoint2_no = 0; spoint2_no < old_spoints; spoint2_no++) {
		spoint *spoint1_ptr, *spoint2_ptr;

		// Get pointers to the screen points for this edge.

		spoint1_ptr = &main_spoint_list[spoint1_no];
		spoint2_ptr = &main_spoint_list[spoint2_no];

		// Compare the polygon edge against the top display edge add zero, one or
		// two screen points to the temporary screen point list.

		if (spoint1_ptr->sy >= top_sy) {
			if (spoint2_ptr->sy >= top_sy)
				temp_spoint_list[new_spoints++] = *spoint2_ptr;
			else
				clip_2D_line_to_y(spoint1_ptr, spoint2_ptr, top_sy,
					&temp_spoint_list[new_spoints++]);
		} else {
			if (spoint2_ptr->sy >= top_sy) {
				clip_2D_line_to_y(spoint1_ptr, spoint2_ptr, top_sy,
					&temp_spoint_list[new_spoints++]);
				temp_spoint_list[new_spoints++] = *spoint2_ptr;
			}
		}
		
		// Move onto the next edge.
	
		spoint1_no = spoint2_no;
	}

	// Clip the projected polygon against the bottom screen edge.

	old_spoints = new_spoints;
	new_spoints = 0;
	spoint1_no = old_spoints - 1;
	for (spoint2_no = 0; spoint2_no < old_spoints; spoint2_no++) {
		spoint *spoint1_ptr, *spoint2_ptr;

		// Get pointers to the screen points for this edge.

		spoint1_ptr = &temp_spoint_list[spoint1_no];
		spoint2_ptr = &temp_spoint_list[spoint2_no];

		// Compare the polygon edge against the bottom display edge and add zero,
		// one or two screen points to the main screen point list.

		if (spoint1_ptr->sy <= bottom_sy) {
			if (spoint2_ptr->sy <= bottom_sy)
				main_spoint_list[new_spoints++] = *spoint2_ptr;
			else
				clip_2D_line_to_y(spoint1_ptr, spoint2_ptr, bottom_sy,
					&main_spoint_list[new_spoints++]);
		} else {
			if (spoint2_ptr->sy <= bottom_sy) {
				clip_2D_line_to_y(spoint1_ptr, spoint2_ptr, bottom_sy,
					&main_spoint_list[new_spoints++]);
				main_spoint_list[new_spoints++] = *spoint2_ptr;
			}
		}
		
		// Move onto the next edge.
	
		spoint1_no = spoint2_no;
	}

	// Set the final number of screen points.

	spoints = new_spoints;

	END_SUMMING(clip_2D_polygon_cycles);
}

//------------------------------------------------------------------------------
// Determine if the mouse is currently pointing at the given polygon.
//------------------------------------------------------------------------------

static bool
check_for_polygon_selection(void)
{
	int spoint1_no, spoint2_no;

	// Step through the screen point list, and determine whether the mouse
	// position is inside the polygon.

	spoint1_no = spoints - 1;
	for (spoint2_no = 0; spoint2_no < spoints; spoint2_no++) {
		spoint *spoint1_ptr, *spoint2_ptr;
		float relationship;
		
		// Get pointers to the screen points for this edge.

		spoint1_ptr = &main_spoint_list[spoint1_no];
		spoint2_ptr = &main_spoint_list[spoint2_no];

		// Compute the relationship between the mouse position and the edge.

		relationship = 
			(spoint2_ptr->sy - spoint1_ptr->sy) * (mouse_x - spoint1_ptr->sx) -
			(spoint2_ptr->sx - spoint1_ptr->sx) * (mouse_y - spoint1_ptr->sy);

		// If the mouse position is behind this edge, it is outside of the
		// polygon and hence it isn't selected.  If the polygon is double-sided 
		// and we're looking at the back face, the edge test must be reversed.

		if (front_face_visible) {
			if (FGE(relationship, 0.0f))
				return(false);
		} else {
			if (FLE(relationship, 0.0f))
				return(false);
		}

		// Move onto the next edge.
	
		spoint1_no = spoint2_no;
	}

	// The polygon was selected.

	return(true);
}

//------------------------------------------------------------------------------
// Select the next left edge by moving in an anti-clockwise direction through
// the spoint list.
//------------------------------------------------------------------------------

static void
next_left_edge(void)
{
	left_spoint1_ptr = left_spoint2_ptr;
	if (left_spoint2_ptr == first_spoint_ptr)
		left_spoint2_ptr = last_spoint_ptr;
	else
		left_spoint2_ptr--;
}

//------------------------------------------------------------------------------
// Select the next right edge by moving in a clockwise direction through the
// spoint list.
//------------------------------------------------------------------------------

static void
next_right_edge(void)
{
	right_spoint1_ptr = right_spoint2_ptr;
	if (right_spoint2_ptr == last_spoint_ptr)
		right_spoint2_ptr = first_spoint_ptr;
	else
		right_spoint2_ptr++;
}

//------------------------------------------------------------------------------
// Compute the slopes for the values to be interpolated down a polygon edge.
//------------------------------------------------------------------------------

static bool
compute_slopes(spoint *spoint1_ptr, spoint *spoint2_ptr, float scan_sy,
			   edge *edge_ptr, edge *slope_ptr)
{
	float delta_sy;

	// Determine the height of the edge, and if it's zero reject it.

	delta_sy = spoint2_ptr->sy - spoint1_ptr->sy;
	if (delta_sy == 0.0)
		return(false);

	// Compute the slopes of the interpolants.

	slope_ptr->sx = (spoint2_ptr->sx - spoint1_ptr->sx) / delta_sy;
	slope_ptr->one_on_tz = (spoint2_ptr->one_on_tz - spoint1_ptr->one_on_tz) /
		delta_sy;
	slope_ptr->u_on_tz = (spoint2_ptr->u_on_tz - spoint1_ptr->u_on_tz) /
		delta_sy;
	slope_ptr->v_on_tz = (spoint2_ptr->v_on_tz - spoint1_ptr->v_on_tz) /
		delta_sy;

	// Determine the initial values for the interpolants.  If the top screen Y
	// coordinate is above the Y coordinate of the scan line, we need to adjust
	// the initial interpolants so that they represent the values at the scan
	// line.

	edge_ptr->sx = spoint1_ptr->sx;
	edge_ptr->one_on_tz = spoint1_ptr->one_on_tz;
	edge_ptr->u_on_tz = spoint1_ptr->u_on_tz;
	edge_ptr->v_on_tz = spoint1_ptr->v_on_tz;
	delta_sy = scan_sy - spoint1_ptr->sy;
	if (delta_sy > 0.0) {
		edge_ptr->sx += slope_ptr->sx * delta_sy;
		edge_ptr->one_on_tz += slope_ptr->one_on_tz * delta_sy;
		edge_ptr->u_on_tz += slope_ptr->u_on_tz * delta_sy;
		edge_ptr->v_on_tz += slope_ptr->v_on_tz * delta_sy;
	}
	return(true);
}

//------------------------------------------------------------------------------
// Get the next left or right edge, and compute the slopes for the values to
// interpolate down that edge.  A boolean flag is returned to indicate if the
// bottom of the polygon was reached.
//------------------------------------------------------------------------------

static bool
prepare_next_left_edge(float scan_sy)
{
	// Step through left edges until we either reach the bottom of the polygon,
	// or find one that isn't above the requested scan line. 
	
	do {
		if (left_spoint2_ptr == bottom_spoint_ptr)
			return(false);
		next_left_edge();
	} while (left_spoint2_ptr->sy < scan_sy || 
		!compute_slopes(left_spoint1_ptr, left_spoint2_ptr, scan_sy,
			&left_edge, &left_slope));
	return(true);
}

static bool
prepare_next_right_edge(float scan_sy)
{
	// Step through right edges until we either reach the bottom of the polygon,
	// or find one that isn't above the requested scan line.

	do {
		if (right_spoint2_ptr == bottom_spoint_ptr)
			return(false);
		next_right_edge();
	} while (right_spoint2_ptr->sy < scan_sy || 
		!compute_slopes(right_spoint1_ptr, right_spoint2_ptr, scan_sy,
			&right_edge, &right_slope));
	return(true);
}

//------------------------------------------------------------------------------
// Render a polygon.
//------------------------------------------------------------------------------

static void
render_polygon(polygon *polygon_ptr, float turn_angle)
{
	spolygon *spolygon_ptr;
	vertex polygon_centroid;
	vector normal_vector;
	float brightness;
	int brightness_index;
	RGBcolour colour;
	pixel colour_pixel;
	vertex *furthest_tvertex_ptr;
	part *part_ptr;
	texture *texture_ptr;
	pixmap *pixmap_ptr;
	float sy, end_sy;

	START_SUMMING;

	// Increment the count of polygons processed late in the frame.

	polygons_processed_late_in_frame++;

	// Get a pointer to the part and texture.
	
	part_ptr = polygon_ptr->part_ptr;
	texture_ptr = part_ptr->texture_ptr;

	// Get a pointer to the next available screen polygon, and make it's screen
	// point list be the main screen point list.

	spolygon_ptr = get_next_screen_polygon();
	main_spoint_list = spolygon_ptr->spoint_list;

	// Get a pointer to the furthest transformed vertex.
	
	furthest_tvertex_ptr = get_furthest_vertex(polygon_ptr);

	// If the texture exists, get a pointer to the current pixmap.

	if (texture_ptr) {
		switch (curr_block_type) {
		case PLAYER_SPRITE:
		case MULTIFACETED_SPRITE:
			pixmap_ptr = 
				&texture_ptr->pixmap_list[curr_block_ptr->pixmap_index];
			break;
		default:
			if (texture_ptr->loops)
				pixmap_ptr = texture_ptr->get_curr_pixmap_ptr(curr_time_ms -
					start_time_ms);
			else
				pixmap_ptr = texture_ptr->get_curr_pixmap_ptr(curr_time_ms -
					curr_block_ptr->start_time_ms);
		}
	} else
		pixmap_ptr = NULL;

	// Rotate the polygon's normal vector by the turn angle, then reverse it if
	// the back of the polygon is visible.

	normal_vector = polygon_ptr->normal_vector;
	normal_vector.rotatey(turn_angle);
	if (!front_face_visible)
		normal_vector = -normal_vector;

	// If hardware acceleration is enabled...

	if (hardware_acceleration) {
		int vertex_no;
		RGBcolour *vertex_colour_ptr;
		vertex polygon_vertex, *vertex_ptr;

		START_SUMMING;

		// Compute the normalised lit colour for all polygon vertices, after
		// rotating them by the turn angle.  If the turn angle is zero, we skip
		// that step to save time.

		PREPARE_VERTEX_LIST(curr_block_ptr);
		PREPARE_VERTEX_DEF_LIST(polygon_ptr);
		if (FEQ(turn_angle, 0.0f)) {
			for (vertex_no = 0; vertex_no < polygon_ptr->vertices; vertex_no++) {
				vertex_ptr = VERTEX_PTR(vertex_no);
				vertex_colour_ptr = &vertex_colour_list[vertex_no];
				compute_vertex_colour(vertex_ptr, &normal_vector, 
					vertex_colour_ptr);
				vertex_colour_ptr->normalise();
			}
		} else {
			for (vertex_no = 0; vertex_no < polygon_ptr->vertices; vertex_no++) {
				polygon_vertex = *VERTEX_PTR(vertex_no);
				vertex_colour_ptr = &vertex_colour_list[vertex_no];
				if (curr_block_type != PLAYER_SPRITE)
					polygon_vertex -= block_centre;
				polygon_vertex.rotatey(turn_angle);
				polygon_vertex += block_centre;
				compute_vertex_colour(&polygon_vertex, &normal_vector,
					vertex_colour_ptr);
				vertex_colour_ptr->normalise();
			}
		}

		// If this polygon has no pixmap, blend it's colour with the vertex
		// colours.

		if (pixmap_ptr == NULL)
			for (vertex_no = 0; vertex_no < polygon_ptr->vertices; vertex_no++)
				vertex_colour_list[vertex_no].blend(part_ptr->normalised_colour);

		END_SUMMING(lighting_cycles);
	} 
	
	// If hardware acceleration is not enabled, compute the brightness at the
	// polygon centroid, and compute the colour pixel.

	else {
		polygon_centroid = polygon_ptr->centroid + block_translation;
		brightness = compute_vertex_brightness(&polygon_centroid,
			&normal_vector);
		brightness_index = get_brightness_index(brightness);
		colour = part_ptr->colour;
		colour.adjust_brightness(brightness);
		colour_pixel = RGB_to_display_pixel(colour);
	}
			
	// Clip the polygon against the viewing plane, creating a new polygon
	// whose screen points are in the main screen point list.
	
	clip_3D_polygon(polygon_ptr, pixmap_ptr);

	// Scale the texture interpolants in the main screen point list.

	scale_texture_interpolants(pixmap_ptr, part_ptr->texture_style);

	// Clip the projected polygon to the display screen.  If there are no screen
	// points left after this  process, the polygon was off-screen and does not
	// need to be rendered.

	clip_2D_polygon();
	if (spoints == 0) {
		END_SUMMING(render_polygon_cycles);
		return;
	}

	// Set the pixmap pointer, number of screen points, and alpha value in the 
	// screen polygon.

	spolygon_ptr->pixmap_ptr = pixmap_ptr;
	spolygon_ptr->spoints = spoints;
	spolygon_ptr->alpha = part_ptr->alpha;

	// Check whether this polygon is selected by the mouse, and if so remember
	// it as the currently selected square, popup and exit.  However, if the
	// polygon has a transparent texture or is translucent, and it doesn't have
	// an exit or any mouse-based triggers, then ignore it.
	// XXX -- movable blocks cannot be selected.

	if (curr_square_ptr && !found_selection && check_for_polygon_selection() &&
		(curr_square_ptr->exit_ptr || 
		 (curr_square_ptr->popup_trigger_flags & MOUSE_TRIGGERS) ||
		 (curr_square_ptr->block_trigger_flags & MOUSE_TRIGGERS) ||
		 (curr_square_ptr->square_trigger_flags & MOUSE_TRIGGERS) ||
		 (!hardware_acceleration || part_ptr->alpha == 1.0f) &&
		 (!texture_ptr || !texture_ptr->transparent))) {
		found_selection = true;
		curr_selected_square_ptr = curr_square_ptr;
		curr_popup_square_ptr = curr_square_ptr;
		curr_selected_exit_ptr = curr_square_ptr->exit_ptr;
		curr_selected_polygon_no = 
			(polygon_ptr - curr_block_ptr->polygon_list) + 1;
		curr_selected_block_def_ptr = curr_block_ptr->block_def_ptr;
	}

	// If using hardware acceleration, add the screen polygon to a list in
	// the pixmap if it has a texture, a special transparent list if it's
	// translucent or transparent, or a special colour list if it has no
	// texture.

	if (hardware_acceleration) {
		if (pixmap_ptr) {
			if (part_ptr->alpha < 1.0 || pixmap_ptr->transparent_index >= 0) {
				spolygon_ptr->next_spolygon_ptr2 = transparent_spolygon_list;
				transparent_spolygon_list = spolygon_ptr;
			} else {
				spolygon_ptr->next_spolygon_ptr2 = pixmap_ptr->spolygon_list;
				pixmap_ptr->spolygon_list = spolygon_ptr;
			}
		} else {
			spolygon_ptr->next_spolygon_ptr2 = colour_spolygon_list;
			colour_spolygon_list = spolygon_ptr;
		}
	}

	// If we're not using hardware acceleration, perform the polygon
	// rasterisation in software...

	else {
		spoint *left_spoint_ptr, *right_spoint_ptr;

		// Obtain pointers to the screen points representing the bounding box of
		// the polygon, as well as the last screen point in the list.

		get_polygon_bounding_box(first_spoint_ptr, last_spoint_ptr, 
			top_spoint_ptr, bottom_spoint_ptr, left_spoint_ptr, 
			right_spoint_ptr);

		// The ceiling of the top display y coordinate becomes the initial
		// display y coordinate, and the ceiling of the bottom screen y
		// coordinate becomes the last screen y coordinate + 1.  If these
		// are one and the same, then there is nothing to render.

		sy = CEIL(top_spoint_ptr->sy);
		end_sy = CEIL(bottom_spoint_ptr->sy);
		if (sy == end_sy) {
			END_SUMMING(render_polygon_cycles);
			return;
		}	

		// Prepare the first left and right edge for rendering.

		left_spoint2_ptr = top_spoint_ptr;
		prepare_next_left_edge(sy);
		right_spoint2_ptr = top_spoint_ptr;
		prepare_next_right_edge(sy);
		
		// Add the polygon to the span buffer row by row, recomputing the
		// slopes of the interpolants as as we pass the next left and right
		// vertex, until we've reached the bottom vertex or the bottom of
		// the display.

		while ((int)sy < window_height) {

			// Add this span to the span buffer.

			if (curr_block_movable)
				add_movable_span((int)sy, &left_edge, &right_edge, pixmap_ptr,
					colour_pixel, brightness_index);
			else 
				add_span((int)sy, &left_edge, &right_edge, pixmap_ptr, 
					colour_pixel, brightness_index, false);

			// Move to next row.

			sy += 1.0;
			if (sy < left_spoint2_ptr->sy) {
				left_edge.sx += left_slope.sx;
				left_edge.one_on_tz += left_slope.one_on_tz;
				left_edge.u_on_tz += left_slope.u_on_tz;
				left_edge.v_on_tz += left_slope.v_on_tz;
			} else if (!prepare_next_left_edge(sy))
				break;
			if (sy < right_spoint2_ptr->sy) {
				right_edge.sx += right_slope.sx;
				right_edge.one_on_tz += right_slope.one_on_tz;
				right_edge.u_on_tz += right_slope.u_on_tz;
				right_edge.v_on_tz += right_slope.v_on_tz;
			} else if (!prepare_next_right_edge(sy))
				break;
		}
	}

	// Increment the number of polygons rendered in this block.

	polygons_rendered_in_block++;

	END_SUMMING(render_polygon_cycles);
}

//------------------------------------------------------------------------------
// Render the player block, which is a sprite block.
//------------------------------------------------------------------------------

static void
render_player_block(void)
{
	polygon *polygon_ptr;
	part *part_ptr;
	texture *texture_ptr;
	float delta_x, delta_z;
	float distance;
	static float delta_distance = 0.0;

	// Rotate the player sprite by 180 degrees, tilt the it by the look angle,
	// and translate it by the current camera offset; the transformed vertices
	// are put into a global list.

	for (int vertex_no = 0; vertex_no < player_block_ptr->vertices;
		vertex_no++) {
		vertex temp_vertex;

		temp_vertex = player_block_ptr->vertex_list[vertex_no];
		temp_vertex.rotatey(180.0);
		temp_vertex.rotatex(-player_viewpoint.look_angle);
		temp_vertex -= player_camera_offset;
		block_tvertex_list[vertex_no] = temp_vertex;
	}

	// Locate the closest lights to the player position.

	find_closest_lights(&player_viewpoint.position);
	
	// Render the player sprite.

	block_translation = player_block_ptr->translation;
	polygon_ptr = player_block_ptr->polygon_list;
	front_face_visible = true;
	curr_block_movable = true;
	render_polygon(polygon_ptr, player_viewpoint.turn_angle - 180.0f);

	// If the player sprite has no texture or has only one pixmap, we're done.

	part_ptr = polygon_ptr->part_ptr;
	texture_ptr = part_ptr->texture_ptr;
	if (texture_ptr == NULL || texture_ptr->pixmaps == 1)
		return;
	
	// Increment or decrement the mipmap index if a distance of more than half
	// a block has been traversed.

	delta_x = player_viewpoint.position.x - player_viewpoint.last_position.x;
	delta_z = player_viewpoint.position.z - player_viewpoint.last_position.z;
	distance = (float)sqrt(delta_x * delta_x + delta_z * delta_z);
	if (forward_movement)
		delta_distance += distance;
	else
		delta_distance -= distance;
	if (FGE(delta_distance, 0.5f)) {
		player_block_ptr->pixmap_index = 
			(player_block_ptr->pixmap_index + 1) % texture_ptr->pixmaps;
		delta_distance = 0.0;
	} else if (FLT(delta_distance, -0.5f)) {
		player_block_ptr->pixmap_index = 
			(player_block_ptr->pixmap_index + texture_ptr->pixmaps - 1) % 
			texture_ptr->pixmaps;
		delta_distance = 0.0;
	}
}

//------------------------------------------------------------------------------
// Determine whether the camera is in front or behind a polygon by substituting
// the camera's relative position into the polygon's plane equation.
//------------------------------------------------------------------------------

static bool
camera_in_front(polygon *polygon_ptr)
{
	vector *normal_vector_ptr = &polygon_ptr->normal_vector;
	float relationship = normal_vector_ptr->dx * relative_camera_position.x +
		normal_vector_ptr->dy * relative_camera_position.y +
		normal_vector_ptr->dz * relative_camera_position.z + 
		polygon_ptr->plane_offset;
	return (FGE(relationship, 0.0f));
}

//------------------------------------------------------------------------------
// Render the polygons in a block by traversing the block's BSP tree.  The
// return value indicates if at least one polygon in the block was rendered.
//------------------------------------------------------------------------------

static void
render_polygons_in_block(BSP_node *BSP_node_ptr)
{
	int polygon_no;
	polygon *polygon_ptr;
	bool polygon_active;

	// Depending on whether the camera is in front or behind the polygon,
	// traverse the tree in front-node-back or back-node-front order.

	polygon_no = BSP_node_ptr->polygon_no;
	polygon_ptr = &curr_block_ptr->polygon_list[polygon_no];
	polygon_active = curr_block_ptr->polygon_active_list[polygon_no];
	if (camera_in_front(polygon_ptr)) {
		if (BSP_node_ptr->front_node_ptr)
			render_polygons_in_block(BSP_node_ptr->front_node_ptr);
		if (polygon_active && polygon_visible(polygon_ptr))
			render_polygon(polygon_ptr, 0.0f);
		if (BSP_node_ptr->rear_node_ptr)
			render_polygons_in_block(BSP_node_ptr->rear_node_ptr);
	} else {
		if (BSP_node_ptr->rear_node_ptr)
			render_polygons_in_block(BSP_node_ptr->rear_node_ptr);
		if (polygon_active && polygon_visible(polygon_ptr))
			render_polygon(polygon_ptr, 0.0f);
		if (BSP_node_ptr->front_node_ptr)
			render_polygons_in_block(BSP_node_ptr->front_node_ptr);
	}
}

//------------------------------------------------------------------------------
// Determine if given vertex is outside the frustum, and if it's in front of
// the near plane increment a counter.
//------------------------------------------------------------------------------

static bool
vertex_outside_frustum(float x, float y, float z, int &near_count)
{
	for (int plane_index = 0; plane_index < FRUSTUM_PLANES; plane_index++) {
		vector *vector_ptr = &frustum_normal_vector_list[plane_index];
		if (FGT(vector_ptr->dx * x + vector_ptr->dy * y + 
			vector_ptr->dz * z + frustum_plane_offset_list[plane_index], 
			0.0f)) {
			if (plane_index == FRUSTUM_NEAR_PLANE)
				near_count++;
			return(true);
		}
	}
	return(false);
}

//------------------------------------------------------------------------------
// Render a block on the given square (which may be NULL if the block is
// movable).
//------------------------------------------------------------------------------

static void
render_block(square *square_ptr, block *block_ptr, bool movable)
{
	block_def *block_def_ptr;
	vertex min_block, max_block;
	bool result;
	int count;

	START_SUMMING;

	// Clear the number of polygons rendered in this block.

	polygons_rendered_in_block = 0;

	// Remember the square and block pointers, and whether the block is movable.

	curr_square_ptr = square_ptr;
	curr_block_ptr = block_ptr;
	curr_block_movable = movable;

	// Get a pointer to the block definition and remember it's type.

	block_def_ptr = curr_block_ptr->block_def_ptr;
	curr_block_type = block_def_ptr->type;

	// Save the block translation in a global variable, and compute the
	// camera position relative to this.

	block_translation = curr_block_ptr->translation;
	relative_camera_position = camera_position;
	relative_camera_position -= block_translation;

	// Determine the centre of the block.

	block_centre.x = block_translation.x + world_ptr->half_block_units;
	block_centre.y = block_translation.y + world_ptr->half_block_units;
	block_centre.z = block_translation.z + world_ptr->half_block_units;

	// If fast clipping is enabled...

	if (fast_clipping) {

		// Calculate the minimum and maximum vertex coordinates for the block.

		min_block = block_translation;
		max_block.x = block_translation.x + world_ptr->block_units; 
		max_block.y = block_translation.y + world_ptr->block_units;
		max_block.z = block_translation.z + world_ptr->block_units;

		// If all of the corner vertices of the block are outside of the frustum,
		// and all or none of them are outside the near plane, then cull this
		// block.

		START_SUMMING;
		count = 0;
		result = 
			vertex_outside_frustum(min_block.x,min_block.y,min_block.z,count) &&
			vertex_outside_frustum(min_block.x,min_block.y,max_block.z,count) &&
			vertex_outside_frustum(min_block.x,max_block.y,min_block.z,count) &&
			vertex_outside_frustum(min_block.x,max_block.y,max_block.z,count) &&
			vertex_outside_frustum(max_block.x,min_block.y,min_block.z,count) &&
			vertex_outside_frustum(max_block.x,min_block.y,max_block.z,count) &&
			vertex_outside_frustum(max_block.x,max_block.y,min_block.z,count) &&
			vertex_outside_frustum(max_block.x,max_block.y,max_block.z,count);
		END_SUMMING(cull_block_cycles);
		if (result && (count == 0 || count == 8)) {
			END_SUMMING(render_block_cycles);
			return;
		}
	}

	// Locate the closest lights to the centre of the block. It would be more
	// accurate to do this for every polygon, but that increases processing
	// time.

	{
		START_SUMMING;
		find_closest_lights(&block_centre);
		END_SUMMING(find_light_cycles);
	}

	// If the block is a sprite...

	if (curr_block_type & SPRITE_BLOCK) {
		polygon *polygon_ptr;
		vector line_of_sight;
		float sprite_angle;
		
		// Get a pointer to the sprite polygon.

		polygon_ptr = curr_block_ptr->polygon_list;

		// Compute the angle that the sprite is facing based upon it's type.

		switch (curr_block_type) {
		case MULTIFACETED_SPRITE:
		case FACING_SPRITE:

			// The sprite angle is the player turn angle rotated by 180 degrees.

			sprite_angle = pos_adjust_angle(player_viewpoint.turn_angle +
				180.0f);
			break;
		
		case REVOLVING_SPRITE:

			// The sprite angle must be updated according to the elapsed time
			// and the rotational speed for a revolving sprite.  If delta angle
			// is greater than 45 degrees, clamp it so that the direction of
			// motion is obvious.

			if (block_def_ptr->degrees_per_ms > 0) {
				float delta_angle = clamp_angle((curr_time_ms - 
					curr_block_ptr->last_time_ms) * 
					block_def_ptr->degrees_per_ms, -45.0f, 45.0f);
				curr_block_ptr->sprite_angle = 
					pos_adjust_angle(curr_block_ptr->sprite_angle + delta_angle);
				curr_block_ptr->last_time_ms = curr_time_ms;
			}
			sprite_angle = (float)curr_block_ptr->sprite_angle;
			break;

		case ANGLED_SPRITE:

			// The sprite angle remains constant for an angled sprite.

			sprite_angle = curr_block_ptr->sprite_angle;
		}

		// Rotate each vertex around the block's centre by the sprite angle,
		// then transform each vertex by the player position and orientation,
		// storing them in a global list.

		{
			START_SUMMING;
			for (int vertex_no = 0; vertex_no < curr_block_ptr->vertices; 
				vertex_no++) {
				vertex temp_vertex = curr_block_ptr->vertex_list[vertex_no];
				temp_vertex -= block_centre;
				temp_vertex.rotatey(sprite_angle);
				temp_vertex += block_centre;
				transform_vertex(&temp_vertex, &block_tvertex_list[vertex_no]);
			}
			END_SUMMING(transform_vertex_cycles);
		}

		// If this is a multifaceted sprite and it has a texture, compute the
		// mipmap index based upon a line of sight from the player viewpoint
		// to the sprite centre.

		if (curr_block_type == MULTIFACETED_SPRITE) {
			part *part_ptr = polygon_ptr->part_ptr;
			texture *texture_ptr = part_ptr->texture_ptr;
			if (texture_ptr) {
				float angle, angle_range;

				// Generate a vector from the centre of the block to the 
				// player's position. 

				line_of_sight = player_viewpoint.position - block_centre; 
				line_of_sight.normalise();
			
				// Compute the angle of this vector in the X-Z plane.
				
				if (FEQ(line_of_sight.dx, 0.0f)) {
					if (FGT(line_of_sight.dz, 0.0f))
						angle = 0.0f;
					else
						angle = 180.0f;
				} else {
					angle = (float)DEG(atan(line_of_sight.dz / 
						line_of_sight.dx));
					if (FGT(line_of_sight.dx, 0.0f))
						angle = 90.0f - angle;
					else
						angle = 270.0f - angle;
				}

				// Calculate the pixmap index based upon this angle.

				angle_range = 360.0f / texture_ptr->pixmaps;
				curr_block_ptr->pixmap_index = (int)(angle / angle_range);
			}
		}

		// Render the sprite polygon if it is visible.

		if (polygon_visible(polygon_ptr, sprite_angle))
			render_polygon(polygon_ptr, sprite_angle);
	}
	
	// If the block is not a sprite...
	
	else {
		
		// Transform the vertices by the player's position, turn angle and look
		// angle, storing them in a global list.

		{
			START_SUMMING;
			for (int vertex_no = 0; vertex_no < curr_block_ptr->vertices; 
				vertex_no++) {
				vertex *vertex_ptr = &curr_block_ptr->vertex_list[vertex_no];
				transform_vertex(vertex_ptr, &block_tvertex_list[vertex_no]);
			}
			END_SUMMING(transform_vertex_cycles);
		}

		// If the block has a BSP tree, traverse it to render the polygons in 
		// front-to-back order.  Otherwise render the active polygons in order
		// of appearance (NOTE: if the block is not marked as movable, this will
		// result in gross sorting errors).

		if (block_def_ptr->BSP_tree)
			render_polygons_in_block(block_def_ptr->BSP_tree);
		else {
			int polygon_no;
			polygon *polygon_ptr;
			bool polygon_active;
			
			for (polygon_no = 0; polygon_no < curr_block_ptr->polygons;
				polygon_no++) {
				polygon_ptr = &curr_block_ptr->polygon_list[polygon_no];
				polygon_active = curr_block_ptr->polygon_active_list[polygon_no];
				if (polygon_active && polygon_visible(polygon_ptr))
					render_polygon(polygon_ptr, 0.0f);
			}
		}
	}

	END_SUMMING(render_block_cycles);

	// Update the stats for this block.

	blocks_processed_in_frame++;
	polygons_processed_in_frame += block_ptr->polygons;
	if (polygons_rendered_in_block > 0) {
		blocks_rendered_in_frame++;
		polygons_rendered_in_frame += polygons_rendered_in_block;
	}
}

//------------------------------------------------------------------------------
// Render the block on the given square.
//------------------------------------------------------------------------------

static void
render_block_on_square(int column, int row, int level)
{
	square *square_ptr;
	block *block_ptr;

	// Get a pointer to the square and it's block.  If the location is invalid
	// or there is no block on this square, there is nothing to render.

	if ((square_ptr = world_ptr->get_square_ptr(column, row, level)) == NULL ||
		(block_ptr = square_ptr->block_ptr) == NULL)
		return;

	// Now render the block, which is not movable.

	render_block(square_ptr, block_ptr, false);
}

//------------------------------------------------------------------------------
// Render the specified range of blocks on the map in an implicit BSP order.
//------------------------------------------------------------------------------

static void
render_blocks_on_map(int min_column, int min_row, int min_level,
					 int max_column, int max_row, int max_level)
{
	int column, row, level;

	// Get the map position the camera is in.

	camera_position.get_scaled_map_position(&camera_column, &camera_row,
		&camera_level);

	// Traverse the blocks in an implicit BSP order: first the columns to the
	// right and left of the camera, then the rows to the south and north of
	// the camera, then the levels above and below the camera.  The camera block
	// is rendered first.

	for (column = camera_column; column <= max_column; column++) {
		for (row = camera_row; row <= max_row; row++) {
			for (level = camera_level; level <= max_level; level++)
				render_block_on_square(column, row, level);
			for (level = camera_level - 1; level >= min_level; level--)
				render_block_on_square(column, row, level);
		}
		for (row = camera_row - 1; row >= min_row; row--) {
			for (level = camera_level; level <= max_level; level++)
				render_block_on_square(column, row, level);
			for (level = camera_level - 1; level >= min_level; level--)
				render_block_on_square(column, row, level);
		}
	}
	for (column = camera_column - 1; column >= min_column; column--) {
		for (row = camera_row; row <= max_row; row++) {
			for (level = camera_level; level <= max_level; level++)
				render_block_on_square(column, row, level);
			for (level = camera_level - 1; level >= min_level; level--)
				render_block_on_square(column, row, level);
		}
		for (row = camera_row - 1; row >= min_row; row--) {
			for (level = camera_level; level <= max_level; level++)
				render_block_on_square(column, row, level);
			for (level = camera_level - 1; level >= min_level; level--)
				render_block_on_square(column, row, level);
		}
	}
}

//------------------------------------------------------------------------------
// Compute the view bounding box.
//------------------------------------------------------------------------------

static void
compute_view_bounding_box(void)
{
	int index;
	vertex frustum_vertex;
	
	// Transform the frustum vertices from view space to world space.

	for (index = 0; index < FRUSTUM_VERTICES; index++) {
		frustum_vertex = frustum_vertex_list[index];
		frustum_vertex += player_camera_offset;
		frustum_vertex.rotatex(player_viewpoint.look_angle);
		frustum_vertex.rotatey(player_viewpoint.turn_angle);
		frustum_vertex += player_viewpoint.position;
		frustum_tvertex_list[index] = frustum_vertex;
	}

	// Initialise the minimum and maximum view coordinates to the first frustum
	// vertex.
 
	min_view = frustum_tvertex_list[0];
	max_view = frustum_tvertex_list[0];

	// Step through the remaining frustum vertices and remember the minimum and
	// maximum coordinates.

	for (index = 1; index < FRUSTUM_VERTICES; index++) {
		frustum_vertex = frustum_tvertex_list[index];
		if (frustum_vertex.x < min_view.x)
			min_view.x = frustum_vertex.x;
		else if (frustum_vertex.x > max_view.x)
			max_view.x = frustum_vertex.x;
		if (frustum_vertex.y < min_view.y)
			min_view.y = frustum_vertex.y;
		else if (frustum_vertex.y > max_view.y)
			max_view.y = frustum_vertex.y;
		if (frustum_vertex.z < min_view.z)
			min_view.z = frustum_vertex.z;
		else if (frustum_vertex.z > max_view.z)
			max_view.z = frustum_vertex.z;
	}
}

//------------------------------------------------------------------------------
// Compute the normal vector for a given set of corner vertices (given in
// clockwise order around the front face).
//------------------------------------------------------------------------------

static void
compute_frustum_normal_vector(int plane_index, int vertex1_index, 
							  int vertex2_index, int vertex3_index)
{
	vector vector1 = frustum_tvertex_list[vertex3_index] - 
		frustum_tvertex_list[vertex2_index];
	vector vector2 = frustum_tvertex_list[vertex1_index] - 
		frustum_tvertex_list[vertex2_index];
	vector vector3 = vector1 * vector2;
	vector3.normalise();
	frustum_normal_vector_list[plane_index] = vector3;
}

//------------------------------------------------------------------------------
// Compute the plane offset for a given frustum normal vector and corner vertex.
//------------------------------------------------------------------------------

static void
compute_frustum_plane_offset(int plane_index, int vertex_index)
{
	vector *vector_ptr = &frustum_normal_vector_list[plane_index];
	vertex *vertex_ptr = &frustum_tvertex_list[vertex_index];
	frustum_plane_offset_list[plane_index] = -(vector_ptr->dx * vertex_ptr->x + 
		vector_ptr->dy * vertex_ptr->y + vector_ptr->dz * vertex_ptr->z);
}

//------------------------------------------------------------------------------
// Compute the plane equations for the frustum.
//------------------------------------------------------------------------------

static void
compute_frustum_plane_equations(void)
{
	// Calculate the frustum normal vectors.

	compute_frustum_normal_vector(FRUSTUM_NEAR_PLANE, 3, 0, 1);
	compute_frustum_normal_vector(FRUSTUM_FAR_PLANE, 7, 6, 5);
	compute_frustum_normal_vector(FRUSTUM_LEFT_PLANE, 4, 0, 3);
	compute_frustum_normal_vector(FRUSTUM_RIGHT_PLANE, 5, 6, 2);
	compute_frustum_normal_vector(FRUSTUM_TOP_PLANE, 1, 0, 4);
	compute_frustum_normal_vector(FRUSTUM_BOTTOM_PLANE, 2, 6, 7);

	// Calculate the plane offsets.

	compute_frustum_plane_offset(FRUSTUM_NEAR_PLANE, 0);	
	compute_frustum_plane_offset(FRUSTUM_FAR_PLANE, 6);	
	compute_frustum_plane_offset(FRUSTUM_LEFT_PLANE, 0);	
	compute_frustum_plane_offset(FRUSTUM_RIGHT_PLANE, 6);	
	compute_frustum_plane_offset(FRUSTUM_TOP_PLANE, 0);	
	compute_frustum_plane_offset(FRUSTUM_BOTTOM_PLANE, 6);	
}

//------------------------------------------------------------------------------
// Render the blocks on the map that intersect with the view
//------------------------------------------------------------------------------

static void
render_map(void)
{
	int min_column, min_row, min_level;
	int max_column, max_row, max_level;

	START_TIMING;

	// Turn the coordinates into minimum and maximum map positions, then render
	// the blocks in this range.

	min_view.get_scaled_map_position(&min_column, &max_row, &min_level);
	max_view.get_scaled_map_position(&max_column, &min_row, &max_level);
	render_blocks_on_map(min_column, min_row, min_level, 
		max_column, max_row, max_level);

	END_TIMING("render_map");
}

//------------------------------------------------------------------------------
// Render all movable blocks that intersect the view bounding box.
//------------------------------------------------------------------------------

static void
render_movable_blocks(void)
{
	block *block_ptr;
	COL_MESH *col_mesh_ptr;
	vertex min_bbox, max_bbox;

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
				  min_bbox.z > max_view.z || max_bbox.z < min_view.z))
				render_block(NULL, block_ptr, true);
		}
		block_ptr = block_ptr->next_block_ptr;
	}
}

//------------------------------------------------------------------------------
// Render a popup texture.
//------------------------------------------------------------------------------

static void
render_popup_texture(popup *popup_ptr, pixmap *pixmap_ptr)
{
	// If hardware acceleration is enabled, render the pixmap directly on the
	// frame buffer.

	if (hardware_acceleration)
		draw_pixmap(pixmap_ptr, popup_ptr->brightness_index, 
			popup_ptr->sx, popup_ptr->sy);

	// If not using hardware acceleration, add the popup image to the span
	// buffer one row at a time.

	else {
		int width, height;
		int left_offset, top_offset;
		edge left_edge, right_edge;

		// If the screen x or y coordinates are negative, then we clamp 
		// them at zero and adjust the image offsets and size to match.

		if (popup_ptr->sx < 0) {
			left_offset = -popup_ptr->sx;
			width = pixmap_ptr->width - left_offset;
			popup_ptr->sx = 0;
		} else {
			left_offset = 0;
			width = pixmap_ptr->width;
		}	
		if (popup_ptr->sy < 0) {
			top_offset = -popup_ptr->sy;
			height = pixmap_ptr->height - top_offset;
			popup_ptr->sy = 0;
		} else {
			top_offset = 0;
			height = pixmap_ptr->height;
		}

		// If the popup overlaps the right or bottom edge, clip it.

		if (popup_ptr->sx + width > window_width)
			width = window_width - popup_ptr->sx;
		if (popup_ptr->sy + height > window_height)
			height = window_height - popup_ptr->sy;

		// Initialise the fields of the popup's left and right edge
		// that do not change.

		left_edge.sx = (float)popup_ptr->sx;
		left_edge.one_on_tz = 1.0;
		left_edge.u_on_tz = (float)left_offset;
		right_edge.sx = (float)(popup_ptr->sx + width);
		right_edge.one_on_tz = 1.0;
		right_edge.u_on_tz = (float)(left_offset + width);

		// Fill each span buffer row with a popup span.

		for (int row = 0; row < height; row++) {

			// Set up the left and right edge of the popup for this row,
			// then add the popup span to the span buffer.

			left_edge.v_on_tz = (float)(top_offset + row);
			right_edge.v_on_tz = (float)(top_offset + row);
			add_span(popup_ptr->sy + row, &left_edge, &right_edge, 
				pixmap_ptr, 0, popup_ptr->brightness_index, true);
		}
	}
}

//------------------------------------------------------------------------------
// Create the visible popup list.  If hardware acceleration is enabled, this
// list needs to be in back to front order; otherwise it needs to be in front
// to back order.
//------------------------------------------------------------------------------

static void
create_visible_popup_list(void)
{
	popup *popup_ptr;
	bool visible_by_proximity;
	texture *bg_texture_ptr, *fg_texture_ptr;
	int popup_width, popup_height;
	bool popup_displayable;

	// Initialise the visible popup list and the last visible popup pointer.

	visible_popup_list = NULL;
	last_visible_popup_ptr = NULL;

	// Step through each popup in the global popup list, determining whether
	// the player moved into into the trigger radius of those with a proximity
	// trigger, and adding those that are current visible by proximity or 
	// rollover to the visible popup list.

	popup_ptr = global_popup_list;
	while (popup_ptr) {

		// Determine the size of the popup.  If this is not possible because
		// the popup has neither a background nor foreground texture, skip it.

		bg_texture_ptr = popup_ptr->bg_texture_ptr;
		fg_texture_ptr = popup_ptr->fg_texture_ptr;
		if (bg_texture_ptr) {
			popup_width = bg_texture_ptr->width;
			popup_height = bg_texture_ptr->height;
		} else if (fg_texture_ptr) {
			popup_width = fg_texture_ptr->width;
			popup_height = fg_texture_ptr->height;
		} else {
			popup_ptr = popup_ptr->next_popup_ptr;
			continue;
		}

		// Determine whether or not this popup is displayable.

		if ((!bg_texture_ptr || bg_texture_ptr->pixmap_list) &&
			(!fg_texture_ptr || fg_texture_ptr->pixmap_list))
			popup_displayable = true;
		else
			popup_displayable = false;

		// If this popup is displayable, check whether it's visible by 
		// proximity.

		visible_by_proximity = false;
		if (popup_displayable) {

			// If this popup is always visible, treat it as currently visible
			// by proximity.

			if (popup_ptr->always_visible ||
				(popup_ptr->trigger_flags & EVERYWHERE) != 0)
				visible_by_proximity = true;

			// Otherwise, if this popup has the proximity trigger set, check
			// whether the player viewpoint is within the radius of the popup.

			else if ((popup_ptr->trigger_flags & PROXIMITY) != 0) {
				vector distance;
				float distance_squared;

				// Determine the distance squared from the popup's position to
				// the player's position.  

				distance = popup_ptr->position - player_viewpoint.position;
				distance_squared = distance.dx * distance.dx + 
					distance.dy * distance.dy + distance.dz * distance.dz;

				// If the player is on the same level as the popup, and the
				// distance squared is within the trigger radius squared, then
				// make the popup visible by proximity.

				if (player_viewpoint.position.get_scaled_map_level() ==
					popup_ptr->position.get_scaled_map_level() &&
					distance_squared <= popup_ptr->radius_squared) {
					visible_by_proximity = true;
				}
			}
		}

		// If the popup was not previously visible and has become visible by
		// proximity, set the time that this popup was made visible, and if the
		// window alignment is by mouse then set the position of the popup.

		if (popup_ptr->visible_flags == 0 && visible_by_proximity) {
			popup_ptr->start_time_ms = curr_time_ms;
			if (popup_ptr->window_alignment == MOUSE) {
				popup_ptr->sx = mouse_x - popup_width / 2;
				popup_ptr->sy = mouse_y - popup_height / 2;
			}
		}

		// Set or reset the popup's visible by proximity flag.

		if (visible_by_proximity)
			popup_ptr->visible_flags |= PROXIMITY;
		else
			popup_ptr->visible_flags &= ~PROXIMITY;

		// If the popup is visible by proximity or rollover, and it is
		// displayable, then add it to the visible popup list...

		if (popup_ptr->visible_flags != 0 && popup_displayable) {

			// Set the pointer to the current background pixmap if there is a
			// background texture.  Non-looping textures begin animating from
			// the time the popup becomes visible.

			if (bg_texture_ptr && bg_texture_ptr->pixmap_list) {
				if (bg_texture_ptr->loops)
					popup_ptr->bg_pixmap_ptr = 
						bg_texture_ptr->get_curr_pixmap_ptr(curr_time_ms - 
						start_time_ms);
				else
					popup_ptr->bg_pixmap_ptr = 
						bg_texture_ptr->get_curr_pixmap_ptr(curr_time_ms - 
						popup_ptr->start_time_ms);
			}

			// Compute the screen x and y coordinates of the popup according
			// to the window alignment mode and the size of the popup.
			// These coordinates may be negative.
			
			switch (popup_ptr->window_alignment) {
			case TOP_LEFT:
			case LEFT:
			case BOTTOM_LEFT:
				popup_ptr->sx = 0;
				break;
			case TOP:
			case CENTRE:
			case BOTTOM:
				popup_ptr->sx = (window_width - popup_width) / 2;
				break;
			case TOP_RIGHT:
			case RIGHT:
			case BOTTOM_RIGHT:
				popup_ptr->sx = window_width - popup_width;
			}
			switch (popup_ptr->window_alignment) {
			case TOP_LEFT:
			case TOP:
			case TOP_RIGHT:
				popup_ptr->sy = 0;
				break;
			case LEFT:
			case CENTRE:
			case RIGHT:
				popup_ptr->sy = (window_height - popup_height) / 2;
				break;
			case BOTTOM_LEFT:
			case BOTTOM:
			case BOTTOM_RIGHT:
				popup_ptr->sy = window_height - popup_height;
			}
			
			// If a mouse selection hasn't been found yet, check whether
			// this popup has been selected.  If the popup has no imagemap
			// and is transparent, ignore it.

			if (!found_selection && 
				(popup_ptr->imagemap_ptr || 
				 (popup_ptr->bg_texture_ptr && 
				 !popup_ptr->bg_texture_ptr->transparent) ||
				 !popup_ptr->transparent_background) &&
				check_for_popup_selection(popup_ptr, popup_width, 
				popup_height)) {
				found_selection = true;
				curr_popup_ptr = popup_ptr;
			}

			// If hardware acceleration is enabled, add this popup to the tail
			// of the popup list, otherwise add it to the head.

			if (hardware_acceleration) {
				if (last_visible_popup_ptr)
					last_visible_popup_ptr->next_visible_popup_ptr = popup_ptr;
				else
					visible_popup_list = popup_ptr;
				popup_ptr->next_visible_popup_ptr = NULL;
				last_visible_popup_ptr = popup_ptr;
			} else {
				popup_ptr->next_visible_popup_ptr = visible_popup_list;
				visible_popup_list = popup_ptr;
			}
		}

		// Move onto the next popup in the list.

		popup_ptr = popup_ptr->next_popup_ptr;
	}
}

//------------------------------------------------------------------------------
// Render the orb.
//------------------------------------------------------------------------------

static void
render_orb(void)
{
	pixmap *pixmap_ptr;
	direction orb_direction;
	float one_on_dimensions;
	float width_scale, height_scale;
	float left_offset, top_offset;
	edge left_edge, right_edge;
	RGBcolour dummy_colour;

	// If there is no orb texture loaded, then there is nothing to render.

	if (orb_texture_ptr == NULL)
		return;

	START_TIMING;

	// Get a pointer to the orb pixmap.

	pixmap_ptr = orb_texture_ptr->get_curr_pixmap_ptr(curr_time_ms - 
		start_time_ms);

	// Determine the position and size of the orb on the screen.

	orb_direction.angle_x = orb_light_ptr->curr_dir.angle_x;
	orb_direction.angle_y = orb_light_ptr->curr_dir.angle_y - 180.0f;
	curr_orb_x = half_window_width + neg_adjust_angle(orb_direction.angle_y - 
		player_viewpoint.turn_angle) * horz_pixels_per_degree - half_orb_width;
	curr_orb_y = half_window_height + neg_adjust_angle(orb_direction.angle_x -
		player_viewpoint.look_angle) * vert_pixels_per_degree - half_orb_height;
	width_scale = (float)pixmap_ptr->width / orb_width;
	height_scale = (float)pixmap_ptr->height / orb_height;

	// If the orb is complete outside of the window, don't render it.

	if (FLT(curr_orb_x + orb_width, 0.0f) || FGE(curr_orb_x, window_width) ||
		FLT(curr_orb_y + orb_height, 0.0f) || FGE(curr_orb_y, window_height)) {
		END_TIMING("render_orb");
		return;
	}

	// If the screen x or y coordinates are negative, then we clamp 
	// them at zero and adjust the image offsets and size to match.

	if (curr_orb_x < 0.0f) {
		left_offset = -curr_orb_x;
		curr_orb_width = orb_width - left_offset;
		curr_orb_x = 0.0f;
	} else {
		left_offset = 0.0f;
		curr_orb_width = orb_width;
	}
	if (curr_orb_y < 0.0f) {
		top_offset = -curr_orb_y;
		curr_orb_height = orb_height - top_offset;
		curr_orb_y = 0.0f;
	} else {
		top_offset = 0.0f;
		curr_orb_height = orb_height;
	}

	// If the texture overlaps the right or bottom edge, clip it.

	if (curr_orb_x + curr_orb_width > (float)window_width)
		curr_orb_width = (float)window_width - curr_orb_x;
	if (curr_orb_y + curr_orb_height > (float)window_height)
		curr_orb_height = (float)window_height - curr_orb_y;

	// If using hardware acceleration, render the orb as a 2D polygon.

	if (hardware_acceleration) {
		one_on_dimensions = one_on_dimensions_list[pixmap_ptr->size_index];
		width_scale = (float)pixmap_ptr->width * one_on_dimensions;
		height_scale = (float)pixmap_ptr->height * one_on_dimensions;
		hardware_render_2D_polygon(pixmap_ptr, dummy_colour, orb_brightness,
			curr_orb_x, curr_orb_y, curr_orb_width, curr_orb_height,
			left_offset / orb_width * width_scale, 
			top_offset / orb_height * height_scale, 
			(left_offset + curr_orb_width) / orb_width * width_scale, 
			(top_offset + curr_orb_height) / orb_height * height_scale, false);
	}

	// If not using hardware acceleration, render the orb as a 2D scaled
	// texture in software...

	else {

		// Initialise the fields of the left and right edge that do not change.

		left_edge.sx = curr_orb_x;
		left_edge.one_on_tz = 1.0f;
		left_edge.u_on_tz = left_offset * width_scale;
		right_edge.sx = curr_orb_x + curr_orb_width;
		right_edge.one_on_tz = 1.0f;
		right_edge.u_on_tz = (left_offset + curr_orb_width) * width_scale;

		// Fill each span buffer row with an image span.

		for (int row = 0; row < curr_orb_height; row++) {
			left_edge.v_on_tz = (top_offset + row) * height_scale;
			right_edge.v_on_tz = (top_offset + row) * height_scale;
			add_span((int)(curr_orb_y + row), &left_edge, &right_edge, 
				pixmap_ptr, 0, orb_brightness_index, false);
		}
	}

	END_TIMING("render_orb");
}

//------------------------------------------------------------------------------
// Render the screen polygons that use the given texture, using hardware
// acceleration.
//------------------------------------------------------------------------------

static void
render_textured_polygons(texture *texture_ptr)
{
	int pixmap_no;
	pixmap *pixmap_ptr;
	spolygon *spolygon_ptr;

	// Step through each pixmap in this texture...

	for (pixmap_no = 0; pixmap_no < texture_ptr->pixmaps; pixmap_no++) {
		pixmap_ptr = &texture_ptr->pixmap_list[pixmap_no];

		// Render all the screen polygons in the pixmap via hardware.

		spolygon_ptr = pixmap_ptr->spolygon_list;
		while (spolygon_ptr) {
			hardware_render_polygon(spolygon_ptr);
			spolygon_ptr = spolygon_ptr->next_spolygon_ptr2;
		}
		pixmap_ptr->spolygon_list = NULL;
	}
}

//------------------------------------------------------------------------------
// Render the spans that use the given texture to a 16-bit frame buffer.
//------------------------------------------------------------------------------

static void
render_textured_spans16(texture *texture_ptr)
{
	int pixmap_no;
	pixmap *pixmap_ptr;
	int index;
	span *span_ptr;

	// Step through each pixmap in this texture...

	for (pixmap_no = 0; pixmap_no < texture_ptr->pixmaps; pixmap_no++) {
		pixmap_ptr = &texture_ptr->pixmap_list[pixmap_no];

		// Step through each span list in this pixmap, and render them as 
		// linear or opaque depending on whether it belongs to a popup or
		// not.

		for (index = 0; index < BRIGHTNESS_LEVELS; index++) {
			span_ptr = pixmap_ptr->span_lists[index];
			while (span_ptr) {
				if (span_ptr->is_popup)
					render_popup_span16(span_ptr);
				else
					render_opaque_span16(span_ptr);
				span_ptr = del_span(span_ptr);
			}
			pixmap_ptr->span_lists[index] = NULL;
		}
	}
}

//------------------------------------------------------------------------------
// Render the spans that use the given texture to a 24-bit frame buffer.
//------------------------------------------------------------------------------

static void
render_textured_spans24(texture *texture_ptr)
{
	int pixmap_no;
	pixmap *pixmap_ptr;
	int index;
	span *span_ptr;

	// Step through each pixmap in this texture...

	for (pixmap_no = 0; pixmap_no < texture_ptr->pixmaps; pixmap_no++) {
		pixmap_ptr = &texture_ptr->pixmap_list[pixmap_no];

		// Step through each span list in this pixmap, and render them as 
		// linear or opaque depending on whether it belongs to a popup or
		// not.

		for (index = 0; index < BRIGHTNESS_LEVELS; index++) {
			span_ptr = pixmap_ptr->span_lists[index];
			while (span_ptr) {
				if (span_ptr->is_popup)
					render_popup_span24(span_ptr);
				else
					render_opaque_span24(span_ptr);
				span_ptr = del_span(span_ptr);
			}
			pixmap_ptr->span_lists[index] = NULL;
		}
	}
}

//------------------------------------------------------------------------------
// Render the spans that use the given texture to a 32-bit frame buffer.
//------------------------------------------------------------------------------

static void
render_textured_spans32(texture *texture_ptr)
{
	int pixmap_no;
	pixmap *pixmap_ptr;
	int index;
	span *span_ptr;

	// Step through each pixmap in this texture...

	for (pixmap_no = 0; pixmap_no < texture_ptr->pixmaps; pixmap_no++) {
		pixmap_ptr = &texture_ptr->pixmap_list[pixmap_no];

		// Step through each span list in this pixmap, and render them as 
		// linear or opaque depending on whether it belongs to a popup or
		// not.

		for (index = 0; index < BRIGHTNESS_LEVELS; index++) {
			span_ptr = pixmap_ptr->span_lists[index];
			while (span_ptr) {
				if (span_ptr->is_popup)
					render_popup_span32(span_ptr);
				else
					render_opaque_span32(span_ptr);
				span_ptr = del_span(span_ptr);
			}
			pixmap_ptr->span_lists[index] = NULL;
		}
	}
}

//------------------------------------------------------------------------------
// Render the screen polygons or spans for this frame.
//------------------------------------------------------------------------------

static void
render_textured_polygons_or_spans(void)
{
	blockset *blockset_ptr;
	texture *texture_ptr;
	video_texture *scaled_video_texture_ptr;

	START_TIMING;

	// Handle hardware accelerated rendering...

	if (hardware_acceleration) {
		blockset_ptr = blockset_list_ptr->first_blockset_ptr;
		while (blockset_ptr) {
			texture_ptr = blockset_ptr->first_texture_ptr;
			while (texture_ptr) {
				render_textured_polygons(texture_ptr);
				texture_ptr = texture_ptr->next_texture_ptr;
			}
			blockset_ptr = blockset_ptr->next_blockset_ptr;
		}
		texture_ptr = custom_blockset_ptr->first_texture_ptr;
		while (texture_ptr) {
			render_textured_polygons(texture_ptr);
			texture_ptr = texture_ptr->next_texture_ptr;
		}
		if (unscaled_video_texture_ptr)
			render_textured_polygons(unscaled_video_texture_ptr);
		scaled_video_texture_ptr = scaled_video_texture_list;
		while (scaled_video_texture_ptr) {
			render_textured_polygons(scaled_video_texture_ptr->texture_ptr);
			scaled_video_texture_ptr = 
				scaled_video_texture_ptr->next_video_texture_ptr;
		}
	} 
	
	// Handle software rendering to a 16-bit frame buffer...

	else if (display_depth <= 16) {
		blockset_ptr = blockset_list_ptr->first_blockset_ptr;
		while (blockset_ptr) {
			texture_ptr = blockset_ptr->first_texture_ptr;
			while (texture_ptr) {
				render_textured_spans16(texture_ptr);
				texture_ptr = texture_ptr->next_texture_ptr;
			}
			blockset_ptr = blockset_ptr->next_blockset_ptr;
		}
		texture_ptr = custom_blockset_ptr->first_texture_ptr;
		while (texture_ptr) {
			render_textured_spans16(texture_ptr);
			texture_ptr = texture_ptr->next_texture_ptr;
		}
		if (unscaled_video_texture_ptr)
			render_textured_spans16(unscaled_video_texture_ptr);
		scaled_video_texture_ptr = scaled_video_texture_list;
		while (scaled_video_texture_ptr) {
			render_textured_spans16(scaled_video_texture_ptr->texture_ptr);
			scaled_video_texture_ptr = 
				scaled_video_texture_ptr->next_video_texture_ptr;
		}
	} 
	
	// Handle software rendering to a 24-bit frame buffer...

	else if (display_depth == 24) {
		blockset_ptr = blockset_list_ptr->first_blockset_ptr;
		while (blockset_ptr) {
			texture_ptr = blockset_ptr->first_texture_ptr;
			while (texture_ptr) {
				render_textured_spans24(texture_ptr);
				texture_ptr = texture_ptr->next_texture_ptr;
			}
			blockset_ptr = blockset_ptr->next_blockset_ptr;
		}
		texture_ptr = custom_blockset_ptr->first_texture_ptr;
		while (texture_ptr) {
			render_textured_spans24(texture_ptr);
			texture_ptr = texture_ptr->next_texture_ptr;
		}
		if (unscaled_video_texture_ptr)
			render_textured_spans24(unscaled_video_texture_ptr);
		scaled_video_texture_ptr = scaled_video_texture_list;
		while (scaled_video_texture_ptr) {
			render_textured_spans24(scaled_video_texture_ptr->texture_ptr);
			scaled_video_texture_ptr = 
				scaled_video_texture_ptr->next_video_texture_ptr;
		}
	}  
	
	// Handle software rendering to a 32-bit frame buffer...

	else {
		blockset_ptr = blockset_list_ptr->first_blockset_ptr;
		while (blockset_ptr) {
			texture_ptr = blockset_ptr->first_texture_ptr;
			while (texture_ptr) {
				render_textured_spans32(texture_ptr);
				texture_ptr = texture_ptr->next_texture_ptr;
			}
			blockset_ptr = blockset_ptr->next_blockset_ptr;
		}
		texture_ptr = custom_blockset_ptr->first_texture_ptr;
		while (texture_ptr) {
			render_textured_spans32(texture_ptr);
			texture_ptr = texture_ptr->next_texture_ptr;
		}
		if (unscaled_video_texture_ptr)
			render_textured_spans32(unscaled_video_texture_ptr);
		scaled_video_texture_ptr = scaled_video_texture_list;
		while (scaled_video_texture_ptr) {
			render_textured_spans32(scaled_video_texture_ptr->texture_ptr);
			scaled_video_texture_ptr = 
				scaled_video_texture_ptr->next_video_texture_ptr;
		}
	}

	END_TIMING("render_textured_polygons_or_spans");
}

//------------------------------------------------------------------------------
// Render the screen polygons/spans for this frame that have a solid colour.
//------------------------------------------------------------------------------

static void
render_colour_polygons_or_spans(void)
{
	START_TIMING;

	// If using hardware acceleration, render the screen polygons in the solid
	// colour screen polygon list via hardware.

	if (hardware_acceleration) {
		spolygon *spolygon_ptr = colour_spolygon_list;
		while (spolygon_ptr) {
			hardware_render_polygon(spolygon_ptr);
			spolygon_ptr = spolygon_ptr->next_spolygon_ptr2;
		}
	}

	// If not using OpenGL or hardware acceleration, render the solid colour
	// spans in software.

	else if (display_depth <= 16) {
		span *span_ptr = colour_span_list;
		while (span_ptr) {
			render_colour_span16(span_ptr);
			span_ptr = del_span(span_ptr);
		}
	} else if (display_depth == 24) {
		span *span_ptr = colour_span_list;
		while (span_ptr) {
			render_colour_span24(span_ptr);
			span_ptr = del_span(span_ptr);
		}
	} else {
		span *span_ptr = colour_span_list;
		while (span_ptr) {
			render_colour_span32(span_ptr);
			span_ptr = del_span(span_ptr);
		}
	}

	END_TIMING("render_colour_polygons_or_spans");
}

//------------------------------------------------------------------------------
// Render the screen polygons/spans for this frame that are transparent.
//------------------------------------------------------------------------------

static void
render_transparent_polygons_or_spans(void)
{
	spolygon *spolygon_ptr;
	int row;
	span_row *span_row_ptr;
	span *span_ptr;

	START_TIMING;

	// If using hardware acceleration, render the screen polygons in the
	// transparent screen polygon list in back to front order, via hardware.

	if (hardware_acceleration) {
		spolygon_ptr = transparent_spolygon_list;
		while (spolygon_ptr) {
			hardware_render_polygon(spolygon_ptr);
			spolygon_ptr = spolygon_ptr->next_spolygon_ptr2;
		}
	}

	// If not using hardware acceleration, render the transparent spans from
	// each span buffer row in back to front order, removing them as we go.
	// Spans that have a texture of unlimited size are only used by popups, and
	// are rendered as linear texture-mapped spans.

	else if (display_depth <= 16) {
		for (row = 0; row < window_height; row++) {
			span_row_ptr = (*span_buffer_ptr)[row];
			span_ptr = span_row_ptr->transparent_span_list;
			while (span_ptr) {
				if (span_ptr->is_popup)
					render_popup_span16(span_ptr);
				else
					render_transparent_span16(span_ptr);
				span_ptr = del_span(span_ptr);
			}
		}
	} else if (display_depth == 24) {
		for (row = 0; row < window_height; row++) {
			span_row_ptr = (*span_buffer_ptr)[row];
			span_ptr = span_row_ptr->transparent_span_list;
			while (span_ptr) {
				if (span_ptr->is_popup)
					render_popup_span24(span_ptr);
				else
					render_transparent_span24(span_ptr);
				span_ptr = del_span(span_ptr);
			}
		}
	} else {
		for (row = 0; row < window_height; row++) {
			span_row_ptr = (*span_buffer_ptr)[row];
			span_ptr = span_row_ptr->transparent_span_list;
			while (span_ptr) {
				if (span_ptr->is_popup)
					render_popup_span32(span_ptr);
				else
					render_transparent_span32(span_ptr);
				span_ptr = del_span(span_ptr);
			}
		}
	}

	END_TIMING("render_transparent_polygons_or_spans");
}

//------------------------------------------------------------------------------
// Render the entire frame.
//------------------------------------------------------------------------------

void
render_frame(void)
{
	square *prev_popup_square_ptr;
	pixmap *sky_pixmap_ptr;
	float sky_start_u, sky_start_v, sky_end_u, sky_end_v;
	texture *texture_ptr;
	pixmap *pixmap_ptr;
	int row;
	texture *bg_texture_ptr, *fg_texture_ptr;
	int popup_width, popup_height;

	// If AMD 3DNow! instructions are supported, calculate the current
	// transformation matrix.

	if (AMD_3DNow_supported) {
//		float temp_matrix1[16], temp_matrix2[16];

		// Set the translation and rotation matrices for the current player
		// viewpoint.

		set_translate_matrix(curr_translate_matrix);
		set_rotatey_matrix(curr_rotatey_matrix);
		set_rotatex_matrix(curr_rotatex_matrix);
		set_camera_matrix(curr_camera_matrix);

		// Multiply these together to form the transformation matrix.
		// XXX -- For some reason this doesn't generate the correct
		// transformation matrix.

//		_mul_m4x4(temp_matrix1, curr_translate_matrix, curr_rotatey_matrix);
//		_mul_m4x4(temp_matrix2, temp_matrix1, curr_rotatex_matrix);
//		_mul_m4x4(curr_transform_matrix, temp_matrix2, curr_camera_matrix);
	}

	// Reset counters.

	polygons_rendered_in_frame = 0;
	polygons_processed_in_frame = 0;
	polygons_processed_late_in_frame = 0;
	blocks_rendered_in_frame = 0;
	blocks_processed_in_frame = 0;
	cache_entries_added_in_frame = 0;
	cache_entries_reused_in_frame = 0;
	cache_entries_free_in_frame = 0;

	CLEAR_SUM(render_block_cycles);
	CLEAR_SUM(cull_block_cycles);
	CLEAR_SUM(find_light_cycles);
	CLEAR_SUM(render_polygon_cycles);
	CLEAR_SUM(clip_3D_polygon_cycles);
	CLEAR_SUM(clip_2D_polygon_cycles);
	CLEAR_SUM(lighting_cycles);
	CLEAR_SUM(scale_texture_cycles);
	CLEAR_SUM(transform_vertex_cycles);
	START_TIMING;

	// Lock the frame buffer.

	lock_frame_buffer(frame_buffer_ptr, frame_buffer_width);

	// Reset the span and screen polygon lists.

	colour_span_list = NULL;
	transparent_spolygon_list = NULL;
	colour_spolygon_list = NULL;

	// Reset the screen polygon list.

	reset_screen_polygon_list();

	// Reset the number of the currently selected polygon.

	curr_selected_polygon_no = 0;

	// Remember the previous square selected, then reset the current square
	// selected.

	prev_selected_square_ptr = curr_selected_square_ptr;
	curr_selected_square_ptr = NULL;

	// Remember the previous exit selected, and reset the current exit
	// selected.

	prev_selected_exit_ptr = curr_selected_exit_ptr;
	curr_selected_exit_ptr = NULL;

	// Remember the previous area selected and it's square, and reset the
	// current area selected and it's square.

	prev_selected_area_ptr = curr_selected_area_ptr;
	prev_area_square_ptr = curr_area_square_ptr;
	curr_selected_area_ptr = NULL;
	curr_area_square_ptr = NULL;

	// Remember the previous popup square, and reset the current popup square.

	prev_popup_square_ptr = curr_popup_square_ptr;
	curr_popup_square_ptr = NULL;

	// Reset the current popup square and popup.

	found_selection = false;
	curr_popup_square_ptr = NULL;
	curr_popup_ptr = NULL;

	// Create the visible popup list.

	create_visible_popup_list();

	// Get the sky pixmap, if there is one.

	if (sky_texture_ptr)
		sky_pixmap_ptr = sky_texture_ptr->get_curr_pixmap_ptr(curr_time_ms - 
			start_time_ms);
	else
		sky_pixmap_ptr = NULL;

	// Compute the top-left and bottom-right texture coordinates for the sky,
	// such that a single sky texture map occupies a 15x15 degree area.

	sky_start_u = (float)((int)player_viewpoint.turn_angle % 15) / 15.0f;
	sky_start_v = (float)((int)player_viewpoint.look_angle % 15) / 15.0f;
	sky_end_u = sky_start_u + horz_field_of_view / 15.0f;
	sky_end_v = sky_start_v + vert_field_of_view / 15.0f;
		
	// Compute the view vector in world space.

	view_vector.dx = 0.0;
	view_vector.dy = 0.0;
	view_vector.dz = 1.0;
	view_vector.rotatex(player_viewpoint.look_angle);
	view_vector.rotatey(player_viewpoint.turn_angle);

	// Compute the position of the camera.

	camera_position.x = player_camera_offset.dx;
	camera_position.y = player_camera_offset.dy;
	camera_position.z = player_camera_offset.dz;
	camera_position.rotatex(player_viewpoint.look_angle);
	camera_position.rotatey(player_viewpoint.turn_angle);
	camera_position += player_viewpoint.position;

	// If using hardware acceleration...
	
	if (hardware_acceleration) {

		// Render the sky polygon.

		hardware_render_2D_polygon(sky_pixmap_ptr, sky_colour, sky_brightness,
			0.0f, 0.0f, (float)window_width, (float)window_height,
			sky_start_u, sky_start_v, sky_end_u, sky_end_v, true);

		// Render the orb polygon.

		render_orb();
	}

	// If not using hardware acceleration...
	
	else {

		// Clear the span buffer.

		for (row = 0; row < window_height; row++) {
			span_row *span_row_ptr = (*span_buffer_ptr)[row];
			span_row_ptr->opaque_span_list = NULL;
			span_row_ptr->transparent_span_list = NULL;
		}

		// Render the visible popups in front to back order.

		popup *popup_ptr = visible_popup_list;
		while (popup_ptr) {
			texture_ptr = popup_ptr->fg_texture_ptr;
			if (texture_ptr)
				render_popup_texture(popup_ptr, &texture_ptr->pixmap_list[0]);
			texture_ptr = popup_ptr->bg_texture_ptr;
			if (texture_ptr)
				render_popup_texture(popup_ptr, popup_ptr->bg_pixmap_ptr);
			popup_ptr = popup_ptr->next_visible_popup_ptr;
		}
	}

	// Render the blocks on the map that insect with the view frustum.

#ifdef RENDERSTATS
	int start_render_time_ms = get_time_ms();
#endif
	compute_view_bounding_box();
	compute_frustum_plane_equations();
	render_map();
#ifdef RENDERSTATS
	int end_render_time_ms = get_time_ms();
	diagnose("Map rendering time = %d ms",
		end_render_time_ms - start_render_time_ms);
#endif

	// If not using hardware acceleration...

	if (!hardware_acceleration) {
		float sky_delta_v;
		edge sky_left_edge, sky_right_edge;

		// Render the orb.

		render_orb();

		// Scale the sky texture coordinates if there is a sky pixmap.

		if (sky_pixmap_ptr) {
			sky_start_u *= sky_pixmap_ptr->width;
			sky_start_v *= sky_pixmap_ptr->height;
			sky_end_u *= sky_pixmap_ptr->width;
			sky_end_v *= sky_pixmap_ptr->height;
		}

		// Initialise the fields of the sky's left and right edge.  Notice that
		// the 1/tz values are 0.0 rather than a really small value, since that
		// causes inaccuracies in u/tz and v/tz; the span rendering functions
		// will use a 1/tz value of 1.0 instead when rendering the sky spans.

		sky_left_edge.sx = 0.0f;
		sky_left_edge.one_on_tz = 0.0f;
		sky_left_edge.u_on_tz = sky_start_u;
		sky_left_edge.v_on_tz = sky_start_v; 
		sky_right_edge.sx = (float)window_width;
		sky_right_edge.one_on_tz = 0.0f;
		sky_right_edge.u_on_tz = sky_end_u;
		sky_right_edge.v_on_tz = sky_start_v;

		// Fill each span buffer row with a sky span.

		sky_delta_v = (sky_end_v - sky_start_v) / window_height;
		for (row = 0; row < window_height; row++) {

			// Add the sky span to the span buffer.

			add_span(row, &sky_left_edge, &sky_right_edge, sky_pixmap_ptr,
				sky_colour_pixel, sky_brightness_index, false);

			// Adjust v/tz for the next row.

			sky_left_edge.v_on_tz += sky_delta_v; 
			sky_right_edge.v_on_tz += sky_delta_v;
		}
	}
	
	// Render all movable blocks.
	
	render_movable_blocks();

	// If there is a player block, render it last.

	if (player_block_ptr)
		render_player_block();

	// If not using hardware acceleration, step through all opaque spans in the
	// span buffer, and add each to the span list of the pixmap associated with
	// that span.  Solid colour spans go in their own list.

	if (!hardware_acceleration) {
		for (row = 0; row < window_height; row++) {
			span_row *span_row_ptr = (*span_buffer_ptr)[row];
			span *span_ptr = span_row_ptr->opaque_span_list;
			while (span_ptr) {
				int brightness_index;
				span *next_span_ptr = span_ptr->next_span_ptr;
				pixmap_ptr = span_ptr->pixmap_ptr;
				brightness_index = span_ptr->brightness_index;
				if (pixmap_ptr) {
					span **span_list_ptr = 
						&pixmap_ptr->span_lists[brightness_index];
					span_ptr->next_span_ptr = *span_list_ptr;
					*span_list_ptr = span_ptr;
				} else {
					span_ptr->next_span_ptr = colour_span_list;
					colour_span_list = span_ptr;
				}
				span_ptr = next_span_ptr;
			}
		}
	}

#ifdef RENDERSTATS
	start_render_time_ms = get_time_ms();
#endif

	// Step through each pixmap in each texture, rendering any polygons/spans
	// listed in these pixmaps.

	render_textured_polygons_or_spans();

	// Render the colour polygons/spans, followed by the transparent 
	// polygons/spans.

	render_colour_polygons_or_spans();
	render_transparent_polygons_or_spans();

#ifdef RENDERSTATS
	end_render_time_ms = get_time_ms();
	diagnose("Polygon rendering time = %d ms",
		end_render_time_ms - start_render_time_ms);
#endif

	// Unlock the frame buffer.

	unlock_frame_buffer();

	// If hardware acceleration is enabled, render the visible popup list in
	// back to front order.

	if (hardware_acceleration) {
		popup *popup_ptr = visible_popup_list;
		while (popup_ptr) {
			texture_ptr = popup_ptr->bg_texture_ptr;
			if (texture_ptr)
				render_popup_texture(popup_ptr, popup_ptr->bg_pixmap_ptr);
			texture_ptr = popup_ptr->fg_texture_ptr;
			if (texture_ptr)
				render_popup_texture(popup_ptr, &texture_ptr->pixmap_list[0]);
			popup_ptr = popup_ptr->next_visible_popup_ptr;
		}
	}

	// If the viewpoint has changed since the last frame, reset the current
	// popup square, and if there was a previous popup square make all popups on
	// that square with a rollover trigger invisible.

	if (viewpoint_has_changed) {
		curr_popup_square_ptr = NULL;
		if (prev_popup_square_ptr) {
			popup *popup_ptr = prev_popup_square_ptr->popup_list;
			while (popup_ptr) {
				if ((popup_ptr->trigger_flags & ROLLOVER) != 0)
					popup_ptr->visible_flags &= ~ROLLOVER;
				popup_ptr = popup_ptr->next_square_popup_ptr;
			}
		}
	}

	// If a popup is currently selected that is visible due to a rollover
	// trigger, pretend the current popup square is the same as the
	// previous popup square in order to keep all rollover popups visible.

	else if (curr_popup_ptr && (curr_popup_ptr->visible_flags & ROLLOVER))
		curr_popup_square_ptr = prev_popup_square_ptr;

	// If the current popup square is different to the previous popup square...

	else if (curr_popup_square_ptr != prev_popup_square_ptr) {

		// If there was a previous popup square, make all popups on that square
		// with a rollover trigger invisible.

		if (prev_popup_square_ptr) {
			popup *popup_ptr = prev_popup_square_ptr->popup_list;
			while (popup_ptr) {
				if ((popup_ptr->trigger_flags & ROLLOVER) != 0)
					popup_ptr->visible_flags &= ~ROLLOVER;
				popup_ptr = popup_ptr->next_square_popup_ptr;
			}
		}

		// If there is a current popup square, make all popups on that square
		// with a rollover trigger visible.

		if (curr_popup_square_ptr) {
			popup *popup_ptr = curr_popup_square_ptr->popup_list;
			while (popup_ptr) {
				if ((popup_ptr->trigger_flags & ROLLOVER) != 0) {

					// If the popup was not previously visible, set the time
					// that this popup was made visible, and if the window
					// alignment is by mouse then set the position of the popup.

					if (popup_ptr->visible_flags == 0) {
						popup_ptr->start_time_ms = curr_time_ms;
						if (popup_ptr->window_alignment == MOUSE) {
							bg_texture_ptr = popup_ptr->bg_texture_ptr;
							fg_texture_ptr = popup_ptr->fg_texture_ptr;
							if (bg_texture_ptr) {
								popup_width = bg_texture_ptr->width;
								popup_height = bg_texture_ptr->height;
							} else {
								popup_width = fg_texture_ptr->width;
								popup_height = fg_texture_ptr->height;
							}
							popup_ptr->sx = mouse_x - popup_width / 2;
							popup_ptr->sy = mouse_y - popup_height / 2;
						}
					}
					popup_ptr->visible_flags |= ROLLOVER;
				}
				popup_ptr = popup_ptr->next_square_popup_ptr;
			}
		}
	}

	// If a mouse selection hasn't been found yet, check whether the orb has
	// been selected.  If the orb has no exit, ignore it.

	if (!found_selection && orb_exit_ptr && mouse_x >= curr_orb_x &&
		mouse_x < curr_orb_x + curr_orb_width && mouse_y >= curr_orb_y &&
		mouse_y < curr_orb_y + curr_orb_height) {
		found_selection = true;
		curr_selected_exit_ptr = orb_exit_ptr;
	}

	// Update the total polygons rendered.

	total_polygons_rendered += polygons_rendered_in_frame;

	END_TIMING("render_frame");
	REPORT_SUM("render_block", render_block_cycles);
	REPORT_SUM("culling blocks", cull_block_cycles);
	REPORT_SUM("finding closest lights", find_light_cycles);
	REPORT_SUM("transforming vertices", transform_vertex_cycles);
	REPORT_SUM("render_polygon", render_polygon_cycles);
	REPORT_SUM("clip_3D_polygon", clip_3D_polygon_cycles);
	REPORT_SUM("clip_2D_polygon", clip_2D_polygon_cycles);
	REPORT_SUM("lighting polygons", lighting_cycles);
	REPORT_SUM("scaling textures", scale_texture_cycles);

#ifdef RENDERSTATS
	diagnose("Polygons rendered in this frame = %d of %d processed (%g%%)",
		polygons_rendered_in_frame, polygons_processed_in_frame,
		(float)polygons_rendered_in_frame / (float)polygons_processed_in_frame *
		100.0f);
	diagnose("Polygons reaching render_polygon() function = %d of %d processed "
		"(%g%%)", polygons_processed_late_in_frame, polygons_processed_in_frame,
		(float)polygons_processed_late_in_frame / 
		(float)polygons_processed_in_frame * 100.0f);
	diagnose("Blocks rendered in this frame = %d of %d processed (%g%%)",
		blocks_rendered_in_frame, blocks_processed_in_frame,
		(float)blocks_rendered_in_frame / (float)blocks_processed_in_frame * 
		100.0f);
#endif
}