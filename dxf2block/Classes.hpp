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

// Type definitions for various standard integer types.

typedef unsigned char byte;
typedef unsigned short word;

// Mathematical macros.

#define EPSILON			0.001
#define ABS(x)			((x) < 0 ? -(x) : (x))
#define FEQUAL(x,y)		(ABS((x) - (y)) < EPSILON)
#define FZERO(x)		(ABS(x) < EPSILON)
#define FPOSITIVE(x)	((x) >= EPSILON)

// Enumerated type for comparing polygons with planes.

enum poly_pos { 
	BEHIND_PLANE, 
	IN_FRONT_OF_PLANE, 
	INTERSECTS_PLANE, 
	LIES_ON_PLANE
};

//------------------------------------------------------------------------------
// Vertex structure.
//------------------------------------------------------------------------------

class vertex {
public:
	int vertex_no;					// Vertex number.
	double x, y, z;					// 3D coordinate.
	vertex *next_vertex_ptr;		// Next vertex in list.

	vertex();
	vertex(int vertex_no_value, double x_value, double y_value, double z_value);
	~vertex();
};

//------------------------------------------------------------------------------
// Vector structure.
//------------------------------------------------------------------------------

class vector {
public:
	double dx, dy, dz;				// 3D vector.

	vector();
	vector(double dx_value, double dy_value, double dz_value);
	~vector();
	friend vector operator-(vertex B, vertex A);	// Vector from A to B.
	friend vector operator*(vector A, vector B);	// Cross product.
	friend bool operator==(vector A, vector B);		// Equality test.
	void normalise(void);							// Normalise to unit length.
};

//------------------------------------------------------------------------------
// Polygon class.
//------------------------------------------------------------------------------

class polygon {
public:
	int part_no;					// Part number.
	int vertices;					// Number of vertices in polygon.
	int *vertex_no_list;			// List of original vertex numbers.
	polygon *next_polygon_ptr;		// Next polygon in list.

	polygon();
	polygon(int part_no_value);
	~polygon();
	bool create_vertex_no_list(int vertices_value);
};

//------------------------------------------------------------------------------
// Part class.
//------------------------------------------------------------------------------

class part {
public:
	int part_no;					// Part number.
	char *part_name;				// Part name.
	part *next_part_ptr;			// Next part in list.

	part();
	part(int part_no_value);
	~part();
	bool set_part_name(char *part_name_value);
};

//------------------------------------------------------------------------------
// Block class.
//------------------------------------------------------------------------------

class block {
private:
	vertex *last_vertex_ptr;		// Last vertex in list.
	part *last_part_ptr;			// Last part in list.
	polygon *last_polygon_ptr;		// Last polygon in list.
public:
	vertex *vertex_list;			// List of vertices in block.
	part *part_list;				// List of parts in block.
	polygon *polygon_list;			// List of polygons in block.

	block();
	~block();
	int add_vertex(double x, double y, double z);
	int find_part(char *part_name);
	int new_part(char *part_name);
	bool new_polygon(int part_no, int vertices, int *vertex_no_list);
	vertex *get_vertex(polygon *polygon_ptr, int vertex_index);
};
