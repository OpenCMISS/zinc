/*******************************************************************************
FILE : finite_element_discretization.h

LAST MODIFIED : 12 October 2001

DESCRIPTION :
Functions for discretizing finite elements into points and simple sub-domains.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (FINITE_ELEMENT_DISCRETIZATION_H)
#define FINITE_ELEMENT_DISCRETIZATION_H

#include "computed_field/computed_field.h"
#include "finite_element/finite_element.h"
#include "graphics/auxiliary_graphics_types.h"

class Calculate_xi_points
{
protected:
	int numPoints;
	FE_value weight;

	Calculate_xi_points(int numPointsIn, FE_value totalWeightIn) :
		numPoints(numPointsIn),
		weight(totalWeightIn/numPointsIn)
	{
	}

public:
	int getNumPoints() const
	{
		return this->numPoints;
	}

	FE_value getWeight() const
	{
		return this->weight;
	}
};

class Calculate_xi_points_line_cell_centres : public Calculate_xi_points
{
public:
	Calculate_xi_points_line_cell_centres(int *numbersOfPoints) :
		Calculate_xi_points(numbersOfPoints[0], 1.0)
	{
	}

	template <class ProcessPoint>
	void forEachPoint(ProcessPoint &processPoint)
	{
		FE_value xi;
		const FE_value numPointsReal = static_cast<FE_value>(numPoints);
		int i;
		for (i = 0; i < this->numPoints; ++i)
		{
			xi = (i + 0.5) / numPointsReal;
			if (!processPoint(&xi))
				return;
		}
	}
};

class Calculate_xi_points_square_cell_centres : public Calculate_xi_points
{
	int numPoints1, numPoints2;
public:
	Calculate_xi_points_square_cell_centres(int *numbersOfPoints) :
		Calculate_xi_points(numbersOfPoints[0]*numbersOfPoints[1], 1.0),
		numPoints1(numbersOfPoints[0]),
		numPoints2(numbersOfPoints[1])
	{
	}

	template <class ProcessPoint>
	void forEachPoint(ProcessPoint &processPoint)
	{
		FE_value xi[2];
		const FE_value numPoints1Real = static_cast<FE_value>(this->numPoints1);
		const FE_value numPoints2Real = static_cast<FE_value>(this->numPoints2);
		int i, j;
		for (j = 0; j < numPoints2; ++j)
		{
			xi[1] = (j + 0.5) / numPoints2Real;
			for (i = 0; i < numPoints1; ++i)
			{
				xi[0] = (i + 0.5) / numPoints1Real;
				if (!processPoint(xi))
					return;
			}
		}
	}
};

class Calculate_xi_points_triangle_cell_centres : public Calculate_xi_points
{
	int numSimplexPoints;
public:
	Calculate_xi_points_triangle_cell_centres(int numSimplexPointsIn) :
		Calculate_xi_points(numSimplexPointsIn*numSimplexPointsIn, 0.5),
		numSimplexPoints(numSimplexPointsIn)
	{
	}

	template <class ProcessPoint>
	void forEachPoint(ProcessPoint &processPoint)
	{
		/* Triangular elements are subdivided into equal-sized upright and
		   reversed triangles as follows:

		       /\
		      /__\
		     /\  /\
		    /__\/__\

		   Note that xi0 ranges from 0 to 1 along the base of the triangle,
		   and xi1 ranges from 0 to 1 up the left side. However, only half
		   of a unit square is used: the centre of the far side is thus
		   xi0, xi1 = 0.5, 0.5. */
		FE_value xi[2];
		const FE_value numSimplexPointsReal = static_cast<FE_value>(this->numSimplexPoints);
		const double one_third = 1.0/3.0;
		int i, j, numPoints1;
		// add centres of upright triangles
		for (j = 0; j < this->numSimplexPoints; ++j)
		{
			xi[1] = (j + one_third)/numSimplexPointsReal;
			numPoints1 = this->numSimplexPoints - j;
			for (i = 0; i < numPoints1; ++i)
			{
				xi[0] = (i + one_third)/numSimplexPointsReal;
				if (!processPoint(xi))
					return;
			}
		}
		/* add centres of reversed triangles */
		for (j = 1; j < this->numSimplexPoints; ++j)
		{
			xi[1] = (j - one_third)/numSimplexPointsReal;
			numPoints1 = this->numSimplexPoints - j + 1;
			for (i = 1; i < numPoints1; ++i)
			{
				xi[0] = (i - one_third)/numSimplexPointsReal;
				if (!processPoint(xi))
					return;
			}
		}
	}
};

class Calculate_xi_points_wedge_cell_centres : public Calculate_xi_points
{
	int lineAxis;
	int simplexAxis1;
	int simplexAxis2;
	int numLinePoints;
	int numSimplexPoints;
public:
	// axes must be 0, 1, 2 in some order
	Calculate_xi_points_wedge_cell_centres(int lineAxisIn, int simplexAxis1In, int simplexAxis2In,
			int numLinePointsIn, int numSimplexPointsIn) :
		Calculate_xi_points(numLinePointsIn*numSimplexPointsIn*numSimplexPointsIn, 0.5),
		lineAxis(lineAxisIn),
		simplexAxis1(simplexAxis1In),
		simplexAxis2(simplexAxis2In),
		numLinePoints(numLinePointsIn),
		numSimplexPoints(numSimplexPointsIn)
	{
	}

	template <class ProcessPoint>
	void forEachPoint(ProcessPoint &processPoint)
	{
		// layout as for triangle, multiplied by points on line axis
		FE_value xi[3];
		const FE_value numLinePointsReal = static_cast<FE_value>(this->numLinePoints);
		const FE_value numSimplexPointsReal = static_cast<FE_value>(this->numSimplexPoints);
		const double one_third = 1.0/3.0;
		int i, j, k, numPoints1;
		for (k = 0; k < this->numLinePoints; ++k)
		{
			xi[this->lineAxis] = (k + 0.5)/numLinePointsReal;
			// add centres of upright triangles
			for (j = 0; j < this->numSimplexPoints; ++j)
			{
				xi[this->simplexAxis2] = (j + one_third)/numSimplexPointsReal;
				numPoints1 = this->numSimplexPoints - j;
				for (i = 0; i < numPoints1; ++i)
				{
					xi[this->simplexAxis1] = (i + one_third)/numSimplexPointsReal;
					if (!processPoint(xi))
						return;
				}
			}
			/* add centres of reversed triangles */
			for (j = 1; j < this->numSimplexPoints; ++j)
			{
				xi[this->simplexAxis2] = (j - one_third)/numSimplexPointsReal;
				numPoints1 = this->numSimplexPoints - j + 1;
				for (i = 1; i < numPoints1; ++i)
				{
					xi[this->simplexAxis1] = (i - one_third)/numSimplexPointsReal;
					if (!processPoint(xi))
						return;
				}
			}
		}
	}
};

class Calculate_xi_points_cube_cell_centres : public Calculate_xi_points
{
	int numPoints1, numPoints2, numPoints3;
public:
	Calculate_xi_points_cube_cell_centres(int *numbersOfPoints) :
		Calculate_xi_points(numbersOfPoints[0]*numbersOfPoints[1]*numbersOfPoints[2], 1.0),
		numPoints1(numbersOfPoints[0]),
		numPoints2(numbersOfPoints[1]),
		numPoints3(numbersOfPoints[2])
	{
	}

	template <class ProcessPoint>
	void forEachPoint(ProcessPoint &processPoint)
	{
		FE_value xi[3];
		const FE_value numPoints1Real = static_cast<FE_value>(this->numPoints1);
		const FE_value numPoints2Real = static_cast<FE_value>(this->numPoints2);
		const FE_value numPoints3Real = static_cast<FE_value>(this->numPoints3);
		int i, j, k;
		for (k = 0; k < numPoints3; ++k)
		{
			xi[2] = (k + 0.5) / numPoints3Real;
			for (j = 0; j < numPoints2; ++j)
			{
				xi[1] = (j + 0.5) / numPoints2Real;
				for (i = 0; i < numPoints1; ++i)
				{
					xi[0] = (i + 0.5) / numPoints1Real;
					if (!processPoint(xi))
						return;
				}
			}
		}
	}
};

class Calculate_xi_points_tetrahedron_cell_centres : public Calculate_xi_points
{
	int numSimplexPoints;
public:
	Calculate_xi_points_tetrahedron_cell_centres(int numSimplexPointsIn) :
		Calculate_xi_points(calculateNumPoints(numSimplexPointsIn), 1.0/6.0),
		numSimplexPoints(numSimplexPointsIn)
	{
	}

	static int calculateNumPoints(int numSimplexPointsIn)
	{
		// sum the number of cell centres in sub-tetrahedra
		int numPoints = 1;
		int rowPoints = 1;
		for (int i = 2; i <= numSimplexPointsIn; ++i)
		{
			rowPoints += i;
			numPoints += rowPoints;
		}
		// add 4 tetrahedra per sub-octahedra
		numPoints += 4*(numPoints - rowPoints);
		return numPoints;
	}

	template <class ProcessPoint>
	void forEachPoint(ProcessPoint &processPoint)
	{
		/* Similar to triangle subdivision, tetrahedra are divided into
		   sub-tetrahedra with octahedra (8 equilateral triangle faces)
		   in between them. The octahedra have 4 times the volume of the
		   tetrahedra, hence are arbitrarily subdivided along one of their
		   three long axes to make 4 further tetrahedra. The tetrahedra
		   coming from the octahedra have 2 equilateral triangle faces
		   matching those in the other sub-tetrahedra, but the other 2 faces
		   share a common, elongated edge. Octahedra can be visualised as
		   2 square pyramids back-to-back, and look identical on all three
		   axes. Xi values within tetrahedra vary from 0 to 1 along each
		   axis, but only the lower tetrahedron of this unit cube is used.
		   If a tetrahedron is subdivided in 2*2*2 fashion, it will contain
		   a single octahedron. One of the axes of the octahedron goes
		   through the point xi0, xi1, xi2 = 0.5, 0.0, 0.0, and it is this
		   axis that becomes the "long edge" of the 4 tetrahedra
		   sub-divided from it. */
		FE_value xi[3];
		const FE_value numSimplexPointsReal = static_cast<FE_value>(this->numSimplexPoints);
		int i, j, k, numPoints1, numPoints2;
		// add the centres of the sub-tetrahedra */
		for (k = 0; k < this->numSimplexPoints; ++k)
		{
			xi[2] = (k + 0.25)/numSimplexPointsReal;
			numPoints2 = this->numSimplexPoints - k;
			for (j = 0; j <= numPoints2; ++j)
			{
				xi[1] = (j + 0.25)/numSimplexPointsReal;
				numPoints1 = numPoints2 - j;
				for (i = 0; i < numPoints1; ++i)
				{
					xi[0] = (i + 0.25)/numSimplexPointsReal;
					if (!processPoint(xi))
						return;
				}
			}
		}
		/* Then add points in centres of tetrahedra subdivided from
				octahedra */
		for (k = 1; k < this->numSimplexPoints; ++k)
		{
			xi[2] = (k - 0.5)/numSimplexPointsReal;
			numPoints2 = this->numSimplexPoints - k + 1;
			for (j = 1; j < numPoints2; ++j)
			{
				xi[1] = (j - 0.5)/numSimplexPointsReal;
				numPoints1 = numPoints2 - j + 1;
				for (i = 1; i < numPoints1; ++i)
				{
					xi[0] = (i - 0.75)/numSimplexPointsReal;
					if (!processPoint(xi))
						return;
				}
			}
		}
		for (k = 1; k < this->numSimplexPoints; ++k)
		{
			xi[2] = (k - 0.25)/numSimplexPointsReal;
			numPoints2 = this->numSimplexPoints - k + 1;
			for (j = 1; j < numPoints2; ++j)
			{
				xi[1] = (j - 0.75)/numSimplexPointsReal;
				numPoints1 = numPoints2 - j + 1;
				for (i = 1; i < numPoints1; ++i)
				{
					xi[0] = (i - 0.5)/numSimplexPointsReal;
					if (!processPoint(xi))
						return;
				}
			}
		}
		for (k = 1; k < this->numSimplexPoints; ++k)
		{
			xi[2] = (k - 0.75)/numSimplexPointsReal;
			numPoints2 = this->numSimplexPoints - k + 1;
			for (j = 1; j < numPoints2; ++j)
			{
				xi[1] = (j - 0.25)/numSimplexPointsReal;
				numPoints1 = numPoints2 - j + 1;
				for (i = 1; i < numPoints1; ++i)
				{
					xi[0] = (i - 0.5)/numSimplexPointsReal;
					if (!processPoint(xi))
						return;
				}
			}
		}
		for (k = 1; k < this->numSimplexPoints; ++k)
		{
			xi[2] = (k - 0.5)/numSimplexPointsReal;
			numPoints2 = this->numSimplexPoints - k + 1;
			for (j = 1; j < numPoints2; ++j)
			{
				xi[1] = (j - 0.5)/numSimplexPointsReal;
				numPoints1 = numPoints2 - j + 1;
				for (i = 1; i < numPoints1; ++i)
				{
					xi[0] = (i - 0.25)/numSimplexPointsReal;
					if (!processPoint(xi))
						return;
				}
			}
		}
	}
};

int FE_element_shape_get_xi_points_cell_centres(
	struct FE_element_shape *element_shape, int *number_in_xi,
	int *number_of_xi_points_address, FE_value_triple **xi_points_address);
/*******************************************************************************
LAST MODIFIED : 20 April 2001

DESCRIPTION :
Calculates the <number_of_xi_points> in the centres of uniform cells across the
<element_shape> according to <number_in_xi>.
If <xi_points_address> is supplied an array containing the xi locations of these
points is allocated and put in this address. Xi positions are always returned as
triples with remaining xi coordinates 0 for 1-D and 2-D cases.
Note the actual number and layout is dependent on the <element_shape>; see
comments for simplex and polygons shapes for more details.
==============================================================================*/

int FE_element_shape_get_xi_points_cell_corners(
	struct FE_element_shape *element_shape, int *number_in_xi,
	int *number_of_xi_points_address, FE_value_triple **xi_points_address);
/*******************************************************************************
LAST MODIFIED : 20 April 2001

DESCRIPTION :
Calculates the <number_of_xi_points> in the corners of uniform cells across the
<element_shape> according to <number_in_xi>.
If <xi_points_address> is supplied an array containing the xi locations of these
points is allocated and put in this address. Xi positions are always returned as
triples with remaining xi coordinates 0 for 1-D and 2-D cases.
Note the actual number and layout is dependent on the <element_shape>; see
comments for simplex and polygons shapes for more details.
==============================================================================*/

int FE_element_shape_get_indices_for_xi_location_in_cell_corners(
	struct FE_element_shape *element_shape, const int *number_in_xi,
	const FE_value *xi, int *indices);
/*******************************************************************************
LAST MODIFIED : 18 October 2005

DESCRIPTION :
Determines if <xi> cooresponds to a location on the corners of uniform cells
across the <element_shape> according to <number_in_xi>.
If so, returns 1 and sets the <indices> to match.
Otherwise the routine returns 0.
==============================================================================*/

int FE_element_get_xi_points(struct FE_element *element,
	cmzn_element_point_sampling_mode sampling_mode,
	int *number_in_xi, FE_value_triple exact_xi, cmzn_fieldcache_id field_cache,
	struct Computed_field *coordinate_field, struct Computed_field *density_field,
	int *number_of_xi_points_address, FE_value_triple **xi_points_address);
/*******************************************************************************
LAST MODIFIED : 20 April 2001

DESCRIPTION :
Calculates the <number_of_xi_points> across the <element_shape> according to
the <sampling_mode> and some of <number_in_xi>, <exact_xi>,
<coordinate_field> and <density_field>, depending on the mode.
If <xi_points_address> is supplied an array containing the xi locations of these
points is allocated and put in this address. Xi positions are always returned as
triples with remaining xi coordinates 0 for 1-D and 2-D cases.
<exact_xi> should be supplied for sample mode SET_LOCATION - although it
is trivial, it is passed and used here to provide a consistent interface.]
@param field_cache  cmzn_fieldcache for evaluating coordinate and density
fields, required for DENSITY and POISSON modes. Time is expected to have been
set in the field_cache if needed.
==============================================================================*/

int FE_element_convert_xi_points_cell_corners_to_top_level(
	struct FE_element *element, struct FE_element *top_level_element,
	int *top_level_number_in_xi, int number_of_xi_points, FE_value_triple *xi_points,
	int **top_level_xi_point_numbers_address);
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

int FE_element_get_numbered_xi_point(struct FE_element *element,
	cmzn_element_point_sampling_mode sampling_mode,
	int *number_in_xi, FE_value_triple exact_xi, cmzn_fieldcache_id field_cache,
	struct Computed_field *coordinate_field, struct Computed_field *density_field,
	int xi_point_number, FE_value *xi);
/*******************************************************************************
LAST MODIFIED : 3 December 2001

DESCRIPTION :
Returns in <xi> the location of the <xi_point_number> out of those that would
be calculated by FE_element_get_xi_points. The default_behaviour is to
Call the above function, extract the xi location and DEALLOCATE the xi_points.
This is quite expensive; for this reason cell_centres and cell_corners in line,
square and cube elements, as well as exact_xi, are handled separately since the
calculation is trivial.
@param field_cache  cmzn_fieldcache for evaluating coordinate and density
fields, required for DENSITY and POISSON modes. Time is expected to be set in
the field_cache if needed.
==============================================================================*/

int convert_xi_points_from_element_to_parent(int number_of_xi_points,
	FE_value_triple *xi_points,int element_dimension,int parent_dimension,
	FE_value *element_to_parent);
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
xi(parent) = b + A.xi(element)
while A is the remainder of the matrix. (Appropriate matrices are given by the
face_to_element member of struct FE_element_shape, and by function
FE_element_get_top_level_element_conversion.)
==============================================================================*/

#endif /* !defined (FINITE_ELEMENT_DISCRETIZATION_H) */
