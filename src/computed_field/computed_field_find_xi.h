/*******************************************************************************
FILE : computed_field_find_xi.h

LAST MODIFIED : 18 April 2005

DESCRIPTION :
Implements algorithm to find the element and xi location at which a field
has a particular value.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (COMPUTED_FIELD_FIND_XI_H)
#define COMPUTED_FIELD_FIND_XI_H

#include "region/cmiss_region.hpp"

struct Graphics_buffer_package;

struct Computed_field_find_element_xi_cache;
/*******************************************************************************
LAST MODIFIED : 4 July 2000

DESCRIPTION :
struct Computed_field_find_element_xi_cache is private.
==============================================================================*/

/***************************************************************************//**
 * Find location in mesh or element where the field has same or nearest value to
 * the prescribed values. This routine is either called directly by
 * Computed_field_find_element_xi or if that field is propagating it's values
 * backwards, it is called by the last ancestor field implementing
 * propagate_find_element_xi.
 *
 * @param field  The field whose values need to match.
 * @param field_cache  cmzn_fieldcache for evaluating fields with. Time is
 * expected to have been set in the field_cache if needed.
 * @param values  Array of values to match or get nearest to. Implementation
 * promises to copy this, hence can pass a pointer to field cache values.
 * @param number_of_values  The size of the values array, must equal the number
 * of components of field.
 * @param element_address  Address to return element in. If mesh is omitted,
 * must point at a single element to search.
 * @param xi  Array of same dimension as mesh or element to return chart
 * coordinates in.
 * @param mesh  The mesh to search over. Can be omitted if element specified.
 * @param find_nearest  Set to 1 to find location of nearest field value, or 0
 * to find exact match.
 * @return  1 if search carried out without error including when no element is
 * found, or 0 if failed.
 */
int Computed_field_perform_find_element_xi(struct Computed_field *field,
	cmzn_fieldcache_id field_cache,
	const FE_value *values, int number_of_values,
	struct FE_element **element_address, FE_value *xi,
	cmzn_mesh_id search_mesh, int find_nearest);

int DESTROY(Computed_field_find_element_xi_cache)
	  (struct Computed_field_find_element_xi_cache **cache_address);
/*******************************************************************************
LAST MODIFIED : 20 June 2000

DESCRIPTION :
Frees memory/deaccess cache at <*cache_address>.
==============================================================================*/

#endif /* !defined (COMPUTED_FIELD_FIND_XI_H) */
