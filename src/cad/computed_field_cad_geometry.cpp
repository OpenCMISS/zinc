/*****************************************************************************//**
 * FILE : computed_field_cad_geometry.cpp
 *
 * Implements a cmiss field which wraps an OpenCASCADE shape.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
extern "C" {
#include <stdlib.h>
#include "general/debug.h"
#include "general/mystring.h"
#include "region/cmiss_region.h"
#include "user_interface/message.h"
#include "computed_field/computed_field.h"
}

#include "computed_field/computed_field_private.hpp"
#include "computed_field/computed_field_set.h"

#include "cad/computed_field_cad_geometry.h"
#include "cad/computed_field_cad_topology.h"
#include "cad/computed_field_cad_topology_private.h"
#include "cad/geometricshape.h"
#include "cad/field_location.hpp"
/*
Module types
------------
*/
/*
class Computed_field_cad_geometry_package : public Computed_field_type_package
{
public:
	cmzn_region *root_region;

	Computed_field_cad_geometry_package(cmzn_region *root_region)
	  : root_region(root_region)
	{
		ACCESS(cmzn_region)(root_region);
	}

	~Computed_field_cad_geometry_package()
	{
		DEACCESS(cmzn_region)(&root_region);
	}
};
*/
namespace {

char computed_field_cad_geometry_type_string[] = "cad_geometry";

class Computed_field_cad_geometry : public Computed_field_core
{
public:

	Computed_field_cad_geometry()
		: Computed_field_core()
	{
	}

	~Computed_field_cad_geometry()
	{
	}

private:
	Computed_field_core* copy();

	const char* get_type_string()
	{
		return (computed_field_cad_geometry_type_string);
	}

	int compare(Computed_field_core* other_field);

	int evaluate(cmzn_field_cache& cache, FieldValueCache& valueCache);

	int list();

	//int has_multiple_times() {return 1;}

	char* get_command_string();

	virtual int get_attribute_integer(enum cmzn_field_attribute attribute) const
	{
		if (attribute == CMZN_FIELD_ATTRIBUTE_IS_COORDINATE)
			return 1;
		return 0;
	}

};

/***************************************************************************//**
 * Copy the type specific data used by this type.
 */
Computed_field_core* Computed_field_cad_geometry::copy()
{
	Computed_field_cad_geometry* core = 
		new Computed_field_cad_geometry();

	return (core);
} /* Computed_field_cad_geometry::copy */

/***************************************************************************//**
 * Compare the type specific data.
 */
int Computed_field_cad_geometry::compare(Computed_field_core *other_core)
{
	int return_code;

	ENTER(Computed_field_cad_geometry::compare);
	if (field && dynamic_cast<Computed_field_cad_geometry*>(other_core))
	{
		printf("Comparing a cad geometry field\n");
		return_code = 1;
	}
	else
	{
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_cad_geometry::compare */

int Computed_field_cad_geometry::evaluate(cmzn_field_cache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
	cmzn_field_cad_topology_id cad_topology = reinterpret_cast<cmzn_field_cad_topology_id>(getSourceField(0));
	// @TODO: prescribe location directly in cad_topology field's value cache
	Field_cad_geometry_surface_location *cad_surface_location;
	Field_cad_geometry_curve_location *cad_curve_location;
	if (0 != (cad_surface_location = dynamic_cast<Field_cad_geometry_surface_location*>(cache.getLocation())))
	{
		//printf("compare? 0x%x and 0x%x\n", cad_topology, cad_geometry_location->id());
		double point[3], uDerivative[3], vDerivative[3];
		if ((cad_topology == (cmzn_field_cad_topology_id *)cad_surface_location->get_id()) &&
			Computed_field_cad_topology_get_surface_point(cad_topology,
				cad_surface_location->get_identifier(),
				cad_surface_location->get_u(),
				cad_surface_location->get_v(),
				point, uDerivative, vDerivative))
		{
			valueCache.values[0] = point[0];
			valueCache.values[1] = point[1];
			valueCache.values[2] = point[2];
			int number_of_xi = cache.getRequestedDerivatives();
			if (number_of_xi)
			{
				//printf("START %d ", number_of_xi);
				//printf("(%.2f,%.2f,%.2f) -", uDerivative[0], uDerivative[1], uDerivative[2]);
				//printf(" (%.2f,%.2f,%.2f)\n", vDerivative[0], vDerivative[1], vDerivative[2]);
				valueCache.derivatives[0] = uDerivative[0];
				valueCache.derivatives[2] = uDerivative[1];
				valueCache.derivatives[4] = uDerivative[2];
				valueCache.derivatives[1] = vDerivative[0];
				valueCache.derivatives[3] = vDerivative[1];
				valueCache.derivatives[5] = vDerivative[2];
				valueCache.derivatives_valid = 1;
			}
			else
			{
				valueCache.derivatives_valid = 0;
			}
			//printf( "START[%.2f, %.2f , %.2f] ", valueCache.values[0], valueCache.values[1], valueCache.values[2] );
			return 1;
		}
	}
	else if (0 != (cad_curve_location = dynamic_cast<Field_cad_geometry_curve_location*>(cache.getLocation())))
	{
		double point[3];
		if ((cad_topology == (cmzn_field_cad_topology_id *)cad_curve_location->get_id()) &&
			Computed_field_cad_topology_get_curve_point(cad_topology,
				cad_curve_location->get_identifier(),
				cad_curve_location->get_s(),
				point))
		{
			valueCache.values[0] = point[0];
			valueCache.values[1] = point[1];
			valueCache.values[2] = point[2];
			valueCache.derivatives_valid = 0;
			//printf("START: (%.2f,%.2f,%.2f)\n", point[0], point[1], point[2]);
			return 1;
		}
	}
	return 0;
}

/***************************************************************************//**
 * Writes type-specific details of the field to the console.
 */
int Computed_field_cad_geometry::list()
{
	//char *field_name;
	int return_code;

	ENTER(List_Computed_field_cad_geometry);
	if (field)
	{
		cmzn_field_cad_topology_id cad_topology = cmzn_field_cast_cad_topology(field->source_fields[0]);
		int surface_count = cmzn_field_cad_topology_get_surface_count(cad_topology);
		int curve_count = cmzn_field_cad_topology_get_curve_count(cad_topology);
		display_message(INFORMATION_MESSAGE, "    Cad geometry field : # of surfaces: %d, # of curves: %d", surface_count, curve_count);
		cmzn_field_destroy(reinterpret_cast<cmzn_field_id *>(&cad_topology));
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_cad_geometry.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_cad_geometry */

/***************************************************************************//**
 * Returns allocated command string for reproducing this field. Includes type.
 */
char *Computed_field_cad_geometry::get_command_string()
{
	char *command_string = 0, *field_name = 0;
	int error = 0;

	ENTER(Computed_field_cad_geometry::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string, computed_field_cad_geometry_type_string, &error);
		append_string(&command_string, " field has no command string ", &error);
		if (GET_NAME(Computed_field)(field, &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_cad_geometry::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_cad_geometry::get_command_string */

} //namespace

int cmzn_field_is_cad_geometry( struct Computed_field *field, void *not_in_use )
{
	int return_code = 0;
	USE_PARAMETER(not_in_use);
	//printf( "Checking domain ...\n" );
	if ( field )
	{
		//printf( "Valid field ...\n" );
		if ( NULL != dynamic_cast<Computed_field_cad_geometry*>(field->core) )
		{
			//printf( "Yes, it is a cad geometry field.\n" );
			return_code = 1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_is_cad_geometry.  Invalid argument(s)");
	}

	return return_code;
}

cmzn_field_cad_geometry_id cmzn_field_cad_geometry_cast(cmzn_field_id field)
{
	if (dynamic_cast<Computed_field_cad_geometry*>(field->core))
	{
		return (reinterpret_cast<cmzn_field_cad_geometry_id>(field));
	}
	else
	{
		return (NULL);
	}
}

/**
 * @see cmzn_field_create_cad_geometry
 */
cmzn_field_id Computed_field_module_create_cad_geometry(cmzn_field_module_id field_module, cmzn_field_id cad_topology_source_field)
//cmzn_field_id Computed_field_create_cad_geometry( cmzn_field_id source_field, GeometricShape *shape )
{
	ENTER(Computed_field_create_cad_geometry);
	struct Computed_field *field = NULL;
	Computed_field *source_fields[1];

	if ( cad_topology_source_field )
	{

		source_fields[0] = cad_topology_source_field;
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			/*number_of_components*/3,
			/*number_of_source_fields*/1, source_fields,
			/*number_of_source_values*/0, NULL,
			new Computed_field_cad_geometry());

	}
	else
	{
		display_message( ERROR_MESSAGE, "Computed_field_create_cad_geometry.  "
			"Invalid argument(s)" );
	}
	LEAVE;

	return (field);
} /* Computed_field_create_cad_geometry */

