/*******************************************************************************
FILE : finite_element_to_iso_lines.h

LAST MODIFIED : 28 January 2000

DESCRIPTION :
Functions for computing, sorting and storing polylines of constant scalar field
value over 2-D elements.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (FINITE_ELEMENT_TO_ISO_LINES_H)
#define FINITE_ELEMENT_TO_ISO_LINES_H

#include "computed_field/computed_field.h"
#include "finite_element/finite_element.h"
#include "graphics/auxiliary_graphics_types.h"
#include "graphics/graphics_object.h"

/**
 * Fills <graphics_object> (of type g_POLYLINE_VERTEX_BUFFERS) with polyline contours of
 * <isoscalar_field> at <iso_value>.
 * @param field_cache  cmzn_fieldcache for evaluating fields with. Time is
 * expected to have been set in the field_cache if needed.
 */
int create_iso_lines_from_FE_element(struct FE_element *element,
	cmzn_fieldcache_id field_cache, struct Computed_field *coordinate_field,
	struct Computed_field *isoscalar_field, FE_value iso_value,
	struct Computed_field *data_field,int number_of_segments_in_xi1_requested,
	int number_of_segments_in_xi2_requested,struct FE_element *top_level_element,
	struct Graphics_vertex_array *array);

#endif /* !defined (FINITE_ELEMENT_TO_ISO_LINES_H) */
