/**
 * FILE : fieldparametersprivate.hpp
 * 
 * Implementation of field parameters class.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (CMZN_FIELDPARAMETERSPRIVATE_HPP)
#define CMZN_FIELDPARAMETERSPRIVATE_HPP

#include "opencmiss/zinc/zincconfigure.h"
#include "opencmiss/zinc/types/elementid.h"
#include "opencmiss/zinc/types/fieldid.h"
#include "opencmiss/zinc/fieldparameters.h"
#include "finite_element/finite_element_constants.hpp"

class FE_field_parameters;
class FieldDerivative;

struct cmzn_fieldparameters
{
private:
	cmzn_field *field;  // accessed field owning these parameters, currently only finite element type allowed
	FE_field_parameters *feFieldParameters;  // accessed finite element field parameters
	FieldDerivative *fieldDerivatives[MAXIMUM_PARAMETER_DERIVATIVE_ORDER];  // first and second derivatives wrt parameters
	FE_mesh *meshes[MAXIMUM_ELEMENT_XI_DIMENSIONS];  // accessed meshes for which mixed derivatives are held
	// Following are initiall nullptr, but are accessed pointers once used so maintained for the life of the field parameters
	FieldDerivative *mixedFieldDerivatives[MAXIMUM_ELEMENT_XI_DIMENSIONS][MAXIMUM_MESH_DERIVATIVE_ORDER][MAXIMUM_PARAMETER_DERIVATIVE_ORDER];
	int access_count;

	cmzn_fieldparameters(cmzn_field *fieldIn, FE_field_parameters *feFieldParametersIn);

	~cmzn_fieldparameters();

public:

	/** Only to be called by cmzn_field.
	 * Checks fieldIn is real, general finite element type.
	 * @param fieldIn  Finite element type field
	 * @return  Accessed field parameters or nullptr if invalid field or failed. */
	static cmzn_fieldparameters *create(cmzn_field *fieldIn);

	cmzn_fieldparameters *access()
	{
		++access_count;
		return this;
	}

	static int deaccess(cmzn_fieldparameters* &fieldparameters);

	/** @return  Non-accessed field */
	cmzn_field *getField() const
	{
		return this->field;
	}

	/** Get array of global parameter indexes for field parameters in element.
	 * @param element  The element to query.
	 * @param valuesCount  Size of valuesOut >= number of parameters in element.
	 * @param valuesOut  Location to put parameter indexes.
	 * @param startIndex  Start index for returned parameters: 0 for zero-based indexes, 1 for one-based.
	 * @return Result OK on success, ERROR_NOT_FOUND if field not defined/no parameters on element,
	 * otherwise any other error code. */
	int getElementParameterIndexes(cmzn_element *element, int valuesCount, int *valuesOut, int startIndex);

	/** @return  Non-accessed field derivative w.r.t. parameters of given order or nullptr if invalid order */
	FieldDerivative *getFieldDerivative(int order) const
	{
		if ((order < 1) || (order > 2))
			return nullptr;
		return this->fieldDerivatives[order - 1];
	}

	/** @return  Non-accessed field derivative w.r.t. mesh and parameters
	 * The derivative is held by parameters until parameters object destroyed */
	FieldDerivative *getFieldDerivativeMixed(FE_mesh *mesh, int meshOrder, int parameterOrder);

	/** If parameter is node-based, return the node, field component, value label and version.
	 * @param parameterIndex  Zero-based parameter parameter index.
	 * @param fieldComponent  Return field component starting at 0 for node parameter, or -1 if not.
	 * @param valueLabel  Return value label for node parameter, or INVALID if not.
	 * @param version  Return version starting at 0 for node parameter, or -1 if not.
	 * @return  Non-accessed node if node parameter, or nullptr if not or failed.
	 */
	cmzn_node *getNodeParameter(int parameterIndex, int &fieldComponent, cmzn_node_value_label& valueLabel, int& version);

	/** @return  Number of parameters >=0, or -1 if error */
	int getNumberOfElementParameters(cmzn_element *element) const;

	/** @return  Total number of parameters */
	int getNumberOfParameters() const;

	/* Add incremental values to all field parameters.
	 * @param valuesCount  The size of the valuesIn array >= total number of parameters.
	 * @param valuesIn  Array containing increments to add, in index order.
	 * @return Result OK on success, or error code. */
	int addParameters(int valuesCount, const FE_value *valuesIn);

	/* Get values of all field parameters.
	 * @param valuesCount  The size of the valuesOut array >= total number of parameters.
	 * @param valuesOut  Array to fill with parameter values, in index order.
	 * @return Result OK on success, or error code. */
	int getParameters(int valuesCount, FE_value *valuesOut);

	/* Assign values to all field parameters.
	 * @param valuesCount  The size of the valuesIn array >= total number of parameters.
	 * @param valuesIn  Array containing new parameter values, in index order.
	 * @return Result OK on success, or error code. */
	int setParameters(int valuesCount, const FE_value *valuesIn);

	/** @return  Positive delta to apply when perturbing parameters to calculate numerical derivatives */
	FE_value getPerturbationDelta() const;

};

#endif /* !defined (CMZN_FIELDPARAMETERSPRIVATE_HPP) */
