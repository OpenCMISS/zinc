/*******************************************************************************
FILE : computed_field_find_xi_private.hpp

LAST MODIFIED : 13 June 2008

DESCRIPTION :
Data structures and prototype functions needed for all find xi implementations.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (COMPUTED_FIELD_FIND_XI_PRIVATE_HPP)
#define COMPUTED_FIELD_FIND_XI_PRIVATE_HPP

#include "opencmiss/zinc/mesh.h"

class Computed_field_find_element_xi_base_cache
{
	cmzn_mesh_id search_mesh;
public:
	struct FE_element *element;
	int number_of_values;
	double time;
	FE_value *values;
	FE_value *working_values;
	int in_perform_find_element_xi;
	/* Warn when trying to destroy this cache as it is being filled in */
	
	Computed_field_find_element_xi_base_cache() :
		search_mesh(0),
		element((struct FE_element *)NULL),
		number_of_values(0),
		time(0),
		values((FE_value *)NULL),
		working_values((FE_value *)NULL),
		in_perform_find_element_xi(0)
	{
	}
	
	virtual ~Computed_field_find_element_xi_base_cache()
	{
		if (search_mesh)
		{
			cmzn_mesh_destroy(&search_mesh);
		}
		if (values)
		{
			DEALLOCATE(values);
		}
		if (working_values)
		{
			DEALLOCATE(working_values);
		}
	}

	cmzn_mesh_id get_search_mesh()
	{
		return search_mesh;
	};

	void set_search_mesh(cmzn_mesh_id new_search_mesh)
	{
		if (new_search_mesh)
		{
			cmzn_mesh_access(new_search_mesh);
		}
		if (search_mesh)
		{
			cmzn_mesh_destroy(&search_mesh);
		}
		search_mesh = new_search_mesh;
	};
};

struct Computed_field_find_element_xi_cache
/* cache is wrapped in a struct for compatibility with C code */
{
	Computed_field_find_element_xi_base_cache* cache_data;
};

struct Computed_field_find_element_xi_cache
	*CREATE(Computed_field_find_element_xi_cache)(
		Computed_field_find_element_xi_base_cache *cache_data);
/*******************************************************************************
LAST MODIFIED : 13 June 2008

DESCRIPTION :
Stores cache data for find_element_xi routines.
The new object takes ownership of the <cache_data>.
==============================================================================*/

struct Computed_field_iterative_find_element_xi_data
/*******************************************************************************
LAST MODIFIED: 21 August 2002

DESCRIPTION:
Data for passing to Computed_field_iterative_element_conditional
Important note:
The <values> passed in this structure must not be a pointer to values
inside the field_cache otherwise they may be overwritten if that field
matches the <field> in this structure or one of its source fields.
==============================================================================*/
{
	FE_value xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	cmzn_fieldcache_id field_cache;
	struct Computed_field *field;
	int number_of_values;
	FE_value *values;
	int found_number_of_xi;
	FE_value *found_values;
	FE_value *found_derivatives;
	FE_value xi_tolerance;
	int find_nearest_location;
	struct FE_element *nearest_element;
	FE_value nearest_xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	double nearest_element_distance_squared;
	int start_with_data_xi;
	double time;
}; /* Computed_field_iterative_find_element_xi_data */

int Computed_field_iterative_element_conditional(struct FE_element *element,
	struct Computed_field_iterative_find_element_xi_data *data);
/***************************************************************************//**
 * Searches element for location with matching field values.
 * Important note:
 * The <values> passed in the <data> structure must not be a pointer to values
 * inside a field cache otherwise they may be overwritten if the field is the
 * same as the <data> field or any of its source fields.
 *
 * @return  1 if a valid element xi is found.
 */

#endif /* !defined (COMPUTED_FIELD_FIND_XI_PRIVATE_HPP) */
