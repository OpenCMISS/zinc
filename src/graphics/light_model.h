/*******************************************************************************
FILE : light_model.h

LAST MODIFIED : 21 September 1998

DESCRIPTION :
The data structures used for representing light models.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (LIGHT_MODEL_H)
#define LIGHT_MODEL_H

#include "general/list.h"
#include "general/manager.h"
#include "general/object.h"
#include "graphics/colour.h"
#include "graphics/graphics_library.h"

/*
Global types
------------
*/
enum Light_model_viewer_mode
/*******************************************************************************
Local viewer lighting calculations take into account the angle from the
vertex to the viewer and are more accurate with some speed penalty.
Using an infinite viewer is the faster - and should be the default - option.
==============================================================================*/
{
	LIGHT_MODEL_LOCAL_VIEWER,
	LIGHT_MODEL_INFINITE_VIEWER
};

enum Light_model_side_mode
/*******************************************************************************
Two sided lighting lights the backs of the surfaces the same as the front;
One sided lighting does not.
==============================================================================*/
{
	LIGHT_MODEL_ONE_SIDED,
	LIGHT_MODEL_TWO_SIDED
};

struct Light_model;
/*******************************************************************************
LAST MODIFIED : 3 December 1997

DESCRIPTION :
The contents of this object are private.
==============================================================================*/

DECLARE_LIST_TYPES(Light_model);

DECLARE_MANAGER_TYPES(Light_model);

struct Modify_light_model_data
/*******************************************************************************
LAST MODIFIED : 17 February 1997

DESCRIPTION :
==============================================================================*/
{
	struct Light_model *default_light_model;
	struct MANAGER(Light_model) *light_model_manager;
}; /* struct Modify_light_model_data */

/*
Global functions
----------------
*/
struct Light_model *CREATE(Light_model)(const char *name);
/*******************************************************************************
LAST MODIFIED : 25 September 1995

DESCRIPTION :
Allocates memory and assigns fields for a light model.  Adds the light model to
the list of all light models.
==============================================================================*/

int DESTROY(Light_model)(struct Light_model **light_model_address);
/*******************************************************************************
LAST MODIFIED : 25 September 1995

DESCRIPTION :
Frees the memory for the light model and sets <*light_model_address> to NULL.
==============================================================================*/

//PROTOTYPE_OBJECT_FUNCTIONS(Light_model);
PROTOTYPE_ACCESS_OBJECT_FUNCTION(Light_model);
PROTOTYPE_DEACCESS_OBJECT_FUNCTION(Light_model);
PROTOTYPE_REACCESS_OBJECT_FUNCTION(Light_model);
PROTOTYPE_GET_OBJECT_NAME_FUNCTION(Light_model);

PROTOTYPE_LIST_FUNCTIONS(Light_model);
PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(Light_model,name,const char *);

PROTOTYPE_MANAGER_COPY_FUNCTIONS(Light_model,name,const char *);
PROTOTYPE_MANAGER_FUNCTIONS(Light_model);
PROTOTYPE_MANAGER_IDENTIFIER_FUNCTIONS(Light_model,name,const char *);

int Light_model_get_ambient(struct Light_model *light_model,
	struct Colour *ambient);
/*******************************************************************************
LAST MODIFIED : 4 December 1997

DESCRIPTION :
Returns the ambient colour of the light_model.
==============================================================================*/

int Light_model_set_ambient(struct Light_model *light_model,
	struct Colour *ambient);
/*******************************************************************************
LAST MODIFIED : 4 December 1997

DESCRIPTION :
Sets the ambient colour of the light_model.
==============================================================================*/

int Light_model_get_lighting_status(struct Light_model *light_model);
/*******************************************************************************
LAST MODIFIED : 13 December 1997

DESCRIPTION :
Returns true if lighting is on for <light_model>.
==============================================================================*/

int Light_model_set_lighting_status(struct Light_model *light_model,
	int lighting_on);
/*******************************************************************************
LAST MODIFIED : 13 December 1997

DESCRIPTION :
Sets whether lighting is on in <light_model>. On if <lighting_on> non-zero.
==============================================================================*/

int Light_model_get_side_mode(struct Light_model *light_model,
	enum Light_model_side_mode *side_mode);
/*******************************************************************************
LAST MODIFIED : 4 December 1997

DESCRIPTION :
Returns the side_mode of the light_model (local/infinite).
==============================================================================*/

int Light_model_set_side_mode(struct Light_model *light_model,
	enum Light_model_side_mode side_mode);
/*******************************************************************************
LAST MODIFIED : 4 December 1997

DESCRIPTION :
Sets the side_mode of the light_model (local/infinite).
==============================================================================*/

int Light_model_get_viewer_mode(struct Light_model *light_model,
	enum Light_model_viewer_mode *viewer_mode);
/*******************************************************************************
LAST MODIFIED : 4 December 1997

DESCRIPTION :
Returns the viewer_mode of the light_model (local/infinite).
==============================================================================*/

int Light_model_set_viewer_mode(struct Light_model *light_model,
	enum Light_model_viewer_mode viewer_mode);
/*******************************************************************************
LAST MODIFIED : 4 December 1997

DESCRIPTION :
Sets the viewer_mode of the light_model (local/infinite).
==============================================================================*/

int list_Light_model(struct Light_model *light_model,void *dummy);
/*****************************************************************************
LAST MODIFIED : 27 July 1995

DESCRIPTION :
Write the properties of the <light_model> to the command window.
============================================================================*/

int compile_Light_model(struct Light_model *light_model);
/*******************************************************************************
LAST MODIFIED : 3 December 1997

DESCRIPTION :
Rebuilds the display_list for <light_model> if it is not current. If
<light_model>does not have a display list, first attempts to give it one. The
display list created here may be called using execute_Light_model, below.
???RC Light_models must be compiled before they are executed since openGL
cannot start writing to a display list when one is currently being written to.
???RC The behaviour of light_models is set up to take advantage of pre-computed
display lists. To switch to direct rendering make this routine do nothing and
execute_Light_model should just call direct_render_Light_model.
==============================================================================*/

int execute_Light_model(struct Light_model *light_model);
/*******************************************************************************
LAST MODIFIED : 28 November 1997

DESCRIPTION :
Activates <light_model> by calling its display list. If the display list is not
current, an error is reported.
???RC The behaviour of light_models is set up to take advantage of pre-computed
display lists. To switch to direct rendering this routine should just call
direct_render_Light_model.
==============================================================================*/

int Light_model_set_status(struct Light_model *light_model,int enabled);

const char *Light_model_get_name(struct Light_model *light_model);

#if defined __cplusplus
class Render_graphics_opengl;

int Light_model_render_opengl(Light_model *light_model,
	Render_graphics_opengl *renderer);
#endif /* defined __cplusplus */


struct Light_model_module;

/**
 * Create and return a handle to a new light_model module.
 * Private; only to be called from graphics_module.
 *
 * @return  Handle to the newly created light_model module if successful,
 * otherwise NULL.
 */
Light_model_module *Light_model_module_create();

/**
* Returns a new reference to the light_model module with reference count
* incremented. Caller is responsible for destroying the new reference.
*
* @param light_model_module  The light_model module to obtain a new reference to.
* @return  Light_model module with incremented reference count.
*/
Light_model_module *Light_model_module_access(Light_model_module *light_model_module);

/**
* Destroys this reference to the light_model module (and sets it to NULL).
* Internally this just decrements the reference count.
*
* @param light_model_module_address  Address of handle to light_model module
*   to destroy.
* @return  Status CMZN_OK on success, otherwise CMZN_ERROR_ARGUMENT.
*/
int Light_model_module_destroy(
	Light_model_module **light_model_module_address);

/**
 * Create and return a handle to a new light_model.
 *
 * @param light_model_module  The handle to the light_model module the
 * light_model will belong to.
 * @return  Handle to the newly created light_model if successful, otherwise NULL.
 */
Light_model *Light_model_module_create_light_model(
	Light_model_module *light_model_module);

/**
* Begin caching or increment cache level for this light_model module. Call this
* function before making multiple changes to minimise number of change messages
* sent to clients. Must remember to end_change after completing changes.
* @see Light_model_module_end_change
*
* @param light_model_module  The light_model_module to begin change cache on.
* @return  Status CMZN_OK on success, otherwise CMZN_ERROR_ARGUMENT.
*/
int Light_model_module_begin_change(Light_model_module *light_model_module);

/***************************************************************************//**
* Decrement cache level or end caching of changes for the light_model module.
* Call Light_model_module_begin_change before making multiple changes
* and call this afterwards. When change level is restored to zero,
* cached change messages are sent out to clients.
*
* @param light_model_module  The light model module to end change cache on.
* @return  Status CMZN_OK on success, any other value on failure.
*/
int Light_model_module_end_change(Light_model_module *light_model_module);

/**
* Find the light_model with the specified name, if any.
*
* @param light_model_module  Light_model module to search.
* @param name  The name of the light_model.
* @return  Handle to the light_model of that name, or 0 if not found.
* 	Up to caller to destroy returned handle.
*/
Light_model *Light_model_module_find_light_model_by_name(
	Light_model_module *light_model_module, const char *name);

/**
 * Get the default light_model to be used by new lines, surfaces and
 * isosurfaces graphics. If there is none, one is automatically created with
 * minimum divisions 1, refinement factors 4, and circle divisions 12,
 * and given the name "default".
 *
 * @param light_model_module  Light_model module to query.
 * @return  Handle to the default light_model, or 0 on error.
 * Up to caller to destroy returned handle.
 */
Light_model * Light_model_module_get_default_light_model(
	Light_model_module *light_model_module);

/**
 * Set the default light_model to be used by new lines, surfaces and
 * isosurfaces graphics.
 *
 * @param light_model_module  Light_model module to modify.
 * @param light_model  The light_model to set as default.
 * @return  CMZN_OK on success otherwise CMZN_ERROR_ARGUMENT.
 */
int Light_model_module_set_default_light_model(
	Light_model_module *light_model_module,
	Light_model *light_model);

struct MANAGER(Light_model) *Light_model_module_get_manager(Light_model_module *light_model_module);


#endif
