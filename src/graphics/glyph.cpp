/**
 * FILE : glyph.cpp
 *
 * Glyphs produce a GT_object to represent simple geometric shapes such as
 * cylinders, arrows and axes which are (or should) fit into a unit (1x1x1) cube,
 * with the major axes of the glyph aligned with the x, y and z axes. 
 * The logical centre of each glyph should be at (0,0,0). This should be
 * interpreted as follows:
 * - if the glyph is symmetrical along any axis, its coordinates in that
 * direction should vary from -0.5 to +0.5;
 * - if the glyph involves any sort of arrow that is unsymmetric in its direction
 * (ie. is single-headed), (0,0,0) should be at the base of the arrow.
 * - axes should therefore be centred at (0,0,0) and extend to 1 in each axis
 * direction. Axis titles "x", "y" and "z" may be outside the unit cube.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <algorithm>
#include <iterator>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>
#include "opencmiss/zinc/types/fontid.h"
#include "opencmiss/zinc/status.h"
#include "opencmiss/zinc/material.h"
#include "general/debug.h"
#include "general/enumerator_private.hpp"
#include "general/enumerator_conversion.hpp"
#include "general/manager_private.h"
#include "general/message.h"
#include "general/mystring.h"
#include "graphics/glyph.hpp"
#include "graphics/glyph_axes.hpp"
#include "graphics/glyph_circular.hpp"
#include "graphics/graphics.h"
#include "graphics/graphics_object.h"
#include "graphics/graphics_object_private.hpp"
#include "graphics/spectrum.h"
#include "graphics/graphics_object.hpp"
#include "graphics/render_gl.h"

/*
Module functions
----------------
*/

PROTOTYPE_ACCESS_OBJECT_FUNCTION(cmzn_glyph)
{
	return object->access();
}

PROTOTYPE_DEACCESS_OBJECT_FUNCTION(cmzn_glyph)
{
	return cmzn_glyph::deaccess(object_address);
}

PROTOTYPE_REACCESS_OBJECT_FUNCTION(cmzn_glyph)
{
	if (object_address)
	{
		if (new_object)
			new_object->access();
		if (*object_address)
			cmzn_glyph::deaccess(object_address);
		*object_address = new_object;
		return 1;
	}
	return 0;
}

DECLARE_DEFAULT_GET_OBJECT_NAME_FUNCTION(cmzn_glyph)

DECLARE_INDEXED_LIST_STL_FUNCTIONS(cmzn_glyph)
DECLARE_FIND_BY_IDENTIFIER_IN_INDEXED_LIST_STL_FUNCTION(cmzn_glyph,name,const char *)
DECLARE_INDEXED_LIST_STL_IDENTIFIER_CHANGE_FUNCTIONS(cmzn_glyph,name)
DECLARE_CREATE_INDEXED_LIST_STL_ITERATOR_FUNCTION(cmzn_glyph,cmzn_glyphiterator)

FULL_DECLARE_MANAGER_TYPE(cmzn_glyph);

DECLARE_LOCAL_MANAGER_FUNCTIONS(cmzn_glyph)

DECLARE_MANAGER_FUNCTIONS(cmzn_glyph, manager)

DECLARE_DEFAULT_MANAGED_OBJECT_NOT_IN_USE_FUNCTION(cmzn_glyph, manager)

DECLARE_MANAGER_IDENTIFIER_WITHOUT_MODIFY_FUNCTIONS(cmzn_glyph, name, const char *, manager)

cmzn_glyph::~cmzn_glyph()
{
	if (name)
		DEALLOCATE(name);
}

int cmzn_glyph::setName(const char *newName)
{
	if (!newName)
		return CMZN_ERROR_ARGUMENT;
	if (this->name && (0 == strcmp(this->name, newName)))
		return CMZN_OK;
	cmzn_set_cmzn_glyph *allGlyphs = 0;
	bool restoreObjectToLists = false;
	if (this->manager)
	{
		allGlyphs = reinterpret_cast<cmzn_set_cmzn_glyph *>(this->manager->object_list);
		cmzn_glyph *existingGlyph = FIND_BY_IDENTIFIER_IN_MANAGER(cmzn_glyph, name)(newName, this->manager);
		if (existingGlyph)
		{
			display_message(ERROR_MESSAGE, "cmzn_glyph::setName.  Glyph named '%s' already exists.", newName);
			return CMZN_ERROR_ARGUMENT;
		}
		else
		{
			// this temporarily removes the object from all related lists
			restoreObjectToLists = allGlyphs->begin_identifier_change(this);
			if (!restoreObjectToLists)
			{
				display_message(ERROR_MESSAGE, "cmzn_glyph::setName.  "
					"Could not safely change identifier in manager");
				return CMZN_ERROR_GENERAL;
			}
		}
	}
	if (this->name)
		DEALLOCATE(this->name);
	this->name = duplicate_string(newName);
	if (restoreObjectToLists)
		allGlyphs->end_identifier_change();
	if (this->manager)
		MANAGED_OBJECT_CHANGE(cmzn_glyph)(this, MANAGER_CHANGE_IDENTIFIER(cmzn_glyph));
	return CMZN_OK;
}

void cmzn_glyph_static::materialChange(struct MANAGER_MESSAGE(cmzn_material) *message)
{
	GT_object *object = this->graphicsObject;
	bool changed = false;
	while (object)
	{
		cmzn_material *material = get_GT_object_default_material(object);
		if (material)
		{
			int change_flags = MANAGER_MESSAGE_GET_OBJECT_CHANGE(cmzn_material)(message, material);
			if (change_flags & MANAGER_CHANGE_RESULT(cmzn_material))
			{
				GT_object_changed(object);
				changed = true;
			}
		}
		object = GT_object_get_next_object(object);
	}
	if (changed)
	{
		this->changed();
	}
}

static int tick_mark_get_grid_spacing(FE_value *minor_grid_size,
	int *minor_grids_per_major,FE_value scale, FE_value min_minor_grid,
	FE_value min_major_grid)
/*******************************************************************************
LAST MODIFIED : 19 September 2005

DESCRIPTION :
Returns the spacing of minor and major (major include labels in this case) grids
that cross the drawing area. From an initial, desired <minor_grid_spacing>, this
is enlarged by multiplying by 2,2.5,2 (to give 2x,5x,10x,20x the original value)
until the grid is spaced apart by at least <min_minor_grid_pixels> on the
screen. FE_Value values are multiplied by <scale> to give pixel values.
A suitable number of minor_grids_per_major is then calculated, ensuring that
major grid lines are spaced apart by at least <min_major_grid_pixels>.
Copied from the control curve editor.
==============================================================================*/
{
	int return_code,j;

	ENTER(tick_mark_get_grid_spacing);
	if (*minor_grid_size&&(0< *minor_grid_size)&&minor_grids_per_major&&
		(0.0!=scale))
	{
		j=1;
		while (fabs(scale*(*minor_grid_size)) < min_minor_grid)
		{
			j=(j+1)%3;
			if (j)
			{
				*minor_grid_size *= 2.0;
			}
			else
			{
				*minor_grid_size *= 2.5;
			}
		}
		*minor_grids_per_major=1;
		if (0==j)
		{
			j=1;
			if (fabs(scale*(*minor_grids_per_major)*(*minor_grid_size))<
				min_major_grid)
			{
				*minor_grids_per_major *= 2;
			}
		}
		while (fabs(scale*(*minor_grids_per_major)*(*minor_grid_size))<
			min_major_grid)
		{
			j=(j+1)%2;
			if (j)
			{
				*minor_grids_per_major *= 5;
			}
			else
			{
				*minor_grids_per_major *= 2;
			}
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"tick_mark_get_grid_spacing.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* tick_mark_get_grid_spacing */

static int draw_glyph_axes_general(Triple *coordinate_scaling, 
	int label_bounds_dimension, int label_bounds_components, ZnReal *label_bounds,
	Triple *label_density,
	int primary_axis_number, int label_bounds_component,
	ZnReal major_cross_min, ZnReal major_cross_max,
	ZnReal minor_cross_min, ZnReal minor_cross_max, FE_value minor_grid_size,
	int minor_grids_per_major, FE_value min_minor_grid, FE_value min_major_grid,
	cmzn_material *material, cmzn_material *secondary_material,
	struct cmzn_font *font, Render_graphics *renderer)
/*******************************************************************************
LAST MODIFIED : 22 November 2005

DESCRIPTION :
Renders the label_bounds as lines and labels.  
==============================================================================*/
{
	const char *name = "axes_ticks_temporary";
	char **label_strings;
	FE_value grid_scale;
	int first, j, label_bounds_offset, last, number_of_major_lines, 
		number_of_minor_lines, number_of_labels, number_of_ticks, return_code;
	ZnReal axis_position, fabs_length, length, log_scale;
	struct GT_object *graphics_object;
	Triple *label_string_locations, *label_vertex, *major_linepoints, *major_vertex, 
		*minor_linepoints, *minor_vertex;

	ENTER(draw_glyph_axes_general);
	if ((label_bounds_dimension > 0) && (label_bounds_components > 0) && label_bounds)
	{
		return_code=1;

		switch(primary_axis_number)
		{
			case 0:
			default:
			{
				label_bounds_offset = label_bounds_components;
			} break;
			case 1:
			{
				label_bounds_offset = 2 * label_bounds_components;
			} break;
			case 2:
			{
				label_bounds_offset = 4 * label_bounds_components;
			} break;
		}

		length = label_bounds[label_bounds_offset + label_bounds_component] -
			label_bounds[label_bounds_component];
		fabs_length = fabs(length);

		if (fabs_length > 1e-7)
		{
			minor_grid_size *= fabs_length;
			log_scale = ceil(-0.5 + log10(minor_grid_size * 2.0));
			minor_grid_size = pow(10, log_scale) / 2.0;
			if (label_density)
			{
				grid_scale = 0.01 * (*label_density)[0] / fabs_length;				
			}
			else
			{
				grid_scale = (*coordinate_scaling)[0] / fabs_length;
			}
			
			tick_mark_get_grid_spacing(&minor_grid_size,
				&minor_grids_per_major, grid_scale, min_minor_grid, min_major_grid);
			
			first=(int)ceil(label_bounds[label_bounds_component] / minor_grid_size);
			last=(int)floor(label_bounds[label_bounds_offset + label_bounds_component]
				/ minor_grid_size);
		}
		else
		{
			length = 1e-7f;
			if (label_density)
			{
				grid_scale = 0.01 * (*label_density)[0];				
			}
			else
			{
				grid_scale = 0.01 * (*coordinate_scaling)[0];
			}

			minor_grids_per_major = 5;
  			tick_mark_get_grid_spacing(&minor_grid_size,
				&minor_grids_per_major, grid_scale, min_minor_grid, min_major_grid);

			first = (int)ceil(label_bounds[label_bounds_component] / minor_grid_size);
			last = first;
		}								
		if (first > last)
		{
			j = last;
			last = first;
			first = j;
		}
		number_of_ticks = last - first + 1;

		number_of_minor_lines = 0;
		number_of_major_lines = 0;
		number_of_labels = 0;
		/* This is too much memory but saves us working it out in advance */
		if (ALLOCATE(major_linepoints, Triple, 2 * number_of_ticks) && 
			ALLOCATE(minor_linepoints, Triple, 2 * number_of_ticks) && 
			ALLOCATE(label_string_locations, Triple, number_of_ticks) &&
			ALLOCATE(label_strings, char *, number_of_ticks))
		{
			minor_vertex = minor_linepoints;
			major_vertex = major_linepoints;
			label_vertex = label_string_locations;
			GT_polyline_vertex_buffers *polyline = 0;
			for (j=first;j<=last;j++)
			{
				if (number_of_ticks > 1)
				{
					axis_position = ((ZnReal)j * minor_grid_size - label_bounds[label_bounds_component]) / length;
				}
				else
				{
					axis_position = 0.0;
				}
				if (0==(j % minor_grids_per_major))
				{
					/* Major */
					switch (primary_axis_number)
					{
						case 0:
						default:
						{
							(*major_vertex)[0] = axis_position;
							(*major_vertex)[1] = major_cross_min;
							(*major_vertex)[2] = 0.0;
							major_vertex++;
							(*major_vertex)[0] = axis_position;
							(*major_vertex)[1] = major_cross_max;
							(*major_vertex)[2] = 0.0;
							major_vertex++;
							(*label_vertex)[0] = axis_position;
							(*label_vertex)[1] = 0.0;
							(*label_vertex)[2] = 0.0;
							label_vertex++;
						} break;
						case 1:
						{
							(*major_vertex)[0] = major_cross_min;
							(*major_vertex)[1] = axis_position;
							(*major_vertex)[2] = 0.0;
							major_vertex++;
							(*major_vertex)[0] = major_cross_max;
							(*major_vertex)[1] = axis_position;
							(*major_vertex)[2] = 0.0;
							major_vertex++;
							(*label_vertex)[0] = 0.0;
							(*label_vertex)[1] = axis_position;
							(*label_vertex)[2] = 0.0;
							label_vertex++;
						} break;
						case 2:
						{
							(*major_vertex)[0] = major_cross_min;
							(*major_vertex)[1] = 0.0;
							(*major_vertex)[2] = axis_position;
							major_vertex++;
							(*major_vertex)[0] = major_cross_max;
							(*major_vertex)[1] = 0.0;
							(*major_vertex)[2] = axis_position;
							major_vertex++;
							(*label_vertex)[0] = 0.0;
							(*label_vertex)[1] = 0.0;
							(*label_vertex)[2] = axis_position;
							label_vertex++;
						} break;
					}
					number_of_major_lines++;

					if (ALLOCATE(label_strings[number_of_labels], char, 50))
					{
						sprintf(label_strings[number_of_labels], "%1g",
							(ZnReal)j * minor_grid_size);
					}
					number_of_labels++;
				}
				else
				{
					/* Minor */
					switch (primary_axis_number)
					{
						case 0:
						default:
						{
							(*minor_vertex)[0] = axis_position;
							(*minor_vertex)[1] = minor_cross_min;
							(*minor_vertex)[2] = 0.0;
							minor_vertex++;
							(*minor_vertex)[0] = axis_position;
							(*minor_vertex)[1] = minor_cross_max;
							(*minor_vertex)[2] = 0.0;
							minor_vertex++;
						} break;
						case 1:
						{
							(*minor_vertex)[0] = minor_cross_min;
							(*minor_vertex)[1] = axis_position;
							(*minor_vertex)[2] = 0.0;
							minor_vertex++;
							(*minor_vertex)[0] = minor_cross_max;
							(*minor_vertex)[1] = axis_position;
							(*minor_vertex)[2] = 0.0;
							minor_vertex++;
						} break;
						case 2:
						{
							(*minor_vertex)[0] = minor_cross_min;
							(*minor_vertex)[1] = 0.0;
							(*minor_vertex)[2] = axis_position;
							minor_vertex++;
							(*minor_vertex)[0] = minor_cross_max;
							(*minor_vertex)[1] = 0.0;
							(*minor_vertex)[2] = axis_position;
							minor_vertex++;
						} break;
					}
					number_of_minor_lines++;
				}
			}
			polyline =
				CREATE(GT_polyline_vertex_buffers)(g_PLAIN_DISCONTINUOUS, 0.0);
			if (polyline)
			{
				graphics_object=CREATE(GT_object)(name,g_POLYLINE_VERTEX_BUFFERS,0);
				if (graphics_object)
				{
					unsigned int number_of_vertices = number_of_major_lines * 2;
					struct Graphics_vertex_array *array = GT_object_get_vertex_set(
						graphics_object);
					fill_line_graphics_vertex_array(array,
						number_of_vertices, major_linepoints, 0, 0, 0);
					if (GT_OBJECT_ADD(GT_polyline_vertex_buffers)(graphics_object,polyline))
					{
						renderer->Graphics_object_render_immediate(graphics_object);
					}
					else
					{
						DESTROY(GT_polyline_vertex_buffers)(&polyline);
						return_code = 0;
					}
					DEACCESS(GT_object)(&graphics_object);
				}
				else
				{
					DESTROY(GT_polyline_vertex_buffers)(&polyline);
					return_code = 0;
				}
			}
			DEALLOCATE(major_linepoints);
			GT_pointset_vertex_buffers *label_set = CREATE(GT_pointset_vertex_buffers)(font, g_NO_MARKER, 0.0);
			if (label_set)
			{
				graphics_object=CREATE(GT_object)(name, g_POINT_SET_VERTEX_BUFFERS, material);
				if (graphics_object)
				{
					unsigned int number_of_vertices = number_of_labels;
					struct Graphics_vertex_array *array = GT_object_get_vertex_set(
						graphics_object);
					fill_pointset_graphics_vertex_array(array,
						number_of_vertices,label_string_locations, label_strings, 0, 0);
					if (GT_OBJECT_ADD(GT_pointset_vertex_buffers)(graphics_object,label_set))
					{
						renderer->Graphics_object_render_immediate(graphics_object);
					}
					else
					{
						DESTROY(GT_pointset_vertex_buffers)(&label_set);
						return_code = 0;
					}
					DEACCESS(GT_object)(&graphics_object);
				}
				else
				{
					DESTROY(GT_pointset_vertex_buffers)(&label_set);
					return_code = 0;
				}
			}
			else
			{
				for (j = 0 ; j < number_of_labels ; j++)
				{
					DEALLOCATE(label_strings[j]);
				}
				DEALLOCATE(label_strings);
			}
			DEALLOCATE(label_string_locations);
			polyline =
				CREATE(GT_polyline_vertex_buffers)(g_PLAIN_DISCONTINUOUS, 0.0);
			if (polyline)
			{
				graphics_object=CREATE(GT_object)(name,g_POLYLINE_VERTEX_BUFFERS,secondary_material);
				if (graphics_object)
				{
					unsigned int number_of_vertices = number_of_minor_lines * 2;
					struct Graphics_vertex_array *array = GT_object_get_vertex_set(
						graphics_object);
					fill_line_graphics_vertex_array(array,
						number_of_vertices, minor_linepoints, 0, 0, 0);
					if (GT_OBJECT_ADD(GT_polyline_vertex_buffers)(graphics_object,polyline))
					{
						renderer->Graphics_object_render_immediate(graphics_object);
					}
					else
					{
						DESTROY(GT_polyline_vertex_buffers)(&polyline);
						return_code = 0;
					}
					DEACCESS(GT_object)(&graphics_object);
				}
				else
				{
					DESTROY(GT_polyline_vertex_buffers)(&polyline);
					return_code = 0;
				}
			}
			DEALLOCATE(minor_linepoints);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"draw_glyph_axes_general.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* draw_glyph_axes_general */

static int draw_glyph_axes_ticks(Triple *coordinate_scaling, 
	int label_bounds_dimension, int label_bounds_components, ZnReal *label_bounds,
	Triple *label_density,
	cmzn_material *material, cmzn_material *secondary_material,
	struct cmzn_font *font, Render_graphics *renderer)
/*******************************************************************************
LAST MODIFIED : 22 November 2005

DESCRIPTION :
Renders the label_bounds as a grid in the first two axis directions.  
An assumption is made that the label values have some alignment with the axis 
so only the first component of the label_bounds is drawn.
==============================================================================*/
{
	int return_code;

	ENTER(draw_glyph_axes_ticks);
	if ((label_bounds_dimension > 0) && (label_bounds_components > 0)
		&& label_bounds)
	{
		return_code = draw_glyph_axes_general(coordinate_scaling, 
			label_bounds_dimension, label_bounds_components, label_bounds,
			label_density,
			/*primary_axis_number*/0, /*label_bounds_component*/0,
			/*major_cross_min*/-0.05f, /*major_cross_max*/0.05f,
			/*minor_cross_min*/-0.01f, /*minor_cross_max*/0.01f,
			/*minor_grid_size*/0.01, /*minor_grids_per_major*/5,
			/*min_minor_grid*/0.01, /*min_major_grid*/0.1,
			material, secondary_material, font, renderer);
	}
	else
	{
		display_message(ERROR_MESSAGE,"draw_glyph_axes_ticks.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* draw_glyph_axes_ticks */

static int draw_glyph_grid_lines(Triple *coordinate_scaling, 
	int label_bounds_dimension, int label_bounds_components, ZnReal *label_bounds,
	Triple *label_density,
	cmzn_material *material, cmzn_material *secondary_material,
	struct cmzn_font *font, Render_graphics *renderer)
/*******************************************************************************
LAST MODIFIED : 22 November 2005

DESCRIPTION :
Renders the label_bounds as a grid in the first two axis directions.  
It is assumed that the label values have some alignment with the axes so only
the first component of the label_bounds is drawn along the first axis and the 
second component on the second axis.
==============================================================================*/
{
	int return_code;

	if ((label_bounds_dimension > 0) && (label_bounds_components > 0)
		&& label_bounds)
	{
		return_code = draw_glyph_axes_general(coordinate_scaling, 
			label_bounds_dimension, label_bounds_components, label_bounds,
			label_density,
			/*primary_axis_number*/0, /*label_bounds_component*/0,
			/*major_cross_min*/-0.01f, /*major_cross_max*/1.01f,
			/*minor_cross_min*/-0.01f, /*minor_cross_max*/1.01f,
			/*minor_grid_size*/0.01, /*minor_grids_per_major*/2,
			/*min_minor_grid*/0.1, /*min_major_grid*/0.2,
			material, secondary_material, font, renderer);
		return_code = draw_glyph_axes_general(coordinate_scaling, 
			label_bounds_dimension, label_bounds_components, label_bounds,
			label_density,
			/*primary_axis_number*/1, /*label_bounds_component*/1,
			/*major_cross_min*/-0.01f, /*major_cross_max*/1.01f,
			/*minor_cross_min*/-0.01f, /*minor_cross_max*/1.01f,
			/*minor_grid_size*/0.01, /*minor_grids_per_major*/2,
			/*min_minor_grid*/0.1, /*min_major_grid*/0.2,
			material, secondary_material, font, renderer);
	}
	else
	{
		display_message(ERROR_MESSAGE,"draw_glyph_grid_lines.  Invalid argument(s)");
		return_code=0;
	}

	return (return_code);
}

/*
Global functions
----------------
*/

PROTOTYPE_ENUMERATOR_STRING_FUNCTION(cmzn_glyph_repeat_mode)
{
	switch (enumerator_value)
	{
		case CMZN_GLYPH_REPEAT_MODE_NONE:
			return "REPEAT_MODE_NONE";
			break;
		case CMZN_GLYPH_REPEAT_MODE_AXES_2D:
			return "REPEAT_MODE_AXES_2D";
			break;
		case CMZN_GLYPH_REPEAT_MODE_AXES_3D:
			return "REPEAT_MODE_AXES_3D";
			break;
		case CMZN_GLYPH_REPEAT_MODE_MIRROR:
			return "REPEAT_MODE_MIRROR";
			break;
		default:
			// fall through to normal return
			break;
	}
	return 0;
}

DEFINE_DEFAULT_ENUMERATOR_FUNCTIONS(cmzn_glyph_repeat_mode)

int cmzn_glyph_repeat_mode_get_number_of_glyphs(
	enum cmzn_glyph_repeat_mode glyph_repeat_mode)
{
	switch (glyph_repeat_mode)
	{
		case CMZN_GLYPH_REPEAT_MODE_NONE:
			return 1;
			break;
		case CMZN_GLYPH_REPEAT_MODE_AXES_2D:
		case CMZN_GLYPH_REPEAT_MODE_MIRROR:
			return 2;
			break;
		case CMZN_GLYPH_REPEAT_MODE_AXES_3D:
			return 3;
			break;
		default:
			// fall through to normal return
			break;
	}
	return 0;
}

bool cmzn_glyph_repeat_mode_glyph_number_has_label(
	enum cmzn_glyph_repeat_mode glyph_repeat_mode, int glyph_number)
{
	switch (glyph_repeat_mode)
	{
		case CMZN_GLYPH_REPEAT_MODE_NONE:
		case CMZN_GLYPH_REPEAT_MODE_MIRROR:
			return (glyph_number == 0);
			break;
		case CMZN_GLYPH_REPEAT_MODE_AXES_2D:
			return (glyph_number < 2);
			break;
		case CMZN_GLYPH_REPEAT_MODE_AXES_3D:
			return (glyph_number < 3);
			break;
		default:
			// fall through to normal return
			break;
	}
	return false;
}

class cmzn_glyph_repeat_mode_conversion
{
public:
	static const char *to_string(enum cmzn_glyph_repeat_mode mode)
	{
		const char *enum_string = 0;
		switch (mode)
		{
			case CMZN_GLYPH_REPEAT_MODE_NONE:
				enum_string = "NONE";
				break;
			case CMZN_GLYPH_REPEAT_MODE_MIRROR:
				enum_string = "MIRROR";
				break;
			case CMZN_GLYPH_REPEAT_MODE_AXES_2D:
				enum_string = "AXES_2D";
				break;
			case CMZN_GLYPH_REPEAT_MODE_AXES_3D:
				enum_string = "AXES_3D";
				break;
		default:
			break;
		}
		return enum_string;
	}
};

enum cmzn_glyph_repeat_mode cmzn_glyph_repeat_mode_enum_from_string(const char *string)
{
	return string_to_enum<enum cmzn_glyph_repeat_mode, cmzn_glyph_repeat_mode_conversion>(string);
}

char *cmzn_glyph_repeat_mode_enum_to_string(enum cmzn_glyph_repeat_mode mode)
{
	const char *mode_string = cmzn_glyph_repeat_mode_conversion::to_string(mode);
	return (mode_string ? duplicate_string(mode_string) : 0);
}

class cmzn_glyph_shape_type_conversion
{
public:
	static const char *to_string(enum cmzn_glyph_shape_type type)
	{
		const char *enum_string = 0;
		switch (type)
		{
			case CMZN_GLYPH_REPEAT_MODE_NONE:
				enum_string = "NONE";
				break;
			case CMZN_GLYPH_SHAPE_TYPE_ARROW:
				enum_string = "ARROW";
				break;
			case CMZN_GLYPH_SHAPE_TYPE_ARROW_SOLID:
				enum_string = "ARROW_SOLID";
				break;
			case CMZN_GLYPH_SHAPE_TYPE_AXIS:
				enum_string = "AXIS";
				break;
			case CMZN_GLYPH_SHAPE_TYPE_AXIS_SOLID:
				enum_string = "AXIS_SOLID";
				break;
			case CMZN_GLYPH_SHAPE_TYPE_CONE:
				enum_string = "CONE";
				break;
			case CMZN_GLYPH_SHAPE_TYPE_CONE_SOLID:
				enum_string = "CONE_SOLID";
				break;
			case CMZN_GLYPH_SHAPE_TYPE_CROSS:
				enum_string = "CROSS";
				break;
			case CMZN_GLYPH_SHAPE_TYPE_CUBE_SOLID:
				enum_string = "CUBE_SOLID";
				break;
			case CMZN_GLYPH_SHAPE_TYPE_CUBE_WIREFRAME:
				enum_string = "CUBE_WIREFRAME";
				break;
			case CMZN_GLYPH_SHAPE_TYPE_CYLINDER:
				enum_string = "CYLINDER";
				break;
			case CMZN_GLYPH_SHAPE_TYPE_CYLINDER_SOLID:
				enum_string = "CYLINDER_SOLID";
				break;
			case CMZN_GLYPH_SHAPE_TYPE_DIAMOND:
				enum_string = "DIAMOND";
				break;
			case CMZN_GLYPH_SHAPE_TYPE_LINE:
				enum_string = "LINE";
				break;
			case CMZN_GLYPH_SHAPE_TYPE_POINT:
				enum_string = "POINT";
				break;
			case CMZN_GLYPH_SHAPE_TYPE_SHEET:
				enum_string = "SHEET";
				break;
			case CMZN_GLYPH_SHAPE_TYPE_SPHERE:
				enum_string = "SPHERE";
				break;
			case CMZN_GLYPH_SHAPE_TYPE_AXES:
				enum_string = "AXES";
				break;
			case CMZN_GLYPH_SHAPE_TYPE_AXES_123:
				enum_string = "AXES_123";
				break;
			case CMZN_GLYPH_SHAPE_TYPE_AXES_XYZ:
				enum_string = "AXES_XYZ";
				break;
			case CMZN_GLYPH_SHAPE_TYPE_AXES_COLOUR:
				enum_string = "AXES_COLOUR";
				break;
			case CMZN_GLYPH_SHAPE_TYPE_AXES_SOLID:
				enum_string = "AXES_SOLID";
				break;
			case CMZN_GLYPH_SHAPE_TYPE_AXES_SOLID_123:
				enum_string = "AXES_SOLID_123";
				break;
			case CMZN_GLYPH_SHAPE_TYPE_AXES_SOLID_XYZ:
				enum_string = "AXES_SOLID_XYZ";
				break;
			case CMZN_GLYPH_SHAPE_TYPE_AXES_SOLID_COLOUR:
				enum_string = "AXES_SOLID_COLOUR";
				break;
			default:
				break;
		}
		return enum_string;
	}
};

enum cmzn_glyph_shape_type cmzn_glyph_shape_type_enum_from_string(const char *string)
{
	return string_to_enum<enum cmzn_glyph_shape_type, cmzn_glyph_shape_type_conversion>(string);
}

char *cmzn_glyph_shape_type_enum_to_string(enum cmzn_glyph_shape_type type)
{
	const char *type_string = cmzn_glyph_shape_type_conversion::to_string(type);
	return (type_string ? duplicate_string(type_string) : 0);
}

void resolve_glyph_axes(
	enum cmzn_glyph_repeat_mode glyph_repeat_mode, int glyph_number,
	Triple base_size, Triple scale_factors, Triple offset,
	Triple point, Triple axis1, Triple axis2, Triple axis3, Triple scale,
	Triple final_point, Triple final_axis1, Triple final_axis2, Triple final_axis3)
{
	switch (glyph_repeat_mode)
	{
	case CMZN_GLYPH_REPEAT_MODE_NONE:
	case CMZN_GLYPH_REPEAT_MODE_MIRROR:
	default:
		{
			Triple axis_scale;
			for (int j = 0; j < 3; j++)
			{
				const FE_value sign = (scale[j] < 0.0) ? -1.0 : 1.0;
				axis_scale[j] = sign*base_size[j] + scale[j]*scale_factors[j];
			}
			for (int j = 0; j < 3; j++)
			{
				final_axis1[j] = axis1[j]*axis_scale[0];
				final_axis2[j] = axis2[j]*axis_scale[1];
				final_axis3[j] = axis3[j]*axis_scale[2];
				final_point[j] = point[j]
					+ offset[0]*final_axis1[j]
					+ offset[1]*final_axis2[j]
					+ offset[2]*final_axis3[j];
				if (glyph_repeat_mode == CMZN_GLYPH_REPEAT_MODE_MIRROR)
				{
					if (glyph_number == 1)
					{
						final_axis1[j] = -final_axis1[j];
						final_axis2[j] = -final_axis2[j];
						final_axis3[j] = -final_axis3[j];
					}
					if (scale[0] < 0.0)
					{
						// shift glyph origin to end of axis1 
						final_point[j] -= final_axis1[j];
					}
				}
			}
			/* if required, reverse axis3 to maintain right-handed coordinate system */
			if (0.0 > (
				final_axis3[0]*(final_axis1[1]*final_axis2[2] -
					final_axis1[2]*final_axis2[1]) +
				final_axis3[1]*(final_axis1[2]*final_axis2[0] -
					final_axis1[0]*final_axis2[2]) +
				final_axis3[2]*(final_axis1[0]*final_axis2[1] -
					final_axis1[1]*final_axis2[0])))
			{
				final_axis3[0] = -final_axis3[0];
				final_axis3[1] = -final_axis3[1];
				final_axis3[2] = -final_axis3[2];
			}
		} break;
	case CMZN_GLYPH_REPEAT_MODE_AXES_2D:
	case CMZN_GLYPH_REPEAT_MODE_AXES_3D:
		{
			Triple axis_scale;
			for (int j = 0; j < 3; j++)
			{
				const FE_value sign = (scale[j] < 0.0) ? -1.0 : 1.0;
				axis_scale[j] = sign*base_size[0] + scale[j]*scale_factors[0];
			}
			for (int j = 0; j < 3; j++)
			{
				final_point[j] = point[j]
					+ offset[0]*axis_scale[0]*axis1[j]
					+ offset[1]*axis_scale[1]*axis2[j]
					+ offset[2]*axis_scale[2]*axis3[j];
			}
			GLfloat *use_axis1, *use_axis2;
			const GLfloat use_scale = scale[glyph_number];
			if (glyph_number == 0)
			{
				use_axis1 = axis1;
				use_axis2 = axis2;
			}
			else if (glyph_number == 1)
			{
				use_axis1 = axis2;
				use_axis2 = (glyph_repeat_mode == CMZN_GLYPH_REPEAT_MODE_AXES_2D) ? axis1 : axis3;
			}
			else // if (glyph_number == 2)
			{
				use_axis1 = axis3;
				use_axis2 = axis1;
			}
			GLfloat final_scale1 = base_size[0] + use_scale*scale_factors[0];
			final_axis1[0] = use_axis1[0]*final_scale1;
			final_axis1[1] = use_axis1[1]*final_scale1;
			final_axis1[2] = use_axis1[2]*final_scale1;
			// final_axis3 = normalise(final_axis1 x use_axis2)*scale[0];
			final_axis3[0] = final_axis1[1]*use_axis2[2] - use_axis2[1]*final_axis1[2];
			final_axis3[1] = final_axis1[2]*use_axis2[0] - use_axis2[2]*final_axis1[0];
			final_axis3[2] = final_axis1[0]*use_axis2[1] - final_axis1[1]*use_axis2[0];
			GLfloat magnitude = sqrt(final_axis3[0]*final_axis3[0] + final_axis3[1]*final_axis3[1] + final_axis3[2]*final_axis3[2]);
			if (0.0 < magnitude)
			{
				GLfloat scaling = (base_size[2] + use_scale*scale_factors[2]) / magnitude;
				if ((glyph_repeat_mode == CMZN_GLYPH_REPEAT_MODE_AXES_2D) && (glyph_number > 0))
				{
					scaling *= -1.0;
				}
				final_axis3[0] *= scaling;
				final_axis3[1] *= scaling;
				final_axis3[2] *= scaling;
			}
			// final_axis2 = normalise(final_axis3 x final_axis1)*scale[0];
			final_axis2[0] = final_axis3[1]*final_axis1[2] - final_axis1[1]*final_axis3[2];
			final_axis2[1] = final_axis3[2]*final_axis1[0] - final_axis1[2]*final_axis3[0];
			final_axis2[2] = final_axis3[0]*final_axis1[1] - final_axis3[1]*final_axis1[0];
			magnitude = sqrt(final_axis2[0]*final_axis2[0] + final_axis2[1]*final_axis2[1] + final_axis2[2]*final_axis2[2]);
			if (0.0 < magnitude)
			{
				GLfloat scaling = (base_size[1] + use_scale*scale_factors[1]) / magnitude;
				final_axis2[0] *= scaling;
				final_axis2[1] *= scaling;
				final_axis2[2] *= scaling;
			}
		} break;
	}			
}

/**
 * Creates a graphics object named <name> consisting of a line from <0,0,0> to
 * <1,0,0> with 4 arrow head ticks <head_length> long and <half_head_width> out
 * from the shaft.
 */
struct GT_object *create_GT_object_arrow_line(const char *name,ZnReal head_length,
	ZnReal half_head_width)
{
	int j;
	struct GT_object *glyph = 0;
	Triple *points,*vertex;

	if (name)
	{
		unsigned int number_of_segments = 10;
		unsigned int vertex_start = 0;
		GT_polyline_vertex_buffers *lines =
			CREATE(GT_polyline_vertex_buffers)(
				g_PLAIN_DISCONTINUOUS, /*line_width=default*/0);
		glyph=CREATE(GT_object)(name,g_POLYLINE_VERTEX_BUFFERS,(cmzn_material *)NULL);
		GT_OBJECT_ADD(GT_polyline_vertex_buffers)(glyph, lines);
		if (ALLOCATE(points,Triple,10))
		{
			vertex=points;
			/* most coordinates are 0.0, so clear them all to that */
			for (j=0;j<10;j++)
			{
				(*vertex)[0]=0.0;
				(*vertex)[1]=0.0;
				(*vertex)[2]=0.0;
				vertex++;
			}
			/* x-axis */
			points[ 1][0]=1.0;
			points[ 2][0]=1.0;
			points[ 3][0]=1.0-head_length;
			points[ 3][1]=half_head_width;
			points[ 4][0]=1.0;
			points[ 5][0]=1.0-head_length;
			points[ 5][2]=half_head_width;
			points[ 6][0]=1.0;
			points[ 7][0]=1.0-head_length;
			points[ 7][1]=-half_head_width;
			points[ 8][0]=1.0;
			points[ 9][0]=1.0-head_length;
			points[ 9][2]=-half_head_width;
			struct Graphics_vertex_array *array = GT_object_get_vertex_set(glyph);
			GLfloat floatField[3];
			vertex=points;
			for (j=0;j<10;j++)
			{
				CAST_TO_OTHER(floatField,(*vertex),GLfloat,3);
				array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
					3, 1, floatField);
				vertex++;
			}
			array->add_unsigned_integer_attribute(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
				1, 1, &number_of_segments);
			array->add_unsigned_integer_attribute(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
				1, 1, &vertex_start);
			DEALLOCATE(points);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"create_GT_object_arrow_line.  Invalid argument(s)");
	}
	return (glyph);
}

/**
 * Creates a graphics object named <name> consisting of three axis arrows heading
 * from <0,0,0> to 1 in each of their directions. The arrows are made up of lines,
 * with a 4-way arrow head so it looks normal from the other two axes. If <labels>
 * is specified then it is assumed to point to an array of 3 strings which will
 * be used to label each arrow and are attached to it so
 * that the two objects are displayed and destroyed together. The labels are
 * located on the respective axes <label_offset> past 1.0.
 * The length and width of the arrow heads are specified by the final parameters.
 */
struct GT_object *create_GT_object_axes(const char *name, int make_solid, ZnReal head_length,
	ZnReal half_head_width,const char **labels, ZnReal label_offset,
	struct cmzn_font *font)
{
	char *glyph_name,**text;
	int j;
	struct Colour colour;
	cmzn_material *material;
	struct GT_object *glyph = 0, *labels_object, *last_object = 0;
	Triple *points,*vertex;

	if (name)
	{
		last_object = (struct GT_object *)NULL;
		if (make_solid)
		{
			if (ALLOCATE(glyph_name, char, strlen(name) + 8))
			{
				glyph = create_GT_object_arrow_solid(name, /*primary_axis*/1,
					/*number_of_segments_around*/12, /*shaft_length*/2.f/3.f,
					/*shaft_radius*/1.f/20.f, /*cone_radius*/1.f/8.f);
				material = cmzn_material_create_private();
				cmzn_material_set_name(material, "red");
				colour.red = 1;
				colour.green = 0;
				colour.blue = 0;
				Graphical_material_set_diffuse(material, &colour);
				set_GT_object_default_material(glyph, material);
				cmzn_material_destroy(&material);
				last_object = glyph;

				sprintf(glyph_name, "%s_arrow2", name);
				GT_object *arrow2 = create_GT_object_arrow_solid(glyph_name, /*primary_axis*/2,
					/*number_of_segments_around*/12, /*shaft_length*/2.f/3.f,
					/*shaft_radius*/1.f/20.f, /*cone_radius*/1.f/8.f);
				material = cmzn_material_create_private();
				cmzn_material_set_name(material, "green");
				colour.red = 0;
				colour.green = 1;
				colour.blue = 0;
				Graphical_material_set_diffuse(material, &colour);
				set_GT_object_default_material(arrow2, material);
				cmzn_material_destroy(&material);
				GT_object_set_next_object(last_object, arrow2);
				last_object = arrow2;
				DEACCESS(GT_object)(&arrow2);

				sprintf(glyph_name, "%s_arrow3", name);
				GT_object *arrow3 = create_GT_object_arrow_solid(glyph_name, /*primary_axis*/3,
					/*number_of_segments_around*/12, /*shaft_length*/2.f/3.f,
					/*shaft_radius*/1.f/20.f, /*cone_radius*/1.f/8.f);
				material = cmzn_material_create_private();
				cmzn_material_set_name(material, "blue");
				colour.red = 0;
				colour.green = 0;
				colour.blue = 1;
				Graphical_material_set_diffuse(material, &colour);
				set_GT_object_default_material(arrow3, material);
				cmzn_material_destroy(&material);
				GT_object_set_next_object(last_object, arrow3);
				last_object = arrow3;
				DEACCESS(GT_object)(&arrow3);
				
				DEALLOCATE(glyph_name);
			}
		}
		else
		{
			if (ALLOCATE(points,Triple,30))
			{
				vertex=points;
				/* most coordinates are 0.0, so clear them all to that */
				for (j=0;j<30;j++)
				{
					(*vertex)[0]=0.0;
					(*vertex)[1]=0.0;
					(*vertex)[2]=0.0;
					vertex++;
				}
				/* x-axis */
				points[ 1][0]=1.0;
				points[ 2][0]=1.0;
				points[ 3][0]=1.0-head_length;
				points[ 3][1]=half_head_width;
				points[ 4][0]=1.0;
				points[ 5][0]=1.0-head_length;
				points[ 5][2]=half_head_width;
				points[ 6][0]=1.0;
				points[ 7][0]=1.0-head_length;
				points[ 7][1]=-half_head_width;
				points[ 8][0]=1.0;
				points[ 9][0]=1.0-head_length;
				points[ 9][2]=-half_head_width;
				/* y-axis */
				points[11][1]=1.0;
				points[12][1]=1.0;
				points[13][1]=1.0-head_length;
				points[13][2]=half_head_width;
				points[14][1]=1.0;
				points[15][1]=1.0-head_length;
				points[15][0]=half_head_width;
				points[16][1]=1.0;
				points[17][1]=1.0-head_length;
				points[17][2]=-half_head_width;
				points[18][1]=1.0;
				points[19][1]=1.0-head_length;
				points[19][0]=-half_head_width;
				/* z-axis */
				points[21][2]=1.0;
				points[22][2]=1.0;
				points[23][2]=1.0-head_length;
				points[23][0]=half_head_width;
				points[24][2]=1.0;
				points[25][2]=1.0-head_length;
				points[25][1]=half_head_width;
				points[26][2]=1.0;
				points[27][2]=1.0-head_length;
				points[27][0]=-half_head_width;
				points[28][2]=1.0;
				points[29][2]=1.0-head_length;
				points[29][1]=-half_head_width;
				GT_polyline_vertex_buffers *lines = CREATE(GT_polyline_vertex_buffers)(
					g_PLAIN_DISCONTINUOUS, /*line_width=default*/0);
				glyph=CREATE(GT_object)(name,g_POLYLINE_VERTEX_BUFFERS, (cmzn_material *)NULL);
				if (glyph)
				{
					GT_OBJECT_ADD(GT_polyline_vertex_buffers)(glyph,lines);
					last_object = glyph;
				}
				vertex=points;
				GLfloat floatField[3];
				unsigned int number_of_segments = 30, vertex_start = 0;
				struct Graphics_vertex_array *array = GT_object_get_vertex_set(glyph);
				for (j=0;j<30;j++)
				{
					CAST_TO_OTHER(floatField,(*vertex),GLfloat,3);
					array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
						3, 1, floatField);
					vertex++;
				}
				array->add_unsigned_integer_attribute(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
					1, 1, &number_of_segments);
				array->add_unsigned_integer_attribute(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
					1, 1, &vertex_start);
				DEALLOCATE(points);
			}
		}
		if (glyph && labels)
		{
			if (ALLOCATE(points,Triple,3)&&
				ALLOCATE(text,char *,3)&&
				ALLOCATE(text[0],char,strlen(labels[0]) + 1)&&
				ALLOCATE(text[1],char,strlen(labels[1]) + 1)&&
				ALLOCATE(text[2],char,strlen(labels[2]) + 1)&&
				ALLOCATE(glyph_name,char,strlen(name)+8))
			{
				sprintf(glyph_name,"%s_labels",name);
				points[0][0]=1.0+label_offset;
				points[0][1]=0.0;
				points[0][2]=0.0;
				strcpy(text[0],labels[0]);
				points[1][0]=0.0;
				points[1][1]=1.0+label_offset;
				points[1][2]=0.0;
				strcpy(text[1],labels[1]);
				points[2][0]=0.0;
				points[2][1]=0.0;
				points[2][2]=1.0+label_offset;
				strcpy(text[2],labels[2]);
				GT_pointset_vertex_buffers *pointsets = CREATE(GT_pointset_vertex_buffers)(
					font, g_NO_MARKER, 0.0);
				labels_object=CREATE(GT_object)(glyph_name,g_POINT_SET_VERTEX_BUFFERS,
					(cmzn_material *)NULL);
				if (labels_object)
				{
					GT_OBJECT_ADD(GT_pointset_vertex_buffers)(labels_object,pointsets);
					GT_object_set_next_object(last_object, labels_object);
					last_object = labels_object;
				}
				struct Graphics_vertex_array *array = GT_object_get_vertex_set(last_object);
				fill_pointset_graphics_vertex_array(array,
					3,points, text, 0, 0);
				DEALLOCATE(text[0]);
				DEALLOCATE(text[1]);
				DEALLOCATE(text[2]);
				DEALLOCATE(text);
				DEALLOCATE(points);
			}
		}
		if (!glyph)
		{
			display_message(ERROR_MESSAGE,"create_GT_object_axes.  Error creating glyph");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"create_GT_object_axes.  Invalid argument(s)");
	}

	return (glyph);
}

/**
 * Creates a graphics object named <name> consisting of a 3 lines:
 * from <-0.5,0,0> to <+0.5,0,0>
 * from <0,-0.5,0> to <0,+0.5,0>
 * from <0,0,-0.5> to <0,0,+0.5>
 */
struct GT_object *create_GT_object_cross(const char *name)
{
	struct GT_object *glyph = 0;
	Triple *points;

	if (name)
	{
		if (ALLOCATE(points,Triple,6))
		{
			/* x-line */
			points[0][0]=-0.5;
			points[0][1]=0.0;
			points[0][2]=0.0;
			points[1][0]=+0.5;
			points[1][1]=0.0;
			points[1][2]=0.0;
			/* y-line */
			points[2][0]=0.0;
			points[2][1]=-0.5;
			points[2][2]=0.0;
			points[3][0]=0.0;
			points[3][1]=+0.5;
			points[3][2]=0.0;
			/* z-line */
			points[4][0]=0.0;
			points[4][1]=0.0;
			points[4][2]=-0.5;
			points[5][0]=0.0;
			points[5][1]=0.0;
			points[5][2]=+0.5;
			GT_polyline_vertex_buffers *lines = CREATE(GT_polyline_vertex_buffers)(
				g_PLAIN_DISCONTINUOUS, /*line_width=default*/0);
			glyph=CREATE(GT_object)(name,g_POLYLINE_VERTEX_BUFFERS, (cmzn_material *)NULL);
			if (glyph)
			{
				GT_OBJECT_ADD(GT_polyline_vertex_buffers)(glyph,lines);
			}
			Triple *vertex=points;
			GLfloat floatField[3];
			unsigned int number_of_segments = 6, vertex_start = 0;
			struct Graphics_vertex_array *array = GT_object_get_vertex_set(glyph);
			for (int j=0;j<6;j++)
			{
				CAST_TO_OTHER(floatField,(*vertex),GLfloat,3);
				array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
					3, 1, floatField);
				vertex++;
			}
			array->add_unsigned_integer_attribute(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
				1, 1, &number_of_segments);
			array->add_unsigned_integer_attribute(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
				1, 1, &vertex_start);
			DEALLOCATE(points);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"create_GT_object_cross.  Invalid argument(s)");
	}

	return (glyph);
}

/**
 * Creates a graphics object named <name> consisting of a unit-sized GT_surface
 * cube centred at <0,0,0>.
 */
struct GT_object *create_GT_object_cube_solid(const char *name)
{
	ZnReal factor;
	int a, b, c, i;
	struct GT_object *glyph = 0;
	Triple *point, *points, *normalpoint, *normalpoints;

	if (name)
	{
		if (ALLOCATE(points,Triple,36)&&
			ALLOCATE(normalpoints,Triple,36))
		{
			point = points;
			normalpoint = normalpoints;
			/* all coordinates are +0.5 or -0.5, so clear them all to former */
			for (i = 0; i < 6; i++)
			{
				a = i / 2;
				if ((2*a) == i)
				{
					factor = -1.0;
				}
				else
				{
					factor = 1.0;
				}
				b = (a + 1) % 3;
				c = (a + 2) % 3;
				/* vertices
				 *  4-3   1-3-4 3-1-2
				 *  |/|
				 *  1-2
				 * */
				(*point)[a] = 0.5*factor;
				(*point)[b] = 0.5*factor;
				(*point)[c] = 0.5;
				point++;
				(*point)[a] = 0.5*factor;
				(*point)[b] = -0.5*factor;
				(*point)[c] = -0.5;
				point++;
				(*point)[a] = 0.5*factor;
				(*point)[b] = 0.5*factor;
				(*point)[c] = -0.5;
				point++;
				(*point)[a] = 0.5*factor;
				(*point)[b] = -0.5*factor;
				(*point)[c] = -0.5;
				point++;
				(*point)[a] = 0.5*factor;
				(*point)[b] = 0.5*factor;
				(*point)[c] = 0.5;
				point++;
				(*point)[a] = 0.5*factor;
				(*point)[b] = -0.5*factor;
				(*point)[c] = 0.5;
				point++;
				/* normals */
				(*normalpoint)[a] = factor;
				(*normalpoint)[b] = 0.0;
				(*normalpoint)[c] = 0.0;
				normalpoint++;
				(*normalpoint)[a] = factor;
				(*normalpoint)[b] = 0.0;
				(*normalpoint)[c] = 0.0;
				normalpoint++;
				(*normalpoint)[a] = factor;
				(*normalpoint)[b] = 0.0;
				(*normalpoint)[c] = 0.0;
				normalpoint++;
				(*normalpoint)[a] = factor;
				(*normalpoint)[b] = 0.0;
				(*normalpoint)[c] = 0.0;
				normalpoint++;
				(*normalpoint)[a] = factor;
				(*normalpoint)[b] = 0.0;
				(*normalpoint)[c] = 0.0;
				normalpoint++;
				(*normalpoint)[a] = factor;
				(*normalpoint)[b] = 0.0;
				(*normalpoint)[c] = 0.0;
				normalpoint++;
			}
			point = points;
			normalpoint = normalpoints;
			int polygonType = (int)g_TRIANGLE;
			unsigned int number_of_xi1 = 6, number_of_xi2 = 6, number_of_vertices = 36,
				vertex_start = 0;
			GLfloat floatField[3];
			glyph=CREATE(GT_object)(name,g_SURFACE_VERTEX_BUFFERS,(cmzn_material *)NULL);
			GT_surface_vertex_buffers *surfaces = CREATE(GT_surface_vertex_buffers)(
				g_SH_DISCONTINUOUS, CMZN_GRAPHICS_RENDER_POLYGON_MODE_SHADED);
			GT_OBJECT_ADD(GT_surface_vertex_buffers)(glyph, surfaces);
			struct Graphics_vertex_array *array = GT_object_get_vertex_set(glyph);
			for (unsigned int j=0;j<36;j++)
			{
				CAST_TO_OTHER(floatField,(*point),GLfloat,3);
				array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
					3, 1, floatField);
				point++;
				CAST_TO_OTHER(floatField,(*normalpoint),GLfloat,3);
				array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
					3, 1, floatField);
				normalpoint++;
			}
			array->add_unsigned_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_XI1,
				1, 1, &number_of_xi1);
			array->add_unsigned_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_XI2,
				1, 1, &number_of_xi2);
			array->add_unsigned_integer_attribute(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
				1, 1, &number_of_vertices);
			array->add_unsigned_integer_attribute(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
				1, 1, &vertex_start);
			array->add_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POLYGON,
				1, 1, &polygonType);
			DEALLOCATE(points);
			DEALLOCATE(normalpoints);
		}
		if (!glyph)
		{
			display_message(ERROR_MESSAGE,
				"create_GT_object_cube_solid.  Error creating glyph");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"create_GT_object_cube_solid.  Invalid argument(s)");
	}

	return (glyph);
}

/**
 * Creates a graphics object named <name> consisting of lines marking a
 * unit-sized wireframe cube centred at <0,0,0>.
 */
struct GT_object *create_GT_object_cube_wireframe(const char *name)
{
	int a, b, c, i;
	struct GT_object *glyph = 0;
	Triple *points, *vertex;

	ENTER(create_GT_object_cube_wireframe);
	if (name)
	{
		if (ALLOCATE(points, Triple, 24))
		{
			vertex=points;
			/* all coordinates are +0.5 or -0.5, so clear them all to former */
			for (a = 0; a < 3; a++)
			{
				b = (a + 1) % 3;
				c = (a + 2) % 3;
				for (i = 0; i < 8; i++)
				{
					if (0 == (i % 2))
					{
						(*vertex)[a] = -0.5;
					}
					else
					{
						(*vertex)[a] = 0.5;
					}
					if (2 > (i % 4))
					{
						(*vertex)[b] = -0.5;
					}
					else
					{
						(*vertex)[b] = 0.5;
					}
					if (4 > i)
					{
						(*vertex)[c] = -0.5;
					}
					else
					{
						(*vertex)[c] = 0.5;
					}
					vertex++;
				}
			}
			GT_polyline_vertex_buffers *lines = CREATE(GT_polyline_vertex_buffers)(
				g_PLAIN_DISCONTINUOUS, /*line_width=default*/0);
			glyph=CREATE(GT_object)(name,g_POLYLINE_VERTEX_BUFFERS, (cmzn_material *)NULL);
			if (glyph)
			{
				GT_OBJECT_ADD(GT_polyline_vertex_buffers)(glyph,lines);
			}
			vertex=points;
			GLfloat floatField[3];
			unsigned int number_of_segments = 24, vertex_start = 0;
			struct Graphics_vertex_array *array = GT_object_get_vertex_set(glyph);
			for (int j=0;j<24;j++)
			{
				CAST_TO_OTHER(floatField,(*vertex),GLfloat,3);
				array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
					3, 1, floatField);
				vertex++;
			}
			array->add_unsigned_integer_attribute(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
				1, 1, &number_of_segments);
			array->add_unsigned_integer_attribute(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
				1, 1, &vertex_start);
			DEALLOCATE(points);
		}
		if (!glyph)
		{
			display_message(ERROR_MESSAGE,
				"create_GT_object_cube_wireframe.  Error creating glyph");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"create_GT_object_cube_wireframe.  Invalid argument(s)");
	}

	return (glyph);
}

/**
 * Creates a graphics object named <name> consisting of a line from <0,0,0> to
 * <1,0,0>.
 */
struct GT_object *create_GT_object_line(const char *name)
{
	struct GT_object *glyph = 0;
	Triple *points;

	if (name)
	{
		if (ALLOCATE(points,Triple,2))
		{
			points[0][0]=0.0;
			points[0][1]=0.0;
			points[0][2]=0.0;
			points[1][0]=1.0;
			points[1][1]=0.0;
			points[1][2]=0.0;
			GT_polyline_vertex_buffers *lines = CREATE(GT_polyline_vertex_buffers)(
				g_PLAIN_DISCONTINUOUS, /*line_width=default*/0);
			glyph=CREATE(GT_object)(name,g_POLYLINE_VERTEX_BUFFERS, (cmzn_material *)NULL);
			if (glyph)
			{
				GT_OBJECT_ADD(GT_polyline_vertex_buffers)(glyph,lines);
			}
			Triple *vertex=points;
			GLfloat floatField[3];
			unsigned int number_of_segments = 2, vertex_start = 0;
			struct Graphics_vertex_array *array = GT_object_get_vertex_set(glyph);
			for (int j=0;j<2;j++)
			{
				CAST_TO_OTHER(floatField,(*vertex),GLfloat,3);
				array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
					3, 1, floatField);
				vertex++;
			}
			array->add_unsigned_integer_attribute(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
				1, 1, &number_of_segments);
			array->add_unsigned_integer_attribute(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
				1, 1, &vertex_start);
			DEALLOCATE(points);
		}
		if (!glyph)
		{
			display_message(ERROR_MESSAGE,"create_GT_object_line.  Error creating glyph");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"create_GT_object_line.  Invalid argument(s)");
	}

	return (glyph);
}

/**
 * Creates a graphics object named <name> consisting of a single point at
 * <0,0,0>.
 * The point will be drawn with the given <marker_type> and <marker_size>.
 */
struct GT_object *create_GT_object_point(const char *name,gtMarkerType marker_type,
	ZnReal marker_size)
{
	struct GT_object *glyph = 0;
	Triple *points;

	if (name)
	{
		if (ALLOCATE(points,Triple,1))
		{
			(*points)[0]=0.0;
			(*points)[1]=0.0;
			(*points)[2]=0.0;
			glyph=CREATE(GT_object)(name,g_POINT_SET_VERTEX_BUFFERS,
				(cmzn_material *)NULL);
			GT_pointset_vertex_buffers *pointsets = CREATE(GT_pointset_vertex_buffers)(
				NULL, marker_type, marker_size);
			GT_OBJECT_ADD(GT_pointset_vertex_buffers)(glyph,pointsets);
			struct Graphics_vertex_array *array = GT_object_get_vertex_set(glyph);
			fill_pointset_graphics_vertex_array(array, 1, points, 0, 0, 0);
			DEALLOCATE(points);
		}
		if (!glyph)
		{
			display_message(ERROR_MESSAGE,"create_GT_object_point.  Error creating glyph");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"create_GT_object_point.  Invalid argument(s)");
	}

	return (glyph);
}

/**
 * Creates a graphics object named <name> resembling a square sheet spanning from
 * coordinate <-0.5,-0.5,0> to <0.5,0.5,0>.
 * If define_texturepoints is true then texture coordinates will also be defined
 * ranging from <0.0,0.0,0.0> to <1.0,1.0,0.0>.
 */
struct GT_object *create_GT_object_sheet(const char *name, int define_texturepoints)
{
	struct GT_object *glyph = 0;
	Triple *point,*points,*normalpoints, *texturepoints;

	if (name)
	{
		texturepoints = (Triple *)NULL;
		if (ALLOCATE(points,Triple,6)&&
			ALLOCATE(normalpoints,Triple,6)&&
			(!define_texturepoints || ALLOCATE(texturepoints,Triple,6)))
		{
			point = points;
			/* vertices */
			(*point)[0] = -0.5;
			(*point)[1] = -0.5;
			(*point)[2] = 0.0;
			point++;
			(*point)[0] = 0.5;
			(*point)[1] = 0.5;
			(*point)[2] = 0.0;
			point++;
			(*point)[0] = -0.5;
			(*point)[1] = 0.5;
			(*point)[2] = 0.0;
			point++;
			(*point)[0] = 0.5;
			(*point)[1] = 0.5;
			(*point)[2] = 0.0;
			point++;
			(*point)[0] = -0.5;
			(*point)[1] = -0.5;
			(*point)[2] = 0.0;
			point++;
			(*point)[0] = 0.5;
			(*point)[1] = -0.5;
			(*point)[2] = 0.0;
			/* normals */
			point = normalpoints;
			(*point)[0] = 0.0;
			(*point)[1] = 0.0;
			(*point)[2] = 1.0;
			point++;
			(*point)[0] = 0.0;
			(*point)[1] = 0.0;
			(*point)[2] = 1.0;
			point++;
			(*point)[0] = 0.0;
			(*point)[1] = 0.0;
			(*point)[2] = 1.0;
			point++;
			(*point)[0] = 0.0;
			(*point)[1] = 0.0;
			(*point)[2] = 1.0;
			point++;
			(*point)[0] = 0.0;
			(*point)[1] = 0.0;
			(*point)[2] = 1.0;
			point++;
			(*point)[0] = 0.0;
			(*point)[1] = 0.0;
			(*point)[2] = 1.0;
			/* texture coordinates */
			if (define_texturepoints)
			{
				point = texturepoints;
				(*point)[0] = 0.0;
				(*point)[1] = 0.0;
				(*point)[2] = 0.0;
				point++;
				(*point)[0] = 1.0;
				(*point)[1] = 1.0;
				(*point)[2] = 0.0;
				point++;
				(*point)[0] = 0.0;
				(*point)[1] = 1.0;
				(*point)[2] = 0.0;
				point++;
				(*point)[0] = 1.0;
				(*point)[1] = 1.0;
				(*point)[2] = 0.0;
				point++;
				(*point)[0] = 0.0;
				(*point)[1] = 0.0;
				(*point)[2] = 0.0;
				point++;
				(*point)[0] = 1.0;
				(*point)[1] = 0.0;
				(*point)[2] = 0.0;
			}
			unsigned int number_of_vertices = 6, number_of_xi1 = 2, number_of_xi2 = 3,
				vertex_start = 0;
			int polygonType = (int)g_TRIANGLE;
			Triple *vertex=points, *normal=normalpoints, *texpoints = texturepoints;
			GLfloat floatField[3];
			glyph=CREATE(GT_object)(name,g_SURFACE_VERTEX_BUFFERS,(cmzn_material *)NULL);
			GT_surface_vertex_buffers *surfaces = CREATE(GT_surface_vertex_buffers)(
				g_SH_DISCONTINUOUS_TEXMAP, CMZN_GRAPHICS_RENDER_POLYGON_MODE_SHADED);
			GT_OBJECT_ADD(GT_surface_vertex_buffers)(glyph, surfaces);
			struct Graphics_vertex_array *array = GT_object_get_vertex_set(glyph);
			for (unsigned int j=0;j<number_of_vertices;j++)
			{
				CAST_TO_OTHER(floatField,(*vertex),GLfloat,3);
				array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
					3, 1, floatField);
				vertex++;
				CAST_TO_OTHER(floatField,(*normal),GLfloat,3);
				array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
					3, 1, floatField);
				normal++;
				if (define_texturepoints)
				{
					CAST_TO_OTHER(floatField,(*texpoints),GLfloat,3);
					array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_TEXTURE_COORDINATE_ZERO,
						3, 1, floatField);
					texpoints++;
				}
			}
			array->add_unsigned_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_XI1,
				1, 1, &number_of_xi1);
			array->add_unsigned_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_XI2,
				1, 1, &number_of_xi2);
			array->add_unsigned_integer_attribute(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
				1, 1, &number_of_vertices);
			array->add_unsigned_integer_attribute(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
				1, 1, &vertex_start);
			array->add_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POLYGON,
				1, 1, &polygonType);
			DEALLOCATE(points);
			DEALLOCATE(normalpoints);
			if (define_texturepoints)
				DEALLOCATE(texturepoints);
		}
		if (!glyph)
		{
			display_message(ERROR_MESSAGE,
				"create_GT_object_sheet.  Error creating glyph");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"create_GT_object_sheet.  Invalid argument(s)");
	}

	return (glyph);
}

cmzn_glyph_id cmzn_glyph_access(cmzn_glyph_id glyph)
{
	if (glyph)
		glyph->access();
	return glyph;
}

int cmzn_glyph_destroy(cmzn_glyph_id *glyph_address)
{
	if (glyph_address)
	{
		cmzn_glyph::deaccess(glyph_address);
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

char *cmzn_glyph_get_name(cmzn_glyph_id glyph)
{
	if (glyph)
		return duplicate_string(glyph->getName());
	return 0;
}

int cmzn_glyph_set_name(cmzn_glyph_id glyph, const char *name)
{
	if (glyph)
		return glyph->setName(name);
	return CMZN_ERROR_ARGUMENT;
}

bool cmzn_glyph_is_managed(cmzn_glyph_id glyph)
{
	if (glyph)
		return glyph->isManaged();
	return 0;
}

int cmzn_glyph_set_managed(cmzn_glyph_id glyph, bool value)
{
	if (glyph)
	{
		glyph->setManaged(value);
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_glyphmodulenotifier_id cmzn_glyphmodule_create_glyphmodulenotifier(
	cmzn_glyphmodule_id glyphmodule)
{
	return cmzn_glyphmodulenotifier::create(glyphmodule);
}

cmzn_glyphmodulenotifier::cmzn_glyphmodulenotifier(
	cmzn_glyphmodule *glyphmodule) :
	module(glyphmodule),
	function(0),
	user_data(0),
	access_count(1)
{
	glyphmodule->addNotifier(this);
}

cmzn_glyphmodulenotifier::~cmzn_glyphmodulenotifier()
{
}

int cmzn_glyphmodulenotifier::deaccess(cmzn_glyphmodulenotifier* &notifier)
{
	if (notifier)
	{
		--(notifier->access_count);
		if (notifier->access_count <= 0)
			delete notifier;
		else if ((1 == notifier->access_count) && notifier->module)
			notifier->module->removeNotifier(notifier);
		notifier = 0;
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_glyphmodulenotifier::setCallback(cmzn_glyphmodulenotifier_callback_function function_in,
	void *user_data_in)
{
	if (!function_in)
		return CMZN_ERROR_ARGUMENT;
	this->function = function_in;
	this->user_data = user_data_in;
	return CMZN_OK;
}

void cmzn_glyphmodulenotifier::clearCallback()
{
	this->function = 0;
	this->user_data = 0;
}

void cmzn_glyphmodulenotifier::glyphmoduleDestroyed()
{
	this->module = 0;
	if (this->function)
	{
		cmzn_glyphmoduleevent_id event = cmzn_glyphmoduleevent::create(
			static_cast<cmzn_glyphmodule*>(0));
		event->setChangeFlags(CMZN_SPECTRUM_CHANGE_FLAG_FINAL);
		(this->function)(event, this->user_data);
		cmzn_glyphmoduleevent::deaccess(event);
		this->clearCallback();
	}
}

cmzn_glyphmoduleevent::cmzn_glyphmoduleevent(
	cmzn_glyphmodule *glyphmoduleIn) :
	module(cmzn_glyphmodule_access(glyphmoduleIn)),
	changeFlags(CMZN_TESSELLATION_CHANGE_FLAG_NONE),
	managerMessage(0),
	access_count(1)
{
}

cmzn_glyphmoduleevent::~cmzn_glyphmoduleevent()
{
	if (managerMessage)
		MANAGER_MESSAGE_DEACCESS(cmzn_glyph)(&(this->managerMessage));
	cmzn_glyphmodule_destroy(&this->module);
}

cmzn_glyph_change_flags cmzn_glyphmoduleevent::getGlyphChangeFlags(
	cmzn_glyph *glyph) const
{
	if (glyph && this->managerMessage)
		return MANAGER_MESSAGE_GET_OBJECT_CHANGE(cmzn_glyph)(this->managerMessage, glyph);
	return CMZN_TESSELLATION_CHANGE_FLAG_NONE;
}

void cmzn_glyphmoduleevent::setManagerMessage(
	struct MANAGER_MESSAGE(cmzn_glyph) *managerMessageIn)
{
	this->managerMessage = MANAGER_MESSAGE_ACCESS(cmzn_glyph)(managerMessageIn);
}

struct MANAGER_MESSAGE(cmzn_glyph) *cmzn_glyphmoduleevent::getManagerMessage()
{
	return this->managerMessage;
}

int cmzn_glyphmoduleevent::deaccess(cmzn_glyphmoduleevent* &event)
{
	if (event)
	{
		--(event->access_count);
		if (event->access_count <= 0)
			delete event;
		event = 0;
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_glyphmodulenotifier_clear_callback(
	cmzn_glyphmodulenotifier_id notifier)
{
	if (notifier)
	{
		notifier->clearCallback();
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_glyphmodulenotifier_set_callback(cmzn_glyphmodulenotifier_id notifier,
	cmzn_glyphmodulenotifier_callback_function function_in, void *user_data_in)
{
	if (notifier && function_in)
		return notifier->setCallback(function_in, user_data_in);
	return CMZN_ERROR_ARGUMENT;
}

void *cmzn_glyphmodulenotifier_get_callback_user_data(
 cmzn_glyphmodulenotifier_id notifier)
{
	if (notifier)
		return notifier->getUserData();
	return 0;
}

cmzn_glyphmodulenotifier_id cmzn_glyphmodulenotifier_access(
	cmzn_glyphmodulenotifier_id notifier)
{
	if (notifier)
		return notifier->access();
	return 0;
}

int cmzn_glyphmodulenotifier_destroy(cmzn_glyphmodulenotifier_id *notifier_address)
{
	return cmzn_glyphmodulenotifier::deaccess(*notifier_address);
}

cmzn_glyphmoduleevent_id cmzn_glyphmoduleevent_access(
	cmzn_glyphmoduleevent_id event)
{
	if (event)
		return event->access();
	return 0;
}

int cmzn_glyphmoduleevent_destroy(cmzn_glyphmoduleevent_id *event_address)
{
	return cmzn_glyphmoduleevent::deaccess(*event_address);
}

cmzn_glyph_change_flags cmzn_glyphmoduleevent_get_summary_glyph_change_flags(
	cmzn_glyphmoduleevent_id event)
{
	if (event)
		return event->getChangeFlags();
	return CMZN_TESSELLATION_CHANGE_FLAG_NONE;
}

cmzn_glyph_change_flags cmzn_glyphmoduleevent_get_glyph_change_flags(
	cmzn_glyphmoduleevent_id event, cmzn_glyph_id glyph)
{
	if (event)
		return event->getGlyphChangeFlags(glyph);
	return CMZN_TESSELLATION_CHANGE_FLAG_NONE;
}

cmzn_glyphmodule::cmzn_glyphmodule(cmzn_materialmodule *materialModuleIn) :
	materialModule(cmzn_materialmodule_access(materialModuleIn)),
	manager(CREATE(MANAGER(cmzn_glyph))()),
	manager_callback_id(0),
	defaultPointGlyph(0),
	access_count(1)
{
	this->manager_callback_id = MANAGER_REGISTER(cmzn_glyph)(
		cmzn_glyphmodule::glyph_manager_change, (void *)this, this->manager);
}

cmzn_glyphmodule::~cmzn_glyphmodule()
{
	cmzn_materialmodule_destroy(&materialModule);
	if (defaultPointGlyph)
	{
		DEACCESS(cmzn_glyph)(&(this->defaultPointGlyph));
	}
	MANAGER_DEREGISTER(cmzn_glyph)(this->manager_callback_id, this->manager);
	for (cmzn_glyphmodulenotifier_list::iterator iter = this->notifier_list.begin();
		iter != this->notifier_list.end(); ++iter)
	{
		cmzn_glyphmodulenotifier *notifier = *iter;
		notifier->glyphmoduleDestroyed();
		cmzn_glyphmodulenotifier::deaccess(notifier);
	}
	DESTROY(MANAGER(cmzn_glyph))(&(this->manager));
}

/**
 * Glyph manager callback. Calls notifier callbacks.
 *
 * @param message  The changes to the glyphs in the glyph manager.
 * @param glyphmodule_void  Void pointer to changed glyphmodule).
 */
void cmzn_glyphmodule::glyph_manager_change(
	struct MANAGER_MESSAGE(cmzn_glyph) *message, void *glyphmodule_void)
{
	cmzn_glyphmodule *glyphmodule = (cmzn_glyphmodule *)glyphmodule_void;
	if (message && glyphmodule)
	{
		int change_summary = MANAGER_MESSAGE_GET_CHANGE_SUMMARY(cmzn_glyph)(message);

		if (0 < glyphmodule->notifier_list.size())
		{
			cmzn_glyphmoduleevent_id event = cmzn_glyphmoduleevent::create(glyphmodule);
			event->setChangeFlags(change_summary);
			event->setManagerMessage(message);
			for (cmzn_glyphmodulenotifier_list::iterator iter =
				glyphmodule->notifier_list.begin();
				iter != glyphmodule->notifier_list.end(); ++iter)
			{
				(*iter)->notify(event);
			}
			cmzn_glyphmoduleevent::deaccess(event);
		}
	}
}

cmzn_glyphiterator *cmzn_glyphmodule::createGlyphiterator()
{
	return CREATE_LIST_ITERATOR(cmzn_glyph)(this->manager->object_list);
}

/**
  * If name is not already used by an existing glyph, set name, managed flag
	* and add to glyph module.
	* Always free handle to glyph.
	*/
void cmzn_glyphmodule::defineGlyph(const char *name, cmzn_glyph *glyph, cmzn_glyph_shape_type type)
{
	if (0 == FIND_BY_IDENTIFIER_IN_MANAGER(cmzn_glyph,name)(name, this->getManager()))
	{
		glyph->setName(name);
		glyph->setManaged(true);
		glyph->setType(type);
		this->addGlyph(glyph);
	}
	cmzn_glyph_destroy(&glyph);
}

/**
 * If there is no glyph with the same name as graphicsObject, create a
 * static glyph of that name wrapping it and manage it.
 * Always deaccesses the supplied graphics object to clean it up whether
 * wrapping it or not.
 * @return  true if glyph added, false if not.
 */
bool cmzn_glyphmodule::defineGlyphStatic(GT_object*& graphicsObject, cmzn_glyph_shape_type type)
{
	bool glyphAdded = true;
	char *name = 0;
	GET_NAME(GT_object)(graphicsObject, &name);
	if (0 == FIND_BY_IDENTIFIER_IN_MANAGER(cmzn_glyph,name)(name, this->getManager()))
	{
		GT_object_set_glyph_type(graphicsObject, type);
		cmzn_glyph *glyph = cmzn_glyph_static::create(graphicsObject);
		glyph->setType(type);
		glyph->setName(name);
		glyph->setManaged(true);
		this->addGlyph(glyph);
		cmzn_glyph::deaccess(&glyph);
	}
	else
	{
		glyphAdded = false;
	}
	DEACCESS(GT_object)(&graphicsObject);
	DEALLOCATE(name);
	return glyphAdded;
}

cmzn_glyph *cmzn_glyphmodule::findGlyphByType(enum cmzn_glyph_shape_type glyph_type)
{
	cmzn_set_cmzn_glyph *glyphs = this->getGlyphListPrivate();
	for (cmzn_set_cmzn_glyph::iterator iter = glyphs->begin(); iter != glyphs->end(); ++iter)
	{
		if ((*iter)->getType() == glyph_type)
		{
			return *iter;
		}
	}
	return 0;
}

void cmzn_glyphmodule::addGlyph(cmzn_glyph *glyph)
{
	if (glyph->manager)
	{
		display_message(ERROR_MESSAGE, "cmzn_glyphmodule::addGlyph.  Glyph already managed");
		return;
	}
	if ((0 == glyph->getName()) || (static_cast<cmzn_glyph*>(0) != 
		FIND_BY_IDENTIFIER_IN_MANAGER(cmzn_glyph,name)(glyph->getName(), this->manager)))
	{
		char tempName[20];
		int i = NUMBER_IN_MANAGER(cmzn_glyph)(this->manager);
		do
		{
			i++;
			sprintf(tempName, "temp%d",i);
		}
		while (FIND_BY_IDENTIFIER_IN_MANAGER(cmzn_glyph,name)(tempName, this->manager));
		glyph->setName(tempName);
	}
	ADD_OBJECT_TO_MANAGER(cmzn_glyph)(glyph, this->manager);
}

cmzn_glyph *cmzn_glyphmodule::createStaticGlyphFromGraphics(cmzn_graphics *graphics)
{
	if (graphics)
	{
		GT_object *graphicsObject = cmzn_graphics_copy_graphics_object(graphics);
		if (graphicsObject)
		{
			char temp_name[20];
			int i = NUMBER_IN_MANAGER(cmzn_glyph)(this->manager);
			do
			{
				i++;
				sprintf(temp_name, "temp%d",i);
			}
			while (FIND_BY_IDENTIFIER_IN_MANAGER(cmzn_glyph,name)(temp_name,
				this->manager));
			cmzn_glyph *glyph = cmzn_glyph_static::create(graphicsObject);
			glyph->setType(CMZN_GLYPH_SHAPE_TYPE_INVALID);
			glyph->setName(temp_name);
			glyph->setManaged(false);
			set_GT_object_default_material(graphicsObject, 0);
			this->addGlyph(glyph);
			DEACCESS(GT_object)(&graphicsObject);
			return glyph;
		}
	}
	return 0;
}

int cmzn_glyphmodule::defineStandardGlyphs()
{
	beginChange();
	GT_object *graphicsObject = 0;

	graphicsObject = create_GT_object_arrow_line("arrow", 1.f/3.f, 0.5);
	this->defineGlyphStatic(graphicsObject, CMZN_GLYPH_SHAPE_TYPE_ARROW);

	this->defineGlyph("arrow_solid", cmzn_glyph_arrow_solid::create(1.0/3.0, 1.0/3.0), CMZN_GLYPH_SHAPE_TYPE_ARROW_SOLID);

	graphicsObject = create_GT_object_arrow_line("axis", 0.1, 0.5);
	this->defineGlyphStatic(graphicsObject, CMZN_GLYPH_SHAPE_TYPE_AXIS);

	this->defineGlyph("axis_solid", cmzn_glyph_arrow_solid::create(0.1, 1.0/3.0), CMZN_GLYPH_SHAPE_TYPE_AXIS_SOLID);

	this->defineGlyph("cone", cmzn_glyph_cone::create(), CMZN_GLYPH_SHAPE_TYPE_CONE);
	this->defineGlyph("cone_solid", cmzn_glyph_cone_solid::create(), CMZN_GLYPH_SHAPE_TYPE_CONE_SOLID);

	graphicsObject = create_GT_object_cross("cross");
	this->defineGlyphStatic(graphicsObject, CMZN_GLYPH_SHAPE_TYPE_CROSS);

	graphicsObject = create_GT_object_cube_solid("cube_solid");
	this->defineGlyphStatic(graphicsObject, CMZN_GLYPH_SHAPE_TYPE_CUBE_SOLID);

	graphicsObject = create_GT_object_cube_wireframe("cube_wireframe");
	this->defineGlyphStatic(graphicsObject, CMZN_GLYPH_SHAPE_TYPE_CUBE_WIREFRAME);

	this->defineGlyph("cylinder", cmzn_glyph_cylinder::create(), CMZN_GLYPH_SHAPE_TYPE_CYLINDER);
	this->defineGlyph("cylinder_solid", cmzn_glyph_cylinder_solid::create(), CMZN_GLYPH_SHAPE_TYPE_CYLINDER_SOLID);

	graphicsObject = create_GT_object_sphere("diamond", 4, 2);
	this->defineGlyphStatic(graphicsObject, CMZN_GLYPH_SHAPE_TYPE_DIAMOND);

	graphicsObject = create_GT_object_line("line");
	this->defineGlyphStatic(graphicsObject, CMZN_GLYPH_SHAPE_TYPE_LINE);

	graphicsObject = create_GT_object_point("point", g_POINT_MARKER, 0);
	if (this->defineGlyphStatic(graphicsObject, CMZN_GLYPH_SHAPE_TYPE_POINT) && (!this->defaultPointGlyph))
	{
		setDefaultPointGlyph(this->findGlyphByName("point"));
	}

	graphicsObject = create_GT_object_sheet("sheet", /*define_texturepoints*/0);
	this->defineGlyphStatic(graphicsObject, CMZN_GLYPH_SHAPE_TYPE_SHEET);

	this->defineGlyph("sphere", cmzn_glyph_sphere::create(), CMZN_GLYPH_SHAPE_TYPE_SPHERE);

	cmzn_glyph_axes_id axes = 0;
	cmzn_glyph_id axis = this->findGlyphByType(CMZN_GLYPH_SHAPE_TYPE_AXIS);
	axes = cmzn_glyph_axes::create(axis, /*axis_width*/0.1);
	this->defineGlyph("axes", axes, CMZN_GLYPH_SHAPE_TYPE_AXES);
	axes = cmzn_glyph_axes::create(axis, /*axis_width*/0.1);
	axes->setAxisLabel(1, "1");
	axes->setAxisLabel(2, "2");
	axes->setAxisLabel(3, "3");
	this->defineGlyph("axes_123", axes, CMZN_GLYPH_SHAPE_TYPE_AXES_123);
	axes = cmzn_glyph_axes::create(axis, /*axis_width*/0.1);
	axes->setAxisLabel(1, "x");
	axes->setAxisLabel(2, "y");
	axes->setAxisLabel(3, "z");
	this->defineGlyph("axes_xyz", axes, CMZN_GLYPH_SHAPE_TYPE_AXES_XYZ);

	cmzn_glyph_id arrow_solid = this->findGlyphByType(CMZN_GLYPH_SHAPE_TYPE_ARROW_SOLID);
	axes = cmzn_glyph_axes::create(arrow_solid, /*axis_width*/0.25);
	this->defineGlyph("axes_solid", axes, CMZN_GLYPH_SHAPE_TYPE_AXES_SOLID);
	axes = cmzn_glyph_axes::create(arrow_solid, /*axis_width*/0.25);
	axes->setAxisLabel(1, "1");
	axes->setAxisLabel(2, "2");
	axes->setAxisLabel(3, "3");
	this->defineGlyph("axes_solid_123", axes, CMZN_GLYPH_SHAPE_TYPE_AXES_SOLID_123);
	axes = cmzn_glyph_axes::create(arrow_solid, /*axis_width*/0.25);
	axes->setAxisLabel(1, "x");
	axes->setAxisLabel(2, "y");
	axes->setAxisLabel(3, "z");
	this->defineGlyph("axes_solid_xyz", axes, CMZN_GLYPH_SHAPE_TYPE_AXES_SOLID_XYZ);

	cmzn_material_id red = cmzn_materialmodule_find_material_by_name(this->materialModule, "red");
	cmzn_material_id green = cmzn_materialmodule_find_material_by_name(this->materialModule, "green");
	cmzn_material_id blue = cmzn_materialmodule_find_material_by_name(this->materialModule, "blue");
	if (red && green && blue)
	{
		axes = cmzn_glyph_axes::create(axis, /*axis_width*/0.1);
		axes->setAxisMaterial(1, red);
		axes->setAxisMaterial(2, green);
		axes->setAxisMaterial(3, blue);
		this->defineGlyph("axes_colour", axes, CMZN_GLYPH_SHAPE_TYPE_AXES_COLOUR);

		axes = cmzn_glyph_axes::create(arrow_solid, /*axis_width*/0.25);
		axes->setAxisMaterial(1, red);
		axes->setAxisMaterial(2, green);
		axes->setAxisMaterial(3, blue);
		this->defineGlyph("axes_solid_colour", axes, CMZN_GLYPH_SHAPE_TYPE_AXES_SOLID_COLOUR);
	}
	cmzn_material_destroy(&red);
	cmzn_material_destroy(&green);
	cmzn_material_destroy(&blue);

	endChange();

	return CMZN_OK;
}

int cmzn_glyphmodule::defineStandardCmguiGlyphs()
{
	beginChange();
	GT_object *graphicsObject = 0;

	cmzn_glyph_id axis = this->findGlyphByType(CMZN_GLYPH_SHAPE_TYPE_AXIS);
	cmzn_glyph_axes_id axes = cmzn_glyph_axes::create(axis, /*axis_width*/0.1);
	if (axes)
	{
		axes->setAxisLabel(1, "f");
		axes->setAxisLabel(2, "s");
		axes->setAxisLabel(3, "n");
		this->defineGlyph("axes_fsn", axes, CMZN_GLYPH_SHAPE_TYPE_INVALID);
	}

	graphicsObject = create_GT_object_axes("grid_lines",
		/*make_solid*/0, /*head_length*/0.0, /*half_head_width*/0.0,
		/*labels*/(const char **)NULL, /*label_offset*/0.1f, (cmzn_font*)0);
	Graphics_object_set_glyph_labels_function(graphicsObject, draw_glyph_grid_lines);
	this->defineGlyphStatic(graphicsObject, CMZN_GLYPH_SHAPE_TYPE_INVALID);

	graphicsObject = create_GT_object_line("line_ticks");
	Graphics_object_set_glyph_labels_function(graphicsObject, draw_glyph_axes_ticks);
	this->defineGlyphStatic(graphicsObject, CMZN_GLYPH_SHAPE_TYPE_INVALID);

	graphicsObject = create_GT_object_sheet("textured_sheet", /*define_texturepoints*/1);
	this->defineGlyphStatic(graphicsObject, CMZN_GLYPH_SHAPE_TYPE_INVALID);

	endChange();
	return CMZN_OK;
}

cmzn_set_cmzn_glyph *cmzn_glyphmodule::getGlyphListPrivate()
{
	return reinterpret_cast<cmzn_set_cmzn_glyph *>(this->manager->object_list);
}

void cmzn_glyphmodule::addNotifier(cmzn_glyphmodulenotifier *notifier)
{
	this->notifier_list.push_back(notifier->access());
}

void cmzn_glyphmodule::removeNotifier(cmzn_glyphmodulenotifier *notifier)
{
	if (notifier)
	{
		cmzn_glyphmodulenotifier_list::iterator iter =
			std::find(this->notifier_list.begin(), this->notifier_list.end(), notifier);
		if (iter != this->notifier_list.end())
		{
			cmzn_glyphmodulenotifier::deaccess(notifier);
			this->notifier_list.erase(iter);
		}
	}
}

cmzn_glyphmodule_id cmzn_glyphmodule_create(cmzn_materialmodule *materialModule)
{
	return cmzn_glyphmodule::create(materialModule);
}

struct MANAGER(cmzn_glyph) *cmzn_glyphmodule_get_manager(
	cmzn_glyphmodule_id glyphmodule)
{
	if (glyphmodule)
		return glyphmodule->getManager();
	return 0;
}

cmzn_glyphmodule_id cmzn_glyphmodule_access(
	cmzn_glyphmodule_id glyphmodule)
{
	if (glyphmodule)
		return glyphmodule->access();
	return 0;
}

int cmzn_glyphmodule_destroy(cmzn_glyphmodule_id *glyphmodule_address)
{
	if (glyphmodule_address)
	{
		cmzn_glyphmodule::deaccess(*glyphmodule_address);
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_glyphmodule_begin_change(cmzn_glyphmodule_id glyphmodule)
{
	if (glyphmodule)
		return glyphmodule->beginChange();
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_glyphmodule_end_change(cmzn_glyphmodule_id glyphmodule)
{
	if (glyphmodule)
		return glyphmodule->endChange();
	return CMZN_ERROR_ARGUMENT;
}

cmzn_glyphiterator_id cmzn_glyphmodule_create_glyphiterator(
	cmzn_glyphmodule_id glyphmodule)
{
	if (glyphmodule)
		return glyphmodule->createGlyphiterator();
	return 0;
}

int cmzn_glyphmodule_define_standard_glyphs(
	cmzn_glyphmodule_id glyphmodule)
{
	if (glyphmodule)
		return glyphmodule->defineStandardGlyphs();
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_glyphmodule_define_standard_cmgui_glyphs(
	cmzn_glyphmodule_id glyphmodule)
{
	if (glyphmodule)
		return glyphmodule->defineStandardCmguiGlyphs();
	return CMZN_ERROR_ARGUMENT;
}

cmzn_glyph_id cmzn_glyphmodule_find_glyph_by_name(
	cmzn_glyphmodule_id glyphmodule, const char *name)
{
	cmzn_glyph *glyph = 0;
	if (glyphmodule)
	{
		glyph = glyphmodule->findGlyphByName(name);
		if (glyph)
			glyph->access();
	}
	return glyph;
}

cmzn_glyph_id cmzn_glyphmodule_find_glyph_by_glyph_shape_type(
	cmzn_glyphmodule_id glyphmodule, enum cmzn_glyph_shape_type glyph_type)
{
	cmzn_glyph *glyph = 0;
	if (glyphmodule)
	{
		glyph = glyphmodule->findGlyphByType(glyph_type);
		if (glyph)
			glyph->access();
	}
	return glyph;
}

cmzn_glyph_id cmzn_glyphmodule_get_default_point_glyph(
	cmzn_glyphmodule_id glyphmodule)
{
	if (glyphmodule)
		return glyphmodule->getDefaultPointGlyph();
	return 0;
}

int cmzn_glyphmodule_set_default_point_glyph(
	cmzn_glyphmodule_id glyphmodule, cmzn_glyph_id glyph)
{
	if (glyphmodule)
	{
		glyphmodule->setDefaultPointGlyph(glyph);
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_glyph *cmzn_glyphmodule_create_glyph_static(
	cmzn_glyphmodule_id glyphmodule, GT_object *graphicsObject)
{
	if (glyphmodule && graphicsObject)
	{
		cmzn_glyph *glyph = cmzn_glyph_static::create(graphicsObject);
		glyphmodule->addGlyph(glyph);
		return glyph;
	}
	return 0;
}

cmzn_glyph_id cmzn_glyphmodule_create_static_glyph_from_graphics(
	cmzn_glyphmodule_id glyphmodule, cmzn_graphics_id graphics)
{
	if (glyphmodule && graphics)
	{
		cmzn_glyph *glyph = glyphmodule->createStaticGlyphFromGraphics(graphics);
		return glyph;
	}
	return 0;
}

cmzn_glyphiterator_id cmzn_glyphiterator_access(cmzn_glyphiterator_id iterator)
{
	if (iterator)
		return iterator->access();
	return 0;
}

int cmzn_glyphiterator_destroy(cmzn_glyphiterator_id *iterator_address)
{
	if (!iterator_address)
		return 0;
	return cmzn_glyphiterator::deaccess(*iterator_address);
}

cmzn_glyph_id cmzn_glyphiterator_next(cmzn_glyphiterator_id iterator)
{
	if (iterator)
		return iterator->next();
	return 0;
}

cmzn_glyph_id cmzn_glyphiterator_next_non_access(cmzn_glyphiterator_id iterator)
{
	if (iterator)
		return iterator->next_non_access();
	return 0;
}

int cmzn_glyph_set_graphics_object(cmzn_glyph *glyph, GT_object *graphicsObject)
{
	if (glyph)
		return glyph->setGraphicsObject(graphicsObject);
	return CMZN_ERROR_ARGUMENT;
}

bool cmzn_glyph_contains_surface_primitives(cmzn_glyph *glyph,
	cmzn_tessellation *tessellation, cmzn_material *material, cmzn_font *font)
{
	bool return_code = false;
	if (glyph)
	{
		GT_object *gt_object = glyph->getGraphicsObject(tessellation, material, font);
		if (gt_object)
		{
			switch (GT_object_get_type(gt_object))
			{
				case g_SURFACE_VERTEX_BUFFERS:
					return_code = true;
				break;
				default:
				{
					GT_object *temp_glyph = GT_object_get_next_object(gt_object);
					while (temp_glyph)
					{
						if (g_SURFACE_VERTEX_BUFFERS == GT_object_get_type(temp_glyph))
						{
							return_code =  true;
							temp_glyph = 0;
						}
						else
						{
							temp_glyph = GT_object_get_next_object(temp_glyph);
						}
					}
				}	break;
			}
			DEACCESS(GT_object)(&gt_object);
		}
	}
	return return_code;
}
