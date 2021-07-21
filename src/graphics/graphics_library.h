/*******************************************************************************
FILE : graphics_library.h

LAST MODIFIED : 17 July 2006

DESCRIPTION :
Functions and structures for interfacing with the graphics library.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (GRAPHICS_LIBRARY_H)
#define GRAPHICS_LIBRARY_H

#include "opencmiss/zinc/zincconfigure.h"
#include "opencmiss/zinc/zincsharedobject.h"

#if defined (OPENGL_API)
#	define GL_GLEXT_PROTOTYPES
#	define GLX_GLXEXT_PROTOTYPES
#	if !defined (DARWIN)
#		if defined (WIN32_SYSTEM)
//#			define WINDOWS_LEAN_AND_MEAN
#			if !defined (NOMINMAX)
#				define NOMINMAX
#			endif /*  !defined (NOMINMAX) */
#			include <windows.h>
#		endif
#		if defined (USE_GLEW)
#			include <GL/glew.h>
#		else
#			include <GL/gl.h>
#			include <GL/glu.h>
#		endif
#		if defined (WIN32_SYSTEM)
#			undef GL_NV_vertex_program
#			undef GL_NV_register_combiners2
#		endif /* defined (WIN32_SYSTEM) */
#	endif /* ! defined (DARWIN) */
#endif /* defined (OPENGL_API) */
#include "graphics/texture.h"
#if defined (GTK_USER_INTERFACE)
#	if defined (UNIX)
#		include <gdk/gdkgl.h>
#	endif /* defined (UNIX) */
#endif /* defined (GTK_USER_INTERFACE) */
#if defined (UNIX) && defined (DARWIN)
#	if defined (USE_GLEW)
#		include <GL/glew.h>
#	else
#		include <OpenGL/gl.h>
#		include <OpenGL/glu.h>
#	endif
#endif /* defined UNIX && defined (DARWIN) */

/*
Global types
------------
*/

typedef ZnReal gtMatrix[4][4];

enum Graphics_library_vendor_id
{
	Graphics_library_vendor_unknown,
	Graphics_library_vendor_ati,
	Graphics_library_vendor_nvidia,
	Graphics_library_vendor_mesa,
	Graphics_library_vendor_microsoft,
	Graphics_library_vendor_intel
};

/*
Global variables
----------------
*/
extern int graphics_library_open;

/*
Global functions
----------------
*/

int has_current_context();

int open_graphics_library(void);
/*******************************************************************************
LAST MODIFIED : 23 March 1993

DESCRIPTION :
Function to open the PHIGS graphics library.
==============================================================================*/

int close_graphics_library(void);
/*******************************************************************************
LAST MODIFIED : 23 March 1993

DESCRIPTION :
Function to close the PHIGS graphics library.
==============================================================================*/

int initialize_graphics_library();
/*******************************************************************************
LAST MODIFIED : 14 November 1996

DESCRIPTION :
Sets up the default light, material and light model for the graphics library.
==============================================================================*/

int gtMatrix_is_identity(gtMatrix *matrix);
/*******************************************************************************
LAST MODIFIED : 28 November 2001

DESCRIPTION :
Returns true if <matrix> is the 4x4 identity
==============================================================================*/

int gtMatrix_match(gtMatrix *matrix1, gtMatrix *matrix2);
/*******************************************************************************
LAST MODIFIED : 28 November 2001

DESCRIPTION :
Returns true if <matrix1> and <matrix2> are identical.
==============================================================================*/

int gtMatrix_match_with_tolerance(gtMatrix *matrix1, gtMatrix *matrix2,
	GLfloat tolerance);
/*******************************************************************************
LAST MODIFIED : 5 December 2001

DESCRIPTION :
Returns true if <matrix1> and <matrix2> have no components different by
more than <tolerance> times the largest absolute value in either matrix.
==============================================================================*/

int gtMatrix_to_euler(gtMatrix matrix, GLfloat *euler_angles);
/*******************************************************************************
LAST MODIFIED : 21 November 2002

DESCRIPTION :
Cleaned this up from view/coord_trans.c
Returns <euler_angles> in radians.
==============================================================================*/

int euler_to_gtMatrix(GLfloat *euler_angles, gtMatrix matrix);
/*******************************************************************************
LAST MODIFIED : 21 November 2002

DESCRIPTION :
Cleaned this up from view/coord_trans.c
<euler_angles> are in radians.
==============================================================================*/

/***************************************************************************//**
* Returns the multiplied matrix.
*
*/
int multiply_gtMatrix(gtMatrix *a, gtMatrix *b, gtMatrix *c);

#if defined (OPENGL_API)
void wrapperLoadCurrentMatrix(gtMatrix *theMatrix);
/*******************************************************************************
LAST MODIFIED : 13 February 1995

DESCRIPTION :
==============================================================================*/

void wrapperMultiplyCurrentMatrix(gtMatrix *theMatrix);
/*******************************************************************************
LAST MODIFIED : 13 February 1995

DESCRIPTION :
==============================================================================*/
#endif

#if defined (OPENGL_API)
int query_gl_extension(const char *extName);
/*******************************************************************************
LAST MODIFIED : 9 September 1998

DESCRIPTION :
Search for extName in the GL extensions string. Use of strstr() is not sufficient
because extension names can be prefixes of other extension names. Could use
strtok() but the constant string returned by glGetString might be in read-only
memory.
???SAB.  Taken directly from the insight book on OpenGL Extensions
==============================================================================*/
#endif /* defined (OPENGL_API) */

#if defined (OPENGL_API)
int query_gl_version(int major_version, int minor_version);
/*******************************************************************************
LAST MODIFIED : 16 April 2003

DESCRIPTION :
Returns true if the OpenGL version is at least <major_version>.<minor_version>
==============================================================================*/
#endif /* defined (OPENGL_API) */

int Graphics_library_read_pixels(unsigned char *frame_data,
	int width, int height, enum Texture_storage_type storage,
	int front_buffer);
/*******************************************************************************
LAST MODIFIED : 30 June 2004

DESCRIPTION :
Read pixels from the current graphics context into <frame_data> of size <width>
and <height> according to the storage type.  'MakeCurrent' the desired source
before calling this routine.  If <front_buffer> is 1 then pixels will be read
from the front buffer, otherwise they will be read from the back buffer in a
double buffered context.
==============================================================================*/

enum Graphics_library_vendor_id Graphics_library_get_vendor_id(void);
/*******************************************************************************
LAST MODIFIED : 18 March 2008

DESCRIPTION :
Returns an enumeration which can be used to select for a particular vendor
implementation which is valid for the current OpenGL context.
==============================================================================*/

int Graphics_library_load_extension(const char *extension_name);
/*******************************************************************************
LAST MODIFIED : 2 March 2007

DESCRIPTION :
Attempts to load a single openGL extension.
If the extension symbol and all the functions used in cmgui from that extension are
found then returns GLEXTENSION_AVAILABLE.
If the extension list is defined but this extension is not available then
it returns GLEXTENSION_UNAVAILABLE.
If the extension string is not yet defined then the test is not definitive and
so it returns GLEXTENSION_UNSURE, allowing the calling procedure to react
appropriately.
==============================================================================*/

int Graphics_library_load_extensions(char *extensions);
/*******************************************************************************
LAST MODIFIED : 20 February 2004

DESCRIPTION :
Attempts to load the space separated list of extensions.  Returns true if all
the extensions succeed, false if not.
==============================================================================*/

#if defined (GTK_USER_INTERFACE)
#if defined (UNIX)
int Graphics_library_initialise_gtkglext_glx_extensions(GdkGLConfig *config);
/*******************************************************************************
LAST MODIFIED : 5 February 2007

DESCRIPTION :
To test GLX extensions requires a connection to the display.
==============================================================================*/
#endif /* defined (UNIX) */
#endif /* defined (GTK_USER_INTERFACE) */

#if defined (OPENGL_API)
   /* On UNIX systems we just test if we can load the handles but call the functions
	  directly.  On Win32 (and AIX I think) we need to use function ptrs to call each time */
#  if defined (WIN32_SYSTEM) || defined (AIX)
#    define GRAPHICS_LIBRARY_USE_EXTENSION_FUNCTION_HANDLES
#  endif /* defined (WIN32_SYSTEM) || defined (AIX) */

#define GLEXTENSION_UNSURE (255)
#define GLEXTENSION_AVAILABLE (1)
#define GLEXTENSION_UNAVAILABLE (0)

#  define GLEXTENSIONFLAG( extension_name ) extension_name ## _glextension_flag
#  define Graphics_library_check_extension(extension_name) \
	(GLEXTENSION_UNSURE == extension_name ## _glextension_flag ? (GLEXTENSION_AVAILABLE == Graphics_library_load_extension( #extension_name)) : extension_name ## _glextension_flag)
/*******************************************************************************
LAST MODIFIED : 2 March 2007

DESCRIPTION :
Ensure that the extension is available.
Use before calling functions from that extension.
==============================================================================*/

#  define Graphics_library_tentative_check_extension(extension_name) \
	(GLEXTENSION_UNAVAILABLE != extension_name ## _glextension_flag ? (GLEXTENSION_UNAVAILABLE != Graphics_library_load_extension( #extension_name)) : GLEXTENSION_UNAVAILABLE)
/*******************************************************************************
LAST MODIFIED : 2 March 2007

DESCRIPTION :
This is successful if the extension is available or if it is unsure.
Use before allowing parameters to be set in commands.  It is useful to fail
if we know the command won't work, but if we haven't opened any graphics context
yet we just don't know.
==============================================================================*/

#if defined (GRAPHICS_LIBRARY_USE_EXTENSION_FUNCTION_HANDLES)
#  define GLHANDLE( function_name ) function_name ## _handle
#endif /* defined GRAPHICS_LIBRARY_USE_EXTENSION_FUNCTION_HANDLES */

#if defined (GRAPHICS_LIBRARY_C)
/* This is being included from the C file so do the initialisations */
#  define GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(extension_name) \
	unsigned char extension_name ## _glextension_flag = GLEXTENSION_UNSURE
#  define GRAPHICS_LIBRARY_EXTERN
#else /* defined (GRAPHICS_LIBRARY_C) */
#  define GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(extension_name) \
	extern unsigned char extension_name ## _glextension_flag
#  define GRAPHICS_LIBRARY_EXTERN extern
#endif /* defined (GRAPHICS_LIBRARY_C) */

/* Extension flags */
#if defined (GLX_ARB_get_proc_address)
  GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(GLX_ARB_get_proc_address);
#endif /* defined (GLX_ARB_get_proc_address) */
#if defined (GL_VERSION_1_1)
  GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(GL_VERSION_1_1);
#endif /* defined (GL_VERSION_1_1) */
#if defined (GL_VERSION_1_2)
  GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(GL_VERSION_1_2);
#endif /* defined (GL_VERSION_1_2) */
#if defined (GL_VERSION_1_3)
  GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(GL_VERSION_1_3);
#endif /* defined (GL_VERSION_1_3) */
#if defined (GL_VERSION_1_4)
  GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(GL_VERSION_1_4);
#endif /* defined (GL_VERSION_1_4) */
#if defined (GL_VERSION_1_5)
  GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(GL_VERSION_1_5);
#endif /* defined (GL_VERSION_1_5) */
#if defined (GL_VERSION_2_0)
  GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(GL_VERSION_2_0);
#endif /* defined (GL_VERSION_2_0) */
#if defined (GL_VERSION_3_0)
  GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(GL_VERSION_3_0);
#endif /* defined (GL_VERSION_3_0) */
#if defined (GL_ARB_depth_texture)
  GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(GL_ARB_depth_texture);
#endif /* defined (GL_ARB_depth_texture) */
#if defined (GL_ARB_draw_buffers)
  GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(GL_ARB_draw_buffers);
#endif /* defined (GL_ARB_draw_buffers) */
#if defined (GL_ARB_fragment_program)
  GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(GL_ARB_fragment_program);
#endif /* defined (GL_ARB_fragment_program) */
#if defined (GL_ARB_fragment_program_shadow)
  GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(GL_ARB_fragment_program_shadow);
#endif /* defined (GL_ARB_fragment_program_shadow) */
#if defined (GL_ARB_texture_compression)
  GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(GL_ARB_texture_compression);
#endif /* defined (GL_ARB_texture_compression) */
#if defined (GL_ARB_texture_float)
  GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(GL_ARB_texture_float);
#endif /* defined (GL_ARB_texture_float) */
#if defined (GL_ARB_texture_non_power_of_two)
  GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(GL_ARB_texture_non_power_of_two);
#endif /* defined (GL_ARB_texture_non_power_of_two) */
#if defined (GL_ARB_texture_rectangle)
  GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(GL_ARB_texture_rectangle);
#endif /* defined (GL_ARB_texture_rectangle) */
#if defined (GL_ARB_vertex_buffer_object)
  GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(GL_ARB_vertex_buffer_object);
#endif /* defined (GL_ARB_vertex_buffer_object) */
#if defined (GL_ARB_vertex_program)
  GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(GL_ARB_vertex_program);
#endif /* defined (GL_ARB_vertex_program) */
#if defined (GL_ARB_shadow)
  GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(GL_ARB_shadow);
#endif /* defined (GL_ARB_shadow) */
#if defined (GL_ATI_texture_float)
  GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(GL_ATI_texture_float);
#endif /* defined (GL_ATI_texture_float) */
#if defined (GL_EXT_abgr)
  GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(GL_EXT_abgr);
#endif /* defined (GL_EXT_abgr) */
#if defined (GL_EXT_texture3D)
  GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(GL_EXT_texture3D);
#endif /* defined (GL_EXT_texture3D) */
#if defined (GL_EXT_vertex_array) || defined (GL_VERSION_1_1)
  GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(GL_EXT_vertex_array);
#endif /* defined (GL_EXT_vertex_array) */
#if defined (GL_EXT_framebuffer_object)
  GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(GL_EXT_framebuffer_object);
#endif /* (GL_EXT_framebuffer_object) */
#if defined (GL_EXT_framebuffer_blit)
  GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(GL_EXT_framebuffer_blit);
#endif /* (GL_EXT_framebuffer_blit) */
#if defined (GL_EXT_framebuffer_multisample)
  GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(GL_EXT_framebuffer_multisample);
#endif /* (GL_EXT_framebuffer_multisample) */
#if defined (GL_NV_float_buffer)
  GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(GL_NV_float_buffer);
#endif /* defined (GL_NV_float_buffer) */
#if defined (GL_VERSION_2_0)
  GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(GL_shading_language);
#endif
#if defined (GL_VERSION_1_4)
  GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(GL_SGIS_generate_mipmap);
#endif
/* Fake extension to control whether display lists are used or not.
 * This is only determined by the corresponding CMZN_GL_display_list environment
 * variable. */
GRAPHICS_LIBRARY_INITIALISE_GLEXTENSIONFLAG(GL_display_lists);
/* Extension function handles */
#if defined (GRAPHICS_LIBRARY_USE_EXTENSION_FUNCTION_HANDLES) && !defined (USE_GLEW)
#  if defined (APIENTRY)
   /* Testing to see if we are using Mesa like headers (NVIDIA defined APIENTRY and not APIENTRYP) */

#    if defined (GL_VERSION_1_2) || defined (GL_EXT_texture3D)
	   /* Note that while to strictly satisfy the GL_EXT_texture3D
		this function would have the EXT delimiter the SGI
		OpenGL is version 1.1 with GL_EXT_texture3D but supplies
		all the 1.2 compliant symbols, so the code is simpler with
		just one version */
	   GRAPHICS_LIBRARY_EXTERN PFNGLTEXIMAGE3DPROC GLHANDLE(glTexImage3D);
#      define glTexImage3D (GLHANDLE(glTexImage3D))
#    endif /* defined (GL_VERSION_1_2) || defined (GL_EXT_texture3D) */

#    if defined (GL_VERSION_1_3)
	   GRAPHICS_LIBRARY_EXTERN PFNGLACTIVETEXTUREPROC GLHANDLE(glActiveTexture);
#      define glActiveTexture (GLHANDLE(glActiveTexture))
	   GRAPHICS_LIBRARY_EXTERN PFNGLCLIENTACTIVETEXTUREPROC GLHANDLE(glClientActiveTexture);
#      define glClientActiveTexture (GLHANDLE(glClientActiveTexture))
	   GRAPHICS_LIBRARY_EXTERN PFNGLMULTITEXCOORD3FVPROC GLHANDLE(glMultiTexCoord3fv);
#      define glMultiTexCoord3fv (GLHANDLE(glMultiTexCoord3fv))
#    endif /* defined (GL_VERSION_1_3) */

#    if defined (GL_VERSION_1_4)
	   GRAPHICS_LIBRARY_EXTERN PFNGLBLENDFUNCSEPARATEPROC GLHANDLE(glBlendFuncSeparate);
#      define glBlendFuncSeparate (GLHANDLE(glBlendFuncSeparate))
#    endif /* defined (GL_VERSION_1_4) */

#    if defined (GL_VERSION_1_5)
	   GRAPHICS_LIBRARY_EXTERN PFNGLGENBUFFERSPROC GLHANDLE(glGenBuffers);
#      define glGenBuffers (GLHANDLE(glGenBuffers))
	   GRAPHICS_LIBRARY_EXTERN PFNGLDELETEBUFFERSPROC GLHANDLE(glDeleteBuffers);
#      define glDeleteBuffers (GLHANDLE(glDeleteBuffers))
	   GRAPHICS_LIBRARY_EXTERN PFNGLBINDBUFFERPROC GLHANDLE(glBindBuffer);
#      define glBindBuffer (GLHANDLE(glBindBuffer))
	   GRAPHICS_LIBRARY_EXTERN PFNGLBUFFERDATAPROC GLHANDLE(glBufferData);
#      define glBufferData (GLHANDLE(glBufferData))
	   GRAPHICS_LIBRARY_EXTERN PFNGLBUFFERSUBDATAPROC GLHANDLE(glBufferSubData);
#      define glBufferSubData (GLHANDLE(glBufferSubData))
#    endif /* defined (GL_VERSION_1_5) */

#    if defined (GL_VERSION_2_0)
	   GRAPHICS_LIBRARY_EXTERN PFNGLATTACHSHADERPROC GLHANDLE(glAttachShader);
#      define glAttachShader (GLHANDLE(glAttachShader))
	   GRAPHICS_LIBRARY_EXTERN PFNGLCOMPILESHADERPROC GLHANDLE(glCompileShader);
#      define glCompileShader (GLHANDLE(glCompileShader))
	   GRAPHICS_LIBRARY_EXTERN PFNGLCREATEPROGRAMPROC GLHANDLE(glCreateProgram);
#      define glCreateProgram (GLHANDLE(glCreateProgram))
	   GRAPHICS_LIBRARY_EXTERN PFNGLCREATESHADERPROC GLHANDLE(glCreateShader);
#      define glCreateShader (GLHANDLE(glCreateShader))
	   GRAPHICS_LIBRARY_EXTERN PFNGLDELETEPROGRAMPROC GLHANDLE(glDeleteProgram);
#      define glDeleteProgram (GLHANDLE(glDeleteProgram))
	   GRAPHICS_LIBRARY_EXTERN PFNGLDELETESHADERPROC GLHANDLE(glDeleteShader);
#      define glDeleteShader (GLHANDLE(glDeleteShader))
	   GRAPHICS_LIBRARY_EXTERN PFNGLDRAWBUFFERSPROC GLHANDLE(glDrawBuffers);
#      define glDrawBuffers (GLHANDLE(glDrawBuffers))
	   GRAPHICS_LIBRARY_EXTERN PFNGLGETPROGRAMIVPROC  GLHANDLE(glGetProgramiv);
#      define glGetProgramiv (GLHANDLE(glGetProgramiv))
	   GRAPHICS_LIBRARY_EXTERN PFNGLGETSHADERIVPROC  GLHANDLE(glGetShaderiv);
#      define glGetShaderiv (GLHANDLE(glGetShaderiv))
	   GRAPHICS_LIBRARY_EXTERN PFNGLGETSHADERINFOLOGPROC GLHANDLE(glGetShaderInfoLog);
#      define glGetShaderInfoLog (GLHANDLE(glGetShaderInfoLog))
	   GRAPHICS_LIBRARY_EXTERN PFNGLGETSHADERSOURCEPROC GLHANDLE(glGetShaderSource);
#      define glGetShaderSource (GLHANDLE(glGetShaderSource))
	   GRAPHICS_LIBRARY_EXTERN PFNGLGETUNIFORMLOCATIONPROC GLHANDLE(glGetUniformLocation);
#      define glGetUniformLocation (GLHANDLE(glGetUniformLocation))
	   GRAPHICS_LIBRARY_EXTERN PFNGLISPROGRAMPROC GLHANDLE(glIsProgram);
#      define glIsProgram (GLHANDLE(glIsProgram))
	   GRAPHICS_LIBRARY_EXTERN PFNGLLINKPROGRAMPROC GLHANDLE(glLinkProgram);
#      define glLinkProgram (GLHANDLE(glLinkProgram))
	   GRAPHICS_LIBRARY_EXTERN PFNGLSHADERSOURCEPROC GLHANDLE(glShaderSource);
#      define glShaderSource  (GLHANDLE(glShaderSource))
	   GRAPHICS_LIBRARY_EXTERN PFNGLUSEPROGRAMPROC GLHANDLE(glUseProgram);
#      define glUseProgram (GLHANDLE(glUseProgram))
	   GRAPHICS_LIBRARY_EXTERN PFNGLUNIFORM1IPROC GLHANDLE(glUniform1i);
#      define glUniform1i  (GLHANDLE(glUniform1i))
	   GRAPHICS_LIBRARY_EXTERN PFNGLUNIFORM1FPROC GLHANDLE(glUniform1f);
#      define glUniform1f  (GLHANDLE(glUniform1f))
	   GRAPHICS_LIBRARY_EXTERN PFNGLUNIFORM2FPROC GLHANDLE(glUniform2f);
#      define glUniform2f  (GLHANDLE(glUniform2f))
	   GRAPHICS_LIBRARY_EXTERN PFNGLUNIFORM3FPROC GLHANDLE(glUniform3f);
#      define glUniform3f  (GLHANDLE(glUniform3f))
	   GRAPHICS_LIBRARY_EXTERN PFNGLUNIFORM4FPROC GLHANDLE(glUniform4f);
#      define glUniform4f  (GLHANDLE(glUniform4f))
#    endif /* defined (GL_VERSION_2_0) */

#    if defined (GL_VERSION_3_0)
	   GRAPHICS_LIBRARY_EXTERN PFNGLGENERATEMIPMAPEXTPROC GLHANDLE(glGenerateMipmap);
#      define glGenerateMipmap (GLHANDLE(glGenerateMipmap))
#    endif /* defined (GL_VERSION_3_0) */

#    if defined (GL_ARB_vertex_program) || defined (GL_ARB_fragment_program)
	   GRAPHICS_LIBRARY_EXTERN PFNGLGENPROGRAMSARBPROC GLHANDLE(glGenProgramsARB);
#      define glGenProgramsARB (GLHANDLE(glGenProgramsARB))
	   GRAPHICS_LIBRARY_EXTERN PFNGLBINDPROGRAMARBPROC GLHANDLE(glBindProgramARB);
#      define glBindProgramARB (GLHANDLE(glBindProgramARB))
	   GRAPHICS_LIBRARY_EXTERN PFNGLPROGRAMSTRINGARBPROC GLHANDLE(glProgramStringARB);
#      define glProgramStringARB (GLHANDLE(glProgramStringARB))
	   GRAPHICS_LIBRARY_EXTERN PFNGLDELETEPROGRAMSARBPROC GLHANDLE(glDeleteProgramsARB);
#      define glDeleteProgramsARB (GLHANDLE(glDeleteProgramsARB))
	   GRAPHICS_LIBRARY_EXTERN PFNGLPROGRAMENVPARAMETER4FARBPROC GLHANDLE(glProgramEnvParameter4fARB);
#      define glProgramEnvParameter4fARB (GLHANDLE(glProgramEnvParameter4fARB))
	   GRAPHICS_LIBRARY_EXTERN PFNGLPROGRAMENVPARAMETER4FVARBPROC GLHANDLE(glProgramEnvParameter4fvARB);
#      define glProgramEnvParameter4fvARB (GLHANDLE(glProgramEnvParameter4fvARB))
#    endif /* defined (GL_ARB_vertex_program) || defined (GL_ARB_fragment_program) */

#    if defined (GL_EXT_framebuffer_object)
	  GRAPHICS_LIBRARY_EXTERN PFNGLGENFRAMEBUFFERSEXTPROC GLHANDLE(glGenFramebuffersEXT);
#      define glGenFramebuffersEXT (GLHANDLE(glGenFramebuffersEXT))
	   GRAPHICS_LIBRARY_EXTERN PFNGLBINDFRAMEBUFFEREXTPROC GLHANDLE(glBindFramebufferEXT);
#      define glBindFramebufferEXT (GLHANDLE(glBindFramebufferEXT))
	   GRAPHICS_LIBRARY_EXTERN PFNGLGENRENDERBUFFERSEXTPROC GLHANDLE(glGenRenderbuffersEXT);
#      define glGenRenderbuffersEXT (GLHANDLE(glGenRenderbuffersEXT))
	   GRAPHICS_LIBRARY_EXTERN PFNGLBINDRENDERBUFFEREXTPROC GLHANDLE(glBindRenderbufferEXT);
#      define glBindRenderbufferEXT (GLHANDLE(glBindRenderbufferEXT))
	   GRAPHICS_LIBRARY_EXTERN PFNGLRENDERBUFFERSTORAGEEXTPROC GLHANDLE(glRenderbufferStorageEXT);
#      define glRenderbufferStorageEXT (GLHANDLE(glRenderbufferStorageEXT))
	   GRAPHICS_LIBRARY_EXTERN PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC GLHANDLE(glFramebufferRenderbufferEXT);
#      define glFramebufferRenderbufferEXT (GLHANDLE(glFramebufferRenderbufferEXT))
	   GRAPHICS_LIBRARY_EXTERN PFNGLFRAMEBUFFERTEXTURE2DEXTPROC GLHANDLE(glFramebufferTexture2DEXT);
#      define glFramebufferTexture2DEXT (GLHANDLE(glFramebufferTexture2DEXT))
	   GRAPHICS_LIBRARY_EXTERN PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC GLHANDLE(glCheckFramebufferStatusEXT);
#      define glCheckFramebufferStatusEXT (GLHANDLE(glCheckFramebufferStatusEXT))
	   GRAPHICS_LIBRARY_EXTERN PFNGLDELETEFRAMEBUFFERSEXTPROC GLHANDLE(glDeleteFramebuffersEXT);
#      define glDeleteFramebuffersEXT (GLHANDLE(glDeleteFramebuffersEXT))
	   GRAPHICS_LIBRARY_EXTERN PFNGLDELETERENDERBUFFERSEXTPROC GLHANDLE(glDeleteRenderbuffersEXT);
#      define glDeleteRenderbuffersEXT (GLHANDLE(glDeleteRenderbuffersEXT))
#    endif /* GL_EXT_framebuffer_object */
#    if defined (GL_EXT_framebuffer_blit)
	   GRAPHICS_LIBRARY_EXTERN PFNGLBLITFRAMEBUFFEREXTPROC GLHANDLE(glBlitFramebufferEXT);
#      define glBlitFramebufferEXT (GLHANDLE(glBlitFramebufferEXT))
#    endif /* (GL_EXT_framebuffer_blit) */
#    if defined (GL_EXT_framebuffer_multisample)
	   GRAPHICS_LIBRARY_EXTERN PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC GLHANDLE(glRenderbufferStorageMultisampleEXT);
#      define glRenderbufferStorageMultisampleEXT (GLHANDLE(glRenderbufferStorageMultisampleEXT))
#    endif /* GL_EXT_framebuffer_multisample */

#  else  /* defined (APIENTRY) */
	 /* We don't have types for the functions so we will not use any extensions */
#    undef GL_VERSION_1_2
#    undef GL_VERSION_1_3
#    undef GL_VERSION_1_4
#    undef GL_VERSION_1_5
#    undef GL_VERSION_2_0
#    undef GL_EXT_texture3D
#    undef GL_ARB_vertex_program
#    undef GL_ARB_fragment_program
#    undef GL_EXT_framebuffer_blit
#  endif  /* defined (APIENTRY) */
#endif /* defined GRAPHICS_LIBRARY_USE_EXTENSION_FUNCTION_HANDLES && !defined (USE_GLEW)*/

#endif /* defined (OPENGL_API) */


#endif
