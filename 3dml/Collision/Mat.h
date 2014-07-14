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

#ifndef	_MAT_H
#define	_MAT_H

/*--------------------------------------------------------------

	Defines

--------------------------------------------------------------*/


/*--------------------------------------------------------------

	Structures & Classes

--------------------------------------------------------------*/

struct	MAT33
{
	float		Xx;
	float		Xy;
	float		Xz;

	float		Yx;
	float		Yy;
	float		Yz;

	float		Zx;
	float		Zy;
	float		Zz;
};


/*--------------------------------------------------------------

	Prototypes

--------------------------------------------------------------*/

void	MAT_setIdentity(MAT33 *mat_p);
void	MAT_mulInv(MAT33 *mat1_p, MAT33 *mat2_p, MAT33 *destMat_p);
void	MAT_mul(MAT33 *mat1_p, MAT33 *mat2_p, MAT33 *destMat_p);
void	MAT_setRot(MAT33 *mat_p, float az, float el, float tw);
void	MAT_copyInv(MAT33 *mat_p, MAT33 *destMat_p);


/*--------------------------------------------------------------

	Externs

--------------------------------------------------------------*/


#endif