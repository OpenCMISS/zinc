/*******************************************************************************
FILE : computed_field_compose.c

LAST MODIFIED : 24 August 2006

DESCRIPTION :
Implements a computed_field that uses evaluates one field, does a
"find element_xi" look up on a field in a host element group to find the same
values and then evaluates a third field at that location.
Essentially it is used to embed one mesh in the elements of another.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "opencmiss/zinc/fieldmodule.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_private.hpp"
#include "computed_field/computed_field_set.h"
#include "region/cmiss_region.hpp"
#include "general/debug.h"
#include "general/mystring.h"
#include "general/message.h"
#include "computed_field/field_module.hpp"
#include "mesh/cmiss_element_private.hpp"
#include "computed_field/computed_field_compose.h"

Computed_field_compose::~Computed_field_compose()
{
	cmzn_mesh_destroy(&mesh);
}

Computed_field_core *Computed_field_compose::copy()
{
	return new Computed_field_compose(mesh, find_nearest, use_point_five_when_out_of_bounds);
}

int Computed_field_compose::compare(Computed_field_core *other_core)
{
	int return_code;
	Computed_field_compose *other;

	ENTER(Computed_field_compose::type_specific_contents_match);
	if (field && (0 != (other = dynamic_cast<Computed_field_compose*>(other_core))))
	{
		if (cmzn_mesh_match(mesh, other->mesh) &&
			(find_nearest == other->find_nearest) &&
			(use_point_five_when_out_of_bounds ==
				other->use_point_five_when_out_of_bounds))
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
} /* Computed_field_compose::compare */

bool Computed_field_compose::is_defined_at_location(cmzn_fieldcache& cache)
{
	return (0 != field->evaluate(cache));
}

int Computed_field_compose::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	int return_code = 0;
	const RealFieldValueCache *coordinateValueCache = RealFieldValueCache::cast(getSourceField(0)->evaluate(cache));
	if (coordinateValueCache)
	{
		RealFieldValueCache& valueCache = RealFieldValueCache::cast(inValueCache);
		cmzn_fieldcache& extraCache = *valueCache.getExtraCache();
		extraCache.setTime(cache.getTime());
		/* The values from the first source field are inverted in the
			second source field to get element_xi which is evaluated with
			the third source field */
		cmzn_element_id compose_element =0;
		FE_value compose_xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
		if (Computed_field_find_element_xi(getSourceField(1), &extraCache,
			coordinateValueCache->values,
			coordinateValueCache->componentCount,
			&compose_element, compose_xi,
			mesh, /*propagate_field*/0, find_nearest)
			&& compose_element)
		{
			/* calculate the third source_field at this new location */
			extraCache.setMeshLocation(compose_element, compose_xi);
			const RealFieldValueCache *calculateValueCache = RealFieldValueCache::cast(getSourceField(2)->evaluate(extraCache));
			if (calculateValueCache)
			{
				return_code = 1;
				for (int i=0;i<field->number_of_components;i++)
				{
					valueCache.values[i] = calculateValueCache->values[i];
				}
			}
		}
		else
		{
			if (use_point_five_when_out_of_bounds)
			{
				/* Actually don't fail here, just make the values constant so that
					people can compose outside the valid range */
				return_code = 1;
				for (int i=0;i<field->number_of_components;i++)
				{
					valueCache.values[i]=0.5;
				}
			}
			else
			{
				return_code = 0;
			}
		}
	}
	return return_code;
}

int Computed_field_compose::list()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code = 0;

	ENTER(Computed_field_compose::list);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,"    texture coordinates field :");
		display_message(INFORMATION_MESSAGE," %s\n",
			field->source_fields[0]->name);
		display_message(INFORMATION_MESSAGE,"    find element xi field :");
		display_message(INFORMATION_MESSAGE," %s\n",
			field->source_fields[1]->name);
		display_message(INFORMATION_MESSAGE, "    mesh : ");
		char *mesh_name = cmzn_mesh_get_name(mesh);
		display_message(INFORMATION_MESSAGE, "%s\n", mesh_name);
		DEALLOCATE(mesh_name);
		display_message(INFORMATION_MESSAGE,"    calculate values field :");
		display_message(INFORMATION_MESSAGE," %s\n",
			field->source_fields[2]->name);
		if (find_nearest)
		{
			display_message(INFORMATION_MESSAGE,"    find nearest match\n");
		}
		else
		{
			display_message(INFORMATION_MESSAGE,"    find exact match\n");
		}
		if (use_point_five_when_out_of_bounds)
		{
			display_message(INFORMATION_MESSAGE,"    use point five when out of bounds\n");
		}
		display_message(INFORMATION_MESSAGE,"    element dimension %d\n", cmzn_mesh_get_dimension(mesh));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_compose::list.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
}

char *Computed_field_compose::get_command_string()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name, temp_string[40];
	int error;

	ENTER(Computed_field_compose::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_compose_type_string, &error);
		append_string(&command_string, " texture_coordinates_field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
		append_string(&command_string, " find_element_xi_field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[1], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
		append_string(&command_string, " mesh ", &error);
		char *mesh_name = cmzn_mesh_get_name(mesh);
		append_string(&command_string, mesh_name, &error);
		DEALLOCATE(mesh_name);
		append_string(&command_string, " calculate_values_field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[2], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
		if (find_nearest)
		{
			append_string(&command_string, " find_nearest", &error);
		}
		if (use_point_five_when_out_of_bounds)
		{
			append_string(&command_string, " use_point_five_when_out_of_bounds", &error);
		}
		sprintf(temp_string, " element_dimension %d", cmzn_mesh_get_dimension(mesh));
		append_string(&command_string, temp_string, &error);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_compose::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_compose::get_command_string */

/***************************************************************************//**
 * Creates a field of type COMPUTED_FIELD_COMPOSE. This field allows you to
 * evaluate one field to find "texture coordinates", use a find_element_xi field
 * to then calculate a corresponding element/xi and finally calculate values
 * using this element/xi and a third field.  You can then evaluate values on a
 * "host" mesh for any points "contained" inside.  The <search_mesh> is the
 * domain from which any returned element_xi will belong.
 * If <use_point_five_when_out_of_bounds> is true then if the
 * texture_coordinate_field values cannot be found in the find_element_xi_field,
 * then instead of returning failure, the values will be set to 0.5 and returned
 * as success.
 * NOTE: this field type has been superceded by find_mesh_location combined with
 * embedded field. DO NOT add to external API.
 */
cmzn_field *cmzn_fieldmodule_create_field_compose(cmzn_fieldmodule *fieldmodule,
	cmzn_field *texture_coordinate_field,
	cmzn_field *find_element_xi_field,
	cmzn_field *calculate_values_field,
	cmzn_mesh_id search_mesh,
	int find_nearest, int use_point_five_when_out_of_bounds)
{
	cmzn_field *field = nullptr;

	if (texture_coordinate_field && texture_coordinate_field->isNumerical() &&
		find_element_xi_field && find_element_xi_field->isNumerical() &&
		calculate_values_field && calculate_values_field->isNumerical() &&
		search_mesh &&
		(cmzn_mesh_get_region_internal(search_mesh) ==
			cmzn_fieldmodule_get_region_internal(fieldmodule)))
	{
		if (texture_coordinate_field->number_of_components ==
			find_element_xi_field->number_of_components)
		{
			if (Computed_field_is_find_element_xi_capable(
				find_element_xi_field, /*dummy*/NULL))
			{
				cmzn_field *source_fields[3];
				source_fields[0] = texture_coordinate_field;
				source_fields[1] = find_element_xi_field;
				source_fields[2] = calculate_values_field;
				field = Computed_field_create_generic(fieldmodule,
					/*check_source_field_regions*/true,
					calculate_values_field->number_of_components,
					/*number_of_source_fields*/3, source_fields,
					/*number_of_source_values*/0, NULL,
					new Computed_field_compose(search_mesh, find_nearest,
						use_point_five_when_out_of_bounds));
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"cmzn_fieldmodule_create_field_compose.  "
					"The type of find_element_xi_field supplied has not "
					"been implemented for find_element_xi calculations.");
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"cmzn_fieldmodule_create_field_compose.  "
				"The texture_coordinate_field and find_element_xi_field "
				"must have the same number of components");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_compose.  Invalid argument(s)");
	}
	return (field);
}

/***************************************************************************//**
 * If the field is of type COMPUTED_FIELD_COMPOSE, the function returns the
 * three fields which define the field.
 * Note that the fields and mesh are not ACCESSed.
 */
int Computed_field_compose::get_type(
	struct Computed_field **texture_coordinate_field_address,
	struct Computed_field **find_element_xi_field_address,
	struct Computed_field **calculate_values_field_address,
	cmzn_mesh_id *mesh_address, int *find_nearest_address,
	int *use_point_five_when_out_of_bounds_address)
{
	int return_code;
	if (field && texture_coordinate_field_address &&
		find_element_xi_field_address && calculate_values_field_address &&
		mesh_address && find_nearest_address &&
		use_point_five_when_out_of_bounds_address)
	{
		*texture_coordinate_field_address = field->source_fields[0];
		*find_element_xi_field_address = field->source_fields[1];
		*calculate_values_field_address = field->source_fields[2];
		*mesh_address = mesh;
		*find_nearest_address = find_nearest;
		*use_point_five_when_out_of_bounds_address = use_point_five_when_out_of_bounds;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_compose::get_type.  Invalid argument(s)");
		return_code = 0;
	}
	return (return_code);
}

