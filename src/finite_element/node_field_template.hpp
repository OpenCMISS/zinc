/**
 * FILE : node_field_template.hpp
 *
 * Describes parameter type and version storage at a node for a scalar
 * field or field component.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (CMZN_NODE_FIELD_TEMPLATE_HPP)
#define CMZN_NODE_FIELD_TEMPLATE_HPP

#include "opencmiss/zinc/types/nodeid.h"
#include "general/value.h"
#include <vector>

struct FE_field;
class FE_element_field_template;
struct cmzn_element;
class FE_nodeset;
struct cmzn_node;
struct FE_time_sequence;

/**
 * Specifies how to find the value and derivative parameters for a field
 * component at a node. Supports variable numbers of version per node value
 * type. Parameters are stored in order of value types specified, and versions
 * for a given value type are consecutive.
 */
class FE_node_field_template
{
	friend int global_to_element_map_values(FE_field *field, int componentNumber,
		const FE_element_field_template *eft, cmzn_element *element, FE_value time,
		const FE_nodeset *nodeset, const FE_value *scaleFactors, FE_value*& elementValues);

	// the offset for the field component values within the node values storage
	int valuesOffset;
	// the total number of parameters, sum of versions for each value type
	int totalValuesCount;
	// number of value types (derivatives) defined
	int valueLabelsCount;
	// the value labels defined in order of their parameters, including VALUE
	cmzn_node_value_label *valueLabels;
	// the number of versions for each value type
	int *versionsCounts;

public:

	FE_node_field_template() :
		valuesOffset(-1), // negative until set
		totalValuesCount(0), // dynamically updated as modified
		valueLabelsCount(0),
		valueLabels(0),
		versionsCounts(0)
	{
	}

	FE_node_field_template(const FE_node_field_template &source);

	~FE_node_field_template()
	{
		delete[] valueLabels;
		delete[] versionsCounts;
	}

	FE_node_field_template &operator=(const FE_node_field_template &source);

	
	/** Call once after all value labels added (with 1 version each) to set same number
	  * of versions for all value labels. This is for EX version < 2.
	  * Note this consistent number of versions is consistent with parameters listed
	  * with value labels nested within version; see convertLegacyDOFIndexToValueLabelAndVersion
	  * Note the first valueLabel must be VALUE!
	  * @return  True on success, false if first valueLabel is not VALUE, if
	  * more than one version already specified for any valueLabel or versionCount < 1 */
	bool setLegacyAllValueVersionsCount(int versionsCount);

	/** @param compareValuesOffset  If set, compare valuesOffset.
	  * @return  True if this and other store the same parameters in the same order */
	bool matches(const FE_node_field_template& other, bool compareValuesOffset = false) const;

	/** @return  Highest numerical value of value label above VALUE */
	int getMaximumDerivativeNumber() const
	{
		int maximumDerivativeNumber = 0;
		for (int d = 0; d < this->valueLabelsCount; ++d)
		{
			const int derivativeNumber = (this->valueLabels[d] - CMZN_NODE_VALUE_LABEL_VALUE);
			if (derivativeNumber > maximumDerivativeNumber)
			{
				maximumDerivativeNumber = derivativeNumber;
			}
		}
		return maximumDerivativeNumber;
	}

	/** @return  Highest number of versions for any value label */
	int getMaximumVersionsCount() const
	{
		int versionsCount = 0;
		for (int d = 0; d < this->valueLabelsCount; ++d)
		{
			if (this->versionsCounts[d] > versionsCount)
			{
				versionsCount = this->versionsCounts[d];
			}
		}
		return versionsCount;
	}

	/** Get number of versions for value type
	  * @return  Number of versions >=0, or -1 if invalid valueLabel */
	int getValueNumberOfVersions(cmzn_node_value_label valueLabel) const;

	/** Defines a value type with the specified number of versions, or undefines it
	  * if numberOfVersions is 0
	  * Return result OK on success, otherwise any other result code. Note returns
	  * ALREADY_EXISTS if changing an existing value type which may or may not be
	  * successful, and NOT_FOUND if undefining a non-existent value type */
	int setValueNumberOfVersions(cmzn_node_value_label valueLabel, int numberOfVersions);

	int getTotalValuesCount() const
	{
		return this->totalValuesCount;
	}

	cmzn_node_value_label getValueLabelAtIndex(int index) const
	{
		if ((0 <= index) && (index < this->valueLabelsCount))
		{
			return this->valueLabels[index];
		}
		return CMZN_NODE_VALUE_LABEL_INVALID;
	}

	int getValueLabelsCount() const
	{
		return this->valueLabelsCount;
	}

	void addValuesOffset(int deltaValuesOffset)
	{
		this->valuesOffset += deltaValuesOffset;
	}

	int getValuesOffset() const
	{
		return this->valuesOffset;
	}

	void setValuesOffset(int valuesOffsetIn)
	{
		this->valuesOffset = valuesOffsetIn;
	}

	int getVersionsCountAtIndex(int index) const
	{
		if ((0 <= index) && (index < this->valueLabelsCount))
		{
			return this->versionsCounts[index];
		}
		return 0;
	}

	/** Get relative index of valueLabel, version within parameters for template.
	  * @return  Value index >= 0 for valueLabel and version, or -1 if not defined */
	inline int getValueIndex(cmzn_node_value_label valueLabel, int version) const
	{
		int d = 0;
		int valueIndex = version;
		// handle variable number of versions for each valueLabel
		while ((d < this->valueLabelsCount) && (this->valueLabels[d] != valueLabel))
		{
			valueIndex += this->versionsCounts[d];
			++d;
		}
		if ((d < valueLabelsCount) && (version < this->versionsCounts[d]))
		{
			return valueIndex;
		}
		return -1;
	}

	/** Get value label and version from relative index.
	 * @param valueIndex  Index from 0 to totalValuesCount - 1.
	 * @param version  If a value label is returned, set to version number starting at 0.
	 * @return  Value label, or INVALID if out of range. */
	inline cmzn_node_value_label getValueLabelAndVersion(int valueIndex, int& version) const
	{
		if (valueIndex > 0)
		{
			int index = valueIndex;
			for (int d = 0; d < this->valueLabelsCount; ++d)
			{
				if (index < this->versionsCounts[d])
				{
					version = index;
					return this->valueLabels[d];
				}
				index -= this->versionsCounts[d];
			}
		}
		version = -1;
		return CMZN_NODE_VALUE_LABEL_INVALID;
	}

	/** Convert legacy DOF index which 
	  * @param dofIndex  Legacy index of parameter at node. From 0 to number of DOFs at node - 1.
	  * @param valueLabel  On success, contains the node value label of the DOF index.
	  * @param version  On success, contains the version of the DOF index.
	  * @return  True on success, 0 if failed. */
	bool convertLegacyDOFIndexToValueLabelAndVersion(int dofIndex, cmzn_node_value_label &valueLabel, int &version) const;
};

#endif /* !defined (CMZN_NODE_FIELD_TEMPLATE_HPP) */
