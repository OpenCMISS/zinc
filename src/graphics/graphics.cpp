/**
 * FILE : graphics.cpp
 *
 * Implementation of graphics, a visualisation of fields using an algorithm
 * i.e. points, lines, surfaces, contours and streamlines.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <string>

#include "opencmiss/zinc/zincconfigure.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "opencmiss/zinc/fieldsubobjectgroup.h"
#include "opencmiss/zinc/font.h"
#include "opencmiss/zinc/glyph.h"
#include "opencmiss/zinc/graphics.h"
#include "opencmiss/zinc/material.h"
#include "opencmiss/zinc/mesh.h"
#include "opencmiss/zinc/node.h"
#include "opencmiss/zinc/nodeset.h"
#include "opencmiss/zinc/scenefilter.h"
#include "opencmiss/zinc/status.h"
#include "general/debug.h"
#include "general/enumerator_private.hpp"
#include "general/indexed_list_stl_private.hpp"
#include "general/compare.h"
#include "general/multi_range.h"
#include "general/mystring.h"
#include "general/object.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_finite_element.h"
#include "computed_field/computed_field_set.h"
#include "computed_field/computed_field_wrappers.h"
#include "computed_field/field_module.hpp"
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_discretization.h"
#include "finite_element/finite_element_region.h"
#include "finite_element/finite_element_to_graphics_object.h"
#include "finite_element/finite_element_to_iso_lines.h"
#include "finite_element/finite_element_to_iso_surfaces.h"
#include "finite_element/finite_element_to_streamlines.h"
#include "graphics/auxiliary_graphics_types.h"
#include "graphics/font.h"
#include "graphics/glyph.hpp"
#include "graphics/graphics_object.h"
#include "graphics/graphics_object.hpp"
#include "graphics/scene.hpp"
#include "graphics/graphics.h"
#include "graphics/graphics_module.hpp"
#include "general/message.h"
#include "general/enumerator_conversion.hpp"
#include "graphics/render_gl.h"
#include "graphics/scene_coordinate_system.hpp"
#include "graphics/tessellation.hpp"
#if defined(USE_OPENCASCADE)
#	include "cad/computed_field_cad_geometry.h"
#	include "cad/computed_field_cad_topology.h"
#	include "cad/cad_geometry_to_graphics_object.h"
#endif /* defined(USE_OPENCASCADE) */

struct cmzn_graphics_select_graphics_data
{
	struct FE_region *fe_region;
	struct cmzn_graphics *graphics;
};

enum cmzn_graphics_change
{
	CMZN_GRAPHICS_CHANGE_NONE = 0,
	CMZN_GRAPHICS_CHANGE_REDRAW = 1,          /**< minor change requiring redraw, e.g. visibility flag toggled */
	CMZN_GRAPHICS_CHANGE_RECOMPILE = 2,       /**< graphics display list may need to be recompiled */
	CMZN_GRAPHICS_CHANGE_SELECTION = 3,       /**< change to selected objects */
	CMZN_GRAPHICS_CHANGE_PARTIAL_REBUILD = 4, /**< partial rebuild of graphics object */
	CMZN_GRAPHICS_CHANGE_FULL_REBUILD = 5,    /**< graphics object needs full rebuild */
};

/***************************************************************************//**
 * Call whenever attributes of the graphics have changed to ensure the graphics
 * object is invalidated (if needed) or that the minimum rebuild and redraw is
 * performed.
 */
static int cmzn_graphics_changed(struct cmzn_graphics *graphics,
	enum cmzn_graphics_change change)
{
	int return_code = 1;
	if (graphics)
	{
		switch (change)
		{
		case CMZN_GRAPHICS_CHANGE_REDRAW:
			break;
		case CMZN_GRAPHICS_CHANGE_RECOMPILE:
		case CMZN_GRAPHICS_CHANGE_SELECTION:
			graphics->selected_graphics_changed = 1;
			break;
		case CMZN_GRAPHICS_CHANGE_PARTIAL_REBUILD:
			// partial removal of graphics should have been done by caller
			graphics->graphics_changed = 1;
			break;
		case CMZN_GRAPHICS_CHANGE_FULL_REBUILD:
			graphics->graphics_changed = 1;
			if (graphics->graphics_object)
			{
				DEACCESS(GT_object)(&(graphics->graphics_object));
			}
			break;
		default:
			return_code = 0;
			break;
		}
		graphics->incrementalBuildIndex = DS_LABEL_INDEX_INVALID;
		if (return_code && (graphics->scene))
			graphics->scene->setChanged();
	}
	else
	{
		return_code = 0;
	}
	return return_code;
}

PROTOTYPE_ENUMERATOR_STRING_FUNCTION(cmzn_graphics_type)
{
	const char *enumerator_string;

	ENTER(ENUMERATOR_STRING(cmzn_graphics_type));
	switch (enumerator_value)
	{
		case CMZN_GRAPHICS_TYPE_POINTS:
		{
			enumerator_string = "points";
		} break;
		case CMZN_GRAPHICS_TYPE_LINES:
		{
			enumerator_string = "lines";
		} break;
		case CMZN_GRAPHICS_TYPE_SURFACES:
		{
			enumerator_string = "surfaces";
		} break;
		case CMZN_GRAPHICS_TYPE_CONTOURS:
		{
			enumerator_string = "contours";
		} break;
		case CMZN_GRAPHICS_TYPE_STREAMLINES:
		{
			enumerator_string = "streamlines";
		} break;
		default:
		{
			enumerator_string = (const char *)NULL;
		} break;
	}
	LEAVE;

	return (enumerator_string);
} /* ENUMERATOR_STRING(cmzn_graphics_type) */

DEFINE_DEFAULT_ENUMERATOR_FUNCTIONS(cmzn_graphics_type)

struct cmzn_graphics *CREATE(cmzn_graphics)(
	enum cmzn_graphics_type graphics_type)
{
	struct cmzn_graphics *graphics;

	ENTER(CREATE(cmzn_graphics));
	if ((CMZN_GRAPHICS_TYPE_POINTS==graphics_type)||
		(CMZN_GRAPHICS_TYPE_LINES==graphics_type)||
		(CMZN_GRAPHICS_TYPE_SURFACES==graphics_type)||
		(CMZN_GRAPHICS_TYPE_CONTOURS==graphics_type)||
		(CMZN_GRAPHICS_TYPE_STREAMLINES==graphics_type))
	{
		if (ALLOCATE(graphics,struct cmzn_graphics,1))
		{
			graphics->position=0;
			graphics->scene = NULL;
			graphics->name = (char *)NULL;

			/* geometry settings defaults */
			/* for all graphics types */
			graphics->graphics_type=graphics_type;
			graphics->coordinate_field=(struct Computed_field *)NULL;
			/* For surfaces only at the moment */
			graphics->texture_coordinate_field=(struct Computed_field *)NULL;
			/* for 1-D and 2-D elements only */
			graphics->exterior = false;
			graphics->face = CMZN_ELEMENT_FACE_TYPE_ALL; /* match all elements, face or no face */

			/* line attributes */
			graphics->line_shape = CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_LINE;
			for (int i = 0; i < 2; i++)
			{
				graphics->line_base_size[i] = 0.0;
				graphics->line_scale_factors[i] = 1.0;
			}
			graphics->line_orientation_scale_field = 0;

			/* for contours only */
			graphics->isoscalar_field=(struct Computed_field *)NULL;
			graphics->number_of_isovalues=0;
			graphics->isovalues=(double *)NULL;
			graphics->first_isovalue=0.0;
			graphics->last_isovalue=0.0;
			graphics->decimation_threshold = 0.0;

			/* point attributes */
			graphics->glyph = 0;
			graphics->glyph_repeat_mode = CMZN_GLYPH_REPEAT_MODE_NONE;
			for (int i = 0; i < 3; i++)
			{
				graphics->point_offset[i] = 0.0;
				graphics->point_base_size[i] = 0.0;
				graphics->point_scale_factors[i] = 1.0;
				graphics->label_offset[i] = 0.0;
				graphics->label_text[i] = 0;
			}
			graphics->point_orientation_scale_field = 0;
			graphics->signed_scale_field = 0;
			graphics->label_field = 0;
			graphics->label_density_field = 0;

			graphics->subgroup_field=(struct Computed_field *)NULL;
			graphics->select_mode=CMZN_GRAPHICS_SELECT_MODE_ON;
			switch (graphics_type)
			{
			case CMZN_GRAPHICS_TYPE_POINTS:
				graphics->domain_type = CMZN_FIELD_DOMAIN_TYPE_POINT;
				break;
			case CMZN_GRAPHICS_TYPE_LINES:
				graphics->domain_type = CMZN_FIELD_DOMAIN_TYPE_MESH1D;
				break;
			case CMZN_GRAPHICS_TYPE_SURFACES:
				graphics->domain_type = CMZN_FIELD_DOMAIN_TYPE_MESH2D;
				break;
			default:
				graphics->domain_type = CMZN_FIELD_DOMAIN_TYPE_MESH_HIGHEST_DIMENSION;
				break;
			}
			// for element sampling: element points, streamlines
			graphics->sampling_mode = CMZN_ELEMENT_POINT_SAMPLING_MODE_CELL_CENTRES;
			graphics->sample_density_field = 0;
			for (int i = 0; i < 3; i++)
			{
				graphics->sample_location[i] = 0.0;
			}
			// for tessellating and sampling elements
			graphics->tessellation = 0;
			graphics->tessellation_field = 0;
			/* for settings starting in a particular element */
			graphics->seed_element=(struct FE_element *)NULL;
			/* for streamlines only */
			graphics->stream_vector_field=(struct Computed_field *)NULL;
			graphics->streamlines_track_direction = CMZN_GRAPHICS_STREAMLINES_TRACK_DIRECTION_FORWARD;
			graphics->streamline_length=1.0;
			graphics->seed_nodeset = (cmzn_nodeset_id)0;
			graphics->seed_node_mesh_location_field = (struct Computed_field *)NULL;
			graphics->overlay_flag = 0;
			graphics->overlay_order = 1;
			graphics->coordinate_system = CMZN_SCENECOORDINATESYSTEM_LOCAL;
			/* appearance settings defaults */
			/* for all graphics types */
			graphics->visibility_flag = true;
			graphics->material=(cmzn_material *)NULL;
			graphics->secondary_material=(cmzn_material *)NULL;
			graphics->selected_material=(cmzn_material *)NULL;
			graphics->data_field=(struct Computed_field *)NULL;
			graphics->spectrum=(struct cmzn_spectrum *)NULL;
			graphics->autorange_spectrum_flag = 0;
			/* for glyphsets */
			graphics->font = NULL;
			/* for surface rendering */
			graphics->render_polygon_mode = CMZN_GRAPHICS_RENDER_POLYGON_MODE_SHADED;
			/* for streamlines only */
			graphics->streamlines_colour_data_type = CMZN_GRAPHICS_STREAMLINES_COLOUR_DATA_TYPE_FIELD;
			graphics->render_line_width = 1.0;
			graphics->render_point_size = 1.0;

			/* rendering information defaults */
			graphics->graphics_object = (struct GT_object *)NULL;
			graphics->graphics_changed = 1;
			graphics->incrementalBuildIndex = DS_LABEL_INDEX_INVALID;
			graphics->selected_graphics_changed = 0;
			graphics->timeDependent = false;

			graphics->access_count=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"CREATE(cmzn_graphics).  Insufficient memory");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CREATE(cmzn_graphics).  Invalid graphics type");
		graphics=(struct cmzn_graphics *)NULL;
	}
	LEAVE;

	return (graphics);
} /* CREATE(cmzn_graphics) */

int DESTROY(cmzn_graphics)(
	struct cmzn_graphics **graphics_address)
{
	int return_code;
	struct cmzn_graphics *graphics;

	ENTER(DESTROY(cmzn_graphics));
	if (graphics_address && (graphics= *graphics_address))
	{
		if (graphics->scene)
			cmzn_graphics_set_scene_private(graphics, NULL);
		if (graphics->name)
		{
			DEALLOCATE(graphics->name);
		}
		if (graphics->graphics_object)
		{
			DEACCESS(GT_object)(&(graphics->graphics_object));
		}
		if (graphics->coordinate_field)
		{
			DEACCESS(Computed_field)(&(graphics->coordinate_field));
		}
		if (graphics->texture_coordinate_field)
		{
			DEACCESS(Computed_field)(&(graphics->texture_coordinate_field));
		}
		cmzn_field_destroy(&(graphics->line_orientation_scale_field));
		if (graphics->isoscalar_field)
		{
			DEACCESS(Computed_field)(&(graphics->isoscalar_field));
		}
		if (graphics->isovalues)
		{
			DEALLOCATE(graphics->isovalues);
		}
		if (graphics->glyph)
		{
			cmzn_glyph_destroy(&(graphics->glyph));
		}
		cmzn_field_destroy(&(graphics->point_orientation_scale_field));
		cmzn_field_destroy(&(graphics->signed_scale_field));
		for (int i = 0; i < 3; i++)
		{
			if (graphics->label_text[i])
			{
				DEALLOCATE(graphics->label_text[i]);
			}
		}
		if (graphics->label_field)
		{
			DEACCESS(Computed_field)(&(graphics->label_field));
		}
		if (graphics->label_density_field)
		{
			DEACCESS(Computed_field)(&(graphics->label_density_field));
		}
		if (graphics->subgroup_field)
		{
			DEACCESS(Computed_field)(&(graphics->subgroup_field));
		}
		cmzn_field_destroy(&(graphics->sample_density_field));
		cmzn_field_destroy(&(graphics->tessellation_field));
		if (graphics->tessellation)
		{
			DEACCESS(cmzn_tessellation)(&(graphics->tessellation));
		}
		if (graphics->stream_vector_field)
		{
			DEACCESS(Computed_field)(&(graphics->stream_vector_field));
		}
		/* appearance graphics */
		if (graphics->material)
		{
			cmzn_material_destroy(&(graphics->material));
		}
		if (graphics->secondary_material)
		{
			cmzn_material_destroy(&(graphics->secondary_material));
		}
		if (graphics->selected_material)
		{
			cmzn_material_destroy(&(graphics->selected_material));
		}
		if (graphics->data_field)
		{
			DEACCESS(Computed_field)(&(graphics->data_field));
		}
		if (graphics->spectrum)
		{
			DEACCESS(cmzn_spectrum)(&(graphics->spectrum));
		}
		if (graphics->font)
		{
			DEACCESS(cmzn_font)(&(graphics->font));
		}
		if (graphics->seed_element)
		{
			DEACCESS(FE_element)(&(graphics->seed_element));
		}
		if (graphics->seed_nodeset)
		{
			cmzn_nodeset_destroy(&graphics->seed_nodeset);
		}
		if (graphics->seed_node_mesh_location_field)
		{
			DEACCESS(Computed_field)(&(graphics->seed_node_mesh_location_field));
		}
		DEALLOCATE(*graphics_address);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"DESTROY(cmzn_graphics_address).  "
			"Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
}

int cmzn_graphics_get_domain_dimension(struct cmzn_graphics *graphics)
{
	int dimension = -1;
	if (graphics)
	{
		switch (graphics->domain_type)
		{
		case CMZN_FIELD_DOMAIN_TYPE_POINT:
		case CMZN_FIELD_DOMAIN_TYPE_NODES:
		case CMZN_FIELD_DOMAIN_TYPE_DATAPOINTS:
			dimension = 0;
			break;
		case CMZN_FIELD_DOMAIN_TYPE_MESH1D:
			dimension = 1;
			break;
		case CMZN_FIELD_DOMAIN_TYPE_MESH2D:
			dimension = 2;
			break;
		case CMZN_FIELD_DOMAIN_TYPE_MESH3D:
			dimension = 3;
			break;
		case CMZN_FIELD_DOMAIN_TYPE_MESH_HIGHEST_DIMENSION:
			dimension = 3;
			if (graphics->scene)
			{
				dimension = FE_region_get_highest_dimension(graphics->scene->region->get_FE_region());
				if (0 >= dimension)
					dimension = 3;
			}
			break;
		case CMZN_FIELD_DOMAIN_TYPE_INVALID:
			display_message(ERROR_MESSAGE, "cmzn_graphics_get_domain_dimension.  Unknown graphics type");
			break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_get_domain_dimension.  Invalid argument(s)");
	}
	return (dimension);
}

#ifdef OLD_CODE
// was used to inherit discretisation from an ancestor element satisfying this
struct cmzn_element_conditional_field_data
{
	cmzn_fieldcache_id field_cache;
	cmzn_field_id conditional_field;
};

/** @return true if conditional field evaluates to true in element */
int cmzn_element_conditional_field_is_true(cmzn_element_id element,
	void *conditional_field_data_void)
{
	cmzn_element_conditional_field_data *data =
		reinterpret_cast<cmzn_element_conditional_field_data*>(conditional_field_data_void);
	if (element && data)
	{
		cmzn_fieldcache_set_element(data->field_cache, element);
		if (cmzn_field_evaluate_boolean(data->conditional_field, data->field_cache))
			return 1;
	}
	return 0;
}
#endif // OLD_CODE

/**
 * Converts a finite element into a graphics object with the supplied graphics.
 * @param element  The cmzn_element.
 * @param graphics_to_object_data  Data for converting finite element to graphics.
 * @return return 1 if the element would contribute any graphics generated from the cmzn_graphics
 */
static int FE_element_to_graphics_object(struct FE_element *element,
	cmzn_graphics_to_graphics_object_data *graphics_to_object_data)
{
	FE_value initial_xi[3];
	int i, number_in_xi[MAXIMUM_ELEMENT_XI_DIMENSIONS],
		number_of_xi_points, return_code = 1;
	struct Element_point_ranges_identifier element_point_ranges_identifier;
	struct FE_element *top_level_element;
	struct cmzn_graphics *graphics;
	FE_value_triple *xi_points = NULL;

	ENTER(FE_element_to_graphics_object);
	FE_mesh *fe_mesh = element->getMesh();
	if (fe_mesh && graphics_to_object_data &&
		(NULL != (graphics = graphics_to_object_data->graphics)) &&
		graphics->graphics_object)
	{
		const DsLabelIndex elementIndex = get_FE_element_index(element);
		const int element_dimension = fe_mesh->getDimension();
		const int graphics_domain_dimension = cmzn_graphics_get_domain_dimension(graphics);
		if (element_dimension != graphics_domain_dimension)
			return 1; // shouldn't be iterating over wrong dimension
		if (element_dimension < 3)
		{
			if ((graphics->exterior) && (!fe_mesh->isElementExterior(elementIndex)))
				return 1;
			if (CMZN_ELEMENT_FACE_TYPE_ALL != graphics->face)
			{
				if (CMZN_ELEMENT_FACE_TYPE_NO_FACE == graphics->face)
				{
					if (fe_mesh->getElementParentOnFace(elementIndex, CMZN_ELEMENT_FACE_TYPE_ANY_FACE) >= 0)
						return 1;
				}
				else if (fe_mesh->getElementParentOnFace(elementIndex, graphics->face) < 0)
					return 1;
			}
		}
		cmzn_fieldcache_set_element(graphics_to_object_data->field_cache, element);
		if (graphics->subgroup_field && (graphics_to_object_data->iteration_mesh == graphics_to_object_data->master_mesh))
		{
			if (!cmzn_field_evaluate_boolean(graphics->subgroup_field, graphics_to_object_data->field_cache))
				return 1;
		}
		//cmzn_element_conditional_field_data conditional_field_data = { graphics_to_object_data->field_cache, graphics->subgroup_field };
		bool name_selected = false;
		if ((CMZN_GRAPHICS_SELECT_MODE_DRAW_SELECTED == graphics->select_mode) ||
			(CMZN_GRAPHICS_SELECT_MODE_DRAW_UNSELECTED == graphics->select_mode))
		{
			if (graphics_to_object_data->selection_group_field)
				name_selected = cmzn_field_evaluate_boolean(graphics_to_object_data->selection_group_field, graphics_to_object_data->field_cache);
			if (!((name_selected && (CMZN_GRAPHICS_SELECT_MODE_DRAW_SELECTED == graphics->select_mode)) ||
					((!name_selected) && (CMZN_GRAPHICS_SELECT_MODE_DRAW_SELECTED != graphics->select_mode))))
				return 1;
		}
		/* determine discretization of element for graphics */
		// copy top_level_number_in_xi since scaled by native_discretization in
		// get_FE_element_discretization
		int top_level_number_in_xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
		for (int dim = 0; dim < MAXIMUM_ELEMENT_XI_DIMENSIONS; dim++)
		{
			top_level_number_in_xi[dim] = graphics_to_object_data->top_level_number_in_xi[dim];
		}
		top_level_element = (struct FE_element *)NULL;
		struct FE_field *native_discretization_field = 0;
		if (graphics->tessellation_field)
		{
			Computed_field_get_type_finite_element(graphics->tessellation_field, &native_discretization_field);
		}
		if (get_FE_element_discretization(element,
			graphics->face, native_discretization_field, top_level_number_in_xi,
			&top_level_element, number_in_xi))
		{
			switch (graphics->graphics_type)
			{
				case CMZN_GRAPHICS_TYPE_LINES:
				{
					if (CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_LINE == graphics->line_shape)
					{
						return_code = FE_element_add_line_to_vertex_array(
							element, graphics_to_object_data->field_cache,
							GT_object_get_vertex_set(graphics->graphics_object),
							graphics_to_object_data->rc_coordinate_field,
							graphics_to_object_data->number_of_data_values,
							graphics->data_field,
							graphics->texture_coordinate_field,
							number_in_xi[0], top_level_element);
					}
					else
					{
						return_code = FE_element_add_cylinder_to_vertex_array(
							element, graphics_to_object_data->field_cache,
							GT_object_get_vertex_set(graphics->graphics_object),
							graphics_to_object_data->master_mesh,
							graphics_to_object_data->rc_coordinate_field,
							graphics->data_field,
							graphics->line_base_size,
							graphics->line_scale_factors,
							graphics->line_orientation_scale_field,
							number_in_xi[0],
							cmzn_tessellation_get_circle_divisions(graphics->tessellation),
							graphics->texture_coordinate_field,
							top_level_element);
					}
				} break;
				case CMZN_GRAPHICS_TYPE_SURFACES:
				{
					return_code = FE_element_add_surface_to_vertex_array(
						element, graphics_to_object_data->field_cache,
						graphics_to_object_data->master_mesh,
						GT_object_get_vertex_set(graphics->graphics_object),
						graphics_to_object_data->rc_coordinate_field,
						graphics->texture_coordinate_field,
						graphics->data_field,
						number_in_xi[0], number_in_xi[1],
						/*reverse_normals*/0, top_level_element);
				} break;
				case CMZN_GRAPHICS_TYPE_CONTOURS:
				{
					switch (GT_object_get_type(graphics->graphics_object))
					{
						case g_SURFACE_VERTEX_BUFFERS:
						{
							if (3 == element_dimension)
							{
								return_code = create_iso_surfaces_from_FE_element(element,
									graphics_to_object_data->field_cache,
									graphics_to_object_data->master_mesh,
									GT_object_get_vertex_set(graphics->graphics_object),
									number_in_xi, graphics_to_object_data->iso_surface_specification);
							}
						} break;
						case g_POLYLINE_VERTEX_BUFFERS:
						{
							if (2 == element_dimension)
							{
								if (graphics->isovalues)
								{
									for (i = 0 ; i < graphics->number_of_isovalues ; i++)
									{
										return_code = create_iso_lines_from_FE_element(element,
											graphics_to_object_data->field_cache,
											graphics_to_object_data->rc_coordinate_field,
											graphics->isoscalar_field, graphics->isovalues[i],
											graphics->data_field, number_in_xi[0], number_in_xi[1],
											top_level_element, GT_object_get_vertex_set(graphics->graphics_object));
									}
								}
								else
								{
									double isovalue_range;
									if (graphics->number_of_isovalues > 1)
									{
										isovalue_range =
											(graphics->last_isovalue - graphics->first_isovalue)
											/ (double)(graphics->number_of_isovalues - 1);
									}
									else
									{
										isovalue_range = 0;
									}
									for (i = 0 ; i < graphics->number_of_isovalues ; i++)
									{
										double isovalue =
											graphics->first_isovalue +
											(double)i * isovalue_range;
										return_code = create_iso_lines_from_FE_element(element,
											graphics_to_object_data->field_cache,
											graphics_to_object_data->rc_coordinate_field,
											graphics->isoscalar_field, isovalue,
											graphics->data_field, number_in_xi[0], number_in_xi[1],
											top_level_element, GT_object_get_vertex_set(graphics->graphics_object));
									}
								}
							}
						} break;
						default:
						{
							display_message(ERROR_MESSAGE,"FE_element_to_graphics_object.  "
								"Invalid graphics type for contours");
							return_code = 0;
						} break;
					}
				} break;
				case CMZN_GRAPHICS_TYPE_POINTS:
				{
					for (i = 0; i < 3; i++)
					{
						element_point_ranges_identifier.exact_xi[i] =
								graphics->sample_location[i];
					}
					if (FE_element_get_xi_points(element,
						graphics->sampling_mode, number_in_xi,
						element_point_ranges_identifier.exact_xi,
						graphics_to_object_data->field_cache,
						graphics_to_object_data->rc_coordinate_field,
						graphics->sample_density_field,
						&number_of_xi_points, &xi_points))
					{
						element_point_ranges_identifier.element = element;
						element_point_ranges_identifier.top_level_element = top_level_element;
						element_point_ranges_identifier.sampling_mode = graphics->sampling_mode;
						for (i = 0; i < element_dimension; i++)
							element_point_ranges_identifier.number_in_xi[i] = number_in_xi[i];
						/* NOT an error if no glyph_set produced == empty selection */
						if (0 < number_of_xi_points)
						{
							return_code = add_glyphset_vertex_from_FE_element(
								graphics->graphics_object,
								graphics_to_object_data->field_cache,
								element, top_level_element,
								graphics_to_object_data->rc_coordinate_field,
								number_of_xi_points, xi_points,
								graphics_to_object_data->glyph_gt_object,
								graphics_to_object_data->wrapper_orientation_scale_field,
								graphics->signed_scale_field, graphics->data_field,
								graphics->label_field, graphics->select_mode,
								name_selected ? 1 : 0, (struct Multi_range *)NULL,
								/*point_numbers*/(int *)0);
						}
						DEALLOCATE(xi_points);
					}
					else
					{
						return_code = 0;
					}
				} break;
				case CMZN_GRAPHICS_TYPE_STREAMLINES:
				{
					/* use local copy of sample_location since tracking function updates it */
					for (i = 0; i < 3; i++)
					{
						initial_xi[i] =  element_point_ranges_identifier.exact_xi[i] = graphics->sample_location[i];
					}
					if (FE_element_get_xi_points(element,
						graphics->sampling_mode, number_in_xi,
						element_point_ranges_identifier.exact_xi,
						graphics_to_object_data->field_cache,
						graphics_to_object_data->rc_coordinate_field,
						graphics->sample_density_field,
						&number_of_xi_points, &xi_points))
					{
						switch (graphics->line_shape)
						{
						case CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_LINE:
							{
								for (i = 0; i < number_of_xi_points; i++)
								{
									initial_xi[0] = xi_points[i][0];
									initial_xi[1] = xi_points[i][1];
									initial_xi[2] = xi_points[i][2];
									return_code = create_polyline_streamline_FE_element_vertex_array(
										element, initial_xi, graphics_to_object_data->field_cache,
										graphics_to_object_data->rc_coordinate_field,
										graphics_to_object_data->wrapper_stream_vector_field,
										static_cast<int>(graphics->streamlines_track_direction == CMZN_GRAPHICS_STREAMLINES_TRACK_DIRECTION_REVERSE),
										graphics->streamline_length,
										graphics->streamlines_colour_data_type, graphics->data_field,
										GT_object_get_vertex_set(graphics->graphics_object));
								}
							} break;
						case CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_RIBBON:
						case CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_CIRCLE_EXTRUSION:
						case CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_SQUARE_EXTRUSION:
							{
								for (i = 0; i < number_of_xi_points; i++)
								{
									initial_xi[0] = xi_points[i][0];
									initial_xi[1] = xi_points[i][1];
									initial_xi[2] = xi_points[i][2];
									return_code = create_surface_streamribbon_FE_element_vertex_array(
										element, initial_xi, graphics_to_object_data->field_cache,
										graphics_to_object_data->rc_coordinate_field,
										graphics_to_object_data->wrapper_stream_vector_field,
										static_cast<int>(graphics->streamlines_track_direction == CMZN_GRAPHICS_STREAMLINES_TRACK_DIRECTION_REVERSE),
										graphics->streamline_length,
										graphics->line_shape, cmzn_tessellation_get_circle_divisions(graphics->tessellation),
										graphics->line_base_size, graphics->line_scale_factors,
										graphics->line_orientation_scale_field,
										graphics->streamlines_colour_data_type, graphics->data_field,
										GT_object_get_vertex_set(graphics->graphics_object));
								}
							} break;
						case CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_INVALID:
							{
								display_message(ERROR_MESSAGE,
									"FE_element_to_graphics_object.  Unknown streamline type");
								return_code = 0;
							} break;
						}
					}
					else
					{
						return_code = 0;
					}
					if (xi_points)
						DEALLOCATE(xi_points);
				} break;
				default:
				{
					display_message(ERROR_MESSAGE,"FE_element_to_graphics_object.  "
						"Unknown element graphics type");
					return_code = 0;
				} break;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_element_to_graphics_object.  Could not get discretization");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"FE_element_to_graphics_object.  "
			"Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_element_to_graphics_object */

/***************************************************************************//**
 * Creates a streamline seeded from the location given by the
 * seed_node_mesh_location_field at the node.
 * @param node  The node to seed streamline from.
 * @param graphics_to_object_data  All other data including graphics.
 * @return  1 if successfully added streamline
 */
static int cmzn_node_to_streamline(struct FE_node *node,
	struct cmzn_graphics_to_graphics_object_data *graphics_to_object_data)
{
	int return_code = 1;

	ENTER(node_to_streamline);
	struct cmzn_graphics *graphics = 0;
	if (node && graphics_to_object_data &&
		(NULL != (graphics = graphics_to_object_data->graphics)) &&
		graphics->graphics_object)
	{
		cmzn_fieldcache_set_node(graphics_to_object_data->field_cache, node);
		FE_value xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
		cmzn_element_id element = cmzn_field_evaluate_mesh_location(
			graphics->seed_node_mesh_location_field, graphics_to_object_data->field_cache,
			MAXIMUM_ELEMENT_XI_DIMENSIONS, xi);
		if (element)
		{
			switch (graphics->line_shape)
			{
			case CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_LINE:
				{
					return_code = create_polyline_streamline_FE_element_vertex_array(element,
							xi, graphics_to_object_data->field_cache,
							graphics_to_object_data->rc_coordinate_field,
							graphics_to_object_data->wrapper_stream_vector_field,
							static_cast<int>(graphics->streamlines_track_direction == CMZN_GRAPHICS_STREAMLINES_TRACK_DIRECTION_REVERSE),
							graphics->streamline_length,
							graphics->streamlines_colour_data_type, graphics->data_field,
							GT_object_get_vertex_set(graphics->graphics_object));
				} break;
			case CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_RIBBON:
			case CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_CIRCLE_EXTRUSION:
			case CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_SQUARE_EXTRUSION:
				{
					return_code = create_surface_streamribbon_FE_element_vertex_array(element,
						xi, graphics_to_object_data->field_cache,
						graphics_to_object_data->rc_coordinate_field,
						graphics_to_object_data->wrapper_stream_vector_field,
						static_cast<int>(graphics->streamlines_track_direction == CMZN_GRAPHICS_STREAMLINES_TRACK_DIRECTION_REVERSE),
						graphics->streamline_length,
						graphics->line_shape, cmzn_tessellation_get_circle_divisions(graphics->tessellation),
						graphics->line_base_size, graphics->line_scale_factors,
						graphics->line_orientation_scale_field,
						graphics->streamlines_colour_data_type, graphics->data_field,
						GT_object_get_vertex_set(graphics->graphics_object));
				} break;
			case CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_INVALID:
				{
					display_message(ERROR_MESSAGE,
						"cmzn_node_to_streamline.  Unknown streamline type");
					return_code=0;
				} break;
			}
			cmzn_element_destroy(&element);
		}
		else
		{
			return_code=1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_node_to_streamline.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* node_to_streamline */

int cmzn_graphics_add_to_list(struct cmzn_graphics *graphics,
	int position,struct LIST(cmzn_graphics) *list_of_graphics)
{
	int last_position,return_code;
	struct cmzn_graphics *graphics_in_way;

	ENTER(cmzn_graphics_add_to_list);
	if (graphics&&list_of_graphics&&
		!IS_OBJECT_IN_LIST(cmzn_graphics)(graphics,list_of_graphics))
	{
		return_code=1;
		last_position=NUMBER_IN_LIST(cmzn_graphics)(list_of_graphics);
		if ((1>position)||(position>last_position))
		{
			/* add to end of list */
			position=last_position+1;
		}
		ACCESS(cmzn_graphics)(graphics);
		while (return_code&&graphics)
		{
			graphics->position=position;
			/* is there already a graphics with that position? */
			if (NULL != (graphics_in_way=FIND_BY_IDENTIFIER_IN_LIST(cmzn_graphics,
						position)(position,list_of_graphics)))
			{
				/* remove the old graphics to make way for the new */
				ACCESS(cmzn_graphics)(graphics_in_way);
				REMOVE_OBJECT_FROM_LIST(cmzn_graphics)(
					graphics_in_way,list_of_graphics);
			}
			if (ADD_OBJECT_TO_LIST(cmzn_graphics)(graphics,list_of_graphics))
			{
				DEACCESS(cmzn_graphics)(&graphics);
				/* the old, in-the-way graphics now become the new graphics */
				graphics=graphics_in_way;
				position++;
			}
			else
			{
				DEACCESS(cmzn_graphics)(&graphics);
				if (graphics_in_way)
				{
					DEACCESS(cmzn_graphics)(&graphics_in_way);
				}
				display_message(ERROR_MESSAGE,"cmzn_graphics_add_to_list.  "
					"Could not add graphics - graphics lost");
				return_code=0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_add_to_list.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
}

int cmzn_graphics_remove_from_list(struct cmzn_graphics *graphics,
	struct LIST(cmzn_graphics) *list_of_graphics)
{
	int return_code,next_position;

	ENTER(cmzn_graphics_remove_from_list);
	if (graphics&&list_of_graphics)
	{
		if (IS_OBJECT_IN_LIST(cmzn_graphics)(graphics,list_of_graphics))
		{
			next_position=graphics->position+1;
			return_code=REMOVE_OBJECT_FROM_LIST(cmzn_graphics)(
				graphics,list_of_graphics);
			/* decrement position of all remaining graphics */
			while (return_code&&(graphics=FIND_BY_IDENTIFIER_IN_LIST(
				cmzn_graphics,position)(next_position,list_of_graphics)))
			{
				ACCESS(cmzn_graphics)(graphics);
				REMOVE_OBJECT_FROM_LIST(cmzn_graphics)(graphics,list_of_graphics);
				(graphics->position)--;
				if (ADD_OBJECT_TO_LIST(cmzn_graphics)(graphics,list_of_graphics))
				{
					next_position++;
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"cmzn_graphics_remove_from_list.  "
						"Could not readjust positions - graphics lost");
					return_code=0;
				}
				DEACCESS(cmzn_graphics)(&graphics);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"cmzn_graphics_remove_from_list.  Graphics not in list");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_remove_from_list.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_graphics_remove_from_list */

int cmzn_graphics_modify_in_list(struct cmzn_graphics *graphics,
	struct cmzn_graphics *new_graphics,
	struct LIST(cmzn_graphics) *list_of_graphics)
{
	int return_code,old_position;

	ENTER(cmzn_graphics_modify_in_list);
	if (graphics&&new_graphics&&list_of_graphics)
	{
		if (IS_OBJECT_IN_LIST(cmzn_graphics)(graphics,list_of_graphics))
		{
			/* save the current position */
			old_position=graphics->position;
			return_code=cmzn_graphics_copy_without_graphics_object(graphics,new_graphics);
			graphics->position=old_position;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"cmzn_graphics_modify_in_list.  graphics not in list");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_modify_in_list.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_graphics_modify_in_list */

DECLARE_OBJECT_FUNCTIONS(cmzn_graphics);

/** functor for ordering cmzn_set<GT_object> by position */
struct cmzn_graphics_compare_position_functor
{
	bool operator() (const cmzn_graphics* graphics1, const cmzn_graphics* graphics2) const
	{
		return (graphics1->position < graphics2->position);
	}
};

typedef cmzn_set<cmzn_graphics *,cmzn_graphics_compare_position_functor> cmzn_set_cmzn_graphics;

DECLARE_INDEXED_LIST_STL_FUNCTIONS(cmzn_graphics)

PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(/*object_type*/cmzn_graphics, /*identifier*/position, /*identifier_type*/int)
{
	if (list)
	{
		CMZN_SET(cmzn_graphics) *cmiss_set = reinterpret_cast<CMZN_SET(cmzn_graphics) *>(list);
		for (CMZN_SET(cmzn_graphics)::iterator iter = cmiss_set->begin(); iter != cmiss_set->end(); ++iter)
		{
			if ((*iter)->position == position)
			{
				return *iter;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE, "FIND_BY_IDENTIFIER_IN_LIST(cmzn_graphics,position).  Invalid argument");
	}
	return (0);
}

#if defined (USE_OPENCASCADE)

int cmzn_graphics_selects_cad_primitives(struct cmzn_graphics *graphics)
{
	int return_code;

	if (graphics)
	{
		return_code=(CMZN_GRAPHICS_SELECT_MODE_OFF != graphics->select_mode)&&(
			(CMZN_GRAPHICS_TYPE_LINES==graphics->graphics_type)||
			(CMZN_GRAPHICS_TYPE_SURFACES==graphics->graphics_type));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_selects_cad_primitives.  Invalid argument(s)");
		return_code=0;
	}

	return (return_code);
}

#endif /*defined (USE_OPENCASCADE) */

bool cmzn_graphics_selects_elements(struct cmzn_graphics *graphics)
{
	return (CMZN_GRAPHICS_SELECT_MODE_OFF != graphics->select_mode) &&
		(0 < cmzn_graphics_get_domain_dimension(graphics));
}

cmzn_scene_id cmzn_graphics_get_scene(cmzn_graphics_id graphics)
{
	if (graphics)
		return graphics->scene->access();
	return 0;
}

enum cmzn_scenecoordinatesystem cmzn_graphics_get_scenecoordinatesystem(
	struct cmzn_graphics *graphics)
{
	if (graphics)
		return graphics->coordinate_system;
	return CMZN_SCENECOORDINATESYSTEM_INVALID;
}

int cmzn_graphics_set_scenecoordinatesystem(
	struct cmzn_graphics *graphics, enum cmzn_scenecoordinatesystem coordinate_system)
{
	if (graphics)
	{
		if (coordinate_system != graphics->coordinate_system)
		{
			graphics->coordinate_system = coordinate_system;
			if (cmzn_scenecoordinatesystem_is_window_relative(coordinate_system))
			{
				graphics->overlay_flag = 1;
				graphics->overlay_order = 1;
			}
			else
			{
				graphics->overlay_flag = 0;
				graphics->overlay_order = 0;
			}
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_REDRAW);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

bool cmzn_graphics_get_visibility_flag(struct cmzn_graphics *graphics)
{
	if (graphics)
	{
		return graphics->visibility_flag;
	}
	return false;
}

int cmzn_graphics_set_visibility_flag(struct cmzn_graphics *graphics,
	bool visibility_flag)
{
	if (graphics)
	{
		if (graphics->visibility_flag != visibility_flag)
		{
			graphics->visibility_flag = visibility_flag;
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_REDRAW);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_graphics_and_scene_visibility_flags_is_set(struct cmzn_graphics *graphics)
{
	int return_code;

	ENTER(cmzn_graphics_and_scene_visibility_flags_set);
	if (graphics)
	{
		if (graphics->visibility_flag && cmzn_scene_is_visible_hierarchical(graphics->scene))
		{
			return_code = 1;
		}
		else
		{
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_and_scene_visibility_flags_set.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
}

int cmzn_graphics_is_from_region_hierarchical(struct cmzn_graphics *graphics, struct cmzn_region *region)
{
	int return_code = 0;

	ENTER(cmzn_graphics_is_from_region_hierarchical);
	if (graphics && region)
	{
		struct cmzn_region *scene_region = cmzn_scene_get_region_internal(graphics->scene);
		if ((scene_region == region) ||
			(cmzn_region_contains_subregion(region, scene_region)))
		{
			return_code = 1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_is_from_region_hierarchical.  Invalid argument(s)");
	}

	return (return_code);
}

cmzn_field_id cmzn_graphics_get_coordinate_field(cmzn_graphics_id graphics)
{
	cmzn_field_id coordinate_field = 0;
	if (graphics)
	{
		if (graphics->coordinate_field)
		{
			coordinate_field = ACCESS(Computed_field)(graphics->coordinate_field);
		}
	}
	return (coordinate_field);
}

int cmzn_graphics_set_coordinate_field(cmzn_graphics_id graphics,
	cmzn_field_id coordinate_field)
{
	if (graphics && ((0 == coordinate_field) ||
		(3 >= cmzn_field_get_number_of_components(coordinate_field))))
	{
		if (coordinate_field != graphics->coordinate_field)
		{
			if (graphics->scene && graphics->coordinate_field)
			{
				graphics->scene->deregisterCoordinateField(graphics->coordinate_field);
				if (graphics->point_orientation_scale_field)
					graphics->scene->deregisterVectorField(
						graphics->point_orientation_scale_field, graphics->coordinate_field);
				if (graphics->stream_vector_field)
					graphics->scene->deregisterVectorField(
						graphics->stream_vector_field, graphics->coordinate_field);
			}
			REACCESS(Computed_field)(&(graphics->coordinate_field), coordinate_field);
			if (graphics->scene && graphics->coordinate_field)
			{
				graphics->scene->registerCoordinateField(graphics->coordinate_field);
				if (graphics->point_orientation_scale_field)
					graphics->scene->registerVectorField(
						graphics->point_orientation_scale_field, graphics->coordinate_field);
				if (graphics->stream_vector_field)
					graphics->scene->registerVectorField(
						graphics->stream_vector_field, graphics->coordinate_field);
			}
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_field_id cmzn_graphics_get_data_field(cmzn_graphics_id graphics)
{
	cmzn_field_id data_field = 0;
	if (graphics)
	{
		if (graphics->data_field)
		{
			data_field = ACCESS(Computed_field)(graphics->data_field);
		}
	}
	return (data_field);
}

int cmzn_graphics_set_data_field(cmzn_graphics_id graphics,
	cmzn_field_id data_field)
{
	int return_code = 0;
	if (graphics)
	{
		if (data_field != graphics->data_field)
		{
			REACCESS(Computed_field)(&(graphics->data_field), data_field);
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
		}
		return_code = 1;
	}
	return (return_code);
}

bool cmzn_graphics_is_exterior(cmzn_graphics_id graphics)
{
	if (graphics)
		return graphics->exterior;
	return false;
}

int cmzn_graphics_set_exterior(cmzn_graphics_id graphics, bool exterior)
{
	if (graphics)
	{
		if (exterior != graphics->exterior)
		{
			graphics->exterior = exterior;
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

enum cmzn_element_face_type cmzn_graphics_get_element_face_type(cmzn_graphics_id graphics)
{
	if (graphics)
		return graphics->face;
	return CMZN_ELEMENT_FACE_TYPE_INVALID;
}

int cmzn_graphics_set_element_face_type(cmzn_graphics_id graphics, enum cmzn_element_face_type face_type)
{
	if (graphics)
	{
		if (face_type != graphics->face)
		{
			graphics->face = face_type;
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_graphics_update_selected(struct cmzn_graphics *graphics, void *dummy_void)
{
	USE_PARAMETER(dummy_void);
	if (graphics)
	{
		switch (graphics->select_mode)
		{
		case CMZN_GRAPHICS_SELECT_MODE_ON:
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_SELECTION);
			break;
		case CMZN_GRAPHICS_SELECT_MODE_OFF:
			/* nothing to do as selection doesn't affect appearance in this mode */
			break;
		case CMZN_GRAPHICS_SELECT_MODE_DRAW_SELECTED:
		case CMZN_GRAPHICS_SELECT_MODE_DRAW_UNSELECTED:
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
			break;
		default:
			display_message(ERROR_MESSAGE,
				"cmzn_graphics_update_selected.  Unknown select_mode");
			break;
		}
		return 1;
	}
	return 0;
}

/** update 'trivial' attribute glyph graphics object */
void cmzn_graphics_update_graphics_object_trivial_glyph(struct cmzn_graphics *graphics)
{
	if (graphics && graphics->graphics_object &&
		(CMZN_GRAPHICS_TYPE_POINTS == graphics->graphics_type))
	{
		if (graphics->glyph)
		{
			GT_object *glyph_gt_object = graphics->glyph->getGraphicsObject(graphics->tessellation, graphics->material, graphics->font);
			set_GT_object_glyph(graphics->graphics_object, glyph_gt_object);
			DEACCESS(GT_object)(&glyph_gt_object);
		}
		else
		{
			set_GT_object_glyph(graphics->graphics_object, static_cast<GT_object*>(0));
		}
	}
}

/** replace materials, spectrum and other 'trivial' attributes of existing
  * graphics object so it doesn't need complete rebuilding */
int cmzn_graphics_update_graphics_object_trivial(struct cmzn_graphics *graphics)
{
	int return_code = 0;
	if (graphics && graphics->graphics_object)
	{
		set_GT_object_default_material(graphics->graphics_object,
			graphics->material);
		set_GT_object_secondary_material(graphics->graphics_object,
			graphics->secondary_material);
		set_GT_object_selected_material(graphics->graphics_object,
			graphics->selected_material);
		set_GT_object_Spectrum(graphics->graphics_object, graphics->spectrum);
		if (CMZN_GRAPHICS_TYPE_POINTS == graphics->graphics_type)
		{
			cmzn_graphics_update_graphics_object_trivial_glyph(graphics);
			set_GT_object_glyph_repeat_mode(graphics->graphics_object, graphics->glyph_repeat_mode);
			Triple base_size, scale_factors, offset, label_offset;
			for (int i = 0; i < 3; ++i)
			{
				base_size[i] = static_cast<GLfloat>(graphics->point_base_size[i]);
				scale_factors[i] = static_cast<GLfloat>(graphics->point_scale_factors[i]);
				offset[i] = static_cast<GLfloat>(graphics->point_offset[i]);
				label_offset[i] = static_cast<GLfloat>(graphics->label_offset[i]);
			}
			set_GT_object_glyph_base_size(graphics->graphics_object, base_size);
			set_GT_object_glyph_scale_factors(graphics->graphics_object, scale_factors);
			set_GT_object_glyph_offset(graphics->graphics_object, offset);
			set_GT_object_font(graphics->graphics_object, graphics->font);
			set_GT_object_glyph_label_offset(graphics->graphics_object, label_offset);
			set_GT_object_glyph_label_text(graphics->graphics_object, graphics->label_text);
		}
		set_GT_object_render_polygon_mode(graphics->graphics_object, graphics->render_polygon_mode);
		set_GT_object_render_line_width(graphics->graphics_object, graphics->render_line_width);
		set_GT_object_render_point_size(graphics->graphics_object, graphics->render_point_size);
		return_code = 1;
	}
	return return_code;
}

cmzn_material_id cmzn_graphics_get_material(
	cmzn_graphics_id graphics)
{
	if (graphics)
	{
		return ACCESS(cmzn_material)(graphics->material);
	}
	return 0;
}

int cmzn_graphics_set_material(cmzn_graphics_id graphics,
	cmzn_material_id material)
{
	if (graphics && material)
	{
		if (material != graphics->material)
		{
			REACCESS(cmzn_material)(&(graphics->material), material);
			cmzn_graphics_update_graphics_object_trivial(graphics);
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_RECOMPILE);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_material *cmzn_graphics_get_selected_material(
	struct cmzn_graphics *graphics)
{
	if (graphics)
	{
		return ACCESS(cmzn_material)(graphics->selected_material);
	}
	return 0;
}

int cmzn_graphics_set_selected_material(cmzn_graphics_id graphics,
	cmzn_material_id selected_material)
{
	if (graphics && selected_material)
	{
		if (selected_material != graphics->selected_material)
		{
			REACCESS(cmzn_material)(&(graphics->selected_material),
				selected_material);
			cmzn_graphics_update_graphics_object_trivial(graphics);
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_RECOMPILE);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

char *cmzn_graphics_get_name(cmzn_graphics_id graphics)
{
	char *name = NULL;
	if (graphics && graphics->name)
	{
		name = duplicate_string(graphics->name);
	}

	return name;
}

char *cmzn_graphics_get_name_internal(struct cmzn_graphics *graphics)
{
	char *name = 0;
	if (graphics)
	{
		if (graphics->name)
		{
			name = duplicate_string(graphics->name);
		}
		else
		{
			char temp[30];
			sprintf(temp, "%d", graphics->position);
			name = duplicate_string(temp);
		}
	}
	return name;
}

int cmzn_graphics_set_name(struct cmzn_graphics *graphics, const char *name)
{
	if (graphics)
	{
		if (graphics->name)
		{
			DEALLOCATE(graphics->name);
		}
		if (name)
		{
			graphics->name = duplicate_string(name);
		}
		else
		{
			graphics->name = 0;
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

char *cmzn_graphics_get_summary_string(struct cmzn_graphics *graphics)
{
	if (!graphics)
		return 0;
	char *graphics_string = 0;
	int error = 0;
	char temp_string[100];
	if (graphics->name)
	{
		sprintf(temp_string, "%s. ", graphics->name);
	}
	else
	{
		sprintf(temp_string, "%i. ", graphics->position);
	}
	append_string(&graphics_string, temp_string, &error);
	append_string(&graphics_string,
		ENUMERATOR_STRING(cmzn_graphics_type)(graphics->graphics_type),
		&error);
	append_string(&graphics_string, " ", &error);
	append_string(&graphics_string, ENUMERATOR_STRING(cmzn_field_domain_type)(graphics->domain_type), &error);
	if (graphics->subgroup_field)
	{
		char *name = cmzn_field_get_name(graphics->subgroup_field);
		append_string(&graphics_string, " subgroup ", &error);
		append_string(&graphics_string, name, &error);
		DEALLOCATE(name);
	}
	return graphics_string;
}

char *cmzn_graphics_string(struct cmzn_graphics *graphics,
	enum cmzn_graphics_string_details graphics_detail)
{
	char *graphics_string = NULL,temp_string[100],*name;
	int error,i;

	ENTER(cmzn_graphics_string);
	graphics_string=(char *)NULL;
	if (graphics&&(
		(GRAPHICS_STRING_GEOMETRY==graphics_detail)||
		(GRAPHICS_STRING_COMPLETE==graphics_detail)||
		(GRAPHICS_STRING_COMPLETE_PLUS==graphics_detail)))
	{
		error=0;
		if (GRAPHICS_STRING_COMPLETE_PLUS==graphics_detail)
		{
			if (graphics->name)
			{
				sprintf(temp_string,"%i. (%s) ",graphics->position, graphics->name);
			}
			else
			{
				sprintf(temp_string,"%i. ",graphics->position);
			}
			append_string(&graphics_string,temp_string,&error);
		}

		/* show geometry graphics */
		/* for all graphics types */
		/* write graphics type = "points", "lines" etc. */
		append_string(&graphics_string,
			ENUMERATOR_STRING(cmzn_graphics_type)(graphics->graphics_type),
			&error);
		append_string(&graphics_string, " ", &error);
		append_string(&graphics_string, ENUMERATOR_STRING(cmzn_field_domain_type)(graphics->domain_type), &error);
		if (graphics->name)
		{
			sprintf(temp_string," as %s", graphics->name);
			append_string(&graphics_string,temp_string,&error);
		}
		if (graphics->subgroup_field)
		{
			if (GET_NAME(Computed_field)(graphics->subgroup_field,&name))
			{
				/* put quotes around name if it contains special characters */
				make_valid_token(&name);
				append_string(&graphics_string," subgroup ",&error);
				append_string(&graphics_string,name,&error);
				DEALLOCATE(name);
			}
		}
		if (graphics->coordinate_field)
		{
			append_string(&graphics_string," coordinate ",&error);
			name=(char *)NULL;
			if (GET_NAME(Computed_field)(graphics->coordinate_field,&name))
			{
				/* put quotes around name if it contains special characters */
				make_valid_token(&name);
				append_string(&graphics_string,name,&error);
				DEALLOCATE(name);
			}
			else
			{
				append_string(&graphics_string,"NONE",&error);
			}
		}

		/* for 1-D and 2-D elements only */
		const int domain_dimension = cmzn_graphics_get_domain_dimension(graphics);
		if ((1 == domain_dimension) || (2 == domain_dimension))
		{
			if (graphics->exterior)
			{
				append_string(&graphics_string, " exterior", &error);
			}
			append_string(&graphics_string, " face ", &error);
			append_string(&graphics_string, ENUMERATOR_STRING(cmzn_element_face_type)(graphics->face), &error);
		}

		append_string(&graphics_string, " tessellation ", &error);
		if (graphics->tessellation)
		{
			name = cmzn_tessellation_get_name(graphics->tessellation);
			/* put quotes around name if it contains special characters */
			make_valid_token(&name);
			append_string(&graphics_string, name, &error);
			DEALLOCATE(name);
		}
		else
		{
			append_string(&graphics_string,"NONE",&error);
		}

		append_string(&graphics_string," ",&error);
		append_string(&graphics_string,
			ENUMERATOR_STRING(cmzn_scenecoordinatesystem)(graphics->coordinate_system),&error);

		if ((graphics->render_line_width < 0.99999) || (1.00001 < graphics->render_line_width))
		{
			sprintf(temp_string, " line_width %g", graphics->render_line_width);
			append_string(&graphics_string,temp_string,&error);
		}
		if ((graphics->render_point_size < 0.99999) || (1.00001 < graphics->render_point_size))
		{
			sprintf(temp_string, " point_size %g", graphics->render_point_size);
			append_string(&graphics_string,temp_string,&error);
		}

		/* overlay is temporarily disabled, instead the functionality is replaced
			 by coordinate_system
		if (CMZN_GRAPHICS_STATIC==graphics->graphics_type)
		{
			if (graphics->overlay_flag == 0 )
			{
				append_string(&graphics_string, " no_overlay ",&error);
			}
			else
			{
				sprintf(temp_string, " overlay %d", graphics->overlay_order);
				append_string(&graphics_string,temp_string,&error);
			}
		}
		*/

		if (CMZN_GRAPHICS_TYPE_CONTOURS == graphics->graphics_type)
		{
			if (graphics->isoscalar_field)
			{
				if (GET_NAME(Computed_field)(graphics->isoscalar_field,&name))
				{
					/* put quotes around name if it contains special characters */
					make_valid_token(&name);
					append_string(&graphics_string," iso_scalar ",&error);
					append_string(&graphics_string,name,&error);
					DEALLOCATE(name);
				}
				else
				{
					DEALLOCATE(graphics_string);
					error=1;
				}
			}
			if (graphics->isovalues)
			{
				sprintf(temp_string," iso_values");
				append_string(&graphics_string,temp_string,&error);
				for (i = 0 ; i < graphics->number_of_isovalues ; i++)
				{
					sprintf(temp_string, " %g", graphics->isovalues[i]);
					append_string(&graphics_string,temp_string,&error);
				}
			}
			else
			{
				sprintf(temp_string," range_number_of_iso_values %d",
					graphics->number_of_isovalues);
				append_string(&graphics_string,temp_string,&error);
				sprintf(temp_string," first_iso_value %g",
					graphics->first_isovalue);
				append_string(&graphics_string,temp_string,&error);
				sprintf(temp_string," last_iso_value %g",
					graphics->last_isovalue);
				append_string(&graphics_string,temp_string,&error);
			}
			if (graphics->decimation_threshold > 0.0)
			{
				sprintf(temp_string," decimation_threshold %g",
					graphics->decimation_threshold);
				append_string(&graphics_string,temp_string,&error);
			}
		}

		// line attributes
		if ((graphics->graphics_type == CMZN_GRAPHICS_TYPE_LINES) ||
			(graphics->graphics_type == CMZN_GRAPHICS_TYPE_STREAMLINES))
		{
			append_string(&graphics_string," ",&error);
			append_string(&graphics_string,
				ENUMERATOR_STRING(cmzn_graphicslineattributes_shape_type)(graphics->line_shape),&error);

			append_string(&graphics_string, " line_base_size ", &error);
			if (graphics->line_base_size[1] == graphics->line_base_size[0])
			{
				sprintf(temp_string, "%g", graphics->line_base_size[0]);
			}
			else
			{
				sprintf(temp_string, "\"%g*%g\"", graphics->line_base_size[0], graphics->line_base_size[1]);
			}
			append_string(&graphics_string, temp_string, &error);

			if (graphics->line_orientation_scale_field)
			{
				name = cmzn_field_get_name(graphics->line_orientation_scale_field);
				/* put quotes around name if it contains special characters */
				make_valid_token(&name);
				append_string(&graphics_string, " line_orientation_scale ", &error);
				append_string(&graphics_string, name, &error);
				DEALLOCATE(name);

				append_string(&graphics_string, " line_scale_factors ", &error);
				if (graphics->line_scale_factors[1] == graphics->line_scale_factors[0])
				{
					sprintf(temp_string,"%g", graphics->line_scale_factors[0]);
				}
				else
				{
					sprintf(temp_string,"\"%g*%g\"", graphics->line_scale_factors[0], graphics->line_scale_factors[1]);
				}
				append_string(&graphics_string,temp_string,&error);
			}
		}

		// point attributes
		if (CMZN_GRAPHICS_TYPE_POINTS == graphics->graphics_type)
		{
			append_string(&graphics_string," glyph ",&error);
			if (graphics->glyph)
			{
				name = cmzn_glyph_get_name(graphics->glyph);
				append_string(&graphics_string,name,&error);
				DEALLOCATE(name);
			}
			else
			{
				append_string(&graphics_string, "none", &error);
			}
			if (graphics->glyph_repeat_mode != CMZN_GLYPH_REPEAT_MODE_NONE)
			{
				append_string(&graphics_string, " ", &error);
				append_string(&graphics_string,
					ENUMERATOR_STRING(cmzn_glyph_repeat_mode)(graphics->glyph_repeat_mode), &error);
			}
			sprintf(temp_string," size \"%g*%g*%g\"",graphics->point_base_size[0],
				graphics->point_base_size[1],graphics->point_base_size[2]);
			append_string(&graphics_string,temp_string,&error);
			sprintf(temp_string," offset %g,%g,%g",
				graphics->point_offset[0], graphics->point_offset[1], graphics->point_offset[2]);
			append_string(&graphics_string,temp_string,&error);
			if (graphics->font)
			{
				append_string(&graphics_string," font ",&error);
				GET_NAME(cmzn_font)(graphics->font, &name);
				make_valid_token(&name);
				append_string(&graphics_string,name,&error);
				DEALLOCATE(name);
			}
			if (graphics->label_field)
			{
				name = cmzn_field_get_name(graphics->label_field);
				make_valid_token(&name);
				append_string(&graphics_string," label ",&error);
				append_string(&graphics_string,name,&error);
				DEALLOCATE(name);
			}
			const int number_of_glyphs =
				cmzn_glyph_repeat_mode_get_number_of_glyphs(graphics->glyph_repeat_mode);
			int last_glyph_number_with_label_text = -1;
			for (int glyph_number = 0; glyph_number < number_of_glyphs; ++glyph_number)
			{
				if (cmzn_glyph_repeat_mode_glyph_number_has_label(graphics->glyph_repeat_mode, glyph_number) &&
					(0 != graphics->label_text[glyph_number]))
				{
					last_glyph_number_with_label_text = glyph_number;
				}
			}
			if (graphics->label_field || (last_glyph_number_with_label_text >= 0))
			{
				sprintf(temp_string," label_offset %g,%g,%g",graphics->label_offset[0],
					graphics->label_offset[1],graphics->label_offset[2]);
				append_string(&graphics_string,temp_string,&error);
			}
			if (last_glyph_number_with_label_text >= 0)
			{
				append_string(&graphics_string, " label_text ", &error);
				int number_of_labels = 0;
				for (int glyph_number = 0; glyph_number <= last_glyph_number_with_label_text; ++glyph_number)
				{
					if (cmzn_glyph_repeat_mode_glyph_number_has_label(graphics->glyph_repeat_mode, glyph_number))
					{
						if (number_of_labels > 0)
						{
							append_string(&graphics_string, " & ", &error);
						}
						if (graphics->label_text[number_of_labels])
						{
							char *label_text = duplicate_string(graphics->label_text[number_of_labels]);
							make_valid_token(&label_text);
							append_string(&graphics_string, label_text, &error);
							DEALLOCATE(label_text);
						}
						else
						{
							append_string(&graphics_string, "\"\"", &error);
						}
						++number_of_labels;
					}
				}
			}
			if (graphics->label_density_field)
			{
				append_string(&graphics_string," ldensity ",&error);
				GET_NAME(Computed_field)(graphics->label_density_field,&name);
				make_valid_token(&name);
				append_string(&graphics_string,name,&error);
				DEALLOCATE(name);
			}
			if (graphics->point_orientation_scale_field)
			{
				append_string(&graphics_string," orientation ",&error);
				GET_NAME(Computed_field)(graphics->point_orientation_scale_field,&name);
				make_valid_token(&name);
				append_string(&graphics_string,name,&error);
				DEALLOCATE(name);
			}
			if (graphics->signed_scale_field)
			{
				append_string(&graphics_string," variable_scale ",&error);
				GET_NAME(Computed_field)(graphics->signed_scale_field,&name);
				make_valid_token(&name);
				append_string(&graphics_string,name,&error);
				DEALLOCATE(name);
			}
			if (graphics->point_orientation_scale_field || graphics->signed_scale_field)
			{
				sprintf(temp_string," scale_factors \"%g*%g*%g\"",
					graphics->point_scale_factors[0],
					graphics->point_scale_factors[1],
					graphics->point_scale_factors[2]);
				append_string(&graphics_string,temp_string,&error);
			}
		}

		/* for sampling points only */
		if ((domain_dimension > 0) && (
			(CMZN_GRAPHICS_TYPE_POINTS == graphics->graphics_type) ||
			(CMZN_GRAPHICS_TYPE_STREAMLINES == graphics->graphics_type)))
		{
			append_string(&graphics_string," ",&error);
			append_string(&graphics_string, ENUMERATOR_STRING(cmzn_element_point_sampling_mode)(
				graphics->sampling_mode), &error);
			if (CMZN_ELEMENT_POINT_SAMPLING_MODE_SET_LOCATION != graphics->sampling_mode)
			{
				if (CMZN_ELEMENT_POINT_SAMPLING_MODE_CELL_POISSON == graphics->sampling_mode)
				{
					append_string(&graphics_string, " density ", &error);
					if (graphics->sample_density_field)
					{
						if (GET_NAME(Computed_field)(graphics->sample_density_field,&name))
						{
							/* put quotes around name if it contains special characters */
							make_valid_token(&name);
							append_string(&graphics_string, name, &error);
							DEALLOCATE(name);
						}
						else
						{
							DEALLOCATE(graphics_string);
							error = 1;
						}
					}
					else
					{
						append_string(&graphics_string,"NONE",&error);
					}
				}
			}
		}

		if (domain_dimension > 0)
		{
			if (graphics->tessellation_field)
			{
				append_string(&graphics_string," native_discretization ", &error);
				name = cmzn_field_get_name(graphics->tessellation_field);
				/* put quotes around name if it contains special characters */
				make_valid_token(&name);
				append_string(&graphics_string,name,&error);
				DEALLOCATE(name);
			}
		}

		/* for graphics starting in a particular element */
		if (CMZN_GRAPHICS_TYPE_STREAMLINES==graphics->graphics_type)
		{
			if (graphics->seed_element)
			{
				sprintf(temp_string, " seed_element %d", get_FE_element_identifier(graphics->seed_element));
				append_string(&graphics_string, temp_string, &error);
			}
		}

		/* for graphics requiring an exact xi location */
		if ((domain_dimension > 0) && (
			(CMZN_GRAPHICS_TYPE_POINTS == graphics->graphics_type) ||
			(CMZN_GRAPHICS_TYPE_STREAMLINES == graphics->graphics_type)) &&
			(CMZN_ELEMENT_POINT_SAMPLING_MODE_SET_LOCATION == graphics->sampling_mode))
		{
			sprintf(temp_string," xi %g,%g,%g",
				graphics->sample_location[0],graphics->sample_location[1],graphics->sample_location[2]);
			append_string(&graphics_string,temp_string,&error);
		}

		/* for streamlines only */
		if (CMZN_GRAPHICS_TYPE_STREAMLINES==graphics->graphics_type)
		{
			if (graphics->stream_vector_field)
			{
				if (GET_NAME(Computed_field)(graphics->stream_vector_field,&name))
				{
					/* put quotes around name if it contains special characters */
					make_valid_token(&name);
					append_string(&graphics_string," vector ",&error);
					append_string(&graphics_string,name,&error);
					DEALLOCATE(name);
				}
				else
				{
					DEALLOCATE(graphics_string);
					error=1;
				}
			}
			append_string(&graphics_string, " ", &error);
			append_string(&graphics_string,
				ENUMERATOR_STRING(cmzn_graphics_streamlines_track_direction)(graphics->streamlines_track_direction), &error);
			sprintf(temp_string," length %g ", graphics->streamline_length);
			append_string(&graphics_string,temp_string,&error);
			append_string(&graphics_string,
				ENUMERATOR_STRING(cmzn_graphics_streamlines_colour_data_type)(graphics->streamlines_colour_data_type),&error);
			if (graphics->seed_nodeset)
			{
				append_string(&graphics_string, " seed_nodeset ", &error);
				char *nodeset_name = cmzn_nodeset_get_name(graphics->seed_nodeset);
				make_valid_token(&nodeset_name);
				append_string(&graphics_string, nodeset_name, &error);
				DEALLOCATE(nodeset_name);
			}
			if (graphics->seed_node_mesh_location_field)
			{
				if (GET_NAME(Computed_field)(graphics->seed_node_mesh_location_field,&name))
				{
					/* put quotes around name if it contains special characters */
					make_valid_token(&name);
					append_string(&graphics_string," seed_node_mesh_location_field ",&error);
					append_string(&graphics_string,name,&error);
					DEALLOCATE(name);
				}
				else
				{
					DEALLOCATE(graphics_string);
					error=1;
				}
			}
		}
		append_string(&graphics_string," ",&error);
		append_string(&graphics_string,
			ENUMERATOR_STRING(cmzn_graphics_select_mode)(graphics->select_mode),&error);

		if ((GRAPHICS_STRING_COMPLETE==graphics_detail)||
			(GRAPHICS_STRING_COMPLETE_PLUS==graphics_detail))
		{
			/* show appearance graphics */
			/* for all graphics types */
			if (!graphics->visibility_flag)
			{
				append_string(&graphics_string," invisible",&error);
			}
			if (graphics->material&&
				GET_NAME(cmzn_material)(graphics->material,&name))
			{
				/* put quotes around name if it contains special characters */
				make_valid_token(&name);
				append_string(&graphics_string," material ",&error);
				append_string(&graphics_string,name,&error);
				DEALLOCATE(name);
			}
			if (graphics->secondary_material&&
				GET_NAME(cmzn_material)(graphics->secondary_material,&name))
			{
				/* put quotes around name if it contains special characters */
				make_valid_token(&name);
				append_string(&graphics_string," secondary_material ",&error);
				append_string(&graphics_string,name,&error);
				DEALLOCATE(name);
			}
			if (graphics->texture_coordinate_field)
			{
				if (GET_NAME(Computed_field)(graphics->texture_coordinate_field,&name))
				{
					/* put quotes around name if it contains special characters */
					make_valid_token(&name);
					append_string(&graphics_string," texture_coordinates ",&error);
					append_string(&graphics_string,name,&error);
					DEALLOCATE(name);
				}
				else
				{
					DEALLOCATE(graphics_string);
					error=1;
				}
			}
			if (graphics->data_field)
			{
				if (GET_NAME(Computed_field)(graphics->data_field,&name))
				{
					/* put quotes around name if it contains special characters */
					make_valid_token(&name);
					append_string(&graphics_string," data ",&error);
					append_string(&graphics_string,name,&error);
					DEALLOCATE(name);
				}
				else
				{
					DEALLOCATE(graphics_string);
					error=1;
				}
				if (graphics->spectrum&&
					GET_NAME(cmzn_spectrum)(graphics->spectrum,&name))
				{
					/* put quotes around name if it contains special characters */
					make_valid_token(&name);
					append_string(&graphics_string," spectrum ",&error);
					append_string(&graphics_string,name,&error);
					DEALLOCATE(name);
				}
			}
			if (graphics->selected_material&&
				GET_NAME(cmzn_material)(graphics->selected_material,&name))
			{
				/* put quotes around name if it contains special characters */
				make_valid_token(&name);
				append_string(&graphics_string," selected_material ",&error);
				append_string(&graphics_string,name,&error);
				DEALLOCATE(name);
			}
			/* for surfaces rendering */
			append_string(&graphics_string," ",&error);
			append_string(&graphics_string,
				ENUMERATOR_STRING(cmzn_graphics_render_polygon_mode)(graphics->render_polygon_mode),&error);
		}
		if (error)
		{
			display_message(ERROR_MESSAGE,
				"cmzn_graphics_string.  Error creating string");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_string.  Invalid argument(s)");
	}
	LEAVE;

	return graphics_string;
} /* cmzn_graphics_string */


int cmzn_graphics_to_point_vertex_buffer(struct cmzn_graphics *graphics,
	struct cmzn_graphics_to_graphics_object_data *graphics_to_object_data)
{
	int return_code = 1;

	if (graphics && graphics_to_object_data)
	{
		FE_value coordinates[3] = { 0.0, 0.0, 0.0 };
		if (graphics->coordinate_field)
		{
			if (CMZN_OK != cmzn_field_evaluate_real(graphics->coordinate_field, graphics_to_object_data->field_cache, 3, coordinates))
				return 0;
		}
		FE_value a[3], b[3], c[3], size[3];
		FE_value orientationScale[9];
		int orientationScaleComponentCount = 0;
		if (graphics->point_orientation_scale_field)
		{
			orientationScaleComponentCount = cmzn_field_get_number_of_components(graphics->point_orientation_scale_field);
			if (CMZN_OK != cmzn_field_evaluate_real(graphics->point_orientation_scale_field,
				graphics_to_object_data->field_cache, orientationScaleComponentCount, orientationScale))
			{
				display_message(WARNING_MESSAGE, "Orientation scale field not defined at point");
			}
		}
		if (!make_glyph_orientation_scale_axes(orientationScaleComponentCount,
			orientationScale, a, b, c, size))
		{
			display_message(WARNING_MESSAGE, "Invalid orientation scale at point");
		}
		if (graphics->signed_scale_field)
		{
			FE_value signedScale[3];
			if (CMZN_OK == cmzn_field_evaluate_real(graphics->signed_scale_field,
				graphics_to_object_data->field_cache, /*number_of_values*/3, signedScale))
			{
				const int componentCount = cmzn_field_get_number_of_components(graphics->signed_scale_field);
				for (int j = 0; j < componentCount; j++)
				{
					size[j] *= signedScale[j];
				}
			}
			else
			{
				display_message(WARNING_MESSAGE, "Variable/signed scale field not defined at point");
			}
		}
		FE_value *data = 0;
		int dataComponentCount = 0;
		if (graphics->data_field)
		{
			dataComponentCount = cmzn_field_get_number_of_components(graphics->data_field);
			data = new FE_value[dataComponentCount];
			if (CMZN_OK != cmzn_field_evaluate_real(graphics->data_field,
				graphics_to_object_data->field_cache, dataComponentCount, data))
			{
				display_message(WARNING_MESSAGE, "Data field not defined at point");
			}
		}
		char **labels = 0;
		if (graphics->label_field)
		{
			ALLOCATE(labels, char *, 1);
			*labels = cmzn_field_evaluate_string(graphics->label_field, graphics_to_object_data->field_cache);
		}
		GT_object_clear_primitives(graphics->graphics_object);
		Triple *point_list, *axis1_list, *axis2_list, *axis3_list,
			*scale_list;
		ALLOCATE(point_list, Triple, 1);
		ALLOCATE(axis1_list, Triple, 1);
		ALLOCATE(axis2_list, Triple, 1);
		ALLOCATE(axis3_list, Triple, 1);
		ALLOCATE(scale_list, Triple, 1);
		for (int j = 0; j < 3; j++)
		{
			(*point_list)[j] = static_cast<GLfloat>(coordinates[j]);
			(*axis1_list)[j] = static_cast<GLfloat>(a[j]);
			(*axis2_list)[j] = static_cast<GLfloat>(b[j]);
			(*axis3_list)[j] = static_cast<GLfloat>(c[j]);
			(*scale_list)[j] = static_cast<GLfloat>(size[j]);
		}
		GLfloat *floatData = 0;
		if (data)
		{
			ALLOCATE(floatData, GLfloat, dataComponentCount);
			for (int i = 0; i < dataComponentCount; ++i)
			{
				floatData[i] = static_cast<GLfloat>(data[i]);
			}
		}
		Triple glyph_base_size, glyph_scale_factors, glyph_offset, glyph_label_offset;
		for (int i = 0; i < 3; i++)
		{
			glyph_base_size[i] = static_cast<GLfloat>(graphics->point_base_size[i]);
			glyph_scale_factors[i] = static_cast<GLfloat>(graphics->point_scale_factors[i]);
			glyph_offset[i] = static_cast<GLfloat>(graphics->point_offset[i]);
			glyph_label_offset[i] = static_cast<GLfloat>(graphics->label_offset[i]);
		}
		GT_glyphset_vertex_buffers *glyphset =
			CREATE(GT_glyphset_vertex_buffers)();
		GT_glyphset_vertex_buffers_setup(glyphset,
			graphics_to_object_data->glyph_gt_object, graphics->glyph_repeat_mode,
			glyph_base_size, glyph_scale_factors, glyph_offset, graphics->font,
			glyph_label_offset, graphics->label_text,
			/*label_bounds_dimension*/0, /*label_bounds_components*/0);
		if (0 == fill_glyph_graphics_vertex_array(
			GT_object_get_vertex_set(graphics->graphics_object), /*vertex_location*/-1,
			1, point_list,	axis1_list, axis2_list, axis3_list, scale_list,
			dataComponentCount, floatData, 0, /*object_name*/-1, 0, labels, 0, 0, 0))
		{
			DESTROY(GT_glyphset_vertex_buffers)(&glyphset);
		}
		if (glyphset)
		{
			if (!GT_OBJECT_ADD(GT_glyphset_vertex_buffers)(
				graphics->graphics_object, glyphset))
			{
				DESTROY(GT_glyphset_vertex_buffers)(&glyphset);
				return_code=0;
			}
		}

		DEALLOCATE(point_list);
		DEALLOCATE(axis1_list);
		DEALLOCATE(axis2_list);
		DEALLOCATE(axis3_list);
		DEALLOCATE(scale_list);
		if (labels)
		{
			DEALLOCATE(labels);
		}
		if (floatData)
			DEALLOCATE(floatData);
		delete[] data;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_to_point.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
}

#if defined (USE_OPENCASCADE)
static int Cad_shape_to_graphics_object(struct Computed_field *field,
	struct cmzn_graphics_to_graphics_object_data *graphics_to_object_data)
{
	int return_code = 0;
	GLfloat time = 0.0;
	struct cmzn_graphics *graphics = graphics_to_object_data->graphics;
	cmzn_field_cad_topology_id cad_topology = cmzn_field_cast_cad_topology(field);

	if (cad_topology)
	{
		switch (graphics->graphics_type)
		{
			case CMZN_GRAPHICS_TYPE_SURFACES:
			{
				//printf( "Building cad geometry surfaces\n" );
				int surface_count = cmzn_field_cad_topology_get_surface_count(cad_topology);
				if (surface_count > 0)
				{
					return_code = 1;
				}
				for (int i = 0; i < surface_count && return_code; i++)
				{
					cmzn_cad_surface_identifier identifier = i;
					struct GT_surface *surface = create_surface_from_cad_shape(cad_topology, graphics_to_object_data->field_cache, graphics_to_object_data->rc_coordinate_field, graphics->data_field, graphics->render_polygon_mode, identifier);
					if (surface && GT_OBJECT_ADD(GT_surface)(graphics->graphics_object, time, surface))
					{
						//printf( "Surface added to graphics object\n" );
						return_code = 1;
					}
					else
					{
						return_code = 0;
					}
				}
				break;
			}
			case CMZN_GRAPHICS_TYPE_LINES:
			{
				//struct GT_object *graphics_object = settings->graphics_object;
				/*
				GT_polyline_vertex_buffers *lines =
					CREATE(GT_polyline_vertex_buffers)(
					g_PLAIN);
				*/
				GT_polyline_vertex_buffers *lines = create_curves_from_cad_shape(cad_topology, graphics_to_object_data->field_cache, graphics_to_object_data->rc_coordinate_field, graphics->data_field, graphics->graphics_object);
				if (lines && GT_OBJECT_ADD(GT_polyline_vertex_buffers)(
					graphics->graphics_object, lines))
				{
					//printf("Adding lines for cad shape\n");
					return_code = 1;
				}
				else
				{
					//DESTROY(GT_polyline_vertex_buffers)(&lines);
					return_code = 0;
				}
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,"Cad_geometry_to_graphics_object.  "
					"Can't handle this type of graphics");
				return_code = 0;
			}
		}
		cmzn_field_destroy((cmzn_field_id*)&cad_topology);
	}

	return return_code;
}
#endif /* (USE_OPENCASCADE) */

#if defined (USE_OPENCASCADE)
SubObjectGroupHighlightFunctor *create_highlight_functor_cad_primitive(
	struct Computed_field *group_field, cmzn_field_cad_topology_id cad_topology_domain)
{
	SubObjectGroupHighlightFunctor *highlight_functor = NULL;
	if (group_field)
	{
		cmzn_field_group_id sub_group = cmzn_field_cast_group(group_field);

		//cmzn_field_id cad_primitive_group_field = cmzn_field_group_get_cad_primitive_group(sub_group, cad_topology_domain);
		cmzn_field_id cad_primitive_subgroup_field = cmzn_field_group_get_subobject_group_field_for_domain_field(sub_group,
			reinterpret_cast<cmzn_field_id>(cad_topology_domain));
		cmzn_field_cad_primitive_group_template_id cad_primitive_group = NULL;
		if (cad_primitive_subgroup_field)
		{
			cad_primitive_group =
				cmzn_field_cast_cad_primitive_group_template(cad_primitive_subgroup_field);
			cmzn_field_destroy(&cad_primitive_subgroup_field);
			if (cad_primitive_group)
			{
				Computed_field_sub_group_object<cmzn_cad_identifier_id> *group_core =
					Computed_field_sub_group_object_core_cast<cmzn_cad_identifier_id,
					cmzn_field_cad_primitive_group_template_id>(cad_primitive_group);
				highlight_functor =
					new SubObjectGroupHighlightFunctor(group_core,
					&Computed_field_subobject_group::isIdentifierInList);
				cmzn_field_id temporary_handle =
					reinterpret_cast<Computed_field *>(cad_primitive_group);
				cmzn_field_destroy(&temporary_handle);
			}
		}
		if (sub_group)
		{
			cmzn_field_group_destroy(&sub_group);
		}
	}

	return (highlight_functor);
}
#endif /* defined (USE_OPENCASCADE) */

SubObjectGroupHighlightFunctor *create_highlight_functor_element(
	struct Computed_field *group_field, cmzn_mesh_id mesh)
{
	SubObjectGroupHighlightFunctor *highlight_functor = NULL;
	if (group_field)
	{
		cmzn_field_group_id sub_group = cmzn_field_cast_group(group_field);
		if (cmzn_field_group_contains_local_region(sub_group))
		{
			highlight_functor =	new SubObjectGroupHighlightFunctor(NULL, NULL);
			highlight_functor->setContainsAll(1);
		}
		else
		{
			cmzn_field_element_group_id element_group = cmzn_field_group_get_field_element_group(sub_group, mesh);
			if (element_group)
			{
				Computed_field_element_group *group_core =
					Computed_field_element_group_core_cast(element_group);
				highlight_functor =
					new SubObjectGroupHighlightFunctor(group_core,
					&Computed_field_subobject_group::containsIndex);
				cmzn_field_element_group_destroy(&element_group);
			}
		}
	if (sub_group)
		cmzn_field_group_destroy(&sub_group);
	}

	return (highlight_functor);
}

SubObjectGroupHighlightFunctor *create_highlight_functor_nodeset(
	struct Computed_field *group_field, cmzn_nodeset_id nodeset)
{
  SubObjectGroupHighlightFunctor *highlight_functor = NULL;
	if (group_field)
	{
	  cmzn_field_group_id sub_group = cmzn_field_cast_group(group_field);
	  if (cmzn_field_group_contains_local_region(sub_group))
	  {
		highlight_functor = new SubObjectGroupHighlightFunctor(NULL, NULL);
		highlight_functor->setContainsAll(1);
	  }
	  else
	  {
		cmzn_field_node_group_id node_group = cmzn_field_group_get_field_node_group(sub_group, nodeset);
		if (node_group)
		{
			Computed_field_node_group *group_core =
				Computed_field_node_group_core_cast(node_group);
			highlight_functor =	new SubObjectGroupHighlightFunctor(group_core,
				&Computed_field_subobject_group::isIdentifierInList);
			cmzn_field_node_group_destroy(&node_group);
		}
	  }
	if (sub_group)
		{
		cmzn_field_group_destroy(&sub_group);
		}
	}

	return (highlight_functor);
}

int cmzn_graphics_remove_renderer_highlight_functor(struct cmzn_graphics *graphics,
	void *renderer_void)
{
	Render_graphics *renderer = 0;
	if (graphics && (NULL != (renderer = (Render_graphics *)renderer_void)))
	{
		renderer->set_highlight_functor(NULL);
		return 1;
	}
	return 0;
}

int cmzn_graphics_set_renderer_highlight_functor(struct cmzn_graphics *graphics,
	void *renderer_void)
{
	int return_code = 0;

	Render_graphics *renderer = 0;
	if (graphics && (NULL != (renderer = (Render_graphics *)renderer_void)) && graphics->scene)
	{
		cmzn_field_id group_field =
			cmzn_scene_get_selection_group_private_for_highlighting(graphics->scene);
		cmzn_fieldmodule_id field_module = NULL;
		if (group_field &&
			(NULL != (field_module = cmzn_field_get_fieldmodule(group_field))))
		{
			if ((CMZN_GRAPHICS_SELECT_MODE_ON == graphics->select_mode) ||
				(CMZN_GRAPHICS_SELECT_MODE_DRAW_SELECTED == graphics->select_mode))
			{
				SubObjectGroupHighlightFunctor *functor = 0;
				switch (graphics->domain_type)
				{
				case CMZN_FIELD_DOMAIN_TYPE_POINT:
				{
					// no functor
				} break;
				case CMZN_FIELD_DOMAIN_TYPE_DATAPOINTS:
				case CMZN_FIELD_DOMAIN_TYPE_NODES:
				{
					cmzn_nodeset_id nodeset =
						cmzn_fieldmodule_find_nodeset_by_field_domain_type(field_module, graphics->domain_type);
					functor = create_highlight_functor_nodeset(group_field, nodeset);
					cmzn_nodeset_destroy(&nodeset);
				} break;
				case CMZN_FIELD_DOMAIN_TYPE_MESH1D:
				case CMZN_FIELD_DOMAIN_TYPE_MESH2D:
				case CMZN_FIELD_DOMAIN_TYPE_MESH3D:
				case CMZN_FIELD_DOMAIN_TYPE_MESH_HIGHEST_DIMENSION:
				{
#if defined(USE_OPENCASCADE)
					if (graphics->graphics_type == CMZN_GRAPHICS_TYPE_SURFACES)
					{
						// test here for domain of object coordinate_field
						// if it is a cad_geometry do something about it
						struct LIST(Computed_field) *domain_field_list = CREATE_LIST(Computed_field)();
						int return_code = Computed_field_get_domain( graphics->coordinate_field, domain_field_list );
						if ( return_code )
						{
							// so test for topology domain
							struct Computed_field *cad_topology_field = FIRST_OBJECT_IN_LIST_THAT(Computed_field)
											( cmzn_field_is_type_cad_topology, (void *)NULL, domain_field_list );
							if ( cad_topology_field )
							{
								cmzn_field_cad_topology_id cad_topology_domain =
									cmzn_field_cast_cad_topology(cad_topology_field);
								functor = create_highlight_functor_cad_primitive(
									group_field, cad_topology_domain);
							}
						}
						if ( domain_field_list )
							DESTROY_LIST(Computed_field)(&domain_field_list);
					}
					if (!functor)
					{
#endif // defined(USE_OPENCASCADE)
						if (graphics->graphics_type != CMZN_GRAPHICS_TYPE_STREAMLINES)
						{
							int dimension = cmzn_graphics_get_domain_dimension(graphics);
							cmzn_mesh_id temp_mesh = cmzn_fieldmodule_find_mesh_by_dimension(field_module, dimension);
							functor = create_highlight_functor_element(group_field, temp_mesh);
							cmzn_mesh_destroy(&temp_mesh);
						}
#if defined(USE_OPENCASCADE)
					}
#endif // defined(USE_OPENCASCADE)
				} break;
				default:
				{
					display_message(ERROR_MESSAGE,
						"cmzn_graphics_set_renderer_highlight_functor.  Unknown domain type");
				} break;
				}
				if (!(renderer->set_highlight_functor(functor)) && functor)
				{
					delete functor;
				}
			}
			cmzn_fieldmodule_destroy(&field_module);
		}
		return_code = 1;
	}

	return return_code;
}

int cmzn_graphics_get_iteration_domain(cmzn_graphics_id graphics,
	cmzn_graphics_to_graphics_object_data *graphics_to_object_data)
{
	if (!graphics || !graphics_to_object_data)
		return 0;
	graphics_to_object_data->master_mesh = 0;
	graphics_to_object_data->iteration_mesh = 0;
	int dimension = cmzn_graphics_get_domain_dimension(graphics);
	if (dimension > 0)
	{
		graphics_to_object_data->master_mesh =
			cmzn_fieldmodule_find_mesh_by_dimension(graphics_to_object_data->field_module, dimension);
		if (graphics->subgroup_field)
		{
			cmzn_field_group_id group = cmzn_field_cast_group(graphics->subgroup_field);
			if (group)
			{
				cmzn_field_element_group_id element_group = cmzn_field_group_get_field_element_group(group, graphics_to_object_data->master_mesh);
				if (element_group)
				{
					graphics_to_object_data->iteration_mesh =
						cmzn_mesh_group_base_cast(cmzn_field_element_group_get_mesh_group(element_group));
					cmzn_field_element_group_destroy(&element_group);
				}
				cmzn_field_group_destroy(&group);
			}
			else
			{
				cmzn_field_element_group_id element_group = cmzn_field_cast_element_group(graphics->subgroup_field);
				if (element_group)
				{
					// check group is for same master mesh
					graphics_to_object_data->iteration_mesh = cmzn_mesh_group_base_cast(cmzn_field_element_group_get_mesh_group(element_group));
					cmzn_mesh_id temp_master_mesh = cmzn_mesh_get_master_mesh(graphics_to_object_data->iteration_mesh);
					if (!cmzn_mesh_match(graphics_to_object_data->master_mesh, temp_master_mesh))
					{
						cmzn_mesh_destroy(&graphics_to_object_data->iteration_mesh);
					}
					cmzn_mesh_destroy(&temp_master_mesh);
					cmzn_field_element_group_destroy(&element_group);
				}
				else
				{
					graphics_to_object_data->iteration_mesh = cmzn_mesh_access(graphics_to_object_data->master_mesh);
				}
			}
		}
		else
		{
			graphics_to_object_data->iteration_mesh = cmzn_mesh_access(graphics_to_object_data->master_mesh);
		}
	}
	return (0 != graphics_to_object_data->iteration_mesh);
}

static char *cmzn_graphics_get_graphics_object_name(cmzn_graphics *graphics, const char *name_prefix)
{
	if (!graphics || !name_prefix)
		return 0;
	int error = 0;
	char *graphics_object_name = 0;
	append_string(&graphics_object_name, name_prefix, &error);
	if (graphics->subgroup_field)
	{
		char *subgroup_name = cmzn_field_get_name(graphics->subgroup_field);
		append_string(&graphics_object_name, subgroup_name, &error);
		append_string(&graphics_object_name, "/", &error);
		DEALLOCATE(subgroup_name);
	}
	append_string(&graphics_object_name, ".", &error);
	char temp[20];
	sprintf(temp, "%d", graphics->position);
	append_string(&graphics_object_name, temp, &error);
	if (graphics->name)
	{
		append_string(&graphics_object_name, "_", &error);
		append_string(&graphics_object_name, graphics->name, &error);
	}
	return graphics_object_name;
}

static int cmzn_mesh_to_graphics(cmzn_mesh_id mesh, cmzn_graphics_to_graphics_object_data *graphics_to_object_data)
{
	cmzn_elementiterator_id iterator = cmzn_mesh_create_elementiterator(mesh);
	if (!iterator)
		return 0;
	int return_code = 1;
	cmzn_element_id element = 0;
	GraphicsIncrementalBuild *incrementalBuild = graphics_to_object_data->incrementalBuild;
	cmzn_graphics *graphics = graphics_to_object_data->graphics;
	if ((incrementalBuild) && (graphics->incrementalBuildIndex != DS_LABEL_INDEX_INVALID))
		iterator->setIndex(graphics->incrementalBuildIndex);
	while (0 != (element = cmzn_elementiterator_next_non_access(iterator)))
	{
		if (!FE_element_to_graphics_object(element, graphics_to_object_data))
		{
			return_code = 0;
			break;
		}
		if ((incrementalBuild) && incrementalBuild->incrementDone())
		{
			graphics->incrementalBuildIndex = get_FE_element_index(element);
			if (0 != (element = cmzn_elementiterator_next_non_access(iterator)))
				incrementalBuild->setMoreWorkToDo();
			break;
		}
	}
	cmzn_elementiterator_destroy(&iterator);
	if ((incrementalBuild) && !incrementalBuild->isMoreWorkToDo())
		graphics->incrementalBuildIndex = DS_LABEL_INDEX_INVALID;
	return return_code;
}

int cmzn_graphics_to_graphics_object_no_check_on_filter(struct cmzn_graphics *graphics,
	cmzn_graphics_to_graphics_object_data *graphics_to_object_data)
{
	int return_code = 1;
	if (graphics && graphics_to_object_data)
	{
		GraphicsIncrementalBuild *incrementalBuild = graphics_to_object_data->incrementalBuild;
		bool buildNow = (0 != graphics->graphics_changed);
		if (buildNow)
			if (incrementalBuild)
				if (incrementalBuild->incrementDone())
				{
					buildNow = false; // build next time
					incrementalBuild->setMoreWorkToDo();
				}
		if (buildNow)
		{
			cmzn_fieldcache_clear_location(graphics_to_object_data->field_cache);
			cmzn_fieldcache_set_time(graphics_to_object_data->field_cache, graphics_to_object_data->time);
			Computed_field *coordinate_field = graphics->coordinate_field;
			if (coordinate_field ||
				(graphics->domain_type == CMZN_FIELD_DOMAIN_TYPE_POINT))
			{
				/* RC coordinate_field to pass to FE_element_to_graphics_object */
				graphics_to_object_data->rc_coordinate_field = (cmzn_field_id)0;
				graphics_to_object_data->wrapper_orientation_scale_field = (cmzn_field_id)0;
				graphics_to_object_data->wrapper_stream_vector_field = (cmzn_field_id)0;
				if (coordinate_field)
				{
					graphics_to_object_data->rc_coordinate_field = graphics->scene->getCoordinateFieldWrapper(coordinate_field);
					if (!graphics_to_object_data->rc_coordinate_field)
					{
						display_message(ERROR_MESSAGE,
							"cmzn_graphics_to_graphics_object.  Could not get rc_coordinate_field wrapper");
						return_code = 0;
					}
				}
				if (return_code && graphics->point_orientation_scale_field)
				{
					graphics_to_object_data->wrapper_orientation_scale_field =
						graphics->scene->getVectorFieldWrapper(graphics->point_orientation_scale_field, coordinate_field);
					if (!graphics_to_object_data->wrapper_orientation_scale_field)
					{
						display_message(ERROR_MESSAGE,
							"cmzn_graphics_to_graphics_object.  Could not get orientation_scale_field wrapper");
						return_code = 0;
					}
				}
				if (return_code && graphics->stream_vector_field)
				{
					graphics_to_object_data->wrapper_stream_vector_field =
						graphics->scene->getVectorFieldWrapper(graphics->stream_vector_field, coordinate_field);
					if (!graphics_to_object_data->wrapper_stream_vector_field)
					{
						display_message(ERROR_MESSAGE,
							"cmzn_graphics_to_graphics_object.  Could not get stream_vector_field wrapper");
						return_code = 0;
					}
				}
				if (return_code && graphics->glyph)
				{
					graphics_to_object_data->glyph_gt_object =
						graphics->glyph->getGraphicsObject(graphics->tessellation, graphics->material, graphics->font);
				}
				else
				{
					graphics_to_object_data->glyph_gt_object = 0;
				}
				if (return_code)
				{
					char *graphics_string;
#if defined (DEBUG_CODE)
					/*???debug*/
					if ((graphics_string = cmzn_graphics_string(graphics,
						GRAPHICS_STRING_COMPLETE_PLUS)) != NULL)
					{
						printf("> building %s\n", graphics_string);
						DEALLOCATE(graphics_string);
					}
#endif /* defined (DEBUG_CODE) */
					cmzn_graphics_get_top_level_number_in_xi(graphics,
						MAXIMUM_ELEMENT_XI_DIMENSIONS, graphics_to_object_data->top_level_number_in_xi);
					/* work out the name the graphics object is to have */
					char *graphics_object_name = cmzn_graphics_get_graphics_object_name(graphics, graphics_to_object_data->name_prefix);
					if (graphics_object_name)
					{
						if (graphics->graphics_object)
						{
							GT_object_set_name(graphics->graphics_object, graphics_object_name);
						}
						else
						{
							enum GT_object_type graphics_object_type = cmzn_graphics_get_graphics_object_type(graphics);
							if (graphics_object_type == g_OBJECT_TYPE_INVALID)
								return_code = 1;
							if (return_code)
							{
								graphics->graphics_object = CREATE(GT_object)(
									graphics_object_name, graphics_object_type,
									graphics->material);
								set_GT_object_render_line_width(graphics->graphics_object, graphics->render_line_width);
								set_GT_object_render_point_size(graphics->graphics_object, graphics->render_point_size);
								GT_object_set_select_mode(graphics->graphics_object,
									graphics->select_mode);
								if (graphics->secondary_material)
								{
									set_GT_object_secondary_material(graphics->graphics_object,
										graphics->secondary_material);
								}
								if (graphics->selected_material)
								{
									set_GT_object_selected_material(graphics->graphics_object,
										graphics->selected_material);
								}
							}
						}
						DEALLOCATE(graphics_object_name);
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"cmzn_graphics_to_graphics_object.  "
							"Unable to make graphics object name");
						return_code = 0;
					}
					if (graphics->data_field)
					{
						graphics_to_object_data->number_of_data_values =
							cmzn_field_get_number_of_components(graphics->data_field);
						ALLOCATE(graphics_to_object_data->data_copy_buffer,
							FE_value, graphics_to_object_data->number_of_data_values);
					}
					if (graphics->graphics_object)
					{
						graphics->selected_graphics_changed=1;
						/* need graphics for FE_element_to_graphics_object routine */
						graphics_to_object_data->graphics=graphics;
						cmzn_graphics_get_iteration_domain(graphics, graphics_to_object_data);
						switch (graphics->graphics_type)
						{
						case CMZN_GRAPHICS_TYPE_POINTS:
						{
							switch (graphics->domain_type)
							{
							case CMZN_FIELD_DOMAIN_TYPE_NODES:
							case CMZN_FIELD_DOMAIN_TYPE_DATAPOINTS:
							{
								// currently graphics for all nodes/datapoints are always rebuilt entirely
								GT_object_clear_primitives(graphics->graphics_object);
								cmzn_nodeset_id master_nodeset = cmzn_fieldmodule_find_nodeset_by_field_domain_type(
									graphics_to_object_data->field_module, graphics->domain_type);
								cmzn_nodeset_id iteration_nodeset = 0;
								if (graphics->subgroup_field)
								{
									cmzn_field_group_id group = cmzn_field_cast_group(graphics->subgroup_field);
									if (group)
									{
										cmzn_field_node_group_id node_group = cmzn_field_group_get_field_node_group(group, master_nodeset);
										if (node_group)
										{
											iteration_nodeset =
												cmzn_nodeset_group_base_cast(cmzn_field_node_group_get_nodeset_group(node_group));
											cmzn_field_node_group_destroy(&node_group);
										}
										cmzn_field_group_destroy(&group);
									}
									else
									{
										cmzn_field_node_group_id node_group = cmzn_field_cast_node_group(graphics->subgroup_field);
										if (node_group)
										{
											// check group is for same master nodeset
											iteration_nodeset = cmzn_nodeset_group_base_cast(cmzn_field_node_group_get_nodeset_group(node_group));
											cmzn_nodeset_id temp_master_nodeset = cmzn_nodeset_get_master_nodeset(iteration_nodeset);
											if (!cmzn_nodeset_match(master_nodeset, temp_master_nodeset))
											{
												cmzn_nodeset_destroy(&iteration_nodeset);
											}
											cmzn_nodeset_destroy(&temp_master_nodeset);
											cmzn_field_node_group_destroy(&node_group);
										}
										else
										{
											iteration_nodeset = cmzn_nodeset_access(master_nodeset);
										}
									}
								}
								else
								{
									iteration_nodeset = cmzn_nodeset_access(master_nodeset);
								}
								if (iteration_nodeset)
								{
									GT_glyphset_vertex_buffers *glyphset = Nodeset_create_vertex_array(
										iteration_nodeset, graphics_to_object_data->field_cache,
										graphics->graphics_object,
										graphics->glyph_repeat_mode,
										graphics_to_object_data->rc_coordinate_field,
										graphics->data_field,
										graphics_to_object_data->wrapper_orientation_scale_field,
										graphics->signed_scale_field,
										graphics->label_field,
										graphics->label_density_field,
										(iteration_nodeset == master_nodeset) ? graphics->subgroup_field : 0,
										graphics_to_object_data->selection_group_field,
										graphics_to_object_data->glyph_gt_object,
										graphics->point_base_size, graphics->point_offset, graphics->point_scale_factors,
										graphics->font,  graphics->label_offset,
										graphics->label_text,
										graphics->select_mode);
									if (!GT_OBJECT_ADD(GT_glyphset_vertex_buffers)(
											graphics->graphics_object, glyphset))
									{
										DESTROY(GT_glyphset_vertex_buffers)(&glyphset);
										return_code = 0;
									}
									cmzn_nodeset_destroy(&iteration_nodeset);
								}
								cmzn_nodeset_destroy(&master_nodeset);
							} break;
							case CMZN_FIELD_DOMAIN_TYPE_POINT:
							{
								cmzn_graphics_to_point_vertex_buffer(graphics, graphics_to_object_data);
							} break;
							default: // ELEMENTS
							{
								GT_glyphset_vertex_buffers *glyphset = GT_object_get_GT_glyphset_vertex_buffers(graphics->graphics_object);
								if (glyphset)
									GT_object_reset_buffer_binding(graphics->graphics_object);
								else
								{
									glyphset = CREATE(GT_glyphset_vertex_buffers)();
									if (!GT_OBJECT_ADD(GT_glyphset_vertex_buffers)(graphics->graphics_object, glyphset))
									{
										DESTROY(GT_glyphset_vertex_buffers)(&glyphset);
										return_code = 0;
									}
								}
								if (return_code)
								{
									Triple glyph_base_size, glyph_scale_factors, glyph_offset, glyph_label_offset;
									for (int i = 0; i < 3; i++)
									{
										glyph_base_size[i] = static_cast<GLfloat>(graphics->point_base_size[i]);
										glyph_scale_factors[i] = static_cast<GLfloat>(graphics->point_scale_factors[i]);
										glyph_offset[i] = static_cast<GLfloat>(graphics->point_offset[i]);
										glyph_label_offset[i] = static_cast<GLfloat>(graphics->label_offset[i]);
									}
									GT_glyphset_vertex_buffers_setup(glyphset, graphics_to_object_data->glyph_gt_object, graphics->glyph_repeat_mode,
										glyph_base_size, glyph_scale_factors, glyph_offset, graphics->font,
										glyph_label_offset, graphics->label_text, /*label_bounds_dimension*/0,
										/*label_bounds_components*/0);
									return_code = cmzn_mesh_to_graphics(graphics_to_object_data->iteration_mesh, graphics_to_object_data);
								}
							} break;
							}
						} break;
						case CMZN_GRAPHICS_TYPE_LINES:
						{
#if defined(USE_OPENCASCADE)
							// test here for domain of rc_coordinate_field
							// if it is a cad_geometry do something about it
							struct LIST(Computed_field) *domain_field_list = CREATE_LIST(Computed_field)();
							int return_code = Computed_field_get_domain( graphics_to_object_data->rc_coordinate_field, domain_field_list );
							if ( return_code )
							{
								// so test for topology domain
								struct Computed_field *cad_topology_field = FIRST_OBJECT_IN_LIST_THAT(Computed_field)
									( cmzn_field_is_type_cad_topology, (void *)NULL, domain_field_list );
								if ( cad_topology_field )
								{
									// if topology domain then draw item at location
									return_code = Cad_shape_to_graphics_object( cad_topology_field, graphics_to_object_data );
									DESTROY_LIST(Computed_field)(&domain_field_list);
									break;
								}
							}
							if ( domain_field_list )
								DESTROY_LIST(Computed_field)(&domain_field_list);
#endif /* defined(USE_OPENCASCADE) */
							if (GT_object_get_number_of_times(graphics->graphics_object) == 0)
							{
								if (CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_LINE == graphics->line_shape)
								{
									GT_polyline_vertex_buffers *lines =
										CREATE(GT_polyline_vertex_buffers)(g_PLAIN, graphics->render_line_width);
									if (!GT_OBJECT_ADD(GT_polyline_vertex_buffers)(graphics->graphics_object, lines))
									{
										DESTROY(GT_polyline_vertex_buffers)(&lines);
										return_code = 0;
									}
								}
								else if (graphics_to_object_data->iteration_mesh)
								{
									GT_surface_vertex_buffers *surfaces =
										CREATE(GT_surface_vertex_buffers)(g_SHADED_TEXMAP, graphics->render_polygon_mode);
									if (!GT_OBJECT_ADD(GT_surface_vertex_buffers)(graphics->graphics_object, surfaces))
									{
										DESTROY(GT_surface_vertex_buffers)(&surfaces);
										return_code = 0;
									}
								}
							}
							else
								GT_object_reset_buffer_binding(graphics->graphics_object);
							if (return_code && (graphics_to_object_data->iteration_mesh))
								return_code = cmzn_mesh_to_graphics(graphics_to_object_data->iteration_mesh, graphics_to_object_data);
						} break;
						case CMZN_GRAPHICS_TYPE_SURFACES:
						{
							bool cad_surfaces = false;
#if defined(USE_OPENCASCADE)
							{
								// test here for domain of rc_coordinate_field
								// if it is a cad_geometry do something about it
								//if ( is_cad_geometry( settings_to_object_data->rc_coordinate_field->get_domain() ) )
								struct LIST(Computed_field) *domain_field_list = CREATE_LIST(Computed_field)();
								int return_code = Computed_field_get_domain( graphics_to_object_data->rc_coordinate_field, domain_field_list );
								if ( return_code )
								{
									//printf( "got domain of rc_coordinate_field (%d)\n", NUMBER_IN_LIST(Computed_field)(domain_field_list) );
									// so test for topology domain
									struct Computed_field *cad_topology_field = FIRST_OBJECT_IN_LIST_THAT(Computed_field)
										( cmzn_field_is_type_cad_topology, (void *)NULL, domain_field_list );
									if ( cad_topology_field )
									{
										cad_surfaces = true;
										//printf( "hurrah, we have a cad topology domain.\n" );
										// if topology domain then draw item at location
										return_code = Cad_shape_to_graphics_object( cad_topology_field, graphics_to_object_data );
										DESTROY_LIST(Computed_field)(&domain_field_list);
										break;
									}
								}
								if ( domain_field_list )
									DESTROY_LIST(Computed_field)(&domain_field_list);
							}
#endif /* defined(USE_OPENCASCADE) */
							if (!cad_surfaces)
							{
								if (GT_object_get_number_of_times(graphics->graphics_object) == 0)
								{
									GT_surface_vertex_buffers *surfaces =
										CREATE(GT_surface_vertex_buffers)(g_SHADED_TEXMAP, graphics->render_polygon_mode);
									if (!GT_OBJECT_ADD(GT_surface_vertex_buffers)(graphics->graphics_object, surfaces))
									{
										DESTROY(GT_surface_vertex_buffers)(&surfaces);
										return_code = 0;
									}
								}
								else
									GT_object_reset_buffer_binding(graphics->graphics_object);
								if (return_code && (graphics_to_object_data->iteration_mesh))
									return_code = cmzn_mesh_to_graphics(graphics_to_object_data->iteration_mesh, graphics_to_object_data);
							}
						} break;
						case CMZN_GRAPHICS_TYPE_CONTOURS:
						{
							// Used to call GT_object_clear_primitives(graphics->graphics_object) here
							// which is very expensive when partial editing. Seems to work without it.
							if (0 < graphics->number_of_isovalues)
							{
								if (GT_object_get_number_of_times(graphics->graphics_object) == 0)
								{
									if (g_SURFACE_VERTEX_BUFFERS == GT_object_get_type(graphics->graphics_object))
									{
										GT_surface_vertex_buffers *surfaces =
											CREATE(GT_surface_vertex_buffers)(g_SH_DISCONTINUOUS_TEXMAP, graphics->render_polygon_mode);
										if (!GT_OBJECT_ADD(GT_surface_vertex_buffers)(graphics->graphics_object, surfaces))
										{
											DESTROY(GT_surface_vertex_buffers)(&surfaces);
											return_code = 0;
										}
									}
									else if (g_POLYLINE_VERTEX_BUFFERS == GT_object_get_type(graphics->graphics_object))
									{
										GT_polyline_vertex_buffers *lines =
											CREATE(GT_polyline_vertex_buffers)(g_PLAIN, graphics->render_line_width);
										if (0 == (GT_OBJECT_ADD(GT_polyline_vertex_buffers)(graphics->graphics_object, lines)))
										{
											DESTROY(GT_polyline_vertex_buffers)(&lines);
											return_code = 0;
										}
									}
								}
								else
									GT_object_reset_buffer_binding(graphics->graphics_object);
								if (return_code && (graphics_to_object_data->iteration_mesh))
								{
									if (g_SURFACE_VERTEX_BUFFERS == GT_object_get_type(graphics->graphics_object))
									{
										graphics_to_object_data->iso_surface_specification =
											Iso_surface_specification_create(
												graphics->number_of_isovalues, graphics->isovalues,
												graphics->first_isovalue, graphics->last_isovalue,
												graphics_to_object_data->rc_coordinate_field,
												graphics->data_field,
												graphics->isoscalar_field,
												graphics->texture_coordinate_field);
									}
									return_code = cmzn_mesh_to_graphics(graphics_to_object_data->iteration_mesh, graphics_to_object_data);
									if (g_SURFACE_VERTEX_BUFFERS == GT_object_get_type(graphics->graphics_object))
									{
										Iso_surface_specification_destroy(&graphics_to_object_data->iso_surface_specification);
									}
								}
							}
						} break;
						case CMZN_GRAPHICS_TYPE_STREAMLINES:
						{
							if (GT_object_get_number_of_times(graphics->graphics_object) == 0)
							{
								if (g_SURFACE_VERTEX_BUFFERS == GT_object_get_type(graphics->graphics_object))
								{
									GT_surface_vertex_buffers *surfaces =
										CREATE(GT_surface_vertex_buffers)(g_SHADED_TEXMAP, graphics->render_polygon_mode);
									GT_OBJECT_ADD(GT_surface_vertex_buffers)(graphics->graphics_object, surfaces);
								}
								else if (g_POLYLINE_VERTEX_BUFFERS == GT_object_get_type(graphics->graphics_object))
								{
									GT_polyline_vertex_buffers *lines =
										CREATE(GT_polyline_vertex_buffers)(g_PLAIN, graphics->render_line_width);
									GT_OBJECT_ADD(GT_polyline_vertex_buffers)(graphics->graphics_object, lines);
								}
							}
							else
								GT_object_reset_buffer_binding(graphics->graphics_object);
							if (graphics->seed_element)
							{
								return_code = FE_element_to_graphics_object(
									graphics->seed_element, graphics_to_object_data);
							}
							else if (graphics->seed_nodeset &&
								graphics->seed_node_mesh_location_field)
							{
								cmzn_nodeiterator_id iterator = cmzn_nodeset_create_nodeiterator(graphics->seed_nodeset);
								cmzn_node_id node = 0;
								while (0 != (node = cmzn_nodeiterator_next_non_access(iterator)))
								{
									if (!cmzn_node_to_streamline(node, graphics_to_object_data))
									{
										return_code = 0;
										break;
									}
								}
								cmzn_nodeiterator_destroy(&iterator);
							}
							else
							{
								if (graphics_to_object_data->iteration_mesh)
								{
									return_code = cmzn_mesh_to_graphics(graphics_to_object_data->iteration_mesh, graphics_to_object_data);
								}
							}
						} break;
						default:
						{
							return_code = 0;
						} break;
						} /* end of switch */
						cmzn_mesh_destroy(&graphics_to_object_data->iteration_mesh);
						cmzn_mesh_destroy(&graphics_to_object_data->master_mesh);
						if (return_code)
						{
							/* set the spectrum in the graphics object - if required */
							if ((graphics->data_field) ||
								((CMZN_GRAPHICS_TYPE_STREAMLINES == graphics->graphics_type) &&
									(CMZN_GRAPHICS_STREAMLINES_COLOUR_DATA_TYPE_FIELD != graphics->streamlines_colour_data_type)))
							{
								set_GT_object_Spectrum(graphics->graphics_object, graphics->spectrum);
							}
							if (!((incrementalBuild) && incrementalBuild->isMoreWorkToDo()))
								graphics->graphics_changed = 0;
							/* mark display list as needing updating */
							GT_object_changed(graphics->graphics_object);
						}
						else
						{
							graphics_string = cmzn_graphics_string(graphics,
								GRAPHICS_STRING_COMPLETE_PLUS);
							display_message(ERROR_MESSAGE,
								"cmzn_graphics_to_graphics_object.  "
								"Could not build '%s'",graphics_string);
							DEALLOCATE(graphics_string);
							/* set return_code to 1, so rest of graphics can be built */
							return_code = 1;
						}
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"cmzn_graphics_to_graphics_object.  "
							"Could not create graphics object");
						return_code = 0;
					}
					if (graphics->data_field)
					{
						graphics_to_object_data->number_of_data_values = 0;
						DEALLOCATE(graphics_to_object_data->data_copy_buffer);
					}
				}
				if (graphics_to_object_data->glyph_gt_object)
				{
					DEACCESS(GT_object)(&(graphics_to_object_data->glyph_gt_object));
				}
			}
		}
		if (graphics->selected_graphics_changed)
		{
			if (graphics->graphics_object)
				GT_object_changed(graphics->graphics_object);
			graphics->selected_graphics_changed = 0;
		}
	}
	else
		return_code = 0;
	return return_code;
}

int cmzn_graphics_to_graphics_object(
	struct cmzn_graphics *graphics,void *graphics_to_object_data_void)
{
	int return_code;

	struct cmzn_graphics_to_graphics_object_data *graphics_to_object_data =
		reinterpret_cast<struct cmzn_graphics_to_graphics_object_data *>(graphics_to_object_data_void);
	if (graphics && graphics_to_object_data)
	{
		return_code = 1;
		/* build only if visible... */
		cmzn_scenefilter_id filter = graphics_to_object_data->scenefilter;
		/* build only if visible and changed */
		if ((0 == filter) || (cmzn_scenefilter_evaluate_graphics(filter, graphics)))
		{
			return_code = cmzn_graphics_to_graphics_object_no_check_on_filter(graphics,
				graphics_to_object_data);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_to_graphics_object.  Invalid argument(s)");
		return_code = 0;
	}

	return (return_code);
}

int cmzn_graphics_compile_visible_graphics(
	struct cmzn_graphics *graphics, void *renderer_void)
{
	int return_code = 1;
	Render_graphics *renderer;

	if (graphics && (renderer = static_cast<Render_graphics *>(renderer_void)))
	{
		return_code = 1;
		if (graphics->graphics_object)
		{
			cmzn_scenefilter_id filter = renderer->getScenefilter();
			if ((0 == filter) || (cmzn_scenefilter_evaluate_graphics(filter, graphics)))
			{
				return_code = renderer->Graphics_compile(graphics);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_compile_visible_graphics.  "
			"Invalid argument(s)");
		return_code = 0;
	}

	return (return_code);
} /* cmzn_graphics_compile_visible_graphics */

int cmzn_graphics_execute_visible_graphics(
	struct cmzn_graphics *graphics, void *renderer_void)
{
	int return_code = 1;
	Render_graphics_opengl *renderer;

	ENTER(cmzn_graphics_execute_visible_graphics);
	if (graphics && (renderer = static_cast<Render_graphics_opengl *>
			(renderer_void)))
	{
		return_code = 1;
		if (graphics->graphics_object)
		{
			cmzn_scenefilter_id filter = renderer->getScenefilter();
			if ((0 == filter) || (cmzn_scenefilter_evaluate_graphics(filter, graphics)))
			{
				if (renderer->rendering_layer(graphics->overlay_flag))
				{
					if (renderer->begin_coordinate_system(graphics->coordinate_system))
					{
						if (renderer->picking)
						{
#if defined (OPENGL_API)
						/* use position in list as name for GL picking */
							glLoadName((GLuint)graphics->position);
#endif /* defined (OPENGL_API) */
						}
						return_code = renderer->Graphics_execute(graphics);
						renderer->end_coordinate_system(graphics->coordinate_system);
					}
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_execute_visible_graphics.  "
			"Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_graphics_execute_visible_graphics */

namespace {

/**
 * If any field used by graphics has fully changed (CHANGE_FLAG_DEFINITION or
 * CHANGE_FLAG_DEPENDENCY) set, return the change status immediately to trigger
 * a full rebuild of graphics.
 * Otherwise return whether a partial rebuild is needed or no change.
 */
cmzn_field_change_flags cmzn_graphics_get_most_significant_field_change(
	cmzn_graphics *graphics, cmzn_fieldmoduleevent *event)
{
	cmzn_field_change_flags change = CMZN_FIELD_CHANGE_FLAG_NONE;
	const cmzn_field_change_flags fullChange =
		(CMZN_FIELD_CHANGE_FLAG_DEFINITION | CMZN_FIELD_CHANGE_FLAG_FULL_RESULT);
	cmzn_field_change_flags fieldChange;
	if (graphics->coordinate_field)
	{
		fieldChange = cmzn_fieldmoduleevent_get_field_change_flags(event, graphics->coordinate_field);
		change |= fieldChange;
		if (change & fullChange)
			return change;
	}
	if (graphics->data_field)
	{
		fieldChange = cmzn_fieldmoduleevent_get_field_change_flags(event, graphics->data_field);
		change |= fieldChange;
		if (change & fullChange)
			return change;
	}
	if (graphics->subgroup_field)
	{
		fieldChange = cmzn_fieldmoduleevent_get_field_change_flags(event, graphics->subgroup_field);
		change |= fieldChange;
		if (change & fullChange)
			return change;
	}
	if (graphics->tessellation_field)
	{
		fieldChange = cmzn_fieldmoduleevent_get_field_change_flags(event, graphics->tessellation_field);
		change |= fieldChange;
		if (change & fullChange)
			return change;
	}
	if (graphics->texture_coordinate_field)
	{
		fieldChange = cmzn_fieldmoduleevent_get_field_change_flags(event, graphics->texture_coordinate_field);
		change |= fieldChange;
		if (change & fullChange)
			return change;
	}
	if (graphics->line_orientation_scale_field)
	{
		fieldChange = cmzn_fieldmoduleevent_get_field_change_flags(event, graphics->line_orientation_scale_field);
		change |= fieldChange;
		if (change & fullChange)
			return change;
	}
	if (graphics->isoscalar_field)
	{
		fieldChange = cmzn_fieldmoduleevent_get_field_change_flags(event, graphics->isoscalar_field);
		change |= fieldChange;
		if (change & fullChange)
			return change;
	}
	if (CMZN_GRAPHICS_TYPE_POINTS == graphics->graphics_type)
	{
		if (graphics->point_orientation_scale_field)
		{
			fieldChange = cmzn_fieldmoduleevent_get_field_change_flags(event, graphics->point_orientation_scale_field);
			change |= fieldChange;
			if (change & fullChange)
				return change;
		}
		if (graphics->signed_scale_field)
		{
			fieldChange = cmzn_fieldmoduleevent_get_field_change_flags(event, graphics->signed_scale_field);
			change |= fieldChange;
			if (change & fullChange)
				return change;
		}
		if (graphics->label_field)
		{
			fieldChange = cmzn_fieldmoduleevent_get_field_change_flags(event, graphics->label_field);
			change |= fieldChange;
			if (change & fullChange)
				return change;
		}
		if (graphics->label_density_field)
		{
			fieldChange = cmzn_fieldmoduleevent_get_field_change_flags(event, graphics->label_density_field);
			change |= fieldChange;
			if (change & fullChange)
				return change;
		}
	}
	if (graphics->sample_density_field)
	{
		fieldChange = cmzn_fieldmoduleevent_get_field_change_flags(event, graphics->sample_density_field);
		change |= fieldChange;
		if (change & fullChange)
			return change;
	}
	if (graphics->stream_vector_field)
	{
		fieldChange = cmzn_fieldmoduleevent_get_field_change_flags(event, graphics->stream_vector_field);
		change |= fieldChange;
		if (change & fullChange)
			return change;
	}
	return change;
}

} // namespace anonymous

int cmzn_graphics_field_change(struct cmzn_graphics *graphics,
	void *change_data_void)
{
	cmzn_graphics_field_change_data *change_data =
		static_cast<cmzn_graphics_field_change_data *>(change_data_void);
	if (change_data->selection_changed && (CMZN_GRAPHICS_TYPE_STREAMLINES != graphics->graphics_type))
		cmzn_graphics_update_selected(graphics, (void *)NULL);
	if (0 == graphics->graphics_object)
	{
		cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_REDRAW);
		return 1;
	}
	cmzn_field_change_flags fieldChange =
		cmzn_graphics_get_most_significant_field_change(graphics, change_data->event);
	const int domainDimension = cmzn_graphics_get_domain_dimension(graphics);
	FE_region_changes *feRegionChanges = change_data->event->getFeRegionChanges();
	if (feRegionChanges)
	{
		if (0 == domainDimension)
		{
			// node/data points: currently always rebuilt from scratch
			if (fieldChange & CMZN_FIELD_CHANGE_FLAG_RESULT)
			{
				cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
				return 1;
			}
			// rebuild all if identifiers changed, for correct picking and editing graphics object
			DsLabelsChangeLog *nodeChangeLog = feRegionChanges->getNodeChangeLog(graphics->domain_type);
			// Note we won't get a change log for CMZN_FIELD_DOMAIN_TYPE_POINT
			if ((nodeChangeLog) && (nodeChangeLog->getChangeSummary() & DS_LABEL_CHANGE_TYPE_IDENTIFIER))
			{
				cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
				return 1;
			}
		}
		else
		{
			if (fieldChange & CMZN_FIELD_CHANGE_FLAG_FULL_RESULT)
			{
				cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
				return 1;
			}
			DsLabelsChangeLog *elementChangeLog = feRegionChanges->getElementChangeLog(domainDimension);
			if (elementChangeLog->getChangeSummary() & DS_LABEL_CHANGE_TYPE_ADD)
			{
				// partial rebuild is buggy when new elements are added; workaround is to force full rebuild
				cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
				return 1;
			}
			bool partialUpdate = (0 != (fieldChange & CMZN_FIELD_CHANGE_FLAG_PARTIAL_RESULT));
			if (!partialUpdate)
			{
				// If element identifiers have changed, cmiss_number and derived fields are
				// not marked as changed, so need to manually trigger graphics using them to
				// be updated. However, since renumbering is not common, it's not worth the
				// complexity of checking such a field is being used, so always partial update.
				// Also for the future this allows us to send an identifiers all-change
				// message if reclaimed memory from DsLabels and maps (all indexes changed).
				if (elementChangeLog->getChangeSummary() & DS_LABEL_CHANGE_TYPE_IDENTIFIER)
					partialUpdate = true;
			}
			if (partialUpdate)
			{
				if (graphics->graphics_type == CMZN_GRAPHICS_TYPE_STREAMLINES ||
					graphics->graphics_type == CMZN_GRAPHICS_TYPE_POINTS)
				{
					cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
					return 1;
				}
				feRegionChanges->propagateToDimension(domainDimension);
				if (elementChangeLog->isAllChange() || (elementChangeLog->getChangeCount()*2 >
					FE_region_find_FE_mesh_by_dimension(graphics->scene->region->get_FE_region(), domainDimension)->getSize()))
				{
					// too many changes for partial rebuild
					cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
					return 1;
				}
				/* partial rebuild for few node/element field changes */
				GT_object_invalidate_selected_primitives(graphics->graphics_object,
					feRegionChanges->getElementChangeLog(domainDimension));
				cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_PARTIAL_REBUILD);
			}
		}
	}
	return 1;
}

int cmzn_graphics_get_visible_graphics_object_range(
	struct cmzn_graphics *graphics,void *graphics_range_void)
{
	int return_code = 1;
	struct cmzn_graphics_range *graphics_range =
		(struct cmzn_graphics_range *)graphics_range_void;

	ENTER(cmzn_graphics_get_visible_graphics_object_range);

	if (graphics && graphics_range && graphics_range->graphics_object_range)
	{
		if (graphics->graphics_object &&
			(graphics->coordinate_system == graphics_range->coordinate_system))
		{
			if ((0 == graphics_range->filter) ||
				(cmzn_scenefilter_evaluate_graphics(graphics_range->filter, graphics)))
			{
				return_code=get_graphics_object_range(graphics->graphics_object,
					(void *)graphics_range->graphics_object_range);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_get_visible_graphics_object_range.  "
			"Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_graphics_get_visible_graphics_object_range */

struct GT_object *cmzn_graphics_get_graphics_object(
	struct cmzn_graphics *graphics)
{
	struct GT_object *graphics_object

	ENTER(cmzn_graphics_get_graphics_object);
	if (graphics)
	{
		graphics_object=graphics->graphics_object;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_get_graphics_object.  Invalid argument(s)");
		graphics_object=(struct GT_object *)NULL;
	}
	LEAVE;

	return (graphics_object);
} /* cmzn_graphics_get_graphics_object */

enum cmzn_graphics_select_mode cmzn_graphics_get_select_mode(
	cmzn_graphics_id graphics)
{
	if (graphics)
		return graphics->select_mode;
	return CMZN_GRAPHICS_SELECT_MODE_INVALID;
}

int cmzn_graphics_set_select_mode(cmzn_graphics_id graphics,
	enum cmzn_graphics_select_mode select_mode)
{
	if (graphics && (0 != ENUMERATOR_STRING(cmzn_graphics_select_mode)(select_mode)))
	{
		if (select_mode != graphics->select_mode)
		{
			graphics->select_mode = select_mode;
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_spectrum_id cmzn_graphics_get_spectrum(cmzn_graphics_id graphics)
{
	cmzn_spectrum_id spectrum = 0;
	if (graphics)
	{
		if (graphics->spectrum)
		{
			spectrum = ACCESS(cmzn_spectrum)(graphics->spectrum);
		}
	}
	return spectrum;
}

int cmzn_graphics_set_spectrum(cmzn_graphics_id graphics,
	cmzn_spectrum_id spectrum)
{
	int return_code = 0;
	if (graphics)
	{
		if (spectrum != graphics->spectrum)
		{
			REACCESS(cmzn_spectrum)(&(graphics->spectrum), spectrum);
			cmzn_graphics_update_graphics_object_trivial(graphics);
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_RECOMPILE);
		}
		return_code = 1;
	}
	return return_code;
}

int cmzn_graphics_copy_without_graphics_object(
	struct cmzn_graphics *destination, struct cmzn_graphics *source)
{
	int return_code;

	ENTER(cmzn_graphics_copy_without_graphics_object);
	if (destination && source && (destination != source))
	{
		return_code = 1;
		destination->position = source->position;

		if (destination->name)
		{
			DEALLOCATE(destination->name);
			destination->name = 0;
		}
		if (source->name)
		{
			destination->name = duplicate_string(source->name);
		}

		/* copy geometry graphics */
		/* for all graphics types */
		destination->graphics_type=source->graphics_type;
		destination->domain_type = source->domain_type;
		destination->coordinate_system=source->coordinate_system;
		REACCESS(Computed_field)(&(destination->coordinate_field),
			source->coordinate_field);
		destination->select_mode=source->select_mode;
		/* for surfaces only at the moment */
		REACCESS(Computed_field)(&(destination->texture_coordinate_field),
			source->texture_coordinate_field);
		/* for 1-D and 2-D elements only */
		destination->exterior=source->exterior;
		destination->face=source->face;
		/* overlay_flag */
		destination->overlay_flag = source->overlay_flag;
		destination->overlay_order = source->overlay_order;

		/* line attributes */
		destination->line_shape = source->line_shape;
		if ((CMZN_GRAPHICS_TYPE_LINES == source->graphics_type) ||
			(CMZN_GRAPHICS_TYPE_STREAMLINES == source->graphics_type))
		{
			REACCESS(Computed_field)(&destination->line_orientation_scale_field,
				source->line_orientation_scale_field);
			for (int i = 0; i < 2; i++)
			{
				destination->line_base_size[i] = source->line_base_size[i];
				destination->line_scale_factors[i] = source->line_scale_factors[i];
			}
		}
		else if (destination->line_orientation_scale_field)
		{
			DEACCESS(Computed_field)(&destination->line_orientation_scale_field);
		}

		cmzn_graphics_contours_id contours = cmzn_graphics_cast_contours(destination);
		if (contours)
		{
			cmzn_graphics_contours_set_isoscalar_field(contours, source->isoscalar_field);
			if (source->isovalues)
			{
				cmzn_graphics_contours_set_list_isovalues(contours, source->number_of_isovalues,
					source->isovalues);
			}
			else
			{
				cmzn_graphics_contours_set_range_isovalues(contours, source->number_of_isovalues,
					source->first_isovalue, source->last_isovalue);
			}
			cmzn_graphics_contours_set_decimation_threshold(contours, source->decimation_threshold);
			cmzn_graphics_contours_destroy(&contours);
		}
		else
		{
			if (destination->isoscalar_field)
			{
				DEACCESS(Computed_field)(&destination->isoscalar_field);
			}
			if (destination->isovalues)
			{
				DEALLOCATE(destination->isovalues);
				destination->isovalues = 0;
			}
			destination->number_of_isovalues = 0;
		}

		cmzn_graphicspointattributes_id point_attributes =
			cmzn_graphics_get_graphicspointattributes(destination);
		if (point_attributes)
		{
			cmzn_graphicspointattributes_set_glyph(point_attributes, reinterpret_cast<cmzn_glyph*>(source->glyph));
			destination->glyph_repeat_mode = source->glyph_repeat_mode;
			for (int i = 0; i < 3; i++)
			{
				destination->point_base_size[i] = source->point_base_size[i];
				destination->point_offset[i] = source->point_offset[i];
				destination->point_scale_factors[i] = source->point_scale_factors[i];
				destination->label_offset[i] = source->label_offset[i];
				if (destination->label_text[i])
				{
					DEALLOCATE(destination->label_text[i]);
					destination->label_text[i] = 0;
				}
				if (source->label_text[i])
				{
					destination->label_text[i] = duplicate_string(source->label_text[i]);
				}
			}
		}
		else
		{
			if (destination->glyph)
			{
				cmzn_glyph_destroy(&(destination->glyph));
			}
		}
		REACCESS(Computed_field)(&(destination->point_orientation_scale_field),
			source->point_orientation_scale_field);
		REACCESS(Computed_field)(&(destination->signed_scale_field), source->signed_scale_field);
		REACCESS(Computed_field)(&(destination->label_field),source->label_field);
		REACCESS(Computed_field)(&(destination->subgroup_field),source->subgroup_field);
		cmzn_graphicspointattributes_destroy(&point_attributes);

		destination->overlay_flag = source->overlay_flag;
		destination->overlay_order = source->overlay_order;

		// for element sampling: element points, streamlines
		destination->sampling_mode=source->sampling_mode;
		REACCESS(Computed_field)(&(destination->sample_density_field),
			source->sample_density_field);
		for (int i = 0; i < 3; i++)
		{
			destination->sample_location[i] = source->sample_location[i];
		}

		// for tessellating and sampling elements
		REACCESS(cmzn_tessellation)(&(destination->tessellation),
			source->tessellation);
		REACCESS(Computed_field)(&(destination->tessellation_field),
			source->tessellation_field);
		REACCESS(Computed_field)(&(destination->label_density_field),source->label_density_field);
		/* for graphics starting in a particular element */
		REACCESS(FE_element)(&(destination->seed_element),
			source->seed_element);
		/* for streamlines only */
		REACCESS(Computed_field)(&(destination->stream_vector_field),
			source->stream_vector_field);
		destination->streamlines_track_direction = source->streamlines_track_direction;
		destination->streamline_length=source->streamline_length;
		if (destination->seed_nodeset)
		{
			cmzn_nodeset_destroy(&destination->seed_nodeset);
		}
		if (source->seed_nodeset)
		{
			destination->seed_nodeset = cmzn_nodeset_access(source->seed_nodeset);
		}
		REACCESS(Computed_field)(&(destination->seed_node_mesh_location_field),
			source->seed_node_mesh_location_field);

		/* copy appearance graphics */
		/* for all graphics types */
		destination->visibility_flag = source->visibility_flag;
		destination->render_line_width = source->render_line_width;
		destination->render_point_size = source->render_point_size;
		REACCESS(cmzn_material)(&(destination->material),source->material);
		REACCESS(cmzn_material)(&(destination->secondary_material),
			source->secondary_material);
		cmzn_graphics_set_render_polygon_mode(destination,source->render_polygon_mode);
		REACCESS(Computed_field)(&(destination->data_field), source->data_field);
		REACCESS(cmzn_spectrum)(&(destination->spectrum), source->spectrum);
		destination->streamlines_colour_data_type = source->streamlines_colour_data_type;
		REACCESS(cmzn_material)(&(destination->selected_material),
			source->selected_material);
		destination->autorange_spectrum_flag = source->autorange_spectrum_flag;
		REACCESS(cmzn_font)(&(destination->font), source->font);

		/* ensure destination graphics object is cleared */
		REACCESS(GT_object)(&(destination->graphics_object),
			(struct GT_object *)NULL);
		destination->graphics_changed = 1;
		destination->selected_graphics_changed = 1;

		if (!return_code)
		{
			display_message(ERROR_MESSAGE,"cmzn_graphics_copy_without_graphics_object.  "
				"Error copying graphics");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"cmzn_graphics_copy_without_graphics_object.  "
			"Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_graphics_copy_without_graphics_object */

int cmzn_graphics_has_name(struct cmzn_graphics *graphics,
	void *name_void)
{
	char *name, temp_name[30];
	int return_code;

	ENTER(cmzn_graphics_has_name);
	if (graphics && (name=(char *)name_void))
	{
		return_code = 0;
		if (graphics->name)
		{
			return_code=!strcmp(name,graphics->name);
		}
		if (!return_code)
		{
			/* Compare with number if the graphics
			 has no name or the name didn't match */
			sprintf(temp_name, "%d", graphics->position);
			return_code=!strcmp(name,temp_name);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_has_name.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_graphics_has_name */

/**
 * cmzn_graphics list conditional function returning 1 iff the two
 * graphics have the same geometry and the same nontrivial appearance
 * characteristics. Trivial appearance characteristics can be updated in the
 * graphics object without recalculating it, and include material, spectrum,
 * glyph scalings etc.
 */
int cmzn_graphics_same_non_trivial(cmzn_graphics *graphics,
	cmzn_graphics *second_graphics)
{
	int i, return_code;

	ENTER(cmzn_graphics_same_non_trivial);
	if (graphics && second_graphics)
	{
		return_code=1;

		/* compare geometry graphics */
		/* for all graphics types */
		if (return_code)
		{
			/* note: different if names are different */
			return_code =
				(graphics->graphics_type == second_graphics->graphics_type) &&
				(graphics->domain_type == second_graphics->domain_type) &&
				(graphics->coordinate_field == second_graphics->coordinate_field) &&
				(graphics->subgroup_field == second_graphics->subgroup_field) &&
				((graphics->name == second_graphics->name) ||
					((graphics->name) && (second_graphics->name) &&
					(0 == strcmp(graphics->name, second_graphics->name)))) &&
				(graphics->select_mode == second_graphics->select_mode);
		}

		const int domain_dimension = cmzn_graphics_get_domain_dimension(graphics);

		/* for 1-D and 2-D elements only */
		if (return_code)
		{
			if ((1 == domain_dimension) || (2 == domain_dimension))
			{
				return_code =
					(graphics->exterior == second_graphics->exterior) &&
					(graphics->face == second_graphics->face);
			}
		}

		/* line attributes */
		if (return_code && (
			(CMZN_GRAPHICS_TYPE_LINES==graphics->graphics_type) ||
			(CMZN_GRAPHICS_TYPE_STREAMLINES==graphics->graphics_type)))
		{
			if ((graphics->line_shape != second_graphics->line_shape) ||
				(graphics->line_orientation_scale_field !=
					second_graphics->line_orientation_scale_field))
			{
				return_code = 0;
			}
			else
			{
				for (int i = 0; i < 2; i++)
				{
					if ((graphics->line_base_size[i] != second_graphics->line_base_size[i]) ||
						(graphics->line_scale_factors[i] != second_graphics->line_scale_factors[i]))
					{
						return_code = 0;
					}
				}
			}
		}

		/* for iso_surfaces only */
		if (return_code&&
			(CMZN_GRAPHICS_TYPE_CONTOURS==graphics->graphics_type))
		{
			return_code=(graphics->number_of_isovalues==
				second_graphics->number_of_isovalues)&&
				(graphics->decimation_threshold==second_graphics->decimation_threshold)&&
				(graphics->isoscalar_field==second_graphics->isoscalar_field);
			if (return_code)
			{
				if (graphics->isovalues)
				{
					if (second_graphics->isovalues)
					{
						i = 0;
						while (return_code && (i < graphics->number_of_isovalues))
						{
							if (graphics->isovalues[i] != second_graphics->isovalues[i])
							{
								return_code = 0;
							}
							i++;
						}
					}
					else
					{
						return_code = 0;
					}
				}
				else
				{
					if (second_graphics->isovalues)
					{
						return_code = 0;
					}
					else
					{
						return_code =
							(graphics->first_isovalue == second_graphics->first_isovalue)
							&& (graphics->last_isovalue == second_graphics->last_isovalue);
					}
				}
			}
		}

		if (return_code && (CMZN_GRAPHICS_TYPE_POINTS == graphics->graphics_type))
		{
			return_code=
				(graphics->point_orientation_scale_field==
					second_graphics->point_orientation_scale_field)&&
				(graphics->signed_scale_field==
					second_graphics->signed_scale_field)&&
				(graphics->label_field==second_graphics->label_field)&&
				(graphics->label_density_field==second_graphics->label_density_field);
		}

		if (return_code)
		{
			return_code =
				(graphics->tessellation == second_graphics->tessellation) &&
				(graphics->tessellation_field == second_graphics->tessellation_field);
		}

		// for element sampling: element points, streamlines
		if (return_code && (0 < domain_dimension) && (
			(CMZN_GRAPHICS_TYPE_POINTS == graphics->graphics_type) ||
			(CMZN_GRAPHICS_TYPE_STREAMLINES == graphics->graphics_type)))
		{
			return_code = (graphics->sampling_mode == second_graphics->sampling_mode) &&
				((graphics->sampling_mode != CMZN_ELEMENT_POINT_SAMPLING_MODE_CELL_POISSON) ||
					(graphics->sample_density_field == second_graphics->sample_density_field)) &&
				((graphics->sampling_mode != CMZN_ELEMENT_POINT_SAMPLING_MODE_SET_LOCATION) ||
					((graphics->sample_location[0] == second_graphics->sample_location[0]) &&
					 (graphics->sample_location[1] == second_graphics->sample_location[1]) &&
					 (graphics->sample_location[2] == second_graphics->sample_location[2])));
		}
		/* for graphics starting in a particular element */
		if (return_code&&(CMZN_GRAPHICS_TYPE_STREAMLINES==graphics->graphics_type))
		{
			return_code=
				(graphics->seed_element==second_graphics->seed_element);
		}
		/* for streamlines only */
		if (return_code&&(CMZN_GRAPHICS_TYPE_STREAMLINES==graphics->graphics_type))
		{
			return_code=
				(graphics->stream_vector_field==second_graphics->stream_vector_field)&&
				(graphics->streamlines_track_direction == second_graphics->streamlines_track_direction) &&
				(graphics->streamline_length==second_graphics->streamline_length)&&
				(((graphics->seed_nodeset==0) && (second_graphics->seed_nodeset==0)) ||
					((graphics->seed_nodeset) && (second_graphics->seed_nodeset) &&
						cmzn_nodeset_match(graphics->seed_nodeset, second_graphics->seed_nodeset)))&&
				(graphics->seed_node_mesh_location_field==second_graphics->seed_node_mesh_location_field);
		}

		if (return_code)
		{
			return_code =
				(graphics->data_field==second_graphics->data_field)&&
				(graphics->texture_coordinate_field==second_graphics->texture_coordinate_field)&&
				((CMZN_GRAPHICS_TYPE_STREAMLINES != graphics->graphics_type) ||
				 (graphics->streamlines_colour_data_type==second_graphics->streamlines_colour_data_type));
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_same_non_trivial.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
}

/**
 * Same as cmzn_graphics_same_non_trivial except <graphics> must also have
 * a graphics_object. Used for getting graphics objects from previous graphics
 * that are the same except for trivial differences such as the material and
 * spectrum which can be changed in the graphics object to match the new graphics .
 */
int cmzn_graphics_same_non_trivial_with_graphics_object(
	struct cmzn_graphics *graphics,void *second_graphics_void)
{
	int return_code;

	ENTER(cmzn_graphics_same_non_trivial_with_graphics_object);
	if (graphics)
	{
		return_code=graphics->graphics_object&&
			cmzn_graphics_same_non_trivial(graphics,reinterpret_cast<cmzn_graphics*>(second_graphics_void));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_same_non_trivial_with_graphics_object.  "
			"Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
}

int cmzn_graphics_match(struct cmzn_graphics *graphics1,
	struct cmzn_graphics *graphics2)
{
	int return_code;

	ENTER(cmzn_graphics_match);
	if (graphics1 && graphics2)
	{
		return
			cmzn_graphics_same_non_trivial(graphics1, graphics2) &&
			(graphics1->visibility_flag == graphics2->visibility_flag) &&
			(graphics1->material == graphics2->material) &&
			(graphics1->secondary_material == graphics2->secondary_material) &&
			(graphics1->render_line_width == graphics2->render_line_width) &&
			(graphics1->render_point_size == graphics2->render_point_size) &&
			(graphics1->selected_material == graphics2->selected_material) &&
			(graphics1->spectrum == graphics2->spectrum) &&
			(graphics1->font == graphics2->font) &&
			(graphics1->render_polygon_mode == graphics2->render_polygon_mode) &&
			((CMZN_GRAPHICS_TYPE_POINTS != graphics1->graphics_type) || (
				(graphics1->glyph == graphics2->glyph) &&
				(graphics1->glyph_repeat_mode == graphics2->glyph_repeat_mode) &&
				(graphics1->point_base_size[0] == graphics2->point_base_size[0]) &&
				(graphics1->point_base_size[1] == graphics2->point_base_size[1]) &&
				(graphics1->point_base_size[2] == graphics2->point_base_size[2]) &&
				(graphics1->point_scale_factors[0] == graphics2->point_scale_factors[0]) &&
				(graphics1->point_scale_factors[1] == graphics2->point_scale_factors[1]) &&
				(graphics1->point_scale_factors[2] == graphics2->point_scale_factors[2]) &&
				(graphics1->point_offset[0] == graphics2->point_offset[0]) &&
				(graphics1->point_offset[1] == graphics2->point_offset[1]) &&
				(graphics1->point_offset[2] == graphics2->point_offset[2]) &&
				(graphics1->label_offset[0] == graphics2->label_offset[0]) &&
				(graphics1->label_offset[1] == graphics2->label_offset[1]) &&
				(graphics1->label_offset[2] == graphics2->label_offset[2]) &&
				labels_match(graphics1->label_text[0], graphics2->label_text[0]) &&
				labels_match(graphics1->label_text[1], graphics2->label_text[1]) &&
				labels_match(graphics1->label_text[2], graphics2->label_text[2])));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_match.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_graphics_match */

int cmzn_graphics_same_name(struct cmzn_graphics *graphics,
	void *name_void)
{
	int return_code = 0;
	char *name;

	if (graphics && graphics->name && (NULL != (name =(char *)name_void)))
	{
		return_code = (0==strcmp(graphics->name, name));
	}

	return (return_code);
}

int cmzn_graphics_list_contents(struct cmzn_graphics *graphics,
	void *list_data_void)
{
	int return_code;
	char *graphics_string,line[40];
	struct cmzn_graphics_list_data *list_data;

	ENTER(cmzn_graphics_list_contents);
	if (graphics&&
		NULL != (list_data=(struct cmzn_graphics_list_data *)list_data_void))
	{
		if (NULL != (graphics_string=cmzn_graphics_string(graphics,
					list_data->graphics_string_detail)))
		{
			if (list_data->line_prefix)
			{
				display_message(INFORMATION_MESSAGE,list_data->line_prefix);
			}
			display_message(INFORMATION_MESSAGE,graphics_string);
			if (list_data->line_suffix)
			{
				display_message(INFORMATION_MESSAGE,list_data->line_suffix);
			}
			/*???RC temp */
			if ((GRAPHICS_STRING_COMPLETE_PLUS==list_data->graphics_string_detail)&&
				(graphics->access_count != 1))
			{
				sprintf(line," (access count = %i)",graphics->access_count);
				display_message(INFORMATION_MESSAGE,line);
			}
			display_message(INFORMATION_MESSAGE,"\n");
			DEALLOCATE(graphics_string);
			return_code=1;
		}
		else
		{
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_list_contents.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

			return (return_code);
} /* cmzn_graphics_list_contents */

int cmzn_graphics_get_position_in_list(
	struct cmzn_graphics *graphics,
	struct LIST(cmzn_graphics) *list_of_graphics)
{
	int position;

	ENTER(cmzn_graphics_get_position_in_list);
	if (graphics&&list_of_graphics)
	{
		if (IS_OBJECT_IN_LIST(cmzn_graphics)(graphics,list_of_graphics))
		{
			position=graphics->position;
		}
		else
		{
			position=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_get_position_in_list.  Invalid argument(s)");
		position=0;
	}
	LEAVE;

	return (position);
} /* cmzn_graphics_get_position_in_list */

int cmzn_graphics_copy_and_put_in_list(
	struct cmzn_graphics *graphics,void *list_of_graphics_void)
{
	int return_code;
	struct cmzn_graphics *copy_graphics;
	struct LIST(cmzn_graphics) *list_of_graphics;

	ENTER(cmzn_graphics_copy_and_put_in_list);
	if (graphics&&NULL != (list_of_graphics=
		(struct LIST(cmzn_graphics) *)list_of_graphics_void))
	{
		/* create new graphics to take the copy */
		if (NULL != (copy_graphics=CREATE(cmzn_graphics)(graphics->graphics_type)))
		{
			/* copy and insert in list */
			if (!(return_code=cmzn_graphics_copy_without_graphics_object(
				copy_graphics,graphics)&&
				ADD_OBJECT_TO_LIST(cmzn_graphics)(copy_graphics,
					list_of_graphics)))
			{
				display_message(ERROR_MESSAGE,
					"cmzn_graphics_copy_and_put_in_list.  "
					"Could not put copy in list");
			}
			DEACCESS(cmzn_graphics)(&copy_graphics);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"cmzn_graphics_copy_and_put_in_list.  Could not create copy");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_copy_and_put_in_list.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_graphics_copy_and_put_in_list */

int cmzn_graphics_type_matches(struct cmzn_graphics *graphics,
	void *graphics_type_void)
{
	int return_code;

	ENTER(cmzn_graphics_type_matches);
	if (graphics)
	{
		return_code=((void *)graphics->graphics_type == graphics_type_void);
	}
	else
	{
		display_message(ERROR_MESSAGE,"cmzn_graphics_type_matches.  "
			"Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_graphics_type_matches */

/***************************************************************************//**
 * If <graphics> does not already have a graphics object, this function attempts
 * to find graphics in <list_of_graphics> which differ only trivially in material,
 * spectrum etc. AND have a graphics object. If such a graphics is found, the
 * graphics_object is moved from the matching graphics and put in <graphics>, while
 * any trivial differences are fixed up in the graphics_obejct.
 */
int cmzn_graphics_extract_graphics_object_from_list(
	struct cmzn_graphics *graphics,void *list_of_graphics_void)
{
	int return_code;
	struct cmzn_graphics *matching_graphics;
	struct LIST(cmzn_graphics) *list_of_graphics;

	ENTER(cmzn_graphics_extract_graphics_object_from_list);
	if (graphics&&(list_of_graphics=
		(struct LIST(cmzn_graphics) *)list_of_graphics_void))
	{
		return_code = 1;
		if (!(graphics->graphics_object))
		{
			if (NULL != (matching_graphics = FIRST_OBJECT_IN_LIST_THAT(cmzn_graphics)(
				cmzn_graphics_same_non_trivial_with_graphics_object,
				(void *)graphics,list_of_graphics)))
			{
				/* make sure graphics_changed and selected_graphics_changed flags
					 are brought across */
				graphics->graphics_object = matching_graphics->graphics_object;
				/* make sure graphics and graphics object have same material and
					 spectrum */
				cmzn_graphics_update_graphics_object_trivial(graphics);
				graphics->graphics_changed = matching_graphics->graphics_changed;
				graphics->selected_graphics_changed =
					matching_graphics->selected_graphics_changed;
				/* reset graphics_object and flags in matching_graphics */
				matching_graphics->graphics_object = (struct GT_object *)NULL;
				//cmzn_graphics_changed(matching_graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_extract_graphics_object_from_list.  "
			"Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_graphics_extract_graphics_object_from_list */

enum cmzn_graphics_render_polygon_mode cmzn_graphics_get_render_polygon_mode(
	struct cmzn_graphics *graphics)
{
	if (graphics)
		return graphics->render_polygon_mode;
	return CMZN_GRAPHICS_RENDER_POLYGON_MODE_INVALID;
}

int cmzn_graphics_set_render_polygon_mode(cmzn_graphics_id graphics,
	enum cmzn_graphics_render_polygon_mode render_polygon_mode)
{
	if (graphics && (0 != ENUMERATOR_STRING(cmzn_graphics_render_polygon_mode)(render_polygon_mode)))
	{
		if (graphics->render_polygon_mode != render_polygon_mode)
		{
			graphics->render_polygon_mode = render_polygon_mode;
			cmzn_graphics_update_graphics_object_trivial(graphics);
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_RECOMPILE);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_field_id cmzn_graphics_get_subgroup_field(cmzn_graphics_id graphics)
{
	if (graphics && graphics->subgroup_field)
	{
		return ACCESS(Computed_field)(graphics->subgroup_field);
	}
	return 0;
}

int cmzn_graphics_set_subgroup_field(
	struct cmzn_graphics *graphics, struct Computed_field *subgroup_field)
{
	if (graphics && ((0 == subgroup_field) ||
		Computed_field_is_scalar(subgroup_field, (void*)0)))
	{
		if (subgroup_field != graphics->subgroup_field)
		{
			REACCESS(Computed_field)(&(graphics->subgroup_field), subgroup_field);
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

enum cmzn_graphics_type cmzn_graphics_get_type(cmzn_graphics_id graphics)
{
	if (graphics)
		return graphics->graphics_type;
	return CMZN_GRAPHICS_TYPE_INVALID;
}

cmzn_tessellation_id cmzn_graphics_get_tessellation(
	cmzn_graphics_id graphics)
{
	if (graphics && graphics->tessellation)
	{
		return ACCESS(cmzn_tessellation)(graphics->tessellation);
	}
	return 0;
}

int cmzn_graphics_set_tessellation(
	cmzn_graphics_id graphics, struct cmzn_tessellation *tessellation)
{
	if (graphics && tessellation)
	{
		if (tessellation != graphics->tessellation)
		{
			REACCESS(cmzn_tessellation)(&(graphics->tessellation), tessellation);
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_field_id cmzn_graphics_get_tessellation_field(
	cmzn_graphics_id graphics)
{
	if (graphics && graphics->tessellation_field)
		return ACCESS(Computed_field)(graphics->tessellation_field);
	return 0;
}

int cmzn_graphics_set_tessellation_field(cmzn_graphics_id graphics,
	cmzn_field_id tessellation_field)
{
	if (graphics)
	{
		if (tessellation_field != graphics->tessellation_field)
		{
			REACCESS(Computed_field)(&(graphics->tessellation_field), tessellation_field);
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_graphics_get_top_level_number_in_xi(struct cmzn_graphics *graphics,
	int max_dimensions, int *top_level_number_in_xi)
{
	int return_code = 1;
	if (graphics && (0 < max_dimensions) && top_level_number_in_xi)
	{
		int dim;
		for (dim = 0; dim < max_dimensions; dim++)
		{
			top_level_number_in_xi[dim] = 1;
		}
		if (graphics->tessellation)
		{
			cmzn_tessellation_get_minimum_divisions(graphics->tessellation,
				max_dimensions, top_level_number_in_xi);
			cmzn_field *tessellation_field = graphics->tessellation_field ?
				graphics->tessellation_field : graphics->coordinate_field;
			if (tessellation_field)
			{
				// refine if tessellation field is non-linear
				// first check if its coordinate system is non-linear (cheaper)
				if (((tessellation_field == graphics->coordinate_field) &&
					Coordinate_system_type_is_non_linear(get_coordinate_system_type(
						Computed_field_get_coordinate_system(tessellation_field)))) ||
					Computed_field_is_non_linear(tessellation_field))
				{
					int *refinement_factors = new int[max_dimensions];
					if (cmzn_tessellation_get_refinement_factors(graphics->tessellation,
						max_dimensions, refinement_factors))
					{
						for (dim = 0; dim < max_dimensions; dim++)
						{
							top_level_number_in_xi[dim] *= refinement_factors[dim];
						}
					}
					delete [] refinement_factors;
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_get_top_level_number_in_xi.  Invalid argument(s)");
		return_code = 0;
	}
	return return_code;
}

struct FE_element *cmzn_graphics_get_seed_element(
	struct cmzn_graphics *graphics)
{
	struct FE_element *seed_element;

	ENTER(cmzn_graphics_get_seed_element);
	if (graphics&&(CMZN_GRAPHICS_TYPE_STREAMLINES==graphics->graphics_type))
	{
		seed_element=graphics->seed_element;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_get_seed_element.  Invalid argument(s)");
		seed_element=(struct FE_element *)NULL;
	}
	LEAVE;

	return (seed_element);
} /* cmzn_graphics_get_seed_element */

int cmzn_graphics_set_seed_element(struct cmzn_graphics *graphics,
	struct FE_element *seed_element)
/*******************************************************************************
LAST MODIFIED : 19 March 1999

DESCRIPTION :
For graphics starting in a particular element.
==============================================================================*/
{
	int return_code;

	ENTER(cmzn_graphics_set_seed_element);
	if (graphics&&(CMZN_GRAPHICS_TYPE_STREAMLINES==graphics->graphics_type))
	{
		REACCESS(FE_element)(&graphics->seed_element,seed_element);
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_set_seed_element.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_graphics_set_seed_element */

double cmzn_graphics_get_render_line_width(cmzn_graphics_id graphics)
{
	if (graphics)
		return graphics->render_line_width;
	return 0.0;
}

int cmzn_graphics_set_render_line_width(cmzn_graphics_id graphics, double width)
{
	if (graphics && (width > 0.0))
	{
		if (graphics->render_line_width != width)
		{
			graphics->render_line_width = width;
			cmzn_graphics_update_graphics_object_trivial(graphics);
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_RECOMPILE);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

double cmzn_graphics_get_render_point_size(cmzn_graphics_id graphics)
{
	if (graphics)
		return graphics->render_point_size;
	return 0.0;
}

int cmzn_graphics_set_render_point_size(cmzn_graphics_id graphics, double size)
{
	if (graphics && (size > 0.0))
	{
		if (graphics->render_point_size != size)
		{
			graphics->render_point_size = size;
			cmzn_graphics_update_graphics_object_trivial(graphics);
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_RECOMPILE);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_field_id cmzn_graphics_get_texture_coordinate_field(
	cmzn_graphics_id graphics)
{
	if (graphics && graphics->texture_coordinate_field)
	{
		return ACCESS(Computed_field)(graphics->texture_coordinate_field);
	}
	return 0;
}

int cmzn_graphics_set_texture_coordinate_field(
	cmzn_graphics_id graphics, cmzn_field_id texture_coordinate_field)
{
	if (graphics && ((0 == texture_coordinate_field) ||
		(3 >= cmzn_field_get_number_of_components(texture_coordinate_field))))
	{
		if (texture_coordinate_field != graphics->texture_coordinate_field)
		{
			REACCESS(Computed_field)(&graphics->texture_coordinate_field, texture_coordinate_field);
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_graphics_time_change(
	struct cmzn_graphics *graphics,void *dummy_void)
{
	int return_code;

	ENTER(cmzn_graphics_time_change);
	USE_PARAMETER(dummy_void);
	if (graphics)
	{
		return_code = 1;
		if (graphics->glyph)
		{
			graphics->glyph->timeChange();
		}
		if (graphics->timeDependent)
		{
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_time_change.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_graphics_time_change */

void cmzn_graphics::updateTimeDependence()
{
	if ((this->glyph) && this->glyph->isTimeVarying())
		this->timeDependent = true;
	else if ((this->coordinate_field) && Computed_field_has_multiple_times(this->coordinate_field))
		this->timeDependent = true;
	else if ((this->texture_coordinate_field) && Computed_field_has_multiple_times(this->texture_coordinate_field))
		this->timeDependent = true;
	else if ((this->line_orientation_scale_field) && Computed_field_has_multiple_times(this->line_orientation_scale_field))
		this->timeDependent = true;
	else if (this->isoscalarFieldIsTimeDependent())
		this->timeDependent = true;
	else if (this->pointGlyphScalingIsTimeDependent())
		this->timeDependent = true;
	else if ((this->label_field) && Computed_field_has_multiple_times(this->label_field))
		this->timeDependent = true;
	else if ((this->label_density_field) && Computed_field_has_multiple_times(this->label_density_field))
		this->timeDependent = true;
	else if (this->subgroupFieldIsTimeDependent())
		this->timeDependent = true;
	else if ((this->stream_vector_field) && Computed_field_has_multiple_times(this->stream_vector_field))
		this->timeDependent = true;
	else if (this->dataFieldIsTimeDependent())
		this->timeDependent = true;
	else
		this->timeDependent = false;
}

int cmzn_graphics_update_time_dependence(struct cmzn_graphics *graphics, void *time_dependent_bool_void)
{
	bool *timeDependentAddress = static_cast<bool *>(time_dependent_bool_void);
	if (!((graphics) && (timeDependentAddress)))
	{
		display_message(ERROR_MESSAGE, "cmzn_graphics_is_time_varying.  Invalid argument(s)");
		return 0;
	}
	graphics->updateTimeDependence();
	if (graphics->timeDependent)
		*timeDependentAddress = true;
	return 1;
}

int cmzn_graphics_glyph_change(
	struct cmzn_graphics *graphics, void *manager_message_void)
{
	struct MANAGER_MESSAGE(cmzn_glyph) *manager_message =
		reinterpret_cast<struct MANAGER_MESSAGE(cmzn_glyph) *>(manager_message_void);
	if (graphics && manager_message)
	{
		if (graphics->glyph)
		{
			int change_flags = MANAGER_MESSAGE_GET_OBJECT_CHANGE(cmzn_glyph)(
				manager_message, graphics->glyph);
			if ((change_flags & MANAGER_CHANGE_RESULT(cmzn_glyph)) != 0)
			{
				cmzn_graphics_update_graphics_object_trivial_glyph(graphics);
				cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_RECOMPILE);
			}
		}
		return 1;
	}
	return 0;
}

int cmzn_material_change(
	struct cmzn_graphics *graphics, void *material_manager_message_void)
{
	int return_code;
	struct MANAGER_MESSAGE(cmzn_material) *manager_message =
		(struct MANAGER_MESSAGE(cmzn_material) *)material_manager_message_void;
	if (graphics && manager_message)
	{
		return_code = 1;
		bool material_change = false;
		if (graphics->material)
		{
			int change_flags = MANAGER_MESSAGE_GET_OBJECT_CHANGE(cmzn_material)(
				manager_message, graphics->material);
			material_change = (change_flags & MANAGER_CHANGE_RESULT(cmzn_material)) != 0;
		}
		if (!material_change && graphics->secondary_material)
		{
			int change_flags = MANAGER_MESSAGE_GET_OBJECT_CHANGE(cmzn_material)(
				manager_message, graphics->secondary_material);
			material_change = (change_flags & MANAGER_CHANGE_RESULT(cmzn_material)) != 0;
		}
		if (!material_change && graphics->selected_material)
		{
			int change_flags = MANAGER_MESSAGE_GET_OBJECT_CHANGE(cmzn_material)(
				manager_message, graphics->selected_material);
			material_change = (change_flags & MANAGER_CHANGE_RESULT(cmzn_material)) != 0;
		}
		if (graphics->glyph)
		{
			graphics->glyph->materialChange(manager_message);
		}
		if (material_change)
		{
			if (graphics->graphics_object)
			{
				GT_object_Graphical_material_change(graphics->graphics_object,
					(struct LIST(cmzn_material) *)NULL);
			}
			/* need a way to tell either graphics is used in any scene or not */
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_RECOMPILE);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_material_change.  Invalid argument(s)");
		return_code = 0;
	}
	return return_code;
}

int cmzn_graphics_spectrum_change(
	struct cmzn_graphics *graphics, void *spectrum_manager_message_void)
{
	int return_code;
	struct MANAGER_MESSAGE(cmzn_spectrum) *manager_message =
		(struct MANAGER_MESSAGE(cmzn_spectrum) *)spectrum_manager_message_void;
	if (graphics && manager_message)
	{
		return_code = 1;
		if (graphics->spectrum)
		{
			int change_flags = MANAGER_MESSAGE_GET_OBJECT_CHANGE(cmzn_spectrum)(
				manager_message, graphics->spectrum);
			if (change_flags & MANAGER_CHANGE_RESULT(cmzn_spectrum))
			{
				if (graphics->graphics_object)
				{
					GT_object_Spectrum_change(graphics->graphics_object,
						(struct LIST(cmzn_spectrum) *)NULL);
				}
				/* need a way to tell either graphics is used in any scene or not */
				cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_RECOMPILE);
			}
		}
		/* The material gets it's own notification of the change,
			it should propagate that to the cmzn_graphics */
		struct cmzn_spectrum *colour_lookup;
		if (graphics->material && (colour_lookup =
				Graphical_material_get_colour_lookup_spectrum(graphics->material)))
		{
			int change_flags = MANAGER_MESSAGE_GET_OBJECT_CHANGE(cmzn_spectrum)(
				manager_message, colour_lookup);
			if (change_flags & MANAGER_CHANGE_RESULT(cmzn_spectrum))
			{
				if (graphics->graphics_object)
				{
					GT_object_Graphical_material_change(graphics->graphics_object,
						(struct LIST(cmzn_material) *)NULL);
				}
				/* need a way to tell either graphics is used in any scene or not */
				cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_RECOMPILE);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_spectrum_change.  Invalid argument(s)");
		return_code = 0;
	}
	return return_code;
}

int cmzn_graphics_tessellation_change(struct cmzn_graphics *graphics,
	void *tessellation_manager_message_void)
{
	int return_code;
	struct MANAGER_MESSAGE(cmzn_tessellation) *manager_message =
		(struct MANAGER_MESSAGE(cmzn_tessellation) *)tessellation_manager_message_void;
	if (graphics && manager_message)
	{
		return_code = 1;
		if (graphics->tessellation)
		{
			const cmzn_tessellation_change_detail *change_detail = 0;
			int change_flags = cmzn_tessellation_manager_message_get_object_change_and_detail(
				manager_message, graphics->tessellation, &change_detail);
			if (change_flags & MANAGER_CHANGE_RESULT(cmzn_tessellation))
			{
				if (change_detail->isElementDivisionsChanged() &&
					(0 < cmzn_graphics_get_domain_dimension(graphics)))
				{
					cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
				}
				else if (change_detail->isCircleDivisionsChanged())
				{
					if (CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_CIRCLE_EXTRUSION == graphics->line_shape)
					{
						cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
					}
					else if (graphics->glyph && graphics->glyph->usesCircleDivisions())
					{
						cmzn_graphics_update_graphics_object_trivial_glyph(graphics);
						cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_RECOMPILE);
					}
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_tessellation_change.  Invalid argument(s)");
		return_code = 0;
	}
	return return_code;
}

int cmzn_graphics_font_change(struct cmzn_graphics *graphics,
	void *font_manager_message_void)
{
	int return_code;
	struct MANAGER_MESSAGE(cmzn_font) *manager_message =
		(struct MANAGER_MESSAGE(cmzn_font) *)font_manager_message_void;
	if (graphics && manager_message)
	{
		return_code = 1;
		if ((graphics->graphics_type == CMZN_GRAPHICS_TYPE_POINTS) && (graphics->font))
		{
			int change_flags = MANAGER_MESSAGE_GET_OBJECT_CHANGE(cmzn_font)(
				manager_message, graphics->font);
			if (change_flags & MANAGER_CHANGE_RESULT(cmzn_font))
			{
				bool glyphUsesFont = (0 != graphics->glyph) && graphics->glyph->usesFont();
				if (glyphUsesFont || graphics->label_field || graphics->label_text[0] ||
					graphics->label_text[1] || graphics->label_text[2])
				{
					if (glyphUsesFont)
					{
						graphics->glyph->fontChange();
					}
					if (graphics->graphics_object)
					{
						if (glyphUsesFont)
						{
							cmzn_graphics_update_graphics_object_trivial_glyph(graphics);
						}
						GT_object_changed(graphics->graphics_object);
					}
					cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_RECOMPILE);
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_font_change.  Invalid argument(s)");
		return_code = 0;
	}
	return return_code;
}

int cmzn_graphics_detach_fields(struct cmzn_graphics *graphics, void *dummy_void)
{
	int return_code = 1;
	USE_PARAMETER(dummy_void);

	if (graphics)
	{
		if (graphics->coordinate_field)
		{
			DEACCESS(Computed_field)(&(graphics->coordinate_field));
		}
		if (graphics->texture_coordinate_field)
		{
			DEACCESS(Computed_field)(&(graphics->texture_coordinate_field));
		}
		if (graphics->line_orientation_scale_field)
		{
			DEACCESS(Computed_field)(&(graphics->line_orientation_scale_field));
		}
		if (graphics->isoscalar_field)
		{
			DEACCESS(Computed_field)(&(graphics->isoscalar_field));
		}
		if (graphics->point_orientation_scale_field)
		{
			DEACCESS(Computed_field)(&(graphics->point_orientation_scale_field));
		}
		if (graphics->signed_scale_field)
		{
			DEACCESS(Computed_field)(&(graphics->signed_scale_field));
		}
		if (graphics->label_field)
		{
			DEACCESS(Computed_field)(&(graphics->label_field));
		}
		if (graphics->label_density_field)
		{
			DEACCESS(Computed_field)(&(graphics->label_density_field));
		}
		if (graphics->subgroup_field)
		{
			DEACCESS(Computed_field)(&(graphics->subgroup_field));
		}
		cmzn_field_destroy(&(graphics->sample_density_field));
		cmzn_field_destroy(&(graphics->tessellation_field));
		if (graphics->stream_vector_field)
		{
			DEACCESS(Computed_field)(&(graphics->stream_vector_field));
		}
		if (graphics->data_field)
		{
			DEACCESS(Computed_field)(&(graphics->data_field));
		}
		if (graphics->seed_node_mesh_location_field)
		{
			DEACCESS(Computed_field)(&(graphics->seed_node_mesh_location_field));
		}
	}
	else
	{
		display_message(INFORMATION_MESSAGE,
			"cmzn_graphics_detach_fields.  Invalid argument(s)");
		return_code = 0;
	}

	return return_code;
}

int cmzn_graphics_selected_element_points_change(
	struct cmzn_graphics *graphics,void *dummy_void)
/*******************************************************************************
LAST MODIFIED : 28 February 2000

DESCRIPTION :
Tells <graphics> that if the graphics resulting from it depend on the currently
selected element points, then they should be updated.
Must call cmzn_graphics_to_graphics_object afterwards to complete.
==============================================================================*/
{
	int return_code;

	ENTER(cmzn_graphics_selected_element_points_change);
	USE_PARAMETER(dummy_void);
	if (graphics)
	{
		return_code=1;
		if (graphics->graphics_object&&
			(CMZN_GRAPHICS_TYPE_POINTS == graphics->graphics_type) &&
			(0 < cmzn_graphics_get_domain_dimension(graphics)))
		{
			cmzn_graphics_update_selected(graphics, (void *)NULL);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_selected_element_points_change.  "
			"Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_graphics_selected_element_points_change */

int cmzn_graphics_set_scene_private(struct cmzn_graphics *graphics,
	struct cmzn_scene *scene)
{
	if (graphics && ((NULL == scene) || (NULL == graphics->scene)))
	{
		if (scene != graphics->scene)
		{
			if (graphics->scene)
			{
				// deregister coordinate/vector field wrappers
				if (graphics->coordinate_field)
				{
					if (graphics->point_orientation_scale_field)
						graphics->scene->deregisterVectorField(graphics->point_orientation_scale_field, graphics->coordinate_field);
					if (graphics->stream_vector_field)
						graphics->scene->deregisterVectorField(graphics->stream_vector_field, graphics->coordinate_field);
					graphics->scene->deregisterCoordinateField(graphics->coordinate_field);
				}
			}
			if (scene && (!scene->isEditorCopy()))
			{
				graphics->scene = scene;
				if (graphics->scene)
				{
					// register coordinate/vector field wrappers
					if (graphics->coordinate_field)
					{
						if (graphics->point_orientation_scale_field)
							graphics->scene->registerVectorField(graphics->point_orientation_scale_field, graphics->coordinate_field);
						if (graphics->stream_vector_field)
							graphics->scene->registerVectorField(graphics->stream_vector_field, graphics->coordinate_field);
						graphics->scene->registerCoordinateField(graphics->coordinate_field);
					}
				}
			}
			else
			{
				graphics->scene = 0;
			}
		}
		return 1;
	}
	else
	{
		display_message(INFORMATION_MESSAGE,
			"cmzn_graphics_set_scene_private.  Invalid argument(s)");
	}
	return 0;
}

int cmzn_graphics_set_scene_for_list_private(struct cmzn_graphics *graphics, void *scene_void)
{
	if (graphics)
		return cmzn_graphics_set_scene_private(graphics, static_cast<cmzn_scene *>(scene_void));
	display_message(INFORMATION_MESSAGE,
		"cmzn_graphics_set_scene_for_list_private.  Invalid argument(s)");
	return 0;
}

cmzn_graphics_id cmzn_graphics_access(cmzn_graphics_id graphics)
{
	if (graphics)
		return (ACCESS(cmzn_graphics)(graphics));
	return 0;
}

int cmzn_graphics_destroy(cmzn_graphics_id *graphics_address)
{
	if (graphics_address && *graphics_address)
	{
		DEACCESS(cmzn_graphics)(graphics_address);
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

class cmzn_graphics_type_conversion
{
public:
	static const char *to_string(enum cmzn_graphics_type type)
	{
		const char *enum_string = 0;
		switch (type)
		{
			case CMZN_GRAPHICS_TYPE_POINTS:
				enum_string = "POINTS";
				break;
			case CMZN_GRAPHICS_TYPE_LINES:
				enum_string = "LINES";
				break;
			case CMZN_GRAPHICS_TYPE_SURFACES:
				enum_string = "SURFACES";
				break;
			case CMZN_GRAPHICS_TYPE_CONTOURS:
				enum_string = "CONTOURS";
				break;
			case CMZN_GRAPHICS_TYPE_STREAMLINES:
				enum_string = "STREAMLINES";
				break;
		default:
			break;
		}
		return enum_string;
	}
};

enum cmzn_graphics_type cmzn_graphics_type_enum_from_string(const char *string)
{
	return string_to_enum<enum cmzn_graphics_type, cmzn_graphics_type_conversion>(string);
}

char *cmzn_graphics_type_enum_to_string(enum cmzn_graphics_type type)
{
	const char *type_string = cmzn_graphics_type_conversion::to_string(type);
	return (type_string ? duplicate_string(type_string) : 0);
}

class cmzn_graphicslineattributes_shape_type_conversion
{
public:
	static const char *to_string(enum cmzn_graphicslineattributes_shape_type type)
	{
		const char *enum_string = 0;
		switch (type)
		{
			case CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_LINE:
				enum_string = "LINE";
				break;
			case CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_RIBBON:
				enum_string = "RIBBON";
				break;
			case CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_CIRCLE_EXTRUSION:
				enum_string = "CIRCLE_EXTRUSION";
				break;
			case CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_SQUARE_EXTRUSION:
				enum_string = "SQUARE_EXTRUSION";
				break;
		default:
			break;
		}
		return enum_string;
	}
};

enum cmzn_graphicslineattributes_shape_type cmzn_graphicslineattributes_shape_type_enum_from_string(const char *string)
{
	return string_to_enum<enum cmzn_graphicslineattributes_shape_type, cmzn_graphicslineattributes_shape_type_conversion>(string);
}

char *cmzn_graphicslineattributes_shape_type_enum_to_string(enum cmzn_graphicslineattributes_shape_type type)
{
	const char *type_string = cmzn_graphicslineattributes_shape_type_conversion::to_string(type);
	return (type_string ? duplicate_string(type_string) : 0);
}

class cmzn_graphics_select_mode_conversion
{
public:
	static const char *to_string(enum cmzn_graphics_select_mode type)
	{
		const char *enum_string = 0;
		switch (type)
		{
		case CMZN_GRAPHICS_SELECT_MODE_ON:
			enum_string = "ON";
			break;
		case CMZN_GRAPHICS_SELECT_MODE_OFF:
			enum_string = "OFF";
			break;
		case CMZN_GRAPHICS_SELECT_MODE_DRAW_SELECTED:
			enum_string = "DRAW_SELECTED";
			break;
		case CMZN_GRAPHICS_SELECT_MODE_DRAW_UNSELECTED:
			enum_string = "DRAW_UNSELECTED";
			break;
		default:
			break;
		}
		return enum_string;
	}
};

enum cmzn_graphics_select_mode cmzn_graphics_select_mode_enum_from_string(
	const char *string)
{
	return string_to_enum<enum cmzn_graphics_select_mode,
		cmzn_graphics_select_mode_conversion>(string);
}

char *cmzn_graphics_select_mode_enum_to_string(enum cmzn_graphics_select_mode type)
{
	const char *type_string = cmzn_graphics_select_mode_conversion::to_string(type);
	return (type_string ? duplicate_string(type_string) : 0);
}

class cmzn_graphics_render_polygon_mode_conversion
{
public:
	static const char *to_string(enum cmzn_graphics_render_polygon_mode type)
	{
		const char *enum_string = 0;
		switch (type)
		{
		case CMZN_GRAPHICS_RENDER_POLYGON_MODE_SHADED:
			enum_string = "RENDER_POLYGON_SHADED";
			break;
		case CMZN_GRAPHICS_RENDER_POLYGON_MODE_WIREFRAME:
			enum_string = "RENDER_POLYGON_WIREFRAME";
			break;
		default:
			break;
		}
		return enum_string;
	}
};

enum cmzn_graphics_render_polygon_mode cmzn_graphics_render_polygon_mode_enum_from_string(
	const char *string)
{
	return string_to_enum<enum cmzn_graphics_render_polygon_mode,
		cmzn_graphics_render_polygon_mode_conversion>(string);
}

char *cmzn_graphics_render_polygon_mode_enum_to_string(
	enum cmzn_graphics_render_polygon_mode type)
{
	const char *type_string = cmzn_graphics_render_polygon_mode_conversion::to_string(type);
	return (type_string ? duplicate_string(type_string) : 0);
}

class cmzn_graphics_streamlines_track_direction_conversion
{
public:
	static const char *to_string(enum cmzn_graphics_streamlines_track_direction direction)
	{
		const char *enum_string = 0;
		switch (direction)
		{
		case CMZN_GRAPHICS_STREAMLINES_TRACK_DIRECTION_FORWARD:
			enum_string = "FORWARD";
			break;
		case CMZN_GRAPHICS_STREAMLINES_TRACK_DIRECTION_REVERSE:
			enum_string = "REVERSE";
			break;
		default:
			break;
		}
		return enum_string;
	}
};

enum cmzn_graphics_streamlines_track_direction cmzn_graphics_streamlines_track_direction_enum_from_string(
	const char *string)
{
	return string_to_enum<enum cmzn_graphics_streamlines_track_direction,
		cmzn_graphics_streamlines_track_direction_conversion>(string);
}

char *cmzn_graphics_streamlines_track_direction_enum_to_string(
	enum cmzn_graphics_streamlines_track_direction direction)
{
	const char *string = cmzn_graphics_streamlines_track_direction_conversion::to_string(direction);
	return (string ? duplicate_string(string) : 0);
}

class cmzn_graphics_streamlines_colour_data_type_conversion
{
public:
	static const char *to_string(enum cmzn_graphics_streamlines_colour_data_type type)
	{
		const char *enum_string = 0;
		switch (type)
		{
		case CMZN_GRAPHICS_STREAMLINES_COLOUR_DATA_TYPE_FIELD:
			enum_string = "FIELD";
			break;
		case CMZN_GRAPHICS_STREAMLINES_COLOUR_DATA_TYPE_MAGNITUDE:
			enum_string = "MAGNITUDE";
			break;
		case CMZN_GRAPHICS_STREAMLINES_COLOUR_DATA_TYPE_TRAVEL_TIME:
			enum_string = "TRAVEL_TIME";
			break;
		default:
			break;
		}
		return enum_string;
	}
};

enum cmzn_graphics_streamlines_colour_data_type cmzn_graphics_streamlines_colour_data_type_enum_from_string(
	const char *string)
{
	return string_to_enum<enum cmzn_graphics_streamlines_colour_data_type,
		cmzn_graphics_streamlines_colour_data_type_conversion>(string);
}

char *cmzn_graphics_streamlines_colour_data_type_enum_to_string(
	enum cmzn_graphics_streamlines_colour_data_type type)
{
	const char *string = cmzn_graphics_streamlines_colour_data_type_conversion::to_string(type);
	return (string ? duplicate_string(string) : 0);
}

enum cmzn_field_domain_type cmzn_graphics_get_field_domain_type(
	cmzn_graphics_id graphics)
{
	if (graphics)
		return graphics->domain_type;
	return CMZN_FIELD_DOMAIN_TYPE_INVALID;
}

int cmzn_graphics_set_field_domain_type(cmzn_graphics_id graphics,
	enum cmzn_field_domain_type domain_type)
{
	if (graphics && (domain_type != CMZN_FIELD_DOMAIN_TYPE_INVALID) &&
		(graphics->graphics_type != CMZN_GRAPHICS_TYPE_LINES) &&
		(graphics->graphics_type != CMZN_GRAPHICS_TYPE_SURFACES) &&
		((graphics->graphics_type == CMZN_GRAPHICS_TYPE_POINTS) ||
		 ((domain_type != CMZN_FIELD_DOMAIN_TYPE_POINT) &&
			(domain_type != CMZN_FIELD_DOMAIN_TYPE_NODES) &&
			(domain_type != CMZN_FIELD_DOMAIN_TYPE_DATAPOINTS))))
	{
		graphics->domain_type = domain_type;
		if (domain_type != graphics->domain_type)
		{
			graphics->domain_type = domain_type;
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_graphics_contours_id cmzn_graphics_cast_contours(cmzn_graphics_id graphics)
{
	if (graphics && (graphics->graphics_type == CMZN_GRAPHICS_TYPE_CONTOURS))
	{
		cmzn_graphics_access(graphics);
		return (reinterpret_cast<cmzn_graphics_contours_id>(graphics));
	}
	return 0;
}

int cmzn_graphics_contours_destroy(cmzn_graphics_contours_id *contours_address)
{
	return cmzn_graphics_destroy(reinterpret_cast<cmzn_graphics_id *>(contours_address));
}

double cmzn_graphics_contours_get_decimation_threshold(
	cmzn_graphics_contours_id contours)
{
	if (contours)
	{
		cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics_id>(contours);
		return graphics->decimation_threshold;
	}
	return 0;
}

int cmzn_graphics_contours_set_decimation_threshold(
	cmzn_graphics_contours_id contours, double decimation_threshold)
{
	if (contours)
	{
		cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics_id>(contours);
		if (decimation_threshold != graphics->decimation_threshold)
		{
			graphics->decimation_threshold = decimation_threshold;
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_field_id cmzn_graphics_contours_get_isoscalar_field(
	cmzn_graphics_contours_id contours)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics_id>(contours);
	if (graphics && graphics->isoscalar_field)
	{
		return cmzn_field_access(graphics->isoscalar_field);
	}
	return 0;
}

int cmzn_graphics_contours_set_isoscalar_field(
	cmzn_graphics_contours_id contours,
	cmzn_field_id isoscalar_field)
{
	if (contours && ((0 == isoscalar_field) ||
		(1 == cmzn_field_get_number_of_components(isoscalar_field))))
	{
		cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics_id>(contours);
		if (isoscalar_field != graphics->isoscalar_field)
		{
			REACCESS(Computed_field)(&(graphics->isoscalar_field), isoscalar_field);
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_graphics_contours_get_list_isovalues(
	cmzn_graphics_contours_id contours, int number_of_isovalues,
	double *isovalues)
{
	if (contours && ((number_of_isovalues == 0) ||
		((number_of_isovalues > 0) && isovalues)))
	{
		cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics_id>(contours);
		if (graphics->isovalues)
		{
			const int number_to_copy = (number_of_isovalues < graphics->number_of_isovalues) ?
				number_of_isovalues : graphics->number_of_isovalues;
			for (int i = 0 ; i < number_to_copy ; i++)
			{
				isovalues[i] = graphics->isovalues[i];
			}
			return graphics->number_of_isovalues;
		}
	}
	return 0;
}

int cmzn_graphics_contours_set_list_isovalues(
	cmzn_graphics_contours_id contours, int number_of_isovalues,
	const double *isovalues)
{
	if (contours && ((number_of_isovalues == 0) ||
		((number_of_isovalues > 0) && isovalues)))
	{
		cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics_id>(contours);
		bool changed = false;
		if (number_of_isovalues == graphics->number_of_isovalues)
		{
			if (graphics->isovalues)
			{
				for (int i = 0; i < number_of_isovalues; i++)
				{
					if (isovalues[i] != graphics->isovalues[i])
					{
						changed = true;
						break;
					}
				}
			}
			else
			{
				changed = true;
			}
		}
		else
		{
			changed = true;
		}
		if (changed)
		{
			if (0 < number_of_isovalues)
			{
				double *temp_values;
				REALLOCATE(temp_values, graphics->isovalues, double, number_of_isovalues);
				if (!temp_values)
				{
					return CMZN_ERROR_MEMORY;
				}
				graphics->isovalues = temp_values;
				graphics->number_of_isovalues = number_of_isovalues;
				for (int i = 0 ; i < number_of_isovalues ; i++)
				{
					graphics->isovalues[i] = isovalues[i];
				}
			}
			else
			{
				if (graphics->isovalues)
				{
					DEALLOCATE(graphics->isovalues);
					graphics->isovalues = 0;
				}
				graphics->number_of_isovalues = 0;
			}
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

double cmzn_graphics_contours_get_range_first_isovalue(
	cmzn_graphics_contours_id contours)
{
	if (contours)
	{
		cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics_id>(contours);
		if (0 == graphics->isovalues)
		{
			return graphics->first_isovalue;
		}
	}
	return 0.0;
}

double cmzn_graphics_contours_get_range_last_isovalue(
	cmzn_graphics_contours_id contours)
{
	if (contours)
	{
		cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics_id>(contours);
		if (0 == graphics->isovalues)
		{
			return graphics->last_isovalue;
		}
	}
	return 0.0;
}

int cmzn_graphics_contours_get_range_number_of_isovalues(
	cmzn_graphics_contours_id contours)
{
	if (contours)
	{
		cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics_id>(contours);
		if (0 == graphics->isovalues)
		{
			return graphics->number_of_isovalues;
		}
	}
	return 0;
}

int cmzn_graphics_contours_set_range_isovalues(
	cmzn_graphics_contours_id contours, int number_of_isovalues,
	double first_isovalue, double last_isovalue)
{
	if (contours && (0 <= number_of_isovalues))
	{
		cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics_id>(contours);
		if ((number_of_isovalues != graphics->number_of_isovalues) ||
			(0 != graphics->isovalues) || (first_isovalue != graphics->first_isovalue) ||
			(last_isovalue != graphics->last_isovalue))
		{
			if (graphics->isovalues)
			{
				DEALLOCATE(graphics->isovalues);
				graphics->isovalues = 0;
			}
			graphics->number_of_isovalues = number_of_isovalues;
			graphics->first_isovalue = first_isovalue;
			graphics->last_isovalue = last_isovalue;
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_graphics_lines_id cmzn_graphics_cast_lines(cmzn_graphics_id graphics)
{
	if (graphics && (graphics->graphics_type == CMZN_GRAPHICS_TYPE_LINES))
	{
		cmzn_graphics_access(graphics);
		return (reinterpret_cast<cmzn_graphics_lines_id>(graphics));
	}
	return 0;
}

int cmzn_graphics_lines_destroy(cmzn_graphics_lines_id *lines_address)
{
	return cmzn_graphics_destroy(reinterpret_cast<cmzn_graphics_id *>(lines_address));
}

cmzn_graphics_points_id cmzn_graphics_cast_points(cmzn_graphics_id graphics)
{
	if (graphics && (graphics->graphics_type == CMZN_GRAPHICS_TYPE_POINTS))
	{
		cmzn_graphics_access(graphics);
		return (reinterpret_cast<cmzn_graphics_points_id>(graphics));
	}
	return 0;
}

int cmzn_graphics_points_destroy(cmzn_graphics_points_id *points_address)
{
	return cmzn_graphics_destroy(reinterpret_cast<cmzn_graphics_id *>(points_address));
}

cmzn_graphics_streamlines_id cmzn_graphics_cast_streamlines(cmzn_graphics_id graphics)
{
	if (graphics && (graphics->graphics_type == CMZN_GRAPHICS_TYPE_STREAMLINES))
	{
		cmzn_graphics_access(graphics);
		return (reinterpret_cast<cmzn_graphics_streamlines_id>(graphics));
	}
	return 0;
}

int cmzn_graphics_streamlines_destroy(cmzn_graphics_streamlines_id *streamlines_address)
{
	return cmzn_graphics_destroy(reinterpret_cast<cmzn_graphics_id *>(streamlines_address));
}

enum cmzn_graphics_streamlines_colour_data_type
	cmzn_graphics_streamlines_get_colour_data_type(
		cmzn_graphics_streamlines_id streamlines)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(streamlines);
	if (graphics)
		return graphics->streamlines_colour_data_type;
	return CMZN_GRAPHICS_STREAMLINES_COLOUR_DATA_TYPE_INVALID;
}

int cmzn_graphics_streamlines_set_colour_data_type(
	cmzn_graphics_streamlines_id streamlines,
	enum cmzn_graphics_streamlines_colour_data_type streamlines_colour_data_type)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(streamlines);
	if (graphics && (streamlines_colour_data_type != CMZN_GRAPHICS_STREAMLINES_COLOUR_DATA_TYPE_INVALID))
	{
		if (streamlines_colour_data_type != graphics->streamlines_colour_data_type)
		{
			graphics->streamlines_colour_data_type = streamlines_colour_data_type;
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_field_id cmzn_graphics_streamlines_get_stream_vector_field(
	cmzn_graphics_streamlines_id streamlines)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(streamlines);
	if (graphics && (graphics->stream_vector_field))
	{
		return ACCESS(Computed_field)(graphics->stream_vector_field);
	}
	return 0;
}

int cmzn_graphics_streamlines_set_stream_vector_field(
	cmzn_graphics_streamlines_id streamlines,
	cmzn_field_id stream_vector_field)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(streamlines);
	if (graphics)
	{
		if (stream_vector_field != graphics->stream_vector_field)
		{
			if (graphics->scene && graphics->stream_vector_field && graphics->coordinate_field)
			{
				graphics->scene->deregisterVectorField(
					graphics->stream_vector_field, graphics->coordinate_field);
			}
			REACCESS(Computed_field)(&(graphics->stream_vector_field), stream_vector_field);
			if (graphics->scene && graphics->stream_vector_field && graphics->coordinate_field)
			{
				graphics->scene->registerVectorField(
					graphics->stream_vector_field, graphics->coordinate_field);
			}
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

enum cmzn_graphics_streamlines_track_direction
	cmzn_graphics_streamlines_get_track_direction(
		cmzn_graphics_streamlines_id streamlines)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(streamlines);
	if (graphics)
		return graphics->streamlines_track_direction;
	return CMZN_GRAPHICS_STREAMLINES_TRACK_DIRECTION_INVALID;
}

int cmzn_graphics_streamlines_set_track_direction(
	cmzn_graphics_streamlines_id streamlines,
	enum cmzn_graphics_streamlines_track_direction track_direction)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(streamlines);
	if (graphics && (track_direction != CMZN_GRAPHICS_STREAMLINES_TRACK_DIRECTION_INVALID))
	{
		if (track_direction != graphics->streamlines_track_direction)
		{
			graphics->streamlines_track_direction = track_direction;
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

double cmzn_graphics_streamlines_get_track_length(
	cmzn_graphics_streamlines_id streamlines)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(streamlines);
	if (graphics)
		return graphics->streamline_length;
	return 0.0;
}

int cmzn_graphics_streamlines_set_track_length(
	cmzn_graphics_streamlines_id streamlines, double length)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(streamlines);
	if (graphics && (length >= 0.0))
	{
		if (length != graphics->streamline_length)
		{
			graphics->streamline_length = length;
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_graphics_surfaces_id cmzn_graphics_cast_surfaces(cmzn_graphics_id graphics)
{
	if (graphics && (graphics->graphics_type == CMZN_GRAPHICS_TYPE_SURFACES))
	{
		cmzn_graphics_access(graphics);
		return (reinterpret_cast<cmzn_graphics_surfaces_id>(graphics));
	}
	return 0;
}

int cmzn_graphics_surfaces_destroy(cmzn_graphics_surfaces_id *surfaces_address)
{
	return cmzn_graphics_destroy(reinterpret_cast<cmzn_graphics_id *>(surfaces_address));
}

cmzn_graphicslineattributes_id cmzn_graphics_get_graphicslineattributes(
	cmzn_graphics_id graphics)
{
	if (graphics && (
		(graphics->graphics_type == CMZN_GRAPHICS_TYPE_LINES) ||
		(graphics->graphics_type == CMZN_GRAPHICS_TYPE_STREAMLINES)))
	{
		cmzn_graphics_access(graphics);
		return reinterpret_cast<cmzn_graphicslineattributes_id>(graphics);
	}
	return 0;
}

cmzn_graphicslineattributes_id cmzn_graphicslineattributes_access(
	cmzn_graphicslineattributes_id line_attributes)
{
	cmzn_graphics_access(reinterpret_cast<cmzn_graphics_id>(line_attributes));
	return line_attributes;
}

int cmzn_graphicslineattributes_destroy(
	cmzn_graphicslineattributes_id *line_attributes_address)
{
	return cmzn_graphics_destroy(reinterpret_cast<cmzn_graphics_id *>(line_attributes_address));
}

int cmzn_graphicslineattributes_get_base_size(
	cmzn_graphicslineattributes_id line_attributes, int number,
	double *base_size)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(line_attributes);
	if (graphics && (0 < number) && base_size)
	{
		const int count = (number > 2) ? 2 : number;
		for (int i = 0; i < count; ++i)
		{
			base_size[i] = static_cast<double>(graphics->line_base_size[i]);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_graphicslineattributes_set_base_size(
	cmzn_graphicslineattributes_id line_attributes, int number,
	const double *base_size)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(line_attributes);
	if (graphics && (0 < number) && base_size)
	{
		bool changed = false;
		if (graphics->graphics_type == CMZN_GRAPHICS_TYPE_LINES)
		{
			number = 1; // only equal values supported for lines (cylinders)
		}
		FE_value value;
		for (int i = 0; i < 2; ++i)
		{
			if (i < number)
			{
				value = static_cast<FE_value>(base_size[i]);
			}
			if (graphics->line_base_size[i] != value)
			{
				graphics->line_base_size[i] = value;
				changed = true;
			}
		}
		if (changed)
		{
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_field_id cmzn_graphicslineattributes_get_orientation_scale_field(
	cmzn_graphicslineattributes_id line_attributes)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(line_attributes);
	if (graphics && (graphics->line_orientation_scale_field))
	{
		return ACCESS(Computed_field)(graphics->line_orientation_scale_field);
	}
	return 0;
}

int cmzn_graphicslineattributes_set_orientation_scale_field(
	cmzn_graphicslineattributes_id line_attributes,
	cmzn_field_id orientation_scale_field)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(line_attributes);
	if (graphics)
	{
		if (orientation_scale_field != graphics->line_orientation_scale_field)
		{
			REACCESS(Computed_field)(&(graphics->line_orientation_scale_field), orientation_scale_field);
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_graphicslineattributes_get_scale_factors(
	cmzn_graphicslineattributes_id line_attributes, int number,
	double *scale_factors)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(line_attributes);
	if (graphics && (0 < number) && scale_factors)
	{
		const int count = (number > 2) ? 2 : number;
		for (int i = 0; i < count; ++i)
		{
			scale_factors[i] = static_cast<double>(graphics->line_scale_factors[i]);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_graphicslineattributes_set_scale_factors(
	cmzn_graphicslineattributes_id line_attributes, int number,
	const double *scale_factors)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(line_attributes);
	if (graphics && (0 < number) && scale_factors)
	{
		bool changed = false;
		if (graphics->graphics_type == CMZN_GRAPHICS_TYPE_LINES)
		{
			number = 1; // only equal values supported for lines (cylinders)
		}
		FE_value value;
		for (int i = 0; i < 2; ++i)
		{
			if (i < number)
			{
				value = static_cast<FE_value>(scale_factors[i]);
			}
			if (graphics->line_scale_factors[i] != value)
			{
				graphics->line_scale_factors[i] = value;
				changed = true;
			}
		}
		if (changed)
		{
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

enum cmzn_graphicslineattributes_shape_type cmzn_graphicslineattributes_get_shape_type(
	cmzn_graphicslineattributes_id line_attributes)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(line_attributes);
	if (graphics)
	{
		return graphics->line_shape;
	}
	return CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_INVALID;
}

int cmzn_graphicslineattributes_set_shape_type(
	cmzn_graphicslineattributes_id line_attributes,
	enum cmzn_graphicslineattributes_shape_type shape_type)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(line_attributes);
	if (graphics && (shape_type != CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_INVALID) &&
		((graphics->graphics_type == CMZN_GRAPHICS_TYPE_STREAMLINES) ||
			(shape_type == CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_LINE) ||
			(shape_type == CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_CIRCLE_EXTRUSION)))
	{
		if (shape_type != graphics->line_shape)
		{
			graphics->line_shape = shape_type;
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_graphicspointattributes_id cmzn_graphics_get_graphicspointattributes(
	cmzn_graphics_id graphics)
{
	if (graphics && (
		(graphics->graphics_type == CMZN_GRAPHICS_TYPE_POINTS)))
	{
		cmzn_graphics_access(graphics);
		return reinterpret_cast<cmzn_graphicspointattributes_id>(graphics);
	}
	return 0;
}

cmzn_graphicspointattributes_id cmzn_graphicspointattributes_access(
	cmzn_graphicspointattributes_id point_attributes)
{
	cmzn_graphics_access(reinterpret_cast<cmzn_graphics_id>(point_attributes));
	return point_attributes;
}

int cmzn_graphicspointattributes_destroy(
	cmzn_graphicspointattributes_id *point_attributes_address)
{
	return cmzn_graphics_destroy(reinterpret_cast<cmzn_graphics_id *>(point_attributes_address));
}

int cmzn_graphicspointattributes_get_base_size(
	cmzn_graphicspointattributes_id point_attributes, int number,
	double *base_size)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(point_attributes);
	if (graphics && (0 < number) && base_size)
	{
		const int count = (number > 3) ? 3 : number;
		for (int i = 0; i < count; ++i)
		{
			base_size[i] = static_cast<double>(graphics->point_base_size[i]);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_graphicspointattributes_set_base_size(
	cmzn_graphicspointattributes_id point_attributes, int number,
	const double *base_size)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(point_attributes);
	if (graphics && (0 < number) && base_size)
	{
		bool changed = false;
		FE_value value;
		for (int i = 0; i < 3; ++i)
		{
			if (i < number)
			{
				value = static_cast<FE_value>(base_size[i]);
			}
			if (graphics->point_base_size[i] != value)
			{
				graphics->point_base_size[i] = value;
				changed = true;
			}
		}
		if (changed)
		{
			cmzn_graphics_update_graphics_object_trivial(graphics);
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_RECOMPILE);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_font_id cmzn_graphicspointattributes_get_font(
	cmzn_graphicspointattributes_id point_attributes)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(point_attributes);
	if (graphics && (graphics->font))
	{
		return ACCESS(cmzn_font)(graphics->font);
	}
	return 0;
}

int cmzn_graphicspointattributes_set_font(
	cmzn_graphicspointattributes_id point_attributes,
	cmzn_font_id font)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(point_attributes);
	if (graphics)
	{
		if (font != graphics->font)
		{
			REACCESS(cmzn_font)(&(graphics->font), font);
			cmzn_graphics_update_graphics_object_trivial(graphics);
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_RECOMPILE);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_glyph_id cmzn_graphicspointattributes_get_glyph(
	cmzn_graphicspointattributes_id point_attributes)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(point_attributes);
	if (graphics && graphics->glyph)
	{
		return cmzn_glyph_access(graphics->glyph);
	}
	return 0;
}

int cmzn_graphicspointattributes_set_glyph(
	cmzn_graphicspointattributes_id point_attributes, cmzn_glyph_id glyph)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(point_attributes);
	if (graphics)
	{
		if (glyph != graphics->glyph)
		{
			REACCESS(cmzn_glyph)(&(graphics->glyph), glyph);
			cmzn_graphics_update_graphics_object_trivial(graphics);
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_RECOMPILE);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_graphicspointattributes_get_glyph_offset(
	cmzn_graphicspointattributes_id point_attributes, int number,
	double *offset)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(point_attributes);
	if (graphics && (0 < number) && offset)
	{
		const int count = (number > 3) ? 3 : number;
		for (int i = 0; i < count; ++i)
		{
			offset[i] = static_cast<double>(graphics->point_offset[i]);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_graphicspointattributes_set_glyph_offset(
	cmzn_graphicspointattributes_id point_attributes, int number,
	const double *offset)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(point_attributes);
	if (graphics && (0 < number) && offset)
	{
		bool changed = false;
		FE_value value = 0.0;
		for (int i = 2; 0 <= i; --i)
		{
			if (i < number)
			{
				value = static_cast<FE_value>(offset[i]);
			}
			if (graphics->point_offset[i] != value)
			{
				graphics->point_offset[i] = value;
				changed = true;
			}
		}
		if (changed)
		{
			cmzn_graphics_update_graphics_object_trivial(graphics);
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_RECOMPILE);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

enum cmzn_glyph_repeat_mode
	cmzn_graphicspointattributes_get_glyph_repeat_mode(
		cmzn_graphicspointattributes_id point_attributes)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(point_attributes);
	if (graphics)
	{
		return graphics->glyph_repeat_mode;
	}
	return CMZN_GLYPH_REPEAT_MODE_INVALID;
}

int cmzn_graphicspointattributes_set_glyph_repeat_mode(
	cmzn_graphicspointattributes_id point_attributes,
	enum cmzn_glyph_repeat_mode glyph_repeat_mode)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(point_attributes);
	if (graphics && (glyph_repeat_mode !=
		CMZN_GLYPH_REPEAT_MODE_INVALID))
	{
		if (glyph_repeat_mode != graphics->glyph_repeat_mode)
		{
			graphics->glyph_repeat_mode = glyph_repeat_mode;
			cmzn_graphics_update_graphics_object_trivial(graphics);
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_RECOMPILE);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

enum cmzn_glyph_shape_type cmzn_graphicspointattributes_get_glyph_shape_type(
	cmzn_graphicspointattributes_id point_attributes)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(point_attributes);
	if (graphics)
	{
		if (graphics->glyph)
		{
			return graphics->glyph->getType();
		}
		else
		{
			return CMZN_GLYPH_SHAPE_TYPE_NONE;
		}
	}
	return CMZN_GLYPH_SHAPE_TYPE_INVALID;
}

int cmzn_graphicspointattributes_set_glyph_shape_type(
	cmzn_graphicspointattributes_id point_attributes,
	enum cmzn_glyph_shape_type glyph_shape_type)
{
	int return_code = CMZN_ERROR_ARGUMENT;
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(point_attributes);
	if (graphics && (CMZN_GLYPH_SHAPE_TYPE_INVALID != glyph_shape_type))
	{
		cmzn_graphics_module* graphics_module = graphics->scene->getGraphicsmodule();
		cmzn_glyphmodule_id glyphmodule = cmzn_graphics_module_get_glyphmodule(graphics_module);
		cmzn_glyph_id glyph = glyphmodule->findGlyphByType(glyph_shape_type);
		if (glyph || (glyph_shape_type == CMZN_GLYPH_SHAPE_TYPE_NONE))
		{
			return_code = cmzn_graphicspointattributes_set_glyph(point_attributes, glyph);
		}
		cmzn_glyphmodule_destroy(&glyphmodule);
	}
	return return_code;
}

cmzn_field_id cmzn_graphicspointattributes_get_label_field(
	cmzn_graphicspointattributes_id point_attributes)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(point_attributes);
	if (graphics && (graphics->label_field))
	{
		return ACCESS(Computed_field)(graphics->label_field);
	}
	return 0;
}

int cmzn_graphicspointattributes_set_label_field(
	cmzn_graphicspointattributes_id point_attributes, cmzn_field_id label_field)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(point_attributes);
	if (graphics)
	{
		if (label_field != graphics->label_field)
		{
			REACCESS(Computed_field)(&(graphics->label_field), label_field);
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_graphicspointattributes_get_label_offset(
	cmzn_graphicspointattributes_id point_attributes, int number,
	double *label_offset)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(point_attributes);
	if (graphics && (0 < number) && label_offset)
	{
		const int count = (number > 3) ? 3 : number;
		for (int i = 0; i < count; ++i)
		{
			label_offset[i] = static_cast<double>(graphics->label_offset[i]);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_graphicspointattributes_set_label_offset(
	cmzn_graphicspointattributes_id point_attributes, int number,
	const double *label_offset)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(point_attributes);
	if (graphics && (0 < number) && label_offset)
	{
		bool changed = false;
		FE_value value = 0.0;
		for (int i = 2; 0 <= i; --i)
		{
			if (i < number)
			{
				value = static_cast<FE_value>(label_offset[i]);
			}
			if (graphics->label_offset[i] != value)
			{
				graphics->label_offset[i] = value;
				changed = true;
			}
		}
		if (changed)
		{
			cmzn_graphics_update_graphics_object_trivial(graphics);
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_RECOMPILE);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

char *cmzn_graphicspointattributes_get_label_text(
	cmzn_graphicspointattributes_id point_attributes, int label_number)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(point_attributes);
	if (graphics && (0 < label_number) && (label_number <= 3) &&
		(graphics->label_text[label_number - 1]))
	{
		return duplicate_string(graphics->label_text[label_number - 1]);
	}
	return 0;
}

int cmzn_graphicspointattributes_set_label_text(
	cmzn_graphicspointattributes_id point_attributes, int label_number,
	const char *label_text)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(point_attributes);
	if (graphics && (0 < label_number) && (label_number <= 3))
	{
		if (!labels_match(label_text, graphics->label_text[label_number - 1]))
		{
			if (graphics->label_text[label_number - 1])
			{
				DEALLOCATE(graphics->label_text[label_number - 1]);
			}
			graphics->label_text[label_number - 1] =
				(label_text && (0 < strlen(label_text))) ? duplicate_string(label_text) : 0;
			cmzn_graphics_update_graphics_object_trivial(graphics);
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_RECOMPILE);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_field_id cmzn_graphicspointattributes_get_orientation_scale_field(
	cmzn_graphicspointattributes_id point_attributes)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(point_attributes);
	if (graphics && (graphics->point_orientation_scale_field))
	{
		return ACCESS(Computed_field)(graphics->point_orientation_scale_field);
	}
	return 0;
}

int cmzn_graphicspointattributes_set_orientation_scale_field(
	cmzn_graphicspointattributes_id point_attributes,
	cmzn_field_id orientation_scale_field)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(point_attributes);
	if (graphics && ((0 == orientation_scale_field) ||
		Computed_field_is_orientation_scale_capable(orientation_scale_field, (void *)0)))
	{
		if (orientation_scale_field != graphics->point_orientation_scale_field)
		{
			if (graphics->scene && graphics->point_orientation_scale_field && graphics->coordinate_field)
			{
				graphics->scene->deregisterVectorField(
					graphics->point_orientation_scale_field, graphics->coordinate_field);
			}
			REACCESS(Computed_field)(&(graphics->point_orientation_scale_field), orientation_scale_field);
			if (graphics->scene && graphics->point_orientation_scale_field && graphics->coordinate_field)
			{
				graphics->scene->registerVectorField(
					graphics->point_orientation_scale_field, graphics->coordinate_field);
			}
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_graphicspointattributes_get_scale_factors(
	cmzn_graphicspointattributes_id point_attributes, int number,
	double *scale_factors)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(point_attributes);
	if (graphics && (0 < number) && scale_factors)
	{
		const int count = (number > 3) ? 3 : number;
		for (int i = 0; i < count; ++i)
		{
			scale_factors[i] = static_cast<double>(graphics->point_scale_factors[i]);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_graphicspointattributes_set_scale_factors(
	cmzn_graphicspointattributes_id point_attributes, int number,
	const double *scale_factors)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(point_attributes);
	if (graphics && (0 < number) && scale_factors)
	{
		bool changed = false;
		FE_value value = 0.0;
		for (int i = 0; i < 3; ++i)
		{
			if (i < number)
			{
				value = static_cast<FE_value>(scale_factors[i]);
			}
			if (graphics->point_scale_factors[i] != value)
			{
				graphics->point_scale_factors[i] = value;
				changed = true;
			}
		}
		if (changed)
		{
			cmzn_graphics_update_graphics_object_trivial(graphics);
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_RECOMPILE);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_field_id cmzn_graphicspointattributes_get_signed_scale_field(
	cmzn_graphicspointattributes_id point_attributes)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(point_attributes);
	if (graphics && (graphics->signed_scale_field))
	{
		return ACCESS(Computed_field)(graphics->signed_scale_field);
	}
	return 0;
}

int cmzn_graphicspointattributes_set_signed_scale_field(
	cmzn_graphicspointattributes_id point_attributes,
	cmzn_field_id signed_scale_field)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(point_attributes);
	if (graphics && ((!signed_scale_field) ||
		Computed_field_has_up_to_3_numerical_components(signed_scale_field,(void *)NULL)))
	{
		if (signed_scale_field != graphics->signed_scale_field)
		{
			REACCESS(Computed_field)(&(graphics->signed_scale_field), signed_scale_field);
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_graphicssamplingattributes_id cmzn_graphics_get_graphicssamplingattributes(
	cmzn_graphics_id graphics)
{
	if (graphics && ((graphics->graphics_type == CMZN_GRAPHICS_TYPE_POINTS) ||
		(graphics->graphics_type == CMZN_GRAPHICS_TYPE_STREAMLINES)))
	{
		cmzn_graphics_access(graphics);
		return reinterpret_cast<cmzn_graphicssamplingattributes_id>(graphics);
	}
	return 0;
}

cmzn_graphicssamplingattributes_id cmzn_graphicssamplingattributes_access(
	cmzn_graphicssamplingattributes_id sampling_attributes)
{
	cmzn_graphics_access(reinterpret_cast<cmzn_graphics_id>(sampling_attributes));
	return sampling_attributes;
}

int cmzn_graphicssamplingattributes_destroy(
	cmzn_graphicssamplingattributes_id *sampling_attributes_address)
{
	return cmzn_graphics_destroy(reinterpret_cast<cmzn_graphics_id *>(sampling_attributes_address));
}

cmzn_field_id cmzn_graphicssamplingattributes_get_density_field(
	cmzn_graphicssamplingattributes_id sampling_attributes)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(sampling_attributes);
	if (graphics && graphics->sample_density_field)
		return ACCESS(Computed_field)(graphics->sample_density_field);
	return 0;
}

int cmzn_graphicssamplingattributes_set_density_field(
	cmzn_graphicssamplingattributes_id sampling_attributes,
	cmzn_field_id sample_density_field)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(sampling_attributes);
	if (graphics && ((!sample_density_field) ||
		Computed_field_is_scalar(sample_density_field, (void *)0)))
	{
		if (sample_density_field != graphics->sample_density_field)
		{
			REACCESS(Computed_field)(&(graphics->sample_density_field), sample_density_field);
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_graphicssamplingattributes_get_location(
	cmzn_graphicssamplingattributes_id sampling_attributes,
	int valuesCount, double *valuesOut)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(sampling_attributes);
	if (graphics && (0 < valuesCount) && valuesOut)
	{
		const int count = (valuesCount > 3) ? 3 : valuesCount;
		for (int i = 0; i < count; ++i)
		{
			valuesOut[i] = static_cast<double>(graphics->sample_location[i]);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_graphicssamplingattributes_set_location(
	cmzn_graphicssamplingattributes_id sampling_attributes,
	int valuesCount, const double *valuesIn)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(sampling_attributes);
	if (graphics && (0 < valuesCount) && valuesIn)
	{
		bool changed = false;
		FE_value value = 0.0;
		for (int i = 2; 0 <= i; --i)
		{
			if (i < valuesCount)
			{
				value = static_cast<FE_value>(valuesIn[i]);
			}
			if (graphics->sample_location[i] != value)
			{
				graphics->sample_location[i] = value;
				changed = true;
			}
		}
		if (changed)
		{
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

enum cmzn_element_point_sampling_mode cmzn_graphicssamplingattributes_get_element_point_sampling_mode(
	cmzn_graphicssamplingattributes_id sampling_attributes)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(sampling_attributes);
	if (graphics)
		return graphics->sampling_mode;
	return CMZN_ELEMENT_POINT_SAMPLING_MODE_INVALID;
}

int cmzn_graphicssamplingattributes_set_element_point_sampling_mode(
	cmzn_graphicssamplingattributes_id sampling_attributes,
	enum cmzn_element_point_sampling_mode sampling_mode)
{
	cmzn_graphics *graphics = reinterpret_cast<cmzn_graphics *>(sampling_attributes);
	if (graphics && (0 != ENUMERATOR_STRING(cmzn_element_point_sampling_mode)(sampling_mode)))
	{
		if (sampling_mode != graphics->sampling_mode)
		{
			graphics->sampling_mode = sampling_mode;
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

void cmzn_graphics_flag_glyph_has_changed(struct cmzn_graphics *graphics)
{
	if (graphics)
	{
		cmzn_graphics_update_graphics_object_trivial_glyph(graphics);
		cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_RECOMPILE);
	}
}

int cmzn_graphics_flag_for_full_rebuild(
	struct cmzn_graphics *graphics,void *dummy_void)
{
	int return_code;

	if (graphics)
	{
		return_code = 1;
		if (graphics->timeDependent)
		{
			cmzn_graphics_changed(graphics, CMZN_GRAPHICS_CHANGE_FULL_REBUILD);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_time_change.  Invalid argument(s)");
		return_code=0;
	}

	return (return_code);
}

enum GT_object_type cmzn_graphics_get_graphics_object_type(struct cmzn_graphics *graphics)
{
	if (graphics)
	{
		int dimension = cmzn_graphics_get_domain_dimension(graphics);
		switch (graphics->graphics_type)
		{
		case CMZN_GRAPHICS_TYPE_LINES:
		{
			if (CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_LINE == graphics->line_shape)
			{
				return g_POLYLINE_VERTEX_BUFFERS;
			}
			else
			{
				return g_SURFACE_VERTEX_BUFFERS;
			}
		} break;
		case CMZN_GRAPHICS_TYPE_SURFACES:
		{
			return g_SURFACE_VERTEX_BUFFERS;
		} break;
		case CMZN_GRAPHICS_TYPE_CONTOURS:
		{
			switch (dimension)
			{
			case 3:
			{
				return g_SURFACE_VERTEX_BUFFERS;
			} break;
			case 2:
			{
				return g_POLYLINE_VERTEX_BUFFERS;
			} break;
			default:
			{
				return g_OBJECT_TYPE_INVALID;
			} break;
			}
		} break;
		case CMZN_GRAPHICS_TYPE_POINTS:
		{
			return g_GLYPH_SET_VERTEX_BUFFERS;
		} break;
		case CMZN_GRAPHICS_TYPE_STREAMLINES:
		{
			if (CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_LINE == graphics->line_shape)
			{
				return g_POLYLINE_VERTEX_BUFFERS;
			}
			else
			{
				return g_SURFACE_VERTEX_BUFFERS;
			}
		} break;
		default:
		{
		}break;
		}
	}
	return g_OBJECT_TYPE_INVALID;
}

/**
 * This function first make a copy of the provided graphics then create the
 * gt_object required for generating the vertices. Once vertices are generated,
 * the gt_object will be returned.
 */
struct GT_object *cmzn_graphics_copy_graphics_object(struct cmzn_graphics *graphics)
{
	struct GT_object *return_object = 0;
	if (graphics && graphics->scene)
	{
		cmzn_graphics *copy_graphics=CREATE(cmzn_graphics)(graphics->graphics_type);
		if (copy_graphics)
		{
			/* copy and insert in list */
			if (cmzn_graphics_copy_without_graphics_object(copy_graphics,graphics))
			{
				struct cmzn_graphics_to_graphics_object_data graphics_to_object_data;
				graphics_to_object_data.name_prefix = "temp";
				graphics_to_object_data.graphics = 0;
				graphics_to_object_data.glyph_gt_object = 0;
				graphics_to_object_data.build_graphics = 0;
				graphics_to_object_data.number_of_data_values = 0;
				graphics_to_object_data.rc_coordinate_field = (struct Computed_field *) NULL;
				graphics_to_object_data.wrapper_orientation_scale_field = (struct Computed_field *) NULL;
				graphics_to_object_data.wrapper_stream_vector_field = (struct Computed_field *) NULL;
				graphics_to_object_data.region = cmzn_scene_get_region_internal(graphics->scene);
				graphics_to_object_data.field_module = cmzn_region_get_fieldmodule(
					cmzn_scene_get_region_internal(graphics->scene));
				// cache changes to avoid reporting add/remove temporary wrapper fields
				cmzn_fieldmodule_begin_change(graphics_to_object_data.field_module);
				graphics_to_object_data.field_cache = cmzn_fieldmodule_create_fieldcache(
					graphics_to_object_data.field_module);
				graphics_to_object_data.fe_region = cmzn_scene_get_region_internal(graphics->scene)->get_FE_region();
				graphics_to_object_data.master_mesh = 0;
				graphics_to_object_data.iteration_mesh = 0;
				graphics_to_object_data.scenefilter = 0;
				graphics_to_object_data.time = 0;
				graphics_to_object_data.incrementalBuild = 0;
				graphics_to_object_data.selection_group_field = cmzn_scene_get_selection_field(
					graphics->scene);
				graphics_to_object_data.iso_surface_specification = 0;
				for (int i = 0; i < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++i)
				{
					graphics_to_object_data.top_level_number_in_xi[i] = 0;
				}

				cmzn_graphics_to_graphics_object_no_check_on_filter(copy_graphics,
					&graphics_to_object_data);
				return_object = ACCESS(GT_object)(copy_graphics->graphics_object);
				if (graphics_to_object_data.selection_group_field)
				{
					cmzn_field_destroy(&graphics_to_object_data.selection_group_field);
				}
				cmzn_fieldcache_destroy(&graphics_to_object_data.field_cache);
				cmzn_fieldmodule_end_change(graphics_to_object_data.field_module);
				cmzn_fieldmodule_destroy(&graphics_to_object_data.field_module);
			}
			cmzn_graphics_destroy(&copy_graphics);
		}
	}

	return return_object;
}

bool cmzn_graphicspointattributes_contain_surfaces(cmzn_graphicspointattributes_id point_attributes)
{
	bool return_code = false;
	if (point_attributes)
	{
		cmzn_graphics_id graphics = reinterpret_cast<cmzn_graphics *>(point_attributes);
		cmzn_glyph_id glyph = cmzn_graphicspointattributes_get_glyph(
			point_attributes);
		if (cmzn_glyph_contains_surface_primitives(glyph,
			graphics->tessellation, graphics->material, graphics->font))
		{
			return_code = true;
		}
		cmzn_glyph_destroy(&glyph);
	}
	return return_code;
}
