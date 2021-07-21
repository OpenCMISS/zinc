/*******************************************************************************
FILE : iso_field_calculation.c

LAST MODIFIED : 27 July 1998

DESCRIPTION :
Functions/types for creating iso-surfaces (other than using FE_fields).
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <math.h>
#include "finite_element/finite_element.h"
#include "general/debug.h"
#include "graphics/iso_field_calculation.h"
#include "general/message.h"

/*
Module types
------------
*/
struct Iso_field_calculation_data 
/*******************************************************************************
LAST MODIFIED : 24 September 1997

DESCRIPTION :
Global, but details are private.  Keeps the calculation type and any associated
parameters.
==============================================================================*/
{
	enum Iso_field_calculation_type type;
	union Iso_field_calculation_parameter_union
	{
		struct Iso_field_calculation_plane_struct 
		{
			int number_of_coeffs;
			FE_value *coeffs;
		} floats;
	} parameters;
}; /* struct Iso_field_calculation_data */

/*
Module functions
----------------
*/
static int deallocate_Iso_field_calculation_data(
	struct Iso_field_calculation_data *data )
/*******************************************************************************
LAST MODIFIED : 24 September 1997

DESCRIPTION :
Clears the memory used in storing a type without destroying the structure.
==============================================================================*/
{
	int return_code;

	ENTER(deallocate_Iso_field_calculation_data);
	if (data)
	{
		switch (data->type)
		{
			case COORDINATE_PLANE:
			case COORDINATE_SPHERE:
			case VERTICAL_POINT_TRACE:
			{
				DEALLOCATE(data->parameters.floats.coeffs);
			} break;
			default:
			{
			}
		}
		data->type=NULL_TYPE;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"deallocate_Iso_field_calculation_data.  Invalid data structure");
		return_code=0;
	}
	LEAVE;
	
	return (return_code);
} /* deallocate_Iso_field_calculation_data */

/*
Global functions
----------------
*/
struct Iso_field_calculation_data *CREATE(Iso_field_calculation_data)()
/*******************************************************************************
LAST MODIFIED : 24 September 1997

DESCRIPTION :
Allocates memory and assigns fields for a Iso_field_calculation_data object.
==============================================================================*/
{
	struct Iso_field_calculation_data *data;

	ENTER(CREATE(Iso_field_calculation_data));
	if (ALLOCATE(data,struct Iso_field_calculation_data,1))
	{
		data->type=NULL_TYPE;
	}
	else
	{
		display_message(ERROR_MESSAGE,"CREATE(Iso_field_calculation_data).  "
			"Unable to allocate memory for Iso_field_calculation_data");
		data=(struct Iso_field_calculation_data *)NULL;
	}
	LEAVE;

	return (data);
} /* CREATE(Iso_field_calculation_data) */

int DESTROY(Iso_field_calculation_data)(
	struct Iso_field_calculation_data **data_ptr)
/*******************************************************************************
LAST MODIFIED : 24 September 1997

DESCRIPTION :
Frees the memory for the fields of <**Iso_field_calculation_data>, frees the
memory for <**Iso_field_calculation_data> and sets <*Iso_field_calculation_data>
to NULL.
==============================================================================*/
{
	int return_code;

	ENTER(DESTROY(Iso_field_calculation_data));
	if (data_ptr)
	{
		deallocate_Iso_field_calculation_data(*data_ptr);
		DEALLOCATE(*data_ptr);
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(Iso_field_calculation_data).  Invalid argument");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Iso_field_calculation_data) */

enum Iso_field_calculation_type get_Iso_field_calculation_type( 
	struct Iso_field_calculation_data *data )
/*******************************************************************************
LAST MODIFIED : 24 September 1997

DESCRIPTION :
Gets the enumerated value that identifies the calculation type.
==============================================================================*/
{
	enum Iso_field_calculation_type return_code;

	ENTER(get_Iso_field_calculation_type);
	if (data)
	{
		return_code=data->type;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"get_Iso_field_calculation_type.  Invalid data structure");
		return_code=NULL_TYPE;
	}
	LEAVE;

	return (return_code);
} /* get_Iso_field_calculation_type */

int set_Iso_field_calculation(struct Iso_field_calculation_data *data,
	enum Iso_field_calculation_type type)
/*******************************************************************************
LAST MODIFIED : 24 September 1997

DESCRIPTION :
Sets the data structure so that it represents a calculation of the type
identified in the type enum.
==============================================================================*/
{
	int return_code;

	ENTER(set_Iso_field_calculation);
	if (data)
	{
		deallocate_Iso_field_calculation_data(data);
		switch (type)
		{
			case SCALAR_FIELD:
			{
				data->type=type;
				return_code=1;
			} break;
			case COORDINATE_PLANE:
			case COORDINATE_SPHERE:
			case VERTICAL_POINT_TRACE:
			{
				display_message(ERROR_MESSAGE,"set_Iso_field_calculation.  ",
					"Parameters required for this calculation type");
				return_code=0;
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"set_Iso_field_calculation.  Invalid iso_field_calculation_type");
				return_code=0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"set_Iso_field_calculation.  Invalid data structure");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* set_Iso_field_calculation */

int set_Iso_field_calculation_with_floats(
	struct Iso_field_calculation_data *data,enum Iso_field_calculation_type type,
	int number_of_components,ZnReal *coeffs)
/*******************************************************************************
LAST MODIFIED : 24 September 1997

DESCRIPTION :
Sets the data structure so that it represents a calculation of the type
identified in the type enum and also sets 'number_of_components' parameters.
==============================================================================*/
{
	int i,return_code = 1;

	ENTER(set_Iso_field_calculation_with_floats);
	if (data)
	{
		deallocate_Iso_field_calculation_data(data);
		switch (type)
		{
			case SCALAR_FIELD:
			{
				display_message(ERROR_MESSAGE,
					"set_Iso_field_calculation_with_floats.  "
					"No parameters should be given for this calculation type");
				return_code=0;
			} break;
			case COORDINATE_PLANE:
			case COORDINATE_SPHERE:
			case VERTICAL_POINT_TRACE:
			{
				if (ALLOCATE(data->parameters.floats.coeffs,FE_value,
					number_of_components))
				{
					data->type=type;
					data->parameters.floats.number_of_coeffs=number_of_components;
					for (i=0;i<number_of_components;i++ )
					{
						data->parameters.floats.coeffs[i]=coeffs[i];
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"set_Iso_field_calculation_with_floats.  "
						"Unable to allocate memory for parameters");
					return_code=0;
				}
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"set_Iso_field_calculation.  Invalid iso_field_calculation_type");
				return_code=0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"set_Iso_field_calculation_with_floats.  Invalid data structure");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* set_Iso_field_calculation_with_floats */

FE_value evaluate_Iso_field_calculation(struct Iso_field_calculation_data *data,
	int number_of_components,int iso_field_component_number,FE_value *values)
/*******************************************************************************
LAST MODIFIED : 28 May 1998

DESCRIPTION :
Calculates a single value that represents the field at the values given in
values.  If the iso_field_component_number is -1 then all the components are
assumed to be available.  Otherwise just the single component identified by the
iso_field_component number is expected.
???DB.  iso_field_component_number needs checking
==============================================================================*/
{
	FE_value angle,dot_prod,min_angle,onex,oney,return_value,sign,twox,twoy;
	int i,number_of_points;

	ENTER(evaluate_Iso_field_calculation);
	if (data)
	{
		switch (data->type)
		{
			case SCALAR_FIELD:
			{
				if (-1==iso_field_component_number)
				{
					display_message(ERROR_MESSAGE,"set_Iso_field_calculation_type.  "
			"Only one component should be specified for a scalar field calculation");
					return_value=0;
				}
				else
				{
					return_value=values[0];
				}
			} break;
			case COORDINATE_PLANE:
			{
				if (-1!=iso_field_component_number)
				{
					display_message(ERROR_MESSAGE,"set_Iso_field_calculation_type.  "
						"Multiple component field required for plane calculation");
					return_value=0;
				}
				else
				{
					if (number_of_components!=data->parameters.floats.number_of_coeffs)
					{
						display_message(ERROR_MESSAGE,
							"set_Iso_field_calculation_type.  "
							"Number of components supplied doesn't match calculation data");
						return_value=0;
					}
					else
					{
						return_value=0;
						for (i=0;i<number_of_components;i++)
						{
							return_value += values[i]*(data->parameters.floats.coeffs)[i];
						}
					}
				}
			} break;
			case COORDINATE_SPHERE:
			{
				if (-1!=iso_field_component_number )
				{
					display_message(ERROR_MESSAGE,
						"set_Iso_field_calculation_type.  "
						"Multiple component field required for sphere calculation");
					return_value=0;
				}
				else
				{
					if (number_of_components!=data->parameters.floats.number_of_coeffs)
					{
						display_message(ERROR_MESSAGE,
							"set_Iso_field_calculation_type.  "
							"Number of components supplied doesn't match calculation data");
						return_value=0;
					}
					else
					{
						return_value=0;
						for (i=0;i<number_of_components;i++)
						{
							return_value += (values[i]-(data->parameters.floats.coeffs)[i])*
								(values[i]-(data->parameters.floats.coeffs)[i]);
						}
						return_value=sqrt(return_value);
					}
				}
			} break;
			case VERTICAL_POINT_TRACE:
			{
				if ((-1!=iso_field_component_number)||(3!=number_of_components))
				{
					display_message(ERROR_MESSAGE,"set_Iso_field_calculation_type.  "
						"Three component field required for vertical trace calculation");
					return_value=0;
				}
				else
				{
					return_value=0;
					min_angle=0;
					number_of_points=(data->parameters.floats.number_of_coeffs)/2;
					for (i=0;i<number_of_points-1;i++)
					{
						onex=data->parameters.floats.coeffs[2*i];
						twox=onex-values[0];
						onex -= data->parameters.floats.coeffs[2*i+2];
						oney=data->parameters.floats.coeffs[2*i+1];
						twoy=oney-values[1];
						oney -= data->parameters.floats.coeffs[2*i+3];
						dot_prod=onex*twox+oney*twoy;
						dot_prod /= onex*onex+oney*oney;
						if (dot_prod<0)
						{
							/* calculate angle between and add to function */
							angle=fabs(onex*twox+oney*twoy)/sqrt(onex*onex+oney*oney);
						}
						else
						{
							if (dot_prod>1 )
							{
								twox -= onex;
								twoy -= oney;
								angle=fabs(onex*twox+oney*twoy)/sqrt(onex*onex+oney*oney);
							}
							else
							{
								angle=0;
								twox -= dot_prod*onex;
								twoy -= dot_prod*oney;
							}
						}
						dot_prod=sqrt(twox*twox+twoy*twoy);
						if (onex*twoy-twox*oney>=0)
						{
							sign=1;
						}
						else
						{
							sign= -1;
						}
						if ((0==i)||(dot_prod+angle<fabs(return_value)+min_angle))
						{
							return_value=sign*dot_prod;
							min_angle=angle;
						}
					}
				}
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"evaluate_Iso_field_calculation.  Invalid calculation type");
				return_value=0;
			} break;
		}		
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"evaluate_Iso_field_calculation.  Invalid data structure");
		return_value=0;
	}
	LEAVE;

	return (return_value);
} /* evaluate_Iso_field_calculation */

FE_value evaluate_Iso_field_clip(struct Iso_field_calculation_data *data,
	int number_of_components,int iso_field_component_number,FE_value *values )
/*******************************************************************************
LAST MODIFIED : 24 September 1997

DESCRIPTION :
Calculates a single value that represents the clipping field at the values given
in values.  If the iso_field_component_number is -1 then all the components are
assumed to be available.  Otherwise just the single component identified by the
iso_field_component number is expected.
==============================================================================*/
{
	FE_value angle,clip,dot_prod,min_angle,onex,oney,return_clip = 0.0,return_value,
		sign,twox,twoy;
	int i,number_of_points;

	ENTER(evaluate_Iso_field_calculation);
	if (data)
	{
		return_clip=1;
		switch (data->type)
		{
			case SCALAR_FIELD:
			{
				if (-1==number_of_components)
				{
					display_message(ERROR_MESSAGE,"set_Iso_field_calculation_type.  "
			"Only one component should be specified for a scalar field calculation");
					return_clip=0;
				}
				else
				{
					return_clip=1;
				}
			} break;
			case COORDINATE_PLANE:
			{
				if (-1!=iso_field_component_number)
				{
					display_message(ERROR_MESSAGE,"set_Iso_field_calculation_type.  "
						"Multiple component field required for plane calculation");
					return_clip=0;
				}
				else
				{
					if (number_of_components!=data->parameters.floats.number_of_coeffs)
					{
						display_message(ERROR_MESSAGE,"set_Iso_field_calculation_type.  "
							"Number of components supplied doesn't match calculation data");
						return_clip=0;
					}
					else
					{
						return_clip=1;
					}
				}
			} break;
			case COORDINATE_SPHERE:
			{
				if (-1!=iso_field_component_number)
				{
					display_message(ERROR_MESSAGE,"set_Iso_field_calculation_type.  "
						"Multiple component field required for sphere calculation");
					return_clip=0;
				}
				else
				{
					if (number_of_components!=data->parameters.floats.number_of_coeffs)
					{
						display_message(ERROR_MESSAGE,"set_Iso_field_calculation_type.  "
							"Number of components supplied doesn't match calculation data");
						return_clip=0;
					}
					else
					{
						return_clip=1;
					}
				}
			} break;
			case VERTICAL_POINT_TRACE:
			{
				if ((-1!=iso_field_component_number)||(3!=number_of_components))
				{
					display_message(ERROR_MESSAGE,"set_Iso_field_calculation_type.  "
						"Three component field required for vertical trace calculation");
					return_clip=0;
				}
				else
				{
					return_value=0;
					min_angle=0;
					return_clip=0;
					number_of_points=(data->parameters.floats.number_of_coeffs)/2;
					for (i=0;i<number_of_points-1;i++)
					{
						clip= 0;
						onex=data->parameters.floats.coeffs[2*i];
						twox=onex - values[0];
						onex -= data->parameters.floats.coeffs[2*i+2];
						oney=data->parameters.floats.coeffs[2*i+1];
						twoy=oney - values[1];
						oney -= data->parameters.floats.coeffs[2*i+3];
						dot_prod=onex*twox+oney*twoy;
						dot_prod /= onex*onex+oney*oney;
						if (dot_prod<0)
						{
							/* calculate angle between and add to function */
							angle=fabs(onex*twox+oney*twoy)/sqrt(onex*onex+oney*oney);
						}
						else
						{
							if ( dot_prod > 1 )
							{
								twox -= onex;
								twoy -= oney;
								angle=fabs(onex*twox+oney*twoy)/sqrt(onex*onex+oney*oney);
							}
							else
							{
								angle=0;
								twox -= dot_prod*onex;
								twoy -= dot_prod*oney;
							}
						}
						if (dot_prod<0.5)
						{
							if (0==i)
							{
								clip=dot_prod;
							}
							else
							{
								clip=1-dot_prod;
							}
						}
						else
						{
							if (i==number_of_points-2)
							{
								clip=1-dot_prod;
							}
							else
							{
								clip=dot_prod;
							}
						}
						dot_prod=sqrt(twox*twox+twoy*twoy);
						if (onex*twoy-twox*oney>=0)
						{
							sign=1;
						}
						else
						{
							sign= -1;
						}
						if ((0==i)||(dot_prod+angle<fabs(return_value)+min_angle))
						{
							return_value=sign*dot_prod;
							min_angle=angle;
							return_clip=clip;
						}
					}
				}
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"evaluate_Iso_field_clip.  Invalid calculation type");
				return_value = 0;
			} break;
		}		
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"evaluate_Iso_field_clip.  Invalid data structure");
		return_value = 0;
	}
	LEAVE;

	return (return_clip);
} /* evaluate_Iso_field_clip */
