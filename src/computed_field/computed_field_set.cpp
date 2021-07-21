/*******************************************************************************
FILE : computed_field_set.c

LAST MODIFIED : 7 May 2007

DESCRIPTION :
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "opencmiss/zinc/fieldmodule.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_composite.h"
#include "computed_field/computed_field_set.h"
#include "general/debug.h"
#include "general/mystring.h"
#include "general/message.h"
#include <stdio.h>
#include "computed_field/computed_field_private.hpp"

cmzn_field_id Computed_field_manager_get_field_or_component(
	struct MANAGER(Computed_field) *computed_field_manager, const char *name,
	int *component_number_address)
{
	if ((!computed_field_manager) || (!name) || (!component_number_address))
		return 0;
	cmzn_field_id field =
		FIND_BY_IDENTIFIER_IN_MANAGER(Computed_field,name)(name, computed_field_manager);
	if (field)
	{
		*component_number_address = -1;
	}
	else if (0 != strrchr(name, '.'))
	{
		char *name_copy = duplicate_string(name);
		char *field_component_name = strrchr(name_copy,'.');
		*field_component_name = '\0';
		++field_component_name;
		field = FIND_BY_IDENTIFIER_IN_MANAGER(Computed_field,name)(name_copy, computed_field_manager);
		if (field)
		{
			int component_no = -1;
			const int number_of_components = cmzn_field_get_number_of_components(field);
			for (int i = 0; (i < number_of_components) && (component_no < 0); i++)
			{
				char *candidate_component_name = cmzn_field_get_component_name(field, i + 1);
				if (candidate_component_name)
				{
					if (0 == strcmp(field_component_name, candidate_component_name))
					{
						component_no = i;
					}
					DEALLOCATE(candidate_component_name);
				}
			}
			if (0 <= component_no)
			{
				if (1 == number_of_components)
				{
					/* already a single component field */
					component_no = -1;
				}
				*component_number_address = component_no;
			}
			else
			{
				field = 0;
			}
		}
		DEALLOCATE(name_copy);
	}
	return field;
}

