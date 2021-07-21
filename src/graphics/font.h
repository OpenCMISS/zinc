/*******************************************************************************
FILE : font.h

LAST MODIFIED : 17 November 2005

DESCRIPTION :
This provides a Cmgui interface to the OpenGL contexts of many types.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (GRAPHICS_FONT_H)
#define GRAPHICS_FONT_H

#include "opencmiss/zinc/types/fontid.h"
#include "general/callback.h"
#include "general/manager.h"
#include "general/enumerator.h"

PROTOTYPE_ENUMERATOR_FUNCTIONS(cmzn_font_render_type);
PROTOTYPE_ENUMERATOR_FUNCTIONS(cmzn_font_typeface_type);

/*
Global types
------------
*/

struct cmzn_font;

DECLARE_LIST_TYPES(cmzn_font);

DECLARE_MANAGER_TYPES(cmzn_font);

struct cmzn_fontmodule;
#include "general/manager_private.h"
PROTOTYPE_MANAGER_GET_OWNER_FUNCTION(cmzn_font, struct cmzn_fontmodule);

/*
Global functions
----------------
*/

PROTOTYPE_OBJECT_FUNCTIONS(cmzn_font);

PROTOTYPE_GET_OBJECT_NAME_FUNCTION(cmzn_font);

PROTOTYPE_LIST_FUNCTIONS(cmzn_font);
PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(cmzn_font,name,const char *);

PROTOTYPE_MANAGER_COPY_FUNCTIONS(cmzn_font,name,const char *);
PROTOTYPE_MANAGER_FUNCTIONS(cmzn_font);
PROTOTYPE_MANAGER_IDENTIFIER_FUNCTIONS(cmzn_font,name,const char *);

/**
 * Create and return a handle to a new font module.
 * Private; only to be called from graphics_module.
 *
 * @return  Handle to the newly created font module if successful,
 * otherwise NULL.
 */
cmzn_fontmodule_id cmzn_fontmodule_create();

struct MANAGER(cmzn_font) *cmzn_fontmodule_get_manager(
	cmzn_fontmodule_id fontmodule);

int DESTROY(cmzn_font)(struct cmzn_font **font_address);
/*******************************************************************************
LAST MODIFIED : 17 November 2005

DESCRIPTION :
==============================================================================*/


int cmzn_font_compile(struct cmzn_font *font);
/*******************************************************************************
LAST MODIFIED : 17 November 2005

DESCRIPTION :
Compiles the specified <font> so it can be used by the graphics.  The
<buffer> is required as the Win32 API requires a window context.
==============================================================================*/

int cmzn_font_rendergl_text(struct cmzn_font *font, char *text,
	float x, float y, float z);
/*******************************************************************************
LAST MODIFIED : 17 November 2005

DESCRIPTION :
==============================================================================*/

int cmzn_font_manager_set_owner(struct MANAGER(cmzn_font) *manager,
	struct cmzn_graphics_module *graphics_module);
#endif /* !defined (GRAPHICS_FONT_H) */

