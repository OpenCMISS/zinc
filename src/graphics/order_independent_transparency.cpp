/*******************************************************************************
FILE : order_independent_transparency.cpp

LAST MODIFIED : 19 March 2008

DESCRIPTION :
Implements in NVIDIA and ATI hardware and Mesa software a depth sorting
algorithm where each depth layer is peeled off and then composited back
together from back to front at the end.

HISTORY :
  Based on algorithm from NVIDIA using SGIS_shadow and NV_texture_shader to
  implement Rui Bastos's idea for getting the layers of z sorted
  by depth, and using the RGBA at each layer to re-order transparency
  correctly.
  Cass Everitt 3-28-01
  SAB. I got rid of the glh, c++ and nv_parse stuff.
  Uses ARB vertex and fragment programs.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <stdlib.h>

#include "general/debug.h"
#include "general/message.h"
#include "graphics/graphics_library.h"
#include "graphics/material.h"
#include "graphics/scene.hpp"
#include "graphics/scene_viewer.h"
#include "graphics/material.hpp"

#define GL_GLEXT_PROTOTYPES
#define GLX_GLXEXT_PROTOTYPES

#include "graphics/order_independent_transparency.h"

#if ! defined GL_ARB_texture_rectangle
#   if defined GL_EXT_texture_rectangle
#      define GL_ARB_texture_rectangle GL_EXT_texture_rectangle
#   endif /* defined GL_EXT_texture_rectangle */
#endif /* ! defined GL_ARB_texture_rectangle */

static const char *required_extensions_default[] = {"GL_ARB_texture_rectangle",
													"GL_ARB_fragment_program_shadow",
												  "GL_ARB_depth_texture",
												  "GL_ARB_shadow"};
/* Mesa actually does the shadow test automatically but does not expose
the extension, and ATI on Linux has the extension but does not list it
in the extensions string. */
static const char *required_extensions_mesa_ati[] = {"GL_ARB_texture_rectangle",
												  "GL_ARB_depth_texture"};
#if defined GL_ARB_texture_rectangle && defined GL_ARB_vertex_program \
   && defined GL_ARB_fragment_program && defined  GL_ARB_fragment_program_shadow \
   && defined GL_ARB_depth_texture && defined GL_ARB_shadow
#define ORDER_INDEPENDENT_CAPABLE
#endif

#if defined (ORDER_INDEPENDENT_CAPABLE)
struct cmzn_sceneviewer_transparency_order_independent_data
/*******************************************************************************
LAST MODIFIED : 2 May 2005

DESCRIPTION :
The private user data for this order independent transparency rendering pass.
==============================================================================*/
{
	GLuint ztex_texture_id;
	GLuint *rgba_layer_texture_id;

	int viewport_width;
	int viewport_height;

	int using_stencil_overlay;

	int number_of_layers;
	int maximum_number_of_layers;

	GLuint *zbuffer;

	GLenum depth_format;

	enum Graphics_library_vendor_id vendor_id;

	struct Scene_viewer *scene_viewer;
};

static void set_texgen_planes(GLenum plane_type, GLfloat matrix[16])
/*******************************************************************************
LAST MODIFIED : 14 April 2003

DESCRIPTION :
Sets all the texture generation coordinates to <plane_type> according to
the planes in the <matrix>.
==============================================================================*/
{
	int i;
	GLenum coord[] = {GL_S, GL_T, GL_R, GL_Q };

	ENTER(set_texgen_planes);
	for(i = 0; i < 4; i++)
	{
		glTexGenfv(coord[i], plane_type, matrix + 4 * i);
	}
	LEAVE;
} /* set_texgen_planes */

void eye_linear_texgen(GLfloat matrix[16])
/*******************************************************************************
LAST MODIFIED : 14 April 2003

DESCRIPTION :
Sets all the texture generation coordinates to GL_EYE_LINEAR according to
the planes in the <matrix>.
==============================================================================*/
{
	ENTER(eye_linear_texgen);

	set_texgen_planes(GL_EYE_PLANE, matrix);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);

	LEAVE;
} /* eye_linear_texgen */

void obj_linear_texgen(GLfloat matrix[16])
/*******************************************************************************
LAST MODIFIED : 14 April 2003

DESCRIPTION :
Sets all the texture generation coordinates to GL_OBJECT_LINEAR according to
the planes in the <matrix>.
==============================================================================*/
{
	ENTER(eye_linear_texgen);

	set_texgen_planes(GL_OBJECT_PLANE, matrix);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

	LEAVE;
} /* obj_linear_texgen */

static int order_independent_init_opengl(
	struct cmzn_sceneviewer_transparency_order_independent_data *data)
/*******************************************************************************
LAST MODIFIED : 2 May 2005

DESCRIPTION :
Initialises the order independent transparency extension.
==============================================================================*/
{
	int return_code;

	ENTER(order_independent_init_opengl);

	return_code = 1;
#if defined (REPORT_GL_ERRORS)
		{
			char message[200];
			GLenum error;
			while(GL_NO_ERROR!=(error = glGetError()))
			{
				strcpy(message,"order_independent_init_opengl A: GL ERROR ");
				strcat(message, (char *)gluErrorString(error));
				display_message(ERROR_MESSAGE, message);
			}
		}
#endif /* defined (REPORT_GL_ERRORS) */
	if (!data->ztex_texture_id)
	{
		glGenTextures(1, &data->ztex_texture_id);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, data->ztex_texture_id);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
		if (Graphics_library_vendor_mesa == data->vendor_id)
		{
			glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
		}
#if defined (REPORT_GL_ERRORS)
		{
			char message[200];
			GLenum error;
			while(GL_NO_ERROR!=(error = glGetError()))
			{
				strcpy(message,"order_independent_init_opengl BA: GL ERROR ");
				strcat(message, (char *)gluErrorString(error));
				display_message(ERROR_MESSAGE, message);
			}
		}
#endif /* defined (REPORT_GL_ERRORS) */

	}

	LEAVE;

	return (return_code);
} /* order_independent_init_opengl */

static void render_scene_from_camera_view(int layer,
	struct Scene_viewer_rendering_data *rendering_data,
	struct cmzn_sceneviewer_transparency_order_independent_data *data,
	double *projection_matrix, double *modelview_matrix)
/*******************************************************************************
LAST MODIFIED : 2 May 2005

DESCRIPTION :
Draws one peeled layer of the scene.
==============================================================================*/
{
	ENTER(render_scene_from_camera_view);
	USE_PARAMETER(modelview_matrix);
	USE_PARAMETER(projection_matrix);

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	if ((Graphics_library_check_extension(GL_ARB_fragment_program)))
	{
		 glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 1,
				static_cast<GLfloat>(data->viewport_width), static_cast<GLfloat>(data->viewport_height), 1.0, 1.0);
	}
	if(layer > 0)
	{
		glActiveTexture(GL_TEXTURE3);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);

		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, data->ztex_texture_id);
		if (Graphics_library_vendor_ati == data->vendor_id)
		{
			glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
			glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
			glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
		}
		else if (Graphics_library_vendor_mesa == data->vendor_id)
		{
			glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
			glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
			glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
		}
		else
		{
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_NONE);
		}

	}
#if defined (DEBUG_CODE)
	else
	{
		/* So we can depth test even on the first path when debugging */
		glActiveTexture(GL_TEXTURE3);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);

		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, data->ztex_texture_id);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
		memset(data->zbuffer, 0, sizeof(GLuint) * data->viewport_width * data->viewport_height);
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, data->depth_format,
			data->viewport_width, data->viewport_height, 0,
			GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, data->zbuffer);

	}
#endif /* defined (DEBUG_CODE) */

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glActiveTexture(GL_TEXTURE0);

	glDisable(GL_TEXTURE_RECTANGLE_ARB);
#if !defined (GL_VERSION_2_0)
	glEnable(GL_VERTEX_PROGRAM_TWO_SIDE_ARB);
#else
	glEnable(GL_VERTEX_PROGRAM_TWO_SIDE);
#endif
	Scene_viewer_call_next_renderer(rendering_data);

	if(layer < data->number_of_layers - 1)
	{
		/* copy the z buffer unless this is the last peeling pass */
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, data->ztex_texture_id);
		glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, 0, 0,
			data->viewport_width, data->viewport_height);
		glActiveTexture(GL_TEXTURE0);
	}

	/* copy the RGBA of the layer */
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, data->rgba_layer_texture_id[layer]);
	glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, 0, 0,
		data->viewport_width, data->viewport_height);


	LEAVE;

} /* render_scene_from_camera_view */

static void draw_sorted_transparency(
	struct cmzn_sceneviewer_transparency_order_independent_data *data,
	enum cmzn_sceneviewer_blending_mode blending_mode)
/*******************************************************************************
LAST MODIFIED : 2 May 2005

DESCRIPTION :
Draw a textured quad for each layer and blend them all together correctly.
==============================================================================*/
{
	int i;
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, data->viewport_width, 0, data->viewport_height, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glClear(GL_COLOR_BUFFER_BIT);

	glDisable(GL_DEPTH_TEST);

	if (Graphics_library_check_extension(GL_shading_language))
	{
#if defined(GL_VERSION_2_0)
		 glUseProgram(0);
#endif
	}
	if (Graphics_library_check_extension(GL_ARB_fragment_program) &&
		Graphics_library_check_extension(GL_ARB_vertex_program))
	{
		 glDisable(GL_VERTEX_PROGRAM_ARB);
		 glDisable(GL_FRAGMENT_PROGRAM_ARB);
	}

	glActiveTexture(GL_TEXTURE3);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);

	glActiveTexture(GL_TEXTURE0);

	glDisable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	if (data->using_stencil_overlay)
	{
		/* disable stencil buffer to get the overlay in the back plane */
		glDisable(GL_STENCIL_TEST);
	}

	switch(blending_mode)
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
			glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
				GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_BLEND);
		} break;
#endif /* defined GL_VERSION_1_4 */
	}

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	for(i = data->number_of_layers - 1 ; i >= 0 ; i--)
	{
		if (data->number_of_layers - 2 == i)
		{
			if (data->using_stencil_overlay)
			{
				glEnable(GL_STENCIL_TEST);
			}
		}

		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, data->rgba_layer_texture_id[i]);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);

		glBegin(GL_QUADS);
		glTexCoord2f(0, 0);
		glVertex2f(0, 0);
		glTexCoord2f(0, static_cast<GLfloat>(data->viewport_height));
		glVertex2f(0, static_cast<GLfloat>(data->viewport_height));
		glTexCoord2f(static_cast<GLfloat>(data->viewport_width), static_cast<GLfloat>(data->viewport_height));
		glVertex2f(static_cast<GLfloat>(data->viewport_width), static_cast<GLfloat>(data->viewport_height));
		glTexCoord2f(static_cast<GLfloat>(data->viewport_width), 0);
		glVertex2f(static_cast<GLfloat>(data->viewport_width), 0);
		glEnd();
	}

	glDisable(GL_TEXTURE_RECTANGLE_ARB);

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);


	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	LEAVE;

}
#endif /* defined (ORDER_INDEPENDENT_CAPABLE) */

int order_independent_capable(void)
/*******************************************************************************
LAST MODIFIED : 2 May 2005

DESCRIPTION :
Returns true if the current display is capable of order independent transparency.
==============================================================================*/
{
#if defined (ORDER_INDEPENDENT_CAPABLE)
	GLint alpha_bits, depth_bits;
#endif /* defined (ORDER_INDEPENDENT_CAPABLE) */
	const char **required_extensions;
	enum Graphics_library_vendor_id vendor_id;
	int return_code;
	unsigned int i, number_of_required_extensions;
	ENTER(order_independent_capable);

	vendor_id = Graphics_library_get_vendor_id();
	if ((Graphics_library_vendor_mesa == vendor_id) ||
		(Graphics_library_vendor_ati == vendor_id))
	{
		required_extensions = required_extensions_mesa_ati;
		number_of_required_extensions = (sizeof(required_extensions_mesa_ati) / sizeof (char *));
	}
	else
	{
		required_extensions = required_extensions_default;
		number_of_required_extensions = (sizeof(required_extensions_default) / sizeof (char *));
	}
#if defined (ORDER_INDEPENDENT_CAPABLE)
	return_code = 1;
	if (Graphics_library_load_extension("GL_shading_language") ||
		(Graphics_library_load_extension("GL_ARB_vertex_program") &&
			Graphics_library_load_extension("GL_ARB_fragment_program")))
	{
		for (i = 0 ; i < number_of_required_extensions ; i++)
		{
			if (!Graphics_library_load_extension(required_extensions[i]))
			{
				return_code = 0;
			}
		}
	}
	glGetIntegerv(GL_DEPTH_BITS, &depth_bits);
	if ((depth_bits != 16) && (depth_bits != 24))
	{
		return_code = 0;
	}
	glGetIntegerv(GL_ALPHA_BITS, &alpha_bits);
	if (alpha_bits < 8)
	{
		return_code = 0;
	}
	if (!return_code)
	{
		display_message(ERROR_MESSAGE,
			"Order independent transparency not supported on this display\n"
			"It requries at least 8 alpha bits (detected %d), 16 or 24 bit depth buffer (detected %d) and "
			"these OpenGL extensions: ", alpha_bits, depth_bits);
		for (i = 0 ; i < number_of_required_extensions ; i++)
		{
			if (Graphics_library_load_extension(required_extensions[i]))
			{
				display_message(ERROR_MESSAGE,"%s: Available", required_extensions[i]);
			}
			else
			{
				display_message(ERROR_MESSAGE,"%s: Not Available", required_extensions[i]);
			}
		}
	}
#else /* defined (ORDER_INDEPENDENT_CAPABLE) */
	display_message(ERROR_MESSAGE,
		"Order independent transparency not compiled into this executable.  \n"
		"It requries at least 8 alpha bits, 16 or 24 bit depth buffer and "
		"these OpenGL extensions: ");
	for (i = 0 ; i < number_of_required_extensions ; i++)
	{
		display_message(ERROR_MESSAGE,"%s ", required_extensions[i]);
	}
	return_code = 0;
#endif /* defined (ORDER_INDEPENDENT_CAPABLE) */

	LEAVE;

	return (return_code);
} /* order_independent_capable */

struct cmzn_sceneviewer_transparency_order_independent_data *
   order_independent_initialise(struct Scene_viewer *scene_viewer)
/*******************************************************************************
LAST MODIFIED : 2 May 2005

DESCRIPTION :
Initialises the order independent transparency extension.
==============================================================================*/
{
#if defined (ORDER_INDEPENDENT_CAPABLE)
	GLint alpha_bits, depth_bits;
	int return_code;
#endif /* defined (ORDER_INDEPENDENT_CAPABLE) */
	struct cmzn_sceneviewer_transparency_order_independent_data *data;

	ENTER(order_independent_initialise);

#if defined (ORDER_INDEPENDENT_CAPABLE)
	if (ALLOCATE(data, struct cmzn_sceneviewer_transparency_order_independent_data,
		1))
	{
		return_code = 1;

		data->ztex_texture_id = 0;
		data->rgba_layer_texture_id = (GLuint *)NULL;

		data->viewport_width = 0;
		data->viewport_height = 0;

		data->number_of_layers = 0;
		data->maximum_number_of_layers = 0;

		data->zbuffer = (GLuint *)NULL;

		data->scene_viewer = scene_viewer;

		data->vendor_id = Graphics_library_get_vendor_id();

		glGetIntegerv(GL_DEPTH_BITS, &depth_bits);
		glGetIntegerv(GL_ALPHA_BITS, &alpha_bits);

		switch (depth_bits)
		{
			case 16:
			{
				data->depth_format = GL_DEPTH_COMPONENT16_ARB;
			} break;
			case 24:
			{
				data->depth_format = GL_DEPTH_COMPONENT24_ARB;
			} break;
			default:
			{
				display_message(ERROR_MESSAGE, "order_independent_initialise.  "
					"Unsupported depth format for order independent transparency");
				return_code = 0;
			} break;
		}

		if (alpha_bits < 8)
		{
			display_message(ERROR_MESSAGE, "order_independent_initialise.  "
				"This extension requires alpha planes to work, alpha_bits = %d",
				alpha_bits);
			return_code = 0;
		}

		if (!return_code)
		{
			DEALLOCATE(data);
			data = (struct cmzn_sceneviewer_transparency_order_independent_data *)NULL;
		}
		else
		{
			order_independent_init_opengl(data);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE, "order_independent_initialise.  "
				"Unable to allocate data structure\n");
		data = (struct cmzn_sceneviewer_transparency_order_independent_data *)NULL;
	}
#else /* defined (ORDER_INDEPENDENT_CAPABLE) */
	USE_PARAMETER(scene_viewer);
	data = (struct cmzn_sceneviewer_transparency_order_independent_data *)NULL;
#endif /* defined (ORDER_INDEPENDENT_CAPABLE) */

	LEAVE;

	return (data);
} /* order_independent_initialise */

int order_independent_reshape(
	struct cmzn_sceneviewer_transparency_order_independent_data *data,
	int width, int height, int layers, int using_stencil_overlay)
/*******************************************************************************
LAST MODIFIED : 2 May 2005

DESCRIPTION :
Initialises per rendering parts of this extension.
==============================================================================*/
{
#if defined (ORDER_INDEPENDENT_CAPABLE)
	int i;
#endif /* defined (ORDER_INDEPENDENT_CAPABLE) */
	int return_code;

	ENTER(order_independent_reshape);

#if defined (ORDER_INDEPENDENT_CAPABLE)
	data->using_stencil_overlay = using_stencil_overlay;
	/* We need one more layer for the background */
	layers++;
	if (data->ztex_texture_id && data->zbuffer && (data->viewport_width == width)
		&& (data->viewport_height == height) && (layers <= data->maximum_number_of_layers))
	{
		data->number_of_layers = layers;
		return_code = 1;
	}
	else
	{
		return_code = 1;
		if (!data->ztex_texture_id)
		{
			glGenTextures(1, &data->ztex_texture_id);
		}

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, data->ztex_texture_id);

		if (REALLOCATE(data->zbuffer, data->zbuffer, GLuint, width * height))
		{
			memset(data->zbuffer, 0, sizeof(GLuint) * width * height);
			data->viewport_width = width;
			data->viewport_height = height;
			glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, data->depth_format,
				width, height, 0,
				GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, data->zbuffer);
		}
		else
		{
			display_message(ERROR_MESSAGE, "order_independent_reshape.  "
				"Unable to allocate ztex buffer\n");
			return_code = 0;
		}
		glActiveTexture(GL_TEXTURE0);

		if (return_code && (!data->rgba_layer_texture_id ||
			(layers > data->maximum_number_of_layers)))
		{
			if (REALLOCATE(data->rgba_layer_texture_id, data->rgba_layer_texture_id,
				GLuint, layers))
			{
				for (i = data->maximum_number_of_layers ; i < layers ; i++)
				{
					glGenTextures(1, &(data->rgba_layer_texture_id[i]));
				}
				data->maximum_number_of_layers = layers;
			}
			else
			{
				display_message(ERROR_MESSAGE, "order_independent_reshape.  "
					"Unable to allocate rgba layer ids\n");
				return_code = 0;
			}
		}

		if (return_code)
		{
			data->number_of_layers = layers;
			for (i = 0 ; i < data->maximum_number_of_layers ; i++)
			{
				glBindTexture(GL_TEXTURE_RECTANGLE_ARB, data->rgba_layer_texture_id[i]);
				glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA8, width, height, 0,
					GL_RGBA, GL_UNSIGNED_BYTE, data->zbuffer);
				glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			}
		}
	}
#else /* defined (ORDER_INDEPENDENT_CAPABLE) */
	USE_PARAMETER(data);
	USE_PARAMETER(width);
	USE_PARAMETER(height);
	USE_PARAMETER(layers);
	USE_PARAMETER(using_stencil_overlay);
	return_code = 0;
#endif /* defined (ORDER_INDEPENDENT_CAPABLE) */

	LEAVE;

	return (return_code);
} /* order_independent_reshape */

void order_independent_display(struct Scene_viewer_rendering_data *rendering_data,
	struct cmzn_sceneviewer_transparency_order_independent_data *data,
	double *projection_matrix, double *modelview_matrix,
	enum cmzn_sceneviewer_blending_mode blending_mode)
/*******************************************************************************
LAST MODIFIED : 2 May 2005

DESCRIPTION :
Actually preforms the rendering pass.
==============================================================================*/
{
#if defined (ORDER_INDEPENDENT_CAPABLE)
	int layer;
#endif /* defined (ORDER_INDEPENDENT_CAPABLE) */

	ENTER(order_independent_display);

#if defined (ORDER_INDEPENDENT_CAPABLE)
	/* glViewport(0, 0, data->viewport_width, data->viewport_height); */

	/* Copy the image that is already drawn into the back layer */
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,
		data->rgba_layer_texture_id[data->number_of_layers - 1]);
	glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, 0, 0,
		data->viewport_width, data->viewport_height);

	/* Always create the textures with this, use the blending mode
		to composite the images together */
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);
	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL_EXT, GL_SEPARATE_SPECULAR_COLOR_EXT);

	USE_PARAMETER(data);
	USE_PARAMETER(projection_matrix);
	USE_PARAMETER(modelview_matrix);

	for(layer = 0; layer < data->number_of_layers - 1 ; layer++)
	{
		/* Recompile the materials for order_independent_transaprency */
		{
			struct Material_order_independent_transparency material_data;

			material_data.layer = layer + 1;
			material_data.renderer = Scene_viewer_rendering_data_get_renderer(rendering_data);

			cmzn_scene_id scene = cmzn_sceneviewer_get_scene(data->scene_viewer);
			cmzn_scene_for_each_material(scene,
				compile_Graphical_material_for_order_independent_transparency,
				(void *)&material_data);
			cmzn_scene_destroy(&scene);
		}

		render_scene_from_camera_view(layer, rendering_data, data,
			projection_matrix, modelview_matrix);
	}

	draw_sorted_transparency(data, blending_mode);

	/* Recompile the materials back to the original state */
	{
		struct Material_order_independent_transparency material_data;

		material_data.layer = 0;
		material_data.renderer = Scene_viewer_rendering_data_get_renderer(rendering_data);
		cmzn_scene_id scene = cmzn_sceneviewer_get_scene(data->scene_viewer);
		cmzn_scene_for_each_material(scene,
			compile_Graphical_material_for_order_independent_transparency,
			(void *)&material_data);
		cmzn_scene_destroy(&scene);
	}

#if defined (DEBUG_CODE)
	{
		FILE *out;
		printf("Writing depth.rgba display -size %dx%d -depth 8 depth.rgba\n", data->viewport_width, data->viewport_height);
		glReadPixels(0, 0, data->viewport_width, data->viewport_height, GL_DEPTH_COMPONENT,
			GL_UNSIGNED_INT, data->zbuffer);
		out = fopen("depth.rgba", "w");
		fwrite(data->zbuffer, sizeof(GLuint), data->viewport_width * data->viewport_height,
			out);
		fclose(out);
	}
#endif /* defined (DEBUG_CODE) */

#else /* defined (ORDER_INDEPENDENT_CAPABLE) */
	USE_PARAMETER(rendering_data);
	USE_PARAMETER(data);
	USE_PARAMETER(projection_matrix);
	USE_PARAMETER(modelview_matrix);
	USE_PARAMETER(blending_mode);
#endif /* defined (ORDER_INDEPENDENT_CAPABLE) */

	LEAVE;
} /* order_independent_display */

int order_independent_finalise(
	struct cmzn_sceneviewer_transparency_order_independent_data **data_address)
/*******************************************************************************
LAST MODIFIED : 2 May 2005

DESCRIPTION :
Frees the memory associated with the <data_address> and sets <data_address> to NULL.
==============================================================================*/
{
	int return_code;
#if defined (ORDER_INDEPENDENT_CAPABLE)
	int i;
	struct cmzn_sceneviewer_transparency_order_independent_data *data;
#endif /* defined (ORDER_INDEPENDENT_CAPABLE) */

	ENTER(order_independent_finalise);

#if defined (ORDER_INDEPENDENT_CAPABLE)
	if (data_address && (data = *data_address))
	{
		DEALLOCATE(data->zbuffer);
		for (i = 0 ; i < data->maximum_number_of_layers ; i++)
		{
			glDeleteTextures(1, &data->rgba_layer_texture_id[i]);
		}
		DEALLOCATE(data->rgba_layer_texture_id);

		if (data->ztex_texture_id)
		{
			glDeleteTextures(1, &data->ztex_texture_id);
		}

		DEALLOCATE(*data_address);
		*data_address = (struct cmzn_sceneviewer_transparency_order_independent_data *)NULL;

		return_code = 1;
	}
	else
	{
		return_code = 0;
	}
#else /* defined (ORDER_INDEPENDENT_CAPABLE) */
	USE_PARAMETER(data_address);
	return_code = 0;
#endif /* defined (ORDER_INDEPENDENT_CAPABLE) */

	LEAVE;

	return (return_code);
} /* order_independent_finalise */
