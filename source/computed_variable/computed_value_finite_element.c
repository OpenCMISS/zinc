/*******************************************************************************
FILE : computed_value_finite_element.c

LAST MODIFIED : 30 July 2003

DESCRIPTION :
Implements computed values which interface to finite elements:
- element_xi
==============================================================================*/
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is cmgui.
 *
 * The Initial Developer of the Original Code is
 * Auckland Uniservices Ltd, Auckland, New Zealand.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
#include "computed_variable/computed_value_fe_value.h"
#include "computed_variable/computed_value_finite_element.h"
#include "computed_variable/computed_value_private.h"
#include "general/debug.h"
#include "user_interface/message.h"

/*
Module methods
--------------
*/
static char Cmiss_value_element_xi_type_string[]="Element_xi";
	/*???DB.  End up with same pointer as
		computed_variable_element_xi_type_string? */

struct Cmiss_value_element_xi_type_specific_data
/*******************************************************************************
LAST MODIFIED : 11 July 2003

DESCRIPTION :
???DB.  Have an xi number?  Probably better on variable?
==============================================================================*/
{
	FE_value *xi;
	int dimension;
	struct FE_element *element;
}; /* struct Cmiss_value_element_xi_type_specific_data */

static START_CMISS_VALUE_CLEAR_TYPE_SPECIFIC_FUNCTION(element_xi)
{
	DEALLOCATE(data->xi);
	if (data->element)
	{
		DEACCESS(FE_element)(&(data->element));
	}
	return_code=1;
}
END_CMISS_VALUE_CLEAR_TYPE_SPECIFIC_FUNCTION(element_xi)

static START_CMISS_VALUE_DUPLICATE_DATA_TYPE_SPECIFIC_FUNCTION(element_xi)
{
	FE_value *destination_xi,*source_xi;
	int number_of_values;

	if (source->element)
	{
		if (source_xi=source->xi)
		{
			number_of_values=source->dimension;
			if ((0<number_of_values)&&ALLOCATE(destination_xi,FE_value,
				number_of_values))
			{
				destination->dimension=number_of_values;
				destination->xi=destination_xi;
				while (number_of_values>0)
				{
					*destination_xi= *source_xi;
					destination_xi++;
					source_xi++;
					number_of_values--;
				}
				destination->element=ACCESS(FE_element)(source->element);
			}
			else
			{
				DEALLOCATE(destination);
			}
		}
		else
		{
			DEALLOCATE(destination);
		}
	}
	else
	{
		if (source->xi)
		{
			DEALLOCATE(destination);
		}
		else
		{
			destination->dimension=source->dimension;
			destination->xi=(FE_value *)NULL;
			destination->element=(struct FE_element *)NULL;
		}
	}
}
END_CMISS_VALUE_DUPLICATE_DATA_TYPE_SPECIFIC_FUNCTION(element_xi)

static START_CMISS_VALUE_GET_REALS_TYPE_SPECIFIC_FUNCTION(element_xi)
{
	FE_value *destination_real,*source_real;
	int number_of_reals;

	number_of_reals=data->dimension;
	if (!reals_address||!(source_real=data->xi)||
		ALLOCATE(destination_real,FE_value,number_of_reals))
	{
		*number_of_reals_address=number_of_reals;
		if (reals_address)
		{
			if (source_real)
			{
				*reals_address=destination_real;
				while (number_of_reals>0)
				{
					*destination_real= *source_real;
					source_real++;
					destination_real++;
					number_of_reals--;
				}
			}
			else
			{
				*reals_address=(FE_value *)NULL;
			}
		}
		return_code=1;
	}
}
END_CMISS_VALUE_GET_REALS_TYPE_SPECIFIC_FUNCTION(element_xi)

#define Cmiss_value_element_xi_get_string_type_specific \
	Cmiss_value_default_get_string

static START_CMISS_VALUE_INCREMENT_TYPE_SPECIFIC_FUNCTION(element_xi)
/*******************************************************************************
LAST MODIFIED : 30 July 2003

DESCRIPTION :
==============================================================================*/
{
	FE_value *fe_value_vector;
	int number_of_fe_values;

	/* check that <value> and <increment> "match" */
	if (Cmiss_value_FE_value_vector_get_type(increment,&number_of_fe_values,
		&fe_value_vector)&&(data_value->dimension=number_of_fe_values))
	{
		return_code=FE_element_xi_increment(&(data_value->element),data_value->xi,
			fe_value_vector);
	}
}
END_CMISS_VALUE_INCREMENT_TYPE_SPECIFIC_FUNCTION(element_xi)

static START_CMISS_VALUE_MULTIPLY_AND_ACCUMULATE_TYPE_SPECIFIC_FUNCTION(
	element_xi)
{
	FE_value *xi_total,*xi_1,*xi_2;
	int number_of_xi;
	struct Cmiss_value_element_xi_type_specific_data *data_total,*data_1,
		*data_2;

	data_1=(struct Cmiss_value_element_xi_type_specific_data *)
		Cmiss_value_get_type_specific_data(value_1);
	data_2=(struct Cmiss_value_element_xi_type_specific_data *)
		Cmiss_value_get_type_specific_data(value_2);
	data_total=(struct Cmiss_value_element_xi_type_specific_data *)
		Cmiss_value_get_type_specific_data(total);
	ASSERT_IF(data_1&&data_2&&data_total,return_code,0)
	{
		if ((data_1->element)&&(xi_1=data_1->xi)&&
			(0<(number_of_xi=data_1->dimension))&&(data_2->element)&&
			(xi_2=data_2->xi)&&(number_of_xi==data_2->dimension)&&
			(data_total->element)&&(xi_total=data_total->xi)&&
			(number_of_xi==data_total->dimension))
		{
			while (number_of_xi>0)
			{
				*xi_total += (*xi_1)*(*xi_2);
				xi_total++;
				xi_1++;
				xi_2++;
				number_of_xi--;
			}
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Cmiss_value_element_xi_multiply_and_accumulate_type_specific. "
				"Inconsistent element/xi");
		}
	}
}
END_CMISS_VALUE_MULTIPLY_AND_ACCUMULATE_TYPE_SPECIFIC_FUNCTION(element_xi)

static START_CMISS_VALUE_SAME_SUB_TYPE_TYPE_SPECIFIC_FUNCTION(element_xi)
{
	struct Cmiss_value_element_xi_type_specific_data *data_1,*data_2;

	data_1=(struct Cmiss_value_element_xi_type_specific_data *)
		Cmiss_value_get_type_specific_data(value_1);
	data_2=(struct Cmiss_value_element_xi_type_specific_data *)
		Cmiss_value_get_type_specific_data(value_2);
	ASSERT_IF(data_1&&data_2,return_code,0)
	{
		if (data_1->element)
		{
			if (data_2->element)
			{
				if (data_1->dimension==data_2->dimension)
				{
					return_code=1;
				}
			}
		}
		else
		{
			if (!(data_2->element))
			{
				if (data_1->dimension==data_2->dimension)
				{
					return_code=1;
				}
			}
		}
	}
}
END_CMISS_VALUE_SAME_SUB_TYPE_TYPE_SPECIFIC_FUNCTION(element_xi)

static START_CMISS_VALUE_SCALAR_MULTIPLY_TYPE_SPECIFIC_FUNCTION(element_xi)
/*******************************************************************************
LAST MODIFIED : 30 July 2003

DESCRIPTION :
==============================================================================*/
{
	USE_PARAMETER(scalar);
	return_code=0;
}
END_CMISS_VALUE_SCALAR_MULTIPLY_TYPE_SPECIFIC_FUNCTION(element_xi)

/*
Global functions
----------------
*/
int Cmiss_value_element_xi_set_type(Cmiss_value_id value,int dimension,
	struct FE_element *element,FE_value *xi)
/*******************************************************************************
LAST MODIFIED : 11 July 2003

DESCRIPTION :
Makes <value> of type element_xi and sets its <dimension>, <element> and <xi).
<dimension> must be positive or <element> must be non-NULL or {<dimension> must
be zero, <element> must be NULL and <xi must be NULL>}.  If <dimension> is
positive and <element> is non-NULL then <dimension> should equal the dimension
of the element.  After success, the <value> is responsible for DEALLOCATEing
<xi>.

???DB.  Assuming that the <element> knows its FE_region (can get manager)
==============================================================================*/
{
	int return_code;
	struct Cmiss_value_element_xi_type_specific_data *data;

	ENTER(Cmiss_value_element_xi_set_type);
	return_code=0;
	/* check arguments */
	if (value&&(((dimension==0)&&!element&&!xi)||
		((dimension>0)&&!element)||((dimension==0)&&element)||
		((dimension>0)&&element&&(dimension==get_FE_element_dimension(element)))))
	{
		/* 1.  Make dynamic allocations for any new type-specific data */
		if (ALLOCATE(data,struct Cmiss_value_element_xi_type_specific_data,1))
		{
			/* 2.  Clear current type-specific data */
			Cmiss_value_clear_type(value);
			/* 3.  Establish the new type */
			Cmiss_value_set_type_specific_information(value,
				Cmiss_value_element_xi_type_string,(void *)data);
			if (element)
			{
				data->element=ACCESS(FE_element)(element);
				data->dimension=get_FE_element_dimension(element);
			}
			else
			{
				data->element=(struct FE_element *)NULL;
				data->dimension=dimension;
			}
			data->xi=xi;
			/* set all the methods */
			return_code=CMISS_VALUE_ESTABLISH_METHODS(value,element_xi);
		}
		else
		{
			display_message(ERROR_MESSAGE,"Cmiss_value_element_xi_set_type.  "
				"Could not ALLOCATE type specific data");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Cmiss_value_element_xi_set_type.  "
			"Invalid argument(s).  %p %d %p",value,dimension,element);
	}
	LEAVE;

	return (return_code);
} /* Cmiss_value_element_xi_set_type */

DECLARE_CMISS_VALUE_IS_TYPE_FUNCTION(element_xi)

int Cmiss_value_element_xi_get_type(Cmiss_value_id value,int *dimension_address,
	struct FE_element **element_address,FE_value **xi_address)
/*******************************************************************************
LAST MODIFIED : 11 July 2003

DESCRIPTION :
If <value> is of type element_xi, gets its <*dimension_address>,
<*element_address> and <*xi_address).

The calling program must not DEALLOCATE the returned <*xi_address>.
==============================================================================*/
{
	int return_code;
	struct Cmiss_value_element_xi_type_specific_data *data;

	ENTER(Cmiss_value_element_xi_get_type);
	return_code=0;
	/* check arguments */
	if (value&&(dimension_address||element_address||xi_address))
	{
		data=(struct Cmiss_value_element_xi_type_specific_data *)
			Cmiss_value_get_type_specific_data(value);
		ASSERT_IF(data,return_code,0)
		{
			if (dimension_address)
			{
				*dimension_address=data->dimension;
			}
			if (element_address)
			{
				*element_address=data->element;
			}
			if (xi_address)
			{
				*xi_address=data->xi;
			}
			return_code=1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Cmiss_value_element_xi_get_type.  "
			"Invalid argument(s).  %p %p %p",value,element_address,xi_address);
	}
	LEAVE;

	return (return_code);
} /* Cmiss_value_element_xi_get_type */
