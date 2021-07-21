/*******************************************************************************
FILE : finite_element_discretization.c

LAST MODIFIED : 21 March 2003

DESCRIPTION :
Functions for discretizing finite elements into points and simple sub-domains.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <math.h>
#include <stdlib.h>

#include "opencmiss/zinc/element.h"
#include "opencmiss/zinc/fieldcache.h"
#include "opencmiss/zinc/status.h"
#include "computed_field/field_cache.hpp"
#include "element/element_operations.h"
#include "finite_element/finite_element_discretization.h"
#include "general/debug.h"
#include "general/matrix_vector.h"
#include "general/random.h"
#include "graphics/graphics_object.h"
#include "general/message.h"
#include "general/statistics.h"

/*
Module types
------------
*/

enum FE_element_shape_category
/*******************************************************************************
LAST MODIFIED : 19 April 2001

DESCRIPTION :
Returned by function categorize_FE_element_shape, this enumerator describes
the overall shape described by an FE_element_shape.
==============================================================================*/
{
	ELEMENT_CATEGORY_1D_LINE,
	ELEMENT_CATEGORY_2D_SQUARE,
	ELEMENT_CATEGORY_2D_TRIANGLE,
	ELEMENT_CATEGORY_2D_POLYGON,
	ELEMENT_CATEGORY_3D_CUBE,
	ELEMENT_CATEGORY_3D_TETRAHEDRON,
	ELEMENT_CATEGORY_3D_TRIANGLE_LINE,
	ELEMENT_CATEGORY_3D_POLYGON_LINE
}; /* enum FE_element_shape_category */

/*
Module functions
----------------
*/

static int categorize_FE_element_shape(struct FE_element_shape *element_shape,
	enum FE_element_shape_category *element_shape_category_address,
	int *number_of_polygon_sides_address, int *linked_xi_directions,
	int *line_direction_address)
/*******************************************************************************
LAST MODIFIED : 19 March 2003

DESCRIPTION :
Upon success the overall <element_shape_category> determined from
<element_shape> will be returned.
If the shape involves a polygon, the <number_of_polygon_sides> is returned.
Returns the 2 <linked_xi_directions>, where first xi is 0, for <element_shape>s
ELEMENT_CATEGORY_3D_TRIANGLE_LINE and ELEMENT_CATEGORY_3D_POLYGON_LINE,
and in <line_direction_address> the xi direction of LINE_SHAPE.
==============================================================================*/
{
	enum FE_element_shape_type shape_type1, shape_type2, shape_type3;
	int return_code;

	ENTER(categorize_FE_element_shape);
	if (element_shape && element_shape_category_address &&
		number_of_polygon_sides_address && linked_xi_directions &&
		line_direction_address)
	{
		return_code = 1;
		const int dimension = get_FE_element_shape_dimension(element_shape);
		switch (dimension)
		{
			case 1:
			{
				/* 1-D can only be line elements */
				*element_shape_category_address = ELEMENT_CATEGORY_1D_LINE;
			} break;
			case 2:
			{
				get_FE_element_shape_xi_shape_type(element_shape,
					0, &shape_type1);
				if (LINE_SHAPE == shape_type1)
				{
					*element_shape_category_address = ELEMENT_CATEGORY_2D_SQUARE;
				}
				else if (SIMPLEX_SHAPE == shape_type1)
				{
					*element_shape_category_address = ELEMENT_CATEGORY_2D_TRIANGLE;
				}
				else if (POLYGON_SHAPE == shape_type1)
				{
					*element_shape_category_address = ELEMENT_CATEGORY_2D_POLYGON;
					get_FE_element_shape_xi_linkage_number(element_shape,
						/*xi_number1*/0, /*xi_number2*/1,
						number_of_polygon_sides_address);
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"categorize_FE_element_shape.  Unknown 2-D shape");
					return_code = 0;
				}
			} break;
			case 3:
			{
				get_FE_element_shape_xi_shape_type(element_shape,
					0, &shape_type1);
				get_FE_element_shape_xi_shape_type(element_shape,
					1, &shape_type2);
				get_FE_element_shape_xi_shape_type(element_shape,
					2, &shape_type3);
				if ((LINE_SHAPE == shape_type1) && (LINE_SHAPE == shape_type2))
				{
					*element_shape_category_address = ELEMENT_CATEGORY_3D_CUBE;
				}
				else if ((SIMPLEX_SHAPE == shape_type1) &&
					(SIMPLEX_SHAPE == shape_type2) && (SIMPLEX_SHAPE == shape_type3))
				{
					*element_shape_category_address = ELEMENT_CATEGORY_3D_TETRAHEDRON;
				}
				else if ((SIMPLEX_SHAPE == shape_type1) ||
					(SIMPLEX_SHAPE == shape_type2))
				{
					*element_shape_category_address = ELEMENT_CATEGORY_3D_TRIANGLE_LINE;
					/* determine linked xi directions */
					if (SIMPLEX_SHAPE == shape_type1)
					{
						linked_xi_directions[0] = 0;
						if (SIMPLEX_SHAPE == shape_type2)
						{
							linked_xi_directions[1] = 1;
							*line_direction_address = 2;
						}
						else
						{
							linked_xi_directions[1] = 2;
							*line_direction_address = 1;
						}
					}
					else
					{
						linked_xi_directions[0] = 1;
						linked_xi_directions[1] = 2;
						*line_direction_address = 0;
					}
				}
				else if ((POLYGON_SHAPE == shape_type1) ||
					(POLYGON_SHAPE == shape_type2))
				{
					*element_shape_category_address = ELEMENT_CATEGORY_3D_POLYGON_LINE;
					/* determine linked xi directions and number of polygon sides */
					if (POLYGON_SHAPE == shape_type1)
					{
						linked_xi_directions[0] = 0;
						if (POLYGON_SHAPE == shape_type2)
						{
							linked_xi_directions[1] = 1;
							*line_direction_address = 2;
						}
						else
						{
							linked_xi_directions[1] = 2;
							*line_direction_address = 1;
						}
					}
					else
					{
						linked_xi_directions[0] = 1;
						linked_xi_directions[1] = 2;
						*line_direction_address = 0;
					}
					get_FE_element_shape_xi_linkage_number(element_shape,
						linked_xi_directions[0], linked_xi_directions[1],
						number_of_polygon_sides_address);
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"categorize_FE_element_shape.  Unknown 3-D shape");
					return_code = 0;
				}
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"categorize_FE_element_shape.  Invalid dimension");
				return_code = 0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"categorize_FE_element_shape.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* categorize_FE_element_shape */

namespace {

class AddXi1
{
	FE_value_triple* xiPos;

public:
	AddXi1(FE_value_triple* xiArray) :
		xiPos(xiArray)
	{
	}

	inline bool operator()(FE_value *xi)
	{
		(*xiPos)[0] = xi[0];
		(*xiPos)[1] = 0.0;
		(*xiPos)[2] = 0.0;
		++xiPos;
		return true;
	}
};

class AddXi2
{
	FE_value_triple* xiPos;

public:
	AddXi2(FE_value_triple* xiArray) :
		xiPos(xiArray)
	{
	}

	inline bool operator()(FE_value *xi)
	{
		(*xiPos)[0] = xi[0];
		(*xiPos)[1] = xi[1];
		(*xiPos)[2] = 0.0;
		++xiPos;
		return true;
	}
};

class AddXi3
{
	FE_value_triple* xiPos;

public:
	AddXi3(FE_value_triple* xiArray) :
		xiPos(xiArray)
	{
	}

	inline bool operator()(FE_value *xi)
	{
		(*xiPos)[0] = xi[0];
		(*xiPos)[1] = xi[1];
		(*xiPos)[2] = xi[2];
		++xiPos;
		return true;
	}
};

} // anonymous namespace

/*
Global functions
----------------
*/

int FE_element_shape_get_xi_points_cell_centres(
	struct FE_element_shape *element_shape, int *number_in_xi,
	int *number_of_xi_points_address, FE_value_triple **xi_points_address)
/*******************************************************************************
LAST MODIFIED : 24 April 2001

DESCRIPTION :
Calculates the <number_of_xi_points> in the centres of uniform cells across the
<element_shape> according to <number_in_xi>.
If <xi_points_address> is supplied an array containing the xi locations of these
points is allocated and put in this address. Xi positions are always returned as
triples with remaining xi coordinates 0 for 1-D and 2-D cases.
Note the actual number and layout is dependent on the <element_shape>; see
comments for simplex and polygons shapes for more details.
==============================================================================*/
{
	enum FE_element_shape_category element_shape_category;
	ZnReal xi_j, xi_k;
	int i, j, k, line_direction, linked_xi_directions[2],
		number_in_xi_around_polygon = 0,
		number_in_xi_simplex = 0, number_of_polygon_sides,
		number_of_xi_points, return_code;
	FE_value_triple *xi, *xi_points;

	ENTER(FE_element_shape_get_xi_points_cell_centres);
	const int element_dimension = get_FE_element_shape_dimension(element_shape);
	if ((0 < element_dimension) && number_in_xi && number_of_xi_points_address)
	{
		return_code = 1;
		number_of_xi_points = 0;
		/* check the number_in_xi */
		for (i = 0; (i < element_dimension) && return_code ; i++)
		{
			if (1 > number_in_xi[i])
			{
				display_message(ERROR_MESSAGE,
					"FE_element_shape_get_xi_points_cell_centres.  "
					"Non-positive number_in_xi");
				return_code = 0;
			}
		}
		/* extract useful information about the element_shape */
		if (!categorize_FE_element_shape(element_shape, &element_shape_category,
			&number_of_polygon_sides, linked_xi_directions, &line_direction))
		{
			display_message(ERROR_MESSAGE,
				"FE_element_shape_get_xi_points_cell_centres.  "
				"Could not categorize element_shape");
			return_code = 0;
		}
		if (return_code)
		{
			/* calculate number_of_xi_points */
			switch (element_shape_category)
			{
				case ELEMENT_CATEGORY_1D_LINE:
				{
					number_of_xi_points = number_in_xi[0];
				} break;
				case ELEMENT_CATEGORY_2D_SQUARE:
				{
					number_of_xi_points = number_in_xi[0]*number_in_xi[1];
				} break;
				case ELEMENT_CATEGORY_2D_TRIANGLE:
				{
					/* always use the maximum number_in_xi in linked directions */
					number_in_xi_simplex = number_in_xi[0];
					if (number_in_xi[1] > number_in_xi_simplex)
					{
						number_in_xi_simplex = number_in_xi[1];
					}
					number_of_xi_points = number_in_xi_simplex*number_in_xi_simplex;
				} break;
				case ELEMENT_CATEGORY_2D_POLYGON:
				{
					/* multiply by number of sides around first linked xi direction */
					number_in_xi_around_polygon = number_in_xi[0]*number_of_polygon_sides;
					number_of_xi_points = number_in_xi_around_polygon*number_in_xi[1];
				} break;
				case ELEMENT_CATEGORY_3D_CUBE:
				{
					number_of_xi_points =
						number_in_xi[0]*number_in_xi[1]*number_in_xi[2];
				} break;
				case ELEMENT_CATEGORY_3D_TETRAHEDRON:
				{
					/* always use the maximum number_in_xi in linked xi directions */
					number_in_xi_simplex = number_in_xi[0];
					if (number_in_xi[1] > number_in_xi_simplex)
					{
						number_in_xi_simplex = number_in_xi[1];
					}
					if (number_in_xi[2] > number_in_xi_simplex)
					{
						number_in_xi_simplex = number_in_xi[2];
					}
					number_of_xi_points = Calculate_xi_points_tetrahedron_cell_centres::calculateNumPoints(number_in_xi_simplex);
				} break;
				case ELEMENT_CATEGORY_3D_TRIANGLE_LINE:
				{
					/* always use the maximum number_in_xi in linked xi directions */
					number_in_xi_simplex = number_in_xi[linked_xi_directions[0]];
					if (number_in_xi[linked_xi_directions[1]] > number_in_xi_simplex)
					{
						number_in_xi_simplex = number_in_xi[linked_xi_directions[1]];
					}
					number_of_xi_points = number_in_xi_simplex*number_in_xi_simplex*
						number_in_xi[line_direction];
				} break;
				case ELEMENT_CATEGORY_3D_POLYGON_LINE:
				{
					/* multiply by number of sides around first linked xi direction */
					number_in_xi_around_polygon =
						number_in_xi[linked_xi_directions[0]]*number_of_polygon_sides;
					number_of_xi_points = number_in_xi_around_polygon*
						number_in_xi[linked_xi_directions[1]]*number_in_xi[line_direction];
				} break;
				default:
				{
					display_message(ERROR_MESSAGE,
						"FE_element_shape_get_xi_points_cell_centres.  "
						"Unknown element shape");
					return_code = 0;
				} break;
			}
			*number_of_xi_points_address = number_of_xi_points;
		}
		if (return_code && xi_points_address)
		{
			if (ALLOCATE(xi_points, FE_value_triple, number_of_xi_points))
			{
				xi = xi_points;
				switch (element_shape_category)
				{
					case ELEMENT_CATEGORY_1D_LINE:
					{
						Calculate_xi_points_line_cell_centres lineCentres(number_in_xi);
						AddXi1 addXi1(xi_points);
						lineCentres.forEachPoint(addXi1);
					} break;
					case ELEMENT_CATEGORY_2D_SQUARE:
					{
						Calculate_xi_points_square_cell_centres squareCentres(number_in_xi);
						AddXi2 addXi2(xi_points);
						squareCentres.forEachPoint(addXi2);
					} break;
					case ELEMENT_CATEGORY_2D_TRIANGLE:
					{
						Calculate_xi_points_triangle_cell_centres triangleCentres(number_in_xi_simplex);
						AddXi2 addXi2(xi_points);
						triangleCentres.forEachPoint(addXi2);
					} break;
					case ELEMENT_CATEGORY_2D_POLYGON:
					{
						for (j = 0; j < number_in_xi[1]; j++)
						{
							xi_j = ((FE_value)j + 0.5)/(FE_value)number_in_xi[1];
							for (i = 0; i < number_in_xi_around_polygon; i++)
							{
								(*xi)[0] = ((FE_value)i + 0.5)/(FE_value)number_in_xi_around_polygon;
								(*xi)[1] = xi_j;
								(*xi)[2] = 0.0;
								xi++;
							}
						}
					} break;
					case ELEMENT_CATEGORY_3D_CUBE:
					{
						Calculate_xi_points_cube_cell_centres cubeCentres(number_in_xi);
						AddXi3 addXi3(xi_points);
						cubeCentres.forEachPoint(addXi3);
					} break;
					case ELEMENT_CATEGORY_3D_TETRAHEDRON:
					{
						Calculate_xi_points_tetrahedron_cell_centres tetrahedronCentres(number_in_xi_simplex);
						AddXi3 addXi3(xi_points);
						tetrahedronCentres.forEachPoint(addXi3);
					} break;
					case ELEMENT_CATEGORY_3D_TRIANGLE_LINE:
					{
						Calculate_xi_points_wedge_cell_centres wedgeCentres(
							line_direction, linked_xi_directions[0], linked_xi_directions[1],
							number_in_xi[line_direction], number_in_xi_simplex);
						AddXi3 addXi3(xi_points);
						wedgeCentres.forEachPoint(addXi3);
					} break;
					case ELEMENT_CATEGORY_3D_POLYGON_LINE:
					{
						for (k = 0; k < number_in_xi[line_direction]; k++)
						{
							xi_k = ((FE_value)k + 0.5)/(FE_value)number_in_xi[line_direction];
							for (j = 0; j < number_in_xi[linked_xi_directions[1]]; j++)
							{
								xi_j =
									((FE_value)j + 0.5)/(FE_value)number_in_xi[linked_xi_directions[1]];
								for (i = 0; i < number_in_xi_around_polygon; i++)
								{
									(*xi)[linked_xi_directions[0]] =
										((FE_value)i + 0.5)/(FE_value)number_in_xi_around_polygon;
									(*xi)[linked_xi_directions[1]] = xi_j;
									(*xi)[line_direction] = xi_k;
									xi++;
								}
							}
						}
					} break;
					default:
					{
						display_message(ERROR_MESSAGE,
							"FE_element_shape_get_xi_points_cell_centres.  "
							"Unknown element shape");
						return_code = 0;
					} break;
				}
				if (return_code)
				{
					*xi_points_address = xi_points;
				}
				else
				{
					DEALLOCATE(xi_points);
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"FE_element_shape_get_xi_points_cell_centres.  "
					"Could not allocate xi_points");
				return_code = 0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_element_shape_get_xi_points_cell_centres.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_element_shape_get_xi_points_cell_centres */

int FE_element_shape_get_xi_points_cell_corners(
	struct FE_element_shape *element_shape, int *number_in_xi,
	int *number_of_xi_points_address, FE_value_triple **xi_points_address)
/*******************************************************************************
LAST MODIFIED : 23 April 2001

DESCRIPTION :
Calculates the <number_of_xi_points> in the corners of uniform cells across the
<element_shape> according to <number_in_xi>.
If <xi_points_address> is supplied an array containing the xi locations of these
points is allocated and put in this address. Xi positions are always returned as
triples with remaining xi coordinates 0 for 1-D and 2-D cases.
Note the actual number and layout is dependent on the <element_shape>; see
comments for simplex and polygons shapes for more details.
==============================================================================*/
{
	enum FE_element_shape_category element_shape_category;
	FE_value xi_j, xi_k;
	int i, j, k, line_direction, linked_xi_directions[2],
		number_in_xi0, number_in_xi1, number_in_xi_around_polygon = 0,
		number_in_xi_simplex = 0, number_of_polygon_sides, number_of_xi_points,
		points_per_row, return_code;
	FE_value_triple *xi, *xi_points;

	ENTER(FE_element_shape_get_xi_points_cell_corners);
	const int element_dimension = get_FE_element_shape_dimension(element_shape);
	if ((0 < element_dimension) && number_in_xi && number_of_xi_points_address)
	{
		return_code = 1;
		number_of_xi_points = 0;
		/* check the number_in_xi */
		for (i = 0; (i < element_dimension) && return_code ; i++)
		{
			if (1 > number_in_xi[i])
			{
				display_message(ERROR_MESSAGE,
					"FE_element_shape_get_xi_points_cell_corners.  "
					"Non-positive number_in_xi");
				return_code = 0;
			}
		}
		/* extract useful information about the element_shape */
		if (!categorize_FE_element_shape(element_shape, &element_shape_category,
			&number_of_polygon_sides, linked_xi_directions, &line_direction))
		{
			display_message(ERROR_MESSAGE,
				"FE_element_shape_get_xi_points_cell_corners.  "
				"Could not categorize element_shape");
			return_code = 0;
		}
		if (return_code)
		{
			/* calculate number_of_xi_points */
			switch (element_shape_category)
			{
				case ELEMENT_CATEGORY_1D_LINE:
				{
					number_of_xi_points = (number_in_xi[0] + 1);
				} break;
				case ELEMENT_CATEGORY_2D_SQUARE:
				{
					number_of_xi_points = (number_in_xi[0] + 1)*(number_in_xi[1] + 1);
				} break;
				case ELEMENT_CATEGORY_2D_TRIANGLE:
				{
					/* always use the maximum number_in_xi in linked directions */
					number_in_xi_simplex = number_in_xi[0];
					if (number_in_xi[1] > number_in_xi_simplex)
					{
						number_in_xi_simplex = number_in_xi[1];
					}
					/* (n + 1) + n + (n - 1) + ... + 2 */
					number_of_xi_points = 1;
					for (i = 0; i < number_in_xi_simplex; i++)
					{
						number_of_xi_points += (i + 2);
					}
				} break;
				case ELEMENT_CATEGORY_2D_POLYGON:
				{
					/* multiply by number of sides around first linked xi direction */
					number_in_xi_around_polygon = number_in_xi[0]*number_of_polygon_sides;
					number_of_xi_points =
						number_in_xi_around_polygon*(number_in_xi[1] + 1);
				} break;
				case ELEMENT_CATEGORY_3D_CUBE:
				{
					number_of_xi_points =
						(number_in_xi[0] + 1)*(number_in_xi[1] + 1)*(number_in_xi[2] + 1);
				} break;
				case ELEMENT_CATEGORY_3D_TETRAHEDRON:
				{
					/* always use the maximum number_in_xi in linked xi directions */
					number_in_xi_simplex = number_in_xi[0];
					if (number_in_xi[1] > number_in_xi_simplex)
					{
						number_in_xi_simplex = number_in_xi[1];
					}
					if (number_in_xi[2] > number_in_xi_simplex)
					{
						number_in_xi_simplex = number_in_xi[2];
					}
					number_of_xi_points = 4;
					points_per_row = 3;
					for (i = 1; i < number_in_xi_simplex; i++)
					{
						points_per_row += (i + 2);
						number_of_xi_points += points_per_row;
					}
				} break;
				case ELEMENT_CATEGORY_3D_TRIANGLE_LINE:
				{
					/* always use the maximum number_in_xi in linked xi directions */
					number_in_xi_simplex = number_in_xi[linked_xi_directions[0]];
					if (number_in_xi[linked_xi_directions[1]] > number_in_xi_simplex)
					{
						number_in_xi_simplex = number_in_xi[linked_xi_directions[1]];
					}
					/* (n + 1) + n + (n - 1) + ... + 2 */
					number_of_xi_points = 1;
					for (i = 0; i < number_in_xi_simplex; i++)
					{
						number_of_xi_points += (i + 2);
					}
					number_of_xi_points *= (number_in_xi[line_direction] + 1);
				} break;
				case ELEMENT_CATEGORY_3D_POLYGON_LINE:
				{
					/* multiply by number of sides around first linked xi direction */
					number_in_xi_around_polygon =
						number_in_xi[linked_xi_directions[0]]*number_of_polygon_sides;
					number_of_xi_points = number_in_xi_around_polygon*
						(number_in_xi[linked_xi_directions[1]] + 1)*
						(number_in_xi[line_direction] + 1);
				} break;
				default:
				{
					display_message(ERROR_MESSAGE,
						"FE_element_shape_get_xi_points_cell_corners.  "
						"Unknown element shape");
					return_code = 0;
				} break;
			}
			*number_of_xi_points_address = number_of_xi_points;
		}
		if (return_code && xi_points_address)
		{
			if (ALLOCATE(xi_points, FE_value_triple, number_of_xi_points))
			{
				xi = xi_points;
				switch (element_shape_category)
				{
					case ELEMENT_CATEGORY_1D_LINE:
					{
						for (i = 0; i <= number_in_xi[0]; i++)
						{
							(*xi)[0] = (FE_value)i/(FE_value)number_in_xi[0];
							(*xi)[1] = 0.0;
							(*xi)[2] = 0.0;
							xi++;
						}
					} break;
					case ELEMENT_CATEGORY_2D_SQUARE:
					{
						for (j = 0; j <= number_in_xi[1]; j++)
						{
							xi_j = (FE_value)j/(FE_value)number_in_xi[1];
							for (i = 0; i <= number_in_xi[0]; i++)
							{
								(*xi)[0] = (FE_value)i/(FE_value)number_in_xi[0];
								(*xi)[1] = xi_j;
								(*xi)[2] = 0.0;
								xi++;
							}
						}
					} break;
					case ELEMENT_CATEGORY_2D_TRIANGLE:
					{
						for (j = 0; j <= number_in_xi_simplex; j++)
						{
							xi_j = (FE_value)j/(FE_value)number_in_xi_simplex;
							number_in_xi0 = number_in_xi_simplex - j;
							for (i = 0; i <= number_in_xi0; i++)
							{
								(*xi)[0] = (FE_value)i/(FE_value)number_in_xi_simplex;
								(*xi)[1] = xi_j;
								(*xi)[2] = 0.0;
								xi++;
							}
						}
					} break;
					case ELEMENT_CATEGORY_2D_POLYGON:
					{
						for (j = 0; j <= number_in_xi[1]; j++)
						{
							xi_j = (FE_value)j/(FE_value)number_in_xi[1];
							for (i = 0; i < number_in_xi_around_polygon; i++)
							{
								(*xi)[0] = (FE_value)i/(FE_value)number_in_xi_around_polygon;
								(*xi)[1] = xi_j;
								(*xi)[2] = 0.0;
								xi++;
							}
						}
					} break;
					case ELEMENT_CATEGORY_3D_CUBE:
					{
						for (k = 0; k <= number_in_xi[2]; k++)
						{
							xi_k = (FE_value)k/(FE_value)number_in_xi[2];
							for (j = 0; j <= number_in_xi[1]; j++)
							{
								xi_j = (FE_value)j/(FE_value)number_in_xi[1];
								for (i = 0; i <= number_in_xi[0]; i++)
								{
									(*xi)[0] = (FE_value)i/(FE_value)number_in_xi[0];
									(*xi)[1] = xi_j;
									(*xi)[2] = xi_k;
									xi++;
								}
							}
						}
					} break;
					case ELEMENT_CATEGORY_3D_TETRAHEDRON:
					{
						for (k = 0; k <= number_in_xi_simplex; k++)
						{
							xi_k = (FE_value)k/(FE_value)number_in_xi_simplex;
							number_in_xi1 = number_in_xi_simplex - k;
							for (j = 0; j <= number_in_xi1; j++)
							{
								xi_j = (FE_value)j/(FE_value)number_in_xi_simplex;
								number_in_xi0 = number_in_xi1 - j;
								for (i = 0; i <= number_in_xi0; i++)
								{
									(*xi)[0] = (FE_value)i/(FE_value)number_in_xi_simplex;
									(*xi)[1] = xi_j;
									(*xi)[2] = xi_k;
									xi++;
								}
							}
						}
					} break;
					case ELEMENT_CATEGORY_3D_TRIANGLE_LINE:
					{
						/*???debug*/int n = 0;
						for (k = 0; k <= number_in_xi[line_direction]; k++)
						{
							xi_k = (FE_value)k/(FE_value)number_in_xi[line_direction];
							for (j = 0; j <= number_in_xi_simplex; j++)
							{
								xi_j = (FE_value)j/(FE_value)number_in_xi_simplex;
								number_in_xi0 = number_in_xi_simplex - j;
								for (i = 0; i <= number_in_xi0; i++)
								{
						/*???debug*/n++;
									(*xi)[linked_xi_directions[0]] =
										(FE_value)i/(FE_value)number_in_xi_simplex;
									(*xi)[linked_xi_directions[1]] = xi_j;
									(*xi)[line_direction] = xi_k;
									xi++;
								}
							}
						}
#if defined (DEBUG_CODE)
						/*???debug*/printf("3D_TRIANGLE_LINE n = %d = %d\n",
							n, number_of_xi_points);
#endif /* defined (DEBUG_CODE) */
					} break;
					case ELEMENT_CATEGORY_3D_POLYGON_LINE:
					{
						for (k = 0; k <= number_in_xi[line_direction]; k++)
						{
							xi_k = (FE_value)k/(FE_value)number_in_xi[line_direction];
							for (j = 0; j <= number_in_xi[linked_xi_directions[1]]; j++)
							{
								xi_j = (FE_value)j/(FE_value)number_in_xi[linked_xi_directions[1]];
								for (i = 0; i < number_in_xi_around_polygon; i++)
								{
									(*xi)[linked_xi_directions[0]] =
										(FE_value)i/(FE_value)number_in_xi_around_polygon;
									(*xi)[linked_xi_directions[1]] = xi_j;
									(*xi)[line_direction] = xi_k;
									xi++;
								}
							}
						}
					} break;
					default:
					{
						display_message(ERROR_MESSAGE,
							"FE_element_shape_get_xi_points_cell_corners.  "
							"Unknown element shape");
						return_code = 0;
					} break;
				}
				if (return_code)
				{
					*xi_points_address = xi_points;
				}
				else
				{
					DEALLOCATE(xi_points);
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"FE_element_shape_get_xi_points_cell_corners.  "
					"Could not allocate xi_points");
				return_code = 0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_element_shape_get_xi_points_cell_corners.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_element_shape_get_xi_points_cell_corners */

#define TOLERANCE 0.0001
#define WITHIN_TOLERANCE(value1, value2) \
	((value1 < value2 + TOLERANCE) && (value1 > value2 - TOLERANCE))

int FE_element_shape_get_indices_for_xi_location_in_cell_corners(
	struct FE_element_shape *element_shape, const int *number_in_xi,
	const FE_value *xi, int *indices)
/*******************************************************************************
LAST MODIFIED : 18 October 2005

DESCRIPTION :
Determines if <xi> cooresponds to a location on the corners of uniform cells
across the <element_shape> according to <number_in_xi>.
If so, returns 1 and sets the <indices> to match.
Otherwise the routine returns 0.
==============================================================================*/
{
	enum FE_element_shape_category element_shape_category;
	int i, line_direction, linked_xi_directions[2],
		number_of_polygon_sides, return_code = 1;

	const int element_dimension = get_FE_element_shape_dimension(element_shape);
	if ((0 < element_dimension) && number_in_xi && xi && indices)
	{
		/* check the number_in_xi */
		for (i = 0; (i < element_dimension) && return_code ; i++)
		{
			if (number_in_xi[i] < 0)
			{
				display_message(ERROR_MESSAGE,
					"FE_element_shape_get_indices_for_xi_location_in_cell_corners.  "
					"Negative number_in_xi");
				return 0;
			}
		}
		/* extract useful information about the element_shape */
		if (!categorize_FE_element_shape(element_shape, &element_shape_category,
			&number_of_polygon_sides, linked_xi_directions, &line_direction))
		{
			display_message(ERROR_MESSAGE,
				"FE_element_shape_get_indices_for_xi_location_in_cell_corners.  "
				"Could not categorize element_shape");
			return 0;
		}
		if (return_code)
		{
			switch (element_shape_category)
			{
				case ELEMENT_CATEGORY_1D_LINE:
				case ELEMENT_CATEGORY_2D_SQUARE:
				case ELEMENT_CATEGORY_3D_CUBE:
				{
					for (i = 0 ; i < element_dimension ; i++)
					{
						if (number_in_xi[i] == 0)
						{
							indices[i] = 0; // constant in xi direction, so any xi coordinate is fine
						}
						else
						{
							indices[i] = (int)(number_in_xi[i] * xi[i] + 0.5);
							if (!WITHIN_TOLERANCE((FE_value)indices[i] / (FE_value)number_in_xi[i], xi[i]))
							{
								return 0;
							}
						}
					}
				} break;
				case ELEMENT_CATEGORY_2D_TRIANGLE:
				case ELEMENT_CATEGORY_2D_POLYGON:
				case ELEMENT_CATEGORY_3D_TETRAHEDRON:
				case ELEMENT_CATEGORY_3D_TRIANGLE_LINE:
				case ELEMENT_CATEGORY_3D_POLYGON_LINE:
				default:
				{
					display_message(ERROR_MESSAGE,
						"FE_element_shape_get_indices_for_xi_location_in_cell_corners.  "
						"Unknown element shape");
					return 0;
				} break;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_element_shape_get_indices_for_xi_location_in_cell_corners.  Invalid argument(s)");
		return 0;
	}
	return 1;
}

#define XI_POINTS_REALLOCATE_SIZE 50

static int FE_element_add_xi_points_1d_line_cell_random(
	struct FE_element	*element,
	cmzn_element_point_sampling_mode sampling_mode, FE_value xi_centre,
	FE_value delta_xi, cmzn_fieldcache_id field_cache,
	struct Computed_field *coordinate_field,
	struct Computed_field *density_field, int *number_of_xi_points,
	FE_value_triple **xi_points, int *number_of_xi_points_allocated)
/*******************************************************************************
LAST MODIFIED : 21 March 2003

DESCRIPTION :
Adds to the <number_of_xi_points> the number of points to be added according to
the <sampling_mode>; see FE_element_get_xi_points_cell_random.
If <xi_points> and current <number_of_xi_points_allocated> are supplied, the
array is enlarged if necessary and the new points added at random locations.
#Define XI_POINTS_REALLOCATE_SIZE provides a minimum step size for enlarging the
<xi_points> array, to prevent too many allocations.
==============================================================================*/
{
	ZnReal a[3], length, expected_number;
	FE_value coordinates[3], density;
	FE_value centre_xi1, dxi1, jacobian[3];
	int j, number_of_coordinate_components, number_of_points_in_line,
		return_code;
	FE_value_triple *xi;

	ENTER(FE_element_add_xi_points_1d_line_cell_random);
	if (element && (1 == get_FE_element_dimension(element)) &&
		number_of_xi_points && ((xi_points && number_of_xi_points_allocated) ||
			(!(xi_points) && !(number_of_xi_points_allocated))))
	{
		return_code = 1;
		centre_xi1 = (FE_value)xi_centre;
		dxi1 = (FE_value)delta_xi;
		if (CMZN_ELEMENT_POINT_SAMPLING_MODE_CELL_POISSON == sampling_mode)
		{
			if (coordinate_field && Computed_field_has_up_to_3_numerical_components(
				coordinate_field,	(void *)NULL) &&
				((1 == (number_of_coordinate_components =
					cmzn_field_get_number_of_components(coordinate_field))) ||
					(2 == number_of_coordinate_components) ||
					(3 == number_of_coordinate_components)) &&
				(CMZN_OK == field_cache->setMeshLocation(element, &xi_centre)) &&
				(CMZN_OK == cmzn_field_evaluate_real_with_derivatives(coordinate_field,
					field_cache, number_of_coordinate_components, coordinates, /*number_of_derivatives*/1, jacobian)) &&
				(CMZN_OK == cmzn_field_evaluate_real(density_field, field_cache, 1, &density)))
			{
				/* calculate the volume from the jacobian and dxi */
				a[0] = (jacobian[0]);
				if (1 < number_of_coordinate_components)
				{
					a[1] = (jacobian[1]);
					if (2 < number_of_coordinate_components)
					{
						a[2] = (jacobian[2]);
					}
					else
					{
						a[2] = 0.0;
					}
				}
				else
				{
					a[1] = 0.0;
					a[2] = 0.0;
				}
				length = dxi1*norm3(a);
				expected_number = length*density;
				if (0.0 <= expected_number)
				{
					number_of_points_in_line = sample_Poisson_distribution(expected_number);
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"FE_element_add_xi_points_1d_line_cell_random.  "
						"Negative number of points expected in volume");
					return_code = 0;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"FE_element_add_xi_points_1d_line_cell_random.  "
					"Could not evaluate density and/or length");
				return_code = 0;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_element_add_xi_points_1d_line_cell_random.  "
				"Invalid sampling_mode");
			return_code = 0;
		}
		if (return_code)
		{
			if (xi_points)
			{
				/* check if we need to reallocate the xi_points array */
				if ((*number_of_xi_points + number_of_points_in_line) >
					(*number_of_xi_points_allocated))
				{
					if (REALLOCATE(xi, *xi_points, FE_value_triple,
						*number_of_xi_points + number_of_points_in_line +
						XI_POINTS_REALLOCATE_SIZE))
					{
						*xi_points = xi;
						*number_of_xi_points_allocated = *number_of_xi_points +
							number_of_points_in_line + XI_POINTS_REALLOCATE_SIZE;
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"FE_element_add_xi_points_1d_line_cell_random.  "
							"Could not reallocate xi_points");
						return_code = 0;
					}
				}
				if (return_code)
				{
					xi = *xi_points + (*number_of_xi_points);
					for (j = 0; j < number_of_points_in_line; j++)
					{
						(*xi)[0] = centre_xi1 + dxi1*(CMGUI_RANDOM(FE_value) - 0.5);
						(*xi)[1] = 0.0;
						(*xi)[2] = 0.0;
#if defined (DEBUG_CODE)
						/*???debug*/
						printf("FE_element_add_xi_points_1d_line_cell_random.  "
							"xi(%d) = %6.3f %6.3f %6.3f\n", *number_of_xi_points,
							(*xi)[0], (*xi)[1], (*xi)[2]);
#endif /* defined (DEBUG_CODE) */
						xi++;
						(*number_of_xi_points)++;
					}
				}
			}
			else
			{
				*number_of_xi_points += number_of_points_in_line;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_element_add_xi_points_1d_line_cell_random.  "
				"Could not evaluate density and/or volume");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_element_add_xi_points_1d_line_cell_random.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_element_add_xi_points_1d_line_cell_random */

static int FE_element_add_xi_points_2d_square_cell_random(
	struct FE_element	*element, cmzn_element_point_sampling_mode sampling_mode,
	enum FE_element_shape_category element_shape_category,
	FE_value *centre_xi, FE_value *dxi, cmzn_fieldcache_id field_cache,
	struct Computed_field *coordinate_field,
	struct Computed_field *density_field, int *number_of_xi_points,
	FE_value_triple **xi_points, int *number_of_xi_points_allocated,
	FE_value *xi_offset)
/*******************************************************************************
LAST MODIFIED : 21 March 2003

DESCRIPTION :
Adds to the <number_of_xi_points> the number of points to be added according to
the <sampling_mode>; see FE_element_get_xi_points_cell_random.
If <xi_points> and current <number_of_xi_points_allocated> are supplied, the
array is enlarged if necessary and the new points added at random locations.
#Define XI_POINTS_REALLOCATE_SIZE provides a minimum step size for enlarging the
<xi_points> array, to prevent too many allocations.
==============================================================================*/
{
	double a[3], area, b[3], c[3], expected_number;
	FE_value coordinates[3], density;
	FE_value centre_xi1, centre_xi2, dxi1, dxi2, jacobian[6];
	int j, number_of_coordinate_components, number_of_points_in_square,
		return_code;
	FE_value_triple *xi;

	ENTER(FE_element_add_xi_points_2d_square_cell_random);
	if (element && (2 == get_FE_element_dimension(element)) && centre_xi && dxi &&
		number_of_xi_points && ((xi_points && number_of_xi_points_allocated) ||
			(!(xi_points) && !(number_of_xi_points_allocated))))
	{
		return_code = 1;
		centre_xi1 = (FE_value)centre_xi[0];
		centre_xi2 = (FE_value)centre_xi[1];
		dxi1 = (FE_value)dxi[0];
		dxi2 = (FE_value)dxi[1];
		if (CMZN_ELEMENT_POINT_SAMPLING_MODE_CELL_POISSON == sampling_mode)
		{
			if (coordinate_field && Computed_field_has_up_to_3_numerical_components(
				coordinate_field,	(void *)NULL) &&
				((2 == (number_of_coordinate_components =
					cmzn_field_get_number_of_components(coordinate_field))) ||
					(3 == number_of_coordinate_components)) &&
				(CMZN_OK == field_cache->setMeshLocation(element, centre_xi)) &&
				(CMZN_OK == cmzn_field_evaluate_real_with_derivatives(coordinate_field,
					field_cache, number_of_coordinate_components, coordinates, /*number_of_derivatives*/2, jacobian)) &&
				(CMZN_OK == cmzn_field_evaluate_real(density_field, field_cache, 1, &density)))
			{
				/* calculate the volume from the jacobian and dxi */
				a[0] = (double)(jacobian[0]);
				b[0] = (double)(jacobian[1]);
				a[1] = (double)(jacobian[2]);
				b[1] = (double)(jacobian[3]);
				if (3 == number_of_coordinate_components)
				{
					a[2] = (double)(jacobian[4]);
					b[2] = (double)(jacobian[5]);
				}
				else
				{
					a[2] = 0.0;
					b[2] = 0.0;
				}
				cross_product3(a, b, c);
				area = (double)dxi1*(double)dxi2*norm3(c);
				expected_number = area*(double)density;
				if (0.0 <= expected_number)
				{
					number_of_points_in_square = sample_Poisson_distribution(expected_number);
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"FE_element_add_xi_points_2d_square_cell_random.  "
						"Negative number of points expected in volume");
					return_code = 0;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"FE_element_add_xi_points_2d_square_cell_random.  "
					"Could not evaluate density and/or area");
				return_code = 0;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_element_add_xi_points_2d_square_cell_random.  "
				"Invalid sampling_mode");
			return_code = 0;
		}
		if (return_code)
		{
			if (xi_points)
			{
				/* check if we need to reallocate the xi_points array */
				if ((*number_of_xi_points + number_of_points_in_square) >
					(*number_of_xi_points_allocated))
				{
					if (REALLOCATE(xi, *xi_points, FE_value_triple,
						*number_of_xi_points + number_of_points_in_square +
						XI_POINTS_REALLOCATE_SIZE))
					{
						*xi_points = xi;
						*number_of_xi_points_allocated = *number_of_xi_points +
							number_of_points_in_square + XI_POINTS_REALLOCATE_SIZE;
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"FE_element_add_xi_points_2d_square_cell_random.  "
							"Could not reallocate xi_points");
						return_code = 0;
					}
				}
				if (return_code)
				{
					switch (element_shape_category)
					{
						case ELEMENT_CATEGORY_2D_SQUARE:
						{
							xi = *xi_points + (*number_of_xi_points);
							for (j = 0; j < number_of_points_in_square; j++)
							{
								(*xi)[0] = centre_xi1 + dxi1*(CMGUI_RANDOM(FE_value) - (FE_value)xi_offset[0]);
								(*xi)[1] = centre_xi2 + dxi2*(CMGUI_RANDOM(FE_value) - (FE_value)xi_offset[1]);
								(*xi)[2] = 0.0;
#if defined (DEBUG_CODE)
								/*???debug*/
								printf("FE_element_add_xi_points_2d_square_cell_random.  "
									"xi(%d) = %6.3f %6.3f %6.3f\n", *number_of_xi_points,
									(*xi)[0], (*xi)[1], (*xi)[2]);
#endif /* defined (DEBUG_CODE) */
								xi++;
								(*number_of_xi_points)++;
							}
						} break;
						case ELEMENT_CATEGORY_2D_TRIANGLE:
						{
							xi = *xi_points + (*number_of_xi_points);
							FE_value xi0, xi1;
							for (j = 0; j < number_of_points_in_square; j++)
							{
								xi0 = centre_xi1 + dxi1*(CMGUI_RANDOM(FE_value) - (FE_value)xi_offset[0]);
								xi1 = centre_xi2 + dxi2*(CMGUI_RANDOM(FE_value) - (FE_value)xi_offset[1]);
								if ((xi0 + xi1) < 1 )
								{
									(*xi)[0] = xi0;
									(*xi)[1] = xi1;
									(*xi)[2] = 0.0;
									xi++;
									(*number_of_xi_points)++;
								}
#if defined (DEBUG_CODE)
								/*???debug*/
								printf("FE_element_add_xi_points_2d_cube_cell_random.  "
								"xi(%d) = %6.3f %6.3f %6.3f\n", *number_of_xi_points,
									(*xi)[0], (*xi)[1], (*xi)[2]);
#endif /* defined (DEBUG_CODE) */
							}
						} break;
						default:
						{
							display_message(ERROR_MESSAGE,
								"FE_element_add_xi_points_2d_square_cell_random.  "
								"Element shape not supported");
							return_code = 0;
						} break;
					}
				}
			}
			else
			{
				*number_of_xi_points += number_of_points_in_square;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_element_add_xi_points_2d_square_cell_random.  "
				"Could not evaluate density and/or volume");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_element_add_xi_points_2d_square_cell_random.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_element_add_xi_points_2d_square_cell_random */

static int FE_element_add_xi_points_3d_cube_cell_random(
	struct FE_element	*element,
	cmzn_element_point_sampling_mode sampling_mode,
	enum FE_element_shape_category element_shape_category,
	FE_value *centre_xi, FE_value *dxi, cmzn_fieldcache_id field_cache,
	struct Computed_field *coordinate_field,
	struct Computed_field *density_field, int *number_of_xi_points,
	FE_value_triple **xi_points, int *number_of_xi_points_allocated,
	FE_value *xi_offset)
/*******************************************************************************
LAST MODIFIED : 21 March 2003

DESCRIPTION :
Adds to the <number_of_xi_points> the number of points to be added according to
the <sampling_mode>; see FE_element_get_xi_points_cell_random.
If <xi_points> and current <number_of_xi_points_allocated> are supplied, the
array is enlarged if necessary and the new points added at random locations.
#Define XI_POINTS_REALLOCATE_SIZE provides a minimum step size for enlarging the
<xi_points> array, to prevent too many allocations.
==============================================================================*/
{
	ZnReal a[3], b[3], c[3], expected_number, volume;
	FE_value coordinates[3], density;
	FE_value centre_xi1, centre_xi2, centre_xi3, dxi1, dxi2, dxi3, jacobian[9];
	int j, number_of_points_in_cube, return_code;
	FE_value_triple *xi;

	ENTER(FE_element_add_xi_points_3d_cube_cell_random);
	if (element && (3 == get_FE_element_dimension(element)) && centre_xi && dxi &&
		number_of_xi_points && ((xi_points && number_of_xi_points_allocated) ||
			(!(xi_points) && !(number_of_xi_points_allocated))))
	{
		return_code = 1;
		centre_xi1 = centre_xi[0];
		centre_xi2 = centre_xi[1];
		centre_xi3 = centre_xi[2];
		dxi1 = dxi[0];
		dxi2 = dxi[1];
		dxi3 = dxi[2];
		if (CMZN_ELEMENT_POINT_SAMPLING_MODE_CELL_POISSON == sampling_mode)
		{
			if (coordinate_field && Computed_field_has_up_to_3_numerical_components(
				coordinate_field,	(void *)NULL) &&
				(3 == cmzn_field_get_number_of_components(coordinate_field)) &&
				(CMZN_OK == field_cache->setMeshLocation(element, centre_xi)) &&
				(CMZN_OK == cmzn_field_evaluate_real_with_derivatives(coordinate_field,
					field_cache, 3, coordinates, /*number_of_derivatives*/3, jacobian)) &&
				(CMZN_OK == cmzn_field_evaluate_real(density_field, field_cache, 1, &density)))
			{
				/* calculate the volume from the jacobian and dxi */
				a[0] = (jacobian[0]);
				a[1] = (jacobian[3]);
				a[2] = (jacobian[6]);
				b[0] = (jacobian[1]);
				b[1] = (double)(jacobian[4]);
				b[2] = (double)(jacobian[7]);
				c[0] = (double)(jacobian[2]);
				c[1] = (double)(jacobian[5]);
				c[2] = (double)(jacobian[8]);
				volume = (double)dxi1*(double)dxi2*(double)dxi3*
					scalar_triple_product3(a, b, c);
				expected_number = volume*(double)density;
				if (0.0 > expected_number)
				{
					expected_number = -expected_number;
				}
				number_of_points_in_cube = sample_Poisson_distribution(expected_number);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"FE_element_add_xi_points_3d_cube_cell_random.  "
					"Could not evaluate density and/or volume");
				return_code = 0;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_element_add_xi_points_3d_cube_cell_random.  "
				"Invalid sampling_mode");
			return_code = 0;
		}
		if (return_code)
		{
			if (xi_points)
			{
				/* check if we need to reallocate the xi_points array */
				if ((*number_of_xi_points + number_of_points_in_cube) >
					(*number_of_xi_points_allocated))
				{
					if (REALLOCATE(xi, *xi_points, FE_value_triple,
						*number_of_xi_points + number_of_points_in_cube +
						XI_POINTS_REALLOCATE_SIZE))
					{
						*xi_points = xi;
						*number_of_xi_points_allocated = *number_of_xi_points +
							number_of_points_in_cube + XI_POINTS_REALLOCATE_SIZE;
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"FE_element_add_xi_points_3d_cube_cell_random.  "
							"Could not reallocate xi_points");
						return_code = 0;
					}
				}
				if (return_code)
				{
					switch (element_shape_category)
					{
						case ELEMENT_CATEGORY_3D_CUBE:
						{
							xi = *xi_points + (*number_of_xi_points);
							for (j = 0; j < number_of_points_in_cube; j++)
							{
								(*xi)[0] = centre_xi1 + dxi1*(CMGUI_RANDOM(FE_value) - (FE_value)xi_offset[0]);
								(*xi)[1] = centre_xi2 + dxi2*(CMGUI_RANDOM(FE_value) - (FE_value)xi_offset[1]);
								(*xi)[2] = centre_xi3 + dxi3*(CMGUI_RANDOM(FE_value) - (FE_value)xi_offset[2]);
#if defined (DEBUG_CODE)
								/*???debug*/
								printf("FE_element_add_xi_points_3d_cube_cell_random.  "
									"xi(%d) = %6.3f %6.3f %6.3f\n", *number_of_xi_points,
									(*xi)[0], (*xi)[1], (*xi)[2]);
#endif /* defined (DEBUG_CODE) */
								xi++;
								(*number_of_xi_points)++;
							}

						} break;
						case ELEMENT_CATEGORY_3D_TETRAHEDRON:
						{
							xi = *xi_points + (*number_of_xi_points);
							FE_value xi0, xi1, xi2;
							for (j = 0; j < number_of_points_in_cube; j++)
							{
								xi0 = centre_xi1 + dxi1*(CMGUI_RANDOM(FE_value) - xi_offset[0]);
								xi1 = centre_xi2 + dxi2*(CMGUI_RANDOM(FE_value) - xi_offset[1]);
								xi2 = centre_xi3 + dxi2*(CMGUI_RANDOM(FE_value) - xi_offset[2]);
								if ((xi0 + xi1 + xi2) < 1 )
								{
									(*xi)[0] = xi0;
									(*xi)[1] = xi1;
									(*xi)[2] = xi2;
									xi++;
									(*number_of_xi_points)++;
								}
#if defined (DEBUG_CODE)
								/*???debug*/
								printf("FE_element_add_xi_points_3d_cube_cell_random.  "
									"xi(%d) = %6.3f %6.3f %6.3f\n", *number_of_xi_points,
									xi0, xi1, xi2);
#endif /* defined (DEBUG_CODE) */
							}
						} break;
						case ELEMENT_CATEGORY_3D_TRIANGLE_LINE:
						{
							xi = *xi_points + (*number_of_xi_points);
							FE_value r1, r2, r3;
							FE_value sign = ((xi_offset[0] < 0.0) || (xi_offset[1] < 0.0)) ? -1 : 1;
							FE_value base_xi1 = centre_xi1 - dxi1*xi_offset[0];
							FE_value base_xi2 = centre_xi2 - dxi2*xi_offset[1];
							FE_value base_xi3 = centre_xi3 - dxi3*xi_offset[2];
							FE_value signed_dxi1 = sign*dxi1;
							FE_value signed_dxi2 = sign*dxi2;
							FE_value signed_dxi3 = sign*dxi3;
							if (xi_offset[0] == (FE_value)0.5)
							{
								signed_dxi1 = dxi1;
							}
							else if (xi_offset[1] == (FE_value)0.5)
							{
								signed_dxi2 = dxi2;
							}
							else // if (xi_offset[2] == (FE_value)0.5)
							{
								signed_dxi3 = dxi3;
							}
							for (j = 0; j < number_of_points_in_cube; j++)
							{
								r1 = CMGUI_RANDOM(FE_value);
								r2 = CMGUI_RANDOM(FE_value);
								r3 = CMGUI_RANDOM(FE_value);
								if (((xi_offset[0] == (FE_value)0.5) && (r2 + r3 < 1.0)) ||
									((xi_offset[1] == (FE_value)0.5) && (r3 + r1 < 1.0)) ||
									((xi_offset[2] == (FE_value)0.5) && (r1 + r2 < 1.0)))
								{
									(*xi)[0] = base_xi1 + signed_dxi1*r1;
									(*xi)[1] = base_xi2 + signed_dxi2*r2;
									(*xi)[2] = base_xi3 + signed_dxi3*r3;
									xi++;
									(*number_of_xi_points)++;
								}
#if defined (DEBUG_CODE)
								/*???debug*/
								printf("FE_element_add_xi_points_3d_cube_cell_random.  "
									"xi(%d) = %6.3f %6.3f %6.3f\n", *number_of_xi_points,
									(*xi)[0], (*xi)[1], (*xi)[2]);
#endif /* defined (DEBUG_CODE) */
							}
						} break;
						default:
						{
							display_message(ERROR_MESSAGE,
								"FE_element_add_xi_points_3d_cube_cell_random.  "
								"Element shape not supported");
							return_code = 0;
						} break;
					}
				}
			}
			else
			{
				*number_of_xi_points += number_of_points_in_cube;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_element_add_xi_points_3d_cube_cell_random.  "
				"Could not evaluate density and/or volume");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_element_add_xi_points_3d_cube_cell_random.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_element_add_xi_points_3d_cube_cell_random */

static int FE_element_get_xi_points_cell_random(struct FE_element *element,
	cmzn_element_point_sampling_mode sampling_mode, int *number_in_xi,
	cmzn_fieldcache_id field_cache, struct Computed_field *coordinate_field,
	struct Computed_field *density_field, int *number_of_xi_points_address,
	FE_value_triple **xi_points_address)
/*******************************************************************************
LAST MODIFIED : 3 December 2001

DESCRIPTION :
Calculates the <number_of_xi_points> to be randomly located in uniform cells
across the <element_shape> according to <number_in_xi>. The number of points
placed in each cell depends on the <sampling_mode> which can be one of:
CELL_POISSON = as for above but the actual number is sampled
  from a Poisson distribution with mean given by the expected number. While this
	adds noise to the density function, it overcomes the problem that small cells
	with low densities can never be represented by xi points with CELL_DENSITY.
The density and the length/area/volume of the cell are evaluated from the
<density_field> and <coordinate_field> at the cell centre, respectively.
User should call CMGUI_SEED_RANDOM with the element number before calling this
if they wish to get consistent random layouts in a given element over time.
If <xi_points_address> is supplied an array containing the xi locations of these
points is allocated and put in this address. Xi positions are always returned as
triples with remaining xi coordinates 0 for 1-D and 2-D cases.
Note the actual number and layout is dependent on the <element_shape>; see
comments for simplex and polygons shapes for more details.
@param field_cache  cmzn_fieldcache for evaluating coordinate and density
fields, required for DENSITY and POISSON modes.
==============================================================================*/
{
	enum FE_element_shape_category element_shape_category;
	FE_value centre_xi[3], delta_xi, dxi[3], xi_centre, xi_offset[3];
	int element_dimension, i, j, k, line_direction, linked_xi_directions[2],
		number_of_polygon_sides, number_of_xi_points, number_of_xi_points_allocated,
		return_code;
	FE_value_triple *xi_points;

	ENTER(FE_element_get_xi_points_cell_random);
	FE_element_shape *element_shape = get_FE_element_shape(element);
	if ((element_shape) &&
		(0 < (element_dimension = get_FE_element_shape_dimension(element_shape))) &&
		number_in_xi &&
		(CMZN_ELEMENT_POINT_SAMPLING_MODE_CELL_POISSON == sampling_mode) &&
			coordinate_field && Computed_field_has_up_to_3_numerical_components(
				coordinate_field,	(void *)NULL) &&
			(cmzn_field_get_number_of_components(coordinate_field) >=
				element_dimension) && density_field &&
			Computed_field_is_scalar(density_field, (void *)NULL) &&
			field_cache && number_of_xi_points_address)
	{
		return_code = 1;
		number_of_xi_points = 0;
		/* check the number_in_xi */
		for (i = 0; (i < element_dimension) && return_code ; i++)
		{
			if (1 > number_in_xi[i])
			{
				display_message(ERROR_MESSAGE,
					"FE_element_get_xi_points_cell_random.  "
					"Non-positive number_in_xi");
				return_code = 0;
			}
		}
		/* extract useful information about the element_shape */
		if (!categorize_FE_element_shape(element_shape, &element_shape_category,
			&number_of_polygon_sides, linked_xi_directions, &line_direction))
		{
			display_message(ERROR_MESSAGE,
				"FE_element_get_xi_points_cell_random.  "
				"Could not categorize element_shape");
			return_code = 0;
		}
		if (return_code)
		{
			number_of_xi_points = 0;
			if (xi_points_address)
			{
				*xi_points_address = (FE_value_triple *)NULL;
				number_of_xi_points_allocated = 0;
			}
			/* need to compute all the points at all times, even if just enquiring
				 about the number of points */
			switch (element_shape_category)
			{
				case ELEMENT_CATEGORY_1D_LINE:
				{
					delta_xi = 1.0 / (FE_value)number_in_xi[0];
					for (i = 0; i < number_in_xi[0]; i++)
					{
						xi_centre = ((FE_value)i + 0.5)/(FE_value)number_in_xi[0];
						return_code = FE_element_add_xi_points_1d_line_cell_random(
							element, sampling_mode, xi_centre, delta_xi,
							field_cache, coordinate_field, density_field,
							&number_of_xi_points, xi_points_address,
							&number_of_xi_points_allocated);
					}
				} break;
				case ELEMENT_CATEGORY_2D_SQUARE:
				{
					dxi[0] = 1.0 / (FE_value)number_in_xi[0];
					dxi[1] = 1.0 / (FE_value)number_in_xi[1];
					xi_offset[0] = (FE_value)0.5;
					xi_offset[1] = (FE_value)0.5;
					xi_offset[2] = (FE_value)0;
					for (j = 0; j < number_in_xi[1]; j++)
					{
						centre_xi[1] = ((FE_value)j + 0.5)/(FE_value)number_in_xi[1];
						for (i = 0; i < number_in_xi[0]; i++)
						{
							centre_xi[0] = ((FE_value)i + 0.5)/(FE_value)number_in_xi[0];
							return_code = FE_element_add_xi_points_2d_square_cell_random(
								element, sampling_mode, element_shape_category,
								centre_xi, dxi, field_cache, coordinate_field, density_field,
								&number_of_xi_points, xi_points_address,
								&number_of_xi_points_allocated, xi_offset);
						}
					}
				} break;
				case ELEMENT_CATEGORY_2D_TRIANGLE:
				{
					int number_in_xi_simplex = number_in_xi[0];
					if (number_in_xi[1] > number_in_xi_simplex)
					{
						number_in_xi_simplex = number_in_xi[1];
					}
					dxi[0] = 1.0 / (FE_value)number_in_xi_simplex;
					dxi[1] = 1.0 / (FE_value)number_in_xi_simplex;
					dxi[2] = 1.0 / (FE_value)number_in_xi_simplex;
					int number_in_xi0 = 0;
					xi_offset[0] = (FE_value)(1.0/3.0);
					xi_offset[1] = (FE_value)(1.0/3.0);
					xi_offset[2] = (FE_value)0;
					for (j = 0; j < number_in_xi_simplex; j++)
					{
						centre_xi[1] = ((FE_value)j + (1.0/3.0))/(FE_value)number_in_xi_simplex;
						number_in_xi0 = number_in_xi_simplex - j;
						for (i = 0; i < number_in_xi0; i++)
						{
							centre_xi[0] = ((FE_value)i + (1.0/3.0))/(FE_value)number_in_xi_simplex;
							centre_xi[2] = 0.0;
							return_code = FE_element_add_xi_points_2d_square_cell_random(
								element, sampling_mode, element_shape_category,
								centre_xi, dxi, field_cache, coordinate_field, density_field,
								&number_of_xi_points, xi_points_address,
								&number_of_xi_points_allocated, xi_offset);
						}
					}
				} break;
				case ELEMENT_CATEGORY_2D_POLYGON:
				{
					display_message(ERROR_MESSAGE,
						"FE_element_get_xi_points_cell_random.  "
						"Not implemented for ELEMENT_CATEGORY_2D_POLYGON");
					return_code = 0;
				} break;
				case ELEMENT_CATEGORY_3D_CUBE:
				{
					xi_offset[0] = (FE_value)0.5;
					xi_offset[1] = (FE_value)0.5;
					xi_offset[2] = (FE_value)0.5;
					dxi[0] = 1.0 / (FE_value)number_in_xi[0];
					dxi[1] = 1.0 / (FE_value)number_in_xi[1];
					dxi[2] = 1.0 / (FE_value)number_in_xi[2];
					for (k = 0; (k < number_in_xi[2]) && return_code; k++)
					{
						centre_xi[2] = ((FE_value)k + 0.5)/(FE_value)number_in_xi[2];
						for (j = 0; (j < number_in_xi[1]) && return_code; j++)
						{
							centre_xi[1] = ((FE_value)j + 0.5)/(FE_value)number_in_xi[1];
							for (i = 0; (i <number_in_xi[0]) && return_code; i++)
							{
								centre_xi[0] = ((FE_value)i + 0.5)/(FE_value)number_in_xi[0];
								return_code = FE_element_add_xi_points_3d_cube_cell_random(
									element, sampling_mode, element_shape_category,
									centre_xi, dxi, field_cache, coordinate_field, density_field,
									&number_of_xi_points, xi_points_address,
									&number_of_xi_points_allocated, xi_offset);
							}
						}
					}
				} break;
				case ELEMENT_CATEGORY_3D_TETRAHEDRON:
				{
					int number_in_xi_simplex = number_in_xi[0];
					if (number_in_xi[1] > number_in_xi_simplex)
					{
						number_in_xi_simplex = number_in_xi[1];
					}
					if (number_in_xi[2] > number_in_xi_simplex)
					{
						number_in_xi_simplex = number_in_xi[2];
					}
					dxi[0] = 1.0 / (FE_value)number_in_xi_simplex;
					dxi[1] = 1.0 / (FE_value)number_in_xi_simplex;
					dxi[2] = 1.0 / (FE_value)number_in_xi_simplex;
					int	number_in_xi1 = 0, number_in_xi0 = 0;
					xi_offset[0] = (FE_value)0.25;
					xi_offset[1] = (FE_value)0.25;
					xi_offset[2] = (FE_value)0.25;
					for (k = 0; k < number_in_xi_simplex; k++)
					{
						centre_xi[2] = ((FE_value)k + 0.25)/(FE_value)number_in_xi_simplex;
						number_in_xi1 = number_in_xi_simplex - k;
						for (j = 0; j <= number_in_xi1; j++)
						{
							centre_xi[1] = ((FE_value)j + 0.25)/(FE_value)number_in_xi_simplex;
							number_in_xi0 = number_in_xi1 - j;
							for (i = 0; i < number_in_xi0; i++)
							{
								centre_xi[0] = ((FE_value)i + 0.25)/(FE_value)number_in_xi_simplex;
								return_code = FE_element_add_xi_points_3d_cube_cell_random(
									element, sampling_mode, element_shape_category,
									centre_xi, dxi, field_cache, coordinate_field, density_field,
									&number_of_xi_points, xi_points_address,
									&number_of_xi_points_allocated, xi_offset);
							}
						}
					}
				} break;
				case ELEMENT_CATEGORY_3D_TRIANGLE_LINE:
				{
					int number_in_xi_simplex = number_in_xi[linked_xi_directions[0]];
					if (number_in_xi[linked_xi_directions[1]] > number_in_xi_simplex)
					{
						number_in_xi_simplex = number_in_xi[linked_xi_directions[1]];
					}

					double xi_j, xi_k;
					int number_in_xi0 = 0;
					dxi[0] = 1.0 / (FE_value)number_in_xi_simplex;
					dxi[1] = 1.0 / (FE_value)number_in_xi_simplex;
					dxi[2] = 1.0 / (FE_value)number_in_xi_simplex;
					dxi[line_direction] = 1.0 / (FE_value)number_in_xi[line_direction];
					xi_offset[line_direction] = (FE_value)0.5;
					for (k = 0; k < number_in_xi[line_direction]; k++)
					{
						xi_offset[linked_xi_directions[0]] = (FE_value)(1.0/3.0);
						xi_offset[linked_xi_directions[1]] = (FE_value)(1.0/3.0);
						xi_k = ((FE_value)k + 0.5)/(FE_value)number_in_xi[line_direction];
						for (j = 0; j < number_in_xi_simplex; j++)
						{
							xi_j = ((double)j + (1.0/3.0))/(double)number_in_xi_simplex;
							number_in_xi0 = number_in_xi_simplex - j;
							for (i = 0; i < number_in_xi0; i++)
							{
								centre_xi[linked_xi_directions[0]] =
									((double)i + (1.0/3.0))/(double)number_in_xi_simplex;
								centre_xi[linked_xi_directions[1]] = xi_j;
								centre_xi[line_direction] = xi_k;
								return_code = FE_element_add_xi_points_3d_cube_cell_random(
									element, sampling_mode, element_shape_category,
									centre_xi, dxi, field_cache, coordinate_field, density_field,
									&number_of_xi_points, xi_points_address,
									&number_of_xi_points_allocated, xi_offset);
							}
						}
						xi_offset[linked_xi_directions[0]] = (FE_value)(-1.0/3.0);
						xi_offset[linked_xi_directions[1]] = (FE_value)(-1.0/3.0);
						for (j = 1; j < number_in_xi_simplex; j++)
						{
							xi_j = ((double)j - (1.0/3.0))/(double)number_in_xi_simplex;
							number_in_xi0 = number_in_xi_simplex - j + 1;
							for (i = 1; i < number_in_xi0; i++)
							{
								centre_xi[linked_xi_directions[0]] =
									((double)i - (1.0/3.0))/(double)number_in_xi_simplex;
								centre_xi[linked_xi_directions[1]] = xi_j;
								centre_xi[line_direction] = xi_k;
								return_code = FE_element_add_xi_points_3d_cube_cell_random(
									element, sampling_mode, element_shape_category,
									centre_xi, dxi, field_cache, coordinate_field, density_field,
									&number_of_xi_points, xi_points_address,
									&number_of_xi_points_allocated, xi_offset);
							}
						}
					}
				} break;
				case ELEMENT_CATEGORY_3D_POLYGON_LINE:
				{
					display_message(ERROR_MESSAGE,
						"FE_element_get_xi_points_cell_random.  "
						"Not implemented for ELEMENT_CATEGORY_3D_POLYGON_LINE");
					return_code = 0;
				} break;
				default:
				{
					display_message(ERROR_MESSAGE,
						"FE_element_get_xi_points_cell_random.  "
						"Unknown element shape");
					return_code = 0;
				} break;
			}
			if (return_code)
			{
#if defined (DEBUG_CODE)
				/*???debug*/
				printf("FE_element_get_xi_points_cell_random.  "
					"Used %d out of %d points allocated\n",number_of_xi_points,number_of_xi_points_allocated);
#endif /* defined (DEBUG_CODE) */
				*number_of_xi_points_address = number_of_xi_points;
				if (xi_points_address)
				{
					/* free over-allocated space in the xi_points array */
					if (number_of_xi_points < number_of_xi_points_allocated)
					{
						if (REALLOCATE(xi_points, *xi_points_address, FE_value_triple,
								number_of_xi_points))
						{
							*xi_points_address = xi_points;
						}
					}
				}
			}
			else
			{
				if (xi_points_address && *xi_points_address)
				{
					DEALLOCATE(*xi_points_address);
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_element_get_xi_points_cell_random.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_element_get_xi_points_cell_random */

int FE_element_get_xi_points(struct FE_element *element,
	cmzn_element_point_sampling_mode sampling_mode,
	int *number_in_xi, FE_value_triple exact_xi, cmzn_fieldcache_id field_cache,
	struct Computed_field *coordinate_field, struct Computed_field *density_field,
	int *number_of_xi_points_address, FE_value_triple **xi_points_address)
{
	int return_code;
	FE_value_triple *xi_points;

	ENTER(FE_element_get_xi_points);
	FE_element_shape *element_shape = get_FE_element_shape(element);
	if ((element_shape) && number_in_xi && number_of_xi_points_address)
	{
		return_code = 1;
		switch (sampling_mode)
		{
			case CMZN_ELEMENT_POINT_SAMPLING_MODE_CELL_CENTRES:
			{
				return_code = FE_element_shape_get_xi_points_cell_centres(element_shape,
					number_in_xi, number_of_xi_points_address, xi_points_address);
			} break;
			case CMZN_ELEMENT_POINT_SAMPLING_MODE_CELL_CORNERS:
			{
				return_code = FE_element_shape_get_xi_points_cell_corners(element_shape,
					number_in_xi, number_of_xi_points_address, xi_points_address);
			} break;
			case CMZN_ELEMENT_POINT_SAMPLING_MODE_CELL_POISSON:
			{
				/* seed random number generator with the element number so "random"
					 layout is consistent for the same element */
				CMGUI_SEED_RANDOM(get_FE_element_identifier(element));
				return_code = FE_element_get_xi_points_cell_random(element,
					sampling_mode, number_in_xi, field_cache, coordinate_field, density_field,
					number_of_xi_points_address, xi_points_address);
			} break;
			case CMZN_ELEMENT_POINT_SAMPLING_MODE_SET_LOCATION:
			{
				if (exact_xi)
				{
					*number_of_xi_points_address = 1;
					if (xi_points_address)
					{
						if (ALLOCATE(xi_points, FE_value_triple, 1))
						{
							*xi_points_address = xi_points;
							(*xi_points)[0] = exact_xi[0];
							(*xi_points)[1] = exact_xi[1];
							(*xi_points)[2] = exact_xi[2];
						}
						else
						{
							display_message(ERROR_MESSAGE,"FE_element_get_xi_points.  "
								"Could not allocate xi_points for set location");
							return_code = 0;
						}
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"FE_element_get_xi_points.  Missing exact_xi");
					return_code = 0;
				}
			} break;
			case CMZN_ELEMENT_POINT_SAMPLING_MODE_GAUSSIAN_QUADRATURE:
			{
				// not efficient; should be cached between elements
				IntegrationPointsCache integrationCache(CMZN_ELEMENT_QUADRATURE_RULE_GAUSSIAN,
					cmzn_element_get_dimension(element), number_in_xi);
				double xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
				for (int j = 0; j < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++j)
					xi[j] = 0.0;
				double weight;
				IntegrationShapePoints *shapePoints = integrationCache.getPoints(element);
				if (shapePoints)
				{
					int numPoints = shapePoints->getNumPoints();
					if (ALLOCATE(xi_points, FE_value_triple, numPoints))
					{
						*number_of_xi_points_address = numPoints;
						*xi_points_address = xi_points;
						for (int p = 0; p < numPoints; ++p)
						{
							shapePoints->getPoint(p, xi, &weight);
							for (int j = 0; j < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++j)
								xi_points[p][j] = xi[j];
						}
					}
					else
					{
						display_message(ERROR_MESSAGE, "FE_element_get_xi_points.  "
							"Could not allocate xi_points for gaussian quadrature");
						return_code = 0;
					}
				}
				else
				{
					display_message(ERROR_MESSAGE, "FE_element_get_xi_points.  "
						"Gauss points cannot be determined for element shape");
					return_code = 0;
				}
			} break;
			case CMZN_ELEMENT_POINT_SAMPLING_MODE_INVALID:
			{
				display_message(ERROR_MESSAGE,
					"FE_element_get_xi_points.  Unknown sampling_mode");
				return_code = 0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_element_get_xi_points.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_element_get_xi_points */

int FE_element_convert_xi_points_cell_corners_to_top_level(
	struct FE_element *element, struct FE_element *top_level_element,
	int *top_level_number_in_xi, int number_of_xi_points, FE_value_triple *xi_points,
	int **top_level_xi_point_numbers_address)
/*******************************************************************************
LAST MODIFIED : 23 April 2001

DESCRIPTION :
If the element is a face or line of a cube or square top-level element, converts
the <number_of_xi_points> <xi_points> to be locations in the top-level element.
Also allocates the *<top_level_xi_point_numbers_address> to contain the
appropriate xi_point_numbers relative to the top-level element.
Notes:
1. The xi_points put into this function must have been calculated with the
CMZN_ELEMENT_POINT_SAMPLING_MODE_CELL_CORNERS more and the number_in_xi determined from the
relation from <element> to <top_level_element> and its <top_level_number_in_xi>.
2. Sets *<top_level_xi_point_numbers_address> to NULL if not ALLOCATED; hence
a return value here indicates that the xi_points have been converted.
==============================================================================*/
{
	enum FE_element_shape_category top_level_element_shape_category;
	FE_value element_to_top_level[9];
	int base_grid_offset, element_dimension, expected_number_of_xi_points,
		grid_offset_in_xi[MAXIMUM_ELEMENT_XI_DIMENSIONS], i,
		line_direction, linked_xi_directions[2], number_of_polygon_sides,
		number_in_xi[MAXIMUM_ELEMENT_XI_DIMENSIONS], number_in_xi1, return_code,
		top_level_element_dimension, *top_level_xi_point_numbers;
	struct FE_element *temp_element;

	ENTER(FE_element_convert_xi_points_cell_corners_to_top_level);
	if (top_level_xi_point_numbers_address)
	{
		*top_level_xi_point_numbers_address = (int *)NULL;
	}
	FE_element_shape *element_shape = get_FE_element_shape(element);
	FE_element_shape *top_level_element_shape = get_FE_element_shape(top_level_element);
	if ((element_shape) && (top_level_element_shape) &&
		top_level_number_in_xi && (0 < number_of_xi_points) &&
		xi_points && top_level_xi_point_numbers_address)
	{
		return_code = 1;
		element_dimension = get_FE_element_dimension(element);
		top_level_element_dimension = get_FE_element_dimension(top_level_element);
		/* extract useful information about the element_shape */
		if (!categorize_FE_element_shape(top_level_element_shape,
			&top_level_element_shape_category,
			&number_of_polygon_sides, linked_xi_directions, &line_direction))
		{
			display_message(ERROR_MESSAGE,
				"FE_element_convert_xi_points_cell_corners_to_top_level.  "
				"Could not categorize top_level_element_shape");
			return_code = 0;
		}
		/* check if descended from a line*line or line*line*line element */
		if (return_code &&
			(element_dimension < top_level_element_dimension) &&
			((ELEMENT_CATEGORY_2D_SQUARE == top_level_element_shape_category) ||
				(ELEMENT_CATEGORY_3D_CUBE == top_level_element_shape_category)))
		{
			if ((temp_element = FE_element_get_top_level_element_conversion(
				element, top_level_element,
				CMZN_ELEMENT_FACE_TYPE_ALL, element_to_top_level)) &&
				(temp_element == top_level_element) &&
				calculate_grid_field_offsets(element_dimension,
					top_level_element_dimension, top_level_number_in_xi,
					element_to_top_level, number_in_xi,
					&base_grid_offset, grid_offset_in_xi) &&
				/* check we have the appropriate number of points */
				FE_element_shape_get_xi_points_cell_corners(element_shape, number_in_xi,
					&expected_number_of_xi_points,
					/*xi_points_address*/(FE_value_triple **)NULL) &&
				(expected_number_of_xi_points == number_of_xi_points) &&
				ALLOCATE(top_level_xi_point_numbers, int, number_of_xi_points))
			{
				/* convert xi positions to be on top_level_element */
				convert_xi_points_from_element_to_parent(number_of_xi_points,
					xi_points, element_dimension, top_level_element_dimension,
					element_to_top_level);
				/* calculate the top_level_xi_point_numbers */
				if (1 == element_dimension)
				{
					for (i = 0; i < number_of_xi_points; i++)
					{
						top_level_xi_point_numbers[i] =
							base_grid_offset + grid_offset_in_xi[0]*i;
					}
				}
				else
				{
					number_in_xi1 = number_in_xi[0]+1;
					for (i = 0; i < number_of_xi_points; i++)
					{
						top_level_xi_point_numbers[i] = base_grid_offset +
							grid_offset_in_xi[0] * (i % number_in_xi1) +
							grid_offset_in_xi[1] * (i / number_in_xi1);
					}
				}
				*top_level_xi_point_numbers_address = top_level_xi_point_numbers;
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"FE_element_convert_xi_points_cell_corners_to_top_level.  "
					"Error converting to top-level");
				return_code = 0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_element_convert_xi_points_cell_corners_to_top_level.  "
			"Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_element_convert_xi_points_cell_corners_to_top_level */

int FE_element_get_numbered_xi_point(struct FE_element *element,
	cmzn_element_point_sampling_mode sampling_mode,
	int *number_in_xi, FE_value_triple exact_xi, cmzn_fieldcache_id field_cache,
	struct Computed_field *coordinate_field, struct Computed_field *density_field,
	int xi_point_number, FE_value *xi)
{
	enum FE_element_shape_category element_shape_category;
	int default_behaviour, i, j, k, line_direction, linked_xi_directions[2], m, n,
		number_of_polygon_sides, number_of_xi_points, return_code;
	FE_value_triple *xi_points;

	ENTER(FE_element_get_numbered_xi_point);
	FE_element_shape *element_shape = get_FE_element_shape(element);
	if ((element_shape) && number_in_xi && xi)
	{
		return_code = 1;
		/* extract useful information about the element_shape */
		if (!categorize_FE_element_shape(element_shape, &element_shape_category,
			&number_of_polygon_sides, linked_xi_directions, &line_direction))
		{
			display_message(ERROR_MESSAGE,
				"FE_element_get_numbered_xi_point.  "
				"Could not categorize element_shape");
			return_code = 0;
		}
		if (return_code)
		{
			default_behaviour = 1;
			/* for efficiency, handle some simple cases to avoid the slower default */
			switch (sampling_mode)
			{
				case CMZN_ELEMENT_POINT_SAMPLING_MODE_CELL_CENTRES:
				{
					switch (element_shape_category)
					{
						case ELEMENT_CATEGORY_1D_LINE:
						{
							number_of_xi_points = number_in_xi[0];
							if ((0 <= xi_point_number) &&
								(xi_point_number < number_of_xi_points))
							{
								xi[0] = ((FE_value)xi_point_number + 0.5)/(FE_value)number_in_xi[0];
								xi[1] = 0.0;
								xi[2] = 0.0;
								default_behaviour = 0;
							}
							else
							{
								display_message(ERROR_MESSAGE,
									"FE_element_get_numbered_xi_point.  "
									"xi_point_number must be from 0 to %d",
									number_of_xi_points - 1);
								return_code = 0;
							}
						} break;
						case ELEMENT_CATEGORY_2D_SQUARE:
						{
							number_of_xi_points = number_in_xi[0]*number_in_xi[1];
							if ((0 <= xi_point_number) &&
								(xi_point_number < number_of_xi_points))
							{
								j = (xi_point_number / number_in_xi[0]);
								i = (xi_point_number % number_in_xi[0]);
								xi[0] = ((FE_value)i + 0.5)/(FE_value)number_in_xi[0];
								xi[1] = ((FE_value)j + 0.5)/(FE_value)number_in_xi[1];
								xi[2] = 0.0;
								default_behaviour = 0;
							}
							else
							{
								display_message(ERROR_MESSAGE,
									"FE_element_get_numbered_xi_point.  "
									"xi_point_number must be from 0 to %d",
									number_of_xi_points - 1);
								return_code = 0;
							}
						} break;
						case ELEMENT_CATEGORY_3D_CUBE:
						{
							number_of_xi_points =
								number_in_xi[0]*number_in_xi[1]*number_in_xi[2];
							if ((0 <= xi_point_number) &&
								(xi_point_number < number_of_xi_points))
							{
								m = number_in_xi[0]*number_in_xi[1];
								k = xi_point_number / m;
								n = xi_point_number % m;
								j = n / number_in_xi[0];
								i = n % number_in_xi[0];
								xi[0] = ((FE_value)i + 0.5)/(FE_value)number_in_xi[0];
								xi[1] = ((FE_value)j + 0.5)/(FE_value)number_in_xi[1];
								xi[2] = ((FE_value)k + 0.5)/(FE_value)number_in_xi[2];
								default_behaviour = 0;
							}
							else
							{
								display_message(ERROR_MESSAGE,
									"FE_element_get_numbered_xi_point.  "
									"xi_point_number must be from 0 to %d",
									number_of_xi_points - 1);
								return_code = 0;
							}
						} break;
						default:
						{
							display_message(ERROR_MESSAGE,
								"FE_element_get_numbered_xi_point.  "
								"Element shape not supported");
							return_code = 0;
						} break;
					}
				} break;
				case CMZN_ELEMENT_POINT_SAMPLING_MODE_CELL_CORNERS:
				{
					switch (element_shape_category)
					{
						case ELEMENT_CATEGORY_1D_LINE:
						{
							number_of_xi_points = (number_in_xi[0] + 1);
							if ((0 <= xi_point_number) &&
								(xi_point_number < number_of_xi_points))
							{
								xi[0] = (FE_value)xi_point_number/(FE_value)number_in_xi[0];
								xi[1] = 0.0;
								xi[2] = 0.0;
								default_behaviour = 0;
							}
							else
							{
								display_message(ERROR_MESSAGE,
									"FE_element_get_numbered_xi_point.  "
									"xi_point_number must be from 0 to %d",
									number_of_xi_points - 1);
								return_code = 0;
							}
						} break;
						case ELEMENT_CATEGORY_2D_SQUARE:
						{
							number_of_xi_points = (number_in_xi[0] + 1)*(number_in_xi[1] + 1);
							if ((0 <= xi_point_number) &&
								(xi_point_number < number_of_xi_points))
							{
								j = (xi_point_number / (number_in_xi[0]+1));
								i = (xi_point_number % (number_in_xi[0]+1));
								xi[0] = (FE_value)i/(FE_value)number_in_xi[0];
								xi[1] = (FE_value)j/(FE_value)number_in_xi[1];
								xi[2] = 0.0;
								default_behaviour = 0;
							}
							else
							{
								display_message(ERROR_MESSAGE,
									"FE_element_get_numbered_xi_point.  "
									"xi_point_number must be from 0 to %d",
									number_of_xi_points - 1);
								return_code = 0;
							}
						} break;
						case ELEMENT_CATEGORY_3D_CUBE:
						{
							number_of_xi_points = (number_in_xi[0] + 1)*
								(number_in_xi[1] + 1)*(number_in_xi[2] + 1);
							if ((0 <= xi_point_number) &&
								(xi_point_number < number_of_xi_points))
							{
								m = (number_in_xi[0]+1)*(number_in_xi[1]+1);
								k = xi_point_number / m;
								n = xi_point_number % m;
								j = n / (number_in_xi[0]+1);
								i = n % (number_in_xi[0]+1);
								xi[0] = (FE_value)i/(FE_value)number_in_xi[0];
								xi[1] = (FE_value)j/(FE_value)number_in_xi[1];
								xi[2] = (FE_value)k/(FE_value)number_in_xi[2];
								default_behaviour = 0;
							}
							else
							{
								display_message(ERROR_MESSAGE,
									"FE_element_get_numbered_xi_point.  "
									"xi_point_number must be from 0 to %d",
									number_of_xi_points - 1);
								return_code = 0;
							}
						} break;
						default:
						{
							display_message(ERROR_MESSAGE,
								"FE_element_get_numbered_xi_point.  "
								"Element shape not supported");
							return_code = 0;
						} break;
					}
				} break;
				case CMZN_ELEMENT_POINT_SAMPLING_MODE_SET_LOCATION:
				{
					if (exact_xi)
					{
						if (0 == xi_point_number)
						{
							xi[0] = exact_xi[0];
							xi[1] = exact_xi[1];
							xi[2] = exact_xi[2];
							default_behaviour = 0;
						}
						else
						{
							display_message(ERROR_MESSAGE,
								"FE_element_get_numbered_xi_point.  "
								"xi_point_number must be 0 for exact_xi");
							return_code = 0;
						}
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"FE_element_get_numbered_xi_point.  Missing exact Xi");
						return_code = 0;
					}
				} break;
				default:
				{
					display_message(ERROR_MESSAGE,
						"FE_element_get_numbered_xi_point.  "
						"Discretization mode not supported");
					return_code = 0;
				} break;
			}
			if (return_code && default_behaviour)
			{
				if (FE_element_get_xi_points(element, sampling_mode,
					number_in_xi, exact_xi, field_cache, coordinate_field, density_field,
					&number_of_xi_points, &xi_points))
				{
					if ((0 <= xi_point_number) &&
						(xi_point_number < number_of_xi_points))
					{
						xi[0] = xi_points[xi_point_number][0];
						xi[1] = xi_points[xi_point_number][1];
						xi[2] = xi_points[xi_point_number][2];
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"FE_element_get_numbered_xi_point.  "
							"xi_point_number is out of range");
						return_code = 0;
					}
					DEALLOCATE(xi_points);
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"FE_element_get_numbered_xi_point.  Could not get xi_points");
					return_code = 0;
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_element_get_numbered_xi_point.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_element_get_numbered_xi_point */

int convert_xi_points_from_element_to_parent(int number_of_xi_points,
	FE_value_triple *xi_points,int element_dimension,int parent_dimension,
	FE_value *element_to_parent)
/*******************************************************************************
LAST MODIFIED : 17 September 1998

DESCRIPTION :
Converts the list of <number_of_xi_points> xi coordinates in the <xi_points>
array from the <element_dimension> (=number of xi coordinates) to the
<parent_dimension>, which must be greater. Only the first <element_dimension>
components of the <xi_points> will therefore have useful values - the remaining
of the 3 components should be zero.

Matrix <element_to_parent> is (parent_dimension) rows X (element_dimension+1)
columns in size, with the first column being the xi offset vector b, such that,
xi<parent> = b + A.xi<element>
while A is the remainder of the matrix. (Appropriate matrices are given by the
face_to_element member of struct FE_element_shape, and by function
FE_element_get_top_level_element_conversion.)

???RC  This may be redundant now that Computed_fields are automatically using
parent elements where necessary.
==============================================================================*/
{
	FE_value final_xi_value;
	int i,j,k,return_code,rowsize;
	FE_value_triple *xi,xi_value;

	ENTER(convert_xi_points_from_element_to_parent);
	if ((0<number_of_xi_points)&&xi_points&&(0<element_dimension)&&
		(element_dimension<parent_dimension)&&element_to_parent)
	{
		rowsize=element_dimension+1;
		xi=xi_points;
		for (i=number_of_xi_points;i>0;i--)
		{
			for (k=0;k<element_dimension;k++)
			{
				xi_value[k] = (*xi)[k];
			}
			for (j=0;j<parent_dimension;j++)
			{
				final_xi_value = element_to_parent[j*rowsize];
				for (k=0;k<element_dimension;k++)
				{
					final_xi_value += element_to_parent[j*rowsize+k+1]*xi_value[k];
				}
				(*xi)[j]=final_xi_value;
			}
			xi++;
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"convert_xi_points_from_element_to_parent.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* convert_xi_points_from_element_to_parent */
