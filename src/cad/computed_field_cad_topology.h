/*
 * FILE : computed_field_cad_topology.h
 *
 * Implements a cmiss field which wraps an OpenCASCADE shape.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (COMPUTED_FIELD_CAD_TOPOLOGY_H)
#define COMPUTED_FIELD_CAD_TOPOLOGY_H

extern "C" {
#include "api/cmiss_field.h"
#include "api/cmiss_field_cad.h"
#include "command/parser.h"
#include "computed_field/computed_field.h"
}
#include "cad/topologicalshape.h"
#include "cad/geometricshape.h"
#include "cad/cad_element.h"


//#define Computed_field_module_create_cad_topology cmzn_field_create_cad_topology
//#define cmzn_field_cad_topology Computed_field_cad_topology

//class cmzn_field_cad_topology;


cmzn_field_id cmzn_field_module_create_cad_topology(cmzn_field_module *field_module, TopologicalShape *shape );

cmzn_field_cad_topology_id cmzn_field_cast_cad_topology(cmzn_field_id cad_topology_field );

int cmzn_field_cad_topology_destroy(cmzn_field_cad_topology_id *cad_topology_field_address);

cmzn_field_cad_topology_id cmzn_field_cad_topology_access(cmzn_field_cad_topology_id cad_topology_field);

int cmzn_field_cad_topology_get_surface_count(cmzn_field_cad_topology_id cad_topology_field);

int cmzn_field_cad_topology_get_curve_count(cmzn_field_cad_topology_id cad_topology_field);

int cmzn_field_cad_topology_get_surface_point_count(cmzn_field_cad_topology_id cad_topology_field, cmzn_cad_surface_identifier identifier);

int cmzn_field_cad_topology_get_curve_point_count(cmzn_field_cad_topology_id cad_topology_field, cmzn_cad_curve_identifier identifier);

int cmzn_field_cad_topology_get_surface_point_uv_coordinates(cmzn_field_cad_topology_id cad_topology_field,
	cmzn_cad_surface_identifier identifier,
	cmzn_cad_surface_point_identifier uv_identifier,
	double &u, double &v);

int cmzn_field_cad_topology_get_curve_point_s_coordinate(cmzn_field_cad_topology_id cad_topology_field,
	cmzn_cad_curve_identifier identifier,
	cmzn_cad_curve_point_identifier s_identifier,
	double &s);

void cmzn_field_cad_topology_set_geometric_shape(cmzn_field_cad_topology_id cad_topology_field, GeometricShape *shape);

int cmzn_field_is_type_cad_topology(cmzn_field_id field, void *not_in_use);

void Cad_topology_information( cmzn_field_cad_topology_id cad_topology_field, Cad_primitive_identifier information );

int gfx_list_cad_entity(struct Parse_state *state, void *cad_element_type_void, void *root_region_void);

#endif /* !defined (COMPUTED_FIELD_CAD_SHAPE_H) */

