/*******************************************************************************
FILE : computed_field_derivative_image_filter.c

LAST MODIFIED : 18 Nov 2006

DESCRIPTION :
Wraps itk::DerivativeImageFilter
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
#include "image_processing/computed_field_derivative_image_filter.h"
#include "itkImage.h"
#include "itkVector.h"
#include "itkDerivativeImageFilter.h"

using namespace CMZN;

namespace {

char computed_field_derivative_image_filter_type_string[] = "derivative_filter";

class Computed_field_derivative_image_filter : public computed_field_image_filter
{

public:
	int order;
	int direction;
       

	Computed_field_derivative_image_filter(Computed_field *source_field,
		int order, int direction);

	~Computed_field_derivative_image_filter()
	{
	};

private:
	virtual void create_functor();

	Computed_field_core *copy()
	{
		return new Computed_field_derivative_image_filter(field->source_fields[0],
			order, direction);
	}

	const char *get_type_string()
	{
		return(computed_field_derivative_image_filter_type_string);
	}

	int compare(Computed_field_core* other_field);

	int list();

	char* get_command_string();
};

int Computed_field_derivative_image_filter::compare(Computed_field_core *other_core)
/*******************************************************************************
LAST MODIFIED : 18 October 2006

DESCRIPTION :
Compare the type specific data.
==============================================================================*/
{
	Computed_field_derivative_image_filter* other;
	int return_code;

	ENTER(Computed_field_derivative_image_filter::compare);
	if (field && (other = dynamic_cast<Computed_field_derivative_image_filter*>(other_core)))
	{
		if ((dimension == other->dimension)
		        && (order == other->order)
			&& (direction == other->direction))
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
} /* Computed_field_derivative_image_filter::compare */

int Computed_field_derivative_image_filter::list()
/*******************************************************************************
LAST MODIFIED : 18 October 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code = 0;

	ENTER(List_Computed_field_derivative_image_filter);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    source field : %s\n",field->source_fields[0]->name);
		display_message(INFORMATION_MESSAGE,
			"    filter order : %d\n", order);
		display_message(INFORMATION_MESSAGE,
			"    filter direction : %d\n", direction);

	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_derivative_image_filter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_derivative_image_filter */

char *Computed_field_derivative_image_filter::get_command_string()
/*******************************************************************************
LAST MODIFIED : 18 October 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name, temp_string[40];
	int error;

	ENTER(Computed_field_derivative_image_filter::get_command_string);
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
		sprintf(temp_string, " order %d", order);
		sprintf(temp_string, " direction %d", direction);
		append_string(&command_string, temp_string, &error);	
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_derivative_image_filter::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_derivative_image_filter::get_command_string */

template < class ImageType >
class Computed_field_derivative_image_filter_Functor :
	public computed_field_image_filter_FunctorTmpl< ImageType >
/*******************************************************************************
LAST MODIFIED : 12 September 2006

DESCRIPTION :
This class actually does the work of processing images with the filter.
It is instantiated for each of the chosen ImageTypes.
==============================================================================*/
{
	Computed_field_derivative_image_filter *derivative_image_filter;

public:

	Computed_field_derivative_image_filter_Functor(
		Computed_field_derivative_image_filter *derivative_image_filter) :
		computed_field_image_filter_FunctorTmpl< ImageType >(derivative_image_filter),
		derivative_image_filter(derivative_image_filter)
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

		typedef itk::DerivativeImageFilter< ImageType , ImageType > FilterType;
		
		typename FilterType::Pointer filter = FilterType::New();

		filter->SetOrder( derivative_image_filter->order );
		filter->SetDirection( derivative_image_filter->direction );
		
		return_code = derivative_image_filter->update_output_image
			(cache, filter, this->outputImage,
			static_cast<ImageType*>(NULL), static_cast<FilterType*>(NULL));
		
		return (return_code);
	} /* set_filter */

}; /* template < class ImageType > class Computed_field_derivative_image_filter_Functor */

Computed_field_derivative_image_filter::Computed_field_derivative_image_filter(
	Computed_field *source_field, int order, int direction) : 
	computed_field_image_filter(source_field), 
	order(order), direction(direction)
/*******************************************************************************
LAST MODIFIED : 12 September 2006

DESCRIPTION :
Create the computed_field representation of the DerivativeImageFilter.
==============================================================================*/
{
}

void Computed_field_derivative_image_filter::create_functor()
{
#if defined DONOTUSE_TEMPLATETEMPLATES
	create_filters_singlecomponent_multidimensions(
		Computed_field_derivative_image_filter_Functor, this);
#else
	create_filters_singlecomponent_multidimensions
		< Computed_field_derivative_image_filter_Functor, Computed_field_derivative_image_filter >
		(this);
#endif
}

} //namespace

struct Computed_field *cmzn_fieldmodule_create_field_imagefilter_derivative(
	struct cmzn_fieldmodule *field_module,
	struct Computed_field *source_field, int order, int direction)
{
	Computed_field *field = NULL;
	if (source_field && Computed_field_is_scalar(source_field, (void *)NULL))
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field->number_of_components,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_derivative_image_filter(source_field, order, direction));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_imagefilter_derivative.  Invalid argument(s)");
	}

	return (field);
}

int cmzn_field_get_type_derivative_image_filter(struct Computed_field *field,
	struct Computed_field **source_field, int *order, int *direction)
/*******************************************************************************
LAST MODIFIED : 18 October 2006

DESCRIPTION :
If the field is of type DERIVATIVE, the source_field and derivative_image_filter
used by it are returned - otherwise an error is reported.
==============================================================================*/
{
	Computed_field_derivative_image_filter* core;
	int return_code;

	ENTER(cmzn_field_get_type_derivative_image_filter);
	if (field && (core = dynamic_cast<Computed_field_derivative_image_filter*>(field->core))
		&& source_field)
	{
		*source_field = field->source_fields[0];
		*order = core->order;
		*direction=core->direction;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_field_get_type_derivative_image_filter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_field_get_type_derivative_image_filter */

