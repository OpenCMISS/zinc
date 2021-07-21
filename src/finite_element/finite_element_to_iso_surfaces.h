/***************************************************************************//**
 * FILE : finite_element_to_iso_surfaces.h
 * 
 * Functions for creating graphical iso-surfaces from finite element fields.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (FINITE_ELEMENT_TO_ISO_SURFACES_H)
#define FINITE_ELEMENT_TO_ISO_SURFACES_H

#include "graphics/graphics_object.h"

struct Iso_surface_specification;

/***************************************************************************//**
 * Creates sharable specification of iso-surfaces are to be generated.
 * 
 * @param number_of_iso_values  Number of iso-values to create surfaces from.
 * @param iso_values  If non-NULL lists the iso_values in create order. If NULL,
 * space iso-values regularly from first_iso_value to last_iso_value.
 * @param first_iso_value  First iso-value if iso_values not supplied.
 * @param last_iso_value  Last iso-value if iso_values not supplied.
 */
struct Iso_surface_specification *Iso_surface_specification_create(
	int number_of_iso_values, const double *iso_values,
	double first_iso_value, double last_iso_value,
	struct Computed_field *coordinate_field, struct Computed_field *data_field,
	struct Computed_field *scalar_field,
	struct Computed_field *texture_coordinate_field);

/***************************************************************************//**
 * Clean up iso-surface specification object. Clears pointer to object.
 * 
 * @param specification_address  Address where pointer to specification held.
 * @return  Non-zero on success.
 */
int Iso_surface_specification_destroy(
	struct Iso_surface_specification **specification_address);

/***************************************************************************//**
 * Converts a 3-D element into an iso_surface as a GT_surface_vertex_buffer
 */
int create_iso_surfaces_from_FE_element(struct FE_element *element,
	cmzn_fieldcache_id field_cache, cmzn_mesh_id mesh,
	struct Graphics_vertex_array *array,
	int *number_in_xi, struct Iso_surface_specification *specification);

#endif /* !defined (FINITE_ELEMENT_TO_ISO_SURFACES_H) */
