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

// Tokens.

enum token {

	// No token.

	TOKEN_NONE = 0,

	// Symbol tokens.

	TOKEN_OPEN_TAG = 1,	
	TOKEN_CLOSE_TAG,
	TOKEN_OPEN_END_TAG,
	TOKEN_CLOSE_SINGLE_TAG,
	TOKEN_OPEN_COMMENT,	
	TOKEN_CLOSE_COMMENT,
	TOKEN_EQUAL,

	// Tag and parameter name tokens.

	TOKEN_ALIGN = 8,
	TOKEN_ANGLE,
	TOKEN_BLOCK,
	TOKEN_BRIGHTNESS,
	TOKEN_BSP_TREE,
	TOKEN_COLOUR,
	TOKEN_CONE,
	TOKEN_COORDS,
	TOKEN_DELAY,
	TOKEN_DIRECTION,
	TOKEN_DOUBLE,
	TOKEN_ENTRANCE,
	TOKEN_EXIT,
	TOKEN_FACES,
	TOKEN_FILE,
	TOKEN_FLOOD,
	TOKEN_FRONT,
	TOKEN_GROUND,
	TOKEN_HREF,
	TOKEN_NAME,
	TOKEN_ORB,
	TOKEN_ORIENT,
	TOKEN_PARAM,
	TOKEN_PART,
	TOKEN_PARTS,
	TOKEN_PLACEHOLDER,
	TOKEN_PLAYBACK,
	TOKEN_POINT_LIGHT,
	TOKEN_POLYGON,
	TOKEN_POLYGONS,
	TOKEN_POSITION,
	TOKEN_RADIUS,
	TOKEN_REAR,
	TOKEN_ROLLOFF,
	TOKEN_ROOT,
	TOKEN_REF,
	TOKEN_SKY,
	TOKEN_SOLID,
	TOKEN_SOUND,
	TOKEN_SPEED,
	TOKEN_SPOT_LIGHT,
	TOKEN_SPRITE,
	TOKEN_STYLE,
	TOKEN_SYMBOL,
	TOKEN_SYNOPSIS,
	TOKEN_TARGET,
	TOKEN_TEXCOORDS,
	TOKEN_TEXT,
	TOKEN_TEXTURE,
	TOKEN_TRANSLUCENCY,
	TOKEN_TRIGGER,
	TOKEN__TYPE,
	TOKEN_VERSION,
	TOKEN_VERTEX,
	TOKEN_VERTICES,
	TOKEN_VOLUME,

	// Value type tokens.

	VALUE_BLOCK_NAME = 128,
	VALUE_BLOCK_TYPE,
	VALUE_BOOL,
	VALUE_CHAR,
	VALUE_DEGREES,
	VALUE_DOUBLE,
	VALUE_INTEGER,
	VALUE_PART_NAME,
	VALUE_PERCENTAGE,
	VALUE_REF_LIST,
	VALUE_RGB,
	VALUE_STRING,
	VALUE_TEXCOORDS_LIST,
	VALUE_TEXTURE_STYLE,
	VALUE_VERTEX_COORDS,

	// Unknown token.

	TOKEN_UNKNOWN = 255
};
