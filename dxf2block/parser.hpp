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

// Line buffer.

extern char line[BUFSIZ];

// Initialisation function.

void
init_parser(void);

// File functions.

bool
open_file(char *file_path, char *flags);

void
close_file(void);

void
write_file(char *format, ...);

// Error checking and reporting functions.

void
file_error(char *format, ...);

void
memory_error(char *object);

// Parsing functions.

bool
read_next_line(void);

bool
line_is(char *expected_line);

bool
verify_line(char *expected_line);

bool
read_int(int *int_ptr);

bool
read_double(double *double_ptr);
