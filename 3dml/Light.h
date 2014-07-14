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

// Externally visible functions.

void
set_ambient_light(float brightness, RGBcolour colour);

void
set_master_intensity(float brightness);

void
find_closest_lights(vertex *vertex_ptr);

void
compute_vertex_colour(vertex *vertex_ptr, vector *normal_ptr, 
					  RGBcolour *colour_ptr);

float
compute_vertex_brightness(vertex *vertex_ptr, vector *normal_ptr);
