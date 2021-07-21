/*******************************************************************************
FILE : computed_field_image_resample.cpp

LAST MODIFIED : 7 March 2007

DESCRIPTION :
Field that changes the native resolution of a computed field.
Image processing fields use the native resolution to determine their image size.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "opencmiss/zinc/fieldimageprocessing.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_private.hpp"
#include "computed_field/computed_field_set.h"
#include "finite_element/finite_element_region.h"
#include "region/cmiss_region.hpp"
#include "general/debug.h"
#include "general/mystring.h"
#include "general/message.h"
#include "image_processing/computed_field_image_resample.h"

namespace {

	const char computed_field_image_resample_type_string[] = "image_resample";

	class Computed_field_image_resample : public Computed_field_core
	{

	public:
		int dimension;  // Should match the dimension of the source field,
						// kept here just to help with iterating through array
		int *sizes; // Resolution in each direction of <dimension>
		double *lookup_coordinates_min, *lookup_coordinates_max;
		double *input_coordinates_min, *input_coordinates_max;
		double *lookup_coordinates_diff, *input_coordinates_sum;

		Computed_field_image_resample(int dimension, int *sizes_in) :
			Computed_field_core(), dimension(dimension)
		{
			int i;
			sizes = new int[dimension];
			for (i = 0 ; i < dimension ; i++)
			{
				sizes[i] = sizes_in[i];
			}
			lookup_coordinates_min = new double[dimension];
			for (i = 0 ; i < dimension ; i++)
			{
				lookup_coordinates_min[i] = 0.0;
			}
			lookup_coordinates_max = new double[dimension];
			for (i = 0 ; i < dimension ; i++)
			{
				lookup_coordinates_max[i] = 1.0;
			}
			lookup_coordinates_diff = new double[dimension];
			updateLookupCoordinatesDiff();
			input_coordinates_min = new double[dimension];
			for (i = 0 ; i < dimension ; i++)
			{
				input_coordinates_min[i] = 0.0;
			}
			input_coordinates_max = new double[dimension];
			for (i = 0 ; i < dimension ; i++)
			{
				input_coordinates_max[i] = 1.0;
			}
			input_coordinates_sum = new double[dimension];
			updateInputCoordinatesSum();
		}

		~Computed_field_image_resample()
		{
			if (sizes)
			{
				delete[] sizes;
			}
			if (lookup_coordinates_min)
			{
				delete[] lookup_coordinates_min;
			}
			if (lookup_coordinates_max)
			{
				delete[] lookup_coordinates_max;
			}
			if (lookup_coordinates_diff)
			{
				delete[] lookup_coordinates_diff;
			}
			if (input_coordinates_min)
			{
				delete[] input_coordinates_min;
			}
			if (input_coordinates_max)
			{
				delete[] input_coordinates_max;
			}
			if (input_coordinates_sum)
			{
				delete[] input_coordinates_sum;
			}
		}

		void updateLookupCoordinatesDiff()
		{
			for (int i = 0 ; i < dimension ; i++)
			{
				lookup_coordinates_diff[i] =
					lookup_coordinates_max[i] - lookup_coordinates_min[i];
			}
		}

		void updateInputCoordinatesSum()
		{
			for (int i = 0 ; i < dimension ; i++)
			{
				input_coordinates_sum[i] =
					input_coordinates_min[i] + input_coordinates_max[i];
			}
		}

		int set_lookup_coordinates_minimum(int numberOfValues, const double *lookupCoordinatesMinimumIn)
		{
			if (numberOfValues > 0 && lookupCoordinatesMinimumIn)
			{
				for (int i = 0; (i < numberOfValues) && (i < dimension); i++)
				{
					lookup_coordinates_min[i] = lookupCoordinatesMinimumIn[i];
				}
				updateLookupCoordinatesDiff();
				return CMZN_OK;
			}
			return CMZN_ERROR_ARGUMENT;
		}

		int set_lookup_coordinates_maximum(int numberOfValues, const double *lookupCoordinatesMaximumIn)
		{
			if (numberOfValues > 0 && lookupCoordinatesMaximumIn)
			{
				for (int i = 0; (i < numberOfValues) && (i < dimension); i++)
				{
					lookup_coordinates_max[i] = lookupCoordinatesMaximumIn[i];
				}
				updateLookupCoordinatesDiff();
				return CMZN_OK;
			}
			return CMZN_ERROR_ARGUMENT;
		}

		int set_input_coordinates_minimum(int numberOfValues, const double *inputCoordinatesMinimumIn)
		{
			if (numberOfValues > 0 && inputCoordinatesMinimumIn)
			{
				for (int i = 0; (i < numberOfValues) && (i < dimension); i++)
				{
					input_coordinates_min[i] = inputCoordinatesMinimumIn[i];
				}
				updateInputCoordinatesSum();
				return CMZN_OK;
			}
			return CMZN_ERROR_ARGUMENT;
		}

		int set_input_coordinates_maximum(int numberOfValues, const double *inputCoordinatesMaximumIn)
		{
			if (numberOfValues > 0 && inputCoordinatesMaximumIn)
			{
				for (int i = 0; (i < numberOfValues) && (i < dimension); i++)
				{
					input_coordinates_max[i] = inputCoordinatesMaximumIn[i];
				}
				updateInputCoordinatesSum();
				return CMZN_OK;
			}
			return CMZN_ERROR_ARGUMENT;
		}

		int get_lookup_coordinates_minimum(int numberOfValues, double *lookupCoordinatesMinimumOut)
		{
			if (numberOfValues > 0 && lookupCoordinatesMinimumOut)
			{
				for (int i = 0; (i < numberOfValues) && (i < dimension); i++)
				{
					lookupCoordinatesMinimumOut[i] = lookup_coordinates_min[i];
				}
				return dimension;
			}
			return CMZN_ERROR_ARGUMENT;
		}

		int get_lookup_coordinates_maximum(int numberOfValues, double *lookupCoordinatesMaximumOut)
		{
			if (numberOfValues > 0 && lookupCoordinatesMaximumOut)
			{
				for (int i = 0; (i < numberOfValues) && (i < dimension); i++)
				{
					lookupCoordinatesMaximumOut[i] = lookup_coordinates_max[i];
				}
				return dimension;
			}
			return CMZN_ERROR_ARGUMENT;
		}

		int get_input_coordinates_minimum(int numberOfValues, double *inputCoordinatesMinimumOut)
		{
			if (numberOfValues > 0 && inputCoordinatesMinimumOut)
			{
				for (int i = 0; (i < numberOfValues) && (i < dimension); i++)
				{
					inputCoordinatesMinimumOut[i] = input_coordinates_min[i];
				}
				return dimension;
			}
			return CMZN_ERROR_ARGUMENT;
		}

		int get_input_coordinates_maximum(int numberOfValues, double *inputCoordinatesMaximumOut)
		{
			if (numberOfValues > 0 && inputCoordinatesMaximumOut)
			{
				for (int i = 0; (i < numberOfValues) && (i < dimension); i++)
				{
					inputCoordinatesMaximumOut[i] = input_coordinates_min[i];
				}
				return dimension;
			}
			return CMZN_ERROR_ARGUMENT;
		}

	private:
		Computed_field_core *copy()
		{
			return new Computed_field_image_resample(dimension, sizes);
		}

		virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

		virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
		{
			return this->evaluateDerivativeFiniteDifference(cache, inValueCache, fieldDerivative);
		}

		const char *get_type_string()
		{
			return(computed_field_image_resample_type_string);
		}

		int compare(Computed_field_core* other_field);

		int list();

		char* get_command_string();

		int get_native_resolution(int *dimension,
			int **sizes, Computed_field **texture_coordinate_field);
	};

int Computed_field_image_resample::compare(Computed_field_core *other_core)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Compare the type specific data.
==============================================================================*/
{
	Computed_field_image_resample* other;
	int i, return_code = 0;

	ENTER(Computed_field_image_resample::compare);
	if (field && (other = dynamic_cast<Computed_field_image_resample*>(other_core)))
	{
		if (dimension == other->dimension)
		{
			return_code = 1;
			i = dimension;
			while (return_code && (i < dimension))
			{
				if (sizes[i] != other->sizes[i])
				{
					return_code = 0;
				}
				if (lookup_coordinates_min[i] != other->lookup_coordinates_min[i])
				{
					return_code = 0;
				}
				if (lookup_coordinates_max[i] != other->lookup_coordinates_max[i])
				{
					return_code = 0;
				}
				if (input_coordinates_min[i] != other->input_coordinates_min[i])
				{
					return_code = 0;
				}
				if (input_coordinates_max[i] != other->input_coordinates_max[i])
				{
					return_code = 0;
				}
				i++;
			}
		}
	}
	else
	{
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_image_resample::compare */

int Computed_field_image_resample::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
	const RealFieldValueCache *sourceCache = 0;
	const Field_location_field_values* coordinate_location = 0;
	if ((coordinate_location = cache.get_location_field_values()))
	{
		cmzn_fieldcache *field_cache = valueCache.getOrCreateSharedExtraCache(cache);
		if (!field_cache)
			return 0;
		const int numberOfValues= coordinate_location->get_number_of_values();
		const FE_value *cacheValues = coordinate_location->get_values();
		FE_value *pixelValues = new double[numberOfValues];  // GRC should avoid allocations here
		for (int i = 0 ; i < dimension; i++)
		{
			if (numberOfValues > i)
			{
				double cacheValue = cacheValues[i];
				if (cacheValue < input_coordinates_min[i])
				{
					cacheValue = input_coordinates_min[i];
				}
				else if (cacheValue > input_coordinates_max[i])
				{
					cacheValue = input_coordinates_max[i];
				}
				pixelValues[i] = lookup_coordinates_min[i] + (lookup_coordinates_diff[i] *
					(cacheValue - input_coordinates_min[i]) / input_coordinates_sum[i]);
			}
			else
			{
				pixelValues[i] = (lookup_coordinates_max[i] + lookup_coordinates_min[i]) / 2.0;
			}
		}
		if (numberOfValues > dimension)
		{
			for (int i = dimension ; i < numberOfValues; i++)
			{
				pixelValues[i] = cacheValues[i];
			}
		}
		int return_code = 0;
		field_cache->setFieldReal(coordinate_location->get_field(), numberOfValues, pixelValues);
		field_cache->setTime(cache.getTime());
		sourceCache = RealFieldValueCache::cast(getSourceField(0)->evaluate(*field_cache));
		if (sourceCache)
		{
			valueCache.copyValues(*sourceCache);
			return_code = 1;
		}
		delete[] pixelValues;
		return return_code;
	}
	else
	{
		sourceCache = RealFieldValueCache::cast(getSourceField(0)->evaluate(cache));
		if (sourceCache)
		{
			valueCache.copyValues(*sourceCache);
			return 1;
		}
	}
	return 0;
}

int Computed_field_image_resample::get_native_resolution(int *return_dimension,
	int **return_sizes, Computed_field **return_texture_coordinate_field)
/*******************************************************************************
LAST MODIFIED : 7 March 2007

DESCRIPTION :
==============================================================================*/
{
	int i, return_code, source_dimension, *source_sizes;

	ENTER(Computed_field_image_resample::get_native_resolution);
	if (field)
	{
		return_code = Computed_field_get_native_resolution(
			field->source_fields[0], &source_dimension, &source_sizes,
			return_texture_coordinate_field);
		if (dimension == source_dimension)
		{
			*return_dimension = dimension;
			// The source sizes is allocated and now ours to pass back
			for (i = 0 ; i < dimension ; i++)
			{
				source_sizes[i] = sizes[i];
			}
			*return_sizes = source_sizes;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Computed_field_image_resample::get_native_resolution.  "
				"Source dimension and field dimension do not match.");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_image_resample::get_native_resolution.  Missing field");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_image_resample::get_native_resolution */

int Computed_field_image_resample::list()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
==============================================================================*/
{
	int i, return_code;

	ENTER(List_Computed_field_time_image_resample);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    field : %s\n",
			field->source_fields[0]->name);

		display_message(INFORMATION_MESSAGE,
			"    sizes :");
		for (i = 0 ; i < dimension ; i++)
		{
			display_message(INFORMATION_MESSAGE,
				" %d", sizes[i]);
		}
		display_message(INFORMATION_MESSAGE,
			"    input_coordinates_min :");
		for (i = 0 ; i < dimension ; i++)
		{
			display_message(INFORMATION_MESSAGE,
				" %g", input_coordinates_min[i]);
		}
		display_message(INFORMATION_MESSAGE,
			"    input_coordinates_max :");
		for (i = 0 ; i < dimension ; i++)
		{
			display_message(INFORMATION_MESSAGE,
				" %g", input_coordinates_max[i]);
		}
		display_message(INFORMATION_MESSAGE,
			"    lookup_coordinates_min :");
		for (i = 0 ; i < dimension ; i++)
		{
			display_message(INFORMATION_MESSAGE,
				" %g", lookup_coordinates_min[i]);
		}
		display_message(INFORMATION_MESSAGE,
			"    lookup_coordinates_max :");
		for (i = 0 ; i < dimension ; i++)
		{
			display_message(INFORMATION_MESSAGE,
				" %g", lookup_coordinates_max[i]);
		}
		display_message(INFORMATION_MESSAGE,
			"\n");
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_image_resample.  Invalid field");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_image_resample */

char *Computed_field_image_resample::get_command_string(
  )
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field.
==============================================================================*/
{
	char *command_string, *field_name, temp_string[40];
	int error, i;

	ENTER(Computed_field_time_image_resample::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_image_resample_type_string, &error);
		append_string(&command_string, " field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
		append_string(&command_string, " sizes ", &error);
		for (i = 0 ; i < dimension ; i++)
		{
			sprintf(temp_string, " %d", sizes[i]);
			append_string(&command_string, temp_string, &error);
		}
		append_string(&command_string, " input_coordinates_min ", &error);
		for (i = 0 ; i < dimension ; i++)
		{
			sprintf(temp_string, " %g", input_coordinates_min[i]);
			append_string(&command_string, temp_string, &error);
		}
		append_string(&command_string, " input_coordinates_max ", &error);
		for (i = 0 ; i < dimension ; i++)
		{
			sprintf(temp_string, " %g", input_coordinates_max[i]);
			append_string(&command_string, temp_string, &error);
		}
		append_string(&command_string, " lookup_coordinates_min ", &error);
		for (i = 0 ; i < dimension ; i++)
		{
			sprintf(temp_string, " %g", lookup_coordinates_min[i]);
			append_string(&command_string, temp_string, &error);
		}
		append_string(&command_string, " lookup_coordinates_max ", &error);
		for (i = 0 ; i < dimension ; i++)
		{
			sprintf(temp_string, " %g", lookup_coordinates_max[i]);
			append_string(&command_string, temp_string, &error);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_image_resample::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_image_resample::get_command_string */

} //namespace

inline Computed_field_image_resample *Computed_field_image_resample_core_cast(
	cmzn_field_image_resample *image_resample_field)
{
	return (static_cast<Computed_field_image_resample*>(
		reinterpret_cast<Computed_field*>(image_resample_field)->core));
}

Computed_field *cmzn_fieldmodule_create_field_image_resample(
	struct cmzn_fieldmodule *field_module,
	struct Computed_field *source_field, int dimension, int *sizes)
{
	Computed_field *field = NULL;
	if (source_field && source_field->isNumerical() && dimension && sizes)
	{
		Computed_field *source_texture_coordinate_field = NULL;
		int source_field_dimension = 0;
		int *source_sizes = NULL;
		int return_code = Computed_field_get_native_resolution(
			source_field, &source_field_dimension, &source_sizes,
			&source_texture_coordinate_field);
		DEALLOCATE(source_sizes);
		if (return_code && (dimension == source_field_dimension))
		{
			field = Computed_field_create_generic(field_module,
				/*check_source_field_regions*/true,
				source_field->number_of_components,
				/*number_of_source_fields*/1, &source_field,
				/*number_of_source_values*/0, NULL,
				new Computed_field_image_resample(dimension, sizes));
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"cmzn_fieldmodule_create_field_image_resample.  "
				"Specified dimension and source field dimension do not match.");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_image_resample.  Invalid argument(s)");
	}

	return (field);
}

int cmzn_field_get_type_image_resample(struct Computed_field *field,
	struct Computed_field **source_field, int *dimension, int **sizes)
/*******************************************************************************
LAST MODIFIED : 7 March 2007

DESCRIPTION :
If the field is of type COMPUTED_FIELD_IMAGE_RESAMPLE, the function returns the source
<image_resample_field>, <dimension> and <sizes> used in each image direction.
==============================================================================*/
{
	Computed_field_image_resample* core;
	int i, return_code = 0;

	ENTER(cmzn_field_get_type_image_resample);
	if (field && (core = dynamic_cast<Computed_field_image_resample*>(field->core)))
	{
		if (ALLOCATE(*sizes, int, core->dimension))
		{
			for (i = 0 ; i < core->dimension ; i++)
			{
				(*sizes)[i] = core->sizes[i];
			}
			*dimension = core->dimension;
			*source_field = field->source_fields[0];
			return_code = 1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"cmzn_field_get_type_image_resample.  Unable to allocate array.");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_field_get_type_image_resample.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_field_get_type_image_resample */

cmzn_field_image_resample_id cmzn_field_cast_image_resample(cmzn_field_id field)
{
	if (field && (dynamic_cast<Computed_field_image_resample*>(field->core)))
	{
		cmzn_field_access(field);
		return (reinterpret_cast<cmzn_field_image_resample_id>(field));
	}
	else
	{
		return (NULL);
	}
}

int cmzn_field_image_resample_destroy(cmzn_field_image_resample_id *image_resample_address)
{
	return cmzn_field_destroy(reinterpret_cast<cmzn_field_id *>(image_resample_address));
}

int cmzn_field_image_resample_set_input_coordinates_minimum(cmzn_field_image_resample_id image_resample,
	int dimension, const double *input_coordinates_minimum)
{
	if (image_resample)
	{
		Computed_field_image_resample *image_resample_core =
			Computed_field_image_resample_core_cast(image_resample);
		return image_resample_core->set_input_coordinates_minimum(
			dimension, input_coordinates_minimum);
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_field_image_resample_set_input_coordinates_maximum(cmzn_field_image_resample_id image_resample,
	int dimension, const double *input_coordinates_maximum)
{
	if (image_resample)
	{
		Computed_field_image_resample *image_resample_core =
			Computed_field_image_resample_core_cast(image_resample);
		return image_resample_core->set_input_coordinates_maximum(
			dimension, input_coordinates_maximum);
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_field_image_resample_set_lookup_coordinates_minimum(cmzn_field_image_resample_id image_resample,
	int dimension, const double *lookup_coordinates_minimum)
{
	if (image_resample)
	{
		Computed_field_image_resample *image_resample_core =
			Computed_field_image_resample_core_cast(image_resample);
		return image_resample_core->set_lookup_coordinates_minimum(
			dimension, lookup_coordinates_minimum);
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_field_image_resample_set_lookup_coordinates_maximum(cmzn_field_image_resample_id image_resample,
	int dimension, const double *lookup_coordinates_maximum)
{
	if (image_resample)
	{
		Computed_field_image_resample *image_resample_core =
			Computed_field_image_resample_core_cast(image_resample);
		return image_resample_core->set_lookup_coordinates_maximum(
			dimension, lookup_coordinates_maximum);
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_field_image_resample_get_input_coordinates_minimum(cmzn_field_image_resample_id image_resample,
	int dimension, double *input_coordinates_minimum)
{
	if (image_resample)
	{
		Computed_field_image_resample *image_resample_core =
			Computed_field_image_resample_core_cast(image_resample);
		return image_resample_core->get_input_coordinates_minimum(
			dimension, input_coordinates_minimum);
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_field_image_resample_get_input_coordinates_maximum(cmzn_field_image_resample_id image_resample,
	int dimension, double *input_coordinates_maximum)
{
	if (image_resample)
	{
		Computed_field_image_resample *image_resample_core =
			Computed_field_image_resample_core_cast(image_resample);
		return image_resample_core->get_input_coordinates_maximum(
			dimension, input_coordinates_maximum);
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_field_image_resample_get_lookup_coordinates_minimum(cmzn_field_image_resample_id image_resample,
	int dimension, double *lookup_coordinates_minimum)
{
	if (image_resample)
	{
		Computed_field_image_resample *image_resample_core =
			Computed_field_image_resample_core_cast(image_resample);
		return image_resample_core->get_lookup_coordinates_minimum(
			dimension, lookup_coordinates_minimum);
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_field_image_resample_get_lookup_coordinates_maximum(cmzn_field_image_resample_id image_resample,
	int dimension, double *lookup_coordinates_maximum)
{
	if (image_resample)
	{
		Computed_field_image_resample *image_resample_core =
			Computed_field_image_resample_core_cast(image_resample);
		return image_resample_core->get_lookup_coordinates_maximum(
			dimension, lookup_coordinates_maximum);
	}
	return CMZN_ERROR_ARGUMENT;
}
