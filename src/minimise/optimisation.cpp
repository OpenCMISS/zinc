/***************************************************************************//**
 * @file optimisation.cpp
 *
 * Implementation of Minimisation object for performing optimisation algorithm
 * from description in cmzn_optimisation.
 *
 * @see-also api/zinc/optimisation.h
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <math.h>
#include "opencmiss/zinc/field.h"
#include "opencmiss/zinc/fieldmodule.h"
#include "opencmiss/zinc/core.h"
#include "opencmiss/zinc/node.h"
#include "opencmiss/zinc/nodeset.h"
#include "opencmiss/zinc/field.hpp"
#include "opencmiss/zinc/fieldcache.hpp"
#include "opencmiss/zinc/fieldarithmeticoperators.hpp"
#include "opencmiss/zinc/fieldcomposite.hpp"
#include "opencmiss/zinc/fieldmodule.hpp"
#include "opencmiss/zinc/fieldparameters.hpp"
#include "opencmiss/zinc/fieldvectoroperators.hpp"
#include "opencmiss/zinc/node.hpp"
#include "opencmiss/zinc/nodeset.hpp"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_composite.h"
#include "computed_field/computed_field_set.h"
#include "computed_field/computed_field_finite_element.h"
#include "computed_field/fieldassignmentprivate.hpp"
#include "computed_field/fieldparametersprivate.hpp"
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_private.h"
#include "finite_element/finite_element_region.h"
#include "general/callback_private.h"
#include "general/compare.h"
#include "general/debug.h"
#include "general/indexed_list_private.h"
#include "general/object.h"
#include "mesh/cmiss_node_private.hpp"
#include "time/time_keeper.hpp"
#include "general/message.h"
#include "computed_field/computed_field_private.hpp"
#include "minimise/optimisation.hpp"
#include "general/enumerator_private.hpp"
#include "mesh/cmiss_element_private.hpp"
#include "computed_field/field_module.hpp"
#include <iostream>
#include <sstream>
#include <vector>
using namespace std;

// OPT++ includes and namespaces
#include <LSQNLF.h>
#include <NLF.h>
#include <NLP.h>
#include <OptQNewton.h>
#include <OptNewton.h>

using NEWMAT::ColumnVector;
using namespace ::OPTPP;
using namespace OpenCMISS::Zinc;

// global variable needed to pass minimisation object to Opt++ init functions.
static void* GlobalVariableMinimisation = NULL;

int ObjectiveFieldData::prepareTerms()
{
	cmzn_fieldmodule_id field_module = cmzn_field_get_fieldmodule(field);
	cmzn_fieldcache_id field_cache = cmzn_fieldmodule_create_fieldcache(field_module);
	numTerms = field->get_number_of_sum_square_terms(*field_cache);
	cmzn_fieldcache_destroy(&field_cache);
	cmzn_fieldmodule_destroy(&field_module);
	bufferSize = numComponents;
	if (numTerms > 0)
		bufferSize *= numTerms;
	buffer = new FE_value[bufferSize];
	return (0 != buffer);
}

Minimisation::~Minimisation()
{
	delete[] objectiveValues;
	if (dof_storage_array) DEALLOCATE(dof_storage_array);
	if (dof_initial_values) DEALLOCATE(dof_initial_values);
	cmzn_fieldcache_destroy(&field_cache);
	cmzn_fieldmodule_destroy(&field_module);
	for (ObjectiveFieldDataVector::iterator iter = objectiveFields.begin();
		iter != objectiveFields.end(); ++iter)
	{
		delete *iter;
	}
}

int Minimisation::prepareOptimisation()
{
	int return_code = CMZN_OK;
	cmzn_fieldmodule_begin_change(field_module);
	if (optimisation.objectiveFields.size() != objectiveFields.size())
		return_code = CMZN_ERROR_ARGUMENT;
	if ((return_code == CMZN_OK) && (CMZN_OK != construct_dof_arrays()))
		return_code = CMZN_ERROR_GENERAL;
	if (optimisation.method == CMZN_OPTIMISATION_METHOD_LEAST_SQUARES_QUASI_NEWTON)
	{
		totalLeastSquaresTerms = 0;
		for (ObjectiveFieldDataVector::iterator iter = objectiveFields.begin();
			iter != objectiveFields.end(); ++iter)
		{
			ObjectiveFieldData *objective = *iter;
			if (!objective->prepareTerms())
			{
				return_code = CMZN_ERROR_GENERAL;
				break;
			}
			totalLeastSquaresTerms += objective->bufferSize;
		}
	}
	if (return_code != CMZN_OK)
	{
		display_message(ERROR_MESSAGE, "Minimisation::prepareOptimisation() Failed");
	}
	cmzn_fieldmodule_end_change(field_module);
	return return_code;
}

/**
 * Ensures dependent fields are marked as changed, so graphics update.
 * Only needed by QUASI_NEWTON and LEAST_SQUARES_QUASI_NEWTON which
 * use unofficial methods to modify parameters at this time.
 */
void Minimisation::touch_dependent_fields()
{
	DependentAndConditionalFieldsList::iterator iter;
	for (iter = optimisation.dependentFields.begin();
		iter != optimisation.dependentFields.end(); ++iter)
	{
		iter->dependentField->setChanged();
	}
}

int Minimisation::runOptimisation()
{
	// Minimise the objective function
	int return_code = 0;
	switch (this->optimisation.getMethod())
	{
	case CMZN_OPTIMISATION_METHOD_QUASI_NEWTON:
		return_code = minimise_QN();
		touch_dependent_fields();
		this->do_fieldassignments();
		break;
	case CMZN_OPTIMISATION_METHOD_LEAST_SQUARES_QUASI_NEWTON:
		return_code = minimise_LSQN();
		touch_dependent_fields();
		this->do_fieldassignments();
		break;
	case CMZN_OPTIMISATION_METHOD_NEWTON:
		return_code = minimise_Newton();
		break;
	default:
		display_message(ERROR_MESSAGE, "cmzn_optimisation::runOptimisation. "
			"Unknown minimisation method.");
		break;
	}
	if (!return_code)
	{
		display_message(ERROR_MESSAGE, "Minimisation::runOptimisation() Failed");
		return CMZN_ERROR_GENERAL;
	}
	return CMZN_OK;
}

/***************************************************************************//**
 *  Populates the array of pointers to the dof values and the array of the dof
 *  initial values. Notice the population includes both nodal values and
 *  derivatives but does NOT handle versions - this needs to be added by someone
 *  who understands and can test versions.
 */
int Minimisation::construct_dof_arrays()
{
	int return_code = 1;
	if (dof_storage_array)
	{
		DEALLOCATE(dof_storage_array);
		dof_storage_array = 0;
	}
	if (dof_initial_values)
	{
		DEALLOCATE(dof_initial_values);
		dof_initial_values = 0;
	}
	this->total_dof = 0;
	DependentAndConditionalFieldsList::iterator iter;
	for (iter = optimisation.dependentFields.begin();
		iter != optimisation.dependentFields.end(); ++iter)
	{
		cmzn_field *dependentField = iter->dependentField;
		const int componentCount = cmzn_field_get_number_of_components(dependentField);
		cmzn_fieldcache_id cache = 0;
		cmzn_field *conditionalField = iter->conditionalField;
		FE_value *conditionalValues = 0;
		int conditionalComponents = 0;
		if (conditionalField)
		{
			conditionalComponents = cmzn_field_get_number_of_components(conditionalField);
			conditionalValues = new FE_value[conditionalComponents];
			cache = cmzn_fieldmodule_create_fieldcache(this->field_module);
		}
		if (Computed_field_is_type_finite_element(dependentField))
		{
			// should only have one dependent field
			FE_field *fe_field;
			Computed_field_get_type_finite_element(dependentField, &fe_field);
			cmzn_nodeset_id nodeset = cmzn_fieldmodule_find_nodeset_by_field_domain_type(field_module, CMZN_FIELD_DOMAIN_TYPE_NODES);
			cmzn_nodeiterator_id iterator = cmzn_nodeset_create_nodeiterator(nodeset);
			cmzn_node_id node = 0;
			while ((0 != (node = cmzn_nodeiterator_next_non_access(iterator))) && return_code)
			{
				if (conditionalField)
				{
					cmzn_fieldcache_set_node(cache, node);
					int result = cmzn_field_evaluate_real(conditionalField, cache, conditionalComponents, conditionalValues);
					if (result != CMZN_OK)
						continue; // conditionalField not defined => skip
					if ((1 == conditionalComponents) && (conditionalValues[0] == 0.0))
						continue; // scalar conditional field is zero => skip
				}
				const FE_node_field *node_field = node->getNodeField(fe_field);
				if (node_field)
				{
					for (int c = 0; c < componentCount; ++c)
					{
						if ((conditionalComponents > 1) && (conditionalValues[c] == 0.0))
							continue;
						const FE_node_field_template *nft = node_field->getComponent(c);
						const int totalValuesCount = nft->getTotalValuesCount();
						if (totalValuesCount == 0)
						{
							continue;
						}
						FE_value **temp_dof_storage_array;
						if (REALLOCATE(temp_dof_storage_array, this->dof_storage_array, FE_value *, this->total_dof + totalValuesCount))
						{
							this->dof_storage_array = temp_dof_storage_array;
						}
						else
						{
							return_code = 0;
							break;
						}
						FE_value *temp_dof_initial_values;
						if (REALLOCATE(temp_dof_initial_values, this->dof_initial_values, FE_value, this->total_dof + totalValuesCount))
						{
							this->dof_initial_values = temp_dof_initial_values;
						}
						else
						{
							return_code = 0;
							break;
						}
						const int valueLabelsCount = nft->getValueLabelsCount();
						for (int d = 0; (d < valueLabelsCount) && return_code; ++d)
						{
							cmzn_node_value_label valueLabel = nft->getValueLabelAtIndex(d);
							const int versionsCount = nft->getVersionsCountAtIndex(d);
							for (int v = 0; v < versionsCount; ++v)
							{
								if (CMZN_OK == get_FE_nodal_FE_value_storage(node, fe_field, c,
									valueLabel, v, current_time, &(this->dof_storage_array[total_dof])))
								{
									// get initial value from value storage pointer
									this->dof_initial_values[total_dof] = *(this->dof_storage_array[total_dof]);
									/*cout << dof_storage_array[total_dof - 1] << "   "
									<< dof_initial_values[total_dof - 1] << endl;*/
									++(this->total_dof);
								}
								else
								{
									display_message(ERROR_MESSAGE, "cmzn_optimisation::construct_dof_arrays. "
										"get_FE_nodal_FE_value_storage failed.");
									return_code = 0;
									break;
								}
							}
						}
					}
				}
			}
			cmzn_nodeiterator_destroy(&iterator);
			cmzn_nodeset_destroy(&nodeset);
		}
		else if (Computed_field_is_constant(dependentField))
		{
			FE_value *constant_values_storage = Computed_field_constant_get_values_storage(dependentField);
			if (constant_values_storage)
			{
				FE_value **temp_dof_storage_array;
				if (REALLOCATE(temp_dof_storage_array, this->dof_storage_array, FE_value *, this->total_dof + componentCount))
				{
					this->dof_storage_array = temp_dof_storage_array;
				}
				else
				{
					return_code = 0;
				}
				FE_value *temp_dof_initial_values;
				if (REALLOCATE(temp_dof_initial_values, this->dof_initial_values, FE_value, this->total_dof + componentCount))
				{
					this->dof_initial_values = temp_dof_initial_values;
				}
				else
				{
					return_code = 0;
				}
				if (return_code)
				{
					for (int c = 0; c < componentCount; ++c)
					{
						this->dof_storage_array[total_dof] = constant_values_storage + c;
						this->dof_initial_values[total_dof] = *(this->dof_storage_array[total_dof]);
						/*cout << dof_storage_array[total_dof] << "   "
								<< dof_initial_values[total_dof] << endl;*/
						++(this->total_dof);
					}
				}
			}
			else
			{
				char *field_name = cmzn_field_get_name(dependentField);
				display_message(WARNING_MESSAGE, "Minimisation::construct_dof_arrays.  "
					"Dependent field '%s' is not a constant. Skipping.", field_name);
				DEALLOCATE(field_name);
				return_code = 0;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE, "cmzn_optimisation::construct_dof_arrays. "
				"Invalid dependent field type.");
			return_code = 0;
		}
		delete[] conditionalValues;
		cmzn_fieldcache_destroy(&cache);
	}
	return return_code;
}

/***************************************************************************//**
 * Simple function to list the dof values. This is mainly for debugging purposes
 * and may be removed later.
 */
void Minimisation::list_dof_values()
{
	for (int i = 0; i < total_dof; i++) {
		cout << "dof[" << i << "] = " << *dof_storage_array[i] << endl;
	}
}

/**
 * Must call this function after updating dependent field DOFs to ensure
 * dependent field caches are fully recalculated with the DOF changes.
 * Only needed by QUASI_NEWTON and LEAST_SQUARES_QUASI_NEWTON which
 * use unofficial methods to modify parameters at this time.
 */
void Minimisation::invalidate_dependent_field_caches()
{
	for (DependentAndConditionalFieldsList::iterator iter = optimisation.dependentFields.begin();
		iter != optimisation.dependentFields.end(); ++iter)
	{
		iter->dependentField->clearCaches();
	}
}

// Perform any field assignments; called before evaluating objective function(s)
// but after dependent DOFs have been set. Also call at completion of optimisation.
void Minimisation::do_fieldassignments()
{
	for (auto iter = this->optimisation.fieldassignments.begin(); iter != this->optimisation.fieldassignments.end(); ++iter)
	{
		(*iter)->assign();
	}
}

/***************************************************************************//**
 * Evaluates the scalar objective function value given the current DOF values.
 * Equals sum of all objective field components.
 */
int Minimisation::evaluate_objective_function(FE_value *valueAddress)
{
	int return_code = 1;
	*valueAddress = 0.0;
	invalidate_dependent_field_caches();
	int offset = 0;
	for (ObjectiveFieldDataVector::iterator iter = objectiveFields.begin();
		iter != objectiveFields.end(); ++iter)
	{
		ObjectiveFieldData *objective = *iter;
		if (CMZN_OK != cmzn_field_evaluate_real(objective->field, field_cache,
			objective->numComponents, objectiveValues + offset))
		{
			display_message(ERROR_MESSAGE, "Failed to evaluate objective field %s", objective->field->name);
			return_code = 0;
			break;
		}
		offset += objective->numComponents;
	}
	for (int i = 0; i < totalObjectiveFieldComponents; ++i)
	{
		*valueAddress += objectiveValues[i];
	}
	return return_code;
}

/***************************************************************************//**
 * One time initialisation code required by the Opt++ quasi-Newton and least-
 * squares quasi-Newton minimisation algorithms.
 * Copies initial DOF values.
 */
static void init_dof_initial_values(int ndim, ColumnVector& x)
{
	int i;
	Minimisation* minimisation = static_cast<Minimisation*> (GlobalVariableMinimisation);
	FE_value *dof_initial_values = minimisation->get_dof_initial_values();
	for (i = 0; i < ndim; i++)
	{
		x(i+1) = static_cast<double>(dof_initial_values[i]);
	}
}

/***************************************************************************//**
 * The objective function for the Opt++ quasi-Newton minimisation.
 */
void objective_function_QN(int ndim, const ColumnVector& x, double& fx,
		int& result)
{
	int i;
	Minimisation* minimisation = static_cast<Minimisation*> (GlobalVariableMinimisation);
	// ColumnVector's index'd from 1...
	for (i = 0; i < ndim; i++)
	{
		minimisation->set_dof_value(i, x(i + 1));
	}
	//minimisation->list_dof_values();
	minimisation->do_fieldassignments();

	FE_value objectiveFunctionValue = 0.0;
	minimisation->evaluate_objective_function(&objectiveFunctionValue);
	fx = static_cast<double>(objectiveFunctionValue);
	//cout << "Objective Value = " << fx << endl;
	result = NLPFunction;
}

/**
 * Naive wrapper around a quasi-Newton minimisation.
 */
int Minimisation::minimise_QN()
{
	cmzn_fieldmodule_begin_change(field_module);
	char message[] = { "Solution from quasi-newton" };
	// need a handle on this object...
	// FIXME: need to find and use "user data" in the Opt++ methods.
	GlobalVariableMinimisation = static_cast<void*> (this);

	FDNLF1 nlp(total_dof, objective_function_QN, init_dof_initial_values);
	OptQNewton objfcn(&nlp);
	objfcn.setSearchStrategy(LineSearch);
	objfcn.setFcnTol(optimisation.functionTolerance);
	objfcn.setGradTol(optimisation.gradientTolerance);
	objfcn.setStepTol(optimisation.stepTolerance);
	objfcn.setMaxIter(optimisation.maximumIterations);
	objfcn.setMaxFeval(optimisation.maximumNumberFunctionEvaluations);
	objfcn.setMaxStep(optimisation.maximumStep);
	objfcn.setMinStep(optimisation.minimumStep);
	objfcn.setLineSearchTol(optimisation.linesearchTolerance);
	objfcn.setMaxBacktrackIter(optimisation.maximumBacktrackIterations);
	/* @todo Add in support for trust region methods
	objfcn.setTRSize(optimisation.trustRegionSize);
	*/

	// send Opt++ log text to string buffer
	if (!objfcn.setOutputFile(optppMessageStream))
		cerr << "main: output file open failed" << endl;
	objfcn.optimize();
	objfcn.printStatus(message);
	objfcn.cleanup();

	ColumnVector solution = nlp.getXc();
	int i;
	for (i = 0; i < total_dof; i++)
		this->set_dof_value(i, solution(i + 1));
	//list_dof_values();
	cmzn_fieldmodule_end_change(field_module);
	return 1;
}

/***************************************************************************//**
 * The objective function for the Opt++ least-squares quasi-Newton minimisation.
 */
void objective_function_LSQ(int ndim, const ColumnVector& x, ColumnVector& fx,
		int& result, void* iterationCounterVoid)
{
	//int* iterationCounter = static_cast<int*>(iterationCounterVoid);
	//std::cout << "objective function called " << ++(*iterationCounter) << " times." << std::endl;
	USE_PARAMETER(iterationCounterVoid);
	int i;
	Minimisation* minimisation = static_cast<Minimisation*> (GlobalVariableMinimisation);
	// ColumnVector's index'd from 1...
	for (i = 0; i < ndim; i++)
	{
		minimisation->set_dof_value(i, x(i + 1));
	}
	//minimisation->list_dof_values();
	minimisation->invalidate_dependent_field_caches();
	minimisation->do_fieldassignments();

	int return_code = 1;
	// NEWMAT::ColumnVector::element(int m) is 0-based, not 1 as are other interfaces
	int termIndex = 0;
	for (ObjectiveFieldDataVector::iterator iter = minimisation->objectiveFields.begin();
		iter != minimisation->objectiveFields.end(); ++iter)
	{
		ObjectiveFieldData *objective = *iter;
		const int bufferSize = objective->bufferSize;
		FE_value *buffer = objective->buffer;
		if (objective->numTerms > 0)
			return_code = objective->field->evaluate_sum_square_terms(*(minimisation->field_cache), bufferSize, buffer);
		else
			return_code = (CMZN_OK == cmzn_field_evaluate_real(objective->field, minimisation->field_cache, objective->bufferSize, objective->buffer));
		if (!return_code)
		{
			// GRC: should record failure properly
			display_message(ERROR_MESSAGE, "Failed to evaluate least squares terms for objective field %s", objective->field->name);
			break;
		}
		for (i = 0; i < bufferSize; ++i)
			fx.element(termIndex++) = buffer[i];
	}
	result = NLPFunction;
}

/**
 * Least-Squares Quasi-Newton minimisation using Opt++
 */
int Minimisation::minimise_LSQN()
{
	cmzn_fieldmodule_begin_change(field_module);
	int iterationCounter = 0;
	char message[] = { "Solution from newton least squares" };
	// need a handle on this object...
	GlobalVariableMinimisation = static_cast<void*> (this);
	LSQNLF nlp(total_dof, totalLeastSquaresTerms,
		objective_function_LSQ, init_dof_initial_values, (OPTPP::INITCONFCN)NULL,
		(void*)(&iterationCounter));
	OptNewton objfcn(&nlp);
	objfcn.setSearchStrategy(LineSearch);
	// send Opt++ log text to string buffer
	if (!objfcn.setOutputFile(optppMessageStream))
		cerr << "main: output file open failed" << endl;
	objfcn.setFcnTol(optimisation.functionTolerance);
	objfcn.setGradTol(optimisation.gradientTolerance);
	objfcn.setStepTol(optimisation.stepTolerance);
	objfcn.setMaxIter(optimisation.maximumIterations);
	objfcn.setMaxFeval(optimisation.maximumNumberFunctionEvaluations);
	objfcn.setMaxStep(optimisation.maximumStep);
	objfcn.setMinStep(optimisation.minimumStep);
	objfcn.setLineSearchTol(optimisation.linesearchTolerance);
	objfcn.setMaxBacktrackIter(optimisation.maximumBacktrackIterations);
	/* @todo Add in support for trust region methods
	objfcn.setTRSize(optimisation.trustRegionSize);
	*/
	//nlp.setIsExpensive(false);
	//nlp.setDebug();
	objfcn.optimize();
	objfcn.printStatus(message);
	ColumnVector solution = nlp.getXc();
	int i;
	for (i = 0; i < total_dof; i++)
		this->set_dof_value(i, solution(i + 1));
	//list_dof_values();
	cmzn_fieldmodule_end_change(field_module);
	return 1;
}

/**
 * Newton minimisation directly using Zinc field parameter derivatives and Newmat.
 */
int Minimisation::minimise_Newton()
{
	if (this->optimisation.dependentFields.size() != 1)
	{
		display_message(ERROR_MESSAGE, "Optimisation optimise NEWTON:  Newton method only works with one dependent field");
		return 0;
	}
	Fieldmodule fieldmodule(cmzn_fieldmodule_access(this->field_module));
	Field dependentField(cmzn_field_access(this->optimisation.dependentFields.front().dependentField));
	Fieldparameters fieldparameters = dependentField.getFieldparameters();
	if (!fieldparameters.isValid())
	{
		display_message(ERROR_MESSAGE, "Optimisation optimise NEWTON:  Could not get fieldparameters for dependent field which must be finite element type");
		return 0;
	}
	const int globalParameterCount = fieldparameters.getNumberOfParameters();
	display_message(INFORMATION_MESSAGE, "Optimisation optimise NEWTON:  Parameters count %d\n", globalParameterCount);

	Mesh mesh;
	// use highest dimension mesh with elements in it
	for (int d = 3; d >= 1; --d)
	{
		mesh = fieldmodule.findMeshByDimension(d);
		if (mesh.getSize() > 0)
			break;
	}

	fieldmodule.beginChange();
	// get scalar objective field, summing all objective field components if needed
	Field objectiveField;
	if (this->objectiveFields.size() == 1)
	{
		ObjectiveFieldData *objective = *(this->objectiveFields.begin());
		objectiveField = Field(cmzn_field_access(objective->field));
		if (objectiveField.getNumberOfComponents() > 1)
			objectiveField = fieldmodule.createFieldSumComponents(objectiveField);
	}
	else
	{
		bool allScalar = true;
		std::vector<Field> fields;
		for (ObjectiveFieldDataVector::iterator iter = this->objectiveFields.begin();
			iter != this->objectiveFields.end(); ++iter)
		{
			ObjectiveFieldData *objective = *iter;
			objectiveField = Field(cmzn_field_access(objective->field));
			if (objectiveField.getNumberOfComponents() > 1)
				allScalar = false;
			fields.push_back(objectiveField);
		}
		if (allScalar && (fields.size() == 2))
			objectiveField = fieldmodule.createFieldAdd(fields[0], fields[1]);
		else
			objectiveField = fieldmodule.createFieldSumComponents(fieldmodule.createFieldConcatenate(static_cast<int>(fields.size()), fields.data()));
		fields.clear();
	}
	fieldmodule.endChange();
	if (!objectiveField.isValid())
	{
		display_message(ERROR_MESSAGE, "Optimisation optimise NEWTON:  Failed to get scalar objective field");
		return 0;
	}

	Differentialoperator parameterDerivative1 = fieldparameters.getDerivativeOperator(/*order*/1);
	Differentialoperator parameterDerivative2 = fieldparameters.getDerivativeOperator(/*order*/2);

	NEWMAT::ColumnVector globalJacobian(globalParameterCount);
	globalJacobian = 0.0;
	NEWMAT::SquareMatrix globalHessian(globalParameterCount);
	globalHessian = 0.0;
	std::vector<unsigned char> parameterUsed(globalParameterCount, 0);  // set to 1 if parameter used in element

	Fieldcache fieldcache = fieldmodule.createFieldcache();
	std::vector<int> elementParameterIndexes;  // grows to fit maximum elementParametersCount
	std::vector<double> elementJacobian;  // grows to fit maximum elementParametersCount
	std::vector<double> elementHessian;  // grows to fit maximum elementParametersCount*elementParametersCount
	Element element;
	Elementiterator iter = mesh.createElementiterator();
	int return_code = 1;
	const int elementCount = mesh.getSize();
	int elementIndex = 0;
	while ((element = iter.next()).isValid())
	{
		++elementIndex;
		display_message(INFORMATION_MESSAGE, "Element %d (%d/%d)\n", element.getIdentifier(), elementIndex, elementCount);
		fieldcache.setElement(element);
		const int elementParametersCount = fieldparameters.getNumberOfElementParameters(element);
		if (elementParametersCount <= 0)
			continue;  // GRC handle -1 error?
		if (elementParametersCount > elementParameterIndexes.size())
		{
			elementParameterIndexes.resize(elementParametersCount);
			elementJacobian.resize(elementParametersCount);
			elementHessian.resize(elementParametersCount*elementParametersCount);
		}
		fieldparameters.getElementParameterIndexes(element, elementParametersCount, elementParameterIndexes.data());

		const int result1 = objectiveField.evaluateDerivative(parameterDerivative1, fieldcache, elementParametersCount, elementJacobian.data());
		if (result1 != CMZN_OK)
		{
			display_message(ERROR_MESSAGE, "Optimisation optimise NEWTON:  Failed to evaluate element Jacobian");
			return_code = 0;
		}
		const int result2 = objectiveField.evaluateDerivative(parameterDerivative2, fieldcache, elementParametersCount*elementParametersCount, elementHessian.data());
		if (result2 != CMZN_OK)
		{
			display_message(ERROR_MESSAGE, "Optimisation optimise NEWTON:  Failed to evaluate element Hessian");
			return_code = 0;
		}

		// assemble
		const double *elementHessianRow = elementHessian.data();
		for (int i = 0; i < elementParametersCount; ++i)
		{
			const int row = elementParameterIndexes[i];
			parameterUsed[row - 1] = 1;
			globalJacobian(row) -= elementJacobian[i];
			for (int j = 0; j < elementParametersCount; ++j)
				globalHessian(row, elementParameterIndexes[j]) += elementHessianRow[j];
			elementHessianRow += elementParametersCount;
		}
	}
	if (!return_code)
		return return_code;

	// need internal class to access some query API which is not yet publicly exposed
	cmzn_fieldparameters *fieldparametersInternal = fieldparameters.getId();
	for (int i = 0; i < globalParameterCount; ++i)
	{
		if (!parameterUsed[i])
		{
			const int row = i + 1;
			globalHessian(row, row) = 1.0;
			// warn which parameter is eliminated
			cmzn_node_value_label valueLabel;
			int fieldComponent, version;
			cmzn_node *node = fieldparametersInternal->getNodeParameter(i, fieldComponent, valueLabel, version);
			display_message(WARNING_MESSAGE, "Optimisation optimise NEWTON:  Parameter %d (node %d, component %d, %s, version %d) is unused in elements; eliminating.",
				row, (node) ? node->getIdentifier() : -1, fieldComponent + 1, cmzn_node_value_label_conversion::to_string(valueLabel), version + 1);
		}
	}

	// solve
	NEWMAT::CroutMatrix LUmatrix = globalHessian;
	if (LUmatrix.IsSingular())
	{
		display_message(ERROR_MESSAGE, "Optimisation optimise NEWTON:  Solution is singular.");
		return 0;
	}
	NEWMAT::ColumnVector increment = LUmatrix.i()*globalJacobian;

	const int result = fieldparameters.addParameters(globalParameterCount, increment.data());
	if (result != CMZN_OK)
	{
		display_message(ERROR_MESSAGE, "Optimisation optimise NEWTON:  Failed to add solution vector.");
		return 0;
	}

	return 1;
}
