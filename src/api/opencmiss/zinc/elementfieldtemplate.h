/**
 * @file elementfieldtemplate.h
 *
 * The public interface to Zinc element field templates.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef CMZN_ELEMENTFIELDTEMPLATE_H__
#define CMZN_ELEMENTFIELDTEMPLATE_H__

#include "types/elementbasisid.h"
#include "types/elementfieldtemplateid.h"
#include "types/nodeid.h"

#include "opencmiss/zinc/zincsharedobject.h"

/*
Global types
------------
*/

#ifdef __cplusplus
extern "C" {
#endif

/*
Global functions
----------------
*/

/**
 * Returns a new handle to the element field template with reference count
 * incremented.
 *
 * @param elementfieldtemplate  The element field template to obtain a new
 * handle to.
 * @return  New handle to element template, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_elementfieldtemplate_id cmzn_elementfieldtemplate_access(
	cmzn_elementfieldtemplate_id elementfieldtemplate);

/**
 * Destroys this handle to the element field template and sets it to NULL.
 * Internally this decrements the reference count.
 *
 * @param elementfieldtemplate_address  Address of handle to element field
 * template to destroy.
 * @return  Result OK on success, otherwise an error code.
 */
ZINC_API int cmzn_elementfieldtemplate_destroy(
	cmzn_elementfieldtemplate_id *elementfieldtemplate_address);

/**
 * Get the element basis used by this element field template.
 *
 * @param elementfieldtemplate  Element field template to query.
 * @return  Handle to element basis, or NULL/invalid handle if failed.
 
 The parameter mapping mode or INVALID on error.
 */
ZINC_API cmzn_elementbasis_id cmzn_elementfieldtemplate_get_elementbasis(
	cmzn_elementfieldtemplate_id elementfieldtemplate);

/**
 * Get the number of terms that are summed to give the element parameter weighting
 * the given function number.
 *
 * @param elementfieldtemplate  Element field template to query.
 * @param functionNumber  Basis function number from 1 to number of functions.
 * @return  The number of terms >= 0, or -1 if invalid arguments.
 */
ZINC_API int cmzn_elementfieldtemplate_get_function_number_of_terms(
	cmzn_elementfieldtemplate_id elementfieldtemplate, int functionNumber);

/**
 * Set the number of terms that are summed to give the element parameter
 * weighting the given basis function number. Currently only supported for node
 * mapping - other parameter mapping modes are fixed at 1 term.
 * If reducing number, existing mappings for higher terms are discarded.
 * If increasing number, new mappings must be completely specified by
 * subsequent calls; new mappings are unscaled by default.
 *
 * @param elementfieldtemplate  Element field template to modify.
 * @param functionNumber  Basis function number from 1 to number of functions.
 * @param newNumberOfTerms  New number of terms to be summed, >= 0.
 * @return  Result OK on success, otherwise an error code.
 */
ZINC_API int cmzn_elementfieldtemplate_set_function_number_of_terms(
	cmzn_elementfieldtemplate_id elementfieldtemplate, int functionNumber,
	int newNumberOfTerms);

/**
 * Get the number of basis functions in the element basis used by the template.
 *
 * @param elementfieldtemplate  Element field template to query.
 * @return  The number of basis functions, 0 on error.
 */
ZINC_API int cmzn_elementfieldtemplate_get_number_of_functions(
	cmzn_elementfieldtemplate_id elementfieldtemplate);

/**
 * Get the number of local nodes this element field template uses.
 *
 * @param elementfieldtemplate  Element field template to query.
 * @return  The number of local nodes, or 0 if not node parameter mapping.
 */
ZINC_API int cmzn_elementfieldtemplate_get_number_of_local_nodes(
	cmzn_elementfieldtemplate_id elementfieldtemplate);

/**
 * Set the number of local nodes this element field template uses. If reducing
 * number, template is only valid once all indexes are in the range [1..number].
 * Only valid in node mapping mode.
 *
 * @param elementfieldtemplate  Element field template to modify.
 * @param number  The number of local nodes to set, > 0.
 * @return  Result OK on success, otherwise an error code.
 */
ZINC_API int cmzn_elementfieldtemplate_set_number_of_local_nodes(
	cmzn_elementfieldtemplate_id elementfieldtemplate, int number);

/**
 * Get the number of local scale factors this element field template uses.
 *
 * @param elementfieldtemplate  Element field template to query.
 * @return  The number of local scale factors, 0 if none or error.
 */
ZINC_API int cmzn_elementfieldtemplate_get_number_of_local_scale_factors(
	cmzn_elementfieldtemplate_id elementfieldtemplate);

/**
 * Set the number of local scale factors this element field template uses.
 * If reducing number, template is only valid once all indexes are in the range
 * [1..number]. Only valid in node mapping mode.
 * New local scale factors default to type LOCAL_GENERAL, version 1.
 *
 * @param elementfieldtemplate  Element field template to modify.
 * @param number  The number of local scale factors to set, >= 0.
 * @return  Result OK on success, otherwise an error code.
 */
ZINC_API int cmzn_elementfieldtemplate_set_number_of_local_scale_factors(
	cmzn_elementfieldtemplate_id elementfieldtemplate, int number);

/**
 * Get the parameter mapping mode used for all parameters in the template.
 *
 * @param elementfieldtemplate  Element field template to query.
 * @return  The parameter mapping mode or INVALID on error.
 */
ZINC_API enum cmzn_elementfieldtemplate_parameter_mapping_mode
	cmzn_elementfieldtemplate_get_parameter_mapping_mode(
		cmzn_elementfieldtemplate_id elementfieldtemplate);

/**
 * Set the parameter mapping mode used for all parameters in the template.
 * A current restriction is that all parameters are mapped by NODE, ELEMENT
 * or FIELD.
 * Note this resets the mapping to the default for the given mapping mode:
 * one term per basis function with no scaling hence should be the first
 * setting changed.
 *
 * @param elementfieldtemplate  Element field template to modify.
 * @param mode  The parameter mapping mode to set.
 * @return  Result OK on success, otherwise an error code.
 */
ZINC_API int cmzn_elementfieldtemplate_set_parameter_mapping_mode(
	cmzn_elementfieldtemplate_id elementfieldtemplate,
	enum cmzn_elementfieldtemplate_parameter_mapping_mode mode);

/**
 * Get the identifier of the scale factor mapped to the local scale factor
 * index. Used to match common scale factors between elements and fields.
 *
 * @param elementfieldtemplate  Element field template to query.
 * @param localScaleFactorIndex  The local scale factor index from 1 to number
 * of local scale factors.
 * @return  The element scale factor identifier >= 0, or -1 if invalid or
 * error.
 */
ZINC_API int cmzn_elementfieldtemplate_get_scale_factor_identifier(
	cmzn_elementfieldtemplate_id elementfieldtemplate,
	int localScaleFactorIndex);

/**
 * Set the identifier of the scale factor mapped to the local scale factor
 * index. Used to match common scale factors between elements and fields.
 * Global scale factors are matched by type and as appropriate
 * global node/element and local or global scale factor identifier.
 * For each global scale factor type, the identifier is globally unique.
 * For each node-based scale factor type, the identifier is unique at each
 * node. For each element-based scale factor, only the special identifier 0
 * is currently permitted, and scale factors are private to each element and
 * element field template.
 * To tie node-based scale factor identifiers to node value labels/derivatives,
 * the recommended convention is to use an identifier matching the numerical
 * value of the node value label enumeration, and for each distinct version
 * of the scale factor, add 100 times the version number (to allow for new
 * node value labels in future).
 * New scale factors default to ELEMENT GENERAL type with identifier 0.
 *
 * @param elementfieldtemplate  Element field template to modify.
 * @param localScaleFactorIndex  The local scale factor index from 1 to number
 * of local scale factors.
 * @param identifier  The identifier >= 0. Special value 0 is only permitted
 * for element type scale factors, but this is not checked until final
 * validation.
 * @return  Result OK on success, otherwise an error code.
 */
ZINC_API int cmzn_elementfieldtemplate_set_scale_factor_identifier(
	cmzn_elementfieldtemplate_id elementfieldtemplate,
	int localScaleFactorIndex, int identifier);

/**
 * Get the type of scale factor mapped to the local scale factor index.
 * Used to match common scale factors between elements and fields.
 *
 * @param elementfieldtemplate  Element field template to query.
 * @param localScaleFactorIndex  The local scale factor index from 1 to number
 * of local scale factors.
 * @return  The element scale factor type, or INVALID on error.
 */
ZINC_API enum cmzn_elementfieldtemplate_scale_factor_type
	cmzn_elementfieldtemplate_get_scale_factor_type(
		cmzn_elementfieldtemplate_id elementfieldtemplate,
		int localScaleFactorIndex);

/**
 * Set the type of scale factor mapped to the local scale factor index.
 * Used to match common scale factors between elements and fields.
 * Global scale factors are matched by type and as appropriate
 * global node/element and local or global scale factor identifier.
 * New scale factors default to ELEMENT GENERAL type.
 *
 * @param elementfieldtemplate  Element field template to query.
 * @param localScaleFactorIndex  The local scale factor index from 1 to number
 * of local scale factors.
 * @param type  The element scale factor type.
 * @return  Result OK on success, otherwise an error code.
 */
ZINC_API int cmzn_elementfieldtemplate_set_scale_factor_type(
	cmzn_elementfieldtemplate_id elementfieldtemplate,
	int localScaleFactorIndex,
	enum cmzn_elementfieldtemplate_scale_factor_type type);

/**
 * Get the local node index from which a node parameter is extracted for the
 * given term for the function number. For parameter mapping mode NODE only.
 *
 * @param elementfieldtemplate  Element field template to query.
 * @param functionNumber  Basis function number from 1 to number of functions.
 * @param term  The term number, from 1 to function number of terms.
 * @return  The local node index from 1 to number of local nodes, or 0 if
 * invalid or error.
 */
ZINC_API int cmzn_elementfieldtemplate_get_term_local_node_index(
	cmzn_elementfieldtemplate_id elementfieldtemplate, int functionNumber,
	int term);

/**
 * Get the node parameter value label mapped to the given term for the function
 * number. For parameter mapping mode NODE only.
 *
 * @param elementfieldtemplate  Element field template to query.
 * @param functionNumber  Basis function number from 1 to number of functions.
 * @param term  The term number, from 1 to function number of terms.
 * @return  The node value label, or INVALID if invalid or error.
 */
ZINC_API enum cmzn_node_value_label cmzn_elementfieldtemplate_get_term_node_value_label(
	cmzn_elementfieldtemplate_id elementfieldtemplate, int functionNumber,
	int term);

/**
 * Get the node parameter version mapped to the given term for the function
 * number. For parameter mapping mode NODE only.
 *
 * @param elementfieldtemplate  Element field template to query.
 * @param functionNumber  Basis function number from 1 to number of functions.
 * @param term  The term number, from 1 to function number of terms.
 * @return  The node parameter version >= 1, or 0 if invalid or error.
 */
ZINC_API int cmzn_elementfieldtemplate_get_term_node_version(
	cmzn_elementfieldtemplate_id elementfieldtemplate, int functionNumber,
	int term);

/**
 * Set the node parameter mapped to the given term for the function number, via
 * the local node index, node value label and node parameter version.
 * For parameter mapping mode NODE only.
 *
 * @param elementfieldtemplate  Element field template to modify.
 * @param functionNumber  Basis function number from 1 to number of functions.
 * @param term  The term number, from 1 to function number of terms.
 * @param localNodeIndex  The local node index from 1 to number of local nodes.
 * @param valueLabel  The node parameter value label.
 * @param version  The node parameter version >= 1.
 * @return  Result OK on success, otherwise an error code.
 */
ZINC_API int cmzn_elementfieldtemplate_set_term_node_parameter(
	cmzn_elementfieldtemplate_id elementfieldtemplate, int functionNumber,
	int term, int localNodeIndex, cmzn_node_value_label valueLabel, int version);

/**
 * Get the number and local indexes of scale factors multiplying the given term
 * for the function number. For parameter mapping mode NODE only.
 *
 * @param elementfieldtemplate  Element field template to query.
 * @param functionNumber  Basis function number from 1 to number of functions.
 * @param term  The term number, from 1 to function number of terms.
 * @param indexesCount  The size of the indexes array; can be more or less than
 * the number of indexes in use.
 * @param indexesOut  Array to store indexesCount indexes. Can be NULL if
 * indexesCount is zero.
 * @return  The actual number of scaling indexes >= 0 for term, or -1 on error.
 */
ZINC_API int cmzn_elementfieldtemplate_get_term_scaling(
	cmzn_elementfieldtemplate_id elementfieldtemplate, int functionNumber,
	int term, int indexesCount, int *indexesOut);

/**
 * Set scaling of the function term by the product of scale factors at the
 * given local scale factor indexes.
 * Must have set positive number of local scale factors before calling.
 * For parameter mapping mode NODE only.
 *
 * @param elementfieldtemplate  Element field template to modify.
 * @param functionNumber  Basis function number from 1 to number of functions.
 * @param term  The term number, from 1 to function number of terms.
 * @param indexesCount  The size of the indexes array; can be more or less than
 * the number of indexes in use.
 * @param indexesIn  Array of indexesCount indexes, each from 1 to number of
 * local scale factors. Can be NULL if indexesCount is zero.
 * @return  Result OK on success, otherwise an error code.
 */
ZINC_API int cmzn_elementfieldtemplate_set_term_scaling(
	cmzn_elementfieldtemplate_id elementfieldtemplate, int functionNumber,
	int term, int indexesCount, const int *indexesIn);

/**
 * Query whether element field template passes validation. Failure typically
 * results from local node or scale factor indexes being out of range, or
 * use of combinations of values that are not supported, e.g. scale factor
 * identifiers have various restrictions for each type.
 * Refer to the respective API help to determine what is valid.
 * Validation errors are sent to the logger; clients may either inspect
 * logger output or query template to determine source of error.
 *
 * @param elementfieldtemplate  Element field template to query.
 * @return  True if element field template passes validation, otherwise false.
 */
ZINC_API bool cmzn_elementfieldtemplate_validate(
	cmzn_elementfieldtemplate_id elementfieldtemplate);

#ifdef __cplusplus
}
#endif

#endif /* CMZN_ELEMENTFIELDTEMPLATE_H__ */
