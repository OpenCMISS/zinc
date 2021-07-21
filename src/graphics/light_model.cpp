/*******************************************************************************
FILE : light_model.c

LAST MODIFIED : 21 January 2002

DESCRIPTION :
The functions for manipulating light models.

???RC Only OpenGL is supported now.

???RC 3 December 97.
		Two functions are now used for activating the light_model:
- compile_Light_model puts the material into its own display list.
- execute_Light_model calls that display list.
		The separation into two functions was needed because OpenGL cannot start a
new list while one is being written to. As a consequence, you must precompile
all objects that are executed from within another display list. To make this job
easier, compile_Light_model is a list/manager iterator function.
		Future updates of OpenGL may overcome this limitation, in which case the
execute function can take over compiling as well. Furthermore, it is easy to
return to direct rendering, as described with these routines.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "opencmiss/zinc/status.h"
#include "opencmiss/zinc/zincconfigure.h"

#include "general/debug.h"
#include "general/list_private.h"
#include "general/manager_private.h"
#include "general/mystring.h"
#include "general/object.h"
#include "graphics/colour.h"
#include "graphics/graphics_library.h"
#include "graphics/light_model.h"
#include "general/message.h"
#include "graphics/render_gl.h"

/*
Module types
------------
*/

struct Light_model_module
{

private:

	struct MANAGER(Light_model) *lightModelManager;
	Light_model *defaultLightModel;
	int access_count;

	Light_model_module() :
		lightModelManager(CREATE(MANAGER(Light_model))()),
		defaultLightModel(0),
		access_count(1)
	{
	}

	~Light_model_module()
	{
		DEACCESS(Light_model)(&this->defaultLightModel);
		DESTROY(MANAGER(Light_model))(&(this->lightModelManager));
	}

public:

	static Light_model_module *create()
	{
		return new Light_model_module();
	}

	Light_model_module *access()

	{
		++access_count;
		return this;
	}

	static int deaccess(Light_model_module* &light_module)
	{
		if (light_module)
		{
			--(light_module->access_count);
			if (light_module->access_count <= 0)
			{
				delete light_module;
			}
			light_module = 0;
			return CMZN_OK;
		}
		return CMZN_ERROR_ARGUMENT;
	}

	struct MANAGER(Light_model) *getManager()
	{
		return this->lightModelManager;
	}

	int beginChange()
	{
		return MANAGER_BEGIN_CACHE(Light_model)(this->lightModelManager);
	}

	int endChange()
	{
		return MANAGER_END_CACHE(Light_model)(this->lightModelManager);
	}

	Light_model *createLightModel()
	{
		Light_model *lightModel = NULL;
		char temp_name[20];
		int i = NUMBER_IN_MANAGER(Light_model)(this->lightModelManager);
		do
		{
			i++;
			sprintf(temp_name, "temp%d",i);
		}
		while (FIND_BY_IDENTIFIER_IN_MANAGER(Light_model,name)(temp_name,
			this->lightModelManager));
		lightModel = CREATE(Light_model)(temp_name);
		if (!ADD_OBJECT_TO_MANAGER(Light_model)(lightModel, this->lightModelManager))
		{
			DEACCESS(Light_model)(&lightModel);
		}
		return lightModel;
	}

	Light_model *findLightModelByName(const char *name)
	{
		Light_model *lightModel = FIND_BY_IDENTIFIER_IN_MANAGER(Light_model,name)(name,
			this->lightModelManager);
		if (lightModel)
		{
			return ACCESS(Light_model)(lightModel);
		}
		return 0;
	}

	Light_model *getDefaultLightModel()
	{
		if (this->defaultLightModel)
		{
			ACCESS(Light_model)(this->defaultLightModel);
		}
		else
		{
			this->beginChange();
			Light_model *lightModel = CREATE(Light_model)("default");
			if (lightModel)
			{
				struct Colour ambient_colour;
				ambient_colour.red=0.1f;
				ambient_colour.green=0.1f;
				ambient_colour.blue=0.1f;
				Light_model_set_ambient(lightModel,&ambient_colour);
				Light_model_set_side_mode(lightModel,
					LIGHT_MODEL_TWO_SIDED);
				if (!ADD_OBJECT_TO_MANAGER(Light_model)(lightModel, this->lightModelManager))
				{
					DEACCESS(Light_model)(&lightModel);
				}
			}
			this->setDefaultLightModel(lightModel);
			this->endChange();
		}
		return this->defaultLightModel;
	}

	int setDefaultLightModel(Light_model *lightModel)
	{
		REACCESS(Light_model)(&this->defaultLightModel, lightModel);
		return CMZN_OK;
	}

};

struct Light_model
/*******************************************************************************
LAST MODIFIED : 13 December 1997

DESCRIPTION :
The properties of a light model.
==============================================================================*/
{
	/* the name of the light model */
	char *name;
	/* flag for whether lighting is on */
	int enabled;
	/* ambient light in the scene not associated with any light */
	struct Colour ambient;
	/* For calculating specular lighting, a Local viewer take into account the */
	/* angle from the vertex to the viewer and is more accurate but slower than */
	/* an infinite viewer. */
	enum Light_model_viewer_mode viewer_mode;
	/* Two sided lighting lights the backs of the surfaces the same as the */
	/* front. One sided lighting does not. */
	enum Light_model_side_mode side_mode;
	GLuint display_list;
	int display_list_current;

	/* after clearing in create, following to be modified only by manager */
	struct MANAGER(Light_model) *manager;
	int manager_change_status;

	/* the number of structures that point to this light model. The light model
		cannot be destroyed while this is greater than 0 */
	int access_count;
}; /* struct Light_model */

FULL_DECLARE_LIST_TYPE(Light_model);

FULL_DECLARE_MANAGER_TYPE(Light_model);

/*
Module variables
----------------
*/

/*
Module functions
----------------
*/
DECLARE_LOCAL_MANAGER_FUNCTIONS(Light_model)

int Light_model_render_opengl(Light_model *light_model,
	Render_graphics_opengl *renderer)
/*******************************************************************************
LAST MODIFIED : 13 December 1997

DESCRIPTION :
Activates the current light model as part of the rendering loop.
==============================================================================*/
{
	int return_code;
	GLfloat values[4];

	ENTER(direct_render_Light_model);
	USE_PARAMETER(renderer);
	if (light_model)
	{
		if (light_model->enabled)
		{
			values[0]=(light_model->ambient).red;
			values[1]=(light_model->ambient).green;
			values[2]=(light_model->ambient).blue;
			values[3]=1.;
			glLightModelfv(GL_LIGHT_MODEL_AMBIENT,values);
			if (LIGHT_MODEL_LOCAL_VIEWER==light_model->viewer_mode)
			{
				glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,GL_TRUE);
			}
			else
			{
				glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,GL_FALSE);
			}
			if (LIGHT_MODEL_TWO_SIDED==light_model->side_mode)
			{
				glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);
			}
			else
			{
				glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_FALSE);
			}
			glEnable(GL_LIGHTING);
		}
		else
		{
			glDisable(GL_LIGHTING);
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"direct_render_Light_model.  Missing light model");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* direct_render_Light_model */

/*
Global functions
----------------
*/
struct Light_model *CREATE(Light_model)(const char *name)
/*******************************************************************************
LAST MODIFIED : 5 December 1997

DESCRIPTION :
Allocates memory and assigns fields for a light model.  Adds the light model to
the list of all light models.
==============================================================================*/
{
	struct Light_model *light_model;

	ENTER(CREATE(Light_model));
	if (name)
	{
		/* allocate memory for structure */
		if (ALLOCATE(light_model,struct Light_model,1)&&
			ALLOCATE(light_model->name,char,strlen(name)+1))
		{
			strcpy(light_model->name,name);
			light_model->access_count=1;
			light_model->manager = (struct MANAGER(Light_model) *)NULL;
			light_model->manager_change_status = MANAGER_CHANGE_NONE(Light_model);
			light_model->enabled=1;
			(light_model->ambient).red=0.;
			(light_model->ambient).green=0.;
			(light_model->ambient).blue=0.;
			light_model->side_mode=LIGHT_MODEL_TWO_SIDED;
			light_model->viewer_mode=LIGHT_MODEL_INFINITE_VIEWER;
			light_model->display_list=0;
			light_model->display_list_current=0;
		}
		else
		{
			if (light_model)
			{
				DEALLOCATE(light_model);
			}
			display_message(ERROR_MESSAGE,
				"CREATE(Light_model).  Not enough memory");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CREATE(Light_model).  Missing name");
		light_model=(struct Light_model *)NULL;
	}
	LEAVE;

	return (light_model);
} /* CREATE(Light_model) */

int DESTROY(Light_model)(struct Light_model **light_model_address)
/*******************************************************************************
LAST MODIFIED : 5 December 1997

DESCRIPTION :
Frees the memory for the light model and sets <*light_model_address> to NULL.
==============================================================================*/
{
	int return_code;
	struct Light_model *light_model;

	ENTER(DESTROY(Light_model));
	if (light_model_address&&(light_model= *light_model_address))
	{
		if (0==light_model->access_count)
		{
			DEALLOCATE(light_model->name);
			if (light_model->display_list)
			{
				glDeleteLists(light_model->display_list,1);
			}
			DEALLOCATE(*light_model_address);
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"DESTROY(Light_model).  Non-zero access count!");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"DESTROY(Light_model).  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Light_model) */

DECLARE_OBJECT_FUNCTIONS(Light_model)
DECLARE_DEFAULT_GET_OBJECT_NAME_FUNCTION(Light_model)

DECLARE_LIST_FUNCTIONS(Light_model)
DECLARE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(Light_model,name,const char *,strcmp)
DECLARE_LIST_IDENTIFIER_CHANGE_FUNCTIONS(Light_model,name)

PROTOTYPE_MANAGER_COPY_WITH_IDENTIFIER_FUNCTION(Light_model,name)
{
	char *name;
	int return_code;

	ENTER(MANAGER_COPY_WITH_IDENTIFIER(Light_model,name));
	/* check arguments */
	if (source&&destination)
	{
		if (source->name)
		{
			if (ALLOCATE(name,char,strlen(source->name)+1))
			{
				strcpy(name,source->name);
				return_code=1;
			}
			else
			{
				display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITH_IDENTIFIER(Light_model,name).  Insufficient memory");
				return_code=0;
			}
		}
		else
		{
			name=(char *)NULL;
			return_code=1;
		}
		if (return_code)
		{
			return_code = MANAGER_COPY_WITHOUT_IDENTIFIER(Light_model,name)(
				destination,source);
			if (return_code)
			{
				/* copy values */
				DEALLOCATE(destination->name);
				destination->name=name;
			}
			else
			{
				DEALLOCATE(name);
				display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITH_IDENTIFIER(Light_model,name).  Could not copy without identifier");
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITH_IDENTIFIER(Light_model,name).  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* MANAGER_COPY_WITH_IDENTIFIER(Light_model,name) */

PROTOTYPE_MANAGER_COPY_WITHOUT_IDENTIFIER_FUNCTION(Light_model,name)
{
	int return_code;

	ENTER(MANAGER_COPY_WITHOUT_IDENTIFIER(Light_model,name));
	/* check arguments */
	if (source&&destination)
	{
		/* copy values */
		(destination->ambient).red=(source->ambient).red;
		(destination->ambient).green=(source->ambient).green;
		(destination->ambient).blue=(source->ambient).blue;
		destination->enabled=source->enabled;
		destination->side_mode=source->side_mode;
		destination->viewer_mode=source->viewer_mode;
		destination->display_list_current=0;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITHOUT_IDENTIFIER(Light_model,name).  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* MANAGER_COPY_WITHOUT_IDENTIFIER(Light_model,name) */

PROTOTYPE_MANAGER_COPY_IDENTIFIER_FUNCTION(Light_model,name,const char *)
{
	char *destination_name = NULL;
	int return_code = 0;

	ENTER(MANAGER_COPY_IDENTIFIER(Light_model,name));
	/* check arguments */
	if (name&&destination)
	{
		if (name)
		{
			if (ALLOCATE(destination_name,char,strlen(name)+1))
			{
				strcpy(destination_name,name);
				return_code=1;
			}
			else
			{
				display_message(ERROR_MESSAGE,
			"MANAGER_COPY_IDENTIFIER(Light_model,name).  Insufficient memory");
				return_code=0;
			}
		}
		else
		{
			name=(char *)NULL;
			return_code=1;
		}
		if (return_code)
		{
			/* copy name */
			DEALLOCATE(destination->name);
			destination->name=destination_name;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"MANAGER_COPY_IDENTIFIER(Light_model,name).  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* MANAGER_COPY_IDENTIFIER(Light_model,name) */

DECLARE_MANAGER_FUNCTIONS(Light_model,manager)

DECLARE_DEFAULT_MANAGED_OBJECT_NOT_IN_USE_FUNCTION(Light_model,manager)

DECLARE_MANAGER_IDENTIFIER_FUNCTIONS(Light_model,name,const char *,manager)

int Light_model_get_ambient(struct Light_model *light_model,
	struct Colour *ambient)
/*******************************************************************************
LAST MODIFIED : 4 December 1997

DESCRIPTION :
Returns the ambient colour of the light_model.
==============================================================================*/
{
	int return_code;

	ENTER(Light_model_get_ambient);
	if (light_model&&ambient)
	{
		ambient->red=light_model->ambient.red;
		ambient->green=light_model->ambient.green;
		ambient->blue=light_model->ambient.blue;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Light_model_get_ambient.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Light_model_get_ambient */

int Light_model_set_ambient(struct Light_model *light_model,
	struct Colour *ambient)
/*******************************************************************************
LAST MODIFIED : 13 December 1997

DESCRIPTION :
Sets the ambient colour of the light_model.
==============================================================================*/
{
	int return_code;

	ENTER(Light_model_set_ambient);
	if (light_model&&ambient)
	{
		if ((ambient->red != light_model->ambient.red)||
			(ambient->green != light_model->ambient.green)||
			(ambient->blue != light_model->ambient.blue))
		{
			light_model->ambient.red=ambient->red;
			light_model->ambient.green=ambient->green;
			light_model->ambient.blue=ambient->blue;
			/* display list needs to be compiled again */
			light_model->display_list_current=0;
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Light_model_set_ambient.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Light_model_set_ambient */

int Light_model_get_status(struct Light_model *light_model)
/*******************************************************************************
LAST MODIFIED : 13 December 1997

DESCRIPTION :
Returns true if lighting is on for <light_model>.
==============================================================================*/
{
	int return_code;

	ENTER(Light_model_get_status);
	if (light_model)
	{
		return_code=light_model->enabled;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Light_model_get_status.  Missing light model");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Light_model_get_status */

int Light_model_set_status(struct Light_model *light_model,int enabled)
/*******************************************************************************
LAST MODIFIED : 13 December 1997

DESCRIPTION :
Sets whether lighting is on in <light_model>. On if <enabled> non-zero.
==============================================================================*/
{
	int return_code;

	ENTER(Light_model_set_status);
	if (light_model)
	{
		if (enabled)
		{
			enabled=1;
		}
		if (enabled != light_model->enabled)
		{
			light_model->enabled=enabled;
			/* display list needs to be compiled again */
			light_model->display_list_current=0;
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Light_model_set_status.  Missing light model");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Light_model_set_status */

const char *Light_model_get_name(struct Light_model *light_model)
{
	return light_model->name;
}

int Light_model_get_side_mode(struct Light_model *light_model,
	enum Light_model_side_mode *side_mode)
/*******************************************************************************
LAST MODIFIED : 4 December 1997

DESCRIPTION :
Returns the side_mode of the light_model (local/infinite).
==============================================================================*/
{
	int return_code;

	ENTER(Light_model_get_side_mode);
	if (light_model&&side_mode)
	{
		*side_mode=light_model->side_mode;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Light_model_get_side_mode.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Light_model_get_side_mode */

int Light_model_set_side_mode(struct Light_model *light_model,
	enum Light_model_side_mode side_mode)
/*******************************************************************************
LAST MODIFIED : 13 December 1997

DESCRIPTION :
Sets the side_mode of the light_model (local/infinite).
==============================================================================*/
{
	int return_code;

	ENTER(Light_model_set_side_mode);
	if (light_model&&((LIGHT_MODEL_ONE_SIDED==side_mode)||
		(LIGHT_MODEL_TWO_SIDED==side_mode)))
	{
		if (side_mode != light_model->side_mode)
		{
			light_model->side_mode=side_mode;
			/* display list needs to be compiled again */
			light_model->display_list_current=0;
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Light_model_set_side_mode.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Light_model_set_side_mode */

int Light_model_get_viewer_mode(struct Light_model *light_model,
	enum Light_model_viewer_mode *viewer_mode)
/*******************************************************************************
LAST MODIFIED : 4 December 1997

DESCRIPTION :
Returns the viewer_mode of the light_model (local/infinite).
==============================================================================*/
{
	int return_code;

	ENTER(Light_model_get_viewer_mode);
	if (light_model&&viewer_mode)
	{
		*viewer_mode=light_model->viewer_mode;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Light_model_get_viewer_mode.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Light_model_get_viewer_mode */

int Light_model_set_viewer_mode(struct Light_model *light_model,
	enum Light_model_viewer_mode viewer_mode)
/*******************************************************************************
LAST MODIFIED : 13 December 1997

DESCRIPTION :
Sets the viewer_mode of the light_model (local/infinite).
==============================================================================*/
{
	int return_code;

	ENTER(Light_model_set_viewer_mode);
	if (light_model&&((LIGHT_MODEL_LOCAL_VIEWER==viewer_mode)||
		(LIGHT_MODEL_INFINITE_VIEWER==viewer_mode)))
	{
		if (viewer_mode != light_model->viewer_mode)
		{
			light_model->viewer_mode=viewer_mode;
			/* display list needs to be compiled again */
			light_model->display_list_current=0;
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Light_model_set_viewer_mode.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Light_model_set_viewer_mode */

int list_Light_model(struct Light_model *light_model,void *dummy)
/*******************************************************************************
LAST MODIFIED : 22 December 2000

DESCRIPTION :
Writes the properties of the <light_model> to the command window.
==============================================================================*/
{
	char line[80];
	int return_code;

	ENTER(list_Light_model);
	USE_PARAMETER(dummy);
	if (light_model)
	{
		display_message(INFORMATION_MESSAGE,"light model : ");
		display_message(INFORMATION_MESSAGE,light_model->name);
		display_message(INFORMATION_MESSAGE,"\n");
		if (light_model->enabled)
		{
			display_message(INFORMATION_MESSAGE,"  lighting enabled\n");
			if (LIGHT_MODEL_LOCAL_VIEWER==light_model->viewer_mode)
			{
				display_message(INFORMATION_MESSAGE,"  local view point\n");
			}
			else
			{
				display_message(INFORMATION_MESSAGE,"  infinite view point\n");
			}
			if (LIGHT_MODEL_TWO_SIDED==light_model->side_mode)
			{
				display_message(INFORMATION_MESSAGE,"  two-sided\n");
			}
			else
			{
				display_message(INFORMATION_MESSAGE,"  one-sided\n");
			}
			sprintf(line,"  ambient.  red = %.3g, green = %.3g, blue = %.3g\n",
				(light_model->ambient).red,(light_model->ambient).green,
				(light_model->ambient).blue);
			display_message(INFORMATION_MESSAGE,line);
		}
		else
		{
			display_message(INFORMATION_MESSAGE,"  lighting disabled\n");
		}
#if defined (DEBUG_CODE)
		display_message(INFORMATION_MESSAGE,"  access count = %d\n",
			light_model->access_count);
#endif /* defined (DEBUG_CODE) */
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"list_Light_model.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* list_Light_model */

int compile_Light_model(struct Light_model *light_model)
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
{
	int return_code;

	ENTER(compile_Light_model);
	if (light_model)
	{
		if (!light_model->display_list_current)
		{
			if (light_model->display_list||(light_model->display_list=glGenLists(1)))
			{
				glNewList(light_model->display_list,GL_COMPILE);
				//renderer->Light_model_execute(light_model);
				glEndList();
				light_model->display_list_current=1;
				return_code=1;
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"compile_Light_model.  Could not generate display list");
				return_code=0;
			}
		}
		else
		{
			return_code=1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"compile_Light_model.  Missing light_model");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* compile_Light_model */

int execute_Light_model(struct Light_model *light_model)
/*******************************************************************************
LAST MODIFIED : 28 November 1997

DESCRIPTION :
Activates <light_model> by calling its display list. If the display list is not
current, an error is reported.
???RC The behaviour of light_models is set up to take advantage of pre-computed
display lists. To switch to direct rendering this routine should just call
direct_render_Light_model.
==============================================================================*/
{
	int return_code;

	ENTER(execute_Light_model);
	if (light_model)
	{
		if (light_model->display_list_current)
		{
			glCallList(light_model->display_list);
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"execute_Light_model.  display list not current");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"execute_Light_model.  Missing light_model");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* execute_Light_model */

struct MANAGER(Light_model) *Light_model_module_get_manager(Light_model_module *light_model_module)
{
	if (light_model_module)
		return light_model_module->getManager();
	return 0;
}

Light_model_module *Light_model_module_create()
{
	return Light_model_module::create();
}

Light_model_module *Light_model_module_access(
	Light_model_module *light_model_module)
{
	if (light_model_module)
		return light_model_module->access();
	return 0;
}

int Light_model_module_destroy(Light_model_module **light_model_module_address)
{
	if (light_model_module_address)
		return Light_model_module::deaccess(*light_model_module_address);
	return CMZN_ERROR_ARGUMENT;
}

Light_model *Light_model_module_create_light_model(
	Light_model_module *light_model_module)
{
	if (light_model_module)
		return light_model_module->createLightModel();
	return 0;
}

int Light_model_module_begin_change(Light_model_module *light_model_module)
{
	if (light_model_module)
		return light_model_module->beginChange();
   return CMZN_ERROR_ARGUMENT;
}

int Light_model_module_end_change(Light_model_module *light_model_module)
{
	if (light_model_module)
		return light_model_module->endChange();
   return CMZN_ERROR_ARGUMENT;
}

Light_model *Light_model_module_find_light_model_by_name(
	Light_model_module *light_model_module, const char *name)
{
	if (light_model_module)
		return light_model_module->findLightModelByName(name);
   return 0;
}

Light_model *Light_model_module_get_default_light_model(Light_model_module *light_model_module)
{
	if (light_model_module)
		return light_model_module->getDefaultLightModel();
	return 0;
}

int Light_model_module_set_default_light_model(Light_model_module *light_model_module, Light_model *light_model)
{
	if (light_model_module)
		return light_model_module->setDefaultLightModel(light_model);
	return 0;
}
