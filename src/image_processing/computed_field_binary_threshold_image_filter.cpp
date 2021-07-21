/*******************************************************************************
FILE : computed_field_binary_threshold_image_filter.cpp

LAST MODIFIED : 16 May 2008

DESCRIPTION :
Wraps itk::BinaryThresholdImageFilter
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "opencmiss/zinc/fieldimageprocessing.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_private.hpp"
#include "image_processing/computed_field_image_filter.h"
#include "computed_field/computed_field_set.h"
#include "general/debug.h"
#include "general/mystring.h"
#include "general/message.h"
#include "image_processing/computed_field_binary_threshold_image_filter.h"
#include "itkImage.h"
#include "itkVector.h"
#include "itkBinaryThresholdImageFilter.h"

using namespace CMZN;

namespace {

char computed_field_binary_threshold_image_filter_type_string[] = "binary_threshold_filter";

class Computed_field_binary_threshold_image_filter : public computed_field_image_filter
{

public:
	double lower_threshold;
	double upper_threshold;

	Computed_field_binary_threshold_image_filter(Computed_field *source_field,
		double lower_threshold = 0.0, double upper_threshold = 1.0);

	~Computed_field_binary_threshold_image_filter()
	{
	}

	double getLowerThreshold()
	{
		return lower_threshold;
	}

	int setLowerThreshold(double lowerThresholdIn)
	{
		if (lower_threshold != lowerThresholdIn)
		{
			lower_threshold = lowerThresholdIn;
			clear_cache();
		}
		return CMZN_OK;
	}

	double getUpperThreshold()
	{
		return upper_threshold;
	}

	int setUpperThreshold(double upperThresholdIn)
	{
		if (upper_threshold != upperThresholdIn)
		{
			upper_threshold = upperThresholdIn;
			clear_cache();
		}
		return CMZN_OK;
	}

private:
	virtual void create_functor();

	Computed_field_core *copy()
	{
		return new Computed_field_binary_threshold_image_filter(field->source_fields[0],
			lower_threshold, upper_threshold);
	}

	const char *get_type_string()
	{
		return(computed_field_binary_threshold_image_filter_type_string);
	}

	int compare(Computed_field_core* other_field);

	int list();

	char* get_command_string();
};

inline Computed_field_binary_threshold_image_filter *
	Computed_field_binary_threshold_image_filter_core_cast(
		cmzn_field_imagefilter_binary_threshold *imagefilter_binary_threshold)
{
	return (static_cast<Computed_field_binary_threshold_image_filter*>(
		reinterpret_cast<Computed_field*>(imagefilter_binary_threshold)->core));
}

/*****************************************************************************//**
 * Compare the type specific data
 * 
 * @param other_core Core of other field to compare against
 * @return Return code indicating field is the same (1) or different (0)
*/
int Computed_field_binary_threshold_image_filter::compare(Computed_field_core *other_core)
{
	Computed_field_binary_threshold_image_filter* other;
	int return_code;

	ENTER(Computed_field_binary_threshold_image_filter::compare);
	if (field && (other = dynamic_cast<Computed_field_binary_threshold_image_filter*>(other_core)))
	{
		if ((dimension == other->dimension)
			&& (lower_threshold == other->lower_threshold)
			&& (upper_threshold == other->upper_threshold))
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
} /* Computed_field_binary_threshold_image_filter::compare */

template < class ImageType >
class Computed_field_binary_threshold_image_filter_Functor :
	public computed_field_image_filter_FunctorTmpl< ImageType >
/*******************************************************************************
LAST MODIFIED : 12 September 2006

DESCRIPTION :
This class actually does the work of processing images with the filter.
It is instantiated for each of the chosen ImageTypes.
==============================================================================*/
{
	Computed_field_binary_threshold_image_filter *binary_threshold_image_filter;

public:

	Computed_field_binary_threshold_image_filter_Functor(
		Computed_field_binary_threshold_image_filter *binary_threshold_image_filter) :
		computed_field_image_filter_FunctorTmpl< ImageType >(binary_threshold_image_filter),
		binary_threshold_image_filter(binary_threshold_image_filter)
	{
	}

/*****************************************************************************//**
 * Create a filter of the correct type, set the filter specific parameters
 * and generate the output
 * 
 * @param location Field location
 * @return Return code indicating succes (1) or failure (0)
*/
	int set_filter(cmzn_fieldcache& cache)
	{
		int return_code;
		
		typedef itk::BinaryThresholdImageFilter< ImageType , ImageType > FilterType;
		
		typename FilterType::Pointer filter = FilterType::New();
		
		filter->SetLowerThreshold( binary_threshold_image_filter->lower_threshold );
		filter->SetUpperThreshold( binary_threshold_image_filter->upper_threshold );
		
		return_code = binary_threshold_image_filter->update_output_image
			(cache, filter, this->outputImage,
			static_cast<ImageType*>(NULL), static_cast<FilterType*>(NULL));
		
		return (return_code);
	} /* set_filter */

}; /* template < class ImageType > class Computed_field_binary_threshold_image_filter_Functor */


/*****************************************************************************//**
 * Constructor for a binary threshold image filter field
 * Creates the computed field representation of the binary threshold image filter
 * 
 * @param field Generic computed field
 * @param lower_threshold Lower threshold value (below which values will be set to 0)
 * @param upper_threshold Upper threshold value (above which values will be set to 0)
 * @return Return code indicating succes (1) or failure (0)
*/
Computed_field_binary_threshold_image_filter::Computed_field_binary_threshold_image_filter(
	Computed_field *source_field, double lower_threshold, double upper_threshold) :
	computed_field_image_filter(source_field),
	lower_threshold(lower_threshold), upper_threshold(upper_threshold)
{
}

void Computed_field_binary_threshold_image_filter::create_functor()
{
#if defined DONOTUSE_TEMPLATETEMPLATES
	create_filters_singlecomponent_multidimensions(
		Computed_field_binary_threshold_image_filter_Functor, this);
#else
	create_filters_singlecomponent_multidimensions
		< Computed_field_binary_threshold_image_filter_Functor,
		Computed_field_binary_threshold_image_filter >
		(this);
#endif
}

/*****************************************************************************//**
 * List field parameters
 * 
 * @return Return code indicating succes (1) or failure (0)
*/
int Computed_field_binary_threshold_image_filter::list()
{
	int return_code = 0;

	ENTER(List_Computed_field_binary_threshold_image_filter);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    source field : %s\n",field->source_fields[0]->name);
		display_message(INFORMATION_MESSAGE,
			"    lower_threshold : %g\n", lower_threshold);
		display_message(INFORMATION_MESSAGE,
			"    upper_threshold : %g\n", upper_threshold);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_binary_threshold_image_filter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_binary_threshold_image_filter */

/*****************************************************************************//**
 * Returns allocated command string for reproducing field. Includes type.
 * 
 * @return Command string to create field
*/
char *Computed_field_binary_threshold_image_filter::get_command_string()
{
	char *command_string, *field_name, temp_string[40];
	int error;

	ENTER(Computed_field_binary_threshold_image_filter::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string, get_type_string(), &error);
		append_string(&command_string, " field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
		sprintf(temp_string, " lower_threshold %g", lower_threshold);
		append_string(&command_string, temp_string, &error);		
		sprintf(temp_string, " upper_threshold %g", upper_threshold);	
		append_string(&command_string, temp_string, &error);		
}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_binary_threshold_image_filter::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_binary_threshold_image_filter::get_command_string */

} //namespace

cmzn_field_imagefilter_binary_threshold_id cmzn_field_cast_imagefilter_binary_threshold(cmzn_field_id field)
{
	if (dynamic_cast<Computed_field_binary_threshold_image_filter*>(field->core))
	{
		cmzn_field_access(field);
		return (reinterpret_cast<cmzn_field_imagefilter_binary_threshold_id>(field));
	}
	else
	{
		return (NULL);
	}
}

double cmzn_field_imagefilter_binary_threshold_get_lower_threshold(
	cmzn_field_imagefilter_binary_threshold_id imagefilter_binary_threshold)
{
	if (imagefilter_binary_threshold)
	{
		Computed_field_binary_threshold_image_filter *filter_core =
			Computed_field_binary_threshold_image_filter_core_cast(
				imagefilter_binary_threshold);
		return filter_core->getLowerThreshold();
	}

	return 0.0;
}

int cmzn_field_imagefilter_binary_threshold_set_lower_threshold(
	cmzn_field_imagefilter_binary_threshold_id imagefilter_binary_threshold,
	double lower_threshold)
{
	if (imagefilter_binary_threshold)
	{
		Computed_field_binary_threshold_image_filter *filter_core =
			Computed_field_binary_threshold_image_filter_core_cast(
				imagefilter_binary_threshold);
		return filter_core->setLowerThreshold(lower_threshold);
	}
	return CMZN_ERROR_ARGUMENT;
}

double cmzn_field_imagefilter_binary_threshold_get_upper_threshold(
	cmzn_field_imagefilter_binary_threshold_id imagefilter_binary_threshold)
{
	if (imagefilter_binary_threshold)
	{
		Computed_field_binary_threshold_image_filter *filter_core =
			Computed_field_binary_threshold_image_filter_core_cast(
				imagefilter_binary_threshold);
		return filter_core->getUpperThreshold();
	}

	return 0.0;
}

int cmzn_field_imagefilter_binary_threshold_set_upper_threshold(
	cmzn_field_imagefilter_binary_threshold_id imagefilter_binary_threshold,
	double upper_threshold)
{
	if (imagefilter_binary_threshold)
	{
		Computed_field_binary_threshold_image_filter *filter_core =
			Computed_field_binary_threshold_image_filter_core_cast(
				imagefilter_binary_threshold);
		return filter_core->setUpperThreshold(upper_threshold);
	}
	return CMZN_ERROR_ARGUMENT;
}
int cmzn_field_imagefilter_binary_threshold_destroy(
	cmzn_field_imagefilter_binary_threshold_id *imagefilter_binary_threshold_address)
{
	return cmzn_field_destroy(reinterpret_cast<cmzn_field_id *>(imagefilter_binary_threshold_address));
}

cmzn_field_id cmzn_fieldmodule_create_field_imagefilter_binary_threshold(
	cmzn_fieldmodule_id field_module, cmzn_field_id source_field)
{
	Computed_field *field = NULL;
	if (source_field && Computed_field_is_scalar(source_field, (void *)NULL))
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field->number_of_components,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_binary_threshold_image_filter(source_field));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_imagefilter_binary_threshold.  Invalid argument(s)");
	}

	return (field);
}

int cmzn_field_get_type_binary_threshold_image_filter(struct Computed_field *field,
	struct Computed_field **source_field, double *lower_threshold,
	double *upper_threshold)
{
	Computed_field_binary_threshold_image_filter* core;
	int return_code;

	ENTER(cmzn_field_get_type_binary_threshold_image_filter);
	if (field && (core = dynamic_cast<Computed_field_binary_threshold_image_filter*>(field->core))
		&& source_field)
	{
		*source_field = field->source_fields[0];
		*lower_threshold = core->lower_threshold;
		*upper_threshold = core->upper_threshold;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_field_get_type_binary_threshold_image_filter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_field_get_type_binary_threshold_image_filter */

