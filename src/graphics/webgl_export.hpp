/***************************************************************************//**
 * FILE : webgl_export.hpp
 *
 * File containing a class for exporting webgl vertices and script for rendering.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "general/mystring.h"
#include "graphics/graphics_library.h"
#include "graphics/render_gl.h"
#include <string>

struct GT_object;

class Webgl_export
{
private:
	char *filename;
	FILE *webgl_file;
	std::string drawFunctionsString;
	std::string bindBuffersString;

	std::string *writeVertexBuffer(const char *output_variable_name,
		GLfloat *vertex_buffer, unsigned int values_per_vertex,
		unsigned int vertex_count);

	void writeWebGLBindingFunction(std::string *position_string,
		std::string *colour_string, std::string *normal_string,
		std::string *tex_coord_string, std::string *index_string,
		const char *export_name);

	void writeWebGLDrawingFunction(std::string *position_string,
		std::string *colour_string, std::string *normal_string,
		std::string *tex_coord_string, std::string *index_string,
		const char *export_name);

	std::string *writeIndexBuffer(struct GT_object *object, const char *export_name);

public:

	Webgl_export(const char *filename) :
		filename(duplicate_string(filename)), webgl_file(0)
	{
		drawFunctionsString.clear();
		bindBuffersString.clear();
	}

	~Webgl_export();

	int exportGraphicsObject(struct GT_object *object,	const char *export_name);

	int beginExport();

	int endExport();

};
