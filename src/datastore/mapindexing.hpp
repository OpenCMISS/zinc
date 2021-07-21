/**
 * FILE : datastore/mapindexing.hpp
 * 
 * Implements a multidimensional indexing object for addressing data in a
 * datastore map.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (CMZN_DATASTORE_MAPINDEXING_HPP)
#define CMZN_DATASTORE_MAPINDEXING_HPP

#include "datastore/labels.hpp"
#include "datastore/labelsgroup.hpp"

// Declare integer type large enough to address map
// For 64-bit builds and large datasets we will eventually want an 8-byte integer e.g.:
// typedef unsigned long long int DsMapAddressType;
typedef unsigned int DsMapAddressType;

class DsMapBase;

/**
 * Index to subset of a DsMap via array of DsLabels, used to get and set values.
 * For each DsLabels it may represent:
 * - unspecified = all entries in DsLabels in order of increasing identifier.
 * - a single entry, specified using a DsLabelIterator
 * - a group of entries in order of increasing identifier, specified using a
 *   DsLabelsGroup.
 * - (Future: may need to permit a sequence of entries in specified order)
 * Where there are groups specified for multiple DsLabels, all permutations of
 * all entries in the groups are indexed in order of first DsLabels changing
 * slowest to last DsLabels changing fastest, like C arrays.
 * GRC may review order
 * Note:
 * Not to be shared between threads as internal mutable members are modified
 * by DsMap<>::setValues();
 */
struct DsMapIndexing : public cmzn::RefCounted
{
	friend class DsMapBase;

	class Indexing
	{

	public:
		const DsLabels *labels;
		// only zero or one of following set; zero means use all labels
		DsLabelIterator *iterator;
		DsLabelsGroup *labelsGroup;
		// following mutable members are used by DsMap<>::setValues
		mutable DsLabelIndex indexLimit;
		mutable bool firstIndexValid;
		mutable DsLabelIterator *valuesIterator;

	private:

		void clearLabelIterator()
		{
			cmzn::Deaccess(this->iterator);
		}

		void clearLabelsGroup()
		{
			cmzn::Deaccess(this->labelsGroup);
		}

	public:

		Indexing() :
			labels(0),
			iterator(0),
			labelsGroup(0),
			indexLimit(0),
			firstIndexValid(false),
			valuesIterator(0)
		{
		}

		/* second stage of construction */
		void setLabels(const DsLabels *labelsIn)
		{
			this->labels = labelsIn;
		}

		~Indexing()
		{
			cmzn::Deaccess(this->valuesIterator);
			clearLabelIterator();
			clearLabelsGroup();
		}

		unsigned int getEntryCount() const
		{
			if (this->iterator)
				return (this->iterator->getIndex() >= 0) ? 1 : 0;
			if (this->labelsGroup)
				return this->labelsGroup->getSize();
			return this->labels->getSize();
		}

		/** Return identifier if singly indexed */
		DsLabelIdentifier getSparseIdentifier() const
		{
			if (this->iterator)
				return this->iterator->getIdentifier();
			return DS_LABEL_IDENTIFIER_INVALID;
		}

		/** Return index if singly indexed */
		DsLabelIndex getSparseIndex() const
		{
			if (this->iterator)
				return this->iterator->getIndex();
			return DS_LABEL_INDEX_INVALID;
		}

		void setAll()
		{
			clearLabelIterator();
			clearLabelsGroup();
		}

		void setEntry(DsLabelIterator& iteratorIn)
		{
			this->setIndex(iteratorIn.getIndex());
		}

		void setIndex(DsLabelIndex indexIn)
		{
			if (!this->iterator)
				this->iterator = this->labels->createLabelIterator();
			this->iterator->setIndex(indexIn);
			clearLabelsGroup();
		}

		void setGroup(DsLabelsGroup *labelsGroupIn)
		{
			cmzn::Reaccess(this->labelsGroup, labelsGroupIn);
			clearLabelIterator();
		}

		void calculateIndexLimit()
		{
			if (iterator)
				indexLimit = iterator->getIndex() + 1;
			else if (labelsGroup)
				indexLimit = labelsGroup->getIndexLimit();
			else
				indexLimit = this->labels->getIndexSize(); // GRC this is conservative
		}

		/** @return true if indexing is dense from 0..indexLimit-1 */
		bool isDense()
		{
			if (this->labels->hasUnusedIndexes())
				return false;
			if (iterator)
				return (iterator->getIndex() == 0);
			if (labelsGroup)
				return labelsGroup->isDense();
			return true;
		}

		bool isDenseAbove(DsLabelIndex belowIndex)
		{
			if (this->labels->hasUnusedIndexes())
				return false;
			if (iterator)
				return (iterator->getIndex() == (belowIndex + 1));
			if (labelsGroup)
				return labelsGroup->isDenseAbove(belowIndex);
			return true;
		}

		/**
		 * @param innerIndexing unless true, advance to first value
		 */
		bool iterationBegin(bool innerIndexing)
		{
			if (this->iterator)
			{
				// flag ensures inner indexing returns at least one valid value
				this->firstIndexValid = innerIndexing;
				// no valuesIterator when single entry is indexed
				this->valuesIterator = 0;
				return true;
			}
			// must prove there is at least one value in indexing	
			if (this->labelsGroup)
				this->valuesIterator = this->labelsGroup->createLabelIterator();
			else
				this->valuesIterator = this->labels->createLabelIterator();
			if ((this->valuesIterator) && this->valuesIterator->increment())			
			{
				if (innerIndexing)
					this->valuesIterator->setIndex(DS_LABEL_INDEX_INVALID); // so next increment is to first
				return true;
			}
			return false;
		}

		DsLabelIndex getIterationIndex()
		{
			return (this->iterator ? this->iterator->getIndex() : this->valuesIterator->getIndex());
		}

		/**
		 * @return  true if more to come, false if last
		 * Automatically sets to recycle.
		 */
		bool iterationNext()
		{
			if (iterator)
			{
				if (this->firstIndexValid)
				{
					this->firstIndexValid = false;
					return true;
				}
				return this->firstIndexValid;
			}
			if (this->valuesIterator->increment())
				return true;
			// set before start and iterate to first valid object
			valuesIterator->setIndex(DS_LABEL_INDEX_INVALID);
			this->valuesIterator->increment();
			return false;
		}

		void iterationEnd()
		{
			if (valuesIterator)
				cmzn::Deaccess(this->valuesIterator);
		}

	};

private:
	DsMapBase *mapBase;
	int labelsArraySize;
	Indexing *indexing;

	DsMapIndexing(DsMapBase& mapBaseIn,
		int labelsArraySizeIn, const DsLabels **labelsArrayIn);
	
	static DsMapIndexing *create(DsMapBase& mapBaseIn,
		int labelsArraySizeIn, const DsLabels **labelsArrayIn);

	/** get internal Indexing object for supplied DsLabels */
	Indexing *getIndexingForLabels(const DsLabels *labelsIn)
	{
		for (int i = 0; i < labelsArraySize; i++)
		{
			if (labelsIn == indexing[i].labels)
				return &(indexing[i]);
		}
		return 0;
	}

public:

	virtual ~DsMapIndexing();

	/** @return  true if the map of this index matches that passed, otherwise false */
	bool indexesMap(const DsMapBase *mapBaseIn) const
	{
		return (mapBaseIn == this->mapBase);
	}

	int hasLabelsArray(int labelsArraySizeIn, const DsLabels **labelsArrayIn)
	{
		if (labelsArraySizeIn != labelsArraySize)
			return 0;
		for (int i = 0; i < labelsArraySize; i++)
		{
			if ((!labelsArrayIn[i]) || (labelsArrayIn[i] != indexing[i].labels))
				return 0;
		}
		return 1;
	}

	/** gets the number of permutations of labels this index covers */
	DsMapAddressType getEntryCount() const
	{
		DsMapAddressType count = 1;
		for (int i = 0; i < labelsArraySize; i++)
		{
			count *= indexing[i].getEntryCount();
		}
		return count;
	}

	int setAllLabels(const DsLabels &labels)
	{
		Indexing *indexing = getIndexingForLabels(&labels);
		if (!indexing)
			return 0;
		indexing->setAll();
		return 1;
	}

	int setEntry(DsLabelIterator& iterator)
	{
		Indexing *indexing = getIndexingForLabels(iterator.getLabels());
		if (!indexing)
			return 0;
		indexing->setEntry(iterator);
		return 1;
	}

	int setEntryIndex(const DsLabels& labels, DsLabelIndex indexIn)
	{
		Indexing *indexing = getIndexingForLabels(&labels);
		if (!indexing)
			return 0;
		indexing->setIndex(indexIn);
		return 1;
	}

	int setEntryIdentifier(const DsLabels& labels, DsLabelIdentifier identifierIn)
	{
		Indexing *indexing = getIndexingForLabels(&labels);
		if (!indexing)
			return 0;
		DsLabelIndex index = labels.findLabelByIdentifier(identifierIn);
		if (index == DS_LABEL_INDEX_INVALID)
			return 0;
		indexing->setIndex(index);
		return 1;
	}

	int setGroup(DsLabelsGroup *labelsGroup)
	{
		Indexing *indexing = getIndexingForLabels(labelsGroup->getLabels());
		if (!indexing)
			return 0;
		indexing->setGroup(labelsGroup);
		return 1;
	}

	/**
	 * Precalculates indexLimit values for each labels.
	 * Note: must call getEntryCount() first to confirm no invalid indexes!
	 */
	void calculateIndexLimits()
	{
		for (int i = 0; i < labelsArraySize; i++)
		{
			indexing[i].calculateIndexLimit();
		}
	}

	int getLabelsArraySize() const
	{
		return this->labelsArraySize;
	}

	/**
	 * @param labelsNumber  Index from 0 to labelsArraySize-1
	 * @return  Accessed pointer to DsLabels or 0 if invalid number or error
	 */
	const DsLabels* getLabels(int labelsNumber) const
	{
		if ((0 <= labelsNumber) && (labelsNumber < this->labelsArraySize))
			return cmzn::Access(this->indexing[labelsNumber].labels);
		return 0;
	}

	/** Get the current label identifier for the labelsNumber, if it is sparsely
	 * indexed by an iterator.
	 * @return  The label identifier, or DS_LABEL_IDENTIFIER_INVALID if not
	 * indexed by an iterator. */
	DsLabelIdentifier getSparseIdentifier(int labelsNumber) const
	{
		return this->indexing[labelsNumber].getSparseIdentifier();
	}

	/** Get the current label index for the labelsNumber, if it is sparsely
	 * indexed by an iterator.
	 * @return  The label index, or DS_LABEL_INDEX_INVALID if not
	 * indexed by an iterator. */
	DsLabelIndex getSparseIndex(int labelsNumber) const
	{
		return this->indexing[labelsNumber].getSparseIndex();
	}

	/**
	 * Returns limiting entry index for the indexing of the selected labels.
	 * Must have called calculateIndexLimits first
	 * @param labelsNumber  Index from 0 to labelsArraySize-1
	 */
	DsLabelIndex getIndexLimit(int labelsNumber)
	{
		if ((0 <= labelsNumber) && (labelsNumber < this->labelsArraySize))
			return indexing[labelsNumber].indexLimit;
		return 0;
	}

	/**
	 * Returns true if all indexes are specified from 0..indexLimit-1
	 * for the selected labels 
	 * @param labelsNumber  Index from 0 to labelsArraySize-1
	 */
	bool isDenseOnLabels(int labelsNumber)
	{
		if ((0 <= labelsNumber) && (labelsNumber < this->labelsArraySize))
			return indexing[labelsNumber].isDense();
		return false;
	}

	/**
	 * Returns true if all indexes are specified from belowIndex+1..indexLimit-1
	 * for the selected labels 
	 * @param labelsNumber  Index from 0 to labelsArraySize-1
	 * @param belowIndex  Index one below dense range being checked.
	 */
	bool isDenseOnLabelsAbove(int labelsNumber,
		DsLabelIndex belowIndex)
	{
		if ((0 <= labelsNumber) && (labelsNumber < this->labelsArraySize))
			return indexing[labelsNumber].isDenseAbove(belowIndex);
		return false;
	}

	/** obtain iterators ready to point to first indexes in each labels indexing
	 * at next call to iterationNext().
	 * Callers must invoke iterationEnd() after successful call!
	 * @return  true on success, false on failure with temporaries cleaned up.
	 */
	bool iterationBegin()
	{
		bool innerIndexing = true;
		for (int i = labelsArraySize - 1; i >= 0; --i)
		{
			// only inner indexing points before first, so first increment is valid
			if (!indexing[i].iterationBegin(innerIndexing))
			{
				iterationEnd();
				return false;
			}
			innerIndexing = false;
		}
		return true;
	}

	DsLabelIndex getIterationIndex(int labelsNumber)
	{
		return indexing[labelsNumber].getIterationIndex();
	}

	/** @return  true if more to come, false if past last */
	bool iterationNext()
	{
		for (int i = labelsArraySize - 1; i >= 0; --i)
		{
			if (indexing[i].iterationNext())
				return true;
		}
		return false;
	}

	void iterationEnd()
	{
		for (int i = 0; i < labelsArraySize; i++)
		{
			indexing[i].iterationEnd();
		}			
	}

	/** Call before iterating with incrementSparseIterators
	 * Optional labelNumber is used to partially reset
	 */
	void resetSparseIterators(int labelsNumber = 0)
	{
		int i = this->labelsArraySize - 1;
		for (; labelsNumber <= i; --i)
		{
			if (this->indexing[i].iterator)
			{
				this->indexing[i].iterator->setIndex(DS_LABEL_INDEX_INVALID);
				--i;
				break;
			}
		}
		for (; labelsNumber <= i; --i)
		{
			if (this->indexing[i].iterator)
				this->indexing[i].iterator->setIndex(indexing[i].labels->getFirstIndex());
		}
	}

	/** @return  true if more to come, false if past last */
	bool incrementSparseIterators()
	{
		for (int i = labelsArraySize - 1; 0 <= i; --i)
		{
			if (this->indexing[i].iterator)
			{
				if (this->indexing[i].iterator->increment())
					return true;
				// reset to start
				if (!this->indexing[i].iterator->increment())
					return false; // only happens if no labels
			}
		}
		return false;
	}

	// call when a sparse iterator goes beyond index size for contiguous labels.
	// saves iterating over all permutations of inner indices to get next values.
	// @return true if more iteration to do, false if iteration finished
	bool advanceSparseIterator(int labelsNumber)
	{
		resetSparseIterators(labelsNumber);
		for (int i = labelsNumber - 1; 0 <= i; --i)
		{
			if (this->indexing[i].iterator)
			{
				if (this->indexing[i].iterator->increment())
					return true;
				// reset to start
				if (!this->indexing[i].iterator->increment())
					return false; // only happens if no labels
			}
		}
		return false;
	}

};

typedef cmzn::RefHandle<DsMapIndexing> HDsMapIndexing;

#endif /* !defined (CMZN_DATASTORE_MAPINDEXING_HPP) */
