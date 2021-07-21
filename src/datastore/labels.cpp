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

#include "opencmiss/zinc/status.h"
#include "datastore/labels.hpp"
#include "general/message.h"

DsLabels::DsLabels() :
	cmzn::RefCounted(),
	contiguous(true),
	firstFreeIdentifier(1),
	firstIdentifier(DS_LABEL_IDENTIFIER_INVALID),
	lastIdentifier(DS_LABEL_IDENTIFIER_INVALID),
	labelsCount(0),
	indexSize(0),
	activeIterators(0)
{
};

DsLabels::~DsLabels()
{
	// can't free externally held objects, hence just invalidate for safety
	this->invalidateLabelIterators();
}

/** restore to initial empty, contiguous state. Keeps current name, if any */
void DsLabels::clear()
{
	// can't free externally held objects, hence just invalidate for safety
	this->invalidateLabelIterators();
	this->contiguous = true;
	this->firstFreeIdentifier = 1;
	this->firstIdentifier = DS_LABEL_IDENTIFIER_INVALID;
	this->lastIdentifier = DS_LABEL_IDENTIFIER_INVALID;
	this->identifiers.clear();
	this->identifierToIndexMap.clear();
	this->labelsCount = 0;
	this->indexSize = 0;
}

/**
 * Get the next unused identifier of at least startIdentifier,
 * or 1 if startIdentifier is not positive.
 */
DsLabelIdentifier DsLabels::getFirstFreeIdentifier(DsLabelIdentifier startIdentifier)
{
	DsLabelIdentifier identifier =
		(startIdentifier < this->firstFreeIdentifier) ? this->firstFreeIdentifier : startIdentifier;
	if (!this->contiguous)
	{
		// this can be optimised using an iterator:
		while (DS_LABEL_INDEX_INVALID != findLabelByIdentifier(identifier))
			++identifier;
		if (startIdentifier <= this->firstFreeIdentifier)
			this->firstFreeIdentifier = identifier;
	}
	return identifier;
}

int DsLabels::setNotContiguous()
{
	if (this->contiguous)
	{
		// this can be optimised:
		DsLabelIdentifier identifier = this->firstIdentifier;
		for (DsLabelIndex index = 0; index < this->indexSize; ++index)
		{
			if (!(this->identifiers.setValue(index, identifier) &&
					this->identifierToIndexMap.insert(*this, index)))
			{
				display_message(ERROR_MESSAGE, "DsLabels::setNotContiguous.  Failed");
				return CMZN_ERROR_MEMORY;
			}
			++identifier;
		}
		this->contiguous = false;
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

/** private: caller must have checked identifier is not in use! */
DsLabelIndex DsLabels::createLabelPrivate(DsLabelIdentifier identifier)
{
	if (identifier < 0)
		return DS_LABEL_INDEX_INVALID;
	this->invalidateLabelIterators();
	DsLabelIndex index = DS_LABEL_INDEX_INVALID;
	if (this->contiguous)
	{
		if (0 == this->labelsCount)
		{
			this->firstIdentifier = identifier;
			this->lastIdentifier = identifier - 1; // to trigger if statement below
		}
		if (identifier == (this->lastIdentifier + 1))
		{
			index = this->indexSize;
			this->lastIdentifier = identifier;
			if (identifier == this->firstFreeIdentifier)
				++this->firstFreeIdentifier;
			++this->labelsCount;
			++this->indexSize;
			return index;
		}
		if (CMZN_OK != this->setNotContiguous())
			return DS_LABEL_INDEX_INVALID;
	}
	// Future memory optimisation: reclaim unused indexes for new labels.
	// Note for reclaim to work would have to clear indexes into maps
	// as labels are deleted [for maps indexed by these labels].
	if (this->identifiers.setValue(this->indexSize, identifier))
	{
		index = this->indexSize;
		// must increase these now to allow identifiers to be queried by map
		++this->labelsCount;
		++this->indexSize;
		if (!this->identifierToIndexMap.insert(*this, index))
		{
			display_message(ERROR_MESSAGE, "DsLabels::createLabelPrivate. Failed to insert index into map");
			--this->labelsCount;
			--this->indexSize;
			return DS_LABEL_INDEX_INVALID;
		}
		if (identifier == this->firstFreeIdentifier)
			++this->firstFreeIdentifier;
	}
	else
		display_message(ERROR_MESSAGE, "DsLabels::createLabelPrivate. Failed to set identifier");
	return index;
}

/** create with auto-generated unique identifier */
DsLabelIndex DsLabels::createLabel()
{
	return createLabelPrivate(this->getFirstFreeIdentifier());
}

/** fails if identifier already in use */
DsLabelIndex DsLabels::createLabel(DsLabelIdentifier identifier)
{
	DsLabelIndex index = findLabelByIdentifier(identifier);
	if (index >= 0)
		return DS_LABEL_INDEX_INVALID;
	return createLabelPrivate(identifier);
}

/** finds existing entry with identifier, or creates new one if none */
DsLabelIndex DsLabels::findOrCreateLabel(DsLabelIdentifier identifier)
{
	DsLabelIndex index = findLabelByIdentifier(identifier);
	if (index >= 0)
		return index;
	return createLabelPrivate(identifier);
}

int DsLabels::addLabelsRange(DsLabelIdentifier min, DsLabelIdentifier max,
	DsLabelIdentifier stride)
{
	if ((max < min) || (stride < 1))
		return CMZN_ERROR_ARGUMENT;
	if (this->contiguous && (stride == 1) && (0 == this->labelsCount))
	{
		// fast case
		this->firstIdentifier = min;
		this->lastIdentifier = max;
		this->firstFreeIdentifier = max + 1;
		this->labelsCount = max - min + 1;
		this->indexSize = max - min + 1;
	}
	else
	{
		for (DsLabelIdentifier identifier = min; identifier <= max; identifier += stride)
		{
			DsLabelIndex index = this->findOrCreateLabel(identifier);
			if (index < 0)
				return CMZN_ERROR_GENERAL;
		}
	}
	return CMZN_OK;
}

int DsLabels::removeLabel(DsLabelIndex index)
{
	if ((index < 0) || (index >= this->indexSize))
		return 0;
	if (this->contiguous)
	{
		const int result = setNotContiguous();
		if (result != CMZN_OK)
			return 0;
	}
	this->invalidateLabelIterators();
	DsLabelIdentifier identifier;
	if (this->identifiers.getValue(index, identifier))
	{
		if (identifier >= 0)
		{
			this->identifierToIndexMap.erase(*this, index);
			this->identifiers.setValue(index, DS_LABEL_IDENTIFIER_INVALID);
			if (identifier < this->firstFreeIdentifier)
				this->firstFreeIdentifier = identifier;
			--this->labelsCount;
			if (0 == this->labelsCount)
				this->clear();
			return 1;
		}
	}
	return 0;
}

int DsLabels::removeLabelWithIdentifier(DsLabelIdentifier identifier)
{
	DsLabelIndex index = findLabelByIdentifier(identifier);
	if (index >= 0)
		return removeLabel(index);
	return 0;
}

/**
 * Safely changes the identifier at index to identifier, by removing index from
 * the identifier-to-index map and any other related maps, then changing the
 * identifier and re-inserting it where it was removed.
 * @return  CMZN_OK on success, any other error code if failed.
 */
int DsLabels::setIdentifier(DsLabelIndex index, DsLabelIdentifier identifier)
{
	if ((index < 0) || (index >= this->indexSize))
		return CMZN_ERROR_ARGUMENT;
	DsLabelIdentifier oldIdentifier = this->getIdentifier(index);
	if (oldIdentifier < 0)
		return CMZN_ERROR_ARGUMENT;
	DsLabelIndex existingIndex = this->findLabelByIdentifier(identifier);
	if (existingIndex != DS_LABEL_INDEX_INVALID)
	{
		if (existingIndex == index)
			return CMZN_OK;
		return CMZN_ERROR_ALREADY_EXISTS;
	}
	if (this->contiguous)
	{
		int result = this->setNotContiguous();
		if (CMZN_OK != result)
			return result;
	}
	this->invalidateLabelIterators();
	int return_code = CMZN_OK;
	if (this->identifierToIndexMap.begin_identifier_change(*this, index))
	{
		if (this->identifiers.setValue(index, identifier))
		{
			if (oldIdentifier < this->firstFreeIdentifier)
				this->firstFreeIdentifier = oldIdentifier;
		}
		else
			return_code = CMZN_ERROR_GENERAL;
		this->identifierToIndexMap.end_identifier_change(*this);
	}
	else
		return_code = CMZN_ERROR_GENERAL;
	return return_code;
}

DsLabelIndex DsLabels::getFirstIndex() const
{
	if (0 == this->labelsCount)
		return DS_LABEL_INDEX_INVALID;
	if (this->contiguous)
		return 0;
	return this->identifierToIndexMap.get_first_object();
}

DsLabelIterator *DsLabels::createLabelIterator(bool_array<DsLabelIndex> *condition) const
{
	DsLabelIterator *iterator = new DsLabelIterator();
	if (iterator)
	{
		iterator->labels = this;
		iterator->iter = (this->contiguous) ? 0 : new DsLabelIdentifierToIndexMap::ext_iterator(&this->identifierToIndexMap);
		iterator->condition = condition;
		iterator->index = DS_LABEL_INDEX_INVALID;
		iterator->next = this->activeIterators;
		iterator->previous = 0;
		if (this->activeIterators)
			this->activeIterators->previous = iterator;
		this->activeIterators = iterator;
	}
	return iterator;
}

void DsLabels::removeLabelIterator(DsLabelIterator *iterator) const
{
	if (iterator)
	{
		if (iterator->previous)
			iterator->previous->next = iterator->next;
		else
			this->activeIterators = iterator->next;
		if (iterator->next)
			iterator->next->previous = iterator->previous;
		// Following not necessary since only called from ~DsLabelIterator:
		//iterator->invalidate();
	}
}

void DsLabels::invalidateLabelIterators()
{
	DsLabelIterator *iterator = this->activeIterators;
	DsLabelIterator *nextIterator;
	while (iterator)
	{
		nextIterator = iterator->next;
		iterator->invalidate();
		iterator = nextIterator;
	}
	this->activeIterators = 0;
}

void DsLabels::invalidateLabelIteratorsWithCondition(bool_array<DsLabelIndex> *condition)
{
	DsLabelIterator *iterator = this->activeIterators;
	while (iterator)
	{
		if (iterator->condition == condition)
		{
			DsLabelIterator *nextIterator = iterator->next;
			if (iterator->previous)
				iterator->previous->next = iterator->next;
			else
				this->activeIterators = iterator->next;
			if (iterator->next)
				iterator->next->previous = iterator->previous;
			iterator->invalidate();
			iterator = nextIterator;
		}
		else
			iterator = iterator->next;
	}
}

int DsLabels::getIdentifierRanges(DsLabelIdentifierRanges& ranges) const
{
	ranges.clear();
	DsLabelIterator *iterator = this->createLabelIterator();
	if (!iterator)
		return CMZN_ERROR_MEMORY;
	if (iterator->nextIndex() != DS_LABEL_INDEX_INVALID)
	{
		DsLabelIdentifier identifier = iterator->getIdentifier();
		DsLabelIdentifierRange range = { identifier, identifier };
		while (iterator->nextIndex() != DS_LABEL_INDEX_INVALID)
		{
			identifier = iterator->getIdentifier();
			if (identifier != (range.last + 1))
			{
				ranges.push_back(range);
				range.first = identifier;
			}
			range.last = identifier;
		}
		ranges.push_back(range);
	}
	cmzn::Deaccess(iterator);
	return CMZN_OK;
}

DsLabelIterator::DsLabelIterator() :
	cmzn::RefCounted(),
	labels(0),
	iter(0),
	condition(0),
	index(DS_LABEL_INDEX_INVALID),
	next(0),
	previous(0)
{
}

DsLabelIterator::~DsLabelIterator()
{
	if (this->labels)
		this->labels->removeLabelIterator(this);
	delete this->iter;
}

void DsLabelIterator::invalidate()
{
	if (this->labels)
	{
		delete this->iter;
		this->iter = 0;
		this->labels = 0;
		this->condition = 0;
		this->index = DS_LABEL_INDEX_INVALID;
		this->previous = 0;
		this->next = 0;
	}
}

void DsLabelIterator::setIndex(DsLabelIndex newIndex)
{
	if (this->labels)
	{
		if (this->iter)
		{
			if (!this->iter->set_object(*labels, newIndex))
				display_message(ERROR_MESSAGE, "DsLabelIterator::setIndex  Failed");
		}
		this->index = newIndex;
	}
	else
		display_message(ERROR_MESSAGE, "DsLabelIterator::setIndex  Iterator has been invalidated");
}

void DsLabels::list_storage_details() const
{
	if (this->contiguous)
	{
		display_message(INFORMATION_MESSAGE, "  Size = %d\n", this->labelsCount);
		display_message(INFORMATION_MESSAGE, "  Contiguous");
		display_message(INFORMATION_MESSAGE, "  First identifier = %d", this->firstIdentifier);
		display_message(INFORMATION_MESSAGE, "  Last identifier = %d", this->lastIdentifier);
	}
	else
	{
		int stem_count = 0;
		int leaf_count = 0;
		int min_leaf_depth = 0;
		int max_leaf_depth = 0;
		double mean_leaf_depth = 0.0;
		double mean_stem_occupancy = 0.0;
		double mean_leaf_occupancy = 0.0;
		this->identifierToIndexMap.get_statistics(stem_count, leaf_count,
			min_leaf_depth, max_leaf_depth, mean_leaf_depth,
			mean_stem_occupancy, mean_leaf_occupancy);
		display_message(INFORMATION_MESSAGE, "  Size = %d\n", this->identifierToIndexMap.size());
		display_message(INFORMATION_MESSAGE, "  Stem count = %d\n", stem_count);
		display_message(INFORMATION_MESSAGE, "  Leaf count = %d\n", leaf_count);
		display_message(INFORMATION_MESSAGE, "  Min leaf depth = %d\n", min_leaf_depth);
		display_message(INFORMATION_MESSAGE, "  Max leaf depth = %d\n", max_leaf_depth);
		display_message(INFORMATION_MESSAGE, "  Mean leaf depth = %g\n", mean_leaf_depth);
		display_message(INFORMATION_MESSAGE, "  Mean stem occupancy = %g\n", mean_stem_occupancy);
		display_message(INFORMATION_MESSAGE, "  Mean leaf occupancy = %g\n", mean_leaf_occupancy);
	}
}
