/**
 * FILE : finite_element_shape.cpp
 *
 * Finite element shape bounds.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cmath>
#include "finite_element/finite_element_region.h"
#include "finite_element/finite_element_shape.hpp"
#include "general/debug.h"
#include "general/list_private.h"
#include "general/matrix_vector.h"
#include "general/message.h"
#include "general/mystring.h"
#include "general/object.h"

/*
Module types
------------
*/

struct FE_element_shape
/*******************************************************************************
LAST MODIFIED : 9 October 2002

DESCRIPTION :
A description of the shape of an element in Xi space.  It includes how to
calculate face coordinates from element coordinates and how to calculate element
coordinates from face coordinates.
==============================================================================*/
{
	/* the number of xi coordinates */
	int dimension;
	/* the structure description.  Similar to type for a FE_basis
		- doesn't have the dimension in the first position (type[0])
		- the diagonal entry is the shape type
		- non-zero off-diagonal entries indicate that the dimensions are linked.
			For a polygon, it is the number of vertices
		eg. a 5-gon in dimensions 1 and 2 and linear in the third dimension
			POLYGON_SHAPE 5             0
										POLYGON_SHAPE 0
																	LINE_SHAPE
		eg. tetrahedron
			SIMPLEX_SHAPE 1             1
										SIMPLEX_SHAPE 1
																	SIMPLEX_SHAPE */
	int *type;
	/* the number of faces */
	int number_of_faces;
	/* the equations for the faces of the element.  This is a linear system
		b = A xi, where A is <number_of_faces> by <dimension> matrix whose entries
		are either 0 or 1 and b is a vector whose entries are
		0,...number_of_faces_in_this_dimension-1.  For a cube the system would be
			0   1 0 0  xi1       2
			1   1 0 0  xi2       3
			0 = 0 1 0  xi3       4
			1   0 1 0            5
			0   0 0 1            8
			1   0 0 1            9
		For a 5-gon by linear above the system would be
			0   1 0 0  xi1       5
			1   1 0 0  xi2       6
			2   1 0 0  xi3       7
			3 = 1 0 0            8
			4   1 0 0            9
			0   0 0 1           10
			1   0 0 1           11
			The "equations" for the polygon faces, don't actually describe the faces,
				but are in 1 to 1 correspondence - first represents 0<=xi1<=1/5 and
				xi2=1.
		For a tetrahedron the system would be
			0   1 0 0  xi1       2
			0 = 0 1 0  xi2       4
			0   0 0 1  xi3       8
			1   1 1 1           15
		A unique number is calculated for each number as follows
		- a value is calculated for each column by multiplying the number for the
			previous column (start with 1, left to right) by
			- the number_of_vertices for the first polygon column
			- 1 for the second polygon column
			- 2 otherwise
		- a value for each row by doing the dot product of the row and the column
			values
		- the entry of b for that row is added to the row value to give the unique
			number
		These numbers are stored in <faces>.
		The values for the above examples are given on the right */
	int *faces;
	/* for each face an outwards pointing normal stored as a vector of size <dimension> */
	FE_value *face_normals;
	/* for each face an affine transformation, b + A xi for calculating the
		element xi coordinates from the face xi coordinates.  For each face there is
		a <number_of_xi_coordinates> by <number_of_xi_coordinates> array with the
		translation b in the first column.  For a cube the translations could be
		(not unique)
		face 2 :  0 0 0 ,  face 3 : 1 0 0 ,  face 4 : 0 0 1 , 1(face) goes to 3 and
		          0 1 0             0 1 0             0 0 0   2(face) goes to 1 to
		          0 0 1             0 0 1             0 1 0   maintain right-
		                                                      handedness (3x1=2)
		face 5 :  0 0 1 ,  face 8 : 0 1 0 ,  face 9 : 0 1 0
		          1 0 0             0 0 1             0 0 1
		          0 1 0             0 0 0             1 0 0
		For the 5-gon by linear the faces would be
		face 5 :  0 1/5 0 ,  face 6 : 1/5 1/5 0 ,  face 7 : 2/5 1/5 0
		          1 0   0             1   0   0             1   0   0
		          0 0   1             0   0   1             0   0   1
		face 8 :  3/5 1/5 0 ,  face 9 : 4/5 1/5 0
		          1   0   0             1   0   0
		          0   0   1             0   0   1
		face 10 : 0 1 0 ,  face 11 : 0 1 0
		          0 0 1              0 0 1
		          0 0 0              1 0 0
		For the tetrahedron the faces would be
		face 2 :  0 0 0 ,  face 4 : 1 -1 -1 ,  face 8 : 0  0  1
		          0 1 0             0  0  0             1 -1 -1
		          0 0 1             0  1  0             0  0  0
		face 15 : 0  1  0
		          0  0  1
		          1 -1 -1
		The transformations are stored by row (ie. column number varying fastest) */
	FE_value *face_to_element;
	/* the number of structures that point to this shape.  The shape cannot be
		destroyed while this is greater than 0 */
	int access_count;
}; /* struct FE_element_shape */

FULL_DECLARE_LIST_TYPE(FE_element_shape);

/*
Module functions
----------------
*/

struct Match_FE_element_shape_data
{
	int dimension;
	const int *type;
}; /* struct Match_FE_element_shape_data */

static int match_FE_element_shape(struct FE_element_shape *shape,
	void *match_FE_element_shape_data)
/*******************************************************************************
LAST MODIFIED : 30 April 2003

DESCRIPTION :
Checks if the <match_FE_element_shape_data> matchs the <shape>.
Note a NULL shape type means an unspecified shape of that dimension.
==============================================================================*/
{
	const int *find_type;
	int i, return_code, *shape_type;
	struct Match_FE_element_shape_data *match_data;

	ENTER(match_FE_element_shape);
	if (shape && (match_data =
		(struct Match_FE_element_shape_data *)match_FE_element_shape_data))
	{
		if (match_data->dimension == shape->dimension)
		{
			find_type = match_data->type;
			shape_type = shape->type;
			if (find_type && shape_type)
			{
				i = (match_data->dimension)*((match_data->dimension)+1)/2;
				while ((i>0)&&(*find_type== *shape_type))
				{
					find_type++;
					shape_type++;
					i--;
				}
				if (i>0)
				{
					return_code=0;
				}
				else
				{
					return_code=1;
				}
			}
			else if ((!find_type) && (!shape_type))
			{
				/* no type array: unspecified shape */
				return_code = 1;
			}
			else
			{
				return_code = 0;
			}
		}
		else
		{
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"match_FE_element_shape.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* match_FE_element_shape */

static struct FE_element_shape *find_FE_element_shape_in_list(int dimension,
	const int *type,struct LIST(FE_element_shape) *list)
/*******************************************************************************
LAST MODIFIED : 18 November 2002

DESCRIPTION :
Searchs the <list> for the element shape with the specified <dimension> and
<type> and returns the address of the element_shape.
A NULL <type> means an unspecified shape of <dimension>.
==============================================================================*/
{
	struct Match_FE_element_shape_data match_data;
	struct FE_element_shape *shape;

	ENTER(find_FE_element_shape_in_list);
	if ((dimension > 0) && list)
	{
		match_data.dimension = dimension;
		match_data.type = type;
		shape = FIRST_OBJECT_IN_LIST_THAT(FE_element_shape)(match_FE_element_shape,
			(void *)(&match_data),list);
	}
	else
	{
		shape = (struct FE_element_shape *)NULL;
	}
	LEAVE;

	return (shape);
} /* find_FE_element_shape_in_list */

/*
Global functions
----------------
*/

struct FE_element_shape *CREATE(FE_element_shape)(int dimension,
	const int *type, struct FE_region *fe_region)
/*******************************************************************************
LAST MODIFIED : 8 July 2003

DESCRIPTION :
Searchs the <element_shape_list> for a shape with the specified <dimension> and
<type>.  If one is not found, a shape is created (with <type> duplicated) and
added to the list.  The shape is returned.
<type> is analogous to the basis type array, except that the entries are 0 or 1.
If <type> is omitted an "unspecified" shape of the given <dimension> is
returned. An element with such a shape may not have fields defined on it until
it is given a proper shape.
==============================================================================*/
{
	FE_value *face_to_element, *face_normals, weight;
	const int *type_entry;
	int dimension_2,*face,i,j,k,k_set,l,linked_coordinate,linked_k,linked_offset,
		*linked_offsets,no_error,number_of_faces,number_of_polygon_verticies,offset,
		*shape_type,simplex_coordinate,simplex_dimension,temp_int, xi_coordinate;
	struct FE_element_shape *shape;

	ENTER(CREATE(FE_element_shape));
	shape = (struct FE_element_shape *)NULL;
/*???debug */
/*printf("enter CREATE(FE_element_shape)\n");*/
	if (dimension > 0)
	{
		/* check if the shape already exists */
		if (!(shape = find_FE_element_shape_in_list(dimension, type,
			FE_region_get_FE_element_shape_list(fe_region))))
		{
			if (ALLOCATE(shape, struct FE_element_shape, 1))
			{
				/* make an unspecified shape = no type array or faces */
				shape->dimension = dimension;
				shape->type = (int *)NULL;
				shape->number_of_faces = 0;
				shape->faces = (int *)NULL;
				shape->face_normals = (FE_value *)NULL;
				shape->face_to_element = (FE_value *)NULL;
				shape->access_count = 0;
			}
			shape_type = (int *)NULL;
			linked_offsets = (int *)NULL;
			if (type)
			{
				ALLOCATE(shape_type,int,(dimension*(dimension+1))/2);
				/* offsets is working storage used within this function */
				ALLOCATE(linked_offsets,int,dimension);
			}
			if (shape && shape_type && linked_offsets)
			{
				shape->type = shape_type;
				if (1==dimension)
				{
					if ((LINE_SHAPE == *type) ||
						(UNSPECIFIED_SHAPE == *type))
					{
						*(shape_type) = *type;
						shape->number_of_faces = 0;
						shape->faces = (int *)NULL;
						shape->face_to_element = (FE_value *)NULL;
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"CREATE(FE_element_shape).  Invalid shape type");
						shape=(struct FE_element_shape *)NULL;
					}
				}
				else
				{
					/* copy the type */
					type_entry=type;
					for (i=(dimension*(dimension+1))/2;i>0;i--)
					{
						*shape_type= *type_entry;
						shape_type++;
						type_entry++;
					}
					/* check that the type is valid and calculate the number of faces */
					no_error=1;
					number_of_faces=0;
					xi_coordinate=0;
					shape_type=shape->type;
					while (no_error&&(xi_coordinate<dimension))
					{
						linked_offsets[xi_coordinate]=0;
						xi_coordinate++;
						switch (*shape_type)
						{
							case UNSPECIFIED_SHAPE:
							{
								/* unspecified */
								number_of_faces = 0;
								/* check that not connected to anything else */
								i = dimension - xi_coordinate;
								shape_type++;
								while (no_error && (i>0))
								{
									if (0 != *shape_type)
									{
										no_error = 0;
									}
									shape_type++;
									i--;
								}
							} break;
							case LINE_SHAPE:
							{
								/* line */
								number_of_faces += 2;
								/* check that not connected to anything else */
								i=dimension-xi_coordinate;
								shape_type++;
								while (no_error&&(i>0))
								{
									if (0!= *shape_type)
									{
										no_error=0;
									}
									shape_type++;
									i--;
								}
							} break;
							case POLYGON_SHAPE:
							{
								/* polygon */
								/* check if the other polygon coordinate is before */
								type_entry=shape_type;
								i=xi_coordinate-1;
								j=dimension-xi_coordinate;
								number_of_polygon_verticies=0;
								while (no_error&&(i>0))
								{
									j++;
									type_entry -= j;
									if (*type_entry)
									{
										if (number_of_polygon_verticies)
										{
											no_error=0;
										}
										else
										{
											number_of_polygon_verticies= *type_entry;
											if (number_of_polygon_verticies>=3)
											{
												linked_offsets[xi_coordinate-1]=i-xi_coordinate;
											}
											else
											{
												no_error=0;
											}
										}
									}
									i--;
								}
								if (no_error)
								{
									if (number_of_polygon_verticies)
									{
										/* other polygon coordinate is before */
										/* check that not connected to anything else */
										i=dimension-xi_coordinate;
										shape_type++;
										while (no_error&&(i>0))
										{
											if (0!= *shape_type)
											{
												no_error=0;
											}
											shape_type++;
											i--;
										}
									}
									else
									{
										/* check if the other polygon coordinate is after */
										shape_type++;
										i=dimension-xi_coordinate;
										number_of_polygon_verticies=0;
										while (no_error&&(i>0))
										{
											if (*shape_type)
											{
												if (number_of_polygon_verticies)
												{
													no_error=0;
												}
												else
												{
													number_of_polygon_verticies= *shape_type;
													if (3<=number_of_polygon_verticies)
													{
														linked_offset=dimension-xi_coordinate+1-i;
														if (POLYGON_SHAPE==shape_type[(linked_offset*
															(2*(dimension-xi_coordinate+1)-linked_offset+1))/
															2-linked_offset])
														{
															linked_offsets[xi_coordinate-1]=linked_offset;
															number_of_faces += number_of_polygon_verticies;
														}
														else
														{
															no_error=0;
														}
													}
													else
													{
														no_error=0;
													}
												}
											}
											shape_type++;
											i--;
										}
										if (number_of_polygon_verticies<3)
										{
											no_error=0;
										}
									}
								}
							} break;
							case SIMPLEX_SHAPE:
							{
								/* simplex */
								/* check preceding dimensions */
								type_entry=shape_type;
								i=xi_coordinate-1;
								j=dimension-xi_coordinate;
								while (no_error&&(i>0))
								{
									j++;
									type_entry -= j;
									if (*type_entry)
									{
										if (SIMPLEX_SHAPE== *(type_entry-(xi_coordinate-i)))
										{
											linked_offsets[xi_coordinate-1]=i-xi_coordinate;
										}
										else
										{
											no_error=0;
										}
									}
									i--;
								}
								if (0==linked_offsets[xi_coordinate-1])
								{
									/* this is first simplex coordinate */
									number_of_faces++;
								}
								else
								{
									/* check intermediary links */
									/* calculate first simplex coordinate */
									i=xi_coordinate+linked_offsets[xi_coordinate-1];
									type_entry=(shape->type)+((i-1)*(2*dimension-i+2))/2;
									j=dimension-i;
									k=xi_coordinate-i;
									i++;
									while (no_error&&(i<xi_coordinate))
									{
										j--;
										k += j;
										type_entry++;
										if (((0== *type_entry)&&(0!=type_entry[k]))||
											((0!= *type_entry)&&(0==type_entry[k])))
										{
											no_error=0;
										}
										i++;
									}
								}
								number_of_faces++;
								/* check succeeding dimensions */
								shape_type++;
								i=dimension-xi_coordinate;
								while (no_error&&(i>0))
								{
									if (*shape_type)
									{
										linked_offset=dimension-xi_coordinate+1-i;
										if (SIMPLEX_SHAPE==shape_type[(linked_offset*
											(2*(dimension-xi_coordinate+1)-linked_offset+1))/
											2-linked_offset])
										{
											if (linked_offsets[xi_coordinate-1]<=0)
											{
												linked_offsets[xi_coordinate-1]=linked_offset;
											}
										}
										else
										{
											no_error=0;
										}
									}
									shape_type++;
									i--;
								}
								if (0==linked_offsets[xi_coordinate-1])
								{
									no_error=0;
								}
							} break;
							default:
							{
								no_error=0;
							} break;
						}
					}
					if (no_error)
					{
						dimension_2=dimension*dimension;
						if (ALLOCATE(face,int,number_of_faces)&&
							ALLOCATE(face_normals,FE_value,number_of_faces*dimension)&&
							ALLOCATE(face_to_element,FE_value,number_of_faces*dimension_2))
						{
							shape->number_of_faces=number_of_faces;
							shape->faces=face;
							shape->face_normals=face_normals;
							shape->face_to_element=face_to_element;
							for (i=number_of_faces*dimension_2;i>0;i--)
							{
								*face_to_element=0;
								face_to_element++;
							}
							face_to_element=shape->face_to_element;
							for (i=number_of_faces;i>0;i--)
							{
								*face=0;
								face++;
							}
							face=shape->faces;
							for (i=number_of_faces*dimension;i>0;i--)
							{
								*face_normals=0.0;
								face_normals++;
							}
							face_normals=shape->face_normals;
							shape_type=shape->type;
							offset=1;
							/* loop over the coordinates calculating the face matrices
								(number dependent on <*shape_type>) for each */
							for (xi_coordinate=0;xi_coordinate<dimension;xi_coordinate++)
							{
								switch (*shape_type)
								{
									case LINE_SHAPE:
									{
										/* line */
										/* two faces for this coordinate */
										offset *= 2;
										*face=offset;
										face++;
										*face=offset+1;
										face++;
										*(face_normals + xi_coordinate) = -1.0;
										face_normals += dimension;
										*(face_normals + xi_coordinate) = 1.0;
										face_normals += dimension;
										face_to_element[dimension_2+xi_coordinate*dimension]=1;
#define CALCULATE_K_SET() \
k_set=k; \
if (POLYGON_SHAPE== *type_entry) \
{ \
	linked_offset=linked_offsets[j]; \
	/* for when *shape_type is POLYGON_SHAPE */ \
	if (xi_coordinate!=j+linked_offset) \
	{ \
		if (0<linked_offset) \
		{ \
			/* first polygon coordinate */ \
			linked_k=k; \
			if (linked_k<=xi_coordinate) \
			{ \
				linked_k--; \
			} \
			linked_k += linked_offset; \
			if (linked_k>=dimension) \
			{ \
				linked_k -= dimension; \
			} \
			if (linked_k<xi_coordinate) \
			{ \
				linked_k++; \
			} \
			if (linked_k<k) \
			{ \
				k_set=linked_k; \
			} \
		} \
		else \
		{ \
			/* second polygon coordinate */ \
			linked_k=k; \
			if (linked_k<=xi_coordinate) \
			{ \
				linked_k--; \
			} \
			linked_k -= linked_offset; \
			if (linked_k>=dimension) \
			{ \
				linked_k -= dimension; \
			} \
			if (linked_k<xi_coordinate) \
			{ \
				linked_k++; \
			} \
			if (k<linked_k) \
			{ \
				k_set=linked_k; \
			} \
		} \
	} \
}
										k=xi_coordinate+1;
										if (k>=dimension)
										{
											k=1;
										}
										type_entry=shape->type;
										for (j=0;j<dimension;j++)
										{
											if (j!=xi_coordinate)
											{
												CALCULATE_K_SET();
												face_to_element[k_set]=1;
												face_to_element[dimension_2+k_set]=1;
												k++;
												if (k>=dimension)
												{
													k=1;
												}
											}
											face_to_element += dimension;
											type_entry += dimension-j;
										}
										face_to_element += dimension_2;
									} break;
									case POLYGON_SHAPE:
									{
										/* polygon */
										/* <number_of_polygon_verticies>+1 faces in total for the
											two polygon dimension */
										if (0<(linked_offset=linked_offsets[xi_coordinate]))
										{
											/* first polygon dimension */
											number_of_polygon_verticies=shape_type[linked_offset];
											offset *= number_of_polygon_verticies;
											linked_offset += xi_coordinate;
											for (i=0;i<number_of_polygon_verticies;i++)
											{
												*face=offset;
												face++;
												offset++;
												*(face_normals + linked_offset) = 1.0;
												face_normals += dimension;
												k=xi_coordinate+1;
												if (k>=dimension)
												{
													k=1;
												}
												type_entry=shape->type;
												j=0;
												while (j<xi_coordinate)
												{
													CALCULATE_K_SET();
													face_to_element[k_set]=1;
													k++;
													if (k>=dimension)
													{
														k=1;
													}
													type_entry += dimension-j;
													j++;
													face_to_element += dimension;
												}
												*face_to_element=
													(FE_value)i/(FE_value)number_of_polygon_verticies;
												face_to_element[k]=
													1./(FE_value)number_of_polygon_verticies;
												k++;
												if (k>=dimension)
												{
													k=1;
												}
												type_entry += dimension-j;
												j++;
												face_to_element += dimension;
												while (j<linked_offset)
												{
													CALCULATE_K_SET();
													face_to_element[k_set]=1;
													k++;
													if (k>=dimension)
													{
														k=1;
													}
													type_entry += dimension-j;
													j++;
													face_to_element += dimension;
												}
												*face_to_element=1;
												face_to_element += dimension;
												type_entry += dimension-j;
												j++;
												while (j<dimension)
												{
													CALCULATE_K_SET();
													face_to_element[k_set]=1;
													k++;
													if (k>=dimension)
													{
														k=1;
													}
													type_entry += dimension-j;
													j++;
													face_to_element += dimension;
												}
											}
										}
									} break;
									case SIMPLEX_SHAPE:
									{
										offset *= 2;
										*face=offset;
										face++;
										*(face_normals + xi_coordinate) = -1.0;
										face_normals += dimension;
										simplex_dimension=0;
										/* calculate the simplex dimension */
										simplex_coordinate=xi_coordinate;
										do
										{
											simplex_coordinate += linked_offsets[simplex_coordinate];
											simplex_dimension++;
										} while (simplex_coordinate!=xi_coordinate);
										simplex_coordinate=xi_coordinate;
										linked_offset=simplex_dimension;
										if (0<linked_offsets[xi_coordinate])
										{
											do
											{
												simplex_coordinate +=
													linked_offsets[simplex_coordinate];
												linked_offset--;
											} while (linked_offsets[simplex_coordinate]>0);
											face[simplex_coordinate-xi_coordinate] += offset;
										}
										else
										{
											/* last simplex dimension */
											*face += offset+1;
											face++;
											weight = 1.0/sqrt((double)simplex_dimension);
											simplex_coordinate=xi_coordinate;
											for (j=simplex_dimension;j>0;j--)
											{
												face_normals[simplex_coordinate]=weight;
												simplex_coordinate += linked_offsets[simplex_coordinate];
											}
											face_normals += dimension;
											simplex_coordinate=xi_coordinate;
										}
										linked_offset--;
										simplex_coordinate += linked_offsets[simplex_coordinate];
										linked_coordinate=simplex_coordinate;
										for (j=simplex_dimension-linked_offset;j>0;j--)
										{
											linked_coordinate += linked_offsets[linked_coordinate];
										}
										k=xi_coordinate+1;
										if (k>=dimension)
										{
											k=1;
										}
										if (simplex_dimension<dimension)
										{
											/* make sure that k is not in the simplex containing
												xi_coordinate */
											i=1;
											do
											{
												if (k>xi_coordinate)
												{
													i=k;
												}
												else
												{
													i=k-1;
												}
												if (0==linked_offsets[i])
												{
													i=0;
												}
												else
												{
													l=i;
													do
													{
														l += linked_offsets[l];
													} while ((l!=i)&&(l!=xi_coordinate));
													if (l==xi_coordinate)
													{
														k++;
														if (k>=dimension)
														{
															k=1;
														}
														i=1;
													}
													else
													{
														i=0;
													}
												}
											} while (1==i);
										}
										/* start with
											- simplex_coordinate being the first simplex coordinate
											- (simplex_dimension-linked_offset)-1 being the
												coordinate in the simplex of the xi_coordinate
											- linked_coordinate being the coordinate after
												xi_coordinate in the simplex
											*/
										type_entry=shape->type;
										for (j=0;j<dimension;j++)
										{
											if (j==simplex_coordinate)
											{
												simplex_coordinate +=
													linked_offsets[simplex_coordinate];
												linked_offset--;
												if (j!=xi_coordinate)
												{
													if (0==linked_offset)
													{
														face_to_element[0]=1;
														temp_int=xi_coordinate+
															linked_offsets[xi_coordinate];
														while (xi_coordinate<temp_int)
														{
															face_to_element[temp_int]= -1;
															temp_int += linked_offsets[temp_int];
														}
														while (temp_int<xi_coordinate)
														{
															face_to_element[temp_int+1]= -1;
															temp_int += linked_offsets[temp_int];
														}
													}
													else
													{
														if (linked_coordinate<xi_coordinate)
														{
															face_to_element[linked_coordinate+1]=1;
														}
														else
														{
															if (xi_coordinate<linked_coordinate)
															{
																face_to_element[linked_coordinate]=1;
															}
														}
														linked_coordinate +=
															linked_offsets[linked_coordinate];
													}
												}
												else
												{
													linked_coordinate +=
														linked_offsets[linked_coordinate];
												}
											}
											else
											{
												if (j!=xi_coordinate)
												{
													CALCULATE_K_SET();
													face_to_element[k_set]=1;
													k++;
													if (k>=dimension)
													{
														k=1;
													}
													if (simplex_dimension<dimension)
													{
														/* make sure that k is not in the simplex containing
															xi_coordinate */
														i=1;
														do
														{
															if (k>xi_coordinate)
															{
																i=k;
															}
															else
															{
																i=k-1;
															}
															if (0==linked_offsets[i])
															{
																i=0;
															}
															else
															{
																l=i;
																do
																{
																	l += linked_offsets[l];
																} while ((l!=i)&&(l!=xi_coordinate));
																if (l==xi_coordinate)
																{
																	k++;
																	if (k>=dimension)
																	{
																		k=1;
																	}
																	i=1;
																}
																else
																{
																	i=0;
																}
															}
														} while (1==i);
													}
												}
											}
											face_to_element += dimension;
											type_entry += dimension-j;
										}
										if (linked_offsets[xi_coordinate]<0)
										{
											/* last simplex dimension */
											linked_offset=simplex_dimension;
											simplex_coordinate=xi_coordinate+
												linked_offsets[xi_coordinate];
											linked_coordinate=simplex_coordinate;
											k=xi_coordinate+1;
											if (k>=dimension)
											{
												k=1;
											}
											if (simplex_dimension<dimension)
											{
												/* make sure that k is not in the simplex containing
													xi_coordinate */
												i=1;
												do
												{
													if (k>xi_coordinate)
													{
														i=k;
													}
													else
													{
														i=k-1;
													}
													if (0==linked_offsets[i])
													{
														i=0;
													}
													else
													{
														l=i;
														do
														{
															l += linked_offsets[l];
														} while ((l!=i)&&(l!=xi_coordinate));
														if (l==xi_coordinate)
														{
															k++;
															if (k>=dimension)
															{
																k=1;
															}
															i=1;
														}
														else
														{
															i=0;
														}
													}
												} while (1==i);
											}
											type_entry=shape->type;
											for (j=0;j<dimension;j++)
											{
												if (j==simplex_coordinate)
												{
													simplex_coordinate +=
														linked_offsets[simplex_coordinate];
													linked_offset--;
													if (0==linked_offset)
													{
														face_to_element[0]=1;
														temp_int=xi_coordinate+
															linked_offsets[xi_coordinate];
														while (xi_coordinate<temp_int)
														{
															face_to_element[temp_int]= -1;
															temp_int += linked_offsets[temp_int];
														}
														while (temp_int<xi_coordinate)
														{
															face_to_element[temp_int+1]= -1;
															temp_int += linked_offsets[temp_int];
														}
													}
													else
													{
														if (linked_coordinate<xi_coordinate)
														{
															face_to_element[linked_coordinate+1]=1;
														}
														else
														{
															if (xi_coordinate<linked_coordinate)
															{
																face_to_element[linked_coordinate]=1;
															}
														}
														linked_coordinate +=
															linked_offsets[linked_coordinate];
													}
												}
												else
												{
													if (j!=xi_coordinate)
													{
														CALCULATE_K_SET();
														face_to_element[k_set]=1;
														k++;
														if (k>=dimension)
														{
															k=1;
														}
														if (simplex_dimension<dimension)
														{
															/* make sure that k is not in the simplex
																containing xi_coordinate */
															i=1;
															do
															{
																if (k>xi_coordinate)
																{
																	i=k;
																}
																else
																{
																	i=k-1;
																}
																if (0==linked_offsets[i])
																{
																	i=0;
																}
																else
																{
																	l=i;
																	do
																	{
																		l += linked_offsets[l];
																	} while ((l!=i)&&(l!=xi_coordinate));
																	if (l==xi_coordinate)
																	{
																		k++;
																		if (k>=dimension)
																		{
																			k=1;
																		}
																		i=1;
																	}
																	else
																	{
																		i=0;
																	}
																}
															} while (1==i);
														}
													}
												}
												face_to_element += dimension;
												type_entry += dimension-j;
											}
										}
									} break;
								}
								shape_type += dimension-xi_coordinate;
							}
#if defined (DEBUG_CODE)
							/*???debug */
							face_to_element=shape->face_to_element;
							face=shape->faces;
							for (i=1;i<=number_of_faces;i++)
							{
								printf("face %d %d:\n",i,*face);
								face++;
								for (j=dimension;j>0;j--)
								{
									for (k=dimension;k>0;k--)
									{
										printf(" %g",*face_to_element);
										face_to_element++;
									}
									printf("\n");
								}
							}
#endif /* defined (DEBUG_CODE) */
						}
						else
						{
							display_message(ERROR_MESSAGE,
							"CREATE(FE_element_shape).  Could not allocate memory for faces");
							DEALLOCATE(shape);
							DEALLOCATE(shape_type);
							DEALLOCATE(face);
						}
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"CREATE(FE_element_shape).  Invalid shape type");
						DEALLOCATE(shape->type);
						DEALLOCATE(shape);
					}
				}
			}
			else
			{
				/* unspecified shape creator ends up here legitimately */
				if (type || (!shape))
				{
					display_message(ERROR_MESSAGE,
						"CREATE(FE_element_shape).  Could not allocate memory for shape");
					DEALLOCATE(shape);
					DEALLOCATE(shape_type);
				}
			}
			if (shape)
			{
#if defined (DEBUG_CODE)
				printf("Created shape: %p\n", shape);
				printf("  shape->dimension: %d\n", shape->dimension);
				printf("  shape->type:");
				for (i = 0 ; i < (shape->dimension*(shape->dimension+1))/2 ; i++)
				{
					printf(" %d", shape->type[i]);
				}
				printf("\n");
				printf("  shape->number_of_faces: %d\n", shape->number_of_faces);
				printf("  shape->faces:");
				for (i = 0 ; i < shape->number_of_faces ; i++)
				{
					printf(" %d (", shape->faces[i]);
					for (j = 0 ; j < shape->dimension ; j++)
					{
						printf("%f,", shape->face_normals[j + i * shape->dimension]);
					}
					printf(")\n");
				}
#endif /* defined (DEBUG_CODE) */
				/* add the shape to the list of all shapes */
				if (!ADD_OBJECT_TO_LIST(FE_element_shape)(shape,
					FE_region_get_FE_element_shape_list(fe_region)))
				{
					display_message(ERROR_MESSAGE, "CREATE(FE_element_shape).  "
						"Could not add shape to the list of all shapes");
					DEALLOCATE(shape->type);
					DEALLOCATE(shape->faces);
					DEALLOCATE(shape->face_to_element);
					DEALLOCATE(shape);
				}
			}
			if (linked_offsets)
			{
				DEALLOCATE(linked_offsets);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CREATE(FE_element_shape).  Invalid argument");
	}
/*???debug */
/*printf("leave CREATE(FE_element_shape)\n");*/
	LEAVE;

	return (shape);
} /* CREATE(FE_element_shape) */

int DESTROY(FE_element_shape)(struct FE_element_shape **element_shape_address)
/*******************************************************************************
LAST MODIFIED : 24 September 1995

DESCRIPTION :
Remove the shape from the list of all shapes.  Free the memory for the shape and
sets <*element_shape_address> to NULL.
==============================================================================*/
{
	int return_code;
	struct FE_element_shape *shape;

	ENTER(DESTROY(FE_element_shape));
	/* check the arguments */
	if ((element_shape_address)&&(shape= *element_shape_address))
	{
		if (0==shape->access_count)
		{
			DEALLOCATE(shape->type);
			DEALLOCATE(shape->faces);
			DEALLOCATE(shape->face_normals);
			DEALLOCATE(shape->face_to_element);
			DEALLOCATE(*element_shape_address);
			return_code=1;
		}
		else
		{
			return_code=1;
			*element_shape_address=(struct FE_element_shape *)NULL;
		}
	}
	else
	{
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(FE_element_shape) */

DECLARE_OBJECT_FUNCTIONS(FE_element_shape)
DECLARE_LIST_FUNCTIONS(FE_element_shape)

#if (MAXIMUM_ELEMENT_XI_DIMENSIONS > 3)
#error Resize following define.
#endif
#define INT_SHAPE_TYPE_ARRAY_LENGTH 6

struct cmzn_element_shape_type_map
{
	enum cmzn_element_shape_type shape_type;
	const int dimension;
	const int int_shape_type_array[INT_SHAPE_TYPE_ARRAY_LENGTH];
};

const struct cmzn_element_shape_type_map standard_shape_type_maps[] =
{
	{ CMZN_ELEMENT_SHAPE_TYPE_LINE,        1, { LINE_SHAPE, 0, 0, 0, 0, 0 } },
	{ CMZN_ELEMENT_SHAPE_TYPE_SQUARE,      2, { LINE_SHAPE, 0, LINE_SHAPE, 0, 0, 0 } },
	{ CMZN_ELEMENT_SHAPE_TYPE_TRIANGLE,    2, { SIMPLEX_SHAPE, 1, SIMPLEX_SHAPE, 0, 0, 0 } },
	{ CMZN_ELEMENT_SHAPE_TYPE_CUBE,        3, { LINE_SHAPE, 0, 0, LINE_SHAPE, 0, LINE_SHAPE } },
	{ CMZN_ELEMENT_SHAPE_TYPE_TETRAHEDRON, 3, { SIMPLEX_SHAPE, 1, 1, SIMPLEX_SHAPE, 1, SIMPLEX_SHAPE } },
	{ CMZN_ELEMENT_SHAPE_TYPE_WEDGE12,     3, { SIMPLEX_SHAPE, 1, 0, SIMPLEX_SHAPE, 0, LINE_SHAPE } },
	{ CMZN_ELEMENT_SHAPE_TYPE_WEDGE13,     3, { SIMPLEX_SHAPE, 0, 1, LINE_SHAPE, 0, SIMPLEX_SHAPE } },
	{ CMZN_ELEMENT_SHAPE_TYPE_WEDGE23,     3, { LINE_SHAPE, 0, 0, SIMPLEX_SHAPE, 1, SIMPLEX_SHAPE } }
};

const int standard_shape_type_maps_length = sizeof(standard_shape_type_maps) / sizeof(struct cmzn_element_shape_type_map);

struct FE_element_shape *FE_element_shape_create_simple_type(
	struct FE_region *fe_region, enum cmzn_element_shape_type shape_type)
{
	struct FE_element_shape *fe_element_shape;
	int i;

	fe_element_shape = NULL;
	if (fe_region)
	{
		for (i = 0; i < standard_shape_type_maps_length; i++)
		{
			if (standard_shape_type_maps[i].shape_type == shape_type)
			{
				fe_element_shape = ACCESS(FE_element_shape)(
					CREATE(FE_element_shape)(standard_shape_type_maps[i].dimension,
						standard_shape_type_maps[i].int_shape_type_array, fe_region));
				break;
			}
		}
	}
	if (!fe_element_shape)
	{
		display_message(ERROR_MESSAGE,
			"FE_element_shape_create_simple_type.  Invalid arguments");
	}
	return fe_element_shape;
}

int FE_element_shape_get_number_of_faces(const FE_element_shape *element_shape)
{
	if (element_shape)
		return element_shape->number_of_faces;
	return 0;
}

enum cmzn_element_shape_type FE_element_shape_get_simple_type(
	struct FE_element_shape *element_shape)
{
	int dimension, i, j, length;
	enum cmzn_element_shape_type shape_type;

	shape_type = CMZN_ELEMENT_SHAPE_TYPE_INVALID;
	if (element_shape && element_shape->type)
	{
		dimension = element_shape->dimension;
		length = dimension*(dimension + 1)/2;
		for (i = 0; i < standard_shape_type_maps_length; i++)
		{
			if (standard_shape_type_maps[i].dimension == dimension)
			{
				for (j = 0; j < length; j++)
				{
					if (element_shape->type[j] != standard_shape_type_maps[i].int_shape_type_array[j])
						break;
				}
				if (j == length)
				{
					shape_type = standard_shape_type_maps[i].shape_type;
					break;
				}
			}
		}
	}
	return shape_type;
}

// Currently limited to handling one polygon or one simplex. Will have to
// be rewritten for 4-D and above elements.
char *FE_element_shape_get_EX_description(struct FE_element_shape *element_shape)
{
	if (!element_shape)
		return 0;
	char *description = 0;
	int next_xi_number, number_of_polygon_vertices;
	int error = 0;
	enum FE_element_shape_type shape_type;
	int linked_dimensions = 0;
	for (int xi_number = 0; xi_number < element_shape->dimension; xi_number++)
	{
		if (xi_number > 0)
		{
			append_string(&description, "*", &error);
		}
		if (get_FE_element_shape_xi_shape_type(element_shape, xi_number, &shape_type))
		{
			switch (shape_type)
			{
				case LINE_SHAPE:
				{
					append_string(&description, "line", &error);
				} break;
				case POLYGON_SHAPE:
				{
					/* logic currently limited to one polygon in shape - ok up to 3D */
					append_string(&description, "polygon", &error);
					if (0 == linked_dimensions)
					{
						if ((!get_FE_element_shape_next_linked_xi_number(element_shape,
							xi_number, &next_xi_number, &number_of_polygon_vertices)) ||
							(next_xi_number <= 0))
						{
							display_message(ERROR_MESSAGE, "write_FE_element_shape.  "
								"No second linked dimensions in polygon");
							DEALLOCATE(description);
							return 0;
						}
						else if (number_of_polygon_vertices < 3)
						{
							display_message(ERROR_MESSAGE, "write_FE_element_shape.  "
								"Invalid number of vertices in polygon: %d",
								number_of_polygon_vertices);
							DEALLOCATE(description);
							return 0;
						}
						else
						{
							char tmp_string[50];
							sprintf(tmp_string, "(%d;%d)", number_of_polygon_vertices, next_xi_number + 1);
							append_string(&description, tmp_string, &error);
						}
					}
					linked_dimensions++;
					if (2 < linked_dimensions)
					{
						display_message(ERROR_MESSAGE, "write_FE_element_shape.  "
							"Too many linked dimensions in polygon");
						DEALLOCATE(description);
						return 0;
					}
				} break;
				case SIMPLEX_SHAPE:
				{
					/* logic currently limited to one simplex in shape - OK up to 3D */
					append_string(&description, "simplex", &error);
					if (0 == linked_dimensions)
					{
						char tmp_string[50];
						linked_dimensions++;
						/* for first linked simplex dimension write (N1[;N2]) where N1 is
							 first linked dimension, N2 is the second - for tetrahedra */
						append_string(&description, "(", &error);
						next_xi_number = xi_number;
						while (next_xi_number < element_shape->dimension)
						{
							if (get_FE_element_shape_next_linked_xi_number(element_shape,
								next_xi_number, &next_xi_number, &number_of_polygon_vertices))
							{
								if (0 < next_xi_number)
								{
									linked_dimensions++;
									if (2 < linked_dimensions)
									{
										append_string(&description, ";", &error);
									}
									sprintf(tmp_string, "%d", next_xi_number + 1);
									append_string(&description, tmp_string, &error);
								}
								else
								{
									next_xi_number = element_shape->dimension;
								}
							}
							else
							{
								display_message(ERROR_MESSAGE, "write_FE_element_shape.  "
									"Could not get next linked xi number for simplex");
								DEALLOCATE(description);
								return 0;
							}
						}
						append_string(&description, ")", &error);
						if (1 == linked_dimensions)
						{
							display_message(ERROR_MESSAGE,"write_FE_element_shape.  "
								"Too few linked dimensions in simplex");
							DEALLOCATE(description);
							return 0;
						}
					}
				} break;
				default:
				{
					display_message(ERROR_MESSAGE,
						"write_FE_element_shape.  Unknown shape type");
					DEALLOCATE(description);
					return 0;
				} break;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"write_FE_element_shape.  Could not get shape type");
			DEALLOCATE(description);
			return 0;
		}
	}
	return description;
}

struct FE_element_shape *FE_element_shape_create_unspecified(
	struct FE_region *fe_region, int dimension)
{
	struct FE_element_shape *fe_element_shape = NULL;
	if (fe_region && (1 <= dimension) && (dimension <= 3))
	{
		fe_element_shape = ACCESS(FE_element_shape)(
			CREATE(FE_element_shape)(dimension, (int *)NULL, fe_region));
	}
	if (!fe_element_shape)
	{
		display_message(ERROR_MESSAGE,
			"FE_element_shape_create_simple_type.  Invalid arguments");
	}
	return fe_element_shape;
}

bool FE_element_shape_is_line(struct FE_element_shape *element_shape)
{
	if (element_shape)
	{
		return (element_shape->type[0] == LINE_SHAPE) &&
			((1 == element_shape->dimension) ||
			((2 == element_shape->dimension) &&
				(element_shape->type[2] == LINE_SHAPE)) ||
			((3 == element_shape->dimension) &&
				(element_shape->type[3] == LINE_SHAPE) &&
				(element_shape->type[5] == LINE_SHAPE)));
	}
	display_message(ERROR_MESSAGE, "FE_element_shape_is_line.  Missing shape");
	return false;
}

bool FE_element_shape_is_triangle(struct FE_element_shape *element_shape)
{
	if (element_shape)
	{
		return (element_shape->dimension == 2)
			&& (element_shape->type[0] == SIMPLEX_SHAPE)
			&& (element_shape->type[2] == SIMPLEX_SHAPE);
	}
	display_message(ERROR_MESSAGE, "FE_element_shape_is_triangle.  Missing shape");
	return false;
}

struct FE_element_shape *get_FE_element_shape_of_face(
	const FE_element_shape *shape,int face_number, struct FE_region *fe_region)
/*******************************************************************************
LAST MODIFIED : 7 July 2003

DESCRIPTION :
From the parent <shape> returns the FE_element_shape for its face <face_number>.
The <shape> must be of dimension 2 or 3. Faces of 2-D elements are always lines.
==============================================================================*/
{
	int face_code,face_type[3],line_xi_bit,number_of_polygon_vertices,
		polygon_face;
	struct FE_element_shape *face_shape;

	ENTER(get_FE_element_shape_of_face);
	face_shape = (struct FE_element_shape *)NULL;
	if (shape&&(0<=face_number)&&(face_number<shape->number_of_faces)&&
		shape->type&&shape->faces)
	{
		switch (shape->dimension)
		{
			case 2:
			{
				/* faces of 2-D shapes are always lines */
				face_type[0]=LINE_SHAPE;
				face_shape=CREATE(FE_element_shape)(/*dimension*/1,face_type,
					fe_region);
			} break;
			case 3:
			{
				/* if all shape types on main diagonal are the same, then it is either
					 a square block or a tetrahedron. Sub-triangle of shape->type is then
					 valid for any of its faces */
				if (((shape->type)[0]==(shape->type)[3])&&
					((shape->type)[0]==(shape->type)[5]))
				{
					face_shape=CREATE(FE_element_shape)(/*dimension*/2,shape->type+3,
						fe_region);
				}
				else if ((POLYGON_SHAPE==(shape->type)[0])||
					(POLYGON_SHAPE==(shape->type)[3])||
					(POLYGON_SHAPE==(shape->type)[5]))
				{
					/* 2 out of 3 xi directions must be linked in a polygon: hence need
						to determine if face is a polygon or a square */
					face_code=shape->faces[face_number];
					/* work out number_of_polygon_vertices */
					polygon_face=0;
					if (POLYGON_SHAPE==(shape->type)[0])
					{
						if (POLYGON_SHAPE==(shape->type)[3])
						{
							number_of_polygon_vertices=(shape->type)[1];
							/* polygon-polygon-line */
							if (face_code>=number_of_polygon_vertices*2)
							{
								polygon_face=1;
							}
						}
						else
						{
							number_of_polygon_vertices=(shape->type)[2];
							/* polygon-line-polygon */
							if (face_code>=number_of_polygon_vertices*2)
							{
								polygon_face=1;
							}
						}
					}
					else
					{
						number_of_polygon_vertices=(shape->type)[4];
						/* line-polygon-polygon */
						if (face_code<number_of_polygon_vertices*2)
						{
							polygon_face=1;
						}
					}
					if (polygon_face)
					{
						/* face has a polygon shape */
						face_type[0]=POLYGON_SHAPE;
						face_type[1]=number_of_polygon_vertices;
						face_type[2]=POLYGON_SHAPE;
					}
					else
					{
						/* face has a square shape */
						face_type[0]=LINE_SHAPE;
						face_type[1]=0;
						face_type[2]=LINE_SHAPE;
					}
					face_shape=CREATE(FE_element_shape)(/*dimension*/2,face_type,
						fe_region);
				}
				else if ((SIMPLEX_SHAPE==(shape->type)[0])||
					(SIMPLEX_SHAPE==(shape->type)[3])||
					(SIMPLEX_SHAPE==(shape->type)[5]))
				{
					/* 2 out of 3 xi directions must be linked in a triangle: hence need
						 to determine if face is a triangle or a square */
					face_code=shape->faces[face_number];
					/* work out which xi direction is not SIMPLEX_SHAPE (=LINE_SHAPE) */
					if (SIMPLEX_SHAPE==(shape->type)[0])
					{
						if (SIMPLEX_SHAPE==(shape->type)[3])
						{
							line_xi_bit=8;
						}
						else
						{
							line_xi_bit=4;
						}
					}
					else
					{
						line_xi_bit=2;
					}
					if (face_code & line_xi_bit)
					{
						/* face has a triangle shape */
						face_type[0]=SIMPLEX_SHAPE;
						face_type[1]=1;
						face_type[2]=SIMPLEX_SHAPE;
					}
					else
					{
						/* face has a square shape */
						face_type[0]=LINE_SHAPE;
						face_type[1]=0;
						face_type[2]=LINE_SHAPE;
					}
					face_shape=CREATE(FE_element_shape)(/*dimension*/2,face_type,
						fe_region);
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"get_FE_element_shape_of_face.  Unknown element shape");
				}
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"get_FE_element_shape_of_face.  Invalid dimension");
			} break;
		}
		if (!face_shape)
		{
			display_message(ERROR_MESSAGE,"get_FE_element_shape_of_face.  Failed");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"get_FE_element_shape_of_face.  Invalid argument(s)");
	}
	LEAVE;

	return (face_shape);
} /* get_FE_element_shape_of_face */

int get_FE_element_shape_dimension(struct FE_element_shape *element_shape)
{
	if (element_shape)
		return element_shape->dimension;
	return 0;
}

const FE_value *get_FE_element_shape_face_to_element(
	struct FE_element_shape *element_shape, int face_number)
{
	if (element_shape && (0 <= face_number) &&
		(face_number < element_shape->number_of_faces))
	{
		return element_shape->face_to_element +
			(face_number*element_shape->dimension*element_shape->dimension);
	}
	display_message(ERROR_MESSAGE, "get_FE_element_shape_face_to_element.  Invalid argument(s)");
	return 0;
}

int FE_element_shape_find_face_number_for_xi(struct FE_element_shape *shape,
	FE_value *xi, int *face_number)
/*******************************************************************************
LAST MODIFIED : 11 June 1999

DESCRIPTION :
This function checks to see if the given <xi> location (of dimension
<shape>->dimension) specifys a location on a face.  If it does then the function
returns 1 and <face_number> is set.  Otherwise the function returns 0.
SAB Doesn't work for polygons at the moment.
==============================================================================*/
{
	int bit, i, j, return_code;
	FE_value sum;

	ENTER(FE_element_shape_find_face_number_for_xi);

	if (shape&&face_number)
	{
		return_code = 0;
		for (i = 0 ; (!return_code) && (i < shape->number_of_faces) ; i++)
		{
			sum = 0.0;
			bit = 2;
			for (j = 0 ; j < shape->dimension ; j++)
			{
				if (shape->faces[i] & bit)
				{
					sum += xi[j];
				}
				bit *= 2;
			}
			if (shape->faces[i] & 1)
			{
				if (sum >= 1.0)
				{
					*face_number = i;
					return_code = 1;
				}
			}
			else
			{
				if (sum <= 0.0)
				{
					*face_number = i;
					return_code = 1;
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"find_face_number_of_face_type.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_element_shape_find_face_number_for_xi */

int get_FE_element_shape_xi_linkage_number(
	struct FE_element_shape *element_shape, int xi_number1, int xi_number2,
	int *xi_linkage_number_address)
/*******************************************************************************
LAST MODIFIED : 13 March 2003

DESCRIPTION :
Returns a number indicating how the dimension <xi_number1> and <xi_number2> are
linked in <element_shape>.
If they are linked in a simplex, a non-zero return indicates they are linked.
If they are linked in a polygon, the returned number is the number of sides in
the polygon.
A value of zero indicates the dimensions are not linked.
Note the first xi_number is 0.
==============================================================================*/
{
	int i, offset, return_code, tmp;

	ENTER(get_FE_element_shape_xi_linkage_number);
	if (element_shape && element_shape->type && xi_linkage_number_address &&
		(0 <= xi_number1) && (xi_number1 < element_shape->dimension) &&
		(0 <= xi_number2) && (xi_number2 < element_shape->dimension) &&
		(xi_number1 != xi_number2))
	{
		/* make sure xi_number1 < xi_number2 */
		if (xi_number2 < xi_number1)
		{
			tmp = xi_number1;
			xi_number1 = xi_number2;
			xi_number2 = tmp;
		}
		offset = 0;
		for (i = 0; i < xi_number1; i++)
		{
			offset += element_shape->dimension - i;
		}
		*xi_linkage_number_address =
			element_shape->type[offset + xi_number2 - xi_number1];
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"get_FE_element_shape_xi_linkage_number.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* get_FE_element_shape_xi_linkage_number */

int get_FE_element_shape_xi_shape_type(struct FE_element_shape *element_shape,
	int xi_number, enum FE_element_shape_type *shape_type_address)
/*******************************************************************************
LAST MODIFIED : 25 February 2003

DESCRIPTION :
Returns the shape type of <element_shape> on <xi_number> -- on main diagonal of
type array. The first xi_number is 0.
==============================================================================*/
{
	int i, offset, return_code;

	ENTER(get_FE_element_shape_xi_shape_type);
	if (element_shape && element_shape->type && (0 <= xi_number) &&
		(xi_number < element_shape->dimension) && shape_type_address)
	{
		offset = 0;
		for (i = 0; i < xi_number; i++)
		{
			offset += element_shape->dimension - i;
		}
		*shape_type_address =
			(enum FE_element_shape_type)(element_shape->type[offset]);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"get_FE_element_shape_xi_shape_type.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* get_FE_element_shape_xi_shape_type */

int get_FE_element_shape_next_linked_xi_number(
	struct FE_element_shape *element_shape, int xi_number,
	int *next_xi_number_address, int *xi_link_number_address)
/*******************************************************************************
LAST MODIFIED : 25 February 2003

DESCRIPTION :
Returns in <next_xi_number_address> the next xi number higher than <xi_number>
which is linked in shape with it, plus in <xi_link_number_address> the number
denoting how it is linked; currently used only for polygon shapes to denote the
number of polygon sides.
If there is no remaining linked dimension, 0 is returned in both addresses.
<xi_number> is from 0 to one less than the shape dimension.
Also checks that the linked xi numbers have the same shape type.
==============================================================================*/
{
	enum FE_element_shape_type shape_type;
	int i, limit, offset, return_code;

	ENTER(get_FE_element_shape_next_linked_xi_number);
	if (element_shape && element_shape->type &&
		(0 <= xi_number) && (xi_number < element_shape->dimension) &&
		next_xi_number_address && xi_link_number_address)
	{
		return_code = 1;
		offset = 0;
		for (i = 0; i < xi_number; i++)
		{
			offset += element_shape->dimension - i;
		}
		shape_type = (enum FE_element_shape_type)(element_shape->type[offset]);
		limit = element_shape->dimension - xi_number;
		offset++;
		for (i = 1; (i < limit) && (0 == element_shape->type[offset]); i++)
		{
			offset++;
		}
		if (i < limit)
		{
			*next_xi_number_address = i + xi_number;
			*xi_link_number_address = element_shape->type[offset];
			/* finally check the shape type matches */
			offset = 0;
			for (i = 0; i < *next_xi_number_address; i++)
			{
				offset += element_shape->dimension - i;
			}
			if ((enum FE_element_shape_type)element_shape->type[offset] != shape_type)
			{
				display_message(ERROR_MESSAGE,
					"get_FE_element_shape_next_linked_xi_number.  "
					"Shape has linked xi directions with different shape type");
				return_code = 0;
			}
		}
		else
		{
			*next_xi_number_address = 0;
			*xi_link_number_address = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"get_FE_element_shape_next_linked_xi_number.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* get_FE_element_shape_next_linked_xi_number */

int FE_element_shape_limit_xi_to_element(struct FE_element_shape *shape,
	FE_value *xi, FE_value tolerance)
/*******************************************************************************
LAST MODIFIED : 12 March 2003

DESCRIPTION :
Checks that the <xi> location is valid for elements with <shape>.
The <tolerance> allows the location to go slightly outside.  If the values for
<xi> location are further than <tolerance> outside the element then the values
are modified to put it on the nearest face.
==============================================================================*/
{
	int i, return_code, simplex_dimensions,
		simplex_direction[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	FE_value delta;

	ENTER(FE_element_shape_limit_xi_to_element);
	if (shape && xi)
	{
		return_code = 1;
		/* determine whether the element is simplex to limit xi space */
		simplex_dimensions = 0;
		switch (shape->dimension)
		{
			case 2:
			{
				if (SIMPLEX_SHAPE == shape->type[0])
				{
					simplex_dimensions = 2;
					simplex_direction[0] = 0;
					simplex_direction[1] = 1;
				}
			} break;
			case 3:
			{
				if (SIMPLEX_SHAPE == shape->type[0])
				{
					if (LINE_SHAPE == shape->type[3])
					{
						simplex_dimensions = 2;
						simplex_direction[0] = 0;
						simplex_direction[1] = 2;
					}
					else if (LINE_SHAPE == shape->type[5])
					{
						simplex_dimensions = 2;
						simplex_direction[0] = 0;
						simplex_direction[1] = 1;
					}
					else
					{
						/* tetrahedron */
						simplex_dimensions = 3;
						simplex_direction[0] = 0;
						simplex_direction[1] = 1;
						simplex_direction[2] = 2;
					}
				}
				else if (SIMPLEX_SHAPE == shape->type[3])
				{
					simplex_dimensions = 2;
					simplex_direction[0] = 1;
					simplex_direction[1] = 2;
				}
			} break;
		}
		/* keep xi within simplex bounds plus tolerance */
		if (simplex_dimensions)
		{
			/* calculate distance out of element in xi space */
			delta = -1.0 - tolerance;
			for (i = 0; i < simplex_dimensions; i++)
			{
				delta += xi[simplex_direction[i]];
			}
			if (delta > 0.0)
			{
				/* subtract delta equally from all directions */
				delta /= simplex_dimensions;
				for (i = 0; i < simplex_dimensions; i++)
				{
					xi[simplex_direction[i]] -= delta;
				}
			}
		}
		/* keep xi within 0.0 to 1.0 bounds plus tolerance */
		for (i = 0; i < shape->dimension; i++)
		{
			if (xi[i] < -tolerance)
			{
				xi[i] = -tolerance;
			}
			else if (xi[i] > 1.0 + tolerance)
			{
				xi[i] = 1.0 + tolerance;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"FE_element_shape_limit_xi_to_element.  "
			"Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_element_shape_limit_xi_to_element */

int FE_element_shape_calculate_face_xi_normal(struct FE_element_shape *shape,
	int face_number, FE_value *normal)
{
	double determinant, *matrix_double, norm;
	FE_value *face_to_element, sign;
	int i, j, k, *pivot_index, return_code;

	ENTER(face_calculate_xi_normal);
	const int dimension = get_FE_element_shape_dimension(shape);
	if ((0 < dimension) && (0<=face_number) && (face_number < shape->number_of_faces) && normal)
	{
		return_code = 1;
		if (1==dimension)
		{
			normal[0] = (FE_value)1;
		}
		else
		{
			/* working storage */
			ALLOCATE(matrix_double, double, (dimension-1)*(dimension-1));
			ALLOCATE(pivot_index, int, dimension-1);
			if (matrix_double&&pivot_index)
			{
				sign = (FE_value)1;
				face_to_element = (shape->face_to_element)+
					(face_number*dimension*dimension);
				i = 0;
				while (return_code&&(i<dimension))
				{
					/* calculate the determinant of face_to_element excluding column 1 and
						row i+1 */
					for (j = 0; j<i; j++)
					{
						for (k = 1; k<dimension; k++)
						{
							matrix_double[j*(dimension-1)+(k-1)] =
								(double)face_to_element[j*dimension+k];
						}
					}
					for (j = i+1; j<dimension; j++)
					{
						for (k = 1; k<dimension; k++)
						{
							matrix_double[(j-1)*(dimension-1)+(k-1)] =
								(double)face_to_element[j*dimension+k];
						}
					}
					if (LU_decompose(dimension-1, matrix_double, pivot_index,
						&determinant,/*singular_tolerance*/1.0e-12))
					{
						for (j = 0; j<dimension-1; j++)
						{
							determinant *= matrix_double[j*(dimension-1)+j];
						}
						normal[i] = sign*(FE_value)determinant;
						sign = -sign;
					}
					else
					{
						normal[i] = 0.0;
						sign = -sign;
					}
					i++;
				}
				if (return_code)
				{
					/* normalize face_normal */
					norm = (double)0;
					for (i = 0; i<dimension; i++)
					{
						norm += (double)(normal[i])*(double)(normal[i]);
					}
					if (0<norm)
					{
						norm = sqrt(norm);
						for (i = 0; i<dimension; i++)
						{
							normal[i] /= (FE_value)norm;
						}
					}
					else
					{
						return_code = 0;
					}
				}
			}
			else
			{
				display_message(ERROR_MESSAGE, "face_calculate_xi_normal.  "
					"Could not allocate matrix_double (%p) or pivot_index (%p)",
					matrix_double, pivot_index);
				return_code = 0;
			}
			DEALLOCATE(pivot_index);
			DEALLOCATE(matrix_double);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE, "face_calculate_xi_normal.  "
			"Invalid argument(s).  %p %d %d %p", shape, dimension, face_number, normal);
		return_code = 0;
	}
	LEAVE;

	return (return_code);
}

bool FE_element_shape_face_has_inward_normal(struct FE_element_shape *shape,
	int face_number)
{
	if (shape && (3 == shape->dimension) &&
		(0 <= face_number) && (face_number <= shape->number_of_faces))
	{
		FE_value *outward_face_normal;
		FE_value face_xi1[3], face_xi2[3], actual_face_normal[3];
		FE_value *face_to_element = shape->face_to_element +
			face_number*shape->dimension*shape->dimension;
		face_xi1[0] = face_to_element[1];
		face_xi1[1] = face_to_element[4];
		face_xi1[2] = face_to_element[7];
		face_xi2[0] = face_to_element[2];
		face_xi2[1] = face_to_element[5];
		face_xi2[2] = face_to_element[8];
		cross_product_FE_value_vector3(face_xi1, face_xi2, actual_face_normal);
		outward_face_normal = shape->face_normals + face_number*shape->dimension;
		FE_value result = actual_face_normal[0]*outward_face_normal[0] +
			actual_face_normal[1]*outward_face_normal[1] +
			actual_face_normal[2]*outward_face_normal[2];
		if (result < 0.0)
			return true;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_element_shape_face_has_inward_normal.  Invalid argument(s)");
	}
	return false;
}

/**
 * Adds the <increment> to <xi>.  If this moves <xi> outside of the shape, then
 * the step is limited to take <xi> to the boundary, <face_number> is set to be
 * the limiting face, <fraction> is updated with the fraction of the <increment>
 * actually used, the <increment> is updated to contain the part not used,
 * the <xi_face> are calculated for that face and the <xi> are changed to be
 * on the boundary of the shape.
 */
int FE_element_shape_xi_increment(struct FE_element_shape *shape,
	FE_value *xi, FE_value *increment, FE_value *step_size,
	int *face_number_address, FE_value *xi_face)
{
	double determinant, step_size_local;
	FE_value *face_to_element;
	int dimension, face_number, i, j, k, return_code;
	/* While we are calculating this in doubles all the xi locations are
		stored in floats and so when we change_element and are numerically just
		outside it can be a value as large as this */
		// ???DB.  Assumes that <xi> is inside
		// ???DB.  Needs reordering inside finite_element
#define SMALL_STEP (1e-4)

	ENTER(FE_element_shape_xi_increment);
	return_code = 0;
	if ((shape) && (0 < (dimension = shape->dimension)) && (dimension <= MAXIMUM_ELEMENT_XI_DIMENSIONS) &&
		xi && increment && face_number_address && xi_face)
	{
		/* working space */
		double face_matrix[MAXIMUM_ELEMENT_XI_DIMENSIONS*MAXIMUM_ELEMENT_XI_DIMENSIONS];
		double face_rhs[MAXIMUM_ELEMENT_XI_DIMENSIONS];
		int pivot_index[MAXIMUM_ELEMENT_XI_DIMENSIONS];
		return_code = 1;
		/* check if it is in the shape */
		face_number = -1;
		*step_size = (FE_value)1;
		face_to_element = shape->face_to_element;
		for (i = 0; i<shape->number_of_faces; i++)
		{
			for (j = 0; j<dimension; j++)
			{
				face_matrix[j*dimension] = (double)(-increment[j]);
				face_rhs[j] = (double)(xi[j]-(*face_to_element));
				face_to_element++;
				for (k = 1; k<dimension; k++)
				{
					face_matrix[j*dimension+k] = (double)(*face_to_element);
					face_to_element++;
				}
			}
			/* Don't terminate if the LU_decompose fails as the matrix is
				probably singular, just implying that we are travelling parallel
				to the face */
			if (LU_decompose(dimension, face_matrix, pivot_index,
				&determinant,/*singular_tolerance*/1.0e-12)&&
				LU_backsubstitute(dimension, face_matrix,
					pivot_index, face_rhs))
			{
				step_size_local = face_rhs[0];
				if ((step_size_local > -SMALL_STEP) && (step_size_local < SMALL_STEP) && ((FE_value)step_size_local<*step_size))
				{
					determinant = 0.0;
					for (j = 0; j < dimension; j++)
					{
						determinant += shape->face_normals[i * dimension + j] *
							increment[j];
					}
					if (determinant > -SMALL_STEP)
					{
						/* We are stepping out of the element so this is a boundary */
						for (j = 1; j<dimension; j++)
						{
							xi_face[j-1] = (FE_value)(face_rhs[j]);
						}
						face_number = i;
						*step_size = (FE_value)step_size_local; /* or set to zero exactly?? */
					}
				}
				else
				{
					if ((0 < step_size_local) && ((FE_value)step_size_local<*step_size))
					{
						for (j = 1; j<dimension; j++)
						{
							xi_face[j-1] = (FE_value)(face_rhs[j]);
						}
						face_number = i;
						*step_size = (FE_value)step_size_local;
					}
				}
			}
		}
		*face_number_address = face_number;
		if (-1==face_number)
		{
			/* inside */
			for (i = 0; i<dimension; i++)
			{
				xi[i] += increment[i];
				increment[i] = (FE_value)0;
			}
		}
		else
		{
			/* not inside */
			for (i = 0; i<dimension; i++)
			{
				/* SAB Could use use face_to_element and face_xi to calculate
					updated xi location instead */
				xi[i] = xi[i] + *step_size * increment[i];
				increment[i] *= (1. - *step_size);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE, "FE_element_shape_xi_increment.  "
			"Invalid argument(s).  %p %p %p %p dimension %d", shape, xi, increment,
			face_number_address, get_FE_element_shape_dimension(shape));
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_element_shape_xi_increment */
