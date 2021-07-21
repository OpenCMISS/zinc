/*******************************************************************************
FILE : computed_field_histogram_image_filter.c

LAST MODIFIED : 26 March 2008

DESCRIPTION :
Wraps itk::Statistics::ScalarImageToHistogramGenerator
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
#include "image_processing/computed_field_histogram_image_filter.h"
#include "itkImage.h"
#include "itkVector.h"
#include "itkScalarImageToHistogramGenerator.h"
#include "itkImageToHistogramFilter.h"

using namespace CMZN;

namespace {

char computed_field_histogram_image_filter_type_string[] = "histogram_filter";

class Computed_field_histogram_image_filter : public computed_field_image_filter
{

public:
	/* Number of bins with respect to each component */
	int sourceNumberOfComponents;
	int *numberOfBins;
	double marginalScale;
	double *histogramMinimum;
	double *histogramMaximum;
	int totalPixels;

	Computed_field_histogram_image_filter(Computed_field *source_field);
       
	Computed_field_histogram_image_filter(Computed_field *source_field,
		const int *numberOfBins, double marginalScale, const double *histogramMinimumIn, const double *histogramMaximumIn);

	~Computed_field_histogram_image_filter()
	{
		if (numberOfBins)
			delete [] numberOfBins;
		if (histogramMaximum)
			delete [] histogramMaximum;
		if (histogramMinimum)
			delete [] histogramMinimum;
	};

	template <class ImageType, class HistogramFilterType >
	int update_histogram(cmzn_fieldcache& cache,
		typename HistogramFilterType::Pointer filter,
		const typename HistogramFilterType::HistogramType *&outputHistogram,
		ImageType *dummytemplarg1, HistogramFilterType *dummytemplarg2);

	template <class ImageType, class HistogramFilterType >
	int update_histogram_scalar(cmzn_fieldcache& cache,
		typename HistogramFilterType::Pointer filter,
		const typename HistogramFilterType::HistogramType *&outputHistogram,
		ImageType *dummytemplarg1, HistogramFilterType *dummytemplarg2);

	template <class ImageType, class HistogramFilterType >
	int evaluate_histogram(cmzn_fieldcache& cache, RealFieldValueCache& valueCache,
		const typename HistogramFilterType::HistogramType *outputHistogram,
		ImageType *dummytemplarg, HistogramFilterType *dummytemplarg2);

	double getMaginalScale()
	{
		return marginalScale;
	}

	int setMaginalScale(double maginalScaleIn)
	{
		marginalScale = maginalScaleIn;
		clear_cache();
		return CMZN_OK;
	}

	int getNumberOfBins(int valuesCount, int *valuesOut)
	{
		if ((valuesCount == 0) || ((valuesCount > 0) && valuesOut))
		{
			for (int i = 0; i < sourceNumberOfComponents; i++)
			{
				valuesOut[i] = numberOfBins[i];
			}
			return sourceNumberOfComponents;
		}
		return 0;
	}

	int setNumberOfBins(int valuesCount, const int *valuesIn)
	{
		if ((valuesCount > 0) && valuesIn)
		{
			for (int i = 0; i < sourceNumberOfComponents; i++)
			{
				if (i > valuesCount)
				{
					numberOfBins[i] = valuesIn[valuesCount - 1];
				}
				else
				{
					numberOfBins[i] = valuesIn[i];
				}
			}
			clear_cache();
			return CMZN_OK;
		}
		return CMZN_ERROR_ARGUMENT;
	}

	int getHistogramMinimum(int valuesCount, double *valuesOut)
	{
		if ((valuesCount == 0) || ((valuesCount > 0) && valuesOut))
		{
			for (int i = 0; i < sourceNumberOfComponents; i++)
			{
				valuesOut[i] = histogramMinimum[i];
			}
			return sourceNumberOfComponents;
		}
		return 0;
	}

	int setHistogramMinimum(int valuesCount, const double *valuesIn)
	{
		if ((valuesCount > 0) && valuesIn)
		{
			for (int i = 0; i < sourceNumberOfComponents; i++)
			{
				if (i > valuesCount)
				{
					histogramMinimum[i] = valuesIn[valuesCount - 1];
				}
				else
				{
					histogramMinimum[i] = valuesIn[i];
				}
			}
			clear_cache();
			return CMZN_OK;
		}
		return CMZN_ERROR_ARGUMENT;
	}

	int getHistogramMaximum(int valuesCount, double *valuesOut)
	{
		if ((valuesCount == 0) || ((valuesCount > 0) && valuesOut))
		{
			for (int i = 0; i < sourceNumberOfComponents; i++)
			{
				valuesOut[i] = histogramMaximum[i];
			}
			return sourceNumberOfComponents;
		}
		return 0;
	}

	int setHistogramMaximum(int valuesCount, const double *valuesIn)
	{
		if ((valuesCount > 0) && valuesIn)
		{
			for (int i = 0; i < sourceNumberOfComponents; i++)
			{
				if (i > valuesCount)
				{
					histogramMaximum[i] = valuesIn[valuesCount - 1];
				}
				else
				{
					histogramMaximum[i] = valuesIn[i];
				}
			}
			clear_cache();
			return CMZN_OK;
		}
		return CMZN_ERROR_ARGUMENT;
	}

private:
	virtual void create_functor();

	int get_native_resolution(
		int *native_dimension, int **native_sizes, 
		struct Computed_field **native_texture_coordinate_field)
	{
		int i, return_code;

		ENTER(Computed_field_default_get_native_resolution);
		if (field&&native_dimension&&native_sizes&&
			native_texture_coordinate_field&&
			ALLOCATE(*native_sizes, int, sourceNumberOfComponents))
		{
			*native_dimension = sourceNumberOfComponents;
			for (i = 0 ; i < sourceNumberOfComponents ; i++)
			{
				(*native_sizes)[i] = numberOfBins[i];
			}
			*native_texture_coordinate_field = texture_coordinate_field;
			return_code = 1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Computed_field_histogram_image_filter::get_native_resolution.  "
				"Invalid argument(s)");
			return_code = 0;
		}
		LEAVE;
		
		return (return_code);
	} /* Computed_field_default_get_native_resolution */

	Computed_field_core *copy()
	{
		return new Computed_field_histogram_image_filter(field->source_fields[0],
			numberOfBins, marginalScale, histogramMinimum, histogramMaximum);
	}

	const char *get_type_string()
	{
		return(computed_field_histogram_image_filter_type_string);
	}

	int compare(Computed_field_core* other_field);

	int list();

	char* get_command_string();

	template < class ComputedFieldFilter >
	int create_histogram_filters_multicomponent_multidimensions(
		ComputedFieldFilter* filter);

};

inline Computed_field_histogram_image_filter *
	Computed_field_histogram_image_filter_core_cast(
		cmzn_field_imagefilter_histogram *imagefilter_histogram)
{
	return (static_cast<Computed_field_histogram_image_filter*>(
		reinterpret_cast<Computed_field*>(imagefilter_histogram)->core));
}

template <class ImageType, class HistogramFilterType >
int Computed_field_histogram_image_filter::update_histogram_scalar(
	cmzn_fieldcache& cache,
	typename HistogramFilterType::Pointer filter,
	const typename HistogramFilterType::HistogramType *&outputHistogram,
	ImageType *dummytemplarg1, HistogramFilterType *dummytemplarg2)
/*******************************************************************************
LAST MODIFIED : 25 March 2008

DESCRIPTION :
Evaluate the templated version of this filter
==============================================================================*/
{
	USE_PARAMETER(dummytemplarg2);
	typename ImageType::Pointer inputImage;

	if (create_input_image(cache, inputImage, dummytemplarg1))
	{
		try
		{
			filter->SetInput( inputImage );

			//Histogram generators are not pipeline objects so
			//we have to explicitly update the input image

			inputImage->Update();

			filter->Compute();

			outputHistogram = filter->GetOutput();

		} catch ( itk::ExceptionObject & err )
		{
			display_message(ERROR_MESSAGE,
				"ExceptionObject caught!");
			display_message(ERROR_MESSAGE,
				(char *)err.GetDescription());
		}

		if (outputHistogram)
		{
			return 1;
		}
	}
	return 0;
}

template <class ImageType, class HistogramFilterType >
int Computed_field_histogram_image_filter::update_histogram(
	cmzn_fieldcache& cache,
	typename HistogramFilterType::Pointer filter,
	const typename HistogramFilterType::HistogramType *&outputHistogram,
	ImageType *dummytemplarg1, HistogramFilterType *dummytemplarg2)
/*******************************************************************************
LAST MODIFIED : 25 March 2008

DESCRIPTION :
Evaluate the templated version of this filter
==============================================================================*/
{
	USE_PARAMETER(dummytemplarg2);
	typename ImageType::Pointer inputImage;

	if (create_input_image(cache, inputImage, dummytemplarg1))
	{
		try
		{
			filter->SetInput( inputImage );

			//Histogram generators are not pipeline objects so
			//we have to explicitly update the input image

			inputImage->Update();

			filter->Update();

			outputHistogram = filter->GetOutput();
			
		} catch ( itk::ExceptionObject & err )
		{
			display_message(ERROR_MESSAGE,
				"ExceptionObject caught!");
			display_message(ERROR_MESSAGE,
				(char *)err.GetDescription());
		}

		if (outputHistogram)
		{
			return 1;
		}
	}
	return 0;
}
	
template <class ImageType, class HistogramFilterType >
int Computed_field_histogram_image_filter::evaluate_histogram(
	cmzn_fieldcache& cache, RealFieldValueCache& valueCache,
	const typename HistogramFilterType::HistogramType *outputHistogram,
	ImageType *dummytemplarg, HistogramFilterType *dummytemplarg2)
{
	USE_PARAMETER(dummytemplarg);
	USE_PARAMETER(dummytemplarg2);
	const Field_location_element_xi* element_xi_location;
	const Field_location_field_values* coordinate_location = 0;
	const FE_value* xi = NULL;

	if (element_xi_location = cache.get_location_element_xi())
	{
		xi = element_xi_location->get_xi();
	}
	else if (coordinate_location = cache.get_location_field_values())
	{
		xi = coordinate_location->get_values();
	}

	if (xi && outputHistogram)
	{
		unsigned int bin = 0;
		unsigned int offset = 1;

		for (int i = 0 ; i < sourceNumberOfComponents ; i++)
		{
			if (xi[i] >= 1.0)
			{
				bin += offset * (numberOfBins[i] - 1);
			}
			else if (xi[i] <= 0.0)
			{
				/* nothing to add */
			}
			else
			{
				bin += offset * static_cast<unsigned int>(
					floor((double)numberOfBins[i] * xi[i]));
			}
			offset *= numberOfBins[i];
		}

		valueCache.values[0] = outputHistogram->GetFrequency( bin ) /
			(double)totalPixels;
		return 1;
	}
	return 0;
}

int Computed_field_histogram_image_filter::compare(Computed_field_core *other_core)
/*******************************************************************************
LAST MODIFIED : 18 October 2006

DESCRIPTION :
Compare the type specific data.
==============================================================================*/
{
	Computed_field_histogram_image_filter* other;
	int i, return_code;

	ENTER(Computed_field_histogram_image_filter::compare);
	if (field && (other = dynamic_cast<Computed_field_histogram_image_filter*>(other_core)))
	{
		if ((dimension == other->dimension)
			&& (sourceNumberOfComponents == other->sourceNumberOfComponents)
			&& (marginalScale == other->marginalScale))
		{
			return_code = 1;
			for (i = 0 ; return_code && i < sourceNumberOfComponents ; i++)
			{
				if (numberOfBins[i] != other->numberOfBins[i])
				{
					return_code = 0;
				}
			}
			if (histogramMinimum)
			{
				if (other->histogramMinimum)
				{
					for (i = 0 ; return_code && i < sourceNumberOfComponents ; i++)
					{
						if (histogramMinimum[i] != other->histogramMinimum[i])
						{
							return_code = 0;
						}
					}
				}
				else
					return_code = 0;
			}
			else if (other->histogramMinimum)
				return_code = 0;
			if (histogramMaximum)
			{
				if (other->histogramMaximum)
				{
					for (i = 0 ; return_code && i < sourceNumberOfComponents ; i++)
					{
						if (histogramMaximum[i] != other->histogramMaximum[i])
						{
							return_code = 0;
						}
					}
				}
				else
					return_code = 0;
			}
			else if (other->histogramMaximum)
				return_code = 0;
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
} /* Computed_field_histogram_image_filter::compare */

int Computed_field_histogram_image_filter::list()
/*******************************************************************************
LAST MODIFIED : 18 October 2006

DESCRIPTION :
==============================================================================*/
{
	int i, return_code = 0;

	ENTER(List_Computed_field_histogram_image_filter);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    source field : %s\n",field->source_fields[0]->name);
		display_message(INFORMATION_MESSAGE,
			"    filter number of bins :");
		for (i = 0 ; i < sourceNumberOfComponents ; i++)
		{
			display_message(INFORMATION_MESSAGE,
				" %d", numberOfBins[i]);
		}
		display_message(INFORMATION_MESSAGE,
			"    histogram minimum :");
		if (histogramMinimum)
		{
			for (i = 0 ; return_code && i < sourceNumberOfComponents ; i++)
			{
				display_message(INFORMATION_MESSAGE,
					" %g", histogramMinimum[i]);
			}
		}
		else
			display_message(INFORMATION_MESSAGE,
				" not set");
		display_message(INFORMATION_MESSAGE,
			"    histogram maximum :");
		if (histogramMaximum)
		{
			for (i = 0 ; return_code && i < sourceNumberOfComponents ; i++)
			{
				display_message(INFORMATION_MESSAGE,
					" %g", histogramMaximum[i]);
			}
		}
		else
			display_message(INFORMATION_MESSAGE,
				" not set");
			
		display_message(INFORMATION_MESSAGE,
			"    filter marginal scale : %g\n", marginalScale);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_histogram_image_filter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_histogram_image_filter */

char *Computed_field_histogram_image_filter::get_command_string()
/*******************************************************************************
LAST MODIFIED : 18 October 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name, temp_string[40];
	int i, error;

	ENTER(Computed_field_histogram_image_filter::get_command_string);
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
		sprintf(temp_string, " number_of_bins");
		append_string(&command_string, temp_string, &error);	
		for (i = 0 ; i < sourceNumberOfComponents ; i++)
		{
			sprintf(temp_string, " %d", numberOfBins[i]);
			append_string(&command_string, temp_string, &error);	
		}
		if (histogramMinimum)
		{
			append_string(&command_string,  " minimums", &error);	
			for (i = 0 ; i < sourceNumberOfComponents ; i++)
			{
				sprintf(temp_string, " %g", histogramMinimum[i]);
				append_string(&command_string, temp_string, &error);	
			}
		}
		if (histogramMaximum)
		{
			append_string(&command_string,  " maximums", &error);	
			for (i = 0 ; i < sourceNumberOfComponents ; i++)
			{
				sprintf(temp_string, " %g", histogramMaximum[i]);
				append_string(&command_string, temp_string, &error);	
			}
		}
		sprintf(temp_string, " marginal_scale  %g", marginalScale);
		append_string(&command_string, temp_string, &error);	
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_histogram_image_filter::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_histogram_image_filter::get_command_string */

template < class ImageType, class HistogramFilterType >
class Computed_field_histogram_image_filter_Functor :
	public computed_field_image_filter_Functor
/*******************************************************************************
LAST MODIFIED : 26 March 2008

DESCRIPTION :
This class actually does the work of processing images with the filter.
It is instantiated for each of the chosen ImageTypes.
==============================================================================*/
{
protected:
	Computed_field_histogram_image_filter *histogram_image_filter;

	const typename HistogramFilterType::HistogramType * histogram;

	typename HistogramFilterType::Pointer filter;

public:

	Computed_field_histogram_image_filter_Functor(
		Computed_field_histogram_image_filter *histogram_image_filter) :
		histogram_image_filter(histogram_image_filter),
		histogram(NULL), filter(NULL)
	{
	}

	int update_and_evaluate_filter(cmzn_fieldcache& cache, RealFieldValueCache& valueCache)
/*******************************************************************************
LAST MODIFIED : 26 March 2008

DESCRIPTION :
Updates the outputImage if required and then evaluates the outputImage at the 
location.
==============================================================================*/
	{
		int return_code;
		if (!histogram)
		{
			return_code = set_filter(cache);
			if (return_code )
			{
				return_code = histogram_image_filter->evaluate_histogram
					(cache, valueCache, histogram,
					 static_cast<ImageType*>(NULL),
					 static_cast<HistogramFilterType*>(NULL));
			}
		}
		else
		{
			return_code = histogram_image_filter->evaluate_histogram
				(cache, valueCache, histogram,
					static_cast<ImageType*>(NULL),
					static_cast<HistogramFilterType*>(NULL));
		}
		return(return_code);
	}

	int clear_cache() // GRC check what this does
	{
		histogram = NULL;
		return (1);
	}

}; /* template < class ImageType > class Computed_field_histogram_image_filter_Functor */

template < class ImageType >
class Computed_field_histogram_scalar_image_filter_Functor :
	public Computed_field_histogram_image_filter_Functor< ImageType,
		itk::Statistics::ScalarImageToHistogramGenerator< ImageType > >
/*******************************************************************************
LAST MODIFIED : 26 March 2008

DESCRIPTION :
This class actually does the work of processing images with the filter.
It is instantiated for each of the chosen ImageTypes.
==============================================================================*/
{
public:
	Computed_field_histogram_scalar_image_filter_Functor(
		Computed_field_histogram_image_filter *histogram_image_filter) :
	Computed_field_histogram_image_filter_Functor< ImageType,
		itk::Statistics::ScalarImageToHistogramGenerator< ImageType > >
	   (histogram_image_filter)
	{
	}

	int set_filter(cmzn_fieldcache& cache)
/*******************************************************************************
LAST MODIFIED : 26 March 2008

DESCRIPTION :
Create a filter of the correct type, set the filter specific parameters
and generate the outputImage.
==============================================================================*/
	{
		int return_code;

		typedef itk::Statistics::ScalarImageToHistogramGenerator< ImageType > HistogramFilterType;

		this->filter = HistogramFilterType::New();

		this->filter->SetNumberOfBins( this->histogram_image_filter->numberOfBins[0] );
		this->filter->SetMarginalScale( this->histogram_image_filter->marginalScale );
		if (this->histogram_image_filter->histogramMinimum)
			this->filter->SetHistogramMin( this->histogram_image_filter->histogramMinimum[0] );
		if (this->histogram_image_filter->histogramMaximum)
			this->filter->SetHistogramMax( this->histogram_image_filter->histogramMaximum[0] );
		
		return_code = this->histogram_image_filter->update_histogram_scalar
			(cache, this->filter, this->histogram,
			static_cast<ImageType*>(NULL),
			static_cast<HistogramFilterType*>(NULL));
		
		return (return_code);
	} /* set_filter */

}; /* template < class ImageType > class Computed_field_histogram_image_filter_Functor */

template < class ImageType >
class Computed_field_histogram_nonscalar_image_filter_Functor :
	public Computed_field_histogram_image_filter_Functor< ImageType,
		itk::Statistics::ImageToHistogramFilter< ImageType > >
/*******************************************************************************
LAST MODIFIED : 26 March 2008

DESCRIPTION :
This class actually does the work of processing images with the filter.
It is instantiated for each of the chosen ImageTypes.
==============================================================================*/
{
public:
	Computed_field_histogram_nonscalar_image_filter_Functor(
		Computed_field_histogram_image_filter *histogram_image_filter) :
	Computed_field_histogram_image_filter_Functor<ImageType,
		itk::Statistics::ImageToHistogramFilter< ImageType > >
	   (histogram_image_filter)
	{
	}

	int set_filter(cmzn_fieldcache& cache)
/*******************************************************************************
LAST MODIFIED : 26 March 2008

DESCRIPTION :
Create a filter of the correct type, set the filter specific parameters
and generate the outputImage.
==============================================================================*/
	{
		int return_code;

		typedef itk::Statistics::ImageToHistogramFilter< ImageType > HistogramFilterType;

		this->filter = HistogramFilterType::New();

		typename HistogramFilterType::HistogramSizeType binSizes;
		int size = binSizes.GetSize();
		for (int i = 0; i < size; i++)
			binSizes[0] = this->histogram_image_filter->numberOfBins[0];
		this->filter->SetHistogramSize( binSizes );
		this->filter->SetMarginalScale(
			this->histogram_image_filter->marginalScale );
		if (this->histogram_image_filter->histogramMinimum)
		{
			typename HistogramFilterType::HistogramMeasurementVectorType lowerBound(this->histogram_image_filter->sourceNumberOfComponents);
			for (int i = 0; i < this->histogram_image_filter->sourceNumberOfComponents; i++)
			{
				lowerBound[i] = this->histogram_image_filter->histogramMinimum[i];
			}
			this->filter->SetHistogramBinMinimum(lowerBound);
		}
		if (this->histogram_image_filter->histogramMaximum)
		{
			typename HistogramFilterType::HistogramMeasurementVectorType upperBound(this->histogram_image_filter->sourceNumberOfComponents);
			for (int i = 0; i < this->histogram_image_filter->sourceNumberOfComponents; i++)
			{
				upperBound[i] = this->histogram_image_filter->histogramMaximum[i];
			}
			this->filter->SetHistogramBinMaximum( upperBound );
		}

		return_code = this->histogram_image_filter->update_histogram
			(cache, this->filter, this->histogram,
			static_cast<ImageType*>(NULL),
			static_cast<HistogramFilterType*>(NULL));
		
		return (return_code);
	} /* set_filter */

}; /* template < class ImageType > class Computed_field_histogram_image_filter_Functor */

template < class ComputedFieldFilter >
int Computed_field_histogram_image_filter::create_histogram_filters_multicomponent_multidimensions(
  ComputedFieldFilter* const filter)
/*******************************************************************************
LAST MODIFIED : 7 September 2006

DESCRIPTION :
Evaluate the fields cache at the location
==============================================================================*/
{
	int return_code;

	ENTER(computed_field_image_filter::select_filter_single_component);

	switch (dimension)
	{
		case 1:
		{
			switch (sourceNumberOfComponents)
			{
				case 1:
				{
					functor = new Computed_field_histogram_scalar_image_filter_Functor
						< itk::Image< ZnReal, 1 > >
						(filter);
					return_code = 1;
				} break;
				case 2:
				{
					functor = new Computed_field_histogram_nonscalar_image_filter_Functor
						< itk::Image< itk::Vector<ZnReal, 2>, 1 > >
						(filter);
					return_code = 1;
				} break;
				case 3:
				{
					functor = new Computed_field_histogram_nonscalar_image_filter_Functor
						< itk::Image< itk::Vector<ZnReal, 3>, 1 > >
						(filter);
					return_code = 1;
				} break;
				case 4:
				{
					functor = new Computed_field_histogram_nonscalar_image_filter_Functor
						< itk::Image< itk::Vector<ZnReal, 4>, 1 > >
						(filter);
					return_code = 1;
				} break;
				default:
				{
					display_message(ERROR_MESSAGE,
						"computed_field_image_filter::create_filters_multicomponent_multidimensions.  "
						"Template invocation not declared for number of components %d.",
						sourceNumberOfComponents);
					return_code = 0;
				} break;
			}
		} break;
		case 2:
		{
			switch (sourceNumberOfComponents)
			{
				case 1:
				{
					functor = new Computed_field_histogram_scalar_image_filter_Functor
						< itk::Image< ZnReal, 2 > >
						(filter);
					return_code = 1;
				} break;
				case 2:
				{
					functor = new Computed_field_histogram_nonscalar_image_filter_Functor
						< itk::Image< itk::Vector<ZnReal, 2>, 2 > >
						(filter);
					return_code = 1;
				} break;
				case 3:
				{
					functor = new Computed_field_histogram_nonscalar_image_filter_Functor
						< itk::Image< itk::Vector<ZnReal, 3>, 2 > >
						(filter);
					return_code = 1;
				} break;
				case 4:
				{
					functor = new Computed_field_histogram_nonscalar_image_filter_Functor
						< itk::Image< itk::Vector<ZnReal, 4>, 2 > >
						(filter);
					return_code = 1;
				} break;
				default:
				{
					display_message(ERROR_MESSAGE,
						"computed_field_image_filter::create_filters_multicomponent_multidimensions.  "
						"Template invocation not declared for number of components %d.",
						sourceNumberOfComponents);
					return_code = 0;
				} break;
			}
		} break;
		case 3:
		{
			switch (sourceNumberOfComponents)
			{
				case 1:
				{
					functor = new Computed_field_histogram_scalar_image_filter_Functor
						< itk::Image< ZnReal, 3 > >
						(filter);
					return_code = 1;
				} break;
				case 2:
				{
					functor = new Computed_field_histogram_nonscalar_image_filter_Functor
						< itk::Image< itk::Vector<ZnReal, 2>, 3 > >
						(filter);
					return_code = 1;
				} break;
				case 3:
				{
					functor = new Computed_field_histogram_nonscalar_image_filter_Functor
						< itk::Image< itk::Vector<ZnReal, 3>, 3 > >
						(filter);
					return_code = 1;
				} break;
				case 4:
				{
					functor = new Computed_field_histogram_nonscalar_image_filter_Functor
						< itk::Image< itk::Vector<ZnReal, 4>, 3 > >
						(filter);
					return_code = 1;
				} break;
				default:
				{
					display_message(ERROR_MESSAGE,
						"computed_field_image_filter::create_filters_multicomponent_multidimensions.  "
						"Template invocation not declared for number of components %d.",
						sourceNumberOfComponents);
					return_code = 0;
				} break;
			}
		} break;
		default:
		{
			display_message(ERROR_MESSAGE,
				"computed_field_image_filter::create_filters_multicomponent_multidimensions.  "
				"Template invocation not declared for dimension %d.", 
				dimension);
			return_code = 0;
		} break;
	}
	LEAVE;

	return (return_code);
} /* computed_field_image_filter::create_filters_multicomponent_multidimensions */


Computed_field_histogram_image_filter::Computed_field_histogram_image_filter(
	Computed_field *source_field) :	computed_field_image_filter(source_field)
/*******************************************************************************
LAST MODIFIED : 25 March 2008

DESCRIPTION :
Create the computed_field representation of the RescaleIntensityImageFilter.
==============================================================================*/
{
	int i;
	sourceNumberOfComponents = source_field->number_of_components;
	numberOfBins = new int [sourceNumberOfComponents];
	for (i = 0 ; i < sourceNumberOfComponents ; i++)
	{
		numberOfBins[i] = 64;
	}
	histogramMinimum = new double [sourceNumberOfComponents];
	for (i = 0 ; i < sourceNumberOfComponents ; i++)
	{
		histogramMinimum[i] = 0.0;
	}
	histogramMaximum = new double [sourceNumberOfComponents];
	for (i = 0 ; i < sourceNumberOfComponents ; i++)
	{
		histogramMaximum[i] = 1.0;
	}
	marginalScale = 10.0;
	if (dimension > 0 && sizes)
	{
		totalPixels = sizes[0];
		for (i = 1 ; i < dimension ; i++)
		{
			totalPixels *= sizes[i];
		}
	}
}

Computed_field_histogram_image_filter::Computed_field_histogram_image_filter(
	Computed_field *source_field, const int *numberOfBinsIn, double marginalScale,
	const double *histogramMinimumIn, const double *histogramMaximumIn) :
	computed_field_image_filter(source_field), marginalScale(marginalScale)
/*******************************************************************************
LAST MODIFIED : 25 March 2008

DESCRIPTION :
Create the computed_field representation of the RescaleIntensityImageFilter.
==============================================================================*/
{
	int i;
	sourceNumberOfComponents = source_field->number_of_components;
	numberOfBins = new int [sourceNumberOfComponents];
	for (i = 0 ; i < sourceNumberOfComponents ; i++)
	{
		numberOfBins[i] = numberOfBinsIn[i];
	}
	if (histogramMinimumIn)
	{
		histogramMinimum = new double [sourceNumberOfComponents];
		for (i = 0 ; i < sourceNumberOfComponents ; i++)
		{
			histogramMinimum[i] = histogramMinimumIn[i];
		}
	}
	else
		histogramMinimum = (double *)NULL;
	if (histogramMaximumIn)
	{
		histogramMaximum = new double [sourceNumberOfComponents];
		for (i = 0 ; i < sourceNumberOfComponents ; i++)
		{
			histogramMaximum[i] = histogramMaximumIn[i];
		}
	}
	else
		histogramMaximum = (double *)NULL;
	/* We need to leave the sizes and dimension for
		using the functions from computed_field_image_filter::set_input_image */
	if (dimension > 0 && sizes)
	{
		totalPixels = sizes[0];
		for (i = 1 ; i < dimension ; i++)
		{
			totalPixels *= sizes[i];
		}
	}
}

void Computed_field_histogram_image_filter::create_functor()
{
	create_histogram_filters_multicomponent_multidimensions
		< Computed_field_histogram_image_filter >
		(this);
}

} //namespace

cmzn_field_imagefilter_histogram_id cmzn_field_cast_imagefilter_histogram(cmzn_field_id field)
{
	if (dynamic_cast<Computed_field_histogram_image_filter*>(field->core))
	{
		cmzn_field_access(field);
		return (reinterpret_cast<cmzn_field_imagefilter_histogram_id>(field));
	}
	else
	{
		return (NULL);
	}
}

int cmzn_field_imagefilter_histogram_destroy(
	cmzn_field_imagefilter_histogram_id *imagefilter_histogram_address)
{
	return cmzn_field_destroy(reinterpret_cast<cmzn_field_id *>(
		imagefilter_histogram_address));
}

int cmzn_field_imagefilter_histogram_get_compute_minimum_values(
	cmzn_field_imagefilter_histogram_id imagefilter_histogram,
	int valuesCount, double *valuesOut)
{
	if (imagefilter_histogram)
	{
		Computed_field_histogram_image_filter *filter_core =
			Computed_field_histogram_image_filter_core_cast(imagefilter_histogram);
		return filter_core->getHistogramMinimum(valuesCount, valuesOut);
	}
	return 0;
}

int cmzn_field_imagefilter_histogram_set_compute_minimum_values(
	cmzn_field_imagefilter_histogram_id imagefilter_histogram,
	int valuesCount, const double *valuesIn)
{
	if (imagefilter_histogram)
	{
		Computed_field_histogram_image_filter *filter_core =
			Computed_field_histogram_image_filter_core_cast(imagefilter_histogram);
		return filter_core->setHistogramMinimum(valuesCount, valuesIn);
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_field_imagefilter_histogram_get_compute_maximum_values(
	cmzn_field_imagefilter_histogram_id imagefilter_histogram,
	int valuesCount, double *valuesOut)
{
	if (imagefilter_histogram)
	{
		Computed_field_histogram_image_filter *filter_core =
			Computed_field_histogram_image_filter_core_cast(imagefilter_histogram);
		return filter_core->getHistogramMaximum(valuesCount, valuesOut);
	}
	return 0;
}

int cmzn_field_imagefilter_histogram_set_compute_maximum_values(
	cmzn_field_imagefilter_histogram_id imagefilter_histogram,
	int valuesCount, const double *valuesIn)
{
	if (imagefilter_histogram)
	{
		Computed_field_histogram_image_filter *filter_core =
			Computed_field_histogram_image_filter_core_cast(imagefilter_histogram);
		return filter_core->setHistogramMaximum(valuesCount, valuesIn);
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_field_imagefilter_histogram_get_number_of_bins(
	cmzn_field_imagefilter_histogram_id imagefilter_histogram,
	int valuesCount, int *valuesOut)
{
	if (imagefilter_histogram)
	{
		Computed_field_histogram_image_filter *filter_core =
			Computed_field_histogram_image_filter_core_cast(imagefilter_histogram);
		return filter_core->getNumberOfBins(valuesCount, valuesOut);
	}
	return 0;
}

int cmzn_field_imagefilter_histogram_set_number_of_bins(
	cmzn_field_imagefilter_histogram_id imagefilter_histogram,
	int valuesCount, const int *valuesIn)
{
	if (imagefilter_histogram)
	{
		Computed_field_histogram_image_filter *filter_core =
			Computed_field_histogram_image_filter_core_cast(imagefilter_histogram);
		return filter_core->setNumberOfBins(valuesCount, valuesIn);
	}
	return CMZN_ERROR_ARGUMENT;
}

double cmzn_field_imagefilter_histogram_get_marginal_scale(
	cmzn_field_imagefilter_histogram_id imagefilter_histogram)
{
	if (imagefilter_histogram)
	{
		Computed_field_histogram_image_filter *filter_core =
			Computed_field_histogram_image_filter_core_cast(imagefilter_histogram);
		return filter_core->getMaginalScale();
	}
	return 0.0;
}

int cmzn_field_imagefilter_histogram_set_marginal_scale(cmzn_field_imagefilter_histogram_id
	imagefilter_histogram, double marginal_scale)
{
	if (imagefilter_histogram)
	{
		Computed_field_histogram_image_filter *filter_core =
			Computed_field_histogram_image_filter_core_cast(imagefilter_histogram);
		return filter_core->setMaginalScale(marginal_scale);
	}
	return CMZN_ERROR_ARGUMENT;
}

struct Computed_field *cmzn_fieldmodule_create_field_imagefilter_histogram(
	struct cmzn_fieldmodule *field_module,	struct Computed_field *source_field)
{
	Computed_field *field = NULL;
	if (source_field && Computed_field_is_scalar(source_field, (void *)NULL))
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			/*number_of_components*/1,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_histogram_image_filter(source_field));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_imagefilter_histogram.  Invalid argument(s)");
	}

	return (field);
}

int cmzn_field_get_type_histogram_image_filter(struct Computed_field *field,
	struct Computed_field **source_field, int **numberOfBins, double *marginalScale,
	double **histogramMinimum, double **histogramMaximum)
/*******************************************************************************
LAST MODIFIED : 25 March 2008

DESCRIPTION :
If the field is of type histogram, the source_field and histogram_image_filter
used by it are returned - otherwise an error is reported.
==============================================================================*/
{
	Computed_field_histogram_image_filter* core;
	int i, return_code;

	ENTER(cmzn_field_get_type_histogram_image_filter);
	if (field && (core = dynamic_cast<Computed_field_histogram_image_filter*>(field->core))
		&& source_field &&
		ALLOCATE(*numberOfBins, int, core->sourceNumberOfComponents)
		&& (!core->histogramMinimum || ALLOCATE(*histogramMaximum, double, core->sourceNumberOfComponents))
		&& (!core->histogramMaximum || ALLOCATE(*histogramMaximum, double, core->sourceNumberOfComponents)))
	{
		*source_field = field->source_fields[0];
		for (i = 0 ; i < core->sourceNumberOfComponents ; i++)
		{
			(*numberOfBins)[i] = core->numberOfBins[i];
		}
		if (core->histogramMinimum)
		{
			for (i = 0 ; i < core->sourceNumberOfComponents ; i++)
			{
				(*histogramMinimum)[i] = core->histogramMinimum[i];
			}
		}
		else
			*histogramMinimum = (double *)NULL;
		if (core->histogramMaximum)
		{
			for (i = 0 ; i < core->sourceNumberOfComponents ; i++)
			{
				(*histogramMaximum)[i] = core->histogramMaximum[i];
			}
		}
		else
			*histogramMaximum = (double *)NULL;
		*marginalScale = core->marginalScale;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_field_get_type_histogram_image_filter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_field_get_type_histogram_image_filter */

