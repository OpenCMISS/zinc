/***************************************************************************//**
 * render_to_triangularisation.hpp
 * OpenGL rendering calls
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (RENDER_TRIANGULARISATION_HPP)
#define RENDER_TRIANGULARISATION_HPP

#include "graphics/graphics_object.h"
#include "graphics/render.hpp"
#include "graphics/scene_coordinate_system.hpp"
#include "graphics/triangle_mesh.hpp"

struct Graphics_buffer;

/**
 * Renderer that converts graphics to triangles e.g. for output to STL.
 */
class Render_graphics_triangularisation : public Render_graphics_compile_members
{
public:
	Graphics_buffer *graphics_buffer;	
	Triangle_mesh *trimesh;

public:
	Render_graphics_triangularisation(Graphics_buffer *graphics_buffer, GLfloat tolerance) :
		graphics_buffer(graphics_buffer), trimesh(new Triangle_mesh(tolerance))
	{
	}

	virtual ~Render_graphics_triangularisation();

	/***************************************************************************//**
	 * Execute the Graphics_object.
	 */
	virtual int Graphics_object_execute(GT_object *)
	{
		return 1;
	}

	/***************************************************************************//**
	 * Compile the Graphics_object.
	 */
	virtual int Graphics_compile(cmzn_graphics *)
	{
		return 1;
	}

	virtual int Graphics_execute(cmzn_graphics *)
	{
		return 1;
	}

	/***************************************************************************//**
	 * Compile the Graphics_object.
	 */
	virtual int Graphics_object_compile(GT_object *)
	{
		return 1;
	}

	virtual int cmzn_scene_execute(cmzn_scene *scene)
	{
		USE_PARAMETER(scene);
		return 1;
	}

	virtual int cmzn_scene_execute_graphics(cmzn_scene *scene)
	{
		USE_PARAMETER(scene);
		return 1;
	}
	
	virtual int cmzn_scene_execute_child_scene(cmzn_scene *scene)
	{
		USE_PARAMETER(scene);
		return 1;
	}

	/***************************************************************************//**
	 * Render the Graphics_object.  Typically as the graphics_object is temporary
	 * this method suggests to renderers that compile and then render that this object
	 * should instead render now.
	 */
	virtual int Graphics_object_render_immediate(GT_object *graphics_object)
	{
		USE_PARAMETER(graphics_object);
		return 1;
	}

	/***************************************************************************//**
	 * Compile the Material.
	 */
	virtual int Material_compile(cmzn_material *material)
	{
		USE_PARAMETER(material);
		return 1;
	}
	
	/***************************************************************************//**
	 * Execute the Material.
	 */
	virtual int Material_execute(cmzn_material *material)
	{
		USE_PARAMETER(material);
		return 1;
	}
	
	/***************************************************************************//**
	 * Execute the Texture.
	 */
	virtual int Texture_execute(Texture *texture)
	{
		USE_PARAMETER(texture);
		return 1;
	}

	/** Not relevant to renderer */
	virtual void Light_model_enable(Colour&, bool, bool)
	{
	}

	/** Not relevant to renderer */
	virtual void Light_model_disable()
	{
	}
	
	/***************************************************************************//**
	 * Execute the cmzn_light.
	 */
	virtual int cmzn_light_execute(cmzn_light *light)
	{
		USE_PARAMETER(light);
		return 1;
	}

	virtual int Scene_tree_execute(cmzn_scene *scene);

	/** Only draw graphics in world and local coordinates. Not fully implemented */
	virtual int begin_coordinate_system(enum cmzn_scenecoordinatesystem coordinate_system)
	{
		return !cmzn_scenecoordinatesystem_is_window_relative(coordinate_system);
	}

	/** Not fully implemented */
	virtual void end_coordinate_system(enum cmzn_scenecoordinatesystem /*coordinate_system*/)
	{
	}

	Triangle_mesh *get_triangle_mesh();
	
}; /* class Render_graphics */

int render_scene_triangularisation(cmzn_scene_id scene,
	cmzn_scenefilter_id filter, Triangle_mesh *trimesh);

#endif /* !defined (RENDER_TRIANGULARISATION_HPP) */
