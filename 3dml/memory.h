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

#ifdef TRACE

// Macros for allocating and freeing memory.

#define NEW(ptr, type) \
{ \
	trace_new(sizeof(type), #type); \
	ptr = new type; \
}
#define NEWARRAY(ptr, type, elements) \
{ \
	trace_newarray(sizeof(type), elements, #type); \
	ptr = new type[elements]; \
}
#define DEL(ptr, type) \
{ \
	trace_del(sizeof(type), #type); \
	delete ptr; \
}
#define DELARRAY(ptr, type, elements) \
{ \
	trace_delarray(sizeof(type), elements, #type); \
	delete []ptr; \
}

#else

#define NEW(ptr, type)					ptr = new type
#define NEWARRAY(ptr, type, elements)	ptr = new type[elements]
#define DEL(ptr, type)					delete ptr
#define DELARRAY(ptr, type, elements)	delete []ptr

#endif

#ifdef TRACE

// Functions for tracing memory allocations and frees.

void
start_trace();

void
trace_new(int bytes, char *type);

void
trace_newarray(int bytes, int elements, char *type);

void
trace_del(int bytes, char *type);

void
trace_delarray(int bytes, int elements, char *type);

void
end_trace();

#endif

// Functions for managing free spans.

void
init_free_span_list(void);

void
delete_free_span_list(void);

span *
new_span(void);

span *
dup_span(span *old_span_ptr);

span *
del_span(span *span_ptr);

// Functions for managing screen polygons.

void
init_screen_polygon_list(void);

void
set_max_screen_points(int set_spoints);

void
delete_screen_polygon_list(void);

void
reset_screen_polygon_list(void);

spolygon *
get_next_screen_polygon(void);

// Functions for managing free triggers.

void
init_free_trigger_list(void);

void
delete_free_trigger_list(void);

trigger *
new_trigger(void);

trigger *
dup_trigger(trigger *old_trigger_ptr);

trigger *
del_trigger(trigger *trigger_ptr);

// Functions for managing free hyperlinks.

void
init_free_hyperlink_list(void);

void
delete_free_hyperlink_list(void);

hyperlink *
new_hyperlink(void);

hyperlink *
dup_hyperlink(hyperlink *old_hyperlink_ptr);

hyperlink *
del_hyperlink(hyperlink *hyperlink_ptr);

// Functions for managing free lights.

void
init_free_light_list(void);

void
delete_free_light_list(void);

light *
new_light(void);

light *
dup_light(light *old_light_ptr);

light *
del_light(light *light_ptr);

// Functions for managing free sounds.

void
init_free_sound_list(void);

void
delete_free_sound_list(void);

sound *
new_sound(void);

sound *
dup_sound(sound *old_sound_ptr);

sound *
del_sound(sound *sound_ptr);

// Functions for managing free locations.

void
init_free_location_list(void);

void
delete_free_location_list(void);

location *
new_location(void);

location *
del_location(location *location_ptr);

// Functions for managing free entrances.

void
init_free_entrance_list(void);

void
delete_free_entrance_list(void);

entrance *
new_entrance(void);

entrance *
del_entrance(entrance *entrance_ptr);

// Functions for managing free popups.

void
init_free_popup_list(void);

void
delete_free_popup_list(void);

popup *
new_popup(void);

popup *
dup_popup(popup *old_popup_ptr);

popup *
del_popup(popup *popup_ptr);
