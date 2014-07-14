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

// Style object.

extern style *style_ptr;

// Flag indicating if blockset has been changed.

extern bool blockset_changed;

// Directory and file paths.

extern char blocksets_dir[_MAX_PATH];
extern char blocks_dir[_MAX_PATH];
extern char textures_dir[_MAX_PATH];
extern char sounds_dir[_MAX_PATH];
extern string temp_dir;
extern string temp_blocks_dir;
extern string temp_textures_dir;
extern string temp_sounds_dir;
extern char blockset_file_path[_MAX_PATH];

// Externally visible functions.

void
diagnose(char *format, ...);

void
message(char *title, char *format, ...);

block_def *
add_block_to_style(char block_symbol, char *block_double_symbol,
				   char *block_file_name, block *block_ptr);

void
write_directory(char *source_dir, char *target_dir);

