/*******************************************************************************
FILE : node_operations.h

LAST MODIFIED : 3 March 2003

DESCRIPTION :
FE_node functions that utilise non finite element data structures and therefore
cannot reside in finite element modules.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (NODE_OPERATIONS_H)
#define NODE_OPERATIONS_H

#include "computed_field/computed_field.h"
#include "finite_element/finite_element.h"
#include "general/multi_range.h"

/*
Global functions
----------------
*/

/**
 * Changes the identifiers of all nodes in nodeset.
 * If <sort_by_field> is NULL, adds <node_offset> to the identifiers.
 * If <sort_by_field> is specified, it is evaluated for all nodes
 * in nodeset and they are sorted by it - changing fastest with the first
 * component and keeping the current order where the field has the same values.
 * Checks for and fails if attempting to give any of the nodes an
 * identifier already used by a node in the same master nodeset.
 * Note function avoids iterating through nodeset's own list as this is not
 * allowed during identifier changes.
 */
int cmzn_nodeset_change_node_identifiers(cmzn_nodeset_id nodeset,
	int node_offset, cmzn_field_id sort_by_field, FE_value time);

/**
* @return  A conditional field returning 1 (true) for all node identifiers
* of mesh in the given ranges. Returned field is accessed. Returns 0 on error.
*/
cmzn_field_id cmzn_nodeset_create_conditional_field_from_identifier_ranges(
	cmzn_nodeset_id nodeset, struct Multi_range *identifierRanges);

/**
* @param time  If other conditional fields are time-varying the result is
* wrapped in a time_lookup field for this time to ensure correct evaluation.
* @return  A conditional field that is the logical AND of a node group
* field formed from the optional identifier ranges with any of the 3 supplied
* conditional fields. Field returns true if no ranges or conditionals supplied.
* Returned field is accessed. Returns 0 on error.
*/
cmzn_field_id cmzn_nodeset_create_conditional_field_from_ranges_and_selection(
	cmzn_nodeset_id nodeset, struct Multi_range *identifierRanges,
	cmzn_field_id conditionalField1, cmzn_field_id conditionalField2,
	cmzn_field_id conditionalField3, FE_value time);

#endif /* !defined (NODE_OPERATIONS_H) */
