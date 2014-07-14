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

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <winuser.h>
#include <commdlg.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <direct.h>
#include <io.h>
#include "classes.h"
#include "parser.h"
#include "fileio.h"
#include "resource.h"

// File types.

#define BLOCKSET_FILETYPE	"Blockset (*.bset)\0*.bset\0"
#ifdef SUPPORT_DXF
#define BLOCK_FILETYPES	"Block file (*.block,*.dxf)\0*.block;*.dxf\0"
#else
#define BLOCK_FILETYPES		"Block file (*.block)\0*.block\0"
#endif
#define TEXTURE_FILETYPES	"Texture file (*.gif,*.jpg)\0*.gif;*.jpg\0"
#define SOUND_FILETYPE		"Sound file (*.wav)\0*.wav\0"

// Style object.

style *style_ptr;

// Selected block definition and part.

static block_def *selected_block_def_ptr;
static part *selected_part_ptr;

// Block definition being edited.

static block_def *edited_block_def_ptr;

// Flags indicating when a blockset is loaded, and if it has been changed.

static bool blockset_loaded;
bool blockset_changed;

// Data used in the creation of windows.

static HWND parent_window_handle;
static HWND curr_window_handle;
static bool window_has_menu;
static int window_x, min_window_x, max_window_x;
static int window_y, max_window_y;

// Instance handle.

static HINSTANCE instance_handle;

// Window handles.

static HWND style_window_handle;

static HWND style_tags_title_handle;
static HWND edit_version_button_handle;
static HWND edit_placeholder_button_handle;
static HWND edit_sky_button_handle;
static HWND edit_ground_button_handle;
static HWND edit_orb_button_handle;

static HWND block_list_title_handle;
static HWND block_list_view_handle;
static HWND add_block_button_handle;
static HWND remove_block_button_handle;
static HWND rename_block_button_handle;
static HWND edit_block_button_handle;
static HWND add_BSP_trees_button_handle;

static HWND texture_list_title_handle;
static HWND texture_list_box_handle;
static HWND add_texture_button_handle;
static HWND remove_texture_button_handle;
static HWND rename_texture_button_handle;

static HWND sound_list_title_handle;
static HWND sound_list_box_handle;
static HWND add_sound_button_handle;
static HWND remove_sound_button_handle;
static HWND rename_sound_button_handle;

static HWND block_window_handle;

static HWND part_list_title_handle;
static HWND part_list_box_handle;
static HWND edit_part_button_handle;

static HWND block_name_label_handle;
static HWND block_name_edit_box_handle;
static HWND block_symbol_label_handle;
static HWND block_symbol_edit_box_handle;
static HWND block_double_label_handle;
static HWND block_double_edit_box_handle;
static HWND block_entrance_label_handle;
static HWND block_entrance_edit_box_handle;

static HWND block_ok_button_handle;
static HWND block_cancel_button_handle;

static HWND block_tags_title_handle;
static HWND edit_param_button_handle;
static HWND edit_light_button_handle;
static HWND edit_sound_button_handle;
static HWND edit_exit_button_handle;

static HWND dialog_box_handle;
static HWND polygons_processed_handle;
static HWND polygons_added_handle;
static HWND current_polygon_handle;

// Directory paths.

static char app_dir[_MAX_PATH];
char blocksets_dir[_MAX_PATH];
char blocks_dir[_MAX_PATH];
char textures_dir[_MAX_PATH];
char sounds_dir[_MAX_PATH];
string temp_dir;
string temp_blocks_dir;
string temp_textures_dir;
string temp_sounds_dir;

// File paths.

static char user_file_path[_MAX_PATH];
static char config_file_path[_MAX_PATH];
static char log_file_path[_MAX_PATH];
char blockset_file_path[_MAX_PATH];

// Used during BSP tree creation.

static block *BSP_block_ptr;
static polygon *BSP_polygon_list;
static int polygons_processed;
static int polygons_added;
static int polygons_active;
static char text[80];

// Polygon comparison table.

#define MAX_POLYGONS	8192
static unsigned char poly_cmp_table[MAX_POLYGONS][MAX_POLYGONS / 4];

//==============================================================================
// Diagnostic functions.
//==============================================================================

//------------------------------------------------------------------------------
// Function to display a message in a message box.
//------------------------------------------------------------------------------

void
message(char *window_title, char *format, ...)
{
	va_list arg_ptr;
	char message[BUFSIZ];

	va_start(arg_ptr, format);
	vsprintf(message, format, arg_ptr);
	va_end(arg_ptr);
	MessageBox(NULL, message, window_title, MB_OK);
}

//------------------------------------------------------------------------------
// Function to write a diagnostic message to the log file.
//------------------------------------------------------------------------------

void
diagnose(char *format, ...)
{
	va_list arg_ptr;
	char message[BUFSIZ];
	FILE *fp;

	va_start(arg_ptr, format);
	vsprintf(message, format, arg_ptr);
	va_end(arg_ptr);
	if ((fp = fopen(log_file_path, "a")) != NULL) {
		fprintf(fp, "%s\n", message);
		fclose(fp);
	}
}

//==============================================================================
// Window creation functions.
//==============================================================================

//------------------------------------------------------------------------------
// Create a window.
//------------------------------------------------------------------------------

static HWND
create_window(char *window_class, char *window_title, DWORD window_style, 
			  DWORD extended_style, int x, int y, int width, int height,
			  HWND parent_window_handle)
{
	HWND window_handle;

	window_handle = CreateWindowEx(extended_style, window_class, window_title,
		window_style, x, y, width, height, parent_window_handle, NULL,
		instance_handle, NULL);
	return(window_handle);
}

//------------------------------------------------------------------------------
// Initialise the position within the given window.
//------------------------------------------------------------------------------

static void
first_window_pos(HWND parent_handle, HWND window_handle)
{
	parent_window_handle = parent_handle;
	curr_window_handle = window_handle;
	window_x = 0;
	window_y = 0;
	min_window_x = 0;
	max_window_x = 0;
	max_window_y = 0;
}

//------------------------------------------------------------------------------
// Move onto the next position in the current window.
//------------------------------------------------------------------------------

static void
next_window_pos(int width, int height, bool next_row)
{
	window_x += width;
	if (window_x > max_window_x)
		max_window_x = window_x;
	if (next_row) {
		window_x = min_window_x;
		window_y += height;
		if (window_y > max_window_y)
			max_window_y = window_y;
	}
}

//------------------------------------------------------------------------------
// Move onto a new column in the current window.
//------------------------------------------------------------------------------

static void
new_window_col(int x)
{
	window_x = x;
	window_y = 0;
	min_window_x = x;
}

//------------------------------------------------------------------------------
// Adjust the size of the current window to fix the controls within it.
//------------------------------------------------------------------------------

static void
last_window_pos()
{
	POINT pos;

	if (parent_window_handle == NULL)
		MoveWindow(curr_window_handle, 0, 0, max_window_x + 10, window_y + 48,
			TRUE);
	else {
		pos.x = 0;
		pos.y = 0;
		ClientToScreen(parent_window_handle, &pos);
		MoveWindow(curr_window_handle, pos.x, pos.y, max_window_x + 10,
			max_window_y + 29, TRUE);
	}
}

//------------------------------------------------------------------------------
// Create a title in the current window.
//------------------------------------------------------------------------------

static HWND
create_title(char *label, int width, int height, bool next_row)
{
	HWND title_handle;

	title_handle = CreateWindow("STATIC", label, WS_CHILD | WS_VISIBLE | 
		WS_DISABLED | SS_CENTER | SS_CENTERIMAGE, window_x, window_y,
		width, height, curr_window_handle, NULL, instance_handle, NULL);
	next_window_pos(width, height, next_row);
	return(title_handle);
}

//------------------------------------------------------------------------------
// Create a label in the current window.
//------------------------------------------------------------------------------

static HWND
create_label(char *label, int width, int height, bool next_row)
{
	HWND label_handle;

	label_handle = CreateWindow("STATIC", label, WS_CHILD | WS_VISIBLE | 
		WS_DISABLED | SS_RIGHT | SS_CENTERIMAGE, window_x, window_y,
		width, height, curr_window_handle, NULL, instance_handle, NULL);
	SendMessage(label_handle, WM_SETFONT, 
		(WPARAM)GetStockObject(ANSI_VAR_FONT), 0);
	next_window_pos(width, height, next_row);
	return(label_handle);
}

//------------------------------------------------------------------------------
// Create a list view in the current window.
//------------------------------------------------------------------------------

static HWND
create_list_view(int width, int height, bool next_row)
{
	HWND list_view_handle;

	list_view_handle = CreateWindowEx(WS_EX_DLGMODALFRAME, WC_LISTVIEW, NULL,
		WS_CHILD | WS_VISIBLE | WS_DISABLED | LVS_REPORT | LVS_SINGLESEL |
		LVS_NOSORTHEADER | LVS_SORTASCENDING | LVS_SHOWSELALWAYS, window_x,
		window_y, width, height, curr_window_handle, NULL, instance_handle,
		NULL);
	next_window_pos(width, height, next_row);
	return(list_view_handle);
}

//------------------------------------------------------------------------------
// Create a button in the current window.
//------------------------------------------------------------------------------

static HWND
create_button(char *label, int width, int height, bool next_row)
{
	HWND button_handle;

	button_handle = CreateWindow("BUTTON", label, WS_CHILD | WS_VISIBLE | 
		WS_DISABLED | WS_TABSTOP | BS_PUSHBUTTON, window_x, window_y, width,
		height, curr_window_handle, NULL, instance_handle, NULL);
	SendMessage(button_handle, WM_SETFONT, 
		(WPARAM)GetStockObject(ANSI_VAR_FONT), 0);
	next_window_pos(width, height, next_row);
	return(button_handle);
}

//------------------------------------------------------------------------------
// Create a default button in the current window.
//------------------------------------------------------------------------------

static HWND
create_default_button(char *label, int width, int height, bool next_row)
{
	HWND button_handle;

	button_handle = CreateWindow("BUTTON", label, WS_CHILD | WS_VISIBLE | 
		WS_DISABLED | WS_TABSTOP | BS_DEFPUSHBUTTON, window_x, window_y, width,
		height, curr_window_handle, NULL, instance_handle, NULL);
	SendMessage(button_handle, WM_SETFONT, 
		(WPARAM)GetStockObject(ANSI_VAR_FONT), 0);
	next_window_pos(width, height, next_row);
	return(button_handle);
}

//------------------------------------------------------------------------------
// Create a list box in the current window.
//------------------------------------------------------------------------------

static HWND
create_list_box(int width, int height, bool next_row)
{
	HWND list_box_handle;

	list_box_handle = CreateWindowEx(WS_EX_DLGMODALFRAME, "LISTBOX", NULL,
		WS_CHILD | WS_VISIBLE | WS_DISABLED | LBS_STANDARD | 
		LBS_NOINTEGRALHEIGHT, window_x, window_y, width, height, 
		curr_window_handle, NULL, instance_handle, NULL);
	SendMessage(list_box_handle, WM_SETFONT, 
		(WPARAM)GetStockObject(ANSI_VAR_FONT), 0);
	next_window_pos(width, height, next_row);
	return(list_box_handle);
}

//------------------------------------------------------------------------------
// Create an edit box in the current window.
//------------------------------------------------------------------------------

static HWND
create_edit_box(int width, int height, bool next_row)
{
	HWND edit_box_handle;

	edit_box_handle = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD |
		WS_VISIBLE | WS_BORDER | WS_DISABLED | WS_TABSTOP, window_x, window_y,
		width, height, curr_window_handle, NULL, instance_handle, NULL);
	SendMessage(edit_box_handle, WM_SETFONT, 
		(WPARAM)GetStockObject(ANSI_VAR_FONT), 0);
	next_window_pos(width, height, next_row);
	return(edit_box_handle);
}

//------------------------------------------------------------------------------
// Add a column to a list view control.
//------------------------------------------------------------------------------

void
add_list_view_column(HWND list_view_handle, char *column_title, int column_index,
					 int column_width)
{
	LV_COLUMN list_view_column;

	list_view_column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	list_view_column.fmt = LVCFMT_LEFT;
	list_view_column.cx = column_width;
	list_view_column.pszText = column_title;
	list_view_column.iSubItem = column_index;
	ListView_InsertColumn(list_view_handle, column_index, &list_view_column);
}

//------------------------------------------------------------------------------
// Add an item to a list view control, returning the item index.
//------------------------------------------------------------------------------

int
add_list_view_item(HWND list_view_handle, char *text, LPARAM param)
{
	LV_ITEM list_view_item;

	list_view_item.mask = LVIF_STATE | LVIF_TEXT | LVIF_PARAM;
	list_view_item.iItem = 0;
	list_view_item.iSubItem = 0;
	list_view_item.state = 0;
	list_view_item.stateMask = 0;
	list_view_item.pszText = text;
	list_view_item.lParam = param;
	return(ListView_InsertItem(list_view_handle, &list_view_item));
}

//------------------------------------------------------------------------------
// Add a block to the style, and return a pointer to the block definition.
//------------------------------------------------------------------------------

block_def *
add_block_to_style(char block_symbol, char *block_double_symbol,
				   char *block_file_name, block *block_ptr)
{
	block_def *block_def_ptr;
	char symbol[8];
	char *BSP_tree_status;

	// Add the block to the style object, creating a block definition in the
	// process.

	if ((block_def_ptr = style_ptr->add_block_def(block_symbol, 
		block_double_symbol, block_file_name, block_ptr)) == NULL) {
		message("Error while adding block to list", "Out of memory");
		return(NULL);
	}

	// Insert a new item into the block list view control.

	block_def_ptr->item_index = add_list_view_item(block_list_view_handle, 
		block_def_ptr->file_name, (LPARAM)block_def_ptr);
	ListView_SetItemText(block_list_view_handle, block_def_ptr->item_index, 1, 
		block_ptr->name);
	symbol[0] = block_def_ptr->symbol;
	if (block_def_ptr->flags & BLOCK_DOUBLE_SYMBOL) {
		symbol[1] = ' ';
		symbol[2] = 'o';
		symbol[3] = 'r';
		symbol[4] = ' ';
		symbol[5] = block_def_ptr->double_symbol[0];
		symbol[6] = block_def_ptr->double_symbol[1];
		symbol[7] = '\0';
	} else
		symbol[1] = '\0';
	ListView_SetItemText(block_list_view_handle, block_def_ptr->item_index, 2,
		symbol);
	ListView_SetItemText(block_list_view_handle, block_def_ptr->item_index, 3,
		get_block_type(block_ptr->type));
	if (block_ptr->type == STRUCTURAL_BLOCK) {
		if (block_ptr->has_BSP_tree)
			BSP_tree_status = block_ptr->root_polygon_ptr ? "yes" : "empty";
		else
			BSP_tree_status = "no";
	} else
		BSP_tree_status = "N/A";
	ListView_SetItemText(block_list_view_handle, block_def_ptr->item_index, 4,
		BSP_tree_status);

	// Return a pointer to the block definition.

	return(block_def_ptr);
}

//------------------------------------------------------------------------------
// Display a dialog box for selecting a file to open.
//------------------------------------------------------------------------------

static bool
get_open_file_name(char *title, char *filter, char *initial_dir)
{
	OPENFILENAME open_file_name;

	memset(&open_file_name, 0, sizeof(OPENFILENAME));
	open_file_name.lStructSize = sizeof(OPENFILENAME);
	open_file_name.lpstrFilter = filter;
	if (initial_dir && initial_dir[0])
		open_file_name.lpstrInitialDir = initial_dir;
	user_file_path[0] = '\0';
	open_file_name.lpstrFile = user_file_path;
	open_file_name.nMaxFile = _MAX_PATH;
	open_file_name.lpstrTitle = title;
	open_file_name.Flags = OFN_LONGNAMES | OFN_FILEMUSTEXIST |
		OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	return(GetOpenFileName(&open_file_name) != 0);
}

//------------------------------------------------------------------------------
// Display a dialog box for selecting a file to save.
//------------------------------------------------------------------------------

static bool
get_save_file_name(char *title, char *filter, char *initial_file_path)
{
	OPENFILENAME save_file_name;
	char initial_dir[_MAX_PATH];
	char *file_name_ptr;

	// Split the initial file path into directory and file name, if it exists,
	// otherwise use the blocksets directory and no file name.

	if (initial_file_path[0] != '\0') {
		strcpy(initial_dir, initial_file_path);
		file_name_ptr = strrchr(initial_dir, '\\');
		*file_name_ptr = '\0';
		strcpy(user_file_path, file_name_ptr + 1);
	} else {
		strcpy(initial_dir, blocksets_dir);
		user_file_path[0] = '\0';
	}

	// Set up the save file dialog and call it.

	memset(&save_file_name, 0, sizeof(OPENFILENAME));
	save_file_name.lStructSize = sizeof(OPENFILENAME);
	save_file_name.lpstrFilter = filter;
	save_file_name.lpstrInitialDir = initial_dir;
	save_file_name.lpstrFile = user_file_path;
	save_file_name.nMaxFile = _MAX_PATH;
	save_file_name.lpstrTitle = title;
	save_file_name.Flags = OFN_LONGNAMES | OFN_PATHMUSTEXIST | 
		OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
	return(GetSaveFileName(&save_file_name) != 0);
}

//------------------------------------------------------------------------------
// Default dialog procedure.
//------------------------------------------------------------------------------

static BOOL CALLBACK
default_dialog_proc(HWND window_handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_INITDIALOG)
		return(TRUE);
	else
		return(FALSE);
}

//==============================================================================
// BSP tree creation functions.
//==============================================================================

//------------------------------------------------------------------------------
// Compare the second polygon with the plane of the first, and return one of
// the following values: IN_FRONT_OF_PLANE, BEHIND_PLANE, or INTERSECTS_PLANE.
//------------------------------------------------------------------------------

static unsigned char
compare_polygon_with_plane(block *block_ptr, polygon *polygon1_ptr,
						   polygon *polygon2_ptr)
{
	int polygon1_no, polygon2_no;
	int first, second, field, shift;
	unsigned char mask, value;

	// First look up the approapiate entry in the polygon comparison table; if
	// it's set, just return that value.

	polygon1_no = polygon1_ptr->polygon_no - 1;
	polygon2_no = polygon2_ptr->polygon_no - 1; 
	first = polygon1_no;
	second = polygon2_no / 4;
	field = polygon2_no % 4;
	shift = field * 2;
	mask = 0x03 << shift;
	value = (poly_cmp_table[first][second] & mask) >> shift;
	if (value > 0)
		return(value);

	// Get the plane normal and plane offset from polygon 1.

	vector plane_normal = polygon1_ptr->plane_normal;
	double plane_offset = polygon1_ptr->plane_offset;

	// Plug the vertices of polygon 2 into the plane equation of polygon 1,
	// counting how many are in front, behind and on the plane.

	int vertices_in_front = 0;
	int vertices_behind = 0;
	int vertices_on = 0;
	for (int index = 0; index < polygon2_ptr->vertices; index++) {
		vertex *vertex_ptr = 
			block_ptr->get_vertex(polygon2_ptr->get_vertex_def(index));
		double side = plane_normal.dx * vertex_ptr->x + plane_normal.dy *
			vertex_ptr->y + plane_normal.dz * vertex_ptr->z + plane_offset;
		if (FPOS(side))
			vertices_in_front++;
		else if (FNEG(side))
			vertices_behind++;
		else
			vertices_on++;
	}

	// Return the position of the second polygon in relation to the plane of
	// the first, after saving it in the polygon comparison table.

	if (vertices_in_front + vertices_on == polygon2_ptr->vertices)
		value = IN_FRONT_OF_PLANE;
	else if (vertices_behind + vertices_on == polygon2_ptr->vertices)
		value = BEHIND_PLANE;
	else
		value = INTERSECTS_PLANE;
	poly_cmp_table[first][second] |= value << shift;
	return(value);
}

//------------------------------------------------------------------------------
// Split the second polygon by the plane of the first polygon, resulting in two
// polygons, one on either side of the plane.  Pointers to these two polygons
// are returned.
//------------------------------------------------------------------------------

static void
split_polygon_by_plane(block *block_ptr, polygon *polygon1_ptr, 
					   polygon *polygon2_ptr, polygon *&front_polygon_ptr,
					   polygon *&rear_polygon_ptr)
{
	vector plane_normal;
	double plane_offset;
	vertex *plane_vertex_ptr;
	double prev_side, side;
	vertex_def *vertex_def_list;
	vertex_def *vertex_def_ptr, *prev_vertex_def_ptr;
	vertex *vertex_ptr;

	// Get the plane normal and plane offset from the first polygon, and the
	// vertex definition list from the second polygon.

	plane_normal = polygon1_ptr->plane_normal;
	plane_offset = polygon1_ptr->plane_offset;
	plane_vertex_ptr = block_ptr->get_vertex(polygon1_ptr->get_vertex_def(0));
	vertex_def_list = polygon2_ptr->vertex_def_list;

	// Create a new "rear" and "front" polygon that is initialised with the
	// contents of the second polygon.
	
	if ((front_polygon_ptr = block_ptr->add_polygon()) == NULL ||
		(rear_polygon_ptr = block_ptr->add_polygon()) == NULL)
		memory_error("polygon");
	front_polygon_ptr->init_polygon(polygon2_ptr);
	rear_polygon_ptr->init_polygon(polygon2_ptr);

	// Set the part number of the second polygon to zero, so that it won't be
	// included in the final list of polygons, and increment the active polygon
	// count.

	polygon2_ptr->part_no = 0;
	polygons_active++;

	// Step through the vertex definition list of the second polygon, and add
	// them to the approapiate polygon (front or rear).  

	vertex_def_ptr = vertex_def_list;
	prev_vertex_def_ptr = NULL;
	prev_side = 0.0;
	while (vertex_def_ptr) {

		// Determine whether this vertex is behind, on, or in front of the
		// plane of the first polygon.

		vertex_ptr = block_ptr->get_vertex(vertex_def_ptr);
		side = plane_normal.dx * vertex_ptr->x + plane_normal.dy *
			vertex_ptr->y + plane_normal.dz * vertex_ptr->z + plane_offset;

		// If this vertex is on the plane, add it to both lists.

		if (FZERO(side)) {
			if (front_polygon_ptr->add_vertex_def(vertex_def_ptr->vertex_no,
				vertex_def_ptr->u, vertex_def_ptr->v) == NULL ||
				rear_polygon_ptr->add_vertex_def(vertex_def_ptr->vertex_no,
				vertex_def_ptr->u, vertex_def_ptr->v) == NULL)
				memory_error("split vertex definitions");
		}

		// If this vertex is on either side of the plane...

		else {

			// If the previous vertex was on the other side of the plane, then
			// create a new vertex that intersect the plane, and add it's vertex
			// definition to both lists.

			if (!FZERO(prev_side) && ((FPOS(prev_side) && FNEG(side)) ||
				(FNEG(prev_side) && FPOS(side)))) {
				vertex *prev_vertex_ptr, *new_vertex_ptr;
				vector line;
				double distance, length, ratio;
				double ix, iy, iz, iu, iv;

				// Determine the distance from the previous vertex to the plane.

				prev_vertex_ptr = block_ptr->get_vertex(prev_vertex_def_ptr);
				line = *prev_vertex_ptr - *plane_vertex_ptr;
				distance = ABS(line & plane_normal);

				// Determine the length of the vector from the previous vertex
				// to this vertex.

				line = *vertex_ptr - *prev_vertex_ptr;
				length = ABS(line & plane_normal);

				// Compute how far along the vector the intersection with the
				// plane occurs, then use this to create a new vertex and a
				// new vertex definition.

				ratio = distance / length;
				ix = prev_vertex_ptr->x + line.dx * ratio;
				iy = prev_vertex_ptr->y + line.dy * ratio;
				iz = prev_vertex_ptr->z + line.dz * ratio;
				iu = prev_vertex_def_ptr->u + 
					(vertex_def_ptr->u - prev_vertex_def_ptr->u) * ratio;
				iv = prev_vertex_def_ptr->v +
					(vertex_def_ptr->v - prev_vertex_def_ptr->v) * ratio;

				// Add the new vertex to the block.

				if ((new_vertex_ptr = block_ptr->add_vertex(ix, iy, iz)) == NULL)
					memory_error("vertex");

				// Add a vertex definition to both lists.

				if (front_polygon_ptr->add_vertex_def(new_vertex_ptr->vertex_no,
					iu, iv) == NULL ||
					rear_polygon_ptr->add_vertex_def(new_vertex_ptr->vertex_no,
					iu, iv) == NULL)
					memory_error("vertex definition");
			}

			// Add a duplicate of the vertex definition to the appropiate list.

			if (FPOS(side)) {
				if (front_polygon_ptr->add_vertex_def(vertex_def_ptr->vertex_no,
					vertex_def_ptr->u, vertex_def_ptr->v) == NULL)
					memory_error("vertex definition");
			} else {
				if (rear_polygon_ptr->add_vertex_def(vertex_def_ptr->vertex_no,
					vertex_def_ptr->u, vertex_def_ptr->v) == NULL)
					memory_error("vertex definition");
			}
		}

		// Move onto the next vertex definition.

		prev_side = side;
		prev_vertex_def_ptr = vertex_def_ptr;
		vertex_def_ptr = vertex_def_ptr->next_vertex_def_ptr;
	}

	// If the first vertex definition is on the opposite side of the plane to
	// the last vertex definition, then create a new vertex that intersect the
	// plane, and add it's vertex definition to both lists.

	vertex_ptr = block_ptr->get_vertex(vertex_def_list);
	side = plane_normal.dx * vertex_ptr->x + plane_normal.dy *
		vertex_ptr->y + plane_normal.dz * vertex_ptr->z + plane_offset;
	if (!FZERO(prev_side) && ((FPOS(prev_side) && FNEG(side)) ||
		(FNEG(prev_side) && FPOS(side)))) {
		vertex *prev_vertex_ptr, *new_vertex_ptr;
		vector line;
		double distance, length, ratio;
		double ix, iy, iz, iu, iv;

		// Determine the distance from the previous vertex to the plane.

		prev_vertex_ptr = block_ptr->get_vertex(prev_vertex_def_ptr);
		line = *prev_vertex_ptr - *plane_vertex_ptr;
		distance = ABS(line & plane_normal);

		// Determine the length of the vector from the previous vertex
		// to this vertex.

		line = *vertex_ptr - *prev_vertex_ptr;
		length = ABS(line & plane_normal);

		// Compute how far along the vector the intersection with the
		// plane occurs, then use this to create a new vertex and a
		// new vertex definition.

		ratio = distance / length;
		ix = prev_vertex_ptr->x + line.dx * ratio;
		iy = prev_vertex_ptr->y + line.dy * ratio;
		iz = prev_vertex_ptr->z + line.dz * ratio;
		iu = prev_vertex_def_ptr->u + 
			(vertex_def_list->u - prev_vertex_def_ptr->u) * ratio;
		iv = prev_vertex_def_ptr->v +
			(vertex_def_list->v - prev_vertex_def_ptr->v) * ratio;

		// Add the new vertex to the block.

		if ((new_vertex_ptr = block_ptr->add_vertex(ix, iy, iz)) == NULL)
			memory_error("vertex");

		// Add a vertex definition to both lists.

		if (front_polygon_ptr->add_vertex_def(new_vertex_ptr->vertex_no,
			iu, iv) == NULL ||
			rear_polygon_ptr->add_vertex_def(new_vertex_ptr->vertex_no,
			iu, iv) == NULL)
			memory_error("vertex definition");
	}
}

//------------------------------------------------------------------------------
// Create a BSP tree containing the polygons in the given polygon list, and
// return a pointer to the polygon in the root node.
//------------------------------------------------------------------------------

static polygon *
build_BSP_tree(block *block_ptr, polygon *polygon_list, int polygons)
{
	polygon *polygon_ptr, *best_polygon_ptr;
	int min_splits;
	int current_polygon_no;
	polygon *front_polygon_list, *rear_polygon_list;
	polygon *front_polygon_ptr, *rear_polygon_ptr;
	int front_polygons, rear_polygons;

	// If the polygon list is empty, there is nothing to do.

	if (polygon_list == NULL)
		return(NULL);

	// Display the number of polygons processed and added.

	sprintf(text, "Added %d of %d polygons to BSP tree", polygons_processed,
		polygons_active); 
	SendMessage(polygons_processed_handle, WM_SETTEXT, 0, (LPARAM)text);
	polygons_processed++;
	sprintf(text, "Split %d polygons", polygons_added); 
	SendMessage(polygons_added_handle, WM_SETTEXT, 0, (LPARAM)text);

	// Step through the polygon list, and determine how many splits each polygon
	// generates.  Remember the polygon that generates the least splits.  To
	// speed things up a little, if a polygon with zero splits is encountered,
	// we accept it and break out of the loop.

	current_polygon_no = 0;
	best_polygon_ptr = NULL;
	min_splits = polygons + 1;
	polygon_ptr = polygon_list;
	while (polygon_ptr) {
		polygon *curr_polygon_ptr = polygon_list;
		int splits = 0;
		while (curr_polygon_ptr) {
			if (compare_polygon_with_plane(block_ptr, polygon_ptr, 
				curr_polygon_ptr) == INTERSECTS_PLANE)
				splits++;
			curr_polygon_ptr = curr_polygon_ptr->next_BSP_polygon_ptr;
		}
		current_polygon_no++;
		sprintf(text, "Processed polygon %d of %d (%d splits)", 
			current_polygon_no, polygons, splits); 
		SendMessage(current_polygon_handle, WM_SETTEXT, 0, (LPARAM)text);
		if (splits < min_splits) {
			min_splits = splits;
			best_polygon_ptr = polygon_ptr;
		}
		if (splits == 0)
			break;
		polygon_ptr = polygon_ptr->next_BSP_polygon_ptr;
	}

	// Update the number of polygons that have been split.

	polygons_added += min_splits;
	
	// Divide the polygon list into two, those in front of the chosen polygon's
	// plane, and those behind.  Polygons that intersect the chosen polygon's
	// plane must be split into two polygons, one on either side of the plane.

	front_polygon_list = NULL;
	front_polygons = 0;
	rear_polygon_list = NULL;
	rear_polygons = 0;
	polygon_ptr = polygon_list;
	while (polygon_ptr) {
		polygon *next_polygon_ptr = polygon_ptr->next_BSP_polygon_ptr;
		if (polygon_ptr != best_polygon_ptr)
			switch (compare_polygon_with_plane(block_ptr, best_polygon_ptr,
				polygon_ptr)) {
			case BEHIND_PLANE:
				polygon_ptr->next_BSP_polygon_ptr = rear_polygon_list;
				rear_polygon_list = polygon_ptr;
				rear_polygons++;
				break;
			case IN_FRONT_OF_PLANE:
				polygon_ptr->next_BSP_polygon_ptr = front_polygon_list;
				front_polygon_list = polygon_ptr;
				front_polygons++;
				break;
			case INTERSECTS_PLANE:
				split_polygon_by_plane(block_ptr, best_polygon_ptr, polygon_ptr,
					front_polygon_ptr, rear_polygon_ptr);
				front_polygon_ptr->next_BSP_polygon_ptr = front_polygon_list;
				front_polygon_list = front_polygon_ptr;
				front_polygons++;
				rear_polygon_ptr->next_BSP_polygon_ptr = rear_polygon_list;
				rear_polygon_list = rear_polygon_ptr;
				rear_polygons++;
			}
		polygon_ptr = next_polygon_ptr;
	}

	// Build a BSP tree for the two polygon lists, and store the pointers to
	// the root node polygons in the chosen polygon.

	best_polygon_ptr->front_polygon_ptr = build_BSP_tree(block_ptr,
		front_polygon_list, front_polygons);
	best_polygon_ptr->rear_polygon_ptr = build_BSP_tree(block_ptr, 
		rear_polygon_list, rear_polygons);

	// Return a pointer to the polygon chosen.

	return(best_polygon_ptr);
}

//------------------------------------------------------------------------------
// Add BSP trees to all blocks.
//------------------------------------------------------------------------------

static void
add_BSP_trees(void)
{
	block_def *block_def_ptr;
	HWND progress_dialog_handle;
	HWND block_name_handle;

	// Create a dialog box containing the name of the block being processed,
	// how many polygons have been processed, and the progress bar (number
	// of blocks processed).

	progress_dialog_handle = CreateDialog(instance_handle, 
		MAKEINTRESOURCE(IDD_BSP_PROGRESS), style_window_handle, 
		default_dialog_proc);
	block_name_handle = GetDlgItem(progress_dialog_handle, IDC_BLOCK_NAME);
	polygons_processed_handle = GetDlgItem(progress_dialog_handle,
		IDC_POLYGONS_PROCESSED);
	polygons_added_handle = GetDlgItem(progress_dialog_handle,
		IDC_POLYGONS_ADDED);
	current_polygon_handle = GetDlgItem(progress_dialog_handle,
		IDC_CURRENT_POLYGON);

	// Step through the block definition list, and add BSP trees to all
	// blocks that need them.

	block_def_ptr = style_ptr->block_def_list;
	while (block_def_ptr) {
		block *block_ptr = block_def_ptr->block_ptr;

		// Create the BSP tree if this is a structural block that hasn't
		// yet got a BSP tree.

		if (block_ptr->type == STRUCTURAL_BLOCK && !block_ptr->has_BSP_tree) {
			LV_FINDINFO list_view_find_info;
			int item_index;

			// Set the block name in the dialog box.

			sprintf(text, "Adding BSP tree to %s", block_def_ptr->file_name);
			SendMessage(block_name_handle, WM_SETTEXT, 0, (LPARAM)text);

			// Initialise polygon comparison table.

			for (int first = 0; first < MAX_POLYGONS; first++)
				for (int second = 0; second < MAX_POLYGONS / 4; second++)
					poly_cmp_table[first][second] = 0;

			// Create the initial BSP polygon list.

			block_ptr->create_BSP_polygon_list();

			// Create the BSP tree.

			polygons_processed = 0;
			polygons_added = 0;
			polygons_active = block_ptr->polygons;
			block_ptr->root_polygon_ptr = build_BSP_tree(block_ptr, 
				block_ptr->polygon_list, block_ptr->polygons);
			block_ptr->has_BSP_tree = true;
			blockset_changed = true;

			// If the BSP tree was not empty...

			if (block_ptr->root_polygon_ptr) {

				// Renumber the final polygon list.

				block_ptr->renumber_polygon_list();

				// Find and update the list view item.

				list_view_find_info.flags = LVFI_PARAM;
				list_view_find_info.lParam = (LPARAM)block_def_ptr;
				item_index = ListView_FindItem(block_list_view_handle, -1,
					&list_view_find_info);
				ListView_SetItemText(block_list_view_handle, item_index, 4, 
					"yes");
			}

			// If the BSP tree was empty...

			else {
				list_view_find_info.flags = LVFI_PARAM;
				list_view_find_info.lParam = (LPARAM)block_def_ptr;
				item_index = ListView_FindItem(block_list_view_handle, -1,
					&list_view_find_info);
				ListView_SetItemText(block_list_view_handle, item_index, 4, 
					"empty");
			}
		}

		// Move onto the next block definition.

		block_def_ptr = block_def_ptr->next_block_def_ptr;
	}

	// Destroy the progress dialog box.

	DestroyWindow(progress_dialog_handle);
}

//==============================================================================
// Directory and file functions.
//==============================================================================

//------------------------------------------------------------------------------
// Write all files in the given source directory to the target directory in the
// currently open blockset file.
//------------------------------------------------------------------------------

void
write_directory(char *source_dir, char *target_dir)
{
	char source_file_path[_MAX_PATH];
	char target_file_path[_MAX_PATH];
	char wildcard_file_path[_MAX_PATH];
	struct _finddata_t file_info;
	long find_handle;

	// Construct the wildcard file path for searching the given source
	// directory.

	strcpy(wildcard_file_path, source_dir);
	strcat(wildcard_file_path, "*.*");

	// If there are no files in the given source directory, just return. 

	if ((find_handle = _findfirst(wildcard_file_path, &file_info)) == -1)
       return;

	// Otherwise write each file found to the target directory in the
	// currently open blockset file.

	do {

		// If the file name is "." or "..", skip over this file.

		if (!strcmp(file_info.name, ".") || !strcmp(file_info.name, ".."))
			continue;

		// Create the full path to the file that was found.

		strcpy(source_file_path, source_dir);
		strcat(source_file_path, file_info.name);

		// Open the file, write it to the blockset, then close the file.

		if (!push_file(source_file_path))
			file_error("Unable to open file '%s' for reading",
				source_file_path);
		strcpy(target_file_path, target_dir);
		strcat(target_file_path, file_info.name);
		if (!write_ZIP_file(target_file_path))
			file_error("Unable to write file '%s' to blockset", 
				blockset_file_path);
		pop_file();

		// Find next file.

	} while (_findnext(find_handle, &file_info) == 0);

	// Done searching the directory.

   _findclose(find_handle);
}

//------------------------------------------------------------------------------
// Fill the given list box with the files in the given temporary directory.
//------------------------------------------------------------------------------

void
list_directory(HWND list_handle, char *temp_dir)
{
	char wildcard_file_path[_MAX_PATH];
	struct _finddata_t file_info;
	long find_handle;

	// Clear the list box.

	SendMessage(list_handle, LB_RESETCONTENT, 0, 0);

	// Construct the wildcard file path for searching the given temporary
	// directory.

	strcpy(wildcard_file_path, temp_dir);
	strcat(wildcard_file_path, "*.*");

	// If there are no files in the given temporary directory, just return.

	if ((find_handle = _findfirst(wildcard_file_path, &file_info)) == -1)
		return;

	// Otherwise add each file found to the specified list box.

	do {
		// If the file name is "." or "..", skip over this file.

		if (!strcmp(file_info.name, ".") || !strcmp(file_info.name, ".."))
			continue;

		// Add the file name to the list box.

		SendMessage(list_handle, LB_ADDSTRING, 0, (LPARAM)file_info.name);

		// Find next file.

	} while (_findnext(find_handle, &file_info) == 0);

	// Done searching the directory.

   _findclose(find_handle);
}

//------------------------------------------------------------------------------
// Delete a temporary directory and all of it's contents.
//------------------------------------------------------------------------------

static void
delete_directory(char *temp_dir)
{
	char wildcard_file_path[_MAX_PATH];
	char temp_file_path[_MAX_PATH];
	struct _finddata_t file_info;
	long find_handle;

	// Construct the wildcard file path for searching the given temporary
	// directory.

	strcpy(wildcard_file_path, temp_dir);
	strcat(wildcard_file_path, "*.*");

	// If there are no files in the given temporary directory, just delete the
	// directory and return. 

	if ((find_handle = _findfirst(wildcard_file_path, &file_info)) == -1) {
		rmdir(temp_dir);
		return;
	}

	// Otherwise add each file found to the specified list box.

	do {
		// If the file name is "." or "..", skip over this file.

		if (!strcmp(file_info.name, ".") || !strcmp(file_info.name, ".."))
			continue;

		// Create the full path to the file that was found.

		strcpy(temp_file_path, temp_dir);
		strcat(temp_file_path, file_info.name);

		// Delete the file.

		remove(temp_file_path);

		// Find next file.

	} while (_findnext(find_handle, &file_info) == 0);

	// Done searching the directory, so delete it.

   _findclose(find_handle);
   rmdir(temp_dir);
}

//------------------------------------------------------------------------------
// Initialise the style window.
//------------------------------------------------------------------------------

static void
init_style_window(void)
{
	HMENU menu_handle, submenu_handle;

	// Enable all approapiate windows.

	EnableWindow(style_tags_title_handle, TRUE);
	EnableWindow(edit_version_button_handle, TRUE);
	EnableWindow(edit_placeholder_button_handle, TRUE);
	EnableWindow(edit_sky_button_handle, TRUE);
	EnableWindow(edit_ground_button_handle, TRUE);
	EnableWindow(edit_orb_button_handle, TRUE);

	EnableWindow(block_list_title_handle, TRUE);
	EnableWindow(block_list_view_handle, TRUE);
	EnableWindow(add_block_button_handle, TRUE);
	EnableWindow(add_BSP_trees_button_handle, TRUE);

	EnableWindow(texture_list_title_handle, TRUE);
	EnableWindow(texture_list_box_handle, TRUE);
	EnableWindow(add_texture_button_handle, TRUE);

	EnableWindow(sound_list_title_handle, TRUE);
	EnableWindow(sound_list_box_handle, TRUE);
	EnableWindow(add_sound_button_handle, TRUE);

	// Disable the new and open blockset items, and enable the save, save as
	// and close blockset items on the file menu.

	menu_handle = GetMenu(style_window_handle);
	submenu_handle = GetSubMenu(menu_handle, 0);
	EnableMenuItem(submenu_handle, MENU_FILE_NEW,
		MF_BYCOMMAND | MF_GRAYED);
	EnableMenuItem(submenu_handle, MENU_FILE_OPEN,
		MF_BYCOMMAND | MF_GRAYED);
	EnableMenuItem(submenu_handle, MENU_FILE_SAVE, 
		MF_BYCOMMAND | MF_ENABLED);
	EnableMenuItem(submenu_handle, MENU_FILE_SAVE_AS,
		MF_BYCOMMAND | MF_ENABLED);
	EnableMenuItem(submenu_handle, MENU_FILE_CLOSE, 
		MF_BYCOMMAND | MF_ENABLED);
}

//------------------------------------------------------------------------------
// Create the temporary directories.
//------------------------------------------------------------------------------

static void
create_temporary_directories(void)
{
	mkdir(temp_dir);
	mkdir(temp_blocks_dir);
	mkdir(temp_textures_dir);
	mkdir(temp_sounds_dir);
}

//------------------------------------------------------------------------------
// Open a blockset.
//------------------------------------------------------------------------------

static void
open_blockset(void)
{
	// Create the temporary directories.

	create_temporary_directories();

	// Get the blockset name, and attempt to load the blockset.

	if (get_open_file_name("Open Blockset...", BLOCKSET_FILETYPE, 
		blocksets_dir)) {

		// Load the blockset.

		if (load_blockset(user_file_path)) {

			// Indicate a blockset is loaded, but it hasn't yet been changed.

			blockset_loaded = true;
			blockset_changed = false;

			// List the textures and sounds available.

			list_directory(texture_list_box_handle, temp_textures_dir);
			list_directory(sound_list_box_handle, temp_sounds_dir);

			// Set up the style window.

			init_style_window();
		}

		// If this fails, destroy the style object.

		else
			delete style_ptr;
	}
}

//------------------------------------------------------------------------------
// Close the currently loaded blockset.
//------------------------------------------------------------------------------

static void
close_blockset(void)
{
	int result;
	HMENU menu_handle, submenu_handle;

	// If the blockset has been changed, ask the user if they want to save
	// it.

	if (blockset_changed) {
		result = MessageBox(NULL, "Changes have been made to the blockset.\n"
			"Do you wish to save it?", "Blockset not saved", MB_YESNO);
		if (result == IDYES) {
			if (get_save_file_name("Save blockset", BLOCKSET_FILETYPE,
				blockset_file_path)) {
				if (save_blockset(user_file_path))
					blockset_changed = false;
			}
		} 
	}
	
	// Delete the block definition list in the style, then the style object
	// itself.

	style_ptr->del_block_def_list();
	delete style_ptr;

	// Delete the block list view.

	ListView_DeleteAllItems(block_list_view_handle);

	// Indicate the blockset is no longer loaded.

	blockset_loaded = false;

	// Delete the blocks, textures, sounds and temporary directories.

	delete_directory(temp_blocks_dir);
	delete_directory(temp_textures_dir);
	delete_directory(temp_sounds_dir);
	delete_directory(temp_dir);

	// Reset the content of the texture and sound list boxes.

	SendMessage(texture_list_box_handle, LB_RESETCONTENT, 0, 0);
	SendMessage(sound_list_box_handle, LB_RESETCONTENT, 0, 0);

	// Disable all windows.

	EnableWindow(style_tags_title_handle, FALSE);
	EnableWindow(edit_version_button_handle, FALSE);
	EnableWindow(edit_placeholder_button_handle, FALSE);
	EnableWindow(edit_sky_button_handle, FALSE);
	EnableWindow(edit_ground_button_handle, FALSE);
	EnableWindow(edit_orb_button_handle, FALSE);

	EnableWindow(block_list_title_handle, FALSE);
	EnableWindow(block_list_view_handle, FALSE);
	EnableWindow(add_block_button_handle, FALSE);
	EnableWindow(remove_block_button_handle, FALSE);
	EnableWindow(rename_block_button_handle, FALSE);
	EnableWindow(edit_block_button_handle, FALSE);
	EnableWindow(add_BSP_trees_button_handle, FALSE);

	EnableWindow(texture_list_title_handle, FALSE);
	EnableWindow(texture_list_box_handle, FALSE);
	EnableWindow(add_texture_button_handle, FALSE);
	EnableWindow(remove_texture_button_handle, FALSE);
	EnableWindow(rename_texture_button_handle, FALSE);

	EnableWindow(sound_list_title_handle, FALSE);
	EnableWindow(sound_list_box_handle, FALSE);
	EnableWindow(add_sound_button_handle, FALSE);
	EnableWindow(remove_sound_button_handle, FALSE);
	EnableWindow(rename_sound_button_handle, FALSE);

	// Enable the new and open blockset items, and disable the save, save as
	// and close blockset items in the file menu.

	menu_handle = GetMenu(style_window_handle);
	submenu_handle = GetSubMenu(menu_handle, 0);
	EnableMenuItem(submenu_handle, MENU_FILE_NEW, MF_BYCOMMAND | MF_ENABLED);
	EnableMenuItem(submenu_handle, MENU_FILE_OPEN, MF_BYCOMMAND | MF_ENABLED);
	EnableMenuItem(submenu_handle, MENU_FILE_SAVE, MF_BYCOMMAND | MF_GRAYED);
	EnableMenuItem(submenu_handle, MENU_FILE_SAVE_AS, MF_BYCOMMAND | MF_GRAYED);
	EnableMenuItem(submenu_handle, MENU_FILE_CLOSE, 
		MF_BYCOMMAND | MF_GRAYED);
}

//------------------------------------------------------------------------------
// Make a copy of a file.
//------------------------------------------------------------------------------

static bool
copy_file(char *old_file_path, char *new_file_path)
{
	FILE *old_fp, *new_fp;
	char buffer[BUFSIZ];
	unsigned int size;

	// Open old and new files.

	if ((old_fp = fopen(old_file_path, "rb")) == NULL) {
		message("File error", "Unable to open file '%s' for reading",
			old_file_path);
		return(false);
	}
	if ((new_fp = fopen(new_file_path, "wb")) == NULL) {
		fclose(old_fp);
		message("File error", "Unable to open file '%s' for writing",
			new_file_path);
		return(false);
	}

	// Copy old file to new file one character at a time.

	while ((size = fread(buffer, 1, BUFSIZ, old_fp)) > 0) {
		if (fwrite(buffer, 1, size, new_fp) != size) {
			fclose(old_fp);
			fclose(new_fp);
			message("File error", "Unable to write to file '%s'",
				new_file_path);
			return(false);
		}
	}

	// Verify the reading didn't stop due to an error.

	if (ferror(old_fp)) {
		fclose(old_fp);
		fclose(new_fp);
		message("File error", "Unable to read from file '%s'",
			old_file_path);
		return(false);
	}

	// Close both files and exit.

	fclose(old_fp);
	fclose(new_fp);
	return(true);
}

//------------------------------------------------------------------------------
// Update a string with the contents of an edit box.
//------------------------------------------------------------------------------

static void
update_string(string *string_ptr, int *flags_ptr, int flag, HWND window_handle,
			  int edit_box_ID)
{
	char text[256];

	GetWindowText(GetDlgItem(window_handle, edit_box_ID), text, 256);
	if (strcmp(*string_ptr, text)) {
		blockset_changed = true;
		*string_ptr = text;
	}
	if (strlen(text) > 0)
		*flags_ptr |= flag;
	else
		*flags_ptr &= ~flag;
}

//==============================================================================
// Event functions.
//==============================================================================

//------------------------------------------------------------------------------
// Handle renaming of a block.
//------------------------------------------------------------------------------

static BOOL CALLBACK
rename_block_handler(HWND window_handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
	char file_name[256];
	char *ext_ptr;
	LV_FINDINFO find_info;
	int index;
	string old_file_name, new_file_name;

	switch (msg) {
	case WM_INITDIALOG:
		SetWindowText(window_handle, "Rename block file");
		SetWindowText(GetDlgItem(window_handle, IDC_FILE_NAME),
			selected_block_def_ptr->file_name);
		return(TRUE);
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
			switch (LOWORD(wParam)) {
			case IDOK:

				// Get the chosen block file name.  If it hasn't changed,
				// then just close the dialog box.

				GetWindowText(GetDlgItem(window_handle, IDC_FILE_NAME), 
					file_name, 256);
				if (!strcmp(file_name, selected_block_def_ptr->file_name)) {
					EndDialog(window_handle, TRUE);
					return(TRUE);
				}

				// Get the chosen block file name, and make sure it not blank,
				// has no path delimiters, and ends with ".block".

				if (strlen(file_name) == 0) {
					message("Invalid block file name", "You must enter a file "
						"name for the block");
					return(TRUE);
				}
				if (strchr(file_name, '\\') || strchr(file_name, '/')) {
					message("Invalid block file name", "It is not permissible "
						"to use '\\' or '/' in a block file name");
					return(TRUE);
				}
				ext_ptr = strrchr(file_name, '.');
				if (ext_ptr == NULL || stricmp(ext_ptr, ".block")) {
					message("Invalid block file name", "The block file name "
						"must end with '.block'");
					return(TRUE);
				}

				// If the block already exists in the block list with the chosen
				// block file name, reject it.

				find_info.flags = LVFI_STRING;
				find_info.psz = file_name;
				index = ListView_FindItem(block_list_view_handle, -1, 
					&find_info);
				if (index >= 0) {
					message("Block exists", "A block with the file name '%s' "
						"already exists in the blockset", file_name);
					return(TRUE);
				}

				// Rename the file in the temporary blocks directory.

				old_file_name = temp_blocks_dir;
				old_file_name += selected_block_def_ptr->file_name;
				new_file_name = temp_blocks_dir;
				new_file_name += file_name;
				rename(old_file_name, new_file_name);

				// Update the name field in the block definition and block
				// list view.

				selected_block_def_ptr->file_name = file_name;
				ListView_SetItemText(block_list_view_handle, 
					selected_block_def_ptr->item_index, 0, file_name);

				// Indicate the blockset has changed, and close the dialog box
				// with a success status.

				blockset_changed = true;
				EndDialog(window_handle, TRUE);
				break;
			case IDCANCEL:
				EndDialog(window_handle, FALSE);
			}
			return(TRUE);
	default:
		return(FALSE);
	}
}

//------------------------------------------------------------------------------
// Handle editing of part tag.
//------------------------------------------------------------------------------

static BOOL CALLBACK
part_handler(HWND window_handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
	char part_name[256];
	block *block_ptr;
	int index;

	switch (msg) {
	case WM_INITDIALOG:
		SetWindowText(GetDlgItem(window_handle, IDC_PART_NAME),
			selected_part_ptr->name);
		SetWindowText(GetDlgItem(window_handle, IDC_PART_TEXTURE),
			selected_part_ptr->texture);
		SetWindowText(GetDlgItem(window_handle, IDC_PART_COLOUR),
			selected_part_ptr->colour);
		SetWindowText(GetDlgItem(window_handle, IDC_PART_TRANSLUCENCY),
			selected_part_ptr->translucency);
		SetWindowText(GetDlgItem(window_handle, IDC_PART_STYLE),
			selected_part_ptr->style);
		SetWindowText(GetDlgItem(window_handle, IDC_PART_FACES),
			selected_part_ptr->faces);
		SetWindowText(GetDlgItem(window_handle, IDC_PART_ANGLE),
			selected_part_ptr->angle);
		return(TRUE);
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
			switch (LOWORD(wParam)) {
			case IDOK:

				// Don't close the part window if the part name is empty.

				GetWindowText(GetDlgItem(window_handle, IDC_PART_NAME),
					part_name, 256);
				if (strlen(part_name) == 0) {
					message("Missing part name", 
						"You must give the part a name");
					return(TRUE);
				}

				// If the part name has changed, make sure it isn't a duplicate
				// of another name.  Otherwise change the selected part's name,
				// update the part list, and disable the edit part button.

				else if (strcmp(selected_part_ptr->name, part_name)) {
					block_ptr = edited_block_def_ptr->block_ptr;
					if (block_ptr->find_part(part_name)) {
						message("Duplicate part name", "You must give the part "
							"a unique name");
						return(TRUE);
					} else {
						blockset_changed = true;
						index = SendMessage(part_list_box_handle,
							LB_FINDSTRINGEXACT, 0, 
							(LPARAM)(char *)selected_part_ptr->name);
						selected_part_ptr->name = part_name;
						SendMessage(part_list_box_handle, LB_DELETESTRING,
							index, 0);
						SendMessage(part_list_box_handle, LB_ADDSTRING, 0, 
							(LPARAM)part_name);
						EnableWindow(edit_part_button_handle, FALSE);
					}
				}

				// Update the optional parameters.

				update_string(&selected_part_ptr->texture, 
					&selected_part_ptr->flags, PART_TEXTURE, window_handle,
					IDC_PART_TEXTURE);
				update_string(&selected_part_ptr->colour, 
					&selected_part_ptr->flags, PART_COLOUR, window_handle,
					IDC_PART_COLOUR);
				update_string(&selected_part_ptr->translucency, 
					&selected_part_ptr->flags, PART_TRANSLUCENCY, window_handle,
					IDC_PART_TRANSLUCENCY);
				update_string(&selected_part_ptr->style, 
					&selected_part_ptr->flags, PART_STYLE, window_handle,
					IDC_PART_STYLE);
				update_string(&selected_part_ptr->faces, 
					&selected_part_ptr->flags, PART_FACES, window_handle,
					IDC_PART_FACES);
				update_string(&selected_part_ptr->angle, 
					&selected_part_ptr->flags, PART_ANGLE, window_handle,
					IDC_PART_ANGLE);
				EndDialog(window_handle, TRUE);
				break;
			case IDCANCEL:
				EndDialog(window_handle, FALSE);
			}
			return(TRUE);
	default:
		return(FALSE);
	}
}

//------------------------------------------------------------------------------
// Handle editing of param tag.
//------------------------------------------------------------------------------

static BOOL CALLBACK
param_handler(HWND window_handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
	block *block_ptr = edited_block_def_ptr->block_ptr;
	switch (msg) {
	case WM_INITDIALOG:
		SetWindowText(GetDlgItem(window_handle, IDC_PARAM_ORIENT),
			block_ptr->param_orient);
		SetWindowText(GetDlgItem(window_handle, IDC_PARAM_SOLID),
			block_ptr->param_solid);
		SetWindowText(GetDlgItem(window_handle, IDC_PARAM_ALIGN),
			block_ptr->param_align);
		SetWindowText(GetDlgItem(window_handle, IDC_PARAM_SPEED),
			block_ptr->param_speed);
		SetWindowText(GetDlgItem(window_handle, IDC_PARAM_ANGLE),
			block_ptr->param_angle);
		return(TRUE);
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
			switch (LOWORD(wParam)) {
			case IDOK:
				update_string(&block_ptr->param_orient, 
					&block_ptr->flags, PARAM_ORIENT, window_handle,
					IDC_PARAM_ORIENT);
				update_string(&block_ptr->param_solid, 
					&block_ptr->flags, PARAM_SOLID, window_handle,
					IDC_PARAM_SOLID);
				update_string(&block_ptr->param_align, 
					&block_ptr->flags, PARAM_ALIGN, window_handle,
					IDC_PARAM_ALIGN);
				update_string(&block_ptr->param_speed, 
					&block_ptr->flags, PARAM_SPEED, window_handle,
					IDC_PARAM_SPEED);
				update_string(&block_ptr->param_angle, 
					&block_ptr->flags, PARAM_ANGLE, window_handle,
					IDC_PARAM_ANGLE);
				EndDialog(window_handle, TRUE);
				break;
			case IDCANCEL:
				EndDialog(window_handle, FALSE);
			}
			return(TRUE);
	default:
		return(FALSE);
	}
}

//------------------------------------------------------------------------------
// Handle editing of exit tag.
//------------------------------------------------------------------------------

static BOOL CALLBACK
exit_handler(HWND window_handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
	block *block_ptr = edited_block_def_ptr->block_ptr;
	switch (msg) {
	case WM_INITDIALOG:
		SetWindowText(GetDlgItem(window_handle, IDC_EXIT_HREF),
			block_ptr->exit_href);
		SetWindowText(GetDlgItem(window_handle, IDC_EXIT_TARGET),
			block_ptr->exit_target);
		SetWindowText(GetDlgItem(window_handle, IDC_EXIT_TEXT),
			block_ptr->exit_text);
		SetWindowText(GetDlgItem(window_handle, IDC_EXIT_TRIGGER),
			block_ptr->exit_trigger);
		return(TRUE);
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
			switch (LOWORD(wParam)) {
			case IDOK:
				update_string(&block_ptr->exit_href, 
					&block_ptr->flags, EXIT_HREF, window_handle,
					IDC_EXIT_HREF);
				update_string(&block_ptr->exit_target, 
					&block_ptr->flags, EXIT_TARGET, window_handle,
					IDC_EXIT_TARGET);
				update_string(&block_ptr->exit_text, 
					&block_ptr->flags, EXIT_TEXT, window_handle,
					IDC_EXIT_TEXT);
				update_string(&block_ptr->exit_trigger, 
					&block_ptr->flags, EXIT_TRIGGER, window_handle,
					IDC_EXIT_TRIGGER);
				EndDialog(window_handle, TRUE);
				break;
			case IDCANCEL:
				EndDialog(window_handle, FALSE);
			}
			return(TRUE);
	default:
		return(FALSE);
	}
}

//------------------------------------------------------------------------------
// Handle editing of light tag.
//------------------------------------------------------------------------------

static BOOL CALLBACK
light_handler(HWND window_handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
	block *block_ptr = edited_block_def_ptr->block_ptr;
	switch (msg) {
	case WM_INITDIALOG:
		if (block_ptr->point_light)
			SendMessage(GetDlgItem(window_handle, IDC_POINT_LIGHT),
				BM_SETCHECK, BST_CHECKED, 0);
		else
			SendMessage(GetDlgItem(window_handle, IDC_SPOT_LIGHT),
				BM_SETCHECK, BST_CHECKED, 0);
		SetWindowText(GetDlgItem(window_handle, IDC_LIGHT_STYLE),
			block_ptr->light_style);
		SetWindowText(GetDlgItem(window_handle, IDC_LIGHT_BRIGHTNESS),
			block_ptr->light_brightness);
		SetWindowText(GetDlgItem(window_handle, IDC_LIGHT_COLOUR),
			block_ptr->light_colour);
		SetWindowText(GetDlgItem(window_handle, IDC_LIGHT_RADIUS),
			block_ptr->light_radius);
		SetWindowText(GetDlgItem(window_handle, IDC_LIGHT_POSITION),
			block_ptr->light_position);
		SetWindowText(GetDlgItem(window_handle, IDC_LIGHT_SPEED),
			block_ptr->light_speed);
		SetWindowText(GetDlgItem(window_handle, IDC_LIGHT_FLOOD),
			block_ptr->light_flood);
		SetWindowText(GetDlgItem(window_handle, IDC_LIGHT_DIRECTION),
			block_ptr->light_direction);
		SetWindowText(GetDlgItem(window_handle, IDC_LIGHT_CONE),
			block_ptr->light_cone);
		return(TRUE);
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
			switch (LOWORD(wParam)) {
			case IDOK:
				if (SendMessage(GetDlgItem(window_handle, IDC_POINT_LIGHT),
					BM_GETCHECK, 0, 0) == BST_CHECKED)
					block_ptr->point_light = true;
				else
					block_ptr->point_light = false;
				update_string(&block_ptr->light_style, 
					&block_ptr->flags, LIGHT_STYLE, window_handle,
					IDC_LIGHT_STYLE);
				update_string(&block_ptr->light_brightness, 
					&block_ptr->flags, LIGHT_BRIGHTNESS, window_handle,
					IDC_LIGHT_BRIGHTNESS);
				update_string(&block_ptr->light_colour, 
					&block_ptr->flags, LIGHT_COLOUR, window_handle,
					IDC_LIGHT_COLOUR);
				update_string(&block_ptr->light_radius, 
					&block_ptr->flags, LIGHT_RADIUS, window_handle,
					IDC_LIGHT_RADIUS);
				update_string(&block_ptr->light_position, 
					&block_ptr->flags, LIGHT_POSITION, window_handle,
					IDC_LIGHT_POSITION);
				update_string(&block_ptr->light_speed, 
					&block_ptr->flags, LIGHT_SPEED, window_handle,
					IDC_LIGHT_SPEED);
				update_string(&block_ptr->light_flood, 
					&block_ptr->flags, LIGHT_FLOOD, window_handle,
					IDC_LIGHT_FLOOD);
				update_string(&block_ptr->light_direction, 
					&block_ptr->flags, LIGHT_DIRECTION, window_handle,
					IDC_LIGHT_DIRECTION);
				update_string(&block_ptr->light_cone, 
					&block_ptr->flags, LIGHT_CONE, window_handle,
					IDC_LIGHT_CONE);
				EndDialog(window_handle, TRUE);
				break;
			case IDCANCEL:
				EndDialog(window_handle, FALSE);
			}
			return(TRUE);
	default:
		return(FALSE);
	}
}

//------------------------------------------------------------------------------
// Handle editing of sound tag.
//------------------------------------------------------------------------------

static BOOL CALLBACK
sound_handler(HWND window_handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
	block *block_ptr = edited_block_def_ptr->block_ptr;
	switch (msg) {
	case WM_INITDIALOG:
		SetWindowText(GetDlgItem(window_handle, IDC_SOUND_FILE),
			block_ptr->sound_file);
		SetWindowText(GetDlgItem(window_handle, IDC_SOUND_RADIUS),
			block_ptr->sound_radius);
		SetWindowText(GetDlgItem(window_handle, IDC_SOUND_VOLUME),
			block_ptr->sound_volume);
		SetWindowText(GetDlgItem(window_handle, IDC_SOUND_PLAYBACK),
			block_ptr->sound_playback);
		SetWindowText(GetDlgItem(window_handle, IDC_SOUND_DELAY),
			block_ptr->sound_delay);
		SetWindowText(GetDlgItem(window_handle, IDC_SOUND_FLOOD),
			block_ptr->sound_flood);
		SetWindowText(GetDlgItem(window_handle, IDC_SOUND_ROLLOFF),
			block_ptr->sound_rolloff);
		return(TRUE);
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
			switch (LOWORD(wParam)) {
			case IDOK:
				update_string(&block_ptr->sound_file, 
					&block_ptr->flags, SOUND_FILE, window_handle,
					IDC_SOUND_FILE);
				update_string(&block_ptr->sound_radius, 
					&block_ptr->flags, SOUND_RADIUS, window_handle,
					IDC_SOUND_RADIUS);
				update_string(&block_ptr->sound_volume, 
					&block_ptr->flags, SOUND_VOLUME, window_handle,
					IDC_SOUND_VOLUME);
				update_string(&block_ptr->sound_playback, 
					&block_ptr->flags, SOUND_PLAYBACK, window_handle,
					IDC_SOUND_PLAYBACK);
				update_string(&block_ptr->sound_delay, 
					&block_ptr->flags, SOUND_DELAY, window_handle,
					IDC_SOUND_DELAY);
				update_string(&block_ptr->sound_flood, 
					&block_ptr->flags, SOUND_FLOOD, window_handle,
					IDC_SOUND_FLOOD);
				update_string(&block_ptr->sound_rolloff, 
					&block_ptr->flags, SOUND_ROLLOFF, window_handle,
					IDC_SOUND_ROLLOFF);
				EndDialog(window_handle, TRUE);
				break;
			case IDCANCEL:
				EndDialog(window_handle, FALSE);
			}
			return(TRUE);
	default:
		return(FALSE);
	}
}

//------------------------------------------------------------------------------
// Handle block window commands.
//------------------------------------------------------------------------------

static void
handle_block_command(HWND window_handle, int control_ID, HWND control_handle,
					 UINT notify_code)
{
	int index;
	char text[256];
	string name;
	char symbol;
	string double_symbol;
	block *block_ptr;

	// Get a pointer to the block being edited.

	block_ptr = edited_block_def_ptr->block_ptr;

	// If a selection change has occurred in the part list box, enable or
	// disable the edit part button.

	if (control_handle == part_list_box_handle && 
		notify_code == LBN_SELCHANGE) {
		index = SendMessage(part_list_box_handle, LB_GETCURSEL, 0, 0);
		if (index >= 0)
			EnableWindow(edit_part_button_handle, TRUE);
		else
			EnableWindow(edit_part_button_handle, FALSE);
	}

	// If the edit part tag button was clicked, get a pointer to the selected
	// part, then bring up the edit part tag dialog box.

	else if ((control_handle == edit_part_button_handle &&
		notify_code == BN_CLICKED) ||
		(control_handle == part_list_box_handle && notify_code == LBN_DBLCLK)) {
		index = SendMessage(part_list_box_handle, LB_GETCURSEL, 0, 0);
		if (index >= 0) {
			SendMessage(part_list_box_handle, LB_GETTEXT, index, (LPARAM)text);
			selected_part_ptr = block_ptr->find_part(text);
			DialogBox(instance_handle, MAKEINTRESOURCE(IDD_PART),
				window_handle, part_handler);
		}
	}

	// If the edit param tag button was clicked, bring up the edit param tag
	// dialog box.

	else if (control_handle == edit_param_button_handle &&
		notify_code == BN_CLICKED) {
		DialogBox(instance_handle, MAKEINTRESOURCE(IDD_PARAM),
			window_handle, param_handler);
	}

	// If the edit exit tag button was clicked, bring up the edit exit tag
	// dialog box.

	else if (control_handle == edit_exit_button_handle &&
		notify_code == BN_CLICKED) {
		DialogBox(instance_handle, MAKEINTRESOURCE(IDD_EXIT),
			window_handle, exit_handler);
	}


	// If the edit light tag button was clicked, bring up the edit light tag
	// dialog box.

	else if (control_handle == edit_light_button_handle &&
		notify_code == BN_CLICKED) {
		DialogBox(instance_handle, MAKEINTRESOURCE(IDD_LIGHT),
			window_handle, light_handler);
	}

	// If the edit sound tag button was clicked, bring up the edit sound tag
	// dialog box.

	else if (control_handle == edit_sound_button_handle &&
		notify_code == BN_CLICKED) {
		DialogBox(instance_handle, MAKEINTRESOURCE(IDD_SOUND),
			window_handle, sound_handler);
	}

	// If the ok button was clicked, set the block name, symbol and entrance
	// before destroying the block window.  If either the name or symbol field
	// is empty, don't let them close the block window.

	else if (control_handle == block_ok_button_handle &&
		notify_code == BN_CLICKED) {

		// Verify the name is set, and that it doesn't match the name of
		// another block.

		GetWindowText(block_name_edit_box_handle, text, 256);
		if (strlen(text) == 0) {
			message("Missing block name", "You must give the block a name");
			return;
		} else if (strcmp(block_ptr->name, text)) {
			if (style_ptr->name_unique(edited_block_def_ptr, text))
				blockset_changed = true;
			else {
				message("Duplicate block name", "There is already a block "
					"that uses the name '%s'", text);
				return;
			}
		}
		name = text;

		// Verify the symbol is set.

		GetWindowText(block_symbol_edit_box_handle, text, 256);
		if (strlen(text) == 0) {
			message("Missing block symbol", "You must give the block a symbol");
			return;
		} else if (edited_block_def_ptr->symbol != text[0]) {
			if (style_ptr->symbol_unique(edited_block_def_ptr, text[0]))
				blockset_changed = true;
			else {
				message("Duplicate block symbol", "There is already a block "
					"that uses the symbol '%c'", text[0]);
				return;
			}
		}
		symbol = text[0];

		// Set the double symbol.

		GetWindowText(block_double_edit_box_handle, text, 256);
		if (strlen(text) == 0)
			edited_block_def_ptr->flags &= ~BLOCK_DOUBLE_SYMBOL;
		else if (strlen(text) == 2)
			edited_block_def_ptr->flags |= BLOCK_DOUBLE_SYMBOL;
		else {
			message("Invalid double symbol", "The double symbol must consist "
				"of precisely two characters");
			return;
		}
		if (strcmp(edited_block_def_ptr->double_symbol, text)) {
			if (style_ptr->double_symbol_unique(edited_block_def_ptr, text))
				blockset_changed = true;
			else {
				message("Duplicate block double symbol", "There is already a "
					"block that uses the double symbol '%s'", text);
				return;
			}
		}
		double_symbol = text;

		// Update the name, symbol and double symbol fields.

		block_ptr->name = name;
		edited_block_def_ptr->symbol = symbol;
		edited_block_def_ptr->double_symbol = double_symbol;

		// Update the name field in the block list view.

		ListView_SetItemText(block_list_view_handle, 
			edited_block_def_ptr->item_index, 1, name);

		// Update the symbol field in the block list view.
		
		text[0] = symbol;
		if (edited_block_def_ptr->flags & BLOCK_DOUBLE_SYMBOL) {
			text[1] = ' ';
			text[2] = 'o';
			text[3] = 'r';
			text[4] = ' ';
			text[5] = double_symbol[0];
			text[6] = double_symbol[1];
			text[7] = '\0';
		} else
			text[1] = '\0';
		ListView_SetItemText(block_list_view_handle, 
			edited_block_def_ptr->item_index, 2, text);

		// Set or reset the entrance.

		GetWindowText(block_entrance_edit_box_handle, text, 256);
		if (strlen(text) == 0)
			block_ptr->flags &= ~BLOCK_ENTRANCE;
		else
			block_ptr->flags |= BLOCK_ENTRANCE;
		if (strcmp(block_ptr->entrance, text)) {
			blockset_changed = true;
			block_ptr->entrance = text;
		}

		// Destroy the edit block window.

		DestroyWindow(block_window_handle);
	}

	// If the cancel button was clicked, destroy the block window.

	else if (control_handle == block_cancel_button_handle &&
		notify_code == BN_CLICKED)
		DestroyWindow(block_window_handle);
}

//------------------------------------------------------------------------------
// Block window handler.
//------------------------------------------------------------------------------

static LRESULT CALLBACK
block_window_handler(HWND window_handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_DESTROY:
		EnableWindow(style_window_handle, TRUE);
		SetFocus(style_window_handle);
		return(0);
	HANDLE_MSG(window_handle, WM_COMMAND, handle_block_command);
	default:
		return(DefWindowProc(window_handle, msg, wParam, lParam));
	}
}

//------------------------------------------------------------------------------
// Edit a block.
//------------------------------------------------------------------------------

static void
edit_block(block_def *block_def_ptr, bool allow_cancel)
{
	int option;
	block *block_ptr;
	part *part_ptr;
	char symbol[2];

	// Remember the block definition being edited.

	edited_block_def_ptr = block_def_ptr;

	// Disable the style window.

	EnableWindow(style_window_handle, FALSE);

	// Create the block window.

	if (allow_cancel)
		option = WS_SYSMENU;
	else
		option = 0;
	block_window_handle = create_window("BlockWindow", "Edit block", 
		WS_OVERLAPPED | WS_CAPTION | option, WS_EX_OVERLAPPEDWINDOW |
		WS_EX_CONTROLPARENT, 0, 0, CW_USEDEFAULT, CW_USEDEFAULT, NULL);

	// Create the part list title and box.

	first_window_pos(style_window_handle, block_window_handle);
	part_list_title_handle = create_title("Part list", 250, 23, true);
	part_list_box_handle = create_list_box(250, 164, true);

	// Create the part tag edit button.

	edit_part_button_handle = create_button("Edit part tag", 250, 20, true);

	// Create the optional block tags title.

	block_tags_title_handle = create_title("Optional block tags", 500, 23, 
		true);

	// Create the optional block tag buttons.

	edit_param_button_handle = create_button("Edit param tag",
		125, 20, false);
	edit_exit_button_handle = create_button("Edit exit tag", 125, 20, false);
	edit_light_button_handle = create_button("Edit light tag", 125, 20,
		false);
	edit_sound_button_handle = create_button("Edit sound tag", 125, 20,
		true);

	// Create the block name, symbol and entrance edit boxes.

	new_window_col(250);
	window_y += 20;
	block_name_label_handle = create_label("NAME=", 75, 20, false);
	window_x += 5;
	block_name_edit_box_handle = create_edit_box(160, 20, true);
	window_y += 5;
	block_symbol_label_handle = create_label("SYMBOL=", 75, 20, false);
	window_x += 5;
	block_symbol_edit_box_handle = create_edit_box(30, 20, true);
	window_y += 5;
	block_double_label_handle = create_label("DOUBLE=", 75, 20, false);
	window_x += 5;
	block_double_edit_box_handle = create_edit_box(30, 20, true);
	window_y += 5;
	block_entrance_label_handle = create_label("ENTRANCE=", 75, 20, false);
	window_x += 5;
	block_entrance_edit_box_handle = create_edit_box(30, 20, true);

	// Create the ok and cancel buttons (if allowed).

	window_x += 10;
	window_y += 10;
	block_ok_button_handle = create_default_button("OK", 110, 20, false);
	if (allow_cancel) {
		window_x += 10;
		block_cancel_button_handle = create_button("Cancel", 110, 20, true);
	}
	last_window_pos();

	// Show the block window.

	ShowWindow(block_window_handle, SW_SHOWNORMAL);
	UpdateWindow(block_window_handle);

	// Initialise the block name, symbols and entrance.

	block_ptr = block_def_ptr->block_ptr;
	SetWindowText(block_name_edit_box_handle, block_ptr->name);
	symbol[0] = block_def_ptr->symbol;
	symbol[1] = '\0';
	SetWindowText(block_symbol_edit_box_handle, symbol);
	SetWindowText(block_double_edit_box_handle, block_def_ptr->double_symbol);
	SetWindowText(block_entrance_edit_box_handle, block_ptr->entrance);

	// Enable all controls other than the edit part tag button.

	EnableWindow(part_list_title_handle, TRUE);
	EnableWindow(part_list_box_handle, TRUE);
	EnableWindow(block_tags_title_handle, TRUE);
	EnableWindow(edit_param_button_handle, TRUE);
	EnableWindow(edit_light_button_handle, TRUE);
	EnableWindow(edit_sound_button_handle, TRUE);
	EnableWindow(edit_exit_button_handle, TRUE);
	EnableWindow(block_name_label_handle, TRUE);
	EnableWindow(block_name_edit_box_handle, TRUE);
	EnableWindow(block_symbol_label_handle, TRUE);
	EnableWindow(block_symbol_edit_box_handle, TRUE);
	EnableWindow(block_double_label_handle, TRUE);
	EnableWindow(block_double_edit_box_handle, TRUE);
	EnableWindow(block_entrance_label_handle, TRUE);
	EnableWindow(block_entrance_edit_box_handle, TRUE);
	EnableWindow(block_ok_button_handle, TRUE);
	EnableWindow(block_cancel_button_handle, TRUE);

	// Step through the part list, adding each part name to the part
	// list box.

	part_ptr = block_ptr->part_list;
	while (part_ptr) {
		SendMessage(part_list_box_handle, LB_ADDSTRING, 0, 
			(LPARAM)(char *)part_ptr->name);
		part_ptr = part_ptr->next_part_ptr;
	}
}

//------------------------------------------------------------------------------
// Handle renaming of a texture.
//------------------------------------------------------------------------------

static BOOL CALLBACK
rename_texture_handler(HWND window_handle, UINT msg, WPARAM wParam, 
					   LPARAM lParam)
{
	char old_file_name[256], new_file_name[256];
	string old_file_path, new_file_path;
	int index;

	switch (msg) {
	case WM_INITDIALOG:
		SetWindowText(window_handle, "Rename texture file");
		index = SendMessage(texture_list_box_handle, LB_GETCURSEL, 0, 0);
		SendMessage(texture_list_box_handle, LB_GETTEXT, index,
			(LPARAM)old_file_name);
		SetWindowText(GetDlgItem(window_handle, IDC_FILE_NAME),
			old_file_name);
		return(TRUE);
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
			switch (LOWORD(wParam)) {
			case IDOK:

				// Retrieve the old file name from the texture list box.

				index = SendMessage(texture_list_box_handle, LB_GETCURSEL, 0, 0);
				SendMessage(texture_list_box_handle, LB_GETTEXT, index,
					(LPARAM)old_file_name);

				// Get the new texture file name.  If it's the same as the
				// old, just close the dialog box.

				GetWindowText(GetDlgItem(window_handle, IDC_FILE_NAME), 
					new_file_name, 256);
				if (!strcmp(new_file_name, old_file_name)) {
					EndDialog(window_handle, TRUE);
					return(TRUE);
				}

				// Make sure the new file name is not blank and does not
				// include path delimiters.

				if (strlen(new_file_name) == 0) {
					message("Invalid texture file name", "You must enter a"
						"file name for the texture");
					return(TRUE);
				}
				if (strchr(new_file_name, '\\') || strchr(new_file_name, '/')) {
					message("Invalid texture file name", "It is not permissible "
						"to use '\\' or '/' in a texture file name");
					return(TRUE);
				}

				// If a texture already exists in the texture list with the
				// new file name, reject it.

				if (SendMessage(texture_list_box_handle, LB_FINDSTRINGEXACT, 0,
					(LPARAM)new_file_name) != LB_ERR) {
					message("Texture exists", "A texture with the file name "
						"'%s' already exists in the blockset", new_file_name);
					return(TRUE);
				}

				// Delete the old file name from the texture list box, then add
				// the new file name, making sure it is selected.

				SendMessage(texture_list_box_handle, LB_DELETESTRING, index, 0);
				index = SendMessage(texture_list_box_handle, LB_ADDSTRING, 0,
					(LPARAM)new_file_name);
				SendMessage(texture_list_box_handle, LB_SETCURSEL, index, 0);

				// Rename the file in the temporary textures directory.

				old_file_path = temp_textures_dir;
				old_file_path += old_file_name;
				new_file_path = temp_textures_dir;
				new_file_path += new_file_name;
				rename(old_file_path, new_file_path);

				// Indicate the blockset has changed, and close the dialog box
				// with a success status.

				blockset_changed = true;
				EndDialog(window_handle, TRUE);
				break;
			case IDCANCEL:
				EndDialog(window_handle, FALSE);
			}
			return(TRUE);
	default:
		return(FALSE);
	}
}

//------------------------------------------------------------------------------
// Handle renaming of a sound.
//------------------------------------------------------------------------------

static BOOL CALLBACK
rename_sound_handler(HWND window_handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
	char old_file_name[256], new_file_name[256];
	string old_file_path, new_file_path;
	int index;

	switch (msg) {
	case WM_INITDIALOG:
		SetWindowText(window_handle, "Rename sound file");
		index = SendMessage(sound_list_box_handle, LB_GETCURSEL, 0, 0);
		SendMessage(sound_list_box_handle, LB_GETTEXT, index,
			(LPARAM)old_file_name);
		SetWindowText(GetDlgItem(window_handle, IDC_FILE_NAME),
			old_file_name);
		return(TRUE);
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
			switch (LOWORD(wParam)) {
			case IDOK:

				// Retrieve the old file name from the sound list box.

				index = SendMessage(sound_list_box_handle, LB_GETCURSEL, 0, 0);
				SendMessage(sound_list_box_handle, LB_GETTEXT, index,
					(LPARAM)old_file_name);

				// Get the new sound file name.  If it's the same as the
				// old, just close the dialog box.

				GetWindowText(GetDlgItem(window_handle, IDC_FILE_NAME), 
					new_file_name, 256);
				if (!strcmp(new_file_name, old_file_name)) {
					EndDialog(window_handle, TRUE);
					return(TRUE);
				}

				// Make sure the new file name is not blank and does not 
				// include path delimiters.

				if (strlen(new_file_name) == 0) {
					message("Invalid sound file name", "You must enter a"
						"file name for the sound");
					return(TRUE);
				}
				if (strchr(new_file_name, '\\') || strchr(new_file_name, '/')) {
					message("Invalid sound file name", "It is not permissible "
						"to use '\\' or '/' in a sound file name");
					return(TRUE);
				}

				// If a sound already exists in the sound list with the
				// new file name, reject it.

				if (SendMessage(sound_list_box_handle, LB_FINDSTRINGEXACT, 0,
					(LPARAM)new_file_name) != LB_ERR) {
					message("Sound exists", "A sound with the file name "
						"'%s' already exists in the blockset", new_file_name);
					return(TRUE);
				}

				// Delete the old file name from the sound list box, then add
				// the new file name, making sure it's selected.

				SendMessage(sound_list_box_handle, LB_DELETESTRING, index, 0);
				index = SendMessage(sound_list_box_handle, LB_ADDSTRING, 0,
					(LPARAM)new_file_name);
				SendMessage(sound_list_box_handle, LB_SETCURSEL, index, 0);

				// Rename the file in the temporary sounds directory.

				old_file_path = temp_sounds_dir;
				old_file_path += old_file_name;
				new_file_path = temp_sounds_dir;
				new_file_path += new_file_name;
				rename(old_file_path, new_file_path);

				// Indicate the blockset has changed, and close the dialog box
				// with a success status.

				blockset_changed = true;
				EndDialog(window_handle, TRUE);
				break;
			case IDCANCEL:
				EndDialog(window_handle, FALSE);
			}
			return(TRUE);
	default:
		return(FALSE);
	}
}

//------------------------------------------------------------------------------
// Handle editing of version tag.
//------------------------------------------------------------------------------

static BOOL CALLBACK
version_handler(HWND window_handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		SetWindowText(GetDlgItem(window_handle, IDC_STYLE_VERSION),
			style_ptr->style_version);
		SetWindowText(GetDlgItem(window_handle, IDC_STYLE_NAME),
			style_ptr->style_name);
		SetWindowText(GetDlgItem(window_handle, IDC_STYLE_SYNOPSIS),
			style_ptr->style_synopsis);
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
			switch (LOWORD(wParam)) {
			case IDOK:
				update_string(&style_ptr->style_version, 
					&style_ptr->style_flags, STYLE_VERSION, window_handle,
					IDC_STYLE_VERSION);
				update_string(&style_ptr->style_name, 
					&style_ptr->style_flags, STYLE_NAME, window_handle,
					IDC_STYLE_NAME);
				update_string(&style_ptr->style_synopsis, 
					&style_ptr->style_flags, STYLE_SYNOPSIS, window_handle,
					IDC_STYLE_SYNOPSIS);
				EndDialog(window_handle, TRUE);
				break;
			case IDCANCEL:
				EndDialog(window_handle, FALSE);
			}
			return(TRUE);
	default:
		return(FALSE);
	}
}

//------------------------------------------------------------------------------
// Handle editing of placeholder tag.
//------------------------------------------------------------------------------

static BOOL CALLBACK
placeholder_handler(HWND window_handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		SetWindowText(GetDlgItem(window_handle, IDC_PLACEHOLDER_TEXTURE),
			style_ptr->placeholder_texture);
		return(TRUE);
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
			switch (LOWORD(wParam)) {
			case IDOK:
				update_string(&style_ptr->placeholder_texture, 
					&style_ptr->style_flags, PLACEHOLDER_TEXTURE, window_handle,
					IDC_PLACEHOLDER_TEXTURE);
				EndDialog(window_handle, TRUE);
				break;
			case IDCANCEL:
				EndDialog(window_handle, FALSE);
			}
			return(TRUE);
	default:
		return(FALSE);
	}
}

//------------------------------------------------------------------------------
// Handle editing of sky tag.
//------------------------------------------------------------------------------

static BOOL CALLBACK
sky_handler(HWND window_handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		SetWindowText(GetDlgItem(window_handle, IDC_SKY_TEXTURE),
			style_ptr->sky_texture);
		SetWindowText(GetDlgItem(window_handle, IDC_SKY_COLOUR),
			style_ptr->sky_colour);
		SetWindowText(GetDlgItem(window_handle, IDC_SKY_BRIGHTNESS),
			style_ptr->sky_brightness);
		return(TRUE);
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
			switch (LOWORD(wParam)) {
			case IDOK:
				update_string(&style_ptr->sky_texture, &style_ptr->style_flags,
					SKY_TEXTURE, window_handle, IDC_SKY_TEXTURE);
				update_string(&style_ptr->sky_colour, &style_ptr->style_flags,
					SKY_COLOUR, window_handle, IDC_SKY_COLOUR);
				update_string(&style_ptr->sky_brightness, 
					&style_ptr->style_flags, SKY_BRIGHTNESS, window_handle,
					IDC_SKY_BRIGHTNESS);
				EndDialog(window_handle, TRUE);
				break;
			case IDCANCEL:
				EndDialog(window_handle, FALSE);
			}
			return(TRUE);
	default:
		return(FALSE);
	}
}

//------------------------------------------------------------------------------
// Handle editing of ground tag.
//------------------------------------------------------------------------------

static BOOL CALLBACK
ground_handler(HWND window_handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		SetWindowText(GetDlgItem(window_handle, IDC_GROUND_TEXTURE),
			style_ptr->ground_texture);
		SetWindowText(GetDlgItem(window_handle, IDC_GROUND_COLOUR),
			style_ptr->ground_colour);
		return(TRUE);
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
			switch (LOWORD(wParam)) {
			case IDOK:
				update_string(&style_ptr->ground_texture, 
					&style_ptr->style_flags, GROUND_TEXTURE, window_handle,
					IDC_GROUND_TEXTURE);
				update_string(&style_ptr->ground_colour, 
					&style_ptr->style_flags, GROUND_COLOUR, window_handle,
					IDC_GROUND_COLOUR);
				EndDialog(window_handle, TRUE);
				break;
			case IDCANCEL:
				EndDialog(window_handle, FALSE);
			}
	default:
		return(FALSE);
	}
}

//------------------------------------------------------------------------------
// Handle editing of orb tag.
//------------------------------------------------------------------------------

static BOOL CALLBACK
orb_handler(HWND window_handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		SetWindowText(GetDlgItem(window_handle, IDC_ORB_TEXTURE),
			style_ptr->orb_texture);
		SetWindowText(GetDlgItem(window_handle, IDC_ORB_COLOUR),
			style_ptr->orb_colour);
		SetWindowText(GetDlgItem(window_handle, IDC_ORB_BRIGHTNESS),
			style_ptr->orb_brightness);
		SetWindowText(GetDlgItem(window_handle, IDC_ORB_POSITION),
			style_ptr->orb_position);
		SetWindowText(GetDlgItem(window_handle, IDC_ORB_HREF),
			style_ptr->orb_href);
		SetWindowText(GetDlgItem(window_handle, IDC_ORB_TARGET),
			style_ptr->orb_target);
		SetWindowText(GetDlgItem(window_handle, IDC_ORB_TEXT),
			style_ptr->orb_text);
		return(TRUE);
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
			switch (LOWORD(wParam)) {
			case IDOK:
				update_string(&style_ptr->orb_texture, 
					&style_ptr->style_flags, ORB_TEXTURE, window_handle,
					IDC_ORB_TEXTURE);
				update_string(&style_ptr->orb_colour, 
					&style_ptr->style_flags, ORB_COLOUR, window_handle,
					IDC_ORB_COLOUR);
				update_string(&style_ptr->orb_brightness, 
					&style_ptr->style_flags, ORB_BRIGHTNESS, window_handle,
					IDC_ORB_BRIGHTNESS);
				update_string(&style_ptr->orb_position, 
					&style_ptr->style_flags, ORB_POSITION, window_handle,
					IDC_ORB_POSITION);
				update_string(&style_ptr->orb_href, 
					&style_ptr->style_flags, ORB_HREF, window_handle,
					IDC_ORB_HREF);
				update_string(&style_ptr->orb_target, 
					&style_ptr->style_flags, ORB_TARGET, window_handle,
					IDC_ORB_TARGET);
				update_string(&style_ptr->orb_text, 
					&style_ptr->style_flags, ORB_TEXT, window_handle,
					IDC_ORB_TEXT);
				EndDialog(window_handle, TRUE);
				break;
			case IDCANCEL:
				EndDialog(window_handle, FALSE);
			}
			return(TRUE);
	default:
		return(FALSE);
	}
}

//------------------------------------------------------------------------------
// Handle about dialog box.
//------------------------------------------------------------------------------

static BOOL CALLBACK
about_handler(HWND window_handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		return(TRUE);
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
			switch (LOWORD(wParam)) {
			case IDOK:
				EndDialog(window_handle, TRUE);
				break;
			case IDCANCEL:
				EndDialog(window_handle, FALSE);
			}
			return(TRUE);
	default:
		return(FALSE);
	}
}
//------------------------------------------------------------------------------
// Handle style window commands.
//------------------------------------------------------------------------------

static void
handle_style_command(HWND window_handle, int control_ID, HWND control_handle,
					 UINT notify_code)
{
	int index;
	LV_FINDINFO find_info;
#ifdef SUPPORT_DXF
	bool DXF_file;
#endif
	char block_name[_MAX_PATH];
	char file_name[_MAX_PATH];
	char file_path[_MAX_PATH];
	char *file_name_ptr, *ext_ptr;
	block *block_ptr;
	block_def *block_def_ptr;

	// If the add block button was clicked...

	if (control_handle == add_block_button_handle &&
		notify_code == BN_CLICKED) {

		// Get the file name of the new block.

		if (get_open_file_name("Add block", BLOCK_FILETYPES, blocks_dir)) {

			// Extract the block file name from the chosen file path.  If the
			// chosen file is a DXF file, the extension needs to be changed.

			file_name_ptr = strrchr(user_file_path, '\\') + 1;
			strcpy(file_name, file_name_ptr);
			ext_ptr = strrchr(file_name, '.');
#ifdef SUPPORT_DXF
			if (!stricmp(ext_ptr, ".dxf"))
				DXF_file = true;
			else
				DXF_file = false;
#endif
			*ext_ptr = '\0';
			strcpy(block_name, file_name);
			strcat(file_name, ".block");

			// If the block already exists in the block list with the given
			// block file name, reject it.

			find_info.flags = LVFI_STRING;
			find_info.psz = file_name;
			index = ListView_FindItem(block_list_view_handle, -1, &find_info);
			if (index >= 0)
				message("Block exists", "A block with the file name '%s' "
					"already exists in the blockset", file_name);

			// Otherwise load the block and add it to the block list.

			else {
				
				// Extract the directory path to the block file.

				strcpy(blocks_dir, user_file_path);
				file_name_ptr = strrchr(blocks_dir, '\\');
				*file_name_ptr = '\0';

#ifdef SUPPORT_DXF

				// Read the DXF or block file, creating a new block.

				if (DXF_file)
					block_ptr = read_DXF_file(user_file_path, block_name);
				else
					block_ptr = read_block_file(user_file_path);

#else

				// Read the block file, creating a new block.

				block_ptr = read_block_file(user_file_path);

#endif

				// If the block was created, add it to the block list, and if
				// that succeeded bring up the edit block dialog box.

				if (block_ptr && (block_def_ptr = add_block_to_style(0, "", 
					file_name, block_ptr)) != NULL) {
					blockset_changed = true;
					edit_block(block_def_ptr, false);
				}
			}
		}
	}

	// If the remove block button was clicked, remove the currently selected
	// block definition, update the block list, then disable the remove, rename
	// and edit block buttons.

	else if (control_handle == remove_block_button_handle &&
		notify_code == BN_CLICKED) {
		if (selected_block_def_ptr) {
			blockset_changed = true;
			index = selected_block_def_ptr->item_index;
			style_ptr->del_block_def(selected_block_def_ptr);
			ListView_DeleteItem(block_list_view_handle, index);
			EnableWindow(remove_block_button_handle, FALSE);
			EnableWindow(rename_block_button_handle, FALSE);
			EnableWindow(edit_block_button_handle, FALSE);
		}
	}

	// If the rename block button was clicked, bring up the rename block 
	// window.

	else if (control_handle == rename_block_button_handle &&
		notify_code == BN_CLICKED)
		DialogBox(instance_handle, MAKEINTRESOURCE(IDD_RENAME),
			window_handle, rename_block_handler);

	// If the edit block button was clicked, bring up the edit block window.

	else if (control_handle == edit_block_button_handle &&
		notify_code == BN_CLICKED)
		edit_block(selected_block_def_ptr, true);

	// If the add BSP trees button was clicked, add a BSP tree to the blocks
	// that need them.

	else if (control_handle == add_BSP_trees_button_handle &&
		notify_code == BN_CLICKED)
		add_BSP_trees();

	// If a selection change has occurred in the texture list box, enable or
	// disable the remove texture and rename texture buttons.

	else if (control_handle == texture_list_box_handle &&
		notify_code == LBN_SELCHANGE) {
		index = SendMessage(texture_list_box_handle, LB_GETCURSEL, 0, 0);
		if (index >= 0) {
			EnableWindow(remove_texture_button_handle, TRUE);
			EnableWindow(rename_texture_button_handle, TRUE);
		} else {
			EnableWindow(remove_texture_button_handle, FALSE);
			EnableWindow(rename_texture_button_handle, FALSE);
		}
	}

	// If the add texture button was clicked, get the file path of a texture to
	// add, copy it to the textures directory and update the texture list,
	// then disable the remove texture button.

	else if (control_handle == add_texture_button_handle &&
		notify_code == BN_CLICKED) {
		if (get_open_file_name("Add texture", TEXTURE_FILETYPES, textures_dir)) {
			file_name_ptr = strrchr(user_file_path, '\\') + 1;
			if (SendMessage(texture_list_box_handle, LB_FINDSTRINGEXACT, 0,
				(LPARAM)file_name_ptr) != LB_ERR)
				message("Texture exists", "A texture with the file name '%s' "
					"already exists in the blockset", file_name_ptr);
			else {

				// Construct the destination file path.

				strcpy(file_path, temp_textures_dir);
				strcat(file_path, file_name_ptr);

				// Copy the texture file and update the texture list box.

				if (copy_file(user_file_path, file_path)) {
					blockset_changed = true;
					SendMessage(texture_list_box_handle, LB_ADDSTRING, 0,
						(LPARAM)file_name_ptr);
				}

				// Extract the directory path from the texture file.

				strcpy(textures_dir, user_file_path);
				file_name_ptr = strrchr(textures_dir, '\\');
				*file_name_ptr = '\0';
			}
		}
	}

	// If the remove texture button was clicked, remove the selected texture
	// file from the textures directory and it's name from the texture list box,
	// then disable the remove texture and rename texture buttons.

	else if (control_handle == remove_texture_button_handle &&
		notify_code == BN_CLICKED) {
		index = SendMessage(texture_list_box_handle, LB_GETCURSEL, 0, 0);
		if (index >= 0) {
			blockset_changed = true;
			SendMessage(texture_list_box_handle, LB_GETTEXT, index,
				(LPARAM)file_name);
			strcpy(file_path, temp_textures_dir);
			strcat(file_path, file_name);
			remove(file_path);
			SendMessage(texture_list_box_handle, LB_DELETESTRING, index, 0);
			EnableWindow(remove_texture_button_handle, FALSE);
			EnableWindow(rename_texture_button_handle, FALSE);
		}
	}

	// If the rename texture button was clicked, or the name in the list box
	// was double clicked, bring up the rename texture dialog box.

	else if ((control_handle == rename_texture_button_handle &&
		notify_code == BN_CLICKED) || 
		(control_handle == texture_list_box_handle && 
		notify_code == LBN_DBLCLK))
		DialogBox(instance_handle, MAKEINTRESOURCE(IDD_RENAME),
			window_handle, rename_texture_handler);

	// If a selection change has occurred in the sound list box, enable or
	// disable the remove sound and rename sound buttons.

	if (control_handle == sound_list_box_handle && 
		notify_code == LBN_SELCHANGE) {
		index = SendMessage(sound_list_box_handle, LB_GETCURSEL, 0, 0);
		if (index >= 0) {
			EnableWindow(remove_sound_button_handle, TRUE);
			EnableWindow(rename_sound_button_handle, TRUE);
		} else {
			EnableWindow(remove_sound_button_handle, FALSE);
			EnableWindow(rename_sound_button_handle, FALSE);
		}
	}

	// If the add sound button was clicked, copy the selected sound file to the
	// sounds directory and update the sound list.

	else if (control_handle == add_sound_button_handle &&
		notify_code == BN_CLICKED) {
		if (get_open_file_name("Add sound", SOUND_FILETYPE, sounds_dir)) {
			file_name_ptr = strrchr(user_file_path, '\\') + 1;
			if (SendMessage(sound_list_box_handle, LB_FINDSTRINGEXACT, 0,
				(LPARAM)file_name_ptr) != LB_ERR)
				message("Sound exists", "A sound with the file name '%s' "
					"already exists in the blockset", file_name_ptr);
			else {

				// Construct the destination file path.

				strcpy(file_path, temp_sounds_dir);
				strcat(file_path, file_name_ptr);

				// Copy the sound file and update the sound list box.

				if (copy_file(user_file_path, file_path)) {
					blockset_changed = true;
					SendMessage(sound_list_box_handle, LB_ADDSTRING, 0,
						(LPARAM)file_name_ptr);
				}

				// Extract the directory path from the sound file.

				strcpy(sounds_dir, user_file_path);
				file_name_ptr = strrchr(sounds_dir, '\\');
				*file_name_ptr = '\0';
			}
		}
	}

	// If the remove sound button was clicked, remove the selected sound file
	// from the sounds directory and it's name from the sound list box, then
	// disable the remove sound and rename sound buttons.

	else if (control_handle == remove_sound_button_handle &&
		notify_code == BN_CLICKED) {
		index = SendMessage(sound_list_box_handle, LB_GETCURSEL, 0, 0);
		if (index >= 0) {
			blockset_changed = true;
			SendMessage(sound_list_box_handle, LB_GETTEXT, index,
				(LPARAM)file_name);
			strcpy(file_path, temp_sounds_dir);
			strcat(file_path, file_name);
			remove(file_path);
			SendMessage(sound_list_box_handle, LB_DELETESTRING, index, 0);
			EnableWindow(remove_sound_button_handle, FALSE);
			EnableWindow(rename_sound_button_handle, FALSE);
		}
	}

	// If the rename sound button was clicked, bring up the rename sound
	// dialog box.

	else if ((control_handle == rename_sound_button_handle &&
		notify_code == BN_CLICKED) ||
		(control_handle == sound_list_box_handle && 
		notify_code == LBN_DBLCLK))
		DialogBox(instance_handle, MAKEINTRESOURCE(IDD_RENAME),
			window_handle, rename_sound_handler);

	// If the edit version tag button was clicked, bring up the edit version
	// tag dialog box.

	else if (control_handle == edit_version_button_handle &&
		notify_code == BN_CLICKED)
		DialogBox(instance_handle, MAKEINTRESOURCE(IDD_VERSION),
			window_handle, version_handler);

	// If the edit placeholder tag button was clicked, bring up the edit 
	// placeholder tag dialog box.

	else if (control_handle == edit_placeholder_button_handle &&
		notify_code == BN_CLICKED)
		DialogBox(instance_handle, MAKEINTRESOURCE(IDD_PLACEHOLDER),
			window_handle, placeholder_handler);

	// If the edit sky tag button was clicked, bring up the edit sky tag dialog
	// box.

	else if (control_handle == edit_sky_button_handle &&
		notify_code == BN_CLICKED)
		DialogBox(instance_handle, MAKEINTRESOURCE(IDD_SKY),
			window_handle, sky_handler);

	// If the edit ground tag button was clicked, bring up the edit ground tag
	// dialog box.

	else if (control_handle == edit_ground_button_handle &&
		notify_code == BN_CLICKED)
		DialogBox(instance_handle, MAKEINTRESOURCE(IDD_GROUND),
			window_handle, ground_handler);

	// If the edit orb tag button was clicked, bring up the edit orb tag
	// dialog box.

	else if (control_handle == edit_orb_button_handle &&
		notify_code == BN_CLICKED)
		DialogBox(instance_handle, MAKEINTRESOURCE(IDD_ORB),
			window_handle, orb_handler);

	// The command must be a menu item selection...

	else {
		switch (control_ID) {

		// If the New file menu item was selected, create a new blockset.

		case MENU_FILE_NEW:
			try {
				if ((style_ptr = new style) == NULL)
					memory_error("style");
			}
			catch (char *error_str) {
				message("Error", error_str);
				break;
			}
			create_temporary_directories();
			init_style_window();
			blockset_file_path[0] = '\0';
			blockset_loaded = true;
			blockset_changed = false;
			break;

		// If the Open file menu item was selected then open a blockset.

		case MENU_FILE_OPEN:
			open_blockset();
			break;

		// If the Save file menu item was selected, save the current blockset
		// in the current blockset file, if there is one.  Otherwise fall
		// through to the Save As case.

		case MENU_FILE_SAVE:
			if (blockset_file_path[0] != '\0') {
				if (save_blockset(blockset_file_path))
					blockset_changed = false;
				break;
			}

		// If the Save As file menu item was selected, save the current
		// blockset in a user selected file.

		case MENU_FILE_SAVE_AS:
			if (get_save_file_name("Save Blockset As...", BLOCKSET_FILETYPE,
				blockset_file_path)) {
				if (save_blockset(user_file_path))
					blockset_changed = false;
			}
			break;

		// If the Close file menu item was selected, close the blockset.

		case MENU_FILE_CLOSE:
			close_blockset();
			break;

		// If the Exit file menu item was selected, post a quit message.

		case MENU_FILE_EXIT:
			PostQuitMessage(0);
			break;

		// If the About help menu item was selected, display the about dialog
		// box.

		case MENU_HELP_ABOUT:
			DialogBox(instance_handle, MAKEINTRESOURCE(IDD_ABOUT),
				window_handle, about_handler);
		}
	}
}

//------------------------------------------------------------------------------
// Handle notify messages from the block list view control.
//------------------------------------------------------------------------------

static LRESULT
block_list_handler(HWND window_handle, int control_ID, NMHDR *notify_ptr)
{
	int item_index;
	LV_ITEM list_view_item;

	switch(notify_ptr->code)
	{
	case LVN_ITEMCHANGED:

		// Determine which item was selected.

		item_index = ListView_GetNextItem(block_list_view_handle, -1, 
			LVNI_FOCUSED);
		if (item_index >= 0) {

			// Determine which block was selected.

			list_view_item.mask = LVIF_PARAM;
			list_view_item.iItem = item_index;
			list_view_item.iSubItem = 0;
			ListView_GetItem(block_list_view_handle, &list_view_item);
			selected_block_def_ptr = (block_def *)list_view_item.lParam;
			selected_block_def_ptr->item_index = item_index;

			// Enable the remove, rename and edit block buttons.

			EnableWindow(remove_block_button_handle, TRUE);
			EnableWindow(rename_block_button_handle, TRUE);
			EnableWindow(edit_block_button_handle, TRUE);
		} 
		
		// If no item was selected, disable the remove, rename and edit block 
		// buttons.
 		
		else {
			EnableWindow(remove_block_button_handle, FALSE);
			EnableWindow(rename_block_button_handle, FALSE);
			EnableWindow(edit_block_button_handle, FALSE);
		}
		break;
	case LVN_ITEMACTIVATE:
		edit_block(selected_block_def_ptr, true);
	}
	return(FORWARD_WM_NOTIFY(window_handle, control_ID, notify_ptr,
		DefWindowProc));
}

//------------------------------------------------------------------------------
// Main window procedure.
//------------------------------------------------------------------------------

static LRESULT CALLBACK
style_window_handler(HWND window_handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_CLOSE:
		DestroyWindow(window_handle);
		return(0);
	case WM_DESTROY:
		PostQuitMessage(0);
		return(0);
	HANDLE_MSG(window_handle, WM_COMMAND, handle_style_command);
	HANDLE_MSG(window_handle, WM_NOTIFY, block_list_handler);
	default:
		return(DefWindowProc(window_handle, msg, wParam, lParam));
	}
}

//==============================================================================
// Main program.
//==============================================================================

int WINAPI 
WinMain(HINSTANCE curr_instance_handle, HINSTANCE prev_instance_handle,
		LPSTR command_line, int show_window_command)
{
	char *file_name_ptr;
	FILE *fp;
	WNDCLASS main_window_class;
	MSG msg;

	// Determine the application directory.

	GetModuleFileName(NULL, app_dir, _MAX_PATH);
	file_name_ptr = strrchr(app_dir, '\\') + 1;
	*file_name_ptr = '\0';

	// Create the paths to the temporary directories.

	temp_dir = app_dir;
	temp_dir += "temp\\";
	temp_blocks_dir = temp_dir;
	temp_blocks_dir += "blocks\\";
	temp_textures_dir = temp_dir;
	temp_textures_dir += "textures\\";
	temp_sounds_dir = temp_dir;
	temp_sounds_dir += "sounds\\";

	// Create the log file path and empty the log file.

	strcpy(log_file_path, app_dir);
	strcat(log_file_path, "log.txt");
	if ((fp = fopen(log_file_path, "w")) != NULL)
		fclose(fp);

	// Create the configuration file path.  Then attempt to open this file and
	// read the blocksets, blocks, textures and sounds directory paths.

	strcpy(config_file_path, app_dir);
	strcat(config_file_path, "config.txt");
	blocksets_dir[0] = '\0';
	blocks_dir[0] = '\0';
	textures_dir[0] = '\0';
	sounds_dir[0] = '\0';
	if ((fp = fopen(config_file_path, "r")) != NULL) {
		fgets(blocksets_dir, _MAX_PATH, fp);
		strip_cr(blocksets_dir);
		fgets(blocks_dir, _MAX_PATH, fp);
		strip_cr(blocks_dir);
		fgets(textures_dir, _MAX_PATH, fp);
		strip_cr(textures_dir);
		fgets(sounds_dir, _MAX_PATH, fp);
		strip_cr(sounds_dir);
		fclose(fp);
	}

	// Initialise the instance handle, parser and the common controls.

	instance_handle = curr_instance_handle;
	init_parser();
	InitCommonControls();

	// Initialise global variables.

	selected_block_def_ptr = NULL;
	blockset_loaded = false;

	// Register the style window class.

	main_window_class.style = CS_HREDRAW | CS_VREDRAW;
	main_window_class.lpfnWndProc = style_window_handler;
	main_window_class.cbClsExtra = 0;
	main_window_class.cbWndExtra = 0;
	main_window_class.hInstance = instance_handle;
	main_window_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	main_window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	main_window_class.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	main_window_class.lpszMenuName = MAKEINTRESOURCE(IDR_MENU);
	main_window_class.lpszClassName = "StyleWindow";
	RegisterClass(&main_window_class);

	// Register the block window class.

	main_window_class.style = CS_HREDRAW | CS_VREDRAW;
	main_window_class.lpfnWndProc = block_window_handler;
	main_window_class.cbClsExtra = 0;
	main_window_class.cbWndExtra = 0;
	main_window_class.hInstance = instance_handle;
	main_window_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	main_window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	main_window_class.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	main_window_class.lpszMenuName = NULL;
	main_window_class.lpszClassName = "BlockWindow";
	RegisterClass(&main_window_class);

	// Create the style window.

	style_window_handle = create_window("StyleWindow", "EditBset", 
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, 
		WS_EX_OVERLAPPEDWINDOW, 0, 0, CW_USEDEFAULT, CW_USEDEFAULT, NULL);

	// Create a title for the block list.

	first_window_pos(NULL, style_window_handle);
	block_list_title_handle = create_title("Block list", 500, 23, true);

	// Create the block list box.

	block_list_view_handle = create_list_view(510, 220, true);
	add_list_view_column(block_list_view_handle, "File name", 0, 150);
	add_list_view_column(block_list_view_handle, "Name", 1, 120);
	add_list_view_column(block_list_view_handle, "Symbol", 2, 60);
	add_list_view_column(block_list_view_handle, "Type", 3, 100);
	add_list_view_column(block_list_view_handle, "BSP tree", 4, 60);

	// Create the block list buttons.

	add_block_button_handle = create_button("Add block", 102, 20, false);
	remove_block_button_handle = create_button("Remove block", 102, 20, false);
	rename_block_button_handle = create_button("Rename block", 102, 20, false);
	edit_block_button_handle = create_button("Edit block", 102, 20, false);
	add_BSP_trees_button_handle = create_button("Add BSP trees", 102, 20, true);

	// Create a title for the texture, sound list and part lists.

	texture_list_title_handle = create_title("Texture list", 255, 23, false);
	sound_list_title_handle = create_title("Sound list", 255, 23, true);

	// Create the texture, sound and part lists.

	texture_list_box_handle = create_list_box(255, 164, false);
	sound_list_box_handle = create_list_box(255, 164, true);

	// Create the texture, sound and part list buttons.

	add_texture_button_handle = create_button("Add texture", 85, 20, false);
	remove_texture_button_handle = create_button("Remove texture", 85, 20,
		false);
	rename_texture_button_handle = create_button("Rename texture", 85, 20,
		false);
	add_sound_button_handle = create_button("Add sound", 85, 20, false);
	remove_sound_button_handle = create_button("Remove sound", 85, 20, false);
	rename_sound_button_handle = create_button("Rename sound", 85, 20, true);

	// Create the optional style tags title.

	style_tags_title_handle = create_title("Optional tags", 510, 23, true);

	// Create the optional style tag buttons.

	edit_version_button_handle = create_button("Edit version tag",
		102, 20, false);
	edit_placeholder_button_handle = create_button("Edit placeholder tag",
		102, 20, false);
	edit_sky_button_handle = create_button("Edit sky tag", 102, 20, false);
	edit_ground_button_handle = create_button("Edit ground tag", 102, 20,
		false);
	edit_orb_button_handle = create_button("Edit orb tag", 102, 20, true);
	last_window_pos();

	// Show the style window.

	ShowWindow(style_window_handle, show_window_command);
	UpdateWindow(style_window_handle);

	// Enter event loop.

	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// Close the blockset if there is one loaded.

	if (blockset_loaded)
		close_blockset();

	// Save the blocksets, blocks, textures and sounds directory paths to
	// the configuration file.

	if ((fp = fopen(config_file_path, "w")) != NULL) {
		fprintf(fp, "%s\n", blocksets_dir);
		fprintf(fp, "%s\n", blocks_dir);
		fprintf(fp, "%s\n", textures_dir);
		fprintf(fp, "%s\n", sounds_dir);
		fclose(fp);
	}
	return(msg.wParam);
}