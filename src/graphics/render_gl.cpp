/*******************************************************************************
FILE : rendergl.cpp

DESCRIPTION :
GL rendering calls - API specific.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stdio.h>
#include <math.h>
#include <map>
#include "opencmiss/zinc/zincconfigure.h"

#include "general/mystring.h"
#include "general/debug.h"
#include "graphics/auxiliary_graphics_types.h"
#include "graphics/graphics_library.h"
#include "graphics/font.h"
#include "graphics/glyph.hpp"
#include "graphics/graphics.h"
#include "graphics/graphics_object.h"
#include "graphics/mcubes.h"
#include "graphics/light.hpp"
#include "graphics/spectrum.h"
#include "graphics/tile_graphics_objects.h"
#include "general/message.h"
#include "graphics/graphics_object_private.hpp"
#include "graphics/material.hpp"
#include "graphics/render_gl.h"
#include "graphics/scene.hpp"
#include "graphics/scene_coordinate_system.hpp"
#include "graphics/spectrum.hpp"
#include "graphics/texture.hpp"
#include "graphics/threejs_export.hpp"
#include "graphics/webgl_export.hpp"
#include "jsoncpp/json.h"

/*
Module variables
----------------
*/

// limitation of OpenGL fixed pipeline:
#define MAXIMUM_NUMBER_OF_ACTIVE_LIGHTS 8

static GLenum light_identifiers[MAXIMUM_NUMBER_OF_ACTIVE_LIGHTS]=
{
	GL_LIGHT0,GL_LIGHT1,GL_LIGHT2,GL_LIGHT3,GL_LIGHT4,GL_LIGHT5,GL_LIGHT6,
	GL_LIGHT7
};

#define BUFFER_OFFSET(bytes) ((GLubyte*) NULL + (bytes))

/*
Module types
------------
*/

/**
 * Specifies the rendering type for this graphics_object.  The render function
 * could be split for different types or virtual methods or function callbacks
 * added but much of the code would be repeated with the first strategy and
 * there are a number of different points where the behaviour changes which would
 * need over-riding for the second.  So the current plan is to keep a single
 * routine and switch on this type.
 */
enum Graphics_object_rendering_type
{
	GRAPHICS_OBJECT_RENDERING_TYPE_GLBEGINEND,
	GRAPHICS_OBJECT_RENDERING_TYPE_CLIENT_VERTEX_ARRAYS,
	GRAPHICS_OBJECT_RENDERING_TYPE_VERTEX_BUFFER_OBJECT
};

/**
 * Convert graphical object into API object.
 */
static int render_GT_object_opengl_immediate(gtObject *object,
	int draw_selected, Render_graphics_opengl *renderer,
	Graphics_object_rendering_type type);

static int Graphics_object_compile_opengl_display_list(GT_object *graphics_object,
	Callback_base< GT_object * > *execute_function,
	Render_graphics_opengl *renderer);

static int Graphics_object_execute_opengl_display_list(GT_object *graphics_object,
	Render_graphics_opengl *renderer);

/**
 * Call the renderer to compile all the member objects which this GT_object depends
 * on.
 */
static int Graphics_object_compile_members_opengl(GT_object *graphics_object_list,
	Render_graphics_opengl *renderer);

/**
 * Rebuilds the display list for each uncreated graphics object in the
 * <graphics_object>, a simple linked list.
*/
static int Graphics_object_render_opengl(GT_object *graphics_object,
	Render_graphics_opengl *renderer, Graphics_object_rendering_type type);

/**
 * Compile the vertex buffer object for rendering the specified primitive.
 */
static int Graphics_object_compile_opengl_vertex_buffer_object(GT_object *object,
	Render_graphics_opengl *renderer);

/*==== Render_graphics_opengl method implementations ====*/

int Render_graphics_opengl::Graphics_object_compile(GT_object *graphics_object)
{
	return Graphics_object_compile_members_opengl(graphics_object, this);
}

int Render_graphics_opengl::Graphics_compile(cmzn_graphics *graphics)
{
	return Graphics_object_compile_members_opengl(cmzn_graphics_get_graphics_object(
		graphics), this);
}

int Render_graphics_opengl::Material_compile(cmzn_material *material)
{
	return Material_compile_members_opengl(material, this);
}

void Render_graphics_opengl::Graphics_object_execute_point_size(GT_object *graphics_object)
{
	if (graphics_object->render_line_width != 0.0)
	{
		glLineWidth(static_cast<GLfloat>(graphics_object->render_line_width*this->get_point_unit_size_pixels()));
	}
	if (graphics_object->render_point_size != 0.0)
	{
		glPointSize(static_cast<GLfloat>(graphics_object->render_point_size*this->get_point_unit_size_pixels()));
	}
}

void Render_graphics_opengl::reset_lights()
{
	for (unsigned int light_no = 0; light_no < MAXIMUM_NUMBER_OF_ACTIVE_LIGHTS; ++light_no)
		glDisable(light_identifiers[light_no]);
	this->next_light_no = 0;
}

/**
 * An implementation of a render class that uses immediate mode glBegin/glEnd.
 */
class Render_graphics_opengl_glbeginend : public Render_graphics_opengl
{
public:
	Render_graphics_opengl_glbeginend() : Render_graphics_opengl()
	  {
		Render_graphics_opengl::use_display_list = 0;
	  }

	  int Scene_tree_execute(cmzn_scene *scene)
	  {
		  set_Scene(scene);
		  int return_code = Scene_render_opengl(scene, this);
		  return return_code;
	  }

	  int Graphics_object_execute(GT_object *graphics_object)
	  {
		  return Graphics_object_render_opengl(graphics_object, this,
			  GRAPHICS_OBJECT_RENDERING_TYPE_GLBEGINEND);
	  }

	  int Graphics_execute(cmzn_graphics *graphics)
	  {
		  GT_object *graphics_object = cmzn_graphics_get_graphics_object(
			  graphics);
		  return Graphics_object_render_opengl(graphics_object, this,
			  GRAPHICS_OBJECT_RENDERING_TYPE_GLBEGINEND);
	  }

	  int Graphics_object_render_immediate(GT_object *graphics_object)
	  {
		  return Graphics_object_render_opengl(graphics_object, this,
			  GRAPHICS_OBJECT_RENDERING_TYPE_GLBEGINEND);
	  }

	  int cmzn_scene_execute(cmzn_scene *scene)
	  {
		  return execute_cmzn_scene(scene, this);
	  }

	  int cmzn_scene_execute_graphics(cmzn_scene *scene)
	  {
		  if (this->picking)
			  glPushName(0);
		  int return_code = cmzn_scene_graphics_render_opengl(scene, this);
		  if (this->picking)
			  glPopName();
		  return return_code;
	  }

	  int cmzn_scene_execute_child_scene(cmzn_scene *scene)
	  {
		  return cmzn_scene_render_child_scene(scene, this);
	  }

	  int Material_execute(cmzn_material *material)
	  {
		  return Material_render_opengl(material, this);
	  }

	  int Texture_compile(Texture *texture)
	  {
		  return Texture_compile_opengl_texture_object(texture, this);
	  }

	  int Texture_execute(Texture *texture)
	  {
		  return Texture_execute_opengl_texture_object(texture, this);
	  }

		// enable lighting model with supplied ambient colour and other parameters
		void Light_model_enable(Colour& ambientColour, bool lightingLocalViewer, bool lightingTwoSided)
		{
			GLfloat ambientColourFloat[3] =
			{
				static_cast<GLfloat>(ambientColour.red),
				static_cast<GLfloat>(ambientColour.green),
				static_cast<GLfloat>(ambientColour.blue)
			};
			glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientColourFloat);
			glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, lightingLocalViewer ? GL_TRUE : GL_FALSE);
			glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, lightingTwoSided ? GL_TRUE : GL_FALSE);
			glEnable(GL_LIGHTING);
		}

		// disable lighting for flat colouring
		void Light_model_disable()
		{
			glDisable(GL_LIGHTING);
		}

		int cmzn_light_execute(cmzn_light *light)
		{
			int return_code;
			const GLenum gl_light_id = (this->next_light_no < MAXIMUM_NUMBER_OF_ACTIVE_LIGHTS) ?
				light_identifiers[this->next_light_no] : GL_INVALID_ENUM;
			return_code = direct_render_cmzn_light(light, static_cast<unsigned int>(gl_light_id));
			if (return_code == 1)
				++this->next_light_no;
			return return_code;
		}

	  virtual int begin_coordinate_system(enum cmzn_scenecoordinatesystem coordinate_system)
	  {
		  int return_code = 1;
		  switch (coordinate_system)
		  {
		  case CMZN_SCENECOORDINATESYSTEM_LOCAL:
			  {
				  /* Do nothing */
			  } break;
		  case CMZN_SCENECOORDINATESYSTEM_WORLD:
			  {
				  glMatrixMode(GL_MODELVIEW);
				  glPushMatrix();
				  glLoadMatrixd(world_view_matrix);
			  } break;
		  case CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FILL:
		  case CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FIT_CENTRE:
		  case CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FIT_LEFT:
		  case CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FIT_RIGHT:
		  case CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FIT_TOP:
		  case CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FIT_BOTTOM:
		  case CMZN_SCENECOORDINATESYSTEM_WINDOW_PIXEL_BOTTOM_LEFT:
		  case CMZN_SCENECOORDINATESYSTEM_WINDOW_PIXEL_TOP_LEFT:
			  {
				  if (picking)
				  {
					  /* To pick correctly need additional transformation to interaction volume.
					   * skip window-relative graphics for now. */
					  return_code = 0;
				  }
				  else
				  {
					  /* Push the current model matrix and reset the model matrix to identity */
					  glMatrixMode(GL_PROJECTION);
					  glPushMatrix();
					  glLoadIdentity();
					  double left, right, bottom, top;
					  if (cmzn_scenecoordinatesystem_get_viewport(coordinate_system,
						  viewport_width, viewport_height, &left, &right, &bottom, &top))
					  {
						  if (!(NDC_width == 2.0 && NDC_height == 2.0))
						  {
							  double xScale = 2 / NDC_width, yScale = 2 / NDC_height;
							  double xDiff = 1.0, yDiff = 1.0;
							  if (xScale > yScale)
							  {
								  yDiff = xScale / yScale;
								  yScale = xScale;
							  }
							  else
							  {
								  xDiff = yScale / xScale;
								  xScale = yScale;
							  }
							  glScalef(xScale, yScale, 1.0f);
							  glTranslatef(-1.0 * (NDC_left * yDiff + 1.0 / xScale), -1.0 * ((NDC_top - 2.0) * xDiff + 1.0 / yScale) , 0.0);
						  }
						  /* near = 1.0 and far = 3.0 gives -1 to be the near clipping plane
						   * and +1 to be the far clipping plane */
						  glOrtho(left, right, bottom, top, /*nearVal*/1.0, /*farVal*/3.0);
					  }
					  else
					  {
						  return_code = 0;
					  }
					  glMatrixMode(GL_MODELVIEW);
					  glPushMatrix();
					  glLoadIdentity();
					  gluLookAt(/*eye*/0.0,0.0,2.0, /*lookat*/0.0,0.0,0.0,
						  /*up*/0.0,1.0,0.0);
				  }
			  } break;
		  default:
			  {
				  display_message(ERROR_MESSAGE,"begin_coordinate_system.  Invalid scene coordinate system.");
				  return_code = 0;
			  } break;
		  }
		  return return_code;
	  }

	  virtual void end_coordinate_system(enum cmzn_scenecoordinatesystem coordinate_system)
	  {
		  switch (coordinate_system)
		  {
		  case CMZN_SCENECOORDINATESYSTEM_LOCAL:
			  {
				  /* Do nothing */
			  } break;
		  case CMZN_SCENECOORDINATESYSTEM_WORLD:
			  {
				  glMatrixMode(GL_MODELVIEW);
				  glPopMatrix();
			  } break;
		  case CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FILL:
		  case CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FIT_CENTRE:
		  case CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FIT_LEFT:
		  case CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FIT_RIGHT:
		  case CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FIT_TOP:
		  case CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FIT_BOTTOM:
		  case CMZN_SCENECOORDINATESYSTEM_WINDOW_PIXEL_BOTTOM_LEFT:
		  case CMZN_SCENECOORDINATESYSTEM_WINDOW_PIXEL_TOP_LEFT:
			  {
				  /* Pop the model matrix stack */
				  glMatrixMode(GL_PROJECTION);
				  glPopMatrix();
				  glMatrixMode(GL_MODELVIEW);
				  glPopMatrix();
			  } break;
		  default:
			  {
			  } break;
		  }
	  }

}; /* class Render_graphics_opengl_glbeginend */

Render_graphics_opengl *Render_graphics_opengl_create_glbeginend_renderer()
{
	return new Render_graphics_opengl_glbeginend();
}

/**
 * An implementation of a render class that uses client vertex arrays.
 */
class Render_graphics_opengl_client_vertex_arrays : public Render_graphics_opengl_glbeginend
{
public:
	Render_graphics_opengl_client_vertex_arrays() :
	  Render_graphics_opengl_glbeginend()
	  {
	  }

	  /**
		 * Execute the Graphics_object.
		 */
	  int Graphics_object_execute(GT_object *graphics_object)
	  {
		  return Graphics_object_render_opengl(graphics_object, this,
			  GRAPHICS_OBJECT_RENDERING_TYPE_CLIENT_VERTEX_ARRAYS);
	  }

	  int Graphics_execute(cmzn_graphics *graphics)
	  {
		  GT_object *graphics_object = cmzn_graphics_get_graphics_object(
			  graphics);
		  cmzn_graphics_set_renderer_highlight_functor(graphics, (void *)this);
		  int return_code = Graphics_object_render_opengl(graphics_object, this,
			  GRAPHICS_OBJECT_RENDERING_TYPE_CLIENT_VERTEX_ARRAYS);
		  cmzn_graphics_remove_renderer_highlight_functor(graphics, (void *)this);
		  return return_code;
	  }

	  /**
		 * Execute the Graphics_object.
		 */
	  int Graphics_object_render_immediate(GT_object *graphics_object)
	  {
		  return Graphics_object_render_opengl(graphics_object, this,
			  GRAPHICS_OBJECT_RENDERING_TYPE_CLIENT_VERTEX_ARRAYS);
	  }

}; /* class Render_graphics_opengl_client_vertex_arrays */

/**
 * An implementation of a render class that uses client vertex arrays.
 */
class Render_graphics_opengl_vertex_buffer_object : public Render_graphics_opengl_glbeginend
{
public:
	Render_graphics_opengl_vertex_buffer_object() :
	  Render_graphics_opengl_glbeginend()
	  {
	  }

	  /**
		 * Compile the Graphics_object.
		 */
	  int Graphics_object_compile(GT_object *graphics_object)
	  {
		  return Render_graphics_opengl_glbeginend::Graphics_object_compile(
			  graphics_object)
			  && Graphics_object_compile_opengl_vertex_buffer_object(
			  graphics_object, this);
	  }

	  /**
		 * Compile the Graphics.
		 */
	  int Graphics_compile(cmzn_graphics *graphics)
	  {
		  return Graphics_object_compile(cmzn_graphics_get_graphics_object(
			  graphics));
	  }

	  /**
		 * Execute the Graphics_object.
		 */
	  int Graphics_object_execute(GT_object *graphics_object)
	  {
		  return Graphics_object_render_opengl(graphics_object, this,
			  GRAPHICS_OBJECT_RENDERING_TYPE_VERTEX_BUFFER_OBJECT);
	  }

	  int Graphics_execute(cmzn_graphics *graphics)
	  {
		  GT_object *graphics_object = cmzn_graphics_get_graphics_object(
			  graphics);
		  cmzn_graphics_set_renderer_highlight_functor(graphics, (void *)this);
		  int return_code = Graphics_object_render_opengl(graphics_object, this,
			  GRAPHICS_OBJECT_RENDERING_TYPE_VERTEX_BUFFER_OBJECT);
		  cmzn_graphics_remove_renderer_highlight_functor(graphics, (void *)this);
		  return return_code;
	  }

	  /**
		 * Execute the Graphics_object.
		 */
	  int Graphics_object_render_immediate(GT_object *graphics_object)
	  {
		  return Graphics_object_compile_opengl_vertex_buffer_object(
			  graphics_object, this)
			  && Graphics_object_render_opengl(graphics_object, this,
			  GRAPHICS_OBJECT_RENDERING_TYPE_VERTEX_BUFFER_OBJECT);
	  }

}; /* class Render_graphics_opengl_vertex_buffer_object */

Render_graphics_opengl *Render_graphics_opengl_create_client_vertex_arrays_renderer()
{
	return new Render_graphics_opengl_client_vertex_arrays();
}

Render_graphics_opengl *Render_graphics_opengl_create_vertex_buffer_object_renderer()
{
	return new Render_graphics_opengl_vertex_buffer_object();
}

class Render_graphics_opengl_webgl : public Render_graphics_opengl_vertex_buffer_object
{
public:

	Webgl_export webgl_export;
	int current_number;

	Render_graphics_opengl_webgl(const char *filename_in) :
		Render_graphics_opengl_vertex_buffer_object(),
		webgl_export(filename_in)
	{
		current_number = 0;
	}

	~Render_graphics_opengl_webgl()
	{
	}

	/**
	 * Compile the Graphics_object.
	 */
	int Graphics_object_compile(GT_object *)
	{
		return true;
	}

	/**
	 * Compile the Graphics.
	 */
	int Graphics_compile(cmzn_graphics *graphics)
	{
		return Graphics_object_compile(cmzn_graphics_get_graphics_object(
			graphics));
	}

	int Graphics_execute(cmzn_graphics *graphics)
	{
		GT_object *graphics_object = cmzn_graphics_get_graphics_object(
			graphics);
		char *graphics_name = cmzn_graphics_get_name_internal(graphics);
		char export_name[50];
		sprintf(export_name, "object_%d_graphics_%s", current_number, graphics_name);
		int return_code = webgl_export.exportGraphicsObject(graphics_object, export_name);
		DEALLOCATE(graphics_name);
		return return_code;
	}

	int cmzn_scene_execute_graphics(cmzn_scene *scene)
	{
		return cmzn_scene_graphics_render_opengl(scene, this);
	}

	int cmzn_scene_execute(cmzn_scene *scene)
	{
		current_number = current_number + 1;
		return execute_scene_exporter_output(scene, this);
	}

	int Scene_tree_execute(cmzn_scene *scene)
	{
		if (webgl_export.beginExport())
		{
			set_Scene(scene);
			int return_code = Scene_render_opengl(scene, this);
			webgl_export.endExport();
			return return_code;
		}
		return 0;
	}

}; /* class Render_graphics_opengl_webgl */

Render_graphics_opengl *Render_graphics_opengl_create_webgl_renderer(const char *filename)
{
	return new Render_graphics_opengl_webgl(filename);
}

class Render_graphics_opengl_threejs : public Render_graphics_opengl_vertex_buffer_object
{
public:

	std::map<cmzn_graphics *, Threejs_export *> exports_map;
	char *file_prefix;
	double begin_time, end_time;
	int number_of_time_steps, current_time_frame;
	int current_graphics_number;
	enum cmzn_streaminformation_scene_io_data_type mode;
	int *number_of_entries;
	std::string **output_string;
	int morphVertices, morphColours, morphNormals, numberOfResources;
	char **filenames;
	int isInline;

	Render_graphics_opengl_threejs(const char *file_prefix_in,
		int number_of_time_steps_in, double begin_time_in,  double end_time_in,
		enum cmzn_streaminformation_scene_io_data_type mode_in, int *number_of_entries_in,
		std::string **output_string_in, int morphVerticesIn, int morphColoursIn, int morphNormalsIn,
		int numberOfFilesIn, char **filenamesIn, int isInlineIn) :
		Render_graphics_opengl_vertex_buffer_object(),
		file_prefix(duplicate_string(file_prefix_in)), begin_time(begin_time_in),
		end_time(end_time_in), number_of_time_steps(number_of_time_steps_in),
		mode(mode_in), number_of_entries(number_of_entries_in), numberOfResources(numberOfFilesIn),
		filenames(filenamesIn),
		isInline(isInlineIn)
	{
		exports_map.clear();
		current_graphics_number = 0;
		current_time_frame = 0;
		output_string = output_string_in;
		morphVertices = morphVerticesIn;
		morphColours = morphColoursIn;
		morphNormals = morphNormalsIn;
	}

	~Render_graphics_opengl_threejs()
	{
		if (file_prefix)
			DEALLOCATE(file_prefix);
	}

	int get_number_of_entries()
	{
		int size = 0;
		Threejs_export *threejs_export = 0;

		if (isInline)
		{
			// Inline, everything is stored n the meta file.
			return 1;
		}
		else
		{
			for (std::map<cmzn_graphics *, Threejs_export *>::iterator export_iter = exports_map.begin();
				export_iter != exports_map.end(); export_iter++)
			{
				size++;
				threejs_export =  export_iter->second;
				if (dynamic_cast<Threejs_export_glyph*>(threejs_export))
					size++;
			}
		}
		//Also a file providing metafile, only when there is graphics to be exported
		if (size > 0)
			size++;

		return size;
	}

	/* this will generate the meta data string */
	std::string get_metadata_string()
	{
		Json::Value root;
		int i = 1;
		for (std::map<cmzn_graphics *, Threejs_export *>::iterator export_iter = exports_map.begin();
			export_iter != exports_map.end(); export_iter++)
		{
			Json::Value graphics_json;
			graphics_json["MorphVertices"] = export_iter->second->getMorphVerticesExported();
			graphics_json["MorphColours"] = export_iter->second->getMorphColoursExported();
			graphics_json["MorphNormals"] = export_iter->second->getMorphNormalsExported();
			if (isInline)
			{
				graphics_json["Inline"]["URL"]= export_iter->second->getExportJson();
			}
			else
			{
				if (numberOfResources > i)
				{
					graphics_json["URL"] = filenames[i];
				}
				else
				{
					char temp[20];
					sprintf(temp, "temp_%d.json", i+1);
					graphics_json["URL"] = temp;
				}
			}
			char *group_name = export_iter->second->getGroupNameNonAccessed();
			if (group_name)
				graphics_json["GroupName"] = group_name;

			Threejs_export_glyph *glyph_export = dynamic_cast<Threejs_export_glyph*>(export_iter->second);
			Threejs_export_line *line_export = dynamic_cast<Threejs_export_line*>(export_iter->second);
			Threejs_export_point *point_export = dynamic_cast<Threejs_export_point*>(export_iter->second);
			if (glyph_export)
			{
				graphics_json["Type"]="Glyph";
				i++;
				if (isInline)
				{
					graphics_json["Inline"]["GlyphGeometriesURL"] = glyph_export->getGlyphTransformationExportJson();
				}
				else
				{
					if (numberOfResources > i)
					{
						graphics_json["GlyphGeometriesURL"] = filenames[i];
						glyph_export->setGlyphGeometriesURLName(filenames[i]);
					}
					else
					{
						char temp[20];
						sprintf(temp, "temp_%d.json", i+1);
						graphics_json["GlyphGeometriesURL"] = temp;
						glyph_export->setGlyphGeometriesURLName(temp);
					}
				}
			}
			else if (line_export)
			{
				graphics_json["Type"]="Lines";
			}
			else if (point_export)
			{
				graphics_json["Type"]="Points";
			}
			else
			{
				graphics_json["Type"]="Surfaces";
			}
			i++;
			root.append(graphics_json);
		}

		return Json::StyledWriter().write(root);
	}

	int write_output_string()
	{
		*number_of_entries = get_number_of_entries();
		if (*number_of_entries > 0)
		{
			*output_string = new std::string[*number_of_entries];
			(*output_string)[0] = std::string(get_metadata_string());
			if (isInline == 0)
			{
				int i = 1;
				for (std::map<cmzn_graphics *, Threejs_export *>::iterator export_iter = exports_map.begin();
					export_iter != exports_map.end(); export_iter++)
				{
					Threejs_export_glyph *glyph_export = dynamic_cast<Threejs_export_glyph*>(export_iter->second);
					if (glyph_export)
					{
						(*output_string)[i] = std::string(*glyph_export->getGlyphTransformationExportString());
						i++;
					}
					(*output_string)[i] = std::string(*(export_iter->second->getExportString()));
					i++;
				}
			}
		}
		return 1;
	}

	void clear_exports_map()
	{
		for (std::map<cmzn_graphics *, Threejs_export *>::iterator export_iter = exports_map.begin();
			export_iter != exports_map.end(); export_iter++)
		{
			delete export_iter->second;
		}
		exports_map.clear();
	}

	virtual int cmzn_scene_compile_members(cmzn_scene *scene)
	{
		current_graphics_number = 0;
		if (number_of_time_steps == 0)
		{
			cmzn_scene_compile_graphics(scene, this,/*force_rebuild*/0);
			cmzn_scene_execute(scene);
			write_output_string();
			clear_exports_map();
		}
		else
		{
			int current_time = this->time;
			if (begin_time == end_time || ((morphVertices == 0) && (morphColours == 0) && (morphNormals == 0)))
			{
				this->time = begin_time;
				cmzn_scene_compile_graphics(scene, this,/*force_rebuild*/1);
				cmzn_scene_execute(scene);
			}
			else
			{
				int return_code = 1;
				double increment = 0;
				if (number_of_time_steps > 1)
					increment = (end_time - begin_time) / (double)(number_of_time_steps - 1);
				for (int i = 0; i < number_of_time_steps && return_code; i++)
				{
					this->time = begin_time + i * increment;
					cmzn_scene_compile_graphics(scene, this,/*force_rebuild*/1);
					return_code = cmzn_scene_execute(scene);
					current_time_frame++;
				}
			}
			write_output_string();
			clear_exports_map();
			current_time_frame = 0;
			this->time = current_time;
			cmzn_scene_compile_graphics(scene, this,/*force_rebuild*/1);
		}
		return 1;
	}

	/**
	 * Compile the Graphics_object.
	 */
	int Graphics_object_compile(GT_object *)
	{
		return true;
	}

	/**
	 * Compile the Graphics.
	 */
	int Graphics_compile(cmzn_graphics *graphics)
	{
		return Graphics_object_compile(cmzn_graphics_get_graphics_object(
			graphics));
	}

	/** Method to export individual graphics
	  * @param graphics  Not checked, must be non-NULL. */
	template <class Threejs_export_class>
	int Graphics_export(cmzn_graphics *graphics)
	{
		int return_code = 1;
		GT_object *graphics_object = cmzn_graphics_get_graphics_object(
			graphics);
		Threejs_export_class *threejs_export = 0;
		if (number_of_time_steps == 0 || current_time_frame == 0)
		{
			cmzn_material_id material = cmzn_graphics_get_material(graphics);
			/* non-accessed www*/
			struct Texture *texture = Graphical_material_get_texture(
				material);
			double textureSizes[3] = {0.0, 0.0, 0.0};
			if (texture)
			{
				cmzn_texture_get_texture_coordinate_sizes(texture, 3, textureSizes);
			}
			char *graphics_name = cmzn_graphics_get_name_internal(graphics);
			struct cmzn_region *region = cmzn_scene_get_region_internal(graphics->getScene());
			char *group_name = 0;
			cmzn_field_id groupField = 0;
			groupField = cmzn_graphics_get_subgroup_field(graphics);
			if (groupField)
				group_name = cmzn_field_get_name(groupField);
			char *region_name = cmzn_region_get_name(region);
			char new_file_prefix[50];
			if (region_name)
				sprintf(new_file_prefix, "%s_%s_%s", file_prefix, region_name, graphics_name);
			else
				sprintf(new_file_prefix, "%s_%s", file_prefix, graphics_name);
			const bool morphsColoursAllowed = graphics->dataFieldIsTimeDependent() && morphColours;
			const bool graphicsIsTimeDependent = graphics->coordinateFieldIsTimeDependent()
				|| graphics->pointGlyphScalingIsTimeDependent()
				|| graphics->isoscalarFieldIsTimeDependent()
				|| graphics->subgroupFieldIsTimeDependent();
			const bool morphsVerticesAllowed = graphicsIsTimeDependent && morphVertices;
			const bool morphNormalsAllowed = graphicsIsTimeDependent && morphNormals;
			threejs_export = new Threejs_export_class(new_file_prefix, number_of_time_steps, mode,
				morphsVerticesAllowed, morphsColoursAllowed, morphNormalsAllowed, &textureSizes[0], group_name);
			threejs_export->beginExport();
			threejs_export->exportMaterial(material);
			cmzn_material_destroy(&material);
			DEALLOCATE(graphics_name);
			if (region_name)
				DEALLOCATE(region_name);
			cmzn_field_destroy(&groupField);
			if (group_name)
				DEALLOCATE(group_name);
			exports_map.insert(std::make_pair(graphics, threejs_export));
		}
		else
		{
			std::map<cmzn_graphics *, Threejs_export *>::iterator iter = exports_map.find(graphics);
			if (iter != exports_map.end())
			{
				threejs_export = dynamic_cast<Threejs_export_class*>(iter->second);
			}
		}
		return_code = threejs_export->exportGraphicsObject(graphics_object, current_time_frame);
		if ((number_of_time_steps == 0) || (number_of_time_steps == 1) ||
			(number_of_time_steps - 1 == current_time_frame))
		{
			threejs_export->endExport();
		}
		return return_code;

	}

	/* This will call the appropriate threejs export class based on the graphcis type */
	int Graphics_execute(cmzn_graphics *graphics)
	{
		int return_code = 1;
		GT_object *graphics_object = cmzn_graphics_get_graphics_object(
			graphics);
		if (graphics_object &&
			(GT_object_get_type(graphics_object) == g_SURFACE_VERTEX_BUFFERS))
		{
			return_code = Graphics_export<Threejs_export>(graphics);
		}
		else if (cmzn_graphics_get_type(graphics) == CMZN_GRAPHICS_TYPE_POINTS)
		{
			cmzn_graphicspointattributes_id pointAttr = cmzn_graphics_get_graphicspointattributes(
				graphics);
			if (cmzn_graphicspointattributes_contain_surfaces(pointAttr))
			{
				return_code = Graphics_export<Threejs_export_glyph>(graphics);
			}
			else if ((cmzn_graphicspointattributes_get_glyph_shape_type(pointAttr) ==
					CMZN_GLYPH_SHAPE_TYPE_POINT))
			{
				return_code = Graphics_export<Threejs_export_point>(graphics);
			}
			cmzn_graphicspointattributes_destroy(&pointAttr);
		}
		else if (graphics_object && (GT_object_get_type(graphics_object) == g_POLYLINE_VERTEX_BUFFERS))
		{
			return_code = Graphics_export<Threejs_export_line>(graphics);
		}

		return return_code;
	}

	int cmzn_scene_execute_graphics(cmzn_scene *scene)
	{
		return cmzn_scene_graphics_render_opengl(scene, this);
	}

	int cmzn_scene_execute(cmzn_scene *scene)
	{
		current_graphics_number = current_graphics_number + 1;
		return execute_scene_threejs_output(scene, this);
	}

	int Scene_tree_execute(cmzn_scene *scene)
	{
		if (file_prefix)
		{
			set_Scene(scene);
			return Scene_render_opengl(scene, this);
		}
		return 0;
	}

}; /* class Render_graphics_opengl_threejs */

Render_graphics_opengl *Render_graphics_opengl_create_threejs_renderer(
	const char *file_prefix, int number_of_time_steps, double begin_time,
	double end_time, enum cmzn_streaminformation_scene_io_data_type mode,
	int *number_of_entries, std::string **output_string,
	int morphVertices, int morphColours, int morphNormals,
	int numberOfFiles, char **file_names, int isInline)
{
	return new Render_graphics_opengl_threejs(file_prefix, number_of_time_steps,
		begin_time, end_time, mode, number_of_entries, output_string,
		morphVertices, morphColours, morphNormals, numberOfFiles, file_names, isInline);
}

/**
 * An implementation of a render class that wraps another opengl renderer in
 * compile and then execute stages.
 */
template <class Render_immediate> class Render_graphics_opengl_display_list
	: public Render_immediate
{

public:
	Render_graphics_opengl_display_list() : Render_immediate()
	  {
		Render_immediate::use_display_list = 1;
	  }

	  ~Render_graphics_opengl_display_list()
	  {
	  }

	  int cmzn_scene_execute_graphics(cmzn_scene *scene)
	  {
		  if (this->picking)
			  glPushName(0);
		  int return_code = cmzn_scene_graphics_render_opengl(scene, this);
		  if (this->picking)
			  glPopName();
		  return return_code;
	  }

	  int cmzn_scene_execute_child_scene(cmzn_scene *scene)
	  {
		  return cmzn_scene_render_child_scene(scene, this);
	  }

	  int Graphics_object_execute_parent(GT_object *graphics_object)
	  {
		  return Render_immediate::Graphics_object_execute(graphics_object);
	  }

	  int Graphics_object_compile(GT_object *graphics_object)
	  {
		  int return_code;

		  return_code = Render_immediate::Graphics_object_compile(graphics_object);
		  if (return_code)
		  {
			  Callback_member_callback< GT_object*, Render_graphics_opengl_display_list,
				  int (Render_graphics_opengl_display_list::*)(GT_object*) >
				  execute_method(static_cast<Render_graphics_opengl_display_list*>(this),
				  &Render_graphics_opengl_display_list::Graphics_object_execute_parent);
			  return_code = ::Graphics_object_compile_opengl_display_list(graphics_object,
				  &execute_method, this);
		  }
		  return (return_code);
	  }

	  int Graphics_compile(cmzn_graphics *graphics)
	  {
		  int return_code;

		  GT_object *graphics_object = cmzn_graphics_get_graphics_object(graphics);
		  cmzn_graphics_set_renderer_highlight_functor(graphics, (void *)this);
		  return_code = Render_immediate::Graphics_object_compile(graphics_object);
		  if (return_code)
		  {
			  Callback_member_callback< GT_object*, Render_graphics_opengl_display_list,
				  int (Render_graphics_opengl_display_list::*)(GT_object*) >
				  execute_method(static_cast<Render_graphics_opengl_display_list*>(this),
				  &Render_graphics_opengl_display_list::Graphics_object_execute_parent);
			  return_code = ::Graphics_object_compile_opengl_display_list(graphics_object,
				  &execute_method, this);
		  }
		  cmzn_graphics_remove_renderer_highlight_functor(graphics, (void *)this);
		  return (return_code);
	  }

	  int Graphics_object_execute(GT_object *graphics_object)
	  {
		  return ::Graphics_object_execute_opengl_display_list(graphics_object, this);
	  }

	  int Graphics_execute(cmzn_graphics *graphics)
	  {
		  GT_object *graphics_object = cmzn_graphics_get_graphics_object(graphics);
		  return ::Graphics_object_execute_opengl_display_list(graphics_object, this);
	  }

		virtual void Graphics_object_execute_point_size(GT_object *)
		{
			// do nothing so point size and line width not in display_list
		}

	  int Material_execute_parent(cmzn_material *material)
	  {
		  return Render_immediate::Material_execute(material);
	  }

	  int Material_compile(cmzn_material *material)
	  {
		  int return_code;

		  return_code = Render_immediate::Material_compile(material);
		  if (return_code)
		  {
			  Callback_member_callback< cmzn_material*, Render_graphics_opengl_display_list,
				  int (Render_graphics_opengl_display_list::*)(cmzn_material*) >
				  execute_method(static_cast<Render_graphics_opengl_display_list*>(this),
				  &Render_graphics_opengl_display_list::Material_execute_parent);
			  return_code = Material_compile_opengl_display_list(material,
				  &execute_method, this);
		  }
		  return (return_code);
	  }

	  int Material_execute(cmzn_material *material)
	  {
		  return Material_execute_opengl_display_list(material, this);
	  }

	  int Texture_execute_parent(Texture *texture)
	  {
		  return Render_immediate::Texture_execute(texture);
	  }

	  int Texture_compile(Texture *texture)
	  {
		  int return_code;

		  return_code = Render_immediate::Texture_compile(texture);
		  if (return_code)
		  {
			  Callback_member_callback< Texture*, Render_graphics_opengl_display_list,
				  int (Render_graphics_opengl_display_list::*)(Texture*) >
				  execute_method(static_cast<Render_graphics_opengl_display_list*>(this),
				  &Render_graphics_opengl_display_list::Texture_execute_parent);
			  return_code = Texture_compile_opengl_display_list(texture,
				  &execute_method, this);
		  }
		  return (return_code);
	  }

	  int Texture_execute(Texture *texture)
	  {
		  return Texture_execute_opengl_display_list(texture, this);
	  }

}; /* class Render_graphics_opengl_display_list */

Render_graphics_opengl *Render_graphics_opengl_create_glbeginend_display_list_renderer()
{
	return new Render_graphics_opengl_display_list<Render_graphics_opengl_glbeginend>();
}

Render_graphics_opengl *Render_graphics_opengl_create_client_vertex_arrays_display_list_renderer()
{
	return new Render_graphics_opengl_display_list<Render_graphics_opengl_client_vertex_arrays>();
}

Render_graphics_opengl *Render_graphics_opengl_create_vertex_buffer_object_display_list_renderer()
{
	return new Render_graphics_opengl_display_list<Render_graphics_opengl_vertex_buffer_object>();
}

namespace {

inline char *concatenateLabels(char *label1, char *label2, bool &allocatedText)
{
	char *text;
	if (label1)
	{
		if (label2)
		{
			text = new char[strlen(label1) + strlen(label2) + 1];
			strcpy(text, label1);
			strcat(text, label2);
			allocatedText = true;
		}
		else
		{
			text = label1;
			allocatedText = false;
		}
	}
	else
	{
		text = label2;
		allocatedText = false;
	}
	return text;
}

} // namespace

/** Routine that uses the objects material and spectrum to convert
* an array of data to corresponding colour data.
*/
int Graphics_object_create_colour_buffer_from_data(GT_object *object,
	GLfloat **colour_buffer, unsigned int *colour_values_per_vertex,
	unsigned int *colour_vertex_count)
{
	GLfloat *data_buffer = NULL;
	int return_code = 1;
	unsigned int data_values_per_vertex, data_vertex_count;
	struct cmzn_spectrum *spectrum = 0;

	if (object->vertex_array->get_float_vertex_buffer(
		GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
		&data_buffer, &data_values_per_vertex, &data_vertex_count) &&
		(0 != (spectrum = get_GT_object_spectrum(object))))
	{
		cmzn_material *material = get_GT_object_default_material(object);
		/* Ignoring selected state here so we don't need to refer to primitive */
		if (object->buffer_binding || (object->compile_status == GRAPHICS_NOT_COMPILED))
		{
			if (ALLOCATE(*colour_buffer, GLfloat, 4 * data_vertex_count))
			{
				GLfloat *data_vertex;
				GLfloat *colour_vertex;
				unsigned int i;
				ZnReal colData[4];
				if (!cmzn_spectrum_is_material_overwrite(spectrum))
				{
					Colour diffuse_colour;
					Graphical_material_get_diffuse(material, &diffuse_colour);
					MATERIAL_PRECISION prec;
					Graphical_material_get_alpha(material, &prec);
					GLfloat alpha = prec;
					for (i = 0 ; i < data_vertex_count ; i++)
					{
						colData[0] = diffuse_colour.red;
						colData[1] = diffuse_colour.green;
						colData[2] = diffuse_colour.blue;
						colData[3] = alpha;
					}
				}
				colour_vertex = *colour_buffer;
				data_vertex = data_buffer;
				FE_value *feData = new FE_value[data_values_per_vertex];
				for (i = 0 ; i < data_vertex_count ; i++)
				{
					CAST_TO_FE_VALUE(feData,data_vertex,(int)data_values_per_vertex);
					Spectrum_value_to_rgba(spectrum, data_values_per_vertex,
							feData, colData);
					for (unsigned int j = 0 ; j < 4 ; j++)
					{
						colour_vertex[j] = (GLfloat)colData[j];
					}
					colour_vertex += 4;
					data_vertex += data_values_per_vertex;
				}
				Spectrum_end_value_to_rgba(spectrum);

				*colour_vertex_count = data_vertex_count;
				*colour_values_per_vertex = 4;
				delete[] feData;
			}
			else
			{
				return_code = 0;
			}
		}
	}
	else
	{
		return_code = 0;
	}

	return (return_code);
}

static int Graphics_object_enable_opengl_client_vertex_arrays(GT_object *object,
	Render_graphics_opengl *renderer,
	GLfloat **vertex_buffer, GLfloat **colour_buffer, GLfloat **normal_buffer,
	GLfloat **texture_coordinate0_buffer, GLfloat **tangant_buffer)
{
	int return_code;

	USE_PARAMETER(renderer);
	if (object && object->vertex_array)
	{
		return_code = 1;
		switch (GT_object_get_type(object))
		{
		case g_POLYLINE_VERTEX_BUFFERS:
		case g_SURFACE_VERTEX_BUFFERS:
		case g_GLYPH_SET_VERTEX_BUFFERS:
		case g_POINT_SET_VERTEX_BUFFERS:
			{
				if (object->secondary_material)
				{
					display_message(WARNING_MESSAGE,"Graphics_object_enable_opengl_client_vertex_arrays.  "
						"Multipass rendering not implemented with client vertex arrays.");
				}
				unsigned int position_values_per_vertex, position_vertex_count;
				object->vertex_array->get_float_vertex_buffer(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
					vertex_buffer, &position_values_per_vertex, &position_vertex_count);

				glEnableClientState(GL_VERTEX_ARRAY);
				glVertexPointer(position_values_per_vertex, GL_FLOAT,
					/*Packed vertices*/0, /*Client vertex array*/*vertex_buffer);

				unsigned int colour_values_per_vertex, colour_vertex_count;
				*colour_buffer = (GLfloat *)NULL;
				if (Graphics_object_create_colour_buffer_from_data(object,
					colour_buffer,	&colour_values_per_vertex, &colour_vertex_count))
				{
					if (colour_vertex_count == position_vertex_count)
					{
						glEnableClientState(GL_COLOR_ARRAY);
						glColorPointer(/*must be 4*/4, GL_FLOAT,
							/*Packed vertices*/0, /*Client vertex array*/*colour_buffer);
						glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
						glEnable(GL_COLOR_MATERIAL);
					}
					else
					{
						DEALLOCATE(*colour_buffer);
					}
				}

				*normal_buffer = NULL;
				unsigned int normal_values_per_vertex, normal_vertex_count;
				if (object->vertex_array->get_float_vertex_buffer(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
					normal_buffer, &normal_values_per_vertex, &normal_vertex_count)
					&& (3 == normal_values_per_vertex))
				{
					glEnableClientState(GL_NORMAL_ARRAY);
					glNormalPointer(GL_FLOAT, /*Packed vertices*/0,
						/*Client vertex array*/*normal_buffer);
				}

				*texture_coordinate0_buffer = NULL;
				unsigned int texture_coordinate0_values_per_vertex,
					texture_coordinate0_vertex_count;
				if (object->vertex_array->get_float_vertex_buffer(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_TEXTURE_COORDINATE_ZERO,
					texture_coordinate0_buffer, &texture_coordinate0_values_per_vertex,
					&texture_coordinate0_vertex_count)
					&& (texture_coordinate0_vertex_count == position_vertex_count))
				{
					glClientActiveTexture(GL_TEXTURE0);
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					glTexCoordPointer(texture_coordinate0_values_per_vertex,
						GL_FLOAT, /*Packed vertices*/0,
						/*Client vertex array*/*texture_coordinate0_buffer);
				}
				*tangant_buffer = NULL;
				unsigned int tangent_values_per_vertex,
					tangent_vertex_count;
				if (object->vertex_array->get_float_vertex_buffer(
						GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_TANGENT,
					tangant_buffer, &tangent_values_per_vertex,
					&tangent_vertex_count)
					&& (tangent_vertex_count == position_vertex_count))
				{
					glClientActiveTexture(GL_TEXTURE1);
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					glTexCoordPointer(tangent_values_per_vertex,
						GL_FLOAT, /*Packed vertices*/0,
						/*Client vertex array*/*tangant_buffer);
					glClientActiveTexture(GL_TEXTURE0);
				}
			} break;
		default:
			{
				/* Do nothing, satisfy warnings */
			} break;
		}
	}
	else
	{
		/* Nothing to do */
		return_code = 1;
	}
	return (return_code);
} /* Graphics_object_enable_opengl_client_vertex_arrays */

static int Graphics_object_disable_opengl_client_vertex_arrays(GT_object *object,
	Render_graphics_opengl *renderer,
	GLfloat *vertex_buffer, GLfloat *colour_buffer, GLfloat *normal_buffer,
	GLfloat *texture_coordinate0_buffer, GLfloat *tangent_buffer)
{
	int return_code;

	USE_PARAMETER(renderer);
	if (object && object->vertex_array)
	{
		return_code = 1;
		switch (GT_object_get_type(object))
		{
		case g_POLYLINE_VERTEX_BUFFERS:
		case g_SURFACE_VERTEX_BUFFERS:
		case g_GLYPH_SET_VERTEX_BUFFERS:
		case g_POINT_SET_VERTEX_BUFFERS:
			{
				if (vertex_buffer)
				{
					glDisableClientState(GL_VERTEX_ARRAY);
				}
				if (colour_buffer)
				{
					glDisableClientState(GL_COLOR_ARRAY);
					DEALLOCATE(colour_buffer);
					glDisable(GL_COLOR_MATERIAL);
				}
				if (normal_buffer)
				{
					glDisableClientState(GL_NORMAL_ARRAY);
				}
				if (texture_coordinate0_buffer)
				{
					glClientActiveTexture(GL_TEXTURE0);
					glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				}
				if (tangent_buffer)
				{
					glClientActiveTexture(GL_TEXTURE1);
					glDisableClientState(GL_TEXTURE_COORD_ARRAY);
					glClientActiveTexture(GL_TEXTURE0);
				}
			} break;
		default:
			{
				/* Do nothing, satisfy warnings */
			} break;
		}
	}
	else
	{
		/* Nothing to do */
		return_code = 1;
	}
	return (return_code);
} /* Graphics_object_disable_opengl_client_vertex_arrays */

/* Require glDrawBuffers from OpenGL 2.0 */
#if defined (GL_VERSION_2_0)
/***************************************************************************//**
																			 * Uses the secondary material (which is assumed to write GLfloat colour values) to
																			 * calculate a vertex buffer which will be used as the primitive vertex positions
																			 * when finally actually rendering geometry.
																			 */
static int Graphics_object_generate_vertex_positions_from_secondary_material(GT_object *object,
	Render_graphics_opengl *renderer, GLfloat *position_vertex_buffer,
	unsigned int position_values_per_vertex, unsigned int position_vertex_count)
{
	int return_code;

	/* We will be overriding this so we will need to store the original value */
	GLuint object_position_vertex_buffer_object;
	/* Sizes of our first pass render */
	GLuint tex_width = 0, tex_height = 0;

	renderer->Material_compile(object->secondary_material);

	return_code = 1;
	switch (GT_object_get_type(object))
	{
	case g_POLYLINE_VERTEX_BUFFERS:
		{
			/* Add up the number of vertices that we are calculating,
			* we need a pixel in our view buffer for each vertex. */
			/* All lines must be the same length so that the rendered space is rectangular. */
			unsigned int line_count =
				object->vertex_array->get_number_of_vertices(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START);
			unsigned int line_index, line_length;

			object->vertex_array->get_unsigned_integer_attribute(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
				/*line_index*/0, 1, &line_length);
			for (line_index = 1 ; line_index < line_count ; line_index++)
			{
				unsigned int index_count;
				object->vertex_array->get_unsigned_integer_attribute(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
					line_index, 1, &index_count);
				if (index_count != line_length)
				{
					return_code = 0;
				}
			}

			tex_width = line_length;
			tex_height = line_count;
		} break;
	default:
		{
			/* Do nothing */
		} break;
	}

	if (return_code)
	{
		if (!object->multipass_vertex_buffer_object)
		{
			glGenBuffers(1, &object->multipass_vertex_buffer_object);
		}
		if (!object->multipass_frame_buffer_object)
		{
			glGenFramebuffersEXT(1,&object->multipass_frame_buffer_object);
		}
		if ((object->multipass_width != tex_width)
			|| (object->multipass_height != tex_height))
		{
			GLfloat *temp_position_array, *temp_pointer;
			glBindBuffer(GL_ARRAY_BUFFER, object->multipass_vertex_buffer_object);
			/* Initialise the size to hold the original vertex data and the
			* pass 1 frame buffer positions.
			*/
			glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*
				(position_values_per_vertex + 3)*position_vertex_count,
				NULL, GL_STATIC_DRAW);

			/* Calculate and store the tex1 texture coordinates which are
			* the positions that each vertex must have in the frame buffer. */
			ALLOCATE(temp_position_array, GLfloat,3*position_vertex_count);
			temp_pointer = temp_position_array;
			unsigned int i, j;
			for (j = 0 ; j < tex_height ; j++)
			{
				for (i = 0 ; i < tex_width ; i++)
				{
					//We are rendering a line so the x coordinate covers the
					//full range of the image coordinates.
					*temp_pointer = (GLfloat)i * ((GLfloat)tex_width/(GLfloat)(tex_width - 1));
					temp_pointer++;
					*temp_pointer = (GLfloat)j + 0.5;
					temp_pointer++;
					*temp_pointer = 0.0;
					temp_pointer++;
				}
			}

			/* Set pass 1 frame buffer positions. */
			glBufferSubData(GL_ARRAY_BUFFER,
				/*offset*/sizeof(GLfloat)*position_values_per_vertex*position_vertex_count,
				/*size*/sizeof(GLfloat)*3*position_vertex_count,
				temp_position_array);
			DEALLOCATE(temp_position_array);

			if (object->multipass_frame_buffer_texture)
			{
				glDeleteTextures(1, &object->multipass_frame_buffer_texture);
			}
			object->multipass_frame_buffer_texture = Texture_create_float_texture(
				tex_width, tex_height,
				/*initial_data*/NULL,/*alpha*/1, /*fallback_to_shorts*/1);

			object->multipass_width = tex_width;
			object->multipass_height = tex_height;
		}

		glBindBuffer(GL_ARRAY_BUFFER, object->multipass_vertex_buffer_object);
		glClientActiveTexture(GL_TEXTURE1);
		glTexCoordPointer(/*size*/3, GL_FLOAT, /*Packed vertices*/0,
			/*offset*/(void*)(sizeof(GLfloat)*position_values_per_vertex*position_vertex_count));
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glClientActiveTexture(GL_TEXTURE0);

		/* Load the position vertex buffer into the first half of the array
		* as it will be bound with the standard vertex buffer object
		* rendering code as the position vertex buffer.
		*/
#if !defined (GL_PIXEL_PACK_BUFFER_EXT) && defined (GL_PIXEL_PACK_BUFFER_ARB)
#define GL_PIXEL_PACK_BUFFER_EXT GL_PIXEL_PACK_BUFFER_ARB
#endif

		glBufferSubData(GL_ARRAY_BUFFER,
			/*offset*/0,
			/*size*/sizeof(GLfloat)*position_values_per_vertex*position_vertex_count,
			position_vertex_buffer);
		glBindBuffer(GL_PIXEL_PACK_BUFFER_EXT, object->position_vertex_buffer_object);
		glBufferData(GL_PIXEL_PACK_BUFFER_EXT, sizeof(GLfloat)*
			4*tex_width*tex_height,
			/*position_vertex_buffer*/NULL, GL_STATIC_DRAW);
		object_position_vertex_buffer_object = object->position_vertex_buffer_object;
		object->position_vertex_buffer_object = object->multipass_vertex_buffer_object;
		object->position_values_per_vertex = position_values_per_vertex;

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, object->multipass_frame_buffer_object);

		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D,
			object->multipass_frame_buffer_texture, 0);

#if defined (NEW_CODE)
		/* Could generate more of attributes by adding new framebuffer attachments and copying
		* these to other buffers.  Currently no way to determine which attributes
		* should be set by the secondary material and used in the final render. */
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT, GL_TEXTURE_2D, fbo_tex_normals, 0);
		GLenum dbuffers[] =
		{GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT};
		glDrawBuffers(2, dbuffers);
#endif /* defined (NEW_CODE) */

		GLenum dbuffers[] =
		{GL_COLOR_ATTACHMENT0_EXT};
		glDrawBuffers(1, dbuffers);

		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glDisable(GL_CULL_FACE);
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_STENCIL_TEST);
		glDisable(GL_BLEND);

		// Should not be necessary as we should be writing every pixel,
		// but better than random values when debugging if we aren't.
		glClearColor(1, 0.5, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0.0,tex_width,0.0,tex_height,-1.0,1.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glViewport(0,0,tex_width,tex_height);

		//Not available on my implementation, maybe we should be setting this
		//if available to ensure the results aren't clamped.
		//glClampColorARB(GL_CLAMP_VERTEX_COLOR_ARB, GL_FALSE);

		renderer->Material_execute(object->secondary_material);

		//Render my vertices here, the coordinates must fill in
		//the render buffer so we expect that the vertex position
		//from this stage will be read from the GL_TEXTURE1 multi
		//tex coordinates, so this is what the secondary material
		//must do.  We could override the vertex position but it
		//seems more useful to make this available to the program
		//if necessary.
		switch (GT_object_get_type(object))
		{
		case g_POLYLINE_VERTEX_BUFFERS:
			{
				/* Render lines with the original positions set as the vertex
				* position array, enabling all the OpenGL attributes defined on the
				* original geometry to be used when calculating the final coordinates. */
				render_GT_object_opengl_immediate(object, /*draw_selected*/0,
					renderer, GRAPHICS_OBJECT_RENDERING_TYPE_VERTEX_BUFFER_OBJECT);
				render_GT_object_opengl_immediate(object, /*draw_selected*/1,
					renderer, GRAPHICS_OBJECT_RENDERING_TYPE_VERTEX_BUFFER_OBJECT);
			} break;
			// A simpler (and therefore faster) case would be not to render with
			// the original geometry but use a single quad here instead.  In this
			// case the secondary material would need to calculate all the vertex
			// values required just from these values supplied here and not the original geometry.
#if defined (NEW_CODE)
		case ALL_VALUES_CALCULATED:
			{
				glBegin(GL_QUADS);
				glColor3f(1,1,1);
				glMultiTexCoord4f( GL_TEXTURE0, 0.0, 0.0, 0.0, 1.0);
				glVertex2f(0, 0);
				glMultiTexCoord4f( GL_TEXTURE0, 1.0, 0.0, 0.0, 1.0);
				glVertex2f(tex_width,0);
				glMultiTexCoord4f( GL_TEXTURE0, 1.0, 1.0, 0.0, 1.0);
				glVertex2f(tex_width, tex_height);
				glMultiTexCoord4f( GL_TEXTURE0, 0.0, 1.0, 0.0, 1.0);
				glVertex2f(0, tex_height);
				glEnd();
			} break;
#endif //defined (NEW_CODE)
		default:
			{
				/* Do nothing */
			} break;
		}

		glClientActiveTexture(GL_TEXTURE1);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glClientActiveTexture(GL_TEXTURE0);

		/*Set the vertex buffer object back to the final position buffer */
		object->position_vertex_buffer_object = object_position_vertex_buffer_object;
		object->position_values_per_vertex = 4;

		glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
		glBindBuffer(GL_PIXEL_PACK_BUFFER_EXT, object->position_vertex_buffer_object);
		glReadPixels(0, 0, tex_width, tex_height, GL_RGBA, GL_FLOAT, 0);

#if defined (NEW_CODE)
		/* If we were copying other attributes */
		glReadBuffer(GL_COLOR_ATTACHMENT1_EXT);
		glBindBuffer(GL_PIXEL_PACK_BUFFER_EXT, vbo_normals_handle);
		glReadPixels(0, 0, tex_width, tex_height, GL_RGBA, GL_FLOAT, 0);
#endif /* defined (NEW_CODE) */

		glReadBuffer(GL_NONE);
		glBindBuffer(GL_PIXEL_PACK_BUFFER_EXT, 0);

#if defined (DEBUG_CODE)
		// Read the buffer out to memory so we can see the vertex values.
		{
			//GLfloat debugreadbuffer[4 * tex_width * tex_height];
			GLfloat *debugreadbuffer;
			ALLOCATE(debugreadbuffer, GLfloat, 4 * tex_width * tex_height);
			glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
			glReadPixels(0, 0, tex_width, tex_height, GL_RGBA, GL_FLOAT, debugreadbuffer);
			unsigned int i, j;
			for (i = 0 ; i < tex_height ; i++)
				for (j = 0 ; j < tex_width ; j+=100)
				{
					printf("(%d,%d) %f %f %f %f\n", i, j,
						debugreadbuffer[(i * tex_width + j) * 4 + 0],
						debugreadbuffer[(i * tex_width + j) * 4 + 1],
						debugreadbuffer[(i * tex_width + j) * 4 + 2],
						debugreadbuffer[(i * tex_width + j) * 4 + 3]);
				}
				glReadBuffer(GL_NONE);
				DEALLOCATE(debugreadbuffer);
		}
#endif /* defined (DEBUG_CODE) */

		//Set back to the normal screen
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);


		renderer->Material_execute((cmzn_material *)NULL);
	}

	return (return_code);
} /* Graphics_object_generate_vertex_positions_from_secondary_material */
#endif /* defined (GL_VERSION_2_0) */

/***************************************************************************//**
																			 * Compile Graphics_vertex_array data into vertex buffer objects.
																			 */
static int Graphics_object_compile_opengl_vertex_buffer_object(GT_object *object,
	Render_graphics_opengl *renderer)
{
	int return_code;

	ENTER(Graphics_object_compile_opengl_vertex_buffer_object);

	if (object)
	{
		return_code = 1;
		switch (GT_object_get_type(object))
		{
		case g_GLYPH_SET_VERTEX_BUFFERS:
		{
			GT_glyphset_vertex_buffers *glyph_set = NULL;
			if (object->primitive_lists)
				glyph_set = object->primitive_lists->gt_glyphset_vertex_buffers;
			if (glyph_set)
			{
				if (glyph_set->glyph)
				{
					Graphics_object_compile_opengl_vertex_buffer_object(glyph_set->glyph, renderer);
				}
				if (glyph_set->font)
					cmzn_font_compile(glyph_set->font);
			}
		}
		case g_POLYLINE_VERTEX_BUFFERS:
		case g_SURFACE_VERTEX_BUFFERS:
		case g_POINT_SET_VERTEX_BUFFERS:
			{
				unsigned int *partialRedrawIndices = 0;
				unsigned int partialRedrawIndicesPerVertex, partialRedrawIndicesCount;
				unsigned int *redraw_count_buffer = 0;
				unsigned int redrawPerVertex, redrawCount;
				object->vertex_array->get_unsigned_integer_vertex_buffer(
						GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_PARTIAL_REDRAW,
						&partialRedrawIndices, &partialRedrawIndicesPerVertex,
						&partialRedrawIndicesCount);
				object->vertex_array->get_unsigned_integer_vertex_buffer(
						GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_PARTIAL_REDRAW_COUNT,
						&redraw_count_buffer, &redrawPerVertex,
						&redrawCount);
				GLfloat *position_vertex_buffer = NULL;
				unsigned int position_values_per_vertex, position_vertex_count;
				if (object->vertex_array->get_float_vertex_buffer(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
					&position_vertex_buffer, &position_values_per_vertex,
					&position_vertex_count))
				{
					if (!object->position_vertex_buffer_object)
					{
						object->buffer_binding = 1;
						glGenBuffers(1, &object->position_vertex_buffer_object);
					}

					if (object->secondary_material)
					{
						/* Defer to lower in this function as we may want to use the contents of
						* some of the other buffers in our first pass calculation.
						*/
					}
					else if (object->buffer_binding)
					{
						glBindBuffer(GL_ARRAY_BUFFER, object->position_vertex_buffer_object);
						if (!partialRedrawIndices)
							glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*
									position_values_per_vertex*position_vertex_count,
									position_vertex_buffer, GL_STATIC_DRAW);
						else
						{
							GLintptr bytesStart = 0;
							GLsizeiptr size = 0;
							for (unsigned int i = 0; i < redrawCount; i++)
							{
								bytesStart = sizeof(GLfloat)*position_values_per_vertex*partialRedrawIndices[i];
								size = sizeof(GLfloat)*position_values_per_vertex*redraw_count_buffer[i];
								glBufferSubData(GL_ARRAY_BUFFER, bytesStart, size, &(position_vertex_buffer[partialRedrawIndices[i]*position_values_per_vertex]));
							}
	
						}
						object->position_values_per_vertex = position_values_per_vertex;
					}
				}
				else
				{
					if (object->position_vertex_buffer_object)
					{
						glDeleteBuffers(1, &object->position_vertex_buffer_object);
						object->position_vertex_buffer_object = 0;
					}
				}
				unsigned int colour_values_per_vertex, colour_vertex_count;
				GLfloat *colour_buffer = (GLfloat *)NULL;
				if (Graphics_object_create_colour_buffer_from_data(object,
					&colour_buffer,
					&colour_values_per_vertex, &colour_vertex_count))
				{
					if ((object->buffer_binding || (object->compile_status == GRAPHICS_NOT_COMPILED)) &&
							(colour_vertex_count == position_vertex_count))
					{
						if (!object->colour_vertex_buffer_object)
						{
							glGenBuffers(1, &object->colour_vertex_buffer_object);
						}
						glBindBuffer(GL_ARRAY_BUFFER, object->colour_vertex_buffer_object);
						glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)* /*need to be 4 */4 *colour_vertex_count,
							colour_buffer, GL_STATIC_DRAW);
						object->colour_values_per_vertex = colour_values_per_vertex;
						if (colour_buffer)
						{
							DEALLOCATE(colour_buffer);
						}
					}
				}
				else
				{
					if (colour_buffer)
					{
						DEALLOCATE(colour_buffer);
					}
					if (object->colour_vertex_buffer_object)
					{
						glDeleteBuffers(1, &object->colour_vertex_buffer_object);
						object->colour_vertex_buffer_object = 0;
					}
				}

				GLfloat *normal_buffer = NULL;
				unsigned int normal_values_per_vertex, normal_vertex_count;
				if (object->vertex_array->get_float_vertex_buffer(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
					&normal_buffer, &normal_values_per_vertex, &normal_vertex_count)
					&& (3 == normal_values_per_vertex))
				{
					if (!object->normal_vertex_buffer_object)
					{
						glGenBuffers(1, &object->normal_vertex_buffer_object);
					}
					if (object->buffer_binding)
					{
						glBindBuffer(GL_ARRAY_BUFFER, object->normal_vertex_buffer_object);
						if (!partialRedrawIndices)
							glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*normal_values_per_vertex*normal_vertex_count,
								normal_buffer, GL_STATIC_DRAW);
						else
						{
							GLintptr bytesStart = 0;
							GLsizeiptr size = 0;
							for (unsigned int i = 0; i < redrawCount; i++)
							{
								bytesStart = sizeof(GLfloat)*normal_values_per_vertex*partialRedrawIndices[i];
								size = sizeof(GLfloat)*normal_values_per_vertex*redraw_count_buffer[i];
								glBufferSubData(GL_ARRAY_BUFFER, bytesStart, size, &(normal_buffer[partialRedrawIndices[i]*normal_values_per_vertex]));
							}
						}
					}
				}
				else
				{
					if (object->normal_vertex_buffer_object)
					{
						glDeleteBuffers(1, &object->normal_vertex_buffer_object);
						object->normal_vertex_buffer_object = 0;
					}
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
					if (!object->texture_coordinate0_vertex_buffer_object)
					{
						glGenBuffers(1, &object->texture_coordinate0_vertex_buffer_object);
					}
					if (object->buffer_binding)
					{
						glBindBuffer(GL_ARRAY_BUFFER, object->texture_coordinate0_vertex_buffer_object);
						if (!partialRedrawIndices)
							glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*texture_coordinate0_values_per_vertex*texture_coordinate0_vertex_count,
								texture_coordinate0_buffer, GL_STATIC_DRAW);
						else
						{
							GLintptr bytesStart = 0;
							GLsizeiptr size = 0;
							for (unsigned int i = 0; i < redrawCount; i++)
							{
								bytesStart = sizeof(GLfloat)*texture_coordinate0_values_per_vertex*partialRedrawIndices[i];
								size = sizeof(GLfloat)*texture_coordinate0_values_per_vertex*redraw_count_buffer[i];
								glBufferSubData(GL_ARRAY_BUFFER, bytesStart, size,
									&(texture_coordinate0_buffer[partialRedrawIndices[i]*texture_coordinate0_values_per_vertex]));
							}
						}
						object->texture_coordinate0_values_per_vertex = texture_coordinate0_values_per_vertex;
					}
				}
				else
				{
					if (object->texture_coordinate0_vertex_buffer_object)
					{
						glDeleteBuffers(1, &object->texture_coordinate0_vertex_buffer_object);
						object->texture_coordinate0_vertex_buffer_object = 0;
					}
				}

				GLfloat *tangent_buffer = NULL;
				unsigned int tangent_values_per_vertex,
					tangent_vertex_count;
				if (object->vertex_array->get_float_vertex_buffer(
						GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_TANGENT,
					&tangent_buffer, &tangent_values_per_vertex,
					&tangent_vertex_count) &&
					(tangent_vertex_count == position_vertex_count))
				{
					if (!object->tangent_vertex_buffer_object)
					{
						glGenBuffers(1, &object->tangent_vertex_buffer_object);
					}
					glBindBuffer(GL_ARRAY_BUFFER, object->tangent_vertex_buffer_object);
					if (!partialRedrawIndices)
						glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*tangent_values_per_vertex*tangent_vertex_count,
								tangent_buffer, GL_STATIC_DRAW);
					else
					{
						GLintptr bytesStart = 0;
						GLsizeiptr size = 0;
						for (unsigned int i = 0; i < redrawCount; i++)
						{
							bytesStart = sizeof(GLfloat)*tangent_values_per_vertex*partialRedrawIndices[i];
							size = sizeof(GLfloat)*tangent_values_per_vertex*redraw_count_buffer[i];
							glBufferSubData(GL_ARRAY_BUFFER, bytesStart, size, &(tangent_buffer[partialRedrawIndices[i]*tangent_values_per_vertex]));
						}
					}
					object->tangent_values_per_vertex = tangent_values_per_vertex;
				}
				else
				{
					if (object->tangent_vertex_buffer_object)
					{
						glDeleteBuffers(1, &object->tangent_vertex_buffer_object);
						object->tangent_vertex_buffer_object = 0;
					}
				}

				if ((GT_object_get_type(object) == g_POLYLINE_VERTEX_BUFFERS) &&
					position_vertex_buffer && object->secondary_material)
				{
#if defined (GL_VERSION_2_0) && defined (GL_EXT_framebuffer_object)
					if (Graphics_library_check_extension(GL_ARB_draw_buffers)
						/* Need to check extension to load multitexture functions */
						&& Graphics_library_check_extension(GL_VERSION_1_3)
						&& Graphics_library_check_extension(GL_EXT_framebuffer_object))
					{
						return_code = Graphics_object_generate_vertex_positions_from_secondary_material(
							object, renderer, position_vertex_buffer, position_values_per_vertex,
							position_vertex_count);
					}
					else
					{
						display_message(ERROR_MESSAGE,"Graphics_object_compile_opengl_vertex_buffer_object.  "
							"Multipass rendering not available with this OpenGL driver.");
					}
#else /* defined (GL_VERSION_2_0) && defined (GL_EXT_framebuffer_object) */
					display_message(ERROR_MESSAGE,"Graphics_object_compile_opengl_vertex_buffer_object.  "
						"Multipass rendering not compiled in this version.");
#endif /* defined (GL_VERSION_2_0) && defined (GL_EXT_framebuffer_object) */
				}
				unsigned int *index_vertex_buffer, index_values_per_vertex, index_vertex_count;
				if (object->vertex_array->get_unsigned_integer_vertex_buffer(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_STRIP_INDEX_ARRAY,
					&index_vertex_buffer, &index_values_per_vertex,
					&index_vertex_count))
				{
					if (!object->index_vertex_buffer_object)
					{
						glGenBuffers(1, &object->index_vertex_buffer_object);
					}

					if (object->secondary_material)
					{
						/* Defer to lower in this function as we may want to use the contents of
						* some of the other buffers in our first pass calculation.
						*/
					}
					else if (object->buffer_binding)
					{
						glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object->index_vertex_buffer_object);
						glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*
							index_values_per_vertex*index_vertex_count,
							index_vertex_buffer, GL_STATIC_DRAW);
					}
				}
				else
				{
					if (object->index_vertex_buffer_object)
					{
						glDeleteBuffers(1, &object->index_vertex_buffer_object);
						object->index_vertex_buffer_object = 0;
					}
				}
			} break;
		default:
			{
				/* Do nothing */
			} break;
		}
		GT_object *temp_glyph = GT_object_get_next_object(object);
		if (temp_glyph)
		{
			Graphics_object_compile_opengl_vertex_buffer_object(temp_glyph, renderer);
		}
		object->buffer_binding = 0;
		object->vertex_array->clear_specified_buffer(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_PARTIAL_REDRAW);
		object->vertex_array->clear_specified_buffer(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_PARTIAL_REDRAW_COUNT);
		object->compile_status = GRAPHICS_COMPILED;
	}
	else
	{
		/* Nothing to do */
		return_code = 1;
	}
	return (return_code);
} /* Graphics_object_compile_opengl_vertex_buffer_object */

/***************************************************************************//**
																			 * Set the OpenGL state to use vertex buffer objects that have been previously
																			 * created for this object.
																			 */
static int Graphics_object_enable_opengl_vertex_buffer_object(GT_object *object,
	Render_graphics_opengl *renderer)
{
	int return_code;

	USE_PARAMETER(renderer);
	if (object && object->vertex_array)
	{
		return_code = 1;
		switch (GT_object_get_type(object))
		{
		case g_POLYLINE_VERTEX_BUFFERS:
		case g_SURFACE_VERTEX_BUFFERS:
		case g_GLYPH_SET_VERTEX_BUFFERS:
		case g_POINT_SET_VERTEX_BUFFERS:
			{
				if (object->position_vertex_buffer_object)
				{
					glBindBuffer(GL_ARRAY_BUFFER,
						object->position_vertex_buffer_object);
					glEnableClientState(GL_VERTEX_ARRAY);
					glVertexPointer(
						object->position_values_per_vertex,
						GL_FLOAT, /*Packed vertices*/0,
						/*No offset in vertex array*/(void *)0);
				}
				if (object->colour_vertex_buffer_object)
				{
					glBindBuffer(GL_ARRAY_BUFFER,
						object->colour_vertex_buffer_object);
					glEnableClientState(GL_COLOR_ARRAY);
					glColorPointer(
						/*must be 4*/4,
						GL_FLOAT, /*Packed vertices*/0,
						/*No offset in vertex array*/(void *)0);
					glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
					glEnable(GL_COLOR_MATERIAL);
				}
				if (object->normal_vertex_buffer_object)
				{
					glBindBuffer(GL_ARRAY_BUFFER,
						object->normal_vertex_buffer_object);
					glEnableClientState(GL_NORMAL_ARRAY);
					glNormalPointer(
						GL_FLOAT, /*Packed vertices*/0,
						/*No offset in vertex array*/(void *)0);
				}
				if (object->texture_coordinate0_vertex_buffer_object)
				{
					glClientActiveTexture(GL_TEXTURE0);
					glBindBuffer(GL_ARRAY_BUFFER,
						object->texture_coordinate0_vertex_buffer_object);
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					glTexCoordPointer(
						object->texture_coordinate0_values_per_vertex,
						GL_FLOAT, /*Packed vertices*/0,
						/*No offset in vertex array*/(void *)0);
				}
				if (object->tangent_vertex_buffer_object)
				{
					glClientActiveTexture(GL_TEXTURE1);
					glBindBuffer(GL_ARRAY_BUFFER,
						object->tangent_vertex_buffer_object);
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					glTexCoordPointer(
						object->tangent_values_per_vertex,
						GL_FLOAT, /*Packed vertices*/0,
						/*No offset in vertex array*/(void *)0);
					glClientActiveTexture(GL_TEXTURE0);
				}
				if (object->index_vertex_buffer_object)
				{
					glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object->index_vertex_buffer_object);
				}
			} break;
		default:
			{
				/* Do nothing */
			} break;
		}
	}
	else
	{
		/* Nothing to do */
		return_code = 1;
	}
	return (return_code);
} /* Graphics_object_enable_opengl_client_vertex_arrays */

/***************************************************************************//**
																			 * Reset the OpenGL state to not use vertex buffer objects.
																			 */
static int Graphics_object_disable_opengl_vertex_buffer_object(GT_object *object,
	Render_graphics_opengl *renderer)
{
	int return_code;

	USE_PARAMETER(renderer);
	if (object && object->vertex_array)
	{
		return_code = 1;
		switch (GT_object_get_type(object))
		{
		case g_POLYLINE_VERTEX_BUFFERS:
		case g_SURFACE_VERTEX_BUFFERS:
		case g_GLYPH_SET_VERTEX_BUFFERS:
		case g_POINT_SET_VERTEX_BUFFERS:
			{
				if (object->position_vertex_buffer_object)
				{
					glDisableClientState(GL_VERTEX_ARRAY);
				}
				if (object->colour_vertex_buffer_object)
				{
					glDisableClientState(GL_COLOR_ARRAY);
					glDisable(GL_COLOR_MATERIAL);
				}
				if (object->normal_vertex_buffer_object)
				{
					glDisableClientState(GL_NORMAL_ARRAY);
				}
				if (object->texture_coordinate0_vertex_buffer_object)
				{
					glClientActiveTexture(GL_TEXTURE0);
					glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				}
				if (object->tangent_vertex_buffer_object)
				{
					glClientActiveTexture(GL_TEXTURE1);
					glDisableClientState(GL_TEXTURE_COORD_ARRAY);
					glClientActiveTexture(GL_TEXTURE0);
				}
				/* Reset to normal client vertex arrays */
				glBindBuffer(GL_ARRAY_BUFFER, 0);
			} break;
		default:
			{
				/* Do nothing */
			} break;
		}
	}
	else
	{
		/* Nothing to do */
		return_code = 1;
	}
	return (return_code);
} /* Graphics_object_enable_opengl_client_vertex_arrays */

static int draw_vertexBufferPointset(gtObject *object,
	cmzn_material *material, struct cmzn_spectrum *spectrum,
	Render_graphics_opengl *renderer,
	Graphics_object_rendering_type rendering_type)
{
	if (object && object->vertex_array)
	{
		GT_pointset_vertex_buffers *point_set = object->primitive_lists->gt_pointset_vertex_buffers;
		struct cmzn_font *font = point_set->font;
		std::string *label_buffer = 0;
		unsigned int pointset_index = 0;
		unsigned int pointset_count =
			object->vertex_array->get_number_of_vertices(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START);
		GLfloat *position_buffer = 0, *data_buffer = 0;
		unsigned int position_values_per_vertex = 0, position_vertex_count = 0,
			data_values_per_vertex = 0, data_vertex_count = 0,
			label_per_vertex = 0, label_count = 0;
		if (0<pointset_count)
		{
			object->vertex_array->get_float_vertex_buffer(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
				&position_buffer, &position_values_per_vertex, &position_vertex_count);
			object->vertex_array->get_float_vertex_buffer(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
				&data_buffer, &data_values_per_vertex, &data_vertex_count);
			object->vertex_array->get_string_vertex_buffer(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_LABEL,
				&label_buffer, &label_per_vertex, &label_count);
			struct Spectrum_render_data *render_data = NULL;
			if (data_buffer)
			{
				render_data=spectrum_start_renderGL(spectrum,material,data_values_per_vertex);
			}
			switch (rendering_type)
			{
				case GRAPHICS_OBJECT_RENDERING_TYPE_CLIENT_VERTEX_ARRAYS:
				{
					Graphics_object_enable_opengl_client_vertex_arrays(
						object, renderer,
						&position_buffer, &data_buffer, 0,	0, 0);
				} break;
				case GRAPHICS_OBJECT_RENDERING_TYPE_VERTEX_BUFFER_OBJECT:
				{
					Graphics_object_enable_opengl_vertex_buffer_object(
						object, renderer);
				} break;
				default:
				{
				} break;
			}
			for (pointset_index = 0; pointset_index < pointset_count; pointset_index++)
			{
				unsigned int index_start = 0, index_count = 0;
				object->vertex_array->get_unsigned_integer_attribute(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
					pointset_index, 1, &index_start);
				object->vertex_array->get_unsigned_integer_attribute(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
					pointset_index, 1, &index_count);
				std::string *labels = label_buffer + label_per_vertex * index_start;
				switch (rendering_type)
				{
					case GRAPHICS_OBJECT_RENDERING_TYPE_CLIENT_VERTEX_ARRAYS:
					case GRAPHICS_OBJECT_RENDERING_TYPE_VERTEX_BUFFER_OBJECT:
					{
						glDrawArrays(GL_POINTS, index_start, index_count);
					} break;
					default:
					{
					} break;
				}
				if (labels)
				{
					std::string *label = labels;
					GLfloat *position = position_buffer + position_values_per_vertex * index_start;
					GLfloat x = 0.0, y = 0.0, z = 0.0;
					GLfloat *datum = data_buffer + data_values_per_vertex * index_start;
					for (unsigned int i=0;i<index_count;i++)
					{
						x=position[0];
						y=position[1];
						z=position[2];
						position += 3;
						/* set the spectrum for this datum, if any */
						if (datum)
						{
							spectrum_renderGL_value(spectrum,material,render_data,datum);
							datum += data_values_per_vertex;
						}
						cmzn_font_rendergl_text(font, const_cast<char *>(label->c_str()), x, y, z);
						label++;
					}
				}
			}
			switch (rendering_type)
			{
				case GRAPHICS_OBJECT_RENDERING_TYPE_CLIENT_VERTEX_ARRAYS:
				{
					Graphics_object_disable_opengl_client_vertex_arrays(
						object, renderer,
						position_buffer, data_buffer, 0,
						0, 0);
				} break;
				case GRAPHICS_OBJECT_RENDERING_TYPE_VERTEX_BUFFER_OBJECT:
				{
					Graphics_object_disable_opengl_vertex_buffer_object(
						object, renderer);
				} break;
				default:
				{
				}break;
			}
			if (render_data)
			{
				spectrum_end_renderGL(spectrum,render_data);
			}
		}
		return 1;
	}
	return 0;
}

static int draw_vertexBufferGlyphset(gtObject *object,
	cmzn_material *material, cmzn_material *secondary_material,
	struct cmzn_spectrum *spectrum,
	//int draw_selected, int some_selected,struct Multi_range *selected_name_ranges,
	int draw_selected,
	Render_graphics_opengl *renderer, bool &lighting_on,
	Graphics_object_rendering_type rendering_type, bool pick_object_id)
	/*******************************************************************************
	LAST MODIFIED : 22 November 2005

	DESCRIPTION :
	Draws graphics object <glyph> at <number_of_points> points given by the
	positions in <point_list> and oriented and scaled by <axis1_list>, <axis2_list>
	and <axis3_list>, each axis additionally scaled by its value in <scale_list>.
	If the glyph is part of a linked list through its nextobject
	member, these attached glyphs are also executed.
	Writes the <labels> array strings, if supplied, beside each glyph point.
	If <names> are supplied these identify each point/glyph for OpenGL picking.
	If <draw_selected> is set, then only those <names> in <selected_name_ranges>
	are drawn, otherwise only those names not there are drawn.
	If <some_selected> is true, <selected_name_ranges> indicates the points that
	are selected, or all points if <selected_name_ranges> is NULL.
	@param no_lighting  Flag giving current state of whether lighting is off.
	Enable or disable lighting and update as appropriate for point/line or surface
	glyphs to minimise state changes.
	==============================================================================*/
{
	GLfloat transformation[16];
	int draw_all, name_selected = 0, return_code = 1;
	struct Spectrum_render_data *render_data = NULL;
	Triple temp_axis1, temp_axis2, temp_axis3, temp_point;
	SubObjectGroupHighlightFunctor *highlight_functor = renderer->highlight_functor;

	if (object && object->vertex_array)
	{
		unsigned int nodeset_index = 0;
		unsigned int nodeset_count =
			object->vertex_array->get_number_of_vertices(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START);
		GT_glyphset_vertex_buffers *glyph_set = object->primitive_lists->gt_glyphset_vertex_buffers;
		struct GT_object *glyph = glyph_set->glyph;
		GLfloat *position_buffer = 0, *data_buffer = 0, *axis1_buffer = 0,
			*axis2_buffer = 0, *axis3_buffer = 0, *scale_buffer = 0, *normal_buffer = 0,
			*texture_coordinate0_buffer = 0, *label_bound_buffer = 0;
		std::string *label_buffer = 0;
		int *names_buffer = 0;
		unsigned int position_values_per_vertex = 0, position_vertex_count = 0,
			data_values_per_vertex = 0, data_vertex_count = 0, axis1_values_per_vertex = 0,
			axis1_vertex_count = 0, axis2_values_per_vertex = 0, axis2_vertex_count = 0,
			axis3_values_per_vertex = 0, axis3_vertex_count = 0, scale_values_per_vertex = 0,
			scale_vertex_count = 0, names_count = 0, names_per_vertex = 0,
			label_bounds_per_vertex = 0, label_bounds_count = 0,
			label_per_vertex, label_count = 0;
		if (0 < nodeset_count)
		{
			object->vertex_array->get_float_vertex_buffer(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
				&position_buffer, &position_values_per_vertex, &position_vertex_count);
			object->vertex_array->get_float_vertex_buffer(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
				&data_buffer, &data_values_per_vertex, &data_vertex_count);
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
			object->vertex_array->get_integer_vertex_buffer(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_VERTEX_ID,
				&names_buffer, &names_per_vertex, &names_count);
			object->vertex_array->get_float_vertex_buffer(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_LABEL_BOUND,
				&label_bound_buffer, &label_bounds_per_vertex, &label_bounds_count);
			object->vertex_array->get_string_vertex_buffer(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_LABEL,
				&label_buffer, &label_per_vertex, &label_count);
		}
		if ((0 == nodeset_count) || ((0 < nodeset_count) && ((glyph && position_buffer &&
			axis1_buffer && axis2_buffer && axis3_buffer && scale_buffer) || (!glyph && label_buffer))))
		{
			if ((0==nodeset_count) ||
				(draw_selected&&((!names_buffer) || (!highlight_functor))))
			{
				/* nothing to draw */
				return_code=1;
			}
			else
			{
#if defined (OPENGL_API)
				unsigned int label_density_values_per_vertex = 0,label_density_count = 0;
				GLfloat *label_density_buffer = 0;
				object->vertex_array->get_float_vertex_buffer(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_LABEL_DENSITY,
					&label_density_buffer, &label_density_values_per_vertex, &label_density_count);
				bool picking_names = (names_buffer) && (renderer->picking);
				draw_all = ((!names_buffer) || ((!draw_selected)&&(!highlight_functor)));
				if (data_buffer)
				{
					render_data=spectrum_start_renderGL(spectrum,material,data_values_per_vertex);
				}
				// disable highlighting beneath glyph set level
				renderer->push_highlight_functor();
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
						*scale = scale_buffer + scale_values_per_vertex * index_start,
						*datum = data_buffer + data_values_per_vertex * index_start,
						*label_density = label_density_buffer + label_density_values_per_vertex * index_start,
						*label_bounds = label_bound_buffer + label_bounds_per_vertex * index_start;
					std::string *labels = label_buffer + label_per_vertex * index_start;
					std::string *label = labels;
					int object_name = 0;
					object->vertex_array->get_integer_attribute(
						GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_OBJECT_ID,
						nodeset_index, 1, &object_name);
					int *names = names_buffer + names_per_vertex * index_start;
					if (renderer->picking && pick_object_id)
					{
						/* put out name for picking - cast to GLuint */
						glLoadName((GLuint)object_name);
					}
					if (picking_names)
					{
						/* make space for picking name on name stack */
						glPushName(0);
					}
					GT_object *glyph = glyph_set->glyph;
					cmzn_glyph_repeat_mode glyph_repeat_mode = glyph_set->glyph_repeat_mode;

					cmzn_glyph_shape_type glyph_type = glyph ?
						GT_object_get_glyph_type(glyph) : CMZN_GLYPH_SHAPE_TYPE_NONE;
					//TO BE ENABLED:
					// output static labels at each point, if supplied
					const int number_of_glyphs =
						cmzn_glyph_repeat_mode_get_number_of_glyphs(glyph_repeat_mode);
					char **static_labels = 0;
					for (int glyph_number = 0; glyph_number < number_of_glyphs; ++glyph_number)
					{
						if (cmzn_glyph_repeat_mode_glyph_number_has_label(glyph_repeat_mode, glyph_number) &&
							(0 != glyph_set->static_label_text[glyph_number]))
						{
							static_labels = glyph_set->static_label_text;
							break;
						}
					}
//					/* try to draw points and lines faster */
					if ((glyph_type == CMZN_GLYPH_SHAPE_TYPE_POINT) &&
						(glyph_repeat_mode == CMZN_GLYPH_REPEAT_MODE_NONE))
					{
						switch (rendering_type)
						{
							case GRAPHICS_OBJECT_RENDERING_TYPE_CLIENT_VERTEX_ARRAYS:
							{
								Graphics_object_enable_opengl_client_vertex_arrays(
									object, renderer,
									&position_buffer, &data_buffer, &normal_buffer,
									&texture_coordinate0_buffer, 0);
							} break;
							case GRAPHICS_OBJECT_RENDERING_TYPE_VERTEX_BUFFER_OBJECT:
							{
								Graphics_object_enable_opengl_vertex_buffer_object(
									object, renderer);
							} break;
							default:
							{
							} break;
						}
						if (lighting_on)
						{
							/* disable lighting so rendered in flat diffuse colour */
							glDisable(GL_LIGHTING);
							lighting_on = !lighting_on;
						}
						if ((object_name >= 0) && highlight_functor)
						{
							name_selected=highlight_functor->call(object_name);
						}
						glMatrixMode(GL_MODELVIEW);
						for (unsigned i=0;i<index_count;i++)
						{
							if ((object_name < 0) && ((names) && highlight_functor))
							{
								name_selected = highlight_functor->call(*names);
							}
							if (draw_all||(name_selected&&draw_selected)||((!name_selected)&&(!draw_selected)))
							{
								/* set the spectrum for this datum, if any */
								if (data_buffer)
								{
									spectrum_renderGL_value(spectrum,material,render_data,datum);
								}
								if (picking_names)
								{
									glLoadName((GLuint)(*names));
								}
								glPushMatrix();
								resolve_glyph_axes(glyph_repeat_mode, /*glyph_number*/0,
									glyph_set->base_size, glyph_set->scale_factors, glyph_set->offset,
									position, axis1, axis2, axis3, scale,
									temp_point, temp_axis1, temp_axis2, temp_axis3);
								/* make transformation matrix for manipulating glyph */
								transformation[ 0] = 1.0;
								transformation[ 1] = 0.0;
								transformation[ 2] = 0.0;
								transformation[ 3] = 0.0;
								transformation[ 4] = 0.0;
								transformation[ 5] = 1.0;
								transformation[ 6] = 0.0;
								transformation[ 7] = 0.0;
								transformation[ 8] = 0.0;
								transformation[ 9] = 0.0;
								transformation[10] = 1.0;
								transformation[11] = 0.0;
								transformation[12] = glyph_set->offset[0];
								transformation[13] = glyph_set->offset[1];
								transformation[14] = glyph_set->offset[2];
								transformation[15] = 1.0;
								glMultMatrixf(transformation);
								switch (rendering_type)
								{
									case GRAPHICS_OBJECT_RENDERING_TYPE_GLBEGINEND:
									{
										glBegin(GL_POINTS);
										glVertex3fv(temp_point);
										glEnd();
									} break;
									case GRAPHICS_OBJECT_RENDERING_TYPE_CLIENT_VERTEX_ARRAYS:
									case GRAPHICS_OBJECT_RENDERING_TYPE_VERTEX_BUFFER_OBJECT:
									{
										glDrawArrays(GL_POINTS, index_start+i, 1);
									}break;
									default:
									{
									} break;
								}
								glPopMatrix();

								if (label || static_labels)
								{
									Triple lpoint;
									for (int j = 0; j < 3; ++j)
									{
										lpoint[j] = temp_point[j]+ glyph_set->label_offset[0]*temp_axis1[j] +
											glyph_set->label_offset[1]*temp_axis2[j] + glyph_set->label_offset[2]*temp_axis3[j];
									}
									bool allocatedText = false;
									char *text = concatenateLabels(static_labels ? static_labels[0] : 0, label ? const_cast<char *>(label->c_str()) : 0, allocatedText);
									cmzn_font_rendergl_text(glyph_set->font, text, lpoint[0], lpoint[1], lpoint[2]);
									if (allocatedText)
									{
										delete[] text;
									}
								}
							}
							position += position_values_per_vertex;
							axis1 += axis1_values_per_vertex;
							axis2 += axis2_values_per_vertex;
							axis3 += axis3_values_per_vertex;
							scale += scale_values_per_vertex;
							if (data_buffer)
							{
								datum += data_values_per_vertex;
							}
							if (names_buffer)
							{
								names += names_per_vertex;
							}
							if (labels)
							{
								label++;
							}
						}
						switch (rendering_type)
						{
							case GRAPHICS_OBJECT_RENDERING_TYPE_CLIENT_VERTEX_ARRAYS:
							{
								Graphics_object_disable_opengl_client_vertex_arrays(
									object, renderer,
									position_buffer, data_buffer, normal_buffer,
									texture_coordinate0_buffer, 0);
							} break;
							case GRAPHICS_OBJECT_RENDERING_TYPE_VERTEX_BUFFER_OBJECT:
							{
								Graphics_object_disable_opengl_vertex_buffer_object(
									object, renderer);
							} break;
							default:
							{
							} break;
						}
					}
					else
					{
						if (!lighting_on)
						{
							/* enable lighting for general glyphs */
							glEnable(GL_LIGHTING);
							lighting_on = !lighting_on;
						}
						/* must push and pop the modelview matrix */
						glMatrixMode(GL_MODELVIEW);
						if ((object_name >= 0) && highlight_functor)
						{
							name_selected=highlight_functor->call(object_name);
						}
						if (glyph)
						{
							Graphics_object_glyph_labels_function glyph_labels_function =
								Graphics_object_get_glyph_labels_function(glyph);
							for (unsigned int i = 0; i < index_count; i++)
							{
								if ((object_name < 0) && names && highlight_functor)
								{
									name_selected = highlight_functor->call(*names);
								}
								if (draw_all||(name_selected&&draw_selected)||((!name_selected)&&(!draw_selected)))
								{
									if (picking_names)
									{
										glLoadName((GLuint)(*names));
									}
									/* set the spectrum for this datum, if any */
									if (datum)
									{
										spectrum_renderGL_value(spectrum,material,render_data,datum);
									}
									for (int j = 0; j < number_of_glyphs; j++)
									{
										/* store the current modelview matrix */
										glPushMatrix();
										resolve_glyph_axes(glyph_repeat_mode, /*glyph_number*/j,
											glyph_set->base_size, glyph_set->scale_factors, glyph_set->offset,
											position, axis1, axis2, axis3, scale,
											temp_point, temp_axis1, temp_axis2, temp_axis3);

										/* make transformation matrix for manipulating glyph */
										transformation[ 0] = temp_axis1[0];
										transformation[ 1] = temp_axis1[1];
										transformation[ 2] = temp_axis1[2];
										transformation[ 3] = 0.0;
										transformation[ 4] = temp_axis2[0];
										transformation[ 5] = temp_axis2[1];
										transformation[ 6] = temp_axis2[2];
										transformation[ 7] = 0.0;
										transformation[ 8] = temp_axis3[0];
										transformation[ 9] = temp_axis3[1];
										transformation[10] = temp_axis3[2];
										transformation[11] = 0.0;
										transformation[12] = temp_point[0];
										transformation[13] = temp_point[1];
										transformation[14] = temp_point[2];
										transformation[15] = 1.0;
										glMultMatrixf(transformation);
										renderer->Graphics_object_execute(glyph);
										if (glyph_labels_function)
										{
											Triple label_scale;
											for (int j = 0; j < 3; ++j)
											{
												label_scale[j] = glyph_set->base_size[j] + glyph_set->scale_factors[j]*(scale)[j];
											}
											ZnReal *realField = 0;
											if (label_bounds_per_vertex > 0 && label_bounds)
											{
												realField = new ZnReal[label_bounds_per_vertex];
												CAST_TO_OTHER(realField,label_bounds,ZnReal,(int)label_bounds_per_vertex);
											}
											if (label_density)
											{
												Triple label_density_triple;
												label_density_triple[0] = 0;
												label_density_triple[1] = 0;
												label_density_triple[2] = 0;
												for (unsigned int i = 0; i < label_density_values_per_vertex; i++)
												{
													label_density_triple[i] = label_density[i];
												}
												return_code = (*glyph_labels_function)(&label_scale,
													glyph_set->label_bounds_dimension, glyph_set->label_bounds_components, realField,
													&label_density_triple, material, secondary_material, glyph_set->font, renderer);
											}
											else
											{
												return_code = (*glyph_labels_function)(&label_scale,
													glyph_set->label_bounds_dimension, glyph_set->label_bounds_components, realField,
													0, material, secondary_material, glyph_set->font, renderer);
											}
										}
										/* restore the original modelview matrix */
										glPopMatrix();
									}
								}
								/* advance pointers */
								position += position_values_per_vertex;
								axis1 += axis1_values_per_vertex;
								axis2 += axis2_values_per_vertex;
								axis3 += axis3_values_per_vertex;
								scale += scale_values_per_vertex;
								if (datum)
								{
									datum += data_values_per_vertex;
								}
								if (label_density)
								{
									label_density += label_density_values_per_vertex;
								}
								if (names)
								{
									names += names_per_vertex;
								}
								if (label_bounds)
								{
									label_bounds += label_bounds_per_vertex;
								}
							}
						}
						/* output label at each point, if supplied */
						if ((labels || static_labels) && !label_bound_buffer)
						{
							/* Default is to draw the label value at the origin */
							names = names_buffer + names_per_vertex * index_start;
							if (lighting_on)
							{
								/* disable lighting so rendered in flat diffuse colour */
								glDisable(GL_LIGHTING);
								lighting_on = false;
							}
							position = position_buffer + position_values_per_vertex * index_start;
							datum = data_buffer + data_values_per_vertex * index_start;
							axis1 = axis1_buffer + axis1_values_per_vertex * index_start;
							axis2 = axis2_buffer + axis2_values_per_vertex * index_start;
							axis3 = axis3_buffer + axis3_values_per_vertex * index_start;
							scale = scale_buffer + scale_values_per_vertex * index_start;
							datum = data_buffer + data_values_per_vertex * index_start;
							if ((object_name >= 0) && highlight_functor)
							{
								name_selected=highlight_functor->call(object_name);
							}
							for (unsigned int i = 0; i < index_count; i++)
							{
								if ((object_name < 0) && names && highlight_functor)
								{
									name_selected = highlight_functor->call(*names);
								}
								if ((static_labels || label) && (draw_all || ((name_selected) && draw_selected)
									|| ((!name_selected)&&(!draw_selected))))
								{
									if (picking_names)
									{
										glLoadName((GLuint)(*names));
									}
									/* set the spectrum for this datum, if any */
									if (datum)
									{
										spectrum_renderGL_value(spectrum,material,render_data,datum);
									}
									for (int glyph_number = 0; glyph_number < number_of_glyphs; glyph_number++)
									{
										if (cmzn_glyph_repeat_mode_glyph_number_has_label(glyph_repeat_mode, glyph_number) &&
											((labels && (glyph_number == 0) && label) || (static_labels && static_labels[glyph_number])))
										{
											resolve_glyph_axes(glyph_repeat_mode, glyph_number,
												glyph_set->base_size, glyph_set->scale_factors, glyph_set->offset,
												position, axis1, axis2, axis3, scale,
												temp_point, temp_axis1, temp_axis2, temp_axis3);
											if ((glyph_repeat_mode == CMZN_GLYPH_REPEAT_MODE_MIRROR) && ((scale)[0] < 0.0f))
											{
												for (int j = 0; j < 3; ++j)
												{
													temp_point[j] += (
														(1.0 - glyph_set->label_offset[0])*temp_axis1[j] +
														glyph_set->label_offset[1]*temp_axis2[j] +
														glyph_set->label_offset[2]*temp_axis3[j]);
												}
											}
											else
											{
												for (int j = 0; j < 3; ++j)
												{
													temp_point[j] += (
														glyph_set->label_offset[0]*temp_axis1[j] +
														glyph_set->label_offset[1]*temp_axis2[j] +
														glyph_set->label_offset[2]*temp_axis3[j]);
												}
											}
											bool allocatedText = false;
											char *text = concatenateLabels(
												static_labels ? static_labels[glyph_number] : 0,
												(label && (glyph_number == 0)) ? const_cast<char *>(label->c_str()) : 0, allocatedText);
											cmzn_font_rendergl_text(glyph_set->font, text, temp_point[0], temp_point[1], temp_point[2]);
											if (allocatedText)
											{
												delete[] text;
											}
										}
									}
								}
								/* advance pointers */
								position += position_values_per_vertex;
								axis1 += axis1_values_per_vertex;
								axis2 += axis2_values_per_vertex;
								axis3 += axis3_values_per_vertex;
								scale += scale_values_per_vertex;
								if (datum)
								{
									datum += data_values_per_vertex;
								}
								if (label_density)
								{
									label_density += label_density_values_per_vertex;
								}
								if (names)
								{
									names += names_per_vertex;
								}
								if (label)
								{
									label++;
								}
							}
						}
					}
					if (picking_names)
					{
						/* free space for point number on picking name stack */
						glPopName();
					}
					return_code=1;
				}
				renderer->pop_highlight_functor();
				if (data_buffer)
				{
					spectrum_end_renderGL(spectrum,render_data);
				}
#endif /* defined (OPENGL_API) */
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"draw_glyphsetGL. Invalid argument(s)");
		return_code=0;
	}

	return (return_code);
} /* draw_glyphsetGL */

int drawGLSurfaces(gtObject *object, Render_graphics_opengl *renderer,
	union GT_primitive_list *primitive_list, bool picking_names,
	Graphics_object_rendering_type rendering_type, struct cmzn_spectrum *spectrum,
	cmzn_material *material, int draw_selected)
{
	int return_code = 1;
	if (object && renderer && primitive_list)
	{
		int name_selected = 0;
		GT_surface_vertex_buffers *vb_surface = primitive_list->gt_surface_vertex_buffers;
		struct Graphics_vertex_array *array = object->vertex_array;
		if (vb_surface)
		{
			GLenum mode = g_TRIANGLE;
			switch (vb_surface->surface_type)
			{
			case g_SHADED:
			case g_SHADED_TEXMAP:
			{
				/*
				if (object->texture_tiling &&
					(0 < array->get_number_of_vertices(
						GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_TEXTURE_COORDINATE_ZERO)))
				{
					struct Graphics_vertex_array *tile_array =
						tile_GT_surface_vertex_buffer(vb_surface, array, object->texture_tiling);
					mode = GL_TRIANGLES;
				}
				else
				{
					mode = GL_TRIANGLE_STRIP;
				}
				*/
				mode = GL_TRIANGLE_STRIP;
			} break;
			case g_SH_DISCONTINUOUS:
			case g_SH_DISCONTINUOUS_STRIP:
			case g_SH_DISCONTINUOUS_TEXMAP:
			case g_SH_DISCONTINUOUS_STRIP_TEXMAP:
			{
				mode = GL_TRIANGLES;
			} break;
			default:
			{
			} break;
			}
			if (picking_names)
			{
				glPushName(0);
			}
			bool wireframe_flag = (vb_surface->render_polygon_mode == CMZN_GRAPHICS_RENDER_POLYGON_MODE_WIREFRAME);
			if (wireframe_flag)
			{
				glPushAttrib(GL_POLYGON_BIT);
				glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
			}
			struct Spectrum_render_data *spectrum_render_data = NULL;
			return_code = 1;
			unsigned int surface_index;
			unsigned int surface_count =
				array->get_number_of_vertices(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START);
			GLfloat *position_buffer = 0, *data_buffer = 0, *normal_buffer = 0,
				*texture_coordinate0_buffer = 0, *tangent_buffer = 0;
			unsigned int position_values_per_vertex, position_vertex_count,
			data_values_per_vertex, data_vertex_count, normal_values_per_vertex,
			normal_vertex_count, texture_coordinate0_values_per_vertex,
			texture_coordinate0_vertex_count, tangent_values_per_vertex, tangent_vertex_count;
			unsigned int *index_vertex_buffer, index_values_per_vertex, index_vertex_count;
			object->vertex_array->get_unsigned_integer_vertex_buffer(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_STRIP_INDEX_ARRAY,
				&index_vertex_buffer, &index_values_per_vertex,
				&index_vertex_count);

			switch (rendering_type)
			{
			case GRAPHICS_OBJECT_RENDERING_TYPE_GLBEGINEND:
			{
				array->get_float_vertex_buffer(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
					&position_buffer, &position_values_per_vertex, &position_vertex_count);
				array->get_float_vertex_buffer(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
					&data_buffer, &data_values_per_vertex, &data_vertex_count);
				if (data_buffer)
				{
					spectrum_render_data=spectrum_start_renderGL(spectrum,material,data_values_per_vertex);
				}

				array->get_float_vertex_buffer(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
					&normal_buffer, &normal_values_per_vertex, &normal_vertex_count);
				array->get_float_vertex_buffer(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_TEXTURE_COORDINATE_ZERO,
					&texture_coordinate0_buffer, &texture_coordinate0_values_per_vertex,
					&texture_coordinate0_vertex_count);
				array->get_float_vertex_buffer(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_TANGENT,
					&tangent_buffer, &tangent_values_per_vertex,
					&tangent_vertex_count);

				if (object->secondary_material)
				{
					display_message(WARNING_MESSAGE,"render_GT_object_opengl_immediate.  "
						"Multipass rendering not implemented with glbegin/glend rendering.");
				}
			} break;
			case GRAPHICS_OBJECT_RENDERING_TYPE_CLIENT_VERTEX_ARRAYS:
			{
				Graphics_object_enable_opengl_client_vertex_arrays(
					object, renderer,
					&position_buffer, &data_buffer, &normal_buffer,
					&texture_coordinate0_buffer, &tangent_buffer);
			} break;
			case GRAPHICS_OBJECT_RENDERING_TYPE_VERTEX_BUFFER_OBJECT:
			{
				Graphics_object_enable_opengl_vertex_buffer_object(
					object, renderer);
			} break;
			}
			for (surface_index = 0; surface_index < surface_count; surface_index++)
			{
				int object_name = 0;
				if (!array->get_integer_attribute(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_OBJECT_ID,
					surface_index, 1, &object_name))
				{
					/* no object name, draw only glyphs provide no object id */
					object_name = 0;
				}
				if (object_name > -1)
				{
					/* work out if subobjects selected */
					if (renderer->highlight_functor)
					{
						name_selected=(renderer->highlight_functor)->call(object_name);
					}
					else
					{
						name_selected = 0;
					}
					if ((name_selected&&draw_selected)||
						((!name_selected)&&(!draw_selected)))
					{
						if (picking_names)
						{
							/* put out name for picking - cast to GLuint */
							glLoadName((GLuint)object_name);
						}
						unsigned int index_start, index_count;
						GLfloat *position_vertex = NULL;

						array->get_unsigned_integer_attribute(
							GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
							surface_index, 1, &index_start);
						array->get_unsigned_integer_attribute(
							GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
							surface_index, 1, &index_count);
						unsigned int npts1, npts2;
						array->get_unsigned_integer_attribute(
							GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_XI1,
							surface_index, 1, &npts1);
						array->get_unsigned_integer_attribute(
							GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_XI2,
							surface_index, 1, &npts2);
						switch (rendering_type)
						{
							case GRAPHICS_OBJECT_RENDERING_TYPE_GLBEGINEND:
							{
								switch (mode)
								{
									case GL_TRIANGLE_STRIP:
									{
										unsigned int number_of_strips = 0;
										unsigned int strip_start = 0;
										object->vertex_array->get_unsigned_integer_attribute(
											GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_STRIPS,
											surface_index, 1, &number_of_strips);
										object->vertex_array->get_unsigned_integer_attribute(
											GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_STRIP_START,
											surface_index, 1, &strip_start);
										GLfloat *current_position = position_buffer;
										for (unsigned int i = 0; i < number_of_strips; i++)
										{
											unsigned int points_per_strip = 0;
											unsigned int index_start_for_strip = 0;
											object->vertex_array->get_unsigned_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_STRIP_INDEX_START,
												strip_start+i, 1, &index_start_for_strip);
											object->vertex_array->get_unsigned_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_POINTS_FOR_STRIP,
												strip_start+i, 1, &points_per_strip);
											unsigned int *indices = &index_vertex_buffer[index_start_for_strip];
											glBegin(GL_TRIANGLE_STRIP);
											for (unsigned int j = 0; j < points_per_strip; j++)
											{
												current_position = &(position_buffer[*indices * position_values_per_vertex]);
												glVertex3fv(current_position);
												indices++;
											}
											glEnd();
										}
									} break;
									case GL_TRIANGLES:
									{
										position_vertex = position_buffer + position_values_per_vertex * index_start;
										glBegin(GL_TRIANGLES);
										for (unsigned int j = 0; j < index_count; j++)
										{
											glVertex3fv(position_vertex);
											position_vertex += position_values_per_vertex;
										}
										glEnd();
									}break;
								}
								return_code=1;
							} break;
							case GRAPHICS_OBJECT_RENDERING_TYPE_CLIENT_VERTEX_ARRAYS:
							case GRAPHICS_OBJECT_RENDERING_TYPE_VERTEX_BUFFER_OBJECT:
							{
								switch (mode)
								{
									case GL_TRIANGLE_STRIP:
									{
										unsigned int number_of_strips = 0;
										unsigned int strip_start = 0;
										array->get_unsigned_integer_attribute(
											GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_STRIPS,
											surface_index, 1, &number_of_strips);
										array->get_unsigned_integer_attribute(
											GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_STRIP_START,
											surface_index, 1, &strip_start);
										for (unsigned int i = 0; i < number_of_strips; i++)
										{
											unsigned int points_per_strip = 0;
											unsigned int index_start_for_strip = 0;
											array->get_unsigned_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_STRIP_INDEX_START,
												strip_start+i, 1, &index_start_for_strip);
											array->get_unsigned_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_POINTS_FOR_STRIP,
												strip_start+i, 1, &points_per_strip);
											if (object->index_vertex_buffer_object)
											{
												glDrawElements(mode, points_per_strip, GL_UNSIGNED_INT, BUFFER_OFFSET(sizeof(GLuint) * index_start_for_strip));
											}
										}
									} break;
									case GL_TRIANGLES:
									{
										glDrawArrays(mode, index_start, index_count);
									}break;
									default:
									{
										/* Do nothing, satisfy warnings */
									} break;
								} break;
							}
						}
					}
				}
			}
			switch (rendering_type)
			{
				case GRAPHICS_OBJECT_RENDERING_TYPE_GLBEGINEND:
				{
					if (data_buffer)
					{
						spectrum_end_renderGL(spectrum, spectrum_render_data);
					}
				} break;
				case GRAPHICS_OBJECT_RENDERING_TYPE_CLIENT_VERTEX_ARRAYS:
				{
					Graphics_object_disable_opengl_client_vertex_arrays(
						object, renderer,
						position_buffer, data_buffer, normal_buffer,
						texture_coordinate0_buffer, tangent_buffer);
				} break;
				case GRAPHICS_OBJECT_RENDERING_TYPE_VERTEX_BUFFER_OBJECT:
				{
					Graphics_object_disable_opengl_vertex_buffer_object(
						object, renderer);
				}
			}

			if (wireframe_flag)
			{
				glPopAttrib();
			}
			if (picking_names)
			{
				glPopName();
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,"drawGLSurface.  Missing surfaces.");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"drawGLSurface.  Invalid arguments.");
		return_code=0;
	}
	return return_code;
}

int draw_vertexBufferLine(gtObject *object, Render_graphics_opengl *renderer,
	union GT_primitive_list *primitive_list, bool picking_names,
	Graphics_object_rendering_type rendering_type, struct cmzn_spectrum *spectrum,
	cmzn_material *material, int draw_selected)
{
	int return_code = 1;
	GT_polyline_vertex_buffers *line = primitive_list->gt_polyline_vertex_buffers;
	GLenum mode = GL_LINE_STRIP;
	switch (line->polyline_type)
	{
	case g_PLAIN:
	case g_NORMAL:
		{
			mode = GL_LINE_STRIP;
		} break;
	case g_PLAIN_DISCONTINUOUS:
	case g_NORMAL_DISCONTINUOUS:
		{
			mode = GL_LINES;
		} break;
	default:
		{
			display_message(ERROR_MESSAGE,
				"render_GT_object_opengl_immediate.  Invalid line type");
			return_code=0;
		} break;
	}
	if (return_code)
	{
		unsigned int line_index;
		unsigned int line_count = object->vertex_array->get_number_of_vertices(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START);
		GLfloat *position_buffer, *data_buffer, *normal_buffer,
			*texture_coordinate0_buffer;
		struct Spectrum_render_data *render_data = NULL;
		unsigned int position_values_per_vertex, position_vertex_count,
			data_values_per_vertex, data_vertex_count, normal_values_per_vertex,
			normal_vertex_count, texture_coordinate0_values_per_vertex,
			texture_coordinate0_vertex_count;

		position_buffer = (GLfloat *)NULL;
		data_buffer = (GLfloat *)NULL;
		normal_buffer = (GLfloat *)NULL;
		texture_coordinate0_buffer = (GLfloat *)NULL;

		switch (rendering_type)
		{
		case GRAPHICS_OBJECT_RENDERING_TYPE_GLBEGINEND:
			{
				object->vertex_array->get_float_vertex_buffer(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
					&position_buffer, &position_values_per_vertex, &position_vertex_count);

				object->vertex_array->get_float_vertex_buffer(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
					&data_buffer, &data_values_per_vertex, &data_vertex_count);
				if (data_buffer)
				{
					render_data=spectrum_start_renderGL
						(spectrum,material,data_values_per_vertex);
				}

				object->vertex_array->get_float_vertex_buffer(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
					&normal_buffer, &normal_values_per_vertex, &normal_vertex_count);
				object->vertex_array->get_float_vertex_buffer(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_TEXTURE_COORDINATE_ZERO,
					&texture_coordinate0_buffer, &texture_coordinate0_values_per_vertex,
					&texture_coordinate0_vertex_count);

				if (object->secondary_material)
				{
					display_message(WARNING_MESSAGE,"render_GT_object_opengl_immediate.  "
						"Multipass rendering not implemented with glbegin/glend rendering.");
				}
			} break;
		case GRAPHICS_OBJECT_RENDERING_TYPE_CLIENT_VERTEX_ARRAYS:
			{
				Graphics_object_enable_opengl_client_vertex_arrays(
					object, renderer,
					&position_buffer, &data_buffer, &normal_buffer,
					&texture_coordinate0_buffer, 0);
			} break;
		case GRAPHICS_OBJECT_RENDERING_TYPE_VERTEX_BUFFER_OBJECT:
			{
				Graphics_object_enable_opengl_vertex_buffer_object(
					object, renderer);
			} break;
		}

		for (line_index = 0; line_index < line_count; line_index++)
		{
			int object_name = 0;
			if (!object->vertex_array->get_integer_attribute(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_OBJECT_ID,
				line_index, 1, &object_name))
			{
				/* no object name, draw only glyphs provide no object id */
				object_name = 0;
			}
			if (object_name > -1)
			{
				/* work out if subobjects selected */
				int name_selected = 0;
				if (renderer->highlight_functor)
				{
					name_selected=(renderer->highlight_functor)->call(object_name);
				}
				else
				{
					name_selected = 0;
				}
				if ((name_selected&&draw_selected)||
					((!name_selected)&&(!draw_selected)))
				{
					if (picking_names)
					{
						/* put out name for picking - cast to GLuint */
						glLoadName((GLuint)object_name);
					}
					unsigned int i, index_start, index_count;
					GLfloat *position_vertex = NULL, *data_vertex = NULL, *normal_vertex = NULL,
						*texture_coordinate0_vertex = NULL;

					object->vertex_array->get_unsigned_integer_attribute(
						GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
						line_index, 1, &index_start);
					object->vertex_array->get_unsigned_integer_attribute(
						GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
						line_index, 1, &index_count);

					switch (rendering_type)
					{
						case GRAPHICS_OBJECT_RENDERING_TYPE_GLBEGINEND:
						{
							position_vertex = position_buffer +
								position_values_per_vertex * index_start;
							if (data_buffer)
							{
								data_vertex = data_buffer +
									data_values_per_vertex * index_start;
							}
							if (normal_buffer)
							{
								normal_vertex = normal_buffer +
									normal_values_per_vertex * index_start;
							}
							if (texture_coordinate0_buffer)
							{
								texture_coordinate0_vertex = texture_coordinate0_buffer +
									texture_coordinate0_values_per_vertex * index_start;
							}

							glBegin(mode);
							for (i=index_count;i>0;i--)
							{
								if (data_buffer)
								{
									spectrum_renderGL_value(spectrum,material,render_data,data_vertex);
									data_vertex += data_values_per_vertex;
								}
								if (normal_buffer)
								{
									glNormal3fv(normal_vertex);
									normal_vertex += normal_values_per_vertex;
								}
								if (texture_coordinate0_buffer)
								{
									glTexCoord3fv(texture_coordinate0_vertex);
									texture_coordinate0_vertex += texture_coordinate0_values_per_vertex;
								}
								glVertex3fv(position_vertex);
								position_vertex += position_values_per_vertex;
							}
							glEnd();
						} break;
						case GRAPHICS_OBJECT_RENDERING_TYPE_CLIENT_VERTEX_ARRAYS:
						case GRAPHICS_OBJECT_RENDERING_TYPE_VERTEX_BUFFER_OBJECT:
						{
							glDrawArrays(mode, index_start, index_count);
						} break;
					}
				}
			}
		}
		switch (rendering_type)
		{
		case GRAPHICS_OBJECT_RENDERING_TYPE_GLBEGINEND:
			{
				if (data_buffer)
				{
					spectrum_end_renderGL(spectrum, render_data);
				}
			} break;
		case GRAPHICS_OBJECT_RENDERING_TYPE_CLIENT_VERTEX_ARRAYS:
			{
				Graphics_object_disable_opengl_client_vertex_arrays(
					object, renderer,
					position_buffer, data_buffer, normal_buffer,
					texture_coordinate0_buffer, 0);
			} break;
		case GRAPHICS_OBJECT_RENDERING_TYPE_VERTEX_BUFFER_OBJECT:
			{
				Graphics_object_disable_opengl_vertex_buffer_object(
					object, renderer);
			}
		}
	}
	return return_code;
}

static int render_GT_object_opengl_immediate(gtObject *object,
	int draw_selected, Render_graphics_opengl *renderer,
	Graphics_object_rendering_type rendering_type)
{
	ZnReal proportion = 0.0,*times;
	bool picking_names;
	int itime, number_of_times, return_code;
#if defined (OPENGL_API)
	bool lighting_on = true;
#endif /* defined (OPENGL_API) */
	cmzn_material *material, *secondary_material;
	struct cmzn_spectrum *spectrum;
	union GT_primitive_list *primitive_list1 = NULL, *primitive_list2 = NULL;

	ENTER(render_GT_object_opengl_immediate)
		if (object)
		{
			return_code = 1;
			spectrum=get_GT_object_spectrum(object);
			/* determine if picking names are to be output */
			picking_names = renderer->picking &&
				(CMZN_GRAPHICS_SELECT_MODE_OFF != GT_object_get_select_mode(object));
			/* determine which material to use */
			if (draw_selected)
			{
				material = get_GT_object_selected_material(object);
			}
			else
			{
				material = get_GT_object_default_material(object);
			}
			secondary_material = get_GT_object_secondary_material(object);
			number_of_times = GT_object_get_number_of_times(object);
			if (0 < number_of_times)
			{
				itime = number_of_times;
				if ((itime > 1) && (times = object->times))
				{
					itime--;
					times += itime;
					if (renderer->time >= *times)
					{
						proportion = 0;
					}
					else
					{
						while ((itime>0)&&(renderer->time < *times))
						{
							itime--;
							times--;
						}
						if (renderer->time< *times)
						{
							proportion=0;
						}
						else
						{
							proportion=times[1]-times[0];
							if (proportion>0)
							{
								proportion=(renderer->time-times[0])/proportion;
							}
							else
							{
								proportion=0;
							}
						}
					}
				}
				else
				{
					itime = 0;
					proportion = 0;
				}
				if (object->primitive_lists &&
					(primitive_list1 = object->primitive_lists + itime))
				{
					if (proportion > 0)
					{
						if (!(primitive_list2 = object->primitive_lists + itime + 1))
						{
							display_message(ERROR_MESSAGE,
								"render_GT_object_opengl_immediate.  Invalid primitive_list");
							return_code = 0;
						}
					}
					else
					{
						primitive_list2 = (union GT_primitive_list *)NULL;
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"render_GT_object_opengl_immediate.  Invalid primitive_lists");
					return_code = 0;
				}
			}
			if ((0 < number_of_times) && return_code)
			{
				switch (GT_object_get_type(object))
				{
				case g_POINT_SET_VERTEX_BUFFERS:
				{
					GT_pointset_vertex_buffers *point_set = object->primitive_lists->gt_pointset_vertex_buffers;
					if (point_set)
					{
#if defined (OPENGL_API)
						/* disable lighting so rendered in flat diffuse colour */
						/*???RC glPushAttrib and glPopAttrib are *very* slow */
						glPushAttrib(GL_ENABLE_BIT);
						glDisable(GL_LIGHTING);
#endif /* defined (OPENGL_API) */
						draw_vertexBufferPointset(object, material, spectrum, renderer, rendering_type);
#if defined (OPENGL_API)
								/* restore previous lighting state */
						glPopAttrib();
#endif /* defined (OPENGL_API) */
						return_code=1;
					}
					else
					{
						display_message(ERROR_MESSAGE,"render_GT_object_opengl_immediate.  Missing point");
						return_code=0;
					}
				} break;
				case g_GLYPH_SET_VERTEX_BUFFERS:
				{
					GT_glyphset_vertex_buffers *glyph_set = object->primitive_lists->gt_glyphset_vertex_buffers;
					if (glyph_set)
					{
#if defined (OPENGL_API)
						// push enable bit as lighting is switched off and on depending on
						// whether glyph is points or lines vs. surfaces, whether labels are drawn
						glPushAttrib(GL_ENABLE_BIT);
						/* store the transform attribute group to save current matrix mode
						and GL_NORMALIZE flag. */
						glPushAttrib(GL_TRANSFORM_BIT);
						/* Must enable GL_NORMALIZE so that normals are normalized after
						scaling and shear by the transformations in the glyph set -
						otherwise lighting will be wrong. Since this may reduce
						performance, only enable for glyph_sets. */
						glEnable(GL_NORMALIZE);
						if (picking_names)
						{
							glPushName(0);
						}
#endif /* defined (OPENGL_API) */

						draw_vertexBufferGlyphset(object,
							material, secondary_material, spectrum,
							//int draw_selected, int some_selected,struct Multi_range *selected_name_ranges,
							(draw_selected > 0 ),
							renderer, lighting_on, rendering_type, picking_names);
#if defined (OPENGL_API)
						if (picking_names)
						{
							glPopName();
						}
						/* restore the transform attribute group */
						glPopAttrib();
						/* restore previous lighting state */
						glPopAttrib();
#endif /* defined (OPENGL_API) */
					}
					return_code=1;
				} break;
					case g_POLYLINE_VERTEX_BUFFERS:
					{
						GT_polyline_vertex_buffers *line = primitive_list1->gt_polyline_vertex_buffers;
						if (line)
						{
							if (lighting_on && ((g_PLAIN == line->polyline_type) ||
								(g_PLAIN_DISCONTINUOUS == line->polyline_type)))
							{
								/* disable lighting so rendered in flat diffuse colour */
								/*???RC glPushAttrib and glPopAttrib are *very* slow */
								glPushAttrib(GL_ENABLE_BIT);
								glDisable(GL_LIGHTING);
								lighting_on = false;
							}
							if (picking_names)
							{
								glPushName(0);
							}
							draw_vertexBufferLine(object, renderer, primitive_list1, picking_names,
								rendering_type, spectrum, material, draw_selected);
							return_code = 1;

							if (picking_names)
							{
								glPopName();
							}
							if (!lighting_on)
							{
								/* restore previous lighting state */
								glPopAttrib();
							}
						}
						else
						{
							/*???debug*/printf("! render_GT_object_opengl_immediate.  Missing line");
							display_message(ERROR_MESSAGE,"render_GT_object_opengl_immediate.  Missing line");
							return_code=0;
						}
					} break;
					case g_SURFACE_VERTEX_BUFFERS:
					{
						return_code = drawGLSurfaces(object, renderer, primitive_list1, picking_names,
							rendering_type, spectrum, material, draw_selected);
					} break;
				default:
					{
						display_message(ERROR_MESSAGE,"render_GT_object_opengl_immediate.  Invalid object type");
						return_code=0;
					} break;
				}
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,"render_GT_object_opengl_immediate.  Missing object");
			return_code=0;
		}
		LEAVE;

		return (return_code);
} /* render_GT_object_opengl_immediate */

static int Graphics_object_compile_members_opengl(GT_object *graphics_object_list,
	Render_graphics_opengl *renderer)
{
	int return_code;
	struct GT_object *graphics_object;

	if (graphics_object_list)
	{
		return_code = 1;
		for (graphics_object=graphics_object_list;graphics_object != NULL;
			graphics_object=graphics_object->nextobject)
		{
			if (GRAPHICS_COMPILED != graphics_object->compile_status)
			{
				/* compile components of graphics objects first */
				if (graphics_object->default_material)
				{
					renderer->texture_tiling = NULL;
					renderer->allow_texture_tiling = 1;
					renderer->Material_compile(graphics_object->default_material);

					/* The geometry needs to be updated if these are different. */
					if (renderer->texture_tiling || graphics_object->texture_tiling)
					{
						REACCESS(Texture_tiling)(&graphics_object->texture_tiling,
							renderer->texture_tiling);
						graphics_object->compile_status = GRAPHICS_NOT_COMPILED;
					}

					renderer->allow_texture_tiling = 0;
					renderer->texture_tiling = NULL;
				}
				if (graphics_object->selected_material)
				{
					renderer->Material_compile(graphics_object->selected_material);
				}
				if (graphics_object->secondary_material)
				{
					renderer->Material_compile(graphics_object->secondary_material);
				}
				switch (graphics_object->object_type)
				{
				case g_GLYPH_SET_VERTEX_BUFFERS:
					{
						struct GT_glyphset_vertex_buffers *glyph_set;

						if (graphics_object->primitive_lists)
						{
							glyph_set = graphics_object->primitive_lists->gt_glyphset_vertex_buffers;
							if (glyph_set)
							{
								if (glyph_set->glyph)
									renderer->Graphics_object_compile(glyph_set->glyph);
								if (glyph_set->font)
									cmzn_font_compile(glyph_set->font);
							}
						}
					} break;
				case g_POINT_SET_VERTEX_BUFFERS:
					{
						struct GT_pointset_vertex_buffers *point_set;

						if (graphics_object->primitive_lists)
						{
							point_set = graphics_object->primitive_lists->gt_pointset_vertex_buffers;
							if (point_set)
							{
								if (point_set->font)
								{
									cmzn_font_compile(point_set->font);
								}
							}
						}
					} break;
				default:
					{
						/* Do nothing, satisfy warnings */
					} break;
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Graphics_object_compile_members.  Invalid argument(s)");
		return_code = 0;
	}

	return (return_code);
} /* Graphics_object_compile_members */

/**
 * Compile the display list for this object.
 */
static int Graphics_object_compile_opengl_display_list(GT_object *graphics_object_list,
	Callback_base< GT_object * > *execute_function,
	Render_graphics_opengl *renderer)
{
	int return_code;
	struct GT_object *graphics_object;
	USE_PARAMETER(renderer);
	if (graphics_object_list)
	{
		return_code = 1;
		for (graphics_object=graphics_object_list;graphics_object != NULL;
			graphics_object=graphics_object->nextobject)
		{
			if (GRAPHICS_COMPILED != graphics_object->compile_status)
			{
				if (GRAPHICS_NOT_COMPILED == graphics_object->compile_status)
				{
					if (graphics_object->display_list ||
						(graphics_object->display_list=glGenLists(1)))
					{
						glNewList(graphics_object->display_list,GL_COMPILE);
						(*execute_function)(graphics_object);
						glEndList();
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"Graphics_object_compile_opengl_display_list.  Unable to get display list");
						return_code = 0;
					}
				}
				if (return_code)
				{
					graphics_object->compile_status = GRAPHICS_COMPILED;
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Graphics_object_compile_opengl_display_list.  Invalid argument(s)");
		return_code = 0;
	}
	return (return_code);
}

/**
 * Execute the display list for this object.
 */
static int Graphics_object_execute_opengl_display_list(GT_object *graphics_object_list,
	Render_graphics_opengl *renderer)
{
	int return_code;
	if (graphics_object_list && renderer)
	{
		return_code = 1;
		for (struct GT_object *graphics_object=graphics_object_list;graphics_object != NULL;
			graphics_object=graphics_object->nextobject)
		{
			// execute point size and line width now since not in display list
			renderer->Render_graphics_opengl::Graphics_object_execute_point_size(graphics_object);
			if (GRAPHICS_COMPILED == graphics_object->compile_status)
			{
				glCallList(graphics_object->display_list);
			}
			else
			{
				display_message(ERROR_MESSAGE,"Graphics_object_execute_opengl_display_list.  Graphics object not compiled.");
				return_code = 0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Graphics_object_execute_opengl_display_list.  Invalid argument(s)");
		return_code = 0;
	}
	return (return_code);
}

static int Graphics_object_render_opengl(
struct GT_object *graphics_object,
	Render_graphics_opengl *renderer, Graphics_object_rendering_type rendering_type)
{
	int graphics_object_no,return_code;
	struct GT_object *graphics_object_item;

	if (graphics_object)
	{
		return_code=1;
		if (renderer->picking && (NULL != graphics_object->nextobject))
		{
			glPushName(0);
		}
		graphics_object_no=0;
		for (graphics_object_item=graphics_object;graphics_object_item != NULL;
			graphics_object_item=graphics_object_item->nextobject)
		{
			renderer->Graphics_object_execute_point_size(graphics_object_item);
			if (renderer->picking && (0<graphics_object_no))
			{
				glLoadName((GLuint)graphics_object_no);
			}
			graphics_object_no++;

			if ((CMZN_GRAPHICS_SELECT_MODE_ON == graphics_object_item->select_mode) ||
				(CMZN_GRAPHICS_SELECT_MODE_DRAW_SELECTED == graphics_object_item->select_mode))
			{
				if (graphics_object_item->selected_material)
				{
					if (renderer->highlight_functor)
					{
						renderer->Material_execute(
							graphics_object_item->selected_material);
						render_GT_object_opengl_immediate(graphics_object_item,
							/*draw_selected*/1, renderer, rendering_type);
						renderer->Material_execute((cmzn_material *)NULL);
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,"compile_GT_object.  "
						"Graphics object %s has no selected material",
						graphics_object_item->name);
				}
			}
			// there is no highlight_functor when picking, but draw_selected graphics need to be pickable
			if ((CMZN_GRAPHICS_SELECT_MODE_DRAW_SELECTED != graphics_object_item->select_mode) || (!renderer->highlight_functor))
			{
				if (graphics_object_item->default_material)
				{
					renderer->Material_execute(graphics_object_item->default_material);
				}
				render_GT_object_opengl_immediate(graphics_object_item,
					/*draw_selected*/0, renderer, rendering_type);
				if (graphics_object_item->default_material)
				{
					renderer->Material_execute((cmzn_material *)NULL);
				}
			}
		}
		if (renderer->picking && (NULL != graphics_object->nextobject))
		{
			glPopName();
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"execute_GT_object.  Invalid argument(s)");
		return_code=0;
	}

	return (return_code);
} /* execute_GT_object */


