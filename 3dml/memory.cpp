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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Classes.h"
#include "Memory.h"
#include "Parser.h"
#include "Platform.h"
#include "Plugin.h"

#ifdef TRACE

// Flag indicating whether memory tracing is active.

static bool trace_on = false;

// Count of the number of bytes currently allocated via NEW and NEWARRAY.

static unsigned int bytes_allocated;
static unsigned int bytes_freed;

#ifdef VERBOSE

// List of object types that are being traced.

struct object {
	char type[32];
	int bytes_allocated;
	int bytes_freed;
};

#define MAX_OBJECTS	64
static int objects;
static object object_list[MAX_OBJECTS];

#endif // VERBOSE
#endif // TRACE

// Linked list of free spans.

static span *free_span_list;

// Screen polygon list, the last screen polygon in the list, and the
// current screen polygon.

static int max_spoints;
static spolygon *spolygon_list;
static spolygon *last_spolygon_ptr;
static spolygon *curr_spolygon_ptr;

// Linked list of free triggers.

static trigger *free_trigger_list;

// Linked list of free hyperlinks.

static hyperlink *free_hyperlink_list;

// Linked list of free lights.

static light *free_light_list;

// Linked list of free sounds.

static sound *free_sound_list;

// Linked list of free locations.

static location *free_location_list;

// Linked list of free entrances.

static entrance *free_entrance_list;

// Linked list of free popups.

static popup *free_popup_list;

#ifdef TRACE

//------------------------------------------------------------------------------
// Memory tracing functions.
//------------------------------------------------------------------------------

void
start_trace(void)
{
	bytes_allocated = 0;
	bytes_freed = 0;
#ifdef VERBOSE
	objects = 0;
#endif
	trace_on = true;
}

#ifdef VERBOSE
static object *
find_object(char *type)
{
	for (int index = 0; index < objects; index++) {
		if (!stricmp(object_list[index].type, type))
			break;
	}
	if (index == objects) {
		if (objects == MAX_OBJECTS)
			return(NULL);
		objects++;
		strcpy(object_list[index].type, type);
		object_list[index].bytes_allocated = 0;
		object_list[index].bytes_freed = 0;
	}
	return(&object_list[index]);
}
#endif

void
trace_new(int bytes, char *type)
{
	if (trace_on) {
		bytes_allocated += bytes;
#ifdef VERBOSE
		object *object_ptr;
		if ((object_ptr = find_object(type)) != NULL)
			object_ptr->bytes_allocated += bytes;
#endif
	}
}

void
trace_newarray(int bytes, int elements, char *type)
{
	if (trace_on) {
		bytes_allocated += bytes * elements;
#ifdef VERBOSE
		object *object_ptr;
		if ((object_ptr = find_object(type)) != NULL)
			object_ptr->bytes_allocated += bytes * elements;
#endif
	}
}

void
trace_del(int bytes, char *type)
{
	if (trace_on) {
		bytes_freed += bytes;
#ifdef VERBOSE
		object *object_ptr;
		if ((object_ptr = find_object(type)) != NULL)
			object_ptr->bytes_freed += bytes;
#endif
	}
}

void
trace_delarray(int bytes, int elements, char *type)
{
	if (trace_on) {
		bytes_freed += bytes * elements;
#ifdef VERBOSE
		object *object_ptr;
		if ((object_ptr = find_object(type)) != NULL)
			object_ptr->bytes_freed += bytes * elements;
#endif
	}
}

void
end_trace(void)
{
	diagnose("Allocated %d bytes", bytes_allocated);
	if (bytes_allocated > bytes_freed)
		diagnose("*** Memory leak: %d bytes are still allocated", 
			bytes_allocated - bytes_freed);
	else if (bytes_allocated < bytes_freed)
		diagnose("*** Reverse memory leak: %d bytes freed were not allocated",
			bytes_freed - bytes_allocated);
#ifdef VERBOSE
	for (int index = 0; index < objects; index++) {
		object *object_ptr = &object_list[index];
		diagnose("Allocated %d bytes to objects of type %s", 
			object_ptr->bytes_allocated, object_ptr->type);
		if (object_ptr->bytes_allocated > object_ptr->bytes_freed)
			diagnose("*** Memory leak for objects of type %s: "
			"%d bytes are still allocated", object_ptr->type, 
			object_ptr->bytes_allocated - object_ptr->bytes_freed);
		else if (object_ptr->bytes_allocated < object_ptr->bytes_freed)
			diagnose("*** Reverse memory leak for objects of type %s: "
				"%d bytes freed were not allocated", object_ptr->type, 
				object_ptr->bytes_freed - object_ptr->bytes_allocated);
	}
#endif
	trace_on = false;
}

#endif // TRACE

//------------------------------------------------------------------------------
// Free span list management.
//------------------------------------------------------------------------------

// Initialise the free span list.

void
init_free_span_list(void)
{
	free_span_list = NULL;
}

// Delete the free span list.

void
delete_free_span_list(void)
{
	span *next_span_ptr;

	while (free_span_list) {
		next_span_ptr = free_span_list->next_span_ptr;
		DEL(free_span_list, span);
		free_span_list = next_span_ptr;
	}
}

// Return a pointer the next free span, or NULL if we are out of memory.

span *
new_span(void)
{
	span *span_ptr;

	span_ptr = free_span_list;
	if (span_ptr)
		free_span_list = span_ptr->next_span_ptr;
	else
		NEW(span_ptr, span);
	return(span_ptr);
}

// Return a pointer the next free span, after initialising it with the old
// span data.

span *
dup_span(span *old_span_ptr)
{
	span *span_ptr;

	span_ptr = new_span();
	if (span_ptr)
		*span_ptr = *old_span_ptr;
	return(span_ptr);
}

// Add the span to the head of the free span list, and return a pointer to the
// next span.

span *
del_span(span *span_ptr)
{
	span *next_span_ptr = span_ptr->next_span_ptr;
	span_ptr->next_span_ptr = free_span_list;
	free_span_list = span_ptr;
	return(next_span_ptr);
}

//------------------------------------------------------------------------------
// Screen polygon list management.
//------------------------------------------------------------------------------

// Initialise the screen polygon list.

void
init_screen_polygon_list(void)
{
	spolygon_list = NULL;
	last_spolygon_ptr = NULL;
}

// Set the maximum number of screen points allowed per screen polygon.

void
set_max_screen_points(int set_spoints)
{
	max_spoints = set_spoints;
}

// Delete the screen polygon list.

void
delete_screen_polygon_list(void)
{
	spolygon *next_spolygon_ptr;

	while (spolygon_list) {
		next_spolygon_ptr = spolygon_list->next_spolygon_ptr;
		DEL(spolygon_list, spolygon);
		spolygon_list = next_spolygon_ptr;
	}
}

void
reset_screen_polygon_list(void)
{
	curr_spolygon_ptr = spolygon_list;
}

// Return a pointer to the next screen polygon, or create a new one.

spolygon *
get_next_screen_polygon(void)
{
	// If hardware acceleration is enabled...

	if (hardware_acceleration) {
		spolygon *spolygon_ptr;

		// If we have not come to the end of the screen polygon list, get the
		// pointer to the current screen polygon.

		if (curr_spolygon_ptr) {
			spolygon_ptr = curr_spolygon_ptr;
			curr_spolygon_ptr = curr_spolygon_ptr->next_spolygon_ptr;
		} else {

			// Otherwise create a new screen polygon.

			NEW(spolygon_ptr, spolygon);
			if (spolygon_ptr != NULL &&
				!spolygon_ptr->create_spoint_list(max_spoints)) {
				DEL(spolygon_ptr, spolygon);
				spolygon_ptr = NULL;
			}

			// If the screen polygon was created, add it to the end of the
			// screen polygon list.

			if (spolygon_ptr) {
				if (last_spolygon_ptr)
					last_spolygon_ptr->next_spolygon_ptr = spolygon_ptr;
				else
					spolygon_list = spolygon_ptr;
				last_spolygon_ptr = spolygon_ptr;
			}
		}

		// Return the pointer to the screen polygon.

		return(spolygon_ptr);
	}

	// If hardware acceleration is not enabled, simply return the same single
	// screen polygon, creating it first if it doesn't yet exist.

	else {
		if (spolygon_list == NULL) {
			NEW(spolygon_list, spolygon);
			if (spolygon_list != NULL &&
				!spolygon_list->create_spoint_list(max_spoints)) {
				DEL(spolygon_list, spolygon);
				spolygon_list = NULL;
			}
		}
		return(spolygon_list);
	}
}

//------------------------------------------------------------------------------
// Free trigger list management.
//------------------------------------------------------------------------------

// Initialise the free trigger list.

void
init_free_trigger_list(void)
{
	free_trigger_list = NULL;
}

// Delete the free trigger list.

void
delete_free_trigger_list(void)
{
	trigger *next_trigger_ptr;

	while (free_trigger_list) {
		next_trigger_ptr = free_trigger_list->next_trigger_ptr;
		free_trigger_list->action_list = NULL;
		DEL(free_trigger_list, trigger);
		free_trigger_list = next_trigger_ptr;
	}
}

// Return a pointer to the next free trigger, or NULL if we are out of memory.

trigger *
new_trigger(void)
{
	trigger *trigger_ptr;

	trigger_ptr = free_trigger_list;
	if (trigger_ptr)
		free_trigger_list = trigger_ptr->next_trigger_ptr;
	else
		NEW(trigger_ptr, trigger);
	return(trigger_ptr);
}

// Return a pointer to the next free trigger, after initialising it with the
// old trigger data.

trigger *
dup_trigger(trigger *old_trigger_ptr)
{
	trigger *trigger_ptr;

	trigger_ptr = new_trigger();
	if (trigger_ptr)
		*trigger_ptr = *old_trigger_ptr;
	return(trigger_ptr);
}

// Add the trigger to the head of the free trigger list, and return a pointer
// to the next trigger.

trigger *
del_trigger(trigger *trigger_ptr)
{
	trigger *next_trigger_ptr = trigger_ptr->next_trigger_ptr;
	trigger_ptr->next_trigger_ptr = free_trigger_list;
	free_trigger_list = trigger_ptr;
	return(next_trigger_ptr);
}

//------------------------------------------------------------------------------
// Free hyperlink list management.
//------------------------------------------------------------------------------

// Initialise the free hyperlink list.

void
init_free_hyperlink_list(void)
{
	free_hyperlink_list = NULL;
}

// Delete the free hyperlink list.

void
delete_free_hyperlink_list(void)
{
	hyperlink *next_hyperlink_ptr;

	while (free_hyperlink_list) {
		next_hyperlink_ptr = free_hyperlink_list->next_hyperlink_ptr;
		DEL(free_hyperlink_list, hyperlink);
		free_hyperlink_list = next_hyperlink_ptr;
	}
}

// Return a pointer to the next free hyperlink, or NULL if we are out of memory.

hyperlink *
new_hyperlink(void)
{
	hyperlink *hyperlink_ptr;

	hyperlink_ptr = free_hyperlink_list;
	if (hyperlink_ptr)
		free_hyperlink_list = hyperlink_ptr->next_hyperlink_ptr;
	else
		NEW(hyperlink_ptr, hyperlink);
	return(hyperlink_ptr);
}

// Return a pointer to the next free hyperlink, after initialising it with the
// old hyperlink data.

hyperlink *
dup_hyperlink(hyperlink *old_hyperlink_ptr)
{
	hyperlink *hyperlink_ptr;

	hyperlink_ptr = new_hyperlink();
	if (hyperlink_ptr)
		*hyperlink_ptr = *old_hyperlink_ptr;
	return(hyperlink_ptr);
}

// Add the hyperlink to the head of the free hyperlink list, and return a pointer
// to the next hyperlink.

hyperlink *
del_hyperlink(hyperlink *hyperlink_ptr)
{
	hyperlink *next_hyperlink_ptr = hyperlink_ptr->next_hyperlink_ptr;
	hyperlink_ptr->next_hyperlink_ptr = free_hyperlink_list;
	free_hyperlink_list = hyperlink_ptr;
	return(next_hyperlink_ptr);
}

//------------------------------------------------------------------------------
// Free light list management.
//------------------------------------------------------------------------------

// Initialise the free light list.

void
init_free_light_list(void)
{
	free_light_list = NULL;
}

// Delete the free light list.

void
delete_free_light_list(void)
{
	light *next_light_ptr;

	while (free_light_list) {
		next_light_ptr = free_light_list->next_light_ptr;
		DEL(free_light_list, light);
		free_light_list = next_light_ptr;
	}
}

// Return a pointer to the next free light, or NULL if we are out of memory.

light *
new_light(void)
{
	light *light_ptr;

	light_ptr = free_light_list;
	if (light_ptr)
		free_light_list = light_ptr->next_light_ptr;
	else
		NEW(light_ptr, light);
	return(light_ptr);
}

// Return a pointer to the next free light, after initialising it with the
// old light data.

light *
dup_light(light *old_light_ptr)
{
	light *light_ptr;

	light_ptr = new_light();
	if (light_ptr)
		*light_ptr = *old_light_ptr;
	return(light_ptr);
}

// Add the light to the head of the free light list, and return a pointer
// to the next light.

light *
del_light(light *light_ptr)
{
	light *next_light_ptr = light_ptr->next_light_ptr;
	light_ptr->next_light_ptr = free_light_list;
	free_light_list = light_ptr;
	return(next_light_ptr);
}

//------------------------------------------------------------------------------
// Free sound list management.
//------------------------------------------------------------------------------

// Initialise the free sound list.

void
init_free_sound_list(void)
{
	free_sound_list = NULL;
}

// Delete the free sound list.

void
delete_free_sound_list(void)
{
	sound *next_sound_ptr;

	while (free_sound_list) {
		next_sound_ptr = free_sound_list->next_sound_ptr;
		DEL(free_sound_list, sound);
		free_sound_list = next_sound_ptr;
	}
}

// Return a pointer to the next free sound, or NULL if we are out of memory.

sound *
new_sound(void)
{
	sound *sound_ptr;

	sound_ptr = free_sound_list;
	if (sound_ptr)
		free_sound_list = sound_ptr->next_sound_ptr;
	else
		NEW(sound_ptr, sound);
	return(sound_ptr);
}

// Return a pointer to the next free sound, after initialising it with the
// old sound data.

sound *
dup_sound(sound *old_sound_ptr)
{
	sound *sound_ptr;

	sound_ptr = new_sound();
	if (sound_ptr)
		*sound_ptr = *old_sound_ptr;
	return(sound_ptr);
}

// Add the sound to the head of the free sound list, and return a pointer
// to the next sound.

sound *
del_sound(sound *sound_ptr)
{
	sound *next_sound_ptr = sound_ptr->next_sound_ptr;
	sound_ptr->next_sound_ptr = free_sound_list;
	free_sound_list = sound_ptr;
	return(next_sound_ptr);
}

//------------------------------------------------------------------------------
// Free location list management.
//------------------------------------------------------------------------------

// Initialise the free location list.

void
init_free_location_list(void)
{
	free_location_list = NULL;
}

// Delete the free location list.

void
delete_free_location_list(void)
{
	location *next_location_ptr;

	while (free_location_list) {
		next_location_ptr = free_location_list->next_location_ptr;
		DEL(free_location_list, location);
		free_location_list = next_location_ptr;
	}
}

// Return a pointer to the next free location, or NULL if we are out of memory.

location *
new_location(void)
{
	location *location_ptr;

	location_ptr = free_location_list;
	if (location_ptr)
		free_location_list = location_ptr->next_location_ptr;
	else
		NEW(location_ptr, location);
	return(location_ptr);
}

// Add the location to the head of the free location list, and return a pointer
// to the next location.

location *
del_location(location *location_ptr)
{
	location *next_location_ptr = location_ptr->next_location_ptr;
	location_ptr->next_location_ptr = free_location_list;
	free_location_list = location_ptr;
	return(next_location_ptr);
}

//------------------------------------------------------------------------------
// Free entrance list management.
//------------------------------------------------------------------------------

// Initialise the free entrance list.

void
init_free_entrance_list(void)
{
	free_entrance_list = NULL;
}

// Delete the free entrance list.

void
delete_free_entrance_list(void)
{
	entrance *next_entrance_ptr;

	while (free_entrance_list) {
		next_entrance_ptr = free_entrance_list->next_entrance_ptr;
		DEL(free_entrance_list, entrance);
		free_entrance_list = next_entrance_ptr;
	}
}

// Return a pointer to the next free entrance, or NULL if we are out of memory.

entrance *
new_entrance(void)
{
	entrance *entrance_ptr;

	entrance_ptr = free_entrance_list;
	if (entrance_ptr)
		free_entrance_list = entrance_ptr->next_entrance_ptr;
	else
		NEW(entrance_ptr, entrance);
	return(entrance_ptr);
}

// Add the entrance to the head of the free entrance list, and return a pointer
// to the next entrance.

entrance *
del_entrance(entrance *entrance_ptr)
{
	entrance *next_entrance_ptr = entrance_ptr->next_entrance_ptr;
	entrance_ptr->next_entrance_ptr = free_entrance_list;
	free_entrance_list = entrance_ptr;
	return(next_entrance_ptr);
}

//------------------------------------------------------------------------------
// Free popup list management.
//------------------------------------------------------------------------------

// Initialise the free sound list.

void
init_free_popup_list(void)
{
	free_popup_list = NULL;
}

// Delete the free popup list.

void
delete_free_popup_list(void)
{
	popup *next_popup_ptr;

	while (free_popup_list) {
		next_popup_ptr = free_popup_list->next_popup_ptr;
		DEL(free_popup_list, popup);
		free_popup_list = next_popup_ptr;
	}
}

// Return a pointer to the next free popup, or NULL if we are out of memory.

popup *
new_popup(void)
{
	popup *popup_ptr;

	popup_ptr = free_popup_list;
	if (popup_ptr)
		free_popup_list = popup_ptr->next_popup_ptr;
	else
		NEW(popup_ptr, popup);
	return(popup_ptr);
}

// Return a pointer to the next free popup, after initialising it with the
// old popup data.

popup *
dup_popup(popup *old_popup_ptr)
{
	popup *popup_ptr;

	popup_ptr = new_popup();
	if (popup_ptr)
		*popup_ptr = *old_popup_ptr;
	return(popup_ptr);
}

// Add the popup to the head of the free popup list, and return a pointer
// to the next popup.

popup *
del_popup(popup *popup_ptr)
{
	popup *next_popup_ptr = popup_ptr->next_popup_ptr;
	popup_ptr->next_popup_ptr = free_popup_list;
	free_popup_list = popup_ptr;
	return(next_popup_ptr);
}