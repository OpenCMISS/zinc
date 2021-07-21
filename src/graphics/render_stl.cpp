/*******************************************************************************
FILE : renderstl.cpp

LAST MODIFIED : 8 July 2008

DESCRIPTION :
Renders gtObjects to STL stereolithography file.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stack>
#include <stdio.h>
#include "general/debug.h"
#include "general/matrix_vector.h"
#include "general/message.h"
#include "general/mystring.h"
#include "graphics/glyph.hpp"
#include "graphics/graphics_object.h"
#include "graphics/graphics_object_private.hpp"
#include "graphics/render_stl.h"
#include "graphics/scene.hpp"

namespace {

/*
Module types
------------
*/

class Transformation_matrix
{
private:
	double entry[4][4];

public:
	Transformation_matrix(const gtMatrix& gt_matrix)
	{
		for (int i=0; i<4; i++)
		{
			for (int j=0; j<4; j++)
			{
				entry[i][j] = (double)gt_matrix[j][i];
			}
		}
	}

	Transformation_matrix(
		double m11, double m12, double m13, double m14,
		double m21, double m22, double m23, double m24,
		double m31, double m32, double m33, double m34,
		double m41, double m42, double m43, double m44)
	{
		entry[0][0] = m11;
		entry[0][1] = m12;
		entry[0][2] = m13;
		entry[0][3] = m14;

		entry[1][0] = m21;
		entry[1][1] = m22;
		entry[1][2] = m23;
		entry[1][3] = m24;

		entry[2][0] = m31;
		entry[2][1] = m32;
		entry[2][2] = m33;
		entry[2][3] = m34;

		entry[3][0] = m41;
		entry[3][1] = m42;
		entry[3][2] = m43;
		entry[3][3] = m44;
	}

	void post_multiply(const Transformation_matrix& matrix)
	{
		Transformation_matrix prev = *this;
		for (int i=0; i<4; i++)
		{
			for (int j=0; j<4; j++)
			{
				entry[i][j] =
					prev.entry[i][0]*matrix.entry[0][j] +
					prev.entry[i][1]*matrix.entry[1][j] +
					prev.entry[i][2]*matrix.entry[2][j] +
					prev.entry[i][3]*matrix.entry[3][j];
			}
		}
	}

	void transform(double* tv) const
	{
		double x = tv[0];
		double y = tv[1];
		double z = tv[2];
		tv[0] = entry[0][0]*x + entry[0][1]*y + entry[0][2]*z + entry[0][3];
		tv[1] = entry[1][0]*x + entry[1][1]*y + entry[1][2]*z + entry[1][3];
		tv[2] = entry[2][0]*x + entry[2][1]*y + entry[2][2]*z + entry[2][3];
	}
};

class Stl_context
{
private:
	FILE *stl_file;
	char *solid_name;
	/* Future: add binary option */
	std::stack<Transformation_matrix> transformation_stack;

public:
	Stl_context(const char *file_name, const char *solid_name_in) :
		stl_file(fopen(file_name, "w"))
	{
		/* ASCII STL header */
		if (solid_name_in == 0)
			solid_name = duplicate_string("default");
		else
			solid_name = duplicate_string(solid_name_in);

		fprintf(stl_file,"solid %s\n",solid_name);
	}

	~Stl_context()
	{
		fprintf(stl_file,"endsolid %s\n",solid_name);
		DEALLOCATE(solid_name);
		fclose(stl_file);
	}

	/***************************************************************************//**
	 * Confirms STL file is correctly opened.
	 * 
	 * @return true if context is correctly constructed, false if not.
	 */
	bool is_valid() const
	{
		return (stl_file != (FILE *)NULL) && (solid_name != (char *)NULL);
	}

/***************************************************************************//**
 * Pushes another item onto the transformation stack equal to the previous top
 * item multiplied by the incoming matrix. 
 * 
 * @matrix the matrix to post multiply by
 */
	void push_multiply_transformation(const Transformation_matrix& matrix)
	{
		if (transformation_stack.empty())
		{
			transformation_stack.push(matrix);
		}
		else
		{
			transformation_stack.push(transformation_stack.top());
			transformation_stack.top().post_multiply(matrix);
		}
	}

	void pop_transformation(void)
	{
		if (transformation_stack.empty())
		{
			display_message(ERROR_MESSAGE,
				"Stl_context::pop_transformation.  Transformation stack is empty");
		}
		else
		{
			transformation_stack.pop();
		}
	}

	void transform(const Triple& v, double* tv) const
	{
		tv[0] = static_cast<double>(v[0]);
		tv[1] = static_cast<double>(v[1]);
		tv[2] = static_cast<double>(v[2]);
		if (!transformation_stack.empty())
		{
			transformation_stack.top().transform(tv);
		}
	}

	/***************************************************************************//**
	 * Writes a single STL triangle to file.
	 * 
	 * @param v1 coordinates of first vertex
	 * @param v2 coordinates of second vertex
	 * @param v3 coordinates of third vertex
	 */
	void write_triangle(
		const Triple& v1, const Triple& v2, const Triple& v3)
	{
		ZnReal tv1[3], tv2[3], tv3[3];
		transform(v1, tv1);
		transform(v2, tv2);
		transform(v3, tv3);

		ZnReal tangent1[3], tangent2[3], normal[3];
		tangent1[0] = tv2[0] - tv1[0];
		tangent1[1] = tv2[1] - tv1[1];
		tangent1[2] = tv2[2] - tv1[2];
		tangent2[0] = tv3[0] - tv1[0];
		tangent2[1] = tv3[1] - tv1[1];
		tangent2[2] = tv3[2] - tv1[2];
		cross_product3(tangent1, tangent2, normal);
		if (0.0 < normalize3(normal))
		{
			fprintf(stl_file, "facet normal %f %f %f\n", (ZnReal)normal[0], (ZnReal)normal[1], (ZnReal)normal[2]);
			fprintf(stl_file, " outer loop\n");		
			fprintf(stl_file, "  vertex %g %g %g\n", (ZnReal)tv1[0], (ZnReal)tv1[1], (ZnReal)tv1[2]);		
			fprintf(stl_file, "  vertex %g %g %g\n", (ZnReal)tv2[0], (ZnReal)tv2[1], (ZnReal)tv2[2]);
			fprintf(stl_file, "  vertex %g %g %g\n", (ZnReal)tv3[0], (ZnReal)tv3[1], (ZnReal)tv3[2]);
			fprintf(stl_file, " endloop\n");
		 	fprintf(stl_file, "endfacet\n");
		}
	} /* write_triangle_stl */

}; /* class Stl_context */

/*
Module functions
----------------
*/

int makestl(Stl_context& stl_context, gtObject *graphics_object, ZnReal time);


/***************************************************************************//**
 * Writes a glyph set to STL file.
 * Only surface glyphs can be output; other primitives are silently ignored.
 * Transformations are flattened.
 */
static int draw_glyph_set_stl(Stl_context& stl_context, gtObject *object)
{
	if (object && object->vertex_array && object->primitive_lists)
	{
		unsigned int nodeset_index = 0;
		unsigned int nodeset_count =
			object->vertex_array->get_number_of_vertices(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START);
		GT_glyphset_vertex_buffers *glyph_set = object->primitive_lists->gt_glyphset_vertex_buffers;
		GLfloat *position_buffer = 0, *axis1_buffer = 0, *axis2_buffer = 0,
			*axis3_buffer = 0, *scale_buffer = 0;
		unsigned int position_values_per_vertex = 0, position_vertex_count = 0,
			axis1_values_per_vertex = 0,
			axis1_vertex_count = 0, axis2_values_per_vertex = 0, axis2_vertex_count = 0,
			axis3_values_per_vertex = 0, axis3_vertex_count = 0, scale_values_per_vertex = 0,
			scale_vertex_count = 0;
		cmzn_glyph_repeat_mode glyph_repeat_mode = glyph_set->glyph_repeat_mode;
		GT_object *glyph = glyph_set->glyph;
		if (glyph)
		{
			if (0 < nodeset_count)
			{
				object->vertex_array->get_float_vertex_buffer(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
					&position_buffer, &position_values_per_vertex, &position_vertex_count);
				object->vertex_array->get_float_vertex_buffer(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_AXIS1,
					&axis1_buffer, &axis1_values_per_vertex, &axis1_vertex_count);
				object->vertex_array->get_float_vertex_buffer(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_AXIS2,
					&axis2_buffer, &axis2_values_per_vertex, &axis2_vertex_count);
				object->vertex_array->get_float_vertex_buffer(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_AXIS3,
					&axis3_buffer, &axis3_values_per_vertex, &axis3_vertex_count);
				object->vertex_array->get_float_vertex_buffer(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_SCALE,
					&scale_buffer, &scale_values_per_vertex, &scale_vertex_count);
			}

			for (nodeset_index = 0; nodeset_index < nodeset_count; nodeset_index++)
			{
				unsigned int index_start = 0, index_count = 0;
				object->vertex_array->get_unsigned_integer_attribute(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
					nodeset_index, 1, &index_start);
				object->vertex_array->get_unsigned_integer_attribute(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
					nodeset_index, 1, &index_count);
				GLfloat *position = position_buffer + position_values_per_vertex * index_start,
					*axis1 = axis1_buffer + axis1_values_per_vertex * index_start,
					*axis2 = axis2_buffer + axis2_values_per_vertex * index_start,
					*axis3 = axis3_buffer + axis3_values_per_vertex * index_start,
					*scale = scale_buffer + scale_values_per_vertex * index_start;
				const int number_of_glyphs =
					cmzn_glyph_repeat_mode_get_number_of_glyphs(glyph_repeat_mode);
				Triple temp_axis1, temp_axis2, temp_axis3, temp_point;
				for (unsigned int i = 0; i < index_count; i++)
				{
					for (int glyph_number = 0; glyph_number < number_of_glyphs; ++glyph_number)
					{
						resolve_glyph_axes(glyph_repeat_mode, glyph_number,
							glyph_set->base_size, glyph_set->scale_factors, glyph_set->offset,
							position, axis1, axis2, axis3, scale,
							temp_point, temp_axis1, temp_axis2, temp_axis3);
						Transformation_matrix matrix(
							(double)temp_axis1[0], (double)temp_axis2[0], (double)temp_axis3[0], (double)temp_point[0],
							(double)temp_axis1[1], (double)temp_axis2[1], (double)temp_axis3[1], (double)temp_point[1],
							(double)temp_axis1[2], (double)temp_axis2[2], (double)temp_axis3[2], (double)temp_point[2],
							0.0, 0.0, 0.0, 1.0);
						stl_context.push_multiply_transformation(matrix);
						struct GT_object *temp_glyph = glyph;
						/* call the glyph display lists of the linked-list of glyphs */
						while (temp_glyph)
						{
							makestl(stl_context, temp_glyph, 0);
							temp_glyph = GT_object_get_next_object(temp_glyph);
						}
						stl_context.pop_transformation();
					}
					position += position_values_per_vertex;
					axis1 += axis1_values_per_vertex;
					axis2 += axis2_values_per_vertex;
					axis3 += axis3_values_per_vertex;
					scale += scale_values_per_vertex;
				}
			}
		}
		return 1;
	}
	else
	{
		display_message(ERROR_MESSAGE, "draw_glyph_set_stl. Invalid argument(s)");
	}
	return 0;
}

/***************************************************************************//**
 * Writes triangle surface strips to STL file as
 * individual triangles.
 *
 */
int draw_surface_stl(Stl_context& stl_context, gtObject *object)
{
	if (object && object->primitive_lists)
	{
		GT_surface_vertex_buffers *vb_surface =
			object->primitive_lists->gt_surface_vertex_buffers;
		if (vb_surface)
		{
			unsigned int surface_count =
				object->vertex_array->get_number_of_vertices(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START);
			GLfloat *position_buffer = 0;
			unsigned int position_values_per_vertex = 0, position_vertex_count = 0;
			object->vertex_array->get_float_vertex_buffer(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
				&position_buffer, &position_values_per_vertex, &position_vertex_count);
			unsigned int *index_vertex_buffer, index_values_per_vertex, index_vertex_count;
			object->vertex_array->get_unsigned_integer_vertex_buffer(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_STRIP_INDEX_ARRAY,
				&index_vertex_buffer, &index_values_per_vertex,
				&index_vertex_count);
			for (unsigned int surface_index = 0; surface_index < surface_count; surface_index++)
			{
				int object_name = 0;
				if (!object->vertex_array->get_integer_attribute(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_OBJECT_ID,
					surface_index, 1, &object_name))
				{
					object_name = 0;
				}
				if (object_name > -1)
				{
					switch (vb_surface->surface_type)
					{
						case g_SHADED:
						case g_SHADED_TEXMAP:
						{
							unsigned int number_of_strips = 0;
							unsigned int strip_start = 0;
							object->vertex_array->get_unsigned_integer_attribute(
								GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_STRIPS,
								surface_index, 1, &number_of_strips);
							object->vertex_array->get_unsigned_integer_attribute(
								GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_STRIP_START,
								surface_index, 1, &strip_start);
							for (unsigned int i = 0; i < number_of_strips; i++)
							{
								unsigned int points_per_strip = 0;
								unsigned int index_start_for_strip = 0;
								object->vertex_array->get_unsigned_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_STRIP_INDEX_START,
									strip_start+i, 1, &index_start_for_strip);
								object->vertex_array->get_unsigned_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_POINTS_FOR_STRIP,
									strip_start+i, 1, &points_per_strip);
								unsigned int *indices = &index_vertex_buffer[index_start_for_strip];
								Triple point1 = {0.0, 0.0, 0.0}, point2 = {0.0, 0.0, 0.0},
									point3 = {0.0, 0.0, 0.0};
								for (unsigned int j =0; j<points_per_strip - 2; j++)
								{
									point1[2] = 0.0;
									point2[2] = 0.0;
									point3[2] = 0.0;
									if (0 == (j % 2))
									{
										for (unsigned int num = 0; num < position_values_per_vertex; num++)
										{
											point1[num] = position_buffer[indices[j]*position_values_per_vertex + num];
											point2[num] = position_buffer[indices[j + 1]*position_values_per_vertex + num];
											point3[num] = position_buffer[indices[j + 2]*position_values_per_vertex + num];
										}
									}
									else
									{
										for (unsigned int num = 0; num < position_values_per_vertex; num++)
										{
											point1[num] = position_buffer[indices[j + 1]*position_values_per_vertex + num];
											point2[num] = position_buffer[indices[j]*position_values_per_vertex + num];
											point3[num] = position_buffer[indices[j + 2]*position_values_per_vertex + num];
										}
									}
									stl_context.write_triangle(point1, point2, point3);
								}
							}
						} break;
						case g_SH_DISCONTINUOUS:
						case g_SH_DISCONTINUOUS_STRIP:
						case g_SH_DISCONTINUOUS_TEXMAP:
						case g_SH_DISCONTINUOUS_STRIP_TEXMAP:
						{
							unsigned int index_start, index_count;
							object->vertex_array->get_unsigned_integer_attribute(
								GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
								surface_index, 1, &index_start);
							object->vertex_array->get_unsigned_integer_attribute(
								GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
								surface_index, 1, &index_count);
							Triple point1 = {0.0, 0.0, 0.0}, point2 = {0.0, 0.0, 0.0},
								point3 = {0.0, 0.0, 0.0};
							for (unsigned int i = 0; i < index_count;)
							{
								point1[2] = 0.0;
								point2[2] = 0.0;
								point3[2] = 0.0;
								for (unsigned int num = 0; num < position_values_per_vertex; num++)
								{
									point1[num] = position_buffer[(index_start+i)*position_values_per_vertex + num];
									point2[num] = position_buffer[(index_start+i+1)*position_values_per_vertex + num];
									point3[num] = position_buffer[(index_start+i+2)*position_values_per_vertex + num];
								}
								stl_context.write_triangle(point1, point2, point3);
								i += 3;
							}
						}break;
						default:
						{
						} break;
					}
				}
			}
		}
		return 1;
	}

	return 0;
}

/***************************************************************************//**
 * Writes surface primitives in graphics object to STL file.
 * 
 * @param stl_context output STL file and context data
 * @param object graphics object to output
 * @param time time at which graphics are output
 * @return 1 on success, 0 on failure
 */
int makestl(Stl_context& stl_context, gtObject *object, ZnReal time)
{
	int return_code = 0;

	return_code = 1;
	if (object)
	{
		if (return_code)
		{
			switch (object->object_type)
			{
				case g_GLYPH_SET_VERTEX_BUFFERS:
				{
					if (object->vertex_array)
					{
						draw_glyph_set_stl(stl_context,	object);
					}
				} break;
				case g_POINT_SET_VERTEX_BUFFERS:
				case g_POLYLINE_VERTEX_BUFFERS:
				{
					/* not relevant to STL: ignore */
					return_code=1;
				} break;
				case g_SURFACE_VERTEX_BUFFERS:
				{
					if (object->vertex_array)
					{
						draw_surface_stl(stl_context,	object);
					}
					return_code=1;
				} break;
				default:
				{
					display_message(ERROR_MESSAGE,"makestl.  Invalid object type");
					return_code=0;
				} break;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"makestl.  Missing object");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* makestl */

int write_scene_stl(Stl_context& stl_context, cmzn_scene_id scene,
	cmzn_scenefilter_id filter);

/**************************************************************************//**
 * Renders the visible parts of a scene object to STL.
 * 
 * @param stl_context output STL file and context data
 * @param graphics_object the graphics object to output
 * @return 1 on success, 0 on failure
 */
int write_graphics_object_stl(Stl_context& stl_context,
	struct GT_object *graphics_object, double time)
{
	int return_code;

	ENTER(write_graphics_object_stl)
	if (graphics_object)
	{
		return_code = makestl(stl_context,graphics_object, time);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"write_graphics_object_stl.  Missing graphics_object");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* write_graphics_object_stl */

int Graphcis_object_to_stl(struct GT_object *graphics_object, double time,
	void *stl_context_void)
{
	int return_code;

	ENTER(Graphcis_object_to_stl);
	if (graphics_object && stl_context_void)
	{
		return_code = 1;
		Stl_context& stl_context = *(static_cast<Stl_context*>(stl_context_void));
		return_code = write_graphics_object_stl(stl_context, graphics_object, time);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphcis_object_to_stl.  Invalid argument(s)");
		return_code=0;
	}

	LEAVE;

	return (return_code);
} /* Graphcis_object_to_stl */

/**
 * Renders the visible objects in a scene to STL.
 * 
 * @param stl_context output STL file and context data
 * @param scene the scene to output
 * @param filter the filter to filter scenes
 * @return 1 on success, 0 on failure
 */
int write_scene_stl(Stl_context& stl_context, cmzn_scene_id scene,
	cmzn_scenefilter_id filter)
{
	int return_code;
	
	if (scene)
	{
		return_code=for_each_graphics_object_in_scene_tree(scene, filter,
			Graphcis_object_to_stl,(void *)&stl_context);
	}
	else
	{
		display_message(ERROR_MESSAGE,"write_scene_stl.  Missing scene");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* write_scene_stl */

} // anonymous namespace

/*
Global functions
----------------
*/

int export_to_stl(char *file_name, cmzn_scene_id scene, cmzn_scenefilter_id filter)
{
	int return_code;

	if (file_name && scene)
	{
		build_Scene(scene, filter);
		char *solid_name = cmzn_region_get_name(cmzn_scene_get_region_internal(scene));
		Stl_context stl_context(file_name, solid_name);
		if (stl_context.is_valid())
		{
			return_code = write_scene_stl(stl_context, scene, filter);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"export_to_stl.  Could not open stl file %s", file_name);
			return_code = 0;
		}
		DEALLOCATE(solid_name);
	}
	else
	{
		display_message(ERROR_MESSAGE,"export_to_stl.  Invalid argument(s)");
		return_code = 0;
	}

	return( return_code);
} /* export_to_stl */
