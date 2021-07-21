/**
 * FILE : field_derivative.hpp
 *
 * Field derivative defining order and type of derivative operator to apply
 * to any given field. Each FieldDerivative has a unique index within its
 * owning region for efficient look up in field cache. Object describes how to
 * evaluate derivative, including links to next lower FieldDerivative so
 * can evaluate downstream derivatives using rules.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (__FIELD_DERIVATIVE_HPP__)
#define __FIELD_DERIVATIVE_HPP__

#include "opencmiss/zinc/types/regionid.h"

class FE_mesh;
struct cmzn_fieldparameters;
class Field_location;

class FieldDerivative
{
private:
	cmzn_region *region;  // non-accessed pointer to owning region
	FieldDerivative *lowerDerivative;  // next lower field derivative, accessed
	int cacheIndex;  // unique index of values in field value cache for region; 0 == value
	//int order;  // total order of derivative
	FE_mesh *mesh;  // non-accessed as mesh manages FieldDerivative with mesh
	const int meshDimension;  // cached from mesh
	const int meshOrder;  // order of derivatives w.r.t. mesh chart
	cmzn_fieldparameters *fieldparameters;  // non-accessed as managed by it
	const int parameterOrder; // order of derivatives w.r.t. field parameters
	int access_count;

	/**
	 * Note that if mesh and fieldparameters defined, mesh derivative is applied first,
	 * so parameter derivatives cycle faster in results.
	 * @param region  Owning region for this derivative.
	 * @param meshIn  Mesh for chart derivative, or nullptr if none.
	 * @param meshOrderIn  Order of derivative wrt mesh, must be 0 if no mesh.
	 * @param fieldparametersIn  Field parameters derivative is wrt, or nullptr if none.
	 * @param meshOrderIn  Order of derivative wrt parameters, must be 0 if no mesh.
	 * @param lowerDerivative  Must be supplied if orderIn > 1.
	 */
	FieldDerivative(cmzn_region *regionIn,
		FE_mesh *meshIn, int meshOrderIn,
		cmzn_fieldparameters *fieldparametersIn, int parameterOrderIn,
		FieldDerivative *lowerDerivativeIn);

	~FieldDerivative();

	FieldDerivative();  // not implemented
	FieldDerivative(const FieldDerivative &source);  // not implemented
	FieldDerivative& operator=(const FieldDerivative &source);  // not implemented

public:

	/** Create derivative with respect to mesh chart.
	 * @param mesh  Mesh whose chart the derivative is w.r.t..
	 * @param lowerDerivative  Optional lower field derivative to differentiate
	 * further. Note: may only be a derivative w.r.t. the same mesh if any, and
	 * not w.r.t. field parameters.
	 * @return  Accessed pointer to FieldDerivative, or nullptr if failed.
	 */
	static FieldDerivative *createMeshDerivative(FE_mesh *mesh, FieldDerivative *lowerDerivative);

	/** Create derivative with respect to field parameters.
	 * @param fieldparameters  Field parameters derivative is w.r.t.
	 * @param lowerDerivative  Optional lower field derivative to differentiate
	 * further. Note: may only be a derivative w.r.t. the same parameters if any;
	 * may also include derivative w.r.t. mesh chart.
	 * @return  Accessed pointer to FieldDerivative, or nullptr if failed.
	 */
	static FieldDerivative *createParametersDerivative(cmzn_fieldparameters *fieldparameters, FieldDerivative *lowerDerivative);

	FieldDerivative *access()
	{
		++access_count;
		return this;
	}

	static int deaccess(FieldDerivative* &field_derivative);

	/** Only to be called by owning mesh or field parameters on destruction */
	void clearOwnerPrivate()
	{
		this->mesh = nullptr;
		this->fieldparameters = nullptr;
	}

	// should only be called by owning region
	// default arguments clear the region and cache index
	void setRegionAndCacheIndexPrivate(cmzn_region *regionIn = nullptr, int cacheIndexIn = -1)
	{
		this->region = regionIn;
		this->cacheIndex = cacheIndexIn;
	}

	int getCacheIndex() const
	{
		return this->cacheIndex;
	}

	/** @return  Non-accessed lower derivative or nullptr if none == value. */
	const FieldDerivative *getLowerDerivative() const
	{
		return this->lowerDerivative;
	}

	/* Get the tree order for a function of maximum complexity applying to a
	 * source field with the given tree order: if non-zero round up to the
	 * nearest mesh order, if not total order */
	virtual int getMaximumTreeOrder(int sourceOrder) const
	{
		if (sourceOrder == 0)
			return 0;
		if (sourceOrder <= this->meshOrder)
			return this->meshOrder;
		return this->getTotalOrder();
	}

	/* Get the tree order for a function giving the product of two source fields
	 * with the given tree orders: sum, but limit to the lower of mesh order or
	 * mesh order + parameter order if both source orders are below the line */
	virtual int getProductTreeOrder(int sourceOrder1, int sourceOrder2) const
	{
		if (sourceOrder1 == 0)
			return sourceOrder2;
		if (sourceOrder2 == 0)
			return sourceOrder1;
		if (sourceOrder1 <= this->meshOrder)
		{
			if (sourceOrder2 <= this->meshOrder)
			{
				const int productOrder = sourceOrder1 + sourceOrder2;
				if (productOrder < this->meshOrder)
					return productOrder;
				return this->meshOrder;
			}
			return sourceOrder2;
		}
		else if (sourceOrder2 <= this->meshOrder)
		{
			return sourceOrder1;
		}
		const int productOrder = sourceOrder1 + sourceOrder2 - this->meshOrder;
		const int totalOrder = this->getTotalOrder();
		if (productOrder < totalOrder)
			return productOrder;
		return totalOrder;
	}

	/** @return  Non-accessed mesh if derivative w.r.t. mesh chart, otherwise nullptr */
	FE_mesh *getMesh() const
	{
		return this->mesh;
	}

	int getMeshDimension() const
	{
		return this->meshDimension;
	}

	/** Get order of derivative w.r.t. mesh, >= 0 */
	int getMeshOrder() const
	{
		return this->meshOrder;
	}

	bool isMeshOnly() const
	{
		return (this->mesh) && (!this->fieldparameters);
	}

	/** @return  Number of individually evaluatable terms for mesh derivative part */
	int getMeshTermCount() const
	{
		int termCount = 1;
		for (int d = 0; d < this->meshOrder; ++d)
			termCount *= this->meshDimension;
		return termCount;
	}

	/** @return  Non-accessed field parameters is w.r.t. them, otherwise nullptr */
	cmzn_fieldparameters *getFieldparameters() const
	{
		return this->fieldparameters;
	}

	/** Get order of derivative w.r.t. parameters, >= 0 */
	int getParameterOrder() const
	{
		return this->parameterOrder;
	}

	/** @return  Non-accessed owning region, or nullptr if invalid. */
	cmzn_region *getRegion() const
	{
		return this->region;
	}

	/** Get general term count which may depend on location, e.g. 
	 * fieldparameters are only available at mesh location and can vary between
	 * elements. */
	int getTermCount(const Field_location& fieldLocation) const;

	/** Get total order of derivative */
	int getTotalOrder() const
	{
		return this->meshOrder + this->parameterOrder;
	}

};

#endif /* !defined (__FIELD_DERIVATIVE_HPP__) */
