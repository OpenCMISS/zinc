/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>

#include "opencmiss/zinc/mesh.h"
#include "opencmiss/zinc/nodeset.h"
#include "opencmiss/zinc/scenefilter.h"
#include "opencmiss/zinc/status.h"
#include "finite_element/finite_element_region.h"
#include "general/debug.h"
#include "general/object.h"
#include "graphics/graphics_library.h"
#include "graphics/render_gl.h"
#include "graphics/scene.hpp"
#include "graphics/scene_picker.hpp"
#include "graphics/scene_viewer.h"
#include "graphics/scene.hpp"
#include "interaction/interaction_volume.h"
#include "mesh/cmiss_element_private.hpp"
#include "mesh/cmiss_node_private.hpp"
#include "region/cmiss_region.hpp"

#define SELECT_BUFFER_SIZE_INCREMENT 10000

cmzn_scenepicker::cmzn_scenepicker(cmzn_scenefiltermodule_id filter_module_in) :
	interaction_volume(0),
	top_scene(0),
	scene_viewer(0),
	centre_x(0), centre_y(0),
	size_x(0), size_y(0),
	coordinate_system(CMZN_SCENECOORDINATESYSTEM_WINDOW_PIXEL_TOP_LEFT),
	filter(0),
	select_buffer(0),
	select_buffer_size(10000),
	number_of_hits(0),
	filter_module(cmzn_scenefiltermodule_access(filter_module_in)),
	access_count(1)
{
}

cmzn_scenepicker::~cmzn_scenepicker()
{
	if (interaction_volume)
		DEACCESS(Interaction_volume)(&interaction_volume);
	if (scene_viewer)
		cmzn_sceneviewer_destroy(&scene_viewer);
	if (top_scene)
		cmzn_scene_destroy(&top_scene);
	if (filter)
		cmzn_scenefilter_destroy(&filter);
	if (select_buffer)
		DEALLOCATE(select_buffer);
	if (filter_module)
		cmzn_scenefiltermodule_destroy(&filter_module);
}

void cmzn_scenepicker::updateViewerRectangle()
{
	if (scene_viewer)
	{
		if (interaction_volume)
			DEACCESS(Interaction_volume)(&interaction_volume);
		GLdouble temp_modelview_matrix[16], temp_projection_matrix[16];
		double viewport_bottom,viewport_height, viewport_left,viewport_width,
			viewport_pixels_per_unit_x, viewport_pixels_per_unit_y;
		int i, j, width, height;
		for (i=0;i<4;i++)
		{
			for (j=0;j<4;j++)
			{
				temp_modelview_matrix[i*4+j] =
					scene_viewer->modelview_matrix[j*4+i];
				temp_projection_matrix[i*4+j] =
					scene_viewer->window_projection_matrix[j*4+i];
			}
		}

		Scene_viewer_get_viewport_info(scene_viewer,
			&viewport_left, &viewport_bottom, &viewport_pixels_per_unit_x,
			&viewport_pixels_per_unit_y);
		Scene_viewer_get_viewport_size(scene_viewer,	&width, &height);
		viewport_width  = ((double)width)/viewport_pixels_per_unit_x;
		viewport_height = ((double)height)/viewport_pixels_per_unit_y;
		/* recalculate the centre of y as windows coordinate is different from the
		 * GL coordinate */
		double new_centre_y = viewport_height - (double)(centre_y) - 1.0;
		interaction_volume = create_Interaction_volume_ray_frustum(
			temp_modelview_matrix, temp_projection_matrix,
			viewport_left, viewport_bottom, viewport_width, viewport_height,
			centre_x, new_centre_y, size_x, size_y);
	}
}

int cmzn_scenepicker::getPickingVolumeCentre(double *coordinateValuesOut3)
{
	updateViewerRectangle();
	if (interaction_volume)
	{
		double normalised_point[3];
		normalised_point[0]=0.0;
		normalised_point[1]=0.0;
		normalised_point[2]=0.0;
		Interaction_volume_normalised_to_model_coordinates(
			interaction_volume,normalised_point,coordinateValuesOut3);
		return CMZN_OK;
	}
	return CMZN_ERROR_GENERAL;
}

int cmzn_scenepicker::pickObjects()
{
	double modelview_matrix[16],projection_matrix[16];
	GLdouble opengl_modelview_matrix[16],opengl_projection_matrix[16];
	int i, j, return_code = CMZN_ERROR_GENERAL;
	updateViewerRectangle();
	if (select_buffer != NULL)
		return CMZN_OK;
	if (!has_current_context())
		return CMZN_ERROR_GENERAL;
	if (top_scene&&interaction_volume)
	{
		Render_graphics_opengl *renderer = Render_graphics_opengl_create_glbeginend_renderer();
		// Minimal incremental build to avoid locking up with big graphics
		// This means can only pick what's visible now
		// Keeping Scene_compile to ensure all objects correctly built for OpenGL
		GraphicsIncrementalBuild incrementalBuild(0);
		renderer->setIncrementalBuild(&incrementalBuild);
		renderer->picking = 1;
		if (renderer->Scene_compile(top_scene, filter))
		{
			number_of_hits=-1;
			while (0>number_of_hits)
			{
				if (ALLOCATE(select_buffer,GLuint,select_buffer_size))
				{
					Interaction_volume_get_modelview_matrix(interaction_volume,
						modelview_matrix);
					Interaction_volume_get_projection_matrix(interaction_volume,
						projection_matrix);
					/* transpose projection matrix for OpenGL */
					for (i=0;i<4;i++)
					{
						for (j=0;j<4;j++)
						{
							opengl_modelview_matrix[j*4+i] = modelview_matrix[i*4+j];
							opengl_projection_matrix[j*4+i] = projection_matrix[i*4+j];
						}
					}
					renderer->set_world_view_matrix(opengl_modelview_matrix);

					glSelectBuffer(select_buffer_size,select_buffer);
					glRenderMode(GL_SELECT);
					glMatrixMode(GL_PROJECTION);
					glLoadIdentity();
					glMultMatrixd(opengl_projection_matrix);
					glMatrixMode(GL_MODELVIEW);
					glLoadIdentity();
					glMultMatrixd(opengl_modelview_matrix);
					/* set an arbitrary viewport - not really needed
						   SAB 22 July 2004 This is causing the view frustrums
						   to not match when picking, so instead I am not changing the
						   viewport, so presumably the last rendered viewport is OK. */
					/* glViewport(0,0,1024,1024); */
					glDepthRange((GLclampd)0,(GLclampd)1);
					{
						do
						{
							return_code = renderer->Scene_tree_execute(top_scene);
						}
						while (return_code && renderer->next_layer());
					}
					glFlush();
					number_of_hits=glRenderMode(GL_RENDER);
					if (0<=number_of_hits)
					{
						return_code=CMZN_OK;
					}
					else
					{
						/* select buffer overflow; enlarge and repeat */
						select_buffer_size += SELECT_BUFFER_SIZE_INCREMENT;
						DEALLOCATE(select_buffer);
					}
				}
			}
		}

		delete renderer;
	}
	return return_code;
}

void cmzn_scenepicker::reset()
{
	if (select_buffer)
	{
		DEALLOCATE(select_buffer);
		select_buffer = NULL;
	}
	select_buffer_size = 10000;
	number_of_hits = 0;
}

/*provide a select buffer pointer and return the scene and graphics */
int cmzn_scenepicker::getSceneAndGraphics(GLuint *select_buffer_ptr,
	cmzn_scene_id *picked_scene, cmzn_graphics_id *graphics)
{
	if (top_scene && select_buffer_ptr)
	{
		*picked_scene = cmzn_scene_get_child_of_picking_name(top_scene, (int)(select_buffer_ptr[3]));
		*graphics = cmzn_scene_get_graphics_at_position(*picked_scene,
			(int)(select_buffer_ptr[4]));
		return 1;
	}
	else
	{
		*picked_scene = 0;
		*graphics = 0;
		return 0;
	}
}

cmzn_scenefilter_id cmzn_scenepicker::getScenefilter()
{
	return cmzn_scenefilter_access(filter);
}

int cmzn_scenepicker::setScenefilter(cmzn_scenefilter_id filter_in)
{
	if (filter_in != filter)
	{
		reset();
		if (filter)
			cmzn_scenefilter_destroy(&filter);
		if (filter_in)
			filter = cmzn_scenefilter_access(filter_in);
	}
	return CMZN_OK;
}

cmzn_scene_id cmzn_scenepicker::getScene()
{
	return cmzn_scene_access(top_scene);
}

int cmzn_scenepicker::setScene(cmzn_scene_id scene_in)
{
	if (scene_in)
	{
		if (scene_in != top_scene)
		{
			reset();
			if (top_scene)
				cmzn_scene_destroy(&top_scene);
			top_scene = cmzn_scene_access(scene_in);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_scenepicker::setSceneviewerRectangle(cmzn_sceneviewer_id sceneviewerIn,
	enum cmzn_scenecoordinatesystem scenecoordinatesystemIn, double x1,
	double y1, double x2, double y2)
{
	reset();
	if (sceneviewerIn)
	{
		if (this->scene_viewer)
			cmzn_sceneviewer_destroy(&this->scene_viewer);
		this->coordinate_system = scenecoordinatesystemIn;
		this->scene_viewer = cmzn_sceneviewer_access(sceneviewerIn);
		size_x = x2 - x1;
		size_y = y2 - y1;
		centre_x = x1 + size_x/2;
		centre_y = y1 + size_y/2;
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_scenepicker::setInteractionVolume(struct Interaction_volume *interaction_volume_in)
{
	reset();
	if (scene_viewer)
		cmzn_sceneviewer_destroy(&scene_viewer);
	size_x = 0;
	size_y = 0;
	centre_x = 0;
	centre_y = 0;
	REACCESS(Interaction_volume)(&interaction_volume, interaction_volume_in);
	return 1;
}

cmzn_element_id cmzn_scenepicker::getNearestElement()
{
	cmzn_element_id nearest_element = 0;
	if ((CMZN_OK == pickObjects()) && select_buffer)
	{
		int hit_no, number_of_names, current_element_type = 0;
		GLuint *select_buffer_ptr = 0, *next_select_buffer = select_buffer;
		double current_nearest = 0, nearest = 0;
		cmzn_scene_id picked_scene = 0, existing_scene = 0;
		cmzn_graphics_id graphics = 0;
		cmzn_mesh_id mesh = 0;
		for (hit_no=0;(hit_no<number_of_hits);hit_no++)
		{
			/* select_buffer[0] = number_of_names
			 * select_buffer[1] = nearest
			 * select_buffer[2] = furthest
			 * select_buffer[3] = scene
			 * select_buffer[4] = graphics position
			 * select_buffer[5] = element index
			 * select_buffer[6] = point number
			 */
			select_buffer_ptr = next_select_buffer;
			number_of_names=(int)(select_buffer_ptr[0]);
			next_select_buffer = select_buffer_ptr + number_of_names + 3;
			if (number_of_names >= 3)
			{
				/* get range of depth of picked object */
				/*???RC OpenGL Programming Guide p361 says depth values are
					made into integers from 0 to 2^32-1. Just convert to
						doubles for now */
				nearest = (double)(select_buffer_ptr[1]);
				if ((nearest_element == NULL) || (nearest < current_nearest))
				{
					if ((getSceneAndGraphics(select_buffer_ptr,
						&picked_scene, &graphics) && (0 != picked_scene) && (0 != graphics)))
					{
						if (cmzn_graphics_selects_elements(graphics))
						{
							if (picked_scene)
							{
								cmzn_region *region = cmzn_scene_get_region_internal(picked_scene);
								int element_type = cmzn_graphics_get_domain_dimension(graphics);
								if ((existing_scene != picked_scene) ||
									(current_element_type != element_type))
								{
									if (existing_scene)
										cmzn_scene_destroy(&existing_scene);
									existing_scene = picked_scene;
									current_element_type = element_type;
									cmzn_fieldmodule_id field_module = cmzn_region_get_fieldmodule(region);
									if (mesh)
										cmzn_mesh_destroy(&mesh);
									mesh = cmzn_fieldmodule_find_mesh_by_dimension(field_module, current_element_type);
									cmzn_fieldmodule_destroy(&field_module);
								}
								else
								{
									cmzn_scene_destroy(&picked_scene);
								}
								FE_mesh *fe_mesh = cmzn_mesh_get_FE_mesh_internal(mesh);
								cmzn_element_id element = fe_mesh->getElement(select_buffer_ptr[5]);
								if (element)
								{
									ACCESS(FE_element)(element);
									if (nearest_element)
										cmzn_element_destroy(&nearest_element);
									nearest_element = element;
									current_nearest = nearest;
								}
							}
						}
						cmzn_graphics_destroy(&graphics);
					}
				}
			}
		}
		if (existing_scene)
			cmzn_scene_destroy(&existing_scene);
		if (mesh)
			cmzn_mesh_destroy(&mesh);
	}
	return nearest_element;
}

cmzn_node_id cmzn_scenepicker::getNearestNode()
{
	cmzn_node_id nearest_node = 0;
	if ((CMZN_OK == pickObjects()) && select_buffer)
	{
		int number_of_names;
		GLuint *select_buffer_ptr = 0, *next_select_buffer = select_buffer;
		double current_nearest = 0, nearest = 0;
		cmzn_scene_id picked_scene = 0;
		cmzn_scene_id lastScene = 0;
		cmzn_field_domain_type lastDomainType = CMZN_FIELD_DOMAIN_TYPE_INVALID;
		cmzn_graphics_id graphics = 0;
		cmzn_nodeset_id nodeset = 0;
		for (int hit_no = 0; (hit_no < number_of_hits); ++hit_no)
		{
			/* select_buffer[0] = number_of_names
			 * select_buffer[1] = nearest
			 * select_buffer[2] = furthest
			 * select_buffer[3] = scene
			 * select_buffer[4] = graphics position
			 * select_buffer[5] = element index
			 * select_buffer[6] = point number
			 */
			select_buffer_ptr = next_select_buffer;
			number_of_names=(int)(select_buffer_ptr[0]);
			next_select_buffer = select_buffer_ptr + number_of_names + 3;
			if (number_of_names >= 4)
			{
				/* get range of depth of picked object */
				/*???RC OpenGL Programming Guide p361 says depth values are
					made into integers from 0 to 2^32-1. Just convert to
						doubles for now */
				nearest = (double)(select_buffer_ptr[1]);
				if ((nearest_node == NULL) || (nearest < current_nearest))
				{
					if ((getSceneAndGraphics(select_buffer_ptr, &picked_scene, &graphics) &&
						(0 != picked_scene) && (0 != graphics)))
					{
						cmzn_field_domain_type domainType = cmzn_graphics_get_field_domain_type(graphics);
						if ((CMZN_FIELD_DOMAIN_TYPE_DATAPOINTS == domainType) ||
							(CMZN_FIELD_DOMAIN_TYPE_NODES == domainType))
						{
							if ((picked_scene != lastScene) || (domainType != lastDomainType))
							{
								if (lastScene)
									cmzn_scene_destroy(&lastScene);
								lastScene = picked_scene;
								lastDomainType = domainType;
								cmzn_region *region = cmzn_scene_get_region_internal(picked_scene);
								cmzn_fieldmodule_id fieldmodule = cmzn_region_get_fieldmodule(region);
								cmzn_nodeset_destroy(&nodeset);
								nodeset = cmzn_fieldmodule_find_nodeset_by_field_domain_type(fieldmodule, domainType);
								cmzn_fieldmodule_destroy(&fieldmodule);
							}
							else
							{
								cmzn_scene_destroy(&picked_scene);
							}
							cmzn_node_id node = cmzn_nodeset_find_node_by_identifier(nodeset, (int)(select_buffer_ptr[6]));
							if (node)
							{
								if (nearest_node)
									cmzn_node_destroy(&nearest_node);
								nearest_node = node;
								current_nearest = nearest;
							}
						}
						cmzn_graphics_destroy(&graphics);
					}
				}
			}
		}
		if (lastScene)
			cmzn_scene_destroy(&lastScene);
		if (nodeset)
			cmzn_nodeset_destroy(&nodeset);
	}
	return nearest_node;
}

cmzn_graphics_id cmzn_scenepicker::getNearestGraphics(enum cmzn_scenepicker_object_type type)
{
	cmzn_graphics_id nearest_graphics = 0;
	if ((CMZN_OK == pickObjects()) && select_buffer)
	{
		int hit_no, number_of_names;
		GLuint *select_buffer_ptr = 0, *next_select_buffer = select_buffer;
		double current_nearest = 0, nearest = 0;
		cmzn_scene_id picked_scene = 0;
		cmzn_graphics_id graphics = 0;
		for (hit_no=0;(hit_no<number_of_hits);hit_no++)
		{
			/* select_buffer[0] = number_of_names
			 * select_buffer[1] = nearest
			 * select_buffer[2] = furthest
			 * select_buffer[3] = scene
			 * select_buffer[4] = graphics position
			 * select_buffer[5] = element index
			 * select_buffer[6] = point number
			 */
			select_buffer_ptr = next_select_buffer;
			number_of_names=(int)(select_buffer_ptr[0]);
			next_select_buffer = select_buffer_ptr + number_of_names + 3;
			if (number_of_names >= 2)
			{
				/* get range of depth of picked object */
				/*???RC OpenGL Programming Guide p361 says depth values are
					made into integers from 0 to 2^32-1. Just convert to
						doubles for now */
				nearest = (double)(select_buffer_ptr[1]);
				if ((nearest_graphics == NULL) || (nearest < current_nearest))
				{
					if ((getSceneAndGraphics(select_buffer_ptr,
						&picked_scene, &graphics) && (0 != picked_scene) && (0 != graphics)))
					{
						if ((type == CMZN_SCENEPICKER_OBJECT_ANY) ||
							((type == CMZN_SCENEPICKER_OBJECT_ELEMENT) &&
								(cmzn_graphics_selects_elements(graphics))) ||
							((type == CMZN_SCENEPICKER_OBJECT_NODE) && (
								(CMZN_FIELD_DOMAIN_TYPE_DATAPOINTS == cmzn_graphics_get_field_domain_type(graphics)) ||
								(CMZN_FIELD_DOMAIN_TYPE_NODES == cmzn_graphics_get_field_domain_type(graphics)))))
						{
							current_nearest = nearest;
							if (graphics != nearest_graphics)
								REACCESS(cmzn_graphics)(&nearest_graphics, graphics);
						}
						cmzn_graphics_destroy(&graphics);
						cmzn_scene_destroy(&picked_scene);
					}
				}
			}
		}
	}
	return nearest_graphics;
}

int cmzn_scenepicker::addPickedElementsToFieldGroup(cmzn_field_group_id group)
{
	if (!group)
		return CMZN_ERROR_ARGUMENT;
	cmzn_region_id groupRegion = Computed_field_get_region(cmzn_field_group_base_cast(group));
	cmzn_region_begin_hierarchical_change(groupRegion);
	if ((CMZN_OK == pickObjects()) && select_buffer)
	{
		cmzn_mesh_id mesh = 0;
		cmzn_mesh_group_id meshGroup = 0;
		int lastDomainDimension = 0;
		cmzn_scene *lastScene = 0;
		cmzn_scene_id picked_scene = 0;
		cmzn_graphics_id graphics = 0;
		int number_of_names;
		GLuint *select_buffer_ptr = 0, *next_select_buffer = select_buffer;
		for (int hit_no = 0; (hit_no < number_of_hits); ++hit_no)
		{
			/* select_buffer[0] = number_of_names
			 * select_buffer[1] = nearest
			 * select_buffer[2] = furthest
			 * select_buffer[3] = scene
			 * select_buffer[4] = graphics position
			 * select_buffer[5] = element index
			 * select_buffer[6] = point number
			 */
			select_buffer_ptr = next_select_buffer;
			number_of_names=(int)(select_buffer_ptr[0]);
			next_select_buffer = select_buffer_ptr + number_of_names + 3;
			if (number_of_names >= 3)
			{
				if ((getSceneAndGraphics(select_buffer_ptr, &picked_scene, &graphics) &&
					(0 != picked_scene) && (0 != graphics)))
				{
					int domainDimension = cmzn_graphics_get_domain_dimension(graphics);
					if (0 < domainDimension)
					{
						if ((picked_scene != lastScene) || (domainDimension != lastDomainDimension))
						{
							if (lastScene)
								cmzn_scene_destroy(&lastScene);
							lastScene = picked_scene;
							lastDomainDimension = domainDimension;
							cmzn_region *region = cmzn_scene_get_region_internal(picked_scene);
							cmzn_fieldmodule_id fieldmodule = cmzn_region_get_fieldmodule(region);
							cmzn_mesh_destroy(&mesh);
							cmzn_mesh_group_destroy(&meshGroup);
							mesh = cmzn_fieldmodule_find_mesh_by_dimension(fieldmodule, domainDimension);
							cmzn_field_element_group_id element_group = cmzn_field_group_get_field_element_group(group, mesh);
							if (!element_group)
								element_group = cmzn_field_group_create_field_element_group(group, mesh);
							meshGroup = cmzn_field_element_group_get_mesh_group(element_group);
							cmzn_field_element_group_destroy(&element_group);
							cmzn_fieldmodule_destroy(&fieldmodule);
						}
						else
							cmzn_scene_destroy(&picked_scene);
						if (mesh && meshGroup)
						{
							FE_mesh *fe_mesh = cmzn_mesh_get_FE_mesh_internal(mesh);
							cmzn_element_id element = fe_mesh->getElement(select_buffer_ptr[5]);
							cmzn_mesh_group_add_element(meshGroup, element);
						}
					}
					cmzn_graphics_destroy(&graphics);
				}
			}
		}
		cmzn_scene_destroy(&lastScene);
		cmzn_mesh_group_destroy(&meshGroup);
		cmzn_mesh_destroy(&mesh);
	}
	cmzn_region_end_hierarchical_change(groupRegion);
	return CMZN_OK;
}

int cmzn_scenepicker::addPickedNodesToFieldGroup(cmzn_field_group_id group)
{
	if (!group)
		return CMZN_ERROR_ARGUMENT;
	cmzn_region_id groupRegion = Computed_field_get_region(cmzn_field_group_base_cast(group));
	cmzn_region_begin_hierarchical_change(groupRegion);
	if ((CMZN_OK == pickObjects()) && select_buffer)
	{
		cmzn_nodeset_id nodeset = 0;
		cmzn_nodeset_group_id nodesetGroup = 0;
		cmzn_field_domain_type lastDomainType = CMZN_FIELD_DOMAIN_TYPE_INVALID;
		cmzn_scene *lastScene = 0;
		cmzn_scene_id picked_scene = 0;
		cmzn_graphics_id graphics = 0;
		int number_of_names;
		GLuint *select_buffer_ptr = 0, *next_select_buffer = select_buffer;
		for (int hit_no = 0; (hit_no < number_of_hits); ++hit_no)
		{
			/* select_buffer[0] = number_of_names
			 * select_buffer[1] = nearest
			 * select_buffer[2] = furthest
			 * select_buffer[3] = scene
			 * select_buffer[4] = graphics position
			 * select_buffer[5] = node number
			 */
			select_buffer_ptr = next_select_buffer;
			number_of_names=(int)(select_buffer_ptr[0]);
			next_select_buffer = select_buffer_ptr + number_of_names + 3;
			if (number_of_names >= 4)
			{
				if ((getSceneAndGraphics(select_buffer_ptr, &picked_scene, &graphics) &&
					(0 != picked_scene) && (0 != graphics)))
				{
					cmzn_field_domain_type domainType = cmzn_graphics_get_field_domain_type(graphics);
					if ((CMZN_FIELD_DOMAIN_TYPE_DATAPOINTS == domainType) ||
						(CMZN_FIELD_DOMAIN_TYPE_NODES == domainType))
					{
						if ((picked_scene != lastScene) || (domainType != lastDomainType))
						{
							if (lastScene)
								cmzn_scene_destroy(&lastScene);
							lastScene = picked_scene;
							lastDomainType = domainType;
							cmzn_region *region = cmzn_scene_get_region_internal(picked_scene);
							cmzn_fieldmodule_id fieldmodule = cmzn_region_get_fieldmodule(region);
							cmzn_nodeset_destroy(&nodeset);
							cmzn_nodeset_group_destroy(&nodesetGroup);
							nodeset = cmzn_fieldmodule_find_nodeset_by_field_domain_type(fieldmodule, domainType);
							cmzn_field_node_group_id node_group = cmzn_field_group_get_field_node_group(group, nodeset);
							if (!node_group)
								node_group = cmzn_field_group_create_field_node_group(group, nodeset);
							nodesetGroup = cmzn_field_node_group_get_nodeset_group(node_group);
							cmzn_field_node_group_destroy(&node_group);
							cmzn_fieldmodule_destroy(&fieldmodule);
						}
						else
							cmzn_scene_destroy(&picked_scene);
						if (nodeset && nodesetGroup)
						{
							cmzn_node_id node = cmzn_nodeset_find_node_by_identifier(nodeset, (int)(select_buffer_ptr[6]));
							cmzn_nodeset_group_add_node(nodesetGroup, node);
							cmzn_node_destroy(&node);
						}
					}
					cmzn_graphics_destroy(&graphics);
				}
			}
		}
		cmzn_scene_destroy(&lastScene);
		cmzn_nodeset_group_destroy(&nodesetGroup);
		cmzn_nodeset_destroy(&nodeset);
	}
	cmzn_region_end_hierarchical_change(groupRegion);
	return CMZN_OK;
}

int DESTROY(cmzn_scenepicker)(struct cmzn_scenepicker **scenepicker_address)
{
	if (scenepicker_address && (*scenepicker_address))
	{
		delete *scenepicker_address;
		*scenepicker_address = NULL;
		return 1;
	}
	else
	{
		return 0;
	}
}

cmzn_scenepicker_id cmzn_scenepicker_create(cmzn_scenefiltermodule_id filter_module)
{
	return new cmzn_scenepicker(filter_module);
}

DECLARE_OBJECT_FUNCTIONS(cmzn_scenepicker);

cmzn_scenepicker_id cmzn_scenepicker_access(
	cmzn_scenepicker_id scenepicker)
{
	if (scenepicker)
		return scenepicker->access();
	return 0;
}

int cmzn_scenepicker_destroy(cmzn_scenepicker_id *scenepicker_address)
{
	return DEACCESS(cmzn_scenepicker)(scenepicker_address);
}

cmzn_scene_id cmzn_scenepicker_get_scene(cmzn_scenepicker_id scenepicker)
{
	return scenepicker->getScene();
}

int cmzn_scenepicker_set_scene(cmzn_scenepicker_id scenepicker,
	cmzn_scene_id scene)
{
	return scenepicker->setScene(scene);
}

cmzn_scenefilter_id cmzn_scenepicker_get_scenefilter(
	cmzn_scenepicker_id scenepicker)
{
	return scenepicker->getScenefilter();
}

int cmzn_scenepicker_set_scenefilter(cmzn_scenepicker_id scenepicker,
	cmzn_scenefilter_id filter)
{
	return scenepicker->setScenefilter(filter);
}

int cmzn_scenepicker_set_sceneviewer_rectangle(
	cmzn_scenepicker_id scenepicker, cmzn_sceneviewer_id sceneviewer,
	enum cmzn_scenecoordinatesystem scenecoordinatesystem, double x1,
	double y1, double x2, double y2)
{
	return scenepicker->setSceneviewerRectangle(sceneviewer,
		scenecoordinatesystem, x1, y1, x2, y2);
}

cmzn_element_id cmzn_scenepicker_get_nearest_element(cmzn_scenepicker_id scenepicker)
{
	return scenepicker->getNearestElement();
}

cmzn_node_id cmzn_scenepicker_get_nearest_node(cmzn_scenepicker_id scenepicker)
{
	return scenepicker->getNearestNode();
}

cmzn_graphics_id cmzn_scenepicker_get_nearest_graphics(cmzn_scenepicker_id scenepicker)
{
	return scenepicker->getNearestGraphics(CMZN_SCENEPICKER_OBJECT_ANY);
}

cmzn_graphics_id cmzn_scenepicker_get_nearest_element_graphics(cmzn_scenepicker_id scenepicker)
{
	return scenepicker->getNearestGraphics(CMZN_SCENEPICKER_OBJECT_ELEMENT);
}

cmzn_graphics_id cmzn_scenepicker_get_nearest_node_graphics(cmzn_scenepicker_id scenepicker)
{
	return scenepicker->getNearestGraphics(CMZN_SCENEPICKER_OBJECT_NODE);
}

int cmzn_scenepicker_add_picked_elements_to_field_group(cmzn_scenepicker_id scenepicker,
	cmzn_field_group_id group)
{
	return scenepicker->addPickedElementsToFieldGroup(group);
}

int cmzn_scenepicker_add_picked_nodes_to_field_group(cmzn_scenepicker_id scenepicker,
	cmzn_field_group_id group)
{
	return scenepicker->addPickedNodesToFieldGroup(group);
}

int cmzn_scenepicker_get_picking_volume_centre(cmzn_scenepicker_id scenepicker,
	double *coordinateValuesOut3)
{
	return scenepicker->getPickingVolumeCentre(coordinateValuesOut3);
}

int cmzn_scenepicker_set_interaction_volume(cmzn_scenepicker_id scenepicker,
	struct Interaction_volume *interaction_volume)
{
	return scenepicker->setInteractionVolume(interaction_volume);
}
