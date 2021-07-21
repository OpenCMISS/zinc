/*******************************************************************************
FILE : snake.h

LAST MODIFIED : 29 March 2006

DESCRIPTION :
Functions for making a snake of 1-D cubic Hermite elements from a chain of
data points.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (SNAKE_H)
#define SNAKE_H

#include "opencmiss/zinc/element.h"
#include "opencmiss/zinc/node.h"

int create_FE_element_snake_from_data_points(
	struct FE_region *fe_region, struct Computed_field *coordinate_field,
	struct Computed_field *weight_field,
	int number_of_fitting_fields, struct Computed_field **fitting_fields,
	cmzn_nodeset_id data_nodeset,
	int number_of_elements, FE_value density_factor, FE_value stiffness,
	cmzn_nodeset_group_id nodeset_group, cmzn_mesh_group_id mesh_group);
/*******************************************************************************
LAST MODIFIED : 29 March 2006

DESCRIPTION :
Creates a snake out of <number_of_elements> 1-D cubic Hermite elements in
<element_manager> and <element_group>, nodes in <node_manager> and the node
group of the same name in <node_group_manager>. The snake follows the data in
<data_nodeset>; data is unmodified by this function.
The <fitting_fields> which must be defined on the data are fitted and
defined on the created elements.  The <coordinate_field> is used to determine
distances between points.  If specified, the <weight_field> is evaluated at 
each data point and used to weight the varying importance of that data point.
The <density_factor> can vary from 0 to 1.
When <density_factor> is 0 then the elements are spread out to have the same
length in this coordinate field, when the <density_factor> is 1 then the
elements will each correspond to an equal proportion of the data points.
A positive value of <stiffness> penalises solutions with large second
derivatives; helps make smooth snakes from few data points.
@param nodeset_group  Optional nodeset group to put snake nodes in.
@param mesh_group  Optional mesh group to put snake elements in.
==============================================================================*/

#endif /* !defined (SNAKE_H) */
