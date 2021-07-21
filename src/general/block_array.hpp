/**
 * FILE : block_array.hpp
 * 
 * Implements an array container allocated in blocks.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (BLOCK_ARRAY_HPP)
#define BLOCK_ARRAY_HPP

#include "general/debug.h"
#include <cstring>

// DsMapArray assumes following is > 128:
#define CMZN_BLOCK_ARRAY_DEFAULT_BLOCK_SIZE_BYTES 1024

// IndexType = array index type
// EntryType = type of values held in array
template <typename IndexType, typename EntryType> class block_array
{
protected:
	// Note: any new attributes must be handled by swap() and all constructors
	EntryType **blocks;
	IndexType blockCount;
	IndexType blockLength;
	EntryType allocInitValue;

	EntryType* getOrCreateBlock(IndexType blockIndex)
	{
		if (blockIndex >= this->blockCount)
		{
			IndexType newBlockCount = blockIndex + 1;
			if (newBlockCount < this->blockCount*2)
				newBlockCount = this->blockCount*2; // double number of blocks each time at a minimum
			EntryType **newBlocks = new EntryType*[newBlockCount];
			if (!newBlocks)
				return 0;
			memcpy(newBlocks, this->blocks, this->blockCount*sizeof(EntryType*));
			for (IndexType i = blockCount; i < newBlockCount; ++i)
				newBlocks[i] = 0;
			delete[] this->blocks;
			this->blocks = newBlocks;
			this->blockCount = newBlockCount;
		}
		EntryType *block = this->blocks[blockIndex];
		if (!block)
		{
			block = new EntryType[this->blockLength];
			if (block)
			{
				for (IndexType i = 0; i < this->blockLength; ++i)
					block[i] = this->allocInitValue;
				this->blocks[blockIndex] = block;
			}
		}
		return block;
	}

	template<typename TYPE> static void swap_value(TYPE& v1, TYPE& v2)
	{
		TYPE tmp = v1;
		v1 = v2;
		v2 = tmp;
	}

public:
	
	/**
	 * @param allocInitValue = value each array entry is set to on initialisation.
	 * Default 0 only OK for numeric and pointer types.
	 */
	block_array(IndexType blockLengthIn = CMZN_BLOCK_ARRAY_DEFAULT_BLOCK_SIZE_BYTES/sizeof(EntryType), EntryType allocInitValueIn = 0) :
		blocks(0),
		blockCount(0),
		blockLength(blockLengthIn),
		allocInitValue(allocInitValueIn)
	{
		if (this->blockLength <= 0)
			this->blockLength = 1;
	}

	block_array(const block_array& source) :
		blocks(new EntryType*[source.blockCount]),
		blockCount(source.blockCount),
		blockLength(source.blockLength),
		allocInitValue(source.allocInitValue)
	{
		for (int i = 0; i < this->blockCount; ++i)
		{
			if (source.blocks[i])
			{
				this->blocks[i] = new EntryType[this->blockLength];
				memcpy(this->blocks[i], source.blocks[i], this->blockLength*sizeof(EntryType));
			}
			else
				this->blocks[i] = 0;
		}
	}

	virtual ~block_array()
	{
		this->clear();
	}

	virtual void clear()
	{
		for (IndexType i = 0; i < this->blockCount; ++i)
			delete[] this->blocks[i];
		delete[] this->blocks;
		this->blocks = 0;
		this->blockCount = 0;
	}

	/** @param  blockIndex  From 0 to block count - 1. Not checked. */
	EntryType* getBlock(IndexType blockIndex)
	{
		return this->blocks[blockIndex];
	}

	/** @param  blockIndex  From 0 to block count - 1. Not checked. */
	void destroyBlock(IndexType blockIndex)
	{
		delete[] this->blocks[blockIndex];
		this->blocks[blockIndex] = 0;
	}

	IndexType getBlockCount() const
	{
		return this->blockCount;
	}

	IndexType getBlockLength() const
	{
		return this->blockLength;
	}

	/** @return  Allocated block index capacity, for limiting iteration. Returned
	  * value is one greater than last allocated index. Note that entries may be
	  * allocated but not in use. */
	IndexType getIndexLimit() const
	{
		for (IndexType blockIndex = this->blockCount - 1; 0 <= blockIndex; --blockIndex)
		{
			if (this->blocks[blockIndex])
				return (blockIndex + 1)*this->blockLength;
		}
		return 0;
	}

	/** @return  First allocated index or zero if no blocks, for lower limit of index iteration.
	  * Note that entries may be allocated but not in use. */
	IndexType getIndexStart() const
	{
		for (IndexType blockIndex = 0; blockIndex < this->blockCount; ++blockIndex)
		{
			if (this->blocks[blockIndex])
				return blockIndex*this->blockLength;
		}
		return 0; // fall back to first
	}

	/** Swaps all data with other block_array. Cannot fail. */
	void swap(block_array& other)
	{
		swap_value(this->blocks, other.blocks);
		swap_value(this->blockCount, other.blockCount);
		swap_value(this->blockLength, other.blockLength);
		swap_value(this->allocInitValue, other.allocInitValue);
	}

	/**
	 * @param index  The index of the address to retrieve, starting at 0.
	 * @return  The address for value for given index, or 0 if none.
	 */
	EntryType *getAddress(IndexType index) const
	{
		IndexType blockIndex = index / blockLength;
		if (blockIndex < blockCount)
		{
			EntryType *block = blocks[blockIndex];
			if (block)
				return block + (index % blockLength);
		}
		return 0;
	}

	/**
	 * Gets or creates block containing values at index and returns address of index.
	 * Uses default initialisation of values in new blocks.
	 * @param index  The index of the address to create.
	 * @return  The address for value for given index, or 0 if failed.
	 */
	EntryType *getOrCreateAddress(IndexType index)
	{
		IndexType blockIndex = index / blockLength;
		EntryType *block = 0;
		if (blockIndex < blockCount)
			block = blocks[blockIndex];
		if (!block)
		{
			block = getOrCreateBlock(blockIndex);
			if (!block)
				return 0;
		}
		return block + (index % blockLength);
	}

	/**
	 * Gets or creates block containing values at index and returns address of index.
	 * If block is newly created it is initialised as described below.
	 * @param index  The index of the address to create.
	 * @param initValue  Value to initialise entries in block to.
	 * @param initIndexSpacing  Spacing of values to be initialised >= 1,
	 * e.g. 10 initialises:
	 * 0 10 20 ... blockLength (truncated to nearest multiple of 10)
	 * @return  The address for value for given index, or 0 if failed.
	 */
	EntryType *getOrCreateAddressArrayInit(IndexType index, EntryType& initValue, IndexType initIndexSpacing)
	{
		if (initIndexSpacing < 1)
			return 0;
		IndexType blockIndex = index / blockLength;
		EntryType *block = 0;
		if (blockIndex < blockCount)
			block = blocks[blockIndex];
		if (!block)
		{
			block = getOrCreateBlock(blockIndex);
			if (!block)
				return 0;
			if (initIndexSpacing > 0)
			{
				for (IndexType i = 0; i < blockLength; i += initIndexSpacing)
					block[i] = initValue;
			}
		}
		return block + (index % blockLength);
	}

	/**
	 * Get a value from the block_array.
	 * @param index  The index of the value to retrieve, starting at 0.
	 * @param value  On success, filled with value held at index.
	 * @return  Boolean true if value returned, false if no value at index.
	 */
	bool getValue(IndexType index, EntryType& value) const
	{
		IndexType blockIndex = index / blockLength;
		if (blockIndex < blockCount)
		{
			EntryType *block = blocks[blockIndex];
			if (block)
			{
				IndexType entryIndex = index % blockLength;
				value = block[entryIndex];
				return true;
			}
		}
		return false;
	}

	/**
	 * Get a value from the block_array. Variant suitable only for arrays
	 * where allocInitValue is not a valid value.
	 * @param index  The index of the value to retrieve, starting at 0.
	 * @return  The value at the index, or allocInitValue if none.
	 */
	EntryType getValue(IndexType index) const
	{
		IndexType blockIndex = index / blockLength;
		if (blockIndex < blockCount)
		{
			EntryType *block = blocks[blockIndex];
			if (block)
			{
				IndexType entryIndex = index % blockLength;
				return block[entryIndex];
			}
		}
		// do not warn about invalid index as used to query whether set
		return this->allocInitValue;
	}

	/**
	 * Check matching value held in block_array.
	 * @param index  The index of the value to check, starting at 0. Must be non-negative.
	 * @param value  Value to check at index.
	 * @return  Boolean true if array has value at index.
	 */
	bool hasValue(IndexType index, EntryType value) const
	{
		IndexType blockIndex = index / blockLength;
		if (blockIndex < blockCount)
		{
			EntryType *block = blocks[blockIndex];
			if (block)
				return (block[index % blockLength] == value);
		}
		return false;
	}

	/**
	 * Set a value in the block_array.
	 * @param index  The index of the value to set, starting at 0.
	 * @param value  Value to set at index.
	 * @return  Boolean true on success, false on failure.
	 */
	bool setValue(IndexType index, EntryType value)
	{
		const IndexType blockIndex = index / blockLength;
		EntryType* block = getOrCreateBlock(blockIndex);
		if (!block)
			return false;
		IndexType entryIndex = index % blockLength;
		block[entryIndex] = value;
		return true;
	}

	/**
	 * Set consecutive values from start index up to supplied value.
	 * @param startIndex  The lowest index to set value at >= 0.
	 * @param count  The number of consecutive values to set.
	 * @param value  The value to assign.
	 * @return  Boolean true on success, false on failure.
	 */
	bool setValues(IndexType startIndex, IndexType count, EntryType value)
	{
		const IndexType stopIndex = startIndex + count - 1;
		const IndexType startBlockIndex = startIndex / blockLength;
		const IndexType stopBlockIndex = stopIndex / blockLength;
		for (IndexType blockIndex = startBlockIndex; blockIndex <= stopBlockIndex; ++blockIndex)
		{
			EntryType* block = getOrCreateBlock(blockIndex);
			if (!block)
				return false;
			const IndexType startEntryIndex = (blockIndex == startBlockIndex) ? (startIndex % blockLength) : 0;
			const IndexType stopEntryIndex = (blockIndex == stopBlockIndex) ? (stopIndex % blockLength) : (blockLength - 1);
			for (IndexType entryIndex = startEntryIndex; entryIndex <= stopEntryIndex; ++entryIndex)
				block[entryIndex] = value;
		}
		return true;
	}
	
};

/** Variant of block_array for storing pointers to allocated array values.
  * Calls array delete[] on non-NULL entries on clear and in destructor */
template <typename IndexType, typename EntryType> class dynarray_block_array :
	public block_array<IndexType, EntryType>
{
public:
	dynarray_block_array(IndexType blockLengthIn = CMZN_BLOCK_ARRAY_DEFAULT_BLOCK_SIZE_BYTES/sizeof(EntryType)) :
		block_array<IndexType, EntryType>(blockLengthIn, 0) // pointers must be initialised to 0
	{
	}

private:
	dynarray_block_array(const dynarray_block_array& source); // not implemented

public:
	virtual void clear()
	{
		for (IndexType blockIndex = 0; blockIndex < this->blockCount; ++blockIndex)
		{
			EntryType* block = this->blocks[blockIndex];
			if (block)
			{
				for (IndexType i = 0; i < this->blockLength; ++i)
					delete[] block[i];
			}
		}
		this->block_array<IndexType, EntryType>::clear();
	}
};

/** stores boolean values as individual bits, with no value equivalent to false */
template <typename IndexType>
	class bool_array : private block_array<IndexType, unsigned int>
{
public:
	// default size fits same number of entries as 32-bit index
	bool_array(IndexType intBlockLength = CMZN_BLOCK_ARRAY_DEFAULT_BLOCK_SIZE_BYTES/32) :
		block_array<IndexType, unsigned int>(intBlockLength, /*allocInitValueIn*/0)
	{
	}

	void clear()
	{
		block_array<IndexType, unsigned int>::clear();
	}

	void swap(bool_array& other)
	{
		block_array<IndexType, unsigned int>::swap(other);
	}

	using block_array<IndexType, unsigned int>::getBlockCount;
	using block_array<IndexType, unsigned int>::getBlockLength;
	using block_array<IndexType, unsigned int>::getValue;
	using block_array<IndexType, unsigned int>::setValue;
	using block_array<IndexType, unsigned int>::setValues;

	/** @param oldValue  Returns old value so client can determine if status changed */
	bool setBool(IndexType index, bool value, bool& oldValue)
	{
		IndexType intIndex = index >> 5;
		unsigned int intValue = 0;
		bool hasValue = getValue(intIndex, intValue);
		if (hasValue || value)
		{
			unsigned int mask = (1 << (index & 0x1F));
			oldValue = (0 != (intValue & mask));
			if (oldValue != value)
			{
				return setValue(intIndex, intValue ^ mask);
			}
		}
		else
		{
			oldValue = false;
		}
		return true;
	}

	bool getBool(IndexType index) const
	{
		IndexType intIndex = index >> 5;
		unsigned int intValue;
		if (getValue(intIndex, intValue))
		{
			unsigned int mask = (1 << (index & 0x1F));
			return (0 != (intValue & mask));
		}
		return false;
	}

	/**
	 * Advance index while bool array value is false.
	 * Efficiently skips whole blocks, and 32-bit zeroes within blocks.
	 * @param index  The index to advance while bool value is false.
	 * @param limit  One past the last index to check.
	 * @return  True if index found, false if reached limit.
	 */
	bool advanceIndexWhileFalse(IndexType& index, IndexType limit)
	{
		const IndexType blockIndexSize = this->getBlockLength()*32;
		const IndexType blockLimit = this->getBlockCount()*blockIndexSize;
		const IndexType useLimit = (limit < blockLimit) ? limit : blockLimit;
		while (index < useLimit)
		{
			const IndexType intIndex = index >> 5;
			unsigned int intValue;
			if (!getValue(intIndex, intValue))
				index = ((index / blockIndexSize) + 1)*blockIndexSize; // advance to next block
			else
			{
				unsigned int mask = (1 << (index & 0x1F));
				if (intValue < mask)
					index = (intIndex + 1) << 5; // advance to next int index
				else
				{
					do
					{
						if (intValue & mask)
							return true;
						++index;
						mask <<= 1;
					}
					while (index & 0x1F);
				}
			}
		}
		return false;
	}

	/**
	 * @param lastTrueIndex  Updated to equal or next lower index with true value.  
	 * @return  true if found, false if none.
	 */
	bool updateLastTrueIndex(IndexType& lastTrueIndex)
	{
		// GRC this can be made much more efficient
		while (!getBool(lastTrueIndex))
		{
			--lastTrueIndex;
			if (lastTrueIndex < 0)
				return false;
		}
		return true;
	}

	/** @return  true if values for all indexes in range are true; false otherwise */
	bool isRangeTrue(IndexType minIndex, IndexType maxIndex)
	{
		if (minIndex > maxIndex)
			return false;
		// GRC this can be made much more efficient
		IndexType index = minIndex;
		while (getBool(index))
		{
			if (index == maxIndex)
				return true;
			index++;
		}
		return false;
	}

	/** Sets all entries from index 0..indexCount-1 to true.
	 * @return  true if completely successful, false otherwise */
	bool setAllTrue(IndexType indexCount)
	{
		IndexType intIndexCount = indexCount >> 5;
		// bulk set the flags in lots of 32 bits
		if (intIndexCount > 0)
		{
			if (!setValues(0, intIndexCount, 0xFFFFFFFF))
				return false;
		}
		// individually set remaining bits
		for (IndexType index = intIndexCount*32; index < indexCount; index++)
		{
			bool oldValue;
			if (!setBool(index, true, oldValue))
				return false;
		}
		return true;
	}

	/**
	 * @return  true if all bits in bool array are either all on or all off over
	 * all consecutive subarrays of the given size, otherwise false. Used to
	 * determine sparsity of map indexing.
	 */
	bool isBanded(IndexType bandSize, IndexType numberOfBands)
	{
		for (IndexType band = 0; band < numberOfBands; ++band)
		{
			// GRC this can be made much more efficient
			IndexType index = band*bandSize;
			bool firstBool = getBool(index);
			IndexType indexLimit = (band + 1)*bandSize;
			for (++index; index < indexLimit; ++index)
				if (getBool(index) != firstBool)
					return false;
		}
		return true;
	}

};

#endif /* !defined (BLOCK_ARRAY_HPP) */
