/*******************************************************************************
FILE : multi_range.h

LAST MODIFIED : 21 September 2000

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
#if !defined (MULTI_RANGE_H)
#define MULTI_RANGE_H

/*
Global types
------------
*/

struct Single_range
/*******************************************************************************
LAST MODIFIED : 1 September 2000

DESCRIPTION :
A Multi_range is built out of list of these; this structure is made available
for use eg. in iterator functions for modifing Multi_ranges in objects.
==============================================================================*/
{
	int start,stop;
};

struct Multi_range;

/*
Global functions
----------------
*/
struct Multi_range *CREATE(Multi_range)(void);
/*******************************************************************************
LAST MODIFIED : 11 March 1998

DESCRIPTION :
Creates and returns an empty Multi_range structure.
==============================================================================*/

int DESTROY(Multi_range)(struct Multi_range **multi_range_address);
/*******************************************************************************
LAST MODIFIED : 11 March 1998

DESCRIPTION :
Frees the space used by the Multi_range structure.
==============================================================================*/

int Multi_range_clear(struct Multi_range *multi_range);
/*******************************************************************************
LAST MODIFIED : 25 March 1998

DESCRIPTION :
Clears all the ranges in <multi_range>.
==============================================================================*/

int Multi_range_copy(struct Multi_range *destination,
	struct Multi_range *source);
/*******************************************************************************
LAST MODIFIED : 26 April 1998

DESCRIPTION :
Makes the <destination> Multi_range an exact copy of <source>.
==============================================================================*/

int Multi_range_add_range(struct Multi_range *multi_range,int start,int stop);
/*******************************************************************************
LAST MODIFIED : 15 April 1999

DESCRIPTION :
Makes sure <multi_range> contains a contiguous range from start to stop. Any
ranges that intersect with the new range are combined with it.
If <start> is greater than <stop>, the two are swapped and the range added.
==============================================================================*/

int Multi_range_remove_range(struct Multi_range *multi_range,
	int start,int stop);
/*******************************************************************************
LAST MODIFIED : 15 April 1999

DESCRIPTION :
Makes sure <multi_range> does not have any entries from start to stop.
If <start> is greater than <stop>, the two are swapped and the range added.
==============================================================================*/

int Multi_range_toggle_range(struct Multi_range *multi_range,int start,
	int stop);
/*******************************************************************************
LAST MODIFIED : 29 February 2000

DESCRIPTION :
Toggles the status of all values in <multi_range> from start to stop.
If <start> is greater than <stop>, the two are swapped and the range toggled.
==============================================================================*/

int Multi_range_is_value_in_range(struct Multi_range *multi_range,int value);
/*******************************************************************************
LAST MODIFIED : 12 March 1998

DESCRIPTION :
Returns true if <value> is in any range in <multi_range>.
==============================================================================*/

int Multi_range_intersect(struct Multi_range *multi_range,
	struct Multi_range *other_multi_range);
/*******************************************************************************
LAST MODIFIED : 21 March 2000

DESCRIPTION :
Modifies <multi_range> so it contains only ranges or part ranges in both it and
<other_multi_range>.
==============================================================================*/

int Multi_ranges_overlap(struct Multi_range *multi_range1,
	struct Multi_range *multi_range2);
/*******************************************************************************
LAST MODIFIED : 21 April 1999

DESCRIPTION :
Returns true if <multi_range1> and <multi_range2> have any overlapping ranges.
==============================================================================*/

int Multi_range_get_last_start_value(struct Multi_range *multi_range,int value,
	int *last_start_value);
/*******************************************************************************
LAST MODIFIED : 1 April 1998

DESCRIPTION :
Returns the next lower start value before value in the <multi_range>.
If there is none or an error occurs, the return_code will be 0.
==============================================================================*/

int Multi_range_get_last_stop_value(struct Multi_range *multi_range,int value,
	int *last_stop_value);
/*******************************************************************************
LAST MODIFIED : 12 March 1998

DESCRIPTION :
Returns the next lower stop value to value in the <multi_range>. If there is
none or an error occurs, the return_code will be 0.
==============================================================================*/

int Multi_range_get_next_start_value(struct Multi_range *multi_range,int value,
	int *next_start_value);
/*******************************************************************************
LAST MODIFIED : 12 March 1998

DESCRIPTION :
Returns the next higher start value to value in the <multi_range>. If there is
none or an error occurs, the return_code will be 0.
==============================================================================*/

int Multi_range_get_next_stop_value(struct Multi_range *multi_range,int value,
	int *next_stop_value);
/*******************************************************************************
LAST MODIFIED : 1 April 1998

DESCRIPTION :
Returns the next higher stop value to value in the <multi_range>. If there is
none or an error occurs, the return_code will be 0.
==============================================================================*/

int Multi_range_get_number_of_ranges(struct Multi_range *multi_range);
/*******************************************************************************
LAST MODIFIED : 12 March 1998

DESCRIPTION :
==============================================================================*/

int Multi_range_get_range(struct Multi_range *multi_range,int range_no,
	int *start,int *stop);
/*******************************************************************************
LAST MODIFIED : 12 March 1998

DESCRIPTION :
Returns the start and stop values for range[range_no] in <multi_range>.
Valid range numbers are from 0 to number_of_ranges-1.
==============================================================================*/

char *Multi_range_get_ranges_string(struct Multi_range *multi_range);
/*******************************************************************************
LAST MODIFIED : 22 March 2000

DESCRIPTION :
Returns the <multi_range> as an allocated, comma separated string of ranges,
eg. "1,3..7,22". Up to calling function to DEALLOCATE the returned string.
==============================================================================*/

int Multi_range_get_total_number_in_ranges(struct Multi_range *multi_range);
/*******************************************************************************
LAST MODIFIED : 21 March 2000

DESCRIPTION :
Returns the sum of all the number of numbers in the ranges of <multi_range>.
==============================================================================*/

int Multi_range_print(struct Multi_range *multi_range);
/*******************************************************************************
LAST MODIFIED : 12 March 1998

DESCRIPTION :
Writes out the contents of the <multi_range>.
==============================================================================*/

int Multi_range_display_ranges(struct Multi_range *multi_range);
/*******************************************************************************
LAST MODIFIED : 21 September 2000

DESCRIPTION :
Writes the multi-range as a comma separated list to the command window,
eg. 1,5,11..15. If the list is very long it is broken into lines of maximum
length MAX_MULTI_RANGE_DISPLAY_COLUMNS.
Writes <empty> if there is nothing in the multi-range.
==============================================================================*/

int Multi_range_test(void);
/*******************************************************************************
LAST MODIFIED : 12 March 1998

DESCRIPTION :
Temporary.
==============================================================================*/

#endif /* !defined (MULTI_RANGE_H) */
