/*******************************************************************************
FILE : indexed_multi_range.h

LAST MODIFIED : 1 November 2000

DESCRIPTION :
Defines an object with a multirange at each value of an index.  An indexed list
can then be created for a group of these objects allowing rapid lookup.
This code was originally mirage/tracking_editor_data.c and refered explicitly
to Node_status for pending/placed frames etc.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (INDEXED_MULTI_RANGE_H)
#define INDEXED_MULTI_RANGE_H

#include "general/list.h"
#include "general/object.h"

/*
Global types
------------
*/
struct Index_multi_range;

DECLARE_LIST_TYPES(Index_multi_range);

struct Index_multi_range_value_pair
/*******************************************************************************
LAST MODIFIED : 6 September 2000

DESCRIPTION :
Data for passing to Index_multi_range_value_pair_have_different_status iterator
function.
==============================================================================*/
{
	int value1,value2;
};

/*
Global functions
----------------
*/

struct Index_multi_range *CREATE(Index_multi_range)(int index);
/*******************************************************************************
LAST MODIFIED : 23 March 1998

DESCRIPTION :
Returns a new Index_multi_range structure for following the tracking status of
node index.
==============================================================================*/

int DESTROY(Index_multi_range)(struct Index_multi_range **index_multi_range_address);
/*******************************************************************************
LAST MODIFIED : 23 March 1998

DESCRIPTION :
Frees the memory for the index_multi_range and sets <*index_multi_range_address>
to NULL.
==============================================================================*/

PROTOTYPE_OBJECT_FUNCTIONS(Index_multi_range);

PROTOTYPE_LIST_FUNCTIONS(Index_multi_range);
PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(Index_multi_range,index_number,int);

int Index_multi_range_add(struct Index_multi_range *index_multi_range,
	struct Index_multi_range *index_multi_range_to_add);
/*******************************************************************************
LAST MODIFIED : 24 March 1998

DESCRIPTION :
Adds the ranges in the second Index_multi_range to the first.
==============================================================================*/

int Index_multi_range_clear(struct Index_multi_range *index_multi_range,void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 25 March 1998

DESCRIPTION :
Clears all ranges in the index_multi_range.
==============================================================================*/

int Index_multi_range_copy(struct Index_multi_range *destination,
	struct Index_multi_range *source);
/*******************************************************************************
LAST MODIFIED : 26 April 1998

DESCRIPTION :
Makes <destination> ranges an exact copy of those in <source>.
The index member of destination is not modified.
==============================================================================*/

int Index_multi_range_get_index_number(struct Index_multi_range *index_multi_range);
/*******************************************************************************
LAST MODIFIED : 6 April 1998

DESCRIPTION :
Returns the node number of <index_multi_range>.
==============================================================================*/

int Index_multi_range_get_number_of_ranges(
	struct Index_multi_range *index_multi_range);
/*******************************************************************************
LAST MODIFIED : 3 April 1998

DESCRIPTION :
==============================================================================*/

int Index_multi_range_get_total_number_in_ranges(
	struct Index_multi_range *index_multi_range);
/*******************************************************************************
LAST MODIFIED : 1 November 2000

DESCRIPTION :
==============================================================================*/

int Index_multi_range_get_range(struct Index_multi_range *index_multi_range,int range_no,
	int *start,int *stop);
/*******************************************************************************
LAST MODIFIED : 3 April 1998

DESCRIPTION :
Returns the start and stop values for range[range_no] in <index_multi_range>.
Valid range numbers are from 0 to number_of_ranges-1.
==============================================================================*/

int Index_multi_range_get_range_containing_value(struct Index_multi_range *index_multi_range,
	int value,int *start,int *stop);
/*******************************************************************************
LAST MODIFIED : 1 April 1998

DESCRIPTION :
Assuming value is in a range of the index_multi_range, this function returns the
start and stop values for that range.
==============================================================================*/

int Index_multi_range_get_last_start_value(struct Index_multi_range *index_multi_range,
	int value,int *last_start_value);
/*******************************************************************************
LAST MODIFIED : 10 April 1998

DESCRIPTION :
==============================================================================*/

int Index_multi_range_get_last_stop_value(struct Index_multi_range *index_multi_range,
	int value,int *last_stop_value);
/*******************************************************************************
LAST MODIFIED : 2 April 1998

DESCRIPTION :
==============================================================================*/

int Index_multi_range_get_next_start_value(struct Index_multi_range *index_multi_range,
	int value,int *next_start_value);
/*******************************************************************************
LAST MODIFIED : 2 April 1998

DESCRIPTION :
==============================================================================*/

int Index_multi_range_get_next_stop_value(struct Index_multi_range *index_multi_range,
	int value,int *next_stop_value);
/*******************************************************************************
LAST MODIFIED : 10 April 1998

DESCRIPTION :
==============================================================================*/

int Index_multi_range_is_value_in_range(struct Index_multi_range *index_multi_range,int value);
/*******************************************************************************
LAST MODIFIED : 26 March 1998

DESCRIPTION :
Returns true if <value> is in any range in <index_multi_range>.
==============================================================================*/

int Index_multi_range_is_value_in_range_iterator(struct Index_multi_range *index_multi_range,
	void *value_address_void);
/*******************************************************************************
LAST MODIFIED : 1 September 2000

DESCRIPTION :
Iterator version of Index_multi_range_is_value_in_range. <value_address> points at the
integer value.
==============================================================================*/

int Index_multi_range_value_pair_have_different_status(
	struct Index_multi_range *index_multi_range,void *value_pair_void);
/*******************************************************************************
LAST MODIFIED : 6 September 2000

DESCRIPTION :
Iterator function returning true if value1 and value2 in the <value_pair> are
not in the same state in the <index_multi_range>, ie. one is in and one is out.
==============================================================================*/

int Index_multi_range_not_clear(struct Index_multi_range *index_multi_range,void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 7 April 1998

DESCRIPTION :
Iterator function returning true if there are any ranges defined for this
index_multi_range.
==============================================================================*/

int Index_multi_range_subtract(struct Index_multi_range *index_multi_range,
	struct Index_multi_range *index_multi_range_to_add);
/*******************************************************************************
LAST MODIFIED : 24 March 1998

DESCRIPTION :
Removes the ranges in the second Index_multi_range from the first.
==============================================================================*/

int Index_multi_range_add_range(struct Index_multi_range *index_multi_range,int start,int stop);
/*******************************************************************************
LAST MODIFIED : 24 March 1998

DESCRIPTION :
Adds the range from start to stop to index_multi_range.
==============================================================================*/

int Index_multi_range_remove_range(struct Index_multi_range *index_multi_range,
	int start,int stop);
/*******************************************************************************
LAST MODIFIED : 24 March 1998

DESCRIPTION :
Removes the range from start to stop from index_multi_range.
==============================================================================*/

int Index_multi_range_remove_range_iterator(struct Index_multi_range *index_multi_range,
	void *single_range_void);
/*******************************************************************************
LAST MODIFIED : 1 September 2000

DESCRIPTION :
Removes from <index_multi_range> the range in <single_range>, a struct Single_range *.
==============================================================================*/

int Index_multi_range_list_add(struct LIST(Index_multi_range) *index_multi_range_list,
	struct LIST(Index_multi_range) *index_multi_range_list_to_add);
/*******************************************************************************
LAST MODIFIED : 24 March 1998

DESCRIPTION :
Adds the ranges in the second index_multi_range_list to those in the first.
==============================================================================*/

int Index_multi_range_list_add_at_value(struct LIST(Index_multi_range) *index_multi_range_list,
	struct LIST(Index_multi_range) *index_multi_range_list_to_add,int value);
/*******************************************************************************
LAST MODIFIED : 10 April 1998

DESCRIPTION :
Adds the ranges in the second index_multi_range_list to those in the first, but only
at the single specified value.
==============================================================================*/

int Index_multi_range_list_clear(struct LIST(Index_multi_range) *index_multi_range_list);
/*******************************************************************************
LAST MODIFIED : 25 March 1998

DESCRIPTION :
Clears all ranges for all nodes in the list.
==============================================================================*/

struct LIST(Index_multi_range) *Index_multi_range_list_duplicate(
	struct LIST(Index_multi_range) *index_multi_range_list);
/*******************************************************************************
LAST MODIFIED : 26 April 1998

DESCRIPTION :
Returns a new list containing an exact copy of <index_multi_range_list>.
==============================================================================*/

int Index_multi_range_list_subtract(struct LIST(Index_multi_range) *index_multi_range_list,
	struct LIST(Index_multi_range) *Index_multi_range_list_to_subtract);
/*******************************************************************************
LAST MODIFIED : 24 March 1998

DESCRIPTION :
Subtracts the ranges in the second index_multi_range_list from those in the first.
==============================================================================*/

int Index_multi_range_list_subtract_at_value(
	struct LIST(Index_multi_range) *index_multi_range_list,
	struct LIST(Index_multi_range) *index_multi_range_list_to_subtract,int value);
/*******************************************************************************
LAST MODIFIED : 10 April 1998

DESCRIPTION :
Subtracts the ranges in the second index_multi_range_list from those in the first, but
only at the single specified value.
==============================================================================*/

int Index_multi_range_list_read(struct LIST(Index_multi_range) *index_multi_range_list,
	char *file_name);
/*******************************************************************************
LAST MODIFIED : 24 March 1998

DESCRIPTION :
Reads the lines containing index, start and stop from file_name into the
index_multi_range_list.
==============================================================================*/

int Index_multi_range_list_write(struct LIST(Index_multi_range) *index_multi_range_list,
	char *file_name,char *header_text,char *line_format);
/*******************************************************************************
LAST MODIFIED : 24 March 1998

DESCRIPTION :
For each range of each node write the index, start and stop with the supplied
line format. Also write the header_text at the top of the file.
==============================================================================*/


#endif /* !defined (INDEXED_MULTI_RANGE_H) */
