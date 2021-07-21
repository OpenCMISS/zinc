/*******************************************************************************
FILE : computed_field_finite_element.h

LAST MODIFIED : 25 May 2006

DESCRIPTION :
Implements computed fields which interface to finite element fields.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (COMPUTED_FIELD_FINITE_ELEMENT_H)
#define COMPUTED_FIELD_FINITE_ELEMENT_H

#include "opencmiss/zinc/types/fieldfiniteelementid.h"
#include "computed_field/computed_field.h"
#include "general/enumerator.h"
#include "general/list.h"

/*
Global functions
----------------
*/

/**
 * internal-only constructor for creating wrapper field for an FE_field
 * Call only from cmzn_region::FeRegionChange
 */
cmzn_field *cmzn_fieldmodule_create_field_finite_element_wrapper(
	cmzn_fieldmodule *fieldmodule, struct FE_field *fe_field);

int Computed_field_is_type_finite_element(struct Computed_field *field);
/*******************************************************************************
LAST MODIFIED : 18 July 2000

DESCRIPTION :
==============================================================================*/

int Computed_field_is_type_finite_element_iterator(
	struct Computed_field *field, void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 16 March 2007

DESCRIPTION :
Iterator/conditional function returning true if <field> is read only and a
wrapper for an FE_field.
==============================================================================*/

/**
 * If the field is of type COMPUTED_FIELD_FINITE_ELEMENT, the FE_field being
 * wrapped by it is returned, or 0 with no errors if not of correct type.
 * The returned fe_field is NOT accessed.
 */
int Computed_field_get_type_finite_element(struct Computed_field *field,
	struct FE_field **fe_field);

/*******************************************************************************
 * Iterator/conditional function returning true if <field> is the finite_element
 * computed field wrapper for the FE_field.
 * @param fe_field_void  Void pointer to a struct FE_field.
 */
int Computed_field_wraps_fe_field(struct Computed_field *field,
	void *fe_field_void);

struct LIST(FE_field)
	*Computed_field_get_defining_FE_field_list(struct Computed_field *field);
/*******************************************************************************
LAST MODIFIED : 2 April 2003

DESCRIPTION :
Returns the list of FE_fields that <field> depends on.
==============================================================================*/

struct LIST(FE_field)
	*Computed_field_array_get_defining_FE_field_list(
		int number_of_fields, struct Computed_field **field_array);
/*******************************************************************************
LAST MODIFIED : 5 April 2006

DESCRIPTION :
Returns the compiled list of FE_fields that are required by any of
the <number_of_fields> fields in <field_array>.
==============================================================================*/

int Computed_field_is_type_cmiss_number(struct Computed_field *field);
/*******************************************************************************
LAST MODIFIED : 18 July 2000

DESCRIPTION :
==============================================================================*/

/*****************************************************************************//**
 * Creates a cmiss_number type fields whose value is the number of the element
 * or node location.
 *
 * @param field_module  Region field module which will own new field.
 * @return Newly created field
 */
cmzn_field *cmzn_fieldmodule_create_field_cmiss_number(
	cmzn_fieldmodule *fieldmodule);

int Computed_field_has_coordinate_fe_field(struct Computed_field *field,
	void *dummy);
/*******************************************************************************
LAST MODIFIED : 4 December 2001

DESCRIPTION :
Iterator/conditional function returning true if <field> is a wrapper for a
coordinate type fe_field.
==============================================================================*/

int Computed_field_has_element_xi_fe_field(struct Computed_field *field,
	void *dummy);
/*******************************************************************************
LAST MODIFIED : 2 June 2006

DESCRIPTION :
Iterator/conditional function returning true if <field> is a wrapper for an
element_xi type fe_field.
==============================================================================*/

int Computed_field_is_scalar_integer(struct Computed_field *field,
	void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 20 June 2000

DESCRIPTION :
Returns true if <field> is a 1 integer component FINITE_ELEMENT wrapper.
==============================================================================*/

/**
 * Returns true if field is a 1 component integer finite element field which is
 * defined grid-based in element.
 * Used for choosing field suitable for identifying grid points.
 */
int Computed_field_is_scalar_integer_grid_in_element(
	cmzn_field_id field, void *element_void);

int Computed_field_is_type_xi_coordinates(struct Computed_field *field,
	void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 15 January 2002

DESCRIPTION :
==============================================================================*/

/*****************************************************************************//**
 * Creates a field whose return values are the xi coordinate location.
 * Currently fixed at 3 components, padding with zeros for lower dimension
 * elements.
 *
 * @param field_module  Region field module which will own new field.
 * @return Newly created field
 */
cmzn_field *cmzn_fieldmodule_create_field_xi_coordinates(
	cmzn_fieldmodule *fieldmodule);

struct FE_time_sequence *Computed_field_get_FE_node_field_FE_time_sequence(
	 struct Computed_field *computed_field, struct FE_node *node);

PROTOTYPE_ENUMERATOR_FUNCTIONS(cmzn_field_edge_discontinuity_measure);

enum cmzn_element_face_type cmzn_field_is_on_face_get_face_type(cmzn_field_id field);

/** If field is node_value type, return the value label, otherwise INVALID */
cmzn_node_value_label cmzn_field_node_value_get_value_label(cmzn_field_id field);

/** If field is node_value type, return the version_number >= 1, otherwise 0 */
int cmzn_field_node_value_get_version_number(cmzn_field_id field);

enum cmzn_field_edge_discontinuity_measure
cmzn_field_edge_discontinuity_measure_enum_from_string(const char *string);

char *cmzn_field_edge_discontinuity_measure_enum_to_string(
	enum cmzn_field_edge_discontinuity_measure measure);

/** @return  Host mesh for find or stored mesh location fields. For time being must check if none. */
FE_mesh *cmzn_field_get_host_FE_mesh(cmzn_field_id field);

/** Discover and set destination host mesh from source field, or check it matches if already set.
  * @return  Result OK on success, any other value on failure. */
int cmzn_field_discover_element_xi_host_mesh_from_source(cmzn_field *destination_field, cmzn_field * source_field);

class FE_element_field_evaluation;

/** Special function used only by Computed_field_core::evaluateDerivativeFiniteDifference.
 * Get internal finite element field evaluation object for finite element field,
 * creating if needed, in fieldcache at element location contained in it.
 * Used to apply parameter perturbation to for calculating parameter derivatives.
 * @param field  Field, must be of finite element type with real components.
 * @param fieldcache  Field cache in which to find or create object & containing element location.
 * @return  Pointer to object or nullptr if failed. */
FE_element_field_evaluation *cmzn_field_get_cache_FE_element_field_evaluation(cmzn_field *field, cmzn_fieldcache *fieldcache);

#endif /* !defined (COMPUTED_FIELD_FINITE_ELEMENT_H) */
