/**
 * FILE : fieldparametersprivate.cpp
 * 
 * Implementation of field parameters class.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "opencmiss/zinc/field.h"
#include "opencmiss/zinc/fieldfiniteelement.h"
#include "opencmiss/zinc/result.h"
#include "computed_field/field_derivative.hpp"
#include "computed_field/fieldparametersprivate.hpp"
#include "computed_field/differential_operator.hpp"
#include "computed_field/computed_field_private.hpp"
#include "computed_field/computed_field_finite_element.h"
#include "finite_element/finite_element_field.hpp"
#include "finite_element/finite_element_field_parameters.hpp"
#include "general/debug.h"
#include "general/message.h"

cmzn_fieldparameters::cmzn_fieldparameters(cmzn_field *fieldIn, FE_field_parameters *feFieldParametersIn) :
	field(fieldIn->access()),
	feFieldParameters(feFieldParametersIn),  // take over access count
	access_count(1)
{
	for (int i = 0; i < MAXIMUM_PARAMETER_DERIVATIVE_ORDER; ++i)
		this->fieldDerivatives[i] = FieldDerivative::createParametersDerivative(this, (i > 0) ? this->fieldDerivatives[i - 1] : nullptr);
	for (int meshIndex = 0; meshIndex < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++meshIndex)
	{
		this->meshes[meshIndex] = nullptr;
		for (int mo = 0; mo < MAXIMUM_MESH_DERIVATIVE_ORDER; ++mo)
			for (int po = 0; po < MAXIMUM_PARAMETER_DERIVATIVE_ORDER; ++po)
				this->mixedFieldDerivatives[meshIndex][mo][po] = nullptr;
	}
}

cmzn_fieldparameters::~cmzn_fieldparameters()
{
	for (int meshIndex = 0; meshIndex < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++meshIndex)
		if (this->meshes[meshIndex])
		{
			for (int mo = 0; mo < MAXIMUM_MESH_DERIVATIVE_ORDER; ++mo)
				for (int po = 0; po < MAXIMUM_PARAMETER_DERIVATIVE_ORDER; ++po)
					if (this->mixedFieldDerivatives[meshIndex][mo][po])
					{
						this->mixedFieldDerivatives[meshIndex][mo][po]->clearOwnerPrivate();
						FieldDerivative::deaccess(this->mixedFieldDerivatives[meshIndex][mo][po]);
					}
			FE_mesh::deaccess(this->meshes[meshIndex]);
		}
	for (int i = 0; i < MAXIMUM_PARAMETER_DERIVATIVE_ORDER; ++i)
	{
		this->fieldDerivatives[i]->clearOwnerPrivate();
		FieldDerivative::deaccess(this->fieldDerivatives[i]);
	}
	FE_field_parameters::deaccess(this->feFieldParameters);
	this->field->clearFieldparameters();
	cmzn_field::deaccess(&(this->field));
}

cmzn_fieldparameters *cmzn_fieldparameters::create(cmzn_field *fieldIn)
{
	FE_field *feField = nullptr;
	if (Computed_field_get_type_finite_element(fieldIn, &feField))
	{
		// this requires a field is real, general type
		FE_field_parameters *feFieldParameters = feField->get_FE_field_parameters();
		if (feFieldParameters)
			return new cmzn_fieldparameters(fieldIn, feFieldParameters);
	}
	display_message(ERROR_MESSAGE, "Field getFieldparameters:  Can only get from finite element field");
	return nullptr;
}

int cmzn_fieldparameters::deaccess(cmzn_fieldparameters* &fieldparameters)
{
	if (!fieldparameters)
		return CMZN_RESULT_ERROR_ARGUMENT;
	--(fieldparameters->access_count);
	if (fieldparameters->access_count <= 0)
		delete fieldparameters;
	fieldparameters = 0;
	return CMZN_RESULT_OK;
}

int cmzn_fieldparameters::getElementParameterIndexes(cmzn_element *element, int valuesCount, int *valuesOut, int startIndex)
{
	return this->feFieldParameters->getElementParameterIndexes(element, valuesCount, valuesOut, startIndex);
}

FieldDerivative *cmzn_fieldparameters::getFieldDerivativeMixed(FE_mesh *mesh, int meshOrder, int parameterOrder)
{
	if ((!mesh) || (mesh->get_FE_region() != this->feFieldParameters->getField()->get_FE_region())
		|| (meshOrder < 1) || (meshOrder > MAXIMUM_MESH_DERIVATIVE_ORDER)
		|| ((parameterOrder < 1) || (parameterOrder > MAXIMUM_PARAMETER_DERIVATIVE_ORDER)))
	{
		display_message(ERROR_MESSAGE, "Fieldparameters getFieldDerivativeMixed:  Invalid arguments");
		return nullptr;
	}
	const int meshIndex = mesh->getDimension() - 1;
	if (!this->meshes[meshIndex])
		this->meshes[meshIndex] = mesh->access();  // so mesh exists while this holds mesh derivatives for it
	FieldDerivative *fieldDerivative = this->mixedFieldDerivatives[meshIndex][meshOrder - 1][parameterOrder - 1];
	if (!fieldDerivative)
	{
		FieldDerivative *lowerDerivative = (parameterOrder > 1) ?
			this->getFieldDerivativeMixed(mesh, meshOrder, parameterOrder - 1) : mesh->getFieldDerivative(meshOrder);
		if (!lowerDerivative)
			return nullptr;
		fieldDerivative = this->mixedFieldDerivatives[meshIndex][meshOrder - 1][parameterOrder - 1] =
			FieldDerivative::createParametersDerivative(this, lowerDerivative);
		if (!fieldDerivative)
			return nullptr;
	}
	return fieldDerivative;
}

cmzn_node *cmzn_fieldparameters::getNodeParameter(int parameterIndex, int &fieldComponent, cmzn_node_value_label& valueLabel, int& version)
{
	return this->feFieldParameters->getNodeParameter(parameterIndex, fieldComponent, valueLabel, version);
}

int cmzn_fieldparameters::getNumberOfElementParameters(cmzn_element *element) const
{
	return this->feFieldParameters->getNumberOfElementParameters(element);
}

int cmzn_fieldparameters::getNumberOfParameters() const
{
	return this->feFieldParameters->getNumberOfParameters();
}

int cmzn_fieldparameters::addParameters(int valuesCount, const FE_value *valuesIn)
{
	return this->feFieldParameters->addParameters(valuesCount, valuesIn);
}

int cmzn_fieldparameters::getParameters(int valuesCount, FE_value *valuesOut)
{
	return this->feFieldParameters->getParameters(valuesCount, valuesOut);
}

int cmzn_fieldparameters::setParameters(int valuesCount, const FE_value *valuesIn)
{
	return this->feFieldParameters->setParameters(valuesCount, valuesIn);
}

FE_value cmzn_fieldparameters::getPerturbationDelta() const
{
	return this->feFieldParameters->getPerturbationDelta();
}

/*
Global functions
----------------
*/

cmzn_fieldparameters_id cmzn_fieldparameters_access(cmzn_fieldparameters_id fieldparameters)
{
	if (fieldparameters)
		return fieldparameters->access();
	return 0;
}

int cmzn_fieldparameters_destroy(cmzn_fieldparameters_id *fieldparameters_address)
{
	if (!fieldparameters_address)
		return CMZN_RESULT_ERROR_ARGUMENT;
	return cmzn_fieldparameters::deaccess(*fieldparameters_address);
}

cmzn_differentialoperator_id cmzn_fieldparameters_get_derivative_operator(
	cmzn_fieldparameters_id fieldparameters, int order)
{
	if (fieldparameters)
	{
		FieldDerivative *fieldDerivative = fieldparameters->getFieldDerivative(order);
		if (fieldDerivative)
			return cmzn_differentialoperator::create(fieldDerivative, -1);
	}
	display_message(ERROR_MESSAGE, "Mesh getChartDifferentialoperator.  Invalid argument(s)");
	return nullptr;
}

int cmzn_fieldparameters_get_element_parameter_indexes(
	cmzn_fieldparameters_id fieldparameters, cmzn_element_id element,
	int valuesCount, int *valuesOut)
{
	if (fieldparameters)
		return fieldparameters->getElementParameterIndexes(element, valuesCount, valuesOut, 1);
	display_message(ERROR_MESSAGE, "Fieldparameters getElementParametersIndexes:  Invalid Fieldparameters");
	return CMZN_RESULT_ERROR_ARGUMENT;
}

int cmzn_fieldparameters_get_element_parameter_indexes_zero(
	cmzn_fieldparameters_id fieldparameters, cmzn_element_id element,
	int valuesCount, int *valuesOut)
{
	if (fieldparameters)
		return fieldparameters->getElementParameterIndexes(element, valuesCount, valuesOut, 0);
	display_message(ERROR_MESSAGE, "Fieldparameters getElementParameterIndexesZero:  Invalid Fieldparameters");
	return CMZN_RESULT_ERROR_ARGUMENT;
}

cmzn_field_id cmzn_fieldparameters_get_field(
	cmzn_fieldparameters_id fieldparameters)
{
	if (fieldparameters)
		return cmzn_field_access(fieldparameters->getField());
	display_message(ERROR_MESSAGE, "Fieldparameters getField:  Invalid field parameters object");
	return nullptr;
}

int cmzn_fieldparameters_get_number_of_element_parameters(
	cmzn_fieldparameters_id fieldparameters, cmzn_element_id element)
{
	if (fieldparameters)
		return fieldparameters->getNumberOfElementParameters(element);
	display_message(ERROR_MESSAGE, "Fieldparameters getNumberOfElementParameters:  Invalid Fieldparameters");
	return -1;
}

int cmzn_fieldparameters_get_number_of_parameters(
	cmzn_fieldparameters_id fieldparameters)
{
	if (fieldparameters)
		return fieldparameters->getNumberOfParameters();
	display_message(ERROR_MESSAGE, "Fieldparameters getNumberOfParameters:  Invalid Fieldparameters");
	return -1;
}

int cmzn_fieldparameters_add_parameters(
	cmzn_fieldparameters_id fieldparameters,
	int valuesCount, const double *valuesIn)
{
	if (fieldparameters)
		return fieldparameters->addParameters(valuesCount, valuesIn);
	display_message(ERROR_MESSAGE, "Fieldparameters addParameters:  Invalid Fieldparameters");
	return -1;
}

int cmzn_fieldparameters_get_parameters(
	cmzn_fieldparameters_id fieldparameters,
	int valuesCount, double *valuesOut)
{
	if (fieldparameters)
		return fieldparameters->getParameters(valuesCount, valuesOut);
	display_message(ERROR_MESSAGE, "Fieldparameters addParameters:  Invalid Fieldparameters");
	return -1;
}

int cmzn_fieldparameters_set_parameters(
	cmzn_fieldparameters_id fieldparameters,
	int valuesCount, const double *valuesIn)
{
	if (fieldparameters)
		return fieldparameters->setParameters(valuesCount, valuesIn);
	display_message(ERROR_MESSAGE, "Fieldparameters setParameters:  Invalid Fieldparameters");
	return -1;
}
