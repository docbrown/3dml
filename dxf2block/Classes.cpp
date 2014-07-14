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
// The Original Code is DXF2Block. 
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
#include "classes.hpp"

//------------------------------------------------------------------------------
// Vertex class.
//------------------------------------------------------------------------------

// Default constructor initialises the next vertex pointer.

vertex::vertex()
{
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

// Default destructor deletes the next vertex, if it exists.

vertex::~vertex()
{
	if (next_vertex_ptr)
		delete next_vertex_ptr;
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
	return(FEQUAL(A.dx, B.dx) && FEQUAL(A.dy, B.dy) && FEQUAL(A.dz, B.dz));
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
// Polygon class.
//------------------------------------------------------------------------------

// Default constructor initialises vertex number list and pointer to next
// polygon.

polygon::polygon()
{
	vertex_no_list = NULL;
	next_polygon_ptr = NULL;
}

// Constructor to initialise polygon with the given part number.

polygon::polygon(int part_no_value)
{
	part_no = part_no_value;
	vertex_no_list = NULL;
	next_polygon_ptr = NULL;
}

// Default destructor deletes the vertex number list and next polygon, if they
// exist.

polygon::~polygon()
{
	if (vertex_no_list)
		delete []vertex_no_list;
	if (next_polygon_ptr)
		delete next_polygon_ptr;
}

// Method to create the vertex number list.

bool
polygon::create_vertex_no_list(int vertices_value)
{
	vertices = vertices_value;
	if ((vertex_no_list = new int[vertices]) == NULL)
		return(false);
	return(true);
}

//------------------------------------------------------------------------------
// Part class.
//------------------------------------------------------------------------------

// Default constructor initialises name and pointer to next part.

part::part()
{
	part_name = NULL;
	next_part_ptr = NULL;
}

// Constructor initialises part number, name and pointer to next part.

part::part(int part_no_value)
{
	part_no = part_no_value;
	part_name = NULL;
	next_part_ptr = NULL;
}

// Default destructor deletes part name and next part, if they exist.

part::~part()
{
	if (part_name)
		delete []part_name;
	if (next_part_ptr)
		delete next_part_ptr;
}

// Method to set part name.

bool
part::set_part_name(char *part_name_value)
{
	if (part_name_value) {
		if ((part_name = new char[strlen(part_name_value) + 1]) == NULL)
			return(false);
		strcpy(part_name, part_name_value);
	}
	return(true);
}

//------------------------------------------------------------------------------
// Block class.
//------------------------------------------------------------------------------

// Default constructor initialises vertex, part and polygon lists.
	
block::block()
{
	vertex_list = NULL;
	part_list = NULL;
	polygon_list = NULL;
}

// Default destructor deletes the vertex, part and polygon lists, if they exist.
	
block::~block()
{
	if (vertex_list)
		delete vertex_list;
	if (part_list)
		delete part_list;
	if (polygon_list)
		delete polygon_list;
}

// Method to add a vertex to the block.  If a vertex with the same (x,y,z)
// coordinates already exists, it's number will be returned.

int
block::add_vertex(double x, double y, double z)
{
	vertex *vertex_ptr;
	int vertex_no;

	// Search for a vertex that matches the one that is to be added.  If
	// found, return it's number.

	vertex_ptr = vertex_list;
	while (vertex_ptr) {
		if (vertex_ptr->x == x && vertex_ptr->y == y && vertex_ptr->z == z)
			return(vertex_ptr->vertex_no);
		vertex_ptr = vertex_ptr->next_vertex_ptr;
	}

	// Determine the vertex number for the new vertex.

	if (vertex_list)
		vertex_no = last_vertex_ptr->vertex_no + 1;
	else
		vertex_no = 1;

	// Create a new vertex object.  If this fails, return 0 as the vertex
	// number.

	if ((vertex_ptr = new vertex(vertex_no, x, y, z)) == NULL)
		return(0);

	// Add it to the end of the vertex list.

	if (vertex_list)
		last_vertex_ptr->next_vertex_ptr = vertex_ptr;
	else
		vertex_list = vertex_ptr;

	// Remember it as last vertex object added.

	last_vertex_ptr = vertex_ptr;

	// Return the vertex number.

	return(vertex_no);
}

// Method to find a part with the given part name, and to return it's number.
// If not found, zero is returned.

int
block::find_part(char *part_name)
{
	part *part_ptr = part_list;
	while (part_ptr) {
		if (!stricmp(part_ptr->part_name, part_name))
			break;
		part_ptr = part_ptr->next_part_ptr;
	}
	if (part_ptr)
		return(part_ptr->part_no);
	return(0);
}

// Method to create a new part, and to return it's number.

int
block::new_part(char *part_name)
{
	part *part_ptr;
	int part_no;

	// If a part already exists with the desired part name, this is an error.

	if (find_part(part_name))
		return(0);

	// Determine the part number of the new part.

	if (part_list)
		part_no = last_part_ptr->part_no + 1;
	else
		part_no = 1;

	// Create the part and set it's number and name.

	if ((part_ptr = new part(part_no)) == NULL ||
		!part_ptr->set_part_name(part_name))
		return(0);

	// Add the part to the end of the part list.

	if (part_list)
		last_part_ptr->next_part_ptr = part_ptr;
	else
		part_list = part_ptr;

	// Remember it as the last part added.

	last_part_ptr = part_ptr;

	// Return the part number.

	return(part_no);
}

// Method to create a new polygon with the given part number and vertex number
// list.

bool
block::new_polygon(int part_no, int vertices, int *vertex_no_list)
{
	polygon *polygon_ptr;
	int index;

	// Create the polygon object and it's polygon list.

	if ((polygon_ptr = new polygon(part_no)) == NULL ||
		!polygon_ptr->create_vertex_no_list(vertices))
		return(false);

	// Initialise the vertex number list.

	for (index = 0; index < vertices; index++)
		polygon_ptr->vertex_no_list[index] = vertex_no_list[index];

	// Add the polygon to the end of the polygon list.

	if (polygon_list)
		last_polygon_ptr->next_polygon_ptr = polygon_ptr;
	else
		polygon_list = polygon_ptr;

	// Remember it as the last polygon added.

	last_polygon_ptr = polygon_ptr;
	return(true);
}

// Method to return a vertex indexed in a polygon object.

vertex *
block::get_vertex(polygon *polygon_ptr, int vertex_index)
{
	int vertex_no = polygon_ptr->vertex_no_list[vertex_index];
	vertex *vertex_ptr = vertex_list;
	while (vertex_ptr) {
		if (vertex_ptr->vertex_no == vertex_no)
			return(vertex_ptr);
		vertex_ptr = vertex_ptr->next_vertex_ptr;
	}
	return(NULL);
}