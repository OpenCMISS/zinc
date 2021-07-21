/***************************************************************************//**
 * FILE : field_cache.hpp
 * 
 * Implements a cache for prescribed domain locations and field evaluations.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (FIELD_CACHE_HPP)
#define FIELD_CACHE_HPP

#include "opencmiss/zinc/element.h"
#include "opencmiss/zinc/fieldcache.h"
#include "opencmiss/zinc/fieldmodule.h"
#include "opencmiss/zinc/region.h"
#include "opencmiss/zinc/status.h"
#include "general/debug.h"
#include "region/cmiss_region.hpp"
#include "computed_field/field_location.hpp"
#include <vector>

struct Computed_field_find_element_xi_cache;

// dynamic_cast may make cache value type crashes more predictable.
// Enable for spurious errors, but switching off for performance reasons, release and debug.
#if defined (TEST_FIELD_VALUE_CACHE_CAST)
#  if (defined (OPTIMISED)) || (defined (NDEBUG)) // macros not consistent across platforms
#    define FIELD_VALUE_CACHE_CAST static_cast
#  else
#    define FIELD_VALUE_CACHE_CAST dynamic_cast
#  endif
#else // defined (TEST_FIELD_VALUE_CACHE_CAST)
#  define FIELD_VALUE_CACHE_CAST static_cast
#endif // defined (TEST_FIELD_VALUE_CACHE_CAST)

class FieldValueCache
{
private:
	cmzn_fieldcache *extraCache; // optional extra cache for working evaluations at different locations

	void operator=(const FieldValueCache&); // private to prohibit

public:
	int evaluationCounter; // set to cmzn_fieldcache::locationCounter when field evaluated

	FieldValueCache() :
		extraCache(0),
		evaluationCounter(-1)
	{
	}

	virtual ~FieldValueCache();

	/** override if needed to clear type-specific evaluation counters & call this */
	virtual void resetEvaluationCounter()
	{
		this->evaluationCounter = -1;
	}

	/** override to clear type-specific buffer information & call this */
	virtual void clear();

	cmzn_fieldcache *getOrCreatePrivateExtraCache(cmzn_region *region);

	cmzn_fieldcache *getOrCreateSharedExtraCache(cmzn_fieldcache& parentCache);

	/** can use after calling getOrCreatePrivateExtraCache or getOrCreateSharedExtraCache*/
	cmzn_fieldcache *getExtraCache()
	{
		return this->extraCache;
	}

	/** all derived classed must implement function to return values as string
	 * @return  allocated string.
	 */
	virtual char *getAsString() const = 0;

};

/** Storage of derivative values */
class DerivativeValueCache
{
public:
	FE_value *values;
	const int componentCount;
	int evaluationCounter;  // set to cmzn_fieldcache::locationCounter when evaluated
	int termCount;  // number of derivative terms
	int termCountAllocated;

private:
	DerivativeValueCache(); // not implemented
	DerivativeValueCache(const DerivativeValueCache &source); // not implemented
	DerivativeValueCache& operator=(const DerivativeValueCache &source); // not implemented

public:

	DerivativeValueCache(int componentCountIn, int termCountIn) :
		values(nullptr),
		componentCount(componentCountIn),
		evaluationCounter(-1),
		termCount(0),
		termCountAllocated(0)
	{
		this->setTermCount(termCountIn);
	}

	~DerivativeValueCache()
	{
		delete[] values;
	}

	void copyValues(const DerivativeValueCache& source)
	{
		const int valueCount = this->componentCount*this->termCount;
		for (int i = 0; i < valueCount; ++i)
			this->values[i] = source.values[i];
	}

	int getComponentCount() const
	{
		return this->componentCount;
	}

	int getTermCount() const
	{
		return this->termCount;
	}

	int getValueCount() const
	{
		return this->componentCount*this->termCount;
	}

	void resetEvaluationCounter()
	{
		this->evaluationCounter = -1;
	}

	/** For variable number of derivative terms */
	void setTermCount(int termCountIn)
	{
		if (termCountIn > this->termCountAllocated)
		{
			FE_value *newValues = new FE_value[this->componentCount*termCountIn];
			delete[] this->values;
			this->values = newValues;
			this->termCountAllocated = termCountIn;
		}
		this->termCount = termCountIn;
	}

	void zeroValues()
	{
		const int valueCount = this->componentCount*this->termCount;
		for (int i = 0; i < valueCount; ++i)
			this->values[i] = 0.0;
	}

};

class RealFieldValueCache : public FieldValueCache
{
public:
	FE_value *values;
	const int componentCount;
	std::vector<DerivativeValueCache *> derivatives;
	Computed_field_find_element_xi_cache *find_element_xi_cache;

	RealFieldValueCache(int componentCountIn) :
		FieldValueCache(),
		values(new FE_value[componentCountIn]),
		componentCount(componentCountIn),
		find_element_xi_cache(0)
	{
	}

	virtual ~RealFieldValueCache();

	virtual void resetEvaluationCounter();

	virtual void clear();

	static const RealFieldValueCache* cast(const FieldValueCache* valueCache)
	{
		return FIELD_VALUE_CACHE_CAST<const RealFieldValueCache*>(valueCache);
	}

	static RealFieldValueCache* cast(FieldValueCache* valueCache)
	{
		return FIELD_VALUE_CACHE_CAST<RealFieldValueCache*>(valueCache);
	}

	static const RealFieldValueCache& cast(const FieldValueCache& valueCache)
	{
		return FIELD_VALUE_CACHE_CAST<const RealFieldValueCache&>(valueCache);
	}

	static RealFieldValueCache& cast(FieldValueCache& valueCache)
	{
		return FIELD_VALUE_CACHE_CAST<RealFieldValueCache&>(valueCache);
	}

	void copyValues(const RealFieldValueCache& source)
	{
		for (int i = 0; i < this->componentCount; ++i)
			values[i] = source.values[i];
	}

	/** copy values and all derivatives existing on both source and this value cache */
	void copyValuesAndDerivatives(const RealFieldValueCache& source)
	{
		this->copyValues(source);
		const size_t commonSize = this->derivatives.size() < source.derivatives.size() ? this->derivatives.size() : source.derivatives.size();
		for (size_t i = 0; i < commonSize; ++i)
			if ((this->derivatives[i]) && (source.derivatives[i]))
				this->derivatives[i]->copyValues(*source.derivatives[i]);
	}

	virtual char *getAsString() const;

	void setValues(const FE_value *values_in)
	{
		for (int i = 0; i < this->componentCount; ++i)
			values[i] = values_in[i];
	}

	/** Call only if known that getOrCreateDerivativeValueCache has been called */
	DerivativeValueCache *getDerivativeValueCache(const FieldDerivative& fieldDerivative) const
	{
		return this->derivatives[fieldDerivative.getCacheIndex()];
	}

	/**
	 * @param location  Location to evaluate derivative at; some derivatives have a variable number of terms depending on it.
	 */
	DerivativeValueCache *getOrCreateDerivativeValueCache(const FieldDerivative& fieldDerivative, const Field_location& location)
	{
		const int cacheIndex = fieldDerivative.getCacheIndex();
		if (this->derivatives.size() <= cacheIndex)
			this->derivatives.resize(cacheIndex + 1, nullptr);
		const int termCount = fieldDerivative.getTermCount(location);
		DerivativeValueCache *derivativeValueCache = this->derivatives[cacheIndex];
		if (derivativeValueCache)
			derivativeValueCache->setTermCount(termCount);
		else
			derivativeValueCache = this->derivatives[cacheIndex] = new DerivativeValueCache(this->componentCount, termCount);
		return derivativeValueCache;
	}

	/** remove any derivative value cache for index */
	void removeDerivativeValueCacheAtIndex(int derivativeCacheIndex)
	{
		if (derivativeCacheIndex < this->derivatives.size())
		{
			DerivativeValueCache* &derivativeValueCache = this->derivatives[derivativeCacheIndex];
			delete derivativeValueCache;
			derivativeValueCache = nullptr;
		}
	}

	void zeroValues()
	{
		for (int i = 0; i < this->componentCount; ++i)
			this->values[i] = 0.0;
	}

private:
	RealFieldValueCache(); // not implemented
	RealFieldValueCache(const RealFieldValueCache &source); // not implemented
	RealFieldValueCache& operator=(const RealFieldValueCache &source); // not implemented
};

typedef std::vector<FieldValueCache*> ValueCacheVector;

struct cmzn_fieldcache
{
private:
	cmzn_region *region;  // accessed: not thread safe. Means region is guaranteed to exist.
	int locationCounter; // incremented whenever domain location changes
	int modifyCounter; // set to match region when location changes; if region value changes, cache is invalid
	Field_location_element_xi location_element_xi;
	Field_location_field_values location_field_values;
	Field_location_node location_node;
	Field_location_time location_time;
	Field_location_element_xi *indexed_location_element_xi;
	unsigned int number_of_indexed_location_element_xi;
	Field_location *location;  // points to currently active location from the above. Always valid.
	ValueCacheVector valueCaches;
	bool assignInCache;
	cmzn_fieldcache *parentCache;  // non-accessed parent cache if this is its sharedWorkingCache; finite element evaluation caches are shared with parent
	cmzn_fieldcache *sharedWorkingCache;  // optional working cache shared by fields evaluating at the same time value
	int access_count;

	/** call whenever location changes to increment location counter */
	void locationChanged()
	{
		++(this->locationCounter);
		this->modifyCounter = this->region->getFieldModifyCounter();
		// Must reset location and evaluation counters otherwise fields will not be re-evaluated
		// Logic assumes counter will overflow back to a negative value to trigger reset
		if (this->locationCounter < 0)
		{
			this->locationCounter = 0;
			this->resetValueCacheEvaluationCounters();
		}
	}

public:

	/** @param parentCacheIn  Optional parent cache this is the sharedWorkingCache of */
	cmzn_fieldcache(cmzn_region *regionIn, cmzn_fieldcache *parentCacheIn = 0);

	~cmzn_fieldcache();

	static cmzn_fieldcache *create(cmzn_region *regionIn);

	cmzn_fieldcache *access()
	{
		++access_count;
		return this;
	}

	static int deaccess(cmzn_fieldcache*& cache)
	{
		if (!cache)
			return CMZN_ERROR_ARGUMENT;
		--(cache->access_count);
		if (cache->access_count <= 0)
			delete cache;
		cache = 0;
		return CMZN_OK;
	}

	inline bool hasRegionModifications() const
	{
		return this->modifyCounter != this->region->getFieldModifyCounter();
	}

	/** Copy location from another field cache */
	void copyLocation(const cmzn_fieldcache &source);

	/** @return  Const reference to current cache location */
	const Field_location& get_location() const
	{
		return *(this->location);
	}

	/** @return  Pointer to element xi location, or nullptr if not this type of location */
	const Field_location_element_xi *get_location_element_xi() const
	{
		return this->location->cast_element_xi();
	}

	/** @return  Pointer to field values location, or nullptr if not this type of location */
	const Field_location_field_values *get_location_field_values() const
	{
		return this->location->cast_field_values();
	}

	/** @return  Pointer to node location, or nullptr if not this type of location */
	const Field_location_node *get_location_node() const
	{
		return this->location->cast_node();
	}

	/** @return  Pointer to time location, or nullptr if not this type of location */
	const Field_location_time *get_location_time() const
	{
		return this->location->cast_time();
	}

	inline int getLocationCounter() const
	{
		return this->locationCounter;
	}

	/** @return  Non-accessed region */
	cmzn_region *getRegion() const
	{
		return this->region;
	}

	/** clear location including resetting time to 0.0 */
	void clearLocation()
	{
		this->location = &this->location_time;
		this->location->set_time(0.0);
		this->locationChanged();
	}

	FE_value getTime() const
	{
		return this->location->get_time();
	}

	/** Set time in location without changing location type. Call clear location first to change to time location type. */
	void setTime(FE_value time)
	{
		// avoid re-setting current time as cache may have valid values already
		if (this->location->get_time() != time)
		{
			this->location->set_time(time);
			this->locationChanged();
		}
	}

	int setElement(cmzn_element *element)
	{
		const double chart_coordinates[MAXIMUM_ELEMENT_XI_DIMENSIONS] = { 0.0, 0.0, 0.0 };
		return this->setMeshLocation(element, chart_coordinates);
	}

	/** @param topLevelElement  Optional top-level element to inherit fields from */
	int setMeshLocation(cmzn_element *element, const double *chart_coordinates,
		cmzn_element *top_level_element = 0)
	{
		if (element && chart_coordinates)
		{
			this->location_element_xi.set_element_xi(element, chart_coordinates, top_level_element);
			if (this->location != &this->location_element_xi)
			{
				this->location_element_xi.set_time(this->location->get_time());
				this->location = &this->location_element_xi;
			}
			this->locationChanged();
			return CMZN_OK;
		}
		return CMZN_ERROR_ARGUMENT;
	}

	/** Set a mesh location where the chart_coordinates are likely to be the same at that index.
	 * Allows pre-calculated basis functions to be kept.
	 * @param topLevelElement  Optional top-level element to inherit fields from */
	int setIndexedMeshLocation(unsigned int index, cmzn_element *element, const double *chart_coordinates,
		cmzn_element *top_level_element = 0);

	int setNode(cmzn_node *node)
	{
		this->location_node.set_node(node);
		if (this->location != &this->location_node)
		{
			this->location_node.set_time(this->location->get_time());
			this->location = &this->location_node;
		}
		this->locationChanged();
		return CMZN_OK;
	}

	/** Set node and the element it is embedded in so number of derivatives
	 * w.r.t. host element parameters can be determined */
	int setNodeWithHostElement(cmzn_node *node, cmzn_element *element)
	{
		this->location_node.set_node_with_host_element(node, element);
		if (this->location != &this->location_node)
		{
			this->location_node.set_time(this->location->get_time());
			this->location = &this->location_node;
		}
		this->locationChanged();
		return CMZN_OK;
	}

	int setFieldReal(cmzn_field *field, int numberOfValues, const double *values);

	FieldValueCache* getValueCache(int cacheIndex) const
	{
		return this->valueCaches[cacheIndex];
	}

	/** call if new field added to initialise value cache, and when cache created for field */
	// NOT THREAD SAFE
	void setValueCache(int cacheIndex, FieldValueCache* valueCache)
	{
		if (cacheIndex < static_cast<int>(valueCaches.size()))
		{
			delete valueCaches[cacheIndex];
		}
		else
		{
			for (int i = static_cast<int>(valueCaches.size()); i <= cacheIndex; ++i)
				valueCaches.push_back(0);
		}
		valueCaches[cacheIndex] = valueCache;
	}

	void resetValueCacheEvaluationCounters()
	{
		const int size = static_cast<int>(valueCaches.size());
		for (int i = 0; i < size; ++i)
		{
			if (valueCaches[i])
				valueCaches[i]->resetEvaluationCounter();
		}
	}

	bool assignInCacheOnly() const
	{
		return assignInCache;
	}

	/** @return  Previous value of flag */
	bool setAssignInCacheOnly(bool newAssignInCache)
	{
		bool oldAssignInCache = assignInCache;
		assignInCache = newAssignInCache;
		locationChanged();
		return oldAssignInCache;
	}

	cmzn_fieldcache *getParentCache()
	{
		return this->parentCache;
	}

	/** @return  Non-accessed field cache */
	cmzn_fieldcache *getOrCreateSharedWorkingCache()
	{
		if (!this->sharedWorkingCache)
			this->sharedWorkingCache = new cmzn_fieldcache(this->region, this);
		return this->sharedWorkingCache;
	}

	/** Remove derivative caches for derivativeCacheIndex for all real field value caches */
	void removeDerivativeCaches(int derivativeCacheIndex);

};

/** Return private extraCache for evaluating fields at different locations and different time. */
inline cmzn_fieldcache *FieldValueCache::getOrCreatePrivateExtraCache(cmzn_region *region)
{
	if (!this->extraCache)
		this->extraCache = new cmzn_fieldcache(region);
	return this->extraCache;
}

/** Return shared extraCache for evaluating at different locations but the same time.
 * Hence can share finite element evaluation caches from fieldCache. */
inline cmzn_fieldcache *FieldValueCache::getOrCreateSharedExtraCache(cmzn_fieldcache& fieldCache)
{
	if (!this->extraCache)
		this->extraCache = fieldCache.getOrCreateSharedWorkingCache()->access();
	return this->extraCache;
}

class StringFieldValueCache : public FieldValueCache
{
public:
	char *stringValue;

	StringFieldValueCache() :
		FieldValueCache(),
		stringValue(0)
	{
	}

	virtual ~StringFieldValueCache()
	{
		DEALLOCATE(stringValue);
	}

	static const StringFieldValueCache* cast(const FieldValueCache* valueCache)
	{
		return FIELD_VALUE_CACHE_CAST<const StringFieldValueCache*>(valueCache);
	}

	static StringFieldValueCache* cast(FieldValueCache* valueCache)
	{
		return FIELD_VALUE_CACHE_CAST<StringFieldValueCache*>(valueCache);
	}

	static const StringFieldValueCache& cast(const FieldValueCache& valueCache)
	{
		return FIELD_VALUE_CACHE_CAST<const StringFieldValueCache&>(valueCache);
	}

	static StringFieldValueCache& cast(FieldValueCache& valueCache)
	{
		return FIELD_VALUE_CACHE_CAST<StringFieldValueCache&>(valueCache);
	}

	void setString(const char *string_in);

	virtual char *getAsString() const;

};

class MeshLocationFieldValueCache : public FieldValueCache
{
public:
	cmzn_element *element;
	FE_value xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];

	MeshLocationFieldValueCache() :
		FieldValueCache(),
		element(0)
	{
	}

	virtual ~MeshLocationFieldValueCache()
	{
		cmzn_element::deaccess(this->element);
	}

	virtual void clear();

	void clearElement()
	{
		cmzn_element::deaccess(this->element);
	}

	static const MeshLocationFieldValueCache* cast(const FieldValueCache* valueCache)
	{
		return FIELD_VALUE_CACHE_CAST<const MeshLocationFieldValueCache*>(valueCache);
	}

	static MeshLocationFieldValueCache* cast(FieldValueCache* valueCache)
	{
		return FIELD_VALUE_CACHE_CAST<MeshLocationFieldValueCache*>(valueCache);
	}

	static const MeshLocationFieldValueCache& cast(const FieldValueCache& valueCache)
	{
		return FIELD_VALUE_CACHE_CAST<const MeshLocationFieldValueCache&>(valueCache);
	}

	static MeshLocationFieldValueCache& cast(FieldValueCache& valueCache)
	{
		return FIELD_VALUE_CACHE_CAST<MeshLocationFieldValueCache&>(valueCache);
	}

	void setMeshLocation(cmzn_element *elementIn, const FE_value *xiIn)
	{
		cmzn_element::reaccess(this->element, elementIn);
		if (elementIn)
		{
			const int dimension = elementIn->getDimension();
			for (int i = 0; i < dimension; ++i)
			{
				this->xi[i] = xiIn[i];
			}
		}
	}

	virtual char *getAsString() const;

};

#endif /* !defined (FIELD_CACHE_HPP) */
