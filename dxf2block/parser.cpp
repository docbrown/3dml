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
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "classes.hpp"
#include "parser.hpp"

// Open file information.

static char name[256];
static FILE *fp;
static int line_no;
char line[BUFSIZ];

//==============================================================================
// Parser intialisation function.
//==============================================================================

void
init_parser(void)
{
	fp = NULL;
}

//==============================================================================
// File functions.
//==============================================================================

//------------------------------------------------------------------------------
// Open a file using the given flags.
//------------------------------------------------------------------------------

bool
open_file(char *file_path, char *flags)
{
	if ((fp = fopen(file_path, flags)) != NULL) {
		strcpy(name, file_path);
		line_no = 0;
		return(true);
	}
	return(false);
}

//------------------------------------------------------------------------------
// Close the open file.
//------------------------------------------------------------------------------

void
close_file(void)
{
	if (fp != NULL) {
		fclose(fp);
		fp = NULL;
	}
}

//------------------------------------------------------------------------------
// Read the next line from the open file.
//------------------------------------------------------------------------------

bool
read_next_line(void)
{
	int len;

	// If there is no file open, generate an error.

	if (fp == NULL) {
		file_error("no file open");
		return(false);
	}

	// Otherwise increment the line number, and read the next line from the
	// file into it's buffer.

	line_no++;
	if (!fgets(line, BUFSIZ, fp))
		return(false);
	len = strlen(line);

	// If there is a carriage return at the end of the line, remove it.

	if (len > 0 && line[len - 1] == '\n')
		line[len - 1] = '\0';
	return(true);
}

//------------------------------------------------------------------------------
// Write to the open file.
//------------------------------------------------------------------------------

void
write_file(char *format, ...)
{
	va_list arg_ptr;
	char message[BUFSIZ];

	// Build the message from the format string and function parameters.

	va_start(arg_ptr, format);
	vsprintf(message, format, arg_ptr);
	va_end(arg_ptr);

	// Write the message to the file.

	fprintf(fp, "%s", message);
}

//==============================================================================
// Error checking and reporting functions.
//==============================================================================

//------------------------------------------------------------------------------
// Display a formatted error message in a similiar fashion to printf(), then
// close the open file.
//------------------------------------------------------------------------------

void
file_error(char *format, ...)
{
	va_list arg_ptr;
	char message[BUFSIZ];

	// Build the message from the format string and function parameters.

	va_start(arg_ptr, format);
	vsprintf(message, format, arg_ptr);
	va_end(arg_ptr);

	// Display the message with file name and line information, if approapiate.

	if (fp != NULL) {
		if (line_no > 0)
			printf("Error reading file %s, line %d: %s\n", name, line_no,
				message);
		else
			printf("Error reading file %s: %s\n", name, message);
	} else
		printf("Error: %s\n", message);

	// Close the file, if open.

	close_file();
}

//------------------------------------------------------------------------------
// Display a formatted memory error message for the given object, then close 
// the open file.
//------------------------------------------------------------------------------

void
memory_error(char *object_name)
{
	printf("Error: could not allocate memory for %s\n", object_name);
	close_file();
}

//------------------------------------------------------------------------------
// Display an error message for a bad line.
//------------------------------------------------------------------------------

static void
bad_line(char *expected_line)
{
	// If the expected token contains spaces, assume it's a general message
	// rather than a token.

	if (strchr(expected_line, ' '))
		file_error("I was expecting %s rather than '%s'", expected_line,
			line);
	else
		file_error("I was expecting '%s' rather than '%s'", expected_line,
			line);
}

//==============================================================================
// Parsing functions.
//==============================================================================

//------------------------------------------------------------------------------
// Verify the current line matches the expected line.
//------------------------------------------------------------------------------

bool
line_is(char *expected_line)
{
	if (!stricmp(line, expected_line))
		return(true);
	return(false);
}

//------------------------------------------------------------------------------
// Verify the current line matches the expected line, and display an error if
// it doesn't.
//------------------------------------------------------------------------------

bool
verify_line(char *expected_line)
{
	if (!line_is(expected_line)) {
		bad_line(expected_line);
		return(false);
	}
	return(true);
}

//------------------------------------------------------------------------------
// Read the next line and parse it as an integer.
//------------------------------------------------------------------------------

bool
read_int(int *int_ptr)
{
	// Read the next line.

	if (!read_next_line())
		return(false);

	// Parse the line as an integer.

	if (sscanf(line, "%d", int_ptr) != 1) {
		file_error("'%s' is not a valid integer", line);
		return(false);
	}
	return(true);
}

//------------------------------------------------------------------------------
// Read the next line and parse it as a double.
//------------------------------------------------------------------------------

bool
read_double(double *double_ptr)
{
	// Extract the next file token.

	if (!read_next_line())
		return(false);

	// Parse the token as a double.

	if (sscanf(line, "%lf", double_ptr) != 1) {
		file_error("'%s' is not a valid floating point number", line);
		return(false);
	}
	return(true);
}
