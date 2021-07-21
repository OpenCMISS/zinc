/**
 * FILE : datastore/labels.hpp
 * 
 * Implements a set of labels identifying nodes, elements, field components.
 * Used to index a dimension of a datastore map.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (CMZN_DATASTORE_LABELS_HPP)
#define CMZN_DATASTORE_LABELS_HPP

#include <string>
#include <vector>
#include "general/block_array.hpp"
#include "general/cmiss_btree_index.hpp"
#include "general/message.h"
#include "general/refcounted.hpp"
#include "general/refhandle.hpp"

/**
 * The type of the unique identifier i.e. user number for each label in DsLabels
 * Currently restricted to be non-negative integer.
 */
typedef int DsLabelIdentifier;

struct DsLabelIdentifierRange
{
	DsLabelIdentifier first, last;
};

typedef std::vector<DsLabelIdentifierRange> DsLabelIdentifierRanges;

const DsLabelIdentifier DS_LABEL_IDENTIFIER_INVALID = -1;

/**
 * The array index of a label in the DsLabels, starting at 0.
 * Special value DS_LABEL_INDEX_INVALID means invalid.
 */
typedef int DsLabelIndex;

// following is used to mark an invalid index i.e. not set
// Comment any code assuming this value with "Assumes DS_LABEL_INDEX_INVALID == -1"
const DsLabelIndex DS_LABEL_INDEX_INVALID = -1;

// following is used to mark an array of indexes as invalid
const DsLabelIndex DS_LABEL_INDEX_UNALLOCATED = -2;

class DsLabels;

typedef block_array<DsLabelIndex, DsLabelIdentifier> DsLabelIdentifierArray;
typedef cmzn_btree_index<DsLabels, DsLabelIndex, DsLabelIdentifier, DS_LABEL_INDEX_INVALID> DsLabelIdentifierToIndexMap;

class DsLabelIterator;

/**
 * A set of entries with unique identifiers, used to label nodes, elements,
 * field components etc. for indexing into a datastore map.
 */
class DsLabels : public cmzn::RefCounted
{
	std::string name; // optional
	bool contiguous; // true while all entries from firstIdentifier..lastIdentifier exist and are in order
	DsLabelIdentifier firstFreeIdentifier; // exact only if contiguous, otherwise only a minimum
	DsLabelIdentifier firstIdentifier; // used if contiguous: identifier of first index
	DsLabelIdentifier lastIdentifier; // used if contiguous: identifier of last valid index
	DsLabelIdentifierArray identifiers; // used only if not contiguous
	DsLabelIdentifierToIndexMap identifierToIndexMap; // used only if not contiguous
	int labelsCount; // number of valid labels
	int indexSize; // allocated label array size; can have holes where labels removed

	// linked-lists of active iterators, to invalidate when labels set changes
	// including eventually when defragmenting memory
	mutable DsLabelIterator *activeIterators;

public:

	DsLabels();
	DsLabels(const DsLabels&); // not implemented
	~DsLabels();
	DsLabels& operator=(const DsLabels&); // not implemented

	void clear();

private:

	int setNotContiguous();

	DsLabelIndex createLabelPrivate(DsLabelIdentifier identifier);

public:

	bool isContiguous() const
	{
		return this->contiguous;
	}

	std::string getName() const
	{
		return this->name;
	}

	void setName(const std::string& nameIn)
	{
		this->name = nameIn;
	}

	DsLabelIndex getSize() const
	{
		return labelsCount;
	}

	/** @return  true if any indexes are unused, false if all are valid */
	bool hasUnusedIndexes() const
	{
		return (labelsCount < indexSize);
	}

	/** @return  maximum index which has ever existed; entry may be void */
	DsLabelIndex getIndexSize() const
	{
		return indexSize;
	}

	DsLabelIdentifier getFirstFreeIdentifier(DsLabelIdentifier startIdentifier = 0);
	
	/** @return CMZN_OK on success, any other error code on failure */
	int addLabelsRange(DsLabelIdentifier min, DsLabelIdentifier max,
		DsLabelIdentifier stride = 1);

	DsLabelIndex createLabel();

	DsLabelIndex createLabel(DsLabelIdentifier identifier);

	DsLabelIndex findOrCreateLabel(DsLabelIdentifier identifier);

	DsLabelIndex findLabelByIdentifier(DsLabelIdentifier identifier) const;

	int removeLabel(DsLabelIndex index);

	int removeLabelWithIdentifier(DsLabelIdentifier identifier);

	DsLabelIdentifier getIdentifier(DsLabelIndex index) const
	{
		DsLabelIdentifier identifier = DS_LABEL_IDENTIFIER_INVALID;
		if ((0 <= index) && (index < this->indexSize))
		{
			if (this->contiguous)
				identifier = this->firstIdentifier + static_cast<DsLabelIdentifier>(index);
			else
				this->identifiers.getValue(index, identifier);
		}
		return identifier;
	}

	int setIdentifier(DsLabelIndex index, DsLabelIdentifier identifier);

	/** @return  True if index exists in label set, otherwise false. */
	bool hasIndex(DsLabelIndex index) const
	{
		if ((0 <= index) && (index < this->indexSize))
		{
			if (this->contiguous)
				return true;
			DsLabelIdentifier identifier;
			if (this->identifiers.getValue(index, identifier) && (identifier != DS_LABEL_IDENTIFIER_INVALID))
				return true;
		}
		return false;
	}

	/**
	 * Get first label index in set or DS_LABEL_INDEX_INVALID if none.
	 * Currently returns index with the lowest identifier in set
	 */
	DsLabelIndex getFirstIndex() const;

	/**
	 * Create new iterator initially pointing before first label.
	 * @param  condition  Boolean array which must be true for given index to include.
	 * @return accessed iterator, or 0 if failed.
	 */
	DsLabelIterator *createLabelIterator(bool_array<DsLabelIndex> *condition = 0) const;

	void removeLabelIterator(DsLabelIterator *iterator) const; // only used by ~DsLabelIterator;

	void invalidateLabelIterators();

	void invalidateLabelIteratorsWithCondition(bool_array<DsLabelIndex> *condition); // used from DsLabelsGroup

	int getIdentifierRanges(DsLabelIdentifierRanges& ranges) const;

	void list_storage_details() const;
};

typedef cmzn::RefHandle<const DsLabels> HCDsLabels;
typedef cmzn::RefHandle<DsLabels> HDsLabels;

/**
 * An external iterator/handle to a single Label in the DsLabels.
 * Maintains pointer to owning labels for type safety.
 */
class DsLabelIterator : public cmzn::RefCounted
{
	friend class DsLabels;
	
private:
	const DsLabels *labels;
	DsLabelIdentifierToIndexMap::ext_iterator *iter; // set and used only if non-contiguous iteration
	bool_array<DsLabelIndex> *condition; // set and used if iterating over DsLabelsGroup
	DsLabelIndex index;
	DsLabelIterator *next, *previous; // for linked-list in owning DsLabels

	DsLabelIterator();
	DsLabelIterator(const DsLabelIterator&); // not implemented
	virtual ~DsLabelIterator();
	DsLabelIterator& operator=(const DsLabelIterator&); // not implemented

	void invalidate();
public:

	// caller must check valid index, use DS_LABEL_INDEX_INVALID to reset to before start
	void setIndex(DsLabelIndex newIndex);

	const DsLabels *getLabels() const { return this->labels; }

	DsLabelIndex getIndex() const { return this->index; }

	DsLabelIdentifier getIdentifier() const
	{
		return this->labels->getIdentifier(this->index);
	}

	// return next index or DS_LABEL_INDEX_INVALID if finished
	inline DsLabelIndex nextIndex()
	{
		if (!this->labels)
		{
			display_message(ERROR_MESSAGE, "DsLabelIterator::nextIndex  Iterator has been invalidated");
			return DS_LABEL_INDEX_INVALID;
		}
		if (this->iter) // non-contiguous identifier order:
		{
			this->index = this->iter->next();
			if (this->condition)
			{
				// quite expensive for small sub-groups
				while ((this->index != DS_LABEL_INDEX_INVALID) && (!this->condition->getBool(this->index)))
					this->index = this->iter->next();
			}
		}
		else
		{
			if (this->index < (this->labels->getIndexSize() - 1))
				++this->index;
			else
				this->index = DS_LABEL_INDEX_INVALID;
			if ((this->condition) && (this->index != DS_LABEL_INDEX_INVALID))
				if (!this->condition->advanceIndexWhileFalse(this->index, this->labels->getIndexSize()))
					this->index = DS_LABEL_INDEX_INVALID;
		}
		return this->index;
	}

	// return true if valid index, false if reached end
	inline bool increment()
	{
		return this->nextIndex() != DS_LABEL_INDEX_INVALID;
	}

};

typedef cmzn::RefHandle<DsLabelIterator> HDsLabelIterator;

inline DsLabelIndex DsLabels::findLabelByIdentifier(DsLabelIdentifier identifier) const
{
	if (this->contiguous)
	{
		if ((identifier >= this->firstIdentifier) && (identifier <= this->lastIdentifier))
			return static_cast<DsLabelIndex>(identifier - this->firstIdentifier);
	}
	else
	{
		return this->identifierToIndexMap.find_object_by_identifier(*this, identifier);
	}
	return DS_LABEL_INDEX_INVALID;
}

#endif /* !defined (CMZN_DATASTORE_LABELS_HPP) */
