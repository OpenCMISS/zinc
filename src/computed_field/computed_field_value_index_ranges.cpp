/*******************************************************************************
FILE : computed_field_value_index_ranges.c

LAST MODIFIED : 24 August 2006

DESCRIPTION :
Stores ranges of indices of field values in a multi-range for a Computed_field.
Used, eg., to indicate which components have been modified in an editor.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stdlib.h>
#include <stdio.h>
#include "computed_field/computed_field_value_index_ranges.h"
#include "general/compare.h"
#include "general/debug.h"
#include "general/indexed_list_private.h"
#include "general/message.h"

/*
Module types
------------
*/

struct Field_value_index_ranges
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Stores a multi_range indexed by field.
Used, eg., to indicate which components have been modified in an editor.
==============================================================================*/
{
	struct Computed_field *field;
	struct Multi_range *ranges;
	int access_count;
}; /* struct Field_value_index_ranges */

FULL_DECLARE_INDEXED_LIST_TYPE(Field_value_index_ranges);

/*
Module functions
----------------
*/

DECLARE_INDEXED_LIST_MODULE_FUNCTIONS(Field_value_index_ranges,field, \
	struct Computed_field *,compare_pointer)

/*
Global functions
----------------
*/

struct Field_value_index_ranges *CREATE(Field_value_index_ranges)(
	struct Computed_field *field)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Creates a Field_value_index_ranges object for storing ranges of value indices
stored in <field>.
==============================================================================*/
{
	struct Field_value_index_ranges *field_value_index_ranges;

	ENTER(CREATE(Field_value_index_ranges));
	if (field)
	{
		if (ALLOCATE(field_value_index_ranges,struct Field_value_index_ranges,1)&&
			(field_value_index_ranges->ranges=CREATE(Multi_range)()))
		{
			field_value_index_ranges->field=ACCESS(Computed_field)(field);
			field_value_index_ranges->access_count=0;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"CREATE(Field_value_index_ranges).  Not enough memory");
			DEALLOCATE(field_value_index_ranges);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CREATE(Field_value_index_ranges).  Invalid field");
		field_value_index_ranges=(struct Field_value_index_ranges *)NULL;
	}
	LEAVE;

	return (field_value_index_ranges);
} /* CREATE(Field_value_index_ranges) */

int DESTROY(Field_value_index_ranges)(
	struct Field_value_index_ranges **field_value_index_ranges_address)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Destroys the Field_value_index_ranges.
==============================================================================*/
{
	int return_code;
	struct Field_value_index_ranges *field_value_index_ranges;

	ENTER(DESTROY(Field_value_index_ranges));
	if (field_value_index_ranges_address&&
		(field_value_index_ranges= *field_value_index_ranges_address))
	{
		if (0==field_value_index_ranges->access_count)
		{
			DEACCESS(Computed_field)(&(field_value_index_ranges->field));
			DESTROY(Multi_range)(&(field_value_index_ranges->ranges));
			DEALLOCATE(*field_value_index_ranges_address);
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"DESTROY(Field_value_index_ranges).  Non-zero access count!");
			*field_value_index_ranges_address=(struct Field_value_index_ranges *)NULL;
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(Field_value_index_ranges).  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Field_value_index_ranges) */

DECLARE_OBJECT_FUNCTIONS(Field_value_index_ranges)
DECLARE_INDEXED_LIST_FUNCTIONS(Field_value_index_ranges)
DECLARE_FIND_BY_IDENTIFIER_IN_INDEXED_LIST_FUNCTION(Field_value_index_ranges, \
	field,struct Computed_field *,compare_pointer)

int Field_value_index_ranges_add_range(
	struct Field_value_index_ranges *field_value_index_ranges,int start,int stop)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Adds the range from <start> to <stop> to the ranges in
<field_value_index_ranges>.
==============================================================================*/
{
	int return_code;

	ENTER(Field_value_index_ranges_add_range);
	if (field_value_index_ranges)
	{
		/* check start/stop are non-negative */
		if ((0<=start)&&(0<=stop))
		{
			return_code=
				Multi_range_add_range(field_value_index_ranges->ranges,start,stop);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Field_value_index_ranges_add_range.  Invalid range");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Field_value_index_ranges_add_range.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Field_value_index_ranges_add_range */

int Field_value_index_ranges_is_index_in_range(
	struct Field_value_index_ranges *field_value_index_ranges,int index)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Returns true if the <index> is in the ranges of <field_value_index_ranges>.
==============================================================================*/
{
	int return_code;

	ENTER(Field_value_index_ranges_is_index_in_range);
	if (field_value_index_ranges)
	{
		return_code=
			Multi_range_is_value_in_range(field_value_index_ranges->ranges,index);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Field_value_index_ranges_is_index_in_range.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Field_value_index_ranges_is_index_in_range */

int Field_value_index_ranges_add_to_list(
	struct Field_value_index_ranges *field_value_index_ranges,
	void *field_value_index_ranges_list_void)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Ensures the <field_value_index_ranges> are in <field_value_index_ranges_list>.
==============================================================================*/
{
	int i,number_of_ranges,return_code,start,stop;
	struct Field_value_index_ranges *existing_field_value_index_ranges,
		*new_field_value_index_ranges;
	struct LIST(Field_value_index_ranges) *field_value_index_ranges_list;

	ENTER(Field_value_index_ranges_add_to_list);
	if (field_value_index_ranges&&
		(field_value_index_ranges_list=(struct LIST(Field_value_index_ranges) *)
			field_value_index_ranges_list_void)&&
		(0<(number_of_ranges=
			Multi_range_get_number_of_ranges(field_value_index_ranges->ranges))))
	{
		existing_field_value_index_ranges=
			FIND_BY_IDENTIFIER_IN_LIST(Field_value_index_ranges,field)(
				field_value_index_ranges->field,field_value_index_ranges_list);
		if (existing_field_value_index_ranges != 0)
		{
			return_code=1;
			for (i=0;(i<number_of_ranges)&&return_code;i++)
			{
				if (!(Multi_range_get_range(field_value_index_ranges->ranges,i,
					&start,&stop)&&
					Multi_range_add_range(existing_field_value_index_ranges->ranges,
						start,stop)))
				{
					display_message(ERROR_MESSAGE,
						"Field_value_index_ranges_add_to_list.  Could not add range");
					return_code=0;
				}
			}
		}
		else
		{
			if ((new_field_value_index_ranges=CREATE(Field_value_index_ranges)(
				field_value_index_ranges->field))&&
				Multi_range_copy(new_field_value_index_ranges->ranges,
					field_value_index_ranges->ranges)&&
				ADD_OBJECT_TO_LIST(Field_value_index_ranges)(
					new_field_value_index_ranges,field_value_index_ranges_list))
			{
				return_code=1;
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Field_value_index_ranges_add_to_list.  Could not add");
				if (new_field_value_index_ranges)
				{
					DESTROY(Field_value_index_ranges)(&new_field_value_index_ranges);
				}
				return_code=0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Field_value_index_ranges_add_to_list.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Field_value_index_ranges_add_to_list */

int Field_value_index_ranges_list_add_field_value_index(
	struct LIST(Field_value_index_ranges) *field_value_index_ranges_list,
	struct Computed_field *field,int index)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Shortcut for ensuring <field><index> is in the <field_value_index_ranges_list>.
==============================================================================*/
{
	int return_code;
	struct Field_value_index_ranges *existing_field_value_index_ranges,
		*new_field_value_index_ranges;

	ENTER(Field_value_index_ranges_list_add_field_value_index);
	if (field_value_index_ranges_list&&field&&(0<=index))
	{
		existing_field_value_index_ranges=
			FIND_BY_IDENTIFIER_IN_LIST(Field_value_index_ranges,field)(
				field,field_value_index_ranges_list);
		if (existing_field_value_index_ranges != 0)
		{
			return_code=Multi_range_add_range(
				existing_field_value_index_ranges->ranges,index,index);
		}
		else
		{
			if ((new_field_value_index_ranges=
				CREATE(Field_value_index_ranges)(field))&&
				Multi_range_add_range(new_field_value_index_ranges->ranges,
					index,index)&&
				ADD_OBJECT_TO_LIST(Field_value_index_ranges)(
					new_field_value_index_ranges,field_value_index_ranges_list))
			{
				return_code=1;
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Field_value_index_ranges_list_add_field_value_index.  "
					"Could not add field index");
				if (new_field_value_index_ranges)
				{
					DESTROY(Field_value_index_ranges)(&new_field_value_index_ranges);
				}
				return_code=0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Field_value_index_ranges_list_add_field_value_index.  "
			"Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Field_value_index_ranges_list_add_field_value_index */

int Field_value_index_ranges_remove_from_list(
	struct Field_value_index_ranges *field_value_index_ranges,
	void *field_value_index_ranges_list_void)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Ensures the <field_value_index_ranges> is not in
<field_value_index_ranges_list>.
==============================================================================*/
{
	int i,number_of_ranges,return_code,start,stop;
	struct Field_value_index_ranges *existing_field_value_index_ranges;
	struct LIST(Field_value_index_ranges) *field_value_index_ranges_list;

	ENTER(Field_value_index_ranges_remove_from_list);
	if (field_value_index_ranges&&
		(field_value_index_ranges_list=(struct LIST(Field_value_index_ranges) *)
			field_value_index_ranges_list_void)&&
		(0<(number_of_ranges=
			Multi_range_get_number_of_ranges(field_value_index_ranges->ranges))))
	{
		existing_field_value_index_ranges=
			FIND_BY_IDENTIFIER_IN_LIST(Field_value_index_ranges,field)(
				field_value_index_ranges->field,field_value_index_ranges_list);
		if (existing_field_value_index_ranges != 0)
		{
			return_code=1;
			for (i=0;(i<number_of_ranges)&&return_code;i++)
			{
				if (!(Multi_range_get_range(field_value_index_ranges->ranges,i,
					&start,&stop)&&
					Multi_range_remove_range(existing_field_value_index_ranges->ranges,
						start,stop)))
				{
					display_message(ERROR_MESSAGE,
						"Field_value_index_ranges_remove_from_list.  "
						"Could not remove range");
					return_code=0;
				}
			}
			/* remove existing_field_value_index_ranges if empty */
			if (0==Multi_range_get_number_of_ranges(
				existing_field_value_index_ranges->ranges))
			{
				REMOVE_OBJECT_FROM_LIST(Field_value_index_ranges)(
					existing_field_value_index_ranges,field_value_index_ranges_list);
				return_code=1;
			}
		}
		else
		{
			return_code=1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Field_value_index_ranges_remove_from_list.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Field_value_index_ranges_remove_from_list */

struct Computed_field *Field_value_index_ranges_get_field(
	struct Field_value_index_ranges *field_value_index_ranges)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Returns the field from the <field_value_index_ranges.
==============================================================================*/
{
	struct Computed_field *field;

	ENTER(Field_value_index_ranges_get_field);
	if (field_value_index_ranges)
	{
		field=field_value_index_ranges->field;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Field_value_index_ranges_get_field.  Invalid argument(s)");
		field=(struct Computed_field *)NULL;
	}
	LEAVE;

	return (field);
} /* Field_value_index_ranges_get_field */

struct Multi_range *Field_value_index_ranges_get_ranges(
	struct Field_value_index_ranges *field_value_index_ranges)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Returns the ranges from the <field_value_index_ranges.
==============================================================================*/
{
	struct Multi_range *ranges;

	ENTER(Field_value_index_ranges_get_ranges);
	if (field_value_index_ranges)
	{
		ranges=field_value_index_ranges->ranges;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Field_value_index_ranges_get_field.  Invalid argument(s)");
		ranges=(struct Multi_range *)NULL;
	}
	LEAVE;

	return (ranges);
} /* Field_value_index_ranges_get_field */
