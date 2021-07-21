/**
 * @file fieldnodesetoperators.h
 *
 * Implements field operators that sum or process fields over a nodeset.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef CMZN_FIELDNODESETOPERATORS_H__
#define CMZN_FIELDNODESETOPERATORS_H__

#include "types/fieldid.h"
#include "types/fieldmoduleid.h"
#include "types/fieldnodesetoperatorsid.h"
#include "types/nodesetid.h"

#include "opencmiss/zinc/zincsharedobject.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * If the field is a nodeset operator type field, return the derived field
 * handle. This is a common base class for nodeset sum, mean, minimum, maximum
 * and other operators.
 *
 * @param field  The field to be cast.
 * @return  Handle to derived nodeset operator field, or NULL/invalid handle if
 * wrong type or failed.
 */
ZINC_API cmzn_field_nodeset_operator_id cmzn_field_cast_nodeset_operator(
	cmzn_field_id field);

/**
 * Cast nodeset operator field back to its base field and return the field.
 * IMPORTANT NOTE: Returned field does not have incremented reference count and
 * must not be destroyed. Use cmzn_field_access() to add a reference if
 * maintaining returned handle beyond the lifetime of the derived field.
 * Use this function to call base-class API, e.g.:
 * cmzn_field_set_name(cmzn_field_derived_base_cast(derived_field), "bob");
 *
 * @param nodeset_operator_field  Handle to the nodeset operator field to cast.
 * @return  Non-accessed handle to the base field or NULL if failed.
 */
ZINC_C_INLINE cmzn_field_id cmzn_field_nodeset_operator_base_cast(
	cmzn_field_nodeset_operator_id nodeset_operator_field)
{
	return (cmzn_field_id)(nodeset_operator_field);
}

/**
 * Destroys handle to the nodeset operator field (and sets it to NULL).
 * Internally this decrements the reference count.
 *
 * @param nodeset_operator_field_address  Address of handle to the field to
 * destroy.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
ZINC_API int cmzn_field_nodeset_operator_destroy(
	cmzn_field_nodeset_operator_id *nodeset_operator_field_address);

/**
 * Get field mapping from nodes to elements, if any, which if valid means
 * nodeset operator is evaluated per element at element/mesh locations, but
 * over the whole nodeset at any other location.
 *
 * @param nodeset_operator_field  Nodeset operator field to query.
 * @return  Handle to element map field if nodeset operator is in element mode,
 * otherwise null/invalid handle.
 */
ZINC_API cmzn_field_id cmzn_field_nodeset_operator_get_element_map_field(
	cmzn_field_nodeset_operator_id nodeset_operator_field);

/**
 * Set or unset field mapping from nodes to elements. If a valid element map
 * field is set, then when evaluating the field at an element/mesh location,
 * the operator is evaluated in that element, applying only to nodes mapped
 * into it. A reverse element->node map is maintained internally for this.
 * Operators such as sum have a zero value where no nodes are mapped,
 * which includes elements outside the host mesh of the map. Other operators
 * including mean, minimum, maximum do not have valid values where no nodes
 * are mapped.
 * If no element map field is set (default), or if evaluating at a non-element
 * location, the operator applies over all nodes as normal.
 *
 * @param nodeset_operator_field  Nodeset operator field to modify.
 * @param element_map_field  Field mapping from nodes to element locations for
 * which a reverse map exists. Currently must be stored mesh location type.
 * Pass null/invalid handle to clear element mode.
 * @return  Result OK on success, otherwise any other error code.
 */
ZINC_API int cmzn_field_nodeset_operator_set_element_map_field(
	cmzn_field_nodeset_operator_id nodeset_operator_field,
	cmzn_field_id element_map_field);

/**
 * Creates a field which computes the sum of each source field component over
 * all nodes in the nodeset for which it is defined. Returned field has same
 * number of components as the source field.
 *
 * @param field_module  Region field module which will own new field.
 * @param source_field  Field to sum.
 * @param nodeset  The set of nodes to sum field over.
 * @return  Handle to new field, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_field_id cmzn_fieldmodule_create_field_nodeset_sum(
	cmzn_fieldmodule_id field_module, cmzn_field_id source_field,
	cmzn_nodeset_id nodeset);

/**
 * Creates a field which computes the mean of each source field component over
 * all nodes in the nodeset for which it is defined. Returned field has same
 * number of components as the source field.
 *
 * @param field_module  Region field module which will own new field.
 * @param source_field  Field to obtain mean component values for.
 * @param nodeset  The set of nodes to obtain mean over.
 * @return  Handle to new field, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_field_id cmzn_fieldmodule_create_field_nodeset_mean(
	cmzn_fieldmodule_id field_module, cmzn_field_id source_field,
	cmzn_nodeset_id nodeset);

/**
 * Creates a field which computes the sum of the squares of each source field
 * component over all nodes in the nodeset for which it is defined. Returned
 * field has same number of components as the source field.
 * This field type supports least-squares optimisation by giving individual
 * terms being squared and summed.
 * @see cmzn_optimisation_add_objective_field
 *
 * @param field_module  Region field module which will own new field.
 * @param source_field  Field to sum squared component values of.
 * @param nodeset  The set of nodes to sum field over.
 * @return  Handle to new field, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_field_id cmzn_fieldmodule_create_field_nodeset_sum_squares(
	cmzn_fieldmodule_id field_module, cmzn_field_id source_field,
	cmzn_nodeset_id nodeset);

/**
 * Creates a field which computes the mean of the squares of each source field
 * component over all nodes in the nodeset for which it is defined. Returned
 * field has same number of components as the source field.
 * This field type supports least-squares optimisation by giving individual
 * terms being squared and summed, each divided by the square root of the number
 * of terms.
 * @see cmzn_optimisation_add_objective_field
 *
 * @param field_module  Region field module which will own new field.
 * @param source_field  Field to obtain mean squared component values for.
 * @param nodeset  The set of nodes to obtain mean over.
 * @return  Handle to new field, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_field_id cmzn_fieldmodule_create_field_nodeset_mean_squares(
	cmzn_fieldmodule_id field_module, cmzn_field_id source_field,
	cmzn_nodeset_id nodeset);

/**
 * Creates a field which computes the minimum of each source field
 * component over all nodes in the nodeset for which it is defined. Returned
 * field has same number of components as the source field.
 *
 * @param field_module  Region field module which will own new field.
 * @param source_field  Field to obtain minimum values for.
 * @param nodeset  The set of nodes to obtain minimum over.
 * @return  Handle to new field, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_field_id cmzn_fieldmodule_create_field_nodeset_minimum(
	cmzn_fieldmodule_id field_module, cmzn_field_id source_field,
	cmzn_nodeset_id nodeset);

/**
 * Creates a field which computes the maximum of each source field
 * component over all nodes in the nodeset for which it is defined. Returned
 * field has same number of components as the source field.
 *
 * @param field_module  Region field module which will own new field.
 * @param source_field  Field to obtain maximum values for.
 * @param nodeset  The set of nodes to obtain maximum over.
 * @return  Handle to new field, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_field_id cmzn_fieldmodule_create_field_nodeset_maximum(
	cmzn_fieldmodule_id field_module, cmzn_field_id source_field,
	cmzn_nodeset_id nodeset);

#ifdef __cplusplus
}
#endif

#endif
