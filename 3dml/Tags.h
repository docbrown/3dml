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

//------------------------------------------------------------------------------
// Tag definitions.  Optional tags are assigned named constants so that they
// can be refered to in fileio.cpp.
//------------------------------------------------------------------------------

// Block tag.

#define BLOCK_PARAMS	3
#define BLOCK_TYPE		1
#define BLOCK_ENTRANCE	2
static string block_name;
static blocktype block_type;
static bool block_entrance;
static param block_param_list[BLOCK_PARAMS] = {
	{TOKEN_NAME, VALUE_NAME, &block_name, true},
	{TOKEN__TYPE, VALUE_BLOCK_TYPE, &block_type, false},
	{TOKEN_ENTRANCE, VALUE_BOOLEAN, &block_entrance, false}
};

// Vertex tag.

#define VERTEX_PARAMS	2
static int vertex_ref;
static vertex_entry vertex_coords;
static param vertex_param_list[VERTEX_PARAMS] = {
	{TOKEN_REF,	VALUE_INTEGER, &vertex_ref,	true},
	{TOKEN_COORDS, VALUE_VERTEX_COORDS,	&vertex_coords, true}
};

// Polygon tag.

#define POLYGON_PARAMS		5
#define POLYGON_FRONT		3
#define POLYGON_REAR		4 
static int polygon_ref;
static string polygon_vertices;
static string polygon_texcoords;
static int polygon_front;
static int polygon_rear;
static param polygon_param_list[POLYGON_PARAMS] = {
	{TOKEN_REF,	VALUE_INTEGER, &polygon_ref, true},
	{TOKEN_VERTICES, VALUE_STRING, &polygon_vertices, true},
	{TOKEN_TEXCOORDS, VALUE_STRING, &polygon_texcoords, true},
	{TOKEN_FRONT, VALUE_INTEGER, &polygon_front, false},
	{TOKEN_REAR, VALUE_INTEGER, &polygon_rear, false}
};

// Part tag.

#define PART_PARAMS			8
#define SPRITE_PART_PARAMS	6
#define PART_TEXTURE		1
#define PART_STREAM			2
#define PART_COLOUR			3
#define PART_TRANSLUCENCY	4
#define PART_STYLE			5
#define PART_FACES			6
#define PART_ANGLE			7
static string part_name;
static string part_texture;
static string part_stream;
static RGBcolour part_colour;
static float part_translucency;
static texstyle part_style;
static int part_faces;
static float part_angle;
static param part_param_list[PART_PARAMS] = {
	{TOKEN_NAME, VALUE_NAME, &part_name, true},
	{TOKEN_TEXTURE, VALUE_STRING, &part_texture, false},
	{TOKEN_STREAM, VALUE_NAME, &part_stream, false},
	{TOKEN_COLOUR, VALUE_RGB, &part_colour, false},
	{TOKEN_TRANSLUCENCY, VALUE_PERCENTAGE, &part_translucency, false},
	{TOKEN_STYLE, VALUE_TEXTURE_STYLE, &part_style, false},
	{TOKEN_FACES, VALUE_INTEGER, &part_faces, false},
	{TOKEN_ANGLE, VALUE_DEGREES, &part_angle, false}
};

// Part tag for create tag.

#define CREATE_PART_PARAMS	11
#define PART_PROJECTION		8
#define PART_SOLID			9
#define PART_RECT			10
static compass part_projection;
static bool part_solid;
static video_rect part_rect;
static param create_part_param_list[CREATE_PART_PARAMS] = {
	{TOKEN_NAME, VALUE_NAME_LIST, &part_name, true},
	{TOKEN_TEXTURE,	VALUE_STRING, &part_texture, false},
	{TOKEN_STREAM, VALUE_NAME, &part_stream, false},
	{TOKEN_COLOUR, VALUE_RGB, &part_colour, false},
	{TOKEN_TRANSLUCENCY, VALUE_PERCENTAGE, &part_translucency, false},
	{TOKEN_STYLE, VALUE_TEXTURE_STYLE, &part_style, false},
	{TOKEN_FACES, VALUE_INTEGER, &part_faces, false},
	{TOKEN_ANGLE, VALUE_DEGREES, &part_angle, false},
	{TOKEN_PROJECTION, VALUE_PROJECTION, &part_projection, false},
	{TOKEN_SOLID, VALUE_BOOLEAN, &part_solid, false},
	{TOKEN_RECT, VALUE_RECT, &part_rect, false}
};

// Param tag.

#define PARAM_PARAMS		3
#define PARAM_ORIENTATION	0
#define PARAM_SOLID			1
#define PARAM_MOVABLE		2
static orientation param_orientation;
static bool param_solid;
static bool param_movable;
static param param_param_list[PARAM_PARAMS] = {
	{TOKEN_ORIENTATION, VALUE_ORIENTATION, &param_orientation, false},
	{TOKEN_SOLID, VALUE_BOOLEAN, &param_solid, false},
	{TOKEN_MOVABLE, VALUE_BOOLEAN, &param_movable, false},
};

// Param tag for sprites.

#define SPRITE_PARAM_PARAMS 6
#define SPRITE_ANGLE		0
#define SPRITE_SPEED		1
#define SPRITE_ALIGNMENT	2
#define SPRITE_SOLID		3
#define SPRITE_MOVABLE		4
#define SPRITE_SIZE			5
static float sprite_angle;
static float sprite_speed;
static alignment sprite_alignment;
static bool sprite_solid;
static bool sprite_movable;
static size sprite_size;
static param sprite_param_param_list[SPRITE_PARAM_PARAMS] = {
	{TOKEN_ANGLE, VALUE_DEGREES, &sprite_angle, false},
	{TOKEN_SPEED, VALUE_FLOAT, &sprite_speed, false},
	{TOKEN_ALIGN, VALUE_VALIGNMENT, &sprite_alignment, false},
	{TOKEN_SOLID, VALUE_BOOLEAN, &sprite_solid, false},
	{TOKEN_MOVABLE, VALUE_BOOLEAN, &sprite_movable, false},
	{TOKEN_SIZE, VALUE_SPRITE_SIZE, &sprite_size, false}
};
	
// Point light tag.

#define POINT_LIGHT_PARAMS		8
#define POINT_LIGHT_STYLE		0
#define POINT_LIGHT_POSITION	1
#define POINT_LIGHT_BRIGHTNESS	2
#define POINT_LIGHT_RADIUS		3
#define POINT_LIGHT_SPEED		4
#define POINT_LIGHT_FLOOD		5
#define POINT_LIGHT_COLOUR		6
static lightstyle point_light_style;
static vertex point_light_position;
static pcrange point_light_brightness;
static float point_light_radius;
static float point_light_speed;
static bool point_light_flood;
static RGBcolour point_light_colour;
static mapcoords point_light_location;
static param point_light_param_list[POINT_LIGHT_PARAMS] = {
	{TOKEN_STYLE, VALUE_POINT_LIGHT_STYLE, &point_light_style, false},
	{TOKEN_POSITION, VALUE_VERTEX_COORDS, &point_light_position, false},
	{TOKEN_BRIGHTNESS, VALUE_PCRANGE, &point_light_brightness, false},
	{TOKEN_RADIUS, VALUE_RADIUS, &point_light_radius, false},
	{TOKEN_SPEED, VALUE_FLOAT, &point_light_speed, false},
	{TOKEN_FLOOD, VALUE_BOOLEAN, &point_light_flood, false},
	{TOKEN_COLOUR, VALUE_RGB, &point_light_colour, false},
	{TOKEN_LOCATION, VALUE_MAP_COORDS, &point_light_location, true}
};

// Spot light tag.

#define SPOT_LIGHT_PARAMS		10
#define SPOT_LIGHT_STYLE		0
#define SPOT_LIGHT_POSITION		1
#define SPOT_LIGHT_BRIGHTNESS	2
#define SPOT_LIGHT_RADIUS		3
#define SPOT_LIGHT_DIRECTION	4
#define SPOT_LIGHT_SPEED		5
#define SPOT_LIGHT_CONE			6
#define SPOT_LIGHT_FLOOD		7
#define SPOT_LIGHT_COLOUR		8
static lightstyle spot_light_style;
static vertex spot_light_position;
static float spot_light_brightness;
static float spot_light_radius;
static dirrange spot_light_direction;
static float spot_light_speed;
static float spot_light_cone;
static bool spot_light_flood;
static RGBcolour spot_light_colour;
static mapcoords spot_light_location;
static param spot_light_param_list[SPOT_LIGHT_PARAMS] = {
	{TOKEN_STYLE, VALUE_SPOT_LIGHT_STYLE, &spot_light_style, false},
	{TOKEN_POSITION, VALUE_VERTEX_COORDS, &spot_light_position, false},
	{TOKEN_BRIGHTNESS, VALUE_PERCENTAGE, &spot_light_brightness, false},
	{TOKEN_RADIUS, VALUE_RADIUS, &spot_light_radius, false},
	{TOKEN_DIRECTION, VALUE_DIRRANGE, &spot_light_direction, false},
	{TOKEN_SPEED, VALUE_FLOAT, &spot_light_speed, false},
	{TOKEN_CONE, VALUE_DEGREES, &spot_light_cone, false},
	{TOKEN_FLOOD, VALUE_BOOLEAN, &spot_light_flood, false},
	{TOKEN_COLOUR, VALUE_RGB, &spot_light_colour, false},
	{TOKEN_LOCATION, VALUE_MAP_COORDS, &spot_light_location, true}
};

// Sound tag.

#define SOUND_PARAMS	9
#define SOUND_FILE		0
#define SOUND_STREAM	1
#define SOUND_RADIUS	2
#define SOUND_VOLUME	3
#define SOUND_PLAYBACK	4
#define SOUND_DELAY		5
#define SOUND_FLOOD		6
#define SOUND_ROLLOFF	7
static string sound_file;
static string sound_stream;
static float sound_radius;
static float sound_volume;
static playmode sound_playback;
static delayrange sound_delay;
static bool sound_flood;
static float sound_rolloff;
static mapcoords sound_location;
static param sound_param_list[SOUND_PARAMS] = {
	{TOKEN_FILE, VALUE_STRING, &sound_file, false},
	{TOKEN_STREAM, VALUE_NAME, &sound_stream, false},
	{TOKEN_RADIUS, VALUE_RADIUS, &sound_radius, false},
	{TOKEN_VOLUME, VALUE_PERCENTAGE, &sound_volume, false},
	{TOKEN_PLAYBACK, VALUE_PLAYBACK_MODE, &sound_playback, false},
	{TOKEN_DELAY, VALUE_DELAY_RANGE, &sound_delay, false},
	{TOKEN_FLOOD, VALUE_BOOLEAN, &sound_flood, false},
	{TOKEN_ROLLOFF, VALUE_FLOAT, &sound_rolloff, false},
	{TOKEN_LOCATION, VALUE_MAP_COORDS, &sound_location, true}
};

// Entrance tag.

#define ENTRANCE_PARAMS 3
#define ENTRANCE_ANGLE	1
static string entrance_name;
static direction entrance_angle;
static mapcoords entrance_location;
static param entrance_param_list[ENTRANCE_PARAMS] = {
	{TOKEN_NAME, VALUE_NAME, &entrance_name, true},
	{TOKEN_ANGLE, VALUE_HEADING, &entrance_angle, false},
	{TOKEN_LOCATION, VALUE_MAP_COORDS, &entrance_location, true}
};

// Exit tag.

#define EXIT_PARAMS			5
#define ACTION_EXIT_PARAMS	2
#define EXIT_TARGET			1
#define EXIT_TRIGGER		2
#define EXIT_TEXT			3
static string exit_href;
static string exit_target;
static int exit_trigger;
static string exit_text;
static mapcoords exit_location;
static param exit_param_list[EXIT_PARAMS] = {
	{TOKEN_HREF, VALUE_STRING, &exit_href, true},
	{TOKEN_TARGET, VALUE_STRING, &exit_target, false},
	{TOKEN_TRIGGER, VALUE_EXIT_TRIGGER, &exit_trigger, false},
	{TOKEN_TEXT, VALUE_STRING, &exit_text, false},
	{TOKEN_LOCATION, VALUE_MAP_COORDS, &exit_location, true}
};

// Replace tag.

#define REPLACE_PARAMS	2
#define REPLACE_TARGET	1
static blockref replace_source;
static relcoords replace_target;
static param replace_param_list[REPLACE_PARAMS] = {
	{TOKEN__SOURCE, VALUE_BLOCK_REF, &replace_source, true},
	{TOKEN_TARGET, VALUE_REL_COORDS, &replace_target, false},
};

// Set stream tag.

/*
#define SET_STREAM_PARAMS	2
static string set_stream_name;
static int set_stream_command;
static param set_stream_param_list[SET_STREAM_PARAMS] = {
	{TOKEN_NAME, VALUE_NAME, &set_stream_name, true},
	{TOKEN_COMMAND, VALUE_STREAM_COMMAND, &set_stream_command, true}
};
*/

// Action tag.

#define ACTION_PARAMS	6
#define ACTION_TRIGGER	0
#define ACTION_TEXT		1
#define ACTION_RADIUS	2
#define ACTION_DELAY	3
#define ACTION_TARGET	4
static int action_trigger;
static float action_radius;
static delayrange action_delay;
static string action_text;
static mapcoords action_target;
static mapcoords action_location;
static param action_param_list[ACTION_PARAMS] = {
	{TOKEN_TRIGGER, VALUE_ACTION_TRIGGER, &action_trigger, false},
	{TOKEN_TEXT, VALUE_STRING, &action_text, false},
	{TOKEN_RADIUS, VALUE_RADIUS, &action_radius, false},
	{TOKEN_DELAY, VALUE_DELAY_RANGE, &action_delay, false},
	{TOKEN_TARGET, VALUE_MAP_COORDS, &action_target, false},
	{TOKEN_LOCATION, VALUE_MAP_COORDS, &action_location, true},
};

// Action tag for imagemaps.

#define IMAGEMAP_ACTION_PARAMS	4
static shape action_shape;
static string action_coords;
static param imagemap_action_param_list[IMAGEMAP_ACTION_PARAMS] = {
	{TOKEN_TRIGGER, VALUE_ACTION_TRIGGER, &action_trigger, false},
	{TOKEN_TEXT, VALUE_STRING, &action_text, false},
	{TOKEN_SHAPE, VALUE_SHAPE, &action_shape, true},
	{TOKEN_COORDS, VALUE_STRING, &action_coords, true},
};

// BSP_tree tag.

#define BSP_TREE_PARAMS 1
static int BSP_tree_root;
static param BSP_tree_param_list[BSP_TREE_PARAMS] = {
	{TOKEN_ROOT, VALUE_INTEGER, &BSP_tree_root, true}
};


// Video tag.

#define STREAM_PARAMS	3
#define STREAM_RP		1
#define STREAM_WMP		2
static string stream_name;
static string stream_rp;
static string stream_wmp;
static param stream_param_list[STREAM_PARAMS] = {
	{TOKEN_NAME, VALUE_NAME, &stream_name, true},
	{TOKEN_RP, VALUE_STRING, &stream_rp, false},
	{TOKEN_WMP, VALUE_STRING, &stream_wmp, false}
};

// Placeholder tag.

#define PLACEHOLDER_PARAMS 1
static string placeholder_texture;
static param placeholder_param_list[PLACEHOLDER_PARAMS] = {
	{TOKEN_TEXTURE, VALUE_STRING, &placeholder_texture, true}
};

// Sky tag.

#define SKY_PARAMS		4
#define SKY_TEXTURE		0
#define SKY_COLOUR		1
#define SKY_BRIGHTNESS	2
#define SKY_STREAM		3
static string sky_texture;
static RGBcolour sky_colour;
static float sky_intensity;
static string sky_stream;
static param sky_param_list[SKY_PARAMS] = {
	{TOKEN_TEXTURE,	VALUE_STRING, &sky_texture, false},
	{TOKEN_COLOUR, VALUE_RGB, &sky_colour, false},
	{TOKEN_BRIGHTNESS, VALUE_PERCENTAGE, &sky_intensity, false},
	{TOKEN_STREAM, VALUE_NAME, &sky_stream, false}
};

// Ground tag.

#define GROUND_PARAMS	3
#define GROUND_TEXTURE	0
#define GROUND_COLOUR	1
#define GROUND_STREAM	2
static string ground_texture;
static RGBcolour ground_colour;
static string ground_stream;
static param ground_param_list[GROUND_PARAMS] = {
	{TOKEN_TEXTURE, VALUE_STRING, &ground_texture, false},
	{TOKEN_COLOUR, VALUE_RGB, &ground_colour, false},
	{TOKEN_STREAM, VALUE_NAME, &ground_stream, false}
};

// Orb tag.

#define ORB_PARAMS		8
#define ORB_TEXTURE		0
#define ORB_POSITION	1
#define ORB_BRIGHTNESS	2
#define ORB_COLOUR		3
#define ORB_HREF		4
#define ORB_TARGET		5
#define ORB_TEXT		6
#define ORB_STREAM		7
static string orb_texture;
static direction orb_position;
static float orb_intensity;
static RGBcolour orb_colour;
static string orb_href;
static string orb_target;
static string orb_text;
static string orb_stream;
static param orb_param_list[ORB_PARAMS] = {
	{TOKEN_TEXTURE, VALUE_STRING, &orb_texture, false},
	{TOKEN_POSITION, VALUE_DIRECTION, &orb_position, false},
	{TOKEN_BRIGHTNESS, VALUE_PERCENTAGE, &orb_intensity, false},
	{TOKEN_COLOUR, VALUE_RGB, &orb_colour, false},
	{TOKEN_HREF, VALUE_STRING, &orb_href, false},
	{TOKEN_TARGET, VALUE_STRING, &orb_target, false},
	{TOKEN_TEXT, VALUE_STRING, &orb_text, false},
	{TOKEN_STREAM, VALUE_NAME, &orb_stream, false}
};

// Block tag for style file.

#define STYLE_BLOCK_PARAMS	4
#define STYLE_BLOCK_DOUBLE	1
#define STYLE_BLOCK_NAME	2
static char style_block_symbol;
static word style_block_double;
static string style_block_name;
static string style_block_file;
static param style_block_param_list[STYLE_BLOCK_PARAMS] = {
	{TOKEN_SYMBOL, VALUE_SINGLE_SYMBOL, &style_block_symbol, true},
	{TOKEN_DOUBLE, VALUE_DOUBLE_SYMBOL, &style_block_double, false},
	{TOKEN_NAME, VALUE_NAME, &style_block_name, false},
	{TOKEN_FILE, VALUE_STRING, &style_block_file, true}
};

// Title tag.

#define TITLE_PARAMS 1
static string title_name;
static param title_param_list[TITLE_PARAMS] = {
	{TOKEN_NAME, VALUE_STRING, &title_name, true}
};

// Blockset tag.

#define BLOCKSET_PARAMS 1
static string blockset_href;
static param blockset_param_list[BLOCKSET_PARAMS] = {
	{TOKEN_HREF, VALUE_STRING, &blockset_href, true}
};

// Map tag.

#define MAP_PARAMS	3
#define MAP_SCALE	1
#define MAP_STYLE	2
static mapcoords map_dimensions;
static float map_scale;
static mapstyle map_style;
static param map_param_list[MAP_PARAMS] = {
	{TOKEN_DIMENSIONS, VALUE_MAP_DIMENSIONS, &map_dimensions, true},
	{TOKEN_SCALE, VALUE_MAP_SCALE, &map_scale, false},
	{TOKEN_STYLE, VALUE_MAP_STYLE, &map_style, false}
};

// Ambient_light tag.

#define AMBIENT_LIGHT_PARAMS	2
#define AMBIENT_LIGHT_COLOUR	1
static float ambient_light_brightness;
static RGBcolour ambient_light_colour;
static param ambient_light_param_list[AMBIENT_LIGHT_PARAMS] = {
	{TOKEN_BRIGHTNESS, VALUE_PERCENTAGE, &ambient_light_brightness, true},
	{TOKEN_COLOUR, VALUE_RGB, &ambient_light_colour, false}
};

// Ambient_sound tag.

#define AMBIENT_SOUND_PARAMS	5
#define AMBIENT_SOUND_FILE		0
#define AMBIENT_SOUND_STREAM	1
#define AMBIENT_SOUND_VOLUME	2
#define AMBIENT_SOUND_PLAYBACK	3
#define AMBIENT_SOUND_DELAY		4
static string ambient_sound_file;
static string ambient_sound_stream;
static float ambient_sound_volume;
static playmode ambient_sound_playback;
static delayrange ambient_sound_delay;
static param ambient_sound_param_list[] = {
	{TOKEN_FILE, VALUE_STRING, &ambient_sound_file, false},
	{TOKEN_STREAM, VALUE_NAME, &ambient_sound_stream, false},
	{TOKEN_VOLUME, VALUE_PERCENTAGE, &ambient_sound_volume, false},
	{TOKEN_PLAYBACK, VALUE_PLAYBACK_MODE, &ambient_sound_playback, false},
	{TOKEN_DELAY, VALUE_DELAY_RANGE, &ambient_sound_delay, false}
};

// Popup tag.

#define POPUP_PARAMS		13
#define POPUP_PLACEMENT		0
#define POPUP_RADIUS		1
#define POPUP_BRIGHTNESS	2
#define POPUP_TEXTURE		3
#define POPUP_COLOUR		4
#define POPUP_SIZE			5
#define POPUP_TEXT			6
#define POPUP_TEXTCOLOUR	7
#define POPUP_TEXTALIGN		8
#define POPUP_IMAGEMAP		9
#define POPUP_TRIGGER		10
#define POPUP_STREAM		11
#define POPUP_LOCATION		12
static alignment popup_placement;
static float popup_radius;
static float popup_brightness;
static string popup_texture;
static RGBcolour popup_colour;
static size popup_size;
static string popup_text;
static RGBcolour popup_textcolour;
static alignment popup_textalign;
static string popup_imagemap;
static int popup_trigger;
static string popup_stream;
static mapcoords popup_location;
static param popup_param_list[POPUP_PARAMS] = {
	{TOKEN_PLACEMENT, VALUE_PLACEMENT, &popup_placement, false},
	{TOKEN_RADIUS, VALUE_RADIUS, &popup_radius, false},
	{TOKEN_BRIGHTNESS, VALUE_PERCENTAGE, &popup_brightness, false},
	{TOKEN_TEXTURE,	VALUE_STRING, &popup_texture, false},
	{TOKEN_COLOUR, VALUE_RGB, &popup_colour, false},
	{TOKEN_SIZE, VALUE_SIZE, &popup_size, false},
	{TOKEN_TEXT, VALUE_STRING, &popup_text, false},
	{TOKEN_TEXTCOLOUR, VALUE_RGB, &popup_textcolour, false},
	{TOKEN_TEXTALIGN, VALUE_ALIGNMENT, &popup_textalign, false},
	{TOKEN_IMAGEMAP, VALUE_NAME, &popup_imagemap, false},
	{TOKEN_TRIGGER, VALUE_POPUP_TRIGGER, &popup_trigger, false},
	{TOKEN_STREAM, VALUE_NAME, &popup_stream, false},
	{TOKEN_LOCATION, VALUE_MAP_COORDS, &popup_location, false}
};

// Area tag.

#define AREA_PARAMS 5
#define AREA_TARGET	3
#define AREA_TEXT	4
static shape area_shape;
static string area_coords;
static string area_href;
static string area_target;
static string area_text;
static param area_param_list[AREA_PARAMS] = {
	{TOKEN_SHAPE, VALUE_SHAPE, &area_shape, true},
	{TOKEN_COORDS, VALUE_STRING, &area_coords, true},
	{TOKEN_HREF, VALUE_STRING, &area_href, true},
	{TOKEN_TARGET, VALUE_STRING, &area_target, false},
	{TOKEN_TEXT, VALUE_STRING, &area_text, false}
};

// Create tag.

#define CREATE_PARAMS 2
static word create_symbol;
static string create_block;
static param create_param_list[CREATE_PARAMS] = {
	{TOKEN_SYMBOL, VALUE_SYMBOL, &create_symbol, true},
	{TOKEN_BLOCK, VALUE_STRING, &create_block, true}
};

// Player tag.

#define PLAYER_PARAMS	2
#define PLAYER_CAMERA	1
static word player_block;
static vertex player_camera;
static param player_param_list[PLAYER_PARAMS] = {
	{TOKEN_BLOCK, VALUE_SYMBOL, &player_block, true},
	{TOKEN_CAMERA, VALUE_VERTEX_COORDS, &player_camera, false}
};

// Imagemap tag.

#define IMAGEMAP_PARAMS 1
static string imagemap_name;
static param imagemap_param_list[IMAGEMAP_PARAMS] = {
	{TOKEN_NAME, VALUE_NAME, &imagemap_name, true}
};

// Level tag.

#define LEVEL_PARAMS	1
#define LEVEL_NUMBER	0
static int level_number;
static param level_param_list[LEVEL_PARAMS] = {
	{TOKEN_NUMBER, VALUE_INTEGER, &level_number, false}
};

// Load tag.

#define LOAD_PARAMS		2
#define LOAD_TEXTURE	0
#define LOAD_SOUND		1
static string load_texture_href;
static string load_sound_href;
static param load_param_list[LOAD_PARAMS] = {
	{TOKEN_TEXTURE, VALUE_STRING, &load_texture_href, false},
	{TOKEN_SOUND, VALUE_STRING, &load_sound_href, false}
};

// Style tag.

#define STYLE_PARAMS	3
#define STYLE_NAME		0
#define STYLE_SYNOPSIS	1
#define STYLE_VERSION	2
static string style_name;
static string style_synopsis;
static int style_version;
static param style_param_list[STYLE_PARAMS] = {
	{TOKEN_NAME, VALUE_STRING, &style_name, false},
	{TOKEN_SYNOPSIS, VALUE_STRING, &style_synopsis, false},
	{TOKEN_VERSION, VALUE_VERSION, &style_version, false}
};

// Cached blockset tag.

#define CACHED_BLOCKSET_PARAMS		6
#define CACHED_BLOCKSET_NAME		3
#define CACHED_BLOCKSET_SYNOPSIS	4
#define CACHED_BLOCKSET_VERSION		5
static string cached_blockset_href;
static int cached_blockset_size;
static int cached_blockset_updated;
static string cached_blockset_name;
static string cached_blockset_synopsis;
static unsigned int cached_blockset_version;
static param cached_blockset_param_list[CACHED_BLOCKSET_PARAMS] = {
	{TOKEN_HREF, VALUE_STRING, &cached_blockset_href, true},
	{TOKEN_SIZE, VALUE_INTEGER, &cached_blockset_size, true},
	{TOKEN_UPDATED, VALUE_INTEGER, &cached_blockset_updated, true},
	{TOKEN_NAME, VALUE_STRING, &cached_blockset_name, false},
	{TOKEN_SYNOPSIS, VALUE_STRING, &cached_blockset_synopsis, false},
	{TOKEN_VERSION, VALUE_VERSION, &cached_blockset_version, false}
};

// Blockset version tag.

#define BLOCKSET_VERSION_PARAMS	2
static unsigned int blockset_version_id;
static int blockset_version_size;
static param blockset_version_param_list[BLOCKSET_VERSION_PARAMS] = {
	{TOKEN_ID, VALUE_VERSION, &blockset_version_id, true},
	{TOKEN_SIZE, VALUE_STRING, &blockset_version_size, true}
};

// Rover version tag.

#define ROVER_VERSION_PARAMS 1
static unsigned int rover_version_id;
static param rover_version_param_list[ROVER_VERSION_PARAMS] = {
	{TOKEN_ID, VALUE_VERSION, &rover_version_id, true}
};

// OnLoad tag.

#define ONLOAD_PARAMS	2
#define ONLOAD_TARGET	1
static string onload_href;
static string onload_target;
static param onload_param_list[ONLOAD_PARAMS] = {
	{TOKEN_HREF, VALUE_STRING, &onload_href, true},
	{TOKEN_TARGET, VALUE_STRING, &onload_target, false}
};

// Spot tag.

#define SPOT_PARAMS		1
#define SPOT_VERSION	0
static int spot_version;
static param spot_param_list[SPOT_PARAMS] = {
	{TOKEN_VERSION, VALUE_VERSION, &spot_version, false}
};

//------------------------------------------------------------------------------
// Tag lists.
//------------------------------------------------------------------------------

// Vertices tag list.

static tag vertices_tag_list[] = {
	{TOKEN_VERTEX, vertex_param_list, VERTEX_PARAMS, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_NONE}
};

// Part tag list.

static tag part_tag_list[] = {
	{TOKEN_POLYGON, polygon_param_list, POLYGON_PARAMS, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_NONE}
};

// Parts tag list.

static tag parts_tag_list[] = {
	{TOKEN_PART, part_param_list, PART_PARAMS, TOKEN_CLOSE_TAG},
	{TOKEN_NONE}
};

// Block tag list.

static tag block_tag_list[] = {
	{TOKEN_VERTICES, NULL, 0, TOKEN_CLOSE_TAG},
	{TOKEN_PARTS, NULL, 0, TOKEN_CLOSE_TAG},
	{TOKEN_PARAM, param_param_list, PARAM_PARAMS, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_POINT_LIGHT, point_light_param_list, POINT_LIGHT_PARAMS - 1, 
	 TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_SPOT_LIGHT, spot_light_param_list, SPOT_LIGHT_PARAMS - 1, 
	 TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_SOUND, sound_param_list, SOUND_PARAMS - 1, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_EXIT, exit_param_list, EXIT_PARAMS - 1, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_BSP_TREE, BSP_tree_param_list, BSP_TREE_PARAMS, 
	 TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_NONE}
};

// Block tag list for sprites.

static tag sprite_block_tag_list[] = {
	{TOKEN_PART, part_param_list, SPRITE_PART_PARAMS, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_PARAM, sprite_param_param_list, SPRITE_PARAM_PARAMS, 
	 TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_POINT_LIGHT, point_light_param_list, POINT_LIGHT_PARAMS - 1, 
	 TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_SPOT_LIGHT, spot_light_param_list, SPOT_LIGHT_PARAMS - 1, 
	 TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_SOUND, sound_param_list, SOUND_PARAMS - 1, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_EXIT, exit_param_list, EXIT_PARAMS - 1, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_NONE}
};

// Style tag list.

static tag style_tag_list[] = {
	{TOKEN_PLACEHOLDER, placeholder_param_list, PLACEHOLDER_PARAMS, 
	 TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_SKY, sky_param_list, SKY_PARAMS, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_GROUND, ground_param_list, GROUND_PARAMS, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_ORB, orb_param_list, ORB_PARAMS, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_BLOCK, style_block_param_list, STYLE_BLOCK_PARAMS, 
	 TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_NONE}
};

// Head tag list.

static tag head_tag_list[] = {
	{TOKEN_TITLE, title_param_list, TITLE_PARAMS, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_BLOCKSET, blockset_param_list, BLOCKSET_PARAMS, 
	 TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_MAP, map_param_list, MAP_PARAMS, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_PLACEHOLDER, placeholder_param_list, PLACEHOLDER_PARAMS, 
	 TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_SKY, sky_param_list, SKY_PARAMS, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_GROUND, ground_param_list, GROUND_PARAMS, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_ORB, orb_param_list, ORB_PARAMS, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_AMBIENT_LIGHT, ambient_light_param_list, AMBIENT_LIGHT_PARAMS, 
	 TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_AMBIENT_SOUND, ambient_sound_param_list, AMBIENT_SOUND_PARAMS, 
	 TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_DEBUG, NULL, 0, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_ONLOAD, onload_param_list, ONLOAD_PARAMS, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_STREAM, stream_param_list, STREAM_PARAMS, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_NONE}
};

// Action tag list.

static tag action_tag_list[] = {
	{TOKEN_REPLACE, replace_param_list, REPLACE_PARAMS, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_EXIT, exit_param_list, ACTION_EXIT_PARAMS, TOKEN_CLOSE_SINGLE_TAG},
//	{TOKEN_SET_STREAM, set_stream_param_list, SET_STREAM_PARAMS,
//	 TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_NONE}
};

// Create tag list for structural blocks.

static tag structural_create_tag_list[] = {
	{TOKEN_PART, create_part_param_list, CREATE_PART_PARAMS, 
	 TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_PARAM, param_param_list, PARAM_PARAMS, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_POINT_LIGHT, point_light_param_list, POINT_LIGHT_PARAMS - 1,
	 TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_SPOT_LIGHT, spot_light_param_list, SPOT_LIGHT_PARAMS - 1, 
	 TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_SOUND, sound_param_list, SOUND_PARAMS - 1, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_POPUP, popup_param_list, POPUP_PARAMS - 1, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_ENTRANCE, entrance_param_list, ENTRANCE_PARAMS - 1, 
	 TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_EXIT, exit_param_list, EXIT_PARAMS - 1, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_ACTION, action_param_list, ACTION_PARAMS - 1, TOKEN_CLOSE_TAG},
	{TOKEN_NONE}
};

// Create tag list for sprites.

static tag sprite_create_tag_list[] = {
	{TOKEN_PART, create_part_param_list, PART_PARAMS, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_PARAM, sprite_param_param_list, SPRITE_PARAM_PARAMS, 
	 TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_POINT_LIGHT, point_light_param_list, POINT_LIGHT_PARAMS - 1, 
	 TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_SPOT_LIGHT, spot_light_param_list, SPOT_LIGHT_PARAMS - 1, 
	 TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_SOUND, sound_param_list, SOUND_PARAMS - 1, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_POPUP, popup_param_list, POPUP_PARAMS - 1, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_ENTRANCE, entrance_param_list, ENTRANCE_PARAMS - 1, 
	 TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_EXIT, exit_param_list, EXIT_PARAMS - 1, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_ACTION, action_param_list, ACTION_PARAMS - 1, TOKEN_CLOSE_TAG},
	{TOKEN_NONE}
};

// Imagemap tag list.

static tag imagemap_tag_list[] = {
	{TOKEN_AREA, area_param_list, AREA_PARAMS, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_ACTION, imagemap_action_param_list, IMAGEMAP_ACTION_PARAMS,
	 TOKEN_CLOSE_TAG},
	{TOKEN_NONE}
};

// Body tag list.

static tag body_tag_list[] = {
	{TOKEN_CREATE, create_param_list, CREATE_PARAMS, TOKEN_CLOSE_TAG},
	{TOKEN_PLAYER, player_param_list, PLAYER_PARAMS, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_ENTRANCE, entrance_param_list, ENTRANCE_PARAMS, 
	 TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_EXIT, exit_param_list, EXIT_PARAMS, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_IMAGEMAP, imagemap_param_list, IMAGEMAP_PARAMS, TOKEN_CLOSE_TAG},
	{TOKEN_POPUP, popup_param_list, POPUP_PARAMS, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_POINT_LIGHT, point_light_param_list, POINT_LIGHT_PARAMS, 
	 TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_SPOT_LIGHT, spot_light_param_list, SPOT_LIGHT_PARAMS, 
	 TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_SOUND, sound_param_list, SOUND_PARAMS, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_LEVEL, level_param_list, LEVEL_PARAMS, TOKEN_CLOSE_TAG},
	{TOKEN_ACTION, action_param_list, ACTION_PARAMS, TOKEN_CLOSE_TAG},
	{TOKEN_LOAD, load_param_list, LOAD_PARAMS, TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_NONE}
};

// Spot tag list.

static tag spot_tag_list[] = {
	{TOKEN_HEAD, NULL, 0, TOKEN_CLOSE_TAG},
	{TOKEN_BODY, NULL, 0, TOKEN_CLOSE_TAG},
	{TOKEN_NONE}
};

// Cache tag list.

static tag cache_tag_list[] = {
	{TOKEN_BLOCKSET, cached_blockset_param_list, CACHED_BLOCKSET_PARAMS,
		TOKEN_CLOSE_SINGLE_TAG},
	{TOKEN_NONE}
};