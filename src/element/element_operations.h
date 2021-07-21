/*******************************************************************************
FILE : element_operations.h

LAST MODIFIED : 3 March 2003

DESCRIPTION :
FE_element functions that utilise non finite element data structures and
therefore cannot reside in finite element modules.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (ELEMENT_OPERATIONS_H)
#define ELEMENT_OPERATIONS_H

#include "opencmiss/zinc/types/fieldfiniteelementid.h"
#include "opencmiss/zinc/types/fieldsubobjectgroupid.h"
#include "opencmiss/zinc/types/nodesetid.h"
#include "computed_field/computed_field.h"
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_region.h"
#include "general/multi_range.h"
#include "selection/element_point_ranges_selection.h"
#include <vector>

class IntegrationShapePoints
{
	friend class IntegrationPointsCache;

protected:
	FE_element_shape *shape;  // not accessed
	int dimension;
	int numbersOfPoints[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	int numPoints;
	FE_value *points;
	FE_value *weights;

public:
	typedef bool (*InvokeFunction)(void *, FE_value *xi, FE_value weight);

	// takes ownership of points and weights arrays
	IntegrationShapePoints(FE_element_shape *shapeIn, int *numbersOfPointsIn,
		int numPointsIn, FE_value *pointsIn, FE_value *weightsIn) :
		shape(shapeIn),
		dimension(get_FE_element_shape_dimension(shapeIn)),
		numPoints(numPointsIn),
		points(pointsIn),
		weights(weightsIn)
	{
		for (int i = 0; i < this->dimension; ++i)
			this->numbersOfPoints[i] = numbersOfPointsIn[i];
	}

	virtual ~IntegrationShapePoints()
	{
		delete[] this->points;
		delete[] this->weights;
	}

	int getNumPoints()
	{
		return this->numPoints;
	}

	void getPoint(int index, FE_value *xi, FE_value *weight)
	{
		for (int i = 0; i < this->dimension; ++i)
			xi[i] = this->points[index*dimension + i];
		*weight = this->weights[index];
	}

	template<class IntegralTerm>
		void forEachPoint(IntegralTerm& term)
	{
		if (this->points)
		{
			for (int i = 0; i < this->numPoints; ++i)
				if (!term(points + i*dimension, weights[i]))
					return;
		}
		else
			this->forEachPointVirtual(IntegralTerm::invoke, (void*)&term);
	}

	virtual void forEachPointVirtual(InvokeFunction invokeFunction, void *termVoid)
	{
		for (int i = 0; i < this->numPoints; ++i)
			if (!(invokeFunction)(termVoid, points + i*dimension, weights[i]))
				return;
	}

private:
	IntegrationShapePoints();
	IntegrationShapePoints(const IntegrationShapePoints& source);
	IntegrationShapePoints& operator=(const IntegrationShapePoints& source);
};

class IntegrationPointsCache
{
private:
	std::vector<IntegrationShapePoints*> knownShapePoints;
	cmzn_element_quadrature_rule quadratureRule;
	int numbersOfPoints[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	bool variableNumbersOfPoints;

	void clearCache();

public:
	IntegrationPointsCache(cmzn_element_quadrature_rule quadratureRuleIn,
		int numbersOfPointsCountIn, const int *numbersOfPointsIn);

	~IntegrationPointsCache();

	/** Set the supplied quadrature rule and numbers of points.
	 * Cache is cleared if values change */
	void setQuadrature(cmzn_element_quadrature_rule quadratureRuleIn,
		int numbersOfPointsCountIn, const int *numbersOfPointsIn);

	/** @return number of points */
	IntegrationShapePoints *getPoints(cmzn_element *element);
};

/*
Global functions
----------------
*/

/**
 * Changes the identifiers of all elements of dimension in fe_region, or
 * restricted to optional element_group. If sort_by_field is NULL, adds
 * element_offset to the identifiers.
 * If sort_by_field is specified, it is evaluated at the centre of all elements
 * in the group and the elements are sorted by it - changing fastest with the
 * first component and keeping the current order where the field has the same
 * values.
 * Fails with no change if any other elements other than those being changed
 * have any of the new identifiers.
 * @param element_group_field  Optional element group field to change
 * identifiers for. Must be from same region.
 * @return  Zinc status code CMZN_OK on success, any other value on failure.
 */
int FE_region_change_element_identifiers(struct FE_region *fe_region,
	int dimension, int element_offset,
	struct Computed_field *sort_by_field, FE_value time,
	cmzn_field_element_group_id element_group_field);

/**
 * @return  A conditional field returning 1 (true) for all element identifiers
 * of mesh in the given ranges. Returned field is accessed. Returns 0 on error.
 */
cmzn_field_id cmzn_mesh_create_conditional_field_from_identifier_ranges(
	cmzn_mesh_id mesh, struct Multi_range *identifierRanges);

/**
 * @param time  If other conditional fields are time-varying the result is
 * wrapped in a time_lookup field for this time to ensure correct evaluation.
 * @return  A conditional field that is the logical AND of an element group
 * field formed from the optional identifier ranges with any of the 3 supplied
 * conditional fields. Field returns true if no ranges or conditionals supplied.
 * Returned field is accessed. Returns 0 on error.
 */
cmzn_field_id cmzn_mesh_create_conditional_field_from_ranges_and_selection(
	cmzn_mesh_id mesh, struct Multi_range *identifierRanges,
	cmzn_field_id conditionalField1, cmzn_field_id conditionalField2,
	cmzn_field_id conditionalField3, FE_value time);

/***************************************************************************//**
 * Create points in gauss_points_nodeset with embedded locations and weights
 * matching the Gauss quadrature points in all elements of mesh.
 * Supports all main element shapes up to order 4.
 *
 * @param mesh  The mesh to create gauss points for.
 * @param order  The 1-D polynomial order from 1 to 4, which for line shapes
 * gives that number of Gauss points per element dimension.
 * @param gauss_points_nodeset  The nodeset to create gauss points in.
 * @param first_identifier  The minimum identifier to use for the gauss points.
 * @param gauss_location_field  Field to define at gauss points for storing
 * gauss point element_xi location.
 * @param gauss_weight_field  Scalar field to define at gauss points for storing
 * real Gauss point weight.
 * @return  1 on success, 0 on failure.
 */
int cmzn_mesh_create_gauss_points(cmzn_mesh_id mesh, int order,
	cmzn_nodeset_id gauss_points_nodeset, int first_identifier,
	cmzn_field_stored_mesh_location_id gauss_location_field,
	cmzn_field_finite_element_id gauss_weight_field);

#endif /* !defined (ELEMENT_OPERATIONS_H) */
