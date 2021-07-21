/***************************************************************************//**
 * render_triangularisation.cpp
 * Rendering calls - Non API specific.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "opencmiss/zinc/scene.h"
#include "general/debug.h"
#include "graphics/auxiliary_graphics_types.h"
#include "graphics/scene.hpp"
#include "graphics/graphics_object.h"
#include "graphics/triangle_mesh.hpp"
#include "graphics/graphics_object_private.hpp"
#include "graphics/render_triangularisation.hpp"

#include <math.h>

int write_scene_triangle_mesh(Triangle_mesh& trimesh, struct Scene *scene);

int draw_surface_triangle_mesh(Triangle_mesh& trimesh,
	Graphics_vertex_array *vertex_array)
{
	int return_code = 0;

	if (vertex_array)
	{
		unsigned int surface_index;
		unsigned int surface_count =
			vertex_array->get_number_of_vertices(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START);
		GLfloat *position_buffer = 0;
		unsigned int position_values_per_vertex, position_vertex_count;

		if (0 == surface_count)
			return 1;

		vertex_array->get_float_vertex_buffer(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
			&position_buffer, &position_values_per_vertex, &position_vertex_count);

		GLfloat *current_buffer = position_buffer;
		Triple point;
		point[0] = 0.0;
		point[1] = 0.0;
		point[2] = 0.0;

		const Triangle_vertex **tri_vertex = NULL;
		tri_vertex = new const Triangle_vertex*[position_vertex_count];
		for (unsigned int i = 0; i < position_vertex_count; i++)
		{
			for (unsigned int j = 0; j < position_values_per_vertex; j++)
			{
				point[j] = *current_buffer;
				current_buffer++;
			}
			tri_vertex[i] = trimesh.add_vertex(point);
		}

		unsigned int *index_vertex_buffer = 0, index_values_per_vertex = 0, index_vertex_count = 0;
		vertex_array->get_unsigned_integer_vertex_buffer(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_STRIP_INDEX_ARRAY,
			&index_vertex_buffer, &index_values_per_vertex,
			&index_vertex_count);

		for (surface_index = 0; surface_index < surface_count; surface_index++)
		{
			int object_name = 0;
			vertex_array->get_integer_attribute(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_OBJECT_ID,
				surface_index, 1, &object_name);
			if (object_name > -1)
			{
				if (index_vertex_buffer)
				{
					unsigned int number_of_strips = 0;
					unsigned int strip_start = 0;
					vertex_array->get_unsigned_integer_attribute(
						GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_STRIPS,
						surface_index, 1, &number_of_strips);
					vertex_array->get_unsigned_integer_attribute(
						GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_STRIP_START,
						surface_index, 1, &strip_start);
					for (unsigned int i = 0; i < number_of_strips; i++)
					{
						unsigned int points_per_strip = 0;
						unsigned int index_start_for_strip = 0;
						vertex_array->get_unsigned_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_STRIP_INDEX_START,
							strip_start+i, 1, &index_start_for_strip);
						vertex_array->get_unsigned_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_POINTS_FOR_STRIP,
							strip_start+i, 1, &points_per_strip);
						unsigned int *indices = &index_vertex_buffer[index_start_for_strip];
						for (unsigned int j = 0; j < points_per_strip - 2; j++)
						{
							if (0 == (j % 2))
							{
								trimesh.add_triangle(tri_vertex[indices[j]],
									tri_vertex[indices[j+1]],
									tri_vertex[indices[j+2]]);
							}
							else
							{
								trimesh.add_triangle(tri_vertex[indices[j+1]],
									tri_vertex[indices[j]],
									tri_vertex[indices[j+2]]);
							}
						}
					}
				}
				else
				{
					unsigned int index_start, index_count;
					vertex_array->get_unsigned_integer_attribute(
						GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
						surface_index, 1, &index_start);
					vertex_array->get_unsigned_integer_attribute(
						GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
						surface_index, 1, &index_count);
					for (unsigned int i = 0; i < index_count;)
					{
						trimesh.add_triangle(tri_vertex[index_start+i],
							tri_vertex[index_start+i+1],
							tri_vertex[index_start+i+2]);
						i += 3;
					}
				}
			}
		}
		delete[] tri_vertex;
		return_code = 1;
	}

	return (return_code);
} /* draw_surface_stl */

/***************************************************************************//**
 * Writes surface primitives in graphics object to STL file.
 * 
 * @param trimesh output STL file and context data
 * @param object graphics object to output
 * @return 1 on success, 0 on failure
 */
int maketriangle_mesh(Triangle_mesh& trimesh, gtObject *object)
{
	int return_code = 0;

	if (object && object->primitive_lists)
	{
		switch (object->object_type)
		{
			case g_SURFACE_VERTEX_BUFFERS:
			{
				struct GT_surface_vertex_buffers *surface =
					object->primitive_lists->gt_surface_vertex_buffers;
				if (0 != surface)
				{
					draw_surface_triangle_mesh(trimesh,
						object->vertex_array);
					return_code=1;
				}
				else
				{
					display_message(ERROR_MESSAGE,"maketriangle_mesh.  Missing surface");
					return_code=0;
				}
			} break;
			default:
			{
				return_code=1;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"maketriangle_mesh.  Missing object");
	}

	return (return_code);
}

static int graphics_object_export_to_triangularisation(struct GT_object *gt_object,
	double time,void *trimesh_void)
{
	int return_code = 0;
	USE_PARAMETER(time);

	if (trimesh_void)
	{
		Triangle_mesh& trimesh = *(static_cast<Triangle_mesh*>(trimesh_void));	
		return_code = maketriangle_mesh(trimesh, gt_object);
	}

	return return_code;
}

int render_scene_triangularisation(cmzn_scene_id scene,
	cmzn_scenefilter_id filter, Triangle_mesh *trimesh)
{
	int return_code = 0;

	return_code = for_each_graphics_object_in_scene_tree(scene, filter,
		graphics_object_export_to_triangularisation,(void *)trimesh);

	return return_code;
}

Render_graphics_triangularisation::~Render_graphics_triangularisation()
{
	delete trimesh;
}

int Render_graphics_triangularisation::Scene_tree_execute(cmzn_scene_id scene)
{
	set_Scene(scene);
	return render_scene_triangularisation(scene, this->getScenefilter(), trimesh);
}

Triangle_mesh *Render_graphics_triangularisation::get_triangle_mesh()
{
	return trimesh;
}

