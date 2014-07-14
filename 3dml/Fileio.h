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

// Externally visible functions.

void 
parse_spot_file(char *spot_URL, char *spot_file_path);

void
delete_cached_blockset_list(void);

cached_blockset *
new_cached_blockset(const char *path, const char *href, int size, int updated);

bool
load_cached_blockset_list(void);

void
save_cached_blockset_list(void);

void
create_cached_blockset_list(void);

cached_blockset *
find_cached_blockset(const char *href);

bool
delete_cached_blockset(const char *href);

bool
check_for_blockset_update(const char *version_file_URL, const char *blockset_name,
						  unsigned int blockset_version);

bool
parse_rover_version_file(unsigned int &version_number, string &message);