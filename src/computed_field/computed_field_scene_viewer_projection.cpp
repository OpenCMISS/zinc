/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <math.h>

#include "opencmiss/zinc/fieldmodule.h"
#include "opencmiss/zinc/fieldsceneviewerprojection.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_private.hpp"
#include "computed_field/computed_field_image.h"
#include "computed_field/computed_field_set.h"
#include "computed_field/field_module.hpp"
#include "general/debug.h"
#include "general/matrix_vector.h"
#include "general/mystring.h"
#include "graphics/scene.hpp"
#include "graphics/scene_viewer.h"
#include "general/message.h"
#include "computed_field/computed_field_scene_viewer_projection.h"
#include "graphics/scene_coordinate_system.hpp"

namespace {

const char computed_field_scene_viewer_projection_type_string[] = "window_projection";

void Computed_field_scene_viewer_projection_scene_viewer_callback(
	cmzn_sceneviewerevent_id event, void *field_void);

void Computed_field_scene_projection_transformation_callback(
	cmzn_scene_id scene, void *dummy_void, void *field_void);

void Computed_field_scene_viewer_top_scene_change_callback(
	cmzn_scene_id scene, cmzn_scene_id top_scene, void *field_void);

class Computed_field_scene_viewer_projection : public Computed_field_core
{
public:
	/* Hold onto the graphics window name so we can write out the command. */
	char *graphics_window_name;
	int pane_number;

	/* The scene_viewer is not accessed by the computed field so that we
		can still destroy the window and so the computed field responds to
		a destroy callback and then must evaluate correctly with a NULL scene_viewer */
	struct Scene_viewer *scene_viewer;
	gtMatrix *current_local_transformation;
	enum cmzn_scenecoordinatesystem from_coordinate_system;
	enum cmzn_scenecoordinatesystem to_coordinate_system;
	int change_required;
	cmzn_scene_id current_scene;
	cmzn_sceneviewernotifier_id sceneviewernotifier;
	int transformation_callback_flag;

	Computed_field_scene_viewer_projection(
		Scene_viewer *scene_viewer,
		enum cmzn_scenecoordinatesystem from_coordinate_system,
		enum cmzn_scenecoordinatesystem to_coordinate_system) :
		Computed_field_core(),
		graphics_window_name(NULL),
		pane_number(-1), scene_viewer(scene_viewer),
		current_local_transformation(NULL),
		from_coordinate_system(from_coordinate_system),
		to_coordinate_system(to_coordinate_system),
		change_required(1),
		current_scene(NULL),
		sceneviewernotifier(0)
	{
		transformation_callback_flag = 0;
		projection_matrix = (double*)NULL;
	}

	virtual bool attach_to_field(Computed_field *parent)
	{
		if (Computed_field_core::attach_to_field(parent))
		{
			return true;
		}
		return false;
	}

	~Computed_field_scene_viewer_projection();

	void set_graphics_window_name(const char *name);

	void set_pane_number(int number);

	int requiredProjectionMatrixUpdate();

	void update_current_scene();

	void remove_transformation_callback();

private:

	double *projection_matrix;

	Computed_field_core *copy()
	{
		Computed_field_scene_viewer_projection *projection_field = new Computed_field_scene_viewer_projection(
			scene_viewer, from_coordinate_system, to_coordinate_system);
		projection_field->set_graphics_window_name(graphics_window_name);
		projection_field->set_pane_number(pane_number);
		return projection_field;
	}

	const char *get_type_string()
	{
		return(computed_field_scene_viewer_projection_type_string);
	}

	int compare(Computed_field_core* other_field);

	virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
	{
		return this->evaluateDerivativeFiniteDifference(cache, inValueCache, fieldDerivative);
	}

	int list();

	char* get_command_string();

	int calculate_matrix();

	void add_transformation_callback();

};

int Computed_field_scene_viewer_projection::calculate_matrix()
{
	int return_code = 0;
	change_required = 0;
	if (field)
	{
		if (projection_matrix || ALLOCATE(projection_matrix, double, 16))
		{
			return_code = 1;
			/* make sure we are getting scene viewer callbacks once the
				 projection matrix exists */
			if (!sceneviewernotifier)
			{
				sceneviewernotifier = cmzn_sceneviewer_create_sceneviewernotifier(scene_viewer);
				cmzn_sceneviewernotifier_set_callback(sceneviewernotifier,
					Computed_field_scene_viewer_projection_scene_viewer_callback, (void *)field);

			}
			if ((from_coordinate_system == CMZN_SCENECOORDINATESYSTEM_LOCAL) ||
				(to_coordinate_system == CMZN_SCENECOORDINATESYSTEM_LOCAL))
			{
				add_transformation_callback();
			}
			int result = this->scene_viewer->getTransformationMatrix(
				this->from_coordinate_system, this->to_coordinate_system, this->current_local_transformation,
				projection_matrix);
			if (CMZN_OK != result)
				return_code = 0;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Computed_field_scene_viewer_projection::calculate_matrix.  "
				"Not enough memory for matrix");
			return_code = 0;
		}
		if (!return_code)
		{
			/* Only keep projection_matrix if valid */
			if (projection_matrix)
			{
				DEALLOCATE(projection_matrix);
				projection_matrix = (double *)NULL;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_scene_viewer_projection::calculate_matrix.  "
			"Invalid arguments.");
		return_code = 0;
	}
	return (return_code);
}

Computed_field_scene_viewer_projection::~Computed_field_scene_viewer_projection()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Clear the type specific data used by this type.
==============================================================================*/
{

	ENTER(Computed_field_scene_viewer_projection::~Computed_field_scene_viewer_projection);
	if (field)
	{
		if (current_local_transformation)
		{
			DEALLOCATE(current_local_transformation);
		}
		if (sceneviewernotifier)
		{
			cmzn_sceneviewernotifier_destroy(&sceneviewernotifier);
		}
		remove_transformation_callback();
		if (projection_matrix)
		{
			DEALLOCATE(projection_matrix);
		}
		if (current_scene)
		{
			cmzn_scene_destroy(&current_scene);
		}
		if (graphics_window_name)
		{
			DEALLOCATE(graphics_window_name);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_scene_viewer_projection::~Computed_field_scene_viewer_projection.  "
			"Invalid arguments.");
	}
	LEAVE;

} /* Computed_field_scene_viewer_projection::~Computed_field_scene_viewer_projection */

int Computed_field_scene_viewer_projection::compare(Computed_field_core *other_core)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Compare the type specific data
==============================================================================*/
{
	Computed_field_scene_viewer_projection *other;
	int return_code;

	ENTER(Computed_field_scene_viewer_projection::compare);
	if (field && (other = dynamic_cast<Computed_field_scene_viewer_projection*>(other_core)))
	{
		if ((scene_viewer == other->scene_viewer) &&
			(from_coordinate_system == other->from_coordinate_system) &&
			(to_coordinate_system == other->to_coordinate_system))
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
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_scene_viewer_projection::compare */

int Computed_field_scene_viewer_projection::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
	if (scene_viewer)
	{
		// note, assuming matrix is owned by field not cache. Not thread safe.
		if (!requiredProjectionMatrixUpdate() || calculate_matrix())
		{
			for (int i = 0 ; i < field->number_of_components ; i++)
			{
				valueCache.values[i] = projection_matrix[i];
			}
			return 1;
		}
	}
	//else
	//{
	//	/* Just set everything to zero */
	//	for (int i = 0 ; i < field->number_of_components ; i++)
	//	{
	//		valueCache.values[i] = 0.0;
	//	}
	//}
	//return 1;
	return 0;
}

/* return 1 if projection matrix requires an update */
int Computed_field_scene_viewer_projection::requiredProjectionMatrixUpdate()
{
	int return_code = 0;
	if ((from_coordinate_system == CMZN_SCENECOORDINATESYSTEM_LOCAL) ||
		(to_coordinate_system == CMZN_SCENECOORDINATESYSTEM_LOCAL))
	{
		cmzn_field_id field = getField();
		cmzn_fieldmodule_id field_module = cmzn_field_get_fieldmodule(field);
		if (!field_module)
		{
			return 0;
		}
		cmzn_region_id region = cmzn_fieldmodule_get_region_internal(field_module);
		cmzn_scene_id scene = region->getScene();
		cmzn_scene_id top_scene = cmzn_sceneviewer_get_scene(scene_viewer);
		gtMatrix *local_transformation_matrix = cmzn_scene_get_total_transformation(
			scene, top_scene);
		cmzn_scene_destroy(&top_scene);
		if (!current_local_transformation && local_transformation_matrix)
		{
			return_code = 1;
		}
		else
		{
			if (!change_required && local_transformation_matrix)
			{
				int i, j;
				for (i=0; i<4 && !return_code; i++)
				{
					for (j=0; j<4 && !return_code; j++)
					{
						if (fabs(((*(local_transformation_matrix))[i][j]) -
							((*(current_local_transformation))[i][j])) > 0.0000001)
						{
							return_code = 1;
							break;
						}
					}
				}
			}
			else
			{
				return_code = 1;
			}
		}
		cmzn_fieldmodule_destroy(&field_module);
		if (current_local_transformation)
			DEALLOCATE(current_local_transformation);
		current_local_transformation = local_transformation_matrix;
	}
	else
	{
		if (change_required)
			return_code = 1;
	}
	return return_code;
}


int Computed_field_scene_viewer_projection::list()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_scene_viewer_projection);
	if (field)
	{
		return_code = 1;
		if (graphics_window_name)
			display_message(INFORMATION_MESSAGE,"    window : %s\n",
				graphics_window_name);
		if (pane_number > -1)
			display_message(INFORMATION_MESSAGE,"    pane number : %d\n",
				pane_number + 1);
		display_message(INFORMATION_MESSAGE,"    from_coordinate_system : %s\n",
			ENUMERATOR_STRING(cmzn_scenecoordinatesystem)(from_coordinate_system));
		display_message(INFORMATION_MESSAGE,"    to_coordinate_system : %s\n",
			ENUMERATOR_STRING(cmzn_scenecoordinatesystem)(to_coordinate_system));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_scene_viewer_projection.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_scene_viewer_projection */

char *Computed_field_scene_viewer_projection::get_command_string()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string;
	int error;

	ENTER(Computed_field_scene_viewer_projection::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_scene_viewer_projection_type_string, &error);
		append_string(&command_string, " field ", &error);
		if (graphics_window_name)
		{
			append_string(&command_string, " window ", &error);
			append_string(&command_string, graphics_window_name, &error);
		}
		if (pane_number > -1)
		{
			char temp_string[40];
			sprintf(temp_string, " pane_number %d ", pane_number + 1);
			append_string(&command_string, temp_string, &error);
		}
		append_string(&command_string, " from_coordinate_system ", &error);
		append_string(&command_string,
			ENUMERATOR_STRING(cmzn_scenecoordinatesystem)(from_coordinate_system), &error);
		append_string(&command_string, " to_coordinate_system ", &error);
		append_string(&command_string,
			ENUMERATOR_STRING(cmzn_scenecoordinatesystem)(to_coordinate_system), &error);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_scene_viewer_projection::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_scene_viewer_projection::get_command_string */

void Computed_field_scene_viewer_projection::set_graphics_window_name(const char *name)
{
	if (name)
		graphics_window_name = duplicate_string(name);

}

void Computed_field_scene_viewer_projection::set_pane_number(int number)
{
	pane_number = number;
}

void Computed_field_scene_viewer_projection::add_transformation_callback()
{
	if (!transformation_callback_flag)
	{
		if (current_scene)
			cmzn_scene_destroy(&current_scene);
		current_scene = cmzn_sceneviewer_get_scene(scene_viewer);
		cmzn_field_id field = getField();
		cmzn_fieldmodule_id field_module = cmzn_field_get_fieldmodule(field);
		if (field_module)
		{
			cmzn_region_id region = cmzn_fieldmodule_get_region_internal(field_module);
			struct cmzn_scene *scene = region->getScene();
			transformation_callback_flag = cmzn_scene_add_total_transformation_callback(
				scene, current_scene,
				Computed_field_scene_projection_transformation_callback,
				Computed_field_scene_viewer_top_scene_change_callback, (void *)field);
			cmzn_fieldmodule_destroy(&field_module);
		}
	}
}

void Computed_field_scene_viewer_projection::remove_transformation_callback()
{
	if (transformation_callback_flag)
	{
		cmzn_field_id field = getField();
		cmzn_fieldmodule_id field_module = cmzn_field_get_fieldmodule(field);
		if (field_module)
		{
			cmzn_region_id region = cmzn_fieldmodule_get_region_internal(field_module);
			struct cmzn_scene *scene = region->getScene();
			cmzn_scene_remove_total_transformation_callback(scene,
				current_scene, Computed_field_scene_projection_transformation_callback,
				Computed_field_scene_viewer_top_scene_change_callback, (void *)field);
			cmzn_fieldmodule_destroy(&field_module);
			transformation_callback_flag = 0;
		}
	}
}

void Computed_field_scene_viewer_projection::update_current_scene()
{
	cmzn_scene_id top_scene = cmzn_sceneviewer_get_scene(scene_viewer);
	if (current_scene != top_scene)
	{
		if ((from_coordinate_system == CMZN_SCENECOORDINATESYSTEM_LOCAL) ||
			(to_coordinate_system == CMZN_SCENECOORDINATESYSTEM_LOCAL))
		{
			remove_transformation_callback();
			add_transformation_callback();
		}
		if (current_scene)
			cmzn_scene_destroy(&current_scene);
		current_scene = cmzn_scene_access(top_scene);
	}
	cmzn_scene_destroy(&top_scene);
}

void Computed_field_scene_viewer_projection_scene_viewer_callback(
	cmzn_sceneviewerevent_id event, void *field_void)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Clear the projection matrix as it is no longer valid and notify the manager
that the computed field has changed.
==============================================================================*/
{
	Computed_field* field;
	Computed_field_scene_viewer_projection* core;

	if (event && (field = (Computed_field *)field_void) &&
		(core = dynamic_cast<Computed_field_scene_viewer_projection*>(field->core)))
	{
		cmzn_sceneviewerevent_change_flags changeFlags =
			cmzn_sceneviewerevent_get_change_flags(event);
		if (changeFlags & CMZN_SCENEVIEWEREVENT_CHANGE_FLAG_TRANSFORM)
		{
			core->update_current_scene();
			if (!core->change_required)
			{
				core->change_required = 1;
				if (field->manager)
				{
					Computed_field_dependency_changed(field);
				}
			}
		}
		if (changeFlags & CMZN_SCENEVIEWEREVENT_CHANGE_FLAG_FINAL)
		{
			if (core->sceneviewernotifier)
			{
				cmzn_sceneviewernotifier_destroy(&core->sceneviewernotifier);
			}
			if (core->graphics_window_name)
			{
				DEALLOCATE(core->graphics_window_name);
			}
			core->pane_number = 0;
			core->scene_viewer = (struct Scene_viewer *)NULL;
			if (field->manager)
			{
				field->setChanged();
			}
		}
	}

} /* Computed_field_scene_viewer_projection_scene_viewer_callback */

void Computed_field_scene_projection_transformation_callback(
	cmzn_scene_id scene, void *dummy_void, void *field_void)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Clear the projection matrix as it is no longer valid and notify the manager
that the computed field has changed.
==============================================================================*/
{
	Computed_field* field;
	Computed_field_scene_viewer_projection* core;
	USE_PARAMETER(dummy_void);

	if (scene && (field = (Computed_field *)field_void) &&
		(core = dynamic_cast<Computed_field_scene_viewer_projection*>(field->core)))
	{
		if (!core->change_required)
		{
			if (field->manager)
			{
				Computed_field_dependency_changed(field);
			}
			core->change_required = 1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_scene_projection_transformation_callback.  "
			"Invalid arguments.");
	}
}

void Computed_field_scene_viewer_top_scene_change_callback(
	cmzn_scene_id scene, cmzn_scene_id top_scene, void *field_void)
{
	Computed_field* field;
	Computed_field_scene_viewer_projection* core;

	if (scene && (field = (Computed_field *)field_void) &&
		(core = dynamic_cast<Computed_field_scene_viewer_projection*>(field->core)))
	{
		if (core->current_scene == top_scene)
		{
			core->remove_transformation_callback();
			if (!core->change_required)
			{
				if (field->manager)
				{
					Computed_field_dependency_changed(field);
				}
				core->change_required = 1;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_scene_viewer_top_scene_change_callback.  "
			"Invalid arguments.");
	}
}

} //namespace

cmzn_field_id cmzn_fieldmodule_create_field_sceneviewer_projection(
	cmzn_fieldmodule_id field_module, cmzn_sceneviewer_id sceneviewer,
	enum cmzn_scenecoordinatesystem from_coordinate_system,
	enum cmzn_scenecoordinatesystem to_coordinate_system)
{
	cmzn_field *field = nullptr;
	if (sceneviewer)
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/false,
			/*number_of_components*/16,
			/*number_of_source_fields*/0, NULL,
			/*number_of_source_values*/0, NULL,
			new Computed_field_scene_viewer_projection(sceneviewer, from_coordinate_system,
				to_coordinate_system));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_sceneviewer_projection.  Invalid argument(s)");
	}

	return (field);
}

int cmzn_field_projection_set_window_name(struct Computed_field *field, const char *graphics_window_name)
{
	int return_code = 0;
	Computed_field_scene_viewer_projection* core = 0;

	if (field && (core = dynamic_cast<Computed_field_scene_viewer_projection*>(field->core)))
	{
		core->set_graphics_window_name(graphics_window_name);
		return_code = 1;
	}

	return return_code;
}

int cmzn_field_projection_set_pane_number(struct Computed_field *field, int pane_number)
{
	int return_code = 0;
	Computed_field_scene_viewer_projection* core = 0;

	if (field && (core = dynamic_cast<Computed_field_scene_viewer_projection*>(field->core)))
	{
		core->set_pane_number(pane_number);
		return_code = 1;
	}

	return return_code;
}

int Computed_field_get_type_scene_viewer_projection(struct Computed_field *field,
	struct Scene_viewer **scene_viewer, char **graphics_window_name, int *pane_number,
	enum cmzn_scenecoordinatesystem *from_coordinate_system,
	enum cmzn_scenecoordinatesystem *to_coordinate_system)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_scene_viewer_projection,
<graphics_window>, <pane_number> and <projection_type> used by it are returned.
Use function Computed_field_get_type to determine the field type.
==============================================================================*/
{
	Computed_field_scene_viewer_projection* core;
	int return_code;

	ENTER(Computed_field_get_type_scene_viewer_projection);
	if (field&&(core = dynamic_cast<Computed_field_scene_viewer_projection*>(field->core))
		&& scene_viewer)
	{
		*scene_viewer = core->scene_viewer;
		if (core->graphics_window_name)
			*graphics_window_name = duplicate_string(core->graphics_window_name);
		else
			*graphics_window_name = NULL;
		*pane_number = core->pane_number;
		*from_coordinate_system = core->from_coordinate_system;
		*to_coordinate_system = core->to_coordinate_system;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_scene_viewer_projection.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_scene_viewer_projection */

