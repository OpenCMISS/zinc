/**
 * @file cmiss_optimisation_private.hpp
 *
 * The private methods for the cmzn_optimisation object. Required to allow creation
 * of optimisation objects.
 *
 * @see-also api/zinc/optimisation.h
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __CMZN_OPTIMISATION_PRIVATE_HPP__
#define __CMZN_OPTIMISATION_PRIVATE_HPP__

#include <list>
#include <sstream>
#include "opencmiss/zinc/types/fieldassignmentid.h"
#include "opencmiss/zinc/optimisation.h"
#include "opencmiss/zinc/status.h"
#include "computed_field/field_module.hpp"

struct DependentAndConditionalFields
{
	cmzn_field_id dependentField;
	cmzn_field_id conditionalField;
};

typedef std::list<cmzn_field_id> FieldList;
typedef std::list<DependentAndConditionalFields> DependentAndConditionalFieldsList;

struct cmzn_optimisation
{
friend class Minimisation;
private:
	cmzn_fieldmodule_id fieldModule;
	cmzn_optimisation_method method;
	DependentAndConditionalFieldsList dependentFields;
	FieldList objectiveFields;
	std::list<cmzn_fieldassignment *> fieldassignments;
	int access_count;
public:
	// Opt++ stopping tolerances
	double functionTolerance;
	double gradientTolerance;
	double stepTolerance;
	int maximumIterations;
	int maximumNumberFunctionEvaluations;
	// Opt++ steplength control
	double maximumStep;
	double minimumStep;
	// Opt++ globalisation strategy parameters
	double linesearchTolerance;
	int maximumBacktrackIterations;
	double trustRegionSize;
	std::stringbuf solution_report; // solution details output by Opt++ during and after solution

	~cmzn_optimisation();
	// not implemented:
	cmzn_optimisation();
	cmzn_optimisation(const cmzn_optimisation& source);
	cmzn_optimisation& operator=(const cmzn_optimisation& source);

public:
	cmzn_optimisation(cmzn_fieldmodule_id field_module);

	cmzn_optimisation_id access()
	{
		++access_count;
		return this;
	}

	static int deaccess(cmzn_optimisation_id &optimisation)
	{
		if (!optimisation)
			return 0;
		--(optimisation->access_count);
		if (optimisation->access_count <= 0)
			delete optimisation;
		optimisation = 0;
		return 1;
	}

	cmzn_fieldmodule_id getFieldmodule() const
	{
		return this->fieldModule;
	}

	cmzn_optimisation_method getMethod() const
	{
		return this->method;
	}

	int setMethod(cmzn_optimisation_method methodIn)
	{
		if ((methodIn == CMZN_OPTIMISATION_METHOD_QUASI_NEWTON) ||
			(methodIn == CMZN_OPTIMISATION_METHOD_LEAST_SQUARES_QUASI_NEWTON) ||
			(methodIn == CMZN_OPTIMISATION_METHOD_NEWTON))
		{
			this->method = methodIn;
			return CMZN_OK;
		}
		return CMZN_ERROR_ARGUMENT;
	}

	cmzn_field_id getConditionalField(cmzn_field_id dependentField) const;

	int setConditionalField(cmzn_field_id dependentField, cmzn_field_id conditionalField);

	int addFieldassignment(cmzn_fieldassignment *fieldassignment);

	cmzn_field_id getFirstDependentField() const;

	cmzn_field_id getNextDependentField(cmzn_field_id ref_field) const;

	int addDependentField(cmzn_field_id field);

	int removeDependentField(cmzn_field_id field);

	cmzn_field_id getFirstObjectiveField() const;

	cmzn_field_id getNextObjectiveField(cmzn_field_id ref_field) const;

	int addObjectiveField(cmzn_field_id field);

	int removeObjectiveField(cmzn_field_id field);

	char *getSolutionReport();

	int runOptimisation();
};

#endif // __CMZN_OPTIMISATION_PRIVATE_HPP__
