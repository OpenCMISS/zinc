/*******************************************************************************
FILE : finite_element_to_streamlines.c

LAST MODIFIED : 23 May 2001

DESCRIPTION :
Functions for calculating streamlines in finite elements.
???DB.  Put into finite_element_to_graphics_object or split
	finite_element_to_graphics_object further ?
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stdlib.h>
#include <math.h>
#include "computed_field/computed_field.h"
#include "finite_element/finite_element_to_graphics_object.h"
#include "finite_element/finite_element_region.h"
#include "finite_element/finite_element_to_streamlines.h"
#include "general/debug.h"
#include "general/geometry.h"
#include "general/matrix_vector.h"
#include "general/random.h"
#include "graphics/graphics_object.h"
#include "graphics/graphics_object.hpp"
#include "general/message.h"
/* SAB Trying to hide the guts of GT_object and its primitives,
	however the stream point stuff currently messes around in the guts
	of a pointset. */
#include "graphics/graphics_object_private.hpp"

/*
Module types
------------
*/

struct Streampoint
{
	/* The element which the streampoint is in */
	struct FE_element *element;
	/* The position in the element */
	FE_value xi[3];
	/* The index to the point in the pointlist */
	int index;
	/* The location where the pointer to the pointlist is stored,
		this is done with a second reference so that the pointlist
		can be reallocated and shifted around but only needs
		updating in the Pointset object itself, this points to the
		same location */
	Triple **pointlist;
	/* Points to the place in the pointset object which contains this
	streampoints position*/
/*	Triple *position;*/
	/* The Graphics Object to which this streampoint belongs, therefore whenever
		the streampoints position is changed the graphics object's status should be reset */
	gtObject *graphics_object;

	struct Streampoint *next;
}; /* struct Streampoint */

/*
Module functions
----------------
*/

static int calculate_delta_xi(int vector_dimension,FE_value *vector,
	int element_dimension, FE_value *dx_dxi, FE_value *delta_xi)
/*******************************************************************************
LAST MODIFIED : 18 December 2003

DESCRIPTION :
Converts a vector <vector> in world space into xi space <delta_xi> by
calculating the inverse of the Jacobian matrix <dxdxi> and multiplying.
==============================================================================*/
{
	double a[MAXIMUM_ELEMENT_XI_DIMENSIONS*MAXIMUM_ELEMENT_XI_DIMENSIONS],
		b[MAXIMUM_ELEMENT_XI_DIMENSIONS], d;
	int i, index[MAXIMUM_ELEMENT_XI_DIMENSIONS], j, k, return_code;

	ENTER(calculate_delta_xi);
	if (vector && dx_dxi && delta_xi)
	{
		if (element_dimension == vector_dimension)
		{
			/* Solve directly */
			for (i = 0 ; i < element_dimension * element_dimension ; i++)
			{
				a[i] = dx_dxi[i];
			}
			for (i = 0 ; i < element_dimension ; i++)
			{
				b[i] = vector[i];
			}
			i = 0;
			if (LU_decompose(element_dimension, a, index, &d,/*singular_tolerance*/1.0e-12) &&
				LU_backsubstitute(element_dimension, a, index, b))
			{
				for (i = 0 ; i < element_dimension ; i++)
				{
					delta_xi[i] = b[i];
				}
			}
			/* Clear out the other delta_xi's until the rest of the code is
				generalised to an arbitrary number of dimensions */
			for ( ; i < MAXIMUM_ELEMENT_XI_DIMENSIONS ; i++)
			{
				delta_xi[i] = 0.0;
			}
			return_code = 1;
		}
		else if (element_dimension < vector_dimension)
		{
			/* Overdetermined system, solve least squares */
			/* A transpose A x = A transpose b */
			for (i = 0 ; i < element_dimension ; i++)
			{
				for (j = 0 ; j < element_dimension ; j++)
				{
					a[i * element_dimension + j] = 0.0;
					for (k = 0 ; k < vector_dimension ; k++)
					{
						a[i * element_dimension + j] += dx_dxi[i + element_dimension + k] * 
							dx_dxi[j + element_dimension + k];
					}
				}
				b[i] = 0.0;
				for (k = 0 ; k < vector_dimension ; k++)
				{
					b[i] += dx_dxi[i + element_dimension + k] * vector[k];
				}
			}
			i = 0;
			if (LU_decompose(element_dimension, a, index, &d,/*singular_tolerance*/1.0e-12) &&
				LU_backsubstitute(element_dimension, a, index, b))
			{
				for (i = 0 ; i < element_dimension ; i++)
				{
					delta_xi[i] = b[i];
				}
			}
			/* Clear out the other delta_xi's until the rest of the code is
				generalised to an arbitrary number of dimensions */
			for ( ; i < MAXIMUM_ELEMENT_XI_DIMENSIONS ; i++)
			{
				delta_xi[i] = 0.0;
			}
			return_code = 1;
		}
		else
		{
			/* Underdetermined system, not implemented */
			display_message(ERROR_MESSAGE, "calculate_delta_xi.  "
				"Underdetermined systems not implemented.");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"calculate_delta_xi.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* calculate_delta_xi */

static int update_adaptive_imp_euler(cmzn_fieldcache_id field_cache,
	struct Computed_field *coordinate_field,
	struct Computed_field *stream_vector_field,int reverse_track,
	struct FE_element **element,FE_value *xi,
	FE_value *point,FE_value *step_size,
	FE_value *total_stepped, int *keep_tracking)
/*******************************************************************************
LAST MODIFIED : 23 June 2004

DESCRIPTION :
Update the xi coordinates using the <stream_vector_field> with adaptive step
size control and the improved euler method.  The function updates the <total_stepped>.
If <reverse_track> is true, the reverse of vector field is tracked.
==============================================================================*/
{
	int element_dimension,face_number,i,initial_face_number,j,
		number_of_permutations, permutation, return_code,vector_dimension, face_numberB = 0;
	FE_value coordinate_length, coordinate_point_error, coordinate_point_vector, coordinate_tolerance,
		deltaxi[MAXIMUM_ELEMENT_XI_DIMENSIONS],deltaxiA[MAXIMUM_ELEMENT_XI_DIMENSIONS],
		deltaxiC[MAXIMUM_ELEMENT_XI_DIMENSIONS], deltaxiD[MAXIMUM_ELEMENT_XI_DIMENSIONS],
		deltaxiE[MAXIMUM_ELEMENT_XI_DIMENSIONS], 
		dxdxi[MAXIMUM_ELEMENT_XI_DIMENSIONS*MAXIMUM_ELEMENT_XI_DIMENSIONS], error, fraction,
		increment_xi[MAXIMUM_ELEMENT_XI_DIMENSIONS], local_step_size,
		point1[3], point2[3], point3[3], tolerance, 
		vector[MAXIMUM_ELEMENT_XI_DIMENSIONS*MAXIMUM_ELEMENT_XI_DIMENSIONS], 
		xiA[MAXIMUM_ELEMENT_XI_DIMENSIONS], xiB[MAXIMUM_ELEMENT_XI_DIMENSIONS], 
		xiC[MAXIMUM_ELEMENT_XI_DIMENSIONS], xiD[MAXIMUM_ELEMENT_XI_DIMENSIONS],
		xiE[MAXIMUM_ELEMENT_XI_DIMENSIONS], xiF[MAXIMUM_ELEMENT_XI_DIMENSIONS],
		xi_face[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	struct FE_element *initial_element;

	ENTER(update_adaptive_imp_euler);
	/* clear coordinates in case fewer than 3 components */
	FE_element_shape *element_shape = get_FE_element_shape(*element);
	element_dimension = get_FE_element_shape_dimension(element_shape);
	/* It is expected that the coordinate dimension and vector dimension match as
		far as tracking is concerned,
		the vector field may have extra components related to the cross directions
	   which are used to orient stream ribbons and tubes */
	vector_dimension = cmzn_field_get_number_of_components(coordinate_field);
	point1[0]=0.0;
	point1[1]=0.0;
	point1[2]=0.0;
	point2[0]=0.0;
	point2[1]=0.0;
	point2[2]=0.0;
	point3[0]=0.0;
	point3[1]=0.0;
	point3[2]=0.0;
	tolerance=1.0e-4;
	error=1.0;
	coordinate_point_error = 1.0;
	coordinate_tolerance = 1.0e-2;  /* We are tolerating a greater error in the coordinate
											  positions so long as the tracking is valid */
	local_step_size = *step_size;
	return_code = (CMZN_OK == field_cache->setMeshLocation(*element, xi)) &&
		(CMZN_OK == cmzn_field_evaluate_real_with_derivatives(coordinate_field, field_cache,
			vector_dimension, point1, /*number_of_derivatives*/element_dimension, dxdxi)) &&
		(CMZN_OK == cmzn_field_evaluate_real(stream_vector_field, field_cache,
			MAXIMUM_ELEMENT_XI_DIMENSIONS*MAXIMUM_ELEMENT_XI_DIMENSIONS, vector));

	/* Get a length scale estimate */
	coordinate_length = 0;
	for (i = 0 ; i < vector_dimension ; i++)
	{
		for (j = 0 ; j < element_dimension ; j++)
		{
			coordinate_length += dxdxi[i + j * vector_dimension] * 
				dxdxi[i + j * vector_dimension];
		}
	}
	coordinate_length = sqrt(coordinate_length / (FE_value)element_dimension);

	if (reverse_track)
	{
		for (i = 0 ; i < vector_dimension ; i++)
		{
			vector[i] = -vector[i];
		}
	}
	if (return_code)
	{
		return_code=calculate_delta_xi(vector_dimension,vector,element_dimension,
			dxdxi,deltaxi);
	}
	if (return_code)
	{
		if (local_step_size == 0.0)
		{
			/* This is the first step, set the step_size to make the
			 magnitude of deltaxi 0.01 */
			local_step_size=1.0e-2/sqrt(deltaxi[0]*deltaxi[0]+deltaxi[1]*deltaxi[1]
				+deltaxi[2]*deltaxi[2]);
		}
		/* whole step */
		for (i = 0 ; i < element_dimension ; i++)
		{
			xiA[i] = xi[i];
			increment_xi[i] = local_step_size * deltaxi[i];
		}
		return_code = FE_element_shape_xi_increment(element_shape, xiA, increment_xi, &fraction,
		   &face_number, xi_face);
		/* If an element boundary is reached then xiA will automatically be on this boundary.
			We want to go to the same location with the more accurate xiB integration so 
			leave the step size alone and adjust the stepsize after xiB */
		if (return_code)
		{
			return_code = (CMZN_OK == field_cache->setMeshLocation(*element, xiA)) &&
				(CMZN_OK == cmzn_field_evaluate_real_with_derivatives(coordinate_field, field_cache,
					vector_dimension, point3, /*number_of_derivatives*/element_dimension, dxdxi)) &&
				(CMZN_OK == cmzn_field_evaluate_real(stream_vector_field, field_cache,
					MAXIMUM_ELEMENT_XI_DIMENSIONS*MAXIMUM_ELEMENT_XI_DIMENSIONS, vector));
			if (reverse_track)
			{
				for (i = 0 ; i < vector_dimension ; i++)
				{
					vector[i] = -vector[i];
				}
			}
		}
		if (return_code)
		{
			return_code=calculate_delta_xi(vector_dimension,vector,element_dimension,
				dxdxi,deltaxiA);
		}
		if (return_code)
		{
			for (i = 0 ; i < element_dimension ; i++)
			{
				xiB[i] = xi[i];
				increment_xi[i] = local_step_size*(deltaxi[i]+deltaxiA[i])/2.0;
			}
			return_code = FE_element_shape_xi_increment(element_shape, xiB,
				increment_xi, &fraction, &face_number, xi_face);
			face_numberB = face_number;
		}
		if (face_number != -1)
		{
			int onBoundary = 0;
			for (i = 0 ; i < element_dimension ; i++)
			{
				if (xiB[i] == xi[i] && (xiB[i] == 0.0 || xiB[i] == 1.0))
				{
					onBoundary = 1;
					break;
				}
			}
			if (0.0 >= fraction && !onBoundary)
			{
				/* Don't go into the loop, so set error, coordinate_error and xiF */
				error = 0.0;
				coordinate_point_error = 0.0;
				local_step_size = 0.0;
				for (i = 0 ; i < element_dimension ; i++)
				{
					xiF[i] = xiB[i];
				}
			}
			else
			{
				/* Reduce the step to the fraction that was successful plus a bit 
					to make it likely that the more accurate integration below will
					still reach the boundary */
				local_step_size *= (fraction + 0.001);
			}
		}
	}
	while (((error>tolerance)||(coordinate_point_error>coordinate_tolerance))
		&&return_code)
	{
		for (i = 0 ; i < element_dimension ; i++)
		{
			xiC[i] = xi[i];
			increment_xi[i] = local_step_size*deltaxi[i]/2.0;
		}
		return_code = FE_element_shape_xi_increment(element_shape, xiC,
			increment_xi, &fraction, &face_number, xi_face);
		if (return_code)
		{
			return_code = (CMZN_OK == field_cache->setMeshLocation(*element, xiC)) &&
				(CMZN_OK == cmzn_field_evaluate_real_with_derivatives(coordinate_field, field_cache,
					vector_dimension, point2, /*number_of_derivatives*/element_dimension, dxdxi)) &&
				(CMZN_OK == cmzn_field_evaluate_real(stream_vector_field, field_cache,
					MAXIMUM_ELEMENT_XI_DIMENSIONS*MAXIMUM_ELEMENT_XI_DIMENSIONS, vector));
			if (reverse_track)
			{
				for (i = 0 ; i < vector_dimension ; i++)
				{
					vector[i] = -vector[i];
				}
			}
		}
		if (return_code)
		{
			return_code=calculate_delta_xi(vector_dimension,vector,element_dimension,
				dxdxi,deltaxiC);
		}
		if (return_code)
		{
			for (i = 0 ; i < element_dimension ; i++)
			{
				xiD[i] = xi[i];
				increment_xi[i] = local_step_size*(deltaxi[i]+deltaxiC[i])/4.0;
			}
			return_code = FE_element_shape_xi_increment(element_shape, xiD,
				increment_xi, &fraction, &face_number, xi_face);
		}

		/* On boundary but check either it is already on boundary from the beginning */
		/* if a larger step has moved it out of the boundary than use it*/
		if (face_number != -1)
		{
			for (i = 0 ; i < element_dimension ; i++)
			{
				if (xiD[i] == xi[i] && (xiD[i] == 0.0 || xiD[i] == 1.0))
				{
					if (face_numberB == -1)
					{
						for (i = 0 ; i < element_dimension ; i++)
						{
							face_number = -1;
							xiD[i] = xiB[i];
						}
					}
					break;
				}
			}
		}

		/* If we are really close to the boundary then the extra bit added on
			above may push us to the boundary in this half step so we should check
			here.  Steps xi->xiD and xiD->xiF are our final steps so check these */
		if (face_number == -1)
		{
			if (return_code)
			{
				return_code = (CMZN_OK == field_cache->setMeshLocation(*element, xiD)) &&
					(CMZN_OK == cmzn_field_evaluate_real_with_derivatives(coordinate_field, field_cache,
						vector_dimension, point2, /*number_of_derivatives*/element_dimension, dxdxi)) &&
					(CMZN_OK == cmzn_field_evaluate_real(stream_vector_field, field_cache,
						MAXIMUM_ELEMENT_XI_DIMENSIONS*MAXIMUM_ELEMENT_XI_DIMENSIONS, vector));
				if (reverse_track)
				{
					for (i = 0 ; i < vector_dimension ; i++)
					{
						vector[i] = -vector[i];
					}
				}
			}
			if (return_code)
			{
				return_code=calculate_delta_xi(vector_dimension,vector,element_dimension,
					dxdxi,deltaxiD);
			}
			if (return_code)
			{
				for (i = 0 ; i < element_dimension ; i++)
				{
					xiE[i] = xiD[i];
					increment_xi[i] = local_step_size*deltaxiD[i]/2.0;
				}
				return_code = FE_element_shape_xi_increment(element_shape, xiE,
					increment_xi, &fraction, &face_number, xi_face);
			}
			if (return_code)
			{
				return_code = (CMZN_OK == field_cache->setMeshLocation(*element, xiE)) &&
					(CMZN_OK == cmzn_field_evaluate_real_with_derivatives(coordinate_field, field_cache,
						vector_dimension, point3, /*number_of_derivatives*/element_dimension, dxdxi)) &&
					(CMZN_OK == cmzn_field_evaluate_real(stream_vector_field, field_cache,
						MAXIMUM_ELEMENT_XI_DIMENSIONS*MAXIMUM_ELEMENT_XI_DIMENSIONS, vector));
				if (reverse_track)
				{
					for (i = 0 ; i < vector_dimension ; i++)
					{
						vector[i] = -vector[i];
					}
				}
			}
			if (return_code)
			{
				return_code=calculate_delta_xi(vector_dimension,vector,element_dimension,
					dxdxi,deltaxiE);
			}
			if (return_code)
			{
				for (i = 0 ; i < element_dimension ; i++)
				{
					xiF[i] = xiD[i];
					increment_xi[i] = local_step_size*(deltaxiD[i]+deltaxiE[i])/4.0;
				}
				return_code = FE_element_shape_xi_increment(element_shape, xiF,
					increment_xi, &fraction, &face_number, xi_face);
				if (face_number != -1)
				{
					/* Reduce the step size to that which was actually taken */
					local_step_size *= (0.5 + fraction / 2.0);
				}
			}
		}
		else
		{
			/* Reduce the step size to that which was actually taken */
			local_step_size *= fraction / 2.0;
			for (i = 0 ; i < element_dimension ; i++)
			{
				/* Copy the values from the half step result */
				xiF[i] = xiD[i];
			}			
		}
		if (return_code)
		{
			/* Calculate halfway coordinate position from start and end, 
			 error is deviation from halfway point divided by length from
			start to end to normalise it. */
			coordinate_point_error = 0.0;
			for (i = 0 ; i < vector_dimension ; i++)
			{
				coordinate_point_vector = point2[i] - 
					(point1[i] * fraction + point3[i]) / (1.0 + fraction);
				coordinate_point_error += coordinate_point_vector * 
					coordinate_point_vector;
			}
			coordinate_point_error = sqrt(coordinate_point_error) / 
				coordinate_length;

			error=sqrt((xiF[0]-xiB[0])*(xiF[0]-xiB[0])+
				(xiF[1]-xiB[1])*(xiF[1]-xiB[1])+(xiF[2]-xiB[2])*(xiF[2]-xiB[2]));
			if ((local_step_size*sqrt(deltaxiC[0]*deltaxiC[0]+
				deltaxiC[1]*deltaxiC[1]+deltaxiC[2]*deltaxiC[2]))<1.0e-3)
			{
				error=0.0;
			}
		}
		if (((error>tolerance)||(coordinate_point_error>coordinate_tolerance))&&
			return_code)
		{
			local_step_size /= 2.0;
			xiB[0]=xiD[0];
			xiB[1]=xiD[1];
			xiB[2]=xiD[2];
		}
	}
	if (return_code)
	{
			*total_stepped += local_step_size;
			if (face_number != -1)
			{
				initial_element = *element;
				initial_face_number = face_number;
				xiD[0]=xiF[0];
				xiD[1]=xiF[1];
				xiD[2]=xiF[2];
				/* The last increment should have been the most accurate, if
				it wants to change then change element if we can */
				return_code = FE_element_change_to_adjacent_element(element,
					xiF, (FE_value *)NULL, &face_number, xi_face, /*permutation*/0);
				if (face_number == -1)
				{
					/* There is no adjacent element */
					*keep_tracking = 0;
				}
				else
				{
					/* Check the new xi coordinates are correct for our
					coordinate field and if not try rotating them */

					return_code = (CMZN_OK == field_cache->setMeshLocation(*element, xiF)) &&
						(CMZN_OK == cmzn_field_evaluate_real(coordinate_field, field_cache, vector_dimension, point1));
					coordinate_point_error = 0.0;
					for (i = 0 ; i < vector_dimension ; i++)
					{
						coordinate_point_error += (point1[i] - point3[i]) * 
							(point1[i] - point3[i]);
					}
					coordinate_point_error = sqrt(coordinate_point_error) / coordinate_length;
					// this permutation loop is inefficient; should extract common adjacent element code
					number_of_permutations =
						FE_element_get_number_of_change_to_adjacent_element_permutations(
							*element, xiF, face_number);
					/* We have already tried permutation 0 */
					permutation = 1;
					while ((permutation < number_of_permutations) &&
						(coordinate_point_error > coordinate_tolerance))
					{
						*element = initial_element;
						face_number = initial_face_number;
						xiF[0]=xiD[0];
						xiF[1]=xiD[1];
						xiF[2]=xiD[2];
						return_code = FE_element_change_to_adjacent_element(element,
							xiF, (FE_value *)NULL, &face_number, xi_face, permutation);
						return_code = (CMZN_OK == field_cache->setMeshLocation(*element, xiF)) &&
							(CMZN_OK == cmzn_field_evaluate_real(coordinate_field, field_cache, vector_dimension, point1));
						coordinate_point_error = 0.0;
						for (i = 0 ; i < vector_dimension ; i++)
						{
							coordinate_point_error += (point1[i] - point3[i]) *
								(point1[i] - point3[i]);
						}
						coordinate_point_error = sqrt(coordinate_point_error) / coordinate_length;
						permutation++;
					}
					element_shape = get_FE_element_shape(*element);
					if (!element_shape)
					{
						display_message(ERROR_MESSAGE, "track_streamline_from_FE_element.  Missing shape.");
						*keep_tracking = 0;
					}
					if (coordinate_point_error > coordinate_tolerance)
					{
						display_message(ERROR_MESSAGE,"track_streamline_from_FE_element.  "
							"Coordinates don't match after changing elements.");
						*keep_tracking = 0;
						*element = initial_element;
						xiF[0]=xiD[0];
						xiF[1]=xiD[1];
						xiF[2]=xiD[2];
					}
				}
			}
			else
			{
				/* Update the global step_size */
				if (error<tolerance/10.0)
				{
					local_step_size *= 2.0;
				}
				*step_size = local_step_size;
			}
	}
	if (return_code)
	{
		xi[0]=xiF[0];
		xi[1]=xiF[1];
		xi[2]=xiF[2];
		point[0] = point3[0];
		point[1] = point3[1];
		point[2] = point3[2];
	}
	LEAVE;

	return (return_code);
} /* update_adaptive_imp_euler */

static int update_interactive_streampoint(FE_value *point_coordinates,
	struct FE_element **element, cmzn_fieldcache_id field_cache,
	struct Computed_field *coordinate_field, FE_value *xi, FE_value *translate)
/*******************************************************************************
LAST MODIFIED : 3 December 2001

DESCRIPTION :
Updates the <GT_pointset> streampoint using the xi coordinates.  If the given
translation in world coordinates is non NULL the xi coordinates are updated
(this is transformed by the local dxdxi derivatives to xi values so only
accurate if small), also ensuring that the element is updated.
==============================================================================*/
{
	FE_value deltaxi[MAXIMUM_ELEMENT_XI_DIMENSIONS],
	   dxdxi[MAXIMUM_ELEMENT_XI_DIMENSIONS*MAXIMUM_ELEMENT_XI_DIMENSIONS],
	   point[MAXIMUM_ELEMENT_XI_DIMENSIONS],xiA[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	int return_code;

	ENTER(update_interactive_streampoint);
	/* check the arguments */
	if ((*element)&&(3==get_FE_element_dimension(*element))&&
		coordinate_field&&
		(3>=cmzn_field_get_number_of_components(coordinate_field)))
	{
		point[0]=0.0;
		point[1]=0.0;
		point[2]=0.0;
		if ( translate )
		{
			if ((CMZN_OK == field_cache->setMeshLocation(*element, xi)) &&
				(CMZN_OK == cmzn_field_evaluate_real_with_derivatives(coordinate_field, field_cache,
					/*number_of_values*/3, point, /*number_of_derivatives*/3, dxdxi)))
			{
				if (!(calculate_delta_xi(/*vector_dimension*/3,translate,
				   /*element_dimension*/3,dxdxi,deltaxi)))
				{
					/* dxdxi tensor is singular, peturb xi by random small amount */
					deltaxi[0] = 0.002*CMGUI_RANDOM(FE_value) - 0.001;
					deltaxi[1] = 0.002*CMGUI_RANDOM(FE_value) - 0.001;
					deltaxi[2] = 0.002*CMGUI_RANDOM(FE_value) - 0.001;
				}
				return_code = FE_element_xi_increment(element, xiA, deltaxi);
			}
			else
			{
				display_message(ERROR_MESSAGE,"update_interactive_streampoint.  "
					"Error calculating coordinate field with derivatives");
				return_code = 0;
			}
		}
		else
		{
			/* No translation required */
			return_code = 1;
		}
		if ( return_code )
		{
			if ((CMZN_OK != field_cache->setMeshLocation(*element, xi)) ||
				(CMZN_OK != cmzn_field_evaluate_real(coordinate_field, field_cache,
					/*number_of_values*/3, point_coordinates)))
			{
				display_message(ERROR_MESSAGE,
			"update_interactive_streampoint.  Error calculating coordinate field");
				return_code = 0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"update_interactive_streampoint.  Invalid argument(s)");
		display_message(ERROR_MESSAGE,"  element %p",*element);
		return_code=0;
	}
	LEAVE;

	return(return_code);
} /* update_interactive_streampoint */

static int track_streamline_from_FE_element(struct FE_element **element,
	FE_value *xi, cmzn_fieldcache_id field_cache, struct Computed_field *coordinate_field,
	struct Computed_field *stream_vector_field,int reverse_track,
	FE_value length, enum cmzn_graphics_streamlines_colour_data_type colour_data_type,
	struct Computed_field *data_field,int *number_of_points,
	Triple **stream_points,Triple **stream_vectors,Triple **stream_normals,
	GLfloat **stream_data)
/*******************************************************************************
LAST MODIFIED : 23 June 2004

DESCRIPTION :
Tracks the stream following <stream_vector_field> starting in the <*element> at
the supplied <xi> coordinates. The streamline is returned in the following
dynamically allocated and reallocated arrays:
<*stream_points> = points along the streamline;
<*stream_vectors> = stream vectors at each point on the streamline;
<*stream_normals> = unit normals to the vectors, appropriate to field tracked;
<*stream_data> = streamline data along the streamlines according to colour_data_type
and data_field.
The <*element> and <xi> values are updated to where the stream is tracked to, so
that tracking can be continued.
On unsuccessful return the above arrays are deallocated.

If <reverse_track> is true, the reverse of <stream_vector_field> is tracked, and
the negative travel_scalar is recorded, if requested.

The <stream_vector_field> may have 3, 6 or 9 components, the first 3 components
of which returns the vector along which the streamline is tracked. Additional
information about lateral direction and normal to a streamribbon are found by
different meands depending on the number of components in this field in the
following way:
3 = 1 3-D vector (lateral direction and normal worked out from curl of field);
6 = 2 3-D vectors (2nd vector is lateral direction. Stream ribbon normal found
    from cross product);
9 = 3 3-D vectors (2nd vector is lateral direction; 3rd vector is stream ribbon
    normal).
==============================================================================*/
{
	FE_value angle,coordinates[3],cos_angle,curl[3],curl_component,data_value,
		displacement[3] = {0.0, 0.0, 0.0},dv_dxi[9],dv_dx[9],dx_dxi[9],dxi_dx[9],
		magnitude,normal[3],normal2[3] = {0.0, 0.0, 0.0},old_normal[3] = {0.0, 0.0, 0.0},
		previous_curl_component = 0.0,previous_total_stepped_A,
		previous_total_stepped_B,sin_angle,step_size,stream_vector_values[9],
		temp,total_stepped,vector[3],vector_magnitude;
	GLfloat *stream_datum,*tmp_stream_data;
	int add_point,allocated_number_of_points,calculate_curl,element_dimension,
		i,keep_tracking,number_of_coordinate_components,
		number_of_stream_vector_components,return_code;
	struct FE_element *previous_element_A = NULL, *previous_element_B = NULL;
	Triple *stream_point,*stream_vector,*stream_normal,*tmp_triples = NULL;

	ENTER(track_streamline_from_FE_element);
	const bool hasData =
		(colour_data_type != CMZN_GRAPHICS_STREAMLINES_COLOUR_DATA_TYPE_FIELD) || (data_field);
	if (element&&(*element)&&FE_element_is_top_level(*element, NULL)&&
		(element_dimension = get_FE_element_dimension(*element))&&
		((3 == element_dimension) || (2 == element_dimension)) &&
		xi&&(0.0 <= xi[0])&&(1.0 >= xi[0])&&(0.0 <= xi[1])&&(1.0 >= xi[1])&&
		(0.0 <= xi[2])&&(1.0 >= xi[2])&& (field_cache) && coordinate_field &&
		(number_of_coordinate_components=cmzn_field_get_number_of_components(coordinate_field))&&
		stream_vector_field&&(number_of_stream_vector_components=
		cmzn_field_get_number_of_components(stream_vector_field))
		&& (((3 == number_of_coordinate_components) &&
		((3==number_of_stream_vector_components)||
		(6==number_of_stream_vector_components)||
		(9==number_of_stream_vector_components)))
		|| ((2 == number_of_coordinate_components) &&
		(2==number_of_stream_vector_components)))&&
		(0.0<length) && ((colour_data_type != CMZN_GRAPHICS_STREAMLINES_COLOUR_DATA_TYPE_FIELD) ||
		(0 == data_field) || (1 == cmzn_field_get_number_of_components(data_field))) &&
		number_of_points&&stream_points&&stream_vectors&&stream_normals&&
		((!hasData) || stream_data))
	{
		/*	step_size of zero indicates first step */
		step_size = 0;
		total_stepped = 0.0;
		previous_total_stepped_A = 0.0;

		calculate_curl = 1;
		return_code=1;
		/* clear coordinates in case coordinate field is not 3 component */
		coordinates[0]=0.0;
		coordinates[1]=0.0;
		coordinates[2]=0.0;
		*stream_points=(Triple *)NULL;
		*stream_vectors=(Triple *)NULL;
		*stream_normals=(Triple *)NULL;
		*stream_data=0;
		*number_of_points=0;
		allocated_number_of_points=100;
		if (ALLOCATE(*stream_points,Triple,allocated_number_of_points)&&
			ALLOCATE(*stream_vectors,Triple,allocated_number_of_points)&&
			ALLOCATE(*stream_normals,Triple,allocated_number_of_points)&&
			((!hasData) || ALLOCATE(*stream_data, GLfloat, allocated_number_of_points)))
		{
			stream_point= *stream_points;
			stream_vector= *stream_vectors;
			stream_normal= *stream_normals;
			stream_datum= *stream_data;
			i=0;
			add_point = 1;
			keep_tracking = 1;
			while (return_code && add_point)
			{
				while (return_code && add_point && (i<allocated_number_of_points))
				{
					/* evaluate the coordinate and stream_vector fields */
					return_code = (CMZN_OK == field_cache->setMeshLocation(*element, xi));
					switch (number_of_stream_vector_components)
					{
						case 2:
						{
							stream_vector_values[2] = 0.0;
						} /* no break */
						case 3:
						{
							/* need derivatives of coordinate_field and stream_vector_field
								for calculation of curl. Calculate coordinates first, since
								may be required for evaluating the stream_vector_field. */
							return_code = return_code &&
								(CMZN_OK == cmzn_field_evaluate_real_with_derivatives(coordinate_field, field_cache,
									number_of_coordinate_components, coordinates, element_dimension, dx_dxi));
							if (return_code)
							{
								if (calculate_curl)
								{
									if (CMZN_OK != cmzn_field_evaluate_real_with_derivatives(stream_vector_field, field_cache,
										number_of_stream_vector_components, stream_vector_values, element_dimension, dv_dxi))
									{
										return_code = (CMZN_OK == cmzn_field_evaluate_real(stream_vector_field, field_cache,
											number_of_stream_vector_components, stream_vector_values));
										calculate_curl = 0;
										display_message(WARNING_MESSAGE,
											"Stream vector field derivatives are unavailable, "
											"continuing but not integrating the curl.");
									}
								}
								else
								{
									return_code = (CMZN_OK == cmzn_field_evaluate_real(stream_vector_field, field_cache,
										number_of_stream_vector_components, stream_vector_values));
								}
							}
						} break;
						case 6:
						case 9:
						{
							/* derivatives are not required with 6 or 9 components in the
								stream_vector_field, since the lateral direction and ribbon
								normal are taken or calculated directly from it. Want to
								evaluate stream_vector_field first, since in many cases it will
								force a calculation of the coordinate field with derivatives,
								thus saving the coordinates from being recalculated */
							return_code =
								(CMZN_OK == cmzn_field_evaluate_real(stream_vector_field, field_cache,
									number_of_stream_vector_components, stream_vector_values)) &&
								(CMZN_OK == cmzn_field_evaluate_real(coordinate_field, field_cache,
									number_of_coordinate_components, coordinates));
						} break;
					}
					/* extract stream vector from stream_vector_values */
					if (reverse_track)
					{
						vector[0]=-stream_vector_values[0];
						vector[1]=-stream_vector_values[1];
						vector[2]=-stream_vector_values[2];
					}
					else
					{
						vector[0]=stream_vector_values[0];
						vector[1]=stream_vector_values[1];
						vector[2]=stream_vector_values[2];
					}
					if (1e-30>(vector_magnitude=sqrt(
						vector[0]*vector[0]+vector[1]*vector[1]+vector[2]*vector[2])))
					{
						/* streamline is not going anywhere */
						add_point = 0;
					}
					if (add_point)
					{
						/* get vector, normal and normal2 - latter are unit vectors normal
							 to the streamline - normal is in lateral direction across sheet,
							 normal2 is normal to the ribbon sheet */
						switch (element_dimension)
						{
							case 2:
							{
								switch (number_of_stream_vector_components)
								{
									case 2:
									{
										/* Normal2 is out of the 2D plane */
										normal2[0] = 0.0;
										normal2[1] = 0.0;
										normal2[2] = 1.0;
										/* get normal = vector (x) normal2 */
										normal[0]=vector[1]*normal2[2]-vector[2]*normal2[1];
										normal[1]=vector[2]*normal2[0]-vector[0]*normal2[2];
										normal[2]=vector[0]*normal2[1]-vector[1]*normal2[0];
										/* make normal unit length */
										magnitude=sqrt(normal[0]*normal[0]+normal[1]*normal[1]+
											normal[2]*normal[2]);
										if (0.0<magnitude)
										{
											normal[0] /= magnitude;
											normal[1] /= magnitude;
											normal[2] /= magnitude;
										}
									} break;
									case 3:
									{
										/* Normal2 is the cross of the derivatives */
										normal2[0] = dx_dxi[2] * dx_dxi[5] - dx_dxi[4] * dx_dxi[3];
										normal2[1] = dx_dxi[4] * dx_dxi[1] - dx_dxi[0] * dx_dxi[5];
										normal2[2] = dx_dxi[0] * dx_dxi[3] - dx_dxi[2] * dx_dxi[1];
										/* make normal2 unit length */
										magnitude=sqrt(normal2[0]*normal2[0]+normal2[1]*normal2[1]+
											normal2[2]*normal2[2]);
										if (0.0<magnitude)
										{
											normal2[0] /= magnitude;
											normal2[1] /= magnitude;
											normal2[2] /= magnitude;
										}
										/* get normal = vector (x) normal2 */
										normal[0]=vector[1]*normal2[2]-vector[2]*normal2[1];
										normal[1]=vector[2]*normal2[0]-vector[0]*normal2[2];
										normal[2]=vector[0]*normal2[1]-vector[1]*normal2[0];
										/* make normal unit length */
										magnitude=sqrt(normal[0]*normal[0]+normal[1]*normal[1]+
											normal[2]*normal[2]);
										if (0.0<magnitude)
										{
											normal[0] /= magnitude;
											normal[1] /= magnitude;
											normal[2] /= magnitude;
										}
									} break;
									default:
									{
										display_message(ERROR_MESSAGE,
											"track_streamline_from_FE_element.  "
											"Incompatible element dimension and vector components.");
										return_code = 0;
									}
								}
							} break;
							case 3:
							{
								switch (number_of_stream_vector_components)
								{
									case 3:
									{
										if (calculate_curl)
										{
											/* evaluate the curl, and its component along vector */
											if (invert_FE_value_matrix3(dx_dxi,dxi_dx)&&
												multiply_FE_value_matrix3(dv_dxi,dxi_dx,dv_dx))
											{
												curl[0] = dv_dx[7] - dv_dx[5];
												curl[1] = dv_dx[2] - dv_dx[6];
												curl[2] = dv_dx[3] - dv_dx[1];
												curl_component=(curl[0]*vector[0]+curl[1]*vector[1]+
													curl[2]*vector[2])/vector_magnitude;
											}
											else
											{
												curl_component=0.0;
											}
										}
										else
										{
											curl_component = 0.0;
										}
										if (0.0==total_stepped)
										{
											angle = 0.0;
											previous_total_stepped_A = 0.0;
											/* get normal2 = a vector not co-linear with stream vector */
											normal2[0]=0.0;
											normal2[1]=0.0;
											normal2[2]=0.0;
											if (fabs(vector[0]) < fabs(vector[1]))
											{
												if (fabs(vector[2]) < fabs(vector[0]))
												{
													normal2[2]=1.0;
												}
												else
												{
													normal2[0]=1.0;
												}
											}
											else
											{
												if (fabs(vector[2]) < fabs(vector[1]))
												{
													normal2[2]=1.0;
												}
												else
												{
													normal2[1]=1.0;
												}
											}
										}
										else
										{
											/* get angle from average curl along segment of streamline */
											angle = 0.5*(previous_curl_component + curl_component)
												* (total_stepped - previous_total_stepped_A);
											if (reverse_track)
											{
												angle = -angle;
											}
											/* get displacement from old to current coordinates */
											displacement[0]=coordinates[0]-displacement[0];
											displacement[1]=coordinates[1]-displacement[1];
											displacement[2]=coordinates[2]-displacement[2];
											magnitude=displacement[0]*displacement[0]+
												displacement[1]*displacement[1]+
												displacement[2]*displacement[2];
											if (0.0<magnitude)
											{
												/* get normal2 = displacement (x) old_normal */
												normal2[0]=displacement[1]*old_normal[2]-
													displacement[2]*old_normal[1];
												normal2[1]=displacement[2]*old_normal[0]-
													displacement[0]*old_normal[2];
												normal2[2]=displacement[0]*old_normal[1]-
													displacement[1]*old_normal[0];
												magnitude=normal2[0]*normal2[0]+
													normal2[1]*normal2[1]+normal2[2]*normal2[2];
											}
											if (0.0<magnitude)
											{
												/* displacement is zero or displacement and old_normal 
													are co-linear */
												/* get normal2 = vector (x) old_normal */
												normal2[0]=vector[1]*old_normal[2]-vector[2]*old_normal[1];
												normal2[1]=vector[2]*old_normal[0]-vector[0]*old_normal[2];
												normal2[2]=vector[0]*old_normal[1]-vector[1]*old_normal[0];
											}
										}
										/* get normal = normal2 (x) vector */
										normal[0]=normal2[1]*vector[2]-normal2[2]*vector[1];
										normal[1]=normal2[2]*vector[0]-normal2[0]*vector[2];
										normal[2]=normal2[0]*vector[1]-normal2[1]*vector[0];
										/* make normal unit length */
										magnitude=sqrt(normal[0]*normal[0]+normal[1]*normal[1]+
											normal[2]*normal[2]);
										if (0.0<magnitude)
										{
											normal[0] /= magnitude;
											normal[1] /= magnitude;
											normal[2] /= magnitude;
										}
										/* get normal2 = vector (x) normal */
										normal2[0]=vector[1]*normal[2]-vector[2]*normal[1];
										normal2[1]=vector[2]*normal[0]-vector[0]*normal[2];
										normal2[2]=vector[0]*normal[1]-vector[1]*normal[0];
										/* make normal2 unit length */
										magnitude=sqrt(normal2[0]*normal2[0]+normal2[1]*normal2[1]+
											normal2[2]*normal2[2]);
										if (0.0<magnitude)
										{
											normal2[0] /= magnitude;
											normal2[1] /= magnitude;
											normal2[2] /= magnitude;
										}
										/* rotate normals by angle */
										cos_angle=cos(angle);
										sin_angle=sin(angle);
										temp = normal[0];
										normal[0]  = normal[0]  * cos_angle + normal2[0] * sin_angle;
										normal2[0] = normal2[0] * cos_angle - temp       * sin_angle;
										temp = normal[1];
										normal[1]  = normal[1]  * cos_angle + normal2[1] * sin_angle;
										normal2[1] = normal2[1] * cos_angle - temp       * sin_angle;
										temp = normal[2];
										normal[2]  = normal[2]  * cos_angle + normal2[2] * sin_angle;
										normal2[2] = normal2[2] * cos_angle - temp       * sin_angle;
										/* store old_normal, and old coordinates in displacement */
										old_normal[0]=normal[0];
										old_normal[1]=normal[1];
										old_normal[2]=normal[2];
										displacement[0]=coordinates[0];
										displacement[1]=coordinates[1];
										displacement[2]=coordinates[2];
										previous_curl_component = curl_component;
									} break;
									case 6:
									{
										/* get stream vector and normal from stream_vector_values */
										if (reverse_track)
										{
											normal[0]=-stream_vector_values[3];
											normal[1]=-stream_vector_values[4];
											normal[2]=-stream_vector_values[5];
										}
										else
										{
											normal[0]=stream_vector_values[3];
											normal[1]=stream_vector_values[4];
											normal[2]=stream_vector_values[5];
										}
										/* get normal2 = vector (x) normal */
										normal2[0]=vector[1]*normal[2]-vector[2]*normal[1];
										normal2[1]=vector[2]*normal[0]-vector[0]*normal[2];
										normal2[2]=vector[0]*normal[1]-vector[1]*normal[0];
										/* make normal2 unit length */
										magnitude=sqrt(normal2[0]*normal2[0]+
											normal2[1]*normal2[1]+normal2[2]*normal2[2]);
										if (0.0<magnitude)
										{
											normal2[0] /= magnitude;
											normal2[1] /= magnitude;
											normal2[2] /= magnitude;
										}
									} break;
									case 9:
									{
										/* get vector, normal and normal2 from stream_vector_values */
										if (reverse_track)
										{
											normal[0]=-stream_vector_values[3];
											normal[1]=-stream_vector_values[4];
											normal[2]=-stream_vector_values[5];
										}
										else
										{
											normal[0]=stream_vector_values[3];
											normal[1]=stream_vector_values[4];
											normal[2]=stream_vector_values[5];
										}
										normal2[0]=stream_vector_values[6];
										normal2[1]=stream_vector_values[7];
										normal2[2]=stream_vector_values[8];
									} break;
									default:
									{
										display_message(ERROR_MESSAGE,
											"track_streamline_from_FE_element.  "
											"Incompatible element dimension and vector components.");
										return_code = 0;
									}
								}
							} break;
							default:
							{
								display_message(ERROR_MESSAGE,
									"track_streamline_from_FE_element.  "
									"Unsupported number of element dimension.");
								return_code = 0;
							} break;
						}
						/* calculate data */
						switch (colour_data_type)
						{
							case CMZN_GRAPHICS_STREAMLINES_COLOUR_DATA_TYPE_FIELD:
							{
								if (data_field)
								{
									// cache location should be unchanged from earlier
									if (CMZN_OK != cmzn_field_evaluate_real(data_field, field_cache, /*number_of_values*/1, &data_value))
									{
										display_message(ERROR_MESSAGE,
											"track_streamline_from_FE_element.   Error calculating data field");
										return_code = 0;
									}
								}
							}	break;
							case CMZN_GRAPHICS_STREAMLINES_COLOUR_DATA_TYPE_MAGNITUDE:
							{
								data_value = vector_magnitude;
							} break;
							case CMZN_GRAPHICS_STREAMLINES_COLOUR_DATA_TYPE_TRAVEL_TIME:
							{
								data_value = (reverse_track) ? -total_stepped : total_stepped;
							} break;
							default:
							{
								display_message(ERROR_MESSAGE,
									"track_streamline_from_FE_element.  Unknown streamlines data type");
								return_code = 0;
							} break;
						}
						/* set point, normal and data */
						(*stream_point)[0] = GLfloat(coordinates[0]);
						(*stream_point)[1] = GLfloat(coordinates[1]);
						(*stream_point)[2] = GLfloat(coordinates[2]);
						stream_point++;
						(*stream_vector)[0] = GLfloat(vector[0]);
						(*stream_vector)[1] = GLfloat(vector[1]);
						(*stream_vector)[2] = GLfloat(vector[2]);
						stream_vector++;
						(*stream_normal)[0] = GLfloat(normal2[0]);
						(*stream_normal)[1] = GLfloat(normal2[1]);
						(*stream_normal)[2] = GLfloat(normal2[2]);
						stream_normal++;
						if (hasData)
						{
							*stream_datum = (ZnReal)data_value;
							stream_datum++;
						}
						i++;
						if (keep_tracking && (total_stepped<length))
						{
							/* perform the tracking, changing elements as necessary */
							if (total_stepped + step_size > length)
							{
								step_size = length - total_stepped;
							}
							previous_total_stepped_B = previous_total_stepped_A;
							previous_total_stepped_A = total_stepped;
							previous_element_B = previous_element_A;
							previous_element_A = *element;
							return_code=update_adaptive_imp_euler(field_cache,coordinate_field,
								stream_vector_field,reverse_track,element,xi,
								coordinates,&step_size,&total_stepped,&keep_tracking);
							/* If we haven't gone anywhere and are changing back to the previous
								element then we are stuck */
							if (total_stepped == previous_total_stepped_B)
							{
								if ((*element == previous_element_B) &&
									(*element != previous_element_A))
								{
									add_point = 0;
									keep_tracking = 0;
									printf("track_streamline_from_FE_element.  "
										"trapped between two elements with opposing directions\n");
								}
								else if (previous_element_B)
								{
									add_point = 0;
									keep_tracking = 0;
									printf("track_streamline_from_FE_element.  "
										"streamline has stopped progressing.\n");
								}
							}
						}
						else
						{
							add_point = 0;
						}
					}
				}
				if (add_point||(i<allocated_number_of_points))
				{
					if (add_point)
					{
						/* allocate 100 more spaces for points, normals and data */
						allocated_number_of_points += 100;
					}
					else
					{
						/* free unused space in arrays */
						allocated_number_of_points = i;
					}
					if (allocated_number_of_points != 0)
					{
						if (REALLOCATE(tmp_triples,*stream_points,Triple,
							allocated_number_of_points))
						{
							*stream_points = tmp_triples;
							stream_point = (*stream_points) + i;
						}
						else
						{
							return_code = 0;
						}
						if (REALLOCATE(tmp_triples,*stream_vectors,Triple,
							allocated_number_of_points))
						{
							*stream_vectors = tmp_triples;
							stream_vector = (*stream_vectors) + i;
						}
						else
						{
							return_code = 0;
						}
						if (REALLOCATE(tmp_triples,*stream_normals,Triple,
							allocated_number_of_points))
						{
							*stream_normals = tmp_triples;
							stream_normal = (*stream_normals) + i;
						}
						else
						{
							return_code = 0;
						}
						if (hasData)
						{
							if (REALLOCATE(tmp_stream_data,*stream_data,GLfloat,
								allocated_number_of_points))
							{
								*stream_data=tmp_stream_data;
								stream_datum = (*stream_data) + i;
							}
							else
							{
								return_code=0;
							}
						}
						if (!return_code)
						{
							display_message(ERROR_MESSAGE,
								"track_streamline_from_FE_element.  Could not reallocate");
							return_code=0;
						}
					}
					else
					{
						return_code = 0;
					}
				}
			}
			*number_of_points = i;
		}
		else
		{
			display_message(ERROR_MESSAGE,"track_streamline_from_FE_element.  "
				"Not enough memory for streamline");
			return_code=0;
		}
		if (!return_code)
		{
			/* free up any space allocated for the streamline */
			if (*stream_points)
			{
				DEALLOCATE(*stream_points);
			}
			if (*stream_vectors)
			{
				DEALLOCATE(*stream_vectors);
			}
			if (*stream_normals)
			{
				DEALLOCATE(*stream_normals);
			}
			if (*stream_data)
			{
				DEALLOCATE(*stream_data);
			}
			*number_of_points=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"track_streamline_from_FE_element.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* track_streamline_from_FE_element */

/*
Global functions
----------------
*/

int create_polyline_streamline_FE_element_vertex_array(
	struct FE_element *element,FE_value *start_xi,
	cmzn_fieldcache_id field_cache, struct Computed_field *coordinate_field,
	struct Computed_field *stream_vector_field,int reverse_track,
	FE_value length, enum cmzn_graphics_streamlines_colour_data_type colour_data_type,
	struct Computed_field *data_field,
	struct Graphics_vertex_array *array)
{
	GLfloat *stream_data;
	int element_dimension,number_of_stream_points,number_of_coordinate_components,
		number_of_stream_vector_components, return_code = 1;
	Triple *stream_points,*stream_normals,*stream_vectors;

	if (stream_vector_field)
	{
		if (element&&FE_element_is_top_level(element, NULL)&&
			(element_dimension = get_FE_element_dimension(element))&&
			((3 == element_dimension) || (2 == element_dimension))
			&&start_xi&&coordinate_field&&
			(number_of_coordinate_components=cmzn_field_get_number_of_components(coordinate_field))&&
			(number_of_stream_vector_components=cmzn_field_get_number_of_components(stream_vector_field))
			&& (((3 == number_of_coordinate_components) &&
			((3==number_of_stream_vector_components)||
			(6==number_of_stream_vector_components)||
			(9==number_of_stream_vector_components)))
			|| ((2 == number_of_coordinate_components) &&
			(2==number_of_stream_vector_components)))&&
			(0.0<length) && array)
		{
			/* track points and normals on streamline, and data if requested */
			if (track_streamline_from_FE_element(&element,start_xi,
				field_cache, coordinate_field,stream_vector_field,reverse_track,length,
				colour_data_type,data_field,&number_of_stream_points,&stream_points,
					&stream_vectors,&stream_normals,&stream_data))
			{
				if (0<number_of_stream_points)
				{
					unsigned int total_number_of_vertices = number_of_stream_points;
					unsigned int vertex_start = array->get_number_of_vertices(
						GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION);

					array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
						3, number_of_stream_points, &(stream_points[0][0]));
					if (stream_data)
						array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
							1, number_of_stream_points, stream_data);
					array->add_unsigned_integer_attribute(
						GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
						1, 1, &total_number_of_vertices);
					array->add_unsigned_integer_attribute(
						GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
						1, 1, &vertex_start);
					DEALLOCATE(stream_points);
					DEALLOCATE(stream_data);
					DEALLOCATE(stream_vectors);
					DEALLOCATE(stream_normals);
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
			return_code = 0;
		}
	}
	else
	{
		return_code = 0;
	}

	return (return_code);
}

int create_surface_streamribbon_FE_element_vertex_array(
	struct FE_element *element,FE_value *start_xi,
	cmzn_fieldcache_id field_cache, struct Computed_field *coordinate_field,
	struct Computed_field *stream_vector_field,int reverse_track, FE_value length,
	enum cmzn_graphicslineattributes_shape_type line_shape, int circleDivisions,
	FE_value *line_base_size, FE_value *line_scale_factors,
	struct Computed_field *line_orientation_scale_field,
	enum cmzn_graphics_streamlines_colour_data_type colour_data_type, struct Computed_field *data_field,
	struct Graphics_vertex_array *array)
{
	double cosw,magnitude,sinw;
	GLfloat *stream_data,stream_datum= 0.0;
	int d,element_dimension,i,number_of_stream_points,number_of_coordinate_components,
		number_of_stream_vector_components,surface_points_per_step, return_code = 1;
	Triple cross_thickness,cross_width,normal,point,stream_cross,
		stream_normal,*stream_normals,stream_point,*stream_points,
		stream_unit_vector = {1.0, 0.0, 0.0},stream_vector,*stream_vectors;

	USE_PARAMETER(line_scale_factors);
	USE_PARAMETER(line_orientation_scale_field);
	if (stream_vector_field)
	{
		if (element&&FE_element_is_top_level(element, NULL)&&
			(0 < (element_dimension = get_FE_element_dimension(element))) &&
			((3 == element_dimension) || (2 == element_dimension))
			&&start_xi&&coordinate_field&&
			(0 < (number_of_coordinate_components=cmzn_field_get_number_of_components(coordinate_field))) &&
			(number_of_stream_vector_components=
				cmzn_field_get_number_of_components(stream_vector_field))
				&& (((3 == number_of_coordinate_components) &&
					((3==number_of_stream_vector_components)||
						(6==number_of_stream_vector_components)||
						(9==number_of_stream_vector_components)))
						|| ((2 == number_of_coordinate_components) &&
							(2==number_of_stream_vector_components)))&&
							(0.0<length) && array)
		{
			const bool hasData =
				(colour_data_type != CMZN_GRAPHICS_STREAMLINES_COLOUR_DATA_TYPE_FIELD) || (data_field);
			const FE_value width = line_base_size[0];
			const FE_value thickness = line_base_size[1];

			/* track points and normals on streamline, and data if requested */
			if (track_streamline_from_FE_element(&element,start_xi,
				field_cache, coordinate_field,stream_vector_field,reverse_track,length,
				colour_data_type,data_field,&number_of_stream_points,&stream_points,
				&stream_vectors,&stream_normals,&stream_data))
			{
				if (0<number_of_stream_points)
				{
					switch (line_shape)
					{
						case CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_SQUARE_EXTRUSION:
						{
							surface_points_per_step = 8;
						} break;
						case CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_CIRCLE_EXTRUSION:
						{
							surface_points_per_step = circleDivisions + 1;
						} break;
						case CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_RIBBON:
						default:
						{
							surface_points_per_step = 2;
						} break;
					}

					unsigned int vertex_start = array->get_number_of_vertices(
						GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION);
					unsigned int number_of_vertices = surface_points_per_step * number_of_stream_points;
					/* now fill in the points and data from the streamline */
					GLfloat floatField[3];
					for (i=0;i<number_of_stream_points;i++)
					{
						stream_point[0]=stream_points[i][0];
						stream_point[1]=stream_points[i][1];
						stream_point[2]=stream_points[i][2];
						stream_vector[0]=stream_vectors[i][0];
						stream_vector[1]=stream_vectors[i][1];
						stream_vector[2]=stream_vectors[i][2];
						stream_normal[0]=stream_normals[i][0];
						stream_normal[1]=stream_normals[i][1];
						stream_normal[2]=stream_normals[i][2];
						if (stream_data)
						{
							stream_datum=stream_data[i];
						}
						if (0.0 < (magnitude = sqrt(stream_vector[0]*stream_vector[0]+
							stream_vector[1]*stream_vector[1]+
							stream_vector[2]*stream_vector[2])))
						{
							stream_unit_vector[0] = stream_vector[0] / GLfloat(magnitude);
							stream_unit_vector[1] = stream_vector[1] / GLfloat(magnitude);
							stream_unit_vector[2] = stream_vector[2] / GLfloat(magnitude);
						}
						/* get stream_cross = stream_normal (x) stream_unit_vector */
						stream_cross[0]=stream_normal[1]*stream_unit_vector[2]-
							stream_normal[2]*stream_unit_vector[1];
						stream_cross[1]=stream_normal[2]*stream_unit_vector[0]-
							stream_normal[0]*stream_unit_vector[2];
						stream_cross[2]=stream_normal[0]*stream_unit_vector[1]-
							stream_normal[1]*stream_unit_vector[0];
						cross_width[0] = stream_cross[0] * 0.5f * width;
						cross_width[1] = stream_cross[1] * 0.5f * width;
						cross_width[2] = stream_cross[2] * 0.5f * width;
						cross_thickness[0] = stream_normal[0] * 0.5f * GLfloat(thickness);
						cross_thickness[1] = stream_normal[1] * 0.5f * GLfloat(thickness);
						cross_thickness[2] = stream_normal[2] * 0.5f * GLfloat(thickness);
						switch (line_shape)
						{
							case CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_RIBBON:
							default:
							{
								point[0] = stream_point[0] + cross_width[0];
								point[1] = stream_point[1] + cross_width[1];
								point[2] = stream_point[2] + cross_width[2];
								CAST_TO_OTHER(floatField,point,GLfloat,3);
								array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
									3, 1, floatField);
								normal[0] = stream_normal[0];
								normal[1] = stream_normal[1];
								normal[2] = stream_normal[2];
								CAST_TO_OTHER(floatField,normal,GLfloat,3);
								array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
									3, 1, floatField);
								if (hasData && (stream_data))
								{
									array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
										1, 1, &stream_datum);
								}
								point[0] = stream_point[0] - cross_width[0];
								point[1] = stream_point[1] - cross_width[1];
								point[2] = stream_point[2] - cross_width[2];
								CAST_TO_OTHER(floatField,point,GLfloat,3);
								array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
									3, 1, floatField);
								normal[0] = stream_normal[0];
								normal[1] = stream_normal[1];
								normal[2] = stream_normal[2];
								CAST_TO_OTHER(floatField,normal,GLfloat,3);
								array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
									3, 1, floatField);
								if (hasData && stream_data)
								{
									array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
										1, 1, &stream_datum);
								}
							} break;
							case CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_CIRCLE_EXTRUSION:
							{
								for (d = 0 ; d < surface_points_per_step ; d++)
								{
									sinw = sin( 2 * PI * (double)d /
										(double)(surface_points_per_step - 1));
									cosw = cos( 2 * PI * (double)d /
										(double)(surface_points_per_step - 1));
									point[0] = stream_point[0] + GLfloat(sinw) * cross_width[0] + GLfloat(cosw) * cross_thickness[0];
									point[1] = stream_point[1] + GLfloat(sinw) * cross_width[1] + GLfloat(cosw) * cross_thickness[1];
									point[2] = stream_point[2] + GLfloat(sinw) * cross_width[2] + GLfloat(cosw) * cross_thickness[2];
									CAST_TO_OTHER(floatField,point,GLfloat,3);
									array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
										3, 1, floatField);
									normal[0] = -GLfloat(sinw) * stream_cross[0] * 0.5f * GLfloat(thickness) -
										GLfloat(cosw) * stream_normal[0] * 0.5f * width;
									normal[1] = -GLfloat(sinw) * stream_cross[1] * 0.5f * GLfloat(thickness) -
										GLfloat(cosw) * stream_normal[1] * 0.5f * GLfloat(thickness);
									normal[2] = -GLfloat(sinw) * stream_cross[2] * 0.5f * GLfloat(thickness) -
										GLfloat(cosw) * stream_normal[2] * 0.5f * GLfloat(thickness);
									magnitude = sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
									if (0.0<magnitude)
									{
										normal[0] /= GLfloat(magnitude);
										normal[1] /= GLfloat(magnitude);
										normal[2] /= GLfloat(magnitude);
									}
									CAST_TO_OTHER(floatField,normal,GLfloat,3);
									array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
										3, 1, floatField);
									if (hasData && stream_data)
									{
										array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
											1, 1, &stream_datum);
									}
								}
							} break;
							case CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_SQUARE_EXTRUSION:
							{
								point[0] = stream_point[0] + cross_width[0] + cross_thickness[0];
								point[1] = stream_point[1] + cross_width[1] + cross_thickness[1];
								point[2] = stream_point[2] + cross_width[2] + cross_thickness[2];
								CAST_TO_OTHER(floatField,point,GLfloat,3);
								array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
									3, 1, floatField);
								normal[0] = stream_normal[0];
								normal[1] = stream_normal[1];
								normal[2] = stream_normal[2];
								CAST_TO_OTHER(floatField,normal,GLfloat,3);
								array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
									3, 1, floatField);
								if (hasData && stream_data)
								{
									array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
										1, 1, &stream_datum);
								}
								point[0] = stream_point[0] - cross_width[0] + cross_thickness[0];
								point[1] = stream_point[1] - cross_width[1] + cross_thickness[1];
								point[2] = stream_point[2] - cross_width[2] + cross_thickness[2];
								CAST_TO_OTHER(floatField,point,GLfloat,3);
								array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
									3, 1, floatField);
								normal[0] = stream_normal[0];
								normal[1] = stream_normal[1];
								normal[2] = stream_normal[2];
								CAST_TO_OTHER(floatField,normal,GLfloat,3);
								array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
									3, 1, floatField);
								if (hasData && stream_data)
								{
									array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
										1, 1, &stream_datum);
								}

								point[0] = stream_point[0] - cross_width[0] + cross_thickness[0];
								point[1] = stream_point[1] - cross_width[1] + cross_thickness[1];
								point[2] = stream_point[2] - cross_width[2] + cross_thickness[2];
								CAST_TO_OTHER(floatField,point,GLfloat,3);
								array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
									3, 1, floatField);
								normal[0] = -stream_cross[0];
								normal[1] = -stream_cross[1];
								normal[2] = -stream_cross[2];
								CAST_TO_OTHER(floatField,normal,GLfloat,3);
								array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
									3, 1, floatField);
								if (hasData && stream_data)
								{
									array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
										1, 1, &stream_datum);
								}
								point[0] = stream_point[0] - cross_width[0] - cross_thickness[0];
								point[1] = stream_point[1] - cross_width[1] - cross_thickness[1];
								point[2] = stream_point[2] - cross_width[2] - cross_thickness[2];
								CAST_TO_OTHER(floatField,point,GLfloat,3);
								array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
									3, 1, floatField);
								normal[0] = -stream_cross[0];
								normal[1] = -stream_cross[1];
								normal[2] = -stream_cross[2];
								CAST_TO_OTHER(floatField,normal,GLfloat,3);
								array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
									3, 1, floatField);
								if (hasData && stream_data)
								{
									array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
										1, 1, &stream_datum);
								}

								point[0] = stream_point[0] - cross_width[0] - cross_thickness[0];
								point[1] = stream_point[1] - cross_width[1] - cross_thickness[1];
								point[2] = stream_point[2] - cross_width[2] - cross_thickness[2];
								CAST_TO_OTHER(floatField,point,GLfloat,3);
								array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
									3, 1, floatField);
								normal[0] = -stream_normal[0];
								normal[1] = -stream_normal[1];
								normal[2] = -stream_normal[2];
								CAST_TO_OTHER(floatField,normal,GLfloat,3);
								array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
									3, 1, floatField);
								if (hasData && stream_data)
								{
									array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
										1, 1, &stream_datum);
								}
								point[0] = stream_point[0] + cross_width[0] - cross_thickness[0];
								point[1] = stream_point[1] + cross_width[1] - cross_thickness[1];
								point[2] = stream_point[2] + cross_width[2] - cross_thickness[2];
								CAST_TO_OTHER(floatField,point,GLfloat,3);
								array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
									3, 1, floatField);
								normal[0] = -stream_normal[0];
								normal[1] = -stream_normal[1];
								normal[2] = -stream_normal[2];
								CAST_TO_OTHER(floatField,normal,GLfloat,3);
								array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
									3, 1, floatField);
								if (hasData && stream_data)
								{
									array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
										1, 1, &stream_datum);
								}

								point[0] = stream_point[0] + cross_width[0] - cross_thickness[0];
								point[1] = stream_point[1] + cross_width[1] - cross_thickness[1];
								point[2] = stream_point[2] + cross_width[2] - cross_thickness[2];
								CAST_TO_OTHER(floatField,point,GLfloat,3);
								array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
									3, 1, floatField);
								normal[0] = stream_cross[0];
								normal[1] = stream_cross[1];
								normal[2] = stream_cross[2];
								CAST_TO_OTHER(floatField,normal,GLfloat,3);
								array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
									3, 1, floatField);
								if (hasData && stream_data)
								{
									array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
										1, 1, &stream_datum);
								}
								point[0] = stream_point[0] + cross_width[0] + cross_thickness[0];
								point[1] = stream_point[1] + cross_width[1] + cross_thickness[1];
								point[2] = stream_point[2] + cross_width[2] + cross_thickness[2];
								CAST_TO_OTHER(floatField,point,GLfloat,3);
								array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
									3, 1, floatField);
								normal[0] = stream_cross[0];
								normal[1] = stream_cross[1];
								normal[2] = stream_cross[2];
								CAST_TO_OTHER(floatField,normal,GLfloat,3);
								array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
									3, 1, floatField);
								if (hasData && stream_data)
								{
									array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
										1, 1, &stream_datum);
								}
							} break;
						}
					}
					int polygonType = (int)g_TRIANGLE;

					unsigned int number_of_xi1 = surface_points_per_step,
						number_of_xi2 = number_of_stream_points;
					array->add_unsigned_integer_attribute(
						GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
						1, 1, &number_of_vertices);
					array->add_unsigned_integer_attribute(
						GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
						1, 1, &vertex_start);
					array->add_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POLYGON,
						1, 1, &polygonType);
					array->add_unsigned_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_XI1,
						1, 1, &number_of_xi1);
					array->add_unsigned_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_XI2,
						1, 1, &number_of_xi2);
					array->fill_element_index(vertex_start, number_of_xi1, number_of_xi2,
						ARRAY_SHAPE_TYPE_UNSPECIFIED);
					/* no longer need original streamline */
					DEALLOCATE(stream_points);
					DEALLOCATE(stream_vectors);
					DEALLOCATE(stream_normals);
					DEALLOCATE(stream_data);
				}
				else
				{
					/* no error: streamline empty */
					return_code = 0;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"create_surface_streamribbon_FE_element_vertex_array.  "
					"failed to track streamline");
				return_code = 0;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"create_surface_streamribbon_FE_element_vertex_array.  Invalid argument(s)");
			return_code = 0;
		}
	}
	else
	{
		return_code = 0;
	}

	return (return_code);
}

int add_flow_particle(struct Streampoint **list,FE_value *xi,
	struct FE_element *element,Triple **pointlist,int index,
	cmzn_fieldcache_id field_cache, struct Computed_field *coordinate_field,
	gtObject *graphics_object)
/*******************************************************************************
LAST MODIFIED : 17 March 1999

DESCRIPTION :
Adds a new flow particle structure to the start of the Streampoint list
==============================================================================*/
{
	int return_code;
	struct Streampoint *point;

	ENTER( add_flow_particle );

	if ( list && xi && element && pointlist )
	{
		if ( ALLOCATE( point, struct Streampoint, 1 ))
		{
			point->xi[0] = xi[0];
			point->xi[1] = xi[1];
			point->xi[2] = xi[2];
			point->element = element;
			point->pointlist = pointlist;
			point->index = index;
			point->graphics_object = graphics_object;
			point->next = *list;

			*list = point;

			return_code = update_interactive_streampoint(
				(FE_value *)((*pointlist) + index), &element, field_cache,
				coordinate_field, xi, (FE_value *)NULL );
		}
		else
		{
			display_message(ERROR_MESSAGE,"add_flow_particle.  Could not allocate memory for streampoint");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"add_flow_particle.  Invalid argument");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* add_flow_particle */

int element_to_particle(struct FE_element *element,
	void *void_element_to_particle_data)
/*******************************************************************************
LAST MODIFIED : 17 March 1999

DESCRIPTION :
Converts a 3-D element into an array of particles.
==============================================================================*/
{	
	int return_code;
	struct Element_to_particle_data *element_to_particle_data;

	ENTER(element_to_particle);
	if (element&&(element_to_particle_data=
		(struct Element_to_particle_data *)void_element_to_particle_data))
	{
		/* determine if the element is required */
		if ((3==get_FE_element_dimension(element))&&
			((!element_to_particle_data->element_number)||
			(element_to_particle_data->element_number == get_FE_element_identifier(element))))
		{
			if (add_flow_particle(element_to_particle_data->list, 
				element_to_particle_data->xi,element,
				element_to_particle_data->pointlist,
				element_to_particle_data->index,
				element_to_particle_data->field_cache,
				element_to_particle_data->coordinate_field,
				element_to_particle_data->graphics_object))
			{
				element_to_particle_data->index++;
				element_to_particle_data->number_of_particles++;
			}
			return_code=1;
		}
		else
		{
			return_code=1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"element_to_particle.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* element_to_particle */
