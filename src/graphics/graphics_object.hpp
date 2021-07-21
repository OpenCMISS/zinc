/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/**
 * C++ interfaces for graphics_object.cpp
 */

#ifndef GRAPHICS_OBJECT_HPP
#define GRAPHICS_OBJECT_HPP

#include "graphics/graphics_vertex_array.hpp"

class Render_graphics;

typedef int (*Graphics_object_glyph_labels_function)(Triple *coordinate_scaling,
	int label_bounds_dimension, int label_bounds_components, ZnReal *label_bounds,
	Triple *label_density,
	cmzn_material *material, cmzn_material *secondary_material,
	struct cmzn_font *font, Render_graphics *renderer);
/*******************************************************************************
LAST MODIFIED : 18 November 2005

DESCRIPTION :
Used for rendering a per compile custom addon to a glyph, such as a grid or tick
marks showing the scale of the glyph.
<coordinate_scaling> gives a representative size for determining the number of 
ticks.
==============================================================================*/

Graphics_object_glyph_labels_function Graphics_object_get_glyph_labels_function(
	struct GT_object *graphics_object);
/*******************************************************************************
LAST MODIFIED : 19 September 2005

DESCRIPTION :
Gets the glyph_labels_function of the <graphics_object>.
This function enables a custom, per compile, labelling for a graphics object 
==============================================================================*/

int Graphics_object_set_glyph_labels_function(struct GT_object *graphics_object,
	Graphics_object_glyph_labels_function glyph_labels_function);
/*******************************************************************************
LAST MODIFIED : 19 September 2005

DESCRIPTION :
Sets the glyph_labels_function of the <graphics_object>.
This function enables a custom, per compile, labelling for a graphics object 
==============================================================================*/

#endif /* GRAPHICS_OBJECT_HPP */
