/*******************************************************************************
FILE : computed_field_lookup.h

LAST MODIFIED : 10 October 2003

DESCRIPTION :
Implements computed fields for lookups.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (COMPUTED_FIELD_LOOKUP_H)
#define COMPUTED_FIELD_LOOKUP_H

#include "region/cmiss_region.hpp"

/***************************************************************************//**
 * If the field is of type COMPUTED_FIELD_LOOKUP, the function returns the
 * source field and the  lookup node where it is evaluated.
 * Note that nothing returned has been ACCESSed.
 */
int Computed_field_get_type_nodal_lookup(struct Computed_field *field,
  struct Computed_field **source_field, struct FE_node **lookup_node);

cmzn_field *cmzn_fieldmodule_create_field_quaternion_SLERP(
	cmzn_fieldmodule_id fieldmodule, cmzn_field_id source_field,
	cmzn_node_id quaternion_SLERP_node);

#endif /* !defined (COMPUTED_FIELD_LOOKUP_H) */
