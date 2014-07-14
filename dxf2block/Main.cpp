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
// The Original Code is DXF2Block. 
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
#include <io.h>
#include "classes.hpp"
#include "parser.hpp"

static char block_name[_MAX_PATH];
static int block_vertex_index;
static double block_vertex_x[24], block_vertex_y[24], block_vertex_z[24];

//------------------------------------------------------------------------------
// Parse a 3DFACE entity.
//------------------------------------------------------------------------------

bool
parse_3D_face(block *block_ptr)
{
	int param, index;
	int vertices;
	double x[4], y[4], z[4];
	int vertex_no_list[4];
	char part_name[BUFSIZ], *end_ptr;
	int part_no;

	// Read each parameter of the 3D face until we run out of parameters.

	if (!read_int(&param))
		return(false);
	while (param != 0) {
		switch (param) {

		// If the parameter is a layer name, parse it as a part name.  All
		// characters after the first underscore are ignored.

		case 8:
			if (!read_next_line())
				return(false);
			end_ptr = strchr(line, '_');
			if (end_ptr) {
				strncpy(part_name, line, end_ptr - line);
				part_name[end_ptr - line] = '\0';
			} else
				strcpy(part_name, line);
			break;

		// If the parameter is an x, y or z coordinate, parse it.  Note that
		// the Y and Z coordinates are swapped to match the coordinate system
		// used by blocks.

		case 10:
		case 11:
		case 12:
		case 13:
			if (!read_double(&x[param - 10]))
				return(false);
			break;

		case 20:
		case 21:
		case 22:
		case 23:
			if (!read_double(&z[param - 20]))
				return(false);
			break;

		case 30:
		case 31:
		case 32:
		case 33:
			if (!read_double(&y[param - 30]))
				return(false);
			break;

		// Skip over all other parameters.

		default:
			if (!read_next_line())
				return(false);
			break;
		}

		// Read start of next parameter.

		if (!read_int(&param))
			return(false);
	}

	// If the part name is "block", then store the vertices of this face in a
	// special set of arrays, then return.

	if (!strnicmp(part_name, "block", 5)) {
		for (index = 0; index < 4; index++) {
			block_vertex_x[block_vertex_index] = x[index];
			block_vertex_y[block_vertex_index] = y[index];
			block_vertex_z[block_vertex_index] = z[index];
			block_vertex_index++;
		}
		return(true);
	}

	// If the part name is new, create a new part object.

	if (!(part_no = block_ptr->find_part(part_name)) && 
		!(part_no = block_ptr->new_part(part_name))) {
		memory_error("part object");
		return(false);
	}

	// If the last two vertices are identical, then the face is a triangle,
	// otherwise it is a rectangle.

	if (x[2] == x[3] && y[2] == y[3] && z[2] == z[3])
		vertices = 3;
	else
		vertices = 4;

	// Add the vertices to the block, remembering the vertex numbers.

	for (index = 0; index < vertices; index++) {
		if (!(vertex_no_list[index] = block_ptr->add_vertex(x[index], y[index],
			  z[index]))) {
			memory_error("vertex object");
			return(false);
		}
	}
		
	// Create a new polygon object that references these vertices.

	if (!block_ptr->new_polygon(part_no, vertices, vertex_no_list)) { 
		memory_error("polygon object");
		return(false);	
	}
	return(true);
}

//------------------------------------------------------------------------------
// Parse a POLYLINE entity.
//------------------------------------------------------------------------------

bool
parse_polyline(block *block_ptr)
{
	int param, index;
	int vertices;
	double x[32], y[32], z[32];
	int vertex_no_list[32];
	char part_name[BUFSIZ], *end_ptr;
	int part_no;
	bool done;

	// Read each parameter of the polyline until we encounter "SEQEND"

	if (!read_int(&param))
		return(false);
	done = false;
	vertices = 0;
	while (!done) {
		switch (param) {

		// If the parameter is a layer name, parse it as a part name.

		case 8:
			if (!read_next_line())
				return(false);
			end_ptr = strchr(line, '_');
			if (end_ptr) {
				strncpy(part_name, line, end_ptr - line);
				part_name[end_ptr - line] = '\0';
			} else
				strcpy(part_name, line);
			break;

		// If the parameter is "ENDSEQ" or "VERTEX"...

		case 0:
			read_next_line();

			// If the parameter is "ENDSEQ", break out of this loop.

			if (line_is("SEQEND")) {
				done = true;
				break;
			}
			
			// If the parameter is not "VERTEX", this is an error.

			if (!verify_line("VERTEX"))
				return(false);

			// Read the vertex parameters.

			if (!read_int(&param))
				return(false);
			while (param != 0) {
				switch (param) {
				case 10:
					if (!read_double(&x[vertices]))
						return(false);
					break;
				case 20:
					if (!read_double(&z[vertices]))
						return(false);
					break;
				case 30:
					if (!read_double(&y[vertices]))
						return(false);
					break;
				default:
					if (!read_next_line())
						return(false);
				}
				if (!read_int(&param))
					return(false);
			}

			// Increment the vertex count and continue parsing POLYLINE
			// parameters.

			vertices++;
			continue;

		// Skip over all other parameters.

		default:
			if (!read_next_line())
				return(false);
		}

		// Read start of next parameter.

		if (!read_int(&param))
			return(false);
	}

	// If the part name is new, create a new part object.

	if (!(part_no = block_ptr->find_part(part_name)) && 
		!(part_no = block_ptr->new_part(part_name))) {
		memory_error("part object");
		return(false);
	}

	// Add the vertices to the block, remembering the vertex numbers.

	for (index = 0; index < vertices; index++) {
		if (!(vertex_no_list[index] = block_ptr->add_vertex(x[index], y[index],
			  z[index]))) {
			memory_error("vertex object");
			return(false);
		}
	}
		
	// Create a new polygon object that references these vertices.

	if (!block_ptr->new_polygon(part_no, vertices, vertex_no_list)) { 
		memory_error("polygon object");
		return(false);	
	}
	return(true);
}

//------------------------------------------------------------------------------
// Parse a DXF file.
//------------------------------------------------------------------------------

bool
parse_DXF_file(block *block_ptr)
{
	int param;

	// Initialise the block vertex index.

	block_vertex_index = 0;

	// Parse each section of the DXF file...

	read_int(&param);
	if (param != 0)
		return(false);
	read_next_line();
	while (line_is("SECTION")) {

		// Determine what type of section it is.

		read_int(&param);
		if (param != 2)
			return(false);
		read_next_line();

		// If this is a HEADER, TABLES or BLOCKS section, skip over it.

		if (line_is("HEADER") || line_is("TABLES") || line_is("BLOCKS")) {
			read_next_line();
			while (!line_is("ENDSEC"))
				read_next_line();
		}

		// If this is an ENTITIES section, parse each entity...

		else if (verify_line("ENTITIES")) {
			read_int(&param);
			if (param != 0)
				return(false);
			read_next_line();
			while (!line_is("ENDSEC")) {
				
				// If this entity is "3DFACE", then parse a 3D face (triangle
				// or rectangle).

				if (line_is("3DFACE")) {
					if (!parse_3D_face(block_ptr))
						return(false);
				} 
				
				// If this entity is "POLYLINE", then parse a polygon that may
				// have more than 4 sides.

				else if (line_is("POLYLINE")) {
					if (!parse_polyline(block_ptr))
						return(false);
				}
				
				// Any other entity type is an error.

				else {
					file_error("Unsupported entity type %s", line);
					return(false);
				}

				// Read the start of the next entity.

				read_next_line();
			}
		}

		// Any other section type indicates a corrupt DXF file.

		else {
			file_error("unrecognised section %s", line);
			return(false);
		}

		// Parse start of next section.

		read_int(&param);
		if (param != 0)
			return(false);
		read_next_line();
	}

	// Make sure there were six "block" faces parsed.

	if (block_vertex_index != 24) {
		file_error("only %d \"block\" faces were parsed", 
			block_vertex_index / 4);
		return(false);
	}

	// Make sure the DXF file ends with "EOF".

	return(verify_line("EOF"));
}

//------------------------------------------------------------------------------
// Adjust the vertex list so that the X, Y and Z coordinates are in the range
// of 0..256.
//------------------------------------------------------------------------------

void
adjust_vertex_list(block *block_ptr)
{
	int index;
	vertex *vertex_ptr;
	double smallest_x, smallest_y, smallest_z;
	double largest_x, largest_y, largest_z;
	double x_scale, y_scale, z_scale;

	// Find the smallest and largest X, Y and Z coordinate of the "block" part.

	smallest_x = largest_x = block_vertex_x[0];
	smallest_y = largest_y = block_vertex_y[0];
	smallest_z = largest_z = block_vertex_z[0];
	for (index = 0; index < block_vertex_index; index++) {
		if (block_vertex_x[index] < smallest_x)
			smallest_x = block_vertex_x[index];
		else if (block_vertex_x[index] > largest_x)
			largest_x = block_vertex_x[index];
		if (block_vertex_y[index] < smallest_y)
			smallest_y = block_vertex_y[index];
		else if (block_vertex_y[index] > largest_y)
			largest_y = block_vertex_y[index];
		if (block_vertex_z[index] < smallest_z)
			smallest_z = block_vertex_z[index];
		else if (block_vertex_z[index] > largest_z)
			largest_z = block_vertex_z[index];
	}

	// Compute the X, Y and Z scale.

	x_scale = 256.0 / (largest_x - smallest_x);
	y_scale = 256.0 / (largest_y - smallest_y);
	z_scale = 256.0 / (largest_z - smallest_z);

	// Now adjust all vertex coordinates to be in the range of 0..255.

	vertex_ptr = block_ptr->vertex_list;
	while (vertex_ptr) {
		vertex_ptr->x = (vertex_ptr->x - smallest_x) * x_scale;
		vertex_ptr->y = (vertex_ptr->y - smallest_y) * y_scale;
		vertex_ptr->z = (vertex_ptr->z - smallest_z) * z_scale;
		vertex_ptr = vertex_ptr->next_vertex_ptr;
	}
}

//------------------------------------------------------------------------------
// Output the vertex list, part list, polygon list and BSP tree in block file 
// format.
//------------------------------------------------------------------------------

void
create_block_file(block *block_ptr)
{
	vertex *vertex_ptr;
	part *part_ptr;
	int polygon_no;

	// Output the vertex list.

	write_file("<BLOCK NAME=\"%s\">\n", block_name);
	write_file("\t<VERTICES>\n");
	vertex_ptr = block_ptr->vertex_list;
	while (vertex_ptr) {
		write_file("\t\t<VERTEX REF=\"%5d\" "
			"COORDS=\"(%10.5f,%10.5f,%10.5f)\"/>\n", vertex_ptr->vertex_no,
			vertex_ptr->x, vertex_ptr->y, vertex_ptr->z);
		vertex_ptr = vertex_ptr->next_vertex_ptr;
	}
	write_file("\t</VERTICES>\n");

	// Output the part list.  We use the part name as the name of
	// the GIF file, since the ASC file does not output texture file names.

	write_file("\t<PARTS>\n");
	part_ptr = block_ptr->part_list;
	polygon_no = 0;
	while (part_ptr) {

		// Output the start tag for the part list.

		write_file("\t\t<PART NAME=\"%s\" TEXTURE=\"???\">\n", 
			part_ptr->part_name);

		// Output all polygons that belong to this part.

		polygon *polygon_ptr = block_ptr->polygon_list;
		while (polygon_ptr) {
			if (polygon_ptr->part_no == part_ptr->part_no) {

				// Output the vertex list.

				vertex *vertex_ptr = block_ptr->get_vertex(polygon_ptr, 0);
				write_file("\t\t\t<POLYGON REF=\"%2d\" VERTICES=\"%2d",
					++polygon_no, vertex_ptr->vertex_no);
				for (int index = 1; index < polygon_ptr->vertices; index++) {
					vertex_ptr = block_ptr->get_vertex(polygon_ptr, index);
					write_file(",%2d", vertex_ptr->vertex_no);
				}
				write_file("\"");
				write_file("/>\n");
			}
			polygon_ptr = polygon_ptr->next_polygon_ptr;
		}

		// Output the end tag for the part list.

		write_file("\t\t</PART>\n");
		part_ptr = part_ptr->next_part_ptr;
	}
	write_file("\t</PARTS>\n");
	write_file("</BLOCK>\n");
}

//------------------------------------------------------------------------------
// Main program.
//------------------------------------------------------------------------------

int
main(int argc, char **argv)
{
    struct _finddata_t file_info;
	char DXF_file[_MAX_PATH], block_file[_MAX_PATH];
	char *ext_ptr;
	block *block_ptr;
    long find_handle;

	// Make sure the right number of arguments are given.

	if (argc != 2) {
		printf("Usage: DXF2block file.dxf");
		return(false);
	}

	// Find the first matching file.

	if ((find_handle = _findfirst(argv[1], &file_info)) == -1L) {
		printf("No matching files found\n");
		return(false);
	}

	// Process each file...

	do {

		// Make sure this is a DXF file.  If not, skip over it.

		ext_ptr = strrchr(file_info.name, '.');
		if (ext_ptr == NULL || stricmp(ext_ptr, ".dxf")) {
			printf("%s is not a DXF file...skipping\n", argv[1]);
			continue;
		}

		// Create the DXF and block file names.

		strcpy(DXF_file, file_info.name);
		strcpy(block_file, file_info.name);
		ext_ptr = strrchr(block_file, '.');
		*ext_ptr = '\0';
		strcpy(block_name, block_file);
		strcat(block_file, ".block");

		// Initialise the parser.

		init_parser();

		// Attempt to open the DXF file for reading.

		if (!open_file(DXF_file, "r")) {
			file_error("unable to open DXF file '%s' for reading", DXF_file);
			return(false);
		}

		// Create the block object.

		if ((block_ptr = new block) == NULL) {
			memory_error("block object");
			return(false);
		}

		// Parse the DXF file, filling in the block object.

		if (!parse_DXF_file(block_ptr)) {
			close_file();
			delete block_ptr;
			return(false);
		}
		close_file();

		// Adjust all vertex coordinates so that they are in the range 0..256.

		adjust_vertex_list(block_ptr);

		// Attempt to open the block file for writing.

		if (!open_file(block_file, "w")) {
			file_error("unable to open block file '%s' for writing", block_file);
			return(false);
		}

		// Create the block file from the block object.

		printf("Creating block file '%s'\n", block_file);
		create_block_file(block_ptr);
		close_file();

		// Destroy the block object.

		delete block_ptr;

	// Move onto the next file, if there is one.

	} while (_findnext(find_handle, &file_info) == 0);
	_findclose(find_handle);
	return(true);
}
