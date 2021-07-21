/*******************************************************************************
FILE : computed_field_update.h

LAST MODIFIED : 7 January 2003

DESCRIPTION :
Functions for updating values of one computed field from those of another.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (COMPUTED_FIELD_UPDATE_H)
#define COMPUTED_FIELD_UPDATE_H

#include "finite_element/finite_element.h"
#include "region/cmiss_region.hpp"
#include "selection/element_point_ranges_selection.h"
#include "general/message.h"

/*
Global types
------------
*/

/*******************************************************************************
 * Assign values of source field to destination field for nodes in nodeset at
 * the given time. Ignores nodes where either source or destination field is
 * undefined. Optional conditional field allows conditional assignment.
 *
 * @param nodeset  The nodeset to assign in.
 * @param destination_field  The field to assign values to.
 * @param source_field  The field to evaluate values from.
 * @param conditional_field  If supplied, only assigns to nodes for which this
 * field evaluates to true. If NULL, assigns to all nodes in nodeset.
 * @param time  The time to assign values at
 * @return  Result OK on success, WARNING_PART_DONE if only some values assigned,
 * otherwise any other error.
 */
int cmzn_nodeset_assign_field_from_source(
	cmzn_nodeset_id nodeset, cmzn_field_id destination_field,
	cmzn_field_id source_field, cmzn_field_id conditional_field,
	FE_value time);

/*******************************************************************************
 * Assign values of source field to grid-based destination field for elements in
 * mesh at the given time. Ignores elements where either source or destination
 * field is undefined. Optional conditional field allows conditional assignment.
 *
 * @param mesh  The mesh to assign in.
 * @param destination_field  The field to assign values to.
 * @param source_field  The field to evaluate values from.
 * @param conditional_field  If supplied, does not assign to grid points for
 * which this field evaluates to false.
 * @param element_point_ranges_selection  If supplied, does not assign to grid
 * points which are not in this selection.
 * @param time  The time to assign values at
 * @return  1 on success, 0 on error.
 */
int cmzn_mesh_assign_grid_field_from_source(
	cmzn_mesh_id mesh, cmzn_field_id destination_field,
	cmzn_field_id source_field, cmzn_field_id conditional_field,
	struct Element_point_ranges_selection *element_point_ranges_selection,
	FE_value time);

#endif /* !defined (COMPUTED_FIELD_UPDATE_H) */
