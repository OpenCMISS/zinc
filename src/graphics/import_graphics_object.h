/*******************************************************************************
FILE : import_graphics_object.h

LAST MODIFIED : 6 December 2004

DESCRIPTION :
Function prototype for reading graphics object data from a file.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (IMPORT_GRAPHICS_OBJECT_H)
#define IMPORT_GRAPHICS_OBJECT_H

struct IO_stream_package;
struct cmzn_materialmodule;
struct cmzn_glyphmodule;

/*
Global functions
----------------
*/

int file_read_graphics_objects(char *file_name,
	struct IO_stream_package *io_stream_package,
	struct cmzn_materialmodule *materialmodule,
	struct cmzn_glyphmodule *glyphmodule);

// Note the time argument is not used since Cmgui 3.1
// We may add future support perhaps by having a linked list of graphics objects
// each with their own time
int file_read_surface_graphics_object_from_obj(char *file_name,
	struct IO_stream_package *io_stream_package,
	char *graphics_object_name, enum cmzn_graphics_render_polygon_mode render_polygon_mode,
	ZnReal time, struct cmzn_materialmodule *materialmodule,
	struct cmzn_glyphmodule *glyphmodule);

#endif
