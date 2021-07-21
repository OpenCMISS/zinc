/*******************************************************************************
FILE : computed_field_fast_marching_image_filter.cpp

LAST MODIFIED : 22 February 2007

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
#include "image_processing/computed_field_fast_marching_image_filter.h"

#include "itkImage.h"
#include "itkVector.h"
#include "itkFastMarchingImageFilter.h"

using namespace CMZN;

namespace {

char computed_field_fast_marching_image_filter_type_string[] = "fast_marching_filter";

class Computed_field_fast_marching_image_filter : public computed_field_image_filter
{

public:
	double stopping_value;
	int num_seed_points;
	double *seed_points;  // seed positions
	double *seed_values;  // seed values at each position
	int *output_size;

	// Need something to represent index

	Computed_field_fast_marching_image_filter(Computed_field *source_field,
		double stopping_value, int num_seed_points, const double *seed_points,
		const double *seed_values, const int *output_size);

	~Computed_field_fast_marching_image_filter()
	{

		// delete memory used by arrays

		if (seed_points) {
			delete [] seed_points;
		}

		if (seed_values) {
			delete [] seed_values;
		}

		if (output_size) {
			delete [] output_size;
		}				

	}

private:
	virtual void create_functor();

	Computed_field_core *copy()
	{
		return new Computed_field_fast_marching_image_filter(field->source_fields[0],
 		  stopping_value, num_seed_points, seed_points, seed_values, output_size);
	}

	const char *get_type_string()
	{
		return(computed_field_fast_marching_image_filter_type_string);
	}

	int compare(Computed_field_core* other_field);

	int list();

	char* get_command_string();
};

int Computed_field_fast_marching_image_filter::compare(Computed_field_core *other_core)
/*******************************************************************************
LAST MODIFIED : 22 February 2007

DESCRIPTION :
Compare the type specific data.
==============================================================================*/
{
	Computed_field_fast_marching_image_filter* other;
	int return_code;
	int i;

	ENTER(Computed_field_fast_marching_image_filter::compare);
	if (field && (other = dynamic_cast<Computed_field_fast_marching_image_filter*>(other_core)))
	{

		if ((dimension == other->dimension)
				&& (stopping_value == other->stopping_value)
				&& (num_seed_points == other->num_seed_points)) {

			return_code = 1;

			// check that all seed_points match
			for (i = 0 ; return_code && (i < dimension*num_seed_points) ; i++) {
				if (seed_points[i] != other->seed_points[i]) {
					return_code = 0;
				}
			}

			// check that all seed_values match
			for (i = 0 ; return_code && (i < num_seed_points) ; i++) {
				if (seed_values[i] != other->seed_values[i]) {
					return_code = 0;
				}
			}

			// check that all output_size values match
			for (i = 0 ; return_code && (i < dimension) ; i++) {
				if (output_size[i] != other->output_size[i]) {
					return_code = 0;
				}
			}
			
		} else {
			return_code = 0;
		}
	}
	else
	{
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_fast_marching_image_filter::compare */

template < class ImageType >
class Computed_field_fast_marching_image_filter_Functor :
	public computed_field_image_filter_FunctorTmpl< ImageType >
/*******************************************************************************
LAST MODIFIED : 22 February 2007

DESCRIPTION :
This class actually does the work of processing images with the filter.
It is instantiated for each of the chosen ImageTypes.
==============================================================================*/
{
	Computed_field_fast_marching_image_filter *fast_marching_image_filter;

public:

	Computed_field_fast_marching_image_filter_Functor(
		Computed_field_fast_marching_image_filter *fast_marching_image_filter) :
		computed_field_image_filter_FunctorTmpl< ImageType >(fast_marching_image_filter),
		fast_marching_image_filter(fast_marching_image_filter)
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

		dimension  = fast_marching_image_filter->dimension;
		num_seed_points = fast_marching_image_filter->num_seed_points;
		image_size = fast_marching_image_filter->sizes;
		
	  // create a fastmarching object

		typedef itk::FastMarchingImageFilter<ImageType,ImageType> FMType;
		
		typename FMType::Pointer filter = FMType::New();

		// set trial points
				
		// create a node container to hold the seeds
		typedef typename FMType::NodeType NodeType;
		typedef typename FMType::NodeContainer NodeContainer;	 
		typename NodeContainer::Pointer seeds = NodeContainer::New();		
		typename ImageType::IndexType seedPosition;
		
		NodeType node;
		double seedValue;
		
		// intialise seeds container
		seeds->Initialize();
		
		// add seeds to container
		// loop over number of seeds, setting the seed value and position for each seed		
		
		for (i = 0; i < num_seed_points; i++) {
		
			// intially we will just use zero for all seedValues
			// change this once the filter is working
			seedValue = fast_marching_image_filter->seed_values[i];

			for (j = 0; j < dimension; j++) {

				// convert seed position values from xi space to values in terms 
				// of image pixels (based on image size)
				seedPosition[j] = (long int)(fast_marching_image_filter->seed_points[i * dimension + j]*image_size[j]+0.5);

				//				printf("seed position %ld\n",seedPosition[j]);	

			}

			node.SetValue( seedValue );
			if (dimension > 0)
			{
				node.SetIndex( seedPosition );
			}
			seeds->InsertElement( i, node );		
		}

		// set trial points to be those in the seeds container
		filter->SetTrialPoints(seeds);


		// The normalisation factor is used in the fast marching equations
		// to allow integer values
		// It appears that the default equations are designed to work on values
		// in the range 0 to 255.  If we wish to calculate a fast maching
		// solution based on values in the range 0 to 1, then the normalization
		// factor needs to be adjusted.  
		// Values are divided by the normalization factor so to scale from 1 to 
		// 255 requires setting a normalization factor of 1/255.

		filter->SetNormalizationFactor(1/255.0);

		// IMPORTANT: if we wish to use field values that vary between a range
		// other than 0 to 1, then the normilzation factor will need to be
		// set to a sensible value based on a user defined parameter,
		// as part of the gfx def field command for the fast marching filter


		// set the output size

		// initially we use image_size values as default output image_size
		// could eventutally	fetch output image size array from the 
		// fast_marching_image_filter options
		// seems to have no effect what the output size is set to.

		typename ImageType::SizeType output_size;
		for (i=0;i <dimension;i++) {
			output_size[i] = image_size[i];
		}
		output_size[0]=1;
		output_size[1]=1;

		filter->SetOutputSize(output_size);

		// set stopping value
		filter->SetStoppingValue( fast_marching_image_filter->stopping_value);
		//		printf("Stopping value is %f", fast_marching_image_filter->stopping_value);

		return_code = fast_marching_image_filter->update_output_image
			(cache, filter, this->outputImage,
			static_cast<ImageType*>(NULL), static_cast<FMType*>(NULL));
		
		return (return_code);
	} /* set_filter */

}; /* template < class ImageType > class Computed_field_fast_marching_image_filter_Functor */

Computed_field_fast_marching_image_filter::Computed_field_fast_marching_image_filter(
	Computed_field *source_field, double stopping_value, int num_seed_points,
	const double *seed_points_in, const double *seed_values_in, const int *output_size_in) :
	computed_field_image_filter(source_field),
	stopping_value(stopping_value), num_seed_points(num_seed_points)
/*******************************************************************************
LAST MODIFIED : 22 February 2007

DESCRIPTION :
Create the computed_field representation of the fast marching image filter.
==============================================================================*/
{
	// NEED to allocate seed_points array by copying it to memory here.
	// see resample code for some idea
	int i;
	int length_seed_points;
	length_seed_points = dimension * num_seed_points;
	seed_points = new double[length_seed_points];
	seed_values = new double[num_seed_points];
	output_size = new int[dimension];

	for (i = 0 ; i < length_seed_points ; i++) {
		seed_points[i] = seed_points_in[i];
	}

	for (i = 0 ; i < num_seed_points ; i++) {
		seed_values[i] = seed_values_in[i];
	}

	for (i = 0 ; i < dimension ; i++) {
		output_size[i] = output_size_in[i];
	}
}

void Computed_field_fast_marching_image_filter::create_functor()
{
#if defined DONOTUSE_TEMPLATETEMPLATES
	create_filters_singlecomponent_multidimensions(
		Computed_field_fast_marching_image_filter_Functor, this);
#else
	create_filters_singlecomponent_multidimensions
		< Computed_field_fast_marching_image_filter_Functor,
		Computed_field_fast_marching_image_filter >
		(this);
#endif
}

int Computed_field_fast_marching_image_filter::list()
/*******************************************************************************
LAST MODIFIED : 22 February 2007

DESCRIPTION :
==============================================================================*/
{
	int return_code = 0;
	int i;

	ENTER(List_Computed_field_fast_marching_image_filter);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    source field : %s\n",field->source_fields[0]->name);
		display_message(INFORMATION_MESSAGE,
			"    lower_threshold : %g\n", stopping_value);
		display_message(INFORMATION_MESSAGE,
			"    upper_threshold : %d\n", num_seed_points);
		display_message(INFORMATION_MESSAGE,
			"    replace_value : %d\n", dimension);

		display_message(INFORMATION_MESSAGE,"    seed_points :");
		for (i = 0; i < dimension*num_seed_points; i++) {
			display_message(INFORMATION_MESSAGE," %g",seed_points[i]);
		}		

		display_message(INFORMATION_MESSAGE,"    seed_values :");
		for (i = 0; i < num_seed_points; i++) {
			display_message(INFORMATION_MESSAGE," %g",seed_values[i]);
		}		

		display_message(INFORMATION_MESSAGE,"    output_size :");
		for (i = 0; i < dimension; i++) {
			display_message(INFORMATION_MESSAGE," %d",output_size[i]);
		}		

		display_message(INFORMATION_MESSAGE,"\n");

	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_fast_marching_image_filter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_fast_marching_image_filter */

char *Computed_field_fast_marching_image_filter::get_command_string()
/*******************************************************************************
LAST MODIFIED : 22 February 2007

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name, temp_string[40];
	int error;
	int i;

	
	ENTER(Computed_field_fast_marching_image_filter::get_command_string);
	command_string = (char *)NULL;
	if (field) {

		error = 0;
		append_string(&command_string, get_type_string(), &error);
		append_string(&command_string, " field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
		sprintf(temp_string, " stopping_value %g", stopping_value);
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

		append_string(&command_string, " seed_values", &error);
		for (i = 0; i < num_seed_points; i++)
		{
			sprintf(temp_string, " %g", seed_values[i]);
			append_string(&command_string, temp_string, &error);
		}

		append_string(&command_string, " output_size", &error);
		for (i = 0; i < dimension; i++)
		{
			sprintf(temp_string, " %d", output_size[i]);
			append_string(&command_string, temp_string, &error);
		}

	} else {

		display_message(ERROR_MESSAGE,
			"Computed_field_fast_marching_image_filter::get_command_string.  Invalid field");
		
	}
	LEAVE;

	return (command_string);
} /* Computed_field_fast_marching_image_filter::get_command_string */

} //namespace

struct Computed_field *cmzn_fieldmodule_create_field_imagefilter_fast_marching(
	struct cmzn_fieldmodule *field_module,
	struct Computed_field *source_field, double stopping_value,
  int num_seed_points, int dimension, const double *seed_points, const double *seed_values,
  const int *output_size)
{
	Computed_field *field = NULL;
	USE_PARAMETER(dimension);
	if (source_field && Computed_field_is_scalar(source_field, (void *)NULL))
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field->number_of_components,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_fast_marching_image_filter(source_field, stopping_value,
				num_seed_points, seed_points, seed_values, output_size));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_imagefilter_fast_marching.  Invalid argument(s)");
	}

	return (field);
}

int cmzn_field_get_type_fast_marching_image_filter(struct Computed_field *field,
  struct Computed_field **source_field, double *stopping_value, 
		int *num_seed_points, int *dimension, double **seed_points, 
		double **seed_values, int **output_size)
/*******************************************************************************
LAST MODIFIED : 22 February 2007

DESCRIPTION :
If the field is of type COMPUTED_FIELD_FAST_MARCHING_IMAGE_FILTER, 
the source_field and thresholds used by it are returned - 
otherwise an error is reported.
==============================================================================*/
{
	Computed_field_fast_marching_image_filter* core;
	int return_code;
	int i, length_seed_points;


	ENTER(cmzn_field_get_type_fast_marching_image_filter);
	if (field && (core = dynamic_cast<Computed_field_fast_marching_image_filter*>(field->core))
		&& source_field)
	{
		*source_field = field->source_fields[0];
		*stopping_value = core->stopping_value;
		*num_seed_points = core->num_seed_points;
		*dimension = core->dimension;

		length_seed_points = core->dimension * core->num_seed_points;

		ALLOCATE(*seed_points, double, length_seed_points);
		for (i=0; i < length_seed_points ;i++) {
			(*seed_points)[i]=core->seed_points[i];
		}

		ALLOCATE(*seed_values, double, *num_seed_points);
		for (i=0; i < *num_seed_points ;i++) {
			(*seed_values)[i]=core->seed_values[i];
		}

		ALLOCATE(*output_size, int, *dimension);
		for (i=0; i < *dimension ;i++) {
			(*output_size)[i]=core->output_size[i];
		}
		
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_field_get_type_fast_marching_image_filter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_field_get_type_fast_marching_image_filter */

