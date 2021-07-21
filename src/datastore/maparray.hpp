/**
 * FILE : datastore/maparray.hpp
 * 
 * Template implementing a map from a single datastore labels set to fixed size
 * array of given type.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (CMZN_DATASTORE_MAPARRAY_HPP)
#define CMZN_DATASTORE_MAPARRAY_HPP

#include "datastore/labels.hpp"
#include "general/block_array.hpp"

// IndexType is normally DsLabelIndex, but must be raised to 64-bit integer
// once arraySize*number-of-labels exceeds 2^31.
template <typename IndexType, typename ValueType>
class DsMapArray
{
	DsLabels *labels;
	const IndexType arraySize;
	const IndexType arraysPerBlock;
	block_array<IndexType, ValueType> values;
	// First value in array is initialised to this->unallocatedValue if not allocated,
	// any other value if it is in use. Hence this cannot be a valid value for the
	// supplied ValueType.
	ValueType unallocatedValue;
	// value to initialise all other array members to.
	ValueType initValue;

	DsMapArray(const DsMapArray&); // not implemented
	DsMapArray& operator=(const DsMapArray&); // not implemented

public:

	DsMapArray(DsLabels *labelsIn, DsLabelIndex arraySizeIn, ValueType unallocatedValueIn = 0, ValueType initValueIn = 0) :
		labels(Access(labelsIn)),
		// array blocks must be multiple of arraySize
		// use exact size if large enough to prevent wastage when sparse
		arraySize(static_cast<IndexType>(arraySizeIn)),
		arraysPerBlock(arraySizeIn*sizeof(ValueType) >= 128 ? 1 :
			(CMZN_BLOCK_ARRAY_DEFAULT_BLOCK_SIZE_BYTES / (arraySizeIn*sizeof(ValueType)))),
		values(arraysPerBlock*arraySizeIn, initValueIn),
		unallocatedValue(unallocatedValueIn),
		initValue(initValueIn)
	{
	}

	virtual ~DsMapArray()
	{
		Deaccess(this->labels);
	}

	/** Clear all data held for all indexes */
	void clear()
	{
		this->values.clear();
	}

	/**
	 * Clear array held for index, if any. Does not free memory.
	 */
	void clearArray(DsLabelIndex index)
	{
		const IndexType arrayIndex = index*this->arraySize;
		ValueType *array = this->values.getAddress(arrayIndex);
		if (array && (*array != this->unallocatedValue))
		{
			*array = this->unallocatedValue;
			for (IndexType i = 1; i < this->arraySize; ++i)
				array[i] = this->initValue;
		}
	}

	/** remove array at index, freeing block if all arrays in it destroyed
	  * @param index  Not checked */
	void destroyArray(DsLabelIndex index)
	{
		const IndexType arrayIndex = index*this->arraySize;
		ValueType *array = this->values.getAddress(arrayIndex);
		if (array)
		{
			*array = this->unallocatedValue;
			const IndexType blockIndex = index/this->arraysPerBlock;
			// block must exist as just found array in it
			ValueType *block = this->values.getBlock(blockIndex);
			for (int i = 0; i < this->arraysPerBlock; ++i)
			{
				if (block[i*this->arraySize] != this->unallocatedValue)
					return;
			}
			this->values.destroyBlock(blockIndex);
		}
	}

	/**
	 * @return  Address of array for the given labels index, or 0 if unallocated.
	 */
	ValueType *getArray(DsLabelIndex index) const
	{
		const IndexType arrayIndex = index*this->arraySize;
		ValueType *array = this->values.getAddress(arrayIndex);
		if (array && (*array != this->unallocatedValue))
			return array;
		return 0;
	}

	/**
	 * @return address of existing or new array for the given labels index, or 0 if failed.
	 */
	ValueType *getOrCreateArray(DsLabelIndex index)
	{
		const IndexType arrayIndex = index*this->arraySize;
		ValueType *array = this->values.getOrCreateAddressArrayInit(arrayIndex, this->unallocatedValue, this->arraySize);
		if (array && (*array == this->unallocatedValue))
		{
			// first value is initially this->unallocatedValue: change so marked as allocated
			// all other values already equal this->initValue
			*array = this->initValue;
		}
		return array;
	}

	/* get the highest index data is held for, minimum from allocated blocks or labels size */
	DsLabelIndex getIndexLimit() const
	{
		DsLabelIndex indexLimit = this->values.getBlockCount()*this->arraysPerBlock;
		if (indexLimit > this->labels->getIndexSize())
			indexLimit = this->labels->getIndexSize();
		return indexLimit;
	}

};

typedef DsMapArray<DsLabelIndex, DsLabelIndex> DsMapArrayLabelIndex;

#endif // CMZN_DATASTORE_MAPARRAY_HPP
