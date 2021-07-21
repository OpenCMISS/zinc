/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

extern "C" {
#include "computed_field/computed_field.h"
}

#include "general/value.h"
#include "cad/field_location.hpp"

Field_cad_geometry_location::Field_cad_geometry_location(cmzn_field_cad_topology_id id, FE_value time, int number_of_derivatives)
	: Field_location(time, number_of_derivatives)
	, id(id)
{
}

Field_cad_geometry_location::~Field_cad_geometry_location()
{
}

Field_cad_geometry_surface_location::Field_cad_geometry_surface_location(cmzn_field_cad_topology_id id, cmzn_cad_surface_identifier identifier, double u, double v, FE_value time, int number_of_derivatives)
	: Field_cad_geometry_location(id, time, number_of_derivatives)
	, identifier(identifier)
	, u(u)
	, v(v)
{
}

Field_cad_geometry_surface_location::~Field_cad_geometry_surface_location()
{
}

Field_cad_geometry_curve_location::Field_cad_geometry_curve_location(cmzn_field_cad_topology_id id, cmzn_cad_curve_identifier identifier, double s, FE_value time, int number_of_derivatives)
	: Field_cad_geometry_location(id, time, number_of_derivatives)
	, identifier(identifier)
	, s(s)
{
}

Field_cad_geometry_curve_location::~Field_cad_geometry_curve_location()
{
}

