/***************************************************************************//**
 * @file optimisation.hpp
 *
 * Minimisation object for performing optimisation algorithm from description
 * in cmzn_optimisation.
 *
 * @see-also api/zinc/optimisation.h
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef OPTIMISATION_HPP_
#define OPTIMISATION_HPP_

#include <vector>
#include "minimise/cmiss_optimisation_private.hpp"

class ObjectiveFieldData
{
public:
	cmzn_field_id field;
	int numComponents;
	int numTerms;
	int bufferSize;
	FE_value *buffer;

	ObjectiveFieldData(cmzn_field_id objectiveField) :
		field(cmzn_field_access(objectiveField)),
		numComponents(cmzn_field_get_number_of_components(field)),
		numTerms(0),
		bufferSize(0),
		buffer(0)
	{
	}

	~ObjectiveFieldData()
	{
		cmzn_field_destroy(&field);
		delete[] buffer;
	}

	int prepareTerms();
};

typedef std::vector<ObjectiveFieldData*> ObjectiveFieldDataVector;

class Minimisation
{
private:
	cmzn_optimisation& optimisation;

public:
	cmzn_fieldmodule_id field_module;
	cmzn_fieldcache_id field_cache;
	FE_value current_time;
	int total_dof;
	ObjectiveFieldDataVector objectiveFields;

private:
	FE_value **dof_storage_array;
	FE_value *dof_initial_values;
	int totalObjectiveFieldComponents;
	int totalLeastSquaresTerms;
	FE_value *objectiveValues;
	std::ostream optppMessageStream;

public:
	Minimisation(cmzn_optimisation& optimisation) :
		optimisation(optimisation),
		field_module(cmzn_fieldmodule_access(optimisation.getFieldmodule())),
		field_cache(cmzn_fieldmodule_create_fieldcache(field_module)),
		current_time(0.0),
		total_dof(0),
		dof_storage_array(0),
		dof_initial_values(0),
		optppMessageStream(&optimisation.solution_report)
	{
		totalObjectiveFieldComponents = 0;
		for (FieldList::iterator iter = optimisation.objectiveFields.begin();
			iter != optimisation.objectiveFields.end(); ++iter)
		{
			cmzn_field_id objectiveField = *iter;
			totalObjectiveFieldComponents += cmzn_field_get_number_of_components(objectiveField);
			objectiveFields.push_back(new ObjectiveFieldData(objectiveField));
		}
		objectiveValues = new FE_value[totalObjectiveFieldComponents];
		totalLeastSquaresTerms = 0;
	};

	~Minimisation();

	/** Prepare data structures for optimisation
	 * @return  1 on success, 0 on failure */
	int prepareOptimisation();

	/** Perform optimisation until ending condition met or error
	 * Must have successfully called prepareOptimisation() first.
	 * @return  1 on success, 0 on failure */
	int runOptimisation();

	FE_value *get_dof_initial_values()
	{
		return dof_initial_values;
	}

	inline void set_dof_value(int dof_index, FE_value new_value)
	{
		*dof_storage_array[dof_index] = new_value;
	}

	void list_dof_values();

	void invalidate_dependent_field_caches();

	void do_fieldassignments();

	/** @return  1 on success, 0 on failure */
	int evaluate_objective_function(FE_value *valueAddress);

private:

	int construct_dof_arrays();

	void touch_dependent_fields();

	int minimise_QN();

	int minimise_LSQN();

	int minimise_Newton();

};

#endif /* OPTIMISATION_HPP_ */
