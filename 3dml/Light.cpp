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
// Contributor(s): Matt Wilkinson, Philip Stephens.
//******************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include "Classes.h"
#include "Fileio.h"
#include "Light.h"
#include "Main.h"
#include "Parser.h"
#include "Platform.h"
#include "Plugin.h"
#include "Spans.h"
#include "Utils.h"

// Some useful constants.

#define MAX_CLOSEST_LIGHTS	3
#define	ONE_OVER_THREE	0.3333333333f

// Ambient colour, and master intensity and colour.

static float ambient_red, ambient_green, ambient_blue;
static float master_intensity;
static float master_red, master_green, master_blue;

// Array of closest lights, to be populated before each block is rendered.

static int closest_lights;	
static light *closest_light_list[MAX_CLOSEST_LIGHTS];
static float distance_list[MAX_CLOSEST_LIGHTS];

//------------------------------------------------------------------------------
// Set the ambient light.
//------------------------------------------------------------------------------

void
set_ambient_light(float brightness, RGBcolour colour)
{
	ambient_red = colour.red * brightness;
	ambient_green = colour.green * brightness;
	ambient_blue = colour.blue * brightness;
}

//------------------------------------------------------------------------------
// Set the master intensity and colour (brightness must be between -1 and 1).
//------------------------------------------------------------------------------

void
set_master_intensity(float brightness)
{
	master_intensity = brightness;
	master_red = 255.0f * brightness;
	master_green = 255.0f * brightness;
	master_blue = 255.0f * brightness;
}

//------------------------------------------------------------------------------
// Find the closest lights to the specified vertex.
//------------------------------------------------------------------------------

void
find_closest_lights(vertex *vertex_ptr)
{
	light *light_ptr;
	int index;
	int	j, k;
	bool done;
	float dx, dy, dz, distance;

	// If there are the same or less lights in the world as have been requested,
	// just return all available lights.

	if (global_lights <= MAX_CLOSEST_LIGHTS) {
		light_ptr = global_light_list;
		for (index = 0; index < global_lights; index++) {
			closest_light_list[index] = light_ptr;
			light_ptr = light_ptr->next_light_ptr;
		}
		closest_lights = global_lights;
		return;
	}

	// Initialise the closest light list, and the distance list.

	for (index = 0; index < MAX_CLOSEST_LIGHTS; index++) {
		closest_light_list[index] = NULL;
		distance_list[index] = -1.0f;
	}

	// Search through the global list of lights and remember the three closest.

	closest_lights = MAX_CLOSEST_LIGHTS;
	light_ptr = global_light_list;
	for (index = 0; index < global_lights; index++) {

		// Compute the distance from the light to the vertex.

		dx = vertex_ptr->x - light_ptr->pos.x;
		dy = vertex_ptr->y - light_ptr->pos.y;
		dz = vertex_ptr->z - light_ptr->pos.z;
		distance = (dx * dx) + (dy * dy) + (dz * dz);

		// If this light is closer than any in the list, insert it into the
		// list.  The list is kept sorted from nearest to furthest.

		j = 0;
		done = false;
		while (j < MAX_CLOSEST_LIGHTS && !done) {
			if (distance_list[j] < 0.0 || distance < distance_list[j]) {
				if (j < MAX_CLOSEST_LIGHTS - 1) {
					for (k = MAX_CLOSEST_LIGHTS - 1; k > j; k--) {
						distance_list[k] = distance_list[k - 1];
						closest_light_list[k] = closest_light_list[k - 1];
					}
				}
				distance_list[j] = distance;
				closest_light_list[j] = light_ptr;
				done = true;
			}
			j++;
		}

		// Check the next light.

		light_ptr = light_ptr->next_light_ptr;
	}
}

//------------------------------------------------------------------------------
// Compute the lit colour of a vertex.
//------------------------------------------------------------------------------

void
compute_vertex_colour(vertex *vertex_ptr, vector *normal_ptr, 
					  RGBcolour *colour_ptr)
{
	int index;
	light *light_ptr;
	vector light_vector;
	float red, green, blue;
	float dot, dot2, distance;
	float intensity;

	// Initialise the additive polygon colour to be the ambient colour.

	red = ambient_red;
	green = ambient_green;
	blue = ambient_blue;

	// If an orb light has been defined, add it to the polygon colour.
	// The intensity of a directional light is based upon the cosine of
	// the angle between the light ray and the normal vector of the
	// polygon.  If the polygon is facing away from the light, it is
	// not affected.

	if (orb_light_ptr) {
		dot = -(orb_light_ptr->dir & *normal_ptr);
		if (FGT(dot, 0.0)) {
			red += orb_light_ptr->lit_colour.red * dot;
			green += orb_light_ptr->lit_colour.green * dot;
			blue += orb_light_ptr->lit_colour.blue * dot;
		}
	}

	// Now step through the list of closest lights.

	for (index = 0; index < closest_lights; index++) {
 		light_ptr = closest_light_list[index];

		// Work out the light intensity based upon it's style.

		switch(light_ptr->style) {
		case STATIC_POINT_LIGHT:
		case PULSATING_POINT_LIGHT:

			// The intensity of a point light is based upon the cosine of the
			// angle between the light ray and the normal vector of the polygon,
			// multiplied by the fraction of the distance to the light's
			// radius if it's not a flood light. If the polygon is facing away
			// from the light, it is not affected.

			light_vector = light_ptr->pos - *vertex_ptr;
			distance = light_vector.length();
			dot = light_vector & *normal_ptr;
			if (distance <= light_ptr->radius && dot > 0.0f) {
				if (light_ptr->flood)
					intensity = light_ptr->intensity;
				else
					intensity = light_ptr->intensity * dot *
						((light_ptr->radius - distance) * 
						light_ptr->one_on_radius);
				red += light_ptr->colour.red * intensity;
				green += light_ptr->colour.green * intensity;
				blue += light_ptr->colour.blue * intensity;
			}
			break;

		case STATIC_SPOT_LIGHT:
		case REVOLVING_SPOT_LIGHT:
		case SEARCHING_SPOT_LIGHT:

			// The intensity of a spot light is a combination of the cosine of
			// the angle between the light ray and the cone, and the light ray
			// and the normal vector of the polygon, multiplied by the fraction
			// of the distance to the light's radius if it's not a flood light.
			// If the polygon is facing away from the light, it is not affected.

			light_vector = -(light_ptr->pos - *vertex_ptr);
			distance = light_vector.length();
			dot = light_vector & light_ptr->dir;
			dot2 = -(light_vector & *normal_ptr);
			if (distance <= light_ptr->radius && dot2 > 0.0f &&
				dot > light_ptr->cos_cone_angle) {
				if (light_ptr->flood)
					intensity = light_ptr->intensity;
				else 
					intensity = light_ptr->intensity * dot2 *
						((dot - light_ptr->cos_cone_angle) *
						light_ptr->cone_angle_M) * 
						((light_ptr->radius - distance) * 
						light_ptr->one_on_radius);
				red += light_ptr->colour.red * intensity;
				green += light_ptr->colour.green * intensity;
				blue += light_ptr->colour.blue * intensity;
			}
			break;
		}
	}

	// Adjust the final vertex colour by the master intensity, making sure the
	// component values stay within 0 and 255.

	red += master_red;
	if (FLT(red, 0.0f))
		red = 0.0f;
	else if (FGT(red, 255.0f))
		red = 255.0f;
	green += master_green;
	if (FLT(green, 0.0f))
		green = 0.0f;
	else if (FGT(green, 255.0f))
		green = 255.0f;
	blue += master_blue;
	if (FLT(blue, 0.0f))
		blue = 0.0f;
	else if (FGT(blue, 255.0f))
		blue = 255.0f;

	// Set the final vertex colour.

	colour_ptr->red = red;
	colour_ptr->green = green;
	colour_ptr->blue = blue;
}

//------------------------------------------------------------------------------
// Compute the brightness of a vertex.
//------------------------------------------------------------------------------

float
compute_vertex_brightness(vertex *vertex_ptr, vector *normal_ptr)
{
	RGBcolour vertex_colour;

	compute_vertex_colour(vertex_ptr, normal_ptr, &vertex_colour);
	return(((vertex_colour.red + vertex_colour.blue + 
		vertex_colour.green) * ONE_OVER_THREE) / 255.0f);
}
