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

#include "tokens.h"

// Maximum number of parameters expected in a tag.

#define MAX_PARAMS	16

// Parameter class.

struct param {
	token param_name;
	token value_type;
	void *variable_ptr;
	bool required;
	bool alternative;
};

// Tag class.

struct tag {
	token tag_name;
	param *param_list;
	token end_token;
};

// Information about last start or single tag parsed.

extern bool start_tag;
extern token tag_name;
extern int params_parsed;
extern bool matched_param[MAX_PARAMS];

// Last buffer token parsed, and it's type.

extern char file_token_str[BUFSIZ];
extern token file_token;

// Initialisation function.

void
init_parser(void);

// File functions.

bool
open_ZIP_archive(char *file_path);

bool
create_ZIP_archive(char *file_path);

void
close_ZIP_archive(void);

bool
push_file(char *file_path);

bool
push_ZIP_file(char *file_path);

bool
write_ZIP_file(char *file_path);

void
pop_file(void);

void
pop_all_files(void);

void
read_next_line(void);

// Error checking and reporting functions.

void
file_error(char *format, ...);

void
memory_error(char *object);

void
check_int_range(char *message, int value, int min, int max);

void
check_double_range(char *message, double value, double min, double max);

// Basic parsing functions.

void
read_int(int *int_ptr);

void
verify_int(int value);

void
read_double(double *double_ptr);

void
read_name(char *name);

bool
line_is(char *expected_line);

void
verify_line(char *expected_line);

bool
parse_name(char *old_ident, char *new_ident, int is_expression);

void
strip_cr(char *string_to_strip);

// Parameter list and tag parsing functions.

void
parse_param_list(token tag_name, param *param_list, token end_token);

void
parse_start_tag(token tag, param *param_list);

void
parse_end_tag(token tag);

tag *
parse_next_tag(tag *tag_list, token end_tag_name,
			   char *end_tag_name_str = NULL);