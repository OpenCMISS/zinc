/*******************************************************************************
FILE : computed_field_connected_threshold_image_filter.c

LAST MODIFIED : 16 July 2007

DESCRIPTION :
Wraps itk::MeanImageFilter
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
#include "image_processing/computed_field_connected_threshold_image_filter.h"

#include "itkImage.h"
#include "itkVector.h"
#include "itkConnectedThresholdImageFilter.h"

using namespace CMZN;

namespace {

char computed_field_connected_threshold_image_filter_type_string[] = "connected_threshold_filter";

class Computed_field_connected_threshold_image_filter : public computed_field_image_filter
{

public:
	double lower_threshold;
	double upper_threshold;
  double replace_value;
	int num_seed_points;
	double *seed_points;

	// Need something to represent index

	Computed_field_connected_threshold_image_filter(Computed_field *source_field,
		double lower_threshold, double upper_threshold, double replace_value, int num_seed_points, const double *seed_points);

	~Computed_field_connected_threshold_image_filter()
	{
		if (seed_points) {
			delete [] seed_points;
		}
	}

private:
	virtual void create_functor();

	Computed_field_core *copy()
	{
		return new Computed_field_connected_threshold_image_filter(field->source_fields[0],
		  lower_threshold, upper_threshold, replace_value, num_seed_points, seed_points);
	}

	const char *get_type_string()
	{
		return(computed_field_connected_threshold_image_filter_type_string);
	}

	int compare(Computed_field_core* other_field);

	int list();

	char* get_command_string();
};

int Computed_field_connected_threshold_image_filter::compare(Computed_field_core *other_core)
/*******************************************************************************
LAST MODIFIED : 16 July 2007

DESCRIPTION :
Compare the type specific data.
==============================================================================*/
{
	Computed_field_connected_threshold_image_filter* other;
	int return_code;
	int i;

	ENTER(Computed_field_connected_threshold_image_filter::compare);
	if (field && (other = dynamic_cast<Computed_field_connected_threshold_image_filter*>(other_core)))
	{

		if ((dimension == other->dimension)
				&& (lower_threshold == other->lower_threshold)
				&& (upper_threshold == other->upper_threshold)
				&& (replace_value == other->replace_value)
				&& (num_seed_points == other->num_seed_points)) {

			return_code = 1;

			// check that all seed_points match
			for (i = 0 ; return_code && (i < dimension*num_seed_points) ; i++)
				{
				if (seed_points[i] != other->seed_points[i])
				{
					return_code = 0;
				}
			}

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
} /* Computed_field_connected_threshold_image_filter::compare */

template < class ImageType >
class Computed_field_connected_threshold_image_filter_Functor :
	public computed_field_image_filter_FunctorTmpl< ImageType >
/*******************************************************************************
LAST MODIFIED : 16 July 2007

DESCRIPTION :
This class actually does the work of processing images with the filter.
It is instantiated for each of the chosen ImageTypes.
==============================================================================*/
{
	Computed_field_connected_threshold_image_filter *connected_threshold_image_filter;

public:

	Computed_field_connected_threshold_image_filter_Functor(
		Computed_field_connected_threshold_image_filter *connected_threshold_image_filter) :
		computed_field_image_filter_FunctorTmpl< ImageType >(connected_threshold_image_filter),
		connected_threshold_image_filter(connected_threshold_image_filter)
	{
	}

	int set_filter(cmzn_fieldcache& cache)
/*******************************************************************************
LAST MODIFIED : 12 September 2006

DESCRIPTION :
Create a filter of the correct type, set the filter specific parameters
and generate the outputImage.
==============================================================================*/
	{
		int return_code;
		int *image_size;
		int i, j, dimension, num_seed_points;

		typedef itk::ConnectedThresholdImageFilter< ImageType , ImageType > FilterType;

		typename FilterType::Pointer filter = FilterType::New();

		filter->SetLower( connected_threshold_image_filter->lower_threshold );
		filter->SetUpper( connected_threshold_image_filter->upper_threshold );
		filter->SetReplaceValue( connected_threshold_image_filter->replace_value );

		typename ImageType::IndexType seedIndex;

		image_size = connected_threshold_image_filter->sizes;
		dimension  = connected_threshold_image_filter->dimension;
		num_seed_points = connected_threshold_image_filter->num_seed_points;

		// get first seed  point
		for (i = 0; i < dimension; i++)
		{
			seedIndex[i]=(int)(connected_threshold_image_filter->seed_points[i] * image_size[i]);
		}
		filter->SetSeed(seedIndex);

		// add any other seed points
		for (j = 1; j < num_seed_points; j++) {

			for (i = 0; i < dimension; i++) {
				seedIndex[i]=(int)(connected_threshold_image_filter->seed_points[j * dimension + i] * image_size[i]);
			}
			filter->AddSeed(seedIndex);
		}


		return_code = connected_threshold_image_filter->update_output_image
			(cache, filter, this->outputImage,
			static_cast<ImageType*>(NULL), static_cast<FilterType*>(NULL));

		return (return_code);
	} /* set_filter */

}; /* template < class ImageType > class Computed_field_connected_threshold_image_filter_Functor */

Computed_field_connected_threshold_image_filter::Computed_field_connected_threshold_image_filter(
	Computed_field *source_field, double lower_threshold, double upper_threshold,
	double replace_value, int num_seed_points, const double *seed_points_in) :
	computed_field_image_filter(source_field),
	lower_threshold(lower_threshold), upper_threshold(upper_threshold),
  replace_value(replace_value), num_seed_points(num_seed_points)
/*******************************************************************************
LAST MODIFIED : 16 July 2007

DESCRIPTION :
Create the computed_field representation of the connected threshold image filter.
==============================================================================*/
{
	int i;
	int seed_points_length;
	seed_points_length = dimension * num_seed_points;
	seed_points = new double[seed_points_length];
	for (i = 0 ; i < seed_points_length; i++) {
		seed_points[i] = seed_points_in[i];
	}
}

void Computed_field_connected_threshold_image_filter::create_functor()
{
#if defined DONOTUSE_TEMPLATETEMPLATES
	create_filters_singlecomponent_multidimensions(
		Computed_field_connected_threshold_image_filter_Functor, this);
#else
	create_filters_singlecomponent_multidimensions
		< Computed_field_connected_threshold_image_filter_Functor,
		Computed_field_connected_threshold_image_filter >
		(this);
#endif
}

int Computed_field_connected_threshold_image_filter::list()
/*******************************************************************************
LAST MODIFIED : 16 July 2007

DESCRIPTION :
==============================================================================*/
{
	int return_code = 0;
	int i;

	ENTER(List_Computed_field_connected_threshold_image_filter);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    source field : %s\n",field->source_fields[0]->name);
		display_message(INFORMATION_MESSAGE,
			"    lower_threshold : %g\n", lower_threshold);
		display_message(INFORMATION_MESSAGE,
			"    upper_threshold : %g\n", upper_threshold);
		display_message(INFORMATION_MESSAGE,
			"    replace_value : %g\n", replace_value);
		display_message(INFORMATION_MESSAGE,
			"    num_seed_points : %d\n", num_seed_points);
		display_message(INFORMATION_MESSAGE,
			"    dimension : %d\n", dimension);
		display_message(INFORMATION_MESSAGE,"    seed_points :");

		for (i = 0; i < dimension*num_seed_points; i++) {
			display_message(INFORMATION_MESSAGE," %g",seed_points[i]);
		}
		display_message(INFORMATION_MESSAGE,"\n");

	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_connected_threshold_image_filter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_connected_threshold_image_filter */

char *Computed_field_connected_threshold_image_filter::get_command_string()
/*******************************************************************************
LAST MODIFIED : 16 July 2007

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name, temp_string[40];
	int error;
	int i;


	ENTER(Computed_field_connected_threshold_image_filter::get_command_string);
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
		sprintf(temp_string, " replace_value %g", replace_value);
		append_string(&command_string, temp_string, &error);
		sprintf(temp_string, " num_seed_points %d", num_seed_points);
		append_string(&command_string, temp_string, &error);
		sprintf(temp_string, " dimension %d", dimension);
		append_string(&command_string, temp_string, &error);
		append_string(&command_string, " seed_points", &error);
		for (i = 0; i < dimension*num_seed_points; i++)
		{
			sprintf(temp_string, " %g", seed_points[i]);
			append_string(&command_string, temp_string, &error);
		}

}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_connected_threshold_image_filter::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_connected_threshold_image_filter::get_command_string */

} //namespace

cmzn_field_id cmzn_fieldmodule_create_field_imagefilter_connected_threshold(
	cmzn_fieldmodule_id field_module, cmzn_field_id source_field,
	double lower_threshold, double upper_threshold, double replace_value,
	int num_seed_points, int dimension, const double *seed_points)
{
	cmzn_field *field = NULL;
	USE_PARAMETER(dimension);
	if (source_field && Computed_field_is_scalar(source_field, (void *)NULL))
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field->number_of_components,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_connected_threshold_image_filter(source_field, lower_threshold,
				upper_threshold, replace_value, num_seed_points, seed_points));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_imagefilter_connected_threshold.  Invalid argument(s)");
	}

	return (field);
}

int cmzn_field_get_type_connected_threshold_image_filter(struct Computed_field *field,
  struct Computed_field **source_field, double *lower_threshold, double *upper_threshold,
	  double *replace_value, int *num_seed_points, int *seed_dimension, double **seed_points)
/*******************************************************************************
LAST MODIFIED : 16 July 2007

DESCRIPTION :
If the field is of type COMPUTED_FIELD_CONNECTED_THRESHOLD_IMAGE_FILTER,
the source_field and thresholds used by it are returned -
otherwise an error is reported.
==============================================================================*/
{
	Computed_field_connected_threshold_image_filter* core;
	int return_code;
	int i, seed_points_length;


	ENTER(cmzn_field_get_type_connected_threshold_image_filter);
	if (field && (core = dynamic_cast<Computed_field_connected_threshold_image_filter*>(field->core))
		&& source_field)
	{
		*source_field = field->source_fields[0];
		*lower_threshold = core->lower_threshold;
		*upper_threshold = core->upper_threshold;
		*replace_value = core->replace_value;
		*num_seed_points = core->num_seed_points;
		*seed_dimension = core->dimension;

		seed_points_length = core->dimension * core->num_seed_points;
		ALLOCATE(*seed_points, double, seed_points_length);
		for (i=0; i < seed_points_length;i++) {
			(*seed_points)[i]=core->seed_points[i];
		}

		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_field_get_type_connected_threshold_image_filter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_field_get_type_connected_threshold_image_filter */

