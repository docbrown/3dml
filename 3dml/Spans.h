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

// Dimensions of each image size.

extern int image_dimensions_list[IMAGE_SIZES];

// Cache stats.

extern int cache_entries_added_in_frame;
extern int cache_entries_reused_in_frame;
extern int cache_entries_free_in_frame;

// Externally visible functions.

cache_entry *
get_cache_entry(pixmap *pixmap_ptr, int brightness_index);

bool
create_image_caches(void);

void
purge_image_caches(int delta_frames);

void
delete_image_caches(void);

int
get_size_index(int texture_width, int texture_height);

void
set_size_indices(texture *texture_ptr);

bool
add_span(int sy, edge *left_edge_ptr, edge *right_edge_ptr, pixmap *pixmap_ptr,
		 pixel colour_pixel, int brightness_index, bool is_popup);

void
add_movable_span(int sy, edge *left_edge_ptr, edge *right_edge_ptr, 
				 pixmap *pixmap_ptr, pixel colour_pixel, int brightness_index);
