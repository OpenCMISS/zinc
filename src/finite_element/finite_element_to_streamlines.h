/*******************************************************************************
FILE : finite_element_to_streamlines.h

LAST MODIFIED : 14 January 2003

DESCRIPTION :
Functions for calculating streamlines in finite elements.
???DB.  Put into finite_element_to_graphics_object or split
	finite_element_to_graphics_object further ?
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (FINITE_ELEMENT_TO_STREAMLINES_H)
#define FINITE_ELEMENT_TO_STREAMLINES_H
#include "finite_element/finite_element.h"
#include "general/list.h"
#include "general/manager.h"
#include "general/object.h"

/*
Global types
----------------
*/

struct Streampoint;
/*******************************************************************************
LAST MODIFIED : 11 November 1997

DESCRIPTION :
Streampoint type is private.
==============================================================================*/

struct Element_to_particle_data
/*******************************************************************************
LAST MODIFIED : 17 March 1999

DESCRIPTION :
Data for converting a 3-D element into an array of particles.
==============================================================================*/
{
	FE_value xi[3];
	int element_number;
	int number_of_particles;
	struct GT_object *graphics_object;
	struct Streampoint **list;
	cmzn_fieldcache_id field_cache;
	struct Computed_field *coordinate_field,*stream_vector_field;
	int index;
	Triple **pointlist;
}; /* struct Element_to_particle_data */

/*
Global functions
----------------
*/


/**
 * Creates a <GT_polyline_vertex_buffers> streamline from the <coordinate_field> tracking
 * <stream_vector_field> (with 3, 6 or 9 components) starting in the element and at
 * the xi coordinates supplied. If <reverse_track> is true, the reverse of the
 * stream vector is tracked, and the travel_scalar is made negative.
 * @param field_cache  cmzn_fieldcache for evaluating fields with. Time is
 * expected to have been set in the field_cache if needed.
 */
int create_polyline_streamline_FE_element_vertex_array(
	struct FE_element *element,FE_value *start_xi,
	cmzn_fieldcache_id field_cache, struct Computed_field *coordinate_field,
	struct Computed_field *stream_vector_field,int reverse_track,
	FE_value length, enum cmzn_graphics_streamlines_colour_data_type colour_data_type,
	struct Computed_field *data_field,
	struct Graphics_vertex_array *array);

/**
 * Fills the array with coordinates of the streamline from the <coordinate_field> following
 * <stream_vector_field> (with 3, 6 or 9 components) starting in the element and at
 * the xi coordinates supplied. If <reverse_track> is true, the reverse of the
 * stream vector is tracked, and the travel_scalar is made negative.
 * @param field_cache  cmzn_fieldcache for evaluating fields with. Time is
 * expected to have been set in the field_cache if needed.
 * @param line_shape  LINE, RIBBON, CIRCLE_EXTRUSION or SQUARE_EXTRUSION.
 * @param line_base_size  width and thickness of line, use depends on shape.
 * @param line_scale_factors  Ignored. For future use.
 * @param line_orientation_scale_field  Ignored. For future use.
 */
int create_surface_streamribbon_FE_element_vertex_array(
	struct FE_element *element,FE_value *start_xi,
	cmzn_fieldcache_id field_cache, struct Computed_field *coordinate_field,
	struct Computed_field *stream_vector_field,int reverse_track, FE_value length,
	enum cmzn_graphicslineattributes_shape_type line_shape, int circleDivisions,
	FE_value *line_base_size, FE_value *line_scale_factors,
	struct Computed_field *line_orientation_scale_field,
	enum cmzn_graphics_streamlines_colour_data_type colour_data_type, struct Computed_field *data_field,
	struct Graphics_vertex_array *array);

int add_flow_particle(struct Streampoint **list,FE_value *xi,
	struct FE_element *element,Triple **pointlist,int index,
	cmzn_fieldcache_id field_cache, struct Computed_field *coordinate_field,
	gtObject *graphics_object);
/*******************************************************************************
LAST MODIFIED : 17 March 1999

DESCRIPTION :
Adds a new flow particle structure to the start of the Streampoint list
==============================================================================*/

int element_to_particle(struct FE_element *element,
	void *void_element_to_particle_data);
/*******************************************************************************
LAST MODIFIED : 6 January 1998

DESCRIPTION :
Converts a 3-D element into an array of particles.
==============================================================================*/

#endif /* !defined (FINITE_ELEMENT_TO_STREAMLINES_H) */
