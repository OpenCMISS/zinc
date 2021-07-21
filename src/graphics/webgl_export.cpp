/***************************************************************************//**
 * FILE : webgl_export.cpp
 *
 * File containing a class for exporting webgl vertices and script for rendering.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "general/debug.h"
#include "graphics/webgl_export.hpp"
#include "graphics/graphics_object.h"
#include "graphics/graphics_object_private.hpp"
#include <iostream>
#include <string>

using namespace std;

Webgl_export::~Webgl_export()
{
	DEALLOCATE(filename);
}

int Webgl_export::beginExport()
{
	webgl_file=fopen(filename,"w");
	if (webgl_file)
		return 1;
	else
		return 0;
}

int Webgl_export::endExport()
{
	if (webgl_file)
	{
		fprintf(webgl_file, "\nfunction bind_zinc_objects_buffer()\n{\n");
		fprintf(webgl_file, "%s", bindBuffersString.c_str());
		fprintf(webgl_file, "}\n");
		fprintf(webgl_file, "\nfunction draw_zinc_objects()\n{\n");
		fprintf(webgl_file, "%s", drawFunctionsString.c_str());
		fprintf(webgl_file, "}\n");
		return (0 == fclose(webgl_file));
	}
	else
		return 0;
}

string *Webgl_export::writeVertexBuffer(const char *output_variable_name,
	GLfloat *vertex_buffer, unsigned int values_per_vertex,
	unsigned int vertex_count)
{
	if (vertex_buffer && (values_per_vertex > 0)  && (vertex_count > 0))
	{
		char temp[1000];
		string *buffer_string = new string();
		sprintf(temp, "\t%s_buffer = gl.createBuffer();\n",
			output_variable_name);
		*buffer_string += &temp[0];
		sprintf(temp, "\tgl.bindBuffer(gl.ARRAY_BUFFER, %s_buffer);\n",
			output_variable_name);
		*buffer_string += &temp[0];
		sprintf(temp, "\tvar %s_vertices = [\n", output_variable_name);
		*buffer_string += &temp[0];

		GLfloat *currentVertex = vertex_buffer;
		for (unsigned int i = 0; i < vertex_count; i++)
		{
			sprintf(temp, "\t\t%g", currentVertex[0]);
			*buffer_string += &temp[0];
			for (unsigned int k = 1; k < values_per_vertex; k++)
			{
				sprintf(temp, ", %g", currentVertex[k]);
				*buffer_string += &temp[0];
			}
			if (i != (vertex_count - 1))
			{
				*buffer_string += ",\n";
			}
			else
			{
				*buffer_string += "\n";
			}
			currentVertex+=values_per_vertex;
		}
		*buffer_string += "\t];\n";
		sprintf(temp,"\tgl.bufferData(gl.ARRAY_BUFFER, new Float32Array(%s_vertices), gl.STATIC_DRAW);\n",
			output_variable_name);
		*buffer_string += &temp[0];
		sprintf(temp,"\t%s_buffer.numItems = %d;\n", output_variable_name, vertex_count);
		*buffer_string += &temp[0];
		sprintf(temp,"\t%s_buffer.itemSize = %d;\n\n", output_variable_name, values_per_vertex);
		*buffer_string += &temp[0];
		return buffer_string;
	}
	return 0;
}

void Webgl_export::writeWebGLBindingFunction(std::string *position_string,
	string *colour_string, string *normal_string,
	string *tex_coord_string, string *index_string,
	const char *export_name)
{
	if (position_string)
	{
		fprintf(webgl_file, "\nfunction bind_%s_buffer()\n{\n", export_name);
		fprintf(webgl_file, "%s", position_string->c_str());
		if (colour_string)
			fprintf(webgl_file, "%s", colour_string->c_str());
		if (normal_string)
			fprintf(webgl_file, "%s", normal_string->c_str());
		if (tex_coord_string)
			fprintf(webgl_file, "%s", tex_coord_string->c_str());
		if (index_string)
			fprintf(webgl_file, "%s", index_string->c_str());
		fprintf(webgl_file, "%s", "}\n");
		bindBuffersString += "\tbind_";
		bindBuffersString += export_name;
		bindBuffersString += "_buffer();\n";
	}
}

void Webgl_export::writeWebGLDrawingFunction(string *position_string,
	string *colour_string, string *normal_string,
	string *tex_coord_string, string *index_string,
	const char *export_name)
{
	if (position_string)
	{
		char temp[100];
		fprintf(webgl_file, "\nfunction draw_%s()\n{\n", export_name);
		fprintf(webgl_file, "\tmat4.translate(mvMatrix, [0.0, 0.0, 0.0]);\n");
		fprintf(webgl_file, "\tmvPushMatrix();\n\n");
		sprintf(temp, "%s_position_buffer", export_name);
		fprintf(webgl_file, "\tgl.bindBuffer(gl.ARRAY_BUFFER, %s);\n", temp);
		fprintf(webgl_file, "\tgl.vertexAttribPointer(shaderProgram.vertexPositionAttribute, %s.itemSize, gl.FLOAT, false, 0, 0);\n\n",
			temp);
		sprintf(temp, "%s_colour_buffer", export_name);
		if (colour_string)
		{
			fprintf(webgl_file, "\tgl.bindBuffer(gl.ARRAY_BUFFER, %s);\n", temp);
			fprintf(webgl_file, "\tgl.vertexAttribPointer(shaderProgram.vertexColorAttribute, %s.itemSize, gl.FLOAT, false, 0, 0);\n\n",
				temp);
		}
		sprintf(temp, "%s_normal_buffer", export_name);
		if (normal_string)
		{
			fprintf(webgl_file, "\tgl.bindBuffer(gl.ARRAY_BUFFER, %s);\n", temp);
			fprintf(webgl_file, "\tgl.vertexAttribPointer(shaderProgram.vertexNormalAttribute, %s.itemSize, gl.FLOAT, false, 0, 0);\n\n",
				temp);
		}
		fprintf(webgl_file, "\tsetMatrixUniforms();\n");
		if (index_string)
		{
			sprintf(temp, "%s_index_buffer", export_name);
			fprintf(webgl_file, "\tgl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, %s);\n", temp);
			sprintf(temp, "%s_index_points_for_strip_array", export_name);
			fprintf(webgl_file, "\tvar i = 0;\n");
			fprintf(webgl_file, "\tfor (var k = 0; k < %s.length; k++)\n\t{\n", temp);
			fprintf(webgl_file, "\t\tgl.drawElements(gl.TRIANGLE_STRIP, %s[k], gl.UNSIGNED_SHORT, i * 2);\n", temp);
			fprintf(webgl_file, "\t\ti += %s[k];\n\t}\n", temp);
		}
		//if (tex_coord_string)
		//	fprintf(webgl_file, "%s", tex_coord_string->c_str());
		fprintf(webgl_file, "\tmvPopMatrix();\n");
		fprintf(webgl_file, "%s", "}\n");
		sprintf(temp, "\tdraw_%s();\n", export_name);
		drawFunctionsString += temp;
	}
}

string *Webgl_export::writeIndexBuffer(struct GT_object *object, const char *export_name)
{
	if (webgl_file && object)
	{
		char variable_name[100];

		sprintf(variable_name, "%s_index", export_name);
		unsigned int *strip_counts_buffer, per_vertex, counts_buffer_count,
			*strip_index_buffer, index_count, *points_per_strip_buffer,
			points_count;
		object->vertex_array->get_unsigned_integer_vertex_buffer(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_STRIPS,
				&strip_counts_buffer, &per_vertex, &counts_buffer_count);
		object->vertex_array->get_unsigned_integer_vertex_buffer(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_STRIP_INDEX_ARRAY,
				&strip_index_buffer, &per_vertex, &index_count);
		object->vertex_array->get_unsigned_integer_vertex_buffer(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_POINTS_FOR_STRIP,
				&points_per_strip_buffer, &per_vertex, &points_count);

		if (strip_counts_buffer && (counts_buffer_count > 0) &&
			strip_index_buffer && (index_count > 0 ) &&
			points_per_strip_buffer && (points_count > 0 ))
		{
			string *buffer_string = new string();
			fprintf(webgl_file,"var %s_buffer;\n", variable_name);
			fprintf(webgl_file,"var %s_number_of_strips_array;\n", variable_name);
			fprintf(webgl_file,"var %s_points_for_strip_array;\n", variable_name);

			char temp[1000];
			sprintf(temp, "\t%s_buffer = gl.createBuffer();\n",
				variable_name);
			*buffer_string += &temp[0];
			sprintf(temp, "\tgl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, %s_buffer);\n",
				variable_name);
			*buffer_string += &temp[0];
			sprintf(temp, "\tvar %s_vertices = [\n", variable_name);
			*buffer_string += &temp[0];
			for (unsigned int i = 0; i < index_count; i++)
			{
				if ((i % 10) == 0)
				{
					sprintf(temp, "\t\t");
					*buffer_string += &temp[0];
				}
				if ((index_count - 1) != i)
				{
					sprintf(temp, "%u, ", strip_index_buffer[i]);
					buffer_string->append(temp);
				}
				else
				{
					sprintf(temp, "%u", strip_index_buffer[i]);
					*buffer_string += &temp[0];
				}
				if ((i % 10) == 9)
				{
					*buffer_string += "\n";
				}
			}
			*buffer_string += "\t];\n";
			sprintf(temp,"\tgl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array(%s_vertices), gl.STATIC_DRAW);\n",
				variable_name);
			*buffer_string += &temp[0];
			sprintf(temp,"\t%s_buffer.itemSize = 1;\n", variable_name);
			*buffer_string += &temp[0];
			sprintf(temp,"\t%s_buffer.numItems = %d;\n\n", variable_name, index_count);
			*buffer_string += &temp[0];
			sprintf(temp, "\t%s_number_of_strips_array = [\n", variable_name);
			*buffer_string += &temp[0];
			for (unsigned int i = 0; i < counts_buffer_count; i++)
			{
				if ((i % 10) == 0)
				{
					sprintf(temp, "\t\t");
					*buffer_string += &temp[0];
				}
				if ((counts_buffer_count - 1) != i)
				{
					sprintf(temp, "%u, ", strip_counts_buffer[i]);
					buffer_string->append(temp);
				}
				else
				{
					sprintf(temp, "%u", strip_counts_buffer[i]);
					*buffer_string += &temp[0];
				}
				if ((i % 10) == 9)
				{
					*buffer_string += "\n";
				}
			}
			*buffer_string += "\t];\n";
			sprintf(temp, "\t%s_points_for_strip_array = [\n", variable_name);
			*buffer_string += &temp[0];
			for (unsigned int i = 0; i < points_count; i++)
			{
				if ((i % 10) == 0)
				{
					sprintf(temp, "\t\t");
					*buffer_string += &temp[0];
				}
				if ((points_count - 1) != i)
				{
					sprintf(temp, "%u, ", points_per_strip_buffer[i]);
					buffer_string->append(temp);
				}
				else
				{
					sprintf(temp, "%u", points_per_strip_buffer[i]);
					*buffer_string += &temp[0];
				}
				if ((i % 10) == 9)
				{
					*buffer_string += "\n";
				}
			}
			*buffer_string += "\t];\n";


			return buffer_string;
		}
	}
	return 0;
}


int Webgl_export::exportGraphicsObject(struct GT_object *object,	const char *export_name)
{
	if (webgl_file && object)
	{
		string *position_string = 0, *colour_string = 0, *normal_string = 0,
			*tex_coord_string = 0, *index_string = 0;

		switch (GT_object_get_type(object))
		{
		case g_POLYLINE_VERTEX_BUFFERS:
		case g_SURFACE_VERTEX_BUFFERS:
		case g_POINT_SET_VERTEX_BUFFERS:
		{
			char variable_name[100];
			GLfloat *position_vertex_buffer = NULL;
			unsigned int position_values_per_vertex, position_vertex_count;
			if (object->vertex_array->get_float_vertex_buffer(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
				&position_vertex_buffer, &position_values_per_vertex,
				&position_vertex_count))
			{
				sprintf(variable_name, "%s_position", export_name);
				position_string = writeVertexBuffer(variable_name,
					position_vertex_buffer, position_values_per_vertex,
					position_vertex_count);
				if (position_string)
					fprintf(webgl_file,"var %s_buffer;\n", variable_name);
			}

			unsigned int colour_values_per_vertex, colour_vertex_count;
			GLfloat *colour_buffer = (GLfloat *)NULL;
			if (Graphics_object_create_colour_buffer_from_data(object,
				&colour_buffer,
				&colour_values_per_vertex, &colour_vertex_count)
				&& (colour_vertex_count == position_vertex_count))
			{
				sprintf(variable_name, "%s_colour", export_name);
				colour_string = writeVertexBuffer(variable_name,
					colour_buffer, colour_values_per_vertex,
					colour_vertex_count);
				if (colour_buffer)
				{
					DEALLOCATE(colour_buffer);
				}
				if (colour_string)
					fprintf(webgl_file,"var %s_buffer;\n", variable_name);
			}

			GLfloat *normal_buffer = NULL;
			unsigned int normal_values_per_vertex, normal_vertex_count;
			if (object->vertex_array->get_float_vertex_buffer(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
				&normal_buffer, &normal_values_per_vertex, &normal_vertex_count)
				&& (3 == normal_values_per_vertex))
			{
				sprintf(variable_name, "%s_normal", export_name);
				normal_string = writeVertexBuffer(variable_name,
					normal_buffer, normal_values_per_vertex,
					normal_vertex_count);
				if (normal_string)
					fprintf(webgl_file,"var %s_buffer;\n", variable_name);
			}

			GLfloat *texture_coordinate0_buffer = NULL;
			unsigned int texture_coordinate0_values_per_vertex,
			texture_coordinate0_vertex_count;
			if (object->vertex_array->get_float_vertex_buffer(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_TEXTURE_COORDINATE_ZERO,
				&texture_coordinate0_buffer, &texture_coordinate0_values_per_vertex,
				&texture_coordinate0_vertex_count)
				&& (texture_coordinate0_vertex_count == position_vertex_count))
			{
				sprintf(variable_name, "%s_texture_coord0", export_name);
				tex_coord_string = writeVertexBuffer(variable_name,
					texture_coordinate0_buffer, texture_coordinate0_values_per_vertex,
					texture_coordinate0_vertex_count);
				if (tex_coord_string)
					fprintf(webgl_file,"var %s_buffer;\n", variable_name);
			}
			index_string = writeIndexBuffer(object, export_name);
		} break;
		default:
			break;
		}
		writeWebGLBindingFunction(position_string, colour_string, normal_string, tex_coord_string, index_string,
			export_name);
		writeWebGLDrawingFunction(position_string, colour_string, normal_string, tex_coord_string, index_string,
			export_name);
		if (position_string)
		{
			delete position_string;
		}
		if (colour_string)
		{
			delete colour_string;
		}
		if (normal_string)
		{
			delete normal_string;
		}
		if (tex_coord_string)
		{
			delete tex_coord_string;
		}
		if (index_string)
		{
			delete index_string;
		}
		return 1;
	}
	return 0;
}
