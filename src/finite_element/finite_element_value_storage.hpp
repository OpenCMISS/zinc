/**
 * FILE : finite_element_values_storage.hpp
 *
 * Low level functions for packing data in fields and nodes.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (FINITE_ELEMENT_VALUES_STORAGE_HPP)
#define FINITE_ELEMENT_VALUES_STORAGE_HPP

#include <opencmiss/zinc/zincconfigure.h>
#include "finite_element/finite_element_time.h"
#include "general/value.h"

/*
Global Defines
--------------
*/

/* need following to handle 64-bit alignment problems of 64-bit quantities in
   64-bit version */
#if defined (O64)
#define VALUE_STORAGE_ARRAY_LENGTH_INT_SIZE 8
#define ADJUST_VALUE_STORAGE_SIZE( new_values_storage_size ) \
/* round size up to nearest 64-bit boundary */ \
if (new_values_storage_size % 8) \
{ \
	new_values_storage_size += (8 - (new_values_storage_size % 8)); \
}
#else /* defined (O64) */
#define VALUE_STORAGE_ARRAY_LENGTH_INT_SIZE sizeof(int)
#define ADJUST_VALUE_STORAGE_SIZE( new_values_storage_size )
   /* do nothing */
#endif /* defined (O64) */

/*
Global types
------------
*/

/*
Global functions
----------------
*/

int get_Value_storage_size(enum Value_type value_type,
	struct FE_time_sequence *time_sequence);
/*******************************************************************************
LAST MODIFIED : 20 November 2001

DESCRIPTION :
Given the value type, returns the size in bytes of the memory required to store
the following:
For non-array type, the actual data.
For array types, an integer storing the number of array values, and a pointer to
the array values.
for time depedant types, a pointer to the values.
==============================================================================*/

int free_value_storage_array(Value_storage *values_storage,
	enum Value_type value_type, struct FE_time_sequence *time_sequence,
	int number_of_values);
/*******************************************************************************
LAST MODIFIED: 20 November 2001

DESCRIPTION:
DEACCESSes objects and DEALLOCATEs dynamic storage in use by <values_storage>,
which is assumed to have <number_of_values> of the given <value_type> and
whether <time_sequence> is set.
Note that the values_storage array itself is not DEALLOCATED - up to the
calling function to do this.
Only certain value types, eg. arrays, strings, element_xi require this.
==============================================================================*/

int allocate_and_copy_values_storage_array(Value_storage *source,
	enum Value_type value_type, Value_storage *dest);
/************************************************************************
LAST MODIFIED : 8 June 1999

DESCRIPTION
Allocate an array of type value_type, of the length stored in source.
Copy the data from the array referenced by the pointer in source to the
alocated array. Copy the number of array values and the pointer to the
allocated array into dest.

Therefore must must free dest in calling function, and dest must be
an unallocated pointer when this function is called.

NOTE:
For array types, the contents of values_storage is:
 | int (number of array values) | pointer to array (eg double *) |
  x number_of_values
Assumes that sizeof(int) = 1 DWORD, so that the pointers are a DWORD aligned
in memory. If pointers weren't DWORD aligned get bus errors on SGIs.
=======================================================================*/

int copy_time_sequence_values_storage_array(Value_storage *source,
	enum Value_type value_type, struct FE_time_sequence *source_time_sequence,
	struct FE_time_sequence *destination_time_sequence, Value_storage *dest);
/************************************************************************
LAST MODIFIED : 21 November 2001

DESCRIPTION
Copy the data from the array referenced by the pointer in <source> to the
array referenced by the pointer in <dest>.

The destination must already have arrays allocated corresponding to the
<destination_time_sequence>.

NOTE:
For array types, the contents of values_storage is:
 | pointer to array (eg double *) |
  x number_of_values
Assumes that sizeof(int) = 1 DWORD, so that the pointers are a DWORD aligned
in memory. If pointers weren't DWORD aligned get bus errors on SGIs.
=======================================================================*/

int copy_time_sequence_values_storage_arrays(Value_storage *destination,
	enum Value_type value_type, struct FE_time_sequence *destination_time_sequence,
	struct FE_time_sequence *source_time_sequence, int number_of_values,
	Value_storage *source);
/*******************************************************************************
LAST MODIFIED : 1 November 2002

DESCRIPTION :
Calls copy_time_sequence_values_storage for all <number_of_values> items of
the appropriate value size to transfer time value arrays from <source> with
<source_time_sequence> to <destination> with <destination_time_sequence>.
==============================================================================*/

int allocate_time_values_storage_array(enum Value_type value_type,
	struct FE_time_sequence *destination_time_sequence, Value_storage *dest,
	int initialise_storage);
/************************************************************************
LAST MODIFIED : 21 November 2001

DESCRIPTION
Allocate an array of type value_type with size determined by the
destination_time_sequence.  If <initialise_storage> is true then the
values in the array are set to the zero.

NOTE:
For time types, the contents of values_storage is:
 | pointer to array (eg double *) |
  x number_of_values
Assumes that sizeof(int) = 1 DWORD, so that the pointers are a DWORD aligned
in memory. If pointers weren't DWORD aligned get bus errors on SGIs.
=======================================================================*/

int reallocate_time_values_storage_array(enum Value_type value_type,
	int new_number_of_values, Value_storage *new_array,
	Value_storage *previous_array,
	int initialise_storage, int previous_number_of_values);
/************************************************************************
LAST MODIFIED : 20 December 2005

DESCRIPTION
Reallocate an array of type value_type with number_of_values.  If
<initialise_storage> is true then the values from
<previous_number_of_values> + 1 to <new_number_of_values> are set to zero.
The routine will potentially overallocate the array to accelerate when
these arrays are expanded out one value at a time, many times over.
Warning: reallocates previous array and moves to new_array. Use only when
appending values to existing time array, when merging nodes from
temporary region.
=======================================================================*/

int copy_value_storage_array(Value_storage *destination,
	enum Value_type value_type, struct FE_time_sequence *destination_time_sequence,
	struct FE_time_sequence *source_time_sequence, int number_of_values,
	Value_storage *source, int optimised_merge);
/*******************************************************************************
LAST MODIFIED : 21 November 2001

DESCRIPTION :
Copies the <number_of_values> of <value_type> stored in <source> into
<destination>. Arrays, strings and other dynamic values are allocated afresh
for <destination>. <destination> is assumed to be blank and large enough to
contain such values.  If <source_time_sequence> and <destination_time_sequence>
are non NULL then the value storage correspond to an arrays of values at those
times.
If <optimised_merge> is set then time sequences may be transferred from source
to destination in certain cases instead of copied; only call from merge_FE_node.
==============================================================================*/

int initialise_value_storage_array(Value_storage *values_storage,
	enum Value_type value_type, struct FE_time_sequence *time_sequence,
	int number_of_values);
/*******************************************************************************
LAST MODIFIED : 6 May 2003

DESCRIPTION :
Clears values_storage array to suitable defaults for the <value_type> and
<time_sequence>.
<number_of_values> of the given <value_type>.
For non-array types, the contents of field->values_storage is:
   | data type (eg FE_value) | x number_of_values
For array types, the contents of field->values_storage is:
   ( | int (number of array values) | pointer to array (eg double *) |
   x number_of_values )
For time types where the <time_sequence> is nonNULL then then values_storage is:
   ( | pointer to array (eg double *) | x number_of_values )
Sets data in this memory to 0, pointers to NULL.
==============================================================================*/

Value_storage *make_value_storage_array(enum Value_type value_type,
	struct FE_time_sequence *time_sequence, int number_of_values);
/*******************************************************************************
LAST MODIFIED : 26 February 2003

DESCRIPTION :
Allocates space large enough to contain <number_of_values> of the given
<value_type>.
For non-array types, the contents of field->values_storage is:
   | data type (eg FE_value) | x number_of_values
For array types, the contents of field->values_storage is:
   ( | int (number of array values) | pointer to array (eg double *) |
   x number_of_values )
For time types where the <time_sequence> is nonNULL then then values_storage is:
   ( | pointer to array (eg double *) | x number_of_values )
Initialises the contents to be zero for values, NULL for pointers.
==============================================================================*/

#endif /* !defined (FINITE_ELEMENT_VALUES_STORAGE_HPP) */
