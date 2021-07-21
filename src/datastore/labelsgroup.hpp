/**
 * FILE : datastore/labelsgroup.hpp
 * 
 * Implements a subset of a datastore labels set.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (CMZN_DATASTORE_LABELSGROUP_HPP)
#define CMZN_DATASTORE_LABELSGROUP_HPP

#include "datastore/labels.hpp"

/**
 * A subset of a datastore labels set.
 * Implemented using a bool_array
 */
class DsLabelsGroup : public cmzn::RefCounted
{
protected:
	DsLabels *labels;
	// Note: ensure all members are transferred by swap() method
	int labelsCount;
	// indexLimit is at least one greater than highest index in group, updated to exact index when queried
	int indexLimit;
	bool_array<DsLabelIndex> values;

	DsLabelsGroup(DsLabels *labelsIn);
	DsLabelsGroup(const DsLabelsGroup&); // not implemented
	~DsLabelsGroup();
	DsLabelsGroup& operator=(const DsLabelsGroup&); // not implemented

public:
	static DsLabelsGroup *create(DsLabels *labelsIn);

	/** Swaps all data with other block_array. Cannot fail. */
	void swap(DsLabelsGroup& other);

	DsLabels *getLabels()
	{
		return this->labels;
	}
	
	void clear();

	DsLabelIndex getSize() const
	{
		return labelsCount;
	}

	DsLabelIndex getIndexLimit()
	{
		if (indexLimit > 0)
		{
			DsLabelIndex index = indexLimit - 1;
			if (values.updateLastTrueIndex(index))
				indexLimit = index + 1;
		}
		return indexLimit;
	}

	/** @return true if group contains all entries from 1..indexLimit */
	bool isDense()
	{
		getIndexLimit();
		return (labelsCount == indexLimit);
	}

	/** @return true if group contains all entries from belowIndex+1..indexLimit */
	bool isDenseAbove(DsLabelIndex belowIndex)
	{
		getIndexLimit();
		// GRC: this can be expensive:
		return values.isRangeTrue(/*minIndex*/belowIndex + 1, /*minIndex*/this->indexLimit-1);
	}
	
	/** caller must be careful that index is for this labels */
	bool hasIndex(DsLabelIndex index) const
	{
		if (index < 0)
			return false;
		return values.getBool(/*index*/index);
	}

	/**
	 * Ensure index is in the group.
	 * Be careful that index is for this labels.
	 * @return  CMZN_OK on success, CMZN_ALREADY_EXISTS if adding when already added,
	 * CMZN_ERROR_NOT_FOUND if removing when already removed, any other error code on failure.
	 */
	int setIndex(DsLabelIndex index, bool inGroup);

	DsLabelIndex getFirstIndex(DsLabelIterator &iterator);

	/**
	 * Create new iterator initially pointing before first label.
	 * @return accessed iterator, or 0 if failed.
	 */
	DsLabelIterator *createLabelIterator();

	/**
	 * Increment index to next index in group.
	 * Assumes index set to DS_LABEL_INDEX_INVALID (-1) before start.
	 * @return true if index advanced to valid index in group, false if iteration over
	 */
	bool incrementIndex(DsLabelIndex& index)
	{
		++index;
		return this->values.advanceIndexWhileFalse(index, this->labels->getIndexSize());
	}

	void invalidateLabelIterators()
	{
		this->labels->invalidateLabelIteratorsWithCondition(&(this->values));
	}

};

#endif /* !defined (CMZN_DATASTORE_LABELSGROUP_HPP) */
