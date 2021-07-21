/*******************************************************************************
FILE : multi_range.c

LAST MODIFIED : 3 October 2000

DESCRIPTION :
Structure for storing and manipulating multiple, non-overlapping ranges of
values, eg. 1-5,7-7,29-100.
At present, limited to int type, but could be converted to other number types.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stdio.h>

#include "general/debug.h"
#include "general/object.h"
#include "general/multi_range.h"
#include "general/mystring.h"
#include "general/message.h"

/*
Module types
------------
*/

struct Multi_range
{
	int number_of_ranges;
	struct Single_range *range;
};

/*
Module functions
----------------
*/

/*
Global functions
----------------
*/
struct Multi_range *CREATE(Multi_range)(void)
/*******************************************************************************
LAST MODIFIED : 11 March 1998

DESCRIPTION :
Creates and returns an empty Multi_range structure.
==============================================================================*/
{
	struct Multi_range *multi_range;

	ENTER(CREATE(Multi_range));
	if (ALLOCATE(multi_range,struct Multi_range,1))
	{
		multi_range->number_of_ranges=0;
		multi_range->range=(struct Single_range *)NULL;
	}
	else
	{
		display_message(ERROR_MESSAGE,"CREATE(Multi_range).  Not enough memory");
		multi_range=(struct Multi_range *)NULL;
	}
	LEAVE;

	return (multi_range);
} /* CREATE(Multi_range) */

int DESTROY(Multi_range)(struct Multi_range **multi_range_address)
/*******************************************************************************
LAST MODIFIED : 11 March 1998

DESCRIPTION :
Frees the space used by the Multi_range structure.
==============================================================================*/
{
	int return_code;

	ENTER(DESTROY(Multi_range));
	if (multi_range_address&&*multi_range_address)
	{
		if ((*multi_range_address)->range)
		{
			DEALLOCATE((*multi_range_address)->range);
		}
		DEALLOCATE(*multi_range_address);
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"DESTROY(Multi_range).  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Multi_range) */

int Multi_range_clear(struct Multi_range *multi_range)
/*******************************************************************************
LAST MODIFIED : 25 March 1998

DESCRIPTION :
Clears all the ranges in <multi_range>.
==============================================================================*/
{
	int return_code;

	ENTER(Multi_range_clear);
	if (multi_range)
	{
		multi_range->number_of_ranges=0;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"Multi_range_clear.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Multi_range_clear */

int Multi_range_copy(struct Multi_range *destination,
	struct Multi_range *source)
/*******************************************************************************
LAST MODIFIED : 26 April 1998

DESCRIPTION :
Makes the <destination> Multi_range an exact copy of <source>.
==============================================================================*/
{
	int return_code,i;
	struct Single_range *new_range;

	ENTER(Multi_range_copy);
	if (destination&&source)
	{
		if (0 == source->number_of_ranges)
		{
			destination->number_of_ranges=0;
			return_code=1;
		}
		else
		{
			if (REALLOCATE(new_range,destination->range,struct Single_range,
				source->number_of_ranges))
			{
				destination->range=new_range;
				for (i=0;i<source->number_of_ranges;i++)
				{
					destination->range[i].start=source->range[i].start;
					destination->range[i].stop=source->range[i].stop;
				}
				destination->number_of_ranges=source->number_of_ranges;
				return_code=1;
			}
			else
			{
				display_message(ERROR_MESSAGE,"Multi_range_copy.  Could not copy");
				return_code=0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Multi_range_copy.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Multi_range_clear */

int Multi_range_add_range(struct Multi_range *multi_range,int start,int stop)
/*******************************************************************************
LAST MODIFIED : 15 April 1999

DESCRIPTION :
Makes sure <multi_range> contains a contiguous range from start to stop. Any
ranges that intersect with the new range are combined with it.
Note that the range array is not made smaller except by accident - when a
realloc is performed to add space for a new range.
If <start> is greater than <stop>, the two are swapped and the range added.
==============================================================================*/
{
	int return_code,i,int_range_no,swallowed,num_swallowed;
	struct Single_range *new_range;

	ENTER(Multi_range_add_range);
	if (multi_range)
	{
		if (start>stop)
		{
			i=start;
			start=stop;
			stop=i;
		}
		/* get first range intersecting or touching new range */
		int_range_no=multi_range->number_of_ranges;
		for (i=0;i<int_range_no;i++)
		{
			if ((start <= multi_range->range[i].stop+1)&&
				(stop >= multi_range->range[i].start-1))
			{
				int_range_no=i;
			}
		}
		if (int_range_no == multi_range->number_of_ranges)
		{
			/* new range not touching any existing range */
			if (REALLOCATE(new_range,multi_range->range,struct Single_range,
				multi_range->number_of_ranges+1))
			{
				multi_range->range=new_range;
				/* find where to put range and make space for it */
				int_range_no=multi_range->number_of_ranges;
				for (i=0;i<int_range_no;i++)
				{
					if (start < multi_range->range[i].start)
					{
						int_range_no=i;
					}
				}
				for (i=multi_range->number_of_ranges;i>int_range_no;i--)
				{
					multi_range->range[i].start=multi_range->range[i-1].start;
					multi_range->range[i].stop=multi_range->range[i-1].stop;
				}
				multi_range->range[int_range_no].start=start;
				multi_range->range[int_range_no].stop=stop;
				multi_range->number_of_ranges++;
				return_code=1;
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Multi_range_add_range.  Could not allocate new range");
				return_code=0;
			}
		}
		else
		{
			/* new range intersects one or more existing ranges */
			return_code=1;
			if (start < multi_range->range[int_range_no].start)
			{
				multi_range->range[int_range_no].start=start;
			}
			if (stop > multi_range->range[int_range_no].stop)
			{
				multi_range->range[int_range_no].stop=stop;
				/* swallow up any subsequent intersecting or touching ranges */
				swallowed=1;
				num_swallowed=0;
				for (i=int_range_no+1;swallowed&&(i<multi_range->number_of_ranges);i++)
				{
					if (multi_range->range[i].start <=
						(multi_range->range[int_range_no].stop+1))
					{
						if (multi_range->range[i].stop >
							multi_range->range[int_range_no].stop)
						{
							multi_range->range[int_range_no].stop=multi_range->range[i].stop;
						}
						num_swallowed++;
					}
					else
					{
						swallowed=0;
					}
				}
				if (num_swallowed)
				{
					multi_range->number_of_ranges -= num_swallowed;
					for (i=int_range_no+1;i<multi_range->number_of_ranges;i++)
					{
						multi_range->range[i].start=
							multi_range->range[i+num_swallowed].start;
						multi_range->range[i].stop=
							multi_range->range[i+num_swallowed].stop;
					}
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Multi_range_add_range.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Multi_range_add_range */

int Multi_range_remove_range(struct Multi_range *multi_range,
	int start,int stop)
/*******************************************************************************
LAST MODIFIED : 15 April 1999

DESCRIPTION :
Makes sure <multi_range> does not have any entries from start to stop.
If <start> is greater than <stop>, the two are swapped and the range removed.
==============================================================================*/
{
	int return_code,i,int_range_no,swallowed,num_swallowed;
	struct Single_range *new_range;

	ENTER(Multi_range_remove_range);
	if (multi_range)
	{
		if (start>stop)
		{
			i=start;
			start=stop;
			stop=i;
		}
		/* get first range intersecting remove range */
		int_range_no=multi_range->number_of_ranges;
		for (i=0;i<int_range_no;i++)
		{
			if ((start <= multi_range->range[i].stop)&&
				(stop >= multi_range->range[i].start))
			{
				int_range_no=i;
			}
		}
		if (int_range_no == multi_range->number_of_ranges)
		{
			/* already outside of all ranges */
			return_code=1;
		}
		else
		{
			return_code=1;
			if (start > multi_range->range[int_range_no].start)
			{
				if (stop < multi_range->range[int_range_no].stop)
				{
					/* split range in two */
					if (REALLOCATE(new_range,multi_range->range,struct Single_range,
						multi_range->number_of_ranges+1))
					{
						multi_range->range=new_range;
						for (i=multi_range->number_of_ranges;i>int_range_no;i--)
						{
							multi_range->range[i].start=multi_range->range[i-1].start;
							multi_range->range[i].stop=multi_range->range[i-1].stop;
						}
						multi_range->range[int_range_no].stop=start-1;
						multi_range->range[int_range_no+1].start=stop+1;
						multi_range->number_of_ranges++;
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"Multi_range_remove_range.  Could not allocate new range");
						return_code=0;
					}
				}
				else
				{
					multi_range->range[int_range_no].stop=start-1;
				}
				int_range_no++;
			}
			if (return_code)
			{
				/* swallow or shift start of any following ranges that intersect */
				num_swallowed=0;
				swallowed=1;
				for (i=int_range_no;swallowed&&(i<multi_range->number_of_ranges);i++)
				{
					if (stop >= multi_range->range[i].start)
					{
						if (stop >= multi_range->range[i].stop)
						{
							num_swallowed++;
						}
						else
						{
							multi_range->range[i].start=stop+1;
							swallowed=0;
						}
					}
					else
					{
						swallowed=0;
					}
				}
				if (num_swallowed)
				{
					multi_range->number_of_ranges -= num_swallowed;
					for (i=int_range_no;i<multi_range->number_of_ranges;i++)
					{
						multi_range->range[i].start=
							multi_range->range[i+num_swallowed].start;
						multi_range->range[i].stop=
							multi_range->range[i+num_swallowed].stop;
					}
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Multi_range_remove_range.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Multi_range_remove_range */

int Multi_range_toggle_range(struct Multi_range *multi_range,int start,
	int stop)
/*******************************************************************************
LAST MODIFIED : 29 February 2000

DESCRIPTION :
Toggles the status of all values in <multi_range> from start to stop.
If <start> is greater than <stop>, the two are swapped and the range toggled.
==============================================================================*/
{
	int currently_in,i,return_code,toggle_start,toggle_stop;

	ENTER(Multi_range_toggle_range);
	if (multi_range)
	{
		if (start>stop)
		{
			i=start;
			start=stop;
			stop=i;
		}
		toggle_start=start;
		toggle_stop=start-1;
		currently_in=Multi_range_is_value_in_range(multi_range,start);
		return_code=1;
		while ((toggle_stop<stop)&&return_code)
		{
			if (currently_in)
			{
				if (Multi_range_get_next_stop_value(multi_range,toggle_start-1,
					&toggle_stop))
				{
					if (toggle_stop>stop)
					{
						toggle_stop=stop;
					}
					return_code=
						Multi_range_remove_range(multi_range,toggle_start,toggle_stop);
				}
				else
				{
					return_code=0;
				}
			}
			else
			{
				if (Multi_range_get_next_start_value(multi_range,toggle_start,
					&toggle_stop))
				{
					toggle_stop--;
					if (toggle_stop>stop)
					{
						toggle_stop=stop;
					}
				}
				else
				{
					toggle_stop=stop;
				}
				return_code=Multi_range_add_range(multi_range,toggle_start,toggle_stop);
			}
			toggle_start=toggle_stop+1;
			currently_in = !currently_in;
		}
		if (!return_code)
		{
			display_message(ERROR_MESSAGE,"Multi_range_toggle_range.  Failed");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Multi_range_toggle_range.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Multi_range_toggle_range */

int Multi_range_is_value_in_range(struct Multi_range *multi_range,int value)
/*******************************************************************************
LAST MODIFIED : 12 March 1998

DESCRIPTION :
Returns true if <value> is in any range in <multi_range>.
==============================================================================*/
{
	int return_code,i,int_range_no,keep_searching;

	ENTER(Multi_range_is_value_in_range);
	if (multi_range)
	{
		return_code=0;
		/* get first range intersecting remove range */
		int_range_no=multi_range->number_of_ranges;
		keep_searching=1;
		for (i=0;keep_searching&&(i<int_range_no);i++)
		{
			if (value >= multi_range->range[i].start)
			{
				if (value <= multi_range->range[i].stop)
				{
					int_range_no=i;
					return_code=1;
				}
			}
			else
			{
				keep_searching=0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Multi_range_is_value_in_range.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Multi_range_is_value_in_range */

int Multi_range_intersect(struct Multi_range *multi_range,
	struct Multi_range *other_multi_range)
/*******************************************************************************
LAST MODIFIED : 3 October 2000

DESCRIPTION :
Modifies <multi_range> so it contains only ranges or part ranges in both it and
<other_multi_range>.
==============================================================================*/
{
	int last_stop,return_code,start,stop;

	ENTER(Multi_range_intersect);
	if (multi_range&&other_multi_range)
	{
		return_code=1;
		if (0<multi_range->number_of_ranges)
		{
			start=multi_range->range[0].start;
			stop=start-1;
			if (Multi_range_is_value_in_range(other_multi_range,start))
			{
				Multi_range_get_next_stop_value(other_multi_range,stop,&start);
				stop=start;
				start++;
			}
			last_stop=multi_range->range[multi_range->number_of_ranges-1].stop;
			while ((stop<last_stop)&&return_code)
			{
				if (Multi_range_get_next_start_value(other_multi_range,start,&stop))
				{
					stop--;
				}
				else
				{
					stop=last_stop;
				}
				return_code = Multi_range_remove_range(multi_range,start,stop);
				if (return_code)
				{
					Multi_range_get_next_stop_value(other_multi_range,stop,&start);
					stop=start;
					start++;
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Multi_range_intersect.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Multi_range_intersect */

int Multi_ranges_overlap(struct Multi_range *multi_range1,
	struct Multi_range *multi_range2)
/*******************************************************************************
LAST MODIFIED : 21 April 1999

DESCRIPTION :
Returns true if <multi_range1> and <multi_range2> have any overlapping ranges.
==============================================================================*/
{
	int i,j,return_code,start1,stop1;

	ENTER(Multi_ranges_overlap);
	if (multi_range1&&multi_range2)
	{
		return_code=0;
		for (i=0;(i<multi_range1->number_of_ranges)&&!return_code;i++)
		{
			start1=multi_range1->range[i].start;
			stop1=multi_range1->range[i].stop;
			for (j=0;(j<multi_range2->number_of_ranges)&&!return_code;j++)
			{
				return_code=
					(start1 <= multi_range2->range[j].stop)&&
					(stop1  >= multi_range2->range[j].start);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Multi_ranges_overlap.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Multi_ranges_overlap */

int Multi_range_get_last_start_value(struct Multi_range *multi_range,int value,
	int *last_start_value)
/*******************************************************************************
LAST MODIFIED : 1 April 1998

DESCRIPTION :
Returns the next lower start value before value in the <multi_range>.
If there is none or an error occurs, the return_code will be 0.
==============================================================================*/
{
	int return_code,i;

	ENTER(Multi_range_get_last_start_value);
	if (multi_range&&last_start_value)
	{
		return_code=0;
		for (i=multi_range->number_of_ranges-1;(!return_code)&&(-1<i);i--)
		{
			if (value > multi_range->range[i].start)
			{
				*last_start_value=multi_range->range[i].start;
				return_code=1;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Multi_range_get_last_start_value.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Multi_range_get_last_start_value */

int Multi_range_get_last_stop_value(struct Multi_range *multi_range,int value,
	int *last_stop_value)
/*******************************************************************************
LAST MODIFIED : 12 March 1998

DESCRIPTION :
Returns the next lower stop value to value in the <multi_range>. If there is
none or an error occurs, the return_code will be 0.
==============================================================================*/
{
	int return_code,i;

	ENTER(Multi_range_get_last_stop_value);
	if (multi_range&&last_stop_value)
	{
		return_code=0;
		for (i=multi_range->number_of_ranges-1;(!return_code)&&(-1<i);i--)
		{
			if (value > multi_range->range[i].stop)
			{
				*last_stop_value=multi_range->range[i].stop;
				return_code=1;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Multi_range_get_last_stop_value.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Multi_range_get_last_stop_value */

int Multi_range_get_next_start_value(struct Multi_range *multi_range,int value,
	int *next_start_value)
/*******************************************************************************
LAST MODIFIED : 12 March 1998

DESCRIPTION :
Returns the next higher start value to value in the <multi_range>. If there is
none or an error occurs, the return_code will be 0.
==============================================================================*/
{
	int return_code,i;

	ENTER(Multi_range_get_next_start_value);
	if (multi_range&&next_start_value)
	{
		return_code=0;
		for (i=0;(!return_code)&&(i<multi_range->number_of_ranges);i++)
		{
			if (value < multi_range->range[i].start)
			{
				*next_start_value=multi_range->range[i].start;
				return_code=1;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Multi_range_get_next_start_value.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Multi_range_get_next_start_value */

int Multi_range_get_next_stop_value(struct Multi_range *multi_range,int value,
	int *next_stop_value)
/*******************************************************************************
LAST MODIFIED : 1 April 1998

DESCRIPTION :
Returns the next higher stop value to value in the <multi_range>. If there is
none or an error occurs, the return_code will be 0.
==============================================================================*/
{
	int return_code,i;

	ENTER(Multi_range_get_next_stop_value);
	if (multi_range&&next_stop_value)
	{
		return_code=0;
		for (i=0;(!return_code)&&(i<multi_range->number_of_ranges);i++)
		{
			if (value < multi_range->range[i].stop)
			{
				*next_stop_value=multi_range->range[i].stop;
				return_code=1;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Multi_range_get_next_stop_value.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Multi_range_get_next_stop_value */

int Multi_range_get_number_of_ranges(struct Multi_range *multi_range)
/*******************************************************************************
LAST MODIFIED : 12 March 1998

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(Multi_range_get_number_of_ranges);
	if (multi_range)
	{
		return_code=multi_range->number_of_ranges;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Multi_range_get_number_of_ranges.  Invalid argument");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Multi_range_get_number_of_ranges */

int Multi_range_get_range(struct Multi_range *multi_range,int range_no,
	int *start,int *stop)
/*******************************************************************************
LAST MODIFIED : 12 March 1998

DESCRIPTION :
Returns the start and stop values for range[range_no] in <multi_range>.
Valid range numbers are from 0 to number_of_ranges-1.
==============================================================================*/
{
	int return_code;

	ENTER(Multi_range_get_range);
	if (multi_range&&(0<=range_no)&&(range_no<multi_range->number_of_ranges)&&
		start&&stop)
	{
		*start=multi_range->range[range_no].start;
		*stop=multi_range->range[range_no].stop;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Multi_range_get_range.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Multi_range_get_range */

char *Multi_range_get_ranges_string(struct Multi_range *multi_range)
/*******************************************************************************
LAST MODIFIED : 22 March 2000

DESCRIPTION :
Returns the <multi_range> as an allocated, comma separated string of ranges,
eg. "1,3..7,22". Up to calling function to DEALLOCATE the returned string.
==============================================================================*/
{
	char *ranges_string,temp_string[50];
	int error,i;

	ENTER(Multi_range_get_ranges_string);
	if (multi_range)
	{
		ranges_string=(char *)NULL;
		error=0;
		for (i=0;(i<multi_range->number_of_ranges)&&(!error);i++)
		{
			if (0<i)
			{
				append_string(&ranges_string,",",&error);
			}
			if (multi_range->range[i].stop > multi_range->range[i].start)
			{
				sprintf(temp_string,"%d..%d",multi_range->range[i].start,
					multi_range->range[i].stop);
			}
			else
			{
				sprintf(temp_string,"%d",multi_range->range[i].start);
			}
			append_string(&ranges_string,temp_string,&error);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Multi_range_get_ranges_string.  Invalid argument(s)");
		ranges_string=(char *)NULL;
	}
	LEAVE;

	return (ranges_string);
} /* Multi_range_get_ranges_string */

int Multi_range_get_total_number_in_ranges(struct Multi_range *multi_range)
/*******************************************************************************
LAST MODIFIED : 21 March 2000

DESCRIPTION :
Returns the sum of all the number of numbers in the ranges of <multi_range>.
==============================================================================*/
{
	int i,number_in_ranges;

	ENTER(Multi_range_get_total_number_of_ranges);
	number_in_ranges=0;
	if (multi_range)
	{
		for (i=0;i<multi_range->number_of_ranges;i++)
		{
			number_in_ranges +=
				(multi_range->range[i].stop - multi_range->range[i].start + 1);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Multi_range_get_total_number_of_ranges.  Invalid argument");
	}
	LEAVE;

	return (number_in_ranges);
} /* Multi_range_get_total_number_of_ranges */

int Multi_range_print(struct Multi_range *multi_range)
/*******************************************************************************
LAST MODIFIED : 12 March 1998

DESCRIPTION :
Writes out the contents of the <multi_range>.
==============================================================================*/
{
	int return_code,i;

	ENTER(Multi_range_print);
	if (multi_range)
	{
		if (0<multi_range->number_of_ranges)
		{
			for (i=0;i<multi_range->number_of_ranges;i++)
			{
				printf("  %6i: %6i - %6i\n",i,multi_range->range[i].start,
					multi_range->range[i].stop);
			}
		}
		else
		{
			printf("  No ranges defined.\n");
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"Multi_range_print.  Invalid argument");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Multi_range_print */

int Multi_range_display_ranges(struct Multi_range *multi_range)
/*******************************************************************************
LAST MODIFIED : 21 September 2000

DESCRIPTION :
Writes the multi-range as a comma separated list to the command window,
eg. 1,5,11..15. If the list is very long it is broken into lines of maximum
length MAX_MULTI_RANGE_DISPLAY_COLUMNS.
Writes <empty> if there is nothing in the multi-range.
==============================================================================*/
{
#define MAX_MULTI_RANGE_DISPLAY_COLUMNS 80
	char *ranges_string,*remaining_string;
	int length_to_print,remaining_length,return_code;

	ENTER(Multi_range_display_ranges);
	if (multi_range)
	{
		return_code=1;
		if (0<multi_range->number_of_ranges)
		{
			if (NULL != (ranges_string=Multi_range_get_ranges_string(multi_range)))
			{
				remaining_string = ranges_string;
				remaining_length = strlen(remaining_string);
				while ((0<remaining_length)&&return_code)
				{
					if (remaining_length < MAX_MULTI_RANGE_DISPLAY_COLUMNS)
					{
						display_message(INFORMATION_MESSAGE,remaining_string);
						display_message(INFORMATION_MESSAGE,"\n");
						remaining_length=0;
					}
					else
					{
						/* go back to last comma in string */
						length_to_print = MAX_MULTI_RANGE_DISPLAY_COLUMNS;
						while (length_to_print&&(remaining_string[length_to_print] != ','))
						{
							length_to_print--;
						}
						if (0<length_to_print)
						{
							/* null terminate string */
							remaining_string[length_to_print] = '\0';
							display_message(INFORMATION_MESSAGE,remaining_string);
							display_message(INFORMATION_MESSAGE,",\n");
							remaining_string += (length_to_print+1);
							remaining_length -= (length_to_print+1);
						}
						else
						{
							return_code=0;
						}
					}
				}
				DEALLOCATE(ranges_string);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Multi_range_display_ranges.  Could not get ranges string");
				return_code=0;
			}
		}
		else
		{
			/* no ranges */
			display_message(INFORMATION_MESSAGE,"<empty>\n");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Multi_range_display_ranges.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Multi_range_display_ranges */

int Multi_range_test(void)
/*******************************************************************************
LAST MODIFIED : 12 March 1998

DESCRIPTION :
Temporary.
==============================================================================*/
{
	char cmd;
	int return_code,v1,v2,r = 0;
	struct Multi_range *multi_range;

	ENTER(Multi_range_test);
	multi_range=CREATE(Multi_range)();
	cmd='a';
	while (cmd != 'q')
	{
		printf("\nRanges:\n");
		Multi_range_print(multi_range);
		printf("\na/r/i/l/n/m/g # #: \n");
		if (1 < scanf("%c %d %d",&cmd,&v1,&v2))
		{
			switch (cmd)
			{
				case 'a':
				{
					printf("Add: %i %i\n",v1,v2);
					Multi_range_add_range(multi_range,v1,v2);
				} break;
				case 'r':
				{
					printf("Remove: %i %i\n",v1,v2);
					Multi_range_remove_range(multi_range,v1,v2);
				} break;
				case 'i':
				{
					r=Multi_range_is_value_in_range(multi_range,v1);
					printf("Is %i in range: %i\n",v1,r);
				} break;
				case 'l':
				{
					if (Multi_range_get_last_stop_value(multi_range,v1,&r))
					{
						printf("Last stop value before %i: %i\n",v1,r);
					}
					else
					{
						printf("No Last stop value before %i\n",v1);
					}
				} break;
				case 'n':
				{
					if (Multi_range_get_next_start_value(multi_range,v1,&r))
					{
						printf("Next start value after %i: %i\n",v1,r);
					}
					else
					{
						printf("No Next start value after %i\n",v1);
					}
				} break;
				case 'm':
				{
					r=Multi_range_get_number_of_ranges(multi_range);
					printf("Number of ranges: %i\n",r);
				} break;
				case 'g':
				{
					Multi_range_get_range(multi_range,v1,&v2,&r);
					printf("Range[%i]: %i %i\n",v1,v2,r);
				} break;
			}
		}
		else
		{
			printf("Error parsing command\n");
		}
	}
	return_code=1;
	LEAVE;

	return (return_code);
} /* Multi_range_test */

