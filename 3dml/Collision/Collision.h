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
// Contributor(s): Matt Wilkinson, Philip Stephens
//******************************************************************************

/********************************************************************

	DESCRIPTION :	Header file

	AUTHOR		:	Matt Wilkinson

********************************************************************/


#ifndef _COLLISION_H
#define _COLLISION_H

/*--------------------------------------------------------------

	Includes

--------------------------------------------------------------*/

#include "col.h"
struct block;
struct block_def;


/*--------------------------------------------------------------

	Defines

--------------------------------------------------------------*/


/*--------------------------------------------------------------

	Structures & Classes

--------------------------------------------------------------*/


/*--------------------------------------------------------------

	Prototypes

--------------------------------------------------------------*/

bool
COL_createBlockColMesh(block *block_ptr, block_def *block_def_ptr);

void 
COL_convertBlockToColMesh(COL_MESH *mesh_ptr, block_def *block_def_ptr);

bool
COL_createSpriteColMesh(block *block_ptr);

void 
COL_convertSpriteToColMesh(COL_MESH *mesh_ptr, float minX, float minY, 
						   float minZ, float maxX, float maxY, float maxZ);

/*--------------------------------------------------------------

	Externs

--------------------------------------------------------------*/

#endif

