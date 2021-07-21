/*******************************************************************************
FILE : computed_field_integration.cpp

LAST MODIFIED : 24 August 2006

DESCRIPTION :
Implements a computed field which integrates along elements, including
travelling between adjacent elements, using the faces for 2D and 3D elements
and the nodes for 1D elements.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <math.h>
#include <stdio.h>
#include "opencmiss/zinc/fieldmodule.h"
#include "opencmiss/zinc/mesh.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_composite.h"
#include "computed_field/computed_field_finite_element.h"
#include "computed_field/computed_field_private.hpp"
#include "computed_field/computed_field_set.h"
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_mesh.hpp"
#include "finite_element/finite_element_region.h"
#include "finite_element/finite_element_adjacent_elements.h"
#include "general/compare.h"
#include "general/debug.h"
#include "general/mystring.h"
#include "general/indexed_list_private.h"
#include "general/indexed_multi_range.h"
#include "general/list_private.h"
#include "general/message.h"
#include "computed_field/computed_field_integration.h"
#include "mesh/cmiss_element_private.hpp"

namespace {

class Computed_field_integration_package : public Computed_field_type_package
{
public:
	cmzn_region *root_region;
};

struct Computed_field_element_integration_mapping
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
==============================================================================*/
{
	/* Holds the pointer to the element */
	FE_element *element;
	/* The values for the xi1 = 0, xi2 = 0, xi3 = 0 corner,
		a vector of size number_of_components. */
	FE_value *values;

	int access_count;
}; /* Computed_field_element_integration_mapping */

struct Computed_field_node_integration_mapping
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
==============================================================================*/
{
	/* Holds the pointer to the node, can't use node as this is a symbol in the LISTS */
	FE_node *node_ptr;
	/* The field values for the node. */
	FE_value *values;

	int access_count;
}; /* Computed_field_node_integration_mapping */

struct Computed_field_element_integration_mapping_fifo
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Simple linked-list node structure for building a FIFO stack for the mapping
structure - needed for consistent growth of integration.
==============================================================================*/
{
	Computed_field_element_integration_mapping *mapping_item;
	Computed_field_element_integration_mapping_fifo *next;
}; /* Computed_field_element_integration_mapping_fifo */

struct Computed_field_integration_has_values_data
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
==============================================================================*/
{
	int number_of_values;
	FE_value *values;
}; /* Computed_field_integration_has_values_data */

Computed_field_element_integration_mapping *CREATE(Computed_field_element_integration_mapping)
	(FE_element *element, int number_of_components)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Creates a basic Computed_field with the given <name>. Its type is initially
COMPUTED_FIELD_CONSTANT with 1 component, returning a value of zero.
==============================================================================*/
{
	int i;
	Computed_field_element_integration_mapping *mapping_item;

	ENTER(CREATE(Computed_field_element_integration_mapping));

	if (element &&
		ALLOCATE(mapping_item,Computed_field_element_integration_mapping,1) &&
		ALLOCATE(mapping_item->values, FE_value, number_of_components))
	{
		mapping_item->element = ACCESS(FE_element)(element);
		for (i = 0 ; i < number_of_components ; i++)
		{
			mapping_item->values[i] = 0.0;
		}
		mapping_item->access_count = 0;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CREATE(Computed_field_element_integration_mapping).  Not enough memory");
		mapping_item = (Computed_field_element_integration_mapping *)NULL;
	}
	LEAVE;

	return (mapping_item);
} /* CREATE(Computed_field_element_integration_mapping) */

int DESTROY(Computed_field_element_integration_mapping)
	  (Computed_field_element_integration_mapping **mapping_address)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Frees memory/deaccess mapping at <*mapping_address>.
==============================================================================*/
{
	int return_code;

	ENTER(DESTROY(Computed_field_element_integration_mapping));
	if (mapping_address&&*mapping_address)
	{
		if (0 >= (*mapping_address)->access_count)
		{
			if ((*mapping_address)->element)
			{
				DEACCESS(FE_element)(&((*mapping_address)->element));
			}
			DEALLOCATE((*mapping_address)->values);
			DEALLOCATE(*mapping_address);
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"DESTROY(Computed_field_element_integration_mapping).  Positive access_count");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(Computed_field_element_integration_mapping).  Missing mapping");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Computed_field_element_integration_mapping) */

DECLARE_OBJECT_FUNCTIONS(Computed_field_element_integration_mapping)
DECLARE_LIST_TYPES(Computed_field_element_integration_mapping);
FULL_DECLARE_INDEXED_LIST_TYPE(Computed_field_element_integration_mapping);
DECLARE_INDEXED_LIST_MODULE_FUNCTIONS(Computed_field_element_integration_mapping,
  element,FE_element *,compare_pointer)
DECLARE_INDEXED_LIST_FUNCTIONS(Computed_field_element_integration_mapping)
DECLARE_FIND_BY_IDENTIFIER_IN_INDEXED_LIST_FUNCTION(
	Computed_field_element_integration_mapping,
	element,FE_element *,compare_pointer)

Computed_field_node_integration_mapping *CREATE(Computed_field_node_integration_mapping)
	(FE_node *node, int number_of_components)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Creates a basic Computed_field with the given <name>. Its type is initially
COMPUTED_FIELD_CONSTANT with 1 component, returning a value of zero.
==============================================================================*/
{
	int i;
	Computed_field_node_integration_mapping *mapping_item;

	ENTER(CREATE(Computed_field_node_integration_mapping));

	if (node &&
		ALLOCATE(mapping_item,Computed_field_node_integration_mapping,1) &&
		ALLOCATE(mapping_item->values, FE_value, number_of_components))
	{
		mapping_item->node_ptr = node->access();
		for (i = 0 ; i < number_of_components ; i++)
		{
			mapping_item->values[i] = 0.0;
		}
		mapping_item->access_count = 0;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CREATE(Computed_field_node_integration_mapping).  Not enough memory");
		mapping_item = (Computed_field_node_integration_mapping *)NULL;
	}
	LEAVE;

	return (mapping_item);
} /* CREATE(Computed_field_node_integration_mapping) */

int DESTROY(Computed_field_node_integration_mapping)
	  (Computed_field_node_integration_mapping **mapping_address)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Frees memory/deaccess mapping at <*mapping_address>.
==============================================================================*/
{
	int return_code;

	ENTER(DESTROY(Computed_field_node_integration_mapping));
	if (mapping_address&&*mapping_address)
	{
		if (0 >= (*mapping_address)->access_count)
		{
			if ((*mapping_address)->node_ptr)
			{
				cmzn_node::deaccess((*mapping_address)->node_ptr);
			}
			DEALLOCATE((*mapping_address)->values);
			DEALLOCATE(*mapping_address);
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"DESTROY(Computed_field_node_integration_mapping).  Positive access_count");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(Computed_field_node_integration_mapping).  Missing mapping");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Computed_field_node_integration_mapping) */

DECLARE_OBJECT_FUNCTIONS(Computed_field_node_integration_mapping)
DECLARE_LIST_TYPES(Computed_field_node_integration_mapping);
FULL_DECLARE_INDEXED_LIST_TYPE(Computed_field_node_integration_mapping);
DECLARE_INDEXED_LIST_MODULE_FUNCTIONS(Computed_field_node_integration_mapping,
  node_ptr,FE_node *,compare_pointer)
DECLARE_INDEXED_LIST_FUNCTIONS(Computed_field_node_integration_mapping)
DECLARE_FIND_BY_IDENTIFIER_IN_INDEXED_LIST_FUNCTION(
	Computed_field_node_integration_mapping,
	node_ptr,FE_node *,compare_pointer)

int write_Computed_field_element_integration_mapping(
	Computed_field_element_integration_mapping *mapping,
	void *user_data)
{
	USE_PARAMETER(user_data);
	printf("Mapping %p Element %p (%f)\n",
		mapping, mapping->element, mapping->values[0]);

	return( 1 );
}

const char computed_field_integration_type_string[] = "integration";
const char computed_field_xi_texture_coordinates_type_string[] = "xi_texture_coordinates";

class Computed_field_integration : public Computed_field_core
{
public:
	FE_value cached_time;
	cmzn_mesh_id mesh;
	cmzn_element_id seed_element;
	/* Whether to integrate wrt to each coordinate separately or wrt the
		magnitude of the coordinate field */
	int magnitude_coordinates;
	LIST(Computed_field_element_integration_mapping) *texture_mapping;
	LIST(Computed_field_node_integration_mapping) *node_mapping;
	/* last mapping successfully used by Computed_field_find_element_xi so
		that it can first try this element again */
	Computed_field_element_integration_mapping *find_element_xi_mapping;

	Computed_field_integration(cmzn_mesh_id mesh, cmzn_element_id seed_element,
		int magnitude_coordinates) :
		Computed_field_core(),
		mesh(cmzn_mesh_access(mesh)),
		seed_element(seed_element->access()),
		magnitude_coordinates(magnitude_coordinates)
	{
		cached_time = 0;
		texture_mapping =
			(LIST(Computed_field_element_integration_mapping) *)NULL;
		node_mapping =
			(LIST(Computed_field_node_integration_mapping) *)NULL;
		find_element_xi_mapping=
			(Computed_field_element_integration_mapping *)NULL;
	};

	~Computed_field_integration();

private:
	Computed_field_core *copy();

	const char *get_type_string()
	{
		return(computed_field_integration_type_string);
	}

	int compare(Computed_field_core* other_field);

	int list();

	char* get_command_string();

	int integrate_path(FE_element *element,
		FE_value *initial_values, FE_value *initial_xi, FE_value *final_xi,
		int number_of_gauss_points, cmzn_fieldcache& workingCache, Computed_field *integrand,
		int magnitude_coordinates, Computed_field *coordinate_field,
		FE_value *values);

	int add_neighbours(cmzn_fieldcache& workingCache,
		Computed_field_element_integration_mapping *mapping_item,
		LIST(Computed_field_element_integration_mapping) *texture_mapping,
		Computed_field_element_integration_mapping_fifo **last_to_be_checked,
		Computed_field *integrand,
		int magnitude_coordinates, Computed_field *coordinate_field,
		AdjacentElements1d **adjacentElements1d,
		LIST(Computed_field_element_integration_mapping) *previous_texture_mapping,
		ZnReal time_step,
		LIST(Computed_field_node_integration_mapping) *node_mapping);

	int calculate_mapping(FE_value time);

	int clear_cache();

	bool is_defined_at_location(cmzn_fieldcache& cache);

	virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
	{
		return this->evaluateDerivativeFiniteDifference(cache, inValueCache, fieldDerivative);
	}

};

int Computed_field_integration::integrate_path(FE_element *element,
	FE_value *initial_values, FE_value *initial_xi, FE_value *final_xi,
	int number_of_gauss_points, cmzn_fieldcache& workingCache, Computed_field *integrand,
	int magnitude_coordinates, Computed_field *coordinate_field,
	FE_value *values)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Calculates the <values> by adding to the initial values the integrals evaluated
along the line from <initial_xi> to <final_xi> using the <number_of_gauss_points>
specified.
time is supplied in the workingCache
==============================================================================*/
{
	double dsdxi;
#define MAXIMUM_GAUSS_POINTS_DEFINED (2)
	FE_value final_position, initial_position,
		/* These two arrays are lower left diagonals matrices which for each row
			have the positions and weights of a gauss point scheme for integrating
			with the number of points corresponding to the row. */
		gauss_positions[MAXIMUM_GAUSS_POINTS_DEFINED][MAXIMUM_GAUSS_POINTS_DEFINED]
			= {{0.5, 0},
			  {0.25, 0.75}},
		gauss_weights[MAXIMUM_GAUSS_POINTS_DEFINED][MAXIMUM_GAUSS_POINTS_DEFINED]
			= {{1, 0},
			  {0.5, 0.5}},
		xi[MAXIMUM_ELEMENT_XI_DIMENSIONS],
		xi_vector[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	int coordinate_dimension, j, k, m, element_dimension, return_code;

	ENTER(Computed_field_integration_integrate_path);
	if (integrand && coordinate_field &&
		initial_values && values && initial_xi && final_xi &&
		(number_of_gauss_points > 0) &&
		(number_of_gauss_points <= MAXIMUM_GAUSS_POINTS_DEFINED))
	{
		return_code = 1;
		for (k = 0; k < MAXIMUM_ELEMENT_XI_DIMENSIONS; k++)
		{
			xi[k] = 0.0;
		}
		element_dimension = element->getDimension();
		coordinate_dimension = cmzn_field_get_number_of_components(coordinate_field);
		if (Computed_field_is_type_xi_coordinates(coordinate_field, NULL))
		{
			/* Unlike the xi field we only deal with top level elements of a
				single dimension so we can match that dimension for our number
				of coordinates */
			coordinate_dimension = element_dimension;
		}
		if (magnitude_coordinates)
		{
			values[0] = initial_values[0];
		}
		else
		{
			for (j = 0 ; j < coordinate_dimension ; j++)
			{
				values[j] = initial_values[j];
			}
		}
		for (k = 0 ; k < element_dimension ; k++)
		{
			xi_vector[k] = final_xi[k] - initial_xi[k];
		}
		const FieldDerivative& fieldDerivative = *element->getMesh()->getFieldDerivative(/*order*/1);
		for (m = 0 ; m < number_of_gauss_points ; m++)
		{
			final_position = gauss_positions[number_of_gauss_points - 1][m];
			initial_position = (1.0 - final_position);
			for (k = 0 ; k < element_dimension ; k++)
			{
				xi[k] = initial_xi[k] * initial_position
					+ final_xi[k] * final_position;
			}
			/* Integrand elements should always be top level */
			workingCache.setMeshLocation(element, xi);
			const DerivativeValueCache* coordinateDerivativeCache = coordinate_field->evaluateDerivative(workingCache, fieldDerivative);
			const RealFieldValueCache* integrandValueCache = RealFieldValueCache::cast(integrand->evaluate(workingCache));
			if ((coordinateDerivativeCache) && (integrandValueCache))
			{
				const FE_value *coordinateDerivatives = coordinateDerivativeCache->values;
				if (magnitude_coordinates)
				{
					for (k = 0; k < element_dimension; k++)
					{
						dsdxi = 0.0;
						for (j = 0; j < coordinate_dimension; j++)
						{
							dsdxi +=
								coordinateDerivatives[j * element_dimension + k] *
								coordinateDerivatives[j * element_dimension + k];
						}
						dsdxi = sqrt(dsdxi);
						values[0] += integrandValueCache->values[0] *
							xi_vector[k] * dsdxi *
							gauss_weights[number_of_gauss_points - 1][m];
					}
				}
				else
				{
					for (k = 0; k < element_dimension; k++)
					{
						for (j = 0; j < coordinate_dimension; j++)
						{
							values[j] += integrandValueCache->values[0] * xi_vector[k] *
								coordinateDerivatives[j * element_dimension + k] *
								gauss_weights[number_of_gauss_points - 1][m];
						}
					}
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_integration_integrate_path.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return(return_code);
} /* Computed_field_integration_integrate_path */

int Computed_field_integration::add_neighbours(cmzn_fieldcache& workingCache,
	Computed_field_element_integration_mapping *mapping_item,
	LIST(Computed_field_element_integration_mapping) *texture_mapping,
	Computed_field_element_integration_mapping_fifo **last_to_be_checked,
	Computed_field *integrand,
	int magnitude_coordinates, Computed_field *coordinate_field,
	AdjacentElements1d **adjacentElements1d,
	LIST(Computed_field_element_integration_mapping) *previous_texture_mapping,
	ZnReal time_step,
	LIST(Computed_field_node_integration_mapping) *node_mapping)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Add the neighbours that haven't already been put in the texture_mapping list and
puts each new member in the texture_mapping list and the to_be_checked list.
If <previous_texture_mapping> is set then this is being used to update a time
varying integration and the behaviour is significantly different.
time is supplied in the workingCache.
==============================================================================*/
{
	FE_value final_xi[MAXIMUM_ELEMENT_XI_DIMENSIONS],
		initial_xi[MAXIMUM_ELEMENT_XI_DIMENSIONS],
		xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	int element_dimension, face_number, i, j, k, number_of_components,
		number_of_element_field_nodes, number_of_faces,
		number_of_neighbour_elements, return_code;
	Computed_field_element_integration_mapping *mapping_neighbour;
	Computed_field_element_integration_mapping_fifo *fifo_node;
	Computed_field_node_integration_mapping *node_map;
	FE_element *integrate_element = NULL, **neighbour_elements;
	FE_element_shape *shape;
	FE_field *fe_field;
	FE_node **element_field_nodes_array;
	LIST(FE_field) *fe_field_list;

	ENTER(Computed_field_integration_add_neighbours);
	if (mapping_item && texture_mapping && last_to_be_checked &&
		(*last_to_be_checked))
	{
		return_code=CMZN_OK;
		element_dimension = get_FE_element_dimension(mapping_item->element);
		number_of_faces = element_dimension*2;
		if (magnitude_coordinates)
		{
			number_of_components = 1;
		}
		else
		{
			number_of_components = cmzn_field_get_number_of_components(
				coordinate_field);
		}
		for (i = 0; (return_code == CMZN_OK)&&(i<number_of_faces);i++)
		{
			if (element_dimension == 1)
			{
				// if we have 1D elements then we use the nodes to get to the
				// adjacent elements; normally use the faces
				if (!(*adjacentElements1d))
				{
					*adjacentElements1d = AdjacentElements1d::create(mesh, coordinate_field);
				}
				if (!(*adjacentElements1d && (*adjacentElements1d)->getAdjacentElements(mapping_item->element, i,
					&number_of_neighbour_elements, &neighbour_elements)))
				{
					number_of_neighbour_elements = 0;
				}
			}
			else
			{
				/* else we need an xi location to find the appropriate face for */
				xi[0] = 0.5;
				xi[1] = 0.5;
				xi[2] = 0.5;
				switch (i)
				{
					case 0:
					{
						xi[0] = 0.0;
					} break;
					case 1:
					{
						xi[0] = 1.0;
					} break;
					case 2:
					{
						xi[1] = 0.0;
					} break;
					case 3:
					{
						xi[1] = 1.0;
					} break;
					case 4:
					{
						xi[2] = 0.0;
					} break;
					case 5:
					{
						xi[2] = 1.0;
					} break;
				}
				shape = get_FE_element_shape(mapping_item->element);
				if (!(FE_element_shape_find_face_number_for_xi(shape, xi, &face_number) &&
					(((CMZN_OK == adjacent_FE_element(mapping_item->element, face_number,
						&number_of_neighbour_elements, &neighbour_elements))))))
				{
					number_of_neighbour_elements = 0;
				}
			}
			if (number_of_neighbour_elements)
			{
				for (j = 0 ; j < number_of_neighbour_elements ; j++)
				{
					if(!(mapping_neighbour = FIND_BY_IDENTIFIER_IN_LIST(
						Computed_field_element_integration_mapping, element)
						(neighbour_elements[j], texture_mapping)))
					{
						if (ALLOCATE(fifo_node,
							Computed_field_element_integration_mapping_fifo,1)&&
							(mapping_neighbour =
								CREATE(Computed_field_element_integration_mapping)(
									neighbour_elements[j], number_of_components)))
						{
							for (k = 0 ; k < element_dimension ; k++)
							{
								initial_xi[k] = 0.0;
								final_xi[k] = 0.0;
							}
							switch (i)
							{
								case 0:
								case 2:
								case 4:
								{
									initial_xi[i / 2] = 1.0;
									integrate_element = mapping_neighbour->element;
								} break;
								case 1:
								case 3:
								case 5:
								{
									final_xi[(i - 1)/ 2] = 1.0;
									integrate_element = mapping_item->element;
								} break;
							}
							integrate_path(integrate_element,
								mapping_item->values, initial_xi, final_xi,
								/*number_of_gauss_points*/2, workingCache, integrand,
								magnitude_coordinates, coordinate_field,
								mapping_neighbour->values);
							USE_PARAMETER(previous_texture_mapping);
							USE_PARAMETER(time_step);
							if (ADD_OBJECT_TO_LIST(Computed_field_element_integration_mapping)(
								mapping_neighbour, texture_mapping))
							{
								/* fill the fifo_node for the mapping_item; put at end of list */
								fifo_node->mapping_item=mapping_neighbour;
								fifo_node->next=
									(Computed_field_element_integration_mapping_fifo *)NULL;
								(*last_to_be_checked)->next = fifo_node;
								(*last_to_be_checked) = fifo_node;
							}
							else
							{
								printf("Error adding neighbour\n");
								write_Computed_field_element_integration_mapping(mapping_neighbour, NULL);
								printf("Texture mapping list\n");
								FOR_EACH_OBJECT_IN_LIST(Computed_field_element_integration_mapping)(
									write_Computed_field_element_integration_mapping, NULL, texture_mapping);
								DEALLOCATE(fifo_node);
								DESTROY(Computed_field_element_integration_mapping)(
									&mapping_neighbour);
								return_code=CMZN_ERROR_GENERAL;
							}
						}
						else
						{
							display_message(ERROR_MESSAGE,
								"Computed_field_integration::add_neighbours.  "
								"Unable to allocate member");
							DEALLOCATE(fifo_node);
							return_code=CMZN_ERROR_MEMORY;
						}
					}
				}
				DEALLOCATE(neighbour_elements);
			}
		}
		/* Try to add mappings for nodes too. */
		if ((fe_field_list = Computed_field_get_defining_FE_field_list(coordinate_field))
			&& (1 == NUMBER_IN_LIST(FE_field)(fe_field_list)))
		{
			fe_field = FIRST_OBJECT_IN_LIST_THAT(FE_field)(
				(LIST_CONDITIONAL_FUNCTION(FE_field) *)NULL, (void *)NULL,
				fe_field_list);
		}
		else
		{
			/* We would like this to work for xi and other computed fields that
				have no fe_fields so we use the 'feature' that allows us to pass NULL */
			fe_field = (FE_field *)NULL;
		}
		if (fe_field_list)
		{
			DESTROY(LIST(FE_field))(&fe_field_list);
		}
		shape = get_FE_element_shape(mapping_item->element);
		if (node_mapping
			&& FE_element_shape_is_line(shape)
			&& (CMZN_OK == calculate_FE_element_field_nodes(mapping_item->element,
				/*inherit_face_number*/-1,
				fe_field, &number_of_element_field_nodes,
				&element_field_nodes_array, mapping_item->element)))
		{
			/* Make assumptions about the distribution of the nodes */
			if (number_of_element_field_nodes == pow(2.0, element_dimension))
			{
				/* Add nodes not already included */
				for (i = 0 ; i < number_of_element_field_nodes ; i++)
				{
					if (!(FIND_BY_IDENTIFIER_IN_LIST(
						Computed_field_node_integration_mapping, node_ptr)
						(element_field_nodes_array[i], node_mapping)))
					{
						node_map = CREATE(Computed_field_node_integration_mapping)(
							element_field_nodes_array[i], number_of_components);

						for (k = 0 ; k < element_dimension ; k++)
						{
							initial_xi[k] = 0.0;
							if (i & (1 << k))
							{
								final_xi[k] = 1.0;
							}
							else
							{
								final_xi[k] = 0.0;
							}
						}
						integrate_path(mapping_item->element,
							mapping_item->values, initial_xi, final_xi,
							/*number_of_gauss_points*/2, workingCache, integrand,
							magnitude_coordinates, coordinate_field,
							node_map->values);

						ADD_OBJECT_TO_LIST(Computed_field_node_integration_mapping)(
							node_map, node_mapping);
					}
				}
			}
			/* else we don't know what we are looking at so don't do anything */

			for (i = 0 ; i < number_of_element_field_nodes ; i++)
			{
				cmzn_node::deaccess(element_field_nodes_array[i]);
			}
			DEALLOCATE(element_field_nodes_array);
		}

	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_integration::add_neighbours.  Invalid argument(s)");
		return_code=CMZN_ERROR_ARGUMENT;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_integration::add_neighbours */

/***************************************************************************//**
 * Calculates the mapping for the time and location details in the cache.
 */
int Computed_field_integration::calculate_mapping(FE_value time)
{
	int return_code;
	Computed_field *integrand, *coordinate_field;
	Computed_field_element_integration_mapping *mapping_item;
	Computed_field_element_integration_mapping_fifo *fifo_node,
		*first_to_be_checked, *last_to_be_checked;

	if (field && (integrand = field->source_fields[0])
		&& (coordinate_field = field->source_fields[1]))
	{
		// use a temporary working cache
		cmzn_fieldmodule_id field_module = cmzn_field_get_fieldmodule(field);
		cmzn_fieldcache_id field_cache = cmzn_fieldmodule_create_fieldcache(field_module);
		field_cache->setTime(time);
		return_code = CMZN_OK;
		first_to_be_checked=last_to_be_checked=
			(Computed_field_element_integration_mapping_fifo *)NULL;
		AdjacentElements1d *adjacentElements1d = 0;
		if ((texture_mapping = CREATE_LIST(Computed_field_element_integration_mapping)())
			&& (node_mapping = CREATE_LIST(Computed_field_node_integration_mapping)()))
		{
			if (ALLOCATE(fifo_node,
				Computed_field_element_integration_mapping_fifo,1)&&
				(mapping_item=CREATE(Computed_field_element_integration_mapping)(
					 seed_element, cmzn_field_get_number_of_components(field))))
			{
				ADD_OBJECT_TO_LIST(Computed_field_element_integration_mapping)
					(mapping_item, texture_mapping);
				/* fill the fifo_node for the mapping_item; put at end of list */
				fifo_node->mapping_item=mapping_item;
				fifo_node->next=
					(Computed_field_element_integration_mapping_fifo *)NULL;
				first_to_be_checked=last_to_be_checked=fifo_node;
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Computed_field_integration::calculate_mapping.  "
					"Unable to allocate member");
				DEALLOCATE(fifo_node);
				return_code=CMZN_ERROR_MEMORY;
			}
			if (CMZN_OK == return_code)
			{
				while ((CMZN_OK == return_code) && first_to_be_checked)
				{
					return_code = add_neighbours(*field_cache,
						first_to_be_checked->mapping_item, texture_mapping,
						&last_to_be_checked, integrand,
						magnitude_coordinates, coordinate_field,
						&adjacentElements1d,
						(LIST(Computed_field_element_integration_mapping) *)NULL,
						0.0, node_mapping);

#if defined (DEBUG_CODE)
					printf("Item removed\n");
					write_Computed_field_element_integration_mapping(mapping_item, NULL);
#endif /* defined (DEBUG_CODE) */

					/* remove first_to_be_checked */
					fifo_node=first_to_be_checked;
					if (!(first_to_be_checked=first_to_be_checked->next))
					{
						last_to_be_checked=
							(Computed_field_element_integration_mapping_fifo *)NULL;
					}
					DEALLOCATE(fifo_node);

#if defined (DEBUG_CODE)
					printf("Texture mapping list\n");
					FOR_EACH_OBJECT_IN_LIST(Computed_field_element_integration_mapping)(
						write_Computed_field_element_integration_mapping, NULL, texture_mapping);
					//printf("To be checked list\n");
					//FOR_EACH_OBJECT_IN_LIST(Computed_field_element_integration_mapping)(
					//	write_Computed_field_element_integration_mapping, NULL, to_be_checked);
#endif /* defined (DEBUG_CODE) */
				}
				cached_time = time;
				if (0 == NUMBER_IN_LIST(Computed_field_node_integration_mapping)(node_mapping))
				{
					DESTROY(LIST(Computed_field_node_integration_mapping)(&node_mapping));
				}
				find_element_xi_mapping=
					(Computed_field_element_integration_mapping *)NULL;
			}
			/* clean up to_be_checked list */
			while (first_to_be_checked)
			{
				fifo_node = first_to_be_checked;
				first_to_be_checked = first_to_be_checked->next;
				DEALLOCATE(fifo_node);
			}
			if (return_code != CMZN_OK)
			{
				DESTROY_LIST(Computed_field_element_integration_mapping)(&texture_mapping);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Computed_field_integration::calculate_mapping.  "
				"Unable to create mapping list.");
			return_code=CMZN_ERROR_GENERAL;
		}
		delete adjacentElements1d;
		cmzn_fieldcache_destroy(&field_cache);
		cmzn_fieldmodule_destroy(&field_module);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_integration::calculate_mapping.  "
			"Invalid arguments.");
		return_code=CMZN_ERROR_ARGUMENT;
	}
	return (return_code);
} /* Computed_field_integration::calculate_mapping */

int Computed_field_element_integration_mapping_has_values(
	Computed_field_element_integration_mapping *mapping, void *user_data)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Compares the user_data values with the offsets in the <mapping>
==============================================================================*/
{
	int i, return_code;
	Computed_field_integration_has_values_data *data;

	ENTER(Computed_field_element_integration_mapping_has_values);
	if (mapping && (data =
			(Computed_field_integration_has_values_data *)user_data))
	{
		return_code = 1;
		for (i = 0 ; return_code && (i < data->number_of_values) ; i++)
		{
			if (data->values[i] != mapping->values[i])
			{
				return_code = 0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_element_integration_mapping_has_values.  "
			"Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_element_integration_mapping_has_values */

Computed_field_integration::~Computed_field_integration()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Clear the type specific data used by this type.
==============================================================================*/
{

	ENTER(Computed_field_integration::~Computed_field_integration);
	if (field)
	{
		cmzn_mesh_destroy(&mesh);
		if (seed_element)
		{
			DEACCESS(FE_element)(&(seed_element));
		}
		if (texture_mapping)
		{
			DESTROY_LIST(Computed_field_element_integration_mapping)
				(&texture_mapping);
		}
		if (node_mapping)
		{
			DESTROY_LIST(Computed_field_node_integration_mapping)
				(&node_mapping);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_integration::~Computed_field_integration.  "
			"Invalid arguments.");
	}
	LEAVE;

} /* Computed_field_integration::~Computed_field_integration */

Computed_field_core* Computed_field_integration::copy()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Copy the type specific data used by this type.
==============================================================================*/
{
	Computed_field_integration *core =
		new Computed_field_integration(mesh, seed_element, magnitude_coordinates);

	return (core);
} /* Computed_field_integration::copy */

int Computed_field_integration::clear_cache()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_integration::clear_cache);
	if (field)
	{
		if (find_element_xi_mapping)
		{
			DEACCESS(Computed_field_element_integration_mapping)
				(&find_element_xi_mapping);
		}
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_integration::clear_cache.  "
			"Invalid arguments.");
		return_code = 0;
	}
	LEAVE;

	return(return_code);
} /* Computed_field_integration::clear_cache */

int Computed_field_integration::compare(Computed_field_core *other_core)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Compare the type specific data
==============================================================================*/
{
	int return_code;
	Computed_field_integration *other;

	ENTER(Computed_field_integration::compare);
	if (field && (other = dynamic_cast<Computed_field_integration*>(other_core)))
	{
		if (seed_element == other->seed_element)
		{
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
	LEAVE;

	return (return_code);
} /* Computed_field_integration::compare */

/***************************************************************************//**
 * @return  true if the all the source fields are defined in the supplied
 * <element> and the mapping is defined for this element.
 */
bool Computed_field_integration::is_defined_at_location(cmzn_fieldcache& cache)
{
	return (0 != field->evaluate(cache));
#if defined (TODO_CODE)
	// @TODO: resurrect the slightly more efficient old code?
	FE_value element_to_top_level[9];
	int return_code;
	FE_element *top_level_element;

	ENTER(Computed_field_default::is_defined_at_location);
	if (field && location)
	{
		return_code = Computed_field_core::is_defined_at_location(location);
		if (return_code)
		{
			Field_location_element_xi *element_xi_location;
			Field_location_node *node_location;

			element_xi_location =
				dynamic_cast<Field_location_element_xi*>(location);
			if (element_xi_location != 0)
			{
				FE_element* element = element_xi_location->get_element();

				if (!texture_mapping)
				{
					/* Try time 0 */
					calculate_mapping(/*time*/0.0);
				}
				else
				{
					/* Use the mapping from whatever time */
				}
				/* 1. Get top_level_element for types that must be calculated on them */
				if (FE_element_is_top_level(element, (void *)NULL))
				{
					top_level_element=element;
				}
				else
				{
					/* check or get top_level element,
						we don't have a top_level element to test with */
					if (!(top_level_element=FE_element_get_top_level_element_conversion(
						element,(struct FE_element *)NULL,
						CMZN_ELEMENT_FACE_TYPE_ALL, element_to_top_level)))
					{
						return_code=0;
					}
				}
				/* 2. Calculate the field */
				if (return_code && texture_mapping)
				{
					if (FIND_BY_IDENTIFIER_IN_LIST
						(Computed_field_element_integration_mapping,element)
						(top_level_element, texture_mapping))
					{
						return_code = 1;
					}
					else
					{
						return_code = 0;
					}
				}
			}
			else if (0 != (node_location =
				dynamic_cast<Field_location_node*>(location)))
			{
				FE_node *node = node_location->get_node();

				if (!texture_mapping)
				{
					/* Try time 0 */
					calculate_mapping(/*time*/0.0);
				}
				else
				{
					/* Use the mapping from whatever time */
				}
				/* If we calculated the mapping but no node_mapping was created then
					the field is not defined at nodes. */
				if (node_mapping)
				{
					/* If we have a mapping look for the node */
					if (FIND_BY_IDENTIFIER_IN_LIST
						(Computed_field_node_integration_mapping,node_ptr)
						(node, node_mapping))
					{
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
				return_code = 0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_default::is_defined_at_location.  "
			"Invalid arguments.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
#endif // defined (TODO_CODE)
}

int Computed_field_integration::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache& valueCache = RealFieldValueCache::cast(inValueCache);
	FE_value time = cache.getTime();

	FE_value element_to_top_level[9],initial_xi[MAXIMUM_ELEMENT_XI_DIMENSIONS],
		top_level_xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	int coordinate_dimension, element_dimension, i, j, k,
		top_level_element_dimension = -1;

	int return_code = 1;
	const Field_location_element_xi *element_xi_location;
	const Field_location_node *node_location;

	if (element_xi_location = cache.get_location_element_xi())
	{
		FE_element* element = element_xi_location->get_element();
		FE_element* top_level_element = element_xi_location->get_top_level_element();
		const FE_value* xi = element_xi_location->get_xi();

		Computed_field_element_integration_mapping *mapping;

		if (!texture_mapping)
		{
			calculate_mapping(time);
		}
		else
		{
			if ((time != cached_time)
				&& (Computed_field_has_multiple_times(field->source_fields[0])
					|| Computed_field_has_multiple_times(field->source_fields[1])))
			{
				DESTROY_LIST(Computed_field_element_integration_mapping)
					(&texture_mapping);
				calculate_mapping(time);
			}
		}
		/* 1. Get top_level_element for types that must be calculated on them */
		element_dimension=get_FE_element_dimension(element);
		if (FE_element_is_top_level(element, (void *)NULL))
		{
			top_level_element=element;
			for (i=0;i<element_dimension;i++)
			{
				top_level_xi[i]=xi[i];
			}
			/* do not set element_to_top_level */
			top_level_element_dimension=element_dimension;
		}
		else
		{
			/* check or get top_level element and xi coordinates for it */
			top_level_element=FE_element_get_top_level_element_conversion(
				element, top_level_element,
				CMZN_ELEMENT_FACE_TYPE_ALL, element_to_top_level);
			if (top_level_element != 0)
			{
				/* convert xi to top_level_xi */
				top_level_element_dimension=get_FE_element_dimension(top_level_element);
				for (j=0;j<top_level_element_dimension;j++)
				{
					top_level_xi[j] = element_to_top_level[j*(element_dimension+1)];
					for (k=0;k<element_dimension;k++)
					{
						top_level_xi[j] +=
							element_to_top_level[j*(element_dimension+1)+k+1]*xi[k];
					}
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Computed_field_integration::evaluate.  "
					"No top-level element found to evaluate field %s on",
					field->name);
				return_code=0;
			}
		}
		cmzn_field *integrand = field->source_fields[0];
		cmzn_field *coordinate_field = field->source_fields[1];
		coordinate_dimension =
			cmzn_field_get_number_of_components(field->source_fields[1]);
		if (Computed_field_is_type_xi_coordinates(field->source_fields[1], NULL))
		{
			/* Unlike the xi field we only deal with top level elements of a
				single dimension so we can match that dimension for our number
				of coordinates */
			coordinate_dimension = top_level_element_dimension;
		}
		/* 2. Calculate the field */
		if (texture_mapping)
		{
			mapping = FIND_BY_IDENTIFIER_IN_LIST
				(Computed_field_element_integration_mapping,element)
				(top_level_element, texture_mapping);
			if (mapping != 0)
			{
				cmzn_fieldcache& workingCache = *(valueCache.getOrCreateSharedExtraCache(cache));
				workingCache.setTime(time);
				/* Integrate to the specified top_level_xi location */
				for (i = 0 ; i < top_level_element_dimension ; i++)
				{
					initial_xi[i] = 0.0;
				}
				integrate_path(top_level_element,
					mapping->values, initial_xi, top_level_xi,
					/*number_of_gauss_points*/2, workingCache, field->source_fields[0],
					magnitude_coordinates, field->source_fields[1],
					valueCache.values);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Computed_field_integration::evaluate."
					"  Element %d not found in Xi texture coordinate mapping field %s",
					get_FE_element_identifier(element), field->name);
				return_code=0;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Computed_field_integration::evaluate.  "
				"Xi texture coordinate mapping not calculated");
			return_code=0;
		}
	}
	else if (node_location = cache.get_location_node())
	{
		FE_node *node = node_location->get_node();

		Computed_field_node_integration_mapping *mapping;

		return_code = 1;

		if (!texture_mapping)
		{
			calculate_mapping(time);
		}
		else
		{
			if ((time != cached_time)
				&& (Computed_field_has_multiple_times(field->source_fields[0])
					|| Computed_field_has_multiple_times(field->source_fields[1])))
			{
				DESTROY_LIST(Computed_field_element_integration_mapping)
					(&texture_mapping);
				calculate_mapping(time);
			}
		}
		/* 2. Calculate the field */
		if (node_mapping)
		{
			mapping = FIND_BY_IDENTIFIER_IN_LIST
				(Computed_field_node_integration_mapping,node_ptr)
				(node, node_mapping);
			if (mapping != 0)
			{
				for(i = 0 ; i < field->number_of_components ; i++)
				{
					valueCache.values[i] = mapping->values[i];
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Computed_field_integration_evaluate_cache_at_node."
					"  Node %d not found in Xi texture coordinate mapping field %s",
					get_FE_node_identifier(node));
				return_code=0;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Computed_field_integration_evaluate_cache_at_node.  "
				"Xi texture coordinate mapping not calculated");
			return_code=0;
		}
	}
	else
	{
		// Location type unknown or not implemented
		return_code = 0;
	}
	return (return_code);
}

// @TODO Migrate to new external field cache
#if defined (FUTURE_CODE)
int Computed_field_integration::propagate_find_element_xi(
	const FE_value *values, int number_of_values, struct FE_element **element_address,
	FE_value *xi, FE_value time, cmzn_mesh_id search_mesh)
{
	FE_value floor_values[3];
	int i, return_code;
	Computed_field_element_integration_mapping *mapping;
	Computed_field_integration_has_values_data has_values_data;

	ENTER(Computed_field_integration::propagate_find_element_xi);
	USE_PARAMETER(time);
	if (field && values && (number_of_values==field->number_of_components) && search_mesh)
	{
		const int element_dimension = search_mesh ?
			cmzn_mesh_get_dimension(search_mesh) : get_FE_element_dimension(*element_address);
		if (!texture_mapping)
		{
			calculate_mapping(/*time*/0);
		}
		else
		{
#if defined (NEW_CODE)
			/* Until we are calculating this at the correct time there is no
				point seeing if the time has changed */
			if ((time != cached_time)
				&& (Computed_field_has_multiple_times(field->source_fields[0])
				|| Computed_field_has_multiple_times(field->source_fields[1])))
			{
				DESTROY_LIST(Computed_field_element_integration_mapping)
					(&texture_mapping);
				Computed_field_integration_calculate_mapping(field, time);
			}
#endif /* defined (NEW_CODE) */
		}
		if (number_of_values<=3)
		{
			for (i = 0 ; i < number_of_values ; i++)
			{
				floor_values[i] = floor(values[i]);
			}
			has_values_data.values = floor_values;
			has_values_data.number_of_values = number_of_values;
			if (texture_mapping)
			{
				return_code=0;
				/* Check the last successful mapping first */
				if (find_element_xi_mapping&&
					Computed_field_element_integration_mapping_has_values(
						find_element_xi_mapping, (void *)&has_values_data))
				{
					return_code = 1;
				}
				else
				{
					/* Find in the list */
					mapping = FIRST_OBJECT_IN_LIST_THAT(Computed_field_element_integration_mapping)
						(Computed_field_element_integration_mapping_has_values, (void *)&has_values_data,
							texture_mapping);
					if (mapping != 0)
					{
						REACCESS(Computed_field_element_integration_mapping)(&(find_element_xi_mapping),
							mapping);
						return_code = 1;
					}
				}
				if (return_code)
				{
					*element_address = find_element_xi_mapping->element;
					for (i = 0 ; i < get_FE_element_dimension(*element_address) ; i++)
					{
						xi[i] = values[i] - floor_values[i];
					}
					if (!cmzn_mesh_contains_element(mesh, *element_address))
					{
						*element_address = (FE_element *)NULL;
						display_message(ERROR_MESSAGE,
							"Computed_field_integration::propagate_find_element_xi.  "
							"Element is not in integration mesh");
						return_code=0;
					}
					if (element_dimension && (element_dimension != get_FE_element_dimension(*element_address)))
					{
						*element_address = (FE_element *)NULL;
						display_message(ERROR_MESSAGE,
							"Computed_field_integration::propagate_find_element_xi.  "
							"Element is not of the required dimension");
						return_code=0;
					}
				}
				else
				{
					return_code=0;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Computed_field_integration::propagate_find_element_xi.  "
					"Xi texture coordinate mapping not calculated");
				return_code=0;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Computed_field_integration::propagate_find_element_xi.  "
				"Only implemented for three or less values");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_integration::propagate_find_element_xi.  "
			"Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_integration::propagate_find_element_xi */
#endif // defined (FUTURE_CODE)

int Computed_field_integration::list(
	)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_integration);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,"    seed_element : %d\n",
			get_FE_element_identifier(seed_element));
		display_message(INFORMATION_MESSAGE,"    integrand field : %s\n",
			field->source_fields[0]->name);
		display_message(INFORMATION_MESSAGE,"    coordinate field : %s\n",
			field->source_fields[1]->name);
		if (magnitude_coordinates)
		{
			display_message(INFORMATION_MESSAGE,"    magnitude_coordinates : true\n");
		}
		else
		{
			display_message(INFORMATION_MESSAGE,"    magnitude_coordinates : false\n");
		}
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_integration.  "
			"Invalid arguments.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_integration */

char *Computed_field_integration::get_command_string()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name, temp_string[40];
	int error, xi_texture_coordinates;

	ENTER(Computed_field_integration::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		xi_texture_coordinates = (
			Computed_field_is_constant_scalar(field->source_fields[0], 1.0) &&
			Computed_field_is_type_xi_coordinates(field->source_fields[1],
				(void *)NULL));
		if (xi_texture_coordinates != 0)
		{
			append_string(&command_string,
				computed_field_xi_texture_coordinates_type_string, &error);
		}
		else
		{
			append_string(&command_string,
				computed_field_integration_type_string, &error);
		}
		sprintf(temp_string, " seed_element %d", get_FE_element_identifier(seed_element));
		append_string(&command_string, temp_string, &error);
		if (!xi_texture_coordinates)
		{
			append_string(&command_string, " integrand ", &error);
			field_name = cmzn_field_get_name(field->source_fields[0]);
			if (NULL != field_name)
			{
				make_valid_token(&field_name);
				append_string(&command_string, field_name, &error);
				DEALLOCATE(field_name);
			}
			append_string(&command_string, " coordinate ", &error);
			field_name = cmzn_field_get_name(field->source_fields[1]);
			if (NULL != field_name)
			{
				make_valid_token(&field_name);
				append_string(&command_string, field_name, &error);
				DEALLOCATE(field_name);
			}
		}
		if (magnitude_coordinates)
		{
			append_string(&command_string, " magnitude_coordinates", &error);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_integration::get_command_string.  "
			"Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_integration::get_command_string */

} //namespace

int Computed_field_is_type_integration(struct Computed_field *field)
/*******************************************************************************
LAST MODIFIED : 14 August 2006

DESCRIPTION :
Returns true if <field> has the appropriate static type string.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_is_type_integration);
	if (field)
	{
		if (dynamic_cast<Computed_field_integration*>(field->core))
		{
			return_code = 1;
		}
		else
		{
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_is_type_integration.  Missing field");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_is_type_integration */

/*****************************************************************************//**
 * Creates a field that computes an integration over a mesh.
 * The seed element is set to the number given and the mapping calculated.
 * Sets the number of components to be the same as the <integrand> field.
 * The <integrand> is the value that is integrated over each element and the
 * <coordinate_field> is used to define the arc length differential for each
 * element. Currently only two gauss points are supported, a linear integration.
 * If <magnitude_coordinates> is false then the resulting field has the same
 * number of components as the <coordinate_field> and each component is the
 * integration with respect to each of the components, if <magnitude_components>
 * is true then the field will have a single component and the magnitude of the
 * <coordinate_field> derivatives are used to calculate arc lengths at each
 * gauss point.
 *
 * @param field_module  Region field module which will own new field.
 * @return Newly created field
 */
cmzn_field *cmzn_fieldmodule_create_field_integration(
	cmzn_fieldmodule *fieldmodule, cmzn_mesh_id mesh,
	cmzn_element_id seed_element, cmzn_field *integrand,
	int magnitude_coordinates, cmzn_field *coordinate_field)
{
	cmzn_field *field = nullptr;
	if (mesh && seed_element && cmzn_mesh_contains_element(mesh, seed_element) &&
		integrand && integrand->isNumerical() && coordinate_field &&
		coordinate_field->isNumerical() && 
		(1 == cmzn_field_get_number_of_components(integrand)))
	{
		int number_of_components = 0;
		if (magnitude_coordinates)
		{
			number_of_components = 1;
		}
		else
		{
			number_of_components = coordinate_field->number_of_components;
			if (Computed_field_is_type_xi_coordinates(coordinate_field, NULL))
			{
				/* Unlike the xi field we only deal with top level elements of a
					single dimension so we can match that dimension for our number
					of coordinates */
				number_of_components = get_FE_element_dimension(seed_element);
			}
		}
		cmzn_field *source_fields[2];
		source_fields[0] = integrand;
		source_fields[1] = coordinate_field;

		field = Computed_field_create_generic(fieldmodule,
			/*check_source_field_regions*/true,
			number_of_components,
			/*number_of_source_fields*/2, source_fields,
			/*number_of_source_values*/0, NULL,
			new Computed_field_integration(mesh, seed_element,
				magnitude_coordinates));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_integration.  Invalid argument(s)");
	}
	return (field);
}

int Computed_field_get_type_integration(Computed_field *field,
	cmzn_mesh_id *mesh_address, FE_element **seed_element,
	Computed_field **integrand, int *magnitude_coordinates,
	Computed_field **coordinate_field)
{
	Computed_field_integration* core;
	int return_code;

	ENTER(Computed_field_get_type_integration);
	if (field && (core = dynamic_cast<Computed_field_integration*>(field->core)))
	{
		*mesh_address = cmzn_mesh_access(core->mesh);
		*seed_element=core->seed_element;
		*integrand=field->source_fields[0];
		*magnitude_coordinates=core->magnitude_coordinates;
		*coordinate_field=field->source_fields[1];
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_integration.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_integration */

