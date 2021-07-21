/*******************************************************************************
FILE : computed_field_threshold_image_filter.cpp

LAST MODIFIED : 26 September 2008

DESCRIPTION :
Wraps itk::ThresholdImageFilter

This enables the use of itk to do general thresholding.  The threshold filter
can be used in three different ways.
- specify one threshold value.  All pixels BELOW this value are set to a
  specified outside value
- specify one threshold value.  All pixels ABOVE this value are set to a
  specified outside value
- specify two threshold values.  All pixels OUTSIDE the range defined by the
  threshold values are set to a specified outside value

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
/* cannot use enumerator_private.h with c++ compiler, use cpp version instead
	eventually should replace enumerator macros with a template */
#include "general/enumerator_private.hpp"
#include "general/mystring.h"
#include "general/message.h"
#include "image_processing/computed_field_threshold_image_filter.h"
#include "itkImage.h"
#include "itkVector.h"
#include "itkThresholdImageFilter.h"

using namespace CMZN;

PROTOTYPE_ENUMERATOR_STRING_FUNCTION(cmzn_field_imagefilter_threshold_condition)
{
	const char *enumerator_string;

	ENTER(ENUMERATOR_STRING(cmzn_field_imagefilter_threshold_condition));
	switch (enumerator_value)
	{
		case CMZN_FIELD_IMAGEFILTER_THRESHOLD_CONDITION_BELOW:
		{
			enumerator_string = "below";
		} break;
		case CMZN_FIELD_IMAGEFILTER_THRESHOLD_CONDITION_ABOVE:
		{
			enumerator_string = "above";
		} break;
		case CMZN_FIELD_IMAGEFILTER_THRESHOLD_CONDITION_OUTSIDE:
		{
			enumerator_string = "outside";
		} break;
		default:
		{
			enumerator_string = (const char *)NULL;
		} break;
	}
	LEAVE;

	return (enumerator_string);
} /* ENUMERATOR_STRING(cmzn_field_imagefilter_threshold_condition) */

DEFINE_DEFAULT_ENUMERATOR_FUNCTIONS(cmzn_field_imagefilter_threshold_condition)

namespace {

	char computed_field_threshold_image_filter_type_string[] = "threshold_filter";

	class Computed_field_threshold_image_filter : public computed_field_image_filter
	{

	public:
		/* to specify the threshold filter we use an enumerated type that
			can take values ABOVE, BELOW or OUTSIDE */
		enum cmzn_field_imagefilter_threshold_condition condition;

		double outsideValue; // used by all modes
		double lowerValue;   // needed for both below and outside mode
		double upperValue;   // needed for both above and outside mode

		Computed_field_threshold_image_filter(Computed_field *source_field,
			enum cmzn_field_imagefilter_threshold_condition condition = CMZN_FIELD_IMAGEFILTER_THRESHOLD_CONDITION_BELOW,
			double outsideValue = 0.0, double lowerValue = 0.5, double upperValue = 0.5);

		~Computed_field_threshold_image_filter()
		{
		}

		enum cmzn_field_imagefilter_threshold_condition getMode()
		{
			return condition;
		}

		int setCondition(enum cmzn_field_imagefilter_threshold_condition conditionIn)
		{
			if (condition != conditionIn)
			{
				condition = conditionIn;
				clear_cache();
			}
			return CMZN_OK;
		}

		double getOutsideValue()
		{
			return outsideValue;
		}

		int setOutsideValue(double outsideValueIn)
		{
			if (outsideValue != outsideValueIn)
			{
				outsideValue = outsideValueIn;
				clear_cache();
			}
			return CMZN_OK;
		}

		double getLowerThreshold()
		{
			return lowerValue;
		}

		int setLowerThreshold(double lowerValueIn)
		{
			if (lowerValue != lowerValueIn)
			{
				lowerValue = lowerValueIn;
				clear_cache();
			}
			return CMZN_OK;
		}

		double getUpperThreshold()
		{
			return upperValue;
		}

		int setUpperThreshold(double upperValueIn)
		{
			if (upperValue != upperValueIn)
			{
				upperValue = upperValueIn;
				clear_cache();
			}
			return CMZN_OK;
		}

	private:
		virtual void create_functor();

		Computed_field_core *copy()
		{
			return new Computed_field_threshold_image_filter(field->source_fields[0],
				condition, outsideValue, lowerValue, upperValue);
		}

		const char *get_type_string()
		{
			return(computed_field_threshold_image_filter_type_string);
		}

		int compare(Computed_field_core* other_field);

		int list();

		char* get_command_string();
	};

	inline Computed_field_threshold_image_filter *
		Computed_field_threshold_image_filter_core_cast(
			cmzn_field_imagefilter_threshold *imagefilter_threshold)
	{
		return (static_cast<Computed_field_threshold_image_filter*>(
			reinterpret_cast<Computed_field*>(imagefilter_threshold)->core));
	}

	int Computed_field_threshold_image_filter::compare(Computed_field_core *other_core)
/*******************************************************************************
LAST MODIFIED : 8 December 2006

DESCRIPTION :
Compare the type specific data.
==============================================================================*/
	{
		Computed_field_threshold_image_filter* other;
		int return_code;

		ENTER(Computed_field_threshold_image_filter::compare);
		if (field && (other = dynamic_cast<Computed_field_threshold_image_filter*>(other_core)))
		{

			/* could get trickier here and check only the relevant above or below
				values depending on filter type.  eg for an above filter,
				the below value is irrelevant and could be ignored.
				Currently we just use a direct comparison of all set variables. */
			if ((dimension == other->dimension)
				&& (condition == other->condition)
				&& (outsideValue == other->outsideValue)
				&& (lowerValue == other->lowerValue)
				&& (upperValue == other->upperValue))
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
	} /* Computed_field_threshold_image_filter::compare */

	template < class ImageType >
	class Computed_field_threshold_image_filter_Functor :
		public computed_field_image_filter_FunctorTmpl< ImageType >
/*******************************************************************************
LAST MODIFIED : 12 September 2006

DESCRIPTION :
This class actually does the work of processing images with the filter.
It is instantiated for each of the chosen ImageTypes.
==============================================================================*/
	{
		Computed_field_threshold_image_filter *threshold_image_filter;

	public:

		Computed_field_threshold_image_filter_Functor(
			Computed_field_threshold_image_filter *threshold_image_filter) :
			computed_field_image_filter_FunctorTmpl< ImageType >(threshold_image_filter),
			threshold_image_filter(threshold_image_filter)
		{
		}

		int set_filter(cmzn_fieldcache& cache)
/*******************************************************************************
LAST MODIFIED : 8 December 2006

DESCRIPTION :
Create a filter of the correct type, set the filter specific parameters
and generate the outputImage.
==============================================================================*/
		{
			int return_code;

			typedef itk::ThresholdImageFilter< ImageType > FilterType;

			typename FilterType::Pointer filter = FilterType::New();

			filter->SetOutsideValue( threshold_image_filter->outsideValue );

			// call appropriate threshold mode based on condition variable

			switch (threshold_image_filter->condition)
			{
				case CMZN_FIELD_IMAGEFILTER_THRESHOLD_CONDITION_BELOW:
				{
					filter->ThresholdBelow( threshold_image_filter->lowerValue );
				} break;
				case CMZN_FIELD_IMAGEFILTER_THRESHOLD_CONDITION_ABOVE:
				{
					filter->ThresholdAbove( threshold_image_filter->upperValue );
				} break;
				case CMZN_FIELD_IMAGEFILTER_THRESHOLD_CONDITION_OUTSIDE:
				{
					filter->ThresholdOutside( threshold_image_filter->lowerValue, threshold_image_filter->upperValue );
				} break;
				default:
				{
					display_message(ERROR_MESSAGE,
						"Unknown threshold mode");
				} break;
			}

			return_code = threshold_image_filter->update_output_image
				(cache, filter, this->outputImage,
				 static_cast<ImageType*>(NULL),
				 static_cast<FilterType*>(NULL));

			return (return_code);
		} /* set_filter */

	}; /* template < class ImageType > class Computed_field_threshold_image_filter_Functor */

	Computed_field_threshold_image_filter::Computed_field_threshold_image_filter(
		Computed_field *source_field,
		enum cmzn_field_imagefilter_threshold_condition condition,
		double outsideValue, double lowerValue, double upperValue) :
		computed_field_image_filter(source_field),
		condition(condition), outsideValue(outsideValue),
		lowerValue(lowerValue),upperValue(upperValue)
/*******************************************************************************
LAST MODIFIED : 12 September 2006

DESCRIPTION :
Create the computed_field representation of the threshold_image_filter.
==============================================================================*/
	{
	}

	void Computed_field_threshold_image_filter::create_functor()
	{
#if defined DONOTUSE_TEMPLATETEMPLATES
		create_filters_singlecomponent_multidimensions(
			Computed_field_threshold_image_filter_Functor, this);
#else
		create_filters_singlecomponent_multidimensions
			< Computed_field_threshold_image_filter_Functor,
			Computed_field_threshold_image_filter >
			(this);
#endif
	}

	int Computed_field_threshold_image_filter::list()
/*******************************************************************************
LAST MODIFIED : 8 December 2006

DESCRIPTION : list out the threshold filter options
==============================================================================*/
	{
		int return_code = 0;

		ENTER(List_Computed_field_threshold_image_filter);
		if (field)
		{
			display_message(INFORMATION_MESSAGE,
				"    source field : %s\n",field->source_fields[0]->name);

			display_message(INFORMATION_MESSAGE,
			"    condition: %s\n", ENUMERATOR_STRING(cmzn_field_imagefilter_threshold_condition)(condition));
			display_message(INFORMATION_MESSAGE,
				"    outside value : %g\n", outsideValue);

			switch (condition)
			{
				case CMZN_FIELD_IMAGEFILTER_THRESHOLD_CONDITION_BELOW:
				{
					display_message(INFORMATION_MESSAGE,
						"    lower value : %g\n", lowerValue);
				} break;
				case CMZN_FIELD_IMAGEFILTER_THRESHOLD_CONDITION_ABOVE:
				{
					display_message(INFORMATION_MESSAGE,
						"    upper value : %g\n", upperValue);
				} break;
				case CMZN_FIELD_IMAGEFILTER_THRESHOLD_CONDITION_OUTSIDE:
				{
					display_message(INFORMATION_MESSAGE,
						"    lower value : %g\n", lowerValue);
					display_message(INFORMATION_MESSAGE,
						"    upper value : %g\n", upperValue);
				} break;
				case CMZN_FIELD_IMAGEFILTER_THRESHOLD_CONDITION_INVALID:
				{
				} break;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"list_Computed_field_threshold_image_filter.  Invalid argument(s)");
			return_code = 0;
		}
		LEAVE;

		return (return_code);
	} /* list_Computed_field_threshold_image_filter */

	char *Computed_field_threshold_image_filter::get_command_string()
/*******************************************************************************
LAST MODIFIED : 30 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
	{
		char *command_string, *field_name, temp_string[40];
		int error;

		ENTER(Computed_field_threshold_image_filter::get_command_string);
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
			sprintf(temp_string, " %s outside_value %g",
				ENUMERATOR_STRING(cmzn_field_imagefilter_threshold_condition)(condition), outsideValue);
			append_string(&command_string, temp_string, &error);

			switch (condition)
			{
				case CMZN_FIELD_IMAGEFILTER_THRESHOLD_CONDITION_BELOW:
				{
					sprintf(temp_string, " below_value %g", lowerValue);
				} break;
				case CMZN_FIELD_IMAGEFILTER_THRESHOLD_CONDITION_ABOVE:
				{
					sprintf(temp_string, " above_value %g", upperValue);
				} break;
				case CMZN_FIELD_IMAGEFILTER_THRESHOLD_CONDITION_OUTSIDE:
				{
					sprintf(temp_string, " below_value %g above_value %g",
						lowerValue, upperValue);
				} break;
				case CMZN_FIELD_IMAGEFILTER_THRESHOLD_CONDITION_INVALID:
				{
				} break;
			}
			append_string(&command_string, temp_string, &error);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Computed_field_threshold_image_filter::get_command_string.  Invalid field");
		}
		LEAVE;

		return (command_string);
	} /* Computed_field_threshold_image_filter::get_command_string */

} //namespace


cmzn_field_imagefilter_threshold_id cmzn_field_cast_imagefilter_threshold(cmzn_field_id field)
{
	if (dynamic_cast<Computed_field_threshold_image_filter*>(field->core))
	{
		cmzn_field_access(field);
		return (reinterpret_cast<cmzn_field_imagefilter_threshold_id>(field));
	}
	else
	{
		return (NULL);
	}
}

int cmzn_field_imagefilter_threshold_destroy(
	cmzn_field_imagefilter_threshold_id *imagefilter_threshold_address)
{
	return cmzn_field_destroy(reinterpret_cast<cmzn_field_id *>(
		imagefilter_threshold_address));
}

enum cmzn_field_imagefilter_threshold_condition cmzn_field_imagefilter_threshold_get_condition(
	cmzn_field_imagefilter_threshold_id imagefilter_threshold)
{
	if (imagefilter_threshold)
	{
		Computed_field_threshold_image_filter *filter_core =
			Computed_field_threshold_image_filter_core_cast(imagefilter_threshold);
		return filter_core->getMode();
	}
	return CMZN_FIELD_IMAGEFILTER_THRESHOLD_CONDITION_BELOW;
}

int cmzn_field_imagefilter_threshold_set_condition(cmzn_field_imagefilter_threshold_id
	imagefilter_threshold, enum cmzn_field_imagefilter_threshold_condition condition)
{
	if (imagefilter_threshold)
	{
		Computed_field_threshold_image_filter *filter_core =
			Computed_field_threshold_image_filter_core_cast(imagefilter_threshold);
		return filter_core->setCondition(condition);
	}
	return CMZN_ERROR_ARGUMENT;
}

double cmzn_field_imagefilter_threshold_get_outside_value(
	cmzn_field_imagefilter_threshold_id imagefilter_threshold)
{
	if (imagefilter_threshold)
	{
		Computed_field_threshold_image_filter *filter_core =
			Computed_field_threshold_image_filter_core_cast(imagefilter_threshold);
		return filter_core->getOutsideValue();
	}
	return 0.0;
}

int cmzn_field_imagefilter_threshold_set_outside_value(cmzn_field_imagefilter_threshold_id
	imagefilter_threshold, double outside_value)
{
	if (imagefilter_threshold)
	{
		Computed_field_threshold_image_filter *filter_core =
			Computed_field_threshold_image_filter_core_cast(imagefilter_threshold);
		return filter_core->setOutsideValue(outside_value);
	}
	return CMZN_ERROR_ARGUMENT;
}

double cmzn_field_imagefilter_threshold_get_lower_threshold(
	cmzn_field_imagefilter_threshold_id imagefilter_threshold)
{
	if (imagefilter_threshold)
	{
		Computed_field_threshold_image_filter *filter_core =
			Computed_field_threshold_image_filter_core_cast(imagefilter_threshold);
		return filter_core->getLowerThreshold();
	}
	return 0.0;
}

int cmzn_field_imagefilter_threshold_set_lower_threshold(
	cmzn_field_imagefilter_threshold_id imagefilter_threshold, double lower_value)
{
	if (imagefilter_threshold)
	{
		Computed_field_threshold_image_filter *filter_core =
			Computed_field_threshold_image_filter_core_cast(imagefilter_threshold);
		return filter_core->setLowerThreshold(lower_value);
	}
	return CMZN_ERROR_ARGUMENT;
}

double cmzn_field_imagefilter_threshold_get_upper_threshold(
	cmzn_field_imagefilter_threshold_id imagefilter_threshold)
{
	if (imagefilter_threshold)
	{
		Computed_field_threshold_image_filter *filter_core =
			Computed_field_threshold_image_filter_core_cast(imagefilter_threshold);
		return filter_core->getUpperThreshold();
	}
	return 0.0;
}

int cmzn_field_imagefilter_threshold_set_upper_threshold(cmzn_field_imagefilter_threshold_id
	imagefilter_threshold, double upper_value)
{
	if (imagefilter_threshold)
	{
		Computed_field_threshold_image_filter *filter_core =
			Computed_field_threshold_image_filter_core_cast(imagefilter_threshold);
		return filter_core->setUpperThreshold(upper_value);
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_field_id cmzn_fieldmodule_create_field_imagefilter_threshold(
	cmzn_fieldmodule_id field_module, cmzn_field_id source_field)
{
	cmzn_field *field = NULL;
	if (source_field && Computed_field_is_scalar(source_field, (void *)NULL))
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field->number_of_components,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_threshold_image_filter(source_field));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_imagefilter_threshold.  Invalid argument(s)");
	}

	return (field);
}

int cmzn_field_get_type_threshold_image_filter(struct Computed_field *field,
	struct Computed_field **source_field,
	enum cmzn_field_imagefilter_threshold_condition *condition,
	double *outsideValue, double *lowerValue,	double *upperValue)
/*******************************************************************************
LAST MODIFIED : 8 December 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_FILTER,
the source_field and thresholds used by it are returned -
otherwise an error is reported.
==============================================================================*/
{
	Computed_field_threshold_image_filter* core;
	int return_code;

	ENTER(cmzn_field_get_type_threshold_image_filter);
	if (field && (core = dynamic_cast<Computed_field_threshold_image_filter*>(field->core))
		&& source_field)
	{
		*source_field = field->source_fields[0];
		*condition = core->condition;
		*outsideValue = core->outsideValue;
		*lowerValue = core->lowerValue;
		*upperValue = core->upperValue;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_field_get_type_threshold_image_filter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_field_get_type_threshold_image_filter */

