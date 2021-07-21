/*******************************************************************************
FILE : finite_element_to_graphics_object.h

LAST MODIFIED : 14 March 2003

DESCRIPTION :
The function prototypes for creating graphical objects from finite elements.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (FINITE_ELEMENT_TO_GRAPHICAL_OBJECT_H)
#define FINITE_ELEMENT_TO_GRAPHICAL_OBJECT_H

#include "opencmiss/zinc/types/nodesetid.h"
#include "computed_field/computed_field.h"
#include "finite_element/finite_element.h"
#include "general/enumerator.h"
#include "general/multi_range.h"
#include "graphics/auxiliary_graphics_types.h"
#include "graphics/graphics_object.h"
#include "graphics/volume_texture.h"

/*
Global types
------------
*/


enum Collapsed_element_type
/*******************************************************************************
LAST MODIFIED : 25 July 2001

DESCRIPTION :
How an element is collapsed.
???DB.  Currently only 2D
==============================================================================*/
{
	ELEMENT_NOT_COLLAPSED,
	ELEMENT_COLLAPSED_XI1_0,
	ELEMENT_COLLAPSED_XI1_1,
	ELEMENT_COLLAPSED_XI2_0,
	ELEMENT_COLLAPSED_XI2_1
}; /* enum Collapsed_element_type */

/*
Global functions
----------------
*/

int make_glyph_orientation_scale_axes(
	int number_of_orientation_scale_values, FE_value *orientation_scale_values,
	FE_value *axis1,FE_value *axis2, FE_value *axis3, FE_value *size);
/*******************************************************************************
LAST MODIFIED : 16 November 2000

DESCRIPTION :
Computes the three glyph orientation axes from the <orientation_scale_values>.

The orientation is understood from the number_of_orientation_scale_values as:
0 = zero scalar (no vector/default orientation);
1 = scalar (no vector/default orientation);
2 = 1 2-D vector (2nd glyph axis is normal in plane, 3rd is out of 2-D plane);
3 = 1 3-D vector (orthogonal 2nd and 3rd glyph axes are arbitrarily chosen);
4 = 2 2-D vectors (3rd glyph axis taken as out of 2-D plane);
6 = 2 3-D vectors (3rd glyph axis found from cross product);
9 = 3 3-D vectors = complete definition of glyph axes;

The scaling behaviour depends on the number of vectors interpreted above, where:
0 = isotropic scaling on all three axes by scalar;
1 = isotropic scaling on all three axes by magnitude of vector;
2 = scaling in direction of 2 vectors, ie. they keep their current length, unit
    vector in 3rd axis;
3 = scaling in direction of 3 vectors - ie. they keep their current length.

Function returns the axes as unit vectors with their magnitudes in the <size>
array. This is always possible if there is a scalar (or zero scalar), but where
zero vectors are either read or calculated from the <orientation_scale_values>,
these are simply returned, since no valid direction can be produced.
@param field_cache  cmzn_fieldcache for evaluating fields. Time is expected
to be set in the field_cache if needed.
==============================================================================*/

int add_glyphset_vertex_from_FE_element(
	struct GT_object *graphics_object,
	cmzn_fieldcache_id field_cache,
	struct FE_element *element, struct FE_element *top_level_element,
	struct Computed_field *coordinate_field,
	int number_of_xi_points, FE_value_triple *xi_points, struct GT_object *glyph,
	struct Computed_field *orientation_scale_field,
	struct Computed_field *variable_scale_field,
	struct Computed_field *data_field,
	struct Computed_field *label_field,
	enum cmzn_graphics_select_mode select_mode, int element_selected,
	struct Multi_range *selected_ranges, int *point_numbers);
/*******************************************************************************
LAST MODIFIED : 18 November 2005

DESCRIPTION :
Converts a finite element into a set of glyphs displaying information
about fields defined over it.
At each of the <number_of_xi_points> <xi_points> the <glyph> of at least
<base_size> with the given glyph <offset> is displayed.
The optional <orientation_scale_field> can be used to orient and scale the
glyph in a manner depending on the number of components in the field. The
optional <variable_scale_field> can provide signed scaling independently of the
glyph axis directions. See function make_glyph_orientation_scale_axes for
details. The combined scale from the above 2 fields is multiplied in each axis
by the <scale_factors> then added to the base_size.
The optional <data_field> (currently only a scalar) is calculated as data over
the glyph_set, for later colouration by a spectrum.
The optional <label_field> is written beside each glyph in string form.
The optional <top_level_element> may be provided as a clue to Computed_fields
to say which parent element they should be evaluated on as necessary.
<select_mode> is used in combination with the <element_selected> and
<selected_ranges> to draw only those points with numbers in or out of the given
ranges when given value CMZN_GRAPHICS_SELECT_MODE_DRAW_SELECTED or CMZN_GRAPHICS_SELECT_MODE_DRAW_UNSELECTED.
If <element_selected> is true, all points are selected, otherwise selection is
determined from the <selected_ranges>, and if <selected_ranges> is NULL, no
numbers are selected.
If <point_numbers> are supplied then points numbers for OpenGL picking are taken
from this array, otherwise they are sequential, starting at 0.
Note:
- the coordinate and orientation fields are assumed to be rectangular cartesian.
- the coordinate system of the variable_scale_field is ignored/not used.
@param field_cache  cmzn_fieldcache for evaluating fields. Time is expected
to be set in the field_cache if needed.
==============================================================================*/

struct GT_glyphset_vertex_buffers *Nodeset_create_vertex_array(
	cmzn_nodeset_id nodeset, cmzn_fieldcache_id field_cache,
	struct GT_object *graphics_object,
	enum cmzn_glyph_repeat_mode glyph_repeat_mode,
	struct Computed_field *coordinate_field,
	struct Computed_field *data_field,
	struct Computed_field *orientation_scale_field,
	struct Computed_field *variable_scale_field,
	struct Computed_field *label_field,
	struct Computed_field *label_density_field,
	struct Computed_field *subgroup_field,
	struct Computed_field *group_field,
	struct GT_object *glyph,
	const FE_value *base_size, const FE_value *offset, const FE_value *scale_factors,
	struct cmzn_font *font, FE_value *label_offset, char *static_label_text[3],
	enum cmzn_graphics_select_mode select_mode);
/*******************************************************************************
Creates a GT_glyphset_vertex_buffer displaying a <glyph> of at least <base_size>, with the
given glyph <offset> at each node in <fe_region>.
The optional <orientation_scale_field> can be used to orient and scale the
glyph in a manner depending on the number of components in the field. The
optional <variable_scale_field> can provide signed scaling independently of the
glyph axis directions. See function make_glyph_orientation_scale_axes for
details. The combined scale from the above 2 fields is multiplied in each axis
by the <scale_factors> then added to the base_size.
The optional <data_field> is calculated as data over the glyph_set, for later
colouration by a spectrum.
The optional <label_field> is written beside each glyph in string form.
The optional <label_density_field> controls the number of times the label
field is rendered for glyphs that can render the label many times, such as axes
and graph grids.
The <select_mode> controls whether node cmiss numbers are output as integer
names with the glyph_set. If <select_mode> is DRAW_SELECTED or DRAW_UNSELECTED,
only nodes in (or not in) the <selected_node_list> are rendered.
Notes:
- the coordinate and orientation fields are assumed to be rectangular cartesian.
- the coordinate system of the variable_scale_field is ignored/not used.
==============================================================================*/

/***************************************************************************//**
 * Adds vertex values to the supplied vertex array to create a line representing
 * the 1-D finite element.
 *
 * @param field_cache  cmzn_fieldcache for evaluating fields. Time is expected
 * to be set in the field_cache if needed.
 * @param element The element that primitives will be generated for.
 * @param vertex_array The array which this primitive will be added to.
 * @param coordinate_field The required position coordinate field.  It is assumed
 * that this field is rectangular cartesian (already wrapped if the original coordinates were not).
 * @param data_field Optional data field, if not wanted set to NULL.
 * @param number_of_data_values The number of components in the data_field and
 * the allocated size of the data_buffer.
 * @param data_buffer A buffer large enough to evaluate the data_field into.
 * @param texture_coordinate_field Optional field usually used for positioning textures.
 * @param number_of_segments The supplied element is broken into this many linear
 * segments in the created primitive.
 * @param top_level_element Optional element may be provided as a clue to Computed_fields
 * to say which parent element they should be evaluated on as necessary.
 */
int FE_element_add_line_to_vertex_array(struct FE_element *element,
	cmzn_fieldcache_id field_cache, struct Graphics_vertex_array *array,
	Computed_field *coordinate_field,
	int number_of_data_values, Computed_field *data_field,
	Computed_field *texture_coordinate_field,
	unsigned int number_of_segments, FE_element *top_level_element);

/***************************************************************************//**
 * Fill the array with coordinates from the <coordinate_field> for the 2-D finite
 * <element> using an array of <number_of_segments_in_xi1> by
 * <number_of_segments_in_xi2> rectangles in xi space.  The spacing is constant
 * in each of xi1 and xi2.
 * The coordinate field is assumed to be rectangular cartesian.
 * The optional <texture_coordinate_field> is evaluated at each vertex for the
 * corresponding texture coordinates.  If none is supplied then a length based
 * algorithm is used instead.
 * The optional <data_field> is calculated as data over the surface, for later
 * colouration by a spectrum.
 * The optional <top_level_element> may be provided as a clue to Computed_fields
 * to say which parent element they should be evaluated on as necessary.
 * @param field_cache  cmzn_fieldcache for evaluating fields. Time is expected
 * to be set in the field_cache if needed.
 * @param surface_mesh  2-D surface mesh being converted to surface graphics.
*/
int FE_element_add_surface_to_vertex_array(struct FE_element *element,
	cmzn_fieldcache_id field_cache, cmzn_mesh_id surface_mesh,
	struct Graphics_vertex_array *array,
	struct Computed_field *coordinate_field,
	struct Computed_field *texture_coordinate_field,
	struct Computed_field *data_field,
	unsigned int number_of_segments_in_xi1_requested,
	unsigned int number_of_segments_in_xi2_requested,
	char reverse_normals, struct FE_element *top_level_element);

/***************************************************************************//**
 * Fills the array with coordinates from the <coordinate_field> and the radius for
 * the 1-D finite <element> using a grid of points.  The cylinder is built out of an
 * array of rectangles <number_of_segments_along> by <number_of_segments_around>
 * the cylinder.The diameter is calculated as:
 * diameter = base_size[0] + scale_factor[0]*orientation_scale_field[0]
 * The optional <data_field> is calculated over the length of the cylinder, for
 * later colouration by a spectrum.
 * The optional <top_level_element> may be provided as a clue to Computed_fields
 * to say which parent element they should be evaluated on as necessary.
 * The first component of <texture_coordinate_field> is used to control the
 * texture coordinates along the element. If not supplied it will match Xi. The
 * texture coordinate around the cylinders is always from 0 to 1.
 * Notes:
 * - the coordinate field is assumed to be rectangular cartesian.
 * @param field_cache  cmzn_fieldcache for evaluating fields. Time is expected
 * to be set in the field_cache if needed.
 */
int FE_element_add_cylinder_to_vertex_array(struct FE_element *element,
	cmzn_fieldcache_id field_cache,
	struct Graphics_vertex_array *array, cmzn_mesh_id line_mesh,
	struct Computed_field *coordinate_field,
	struct Computed_field *data_field,
	const FE_value *base_size,
	const FE_value *scale_factors,
	cmzn_field_id orientation_scale_field,
	int number_of_segments_along, int number_of_segments_around,
	struct Computed_field *texture_coordinate_field,
	struct FE_element *top_level_element);

/****************************************************************************//**
 * Sorts out how standard, polygon and simplex elements are segmented, based on
 * numbers of segments requested for "square" elements.
 */
int get_surface_element_segmentation(struct FE_element *element,
	int number_of_segments_in_xi1_requested,
	int number_of_segments_in_xi2_requested,
	int *number_of_points_in_xi1,int *number_of_points_in_xi2,
	int *number_of_points,int *number_of_polygon_vertices,
	gtPolygonType *polygon_type,enum Collapsed_element_type *collapsed_element,
	enum FE_element_shape_type *shape_type_address);

int Cmiss_graphic_point_graphics_to_vertex_buffer(
	cmzn_fieldcache_id field_cache, FE_value time,
	struct Computed_field *label_field,
	const FE_value *base_size, const FE_value *offset, const FE_value *scale_factors,
	struct Graphics_vertex_array *array);

#endif /* !defined (FINITE_ELEMENT_TO_GRAPHICAL_OBJECT_H) */
