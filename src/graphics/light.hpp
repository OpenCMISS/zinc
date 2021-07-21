/*******************************************************************************
FILE : light.h

LAST MODIFIED : 8 October 2002

DESCRIPTION :
The data structures used for representing graphical lights.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (LIGHT_HPP)
#define LIGHT_HPP

#include "general/enumerator.h"
#include "general/list.h"
#include "general/manager.h"
#include "general/object.h"
#include "graphics/colour.h"
#include "graphics/graphics_library.h"
#include "opencmiss/zinc/light.h"

/*
Global functions
----------------
*/

DECLARE_LIST_TYPES(cmzn_light);
DECLARE_MANAGER_TYPES(cmzn_light);

PROTOTYPE_OBJECT_FUNCTIONS(cmzn_light);
PROTOTYPE_GET_OBJECT_NAME_FUNCTION(cmzn_light);

PROTOTYPE_LIST_FUNCTIONS(cmzn_light);
PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(cmzn_light,name,const char *);
PROTOTYPE_CREATE_LIST_ITERATOR_FUNCTION(cmzn_light,cmzn_lightiterator);

PROTOTYPE_MANAGER_FUNCTIONS(cmzn_light);
PROTOTYPE_MANAGER_IDENTIFIER_WITHOUT_MODIFY_FUNCTIONS(cmzn_light,name,const char *);

PROTOTYPE_ENUMERATOR_FUNCTIONS(cmzn_light_type);

/**
 * Directly outputs the OpenGL commands to activate the <light> with the
 * given light_id.
 * @param light  The light to output.
 * @param int_light_id  Enum of OpenGL light to set: GL_LIGHT0..GL_LIGHT7, or
 * GL_INVALID_ENUM if all lights used up, which doesn't affect ambient.
 * @return  1 if a glLight created, -1 if light was ambient, 0 if failed.
 */
int direct_render_cmzn_light(cmzn_light *light, unsigned int int_light_id);

int cmzn_light_manager_set_owner_private(struct MANAGER(cmzn_light) *manager,
	struct cmzn_lightmodule *lightmodule);

struct cmzn_light_change_detail
{
	virtual ~cmzn_light_change_detail()
	{
	}
};

int list_cmzn_light(struct cmzn_light *light,void *dummy);
/*******************************************************************************
LAST MODIFIED : 21 September 1998

DESCRIPTION :
Writes the properties of the <light> to the command window.
==============================================================================*/

int list_cmzn_light_name(struct cmzn_light *light,void *preceding_text_void);
/*******************************************************************************
LAST MODIFIED : 14 October 1998

DESCRIPTION :
Writes the name of the <light> to the command window, preceded on each line by
the optional <preceding_text> string.
==============================================================================*/

int list_cmzn_light_name_command(struct cmzn_light *light,void *preceding_text_void);
/*******************************************************************************
LAST MODIFIED : 22 January 2002

DESCRIPTION :
Writes the name of the <light> to the command window, preceded on each line by
the optional <preceding_text> string. Makes sure quotes are put around the
name of the light if it contains any special characters.
Follows the light name with semicolon and carriage return.
==============================================================================*/

/**
 * Returns true if <light> is in <light_list>.
 */
int cmzn_light_is_in_list(struct cmzn_light *light, void *light_list_void);

struct cmzn_lightmodule;

/**
 * Create and return a handle to a new light module.
 * Private; only to be called from graphics_module.
 *
 * @return  Handle to the newly created light module if successful,
 * otherwise NULL.
 */
cmzn_lightmodule *cmzn_lightmodule_create();

struct MANAGER(cmzn_light) *cmzn_lightmodule_get_manager(cmzn_lightmodule *lightmodule);

/* forward declaration */
struct cmzn_light *cmzn_light_create_private();

/**
 * Internal variant of public cmzn_lightiterator_next() which does not access
 * the returned light, for more efficient if less safe usage.
 *
 * @param iterator  Light iterator to query and advance.
 * @return  Non-accessed pointer to the next light, or NULL if none remaining.
 */
cmzn_light_id cmzn_lightiterator_next_non_access(
	cmzn_lightiterator_id iterator);

/**
 * Get the total ambient colour as the sum of ambient lights' colours in the list.
 */
Colour Light_list_get_total_ambient_colour(struct LIST(cmzn_light) *light_list);

#endif
