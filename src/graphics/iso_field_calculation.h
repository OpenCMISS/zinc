/*******************************************************************************
FILE : iso_field_calculation.h

LAST MODIFIED : 24 September 1997

DESCRIPTION :
Functions/types for creating iso-surfaces (other than using FE_fields).
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (ISO_FIELD_CALCULATION_H)
#define ISO_FIELD_CALCULATION_H

#include "general/object.h"

/*
Global types
------------
*/
enum Iso_field_calculation_type
/*******************************************************************************
LAST MODIFIED : 24 September 1997

DESCRIPTION :
Types of iso-surface.
==============================================================================*/
{
	NULL_TYPE,
	SCALAR_FIELD,
	COORDINATE_PLANE,
	COORDINATE_SPHERE,
	VERTICAL_POINT_TRACE
}; /* enum Iso_field_calculation_type */

struct Iso_field_calculation_data;
	/* details private */

/*
Global functions
----------------
*/
struct Iso_field_calculation_data *CREATE(Iso_field_calculation_data)();
/*******************************************************************************
LAST MODIFIED : 24 September 1997

DESCRIPTION :
Allocates memory and assigns fields for a Iso_field_calculation_data object.
==============================================================================*/

int DESTROY(Iso_field_calculation_data)(
	struct Iso_field_calculation_data **data_ptr);
/*******************************************************************************
LAST MODIFIED : 24 September 1997

DESCRIPTION :
Frees the memory for the fields of <**Iso_field_calculation_data>, frees the
memory for <**Iso_field_calculation_data> and sets <*Iso_field_calculation_data>
to NULL.
==============================================================================*/

enum Iso_field_calculation_type get_Iso_field_calculation_type( 
	struct Iso_field_calculation_data *data);
/*******************************************************************************
LAST MODIFIED : 24 September 1997

DESCRIPTION :
Gets the enumerated value that identifies the calculation type
==============================================================================*/

FE_value evaluate_Iso_field_clip(struct Iso_field_calculation_data *data,
	int number_of_components,int iso_field_component_number,FE_value *values);
/*******************************************************************************
LAST MODIFIED : 24 September 1997

DESCRIPTION :
Calculates a single value that represents the clipping field at the values given
in values.  If the iso_field_component_number is -1 then all the components are
assumed to be available.  Otherwise just the single component identified by the
iso_field_component number is expected.
==============================================================================*/

int set_Iso_field_calculation(struct Iso_field_calculation_data *data,
	enum Iso_field_calculation_type type);
/*******************************************************************************
LAST MODIFIED : 24 September 1997

DESCRIPTION :
Sets the data structure so that it represents a calculation of the type
identified in the type enum.
==============================================================================*/

int set_Iso_field_calculation_with_floats(
	struct Iso_field_calculation_data *data,enum Iso_field_calculation_type type,
	int number_of_components,ZnReal *coeffs);
/*******************************************************************************
LAST MODIFIED : 24 September 1997

DESCRIPTION :
Sets the data structure so that it represents a calculation of the type
identified in the type enum and also sets 'number_of_components' parameters.
==============================================================================*/

FE_value evaluate_Iso_field_calculation(struct Iso_field_calculation_data *data,
	int number_of_components,int iso_field_component_number,FE_value *values);
/*******************************************************************************
LAST MODIFIED : 24 September 1997

DESCRIPTION :
Calculates a single value that represents the field at the values given in
values.  If the iso_field_component_number is -1 then all the components are
assumed to be available.  Otherwise just the single component identified by the
iso_field_component number is expected.
==============================================================================*/
#endif /* !defined (ISO_FIELD_CALCULATION_H) */
