/*******************************************************************************
FILE : computed_field_find_xi.c

LAST MODIFIED : 24 August 2006

DESCRIPTION :
Implements a special version of find_xi that uses OpenGL to accelerate the
lookup of the element.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <math.h>

#include "general/debug.h"
#include "general/matrix_vector.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_private.hpp"
#include "computed_field/computed_field_find_xi.h"
#include "computed_field/computed_field_find_xi_private.hpp"
#include "finite_element/finite_element_discretization.h"
#include "finite_element/finite_element_region.h"
#include "general/message.h"

#define MAX_FIND_XI_ITERATIONS 50

int Computed_field_iterative_element_conditional(struct FE_element *element,
	struct Computed_field_iterative_find_element_xi_data *data)
{
	double a[MAXIMUM_ELEMENT_XI_DIMENSIONS*MAXIMUM_ELEMENT_XI_DIMENSIONS],
		b[MAXIMUM_ELEMENT_XI_DIMENSIONS], d, sum;
	FE_value *derivatives, last_xi[MAXIMUM_ELEMENT_XI_DIMENSIONS], *values;
	int converged, i, indx[MAXIMUM_ELEMENT_XI_DIMENSIONS], iterations, j, k,
		number_of_xi, number_in_xi[MAXIMUM_ELEMENT_XI_DIMENSIONS], 
		number_of_xi_points_created[MAXIMUM_ELEMENT_XI_DIMENSIONS], return_code;
	struct FE_element_shape *shape;
	FE_value_triple *xi_points;

	ENTER(Computed_field_iterative_element_conditional);
	if (element && data)
	{
		number_of_xi = get_FE_element_dimension(element);
		if (number_of_xi <= data->number_of_values)
		{
			return_code = 1;
			if (data->found_number_of_xi != number_of_xi)
			{
				if (REALLOCATE(derivatives, data->found_derivatives, FE_value,
					data->number_of_values * number_of_xi))
				{
					data->found_derivatives = derivatives;
					data->found_number_of_xi = number_of_xi;
					return_code = 1;
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"Computed_field_iterative_element_conditional.  "
						"Unable to allocate derivative storage");
					return_code = 0;
				}
			}
			if (return_code)
			{
				values = data->found_values;
				derivatives = data->found_derivatives;
				shape = get_FE_element_shape(element);
				if (data->start_with_data_xi)
				{
					for (i = 0 ; i < number_of_xi ; i++)
					{
						last_xi[i] = data->xi[i];
					}
				}
				else
				{
					/* Find a good estimate of the centre to start with */
					for (i = 0 ; i < number_of_xi ; i++)
					{
						/* I want just one location in the middle */
						number_in_xi[i] = 1;
					}
					if (FE_element_shape_get_xi_points_cell_centres(shape,
							number_in_xi, number_of_xi_points_created, &xi_points))
					{
						for (i = 0 ; i < number_of_xi ; i++)
						{
							data->xi[i] = xi_points[0][i];
							last_xi[i] = xi_points[0][i];
						}
						DEALLOCATE(xi_points);
					}
					else
					{
						for (i = 0 ; i < number_of_xi ; i++)
						{
							data->xi[i] = 0.5;
							last_xi[i] = 0.5;
						}
					}
				}
				converged = 0;
				iterations = 0;
				while ((!converged) && return_code)
				{
					if ((CMZN_OK == data->field_cache->setMeshLocation(element, data->xi)) &&
						(CMZN_OK == cmzn_field_evaluate_real_with_derivatives(data->field, data->field_cache,
							data->number_of_values, values, number_of_xi, derivatives)))
					{
						/* least-squares approach: make the derivatives / right hand side
							vector into square system to solve for delta-Xi */
						for (i = 0; i < number_of_xi; i++)
						{
							for (j = 0; j < number_of_xi; j++)
							{
								sum = 0.0;
								for (k = 0; k < data->number_of_values; k++)
								{
									sum += (double)derivatives[k*number_of_xi + j] *
										(double)derivatives[k*number_of_xi + i];
								}
								a[i*number_of_xi + j] = sum;
							}
							sum = 0.0;
							for (k = 0; k < data->number_of_values; k++)
							{
								sum += (double)derivatives[k*number_of_xi + i] *
									((double)data->values[k] - (double)values[k]);
							}
							b[i] = sum;
						}
						if (LU_decompose(number_of_xi, a, indx, &d,/*singular_tolerance*/1.0e-12) &&
							LU_backsubstitute(number_of_xi, a, indx, b))
						{
							converged = 1;
							if (data->find_nearest_location)
							{
								// limit delta xi as search oscillates when surface
								// is very curved and data point is far away
								double limit = 0.2;
								if (iterations > 5)
								{
									limit /= 2.0;
									if (iterations > 20)
										limit /= 4.0;
								}
								double mag_dxi = 0.0;
								for (i = 0; i < number_of_xi; i++)
									mag_dxi += b[i]*b[i];
								mag_dxi = sqrt(mag_dxi);
								if (mag_dxi > limit)
								{
									for (i = 0; i < number_of_xi; i++)
										b[i] *= limit/mag_dxi;
								}
							}
							for (i = 0; i < number_of_xi; i++)
							{
								/* converged if all xi increments on or within tolerance */
								if (fabs(b[i]) > data->xi_tolerance)
								{
									converged = 0;
								}
								data->xi[i] += b[i];
							}
							iterations++;
							if (!converged)
							{
								FE_element_shape_limit_xi_to_element(shape,
									data->xi, data->xi_tolerance);
								if (iterations == MAX_FIND_XI_ITERATIONS)
								{
									/* too many iterations; give up */
									return_code = 0;
								}
								else if (1 < iterations)
								{
									/* give up if xi not changed this iteration; usually means
										the solution is outside this element */
									return_code = 0;
									for (i = 0; i < number_of_xi; i++)
									{
										if (fabs(data->xi[i] - last_xi[i]) > data->xi_tolerance)
										{
											return_code = 1;
										}
										last_xi[i] = data->xi[i];
									}
								}
							}
						}
						else
						{
							/* Probably singular matrix, no longer report error
								as a collapsed element gets reported on every
								pixel making it unusable.*/
							return_code = 0;
						}
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"Computed_field_iterative_element_conditional.  "
							"Could not evaluate field");
						return_code = 0;
					}
				}
				/* if field has more components than xi-directions, must
					check all components have converged */
				if (converged && (data->number_of_values > number_of_xi))
				{
					/* strategy for determining convergence is to multiply
						the derivative for each component by the last increment
						in xi, stored in b, and ensuring it is larger than the
						difference between target and current field values.
						Broadly, this means the given component is not wishing
						the solution to shift more than the last xi increment.
						Note that the increment in xi is doubled to make this
						test slightly less strict than original convergence */
					for (k = 0; k < data->number_of_values; k++)
					{
						sum = 0.0;
						for (i = 0; i < number_of_xi; i++)
						{
							sum += (double)derivatives[k*number_of_xi + i] * b[i];
						}
						if (2.0*fabs(sum) <
							fabs((double)data->values[k] - (double)values[k]))
						{
							return_code = 0;
						}
					}
				}
				if (data->find_nearest_location)
				{
					sum = 0.0;
					for (k = 0; k < data->number_of_values; k++)
					{
						sum += ((double)data->values[k] - (double)values[k]) *
							((double)data->values[k] - (double)values[k]);
					}
					if (!data->nearest_element ||
						(sum < data->nearest_element_distance_squared))
					{
						data->nearest_element = element;
						for (i = 0; i < number_of_xi; i++)
						{
							data->nearest_xi[i] = data->xi[i];
						}
						data->nearest_element_distance_squared = sum;
					}
				}
#if defined (DEBUG_CODE)
				display_message(INFORMATION_MESSAGE,
					"Computed_field_iterative_element_conditional.  "
					"Converged %d iterations %d\n", converged, iterations);
#endif /* defined (DEBUG_CODE) */

			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Computed_field_iterative_element_conditional.  "
				"Unable to solve underdetermined system");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_iterative_element_conditional.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_iterative_element_conditional */

#undef MAX_FIND_XI_ITERATIONS

int Computed_field_perform_find_element_xi(struct Computed_field *field,
	cmzn_fieldcache_id field_cache,
	const FE_value *values, int number_of_values,
	struct FE_element **element_address, FE_value *xi,
	cmzn_mesh_id search_mesh, int find_nearest)
{
	struct Computed_field_iterative_find_element_xi_data find_element_xi_data;
	int i, number_of_xi = -1, return_code;
	RealFieldValueCache *valueCache;

	ENTER(Computed_field_perform_find_element_xi);
	const int element_dimension = search_mesh ?
		cmzn_mesh_get_dimension(search_mesh) : cmzn_element_get_dimension(*element_address);
	if (field && (0 != (valueCache = dynamic_cast<RealFieldValueCache*>(field->getValueCache(*field_cache)))) &&
		values && (number_of_values == field->number_of_components) &&
		element_address && xi && (search_mesh || *element_address) &&
		(number_of_values >= element_dimension))
	{
		return_code = 1;
		Computed_field_find_element_xi_base_cache *cache = 0;
		if (valueCache->find_element_xi_cache && valueCache->find_element_xi_cache->cache_data)
		{
			cache = valueCache->find_element_xi_cache->cache_data;
			if (cache->number_of_values != number_of_values)
			{
				DEALLOCATE(cache->values);
				DEALLOCATE(cache->working_values);
			}
		}
		else
		{
			valueCache->find_element_xi_cache =
				CREATE(Computed_field_find_element_xi_cache)(new Computed_field_find_element_xi_base_cache());
			if (valueCache->find_element_xi_cache != 0)
			{
				cache = valueCache->find_element_xi_cache->cache_data;
			}
			else
			{
				return_code = 0;
			}
		}
		if (cache)
		{
			cache->in_perform_find_element_xi = 1;
		}
		if (return_code && !cache->values)
		{
			cache->time = field_cache->getTime();
			cache->number_of_values = number_of_values;
			if (!ALLOCATE(cache->values, FE_value, number_of_values))
			{
				display_message(ERROR_MESSAGE,
					"Computed_field_perform_find_element_xi.  "
					"Unable to allocate value memory.");
				return_code = 0;
			}
		}
		if (return_code && !cache->working_values)
		{
			if (!ALLOCATE(cache->working_values, FE_value, number_of_values))
			{
				display_message(ERROR_MESSAGE,
					"Computed_field_perform_find_element_xi.  "
					"Unable to allocate working value memory.");
				return_code = 0;
			}
		}
		if (return_code)
		{
			find_element_xi_data.values = cache->values;
			find_element_xi_data.found_values = cache->working_values;
			/* copy the source values */
			for (i = 0; i < number_of_values; i++)
			{
				find_element_xi_data.values[i] = values[i];
			}
			find_element_xi_data.field_cache = field_cache;
			find_element_xi_data.field = field;
			find_element_xi_data.number_of_values = number_of_values;
			find_element_xi_data.found_number_of_xi = 0;
			find_element_xi_data.found_derivatives = (FE_value *)NULL;
			find_element_xi_data.xi_tolerance = 1e-05;
			find_element_xi_data.find_nearest_location = find_nearest;
			find_element_xi_data.nearest_element = (struct FE_element *)NULL;
			find_element_xi_data.nearest_element_distance_squared = 0.0;
			find_element_xi_data.start_with_data_xi = 0;

			if (search_mesh)
			{
				*element_address = (struct FE_element *)NULL;

				/* Try the cached element first if it is in the mesh */
				if ((!find_nearest) && cache->element &&
					cmzn_mesh_contains_element(search_mesh, cache->element))
				{
					if (Computed_field_iterative_element_conditional(
						cache->element, &find_element_xi_data))
					{
						*element_address = cache->element;
					}
				}
				/* Now try every element */
				if (!*element_address)
				{
					cmzn_elementiterator_id iterator = cmzn_mesh_create_elementiterator(search_mesh);
					cmzn_element_id element = 0;
					while (0 != (element = cmzn_elementiterator_next_non_access(iterator)))
					{
						if (Computed_field_iterative_element_conditional(element, &find_element_xi_data))
						{
							*element_address = element;
							break;
						}
					}
					cmzn_elementiterator_destroy(&iterator);
				}
			}
			else
			{
				if ((!Computed_field_iterative_element_conditional(
					*element_address, &find_element_xi_data)))
				{
					*element_address = (struct FE_element *)NULL;
				}
			}
			/* If an exact match is not found then accept the closest one */
			if (!*element_address && find_nearest)
			{
				if (find_element_xi_data.nearest_element)
				{
					*element_address = find_element_xi_data.nearest_element;
					number_of_xi = get_FE_element_dimension(*element_address);
					for (i = 0 ; i < number_of_xi ; i++)
					{
						xi[i] = find_element_xi_data.nearest_xi[i];
					}
				}
			}
			else if (*element_address)
			{
				number_of_xi = get_FE_element_dimension(*element_address);
				for (i = 0 ; i < number_of_xi ; i++)
				{
					xi[i] = find_element_xi_data.xi[i];
				}
			}
			/* The search is valid even if the element wasn't found */
			return_code = 1;
			if (find_element_xi_data.found_derivatives)
			{
				DEALLOCATE(find_element_xi_data.found_derivatives);
			}
			/* Remember the element and search mesh in the cache */
			cache->element = *element_address;
			cache->set_search_mesh(search_mesh);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Computed_field_perform_find_element_xi.  "
				"Unable to allocate value memory.");
			return_code = 0;
		}
		if (cache)
		{
			cache->in_perform_find_element_xi = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_perform_find_element_xi.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_perform_find_element_xi */

struct Computed_field_find_element_xi_cache
	*CREATE(Computed_field_find_element_xi_cache)(
		Computed_field_find_element_xi_base_cache *cache_data)
/*******************************************************************************
LAST MODIFIED : 13 June 2008

DESCRIPTION :
Stores cache data for find_element_xi routines.
The new object takes ownership of the <cache_data>.
==============================================================================*/
{
	struct Computed_field_find_element_xi_cache *cache;

	ENTER(CREATE(Computed_field_find_element_xi_cache));
	cache = (struct Computed_field_find_element_xi_cache *)NULL;
	if (cache_data)
	{
		if (ALLOCATE(cache,struct Computed_field_find_element_xi_cache,1))
		{
			cache->cache_data = cache_data;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"CREATE(Computed_field_find_element_xi_cache).  Not enough memory");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CREATE(Computed_field_find_element_xi_cache).  Invalid argument(s)");
	}
	LEAVE;

	return (cache);
} /* CREATE(Computed_field_find_element_xi_cache) */

int DESTROY(Computed_field_find_element_xi_cache)
	(struct Computed_field_find_element_xi_cache **cache_address)
/*******************************************************************************
LAST MODIFIED : 13 June 2008

DESCRIPTION :
Frees memory/deaccess cache at <*cache_address>.
==============================================================================*/
{
	int return_code;
	class Computed_field_find_element_xi_base_cache

	ENTER(DESTROY(Computed_field_find_element_xi_cache));
	if (cache_address&&*cache_address)
	{
		if (((*cache_address)->cache_data) &&
			(*cache_address)->cache_data->in_perform_find_element_xi)
		{
			display_message(ERROR_MESSAGE,
				"DESTROY(Computed_field_find_element_xi_cache).  "
				"This cache cannot be destroyed.");
			return_code = 0;
		}
		else
		{
			delete (*cache_address)->cache_data;
			DEALLOCATE(*cache_address);
			return_code = 1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(Computed_field_find_element_xi_cache).  Missing cache");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Computed_field_find_element_xi_cache) */
