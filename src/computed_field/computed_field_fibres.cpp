/*******************************************************************************
FILE : computed_field_fibres.c

LAST MODIFIED : 24 August 2006

DESCRIPTION :
Implements a number of basic continuum mechanics fibres operations on
computed fields.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <math.h>

#include "opencmiss/zinc/fieldfibres.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_coordinate.h"
#include "computed_field/computed_field_private.hpp"
#include "computed_field/computed_field_set.h"
#include "general/debug.h"
#include "general/mystring.h"
#include "general/message.h"

class Computed_field_fibres_package : public Computed_field_type_package
{
};

namespace {

const char computed_field_fibre_axes_type_string[] = "fibre_axes";

class Computed_field_fibre_axes : public Computed_field_core
{
public:
	Computed_field_fibre_axes() : Computed_field_core()
	{
	};

private:
	Computed_field_core *copy()
	{
		return new Computed_field_fibre_axes();
	}

	const char *get_type_string()
	{
		return(computed_field_fibre_axes_type_string);
	}

	virtual enum cmzn_field_type get_type()
	{
		return CMZN_FIELD_TYPE_FIBRE_AXES;
	}

	int compare(Computed_field_core* other_field)
	{
		if (dynamic_cast<Computed_field_fibre_axes*>(other_field))
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

	virtual bool is_defined_at_location(cmzn_fieldcache& cache);
};

bool Computed_field_fibre_axes::is_defined_at_location(cmzn_fieldcache& cache)
{
	const Field_location_element_xi* element_xi_location = cache.get_location_element_xi();
	if ((element_xi_location) && (2 <= element_xi_location->get_element_dimension()))
	{
		// check the source fields
		return Computed_field_core::is_defined_at_location(cache);
	}
	return false;
}

/**
 * Compute the three 3-component fibre axes in the order fibre, sheet, normal from
 * the source fibre and coordinate fields. Function reads the coordinate field and
 * derivatives in rectangular cartesian coordinates. The 1 to 3 fibre angles in
 * the fibre_field are used as follows (2 values omit step 3, 1 only does step 1):
 * 1 = fibre angle in xi1-xi2 plane measured from xi1;
 * 2 = imbrication angle, inclination of the fibres from xi1-xi2 plane after step 1.
 * 3 = sheet angle, rotation of the sheet about the fibre vector.
 * coordinates from the source_field values in an arbitrary
 * <element_dimension> may be 2 or 3 only.
 * Derivatives may not be computed for this type of field yet.
 */
int Computed_field_fibre_axes::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	const Field_location_element_xi* element_xi_location = cache.get_location_element_xi();
	if (element_xi_location)
	{
		RealFieldValueCache& valueCache = RealFieldValueCache::cast(inValueCache);
		cmzn_element* element = element_xi_location->get_element();
		const int element_dimension = element_xi_location->get_element_dimension();
		cmzn_element *top_level_element = element_xi_location->get_top_level_element();;
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

		cmzn_field_id fibre_field = getSourceField(0);
		cmzn_field_id coordinate_field = getSourceField(1);
		const RealFieldValueCache *fibreCache = RealFieldValueCache::cast(fibre_field->evaluate(*workingCache));
		const RealFieldValueCache *coordinateCache = RealFieldValueCache::cast(coordinate_field->evaluate(*workingCache));
		const FieldDerivative& fieldDerivative = *top_level_element->getMesh()->getFieldDerivative(/*order*/1);
		const DerivativeValueCache *coordinateDerivativeCache = coordinate_field->evaluateDerivative(*workingCache, fieldDerivative);
		FE_value x[3], dx_dxi[9];
		if (fibreCache && coordinateCache && coordinateDerivativeCache &&
			convert_coordinates_and_derivatives_to_rc(&(coordinate_field->coordinate_system),
				coordinate_field->number_of_components, coordinateCache->values, coordinateDerivativeCache->values,
				top_level_element_dimension, x, dx_dxi))
		{
			FE_value a_x, a_y, a_z, alpha, b_x, b_y, b_z, beta, c_x, c_y, c_z, cos_alpha,
				cos_beta, cos_gamma, f11, f12, f13, f21, f22, f23, f31, f32, f33,
				gamma, length, sin_alpha, sin_beta, sin_gamma;

			/* 2. Calculate the field */
			/* get f1~ = vector in xi1 direction */
			f11=dx_dxi[0];
			f12=dx_dxi[3];
			f13=dx_dxi[6];
			/* get f2~ = vector in xi2 direction */
			f21=dx_dxi[1];
			f22=dx_dxi[4];
			f23=dx_dxi[7];
			/* get f3~ = vector normal to xi1-xi2 plane */
			f31=f12*f23-f13*f22;
			f32=f13*f21-f11*f23;
			f33=f11*f22-f12*f21;
			/* normalise vectors f1~ and f3~ */
			if (0.0<(length=sqrt(f11*f11+f12*f12+f13*f13)))
			{
				f11 /= length;
				f12 /= length;
				f13 /= length;
			}
			if (0.0<(length=sqrt(f31*f31+f32*f32+f33*f33)))
			{
				f31 /= length;
				f32 /= length;
				f33 /= length;
			}
			/* get vector f2~ = f3~ (x) f1~ = normal to xi1 in xi1-xi2 plane */
			f21=f32*f13-f33*f12;
			f22=f33*f11-f31*f13;
			f23=f31*f12-f32*f11;
			/* get sin/cos of fibre angles alpha, beta and gamma */
			alpha = fibreCache->values[0];
			sin_alpha = sin(alpha);
			cos_alpha = cos(alpha);
			if (1 < fibre_field->number_of_components)
			{
				beta = fibreCache->values[1];
				sin_beta = sin(beta);
				cos_beta = cos(beta);
			}
			else
			{
				/* default beta is 0 */
				sin_beta = 0;
				cos_beta = 1;
			}
			if (2 < fibre_field->number_of_components)
			{
				gamma = fibreCache->values[2];
				sin_gamma = sin(gamma);
				cos_gamma = cos(gamma);
			}
			else
			{
				/* default gamma is 0 */
				sin_gamma = 0;
				cos_gamma = 1;
			}
			/* calculate the fibre axes a=fibre, b=sheet, c=normal */
			a_x =  cos_alpha*f11 + sin_alpha*f21;
			a_y =  cos_alpha*f12 + sin_alpha*f22;
			a_z =  cos_alpha*f13 + sin_alpha*f23;
			b_x = -sin_alpha*f11 + cos_alpha*f21;
			b_y = -sin_alpha*f12 + cos_alpha*f22;
			b_z = -sin_alpha*f13 + cos_alpha*f23;
			f11 = a_x;
			f12 = a_y;
			f13 = a_z;
			f21 = b_x;
			f22 = b_y;
			f23 = b_z;
			/* as per KATs change 30Nov00 in back-end function ROT_COORDSYS,
				rotate anticlockwise about axis2, not -axis2 */
			c_x =  cos_beta*f31 + sin_beta*f11;
			c_y =  cos_beta*f32 + sin_beta*f12;
			c_z =  cos_beta*f33 + sin_beta*f13;
			a_x = -sin_beta*f31 + cos_beta*f11;
			a_y = -sin_beta*f32 + cos_beta*f12;
			a_z = -sin_beta*f33 + cos_beta*f13;
			f31 = c_x;
			f32 = c_y;
			f33 = c_z;
			b_x =  cos_gamma*f21 + sin_gamma*f31;
			b_y =  cos_gamma*f22 + sin_gamma*f32;
			b_z =  cos_gamma*f23 + sin_gamma*f33;
			c_x = -sin_gamma*f21 + cos_gamma*f31;
			c_y = -sin_gamma*f22 + cos_gamma*f32;
			c_z = -sin_gamma*f23 + cos_gamma*f33;
			/* put fibre, sheet then normal in field values */
			valueCache.values[0]=a_x;
			valueCache.values[1]=a_y;
			valueCache.values[2]=a_z;
			valueCache.values[3]=b_x;
			valueCache.values[4]=b_y;
			valueCache.values[5]=b_z;
			valueCache.values[6]=c_x;
			valueCache.values[7]=c_y;
			valueCache.values[8]=c_z;
			return 1;
		}
	}
	return 0;
}

int Computed_field_fibre_axes::list()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_fibre_axes);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    coordinate field : %s\n",field->source_fields[1]->name);
		display_message(INFORMATION_MESSAGE,
			"    fibre field : %s\n",field->source_fields[0]->name);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_fibre_axes.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_fibre_axes */

char *Computed_field_fibre_axes::get_command_string(
	)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name;
	int error;

	ENTER(Computed_field_fibre_axes::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_fibre_axes_type_string, &error);
		append_string(&command_string, " coordinate ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[1], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
		append_string(&command_string, " fibre ", &error);
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
			"Computed_field_fibre_axes::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_fibre_axes::get_command_string */

} //namespace

cmzn_field_id cmzn_fieldmodule_create_field_fibre_axes(
	cmzn_fieldmodule_id field_module,
	cmzn_field_id fibre_field, cmzn_field_id coordinate_field)
{
	Computed_field *field = NULL;
	if (field_module && fibre_field && fibre_field->isNumerical() &&
		(3>=fibre_field->number_of_components) &&
		coordinate_field && coordinate_field->isNumerical() &&
		(3 >= coordinate_field->number_of_components))
	{
		Computed_field *source_fields[2];
		source_fields[0] = fibre_field;
		source_fields[1] = coordinate_field;
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			/*number_of_components*/9,
			/*number_of_source_fields*/2, source_fields,
			/*number_of_source_values*/0, NULL,
			new Computed_field_fibre_axes());
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_fibre_axes.  Invalid argument(s)");
	}
	LEAVE;

	return (field);
}

int Computed_field_get_type_fibre_axes(struct Computed_field *field,
	struct Computed_field **fibre_field,struct Computed_field **coordinate_field)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_FIBRE_AXES, the fibre and coordinate
fields used by it are returned - otherwise an error is reported.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_get_type_fibre_axes);
	if (field && (dynamic_cast<Computed_field_fibre_axes*>(field->core)) &&
		fibre_field && coordinate_field)
	{
		/* source_fields: 0=fibre, 1=coordinate */
		*fibre_field=field->source_fields[0];
		*coordinate_field=field->source_fields[1];
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_fibre_axes.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_fibre_axes */

