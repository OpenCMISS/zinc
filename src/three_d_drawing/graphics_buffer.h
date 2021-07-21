/*******************************************************************************
FILE : graphics_buffer.h

LAST MODIFIED : 4 June 2004

DESCRIPTION :
This provides a Cmgui interface to the OpenGL contexts of many types.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (GRAPHICS_BUFFER_H)
#define GRAPHICS_BUFFER_H

#include "opencmiss/zinc/zincconfigure.h"

#if defined (OPENGL_API)
#  if defined (GTK_USER_INTERFACE)
#     define GRAPHICS_BUFFER_USE_BUFFERS
#     define GRAPHICS_BUFFER_USE_OFFSCREEN_BUFFERS
#  endif /* defined (GTK_USER_INTERFACE) */
#endif /* defined (OPENGL_API) */
#if defined (USE_GLEW)
#	include <GL/glew.h>
#endif

#include "general/callback.h"
#include "three_d_drawing/abstract_graphics_buffer.h"

/*
Global functions
----------------
*/

enum Graphics_buffer_type
{
	GRAPHICS_BUFFER_INVALID_TYPE,
	GRAPHICS_BUFFER_GLX_X3D_TYPE,
	GRAPHICS_BUFFER_GLX_DM_PBUFFER_TYPE, /* Special type available only on O2's */
	GRAPHICS_BUFFER_GLX_PBUFFER_TYPE, /* Accelerated offscreen rendering */
	GRAPHICS_BUFFER_GLX_PIXMAP_TYPE, /* Non shared offscreen, no good for our display lists but can be used for find_xi_special buffer */
	GRAPHICS_BUFFER_GTKGLAREA_TYPE,
	GRAPHICS_BUFFER_GTKGLEXT_TYPE,
	GRAPHICS_BUFFER_WIN32_TYPE,
	GRAPHICS_BUFFER_WIN32_COPY_PBUFFER_TYPE, /* Accelerated offscreen rendering, automatically copied on to screen. */
	GRAPHICS_BUFFER_WIN32_COPY_BITMAP_TYPE, /* Non accelerated offscreen rendering, automatically copied on to screen. */
	GRAPHICS_BUFFER_WX_TYPE,
	GRAPHICS_BUFFER_WX_OFFSCREEN_TYPE,
	GRAPHICS_BUFFER_CARBON_TYPE,
	GRAPHICS_BUFFER_ONSCREEN_TYPE,
	GRAPHICS_BUFFER_GL_EXT_FRAMEBUFFER_TYPE
};

enum Graphics_buffer_buffering_mode
{
	GRAPHICS_BUFFER_ANY_BUFFERING_MODE,
	GRAPHICS_BUFFER_SINGLE_BUFFERING,
	GRAPHICS_BUFFER_DOUBLE_BUFFERING,
	GRAPHICS_BUFFER_RENDER_OFFSCREEN_AND_COPY,
	GRAPHICS_BUFFER_RENDER_OFFSCREEN_AND_BLEND
};

enum Graphics_buffer_stereo_mode
{
	GRAPHICS_BUFFER_ANY_STEREO_MODE,
	GRAPHICS_BUFFER_MONO,
	GRAPHICS_BUFFER_STEREO
};

struct Graphics_buffer_package
/*******************************************************************************
LAST MODIFIED : 5 May 2004

DESCRIPTION :
==============================================================================*/
{
	int override_visual_id;
};


struct Graphics_buffer
{
	unsigned int access_count;
	unsigned int width;
	unsigned int height;
	unsigned int origin_x;
	unsigned int origin_y;
	Graphics_buffer_type type;
	enum Graphics_buffer_buffering_mode buffering_mode;
	enum Graphics_buffer_stereo_mode stereo_mode;
	int *attrib_list;
#if defined (OPENGL_API)
	GLuint fbo, depthbuffer, img;
#if defined (USE_MSAA)
	GLuint msbuffer, multi_depthbuffer, multi_fbo;
#endif
#endif
	Graphics_buffer_package *package;

};

struct Graphics_buffer_package *CREATE(Graphics_buffer_package)();
/*******************************************************************************
LAST MODIFIED : 6 May 2004

DESCRIPTION :
Creates a Graphics_buffer_package which enables Graphics_buffers created from
it to share graphics contexts.
==============================================================================*/

int DESTROY(Graphics_buffer_package)(struct Graphics_buffer_package **package_ptr);
/*******************************************************************************
LAST MODIFIED : 5 May 2004

DESCRIPTION :
Closes the Graphics buffer package
==============================================================================*/

int Graphics_buffer_package_set_override_visual_id(
	struct Graphics_buffer_package *graphics_buffer_package,
	int override_visual_id);
/*******************************************************************************
LAST MODIFIED : 21 May 2004

DESCRIPTION :
Sets a particular visual to be used by all graphics buffers.
==============================================================================*/

PROTOTYPE_OBJECT_FUNCTIONS(Graphics_buffer);

int Graphics_buffer_is_visible(struct Graphics_buffer *buffer);

enum Graphics_buffer_type Graphics_buffer_get_type(struct Graphics_buffer *buffer);

unsigned int Graphics_buffer_get_height(struct Graphics_buffer *buffer);

int Graphics_buffer_set_height(struct Graphics_buffer *buffer, unsigned int height);

unsigned int Graphics_buffer_get_width(struct Graphics_buffer *buffer);

int Graphics_buffer_set_width(struct Graphics_buffer *buffer, unsigned int width);

unsigned int Graphics_buffer_get_origin_x(struct Graphics_buffer *buffer);

unsigned int Graphics_buffer_get_origin_y(struct Graphics_buffer *buffer);

int Graphics_buffer_get_buffering_mode(struct Graphics_buffer *buffer,
	enum Graphics_buffer_buffering_mode *buffering_mode);

int Graphics_buffer_get_stereo_mode(struct Graphics_buffer *buffer,
	enum Graphics_buffer_stereo_mode *stereo_mode);

struct Graphics_buffer *CREATE(Graphics_buffer)(
	struct Graphics_buffer_package *graphics_buffer_package,
	enum Graphics_buffer_type type,
	enum Graphics_buffer_buffering_mode buffering_mode,
	enum Graphics_buffer_stereo_mode stereo_mode);
/*******************************************************************************
LAST MODIFIED : 7 December 2006

DESCRIPTION :
==============================================================================*/

int DESTROY(Graphics_buffer)(struct Graphics_buffer **buffer_ptr);

#if defined (OPENGL_API) && defined (GL_EXT_framebuffer_object)
void Graphics_buffer_initialise_framebuffer(struct Graphics_buffer *buffer, int width, int height);

void Graphics_buffer_bind_framebuffer(struct Graphics_buffer *buffer);
#endif

#if defined (OPENGL_API) && defined (USE_MSAA)
int Graphics_buffer_set_multisample_framebuffer(struct Graphics_buffer *buffer, int preferred_antialias);

void Graphics_buffer_blit_framebuffer(struct Graphics_buffer *buffer);

void Graphics_buffer_reset_multisample_framebuffer(struct Graphics_buffer *buffer);
#endif

#endif /* !defined (GRAPHICS_BUFFER_H) */
