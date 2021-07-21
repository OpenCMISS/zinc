/***************************************************************************//**
 * FILE : computed_field_nodeset_operators.cpp
 *
 * Implementation of field operators that act on a nodeset.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "opencmiss/zinc/fieldfiniteelement.h"
#include "opencmiss/zinc/fieldnodesetoperators.h"
#include "opencmiss/zinc/nodeset.h"
#include "computed_field/computed_field_finite_element.h"
#include "computed_field/computed_field_private.hpp"
#include "computed_field/computed_field_nodeset_operators.hpp"
#include "computed_field/computed_field_subobject_group.hpp"
#include "computed_field/field_module.hpp"
#include "mesh/cmiss_node_private.hpp"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_set.h"
#include "region/cmiss_region.hpp"
#include "general/debug.h"
#include "general/mystring.h"
#include "general/message.h"
#include "finite_element/finite_element_region.h"
#include <cmath>
#include <iostream>

using namespace std;

class Computed_field_nodeset_operators_package : public Computed_field_type_package
{
};

namespace {

const char computed_field_nodeset_operator_type_string[] = "nodeset_operator";

class Computed_field_nodeset_operator : public Computed_field_core
{
protected:
	cmzn_nodeset_id nodeset;

public:
	Computed_field_nodeset_operator(cmzn_nodeset_id nodeset_in) :
		Computed_field_core(),
		nodeset(cmzn_nodeset_access(nodeset_in))
	{
	}

	virtual ~Computed_field_nodeset_operator()
	{
		cmzn_nodeset_destroy(&nodeset);
	}

	virtual void inherit_source_field_attributes()
	{
		if (field)
		{
			Computed_field_set_coordinate_system_from_sources(field);
		}
	}

	const char *get_type_string()
	{
		return(computed_field_nodeset_operator_type_string);
	}

	cmzn_nodeset_id get_nodeset()
	{
		return nodeset;
	}

	virtual FieldValueCache *createValueCache(cmzn_fieldcache& fieldCache)
	{
		RealFieldValueCache *valueCache = new RealFieldValueCache(field->number_of_components);
		valueCache->getOrCreateSharedExtraCache(fieldCache);
		return valueCache;
	}

	virtual bool is_defined_at_location(cmzn_fieldcache& cache)
	{
		return true;
	}

	int list();

	char* get_command_string();

	// if the nodeset is a nodeset group, also need to propagate changes from it
	virtual int check_dependency()
	{
		int return_code = Computed_field_core::check_dependency();
		if (!(return_code & MANAGER_CHANGE_FULL_RESULT(Computed_field)))
		{
			cmzn_field_node_group *nodeGroupField = cmzn_nodeset_get_node_group_field_internal(this->nodeset);
			if (nodeGroupField && (MANAGER_CHANGE_NONE(Computed_field) !=
				cmzn_field_node_group_base_cast(nodeGroupField)->manager_change_status))
			{
				this->field->setChangedPrivate(MANAGER_CHANGE_FULL_RESULT(Computed_field));
				return_code = this->field->manager_change_status;
			}
		}
		return return_code;
	}

	/** Get current node-to-element map field, allowing evaluation at elements,
	 * applying only to nodes mapped to the element.
	 * @return  Non-accessed handle to element map field, or nullptr if none */
	cmzn_field *getElementMapField() const
	{
		return (2 == this->field->number_of_source_fields) ? this->field->source_fields[1] : nullptr;
	}

	/** Set or unset node-to-element map field, allowing evaluation at elements,
	 * applying only to nodes mapped to the element.
	 * @param elementMapField  Field giving map from nodes to elements where
	 * field can be evaluated (currently field must be stored mesh location type),
	 * or nullptr to disable */
	int setElementMapField(cmzn_field *elementMapField)
	{
		if (elementMapField)
		{
			if (elementMapField->getManager() != this->field->getManager())
			{
				display_message(ERROR_MESSAGE, "FieldNodesetOperator setElementMapField:  Element map field is from a different region");
				return CMZN_ERROR_ARGUMENT;
			}
			cmzn_field_stored_mesh_location *storedMeshLocation = cmzn_field_cast_stored_mesh_location(elementMapField);
			if (!storedMeshLocation)
			{
				display_message(ERROR_MESSAGE, "FieldNodesetOperator setElementMapField:  Element map field must be stored mesh location type");
				return CMZN_ERROR_ARGUMENT;
			}
			cmzn_field_stored_mesh_location_destroy(&storedMeshLocation);
		}
		return this->field->setOptionalSourceField(2, elementMapField);
	}

protected:
	template <class TermOperator> int evaluateNodesetOperator(cmzn_fieldcache& cache, FieldValueCache& inValueCache, TermOperator& tempOperator);
	template <class TermOperator> int evaluateDerivativeNodesetOperator(cmzn_fieldcache& cache, FieldValueCache& inValueCache,
		TermOperator& tempOperator, const FieldDerivative& fieldDerivative);
};

template <class TermOperator> int Computed_field_nodeset_operator::evaluateNodesetOperator(
	cmzn_fieldcache& cache, FieldValueCache& inValueCache, TermOperator& termOperator)
{
	cmzn_fieldcache& extraCache = *(inValueCache.getExtraCache());
	extraCache.setTime(cache.getTime());
	cmzn_field_id sourceField = getSourceField(0);
	const Field_location_element_xi *element_xi_location = cache.get_location_element_xi();
	cmzn_field *elementMap = this->getElementMapField();
	if ((elementMap) && (element_xi_location))
	{
		cmzn_element *element = element_xi_location->get_element();
		if (!element->getMesh())
		{
			display_message(ERROR_MESSAGE, "FieldNodesetEvaluator evaluate:  Invalid element");
			return 0;  // invalid element
		}
		// evaluate at element, operator applies to nodes with mapping to element
		FE_nodeset *feNodeset = cmzn_nodeset_get_FE_nodeset_internal(this->nodeset);
		cmzn_field_node_group *nodeGroup = cmzn_nodeset_get_node_group_field_internal(this->nodeset);
		FE_field *feField = nullptr;
		Computed_field_get_type_finite_element(elementMap, &feField);
		if (!feField)
		{
			display_message(ERROR_MESSAGE, "FieldNodesetEvaluator evaluate:  Invalid element evaluation map field");
			return 0;
		}
		FE_mesh *hostMesh = feField->getElementXiHostMesh();
		FE_mesh_embedded_node_field *embeddedNodeField = feField->getEmbeddedNodeField(feNodeset);
		if (!embeddedNodeField)
			return 1;  // no values
		if (element->getMesh() != hostMesh)
			return 1;  // future: map nodes embedded in faces
		// iterate over reverse map of element to nodes maintained in embeddedNodeField
		int size = 0;
		const DsLabelIndex *nodeIndexes = embeddedNodeField->getNodeIndexes(element->getIndex(), size);
		for (int i = 0; i < size; ++i)
		{
			const DsLabelIndex nodeIndex = nodeIndexes[i];
			if ((nodeGroup) && !Computed_field_node_group_core_cast(nodeGroup)->containsIndex(nodeIndex))
				continue;
			extraCache.setNodeWithHostElement(feNodeset->getNode(nodeIndex), element);
			const RealFieldValueCache* sourceValueCache = RealFieldValueCache::cast(sourceField->evaluate(extraCache));
			if (sourceValueCache)
				termOperator.processTerm(sourceValueCache->values);
		}
	}
	else
	{
		// iterate over whole nodeset
		cmzn_nodeiterator *iterator = cmzn_nodeset_create_nodeiterator(this->nodeset);
		cmzn_node *node = 0;
		while (0 != (node = cmzn_nodeiterator_next_non_access(iterator)))
		{
			extraCache.setNode(node);
			const RealFieldValueCache* sourceValueCache = RealFieldValueCache::cast(sourceField->evaluate(extraCache));
			if (sourceValueCache)
				termOperator.processTerm(sourceValueCache->values);
		}
		cmzn_nodeiterator_destroy(&iterator);
	}
	return 1;
}

template <class TermOperator> int Computed_field_nodeset_operator::evaluateDerivativeNodesetOperator(
	cmzn_fieldcache& cache, FieldValueCache& inValueCache, TermOperator& termOperator, const FieldDerivative& fieldDerivative)
{
	cmzn_fieldcache& extraCache = *(inValueCache.getExtraCache());
	extraCache.setTime(cache.getTime());
	cmzn_field_id sourceField = getSourceField(0);
	const Field_location_element_xi *element_xi_location = cache.get_location_element_xi();
	cmzn_field *elementMap = this->getElementMapField();
	if (!((elementMap) && (element_xi_location)))
		return 0;
	cmzn_element *element = element_xi_location->get_element();
	if (!element->getMesh())
	{
		display_message(ERROR_MESSAGE, "FieldNodesetEvaluator evaluateDerivative:  Invalid element");
		return 0;  // invalid element
	}
	// evaluate at element, operator applies to nodes with mapping to element
	FE_nodeset *feNodeset = cmzn_nodeset_get_FE_nodeset_internal(this->nodeset);
	cmzn_field_node_group *nodeGroup = cmzn_nodeset_get_node_group_field_internal(this->nodeset);
	FE_field *feField = nullptr;
	Computed_field_get_type_finite_element(elementMap, &feField);
	if (!feField)
	{
		display_message(ERROR_MESSAGE, "FieldNodesetEvaluator evaluateDerivative:  Invalid element evaluation map field");
		return 0;
	}
	FE_mesh *hostMesh = feField->getElementXiHostMesh();
	FE_mesh_embedded_node_field *embeddedNodeField = feField->getEmbeddedNodeField(feNodeset);
	if (!embeddedNodeField)
		return 1;  // no values
	if (element->getMesh() != hostMesh)
		return 1;  // future: map nodes embedded in faces
	// iterate over reverse map of element to nodes maintained in embeddedNodeField
	int size = 0;
	const DsLabelIndex *nodeIndexes = embeddedNodeField->getNodeIndexes(element->getIndex(), size);
	for (int i = 0; i < size; ++i)
	{
		const DsLabelIndex nodeIndex = nodeIndexes[i];
		if ((nodeGroup) && !Computed_field_node_group_core_cast(nodeGroup)->containsIndex(nodeIndex))
			continue;
		extraCache.setNodeWithHostElement(feNodeset->getNode(nodeIndex), element);
		const DerivativeValueCache *sourceDerivativeCache = sourceField->evaluateDerivative(extraCache, fieldDerivative);
		if (sourceDerivativeCache)
			termOperator.processTerm(sourceDerivativeCache->values);
	}
	return 1;
}

/** Lists a description of the nodeset_operator arguments */
int Computed_field_nodeset_operator::list()
{
	if (field)
	{
		display_message(INFORMATION_MESSAGE,"    field : %s\n",
			field->source_fields[0]->name);
		char *nodeset_name = cmzn_nodeset_get_name(nodeset);
		display_message(INFORMATION_MESSAGE,"    nodeset : %s\n", nodeset_name);
		DEALLOCATE(nodeset_name);
		return 1;
	}
	return 0;
}

/** Returns allocated command string for reproducing field. Includes type. */
char *Computed_field_nodeset_operator::get_command_string()
{
	char *command_string = 0;
	if (field)
	{
		int error = 0;
		append_string(&command_string, get_type_string(), &error);
		append_string(&command_string, " field ", &error);
		append_string(&command_string, field->source_fields[0]->name, &error);
		char *nodeset_name = cmzn_nodeset_get_name(nodeset);
		append_string(&command_string, " nodeset ", &error);
		make_valid_token(&nodeset_name);
		append_string(&command_string, nodeset_name, &error);
		DEALLOCATE(nodeset_name);
	}
	return (command_string);
}


const char computed_field_nodeset_sum_type_string[] = "nodeset_sum";

class Computed_field_nodeset_sum : public Computed_field_nodeset_operator
{
public:
	Computed_field_nodeset_sum(cmzn_nodeset_id nodeset_in) :
		Computed_field_nodeset_operator(nodeset_in)
	{
	}

	Computed_field_core *copy()
	{
		return new Computed_field_nodeset_sum(nodeset);
	}

	const char *get_type_string()
	{
		return (computed_field_nodeset_sum_type_string);
	}

	int compare(Computed_field_core* other_core)
	{
		Computed_field_nodeset_sum *other =
			dynamic_cast<Computed_field_nodeset_sum*>(other_core);
		if (other)
			return cmzn_nodeset_match(nodeset, other->get_nodeset());
		return 0;
	}

	virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative);

	virtual int getDerivativeTreeOrder(const FieldDerivative& fieldDerivative)
	{
		return this->field->source_fields[0]->getDerivativeTreeOrder(fieldDerivative);
	}

};

class TermOperatorSum
{
	const int valuesCount;
	FE_value *values;

public:
	TermOperatorSum(int valuesCountIn, FE_value *valuesIn) :
		valuesCount(valuesCountIn),
		values(valuesIn)
	{
		for (int i = 0; i < this->valuesCount; ++i)
			this->values[i] = 0.0;
	}

	inline void processTerm(const FE_value *sourceValues)
	{
		for (int i = 0; i < this->valuesCount; ++i)
			this->values[i] += sourceValues[i];
	}
};

int Computed_field_nodeset_sum::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
	TermOperatorSum termSum(this->field->number_of_components, valueCache.values);
	return this->evaluateNodesetOperator(cache, inValueCache, termSum);
}

int Computed_field_nodeset_sum::evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
{
	RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
	DerivativeValueCache *derivativeCache = inValueCache.getDerivativeValueCache(fieldDerivative);
	TermOperatorSum termSum(derivativeCache->getValueCount(), derivativeCache->values);
	return this->evaluateDerivativeNodesetOperator(cache, inValueCache, termSum, fieldDerivative);
}


const char computed_field_nodeset_mean_type_string[] = "nodeset_mean";

class Computed_field_nodeset_mean : public Computed_field_nodeset_operator
{
public:
	Computed_field_nodeset_mean(cmzn_nodeset_id nodeset_in) :
		Computed_field_nodeset_operator(nodeset_in)
	{
	}

	Computed_field_core *copy()
	{
		return new Computed_field_nodeset_mean(nodeset);
	}

	const char *get_type_string()
	{
		return (computed_field_nodeset_mean_type_string);
	}

	int compare(Computed_field_core* other_core)
	{
		Computed_field_nodeset_mean *other = dynamic_cast<Computed_field_nodeset_mean*>(other_core);
		if (other)
			return cmzn_nodeset_match(nodeset, other->get_nodeset());
		return 0;
	}

	virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
	{
		return this->evaluateDerivativeFiniteDifference(cache, inValueCache, fieldDerivative);
	}

};

class TermOperatorSumCount
{
	const int valuesCount;
	FE_value *values;
	int termCount;

public:
	TermOperatorSumCount(int valuesCountIn, FE_value *valuesIn) :
		valuesCount(valuesCountIn),
		values(valuesIn),
		termCount(0)
	{
		for (int i = 0; i < this->valuesCount; ++i)
			this->values[i] = 0.0;
	}

	inline void processTerm(const FE_value *sourceValues)
	{
		for (int i = 0; i < this->valuesCount; ++i)
			this->values[i] += sourceValues[i];
		++(this->termCount);
	}

	int getTermCount() const
	{
		return this->termCount;
	}
};

int Computed_field_nodeset_mean::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
	TermOperatorSumCount termSumCount(this->field->number_of_components, valueCache.values);
	const int result = this->evaluateNodesetOperator(cache, inValueCache, termSumCount);
	if (result)
	{
		const int termCount = termSumCount.getTermCount();
		if (termCount > 0)
		{
			const FE_value scaling = 1.0 / static_cast<FE_value>(termCount);
			for (int i = 0; i < this->field->number_of_components; ++i)
				valueCache.values[i] *= scaling;
			return 1;
		}
	}
	return 0;
}


const char computed_field_nodeset_sum_squares_type_string[] = "nodeset_sum_squares";

class Computed_field_nodeset_sum_squares : public Computed_field_nodeset_operator
{
public:
	Computed_field_nodeset_sum_squares(cmzn_nodeset_id nodeset_in) :
		Computed_field_nodeset_operator(nodeset_in)
	{
	}

	Computed_field_core *copy()
	{
		return new Computed_field_nodeset_sum_squares(nodeset);
	}

	const char *get_type_string()
	{
		return (computed_field_nodeset_sum_squares_type_string);
	}

	int compare(Computed_field_core* other_core)
	{
		Computed_field_nodeset_sum_squares *other =
			dynamic_cast<Computed_field_nodeset_sum_squares*>(other_core);
		if (other)
			return cmzn_nodeset_match(nodeset, other->get_nodeset());
		return 0;
	}

	virtual bool supports_sum_square_terms() const
	{
		return true;
	}

	virtual int get_number_of_sum_square_terms(cmzn_fieldcache& cache) const;

	virtual int evaluate_sum_square_terms(cmzn_fieldcache& cache, RealFieldValueCache& valueCache,
		int number_of_values, FE_value *values);

	virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
	{
		return this->evaluateDerivativeFiniteDifference(cache, inValueCache, fieldDerivative);
	}

	virtual int getDerivativeTreeOrder(const FieldDerivative& fieldDerivative)
	{
		const int sourceOrder = this->field->source_fields[0]->getDerivativeTreeOrder(fieldDerivative);
		return fieldDerivative.getProductTreeOrder(sourceOrder, sourceOrder);
	}

};

int Computed_field_nodeset_sum_squares::get_number_of_sum_square_terms(
	cmzn_fieldcache& cache) const
{
	// terms are only used with LEAST_SQUARES_QUASI_NEWTON optimisation
	// if ElementEvaluationMap is set, restricts sum to nodes with a valid element location in it
	cmzn_field *elementMap = this->getElementMapField();
	FE_field *elementXiField = nullptr;
	if (elementMap)
	{
		Computed_field_get_type_finite_element(elementMap, &elementXiField);
		if (!elementXiField)
		{
			display_message(ERROR_MESSAGE, "FieldNodesetEvaluator get_number_of_sum_square_terms:  Invalid element evaluation map field");
			return 0;
		}
		FE_mesh *hostMesh = elementXiField->getElementXiHostMesh();
		if (!hostMesh)
			return 0;  // no values
	}
	int number_of_terms = 0;
	cmzn_field_id sourceField = field->source_fields[0];
	cmzn_nodeiterator_id iterator = cmzn_nodeset_create_nodeiterator(nodeset);
	cmzn_node_id node = 0;
	cmzn_element *element;
	FE_value xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	while (0 != (node = cmzn_nodeiterator_next_non_access(iterator)))
	{
		if ((!elementXiField) || (get_FE_nodal_element_xi_value(node, elementXiField, /*component*/0, &element, xi) && (element)))
		{
			cache.setNode(node);
			if (sourceField->core->is_defined_at_location(cache))
				++number_of_terms;
		}
	}
	cmzn_nodeiterator_destroy(&iterator);
	return number_of_terms;
}

int Computed_field_nodeset_sum_squares::evaluate_sum_square_terms(
	cmzn_fieldcache& cache, RealFieldValueCache& valueCache, int number_of_values, FE_value *values)
{
	// terms are only used with LEAST_SQUARES_QUASI_NEWTON optimisation
	// if ElementEvaluationMap is set, restricts sum to nodes with a valid element location in it
	cmzn_field *elementMap = this->getElementMapField();
	FE_field *elementXiField = nullptr;
	if (elementMap)
	{
		Computed_field_get_type_finite_element(elementMap, &elementXiField);
		if (!elementXiField)
		{
			display_message(ERROR_MESSAGE, "FieldNodesetEvaluator get_number_of_sum_square_terms:  Invalid element evaluation map field");
			return 0;
		}
		FE_mesh *hostMesh = elementXiField->getElementXiHostMesh();
		if (!hostMesh)
			return 1;  // no values
	}
	cmzn_fieldcache& extraCache = *(valueCache.getExtraCache());
	extraCache.setTime(cache.getTime());
	int return_code = 1;
	int number_of_terms = 0;
	const int number_of_components = field->number_of_components;
	const int max_terms = number_of_values / number_of_components;
	FE_value *value = values;
	cmzn_field_id sourceField = getSourceField(0);
	int i;
	cmzn_nodeiterator_id iterator = cmzn_nodeset_create_nodeiterator(nodeset);
	cmzn_node_id node = 0;
	cmzn_element *element;
	FE_value xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	while (0 != (node = cmzn_nodeiterator_next_non_access(iterator)))
	{
		if ((!elementXiField) || (get_FE_nodal_element_xi_value(node, elementXiField, /*component*/0, &element, xi) && (element)))
		{
			extraCache.setNode(node);
			const RealFieldValueCache* sourceValueCache = RealFieldValueCache::cast(sourceField->evaluate(extraCache));
			if (sourceValueCache)
			{
				if (number_of_terms >= max_terms)
				{
					return_code = 0;
					break;
				}
				for (i = 0; i < number_of_components; i++)
				{
					*value = sourceValueCache->values[i];
					++value;
				}
				++number_of_terms;
			}
		}
	}
	cmzn_nodeiterator_destroy(&iterator);
	if (number_of_terms*number_of_components != number_of_values)
	{
		return_code = 0;
	}
	return return_code;
}

class TermOperatorSumSquares
{
	const int valuesCount;
	FE_value *values;

public:
	TermOperatorSumSquares(int valuesCountIn, FE_value *valuesIn) :
		valuesCount(valuesCountIn),
		values(valuesIn)
	{
		for (int i = 0; i < this->valuesCount; ++i)
			this->values[i] = 0.0;
	}

	inline void processTerm(const FE_value *sourceValues)
	{
		for (int i = 0; i < this->valuesCount; ++i)
			this->values[i] += sourceValues[i]*sourceValues[i];
	}
};

int Computed_field_nodeset_sum_squares::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
	TermOperatorSumSquares termSumSquares(this->field->number_of_components, valueCache.values);
	return this->evaluateNodesetOperator(cache, inValueCache, termSumSquares);
}


const char computed_field_nodeset_mean_squares_type_string[] = "nodeset_mean_squares";

class Computed_field_nodeset_mean_squares : public Computed_field_nodeset_sum_squares
{
public:
	Computed_field_nodeset_mean_squares(cmzn_nodeset_id nodeset_in) :
		Computed_field_nodeset_sum_squares(nodeset_in)
	{
	}

	Computed_field_core *copy()
	{
		return new Computed_field_nodeset_mean_squares(nodeset);
	}

	const char *get_type_string()
	{
		return (computed_field_nodeset_mean_squares_type_string);
	}

	int compare(Computed_field_core* other_core)
	{
		Computed_field_nodeset_mean_squares *other =
			dynamic_cast<Computed_field_nodeset_mean_squares*>(other_core);
		if (other)
			return cmzn_nodeset_match(nodeset, other->get_nodeset());
		return 0;
	}

	virtual int evaluate_sum_square_terms(cmzn_fieldcache& cache, RealFieldValueCache& valueCache,
		int number_of_values, FE_value *values);

	virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
	{
		return this->evaluateDerivativeFiniteDifference(cache, inValueCache, fieldDerivative);
	}

};

int Computed_field_nodeset_mean_squares::evaluate_sum_square_terms(
	cmzn_fieldcache& cache, RealFieldValueCache& valueCache, int number_of_values, FE_value *values)
{
	int return_code = Computed_field_nodeset_sum_squares::evaluate_sum_square_terms(cache, valueCache, number_of_values, values);
	if (return_code)
	{
		const int termCount = number_of_values / field->number_of_components;
		if (termCount > 0)
		{
			// use square root to get term values before squaring
			const FE_value scaling = 1.0 / sqrt(static_cast<FE_value>(termCount));
			for (int i = 0 ; i < number_of_values ; i++)
				values[i] *= scaling;
		}
		else
		{
			return_code = 0;
		}
	}
	return (return_code);
}

class TermOperatorSumSquaresCount
{
	const int valuesCount;
	FE_value *values;
	int termCount;

public:
	TermOperatorSumSquaresCount(int valuesCountIn, FE_value *valuesIn) :
		valuesCount(valuesCountIn),
		values(valuesIn),
		termCount(0)
	{
		for (int i = 0; i < this->valuesCount; ++i)
			this->values[i] = 0.0;
	}

	inline void processTerm(const FE_value *sourceValues)
	{
		for (int i = 0; i < this->valuesCount; ++i)
			this->values[i] += sourceValues[i]*sourceValues[i];
		++(this->termCount);
	}

	int getTermCount() const
	{
		return this->termCount;
	}
};

int Computed_field_nodeset_mean_squares::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
	TermOperatorSumSquaresCount termSumSquaresCount(this->field->number_of_components, valueCache.values);
	const int result = this->evaluateNodesetOperator(cache, inValueCache, termSumSquaresCount);
	if (result)
	{
		const int termCount = termSumSquaresCount.getTermCount();
		if (termCount > 0)
		{
			const FE_value scaling = 1.0 / static_cast<FE_value>(termCount);
			for (int i = 0; i < this->field->number_of_components; i++)
				valueCache.values[i] *= scaling;
			return 1;
		}
	}
	return 0;
}

const char computed_field_nodeset_minimum_type_string[] = "nodeset_min";

class Computed_field_nodeset_minimum : public Computed_field_nodeset_operator
{
public:
	Computed_field_nodeset_minimum(cmzn_nodeset_id nodeset_in) :
		Computed_field_nodeset_operator(nodeset_in)
	{
	}

	Computed_field_core *copy()
	{
		return new Computed_field_nodeset_minimum(nodeset);
	}

	const char *get_type_string()
	{
		return (computed_field_nodeset_minimum_type_string);
	}

	int compare(Computed_field_core* other_core)
	{
		Computed_field_nodeset_minimum *other =
			dynamic_cast<Computed_field_nodeset_minimum*>(other_core);
		if (other)
			return cmzn_nodeset_match(nodeset, other->get_nodeset());
		return 0;
	}

	virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
	{
		return this->evaluateDerivativeFiniteDifference(cache, inValueCache, fieldDerivative);
	}

};

class TermOperatorMinimum
{
	const int valuesCount;
	FE_value *values;
	bool first;

public:
	TermOperatorMinimum(int valuesCountIn, FE_value *valuesIn) :
		valuesCount(valuesCountIn),
		values(valuesIn),
		first(true)
	{
	}

	inline void processTerm(const FE_value *sourceValues)
	{
		if (this->first)
		{
			for (int i = 0; i < this->valuesCount; ++i)
				values[i] = sourceValues[i];
			this->first = false;
		}
		else
		{
			for (int i = 0; i < this->valuesCount; ++i)
				if (sourceValues[i] < this->values[i])
					this->values[i] = sourceValues[i];
		}
	}

	bool noValues() const
	{
		return this->first;
	}
};

int Computed_field_nodeset_minimum::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
	TermOperatorMinimum termMinimum(this->field->number_of_components, valueCache.values);
	const int result = this->evaluateNodesetOperator(cache, inValueCache, termMinimum);
	if (termMinimum.noValues())
		return 0;
	return result;
}

const char computed_field_nodeset_maximum_type_string[] = "nodeset_max";

class Computed_field_nodeset_maximum : public Computed_field_nodeset_operator
{
public:
	Computed_field_nodeset_maximum(cmzn_nodeset_id nodeset_in) :
		Computed_field_nodeset_operator(nodeset_in)
	{
	}

	Computed_field_core *copy()
	{
		return new Computed_field_nodeset_maximum(nodeset);
	}

	const char *get_type_string()
	{
		return (computed_field_nodeset_maximum_type_string);
	}

	int compare(Computed_field_core* other_core)
	{
		Computed_field_nodeset_maximum *other =
			dynamic_cast<Computed_field_nodeset_maximum*>(other_core);
		if (other)
			return cmzn_nodeset_match(nodeset, other->get_nodeset());
		return 0;
	}

	virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
	{
		return this->evaluateDerivativeFiniteDifference(cache, inValueCache, fieldDerivative);
	}

};

class TermOperatorMaximum
{
	const int valuesCount;
	FE_value *values;
	bool first;

public:
	TermOperatorMaximum(int valuesCountIn, FE_value *valuesIn) :
		valuesCount(valuesCountIn),
		values(valuesIn),
		first(true)
	{
	}

	inline void processTerm(const FE_value *sourceValues)
	{
		if (this->first)
		{
			for (int i = 0; i < this->valuesCount; ++i)
				values[i] = sourceValues[i];
			this->first = false;
		}
		else
		{
			for (int i = 0; i < this->valuesCount; ++i)
				if (sourceValues[i] > this->values[i])
					this->values[i] = sourceValues[i];
		}
	}

	bool noValues() const
	{
		return this->first;
	}
};

int Computed_field_nodeset_maximum::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
	TermOperatorMaximum termMaximum(this->field->number_of_components, valueCache.values);
	const int result = this->evaluateNodesetOperator(cache, inValueCache, termMaximum);
	if (termMaximum.noValues())
		return 0;
	return result;
}

} //namespace

cmzn_field_nodeset_operator_id cmzn_field_cast_nodeset_operator(cmzn_field_id field)
{
	if (field && (dynamic_cast<Computed_field_nodeset_operator*>(field->core)))
	{
		cmzn_field_access(field);
		return (reinterpret_cast<cmzn_field_nodeset_operator_id>(field));
	}
	return nullptr;
}

int cmzn_field_nodeset_operator_destroy(
	cmzn_field_nodeset_operator_id *nodeset_operator_field_address)
{
	return cmzn_field_destroy(reinterpret_cast<cmzn_field_id *>(nodeset_operator_field_address));
}

inline Computed_field_nodeset_operator *Computed_field_nodeset_operator_core_cast(
	cmzn_field_nodeset_operator *nodeset_operator_field)
{
	return (static_cast<Computed_field_nodeset_operator*>(
		reinterpret_cast<Computed_field*>(nodeset_operator_field)->core));
}

cmzn_field_id
cmzn_field_nodeset_operator_get_element_map_field(
	cmzn_field_nodeset_operator_id nodeset_operator_field)
{
	if (!nodeset_operator_field)
		return nullptr;
	Computed_field_nodeset_operator *nodeset_operator_core =
		Computed_field_nodeset_operator_core_cast(nodeset_operator_field);
	cmzn_field *element_map_field = nodeset_operator_core->getElementMapField();
	if (element_map_field)
		element_map_field->access();
	return element_map_field;
}

int cmzn_field_nodeset_operator_set_element_map_field(
	cmzn_field_nodeset_operator_id nodeset_operator_field,
	cmzn_field_id element_map_field)
{
	if (!nodeset_operator_field)
		return CMZN_ERROR_ARGUMENT;
	Computed_field_nodeset_operator *nodeset_operator_core =
		Computed_field_nodeset_operator_core_cast(nodeset_operator_field);
	return nodeset_operator_core->setElementMapField(element_map_field);
}

cmzn_field_id cmzn_fieldmodule_create_field_nodeset_sum(
	cmzn_fieldmodule_id field_module, cmzn_field_id source_field,
	cmzn_nodeset_id nodeset)
{
	cmzn_field_id field = 0;
	if (source_field && source_field->isNumerical() && nodeset &&
		(cmzn_fieldmodule_get_region_internal(field_module) ==
			cmzn_nodeset_get_region_internal(nodeset)))
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field->number_of_components,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_nodeset_sum(nodeset));
	}
	return field;
}

cmzn_field_id cmzn_fieldmodule_create_field_nodeset_mean(
	cmzn_fieldmodule_id field_module, cmzn_field_id source_field,
	cmzn_nodeset_id nodeset)
{
	cmzn_field_id field = 0;
	if (source_field && source_field->isNumerical() && nodeset &&
		(cmzn_fieldmodule_get_region_internal(field_module) ==
			cmzn_nodeset_get_region_internal(nodeset)))
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field->number_of_components,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_nodeset_mean(nodeset));
	}
	return field;
}

cmzn_field_id cmzn_fieldmodule_create_field_nodeset_sum_squares(
	cmzn_fieldmodule_id field_module, cmzn_field_id source_field,
	cmzn_nodeset_id nodeset)
{
	cmzn_field_id field = 0;
	if (source_field && source_field->isNumerical() && nodeset &&
		(cmzn_fieldmodule_get_region_internal(field_module) ==
			cmzn_nodeset_get_region_internal(nodeset)))
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field->number_of_components,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_nodeset_sum_squares(nodeset));
	}
	return field;
}

cmzn_field_id cmzn_fieldmodule_create_field_nodeset_mean_squares(
	cmzn_fieldmodule_id field_module, cmzn_field_id source_field,
	cmzn_nodeset_id nodeset)
{
	cmzn_field_id field = 0;
	if (source_field && source_field->isNumerical() && nodeset &&
		(cmzn_fieldmodule_get_region_internal(field_module) ==
			cmzn_nodeset_get_region_internal(nodeset)))
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field->number_of_components,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_nodeset_mean_squares(nodeset));
	}
	return field;
}

cmzn_field_id cmzn_fieldmodule_create_field_nodeset_minimum(
	cmzn_fieldmodule_id field_module, cmzn_field_id source_field,
	cmzn_nodeset_id nodeset)
{
	cmzn_field_id field = 0;
	if (source_field && source_field->isNumerical() && nodeset &&
		(cmzn_fieldmodule_get_region_internal(field_module) ==
			cmzn_nodeset_get_region_internal(nodeset)))
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field->number_of_components,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_nodeset_minimum(nodeset));
	}
	return field;
}

cmzn_field_id cmzn_fieldmodule_create_field_nodeset_maximum(
	cmzn_fieldmodule_id field_module, cmzn_field_id source_field,
	cmzn_nodeset_id nodeset)
{
	cmzn_field_id field = 0;
	if (source_field && source_field->isNumerical() && nodeset &&
		(cmzn_fieldmodule_get_region_internal(field_module) ==
			cmzn_nodeset_get_region_internal(nodeset)))
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field->number_of_components,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_nodeset_maximum(nodeset));
	}
	return field;
}

int cmzn_field_is_valid_nodeset_operator_element_map(cmzn_field_id field, void *)
{
	if (field)
	{
		cmzn_field_stored_mesh_location *stored_mesh_location = cmzn_field_cast_stored_mesh_location(field);
		if (stored_mesh_location)
		{
			cmzn_field_stored_mesh_location_destroy(&stored_mesh_location);
			return 1;
		}
	}
	return 0;
}

