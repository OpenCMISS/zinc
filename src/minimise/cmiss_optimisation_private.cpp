/**
 * @file cmiss_optimisation_private.cpp
 *
 * The cmzn_optimisation object.
 *
 * @see-also api/zinc/optimisation.h
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cstdio>
#include "opencmiss/zinc/field.h"
#include "opencmiss/zinc/region.h"
#include "computed_field/computed_field.h"
#include "computed_field/fieldassignmentprivate.hpp"
#include "computed_field/computed_field_composite.h"
#include "computed_field/computed_field_finite_element.h"
#include "general/mystring.h"
#include "general/debug.h"
#include "general/enumerator_conversion.hpp"
#include "minimise/cmiss_optimisation_private.hpp"
#include "minimise/optimisation.hpp"

cmzn_optimisation::cmzn_optimisation(cmzn_fieldmodule_id field_module) :
	fieldModule(cmzn_region_get_fieldmodule(cmzn_fieldmodule_get_region_internal(field_module))),
	method(CMZN_OPTIMISATION_METHOD_QUASI_NEWTON),
	access_count(1),
	functionTolerance(1.49012e-8),
	gradientTolerance(6.05545e-6),
	stepTolerance(1.49012e-8),
	maximumIterations(100),
	maximumNumberFunctionEvaluations(1000),
	maximumStep(1.0e3),
	minimumStep(1.49012e-8),
	linesearchTolerance(1.e-4),
	maximumBacktrackIterations(5),
	trustRegionSize(0.1)
{
}

cmzn_optimisation::~cmzn_optimisation()
{
	for (auto iter = this->dependentFields.begin(); iter != this->dependentFields.end(); ++iter)
	{
		cmzn_field_destroy(&(iter->dependentField));
		cmzn_field_destroy(&(iter->conditionalField));
	}
	for (auto iter = this->objectiveFields.begin(); iter != this->objectiveFields.end(); ++iter)
	{
		cmzn_field_destroy(&(*iter));
	}
	for (auto iter = this->fieldassignments.begin(); iter != this->fieldassignments.end(); ++iter)
	{
		cmzn_fieldassignment_destroy(&(*iter));
	}
	cmzn_fieldmodule_destroy(&fieldModule);
}

cmzn_field_id cmzn_optimisation::getConditionalField(cmzn_field_id dependentField) const
{
	if (dependentField)
	{
		DependentAndConditionalFieldsList::const_iterator iter;
		for (iter = dependentFields.begin(); iter != dependentFields.end(); ++iter)
			if (iter->dependentField == dependentField)
				return cmzn_field_access(iter->conditionalField);
	}
	return 0;
}

/**
 * @param dependentField  The dependent field to apply condition to.
 * @param conditionalField  Field with either 1 component, or as many components
 * as dependentField. NULL to clear.
 */
int cmzn_optimisation::setConditionalField(cmzn_field_id dependentField,
	cmzn_field_id conditionalField)
{
	int conditionalComponents;
	if (dependentField && ((0 == conditionalField) ||
		(1 == (conditionalComponents = cmzn_field_get_number_of_components(conditionalField))) ||
		(cmzn_field_get_number_of_components(dependentField) == conditionalComponents)))
	{
		DependentAndConditionalFieldsList::iterator iter;
		for (iter = dependentFields.begin(); iter != dependentFields.end(); ++iter)
			if (iter->dependentField == dependentField)
			{
				REACCESS(Computed_field)(&(iter->conditionalField), conditionalField);
				return CMZN_OK;
			}
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_optimisation::addFieldassignment(cmzn_fieldassignment *fieldassignment)
{
	if ((!fieldassignment) || (Computed_field_get_region(fieldassignment->getTargetField())
		!= (cmzn_fieldmodule_get_region_internal(this->fieldModule))))
	{
		display_message(ERROR_MESSAGE, "Optimisation addFieldassignment.  Missing or invalid field assignment for this optimisation");
		return CMZN_ERROR_ARGUMENT;
	}
	this->fieldassignments.push_back(fieldassignment->access());
	return CMZN_OK;
}

cmzn_field_id cmzn_optimisation::getFirstDependentField() const
{
	DependentAndConditionalFieldsList::const_iterator iter = dependentFields.begin();
	if (iter != dependentFields.end())
		return cmzn_field_access(iter->dependentField);
	return 0;
}

cmzn_field_id cmzn_optimisation::getNextDependentField(cmzn_field_id ref_field) const
{
	DependentAndConditionalFieldsList::const_iterator iter;
	for (iter = dependentFields.begin(); iter != dependentFields.end(); ++iter)
		if (iter->dependentField == ref_field)
		{
			++iter;
			if (iter != dependentFields.end())
				return cmzn_field_access(iter->dependentField);
			break;
		}
	return 0;
}

int cmzn_optimisation::addDependentField(cmzn_field_id field)
{
	if (!cmzn_fieldmodule_contains_field(fieldModule, field))
		return CMZN_ERROR_ARGUMENT;
	if (!(Computed_field_is_constant(field) || Computed_field_is_type_finite_element(field)))
		return CMZN_ERROR_ARGUMENT;
	if (cmzn_field_get_value_type(field) != CMZN_FIELD_VALUE_TYPE_REAL)
		return CMZN_ERROR_ARGUMENT;
	DependentAndConditionalFieldsList::const_iterator iter;
	for (iter = dependentFields.begin(); iter != dependentFields.end(); ++iter)
	{
		if (iter->dependentField == field)
			return CMZN_ERROR_ARGUMENT;
	}
	DependentAndConditionalFields newRecord;
	newRecord.dependentField = cmzn_field_access(field);
	newRecord.conditionalField = 0;
	this->dependentFields.push_back(newRecord);
	return CMZN_OK;
}

int cmzn_optimisation::removeDependentField(cmzn_field_id field)
{
	DependentAndConditionalFieldsList::iterator iter;
	for (iter = dependentFields.begin(); iter != dependentFields.end(); ++iter)
	{
		if (iter->dependentField == field)
		{
			cmzn_field_destroy(&(iter->dependentField));
			cmzn_field_destroy(&(iter->conditionalField));
			dependentFields.erase(iter);
			return CMZN_OK;
		}
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_field_id cmzn_optimisation::getFirstObjectiveField() const
{
	cmzn_field_id field = 0;
	FieldList::const_iterator iter = objectiveFields.begin();
	if (iter != objectiveFields.end())
	{
		field = cmzn_field_access(*iter);
	}
	return field;
}

cmzn_field_id cmzn_optimisation::getNextObjectiveField(cmzn_field_id ref_field) const
{
	cmzn_field_id field = 0;
	FieldList::const_iterator iter = objectiveFields.begin();
	while (iter != objectiveFields.end())
	{
		if (*iter == ref_field)
		{
			++iter;
			if (iter != objectiveFields.end())
			{
				field = cmzn_field_access(*iter);
			}
			break;
		}
		++iter;
	}
	return field;
}

int cmzn_optimisation::addObjectiveField(cmzn_field_id field)
{
	if (!cmzn_fieldmodule_contains_field(fieldModule, field))
		return CMZN_ERROR_ARGUMENT;
	if (cmzn_field_get_value_type(field) != CMZN_FIELD_VALUE_TYPE_REAL)
		return CMZN_ERROR_ARGUMENT;
	FieldList::const_iterator iter;
	for (iter = objectiveFields.begin(); iter != objectiveFields.end(); ++iter)
	{
		if (*iter == field)
			return CMZN_ERROR_ARGUMENT;
	}
	objectiveFields.push_back(cmzn_field_access(field));
	return CMZN_OK;
}

int cmzn_optimisation::removeObjectiveField(cmzn_field_id field)
{
	FieldList::iterator iter;
	for (iter = objectiveFields.begin(); iter != objectiveFields.end(); ++iter)
	{
		if (*iter == field)
		{
			cmzn_field_destroy(&(*iter));
			objectiveFields.erase(iter);
			return CMZN_OK;
		}
	}
	return CMZN_ERROR_ARGUMENT;
}

char *cmzn_optimisation::getSolutionReport()
{
	std::string temp = solution_report.str();
	return duplicate_string(temp.c_str());
}

int cmzn_optimisation::runOptimisation()
{
	int return_code = CMZN_OK;
	solution_report.str("");
	// check required attributes are set
	if (method == CMZN_OPTIMISATION_METHOD_INVALID)
	{
		display_message(ERROR_MESSAGE, "Optimisation optimise.  Optimisation method invalid or not set.");
		return_code = CMZN_ERROR_ARGUMENT;
	}
	if (dependentFields.size() < 1)
	{
		display_message(ERROR_MESSAGE, "Optimisation optimise.  Must set at least one dependent field.");
		return_code = CMZN_ERROR_ARGUMENT;
	}
	if (objectiveFields.size() < 1)
	{
		display_message(ERROR_MESSAGE, "Optimisation optimise.  Must set at least one objective field.");
		return_code = CMZN_ERROR_ARGUMENT;
	}
	if (return_code == CMZN_OK)
	{
		Minimisation minimisation(*this);
		return_code = minimisation.prepareOptimisation();
		if (return_code == CMZN_OK)
			return_code = minimisation.runOptimisation();
	}
	return return_code;
}

cmzn_optimisation_id cmzn_fieldmodule_create_optimisation(cmzn_fieldmodule_id field_module)
{
	if (field_module)
		return new cmzn_optimisation(field_module);
	return 0;
}

cmzn_optimisation_id cmzn_optimisation_access(cmzn_optimisation_id optimisation)
{
	if (optimisation)
		return optimisation->access();
	return 0;
}

int cmzn_optimisation_destroy(cmzn_optimisation_id *optimisation_address)
{
	if (optimisation_address)
		return cmzn_optimisation::deaccess(*optimisation_address);
	return CMZN_ERROR_ARGUMENT;
}

cmzn_field_id cmzn_optimisation_get_conditional_field(
	cmzn_optimisation_id optimisation, cmzn_field_id dependent_field)
{
	if (optimisation)
		return optimisation->getConditionalField(dependent_field);
	return 0;
}

int cmzn_optimisation_set_conditional_field(
	cmzn_optimisation_id optimisation, cmzn_field_id dependent_field,
	cmzn_field_id conditional_field)
{
	if (optimisation)
		return optimisation->setConditionalField(dependent_field, conditional_field);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_optimisation_add_fieldassignment(
	cmzn_optimisation_id optimisation, cmzn_fieldassignment_id fieldassignment)
{
	if (optimisation)
	{
		return optimisation->addFieldassignment(fieldassignment);
	}
	return CMZN_ERROR_ARGUMENT;
}

enum cmzn_optimisation_method cmzn_optimisation_get_method(cmzn_optimisation_id optimisation)
{
	if (optimisation)
		return optimisation->getMethod();
	return CMZN_OPTIMISATION_METHOD_INVALID;
}

int cmzn_optimisation_set_method(cmzn_optimisation_id optimisation,
		enum cmzn_optimisation_method method)
{
	if (optimisation)
		return optimisation->setMethod(method);
	return CMZN_ERROR_ARGUMENT;
}

class cmzn_optimisation_method_conversion
{
public:
	static const char *to_string(enum cmzn_optimisation_method method)
	{
		const char *enum_string = 0;
		switch (method)
		{
			case CMZN_OPTIMISATION_METHOD_QUASI_NEWTON:
				enum_string = "QUASI_NEWTON";
				break;
			case CMZN_OPTIMISATION_METHOD_LEAST_SQUARES_QUASI_NEWTON:
				enum_string = "LEAST_SQUARES_QUASI_NEWTON";
				break;
			case CMZN_OPTIMISATION_METHOD_NEWTON:
				enum_string = "NEWTON";
				break;
			default:
				break;
		}
		return enum_string;
	}
};

enum cmzn_optimisation_method
	cmzn_optimisation_method_enum_from_string(const char *string)
{
	return string_to_enum<enum cmzn_optimisation_method,
	cmzn_optimisation_method_conversion>(string);
}

char *cmzn_optimisation_method_enum_to_string(
	enum cmzn_optimisation_method method)
{
	const char *method_string = cmzn_optimisation_method_conversion::to_string(method);
	return (method_string ? duplicate_string(method_string) : 0);
}

int cmzn_optimisation_get_attribute_integer(cmzn_optimisation_id optimisation,
		enum cmzn_optimisation_attribute attribute)
{
	if (optimisation)
	{
		switch (attribute)
		{
		case CMZN_OPTIMISATION_ATTRIBUTE_MAXIMUM_ITERATIONS:
			return optimisation->maximumIterations;
			break;
		case CMZN_OPTIMISATION_ATTRIBUTE_MAXIMUM_FUNCTION_EVALUATIONS:
			return optimisation->maximumNumberFunctionEvaluations;
			break;
		case CMZN_OPTIMISATION_ATTRIBUTE_MAXIMUM_BACKTRACK_ITERATIONS:
			return optimisation->maximumBacktrackIterations;
			break;
		default:
			break;
		}
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_optimisation_set_attribute_integer(cmzn_optimisation_id optimisation,
		enum cmzn_optimisation_attribute attribute, int value)
{
	int return_code = CMZN_ERROR_ARGUMENT;
	if (optimisation)
	{
		return_code = CMZN_OK;
		switch (attribute)
		{
		case CMZN_OPTIMISATION_ATTRIBUTE_MAXIMUM_ITERATIONS:
			optimisation->maximumIterations = value;
			break;
		case CMZN_OPTIMISATION_ATTRIBUTE_MAXIMUM_FUNCTION_EVALUATIONS:
			optimisation->maximumNumberFunctionEvaluations = value;
			break;
		case CMZN_OPTIMISATION_ATTRIBUTE_MAXIMUM_BACKTRACK_ITERATIONS:
			optimisation->maximumBacktrackIterations = value;
			break;
		default:
			return_code = CMZN_ERROR_ARGUMENT;
			break;
		}
	}
	return return_code;
}

double cmzn_optimisation_get_attribute_real(cmzn_optimisation_id optimisation,
		enum cmzn_optimisation_attribute attribute)
{
	if (optimisation)
	{
		switch (attribute)
		{
		case CMZN_OPTIMISATION_ATTRIBUTE_FUNCTION_TOLERANCE:
			return optimisation->functionTolerance;
			break;
		case CMZN_OPTIMISATION_ATTRIBUTE_GRADIENT_TOLERANCE:
			return optimisation->gradientTolerance;
			break;
		case CMZN_OPTIMISATION_ATTRIBUTE_STEP_TOLERANCE:
			return optimisation->stepTolerance;
			break;
		case CMZN_OPTIMISATION_ATTRIBUTE_MAXIMUM_STEP:
			return optimisation->maximumStep;
			break;
		case CMZN_OPTIMISATION_ATTRIBUTE_MINIMUM_STEP:
			return optimisation->minimumStep;
			break;
		case CMZN_OPTIMISATION_ATTRIBUTE_LINESEARCH_TOLERANCE:
			return optimisation->linesearchTolerance;
			break;
		case CMZN_OPTIMISATION_ATTRIBUTE_TRUST_REGION_SIZE:
			return optimisation->trustRegionSize;
			break;
		default:
			break;
		}
	}
	return 0.0;
}

int cmzn_optimisation_set_attribute_real(cmzn_optimisation_id optimisation,
		enum cmzn_optimisation_attribute attribute, double value)
{
	int return_code = CMZN_ERROR_ARGUMENT;
	if (optimisation)
	{
		return_code = CMZN_OK;
		switch (attribute)
		{
		case CMZN_OPTIMISATION_ATTRIBUTE_FUNCTION_TOLERANCE:
			optimisation->functionTolerance = value;
			break;
		case CMZN_OPTIMISATION_ATTRIBUTE_GRADIENT_TOLERANCE:
			optimisation->gradientTolerance = value;
			break;
		case CMZN_OPTIMISATION_ATTRIBUTE_STEP_TOLERANCE:
			optimisation->stepTolerance = value;
			break;
		case CMZN_OPTIMISATION_ATTRIBUTE_MAXIMUM_STEP:
			optimisation->maximumStep = value;
			break;
		case CMZN_OPTIMISATION_ATTRIBUTE_MINIMUM_STEP:
			optimisation->minimumStep = value;
			break;
		case CMZN_OPTIMISATION_ATTRIBUTE_LINESEARCH_TOLERANCE:
			optimisation->linesearchTolerance = value;
			break;
		case CMZN_OPTIMISATION_ATTRIBUTE_TRUST_REGION_SIZE:
			optimisation->trustRegionSize = value;
			break;
		default:
			return_code = CMZN_ERROR_ARGUMENT;
			break;
		}
	}
	return return_code;
}

class cmzn_optimisation_attribute_conversion
{
public:
	static const char *to_string(enum cmzn_optimisation_attribute attribute)
	{
		const char *enum_string = 0;
		switch (attribute)
		{
			case CMZN_OPTIMISATION_ATTRIBUTE_FUNCTION_TOLERANCE:
				enum_string = "FUNCTION_TOLERANCE";
				break;
			case CMZN_OPTIMISATION_ATTRIBUTE_GRADIENT_TOLERANCE:
				enum_string = "GRADIENT_TOLERANCE";
				break;
			case CMZN_OPTIMISATION_ATTRIBUTE_STEP_TOLERANCE:
				enum_string = "STEP_TOLERANCE";
				break;
			case CMZN_OPTIMISATION_ATTRIBUTE_MAXIMUM_ITERATIONS:
				enum_string = "MAXIMUM_ITERATIONS";
				break;
			case CMZN_OPTIMISATION_ATTRIBUTE_MAXIMUM_FUNCTION_EVALUATIONS:
				enum_string = "MAXIMUM_FUNCTION_EVALUATIONS";
				break;
			case CMZN_OPTIMISATION_ATTRIBUTE_MAXIMUM_STEP:
				enum_string = "MAXIMUM_STEP";
				break;
			case CMZN_OPTIMISATION_ATTRIBUTE_MINIMUM_STEP:
				enum_string = "MINIMUM_STEP";
				break;
			case CMZN_OPTIMISATION_ATTRIBUTE_LINESEARCH_TOLERANCE:
				enum_string = "LINESEARCH_TOLERANCE";
				break;
			case CMZN_OPTIMISATION_ATTRIBUTE_MAXIMUM_BACKTRACK_ITERATIONS:
				enum_string = "MAXIMUM_BACKTRACK_ITERATIONS";
				break;
			case CMZN_OPTIMISATION_ATTRIBUTE_TRUST_REGION_SIZE:
				enum_string = "TRUST_REGION_SIZE";
				break;
			default:
				break;
		}
		return enum_string;
	}
};

enum cmzn_optimisation_attribute cmzn_optimisation_attribute_enum_from_string(
	const char *string)
{
	return string_to_enum<enum cmzn_optimisation_attribute,
	cmzn_optimisation_attribute_conversion>(string);
}

char *cmzn_optimisation_attribute_enum_to_string(
	enum cmzn_optimisation_attribute attribute)
{
	const char *attribute_string = cmzn_optimisation_attribute_conversion::to_string(attribute);
	return (attribute_string ? duplicate_string(attribute_string) : 0);
}

cmzn_field_id cmzn_optimisation_get_first_dependent_field(
	cmzn_optimisation_id optimisation)
{
	if (optimisation)
		return optimisation->getFirstDependentField();
	return nullptr;
}

cmzn_field_id cmzn_optimisation_get_next_dependent_field(
	cmzn_optimisation_id optimisation, cmzn_field_id ref_field)
{
	if (optimisation && ref_field)
		return optimisation->getNextDependentField(ref_field);
	return nullptr;
}

int cmzn_optimisation_add_dependent_field(cmzn_optimisation_id optimisation,
	cmzn_field_id field)
{
	if (optimisation && field)
		return optimisation->addDependentField(field);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_optimisation_remove_dependent_field(
	cmzn_optimisation_id optimisation, cmzn_field_id field)
{
	if (optimisation && field)
		return optimisation->removeDependentField(field);
	return CMZN_ERROR_ARGUMENT;
}

/** @deprecated  Misnamed: use dependent field function. */
cmzn_field_id cmzn_optimisation_get_first_independent_field(
	cmzn_optimisation_id optimisation)
{
	if (optimisation)
		return optimisation->getFirstDependentField();
	return nullptr;
}

/** @deprecated  Misnamed: use dependent field function. */
cmzn_field_id cmzn_optimisation_get_next_independent_field(
	cmzn_optimisation_id optimisation, cmzn_field_id ref_field)
{
	if (optimisation && ref_field)
		return optimisation->getNextDependentField(ref_field);
	return nullptr;
}

/** @deprecated  Misnamed: use dependent field function. */
int cmzn_optimisation_add_independent_field(cmzn_optimisation_id optimisation,
	cmzn_field_id field)
{
	if (optimisation && field)
		return optimisation->addDependentField(field);
	return CMZN_ERROR_ARGUMENT;
}

/** @deprecated  Misnamed: use dependent field function. */
int cmzn_optimisation_remove_independent_field(
	cmzn_optimisation_id optimisation, cmzn_field_id field)
{
	if (optimisation && field)
		return optimisation->removeDependentField(field);
	return CMZN_ERROR_ARGUMENT;
}

cmzn_field_id cmzn_optimisation_get_first_objective_field(
	cmzn_optimisation_id optimisation)
{
	if (optimisation)
		return optimisation->getFirstObjectiveField();
	return 0;
}

cmzn_field_id cmzn_optimisation_get_next_objective_field(
	cmzn_optimisation_id optimisation, cmzn_field_id ref_field)
{
	if (optimisation && ref_field)
		return optimisation->getNextObjectiveField(ref_field);
	return 0;
}

int cmzn_optimisation_add_objective_field(cmzn_optimisation_id optimisation,
	cmzn_field_id field)
{
	if (optimisation && field)
		return optimisation->addObjectiveField(field);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_optimisation_remove_objective_field(
	cmzn_optimisation_id optimisation, cmzn_field_id field)
{
	if (optimisation && field)
		return optimisation->removeObjectiveField(field);
	return CMZN_ERROR_ARGUMENT;
}

char *cmzn_optimisation_get_solution_report(cmzn_optimisation_id optimisation)
{
	if (optimisation)
		return optimisation->getSolutionReport();
	return 0;
}

int cmzn_optimisation_optimise(cmzn_optimisation_id optimisation)
{
	if (optimisation)
		return optimisation->runOptimisation();
	return CMZN_ERROR_ARGUMENT;
}
