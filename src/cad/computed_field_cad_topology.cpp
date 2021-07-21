/*****************************************************************************//**
 * FILE : computed_field_cad_topology.cpp
 *
 * Implements a cmiss field which wraps an OpenCASCADE shape.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stdlib.h>

extern "C" {
#include "api/cmiss_scene.h"
#include "general/debug.h"
#include "general/mystring.h"
#include "graphics/scene.h"
#include "region/cmiss_region.h"
#include "user_interface/message.h"
#include "computed_field/computed_field.h"
}

#include "computed_field/computed_field_private.hpp"
#include "computed_field/computed_field_set.h"

#include "cad/field_location.hpp"
#include "cad/computed_field_cad_topology.h"
#include "cad/computed_field_cad_topology_private.h"

namespace {

char computed_field_cad_topology_type_string[] = "cad_topology";

class Computed_field_cad_topology : public Computed_field_core
{
protected:
	TopologicalShape *m_shape;
	GeometricShape *m_geometric_shape;
	std::vector<int> surface_identifiers;

public:
	Computed_field_cad_topology(TopologicalShape *shape)
		: Computed_field_core()
		, m_shape(shape)
		, m_geometric_shape(0)
	{
	}

	~Computed_field_cad_topology()
	{
		if ( m_shape )
			delete m_shape;

		if (m_geometric_shape)
			delete m_geometric_shape;
	}

	inline void topological_shape(TopologicalShape *shape) { m_shape = shape; }
	inline void geometric_shape(GeometricShape *shape) {m_geometric_shape = shape;}
	int get_domain( struct LIST(Computed_field) *domain_field_list ) const;
	int surface_point(cmzn_cad_surface_identifier identifier, double u, double v, double *point, double *uDerivative, double *vDerivative);
	int curve_point(cmzn_cad_curve_identifier identifier, double s, double *point);
	int surface_colour(cmzn_cad_surface_identifier identifier, double u, double v, double *colour);
	int surface_uv_point(cmzn_cad_surface_identifier identifier, cmzn_cad_surface_point_identifier point_identifier, double &u, double &v);
	int curve_s_parameter(cmzn_cad_curve_identifier identifier, cmzn_cad_curve_point_identifier s_identifier, double &s);
	int surface_count() const;
	int curve_count() const;
	int surface_point_count(int i) const;
	int curve_point_count(int i) const;

	void information(Cad_primitive_identifier id) const;

private:
	Computed_field_core* copy();

	const char* get_type_string()
	{
		return (computed_field_cad_topology_type_string);
	}

	int compare(Computed_field_core* other_field);

	int evaluate(cmzn_field_cache& cache, FieldValueCache& valueCache);

	int list();

	char* get_command_string();

	void surface_information(const TopoDS_Face& face) const;
	void curve_information(const TopoDS_Edge& edge) const;
	void shape_information() const;

};

/***************************************************************************//**
 * Copy the type specific data used by this type.
 */
Computed_field_core* Computed_field_cad_topology::copy()
{
	Computed_field_cad_topology* core = 
		new Computed_field_cad_topology(m_shape);;

	return (core);
} /* Computed_field_cad_topology::copy */

/***************************************************************************//**
 * Compare the type specific data.
 */
int Computed_field_cad_topology::compare(Computed_field_core *other_core)
{
	int return_code;

	if (field && dynamic_cast<Computed_field_cad_topology*>(other_core))
	{
		printf("Comparing computed field cad topology, not really\n" );
		return_code = 1;
	}
	else
	{
		return_code = 0;
	}

	return (return_code);
} /* Computed_field_cad_topology::compare */

int Computed_field_cad_topology::evaluate(cmzn_field_cache&, FieldValueCache& /*inValueCache*/)
{
	// @TODO: prescribe location directly in cad_topology field's value cache
	// This means cad_topology field is evaluated if location prescribed, otherwise it fails
	// Should be able to view the value as a string, i.e. as a label field

	//Field_cad_geometry_location *cad_geometry_location = static_cast<Field_cad_geometry_location*>(location);
	//field->values[0] = surface_point( cad_geometry_location->surface_index(), cad_geometry_location->point_index(), 0 );
	//field->values[1] = surface_point( cad_geometry_location->surface_index(), cad_geometry_location->point_index(), 1 );
	//field->values[2] = surface_point( cad_geometry_location->surface_index(), cad_geometry_location->point_index(), 2 );
	//printf( "START[%.2f, %.2f , %.2f] ", field->values[0], field->values[1], field->values[2] );
	return 0;
}

/***************************************************************************//**
 * Writes type-specific details of the field to the console.
 */
int Computed_field_cad_topology::list()
{
	//char *field_name;
	int return_code = 0;

	if (field)
	{
		display_message(INFORMATION_MESSAGE, "    Cad Shape field : ");
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_cad_topology.  Invalid argument(s)");
	}

	return (return_code);
} /* list_Computed_field_cad_topology */

/***************************************************************************//**
 * Returns allocated command string for reproducing this field. Includes type.
 */
char *Computed_field_cad_topology::get_command_string()
{
	char *command_string, *field_name;
	int error;

	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string, computed_field_cad_topology_type_string, &error);
		append_string(&command_string, " field ", &error);
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
			"Computed_field_cad_topology::get_command_string.  Invalid field");
	}

	return (command_string);
} /* Computed_field_cad_topology::get_command_string */

int Computed_field_cad_topology::get_domain( struct LIST(Computed_field) *domain_field_list ) const
{
	int return_code = 0;
	if (domain_field_list)
	{
		if ( ADD_OBJECT_TO_LIST(Computed_field)(field,domain_field_list) )
		{
			return_code = 1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Computed_field_cad_topology::get_domain.  Failed to add object to list" );
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_cad_topology::get_domain.  Invalid argument" );
	}

	return return_code;
}

int Computed_field_cad_topology::surface_count() const
{
	int surface_count = -1;
	if (m_geometric_shape)
	{
		surface_count = m_geometric_shape->surfaceCount();
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_cad_geometry::surface_count.  Invalid geometric shape");
	}

	return surface_count;
}

int Computed_field_cad_topology::curve_count() const
{
	int curve_count = -1;
	if (m_geometric_shape)
	{
		curve_count = m_geometric_shape->curveCount();
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_cad_geometry::curve_count.  Invalid geometric shape");
	}

	return curve_count;
}

int Computed_field_cad_topology::surface_point_count(int i) const
{
	int point_count = -1;
	if (m_geometric_shape)
	{
		int surface_count = m_geometric_shape->surfaceCount();
		if (i >= 0 && i < surface_count)
		{
			point_count = m_geometric_shape->surface(i)->pointCount();
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Computed_field_cad_topology::surface_point_count.  Surface index out of bounds (%d)", i);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_cad_topology::surface_point_count.  Invalid geometric shape");
	}

	return point_count;
}

int Computed_field_cad_topology::curve_point_count(int i) const
{
	int point_count = -1;
	if (m_geometric_shape)
	{
		int curve_count = m_geometric_shape->curveCount();
		if (i >= 0 && i < curve_count)
		{
			point_count = m_geometric_shape->curve(i)->pointCount();
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Computed_field_cad_topology::curve_point_count.  Curve index out of bounds (%d)", i);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_cad_topology::curve_point_count.  Invalid geometric shape");
	}

	return point_count;
}

int Computed_field_cad_topology::surface_uv_point(cmzn_cad_surface_identifier identifier, cmzn_cad_surface_point_identifier point_identifier, double &u, double &v)
{
	int return_code = 0;
	if (field)
	{
		//u = m_geometric_shape->surface(surface_index)->uvPoints()[2 * point_index + 0];
		//v = m_geometric_shape->surface(surface_index)->uvPoints()[2 * point_index + 1];
		return_code = m_geometric_shape->surface(identifier)->uvPoints(point_identifier, u, v);
	}

	return return_code;
}

int Computed_field_cad_topology::curve_s_parameter(cmzn_cad_curve_identifier identifier, cmzn_cad_curve_point_identifier s_identifier, double &s)
{
	int return_code = 0;
	if (field)
	{
		//u = m_geometric_shape->surface(surface_index)->uvPoints()[2 * point_index + 0];
		//v = m_geometric_shape->surface(surface_index)->uvPoints()[2 * point_index + 1];
		return_code = m_geometric_shape->curve(identifier)->sParameter(s_identifier, s);
	}

	return return_code;
}

int Computed_field_cad_topology::surface_point(cmzn_cad_surface_identifier identifier, double u, double v, double *point, double *uDerivative, double *vDerivative)
{
	int return_code = 0;
	if (field)
	{
		return_code = m_geometric_shape->surface(identifier)->surfacePoint(u, v, point, uDerivative, vDerivative);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_cad_topology::surface_point.  Invalid shape");
	}

	return return_code;
}

int Computed_field_cad_topology::curve_point(cmzn_cad_curve_identifier identifier, double s, double *point)
{
	int return_code = 0;
	if (field)
	{
		return_code = m_geometric_shape->curve(identifier)->curvePoint( s, point);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_cad_topology::surface_point.  Invalid shape");
	}

	return return_code;
}

int Computed_field_cad_topology::surface_colour(cmzn_cad_surface_identifier identifier, double u, double v, double *colour)
{
	USE_PARAMETER(u);
	USE_PARAMETER(v);
	int return_code = 0;
	if (field)
	{
		return_code = m_shape->surfaceColour(identifier, colour);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_cad_topology::surface_colour.  Invalid shape");
	}

	return return_code;
}

void Computed_field_cad_topology::shape_information() const
{
	display_message(INFORMATION_MESSAGE, "  Shape type: %s\n", m_shape->shapeType().c_str());
	display_message(INFORMATION_MESSAGE, "  Label: %s\n", m_shape->label());
	double location[3];
	m_shape->location(location);
	display_message(INFORMATION_MESSAGE, "  Location: (%.3f, %.3f, %.3f)\n", location[0], location[1], location[2]);
}

void Computed_field_cad_topology::information(Cad_primitive_identifier id) const
{
	//String info;
	TopoDS_Edge edge;
	TopoDS_Face face;
	display_message(INFORMATION_MESSAGE,
		"Computed_field_cad_topology %s.  Information:\n", cmzn_field_get_name(field));
	switch (id.type)
	{
	case Cad_primitive_CURVE:
		if (m_geometric_shape)
		{
			if (id.number == -1)
			{
				if (curve_count() == 1)
					display_message(INFORMATION_MESSAGE, "  %d Curve\n", curve_count());
				else
					display_message(INFORMATION_MESSAGE, "  %d Curves\n", curve_count());
				for (int i = 0; i < curve_count(); i++)
				{
					const Curve* curve = m_geometric_shape->curve(i);
					curve->information();
				}
			}
			else if (0 <= id.number && id.number < curve_count())
			{
				const Curve* curve = m_geometric_shape->curve(id.number);
				curve->information();
			}
		}
		else
		{
			display_message(INFORMATION_MESSAGE,
				"  No curve information available\n");
		}
		break;
	case Cad_primitive_SURFACE:
		if (m_geometric_shape)
		{
			if (id.number == -1)
			{
				if (curve_count() == 1)
					display_message(INFORMATION_MESSAGE, "  %d Surface\n", surface_count());
				else
					display_message(INFORMATION_MESSAGE, "  %d Surfaces\n", surface_count());
				for (int i = 0; i < surface_count(); i++)
				{
					const Surface* surface = m_geometric_shape->surface(i);
					surface->information();
				}
			}
			else if (0 <= id.number && id.number < surface_count())
			{
				const Surface* surface = m_geometric_shape->surface(id.number);
				surface->information();
			}
		}
		else
		{
			display_message(INFORMATION_MESSAGE,
				"  No surface information available\n");
		}
		break;
	case Cad_primitive_SHAPE:
		shape_information();
		break;
	case Cad_primitive_INVALID:
	case Cad_primitive_POINT:
		{
			display_message(INFORMATION_MESSAGE,
				"Computed_field_cad_topology::information funciton not complete for this case!\n");
			break;
		}
	}
}

char computed_field_cad_group_type_string[] = "cad_group";
/**
 * A group of cad element identifiers
 * Implemented using a std::list
 */
class Field_cad_element_group : public Computed_field_core
{
private:
	Computed_field_cad_topology *topology;
	std::list<Cad_topology_primitive_identifier> element_identifiers;

private:
	Computed_field_core* copy();

	const char* get_type_string()
	{
		return (computed_field_cad_group_type_string);
	}

	int compare(Computed_field_core* other_field);

	int evaluate(cmzn_field_cache& cache, FieldValueCache& valueCache);

	int list();

	char* get_command_string();

};

/**
 * Copy the type specific data used by this type.
 */
Computed_field_core* Field_cad_element_group::copy()
{
	Field_cad_element_group* core = 
		new Field_cad_element_group();

	return (core);
} /* Field_cad_element_group::copy */

/***************************************************************************//**
 * Compare the type specific data.
 */
int Field_cad_element_group::compare(Computed_field_core *other_core)
{
	int return_code;

	if (field && dynamic_cast<Field_cad_element_group*>(other_core))
	{
		printf("Comparing computed field cad group, not really\n" );
		return_code = 1;
	}
	else
	{
		return_code = 0;
	}

	return (return_code);
} /* Field_cad_element_group::compare */


int Field_cad_element_group::evaluate(cmzn_field_cache& /*cache*/, FieldValueCache& /*inValueCache*/)
{
	printf("Hi from Field_cad_element_group::evaluate ...  doing nothing\n");
	return 0;
}

/***************************************************************************//**
 * Writes type-specific details of the field to the console.
 */
int Field_cad_element_group::list()
{
	//char *field_name;
	int return_code = 0;

	if (field)
	{
		display_message(INFORMATION_MESSAGE, "    Cad Group field : ");
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_cad_topology.  Invalid argument(s)");
	}

	return (return_code);
} /* Field_cad_element_group::list */

/***************************************************************************//**
 * Returns allocated command string for reproducing this field. Includes type.
 */
char *Field_cad_element_group::get_command_string()
{
	char *command_string, *field_name;
	int error;

	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string, computed_field_cad_topology_type_string, &error);
		append_string(&command_string, " field ", &error);
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
			"Computed_field_cad_topology::get_command_string.  Invalid field");
	}

	return (command_string);
} /* Field_cad_element_group::get_command_string */

} //namespace

int cmzn_field_is_type_cad_topology(cmzn_field_id field, void *not_in_use)
{
	int return_code = 0;
	USE_PARAMETER(not_in_use);
	//printf( "Checking domain ...\n" );
	if (field)
	{
		//printf( "Valid field ...\n" );
		if ( NULL != dynamic_cast<Computed_field_cad_topology*>(field->core) )
		{
			//printf( "Yes, it is a cad topology.\n" );
			return_code = 1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_field_is_type_cad_topology.  Invalid argument(s)");
	}

	return return_code;
}

cmzn_field_cad_topology_id cmzn_field_cast_cad_topology(cmzn_field_id field)
{
	cmzn_field_cad_topology_id cad_topology = NULL;
	if (dynamic_cast<Computed_field_cad_topology*>(field->core))
	{
		cmzn_field_access(field);
		cad_topology = (reinterpret_cast<cmzn_field_cad_topology_id>(field));
	}

	return cad_topology;
}

int cmzn_field_cad_topology_destroy(cmzn_field_cad_topology_id *cad_topology_field_address)
{
	return cmzn_field_destroy(reinterpret_cast<cmzn_field_id *>(cad_topology_field_address));
}

cmzn_field_cad_topology_id cmzn_field_cad_topology_access(cmzn_field_cad_topology_id cad_topology_field)
{
	if (cad_topology_field)
	{
		Computed_field *computed_field = reinterpret_cast<Computed_field *>(cad_topology_field);
		cmzn_field_access(computed_field);
	}

	return cad_topology_field;
}

/**
 * @see cmzn_field_module_create_cad_topology
 */
Computed_field *cmzn_field_module_create_cad_topology( cmzn_field_module *field_module, TopologicalShape *shape )
{
	Computed_field *field = (Computed_field *)NULL;

	if ( shape )
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			/*number_of_components*/3,
			/*number_of_source_fields*/0, NULL,
			/*number_of_source_values*/0, NULL,
			new Computed_field_cad_topology( shape ));

		//cad_topology_field->topological_shape(shape);
	}
	else
	{
		display_message( ERROR_MESSAGE, "Computed_field_create_image.  "
			"Invalid argument(s)" );
	}

	return (field);
} /* cmzn_field_module_create_cad_topology */

void cmzn_field_cad_topology_set_geometric_shape(cmzn_field_cad_topology_id field, GeometricShape *shape)
{
	if (field && shape)
	{
		Computed_field *computed_field = reinterpret_cast<Computed_field *>(field);
		Computed_field_cad_topology *cad_topology = static_cast<Computed_field_cad_topology*>(computed_field->core);
		cad_topology->geometric_shape(shape);
	}
	else
	{
		display_message( ERROR_MESSAGE, "Computed_field_set_cad_geometry.  "
			"Invalid argument(s)" );
	}
}

int cmzn_field_cad_topology_get_surface_count( cmzn_field_cad_topology_id field )
{
	int surface_count = -1;

	if (field)
	{
		Computed_field *computed_field = reinterpret_cast<Computed_field *>(field);
		Computed_field_cad_topology *cad_topology = static_cast<Computed_field_cad_topology*>(computed_field->core);
		surface_count = cad_topology->surface_count();
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_field_surface_count.  Invalid field");
	}

	return surface_count;
}

int cmzn_field_cad_topology_get_curve_count(cmzn_field_cad_topology_id field)
{
	int curve_count = -1;

	if (field)
	{
		Computed_field *computed_field = reinterpret_cast<Computed_field *>(field);
		Computed_field_cad_topology *cad_topology = static_cast<Computed_field_cad_topology *>(computed_field->core);
		curve_count = cad_topology->curve_count();
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_field_curve_count.  Invalid field");
	}

	return curve_count;
}


int cmzn_field_cad_topology_get_surface_point_count( cmzn_field_cad_topology_id field, cmzn_cad_surface_identifier identifier )
{
	int point_count = -1;
	if (field)
	{
		Computed_field *computed_field = reinterpret_cast<Computed_field *>(field);
		Computed_field_cad_topology *cad_topology = reinterpret_cast<Computed_field_cad_topology*>(computed_field->core);
		point_count = cad_topology->surface_point_count( identifier );
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_field_surface_point_count.  Invalid field");
	}

	return point_count;
}

int cmzn_field_cad_topology_get_curve_point_count( cmzn_field_cad_topology_id field, cmzn_cad_curve_identifier identifier )
{
	int point_count = -1;
	if (field)
	{
		Computed_field *computed_field = reinterpret_cast<Computed_field *>(field);
		Computed_field_cad_topology *cad_topology = reinterpret_cast<Computed_field_cad_topology*>(computed_field->core);
		point_count = cad_topology->curve_point_count( identifier );
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_field_curve_point_count.  Invalid field");
	}

	return point_count;
}

int cmzn_field_cad_topology_get_surface_point_uv_coordinates( cmzn_field_cad_topology_id field,
	cmzn_cad_surface_identifier identifier,
	cmzn_cad_surface_point_identifier uv_identifier,
	double &u, double &v)
{
	int return_code = 0;
	u = 0.0;
	v = 0.0;
	if (field)
	{
		Computed_field *computed_field = reinterpret_cast<Computed_field *>(field);
		Computed_field_cad_topology *cad_topology = reinterpret_cast<Computed_field_cad_topology*>(computed_field->core);
		return_code = cad_topology->surface_uv_point(identifier, uv_identifier, u, v);
	}
	return return_code;
}

int cmzn_field_cad_topology_get_curve_point_s_coordinate( cmzn_field_cad_topology_id field, cmzn_cad_curve_identifier identifier,
	cmzn_cad_curve_point_identifier s_identifier, double &s)
{
	int return_code = 0;
	s = 0.0;
	if (field)
	{
		Computed_field *computed_field = reinterpret_cast<Computed_field *>(field);
		Computed_field_cad_topology *cad_topology = reinterpret_cast<Computed_field_cad_topology*>(computed_field->core);
		return_code = cad_topology->curve_s_parameter(identifier, s_identifier, s);
	}
	return return_code;
}

int Computed_field_cad_topology_get_surface_point(cmzn_field_cad_topology_id field,
	cmzn_cad_surface_identifier identifier,
	double u, double v, double *point, double *uDerivative, double *vDerivative)
{
	int return_code = 0;
	if (field)
	{
		Computed_field *computed_field = reinterpret_cast<Computed_field *>(field);
		Computed_field_cad_topology *cad_topology = reinterpret_cast<Computed_field_cad_topology*>(computed_field->core);
		return_code = cad_topology->surface_point(identifier, u, v, point, uDerivative, vDerivative);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_field_surface_point.  Invalid field");
	}

	return return_code;
}

int Computed_field_cad_topology_get_curve_point(cmzn_field_cad_topology_id field, cmzn_cad_curve_identifier identifier, double s, double *point)
{
	int return_code = 0;
	if (field)
	{
		Computed_field *computed_field = reinterpret_cast<Computed_field *>(field);
		Computed_field_cad_topology *cad_topology = reinterpret_cast<Computed_field_cad_topology*>(computed_field->core);
		return_code = cad_topology->curve_point(identifier, s, point);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_field_surface_point.  Invalid field");
	}

	return return_code;
}

int Computed_field_cad_topology_get_surface_colour(cmzn_field_cad_topology_id field, cmzn_cad_surface_identifier identifier, double u, double v, double *colour)
{
	int return_code = 0;
	if (field)
	{
		Computed_field *computed_field = reinterpret_cast<Computed_field *>(field);
		Computed_field_cad_topology *cad_topology = reinterpret_cast<Computed_field_cad_topology*>(computed_field->core);
		return_code = cad_topology->surface_colour(identifier, u, v, colour);
	}

	return return_code;
}

void Cad_topology_information( cmzn_field_cad_topology_id cad_topology_field, Cad_primitive_identifier id )
{
	if (cad_topology_field)
	{
		Computed_field *field = (reinterpret_cast<Computed_field *>(cad_topology_field));
		Computed_field_cad_topology *cad_topology = reinterpret_cast<Computed_field_cad_topology*>(field->core);
		cad_topology->information(id);
	}
}

int gfx_list_cad_entity(struct Parse_state *state,
	void *cad_element_type_void, void *root_region_void)
{
	int return_code = 0, path_length;//, cad_element_type = 0;
	int identifier_number = -1;
	struct cmzn_region *root_region = NULL;//, *region = NULL;
	char selected_flag, shape_flag = 0x00, surface_flag = 0x00,
		curve_flag = 0x00, point_flag = 0x00;
	struct cmzn_region_path_and_name region_path_and_name;
	struct Computed_field *field = NULL;
	//struct LIST(Computed_field) *list_of_fields = NULL;
	struct Option_table *option_table, *identifier_type_option_table;

	ENTER(execute_command_gfx_import);
	USE_PARAMETER(cad_element_type_void);

	if (state && (root_region = (struct cmzn_region *)root_region_void))
	{
		selected_flag = 0;
		//cad_element_type = *((int *)cad_element_type_void);
		region_path_and_name.region = (struct cmzn_region *)NULL;
		region_path_and_name.region_path = (char *)NULL;
		region_path_and_name.name = (char *)NULL;
		
		option_table=CREATE(Option_table)();
		/* identifier_type */
		identifier_type_option_table=CREATE(Option_table)();
		Option_table_add_entry(identifier_type_option_table, "shape",
			&shape_flag, (void *)"DEFAULT", set_char_flag);
		Option_table_add_entry(identifier_type_option_table, "surface",
			&surface_flag, (void *)NULL, set_char_flag);
		Option_table_add_entry(identifier_type_option_table, "curve",
			&curve_flag, (void *)NULL, set_char_flag);
		Option_table_add_entry(identifier_type_option_table,"point",
			&point_flag, (void *)NULL, set_char_flag);
		Option_table_add_suboption_table(option_table,
			identifier_type_option_table);
		/* identifier_number */
		Option_table_add_entry(option_table,"identifier_number",
			&identifier_number,NULL,set_int);
		/* selected */
		Option_table_add_entry(option_table, "selected", &selected_flag,
			NULL, set_char_flag);
		/* default option: region_path and/or field_name */
		Option_table_add_region_path_and_or_field_name_entry(
			option_table, NULL, &region_path_and_name, root_region);
		if ((return_code = Option_table_multi_parse(option_table,state)))
		{
			field = (struct Computed_field *)NULL;
			if (region_path_and_name.name)
			{
				/* following accesses the field, if any */
				field = cmzn_region_find_field_by_name(region_path_and_name.region,
					region_path_and_name.name);
				if (!field)
				{
					display_message(ERROR_MESSAGE,
						"gfx list field:  There is no field or child region called %s in region %s",
						region_path_and_name.name, region_path_and_name.region_path);
					return_code = 0;
				}
			}
			if (return_code)
			{
				path_length = 0;
				if (region_path_and_name.region_path)
				{
					path_length = strlen(region_path_and_name.region_path);
					if (path_length > 0)
					{
						path_length += strlen(CMZN_REGION_PATH_SEPARATOR_STRING);
					}
				}
				else
				{
					region_path_and_name.region = ACCESS(cmzn_region)(root_region);
				}
				Cad_primitive_identifier cad_identifier;
				cad_identifier.number = identifier_number;
				if (shape_flag)
					cad_identifier.type = Cad_primitive_SHAPE;
				else if (surface_flag)
					cad_identifier.type = Cad_primitive_SURFACE;
				else if (curve_flag)
					cad_identifier.type = Cad_primitive_CURVE;
				else if (point_flag)
					cad_identifier.type = Cad_primitive_POINT;
				else
					cad_identifier.type = Cad_primitive_SHAPE;
				//DEBUG_PRINT("gfx_list_cad_entity:  Region path is '%s', id # %d\n", region_path_and_name.region_path, cad_identifier.number);
				//DEBUG_PRINT("gfx_list_cad_entity:  Flags sh '%c', su '%c', cu '%c', pt '%c'\n", shape_flag, surface_flag, curve_flag, point_flag);
				//DEBUG_PRINT("gfx_list_cad_entity:  id type '%d'\n", cad_identifier.type);
				if (selected_flag)
				{
					cmzn_scene *scene = cmzn_region_get_scene(region_path_and_name.region);
					cmzn_field_group_id selection_group = cmzn_scene_get_selection_group(scene);

					struct MANAGER(Computed_field) *manager = cmzn_region_get_Computed_field_manager(
						region_path_and_name.region);
					const cmzn_set_cmzn_field& field_set = Computed_field_manager_get_fields(manager);
					cmzn_set_cmzn_field::const_iterator it = field_set.begin();
					for (;it != field_set.end(); it++)
					{
						struct Computed_field *field = *it;
						cmzn_field_cad_topology_id cad_topology = cmzn_field_cast_cad_topology(field);
						if (cad_topology)
						{
							char *name = cmzn_field_get_name(field);
							//display_message(INFORMATION_MESSAGE, "cad topology field: %s\n", name);
							free(name);
							cmzn_field_cad_primitive_group_template_id cad_primitive_group = 
								cmzn_field_group_get_cad_primitive_group(selection_group, cad_topology);
							if (cad_primitive_group)
							{
								cmzn_cad_identifier_id cad_identifier = cmzn_field_cad_primitive_group_template_get_first_cad_primitive(cad_primitive_group);
								while (cad_identifier != NULL)
								{
									//display_message(INFORMATION_MESSAGE, "cad id %p %d %d\n", cad_identifier->cad_topology, cad_identifier->identifier.type, cad_identifier->identifier.number);
									Cad_topology_information(cad_identifier->cad_topology, cad_identifier->identifier);
									delete cad_identifier;
									cad_identifier = cmzn_field_cad_primitive_group_template_get_next_cad_primitive(cad_primitive_group);
								}
								cmzn_field_cad_primitive_group_template_destroy(&cad_primitive_group);
							}
							cmzn_field_destroy(&field);
						}
					}
					//DEBUG_PRINT("gfx_list_cad_entity:  Use selection\n");
				}
				else
				{
					/* following accesses the field, if any */
					field = cmzn_region_find_field_by_name(region_path_and_name.region,
						region_path_and_name.name);
					if (!field)
					{
						display_message(ERROR_MESSAGE,
							"gfx_list_cad_entity:  There is no field or child region called %s in region %s",
							region_path_and_name.name, region_path_and_name.region_path);
						return_code = 0;
					}
					if (field)
					{
						struct LIST(Computed_field) *domain_field_list = CREATE_LIST(Computed_field)();
						return_code = Computed_field_get_domain(field, domain_field_list);
						if ( return_code )
						{
							struct Computed_field *cad_topology_field = FIRST_OBJECT_IN_LIST_THAT(Computed_field)
								( cmzn_field_is_type_cad_topology, (void *)NULL, domain_field_list );
							if ( cad_topology_field )
							{
								// if topology domain then draw item at location
								cmzn_field_cad_topology_id cad_topology = cmzn_field_cast_cad_topology(cad_topology_field);
								Cad_topology_information( cad_topology, cad_identifier );
								cmzn_field_cad_topology_destroy(&cad_topology);
							}
						}
						DESTROY_LIST(Computed_field)(&domain_field_list);
						DEACCESS(Computed_field)(&field);
						if (!return_code)
						{
							display_message(ERROR_MESSAGE,
								"gfx_list_cad_entity.  Failed to list cad entity information");
						}
					}
				}
			}
		}
		DESTROY(Option_table)(&option_table);
		if (region_path_and_name.region)
		{
			DEACCESS(cmzn_region)(&region_path_and_name.region);
		}
		if (region_path_and_name.region_path)
		{
			DEALLOCATE(region_path_and_name.region_path);
		}
		if (region_path_and_name.name)
		{
			DEALLOCATE(region_path_and_name.name);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"gfx_list_cad_entity.  Invalid argument(s)\n");
	}
	LEAVE;

	return return_code;
}


