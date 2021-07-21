/***************************************************************************//**
 * FILE : threejs_export.cpp
 *
 * File containing a class for exporting threejs vertices and script for rendering.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "general/debug.h"
#include "opencmiss/zinc/material.h"
#include "graphics/threejs_export.hpp"
#include "graphics/glyph.hpp"
#include "graphics/graphics_object.h"
#include "graphics/graphics_object_private.hpp"
#include "graphics/material.h"
#include "graphics/texture.h"
#include <iostream>
#include <string>
#include <math.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include "jsoncpp/json.h"

using namespace std;

enum ThreejsType
{
	THREEJS_TYPE_TRIANGLE = 0,
	THREEJS_TYPE_MATERIAL = 2,
	THREEJS_TYPE_VERTEX_TEX_COORD = 8,
	THREEJS_TYPE_VERTEX_NORMAL = 32,
	THREEJS_TYPE_FACE_COLOR = 64,
	THREEJS_TYPE_VERTEX_COLOR = 128
};

int rgb_to_hex(float r, float g, float b)
{
	int red = (r * 255) + 0.5, green = (g * 255) + 0.5, blue = (b * 255) + 0.5;
	int hex = 0;
	int remainder = 0, quotient = 0;
	if (red > 0)
	{
		quotient = red / 16;
		remainder = red % 16;
		hex += pow((double)16, 5) * quotient + pow((double)16, 4) * remainder;
	}
	if (green > 0)
	{
		quotient = green / 16;
		remainder = green % 16;
		hex += pow((double)16, 3) * quotient + pow((double)16, 2) * remainder;
	}
	if (blue > 0)
	{
		quotient = blue / 16;
		remainder = blue % 16;
		hex += pow((double)16, 1) * quotient + remainder;
	}
	return hex;
}

Threejs_export::~Threejs_export()
{
	if (groupName)
		DEALLOCATE(groupName);
	DEALLOCATE(filename);
}

int Threejs_export::beginExport()
{
	outputString += "{\n\t\"metadata\" : {\n\t\t\"formatVersion\" : 3,\n";
	outputString += "\t\t\"description\" : \"Exported from LibZinc.\"\n\t},\n\n";
	return 1;
}

int Threejs_export::endExport()
{
	outputString += verticesMorphString;
	outputString += colorsMorphString;
	outputString += normalMorphString;
	outputString += facesString;
	outputString += "}\n";
	return 1;
}

Json::Value Threejs_export::getExportJson()
{
	Json::Value root;
	Json::Reader reader;
	reader.parse( outputString, root );
	return root;
}

std::string *Threejs_export::getExportString()
{
	return &outputString;
}

void Threejs_export::writeIntegerBuffer(const char *output_variable_name,
	int *vertex_buffer, unsigned int values_per_vertex,
	unsigned int vertex_count)
{
	if (vertex_buffer && (values_per_vertex > 0)  && (vertex_count > 0))
	{
		char temp[200];

		unsigned int number_of_valid_output =  values_per_vertex;
		if (number_of_valid_output > 3)
			number_of_valid_output = 3;
		int *currentVertex = vertex_buffer;
		sprintf(temp, "\t\"%s\" : [", output_variable_name);
		outputString += temp;

		for (unsigned int i = 0; i < vertex_count; i++)
		{
			if (((i % 10) == 0))
				outputString += "\n\t\t";
			for (unsigned int k = 0; k < number_of_valid_output; k++)
			{
				sprintf(temp, "%d", currentVertex[k]);
				outputString += temp;
				if (0 == ((k == number_of_valid_output - 1) && (i == vertex_count - 1)))
				{
					outputString += ",";
				}
			}
			currentVertex+=values_per_vertex;
		}
		outputString += "\n\t],\n\n";
	}
}

/* this write out colour buffer at different time step */
void Threejs_export::writeMorphIntegerBuffer(const char *output_variable_name,
	std::string *output, int *vertex_buffer, unsigned int values_per_vertex,
	unsigned int vertex_count, int time_step)
{
	if (vertex_buffer && (values_per_vertex > 0)  && (vertex_count > 0) &&
		(vertex_count > 0) && output)
	{
		if (time_step == 0)
		{
			(*output) += "\t\"morphColors\": [";
		}
		unsigned int number_of_valid_output =  values_per_vertex;
		if (number_of_valid_output > 3)
			number_of_valid_output = 3;
		int *currentVertex = vertex_buffer;
		char temp[300];
		sprintf(temp, "\t{ \"name\": \"%s_color_%03d\", \"%s\": [", filename, time_step, output_variable_name);
		(*output) += temp;
		for (unsigned int i = 0; i < vertex_count; i++)
		{
			if (((i % 10) == 0))
			{
				sprintf(temp,"\n\t\t");
				(*output) += temp;
			}
			for (unsigned int k = 0; k < number_of_valid_output; k++)
			{
				sprintf(temp, "%d", currentVertex[k]);
				(*output) += temp;
				if (0 == ((k == number_of_valid_output - 1) && (i == vertex_count - 1)))
				{
					(*output) += ",";
				}
			}
			currentVertex+=values_per_vertex;
		}
		if (number_of_time_steps - 1 > time_step)
		{
			(*output) += "] },\n";
		}
		else
		{
			(*output) += "] }\n\t],\n\n";
		}
	}
}

void Threejs_export::writeVertexBuffer(const char *output_variable_name,
	GLfloat *vertex_buffer, unsigned int values_per_vertex,
	unsigned int vertex_count)
{
	if (vertex_buffer && (values_per_vertex > 0)  && (vertex_count > 0))
	{
		unsigned int number_of_valid_output = 3;
		GLfloat *currentVertex = vertex_buffer;
		char new_string[100];
		sprintf(new_string, "\t\"%s\" : [", output_variable_name);
		outputString += new_string;
		for (unsigned int i = 0; i < vertex_count; i++)
		{
			if ((i % 10) == 0)
				outputString += "\n\t\t";
			for (unsigned int k = 0; k < number_of_valid_output; k++)
			{
				if ((number_of_valid_output > values_per_vertex) &&
					(k >= values_per_vertex))
					outputString += "0.0";
				else
				{
					sprintf(new_string, "%f", currentVertex[k]);
					outputString += new_string;
				}
				if (0 == ((k == number_of_valid_output - 1) && (i == vertex_count - 1)))
				{
					outputString += ",";
				}
			}
			currentVertex+=values_per_vertex;
		}
		outputString += "\n\t],\n\n";
	}
}

/* this write out vertex buffer at different time step */
void Threejs_export::writeMorphVertexBuffer(const char *output_variable_name,
	std::string *output,	GLfloat *vertex_buffer, unsigned int values_per_vertex,
	unsigned int vertex_count, int time_step)
{
	if (vertex_buffer && (values_per_vertex > 0) &&
		(vertex_count > 0) && output)
	{
		if (time_step == 0)
		{
			if (!strcmp("vertices", output_variable_name))
			{
				(*output) += "\t\"morphTargets\": [";
			}
			else
			{
				(*output) += "\t\"morphNormals\": [";
			}
		}
		unsigned int number_of_valid_output =  3;
		GLfloat *currentVertex = vertex_buffer;

		char temp[300];
		sprintf(temp, "\t{ \"name\": \"%s_%03d\", \"%s\": [", filename, time_step, output_variable_name);
		(*output) += temp;
		for (unsigned int i = 0; i < vertex_count; i++)
		{
			if (((i % 10) == 0))
			{
				sprintf(temp,"\n\t\t");
				(*output) += temp;
			}
			for (unsigned int k = 0; k < number_of_valid_output; k++)
			{
				if ((number_of_valid_output > values_per_vertex) &&
					(k >= values_per_vertex))
					sprintf(temp, "0.0");
				else
					sprintf(temp, "%f", currentVertex[k]);
				(*output) += temp;
				if (0 == ((k == number_of_valid_output - 1) && (i == vertex_count - 1)))
				{
					(*output) += ",";
				}
			}
			currentVertex+=values_per_vertex;
		}
		if (number_of_time_steps - 1 > time_step)
		{
			(*output) += "] },\n";
		}
		else
		{
			(*output) += "] }\n\t],\n\n";
		}
	}
}

/*
 * Export the material used by the graphics, including the name of the texture used.
 */
void Threejs_export::exportMaterial(cmzn_material_id material)
{
	if (material)
	{
		char new_string[1024];
		sprintf(new_string, "\t\"materials\" : [ {\n");
		outputString += new_string;
		sprintf(new_string, "\t\"DbgColor\" : 15658734,\n");
		outputString += new_string;
		sprintf(new_string, "\t\"DbgIndex\" : 0,\n");
		outputString += new_string;
		sprintf(new_string, "\t\"DbgName\" : \"my_material\",\n");
		outputString += new_string;
		double values[3];
		cmzn_material_get_attribute_real3(material,
			CMZN_MATERIAL_ATTRIBUTE_DIFFUSE, &values[0]);
		sprintf(new_string, "\t\"colorDiffuse\" : [%g, %g, %g],\n", values[0], values[1], values[2]);
		outputString += new_string;
		cmzn_material_get_attribute_real3(material,
			CMZN_MATERIAL_ATTRIBUTE_SPECULAR, &values[0]);
		sprintf(new_string, "\t\"colorSpecular\" : [%g, %g, %g],\n", values[0], values[1], values[2]);
		outputString += new_string;

		struct Texture *texture = Graphical_material_get_texture(material);
		if (texture)
		{
			/* non-accessed */
			char *textureName = Texture_get_image_file_name(texture);
			if (!textureName)
				textureName = "my_texture.png";
			sprintf(new_string, "\t\"mapDiffuse\" : \"%s\",\n", textureName);
			outputString += new_string;
			enum Texture_wrap_mode mode = Texture_get_wrap_mode(texture);
			if (mode == TEXTURE_MIRRORED_REPEAT_WRAP)
			{
				sprintf(new_string, "\t\"mapDiffuseWrap\" : [\"mirror\", \"mirror\"],\n");
			}
			else
			{
				sprintf(new_string, "\t\"mapDiffuseWrap\" : [\"repeat\", \"repeat\"],\n");
			}
			outputString += new_string;
		}
		texture = Graphical_material_get_second_texture(material);
		if (texture)
		{
			/* non-accessed */
			char *textureName = Texture_get_image_file_name(texture);
			if (!textureName)
				textureName = "normal_texture.png";
			sprintf(new_string, "\t\"mapNormal\" : \"%s\",\n", textureName);
			outputString += new_string;
			enum Texture_wrap_mode mode = Texture_get_wrap_mode(texture);
			if (mode == TEXTURE_MIRRORED_REPEAT_WRAP)
			{
				sprintf(new_string, "\t\"mapNormalWrap\" : [\"mirror\", \"mirror\"],\n");
			}
			else
			{
				sprintf(new_string, "\t\"mapNormalWrap\" : [\"repeat\", \"repeat\"],\n");
			}
			outputString += new_string;
		}
		sprintf(new_string, "\t\"shading\" : \"Phong\",\n");
		outputString += new_string;
		double shininess = cmzn_material_get_attribute_real(material,
			CMZN_MATERIAL_ATTRIBUTE_SHININESS);
		sprintf(new_string, "\t\"specularCoef\" : %d,\n", (int)(shininess * 100));
		outputString += new_string;
		double alpha = cmzn_material_get_attribute_real(material, CMZN_MATERIAL_ATTRIBUTE_ALPHA);
		sprintf(new_string, "\t\"opacity\" : %g,\n", alpha);
		outputString += new_string;
		sprintf(new_string, "\t\"vertexColors\" : true\n");
		outputString += new_string;
		sprintf(new_string, "\t}],\n\n");
		outputString += new_string;
	}
}

/* write index for triangle surfaces (non triangle-stripe). */
void Threejs_export::writeIndexBufferWithoutIndex(int typeMask, int number_of_points,
	unsigned int offset)
{
	if (number_of_points)
	{
		char temp[100];
		facesString += "\t\"faces\": [\n";
		unsigned int number_of_triangles = number_of_points / 3;
		int current_index = 0;
		for (unsigned i = 0; i < number_of_triangles; i++)
		{
			sprintf(temp,"\t\t%d", typeMask);
			facesString += temp;
			sprintf(temp," ,%d,%d,%d", current_index+offset, current_index+offset+1, current_index+offset+2);
			facesString += temp;
			if (typeMask & THREEJS_TYPE_VERTEX_TEX_COORD)
			{
				sprintf(temp," ,%d,%d,%d", current_index+offset, current_index+offset+1, current_index+offset+2);
				facesString += temp;
			}
			if (typeMask & THREEJS_TYPE_VERTEX_NORMAL)
			{
				sprintf(temp," ,%d,%d,%d", current_index+offset, current_index+offset+1, current_index+offset+2);
				facesString += temp;
			}
			if (typeMask & THREEJS_TYPE_VERTEX_COLOR)
			{
				sprintf(temp," ,%d,%d,%d", current_index+offset, current_index+offset+1, current_index+offset+2);
				facesString += temp;
			}
			current_index += 3;
			if (i != number_of_triangles - 1)
			{
				facesString += ",";
			}
			facesString += "\n";
		}
		facesString += "\t]\n\n";
	}
}

void Threejs_export::writeIndexBuffer(struct GT_object *object, int typeMask, int number_of_points,
	unsigned int offset)
{
	if (object)
	{
		unsigned int *index_vertex_buffer = 0, index_values_per_vertex = 0, index_vertex_count = 0;
		object->vertex_array->get_unsigned_integer_vertex_buffer(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_STRIP_INDEX_ARRAY,
			&index_vertex_buffer, &index_values_per_vertex,
			&index_vertex_count);
		char temp[100];
		unsigned int face_colour_index = 0;
		if (index_vertex_buffer)
		{
			facesString += "\t\"faces\": [\n";
			unsigned int *indices = index_vertex_buffer;
			int current_index = 0;
			unsigned int *number_buffer = 0, number_per_vertex = 0, number_count = 0;
			object->vertex_array->get_unsigned_integer_vertex_buffer(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_POINTS_FOR_STRIP,
				&number_buffer, &number_per_vertex, &number_count);
			unsigned int points_per_strip = 0;
			for (unsigned i = 0; i < number_count; i ++)
			{
				points_per_strip = number_buffer[i];
				for (unsigned int j =0; j< points_per_strip - 2; j++)
				{
					sprintf(temp,"\t\t%d", typeMask);
					facesString += temp;
					if (0 == (j % 2))
					{
						sprintf(temp," ,%d,%d,%d",
							indices[current_index+j]+offset, indices[current_index+j+1]+offset,
							indices[current_index+j+2]+offset);
						facesString += temp;
						if (typeMask & THREEJS_TYPE_VERTEX_TEX_COORD)
						{
							sprintf(temp," ,%d,%d,%d",
								indices[current_index+j]+offset, indices[current_index+j+1]+offset,
								indices[current_index+j+2]+offset);
							facesString += temp;
						}
						if (typeMask & THREEJS_TYPE_VERTEX_NORMAL)
						{
							sprintf(temp," ,%d,%d,%d",
								indices[current_index+j]+offset, indices[current_index+j+1]+offset,
								indices[current_index+j+2]+offset);
							facesString += temp;
						}
						if (typeMask & THREEJS_TYPE_FACE_COLOR)
						{
							sprintf(temp," ,%d",	face_colour_index);
							facesString += temp;
							face_colour_index++;
						}
						if (typeMask & THREEJS_TYPE_VERTEX_COLOR)
						{
							sprintf(temp," ,%d,%d,%d",
								indices[current_index+j]+offset, indices[current_index+j+1]+offset,
								indices[current_index+j+2]+offset);
							facesString += temp;
						}
					}
					else
					{
						sprintf(temp," ,%d,%d,%d",
							indices[current_index+j+1]+offset, indices[current_index+j]+offset,
							indices[current_index+j+2]+offset);
						facesString += temp;
						if (typeMask & THREEJS_TYPE_VERTEX_TEX_COORD)
						{
							sprintf(temp," ,%d,%d,%d",
								indices[current_index+j+1]+offset, indices[current_index+j]+offset,
								indices[current_index+j+2]+offset);
							facesString += temp;
						}
						if (typeMask & THREEJS_TYPE_VERTEX_NORMAL)
						{
							sprintf(temp," ,%d,%d,%d",
								indices[current_index+j+1]+offset, indices[current_index+j]+offset,
								indices[current_index+j+2]+offset);
							facesString += temp;
						}
						if (typeMask & THREEJS_TYPE_FACE_COLOR)
						{
							sprintf(temp," ,%d",	face_colour_index);
							facesString += temp;
							face_colour_index++;
						}
						if (typeMask & THREEJS_TYPE_VERTEX_COLOR)
						{
							sprintf(temp," ,%d,%d,%d",
								indices[current_index+j+1]+offset, indices[current_index+j]+offset,
								indices[current_index+j+2]+offset);
							facesString += temp;
						}
					}
					if (!((i == number_count - 1) && (j == points_per_strip - 3)))
					{
						facesString += ",";
					}
					facesString += "\n";
				}
				current_index += points_per_strip;
			}
			facesString += "\t]\n\n";
		}
		else
		{
			writeIndexBufferWithoutIndex(typeMask, number_of_points, offset);
		}
	}
}

void Threejs_export::writeSpecialDataBuffer(struct GT_object *object, GLfloat *vertex_buffer,
	unsigned int values_per_vertex, unsigned int vertex_count)
{
	char num_string[100];

	if (vertex_buffer && (values_per_vertex > 0)  && (vertex_count > 0))
	{
		outputString += "\t\"colors\" : [";
		if (mode == CMZN_STREAMINFORMATION_SCENE_IO_DATA_TYPE_PER_VERTEX_VALUE)
		{
			GLfloat *currentVertex = vertex_buffer;
			for (unsigned int i = 0; i < vertex_count; i++)
			{
				if (((i % 10) == 0))
					outputString += "\n\t\t";
				for (unsigned int k = 0; k < values_per_vertex; k++)
				{
					sprintf(num_string, "%f", currentVertex[k]);
					outputString += num_string;
					if (0 == ((k == values_per_vertex - 1) && (i == vertex_count - 1)))
					{
						outputString += ",";
					}
				}
				currentVertex+=values_per_vertex;
			}
		}
		else
		{
			unsigned int *index_vertex_buffer = 0, index_values_per_vertex = 0, index_vertex_count = 0;
			object->vertex_array->get_unsigned_integer_vertex_buffer(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_STRIP_INDEX_ARRAY,
				&index_vertex_buffer, &index_values_per_vertex,
				&index_vertex_count);
			if (index_vertex_buffer)
			{
				unsigned int *indices = index_vertex_buffer;
				int current_index = 0;
				unsigned int *number_buffer = 0, number_per_vertex = 0, number_count = 0;
				object->vertex_array->get_unsigned_integer_vertex_buffer(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_POINTS_FOR_STRIP,
					&number_buffer, &number_per_vertex, &number_count);
				unsigned int points_per_strip = 0;
				unsigned int index[3];
				for (unsigned i = 0; i < number_count; i ++)
				{
					outputString += "\n\t\t";
					points_per_strip = number_buffer[i];
					for (unsigned int j =0; j< points_per_strip - 2; j++)
					{
						if (0 == (j % 2))
						{
							index[0] = indices[current_index+j];
							index[1] = indices[current_index+j+1];
							index[2] = indices[current_index+j+2];
						}
						else
						{
							index[0] = indices[current_index+j+1];
							index[1] = indices[current_index+j];
							index[2] = indices[current_index+j+2];
						}
						GLfloat *currentVertex = vertex_buffer;
						for (unsigned int k = 0; k < values_per_vertex; k++)
						{
							GLfloat average = (currentVertex[index[0] * values_per_vertex + k] +
								currentVertex[index[1] * values_per_vertex + k] +
								currentVertex[index[2] * values_per_vertex + k]) / 3;
							sprintf(num_string, "%f", average);
							outputString += num_string;
							if (!((i == number_count - 1) && (k == values_per_vertex - 1) &&
								 (j == points_per_strip - 3)))
							{
								outputString += ",";
							}
						}
					}
					current_index += points_per_strip;
				}

			}
			else
			{
				/* face colors, get the average vertex colors */
				GLfloat *currentVertex = vertex_buffer;
				for (unsigned int i = 0; i < vertex_count; i += 3)
				{
					if (((i % 10) == 0))
						outputString += "\n\t\t";
					for (unsigned int k = 0; k < values_per_vertex; k++)
					{
						GLfloat average = (currentVertex[k] + currentVertex[k + values_per_vertex] +
							currentVertex[k + values_per_vertex * 2]) / 3;
						char num_string[100];
						sprintf(num_string, "%f", average);
						outputString += num_string;
						if (0 == ((k == values_per_vertex - 1) && (i == vertex_count - 1)))
						{
							outputString += ",";
						}
					}
					currentVertex+=values_per_vertex * 3;
				}
			}
		}
		outputString += "\n\t],\n\n";
	}
}

void Threejs_export::writeUVsBuffer(GLfloat *texture_buffer, unsigned int values_per_vertex,
	unsigned int vertex_count)
{
	if (texture_buffer && (values_per_vertex > 0)  && (vertex_count > 0))
	{
		unsigned int valid_output_per_vertex = 2;
		GLfloat *currentVertex = texture_buffer;
		char new_string[100];
		sprintf(new_string, "\t\"uvs\" : [[");
		outputString += new_string;
		for (unsigned int i = 0; i < vertex_count; i++)
		{
			if ((i % 10) == 0)
				outputString += "\n\t\t";
			if (textureSizes[0] > 0.0)
				sprintf(new_string, "%f,", currentVertex[0]/textureSizes[0]);
			else
				sprintf(new_string, "%f,", currentVertex[0]);
			outputString += new_string;
			if (values_per_vertex  == 1)
				sprintf(new_string, "%f", 0.0);
			else if ((values_per_vertex > 1) && textureSizes[1] > 0.0 )
				sprintf(new_string, "%f", currentVertex[1]/textureSizes[1]);
			else
				sprintf(new_string, "%f", currentVertex[1]);
			outputString += new_string;
			if (i < vertex_count - 1)
			{
				outputString += ",";
			}
			currentVertex+=values_per_vertex;
		}
		outputString += "\n\t]],\n\n";
	}
}

/* Export surfaces graphics into a json format recognisable by threejs. */
int Threejs_export::exportGraphicsObject(struct GT_object *object, int time_step)
{
	if (object)
	{
		int typebitmask = 0;
		int buffer_binding = object->buffer_binding;
		object->buffer_binding = 1;

		switch (GT_object_get_type(object))
		{
		case g_SURFACE_VERTEX_BUFFERS:
		{
			/* export the vertices */
			GLfloat *position_vertex_buffer = NULL;
			unsigned int position_values_per_vertex, position_vertex_count;
			if (object->vertex_array->get_float_vertex_buffer(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
				&position_vertex_buffer, &position_values_per_vertex,
				&position_vertex_count))
			{
				if (time_step == 0)
				{
					writeVertexBuffer("vertices",
						position_vertex_buffer, position_values_per_vertex,
						position_vertex_count);
				}
				if (number_of_time_steps > 1)
				{
					if (morphVertices)
					{
						morphVerticesExported = true;
						writeMorphVertexBuffer("vertices", &verticesMorphString,
							position_vertex_buffer, position_values_per_vertex,
							position_vertex_count, time_step);
					}
				}
			}

			/* export the colour buffer */
			if (mode == CMZN_STREAMINFORMATION_SCENE_IO_DATA_TYPE_COLOUR)
			{
				/* this case export the colour */
				unsigned int colour_values_per_vertex, colour_vertex_count;
				GLfloat *colour_buffer = (GLfloat *)NULL;
				if (Graphics_object_create_colour_buffer_from_data(object,
					&colour_buffer,
					&colour_values_per_vertex, &colour_vertex_count)
					&& (colour_vertex_count == position_vertex_count))
				{
					int *hex_colours = new int[colour_vertex_count];
					GLfloat *colours = colour_buffer;
					for (unsigned int i = 0; i < colour_vertex_count; i++)
					{
						hex_colours[i] = rgb_to_hex(colours[0], colours[1], colours[2]);
						colours += colour_values_per_vertex;
					}
					if (time_step == 0)
					{
						typebitmask |= THREEJS_TYPE_VERTEX_COLOR;
						writeIntegerBuffer("colors",
							hex_colours, 1, colour_vertex_count);
					}
					if (number_of_time_steps > 1)
					{
						if (morphColours)
						{
							morphColoursExported = true;
							writeMorphIntegerBuffer("colors", &colorsMorphString,
								hex_colours, 1, colour_vertex_count, time_step);
						}
					}
					delete[] hex_colours;
					if (colour_buffer)
					{
						DEALLOCATE(colour_buffer);
					}
				}
			}
			else if (mode == CMZN_STREAMINFORMATION_SCENE_IO_DATA_TYPE_PER_VERTEX_VALUE ||
					CMZN_STREAMINFORMATION_SCENE_IO_DATA_TYPE_PER_FACE_VALUE)
			{
				/* this case export the field data directly */
				GLfloat *data_buffer = NULL;
				unsigned int data_values_per_vertex, data_vertex_count;
				if (object->vertex_array->get_float_vertex_buffer(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
					&data_buffer, &data_values_per_vertex, &data_vertex_count))
				{
					if (time_step == 0)
					{
						if (mode == CMZN_STREAMINFORMATION_SCENE_IO_DATA_TYPE_PER_FACE_VALUE)
						{
							typebitmask |= THREEJS_TYPE_FACE_COLOR;
						}
						else
						{
							typebitmask |= THREEJS_TYPE_VERTEX_COLOR;
						}
						writeSpecialDataBuffer(object, data_buffer, data_values_per_vertex,
							data_vertex_count);
					}
				}
			}

			/* export the normal buffer */
			GLfloat *normal_buffer = NULL;
			unsigned int normal_values_per_vertex, normal_vertex_count;
			if (object->vertex_array->get_float_vertex_buffer(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
				&normal_buffer, &normal_values_per_vertex, &normal_vertex_count)
				&& (3 == normal_values_per_vertex) && (normal_vertex_count > 0))
			{
				if (time_step == 0)
				{
					typebitmask |= THREEJS_TYPE_VERTEX_NORMAL;
					writeVertexBuffer("normals",
						normal_buffer, normal_values_per_vertex,
						normal_vertex_count);
				}
				if (number_of_time_steps > 1)
				{
					if (morphNormals)
					{
						morphNormalsExported = true;
						writeMorphVertexBuffer("normals", &normalMorphString,
							normal_buffer, normal_values_per_vertex,
							normal_vertex_count, time_step);
					}
				}
			}

			/* export the texture coordinates buffer */
			GLfloat *texture_coordinate0_buffer = NULL;
			unsigned int texture_coordinate0_values_per_vertex,
			texture_coordinate0_vertex_count;
			if (object->vertex_array->get_float_vertex_buffer(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_TEXTURE_COORDINATE_ZERO,
				&texture_coordinate0_buffer, &texture_coordinate0_values_per_vertex,
				&texture_coordinate0_vertex_count) && (texture_coordinate0_vertex_count > 0))
			{
				if (time_step == 0)
				{
					typebitmask |= THREEJS_TYPE_VERTEX_TEX_COORD;
					writeUVsBuffer(texture_coordinate0_buffer, texture_coordinate0_values_per_vertex,
						texture_coordinate0_vertex_count);
				}
			}
			if (time_step == 0)
			{
				writeIndexBuffer(object, typebitmask, position_vertex_count, 0);
			}
		} break;
		default:
			break;
		}
		object->buffer_binding = buffer_binding;
		return 1;
	}
	return 0;
}

Threejs_export_glyph::~Threejs_export_glyph()
{
	if (glyphGeometriesURLName)
		DEALLOCATE(glyphGeometriesURLName);
}

void Threejs_export_glyph::writeGlyphIndexBuffer(struct GT_object *glyph, int typeMask)
{
	GT_object *temp_glyph = glyph;
	unsigned int offset = 0;
	while (temp_glyph)
	{
		if (GT_object_get_type(temp_glyph) == g_SURFACE_VERTEX_BUFFERS)
		{
			unsigned number_of_vertices = temp_glyph->vertex_array->get_number_of_vertices(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION);
			writeIndexBuffer(temp_glyph, typeMask, number_of_vertices, offset);
			offset +=  number_of_vertices* 3;
		}
		temp_glyph = GT_object_get_next_object(temp_glyph);
	}
}

/*
 * A glyph potentially includes multiple glyphs, this method will export them and
 * put them into one json format
 * . */
void Threejs_export_glyph::exportStaticGlyphs(struct GT_object *glyph)
{
	int typebitmask = 0;
	GT_object *temp_glyph = glyph;
	int number_of_surfaces = 0;
	unsigned int positions_buffer_size = 0, normal_buffer_size = 0,
		uvs_buffer_size = 0;
	while (temp_glyph)
	{
		if (GT_object_get_type(temp_glyph) == g_SURFACE_VERTEX_BUFFERS)
		{
			number_of_surfaces++;
		}
		positions_buffer_size += temp_glyph->vertex_array->get_number_of_vertices(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION);
		normal_buffer_size += temp_glyph->vertex_array->get_number_of_vertices(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL);
		uvs_buffer_size += temp_glyph->vertex_array->get_number_of_vertices(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_TEXTURE_COORDINATE_ZERO);
		temp_glyph = GT_object_get_next_object(temp_glyph);
	}
	GLfloat *positions = new GLfloat[positions_buffer_size * 3];
	GLfloat *normals = new GLfloat[normal_buffer_size * 3];
	GLfloat *uvs = new GLfloat[uvs_buffer_size * 3];

	if (number_of_surfaces > 0)
	{
		temp_glyph = glyph;
		unsigned int position_values_per_vertex = 0, total_position_vertex_count = 0;
		unsigned int normal_values_per_vertex = 0, total_normal_vertex_count = 0;
		unsigned int texture_coordinate0_values_per_vertex = 0, total_texture_coordinate0_vertex_count = 0;
		int current_index = 0;
		GLfloat *position_vertex_buffer = 0, *normal_buffer = 0, *texture_coordinate0_buffer = 0;
		while (temp_glyph)
		{
			int temp_binding = temp_glyph->buffer_binding;
			temp_glyph->buffer_binding = 1;
			if (GT_object_get_type(temp_glyph) == g_SURFACE_VERTEX_BUFFERS)
			{
				unsigned int position_vertex_count = 0, normal_vertex_count = 0,
					texture_coordinate0_vertex_count = 0;
				temp_glyph->vertex_array->get_float_vertex_buffer(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
					&position_vertex_buffer, &position_values_per_vertex,
					&position_vertex_count);
				if (position_vertex_count > 0)
				{
					GLfloat *values = &positions[total_position_vertex_count * 3];
					for (unsigned int i = 0; i < position_vertex_count; i++)
					{
						values[0] = position_vertex_buffer[i*3];
						values[1] = position_vertex_buffer[i*3 + 1];
						values[2] = position_vertex_buffer[i*3 + 2];
						values+=3;
					}
				}
				total_position_vertex_count += position_vertex_count;

				temp_glyph->vertex_array->get_float_vertex_buffer(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
					&normal_buffer, &normal_values_per_vertex, &normal_vertex_count);
				if (normal_vertex_count > 0)
				{
					GLfloat *values = &normals[total_normal_vertex_count * 3];
					for (unsigned int i = 0; i < normal_vertex_count; i++)
					{
						values[0] = normal_buffer[i*3];
						values[1] = normal_buffer[i*3 + 1];
						values[2] = normal_buffer[i*3 + 2];
						values += 3;
					}
				}
				total_normal_vertex_count += normal_vertex_count;

				temp_glyph->vertex_array->get_float_vertex_buffer(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_TEXTURE_COORDINATE_ZERO,
					&texture_coordinate0_buffer, &texture_coordinate0_values_per_vertex,
					&texture_coordinate0_vertex_count);
				if (texture_coordinate0_vertex_count > 0)
				{
					GLfloat *values = &uvs[total_texture_coordinate0_vertex_count * 3];
					for (unsigned int i = 0; i < total_texture_coordinate0_vertex_count; i++)
					{
						values[0] = texture_coordinate0_buffer[i*3];
						values[1] = texture_coordinate0_buffer[i*3 + 1];
						values[2] = texture_coordinate0_buffer[i*3 + 2];
						values += 3;
					}
				}
				total_texture_coordinate0_vertex_count += texture_coordinate0_vertex_count;
				current_index++;
			}
			temp_glyph->buffer_binding = temp_binding;
			temp_glyph = GT_object_get_next_object(temp_glyph);
		}
		writeVertexBuffer("vertices",	positions, 3, total_position_vertex_count);
		if (total_normal_vertex_count > 0)
		{
			typebitmask |= THREEJS_TYPE_VERTEX_NORMAL;
			writeVertexBuffer("normals", normals, 3, total_normal_vertex_count);
		}
		if (total_texture_coordinate0_vertex_count > 0)
		{
			typebitmask |= THREEJS_TYPE_VERTEX_TEX_COORD;
			writeUVsBuffer(uvs, 3, total_texture_coordinate0_vertex_count);
		}
		writeGlyphIndexBuffer(glyph, typebitmask);
	}
	delete[] positions;
	delete[] normals;
	delete[] uvs;
}

void Threejs_export_glyph::exportGlyphsLabel(struct GT_object *object)
{
	GT_glyphset_vertex_buffers *glyph_set = NULL;
	if (object->primitive_lists)
		glyph_set = object->primitive_lists->gt_glyphset_vertex_buffers;
	cmzn_glyph_repeat_mode glyph_repeat_mode = glyph_set->glyph_repeat_mode;
	unsigned number_of_vertices = object->vertex_array->get_number_of_vertices(
		GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION);
	if (number_of_vertices > 0)
	{
		unsigned int label_per_vertex = 0, label_count = 0;
		std::string *label_buffer = 0;
		object->vertex_array->get_string_vertex_buffer(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_LABEL,
			&label_buffer, &label_per_vertex, &label_count);
		std::string *label = label_buffer;
		if (label)
		{
			for (unsigned int i = 0; i < number_of_vertices; i++)
			{
				label_json.append(label->c_str());
				label++;
			}
		}
	}
}

void Threejs_export_glyph::exportGlyphsTransformation(struct GT_object *object, int time_step)
{
	GT_glyphset_vertex_buffers *glyph_set = NULL;
	if (object->primitive_lists)
		glyph_set = object->primitive_lists->gt_glyphset_vertex_buffers;
	cmzn_glyph_repeat_mode glyph_repeat_mode = glyph_set->glyph_repeat_mode;
	unsigned number_of_vertices = object->vertex_array->get_number_of_vertices(
		GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION);
	char temp_string[50];
	if (number_of_vertices > 0)
	{
		unsigned number_of_vertices = object->vertex_array->get_number_of_vertices(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION);
		GLfloat *position_buffer = 0, *colour_buffer = 0, *axis1_buffer = 0,
			*axis2_buffer = 0, *axis3_buffer = 0, *scale_buffer = 0;
		unsigned int position_values_per_vertex = 0, position_vertex_count = 0,
			axis1_values_per_vertex = 0, axis1_vertex_count = 0, axis2_values_per_vertex = 0,
			axis2_vertex_count = 0, axis3_values_per_vertex = 0, axis3_vertex_count = 0,
			scale_values_per_vertex = 0, scale_vertex_count = 0,
			colour_values_per_vertex = 0, colour_vertex_count = 0;
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
		Graphics_object_create_colour_buffer_from_data(object,
			&colour_buffer,
			&colour_values_per_vertex, &colour_vertex_count);
		GLfloat *position = position_buffer,
			*axis1 = axis1_buffer, *axis2 = axis2_buffer,
			*axis3 = axis3_buffer, *scale = scale_buffer,
			*colours = colour_buffer;

		if (time_step == 0 || ((number_of_time_steps > 1) && (morphVertices || morphColours)))
		{
			sprintf(temp_string, "%d", time_step);
			for (unsigned int i = 0; i < number_of_vertices; i++)
			{
				if (time_step == 0 || ((number_of_time_steps > 1) && morphVertices))
				{
					if (time_step != 0)
						morphVerticesExported = true;
					for (unsigned int k = 0; k < position_values_per_vertex; k++)
					{
						positions_json[temp_string].append(position[k]);
						axis1_json[temp_string].append(axis1[k]);
						axis2_json[temp_string].append(axis2[k]);
						axis3_json[temp_string].append(axis3[k]);
						scale_json[temp_string].append(scale[k]);
					}
					position += position_values_per_vertex;
					axis1 += axis1_values_per_vertex;
					axis2 += axis2_values_per_vertex;
					axis3 += axis3_values_per_vertex;
					scale += scale_values_per_vertex;
				}
				if (colours && (time_step == 0 || (morphColours && (number_of_time_steps > 1))))
				{
					int hex_value = 0;
					hex_value = rgb_to_hex(colours[0], colours[1], colours[2]);
					color_json[temp_string].append(hex_value);
					colours += colour_values_per_vertex;
					if (time_step != 0)
						morphColoursExported = true;
				}
			}
		}

		if (time_step == 0)
		{
			for (int i = 0; i < 3; i++)
			{
				metadata["base_size"].append(glyph_set->base_size[i]);
				metadata["scale_factors"].append(glyph_set->scale_factors[i]);
				metadata["offset"].append(glyph_set->offset[i]);
			}
			metadata["number_of_vertices"] = number_of_vertices;
			metadata["number_of_time_steps"] = number_of_time_steps;
			switch (glyph_repeat_mode)
			{
				case CMZN_GLYPH_REPEAT_MODE_MIRROR:
					metadata["repeat_mode"] = "MIRROR";
					break;
				case CMZN_GLYPH_REPEAT_MODE_AXES_2D:
					metadata["repeat_mode"] = "AXES_2D";
					break;
				case CMZN_GLYPH_REPEAT_MODE_AXES_3D:
					metadata["repeat_mode"] = "AXES_3D";
					break;
				default:
					metadata["repeat_mode"] = "NONE";
					break;
			}
		}
		metadata["MorphColours"] = morphColoursExported;
		metadata["MorphVertices"] = morphVerticesExported;
	}
}

int Threejs_export_glyph::exportGraphicsObject(struct GT_object *object, int time_step)
{
	if (object)
	{
		int buffer_binding = object->buffer_binding;
		object->buffer_binding = 1;

		switch (GT_object_get_type(object))
		{
		case g_GLYPH_SET_VERTEX_BUFFERS:
		{
			if (time_step == 0)
			{
				GT_glyphset_vertex_buffers *glyph_set = NULL;
				if (object->primitive_lists)
					glyph_set = object->primitive_lists->gt_glyphset_vertex_buffers;
				if (glyph_set && glyph_set->glyph)
				{
					/* first time step, export both glyph primitives and transformation */
					exportStaticGlyphs(glyph_set->glyph);
					exportGlyphsLabel(object);
					exportGlyphsTransformation(object, time_step);
				}
			}
			else
			{
				exportGlyphsTransformation(object, time_step);
			}
		}break;
		default:
			break;
		}
		object->buffer_binding = buffer_binding;
		return 1;
	}
	return 0;
}

void Threejs_export_glyph::setGlyphGeometriesURLName(char *name)
{
	if (name)
		glyphGeometriesURLName = duplicate_string(name);
}

Json::Value Threejs_export_glyph::getGlyphTransformationExportJson()
{
	Json::Value root;
	root["metadata"] = metadata;
	root["positions"] = positions_json;
	root["axis1"] = axis1_json;
	root["axis2"] = axis2_json;
	root["axis3"] = axis3_json;
	root["scale"] = scale_json;
	if (color_json.size() > 0)
		root["colors"] = color_json;
	if (label_json.size() > 0)
		root["label"] = label_json;
	if (glyphGeometriesURLName)
		root["GlyphGeometriesURL"] = glyphGeometriesURLName;
	return root;
}

std::string *Threejs_export_glyph::getGlyphTransformationExportString()
{
	Json::Value root = getGlyphTransformationExportJson();
	glyphTransformationString = Json::StyledWriter().write(root);

	return &glyphTransformationString;
}

/* write index for triangle surfaces (non triangle-stripe). */
void Threejs_export_point::writeIndexBufferWithoutIndex(int typeMask, int number_of_points,
	unsigned int offset)
{
	if (number_of_points)
	{
		char temp[100];
		facesString += "\t\"faces\": [\n";
		unsigned int number_of_triangles = number_of_points / 3;
		int current_index = 0;
		for (unsigned i = 0; i < number_of_triangles; i++)
		{
			sprintf(temp,"\t\t%d", typeMask);
			facesString += temp;
			sprintf(temp," ,%d,%d,%d", current_index+offset, current_index+offset+1, current_index+offset+2);
			facesString += temp;
			if (typeMask & THREEJS_TYPE_VERTEX_COLOR)
			{
				sprintf(temp," ,%d,%d,%d", current_index+offset, current_index+offset+1, current_index+offset+2);
				facesString += temp;
			}
			current_index += 3;
			if (i != number_of_triangles - 1)
			{
				facesString += ",";
			}
			facesString += "\n";
		}
		/* fill it up */
		unsigned int unused_points =  number_of_points - number_of_triangles * 3;
		if (unused_points > 0)
		{
			facesString += ",";
			sprintf(temp,"\t\t%d", typeMask);
			facesString += temp;
			if (unused_points == 1)
			{
				sprintf(temp," ,%d,%d,%d", current_index+offset, current_index+offset, current_index+offset);
				facesString += temp;
				if (typeMask & THREEJS_TYPE_VERTEX_COLOR)
				{
					sprintf(temp," ,%d,%d,%d", current_index+offset, current_index+offset, current_index+offset);
					facesString += temp;
				}
			}
			else
			{
				sprintf(temp," ,%d,%d,%d", current_index+offset, current_index+offset+1, current_index+offset+1);
				facesString += temp;
				if (typeMask & THREEJS_TYPE_VERTEX_COLOR)
				{
					sprintf(temp," ,%d,%d,%d", current_index+offset, current_index+offset+1, current_index+offset+1);
					facesString += temp;
				}
			}
			facesString += "\n";
		}


		facesString += "\t]\n\n";
	}
}

/* Export surfaces graphics into a json format recognisable by threejs. */
int Threejs_export_point::exportGraphicsObject(struct GT_object *object, int time_step)
{
	if (object)
	{
		int typebitmask = 0;
		int buffer_binding = object->buffer_binding;
		object->buffer_binding = 1;

		/* export the vertices */
		GLfloat *position_vertex_buffer = NULL;
		unsigned int position_values_per_vertex, position_vertex_count;
		if (object->vertex_array->get_float_vertex_buffer(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
				&position_vertex_buffer, &position_values_per_vertex,
				&position_vertex_count))
		{
			if (time_step == 0)
			{
				writeVertexBuffer("vertices",
						position_vertex_buffer, position_values_per_vertex,
						position_vertex_count);
			}
			if (number_of_time_steps > 1)
			{
				if (morphVertices)
				{
					morphVerticesExported = true;
					writeMorphVertexBuffer("vertices", &verticesMorphString,
							position_vertex_buffer, position_values_per_vertex,
							position_vertex_count, time_step);
				}
			}
		}

		/* export the colour buffer */
		if (mode == CMZN_STREAMINFORMATION_SCENE_IO_DATA_TYPE_COLOUR)
		{
			/* this case export the colour */
			unsigned int colour_values_per_vertex, colour_vertex_count;
			GLfloat *colour_buffer = (GLfloat *)NULL;
			if (Graphics_object_create_colour_buffer_from_data(object,
					&colour_buffer,
					&colour_values_per_vertex, &colour_vertex_count)
					&& (colour_vertex_count == position_vertex_count))
			{
				int *hex_colours = new int[colour_vertex_count];
				GLfloat *colours = colour_buffer;
				for (unsigned int i = 0; i < colour_vertex_count; i++)
				{
					hex_colours[i] = rgb_to_hex(colours[0], colours[1], colours[2]);
					colours += colour_values_per_vertex;
				}
				if (time_step == 0)
				{
					typebitmask |= THREEJS_TYPE_VERTEX_COLOR;
					writeIntegerBuffer("colors",
							hex_colours, 1, colour_vertex_count);
				}
				if (number_of_time_steps > 1)
				{
					if (morphColours)
					{
						morphColoursExported = true;
						writeMorphIntegerBuffer("colors", &colorsMorphString,
								hex_colours, 1, colour_vertex_count, time_step);
					}
				}
				delete[] hex_colours;
				if (colour_buffer)
				{
					DEALLOCATE(colour_buffer);
				}
			}
		}
		else if (mode == CMZN_STREAMINFORMATION_SCENE_IO_DATA_TYPE_PER_VERTEX_VALUE ||
				CMZN_STREAMINFORMATION_SCENE_IO_DATA_TYPE_PER_FACE_VALUE)
		{
			/* this case export the field data directly */
			GLfloat *data_buffer = NULL;
			unsigned int data_values_per_vertex, data_vertex_count;
			if (object->vertex_array->get_float_vertex_buffer(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
					&data_buffer, &data_values_per_vertex, &data_vertex_count))
			{
				if (time_step == 0)
				{
					if (mode == CMZN_STREAMINFORMATION_SCENE_IO_DATA_TYPE_PER_FACE_VALUE)
					{
						typebitmask |= THREEJS_TYPE_FACE_COLOR;
					}
					else
					{
						typebitmask |= THREEJS_TYPE_VERTEX_COLOR;
					}
					writeSpecialDataBuffer(object, data_buffer, data_values_per_vertex,
							data_vertex_count);
				}
			}
		}

		if (time_step == 0)
		{
			writeIndexBuffer(object, typebitmask, position_vertex_count, 0);
		}

		object->buffer_binding = buffer_binding;
		return 1;
	}
	return 0;
}

/* Export surfaces graphics into a json format recognisable by threejs. */
int Threejs_export_line::exportGraphicsObject(struct GT_object *object, int time_step)
{
	if (object)
	{
		int buffer_binding = object->buffer_binding;
		object->buffer_binding = 1;
		unsigned int line_index;
		unsigned int line_count = object->vertex_array->get_number_of_vertices(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START);
		unsigned int position_values_per_vertex = 0, position_vertex_count = 0,
			data_values_per_vertex = 0, data_vertex_count = 0;
		GLfloat *position_buffer = 0;
		GLfloat *data_buffer = 0;
		object->vertex_array->get_float_vertex_buffer(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
			&position_buffer, &position_values_per_vertex, &position_vertex_count);
		object->vertex_array->get_float_vertex_buffer(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
			&data_buffer, &data_values_per_vertex, &data_vertex_count);

		/* cannot use the index as i will be using GL_LINE for rendering on threejs */
		FE_value *data_values = (0 != data_buffer) ? new FE_value[data_values_per_vertex] : 0;
		int totalVertices =0;

		for (line_index = 0; line_index < line_count; line_index++)
		{
			unsigned int index_count = 0;
			object->vertex_array->get_unsigned_integer_attribute(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
				line_index, 1, &index_count);
			totalVertices += (index_count - 1) * 2;
		}

		GLfloat *positions = 0;
		ALLOCATE(positions, GLfloat, position_values_per_vertex * totalVertices);

		int currentIndex = 0;

		for (line_index = 0; line_index < line_count; line_index++)
		{
			unsigned int i, index_start, index_count;
			GLfloat *position_vertex = 0;

			object->vertex_array->get_unsigned_integer_attribute(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
				line_index, 1, &index_start);
			object->vertex_array->get_unsigned_integer_attribute(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
				line_index, 1, &index_count);
			position_vertex = position_buffer + position_values_per_vertex * index_start;

			for (i = 0; i < index_count; ++i)
			{
				for (unsigned int j = 0; j < position_values_per_vertex; ++j)
				{
					positions[currentIndex+j] = position_vertex[j];
				}
				currentIndex += position_values_per_vertex;
				if ((i != 0) && (i != index_count - 1))
				{
					for (unsigned int j = 0; j < position_values_per_vertex; ++j)
					{
						positions[currentIndex+j] = position_vertex[j];
					}
					currentIndex += position_values_per_vertex;
				}
				position_vertex += position_values_per_vertex;
			}
		}
		if (time_step == 0)
		{
			writeVertexBuffer("vertices", 	positions, position_values_per_vertex, totalVertices);
		}
		if (number_of_time_steps > 1)
		{
			if (morphVertices)
			{
				morphVerticesExported = true;
				writeMorphVertexBuffer("vertices", &verticesMorphString, positions,
					position_values_per_vertex, totalVertices, time_step);
			}
		}
		if (time_step == 0)
		{
			writeIndexBuffer(object, 0, totalVertices, 0);
		}
		object->buffer_binding = buffer_binding;
		DEALLOCATE(positions);
		return 1;
	}

	return 0;

}

