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

#include "tokens.h"

// File stack element class.

struct file_stack_element {
	bool spot_file;
	long file_size;
	long file_position;
	char *file_buffer;
	char *file_buffer_ptr;
};

// Pointer to top file stack element.

extern file_stack_element *top_file_ptr;

// Maximum number of parameters expected in a tag.

#define MAX_PARAMS	16

// Parameter class.

struct param {
	token param_name;
	token value_type;
	void *variable_ptr;
	bool required;
};

// Tag class.

struct tag {
	token tag_name;
	param *param_list;
	int params;
	token end_token;
};

// Information about last start or single tag parsed.

extern bool start_tag;
extern token tag_name;
extern int params_parsed;
extern bool matched_param[MAX_PARAMS];

// Current line buffer.

extern string line_buffer;

// Initialisation function.

void
init_parser(void);

// URL and conversion functions.

void
split_URL(const char *URL, string *URL_dir, string *file_name,
		  string *entrance_name);

string
create_URL(const char *URL_dir, const char *sub_dir, const char *file_name);

string
encode_URL(const char *URL);

string
decode_URL(const char *URL);

string
URL_to_file_path(const char *URL);

void
parse_identifier(const char *identifier, string &style_name, 
				 string &object_name);

bool
not_single_symbol(char ch, bool disallow_dot);

bool
not_double_symbol(char ch1, char ch2, bool disallow_dot_dot);

bool
string_to_single_symbol(const char *string_ptr, char *symbol_ptr,
						bool disallow_dot);

bool
string_to_double_symbol(const char *string_ptr, word *symbol_ptr,
						bool disallow_dot_dot);

char *
version_number_to_string(unsigned int version_number);

// File functions.

bool
open_ZIP_archive(const char *file_path);

bool
open_blockset(const char *blockset_URL, const char *blockset_name);

void
close_ZIP_archive(void);

bool
push_file(const char *file_path, bool spot_file);

bool
push_ZIP_file(const char *file_path);

bool
push_ZIP_file_with_ext(const char *file_ext);

bool
push_buffer(const char *buffer_ptr, int buffer_size);

void
rewind_file(void);

void
pop_file(void);

void
pop_all_files(void);

int
read_file(byte *buffer_ptr, int bytes);

bool
copy_file(const char *file_path, bool text_file);

void
read_line(void);

// Error checking and reporting functions.

void
write_error_log(const char *message);

void 
diagnose(const char *format, ...);

void
diagnose(int tabs, const char *format, ...);

void
warning(const char *format, ...);

void
memory_warning(const char *object);

void
error(const char *format, ...);

void
memory_error(const char *object);

bool
check_int_range(int value, int min, int max);

bool
check_float_range(float value, float min, float max);

// Name parsing function.

bool
parse_name(const char *old_name, string *new_name, bool list);

// Parameter value parsing functions.

void
start_parsing_value(char *value_str);

bool
parse_integer_in_value(int *int_ptr);

bool
parse_float_in_value(float *float_ptr);

bool
token_in_value_is(const char *token_str);

bool
stop_parsing_value(void);

// Parameter list and tag parsing functions.

void
parse_start_tag(token tag, param *param_list, int params);

void
parse_end_tag(token tag);

tag *
parse_next_tag(tag *tag_list, token end_tag_name,
			   char *end_tag_name_str = NULL);