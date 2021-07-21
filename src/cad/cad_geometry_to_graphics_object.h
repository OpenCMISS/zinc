/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined(CAD_CAD_GEOMETRY_TO_GRAPHICS_OBJECT_H)
#define CAD_CAD_GEOMETRY_TO_GRAPHICS_OBJECT_H

#include "cad/computed_field_cad_topology.h"
#include "cad/cad_element.h"

/**
 * Create a GT surface from the given surface identifier taken from the cad topology
 * @param cad_topology a cad topology field upon which the geometry of the cad
 *        shape is based
 * @param field_cache  Cache object for field evaluations.
 * @param coordinate_field the coordinate field
 * @param data_field the data field into which the colour information is placed
 * @param render_type the render type to be displayed wireframe or surface
 * @param surface_index the surface identifier of the surface that is to be created from the 
 *        cad topology
 * @returns a GT surface for display, or NULL if the surface cannot be created
 */
struct GT_surface *create_surface_from_cad_shape(cmzn_field_cad_topology_id cad_topology,
	cmzn_field_cache_id field_cache, struct Computed_field *coordinate_field,
	struct Computed_field *data_field, cmzn_graphics_render_type render_type, cmzn_cad_surface_identifier surface_index);

/**
 * Carete a curve from the given cad topology
 */
struct GT_polyline_vertex_buffers *create_curves_from_cad_shape(cmzn_field_cad_topology_id cad_topology,
	cmzn_field_cache_id field_cache, struct Computed_field *coordinate_field,
	struct Computed_field *data_field, struct GT_object *graphics_object);

#endif /* !defined(CAD_CAD_GEOMETRY_TO_GRAPHICS_OBJECT_H) */
