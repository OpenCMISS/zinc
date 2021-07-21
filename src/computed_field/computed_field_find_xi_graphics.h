/*******************************************************************************
FILE : computed_field_find_xi_graphics.h

LAST MODIFIED : 12 June 2008

DESCRIPTION :
Implements a special version of find_xi that uses OpenGL to accelerate the
lookup of the element.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (COMPUTED_FIELD_FIND_XI_GRAPHICS_H)
#define COMPUTED_FIELD_FIND_XI_GRAPHICS_H

#include "computed_field/computed_field_find_xi.h"
#include "region/cmiss_region.hpp"

struct Graphics_buffer_package;

/***************************************************************************//**
 * This function implements the reverse of some certain computed_fields
 * (Computed_field_is_find_element_xi_capable) so that it tries to find an
 * element and xi which would evaluate to the given values.
 * This implementation of find_element_xi has been separated out as it uses
 * OpenGL to accelerate the element xi lookup.
 * The <graphics_buffer_package> is required to connect to the OpenGL
 * implementation. The <find_element_xi_data> is passed in just to avoid
 * reimplementing the code from Computed_field_find_element_xi.
 * <hint_minimums> and <hint_maximums> are used to indicate the range over which
 * the values supplied will vary and <hint_resolution> indicates the resolution
 * at which values will be sampled for element_xi, as this algorithm will
 * generate an element lookup image using these parameters.
 * An <element_dimension> of 0 searches in elements of all dimension, any other
 * value searches just elements of that dimension.
 * The return code indicates if the algorithm should be relied on or whether a
 * sequential element_xi lookup should now be performed.
 * @param field_cache  cmzn_fieldcache for evaluating fields with. Time is
 * expected to have been set in the field_cache if needed.
 */
int Computed_field_find_element_xi_special(struct Computed_field *field,
	cmzn_fieldcache_id field_cache,
	struct Computed_field_find_element_xi_cache **cache_ptr,
	const FE_value *values, int number_of_values, struct FE_element **element,
	FE_value *xi, cmzn_mesh_id search_mesh,
	Graphics_buffer_package *graphics_buffer_package,
	ZnReal *hint_minimums, ZnReal *hint_maximums, ZnReal *hint_resolution);

#endif /* !defined (COMPUTED_FIELD_FIND_XI_GRAPHICS_H) */
