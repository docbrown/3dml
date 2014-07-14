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
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <direct.h>
#include <time.h>
#include "unzip\unzip.h"
#include "Classes.h"
#include "Fileio.h"
#include "Main.h"
#include "Parser.h"
#include "Platform.h"
#include "Plugin.h"
#include "Utils.h"

// ZIP archive handle.

static unzFile ZIP_archive_handle;

// File stack.

#define FILE_STACK_SIZE	4
static file_stack_element file_stack[4];
static int next_file_index;

// Pointer to file on top of file stack.

file_stack_element *top_file_ptr;

// The current line number in the spot presently open.

int spot_line_no;

// String buffer for error messages.

static char error_str[BUFSIZ];

// Information about last start or single tag parsed.

bool start_tag;
token tag_name;
int params_parsed;
bool matched_param[MAX_PARAMS];

// Current line buffer, and current line buffer pointer.

string line_buffer;
static char *line_buffer_ptr;

// Last file token parsed, as both a string and a numerical token.

static string file_token_str;
static token file_token;

// Token definition class.

struct tokendef {
	char *token_name;
	token token_val;
};

// XML token table.

tokendef xml_token_table[] = {
	{"<!--",	TOKEN_OPEN_COMMENT},
	{"-->",		TOKEN_CLOSE_COMMENT},
	{"</",		TOKEN_OPEN_END_TAG},
	{"/>",		TOKEN_CLOSE_SINGLE_TAG},
	{"<",		TOKEN_OPEN_TAG},
	{">",		TOKEN_CLOSE_TAG},
	{"=",		TOKEN_EQUAL},
	{"\"",		TOKEN_QUOTE},
	{"'",		TOKEN_QUOTE},
	{" ",		TOKEN_WHITESPACE},
	{"\t",		TOKEN_WHITESPACE},
	{NULL,		TOKEN_NONE}
};

// Token table.

tokendef token_table[] = {
	{"ambient_light",	TOKEN_AMBIENT_LIGHT},
	{"ambient_sound",	TOKEN_AMBIENT_SOUND},

	{"translucency",	TOKEN_TRANSLUCENCY},

	{"orientation",		TOKEN_ORIENTATION},
	{"placeholder",		TOKEN_PLACEHOLDER},
	{"point_light",		TOKEN_POINT_LIGHT},

	{"brightness",		TOKEN_BRIGHTNESS},
	{"dimensions",		TOKEN_DIMENSIONS},
	{"projection",		TOKEN_PROJECTION},
//	{"set_stream",		TOKEN_SET_STREAM},
	{"spot_light",		TOKEN_SPOT_LIGHT},
	{"textcolour",		TOKEN_TEXTCOLOUR},

	{"direction",		TOKEN_DIRECTION},
	{"placement",		TOKEN_PLACEMENT},
	{"texcoords",		TOKEN_TEXCOORDS},
	{"textalign",		TOKEN_TEXTALIGN},
	{"textcolor",		TOKEN_TEXTCOLOUR},

	{"blockset",		TOKEN_BLOCKSET},
	{"BSP_tree",		TOKEN_BSP_TREE},
	{"entrance",		TOKEN_ENTRANCE},
	{"imagemap",		TOKEN_IMAGEMAP},
	{"location",		TOKEN_LOCATION},
	{"playback",		TOKEN_PLAYBACK},
	{"polygons",		TOKEN_POLYGONS},
	{"position",		TOKEN_POSITION},
	{"synopsis",		TOKEN_SYNOPSIS},
	{"velocity",		TOKEN_VELOCITY},
	{"vertices",		TOKEN_VERTICES},

	{"command",			TOKEN_COMMAND},
	{"movable",			TOKEN_MOVABLE},
	{"polygon",			TOKEN_POLYGON},
	{"replace",			TOKEN_REPLACE},
	{"rolloff",			TOKEN_ROLLOFF},
	{"texture",			TOKEN_TEXTURE},
	{"trigger",			TOKEN_TRIGGER},
	{"updated",			TOKEN_UPDATED},
	{"version",			TOKEN_VERSION},

	{"action",			TOKEN_ACTION},
	{"camera",			TOKEN_CAMERA},
	{"coords",			TOKEN_COORDS},
	{"colour",			TOKEN_COLOUR},
	{"create",			TOKEN_CREATE},
	{"double",			TOKEN_DOUBLE},
	{"ground",			TOKEN_GROUND},
	{"number",			TOKEN_NUMBER},
	{"onload",			TOKEN_ONLOAD},
	{"orient",			TOKEN_ORIENTATION},
	{"player",			TOKEN_PLAYER},
	{"radius",			TOKEN_RADIUS},
	{"source",			TOKEN__SOURCE},
	{"sprite",			TOKEN_SPRITE},
	{"stream",			TOKEN_STREAM},
	{"symbol",			TOKEN_SYMBOL},
	{"target",			TOKEN_TARGET},
	{"vertex",			TOKEN_VERTEX},
	{"volume",			TOKEN_VOLUME},

	{"align",			TOKEN_ALIGN},
	{"angle",			TOKEN_ANGLE},
	{"block",			TOKEN_BLOCK},
	{"cache",			TOKEN_CACHE},
	{"color",			TOKEN_COLOUR},
	{"debug",			TOKEN_DEBUG},
	{"delay",			TOKEN_DELAY},
	{"faces",			TOKEN_FACES},
	{"flood",			TOKEN_FLOOD},
	{"front",			TOKEN_FRONT},
	{"level",			TOKEN_LEVEL},
	{"light",			TOKEN_POINT_LIGHT},
	{"param",			TOKEN_PARAM},
	{"parts",			TOKEN_PARTS},
	{"popup",			TOKEN_POPUP},
	{"scale",			TOKEN_SCALE},
	{"shape",			TOKEN_SHAPE},
	{"solid",			TOKEN_SOLID},
	{"sound",			TOKEN_SOUND},
	{"speed",			TOKEN_SPEED},
	{"style",			TOKEN_STYLE},
	{"title",			TOKEN_TITLE},

	{"area",			TOKEN_AREA},
	{"body",			TOKEN_BODY},
	{"cone",			TOKEN_CONE},
	{"exit",			TOKEN_EXIT},
	{"file",			TOKEN_FILE},
	{"head",			TOKEN_HEAD},
	{"href",			TOKEN_HREF},
	{"load",			TOKEN_LOAD},
	{"name",			TOKEN_NAME},
	{"part",			TOKEN_PART},
	{"rear",			TOKEN_REAR},
	{"rect",			TOKEN_RECT},
	{"root",			TOKEN_ROOT},
	{"size",			TOKEN_SIZE},
	{"spot",			TOKEN_SPOT},
	{"text",			TOKEN_TEXT},
	{"type",			TOKEN__TYPE},

	{"map",				TOKEN_MAP},
	{"orb",				TOKEN_ORB},
	{"ref",				TOKEN_REF},
	{"sky",				TOKEN_SKY},
	{"wmp",				TOKEN_WMP},

	{"id",				TOKEN_ID},
	{"rp",				TOKEN_RP},

	{NULL,				TOKEN_NONE}
};

// Description of value types.

char *value_type_str[] = {
	"'roll on', 'roll off', 'click on', 'step in', 'step out', 'timer'",
	"'top-left', 'top', 'top-right', 'left', 'center', 'right', "
		"'bottom-left', 'bottom' or 'bottom-right'",
	"a valid block symbol, or map coordinates in the form '(column,row,level)'",
	"'structural', 'multifaceted sprite', 'angled sprite', 'revolving sprite', "
		"'facing sprite' or 'player sprite'",
	"'yes' or 'no'",
	"an angle in degrees",
	"a range of delays, in seconds, in the form 'min..max'",
	"a direction in the form '(turn-angle,tilt-angle)'",
	"a range of directions in the form '(turn1,tilt1)..(turn2,tilt2)'",
	"a valid double character symbol",
	"'click on', 'step on' or both",
	"an angle in degrees, or two angles seperated by a comma",
	"a real number",
	"a whole number",
	"map coordinates in the form '(column,row,level)'",
	"map dimensions in the form '(columns,rows,levels)'",
	"a map scale in pixels",
	"'single' or 'double'",
	"a name containing only letters, digits and underscores",
	"a comma-seperated list of one or more names",
	"an orientation",
	"a range of percentages",
	"a percentage",
	"'top-left', 'top', 'top-right', 'left', 'center', 'right', "
		"'bottom-left', 'bottom', 'bottom-right' or 'mouse'",
	"'looped' or 'random'",
	"'static' or 'pulsate'",
	"one or more of 'proximity', 'rollover', or 'everywhere'",
	"'north', 'south', 'east', 'west', 'top' or 'bottom'",
	"a radius in blocks",
	"a rectangle in the form '(x1,y1,x2,y2)'",
	"Map coordinates in the form '(column,row,level)'; "
		"the coordinates may be relative by appending + or -",
	"an RGB value in the form '(red,green,blue)'",
	"'rect' or 'circle'",
	"a valid single character symbol",
	"a popup size in the form '(width,height)'",
	"'static', 'revolve' or 'search'",
	"a sprite size in the form '(width,height)'",
//	"'play once', 'play looped', 'stop', or 'pause'",
	"a string",
	"a valid single or double character block symbol",
	"'tiled', 'scaled' or 'stretched'",
	"one or more of 'click on', 'step on', 'proximity' or 'rollover'",
	"'top', 'center' or 'bottom'",
	"a valid version number",
	"vertex coordinates",
};

// Pointer to current value string, and the part of the value string being 
// parsed.

static char *value_str;
static char *value_str_ptr;

// Table of unsafe characters in a URL which must be encoded.

#define	UNSAFE_CHARS	13	
static char unsafe_char[UNSAFE_CHARS] = {
	' ', '<', '>', '%', '{', '}', '|', '\\', '^', '~', '[', ']', '`'
};

//==============================================================================
// Parser intialisation function.
//==============================================================================

void
init_parser(void)
{
	// Initialise the ZIP archive handle.

	ZIP_archive_handle = NULL;

	// Initialise the file stack.

	top_file_ptr = NULL;
	next_file_index = 0;
}

//==============================================================================
// URL and conversion functions.
//==============================================================================

//------------------------------------------------------------------------------
// Split a URL into it's components.
//------------------------------------------------------------------------------

void 
split_URL(const char *URL, string *URL_dir, string *file_name,
		  string *entrance_name)
{
	char *name_ptr;
	string new_URL;
	int index;

	// First locate the division between the file path and entrance name, and
	// seperate them.  If there is no entrance name, it is set to "default".
	// If the entrance parameter is NULL, the entrance name is not copied.

	name_ptr = strrchr(URL, '#');
	if (name_ptr != NULL) {
		new_URL = URL;
		new_URL.truncate(name_ptr - URL);
		if (entrance_name)
			*entrance_name = name_ptr + 1;
	} else {
		new_URL = URL;
		if (entrance_name)
			*entrance_name = "default";
	}

	// Next locate the division between the URL directory and the file name,
	// and seperate them.  If there is no URL directory, it is set to an empty
	// string.  If either the URL_dir or the file_name parameter are NULL, the
	// URL directory or file name components are not copied.

	index = strlen(new_URL) - 1;
	name_ptr = new_URL;
	while (index >= 0) {
		if (name_ptr[index] == '/' || name_ptr[index] == '\\')
			break;
		index--;
	}
	if (index >= 0) {
		if (file_name)
			*file_name = name_ptr + index + 1;
		if (URL_dir) {
			*URL_dir = name_ptr;
			URL_dir->truncate(index + 1);
		}
	} else {
		if (file_name)
			*file_name = name_ptr;
		if (URL_dir)
			*URL_dir = (const char *)NULL;
	}
}

//------------------------------------------------------------------------------
// Create a URL from URL directory, subdirectory and file name components.
// The URL parameter must point to a preallocated buffer.  If the file name 
// contains an absolute URL, it overrides the URL directory and subdirectory 
// components.
//------------------------------------------------------------------------------

string
create_URL(const char *URL_dir, const char *sub_dir, const char *file_name)
{
	string URL;

	// If the file name starts with a slash, a drive letter followed by a colon,
	// or the strings "http:", "file:", "javascript:", "mailto:", "telnet:" or
	// "ftp:", then assume it's an absolute URL.

	if (file_name[0] == '/' || file_name[0] == '\\' ||
		(strlen(file_name) > 1 && isalpha(file_name[0]) && file_name[1] == ':')
		|| !strnicmp(file_name, "http:", 5) || !strnicmp(file_name, "file:", 5)
		|| !strnicmp(file_name, "javascript:", 11) ||
		!strnicmp(file_name, "mailto:", 7) || 
		!strnicmp(file_name, "telnet:", 7) || !strnicmp(file_name, "ftp:", 4))
		URL = file_name;

	// Otherwise concatenate the URL path, subdirectory and file name
	// parameters to create the URL.  It is assumed both the URL path and
	// subdirectory parameters have trailing slashes.  If the URL_dir or
	// sub_dir parameters are NULL, they are ignored.
	
	else {
		if (URL_dir)
			URL = URL + URL_dir;
		if (sub_dir)
			URL = URL + sub_dir;
		URL = URL + file_name;
	}
	return(URL);
}

//------------------------------------------------------------------------------
// Convert a character to a hexadecimal digit.
//------------------------------------------------------------------------------

static char
char_to_hex_digit(char ch)
{
	if (ch >= '0' && ch <= '9')
		return(ch - '0');
	else if (ch >= 'A' && ch <= 'F')
		return(ch - 'A' + 10);
	else
		return(ch - 'a' + 10);
}

//------------------------------------------------------------------------------
// Convert a hexadecimal digit to a character.
//------------------------------------------------------------------------------

static char
hex_digit_to_char(char digit)
{
	if (digit >= 0 && digit <= 9)
		return(digit + '0');
	else
		return(digit - 10 + 'A');
}

//------------------------------------------------------------------------------
// Change a URL so that unsafe characters are encoded as "%nn", where nn is the
// ASCII code in hexadecimal.
//------------------------------------------------------------------------------

string
encode_URL(const char *URL)
{
	int unsafe_chars;
	char *new_URL;
	const char *old_char_ptr;
	char *new_char_ptr;
	int index;

	// First step through the URL and count the number of unsafe characters.

	unsafe_chars = 0;
	old_char_ptr = URL;
	while (*old_char_ptr != '\0') {
		for (index = 0; index < UNSAFE_CHARS; index++)
			if (*old_char_ptr == unsafe_char[index]) {
				unsafe_chars++;
				break;
			}
		old_char_ptr++;
	}

	// Allocate a new URL string that is large enough to hold the URL with
	// unsafe characters encoded.

	if ((new_URL = new char[strlen(URL) + (unsafe_chars * 2) + 1]) == NULL)
		return((const char *)NULL);

	// Copy the URL into the new URL string, encoding the unsafe characters
	// along the way.

	old_char_ptr = URL;
	new_char_ptr = new_URL;
	while (*old_char_ptr) {
		for (index = 0; index < UNSAFE_CHARS; index++)
			if (*old_char_ptr == unsafe_char[index]) {
				*new_char_ptr++ = '%';
				*new_char_ptr++ = hex_digit_to_char((*old_char_ptr & 0xf0) >> 4);
				*new_char_ptr++ = hex_digit_to_char(*old_char_ptr & 0x0f);
				break;
			} 
		if (index == UNSAFE_CHARS)
			*new_char_ptr++ = *old_char_ptr;
		old_char_ptr++;
	}
	*new_char_ptr = '\0';
	return(new_URL);
}

//------------------------------------------------------------------------------
// Change a URL so that characters encoded as "%nn" are decoded, where nn is the
// ASCII code in hexadecimal.
//------------------------------------------------------------------------------

string
decode_URL(const char *URL)
{
	string new_URL;
	char *old_char_ptr, *new_char_ptr;

	new_URL = URL;
	old_char_ptr = new_URL;
	new_char_ptr = new_URL;
	while (*old_char_ptr) {
		if (*old_char_ptr == '%') {
			old_char_ptr++;
			char first_digit = char_to_hex_digit(*old_char_ptr++);
			char second_digit = char_to_hex_digit(*old_char_ptr++);
			*new_char_ptr++ = (first_digit << 4) + second_digit;
		} else
			*new_char_ptr++ = *old_char_ptr++;
	}		
	*new_char_ptr = '\0';
	return(new_URL);
}

//------------------------------------------------------------------------------
// Convert a URL to a file path.
//------------------------------------------------------------------------------

string
URL_to_file_path(const char *URL)
{
	string file_path;
	char *char_ptr;

	// If the URL begins with "file:///", "file://localhost/" or "file://",
	// skip over it.

	if (!strnicmp(URL, "file:///", 8))
		file_path = URL + 8;
	else if (!strnicmp(URL, "file://localhost/", 17))
		file_path = URL + 17;
	else if (!strnicmp(URL, "file://", 7))
		file_path = URL + 7;
	else
		file_path = URL;
		
	// Replace forward slashes with back slashes, and vertical bars to colons
	// in the file path.

	char_ptr = file_path;
	while (*char_ptr) {
		if (*char_ptr == '/')
			*char_ptr = '\\';
		else if (*char_ptr == '|')
			*char_ptr = ':';
		char_ptr++;
	}

	// Return the file path.

	return(file_path);
}

//------------------------------------------------------------------------------
// Split an identifier into a style name and object name.
//------------------------------------------------------------------------------

void
parse_identifier(const char *identifier, string &style_name, 
				 string &object_name)
{
	char *colon_ptr;
	int style_name_length;

	// Find the first colon in the identifier.  If not found, the object name is
	// the entire identifier.

	if ((colon_ptr = strchr(identifier, ':')) == NULL) {
		style_name = (const char *)NULL;
		object_name = identifier;
	}

	// Otherwise copy the string before the colon into the style name buffer,
	// and the string after the colon into the object name buffer.

	else {
		style_name_length = colon_ptr - identifier;
		style_name = identifier;
		style_name.truncate(style_name_length);
		object_name = identifier + style_name_length + 1;
	}
}

//------------------------------------------------------------------------------
// Determine if the given character is not a legal symbol.
//------------------------------------------------------------------------------

bool
not_single_symbol(char ch, bool disallow_dot)
{
	return(ch < FIRST_BLOCK_SYMBOL || ch > LAST_BLOCK_SYMBOL ||
		(disallow_dot && ch == '.') || ch == '<');
}

//------------------------------------------------------------------------------
// Determine if the give character combination is a legal symbol.
//------------------------------------------------------------------------------

bool
not_double_symbol(char ch1, char ch2, bool disallow_dot_dot)
{
	return(ch1 < FIRST_BLOCK_SYMBOL || ch1 > LAST_BLOCK_SYMBOL ||
		ch2 < FIRST_BLOCK_SYMBOL || ch2 > LAST_BLOCK_SYMBOL ||
		ch1 == '<' || (disallow_dot_dot && ch1 == '.' && ch2 == '.'));
}

//------------------------------------------------------------------------------
// Parse a string as a single character symbol; it must be a printable ASCII
// character except '<', ' ', or '.' if disallow_dot is TRUE.
//------------------------------------------------------------------------------

bool
string_to_single_symbol(const char *string_ptr, char *symbol_ptr,
						bool disallow_dot)
{
	if (strlen(string_ptr) != 1 || not_single_symbol(*string_ptr, disallow_dot))
		return(false);
	*symbol_ptr = *string_ptr;
	return(true);
}

//------------------------------------------------------------------------------
// Parse a string as a double character symbol; it must be a combination of two
// printable ASCII characters except '<' as the first character or ' ' as either
// character, and cannot be '..' if disallow_dot_dot is TRUE.
//------------------------------------------------------------------------------

bool
string_to_double_symbol(const char *string_ptr, word *symbol_ptr,
						bool disallow_dot_dot)
{
	if (strlen(string_ptr) != 2 || not_double_symbol(*string_ptr, 
		*(string_ptr + 1), disallow_dot_dot))
		return(false);
	if (*string_ptr == '.')
		*symbol_ptr = *(string_ptr + 1);
	else
		*symbol_ptr = (*string_ptr << 7) + *(string_ptr + 1);
	return(true);
}

//------------------------------------------------------------------------------
// Convert a version number to a version string.
//------------------------------------------------------------------------------

char *
version_number_to_string(unsigned int version_number)
{
	int version1, version2, version3, version4;
	static char version_string[16];

	version1 = (version_number >> 24) & 255;
	version2 = (version_number >> 16) & 255;
	version3 = (version_number >> 8) & 255;
	version4 = version_number & 255;
	if (version3 == 0) {
		if (version4 == 255)
			sprintf(version_string, "%d.%d", version1, version2);
		else
			sprintf(version_string, "%d.%db%d", version1, version2, version4);
	} else {
		if (version4 == 255)
			sprintf(version_string, "%d.%d.%d", version1, version2, version3);
		else
			sprintf(version_string, "%d.%d.%db%d", version1, version2, version3,
				version4);
	}
	return(version_string);
}

//==============================================================================
// File functions.
//==============================================================================

//------------------------------------------------------------------------------
// Open a ZIP archive.
//------------------------------------------------------------------------------

bool
open_ZIP_archive(const char *file_path)
{
	// Open the ZIP archive.

	if ((ZIP_archive_handle = unzOpen(file_path)) == NULL)
		return(false);
	return(true);
}

//------------------------------------------------------------------------------
// Download the blockset at the given URL into the cache.
//------------------------------------------------------------------------------

static cached_blockset *
download_blockset(const char *URL, const char *name)
{
	string file_dir;
	string file_name;
	string cache_file_path;
	cached_blockset *cached_blockset_ptr;
	char *folder;
	FILE *fp;
	int size;

	// Split the URL into a file directory and name, omitting the leading
	// "http://".

 	split_URL(URL + 7, &file_dir, &file_name, NULL);

	// Build the cache file path by starting at the Flatland folder and adding
	// each folder of the file directory one by one.  Finally, add the file
	// name to complete the cache file path.

	cache_file_path = flatland_dir;
	folder = strtok(file_dir, "/\\");
	while (folder) {
		cache_file_path = cache_file_path + folder;
		mkdir(cache_file_path);
		cache_file_path = cache_file_path + "\\";
		folder = strtok(NULL, "/\\");
	}
	cache_file_path = cache_file_path + file_name;

	// Download the blockset URL to the cache file path.

	requested_blockset_name = name;
	if (!download_URL(URL, cache_file_path))
		return(NULL);

	// Determine the size of the blockset.

	if ((fp = fopen(cache_file_path, "r")) == NULL)
		return(NULL);
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fclose(fp);

	// Add the cached blockset to the cached blockset list, and save the list
	// to the cache file.  Then return a pointer to the cached blockset entry.

	if ((cached_blockset_ptr = new_cached_blockset(cache_file_path, URL, size, 
		time(NULL))) == NULL)
		return(NULL);
	save_cached_blockset_list();
	return(cached_blockset_ptr);
}

//------------------------------------------------------------------------------
// Open a cached blockset ZIP archive, downloading and caching it first if it
// doesn't already exist. 
//------------------------------------------------------------------------------

bool
open_blockset(const char *blockset_URL, const char *blockset_name)
{
	FILE *fp;
	int size;
	string blockset_path;
	string cached_blockset_URL;
	string cached_blockset_path;
	string version_file_URL;
	char *ext_ptr;
	cached_blockset *cached_blockset_ptr;

	// If the blockset URL begins with "file://", assume it's a local file that
	// can be opened directly.

	if (!strnicmp(blockset_URL, "file://", 7)) {
		blockset_path = URL_to_file_path(blockset_URL);
		if (!open_ZIP_archive(blockset_path))
			return(false);
	}

	// If the blockset path begins with "http://", check to see whether the
	// blockset exists in the cache.  If not, download the blockset and cache
	// it.

	else {

		// Create the file path of the blockset in the cache.

		cached_blockset_URL = create_URL(flatland_dir, NULL, blockset_URL + 7);
		cached_blockset_path = URL_to_file_path(cached_blockset_URL);

		// First look for the blockset in the cached blockset list; if not
		// found, check whether the blockset exists in the cache and add it to
		// the cached blockset list if found; otherwise attempt to download the
		// blockset from the given blockset URL.

		if ((cached_blockset_ptr = find_cached_blockset(blockset_URL)) == NULL) {
			if ((fp = fopen(cached_blockset_path, "r")) == NULL) {
				if ((cached_blockset_ptr = download_blockset(blockset_URL, 
					blockset_name)) == NULL)
					return(false);
			} else {
				fseek(fp, 0, SEEK_END);
				size = ftell(fp);
				fclose(fp);
				if ((cached_blockset_ptr = new_cached_blockset(
					cached_blockset_path, blockset_URL, size, time(NULL))) 
					== NULL)
					return(false);
				save_cached_blockset_list();
			}
		}

		// If the blockset does exist in the cache, the mininum update period has
		// expired, and the current spot is from a web-based URL, check whether
		// a new update is available.

		if (time(NULL) - cached_blockset_ptr->updated > MIN_UPDATE_PERIOD &&
			!strnicmp(curr_spot_URL, "http://", 7)) {

			// Construct the URL to the blockset's version file.

			version_file_URL = blockset_URL;
			ext_ptr = strrchr(version_file_URL, '.');
			version_file_URL.truncate(ext_ptr - (char *)version_file_URL);
			version_file_URL += ".txt";

			// Check if there is a new version of the blockset and the user has
			// agreed to download it.  If so, remove the current version of the
			// blockset and download the new one.

			if (check_for_blockset_update(version_file_URL, blockset_name,
				cached_blockset_ptr->version)) {
				delete_cached_blockset(blockset_URL);
				save_cached_blockset_list();
				remove(cached_blockset_path);
				if (!download_blockset(blockset_URL, blockset_name))
					return(false);
			}

			// If there is no new version or the user has not agreed to download
			// it, set the time of this update check so it won't happen again
			// for another update period.

			else {
				cached_blockset_ptr->updated = time(NULL);
				save_cached_blockset_list();
			}
		}

		// Attempt to open the cached blockset.

		if (!open_ZIP_archive(cached_blockset_path))
			return(false);

		// If the blockset does not come from Flatland's site and is larger
		// than 300KB in size, reject it.

		if (strnicmp(blockset_URL, "http://blocksets.flatland.com/", 30)
			&& cached_blockset_ptr->size > 307200) {
			close_ZIP_archive();
			warning("Blockset from URL %s exceeds the maximum size "
				"limit of 300 KB", blockset_URL);
			return(false);
		}
	}

	// Indicate success.

	return(true);
}

//------------------------------------------------------------------------------
// Close the currently open ZIP archive.
//------------------------------------------------------------------------------

void
close_ZIP_archive(void)
{
	unzClose(ZIP_archive_handle);
	ZIP_archive_handle = NULL;
}

//------------------------------------------------------------------------------
// Open a file and push it onto the parser's file stack.  This file becomes
// the one that is being parsed.
//------------------------------------------------------------------------------

bool
push_file(const char *file_path, bool spot_file)
{
	FILE *fp;
	char *file_buffer_ptr;
	int file_size;

	// Open the file for reading in binary mode.

	if ((fp = fopen(file_path, "rb")) == NULL)
		return(false);

	// Seek to the end of the file to determine it's size, then seek back to
	// the beginning of the file.

	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	// Allocate the file buffer, and read the contents of the file into it.

	if ((file_buffer_ptr = new char[file_size]) == NULL ||
		fread(file_buffer_ptr, file_size, 1, fp) == 0) {
		fclose(fp);
		return(false);
	}

	// Get a pointer to the top file stack element, and initialise it.

	top_file_ptr = &file_stack[next_file_index++];
	top_file_ptr->spot_file = spot_file;
	top_file_ptr->file_buffer = file_buffer_ptr;
	top_file_ptr->file_size = file_size;

	// Initialise the file position and file buffer pointer.

	top_file_ptr->file_position = 0;
	top_file_ptr->file_buffer_ptr = top_file_ptr->file_buffer;

	// If this is a spot file, initialise the current line number.

	if (spot_file)
		spot_line_no = 0;

	// Close the file.

	fclose(fp);
	return(true);
}

//------------------------------------------------------------------------------
// Open a file in the currently open ZIP archive, and push it onto the
// parser's file stack.  This file becomes the one that is being parsed.
//------------------------------------------------------------------------------

bool
push_ZIP_file(const char *file_path)
{
	unz_file_info info;
	char *file_buffer_ptr;

	// Attempt to locate the file in the ZIP archive.

	if (unzLocateFile(ZIP_archive_handle, file_path, 0) != UNZ_OK)
		return(false);

	// Get the uncompressed size of the file, and allocate the file buffer.

	unzGetCurrentFileInfo(ZIP_archive_handle, &info, NULL, 0, NULL, 0, NULL, 0);
	if ((file_buffer_ptr = new char[info.uncompressed_size]) == NULL)
		return(false);

	// Get a pointer to the top file stack element, and initialise it.

	top_file_ptr = &file_stack[next_file_index++];
	top_file_ptr->spot_file = false;
	top_file_ptr->file_buffer = file_buffer_ptr;
	top_file_ptr->file_size = info.uncompressed_size;

	// Open the file, read it into the file buffer, then close the file.

	unzOpenCurrentFile(ZIP_archive_handle);
	unzReadCurrentFile(ZIP_archive_handle, top_file_ptr->file_buffer,
		top_file_ptr->file_size);
	unzCloseCurrentFile(ZIP_archive_handle);

	// Initialise the file position and file buffer pointer.

	top_file_ptr->file_position = 0;
	top_file_ptr->file_buffer_ptr = top_file_ptr->file_buffer;
	return(true);
}

//------------------------------------------------------------------------------
// Open the first file with the given extension in the currently open ZIP 
// archive, and push it onto the parser's file stack.  This file becomes the one
// that is being parsed.
//------------------------------------------------------------------------------

bool
push_ZIP_file_with_ext(const char *file_ext)
{
	unz_file_info info;
	char file_name[_MAX_PATH];
	char *ext_ptr;
	bool found_file;
	char *file_buffer_ptr;

	// Go to the first file in the ZIP archive.

	if (unzGoToFirstFile(ZIP_archive_handle) != UNZ_OK)
		return(false);

	// Look for a file with the requested extension...

	found_file = false;
	do {

		// Get the information for the current file.

		unzGetCurrentFileInfo(ZIP_archive_handle, &info, file_name, _MAX_PATH,
			NULL, 0, NULL, 0);

		// If the file name ends with the requested extension, break out of the
		// loop.

		ext_ptr = strrchr(file_name, '.');
		if (ext_ptr && !stricmp(ext_ptr, file_ext)) {
			found_file = true;
			break;
		}

		// Otherwise move onto the next file in the ZIP archive.

	} while (unzGoToNextFile(ZIP_archive_handle) == UNZ_OK);

	// If we didn't find a file with the requested extension, return a failure
	// status.

	if (!found_file)
		return(false);

	// Get the uncompressed size of the file, and allocate the file buffer.

	if ((file_buffer_ptr = new char[info.uncompressed_size]) == NULL)
		return(false);

	// Get a pointer to the top file stack element, and initialise it.

	top_file_ptr = &file_stack[next_file_index++];
	top_file_ptr->spot_file = false;
	top_file_ptr->file_buffer = file_buffer_ptr;
	top_file_ptr->file_size = info.uncompressed_size;

	// Open the file, read it into the file buffer, then close the file.

	unzOpenCurrentFile(ZIP_archive_handle);
	unzReadCurrentFile(ZIP_archive_handle, top_file_ptr->file_buffer,
		top_file_ptr->file_size);
	unzCloseCurrentFile(ZIP_archive_handle);

	// Initialise the file position and file buffer pointer.

	top_file_ptr->file_position = 0;
	top_file_ptr->file_buffer_ptr = top_file_ptr->file_buffer;
	return(true);
}

//------------------------------------------------------------------------------
// Push a buffer onto the parser's file stack.  This "file" becomes the one
// that is being parsed.
//------------------------------------------------------------------------------

bool
push_buffer(const char *buffer_ptr, int buffer_size)
{
	// Get a pointer to the top file stack element, and initialise it.

	top_file_ptr = &file_stack[next_file_index++];
	top_file_ptr->spot_file = false;
	top_file_ptr->file_buffer = NULL;
	top_file_ptr->file_size = buffer_size;

	// Allocate the file buffer, and copy the contents of the data buffer into
	// it.

	if ((top_file_ptr->file_buffer = new char[buffer_size]) == NULL) {
		pop_file();
		return(false);
	}
	memcpy(top_file_ptr->file_buffer, buffer_ptr, buffer_size);

	// Initialise the file position, and file buffer pointer.

	top_file_ptr->file_position = 0;
	top_file_ptr->file_buffer_ptr = top_file_ptr->file_buffer;
	return(true);
}

//------------------------------------------------------------------------------
// Rewind the file on top of the stack.
//------------------------------------------------------------------------------

void
rewind_file(void)
{
	// If there is no file on the stack, do nothing.

	if (top_file_ptr == NULL)
		return;

	// Initialise the file position, and file buffer pointer.

	top_file_ptr->file_position = 0;
	top_file_ptr->file_buffer_ptr = top_file_ptr->file_buffer;

	// If this is a spot file, initialise the current line number.

	if (top_file_ptr->spot_file)
		spot_line_no = 0;
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

	// Delete the file buffer.

	if (top_file_ptr->file_buffer)
		delete []top_file_ptr->file_buffer;

	// If this is a spot file, reset the current line number.

	if (top_file_ptr->spot_file)
		spot_line_no = 0;
	
	// Pop the file off the stack.  If there is a file below it, make it the
	// new top file.

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
// Read the specified number of bytes from the top file into the specified 
// buffer.  The number of bytes actually read is returned.
//------------------------------------------------------------------------------

int
read_file(byte *buffer_ptr, int bytes)
{
	int bytes_to_copy;

	// If the file position is at the end of the file, don't copy any bytes and
	// return zero.

	if (top_file_ptr->file_position == top_file_ptr->file_size)
		return(0);

	// Determine how many bytes can be read from the current file position.

	if (top_file_ptr->file_position + bytes > top_file_ptr->file_size)
		bytes_to_copy = top_file_ptr->file_size - top_file_ptr->file_position;
	else
		bytes_to_copy = bytes;

	// Copy the available bytes into the supplied buffer, increment the file
	// position and file buffer pointer, and return the number of bytes copied.

	memcpy(buffer_ptr, top_file_ptr->file_buffer_ptr, bytes_to_copy);
	top_file_ptr->file_position += bytes_to_copy;
	top_file_ptr->file_buffer_ptr += bytes_to_copy;
	return(bytes_to_copy);
}

//------------------------------------------------------------------------------
// Copy the contents of the top file into the specified file, performing
// carriage return/line feed conversions if text_file is TRUE.
//------------------------------------------------------------------------------

bool
copy_file(const char *file_path, bool text_file)
{
	FILE *fp;
	char *file_buffer_ptr, *end_buffer_ptr;
	char ch;

	// If there is no file open, return failure.

	if (top_file_ptr == NULL)
		return(false);

	// Open the target file.

	if ((fp = fopen(file_path, "wb")) == NULL)
		return(false);

	// If the top file is a text file, copy the buffer of the top file byte by
	// byte, converting individual 0x0d or 0x0a characters to the combination
	// 0x0d 0x0a.

	if (text_file) {
		file_buffer_ptr = top_file_ptr->file_buffer;
		end_buffer_ptr = file_buffer_ptr + top_file_ptr->file_size;
		while (file_buffer_ptr < end_buffer_ptr) {
			ch = *file_buffer_ptr++;
			switch (ch) {
			case '\r':
				if (file_buffer_ptr < end_buffer_ptr &&
					*file_buffer_ptr == '\n')
					file_buffer_ptr++;
			case '\n':
				fputc('\r', fp);
				fputc('\n', fp);
				break;
			default:
				fputc(ch, fp);
			}
		}
	}
	
	// If the top file is not a text file, copy the entire buffer of the top
	// file into the target file without modifications.

	else {
		if (!fwrite(top_file_ptr->file_buffer, top_file_ptr->file_size, 1, fp)) {
			fclose(fp);
			return(false);
		}
	}
	
	// Return success.

	fclose(fp);
	return(true);
}

//------------------------------------------------------------------------------
// Read the next line from the top file.  If the end of the file has been 
// reached, generate an error.
//------------------------------------------------------------------------------

void
read_line(void)
{
	long line_position, line_length;
	char *line_ptr;
	bool got_carriage_return;

	// If the file position is at the end of the file, generate an error.
	// Otherwise the start of the line is at the current file position.

	if (top_file_ptr->file_position == top_file_ptr->file_size)
		error("Unexpected end of file");
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

	// If this is a spot file, increment the current line number.

	if (top_file_ptr->spot_file)
		spot_line_no++;

	// Copy the line into the line buffer, excluding the end of line character.

	line_length = top_file_ptr->file_position - line_position;
	line_buffer.copy(line_ptr, line_length);
	line_buffer_ptr = line_buffer;

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
// Write a diagnostic message to the log file in the Flatland directory.
//------------------------------------------------------------------------------

void
diagnose(const char *format, ...)
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

//------------------------------------------------------------------------------
// Write a diagnostic message to the log file in the Flatland directory, with
// the given number of tabs prepended to the message.
//------------------------------------------------------------------------------

void
diagnose(int tabs, const char *format, ...)
{
	va_list arg_ptr;
	char message[BUFSIZ];
	FILE *fp;

	va_start(arg_ptr, format);
	vsprintf(message, format, arg_ptr);
	va_end(arg_ptr);
	if ((fp = fopen(log_file_path, "a")) != NULL) {
		while (tabs > 0) {
			fprintf(fp, "  ");
			tabs--;
		}
		fprintf(fp, "%s\n", message);
		fclose(fp);
	}
}

//------------------------------------------------------------------------------
// Write a message to the error log.
//------------------------------------------------------------------------------

void
write_error_log(const char *message)
{
	FILE *fp;

	if ((fp = fopen(error_log_file_path, "a")) != NULL) {
		fputs(message, fp);
		fclose(fp);
	}
}

//------------------------------------------------------------------------------
// Write a formatted warning message in a similiar fashion to printf() in the
// error log file.
//------------------------------------------------------------------------------

void
warning(const char *format, ...)
{
	va_list arg_ptr;
	char message[BUFSIZ];

	// Format the message, then write it to the error log.

	va_start(arg_ptr, format);
	vsprintf(message, format, arg_ptr);
	va_end(arg_ptr);
	if (top_file_ptr != NULL && spot_line_no > 0)
		sprintf(error_str, "<B>Warning on line %d:</B> %s.\n<BR>\n",
			spot_line_no, message);
	else
		sprintf(error_str, "<B>Warning:</B> %s.\n<BR>\n", message);
	write_error_log(error_str);
	warnings = true;
}

//------------------------------------------------------------------------------
// Write a formatted memory warning message in the error log file.
//------------------------------------------------------------------------------

void
memory_warning(const char *object)
{
	warning("Insufficient memory for allocating %s", object);
}

//------------------------------------------------------------------------------
// Display a formatted error message in a similiar fashion to printf(), then
// close the open file(s) and throw an exception.
//------------------------------------------------------------------------------

void
error(const char *format, ...)
{
	va_list arg_ptr;
	char message[BUFSIZ];

	// Create the error message string.

	va_start(arg_ptr, format);
	vsprintf(message, format, arg_ptr);
	va_end(arg_ptr);
	if (top_file_ptr != NULL && spot_line_no > 0)
		sprintf(error_str, "<B>Error on line %d:</B> %s.\n<BR>\n", spot_line_no, 
			message);
	else
		sprintf(error_str, "<B>Error:</B> %s.<BR>\n", message);

	// Throw the error message.

	throw (char *)error_str;
}

//------------------------------------------------------------------------------
// Set the low memory flag, display a formatted memory error message, then close
// the open file(s) and throw an exception.
//------------------------------------------------------------------------------

void
memory_error(const char *object)
{
	low_memory = true;
	error("Insufficient memory for allocating %s", object);
}

//------------------------------------------------------------------------------
// Verify that the given integer is within the specified range; if not,
// generate a fatal error.
//------------------------------------------------------------------------------

bool
check_int_range(int value, int min, int max)
{
	return(value >= min && value <= max); 
}

//------------------------------------------------------------------------------
// Verify that the given float is within the specified range; if not,
// generate a fatal error.
//------------------------------------------------------------------------------

bool
check_float_range(float value, float min, float max)
{
	return(FGE(value, min) && FLE(value, max));
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
// Display an error message for a bad parameter value.
//------------------------------------------------------------------------------

static void
value_error(param *param_ptr)
{
	error("Expected %s as the value for attribute '%s'", 
		value_type_str[param_ptr->value_type - 192],
		get_token_str(param_ptr->param_name));
}

//------------------------------------------------------------------------------
// Display an warning message for a bad parameter value.
//------------------------------------------------------------------------------

static void
value_warning(param *param_ptr)
{
	warning("Expected %s as the value for attribute '%s'; using default "
		"value for this attribute instead", 
		value_type_str[param_ptr->value_type - 192],
		get_token_str(param_ptr->param_name));
}

//------------------------------------------------------------------------------
// Extract the next token from the current line buffer.
//------------------------------------------------------------------------------

static void
extract_token(bool inside_comment)
{
	tokendef *tokendef_ptr, *first_tokendef_ptr;
	char *token_ptr, *end_token_ptr;
	int string_len, min_string_len;

	// Skip over leading white space.

	while (*line_buffer_ptr == ' ' || *line_buffer_ptr == '\t')
		line_buffer_ptr++;

	// If we've come to the end of the line, return TOKEN_NONE.

	if (*line_buffer_ptr == '\0') {
		file_token = TOKEN_NONE;
		file_token_str = "";
		return;
	}

	// Find the first occurrance of an XML token in the line, if any exists

	min_string_len = strlen(line_buffer_ptr);
	tokendef_ptr = xml_token_table;
	while (tokendef_ptr->token_name != NULL) {
		if ((token_ptr = strstr(line_buffer_ptr, tokendef_ptr->token_name)) 
			!= NULL) {
			string_len = token_ptr - line_buffer_ptr;
			if (string_len < min_string_len) {
				min_string_len = string_len;
				first_tokendef_ptr = tokendef_ptr;
			}
		}
		tokendef_ptr++;
	}

	// If an XML token appears at the start of the line...

	if (min_string_len == 0) {

		// If the token is a quotation mark and we are not inside of a comment,
		// all characters up to a matching quotation mark will be returned as a
		// string value.  If the end of the line is reached before a matching
		// quotation mark is found, this is an error.

		if (first_tokendef_ptr->token_val == TOKEN_QUOTE && !inside_comment) {

			// Remember the quotation mark used.

			char quote_ch = *line_buffer_ptr++;

			// Skip over leading white space in string.

			while (*line_buffer_ptr == ' ' || *line_buffer_ptr == '\t')
				line_buffer_ptr++;

			// Now locate end of string.

			token_ptr = line_buffer_ptr;
			while (*token_ptr != '\0' && *token_ptr != quote_ch)
				token_ptr++;
			if (*token_ptr != quote_ch)
				error("String is missing closing quotation mark");
			end_token_ptr = token_ptr;

			// Back up over any trailing white space in string.

			while (token_ptr > line_buffer_ptr && 
				(*(token_ptr - 1) == ' ' || *(token_ptr - 1) == '\t'))
				token_ptr--;

			// Copy string to token buffer, then move the line buffer pointer
			// past the closing quotation mark.

			string_len = token_ptr - line_buffer_ptr;
			file_token = VALUE_STRING;
			file_token_str.copy(line_buffer_ptr, string_len);
			line_buffer_ptr = end_token_ptr + 1;
		}

		// If the token is not a quotation mark or we are inside a comment,
		// simply return the token.

		else {
			file_token = first_tokendef_ptr->token_val;
			file_token_str = first_tokendef_ptr->token_name;
			line_buffer_ptr += strlen(file_token_str);
		}
	}

	// If the XML token does not begin at the start of the line, or there was no
	// XML token...

	else {

		// Extract the token string up to the start of the XML token or end of
		// line.

		file_token_str.copy(line_buffer_ptr, min_string_len);
		line_buffer_ptr += min_string_len;

		// Attempt to match a pre-defined token against the token string.  If 
		// found, return it.

		tokendef_ptr = token_table;
		while (tokendef_ptr->token_name != NULL) {
			if (!stricmp(file_token_str, tokendef_ptr->token_name)) {
				file_token = tokendef_ptr->token_val;
				return;
			}
			tokendef_ptr++;
		}

		// Return the token as an unknown token.

		file_token = TOKEN_UNKNOWN;
	}
}

//------------------------------------------------------------------------------
// Read one token from the currently open file.
//------------------------------------------------------------------------------

static void
read_next_file_token(void)
{
	// If the file has just been opened, read the first line.

	if (top_file_ptr->file_position == 0)
		read_line();

	// Repeat the following loop until we have the next available token.

	while (true) {

		// Extract the next token, skipping over blank lines.

		extract_token(false);
		while (file_token == TOKEN_NONE) {
			read_line();
			extract_token(false);
		}

		// If the token is the start of a comment, skip over all tokens until
		// we find the end of the comment.

		if (file_token == TOKEN_OPEN_COMMENT) {
			extract_token(true);
			while (file_token != TOKEN_CLOSE_COMMENT) {
				if (file_token == TOKEN_NONE)
					read_line();
				extract_token(true);
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
	if (file_token != requested_token)
		error("Expected '%s' rather than '%s'", get_token_str(requested_token),
			file_token_str);
}

//==============================================================================
// General parameter value parsing functions.
//==============================================================================

//------------------------------------------------------------------------------
// Parse a name, or a list containing a single wildcard character or one or more
// names seperated by commas.  Only letters, digits and underscores are allowed
// in a name.  Leading and trailing spaces will be removed.
//------------------------------------------------------------------------------

bool
parse_name(const char *old_name, string *new_name, bool list)
{
	const char *old_name_ptr;
	char *temp_name;
	char *temp_name_ptr;

	// Allocate a temporary name string the same size as the old name string.

	if ((temp_name = new char[strlen(old_name) + 1]) == NULL)
		return(false);

	// Initialise the name pointers.

	old_name_ptr = old_name;
	temp_name_ptr = temp_name;

	// Parse one part name at a time...

	do {
		// Skip over the leading spaces.  If we reach the end of the string,
		// this is an error.

		while (*old_name_ptr == ' ')
			old_name_ptr++;
		if (!*old_name_ptr) {
			delete []temp_name;
			return(false);
		}

		// If the first character is a wildcard...

		if (*old_name_ptr == '*') {

			// If we're not parsing a list of names or this is not the first
			// name parsed, this is an error.

			if (!list || temp_name_ptr != temp_name) {
				delete []temp_name;
				return(false);
			}

			// Copy the wildcard to the temporary string, and skip over
			// trailing spaces.  Any additional characters after this is an
			// error.

			*temp_name_ptr++ = *old_name_ptr++;
			while (*old_name_ptr == ' ')
				old_name_ptr++;
			if (*old_name_ptr) {
				delete []temp_name;
				return(false);
			}

			// Terminate the temporary string, copy it to the new name string,
			// then delete the temporary string before returning success.

			*temp_name_ptr = '\0';
			*new_name = temp_name;
			delete []temp_name;
			return(true);
		}

		// Step through the sequence of letters, digits and underscores, 
		// copying them to the temporary string.

		while (*old_name_ptr == '_' || isalnum(*old_name_ptr))
			*temp_name_ptr++ = *old_name_ptr++;

		// Skip over any trailing spaces.

		while (*old_name_ptr == ' ')
			old_name_ptr++;

		// If the next character is not a string terminator or comma, this is
		// an error.  Otherwise append the string terminator or comma to the
		// temporary string.

		if (*old_name_ptr != '\0' && *old_name_ptr != ',') {
			delete []temp_name;
			return(false);
		}
		*temp_name_ptr = *old_name_ptr;

		// If the last character was a string terminator, we're done so copy
		// the temporary string to the new name string, delete the temporary
		// string and return success.

		if (*old_name_ptr == '\0') {
			*new_name = temp_name;
			delete []temp_name;
			return(true);
		}

		// If a list was not requested, a comma is an error.

		if (!list) {
			delete []temp_name;
			return(false);
		}

		// Otherwise skip over the comma and parse the next name.

		old_name_ptr++;
		temp_name_ptr++;
	} while (true);
}

//------------------------------------------------------------------------------
// Begin the parsing of the current file token as a parameter value string.
//------------------------------------------------------------------------------

void
start_parsing_value(char *token_str = file_token_str)
{
	value_str = token_str;
	value_str_ptr = token_str;
}

//------------------------------------------------------------------------------
// If a token matches in the parameter value string, skip over it and return
// TRUE.
//------------------------------------------------------------------------------

bool
token_in_value_is(const char *token_str)
{
	// Skip over leading white space.

	while (*value_str_ptr == ' ' || *value_str_ptr == '\t')
		value_str_ptr++;

	// If we've reached the end of the parameter value string or the token
	// doesn't match the parameter value string, return false.

	if (*value_str_ptr == '\0' ||
		strnicmp(value_str_ptr, token_str, strlen(token_str)))
		return(false);
	
	// Otherwise skip over the matched token string and return true.

	value_str_ptr += strlen(token_str);
	return(true);
}

//------------------------------------------------------------------------------
// Parse an integer in the current parameter value string.
//------------------------------------------------------------------------------

bool
parse_integer_in_value(int *int_ptr)
{
	char *last_ch_ptr;

	// Skip over leading white space.

	while (*value_str_ptr == ' ' || *value_str_ptr == '\t')
		value_str_ptr++;

	// Attempt to parse the integer; if no characters were parsed, this means
	// there was no valid integer in the parameter value string so return false.

	*int_ptr = strtol(value_str_ptr, &last_ch_ptr, 10);
	if (last_ch_ptr == value_str_ptr)
		return(false);

	// Otherwise skip over matched integer in parameter value string and return
	// true.

	value_str_ptr = last_ch_ptr;
	return(true);
}

//------------------------------------------------------------------------------
// Parse an integer in the current parameter value string; a signed or zero
// integer is considered to be relative.  An unsigned integer must not exceed
// the maximum value given.
//------------------------------------------------------------------------------

static bool
parse_relative_integer_in_value(int *int_ptr, bool *sign_ptr, int max_int)
{
	// If a "+" or "-" sign precedes the integer, set the sign flag and back
	// up one character.

	*sign_ptr = false;
	if (token_in_value_is("+") || token_in_value_is("-")) {
		*sign_ptr = true;
		value_str_ptr--;
	}

	// If the integer cannot be parsed, this is an error.

	if (!parse_integer_in_value(int_ptr))
		return(false);

	// If the integer parsed was zero, set the sign flag.

	if (*int_ptr == 0) {
		*sign_ptr = true;
	}

	// If there was no sign, compare the integer against the maximum value.
	// Then decrease the integer by one to make it zero-based.

	if (!*sign_ptr) {
		if (*int_ptr > max_int)
			return(false);
		(*int_ptr)--;
	}
	return(true);
}

//------------------------------------------------------------------------------
// Parse a float in the current parameter value string.
//------------------------------------------------------------------------------

bool
parse_float_in_value(float *float_ptr)
{
	char *last_ch_ptr;

	// Skip over leading white space.

	while (*value_str_ptr == ' ' || *value_str_ptr == '\t')
		value_str_ptr++;

	// Attempt to parse the float; if no characters were parsed, this means
	// there was no valid float in the parameter value string, so return false.

	*float_ptr = (float)strtod(value_str_ptr, &last_ch_ptr);
	if (last_ch_ptr == value_str_ptr)
		return(false);

	// If the last character passed was a decimal point, and there is a second
	// decimal point immediately following, then back up one character; this
	// allows the ".." token to follow a float.

	if (*(last_ch_ptr - 1) == '.' && *last_ch_ptr == '.')
		last_ch_ptr--;

	// Otherwise skip over matched float in parameter value string and return
	// true.

	value_str_ptr = last_ch_ptr;
	return(true);
}

//------------------------------------------------------------------------------
// Parse a name in the current parameter value string.
//------------------------------------------------------------------------------

static bool
parse_name_in_value(string *string_ptr, bool list)
{
	if (!parse_name(file_token_str, string_ptr, list))
		return(false);
	value_str_ptr += strlen(file_token_str);
	return(true);
}

//------------------------------------------------------------------------------
// End parsing of the current parameter value string.
//------------------------------------------------------------------------------

bool
stop_parsing_value(void)
{
	// Skip over trailing white space in the parameter value string.

	while (*value_str_ptr == ' ' || *value_str_ptr == '\t')
		value_str_ptr++;

	// If we haven't reached the end of the parameter value string, this is an
	// error.

	return(*value_str_ptr == '\0');
}

//==============================================================================
// Specialised parameter value parsing functions.
//==============================================================================

//------------------------------------------------------------------------------
// Parse current token as an action trigger.
//------------------------------------------------------------------------------

static bool
parse_action_trigger(int *trigger_flags_ptr)
{
	if (token_in_value_is("roll on"))
		*trigger_flags_ptr = ROLL_ON;
	else if (token_in_value_is("roll off"))
		*trigger_flags_ptr = ROLL_OFF;
	else if (token_in_value_is("step in"))
		*trigger_flags_ptr = STEP_IN;
	else if (token_in_value_is("step out"))
		*trigger_flags_ptr = STEP_OUT;
	else if (token_in_value_is("click on"))
		*trigger_flags_ptr = CLICK_ON;
	else if (token_in_value_is("timer"))
		*trigger_flags_ptr = TIMER;
	else if (token_in_value_is("location"))
		*trigger_flags_ptr = LOCATION;
	else if (token_in_value_is("proximity"))
		*trigger_flags_ptr = PROXIMITY;
	else
		return(false);
	return(true);
}

//------------------------------------------------------------------------------
// Parse current file token as an alignment mode.
//------------------------------------------------------------------------------

static bool
parse_alignment(alignment *alignment_ptr, bool allow_mouse)
{
	if (token_in_value_is("top-right"))
		*alignment_ptr = TOP_RIGHT;
	else if (token_in_value_is("top-left"))
		*alignment_ptr = TOP_LEFT;
	else if (token_in_value_is("top"))
		*alignment_ptr = TOP;
	else if (token_in_value_is("left"))
		*alignment_ptr = LEFT;
	else if (token_in_value_is("center") || token_in_value_is("centre"))
		*alignment_ptr = CENTRE;
	else if (token_in_value_is("right"))
		*alignment_ptr = RIGHT;
	else if (token_in_value_is("bottom-right"))
		*alignment_ptr = BOTTOM_RIGHT;
	else if (token_in_value_is("bottom-left"))
		*alignment_ptr = BOTTOM_LEFT;
	else if (token_in_value_is("bottom"))
		*alignment_ptr = BOTTOM;
	else if (token_in_value_is("mouse") && allow_mouse)
		*alignment_ptr = MOUSE;
	else
		return(false);
	return(true);
}

//------------------------------------------------------------------------------
// Parse current file token as a block reference (either a symbol or location).
//------------------------------------------------------------------------------

static bool parse_relative_coordinates(relcoords *relcoords_ptr);
static bool parse_symbol(word *symbol_ptr, bool disallow_empty_block_symbol);

static bool
parse_block_reference(blockref *block_ref_ptr)
{
	if (parse_symbol(&block_ref_ptr->symbol, false)) {
		block_ref_ptr->is_symbol = true;
		return(true);
	}
	if (parse_relative_coordinates(&block_ref_ptr->location)) {
		block_ref_ptr->is_symbol = false;
		return(true);
	}
	return(false);
}

//------------------------------------------------------------------------------
// Parse current file token as a block type.
//------------------------------------------------------------------------------

static bool
parse_block_type(blocktype *block_type_ptr)
{
	if (token_in_value_is("structural"))
		*block_type_ptr = STRUCTURAL_BLOCK;
	else if (token_in_value_is("multifaceted sprite"))
		*block_type_ptr = MULTIFACETED_SPRITE;
	else if (token_in_value_is("angled sprite"))
		*block_type_ptr = ANGLED_SPRITE;
	else if (token_in_value_is("revolving sprite"))
		*block_type_ptr = REVOLVING_SPRITE;
	else if (token_in_value_is("facing sprite"))
		*block_type_ptr = FACING_SPRITE;
	else if (token_in_value_is("player sprite"))
		*block_type_ptr = PLAYER_SPRITE;
	else
		return(false);
	return(true);
}

//------------------------------------------------------------------------------
// Parse the current file token as a boolean "yes" or "no".
//------------------------------------------------------------------------------

static bool
parse_boolean(bool *bool_ptr)
{
	if (token_in_value_is("yes"))
		*bool_ptr = true;
	else if (token_in_value_is("no"))
		*bool_ptr = false;
	else
		return(false);
	return(true);
}

//------------------------------------------------------------------------------
// Parse current file token as an angle in degrees, and adjust it so that it's
// in a positive range.
//------------------------------------------------------------------------------

static bool
parse_degrees(float *float_ptr)
{
	if (!parse_float_in_value(float_ptr))
		return(false);
	*float_ptr = pos_adjust_angle(*float_ptr);
	return(true);
}

//------------------------------------------------------------------------------
// Parse current file token as a range of delays, in seconds.
//------------------------------------------------------------------------------

static bool
parse_delay_range(delayrange *delay_range_ptr)
{
	float min_delay, max_delay;

	if (!parse_float_in_value(&min_delay) || min_delay < 0.0f)
		return(false);
	delay_range_ptr->min_delay_ms = (int)(min_delay * 1000.0f);
	if (token_in_value_is("..")) {
		if (!parse_float_in_value(&max_delay) || max_delay < min_delay)
			return(false);
		delay_range_ptr->delay_range_ms = (int)(max_delay * 1000.0f) -
			delay_range_ptr->min_delay_ms;
	} else
		delay_range_ptr->delay_range_ms = 0;
	return(true);
}

//------------------------------------------------------------------------------
// Parse current file token as a direction (rotations around the Y and X axes,
// in that order).
//------------------------------------------------------------------------------

static bool
parse_direction(direction *direction_ptr)
{
	if (!token_in_value_is("(") ||
		!parse_float_in_value(&direction_ptr->angle_y) ||
		!token_in_value_is(",") ||
		!parse_float_in_value(&direction_ptr->angle_x) || 
		!token_in_value_is(")"))
		return(false);
	return(true);
}

//------------------------------------------------------------------------------
// Parse current file token as a range of directions.
//------------------------------------------------------------------------------

static bool
parse_direction_range(dirrange *dirrange_ptr)
{
	if (!parse_direction(&dirrange_ptr->min_direction))
		return(false);
	if (token_in_value_is("..")) {
		if (!parse_direction(&dirrange_ptr->max_direction))
			return(false);
	} else {
		dirrange_ptr->max_direction.angle_y = 
			dirrange_ptr->min_direction.angle_y;
		dirrange_ptr->max_direction.angle_x = 
			dirrange_ptr->min_direction.angle_x;
	}
	return(true);
}

//------------------------------------------------------------------------------
// Parse the current file token as a double character symbol.
//------------------------------------------------------------------------------

static bool
parse_double_symbol(word *symbol_ptr)
{
	if (string_to_double_symbol(file_token_str, symbol_ptr, true)) {
		value_str_ptr += 2;
		return(true);
	}
	return(false);
}

//------------------------------------------------------------------------------
// Parse current token as exit trigger flags.
//------------------------------------------------------------------------------

static bool
parse_exit_trigger(int *trigger_flags_ptr)
{
	*trigger_flags_ptr = 0;
	do {
		if (token_in_value_is("click on"))
			*trigger_flags_ptr |= CLICK_ON;
		else if (token_in_value_is("step on"))
			*trigger_flags_ptr |= STEP_ON;
		else
			return(false);
	} while (token_in_value_is(","));
	return(true);
}

//------------------------------------------------------------------------------
// Parse current file token as a direction (rotations around the Y and X axes,
// in that order), but the second angle is optional.
//------------------------------------------------------------------------------

static bool
parse_heading(direction *direction_ptr)
{
	if (!parse_float_in_value(&direction_ptr->angle_y))
		return(false);
	if (token_in_value_is(",")) {
		if (!parse_float_in_value(&direction_ptr->angle_x))
			return(false);
	} else
		direction_ptr->angle_x = 0.0f;
	return(true);
}

//------------------------------------------------------------------------------
// Parse current file token as map coordinates.
//------------------------------------------------------------------------------

static bool
parse_map_coordinates(mapcoords *mapcoords_ptr)
{
	if (!token_in_value_is("(") || 
		!parse_integer_in_value(&mapcoords_ptr->column) ||
		!check_int_range(mapcoords_ptr->column, 1, world_ptr->columns) ||
		!token_in_value_is(",") ||
		!parse_integer_in_value(&mapcoords_ptr->row) ||
		!check_int_range(mapcoords_ptr->row, 1, world_ptr->rows) ||
		!token_in_value_is(",") ||
		!parse_integer_in_value(&mapcoords_ptr->level) ||
		!check_int_range(mapcoords_ptr->level, 1, world_ptr->levels) ||
		!token_in_value_is(")"))
		return(false);	
	mapcoords_ptr->column--;
	mapcoords_ptr->row--;
	mapcoords_ptr->level--;
	return(true);
}

//------------------------------------------------------------------------------
// Parse current file token as map dimensions.
//------------------------------------------------------------------------------

static bool
parse_map_dimensions(mapcoords *mapdims_ptr)
{
	return(token_in_value_is("(") &&
		parse_integer_in_value(&mapdims_ptr->column) && 
		mapdims_ptr->column > 0 && 
		token_in_value_is(",") &&
		parse_integer_in_value(&mapdims_ptr->row) && 
		mapdims_ptr->row > 0 &&
		token_in_value_is(",") && 
		parse_integer_in_value(&mapdims_ptr->level) &&
		mapdims_ptr->level > 0 &&
		token_in_value_is(")"));
}

//------------------------------------------------------------------------------
// Parse current file token as the map scale in pixels, which is converted to
// a scaling factor.
//------------------------------------------------------------------------------

static bool
parse_map_scale(float *scale_ptr)
{
	if (!parse_float_in_value(scale_ptr) || FLE(*scale_ptr, 0.0f))
		return(false);
	*scale_ptr /= 256.0f;
	return(true);
}

//------------------------------------------------------------------------------
// Parse current file token as the map style.
//------------------------------------------------------------------------------

static bool
parse_map_style(mapstyle *map_style_ptr)
{
	if (token_in_value_is("single"))
		*map_style_ptr = SINGLE_MAP;
	else if (token_in_value_is("double"))
		*map_style_ptr = DOUBLE_MAP;
	else
		return(false);
	return(true);
}

//------------------------------------------------------------------------------
// Parse current file token as an orientation (rotations around the Y, X and Z
// axes, in that order).
//------------------------------------------------------------------------------

static bool
parse_orientation(orientation *orientation_ptr)
{
	// If the parameter value starts with a compass direction, this is the new
	// style of orientation.

	if (token_in_value_is("up"))
		orientation_ptr->direction = UP;
	else if (token_in_value_is("down"))
		orientation_ptr->direction = DOWN;
	else if (token_in_value_is("north"))
		orientation_ptr->direction = NORTH;
	else if (token_in_value_is("south"))
		orientation_ptr->direction = SOUTH;
	else if (token_in_value_is("east"))
		orientation_ptr->direction = EAST;
	else if (token_in_value_is("west"))
		orientation_ptr->direction = WEST;
	else
		orientation_ptr->direction = NONE;

	// If a compass direction was given, parse an angle of rotation around
	// the vector pointing in that direction.

	if (orientation_ptr->direction != NONE) {
		if (token_in_value_is(",")) {
			if (!parse_float_in_value(&orientation_ptr->angle))
				return(false);
			orientation_ptr->angle = 
				pos_adjust_angle(orientation_ptr->angle);
		} else
			orientation_ptr->angle = 0.0f;
	}

	// Otherwise parse three angles of rotation around the Y, X and Z axes,
	// in that order.

	else {
		if (!parse_float_in_value(&orientation_ptr->angle_y))
			return(false);
		orientation_ptr->angle_y = pos_adjust_angle(orientation_ptr->angle_y);
		if (token_in_value_is(",")) {
			if (!parse_float_in_value(&orientation_ptr->angle_x))
				return(false);
			orientation_ptr->angle_x = 
				pos_adjust_angle(orientation_ptr->angle_x);
		} else
			orientation_ptr->angle_x = 0.0f;
		if (token_in_value_is(",")) {
			if (!parse_float_in_value(&orientation_ptr->angle_z))
				return(false);
			orientation_ptr->angle_z = 
				pos_adjust_angle(orientation_ptr->angle_z);
		} else
			orientation_ptr->angle_z = 0.0f;
	}

	// Return success status.

	return(true);
}

//------------------------------------------------------------------------------
// Parse the current file token as a floating point percentage, then convert it
// into a value between 0.0 and 1.0.
//------------------------------------------------------------------------------

static bool
parse_percentage(float *percentage_ptr)
{
	if (!parse_float_in_value(percentage_ptr) ||
		!check_float_range(*percentage_ptr, 0.0, 100.0) ||
		!token_in_value_is("%"))
		return(false);
	*percentage_ptr /= 100.0f;
	return(true);
}

//------------------------------------------------------------------------------
// Parse the current file token as a floating point percentage range.
//------------------------------------------------------------------------------

static bool
parse_percentage_range(pcrange *pcrange_ptr)
{
	if (!parse_percentage(&pcrange_ptr->min_percentage))
		return(false);
	if (token_in_value_is("..")) {
		if (!parse_percentage(&pcrange_ptr->max_percentage))
			return(false);
	} else
		pcrange_ptr->max_percentage = pcrange_ptr->min_percentage;
	return(FLE(pcrange_ptr->min_percentage, pcrange_ptr->max_percentage));
}

//------------------------------------------------------------------------------
// Parse current file token as a playback mode.
//------------------------------------------------------------------------------

static bool
parse_playback_mode(playmode *playback_mode_ptr)
{
	if (token_in_value_is("looped"))
		*playback_mode_ptr = LOOPED_PLAY;
	else if (token_in_value_is("random"))
		*playback_mode_ptr = RANDOM_PLAY;
	else if (token_in_value_is("single"))
		*playback_mode_ptr = SINGLE_PLAY;
	else if (token_in_value_is("once"))
		*playback_mode_ptr = ONE_PLAY;
	else
		return(false);
	return(true);
}

//------------------------------------------------------------------------------
// Parse current file token as a point light style.
//------------------------------------------------------------------------------

static bool
parse_point_light_style(lightstyle *light_style_ptr)
{
	if (token_in_value_is("static"))
		*light_style_ptr = STATIC_POINT_LIGHT;
	else if (token_in_value_is("pulsate"))
		*light_style_ptr = PULSATING_POINT_LIGHT;
	else
		return(false);
	return(true);
}

//------------------------------------------------------------------------------
// Parse current token as popup trigger flags.
//------------------------------------------------------------------------------

static bool
parse_popup_trigger(int *trigger_flags_ptr)
{
	*trigger_flags_ptr = 0;
	do {
		if (token_in_value_is("proximity"))
			*trigger_flags_ptr |= PROXIMITY;
		else if (token_in_value_is("rollover")) 
			*trigger_flags_ptr |= ROLLOVER;
		else if (token_in_value_is("everywhere"))
			*trigger_flags_ptr |= EVERYWHERE;
		else
			return(false);
	} while (token_in_value_is(","));
	return(true);
}

//------------------------------------------------------------------------------
// Parse the current token as a projection (which overrides the direction that
// a polygon faces when calculating tiled or scaled texture projections).
//------------------------------------------------------------------------------

static bool
parse_projection(compass *projection_ptr)
{
	if (token_in_value_is("north"))
		*projection_ptr = NORTH;
	else if (token_in_value_is("south"))
		*projection_ptr = SOUTH;
	else if (token_in_value_is("east"))
		*projection_ptr = EAST;
	else if (token_in_value_is("west"))
		*projection_ptr = WEST;
	else if (token_in_value_is("top"))
		*projection_ptr = UP;
	else if (token_in_value_is("bottom"))
		*projection_ptr = DOWN;
	else
		return(false);
	return(true);
}

//------------------------------------------------------------------------------
// Parse the current file token as a radius in units of blocks, then convert it
// into world units.
//------------------------------------------------------------------------------

static bool
parse_radius(float *radius_ptr)
{
	if (!parse_float_in_value(radius_ptr) || FLE(*radius_ptr, 0.0f))
		return(false);
	*radius_ptr *= UNITS_PER_BLOCK;
	return(true);
}

//------------------------------------------------------------------------------
// Parse the current file token as a rectangle in units of pixels or
// percentages.
//------------------------------------------------------------------------------

static bool
parse_rectangle(video_rect *rect_ptr)
{
	// Parse x1.

	if (!parse_float_in_value(&rect_ptr->x1))
		return(false);
	if (token_in_value_is("%")) {
		rect_ptr->x1_is_ratio = true;
		rect_ptr->x1 /= 100.0f;
	} else
		rect_ptr->x1_is_ratio = false;

	// Parse y1.

	if (!token_in_value_is(",") || !parse_float_in_value(&rect_ptr->y1))
		return(false);
	if (token_in_value_is("%")) {
		rect_ptr->y1_is_ratio = true;
		rect_ptr->y1 /= 100.0f;
	} else
		rect_ptr->x1_is_ratio = false;

	// Parse x2.

	if (!token_in_value_is(",") || !parse_float_in_value(&rect_ptr->x2))
		return(false);
	if (token_in_value_is("%")) {
		rect_ptr->x2_is_ratio = true;
		rect_ptr->x2 /= 100.0f;
	} else
		rect_ptr->x2_is_ratio = false;

	// Parse y2.

	if (!token_in_value_is(",") || !parse_float_in_value(&rect_ptr->y2))
		return(false);
	if (token_in_value_is("%")) {
		rect_ptr->y2_is_ratio = true;
		rect_ptr->y2 /= 100.0f;
	} else
		rect_ptr->y2_is_ratio = false;

	// Return success status.

	return(true);
}

//------------------------------------------------------------------------------
// Parse current file token as absolute or relative map coordinates.
//------------------------------------------------------------------------------

static bool
parse_relative_coordinates(relcoords *relcoords_ptr)
{
	if (!token_in_value_is("(") || 
		!parse_relative_integer_in_value(&relcoords_ptr->column,
		 &relcoords_ptr->relative_column, world_ptr->columns) ||
		!token_in_value_is(",") || 
		!parse_relative_integer_in_value(&relcoords_ptr->row,
		 &relcoords_ptr->relative_row, world_ptr->rows) ||
		!token_in_value_is(",") || 
		!parse_relative_integer_in_value(&relcoords_ptr->level,
		 &relcoords_ptr->relative_level, world_ptr->levels) ||
		!token_in_value_is(")"))
		return(false);
	return(true);
}

//------------------------------------------------------------------------------
// Parse the current file token as an RGB colour triplet.
//------------------------------------------------------------------------------

static bool
parse_RGB(RGBcolour *colour_ptr)
{
	int red, green, blue;

	if (!token_in_value_is("(") || !parse_integer_in_value(&red) ||
		!check_int_range(red, 0, 255) || !token_in_value_is(",") ||
		!parse_integer_in_value(&green) || !check_int_range(green, 0, 255) ||
		!token_in_value_is(",") || !parse_integer_in_value(&blue) ||
		!check_int_range(blue, 0, 255) || !token_in_value_is(")"))
		return(false);
	colour_ptr->red = (float)red;
	colour_ptr->green = (float)green;
	colour_ptr->blue = (float)blue;
	return(true);
}

//------------------------------------------------------------------------------
// Parse the current file token as a shape.
//------------------------------------------------------------------------------

static bool
parse_shape(shape *shape_ptr)
{
	if (token_in_value_is("rect"))
		*shape_ptr = RECT_SHAPE;
	else if (token_in_value_is("circle"))
		*shape_ptr = CIRCLE_SHAPE;
	else
		return(false);
	return(true);
}

//------------------------------------------------------------------------------
// Parse the current file token as a single character symbol.
//------------------------------------------------------------------------------

static bool
parse_single_symbol(char *symbol_ptr)
{
	char symbol;

	if (string_to_single_symbol(file_token_str, &symbol, true)) {
		*symbol_ptr = symbol;
		value_str_ptr++;
		return(true);
	}
	return(false);
}

//------------------------------------------------------------------------------
// Parse the current file token as a size.
//------------------------------------------------------------------------------

static bool
parse_size(size *size_ptr)
{
	return(token_in_value_is("(") && parse_integer_in_value(&size_ptr->width) &&
		size_ptr->width > 0 && token_in_value_is(",") &&
		parse_integer_in_value(&size_ptr->height) && size_ptr->height > 0 &&
		token_in_value_is(")"));
}

//------------------------------------------------------------------------------
// Parse current file token as a spot light style.
//------------------------------------------------------------------------------

static bool
parse_spot_light_style(lightstyle *light_style_ptr)
{
	if (token_in_value_is("static"))
		*light_style_ptr = STATIC_SPOT_LIGHT;
	else if (token_in_value_is("revolve"))
		*light_style_ptr = REVOLVING_SPOT_LIGHT;
	else if (token_in_value_is("search"))
		*light_style_ptr = SEARCHING_SPOT_LIGHT;
	else
		return(false);
	return(true);
}

//------------------------------------------------------------------------------
// Parse the current file token as a size.
//------------------------------------------------------------------------------

static bool
parse_sprite_size(size *size_ptr)
{
	return(token_in_value_is("(") && parse_integer_in_value(&size_ptr->width) &&
		check_int_range(size_ptr->width, 1, 256) && token_in_value_is(",") &&
		parse_integer_in_value(&size_ptr->height) && 
		check_int_range(size_ptr->height, 1, 256) && token_in_value_is(")"));
}

//------------------------------------------------------------------------------
// Parse the current file token as a stream command.
//------------------------------------------------------------------------------

/*
static bool
parse_stream_command(int *command_ptr)
{
	if (token_in_value_is("play once"))
		*command_ptr = STREAM_PLAY_ONCE;
	else if (token_in_value_is("play looped"))
		*command_ptr = STREAM_PLAY_LOOPED;
	else if (token_in_value_is("stop"))
		*command_ptr = STREAM_STOP;
	else if (token_in_value_is("pause"))
		*command_ptr = STREAM_PAUSE;
	else
		return(false);
	return(true);
}
*/

//------------------------------------------------------------------------------
// Parse the current file token as a single or double character symbol,
// depending on what map style is active.
//------------------------------------------------------------------------------

static bool
parse_symbol(word *symbol_ptr, bool disallow_empty_block_symbol)
{
	char symbol;

	switch (world_ptr->map_style) {
	case SINGLE_MAP:
		if (string_to_single_symbol(file_token_str, &symbol, 
			disallow_empty_block_symbol)) {
			*symbol_ptr = symbol;
			value_str_ptr++;
			return(true);
		}
		break;
	case DOUBLE_MAP:
		if (string_to_double_symbol(file_token_str, symbol_ptr, 
			disallow_empty_block_symbol)) {
			value_str_ptr += 2;
			return(true);
		}
	}
	return(false);
}

//------------------------------------------------------------------------------
// Parse current file token as a texture style.
//------------------------------------------------------------------------------

static bool
parse_texture_style(texstyle *texstyle_ptr)
{
	if (token_in_value_is("tiled"))
		*texstyle_ptr = TILED_TEXTURE;
	else if (token_in_value_is("scaled"))
		*texstyle_ptr = SCALED_TEXTURE;
	else if (token_in_value_is("stretched"))
		*texstyle_ptr = STRETCHED_TEXTURE;
	else
		return(false);
	return(true);
}

//------------------------------------------------------------------------------
// Parse current file token as a vertical alignment mode.
//------------------------------------------------------------------------------

static bool
parse_vertical_alignment(alignment *alignment_ptr)
{
	if (token_in_value_is("top"))
		*alignment_ptr = TOP;
	else if (token_in_value_is("center") || token_in_value_is("centre"))
		*alignment_ptr = CENTRE;
	else if (token_in_value_is("bottom"))
		*alignment_ptr = BOTTOM;
	else
		return(false);
	return(true);
}

//------------------------------------------------------------------------------
// Parse current file token as a version string, and convert it into a unique
// integer.
//------------------------------------------------------------------------------

static bool
parse_version(unsigned int *version_ptr)
{
	int version1, version2, version3, version4;

	// Parse first two integers in version string, seperated by a period.  If
	// they are not in the range of 0-255, this is an error.

	if (!parse_integer_in_value(&version1) || version1 < 0 || version1 > 255 ||
		!token_in_value_is(".") || !parse_integer_in_value(&version2) ||
		version2 < 0 || version2 > 255)
		return(false);

	// If the next token is a period, parse the third integer, otherwise set it
	// to zero.  If the parsed value is not in the range of 0-255, this is an
	// error.

	if (token_in_value_is(".")) {
		if (!parse_integer_in_value(&version3) || version3 < 0 || version3 > 255)
			return(false);
	} else
		version3 = 0;

	// If the next token is "b", parse the fourth integer, otherwise set it to
	// 255.  If the parsed value is not in the range of 1-254, this is an error.

	if (token_in_value_is("b")) {
		if (!parse_integer_in_value(&version4) || version4 < 1 || version4 > 254)
			return(false);
	} else
		version4 = 255;

	// Construct a single version number from the four integers.

	*version_ptr = (version1 << 24) | (version2 << 16) | (version3 << 8) | 
		version4;
	return(true);
}

//------------------------------------------------------------------------------
// Parse current file token as vertex coordinates, and convert them into world
// units.
//------------------------------------------------------------------------------

static bool
parse_vertex_coordinates(vertex_entry *vertex_entry_ptr)
{
	if (!token_in_value_is("(") || !parse_float_in_value(&vertex_entry_ptr->x) ||
		!token_in_value_is(",") || !parse_float_in_value(&vertex_entry_ptr->y) ||
		!token_in_value_is(",") || !parse_float_in_value(&vertex_entry_ptr->z) ||
		!token_in_value_is(")"))
		return(false);
	vertex_entry_ptr->x /= TEXELS_PER_UNIT;
	vertex_entry_ptr->y /= TEXELS_PER_UNIT;
	vertex_entry_ptr->z /= TEXELS_PER_UNIT;
	return(true);
}

//==============================================================================
// Parameter list parsing functions.
//==============================================================================

//------------------------------------------------------------------------------
// Prepare for matching a parameter list.
//------------------------------------------------------------------------------

static void
init_param_list(void)
{
	// Reset all flags indicating which parameters were matched.

	for (int param_index = 0; param_index < MAX_PARAMS; param_index++)
		matched_param[param_index] = false;
}

//------------------------------------------------------------------------------
// Parse a parameter name token, and return the index of the parameter entry
// that matches it.  No match or a duplicate match are flagged as an error.
// The equal sign and parameter value is also parsed (it must be a string
// value).
//-----------------------------------------------------------------------------

static int
parse_next_param(param *param_list, int params)
{
	token param_name;
	int param_index;

	// Parse the next valid parameter, skipping over invalid parameters.

	while (true) {

		// If the next token is ">" or "/>" then there are no more parameters
		// to parse, so break out of the loop.

		read_next_file_token();
		if (file_token == TOKEN_CLOSE_TAG || 
			file_token == TOKEN_CLOSE_SINGLE_TAG)
			break;

		// If the token is a string token or an equals sign, this is an error.
		// Otherwise store it as the parameter token.

		if (file_token == VALUE_STRING || file_token == TOKEN_EQUAL)
			error("Expected an attribute name rather than '%s'",
				file_token_str);
		param_name = file_token;
	
		// Parse the equal sign and the parameter value, which must be a
		// string value.

		match_next_file_token(TOKEN_EQUAL);
		read_next_file_token();
		if (file_token != VALUE_STRING)
			error("The value for parameter '%s' is not quoted",
				get_token_str(param_name));

		// If the parameter token was unknown or there is no parameter list,
		// skip over this parameter.

		if (param_name == TOKEN_UNKNOWN || param_list == NULL)
			continue;

		// Compare the parameter name against the entries in the parameter list;
		// if we find a match that hasn't been matched before, return the index
		// of that parameter entry.  Otherwise ignore this parameter and parse
		// the next.

		for (param_index = 0; param_index < params; param_index++)
			if (param_name == param_list[param_index].param_name &&
				!matched_param[param_index])
				return(param_index);
	}

	// Return an index of -1 if the end of the parameter list was encountered.

	return(-1);
}

//------------------------------------------------------------------------------
// Parse a parameter list for a given tag.
//-----------------------------------------------------------------------------

static void
parse_param_list(token tag_name, param *param_list, int params, token end_token)
{
	int param_index;
	param *param_ptr;
	void *variable_ptr;
	string name;
	bool success;

	// Initialise the parameter list.

	init_param_list();

	// Parse the parameter list.

	while ((param_index = parse_next_param(param_list, params)) >= 0) {
		param_ptr = &param_list[param_index];

		// Parse the contents of the parameter value according to the value
		// type expected.

		start_parsing_value();
		variable_ptr = param_ptr->variable_ptr;
		switch (param_ptr->value_type) {
		case VALUE_ACTION_TRIGGER:
			success = parse_action_trigger((int *)variable_ptr);
			break;
		case VALUE_ALIGNMENT:
			success = parse_alignment((alignment *)variable_ptr, false);
			break;
		case VALUE_BLOCK_REF:
			success = parse_block_reference((blockref *)variable_ptr);
			break;
		case VALUE_BLOCK_TYPE:
			success = parse_block_type((blocktype *)variable_ptr);
			break;
		case VALUE_BOOLEAN:
			success = parse_boolean((bool *)variable_ptr);
			break;
		case VALUE_DEGREES:
			success = parse_degrees((float *)variable_ptr);
			break;
		case VALUE_DELAY_RANGE:
			success = parse_delay_range((delayrange *)variable_ptr);
			break;
		case VALUE_DIRECTION:
			success = parse_direction((direction *)variable_ptr);
			break;
		case VALUE_DIRRANGE:
			success = parse_direction_range((dirrange *)variable_ptr);
			break;
		case VALUE_DOUBLE_SYMBOL:
			success = parse_double_symbol((word *)variable_ptr);
			break;
		case VALUE_EXIT_TRIGGER:
			success = parse_exit_trigger((int *)variable_ptr);
			break;
		case VALUE_FLOAT:
			success = parse_float_in_value((float *)variable_ptr);
			break;
		case VALUE_HEADING:
			success = parse_heading((direction *)variable_ptr);
			break;
		case VALUE_INTEGER:
			success = parse_integer_in_value((int *)variable_ptr);
			break;
		case VALUE_MAP_COORDS:
			success = parse_map_coordinates((mapcoords *)variable_ptr);
			break;
		case VALUE_MAP_DIMENSIONS:
			success = parse_map_dimensions((mapcoords *)variable_ptr);
			break;
		case VALUE_MAP_SCALE:
			success = parse_map_scale((float *)variable_ptr);
			break;
		case VALUE_MAP_STYLE:
			success = parse_map_style((mapstyle *)variable_ptr);
			break;
		case VALUE_NAME:
			success = parse_name_in_value((string *)variable_ptr, false);
			break;
		case VALUE_NAME_LIST:
			success = parse_name_in_value((string *)variable_ptr, true);	
			break;
		case VALUE_ORIENTATION:
			success = parse_orientation((orientation *)variable_ptr);
			break;
		case VALUE_PCRANGE:
			success = parse_percentage_range((pcrange *)variable_ptr);
			break;
		case VALUE_PERCENTAGE:
			success = parse_percentage((float *)variable_ptr);
			break;
		case VALUE_PLACEMENT:
			success = parse_alignment((alignment *)variable_ptr, true);
			break;
		case VALUE_PLAYBACK_MODE:
			success = parse_playback_mode((playmode *)variable_ptr);
			break;
		case VALUE_POINT_LIGHT_STYLE:
			success = parse_point_light_style((lightstyle *)variable_ptr);
			break;
		case VALUE_POPUP_TRIGGER:
			success = parse_popup_trigger((int *)variable_ptr);
			break;
		case VALUE_PROJECTION:
			success = parse_projection((compass *)variable_ptr);
			break;
		case VALUE_RADIUS:
			success = parse_radius((float *)variable_ptr);
			break;
		case VALUE_RECT:
			success = parse_rectangle((video_rect *)variable_ptr);
			break;
		case VALUE_REL_COORDS:
			success = parse_relative_coordinates((relcoords *)variable_ptr);
			break;
		case VALUE_RGB:
			success = parse_RGB((RGBcolour *)variable_ptr);
			break;
		case VALUE_SHAPE:
			success = parse_shape((shape *)variable_ptr);
			break;
		case VALUE_SINGLE_SYMBOL:
			success = parse_single_symbol((char *)variable_ptr);
			break;
		case VALUE_SIZE:
			success = parse_size((size *)variable_ptr);
			break;
		case VALUE_SPOT_LIGHT_STYLE:
			success = parse_spot_light_style((lightstyle *)variable_ptr);
			break;
		case VALUE_SPRITE_SIZE:
			success = parse_sprite_size((size *)variable_ptr);
			break;
//		case VALUE_STREAM_COMMAND:
//			success = parse_stream_command((int *)variable_ptr);
//			break;
		case VALUE_STRING:
			*(string *)variable_ptr = file_token_str;
			value_str_ptr += strlen(file_token_str);
			success = true;
			break;
		case VALUE_SYMBOL:
			success = parse_symbol((word *)variable_ptr, true);
			break;
		case VALUE_TEXTURE_STYLE:
			success = parse_texture_style((texstyle *)variable_ptr);
			break;
		case VALUE_VALIGNMENT:
			success = parse_vertical_alignment((alignment *)variable_ptr);
			break;
		case VALUE_VERSION:
			success = parse_version((unsigned int *)variable_ptr);
			break;
		case VALUE_VERTEX_COORDS:
			success = parse_vertex_coordinates((vertex_entry *)variable_ptr);
		}

		// If the value couldn't be parsed, or the unparsed portion of the
		// value string is not empty or white space, this is an error.
		// Otherwise set a flag indicating the parameter was matched.

		if (!success || !stop_parsing_value()) {
			if (param_ptr->required)
				value_error(param_ptr);
			else
				value_warning(param_ptr);
		} else
			matched_param[param_index] = true;
	}

	// If an end_token was specified, then verify that the end token parsed
	// matches it.

	if (end_token != TOKEN_NONE && file_token != end_token)
		error("Expected '%s' rather than '%s'", get_token_str(end_token),
			file_token_str);

	// If there was a parameter list, verify that all required parameters were
	// parsed.

	if (param_list) {
		for (param_index = 0; param_index < params; param_index++)
			if (param_list[param_index].required && 
				!matched_param[param_index])
				error("Parameter '%s' is missing from tag '%s'", 
					get_token_str(param_list[param_index].param_name), 
					get_token_str(tag_name));
	}
}

//------------------------------------------------------------------------------
// Parse a start tag and it's parameter list.
//------------------------------------------------------------------------------

void
parse_start_tag(token tag_name, param *param_list, int params)
{
	match_next_file_token(TOKEN_OPEN_TAG);
	match_next_file_token(tag_name);
	parse_param_list(tag_name, param_list, params, TOKEN_CLOSE_TAG);
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
	string tag_name_str;

	// Parse start and single tags until a valid tag is found.

	while (true) {

		// Update the logo on the task bar.

		update_logo();

		// If the next token is "<" or "</" then there is a tag to be parsed;
		// any other token is simply ordinary text and must be skipped.

		switch (next_file_token()) {
	
		// If this is a start or single tag, then parse it.

		case TOKEN_OPEN_TAG:

			// Read the tag name token.  If it is a string token, this is an
			// error.

			tag_name = next_file_token();
			if (tag_name == VALUE_STRING)
				error("Expected a tag name rather than the quoted string '%s'",
					file_token_str);
			tag_name_str = file_token_str;

			// If a tag list was specified, attempt to match the tag name
			// against the entries of the tag list. If a match is found, parse
			// the parameter list for that tag and then return TRUE.

			if (tag_list) {
				tag_index = 0;
				while (tag_list[tag_index].tag_name != TOKEN_NONE) {
					if (tag_name == tag_list[tag_index].tag_name) {
						parse_param_list(tag_name, 
							tag_list[tag_index].param_list,
							tag_list[tag_index].params,
							tag_list[tag_index].end_token);
						return(&tag_list[tag_index]);
					}
					tag_index++;
				}
			}

			// Otherwise treat this tag as having no parameters, and skip over
			// any parameters found in this tag.

			parse_param_list(tag_name, NULL, 0, TOKEN_NONE);

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
					error("Found end tag '%s' with no matching start tag",
						file_token_str);
			match_next_file_token(TOKEN_CLOSE_TAG);
			return(NULL);
		}
	}
	
	// Required to shut up compiler...

	return(NULL);
}