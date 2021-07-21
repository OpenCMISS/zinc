/*******************************************************************************
FILE : computed_field_canny_edge_detection_image_filter.c

LAST MODIFIED : 9 September 2006

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
#include "image_processing/computed_field_canny_edge_detection_filter.h"
#include "itkImage.h"
#include "itkVector.h"
#include "itkCannyEdgeDetectionImageFilter.h"

using namespace CMZN;

namespace {

char computed_field_canny_edge_detection_image_filter_type_string[] = "canny_edge_detection_filter";

class Computed_field_canny_edge_detection_image_filter : public computed_field_image_filter
{

public:

  double variance;
  double maximumError;
  double upperThreshold;
  double lowerThreshold;
  //  double outsideValue;

  Computed_field_canny_edge_detection_image_filter(Computed_field *source_field,
  	double variance,  double maximumError, double upperThreshold, double lowerThreshold);

	~Computed_field_canny_edge_detection_image_filter()
	{
	}

private:
	virtual void create_functor();

	Computed_field_core *copy()
	{
	  return new Computed_field_canny_edge_detection_image_filter(field->source_fields[0],
	  	variance, maximumError, upperThreshold, lowerThreshold);
	}

	const char *get_type_string()
	{
		return(computed_field_canny_edge_detection_image_filter_type_string);
	}

	int compare(Computed_field_core* other_field);

	int list();

	char* get_command_string();
};

int Computed_field_canny_edge_detection_image_filter::compare(Computed_field_core *other_core)
/*******************************************************************************
LAST MODIFIED : 18 October 2006

DESCRIPTION :
Compare the type specific data.
==============================================================================*/
{
	Computed_field_canny_edge_detection_image_filter* other;
	int return_code;

	ENTER(Computed_field_canny_edge_detection_image_filter::compare);

	// check field exists and can be cast to correct type
	if (field && (other = dynamic_cast<Computed_field_canny_edge_detection_image_filter*>(other_core))) {
		if( variance == other->variance &&
		    maximumError == other->maximumError &&
		    upperThreshold == other->upperThreshold &&
		    lowerThreshold == other->lowerThreshold) {
		  return_code = 1;  // parameters match, field is the same
		} else {
			return_code = 0;
		}
	} else {
		return_code = 0;		
	}

	LEAVE;

	return (return_code);
} /* Computed_field_canny_edge_detection_image_filter::compare */

int Computed_field_canny_edge_detection_image_filter::list()
/*******************************************************************************
LAST MODIFIED : 30 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code = 0;

	ENTER(List_Computed_field_canny_edge_detection_image_filter);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    source field : %s\n",field->source_fields[0]->name);
		display_message(INFORMATION_MESSAGE,
			"    variance : %g\n", variance);
		display_message(INFORMATION_MESSAGE,
			"    maximum_error : %g\n", maximumError);
		display_message(INFORMATION_MESSAGE,
			"    upper_threshold : %g\n", upperThreshold);
		display_message(INFORMATION_MESSAGE,
			"    lower_threshold : %g\n", lowerThreshold);

	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_canny_edge_detection_image_filter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_canny_edge_detection_image_filter */

char *Computed_field_canny_edge_detection_image_filter::get_command_string()
/*******************************************************************************
LAST MODIFIED : 30 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name, temp_string[40];
	int error;

	ENTER(Computed_field_canny_edge_detection_image_filter::get_command_string);
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
		sprintf(temp_string, " variance %g", variance);
		append_string(&command_string, temp_string, &error);		
		sprintf(temp_string, " maximum_error %g", maximumError);
		append_string(&command_string, temp_string, &error);		
		sprintf(temp_string, " upper_threshold %g", upperThreshold);
		append_string(&command_string, temp_string, &error);		
		sprintf(temp_string, " lower_threshold %g", lowerThreshold);
		append_string(&command_string, temp_string, &error);		

	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_canny_edge_detection_image_filter::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_canny_edge_detection_image_filter::get_command_string */

template < class ImageType >
class Computed_field_canny_edge_detection_image_filter_Functor :
	public computed_field_image_filter_FunctorTmpl< ImageType >
/*******************************************************************************
LAST MODIFIED : 12 September 2006

DESCRIPTION :
This class actually does the work of processing images with the filter.
It is instantiated for each of the chosen ImageTypes.
==============================================================================*/
{
	Computed_field_canny_edge_detection_image_filter *canny_edge_detection_image_filter;

public:

	Computed_field_canny_edge_detection_image_filter_Functor(
		Computed_field_canny_edge_detection_image_filter *canny_edge_detection_image_filter) :
		computed_field_image_filter_FunctorTmpl< ImageType >(canny_edge_detection_image_filter),
		canny_edge_detection_image_filter(canny_edge_detection_image_filter)
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
		
		typedef itk::CannyEdgeDetectionImageFilter< ImageType , ImageType > FilterType;
		typedef typename ImageType::PixelType  OutputImagePixelType;
		OutputImagePixelType upper;
		OutputImagePixelType lower;

		typename FilterType::Pointer filter = FilterType::New();
		
		// check whether we need to scale these values from
		// 0-1 to 0-255
		filter->SetVariance( (ZnReal)(canny_edge_detection_image_filter->variance) );
		filter->SetMaximumError( (ZnReal)(canny_edge_detection_image_filter->maximumError) );

		// wrong currently!
		upper = canny_edge_detection_image_filter->upperThreshold;
		filter->SetUpperThreshold(upper);
		lower = canny_edge_detection_image_filter->lowerThreshold;
		filter->SetLowerThreshold(lower);
	
		return_code = canny_edge_detection_image_filter->update_output_image
			(cache, filter, this->outputImage,
			static_cast<ImageType*>(NULL), static_cast<FilterType*>(NULL));
	  
		return (return_code);
	} /* set_filter */

}; /* template < class ImageType > class Computed_field_canny_edge_detection_image_filter_Functor */
	
Computed_field_canny_edge_detection_image_filter::Computed_field_canny_edge_detection_image_filter(
	Computed_field *source_field, double variance, double maximumError,
	double upperThreshold, double lowerThreshold) :
  computed_field_image_filter(source_field), variance(variance), maximumError(maximumError),
  upperThreshold(upperThreshold), lowerThreshold(lowerThreshold)
/*******************************************************************************
LAST MODIFIED : 12 September 2006

DESCRIPTION :
Create the computed_field representation of the CannyEdgeDetectionFilter.
==============================================================================*/
{
} /* Computed_field_canny_edge_detection_image_filter */

void Computed_field_canny_edge_detection_image_filter::create_functor()
{
#if defined DONOTUSE_TEMPLATETEMPLATES
	create_filters_singlecomponent_twoormoredimensions(
		Computed_field_canny_edge_detection_image_filter_Functor, this);
#else
	create_filters_singlecomponent_twoormoredimensions
		< Computed_field_canny_edge_detection_image_filter_Functor,
		Computed_field_canny_edge_detection_image_filter >
		(this);
#endif
}

} //namespace

cmzn_field_id cmzn_fieldmodule_create_field_imagefilter_canny_edge_detection(
	cmzn_fieldmodule_id field_module, cmzn_field_id source_field,
	double variance, double maximumError,
  double upperThreshold, double lowerThreshold)
{
	cmzn_field *field = NULL;
	if (source_field && Computed_field_is_scalar(source_field, (void *)NULL))
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field->number_of_components,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_canny_edge_detection_image_filter(source_field,
				variance, maximumError, upperThreshold, lowerThreshold));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_imagefilter_canny_edge_detection.  Invalid argument(s)");
	}

	return (field);
}

int cmzn_field_get_type_canny_edge_detection_image_filter(struct Computed_field *field,
      struct Computed_field **source_field, double *variance, double *maximumError,
      double *upperThreshold, double *lowerThreshold)
/*******************************************************************************
LAST MODIFIED : 30 August 2006

DESCRIPTION :
If the field is of type computed_field_canny_edge_detection_filter, the source_field
used by it is returned - otherwise an error is reported.
==============================================================================*/
{
	Computed_field_canny_edge_detection_image_filter* core;
	int return_code;

	ENTER(cmzn_field_get_type_canny_edge_detection_image_filter);
	if (field && (core = dynamic_cast<Computed_field_canny_edge_detection_image_filter*>(field->core))
		&& source_field)
	{
		*source_field = field->source_fields[0];
		*variance = core->variance;
		*maximumError = core->maximumError;
		*upperThreshold = core->upperThreshold;
		*lowerThreshold = core->lowerThreshold;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_field_get_type_canny_edge_detection_image_filter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_field_get_type_canny_edge_detection_image_filter */

