/*
 * computed_field_cad_topology_private.h
 *
 *  Created on: 13-Oct-2009
 *      Author: hsorby
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef COMPUTED_FIELD_CAD_TOPOLOGY_PRIVATE_H_
#define COMPUTED_FIELD_CAD_TOPOLOGY_PRIVATE_H_

int Computed_field_cad_topology_get_surface_point(cmzn_field_cad_topology_id field, cmzn_cad_surface_identifier identifier, double u, double v, double *point, double *uDerivative, double *vDerivative);
int Computed_field_cad_topology_get_curve_point(cmzn_field_cad_topology_id field, cmzn_cad_curve_identifier identifier, double s, double *point);
int Computed_field_cad_topology_get_surface_colour(cmzn_field_cad_topology_id field, cmzn_cad_surface_identifier identifier, double u, double v, double *colour);

#endif /* COMPUTED_FIELD_CAD_TOPOLOGY_PRIVATE_H_ */

