/**
 * FILE : finite_element_field_evaluation.hpp
 *
 * Class for caching values and evaluating fields over elements.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (FINITE_ELEMENT_FIELD_EVALUATION_HPP)
#define FINITE_ELEMENT_FIELD_EVALUATION_HPP

#include <opencmiss/zinc/zincconfigure.h>
#include "finite_element/finite_element_basis.hpp"
#include "finite_element/finite_element_constants.hpp"
#include "general/value.h"

/*
Global types
------------
*/

struct cmzn_element;
struct FE_field;

/**
 * Values and methods for interpolating a field over a single element.
 * Caches values mapped from nodes or other parameter storage in efficient
 * form for fast interpolation with basis, allowing derivative evaluation etc.
 * Expensive to compute the first time, but multiple evaluations in the
 * same element should result in savings in overall compute time.
 * Note for time-varying fields: values are valid at a single time only.
 * Client is responsible for recomputing if field parameters change, or
 * evaluating at a different element, time etc.
 */
class FE_element_field_evaluation
{
	friend class FeParameterPerturbation;

	// the FE_field these values are for
	FE_field *field;
	// the element these values are for
	cmzn_element *element;
	// the element the field was inherited from
	cmzn_element *field_element;
	// whether or not these values depend on time
	int time_dependent;
	// if the values are time dependent, the time at which they were calculated
	FE_value time;
	// number of sub-elements in each xi-direction of element for each component.
	// If NULL (for a component) then field is not grid based for that component.
	// Notes:
	// 1.  Grid sub-elements are linear in each direction.  This means that
	//     <component_number_of_values> is not used
	// 2.  the grid-point values are not blended (to monomial) and so
	//     <component_standard_basis_functions> and
	//     <component_standard_basis_function_arguments> are not used
	// 3.  for grid-based <destroy_standard_basis_arguments> is used to specify
	//     if the <component_values> should be destroyed (element field has been
	//     inherited)
	int **component_number_in_xi;
	// specify whether the standard basis arguments should be destroyed (element
	// field has been inherited) or not be destroyed (element field is defined for
	// the element and the basis arguments are being used)
	bool destroy_standard_basis_arguments;
	// the number of field components
	int number_of_components;
	// the number of values for each component
	int *component_number_of_values;
	// the values_storage for each component if grid-based
	const Value_storage **component_grid_values_storage;
	// grid_offset_in_xi is allocated with 2^number_of_xi_coordinates integers
	// giving the increment in index into the values stored with the top_level
	// element for the grid. For top_level_elements the first value is 1, the
	// second is (number_in_xi[0]+1), the third is
	// (number_in_xi[0]+1)*(number_in_xi[1]+1) etc. The base_grid_offset is 0 for
	// top_level_elements. For faces and lines these values are adjusted to get
	// the correct index for the top_level_element
	int *component_base_grid_offset,**component_grid_offset_in_xi;
	// following allocated with 2^number_of_xi for grid-based fields for use in
	// calculate_FE_element_field
	int *element_value_offsets;
	// the values for each component, to dot product with standard basis
	FE_value **component_values;
	// the mapping and basis used for general field components in top-level element only
	// non-accessed; concurrency risk if not following rules of single thread only when modifying
	const FE_element_field_template **component_efts; // set only for general field in top-level element
	// scale factors cached for components in current element or nullptr if none
	// Note: components with the same efts share pointers to the same scale factors
	const FE_value **component_scale_factors;
	// the standard basis function for each component
	Standard_basis_function **component_standard_basis_functions;
	// the arguments for the standard basis function for each component
	int **component_standard_basis_function_arguments;
	// working space for evaluating basis, for grid-based only
	Standard_basis_function_evaluation grid_basis_function_evaluation;
	FE_value last_grid_xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	// parameter perturbation for computing approximate parameter derivatives
	// of other fields by finite difference.
	// the number of active perturbations <= MAXIMUM_PARAMETER_DERIVATIVE_ORDER
	int parameterPerturbationCount;
	// which parameter indexes are perturbed up to parameterPerturbationCount
	int parameterPerturbationIndex[MAXIMUM_PARAMETER_DERIVATIVE_ORDER];
	// size of each perturbation for value -> value + delta*derivative
	FE_value parameterPerturbationDelta[MAXIMUM_PARAMETER_DERIVATIVE_ORDER];
	int access_count;

	FE_element_field_evaluation();

	~FE_element_field_evaluation()
	{
		this->clear();
	}

public:
	/** create on heap with access_count = 1 */
	static FE_element_field_evaluation *create()
	{
		return new FE_element_field_evaluation();
	}

	inline FE_element_field_evaluation *access()
	{
		++access_count;
		return this;
	}

	static int deaccess(FE_element_field_evaluation* &element_field_evaluation);

	/** Free the memory for the fields of the evaluation object.
	 * Restores it to the blank state it was created with.
	 * Must be called before calling calculate_values() again. */
	void clear();

	/** Determine if existing values are valid to use for element and time.
	 * @param time_in  If values are time dependent, match against this time.
	 * @param field_element_in  Optional field element to inherit from; 0 to ignore
	 */
	bool is_for_element_and_time(cmzn_element *element_in, FE_value time_in, cmzn_element *field_element_in) const
	{
		return (element_in == this->element)
			&& ((!field_element_in) || (field_element_in = this->field_element))
			&& ((!this->time_dependent) || (time_in == this->time));
	}

	static inline int get_number_of_mesh_derivatives(int derivative_order, int dimension)
	{
		int number_of_derivatives = 1;
		for (int d = 0; d < derivative_order; ++d)
			number_of_derivatives *= dimension;
		return number_of_derivatives;
	}

	/** Fill the evaluation structure, ready to evaluate field.
	 * Must be freshly created or cleared.
	 * @param topLevelElement  Optional element to inherit field from.
	 */
	int calculate_values(FE_field *field, cmzn_element *element,
		FE_value time, cmzn_element *top_level_element = 0);

	/** Evaluate integer field values in element.
	 * @param component_number  Component number to evaluate starting at 0, or any
	 * other value to evaluate all components.
	 * @param xi_coordinates  Element chart location to evaluate at.
	 * @param values  Caller-supplied space to store the int values. */
	int evaluate_int(int component_number, const FE_value *xi_coordinates,
		int *values);

	/** Evaluate real field/component values in element. Converts other numerical
	 * types to real. Must have called calculate_values first.
	 * @param component_number  Component number to evaluate starting at 0, or any
	 * other value to evaluate all components.
	 * @param xi_coordinates  Element chart location to evaluate at.
	 * @param basis_function_evaluation  Standard basis function evaluation cache.
	 * @param mesh_derivative_order  Derivative order w.r.t. mesh chart starting
	 * at 0 for values, 1 for first derivatives etc.
	 * @param parameter_derivative_order  Derivative order w.r.t. parameters for
	 * field, either 0 for none, 1 for first order. As finite element field is a
	 * linear function of parameters, higher derivatives are zero.
	 * @param values  Caller-supplied space to store the real values. Length
	 * must be at least N components * dimension^derivative_order. Derivative
	 * values vary fastest with the first xi index; values for each component
	 * are consecutive. */
	int evaluate_real(int component_number, const FE_value *xi_coordinates,
		Standard_basis_function_evaluation &basis_function_evaluation,
		int mesh_derivative_order, int parameter_derivative_order, FE_value *values);

	/** Returns allocated copies of the string values of the field in the element.
	 * @param component_number  Component number to evaluate starting at 0, or any
	 * other value to evaluate all components.
	 * @param xi_coordinates  Element chart location to evaluate at.
	 * @param values  Caller-supplied space to store the allocated string/s.
	 * It is up to the caller to deallocate the returned strings. */
	int evaluate_string(int component_number, const FE_value *xi_coordinates,
		char **values);

	/** Calculate the values of element field in element, convert to and
	 * return as an allocated string. Derivatives are not included in string.
	 * @param component_number  Component number to evaluate starting at 0, or any
	 * other value to evaluate all components.
	 * @param xi_coordinates  Element chart location to evaluate at.
	 * @param basis_function_evaluation.  Standard basis function evaluation cache.
	 * @param out_string  Pointer to address to store allocated string pointer.
	 * It is up to the caller to deallocate the returned string. */
	int evaluate_as_string(int component_number, const FE_value *xi_coordinates,
		Standard_basis_function_evaluation &basis_function_evaluation, char **out_string);

	/** Allocates and returns component count and values for component_number.
	 * It is up to the caller to deallocate any returned component values. */
	int get_component_values(int component_number,
		int *number_of_component_values_address, FE_value **component_values_address) const;

	/** If component_number is monomial, integer values describing the monomial
	 * basis are returned. The first number is the dimension, the following
	 * numbers are the order of the monomial in each direction, e.g. 3=cubic.
	 * @param monomial_info  Must point to a block of memory big enough to take
	 * 1 + MAXIMUM_ELEMENT_XI_DIMENSIONS integers. */
	int get_monomial_component_info(int component_number, int *monomial_info) const;

	/* Add a perturbation to an element field parameter so the field values and
	 * mesh derivatives have delta*parameterDerivative[parameterIndex] added.
	 * Used when calculating approximate derivatives of other fields by finite
	 * difference. See: Computed_field_core::evaluateDerivativeFiniteDifference.
	 * Values must have been calculated for a general field on a top-level element
	 * for this to work. Up to MAXIMUM_PARAMETER_DERIVATIVE_ORDER perturbations
	 * may be active at any time.
	 * Must call removeParameterPerturbation to clear.
	 * @param parameterIndex  Valid parameter index for current element field,
	 * starting at 0.
	 * @param parameterDelta  The change in value of the parameter to apply,
	 * positive or negative. Must be appropriately size for parameter range over
	 * element.
	 * @return  true on success, false on failure e.g. index out of range */
	bool addParameterPerturbation(int parameterIndex, FE_value parameterDelta);

	/** Remove the last perturbation added with addParameterPerturbation.
	 * It is important to call this even alternative return paths and exceptions */
	void removeParameterPerturbation();

};

#endif /* !defined (FINITE_ELEMENT_FIELD_EVALUATION_HPP) */
