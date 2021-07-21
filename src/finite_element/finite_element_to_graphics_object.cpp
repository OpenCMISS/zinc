/*******************************************************************************
FILE : finite_element_to_graphics_object.cpp

DESCRIPTION :
The functions for creating graphical objects from finite elements.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <limits.h>
#include <cmath>
#include <cstdlib>
#include "opencmiss/zinc/differentialoperator.h"
#include "opencmiss/zinc/fieldcache.h"
#include "opencmiss/zinc/mesh.h"
#include "opencmiss/zinc/nodeset.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_finite_element.h"
#include "computed_field/computed_field_wrappers.h"
#include "computed_field/field_cache.hpp"
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_adjacent_elements.h"
#include "finite_element/finite_element_discretization.h"
#include "finite_element/finite_element_mesh.hpp"
#include "finite_element/finite_element_region.h"
#include "finite_element/finite_element_to_graphics_object.h"
#include "finite_element/finite_element_to_iso_lines.h"
#include "general/debug.h"
#include "general/enumerator_private.hpp"
#include "general/geometry.h"
#include "general/matrix_vector.h"
#include "general/mystring.h"
#include "general/random.h"
#include "general/statistics.h"
#include "graphics/graphics_object.h"
#include "graphics/iso_field_calculation.h"
#include "graphics/spectrum.h"
#include "graphics/volume_texture.h"
#include "graphics/mcubes.h"
#include "general/message.h"
#include "graphics/graphics_object.hpp"
#include "mesh/cmiss_node_private.hpp"

/*
Module types
------------
*/

struct Glyph_set_data
/*******************************************************************************
LAST MODIFIED : 16 November 2000

DESCRIPTION :
Used with iterators for building glyph sets from nodes.
==============================================================================*/
{
	int number_of_points; // accumulate number of points with mandatory fields defined
	char **label;
	ZnReal *label_bounds;
	FE_value base_size[3], offset[3], *data_values, *label_bounds_vector, scale_factors[3];
	GLfloat *data;
	int graphics_name, *label_bounds_bit_pattern, label_bounds_components, label_bounds_dimension,
		label_bounds_values, n_data_components, *name;
	struct Computed_field *coordinate_field, *data_field, *label_field,
		*label_bounds_field, *label_density_field, *orientation_scale_field, *variable_scale_field,
		*subgroup_field, *group_field;
	Triple *label_density, *point, *axis1, *axis2, *axis3, *scale;
	enum cmzn_graphics_select_mode select_mode;
};

/*
Module functions
----------------
*/

/***************************************************************************//**
 * Adds the coordinates, orientation, data etc. of the fields at the field cache
 * location to the glyph_set_data.
 */
static int field_cache_location_to_glyph_point(cmzn_fieldcache_id field_cache,
	Glyph_set_data *glyph_set_data)
{
	ENTER(field_cache_location_to_glyph_point);
	if (!(field_cache && glyph_set_data))
	{
		display_message(ERROR_MESSAGE,
			"field_cache_location_to_glyph_point.  Invalid argument(s)");
		return 0;
	}
	int return_code = 1;
	bool show_point = true;
	if (glyph_set_data->select_mode == CMZN_GRAPHICS_SELECT_MODE_DRAW_SELECTED ||
		glyph_set_data->select_mode == CMZN_GRAPHICS_SELECT_MODE_DRAW_UNSELECTED)
	{
		if (glyph_set_data->group_field &&
			cmzn_field_evaluate_boolean(glyph_set_data->group_field, field_cache))
		{
			if (glyph_set_data->select_mode == CMZN_GRAPHICS_SELECT_MODE_DRAW_UNSELECTED)
			{
				show_point = false;
			}
		}
		else if (glyph_set_data->select_mode == CMZN_GRAPHICS_SELECT_MODE_DRAW_SELECTED)
		{
			show_point = false;
		}
	}
	if (show_point && glyph_set_data->subgroup_field)
	{
		show_point = cmzn_field_evaluate_boolean(glyph_set_data->subgroup_field, field_cache);
	}
	if (show_point)
	{
		FE_value a[3], b[3], c[3], coordinates[3], orientation_scale[9], size[3],
			variable_scale[3], *vector;
		struct Computed_field *coordinate_field, *data_field,
			*label_field, *label_density_field, *orientation_scale_field, *variable_scale_field;
		int dimension, i, j, k, values;
		Triple *axis1, *axis2, *axis3, *point, *scale;
		int number_of_orientation_scale_components = 0;
		int number_of_variable_scale_components = 0;

		if ((coordinate_field=glyph_set_data->coordinate_field) &&
			(3 >= cmzn_field_get_number_of_components(coordinate_field)) &&
			((!(orientation_scale_field =
					glyph_set_data->orientation_scale_field)) ||
				(9 >= (number_of_orientation_scale_components =
					cmzn_field_get_number_of_components(orientation_scale_field)))) &&
			((!(data_field = glyph_set_data->data_field)) ||
				glyph_set_data->data) &&
			((!(label_field = glyph_set_data->label_field)) ||
				glyph_set_data->label) &&
			((!(label_density_field = glyph_set_data->label_density_field)) ||
				glyph_set_data->label_density) &&
			((!(variable_scale_field = glyph_set_data->variable_scale_field)) ||
				(3 >= (number_of_variable_scale_components =
					cmzn_field_get_number_of_components(variable_scale_field)))) &&
			(point = glyph_set_data->point) &&
			(axis1 = glyph_set_data->axis1) &&
			(axis2 = glyph_set_data->axis2) &&
			(axis3 = glyph_set_data->axis3) &&
			(scale = glyph_set_data->scale))
		{
			/* clear coordinates in case coordinate field is not 3 component */
			coordinates[0] = 0.0;
			coordinates[1] = 0.0;
			coordinates[2] = 0.0;
			/* label_field allowed to be undefined at individual nodes, so default to NULL label */
			if (glyph_set_data->label)
			{
				*(glyph_set_data->label) = NULL;
			}
			FE_value_triple fe_value_label_density;

			/* evaluate the fields at the cache location */
			bool all_fields_defined = (CMZN_OK == cmzn_field_evaluate_real(coordinate_field,
				field_cache, /*number_of_values*/3, coordinates));
			if (all_fields_defined && orientation_scale_field)
			{
				all_fields_defined = (CMZN_OK == cmzn_field_evaluate_real(orientation_scale_field,
					field_cache, /*number_of_values*/9, orientation_scale));
			}
			if (all_fields_defined && variable_scale_field)
			{
				all_fields_defined = (CMZN_OK == cmzn_field_evaluate_real(variable_scale_field,
					field_cache, /*number_of_values*/3, variable_scale));
			}
			if (all_fields_defined && data_field)
			{
				all_fields_defined = (CMZN_OK == cmzn_field_evaluate_real(data_field,
					field_cache, glyph_set_data->n_data_components,
					glyph_set_data->data_values));
			}
			if (all_fields_defined && label_field)
			{
				*(glyph_set_data->label) = cmzn_field_evaluate_string(label_field,
					field_cache);
				if (label_density_field)
				{
					all_fields_defined = (CMZN_OK == cmzn_field_evaluate_real(label_density_field,
						field_cache, /*number_of_values*/3, fe_value_label_density));
				}
			}
			if (all_fields_defined)
			{
				if (make_glyph_orientation_scale_axes(
					number_of_orientation_scale_components, orientation_scale,
					a, b, c, size))
				{
					++(glyph_set_data->number_of_points);
					for (j = 0; j < 3; j++)
					{
						(*scale)[j] = static_cast<GLfloat>(size[j]);
					}
					for (j = 0; j < number_of_variable_scale_components; j++)
					{
						(*scale)[j] *= static_cast<GLfloat>(variable_scale[j]);
					}
					for (j = 0; j < 3; j++)
					{
						(*point)[j] = static_cast<GLfloat>(coordinates[j]);
						(*axis1)[j] = static_cast<GLfloat>(a[j]);
						(*axis2)[j] = static_cast<GLfloat>(b[j]);
						(*axis3)[j] = static_cast<GLfloat>(c[j]);
					}
					(glyph_set_data->point)++;
					(glyph_set_data->axis1)++;
					(glyph_set_data->axis2)++;
					(glyph_set_data->axis3)++;
					(glyph_set_data->scale)++;

					if (glyph_set_data->data_field)
					{
						CAST_TO_OTHER(glyph_set_data->data, glyph_set_data->data_values,
							ZnReal,glyph_set_data->n_data_components);
						glyph_set_data->data +=
							glyph_set_data->n_data_components;
					}
					if (glyph_set_data->name)
					{
						*(glyph_set_data->name) = glyph_set_data->graphics_name;
						(glyph_set_data->name)++;
					}
					if (glyph_set_data->label_field)
					{
						(glyph_set_data->label)++;
						if (label_density_field)
						{
							(*(glyph_set_data->label_density))[0] =
								(GLfloat)fe_value_label_density[0];
							(*(glyph_set_data->label_density))[1] =
								(GLfloat)fe_value_label_density[1];
							(*(glyph_set_data->label_density))[2] =
								(GLfloat)fe_value_label_density[2];
							(glyph_set_data->label_density)++;
						}
					}
					/* Record the value of the label field at the bounds of the glyph size
						into a tensor so that they can be used for adding grids and axes to the glyph */
					// ???GRC This evaluation should be done with a separate field_cache object in future
					if (glyph_set_data->label_bounds)
					{
						int nComponents = glyph_set_data->label_bounds_components;
						FE_value *fieldValues;
						ALLOCATE(fieldValues,FE_value,nComponents);
						dimension = glyph_set_data->label_bounds_dimension;
						values = glyph_set_data->label_bounds_values;
						vector = glyph_set_data->label_bounds_vector;
						FE_value label_scale[3];
						for (k = 0; k < 3; ++k)
						{
							label_scale[k] = glyph_set_data->base_size[k] + size[k]*glyph_set_data->scale_factors[k];
						}
						for (i = 0 ; i < values ; i++)
						{
							for (k = 0 ; k < dimension ; k++)
							{
								vector[k] = coordinates[k] + 
									glyph_set_data->offset[0]*label_scale[0]*a[k] + 
									glyph_set_data->offset[1]*label_scale[1]*b[k] + 
									glyph_set_data->offset[2]*label_scale[2]*c[k]; 
							}
							for (j = 0 ; j < dimension; j++)
							{
								if (i & glyph_set_data->label_bounds_bit_pattern[j])
								{
									switch (j)
									{
										case 0:
										{
											for (k = 0 ; k < dimension ; k++)
											{
												vector[k] += a[k]*label_scale[0];
											}
										} break;
										case 1:
										{
											for (k = 0 ; k < dimension ; k++)
											{
												vector[k] += b[k]*label_scale[1];
											}
										} break;
										case 2:
										{
											for (k = 0 ; k < dimension ; k++)
											{
												vector[k] += c[k]*label_scale[2];
											}
										} break;
									}
								}
							}
							/* Set the offset coordinates in the field cache field and evaluate the label field there */
							cmzn_fieldcache_set_assign_in_cache(field_cache, 1);
							if (CMZN_OK != cmzn_field_assign_real(coordinate_field, field_cache,
								/*number_of_values*/glyph_set_data->label_bounds_dimension, vector))
							{
								return_code = 0;
							}
							if (CMZN_OK != cmzn_field_evaluate_real(glyph_set_data->label_bounds_field,
								field_cache, nComponents, fieldValues))
							{
								return_code = 0;
							}
							cmzn_fieldcache_set_assign_in_cache(field_cache, 0);
							if (return_code)
							{
								CAST_TO_OTHER(glyph_set_data->label_bounds,
									fieldValues, ZnReal, nComponents);
							}
							else
							{
								display_message(WARNING_MESSAGE,
									"field_cache_location_to_glyph_point.  Unable to evaluate label bounds.");
							}
							glyph_set_data->label_bounds += glyph_set_data->label_bounds_components;
						}
						DEALLOCATE(fieldValues);
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"field_cache_location_to_glyph_point.  Failed to make glyph orientation scale axes");
					return_code = 0;
				}
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"field_cache_location_to_glyph_point.  Invalid glyph_set_data");
			return_code = 0;
		}
	}
	LEAVE;

	return (return_code);
}

/*
Global functions
----------------
*/

int make_glyph_orientation_scale_axes(
	int number_of_orientation_scale_values, FE_value *orientation_scale_values,
	FE_value *axis1,FE_value *axis2, FE_value *axis3, FE_value *size)
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
2 = scaling in direction of 2 vectors, ie. they keep their current length, scale
    in 3rd axis equal to magnitude of cross product of first two vectors;
3 = scaling in direction of 3 vectors - ie. they keep their current length.

Function returns the axes as unit vectors with their magnitudes in the <size>
array. This is always possible if there is a scalar (or zero scalar), but where
zero vectors are either read or calculated from the <orientation_scale_values>,
these are simply returned, since no valid direction can be produced.
==============================================================================*/
{
	double magnitude;
	int return_code;

	ENTER(make_glyph_orientation_scale_axes);
	if (((0 == number_of_orientation_scale_values) ||
		((0<number_of_orientation_scale_values) && orientation_scale_values)) &&
		axis1 && axis2 && axis3 && size)
	{
		return_code=1;
		switch (number_of_orientation_scale_values)
		{
			case 0:
			{
				/* zero scalar; axes = x,y,z */
				size[0]=size[1]=size[2]=0.0;
				axis1[0]=1.0;
				axis1[1]=0.0;
				axis1[2]=0.0;
				axis2[0]=0.0;
				axis2[1]=1.0;
				axis2[2]=0.0;
				axis3[0]=0.0;
				axis3[1]=0.0;
				axis3[2]=1.0;
			} break;
			case 1:
			{
				/* scalar; axes = x,y,z */
				size[0]=size[1]=size[2]=orientation_scale_values[0];
				axis1[0]=1.0;
				axis1[1]=0.0;
				axis1[2]=0.0;
				axis2[0]=0.0;
				axis2[1]=1.0;
				axis2[2]=0.0;
				axis3[0]=0.0;
				axis3[1]=0.0;
				axis3[2]=1.0;
			} break;
			case 2:
			{
				/* 1 2-D vector */
				axis1[0]=orientation_scale_values[0];
				axis1[1]=orientation_scale_values[1];
				axis1[2]=0.0;
				if (0.0<(magnitude=sqrt(axis1[0]*axis1[0]+axis1[1]*axis1[1])))
				{
					axis1[0] /= magnitude;
					axis1[1] /= magnitude;
				}
				size[0]=size[1]=size[2]=magnitude;
				/* get axis2 orthogonal to axis 1 in x-y plane (in right hand sense) */
				axis2[0]=-axis1[1];
				axis2[1]=axis1[0];
				axis2[2]=0.0;
				/* axis3 is along the z-axis, of same length as other axes */
				axis3[0]=0.0;
				axis3[1]=0.0;
				axis3[2]=1.0;
			} break;
			case 3:
			{
				/* 1 3-D vector */
				axis1[0]=orientation_scale_values[0];
				axis1[1]=orientation_scale_values[1];
				axis1[2]=orientation_scale_values[2];
				/* get magnitude of axis1 vector to make axis2 and axis3 this size */
				if (0.0<(magnitude=
					sqrt(axis1[0]*axis1[0]+axis1[1]*axis1[1]+axis1[2]*axis1[2])))
				{
					axis1[0] /= magnitude;
					axis1[1] /= magnitude;
					axis1[2] /= magnitude;
					size[0]=size[1]=size[2]=magnitude;
					/* get axis3, non-colinear with axis1 */
					axis3[0]=0.0;
					axis3[1]=0.0;
					axis3[2]=0.0;
					if (fabs(axis1[0]) < fabs(axis1[1]))
					{
						if (fabs(axis1[2]) < fabs(axis1[0]))
						{
							axis3[2]=1.0;
						}
						else
						{
							axis3[0]=1.0;
						}
					}
					else
					{
						if (fabs(axis1[2]) < fabs(axis1[1]))
						{
							axis3[2]=1.0;
						}
						else
						{
							axis3[1]=1.0;
						}
					}
					/* get axis2 = axis3 (x) axis1 = vector orthogonal to axis1 */
					axis2[0]=axis3[1]*axis1[2]-axis3[2]*axis1[1];
					axis2[1]=axis3[2]*axis1[0]-axis3[0]*axis1[2];
					axis2[2]=axis3[0]*axis1[1]-axis3[1]*axis1[0];
					/* make axis2 unit length */
					magnitude=sqrt(axis2[0]*axis2[0]+axis2[1]*axis2[1]+axis2[2]*axis2[2]);
					axis2[0] /= magnitude;
					axis2[1] /= magnitude;
					axis2[2] /= magnitude;
					/* get axis3 = axis1 (x) axis2 = unit vector */
					axis3[0]=axis1[1]*axis2[2]-axis1[2]*axis2[1];
					axis3[1]=axis1[2]*axis2[0]-axis1[0]*axis2[2];
					axis3[2]=axis1[0]*axis2[1]-axis1[1]*axis2[0];
				}
				else
				{
					/* magnitude of axis1 zero, so clear axis2 and axis3 */
					axis2[0]=0.0;
					axis2[1]=0.0;
					axis2[2]=0.0;
					axis3[0]=0.0;
					axis3[1]=0.0;
					axis3[2]=0.0;
					size[0]=size[1]=size[2]=0.0;
				}
			} break;
			case 4:
			case 6:
			{
				/* Two vectors */
				if (number_of_orientation_scale_values == 4)
				{
					/* Two 2-D vectors */
					axis1[0]=orientation_scale_values[0];
					axis1[1]=orientation_scale_values[1];
					axis1[2]=0.0;
					axis2[0]=orientation_scale_values[2];
					axis2[1]=orientation_scale_values[3];
					axis2[2]=0.0;
				}
				else
				{
					/* Two 3-D vectors */
					axis1[0]=orientation_scale_values[0];
					axis1[1]=orientation_scale_values[1];
					axis1[2]=orientation_scale_values[2];
					axis2[0]=orientation_scale_values[3];
					axis2[1]=orientation_scale_values[4];
					axis2[2]=orientation_scale_values[5];
				}
				/* get axis3 = axis1 (x) axis2 */
				axis3[0]=axis1[1]*axis2[2]-axis1[2]*axis2[1];
				axis3[1]=axis1[2]*axis2[0]-axis1[0]*axis2[2];
				axis3[2]=axis1[0]*axis2[1]-axis1[1]*axis2[0];
				/* make all axes into unit vectors with size = magnitude */
				if (0.0<(magnitude=
					sqrt(axis1[0]*axis1[0]+axis1[1]*axis1[1]+axis1[2]*axis1[2])))
				{
					axis1[0] /= magnitude;
					axis1[1] /= magnitude;
					axis1[2] /= magnitude;
				}
				size[0]=magnitude;
				if (0.0<(magnitude=
					sqrt(axis2[0]*axis2[0]+axis2[1]*axis2[1]+axis2[2]*axis2[2])))
				{
					axis2[0] /= magnitude;
					axis2[1] /= magnitude;
					axis2[2] /= magnitude;
				}
				size[1]=magnitude;
				if (0.0<(magnitude=
					sqrt(axis3[0]*axis3[0]+axis3[1]*axis3[1]+axis3[2]*axis3[2])))
				{
					axis3[0] /= magnitude;
					axis3[1] /= magnitude;
					axis3[2] /= magnitude;
				}
				size[2]=magnitude;
			} break;
			case 9:
			{
				/* 3 3-D vectors */
				/* axis 1 */
				axis1[0]=orientation_scale_values[0];
				axis1[1]=orientation_scale_values[1];
				axis1[2]=orientation_scale_values[2];
				if (0.0<(magnitude=
					sqrt(axis1[0]*axis1[0]+axis1[1]*axis1[1]+axis1[2]*axis1[2])))
				{
					axis1[0] /= magnitude;
					axis1[1] /= magnitude;
					axis1[2] /= magnitude;
				}
				size[0]=magnitude;
				/* axis 2 */
				axis2[0]=orientation_scale_values[3];
				axis2[1]=orientation_scale_values[4];
				axis2[2]=orientation_scale_values[5];
				if (0.0<(magnitude=
					sqrt(axis2[0]*axis2[0]+axis2[1]*axis2[1]+axis2[2]*axis2[2])))
				{
					axis2[0] /= magnitude;
					axis2[1] /= magnitude;
					axis2[2] /= magnitude;
				}
				size[1]=magnitude;
				/* axis 3 */
				axis3[0]=orientation_scale_values[6];
				axis3[1]=orientation_scale_values[7];
				axis3[2]=orientation_scale_values[8];
				if (0.0<(magnitude=
					sqrt(axis3[0]*axis3[0]+axis3[1]*axis3[1]+axis3[2]*axis3[2])))
				{
					axis3[0] /= magnitude;
					axis3[1] /= magnitude;
					axis3[2] /= magnitude;
				}
				size[2]=magnitude;
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,"make_glyph_orientation_scale_axes.  "
					"Invalid number_of_orientation_scale_values");
				return_code=0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"make_glyph_orientation_scale_axes.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* make_glyph_orientation_scale_axes */

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
	struct cmzn_font *font,	FE_value *label_offset, char *static_label_text[3],
	enum cmzn_graphics_select_mode select_mode)
{
	GT_glyphset_vertex_buffers *glyphset = 0;
	char *glyph_name, **labels;
	ZnReal *label_bounds;
	FE_value *label_bounds_vector = NULL;
	GLfloat *data;
	int coordinate_dimension, i, *label_bounds_bit_pattern = NULL, label_bounds_components = 0,
		label_bounds_dimension, label_bounds_values = 0, n_data_components, *names, return_code = 1;
	if (field_cache && nodeset && coordinate_field && graphics_object &&
		(3>=(coordinate_dimension=cmzn_field_get_number_of_components(coordinate_field)))&&
		((!orientation_scale_field) ||
			(9 >= cmzn_field_get_number_of_components(orientation_scale_field)))&&
		((glyph && offset && base_size && scale_factors && label_offset &&
		((!variable_scale_field) ||
			(3 >=	cmzn_field_get_number_of_components(variable_scale_field))))
		|| !glyph) &&
		((!label_density_field) ||
			(3 >=	cmzn_field_get_number_of_components(label_density_field))))
	{

		/* label_field is not a required field (if undefined, label is empty) EXCEPT
		 * where glyph bases its glyph_labels_function bounds on it */
		struct Computed_field *label_bounds_field = NULL;
		if (glyph && Graphics_object_get_glyph_labels_function(glyph))
		{
			if (label_field)
			{
					if (Computed_field_has_numerical_components(label_field,NULL))
					{
						label_bounds_field = label_field;
					}
					else
					{
						if (GET_NAME(GT_object)(glyph,&glyph_name))
						{
							display_message(ERROR_MESSAGE,
								"Nodeset_add_glyph_to_vertex_array.  "
								"Label field must be numeric for use with glyph '%s'",
								glyph_name);
							DEALLOCATE(glyph_name);
						}
						return_code = 0;
					}
			}
			else
			{
				label_bounds_field = coordinate_field;
			}
		}
		if (return_code && ((CMZN_GRAPHICS_SELECT_MODE_DRAW_SELECTED!=select_mode) || group_field))
		{
			// allocate for all nodes, trim arrays for fields not defined
			int number_of_points = cmzn_nodeset_get_size(nodeset);
			if (0 < number_of_points)
			{
				unsigned int final_number_of_points = 0; // will include only points with all mandatory fields defined
				Triple *point_list = (Triple *)NULL;
				Triple *axis1_list = (Triple *)NULL;
				Triple *axis2_list = (Triple *)NULL;
				Triple *axis3_list = (Triple *)NULL;
				Triple *scale_list = (Triple *)NULL;
				Triple *label_density_list = (Triple *)NULL;
				labels = (char **)NULL;
				n_data_components = 0;
				data = 0;
				FE_value *data_values = 0;
				names = (int *)NULL;
				label_bounds_dimension = 0;
				label_bounds = (ZnReal *)NULL;
				if (data_field)
				{
					n_data_components =
						cmzn_field_get_number_of_components(data_field);
					ALLOCATE(data, GLfloat, number_of_points*n_data_components);
					data_values = new FE_value[n_data_components];
				}
				if (label_field)
				{
					ALLOCATE(labels, char *, number_of_points);
				}
				if (label_bounds_field)
				{
					label_bounds_dimension = coordinate_dimension;
					label_bounds_values = 1 << label_bounds_dimension;
					label_bounds_components = cmzn_field_get_number_of_components(label_bounds_field);
					ALLOCATE(label_bounds, ZnReal, number_of_points * label_bounds_values *
						label_bounds_components);
					/* Temporary space for evaluating the label field */
					ALLOCATE(label_bounds_vector, FE_value, label_bounds_dimension);
					/* Prcompute bit pattern for label values */
					ALLOCATE(label_bounds_bit_pattern, int, label_bounds_dimension);
					label_bounds_bit_pattern[0] = 1;
					for (i = 1 ; i < label_bounds_dimension ; i++)
					{
						label_bounds_bit_pattern[i] = 2 * label_bounds_bit_pattern[i - 1];
					}
				}
				if (CMZN_GRAPHICS_SELECT_MODE_OFF != select_mode)
				{
					ALLOCATE(names,int,number_of_points);
				}
				if (label_density_field)
				{
					ALLOCATE(label_density_list, Triple, number_of_points);
				}
				ALLOCATE(point_list, Triple, number_of_points);
				ALLOCATE(axis1_list, Triple, number_of_points);
				ALLOCATE(axis2_list, Triple, number_of_points);
				ALLOCATE(axis3_list, Triple, number_of_points);
				ALLOCATE(scale_list, Triple, number_of_points);
				Triple glyph_base_size, glyph_scale_factors, glyph_offset, glyph_label_offset;
				for (i = 0; i < 3; i++)
				{
					glyph_base_size[i] = static_cast<GLfloat>(base_size[i]);
					glyph_scale_factors[i] = static_cast<GLfloat>(scale_factors[i]);
					glyph_offset[i] = static_cast<GLfloat>(offset[i]);
					glyph_label_offset[i] = static_cast<GLfloat>(label_offset[i]);
				}
				if (point_list && axis1_list && axis2_list && axis3_list && scale_list &&
					((!n_data_components) || (data && data_values)) &&
					((!label_field) || labels) &&
					((CMZN_GRAPHICS_SELECT_MODE_OFF==select_mode)||names))
				{
					Glyph_set_data glyph_set_data;
					glyph_set_data.number_of_points = 0;
					/* set up information for the iterator */
					for (i = 0; i < 3; i++)
					{
						glyph_set_data.base_size[i] = base_size[i];
						glyph_set_data.offset[i] = offset[i];
						glyph_set_data.scale_factors[i] = scale_factors[i];
					}
					glyph_set_data.point = point_list;
					glyph_set_data.axis1 = axis1_list;
					glyph_set_data.axis2 = axis2_list;
					glyph_set_data.axis3 = axis3_list;
					glyph_set_data.scale = scale_list;
					glyph_set_data.data = data;
					glyph_set_data.label = labels;
					glyph_set_data.label_density = label_density_list;
					glyph_set_data.coordinate_field = coordinate_field;
					glyph_set_data.orientation_scale_field =
						orientation_scale_field;
					glyph_set_data.variable_scale_field = variable_scale_field;
					glyph_set_data.data_field = data_field;
					glyph_set_data.n_data_components = n_data_components;
					glyph_set_data.data_values = data_values;
					glyph_set_data.label_field = label_field;
					glyph_set_data.label_density_field = label_density_field;
					glyph_set_data.subgroup_field = subgroup_field;
					glyph_set_data.name = names;
					glyph_set_data.label_bounds_bit_pattern = label_bounds_bit_pattern;
					glyph_set_data.label_bounds_components = label_bounds_components;
					glyph_set_data.label_bounds_dimension = label_bounds_dimension;
					glyph_set_data.label_bounds_field = label_bounds_field;
					glyph_set_data.label_bounds_values = label_bounds_values;
					glyph_set_data.label_bounds_vector = label_bounds_vector;
					glyph_set_data.label_bounds = label_bounds;
					glyph_set_data.group_field = group_field;
					glyph_set_data.select_mode = select_mode;

					cmzn_nodeiterator_id iterator = cmzn_nodeset_create_nodeiterator(nodeset);
					cmzn_node_id node = 0;
					while (return_code && (0 != (node = cmzn_nodeiterator_next_non_access(iterator))))
					{
						cmzn_fieldcache_set_node(field_cache, node);
						glyph_set_data.graphics_name = get_FE_node_identifier(node);
						return_code = field_cache_location_to_glyph_point(field_cache, &glyph_set_data);
					}
					cmzn_nodeiterator_destroy(&iterator);
					final_number_of_points = glyph_set_data.number_of_points;
				}
				else
				{
					return_code = 0;
				}
				if (return_code && (0 < final_number_of_points))
				{
					if (final_number_of_points < (unsigned int)number_of_points)
					{
						// assuming reallocating to smaller size always succeeds
						REALLOCATE(point_list, point_list, Triple, final_number_of_points);
						REALLOCATE(axis1_list, axis1_list, Triple, final_number_of_points);
						REALLOCATE(axis2_list, axis2_list, Triple, final_number_of_points);
						REALLOCATE(axis3_list, axis3_list, Triple, final_number_of_points);
						REALLOCATE(scale_list, scale_list, Triple, final_number_of_points);
						if (label_density_field)
						{
							REALLOCATE(label_density_list, label_density_list, Triple, final_number_of_points);
						}
						if (data)
						{
							REALLOCATE(data, data, GLfloat, final_number_of_points*n_data_components);
						}
						if (labels)
						{
							REALLOCATE(labels, labels, char *, final_number_of_points);
						}
						if (names)
						{
							REALLOCATE(names, names, int, final_number_of_points);
						}
					}
					glyphset = CREATE(GT_glyphset_vertex_buffers)();
					GT_glyphset_vertex_buffers_setup(glyphset,
						glyph, glyph_repeat_mode,
						glyph_base_size, glyph_scale_factors, glyph_offset, font,
						glyph_label_offset, static_label_text,
						label_bounds_dimension, label_bounds_components);
					if (0 == fill_glyph_graphics_vertex_array(
						GT_object_get_vertex_set(graphics_object), /*vertex_location*/-1,
						 (unsigned int)final_number_of_points, point_list,
						axis1_list, axis2_list, axis3_list, scale_list,
						n_data_components, data,
						label_density_list, /*object_name*/-1, names, labels,
						label_bounds_values, label_bounds_components, label_bounds))
					{
						DESTROY(GT_glyphset_vertex_buffers)(&glyphset);
					}
				}
				if (label_bounds_field)
				{
					DEALLOCATE(label_bounds);
					/* Temporary space for evaluating the label field */
					DEALLOCATE(label_bounds_vector);
					/* Prcompute bit pattern for label values */
					DEALLOCATE(label_bounds_bit_pattern);
				}

				DEALLOCATE(point_list);
				DEALLOCATE(axis1_list);
				DEALLOCATE(axis2_list);
				DEALLOCATE(axis3_list);
				DEALLOCATE(scale_list);
				DEALLOCATE(label_density_list);
				DEALLOCATE(data);
				DEALLOCATE(names);
				if (labels)
				{
					for (int k = 0; k < (int)final_number_of_points; k++)
						DEALLOCATE(labels[k]);
					DEALLOCATE(labels);
				}
				delete[] data_values;
			} /* no points, hence no glyph_set */
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Nodeset_add_glyph_to_vertex_array.  Invalid argument(s)");
	}

	return glyphset;
}

int FE_element_add_line_to_vertex_array(struct FE_element *element,
	cmzn_fieldcache_id field_cache, struct Graphics_vertex_array *array,
	Computed_field *coordinate_field,
	int number_of_data_values, Computed_field *data_field,
	Computed_field *texture_coordinate_field,
	unsigned int number_of_segments, FE_element *top_level_element)
{
	FE_value distance, xi;
	int return_code;
	unsigned int i, vertex_start, number_of_vertices;
	GLfloat *floatData = 0;
	double *data_buffer = 0;

	ENTER(FE_element_add_line_to_vertex_buffer_set)
	int coordinate_dimension = cmzn_field_get_number_of_components(coordinate_field);
	int texture_coordinate_dimension = texture_coordinate_field ?
		cmzn_field_get_number_of_components(texture_coordinate_field) : 0;
	if (element && field_cache && array && (1 == get_FE_element_dimension(element)) &&
		coordinate_field && (3 >= coordinate_dimension) &&
		(!texture_coordinate_field || (3 >= texture_coordinate_dimension)))
	{
		return_code = 1;

		/* clear coordinates in case coordinate field is not 3 component */

		/* for selective editing of GT_object primitives, record element index */
		const DsLabelIndex elementIndex = get_FE_element_index(element);

		int replaceRequired = 0;
		/* find if vertex already in the array */
		int vertex_location = array->find_first_fast_search_id_location(elementIndex);
		/* vertex found in array, check if update is required */
		if (vertex_location >= 0)
		{
			array->get_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_UPDATE_REQUIRED,
				vertex_location, 1, &replaceRequired);
		}
		if (vertex_location < 0 || replaceRequired)
		{
			if (vertex_location < 0)
			{
				array->add_fast_search_id(elementIndex);
				array->add_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_OBJECT_ID,
					1, 1, &elementIndex);
				int modificationRequired = 0;
				array->add_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_UPDATE_REQUIRED,
					1, 1, &modificationRequired);
				/* case for addition */
				vertex_start = array->get_number_of_vertices(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION);
			}
			else
			{
				array->replace_integer_vertex_buffer_at_position(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_OBJECT_ID, vertex_location, 1, 1,
					&elementIndex);
				/* case for modification */
				array->get_unsigned_integer_attribute(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
					vertex_location, 1, &vertex_start);
			}
			FE_value coordinates[3];
			coordinates[0]=0.0;
			coordinates[1]=0.0;
			coordinates[2]=0.0;

			if (data_field)
			{
				number_of_data_values = cmzn_field_get_number_of_components(data_field);
				floatData = new GLfloat[number_of_data_values];
				data_buffer = new double[number_of_data_values];
			}

			FE_value texture_coordinates[3];
			if (texture_coordinate_field)
			{
				texture_coordinates[0] = 0.0;
				texture_coordinates[1] = 0.0;
				texture_coordinates[2] = 0.0;
			}

			distance=(FE_value)number_of_segments;
			for (i = 0; (i <= number_of_segments); i++)
			{
				xi=((FE_value)i)/distance;
				/* evaluate the fields */
				return_code = (CMZN_OK == field_cache->setIndexedMeshLocation(i, element, &xi, top_level_element));
				if (return_code && (CMZN_OK == cmzn_field_evaluate_real(coordinate_field,
					field_cache, coordinate_dimension, coordinates)) &&
					((!data_field) || (CMZN_OK == cmzn_field_evaluate_real(data_field,
						field_cache, number_of_data_values, data_buffer))) &&
						((!texture_coordinate_field) || (CMZN_OK == cmzn_field_evaluate_real(texture_coordinate_field,
							field_cache, texture_coordinate_dimension, texture_coordinates))))
				{
					GLfloat floatField[3];
					CAST_TO_OTHER(floatField,coordinates,GLfloat,3);
					if (vertex_location < 0)
					{
						array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
							3, 1, floatField);
						if (data_field)
						{
							CAST_TO_OTHER(floatData,data_buffer,GLfloat,number_of_data_values);
							array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
								number_of_data_values, 1, floatData);
						}
						if (texture_coordinate_field)
						{
							CAST_TO_OTHER(floatField,texture_coordinates,GLfloat,3);
							array->add_float_attribute(
								GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_TEXTURE_COORDINATE_ZERO,
								3, 1, floatField);
						}
					}
					else
					{
						array->replace_float_vertex_buffer_at_position(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
							vertex_start + i, 3, 1, floatField);
						if (data_field)
						{
							CAST_TO_OTHER(floatData,data_buffer,GLfloat,number_of_data_values);
							array->replace_float_vertex_buffer_at_position(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
								vertex_start + i, number_of_data_values, 1, floatData);
						}
						if (texture_coordinate_field)
						{
							CAST_TO_OTHER(floatField,texture_coordinates,GLfloat,3);
							array->replace_float_vertex_buffer_at_position(
								GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_TEXTURE_COORDINATE_ZERO,
								vertex_start + i, 3, 1, floatField);
						}
					}
				}
				else
				{
					return_code = 0;
					break;
				}
			}
			if (return_code)
			{
				if (vertex_location < 0)
				{
					number_of_vertices = number_of_segments+1;
					array->add_unsigned_integer_attribute(
						GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
						1, 1, &number_of_vertices);
					array->add_unsigned_integer_attribute(
						GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
						1, 1, &vertex_start);
				}
				else if (replaceRequired)
				{
					int updated = 0;
					array->replace_integer_vertex_buffer_at_position(
						GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_UPDATE_REQUIRED,
						vertex_location, 1, 1, &updated);
				}
			}
			/* else could try and remove vertices that failed */

			/* I don't think I need to clear the field cache's here, instead I have
			 * done it in GT_element_settings_to_graphics_object.
			 */
			delete[] data_buffer;
			delete[] floatData;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_element_add_line_to_vertex_buffer_set.  "
			"Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_element_add_line_to_vertex_buffer_set */

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
	struct FE_element *top_level_element)
{
	FE_value coordinates[3], cos_theta, derivative_xi[3], distance, dS_dxi,
		end_aligned_normal[3], facet_angle, jacobian[9], length, normal_1, normal_2,
		normal_3, *radius_array, diameter_derivative, diameter_value, sin_theta,
		tex_coordinates[3], theta, theta_change, xi, x, y;
	ZnReal texture_coordinate1;
	GLfloat *data = 0, *datum = 0, *original_data = 0;
	int facet_offset,i,j,k,n_data_components,number_of_points, return_code;
	Triple *derivative, *normal = NULL, *normalpoints, *point, *points, *texturepoints, *previous_point,
		*previous_normal, *texture_coordinate;
	unsigned int vertex_start = 0;

	int coordinate_dimension = cmzn_field_get_number_of_components(coordinate_field);
	int texture_coordinate_dimension = texture_coordinate_field ?
		cmzn_field_get_number_of_components(texture_coordinate_field) : 0;
	return_code = 0;
	if (element && field_cache && line_mesh && (1 == get_FE_element_dimension(element)) &&
		(0 < number_of_segments_along) && (1 < number_of_segments_around) &&
		coordinate_field && (3 >= coordinate_dimension) &&
		(0 != base_size) && (0 != scale_factors) &&
		((!orientation_scale_field) ||
			(1 == cmzn_field_get_number_of_components(orientation_scale_field))) &&
		((!texture_coordinate_field) || (3 >= texture_coordinate_dimension)))
	{
		return_code = 1;
		cmzn_differentialoperator_id d_dxi = cmzn_mesh_get_chart_differentialoperator(line_mesh, /*order*/1, 1);
		/* clear coordinates and derivatives not set if coordinate field is not
			 3 component */
		coordinates[1]=0.0;
		coordinates[2]=0.0;
		derivative_xi[1]=0.0;
		derivative_xi[2]=0.0;
		points=(Triple *)NULL;
		normalpoints=(Triple *)NULL;
		texturepoints=(Triple *)NULL;
		n_data_components=0;
		data=0;
		number_of_points=(number_of_segments_along+1)*(number_of_segments_around+1);
		if (data_field)
		{
			n_data_components = cmzn_field_get_number_of_components(data_field);
			if (!ALLOCATE(data,GLfloat,number_of_points*n_data_components))
			{
				display_message(ERROR_MESSAGE,
					"FE_element_add_cylinder_to_vertex_array.  Could allocate data");
				return_code = 0;
			}
			original_data = data;
		}
		const DsLabelIndex elementIndex = get_FE_element_index(element);
		int replaceRequired = 0;
		/* find if vertex already in the array */
		int vertex_location = array->find_first_fast_search_id_location(elementIndex);

		/* vertex found in array, check if update is required */
		if (vertex_location >= 0)
		{
			array->get_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_UPDATE_REQUIRED,
				vertex_location, 1, &replaceRequired);
		}
		if ((vertex_location < 0 || replaceRequired) &&
			(data||(!n_data_components)) &&
			(ALLOCATE(normalpoints,Triple,number_of_points)) &&
			(ALLOCATE(points,Triple,number_of_points)) &&
			(ALLOCATE(texturepoints,Triple,number_of_points)) &&
			(ALLOCATE(radius_array,FE_value,3*(number_of_segments_along+1))))
		{
			FE_value constant_radius = 0.5*base_size[0];
			FE_value scale_factor = 0.5*scale_factors[0];
			/* for selective editing of GT_object primitives, record element ID */
			if (vertex_location < 0)
			{
				array->add_fast_search_id(elementIndex);
				array->add_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_OBJECT_ID,
					1, 1, &elementIndex);
				int modificationRequired = 0;
				array->add_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_UPDATE_REQUIRED,
					1, 1, &modificationRequired);
				vertex_start = array->get_number_of_vertices(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION);
			}
			else
			{
				/* case for modification */
				array->replace_integer_vertex_buffer_at_position(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_OBJECT_ID, vertex_location, 1, 1,
					&elementIndex);
				array->get_unsigned_integer_attribute(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
					vertex_location, 1, &vertex_start);
			}
			point=points;
			derivative=normalpoints;
			texture_coordinate=texturepoints;
			/* Calculate the points and radius and data at the each point */
			FE_value *feData = new FE_value[n_data_components];
			const FE_value xiScale = 1.0 / number_of_segments_along;
			for (i=0;(i<=number_of_segments_along) && return_code;i++)
			{
				xi = i*xiScale;
				/* evaluate the fields */
				if ((CMZN_OK == field_cache->setIndexedMeshLocation(static_cast<unsigned int>(i), element, &xi, top_level_element)) &&
					(CMZN_OK == cmzn_field_evaluate_derivative(coordinate_field,
						d_dxi, field_cache, coordinate_dimension, derivative_xi)) &&
					(CMZN_OK == cmzn_field_evaluate_real(coordinate_field, field_cache,
						coordinate_dimension, coordinates)) &&
					((!data_field) || (CMZN_OK == cmzn_field_evaluate_real(data_field,
						field_cache, n_data_components, feData))) &&
					((!orientation_scale_field) || (
						(CMZN_OK == cmzn_field_evaluate_derivative(orientation_scale_field, d_dxi, field_cache, 1, &diameter_derivative)) &&
						(CMZN_OK == cmzn_field_evaluate_real(orientation_scale_field, field_cache, 1, &diameter_value)))) &&
					((!texture_coordinate_field) || (CMZN_OK == cmzn_field_evaluate_real(texture_coordinate_field, field_cache,
						texture_coordinate_dimension, tex_coordinates))))
				{
					/* store the coordinates in the point */
					(*point)[0]=(ZnReal)coordinates[0];
					(*point)[1]=(ZnReal)coordinates[1];
					(*point)[2]=(ZnReal)coordinates[2];
					/* normalize the line direction (derivative) */
					/* keep dS/dxi in the radius_array for converting derivatives later */
					dS_dxi = sqrt(derivative_xi[0]*derivative_xi[0]+
						derivative_xi[1]*derivative_xi[1]+
						derivative_xi[2]*derivative_xi[2]);
					radius_array[3*i+2] = dS_dxi;
					if (0.0 < dS_dxi)
					{
						derivative_xi[0] /= dS_dxi;
						derivative_xi[1] /= dS_dxi;
						derivative_xi[2] /= dS_dxi;
					}
					/* store the derivative in the normal */
					(*derivative)[0]=(ZnReal)derivative_xi[0];
					(*derivative)[1]=(ZnReal)derivative_xi[1];
					(*derivative)[2]=(ZnReal)derivative_xi[2];
					/* store the radius and derivative in the radius array */
					if (orientation_scale_field)
					{
						radius_array[3*i] = constant_radius + scale_factor*diameter_value;
						radius_array[3*i+1] = diameter_derivative*scale_factor;
					}
					else
					{
						radius_array[3*i] = constant_radius;
						radius_array[3*i+1] = 0.0;
					}
					/* store the data and then we are done with it */
					if (data_field)
					{
						CAST_TO_OTHER(data,feData,GLfloat,n_data_components);
						datum=data;
						for (j=number_of_segments_around;j>=0;j--)
						{
							for (k=0;k<n_data_components;k++)
							{
								data[k] = datum[k];
							}
							data+=n_data_components;
						}
					}
					/* store the first texture coordinate */
					if (texture_coordinate_field)
					{
						(*texture_coordinate)[0] = (ZnReal)(tex_coordinates[0]);
					}
					else
					{
						/* default is to use xi for the first texture coordinate */
						(*texture_coordinate)[0] = (ZnReal)(xi);
					}
				}
				else
				{
					/* error evaluating fields */
					return_code = 0;
				}
				point += number_of_segments_around+1;
				derivative += number_of_segments_around+1;
				texture_coordinate += number_of_segments_around+1;
			}
			delete[] feData;

			// get a representative size for declaring coordinate derivatives to be zero
			previous_point = points;
			point = points + (number_of_segments_around + 1);
			derivative_xi[0] = (*point)[0] - (*previous_point)[0];
			derivative_xi[1] = (*point)[1] - (*previous_point)[1];
			derivative_xi[2] = (*point)[2] - (*previous_point)[2];
			const FE_value zero_dS_dxi = 1.0E-6*(FE_value)number_of_segments_along*
				sqrt(derivative_xi[0]*derivative_xi[0] + derivative_xi[1]*derivative_xi[1] + derivative_xi[2]*derivative_xi[2]);

			if (return_code)
			{
				/* Calculate the normals at the first and last points as we must line
					up at these points so that the next elements join on correctly */
				for (i=0; i<=number_of_segments_along; i+=number_of_segments_along)
				{
					point = points + i * (number_of_segments_around+1);
					derivative = normalpoints + i * (number_of_segments_around+1);
					normal = normalpoints + i * (number_of_segments_around+1) + 1;
					derivative_xi[0] = (*derivative)[0];
					derivative_xi[1] = (*derivative)[1];
					derivative_xi[2] = (*derivative)[2];
					dS_dxi = radius_array[3*i+2];
					/* if the derivative is zero, use the change in location between
						 this and the nearest point along the element so that the normal
						 will at least remain normal to the axis of the cylinder */
					if (dS_dxi <= zero_dS_dxi)
					{
						if (0 == i)
						{
							previous_point = point;
							point = points + (i + 1) * (number_of_segments_around + 1);
						}
						else
						{
							previous_point =
								points + (i - 1) * (number_of_segments_around + 1);
						}
						derivative_xi[0] = (*point)[0] - (*previous_point)[0];
						derivative_xi[1] = (*point)[1] - (*previous_point)[1];
						derivative_xi[2] = (*point)[2] - (*previous_point)[2];
						/* normalise this pseudo derivative */
						if (0.0 < (length = sqrt(derivative_xi[0]*derivative_xi[0] +
							derivative_xi[1]*derivative_xi[1] +
							derivative_xi[2]*derivative_xi[2])))
						{
							derivative_xi[0] /= length;
							derivative_xi[1] /= length;
							derivative_xi[2] /= length;
						}
						/* put it back in the derivatives; we know it is a zero derivative
							 from the stored dS_dxi */
						(*derivative)[0] = (ZnReal)derivative_xi[0];
						(*derivative)[1] = (ZnReal)derivative_xi[1];
						(*derivative)[2] = (ZnReal)derivative_xi[2];
					}
					/* get any vector not aligned with derivative */
					jacobian[0] = 0.0;
					jacobian[1] = 0.0;
					jacobian[2] = 0.0;
					/* make jacobian have 1.0 in the component with the least absolute
						value in derivative_xi */
					if (fabs(derivative_xi[0]) < fabs(derivative_xi[1]))
					{
						if (fabs(derivative_xi[2]) < fabs(derivative_xi[0]))
						{
							jacobian[2] = 1.0;
						}
						else
						{
							jacobian[0] = 1.0;
						}
					}
					else
					{
						if (fabs(derivative_xi[2]) < fabs(derivative_xi[1]))
						{
							jacobian[2] = 1.0;
						}
						else
						{
							jacobian[1] = 1.0;
						}
					}
					/* get cross product of the derivative and this vector
						= vector normal to derivative */
					/* Put this in the normal, we don't need the derivative anymore */
					jacobian[3] =
						derivative_xi[1]*jacobian[2] - derivative_xi[2]*jacobian[1];
					jacobian[4] =
						derivative_xi[2]*jacobian[0] - derivative_xi[0]*jacobian[2];
					jacobian[5] =
						derivative_xi[0]*jacobian[1] - derivative_xi[1]*jacobian[0];
					/* make normal into a unit vector */
					if (0.0 < (length = (ZnReal)sqrt((double)(jacobian[3]*jacobian[3] +
						jacobian[4]*jacobian[4] + jacobian[5]*jacobian[5]))))
					{
						jacobian[3] /= length;
						jacobian[4] /= length;
						jacobian[5] /= length;
					}
					(*normal)[0] = (ZnReal)jacobian[3];
					(*normal)[1] = (ZnReal)jacobian[4];
					(*normal)[2] = (ZnReal)jacobian[5];
				}
				end_aligned_normal[0] = (*normal)[0];
				end_aligned_normal[1] = (*normal)[1];
				end_aligned_normal[2] = (*normal)[2];

				/* Propogate the first normal along the segments keeping it in
					the same plane each step */
				for (i=1;i<=number_of_segments_along;i++)
				{
					point = points + i * (number_of_segments_around+1);
					derivative = normalpoints + i * (number_of_segments_around+1);
					normal = normalpoints + i * (number_of_segments_around+1) + 1;
					previous_point = points + (i-1) * (number_of_segments_around+1);
					previous_normal = normalpoints + (i-1) * (number_of_segments_around+1) + 1;

					/* Get the change in position */
					jacobian[0] = (*point)[0] - (*previous_point)[0];
					jacobian[1] = (*point)[1] - (*previous_point)[1];
					jacobian[2] = (*point)[2] - (*previous_point)[2];

					/* Get the normal to plane which contains change_in_position
						vector and previous_normal_vector */
					jacobian[3] = jacobian[1] * (*previous_normal)[2] -
						jacobian[2] * (*previous_normal)[1];
					jacobian[4] = jacobian[2] * (*previous_normal)[0] -
						jacobian[0] * (*previous_normal)[2];
					jacobian[5] = jacobian[0] * (*previous_normal)[1] -
						jacobian[1] * (*previous_normal)[0];

					/* Get the new normal vector which lies in the plane */
					jacobian[0] = jacobian[4] * (*derivative)[2] -
						jacobian[5] * (*derivative)[1];
					jacobian[1] = jacobian[5] * (*derivative)[0] -
						jacobian[3] * (*derivative)[2];
					jacobian[2] = jacobian[3] * (*derivative)[1] -
						jacobian[4] * (*derivative)[0];

					/* Store this in the other normal space and normalise */
					(*normal)[0] = (ZnReal)jacobian[0];
					(*normal)[1] = (ZnReal)jacobian[1];
					(*normal)[2] = (ZnReal)jacobian[2];
					if (0.0<(distance=
						sqrt((*normal)[0]*(*normal)[0] + (*normal)[1]*(*normal)[1]+
							(*normal)[2]*(*normal)[2])))
					{
						(*normal)[0]/=(ZnReal)distance;
						(*normal)[1]/=(ZnReal)distance;
						(*normal)[2]/=(ZnReal)distance;
					}
				}

				/* Find the closest correlation between the end_aligned_normal and the
					propogated normal */
				derivative = normalpoints + number_of_segments_along * (number_of_segments_around+1);
				normal = normalpoints + number_of_segments_along * (number_of_segments_around+1) + 1;

				/* calculate theta, the angle from the end_aligned_normal to the
					 propagated normal in a right hand sense about the derivative */
				jacobian[0] = end_aligned_normal[0];
				jacobian[1] = end_aligned_normal[1];
				jacobian[2] = end_aligned_normal[2];

				jacobian[3]= (*derivative)[1]*jacobian[2]-(*derivative)[2]*jacobian[1];
				jacobian[4]= (*derivative)[2]*jacobian[0]-(*derivative)[0]*jacobian[2];
				jacobian[5]= (*derivative)[0]*jacobian[1]-(*derivative)[1]*jacobian[0];

				x = (*normal)[0] * jacobian[0] +
					(*normal)[1] * jacobian[1] +
					(*normal)[2] * jacobian[2];
				y = (*normal)[0] * jacobian[3] +
					(*normal)[1] * jacobian[4] +
					(*normal)[2] * jacobian[5];
				theta = atan2(y, x);
				if (theta < 0.0)
				{
					theta += 2*PI;
				}
				facet_angle = 2*PI/number_of_segments_around;
				/* calculate the number of times facet_angle can occur before theta */
				facet_offset = (int)(theta / facet_angle);
				/* get angle from the next lowest whole facet to propagated normal */
				theta -= facet_offset*facet_angle;
				/* nearest facet could be on the otherside of the propagated normal so
					 handle this case; theta_change to nearest facet has opposite sign */
				if (theta > 0.5*facet_angle)
				{
					theta_change = facet_angle - theta;
					facet_offset++;
				}
				else
				{
					theta_change = -theta;
				}

				/* Calculate the actual points and normals */
				for (i=0;(i<=number_of_segments_along);i++)
				{
					/* Get the two normals */
					point = points + i * (number_of_segments_around+1);
					derivative = normalpoints + i * (number_of_segments_around+1);
					normal = normalpoints + i * (number_of_segments_around+1) + 1;

					if (i < number_of_segments_along)
					{
						jacobian[0] = (*normal)[0];
						jacobian[1] = (*normal)[1];
						jacobian[2] = (*normal)[2];
					}
					else
					{
						jacobian[0] = end_aligned_normal[0];
						jacobian[1] = end_aligned_normal[1];
						jacobian[2] = end_aligned_normal[2];
					}

					derivative_xi[0] = (*derivative)[0];
					derivative_xi[1] = (*derivative)[1];
					derivative_xi[2] = (*derivative)[2];

					jacobian[3]=derivative_xi[1]*jacobian[2]-derivative_xi[2]*jacobian[1];
					jacobian[4]=derivative_xi[2]*jacobian[0]-derivative_xi[0]*jacobian[2];
					jacobian[5]=derivative_xi[0]*jacobian[1]-derivative_xi[1]*jacobian[0];

					/* Get the other stored values */
					FE_value radius_value = radius_array[3*i];
					FE_value radius_derivative = radius_array[3*i+1];
					dS_dxi = radius_array[3*i+2];

					/* Write the true normals and positions */
					normal = normalpoints + i * (number_of_segments_around+1);
					for (j=number_of_segments_around;0<=j;j--)
					{
						if (i < number_of_segments_along)
						{
							theta = theta_change * ((ZnReal) i)/ ((ZnReal)number_of_segments_along)
								+ PI*2.0*((ZnReal)j)/((ZnReal)number_of_segments_around);
						}
						else
						{
							theta = PI*2.*((ZnReal)(j + facet_offset))/
								((ZnReal)number_of_segments_around);
						}
						cos_theta=cos(theta);
						sin_theta=sin(theta);
						(normal[j])[0]=ZnReal(cos_theta*jacobian[0]+sin_theta*jacobian[3]);
						(normal[j])[1]=ZnReal(cos_theta*jacobian[1]+sin_theta*jacobian[4]);
						(normal[j])[2]=ZnReal(cos_theta*jacobian[2]+sin_theta*jacobian[5]);
						(point[j])[0]=(point[0])[0]+ZnReal(radius_value*(normal[j])[0]);
						(point[j])[1]=(point[0])[1]+ZnReal(radius_value*(normal[j])[1]);
						(point[j])[2]=(point[0])[2]+ZnReal(radius_value*(normal[j])[2]);
						if (orientation_scale_field && (0.0 != radius_derivative))
						{
							if (0.0 < dS_dxi)
							{
								(normal[j])[0] -= ZnReal((radius_derivative/dS_dxi)*derivative_xi[0]);
								(normal[j])[1] -= ZnReal((radius_derivative/dS_dxi)*derivative_xi[1]);
								(normal[j])[2] -= ZnReal((radius_derivative/dS_dxi)*derivative_xi[2]);
							}
							else
							{
								/* a finite change of radius is happening in an infinitessimal
									 space. Hence, make normal aligned with derivative */
								if (radius_derivative < 0.0)
								{
									(normal[j])[0] = ZnReal(derivative_xi[0]);
									(normal[j])[1] = ZnReal(derivative_xi[1]);
									(normal[j])[2] = ZnReal(derivative_xi[2]);
								}
								else
								{
									(normal[j])[0] = -ZnReal(derivative_xi[0]);
									(normal[j])[1] = -ZnReal(derivative_xi[1]);
									(normal[j])[2] = -ZnReal(derivative_xi[2]);
								}
							}
						}
					}
				}
			}
			if (return_code)
			{
				GLfloat floatField[3];
				unsigned int number_of_vertices = number_of_points;
				unsigned int number_of_xi1 = number_of_segments_around+1;
				unsigned int number_of_xi2 = number_of_segments_along+1;
				int polygonType = (int)g_TRIANGLE;
				if (vertex_location < 0)
				{
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
					array->fill_element_index(vertex_start, number_of_xi1, number_of_xi2,
						ARRAY_SHAPE_TYPE_UNSPECIFIED);
				}
				/* normalize the normals */
				normal=normalpoints;
				point = points;
				datum = original_data;
				unsigned int offset = 0;
				for (i=number_of_points;i>0;i--)
				{
					offset = number_of_points - i;
					normal_1=(*normal)[0];
					normal_2=(*normal)[1];
					normal_3=(*normal)[2];
					if (0.0<(distance=
						sqrt(normal_1*normal_1+normal_2*normal_2+normal_3*normal_3)))
					{
						(*normal)[0]=ZnReal(normal_1/distance);
						(*normal)[1]=ZnReal(normal_2/distance);
						(*normal)[2]=ZnReal(normal_3/distance);
					}
					floatField[0] = (GLfloat)(*normal)[0];
					floatField[1] = (GLfloat)(*normal)[1];
					floatField[2] = (GLfloat)(*normal)[2];
					if (vertex_location < 0)
					{
						array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
							3, 1, floatField);
					}
					else
					{
						array->replace_float_vertex_buffer_at_position(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
							vertex_start + offset, 3, 1, floatField);
					}
					normal++;
					floatField[0] = (GLfloat)(*point)[0];
					floatField[1] = (GLfloat)(*point)[1];
					floatField[2] = (GLfloat)(*point)[2];
					if (vertex_location < 0)
					{
						array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
							3, 1, floatField);
					}
					else
					{
						array->replace_float_vertex_buffer_at_position(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
							vertex_start + offset, 3, 1, floatField);
					}
					point++;
					if (data_field && data)
					{
						if (vertex_location < 0)
						{
							array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
								n_data_components, 1, datum);
						}
						else
						{
							array->replace_float_vertex_buffer_at_position(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
								vertex_start + offset, n_data_components, 1, datum);
						}
						datum += n_data_components;
					}

				}
				/* calculate the texture coordinates */
				/* the texture coordinate along the length has been set above but must
				   be propagated to vertices around the cylinder. The second texture
				   coordinate ranges from from 0 to 1 around the circumference */
				texture_coordinate = texturepoints;
				for (i = 0; i <= number_of_segments_along; i++)
				{
					texture_coordinate1 = (*texture_coordinate)[0];
					for (j = 0; j <= number_of_segments_around; j++)
					{
						floatField[0] = (GLfloat)texture_coordinate1;
						floatField[1] =
							(GLfloat)j / (GLfloat)number_of_segments_around;
						floatField[2] = (GLfloat)0.0;
						if (vertex_location < 0)
						{
							array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_TEXTURE_COORDINATE_ZERO,
								3, 1, floatField);
						}
						else
						{
							array->replace_float_vertex_buffer_at_position(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_TEXTURE_COORDINATE_ZERO,
								vertex_start + offset * number_of_segments_around + j, 3, 1, floatField);
						}
						texture_coordinate++;
					}
				}
			}
			if (radius_array)
				DEALLOCATE(radius_array);
			if (points)
				DEALLOCATE(points);
			if (normalpoints)
				DEALLOCATE(normalpoints);
			if (texturepoints)
				DEALLOCATE(texturepoints);
			if (original_data)
				DEALLOCATE(original_data);
			if (replaceRequired)
			{
				int updated = 0;
				array->replace_integer_vertex_buffer_at_position(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_UPDATE_REQUIRED,
					vertex_location, 1, 1, &updated);
			}
		}
		cmzn_differentialoperator_destroy(&d_dxi);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_element_add_cylinder_to_vertex_array.  Invalid argument(s)");
	}

	return (return_code);
}

int get_surface_element_segmentation(struct FE_element *element,
	int number_of_segments_in_xi1_requested,
	int number_of_segments_in_xi2_requested,
	int *number_of_points_in_xi1,int *number_of_points_in_xi2,
	int *number_of_points,int *number_of_polygon_vertices,
	gtPolygonType *polygon_type,enum Collapsed_element_type *collapsed_element,
	enum FE_element_shape_type *shape_type_address)
{
	const FE_mesh *fe_mesh = element->getMesh();
	if (!fe_mesh)
		return 0;
	DsLabelIndex elementIndex = get_FE_element_index(element);
	const FE_mesh::ElementShapeFaces *elementShapeFaces = fe_mesh->getElementShapeFacesConst(elementIndex);
	if (!elementShapeFaces)
		return 0;
	FE_element_shape *element_shape = elementShapeFaces->getElementShape();
	int return_code = 0;
	if (element_shape && (2 == get_FE_element_shape_dimension(element_shape)) && shape_type_address)
	{
		if (get_FE_element_shape_xi_shape_type(element_shape, /*xi_number*/0,
			shape_type_address))
		{
			return_code = 1;
			*collapsed_element = ELEMENT_NOT_COLLAPSED;
			*number_of_polygon_vertices = 0;
			switch (*shape_type_address)
			{
				case POLYGON_SHAPE:
				{
					/* polygon */
					if (get_FE_element_shape_xi_linkage_number(element_shape,
						/*xi_number1*/0, /*xi_number2*/1, number_of_polygon_vertices) &&
						(2 < *number_of_polygon_vertices))
					{
						*number_of_points_in_xi1=((number_of_segments_in_xi1_requested)/
							(*number_of_polygon_vertices)+1)*(*number_of_polygon_vertices)+1;
						*collapsed_element=ELEMENT_COLLAPSED_XI2_0;
						*number_of_points_in_xi2=number_of_segments_in_xi2_requested+1;
						*number_of_points =
							*number_of_points_in_xi1*(*number_of_points_in_xi2);
						*polygon_type=g_TRIANGLE;
					}
					else
					{
						return_code = 0;
					}
				} break;
				case SIMPLEX_SHAPE:
				{
					/* simplex */
					if (number_of_segments_in_xi1_requested >
						number_of_segments_in_xi2_requested)
					{
						*number_of_points_in_xi1=number_of_segments_in_xi1_requested+1;
						*number_of_points_in_xi2=number_of_segments_in_xi1_requested+1;
					}
					else
					{
						*number_of_points_in_xi1=number_of_segments_in_xi2_requested+1;
						*number_of_points_in_xi2=number_of_segments_in_xi2_requested+1;
					}
					*number_of_points=
						(*number_of_points_in_xi1*(*number_of_points_in_xi1+1))/2;
					*polygon_type=g_TRIANGLE;
				} break;
				default:
				{
					*number_of_points_in_xi1 = number_of_segments_in_xi1_requested + 1;
					const int faceCount = elementShapeFaces->getFaceCount();
					const DsLabelIndex *faces = elementShapeFaces->getElementFaces(elementIndex);

					/* check for collapsed elements */
					if ((LINE_SHAPE == (*shape_type_address)) && (4 == faceCount) && (faces))
					{
						int validFacesCount = 0;
						for (int i = 0; i < faceCount; ++i)
							if (faces[i] >= 0)
								++validFacesCount;
						if (3 == validFacesCount)
						{
							if (faces[0] < 0)
								*collapsed_element=ELEMENT_COLLAPSED_XI1_0;
							else if (faces[1] < 0)
								*collapsed_element=ELEMENT_COLLAPSED_XI1_1;
							else if (faces[2] < 0)
								*collapsed_element=ELEMENT_COLLAPSED_XI2_0;
							else if (faces[3] < 0)
								*collapsed_element=ELEMENT_COLLAPSED_XI2_1;
						}
					}
					*number_of_points_in_xi2=number_of_segments_in_xi2_requested+1;
					*number_of_points =
						(*number_of_points_in_xi1)*(*number_of_points_in_xi2);
					*polygon_type = g_TRIANGLE;
				} break;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"get_surface_element_segmentation.  Could not get shape type");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"get_surface_element_segmentation.  Invalid argument(s)");
	}
	return (return_code);
}


int FE_element_add_surface_to_vertex_array(struct FE_element *element,
	cmzn_fieldcache_id field_cache, cmzn_mesh_id surface_mesh,
	struct Graphics_vertex_array *array,
	struct Computed_field *coordinate_field,
	struct Computed_field *texture_coordinate_field,
	struct Computed_field *data_field,
	unsigned int number_of_segments_in_xi1_requested,
	unsigned int number_of_segments_in_xi2_requested,
	char reverse_normals, struct FE_element *top_level_element)
{
	char modified_reverse_normals, special_normals;
	enum Collapsed_element_type collapsed_element;
	enum FE_element_shape_type shape_type;
	FE_value coordinates[3], derivative_xi1[3], derivative_xi2[3],
		texture_values[3], texture_derivative_xi1[3], texture_derivative_xi2[3],
		texture_determinant;
	gtPolygonType polygon_type;
	int calculate_tangent_points, i,j,n_data_components,number_of_points,
		number_of_points_in_xi1,number_of_points_in_xi2,number_of_polygon_vertices,
		return_code;
	Triple *normal, *normalpoints, *tangent = NULL, *tangentpoints, temp_normal;
	unsigned int vertex_start;
	GLfloat floatField[3];
	return_code = 1;

	int coordinate_dimension = cmzn_field_get_number_of_components(coordinate_field);
	int texture_coordinate_dimension = texture_coordinate_field ?
		cmzn_field_get_number_of_components(texture_coordinate_field) : 0;
	if (element && field_cache && surface_mesh && (2 == get_FE_element_dimension(element)) &&
		(0<number_of_segments_in_xi1_requested)&&
		(0<number_of_segments_in_xi2_requested)&&
		(0 < coordinate_dimension) && (3 >= coordinate_dimension) &&
		((!texture_coordinate_field) || (3 >= texture_coordinate_dimension)))
	{
		cmzn_differentialoperator_id d_dxi1 = cmzn_mesh_get_chart_differentialoperator(surface_mesh, /*order*/1, 1);
		cmzn_differentialoperator_id d_dxi2 = cmzn_mesh_get_chart_differentialoperator(surface_mesh, /*order*/1, 2);
		modified_reverse_normals = reverse_normals;
		const bool reverse_winding = FE_element_is_exterior_face_with_inward_normal(element);
		if (reverse_winding)
		{
			modified_reverse_normals = !modified_reverse_normals;
		}
		/* clear coordinates and derivatives not set if coordinate field is not
			 3 component */
		coordinates[1]=0.0;
		coordinates[2]=0.0;
		derivative_xi1[1]=0.0;
		derivative_xi1[2]=0.0;
		derivative_xi2[1]=0.0;
		derivative_xi2[2]=0.0;
		/* clear texture_values not set if texture_coordinate field is not
			 3 component */
		texture_values[1]=0.0;
		texture_values[2]=0.0;
		texture_derivative_xi1[1]=0.0;
		texture_derivative_xi1[2]=0.0;
		texture_derivative_xi2[1]=0.0;
		texture_derivative_xi2[2]=0.0;
		tangentpoints=(Triple *)NULL;
		normalpoints=(Triple *)NULL;
		return_code = get_surface_element_segmentation(element,
			number_of_segments_in_xi1_requested,number_of_segments_in_xi2_requested,
			&number_of_points_in_xi1,&number_of_points_in_xi2,
			&number_of_points,&number_of_polygon_vertices,&polygon_type,
			&collapsed_element, &shape_type);
		n_data_components = 0;
		if (data_field)
		{
			n_data_components = cmzn_field_get_number_of_components(data_field);
		}
		const DsLabelIndex elementIndex = get_FE_element_index(element);
		GLfloat *floatData = data_field ? new GLfloat[n_data_components] : 0;
		FE_value *xi_points = new FE_value[2*number_of_points];
		int replaceRequired = 0;
		/* find if vertex already in the array */
		int vertex_location = array->find_first_fast_search_id_location(elementIndex);
		/* vertex found in array, check if update is required */
		if (vertex_location >= 0)
		{
			array->get_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_UPDATE_REQUIRED,
				vertex_location, 1, &replaceRequired);
		}
		if ((vertex_location < 0 || replaceRequired) && (NULL != xi_points) &&
			(floatData || (0 == n_data_components)) &&
			(ALLOCATE(normalpoints,Triple,number_of_points)) &&
			(!texture_coordinate_field || (ALLOCATE(tangentpoints,Triple,number_of_points))))
		{
			FE_value *feData = data_field ? new FE_value[n_data_components] : 0;
			/* for selective editing of GT_object primitives, record element ID */
			if (vertex_location < 0)
			{
				array->add_fast_search_id(elementIndex);
				array->add_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_OBJECT_ID,
					1, 1, &elementIndex);
				vertex_start = array->get_number_of_vertices(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION);
				int modificationRequired = 0;
				array->add_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_UPDATE_REQUIRED,
					1, 1, &modificationRequired);
			}
			else
			{
				array->replace_integer_vertex_buffer_at_position(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_OBJECT_ID, vertex_location, 1, 1,
					&elementIndex);
				array->get_unsigned_integer_attribute(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
					vertex_location, 1, &vertex_start);
			}

			/* calculate the xi coordinates and store in xi_points array */
			const FE_value xi_distance1 = (FE_value)(number_of_points_in_xi1 - 1);
			const FE_value xi_distance2 = (FE_value)(number_of_points_in_xi2 - 1);
			FE_value *xi1, xi1_value, *xi2, xi2_value;
			if (SIMPLEX_SHAPE == shape_type)
			{
				/* Standard triangle case lists points in order:
				 *   6
				 *   |\
				 *   4-5
				 *   |\|\
				 *   1-2-3
				 * Reverse winding reverses xi1 coordinate in rows: 3,2,1,5,4,6
				 */
				if (reverse_winding)
				{
					for (i = 0; i < number_of_points_in_xi1; i++)
					{
						xi1_value = (FE_value)(number_of_points_in_xi1 - 1 - i)/xi_distance1;
						xi1 = xi_points + 2*i;
						for (j = 0; j <= i; j++)
						{
							*xi1 = xi1_value;
							xi1 += 2*(number_of_points_in_xi2 - 1 - j);
						}
					}
				}
				else
				{
					for (i = 0; i < number_of_points_in_xi1; i++)
					{
						xi1_value = (FE_value)i/xi_distance1;
						xi1 = xi_points + 2*i;
						for (j = 0; j < number_of_points_in_xi2 - i; j++)
						{
							*xi1 = xi1_value;
							xi1 += 2*(number_of_points_in_xi1 - j);
						}
					}
				}
				xi2 = xi_points + 1;
				for (j = 0; j < number_of_points_in_xi2; j++)
				{
					xi2_value = (FE_value)j/xi_distance2;
					for (i = number_of_points_in_xi1 - j; i > 0; i--)
					{
						*xi2 = xi2_value;
						xi2 += 2;
					}
				}
			}
			else
			{
				/* Standard quadrilateral case lists points in order:
				 *   7-8-9
				 *   | | |
				 *   4-5-6
				 *   | | |
				 *   1-2-3
				 * Reverse winding reverses xi1 coordinate in rows: 3,2,1,6,5,4,9,8,7
				 */
				for (i = 0; i < number_of_points_in_xi1; i++)
				{
					if (reverse_winding)
					{
						xi1_value = (FE_value)(number_of_points_in_xi1 - 1 - i)/xi_distance1;
					}
					else
					{
						xi1_value = (FE_value)i/xi_distance1;
					}
					xi1 = xi_points + 2*i;
					for (j = 0; j < number_of_points_in_xi2; j++)
					{
						*xi1 = xi1_value;
						xi1 += 2*number_of_points_in_xi1;
					}
				}
				xi2 = xi_points + 1;
				for (j = 0; j < number_of_points_in_xi2; j++)
				{
					xi2_value = (FE_value)j/xi_distance2;
					for (i = number_of_points_in_xi1; i > 0 ; i--)
					{
						*xi2 = xi2_value;
						xi2 += 2;
					}
				}
			}
			/* calculate the points, normals and data */
			normal=normalpoints;
			calculate_tangent_points = 0;
			if (texture_coordinate_field)
			{
				calculate_tangent_points = 1;
				tangent=tangentpoints;
			}
			if ((SIMPLEX_SHAPE!=shape_type)&&
				((ELEMENT_COLLAPSED_XI1_0==collapsed_element)||
				(ELEMENT_COLLAPSED_XI1_1==collapsed_element)||
				(ELEMENT_COLLAPSED_XI2_0==collapsed_element)||
				(ELEMENT_COLLAPSED_XI2_1==collapsed_element)))
			{
				special_normals=1;
			}
			else
			{
				special_normals=0;
			}
			const FE_value special_normal_sign = reverse_winding ? -1.0 : 1.0;
			i=0;
			FE_value *xi = xi_points;
			while ((i<number_of_points)&&return_code)
			{
				return_code = (CMZN_OK == field_cache->setIndexedMeshLocation(static_cast<unsigned int>(i), element, xi, top_level_element));
				/* evaluate the fields */
				if ((CMZN_OK != cmzn_field_evaluate_derivative(coordinate_field,
						d_dxi1, field_cache, coordinate_dimension, derivative_xi1)) ||
					(CMZN_OK != cmzn_field_evaluate_derivative(coordinate_field,
						d_dxi2, field_cache, coordinate_dimension, derivative_xi2)) ||
					(CMZN_OK != cmzn_field_evaluate_real(coordinate_field, field_cache,
						coordinate_dimension, coordinates)))
				{
					return_code = 0;
				}
				if (data_field)
				{
					if (CMZN_OK != cmzn_field_evaluate_real(data_field, field_cache, n_data_components, feData))
					{
						return_code = 0;
					}
				}
				if (texture_coordinate_field)
				{
					if (calculate_tangent_points)
					{
						if ((CMZN_OK != cmzn_field_evaluate_derivative(texture_coordinate_field,
								d_dxi1, field_cache, texture_coordinate_dimension, texture_derivative_xi1) ||
							(CMZN_OK != cmzn_field_evaluate_derivative(texture_coordinate_field,
								d_dxi2, field_cache, texture_coordinate_dimension, texture_derivative_xi2)) ||
							(CMZN_OK != cmzn_field_evaluate_real(texture_coordinate_field, field_cache,
								texture_coordinate_dimension, texture_values))))
						{
							calculate_tangent_points = 0;
							display_message(WARNING_MESSAGE,
								"Texture coordinate field derivatives are unavailable, "
								"continuing but not calculating tangent coordinates for displacement mapping.");
						}
					}
					if (!calculate_tangent_points)  /* Do this if just unset above as well as else */
					{
						return_code = (CMZN_OK == cmzn_field_evaluate_real(texture_coordinate_field,
							field_cache, texture_coordinate_dimension, texture_values)) ? 1 : 0;
					}
				}
				if (return_code)
				{
					CAST_TO_OTHER(floatField,coordinates,GLfloat,coordinate_dimension);
					if (vertex_location < 0)
					{
						array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
							coordinate_dimension, 1, floatField);
					}
					else
					{
						array->replace_float_vertex_buffer_at_position(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
							vertex_start + i, coordinate_dimension, 1, floatField);
					}
					(*normal)[0] = ZnReal(derivative_xi1[1]*derivative_xi2[2] - derivative_xi2[1]*derivative_xi1[2]);
					(*normal)[1] = ZnReal(derivative_xi1[2]*derivative_xi2[0] - derivative_xi2[2]*derivative_xi1[0]);
					(*normal)[2] = ZnReal(derivative_xi1[0]*derivative_xi2[1] - derivative_xi2[0]*derivative_xi1[1]);
					if (texture_coordinate_field)
					{
						if (calculate_tangent_points)
						{
							/* tangent is dX/d_xi * inv(dT/dxi) */
							texture_determinant = texture_derivative_xi1[0] * texture_derivative_xi2[1]
								- texture_derivative_xi2[0] * texture_derivative_xi1[1];
							if ((texture_determinant < FE_VALUE_ZERO_TOLERANCE) &&
								(texture_determinant > -FE_VALUE_ZERO_TOLERANCE))
							{
								/* Cannot invert the texture derivative so just use the first xi derivative */
								(*tangent)[0] = ZnReal(derivative_xi1[0]);
								(*tangent)[1] = ZnReal(derivative_xi1[1]);
								(*tangent)[2] = ZnReal(derivative_xi1[2]);
							}
							else
							{
								(*tangent)[0] = ZnReal((derivative_xi1[0] * texture_derivative_xi1[0]
									- derivative_xi2[0] * texture_derivative_xi1[1]) / texture_determinant);
								(*tangent)[1] = ZnReal((derivative_xi1[1] * texture_derivative_xi1[0]
									- derivative_xi2[1] * texture_derivative_xi1[1]) / texture_determinant);
								(*tangent)[2] = ZnReal((derivative_xi1[2] * texture_derivative_xi1[0]
									- derivative_xi2[2] * texture_derivative_xi1[1]) / texture_determinant);
							}
						}
						else
						{
							(*tangent)[0] = 0.0;
							(*tangent)[1] = 0.0;
							(*tangent)[2] = 0.0;
						}
						tangent++;
					}
					/* calculate the normals */
					/* calculate the normal=d/d_xi1 x d/d_xi2 */
					if (special_normals)
					{
						if (((ELEMENT_COLLAPSED_XI1_0==collapsed_element) && (!reverse_winding)) ||
							((ELEMENT_COLLAPSED_XI1_1==collapsed_element) && reverse_winding))
						{
							if (0==i%number_of_points_in_xi1)
							{
								/* save xi1 derivatives, get normal from cross product of
									these */
								(*normal)[0] = ZnReal(derivative_xi1[0]);
								(*normal)[1] = ZnReal(derivative_xi1[1]);
								(*normal)[2] = ZnReal(derivative_xi1[2]);
							}
						}
						else if (ELEMENT_COLLAPSED_XI2_0==collapsed_element)
						{
							if (0==i/number_of_points_in_xi1)
							{
								/* save xi2 derivatives, get normal from cross product of
									these */
								(*normal)[0] = ZnReal(derivative_xi2[0]);
								(*normal)[1] = ZnReal(derivative_xi2[1]);
								(*normal)[2] = ZnReal(derivative_xi2[2]);
							}
						}
						else if (((ELEMENT_COLLAPSED_XI1_1==collapsed_element) && (!reverse_winding)) ||
							((ELEMENT_COLLAPSED_XI1_0==collapsed_element) && reverse_winding))
						{
							if (0 == (i + 1)%number_of_points_in_xi1)
							{
								/* save xi1 derivatives, get normal from cross product of
									these */
								(*normal)[0] = ZnReal(derivative_xi1[0]);
								(*normal)[1] = ZnReal(derivative_xi1[1]);
								(*normal)[2] = ZnReal(derivative_xi1[2]);
							}
						}
						else if (ELEMENT_COLLAPSED_XI2_1==collapsed_element)
						{
							if (number_of_points_in_xi2-1==i/number_of_points_in_xi1)
							{
								/* save xi2 derivatives, get normal from cross product of
									these */
								(*normal)[0] = ZnReal(derivative_xi2[0]);
								(*normal)[1] = ZnReal(derivative_xi2[1]);
								(*normal)[2] = ZnReal(derivative_xi2[2]);
							}
						}
					}
					normal++;
					if (data_field)
					{
						CAST_TO_OTHER(floatData,feData,GLfloat,n_data_components);
						if (vertex_location < 0)
						{
							array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
								n_data_components, 1, floatData);
						}
						else
						{
							array->replace_float_vertex_buffer_at_position(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
								vertex_start + i, n_data_components, 1, floatData);
						}
					}
					if (texture_coordinate_field)
					{
						floatField[0] = GLfloat(texture_values[0]);
						floatField[1] = GLfloat(texture_values[1]);
						floatField[2] = GLfloat(texture_values[2]);
						if (vertex_location < 0)
						{
							array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_TEXTURE_COORDINATE_ZERO,
								texture_coordinate_dimension, 1, floatField);
						}
						else
						{
							array->replace_float_vertex_buffer_at_position(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_TEXTURE_COORDINATE_ZERO,
								vertex_start + i, texture_coordinate_dimension, 1, floatField);
						}
					}
				}
				xi += 2;
				i++;
			}
			if (return_code)
			{
				if (special_normals)
				{
					if (number_of_polygon_vertices>0)
					{
						normal=normalpoints+number_of_points_in_xi1-2;
						derivative_xi2[0]=(*normal)[0];
						derivative_xi2[1]=(*normal)[1];
						derivative_xi2[2]=(*normal)[2];
						normal=normalpoints;
						for (i=number_of_points_in_xi1;i>0;i--)
						{
							derivative_xi1[0]=(*normal)[0];
							derivative_xi1[1]=(*normal)[1];
							derivative_xi1[2]=(*normal)[2];
							(*normal)[0] = ZnReal(special_normal_sign*(derivative_xi1[1]*derivative_xi2[2] -
								derivative_xi2[1]*derivative_xi1[2]));
							(*normal)[1] = ZnReal(special_normal_sign*(derivative_xi1[2]*derivative_xi2[0] -
								derivative_xi2[2]*derivative_xi1[0]));
							(*normal)[2] = ZnReal(special_normal_sign*(derivative_xi1[0]*derivative_xi2[1] -
								derivative_xi2[0]*derivative_xi1[1]));
							derivative_xi2[0]=derivative_xi1[0];
							derivative_xi2[1]=derivative_xi1[1];
							derivative_xi2[2]=derivative_xi1[2];
							normal++;
						}
					}
					else if (((ELEMENT_COLLAPSED_XI1_0==collapsed_element) && (!reverse_winding)) ||
						((ELEMENT_COLLAPSED_XI1_1==collapsed_element) && reverse_winding))
					{
						/* calculate the normals for the xi1=0 edge */
						normal=normalpoints+((number_of_points_in_xi2-1)*
							number_of_points_in_xi1);
						derivative_xi1[0]=(*normal)[0];
						derivative_xi1[1]=(*normal)[1];
						derivative_xi1[2]=(*normal)[2];
						normal=normalpoints;
						derivative_xi2[0]=(*normal)[0];
						derivative_xi2[1]=(*normal)[1];
						derivative_xi2[2]=(*normal)[2];
						temp_normal[0] = (ZnReal)(special_normal_sign*
							(derivative_xi2[1]*derivative_xi1[2] - derivative_xi1[1]*derivative_xi2[2]));
						temp_normal[1] = (ZnReal)(special_normal_sign*
							(derivative_xi2[2]*derivative_xi1[0] - derivative_xi1[2]*derivative_xi2[0]));
						temp_normal[2] = (ZnReal)(special_normal_sign*
							(derivative_xi2[0]*derivative_xi1[1] - derivative_xi1[0]*derivative_xi2[1]));
						for (i=number_of_points_in_xi2;i>0;i--)
						{
							(*normal)[0]=temp_normal[0];
							(*normal)[1]=temp_normal[1];
							(*normal)[2]=temp_normal[2];
							normal += number_of_points_in_xi1;
						}
					}
					else if (ELEMENT_COLLAPSED_XI2_0==collapsed_element)
					{
						/* calculate the normals for the xi2=0 edge */
						normal=normalpoints+(number_of_points_in_xi1-1);
						derivative_xi1[0]=(*normal)[0];
						derivative_xi1[1]=(*normal)[1];
						derivative_xi1[2]=(*normal)[2];
						normal=normalpoints;
						derivative_xi2[0]=(*normal)[0];
						derivative_xi2[1]=(*normal)[1];
						derivative_xi2[2]=(*normal)[2];
						temp_normal[0] = (ZnReal)(special_normal_sign*
							(derivative_xi1[1]*derivative_xi2[2] - derivative_xi2[1]*derivative_xi1[2]));
						temp_normal[1] = (ZnReal)(special_normal_sign*
							(derivative_xi1[2]*derivative_xi2[0] - derivative_xi2[2]*derivative_xi1[0]));
						temp_normal[2] = (ZnReal)(special_normal_sign*
							(derivative_xi1[0]*derivative_xi2[1] - derivative_xi2[0]*derivative_xi1[1]));
						for (i=number_of_points_in_xi1;i>0;i--)
						{
							(*normal)[0]=temp_normal[0];
							(*normal)[1]=temp_normal[1];
							(*normal)[2]=temp_normal[2];
							normal++;
						}
					}
					else if (((ELEMENT_COLLAPSED_XI1_1==collapsed_element) && (!reverse_winding)) ||
						((ELEMENT_COLLAPSED_XI1_0==collapsed_element) && reverse_winding))
					{
						/* calculate the normals for the xi1=1 edge */
						normal = normalpoints +
							(number_of_points_in_xi1*number_of_points_in_xi2 - 1);
						derivative_xi2[0]=(*normal)[0];
						derivative_xi2[1]=(*normal)[1];
						derivative_xi2[2]=(*normal)[2];
						normal = normalpoints + (number_of_points_in_xi1 - 1);
						derivative_xi1[0]=(*normal)[0];
						derivative_xi1[1]=(*normal)[1];
						derivative_xi1[2]=(*normal)[2];
						temp_normal[0] = (ZnReal)(special_normal_sign*
							(derivative_xi2[1]*derivative_xi1[2] - derivative_xi1[1]*derivative_xi2[2]));
						temp_normal[1] = (ZnReal)(special_normal_sign*
							(derivative_xi2[2]*derivative_xi1[0] - derivative_xi1[2]*derivative_xi2[0]));
						temp_normal[2] = (ZnReal)(special_normal_sign*
							(derivative_xi2[0]*derivative_xi1[1] - derivative_xi1[0]*derivative_xi2[1]));
						for (i=number_of_points_in_xi2;i>0;i--)
						{
							(*normal)[0]=temp_normal[0];
							(*normal)[1]=temp_normal[1];
							(*normal)[2]=temp_normal[2];
							normal += number_of_points_in_xi1;
						}
					}
					else if (ELEMENT_COLLAPSED_XI2_1==collapsed_element)
					{
						/* calculate the normals for the xi2=1 edge */
						normal=normalpoints+(number_of_points_in_xi1*
							number_of_points_in_xi2-1);
						derivative_xi2[0]=(*normal)[0];
						derivative_xi2[1]=(*normal)[1];
						derivative_xi2[2]=(*normal)[2];
						normal=normalpoints+(number_of_points_in_xi1*
							(number_of_points_in_xi2-1));
						derivative_xi1[0]=(*normal)[0];
						derivative_xi1[1]=(*normal)[1];
						derivative_xi1[2]=(*normal)[2];
						temp_normal[0] = (ZnReal)(special_normal_sign*
							(derivative_xi1[1]*derivative_xi2[2] - derivative_xi2[1]*derivative_xi1[2]));
						temp_normal[1] = (ZnReal)(special_normal_sign*
							(derivative_xi1[2]*derivative_xi2[0] - derivative_xi2[2]*derivative_xi1[0]));
						temp_normal[2] = (ZnReal)(special_normal_sign*
							(derivative_xi1[0]*derivative_xi2[1] - derivative_xi2[0]*derivative_xi1[1]));
						for (i=number_of_points_in_xi1;i>0;i--)
						{
							(*normal)[0]=temp_normal[0];
							(*normal)[1]=temp_normal[1];
							(*normal)[2]=temp_normal[2];
							normal++;
						}
					}
				}

				ZnReal distance;
				/* normalize the normals */
				normal=normalpoints;
				if (modified_reverse_normals)
				{
					for (i=number_of_points;i>0;i--)
					{
						if (0.0<(distance=sqrt((*normal)[0]*(*normal)[0]+
							(*normal)[1]*(*normal)[1]+(*normal)[2]*(*normal)[2])))
						{
							(*normal)[0] = -(*normal)[0]/distance;
							(*normal)[1] = -(*normal)[1]/distance;
							(*normal)[2] = -(*normal)[2]/distance;
						}
						floatField[0] = (GLfloat)(*normal)[0];
						floatField[1] = (GLfloat)(*normal)[1];
						floatField[2] = (GLfloat)(*normal)[2];
						if (vertex_location < 0)
						{
							array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
								3, 1, floatField);
						}
						else
						{
							array->replace_float_vertex_buffer_at_position(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
								vertex_start + (number_of_points - i), 3, 1, floatField);
						}
						normal++;
					}
				}
				else
				{
					for (i=number_of_points;i>0;i--)
					{
						if (0.0<(distance=sqrt((*normal)[0]*(*normal)[0]+
							(*normal)[1]*(*normal)[1]+(*normal)[2]*(*normal)[2])))
						{
							(*normal)[0] /= distance;
							(*normal)[1] /= distance;
							(*normal)[2] /= distance;
						}
						floatField[0] = (GLfloat)(*normal)[0];
						floatField[1] = (GLfloat)(*normal)[1];
						floatField[2] = (GLfloat)(*normal)[2];
						if (vertex_location < 0)
						{
							array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
								3, 1, floatField);
						}
						else
						{
							array->replace_float_vertex_buffer_at_position(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
								vertex_start + (number_of_points - i), 3, 1, floatField);
						}
						normal++;
					}
				}
				if (calculate_tangent_points)
				{
					/* normalize the tangents */
					tangent=tangentpoints;
					for (i=number_of_points;i>0;i--)
					{
						if (0.0<(distance=sqrt((*tangent)[0]*(*tangent)[0]+
										(*tangent)[1]*(*tangent)[1]+(*tangent)[2]*(*tangent)[2])))
						{
							(*tangent)[0] /= distance;
							(*tangent)[1] /= distance;
							(*tangent)[2] /= distance;
						}
						floatField[0] = (GLfloat)(*tangent)[0];
						floatField[1] = (GLfloat)(*tangent)[1];
						floatField[2] = (GLfloat)(*tangent)[2];
						if (vertex_location < 0)
						{
							array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_TANGENT,
								3, 1, floatField);
						}
						else
						{
							array->replace_float_vertex_buffer_at_position(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_TANGENT,
								vertex_start + (number_of_points - i), 3, 1, floatField);
						}
						tangent++;
					}
				}

			}
			delete[] feData;
		}
		if (return_code && (vertex_location < 0))
		{
			unsigned int number_of_vertices = number_of_points;
			unsigned int number_of_xi1 = number_of_points_in_xi1;
			unsigned int number_of_xi2 = number_of_points_in_xi2;
			int polygonType = (int)polygon_type;
			array->add_unsigned_integer_attribute(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
				1, 1, &number_of_vertices);
			array->add_unsigned_integer_attribute(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
				1, 1, &vertex_start);
			array->add_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POLYGON,
				1, 1, &polygonType);
			array->add_unsigned_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_XI1,
				1, 1, &number_of_xi1);
			array->add_unsigned_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_XI2,
				1, 1, &number_of_xi2);
			if (shape_type == SIMPLEX_SHAPE)
				array->fill_element_index(vertex_start, number_of_xi1, number_of_xi2,
					ARRAY_SHAPE_TYPE_SIMPLEX);
			else
				array->fill_element_index(vertex_start, number_of_xi1, number_of_xi2,
					ARRAY_SHAPE_TYPE_UNSPECIFIED);
		}
		if (replaceRequired)
		{
			unsigned int number_of_vertices = number_of_points;
			int updated = 0;
			array->replace_integer_vertex_buffer_at_position(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_UPDATE_REQUIRED,
				vertex_location, 1, 1, &updated);
			array->add_unsigned_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_PARTIAL_REDRAW,
					1, 1, &vertex_start);
			array->add_unsigned_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_PARTIAL_REDRAW_COUNT,
					1, 1, &number_of_vertices);
		}

		delete[] floatData;
		delete[] xi_points;
		cmzn_differentialoperator_destroy(&d_dxi1);
		cmzn_differentialoperator_destroy(&d_dxi2);
		DEALLOCATE(normalpoints);
		if (tangentpoints)
		{
			DEALLOCATE(tangentpoints);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_element_add_surface_to_vertex_array.  Invalid argument(s)");
	}

	return (return_code);
}

int Set_element_and_local_xi(struct FE_element **element_block,
	int *n_xi, FE_value *xi, struct FE_element **element)
/*******************************************************************************
LAST MODIFIED : 26 April 1999

DESCRIPTION :
Uses the global xi to select an element from the <element_block> and
returns this pointer in <element>.  The <n_xi> represent the maximum number of
elements in each of the three directions.
The <xi> are set to their local values within the returned <element>.
==============================================================================*/
{
	int a, b, c, return_code;

	ENTER(Set_element_and_local_xi);
	if (element_block&&n_xi&&xi&&element)
	{
		a=(int)(floor(((ZnReal)xi[0])));
		b=(int)(floor(((ZnReal)xi[1])));
		c=(int)(floor(((ZnReal)xi[2])));
		/* set any slight outliers to boundary of volume */
		if (a>n_xi[0]-1)
		{
			a=n_xi[0]-1;
		}
		if (b>n_xi[1]-1)
		{
			b=n_xi[1]-1;
		}
		if (c>n_xi[2]-1)
		{
			c=n_xi[2]-1;
		}
		if (a<0)
		{
			a=0;
		}
		if (b<0)
		{
			b=0;
		}
		if (c<0)
		{
			c=0;
		}
		xi[0] -= ((FE_value)a);
		xi[1] -= ((FE_value)b);
		xi[2] -= ((FE_value)c);

		*element = element_block[c*n_xi[0]*n_xi[1]+b*n_xi[0]+a];
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Set_element_and_local_xi.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return(return_code);
}

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
	struct Multi_range *selected_ranges, int *point_numbers)
{
	char **label = 0, **labels = 0;
	FE_value a[3], b[3], c[3], coordinates[3], orientation_scale[9], size[3],
		variable_scale[3], xi[3];
	GLfloat *data = NULL;
	int draw_all, i, j, n_data_components, return_code = 1,
		number_of_orientation_scale_components, number_of_variable_scale_components,
		point_number, point_selected, points_to_draw, *names = 0,  *name = 0;
	Triple *axis1, *axis1_list, *axis2, *axis2_list, *axis3, *axis3_list,
		*point, *point_list, *scale, *scale_list;
	struct Graphics_vertex_array *array;

	/* must set following to 0 in case fields not supplied */
	number_of_orientation_scale_components = 0;
	number_of_variable_scale_components = 0;

	if (graphics_object && (0 != (array = GT_object_get_vertex_set(graphics_object))) &&
		field_cache && element && coordinate_field &&
		(3 >= cmzn_field_get_number_of_components(coordinate_field)) &&
		(0 < number_of_xi_points) && xi_points && ((((!orientation_scale_field)||((9>=(number_of_orientation_scale_components=
			cmzn_field_get_number_of_components(orientation_scale_field)))&&
			Computed_field_is_orientation_scale_capable(orientation_scale_field,
				(void *)NULL))) &&
		((!variable_scale_field)||(3>=(number_of_variable_scale_components=
			cmzn_field_get_number_of_components(variable_scale_field))))) ||
			!glyph))
	{
		int element_dimension = cmzn_element_get_dimension(element);
		/* clear coordinates in case coordinate field is not 3 component */
		coordinates[0] = 0.0;
		coordinates[1] = 0.0;
		coordinates[2] = 0.0;
		n_data_components = 0;
		data = 0;
		if ((CMZN_GRAPHICS_SELECT_MODE_ON == select_mode) ||
			(CMZN_GRAPHICS_SELECT_MODE_OFF == select_mode) ||
			((CMZN_GRAPHICS_SELECT_MODE_DRAW_SELECTED == select_mode) && element_selected))
		{
			points_to_draw = number_of_xi_points;
		}
		else if ((CMZN_GRAPHICS_SELECT_MODE_DRAW_UNSELECTED == select_mode) && element_selected)
		{
			points_to_draw = 0;
		}
		else
		{
			points_to_draw = 0;
			if (selected_ranges)
			{
				for (i = 0; i < number_of_xi_points; i++)
				{
					if (point_numbers)
					{
						point_number = point_numbers[i];
					}
					else
					{
						point_number = i;
					}
					if (Multi_range_is_value_in_range(selected_ranges, point_number))
					{
						points_to_draw++;
					}
				}
			}
			if (CMZN_GRAPHICS_SELECT_MODE_DRAW_UNSELECTED == select_mode)
			{
				points_to_draw = number_of_xi_points - points_to_draw;
			}
		}
		/* store element index in mesh as object_name for editing GT_object primitives */
		const DsLabelIndex elementIndex = get_FE_element_index(element);
		int replaceRequired = 0;
		/* find if vertex already in the array */
		int vertex_location = array->find_first_fast_search_id_location(elementIndex);
		/* vertex found in array, check if update is required */
		if (vertex_location >= 0)
		{
			array->get_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_UPDATE_REQUIRED,
				vertex_location, 1, &replaceRequired);
		}
		if ((0 < points_to_draw) && (vertex_location < 0 || replaceRequired))
		{
			draw_all = (points_to_draw == number_of_xi_points);
			if (data_field)
			{
				n_data_components = cmzn_field_get_number_of_components(data_field);
				ALLOCATE(data, GLfloat, points_to_draw*n_data_components);
			}
			FE_value *feData = new FE_value[n_data_components];
			if (label_field)
			{
				if (ALLOCATE(labels, char *, points_to_draw))
				{
					/* clear labels array pointers so new glyph_set not corrupted */
					for (i = 0; i < points_to_draw; i++)
					{
						labels[i] = (char *)NULL;
					}
				}
			}
			if (CMZN_GRAPHICS_SELECT_MODE_OFF != select_mode)
			{
				ALLOCATE(names,int,points_to_draw);
			}
			if ((data || (!n_data_components)) && ((!label_field) || labels) &&
					((CMZN_GRAPHICS_SELECT_MODE_OFF == select_mode) || names) &&
					ALLOCATE(point_list, Triple, points_to_draw) &&
					ALLOCATE(axis1_list, Triple, points_to_draw) &&
					ALLOCATE(axis2_list, Triple, points_to_draw) &&
					ALLOCATE(axis3_list, Triple, points_to_draw) &&
					ALLOCATE(scale_list, Triple, points_to_draw))
			{
				point = point_list;
				axis1 = axis1_list;
				axis2 = axis2_list;
				axis3 = axis3_list;
				scale = scale_list;
				name = names;
				label = labels;
				GLfloat *data_value = data;
				for (i = 0; i < number_of_xi_points; i++)
				{
					if (point_numbers)
					{
						point_number = point_numbers[i];
					}
					else
					{
						point_number = i;
					}
					if (!draw_all)
					{
						if (selected_ranges)
						{
							point_selected =
								Multi_range_is_value_in_range(selected_ranges, point_number);
						}
						else
						{
							point_selected = 0;
						}
					}
					if (draw_all ||
						((CMZN_GRAPHICS_SELECT_MODE_DRAW_SELECTED == select_mode) && point_selected) ||
						((CMZN_GRAPHICS_SELECT_MODE_DRAW_UNSELECTED == select_mode) && (!point_selected)))
					{
						xi[0] = xi_points[i][0];
						xi[1] = xi_points[i][1];
						xi[2] = xi_points[i][2];
						/* evaluate all the fields in order orientation_scale, coordinate
							 then data (if each specified). Reason for this order is that the
							 orientation_scale field very often requires the evaluation of the
							 same coordinate_field with derivatives, meaning that values for
							 the coordinate_field will already be cached = more efficient. */
						if ((CMZN_OK == field_cache->setIndexedMeshLocation(static_cast<unsigned int>(i), element, xi, top_level_element)) &&
							((!orientation_scale_field) ||
								(CMZN_OK == cmzn_field_evaluate_real(orientation_scale_field, field_cache,
									number_of_orientation_scale_components, orientation_scale))) &&
							((!variable_scale_field) ||
								(CMZN_OK == cmzn_field_evaluate_real(variable_scale_field, field_cache,
									number_of_variable_scale_components, variable_scale))) &&
							(CMZN_OK == cmzn_field_evaluate_real(coordinate_field, field_cache, /*number_of_components*/3, coordinates)) &&
							((!data_field) ||
								(CMZN_OK == cmzn_field_evaluate_real(data_field, field_cache, n_data_components, feData))) &&
							((!label_field) ||
								(0 != (*label = cmzn_field_evaluate_string(label_field, field_cache)))) &&
							make_glyph_orientation_scale_axes(
								number_of_orientation_scale_components, orientation_scale,
								a, b, c, size))
						{
							for (j = 0; j < 3; j++)
							{
								(*scale)[j] = static_cast<GLfloat>(size[j]);
							}
							for (j = 0; j < number_of_variable_scale_components; j++)
							{
								(*scale)[j] *= ZnReal(variable_scale[j]);
							}
							for (j = 0; j < 3; j++)
							{
								(*point)[j] = ZnReal(coordinates[j]);
								(*axis1)[j] = ZnReal(a[j]);
								(*axis2)[j] = ZnReal(b[j]);
								(*axis3)[j] = ZnReal(c[j]);
							}
							point++;
							axis1++;
							axis2++;
							axis3++;
							scale++;
							if (data_field)
							{
								CAST_TO_OTHER(data_value,feData,GLfloat,n_data_components);
								data_value += n_data_components;
							}
							if (names)
							{
								*name = point_number;
								name++;
							}
							if (labels)
							{
								label++;
							}
						}
					}
				}
				return_code = fill_glyph_graphics_vertex_array(
					array, vertex_location, (unsigned int)points_to_draw, point_list,
					axis1_list, axis2_list, axis3_list, scale_list,
					n_data_components, data, 0, elementIndex, names, labels, 0, 0, 0);
				DEALLOCATE(point_list);
				DEALLOCATE(axis1_list);
				DEALLOCATE(axis2_list);
				DEALLOCATE(axis3_list);
				DEALLOCATE(scale_list);
				if (data)
					DEALLOCATE(data);
				if (labels)
				{
					for (int k = 0; k < points_to_draw; k++)
						DEALLOCATE(labels[k]);
					DEALLOCATE(labels);
				}
				DEALLOCATE(names);
			}
			delete[] feData;
		}
		if (replaceRequired)
		{
			int updated = 0;
			array->replace_integer_vertex_buffer_at_position(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_UPDATE_REQUIRED,
				vertex_location, 1, 1, &updated);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"add_glyphset_vertex_from_FE_element.  Invalid argument(s)");
		return_code = 0;
	}

	return (return_code);
}

int Cmiss_graphic_point_graphics_to_vertex_buffer(
	cmzn_fieldcache_id field_cache, FE_value time, struct Computed_field *label_field,
	const FE_value *base_size, const FE_value *offset, const FE_value *scale_factors,
	struct Graphics_vertex_array *array)
{
	int return_code = 1;
//	char **labels = NULL;
	if (array)
	{
		if (field_cache)
		{
			cmzn_fieldcache_set_time(field_cache, time);
			if (label_field)
			{
//				ALLOCATE(labels, char *, 1);
//				*labels = Cmiss_field_evaluate_string(graphic->label_field, field_cache);
			}
		}

		Triple point, axis1, axis2, axis3, scale_list;
		point[0] = (GLfloat)offset[0];
		point[1] = (GLfloat)offset[1];
		point[2] = (GLfloat)offset[2];
		axis1[0] = (GLfloat)base_size[0];
		axis1[1] = 0.0;
		axis1[2] = 0.0;
		axis2[0] = 0.0;
		axis2[1] = (GLfloat)base_size[1];
		axis2[2] = 0.0;
		axis3[0] = 0.0;
		axis3[1] = 0.0;
		axis3[2] = (GLfloat)base_size[2];
		scale_list[0] = (GLfloat)scale_factors[0];
		scale_list[1] = (GLfloat)scale_factors[1];
		scale_list[2] = (GLfloat)scale_factors[2];
		unsigned int number_of_points = 1;
		unsigned int vertex_start = array->get_number_of_vertices(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION);
		array->add_unsigned_integer_attribute(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
			1, 1, &number_of_points);
		array->add_unsigned_integer_attribute(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
			1, 1, &vertex_start);
		array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
			3, number_of_points, point);
		array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_AXIS1,
			3, number_of_points, axis1);
		array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_AXIS2,
			3, number_of_points, axis2);
		array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_AXIS3,
			3, number_of_points, axis3);
		array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_SCALE,
			3, number_of_points, scale_list);
	}
	else
	{
		return_code = 0;
	}

	return (return_code);
}
