/*******************************************************************************
FILE : graphics_object_private.h

LAST MODIFIED : 18 June 2004

DESCRIPTION :
Graphical object data structures.

HISTORY :
Used to be gtypes.h
7 June 1994
	Merged GTTEXT into GTPOINT and GTPOINTSET and added a marker type and a marker
	size.
4 February 1996
	Added time dependence for gtObject.
16 February 1996
	Added graphical finite element structure (code yet to be done).
24 February 1996
	Separated out user defined objects and put in userdef_object.h
4 June 1996
	Replaced gtObjectListItem by struct LIST(gtObject)
5 June 1996
	Changed gtObject to GT_object
11 January 1997
	Added pointers to the nodes in a GTPOINTSET.  This is a temporary measure to
	allow the graphical_node_editor to work (will be replaced by the graphical
	FE_node)
30 June 1997
	Added macros/functions for safer access to graphics objects. Should convert
	rest of code to use them, so that members of graphics_objects are private.
13 October 1998
	SAB Added a callback mechanism so that through GT_object_changed interested
	objects can automatically be notified of changes.
18 June 2004
   SAB Spilt GT_object out into graphics_object_private.h so that we control
   which files can see the guts.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (GRAPHICS_OBJECT_PRIVATE_H)
#define GRAPHICS_OBJECT_PRIVATE_H


#include "opencmiss/zinc/zincconfigure.h"

#include "general/cmiss_set.hpp"
#include "general/geometry.h"
#include "graphics/auxiliary_graphics_types.h"
#include "graphics/graphics_library.h"
#include "graphics/graphics_object.h"
#include "graphics/material.h"
#include "graphics/spectrum.h"
#include "graphics/graphics_object.hpp"
#include "graphics/graphics_object_highlight.hpp"
/*
Global types
------------
*/

/***************************************************************************//**
 * Provides the scene information for the lines stored in the
 * vertex_array. */
struct GT_polyline_vertex_buffers
{
	enum GT_polyline_type polyline_type;
	int line_width;
}; /* struct GT_polyline_vertex_buffers */

struct GT_surface_vertex_buffers
{
	enum GT_surface_type surface_type;
	enum cmzn_graphics_render_polygon_mode render_polygon_mode;
}; /* struct GT_polyline_vertex_buffers */

struct GT_glyphset_vertex_buffers
{
	struct cmzn_font *font;
	struct GT_object *glyph;
	Triple base_size, scale_factors, offset, label_offset;
	char *static_label_text[3];
	enum cmzn_glyph_repeat_mode glyph_repeat_mode;
	int label_bounds_dimension, label_bounds_components;
}; /* struct GT_polyline_vertex_buffers */

struct GT_pointset_vertex_buffers
{
	gtMarkerType marker_type;
	ZnReal marker_size;
	struct cmzn_font *font;
}; /* struct GT_pointset_vertex_buffers */

union GT_primitive_list
/*******************************************************************************
LAST MODIFIED : 17 March 2003

DESCRIPTION :
Storage for a single linked list of primitives of any type. The linked list
maintains a pointer to the first and last primitives so that the list can be
traversed from first to last, and additional primitives added at the end.
==============================================================================*/
{
	/* Only one object allowed for this type */
	struct GT_polyline_vertex_buffers *gt_polyline_vertex_buffers;
	struct GT_surface_vertex_buffers *gt_surface_vertex_buffers;
	struct GT_glyphset_vertex_buffers *gt_glyphset_vertex_buffers;
	struct GT_pointset_vertex_buffers *gt_pointset_vertex_buffers;
}; /* union GT_primitive_list */

struct GT_object
/*******************************************************************************
LAST MODIFIED : 17 March 2003

DESCRIPTION :
Graphical object data structure.
==============================================================================*/
{
	const char *name;
	enum GT_object_type object_type;
	/* for linked object */
	struct GT_object *nextobject;
	/* for inheritance */
	struct GT_object *parentobject;
	/* for selected primitives and subobjects */
	enum cmzn_graphics_select_mode select_mode;
	/* default attributes */
		/*???DB.  Default is a bit of a misnomer.  Here it means the unhighlighted
			colour */
	int default_colourindex;
	cmzn_material *default_material, *secondary_material, 
		*selected_material;
	/* spectrum */
	struct cmzn_spectrum *spectrum;
	/* the object can vary with time.  It is specified at <number_of_times>
		<times>.  The <times> are in increasing order.  The object varies linearly
		between <times> and is constant before the first and after the last */
	int number_of_times;
	ZnReal *times;
	double render_line_width;
	double render_point_size;

	Graphics_vertex_array *vertex_array;

	/* If the graphics object was compiled with respect to a texture
		tiling then this pointer is set to that tiling. */
	struct Texture_tiling *texture_tiling;

	union GT_primitive_list *primitive_lists;
#if defined (OPENGL_API)
	int buffer_binding;
	GLuint display_list;

	GLuint vertex_array_object;

	GLuint position_vertex_buffer_object;
	GLuint position_values_per_vertex;
	GLuint colour_vertex_buffer_object;
	GLuint colour_values_per_vertex;
	GLuint normal_vertex_buffer_object;
	GLuint texture_coordinate0_vertex_buffer_object;
	GLuint texture_coordinate0_values_per_vertex;
	/* tangent buffer will be passed in as coordinates of texture unit 1*/
	GLuint tangent_vertex_buffer_object;
	GLuint tangent_values_per_vertex;
	GLuint index_vertex_buffer_object;
	/* For multipass rendering we use some more vertex_buffers
	 * a framebuffer and a texture. */
	unsigned int multipass_width;
	unsigned int multipass_height;
	GLuint multipass_vertex_buffer_object;
	GLuint multipass_frame_buffer_object;
	GLuint multipass_frame_buffer_texture;
#endif /* defined (OPENGL_API) */
	/* enumeration indicates whether the graphics display list is up to date */
	enum Graphics_compile_status compile_status;

	/* Custom per compile code for graphics_objects used as glyphs. */
	Graphics_object_glyph_labels_function glyph_labels_function;
	/* identifier for quickly matching standard point, line, cross glyphs */
	enum cmzn_glyph_shape_type glyph_type;

	int access_count;
};

class Render_graphics_compile_members;

int Graphics_object_compile_members(GT_object *graphics_object_list,
	Render_graphics_compile_members *renderer);

#endif /* ! defined (GRAPHICS_OBJECT_PRIVATE_H) */
