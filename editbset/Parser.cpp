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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <zip.h>
#include <unzip.h>
#include <time.h>
#include "classes.h"
#include "parser.h"
#include "main.h"

// ZIP archive handle.

static unzFile unzip_handle;
static zipFile zip_handle;
static bool unzipping;

// File stack.

#define FILE_STACK_SIZE	4

struct file_stack_element {
	char file_path[_MAX_PATH];
	long file_size;
	long file_position;
	char *file_buffer;
	char *file_buffer_ptr;
	char line_buf[BUFSIZ];
	char *line_buf_ptr;
	int line_no;
};

static file_stack_element file_stack[4];
static file_stack_element *top_file_ptr;
static int next_file_index;

// String buffer for error and warning messages.

static char error_str[BUFSIZ];
static char warning_str[BUFSIZ];

// Information about last start or single tag parsed.

bool start_tag;
token tag_name;
int params_parsed;
bool matched_param[MAX_PARAMS];

// Last file token parsed, as both a string and a numerical token.

char file_token_str[BUFSIZ];
token file_token;

// Token string to value conversion table.

struct tokendef {
	char *token_name;
	token token_val;
};

tokendef token_table[] = {
	{"translucency",	TOKEN_TRANSLUCENCY},

	{"orientation",	TOKEN_ORIENT},
	{"placeholder",	TOKEN_PLACEHOLDER},
	{"point_light",	TOKEN_POINT_LIGHT},

	{"brightness",	TOKEN_BRIGHTNESS},
	{"spot_light",	TOKEN_SPOT_LIGHT},

	{"direction",	TOKEN_DIRECTION},
	{"texcoords",	TOKEN_TEXCOORDS},

	{"BSP_tree",	TOKEN_BSP_TREE},
	{"entrance",	TOKEN_ENTRANCE},
	{"playback",	TOKEN_PLAYBACK},
	{"polygons",	TOKEN_POLYGONS},
	{"position",	TOKEN_POSITION},
	{"synopsis",	TOKEN_SYNOPSIS},
	{"vertices",	TOKEN_VERTICES},

	{"polygon",	TOKEN_POLYGON},
	{"rolloff",	TOKEN_ROLLOFF},
	{"texture",	TOKEN_TEXTURE},
	{"trigger",	TOKEN_TRIGGER},
	{"version", TOKEN_VERSION},

	{"coords",	TOKEN_COORDS},
	{"colour",	TOKEN_COLOUR},
	{"double",	TOKEN_DOUBLE},
	{"ground",	TOKEN_GROUND},
	{"orient",	TOKEN_ORIENT},
	{"radius",	TOKEN_RADIUS},
	{"sprite",	TOKEN_SPRITE},
	{"symbol",	TOKEN_SYMBOL},
	{"target",	TOKEN_TARGET},
	{"vertex",	TOKEN_VERTEX},
	{"volume",	TOKEN_VOLUME},

	{"align",	TOKEN_ALIGN},
	{"angle",	TOKEN_ANGLE},
	{"block",	TOKEN_BLOCK},
	{"color",	TOKEN_COLOUR},
	{"delay",	TOKEN_DELAY},
	{"faces",	TOKEN_FACES},
	{"flood",	TOKEN_FLOOD},
	{"front",	TOKEN_FRONT},
	{"light",	TOKEN_POINT_LIGHT},
	{"param",	TOKEN_PARAM},
	{"parts",	TOKEN_PARTS},
	{"solid",	TOKEN_SOLID},
	{"sound",	TOKEN_SOUND},
	{"speed",	TOKEN_SPEED},
	{"style",	TOKEN_STYLE},

	{"<!--",	TOKEN_OPEN_COMMENT},
	{"cone",	TOKEN_CONE},
	{"exit",	TOKEN_EXIT},
	{"file",	TOKEN_FILE},
	{"href",	TOKEN_HREF},
	{"name",	TOKEN_NAME},
	{"part",	TOKEN_PART},
	{"rear",	TOKEN_REAR},
	{"root",	TOKEN_ROOT},
	{"text",	TOKEN_TEXT},
	{"type",	TOKEN__TYPE},

	{"-->",	TOKEN_CLOSE_COMMENT},
	{"orb",	TOKEN_ORB},
	{"ref", TOKEN_REF},
	{"sky", TOKEN_SKY},

	{"</",	TOKEN_OPEN_END_TAG},
	{"/>",	TOKEN_CLOSE_SINGLE_TAG},

	{"<",	TOKEN_OPEN_TAG},
	{">",	TOKEN_CLOSE_TAG},
	{"=",	TOKEN_EQUAL},
	{NULL,	TOKEN_NONE}
};

// Pointer to current part of a parameter value string being parsed.

static char *value_str_ptr;

//==============================================================================
// Parser intialisation function.
//==============================================================================

void
init_parser(void)
{
	// Initialise the file stack.

	top_file_ptr = NULL;
	next_file_index = 0;
}

//==============================================================================
// File functions.
//==============================================================================

//------------------------------------------------------------------------------
// Unzip a blockset file into the blockset directory.
//------------------------------------------------------------------------------

bool
open_ZIP_archive(char *file_path)
{
	if ((unzip_handle = unzOpen(file_path)) == NULL)
		return(false);
	unzipping = true;
	return(true);
}

//------------------------------------------------------------------------------
// Zip the blockset directory into a blockset file.
//------------------------------------------------------------------------------

bool
create_ZIP_archive(char *file_path)
{
	if ((zip_handle = zipOpen(file_path, 0)) == NULL)
		return(false);
	unzipping = false;
	return(true);
}

//------------------------------------------------------------------------------
// Close the currently open ZIP archive.
//------------------------------------------------------------------------------

void
close_ZIP_archive(void)
{
	if (unzipping)
		unzClose(unzip_handle);
	else
		zipClose(zip_handle, NULL);
}

//------------------------------------------------------------------------------
// Open a new file and push it onto the parser's file stack.  This file becomes
// the one that is being parsed.
//------------------------------------------------------------------------------

bool
push_file(char *file_path)
{
	FILE *fp;

	// If the file stack is full, do nothing.

	if (next_file_index == FILE_STACK_SIZE)
		return(false);

	// Add the file to the top of the stack.

	top_file_ptr = &file_stack[next_file_index];
	next_file_index++;
	strcpy(top_file_ptr->file_path, file_path);
	top_file_ptr->file_buffer = NULL;
	top_file_ptr->line_no = 0;

	// Open the file for reading in binary mode.

	if ((fp = fopen(file_path, "rb")) == NULL) {
		pop_file();
		return(false);
	}

	// Seek to the end of the file to determine it's size, then seek back to
	// the beginning of the file.

	fseek(fp, 0, SEEK_END);
	top_file_ptr->file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	// Allocate the file buffer, and read the contents of the file into it.

	if ((top_file_ptr->file_buffer = new char[top_file_ptr->file_size]) 
		== NULL ||
		fread(top_file_ptr->file_buffer, top_file_ptr->file_size, 1, fp) == 0) {
		fclose(fp);
		pop_file();
		return(false);
	}

	// Close the file.

	fclose(fp);

	// Initialise the file position and file buffer pointer.

	top_file_ptr->file_position = 0;
	top_file_ptr->file_buffer_ptr = top_file_ptr->file_buffer;
	return(true);
}

//------------------------------------------------------------------------------
// Open a file in the currently open ZIP archive, and push it onto the
// parser's file stack.  This file becomes the one that is being parsed.
//------------------------------------------------------------------------------

bool
push_ZIP_file(char *file_path)
{
	unz_file_info info;

	// If the file stack is full, do nothing.

	if (next_file_index == FILE_STACK_SIZE)
		return(false);

	// Add the file to the top of the stack.

	top_file_ptr = &file_stack[next_file_index];
	next_file_index++;
	strcpy(top_file_ptr->file_path, file_path);
	top_file_ptr->file_buffer = NULL;
	top_file_ptr->line_no = 0;

	// Attempt to locate the file in the ZIP archive.

	if (unzLocateFile(unzip_handle, file_path, 0) != UNZ_OK) {
		pop_file();
		return(false);
	}

	// Get the uncompressed size of the file, and allocate the file buffer.

	unzGetCurrentFileInfo(unzip_handle, &info, NULL, 0, NULL, 0, NULL, 0);
	top_file_ptr->file_size = info.uncompressed_size;
	if ((top_file_ptr->file_buffer = new char[top_file_ptr->file_size])
		== NULL) {
		pop_file();
		return(false);
	}

	// Open the file, read it into the file buffer, then close the file.

	unzOpenCurrentFile(unzip_handle);
	unzReadCurrentFile(unzip_handle, top_file_ptr->file_buffer,
		top_file_ptr->file_size);
	unzCloseCurrentFile(unzip_handle);

	// Initialise the file position and file buffer pointer.

	top_file_ptr->file_position = 0;
	top_file_ptr->file_buffer_ptr = top_file_ptr->file_buffer;
	return(true);
}

//------------------------------------------------------------------------------
// Open a new file in the currently open ZIP archive, and write the top file
// to it.
//------------------------------------------------------------------------------

bool
write_ZIP_file(char *file_path)
{
	time_t time_secs;
	tm *local_time_ptr;
	zip_fileinfo file_info;

	// If there is no file open, do nothing.

	if (top_file_ptr == NULL)
		return(false);

	// Open the new file in the ZIP archive.

	time(&time_secs);
	local_time_ptr = localtime(&time_secs);
	file_info.tmz_date.tm_sec = local_time_ptr->tm_sec;
	file_info.tmz_date.tm_min = local_time_ptr->tm_min;
	file_info.tmz_date.tm_hour = local_time_ptr->tm_hour;
	file_info.tmz_date.tm_mday = local_time_ptr->tm_mday;
	file_info.tmz_date.tm_mon = local_time_ptr->tm_mon;
	file_info.tmz_date.tm_year = local_time_ptr->tm_year;
	file_info.dosDate = 0;
	file_info.internal_fa = 0;
	file_info.external_fa = 0;
	zipOpenNewFileInZip(zip_handle, file_path, &file_info, NULL, 0, NULL, 0,
		NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION);

	// Write the top file buffer to the new file.

	zipWriteInFileInZip(zip_handle, top_file_ptr->file_buffer,
		top_file_ptr->file_size);
	zipCloseFileInZip(zip_handle);
	return(true);
}

//------------------------------------------------------------------------------
// Close the file on the top of the stack, and pop it off.
//------------------------------------------------------------------------------

void
pop_file(void)
{
	// If there is no file on the stack, do nothing.

	if (top_file_ptr == NULL)
		return;

	// Delete the file buffer if it exists.

	if (top_file_ptr->file_buffer)
		delete []top_file_ptr->file_buffer;

	// Pop the top file off the stack.  If there is a file below it, make it
	// the new top file.

	next_file_index--;
	if (next_file_index > 0)
		top_file_ptr = &file_stack[next_file_index - 1];
	else
		top_file_ptr = NULL;
}

//------------------------------------------------------------------------------
// Pop all files from the file stack.
//------------------------------------------------------------------------------

void
pop_all_files(void)
{
	while (top_file_ptr)
		pop_file();
}

//------------------------------------------------------------------------------
// Read the next line from the top file.  If the end of the file has been 
// reached, generate an error.
//------------------------------------------------------------------------------

void
read_next_line(void)
{
	long line_position, line_length;
	char *line_ptr;
	bool got_carriage_return;

	// If there is no file open, do nothing.

	if (top_file_ptr == NULL)
		return;

	// If the file position is at the end of the file, generate an error.
	// Otherwise the start of the line is at the current file position.

	if (top_file_ptr->file_position == top_file_ptr->file_size)
		file_error("Unexpected end of file");
	line_position = top_file_ptr->file_position;
	line_ptr = top_file_ptr->file_buffer_ptr;

	// Increment the file position and file buffer pointer until the end of
	// the current line is found, indicated by a '\r' character (0x0d) or
	// '\n' character (0x0a).

	while (top_file_ptr->file_position < top_file_ptr->file_size &&
		*top_file_ptr->file_buffer_ptr != '\r' &&
		*top_file_ptr->file_buffer_ptr != '\n') {
		top_file_ptr->file_position++;
		top_file_ptr->file_buffer_ptr++;
	}

	// Increment the current line number.

	top_file_ptr->line_no++;

	// Copy the line into the line buffer, excluding the end of line character.

	line_length = top_file_ptr->file_position - line_position;
	strncpy(top_file_ptr->line_buf, line_ptr, line_length);
	top_file_ptr->line_buf[line_length] = '\0'; 
	top_file_ptr->line_buf_ptr = top_file_ptr->line_buf;

	// Skip over the end of line character.  If it was '\r' and '\n'
	// immediately follows, indicating a PC text file format, then skip over
	// it as well.

	got_carriage_return = false;
	if (top_file_ptr->file_position < top_file_ptr->file_size) {
		if (*top_file_ptr->file_buffer_ptr == '\r')
			got_carriage_return = true;
		top_file_ptr->file_position++;
		top_file_ptr->file_buffer_ptr++;
	}
	if (top_file_ptr->file_position < top_file_ptr->file_size &&
		got_carriage_return && *top_file_ptr->file_buffer_ptr == '\n') {
		top_file_ptr->file_position++;
		top_file_ptr->file_buffer_ptr++;
	}
}

//==============================================================================
// Error checking and reporting functions.
//==============================================================================

//------------------------------------------------------------------------------
// Display a formatted error message in a similiar fashion to printf(), then
// close the open file(s) and throw an exception for the main function to catch.
//------------------------------------------------------------------------------

void
file_error(char *format, ...)
{
	va_list arg_ptr;
	char message[BUFSIZ];

	va_start(arg_ptr, format);
	vsprintf(message, format, arg_ptr);
	va_end(arg_ptr);
	if (top_file_ptr != NULL) {
		if (top_file_ptr->line_no > 0)
			sprintf(error_str, "%s, line %d: %s.", top_file_ptr->file_path,
				top_file_ptr->line_no, message);
		else
			sprintf(error_str, "%s: %s.", top_file_ptr->file_path, message);
	} else
		sprintf(error_str, "%s", message);
	throw (char *)&error_str;
}

//------------------------------------------------------------------------------
// Display a formatted memory error message, then close the open file(s) and
// throw an exception for the main function to catch.
//------------------------------------------------------------------------------

void
memory_error(char *object)
{
	// Create the error message, pop the top file off the file stack, and
	// throw the error message.

	sprintf(error_str, "Insufficient memory for %s.\n", object);
	throw (char *)&error_str;
}

//------------------------------------------------------------------------------
// Verify that the given integer is within the specified range; if not,
// generate a fatal error.
//------------------------------------------------------------------------------

void
check_int_range(char *message, int value, int min, int max)
{
	if (value < min || value > max) 
		file_error("%s must be between %d and %d (not %d)", message, min, max,
			value);
}

//------------------------------------------------------------------------------
// Verify that the given double is within the specified range; if not,
// generate a fatal error.
//------------------------------------------------------------------------------

void
check_double_range(char *message, double value, double min, double max)
{
	if (value < min || value > max)
		file_error("%s must be between %g and %g", message, min, max);
}

//------------------------------------------------------------------------------
// Display an error message for a bad token.
//------------------------------------------------------------------------------

static void
expected_token(char *expected_message)
{
	if (strchr(expected_message, ' '))
		file_error("Expected %s rather than%s'%s'", expected_message,
			file_token == VALUE_STRING ? " the string " : " ", file_token_str);
	else
		file_error("Expected '%s' rather than %s'%s'", expected_message,
			file_token == VALUE_STRING ? " the string " : " ", file_token_str);
}

//==============================================================================
// Basic parsing functions.
//==============================================================================

//------------------------------------------------------------------------------
// Return the string associated with a given token.
//------------------------------------------------------------------------------

static char *
get_token_str(token token_val)
{
	int index;

	index = 0;
	while (token_table[index].token_name != NULL) {
		if (token_val == token_table[index].token_val)
			return(token_table[index].token_name);
		index++;
	}
	return("unknown");
}

//------------------------------------------------------------------------------
// Extract a token from the start of the current line buffer.
//------------------------------------------------------------------------------

static void
extract_token(void)
{
	char *line_buf_ptr, *token_ptr, *end_token_ptr;
	int string_len, index;

	// Skip over leading white space.

	line_buf_ptr = top_file_ptr->line_buf_ptr;
	while (*line_buf_ptr == ' ' || *line_buf_ptr == '\t' || 
		   *line_buf_ptr == '\r' || *line_buf_ptr == '\n')
		line_buf_ptr++;

	// If we've come to the end of the line buffer, return no token.

	if (*line_buf_ptr == '\0') {
		file_token = TOKEN_NONE;
		*file_token_str = '\0';
		top_file_ptr->line_buf_ptr = line_buf_ptr;
		return;
	}

	// If the next character is a quotation mark, all characters up to a
	// matching quotation mark will be returned as a string value.  If the
	// end of the line buffer is reached before a matching quotation mark is
	// found, this is an error.

	if (*line_buf_ptr == '"' || *line_buf_ptr == '\'') {
		char quote_ch;

		quote_ch = *line_buf_ptr++;
		token_ptr = file_token_str;
		while (*line_buf_ptr != '\0' && *line_buf_ptr != '\r' &&
			   *line_buf_ptr != '\n' && *line_buf_ptr != quote_ch)
			*token_ptr++ = *line_buf_ptr++;
		if (*line_buf_ptr != quote_ch)
			file_error("string is missing closing quotation mark");
		*token_ptr = '\0';
		line_buf_ptr++;
		file_token = VALUE_STRING;
		top_file_ptr->line_buf_ptr = line_buf_ptr;
		return;
	}

	// Attempt to match a pre-defined token against the start of the string,
	// and if successful return it.

	index = 0;
	while (token_table[index].token_name != NULL) {
		token_ptr = token_table[index].token_name;
		if (!strnicmp(line_buf_ptr, token_ptr, strlen(token_ptr))) {
			file_token = token_table[index].token_val;
			strcpy(file_token_str, token_ptr);
			top_file_ptr->line_buf_ptr = line_buf_ptr + strlen(token_ptr);
			return;
		}
		index++;
	}

	// Return the start of the string as an unknown token; the end of the token
	// is either the first white space or pre-defined token found, or the end
	// of the string.

	end_token_ptr = line_buf_ptr;
	while (*end_token_ptr != '\0' && *end_token_ptr != ' ' &&
		   *end_token_ptr != '\t' && *end_token_ptr != '\r' &&
		   *end_token_ptr != '\n') {
		bool token_found;

		index = 0;
		token_found = false;
		while (token_table[index].token_name != NULL) {
			token_ptr = token_table[index].token_name;
			if (!strnicmp(end_token_ptr, token_ptr, strlen(token_ptr))) {
				token_found = true;
				break;
			}
			index++;
		}
		if (token_found)
			break;
		end_token_ptr++;
	}
	file_token = TOKEN_UNKNOWN;
	string_len = end_token_ptr - line_buf_ptr;
	strncpy(file_token_str, line_buf_ptr, string_len);
	file_token_str[string_len] = '\0';
	top_file_ptr->line_buf_ptr = end_token_ptr;
}

//------------------------------------------------------------------------------
// Read one token from the currently open file.
//------------------------------------------------------------------------------

static void
read_next_file_token(void)
{
	// If no file is open, generate an error.
	
	if (top_file_ptr == NULL)
		file_error("Internal error--attempt to read a file when no file is "
			"open");

	// If no tokens have been read from this file, read the first line.

	if (top_file_ptr->line_no == 0)
		read_next_line();

	// Repeat the following loop until we have the next available token.

	while (true) {

		// Extract the next token from the current line, after skipping over 
		// blank lines.

		extract_token();
		while (file_token == TOKEN_NONE) {
			read_next_line();
			extract_token();
		}

		// If the token is the start of a comment, skip over all tokens until
		// we find the end of the comment.

		if (file_token == TOKEN_OPEN_COMMENT) {
			extract_token();
			while (file_token != TOKEN_CLOSE_COMMENT) {
				if (file_token == TOKEN_NONE)
					read_next_line();
				extract_token();
			}
		}

		// Otherwise break out of this loop.

		else
			break;
	}
}

//------------------------------------------------------------------------------
// Read the next file token and return the token value.
//------------------------------------------------------------------------------

static token
next_file_token(void)
{
	read_next_file_token();
	return(file_token);
}

//------------------------------------------------------------------------------
// Return a boolean indicating if the next file token matches the requested
// token.
//------------------------------------------------------------------------------

static bool
next_file_token_is(token requested_token)
{
	return(next_file_token() == requested_token);
}

//------------------------------------------------------------------------------
// If the next file token does not match the requested token, generate an
// error.
//------------------------------------------------------------------------------

static void
match_next_file_token(token requested_token)
{
	read_next_file_token();
	if (file_token != requested_token) {
		file_error("Expected '%s' rather than '%s'", 
			get_token_str(requested_token), file_token_str);
	}
}

//------------------------------------------------------------------------------
// Read the next line and parse it as an integer.
//------------------------------------------------------------------------------

void
read_int(int *int_ptr)
{
	read_next_line();
	if (sscanf(top_file_ptr->line_buf, "%d", int_ptr) != 1)
		file_error("'%s' is not a valid integer", top_file_ptr->line_buf);
}

//------------------------------------------------------------------------------
// Read an integer, and verify that it matches the given value.
//------------------------------------------------------------------------------

void
verify_int(int value)
{
	int parsed_int;

	read_int(&parsed_int);
	if (parsed_int != value)
		file_error("Expected %d rather than %d", value, parsed_int);
}

//------------------------------------------------------------------------------
// Read the next line and parse it as a double.
//------------------------------------------------------------------------------

void
read_double(double *double_ptr)
{
	read_next_line();
	if (sscanf(top_file_ptr->line_buf, "%lf", double_ptr) != 1)
		file_error("'%s' is not a valid floating point number", 
			top_file_ptr->line_buf);
}

//------------------------------------------------------------------------------
// Read the next line and parse it as a name.
//------------------------------------------------------------------------------

void
read_name(char *name)
{
	char *end_ptr;

	read_next_line();
	end_ptr = strchr(top_file_ptr->line_buf, '_');
	if (end_ptr) {
		strncpy(name, top_file_ptr->line_buf, end_ptr - top_file_ptr->line_buf);
		name[end_ptr - top_file_ptr->line_buf] = '\0';
	} else
		strcpy(name, top_file_ptr->line_buf);
}

//------------------------------------------------------------------------------
// Determine if the current line matches the expected line.
//------------------------------------------------------------------------------

bool
line_is(char *expected_line)
{
	if (!stricmp(top_file_ptr->line_buf, expected_line))
		return(true);
	return(false);
}

//------------------------------------------------------------------------------
// Verify the current line matches the expected line, and display an error if
// it doesn't.
//------------------------------------------------------------------------------

void
verify_line(char *expected_line)
{
	if (!line_is(expected_line))
		file_error("Expected '%s' rather than '%s'", expected_line,
			top_file_ptr->line_buf);
}

//------------------------------------------------------------------------------
// Parse a name, or an expression containing a single wildcard character or one
// or more names seperated by commas.  Only alphanumeric tokens seperated by
// spaces are allowed in a name.  Leading and trailing spaces, as well as 
// additional spaces between tokens will be removed.
//------------------------------------------------------------------------------

bool
parse_name(char *old_name, char *new_name, int is_expression)
{
	char *old_name_ptr, *new_name_ptr;

	// Initialise the name pointers.

	old_name_ptr = old_name;
	new_name_ptr = new_name;

	// Parse one part name at a time...

	do {
		// Skip over the leading spaces.  If we reach the end of the string,
		// this is an error.

		while (*old_name_ptr == ' ')
			old_name_ptr++;
		if (!*old_name_ptr)
			return(false);

		// If the first character is a wildcard and we're parsing a name
		// expression, copy it to the return string, skip over trailing spaces,
		// and return.

		if (*old_name_ptr == '*') {
			if (!is_expression)
				file_error("wildcard character not permitted");
			if (new_name_ptr != new_name)
				return(false);
			*new_name_ptr++ = *old_name_ptr++;
			while (*old_name_ptr == ' ')
				old_name_ptr++;
			if (*old_name_ptr)
				return(false);
			*new_name_ptr = '\0';
			return(true);
		}

		// Alternately step through sequences of alphanumeric characters and
		// spaces, copying them to the return string.  Only the first space of
		// a sequence is copied.

		do {
			while (isalnum(*old_name_ptr))
				*new_name_ptr++ = *old_name_ptr++;
			if (!*old_name_ptr || *old_name_ptr == ',')
				break;
			if (*old_name_ptr != ' ')
				return(false);
			*new_name_ptr++ = *old_name_ptr++;
			while (*old_name_ptr == ' ')
				old_name_ptr++;
			if (!*old_name_ptr || *old_name_ptr == ',')
				break;
			if (!isalnum(*old_name_ptr))
				return(false);
		} while (true);

		// Remove the trailing space, if there is one, and add the last
		// character parsed (either a string terminator or comma).

		if (*(new_name_ptr - 1) == ' ')
			new_name_ptr--;
		*new_name_ptr = *old_name_ptr;

		// If the last character was a string terminator, we're done so return.
		// Otherwise we must parse another part name if we're parsing an
		// expression.

		if (!*old_name_ptr) 
			return(true);
		if (!is_expression)
			return(false);
		old_name_ptr++;
		new_name_ptr++;
	} while (true);
}

//------------------------------------------------------------------------------
// Strip the carriage return from a string, if it exists.
//------------------------------------------------------------------------------

void
strip_cr(char *string_to_strip)
{
	int string_length;

	string_length = strlen(string_to_strip);
	if (string_length > 0 && string_to_strip[string_length - 1] == '\n')
		string_to_strip[string_length - 1] = '\0';
}

//==============================================================================
// General parameter value parsing functions.
//==============================================================================

//------------------------------------------------------------------------------
// Begin the parsing of the current file token as a parameter value string.
//------------------------------------------------------------------------------

static void
start_parsing_value(void)
{
	 value_str_ptr = file_token_str;
}

//------------------------------------------------------------------------------
// If a token matches in the parameter value string, return TRUE.
//------------------------------------------------------------------------------

static bool
token_in_value_is(char *token_str)
{
	// Skip over leading white space in the parameter value string.

	while (*value_str_ptr == ' ' || *value_str_ptr == '\t')
		value_str_ptr++;

	// If we've reached the end of the parameter value string or the token
	// doesn't match the parameter value string, return false.

	if (*value_str_ptr == '\0' ||
		strnicmp(value_str_ptr, token_str, strlen(token_str)))
		return(false);
	
	// Otherwise skip over matched token string in parameter value string, and
	// return true.

	value_str_ptr += strlen(token_str);
	return(true);
}

//------------------------------------------------------------------------------
// Match a single-character symbol in the parameter value string.
//------------------------------------------------------------------------------

static void
match_symbol_in_value(char symbol)
{
	char symbol_str[2];

	symbol_str[0] = symbol;
	symbol_str[1] = '\0';
	if (!token_in_value_is(symbol_str)) {
		if (*value_str_ptr == '\0')
			file_error("Expected '%c' at end of parameter value '%s'",
				symbol, file_token_str);
		else
			file_error("Expected '%c' rather than '%c' in parameter value '%s'",
				symbol, *value_str_ptr, file_token_str);
	}
}

//------------------------------------------------------------------------------
// Match an integer in the current parameter value string.
//------------------------------------------------------------------------------

static int
parse_integer_in_value(void)
{
	int value;
	char *last_ch_ptr;

	// Skip over leading white space in the parameter value string.

	while (*value_str_ptr == ' ' || *value_str_ptr == '\t')
		value_str_ptr++;

	// Attempt to parse the integer; if no characters were parsed, this means
	// there was no valid integer in the parameter value string.

	value = strtol(value_str_ptr, &last_ch_ptr, 10);
	if (last_ch_ptr == value_str_ptr) {
		int parsed_len = value_str_ptr - file_token_str;

		if (parsed_len > 0)
			file_error("Expected an integer after '%.*s' in parameter value "
				"'%s'", parsed_len, file_token_str, file_token_str);
		else
			file_error("Expected an integer at the start of parameter value "
				"'%s'", file_token_str);
	}

	// Otherwise skip over matched integer in parameter value string, and
	// return the integer.

	value_str_ptr = last_ch_ptr;
	return(value);
}

//------------------------------------------------------------------------------
// Match a double in the current parameter value string.
//------------------------------------------------------------------------------

static double
parse_double_in_value(void)
{
	double value;
	char *last_ch_ptr;

	// Skip over leading white space in the parameter value string.

	while (*value_str_ptr == ' ' || *value_str_ptr == '\t')
		value_str_ptr++;

	// Attempt to parse the double; if no characters were parsed, this means
	// there was no valid double in the parameter value string.

	value = strtod(value_str_ptr, &last_ch_ptr);
	if (last_ch_ptr == value_str_ptr) {
		int parsed_len = value_str_ptr - file_token_str;

		if (parsed_len > 0)
			file_error("Expected a floating point number after '%.*s' in "
				"parameter value '%s'", parsed_len, file_token_str,
				file_token_str);
		else
			file_error("Expected a floating point number at the start of "
				"parameter value '%s'", file_token_str);
	}

	// Otherwise skip over matched double in parameter value string, and return
	// the double.

	value_str_ptr = last_ch_ptr;
	return(value);
}

//------------------------------------------------------------------------------
// End parsing of the current parameter value string.
//------------------------------------------------------------------------------

static void
stop_parsing_value(void)
{
	char *last_ch_ptr;

	// Skip over leading white space in the parameter value string.

	last_ch_ptr = value_str_ptr;
	while (*value_str_ptr == ' ' || *value_str_ptr == '\t')
		value_str_ptr++;

	// If we haven't reached the end of the parameter value string, this is an
	// error.

	if (*value_str_ptr != '\0')
		file_error("Didn't expect '%s' at the end of parameter value '%s'",
			last_ch_ptr, file_token_str);
}

//==============================================================================
// Specialised parameter value parsing functions.
//==============================================================================

//------------------------------------------------------------------------------
// Parse current file token as a block type.
//------------------------------------------------------------------------------

static blocktype
parse_block_type(void)
{
	char new_file_token_str[256];
	blocktype block_type;

	// Parse the file token string as a name.

	if (!parse_name(file_token_str, new_file_token_str, false))
		expected_token("a valid block type");

	// Now verify it's a valid block type.

	if (!stricmp(new_file_token_str, "structural"))
		block_type = STRUCTURAL_BLOCK;
	else if (!stricmp(new_file_token_str, "multifaceted sprite"))
		block_type = MULTIFACETED_SPRITE;
	else if (!stricmp(new_file_token_str, "angled sprite"))
		block_type = ANGLED_SPRITE;
	else if (!stricmp(new_file_token_str, "revolving sprite"))
		block_type = REVOLVING_SPRITE;
	else if (!stricmp(new_file_token_str, "facing sprite"))
		block_type = FACING_SPRITE;
	else if (!stricmp(new_file_token_str, "player sprite"))
		block_type = PLAYER_SPRITE;
	else
		expected_token("block type 'structural', 'multifaceted sprite', "
			"'angled sprite', 'revolving sprite', 'facing sprite' or "
			"'player sprite'");
	return(block_type);
}

//------------------------------------------------------------------------------
// Parse the current file token as a boolean "yes" or "no".
//------------------------------------------------------------------------------

static bool
parse_bool()
{
	bool value;

	start_parsing_value();
	if (token_in_value_is("yes"))
		value = true;
	else if (token_in_value_is("no"))
		value = false;
	else
		expected_token("boolean 'yes' or 'no'");
	stop_parsing_value();
	return(value);
}

//------------------------------------------------------------------------------
// Parse the current file token as a single-character string.
//------------------------------------------------------------------------------

static char
parse_char()
{
	if (strlen(file_token_str) != 1)
		expected_token("a single-character string");
	return(*file_token_str);
}

//------------------------------------------------------------------------------
// Parse current file token as an angle in degrees.
//------------------------------------------------------------------------------

static double
parse_degrees(void)
{
	double angle;

	start_parsing_value();
	angle = parse_double_in_value();
	stop_parsing_value();
	return(angle);
}

//------------------------------------------------------------------------------
// Parse the current file token as a double.
//------------------------------------------------------------------------------

static double
parse_double()
{
	double value;

	start_parsing_value();
	value = parse_double_in_value();
	stop_parsing_value();
	return(value);
}

//------------------------------------------------------------------------------
// Parse the current file token as an integer.
//------------------------------------------------------------------------------

static int
parse_integer()
{
	int value;

	start_parsing_value();
	value = parse_integer_in_value();
	stop_parsing_value();
	return(value);
}

//------------------------------------------------------------------------------
// Parse the current file token as a floating point percentage, then convert it
// into a value between 0.0 and 1.0.
//------------------------------------------------------------------------------

static double
parse_percentage(void)
{
	double percentage;

	start_parsing_value();
	percentage = parse_double_in_value();
	match_symbol_in_value('%');
	stop_parsing_value();
	check_double_range("percentage", percentage, 0.0, 100.0);
	return(percentage / 100.0);
}

//------------------------------------------------------------------------------
// Parse a reference list.
//------------------------------------------------------------------------------

static void
parse_ref_list(ref_list *ref_list_ptr)
{
	start_parsing_value();
	ref_list_ptr->delete_ref_list();
	do {
		int ref_no = parse_integer_in_value();
		if (!ref_list_ptr->add_ref(ref_no))
			memory_error("reference");
	} while (token_in_value_is(","));
	stop_parsing_value();
}

//------------------------------------------------------------------------------
// Parse the current file token as an RGB colour triplet.
//------------------------------------------------------------------------------

static RGBcolour
parse_RGB(void)
{
	RGBcolour colour;

	start_parsing_value();
	match_symbol_in_value('(');
	colour.red = parse_integer_in_value();
	match_symbol_in_value(',');
	colour.green = parse_integer_in_value();
	match_symbol_in_value(',');
	colour.blue = parse_integer_in_value();
	match_symbol_in_value(')');
	check_int_range("red component of colour", colour.red, 0, 255);
	check_int_range("green component of colour", colour.green, 0, 255);
	check_int_range("blue component of colour", colour.blue, 0, 255);
	stop_parsing_value();
	return(colour);
}

//------------------------------------------------------------------------------
// Parse the current file token as a string of a maximum length.
//------------------------------------------------------------------------------

static void
parse_string(char *string_buf, unsigned int buf_size)
{
	if (strlen(file_token_str) < buf_size)
		strcpy(string_buf, file_token_str);
	else {
		strncpy(string_buf, file_token_str, buf_size - 1);
		string_buf[buf_size - 1] = '\0';
	}
}

//------------------------------------------------------------------------------
// Parse a texture coordinates list.
//------------------------------------------------------------------------------

static void
parse_texcoords_list(texcoords_list *texcoords_list_ptr)
{
	double u, v;

	start_parsing_value();
	texcoords_list_ptr->delete_texcoords_list();
	do {
		match_symbol_in_value('(');
		u = parse_double_in_value();
		match_symbol_in_value(',');
		v = parse_double_in_value();
		match_symbol_in_value(')');
		if (!texcoords_list_ptr->add_texcoords(u, v))
			memory_error("texture coordinates");
	} while (token_in_value_is(","));
	stop_parsing_value();
}

//------------------------------------------------------------------------------
// Parse current file token as a texture style.
//------------------------------------------------------------------------------

static texstyle
parse_texture_style(void)
{
	texstyle style;

	start_parsing_value();
	if (token_in_value_is("tiled"))
		style = TILED_TEXTURE;
	else if (token_in_value_is("scaled"))
		style = SCALED_TEXTURE;
	else if (token_in_value_is("stretched"))
		style = STRETCHED_TEXTURE;
	else
		expected_token("texture style 'tiled', 'scaled' or 'stretched'");
	stop_parsing_value();
	return(style);
}

//------------------------------------------------------------------------------
// Parse current file token as vertex coordinates.
//------------------------------------------------------------------------------

static vertex
parse_vertex_coordinates(void)
{
	vertex coords;

	start_parsing_value();
	match_symbol_in_value('(');
	coords.x = parse_double_in_value();
	match_symbol_in_value(',');
	coords.y = parse_double_in_value();
	match_symbol_in_value(',');
	coords.z = parse_double_in_value();
	match_symbol_in_value(')');
	stop_parsing_value();
	return(coords);
}

//==============================================================================
// Parameter list parsing functions.
//==============================================================================

//------------------------------------------------------------------------------
// Prepare for matching a parameter list.
//------------------------------------------------------------------------------

void
init_param_list(void)
{
	// Reset all flags indicating which parameters were matched.

	for (int param_index = 0; param_index < MAX_PARAMS; param_index++)
		matched_param[param_index] = false;
}

//------------------------------------------------------------------------------
// Parse a parameter name token, and return a pointer to the parameter entry
// that matches it.  No match or a duplicate match are flagged as an error.
// The equal sign and parameter value is also parsed (it must be a string
// value).
//-----------------------------------------------------------------------------

param *
parse_next_param(param *param_list)
{
	token param_name;
	char param_name_str[256];
	int param_index;
	
	// Parse the next valid parameter, skipping over invalid parameters.

	while (true) {

		// If the next token is ">" or "/>" then there are no more parameters
		// to parse, so break out of the loop.

		read_next_file_token();
		if (file_token == TOKEN_CLOSE_TAG || 
			file_token == TOKEN_CLOSE_SINGLE_TAG)
			break;

		// If the token is a string token, this is an error. Otherwise store
		// it as the parameter token.

		if (file_token == VALUE_STRING)
			expected_token("a parameter name");
		param_name = file_token;
		strcpy(param_name_str, file_token_str);
	
		// Parse the equal sign and the parameter value, which must be a
		// string value.

		match_next_file_token(TOKEN_EQUAL);
		read_next_file_token();
		if (file_token != VALUE_STRING)
			expected_token("a string as a parameter value");

		// If the parameter token was unknown or there is no parameter list,
		// skip over this parameter.

		if (param_name == TOKEN_UNKNOWN || param_list == NULL)
			continue;

		// Compare the parameter name against the entries in the parameter list;
		// if we find a match that hasn't been matched before, return the index
		// of that parameter entry.  Otherwise ignore this parameter and parse
		// the next.

		param_index = 0;
		while (param_list[param_index].param_name != TOKEN_NONE) {
			if (param_name == param_list[param_index].param_name &&
				!matched_param[param_index]) {
				matched_param[param_index] = true;
				return(&param_list[param_index]);
			}
			param_index++;
		}
	}

	// Return a NULL pointer if the end of the parameter list was encountered.

	return(NULL);
}

//------------------------------------------------------------------------------
// Parse a parameter list for a given tag.
//-----------------------------------------------------------------------------

void
parse_param_list(token tag_name, param *param_list, token end_token)
{
	param *param_ptr;
	int param_index;
	char old_name[256];

	// Initialise the parameter list.

	init_param_list();

	// Parse the parameter list.

	while ((param_ptr = parse_next_param(param_list)) != NULL) {
		switch (param_ptr->value_type) {
		case VALUE_BLOCK_NAME:
			parse_string(old_name, 256);
			if (!parse_name(old_name, (char *)param_ptr->variable_ptr, false))
				file_error("Expected a valid block name rather than '%s'",
					old_name);
			break;
		case VALUE_BLOCK_TYPE:
			*(blocktype *)param_ptr->variable_ptr = parse_block_type();
			break;
		case VALUE_BOOL:
			*(bool *)param_ptr->variable_ptr = parse_bool();
			break;
		case VALUE_CHAR:
			*(char *)param_ptr->variable_ptr = parse_char();
			break;
		case VALUE_DEGREES:
			*(double *)param_ptr->variable_ptr = parse_degrees();
			break;
		case VALUE_DOUBLE:
			*(double *)param_ptr->variable_ptr = parse_double();
			break;
		case VALUE_INTEGER:
			*(int *)param_ptr->variable_ptr = parse_integer();
			break;
		case VALUE_PART_NAME:
			parse_string(old_name, 256);
			if (!parse_name(old_name, (char *)param_ptr->variable_ptr, false))
				file_error("Expected a valid part name rather than '%s'",
					old_name);
			break;
		case VALUE_PERCENTAGE:
			*(double *)param_ptr->variable_ptr = parse_percentage();
			break;
		case VALUE_REF_LIST:
			parse_ref_list((ref_list *)param_ptr->variable_ptr);
			break;
		case VALUE_RGB:
			*(RGBcolour *)param_ptr->variable_ptr = parse_RGB();
			break;
		case VALUE_STRING:
			parse_string((char *)param_ptr->variable_ptr, 256);
			break;
		case VALUE_TEXCOORDS_LIST:
			parse_texcoords_list((texcoords_list *)param_ptr->variable_ptr);
			break;
		case VALUE_TEXTURE_STYLE:
			*(texstyle *)param_ptr->variable_ptr = parse_texture_style();
			break;
		case VALUE_VERTEX_COORDS:
			*(vertex *)param_ptr->variable_ptr = parse_vertex_coordinates();
			break;
		default:
			file_error("internal error--unrecognised parameter value type");
		}
	}

	// If an end_token was specified, then verify that the end token parsed
	// matches it.

	if (end_token != TOKEN_NONE && file_token != end_token)
		expected_token(get_token_str(end_token));

	// If there was a parameter list, verify that all required parameters were
	// parsed.

	if (param_list) {
		param_index = 0;
		while (param_list[param_index].param_name != TOKEN_NONE) {
			if (param_list[param_index].required && 
				!matched_param[param_index])
				file_error("parameter '%s' is missing from tag '%s'", 
					get_token_str(param_list[param_index].param_name), 
					get_token_str(tag_name));
			param_index++;
		}
	}
}

//------------------------------------------------------------------------------
// Parse a start tag and it's parameter list.
//------------------------------------------------------------------------------

void
parse_start_tag(token tag_name, param *param_list)
{
	match_next_file_token(TOKEN_OPEN_TAG);
	match_next_file_token(tag_name);
	parse_param_list(tag_name, param_list, TOKEN_CLOSE_TAG);
}

//------------------------------------------------------------------------------
// Parse an end tag.
//------------------------------------------------------------------------------

void
parse_end_tag(token tag)
{
	match_next_file_token(TOKEN_OPEN_END_TAG);
	match_next_file_token(tag);
	match_next_file_token(TOKEN_CLOSE_TAG);
}

//------------------------------------------------------------------------------
// Parse the next start or single tag, skipping over tags not in the specified
// tag list.  Return a pointer to the matching tag element, or NULL if an end
// tag with the specified tag name was parsed.
//-----------------------------------------------------------------------------

tag *
parse_next_tag(tag *tag_list, token end_tag_name, char *end_tag_name_str)
{
	int tag_index;
	token tag_name;
	char tag_name_str[256];

	// Parse start and single tags until a valid tag is found.

	while (true) {

		// If the next token is "<" or "</" then there is a tag to be parsed;
		// any other token is simply ordinary text and must be skipped.

		switch (next_file_token()) {
	
		// If this is a start or single tag, then parse it.

		case TOKEN_OPEN_TAG:

			// Read the tag name token.  If it is a string token, this is an
			// error.

			tag_name = next_file_token();
			if (tag_name == VALUE_STRING)
				file_error("a tag name");
			strcpy(tag_name_str, file_token_str);

			// If a tag list was specified, attempt to match the tag name
			// against the entries of the tag list. If a match is found, parse
			// the parameter list for that tag and then return TRUE.

			if (tag_list) {
				tag_index = 0;
				while (tag_list[tag_index].tag_name != TOKEN_NONE) {
					if (tag_name == tag_list[tag_index].tag_name) {
						parse_param_list(tag_name, 
							tag_list[tag_index].param_list,
							tag_list[tag_index].end_token);
						return(&tag_list[tag_index]);
					}
					tag_index++;
				}
			}

			// Otherwise treat this tag as having no parameters, and skip over
			// any parameters found in this tag.

			parse_param_list(tag_name, NULL, TOKEN_NONE);

			// If this was a start tag, recursively call this function without
			// a tag list to skip over all tags in the content up to and
			// including the matching end tag.

			if (file_token == TOKEN_CLOSE_TAG)
				parse_next_tag(NULL, tag_name, tag_name_str);
			break;

		// If the token is "</", then verify we have the expected end tag and
		// return FALSE, otherwise flag an error.

		case TOKEN_OPEN_END_TAG:
			read_next_file_token();
			if ((end_tag_name == TOKEN_UNKNOWN &&
				 stricmp(file_token_str, end_tag_name_str)) ||
				(end_tag_name != TOKEN_UNKNOWN &&
				 end_tag_name != file_token))
					file_error("found end tag '%s' with no matching start tag",
						file_token_str);
			match_next_file_token(TOKEN_CLOSE_TAG);
			return(NULL);
		}
	}
	
	// Required to shut up compiler...

	return(NULL);
}