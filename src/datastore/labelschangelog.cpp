/**
 * FILE : datastore/labelchangelog.cpp
 *
 * Cache of changes to labels for client notification.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "opencmiss/zinc/status.h"
#include "datastore/labelschangelog.hpp"

DsLabelsChangeLog::DsLabelsChangeLog(DsLabels *labelsIn, int maxChangesIn) :
	DsLabelsGroup(labelsIn),
	maxChanges(maxChangesIn),
	changeSummary(DS_LABEL_CHANGE_TYPE_NONE),
	allChange(false)
{
};

DsLabelsChangeLog::~DsLabelsChangeLog()
{
}

// The following is dodgy as it neither accesses the labels (and mesh passes
// in address of internal member), nor stores in owner to invalidate. However.
// I know the labels are not used by any callers, so leaving for now.
DsLabelsChangeLog *DsLabelsChangeLog::create(DsLabels *labelsIn, int maxChangesIn)
{
	return new DsLabelsChangeLog(labelsIn, maxChangesIn);
}

void DsLabelsChangeLog::setIndexChange(DsLabelIndex index, int change)
{
	this->changeSummary |= change;
	if (!this->allChange)
	{
		const int result = DsLabelsGroup::setIndex(index, true);
		if (result != CMZN_ERROR_ALREADY_EXISTS)
		{
			if ((result != CMZN_OK) || ((this->maxChanges >= 0) && (this->getSize() > this->maxChanges)))
				this->setAllChange(this->changeSummary);
		}
	}
}

void DsLabelsChangeLog::setAllChange(int change)
{
	this->allChange = true;
	this->changeSummary |= change;
	DsLabelsGroup::clear();
}
