/*******************************************************************************
FILE : computed_field_set.h

LAST MODIFIED : 4 December 2003

DESCRIPTION :
Functions for selecting computed fields from the commands line.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (COMPUTED_FIELD_SET_H)
#define COMPUTED_FIELD_SET_H

#include "computed_field/computed_field.h"

/*
Global types
------------
*/

struct Set_Computed_field_conditional_data
/*******************************************************************************
LAST MODIFIED : 3 February 1999

DESCRIPTION :
User data structure passed to set_Computed_field_conditional, containing the
computed_field_package and the optional conditional_function (and
conditional_function_user_data) for selecting a field out of a subset of the
fields in the manager.
==============================================================================*/
{
	MANAGER_CONDITIONAL_FUNCTION(Computed_field) *conditional_function;
	void *conditional_function_user_data;
	struct MANAGER(Computed_field) *computed_field_manager;
}; /* struct Set_Computed_field_conditional_data */

struct Set_Computed_field_array_data
/*******************************************************************************
LAST MODIFIED : 9 March 1999

DESCRIPTION :
User data structure passed to set_Computed_field_array, containing the number of
fields in the array, and a pointer to a Set_Computed_field_conditional_data
structure for passing to set_Computed_field_conditional - this is done for each
element in the array.
==============================================================================*/
{
	int number_of_fields;
	struct Set_Computed_field_conditional_data *conditional_data;
}; /* struct Set_Computed_field_array_data */

/*
Global functions
----------------
*/

/***************************************************************************//**
 * Return the field from the manager with the given name. If none is found with
 * that name, try to find the field component as fieldname.componentname, i.e.
 * with the '.' separator. Since FieldML examples frequently use '.' in the
 * field name itself, search after the last '.' character.
 * @param component_number_address  On successful return this will have the
 * value -1 for the whole field, or the component number from 0 to count - 1.
 * @return  Non-accessed field on success, or NULL on failure.
 */
cmzn_field_id Computed_field_manager_get_field_or_component(
	struct MANAGER(Computed_field) *computed_field_manager, const char *name,
	int *component_number_address);

#endif /* !defined (COMPUTED_FIELD_SET_H) */
