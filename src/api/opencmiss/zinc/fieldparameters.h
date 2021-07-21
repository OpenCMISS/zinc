/**
 * @file fieldparameters.h
 *
 * Public interface to parameters of a field.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef CMZN_FIELDPARAMETERS_H__
#define CMZN_FIELDPARAMETERS_H__

#include "types/differentialoperatorid.h"
#include "types/elementid.h"
#include "types/fieldparametersid.h"
#include "types/fieldid.h"

#include "opencmiss/zinc/zincsharedobject.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get field parameters object for field.
 * Currently only supported by finite element field type.
 * @note
 * Only node-based field parameters are supported at present.
 * @note
 * The client must get the number of parameters before using any other API.
 * @see cmzn_fieldparameters_get_number_of_parameters
 *
 * @param field  The field to get parameters object for.
 * @return  Handle to new field parameters object, or NULL/invalid handle on
 * failure or if not supported by field type.
 */
ZINC_API cmzn_fieldparameters_id cmzn_field_get_fieldparameters(
	cmzn_field_id field);

/**
 * Returns a new handle to the field parameters with reference count
 * incremented.
 *
 * @param fieldparameters  The fieldparameters to obtain a new handle to.
 * @return  New handle to field parameters object, or NULL/invalid handle on
 * failure.
 */
ZINC_API cmzn_fieldparameters_id cmzn_fieldparameters_access(
	cmzn_fieldparameters_id fieldparameters);

/**
 * Destroys handle to the field parameters (and sets it to NULL).
 * Internally this decrements the reference count.
 *
 * @param fieldparametersAddress  Address of handle to field parameters.
 * @return  Result OK on success, any other value on failure.
 */
ZINC_API int cmzn_fieldparameters_destroy(
	cmzn_fieldparameters_id *fieldparametersAddress);

/**
 * Returns a differential operator for evaluating derivatives of a field
 * with respect to these field parameters, generally per-element.
 *
 * @param fieldparameters  The field parameters derivatives are w.r.t.
 * @param order  The order of the derivative. Currently must be 1 or 2.
 * @return  Handle to differential operator, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_differentialoperator_id cmzn_fieldparameters_get_derivative_operator(
	cmzn_fieldparameters_id fieldparameters, int order);

/**
 * Get the array of parameter indexes in the order used in element, used to
 * assemble vector and matrix values obtained from parameter derivatives.
 * This variant returns one-based indexes (first index is 1).
 *
 * @param fieldparameters  The field parameters object to query.
 * @param element  The element to query.
 * @param valuesCount  The size of the valuesOut array to fill, must be at
 * least the number of element parameters.
 * @param valuesOut  Array to receive one-based indexes.
 * @return  Result OK on success, ERROR_NOT_FOUND if field not defined at
 * element, otherwise any other error code.
 */
ZINC_API int cmzn_fieldparameters_get_element_parameter_indexes(
	cmzn_fieldparameters_id fieldparameters, cmzn_element_id element,
	int valuesCount, int *valuesOut);

/**
 * Get the array of parameter indexes in the order used in element, used to
 * assemble vector and matrix values obtained from parameter derivatives.
 * This variant returns zero-based indexes (first index is 0).
 *
 * @param fieldparameters  The field parameters object to query.
 * @param element  The element to query.
 * @param valuesCount  The size of the valuesOut array to fill, must be at
 * least the number of element parameters.
 * @param valuesOut  Array to receive zero-based indexes.
 * @return  Result OK on success, ERROR_NOT_FOUND if field not defined at
 * element, otherwise any other error code.
 */
ZINC_API int cmzn_fieldparameters_get_element_parameter_indexes_zero(
	cmzn_fieldparameters_id fieldparameters, cmzn_element_id element,
	int valuesCount, int *valuesOut);

/**
* Get the field these field parameters are for.
*
* @param fieldparameters  The field parameters object to query.
* @return  Handle to field, or NULL/invalid handle if invalid field
* parameters object.
*/
ZINC_API cmzn_field_id cmzn_fieldparameters_get_field(
	cmzn_fieldparameters_id fieldparameters);

/**
 * Get the number of parameters for the field in element.
 *
 * @param fieldparameters  The field parameters object to query.
 * @param element  The element to query.
 * @return  Number of field parameters in element, including 0 if
 * not defined or no parameters, or -1 if invalid arguments given.
 */
ZINC_API int cmzn_fieldparameters_get_number_of_element_parameters(
	cmzn_fieldparameters_id fieldparameters, cmzn_element_id element);

/**
 * Get the total number of parameters for the field, typically used to
 * initialise sizes of the global solution matrix and vector.
 * A call to this function is needed to build internal parameter maps, both
 * initially and after changing definition of the field e.g. at nodes.
 * This must be done before using other APIs for getting numbers and indexes
 * of element parameters or to get/set/add parameters; the use of those other
 * APIs is expected to be carried out only while the field is unchanging.
 *
 * @param fieldparameters  The field parameters object to query.
 * @return  Number of field parameters or -1 on error.
 */
ZINC_API int cmzn_fieldparameters_get_number_of_parameters(
	cmzn_fieldparameters_id fieldparameters);

/**
 * Add incremental values to all field parameters.
 *
 * @param fieldparameters  The field parameters whose field will be modified.
 * @param valuesCount  The size of the valuesIn array, must be at least the
 * total number of parameters.
 * @param valuesIn  Array containing increments to add to all field parameters,
 * in their default index order i.e. given by get element parameter indexes
 * functions.
 * @return  Result OK on success, otherwise any other error code.
 */
ZINC_API int cmzn_fieldparameters_add_parameters(
	cmzn_fieldparameters_id fieldparameters,
	int valuesCount, const double *valuesIn);

/**
 * Get all field parameters.
 *
 * @param fieldparameters  The field parameters to query.
 * @param valuesCount  The size of the valuesOut array, must be at least the
 * total number of parameters.
 * @param valuesOut  Array to contain values of all field parameters, in their
 * default index order i.e. given by get element parameter indexes functions.
 * @return  Result OK on success, otherwise any other error code.
 */
ZINC_API int cmzn_fieldparameters_get_parameters(
	cmzn_fieldparameters_id fieldparameters,
	int valuesCount, double *valuesOut);

/**
 * Set all field parameters.
 *
 * @param fieldparameters  The field parameters whose field will be modified.
 * @param valuesCount  The size of the valuesIn array, must be at least the
 * total number of parameters.
 * @param valuesIn  Array containing new values for all field parameters, in
 * their default index order i.e. given by get element parameter indexes
 * functions.
 * @return  Result OK on success, otherwise any other error code.
 */
ZINC_API int cmzn_fieldparameters_set_parameters(
	cmzn_fieldparameters_id fieldparameters,
	int valuesCount, const double *valuesIn);

#ifdef __cplusplus
}
#endif

#endif /* CMZN_FIELDPARAMETERS_H__ */
