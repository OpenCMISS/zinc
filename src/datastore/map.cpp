/**
 * FILE : datastore/map.hpp
 * 
 * Implements a semi-dense multidimensional array addressed by sets
 * of labels.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "datastore/map.hpp"

DsMapBase::DsMapBase(int labelsArraySizeIn, const DsLabels **labelsArrayIn) :
	dense(true),
	labelsArraySize(labelsArraySizeIn),
	labelsArray(new const DsLabels*[labelsArraySizeIn])
{
	for (int i = 0; i < labelsArraySize; i++)
		this->labelsArray[i] = cmzn::Access(labelsArrayIn[i]);
}

DsMapBase::~DsMapBase()
{
	for (int i = 0; i < labelsArraySize; i++)
		cmzn::Deaccess(this->labelsArray[i]);
	delete[] this->labelsArray;
}

bool DsMapBase::checkLabelsArrays(int labelsArraySizeIn, const DsLabels **labelsArrayIn)
{
	if ((labelsArraySizeIn < 0) || ((0 < labelsArraySizeIn) && !(labelsArrayIn)))
		return false;
	// check all labels sets are present and none are repeated
	for (int i = 0; i < labelsArraySizeIn; i++)
	{
		if (0 == labelsArrayIn[i])
		{
			display_message(ERROR_MESSAGE, "DsMapBase::create.  Missing labels[%d]", 1);
			return false;
		}
		for (int j = i + 1; j < labelsArraySizeIn; j++)
		{
			if (labelsArrayIn[j] == labelsArrayIn[i])
			{
				display_message(ERROR_MESSAGE,
					"DsMapBase::create.  Repeated labels '%s'", labelsArrayIn[i]->getName().c_str());
				return false;
			}
		}
	}
	return true;
}

DsMapIndexing *DsMapBase::createIndexing()
{
	return DsMapIndexing::create(*this, this->labelsArraySize, this->labelsArray);
}
