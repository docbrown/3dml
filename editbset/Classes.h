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

// Type definitions for various standard integer types.

typedef unsigned char byte;
typedef unsigned short word;

// Mathematical macros.

#define EPSILON			0.0001
#define ABS(x)			((x) < 0 ? -(x) : (x))
#define FEQ(x,y)		(ABS((x) - (y)) < EPSILON)
#define FNE(x,y)		(ABS((x) - (y)) >= EPSILON)
#define FLT(x,y)		((x) < ((y) - EPSILON))
#define FLE(x,y)		((x) <= ((y) + EPSILON))
#define FGT(x,y)		((x) > ((y) + EPSILON))
#define FGE(x,y)		((x) >= ((y) - EPSILON))
#define FZERO(x)		(ABS(x) < EPSILON)
#define FPOS(x)			((x) >= EPSILON)
#define FNEG(x)			((x) <= -EPSILON) 

// Enumerated type for comparing polygons with planes.

#define	NOT_YET_CALCULATED	0
#define IN_FRONT_OF_PLANE	1 
#define BEHIND_PLANE		2
#define INTERSECTS_PLANE	3

// Polygon and light source compass direction types.

enum compass { UP, DOWN, NORTH, EAST, SOUTH, WEST };

// Texture styles.

enum texstyle { TILED_TEXTURE, SCALED_TEXTURE, STRETCHED_TEXTURE };

// Alignment modes.

enum alignment { 
	TOP_LEFT, TOP, TOP_RIGHT,
	LEFT, CENTER, RIGHT, 
	BOTTOM_LEFT, BOTTOM, BOTTOM_RIGHT
};

// Block types.

enum blocktype { 
	STRUCTURAL_BLOCK = 1, 
	MULTIFACETED_SPRITE = 2, 
	ANGLED_SPRITE = 4, 
	REVOLVING_SPRITE = 8,
	FACING_SPRITE = 16,
	PLAYER_SPRITE = 32
};
#define SPRITE_BLOCK	(MULTIFACETED_SPRITE | ANGLED_SPRITE | \
						 REVOLVING_SPRITE | FACING_SPRITE | PLAYER_SPRITE)

// Playback mode of a sound.

enum playmode { LOOPED_PLAY, RANDOM_PLAY };

//------------------------------------------------------------------------------
// String class.  Semantically it behaves as a character array, only one that
// has a variable length.
//------------------------------------------------------------------------------

class string {
private:
	char *text;

public:
	string();
	string(char *new_text);
	string(const char *new_text);
	string(const string &old);
	~string();
	operator char *();
	char& operator[](int index);
	string& operator=(const string &old);
	string& operator +=(const char *new_text);
	string operator +(const char *new_text);
	void truncate(unsigned int length);
	void copy(const char *new_text, unsigned int length);
};

//------------------------------------------------------------------------------
// Definition of a palette entry (indexed 24-bit colour).
//------------------------------------------------------------------------------

struct RGBcolour {
	byte red, green, blue;		// Colour components (if not transparent).
};

//------------------------------------------------------------------------------
// Texture coordinates class.
//------------------------------------------------------------------------------

struct texcoords {
	double u, v;
	texcoords *next_texcoords_ptr;

	texcoords();
	~texcoords();
};

//------------------------------------------------------------------------------
// Texture coordinates list class.
//------------------------------------------------------------------------------

class texcoords_list {
public:
	int elements;
	texcoords *first_texcoords_ptr;
	texcoords *last_texcoords_ptr;

	texcoords_list();
	~texcoords_list();
	void delete_texcoords_list(void);
	bool add_texcoords(double u, double v);
};

//------------------------------------------------------------------------------
// Size class.
//------------------------------------------------------------------------------

struct size {
	int width, height;
};

//------------------------------------------------------------------------------
// Reference class.
//------------------------------------------------------------------------------

class ref {
public:
	int ref_no;
	ref *next_ref_ptr;

	ref();
	~ref();
};

//------------------------------------------------------------------------------
// Reference list class.
//------------------------------------------------------------------------------

class ref_list {
public:
	int refs;
	ref *first_ref_ptr;
	ref *last_ref_ptr;

	ref_list();
	~ref_list();
	void delete_ref_list(void);
	bool add_ref(int ref_no);
};

//------------------------------------------------------------------------------
// Vertex structure.
//------------------------------------------------------------------------------

class vertex {
public:
	int vertex_no;					// Vertex reference number.
	double x, y, z;					// 3D coordinate.
	vertex *next_vertex_ptr;		// Next vertex in list.

	vertex();
	vertex(vertex &old);
	vertex(int vertex_no_value, double x_value, double y_value, double z_value);
	~vertex();
	friend vertex operator +(vertex A, vertex B);
	friend vertex operator /(vertex A, double B);
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
	friend double operator&(vector A, vector B);	// Dot product.
	friend vector operator*(vector A, vector B);	// Cross product.
	friend bool operator==(vector A, vector B);		// Equality test.
	void normalise(void);							// Normalise to unit length.
};

//------------------------------------------------------------------------------
// Vertex definition class.
//------------------------------------------------------------------------------

class vertex_def {
public:
	int vertex_no;					// Vertex reference number.
	double u, v;					// Texture coordinate.
	vertex_def *next_vertex_def_ptr;// Next vertex definition in list.

	vertex_def();
	vertex_def(int vertex_no_value, double u_value, double v_value);
	~vertex_def();
};

//------------------------------------------------------------------------------
// Polygon class.
//------------------------------------------------------------------------------

class block;						// Forward declaration.

class polygon {
private:
	vertex_def *last_vertex_def_ptr;// Pointer to last vertex definition.
public:
	int part_no;					// Part number.
	int polygon_no;					// Final polygon number.
	int vertices;					// Number of vertices in polygon.
	vertex_def *vertex_def_list;	// List of vertex definitions.
	vector plane_normal;			// Plane normal vector.
	double plane_offset;			// Plane offset.
	bool got_texture_angle;			// TRUE if texture angle is valid.
	double texture_angle;			// Texture angle.
	int rear_polygon_no;			// Number of rear polygon in BSP tree.
	int front_polygon_no;			// Number of front polygon in BSP tree.
	polygon *rear_polygon_ptr;		// BSP rear tree branch.
	polygon *front_polygon_ptr;		// BSP front tree branch.
	polygon *next_polygon_ptr;		// Next polygon in list.
	polygon *next_BSP_polygon_ptr;	// Next polygon in BSP list.

	polygon();
	polygon(int polygon_no_value);
	~polygon();
	vertex_def *add_vertex_def(int vertex_no, double u, double v);
	vertex_def *get_vertex_def(int vertex_def_index);
	void init_polygon(polygon *polygon_ptr);
	void compute_plane_equation(block *block_ptr);
	void project_texture(block *block_ptr);
	void stretch_texture(block *block_ptr);
};

//------------------------------------------------------------------------------
// Part class.
//------------------------------------------------------------------------------

#define PART_TEXTURE		1
#define PART_COLOUR			2
#define	PART_TRANSLUCENCY	4
#define PART_STYLE			8
#define PART_FACES			16
#define PART_ANGLE			32

class part {
public:
	int part_no;					// Part number.
	string name;					// Part name.
	int flags;						// Flags indicating which fields are valid.
	string texture;					// Texture file name.
	string colour;					// Part colour.
	string translucency;			// Part translucency.
	string style;					// Texture style.
	string faces;					// Faces on polygons.
	string angle;					// Angle of texture on polygons.
	part *next_part_ptr;			// Next part in list.

	part();
	part(int part_no_value);
	~part();
};

//------------------------------------------------------------------------------
// Block class.
//------------------------------------------------------------------------------

#define BLOCK_TYPE				0x0000001
#define BLOCK_ENTRANCE			0x0000002

#define LIGHT_STYLE				0x0000004
#define LIGHT_BRIGHTNESS		0x0000008
#define LIGHT_COLOUR			0x0000010
#define LIGHT_RADIUS			0x0000020
#define LIGHT_POSITION			0x0000040
#define LIGHT_SPEED				0x0000080
#define LIGHT_FLOOD				0x0000100
#define LIGHT_DIRECTION			0x0000200
#define LIGHT_CONE				0x0000400
#define LIGHT_DEFINED			0x00007fc

#define SOUND_FILE				0x0000800
#define SOUND_RADIUS			0x0001000
#define SOUND_VOLUME			0x0002000
#define SOUND_PLAYBACK			0x0004000
#define SOUND_DELAY				0x0008000
#define SOUND_FLOOD				0x0010000
#define SOUND_ROLLOFF			0x0020000
#define SOUND_DEFINED			0x003f800

#define EXIT_HREF				0x0040000
#define EXIT_TARGET				0x0080000
#define EXIT_TEXT				0x0100000
#define EXIT_TRIGGER			0x0200000
#define EXIT_DEFINED			0x03c0000

#define PARAM_ORIENT			0x0400000
#define PARAM_SOLID				0x0800000
#define PARAM_ALIGN				0x1000000
#define PARAM_SPEED				0x2000000
#define PARAM_ANGLE				0x4000000
#define	PARAM_DEFINED			0x7c00000

class block {
private:
	vertex *last_vertex_ptr;		// Last vertex in list.
	part *last_part_ptr;			// Last part in list.
	polygon *last_polygon_ptr;		// Last polygon in list.
public:
	string name;					// Block name.
	int vertices;					// Number of vertices in block.
	vertex *vertex_list;			// List of vertices in block.
	part *part_list;				// List of parts in block.
	int polygons;					// Number of polygons in block.
	polygon *polygon_list;			// List of polygons in block.
	int flags;						// Flags indicating which fields are valid.
	blocktype type;					// Block type.
	string entrance;				// Entrance allowed flag.
	bool point_light;				// TRUE if point light, FALSE if spot light.
	string light_style;				// Light style.
	string light_brightness;		// Light brightness.
	string light_colour;			// Light colour;
	string light_radius;			// Light radius.
	string light_position;			// Light position.
	string light_speed;				// Light speed.
	string light_flood;				// Light flood.
	string light_direction;			// Light direction.
	string light_cone;				// Light cone.
	string sound_file;				// Wave file path.
	string sound_radius;			// Radius of sound field.
	string sound_volume;			// Volume of sound.
	string sound_playback;			// Playback mode.
	string sound_delay;				// Sound delay range.
	string sound_flood;				// TRUE if sound floods radius.
	string sound_rolloff;			// Sound rolloff factor.
	string exit_href;
	string exit_target;
	string exit_text;
	string exit_trigger;
	string param_orient;
	string param_solid;
	string param_align;
	string param_speed;
	string param_angle;
	bool has_BSP_tree;				// TRUE if block has a BSP tree.
	int root_polygon_no;			// Number of root polygon of BSP tree.
	polygon *root_polygon_ptr;		// Pointer to root polygon of BSP tree.

	block();
	~block();
	vertex *find_vertex(double x, double y, double z);
	vertex *add_vertex(double x, double y, double z);
	part *find_part(char *part_name);
	part *add_part(int part_no, char *part_name);
	polygon *add_polygon(void);
	vertex *get_vertex(vertex_def *vertex_def_ptr);
	polygon *get_polygon(int polygon_no);
	void create_BSP_polygon_list(void);
	void renumber_polygon_list(void);
};

//------------------------------------------------------------------------------
// Block definition class.
//------------------------------------------------------------------------------

#define BLOCK_DOUBLE_SYMBOL			0x0000001

class block_def {
public:
	int item_index;					// Index of block in list view. 
	int flags;						// Flags indicating which fields are valid.
	char symbol;					// Block symbol.
	string double_symbol;			// Double block symbol.
	string file_name;				// Block file name.
	block *block_ptr;				// Pointer to block.
	block_def *next_block_def_ptr;	// Next block definition in list.

	block_def();
	~block_def();
};

//------------------------------------------------------------------------------
// Style class.
//------------------------------------------------------------------------------

#define STYLE_VERSION		0x0000001
#define STYLE_NAME			0x0000002
#define STYLE_SYNOPSIS		0x0000004

#define PLACEHOLDER_TEXTURE	0x0000008

#define SKY_TEXTURE			0x0000010
#define SKY_COLOUR			0x0000020
#define SKY_BRIGHTNESS		0x0000040
#define SKY_DEFINED			0x0000070

#define GROUND_TEXTURE		0x0000080
#define GROUND_COLOUR		0x0000100
#define GROUND_DEFINED		0x0000180

#define ORB_TEXTURE			0x0000200
#define ORB_COLOUR			0x0000400
#define ORB_BRIGHTNESS		0x0000800
#define ORB_POSITION		0x0001000
#define ORB_HREF			0x0002000
#define ORB_TARGET			0x0004000
#define ORB_TEXT			0x0008000
#define ORB_DEFINED			0x000fe00

class style {
public:
	int blocks;
	block_def *block_def_list;
	int style_flags;
	string style_version;
	string style_name;
	string style_synopsis;
	string placeholder_texture;
	string sky_texture;
	string sky_colour;
	string sky_brightness;
	string ground_texture;
	string ground_colour;
	string orb_texture;
	string orb_colour;
	string orb_brightness;
	string orb_position;
	string orb_href;
	string orb_target;
	string orb_text;

	style();
	~style();
	block_def *add_block_def(char block_symbol, char *block_double_symbol,
		char *block_file_name, block *block_ptr);
	bool symbol_unique(block_def *block_def_ptr, char symbol);
	bool double_symbol_unique(block_def *block_def_ptr, char *double_symbol);
	bool name_unique(block_def *block_def_ptr, char *name);
	bool del_block_def(block_def *block_def_ptr);
	void del_block_def_list(void);
};