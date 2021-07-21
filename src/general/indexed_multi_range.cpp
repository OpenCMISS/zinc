/*******************************************************************************
FILE : indexed_multi_range.c

LAST MODIFIED : 1 November 2000

DESCRIPTION :
Defines an object with a multirange at each value of an index_number.  An indexed list
can then be created for a group of these objects allowing rapid lookup.
This code was originally mirage/tracking_editor_data.c and refered explicitly
to index_multi_range for pending/placed frames etc.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "general/compare.h"
#include "general/debug.h"
#include "general/indexed_list_private.h"
#include "general/indexed_multi_range.h"
#include "general/multi_range.h"
#include "general/object.h"
#include "general/message.h"

/*
Module variables
----------------
*/

/*
Global variables
----------------
*/

/*
Global types
------------
*/

struct Index_multi_range
/*******************************************************************************
LAST MODIFIED : 1 November 2000

DESCRIPTION :
The status of a node that is being tracked.
==============================================================================*/
{
	int index_number;
	struct Multi_range *ranges;
	int access_count;
}; /* Index_multi_range */

FULL_DECLARE_INDEXED_LIST_TYPE(Index_multi_range);

/*
Module functions
----------------
*/
DECLARE_INDEXED_LIST_MODULE_FUNCTIONS(Index_multi_range, \
	index_number,int,compare_int)

/*
Global functions
----------------
*/

struct Index_multi_range *CREATE(Index_multi_range)(int index_number)
/*******************************************************************************
LAST MODIFIED : 23 March 1998

DESCRIPTION :
Returns a new status range structure for following the tracking status of
node index_number.
==============================================================================*/
{
	struct Index_multi_range *index_multi_range;

	ENTER(CREATE(Index_multi_range));
	/* allocate memory for structure */
	if (ALLOCATE(index_multi_range,struct Index_multi_range,1)&&
		(index_multi_range->ranges=CREATE(Multi_range)()))
	{
		index_multi_range->index_number=index_number;
		index_multi_range->access_count=0;
	}
	else
	{
		if (index_multi_range)
		{
			DEALLOCATE(index_multi_range);
		}
		display_message(ERROR_MESSAGE,
			"CREATE(Index_multi_range).  Not enough memory");
	}
	LEAVE;

	return (index_multi_range);
} /* CREATE(Index_multi_range) */

int DESTROY(Index_multi_range)(struct Index_multi_range **index_multi_range_address)
/*******************************************************************************
LAST MODIFIED : 1 September 2000

DESCRIPTION :
Frees the memory for the index_multi_range_ranges and sets <*index_multi_range_address>
to NULL.
==============================================================================*/
{
	int return_code;

	ENTER(DESTROY(Index_multi_range));
	if (index_multi_range_address && *index_multi_range_address)
	{
		if (0==(*index_multi_range_address)->access_count)
		{
			DESTROY(Multi_range)(&((*index_multi_range_address)->ranges));
			DEALLOCATE(*index_multi_range_address);
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"DESTROY(Index_multi_range).  Non-zero access count!");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(Index_multi_range).  Missing address");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Index_multi_range) */

DECLARE_OBJECT_FUNCTIONS(Index_multi_range)
DECLARE_INDEXED_LIST_FUNCTIONS(Index_multi_range)
DECLARE_FIND_BY_IDENTIFIER_IN_INDEXED_LIST_FUNCTION(Index_multi_range, \
	index_number,int,compare_int)

int Index_multi_range_add(struct Index_multi_range *index_multi_range,
	struct Index_multi_range *index_multi_range_to_add)
/*******************************************************************************
LAST MODIFIED : 24 March 1998

DESCRIPTION :
Adds the ranges in the second Index_multi_range to the first.
==============================================================================*/
{
	int return_code,number_of_ranges,range_no,start,stop;

	ENTER(Index_multi_range_add);
	if (index_multi_range&&index_multi_range_to_add)
	{
		number_of_ranges=
			Multi_range_get_number_of_ranges(index_multi_range_to_add->ranges);
		return_code=1;
		for (range_no=0;return_code&&(range_no<number_of_ranges);range_no++)
		{
			return_code=Multi_range_get_range(index_multi_range_to_add->ranges,
				range_no,&start,&stop)&&
				Multi_range_add_range(index_multi_range->ranges,start,stop);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Index_multi_range_add.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_add */

int Index_multi_range_add_to_list(struct Index_multi_range *index_multi_range,
	void *index_multi_range_list_void)
/*******************************************************************************
LAST MODIFIED : 24 March 1998

DESCRIPTION :
Iterator function for adding the ranges in index_multi_range to those for the same
index_number in index_multi_range_list.
==============================================================================*/
{
	int return_code;
	struct Index_multi_range *index_multi_range_to_modify;
	struct LIST(Index_multi_range) *index_multi_range_list;

	ENTER(Index_multi_range_add_to_list);
	if (index_multi_range&&(index_multi_range_list=(struct LIST(Index_multi_range) *)
		index_multi_range_list_void))
	{
		index_multi_range_to_modify = FIND_BY_IDENTIFIER_IN_LIST(Index_multi_range,index_number)(
			index_multi_range->index_number,index_multi_range_list);
		if (NULL != index_multi_range_to_modify)
		{
			return_code=Index_multi_range_add(index_multi_range_to_modify,index_multi_range);
		}
		else
		{
			display_message(ERROR_MESSAGE,"Index_multi_range_add_to_list.  "
				"Could not find status of node %d",index_multi_range->index_number);
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Index_multi_range_add_to_list.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_add_to_list */

struct Index_multi_range_modify_at_value_data
{
	struct LIST(Index_multi_range) *index_multi_range_list_to_modify;
	int value;
	/* adding range if the following is set, otherwise removing */
	int adding;
};

static int Index_multi_range_modify_at_value_to_list(struct Index_multi_range *index_multi_range,
	void *modify_data_void)
/*******************************************************************************
LAST MODIFIED : 10 April 1998

DESCRIPTION :
Iterator function for modifying the ranges in index_multi_range to those for the same
index_number in index_multi_range_list - but only at the single value.
The modification is either adding or removing.
Used to update the tracking editor bar chart during processing.
Note that only members common to both the index_multi_range_list_to_modify and the
list it is modified by may be changed; it is no error if they do not match up.
==============================================================================*/
{
	int return_code;
	struct Index_multi_range *index_multi_range_to_modify;
	struct Index_multi_range_modify_at_value_data *modify_data;

	ENTER(Index_multi_range_modify_at_value_to_list);
	if (index_multi_range&&
		(modify_data=(struct Index_multi_range_modify_at_value_data *)modify_data_void))
	{
		if (Multi_range_is_value_in_range(index_multi_range->ranges,modify_data->value))
		{
			index_multi_range_to_modify = FIND_BY_IDENTIFIER_IN_LIST(Index_multi_range,index_number)(
				index_multi_range->index_number, modify_data->index_multi_range_list_to_modify);
			if (NULL != index_multi_range_to_modify)
			{
				if (modify_data->adding)
				{
					return_code=Multi_range_add_range(index_multi_range_to_modify->ranges,
						modify_data->value,modify_data->value);
				}
				else
				{
					return_code=Multi_range_remove_range(index_multi_range_to_modify->ranges,
						modify_data->value,modify_data->value);
				}
			}
			else
			{
				/* not an error - only modify if in both lists */
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
			"Index_multi_range_modify_at_value_to_list.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_modify_at_value_to_list */

int Index_multi_range_clear(struct Index_multi_range *index_multi_range,void *dummy_void)
/*******************************************************************************
LAST MODIFIED : 1 September 2000

DESCRIPTION :
Clears all ranges in the index_multi_range.
==============================================================================*/
{
	int return_code;

	ENTER(Index_multi_range_clear);
	USE_PARAMETER(dummy_void);
	if (index_multi_range)
	{
		return_code=Multi_range_clear(index_multi_range->ranges);
	}
	else
	{
		display_message(ERROR_MESSAGE,"Index_multi_range_clear.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_clear */

int Index_multi_range_copy(struct Index_multi_range *destination,
	struct Index_multi_range *source)
/*******************************************************************************
LAST MODIFIED : 26 April 1998

DESCRIPTION :
Makes <destination> ranges an exact copy of those in <source>.
The index_number member of destination is not modified.
==============================================================================*/
{
	int return_code;

	ENTER(Index_multi_range_copy);
	if (destination&&source)
	{
		return_code=Multi_range_copy(destination->ranges,source->ranges);
	}
	else
	{
		display_message(ERROR_MESSAGE,"Index_multi_range_copy.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_copy */

int Index_multi_range_copy_to_list(struct Index_multi_range *index_multi_range,
	void *index_multi_range_list_copy_void)
/*******************************************************************************
LAST MODIFIED : 26 April 1998

DESCRIPTION :
Puts a copy of <index_multi_range> in <index_multi_range_list_copy>, creating it if needed.
==============================================================================*/
{
	int return_code;
	struct Index_multi_range *index_multi_range_copy;
	struct LIST(Index_multi_range) *index_multi_range_list_copy;

	ENTER(Index_multi_range_copy_to_list);
	if (index_multi_range&&(index_multi_range_list_copy=
		(struct LIST(Index_multi_range) *)index_multi_range_list_copy_void))
	{
		if (!(index_multi_range_copy=FIND_BY_IDENTIFIER_IN_LIST(Index_multi_range,index_number)(
			index_multi_range->index_number,index_multi_range_list_copy)))
		{
			index_multi_range_copy = CREATE(Index_multi_range)(index_multi_range->index_number);
			if (NULL != index_multi_range_copy)
			{
				if (!ADD_OBJECT_TO_LIST(Index_multi_range)(index_multi_range_copy,
					index_multi_range_list_copy))
				{
					DESTROY(Index_multi_range)(&index_multi_range_copy);
				}
			}
		}
		if (index_multi_range_copy)
		{
			return_code=
				Multi_range_copy(index_multi_range_copy->ranges,index_multi_range->ranges);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Index_multi_range_copy_to_list.  Could not get copy");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Index_multi_range_copy_to_list.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_copy_to_list */

int Index_multi_range_get_index_number(struct Index_multi_range *index_multi_range)
/*******************************************************************************
LAST MODIFIED : 6 April 1998

DESCRIPTION :
Returns the node number of <index_multi_range>.
==============================================================================*/
{
	int return_code;

	ENTER(Index_multi_range_get_index_number);
	if (index_multi_range)
	{
		return_code=index_multi_range->index_number;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Index_multi_range_get_index_number.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_get_index_number */

int Index_multi_range_get_number_of_ranges(struct Index_multi_range *index_multi_range)
/*******************************************************************************
LAST MODIFIED : 3 April 1998

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(Index_multi_range_get_number_of_ranges);
	if (index_multi_range)
	{
		return_code=Multi_range_get_number_of_ranges(index_multi_range->ranges);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Index_multi_range_get_number_of_ranges.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_get_number_of_ranges */

int Index_multi_range_get_total_number_in_ranges(
	struct Index_multi_range *index_multi_range)
/*******************************************************************************
LAST MODIFIED : 1 November 2000

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(Index_multi_range_get_total_number_in_ranges);
	if (index_multi_range)
	{
		return_code=Multi_range_get_total_number_in_ranges(index_multi_range->ranges);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Index_multi_range_get_total_number_in_ranges.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_get_total_number_in_ranges */

int Index_multi_range_get_range(struct Index_multi_range *index_multi_range,int range_no,
	int *start,int *stop)
/*******************************************************************************
LAST MODIFIED : 3 April 1998

DESCRIPTION :
Returns the start and stop values for range[range_no] in <index_multi_range>.
Valid range numbers are from 0 to number_of_ranges-1.
==============================================================================*/
{
	int return_code;

	ENTER(Index_multi_range_get_range);
	if (index_multi_range&&start&&stop)
	{
		return_code=Multi_range_get_range(index_multi_range->ranges,range_no,start,stop);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Index_multi_range_get_range.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_get_range */

int Index_multi_range_get_range_containing_value(struct Index_multi_range *index_multi_range,
	int value,int *start,int *stop)
/*******************************************************************************
LAST MODIFIED : 1 April 1998

DESCRIPTION :
Assuming value is in a range of the index_multi_range, this function returns the
start and stop values for that range.
==============================================================================*/
{
	int return_code;

	ENTER(Index_multi_range_get_range_containing_value);
	if (index_multi_range&&Multi_range_is_value_in_range(index_multi_range->ranges,value)&&
		start&&stop)
	{
		return_code=
			Multi_range_get_last_start_value(index_multi_range->ranges,value+1,start)&&
			Multi_range_get_next_stop_value(index_multi_range->ranges,value-1,stop);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Index_multi_range_get_range_containing_value.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_get_range_containing_value */

int Index_multi_range_get_last_start_value(struct Index_multi_range *index_multi_range,
	int value,int *last_start_value)
/*******************************************************************************
LAST MODIFIED : 10 April 1998

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(Index_multi_range_get_last_start_value);
	if (index_multi_range&&last_start_value)
	{
		return_code=Multi_range_get_last_start_value(index_multi_range->ranges,
			value,last_start_value);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Index_multi_range_get_last_start_value.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_get_last_start_value */

int Index_multi_range_get_last_stop_value(struct Index_multi_range *index_multi_range,
	int value,int *last_stop_value)
/*******************************************************************************
LAST MODIFIED : 2 April 1998

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(Index_multi_range_get_last_stop_value);
	if (index_multi_range&&last_stop_value)
	{
		return_code=Multi_range_get_last_stop_value(index_multi_range->ranges,
			value,last_stop_value);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Index_multi_range_get_last_stop_value.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_get_last_stop_value */

int Index_multi_range_get_next_start_value(struct Index_multi_range *index_multi_range,
	int value,int *next_start_value)
/*******************************************************************************
LAST MODIFIED : 2 April 1998

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(Index_multi_range_get_next_start_value);
	if (index_multi_range&&next_start_value)
	{
		return_code=Multi_range_get_next_start_value(index_multi_range->ranges,
			value,next_start_value);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Index_multi_range_get_next_start_value.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_get_next_start_value */

int Index_multi_range_get_next_stop_value(struct Index_multi_range *index_multi_range,
	int value,int *next_stop_value)
/*******************************************************************************
LAST MODIFIED : 10 April 1998

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(Index_multi_range_get_next_stop_value);
	if (index_multi_range&&next_stop_value)
	{
		return_code=Multi_range_get_next_stop_value(index_multi_range->ranges,
			value,next_stop_value);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Index_multi_range_get_next_stop_value.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_get_next_stop_value */

int Index_multi_range_is_value_in_range(struct Index_multi_range *index_multi_range,int value)
/*******************************************************************************
LAST MODIFIED : 1 September 2000

DESCRIPTION :
Returns true if <value> is in any range in <index_multi_range>.
==============================================================================*/
{
	int return_code;

	ENTER(Index_multi_range_is_value_in_range);
	if (index_multi_range)
	{
		return_code=Multi_range_is_value_in_range(index_multi_range->ranges,value);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Index_multi_range_is_value_in_range.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_is_value_in_range */

int Index_multi_range_is_value_in_range_iterator(struct Index_multi_range *index_multi_range,
	void *value_address_void)
/*******************************************************************************
LAST MODIFIED : 1 September 2000

DESCRIPTION :
Iterator version of Index_multi_range_is_value_in_range. <value_address> points at the
integer value.
==============================================================================*/
{
	int return_code,*value_address;

	ENTER(Index_multi_range_is_value_in_range);
	if (index_multi_range&&(value_address=(int *)value_address_void))
	{
		return_code=
			Multi_range_is_value_in_range(index_multi_range->ranges,*value_address);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Index_multi_range_is_value_in_range.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_is_value_in_range */

int Index_multi_range_value_pair_have_different_status(
	struct Index_multi_range *index_multi_range,void *value_pair_void)
/*******************************************************************************
LAST MODIFIED : 6 September 2000

DESCRIPTION :
Iterator function returning true if value1 and value2 in the <value_pair> are
not in the same state in the <index_multi_range>, ie. one is in and one is out.
==============================================================================*/
{
	int return_code;
	struct Index_multi_range_value_pair *value_pair;

	ENTER(Index_multi_range_value_pair_have_different_status);
	if (index_multi_range&&
		(value_pair=(struct Index_multi_range_value_pair *)value_pair_void))
	{
		return_code=(
			Multi_range_is_value_in_range(index_multi_range->ranges,value_pair->value1) !=
			Multi_range_is_value_in_range(index_multi_range->ranges,value_pair->value2));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Index_multi_range_value_pair_have_different_status.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_value_pair_have_different_status */

int Index_multi_range_not_clear(struct Index_multi_range *index_multi_range,void *dummy_void)
/*******************************************************************************
LAST MODIFIED : 1 September 2000

DESCRIPTION :
Iterator function returning true if there are any ranges defined for this
index_multi_range.
==============================================================================*/
{
	int return_code;

	ENTER(Index_multi_range_clear);
	USE_PARAMETER(dummy_void);
	if (index_multi_range)
	{
		return_code=(0<Multi_range_get_number_of_ranges(index_multi_range->ranges));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Index_multi_range_not_clear.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_clear */

int Index_multi_range_subtract(struct Index_multi_range *index_multi_range,
	struct Index_multi_range *index_multi_range_to_add)
/*******************************************************************************
LAST MODIFIED : 24 March 1998

DESCRIPTION :
Removes the ranges in the second Index_multi_range from the first.
==============================================================================*/
{
	int return_code,number_of_ranges,range_no,start,stop;

	ENTER(Index_multi_range_subtract);
	if (index_multi_range&&index_multi_range_to_add)
	{
		number_of_ranges=
			Multi_range_get_number_of_ranges(index_multi_range_to_add->ranges);
		return_code=1;
		for (range_no=0;return_code&&(range_no<number_of_ranges);range_no++)
		{
			return_code=Multi_range_get_range(index_multi_range_to_add->ranges,
				range_no,&start,&stop)&&
				Multi_range_remove_range(index_multi_range->ranges,start,stop);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Index_multi_range_subtract.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_subtract */

int Index_multi_range_subtract_from_list(struct Index_multi_range *index_multi_range,
	void *index_multi_range_list_void)
/*******************************************************************************
LAST MODIFIED : 1 September 2000

DESCRIPTION :
Iterator function for removing the ranges in index_multi_range from those for the
same index_number in index_multi_range_list.
==============================================================================*/
{
	int return_code;
	struct Index_multi_range *index_multi_range_to_modify;
	struct LIST(Index_multi_range) *index_multi_range_list;

	ENTER(Index_multi_range_subtract_from_list);
	if (index_multi_range&&(index_multi_range_list=(struct LIST(Index_multi_range) *)
		index_multi_range_list_void))
	{
		index_multi_range_to_modify = FIND_BY_IDENTIFIER_IN_LIST(Index_multi_range,index_number)(
			index_multi_range->index_number, index_multi_range_list);
		if (NULL != index_multi_range_to_modify)
		{
			return_code=Index_multi_range_subtract(index_multi_range_to_modify,index_multi_range);
		}
		else
		{
			display_message(ERROR_MESSAGE,"Index_multi_range_subtract_from_list.  "
				"Could not find status of node %d",index_multi_range->index_number);
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Index_multi_range_subtract_from_list.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_subtract_from_list */

int Index_multi_range_add_range(struct Index_multi_range *index_multi_range,int start,int stop)
/*******************************************************************************
LAST MODIFIED : 24 March 1998

DESCRIPTION :
Adds the range from start to stop to index_multi_range.
==============================================================================*/
{
	int return_code;

	ENTER(Index_multi_range_add_range);
	if (index_multi_range&&(stop >= start))
	{
		return_code=Multi_range_add_range(index_multi_range->ranges,start,stop);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Index_multi_range_add_range.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_add_range */

int Index_multi_range_remove_range(struct Index_multi_range *index_multi_range,int start,int stop)
/*******************************************************************************
LAST MODIFIED : 24 March 1998

DESCRIPTION :
Removes the range from start to stop from index_multi_range.
==============================================================================*/
{
	int return_code;

	ENTER(Index_multi_range_remove_range);
	if (index_multi_range&&(stop >= start))
	{
		return_code=Multi_range_remove_range(index_multi_range->ranges,start,stop);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Index_multi_range_remove_range.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_remove_range */

int Index_multi_range_remove_range_iterator(struct Index_multi_range *index_multi_range,
	void *single_range_void)
/*******************************************************************************
LAST MODIFIED : 1 September 2000

DESCRIPTION :
Removes from <index_multi_range> the range in <single_range>, a struct Single_range *.
==============================================================================*/
{
	int return_code;
	struct Single_range *single_range;

	ENTER(Index_multi_range_remove_range);
	if (index_multi_range&&(single_range=(struct Single_range *)single_range_void))
	{
		return_code = Index_multi_range_remove_range(index_multi_range,
			single_range->start,single_range->stop);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Index_multi_range_remove_range.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_remove_range */

struct Index_multi_range_write_data
{
	FILE *out_file;
	char *line_format;
};

int Index_multi_range_write(struct Index_multi_range *index_multi_range,
	void *index_multi_range_write_data_void)
/*******************************************************************************
LAST MODIFIED : 24 March 1998

DESCRIPTION :
For each range write the index_number, start and stop with the supplied format.
==============================================================================*/
{
	int return_code,number_of_ranges,range_no,start,stop;
	struct Index_multi_range_write_data *write_data;

	ENTER(Index_multi_range_write);
	if (index_multi_range&&
		(write_data=(struct Index_multi_range_write_data *)index_multi_range_write_data_void))
	{
		number_of_ranges=Multi_range_get_number_of_ranges(index_multi_range->ranges);
		return_code=1;
		for (range_no=0;return_code&&(range_no<number_of_ranges);range_no++)
		{
			if (Multi_range_get_range(index_multi_range->ranges,
				range_no,&start,&stop))
			{
				fprintf(write_data->out_file,write_data->line_format,
					index_multi_range->index_number,start,stop);
			}
			else
			{
				return_code = 0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Index_multi_range_write.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_write */

int Index_multi_range_list_add(struct LIST(Index_multi_range) *index_multi_range_list,
	struct LIST(Index_multi_range) *index_multi_range_list_to_add)
/*******************************************************************************
LAST MODIFIED : 24 March 1998

DESCRIPTION :
Adds the ranges in the second index_multi_range_list to those in the first.
==============================================================================*/
{
	int return_code;

	ENTER(Index_multi_range_list_add);
	if (index_multi_range_list&&index_multi_range_list_to_add)
	{
		return_code=FOR_EACH_OBJECT_IN_LIST(Index_multi_range)(
			Index_multi_range_add_to_list,(void *)index_multi_range_list,index_multi_range_list_to_add);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Index_multi_range_list_add.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_list_add */

int Index_multi_range_list_add_at_value(struct LIST(Index_multi_range) *index_multi_range_list,
	struct LIST(Index_multi_range) *index_multi_range_list_to_add,int value)
/*******************************************************************************
LAST MODIFIED : 10 April 1998

DESCRIPTION :
Adds the ranges in the second index_multi_range_list to those in the first, but only
at the single specified value.
==============================================================================*/
{
	int return_code;
	struct Index_multi_range_modify_at_value_data add_data;

	ENTER(Index_multi_range_list_add_at_value);
	if (index_multi_range_list&&index_multi_range_list_to_add)
	{
		add_data.index_multi_range_list_to_modify=index_multi_range_list;
		add_data.value=value;
		add_data.adding=1;
		return_code=FOR_EACH_OBJECT_IN_LIST(Index_multi_range)(
			Index_multi_range_modify_at_value_to_list,(void *)&add_data,
			index_multi_range_list_to_add);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Index_multi_range_list_add_at_value.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_list_add_at_value */

int Index_multi_range_list_clear(struct LIST(Index_multi_range) *index_multi_range_list)
/*******************************************************************************
LAST MODIFIED : 25 March 1998

DESCRIPTION :
Clears all ranges for all nodes in the list.
==============================================================================*/
{
	int return_code;

	ENTER(Index_multi_range_list_clear);
	if (index_multi_range_list)
	{
		return_code=FOR_EACH_OBJECT_IN_LIST(Index_multi_range)(Index_multi_range_clear,
			(void *)NULL,index_multi_range_list);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Index_multi_range_list_clear.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_list_clear */

struct LIST(Index_multi_range) *Index_multi_range_list_duplicate(
	struct LIST(Index_multi_range) *index_multi_range_list)
/*******************************************************************************
LAST MODIFIED : 1 September 2000

DESCRIPTION :
Returns a new list containing an exact copy of <index_multi_range_list>.
==============================================================================*/
{
	struct LIST(Index_multi_range) *index_multi_range_list_copy;

	ENTER(Index_multi_range_list_duplicate);
	if (index_multi_range_list)
	{
		index_multi_range_list_copy = CREATE(LIST(Index_multi_range))();
		if (NULL != index_multi_range_list_copy)
		{
			if (!FOR_EACH_OBJECT_IN_LIST(Index_multi_range)(Index_multi_range_copy_to_list,
				(void *)index_multi_range_list_copy,index_multi_range_list))
			{
				display_message(ERROR_MESSAGE,
					"Index_multi_range_list_duplicate.  Could not copy list");
				DESTROY(LIST(Index_multi_range))(&index_multi_range_list_copy);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Index_multi_range_list_duplicate.  Could not create copy");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Index_multi_range_list_duplicate.  Invalid argument(s)");
		index_multi_range_list_copy=(struct LIST(Index_multi_range) *)NULL;
	}
	LEAVE;

	return (index_multi_range_list_copy);
} /* Index_multi_range_list_duplicate */

int Index_multi_range_list_subtract(struct LIST(Index_multi_range) *index_multi_range_list,
	struct LIST(Index_multi_range) *index_multi_range_list_to_subtract)
/*******************************************************************************
LAST MODIFIED : 24 March 1998

DESCRIPTION :
Subtracts the ranges in the second index_multi_range_list from those in the first.
==============================================================================*/
{
	int return_code;

	ENTER(Index_multi_range_list_subtract);
	if (index_multi_range_list&&index_multi_range_list_to_subtract)
	{
		return_code=FOR_EACH_OBJECT_IN_LIST(Index_multi_range)(
			Index_multi_range_subtract_from_list,(void *)index_multi_range_list,
			index_multi_range_list_to_subtract);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Index_multi_range_list_subtract.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_list_subtract */

int Index_multi_range_list_subtract_at_value(
	struct LIST(Index_multi_range) *index_multi_range_list,
	struct LIST(Index_multi_range) *index_multi_range_list_to_subtract,int value)
/*******************************************************************************
LAST MODIFIED : 10 April 1998

DESCRIPTION :
Subtracts the ranges in the second index_multi_range_list from those in the first, but
only at the single specified value.
==============================================================================*/
{
	int return_code;
	struct Index_multi_range_modify_at_value_data subtract_data;

	ENTER(Index_multi_range_list_subtract_at_value);
	if (index_multi_range_list&&index_multi_range_list_to_subtract)
	{
		subtract_data.index_multi_range_list_to_modify=index_multi_range_list;
		subtract_data.value=value;
		subtract_data.adding=0;
		return_code=FOR_EACH_OBJECT_IN_LIST(Index_multi_range)(
			Index_multi_range_modify_at_value_to_list,(void *)&subtract_data,
			index_multi_range_list_to_subtract);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Index_multi_range_list_subtract_at_value.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_list_subtract_at_value */

int Index_multi_range_list_read(struct LIST(Index_multi_range) *index_multi_range_list,
	char *file_name)
/*******************************************************************************
LAST MODIFIED : 26 March 1998

DESCRIPTION :
Reads the lines containing index_number, start and stop from file_name into the
index_multi_range_list.
==============================================================================*/
{
	char type,buff[BUFSIZ];
	FILE *in_file;
	int return_code,index_number,start,stop,end_of_file;
	struct Index_multi_range *index_multi_range;

	ENTER(Index_multi_range_list_read);
	if (index_multi_range_list&&file_name)
	{
		in_file = fopen(file_name,"r");
		if (NULL != in_file)
		{
			return_code=1;
			end_of_file=0;
			while (return_code&&!end_of_file)
			{
				if (NULL==fgets(buff,BUFSIZ,in_file))
				{
					end_of_file=1;
				}
				else
				{
					if ((buff[0] != '#')&&(buff[0] != '!'))
					{
						if (4==sscanf(buff,"%c %d %d %d",&type,&index_number,&start,&stop))
						{
							index_multi_range = FIND_BY_IDENTIFIER_IN_LIST(Index_multi_range,index_number)(
								index_number,index_multi_range_list);
							if (NULL != index_multi_range)
							{
								return_code=Index_multi_range_add_range(index_multi_range,start,stop);
							}
							else
							{
								display_message(ERROR_MESSAGE,"Index_multi_range_list_read.  "
									"Could not find information for node %d",index_number);
								return_code=0;
							}
						}
						else
						{
							display_message(ERROR_MESSAGE,
								"Index_multi_range_list_read.  Incomplete line of data in file");
							return_code=0;
						}
					}
				}
			}
			fclose(in_file);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Index_multi_range_list_read.  Could not read file '%s'",file_name);
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Index_multi_range_list_read.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_list_read */

int Index_multi_range_list_write(struct LIST(Index_multi_range) *index_multi_range_list,
	char *file_name,char *header_text,char *line_format)
/*******************************************************************************
LAST MODIFIED : 26 March 1998

DESCRIPTION :
For each range of each node write the index_number, start and stop with the supplied
line format. Also write the header_text at the top of the file.
==============================================================================*/
{
	int return_code;
	struct Index_multi_range_write_data write_data;

	ENTER(Index_multi_range_list_write);
	if (index_multi_range_list&&file_name&&header_text&&line_format)
	{
		write_data.out_file=fopen(file_name,"w");
		if (NULL != write_data.out_file)
		{
			fprintf(write_data.out_file,"%s",header_text);
			write_data.line_format=line_format;
			return_code=FOR_EACH_OBJECT_IN_LIST(Index_multi_range)(Index_multi_range_write,
				(void *)&write_data,index_multi_range_list);
			fclose(write_data.out_file);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Index_multi_range_list_write.  Could not create file '%s'",file_name);
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Index_multi_range_list_write.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Index_multi_range_list_write */
