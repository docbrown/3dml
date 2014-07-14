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
// Contributor(s): Matt Wilkinson.
//******************************************************************************

#ifndef	_VEC_H
#define	_VEC_H

/*--------------------------------------------------------------

	Includes

--------------------------------------------------------------*/

#include "mat.h"


/*--------------------------------------------------------------

	Defines

--------------------------------------------------------------*/


/*--------------------------------------------------------------

	Macros

--------------------------------------------------------------*/

	//------ This converts the signs of the vector elements ----
	//------ into a numerical value ----------------------------

#define VEC3_SIGNS(vecX, vecY, vecZ) ((BNEG(vecX)<<2) + (BNEG(vecY)<<1) + BNEG(vecZ));


/*--------------------------------------------------------------

	Structures & Classes

--------------------------------------------------------------*/

struct	VEC3
{
	float		x;
	float		y;
	float		z;
};


/*--------------------------------------------------------------

	Prototypes

--------------------------------------------------------------*/

void	VEC_set(VEC3 *vec_p, float x, float y, float z);
void	VEC_sub(VEC3 *vec1, VEC3 *vec2, VEC3 *vec3);
void	VEC_add(VEC3 *vec1, VEC3 *vec2, VEC3 *vec3);
void	VEC_mulMat(VEC3 *vec_p, MAT33 *mat_p, VEC3 *destVec_p);
void 	VEC_mulMatInv(VEC3 *vec_p, MAT33 *mat_p, VEC3 *destVec_p);
float	VEC_dot(VEC3 *vec1_p, VEC3 *vec2_p);
void	VEC_normalise(VEC3 *vec_p);
float	VEC_length(VEC3 *vec_p);


/*--------------------------------------------------------------

	Externs

--------------------------------------------------------------*/


#endif
