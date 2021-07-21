/**
 * FILE : datastore/mapindexing.cpp
 * 
 * Implements a multidimensional indexing object for addressing data in a
 * datastore map.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "datastore/map.hpp"
#include "datastore/mapindexing.hpp"

DsMapIndexing::DsMapIndexing(DsMapBase& mapBaseIn,
	int labelsArraySizeIn, const DsLabels **labelsArrayIn) :
		mapBase(cmzn::Access(&mapBaseIn)), // GRC circular reference?
		labelsArraySize(labelsArraySizeIn),
		indexing(new Indexing[labelsArraySize])
{
	for (int i = 0; i < labelsArraySize; i++)
		indexing[i].setLabels(labelsArrayIn[i]);
}

DsMapIndexing::~DsMapIndexing()
{
	delete[] indexing;
	cmzn::Deaccess(this->mapBase);
}

DsMapIndexing *DsMapIndexing::create(DsMapBase& mapBaseIn,
	int labelsArraySizeIn, const DsLabels **labelsArrayIn)
{
	if ((labelsArraySizeIn < 0) ||
		((labelsArraySizeIn > 0) && (0 == labelsArrayIn)))
		return 0;
	for (int i = 0; i < labelsArraySizeIn; i++)
	{
		if (0 == labelsArrayIn[i])
			return 0;
	}
	return new DsMapIndexing(mapBaseIn, labelsArraySizeIn, labelsArrayIn);
}
