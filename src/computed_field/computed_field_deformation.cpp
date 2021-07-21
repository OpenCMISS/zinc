/*******************************************************************************
FILE : computed_field_deformation.c

LAST MODIFIED : 24 August 2006

DESCRIPTION :
Implements a number of basic continuum mechanics deformation operations on
computed fields.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <math.h>
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_deformation.h"
#include "computed_field/computed_field_private.hpp"
#include "computed_field/computed_field_set.h"
#include "general/debug.h"
#include "general/mystring.h"
#include "general/message.h"

class Computed_field_deformation_package : public Computed_field_type_package
{
};

namespace {

char computed_field_2d_strain_type_string[] = "2d_strain";

class Computed_field_2d_strain : public Computed_field_core
{
public:
	Computed_field_2d_strain() : Computed_field_core()
	{
	};

private:
	virtual bool is_defined_at_location(cmzn_fieldcache& cache);

	Computed_field_core *copy()
	{
		return new Computed_field_2d_strain();
	}

	const char *get_type_string()
	{
		return(computed_field_2d_strain_type_string);
	}

	int compare(Computed_field_core* other_field)
	{
		if (dynamic_cast<Computed_field_2d_strain*>(other_field))
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

bool Computed_field_2d_strain::is_defined_at_location(cmzn_fieldcache& cache)
{
	const Field_location_element_xi* element_xi_location = cache.get_location_element_xi();
	if ((element_xi_location) && (2 <= element_xi_location->get_element_dimension()))
	{
		// check the source fields
		return Computed_field_core::is_defined_at_location(cache);
	}
	return false;
}

int Computed_field_2d_strain::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	const Field_location_element_xi* element_xi_location = cache.get_location_element_xi();
	if (element_xi_location)
	{
		cmzn_element* element = element_xi_location->get_element();
		const int element_dimension = element_xi_location->get_element_dimension();
		const FieldDerivative& fieldDerivative = *element->getMesh()->getFieldDerivative(/*order*/1);
		const DerivativeValueCache *deformedDerivativeCache = getSourceField(0)->evaluateDerivative(cache, fieldDerivative);
		const DerivativeValueCache *undeformedDerivativeCache = getSourceField(1)->evaluateDerivative(cache, fieldDerivative);
		const RealFieldValueCache *fibreCache = RealFieldValueCache::cast(getSourceField(2)->evaluate(cache));
		if (deformedDerivativeCache && undeformedDerivativeCache && fibreCache)
		{
			double A,A2,B,B2,cos_fibre_angle,C2,D,dxi_dnu[6],E[4],fibre_angle,
				F_x[6],F_X[6],sin_fibre_angle;
			FE_value def_derivative_xi[9], undef_derivative_xi[9];

			RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
			const FE_value *deformedDerivatives = deformedDerivativeCache->values;
			const FE_value *undeformedDerivatives = undeformedDerivativeCache->values;
			switch (element_dimension)
			{
				case 2:
				{
					def_derivative_xi[0] = deformedDerivatives[0];
					def_derivative_xi[1] = deformedDerivatives[1];
					def_derivative_xi[3] = deformedDerivatives[2];
					def_derivative_xi[4] = deformedDerivatives[3];
					def_derivative_xi[6] = deformedDerivatives[4];
					def_derivative_xi[7] = deformedDerivatives[5];
					undef_derivative_xi[0] = undeformedDerivatives[0];
					undef_derivative_xi[1] = undeformedDerivatives[1];
					undef_derivative_xi[3] = undeformedDerivatives[2];
					undef_derivative_xi[4] = undeformedDerivatives[3];
					undef_derivative_xi[6] = undeformedDerivatives[4];
					undef_derivative_xi[7] = undeformedDerivatives[5];
				} break;
				case 3:
				{
					/* Convert to 2D ignoring xi3, should be able to choose the
						direction that is ignored */
					def_derivative_xi[0] = deformedDerivatives[0];
					def_derivative_xi[1] = deformedDerivatives[1];
					def_derivative_xi[3] = deformedDerivatives[3];
					def_derivative_xi[4] = deformedDerivatives[4];
					def_derivative_xi[6] = deformedDerivatives[6];
					def_derivative_xi[7] = deformedDerivatives[7];
					undef_derivative_xi[0] = undeformedDerivatives[0];
					undef_derivative_xi[1] = undeformedDerivatives[1];
					undef_derivative_xi[3] = undeformedDerivatives[3];
					undef_derivative_xi[4] = undeformedDerivatives[4];
					undef_derivative_xi[6] = undeformedDerivatives[6];
					undef_derivative_xi[7] = undeformedDerivatives[7];
				} break;
				default:
				{
					display_message(ERROR_MESSAGE,
						"Computed_field_evaluate_2d_strain.  Unknown element dimension");
					return 0;
				} break;
			}
			/* do 2D_strain calculation */
			fibre_angle = fibreCache->values[0];

			/* X is the undeformed coordinates
				x is the deformed coordinates
				nu is the fibre coordinate system (2-D,within sheet) */
			/* calculate F_X=dX_dnu and dxi_dnu */
			cos_fibre_angle=cos(fibre_angle);
			sin_fibre_angle=sin(fibre_angle);
			A2=undef_derivative_xi[0]*undef_derivative_xi[0]+
				undef_derivative_xi[3]*undef_derivative_xi[3]+
				undef_derivative_xi[6]*undef_derivative_xi[6];
			A=sqrt(A2);
			B2=undef_derivative_xi[1]*undef_derivative_xi[1]+
				undef_derivative_xi[4]*undef_derivative_xi[4]+
				undef_derivative_xi[7]*undef_derivative_xi[7];
			B=sqrt(B2);
			C2=undef_derivative_xi[0]*undef_derivative_xi[1]+
				undef_derivative_xi[3]*undef_derivative_xi[4]+
				undef_derivative_xi[6]*undef_derivative_xi[7];
			D=sin_fibre_angle*B/(A2*B2-C2*C2);
			dxi_dnu[0] = cos_fibre_angle/A-sin_fibre_angle*C2*D;
			dxi_dnu[1]=D*A2;
			F_X[0]=dxi_dnu[0]*undef_derivative_xi[0]+
				dxi_dnu[1]*undef_derivative_xi[1];
			F_X[2]=dxi_dnu[0]*undef_derivative_xi[3]+
				dxi_dnu[1]*undef_derivative_xi[4];
			F_X[4]=dxi_dnu[0]*undef_derivative_xi[6]+
				dxi_dnu[1]*undef_derivative_xi[7];
			D=cos_fibre_angle*B/(A2*B2-C2*C2);
			dxi_dnu[2]=
				-(sin_fibre_angle/A+cos_fibre_angle*C2*D);
			dxi_dnu[3]=D*A2;
			F_X[1]=dxi_dnu[2]*undef_derivative_xi[0]+
				dxi_dnu[3]*undef_derivative_xi[1];
			F_X[3]=dxi_dnu[2]*undef_derivative_xi[3]+
				dxi_dnu[3]*undef_derivative_xi[4];
			F_X[5]=dxi_dnu[2]*undef_derivative_xi[6]+
				dxi_dnu[3]*undef_derivative_xi[7];
			/* calculate F_x=dx_dnu=dx_dxi*dxi_dnu */
			F_x[0]=dxi_dnu[0]*def_derivative_xi[0]+
				dxi_dnu[1]*def_derivative_xi[1];
			F_x[1]=dxi_dnu[2]*def_derivative_xi[0]+
				dxi_dnu[3]*def_derivative_xi[1];
			F_x[2]=dxi_dnu[0]*def_derivative_xi[3]+
				dxi_dnu[1]*def_derivative_xi[4];
			F_x[3]=dxi_dnu[2]*def_derivative_xi[3]+
				dxi_dnu[3]*def_derivative_xi[4];
			F_x[4]=dxi_dnu[0]*def_derivative_xi[6]+
				dxi_dnu[1]*def_derivative_xi[7];
			F_x[5]=dxi_dnu[2]*def_derivative_xi[6]+
				dxi_dnu[3]*def_derivative_xi[7];
			/* calculate the strain tensor
				E=0.5*(trans(F_x)*F_x-trans(F_X)*F_X) */
			E[0]=0.5*((F_x[0]*F_x[0]+F_x[2]*F_x[2]+
					F_x[4]*F_x[4])-
				(F_X[0]*F_X[0]+F_X[2]*F_X[2]+
					F_X[4]*F_X[4]));
			E[1]=0.5*((F_x[0]*F_x[1]+F_x[2]*F_x[3]+
					F_x[4]*F_x[5])-
				(F_X[0]*F_X[1]+F_X[2]*F_X[3]+
					F_X[4]*F_X[5]));
			E[2]=E[1];
			E[3]=0.5*((F_x[1]*F_x[1]+F_x[3]*F_x[3]+
					F_x[5]*F_x[5])-
				(F_X[1]*F_X[1]+F_X[3]*F_X[3]+
					F_X[5]*F_X[5]));
			valueCache.values[0] = E[0];
			valueCache.values[1] = E[1];
			valueCache.values[2] = E[2];
			valueCache.values[3] = E[3];
			return 1;
		}
	}
	return 0;
}

int Computed_field_2d_strain::list()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_2d_strain);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    deformed coordinate field : %s\n",field->source_fields[0]->name);
		display_message(INFORMATION_MESSAGE,
			"    undeformed coordinate field : %s\n",field->source_fields[1]->name);
		display_message(INFORMATION_MESSAGE,
			"    fibre angle field : %s\n",field->source_fields[2]->name);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_2d_strain.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_2d_strain */

char *Computed_field_2d_strain::get_command_string()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name;
	int error;

	ENTER(Computed_field_2d_strain::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_2d_strain_type_string, &error);
		append_string(&command_string, " deformed_coordinate ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
		append_string(&command_string, " undeformed_coordinate ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[1], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
		append_string(&command_string, " fibre_angle ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[2], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_2d_strain::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_2d_strain::get_command_string */

} //namespace

/***************************************************************************//**
 * Creates a field which preforms a 2-D strain calculation, combining a
 * deformed_coordinate_field, undeformed_coordinate_field and
 * fibre_angle_field.  Sets the number of components to 4.
 * The <coordinate_field>s must have no more than 3 components.
 */
cmzn_field *cmzn_fieldmodule_create_field_2d_strain(
	cmzn_fieldmodule *fieldmodule,
	cmzn_field *deformed_coordinate_field,
	cmzn_field *undeformed_coordinate_field,
	cmzn_field *fibre_angle_field)
{
	cmzn_field *field = NULL;
	if (fieldmodule && deformed_coordinate_field &&
		(3 >= deformed_coordinate_field->number_of_components) &&
		undeformed_coordinate_field &&
		(3 >= undeformed_coordinate_field->number_of_components) &&
		fibre_angle_field)
	{
		Computed_field *source_fields[3];
		source_fields[0] = deformed_coordinate_field;
		source_fields[1] = undeformed_coordinate_field;
		source_fields[2] = fibre_angle_field;
		field = Computed_field_create_generic(fieldmodule,
			/*check_source_field_regions*/true,
			/*number_of_components*/4,
			/*number_of_source_fields*/3, source_fields,
			/*number_of_source_values*/0, NULL,
			new Computed_field_2d_strain());
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_2d_strain.  Invalid argument(s)");
	}

	return (field);
}

int Computed_field_get_type_2d_strain(struct Computed_field *field,
	struct Computed_field **deformed_coordinate_field,
	struct Computed_field **undeformed_coordinate_field,
	struct Computed_field **fibre_angle_field)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_2D_STRAIN, the undeformed and deformed
coordinate fields and the fibre angle field used by it are returned 
- otherwise an error is reported.
Use function Computed_field_get_type to determine the field type.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_get_type_2d_strain);
	if (field && (dynamic_cast<Computed_field_2d_strain*>(field->core)) &&
		undeformed_coordinate_field && deformed_coordinate_field &&
		fibre_angle_field)
	{
		/* source_fields: 0=deformed_coordinate_field,
			1=undeformed_coordinate_field,
			2=fibre_angle_field */
		*deformed_coordinate_field=field->source_fields[0];
		*undeformed_coordinate_field=field->source_fields[1];
		*fibre_angle_field=field->source_fields[2];
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_2d_strain.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_2d_strain */

