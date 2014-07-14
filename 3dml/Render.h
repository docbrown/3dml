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

extern float one_on_dimensions_list[IMAGE_SIZES];

// Externally visible functions.

void 
init_renderer(void);

void
set_up_renderer(void);

void 
clean_up_renderer(void);

void
init_screen_polygon_list(void);

void
delete_screen_polygon_list(void);

void
translate_vertex(vertex *old_vertex_ptr, vertex *new_vertex_ptr);

void
rotate_vertex(vertex *old_vertex_ptr, vertex *new_vertex_ptr);

void
transform_vertex(vertex *old_vertex_ptr, vertex *new_vertex_ptr);

void 
render_frame(void);
