/**
 * @file optimisation.h
 *
 * The public interface to optimisation class which can minimise N
 * objective functions by modifying parameters of M dependent fields.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef CMZN_OPTIMISATION_H__
#define CMZN_OPTIMISATION_H__

/*
 Global types
 ------------
 */

#include "types/fieldassignmentid.h"
#include "types/fieldid.h"
#include "types/fieldmoduleid.h"
#include "types/optimisationid.h"

#include "opencmiss/zinc/zincsharedobject.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 Global functions
 ----------------
 */

/**
 * Create an optimisation object for optimising values and parameters of fields
 * from a field module.
 *
 * @param field_module  The field module to optimise fields from.
 * @return  Handle to new optimisation, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_optimisation_id cmzn_fieldmodule_create_optimisation(
	cmzn_fieldmodule_id field_module);

/**
 * Returns a new handle to the optimisation with reference count incremented.
 *
 * @param optimisation  The optimisation to obtain a new handle to.
 * @return  New handle to optimisation, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_optimisation_id cmzn_optimisation_access(cmzn_optimisation_id optimisation);

/**
 * Destroys handle to the optimisation object and sets pointer/handle to NULL.
 *
 * @param optimisation_address  Address of optimisation object handle.
 * @return  Result OK on success, any other value on failure.
 */
ZINC_API int cmzn_optimisation_destroy(cmzn_optimisation_id *optimisation_address);

/**
 * Get the conditional field which controls which degrees of freedom of a
 * dependent field are included in the optimisation.
 * @see cmzn_optimisation_set_conditional_field
 *
 * @param optimisation  Handle to the optimisation object.
 * @param dependent_field  The dependent field the condition applies to.
 * @return  Handle to conditional field, or NULL/invalid handle if none
 * or failed.
 */
ZINC_API cmzn_field_id cmzn_optimisation_get_conditional_field(
	cmzn_optimisation_id optimisation, cmzn_field_id dependent_field);

/**
 * Set a conditional field which controls which degrees of freedom of a
 * dependent field are included in the optimisation, for all components or
 * per-component. The conditional field is queried at the start of the
 * optimisation, so the number of DOFs remains constant throughout it.
 * The conditional field only applies to finite element dependent fields,
 * and is queried and applied to nodal DOFs only.
 * @note
 * The conditional field is not yet supported by the NEWTON method.
 *
 * @param optimisation  Handle to the optimisation object.
 * @param dependent_field  The dependent field to select a subset of DOFs
 * from. Must already have been added to the optimisation.
 * @param conditional_field  A field with either one component or the same
 * number of components as the dependent field. DOFs for the dependent
 * field (or components of it, if non-scalar) are included only where this
 * field is defined and non-zero. Pass a NULL/invalid handle to clear.
 * @return  Result OK on success, any other value on failure.
 */
ZINC_API int cmzn_optimisation_set_conditional_field(
	cmzn_optimisation_id optimisation, cmzn_field_id dependent_field,
	cmzn_field_id conditional_field);

/**
 * Add a field assignment object to the optimisation, to be applied before
 * objective evaluation with each set of trial dependent field DOFs, and at the
 * end of optimisation. Multiple field assignments are applied in the order
 * they are added.
 * The main use is to partially apply the effect of the dependent field on
 * the DOFs of another field; an example is optimising a constant offset over a
 * subset of nodes, where DOFs at those nodes contribute to the objective.
 * Note if the source field of the assignment is a function of the target field
 * then target DOFs will drift away. In some cases this is solved by making the
 * source field a function of a copy of the target field with its DOFs prior to
 * the optimise call. However to assign multiple versions and derivatives in the
 * target field requires the source field to be a function of it; the solution
 * is to add two field assignments, the first resets target DOFs to their
 * initial values, the second assigns them to the source values.
 * Field assignment is not supported by the NEWTON method.
 *
 * @param optimisation  The optimisation object to modify.
 * @param fieldassignment  Field assignment to apply. Must be for a field in
 * the same fieldmodule as this optimisation object.
 * @return  Result OK if field successfully added, or an error code if
 * failed or already added.
 */
ZINC_API int cmzn_optimisation_add_fieldassignment(
	cmzn_optimisation_id optimisation, cmzn_fieldassignment_id fieldassignment);

/**
 * Get the current optimisation method for the given optimisation object.
 *
 * @param optimisation  Handle to the optimisation object.
 * @return  The current optimisation method.
 */
ZINC_API enum cmzn_optimisation_method cmzn_optimisation_get_method(
	cmzn_optimisation_id optimisation);

/**
 * Set the optimisation method for the given optimisation object.
 *
 * @param optimisation  Handle to the optimisation object.
 * @param method  The optimisation method to use.
 * @return  Result OK on success, otherwise ERROR_ARGUMENT.
 */
ZINC_API int cmzn_optimisation_set_method(cmzn_optimisation_id optimisation,
	enum cmzn_optimisation_method method);

/**
 * Convert a short name into an enum if the name matches any of the members in
 * the enum.
 *
 * @param string  string of the short enumerator name
 * @return  the correct enum type if a match is found.
 */
ZINC_API enum cmzn_optimisation_method cmzn_optimisation_method_enum_from_string(
	const char *string);

/**
 * Return an allocated short name of the enum type from the provided enum.
 * User must call cmzn_deallocate to destroy the successfully returned string.
 *
 * @param format  enum to be converted into string
 * @return  an allocated string which stored the short name of the enum.
 */
ZINC_API char *cmzn_optimisation_method_enum_to_string(enum cmzn_optimisation_method method);

/**
 * Get an integer or Boolean attribute of the optimisation object.
 *
 * @param optimisation  Handle to the optimisation object.
 * @param attribute  The identifier of the integer attribute to get.
 * @return  Value of the attribute. Boolean values are 1 if true, 0 if false.
 */
ZINC_API int cmzn_optimisation_get_attribute_integer(cmzn_optimisation_id optimisation,
	enum cmzn_optimisation_attribute attribute);

/**
 * Set an integer or Boolean attribute of the optimisation object.
 *
 * @param optimisation  Handle to the optimisation object.
 * @param attribute  The identifier of the integer attribute to set.
 * @param value  The new value for the attribute. For Boolean values use 1 for
 * true in case more options are added in future.
 * @return  Result OK if attribute successfully set, or an error code if
 * failed or attribute not valid or able to be set for this optimisation object.
 */
ZINC_API int cmzn_optimisation_set_attribute_integer(cmzn_optimisation_id optimisation,
	enum cmzn_optimisation_attribute attribute, int value);

/**
 * Get a real attribute of the optimisation object.
 *
 * @param optimisation  Handle to the optimisation object.
 * @param attribute  The identifier of the real attribute to get.
 * @return  Value of the attribute.
 */
ZINC_API double cmzn_optimisation_get_attribute_real(cmzn_optimisation_id optimisation,
	enum cmzn_optimisation_attribute attribute);

/**
 * Set a real attribute of the optimisation object.
 *
 * @param optimisation  Handle to the optimisation object.
 * @param attribute  The identifier of the real attribute to set.
 * @param value  The new value for the attribute.
 * @return  Result OK if attribute successfully set, or an error code if
 * failed or attribute not valid or able to be set for this optimisation object.
 */
ZINC_API int cmzn_optimisation_set_attribute_real(cmzn_optimisation_id optimisation,
	enum cmzn_optimisation_attribute attribute, double value);

/**
 * Convert a short attribute name into an enum if the attribute name matches
 * any of the members in the enum.
 *
 * @param attribute_name  string of the short enumerator name
 * @return  the correct enum type if a match is found.
 */
ZINC_API enum cmzn_optimisation_attribute cmzn_optimisation_attribute_enum_from_string(
	const char *string);

/**
 * Return an allocated short name of the enum type from the provided enum.
 * User must call cmzn_deallocate to destroy the successfully returned string.
 *
 * @param attribute  enum to be converted into string
 * @return  an allocated string which stored the short name of the enum.
 */
ZINC_API char *cmzn_optimisation_attribute_enum_to_string(
	enum cmzn_optimisation_attribute attribute);

/**
 * Get the first dependent field from the optimisation problem description.
 * @see cmzn_optimisation_get_next_dependent_field
 *
 * @param optimisation  Handle to the optimisation object to query.
 * @return  Handle to first dependent field, or NULL/invalid handle if none or failed.
 */
ZINC_API cmzn_field_id cmzn_optimisation_get_first_dependent_field(
	cmzn_optimisation_id optimisation);

/**
 * Get the next dependent field in the optimisation problem description after
 * the supplied ref_field.
 *
 * @param optimisation  Handle to the optimisation object to query.
 * @param ref_field  Handle to a dependent field from the optimisation.
 * @return  Handle to next dependent field after ref_field,
 * or NULL/invalid handle if none or failed.
 */
ZINC_API cmzn_field_id cmzn_optimisation_get_next_dependent_field(
	cmzn_optimisation_id optimisation, cmzn_field_id ref_field);

/**
 * Add a dependent field to the optimisation problem, whose parameters are
 * solved for to minimise the objective function/s.
 * Valid dependent fields are limited to constant or finite element types.
 * @note
 * Multiple dependent fields can only be used if all are constant type. The
 * NEWTON method only works with a single finite element dependent field. These
 * are checked when optimisation is run as the method may be set later.
 * @note
 * Some existing cubic Hermite meshes use different value or derivative
 * parameter versions on adjacent elements and require parameter tying to
 * maintain continuity during computation. These models need to be modified to
 * share common parameters and remove unused parameters before use.
 *
 * @param optimisation  Handle to the optimisation object.
 * @param field  Real-valued dependent field to add to the optimisation object.
 * Must be constant or finite element type, and used in the objective
 * expression.
 * @return  Result OK if field successfully added, or an error code if
 * failed or already added.
 */
ZINC_API int cmzn_optimisation_add_dependent_field(cmzn_optimisation_id optimisation,
	cmzn_field_id field);

/**
 * Remove a dependent field from the optimisation problem.
 *
 * @param optimisation  Handle to the optimisation object.
 * @param field  The dependent field to remove.
 * @return  Result OK if field successfully removed, or an error code if
 * failed or field not found.
 */
ZINC_API int cmzn_optimisation_remove_dependent_field(
	cmzn_optimisation_id optimisation, cmzn_field_id field);

/**
 * @deprecated  Misnamed: use dependent field function.
 * @see cmzn_optimisation_get_first_dependent_field
 */
ZINC_API cmzn_field_id cmzn_optimisation_get_first_independent_field(
	cmzn_optimisation_id optimisation);

/**
 * @deprecated  Misnamed: use dependent field function.
 * @see cmzn_optimisation_get_next_dependent_field
 */
ZINC_API cmzn_field_id cmzn_optimisation_get_next_independent_field(
	cmzn_optimisation_id optimisation, cmzn_field_id ref_field);

/**
 * @deprecated  Misnamed: use dependent field function.
 * @see cmzn_optimisation_add_dependent_field
 */
ZINC_API int cmzn_optimisation_add_independent_field(cmzn_optimisation_id optimisation,
	cmzn_field_id field);

/**
 * @deprecated  Misnamed: use dependent field function.
 * @see cmzn_optimisation_remove_dependent_field
 */
ZINC_API int cmzn_optimisation_remove_independent_field(
	cmzn_optimisation_id optimisation, cmzn_field_id field);

/**
 * Get the first objective field from the optimisation problem description.
 * @see cmzn_optimisation_get_next_objective_field
 *
 * @param optimisation  Handle to the optimisation object to query.
 * @return  Handle to first objective field, or NULL/invalid handle if none or failed.
 */
ZINC_API cmzn_field_id cmzn_optimisation_get_first_objective_field(
	cmzn_optimisation_id optimisation);

/**
 * Get the next objective field in the optimisation problem description after
 * the supplied ref_field. Use to iterate over the objective fields, taking
 * care to destroy all returned field handles exactly once:
 *
 * cmzn_field_id field = cmzn_optimisation_get_first_objective_field(optimisation);
 * while (field)
 * {
 *    cmzn_field_id next_field = cmzn_optimisation_get_next_objective_field(optimisation, field);
 *    cmzn_field_destroy(&field);
 *    field = next_field;
 * }
 *
 * @param optimisation  Handle to the optimisation object to query.
 * @param ref_field  Handle to an objective field from the optimisation.
 * @return  Handle to next objective field after ref_field,
 * or NULL/invalid handle if none or failed.
 */
ZINC_API cmzn_field_id cmzn_optimisation_get_next_objective_field(
	cmzn_optimisation_id optimisation, cmzn_field_id ref_field);

/**
 * Add an objective field to the optimisation problem description.
 * Valid objective fields must be spatially constant. The least squares solution
 * method treats fields performing a sum of squares (nodeset_sum_squares,
 * nodeset_mean_squares) specially, passing each term to the optimiser.
 * The overall objective function becomes the sum of all components of all
 * objective fields, or for the least-squares method, the sum of the squares of
 * all terms (or components if the objective field is not a sum of squares).
 *
 * @param optimisation  Handle to the optimisation object.
 * @param field  Real-valued objective field to add to the optimisation object
 * (accessed internally so safe for caller to destroy locally).
 * @return  Result OK if field successfully added, or an error code if
 * failed or already added.
 */
ZINC_API int cmzn_optimisation_add_objective_field(cmzn_optimisation_id optimisation,
	cmzn_field_id field);

/**
 * Remove an objective field from the optimisation problem.
 *
 * @param optimisation  Handle to the optimisation object.
 * @param field  The objective field to remove.
 * @return  Result OK if field successfully removed, or an error code if
 * failed or field not found.
 */
ZINC_API int cmzn_optimisation_remove_objective_field(
	cmzn_optimisation_id optimisation, cmzn_field_id field);

/**
 * Get a textual report on the last solution.
 *
 * @param optimisation  Handle to the optimisation object to query.
 * @return  Allocated string containing report which user must free using
 * cmzn_deallocate, or NULL on error or if haven't performed optimisation yet.
 */
ZINC_API char *cmzn_optimisation_get_solution_report(cmzn_optimisation_id optimisation);

/**
 * Perform the optimisation described by the provided optimisation object.
 *
 * @param optimisation Handle to the zinc optimisation object.
 * @return Result OK if optimisation completed successfully (stopping
 * criteria satisfied), and any other value on failure.
 */
ZINC_API int cmzn_optimisation_optimise(cmzn_optimisation_id optimisation);

#ifdef __cplusplus
}
#endif

#endif
