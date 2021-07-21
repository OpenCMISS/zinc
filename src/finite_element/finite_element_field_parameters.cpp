/**
 * FILE : finite_element_field_parameters.cpp
 *
 * Records field parameter indexing.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "opencmiss/zinc/result.h"
#include "finite_element/finite_element_field.hpp"
#include "finite_element/finite_element_field_parameters.hpp"
#include "finite_element/finite_element_nodeset.hpp"
#include "finite_element/finite_element_private.h"
#include "finite_element/finite_element_region_private.h"
#include "general/message.h"
#include <cmath>
#include <vector>


FE_field_parameters::FE_field_parameters(FE_field *fieldIn) :
	field(fieldIn->access()),
	parameterCount(0),
	nodeParameterMap(/*blockLengthIn*/256, /*allocInitValueIn*/DS_LABEL_INDEX_INVALID),  // Assumes DS_LABEL_INDEX_INVALID == -1
	parameterNodeMap(/*blockLengthIn*/256, /*allocInitValueIn*/DS_LABEL_INDEX_INVALID),  // Assumes DS_LABEL_INDEX_INVALID == -1
	perturbationDelta(1.0E-5),
	access_count(1)
{
}

FE_field_parameters::~FE_field_parameters()
{
	this->field->clear_FE_field_parameters();
	FE_field::deaccess(&(this->field));
}

void FE_field_parameters::generateMaps()
{
	this->nodeParameterMap.clear();
	this->parameterNodeMap.clear();
	FE_region *feRegion = this->field->get_FE_region();
	FE_nodeset *feNodeset = FE_region_find_FE_nodeset_by_field_domain_type(feRegion, CMZN_FIELD_DOMAIN_TYPE_NODES);
	cmzn_nodeiterator *nodeIter = feNodeset->createNodeiterator();
	FE_node_field_info *lastFieldInfo = nullptr;
	const FE_node_field *nodeField = nullptr;
	DsLabelIndex nodeValuesCount = 0;
	cmzn_node *node;
	DsLabelIndex parameterIndex = 0;
	// calculate value ranges to calculate perturbationDelta
	const int componentCount = this->field->getNumberOfComponents();
	std::vector<FE_value> minimumValues;
	std::vector<FE_value> maximumValues;
	std::vector<FE_value> values(componentCount);
	int nodeRangeCount = 0;
	while ((node = nodeIter->nextNode()) != nullptr)
	{
		if (node->fields != lastFieldInfo)
		{
			lastFieldInfo = node->fields;
			nodeField = node->getNodeField(this->field);
			if (!nodeField)
			{
				// not defined on node
				nodeValuesCount = 0;
				continue;
			}
			nodeValuesCount = nodeField->getTotalValuesCount();
		}
		else if (nodeValuesCount == 0)
		{
			continue;  // not defined on node
		}
		this->nodeParameterMap.setValue(node->getIndex(), parameterIndex);
		this->parameterNodeMap.setValues(parameterIndex, nodeValuesCount, node->getIndex());
		parameterIndex += nodeValuesCount;
		if (get_FE_nodal_FE_value_value(node, this->field, /*component*/-1, CMZN_NODE_VALUE_LABEL_VALUE, /*version*/0, /*time*/0.0, values.data()))
		{
			if (nodeRangeCount)
			{
				for (int c = 0; c < componentCount; ++c)
				{
					if (values[c] < minimumValues[c])
						minimumValues[c] = values[c];
					else if (values[c] > maximumValues[c])
						maximumValues[c] = values[c];
				}
			}
			else
			{
				minimumValues = values;
				maximumValues = values;
			}
			++nodeRangeCount;
		}
	}
	this->parameterCount = parameterIndex;
	// following could be improved knowing the number of elements
	// basically want a fraction of typical or minimum element span
	FE_value maxRange = 0.0;
	if (nodeRangeCount > 1)
		for (int c = 0; c < componentCount; ++c)
		{
			const FE_value range = maximumValues[c] - minimumValues[c];
			if (range > maxRange)
				maxRange = range;
		}
	if ((maxRange == 0.0) && (nodeRangeCount))
		for (int c = 0; c < componentCount; ++c)
		{
			const FE_value range = fabs(maximumValues[c]);
			if (range > maxRange)
				maxRange = range;
		}
	this->perturbationDelta = ((maxRange > 0.0) ? maxRange : 1.0)*1.0E-5;
}

FE_field_parameters *FE_field_parameters::create(FE_field *fieldIn)
{
	if ((fieldIn) && (fieldIn->get_FE_field_type() == GENERAL_FE_FIELD) && (fieldIn->getValueType() == FE_VALUE_VALUE))
		return new FE_field_parameters(fieldIn);
	display_message(ERROR_MESSAGE, "FE_field_parameters::create.  Missing or invalid field");
	return nullptr;
}

int FE_field_parameters::deaccess(FE_field_parameters* &fe_field_parameters)
{
	if (!fe_field_parameters)
		return CMZN_RESULT_ERROR_ARGUMENT;
	--(fe_field_parameters->access_count);
	if (fe_field_parameters->access_count <= 0)
		delete fe_field_parameters;
	fe_field_parameters = 0;
	return CMZN_RESULT_OK;
}

int FE_field_parameters::getElementParameterIndexes(cmzn_element *element, int valuesCount, int *valuesOut, int startIndex)
{
	if (!((element) && (valuesCount >= 0) && (valuesOut)))
	{
		display_message(ERROR_MESSAGE, "Fieldparameters getElementParameterIndexes:  Invalid argument(s)");
		return CMZN_RESULT_ERROR_ARGUMENT;
	}
	const FE_mesh_field_data *meshFieldData = this->field->getMeshFieldData(element->getMesh());
	if (!meshFieldData)
		return CMZN_RESULT_ERROR_NOT_FOUND;
	FE_region *feRegion = this->field->get_FE_region();
	const FE_nodeset *feNodeset = FE_region_find_FE_nodeset_by_field_domain_type(feRegion, CMZN_FIELD_DOMAIN_TYPE_NODES);
	int elementParameterCount = 0;
	const DsLabelIndex elementIndex = element->getIndex();
	FE_mesh *mesh = element->getMesh();
	const int componentCount = this->field->getNumberOfComponents();
	// cache last values to save lookup for subsequent components or nodes
	const FE_element_field_template *lastEft = nullptr;
	const DsLabelIndex *nodeIndexes = nullptr;
	const FE_node_field_info *lastNodeFieldInfo = nullptr;
	const FE_node_field *nodeField = nullptr;
	for (int c = 0; c < componentCount; ++c)
	{
		const FE_mesh_field_template *meshFieldTemplate = meshFieldData->getComponentMeshfieldtemplate(c);
		const FE_element_field_template *eft = meshFieldTemplate->getElementfieldtemplate(element->getIndex());
		if (!eft)
			return CMZN_RESULT_ERROR_NOT_FOUND;
		if (eft->getParameterMappingMode() != CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE)
			continue;  // not implemented, skipped for now
		const int componentParameterCount = eft->getParameterCount();
		if ((elementParameterCount + componentParameterCount) > valuesCount)
		{
			display_message(ERROR_MESSAGE, "Fieldparameters getElementParameterIndexes:  Too many values for output array size");
			return CMZN_RESULT_ERROR_ARGUMENT;
		}
		if (eft != lastEft)
		{
			// optimisation for same eft in consecutive components
			FE_mesh_element_field_template_data *meshEftData = mesh->getElementfieldtemplateData(eft->getIndexInMesh());
			nodeIndexes = meshEftData->getElementNodeIndexes(elementIndex);
			if (!nodeIndexes)
			{
				display_message(ERROR_MESSAGE, "Fieldparameters getElementParameterIndexes.  "
					"Element %d field %s component %d missing local-to-global node map.",
					element->getIdentifier(), this->field->getName(), c + 1);
				return CMZN_RESULT_ERROR_GENERAL;  // add ERROR_INCOMPLETE_DATA?
			}
			lastEft = eft;
		}
		for (int p = 0; p < componentParameterCount; ++p)
		{
			// only need first parameter term to get parameter index
			int term;
			const int func = eft->getParameterTermFunctionAndTerm(p, /*parameterTerm*/0, term);
			const int localNodeIndex = eft->getTermLocalNodeIndex(func, term);
			const DsLabelIndex nodeIndex = nodeIndexes[localNodeIndex];
			cmzn_node *node = feNodeset->getNode(nodeIndex);
			if (!node)
			{
				display_message(ERROR_MESSAGE, "Fieldparameters getElementParameterIndexes.  "
					"Element %d field %s component %d missing global node for local node %d.",
					element->getIdentifier(), field->getName(), c + 1, localNodeIndex + 1);
				return CMZN_RESULT_ERROR_GENERAL;  // add ERROR_INCOMPLETE_DATA?
			}
			if (node->getNodeFieldInfo() != lastNodeFieldInfo)
			{
				nodeField = node->getNodeField(field);
				if (!nodeField)
				{
					display_message(ERROR_MESSAGE, "Fieldparameters getElementParameterIndexes.  "
						"Element %d field %s (component %d) not defined at node %d mapped by local node %d",
						element->getIdentifier(), field->getName(), c + 1, node->getIdentifier(), localNodeIndex + 1);
					return CMZN_RESULT_ERROR_GENERAL;  // add ERROR_INCOMPLETE_DATA?
				}
				lastNodeFieldInfo = node->getNodeFieldInfo();
			}
			const cmzn_node_value_label nodeValueLabel = eft->getTermNodeValueLabel(func, term);
			const int version = eft->getTermNodeVersion(func, term);
			const FE_node_field_template *nft = nodeField->getComponent(c);
			const int valueIndex = nft->getValueIndex(nodeValueLabel, version);
			if (valueIndex < 0)
			{
				display_message(ERROR_MESSAGE, "Fieldparameters getElementParameterIndexes.  "
					"Element %d field %s component %d parameter %s version %d not found at node %d mapped by local node %d",
					element->getIdentifier(), field->getName(), c + 1, ENUMERATOR_STRING(cmzn_node_value_label)(nodeValueLabel),
					version + 1, node->getIdentifier(), localNodeIndex + 1);
				return CMZN_RESULT_ERROR_GENERAL;  // add ERROR_INCOMPLETE_DATA?
			}
			DsLabelIndex parameterIndex = this->nodeParameterMap.getValue(nodeIndex);
			if (parameterIndex < 0)
			{
				display_message(ERROR_MESSAGE, "Fieldparameters getElementParameterIndexes.  "
					"Element %d field %s component %d no parameters mapped for node %d mapped by local node %d",
					element->getIdentifier(), field->getName(), c + 1, node->getIdentifier(), localNodeIndex + 1);
				return CMZN_RESULT_ERROR_GENERAL;
			}
			// offset by previous component values counts
			for (int pc = 0; pc < c; ++pc)
				parameterIndex += nodeField->getComponent(pc)->getTotalValuesCount();
			valuesOut[elementParameterCount] = parameterIndex + valueIndex + startIndex;
			++elementParameterCount;
		}
	}
	return CMZN_RESULT_OK;
}

cmzn_node *FE_field_parameters::getNodeParameter(int parameterIndex, int &fieldComponent, cmzn_node_value_label& valueLabel, int& version)
{
	fieldComponent = -1;
	valueLabel = CMZN_NODE_VALUE_LABEL_INVALID;
	version = -1;
	if ((parameterIndex < 0) || (parameterIndex >= this->parameterCount))
	{
		display_message(ERROR_MESSAGE, "Fieldparameters getNodeParameter:  Invalid parameter index");
		return nullptr;
	}
	FE_region *feRegion = this->field->get_FE_region();
	const FE_nodeset *feNodeset = FE_region_find_FE_nodeset_by_field_domain_type(feRegion, CMZN_FIELD_DOMAIN_TYPE_NODES);
	DsLabelIndex nodeIndex = this->parameterNodeMap.getValue(parameterIndex);
	cmzn_node *node = feNodeset->getNode(nodeIndex);
	if (!node)
	{
		display_message(ERROR_MESSAGE, "Fieldparameters getNodeParameter:  Missing node");
	}
	else
	{
		const DsLabelIndex startParameterIndex = this->nodeParameterMap.getValue(nodeIndex);
		const FE_node_field *nodeField = node->getNodeField(this->field);
		if (!nodeField)
		{
			display_message(ERROR_MESSAGE, "Fieldparameters getNodeParameter:  Missing node field for node %d", node->getIdentifier());
		}
		else
		{
			DsLabelIndex parameterOffset = parameterIndex - startParameterIndex;
			const int componentCount = this->field->getNumberOfComponents();
			for (int c = 0; c < componentCount; ++c)
			{
				const FE_node_field_template *nft = nodeField->getComponent(c);
				const int componentValuesCount = nft->getTotalValuesCount();
				if (parameterOffset < componentValuesCount)
				{
					fieldComponent = c;
					valueLabel = nft->getValueLabelAndVersion(parameterOffset, version);
					return node;
				}
				parameterOffset -= componentValuesCount;
			}
			display_message(ERROR_MESSAGE, "Fieldparameters getNodeParameter:  Parameter out of range for node %d", node->getIdentifier());
		}
	}
	return nullptr;
}

int FE_field_parameters::getNumberOfElementParameters(cmzn_element *element)
{
	if (!element)
	{
		display_message(ERROR_MESSAGE, "Fieldparameters getNumberOfElementParameters:  Invalid element");
		return -1;
	}
	const FE_mesh_field_data *meshFieldData = this->field->getMeshFieldData(element->getMesh());
	if (!meshFieldData)
		return 0;  // not defined on this element's mesh
	int elementParameterCount = 0;
	const int componentCount = this->field->getNumberOfComponents();
	for (int c = 0; c < componentCount; ++c)
	{
		const FE_mesh_field_template *meshFieldTemplate = meshFieldData->getComponentMeshfieldtemplate(c);
		const FE_element_field_template *eft = meshFieldTemplate->getElementfieldtemplate(element->getIndex());
		if (!eft)
			return 0;  // not defined on this element
		if (eft->getParameterMappingMode() != CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE)
		{
			display_message(WARNING_MESSAGE, "Fieldparameters getNumberOfElementParameters:  Not implemented for non-nodal parameters: ",
				"Element %d component %d. Skipping.", element->getIdentifier(), c + 1);
			continue;  // not implemented, skipped for now
		}
		elementParameterCount += eft->getParameterCount();
	}
	return elementParameterCount;
}

int FE_field_parameters::getNumberOfParameters()
{
	// force parameter maps to be rebuilt as simpler than working out when it's changed
	this->generateMaps();
	return this->parameterCount;
}

template <class ProcessValuesOperator> int FE_field_parameters::processParameters(ProcessValuesOperator& processValues)
{
	if (!processValues.checkValues(this->parameterCount))
	{
		display_message(ERROR_MESSAGE, "Fieldparameters %s:  Invalid argument(s)", processValues.getApiName());
		return CMZN_RESULT_ERROR_ARGUMENT;
	}
	FE_region *feRegion = this->field->get_FE_region();
	FE_nodeset *feNodeset = FE_region_find_FE_nodeset_by_field_domain_type(feRegion, CMZN_FIELD_DOMAIN_TYPE_NODES);
	cmzn_nodeiterator *nodeIter = feNodeset->createNodeiterator();
	FE_node_field_info *lastFieldInfo = nullptr;
	const FE_node_field *nodeField = nullptr;
	DsLabelIndex nodeValuesCount = 0;
	cmzn_node *node;
	const int componentCount = this->field->getNumberOfComponents();
	int valueIndex = 0;
	while ((node = nodeIter->nextNode()) != nullptr)
	{
		if (node->fields != lastFieldInfo)
		{
			lastFieldInfo = node->fields;
			nodeField = node->getNodeField(this->field);
			if (!nodeField)
			{
				// not defined on node
				nodeValuesCount = 0;
				continue;
			}
			nodeValuesCount = nodeField->getTotalValuesCount();
		}
		else if (nodeValuesCount == 0)
		{
			continue;  // not defined on node
		}
		if ((valueIndex + nodeValuesCount) > this->parameterCount)
		{
			display_message(ERROR_MESSAGE, "Fieldparameters %s:  Not enough values supplied", processValues.getApiName());
			return CMZN_RESULT_ERROR_ARGUMENT;
		}
		for (int c = 0; c < componentCount; ++c)
		{
			const FE_node_field_template *nft = nodeField->getComponent(c);
			const int componentValuesCount = nft->getTotalValuesCount();
			if (!processValues(node, this->field, c, /*time*/0.0, componentValuesCount, valueIndex))
			{
				display_message(ERROR_MESSAGE, "Fieldparameters %s:  Failed to process node field component", processValues.getApiName());
				return CMZN_RESULT_ERROR_GENERAL;
			}
			valueIndex += componentValuesCount;
		}
	}
	return CMZN_RESULT_OK;
}

namespace {

template <typename ValueType> class ProcessValuesOperatorBase
{
protected:
	const int valuesCount;
	ValueType *values;

public:
	ProcessValuesOperatorBase(int valuesCountIn, ValueType *valuesIn) :
		valuesCount(valuesCountIn),
		values(valuesIn)
	{
	}

	bool checkValues(int minimumValueCount)
	{
		return (this->values) && (this->valuesCount >= minimumValueCount);
	}
};

template <typename ValueType> class ProcessValuesOperatorModify : public ProcessValuesOperatorBase<ValueType>
{
private:
	FE_region *feRegion;

public:
	ProcessValuesOperatorModify(FE_region *feRegionIn, int valuesCountIn, ValueType *valuesIn) :
		ProcessValuesOperatorBase<ValueType>(valuesCountIn, valuesIn),
		feRegion(feRegionIn)
	{
		FE_region_begin_change(feRegion);
	}

	~ProcessValuesOperatorModify()
	{
		FE_region_end_change(feRegion);
	}
};

}  // anonymous namespace

int FE_field_parameters::addParameters(int valuesCount, const FE_value *valuesIn)
{
	class ProcessValuesOperatorAdd : public ProcessValuesOperatorModify<const FE_value>
	{
	public:
		ProcessValuesOperatorAdd(FE_region *feRegionIn, int valuesCountIn, const FE_value *valuesIn) :
			ProcessValuesOperatorModify(feRegionIn, valuesCountIn, valuesIn)
		{
		}

		inline int operator() (cmzn_node *node, FE_field *field, int componentNumber, FE_value time, int processValuesCount, int valueIndex)
		{
			return cmzn_node_add_field_component_FE_value_values(node, field, componentNumber,
				/*time*/0.0, processValuesCount, this->values + valueIndex);
		}

		const char *getApiName()
		{
			return "addParameters";
		}
	};
	ProcessValuesOperatorAdd processAdd(this->field->get_FE_region(), valuesCount, valuesIn);
	return this->processParameters(processAdd);
}

int FE_field_parameters::getParameters(int valuesCount, FE_value *valuesOut)
{
	class ProcessValuesOperatorGet : public ProcessValuesOperatorBase<FE_value>
	{
	public:
		ProcessValuesOperatorGet(int valuesCountIn, FE_value *valuesIn) :
			ProcessValuesOperatorBase(valuesCountIn, valuesIn)
		{
		}

		inline int operator() (cmzn_node *node, FE_field *field, int componentNumber, FE_value time, int processValuesCount, int valueIndex)
		{
			return cmzn_node_get_field_component_FE_value_values(node, field, componentNumber,
				/*time*/0.0, processValuesCount, this->values + valueIndex);
		}

		const char *getApiName()
		{
			return "getParameters";
		}
	};
	ProcessValuesOperatorGet processGet(valuesCount, valuesOut);
	return this->processParameters(processGet);
}

int FE_field_parameters::setParameters(int valuesCount, const FE_value *valuesIn)
{
	class ProcessValuesOperatorSet : public ProcessValuesOperatorModify<const FE_value>
	{
	public:
		ProcessValuesOperatorSet(FE_region *feRegionIn, int valuesCountIn, const FE_value *valuesIn) :
			ProcessValuesOperatorModify(feRegionIn, valuesCountIn, valuesIn)
		{
		}

		inline int operator() (cmzn_node *node, FE_field *field, int componentNumber, FE_value time, int processValuesCount, int valueIndex)
		{
			return cmzn_node_set_field_component_FE_value_values(node, field, componentNumber,
				/*time*/0.0, processValuesCount, this->values + valueIndex);
		}

		const char *getApiName()
		{
			return "setParameters";
		}
	};
	ProcessValuesOperatorSet processSet(this->field->get_FE_region(), valuesCount, valuesIn);
	return this->processParameters(processSet);
}
