/**
 * FILE : datastore/labelchangelog.hpp
 *
 * Cache of changes to labels for client notification.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (CMZN_DATASTORE_LABELSCHANGELOG_HPP)
#define CMZN_DATASTORE_LABELSCHANGELOG_HPP

#include "datastore/labelsgroup.hpp"

enum DsLabelChangeType
/**
 * Emumeration describing what has change about a particular object. Values are
 * attributed so that bitwise logical operations can be used to treat different
 * states in the same way.
 */
{
	DS_LABEL_CHANGE_TYPE_NONE = 0,       /*!< no change */
	DS_LABEL_CHANGE_TYPE_ADD = 1,        /*!< label/object added */
	DS_LABEL_CHANGE_TYPE_REMOVE = 2,     /*!< label/object removed */
	DS_LABEL_CHANGE_TYPE_IDENTIFIER = 4, /*!< label/object identifier changed */
	DS_LABEL_CHANGE_TYPE_DEFINITION = 8, /*!< definition of object with label changed */
	DS_LABEL_CHANGE_TYPE_RELATED = 16    /*!< related data for object with label change (e.g. field change) */
};

/**
 * A subset of a datastore labels set.
 * Implemented using a bool_array
 */
class DsLabelsChangeLog : private DsLabelsGroup
{
	int maxChanges; // if negative, no upper limit on number of changes before automatic allChange
	int changeSummary; // logical OR of values from enum DsLabelChangeType
	bool allChange;

	DsLabelsChangeLog(DsLabels *labelsIn, int maxChangesIn);
	DsLabelsChangeLog(const DsLabelsChangeLog&); // not implemented
	~DsLabelsChangeLog();
	DsLabelsChangeLog& operator=(const DsLabelsChangeLog&); // not implemented

public:
	static DsLabelsChangeLog *create(DsLabels *labelsIn, int maxChangesIn = -1);

	inline void access()
	{
		DsLabelsGroup::access();
	}

	inline void deaccess()
	{
		DsLabelsGroup::deaccess();
	}

	DsLabels *getLabels()
	{
		return DsLabelsGroup::getLabels();
	}

	const DsLabelsGroup *getLabelsGroup() const
	{
		return this;
	}

	DsLabelsGroup *getLabelsGroup()
	{
		return this;
	}

	int getChangeSummary() const
	{
		return this->changeSummary;
	}

	/**
	 * Do not call if isAllChange() is true. 
	 * @return  Number of listed changed elements or 0 if allChange set.
	 */
	int getChangeCount() const
	{
		return DsLabelsGroup::getSize();
	}

	/** @param change  A value / logical or of values from enum DsLabelChangeType */
	void setChange(int change)
	{
		this->changeSummary |= change;
	}

	/**
	 * Set index as having the change.
	 * @param index  A valid index for this labels object.
	 * @param change  A value / logical OR of values from enum DsLabelChangeType
	 */
	void setIndexChange(DsLabelIndex index, int change);

	/**
	 * @return  True if index is flagged as having a change, or all change flag set.
	 * @param index  A valid index for this labels object.
	 */
	bool isIndexChange(DsLabelIndex index)
	{
		if (this->allChange)
			return true;
		return DsLabelsGroup::hasIndex(index);
	}

	/**
	 * Mark all labels as having the supplied change. Avoids needing to
	 * store which labels have changed.
	 * @param change  A value / logical OR of values from enum DsLabelChangeType
	 */
	void setAllChange(int change);

	bool isAllChange() const
	{
		return this->allChange;
	}
};

#endif /* !defined (CMZN_DATASTORE_LABELSCHANGELOG_HPP) */
