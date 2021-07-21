/*******************************************************************************
 FILE : element_operations.cpp

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

#include <cmath>
#include <cstdlib>
#include "opencmiss/zinc/element.h"
#include "opencmiss/zinc/fieldcache.h"
#include "opencmiss/zinc/fieldconstant.h"
#include "opencmiss/zinc/fieldfiniteelement.h"
#include "opencmiss/zinc/fieldlogicaloperators.h"
#include "opencmiss/zinc/fieldsubobjectgroup.h"
#include "opencmiss/zinc/fieldtime.h"
#include "opencmiss/zinc/mesh.h"
#include "opencmiss/zinc/node.h"
#include "opencmiss/zinc/nodeset.h"
#include "opencmiss/zinc/nodetemplate.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_subobject_group.hpp"
#include "element/element_operations.h"
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_discretization.h"
#include "finite_element/finite_element_mesh.hpp"
#include "general/debug.h"
#include "graphics/auxiliary_graphics_types.h"
#include "general/message.h"
#include "mesh/cmiss_element_private.hpp"
#include "mesh/cmiss_node_private.hpp"

/*
Global constants
----------------
*/

const int CMZN_MAX_GAUSS_POINTS = 4;

const struct
{
	FE_value location;                               FE_value weight;
} lineGaussPt[10] =
{
	// 1 point
	{ 0.5,                                           1.0 },
	// 2 points
	{ (-1.0/sqrt(3.0)+1.0)/2.0,                      0.5 },
	{ (+1.0/sqrt(3.0)+1.0)/2.0,                      0.5 },
	// 3 points
	{ (-sqrt(0.6)+1.0)/2.0,                          5.0/18.0 },
	{ 0.5, 4.0/9.0 },
	{ (+sqrt(0.6)+1.0)/2.0,                          5.0/18.0 },
	// 4 points
	{ (-sqrt((3.0+2.0*sqrt(6.0/5.0))/7.0)+1.0)/2.0,  (18.0-sqrt(30.0))/72.0 },
	{ (-sqrt((3.0-2.0*sqrt(6.0/5.0))/7.0)+1.0)/2.0,  (18.0+sqrt(30.0))/72.0 },
	{ (+sqrt((3.0-2.0*sqrt(6.0/5.0))/7.0)+1.0)/2.0,  (18.0+sqrt(30.0))/72.0 },
	{ (+sqrt((3.0+2.0*sqrt(6.0/5.0))/7.0)+1.0)/2.0,  (18.0-sqrt(30.0))/72.0 }
};
const int lineOffset[CMZN_MAX_GAUSS_POINTS] = { 0, 1, 3, 6 };

const struct
{
	FE_value location[2];                               FE_value weight;
} triangleGaussPt[14] =
{
	// order 1 = 1 point
	{ { 1.0/3.0, 1.0/3.0 },                             0.5 },
	// order 2 = 3 points
	{ { 1.0/6.0, 1.0/6.0 },                             1.0/6.0 },
	{ { 2.0/3.0, 1.0/6.0 },                             1.0/6.0 },
	{ { 1.0/6.0, 2.0/3.0 },                             1.0/6.0 },
	// order 3 = 4 points
	{ { 1.0/5.0, 1.0/5.0 },                             25.0 / 96.0 },
	{ { 3.0/5.0, 1.0/5.0 },                             25.0 / 96.0 },
	{ { 1.0/5.0, 3.0/5.0 },                             25.0 / 96.0 },
	{ { 1.0/3.0, 1.0/3.0 },                             -27.0 / 96.0 },
	// order 4 = 6 points
	{ { 0.091576213509771, 0.091576213509771 },         0.109951743655322*0.5 },
	{ { 0.816847572980459, 0.091576213509771 },         0.109951743655322*0.5 },
	{ { 0.091576213509771, 0.816847572980459 },         0.109951743655322*0.5 },
	{ { 0.445948490915965, 0.108103018168070 },         0.223381589678011*0.5 },
	{ { 0.108103018168070, 0.445948490915965 },         0.223381589678011*0.5 },
	{ { 0.445948490915965, 0.445948490915965 },         0.223381589678011*0.5 }
};
const int triangleOffset[CMZN_MAX_GAUSS_POINTS] = { 0, 1, 4, 8 };
const int triangleCount[CMZN_MAX_GAUSS_POINTS] = { 1, 3, 4, 6 };

const struct
{
	FE_value location[3];                                          FE_value weight;
} tetrahedronGaussPt[21] =
{
	// order 1 = 1 point
	{ { 0.25, 0.25, 0.25 },                                        1.0/6.0 },
	// order 2 = 4 points
	{ { 0.138196601125011, 0.138196601125011, 0.138196601125011 }, 0.25/6.0 },
	{ { 0.585410196624969, 0.138196601125011, 0.138196601125011 }, 0.25/6.0 },
	{ { 0.138196601125011, 0.585410196624969, 0.138196601125011 }, 0.25/6.0 },
	{ { 0.138196601125011, 0.138196601125011, 0.585410196624969 }, 0.25/6.0 },
	// order 3 = 5 points
	{ { 1.0/6.0, 1.0/6.0, 1.0/6.0 },                               0.45/6.0 },
	{ { 1.0/2.0, 1.0/6.0, 1.0/6.0 },                               0.45/6.0 },
	{ { 1.0/6.0, 1.0/2.0, 1.0/6.0 },                               0.45/6.0 },
	{ { 1.0/6.0, 1.0/6.0, 1.0/2.0 },                               0.45/6.0 },
	{ { 0.25, 0.25, 0.25 },                                        -0.8/6.0 },
	// order 4 = 11 points
	{ { 0.071428571428571, 0.071428571428571, 0.071428571428571 }, 0.007622222222222 },
	{ { 0.785714285714286, 0.071428571428571, 0.071428571428571 }, 0.007622222222222 },
	{ { 0.071428571428571, 0.785714285714286, 0.071428571428571 }, 0.007622222222222 },
	{ { 0.071428571428571, 0.071428571428571, 0.785714285714286 }, 0.007622222222222 },
	{ { 0.399403576166799, 0.100596423833201, 0.100596423833201 }, 0.024888888888889 },
	{ { 0.100596423833201, 0.399403576166799, 0.100596423833201 }, 0.024888888888889 },
	{ { 0.399403576166799, 0.399403576166799, 0.100596423833201 }, 0.024888888888889 },
	{ { 0.100596423833201, 0.100596423833201, 0.399403576166799 }, 0.024888888888889 },
	{ { 0.399403576166799, 0.100596423833201, 0.399403576166799 }, 0.024888888888889 },
	{ { 0.100596423833201, 0.399403576166799, 0.399403576166799 }, 0.024888888888889 },
	{ { 0.25, 0.25, 0.25 },                                       -0.013155555555556 }
};
const int tetrahedronOffset[CMZN_MAX_GAUSS_POINTS] = { 0, 1, 5, 10 };
const int tetrahedronCount[CMZN_MAX_GAUSS_POINTS] = { 1, 4, 5, 11 };

template <class Calculate_xi_points_type>
class IntegrationShapePointsMidpoint: public IntegrationShapePoints
{
	class ProcessPoint
	{
		InvokeFunction invokeFunction;
		void *termVoid;
		FE_value weight;

	public:
		ProcessPoint(InvokeFunction invokeFunctionIn, void *termVoidIn, FE_value weightIn) :
			invokeFunction(invokeFunctionIn),
			termVoid(termVoidIn),
			weight(weightIn)
		{
		}

		inline bool operator()(FE_value *xi)
		{
			return (invokeFunction)(this->termVoid, xi, this->weight);
		}
	};

	Calculate_xi_points_type *calculate_xi_points;

public:
	// takes ownership of calculate_xi_pointsIn
	IntegrationShapePointsMidpoint(FE_element_shape *shapeIn, int *numbersOfPointsIn,
			Calculate_xi_points_type *calculate_xi_pointsIn) :
		IntegrationShapePoints(shapeIn, numbersOfPointsIn, calculate_xi_pointsIn->getNumPoints(), 0, 0),
		calculate_xi_points(calculate_xi_pointsIn)
	{
	}

	virtual ~IntegrationShapePointsMidpoint()
	{
		delete calculate_xi_points;
	}

	virtual void forEachPointVirtual(InvokeFunction invokeFunction, void *termVoid)
	{
		ProcessPoint processPoint(invokeFunction, termVoid, calculate_xi_points->getWeight());
		this->calculate_xi_points->forEachPoint(processPoint);
	}
};

IntegrationPointsCache::IntegrationPointsCache(cmzn_element_quadrature_rule quadratureRuleIn,
	int numbersOfPointsCountIn, const int *numbersOfPointsIn) :
	quadratureRule(quadratureRuleIn),
	variableNumbersOfPoints(false)
{
	for (int i = 0; i < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++i)
		this->numbersOfPoints[i] = 1;
	this->setQuadrature(quadratureRuleIn, numbersOfPointsCountIn, numbersOfPointsIn);
}

IntegrationPointsCache::~IntegrationPointsCache()
{
	this->clearCache();
}

void IntegrationPointsCache::clearCache()
{
	for (std::vector<IntegrationShapePoints*>::iterator iter = knownShapePoints.begin();
		iter != knownShapePoints.end(); ++iter)
	{
		IntegrationShapePoints *shapePoints = *iter;
		delete shapePoints;
	}
	this->knownShapePoints.clear();
}

void IntegrationPointsCache::setQuadrature(cmzn_element_quadrature_rule quadratureRuleIn,
	int numbersOfPointsCountIn, const int *numbersOfPointsIn)
{
	int lastNumPointsInDirection = 1;
	this->variableNumbersOfPoints = false;
	bool changed = quadratureRuleIn != this->quadratureRule;
	this->quadratureRule = quadratureRuleIn;
	for (int i = 0; i < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++i)
	{
		const int oldNumberOfPoints = this->numbersOfPoints[i];
		if ((i < numbersOfPointsCountIn) && (numbersOfPointsIn) &&
			(0 < numbersOfPointsIn[i]))
		{
			if ((quadratureRuleIn == CMZN_ELEMENT_QUADRATURE_RULE_GAUSSIAN) &&
				(numbersOfPointsIn[i] > CMZN_MAX_GAUSS_POINTS))
				this->numbersOfPoints[i] = CMZN_MAX_GAUSS_POINTS;
			else
				this->numbersOfPoints[i] = numbersOfPointsIn[i];
			if ((i > 0) && (lastNumPointsInDirection != this->numbersOfPoints[i]))
				this->variableNumbersOfPoints = true;
			lastNumPointsInDirection = numbersOfPoints[i];
		}
		else
			numbersOfPoints[i] = lastNumPointsInDirection;
		if (this->numbersOfPoints[i] != oldNumberOfPoints)
			changed = true;
	}
	if (changed)
		this->clearCache();
}

IntegrationShapePoints *IntegrationPointsCache::getPoints(cmzn_element *element)
{
	FE_element_shape *shape = get_FE_element_shape(element);
	if (!shape)
		return 0;
	const int elementDimension = cmzn_element_get_dimension(element);
	int *useNumbersOfPoints = this->numbersOfPoints;
	int inheritedNumbersOfPoints[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	if (variableNumbersOfPoints)
	{
		FE_value element_to_top_level[9];
		cmzn_element *top_level_element = FE_element_get_top_level_element_conversion(
			element, /*check_top_level_element*/0,
			CMZN_ELEMENT_FACE_TYPE_ALL, element_to_top_level);
		if (top_level_element != element)
		{
			get_FE_element_discretization_from_top_level(element, inheritedNumbersOfPoints,
				top_level_element, this->numbersOfPoints, element_to_top_level);
			useNumbersOfPoints = inheritedNumbersOfPoints;
		}
	}
	const size_t size = this->knownShapePoints.size();
	IntegrationShapePoints *shapePoints;
	for (size_t i = 0; i < size; ++i)
	{
		shapePoints = this->knownShapePoints[i];
		if (shapePoints->shape == shape)
		{
			bool pointsMatch = true;
			if (variableNumbersOfPoints)
				for (int j = 0; j < elementDimension; ++j)
					if (shapePoints->numbersOfPoints[j] != useNumbersOfPoints[j])
					{
						pointsMatch = false;
						break;
					}
			if (pointsMatch)
				return shapePoints;
		}
	}
	cmzn_element_shape_type shape_type = cmzn_element_get_shape_type(element);
	switch (this->quadratureRule)
	{
	case CMZN_ELEMENT_QUADRATURE_RULE_GAUSSIAN:
		{
			int numPoints = 0;
			FE_value *points = 0;
			FE_value *weights = 0;
			switch (shape_type)
			{
				case CMZN_ELEMENT_SHAPE_TYPE_LINE:
				case CMZN_ELEMENT_SHAPE_TYPE_SQUARE:
				case CMZN_ELEMENT_SHAPE_TYPE_CUBE:
				{
					const int dimension = cmzn_element_get_dimension(element);
					int order_offset[MAXIMUM_ELEMENT_XI_DIMENSIONS];
					numPoints = 1;
					for (int i = 0; i < dimension; ++i)
					{
						numPoints *= useNumbersOfPoints[i];
						order_offset[i] = lineOffset[useNumbersOfPoints[i] - 1];
					}
					points = new FE_value[numPoints*dimension];
					weights = new FE_value[numPoints];
					for (int g = 0; g < numPoints; ++g)
					{
						weights[g] = 1.0;
						int shift_g = g;
						for (int i = 0; i < dimension; ++i)
						{
							int g1 = order_offset[i] + (shift_g % useNumbersOfPoints[i]);
							points[g*dimension + i] = lineGaussPt[g1].location;
							weights[g] *= lineGaussPt[g1].weight;
							shift_g /= useNumbersOfPoints[i];
						}
					}
				} break;
				case CMZN_ELEMENT_SHAPE_TYPE_TRIANGLE:
				{
					const int dimension = 2;
					int order = useNumbersOfPoints[0];
					if (useNumbersOfPoints[1] > order)
						order = useNumbersOfPoints[1];
					numPoints = triangleCount[order - 1];
					const int orderOffset = triangleOffset[order - 1];
					points = new FE_value[numPoints*dimension];
					weights = new FE_value[numPoints];
					for (int g = 0; g < numPoints; ++g)
					{
						weights[g] = triangleGaussPt[orderOffset + g].weight;
						for (int i = 0; i < dimension; ++i)
							points[g*dimension + i] = triangleGaussPt[orderOffset + g].location[i];
					}
				} break;
				case CMZN_ELEMENT_SHAPE_TYPE_TETRAHEDRON:
				{
					const int dimension = 3;
					int order = useNumbersOfPoints[0];
					for (int i = 1; i < dimension; ++i)
						if (useNumbersOfPoints[i] > order)
							order = useNumbersOfPoints[i];
					numPoints = tetrahedronCount[order - 1];
					const int orderOffset = tetrahedronOffset[order - 1];
					points = new FE_value[numPoints*dimension];
					weights = new FE_value[numPoints];
					for (int g = 0; g < numPoints; ++g)
					{
						weights[g] = tetrahedronGaussPt[orderOffset + g].weight;
						for (int i = 0; i < dimension; ++i)
							points[g*dimension + i] = tetrahedronGaussPt[orderOffset + g].location[i];
					}
				} break;
				case CMZN_ELEMENT_SHAPE_TYPE_WEDGE12:
				case CMZN_ELEMENT_SHAPE_TYPE_WEDGE13:
				case CMZN_ELEMENT_SHAPE_TYPE_WEDGE23:
				{
					const int dimension = 3;
					int line_axis, tri_axis1, tri_axis2;
					if (shape_type == CMZN_ELEMENT_SHAPE_TYPE_WEDGE12)
					{
						line_axis = 2;
						tri_axis1 = 0;
						tri_axis2 = 1;
					}
					else if (shape_type == CMZN_ELEMENT_SHAPE_TYPE_WEDGE13)
					{
						line_axis = 1;
						tri_axis1 = 0;
						tri_axis2 = 2;
					}
					else // (shape_type == CMZN_ELEMENT_SHAPE_TYPE_WEDGE23)
					{
						line_axis = 0;
						tri_axis1 = 1;
						tri_axis2 = 2;
					}
					const int lineOrder = useNumbersOfPoints[line_axis];
					const int lineOrderOffset = lineOffset[lineOrder - 1];
					int triOrder = useNumbersOfPoints[tri_axis1];
					if (useNumbersOfPoints[tri_axis2] > triOrder)
						triOrder = useNumbersOfPoints[tri_axis2];
					const int tri_count = triangleCount[triOrder - 1];
					const int tri_offset = triangleOffset[triOrder - 1];
					numPoints = lineOrder*tri_count;
					points = new FE_value[numPoints*dimension];
					weights = new FE_value[numPoints];
					for (int h = 0; h < lineOrder; ++h)
					{
						FE_value lineXi = lineGaussPt[lineOrderOffset + h].location;
						FE_value lineWeight = lineGaussPt[lineOrderOffset + h].weight;
						int htri = h*tri_count;
						for (int g = 0; g < tri_count; ++g)
						{
							points[(htri + g)*dimension + tri_axis1] = triangleGaussPt[g + tri_offset].location[0];
							points[(htri + g)*dimension + tri_axis2] = triangleGaussPt[g + tri_offset].location[1];
							points[(htri + g)*dimension + line_axis] = lineXi;
							weights[htri + g] = lineWeight*triangleGaussPt[g + tri_offset].weight;
						}
					}
				} break;
				default:
				{
					display_message(INFORMATION_MESSAGE, "IntegrationPointsCache::getPoints()  "
						"Unknown shape type encountered first for element %d.",
						cmzn_element_get_identifier(element));
					return 0;
				} break;
			}
			shapePoints = new IntegrationShapePoints(shape, useNumbersOfPoints, numPoints, points, weights);
		} break;
	case CMZN_ELEMENT_QUADRATURE_RULE_MIDPOINT:
		{
			switch (shape_type)
			{
				case CMZN_ELEMENT_SHAPE_TYPE_LINE:
					shapePoints = new IntegrationShapePointsMidpoint<Calculate_xi_points_line_cell_centres>(
						shape, useNumbersOfPoints, new Calculate_xi_points_line_cell_centres(useNumbersOfPoints));
					break;
				case CMZN_ELEMENT_SHAPE_TYPE_SQUARE:
					shapePoints = new IntegrationShapePointsMidpoint<Calculate_xi_points_square_cell_centres>(
						shape, useNumbersOfPoints, new Calculate_xi_points_square_cell_centres(useNumbersOfPoints));
					break;
				case CMZN_ELEMENT_SHAPE_TYPE_TRIANGLE:
				{
					int numberOfSimplexPoints = useNumbersOfPoints[0];
					if (useNumbersOfPoints[1] > numberOfSimplexPoints)
						numberOfSimplexPoints = useNumbersOfPoints[1];
					shapePoints = new IntegrationShapePointsMidpoint<Calculate_xi_points_triangle_cell_centres>(
						shape, useNumbersOfPoints, new Calculate_xi_points_triangle_cell_centres(numberOfSimplexPoints));
				}	break;
				case CMZN_ELEMENT_SHAPE_TYPE_CUBE:
					shapePoints = new IntegrationShapePointsMidpoint<Calculate_xi_points_cube_cell_centres>(
						shape, useNumbersOfPoints, new Calculate_xi_points_cube_cell_centres(useNumbersOfPoints));
					break;
				case CMZN_ELEMENT_SHAPE_TYPE_TETRAHEDRON:
				{
					int numberOfSimplexPoints = useNumbersOfPoints[0];
					if (useNumbersOfPoints[1] > numberOfSimplexPoints)
						numberOfSimplexPoints = useNumbersOfPoints[1];
					if (useNumbersOfPoints[2] > numberOfSimplexPoints)
						numberOfSimplexPoints = useNumbersOfPoints[2];
					shapePoints = new IntegrationShapePointsMidpoint<Calculate_xi_points_tetrahedron_cell_centres>(
						shape, useNumbersOfPoints, new Calculate_xi_points_tetrahedron_cell_centres(numberOfSimplexPoints));
				}	break;
				case CMZN_ELEMENT_SHAPE_TYPE_WEDGE12:
				case CMZN_ELEMENT_SHAPE_TYPE_WEDGE13:
				case CMZN_ELEMENT_SHAPE_TYPE_WEDGE23:
				{
					int line_axis, tri_axis1, tri_axis2;
					if (shape_type == CMZN_ELEMENT_SHAPE_TYPE_WEDGE12)
					{
						line_axis = 2;
						tri_axis1 = 0;
						tri_axis2 = 1;
					}
					else if (shape_type == CMZN_ELEMENT_SHAPE_TYPE_WEDGE13)
					{
						line_axis = 1;
						tri_axis1 = 0;
						tri_axis2 = 2;
					}
					else // (shape_type == CMZN_ELEMENT_SHAPE_TYPE_WEDGE23)
					{
						line_axis = 0;
						tri_axis1 = 1;
						tri_axis2 = 2;
					}
					int numberOfSimplexPoints = useNumbersOfPoints[tri_axis1];
					if (useNumbersOfPoints[tri_axis2] > numberOfSimplexPoints)
						numberOfSimplexPoints = useNumbersOfPoints[tri_axis2];
					shapePoints = new IntegrationShapePointsMidpoint<Calculate_xi_points_wedge_cell_centres>(
						shape, useNumbersOfPoints, new Calculate_xi_points_wedge_cell_centres(line_axis, tri_axis1, tri_axis2,
							useNumbersOfPoints[line_axis], numberOfSimplexPoints));
				} break;
				default:
				{
					display_message(INFORMATION_MESSAGE, "IntegrationPointsCache::getPoints()  "
						"Unknown shape type encountered first for element %d.",
						cmzn_element_get_identifier(element));
					return 0;
				} break;
			}
		} break;
	case CMZN_ELEMENT_QUADRATURE_RULE_INVALID:
		{
			display_message(INFORMATION_MESSAGE, "IntegrationPointsCache::getPoints()  "
				"Invalid quadrature rule.");
			return 0;
		} break;
	}
	this->knownShapePoints.push_back(shapePoints);
	return shapePoints;
}

/*
 Global functions
 ----------------
 */

struct FE_element_values_number
/*******************************************************************************
 LAST MODIFIED : 22 December 2000

 DESCRIPTION :
 Data for changing element identifiers.
 ==============================================================================*/
{
	struct FE_element *element;
	int number_of_values;
	FE_value *values;
	int new_number;
};

static int compare_FE_element_values_number_values(
		const void *element_values1_void, const void *element_values2_void)
/*******************************************************************************
 LAST MODIFIED : 22 December 2000

 DESCRIPTION :
 Compares the values in <element_values1> and <element_values2> from the last to
 then first, returning -1 as soon as a value in <element_values1> is less than
 its counterpart in <element_values2>, or 1 if greater. 0 is returned if all
 values are identival. Used as a compare function for qsort.
 ==============================================================================*/
{
	int i, number_of_values, return_code;
	struct FE_element_values_number *element_values1, *element_values2;

	ENTER(compare_FE_element_values_number_values);
	return_code = 0;
	if ((element_values1
			= (struct FE_element_values_number *) element_values1_void)
			&& (element_values2
				= (struct FE_element_values_number *) element_values2_void)
			&& (0 < (number_of_values = element_values1->number_of_values))
			&& (number_of_values == element_values2->number_of_values))
	{
		for (i = number_of_values - 1; (!return_code) && (0 <= i); i--)
		{
			if (element_values1->values[i] < element_values2->values[i])
			{
				return_code = -1;
			}
			else if (element_values1->values[i] > element_values2->values[i])
			{
				return_code = 1;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"compare_FE_element_values_number_values.  Invalid argument(s)");
	}LEAVE;

	return (return_code);
} /* compare_FE_element_values_number_values */

struct FE_element_and_values_to_array_data
{
	cmzn_fieldcache_id field_cache;
	struct FE_element_values_number *element_values;
	struct Computed_field *sort_by_field;
};

static int FE_element_and_values_to_array(struct FE_element *element,
	FE_element_and_values_to_array_data& array_data)
{
	int number_in_xi[MAXIMUM_ELEMENT_XI_DIMENSIONS], number_of_xi_points;
	int dimension, i, return_code;
	struct FE_element_shape *element_shape;
	FE_value_triple *xi_points;

	if (element)
	{
		return_code = 1;
		array_data.element_values->element = element;
		if (array_data.sort_by_field)
		{
			/* get the centre point of the element */
			dimension = get_FE_element_dimension(element);
			for (i = 0; i < dimension; i++)
			{
				number_in_xi[i] = 1;
			}
			element_shape = get_FE_element_shape(element);
			if (FE_element_shape_get_xi_points_cell_centres(element_shape,
				number_in_xi, &number_of_xi_points, &xi_points))
			{
				if (!(array_data.element_values->values &&
					(CMZN_OK == array_data.field_cache->setMeshLocation(element, *xi_points)) &&
					(CMZN_OK == cmzn_field_evaluate_real(array_data.sort_by_field, array_data.field_cache,
						cmzn_field_get_number_of_components(array_data.sort_by_field), array_data.element_values->values))))
				{
					display_message(ERROR_MESSAGE,
						"FE_element_and_values_to_array.  "
						"sort_by field could not be evaluated in element");
					return_code = 0;
				}
				DEALLOCATE(xi_points);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"FE_element_and_values_to_array.  Error getting centre of element");
				return_code = 0;
			}
		}
		(array_data.element_values)++;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_element_and_values_to_array.  Invalid argument(s)");
		return_code = 0;
	}
	return (return_code);
}

int FE_region_change_element_identifiers(struct FE_region *fe_region,
	int dimension, int element_offset,
	struct Computed_field *sort_by_field, FE_value time,
	cmzn_field_element_group_id element_group_field)
{
	int return_code = CMZN_OK;
	FE_mesh *fe_mesh;
	Computed_field_element_group *element_group = 0;
	if (element_group_field)
		element_group = Computed_field_element_group_core_cast(element_group_field);
	if (fe_region && (fe_mesh = FE_region_find_FE_mesh_by_dimension(fe_region, dimension)) &&
		((!element_group) || (element_group->get_fe_mesh() == fe_mesh)))
	{
		FE_region_begin_change(fe_region);
		const int number_of_elements = (element_group) ? element_group->getSize() : fe_mesh->getSize();
		if (0 < number_of_elements)
		{
			const int number_of_values = (sort_by_field) ? cmzn_field_get_number_of_components(sort_by_field) : 0;
			struct FE_element_values_number *element_values;
			if (ALLOCATE(element_values, struct FE_element_values_number, number_of_elements))
			{
				for (int i = 0; i < number_of_elements; ++i)
				{
					element_values[i].number_of_values = number_of_values;
					element_values[i].values = (FE_value *) NULL;
				}
				if (sort_by_field)
				{
					for (int i = 0; i < number_of_elements; ++i)
					{
						if (!ALLOCATE(element_values[i].values, FE_value, number_of_values))
						{
							return_code = CMZN_ERROR_MEMORY;
							break;
						}
					}
				}
				if (CMZN_OK == return_code)
				{
					/* make a linear array of elements in the group in current order */
					struct FE_element_and_values_to_array_data array_data;
					cmzn_fieldmodule_id field_module = cmzn_region_get_fieldmodule(FE_region_get_cmzn_region(fe_region));
					cmzn_fieldcache_id field_cache = cmzn_fieldmodule_create_fieldcache(field_module);
					cmzn_fieldcache_set_time(field_cache, time);
					array_data.field_cache = field_cache;
					array_data.element_values = element_values;
					array_data.sort_by_field = sort_by_field;
					cmzn_elementiterator *iter = (element_group) ? element_group->createElementiterator() : fe_mesh->createElementiterator();
					if (!iter)
					{
						display_message(ERROR_MESSAGE,
							"FE_region_change_element_identifiers.  Failed to create element iterator");
						return_code = CMZN_ERROR_MEMORY;
					}
					else
					{
						cmzn_element *element;
						while (0 != (element = iter->nextElement()))
						{
							if (!FE_element_and_values_to_array(element, array_data))
							{
								display_message(ERROR_MESSAGE,
									"FE_region_change_element_identifiers.  Could not build element/field values array");
								return_code = CMZN_ERROR_GENERAL;
								break;
							}
						}
					}
					cmzn::Deaccess(iter);
					cmzn_fieldcache_destroy(&field_cache);
					cmzn_fieldmodule_destroy(&field_module);
				}
				if (CMZN_OK == return_code)
				{
					if (sort_by_field)
					{
						/* sort by field values with higher components more significant */
						qsort(element_values, number_of_elements, sizeof(struct FE_element_values_number),
							compare_FE_element_values_number_values);
						/* give the elements sequential values starting at element_offset */
						for (int i = 0; i < number_of_elements; ++i)
							element_values[i].new_number = element_offset + i;
					}
					else
					{
						/* offset element numbers by element_offset */
						for (int i = 0; i < number_of_elements; ++i)
							element_values[i].new_number = get_FE_element_identifier(element_values[i].element) + element_offset;
					}
					/* check element numbers are positive, ascending and no element not being renumbered is using new identifier */
					for (int i = 0; i < number_of_elements; ++i)
					{
						if (0 > element_values[i].new_number)
						{
							display_message(ERROR_MESSAGE,
								"FE_region_change_element_identifiers.  Element offset gives negative element numbers");
							return_code = CMZN_ERROR_ARGUMENT;
							break;
						}
						if ((0 < i) && (element_values[i].new_number <= element_values[i - 1].new_number))
						{
							display_message(ERROR_MESSAGE,
								"FE_region_change_element_identifiers.  Element numbers are not strictly increasing");
							return_code = CMZN_ERROR_GENERAL;
							break;
						}
						if (element_group)
						{
							FE_element *element_with_identifier = fe_mesh->findElementByIdentifier(element_values[i].new_number);
							if (element_with_identifier && (!element_group->containsObject(element_with_identifier)))
							{
								display_message(ERROR_MESSAGE,
									"FE_region_change_element_identifiers.  Elements not in group are using the new identifiers");
								return_code = CMZN_ERROR_GENERAL;
								break;
							}
						}
					}
				}
				if (CMZN_OK == return_code)
				{
					/* change identifiers */
					/* maintain next_spare_element_number to renumber elements in same
					 group which already have the same number as the new_number */
					int next_spare_element_number = element_values[number_of_elements - 1].new_number + 1;
					for (int i = 0; i < number_of_elements; ++i)
					{
						cmzn_element *element_with_identifier = fe_mesh->findElementByIdentifier(element_values[i].new_number);
						/* only modify if element doesn't already have correct identifier */
						if (element_with_identifier != element_values[i].element)
						{
							if (element_with_identifier)
							{
								next_spare_element_number = fe_mesh->get_next_FE_element_identifier(next_spare_element_number);
								return_code = element_with_identifier->setIdentifier(next_spare_element_number);
								if (return_code != CMZN_OK)
								{
									display_message(ERROR_MESSAGE,
										"FE_region_change_element_identifiers.  Could not change identifier of element currently using identifier");
									break;
								}
							}
							return_code = element_values[i].element->setIdentifier(element_values[i].new_number);
							if (return_code != CMZN_OK)
							{
								display_message(ERROR_MESSAGE,
									"FE_region_change_element_identifiers.  Could not change element identifier");
								break;
							}
						}
					}
				}
				for (int i = 0; i < number_of_elements; ++i)
				{
					if (element_values[i].values)
						DEALLOCATE(element_values[i].values);
				}
				DEALLOCATE(element_values);
			}
			else
			{
				display_message(ERROR_MESSAGE, "FE_region_change_element_identifiers.  Not enough memory");
				return_code = CMZN_ERROR_MEMORY;
			}
		}
		FE_region_end_change(fe_region);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_change_element_identifiers.  Invalid argument(s)");
		return_code = CMZN_ERROR_ARGUMENT;
	}
	return (return_code);
}

cmzn_field_id cmzn_mesh_create_conditional_field_from_identifier_ranges(
	cmzn_mesh_id mesh, struct Multi_range *identifierRanges)
{
	if (!(mesh && identifierRanges))
	{
		display_message(ERROR_MESSAGE, "cmzn_mesh_create_conditional_field_from_identifier_ranges.  Invalid argument(s)");
		return 0;
	}
	cmzn_fieldmodule *fieldmodule = cmzn_mesh_get_fieldmodule(mesh);
	cmzn_fieldmodule_begin_change(fieldmodule);
	cmzn_field *conditionalField = cmzn_fieldmodule_create_field_element_group(fieldmodule, mesh);
	cmzn_field_element_group *elementGroupField = cmzn_field_cast_element_group(conditionalField);
	Computed_field_element_group *elementGroup = Computed_field_element_group_core_cast(elementGroupField);
	if (elementGroup)
	{
		const int number_of_ranges = Multi_range_get_number_of_ranges(identifierRanges);
		for (int i = 0; i < number_of_ranges; ++i)
		{
			int start, stop;
			Multi_range_get_range(identifierRanges, i, &start, &stop);
			if (CMZN_OK != elementGroup->addObjectsInIdentifierRange(start, stop))
			{
				cmzn_field_destroy(&conditionalField);
				break;
			}
		}
	}
	else
		cmzn_field_destroy(&conditionalField);
	cmzn_field_element_group_destroy(&elementGroupField);
	cmzn_fieldmodule_end_change(fieldmodule);
	cmzn_fieldmodule_destroy(&fieldmodule);
	if (!conditionalField)
		display_message(ERROR_MESSAGE, "cmzn_mesh_create_conditional_field_from_identifier_ranges.  Failed");
	return conditionalField;
}

cmzn_field_id cmzn_mesh_create_conditional_field_from_ranges_and_selection(
	cmzn_mesh_id mesh, struct Multi_range *identifierRanges,
	cmzn_field_id conditionalField1, cmzn_field_id conditionalField2,
	cmzn_field_id conditionalField3, FE_value time)
{
	if (!mesh)
		return 0;
	bool time_varying = false;
	cmzn_field *returnField = 0;
	bool error = false;
	cmzn_fieldmodule *fieldmodule = cmzn_mesh_get_fieldmodule(mesh);
	cmzn_fieldmodule_begin_change(fieldmodule);
	if (identifierRanges && (Multi_range_get_number_of_ranges(identifierRanges) > 0)) // empty ranges mean not specified
	{
		returnField = cmzn_mesh_create_conditional_field_from_identifier_ranges(mesh, identifierRanges);
		if (!returnField)
			error = true;
	}
	cmzn_field *conditionalFields[3] = { conditionalField1, conditionalField2, conditionalField3 };
	for (int i = 0; i < 3; ++i)
	{
		if (conditionalFields[i])
		{
			if (Computed_field_has_multiple_times(conditionalFields[i]))
				time_varying = true;
			if (returnField)
			{
				cmzn_field *tmpField = returnField;
				returnField = cmzn_fieldmodule_create_field_and(fieldmodule, tmpField, conditionalFields[i]);
				cmzn_field_destroy(&tmpField);
				if (!returnField)
				{
					error = true;
					break;
				}
			}
			else
				returnField = cmzn_field_access(conditionalFields[i]);
		}
	}
	if (returnField)
	{
		if (time_varying)
		{
			cmzn_field *tmpField = returnField;
			cmzn_field *timeField = cmzn_fieldmodule_create_field_constant(fieldmodule, 1, &time);
			returnField = cmzn_fieldmodule_create_field_time_lookup(fieldmodule, tmpField, timeField);
			cmzn_field_destroy(&tmpField);
			if (!returnField)
				error = true;
			cmzn_field_destroy(&timeField);
		}
	}
	else
	{
		// create an always true conditional field
		const double one = 1;
		returnField = cmzn_fieldmodule_create_field_constant(fieldmodule, 1, &one);
		if (!returnField)
			error = true;
	}
	cmzn_fieldmodule_end_change(fieldmodule);
	cmzn_fieldmodule_destroy(&fieldmodule);
	if (error)
	{
		display_message(ERROR_MESSAGE, "cmzn_mesh_create_conditional_field_from_ranges_and_selection.  Failed");
		cmzn_field_destroy(&returnField);
	}
	return returnField;
}

int cmzn_mesh_create_gauss_points(cmzn_mesh_id mesh, int order,
	cmzn_nodeset_id gauss_points_nodeset, int first_identifier,
	cmzn_field_stored_mesh_location_id gauss_location_field,
	cmzn_field_finite_element_id gauss_weight_field)
{
	int return_code = 0;
	cmzn_region_id nodeset_region = cmzn_nodeset_get_region_internal(gauss_points_nodeset);
	cmzn_region_id master_region = cmzn_mesh_get_region_internal(mesh);
	cmzn_field_id gauss_location_field_base = cmzn_field_stored_mesh_location_base_cast(gauss_location_field);
	cmzn_field_id gauss_weight_field_base = cmzn_field_finite_element_base_cast(gauss_weight_field);
	if (mesh && (1 <= order) && (order <= 4) && gauss_points_nodeset && (first_identifier >= 0) &&
		(cmzn_nodeset_get_region_internal(gauss_points_nodeset) == master_region) &&
		gauss_location_field && gauss_weight_field &&
		(cmzn_field_get_number_of_components(gauss_location_field_base) == 1))
	{
		return_code = 1;
		const int dimension = cmzn_mesh_get_dimension(mesh);
		// GRC add more control
		IntegrationPointsCache integrationCache(CMZN_ELEMENT_QUADRATURE_RULE_GAUSSIAN, 1, &order);
		int numPoints;
		double xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
		double weight;
		cmzn_fieldmodule_id field_module = cmzn_region_get_fieldmodule(nodeset_region);
		cmzn_fieldmodule_begin_change(field_module);
		cmzn_fieldcache_id field_cache = cmzn_fieldmodule_create_fieldcache(field_module);
		cmzn_nodetemplate_id node_template = cmzn_nodeset_create_nodetemplate(gauss_points_nodeset);
		if (!cmzn_nodetemplate_define_field(node_template, gauss_location_field_base))
			return_code = 0;
		if (!cmzn_nodetemplate_define_field(node_template, gauss_weight_field_base))
			return_code = 0;
		cmzn_elementiterator_id iterator = cmzn_mesh_create_elementiterator(mesh);
		cmzn_element_id element = 0;
		int id = first_identifier;
		bool first_unknown_shape = true;
		cmzn_nodeset_id master_gauss_points_nodeset = cmzn_nodeset_get_master_nodeset(gauss_points_nodeset);
		while ((0 != (element = cmzn_elementiterator_next_non_access(iterator))) && return_code)
		{
			IntegrationShapePoints *shapePoints = integrationCache.getPoints(element);
			if (0 == shapePoints)
			{
				if (first_unknown_shape)
				{
					display_message(INFORMATION_MESSAGE, "gfx create gauss_points:  "
						"Unknown shape type encountered first for element %d. Ignoring.",
						cmzn_element_get_identifier(element));
					first_unknown_shape = 0;
				}
				continue;
			}
			numPoints = shapePoints->getNumPoints();
			for (int p = 0; p < numPoints; p++)
			{
				shapePoints->getPoint(p, xi, &weight);
				cmzn_node_id node = 0;
				while ((0 != (node = cmzn_nodeset_find_node_by_identifier(master_gauss_points_nodeset, id))))
				{
					cmzn_node_destroy(&node);
					++id;
				}
				node = cmzn_nodeset_create_node(gauss_points_nodeset, id, node_template);
				cmzn_fieldcache_set_node(field_cache, node);
				cmzn_field_assign_mesh_location(gauss_location_field_base, field_cache, element, dimension, xi);
				cmzn_field_assign_real(gauss_weight_field_base, field_cache, /*number_of_values*/1, &weight);
				cmzn_node_destroy(&node);
				id++;
			}
		}
		cmzn_nodeset_destroy(&master_gauss_points_nodeset);
		cmzn_elementiterator_destroy(&iterator);
		cmzn_nodetemplate_destroy(&node_template);
		cmzn_fieldcache_destroy(&field_cache);
		cmzn_fieldmodule_end_change(field_module);
		cmzn_fieldmodule_destroy(&field_module);
	}
	return return_code;
}
