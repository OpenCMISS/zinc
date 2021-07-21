/*******************************************************************************
FILE : computed_field_derivatives.c

LAST MODIFIED : 24 August 2006

DESCRIPTION :
Implements computed_fields for calculating various derivative quantities such
as derivatives w.r.t. Xi, gradient, curl, divergence etc.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "opencmiss/zinc/zincconfigure.h"
#include "opencmiss/zinc/node.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_coordinate.h"
#include "computed_field/computed_field_derivatives.h"
#include "computed_field/computed_field_private.hpp"
#include "computed_field/computed_field_set.h"
#include "finite_element/finite_element_nodeset.hpp"
#include "image_processing/computed_field_image_filter.h"
#include "general/debug.h"
#include "general/matrix_vector.h"
#include "general/mystring.h"
#include "general/message.h"
#include "itkImage.h"
#include "itkVector.h"
#include "itkDerivativeImageFilter.h"

class Computed_field_derivatives_package : public Computed_field_type_package
{
};

using namespace CMZN;

namespace {

const char computed_field_derivative_type_string[] = "derivative";

class Computed_field_derivative_image_filter : public computed_field_image_filter
{
	/* This class is only used when the input is deemed to be grid based,
		the derivative is not calculable on the input field and the
		get_native_resolution method is implemented. */

public:
	int xi_index;
	int derivative_operator_order;

	Computed_field_derivative_image_filter(Computed_field *source_field,
		int xi_index, int derivative_operator_order);

	~Computed_field_derivative_image_filter()
	{
	};

private:
	virtual void create_functor();

	Computed_field_core *copy()
	{
		return new Computed_field_derivative_image_filter(field->source_fields[0], xi_index, derivative_operator_order);
	}

	const char *get_type_string()
	{
		return(computed_field_derivative_type_string);
	}

	int compare(Computed_field_core* other_field);

	int list();

	char* get_command_string();

};

int Computed_field_derivative_image_filter::compare(Computed_field_core *other_core)
/*******************************************************************************
LAST MODIFIED : 2 July 2007

DESCRIPTION :
Not needed but required to construct object.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_derivative_image_filter::compare);
	USE_PARAMETER(other_core);
	display_message(ERROR_MESSAGE,
		"Computed_field_derivative_image_filter::compare.  Not implemented.");
	return_code = 0;
	LEAVE;

	return (return_code);
} /* Computed_field_derivative_image_filter::compare */

int Computed_field_derivative_image_filter::list()
/*******************************************************************************
LAST MODIFIED : 2 July 2007

DESCRIPTION :
Not needed but required to construct object.
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_derivative_image_filter);
	display_message(ERROR_MESSAGE,
		"Computed_field_derivative_image_filter::list.  Not implemented.");
	return_code = 0;
	LEAVE;

	return (return_code);
} /* list_Computed_field_derivative_image_filter */

char *Computed_field_derivative_image_filter::get_command_string()
/*******************************************************************************
LAST MODIFIED : 2 July 2007

DESCRIPTION :
Not needed but required to construct object.
==============================================================================*/
{
	char *command_string;

	ENTER(Computed_field_derivative_image_filter::get_command_string);
	display_message(ERROR_MESSAGE,
		"Computed_field_derivative_image_filter::get_command_string.  Not implemented.");
	command_string = static_cast<char *>(NULL);
	LEAVE;

	return (command_string);
} /* Computed_field_derivative_image_filter::get_command_string */

template < class ImageType >
class Computed_field_derivative_image_filter_Functor :
	public computed_field_image_filter_FunctorTmpl< ImageType >
/*******************************************************************************
LAST MODIFIED : 2 July 2007

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
LAST MODIFIED : 2 July 2007

DESCRIPTION :
Create a filter of the correct type, set the filter specific parameters
and generate the outputImage.
==============================================================================*/
	{
		int return_code;

		typedef itk::DerivativeImageFilter< ImageType , ImageType > FilterType;

		typename FilterType::Pointer filter = FilterType::New();

		filter->SetDirection( derivative_image_filter->xi_index );
		filter->SetOrder( derivative_image_filter->derivative_operator_order );

		return_code = derivative_image_filter->update_output_image
			(cache, filter, this->outputImage,
			static_cast<ImageType*>(NULL), static_cast<FilterType*>(NULL));

		return (return_code);
	} /* set_filter */

}; /* template < class ImageType > class Computed_field_derivative_image_filter_Functor */

Computed_field_derivative_image_filter::Computed_field_derivative_image_filter(
	Computed_field *source_field, int xi_index, int derivative_operator_order) :
	computed_field_image_filter(source_field),
	xi_index(xi_index), derivative_operator_order(derivative_operator_order)
/*******************************************************************************
LAST MODIFIED : 2 July 2007

DESCRIPTION :
Create the ITK implementation for a Computed_field_derivative.
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

class Computed_field_derivative : public Computed_field_core
{
	/* This is the actual Computed_field implementation.
		Normally this derivative is the xi derivative taken from
		the input_field.  If the derivatives are not defined on the
		input field but the get_native_resolution function is then
		an image based derivative is calculated using ITK.
		It creates a Computed_field_derivative_image_filter if and
		when required. */

public:

	int xi_index;

	Computed_field_derivative_image_filter *derivative_image_filter;

	/** @param xi_indexIn  Internal xi index starting at 0. */
	Computed_field_derivative(int xi_indexIn) :
		Computed_field_core(), xi_index(xi_indexIn)
	{
		/* Only construct the image filter version if it is required */
		derivative_image_filter = (Computed_field_derivative_image_filter *)NULL;
	}

	~Computed_field_derivative()
	{
		if (derivative_image_filter)
		{
			delete derivative_image_filter;
		}
	}

private:
	Computed_field_core *copy()
	{
		return new Computed_field_derivative(this->xi_index);
	}

	const char *get_type_string()
	{
		return(computed_field_derivative_type_string);
	}

	virtual enum cmzn_field_type get_type()
	{
		return CMZN_FIELD_TYPE_DERIVATIVE;
	}

	int compare(Computed_field_core* other_field);

	virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative);

	int list();

	char* get_command_string();

	virtual bool is_defined_at_location(cmzn_fieldcache& cache);

};

int Computed_field_derivative::compare(Computed_field_core *other_core)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Compare the type specific data
==============================================================================*/
{
	Computed_field_derivative* other;
	int return_code;

	ENTER(Computed_field_derivative::compare);
	USE_PARAMETER(other_core);
	if (field && (other = dynamic_cast<Computed_field_derivative*>(field->core)))
	{
		if (xi_index == other->xi_index)
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
} /* Computed_field_derivative::compare */

bool Computed_field_derivative::is_defined_at_location(cmzn_fieldcache& cache)
{
	// derivative values are only defined for element_xi locations, and only up to element dimension...
	const Field_location_element_xi* element_xi_location;
	if ((element_xi_location = cache.get_location_element_xi()) &&
		(xi_index < element_xi_location->get_element_dimension()))
	{
		// check the source fields
		return Computed_field_core::is_defined_at_location(cache);
	}
	// ... or image based field derivative with field values location
	else if (cache.get_location_field_values())
	{
		/* This can only be valid if the input field has
			a native resolution as we will be using image filter. */
		if (Computed_field_core::is_defined_at_location(cache))
		{
			int dimension;
			int *sizes = static_cast<int *>(NULL);
			Computed_field *texture_coordinate_field;
			bool result = Computed_field_get_native_resolution(
				field->source_fields[0], &dimension, &sizes,
				&texture_coordinate_field);
			if (sizes)
			{
				DEALLOCATE(sizes);
			}
			return result;
		}
	}
	return false;
}

int Computed_field_derivative::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	/* Only works for element_xi locations, or field locations for image-based fields */
	const Field_location_element_xi* element_xi_location = cache.get_location_element_xi();
	if (element_xi_location)
	{
		cmzn_element* element = element_xi_location->get_element();
		const int element_dimension = element_xi_location->get_element_dimension();
		if (this->xi_index < element_dimension)
		{
			RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
			const FieldDerivative& fieldDerivative = *element->getMesh()->getFieldDerivative(/*order*/1);
			const DerivativeValueCache *sourceDerivativeCache = getSourceField(0)->evaluateDerivative(cache, fieldDerivative);
			if (sourceDerivativeCache)
			{
				const FE_value *sourceDerivatives = sourceDerivativeCache->values + this->xi_index;
				for (int i = 0; i < field->number_of_components; ++i)
					valueCache.values[i] = sourceDerivatives[i*element_dimension];
				return 1;
			}
		}
	}
	else
	{
		int dimension;
		int *sizes = static_cast<int *>(NULL);
		Computed_field *texture_coordinate_field;
		/* If it isn't calculated then try with the ImageFilter */
		if (Computed_field_get_native_resolution(field->source_fields[0],
			&dimension, &sizes, &texture_coordinate_field))
		{
			if (!derivative_image_filter)
			{
				/* Hard coding the default first order operator for now */
				derivative_image_filter = new
					Computed_field_derivative_image_filter(field->source_fields[0],
						xi_index, /*derivative_operator_order*/1);
				// Note: following attaches another core to the same field
				if (derivative_image_filter)
				{
					derivative_image_filter->attach_to_field(field);
				}
			}
			if (sizes)
			{
				DEALLOCATE(sizes);
			}
			if (derivative_image_filter && derivative_image_filter->evaluate(cache, inValueCache))
			{
				return 1;
			}
		}
	}
	return 0;
}

int Computed_field_derivative::evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
{
	// Only implemented for derivative w.r.t. element xi coordinates of same mesh
	const Field_location_element_xi* element_xi_location = cache.get_location_element_xi();
	if (element_xi_location)
	{
		cmzn_element* element = element_xi_location->get_element();
		FE_mesh *mesh = element->getMesh();
		const int elementDimension = mesh->getDimension();
		// following fails if there is a mesh mismatch:
		const FieldDerivative *sourceFieldDerivative = mesh->getHigherFieldDerivative(fieldDerivative);
		if (!sourceFieldDerivative)
			return this->evaluateDerivativeFiniteDifference(cache, inValueCache, fieldDerivative);
		if (this->xi_index < elementDimension)
		{
			const DerivativeValueCache *sourceDerivativeCache = getSourceField(0)->evaluateDerivative(cache, *sourceFieldDerivative);
			if (!sourceDerivativeCache)
				return 0;
			DerivativeValueCache *derivativeCache = inValueCache.getDerivativeValueCache(fieldDerivative);
			const int termCount = derivativeCache->getTermCount();
			const int sourceTermCount = sourceDerivativeCache->getTermCount();
			// the extra mesh derivative added here becomes the first/outermost index
			FE_value *derivatives = derivativeCache->values;
			const FE_value *sourceDerivatives = sourceDerivativeCache->values + this->xi_index*termCount;
			const int componentCount = field->number_of_components;
			for (int i = 0; i < componentCount; ++i)
			{
				for (int j = 0; j < termCount; ++j)
					derivatives[j] = sourceDerivatives[j];
				derivatives += termCount;
				sourceDerivatives += sourceTermCount;
			}
			return 1;
		}
	}
	return 0;
}

int Computed_field_derivative::list()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_derivative);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    field : %s\n",field->source_fields[0]->name);
		display_message(INFORMATION_MESSAGE,
			"    xi number : %d\n",xi_index + 1);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_derivative.  "
			"Invalid arguments.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_derivative */

char *Computed_field_derivative::get_command_string()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name, temp_string[40];
	int error;

	ENTER(Computed_field_derivative::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_derivative_type_string, &error);
		append_string(&command_string, " field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
		sprintf(temp_string, " xi_index %d", xi_index + 1);
		append_string(&command_string, temp_string, &error);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_derivative::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_derivative::get_command_string */

} //namespace

int cmzn_field_derivative_get_xi_index(cmzn_field_id field)
{
	int index = -1;
	if (field && field->core)
	{
		Computed_field_derivative *fieldDerivative= static_cast<Computed_field_derivative*>(
			field->core);
		index = fieldDerivative->xi_index + 1;
	}
	return index;
}

cmzn_field_id cmzn_fieldmodule_create_field_derivative(
	cmzn_fieldmodule_id field_module,
	cmzn_field_id source_field, int xi_index)
{
	cmzn_field *field = NULL;
	if (source_field && source_field->isNumerical() &&
		(0 < xi_index) && (xi_index <= MAXIMUM_ELEMENT_XI_DIMENSIONS))
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field->number_of_components,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_derivative(xi_index - 1));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_derivative.  Invalid argument(s)");
	}

	return (field);
}

int Computed_field_get_type_derivative(struct Computed_field *field,
	struct Computed_field **source_field, int *xi_index)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_DERIVATIVE, the
<source_field> and <xi_index> used by it are returned.
==============================================================================*/
{
	Computed_field_derivative* derivative_core;
	int return_code;

	ENTER(Computed_field_get_type_derivative);
	if (field &&
		(derivative_core = dynamic_cast<Computed_field_derivative*>(field->core))
		&& source_field)
	{
		*source_field = field->source_fields[0];
		*xi_index = derivative_core->xi_index;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_derivative.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_derivative */

namespace {

const char computed_field_curl_type_string[] = "curl";

class Computed_field_curl : public Computed_field_core
{
public:
	Computed_field_curl() : Computed_field_core()
	{
	};

private:
	Computed_field_core *copy()
	{
		return new Computed_field_curl();
	}

	const char *get_type_string()
	{
		return(computed_field_curl_type_string);
	}

	virtual enum cmzn_field_type get_type()
	{
		return CMZN_FIELD_TYPE_CURL;
	}

	int compare(Computed_field_core* other_field)
	{
		if (dynamic_cast<Computed_field_curl*>(other_field))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
	{
		return this->evaluateDerivativeFiniteDifference(cache, inValueCache, fieldDerivative);
	}

	int list();

	char* get_command_string();

};

/**
 * Evaluates the curl of a vector field.
 * If function fails to invert the coordinate derivatives then the curl is
 * returned as 0 with a warning - as may happen at certain locations of the mesh.
 * Note currently requires vector_field to be RC.
 */
int Computed_field_curl::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	const Field_location_element_xi* element_xi_location = cache.get_location_element_xi();
	if (element_xi_location)
	{
		RealFieldValueCache& valueCache = RealFieldValueCache::cast(inValueCache);
		cmzn_field_id source_field = getSourceField(0);
		cmzn_field_id coordinate_field = getSourceField(1);
		cmzn_element* element = element_xi_location->get_element();
		const int element_dimension = element_xi_location->get_element_dimension();
		cmzn_element *top_level_element = element_xi_location->get_top_level_element();
		FE_value top_level_xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
		int top_level_element_dimension = 0;
		FE_element_get_top_level_element_and_xi(element,
			element_xi_location->get_xi(), element_dimension,
			&top_level_element, top_level_xi, &top_level_element_dimension);
		// use the normal cache if already on a top level element, otherwise use extra cache
		cmzn_fieldcache *workingCache = &cache;
		if (top_level_element != element)
		{
			workingCache = valueCache.getOrCreateSharedExtraCache(cache);
			workingCache->setTime(cache.getTime());
			workingCache->setMeshLocation(top_level_element, top_level_xi);
		}
		const FieldDerivative& fieldDerivative = *top_level_element->getMesh()->getFieldDerivative(/*order*/1);
		const DerivativeValueCache *sourceDerivativeCache = source_field->evaluateDerivative(*workingCache, fieldDerivative);
		const RealFieldValueCache *coordinateValueCache = RealFieldValueCache::cast(coordinate_field->evaluateDerivativeTree(*workingCache, fieldDerivative));
		if (sourceDerivativeCache && coordinateValueCache)
		{
			const DerivativeValueCache *coordinateDerivativeCache = coordinateValueCache->getDerivativeValueCache(fieldDerivative);
			FE_value curl,dx_dxi[9],dxi_dx[9],x[3],*source;
			/* curl is only valid in 3 dimensions */
			// Constructor already checked (3==element_dimension)&&(3==coordinate_number_of_components)
			/* only support RC vector fields */
			if (RECTANGULAR_CARTESIAN == source_field->coordinate_system.type)
			{
				cmzn_field_id coordinate_field = getSourceField(1);
				if (convert_coordinates_and_derivatives_to_rc(&(coordinate_field->coordinate_system),
					coordinate_field->number_of_components, coordinateValueCache->values, coordinateDerivativeCache->values,
					top_level_element_dimension, x, dx_dxi))
				{
					if (invert_FE_value_matrix3(dx_dxi,dxi_dx))
					{
						source = sourceDerivativeCache->values;
						/* curl[0] = dVz/dy - dVy/dz */
						curl=0.0;
						for (int i=0;i<top_level_element_dimension;i++)
						{
							curl += (source[6+i]*dxi_dx[3*i+1] - source[3+i]*dxi_dx[3*i+2]);
						}
						valueCache.values[0]=curl;
						/* curl[1] = dVx/dz - dVz/dx */
						curl=0.0;
						for (int i=0;i<top_level_element_dimension;i++)
						{
							curl += (source[  i]*dxi_dx[3*i+2] - source[6+i]*dxi_dx[3*i  ]);
						}
						valueCache.values[1]=curl;
						/* curl[2] = dVy/dx - dVx/dy */
						curl=0.0;
						for (int i=0;i<top_level_element_dimension;i++)
						{
							curl += (source[3+i]*dxi_dx[3*i  ] - source[  i]*dxi_dx[3*i+1]);
						}
						valueCache.values[2]=curl;
					}
					else
					{
						display_message(WARNING_MESSAGE,
							"Could not invert coordinate derivatives; setting curl to 0");
						for (int i=0;i<field->number_of_components;i++)
						{
							valueCache.values[i]=0.0;
						}
					}
					return 1;
				}
			}
			else
			{
				//display_message(ERROR_MESSAGE,
				//	"Computed_field_evaluate_curl.  Vector field must be RC");
			}
		}
	}
	return 0;
}

int Computed_field_curl::list()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_curl);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    coordinate field : %s\n",field->source_fields[1]->name);
		display_message(INFORMATION_MESSAGE,
			"    vector field : %s\n",field->source_fields[0]->name);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_curl.  "
			"Invalid arguments.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_curl */

char *Computed_field_curl::get_command_string()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name;
	int error;

	ENTER(Computed_field_curl::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_curl_type_string, &error);
		append_string(&command_string, " coordinate ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[1], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
		append_string(&command_string, " vector ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_curl::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_curl::get_command_string */

} //namespace

cmzn_field_id cmzn_fieldmodule_create_field_curl(
	cmzn_fieldmodule_id field_module,
	cmzn_field_id vector_field, cmzn_field_id coordinate_field)
{
	cmzn_field *field = NULL;
	if (vector_field && vector_field->isNumerical() && (3 == vector_field->number_of_components) &&
		coordinate_field && coordinate_field->isNumerical() && (3 == coordinate_field->number_of_components) &&
		(RECTANGULAR_CARTESIAN == vector_field->coordinate_system.type))
	{
		cmzn_field *source_fields[2];
		source_fields[0] = vector_field;
		source_fields[1] = coordinate_field;
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			vector_field->number_of_components,
			/*number_of_source_fields*/2, source_fields,
			/*number_of_source_values*/0, NULL,
			new Computed_field_curl());
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_curl.  Invalid argument(s)");
	}

	return (field);
}

int Computed_field_get_type_curl(struct Computed_field *field,
	struct Computed_field **vector_field,struct Computed_field **coordinate_field)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_CURL, the
<source_field> and <coordinate_field> used by it are returned.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_get_type_curl);
	if (field&&(dynamic_cast<Computed_field_curl*>(field->core))
		&&vector_field&&coordinate_field)
	{
		/* source_fields: 0=vector, 1=coordinate */
		*vector_field = field->source_fields[0];
		*coordinate_field=field->source_fields[1];
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_curl.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_curl */

namespace {

const char computed_field_divergence_type_string[] = "divergence";

class Computed_field_divergence : public Computed_field_core
{
public:
	Computed_field_divergence() : Computed_field_core()
	{
	};

private:
	Computed_field_core *copy()
	{
		return new Computed_field_divergence();
	}

	const char *get_type_string()
	{
		return(computed_field_divergence_type_string);
	}

	virtual enum cmzn_field_type get_type()
	{
		return CMZN_FIELD_TYPE_DIVERGENCE;
	}

	int compare(Computed_field_core* other_field)
	{
		if (dynamic_cast<Computed_field_divergence*>(other_field))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
	{
		return this->evaluateDerivativeFiniteDifference(cache, inValueCache, fieldDerivative);
	}

	int list();

	char* get_command_string();

};

/**
 * Evaluates the divergence of a vector field.
 * If function fails to invert the coordinate derivatives then the divergence is
 * returned as 0 with a warning - as may happen at certain locations of the mesh.
 * Note currently requires vector_field to be RC.
 */
int Computed_field_divergence::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache& valueCache = RealFieldValueCache::cast(inValueCache);
	const Field_location_element_xi* element_xi_location = cache.get_location_element_xi();
	if (element_xi_location)
	{
		cmzn_field_id source_field = getSourceField(0);
		cmzn_field_id coordinate_field = getSourceField(1);
		cmzn_element* element = element_xi_location->get_element();
		const int element_dimension = element_xi_location->get_element_dimension();
		cmzn_element *top_level_element = element_xi_location->get_top_level_element();
		FE_value top_level_xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
		int top_level_element_dimension = 0;
		FE_element_get_top_level_element_and_xi(element,
			element_xi_location->get_xi(), element_dimension,
			&top_level_element, top_level_xi, &top_level_element_dimension);
		// use the normal cache if already on a top level element, otherwise use extra cache
		cmzn_fieldcache *workingCache = &cache;
		if (top_level_element != element)
		{
			workingCache = valueCache.getOrCreateSharedExtraCache(cache);
			workingCache->setTime(cache.getTime());
			workingCache->setMeshLocation(top_level_element, top_level_xi);
		}
		const FieldDerivative& fieldDerivative = *top_level_element->getMesh()->getFieldDerivative(/*order*/1);
		const DerivativeValueCache *sourceDerivativeCache = source_field->evaluateDerivative(*workingCache, fieldDerivative);
		const RealFieldValueCache *coordinateValueCache = RealFieldValueCache::cast(coordinate_field->evaluateDerivativeTree(*workingCache, fieldDerivative));
		if (sourceDerivativeCache && coordinateValueCache)
		{
			const DerivativeValueCache *coordinateDerivativeCache = coordinateValueCache->getDerivativeValueCache(fieldDerivative);
			FE_value divergence,dx_dxi[9],dxi_dx[9],x[3],*source;
			int coordinate_components = coordinate_field->number_of_components;
			/* Following asks: can dx_dxi be inverted? */
			if (((3==top_level_element_dimension)&&(3==coordinate_components))||
				((RECTANGULAR_CARTESIAN==coordinate_field->coordinate_system.type)
					&&(coordinate_components==top_level_element_dimension))||
				((CYLINDRICAL_POLAR==coordinate_field->coordinate_system.type)&&
					(2==top_level_element_dimension)&&(2==coordinate_components)))
			{
				/* only support RC vector fields */
				if (RECTANGULAR_CARTESIAN == source_field->coordinate_system.type)
				{
					if (convert_coordinates_and_derivatives_to_rc(&(coordinate_field->coordinate_system),
						coordinate_field->number_of_components, coordinateValueCache->values, coordinateDerivativeCache->values,
						top_level_element_dimension, x, dx_dxi))
					{
						/* if the element_dimension is less than 3, put ones on the main
							 diagonal to allow inversion of dx_dxi */
						if (3>top_level_element_dimension)
						{
							dx_dxi[8]=1.0;
							if (2>top_level_element_dimension)
							{
								dx_dxi[4]=1.0;
							}
						}
						if (invert_FE_value_matrix3(dx_dxi,dxi_dx))
						{
							divergence=0.0;
							source = sourceDerivativeCache->values;
							for (int i=0;i<top_level_element_dimension;i++)
							{
								for (int j=0;j<top_level_element_dimension;j++)
								{
									divergence += (*source) * dxi_dx[3*j+i];
									source++;
								}
							}
							valueCache.values[0]=divergence;
						}
						else
						{
							display_message(WARNING_MESSAGE,
								"Could not invert coordinate derivatives; "
								"setting divergence to 0");
							valueCache.values[0]=0.0;
						}
						return 1;
					}
				}
				else
				{
					//display_message(ERROR_MESSAGE,
					//	"Computed_field_divergence::evaluate.  Vector field must be RC");
				}
			}
			else
			{
				//display_message(ERROR_MESSAGE,
				//	"Computed_field_divergence::evaluate.  Elements of wrong dimension");
			}
		}
	}
	return 0;
}

int Computed_field_divergence::list()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_divergence);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    coordinate field : %s\n",field->source_fields[1]->name);
		display_message(INFORMATION_MESSAGE,
			"    vector field : %s\n",field->source_fields[0]->name);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_divergence.  "
			"Invalid arguments.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_divergence */

char *Computed_field_divergence::get_command_string()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name;
	int error;

	ENTER(Computed_field_divergence::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_divergence_type_string, &error);
		append_string(&command_string, " coordinate ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[1], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
		append_string(&command_string, " vector ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_divergence::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_divergence::get_command_string */

} //namespace

cmzn_field_id cmzn_fieldmodule_create_field_divergence(
	cmzn_fieldmodule_id field_module,
	cmzn_field_id vector_field, cmzn_field_id coordinate_field)
{
	cmzn_field *field = NULL;
	if (vector_field && vector_field->isNumerical() &&
		coordinate_field && coordinate_field->isNumerical() &&
		(3 >= coordinate_field->number_of_components) &&
		(vector_field->number_of_components ==
			coordinate_field->number_of_components) &&
		(RECTANGULAR_CARTESIAN == vector_field->coordinate_system.type))
	{
		cmzn_field *source_fields[2];
		source_fields[0] = vector_field;
		source_fields[1] = coordinate_field;
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			/*number_of_components*/1,
			/*number_of_source_fields*/2, source_fields,
			/*number_of_source_values*/0, NULL,
			new Computed_field_divergence());
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_divergence.  Invalid argument(s)");
	}

	return (field);
}

int Computed_field_get_type_divergence(struct Computed_field *field,
	struct Computed_field **vector_field,struct Computed_field **coordinate_field)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_DIVERGENCE, the
<source_field> and <coordinate_field> used by it are returned.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_get_type_divergence);
	if (field&&(dynamic_cast<Computed_field_divergence*>(field->core))
		&&vector_field&&coordinate_field)
	{
		/* source_fields: 0=vector, 1=coordinate */
		*vector_field = field->source_fields[0];
		*coordinate_field=field->source_fields[1];
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_divergence.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_divergence */

namespace {

const char computed_field_gradient_type_string[] = "gradient";

/** Derived value cache which computes perturbation size from range of
  * coordinate field, and has storage for temporary arrays. */
class GradientRealFieldValueCache : public RealFieldValueCache
{
public:
	bool needEvaluatePerturbationSize;
	FE_value perturbationSize;
	std::vector<FE_value> cacheValues;

	GradientRealFieldValueCache(int sourceComponentCount, int coordinateComponentCount) :
		RealFieldValueCache(sourceComponentCount*coordinateComponentCount),
		needEvaluatePerturbationSize(true),
		perturbationSize(1.0E-5),
		cacheValues(sourceComponentCount*2 + coordinateComponentCount*3)  // large enough for evaluate and getNodePerturbationSize
	{
	}

	virtual ~GradientRealFieldValueCache()
	{
	}

	FE_value getPerturbationSize(cmzn_fieldcache& cache, cmzn_field *coordinateField, const cmzn_fieldcache& source_cache)
	{
		// GRC only implemented properly for node location!
		if (this->needEvaluatePerturbationSize)
		{
			this->needEvaluatePerturbationSize = false;
			this->perturbationSize = 1.0E-5;
			const Field_location_node *node_location = source_cache.get_location_node();
			if (node_location)
			{
				const int componentCount = coordinateField->number_of_components;
				FE_value *minimums = this->cacheValues.data();
				FE_value *maximums = minimums + componentCount;
				FE_value *values = maximums + componentCount;
				FE_nodeset *feNodeset = FE_node_get_FE_nodeset(node_location->get_node());
				// get range of coordinate field over nodeset
				cmzn_nodeiterator *iter = feNodeset->createNodeiterator();
				if (iter)
				{
					cmzn_node *node;
					bool first = true;
					while ((node = iter->nextNode()))
					{
						cache.setNode(node);
						if (CMZN_OK == cmzn_field_evaluate_real(coordinateField, &cache, componentCount, values))
						{
							if (first)
							{
								for (int c = 0; c < componentCount; ++c)
								{
									minimums[c] = maximums[c] = values[c];
								}
								first = false;
							}
							else
							{
								for (int c = 0; c < componentCount; ++c)
								{
									if (values[c] < minimums[c])
									{
										minimums[c] = values[c];
									}
									else if (values[c] > maximums[c])
									{
										maximums[c] = values[c];
									}
								}
							}
						}
					}
					cmzn_nodeiterator_destroy(&iter);
					if (!first)
					{
						first = true;
						FE_value maximumRange = 0.0;
						for (int c = 0; c < componentCount; ++c)
						{
							values[c] = maximums[c] - minimums[c];
							if (values[c] > 0.0)
							{
								if (first)
								{
									maximumRange = values[c];
									first = false;
								}
								else if (values[c] > maximumRange)
								{
									maximumRange = values[c];
								}
							}
						}
						if (maximumRange > 0.0)
						{
							this->perturbationSize = maximumRange*1.0E-6;
						}
					}
				}
			}
		}
		return this->perturbationSize;
	}

	static GradientRealFieldValueCache* cast(FieldValueCache* valueCache)
	{
		return FIELD_VALUE_CACHE_CAST<GradientRealFieldValueCache*>(valueCache);
	}

	static GradientRealFieldValueCache& cast(FieldValueCache& valueCache)
	{
		return FIELD_VALUE_CACHE_CAST<GradientRealFieldValueCache&>(valueCache);
	}

};

class Computed_field_gradient : public Computed_field_core
{
public:
	Computed_field_gradient() : Computed_field_core()
	{
	};

private:
	~Computed_field_gradient()
	{
	}

	Computed_field_core *copy()
	{
		return new Computed_field_gradient();
	}

	const char *get_type_string()
	{
		return(computed_field_gradient_type_string);
	}

	virtual enum cmzn_field_type get_type()
	{
		return CMZN_FIELD_TYPE_GRADIENT;
	}

	int compare(Computed_field_core* other_field)
	{
		if (dynamic_cast<Computed_field_gradient*>(other_field))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	virtual FieldValueCache *createValueCache(cmzn_fieldcache& /*fieldCache*/)
	{
		return new GradientRealFieldValueCache(
			getSourceField(0)->number_of_components,
			getSourceField(1)->number_of_components);
	}

	virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative);

	int list();

	char* get_command_string();
};

int Computed_field_gradient::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	int return_code = 0;
	GradientRealFieldValueCache& valueCache = GradientRealFieldValueCache::cast(inValueCache);
	const Field_location_element_xi* element_xi_location = cache.get_location_element_xi();
	cmzn_field_id sourceField = this->getSourceField(0);
	cmzn_field_id coordinateField = this->getSourceField(1);
	const int sourceComponentCount = sourceField->number_of_components;
	const int coordinateComponentCount = coordinateField->number_of_components;
	if (element_xi_location)
	{
		cmzn_element* element = element_xi_location->get_element();
		const int element_dimension = element_xi_location->get_element_dimension();
		cmzn_element *top_level_element = element_xi_location->get_top_level_element();
		FE_value top_level_xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
		int top_level_element_dimension = 0;
		FE_element_get_top_level_element_and_xi(element,
			element_xi_location->get_xi(), element_dimension,
			&top_level_element, top_level_xi, &top_level_element_dimension);
		// use the normal cache if already on a top level element, otherwise use extra cache
		cmzn_fieldcache *workingCache = &cache;
		if (top_level_element != element)
		{
			workingCache = valueCache.getOrCreateSharedExtraCache(cache);
			workingCache->setTime(cache.getTime());
			workingCache->setMeshLocation(top_level_element, top_level_xi);
		}
		if (top_level_element_dimension != coordinateComponentCount)
		{
			display_message(ERROR_MESSAGE, "FieldGradient evaluate  Cannot invert coordinate derivatives as element dimension is not equal to number of components");
			return 0;
		}
		const FieldDerivative& fieldDerivative = *top_level_element->getMesh()->getFieldDerivative(/*order*/1);
		const DerivativeValueCache *sourceDerivativeCache = sourceField->evaluateDerivative(*workingCache, fieldDerivative);
		const DerivativeValueCache *coordinateDerivativeCache = coordinateField->evaluateDerivative(*workingCache, fieldDerivative);
		if (sourceDerivativeCache && coordinateDerivativeCache)
		{
			// invert to get derivatives of xi w.r.t. coordinates
			const FE_value *dx_dxi = coordinateDerivativeCache->values;
			FE_value dxi_dx[9];
			return_code = 0;
			if (coordinateComponentCount == 3)
				return_code = invert_FE_value_matrix3(dx_dxi, dxi_dx);
			else if (coordinateComponentCount == 2)
				return_code = invert_FE_value_matrix2(dx_dxi, dxi_dx);
			else if ((coordinateComponentCount == 1) && (fabs(*dx_dxi) > 0.0))
			{
				*dxi_dx = 1.0 / *dx_dxi;
				return_code = 1;
			}
			if (!return_code)
			{
				// cannot invert at e.g. apex of heart, so set to zero so can view values elsewhere
				display_message(WARNING_MESSAGE,
					"FieldGradient evaluate.  Could not invert coordinate derivatives; setting gradient to 0");
				valueCache.zeroValues();
				return 1;
			}
			FE_value *values = valueCache.values;
			const FE_value *sourceDerivatives = sourceDerivativeCache->values;
			for (int i = 0; i < sourceComponentCount; ++i)
			{
				for (int j = 0; j < coordinateComponentCount; ++j)
				{
					FE_value sum = 0.0;
					for (int k = 0; k < coordinateComponentCount; ++k)
						sum += sourceDerivatives[k]*dxi_dx[k*coordinateComponentCount + j];
					values[j] = sum;
				}
				values += coordinateComponentCount;
				sourceDerivatives += coordinateComponentCount;
			}
			return 1;
		}
	}
	else // Do a finite difference calculation varying the coordinate field: should work with any location, provided field is a function of it
	{
		// temporary arrays are packed in GradientRealFieldValueCache::cacheValues:
		FE_value *down_values = valueCache.cacheValues.data();
		FE_value *up_values = down_values + sourceComponentCount;
		FE_value *coordinate_values = up_values + sourceComponentCount;

		// evaluate current coordinates in original cache; allows in-cache values to be picked up
		// see field assignment / computed_field_update
		const RealFieldValueCache *originalCoordinateValueCache = RealFieldValueCache::cast(coordinateField->evaluate(cache));
		// do finite difference calculations in extraCache:
		cmzn_fieldcache *extraCache = valueCache.getOrCreateSharedExtraCache(cache);
		if ((originalCoordinateValueCache) && (extraCache))
		{
			const FE_value perturbationSize = valueCache.getPerturbationSize(*extraCache, coordinateField, cache);
			extraCache->copyLocation(cache);
			const FE_value *original_coordinate_values = originalCoordinateValueCache->values;
			return_code = 1;
			for (int i = 0; i < coordinateComponentCount; ++i)
			{
				coordinate_values[i] = original_coordinate_values[i];
			}
			for (int i = 0 ; i < coordinateComponentCount ; ++i)
			{
				for (int k = 0 ; k < 2 ; ++k)
				{
					FE_value *field_values;
					if (k == 0)
					{
						coordinate_values[i] -= perturbationSize;
						field_values = down_values;
					}
					else
					{
						coordinate_values[i] += perturbationSize;
						field_values = up_values;
					}
					/* Set the coordinate field values in cache only and evaluate the source field */
					extraCache->setAssignInCacheOnly(true);
					if (CMZN_OK == cmzn_field_assign_real(coordinateField, extraCache,
						coordinateComponentCount, coordinate_values))
					{
						const RealFieldValueCache *sourceValueCache = RealFieldValueCache::cast(sourceField->evaluate(*extraCache));
						if (sourceValueCache)
						{
							for (int m = 0; m < sourceComponentCount; ++m)
							{
								field_values[m] = sourceValueCache->values[m];
							}
						}
						else
						{
							display_message(WARNING_MESSAGE,
								"Computed_field_gradient::evaluate.  "
								"Unable to evaluate source field when evaluating finite difference.");
							return_code = 0;
							break;
						}
					}
					else
					{
						display_message(WARNING_MESSAGE,
							"Computed_field_gradient::evaluate.  "
							"Unable to set coordinate field when evaluating finite difference.");
						return_code = 0;
						break;
					}
					// restore original coordinate
					coordinate_values[i] = original_coordinate_values[i];
				}
				for (int j = 0 ; j < sourceComponentCount ; ++j)
				{
					valueCache.values[j*coordinateComponentCount + i] =
						(up_values[j] - down_values[j]) / (2.0*perturbationSize);
				}
			}
			extraCache->setAssignInCacheOnly(false);
		}
	}
	return (return_code);
}

int Computed_field_gradient::evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
{
	const Field_location_element_xi* element_xi_location = cache.get_location_element_xi();
	cmzn_field_id coordinateField = this->getSourceField(1);
	const int coordinateOrder = coordinateField->getDerivativeTreeOrder(fieldDerivative);
	if ((!element_xi_location) || (coordinateOrder > 1) || (coordinateOrder > fieldDerivative.getMeshOrder()))
		return this->evaluateDerivativeFiniteDifference(cache, inValueCache, fieldDerivative);
	cmzn_field_id sourceField = this->getSourceField(0);
	const int sourceComponentCount = sourceField->number_of_components;
	const int coordinateComponentCount = coordinateField->number_of_components;
	cmzn_element* element = element_xi_location->get_element();
	const int elementDimension = element_xi_location->get_element_dimension();
	cmzn_element *top_level_element = element_xi_location->get_top_level_element();
	FE_value top_level_xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	int top_level_element_dimension = 0;
	FE_element_get_top_level_element_and_xi(element,
		element_xi_location->get_xi(), elementDimension,
		&top_level_element, top_level_xi, &top_level_element_dimension);
	// this shouldn't happen as derivatives w.r.t. parameters can only be done on top-level-elements
	if (top_level_element != element)
		return this->evaluateDerivativeFiniteDifference(cache, inValueCache, fieldDerivative);

	//GradientRealFieldValueCache& valueCache = GradientRealFieldValueCache::cast(inValueCache);
	if (top_level_element_dimension != coordinateComponentCount)
	{
		display_message(ERROR_MESSAGE, "FieldGradient evaluateDerivative  Cannot invert coordinate derivatives as element dimension is not equal to number of components");
		return 0;
	}
	// need only first derivatives w.r.t. mesh to get dx/dxi to invert
	FE_mesh *mesh = element->getMesh();
	const FieldDerivative *coordinateFieldDerivative = mesh->getFieldDerivative(/*order*/1);
	// following fails if there is a mesh mismatch (but not now as we don't yet allow mesh derivatives in):
	const FieldDerivative *sourceFieldDerivative = mesh->getHigherFieldDerivative(fieldDerivative);
	if (!sourceFieldDerivative)
		return this->evaluateDerivativeFiniteDifference(cache, inValueCache, fieldDerivative);
	const DerivativeValueCache *sourceDerivativeCache = sourceField->evaluateDerivative(cache, *sourceFieldDerivative);
	const DerivativeValueCache *coordinateDerivativeCache = coordinateField->evaluateDerivative(cache, *coordinateFieldDerivative);
	if (!((sourceDerivativeCache) && (coordinateDerivativeCache)))
		return 0;
	// invert to get derivatives of xi w.r.t. coordinates
	const FE_value *dx_dxi = coordinateDerivativeCache->values;
	FE_value dxi_dx[9];
	int inverted = 0;
	if (coordinateComponentCount == 3)
		inverted = invert_FE_value_matrix3(dx_dxi, dxi_dx);
	else if (coordinateComponentCount == 2)
		inverted = invert_FE_value_matrix2(dx_dxi, dxi_dx);
	else if ((coordinateComponentCount == 1) && (fabs(*dx_dxi) > 0.0))
	{
		*dxi_dx = 1.0 / *dx_dxi;
		inverted = 1;
	}
	DerivativeValueCache *derivativeCache = inValueCache.getDerivativeValueCache(fieldDerivative);
	if (!inverted)
	{
		// cannot invert at e.g. apex of heart, so set to zero so can view values elsewhere
		display_message(WARNING_MESSAGE,
			"FieldGradient evaluateDerivative.  Could not invert coordinate derivatives; setting gradient derivative to 0");
		derivativeCache->zeroValues();
		return 1;
	}
	const int termCount = derivativeCache->getTermCount();
	const int sourceTermCount = sourceDerivativeCache->getTermCount();
	FE_value *derivatives = derivativeCache->values;
	const FE_value *sourceDerivatives = sourceDerivativeCache->values;
	for (int i = 0; i < sourceComponentCount; ++i)
	{
		for (int j = 0; j < coordinateComponentCount; ++j)
		{
			for (int t = 0; t < termCount; ++t)
			{
				FE_value sum = 0.0;
				// note: the mesh derivative changes slowest
				for (int k = 0; k < elementDimension; ++k)
					sum += sourceDerivatives[k*termCount + t]*dxi_dx[k*coordinateComponentCount + j];
				derivatives[t] = sum;
			}
			derivatives += termCount;
		}
		sourceDerivatives += sourceTermCount;
	}
	if (coordinateOrder == 0)
		return 1;
	// add extra terms for variation in coordinates
	const FieldDerivative *coordinateFieldDerivative2 = mesh->getFieldDerivative(/*order*/2);
	const DerivativeValueCache *sourceDerivativeCache1 = sourceField->evaluateDerivative(cache, fieldDerivative);
	const DerivativeValueCache *coordinateDerivativeCache2 = coordinateField->evaluateDerivative(cache, *coordinateFieldDerivative2);
	if (!((sourceDerivativeCache1) && (coordinateDerivativeCache2)))
		return 0;
	const int coordinateTermCount2 = coordinateDerivativeCache2->getTermCount();
	FE_value *d2x_dxi2 = coordinateDerivativeCache2->values;
	// evaluate derivative of dxi_dx w.r.t. xi by adding d2x_dxi2 permutations to dx_xi, inverting, and taking difference
	FE_value d2xi_dx_dxi[27];
	const FE_value deltaXi = 1.0E-5;
	const FE_value half__deltaXi = 0.5 / deltaXi;
	for (int d = 0; d < elementDimension; ++d)
	{
		FE_value delta_dxi_dx[9];
		// two-sided finite difference - gave same accuracy for gradientOfGradient2 test; one-sided was 400x worse
		for (int s = 0; s < 2; ++s)
		{
			FE_value offset_dx_dxi[9], offset_dxi_dx[9];
			const FE_value deltaXiSide = (s == 0) ? deltaXi : -deltaXi;
			for (int i = 0; i < coordinateComponentCount; ++i)
				for (int j = 0; j < elementDimension; ++j)
					offset_dx_dxi[i*3 + j] = dx_dxi[i*3 + j] + deltaXiSide*d2x_dxi2[i*coordinateTermCount2 + j*elementDimension + d];

			inverted = 0;
			if (coordinateComponentCount == 3)
				inverted = invert_FE_value_matrix3(offset_dx_dxi, offset_dxi_dx);
			else if (coordinateComponentCount == 2)
				inverted = invert_FE_value_matrix2(offset_dx_dxi, offset_dxi_dx);
			else if ((coordinateComponentCount == 1) && (fabs(*offset_dx_dxi) > 0.0))
			{
				*offset_dxi_dx = 1.0 / *offset_dx_dxi;
				inverted = 1;
			}
			if (!inverted)
			{
				display_message(WARNING_MESSAGE,
					"FieldGradient evaluateDerivative.  Could not invert offset coordinate derivatives; setting gradient derivative to 0");
				for (int k = 0; k < coordinateTermCount2; ++k)
					delta_dxi_dx[k] = 0.0;
				break;
			}
			if (s == 0)
				for (int k = 0; k < coordinateTermCount2; ++k)
					delta_dxi_dx[k] = offset_dxi_dx[k];
			else
				for (int k = 0; k < coordinateTermCount2; ++k)
					delta_dxi_dx[k] -= offset_dxi_dx[k];
		}
		// take difference of offset dxi_dx to centre dxi_dx, divide by deltaXi
		for (int i = 0; i < elementDimension; ++i)
		{
			const int rowi = i*coordinateComponentCount;
			for (int j = 0; j < coordinateComponentCount; ++j)
				d2xi_dx_dxi[i*coordinateTermCount2 + j*elementDimension + d] = delta_dxi_dx[rowi + j]*half__deltaXi;
		}
	}
	derivatives = derivativeCache->values;
	const FE_value *sourceDerivatives1 = sourceDerivativeCache1->values;
	// inner term count eliminates first mesh derivative
	const int innerTermCount = termCount / elementDimension;  // elementDimension == coordinateComponentCount
	for (int i = 0; i < sourceComponentCount; ++i)
	{
		for (int j = 0; j < coordinateComponentCount; ++j)
		{
			for (int d = 0; d < elementDimension; ++d)
			{
				for (int t = 0; t < innerTermCount; ++t)
				{
					FE_value sum = 0.0;
					// note: the mesh derivative changes slowest
					for (int k = 0; k < elementDimension; ++k)
						sum += sourceDerivatives1[k*innerTermCount + t]*d2xi_dx_dxi[k*coordinateTermCount2 + j*elementDimension + d];
					derivatives[t] += sum;
				}
				derivatives += innerTermCount;
			}
		}
		sourceDerivatives1 += termCount;
	}
	return 1;
}

int Computed_field_gradient::list()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_gradient);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    coordinate field : %s\n",field->source_fields[1]->name);
		display_message(INFORMATION_MESSAGE,
			"    source field : %s\n",field->source_fields[0]->name);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_gradient.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_gradient */

char *Computed_field_gradient::get_command_string()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name;
	int error;

	ENTER(Computed_field_gradient::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_gradient_type_string, &error);
		append_string(&command_string, " coordinate ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[1], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
		append_string(&command_string, " field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_gradient::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_gradient::get_command_string */

} //namespace

cmzn_field_id cmzn_fieldmodule_create_field_gradient(
	cmzn_fieldmodule_id field_module,
	cmzn_field_id source_field, cmzn_field_id coordinate_field)
{
	cmzn_field *field = 0;
	if (source_field && source_field->isNumerical() &&
		coordinate_field && coordinate_field->isNumerical() &&
		(3 >= coordinate_field->number_of_components))
	{
		int number_of_components = source_field->number_of_components *
			coordinate_field->number_of_components;
		cmzn_field *source_fields[2];
		source_fields[0] = source_field;
		source_fields[1] = coordinate_field;
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			number_of_components,
			/*number_of_source_fields*/2, source_fields,
			/*number_of_source_values*/0, NULL,
			new Computed_field_gradient());
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_gradient.  Invalid argument(s)");
	}

	return (field);
}

int Computed_field_get_type_gradient(struct Computed_field *field,
	struct Computed_field **source_field,struct Computed_field **coordinate_field)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
If the field is of type 'gradient', the <source_field> and <coordinate_field>
used by it are returned.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_get_type_gradient);
	if (field && (dynamic_cast<Computed_field_gradient*>(field->core)) &&
		source_field && coordinate_field)
	{
		/* source_fields: 0=source, 1=coordinate */
		*source_field = field->source_fields[0];
		*coordinate_field=field->source_fields[1];
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_gradient.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_gradient */

