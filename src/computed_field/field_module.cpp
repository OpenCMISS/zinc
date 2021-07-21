/**
 * FILE : field_module.cpp
 *
 * Internal implementation of field module api.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <string>
#include "opencmiss/zinc/field.h"
#include "opencmiss/zinc/fieldmodule.h"
#include "opencmiss/zinc/timesequence.h"
#include "computed_field/computed_field_private.hpp"
#include "computed_field/field_module.hpp"
#include "computed_field/computed_field_alias.h"
#include "computed_field/computed_field_arithmetic_operators.h"
#include "computed_field/computed_field_compose.h"
#include "computed_field/computed_field_composite.h"
#include "computed_field/computed_field_conditional.h"
#include "computed_field/computed_field_coordinate.h"
#include "computed_field/computed_field_deformation.h"
#if defined (ZINC_USE_ITK)
#include "computed_field/computed_field_derivatives.h"
#endif
#include "computed_field/computed_field_finite_element.h"
#include "computed_field/computed_field_format_output.h"
#include "computed_field/computed_field_function.h"
#include "computed_field/computed_field_integration.h"
#include "computed_field/computed_field_logical_operators.h"
#include "computed_field/computed_field_lookup.h"
#include "computed_field/computed_field_string_constant.h"
#include "computed_field/computed_field_trigonometry.h"
#include "description_io/fieldmodule_json_io.hpp"
#include "image_processing/computed_field_image_resample.h"
#include "general/mystring.h"
#include "finite_element/finite_element_region.h"
#include "mesh/cmiss_element_private.hpp"
#include "mesh/cmiss_node_private.hpp"
#if defined (ZINC_USE_ITK)
#include "image_processing/computed_field_threshold_image_filter.h"
#include "image_processing/computed_field_binary_threshold_image_filter.h"
#include "image_processing/computed_field_canny_edge_detection_filter.h"
#include "image_processing/computed_field_mean_image_filter.h"
#include "image_processing/computed_field_sigmoid_image_filter.h"
#include "image_processing/computed_field_discrete_gaussian_image_filter.h"
#include "image_processing/computed_field_curvature_anisotropic_diffusion_image_filter.h"
#include "image_processing/computed_field_derivative_image_filter.h"
#include "image_processing/computed_field_rescale_intensity_image_filter.h"
#include "image_processing/computed_field_connected_threshold_image_filter.h"
#include "image_processing/computed_field_gradient_magnitude_recursive_gaussian_image_filter.h"
#include "image_processing/computed_field_histogram_image_filter.h"
#include "image_processing/computed_field_fast_marching_image_filter.h"
#include "image_processing/computed_field_binary_dilate_image_filter.h"
#include "image_processing/computed_field_binary_erode_image_filter.h"
#endif
#include "region/cmiss_region.hpp"
#include "general/message.h"
#include "computed_field/computed_field_matrix_operators.hpp"
#include "computed_field/computed_field_nodeset_operators.hpp"
#include "computed_field/computed_field_vector_operators.hpp"

cmzn_fieldmoduleevent::cmzn_fieldmoduleevent(cmzn_region *regionIn) :
	region(cmzn_region_access(regionIn)),
	changeFlags(CMZN_FIELD_CHANGE_FLAG_NONE),
	managerMessage(0),
	feRegionChanges(0),
	access_count(1)
{
}

cmzn_fieldmoduleevent::~cmzn_fieldmoduleevent()
{
	if (managerMessage)
		MANAGER_MESSAGE_DEACCESS(Computed_field)(&(this->managerMessage));
	FE_region_changes::deaccess(this->feRegionChanges);
	cmzn_region_destroy(&this->region);
}

void cmzn_fieldmoduleevent::setFeRegionChanges(FE_region_changes *changes)
{
	this->feRegionChanges = changes->access();
}

/**
 * Object to pass into field create functions, supplying region field is to
 * go into and other default parameters.
 */
struct cmzn_fieldmodule
{
	cmzn_region *region;
	char *field_name;
	struct Coordinate_system coordinate_system;
	int coordinate_system_override; // true if coordinate system has been set
	Computed_field *replace_field;
	int access_count;
};

struct cmzn_fieldmodule *cmzn_fieldmodule_create(struct cmzn_region *region)
{
	ENTER(cmzn_fieldmodule_create);
	cmzn_fieldmodule *fieldmodule = NULL;
	if (region)
	{
		ALLOCATE(fieldmodule, struct cmzn_fieldmodule, sizeof(struct cmzn_fieldmodule));
		if (fieldmodule)
		{
			fieldmodule->region = ACCESS(cmzn_region)(region);
			fieldmodule->field_name = (char *)NULL;
			fieldmodule->replace_field = (Computed_field *)NULL;
			fieldmodule->coordinate_system.type = RECTANGULAR_CARTESIAN;
			fieldmodule->coordinate_system_override = 0;
			fieldmodule->access_count = 1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE, "cmzn_fieldmodule_create.  Missing region");
	}
	LEAVE;

	return (fieldmodule);
};

struct cmzn_fieldmodule *cmzn_fieldmodule_access(struct cmzn_fieldmodule *fieldmodule)
{
	if (fieldmodule)
		++(fieldmodule->access_count);
	return fieldmodule;
}

int cmzn_fieldmodule_destroy(
	struct cmzn_fieldmodule **fieldmodule_address)
{
	struct cmzn_fieldmodule *fieldmodule;
	if (fieldmodule_address && (NULL != (fieldmodule = *fieldmodule_address)))
	{
		fieldmodule->access_count--;
		if (0 == fieldmodule->access_count)
		{
			DEACCESS(cmzn_region)(&fieldmodule->region);
			DEALLOCATE(fieldmodule->field_name)
			REACCESS(Computed_field)(&fieldmodule->replace_field, NULL);
			DEALLOCATE(*fieldmodule_address);
		}
		*fieldmodule_address = 0;
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

char *cmzn_fieldmodule_get_unique_field_name(
	struct cmzn_fieldmodule *fieldmodule)
{
	struct MANAGER(Computed_field) *manager;
	if (fieldmodule && (manager = fieldmodule->region->getFieldManager()))
	{
		return Computed_field_manager_get_unique_field_name(manager);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_get_unique_field_name.  Invalid argument(s)");
	}
	return NULL;
}

struct Computed_field *cmzn_fieldmodule_find_field_by_name(
	struct cmzn_fieldmodule *fieldmodule, const char *field_name)
{
	struct Computed_field *field;
	struct MANAGER(Computed_field) *manager;

	ENTER(cmzn_fieldmodule_find_field_by_name);
	if (fieldmodule && field_name && (manager = fieldmodule->region->getFieldManager()))
	{
		field = FIND_BY_IDENTIFIER_IN_MANAGER(Computed_field,name)(
			(char *)field_name, manager);
		if (field)
		{
			ACCESS(Computed_field)(field);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_find_field_by_name.  Invalid argument(s)");
		field = (struct Computed_field *)NULL;
	}
	LEAVE;

	return (field);
}

bool cmzn_fieldmodule_contains_field(cmzn_fieldmodule_id fieldmodule,
	cmzn_field_id field)
{
	return (cmzn_fieldmodule_get_region_internal(fieldmodule) ==
		Computed_field_get_region(field));
}

struct cmzn_region *cmzn_fieldmodule_get_region_internal(
	struct cmzn_fieldmodule *fieldmodule)
{
	if (fieldmodule)
		return fieldmodule->region;
	return 0;
}

struct cmzn_region *cmzn_fieldmodule_get_region(
	struct cmzn_fieldmodule *fieldmodule)
{
	if (fieldmodule)
		return ACCESS(cmzn_region)(fieldmodule->region);
	return NULL;
}

bool cmzn_fieldmodule_match(cmzn_fieldmodule_id fieldmodule1,
	cmzn_fieldmodule_id fieldmodule2)
{
	if (fieldmodule1 && fieldmodule2)
		return (fieldmodule1->region == fieldmodule2->region);
	return false;
}

int cmzn_fieldmodule_set_field_name(
	struct cmzn_fieldmodule *fieldmodule, const char *field_name)
{
	int return_code = 0;
	if (fieldmodule)
	{
		if (fieldmodule->field_name)
		{
			DEALLOCATE(fieldmodule->field_name);
		}
		fieldmodule->field_name = field_name ? duplicate_string(field_name) : NULL;
		return_code = 1;
	}
	return (return_code);
}

char *cmzn_fieldmodule_get_field_name(
	struct cmzn_fieldmodule *fieldmodule)
{
	if (fieldmodule && fieldmodule->field_name)
	{
		return duplicate_string(fieldmodule->field_name);
	}
	return NULL;
}

int cmzn_fieldmodule_set_coordinate_system(
	struct cmzn_fieldmodule *fieldmodule,
	struct Coordinate_system coordinate_system)
{
	if (fieldmodule)
	{
		fieldmodule->coordinate_system = coordinate_system;
		fieldmodule->coordinate_system_override = 1;
		return 1;
	}
	return 0;
}

struct Coordinate_system cmzn_fieldmodule_get_coordinate_system(
	struct cmzn_fieldmodule *fieldmodule)
{
	if (fieldmodule)
	{
		return fieldmodule->coordinate_system;
	}
	// return dummy
	struct Coordinate_system coordinate_system;
	coordinate_system.type = RECTANGULAR_CARTESIAN;
	return (coordinate_system);
}

int cmzn_fieldmodule_coordinate_system_is_set(
	struct cmzn_fieldmodule *fieldmodule)
{
	if (fieldmodule)
	{
		return fieldmodule->coordinate_system_override;
	}
	return 0;
}

int cmzn_fieldmodule_clear_coordinate_system(
	struct cmzn_fieldmodule *fieldmodule)
{
	if (fieldmodule)
	{
		fieldmodule->coordinate_system_override = 0;
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_fieldmodule_set_replace_field(
	struct cmzn_fieldmodule *fieldmodule,
	struct Computed_field *replace_field)
{
	int return_code;

	if (fieldmodule && ((NULL == replace_field) ||
		(fieldmodule->region == Computed_field_get_region(replace_field))))
	{
		REACCESS(Computed_field)(&fieldmodule->replace_field, replace_field);
		if (replace_field)
		{
			// copy settings from replace_field to be new defaults
			char *field_name = NULL;
			if (GET_NAME(Computed_field)(replace_field, &field_name))
			{
				if (fieldmodule->field_name)
				{
					DEALLOCATE(fieldmodule->field_name);
				}
				fieldmodule->field_name = field_name;
			}
			fieldmodule->coordinate_system = replace_field->coordinate_system;
			fieldmodule->coordinate_system_override = 1;
		}
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_set_replace_field.  Invalid arguments");
		return_code = 0;
	}

	return (return_code);
}

struct Computed_field *cmzn_fieldmodule_get_replace_field(
	struct cmzn_fieldmodule *fieldmodule)
{
	Computed_field *replace_field = NULL;
	if (fieldmodule)
	{
		replace_field = fieldmodule->replace_field;
	}
	return (replace_field);
}

cmzn_fielditerator_id cmzn_fieldmodule_create_fielditerator(
	cmzn_fieldmodule_id fieldmodule)
{
	if (!fieldmodule)
		return 0;
	return fieldmodule->region->createFielditerator();
}

cmzn_fieldmodulenotifier_id cmzn_fieldmodule_create_fieldmodulenotifier(
	cmzn_fieldmodule_id fieldmodule)
{
	return cmzn_fieldmodulenotifier::create(fieldmodule);
}

cmzn_timesequence_id cmzn_fieldmodule_get_matching_timesequence(
	cmzn_fieldmodule_id fieldmodule, int number_of_times, const double *times)
{
	if (!fieldmodule)
		return NULL;
	FE_time_sequence *fe_timesequence = FE_region_get_FE_time_sequence_matching_series(
		fieldmodule->region->get_FE_region(), number_of_times, times);
	ACCESS(FE_time_sequence)(fe_timesequence);
	return reinterpret_cast<cmzn_timesequence_id>(fe_timesequence);
}

cmzn_field_id cmzn_fieldmodule_get_or_create_xi_field(cmzn_fieldmodule_id fieldmodule)
{
	cmzn_field_id xi_field = 0;
	if (fieldmodule)
	{
		const char *default_xi_field_name = "xi";
		char xi_field_name[10];
		strcpy(xi_field_name, default_xi_field_name);
		int i = 2;
		while (true)
		{
			xi_field = cmzn_fieldmodule_find_field_by_name(fieldmodule, xi_field_name);
			if (xi_field)
			{
				if (Computed_field_is_type_xi_coordinates(xi_field, (void *)NULL))
				{
					break;
				}
				cmzn_field_destroy(&xi_field);
			}
			else
			{
				xi_field = cmzn_fieldmodule_create_field_xi_coordinates(fieldmodule);
				cmzn_field_set_name(xi_field, xi_field_name);
				cmzn_field_set_managed(xi_field, true);
				break;
			}
			sprintf(xi_field_name, "%s%d", default_xi_field_name, i++);
		}
	}
	return xi_field;
}

int cmzn_fieldmoduleevent::deaccess(cmzn_fieldmoduleevent* &event)
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

cmzn_fieldmodulenotifier::cmzn_fieldmodulenotifier(cmzn_fieldmodule *fieldmodule) :
	region(fieldmodule->region),
	function(0),
	user_data(0),
	access_count(1)
{
	this->region->addFieldmodulenotifier(this);
}

cmzn_fieldmodulenotifier::~cmzn_fieldmodulenotifier()
{
}

int cmzn_fieldmodulenotifier::deaccess(cmzn_fieldmodulenotifier* &notifier)
{
	if (notifier)
	{
		--(notifier->access_count);
		if (notifier->access_count <= 0)
			delete notifier;
		else if ((1 == notifier->access_count) && notifier->region)
			notifier->region->removeFieldmodulenotifier(notifier);
		notifier = 0;
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_fieldmodulenotifier::setCallback(cmzn_fieldmodulenotifier_callback_function function_in,
	void *user_data_in)
{
	if (!function_in)
		return CMZN_ERROR_ARGUMENT;
	this->function = function_in;
	this->user_data = user_data_in;
	return CMZN_OK;
}

void cmzn_fieldmodulenotifier::clearCallback()
{
	this->function = 0;
	this->user_data = 0;
}

void cmzn_fieldmodulenotifier::regionDestroyed()
{
	this->region = 0;
	if (this->function)
	{
		cmzn_fieldmoduleevent_id event = cmzn_fieldmoduleevent::create(static_cast<cmzn_region*>(0));
		event->setChangeFlags(CMZN_FIELD_CHANGE_FLAG_FINAL);
		(this->function)(event, this->user_data);
		cmzn_fieldmoduleevent::deaccess(event);
		this->clearCallback();
	}
}

int cmzn_fieldmodulenotifier_clear_callback(cmzn_fieldmodulenotifier_id notifier)
{
	if (notifier)
	{
		notifier->clearCallback();
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_fieldmodulenotifier_set_callback(cmzn_fieldmodulenotifier_id notifier,
	cmzn_fieldmodulenotifier_callback_function function_in, void *user_data_in)
{
	if (notifier && function_in)
		return notifier->setCallback(function_in, user_data_in);
	return CMZN_ERROR_ARGUMENT;
}

void *cmzn_fieldmodulenotifier_get_callback_user_data( 
 cmzn_fieldmodulenotifier_id notifier)
{
	if (notifier)
		return notifier->getUserData();
	return 0;
}

cmzn_fieldmodulenotifier_id cmzn_fieldmodulenotifier_access(
	cmzn_fieldmodulenotifier_id notifier)
{
	if (notifier)
		return notifier->access();
	return 0;
}

int cmzn_fieldmodulenotifier_destroy(cmzn_fieldmodulenotifier_id *notifier_address)
{
	return cmzn_fieldmodulenotifier::deaccess(*notifier_address);
}

cmzn_fieldmoduleevent_id cmzn_fieldmoduleevent_access(
	cmzn_fieldmoduleevent_id event)
{
	if (event)
		return event->access();
	return 0;
}

int cmzn_fieldmoduleevent_destroy(cmzn_fieldmoduleevent_id *event_address)
{
	return cmzn_fieldmoduleevent::deaccess(*event_address);
}

cmzn_field_change_flags cmzn_fieldmoduleevent_get_summary_field_change_flags(cmzn_fieldmoduleevent_id event)
{
	if (event)
		return event->getChangeFlags();
	return CMZN_FIELD_CHANGE_FLAG_NONE;
}

cmzn_field_change_flags cmzn_fieldmoduleevent_get_field_change_flags(
	cmzn_fieldmoduleevent_id event, cmzn_field_id field)
{
	if (event)
		return event->getFieldChangeFlags(field);
	return CMZN_FIELD_CHANGE_FLAG_NONE;
}

cmzn_meshchanges_id cmzn_fieldmoduleevent_get_meshchanges(
	cmzn_fieldmoduleevent_id event, cmzn_mesh_id mesh)
{
	return cmzn_meshchanges::create(event, mesh);
}

cmzn_nodesetchanges_id cmzn_fieldmoduleevent_get_nodesetchanges(
	cmzn_fieldmoduleevent_id event, cmzn_nodeset_id nodeset)
{
	return cmzn_nodesetchanges::create(event, nodeset);
}

 char *cmzn_fieldmodule_write_description(cmzn_fieldmodule_id fieldmodule)
 {
 	if (fieldmodule)
 	{
 		FieldmoduleJsonExport jsonExport(fieldmodule);
 		return duplicate_string(jsonExport.getExportString().c_str());
 	}
 	return 0;
 }

 int cmzn_fieldmodule_read_description(
	cmzn_fieldmodule_id fieldmodule, const char *description)
 {
 	if (fieldmodule && description)
 	{
 		FieldmoduleJsonImport jsonImport(fieldmodule);
 		std::string inputString(description);
 		return jsonImport.import(inputString);
 	}
 	return CMZN_ERROR_ARGUMENT;
 }
