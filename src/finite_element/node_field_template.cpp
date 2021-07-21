/**
 * FILE : node_field_template.cpp
 *
 * Describes parameter type and version storage at a node for a scalar
 * field or field component.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "opencmiss/zinc/status.h"
#include "finite_element/node_field_template.hpp"
#include "general/message.h"
#include <cstring>
#include <vector>


namespace {

	/** @return  Allocated copy of sourceValues array containing count values, or 0 if none */
	template<typename VALUETYPE> VALUETYPE *copy_array(VALUETYPE *sourceValues, int count)
	{
		if (!sourceValues)
			return 0;
		VALUETYPE *values = new VALUETYPE[count];
		memcpy(values, sourceValues, count*sizeof(VALUETYPE));
		return values;
	}

} // anonymous namespace

FE_node_field_template::FE_node_field_template(const FE_node_field_template &source) :
	valuesOffset(source.valuesOffset),
	totalValuesCount(source.totalValuesCount),
	valueLabelsCount(source.valueLabelsCount),
	valueLabels(copy_array(source.valueLabels, source.valueLabelsCount)),
	versionsCounts(copy_array(source.versionsCounts, source.valueLabelsCount))
{
}

FE_node_field_template &FE_node_field_template::operator=(const FE_node_field_template &source)
{
	FE_node_field_template tmp(source);
	this->valuesOffset = tmp.valuesOffset;
	this->totalValuesCount = tmp.totalValuesCount;
	this->valueLabelsCount = tmp.valueLabelsCount;
	// swap arrays with tmp
	auto tmpValueLabels = this->valueLabels;
	this->valueLabels = tmp.valueLabels;
	tmp.valueLabels = tmpValueLabels;
	auto tmpVersionsCounts = this->versionsCounts;
	this->versionsCounts = tmp.versionsCounts;
	tmp.versionsCounts = tmpVersionsCounts;
	return *this;
}

bool FE_node_field_template::setLegacyAllValueVersionsCount(int versionsCount)
{
	if ((this->valueLabelsCount < 1)
		|| (this->valueLabels[0] != CMZN_NODE_VALUE_LABEL_VALUE)
		|| (versionsCount < 1)
		|| (this->getMaximumVersionsCount() > 1))
	{
		display_message(ERROR_MESSAGE, "FE_node_field_template::setLegacyAllValueLabelsVersionCount.  Invalid argument(s)");
		return false;
	}
	for (int d = 0; d < this->valueLabelsCount; ++d)
	{
		this->versionsCounts[d] = versionsCount;
	}
	this->totalValuesCount *= versionsCount;
	return true;
}

bool FE_node_field_template::matches(const FE_node_field_template& other, bool compareValuesOffset) const
{
	if ((this->totalValuesCount != other.totalValuesCount)
		|| (this->valueLabelsCount != other.valueLabelsCount))
	{
		return false;
	}
	if (compareValuesOffset && (this->valuesOffset != other.valuesOffset))
	{
		return false;
	}
	for (int d = 0; d < this->valueLabelsCount; ++d)
	{
		if ((this->valueLabels[d] != other.valueLabels[d])
			|| (this->versionsCounts[d] != other.versionsCounts[d]))
		{
			return false;
		}
	}
	return true;
}

int FE_node_field_template::getValueNumberOfVersions(cmzn_node_value_label valueLabel) const
{
	if ((valueLabel < CMZN_NODE_VALUE_LABEL_VALUE) || (valueLabel > CMZN_NODE_VALUE_LABEL_D3_DS1DS2DS3))
	{
		display_message(ERROR_MESSAGE, "FE_node_field_template::getValueNumberOfVersions.  Invalid value label");
		return -1;
	}
	for (int i = 0; (i < this->valueLabelsCount); ++i)
	{
		if (this->valueLabels[i] == valueLabel)
		{
			return this->versionsCounts[i];
		}
	}
	return 0;
}

int FE_node_field_template::setValueNumberOfVersions(cmzn_node_value_label valueLabel, int numberOfVersions)
{
	if ((valueLabel < CMZN_NODE_VALUE_LABEL_VALUE) || (valueLabel > CMZN_NODE_VALUE_LABEL_D3_DS1DS2DS3)
		|| (numberOfVersions < 0))
	{
		display_message(ERROR_MESSAGE, "FE_node_field_template::setValueNumberOfVersions.  Invalid argument(s)");
		return CMZN_ERROR_ARGUMENT;
	}
	int i = 0;
	while ((i < this->valueLabelsCount) && (this->valueLabels[i] != valueLabel))
	{
		++i;
	}
	if (i < this->valueLabelsCount)
	{
		this->totalValuesCount += (numberOfVersions - this->versionsCounts[i]);
		if (numberOfVersions == 0)
		{
			--(this->valueLabelsCount);
			for (int j = i; j < this->valueLabelsCount; ++j)
			{
				this->valueLabels[j] = this->valueLabels[j + 1];
				this->versionsCounts[j] = this->versionsCounts[j + 1];
			}
			return CMZN_OK;
		}
		this->versionsCounts[i] = numberOfVersions;
		return CMZN_ERROR_ALREADY_EXISTS;
	}
	if (numberOfVersions == 0)
	{
		display_message(WARNING_MESSAGE, "FE_node_field_template::setValueNumberOfVersions.  Value label not found");
		return CMZN_ERROR_NOT_FOUND;
	}
	cmzn_node_value_label *newValueLabels = new cmzn_node_value_label[this->valueLabelsCount + 1];
	int *newVersionsCounts = new int[this->valueLabelsCount + 1];
	if (!((newValueLabels) && (newVersionsCounts)))
	{
		delete[] newValueLabels;
		delete[] newVersionsCounts;
		return CMZN_ERROR_MEMORY;
	}
	for (int j = 0; j < this->valueLabelsCount; ++j)
	{
		newValueLabels[j] = this->valueLabels[j];
		newVersionsCounts[j] = this->versionsCounts[j];
	}
	newValueLabels[this->valueLabelsCount] = valueLabel;
	newVersionsCounts[this->valueLabelsCount] = numberOfVersions;
	delete[] this->valueLabels;
	delete[] this->versionsCounts;
	++(this->valueLabelsCount);
	this->valueLabels = newValueLabels;
	this->versionsCounts = newVersionsCounts;
	this->totalValuesCount += numberOfVersions;
	return CMZN_OK;
}

bool FE_node_field_template::convertLegacyDOFIndexToValueLabelAndVersion(int dofIndex, cmzn_node_value_label &valueLabel, int &version) const
{
	if (dofIndex < 0)
	{
		display_message(WARNING_MESSAGE, "FE_node_field_template::convertLegacyDOFIndexToValueLabelAndVersion.  Negative DOF index");
		return false;
	}
	if ((!this->valueLabels) || (this->valueLabelsCount < 1))
	{
		display_message(WARNING_MESSAGE, "FE_node_field_template::convertLegacyDOFIndexToValueLabelAndVersion.  No value labels");
		return false;
	}
	if (this->valueLabels[0] != CMZN_NODE_VALUE_LABEL_VALUE)
	{
		display_message(WARNING_MESSAGE, "FE_node_field_template::convertLegacyDOFIndexToValueLabelAndVersion.  "
			"Require first value label to be VALUE for legacy conversion");
		return false;
	}
	const int versionsCount = this->versionsCounts[0];
	for (int d = 1; d < this->valueLabelsCount; ++d)
	{
		if (this->versionsCounts[d] != versionsCount)
		{
			display_message(WARNING_MESSAGE, "FE_node_field_template::convertLegacyDOFIndexToValueLabelAndVersion.  "
				"Must have same number of versions for all derivatives/values labels");
			return false;
		}
	}
	if (dofIndex >= (this->valueLabelsCount*versionsCount))
	{
		display_message(WARNING_MESSAGE, "FE_node_field_template::convertLegacyDOFIndexToValueLabelAndVersion.  "
			"DOF index %d is out of range of number of derivative/value labels %d time versions %d",
			dofIndex, this->valueLabelsCount, versionsCount);
		return false;
	}
	// use old parameter ordering which nested derivative/value labels within versions:
	valueLabel = this->valueLabels[dofIndex % this->valueLabelsCount];
	version = dofIndex / this->valueLabelsCount;
	return true;
}
