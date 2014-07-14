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
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "classes.h"
#include "main.h"

//------------------------------------------------------------------------------
// String class.
//------------------------------------------------------------------------------

// The empty string: used to guarentee that a string will have a value in case
// of an out of memory status.

static char *empty_string = "";

// Default constructor allocates a empty string.

string::string()
{
	text = empty_string;
}

// Constructor for char *.

string::string(char *new_text)
{
	if (new_text != NULL && new_text[0] != '\0') {
		if ((text = new char[strlen(new_text) + 1]) != NULL)
			strcpy(text, new_text);
	} else
		text = empty_string;
}

// Constructor for const char *.

string::string(const char *new_text)
{
	if (new_text != NULL && new_text[0] != '\0') {
		if ((text = new char[strlen(new_text) + 1]) != NULL)
			strcpy(text, new_text);
	} else
		text = empty_string;
}

// Copy constructor.

string::string(const string &old)
{
	if (old.text != NULL && old.text[0] != '\0') {
		if ((text = new char[strlen(old.text) + 1]) != NULL)
			strcpy(text, old.text);
	} else
		text = empty_string;
}

// Default destructor deletes the text.

string::~string()
{
	if (text && text[0] != '\0')
		delete []text;
}

// Conversion operator to return a pointer to the text.

string::operator char *()
{
	return(text);
}

// Character index operator.

char&
string::operator [](int index)
{
	return(text[index]);
}

// Assignment operator.

string&
string::operator=(const string &old)
{
	if (text && text[0] != '\0') {
		delete []text;
	}
	if (old.text) {
		if ((text = new char[strlen(old.text) + 1]) != NULL)
			strcpy(text, old.text);
	} else
		text = empty_string;
	return(*this);
}

// Concatenate and assignment operator.

string&
string::operator+=(const char *old_text)
{
	if (old_text && old_text[0] != '\0') {
		char *new_text;
		
		if ((new_text = new char[strlen(text) + strlen(old_text) + 1]) 
			!= NULL) {
			strcpy(new_text, text);
			strcat(new_text, old_text);
			delete []text;
			text = new_text;
		}
	}
	return(*this);
}

// Plus operator appends text to the string and returns a new string.

string
string::operator+(const char *new_text)
{
	string new_string;

	new_string = text;
	if (new_text)
		new_string += new_text;
	return(new_string);
}

// Method to truncate a string to the given length.

void
string::truncate(unsigned int length)
{
	char *text_buf;
	
	if (text && strlen(text) > length) {
		if (length == 0 || (text_buf = new char[length + 1]) == NULL) {
			delete []text;
			text = empty_string;
		} else {
			strncpy(text_buf, text, length);
			text_buf[length] = '\0';
			delete []text;
			text = text_buf;
		}
	}
}

// Method to copy a fixed number of characters from a string.  It is assumed
// that the specified length is not greater than the string.

void
string::copy(const char *new_text, unsigned int length)
{
	if (text && text[0] != '\0')
		delete []text;
	if (new_text && new_text[0] != '\0' && length > 0 &&
		(text = new char[length + 1]) != NULL) {
		strncpy(text, new_text, length);
		text[length] = '\0';
	} else
		text = empty_string;
}

//------------------------------------------------------------------------------
// Texture coordinates class.
//------------------------------------------------------------------------------

// Default constructor initialises the texture coordinates.

texcoords::texcoords()
{
	u = 0.0;
	v = 0.0;
	next_texcoords_ptr = NULL;
}

// Destructor does nothing.

texcoords::~texcoords()
{
}

//------------------------------------------------------------------------------
// Texture coordinates list class.
//------------------------------------------------------------------------------

// Default constructor initialises the texture coordinates list.

texcoords_list::texcoords_list()
{
	elements = 0;
	first_texcoords_ptr = NULL;
	last_texcoords_ptr = NULL;
}

// Destructor deletes the texture coordinates list.

texcoords_list::~texcoords_list()
{
	texcoords *texcoords_ptr = first_texcoords_ptr;
	while (texcoords_ptr) {
		texcoords *next_texcoords_ptr = texcoords_ptr->next_texcoords_ptr;
		delete texcoords_ptr;
		texcoords_ptr = next_texcoords_ptr;
	}
}

// Method deletes the texture coordinates list.

void 
texcoords_list::delete_texcoords_list(void)
{
	texcoords *texcoords_ptr = first_texcoords_ptr;
	while (texcoords_ptr) {
		texcoords *next_texcoords_ptr = texcoords_ptr->next_texcoords_ptr;
		delete texcoords_ptr;
		texcoords_ptr = next_texcoords_ptr;
	}
	elements = 0;
	first_texcoords_ptr = NULL;
	last_texcoords_ptr = NULL;
}

// Method to create new texture coordinates and add it to the tail of the list.

bool
texcoords_list::add_texcoords(double u, double v)
{
	texcoords *texcoords_ptr;

	// Create the new texture coordinates and initialise it.
	
	if ((texcoords_ptr = new texcoords) == NULL)
		return(false);
	texcoords_ptr->u = u;
	texcoords_ptr->v = v;

	// If there is no last texture coordinates, add the new texture coordinates
	// to the head of the list, otherwise add it to the end.

	if (last_texcoords_ptr == NULL) {
		first_texcoords_ptr = texcoords_ptr;
		last_texcoords_ptr = texcoords_ptr;
	} else {
		last_texcoords_ptr->next_texcoords_ptr = texcoords_ptr;
		last_texcoords_ptr = texcoords_ptr;
	}

	// Increment the number of texture coordinates in the list.

	elements++;
	return(true);
}

//------------------------------------------------------------------------------
// Reference class.
//------------------------------------------------------------------------------

// Default constructor initialises the reference.

ref::ref()
{
	ref_no = 0;
	next_ref_ptr = NULL;
}

// Destructor does nothing.

ref::~ref()
{
}

//------------------------------------------------------------------------------
// Reference list class.
//------------------------------------------------------------------------------

// Default constructor initialises the reference list.

ref_list::ref_list()
{
	refs = 0;
	first_ref_ptr = NULL;
	last_ref_ptr = NULL;
}

// Destructor deletes the reference list.

ref_list::~ref_list()
{
	ref *ref_ptr = first_ref_ptr;
	while (ref_ptr) {
		ref *next_ref_ptr = ref_ptr->next_ref_ptr;
		delete ref_ptr;
		ref_ptr = next_ref_ptr;
	}
}

// Method deletes the reference list.

void 
ref_list::delete_ref_list(void)
{
	ref *ref_ptr = first_ref_ptr;
	while (ref_ptr) {
		ref *next_ref_ptr = ref_ptr->next_ref_ptr;
		delete ref_ptr;
		ref_ptr = next_ref_ptr;
	}
	refs = 0;
	first_ref_ptr = NULL;
	last_ref_ptr = NULL;
}

// Method to create a new reference and add it to the tail of the list.

bool
ref_list::add_ref(int ref_no)
{
	ref *ref_ptr;

	// Create the new reference and initialise it.
	
	if ((ref_ptr = new ref) == NULL)
		return(false);
	ref_ptr->ref_no = ref_no;

	// If there is no last reference, add the new reference to the head of the
	// list, otherwise add it to the end.

	if (last_ref_ptr == NULL) {
		first_ref_ptr = ref_ptr;
		last_ref_ptr = ref_ptr;
	} else {
		last_ref_ptr->next_ref_ptr = ref_ptr;
		last_ref_ptr = ref_ptr;
	}

	// Increment the number of references in the list.

	refs++;
	return(true);
}

//------------------------------------------------------------------------------
// Vertex class.
//------------------------------------------------------------------------------

// Default constructor creates a zero vertex.

vertex::vertex()
{
	x = 0.0;
	y = 0.0;
	z = 0.0;
	next_vertex_ptr = NULL;
}

// Copy constructor makes sure the pointer to the next vertex is NULL, rather
// than copying the pointer from the old vertex (believe me, this is bad!).

vertex::vertex(vertex& old)
{
	x = old.x;
	y = old.y;
	z = old.z;
	next_vertex_ptr = NULL;
}

// Constructor to initialise vertex.

vertex::vertex(int vertex_no_value, double x_value, double y_value, 
			   double z_value)
{
	vertex_no = vertex_no_value;
	x = x_value;
	y = y_value;
	z = z_value;
	next_vertex_ptr = NULL;
}

// Default destructor.

vertex::~vertex()
{
}

// Operator to add two vertices to create a third.

vertex
operator +(vertex A, vertex B)
{
	vertex C;

	C.x = A.x + B.x;
	C.y = A.y + B.y;
	C.z = A.z + B.z;
	return(C);
}

// Operator to divide a vertex by a constant.

vertex
operator /(vertex A, double B)
{
	vertex C;

	C.x = A.x / B;
	C.y = A.y / B;
	C.z = A.z / B;
	return(C);
}

//------------------------------------------------------------------------------
// Vector class.
//------------------------------------------------------------------------------

// Default constructor creates a zero-length vector.

vector::vector()
{
	dx = 0.0;
	dy = 0.0;
	dz = 0.0;
}

// Constructor to initialise vector.

vector::vector(double dx_value, double dy_value, double dz_value)
{
	dx = dx_value;
	dy = dy_value;
	dz = dz_value;
}

// Default destructor does nothing.

vector::~vector()
{
}

// Operator to create a vector from vertex A to B.

vector
operator -(vertex B, vertex A)
{
	vector C;

	C.dx = B.x - A.x;
	C.dy = B.y - A.y;
	C.dz = B.z - A.z;
	return(C);
}

// Operator to compute the dot product of two vectors.

double
operator &(vector A, vector B)
{
	return(A.dx * B.dx + A.dy * B.dy + A.dz * B.dz);
}

// Operator to compute the cross product of two vectors.

vector
operator *(vector A, vector B)
{
	vector C;

	C.dx = A.dy * B.dz - A.dz * B.dy;
	C.dy = A.dz * B.dx - A.dx * B.dz;
	C.dz = A.dx * B.dy - A.dy * B.dx;
	return(C);
}

// Operator to compare two vectors for equality.

bool
operator==(vector A, vector B)
{
	return(FEQ(A.dx, B.dx) && FEQ(A.dy, B.dy) && FEQ(A.dz, B.dz));
}

// Method to normalise a vector.

void
vector::normalise(void)
{
	double magnitude = sqrt(dx * dx + dy * dy + dz * dz);
	if (magnitude > 0.0) {
		dx = dx / magnitude;
		dy = dy / magnitude;
		dz = dz / magnitude;
	}
}

//------------------------------------------------------------------------------
// Vertex definition class.
//------------------------------------------------------------------------------

// Default constructor initialise the next vertex definition pointer.

vertex_def::vertex_def()
{
	next_vertex_def_ptr = NULL;
}

// Constructor to initialise the vertex definition.

vertex_def::vertex_def(int vertex_no_value, double u_value, double v_value)
{
	vertex_no = vertex_no_value;
	u = u_value;
	v = v_value;
	next_vertex_def_ptr = NULL;
}

// Default destructor deletes the next vertex definition, if it exists.

vertex_def::~vertex_def()
{
}

//------------------------------------------------------------------------------
// Polygon class.
//------------------------------------------------------------------------------

// Default constructor initialises vertex definition list and pointer to next
// polygon.

polygon::polygon()
{
	part_no = 0;
	polygon_no = 0;
	vertices = 0;
	vertex_def_list = NULL;
	got_texture_angle = false;
	texture_angle = 0.0;
	front_polygon_ptr = NULL;
	rear_polygon_ptr = NULL;
	next_polygon_ptr = NULL;
	next_BSP_polygon_ptr = NULL;
}

// Constructor to initialise polygon number

polygon::polygon(int polygon_no_value)
{
	part_no = 0;
	polygon_no = polygon_no_value;
	vertices = 0;
	vertex_def_list = NULL;
	got_texture_angle = false;
	texture_angle = 0.0;
	next_polygon_ptr = NULL;
	next_BSP_polygon_ptr = NULL;
}


// Default destructor.

polygon::~polygon()
{
	if (vertex_def_list) {
		vertex_def *next_vertex_def_ptr = vertex_def_list->next_vertex_def_ptr;
		delete vertex_def_list;
		vertex_def_list = next_vertex_def_ptr;
	}
}

// Method to add a vertex to the block.

vertex_def *
polygon::add_vertex_def(int vertex_no, double u, double v)
{
	vertex_def *vertex_def_ptr;

	// Create a new vertex definition object.

	vertices++;
	if ((vertex_def_ptr = new vertex_def(vertex_no, u, v)) == NULL)
		return(NULL);

	// Add it to the end of the vertex definition list.

	if (vertex_def_list)
		last_vertex_def_ptr->next_vertex_def_ptr = vertex_def_ptr;
	else
		vertex_def_list = vertex_def_ptr;

	// Remember it as last vertex object added.

	last_vertex_def_ptr = vertex_def_ptr;
	return(vertex_def_ptr);
}

// Method to get a vertex definition.

vertex_def *
polygon::get_vertex_def(int vertex_def_index)
{
	vertex_def *vertex_def_ptr = vertex_def_list;
	while (vertex_def_index > 0 && vertex_def_ptr) {
		vertex_def_ptr = vertex_def_ptr->next_vertex_def_ptr;
		vertex_def_index--;
	}
	return(vertex_def_ptr);
}

// Method to initialise the polygon with the contents of another.

void
polygon::init_polygon(polygon *polygon_ptr)
{
	// Make a copy of the part number, plane normal, plane offset, and texture 
	// angle (if present).

	part_no = polygon_ptr->part_no;
	plane_normal = polygon_ptr->plane_normal;
	plane_offset = polygon_ptr->plane_offset;
	if (polygon_ptr->got_texture_angle) {
		got_texture_angle = true;
		texture_angle = polygon_ptr->texture_angle;
	}
}

// Method to compute a polygon's plane equation.

void
polygon::compute_plane_equation(block *block_ptr)
{
	// Get pointers to the first three vertices in the polygon object.

	vertex *vertex1_ptr = block_ptr->get_vertex(get_vertex_def(0));
	vertex *vertex2_ptr = block_ptr->get_vertex(get_vertex_def(1));
	vertex *vertex3_ptr = block_ptr->get_vertex(get_vertex_def(2));

	// Compute the vectors leading from vertex 2 to vertex 1 and 3.

	vector vector21 = *vertex1_ptr - *vertex2_ptr;
	vector vector23 = *vertex3_ptr - *vertex2_ptr;

	// Compute the plane normal as the cross product of vector23 and
	// vector21, normalised to unit length.  The plane normal is pointing
	// away from the front face.  

	plane_normal = vector23 * vector21;
	plane_normal.normalise();

	// Compute the plane offset D = -(Ax + By + Cz), using the plane
	// normal for (A,B,C) and the first vertex for (x,y,z).

	plane_offset = -(plane_normal.dx * vertex1_ptr->x + 
		plane_normal.dy * vertex1_ptr->y + plane_normal.dz * vertex1_ptr->z);
}

// Method to compute the texture coordinates for each vertex based upon the
// following procedure:
// (1) Imagine the texture has been applied onto the outside face of
//	   the block's bounding box.  The face is the one the polygon is
//	   pointing towards.
// (2) Perform an orthographic projection to place the texture onto
//	   the polygon, taking into account the texture offset provided.

void 
polygon::project_texture(block *block_ptr)
{
	compass direction;

	// Determine the direction the polygon is facing in by looking at which
	// component of the plane normal vector is largest, and it's sign.

	if (FGE(ABS(plane_normal.dx), ABS(plane_normal.dy)) &&
		FGE(ABS(plane_normal.dx), ABS(plane_normal.dz))) {
		if (FLT(plane_normal.dx, 0.0))
			direction = WEST;
		else
			direction = EAST;
	}
	if (FGE(ABS(plane_normal.dy), ABS(plane_normal.dx)) &&
		FGE(ABS(plane_normal.dy), ABS(plane_normal.dz))) {
		if (FLT(plane_normal.dy, 0.0))
			direction = DOWN;
		else
			direction = UP;
	}
	if (FGE(ABS(plane_normal.dz), ABS(plane_normal.dx)) &&
		FGE(ABS(plane_normal.dz), ABS(plane_normal.dy))) {
		if (FLT(plane_normal.dz, 0.0))
			direction = SOUTH;
		else
			direction = NORTH;
	}

	// Step through the vertex list.

	for (int index = 0; index < vertices; index++) {
		vertex_def *vertex_def_ptr;
		vertex *vertex_ptr;
		double u, v;

		// Perform the orthogonal projection.

		vertex_def_ptr = get_vertex_def(index);
		vertex_ptr = block_ptr->get_vertex(vertex_def_ptr);
		switch (direction) {
		case UP:
			u = vertex_ptr->x;
			v = 256.0 - vertex_ptr->z;
			break;
		case DOWN:
			u = vertex_ptr->x;
			v = vertex_ptr->z;
			break;
		case NORTH:
			u = 256.0 - vertex_ptr->x;
			v = 256.0 - vertex_ptr->y;
			break;
		case SOUTH:
			u = vertex_ptr->x;
			v = 256.0 - vertex_ptr->y;
			break;
		case EAST:
			u = vertex_ptr->z;
			v = 256.0 - vertex_ptr->y;
			break;
		case WEST:
			u = 256.0 - vertex_ptr->z;
			v = 256.0 - vertex_ptr->y;
		}

		// Save the texture coordinates in the vertex definition
		// structure, normalised to values between 0 and 1.

		vertex_def_ptr->u = u / 256.0;
		vertex_def_ptr->v = v / 256.0;
	}
}

// Method to stretch a texture so that it's corners match the vertices of the
// polygon.  This method is only called if the polygon has 3 or 4 vertices.

void 
polygon::stretch_texture(block *block_ptr)
{
	int index, vertex_index[4] = {-1, -1, -1, -1};
	vertex centroid(0, 0.0, 0.0, 0.0);
	compass dir_xz_plane;
	int rotation;
	vertex_def *vertex_def_ptr;

	// Compute the centroid of the polygon by averaging the components of
	// it's vertices.

	for (index = 0; index < vertices; index++) {
		vertex *vertex_ptr = block_ptr->get_vertex(get_vertex_def(index));
		centroid = centroid + *vertex_ptr;
	}
	centroid = centroid / vertices;

	// Compute the direction of the polygon in the X-Z plane (north, east, south
	// or west).  Only if the polygon is horizontal will it point up or down.

	if (FEQ(plane_normal.dx, 0.0) && FEQ(plane_normal.dz, 0.0)) {
		if (FGE(plane_normal.dy, 0.0))
			dir_xz_plane = UP;
		else
			dir_xz_plane = DOWN;
	} else {
		if (FGE(ABS(plane_normal.dx), ABS(plane_normal.dz))) {
			if (FLT(plane_normal.dx, 0.0))
				dir_xz_plane = WEST;
			else
				dir_xz_plane = EAST;
		} else {
			if (FLT(plane_normal.dz, 0.0))
				dir_xz_plane = SOUTH;
			else
				dir_xz_plane = NORTH;
		}
	}

	// Compute the angle of rotation to apply to the texture 
	// (0, 90, 180 or 270).

	rotation = (int)(texture_angle / 90.0);

	// Compute the indices of the vertices that will recieve each of the
	// four corner texture coordinates, based upon what quandrant the
	// vertices are in relative to the polygon's centroid.

	for (index = 0; index < vertices; index++) {
		vertex *vertex_ptr;
		double dx, dy;
		int quadrant;

		// First compute the 2D coordinates of this vertex relative to the
		// centroid, taking into account the direction the polygon is facing. 

		vertex_ptr = block_ptr->get_vertex(get_vertex_def(index));
		switch (dir_xz_plane) {
		case NORTH:
			dx = centroid.x - vertex_ptr->x;
			dy = vertex_ptr->y - centroid.y;
			break;
		case EAST:
			dx = vertex_ptr->z - centroid.z;
			dy = vertex_ptr->y - centroid.y;
			break;
		case SOUTH:
			dx = vertex_ptr->x - centroid.x;
			dy = vertex_ptr->y - centroid.y;
			break;
		case WEST:
			dx = centroid.z - vertex_ptr->z;
			dy = vertex_ptr->y - centroid.y;
			break;
		case UP:
			dx = vertex_ptr->x - centroid.x;
			dy = vertex_ptr->z - centroid.z;
			break;
		case DOWN:
			dx = centroid.x - vertex_ptr->x;
			dy = vertex_ptr->z - centroid.z;
		}

		// Now determine the quadrant that the vertex is in, adjusting it by
		// the desired angle of rotation.

		if (FLT(dx, 0.0)) {
			if (FGE(dy, 0.0))
				quadrant = 0;
			else
				quadrant = 3;
		} else if (FGT(dx, 0.0)) {
			if (FGT(dy, 0.0))
				quadrant = 1;
			else
				quadrant = 2;
		} else {
			if (FGE(dy, 0.0))
				quadrant = 1;
			else
				quadrant = 3;
		}
		
		// If this quadrant is already occupied, either move this vertex to the
		// next quadrant in a clockwise direction, or move the occupying vertex
		// to the previous quadrant in an anti-clockwise direction.

		if (vertex_index[quadrant] >= 0) {
			int new_quadrant;

			new_quadrant = (quadrant + 1) % 4;
			if (vertex_index[new_quadrant] < 0)
				vertex_index[new_quadrant] = index;
			else {
				new_quadrant = (quadrant + 3) % 4;
				vertex_index[new_quadrant] = vertex_index[quadrant];
				vertex_index[quadrant] = index;
			}
		} else
			vertex_index[quadrant] = index;			
	}

	// Assign the texture coordinates to the vertices, which are normalised
	// between 0 and 1.

	if (vertex_index[0] >= 0 && (vertex_index[0] < 3 || vertices == 4)) {
		vertex_def_ptr = get_vertex_def(vertex_index[0]);
		vertex_def_ptr->u = 0.0;
		vertex_def_ptr->v = 0.0;
	}
	if (vertex_index[1] >= 0 && (vertex_index[1] < 3 || vertices == 4)) {
		vertex_def_ptr = get_vertex_def(vertex_index[1]);
		vertex_def_ptr->u = 1.0;
		vertex_def_ptr->v = 0.0;
	}
	if (vertex_index[2] >= 0 && (vertex_index[2] < 3 || vertices == 4)) {
		vertex_def_ptr = get_vertex_def(vertex_index[2]);
		vertex_def_ptr->u = 1.0;
		vertex_def_ptr->v = 1.0;
	}
	if (vertex_index[3] >= 0 && (vertex_index[3] < 3 || vertices == 4)) {
		vertex_def_ptr = get_vertex_def(vertex_index[3]);
		vertex_def_ptr->u = 0.0;
		vertex_def_ptr->v = 1.0;
	}
}

//------------------------------------------------------------------------------
// Part class.
//------------------------------------------------------------------------------

// Default constructor.

part::part()
{
	flags = 0;
	next_part_ptr = NULL;
}

// Constructor.

part::part(int part_no_value)
{
	part_no = part_no_value;
	flags = 0;
	next_part_ptr = NULL;
}

// Default destructor.

part::~part()
{
}

//------------------------------------------------------------------------------
// Block class.
//------------------------------------------------------------------------------

// Default constructor.
	
block::block()
{
	vertices = 0;
	vertex_list = NULL;
	part_list = NULL;
	polygons = 0;
	polygon_list = NULL;
	type = STRUCTURAL_BLOCK;
	flags = 0;
	has_BSP_tree = false;
	root_polygon_ptr = NULL;
}

// Default destructor.
	
block::~block()
{
	if (vertex_list) {
		vertex *next_vertex_ptr = vertex_list->next_vertex_ptr;
		delete vertex_list;
		vertex_list = next_vertex_ptr;
	}
	while (part_list) {
		part *next_part_ptr = part_list->next_part_ptr;
		delete part_list;
		part_list = next_part_ptr;
	}
	if (polygon_list) {
		polygon *next_polygon_ptr = polygon_list->next_polygon_ptr;
		delete polygon_list;
		polygon_list = next_polygon_ptr;
	}
}

// Method to find a vertex, and return a pointer to it if found.

vertex *
block::find_vertex(double x, double y, double z)
{
	vertex *vertex_ptr = vertex_list;
	while (vertex_ptr) {
		if (vertex_ptr->x == x && vertex_ptr->y == y && vertex_ptr->z == z)
			break;
		vertex_ptr = vertex_ptr->next_vertex_ptr;
	}
	return(vertex_ptr);
}

// Method to add a vertex to the block.

vertex *
block::add_vertex(double x, double y, double z)
{
	vertex *vertex_ptr;

	// Create a new vertex object.

	vertices++;
	if ((vertex_ptr = new vertex(vertices, x, y, z)) == NULL)
		return(NULL);

	// Add it to the end of the vertex list.

	if (vertex_list)
		last_vertex_ptr->next_vertex_ptr = vertex_ptr;
	else
		vertex_list = vertex_ptr;

	// Remember it as last vertex object added.

	last_vertex_ptr = vertex_ptr;
	return(vertex_ptr);
}

// Method to find a part with the given part name, and to return a pointer to
// it.

part *
block::find_part(char *part_name)
{
	part *part_ptr = part_list;
	while (part_ptr) {
		if (!stricmp(part_ptr->name, part_name))
			break;
		part_ptr = part_ptr->next_part_ptr;
	}
	return(part_ptr);
}

// Method to create a new part, and to return a pointer to it.

part *
block::add_part(int part_no, char *part_name)
{
	part *part_ptr;

	// Create the part and set it's number and name.

	if ((part_ptr = new part(part_no)) == NULL)
		return(NULL);
	part_ptr->name = part_name;

	// Add the part to the end of the part list.

	if (part_list)
		last_part_ptr->next_part_ptr = part_ptr;
	else
		part_list = part_ptr;

	// Remember it as the last part added.

	last_part_ptr = part_ptr;
	return(part_ptr);
}

// Method to create a new polygon, and return a pointer to it.

polygon *
block::add_polygon(void)
{
	polygon *polygon_ptr;

	// Create the polygon object.

	polygons++;
	if ((polygon_ptr = new polygon(polygons)) == NULL)
		return(NULL);

	// Add the polygon to the end of the polygon list.

	if (polygon_list)
		last_polygon_ptr->next_polygon_ptr = polygon_ptr;
	else
		polygon_list = polygon_ptr;

	// Remember it as the last polygon added.

	last_polygon_ptr = polygon_ptr;
	return(polygon_ptr);
}

// Method to return a vertex indexed by a vertex definition.

vertex *
block::get_vertex(vertex_def *vertex_def_ptr)
{
	vertex *vertex_ptr = vertex_list;
	while (vertex_ptr) {
		if (vertex_ptr->vertex_no == vertex_def_ptr->vertex_no)
			return(vertex_ptr);
		vertex_ptr = vertex_ptr->next_vertex_ptr;
	}
	return(NULL);
}

// Method to return a polygon with the given polygon number.

polygon *
block::get_polygon(int polygon_no)
{
	polygon *polygon_ptr = polygon_list;
	while (polygon_ptr) {
		if (polygon_ptr->polygon_no == polygon_no)
			return(polygon_ptr);
		polygon_ptr = polygon_ptr->next_polygon_ptr;
	}
	return(NULL);
}

// Method to link all polygons into a "BSP polygon" list.

void
block::create_BSP_polygon_list(void)
{
	polygon *polygon_ptr = polygon_list;
	while (polygon_ptr) {
		polygon_ptr->next_BSP_polygon_ptr = polygon_ptr->next_polygon_ptr;
		polygon_ptr = polygon_ptr->next_polygon_ptr;
	}
}

// Method to compute new polygon numbers.

void
block::renumber_polygon_list(void)
{
	part *part_ptr = part_list;
	int polygon_no = 0;
	while (part_ptr) {
		polygon *polygon_ptr = polygon_list;
		while (polygon_ptr) {
			if (polygon_ptr->part_no == part_ptr->part_no)
				polygon_ptr->polygon_no = ++polygon_no;
			polygon_ptr = polygon_ptr->next_polygon_ptr;
		}
		part_ptr = part_ptr->next_part_ptr;
	}
}

//------------------------------------------------------------------------------
// Block definition class.
//------------------------------------------------------------------------------

// Default constructor.

block_def::block_def()
{
	block_ptr = NULL;
	next_block_def_ptr = NULL;
	flags = 0;
}

// Default destructor.

block_def::~block_def()
{
	if (block_ptr)
		delete block_ptr;
}

//------------------------------------------------------------------------------
// Style class.
//------------------------------------------------------------------------------

// Default constructor.

style::style()
{
	blocks = 0;
	block_def_list = NULL;
	style_flags = 0;
}

// Default destructor.

style::~style()
{
	while (block_def_list) {
		block_def *next_block_def_ptr = block_def_list->next_block_def_ptr;
		delete block_def_list;
		block_def_list = next_block_def_ptr;
	}
}

// Method to add a block definition to the style.

block_def *
style::add_block_def(char block_symbol, char *block_double_symbol,
					 char *block_file_name, block *block_ptr)
{
	block_def *block_def_ptr;

	// Create the block definition, and initialise it.

	if ((block_def_ptr = new block_def) == NULL)
		return(NULL);
	block_def_ptr->symbol = block_symbol;
	if (strlen(block_double_symbol) > 0) {
		block_def_ptr->flags |= BLOCK_DOUBLE_SYMBOL;
		block_def_ptr->double_symbol = block_double_symbol;
	} else
		block_def_ptr->flags &= ~BLOCK_DOUBLE_SYMBOL;
	block_def_ptr->file_name = block_file_name;
	block_def_ptr->block_ptr = block_ptr;

	// Add the block to the head of the list,

	block_def_ptr->next_block_def_ptr = block_def_list;
	block_def_list = block_def_ptr;
	blocks++;
	return(block_def_ptr);
}

// Method to determine if a block definition's symbol is unique.

bool
style::symbol_unique(block_def *block_def_ptr, char symbol)
{
	block_def *curr_block_def_ptr;

	curr_block_def_ptr = block_def_list;
	while (curr_block_def_ptr) {
		if (block_def_ptr != curr_block_def_ptr &&
			symbol == curr_block_def_ptr->symbol)
			return(false);
		curr_block_def_ptr = curr_block_def_ptr->next_block_def_ptr;
	}
	return(true);
}

// Method to determine if a block definition's double symbol is unique.

bool
style::double_symbol_unique(block_def *block_def_ptr, char *double_symbol)
{
	block_def *curr_block_def_ptr;

	curr_block_def_ptr = block_def_list;
	while (curr_block_def_ptr) {
		if (block_def_ptr != curr_block_def_ptr &&
			!strcmp(double_symbol, curr_block_def_ptr->double_symbol))
			return(false);
		curr_block_def_ptr = curr_block_def_ptr->next_block_def_ptr;
	}
	return(true);
}

// Method to determine if a block definition's name is unique.

bool 
style::name_unique(block_def *block_def_ptr, char *name)
{
	block_def *curr_block_def_ptr;
	block *curr_block_ptr;

	curr_block_def_ptr = block_def_list;
	while (curr_block_def_ptr) {
		if (block_def_ptr != curr_block_def_ptr) {
			curr_block_ptr = curr_block_def_ptr->block_ptr;
			if (!stricmp(name, curr_block_ptr->name))
				return(false);
		}
		curr_block_def_ptr = curr_block_def_ptr->next_block_def_ptr;
	}
	return(true);
}

// Method to delete a block definition from the style.

bool
style::del_block_def(block_def *block_def_ptr)
{
	block_def *prev_block_def_ptr, *curr_block_def_ptr;

	// If there is not a block definition with the given pointer in the list,
	// return with a failure status.

	prev_block_def_ptr = NULL;
	curr_block_def_ptr = block_def_list;
	while (curr_block_def_ptr) {
		if (curr_block_def_ptr == block_def_ptr)
			break;
		prev_block_def_ptr = curr_block_def_ptr;
		curr_block_def_ptr = curr_block_def_ptr->next_block_def_ptr;
	}
	if (curr_block_def_ptr == NULL)
		return(false);

	// Remove the block from the list, then delete it.

	if (prev_block_def_ptr)
		prev_block_def_ptr->next_block_def_ptr = 
			block_def_ptr->next_block_def_ptr;
	else
		block_def_list = block_def_ptr->next_block_def_ptr;
	block_def_ptr->next_block_def_ptr = NULL;
	delete block_def_ptr;
	return(true);
}

// Method to delete all block definitions.

void 
style::del_block_def_list(void)
{
	while (block_def_list) {
		block_def *next_block_def_ptr = block_def_list->next_block_def_ptr;
		delete block_def_list;
		block_def_list = next_block_def_ptr;
	}
	blocks = 0;
}