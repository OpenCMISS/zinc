/**
 * @file elementfieldtemplate.hpp
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CMZN_ELEMENTFIELDTEMPLATE_HPP__
#define CMZN_ELEMENTFIELDTEMPLATE_HPP__

#include "opencmiss/zinc/elementfieldtemplate.h"
#include "opencmiss/zinc/elementbasis.hpp"
#include "opencmiss/zinc/node.hpp"

namespace OpenCMISS
{
namespace Zinc
{

class Elementfieldtemplate
{
private:

	cmzn_elementfieldtemplate_id id;

public:

	Elementfieldtemplate() : id(0)
	{ }

	// takes ownership of C handle, responsibility for destroying it
	explicit Elementfieldtemplate(cmzn_elementfieldtemplate_id elementfieldtemplate_id) :
		id(elementfieldtemplate_id)
	{ }

	Elementfieldtemplate(const Elementfieldtemplate& elementfieldtemplate) :
		id(cmzn_elementfieldtemplate_access(elementfieldtemplate.id))
	{ }

	Elementfieldtemplate& operator=(const Elementfieldtemplate& elementfieldemplate)
	{
		cmzn_elementfieldtemplate_id temp_id = cmzn_elementfieldtemplate_access(elementfieldemplate.id);
		if (0 != this->id)
			cmzn_elementfieldtemplate_destroy(&(this->id));
		this->id = temp_id;
		return *this;
	}

	~Elementfieldtemplate()
	{
		cmzn_elementfieldtemplate_destroy(&(this->id));
	}

	enum ParameterMappingMode
	{
		PARAMETER_MAPPING_MODE_INVALID = CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_INVALID,
		PARAMETER_MAPPING_MODE_ELEMENT = CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_ELEMENT,
		PARAMETER_MAPPING_MODE_FIELD = CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_FIELD,
		PARAMETER_MAPPING_MODE_NODE = CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE
	};

	enum ScaleFactorType
	{
		SCALE_FACTOR_TYPE_INVALID = CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_INVALID,
		SCALE_FACTOR_TYPE_ELEMENT_GENERAL = CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_ELEMENT_GENERAL,
		SCALE_FACTOR_TYPE_ELEMENT_PATCH = CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_ELEMENT_PATCH,
		SCALE_FACTOR_TYPE_GLOBAL_GENERAL = CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_GLOBAL_GENERAL,
		SCALE_FACTOR_TYPE_GLOBAL_PATCH = CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_GLOBAL_PATCH,
		SCALE_FACTOR_TYPE_NODE_GENERAL = CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_NODE_GENERAL,
		SCALE_FACTOR_TYPE_NODE_PATCH = CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_NODE_PATCH
	};

	bool isValid() const
	{
		return (0 != this->id);
	}

	cmzn_elementfieldtemplate_id getId() const
	{
		return this->id;
	}

	Elementbasis getElementbasis() const
	{
		return Elementbasis(cmzn_elementfieldtemplate_get_elementbasis(this->id));
	}

	int getFunctionNumberOfTerms(int functionNumber) const
	{
		return cmzn_elementfieldtemplate_get_function_number_of_terms(this->id, functionNumber);
	}

	int setFunctionNumberOfTerms(int functionNumber, int newNumberOfTerms)
	{
		return cmzn_elementfieldtemplate_set_function_number_of_terms(this->id, functionNumber, newNumberOfTerms);
	}

	int getNumberOfFunctions() const
	{
		return cmzn_elementfieldtemplate_get_number_of_functions(this->id);
	}

	int getNumberOfLocalNodes() const
	{
		return cmzn_elementfieldtemplate_get_number_of_local_nodes(this->id);
	}

	int setNumberOfLocalNodes(int number)
	{
		return cmzn_elementfieldtemplate_set_number_of_local_nodes(this->id, number);
	}

	int getNumberOfLocalScaleFactors() const
	{
		return cmzn_elementfieldtemplate_get_number_of_local_scale_factors(this->id);
	}

	int setNumberOfLocalScaleFactors(int number)
	{
		return cmzn_elementfieldtemplate_set_number_of_local_scale_factors(this->id, number);
	}

	ParameterMappingMode getParameterMappingMode() const
	{
		return static_cast<ParameterMappingMode>(cmzn_elementfieldtemplate_get_parameter_mapping_mode(this->id));
	}

	int setParameterMappingMode(ParameterMappingMode mode)
	{
		return cmzn_elementfieldtemplate_set_parameter_mapping_mode(this->id,
			static_cast<cmzn_elementfieldtemplate_parameter_mapping_mode>(mode));
	}

	int getScaleFactorIdentifier(int localScaleFactorIndex) const
	{
		return cmzn_elementfieldtemplate_get_scale_factor_identifier(this->id, localScaleFactorIndex);
	}

	int setScaleFactorIdentifier(int localScaleFactorIndex, int identifier)
	{
		return cmzn_elementfieldtemplate_set_scale_factor_identifier(this->id, localScaleFactorIndex, identifier);
	}

	ScaleFactorType getScaleFactorType(int localScaleFactorIndex) const
	{
		return static_cast<ScaleFactorType>(
			cmzn_elementfieldtemplate_get_scale_factor_type(this->id, localScaleFactorIndex));
	}

	int setScaleFactorType(int localScaleFactorIndex, ScaleFactorType type)
	{
		return cmzn_elementfieldtemplate_set_scale_factor_type(this->id,
			localScaleFactorIndex, static_cast<cmzn_elementfieldtemplate_scale_factor_type>(type));
	}

	int getTermLocalNodeIndex(int functionNumber, int term) const
	{
		return cmzn_elementfieldtemplate_get_term_local_node_index(this->id, functionNumber, term);
	}

	Node::ValueLabel getTermNodeValueLabel(int functionNumber, int term) const
	{
		return static_cast<Node::ValueLabel>(cmzn_elementfieldtemplate_get_term_node_value_label(this->id, functionNumber, term));
	}

	int getTermNodeVersion(int functionNumber, int term) const
	{
		return cmzn_elementfieldtemplate_get_term_node_version(this->id, functionNumber, term);
	}

	int setTermNodeParameter(int functionNumber, int term, int localNodeIndex, Node::ValueLabel nodeValueLabel, int version)
	{
		return cmzn_elementfieldtemplate_set_term_node_parameter(this->id, functionNumber, term,
			localNodeIndex, static_cast<cmzn_node_value_label>(nodeValueLabel), version);
	}

	int getTermScaling(int functionNumber, int term, int indexesCount, int *indexesOut) const
	{
		return cmzn_elementfieldtemplate_get_term_scaling(this->id, functionNumber, term, indexesCount, indexesOut);
	}

	int setTermScaling(int functionNumber, int term, int indexesCount, const int *indexesIn)
	{
		return cmzn_elementfieldtemplate_set_term_scaling(this->id, functionNumber, term, indexesCount, indexesIn);
	}

	bool validate()
	{
		return cmzn_elementfieldtemplate_validate(this->id);
	}

};

}  // namespace Zinc
}

#endif /* CMZN_ELEMENTFIELDTEMPLATE_HPP__ */
