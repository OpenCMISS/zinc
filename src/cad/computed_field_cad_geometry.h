/*
 * FILE : computed_field_cad_geometry.h
 *
 * Implements a cmiss field which extracts the geometry of an OpenCascade shape.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (COMPUTED_FIELD_CAD_GEOMETRY_H)
#define COMPUTED_FIELD_CAD_GEOMETRY_H

extern "C" {
#include "api/cmiss_field.h"
}
#include "cad/geometricshape.h"

#define Computed_field_create_cad_geometry cmzn_field_create_cad_geometry

struct cmzn_field_cad_geometry;

typedef struct cmzn_field_cad_geometry *cmzn_field_cad_geometry_id;

//cmzn_field_id Computed_field_create_cad_geometry( cmzn_field_id field, GeometricShape *shape );
cmzn_field_id Computed_field_module_create_cad_geometry(cmzn_field_module_id field_module, cmzn_field_id cad_topology_field);

cmzn_field_cad_geometry_id cmzn_field_cad_geometry_cast(cmzn_field_id cad_geometry_field);

int cmzn_field_is_cad_geometry(cmzn_field_id field, void *not_in_use);

#endif /* !defined (COMPUTED_FIELD_CAD_GEOMETRY_H) */

