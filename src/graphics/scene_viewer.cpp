/*******************************************************************************
FILE : scene_viewer.cpp

LAST MODIFIED : 17 February 200

DESCRIPTION :
Three_D_drawing derivative for viewing a Scene from an arbitrary position.
The scene viewer has the following modes for handling user input:
SCENE_VIEWER_NO_INPUT ignores any input, leaving it up to the owner of the
scene viewer to set viewing parameters.
SCENE_VIEWER_SELECT performs OpenGL picking and returns the picked
objects to the scene via a callback, along with mouse button press and motion
information in a view-independent format.
SCENE_VIEWER_TRANSFORM allows the view of the scene to be changed by tumbling,
translating and zooming with mouse button and press and motion events.

HISTORY :
November 97 Created from rendering part of Drawing.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <cmath>
#include <cstdio>
#include <map>
#include "opencmiss/zinc/field.h"
#include "opencmiss/zinc/fieldmodule.h"
#include "opencmiss/zinc/scenefilter.h"
#include "opencmiss/zinc/sceneviewerinput.h"
#include "opencmiss/zinc/status.h"
#include "computed_field/computed_field_image.h"
#include "description_io/sceneviewer_json_io.hpp"
#include "general/compare.h"
#include "general/callback_private.h"
#include "general/debug.h"
#include "general/mystring.h"
#include "general/enumerator_conversion.hpp"
#include "general/enumerator_private.hpp"
#include "general/geometry.h"
#include "general/image_utilities.h"
#include "general/list.h"
#include "general/list_private.h"
#include "general/indexed_list_private.h"
#include "general/matrix_vector.h"
#include "general/object.h"
#include "general/message.h"
#include "graphics/colour.h"
#include "graphics/graphics_library.h"
#include "graphics/light.hpp"
#include "graphics/scene.hpp"
#include "graphics/scenefilter.hpp"
#include "graphics/texture.h"
#include "graphics/scene_viewer.h"
#include "three_d_drawing/graphics_buffer.h"
#include "interaction/interactive_event.h"
#include "graphics/render_gl.h"
#include "graphics/scene_coordinate_system.hpp"
#include <algorithm>

#define USE_LAYERZ
#if defined (USE_LAYERZ)
#include "graphics/order_independent_transparency.h"
#endif /* defined (USE_LAYERZ) */

/*
Module constants
----------------
*/
/*
Module types
------------
*/

FULL_DECLARE_CMZN_CALLBACK_TYPES(cmzn_sceneviewermodule_callback, \
	struct cmzn_sceneviewermodule *, void *);

/*
Module functions
----------------
*/

int cmzn_sceneviewer::setBackgroundColourAlpha(double alpha)
{
	this->background_colour.alpha = alpha;
	this->setChangedRepaint();
	return CMZN_OK;
}

int cmzn_sceneviewer::getBackgroundColourRGB(double *valuesOut3) const
{
	if (valuesOut3)
	{
		valuesOut3[0] = this->background_colour.red;
		valuesOut3[1] = this->background_colour.green;
		valuesOut3[2] = this->background_colour.blue;
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_sceneviewer::setBackgroundColourRGB(const double *valuesIn3)
{
	if (valuesIn3)
	{
		this->background_colour.red = valuesIn3[0];
		this->background_colour.green = valuesIn3[1];
		this->background_colour.blue = valuesIn3[2];
		this->setChangedRepaint();
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_sceneviewer::getBackgroundColourRGBA(double *valuesOut4) const
{
	if (valuesOut4)
	{
		valuesOut4[0] = this->background_colour.red;
		valuesOut4[1] = this->background_colour.green;
		valuesOut4[2] = this->background_colour.blue;
		valuesOut4[3] = this->background_colour.alpha;
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_sceneviewer::setBackgroundColourRGBA(const double *valuesIn4)
{
	if (valuesIn4)
	{
		this->background_colour.red = valuesIn4[0];
		this->background_colour.green = valuesIn4[1];
		this->background_colour.blue = valuesIn4[2];
		this->background_colour.alpha = valuesIn4[3];
		this->setChangedRepaint();
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

void cmzn_sceneviewer::notifyClients()
{
	cmzn_sceneviewerevent_id event = new cmzn_sceneviewerevent();
	event->changeFlags = this->changes;
	for (auto iter = this->notifier_list->begin(); iter != this->notifier_list->end(); ++iter)
	{
		(*iter)->notify(event);
	}
	cmzn_sceneviewerevent_destroy(&event);
}

void cmzn_sceneviewer::setLightingLocalViewer(bool value)
{
	if (value != this->lightingLocalViewer)
	{
		this->lightingLocalViewer = value;
		this->setChangedRepaint();
	}
}

void cmzn_sceneviewer::setLightingTwoSided(bool value)
{
	if (value != this->lightingTwoSided)
	{
		this->lightingTwoSided = value;
		this->setChangedRepaint();
	}
}

cmzn_sceneviewerinput_id cmzn_sceneviewer_create_sceneviewerinput(struct Scene_viewer *scene_viewer)
{
	cmzn_sceneviewerinput_id input = 0;
	if (scene_viewer)
	{
		if(ALLOCATE(input, struct cmzn_sceneviewerinput, 1))
		{
			input->access_count = 1;
			input->button_number = 0;
			input->modifiers = CMZN_SCENEVIEWERINPUT_MODIFIER_FLAG_NONE;
			input->key_code = 0;
			input->position_x = 0;
			input->position_y = 0;
			input->type = CMZN_SCENEVIEWERINPUT_EVENT_TYPE_INVALID;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_sceneviewer_create_sceneviewerinput.  Invalid argument(s)");
	}

	return (input);
}

cmzn_sceneviewerinput_id cmzn_sceneviewerinput_access(cmzn_sceneviewerinput_id input)
{
	if (input)
		++(input->access_count);
	return input;
}

int cmzn_sceneviewerinput_destroy(cmzn_sceneviewerinput_id *address_input)
{
	int return_code = 0;
	if (address_input)
	{
		(*address_input)->access_count--;
		if ((*address_input)->access_count == 0)
		{
			DEALLOCATE(*address_input);
			*address_input = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_sceneviewerinput_destroy.  Invalid argument(s)");
	}

	return (return_code);
}

int cmzn_sceneviewerinput_set_position(cmzn_sceneviewerinput_id input, int x, int y)
{
	int return_code = 0;
	if (input)
	{
		return_code = 1;
		input->position_x = x;
		input->position_y = y;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_sceneviewerinput_set_position.  Invalid argument(s)");
	}

	return (return_code);
}

int cmzn_sceneviewerinput_set_modifier_flags(cmzn_sceneviewerinput_id input,
	cmzn_sceneviewerinput_modifier_flags modifier_flags)
{
	if (input)
	{
		input->modifiers = modifier_flags;
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_sceneviewerinput_set_button_type(cmzn_sceneviewerinput_id input,
	cmzn_sceneviewerinput_button_type button_type)
{
	if (input)
	{
		input->button_number = static_cast<int>(button_type) + 1;
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_sceneviewerinput_set_event_type(cmzn_sceneviewerinput_id input, cmzn_sceneviewerinput_event_type event_type)
{
	if (input)
	{
		input->type = event_type;
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_sceneviewer_begin_change(cmzn_sceneviewer_id sceneviewer)
{
	if (sceneviewer)
	{
		sceneviewer->beginChange();
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_sceneviewer_end_change(cmzn_sceneviewer_id sceneviewer)
{
	if (sceneviewer)
	{
		sceneviewer->endChange();
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

struct Scene_viewer_rendering_data
/*******************************************************************************
LAST MODIFIED : 9 April 2003

DESCRIPTION :
Temporary data that only lasts for a single rendering.  Used by all the
rendering functions.
==============================================================================*/
{
	/* The main scene_viewer */
	struct Scene_viewer *scene_viewer;

	/* The render callstack which we are processing */
	struct LIST(Scene_viewer_render_object) *render_callstack;

	/* Information for the rendering */
	int override_antialias;
	int override_transparency_layers;
	/* Width and height of actual viewport used internally for rendering */
	int viewport_left;
	int viewport_bottom;
	int viewport_width;
	int viewport_height;
	/* A flag to indicate to the rendering routines if the current render is
		actually being buffered, it may differ from teh buffering_mode if it
		the scene is being rendered offscreen */
	int rendering_double_buffered;
	/* Stencil buffer depth */
	GLint stencil_depth;

	Render_graphics_opengl *renderer;
}; /* struct Scene_viewer_rendering_data */

struct Scene_viewer_render_object;
DECLARE_LIST_TYPES(Scene_viewer_render_object);

typedef int (Scene_viewer_render_function)(
	struct Scene_viewer_rendering_data *rendering_data);
/*******************************************************************************
LAST MODIFIED : 14 April 2003

DESCRIPTION :
A rendering function that exists in the rendering callstack.  See the
struct Scene_viewer_render_object.
==============================================================================*/

struct Scene_viewer_render_object
/*******************************************************************************
LAST MODIFIED : 4 April 2003

DESCRIPTION :
This object is used to implement a callstack of render functions.  Each render
function may need to call those lower down multiple times and this provides
a facility for building a calltree of such functions.  This enables us to break
up the rendering into more sensible components.
In addition to the <rendering_data> which is private to this module some rendering
functions may have their own user data.
==============================================================================*/
{
	Scene_viewer_render_function *render_function;
	/* A flag used when descending down the call stack to indicate which
		items have already been processed */
	int already_processed;
	int access_count;
}; /* struct Scene_viewer_render_object */

FULL_DECLARE_LIST_TYPE(Scene_viewer);

/* We need to maintain the order, so we do not want an indexed list */
FULL_DECLARE_LIST_TYPE(Scene_viewer_render_object);

/*
Module functions
----------------
*/

PROTOTYPE_LIST_FUNCTIONS(Scene_viewer);

PROTOTYPE_LIST_FUNCTIONS(Scene_viewer_render_object);

static struct Scene_viewer_render_object *CREATE(Scene_viewer_render_object)(
	Scene_viewer_render_function *render_function)
/*******************************************************************************
LAST MODIFIED : 14 April 2003

DESCRIPTION :
A rendering pass that can be incoporated into the rendering callstack.  The
render_function should implement the component of the rendering, calling
Scene_viewer_call_next_renderer to execute the rest of the renderer.
==============================================================================*/
{
	struct Scene_viewer_render_object *render_object;

	ENTER(CREATE(Scene_viewer_render_object));
	if (render_function)
	{
		if (ALLOCATE(render_object,struct Scene_viewer_render_object,1))
		{
			render_object->render_function = render_function;
			render_object->already_processed = 0;
			render_object->access_count = 0;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"CREATE(Scene_viewer_render_object).  Could not allocate memory for node field");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CREATE(Scene_viewer_render_object).  Invalid argument(s)");
		render_object = (struct Scene_viewer_render_object *)NULL;
	}
	LEAVE;

	return (render_object);
} /* CREATE(Scene_viewer_render_object) */

static int Scene_viewer_render_object_has_not_been_processed(
	struct Scene_viewer_render_object *render_object, void *dummy_void)
/*******************************************************************************
LAST MODIFIED : 7 April 2003

DESCRIPTION :
Returns NOT the state of the already_processed flag.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_viewer_render_object_has_not_been_processed);
	USE_PARAMETER(dummy_void);
	if (render_object)
	{
		return_code = ! render_object->already_processed;
	}
	else
	{
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_render_object_has_not_been_processed */

static int DESTROY(Scene_viewer_render_object)(
	struct Scene_viewer_render_object **render_object_address)
/*******************************************************************************
LAST MODIFIED : 4 April 2003

DESCRIPTION :
Frees the memory for the render_object and sets <*render_object_address> to NULL.
==============================================================================*/
{
	int return_code;

	ENTER(DESTROY(Scene_viewer_render_object));
	if (render_object_address)
	{
		if (0 == (*render_object_address)->access_count)
		{
			DEALLOCATE(*render_object_address);
		}
		else
		{
			*render_object_address =
				(struct Scene_viewer_render_object *)NULL;
		}
		return_code = 1;
	}
	else
	{
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Scene_viewer_render_object) */

DECLARE_OBJECT_FUNCTIONS(Scene_viewer_render_object)
DECLARE_LIST_FUNCTIONS(Scene_viewer_render_object)

DEFINE_CMZN_CALLBACK_MODULE_FUNCTIONS(cmzn_sceneviewermodule_callback, void)

DEFINE_CMZN_CALLBACK_FUNCTIONS(cmzn_sceneviewermodule_callback, \
	struct cmzn_sceneviewermodule *,void *)

//-- DEFINE_CMZN_CALLBACK_MODULE_FUNCTIONS(Scene_viewer_input_callback, int)

//-- DEFINE_CMZN_CALLBACK_FUNCTIONS(Scene_viewer_input_callback,
//-- 	struct Scene_viewer *,struct Graphics_buffer_input *)


static int Scene_viewer_render_background_texture(
	struct Scene_viewer *scene_viewer,int viewport_width,int viewport_height,
	Render_graphics *renderer)
/*******************************************************************************
LAST MODIFIED : 11 February 2002

DESCRIPTION :
==============================================================================*/
{
	ZnReal corner_x[4],corner_y[4],corr_x1,corr_x2,corr_y1,corr_y2,
		distortion_centre_x,distortion_centre_y,distortion_factor_k1,
		dist_x,dist_y1,dist_y2,min_x,max_x,min_y,max_y,tex_ratio_x,tex_ratio_y,
		viewport_texture_height,viewport_texture_width;
	ZnReal centre_x,centre_y,factor_k1,texture_width,texture_height,texture_depth;
	GLdouble viewport_left,viewport_right,viewport_bottom,viewport_top;
	int depth_texels, height_texels,i,j,k,min_i,max_i,min_j,max_j,return_code,
		texels_per_polygon_x,texels_per_polygon_y,width_texels;

	ENTER(Scene_viewer_render_background_texture);
	if (scene_viewer&&scene_viewer->image_texture.texture)
	{
		/* get information about the texture */
		Texture_get_original_size(scene_viewer->image_texture.texture,
			&width_texels, &height_texels, &depth_texels);
		Texture_get_physical_size(scene_viewer->image_texture.texture,
			&texture_width, &texture_height, &texture_depth);
		tex_ratio_x=texture_width/width_texels;
		tex_ratio_y=texture_height/height_texels;
		/* note the texture stores radial distortion parameters in terms
			 of its physical space from 0,0 to texture_width,texture_height.
			 We want them in terms of user viewport coordinates */
		Texture_get_distortion_info(scene_viewer->image_texture.texture,
			&centre_x,&centre_y,&factor_k1);
		distortion_centre_x=centre_x;
		distortion_centre_y=centre_y;
		if (scene_viewer->bk_texture_undistort_on)
		{
			distortion_factor_k1=factor_k1;
		}
		else
		{
			distortion_factor_k1=0.0;
		}
		/* set up orthographic projection to match physical/model
			 coordinates of background texture */
		viewport_texture_width=scene_viewer->bk_texture_width;
		if (0.0==viewport_texture_width)
		{
			/* to avoid division by zero */
			viewport_texture_width=1.0;
		}
		viewport_texture_height=scene_viewer->bk_texture_height;
		if (0.0==viewport_texture_height)
		{
			/* to avoid division by zero */
			viewport_texture_height=1.0;
		}
		viewport_left = texture_width/viewport_texture_width*
			(scene_viewer->user_viewport_left - scene_viewer->bk_texture_left);
		viewport_right = viewport_left +
			((double)viewport_width/scene_viewer->user_viewport_pixels_per_unit_x)*
			texture_width/viewport_texture_width;
		viewport_top = texture_height +
			texture_height/viewport_texture_height *
			(scene_viewer->user_viewport_top - scene_viewer->bk_texture_top);
		viewport_bottom=viewport_top -
			((double)viewport_height/scene_viewer->user_viewport_pixels_per_unit_y)*
			texture_height/viewport_texture_height;
#if defined (DEBUG_CODE)
		/*???debug */
		printf("viewport left=%f right=%f  top=%f bottom=%f\n",
			viewport_left,viewport_right,viewport_top,viewport_bottom);
#endif /* defined (DEBUG_CODE) */
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(viewport_left,viewport_right,viewport_bottom,viewport_top,
			-1.0,1.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		renderer->Texture_compile(scene_viewer->image_texture.texture);
		renderer->Texture_execute(scene_viewer->image_texture.texture);

		/* get texels per polygon */
		texels_per_polygon_x=1;
		while ((2*texels_per_polygon_x*
			scene_viewer->user_viewport_pixels_per_unit_x*
			viewport_texture_width/width_texels <=
			scene_viewer->bk_texture_max_pixels_per_polygon)&&
			(texels_per_polygon_x < width_texels))
		{
			texels_per_polygon_x *= 2;
		}
		texels_per_polygon_y=1;
		while ((2*texels_per_polygon_y*
			scene_viewer->user_viewport_pixels_per_unit_y*
			viewport_texture_height/height_texels <=
			scene_viewer->bk_texture_max_pixels_per_polygon)&&
			(texels_per_polygon_y < height_texels))
		{
			texels_per_polygon_y *= 2;
		}
#if defined (DEBUG_CODE)
		/*???debug */
		printf("texels per polygon: x=%i y=%i\n",texels_per_polygon_x,
			texels_per_polygon_y);
#endif /* defined (DEBUG_CODE) */
		/* get range of physical texture coordinates across viewport */
		corner_x[0]=viewport_left;
		corner_x[1]=viewport_right;
		corner_x[2]=viewport_right;
		corner_x[3]=viewport_left;
		corner_y[0]=viewport_top;
		corner_y[1]=viewport_top;
		corner_y[2]=viewport_bottom;
		corner_y[3]=viewport_bottom;
		min_x=max_x=viewport_left;
		min_y=max_y=viewport_top;
		for (k=0;k<4;k++)
		{
			if (corner_x[k]<min_x)
			{
				min_x=corner_x[k];
			}
			if (corner_x[k]>max_x)
			{
				max_x=corner_x[k];
			}
			if (corner_y[k]<min_y)
			{
				min_y=corner_y[k];
			}
			if (corner_y[k]>max_y)
			{
				max_y=corner_y[k];
			}
			if (0!=distortion_factor_k1)
			{
				get_radial_distortion_distorted_coordinates(corner_x[k],
					corner_y[k],distortion_centre_x,distortion_centre_y,
					distortion_factor_k1,/*tolerance*/0.001,&(corner_x[k]),
					&(corner_y[k]));
				if (corner_x[k]<min_x)
				{
					min_x=corner_x[k];
				}
				if (corner_x[k]>max_x)
				{
					max_x=corner_x[k];
				}
				if (corner_y[k]<min_y)
				{
					min_y=corner_y[k];
				}
				if (corner_y[k]>max_y)
				{
					max_y=corner_y[k];
				}
			}
		}
		/* ensure inside actual range of image */
		if (min_x<0)
		{
			min_x=0;
		}
		if (max_x>texture_width)
		{
			max_x=texture_width;
		}
		if (min_y<0)
		{
			min_y=0;
		}
		if (max_y>texture_height)
		{
			max_y=texture_height;
		}
		/* get max_x, max_y in terms of texels */
		min_x /= tex_ratio_x;
		max_x /= tex_ratio_x;
		min_y /= tex_ratio_y;
		max_y /= tex_ratio_y;
#if defined (DEBUG_CODE)
		/*???debug */
		printf("min_x=%f max_x=%f  min_y=%f max_y=%f\n",min_x,max_x,min_y,
			max_y);
#endif /* defined (DEBUG_CODE) */
		min_i = (int)(min_x/(double)texels_per_polygon_x);
		max_i = (int)ceil(0.999999*max_x/(double)texels_per_polygon_x);
		min_j = (int)(min_y/(double)texels_per_polygon_y);
		max_j = (int)ceil(0.999999*max_y/(double)texels_per_polygon_y);
#if defined (DEBUG_CODE)
		/*???debug */
		printf("min_i=%i max_i=%i  min_j=%i max_j=%i\n",min_i,max_i,min_j,
			max_j);
#endif /* defined (DEBUG_CODE) */
		tex_ratio_x *= texels_per_polygon_x;
		tex_ratio_y *= texels_per_polygon_y;
		/* draw the array of polygons */
		for (j=min_j;j<max_j;j++)
		{
			dist_y1=j*tex_ratio_y;
			dist_y2=(j+1)*tex_ratio_y;
			if (dist_y2>texture_height)
			{
				dist_y2=texture_height;
			}
			glBegin(GL_QUAD_STRIP);
			for (i=min_i;i<=max_i;i++)
			{
				dist_x=i*tex_ratio_x;
				if (dist_x>texture_width)
				{
					dist_x=texture_width;
				}
				get_radial_distortion_corrected_coordinates(dist_x,dist_y1,
					distortion_centre_x,distortion_centre_y,distortion_factor_k1,
					&corr_x1,&corr_y1);
				get_radial_distortion_corrected_coordinates(dist_x,dist_y2,
					distortion_centre_x,distortion_centre_y,distortion_factor_k1,
					&corr_x2,&corr_y2);
				glTexCoord2d(dist_x,dist_y1);
				glVertex3d(corr_x1,corr_y1,-0.999);
				glTexCoord2d(dist_x,dist_y2);
				glVertex3d(corr_x2,corr_y2,-0.999);
			}
			glEnd();
		}
		renderer->Texture_execute((struct Texture *)NULL);
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_render_background_texture.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_render_background_texture */

static int Scene_viewer_calculate_transformation(
	struct Scene_viewer *scene_viewer, int viewport_width, int viewport_height)
/*******************************************************************************
LAST MODIFIED : 04 February 2005

DESCRIPTION :
Calculates the projection_matrix, window_projection_matrix and modelview_materix
for the <scene_viewer> onto a given <viewport_width>.<viewport_height> in
pixels, both of which must be positive.
The <left>, <right>, <top> and <bottom> are used to specify the viewing frustrum,
by default these are stored in the scene_viewer but are supplied as parameters
to this function so that they can be overridden.
In CUSTOM projections, the projection_matrix and modelview_matrix are supplied
and hence these are only calculated for other projection modes. In all cases
the projection_matrix is understood to project space onto the NDC_info; In
absolute_viewport mode this is some fixed region relative to the user
viewport coordinates. In relative_viewport mode, the NDC_width and NDC_height
are made as large as can fit in the viewport size without a shape change.
As a result, the projection_matrix in both relative and absolute viewport modes
is not the projection that will fill the entire viewport/window - this function
calculates the window_projection_matrix for this purpose.
Note that this function makes changes to the OpenGL rendering matrices in some
modes, so push/pop them if you want them preserved.
==============================================================================*/
{
	double dx,dy,dz,postmultiply_matrix[16],factor;
	int return_code,i;

	ENTER(Scene_viewer_calculate_transformation);
	if (scene_viewer&&(0<viewport_width)&&(0<viewport_height))
	{
		return_code=1;

		/* 1. calculate and store projection_matrix - no need in CUSTOM mode */
		if (SCENE_VIEWER_CUSTOM != scene_viewer->projection_mode)
		{
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			switch (scene_viewer->projection_mode)
			{
				case SCENE_VIEWER_PARALLEL:
				{
					glOrtho(scene_viewer->left, scene_viewer->right,
						scene_viewer->bottom, scene_viewer->top,
						scene_viewer->near_plane, scene_viewer->far_plane);
				} break;
				case SCENE_VIEWER_PERSPECTIVE:
				{
					/* adjust left, right, bottom, top from lookat plane to near plane */
					dx = scene_viewer->eyex-scene_viewer->lookatx;
					dy = scene_viewer->eyey-scene_viewer->lookaty;
					dz = scene_viewer->eyez-scene_viewer->lookatz;
					factor = scene_viewer->near_plane/sqrt(dx*dx+dy*dy+dz*dz);
					/* perspective projection */

					glFrustum(scene_viewer->left*factor, scene_viewer->right*factor,
						scene_viewer->bottom*factor, scene_viewer->top*factor,
						scene_viewer->near_plane, scene_viewer->far_plane);
				} break;
				case SCENE_VIEWER_CUSTOM:
				{
					/* Do nothing */
				} break;
			}
			glGetDoublev(GL_PROJECTION_MATRIX,scene_viewer->projection_matrix);
		}

		/* 2. calculate and store window_projection_matrix - all modes */
		/* the projection matrix converts the viewing volume into the Normalised
			 Device Coordinates (NDCs) ranging from -1 to +1 in each coordinate
			 direction. Need to scale this range to fit the viewport/window by
			 postmultiplying with a matrix. First start with identity: Note that
			 numbers go down columns first in OpenGL matrices */
		for (i=1;i<15;i++)
		{
			postmultiply_matrix[i] = 0.0;
		}
		postmultiply_matrix[ 0] = 1.0;
		postmultiply_matrix[ 5] = 1.0;
		postmultiply_matrix[10] = 1.0;
		postmultiply_matrix[15] = 1.0;
		switch (scene_viewer->viewport_mode)
		{
			case CMZN_SCENEVIEWER_VIEWPORT_MODE_ABSOLUTE:
			{
				/* absolute viewport: NDC volume is placed in the position
					 described by the NDC_info relative to user viewport
					 coordinates - as with the background texture */
				postmultiply_matrix[0] *= scene_viewer->NDC_width*
					scene_viewer->user_viewport_pixels_per_unit_x/viewport_width;
				postmultiply_matrix[5] *= scene_viewer->NDC_height*
					scene_viewer->user_viewport_pixels_per_unit_y/viewport_height;
				postmultiply_matrix[12] = -1.0+
					((scene_viewer->user_viewport_pixels_per_unit_x)/viewport_width)*
					((scene_viewer->NDC_width)+
						2.0*(scene_viewer->NDC_left-scene_viewer->user_viewport_left));
				postmultiply_matrix[13] =1.0+
					((scene_viewer->user_viewport_pixels_per_unit_y)/viewport_height)*
					(-(scene_viewer->NDC_height)+
						2.0*(scene_viewer->NDC_top-scene_viewer->user_viewport_top));
			} break;
			case CMZN_SCENEVIEWER_VIEWPORT_MODE_RELATIVE:
			{
				/* relative viewport: NDC volume is scaled to the largest size
					 that can fit in the viewport without distorting its shape. Note that
					 the NDC_height and NDC_width are all that is needed to characterise
					 the size/shape of the NDC volume in relative mode */
				if (scene_viewer->NDC_height/scene_viewer->NDC_width >
					(double)viewport_height/(double)viewport_width)
				{
					/* make NDC represent a wider viewing volume. */
					postmultiply_matrix[0] *= (scene_viewer->NDC_width*viewport_height/
						(scene_viewer->NDC_height*viewport_width));
				}
				else
				{
					/* make NDC represent a taller viewing volume */
					postmultiply_matrix[5] *= (scene_viewer->NDC_height*viewport_width/
						(scene_viewer->NDC_width*viewport_height));
				}
			} break;
			case CMZN_SCENEVIEWER_VIEWPORT_MODE_DISTORTING_RELATIVE:
			{
				/* distorting relative viewport: NDC volume is scaled to the largest size
					 that can fit in the viewport. Note that
					 the NDC_height and NDC_width are all that is needed to characterise
					 the size/shape of the NDC volume in relative mode
					 This is a simple no-op, as the identity matrix is sufficient to achieve this.
				*/
			} break;
			case CMZN_SCENEVIEWER_VIEWPORT_MODE_INVALID:
			{
				display_message(ERROR_MESSAGE,
					"Scene_viewer_calculate_transformation.  Invalid viewport mode");
			} break;
		}
		multiply_matrix(4,4,4,scene_viewer->projection_matrix,postmultiply_matrix,
			scene_viewer->window_projection_matrix);

		/* 3. Calculate and store modelview_matrix - no need in CUSTOM mode */
		if (SCENE_VIEWER_CUSTOM != scene_viewer->projection_mode)
		{
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			gluLookAt(scene_viewer->eyex,scene_viewer->eyey,
				scene_viewer->eyez,scene_viewer->lookatx,
				scene_viewer->lookaty,scene_viewer->lookatz,
				scene_viewer->upx,scene_viewer->upy,scene_viewer->upz);
			glGetDoublev(GL_MODELVIEW_MATRIX,scene_viewer->modelview_matrix);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_calculate_transformation.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_calculate_transformation */

Render_graphics_opengl *Scene_viewer_rendering_data_get_renderer(
	Scene_viewer_rendering_data *rendering_data)
{
	return rendering_data->renderer;
}

int Scene_viewer_call_next_renderer(
	struct Scene_viewer_rendering_data *rendering_data)
/*******************************************************************************
LAST MODIFIED : 9 April 2003

DESCRIPTION :
This function calls the next renderer that isn't already being processed and
handles the flags that mark render_objects as processed.
==============================================================================*/
{
	int return_code;
	struct Scene_viewer_render_object *render_object;
	render_object= NULL;

	ENTER(Scene_viewer_call_next_renderer);
	if (rendering_data)
	{
		render_object = FIRST_OBJECT_IN_LIST_THAT(Scene_viewer_render_object)
			(Scene_viewer_render_object_has_not_been_processed, NULL,
			rendering_data->render_callstack);
		render_object->already_processed = 1;
		return_code = (*(render_object->render_function))(
			 rendering_data);
		/* The flag must be set back as we may call parts of the tree
			many times. */
		render_object->already_processed = 0;
	}
	else
	{
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_call_next_renderer */

/***************************************************************************//**
 * Renders graphics in layers with depth buffer cleared between them.
 */
static int Scene_viewer_render_layers(
	struct Scene_viewer_rendering_data *rendering_data)
{
	int return_code = 1;
	if (rendering_data)
	{
		do
		{
			Scene_viewer_call_next_renderer(rendering_data);
		}
		while (rendering_data->renderer->next_layer());
	}
	else
	{
		return_code = 0;
	}
	return (return_code);
}

static int Scene_viewer_execute_scene_non_fastchanging(
	struct Scene_viewer_rendering_data *rendering_data)
/*******************************************************************************
LAST MODIFIED : 4 April 2003

DESCRIPTION :
This is the last function in the render callstack that actually executes the
scene.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_viewer_execute_scene_non_fastchanging);
	if (rendering_data)
	{
		return_code = rendering_data->renderer->Scene_tree_execute(
			rendering_data->scene_viewer->scene);
	}
	else
	{
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_execute_scene_non_fastchanging */

static int Scene_viewer_apply_projection_matrix(
	struct Scene_viewer_rendering_data *rendering_data)
/*******************************************************************************
LAST MODIFIED : 8 April 2003

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(Scene_viewer_apply_projection_matrix);
	if (rendering_data)
	{
		return_code = 1;

		glMatrixMode(GL_PROJECTION);

		glPushMatrix();

		glMultMatrixd(rendering_data->scene_viewer->window_projection_matrix);

		Scene_viewer_call_next_renderer(rendering_data);

		glMatrixMode(GL_PROJECTION);

		glPopMatrix();
	}
	else
	{
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_apply_projection_matrix */

/** Keeps a copy of the scene in a pixel buffer and only updates that image
  * when the scene_viewer->update_pixel_image flag is set.  This is used by the
  * emoter to make icons representing the current scene.*/
static int Scene_viewer_use_pixel_buffer(
	struct Scene_viewer_rendering_data *rendering_data)
{
	GLboolean valid_raster;
	GLdouble obj_x,obj_y,obj_z;
	static GLint viewport[4]={0,0,1,1};
	int return_code;
	struct Scene_viewer *scene_viewer;
	void *new_data;

	if (rendering_data && (scene_viewer = rendering_data->scene_viewer))
	{
		return_code = 1;
		if (scene_viewer->update_pixel_image)
		{
			/* Draw the scene first */
			Scene_viewer_call_next_renderer(rendering_data);

			/* Copy the image from the scene viewer */
			if (REALLOCATE(new_data,rendering_data->scene_viewer->pixel_data,char,
				3*(rendering_data->viewport_width+1)*(rendering_data->viewport_height+1)))
			{
				scene_viewer->pixel_data=new_data;
				glReadPixels(0,0,rendering_data->viewport_width,
					rendering_data->viewport_height,GL_RGB,GL_BYTE,
					scene_viewer->pixel_data);
				scene_viewer->pixel_width=rendering_data->viewport_width;
				scene_viewer->pixel_height=rendering_data->viewport_height;
				scene_viewer->update_pixel_image=0;
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Scene_viewer_use_pixel_buffer.  "
					"Unable to reallocate pixel dataspace");
				scene_viewer->pixel_width=0;
				scene_viewer->pixel_height=0;
			}
		}
		else
		{
			/* Just draw the pixel buffer back into the window */
			glClearColor((scene_viewer->background_colour).red,
				(scene_viewer->background_colour).green,
				(scene_viewer->background_colour).blue,
				(scene_viewer->background_colour).alpha);
			glClearDepth(1.0);
			glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
			glGetDoublev(GL_MODELVIEW_MATRIX,scene_viewer->modelview_matrix);
			glGetDoublev(GL_PROJECTION_MATRIX,scene_viewer->projection_matrix);
			/*				glGetIntegerv(GL_VIEWPORT,viewport);*/
			/* for OpenGL window z coordinates, 0.0=near_plane, 1.0=far */
			if (GL_TRUE==gluUnProject(0.0001,0.0001,0.1,
				scene_viewer->modelview_matrix,scene_viewer->projection_matrix,
				viewport,&obj_x,&obj_y,&obj_z))
			{
				glRasterPos3d(obj_x,obj_y,obj_z);
				glGetBooleanv(GL_CURRENT_RASTER_POSITION_VALID,&valid_raster);
				if (valid_raster)
				{
					glDrawPixels(scene_viewer->pixel_width,scene_viewer->pixel_height,
						GL_RGB,GL_BYTE,scene_viewer->pixel_data);
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"Scene_viewer_use_pixel_buffer.  "
						"Culled raster position for redraw");
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Scene_viewer_use_pixel_buffer.  Unable to unproject");
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE, "Scene_viewer_use_pixel_buffer.  Invalid argument(s)");
		return_code = 0;
	}
	return (return_code);
}

static int Scene_viewer_initialise_matrices_and_swap_buffers(
		struct Scene_viewer_rendering_data *rendering_data)
/*******************************************************************************
LAST MODIFIED : 9 April 2003

DESCRIPTION :
==============================================================================*/
{
	int return_code;
	struct Scene_viewer *scene_viewer;

	ENTER(Scene_viewer_initialise_matrices_and_swap_buffers);
	if (rendering_data && (scene_viewer = rendering_data->scene_viewer))
	{
		return_code = 1;

		/* load identity matrix for rendering normal scene */
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		Scene_viewer_call_next_renderer(rendering_data);

		if (rendering_data->rendering_double_buffered)
		{
			scene_viewer->swap_buffers=1;
		}
		else
		{
			scene_viewer->swap_buffers=0;
		}

		/* SAB  Reapply the projection matrix which was cleared by the
			last glPopMatrix (Apply projection is further down the stack)
			so that unproject gets the full transformation */
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glLoadMatrixd(scene_viewer->window_projection_matrix);

	}
	else
	{
		display_message(ERROR_MESSAGE, "Scene_viewer_initialise_matrices_and_swap_buffers.  "
			"Invalid arguments");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_initialise_matrices_and_swap_buffers */

static int Scene_viewer_apply_modelview_lights_and_clip_planes(
		struct Scene_viewer_rendering_data *rendering_data)
/*******************************************************************************
LAST MODIFIED : 9 April 2003

DESCRIPTION :
==============================================================================*/
{
	int i, return_code;
	struct Scene_viewer *scene_viewer;

	ENTER(Scene_viewer_apply_modelview_turn_on_lights_and_clip_planes);
	if (rendering_data && (scene_viewer = rendering_data->scene_viewer))
	{
		return_code = 1;

		/* ModelView matrix */
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		rendering_data->renderer->reset_lights();
		/* turn on lights that are part of the Scene_viewer,
			ie. headlamps */
		cmzn_lightiterator *lightIter = CREATE_LIST_ITERATOR(cmzn_light)(scene_viewer->list_of_lights);
		cmzn_light *light;
		while (0 != (light = cmzn_lightiterator_next_non_access(lightIter)))
		{
			rendering_data->renderer->cmzn_light_execute(light);
		}
		cmzn_lightiterator_destroy(&lightIter);

		glMultMatrixd(scene_viewer->modelview_matrix);
		/* turn on lights that are part of the Scene and fixed relative
			to it. Note the scene will have compiled them already. */

		/* Clip planes */
		for (i = 0 ; i < MAX_CLIP_PLANES ; i++)
		{
			if (scene_viewer->clip_planes_enable[i])
			{
				switch(i)
				{
					case 0:
					{
						glEnable(GL_CLIP_PLANE0);
						glClipPlane(GL_CLIP_PLANE0,
							&(scene_viewer->clip_planes[i * 4]));
					} break;
					case 1:
					{
						glEnable(GL_CLIP_PLANE1);
						glClipPlane(GL_CLIP_PLANE1,
							&(scene_viewer->clip_planes[i * 4]));
					} break;
					case 2:
					{
						glEnable(GL_CLIP_PLANE2);
						glClipPlane(GL_CLIP_PLANE2,
							&(scene_viewer->clip_planes[i * 4]));
					} break;
					case 3:
					{
						glEnable(GL_CLIP_PLANE3);
						glClipPlane(GL_CLIP_PLANE3,
							&(scene_viewer->clip_planes[i * 4]));
					} break;
					case 4:
					{
						glEnable(GL_CLIP_PLANE4);
						glClipPlane(GL_CLIP_PLANE4,
							&(scene_viewer->clip_planes[i * 4]));
					} break;
					case 5:
					{
						glEnable(GL_CLIP_PLANE5);
						glClipPlane(GL_CLIP_PLANE5,
							&(scene_viewer->clip_planes[i * 4]));
					} break;
				}
			}
			else
			{
				switch(i)
				{
					case 0:
					{
						glDisable(GL_CLIP_PLANE0);
					} break;
					case 1:
					{
						glDisable(GL_CLIP_PLANE1);
					} break;
					case 2:
					{
						glDisable(GL_CLIP_PLANE2);
					} break;
					case 3:
					{
						glDisable(GL_CLIP_PLANE3);
					} break;
					case 4:
					{
						glDisable(GL_CLIP_PLANE4);
					} break;
					case 5:
					{
						glDisable(GL_CLIP_PLANE5);
					} break;
				}
			}
		}

		Scene_viewer_call_next_renderer(rendering_data);

	}
	else
	{
		display_message(ERROR_MESSAGE, "Scene_viewer_apply_modelview_lights_and_clip_planes.  "
			"Invalid arguments");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_apply_modelview_lights_and_clip_planes */

static int Scene_viewer_render_background(
	struct Scene_viewer_rendering_data *rendering_data)
/*******************************************************************************
LAST MODIFIED : 14 March 2017

DESCRIPTION :
Renders the background into the scene.
==============================================================================*/
{
	int return_code;
	struct Scene_viewer *scene_viewer;

	ENTER(Scene_viewer_render_background);
	if (rendering_data && (scene_viewer = rendering_data->scene_viewer))
	{
		return_code = 1;

		/* clear the screen: colour buffer and depth buffer */
		glClearColor((scene_viewer->background_colour).red,
			(scene_viewer->background_colour).green,
			(scene_viewer->background_colour).blue,
			(scene_viewer->background_colour).alpha);
		glClearDepth(1.0);
		if (0 == rendering_data->renderer->get_current_layer())
		{
#if defined (WIN32_SYSTEM)
			/* Clear twice, if set then the glClear in the background will be called
				twice, which appears to work around a rendering bug on ATI windows driver 6.14.0010.6706 */
			if (1 == scene_viewer->clear_twice_flag)
			{
				/* Now that we have a current context check to see what the vendor is */
				if (Graphics_library_vendor_ati == Graphics_library_get_vendor_id())
				{
					/* Don't check again */
					scene_viewer->clear_twice_flag = 2;
				}
				else
				{
					/* Don't need work around */
					scene_viewer->clear_twice_flag = 0;
				}
			}
			if (scene_viewer->clear_twice_flag)
			{
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			}
#endif /* defined (WIN32_SYSTEM) */
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			if (scene_viewer->image_texture.texture)
			{
				glDisable(GL_LIGHTING);
				glColor3f((scene_viewer->background_colour).red,
					(scene_viewer->background_colour).green,
					(scene_viewer->background_colour).blue);
				Scene_viewer_render_background_texture(scene_viewer,
					rendering_data->viewport_width,rendering_data->viewport_height,
					rendering_data->renderer);
				glEnable(GL_LIGHTING);
			}
		}
		else
		{
			// clear only depth buffer between layers
			glClear(GL_DEPTH_BUFFER_BIT);
		}

		Scene_viewer_call_next_renderer(rendering_data);
	}
	else
	{
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_render_background */

static int Scene_viewer_slow_transparency(
	struct Scene_viewer_rendering_data *rendering_data)
/*******************************************************************************
LAST MODIFIED : 4 April 2003

DESCRIPTION :
Render the scene twice.  Once with opaque objects filling the depth buffer
and then again with only semi transparent objects not changing the depth buffer.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_viewer_slow_transparency);
	if (rendering_data)
	{
		return_code = 1;
		glEnable(GL_ALPHA_TEST);
		/* render only fragments with alpha = 1.0, write depth */
		glDepthMask(GL_TRUE);
		glAlphaFunc(GL_EQUAL,1.0);
		Scene_viewer_call_next_renderer(rendering_data);

		/* render fragments with alpha != 1.0; do not write into
			depth buffer */
		glDepthMask(GL_FALSE);
		glAlphaFunc(GL_NOTEQUAL,1.0);
		Scene_viewer_call_next_renderer(rendering_data);

		glDepthMask(GL_TRUE);
		glDisable(GL_ALPHA_TEST);
	}
	else
	{
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_slow_transparency */

static int Scene_viewer_antialias(
	struct Scene_viewer_rendering_data *rendering_data)
/*******************************************************************************
LAST MODIFIED : 7 April 2003

DESCRIPTION :
Render the scene multiple times perturbing the viewing frustrum to antialias
the entire scene.
==============================================================================*/
{
	ZnReal pixel_offset_x = 0.0, pixel_offset_y = 0.0;
	GLdouble temp_matrix[16];
	int accumulation_count, antialias, return_code;
	GLint framebuffer_flag = 0;

	ZnReal j2[2][2]=
		{
			{0.25,0.75},
			{0.75,0.25}
		};
	ZnReal j4[4][2]=
		{
			{0.375,0.25},
			{0.125,0.75},
			{0.875,0.25},
			{0.625,0.75}
		};
	ZnReal j8[8][2]=
		{
			{0.5625,0.4375},
			{0.0625,0.9375},
			{0.3125,0.6875},
			{0.6875,0.8125},
			{0.8125,0.1875},
			{0.9375,0.5625},
			{0.4375,0.0625},
			{0.1875,0.3125}
		};

	ENTER(Scene_viewer_antialias);

#if defined (GL_EXT_framebuffer_object)
	if (Graphics_library_check_extension(GL_EXT_framebuffer_object))
	{
		 glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &framebuffer_flag);
	}
#endif

	if (rendering_data)
	{
		return_code = 1;
		antialias = rendering_data->override_antialias;
		for (accumulation_count = 0 ; accumulation_count < antialias ;
			  accumulation_count++)
		{
			glMatrixMode(GL_PROJECTION);

			/* SAB This should more robustly be a glPushMatrix and glPopMatrix
				pair but the SGI implementations sometimes have only one
				level on the Projection matrix stack and so knowing that this
				matrix is the identity so far I can just reload that */
			glLoadIdentity();

			/********* CALCULATE ANTIALIAS OFFSET MATRIX *********/
			switch(antialias)
			{
				case 0:
				case 1:
				{
					/* Do nothing */
				} break;
				case 2:
				{
					pixel_offset_x=j2[accumulation_count][0]-0.5;
					pixel_offset_y=j2[accumulation_count][1]-0.5;
				} break;
				case 4:
				{
					pixel_offset_x=j4[accumulation_count][0]-0.5;
					pixel_offset_y=j4[accumulation_count][1]-0.5;
				} break;
				case 8:
				{
					pixel_offset_x=j8[accumulation_count][0]-0.5;
					pixel_offset_y=j8[accumulation_count][1]-0.5;
				} break;
				default:
				{
					display_message(ERROR_MESSAGE,
						"Scene_viewer_antialias.  "
						"Invalid antialias number");
					return_code=0;
				} break;
			}
			/* the projection matrix converts the viewing volume into the
				Normalised Device Coordinates ranging from -1 to +1 in each
				coordinate direction. Need to scale this range to fit the
				viewport by premultiplying with a matrix. First start with
				identity: Note that numbers go down columns first in OpenGL
				matrices */
			temp_matrix[0] = 1.0;
			temp_matrix[1] = 0.0;
			temp_matrix[2] = 0.0;
			temp_matrix[3] = 0.0;

			temp_matrix[4] = 0.0;
			temp_matrix[5] = 1.0;
			temp_matrix[6] = 0.0;
			temp_matrix[7] = 0.0;

			temp_matrix[8] = 0.0;
			temp_matrix[9] = 0.0;
			temp_matrix[10] = 1.0;
			temp_matrix[11] = 0.0;
			/* offsetting image by [sub]pixel distances for anti-aliasing.
				offset_x is distance image is shifted to the right, offset_y is
				distance image is shifted up. The actual offsets used are
				fractions of half the viewport width or height,since normalized
				device coordinates (NDCs) range from -1 to +1 */
			temp_matrix[12] = 2.0*pixel_offset_x/rendering_data->viewport_width;
			temp_matrix[13] = 2.0*pixel_offset_y/rendering_data->viewport_height;
			temp_matrix[14] = 0.0;
			temp_matrix[15] = 1.0;

			glMultMatrixd(temp_matrix);

			Scene_viewer_call_next_renderer(rendering_data);

			if (framebuffer_flag == (GLint)0)
			{
				if (0==accumulation_count)
				{
					glAccum(GL_LOAD,1.0f/((ZnReal)antialias));
				}
				else
				{
					glAccum(GL_ACCUM,1.0f/((ZnReal)antialias));
				}

			} /* for (antialias_count) */
		}
		/* We want to ensure that we return white when we accumulate a white
			background */
		if (framebuffer_flag == (GLint)0)
		{
			glAccum(GL_RETURN,1.001f);
		}
		glFlush();
	}
	else
	{
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_antialias */

static int Scene_viewer_depth_of_field(
	struct Scene_viewer_rendering_data *rendering_data)
/*******************************************************************************
LAST MODIFIED : 5 December 2006

DESCRIPTION :
Render the scene multiple times perturbing the viewing frustrum to create a
depth of field effect.
==============================================================================*/
{
	double depth_of_field, dx, dy, focal_depth, pixel_offset_x, pixel_offset_y;
	GLdouble temp_matrix[16];
	int accumulation_count, return_code;
	GLint framebuffer_flag = 0;
	GLfloat j8[8][2]=
		{
			{0.5625,0.4375},
			{0.0625,0.9375},
			{0.3125,0.6875},
			{0.6875,0.8125},
			{0.8125,0.1875},
			{0.9375,0.5625},
			{0.4375,0.0625},
			{0.1875,0.3125}
		};

	ENTER(Scene_viewer_depth_of_field);
#if defined (GL_EXT_framebuffer_object)
	if (Graphics_library_check_extension(GL_EXT_framebuffer_object))
	{
		 glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &framebuffer_flag);
	}
#endif
	if (rendering_data)
	{
		return_code = 1;
		for (accumulation_count = 0 ; accumulation_count < 8 ;
			  accumulation_count++)
		{
			glMatrixMode(GL_PROJECTION);

			/* SAB This should more robustly be a glPushMatrix and glPopMatrix
				pair but the SGI implementations sometimes have only one
				level on the Projection matrix stack and so knowing that this
				matrix is the identity so far I can just reload that */
			glLoadIdentity();

			pixel_offset_x=j8[accumulation_count][0];
			pixel_offset_y=j8[accumulation_count][1];

			focal_depth = rendering_data->scene_viewer->focal_depth;
			depth_of_field = rendering_data->scene_viewer->depth_of_field;

			dx = pixel_offset_x/(depth_of_field*rendering_data->viewport_width);
			dy = pixel_offset_y/(depth_of_field*rendering_data->viewport_height);

			/* the projection matrix converts the viewing volume into the
				Normalised Device Coordinates ranging from -1 to +1 in each
				coordinate direction. Need to scale this range to fit the
				viewport by premultiplying with a matrix. First start with
				identity: Note that numbers go down columns first in OpenGL
				matrices */
			temp_matrix[0] = 1.0;
			temp_matrix[1] = 0.0;
			temp_matrix[2] = 0.0;
			temp_matrix[3] = 0.0;

			temp_matrix[4] = 0.0;
			temp_matrix[5] = 1.0;
			temp_matrix[6] = 0.0;
			temp_matrix[7] = 0.0;

			temp_matrix[8] = dx / (1.0 - focal_depth);
			temp_matrix[9] = dy / (1.0 - focal_depth);
			temp_matrix[10] = 1.0;
			temp_matrix[11] = 0.0;
			/* offsetting image by [sub]pixel distances for anti-aliasing.
				offset_x is distance image is shifted to the right, offset_y is
				distance image is shifted up. The actual offsets used are
				fractions of half the viewport width or height,since normalized
				device coordinates (NDCs) range from -1 to +1 */
			temp_matrix[12] = -dx * focal_depth;
			temp_matrix[13] = -dy * focal_depth;
			temp_matrix[14] = 0.0;
			temp_matrix[15] = 1.0;

			glMultMatrixd(temp_matrix);

			Scene_viewer_call_next_renderer(rendering_data);
			if (framebuffer_flag == (GLint)0)
			{
				if (0==accumulation_count)
				{
					glAccum(GL_LOAD,1.0f/(8.0f));
				}
				else
				{
					glAccum(GL_ACCUM,1.0f/(8.0f));
				}
			}
		} /* for (antialias_count) */

		/* We want to ensure that we return white when we accumulate a white
			background */
		if (framebuffer_flag == (GLint)0)
		{
			glAccum(GL_RETURN,1.001f);
		}
		else
		{
			display_message(WARNING_MESSAGE, "Framebuffer object does not "
				"support accumulation buffer, depth of field is not available.\n");
		}
		glFlush();
	}
	else
	{
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_depth_of_field */

static int Scene_viewer_stereo(
	struct Scene_viewer_rendering_data *rendering_data)
/*******************************************************************************
LAST MODIFIED : 8 April 2003

DESCRIPTION :
Render the scene into the LEFT and RIGHT buffers perturbed slightly to generate
a stereo perspective.
==============================================================================*/
{
	double eye_distance,stereo_angle,stereo_cos,stereo_sin,view[3];
	int return_code;
	GLdouble stereo_matrix[16];
	struct Scene_viewer *scene_viewer;

	ENTER(Scene_viewer_stereo);
	if (rendering_data && (scene_viewer = rendering_data->scene_viewer))
	{
		return_code = 1;

		/* Calculate the angle */
		view[0] = scene_viewer->eyex - scene_viewer->lookatx;
		view[1] = scene_viewer->eyey - scene_viewer->lookaty;
		view[2] = scene_viewer->eyez - scene_viewer->lookatz;
		eye_distance = norm3(view);

		stereo_angle = 2.0*atan2(0.5*scene_viewer->stereo_eye_spacing,
			eye_distance);
		stereo_sin = sin(stereo_angle);
		stereo_cos = cos(stereo_angle);

		glMatrixMode(GL_PROJECTION);

		glPushMatrix();

		if (rendering_data->rendering_double_buffered)
		{
			glDrawBuffer(GL_BACK_LEFT);
		}
		else
		{
			glDrawBuffer(GL_FRONT_LEFT);
		}

		/* the projection matrix converts the viewing volume into the
			Normalised Device Coordinates ranging from -1 to +1 in each
			coordinate direction. Need to scale this range to fit the
			viewport by premultiplying with a matrix. First start with
			identity: Note that numbers go down columns first in OpenGL
			matrices */
		stereo_matrix[0] = stereo_cos;
		stereo_matrix[1] = 0.0;
		stereo_matrix[2] = stereo_sin;
		stereo_matrix[3] = 0.0;

		stereo_matrix[4] = 0.0;
		stereo_matrix[5] = 1.0;
		stereo_matrix[6] = 0.0;
		stereo_matrix[7] = 0.0;

		stereo_matrix[8] = -stereo_sin;
		stereo_matrix[9] = 0.0;
		stereo_matrix[10] = stereo_cos;
		stereo_matrix[11] = 0.0;

		stereo_matrix[12] = 0.0;
		stereo_matrix[13] = 0.0;
		stereo_matrix[14] = 0.0;
		stereo_matrix[15] = 1.0;

		glMultMatrixd(stereo_matrix);

		Scene_viewer_call_next_renderer(rendering_data);

		glMatrixMode(GL_PROJECTION);

		glPopMatrix();

		if (rendering_data->rendering_double_buffered)
		{
			glDrawBuffer(GL_BACK_RIGHT);
		}
		else
		{
			glDrawBuffer(GL_FRONT_RIGHT);
		}

		/* the projection matrix converts the viewing volume into the
			Normalised Device Coordinates ranging from -1 to +1 in each
			coordinate direction. Need to scale this range to fit the
			viewport by premultiplying with a matrix. First start with
			identity: Note that numbers go down columns first in OpenGL
			matrices */
		stereo_matrix[0] = stereo_cos;
		stereo_matrix[1] = 0.0;
		stereo_matrix[2] = -stereo_sin;
		stereo_matrix[3] = 0.0;

		stereo_matrix[4] = 0.0;
		stereo_matrix[5] = 1.0;
		stereo_matrix[6] = 0.0;
		stereo_matrix[7] = 0.0;

		stereo_matrix[8] = stereo_sin;
		stereo_matrix[9] = 0.0;
		stereo_matrix[10] = stereo_cos;
		stereo_matrix[11] = 0.0;

		stereo_matrix[12] = 0.0;
		stereo_matrix[13] = 0.0;
		stereo_matrix[14] = 0.0;
		stereo_matrix[15] = 1.0;

		glMultMatrixd(stereo_matrix);

		Scene_viewer_call_next_renderer(rendering_data);
	}
	else
	{
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_stereo */

static int Scene_viewer_initialise_order_independent_transparency(
	struct Scene_viewer_rendering_data *rendering_data)
/*******************************************************************************
LAST MODIFIED : 14 April 2003

DESCRIPTION :
Render the scene twice.  Once with opaque objects filling the depth buffer
and then again with only semi transparent objects not changing the depth buffer.
==============================================================================*/
{
	int return_code;
	struct Scene_viewer *scene_viewer;

	ENTER(Scene_viewer_initialise_order_independent_transparency);
	if (rendering_data && (scene_viewer = rendering_data->scene_viewer))
	{
		if (!scene_viewer->order_independent_transparency_data)
		{
			scene_viewer->order_independent_transparency_data =
				order_independent_initialise(scene_viewer);
		}

		if (scene_viewer->order_independent_transparency_data)
		{
			order_independent_reshape(scene_viewer->order_independent_transparency_data,
				rendering_data->viewport_width, rendering_data->viewport_height,
				rendering_data->override_transparency_layers,
				(rendering_data->stencil_depth > 0));
			return_code = 1;
		}
		else
		{
			return_code = 0;
		}

	}
	else
	{
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_initialise_order_independent_transparency */

static int Scene_viewer_order_independent_transparency(
	struct Scene_viewer_rendering_data *rendering_data)
/*******************************************************************************
LAST MODIFIED : 7 April 2003

DESCRIPTION :
Render the scene twice.  Once with opaque objects filling the depth buffer
and then again with only semi transparent objects not changing the depth buffer.
==============================================================================*/
{
	int return_code;
	struct Scene_viewer *scene_viewer;

	ENTER(Scene_viewer_order_independent_transparency);
	if (rendering_data && (scene_viewer = rendering_data->scene_viewer))
	{
		return_code = 1;

		if (scene_viewer->order_independent_transparency_data)
		{
			order_independent_display(rendering_data,
				scene_viewer->order_independent_transparency_data,
				scene_viewer->window_projection_matrix,
				scene_viewer->modelview_matrix, scene_viewer->blending_mode);

			if (rendering_data->rendering_double_buffered)
			{
				scene_viewer->swap_buffers=1;
			}
			else
			{
				scene_viewer->swap_buffers=0;
			}
		}
	}
	else
	{
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_order_independent_transparency */

static int Scene_viewer_render_scene_private(struct Scene_viewer *scene_viewer,
	int left, int bottom, int right, int top,
	int override_antialias, int override_transparency_layers)
/*******************************************************************************
LAST MODIFIED : 17 September 2002

DESCRIPTION :
Called to redraw the Scene_viewer scene after changes in the display lists or
viewing transformations.
<left>, <right>, <top> and <bottom> define the viewport to draw into, if they
are all zero then the scene_viewer->widget size is used instead.
If <override_antialias> or <override_transparency_layers> are non zero
then they override the default values for just this call.
There are convenience functions, cmzn_sceneviewer_render_scene,
Scene_viewer_render_scene_in_viewport to access this function.
==============================================================================*/
{
	GLboolean double_buffer = 0;
	int do_render,return_code;
	struct Scene_viewer_rendering_data rendering_data;
	struct Scene_viewer_render_object *render_object;

	ENTER(Scene_viewer_render_scene_private);
	if (scene_viewer)
	{
		return_code=1;
		if ((!left) && (!bottom) && (!right) && (!top))
		{
			rendering_data.viewport_left = Graphics_buffer_get_origin_x(scene_viewer->graphics_buffer);
			rendering_data.viewport_bottom = Graphics_buffer_get_origin_y(scene_viewer->graphics_buffer);
			rendering_data.viewport_width = Graphics_buffer_get_width(scene_viewer->graphics_buffer);
			rendering_data.viewport_height = Graphics_buffer_get_height(scene_viewer->graphics_buffer);
		}
		else
		{
			rendering_data.viewport_left = left;
			rendering_data.viewport_bottom = bottom;
			rendering_data.viewport_width = right - left;
			rendering_data.viewport_height = top - bottom;
		}

#if defined (DEBUG_CODE)
		printf ("Viewport data %d,%d %d,%d\n",
			rendering_data.viewport_left, rendering_data.viewport_bottom,
			rendering_data.viewport_width, rendering_data.viewport_height);
#endif /* defined (DEBUG_CODE) */

		rendering_data.scene_viewer = scene_viewer;
		rendering_data.render_callstack =
			(struct LIST(Scene_viewer_render_object) *)NULL;
		if (override_antialias > 0)
		{
			rendering_data.override_antialias = override_antialias;
		}
		else
		{
			rendering_data.override_antialias = scene_viewer->antialias;
		}
		if (override_transparency_layers > 0)
		{
			rendering_data.override_transparency_layers = override_transparency_layers;
		}
		else
		{
			rendering_data.override_transparency_layers = scene_viewer->transparency_layers;
		}
		/* Set further down */
		rendering_data.rendering_double_buffered = 0;
		rendering_data.stencil_depth = 0;

		/* only redraw if the drawing widget has area and neither it nor any of its
			 parents are unmanaged */
		do_render=(0<rendering_data.viewport_width) && (0<rendering_data.viewport_height)
			&& Graphics_buffer_is_visible(scene_viewer->graphics_buffer);
		if (do_render)
		{
			/* Calculate the transformations before doing the callback list */
			Scene_viewer_calculate_transformation(scene_viewer,
				rendering_data.viewport_width,rendering_data.viewport_height);

			/* Send the transform callback even if transform flag is not set, as local transformation need to be handled too */
			scene_viewer->transform_flag = 0;

			/* work out if the rendering is double buffered. Do not just look at
				the buffer_mode flag as it is overridden in cases such as printing
				the window. */
			glGetBooleanv(GL_DOUBLEBUFFER,&double_buffer);
			/* Make this visible to the rendering routines */
			rendering_data.rendering_double_buffered = double_buffer;

			/* Determine which renderer to use once now rather than when compiling
			 * each individual object.
			 */
			rendering_data.renderer =
				Render_graphics_opengl_create_vertex_buffer_object_renderer();
//#if defined (GL_VERSION_1_5)
//			/* Check for GL_ARB_vertex_buffer_object includes whether OpenGL version is 1.1 or
//			 * greater and we actually use the OpenGL 1.5 interface and just use this
//			 * flag to enable override control.
//			 */
//			if (Graphics_library_check_extension(GL_ARB_vertex_buffer_object))
//			{
//#define USE_DISPLAY_LIST = 0
//#if defined (USE_DISPLAY_LIST)
//				if (Graphics_library_check_extension(GL_display_lists)
//					/* Only allow vertex buffers and display lists to be used
//					 * together on Nvidia and Mesa as other drivers (at least some ATI and Intel)
//					 * generate a segfault in the driver on both linux and windows.
//					 * We can enable other drivers as they are proven reliable,
//					 * although if we adopt vertex buffers widely then display lists aren't
//					 * that useful any more.
//					 * See https://tracker.physiomeproject.org/show_bug.cgi?id=1533
//					 */
//					&& ((Graphics_library_vendor_nvidia == Graphics_library_get_vendor_id())
//					|| (Graphics_library_vendor_mesa == Graphics_library_get_vendor_id())))
//				{
//					rendering_data.renderer =
//						Render_graphics_opengl_create_vertex_buffer_object_display_list_renderer();
//				}
//				else
//				{
//					rendering_data.renderer =
//						Render_graphics_opengl_create_vertex_buffer_object_renderer();
//				}
//
//#else
//				rendering_data.renderer =
//					Render_graphics_opengl_create_vertex_buffer_object_renderer();
//#endif
//			}
//			else
//#endif /* defined (GL_VERSION_1_5) */
//#if defined (GL_VERSION_1_1)
//			/* Check for GL_EXT_vertex_array includes whether OpenGL version is 1.1 or
//			 * greater and we actually use the OpenGL 1.1 interface and just use this
//			 * flag to enable override control.
//			 */
//			if (Graphics_library_check_extension(GL_EXT_vertex_array))
//			{
//				if (Graphics_library_check_extension(GL_display_lists))
//				{
//					rendering_data.renderer =
//						Render_graphics_opengl_create_client_vertex_arrays_display_list_renderer();
//				}
//				else
//				{
//					rendering_data.renderer =
//						Render_graphics_opengl_create_client_vertex_arrays_renderer();
//				}
//			}
//			else
//#endif /* defined (GL_VERSION_1_1) */
//			{
//				if (Graphics_library_check_extension(GL_display_lists))
//				{
//					rendering_data.renderer =
//						Render_graphics_opengl_create_glbeginend_display_list_renderer();
//				}
//				else
//				{
//					rendering_data.renderer =
//						Render_graphics_opengl_create_glbeginend_renderer();
//				}
//			}

			rendering_data.renderer->set_world_view_matrix(scene_viewer->modelview_matrix);
			rendering_data.renderer->viewport_width = (double)rendering_data.viewport_width;
			rendering_data.renderer->viewport_height = (double)rendering_data.viewport_height;
			double NDC_left, NDC_top, NDC_width, NDC_height;
			Scene_viewer_get_NDC_info(scene_viewer, &NDC_left,&NDC_top,&NDC_width,&NDC_height);
			rendering_data.renderer->NDC_width = NDC_width;
			rendering_data.renderer->NDC_height = NDC_height;
			rendering_data.renderer->NDC_left = NDC_left;
			rendering_data.renderer->NDC_top = NDC_top;
			GraphicsIncrementalBuild incrementalBuild;
			rendering_data.renderer->setIncrementalBuild(&incrementalBuild);
			rendering_data.renderer->Scene_compile(scene_viewer->scene, scene_viewer->filter);

			rendering_data.render_callstack = CREATE(LIST(Scene_viewer_render_object))();
			/* Add functionality to the render callstack */

			if (SCENE_VIEWER_NO_INPUT_OR_DRAW==scene_viewer->input_mode)
			{
				glClearColor(0.6f,0.6f,0.6f,0.0);
				glClearDepth(1.0);
				glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
				return_code=1;
			}
			else
			{
				if (SCENE_VIEWER_PIXEL_BUFFER==scene_viewer->buffering_mode)
				{
					render_object = CREATE(Scene_viewer_render_object)(
						Scene_viewer_use_pixel_buffer);
					ADD_OBJECT_TO_LIST(Scene_viewer_render_object)(render_object,
						rendering_data.render_callstack);
				}

				/* Initialise the matricies and handle the double buffer flag */
				render_object = CREATE(Scene_viewer_render_object)(
					Scene_viewer_initialise_matrices_and_swap_buffers);
				ADD_OBJECT_TO_LIST(Scene_viewer_render_object)(render_object,
					rendering_data.render_callstack);

				if (rendering_data.override_antialias > 1)
				{
					render_object = CREATE(Scene_viewer_render_object)(
						Scene_viewer_antialias);
					ADD_OBJECT_TO_LIST(Scene_viewer_render_object)(render_object,
						rendering_data.render_callstack);
				}

				if (scene_viewer->depth_of_field > 0.0)
				{
					render_object = CREATE(Scene_viewer_render_object)(
						Scene_viewer_depth_of_field);
					ADD_OBJECT_TO_LIST(Scene_viewer_render_object)(render_object,
						rendering_data.render_callstack);
				}

				render_object = CREATE(Scene_viewer_render_object)(
					Scene_viewer_render_layers);
				ADD_OBJECT_TO_LIST(Scene_viewer_render_object)(render_object,
					rendering_data.render_callstack);

				/* Render the background */
				render_object = CREATE(Scene_viewer_render_object)(
					Scene_viewer_render_background);
				ADD_OBJECT_TO_LIST(Scene_viewer_render_object)(render_object,
					rendering_data.render_callstack);

				/* Apply the modelview matrix, lights and clip planes */
				render_object = CREATE(Scene_viewer_render_object)(
					Scene_viewer_apply_modelview_lights_and_clip_planes);
				ADD_OBJECT_TO_LIST(Scene_viewer_render_object)(render_object,
					rendering_data.render_callstack);

				if (CMZN_SCENEVIEWER_STEREO_MODE_STEREO == scene_viewer->stereo_mode)
				{
					render_object = CREATE(Scene_viewer_render_object)(
						Scene_viewer_stereo);
					ADD_OBJECT_TO_LIST(Scene_viewer_render_object)(render_object,
						rendering_data.render_callstack);
				}

				switch (scene_viewer->transparency_mode)
				{
					case CMZN_SCENEVIEWER_TRANSPARENCY_MODE_SLOW:
					{
						render_object = CREATE(Scene_viewer_render_object)(
							Scene_viewer_slow_transparency);
						ADD_OBJECT_TO_LIST(Scene_viewer_render_object)(render_object,
							rendering_data.render_callstack);
					} break;
					case CMZN_SCENEVIEWER_TRANSPARENCY_MODE_ORDER_INDEPENDENT:
					{
						Scene_viewer_initialise_order_independent_transparency(&rendering_data);

						render_object = CREATE(Scene_viewer_render_object)(
							Scene_viewer_order_independent_transparency);
						ADD_OBJECT_TO_LIST(Scene_viewer_render_object)(render_object,
							rendering_data.render_callstack);
					}
					default:
					{
						/* Do nothing */
					} break;
				}

				render_object = CREATE(Scene_viewer_render_object)(
					Scene_viewer_apply_projection_matrix);
				ADD_OBJECT_TO_LIST(Scene_viewer_render_object)(render_object,
					rendering_data.render_callstack);

				/* Always add this to the stack last */
				render_object = CREATE(Scene_viewer_render_object)(
					Scene_viewer_execute_scene_non_fastchanging);
				ADD_OBJECT_TO_LIST(Scene_viewer_render_object)(render_object,
					rendering_data.render_callstack);

				return_code=1;

				if (scene_viewer->perturb_lines)
				{
					glPolygonOffset(1.5f,0.000001f);
					glEnable(GL_POLYGON_OFFSET_FILL);
				}
				else
				{
					glDisable(GL_POLYGON_OFFSET_FILL);
				}

#if defined (DEBUG_CODE)
				/*???debug*/
				printf("Scene_viewer: build scene and redraw\n");
#endif /* defined (DEBUG_CODE) */

				glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);

				/* depth tests are against a normalised z coordinate (i.e. [0..1])
					so the following sets this up and turns on the test */

				GLint framebuffer_flag = 0;
#if defined (GL_EXT_framebuffer_object)
				if (Graphics_library_check_extension(GL_EXT_framebuffer_object) &&
					Graphics_library_load_extension("GL_EXT_framebuffer_object"))
				{
					glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &framebuffer_flag);
				}
#endif
				if (!framebuffer_flag)
				{
					if (CMZN_SCENEVIEWER_STEREO_MODE_STEREO != scene_viewer->stereo_mode)
					{
						if (double_buffer)
						{
							glDrawBuffer(GL_BACK);
							/* Multipass rendering types need to have the correct read buffer */
							glReadBuffer(GL_BACK);
						}
						else
						{
							glDrawBuffer(GL_FRONT);
							/* Multipass rendering types need to have the correct read buffer */
							glReadBuffer(GL_FRONT);
						}
					}
				}
				else
				{
#if defined (GL_EXT_framebuffer_object)
					/* framebuffer object is currently bound,
						 assume color_attachment0 is the texture to write too*/
					glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
					glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
#endif
				}
				glDepthRange((GLclampd)0.0,(GLclampd)1.0);
				glDepthMask(GL_TRUE);
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_LESS);
				/* Get size of alpha [blending] buffer. */
				/* glGetIntegerv(GL_ALPHA_BITS,&alpha_bits); */
				/* turn on alpha */
				switch(scene_viewer->blending_mode)
				{
					default:
					case CMZN_SCENEVIEWER_BLENDING_MODE_NORMAL:
					{
						glEnable(GL_BLEND);
						glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
					} break;
					case CMZN_SCENEVIEWER_BLENDING_MODE_NONE:
					{
						glDisable(GL_BLEND);
					} break;
#if defined GL_VERSION_1_4
					case CMZN_SCENEVIEWER_BLENDING_MODE_TRUE_ALPHA:
					{
						/* This function is protected at runtime by testing in the set
							blending mode function */
						glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
							GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
						glEnable(GL_BLEND);
					} break;
#endif /* defined GL_VERSION_1_4 */
				}
				glViewport((GLint)rendering_data.viewport_left,
					(GLint)rendering_data.viewport_bottom,
					(GLint)rendering_data.viewport_width,
					(GLint)rendering_data.viewport_height);

				//glScissor((GLint)rendering_data.viewport_left,
				//	(GLint)rendering_data.viewport_bottom,
				//	(GLint)rendering_data.viewport_width,
				//	(GLint)rendering_data.viewport_height);
				//glEnable(GL_SCISSOR_TEST);

				/* glPushAttrib(GL_VIEWPORT_BIT); */

				rendering_data.renderer->reset_lights();

				/* light model */
				if (0 < NUMBER_IN_LIST(cmzn_light)(scene_viewer->list_of_lights))
				{
					Colour ambientColour = Light_list_get_total_ambient_colour(scene_viewer->list_of_lights);
					rendering_data.renderer->Light_model_enable(ambientColour, scene_viewer->isLightingLocalViewer(), scene_viewer->isLightingTwoSided());
				}
				else
				{
					rendering_data.renderer->Light_model_disable();
				}

				/********* CALL THE RENDERING CALLSTACK **********/
				Scene_viewer_call_next_renderer(&rendering_data);

				glFlush();
			}
#if defined (REPORT_GL_ERRORS)
			{
				char message[200];
				GLenum error;
				int max_error = 200;
				while((max_error--) && (GL_NO_ERROR!=(error = glGetError())))
				{
					strcpy(message,"Scene_viewer_render_scene_private: GL ERROR ");
					strcat(message, (char *)gluErrorString(error));
					display_message(ERROR_MESSAGE, message);
				}
			}
#endif /* defined (REPORT_GL_ERRORS) */
			DESTROY(LIST(Scene_viewer_render_object))(&rendering_data.render_callstack);
			delete rendering_data.renderer;

			if (incrementalBuild.isMoreWorkToDo())
				// request another redraw to build some more graphics
				scene_viewer->scene->setChanged();
		}
		scene_viewer->frame_count++;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_render_scene_private.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_render_scene_private */

int cmzn_sceneviewer_render_scene(struct Scene_viewer *sceneviewer)
{
	int return_code;
	if (sceneviewer)
	{
		return_code = Scene_viewer_render_scene_private(sceneviewer,
			/*left*/0, /*bottom*/0, /*right*/0, /*top*/0, /*override_antialias*/0,
			/*override_transparency_layers*/0);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_sceneviewer_render_scene.  Missing scene_viewer");
		return_code=0;
	}
	return (return_code);
}

int Scene_viewer_render_scene_in_viewport(struct Scene_viewer *scene_viewer,
	int left, int bottom, int right, int top)
/*******************************************************************************
LAST MODIFIED : 1 September 1999

DESCRIPTION :
Called to redraw the Scene_viewer scene after changes in the display lists or
viewing transformations.  Uses the specified viewport to draw into (unless
all the dimensions are zero).
==============================================================================*/
{
	int return_code;

	ENTER(Scene_viewer_render_scene_in_viewport);
	if (scene_viewer)
	{
		return_code=Scene_viewer_render_scene_private(scene_viewer,
			left, bottom, right, top, /*override_antialias*/0,
			/*override_transparency_layers*/0);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_render_scene_in_viewport.  Missing scene_viewer");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_render_scene_in_viewport */

int Scene_viewer_render_scene_in_viewport_with_overrides(
	struct Scene_viewer *scene_viewer, int left, int bottom, int right, int top,
	int antialias, int transparency_layers, int /*drawing_offscreen*/)
/*******************************************************************************
LAST MODIFIED : 11 December 2002

DESCRIPTION :
Called to redraw the Scene_viewer scene after changes in the display lists or
viewing transformations.  Uses the specified viewport to draw into (unless
all the dimensions are zero).  If non_zero then the supplied <antialias> and
<transparency_layers> are used for just this render.
==============================================================================*/
{
	int return_code = 1;

	ENTER(Scene_viewer_render_scene_in_viewport_with_overrides);
	if (scene_viewer)
	{
		return_code=Scene_viewer_render_scene_private(scene_viewer,
			left, bottom, right, top, antialias, transparency_layers);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_render_scene_in_viewport_with_overrides.  Missing scene_viewer");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_render_scene_in_viewport_with_overrides */

static int Scene_viewer_unproject(int pointer_x,int pointer_y,
	double *near_x,double *near_y,double *near_z,
	double *far_x,double *far_y,double *far_z)
/*******************************************************************************
LAST MODIFIED : 13 July 2000

DESCRIPTION :
Converts the pointer location into locations on the near_plane and far planes in
world space.
==============================================================================*/
{
	int return_code;
	GLdouble modelview_matrix[16],projection_matrix[16],obj_x,obj_y,obj_z,
		win_x,win_y;
	GLint viewport[4];

	ENTER(Scene_viewer_unproject);
	if (near_x&&near_y&&near_z&&far_x&&far_y&&far_z)
	{
		glGetDoublev(GL_MODELVIEW_MATRIX,modelview_matrix);
		glGetDoublev(GL_PROJECTION_MATRIX,projection_matrix);
		glGetIntegerv(GL_VIEWPORT,viewport);
		return_code=0;
		win_x=(GLdouble)pointer_x;
		win_y=(GLdouble)(viewport[3]-pointer_y);
		/* for OpenGL window z coordinates, 0.0=near_plane, 1.0=far */
		if (GL_TRUE==gluUnProject(win_x,win_y,0.0,
			modelview_matrix,projection_matrix,viewport,&obj_x,&obj_y,&obj_z))
		{
			*near_x=(double)obj_x;
			*near_y=(double)obj_y;
			*near_z=(double)obj_z;
			if (GL_TRUE==gluUnProject(win_x,win_y,1.0,
				modelview_matrix,projection_matrix,viewport,&obj_x,&obj_y,&obj_z))
			{
				*far_x=(double)obj_x;
				*far_y=(double)obj_y;
				*far_z=(double)obj_z;
				return_code=1;
			}
		}
		if (!return_code)
		{
			display_message(ERROR_MESSAGE,
				"Scene_viewer_unproject.  Unable to unproject");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_unproject.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_unproject */

/***************************************************************************//**
 * Something has changed in the regional computed field manager.
 * Check if the field being used is changed, if so update the scene viewer.
 */
static void Scene_viewer_image_field_change(
	struct MANAGER_MESSAGE(Computed_field) *message, void *scene_viewer_image_texture_void)
{
	struct Scene_viewer_image_texture *image_texture =
		(struct Scene_viewer_image_texture *)scene_viewer_image_texture_void;
	if (message && image_texture)
	{
		int change = MANAGER_MESSAGE_GET_OBJECT_CHANGE(Computed_field)(
				message, cmzn_field_image_base_cast(image_texture->field));
		if (change & MANAGER_CHANGE_RESULT(Computed_field))
		{
			REACCESS(Texture)(&(image_texture->texture),
			cmzn_field_image_get_texture(image_texture->field));
			image_texture->scene_viewer->setChangedRepaint();
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_image_field_change.  Invalid argument(s)");
	}
}

/***************************************************************************//**
 * Set the field and update all the related objects in scene_viewer_image_texture.
 * This will also create a callback for computed field.
 */
int Scene_viewer_image_texture_set_field(struct Scene_viewer_image_texture *image_texture,
	cmzn_field_image_id field)
{
	int return_code = 0;
	if (image_texture)
	{
		return_code = 1;
		if (image_texture->field)
		{
			cmzn_field_image_destroy(&(image_texture->field));
			image_texture->field=NULL;
			if (image_texture->manager &&	image_texture->callback_id)
			{
				MANAGER_DEREGISTER(Computed_field)(image_texture->callback_id,
						image_texture->manager);
				image_texture->callback_id = NULL;
			}
			if (image_texture->texture)
				DEACCESS(Texture)(&(image_texture->texture));
		}
		if (field)
		{
			struct cmzn_region *temp_region = Computed_field_get_region(cmzn_field_image_base_cast(field));
			MANAGER(Computed_field) *field_manager = temp_region->getFieldManager();
			if (field_manager)
			{
				image_texture->callback_id=
					MANAGER_REGISTER(Computed_field)(Scene_viewer_image_field_change,
						(void *)image_texture, field_manager);
				image_texture->manager = field_manager;
				image_texture->field = field;
				cmzn_field_access(cmzn_field_image_base_cast(field));
				image_texture->texture = ACCESS(Texture)(
					cmzn_field_image_get_texture(image_texture->field));
				return_code = 1;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			 "Material_image_texture_set_field.  Missing Material_image_texture");
		return_code = 0;
	}

	return return_code;
}

/*
Global functions
----------------
*/

void cmzn_sceneviewermodule_scenefilter_manager_callback(
	struct MANAGER_MESSAGE(cmzn_scenefilter) *message, void *sceneviewermodule_void);

void cmzn_sceneviewermodule_light_manager_callback(
	struct MANAGER_MESSAGE(cmzn_light) *message, void *sceneviewermodule_void);

static int Scene_viewer_destroy_from_module(
	struct Scene_viewer *scene_viewer, void *module_void)
{
	int return_code;
	struct cmzn_sceneviewermodule *module;

	if (scene_viewer && (module = (struct cmzn_sceneviewermodule *)module_void))
	{
		/* destroy function will also remove scene viewer from module.
		* Removing the pointer to module here to prevent the list being modified.
		*/
		scene_viewer->module = 0;
		cmzn_sceneviewer_destroy(&scene_viewer);
	}
	return_code = 1;

	return (return_code);
}

cmzn_sceneviewermodule::cmzn_sceneviewermodule(struct Colour *background_colourIn,
		cmzn_lightmodule *lightmoduleIn, struct cmzn_light *default_lightIn,
		struct cmzn_light *default_ambient_lightIn,
		cmzn_scenefiltermodule_id scenefiltermoduleIn) :
	graphics_buffer_package(CREATE(Graphics_buffer_package)()),
	background_colour(*background_colourIn),
	lightModule(cmzn_lightmodule_access(lightmoduleIn)),
	default_light(cmzn_light_access(default_lightIn)),
	default_ambient_light(cmzn_light_access(default_ambient_lightIn)),
	filterModule(cmzn_scenefiltermodule_access(scenefiltermoduleIn)),
	scene_viewer_list(CREATE(LIST(Scene_viewer))()),
	destroy_callback_list(CREATE(LIST(CMZN_CALLBACK_ITEM(cmzn_sceneviewermodule_callback)))()),
	scenefilter_manager_callback_id(MANAGER_REGISTER(cmzn_scenefilter)(
		cmzn_sceneviewermodule_scenefilter_manager_callback, (void *)this,
		cmzn_scenefiltermodule_get_manager(this->filterModule))),
	light_manager_callback_id(MANAGER_REGISTER(cmzn_light)(
		cmzn_sceneviewermodule_light_manager_callback, (void *)this,
		cmzn_lightmodule_get_manager(this->lightModule))),
	access_count(1)
{
}

cmzn_sceneviewermodule::~cmzn_sceneviewermodule()
{
	// Call the destroy callbacks
	CMZN_CALLBACK_LIST_CALL(cmzn_sceneviewermodule_callback)(
		this->destroy_callback_list, this, NULL);
	DESTROY(LIST(CMZN_CALLBACK_ITEM(cmzn_sceneviewermodule_callback)))
		(&this->destroy_callback_list);

	// Destroy the scene viewers in the list as they are not accessed or
	// deaccessed by the list (so not destroyed when delisted). The owners of
	// these scene viewers should by virtue of having destroyed the command data
	// and therefore this module no longer reference these scene viewers or
	// they should register for destroy callbacks.
	FOR_EACH_OBJECT_IN_LIST(Scene_viewer)(Scene_viewer_destroy_from_module,
		this, this->scene_viewer_list);
	DESTROY(LIST(Scene_viewer))(&this->scene_viewer_list);
	DESTROY(Graphics_buffer_package)(&this->graphics_buffer_package);

	MANAGER_DEREGISTER(cmzn_scenefilter)(
		this->scenefilter_manager_callback_id,
		cmzn_scenefiltermodule_get_manager(this->filterModule));
	cmzn_scenefiltermodule_destroy(&this->filterModule);
	MANAGER_DEREGISTER(cmzn_light)(
		this->light_manager_callback_id,
		cmzn_lightmodule_get_manager(this->lightModule));
	cmzn_lightmodule_destroy(&this->lightModule);
	cmzn_light_destroy(&this->default_light);
	cmzn_light_destroy(&this->default_ambient_light);
}

cmzn_sceneviewermodule *cmzn_sceneviewermodule::create(struct Colour *background_colourIn,
	cmzn_lightmodule *lightmoduleIn, struct cmzn_light *default_lightIn,
	struct cmzn_light *default_ambient_lightIn,
	cmzn_scenefiltermodule_id scenefiltermoduleIn)
{
	if (!((background_colourIn) && (lightmoduleIn) && (scenefiltermoduleIn)))
	{
		display_message(ERROR_MESSAGE, "cmzn_sceneviewermodule::create.  Invalid argument(s)");
		return 0;
	}
	auto sceneviewermodule = new cmzn_sceneviewermodule(background_colourIn,
		lightmoduleIn, default_lightIn, default_ambient_lightIn, scenefiltermoduleIn);
	if (!((sceneviewermodule) && (sceneviewermodule->graphics_buffer_package) &&
		(sceneviewermodule->scene_viewer_list) && (sceneviewermodule->destroy_callback_list)))
	{
		display_message(ERROR_MESSAGE, "cmzn_sceneviewermodule::create.  Failed");
		delete sceneviewermodule;
		return 0;
	}
	return sceneviewermodule;
}

int cmzn_sceneviewermodule::getDefaultBackgroundColourRGB(double *valuesOut3) const
{
	if (valuesOut3)
	{
		valuesOut3[0] = this->background_colour.red;
		valuesOut3[1] = this->background_colour.green;
		valuesOut3[2] = this->background_colour.blue;
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_sceneviewermodule::setDefaultBackgroundColourRGB(const double *valuesIn3)
{
	if (valuesIn3)
	{
		this->background_colour.red = valuesIn3[0];
		this->background_colour.green = valuesIn3[1];
		this->background_colour.blue = valuesIn3[2];
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_sceneviewermodule::getDefaultBackgroundColourRGBA(double *valuesOut4) const
{
	if (valuesOut4)
	{
		valuesOut4[0] = this->background_colour.red;
		valuesOut4[1] = this->background_colour.green;
		valuesOut4[2] = this->background_colour.blue;
		valuesOut4[3] = this->background_colour.alpha;
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_sceneviewermodule::setDefaultBackgroundColourRGBA(const double *valuesIn4)
{
	if (valuesIn4)
	{
		this->background_colour.red = valuesIn4[0];
		this->background_colour.green = valuesIn4[1];
		this->background_colour.blue = valuesIn4[2];
		this->background_colour.alpha = valuesIn4[3];
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

double cmzn_sceneviewermodule_get_default_background_colour_alpha(
	cmzn_sceneviewermodule_id sceneviewermodule)
{
	if (sceneviewermodule)
		return sceneviewermodule->getDefaultBackgroundColourAlpha();
	return 0.0;
}

int cmzn_sceneviewermodule_set_default_background_colour_alpha(
	cmzn_sceneviewermodule_id sceneviewermodule, double alpha)
{
	if (sceneviewermodule)
		return sceneviewermodule->setDefaultBackgroundColourAlpha(alpha);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_sceneviewermodule_get_default_background_colour_rgb(
	cmzn_sceneviewermodule_id sceneviewermodule, double *valuesOut3)
{
	if (sceneviewermodule)
		return sceneviewermodule->getDefaultBackgroundColourRGB(valuesOut3);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_sceneviewermodule_set_default_background_colour_rgb(
	cmzn_sceneviewermodule_id sceneviewermodule, const double *valuesIn3)
{
	if (sceneviewermodule)
		return sceneviewermodule->setDefaultBackgroundColourRGB(valuesIn3);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_sceneviewermodule_get_default_background_colour_rgba(
	cmzn_sceneviewermodule_id sceneviewermodule, double *valuesOut4)
{
	if (sceneviewermodule)
		return sceneviewermodule->getDefaultBackgroundColourRGBA(valuesOut4);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_sceneviewermodule_set_default_background_colour_rgba(
	cmzn_sceneviewermodule_id sceneviewermodule, const double *valuesIn4)
{
	if (sceneviewermodule)
		return sceneviewermodule->setDefaultBackgroundColourRGBA(valuesIn4);
	return CMZN_ERROR_ARGUMENT;
}

cmzn_sceneviewermodule_id cmzn_sceneviewermodule_access(cmzn_sceneviewermodule_id sceneviewermodule)
{
	if (sceneviewermodule)
		return sceneviewermodule->access();
	return 0;
}

int cmzn_sceneviewermodule_destroy(cmzn_sceneviewermodule_id *sceneviewermodule_address)
{
	if (sceneviewermodule_address)
		return cmzn_sceneviewermodule::deaccess(*sceneviewermodule_address);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_sceneviewermodule_add_destroy_callback(
	struct cmzn_sceneviewermodule *sceneviewermodule,
	CMZN_CALLBACK_FUNCTION(cmzn_sceneviewermodule_callback) *function,
	void *user_data)
/*******************************************************************************
LAST MODIFIED : 24 January 2007

DESCRIPTION :
Adds a callback to the <sceneviewermodule> that is called back before the scene
viewer is destroyed.
==============================================================================*/
{
	int return_code;

	ENTER(cmzn_sceneviewermodule_add_destroy_callback);
	if (sceneviewermodule&&function)
	{
		if (CMZN_CALLBACK_LIST_ADD_CALLBACK(cmzn_sceneviewermodule_callback)(
			sceneviewermodule->destroy_callback_list,function,user_data))
		{
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"cmzn_sceneviewermodule_add_destroy_callback.  Could not add callback");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_sceneviewermodule_add_destroy_callback.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_sceneviewermodule_add_destroy_callback */

int cmzn_sceneviewermodule_remove_destroy_callback(
	struct cmzn_sceneviewermodule *sceneviewermodule,
	CMZN_CALLBACK_FUNCTION(cmzn_sceneviewermodule_callback) *function,
	void *user_data)
/*******************************************************************************
LAST MODIFIED : 24 January 2007

DESCRIPTION :
Removes the callback calling <function> with <user_data> from
<sceneviewermodule>.
==============================================================================*/
{
	int return_code;

	ENTER(cmzn_sceneviewermodule_remove_destroy_callback);
	if (sceneviewermodule&&function)
	{
		if (CMZN_CALLBACK_LIST_REMOVE_CALLBACK(cmzn_sceneviewermodule_callback)(
			sceneviewermodule->destroy_callback_list,function,user_data))
		{
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"cmzn_sceneviewermodule_remove_destroy_callback.  Could not remove callback");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_sceneviewermodule_remove_destroy_callback.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_sceneviewermodule_remove_destroy_callback */

struct Graphics_buffer_package *cmzn_sceneviewermodule_get_graphics_buffer_package(
	struct cmzn_sceneviewermodule *sceneviewermodule)
/*******************************************************************************
LAST MODIFIED : 19 January 2007

DESCRIPTION :
==============================================================================*/
{
	struct Graphics_buffer_package *graphics_buffer_package;

	ENTER(Scene_viewer_get_graphics_buffer_package);
	if (sceneviewermodule)
	{
		graphics_buffer_package =
			sceneviewermodule->graphics_buffer_package;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_get_graphics_buffer_package.  Missing scene_viewer");
		graphics_buffer_package=(struct Graphics_buffer_package *)NULL;
	}
	LEAVE;

	return (graphics_buffer_package);
} /* Scene_viewer_get_graphics_buffer_package */

struct Scene_viewer *CREATE(Scene_viewer)(struct Graphics_buffer *graphics_buffer,
	struct Colour *background_colour,
	struct cmzn_light *default_light,
	struct cmzn_light *default_ambient_light,
	cmzn_scenefilter_id filter)
/*******************************************************************************
LAST MODIFIED : 19 September 2002

DESCRIPTION :
Creates a Scene_viewer in the widget <parent> to display <scene>.
Note: the parent must be an XmForm since form constraints will be applied.
If any of light_manager or scene_manager
are supplied, the scene_viewer will automatically redraw in response to changes
of objects from these managers that are in use by the scene_viewer. Redraws are
performed in idle time so that multiple redraws are avoided.
==============================================================================*/
{
	enum Graphics_buffer_buffering_mode graphics_buffer_buffering_mode = GRAPHICS_BUFFER_DOUBLE_BUFFERING;
	enum Graphics_buffer_stereo_mode graphics_buffer_stereo_mode = GRAPHICS_BUFFER_MONO;
	enum Scene_viewer_buffering_mode buffering_mode = SCENE_VIEWER_PIXEL_BUFFER;
	int return_code,i;
	struct Scene_viewer *scene_viewer;

	if (graphics_buffer && background_colour && default_ambient_light &&
		Graphics_buffer_get_buffering_mode(graphics_buffer,&graphics_buffer_buffering_mode) &&
		Graphics_buffer_get_stereo_mode(graphics_buffer, &graphics_buffer_stereo_mode))
	{
		return_code=1;
		switch(graphics_buffer_buffering_mode)
		{
			case GRAPHICS_BUFFER_SINGLE_BUFFERING:
			{
				buffering_mode = SCENE_VIEWER_SINGLE_BUFFER;
			} break;
			case GRAPHICS_BUFFER_DOUBLE_BUFFERING:
			case GRAPHICS_BUFFER_ANY_BUFFERING_MODE:
			{
				buffering_mode = SCENE_VIEWER_DOUBLE_BUFFER;
			} break;
			default:
			{
				display_message(ERROR_MESSAGE, "CREATE(Scene_viewer).  "
					"Invalid Graphics_buffer_buffering_mode.");
				return_code = 0;
			} break;
		}
		enum cmzn_sceneviewer_stereo_mode stereo_mode;
		switch(graphics_buffer_stereo_mode)
		{
			case GRAPHICS_BUFFER_MONO:
			case GRAPHICS_BUFFER_ANY_STEREO_MODE:
			{
				stereo_mode = CMZN_SCENEVIEWER_STEREO_MODE_MONO;
			} break;
			case GRAPHICS_BUFFER_STEREO:
			{
				stereo_mode = CMZN_SCENEVIEWER_STEREO_MODE_STEREO;
			} break;
			default:
			{
				display_message(ERROR_MESSAGE, "CREATE(Scene_viewer).  "
					"Invalid Graphics_buffer_stereo_mode.");
				return_code = 0;
			} break;
		}
		if (return_code)
		{
			/* allocate memory for the scene_viewer structure */
			ALLOCATE(scene_viewer, Scene_viewer, 1);
			if (scene_viewer &&
				(scene_viewer->list_of_lights=CREATE(LIST(cmzn_light)())))
			{
				scene_viewer->access_count = 1;
				scene_viewer->filter = cmzn_scenefilter_access(filter);
				scene_viewer->graphics_buffer=ACCESS(Graphics_buffer)(graphics_buffer);
				/* access the scene, since don't want it to disappear */
				scene_viewer->input_mode=SCENE_VIEWER_TRANSFORM;
				scene_viewer->temporary_transform_mode=0;
				//-- scene_viewer->user_interface=user_interface;
				scene_viewer->buffering_mode = buffering_mode;
				scene_viewer->stereo_mode = stereo_mode;
				(scene_viewer->background_colour).red=background_colour->red;
				(scene_viewer->background_colour).green=background_colour->green;
				(scene_viewer->background_colour).blue=background_colour->blue;
				(scene_viewer->background_colour).alpha=background_colour->alpha;
				/* set viewing transformation eye pos, look at point and up-vector */
				/* initially view the x,y plane */
				scene_viewer->eyex=0.0;
				scene_viewer->eyey=0.0;
				scene_viewer->eyez=2.0;
				scene_viewer->lookatx=0.0;
				scene_viewer->lookaty=0.0;
				scene_viewer->lookatz=0.0;
				scene_viewer->upx=0.0;
				scene_viewer->upy=1.0;
				scene_viewer->upz=0.0;
				/* Projection specified by viewing volume and perspective flag */
				/* viewing volume initially a unit cube */
				scene_viewer->left=-1.0;
				scene_viewer->right=1.0;
				scene_viewer->bottom=-1.0;
				scene_viewer->top=1.0;
				scene_viewer->near_plane=0.1;
				scene_viewer->far_plane=1000.0;
				scene_viewer->projection_mode=SCENE_VIEWER_PERSPECTIVE;
				scene_viewer->far_plane_fly_debt = 0.0;
				scene_viewer->near_plane_fly_debt = 0.0;
				scene_viewer->translate_rate=1.0;
				scene_viewer->tumble_rate=1.5;
				scene_viewer->zoom_rate=1.0;
				scene_viewer->antialias = 0;
				scene_viewer->perturb_lines = false;
				scene_viewer->blending_mode=CMZN_SCENEVIEWER_BLENDING_MODE_NORMAL;
				scene_viewer->depth_of_field=0.0;  /* default 0==infinite */
				scene_viewer->focal_depth=0.0;
				scene_viewer->transform_flag=0;
				scene_viewer->stereo_eye_spacing=0.25;
				scene_viewer->swap_buffers=0;
				scene_viewer->notifier_list = new cmzn_sceneviewernotifier_list();
				scene_viewer->cache = 0;
				scene_viewer->changes = CMZN_SCENEVIEWEREVENT_CHANGE_FLAG_NONE;
				if (default_light)
					ADD_OBJECT_TO_LIST(cmzn_light)(default_light, scene_viewer->list_of_lights);
				if (default_ambient_light)
					ADD_OBJECT_TO_LIST(cmzn_light)(default_ambient_light, scene_viewer->list_of_lights);
				scene_viewer->lightingLocalViewer = false;
				scene_viewer->lightingTwoSided = true;
				/* managers and callback IDs for automatic updates */
				(scene_viewer->image_texture).texture=(struct Texture *)NULL;
				(scene_viewer->image_texture).manager = NULL;
				(scene_viewer->image_texture).field  = NULL;
				(scene_viewer->image_texture).callback_id = NULL;
				(scene_viewer->image_texture).scene_viewer = scene_viewer;
				scene_viewer->order_independent_transparency_data =
					(struct cmzn_sceneviewer_transparency_order_independent_data *)NULL;

				/* set projection matrices to identity */
				for (i=0;i<16;i++)
				{
					if (0==(i % 5))
					{
						scene_viewer->window_projection_matrix[i] = 1.0;
						scene_viewer->projection_matrix[i]=1.0;
						scene_viewer->modelview_matrix[i]=1.0;
					}
					else
					{
						scene_viewer->window_projection_matrix[i] = 0.0;
						scene_viewer->projection_matrix[i]=0.0;
						scene_viewer->modelview_matrix[i]=0.0;
					}
				}
				scene_viewer->NDC_width=scene_viewer->right-scene_viewer->left;
				scene_viewer->NDC_height=scene_viewer->top-scene_viewer->bottom;
				scene_viewer->NDC_top=scene_viewer->top;
				scene_viewer->NDC_left=scene_viewer->left;
				scene_viewer->viewport_mode = CMZN_SCENEVIEWER_VIEWPORT_MODE_RELATIVE;
				scene_viewer->user_viewport_top=0.0;
				scene_viewer->user_viewport_left=0.0;
				scene_viewer->user_viewport_pixels_per_unit_x=1.0;
				scene_viewer->user_viewport_pixels_per_unit_y=1.0;
				scene_viewer->bk_texture_top=0.0;
				scene_viewer->bk_texture_left=0.0;
				scene_viewer->bk_texture_width=0.0;
				scene_viewer->bk_texture_height=0.0;
				scene_viewer->interact_mode=CMZN_SCENEVIEWER_INTERACT_MODE_STANDARD;
				scene_viewer->drag_mode=SV_DRAG_NOTHING;
				scene_viewer->previous_pointer_x = 0;
				scene_viewer->previous_pointer_y = 0;
				/* automatic tumble */
				scene_viewer->tumble_axis[0] = 1.0;
				scene_viewer->tumble_axis[1] = 0.0;
				scene_viewer->tumble_axis[2] = 0.0;
				scene_viewer->tumble_angle = 0;
				scene_viewer->tumble_active = 0;
				/* by default, use undistort stuff on textures */
				scene_viewer->bk_texture_undistort_on=1;
				scene_viewer->bk_texture_max_pixels_per_polygon=16.0;
				scene_viewer->transparency_mode=CMZN_SCENEVIEWER_TRANSPARENCY_MODE_FAST;
				scene_viewer->transparency_layers=1;
				scene_viewer->pixel_width=0;
				scene_viewer->pixel_height=0;
				scene_viewer->update_pixel_image=0;
				scene_viewer->pixel_data = (char *)NULL;
				scene_viewer->awaken=0;
				scene_viewer->module = 0;
				for (i = 0 ; i < MAX_CLIP_PLANES ; i++)
				{
					scene_viewer->clip_planes_enable[i] = 0;
					scene_viewer->clip_planes[i * 4] = 0.0;
					scene_viewer->clip_planes[i * 4 + 1] = 0.0;
					scene_viewer->clip_planes[i * 4 + 2] = 0.0;
					scene_viewer->clip_planes[i * 4 + 3] = 0.0;
				}
#if defined (WIN32_SYSTEM)
				/* Clear twice, if set then the glClear in the background will be called
					twice, which appears to work around a rendering bug on ATI windows driver 6.14.0010.6706 */
				scene_viewer->clear_twice_flag = 1;
#endif /* defined (WIN32_SYSTEM) */
				scene_viewer->frame_count = 0;

				scene_viewer->scene = 0;
				Scene_viewer_awaken(scene_viewer);
				//-- scene_viewer->graphics_buffer->buffer_awaken();//-- Graphics_buffer_awaken(scene_viewer->graphics_buffer);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"CREATE(Scene_viewer).  Not enough memory for scene_viewer");
			}
		}
		else
		{
			scene_viewer=(struct Scene_viewer *)NULL;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"CREATE(Scene_viewer).  Invalid argument(s)");
		scene_viewer=(struct Scene_viewer *)NULL;
	}
	LEAVE;

	return (scene_viewer);
} /* CREATE(Scene_viewer) */

int DESTROY(Scene_viewer)(struct Scene_viewer **scene_viewer_address)
/*******************************************************************************
LAST MODIFIED : 29 September 2000

DESCRIPTION :
Closes the scene_viewer and disposes of the scene_viewer data structure.
==============================================================================*/
{
	int return_code = 0;
	struct Scene_viewer *scene_viewer = 0;

	if (scene_viewer_address&&(scene_viewer= *scene_viewer_address))
	{
		Scene_viewer_sleep(scene_viewer);
		Scene_viewer_image_texture_set_field(&(scene_viewer->image_texture),
			NULL);
		for (cmzn_sceneviewernotifier_list::iterator iter = scene_viewer->notifier_list->begin();
			iter != scene_viewer->notifier_list->end(); ++iter)
		{
			cmzn_sceneviewernotifier *notifier = *iter;
			notifier->sceneviewerDestroyed();
			cmzn_sceneviewernotifier::deaccess(notifier);
		}
		delete scene_viewer->notifier_list;
		scene_viewer->notifier_list = 0;
		/* send the destroy callbacks */

		/* dispose of our data structure */
		DESTROY(LIST(cmzn_light))(&(scene_viewer->list_of_lights));
		if (scene_viewer->order_independent_transparency_data)
		{
			order_independent_finalise(
				&scene_viewer->order_independent_transparency_data);
		}
		/* must destroy the widget */
		DEACCESS(Graphics_buffer)(&scene_viewer->graphics_buffer);
		if (scene_viewer->pixel_data)
		{
			DEALLOCATE(scene_viewer->pixel_data);
		}
		if (scene_viewer->scene)
		{
			cmzn_scene_triggers_top_region_change_callback(scene_viewer->scene);
			cmzn_scene_destroy(&scene_viewer->scene);
		}
		if (scene_viewer->filter)
		{
			cmzn_scenefilter_destroy(&scene_viewer->filter);
		}
		DEALLOCATE(scene_viewer);
		*scene_viewer_address = 0;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(Scene_viewer).  Missing scene_viewer");
	}

	return (return_code);
} /* DESTROY(Scene_viewer) */

struct Scene_viewer *ACCESS(Scene_viewer)(struct Scene_viewer *scene_viewer)
/*******************************************************************************
LAST MODIFIED : 19 January 2007

DESCRIPTION :
==============================================================================*/
{
	//Do nothing as the scene viewer removes itself from the package list
	return(scene_viewer);
}

cmzn_sceneviewer_id cmzn_sceneviewer_access(cmzn_sceneviewer_id scene_viewer)
{
	if (scene_viewer)
		++(scene_viewer->access_count);
	return scene_viewer;
}

int DEACCESS(Scene_viewer)(struct Scene_viewer **scene_viewer_address)
{
	//Do nothing as the scene viewer removes itself from the package list
	*scene_viewer_address = (struct Scene_viewer *)NULL;
	return(1);
}

DECLARE_LIST_FUNCTIONS(Scene_viewer)

struct Scene_viewer *create_Scene_viewer_from_module(
	struct Graphics_buffer *graphics_buffer,
	struct cmzn_sceneviewermodule *sceneviewermodule)
{
	struct Scene_viewer *scene_viewer = 0;

	if (graphics_buffer && sceneviewermodule)
	{
		cmzn_scenefilter_id filter = cmzn_scenefiltermodule_get_default_scenefilter(
			sceneviewermodule->filterModule);
		scene_viewer = CREATE(Scene_viewer)(graphics_buffer,
			&sceneviewermodule->background_colour,
			sceneviewermodule->default_light,
			sceneviewermodule->default_ambient_light,
			filter);
		cmzn_scenefilter_destroy(&filter);
		if (scene_viewer)
		{
			/* Add this scene_viewer to the module list */
			ADD_OBJECT_TO_LIST(Scene_viewer)(cmzn_sceneviewer_access(scene_viewer),
				sceneviewermodule->scene_viewer_list);
		}
		scene_viewer->module = sceneviewermodule;
	}

	return (scene_viewer);
} /* create_Scene_viewer_from_module */

cmzn_sceneviewer_id cmzn_sceneviewermodule_create_sceneviewer(
	cmzn_sceneviewermodule_id sceneviewermodule,
	enum cmzn_sceneviewer_buffering_mode buffer_mode,
	enum cmzn_sceneviewer_stereo_mode stereo_mode)
{
	enum Graphics_buffer_buffering_mode graphics_buffer_buffering_mode;
	enum Graphics_buffer_stereo_mode graphics_buffer_stereo_mode;
	struct Graphics_buffer *graphics_buffer;
	struct cmzn_sceneviewer *scene_viewer = 0;

	if (sceneviewermodule)
	{
		if (CMZN_SCENEVIEWER_BUFFERING_MODE_DEFAULT==buffer_mode)
		{
			graphics_buffer_buffering_mode = GRAPHICS_BUFFER_ANY_BUFFERING_MODE;
		}
		else if (CMZN_SCENEVIEWER_BUFFERING_MODE_SINGLE==buffer_mode)
		{
			graphics_buffer_buffering_mode = GRAPHICS_BUFFER_SINGLE_BUFFERING;
		}
		else if (CMZN_SCENEVIEWER_BUFFERING_MODE_RENDER_OFFSCREEN_AND_COPY==buffer_mode)
		{
			graphics_buffer_buffering_mode = GRAPHICS_BUFFER_RENDER_OFFSCREEN_AND_COPY;
		}
		else if (CMZN_SCENEVIEWER_BUFFERING_MODE_RENDER_OFFSCREEN_AND_BLEND==buffer_mode)
		{
			graphics_buffer_buffering_mode = GRAPHICS_BUFFER_RENDER_OFFSCREEN_AND_BLEND;
		}
		else
		{
			graphics_buffer_buffering_mode = GRAPHICS_BUFFER_DOUBLE_BUFFERING;
		}
		if (CMZN_SCENEVIEWER_STEREO_MODE_DEFAULT==stereo_mode)
		{
			graphics_buffer_stereo_mode = GRAPHICS_BUFFER_ANY_STEREO_MODE;
		}
		else if (CMZN_SCENEVIEWER_STEREO_MODE_STEREO==stereo_mode)
		{
			graphics_buffer_stereo_mode = GRAPHICS_BUFFER_STEREO;
		}
		else
		{
			graphics_buffer_stereo_mode = GRAPHICS_BUFFER_MONO;
		}
		graphics_buffer = CREATE(Graphics_buffer)(
			cmzn_sceneviewermodule_get_graphics_buffer_package(sceneviewermodule),
			GRAPHICS_BUFFER_ONSCREEN_TYPE,
			graphics_buffer_buffering_mode, graphics_buffer_stereo_mode);
		scene_viewer = create_Scene_viewer_from_module(graphics_buffer,
			sceneviewermodule);
		DEACCESS(Graphics_buffer)(&graphics_buffer);
	}
	else
	{
		display_message(ERROR_MESSAGE,"cmzn_sceneviewermodule_create_sceneviewer.  "
			"The cmzn_sceneviewer data must be initialised before any scene "
			"viewers can be created.");
		scene_viewer=(struct cmzn_sceneviewer *)NULL;
	}

	return (scene_viewer);
}

/**
 * Converts mouse button-press and motion events into viewing transformations
 * in scene viewer.
 */
int Scene_viewer_input_transform(struct Scene_viewer *scene_viewer,
	struct Graphics_buffer_input *input)
{
	int width,height;
	double near_x,near_y,near_z,far_x,far_y,far_z;
	double old_near_x,old_near_y,old_near_z,old_far_x,old_far_y,old_far_z;
	double radius,fact,a[3],b[3],c[3],e[3],eye_distance,tangent_dist,phi,
		axis[3],angle;
	int return_code,pointer_x,pointer_y,i,delta_x,delta_y;

	ENTER(Scene_viewer_input_transform);
	if (scene_viewer && input)
	{
		return_code=1;
		switch (input->type)
		{
			case CMZN_SCENEVIEWERINPUT_EVENT_TYPE_BUTTON_PRESS:
			{
				pointer_x=input->position_x;
				pointer_y=input->position_y;
				/* printf("button %d press at %d %d\n",input->button,
					pointer_x,pointer_y); */
				if (Scene_viewer_unproject(pointer_x,pointer_y,
					&near_x,&near_y,&near_z,&far_x,&far_y,&far_z))
				{
					/*printf("PRESS  Near: %8.4f %8.4f %8.4f  Far: %8.4f %8.4f %8.4f\n",
						near_x,near_y,near_z,far_x,far_y,far_z);*/
					switch (input->button_number)
					{
						case 1:
						{
							if (input->modifiers & INTERACTIVE_EVENT_MODIFIER_SHIFT)
							{
								switch (scene_viewer->interact_mode)
								{
									case CMZN_SCENEVIEWER_INTERACT_MODE_STANDARD:
									{
										if (0.0 != scene_viewer->translate_rate)
										{
											scene_viewer->drag_mode=SV_DRAG_TRANSLATE;
										}
									} break;
									case CMZN_SCENEVIEWER_INTERACT_MODE_2D:
									{
										if (0.0 != scene_viewer->tumble_rate)
										{
											scene_viewer->drag_mode=SV_DRAG_TUMBLE;
										}
									} break;
									case CMZN_SCENEVIEWER_INTERACT_MODE_INVALID:
									{
										// do nothing
									} break;
								}
							}
							else
							{
								scene_viewer->tumble_angle = 0;
								scene_viewer->tumble_active = 0;
								switch (scene_viewer->interact_mode)
								{
								case CMZN_SCENEVIEWER_INTERACT_MODE_STANDARD:
								{
									if (0.0 != scene_viewer->tumble_rate)
									{
										scene_viewer->drag_mode=SV_DRAG_TUMBLE;
									}
								} break;
								case CMZN_SCENEVIEWER_INTERACT_MODE_2D:
								{
									if (0.0 != scene_viewer->translate_rate)
									{
										scene_viewer->drag_mode=SV_DRAG_TRANSLATE;
									}
								} break;
								case CMZN_SCENEVIEWER_INTERACT_MODE_INVALID:
								{
									// do nothing
								} break;
								}
							}
						} break;
						case 2:
						{
							switch (scene_viewer->interact_mode)
							{
								case CMZN_SCENEVIEWER_INTERACT_MODE_STANDARD:
								{
									if (0.0 != scene_viewer->translate_rate)
									{
										scene_viewer->drag_mode=SV_DRAG_TRANSLATE;
									}
								} break;
								case CMZN_SCENEVIEWER_INTERACT_MODE_2D:
								{
									if (0.0 != scene_viewer->tumble_rate)
									{
										scene_viewer->drag_mode=SV_DRAG_TUMBLE;
									}
								} break;
								case CMZN_SCENEVIEWER_INTERACT_MODE_INVALID:
								{
									// do nothing
								} break;
							}
						} break;
						case 3:
						{
							if (0.0 != scene_viewer->zoom_rate)
							{
								if (input->modifiers & INTERACTIVE_EVENT_MODIFIER_SHIFT)
								{
									scene_viewer->drag_mode = SV_DRAG_ZOOM;
								}
								else
								{
									scene_viewer->drag_mode=SV_DRAG_FLY;
								}
							}
						} break;
						default:
						{
						} break;
					}
					scene_viewer->previous_pointer_x=pointer_x;
					scene_viewer->previous_pointer_y=pointer_y;
				}
			} break;
			case CMZN_SCENEVIEWERINPUT_EVENT_TYPE_MOTION_NOTIFY:
			{
				pointer_x=input->position_x;
				pointer_y=input->position_y;
#if defined (DEBUG_CODE)
				printf("mouse move to %d %d\n",pointer_x,pointer_y);
#endif /* defined (DEBUG_CODE) */
				if (Scene_viewer_unproject(pointer_x,pointer_y,
					&near_x,&near_y,&near_z,&far_x,&far_y,&far_z)&&
					Scene_viewer_unproject(scene_viewer->previous_pointer_x,
						scene_viewer->previous_pointer_y, &old_near_x,&old_near_y,
						&old_near_z,&old_far_x,&old_far_y,&old_far_z))
				{
					switch (scene_viewer->drag_mode)
					{
						case SV_DRAG_NOTHING:
						{
						} break;
						case SV_DRAG_TUMBLE:
						{
							/* Tumble works like you are pulling string off a ball placed in
								the middle of the window. The line of the string is in the
								direction of dragging. If you follow back to the tangent point
								on the front of the ball, then the axis of rotation is the
								cross product of the vector from the centre of the ball to this
								tangent point with the direction you are pulling the string.
								As the distance from line you drag along to the centre of the
								window increases, the tumbling increasingly turns to twisting
								about the view direction. */
							width = Graphics_buffer_get_width(scene_viewer->graphics_buffer);
							height = Graphics_buffer_get_height(scene_viewer->graphics_buffer);
							if ((0<width)&&(0<height))
							{
								/* get the radius of the ball */
								radius=0.25*(width+height);
								delta_x=pointer_x-scene_viewer->previous_pointer_x;
								delta_y=scene_viewer->previous_pointer_y-pointer_y;
								if (0<(tangent_dist=sqrt((double)(delta_x*delta_x+delta_y*delta_y))))
								{
									/* get unit vector dx,dy normal to drag line */
									double dx = -(double)delta_y/tangent_dist;
									double dy =  (double)delta_x/tangent_dist;
									/* get shortest distance to centre along drag line normal */
									double d = dx*(pointer_x-0.5*(width-1))+dy*(0.5*(height-1)-pointer_y);
									/* limit d to radius so twists about view direction */
									if (d > radius)
									{
										d = radius;
									}
									else
									{
										if (d < -radius)
										{
											d = -radius;
										}
									}
									/* phi ranges from 0 pointing out of the screen to +/- PI/2
										 in the plane of the window */
									phi=acos(d/radius)-0.5*PI;
									/* apply the tumble_rate to slow/hasten tumble */
									angle=scene_viewer->tumble_rate*tangent_dist/radius;
									/* get axis to rotate about */
									/* a = vector towards viewer = angle to rotate about as a
										 right hand screw when phi = -PI/2 */
									a[0]=scene_viewer->eyex-scene_viewer->lookatx;
									a[1]=scene_viewer->eyey-scene_viewer->lookaty;
									a[2]=scene_viewer->eyez-scene_viewer->lookatz;
									normalize3(a);
									/* b = up vector */
									b[0]=scene_viewer->upx;
									b[1]=scene_viewer->upy;
									b[2]=scene_viewer->upz;
									normalize3(b);
									/* c = b (x) a = vector to the right */
									cross_product3(b,a,c);
									normalize3(c);
									/* e = angle to rotate about if phi = 0 */
									e[0] = dx*c[0] + dy*b[0];
									e[1] = dx*c[1] + dy*b[1];
									e[2] = dx*c[2] + dy*b[2];
									/* get actual angle to rotate by from a, e and phi */
									axis[0]=sin(phi)*a[0]+cos(phi)*e[0];
									axis[1]=sin(phi)*a[1]+cos(phi)*e[1];
									axis[2]=sin(phi)*a[2]+cos(phi)*e[2];
									if (CMZN_OK == Scene_viewer_rotate_about_lookat_point(scene_viewer, axis, -angle))
									{
										//-- if (Interactive_tool_is_Transform_tool(
										//-- 	scene_viewer->interactive_tool) &&
										//-- 	Interactive_tool_transform_get_free_spin(
										//-- 		scene_viewer->interactive_tool))
										if (1)
										{
											/* Store axis and angle so that we can make the
												 scene viewer spin if left alone. */
											scene_viewer->tumble_axis[0] = axis[0];
											scene_viewer->tumble_axis[1] = axis[1];
											scene_viewer->tumble_axis[2] = axis[2];
											scene_viewer->tumble_angle = -angle;
											scene_viewer->tumble_active = 0;
										}
										else
										{
											scene_viewer->tumble_angle = 0;
											scene_viewer->tumble_active = 0;
										}
									}
								}
							}
						} break;
						case SV_DRAG_TRANSLATE:
						{
							/* a = vector towards viewer */
							a[0]=scene_viewer->eyex-scene_viewer->lookatx;
							a[1]=scene_viewer->eyey-scene_viewer->lookaty;
							a[2]=scene_viewer->eyez-scene_viewer->lookatz;
							eye_distance=normalize3(a);
							/* translate at lookat point; proportion from near to far */
							if ((scene_viewer->far_plane > scene_viewer->near_plane)&&
								(eye_distance >= scene_viewer->near_plane)&&
								(eye_distance <= scene_viewer->far_plane))
							{
								fact = (eye_distance-scene_viewer->near_plane)/
									(scene_viewer->far_plane-scene_viewer->near_plane);
							}
							else
							{
								fact = 0.0;
							}
							/* get translation at eye distance between near and far */
							/* apply the translate_rate to slow/hasten translate */
							double offset[3] =
							{
								-scene_viewer->translate_rate*((1.0-fact)*(near_x-old_near_x) + fact*(far_x-old_far_x)),
								-scene_viewer->translate_rate*((1.0-fact)*(near_y-old_near_y) + fact*(far_y-old_far_y)),
								-scene_viewer->translate_rate*((1.0-fact)*(near_z-old_near_z) + fact*(far_z-old_far_z))
							};
							Scene_viewer_translate(scene_viewer, offset);
						} break;
						case SV_DRAG_ZOOM:
						{
							/*??? Handles only symmetric viewing volume */
							radius=0.25*(scene_viewer->right-scene_viewer->left+
								scene_viewer->top-scene_viewer->bottom);
							/* apply the zoom_rate to slow/hasten zoom */
							fact=1.0 + 0.01*scene_viewer->zoom_rate;
							i=pointer_y;
							while (i>scene_viewer->previous_pointer_y)
							{
								radius /= fact;
								i--;
							}
							while (i<scene_viewer->previous_pointer_y)
							{
								radius *= fact;
								i++;
							}
							scene_viewer->left=-radius;
							scene_viewer->right=radius;
							scene_viewer->bottom=-radius;
							scene_viewer->top=radius;
							scene_viewer->setChangedTransform();
						} break;
						case SV_DRAG_FLY:
						{
							width = Graphics_buffer_get_width(scene_viewer->graphics_buffer);
							height = Graphics_buffer_get_height(scene_viewer->graphics_buffer);
							if ((0<width)&&(0<height))
							{
								/* a = vector towards viewer */
								double angle = cmzn_sceneviewer_get_view_angle(scene_viewer);
								a[0] = scene_viewer->eyex-scene_viewer->lookatx;
								a[1] = scene_viewer->eyey-scene_viewer->lookaty;
								a[2] = scene_viewer->eyez-scene_viewer->lookatz;

								delta_y = scene_viewer->previous_pointer_y-pointer_y;
								double dist = norm3(a);
								double dy = 1.5 * delta_y/((double)height);
								if ((dist + dy*dist) > 0.01)
								{
									normalize3(a);
									scene_viewer->eyex += (a[0]*dy*dist);
									scene_viewer->eyey += (a[1]*dy*dist);
									scene_viewer->eyez += (a[2]*dy*dist);
									double near_far_minimum_ratio = 0.0001;
									if ((near_far_minimum_ratio * scene_viewer->far_plane) <
										(scene_viewer->near_plane + dy*dist + scene_viewer->near_plane_fly_debt))
									{
										if (scene_viewer->near_plane_fly_debt != 0.0)
										{
											scene_viewer->near_plane_fly_debt += dy*dist;
											if (scene_viewer->near_plane_fly_debt > 0.0)
											{
												scene_viewer->near_plane += scene_viewer->near_plane_fly_debt;
												scene_viewer->far_plane += scene_viewer->near_plane_fly_debt;
												scene_viewer->near_plane_fly_debt = 0.0;
												scene_viewer->far_plane_fly_debt = 0.0;
											}
										}
										else
										{
											scene_viewer->near_plane += dy*dist;
											scene_viewer->far_plane += dy*dist;
										}
									}
									else
									{
										if (scene_viewer->near_plane_fly_debt == 0.0)
										{
											double diff = scene_viewer->near_plane - near_far_minimum_ratio * scene_viewer->far_plane;
											scene_viewer->near_plane = near_far_minimum_ratio * scene_viewer->far_plane;
											scene_viewer->far_plane -= diff;
											scene_viewer->near_plane_fly_debt -= near_far_minimum_ratio * scene_viewer->far_plane;
										}
										scene_viewer->near_plane_fly_debt += dy*dist;
									}
									cmzn_sceneviewer_set_view_angle(scene_viewer, angle);
								}
							}
						} break;
						default:
						{
						} break;
					}
					scene_viewer->previous_pointer_x=pointer_x;
					scene_viewer->previous_pointer_y=pointer_y;
				}
			} break;
			case CMZN_SCENEVIEWERINPUT_EVENT_TYPE_BUTTON_RELEASE:
			{
				//-- if ((scene_viewer->drag_mode == SV_DRAG_TUMBLE) && scene_viewer->tumble_angle)
				//-- {
				//-- 	scene_viewer->tumble_active = 1;
				//-- }
#if defined (DEBUG_CODE)
				printf("button %d release at %d %d\n",input->button_number,
					input->position_x,input->position_y);
				pointer_x=input->position_x;
				pointer_y=input->position_y;
				if (Scene_viewer_unproject(pointer_x,pointer_y,
					&near_x,&near_y,&near_z,&far_x,&far_y,&far_z))
				{
					printf("RELEASENear: %8.4f %8.4f %8.4f  Far: %8.4f %8.4f %8.4f\n",
						near_x,near_y,near_z,far_x,far_y,far_z);
				}
#endif /* defined (DEBUG_CODE) */
				scene_viewer->drag_mode=SV_DRAG_NOTHING;
			} break;
			case CMZN_SCENEVIEWERINPUT_EVENT_TYPE_KEY_PRESS:
			{
#if defined (DEBUG_CODE)
				printf("key %d press at %d %d\n",input->key_code,input->position_x,
					input->position_y);
#endif /* defined (DEBUG_CODE) */
			} break;
			case CMZN_SCENEVIEWERINPUT_EVENT_TYPE_KEY_RELEASE:
			{
#if defined (DEBUG_CODE)
				printf("key %d release at %d %d\n",input->key_code,input->position_x,
					input->position_y);
#endif /* defined (DEBUG_CODE) */
			} break;
			default:
			{
				printf("Scene_viewer_input_transform.  Invalid X event");
				return_code=0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_input_transform.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_input_transform */

int cmzn_sceneviewer_process_sceneviewerinput(
	struct Scene_viewer *scene_viewer, struct Graphics_buffer_input *input)
{
	return Scene_viewer_input_transform(scene_viewer, input);
}

int Scene_viewer_awaken(struct Scene_viewer *scene_viewer)
/*******************************************************************************
LAST MODIFIED : 29 September 2000

DESCRIPTION :
Restores manager callbacks of previously inactive scene_viewer. Must call after
Scene_viewer_sleep to restore normal activity.
==============================================================================*/
{
	int return_code;

	if (scene_viewer)
	{
		if (scene_viewer->awaken != 1)
		{
			scene_viewer->awaken = 1;
			if (scene_viewer->scene)
			{
				cmzn_scene_add_callback(scene_viewer->scene,
					cmzn_scene_notify_scene_viewer_callback, (void *)scene_viewer);
			}
			/* register for any texture changes */
			if (scene_viewer->image_texture.manager &&
				(!scene_viewer->image_texture.callback_id))
			{
				scene_viewer->image_texture.callback_id=
					MANAGER_REGISTER(Computed_field)(Scene_viewer_image_field_change,
					(void *)&(scene_viewer->image_texture), scene_viewer->image_texture.manager);
			}
		}
		return_code=1;
	}
	else
	{
		return_code=0;
	}

	return (return_code);
} /* Scene_viewer_awaken */

int Scene_viewer_get_freespin_tumble_angle(struct Scene_viewer *scene_viewer,
	double *tumble_angle)
/*******************************************************************************
LAST MODIFIED : 9 October 2003

DESCRIPTION :
Gets the <scene_viewer> tumble angle.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_viewer_get_freespin_tumble_angle);
	if (scene_viewer && tumble_angle)
	{
		*tumble_angle = scene_viewer->tumble_angle;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_get_freespin_tumble_angle.  Missing scene_viewer");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_get_freespin_tumble_angle */

int Scene_viewer_set_freespin_tumble_angle(struct Scene_viewer *scene_viewer,
	double tumble_angle)
/*******************************************************************************
LAST MODIFIED : 17 February 2005

DESCRIPTION :
Sets the <scene_viewer> tumble angle.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_viewer_get_freespin_tumble_angle);
	if (scene_viewer)
	{
		scene_viewer->tumble_angle = tumble_angle;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_set_freespin_tumble_angle.  Missing scene_viewer");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_set_freespin_tumble_angle */

int Scene_viewer_get_freespin_tumble_axis(struct Scene_viewer *scene_viewer,
	double *tumble_axis)
/*******************************************************************************
LAST MODIFIED : 9 October 2003

DESCRIPTION :
Gets the <scene_viewer> tumble axis.  The <tumble_axis> is the vector
about which the scene is turning relative to its lookat point.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_viewer_get_freespin_tumble_axis);
	if (scene_viewer && tumble_axis)
	{
		tumble_axis[0] = scene_viewer->tumble_axis[0];
		tumble_axis[1] = scene_viewer->tumble_axis[1];
		tumble_axis[2] = scene_viewer->tumble_axis[2];
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_get_freespin_tumble_axis.  Missing scene_viewer");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_get_freespin_tumble_axis */

int Scene_viewer_stop_animations(struct Scene_viewer *scene_viewer)
/*******************************************************************************
LAST MODIFIED : 29 September 2000

DESCRIPTION :
Tells the <scene_viewer> to stop all automatic informations that it produces,
eg. automatic tumble.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_viewer_stop_animations);
	if (scene_viewer)
	{
		scene_viewer->tumble_active = 0;
		scene_viewer->tumble_angle = 0.0;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_stop_animations.  Missing scene_viewer");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_stop_animations */

int Scene_viewer_sleep(struct Scene_viewer *scene_viewer)
/*******************************************************************************
LAST MODIFIED : 29 September 2000

DESCRIPTION :
Turns off any pending automatic tumbles or redraws in idle time, and removes
any manager callbacks to minimise impact of inactive scene_viewer on rest of
program. Must call Scene_viewer_awaken to restore manager callbacks.
Must call this in DESTROY function.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_viewer_sleep);
	if (scene_viewer)
	{
		scene_viewer->awaken = 0;
		scene_viewer->tumble_active = 0;
		scene_viewer->tumble_angle = 0.0;
		if (scene_viewer->scene)
		{
			cmzn_scene_remove_callback(scene_viewer->scene,
				cmzn_scene_notify_scene_viewer_callback, (void *)scene_viewer);
		}
		if (scene_viewer->image_texture.callback_id)
		{
			MANAGER_DEREGISTER(Computed_field)(scene_viewer->image_texture.callback_id,
				scene_viewer->image_texture.manager);
			scene_viewer->image_texture.callback_id=(void *)NULL;
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"Scene_viewer_sleep.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_sleep */

int Scene_viewer_get_background_texture_info(struct Scene_viewer *scene_viewer,
	double *bk_texture_left,double *bk_texture_top,
	double *bk_texture_width,double *bk_texture_height,
	int *bk_texture_undistort_on,double *bk_texture_max_pixels_per_polygon)
/*******************************************************************************
LAST MODIFIED : 28 September 1999

DESCRIPTION :
See Scene_viewer_set_background_texture_info for meaning of return values.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_viewer_get_background_texture_info);
	if (scene_viewer&&bk_texture_left&&bk_texture_top&&
		bk_texture_width&&bk_texture_height&&
		bk_texture_undistort_on&&bk_texture_max_pixels_per_polygon)
	{
		*bk_texture_left=scene_viewer->bk_texture_left;
		*bk_texture_top=scene_viewer->bk_texture_top;
		*bk_texture_width=scene_viewer->bk_texture_width;
		*bk_texture_height=scene_viewer->bk_texture_height;
		*bk_texture_undistort_on=scene_viewer->bk_texture_undistort_on;
		*bk_texture_max_pixels_per_polygon=
			scene_viewer->bk_texture_max_pixels_per_polygon;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_get_background_texture_info.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_get_background_texture_info */

int Scene_viewer_set_background_texture_info(struct Scene_viewer *scene_viewer,
	double bk_texture_left,double bk_texture_top,
	double bk_texture_width,double bk_texture_height,
	int bk_texture_undistort_on,double bk_texture_max_pixels_per_polygon)
/*******************************************************************************
LAST MODIFIED : 28 September 1999

DESCRIPTION :
If there is a background_texture in the scene_viewer, these values specify the
top,left corner, in user coordinates, where it will be displayed, while the
next two parameters specify the size it will have in these coordinates.
If the bk_texture_undistort_on flag is set, radial distortion parameters from the background texture are un-distorted when the
texture is displayed. It does this by drawing it as a collection of polygons;
the last parameter controls the size of polygons used to do this.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_viewer_set_background_texture_info);
	if (scene_viewer&&(0.0 != bk_texture_width)&&
		(0.0 != bk_texture_height)&&
		(0.0 < bk_texture_max_pixels_per_polygon))
	{
		scene_viewer->bk_texture_left=bk_texture_left;
		scene_viewer->bk_texture_top=bk_texture_top;
		scene_viewer->bk_texture_width=bk_texture_width;
		scene_viewer->bk_texture_height=bk_texture_height;
		scene_viewer->bk_texture_undistort_on=bk_texture_undistort_on;
		scene_viewer->bk_texture_max_pixels_per_polygon=
			bk_texture_max_pixels_per_polygon;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_set_background_texture_info.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_set_background_texture_info */

enum Scene_viewer_buffering_mode Scene_viewer_get_buffering_mode(
	struct Scene_viewer *scene_viewer)
/*******************************************************************************
LAST MODIFIED : 15 October 1998

DESCRIPTION :
Returns the buffer mode - single_buffer/double_buffer - of the Scene_viewer.
==============================================================================*/
{
	enum Scene_viewer_buffering_mode buffering_mode;

	ENTER(Scene_viewer_get_buffering_mode);
	if (scene_viewer)
	{
		buffering_mode=scene_viewer->buffering_mode;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_get_buffering_mode.  Invalid argument(s)");
		/* return any valid mode */
		buffering_mode=SCENE_VIEWER_DOUBLE_BUFFER;
	}
	LEAVE;

	return (buffering_mode);
} /* Scene_viewer_get_buffering_mode */

enum Scene_viewer_input_mode Scene_viewer_get_input_mode(
	struct Scene_viewer *scene_viewer)
/*******************************************************************************
LAST MODIFIED : 6 October 1998

DESCRIPTION :
Returns the input_mode of the Scene_viewer.
==============================================================================*/
{
	enum Scene_viewer_input_mode input_mode;

	ENTER(Scene_viewer_get_input_mode);
	if (scene_viewer)
	{
		input_mode=scene_viewer->input_mode;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_get_input_mode.  Invalid argument(s)");
		input_mode=SCENE_VIEWER_NO_INPUT;
	}
	LEAVE;

	return (input_mode);
} /* Scene_viewer_get_input_mode */

int Scene_viewer_set_input_mode(struct Scene_viewer *scene_viewer,
	enum Scene_viewer_input_mode input_mode)
/*******************************************************************************
LAST MODIFIED : 6 October 2000

DESCRIPTION :
Sets the input_mode of the Scene_viewer.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_viewer_set_input_mode);
	if (scene_viewer&&((SCENE_VIEWER_NO_INPUT==input_mode)||
		(SCENE_VIEWER_NO_INPUT_OR_DRAW==input_mode)||
		(SCENE_VIEWER_UPDATE_ON_CLICK==input_mode)||
		(SCENE_VIEWER_SELECT==input_mode)||
		(SCENE_VIEWER_TRANSFORM==input_mode)))
	{
		/* clear automatic tumble since cannot make successful input while on */
		scene_viewer->tumble_active = 0;
		scene_viewer->tumble_angle = 0.0;
		scene_viewer->input_mode=input_mode;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_set_input_mode.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_set_input_mode */

int cmzn_sceneviewer_add_light(cmzn_sceneviewer_id sceneviewer,
	cmzn_light_id light)
{
	if (sceneviewer && light)
	{
		if (IS_OBJECT_IN_LIST(cmzn_light)(light, sceneviewer->list_of_lights))
			return CMZN_ERROR_ALREADY_EXISTS;
		if (!ADD_OBJECT_TO_LIST(cmzn_light)(light, sceneviewer->list_of_lights))
			return CMZN_ERROR_GENERAL;
		sceneviewer->setChangedRepaint();
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

bool cmzn_sceneviewer_has_light(cmzn_sceneviewer_id sceneviewer,
	cmzn_light_id light)
{
	if (sceneviewer && light)
	{
		return (0 != IS_OBJECT_IN_LIST(cmzn_light)(light,sceneviewer->list_of_lights));
	}
	return false;
}

int cmzn_sceneviewer_has_light_in_list(struct Scene_viewer *scene_viewer,
	struct LIST(cmzn_light) *light_list)
/*******************************************************************************
LAST MODIFIED : 30 May 2001

DESCRIPTION :
Returns true if the list_of_lights in <Scene> intersects <light_list>.
==============================================================================*/
{
	int return_code;

	ENTER(cmzn_sceneviewer_has_light_in_list);
	if (scene_viewer && light_list)
	{
		if (FIRST_OBJECT_IN_LIST_THAT(cmzn_light)(cmzn_light_is_in_list,
			(void *)light_list, scene_viewer->list_of_lights))
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
			"cmzn_sceneviewer_has_light_in_list.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_sceneviewer_has_light_in_list */

int cmzn_sceneviewer_remove_light(cmzn_sceneviewer_id sceneviewer,
	cmzn_light_id light)
{
	if (sceneviewer && light)
	{
		if (!IS_OBJECT_IN_LIST(cmzn_light)(light, sceneviewer->list_of_lights))
			return CMZN_ERROR_NOT_FOUND;
		if (!REMOVE_OBJECT_FROM_LIST(cmzn_light)(light, sceneviewer->list_of_lights))
			return CMZN_ERROR_GENERAL;
		sceneviewer->setChangedRepaint();
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

bool cmzn_sceneviewer_is_lighting_local_viewer(
	cmzn_sceneviewer_id sceneviewer)
{
	if (sceneviewer)
		return sceneviewer->isLightingLocalViewer();
	return false;
}

int cmzn_sceneviewer_set_lighting_local_viewer(
	cmzn_sceneviewer_id sceneviewer, bool value)
{
	if (sceneviewer)
	{
		sceneviewer->setLightingLocalViewer(value);
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

bool cmzn_sceneviewer_is_lighting_two_sided(
	cmzn_sceneviewer_id sceneviewer)
{
	if (sceneviewer)
		return sceneviewer->isLightingTwoSided();
	return false;
}

int cmzn_sceneviewer_set_lighting_two_sided(
	cmzn_sceneviewer_id sceneviewer, bool value)
{
	if (sceneviewer)
	{
		sceneviewer->setLightingTwoSided(value);
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_sceneviewer::getTransformationMatrix(
	enum cmzn_scenecoordinatesystem fromCoordinateSystem,
	enum cmzn_scenecoordinatesystem toCoordinateSystem,
	const gtMatrix *localToWorldTransformationMatrix,
	double *transformationMatrix16)
{
	if (!transformationMatrix16)
		return CMZN_ERROR_ARGUMENT;
	double from_projection[16], inverse_to_projection[16];
	if (fromCoordinateSystem == toCoordinateSystem)
	{
		/* identity_projection */
		for (int i = 0; i < 16; ++i)
			transformationMatrix16[i] = 0.0;
		for (int i = 0; i < 16; i += 5)
			transformationMatrix16[i] = 1.0;
		return CMZN_OK;
	}
	if (!(Scene_viewer_get_transformation_to_window(this, fromCoordinateSystem,
				localToWorldTransformationMatrix, from_projection) &&
			Scene_viewer_get_transformation_to_window(this, toCoordinateSystem,
				localToWorldTransformationMatrix, inverse_to_projection)))
		return CMZN_ERROR_GENERAL;
	double lu_d, temp;
	int i, j, lu_index[4];
	if (!LU_decompose(/*dimension*/4, inverse_to_projection,
			lu_index, &lu_d,/*singular_tolerance*/1.0e-12))
		return CMZN_ERROR_GENERAL;
	double to_projection[16];
	for (i = 0 ; i < 4 ; i++)
	{
		for (j = 0 ; j < 4 ; j++)
		{
			to_projection[i * 4 + j] = 0.0;
		}
		to_projection[i * 4 + i] = 1.0;
		LU_backsubstitute(/*dimension*/4, inverse_to_projection,
			lu_index, to_projection + i * 4);
	}
	/* transpose */
	for (i = 0 ; i < 4 ; i++)
	{
		for (j = i + 1 ; j < 4 ; j++)
		{
			temp = to_projection[i*4 + j];
			to_projection[i*4 + j] = to_projection[j*4 + i];
			to_projection[j*4 + i] = temp;
		}
	}
	multiply_matrix(4, 4, 4, to_projection, from_projection, transformationMatrix16);
#if defined (TEXTURE_PROJECTION)
	if (Scene_viewer_get_modelview_matrix(scene_viewer,modelview_matrix)&&
		Scene_viewer_get_window_projection_matrix(scene_viewer,
			window_projection_matrix))
	{
		/* Multiply these matrices */
		for (i=0;i<4;i++)
		{
			for (j=0;j<4;j++)
			{
				total_projection_matrix[i * 4 + j] = 0.0;
				for (k=0;k<4;k++)
				{
					total_projection_matrix[i * 4 + j] +=
						window_projection_matrix[i * 4 + k]	*
						modelview_matrix[k * 4 + j];
				}
			}
		}

		/* Get the viewport transformation too */
		Scene_viewer_get_viewport_info(scene_viewer,
			&viewport_left, &viewport_top,
			&viewport_pixels_per_unit_x, &viewport_pixels_per_unit_y);
		Scene_viewer_get_viewport_size(scene_viewer,
			&viewport_width, &viewport_height);
		/* Multiply total_projection by viewport matrices */
		for (i=0;i<4;i++)
		{
			for (j=0;j<4;j++)
			{
				viewport_matrix[i * 4 + j] = 0.0;
				for (k=0;k<4;k++)
				{
					if ((i == 0) && (k == 0))
					{
						viewport_matrix[i * 4 + j] +=
							0.5 * viewport_width / viewport_pixels_per_unit_x  *
							total_projection_matrix[k * 4 + j];
					}
					else if((i == 1) && (k == 1))
					{
						viewport_matrix[i * 4 + j] +=
							0.5 * viewport_height / viewport_pixels_per_unit_y *
							total_projection_matrix[k * 4 + j];
					}
					else if((i == 0) && (k == 3))
					{
						viewport_matrix[i * 4 + j] +=
							(viewport_left + 0.5 * viewport_width
								/ viewport_pixels_per_unit_x) *
								total_projection_matrix[k * 4 + j];
					}
					else if((i == 1) && (k == 3))
					{
						viewport_matrix[i * 4 + j] +=
							(viewport_top - 0.5 * viewport_height
								/ viewport_pixels_per_unit_y) *
								total_projection_matrix[k * 4 + j];
					}
					else if((i == 2) && (k == 2))
					{
						viewport_matrix[i * 4 + j] +=
							total_projection_matrix[k * 4 + j];
					}
					else if((i == 3) && (k == 3))
					{
						viewport_matrix[i * 4 + j] +=
							total_projection_matrix[k * 4 + j];
					}
					else
					{
						viewport_matrix[i * 4 + j] += 0.0;
					}
				}

			}
		}
		/* texture_projection */
		Scene_viewer_get_background_texture_info(scene_viewer,
			&bk_texture_left, &bk_texture_top, &bk_texture_width, &bk_texture_height,
			&bk_texture_undistort_on, &bk_texture_max_pixels_per_polygon);
		cmzn_field_image_id image_field=
			Scene_viewer_get_background_image_field(scene_viewer);
		texture = cmzn_field_image_get_texture(image_field);
		cmzn_field_image_destroy(&image_field);
		if (texture)
		{
			Texture_get_distortion_info(texture, &distortion_centre_x,
				&distortion_centre_y, &distortion_factor_k1);
			Texture_get_physical_size(texture, &texture_width,
				&texture_height, &texture_depth);
			if (bk_texture_undistort_on && distortion_factor_k1)
			{
				display_message(ERROR_MESSAGE,
					"gfx_apply_projection.  Distortion corrected textures are not supported yet");
				return_code=0;
				/* identity_projection */
				for (i=0;i<16;i++)
				{
					if (i % 5)
					{
						projection_matrix[i] = 0;
					}
					else
					{
						projection_matrix[i] = 1;
					}
				}
			}
			else
			{
				/* Multiply viewport_matrix by background texture */
				for (i=0;i<4;i++)
				{
					for (j=0;j<4;j++)
					{
						texture_projection_matrix[i * 4 + j] = 0.0;
						for (k=0;k<4;k++)
						{
							if ((i == 0) && (k == 0))
							{
								texture_projection_matrix[i * 4 + j] +=
									(texture_width / bk_texture_width) *
									viewport_matrix[k * 4 + j];
							}
							else if((i == 1) && (k == 1))
							{
								texture_projection_matrix[i * 4 + j] +=
									(texture_height / bk_texture_height) *
									viewport_matrix[k * 4 + j];
							}
							else if((i == 0) && (k == 3))
							{
								texture_projection_matrix[i * 4 + j] +=
									(-bk_texture_left *
										(texture_width / bk_texture_width)) *
										viewport_matrix[k * 4 + j];
							}
							else if((i == 1) && (k == 3))
							{
								texture_projection_matrix[i * 4 + j] +=
									(- bk_texture_top *
										(texture_height / bk_texture_height)
										+ texture_height) *
										viewport_matrix[k * 4 + j];
							}
							else if((i == 2) && (k == 2))
							{
								texture_projection_matrix[i * 4 + j] +=
									viewport_matrix[k * 4 + j];
							}
							else if((i == 3) && (k == 3))
							{
								texture_projection_matrix[i * 4 + j] +=
									viewport_matrix[k * 4 + j];
							}
							else
							{
								texture_projection_matrix[i * 4 + j] += 0.0;
							}
						}
					}
				}
				for (i=0;i<16;i++)
				{
					projection_matrix[i] = texture_projection_matrix[i];
				}
			}
		}
	}
#endif
	return CMZN_OK;
}

int cmzn_sceneviewer_transform_coordinates(
	cmzn_sceneviewer_id sceneviewer,
	enum cmzn_scenecoordinatesystem in_coordinate_system,
	enum cmzn_scenecoordinatesystem out_coordinate_system,
	cmzn_scene_id local_scene, const double *valuesIn3, double *valuesOut3)
{
	if (!((sceneviewer) &&(sceneviewer->scene) && (valuesIn3) && (valuesOut3)))
		return CMZN_ERROR_ARGUMENT;
	gtMatrix *localToWorldTransformation =
		cmzn_scene_get_total_transformation(local_scene ? local_scene : sceneviewer->scene, sceneviewer->scene);
	double transformationMatrix16[16];
	int result = sceneviewer->getTransformationMatrix(in_coordinate_system, out_coordinate_system,
		localToWorldTransformation, transformationMatrix16);
	if (localToWorldTransformation)
		DEALLOCATE(localToWorldTransformation);
	if (CMZN_OK != result)
		return result;
	const double h =
		transformationMatrix16[12]*valuesIn3[0] +
		transformationMatrix16[13]*valuesIn3[1] +
		transformationMatrix16[14]*valuesIn3[2] +
		transformationMatrix16[15]; // perspective division factor
	valuesOut3[0] = (
		transformationMatrix16[ 0]*valuesIn3[0] +
		transformationMatrix16[ 1]*valuesIn3[1] +
		transformationMatrix16[ 2]*valuesIn3[2] +
		transformationMatrix16[ 3])/h;
	valuesOut3[1] = (
		transformationMatrix16[ 4]*valuesIn3[0] +
		transformationMatrix16[ 5]*valuesIn3[1] +
		transformationMatrix16[ 6]*valuesIn3[2] +
		transformationMatrix16[ 7])/h;
	valuesOut3[2] = (
		transformationMatrix16[ 8]*valuesIn3[0] +
		transformationMatrix16[ 9]*valuesIn3[1] +
		transformationMatrix16[10]*valuesIn3[2] +
		transformationMatrix16[11])/h;
	return CMZN_OK;
}

int Scene_viewer_add_clip_plane(struct Scene_viewer *scene_viewer,
	double A, double B, double C, double D)
/*******************************************************************************
LAST MODIFIED : 12 December 2000

DESCRIPTION :
Sets a clip plane that defines a plane in Modelview space, (Ax+By+Cz=D).
==============================================================================*/
{
	int i, index, return_code;

	ENTER(Scene_viewer_add_clip_plane);
	if (scene_viewer)
	{
		return_code=1;
		index = -1;
		for (i = 0 ; return_code && (i < MAX_CLIP_PLANES) ; i++)
		{
			if (!scene_viewer->clip_planes_enable[i])
			{
				if (index == -1)
				{
					index = i;
				}
			}
			else
			{
				/* Check it doesn't already exist */
				if ((A == scene_viewer->clip_planes[i * 4]) &&
					(B == scene_viewer->clip_planes[i * 4 + 1])  &&
					(C == scene_viewer->clip_planes[i * 4 + 2])  &&
					(D == scene_viewer->clip_planes[i * 4 + 3]))
				{
					display_message(ERROR_MESSAGE, "Scene_viewer_add_clip_plane.  "
						"Clip plane %fx+%fy+%fz=%f already exists", A, B, C, D);
					return_code=0;
				}
			}
		}
		if ((index != -1) && return_code)
		{
			scene_viewer->clip_planes_enable[index] = 1;
			scene_viewer->clip_planes[index * 4] = A;
			scene_viewer->clip_planes[index * 4 + 1] = B;
			scene_viewer->clip_planes[index * 4 + 2] = C;
			scene_viewer->clip_planes[index * 4 + 3] = D;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_add_clip_plane.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_add_clip_plane */

int Scene_viewer_remove_clip_plane(struct Scene_viewer *scene_viewer,
	double A, double B, double C, double D)
/*******************************************************************************
LAST MODIFIED : 12 December 2000

DESCRIPTION :
Removes a clip plane that defines a plane in Modelview space, fails if the
exact plane isn't defined as a clip plane.
==============================================================================*/
{
	int i, return_code;

	ENTER(Scene_viewer_remove_clip_plane);
	if (scene_viewer)
	{
		return_code = 0;
		for (i = 0 ; i < MAX_CLIP_PLANES ; i++)
		{
			if (scene_viewer->clip_planes_enable[i])
			{
				if ((A == scene_viewer->clip_planes[i * 4]) &&
					(B == scene_viewer->clip_planes[i * 4 + 1])  &&
					(C == scene_viewer->clip_planes[i * 4 + 2])  &&
					(D == scene_viewer->clip_planes[i * 4 + 3]))
				{
					scene_viewer->clip_planes_enable[i] = 0;
					scene_viewer->clip_planes[i * 4] = 0.0;
					scene_viewer->clip_planes[i * 4 + 1] = 0.0;
					scene_viewer->clip_planes[i * 4 + 2] = 0.0;
					scene_viewer->clip_planes[i * 4 + 3] = 0.0;
					return_code = 1;
				}
			}
		}
		if (!return_code)
		{
			display_message(ERROR_MESSAGE, "Scene_viewer_remove_clip_plane.  "
				"Clip plane %fx+%fy+%fz=%f not found.", A, B, C, D);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_remove_clip_plane.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_remove_clip_plane */

enum cmzn_sceneviewer_interact_mode cmzn_sceneviewer_get_interact_mode(
	cmzn_sceneviewer_id sceneviewer)
{
	if (sceneviewer)
		return sceneviewer->interact_mode;
	return CMZN_SCENEVIEWER_INTERACT_MODE_INVALID;
}

int cmzn_sceneviewer_set_interact_mode(cmzn_sceneviewer_id sceneviewer,
	enum cmzn_sceneviewer_interact_mode interact_mode)
{
	if (sceneviewer && ((CMZN_SCENEVIEWER_INTERACT_MODE_STANDARD == interact_mode) ||
		(CMZN_SCENEVIEWER_INTERACT_MODE_2D == interact_mode)))
	{
		sceneviewer->interact_mode = interact_mode;
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_sceneviewer_set_eye_position(cmzn_sceneviewer_id scene_viewer,
	const double *eyeValuesIn3)
{
	int return_code = CMZN_ERROR_ARGUMENT;
	if (scene_viewer && eyeValuesIn3)
	{
		scene_viewer->eyex = eyeValuesIn3[0];
		scene_viewer->eyey = eyeValuesIn3[1];
		scene_viewer->eyez = eyeValuesIn3[2];
		scene_viewer->setChangedTransform();
		return_code = CMZN_OK;
	}

	return return_code;
}

int cmzn_sceneviewer_get_eye_position(cmzn_sceneviewer_id scene_viewer,
	double *eyeValuesOut3)
{
	int return_code = CMZN_ERROR_ARGUMENT;
	if (scene_viewer && eyeValuesOut3)
	{
		eyeValuesOut3[0] = scene_viewer->eyex;
		eyeValuesOut3[1] = scene_viewer->eyey;
		eyeValuesOut3[2] = scene_viewer->eyez;
		return_code = CMZN_OK;
	}
	return return_code;
}

int cmzn_sceneviewer_set_lookat_position(cmzn_sceneviewer_id scene_viewer,
	const double *lookatValuesIn3)
{
	int return_code = CMZN_ERROR_ARGUMENT;
	if (scene_viewer && lookatValuesIn3)
	{
		scene_viewer->lookatx = lookatValuesIn3[0];
		scene_viewer->lookaty = lookatValuesIn3[1];
		scene_viewer->lookatz = lookatValuesIn3[2];
		scene_viewer->setChangedTransform();
		return_code = CMZN_OK;
	}

	return return_code;
}

int cmzn_sceneviewer_get_lookat_position(cmzn_sceneviewer_id scene_viewer,
	double *lookatValuesOut3)
{
	int return_code = CMZN_ERROR_ARGUMENT;
	if (scene_viewer && lookatValuesOut3)
	{
		lookatValuesOut3[0] = scene_viewer->lookatx;
		lookatValuesOut3[1] = scene_viewer->lookaty;
		lookatValuesOut3[2] = scene_viewer->lookatz;
		return_code = CMZN_OK;
	}

	return return_code;
}

int cmzn_sceneviewer_set_up_vector(cmzn_sceneviewer_id scene_viewer,
	const double *upVectorValuesIn3)
{
	int return_code = CMZN_ERROR_ARGUMENT;
	if (scene_viewer && upVectorValuesIn3)
	{
		double upVectorUnit[] = {upVectorValuesIn3[0], upVectorValuesIn3[1], upVectorValuesIn3[2]};
		normalize3(upVectorUnit);
		scene_viewer->upx = upVectorUnit[0];
		scene_viewer->upy = upVectorUnit[1];
		scene_viewer->upz = upVectorUnit[2];
		scene_viewer->setChangedTransform();
		return_code = CMZN_OK;
	}

	return return_code;
}

int cmzn_sceneviewer_get_up_vector(cmzn_sceneviewer_id scene_viewer,
	double *upVectorValuesOut3)
{
	int return_code = CMZN_ERROR_ARGUMENT;
	if (scene_viewer && upVectorValuesOut3)
	{
		upVectorValuesOut3[0] = scene_viewer->upx;
		upVectorValuesOut3[1] = scene_viewer->upy;
		upVectorValuesOut3[2] = scene_viewer->upz;
		return_code = CMZN_OK;
	}

	return return_code;
}

int cmzn_sceneviewer_get_lookat_parameters(
	cmzn_sceneviewer_id sceneviewer, double *eyeValuesOut3,
	double *lookatValuesOut3, double *upVectorValuesOut3)
{
	if (sceneviewer && eyeValuesOut3 && lookatValuesOut3 && upVectorValuesOut3)
	{
		eyeValuesOut3[0] = sceneviewer->eyex;
		eyeValuesOut3[1] = sceneviewer->eyey;
		eyeValuesOut3[2] = sceneviewer->eyez;
		lookatValuesOut3[0] = sceneviewer->lookatx;
		lookatValuesOut3[1] = sceneviewer->lookaty;
		lookatValuesOut3[2] = sceneviewer->lookatz;
		upVectorValuesOut3[0] = sceneviewer->upx;
		upVectorValuesOut3[1] = sceneviewer->upy;
		upVectorValuesOut3[2] = sceneviewer->upz;
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int Scene_viewer_set_lookat_parameters(struct Scene_viewer *scene_viewer,
	double eyex,double eyey,double eyez,
	double lookatx,double lookaty,double lookatz,
	double upx,double upy,double upz)
{
	int return_code;
	double upv[3],viewv[3];

	ENTER(Scene_viewer_set_lookat_parameters);
	if (scene_viewer)
	{
		upv[0]=upx;
		upv[1]=upy;
		upv[2]=upz;
		viewv[0]=lookatx-eyex;
		viewv[1]=lookaty-eyey;
		viewv[2]=lookatz-eyez;
		if ((0.0<normalize3(upv))&&(0.0<normalize3(viewv))&&
			(fabs(dot_product3(upv,viewv))<0.999))
		{
			scene_viewer->eyex=eyex;
			scene_viewer->eyey=eyey;
			scene_viewer->eyez=eyez;
			scene_viewer->lookatx=lookatx;
			scene_viewer->lookaty=lookaty;
			scene_viewer->lookatz=lookatz;
			/* set only unit up-vector */
			scene_viewer->upx=upv[0];
			scene_viewer->upy=upv[1];
			scene_viewer->upz=upv[2];
			scene_viewer->setChangedTransform();
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Scene_viewer_set_lookat_parameters.  "
				"Up and view directions zero or colinear");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_set_lookat_parameters.  Missing scene_viewer");
		return_code=0;
	}
	LEAVE;

	return (return_code);
}

int cmzn_sceneviewer_set_lookat_parameters_non_skew(
	cmzn_sceneviewer_id sceneviewer, const double *eyeValuesIn3,
	const double *lookatValuesIn3, const double *upVectorValuesIn3)
{
	double tempv[3],upv[3],viewv[3];
	if (sceneviewer)
	{
		for (int i = 0; i < 3; ++i)
		{
			upv[i] = upVectorValuesIn3[i];
			viewv[i] = lookatValuesIn3[i] - eyeValuesIn3[i];
		}
		if ((0.0<normalize3(upv))&&(0.0<normalize3(viewv))&&
			(fabs(dot_product3(upv,viewv))<0.999))
		{
			sceneviewer->eyex = eyeValuesIn3[0];
			sceneviewer->eyey = eyeValuesIn3[1];
			sceneviewer->eyez = eyeValuesIn3[2];
			sceneviewer->lookatx = lookatValuesIn3[0];
			sceneviewer->lookaty = lookatValuesIn3[1];
			sceneviewer->lookatz = lookatValuesIn3[2];
			/* set only unit up-vector */
			/* make sure up vector is orthogonal to view direction */
			cross_product3(upv,viewv,tempv);
			cross_product3(viewv,tempv,upv);
			normalize3(upv);
			sceneviewer->upx=upv[0];
			sceneviewer->upy=upv[1];
			sceneviewer->upz=upv[2];
			sceneviewer->setChangedTransform();
			return CMZN_OK;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"cmzn_sceneviewer_set_lookat_parameters_non_skew.  "
				"Up and view directions zero or colinear");
		}
	}
	return CMZN_ERROR_ARGUMENT;
}

double Scene_viewer_get_stereo_eye_spacing(struct Scene_viewer *scene_viewer)
/*******************************************************************************
LAST MODIFIED : 13 August 2002

DESCRIPTION :
Returns the Scene_viewer stereo_eye_spacing.
==============================================================================*/
{
	double return_spacing;

	ENTER(Scene_viewer_get_stereo_eye_spacing);
	if (scene_viewer)
	{
		return_spacing=scene_viewer->stereo_eye_spacing;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_get_stereo_eye_spacing.  Invalid argument(s)");
		return_spacing=0;
	}
	LEAVE;

	return (return_spacing);
} /* Scene_viewer_get_stereo_eye_spacing */

int Scene_viewer_set_stereo_eye_spacing(struct Scene_viewer *scene_viewer,
	double stereo_eye_spacing)
/*******************************************************************************
LAST MODIFIED : 13 August 2002

DESCRIPTION :
Sets the Scene_viewer stereo_eye_spacing.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_viewer_set_stereo_eye_spacing);
	if (scene_viewer && stereo_eye_spacing)
	{
		scene_viewer->stereo_eye_spacing = stereo_eye_spacing;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_set_stereo_eye_spacing.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_set_stereo_eye_spacing */

int Scene_viewer_get_modelview_matrix(struct Scene_viewer *scene_viewer,
	double modelview_matrix[16])
/*******************************************************************************
LAST MODIFIED : 21 January 1998

DESCRIPTION :
Reads the modelview matrix in effect.  For custom projections this is set by
Scene_viewer_set_modelview_matrix otherwise it is updated each time the window
is rendered.
The format of the matrix is as in Scene_viewer_set_modelview_matrix.
==============================================================================*/
{
	int return_code,i,j;

	ENTER(Scene_viewer_get_modelview_matrix);
	if (scene_viewer&&modelview_matrix)
	{
		for (i=0;i<4;i++)
		{
			for (j=0;j<4;j++)
			{
				modelview_matrix[i*4+j] =
					scene_viewer->modelview_matrix[j*4+i];
			}
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_get_modelview_matrix.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_get_modelview_matrix */

int Scene_viewer_set_modelview_matrix(struct Scene_viewer *scene_viewer,
	double modelview_matrix[16])
/*******************************************************************************
LAST MODIFIED : 21 January 1998

DESCRIPTION :
Sets the modelview matrix used if the SCENE_VIEWER_CUSTOM projection is in
effect. The 4X4 matrix is stored in an array of 16 double values, with values
consecutive across rows, eg:
[x'] = |  m0  m1  m2  m3 |.[x]
[y']   |  m4  m5  m6  m7 | [y]
[z']   |  m8  m9 m10 m11 | [z]
[w']   | m12 m13 m14 m15 | [w]
==============================================================================*/
{
	int return_code,i,j;

	ENTER(Scene_viewer_set_modelview_matrix);
	if (scene_viewer&&modelview_matrix)
	{
		if (SCENE_VIEWER_CUSTOM==scene_viewer->projection_mode)
		{
			for (i=0;i<4;i++)
			{
				for (j=0;j<4;j++)
				{
					scene_viewer->modelview_matrix[j*4+i]=
						modelview_matrix[i*4+j];
				}
			}
			scene_viewer->setChangedTransform();
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,"Scene_viewer_set_modelview_matrix.  "
				"Must be in CUSTOM projection mode");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_set_modelview_matrix.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_set_modelview_matrix */

int Scene_viewer_get_NDC_info(struct Scene_viewer *scene_viewer,
	double *NDC_left,double *NDC_top,double *NDC_width,double *NDC_height)
/*******************************************************************************
LAST MODIFIED : 21 January 1998

DESCRIPTION :
Returns the NDC_info from the scene_viewer - see Scene_viewer_set_NDC_info.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_viewer_get_NDC_info);
	if (scene_viewer&&NDC_left&&NDC_top&&NDC_width&&NDC_height)
	{
		*NDC_left=scene_viewer->NDC_left;
		*NDC_top=scene_viewer->NDC_top;
		*NDC_width=scene_viewer->NDC_width;
		*NDC_height=scene_viewer->NDC_height;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_get_NDC_info.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_get_NDC_info */

int Scene_viewer_set_NDC_info(struct Scene_viewer *scene_viewer,
	double NDC_left,double NDC_top,double NDC_width,double NDC_height)
/*******************************************************************************
LAST MODIFIED : 14 October 1998

DESCRIPTION :
The projection matrix converts the viewing volume into Normalised Device
Coordinates - NDCs - which range from -1.0 to +1.0 in each axis. However, the
shape of this area in the x,y plane of the screen will not be square in the
general case. The NDC_width, NDC_height, NDC_top and NDC_left values describe
the physical dimensions of the NDC cube, used to draw the image on the screen
without distortion. In RELATIVE viewport_mode, only the ratio of NDC_width to
NDC_height is important. In ABSOLUTE viewport_mode, the top and left values
are used to position the intended viewing volume in user coordinates.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_viewer_set_NDC_info);
	if (scene_viewer)
	{
		if ((0.0 != NDC_width)&&(0.0 != NDC_height))
		{
			scene_viewer->NDC_left=NDC_left;
			scene_viewer->NDC_top=NDC_top;
			scene_viewer->NDC_width=NDC_width;
			scene_viewer->NDC_height=NDC_height;
			scene_viewer->setChangedTransform();
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,"Scene_viewer_set_NDC_info.  "
				"NDC_width or NDC_height zero or negative");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_set_NDC_info.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_set_NDC_info */

int Scene_viewer_get_projection_mode(struct Scene_viewer *scene_viewer,
	enum Scene_viewer_projection_mode *projection_mode)
/*******************************************************************************
LAST MODIFIED : 17 September 2002

DESCRIPTION :
Returns the projection mode - parallel/perspective - of the Scene_viewer.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_viewer_get_projection_mode);
	if (scene_viewer)
	{
		*projection_mode=scene_viewer->projection_mode;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_get_projection_mode.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_get_projection_mode */

int Scene_viewer_set_projection_mode(struct Scene_viewer *scene_viewer,
	enum Scene_viewer_projection_mode projection_mode)
/*******************************************************************************
LAST MODIFIED : 21 January 1998

DESCRIPTION :
Sets the projection mode - parallel/perspective/custom - of the Scene_viewer.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_viewer_set_projection_mode);
	if (scene_viewer&&((SCENE_VIEWER_PARALLEL==projection_mode)||
		(SCENE_VIEWER_PERSPECTIVE==projection_mode)||
		(SCENE_VIEWER_CUSTOM==projection_mode)))
	{
		if (projection_mode != scene_viewer->projection_mode)
		{
			scene_viewer->projection_mode=projection_mode;
			scene_viewer->setChangedTransform();
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_set_projection_mode.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_set_projection_mode */

int Scene_viewer_get_projection_matrix(struct Scene_viewer *scene_viewer,
	double projection_matrix[16])
/*******************************************************************************
LAST MODIFIED : 25 January 1998

DESCRIPTION :
Reads the projection matrix in effect.  For custom projections this is set by
Scene_viewer_set_modelview_matrix otherwise it is updated each time the window
is rendered.
The format of the matrix is as in Scene_viewer_set_projection_matrix.
==============================================================================*/
{
	int return_code,i,j;

	ENTER(Scene_viewer_get_projection_matrix);
	if (scene_viewer&&projection_matrix)
	{
		for (i=0;i<4;i++)
		{
			for (j=0;j<4;j++)
			{
				projection_matrix[j*4+i]=
					(GLdouble)(scene_viewer->projection_matrix)[i*4+j];
			}
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_get_projection_matrix.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_get_projection_matrix */

int Scene_viewer_set_projection_matrix(struct Scene_viewer *scene_viewer,
	double projection_matrix[16])
/*******************************************************************************
LAST MODIFIED : 21 January 1998

DESCRIPTION :
Sets the projection matrix used if the SCENE_VIEWER_CUSTOM projection is in
effect. The 4X4 matrix is stored in an array of 16 double values, with values
consecutive across rows, eg:
[x'] = |  m0  m1  m2  m3 |.[x]
[y']   |  m4  m5  m6  m7 | [y]
[z']   |  m8  m9 m10 m11 | [z]
[w']   | m12 m13 m14 m15 | [w]
==============================================================================*/
{
	int return_code,i,j;

	ENTER(Scene_viewer_set_projection_matrix);
	if (scene_viewer&&projection_matrix)
	{
		if (SCENE_VIEWER_CUSTOM==scene_viewer->projection_mode)
		{
			for (i=0;i<4;i++)
			{
				for (j=0;j<4;j++)
				{
					scene_viewer->projection_matrix[i*4+j] = projection_matrix[j*4+i];
				}
			}
			scene_viewer->setChangedTransform();
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,"Scene_viewer_set_projection_matrix.  "
				"Must be in CUSTOM projection mode");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_set_projection_matrix.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_set_projection_matrix */

double cmzn_sceneviewer_get_translation_rate(cmzn_sceneviewer_id sceneviewer)
{
	if (sceneviewer)
		return sceneviewer->translate_rate;
	return 0.0;
}

int cmzn_sceneviewer_set_translation_rate(cmzn_sceneviewer_id sceneviewer,
	double translation_rate)
{
	if (sceneviewer)
	{
		if (translation_rate != sceneviewer->translate_rate)
		{
			sceneviewer->translate_rate = translation_rate;
			sceneviewer->setChangedRepaint(); // Attribute only?
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

double cmzn_sceneviewer_get_tumble_rate(cmzn_sceneviewer_id sceneviewer)
{
	if (sceneviewer)
		return sceneviewer->tumble_rate;
	return 0.0;
}

int cmzn_sceneviewer_set_tumble_rate(cmzn_sceneviewer_id sceneviewer,
	double tumble_rate)
{
	if (sceneviewer)
	{
		if (tumble_rate != sceneviewer->tumble_rate)
		{
			sceneviewer->tumble_rate = tumble_rate;
			sceneviewer->setChangedRepaint(); // Attribute only?
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

double cmzn_sceneviewer_get_zoom_rate(cmzn_sceneviewer_id sceneviewer)
{
	if (sceneviewer)
		return sceneviewer->zoom_rate;
	return 0.0;
}

int cmzn_sceneviewer_set_zoom_rate(cmzn_sceneviewer_id sceneviewer,
	double zoom_rate)
{
	if (sceneviewer)
	{
		if (zoom_rate != sceneviewer->zoom_rate)
		{
			sceneviewer->zoom_rate = zoom_rate;
			sceneviewer->setChangedRepaint(); // Attribute only?
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

enum cmzn_sceneviewer_transparency_mode cmzn_sceneviewer_get_transparency_mode(
	cmzn_sceneviewer_id scene_viewer)
{
	if (scene_viewer)
	{
		return scene_viewer->transparency_mode;
	}
	return CMZN_SCENEVIEWER_TRANSPARENCY_MODE_INVALID;
}

int cmzn_sceneviewer_set_transparency_mode(cmzn_sceneviewer_id scene_viewer,
	enum cmzn_sceneviewer_transparency_mode transparency_mode)
{
	int return_code = 0;

	if (scene_viewer&&((CMZN_SCENEVIEWER_TRANSPARENCY_MODE_FAST==transparency_mode)||
		(CMZN_SCENEVIEWER_TRANSPARENCY_MODE_SLOW==transparency_mode)||
		(CMZN_SCENEVIEWER_TRANSPARENCY_MODE_ORDER_INDEPENDENT==transparency_mode)))
	{
		return_code=1;
		if (CMZN_SCENEVIEWER_TRANSPARENCY_MODE_ORDER_INDEPENDENT==transparency_mode)
		{
			if (!order_independent_capable())
			{
				/* If we can't do it don't change */
				return_code=0;
			}
		}
		if (return_code)
		{
			if (scene_viewer->transparency_mode!=transparency_mode)
			{
				scene_viewer->transparency_mode=transparency_mode;
				scene_viewer->setChangedRepaint();
			}
		}
	}

	return (return_code);
}

int cmzn_sceneviewer_get_transparency_layers(cmzn_sceneviewer_id scene_viewer)
{
	if (scene_viewer)
	{
		return scene_viewer->transparency_layers;
	}

	return CMZN_ERROR_ARGUMENT;
}

int cmzn_sceneviewer_set_transparency_layers(cmzn_sceneviewer_id scene_viewer,
	int transparency_layers)
{
	if (scene_viewer)
	{
		if (scene_viewer->transparency_layers != transparency_layers)
		{
			scene_viewer->transparency_layers = transparency_layers;
			if (scene_viewer->transparency_mode==CMZN_SCENEVIEWER_TRANSPARENCY_MODE_ORDER_INDEPENDENT)
			{
				scene_viewer->setChangedRepaint();
			}
		}
		return CMZN_OK;
	}

	return CMZN_ERROR_ARGUMENT;
}

double cmzn_sceneviewer_get_view_angle(cmzn_sceneviewer_id sceneviewer)
{
	if (sceneviewer)
	{
		const double diagonal = sqrt((sceneviewer->right - sceneviewer->left)*
			(sceneviewer->right - sceneviewer->left) +
			(sceneviewer->top - sceneviewer->bottom)*
			(sceneviewer->top - sceneviewer->bottom));
		const double view[3] = {
			sceneviewer->eyex - sceneviewer->lookatx,
			sceneviewer->eyey - sceneviewer->lookaty,
			sceneviewer->eyez - sceneviewer->lookatz
		};
		const double eye_distance = norm3(view);
		const double view_angle = 2.0*atan(diagonal/(2.0*eye_distance));
		return view_angle;
	}
	return 0;
}

int cmzn_sceneviewer_set_view_angle(cmzn_sceneviewer_id sceneviewer,
	double view_angle)
{
	if (sceneviewer && (0 < view_angle) && (view_angle < PI) && (
		(SCENE_VIEWER_PARALLEL == sceneviewer->projection_mode) ||
		(SCENE_VIEWER_PERSPECTIVE == sceneviewer->projection_mode)))
	{
		const double width = fabs(sceneviewer->right-sceneviewer->left);
		const double height = fabs(sceneviewer->top-sceneviewer->bottom);
		const double centre_x = (sceneviewer->right+sceneviewer->left)/2.0;
		const double centre_y = (sceneviewer->top+sceneviewer->bottom)/2.0;
		const double diagonal = sqrt(width*width+height*height);
		double view[3] = {
			sceneviewer->eyex - sceneviewer->lookatx,
			sceneviewer->eyey - sceneviewer->lookaty,
			sceneviewer->eyez - sceneviewer->lookatz
		};
		double eye_distance = normalize3(view);
		const double size_ratio = tan(view_angle/2.0)*eye_distance/diagonal;
		sceneviewer->left   = centre_x - width *size_ratio;
		sceneviewer->right  = centre_x + width *size_ratio;
		sceneviewer->bottom = centre_y - height*size_ratio;
		sceneviewer->top    = centre_y + height*size_ratio;
		sceneviewer->setChangedTransform();
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int Scene_viewer_get_horizontal_view_angle(struct Scene_viewer *scene_viewer,
	double *horizontal_view_angle)
/*******************************************************************************
LAST MODIFIED : 6 April 2001

DESCRIPTION :
Gets the horizontal view angle, in radians, of the <scene_viewer>.
View angle is measured across the normalized device coordinates - NDCs.
For PARALLEL and PERSPECTIVE projection modes only.
==============================================================================*/
{
	double eye_distance, view[3];
	int return_code;

	ENTER(Scene_viewer_get_horizontal_view_angle);
	if (scene_viewer && horizontal_view_angle && (
		(SCENE_VIEWER_PARALLEL == scene_viewer->projection_mode) ||
		(SCENE_VIEWER_PERSPECTIVE == scene_viewer->projection_mode)))
	{
		view[0] = scene_viewer->eyex - scene_viewer->lookatx;
		view[1] = scene_viewer->eyey - scene_viewer->lookaty;
		view[2] = scene_viewer->eyez - scene_viewer->lookatz;
		eye_distance = norm3(view);
		*horizontal_view_angle =
			2.0*atan((scene_viewer->right - scene_viewer->left)/(2.0*eye_distance));
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_get_horizontal_view_angle.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_get_horizontal_view_angle */

int Scene_viewer_get_vertical_view_angle(struct Scene_viewer *scene_viewer,
	double *vertical_view_angle)
/*******************************************************************************
LAST MODIFIED : 6 April 2001

DESCRIPTION :
Gets the vertical view angle, in radians, of the <scene_viewer>.
View angle is measured across the normalized device coordinates - NDCs.
For PARALLEL and PERSPECTIVE projection modes only.
==============================================================================*/
{
	double eye_distance, view[3];
	int return_code;

	ENTER(Scene_viewer_get_vertical_view_angle);
	if (scene_viewer && vertical_view_angle && (
		(SCENE_VIEWER_PARALLEL == scene_viewer->projection_mode) ||
		(SCENE_VIEWER_PERSPECTIVE == scene_viewer->projection_mode)))
	{
		view[0] = scene_viewer->eyex - scene_viewer->lookatx;
		view[1] = scene_viewer->eyey - scene_viewer->lookaty;
		view[2] = scene_viewer->eyez - scene_viewer->lookatz;
		eye_distance = norm3(view);
		*vertical_view_angle =
			2.0*atan((scene_viewer->top - scene_viewer->bottom)/(2.0*eye_distance));
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_get_vertical_view_angle.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_get_vertical_view_angle */

int Scene_viewer_set_view_simple(struct Scene_viewer *scene_viewer,
	double centre_x,double centre_y,double centre_z,double radius,
	double view_angle,double clip_distance)
{
	double eyex,eyey,eyez,fact,eye_distance;
	int return_code;

	ENTER(Scene_viewer_set_view_simple);
	if (scene_viewer&&(0.0<radius)&&(1.0 <= view_angle)&&(179.0 >= view_angle))
	{
		/* turn eyex into unit vector in direction back to eye */
		eyex = scene_viewer->eyex-scene_viewer->lookatx;
		eyey = scene_viewer->eyey-scene_viewer->lookaty;
		eyez = scene_viewer->eyez-scene_viewer->lookatz;
		fact = 1.0/sqrt(eyex*eyex+eyey*eyey+eyez*eyez);
		eyex *= fact;
		eyey *= fact;
		eyez *= fact;
		/* look at the centre of the sphere */
		scene_viewer->lookatx=centre_x;
		scene_viewer->lookaty=centre_y;
		scene_viewer->lookatz=centre_z;
		/* shift the eye position to achieve the desired view_angle */
		eye_distance=sqrt(2.0)*radius/tan(view_angle*PI/360.0);
		scene_viewer->eyex = centre_x + eyex*eye_distance;
		scene_viewer->eyey = centre_y + eyey*eye_distance;
		scene_viewer->eyez = centre_z + eyez*eye_distance;
		scene_viewer->left= -radius;
		scene_viewer->right= radius;
		scene_viewer->bottom= -radius;
		scene_viewer->top= radius;
		scene_viewer->far_plane=eye_distance+clip_distance;
		const double nearClippingFactor = 0.95;
		if (clip_distance > nearClippingFactor*eye_distance)
		{
			scene_viewer->near_plane = (1.0 - nearClippingFactor)*eye_distance;
		}
		else
		{
			scene_viewer->near_plane = eye_distance - clip_distance;
		}
		scene_viewer->far_plane_fly_debt = 0.0;
		scene_viewer->near_plane_fly_debt = 0.0;
		scene_viewer->setChangedTransform();
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_set_view_simple.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_set_view_simple */

int Scene_viewer_get_viewing_volume(struct Scene_viewer *scene_viewer,
	double *left,double *right,double *bottom,double *top,
	double *near_plane, double *far_plane)
/*******************************************************************************
LAST MODIFIED : 21 November 1997

DESCRIPTION :
Gets the viewing volume of the Scene_viewer.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_viewer_get_viewing_volume);
	if (scene_viewer&&left&&right&&bottom&&top&&near_plane&&far_plane)
	{
		*left=scene_viewer->left;
		*right=scene_viewer->right;
		*bottom=scene_viewer->bottom;
		*top=scene_viewer->top;
		*near_plane=scene_viewer->near_plane;
		*far_plane=scene_viewer->far_plane;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_get_viewing_volume.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_get_viewing_volume */

int Scene_viewer_get_viewing_volume_and_NDC_info_for_specified_size(Scene_viewer *scene_viewer,
	int target_width, int target_height, int source_width, int source_height, double *left,
	double *right, double *bottom, double *top, double *scaled_NDC_width, double *scaled_NDC_height)
{
	int return_code = 0;

	ENTER(Scene_viewer_get_viewing_volume_for_specified_size);
	if (scene_viewer && left && right && bottom && top && scaled_NDC_width && scaled_NDC_height)
	{
		*left=scene_viewer->left;
		*right=scene_viewer->right;
		*bottom=scene_viewer->bottom;
		*top=scene_viewer->top;
		*scaled_NDC_width = scene_viewer->NDC_width;
		*scaled_NDC_height = scene_viewer->NDC_height;

		double ratio = (double)1.0, centre_x, centre_y, x_size, y_size, source_ratio = (double)1.0,
			rescaled_ratio = (double)1.0;
		const double tolerance = 0.000001;

		if (source_width  > 0 && source_height > 0 && source_width != source_height)
		{
			source_ratio = (double) source_width / source_height;
		}
		if (target_width > 0 && target_height > 0 && target_width != target_height)
		{
			ratio = (double) target_width / target_height;
		}

		/* rescaled ratio is an ratio used to determine how much
			 the shorter side needs to be expanded by and ratio
			 is the ratio of how much the longer side need to be
		   expanded by*/
		if (source_ratio > 1.0 && ratio > 1.0)
		{
			if (source_ratio > ratio)
			{
				rescaled_ratio = ratio / source_ratio;
				ratio = source_ratio;
			}
			source_ratio = (double)1.0;
		}
		else if (source_ratio < 1.0 && ratio < 1.0)
		{
			if (source_ratio < ratio)
			{
				rescaled_ratio = ratio / source_ratio;
				ratio = source_ratio;
			}
			source_ratio = (double)1.0;
		}
		if (ratio > (double)1.0)
		{
			/* Case where the print out image's width is greater then height */
			x_size = (fabs(((scene_viewer->right - scene_viewer->left) * ratio) / (double)2.0)) / source_ratio;
			y_size = fabs(((scene_viewer->top - scene_viewer->bottom) / (source_ratio * rescaled_ratio)) / (double)2.0);
			*scaled_NDC_width = scene_viewer->NDC_width * ratio / source_ratio;
			*scaled_NDC_height = scene_viewer->NDC_height / (rescaled_ratio * source_ratio);
			return_code = 1;
		}
		else if (ratio < (double)1.0)
		{
			/* Case where the print out image's height is greater then width */
			x_size = fabs(((scene_viewer->right - scene_viewer->left) * source_ratio * rescaled_ratio) / (double)2.0);
			y_size = fabs((scene_viewer->top - scene_viewer->bottom) / (ratio * (double)2.0)) * source_ratio;
			*scaled_NDC_width = scene_viewer->NDC_width * rescaled_ratio *source_ratio;
			*scaled_NDC_height = scene_viewer->NDC_height / ratio * source_ratio;
			return_code = 1;
		}
		else if ((tolerance > fabs(ratio - (double)1.0))  && (tolerance < fabs(source_ratio - (double)1.0)))
		{
			/* Case where the print out image is a square but the actual viewing area is not */
			if (source_ratio < (double)1.0)
			{
				source_ratio = (double)1.0 / source_ratio;
			}
			x_size = fabs(((scene_viewer->right - scene_viewer->left) * source_ratio) / (double)2.0);
			y_size = fabs(((scene_viewer->top - scene_viewer->bottom) * source_ratio) / (double)2.0);
			*scaled_NDC_width = scene_viewer->NDC_width * source_ratio;
			*scaled_NDC_height = scene_viewer->NDC_height * source_ratio;
			return_code = 1;
		}

		/* calcuate the required left, right bottom, top of the viewing volume for
			 correct image output */
		if (return_code)
		{
			centre_x = (scene_viewer->right + scene_viewer->left) / (double)2.0;
			centre_y = (scene_viewer->top + scene_viewer->bottom) / (double)2.0;
			*left = centre_x - x_size;
			*right = centre_x + x_size;
			*bottom = centre_y - y_size;
			*top = centre_y + y_size;
		}
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_get_viewing_volume_for_specified_size.  Invalid argument(s)");
		return_code=0;
	}

	LEAVE;

	return (return_code);
}

int Scene_viewer_set_viewing_volume(struct Scene_viewer *scene_viewer,
	double left,double right,double bottom,double top,
	double near_plane,double far_plane)
/*******************************************************************************
LAST MODIFIED : 15 December 1997

DESCRIPTION :
Sets the viewing volume of the Scene_viewer. Unless the viewing volume is the
same shape as the window, taking into account the aspect, the Scene_viewer will
enlarge it to maintain the desired aspect ratio. Hence, the values specified
represent the minimum viewing volume. The left, right, bottom and top values
are at the lookat point, not on the near plane as OpenGL assumes. This gives a
similar sized viewing_volume for both parallel and perspective projections.
The viewing volume can be made unsymmetric to create special effects such as
rendering a higher resolution image in parts.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_viewer_set_viewing_volume);
	if (scene_viewer)
	{
		if ((right>left)&&(top>bottom)&&(0<near_plane)&&
		   (near_plane<far_plane))
		{
			scene_viewer->left=left;
			scene_viewer->right=right;
			scene_viewer->bottom=bottom;
			scene_viewer->top=top;
			scene_viewer->near_plane=near_plane;
			scene_viewer->near_plane_fly_debt = 0.0;
			scene_viewer->far_plane=far_plane;
			scene_viewer->far_plane_fly_debt = 0.0;
			scene_viewer->setChangedTransform();
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Scene_viewer_set_viewing_volume.  Invalid viewing volume");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_set_viewing_volume.  Missing scene_viewer");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_set_viewing_volume */

int Scene_viewer_get_viewport_info(struct Scene_viewer *scene_viewer,
	double *viewport_left,double *viewport_top,double *viewport_pixels_per_unit_x,
	double *viewport_pixels_per_unit_y)
/*******************************************************************************
LAST MODIFIED : 21 January 1998

DESCRIPTION :
See Scene_viewer_set_viewport_info for explanation of the values returned.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_viewer_get_viewport_info);
	if (scene_viewer&&viewport_left&&viewport_top&&viewport_pixels_per_unit_x&&
		viewport_pixels_per_unit_y)
	{
		*viewport_left=scene_viewer->user_viewport_left;
		*viewport_top=scene_viewer->user_viewport_top;
		*viewport_pixels_per_unit_x=scene_viewer->user_viewport_pixels_per_unit_x;
		*viewport_pixels_per_unit_y=scene_viewer->user_viewport_pixels_per_unit_y;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_get_viewport_info.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_get_viewport_info */

int Scene_viewer_set_viewport_info(struct Scene_viewer *scene_viewer,
	double viewport_left,double viewport_top,double viewport_pixels_per_unit_x,
	double viewport_pixels_per_unit_y)
/*******************************************************************************
LAST MODIFIED : 21 January 1998

DESCRIPTION :
The 4 parameters of this routine define the user coordinate system in the
physical viewport. <viewport_left> and <viewport_top> define the exact value
of the user's x and y coordinate at these edges of the viewport.
Note that these are real numbers, and that they refer to the location at the
top-left of the top-left pixel.
???RC Check the above!
The remaining two values specify the scale in pixels per 1 unit of user
coordinates in the x and y direction.
The parameters to the Scene_viewer_set_background_texture_info and
Scene_viewer_set_NDC_info routines are to be given in the user coordinate
system established here. Furthermore, by adjusting the viewport_left and
viewport_top, the image can be translated, while changing the number of
pixels per unit enables zooming to be achieved.
???RC Later: send user coordinates with mouse events.
???RC How to handle y axis pointing down?
==============================================================================*/
{
	int return_code;

	ENTER(Scene_viewer_set_viewport_info);
	if (scene_viewer&&(0.0 != viewport_pixels_per_unit_x)&&
		(0.0 != viewport_pixels_per_unit_y))
	{
		scene_viewer->user_viewport_left=viewport_left;
		scene_viewer->user_viewport_top=viewport_top;
		scene_viewer->user_viewport_pixels_per_unit_x=viewport_pixels_per_unit_x;
		scene_viewer->user_viewport_pixels_per_unit_y=viewport_pixels_per_unit_y;
		scene_viewer->setChangedTransform();
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_set_viewport_info.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_set_viewport_info */

int cmzn_sceneviewer_get_antialias_sampling(cmzn_sceneviewer_id sceneviewer)
{
	if (sceneviewer)
		return sceneviewer->antialias;
	return 0;
}

int cmzn_sceneviewer_set_antialias_sampling(cmzn_sceneviewer_id sceneviewer,
	int number_of_samples)
{
	if (sceneviewer && ((0 == number_of_samples) || (1 == number_of_samples) ||
		(2 == number_of_samples) || (4 == number_of_samples) || (8 == number_of_samples)))
	{
		if (1 == number_of_samples)
		{
			number_of_samples = 0;
		}
		if (number_of_samples != sceneviewer->antialias)
		{
			sceneviewer->antialias = number_of_samples;
			sceneviewer->setChangedRepaint();
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int Scene_viewer_get_depth_of_field(struct Scene_viewer *scene_viewer,
	double *depth_of_field, double *focal_depth)
/*******************************************************************************
LAST MODIFIED : 5 December 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(Scene_viewer_get_depth_of_field);
	if (scene_viewer && depth_of_field && focal_depth)
	{
		*depth_of_field = scene_viewer->depth_of_field;
		*focal_depth = scene_viewer->focal_depth;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_get_depth_of_field.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_get_depth_of_field */

int Scene_viewer_set_depth_of_field(struct Scene_viewer *scene_viewer,
	double depth_of_field, double focal_depth)
/*******************************************************************************
LAST MODIFIED : 5 December 2006

DESCRIPTION :
depth of field 0 == infinite.
==============================================================================*/
{
	int return_code = 1;

	ENTER(Scene_viewer_set_depth_of_field);
	if (scene_viewer)
	{
		scene_viewer->depth_of_field = depth_of_field;
		scene_viewer->focal_depth = focal_depth;
		scene_viewer->setChangedRepaint();
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_set_depth_of_field.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_set_depth_of_field */

enum cmzn_sceneviewer_blending_mode cmzn_sceneviewer_get_blending_mode(
	cmzn_sceneviewer_id sceneviewer)
{
	if (sceneviewer)
		return sceneviewer->blending_mode;
	return CMZN_SCENEVIEWER_BLENDING_MODE_INVALID;
}

int cmzn_sceneviewer_set_blending_mode(cmzn_sceneviewer_id sceneviewer,
	enum cmzn_sceneviewer_blending_mode blending_mode)
{
	if (sceneviewer &&
		/* Check that this represents a valid mode */
		(ENUMERATOR_STRING(cmzn_sceneviewer_blending_mode)(blending_mode)))
	{
		if (CMZN_SCENEVIEWER_BLENDING_MODE_TRUE_ALPHA == blending_mode)
		{
#if defined (GL_VERSION_1_4)
			if (!Graphics_library_check_extension(GL_VERSION_1_4))
			{
#endif /* defined (GL_VERSION_1_4) */
				display_message(ERROR_MESSAGE, "Scene_viewer_set_blending_mode.  "
					"Blend_true_alpha (glBlendFuncSeparate) is not available on this display.");
				return CMZN_ERROR_ARGUMENT; // Add CMZN_ERROR_NOT_SUPPORTED?
#if defined (GL_VERSION_1_4)
			}
#endif /* defined (GL_VERSION_1_4) */
		}
		sceneviewer->blending_mode = blending_mode;
		sceneviewer->setChangedRepaint();
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

bool cmzn_sceneviewer_get_perturb_lines_flag(
	cmzn_sceneviewer_id sceneviewer)
{
	if (sceneviewer)
		return sceneviewer->perturb_lines;
	return false;
}

int cmzn_sceneviewer_set_perturb_lines_flag(cmzn_sceneviewer_id sceneviewer,
	bool value)
{
	if (sceneviewer)
	{
		if (value != sceneviewer->perturb_lines)
		{
			sceneviewer->perturb_lines = value;
			sceneviewer->setChangedRepaint();
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int Scene_viewer_get_viewport_size(struct Scene_viewer *scene_viewer,
	int *width, int *height)
/*******************************************************************************
LAST MODIFIED : 2 July 2002

DESCRIPTION :
Returns the width and height of the Scene_viewers drawing area.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_viewer_get_viewport_size);
	if (scene_viewer&&width&&height)
	{
		*width = Graphics_buffer_get_width(scene_viewer->graphics_buffer);
		*height = Graphics_buffer_get_height(scene_viewer->graphics_buffer);
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_get_viewport_size.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_get_viewport_size */

int cmzn_sceneviewer_set_viewport_size(struct Scene_viewer *scene_viewer,
	int width, int height)
/*******************************************************************************
LAST MODIFIED : 21 January 1998

DESCRIPTION :
Sets the width and height of the Scene_viewers drawing area.
==============================================================================*/
{
	int return_code = 0;

	if (scene_viewer&&(0<width)&&(0<height))
	{
		Graphics_buffer_set_width(scene_viewer->graphics_buffer, width);
		Graphics_buffer_set_height(scene_viewer->graphics_buffer, height);
		// Note: do not set flag CMZN_SCENEVIEWEREVENT_CHANGE_FLAG_REPAINT_REQUIRED
		// as repaint is invariably forced by window invalidation
		scene_viewer->setChangedTransformOnly();
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_sceneviewer_set_viewport_size.  Invalid argument(s)");
	}

	return (return_code);
} /* cmzn_sceneviewer_set_viewport_size */

int Scene_viewer_get_window_projection_matrix(struct Scene_viewer *scene_viewer,
	double window_projection_matrix[16])
/*******************************************************************************
LAST MODIFIED : 7 April 2003

DESCRIPTION :
Returns the actual projection matrix applied to fill the window.
==============================================================================*/
{
	int return_code,i,j;

	ENTER(Scene_viewer_get_window_projection_matrix);
	if (scene_viewer&&window_projection_matrix)
	{
		for (i=0;i<4;i++)
		{
			for (j=0;j<4;j++)
			{
				window_projection_matrix[j*4+i]=
					(GLdouble)(scene_viewer->window_projection_matrix)[i*4+j];
			}
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_get_window_projection_matrix.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_get_window_projection_matrix */

int Scene_viewer_translate(struct Scene_viewer *scene_viewer, const double offset[3])
{
	if (!scene_viewer)
		return CMZN_ERROR_ARGUMENT;
	scene_viewer->eyex += offset[0];
	scene_viewer->eyey += offset[1];
	scene_viewer->eyez += offset[2];
	scene_viewer->lookatx += offset[0];
	scene_viewer->lookaty += offset[1];
	scene_viewer->lookatz += offset[2];
	scene_viewer->setChangedTransform();
	return CMZN_OK;
}

int Scene_viewer_rotate_about_lookat_point(struct Scene_viewer *scene_viewer,
	const double axis[3], double angle)
{
	double a[3] = { axis[0], axis[1], axis[2] };
	if (!(scene_viewer && (0 < normalize3(a))))
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_rotate_about_lookat_point.  Invalid argument(s)");
		return CMZN_ERROR_ARGUMENT;
	}
	double b[3],c[3],v[3],rel_eyex,rel_eyey,rel_eyez,rel_eyea,rel_eyeb,rel_eyec,
		upa,upb,upc,new_b[3],new_c[3],cos_angle,sin_angle;
	/* get coordinate system moving with rotation, consisting of the axis a */
	/* and two othogonal vectors b and c in the plane normal to a. */
	/* v = vector towards viewer */
	v[0]=rel_eyex=scene_viewer->eyex-scene_viewer->lookatx;
	v[1]=rel_eyey=scene_viewer->eyey-scene_viewer->lookaty;
	v[2]=rel_eyez=scene_viewer->eyez-scene_viewer->lookatz;
	normalize3(v);
	/* check v is not too closely in line with a */
	if (0.8 < fabs(v[0]*a[0]+v[1]*a[1]+v[2]*a[2]))
	{
		/* use up-vector instead */
		v[0]=scene_viewer->upx;
		v[1]=scene_viewer->upy;
		v[2]=scene_viewer->upz;
	}
	/* b = axis (x) a, a vector in plane of rotation */
	b[0]=a[1]*v[2]-a[2]*v[1];
	b[1]=a[2]*v[0]-a[0]*v[2];
	b[2]=a[0]*v[1]-a[1]*v[0];
	normalize3(b);
	/* c = b (x) axis, another unit vector in plane of rotation */
	c[0]=a[1]*b[2]-a[2]*b[1];
	c[1]=a[2]*b[0]-a[0]*b[2];
	c[2]=a[0]*b[1]-a[1]*b[0];
	/* define eye position and up vector relative to a, b and c */
	rel_eyea=a[0]*rel_eyex+a[1]*rel_eyey+a[2]*rel_eyez;
	rel_eyeb=b[0]*rel_eyex+b[1]*rel_eyey+b[2]*rel_eyez;
	rel_eyec=c[0]*rel_eyex+c[1]*rel_eyey+c[2]*rel_eyez;
	upa=a[0]*scene_viewer->upx+a[1]*scene_viewer->upy+a[2]*scene_viewer->upz;
	upb=b[0]*scene_viewer->upx+b[1]*scene_viewer->upy+b[2]*scene_viewer->upz;
	upc=c[0]*scene_viewer->upx+c[1]*scene_viewer->upy+c[2]*scene_viewer->upz;
	/* get new b and c from clockwise rotation by <angle> radians about a */
	cos_angle=cos(angle);
	sin_angle=sin(angle);
	new_b[0]=cos_angle*b[0]+sin_angle*c[0];
	new_b[1]=cos_angle*b[1]+sin_angle*c[1];
	new_b[2]=cos_angle*b[2]+sin_angle*c[2];
	new_c[0]=cos_angle*c[0]-sin_angle*b[0];
	new_c[1]=cos_angle*c[1]-sin_angle*b[1];
	new_c[2]=cos_angle*c[2]-sin_angle*b[2];
	/* get eye position and up vector back in world coordinates */
	scene_viewer->eyex=scene_viewer->lookatx+
		a[0]*rel_eyea+new_b[0]*rel_eyeb+new_c[0]*rel_eyec;
	scene_viewer->eyey=scene_viewer->lookaty+
		a[1]*rel_eyea+new_b[1]*rel_eyeb+new_c[1]*rel_eyec;
	scene_viewer->eyez=scene_viewer->lookatz+
		a[2]*rel_eyea+new_b[2]*rel_eyeb+new_c[2]*rel_eyec;
	scene_viewer->upx=a[0]*upa+new_b[0]*upb+new_c[0]*upc;
	scene_viewer->upy=a[1]*upa+new_b[1]*upb+new_c[1]*upc;
	scene_viewer->upz=a[2]*upa+new_b[2]*upb+new_c[2]*upc;
	scene_viewer->setChangedTransform();
	return CMZN_OK;
}

int for_each_cmzn_light_in_Scene_viewer(struct Scene_viewer *scene_viewer,
	LIST_ITERATOR_FUNCTION(cmzn_light) *iterator_function,void *user_data)
/*******************************************************************************
LAST MODIFIED : 18 December 1997

DESCRIPTION :
Allows clients of the <scene_viewer> to perform functions with the lights in it.
The most common task will to list the lights in the scene with show_cmzn_light.
==============================================================================*/
{
	int return_code;

	ENTER(for_each_cmzn_light_in_Scene_viewer);
	if (scene_viewer)
	{
		return_code=FOR_EACH_OBJECT_IN_LIST(cmzn_light)(iterator_function,user_data,
			scene_viewer->list_of_lights);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"for_each_cmzn_light_in_Scene_viewer.  Missing scene_viewer");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* for_each_cmzn_light_in_Scene_viewer */

int Scene_viewer_get_frame_pixels(struct Scene_viewer *scene_viewer,
	enum Texture_storage_type storage, int *width, int *height,
	int preferred_antialias, int preferred_transparency_layers,
	unsigned char **frame_data, int force_onscreen)
/*******************************************************************************
LAST MODIFIED : 18 September 2002

DESCRIPTION :
Returns the contents of the graphics window as pixels.  <width> and <height>
will be respected if the window is drawn offscreen and they are non zero,
otherwise they are set in accordance with current size of the graphics window.
If <preferred_antialias> or <preferred_transparency_layers> are non zero then they
attempt to override the default values for just this call.
If <force_onscreen> is non zero then the pixels will always be grabbed from the
graphics window on screen.
==============================================================================*/
{
	int frame_width, frame_height, number_of_components, return_code, antialias, i, j,
		panel_width, panel_height, tile_height, tile_width, tiles_across, tiles_down,
		patch_width, patch_height;
	double bottom = 0.0, fraction_across, fraction_down, left,
		NDC_left = 0.0, NDC_top = 0.0, NDC_width = 0.0, NDC_height = 0.0,
		original_NDC_left, original_NDC_top, original_NDC_width, original_NDC_height,
		original_left, original_right, original_bottom, original_top,
		original_near_plane, original_far_plane, right, top = 0.0,
		viewport_left, viewport_top = 0.0, viewport_pixels_per_x = 0.0, viewport_pixels_per_y = 0.0,
		original_viewport_left = 0.0, original_viewport_top = 0.0,
		original_viewport_pixels_per_x = 0.0, original_viewport_pixels_per_y = 0.0,
		real_left, real_right, real_bottom, real_top,
		scaled_NDC_width,scaled_NDC_height ;
#if defined (OPENGL_API) && defined (USE_MSAA)
	int multisample_framebuffer_flag = 0;
#endif

	if (scene_viewer && width && height)
	{
		// force complete build of all graphics in scene for image output: not incremental
		build_Scene(scene_viewer->scene, scene_viewer->filter);

		panel_width = Graphics_buffer_get_width(scene_viewer->graphics_buffer);
		panel_height = Graphics_buffer_get_height(scene_viewer->graphics_buffer);
		antialias = preferred_antialias;
		if (antialias == -1)
		{
			antialias = 0;
		}

		if ((*width) && (*height))
		{
			frame_width = *width;
			frame_height = *height;
		}
		else
		{
			/* Only use the window size if either dimension is zero */
			frame_width = panel_width;
			frame_height = panel_height;
			*width = frame_width;
			*height = frame_height;
		}
		if (frame_width <= panel_width)
		{
			tile_width = panel_width;
			fraction_across = 1.0;
			tiles_across = 1;
		}
		else
		{
			tile_width = panel_width;
			fraction_across = (double)frame_width / (double)tile_width;
			tiles_across = (int)ceil(fraction_across);
		}
		if (frame_height <= panel_height)
		{
			tile_height = frame_height;
			fraction_down = 1.0;
			tiles_down = 1;
		}
		else
		{
			tile_height = panel_height;
			fraction_down = (double)frame_height / (double)tile_height;
			tiles_down = (int)ceil(fraction_down);
		}
		/* If working offscreen try and allocate as large an area as possible */
		struct Graphics_buffer *graphics_buffer = CREATE(Graphics_buffer)(
			0,	GRAPHICS_BUFFER_ONSCREEN_TYPE,
			GRAPHICS_BUFFER_DOUBLE_BUFFERING, GRAPHICS_BUFFER_MONO);
		graphics_buffer->width = panel_width;
		graphics_buffer->height = panel_height;
#if defined (OPENGL_API) && (GL_EXT_framebuffer_object)
		if (Graphics_library_load_extension("GL_EXT_framebuffer_object"))
		{
			graphics_buffer->type = GRAPHICS_BUFFER_GL_EXT_FRAMEBUFFER_TYPE;
		}
		Graphics_buffer_initialise_framebuffer(graphics_buffer, tile_width, tile_height);
		Graphics_buffer_bind_framebuffer(graphics_buffer);
#endif
		if (!force_onscreen)
		{
			cmzn_sceneviewer_render_scene(scene_viewer);
			number_of_components =
				Texture_storage_type_get_number_of_components(storage);
			if (ALLOCATE(*frame_data, unsigned char,
				number_of_components * (frame_width) * (frame_height)))
			{
				return_code = 1;
#if defined (OPENGL_API)
#if defined (USE_MSAA)
				if (antialias > 1)
				{
						multisample_framebuffer_flag =
							Graphics_buffer_set_multisample_framebuffer(graphics_buffer, antialias);
				}
#endif
				if (tiles_across > 1)
				{
					glPixelStorei(GL_PACK_ROW_LENGTH, frame_width);
				}
#endif
				if ((tiles_across > 1) || (tiles_down > 1))
				{
					Scene_viewer_get_viewing_volume(scene_viewer,
						&original_left, &original_right, &original_bottom, &original_top,
						&original_near_plane, &original_far_plane);
					Scene_viewer_get_NDC_info(scene_viewer,
						&original_NDC_left, &original_NDC_top, &original_NDC_width, &original_NDC_height);
					Scene_viewer_get_viewport_info(scene_viewer,
						&original_viewport_left, &original_viewport_top,
						&original_viewport_pixels_per_x, &original_viewport_pixels_per_y);
					Scene_viewer_get_viewing_volume_and_NDC_info_for_specified_size(scene_viewer,
							frame_width, frame_height, panel_width, panel_height, &real_left,
						&real_right, &real_bottom, &real_top, &scaled_NDC_width, &scaled_NDC_height);
					NDC_width = scaled_NDC_width / fraction_across;
					NDC_height = scaled_NDC_height / fraction_down ;
					viewport_pixels_per_x = original_viewport_pixels_per_x;
					viewport_pixels_per_y = original_viewport_pixels_per_y;
				}
				for (j = 0 ; return_code && (j < tiles_down) ; j++)
				{
					if ((tiles_across > 1) || (tiles_down > 1))
					{
						bottom = real_bottom + (double)j * (real_top - real_bottom) / fraction_down;
						top = real_bottom
							+ (double)(j + 1) * (real_top - real_bottom) / fraction_down;
						NDC_top = original_NDC_top + (double)j * original_NDC_height / fraction_down;
						viewport_top = ((j + 1) * tile_height - frame_height) / viewport_pixels_per_y;
					}
					for (i = 0 ; return_code && (i < tiles_across) ; i++)
					{
						if ((tiles_across > 1) || (tiles_down > 1))
						{
							left = real_left + (double)i * (real_right - real_left) / fraction_across;
							right = real_left +
								(double)(i + 1) * (real_right - real_left) / fraction_across;
							NDC_left = original_NDC_left + (double)i *
								original_NDC_width / fraction_across;
							viewport_left = i * tile_width / viewport_pixels_per_x;

							Scene_viewer_set_viewing_volume(scene_viewer,
								left, right, bottom, top,
								original_near_plane, original_far_plane);
							Scene_viewer_set_NDC_info(scene_viewer,
								NDC_left, NDC_top, NDC_width, NDC_height);
							Scene_viewer_set_viewport_info(scene_viewer,
								viewport_left, viewport_top,
								viewport_pixels_per_x, viewport_pixels_per_y);
						}
#if defined (OPENGL_API)
#if !defined (USE_MSAA)
						Scene_viewer_render_scene_in_viewport_with_overrides(scene_viewer,
							/*left*/0, /*bottom*/0, /*right*/tile_width, /*top*/tile_height,
							antialias, preferred_transparency_layers,
							/*drawing_offscreen*/1);
#else
						Scene_viewer_render_scene_in_viewport_with_overrides(scene_viewer,
							/*left*/0, /*bottom*/0, /*right*/tile_width, /*top*/tile_height,
							/*preferred_antialias*/0, preferred_transparency_layers,
							/*drawing_offscreen*/1);
#endif
#endif
						if (i < tiles_across - 1)
						{
							patch_width = tile_width;
						}
						else
						{
							patch_width = frame_width - tile_width * (tiles_across - 1);
						}
						if (j < tiles_down - 1)
						{
							patch_height = tile_height;
						}
						else
						{
							patch_height = frame_height - tile_height * (tiles_down - 1);
						}
#if defined (OPENGL_API) && defined (USE_MSAA)
						if (multisample_framebuffer_flag)
						{
							Graphics_buffer_blit_framebuffer(graphics_buffer);
						}
#endif
						return_code=Graphics_library_read_pixels(*frame_data +
							(i * tile_width + (j * tile_height )* frame_width) * number_of_components,
							patch_width, patch_height, storage, /*front_buffer*/0);
#if defined (OPENGL_API) && defined (USE_MSAA)
						if (multisample_framebuffer_flag)
						{
							Graphics_buffer_reset_multisample_framebuffer(graphics_buffer);
						}
#endif
					}
				}
				if (tiles_across > 1)
				{
					glPixelStorei(GL_PACK_ROW_LENGTH, 0);
				}
				if ((tiles_across > 1) || (tiles_down > 1))
				{
					Scene_viewer_set_viewing_volume(scene_viewer,
						original_left, original_right, original_bottom, original_top,
						original_near_plane, original_far_plane);
					Scene_viewer_set_NDC_info(scene_viewer,
						original_NDC_left, original_NDC_top, original_NDC_width, original_NDC_height);
					Scene_viewer_set_viewport_info(scene_viewer,
						original_viewport_left, original_viewport_top,
						original_viewport_pixels_per_x, original_viewport_pixels_per_y);
				}
			}
			else
			{
				return_code=0;
			}
		}
		else
		{
			/* Always use the window size if grabbing from screen */
			*width = Graphics_buffer_get_width(scene_viewer->graphics_buffer);
			*height = Graphics_buffer_get_height(scene_viewer->graphics_buffer);
			Scene_viewer_render_scene_in_viewport_with_overrides(scene_viewer,
				/*left*/0, /*bottom*/0, /*right*/*width, /*top*/*height,
				preferred_antialias, preferred_transparency_layers,
				/*drawing_offscreen*/0);
			number_of_components =
				Texture_storage_type_get_number_of_components(storage);
			if (ALLOCATE(*frame_data, unsigned char,
				number_of_components * (*width) * (*height)))
			{
				if (!(return_code=Graphics_library_read_pixels(*frame_data, *width,
					*height, storage, /*front_buffer*/0)))
				{
					DEALLOCATE(*frame_data);
				}
			}
			else
			{
				return_code=0;
			}
		}
		DEACCESS(Graphics_buffer)(&graphics_buffer);
	}
	else
	{
		return_code=0;
	}

	return return_code;
} /* Graphics_window_get_frame_pixels */

struct Cmgui_image *Scene_viewer_get_image(struct Scene_viewer *scene_viewer,
	int force_onscreen, int preferred_width, int preferred_height,
	int preferred_antialias, int preferred_transparency_layers,
	enum Texture_storage_type storage)
/*******************************************************************************
LAST MODIFIED : 18 September 2002

DESCRIPTION :
Creates and returns a Cmgui_image from the image in <scene_viewer>, usually for
writing. The image has a single depth plane and is in RGBA format.
Up to the calling function to DESTROY the returned Cmgui_image.
If <preferred_width>, <preferred_height>, <preferred_antialias> or
<preferred_transparency_layers> are non zero then they attempt to override the
default values for just this call.
If <force_onscreen> is set then the pixels are grabbed directly from the window
display and the <preferred_width> and <preferred_height> are ignored.
Currently limited to 1 byte per component -- may want to improve for HPC.
==============================================================================*/
{
	unsigned char *frame_data;
	int bytes_per_pixel, height, number_of_bytes_per_component,
		number_of_components, width;
	struct Cmgui_image *cmgui_image;

	ENTER(Scene_viewer_get_image);
	cmgui_image = (struct Cmgui_image *)NULL;
	if (scene_viewer)
	{
		number_of_components =
			Texture_storage_type_get_number_of_components(storage);
		number_of_bytes_per_component = 1;
		bytes_per_pixel = number_of_components*number_of_bytes_per_component;
		width = preferred_width;
		height = preferred_height;
		if (Scene_viewer_get_frame_pixels(scene_viewer, storage,
			&width, &height, preferred_antialias, preferred_transparency_layers,
			&frame_data, force_onscreen))
		{
			cmgui_image = Cmgui_image_constitute(width, height,
				number_of_components, number_of_bytes_per_component,
				width*bytes_per_pixel, frame_data);
			if (!cmgui_image)
			{
				display_message(ERROR_MESSAGE,
					"Scene_viewer_get_image.  Could not constitute image");
			}
			DEALLOCATE(frame_data);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Scene_viewer_get_image.  Could not get frame pixels");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_get_image.  Missing window");
	}
	LEAVE;

	return (cmgui_image);
} /* Scene_viewer_get_image */

int Scene_viewer_set_update_pixel_image(struct Scene_viewer *scene_viewer)
/*******************************************************************************
LAST MODIFIED : 18 May 1998

DESCRIPTION :
Sets a flag so that the redraw will necessarily fully render the scene in
pixel buffer mode
==============================================================================*/
{
	int return_code = 1;

	ENTER(Scene_viewer_set_update_pixel_image);
	if (scene_viewer)
	{
		scene_viewer->buffering_mode = SCENE_VIEWER_PIXEL_BUFFER;
		scene_viewer->update_pixel_image=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_set_update_pixel_image.  Missing scene_viewer");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_set_update_pixel_image */

int Scene_viewer_get_pixel_image(struct Scene_viewer *scene_viewer,
	int *width, int *height, void **data)
/*******************************************************************************
LAST MODIFIED : 18 May 1998

DESCRIPTION :
Returns RGB data grabbed from the scene viewer when buffer type
is SCENE_VIEWER_PIXEL_BUFFER.  The data is handed directly so it should
be used immediately and not DEALLOCATED.  It is expected to be byte sized
values for each of Red Green and Blue only.
==============================================================================*/
{
	int return_code;

	ENTER(Scene_viewer_set_update_pixel_image);
	if (scene_viewer&&(SCENE_VIEWER_PIXEL_BUFFER==scene_viewer->buffering_mode))
	{
		*width=scene_viewer->pixel_width;
		*height=scene_viewer->pixel_height;
		*data=scene_viewer->pixel_data;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_get_pixel_image.  Missing scene_viewer");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_get_pixel_image */

int Scene_viewer_set_pixel_image(struct Scene_viewer *scene_viewer,int width,
	int height,void *data)
/*******************************************************************************
LAST MODIFIED : 18 May 1998

DESCRIPTION :
Sets the RGB data in a scene viewer when buffer type
is SCENE_VIEWER_PIXEL_BUFFER.  The data is copied into the internal buffer.
It is expected to be byte sized values for each of Red Green and Blue only.
==============================================================================*/
{
	int return_code;
	void *new_data;

	ENTER(Scene_viewer_set_update_pixel_image);
	if (scene_viewer&&height&&width&&data)
	{
		scene_viewer->buffering_mode = SCENE_VIEWER_PIXEL_BUFFER;
		if (REALLOCATE(new_data,scene_viewer->pixel_data,char,3*width*height))
		{
			scene_viewer->pixel_width=width;
			scene_viewer->pixel_height=height;
			memcpy(new_data,data,3*width*height);
			scene_viewer->pixel_data=new_data;
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Scene_viewer_set_pixel_image. Unable to reallocate pixel image");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_set_pixel_image.  Missing scene_viewer");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_set_pixel_image */

int cmzn_sceneviewer_view_all(struct Scene_viewer *scene_viewer)
{
	int return_code;

	if (scene_viewer && scene_viewer->scene)
	{
		double centre[3], size[3];
		scene_viewer->scene->getCoordinatesRangeCentreSize(scene_viewer->filter, centre, size);
		double radius = sqrt(size[0]*size[0] + size[1]*size[1] + size[2]*size[2]);
		if (radius > 0.0)
		{
			/* enlarge radius to keep image within edge of window */
			const double width_factor = 1.05; // should be a controllable sceneviewermodule default
			radius *= width_factor;
		}
		else
		{
			radius = 0.5*(scene_viewer->right - scene_viewer->left);
		}
		const double clip_factor = 4.0; // should be a controllable sceneviewermodule default
		return_code = Scene_viewer_set_view_simple(scene_viewer, centre[0], centre[1],
			centre[2], radius, /*view_angle*/40.0, clip_factor*radius);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_sceneviewer_view_all.  Invalid argument(s)");
		return_code=0;
	}

	return (return_code);
}

int Scene_viewer_viewport_zoom(struct Scene_viewer *scene_viewer,
	double zoom_ratio)
/*******************************************************************************
LAST MODIFIED : 16 February 1998

DESCRIPTION :
Scales of the absolute image while keeping the same centre point.
==============================================================================*/
{
	int height,width; /* X widget dimensions */
	int return_code;

	ENTER(Scene_viewer_viewport_zoom);
	if (scene_viewer&&(0.0<zoom_ratio))
	{
		scene_viewer->user_viewport_pixels_per_unit_x *= zoom_ratio;
		scene_viewer->user_viewport_pixels_per_unit_y *= zoom_ratio;
		/* adjust top,left so that zoom comes from centre of viewport */
		width = Graphics_buffer_get_width(scene_viewer->graphics_buffer);
		height = Graphics_buffer_get_height(scene_viewer->graphics_buffer);
		scene_viewer->user_viewport_left += 0.5*(zoom_ratio-1.0)*
			(width/scene_viewer->user_viewport_pixels_per_unit_x);
		scene_viewer->user_viewport_top -= 0.5*(zoom_ratio-1.0)*
			(height/scene_viewer->user_viewport_pixels_per_unit_y);
		scene_viewer->setChangedTransform();
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_viewport_zoom.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Scene_viewer_viewport_zoom */

/**
 * Returns a string label for the <blending_mode>.
 * NOTE: Calling function must not deallocate returned string.
 */
PROTOTYPE_ENUMERATOR_STRING_FUNCTION(cmzn_sceneviewer_blending_mode)
{
	switch (enumerator_value)
	{
		case CMZN_SCENEVIEWER_BLENDING_MODE_NORMAL:
		{
			return "blend_normal";
		} break;
		case CMZN_SCENEVIEWER_BLENDING_MODE_NONE:
		{
			return "blend_none";
		} break;
		case CMZN_SCENEVIEWER_BLENDING_MODE_TRUE_ALPHA:
		{
			return "blend_true_alpha";
		} break;
		case CMZN_SCENEVIEWER_BLENDING_MODE_INVALID:
		{
			// do nothing
		} break;
	}
	return 0;
}

DEFINE_DEFAULT_ENUMERATOR_FUNCTIONS(cmzn_sceneviewer_blending_mode)

const char *Scene_viewer_buffering_mode_string(
	enum Scene_viewer_buffering_mode buffering_mode)
/*******************************************************************************
LAST MODIFIED : 14 October 1998

DESCRIPTION :
Returns a string label for the <buffering_mode>.
NOTE: Calling function must not deallocate returned string.
==============================================================================*/
{
	const char *return_string;

	ENTER(Scene_viewer_buffering_mode_string);
	switch (buffering_mode)
	{
		case SCENE_VIEWER_PIXEL_BUFFER:
		{
			return_string="pixel_buffer";
		} break;
		case SCENE_VIEWER_SINGLE_BUFFER:
		{
			return_string="single_buffer";
		} break;
		case SCENE_VIEWER_DOUBLE_BUFFER:
		{
			return_string="double_buffer";
		} break;
		default:
		{
			display_message(ERROR_MESSAGE,
				"Scene_viewer_buffering_mode_string.  Unknown buffer mode");
			return_string=(const char *)NULL;
		}
	}
	LEAVE;

	return (return_string);
} /* Scene_viewer_buffering_mode_string */

const char *Scene_viewer_projection_mode_string(
	enum Scene_viewer_projection_mode projection_mode)
/*******************************************************************************
LAST MODIFIED : 14 October 1998

DESCRIPTION :
Returns a string label for the <projection_mode>.
NOTE: Calling function must not deallocate returned string.
==============================================================================*/
{
	const char *return_string;

	ENTER(Scene_viewer_projection_mode_string);
	switch (projection_mode)
	{
		case SCENE_VIEWER_PARALLEL:
		{
			return_string="parallel";
		} break;
		case SCENE_VIEWER_PERSPECTIVE:
		{
			return_string="perspective";
		} break;
		case SCENE_VIEWER_CUSTOM:
		{
			return_string="custom";
		} break;
		default:
		{
			display_message(ERROR_MESSAGE,
				"Scene_viewer_projection_mode_string.  Unknown projection mode");
			return_string=(const char *)NULL;
		}
	}
	LEAVE;

	return (return_string);
} /* Scene_viewer_projection_mode_string */

const char *cmzn_sceneviewer_transparency_mode_string(
	enum cmzn_sceneviewer_transparency_mode transparency_mode)
/*******************************************************************************
LAST MODIFIED : 26 June 2003

DESCRIPTION :
Returns a string label for the <transparency_mode>.
NOTE: Calling function must not deallocate returned string.
==============================================================================*/
{
	const char *return_string;

	switch (transparency_mode)
	{
		case CMZN_SCENEVIEWER_TRANSPARENCY_MODE_FAST:
		{
			return_string="fast_transparency";
		} break;
		case CMZN_SCENEVIEWER_TRANSPARENCY_MODE_SLOW:
		{
			return_string="slow_transparency";
		} break;
		case CMZN_SCENEVIEWER_TRANSPARENCY_MODE_ORDER_INDEPENDENT:
		{
			return_string="order_independent_transparency";
		} break;
		default:
		{
			return_string=(const char *)NULL;
		}
	}

	return (return_string);
}

const char *cmzn_sceneviewer_viewport_mode_string(
	enum cmzn_sceneviewer_viewport_mode viewport_mode)
{
	switch (viewport_mode)
	{
		case CMZN_SCENEVIEWER_VIEWPORT_MODE_RELATIVE:
		{
			return "relative_viewport";
		} break;
		case CMZN_SCENEVIEWER_VIEWPORT_MODE_DISTORTING_RELATIVE:
		{
			return "distorting_relative_viewport";
		} break;
		case CMZN_SCENEVIEWER_VIEWPORT_MODE_ABSOLUTE:
		{
			return "absolute_viewport";
		} break;
		case CMZN_SCENEVIEWER_VIEWPORT_MODE_INVALID:
		{
			display_message(ERROR_MESSAGE,
				"cmzn_sceneviewer_viewport_mode_string.  Unknown viewport mode");
		} break;
	}
	return 0;
}

struct Graphics_buffer *Scene_viewer_get_graphics_buffer(
	struct Scene_viewer *scene_viewer)
/*******************************************************************************
LAST MODIFIED : 12 May 2004

DESCRIPTION :
Gets the <graphics_buffer> used for 3D graphics in the scene_viewer.
==============================================================================*/
{
	struct Graphics_buffer *graphics_buffer;

	ENTER(Scene_viewer_get_graphics_buffer);
	if (scene_viewer)
	{
		graphics_buffer = scene_viewer->graphics_buffer;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_get_graphics_buffer.  Missing scene_viewer");
		graphics_buffer = (struct Graphics_buffer *)NULL;
	}
	LEAVE;

	return (graphics_buffer);
} /* Scene_viewer_get_graphics_buffer */

unsigned int Scene_viewer_get_frame_count(struct Scene_viewer *scene_viewer)
/*******************************************************************************
LAST MODIFIED : 24 April 2008

DESCRIPTION :
Returns a count of the number of scene viewer redraws.
==============================================================================*/
{
	unsigned int frame_count;

	ENTER(Scene_viewer_get_frame_count);
	if (scene_viewer)
	{
		frame_count = scene_viewer->frame_count;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_get_frame_count.  Invalid argument(s)");
		frame_count = 0;
	}
	LEAVE;

	return (frame_count);
} /* Scene_viewer_get_frame_count */

cmzn_field_image_id Scene_viewer_get_background_image_field(struct Scene_viewer *scene_viewer)
{
	cmzn_field_image_id image_field = NULL;

	if (scene_viewer)
	{
			image_field = scene_viewer->image_texture.field;
			if (image_field)
			{
				cmzn_field_access(cmzn_field_image_base_cast(image_field));
			}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_get_background_image_field.  Invalid argument(s)");
	}

	return image_field;
}

int Scene_viewer_set_background_image_field(struct Scene_viewer *scene_viewer,
	cmzn_field_image_id image_field)
{
	int return_code = 0;
	if (scene_viewer)
	{
		return_code = Scene_viewer_image_texture_set_field(
			&(scene_viewer->image_texture), image_field);
		scene_viewer->setChangedRepaint();
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Scene_viewer_get_background_image_field.  Invalid argument(s)");
	}

	return return_code;
}

int cmzn_sceneviewer_destroy(cmzn_sceneviewer_id *scene_viewer_id_address)
/*******************************************************************************
LAST MODIFIED : 4 September 2007

DESCRIPTION :
Closes the scene_viewer.
==============================================================================*/
{
	/* The normal destroy will call the Scene_viewer_module callback
		to remove it from the module */
	int return_code = 0;
	cmzn_sceneviewer_id sceneviewer = 0;
	if (scene_viewer_id_address &&
		(0 != (sceneviewer = (cmzn_sceneviewer_id)(*scene_viewer_id_address))))
	{
		sceneviewer->access_count--;
		if ((sceneviewer->access_count == 1) && sceneviewer->module)
		{
			REMOVE_OBJECT_FROM_LIST(Scene_viewer)(sceneviewer,
				sceneviewer->module->scene_viewer_list);
			sceneviewer->module = 0;
			sceneviewer->access_count--;
		}
		if (sceneviewer->access_count <= 0)
		{
			return_code = DESTROY(Scene_viewer)(scene_viewer_id_address);
		}
		else
		{
			return_code = 1;
		}
		*scene_viewer_id_address = 0;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_sceneviewerinput_destroy.  Invalid argument(s)");
	}

	return (return_code);
}

double cmzn_sceneviewer_get_far_clipping_plane(cmzn_sceneviewer_id sceneviewer)
{
	double left, right, bottom, top, far_plane = 0.0, near_plane;

	if (sceneviewer)
	{
		Scene_viewer_get_viewing_volume(sceneviewer,
		  &left, &right, &bottom, &top, &near_plane, &far_plane);
	}

	return far_plane;
}

double cmzn_sceneviewer_get_near_clipping_plane(cmzn_sceneviewer_id sceneviewer)
{
	double left, right, bottom, top, far_plane, near_plane = 0.0;

	if (sceneviewer)
	{
		Scene_viewer_get_viewing_volume(sceneviewer,
		  &left, &right, &bottom, &top, &near_plane, &far_plane);
	}

	return near_plane;
}

int cmzn_sceneviewer_set_far_clipping_plane(cmzn_sceneviewer_id sceneviewer,
	double far_clipping_plane)
{
	double left, right, bottom, top, near_plane, old_far;

	if (sceneviewer)
	{
		if (Scene_viewer_get_viewing_volume(sceneviewer,
			&left, &right, &bottom, &top, &near_plane, &old_far))
		{
			return Scene_viewer_set_viewing_volume(sceneviewer,
				left, right, bottom, top, near_plane, far_clipping_plane);
		}
	}

	return CMZN_ERROR_ARGUMENT;
}

int cmzn_sceneviewer_set_near_clipping_plane(cmzn_sceneviewer_id sceneviewer,
	double near_clipping_plane)
{
	double left, right, bottom, top, old_near, far_plane;

	if (sceneviewer)
	{
		if (Scene_viewer_get_viewing_volume(sceneviewer,
			&left, &right, &bottom, &top, &old_near, &far_plane))
		{
			return Scene_viewer_set_viewing_volume(sceneviewer,
				left, right, bottom, top, near_clipping_plane, far_plane);
		}
	}

	return CMZN_ERROR_ARGUMENT;
}

enum cmzn_sceneviewer_viewport_mode cmzn_sceneviewer_get_viewport_mode(
	cmzn_sceneviewer_id sceneviewer)
{
	if (sceneviewer)
		return sceneviewer->viewport_mode;
	return CMZN_SCENEVIEWER_VIEWPORT_MODE_INVALID;
}

int cmzn_sceneviewer_set_viewport_mode(cmzn_sceneviewer_id sceneviewer,
	enum cmzn_sceneviewer_viewport_mode viewport_mode)
{
	if (sceneviewer && (
		(CMZN_SCENEVIEWER_VIEWPORT_MODE_RELATIVE == viewport_mode) ||
		(CMZN_SCENEVIEWER_VIEWPORT_MODE_ABSOLUTE == viewport_mode) ||
		(CMZN_SCENEVIEWER_VIEWPORT_MODE_DISTORTING_RELATIVE == viewport_mode)))
	{
		sceneviewer->viewport_mode = viewport_mode;
		sceneviewer->setChangedTransform();
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

enum cmzn_sceneviewer_projection_mode
	cmzn_sceneviewer_get_projection_mode(cmzn_sceneviewer_id sceneviewer)
{
	if (sceneviewer)
	{
		if (sceneviewer->projection_mode == SCENE_VIEWER_PERSPECTIVE)
			return CMZN_SCENEVIEWER_PROJECTION_MODE_PERSPECTIVE;
		if (sceneviewer->projection_mode == SCENE_VIEWER_PARALLEL)
			return CMZN_SCENEVIEWER_PROJECTION_MODE_PARALLEL;
		display_message(ERROR_MESSAGE,
			"cmzn_sceneviewer_get_projection_mode.  "
			"Projection mode not supported in public interface.");
	}
	return CMZN_SCENEVIEWER_PROJECTION_MODE_INVALID;
}

int cmzn_sceneviewer_set_projection_mode(cmzn_sceneviewer_id sceneviewer,
	enum cmzn_sceneviewer_projection_mode projection_mode)
{
	if (sceneviewer)
	{
		if (CMZN_SCENEVIEWER_PROJECTION_MODE_PERSPECTIVE == projection_mode)
		{
			Scene_viewer_set_projection_mode(sceneviewer, SCENE_VIEWER_PERSPECTIVE);
			return CMZN_OK;
		}
		if (CMZN_SCENEVIEWER_PROJECTION_MODE_PARALLEL == projection_mode)
		{
			Scene_viewer_set_projection_mode(sceneviewer, SCENE_VIEWER_PARALLEL);
			return CMZN_OK;
		}
	}
	return CMZN_ERROR_ARGUMENT;
}

struct Graphics_buffer *cmzn_sceneviewer_get_graphics_buffer(cmzn_sceneviewer_id scene_viewer)
{
	return scene_viewer->graphics_buffer;
}

double cmzn_sceneviewer_get_background_colour_alpha(
	cmzn_sceneviewer_id sceneviewer)
{
	if (sceneviewer)
		return sceneviewer->getBackgroundColourAlpha();
	return 0.0;
}

int cmzn_sceneviewer_set_background_colour_alpha(
	cmzn_sceneviewer_id sceneviewer, double alpha)
{
	if (sceneviewer)
		return sceneviewer->setBackgroundColourAlpha(alpha);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_sceneviewer_set_background_colour_component_rgb(
	cmzn_sceneviewer_id sceneviewer, double red, double green, double blue)
{
	const double rgb[3] = { red, green, blue };
	if (sceneviewer)
		return sceneviewer->setBackgroundColourRGB(rgb);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_sceneviewer_set_background_colour_component_rgba(
	cmzn_sceneviewer_id sceneviewer, double red, double green, double blue, double alpha)
{
	const double rgba[4] = { red, green, blue, alpha };
	if (sceneviewer)
		return sceneviewer->setBackgroundColourRGBA(rgba);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_sceneviewer_get_background_colour_rgb(
	cmzn_sceneviewer_id sceneviewer, double *valuesOut3)
{
	if (sceneviewer)
		return sceneviewer->getBackgroundColourRGB(valuesOut3);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_sceneviewer_set_background_colour_rgb(
	cmzn_sceneviewer_id sceneviewer, const double *valuesIn3)
{
	if (sceneviewer)
		return sceneviewer->setBackgroundColourRGB(valuesIn3);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_sceneviewer_get_background_colour_rgba(
	cmzn_sceneviewer_id sceneviewer, double *valuesOut4)
{
	if (sceneviewer)
		return sceneviewer->getBackgroundColourRGBA(valuesOut4);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_sceneviewer_set_background_colour_rgba(
	cmzn_sceneviewer_id sceneviewer, const double *valuesIn4)
{
	if (sceneviewer)
		return sceneviewer->setBackgroundColourRGBA(valuesIn4);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_sceneviewer_write_image_to_file(cmzn_sceneviewer_id scene_viewer,
	const char *file_name, int force_onscreen, int preferred_width, int preferred_height,
	int preferred_antialias, int preferred_transparency_layers)
/*******************************************************************************
LAST MODIFIED : 18 September 2002

DESCRIPTION :
Writes the view in the scene_viewer to the specified filename.
==============================================================================*/
{
	enum Texture_storage_type storage;
	int return_code;
	struct Cmgui_image *cmgui_image;
	struct Cmgui_image_information *cmgui_image_information;

	ENTER(cmzn_sceneviewer_write_image_to_file);
	if (scene_viewer && file_name)
	{
		storage = TEXTURE_RGBA;
		cmgui_image = Scene_viewer_get_image(scene_viewer,
			force_onscreen, preferred_width, preferred_height, preferred_antialias,
			preferred_transparency_layers, storage);
		if (cmgui_image != 0)
		{
			cmgui_image_information = CREATE(Cmgui_image_information)();
			Cmgui_image_information_add_file_name(cmgui_image_information,
				(char *)file_name);
			Cmgui_image_write(cmgui_image, cmgui_image_information);
			DESTROY(Cmgui_image_information)(&cmgui_image_information);
			DESTROY(Cmgui_image)(&cmgui_image);
			return_code = 1;
		}
		else
		{
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"cmzn_sceneviewer_write_image_to_file.  "
			"Invalid scene_viewer or file name.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_sceneviewer_write_image_to_file */

int cmzn_sceneviewer_get_NDC_info(cmzn_sceneviewer_id scene_viewer,
	double *NDC_left,double *NDC_top,double *NDC_width,double *NDC_height)
/*******************************************************************************
LAST MODIFIED : 15 November 2005

DESCRIPTION :
Gets the NDC information.
==============================================================================*/
{
	return Scene_viewer_get_NDC_info(scene_viewer, NDC_left, NDC_top,
		NDC_width, NDC_height);
}

int cmzn_sceneviewer_set_NDC_info(cmzn_sceneviewer_id scene_viewer,
	double NDC_left,double NDC_top,double NDC_width,double NDC_height)
/*******************************************************************************
LAST MODIFIED : 15 November 2005

DESCRIPTION :
Gets the NDC information.
==============================================================================*/
{
	return Scene_viewer_set_NDC_info(scene_viewer, NDC_left, NDC_top,
		NDC_width, NDC_height);
}

int cmzn_sceneviewer_get_frame_pixels(cmzn_sceneviewer_id scene_viewer,
	enum cmzn_streaminformation_image_pixel_format storage, int *width, int *height,
	int preferred_antialias, int preferred_transparency_layers,
	unsigned char **frame_data, int force_onscreen)
/*******************************************************************************
LAST MODIFIED : 18 September 2002

DESCRIPTION :
Returns the contents of the scene viewer as pixels.  <width> and <height>
will be respected if the window is drawn offscreen and they are non zero,
otherwise they are set in accordance with current size of the scene viewer.
If <preferred_antialias> or <preferred_transparency_layers> are non zero then they
attempt to override the default values for just this call.
If <force_onscreen> is non zero then the pixels will always be grabbed from the
scene viewer on screen.
==============================================================================*/
{
	enum Texture_storage_type internal_storage_type;
	int return_code = 1;
	switch(storage)
	{
		case CMZN_STREAMINFORMATION_IMAGE_PIXEL_FORMAT_LUMINANCE:
		{
			internal_storage_type = TEXTURE_LUMINANCE;
		} break;
		case CMZN_STREAMINFORMATION_IMAGE_PIXEL_FORMAT_LUMINANCE_ALPHA:
		{
			internal_storage_type = TEXTURE_LUMINANCE_ALPHA;
		} break;
		case CMZN_STREAMINFORMATION_IMAGE_PIXEL_FORMAT_RGB:
		{
			internal_storage_type = TEXTURE_RGB;
		} break;
		case CMZN_STREAMINFORMATION_IMAGE_PIXEL_FORMAT_RGBA:
		{
			internal_storage_type = TEXTURE_RGBA;
		} break;
		case CMZN_STREAMINFORMATION_IMAGE_PIXEL_FORMAT_ABGR:
		{
			internal_storage_type = TEXTURE_ABGR;
		} break;
		case CMZN_STREAMINFORMATION_IMAGE_PIXEL_FORMAT_BGR:
		{
			internal_storage_type = TEXTURE_BGR;
		} break;
		default:
		{
			display_message(ERROR_MESSAGE,
				"cmzn_sceneviewer_get_frame_pixels.  "
				"Unknown storage mode.");
			return_code = 0;
		} break;
	}
	if (return_code)
	{
		return_code = Scene_viewer_get_frame_pixels(
			scene_viewer,
			internal_storage_type, width, height, preferred_antialias,
			preferred_transparency_layers, frame_data, force_onscreen);
	}
	return (return_code);
}

int Scene_viewer_get_transformation_to_window(struct Scene_viewer *scene_viewer,
	enum cmzn_scenecoordinatesystem coordinate_system,
	const gtMatrix *local_transformation_matrix, double *projection)
{
	int return_code = 1;
	if (scene_viewer)
	{
		double viewport_width = Graphics_buffer_get_width(scene_viewer->graphics_buffer);
		double viewport_height = Graphics_buffer_get_height(scene_viewer->graphics_buffer);
		switch (coordinate_system)
		{
			case CMZN_SCENECOORDINATESYSTEM_LOCAL:
			case CMZN_SCENECOORDINATESYSTEM_WORLD:
			{
				double sum;
				int i, j, k;
				for (i = 0; i < 4; i++)
				{
					for (j = 0; j < 4; j++)
					{
						sum = 0.0;
						for (k = 0; k < 4; k++)
						{
							sum += scene_viewer->window_projection_matrix[k*4 + i]*scene_viewer->modelview_matrix[j*4 + k];
						}
						projection[i*4 + j] = sum;
					}
				}
				// convert from left-handed NDC to right-handed normalised window coordinates
				for (i = 8; i < 12; i++)
				{
					projection[i] = -projection[i];
				}
				if (coordinate_system == CMZN_SCENECOORDINATESYSTEM_LOCAL)
				{
					double sum;
					// apply local transformation if there is one
					if (local_transformation_matrix)
					{
						double world_to_ndc_projection[16];
						memcpy(world_to_ndc_projection, projection, 16*sizeof(double));
						for (i = 0 ; i < 4 ; i++)
						{
							for (j = 0 ; j < 4 ; j++)
							{
								sum = 0.0;
								for (k = 0; k < 4; k++)
								{
									sum += world_to_ndc_projection[i*4 + k] * (*(local_transformation_matrix))[j][k];
								}
								projection[i*4 + j] = sum;
							}
						}
					}
				}
				break;
			}
			default:
			{
				static double identity[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
				memcpy(projection, identity, 16*sizeof(double));
				if (coordinate_system != CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FILL)
				{
					double left, right, bottom, top;
					if (cmzn_scenecoordinatesystem_get_viewport(
						coordinate_system, viewport_width, viewport_height,
						&left, &right, &bottom, &top))
					{
						double scale_x = 2.0 / (right - left);
						double scale_y = 2.0 / (top - bottom);
						projection[0] = scale_x;
						projection[3] = -0.5*(left + right)*scale_x;
						projection[5] = scale_y;
						projection[7] = -0.5*(bottom + top)*scale_y;
					}
					else
					{
						return_code = 0;
					}
				}
				break;
			}
		}
	}
	else
	{
		return_code = 0;
	}

	return return_code;
}

/// new APIs
cmzn_scenefilter_id cmzn_sceneviewer_get_scenefilter(cmzn_sceneviewer_id scene_viewer)
{
	if (scene_viewer && scene_viewer->filter)
		return cmzn_scenefilter_access(scene_viewer->filter);
	return 0;
}

int cmzn_sceneviewer_set_scenefilter(cmzn_sceneviewer_id scene_viewer,
	cmzn_scenefilter_id filter)
{
	if (scene_viewer)
	{
		if (filter != scene_viewer->filter)
		{
			REACCESS(cmzn_scenefilter)(&scene_viewer->filter, filter);
			if (scene_viewer->scene)
				scene_viewer->scene->setChanged();
		}
		return CMZN_OK;
	}
	else
	{
		return CMZN_ERROR_ARGUMENT;
	}
}

int cmzn_sceneviewer_scenefilter_change(struct Scene_viewer *scene_viewer,	void *message_void)
{
	int return_code = 1;
	struct MANAGER_MESSAGE(cmzn_scenefilter) *message =
		(struct MANAGER_MESSAGE(cmzn_scenefilter) *)message_void;
	if (scene_viewer && message)
	{
		int change_flags = MANAGER_MESSAGE_GET_OBJECT_CHANGE(cmzn_scenefilter)(
			message, scene_viewer->filter);
		if (change_flags & MANAGER_CHANGE_RESULT(cmzn_scenefilter))
		{
			/* calling scene changed as changing filter may require new graphics to be rebuild*/
			if (scene_viewer->scene)
				scene_viewer->scene->setChanged();
		}
	}
	else
	{
		return_code = 0;
	}
	return return_code;
}

int cmzn_sceneviewer_light_change(struct Scene_viewer *scene_viewer,	void *message_void)
{
	int return_code = 1;
	struct MANAGER_MESSAGE(cmzn_light) *message = (struct MANAGER_MESSAGE(cmzn_light) *)message_void;
	if (scene_viewer && message)
	{
		if (scene_viewer->awaken)
		{
			struct LIST(cmzn_light) *changed_light_list =
				MANAGER_MESSAGE_GET_CHANGE_LIST(cmzn_light)(message, MANAGER_CHANGE_RESULT(cmzn_light));
			if (changed_light_list)
			{
				if (cmzn_sceneviewer_has_light_in_list(scene_viewer, changed_light_list))
				{
					scene_viewer->setChangedRepaint();
				}
				DESTROY_LIST(cmzn_light)(&changed_light_list);
			}
		}
	}
	else
	{
		return_code = 0;
	}
	return return_code;
}

void cmzn_sceneviewermodule_scenefilter_manager_callback(
	struct MANAGER_MESSAGE(cmzn_scenefilter) *message, void *sceneviewermodule_void)
{
	cmzn_sceneviewermodule *sceneviewermodule = (cmzn_sceneviewermodule *)sceneviewermodule_void;
	if (message && sceneviewermodule)
	{
		int change_summary = MANAGER_MESSAGE_GET_CHANGE_SUMMARY(cmzn_scenefilter)(message);
		if (change_summary & MANAGER_CHANGE_RESULT(cmzn_scenefilter))
		{
			// minimise scene messages while updating
			//MANAGER_BEGIN_CACHE(cmzn_scene)(graphics_module->scene_manager);
			FOR_EACH_OBJECT_IN_LIST(Scene_viewer)(
				cmzn_sceneviewer_scenefilter_change,(void *)message,
				sceneviewermodule->scene_viewer_list);
			//MANAGER_END_CACHE(cmzn_scene)(graphics_module->scene_manager);
		}
	}
}

void cmzn_sceneviewermodule_light_manager_callback(
	struct MANAGER_MESSAGE(cmzn_light) *message, void *sceneviewermodule_void)
{
	cmzn_sceneviewermodule *sceneviewermodule = (cmzn_sceneviewermodule *)sceneviewermodule_void;
	if (message && sceneviewermodule)
	{
		int change_summary = MANAGER_MESSAGE_GET_CHANGE_SUMMARY(cmzn_light)(message);
		if (change_summary & MANAGER_CHANGE_RESULT(cmzn_light))
		{
			// minimise scene messages while updating
			//MANAGER_BEGIN_CACHE(cmzn_scene)(graphics_module->scene_manager);
			FOR_EACH_OBJECT_IN_LIST(Scene_viewer)(
				cmzn_sceneviewer_light_change,(void *)message,
				sceneviewermodule->scene_viewer_list);
			//MANAGER_END_CACHE(cmzn_scene)(graphics_module->scene_manager);
		}
	}
}

cmzn_scene_id cmzn_sceneviewer_get_scene(cmzn_sceneviewer_id scene_viewer)
{
	if (scene_viewer && scene_viewer->scene)
	{
		return cmzn_scene_access(scene_viewer->scene);
	}
	return 0;
}

int cmzn_sceneviewer_set_scene(cmzn_sceneviewer_id scene_viewer,
	cmzn_scene_id scene)
{
	if (scene_viewer&&scene)
	{
		if (scene != scene_viewer->scene)
		{
			if (scene_viewer->scene)
			{
				cmzn_scene_triggers_top_region_change_callback(scene_viewer->scene);
				cmzn_scene_remove_callback(scene_viewer->scene,
					cmzn_scene_notify_scene_viewer_callback, (void *)scene_viewer);
			}
			cmzn_scene_destroy(&(scene_viewer->scene));
			scene_viewer->scene = cmzn_scene_access(scene);
			if (scene_viewer->awaken)
			{
				cmzn_scene_add_callback(scene_viewer->scene,
					cmzn_scene_notify_scene_viewer_callback, (void *)scene_viewer);
				scene_viewer->setChangedRepaint();
			}
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int Scene_viewer_scene_change(cmzn_sceneviewer_id scene_viewer)
{
	if (scene_viewer)
	{
		scene_viewer->setChangedRepaint();
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_sceneviewerevent::deaccess(cmzn_sceneviewerevent* &event)
{
	if (event)
	{
		--(event->access_count);
		if (event->access_count <= 0)
			delete event;
		event = 0;
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

void cmzn_sceneviewer_add_sceneviewernotifier(cmzn_sceneviewer *sceneviewer,
	cmzn_sceneviewernotifier *notifier)
{
	if (sceneviewer && notifier)
		sceneviewer->notifier_list->push_back(notifier->access());
}

void cmzn_sceneviewer_remove_sceneviewernotifier(cmzn_sceneviewer *sceneviewer,
	cmzn_sceneviewernotifier *notifier)
{
	if (sceneviewer && notifier)
	{
		cmzn_sceneviewernotifier_list::iterator iter = std::find(
			sceneviewer->notifier_list->begin(), sceneviewer->notifier_list->end(),
			notifier);
		if (iter != sceneviewer->notifier_list->end())
		{
			cmzn_sceneviewernotifier::deaccess(notifier);
			sceneviewer->notifier_list->erase(iter);
		}
	}
}

cmzn_sceneviewernotifier::cmzn_sceneviewernotifier(cmzn_sceneviewer *sceneviewerIn) :
	sceneviewer(sceneviewerIn),
	function(0),
	user_data(0),
	access_count(1)
{
	cmzn_sceneviewer_add_sceneviewernotifier(sceneviewer, this);
}

cmzn_sceneviewernotifier::~cmzn_sceneviewernotifier()
{
}

int cmzn_sceneviewernotifier::deaccess(cmzn_sceneviewernotifier* &notifier)
{
	if (notifier)
	{
		--(notifier->access_count);
		if (notifier->access_count <= 0)
			delete notifier;
		else if ((1 == notifier->access_count) && notifier->sceneviewer)
			cmzn_sceneviewer_remove_sceneviewernotifier(notifier->sceneviewer, notifier);
		notifier = 0;
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_sceneviewernotifier::setCallback(cmzn_sceneviewernotifier_callback_function function_in,
	void *user_data_in)
{
	if (!function_in)
		return CMZN_ERROR_ARGUMENT;
	this->function = function_in;
	this->user_data = user_data_in;
	return CMZN_OK;
}

void cmzn_sceneviewernotifier::clearCallback()
{
	this->function = 0;
	this->user_data = 0;
}

void cmzn_sceneviewernotifier::sceneviewerDestroyed()
{
	this->sceneviewer = 0;
	if (this->function)
	{
		cmzn_sceneviewerevent_id event = new cmzn_sceneviewerevent();
		event->changeFlags = CMZN_SCENEVIEWEREVENT_CHANGE_FLAG_FINAL;
		(this->function)(event, this->user_data);
		cmzn_sceneviewerevent_destroy(&event);
		this->clearCallback();
	}
}

cmzn_sceneviewernotifier_id cmzn_sceneviewer_create_sceneviewernotifier(
	cmzn_sceneviewer_id sceneviewer)
{
	return cmzn_sceneviewernotifier::create(sceneviewer);
}


int cmzn_sceneviewernotifier_clear_callback(cmzn_sceneviewernotifier_id notifier)
{
	if (notifier)
	{
		notifier->clearCallback();
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_sceneviewernotifier_set_callback(cmzn_sceneviewernotifier_id notifier,
	cmzn_sceneviewernotifier_callback_function function_in, void *user_data_in)
{
	if (notifier && function_in)
		return notifier->setCallback(function_in, user_data_in);
	return CMZN_ERROR_ARGUMENT;
}

void *cmzn_sceneviewernotifier_get_callback_user_data(
 cmzn_sceneviewernotifier_id notifier)
{
	if (notifier)
		return notifier->getUserData();
	return 0;
}

cmzn_sceneviewernotifier_id cmzn_sceneviewernotifier_access(
	cmzn_sceneviewernotifier_id notifier)
{
	if (notifier)
		return notifier->access();
	return 0;
}

int cmzn_sceneviewernotifier_destroy(cmzn_sceneviewernotifier_id *notifier_address)
{
	return cmzn_sceneviewernotifier::deaccess(*notifier_address);
}

cmzn_sceneviewerevent_id cmzn_sceneviewerevent_access(
	cmzn_sceneviewerevent_id event)
{
	if (event)
		return event->access();
	return 0;
}

int cmzn_sceneviewerevent_destroy(cmzn_sceneviewerevent_id *event_address)
{
	return cmzn_sceneviewerevent::deaccess(*event_address);
}

cmzn_sceneviewerevent_change_flags cmzn_sceneviewerevent_get_change_flags(
	cmzn_sceneviewerevent_id event)
{
	return event->changeFlags;
}

class cmzn_sceneviewer_projection_mode_conversion
{
public:
	static const char *to_string(enum cmzn_sceneviewer_projection_mode projection_mode)
	{
		const char *enum_string = 0;
		switch (projection_mode)
		{
			case CMZN_SCENEVIEWER_PROJECTION_MODE_PARALLEL:
				enum_string = "PARALLEL";
				break;
			case CMZN_SCENEVIEWER_PROJECTION_MODE_PERSPECTIVE:
				enum_string = "PERSPECTIVE";
				break;
		default:
			break;
		}
		return enum_string;
	}
};

enum cmzn_sceneviewer_projection_mode
	cmzn_sceneviewer_projection_mode_enum_from_string(const char *string)
{
	return string_to_enum<enum cmzn_sceneviewer_projection_mode,
		cmzn_sceneviewer_projection_mode_conversion>(string);
}

char *cmzn_sceneviewer_projection_mode_enum_to_string(
	enum cmzn_sceneviewer_projection_mode mode)
{
	const char *mode_string = cmzn_sceneviewer_projection_mode_conversion::to_string(mode);
	return (mode_string ? duplicate_string(mode_string) : 0);
}


class cmzn_sceneviewer_transparency_mode_conversion
{
public:
	static const char *to_string(enum cmzn_sceneviewer_transparency_mode transparency_mode)
	{
		const char *enum_string = 0;
		switch (transparency_mode)
		{
			case CMZN_SCENEVIEWER_TRANSPARENCY_MODE_FAST:
				enum_string = "FAST";
				break;
			case CMZN_SCENEVIEWER_TRANSPARENCY_MODE_SLOW:
				enum_string = "SLOW";
				break;
			case CMZN_SCENEVIEWER_TRANSPARENCY_MODE_ORDER_INDEPENDENT:
				enum_string = "ORDER_INDEPENDENT";
		default:
			break;
		}
		return enum_string;
	}
};

enum cmzn_sceneviewer_transparency_mode
	cmzn_sceneviewer_transparency_mode_enum_from_string(const char *string)
{
	return string_to_enum<enum cmzn_sceneviewer_transparency_mode,
		cmzn_sceneviewer_transparency_mode_conversion>(string);
}

char *cmzn_sceneviewer_transparency_mode_enum_to_string(
	enum cmzn_sceneviewer_transparency_mode mode)
{
	const char *mode_string = cmzn_sceneviewer_transparency_mode_conversion::to_string(mode);
	return (mode_string ? duplicate_string(mode_string) : 0);
}

int cmzn_sceneviewer_read_description(cmzn_sceneviewer_id sceneviewer, const char *description)
{
	if (sceneviewer && description)
	{
		SceneviewerJsonImport jsonImport(sceneviewer);
		std::string inputString(description);
		return jsonImport.import(inputString);
	}
	return CMZN_ERROR_ARGUMENT;
}

char *cmzn_sceneviewer_write_description(cmzn_sceneviewer_id sceneviewer)
{
	if (sceneviewer)
	{
		SceneviewerJsonExport jsonExport(sceneviewer);
		return duplicate_string(jsonExport.getExportString().c_str());
	}
	return 0;
}
