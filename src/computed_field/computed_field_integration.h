/*******************************************************************************
FILE : computed_field_integration.h

LAST MODIFIED : 26 October 2000

DESCRIPTION :
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (COMPUTED_FIELD_INTEGRATION_H)
#define COMPUTED_FIELD_INTEGRATION_H

int Computed_field_is_type_integration(struct Computed_field *field);
/*******************************************************************************
LAST MODIFIED : 26 October 2000

DESCRIPTION :
==============================================================================*/

/*****************************************************************************//**
 * Creates a field that computes an integration.
 * The seed element is set to the number given and the mapping calculated.
 * Sets the number of components to be the same as the <integrand> field.
 * The <integrand> is the value that is integrated over each element and the
 * <coordinate_field> is used to define the arc length differential for each
 * element. Currently only two gauss points are supported, a linear integration.
 * If <magnitude_coordinates> is false then the resulting field has the same
 * number of components as the <coordinate_field> and each component is the
 * integration with respect to each of the components, if <magnitude_components>
 * is true then the field will have a single component and the magnitude of the
 * <coordinate_field> derivatives are used to calculate arc lengths at each
 * gauss point.
 *
 * @param field_module  Region field module which will own new field.
 * @return Newly created field
 */
cmzn_field *cmzn_fieldmodule_create_field_integration(
	cmzn_fieldmodule *fieldmodule, cmzn_mesh_id mesh,
	cmzn_element_id seed_element, cmzn_field *integrand,
	int magnitude_coordinates, cmzn_field *coordinate_field);

/***************************************************************************//**
 * If the field is of type COMPUTED_FIELD_INTEGRATION, the arguments including
 * seed element used for the mapping are returned.
 * @param mesh_address  Pointer to mesh handle which must be null. On successful
 * return this will be an accessed handle to mesh.
 */
int Computed_field_get_type_integration(struct Computed_field *field,
	cmzn_mesh_id *mesh_address, struct FE_element **seed_element,
	struct Computed_field **integrand, int *magnitude_coordinates,
	struct Computed_field **coordinate_field);

#endif /* !defined (COMPUTED_FIELD_INTEGRATION_H) */
