/**
 * @file elementbasis.h
 *
 * The public interface to Zinc element basis functions.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef CMZN_ELEMENTBASIS_H__
#define CMZN_ELEMENTBASIS_H__

#include "types/elementbasisid.h"
#include "types/fieldmoduleid.h"

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
 * Convert a short name into an enum if the name matches any of the members in
 * the enum.
 *
 * @param string  string of the short enumerator name
 * @return  the correct enum type if a match is found.
 */
ZINC_API enum cmzn_elementbasis_function_type cmzn_elementbasis_function_type_enum_from_string(
	const char *string);

/**
 * Return an allocated short name of the enum type from the provided enum.
 * User must call cmzn_deallocate to destroy the successfully returned string.
 *
 * @param type  enum to be converted into string
 * @return  an allocated string which stored the short name of the enum.
 */
ZINC_API char *cmzn_elementbasis_function_type_enum_to_string(
	enum cmzn_elementbasis_function_type type);

/**
 * Creates an element_basis object for describing element basis functions.
 *
 * @param fieldmodule  Handle to a field module. Note the returned basis can be
 * used to define fields in any field module of the region tree.
 * @param dimension  The dimension of element chart the basis is for.
 * @param function_type  The basis function type to use in each dimension
 * i.e. basis function is initially homogeneous.
 * @return  Handle to new element basis, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_elementbasis_id cmzn_fieldmodule_create_elementbasis(
	cmzn_fieldmodule_id fieldmodule, int dimension,
	enum cmzn_elementbasis_function_type function_type);

/**
 * Returns a new handle to the element basis with reference count incremented.
 *
 * @param mesh  The element basis to obtain a new handle to.
 * @return  New handle to element basis, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_elementbasis_id cmzn_elementbasis_access(
	cmzn_elementbasis_id element_basis);

/**
 * Destroys this handle to the element_basis and sets it to NULL.
 * Internally this decrements the reference count.
 *
 * @param element_basis_address  Address of handle to element_basis to destroy.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
ZINC_API int cmzn_elementbasis_destroy(cmzn_elementbasis_id *element_basis_address);

/**
 * Gets the number of dimensions of the elements this basis works with.
 *
 * @param element_basis  Element basis to query.
 * @return  The number of dimensions.
 */
ZINC_API int cmzn_elementbasis_get_dimension(cmzn_elementbasis_id element_basis);

/**
 * Gets the basis function type for a chart component of the basis.
 *
 * @param element_basis  Element basis to query.
 * @param chart_component  The chart component to get the function for from 1 to
 * dimension, or -1 to request function type if homogeneous for all components.
 * @return  The basis function type, or INVALID type if failed, including if
 * chart_component -1 is used and basis is not homogeneous.
 */
ZINC_API enum cmzn_elementbasis_function_type cmzn_elementbasis_get_function_type(
	cmzn_elementbasis_id element_basis, int chart_component);

/**
 * Sets a simple basis function type for a component of the basis.
 *
 * @param element_basis  Element basis to modify.
 * @param chart_component  The chart component to set the function for from 1 to
 * dimension, or -1 to set all components to use the same function type.
 * @param function_type  The basis function type to set for the component.
 * @return  Result OK on success, any other value on failure.
 */
ZINC_API int cmzn_elementbasis_set_function_type(cmzn_elementbasis_id element_basis,
	int chart_component, enum cmzn_elementbasis_function_type function_type);

/**
 * If the basis is valid, gets the number of nodes the basis expects.
 *
 * @param element_basis  Element basis to query.
 * @return  number of nodes expected, or 0 if basis is incomplete or invalid.
 */
ZINC_API int cmzn_elementbasis_get_number_of_nodes(
	cmzn_elementbasis_id element_basis);

/**
 * If the basis is valid, gets the number of functions it has, which equals the
 * number of parameters required.
 * Note that with Hermite bases the parameters are grouped by nodes, i.e. all
 * parameters for the first node are first, followed by those for the second
 * node, and so on. Within the parameters for each node, parameters are ordered
 * by the value/derivative type: the value is first, then DS1 derivatives cycle
 * fastest followed by DS2 then DS3.
 *
 * @param element_basis  Element basis to query.
 * @return  Number of parameters expected, or 0 if basis is incomplete or
 * invalid.
 */
ZINC_API int cmzn_elementbasis_get_number_of_functions(
	cmzn_elementbasis_id element_basis);

/**
 * Gets the number of functions / parameters for a given basis node.
 * @see cmzn_elementbasis_get_number_of_nodes
 *
 * @param element_basis  Element basis to query.
 * @param basis_node_index  The basis node index starting at 1, up to number
 * used by basis.
 * @return  The number of functions/parameters, or 0 if invalid arguments.
 */
ZINC_API int cmzn_elementbasis_get_number_of_functions_per_node(
	cmzn_elementbasis_id element_basis, int basis_node_index);

#ifdef __cplusplus
}
#endif

#endif /* CMZN_ELEMENTBASIS_H__ */
