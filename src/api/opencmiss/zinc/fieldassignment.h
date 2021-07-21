/**
 * @file fieldassignment.h
 *
 * Public interface to field assignment object.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef CMZN_FIELDASSIGNMENT_H__
#define CMZN_FIELDASSIGNMENT_H__

#include "types/fieldassignmentid.h"
#include "types/fieldid.h"
#include "types/nodesetid.h"

#include "opencmiss/zinc/zincsharedobject.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a field assignment object for assigning values of the target field,
 * from values of the source field.
 *
 * @param targetField  The target field to assign to.
 * @param sourceField  The source field to assign values from.
 * @return  Handle to new field assignment object, or NULL/invalid handle on
 * failure.
 */
ZINC_API cmzn_fieldassignment_id cmzn_field_create_fieldassignment(
	cmzn_field_id targetField, cmzn_field_id sourceField);

/**
 * Returns a new handle to the field assignment with reference count
 * incremented.
 *
 * @param fieldassignment  The fieldassignment to obtain a new handle to.
 * @return  New handle to field assignment object, or NULL/invalid handle on
 * failure.
 */
ZINC_API cmzn_fieldassignment_id cmzn_fieldassignment_access(
	cmzn_fieldassignment_id fieldassignment);

/**
 * Destroys handle to the field assignment (and sets it to NULL).
 * Internally this decrements the reference count.
 *
 * @param fieldassignmentAddress  Address of handle to field assignment.
 * @return  Result OK on success, any other value on failure.
 */
ZINC_API int cmzn_fieldassignment_destroy(
	cmzn_fieldassignment_id *fieldassignmentAddress);

/**
 * Perform assignment of values from source to target field.
 *
 * @param fieldassignment  The field assignment description.
 * @return  Result OK on success, WARNING_PART_DONE if partial success,
 * otherwise any other error.
 */
ZINC_API int cmzn_fieldassignment_assign(
	cmzn_fieldassignment_id fieldassignment);

/**
 * Get the current conditional field for the field assignment.
 * @see cmzn_fieldassignment_set_conditional_field
 *
 * @param fieldassignment  The field assignment object to query.
 * @return  Handle to conditional field, or NULL/invalid handle if none or
 * invalid field assignment object.
 */
ZINC_API cmzn_field_id cmzn_fieldassignment_get_conditional_field(
	cmzn_fieldassignment_id fieldassignment);

/**
 * Set conditional field determining which parts of the target field domain,
 * or domain where DOFs are defined, to assign values over.
 *
 * @param fieldassignment  The field assignment object to modify.
 * @param conditionalField  The conditional field to set, or invalid handle to
 * clear. This is commonly a generic or node group field, but may be any field
 * expression returning non-zero/true at domain locations of the target field
 * where values are to be assigned, zero/false where no assignment is done.
 * @return  Result OK on success, otherwise ERROR_ARGUMENT.
 */
ZINC_API int cmzn_fieldassignment_set_conditional_field(
	cmzn_fieldassignment_id fieldassignment, cmzn_field_id conditionalField);

/**
 * Get the nodeset for the field assignment, if set.
 * @see cmzn_fieldassignment_set_nodeset
 *
 * @param fieldassignment  The field assignment object to query.
 * @return  Handle to conditional field, or NULL/invalid handle if none or
 * invalid field assignment object.
 */
ZINC_API cmzn_nodeset_id cmzn_fieldassignment_get_nodeset(
	cmzn_fieldassignment_id fieldassignment);

/**
 * Set nodeset over which DOFs are assigned. Pass NULL/invalid nodeset to
 * assign over default nodeset 'nodes'.
 *
 * @param fieldassignment  The field assignment object to modify.
 * @param nodeset  The nodeset or nodeset group to assign over, or NULL/invalid
 * to clear to default.
 * @return  Result OK on success, otherwise ERROR_ARGUMENT.
 */
ZINC_API int cmzn_fieldassignment_set_nodeset(
	cmzn_fieldassignment_id fieldassignment, cmzn_nodeset_id nodeset);

/**
* Get the source field for the field assignment.
*
* @param fieldassignment  The field assignment object to query.
* @return  Handle to source field, or NULL/invalid handle if invalid field
* assignment object.
*/
ZINC_API cmzn_field_id cmzn_fieldassignment_get_source_field(
	cmzn_fieldassignment_id fieldassignment);

/**
 * Get the target field for the field assignment.
 *
 * @param fieldassignment  The field assignment object to query.
 * @return  Handle to target field, or NULL/invalid handle if invalid field
 * assignment object.
 */
ZINC_API cmzn_field_id cmzn_fieldassignment_get_target_field(
	cmzn_fieldassignment_id fieldassignment);

#ifdef __cplusplus
}
#endif

#endif /* CMZN_FIELDASSIGNMENT_H__ */
