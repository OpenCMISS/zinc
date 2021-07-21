/**
 * FILE : finite_element_values_storage.cpp
 *
 * Low level functions for packing data in fields and nodes.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cstring>
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_value_storage.hpp"
#include "general/debug.h"
#include "general/message.h"

/*
Module Constants
----------------
*/

#define VALUE_STORAGE_NUMBER_OF_TIMES_BLOCK (30)

/*
Global functions
----------------
*/

int get_Value_storage_size(enum Value_type value_type,
	struct FE_time_sequence *time_sequence)
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
{
	int size;

	ENTER(get_Value_storage_size);
	if (time_sequence)
	{
		switch (value_type)
		{
			case DOUBLE_VALUE:
			{
				size = sizeof(double *);
			} break;
			case FE_VALUE_VALUE:
			{
				size = sizeof(FE_value *);
			} break;
			case FLT_VALUE:
			{
				size = sizeof(float *);
			} break;
			case SHORT_VALUE:
			{
				size = sizeof(short *);
			} break;
			case INT_VALUE:
			{
				size = sizeof(int *);
			} break;
			case UNSIGNED_VALUE:
			{
				size = sizeof(unsigned *);
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"get_Value_storage_size.  Not implemented time array value type.");
				size =0;
			} break;
		}
	}
	else
	{
		switch (value_type)
		{
			case DOUBLE_VALUE:
			{
				size = sizeof(double);
			} break;
			case ELEMENT_XI_VALUE:
			{
				/* need this to handle 64-bit alignment problems of 64-bit quantities in
					64-bit version */
#if defined (O64)
				size = (sizeof(cmzn_element *) + sizeof(FE_value) *
					MAXIMUM_ELEMENT_XI_DIMENSIONS) -
					((sizeof(cmzn_element *) + sizeof(FE_value) *
						MAXIMUM_ELEMENT_XI_DIMENSIONS)%8) + 8;
#else /* defined (O64) */
				size = sizeof(cmzn_element *) + sizeof(FE_value) *
					MAXIMUM_ELEMENT_XI_DIMENSIONS;
#endif /* defined (O64) */
			} break;
			case FE_VALUE_VALUE:
			{
				size = sizeof(FE_value);
			} break;
			case FLT_VALUE:
			{
				size = sizeof(float);
			} break;
			case SHORT_VALUE:
			{
				size = sizeof(short);
			} break;
			case INT_VALUE:
			{
				size = sizeof(int);
			} break;
			case UNSIGNED_VALUE:
			{
				size = sizeof(unsigned);
			} break;
			case DOUBLE_ARRAY_VALUE:
			{
				/* VALUE_STORAGE_ARRAY_LENGTH_INT_SIZE space for number of array values */
				/* (*double) to store pointer to data*/
				size = VALUE_STORAGE_ARRAY_LENGTH_INT_SIZE+sizeof(double *);
			} break;
			case FE_VALUE_ARRAY_VALUE:
			{
				size = VALUE_STORAGE_ARRAY_LENGTH_INT_SIZE+sizeof(FE_value *);
			} break;
			case FLT_ARRAY_VALUE:
			{
				size = VALUE_STORAGE_ARRAY_LENGTH_INT_SIZE+sizeof(float *);
			} break;
			case SHORT_ARRAY_VALUE:
			{
				size = VALUE_STORAGE_ARRAY_LENGTH_INT_SIZE+sizeof(short *);
			} break;
			case INT_ARRAY_VALUE:
			{
				size = VALUE_STORAGE_ARRAY_LENGTH_INT_SIZE+sizeof(int *);
			} break;
			case UNSIGNED_ARRAY_VALUE:
			{
				size = VALUE_STORAGE_ARRAY_LENGTH_INT_SIZE+sizeof(unsigned *);
			} break;
			case STRING_VALUE:
			{
				size = sizeof(char *);
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"get_Value_storage_size.  Unknown value_type");
				size =0;
			} break;
		}
	}
	LEAVE;

	return (size);
} /* get_Value_storage_size */

int free_value_storage_array(Value_storage *values_storage,
	enum Value_type value_type, struct FE_time_sequence *time_sequence,
	int number_of_values)
{
	Value_storage *the_values_storage;
	int i,return_code,size;

	ENTER(free_value_storage_array);
	if (values_storage&&(size=get_Value_storage_size(value_type, time_sequence))&&
		(0<=number_of_values))
	{
		return_code=1;
		the_values_storage = values_storage;
		if (time_sequence)
		{
			switch (value_type)
			{
				case DOUBLE_VALUE:
				{
					double **array_address;

					for (i=0;i<number_of_values;i++)
					{
						array_address = (double **)the_values_storage;
						DEALLOCATE(*array_address);
						the_values_storage += size;
					}
				} break;
				case FE_VALUE_VALUE:
				{
					FE_value **array_address;

					for (i=0;i<number_of_values;i++)
					{
						array_address = (FE_value **)the_values_storage;
						DEALLOCATE(*array_address);
						the_values_storage += size;
					}
				} break;
				case FLT_VALUE:
				{
					float **array_address;

					for (i=0;i<number_of_values;i++)
					{
						array_address = (float **)the_values_storage;
						DEALLOCATE(*array_address);
						the_values_storage += size;
					}
				} break;
				case INT_VALUE:
				{
					int **array_address;

					for (i=0;i<number_of_values;i++)
					{
						array_address = (int **)the_values_storage;
						DEALLOCATE(*array_address);
						the_values_storage += size;
					}
				} break;
				case UNSIGNED_VALUE:
				{
					unsigned **array_address;

					for (i=0;i<number_of_values;i++)
					{
						array_address = (unsigned **)the_values_storage;
						DEALLOCATE(*array_address);
						the_values_storage += size;
					}
				} break;
				default:
				{
					display_message(WARNING_MESSAGE,
						"free_value_storage_array.  Time array not cleaned up for value_type");
				} break;
			}
		}
		else
		{
			switch (value_type)
			{
				case DOUBLE_ARRAY_VALUE:
				{
					double **array_address;

					for (i=0;i<number_of_values;i++)
					{
						array_address = (double **)(the_values_storage+sizeof(int));
						DEALLOCATE(*array_address);
						the_values_storage += size;
					}
				} break;
				case ELEMENT_XI_VALUE:
				{
					cmzn_element **element_address;

					for (i=0;i<number_of_values;i++)
					{
						element_address = (cmzn_element **)the_values_storage;
						if (*element_address)
						{
							DEACCESS(FE_element)(element_address);
						}
						the_values_storage += size;
					}
				} break;
				case FE_VALUE_ARRAY_VALUE:
				{
					FE_value **array_address;

					for (i=0;i<number_of_values;i++)
					{
						array_address = (FE_value **)(the_values_storage+sizeof(int));
						DEALLOCATE(*array_address);
						the_values_storage += size;
					}
				} break;
				case FLT_ARRAY_VALUE:
				{
					float **array_address;

					for (i=0;i<number_of_values;i++)
					{
						array_address = (float **)(the_values_storage+sizeof(int));
						DEALLOCATE(*array_address);
						the_values_storage += size;
					}
				} break;
				case INT_ARRAY_VALUE:
				{
					int **array_address;

					for (i=0;i<number_of_values;i++)
					{
						array_address = (int **)(the_values_storage+sizeof(int));
						DEALLOCATE(*array_address);
						the_values_storage += size;
					}
				} break;
				case UNSIGNED_ARRAY_VALUE:
				{
					unsigned **array_address;

					for (i=0;i<number_of_values;i++)
					{
						array_address = (unsigned **)(the_values_storage+sizeof(int));
						DEALLOCATE(*array_address);
						the_values_storage += size;
					}
				} break;
				case STRING_VALUE:
				{
					char **array_address;

					for (i=0;i<number_of_values;i++)
					{
						array_address = (char **)(the_values_storage);
						DEALLOCATE(*array_address);
						the_values_storage += size;
					}
				} break;
				default:
				{
					// nothing to do for other types
				} break;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"free_value_storage_array. Invalid arguments");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* free_value_storage_array */

int allocate_and_copy_values_storage_array(Value_storage *source,
	enum Value_type value_type, Value_storage *dest)
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
{
	int number_of_array_values,array_size,return_code;


	ENTER(allocate_and_copy_values_storage_array);
	if (source)
	{
		return_code = 1;
		switch (value_type)
		{
			case DOUBLE_ARRAY_VALUE:
			{
				double *dest_array,*source_array,**array_address;
				/* get number of array values from source */
				number_of_array_values = *((int *)source);
				if (number_of_array_values) /* no array values, source array is NULL*/
				{
					/* get address of array from source */
					array_address = (double **)(source+sizeof(int));
					source_array = *array_address;
					array_size = (sizeof(double))*(number_of_array_values);
					/* allocate the dest array */
					if (ALLOCATE(dest_array,double,number_of_array_values))
					{
						/* copy values into the dest array */
						memcpy(dest_array,source_array,array_size);
						/* copy the number of array values into the dest values_storage */
						*((int *)dest) = number_of_array_values;
						/* copy the address of the new array into the dest values_storage*/
						array_address = (double **)(dest+sizeof(int));
						*array_address = dest_array;
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"allocate_and_copy_values_storage_array. Out of memory");
						return_code = 0;
					}
				}
				else
				{
					/* copy the number of array values = 0 into the dest values_storage */
					*((int *)dest) = 0;
					/* copy the  NULL address of the new array into the dest
						values_storage*/
					array_address = (double **)(dest+sizeof(int));
					*array_address = (double *)NULL;
				}
			} break;
			case FE_VALUE_ARRAY_VALUE:
			{
				FE_value *dest_array,*source_array,**array_address;
				/* get number of array values from source */
				number_of_array_values = *((int *)source);
				if (number_of_array_values) /* no array values, source array is NULL*/
				{
					/* get address of array from source */
					array_address = (FE_value **)(source+sizeof(int));
					source_array = *array_address;
					array_size = (sizeof(FE_value))*(number_of_array_values);
					/* allocate the dest array */
					if (ALLOCATE(dest_array,FE_value,number_of_array_values))
					{
						/* copy values into the dest array */
						memcpy(dest_array,source_array,array_size);
						/* copy the number of array values into the dest values_storage */
						*((int *)dest) = number_of_array_values;
						/* copy the address of the new array into the dest values_storage*/
						array_address = (FE_value **)(dest+sizeof(int));
						*array_address = dest_array;
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"allocate_and_copy_values_storage_array. Out of memory");
						return_code = 0;
					}
				}
				else
				{
					/* copy the number of array values = 0 into the dest values_storage */
					*((int *)dest) = 0;
					/* copy the  NULL address of the new array into the dest values_storage*/
					array_address = (FE_value **)(dest+sizeof(int));
					*array_address = (FE_value *)NULL;
				}
			} break;
			case FLT_ARRAY_VALUE:
			{
				float *dest_array,*source_array,**array_address;
				/* get number of array values from source */
				number_of_array_values = *((int *)source);
				if (number_of_array_values) /* no array values, source array is NULL*/
				{
					/* get address of array from source */
					array_address = (float **)(source+sizeof(int));
					source_array = *array_address;
					array_size = (sizeof(float))*(number_of_array_values);
					/* allocate the dest array */
					if (ALLOCATE(dest_array,float,number_of_array_values))
					{
						/* copy values into the dest array */
						memcpy(dest_array,source_array,array_size);
						/* copy the number of array values into the dest values_storage */
						*((int *)dest) = number_of_array_values;
						/* copy the address of the new array into the dest values_storage*/
						array_address = (float **)(dest+sizeof(int));
						*array_address = dest_array;
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"allocate_and_copy_values_storage_array. Out of memory");
						return_code = 0;
					}
				}
				else
				{
					/* copy the number of array values = 0 into the dest values_storage */
					*((int *)dest) = 0;
					/* copy the  NULL address of the new array into the dest values_storage*/
					array_address = (float **)(dest+sizeof(int));
					*array_address = (float *)NULL;
				}
			} break;
			case SHORT_ARRAY_VALUE:
			{
				short *dest_array,*source_array,**array_address;
				/* get number of array values from source */
				number_of_array_values = *((int *)source);
				if (number_of_array_values) /* no array values, source array is NULL*/
				{
					/* get address of array from source */
					array_address = (short **)(source+sizeof(int));
					source_array = *array_address;
					array_size = (sizeof(short))*(number_of_array_values);
					/* allocate the dest array */
					if (ALLOCATE(dest_array,short,number_of_array_values))
					{
						/* copy values into the dest array */
						memcpy(dest_array,source_array,array_size);
						/* copy the number of array values into the dest values_storage */
						*((int *)dest) = number_of_array_values;
						/* copy the address of the new array into the dest values_storage*/
						array_address = (short **)(dest+sizeof(int));
						*array_address = dest_array;
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"allocate_and_copy_values_storage_array. Out of memory");
						return_code = 0;
					}
				}
				else
				{
					/* copy the number of array values = 0 into the dest values_storage */
					*((int *)dest) = 0;
					/* copy the  NULL address of the new array into the dest values_storage*/
					array_address = (short **)(dest+sizeof(int));
					*array_address = (short *)NULL;
				}
			} break;
			case INT_ARRAY_VALUE:
			{
				int *dest_array,*source_array,**array_address;
				/* get number of array values from source */
				number_of_array_values = *((int *)source);
				if (number_of_array_values) /* no array values, source array is NULL*/
				{
					/* get address of array from source */
					array_address = (int **)(source+sizeof(int));
					source_array = *array_address;
					array_size = (sizeof(int))*(number_of_array_values);
					/* allocate the dest array */
					if (ALLOCATE(dest_array,int,number_of_array_values))
					{
						/* copy values into the dest array */
						memcpy(dest_array,source_array,array_size);
						/* copy the number of array values into the dest values_storage */
						*((int *)dest) = number_of_array_values;
						/* copy the address of the new array into the dest values_storage*/
						array_address = (int **)(dest+sizeof(int));
						*array_address = dest_array;
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"allocate_and_copy_values_storage_array. Out of memory");
						return_code = 0;
					}
				}
				else
				{
					/* copy the number of array values = 0 into the dest values_storage */
					*((int *)dest) = 0;
					/* copy the  NULL address of the new array into the dest values_storage*/
					array_address = (int **)(dest+sizeof(int));
					*array_address = (int *)NULL;
				}
			} break;
			case UNSIGNED_ARRAY_VALUE:
			{
				unsigned *dest_array,*source_array,**array_address;
				/* get number of array values from source */
				number_of_array_values = *((int *)source);
				if (number_of_array_values) /* no array values, source array is NULL*/
				{
					/* get address of array from source */
					array_address = (unsigned **)(source+sizeof(int));
					source_array = *array_address;
					array_size = (sizeof(unsigned))*(number_of_array_values);
					/* allocate the dest array */
					if (ALLOCATE(dest_array,unsigned,number_of_array_values))
					{
						/* copy values into the dest array */
						memcpy(dest_array,source_array,array_size);
						/* copy the number of array values into the dest values_storage */
						*((int *)dest) = number_of_array_values;
						/* copy the address of the new array into the dest values_storage*/
						array_address = (unsigned **)(dest+sizeof(int));
						*array_address = dest_array;
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"allocate_and_copy_values_storage_array. Out of memory");
						return_code = 0;
					}
				}
				else
				{
					/* copy the number of array values = 0 into the dest values_storage */
					*((int *)dest) = 0;
					/* copy the  NULL address of the new array into the dest values_storage*/
					array_address = (unsigned **)(dest+sizeof(int));
					*array_address = (unsigned *)NULL;
				}
			} break;
			case STRING_VALUE:
			{
				char *dest_array,*source_array,**array_address;
				/* get address of array from source */
				array_address = (char **)(source);
				if (*array_address)/* if we have a source array*/
				{
					source_array = *array_address;
					array_size = static_cast<int>(strlen(source_array)) + 1; /* +1 for null termination */
					/* allocate the dest array */
					if (ALLOCATE(dest_array,char,array_size))
					{
						/* copy values into the dest array */
						memcpy(dest_array,source_array,array_size);
						/* copy the address of the new array into the dest values_storage*/
						array_address = (char **)(dest);
						*array_address = dest_array;
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"allocate_and_copy_values_storage_array. Out of memory");
						return_code = 0;
					}
				}
				else	/* copy NULL into the dest values_storage*/
				{
					array_address = (char **)(dest);
					*array_address = (char *)NULL;
				}
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"allocate_and_copy_values_storage_array. Invalid type");
				return_code = 0;
			} break;
		} /*switch (the_value_type) */
	}
	else
	{
		display_message(ERROR_MESSAGE,"allocate_and_copy_values_storage_array."
			"Invalid arguments");
		return_code = 0;
	}
	LEAVE;
	return (return_code);
} /* allocate_and_copy_values_storage_array */

int copy_time_sequence_values_storage_array(Value_storage *source,
	enum Value_type value_type, struct FE_time_sequence *source_time_sequence,
	struct FE_time_sequence *destination_time_sequence, Value_storage *dest)
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
{
	int destination_time_index, source_number_of_times,
		source_time_index, return_code;
	FE_value time;

	ENTER(copy_time_sequence_values_storage_array);
	if (source)
	{
		return_code = 1;
		source_number_of_times = FE_time_sequence_get_number_of_times(
			source_time_sequence);
		/* Copy the values into the correct places */
		for (source_time_index = 0 ; source_time_index < source_number_of_times
			 ; source_time_index++)
		{
			if (FE_time_sequence_get_time_for_index(source_time_sequence,
					 source_time_index, &time) &&
				FE_time_sequence_get_index_for_time(destination_time_sequence,
					time, &destination_time_index))
			{
				switch (value_type)
				{
					case DOUBLE_VALUE:
					{
						double *dest_array,*source_array,**array_address;
						/* get address of array from source */
						array_address = (double **)source;
						source_array = *array_address;
						array_address = (double **)dest;
						dest_array = *array_address;
						dest_array[destination_time_index] =
							source_array[source_time_index];
					} break;
					case FE_VALUE_VALUE:
					{
						FE_value *dest_array,*source_array,**array_address;
						/* get address of array from source */
						array_address = (FE_value **)source;
						source_array = *array_address;
						array_address = (FE_value **)dest;
						dest_array = *array_address;
						dest_array[destination_time_index] =
							source_array[source_time_index];
					} break;
					case FLT_VALUE:
					{
						float *dest_array,*source_array,**array_address;
						/* get address of array from source */
						array_address = (float **)source;
						source_array = *array_address;
						array_address = (float **)dest;
						dest_array = *array_address;
						dest_array[destination_time_index] =
							source_array[source_time_index];
					} break;
					case SHORT_VALUE:
					{
						short *dest_array,*source_array,**array_address;
						/* get address of array from source */
						array_address = (short **)source;
						source_array = *array_address;
						array_address = (short **)dest;
						dest_array = *array_address;
						dest_array[destination_time_index] =
							source_array[source_time_index];
					} break;
					case INT_VALUE:
					{
						int *dest_array,*source_array,**array_address;
						/* get address of array from source */
						array_address = (int **)source;
						source_array = *array_address;
						array_address = (int **)dest;
						dest_array = *array_address;
						dest_array[destination_time_index] =
							source_array[source_time_index];
					} break;
					case UNSIGNED_ARRAY_VALUE:
					{
						unsigned *dest_array,*source_array,**array_address;
						/* get address of array from source */
						array_address = (unsigned **)source;
						source_array = *array_address;
						array_address = (unsigned **)dest;
						dest_array = *array_address;
						dest_array[destination_time_index] =
							source_array[source_time_index];
					} break;
					case STRING_VALUE:
					{
						display_message(ERROR_MESSAGE,
							"copy_time_sequence_values_storage_array.  "
							"String type not implemented for multiple times yet.");
						return_code = 0;
					} break;
					default:
					{
						display_message(ERROR_MESSAGE,
							"copy_time_sequence_values_storage_array.  Invalid type");
						return_code = 0;
					} break;
				} /*switch (the_value_type) */

			}
			else
			{
				display_message(ERROR_MESSAGE,"copy_time_sequence_values_storage_array.  "
					"Unable to find destination space for source time index %d",
					source_time_index);
				return_code = 0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"copy_time_sequence_values_storage_array."
			"Invalid arguments");
		return_code = 0;
	}
	LEAVE;
	return (return_code);
} /* copy_time_sequence_values_storage_array */

int copy_time_sequence_values_storage_arrays(Value_storage *destination,
	enum Value_type value_type, struct FE_time_sequence *destination_time_sequence,
	struct FE_time_sequence *source_time_sequence, int number_of_values,
	Value_storage *source)
/*******************************************************************************
LAST MODIFIED : 1 November 2002

DESCRIPTION :
Calls copy_time_sequence_values_storage for all <number_of_values> items of
the appropriate value size to transfer time value arrays from <source> with
<source_time_sequence> to <destination> with <destination_time_sequence>.
==============================================================================*/
{
	int i, return_code, value_size;
	Value_storage *src,*dest;

	ENTER(copy_time_sequence_values_storage_arrays);
	if (destination && destination_time_sequence && source_time_sequence &&
		(0 < number_of_values) && source)
	{
		return_code = 1;
		dest = destination;
		src = source;
		value_size = get_Value_storage_size(value_type, destination_time_sequence);
		for (i = 0; (i < number_of_values) && return_code; i++)
		{
			if (!copy_time_sequence_values_storage_array(src, value_type,
				source_time_sequence, destination_time_sequence, dest))
			{
				display_message(ERROR_MESSAGE,
					"copy_time_sequence_values_storage_arrays.  Failed to copy array");
				return_code = 0;
			}
			dest += value_size;
			src += value_size;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"copy_time_sequence_values_storage_arrays.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* copy_time_sequence_values_storage_arrays */

int allocate_time_values_storage_array(enum Value_type value_type,
	struct FE_time_sequence *destination_time_sequence, Value_storage *dest,
	int initialise_storage)
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
{
	int number_of_times,j,return_code;

	ENTER(allocate_time_values_storage_array);
	if (dest)
	{
		return_code = 1;
		number_of_times = FE_time_sequence_get_number_of_times(
			destination_time_sequence);
		/* Allocate the array */
		switch (value_type)
		{
			case DOUBLE_VALUE:
			{
				double *dest_array,**array_address;
				/* allocate the dest array */
				if (ALLOCATE(dest_array,double,number_of_times))
				{
					if (initialise_storage)
					{
						for (j = 0 ; j < number_of_times ; j++)
						{
							dest_array[j] = 0.0;
						}
					}
					/* copy the address of the new array into the dest values_storage*/
					array_address = (double **)dest;
					*array_address = dest_array;
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"allocate_time_values_storage_array. Out of memory");
					return_code = 0;
				}
			} break;
			case FE_VALUE_VALUE:
			{
				FE_value *dest_array,**array_address;
				/* allocate the dest array */
				if (ALLOCATE(dest_array,FE_value,number_of_times))
				{
					if (initialise_storage)
					{
						for (j = 0 ; j < number_of_times ; j++)
						{
							dest_array[j] = FE_VALUE_INITIALIZER;
						}
					}
					/* copy the address of the new array into the dest values_storage*/
					array_address = (FE_value **)dest;
					*array_address = dest_array;
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"allocate_time_values_storage_array. Out of memory");
					return_code = 0;
				}
			} break;
			case FLT_VALUE:
			{
				float *dest_array,**array_address;
				/* allocate the dest array */
				if (ALLOCATE(dest_array,float,number_of_times))
				{
					if (initialise_storage)
					{
						for (j = 0 ; j < number_of_times ; j++)
						{
							dest_array[j] = 0.0f;
						}
					}
					/* copy the address of the new array into the dest values_storage*/
					array_address = (float **)dest;
					*array_address = dest_array;
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"allocate_time_values_storage_array. Out of memory");
					return_code = 0;
				}
			} break;
			case SHORT_VALUE:
			{
				short *dest_array,**array_address;
				/* allocate the dest array */
				if (ALLOCATE(dest_array,short,number_of_times))
				{
					if (initialise_storage)
					{
						for (j = 0 ; j < number_of_times ; j++)
						{
							dest_array[j] = 0;
						}
					}
					/* copy the address of the new array into the dest values_storage*/
					array_address = (short **)dest;
					*array_address = dest_array;
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"allocate_time_values_storage_array. Out of memory");
					return_code = 0;
				}
			} break;
			case INT_VALUE:
			{
				int *dest_array,**array_address;
				/* allocate the dest array */
				if (ALLOCATE(dest_array,int,number_of_times))
				{
					if (initialise_storage)
					{
						for (j = 0 ; j < number_of_times ; j++)
						{
							dest_array[j] = 0;
						}
					}
					/* copy the address of the new array into the dest values_storage*/
					array_address = (int **)dest;
					*array_address = dest_array;
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"allocate_time_values_storage_array. Out of memory");
					return_code = 0;
				}
			} break;
			case STRING_VALUE:
			{
				display_message(ERROR_MESSAGE,
					"allocate_time_values_storage_array. String type not implemented for multiple times yet.");
				return_code = 0;
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"allocate_time_values_storage_array. Invalid type");
				return_code = 0;
			} break;
		} /*switch (the_value_type) */
	}
	else
	{
		display_message(ERROR_MESSAGE,"allocate_time_values_storage_array."
			"Invalid arguments");
		return_code = 0;
	}
	LEAVE;
	return (return_code);
} /* allocate_time_values_storage_array */

int reallocate_time_values_storage_array(enum Value_type value_type,
	int new_number_of_values, Value_storage *new_array,
	Value_storage *previous_array,
	int initialise_storage, int previous_number_of_values)
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
{
	int allocate_number_of_values, j, return_code;

	ENTER(reallocate_time_values_storage_array);
	if (new_array && previous_array)
	{
		return_code = 1;

		allocate_number_of_values = (new_number_of_values + VALUE_STORAGE_NUMBER_OF_TIMES_BLOCK) -
			(new_number_of_values % VALUE_STORAGE_NUMBER_OF_TIMES_BLOCK);
		switch (value_type)
		{
			case DOUBLE_VALUE:
			{
				double *array;
				if (REALLOCATE(array,*(double **)previous_array,double,allocate_number_of_values))
				{
					if (initialise_storage)
					{
						for (j = previous_number_of_values ; j < new_number_of_values ; j++)
						{
							array[j] = 0.0;
						}
					}
					*(double **)new_array = array;
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"reallocate_time_values_storage_array. Out of memory");
					return_code = 0;
				}
			} break;
			case FE_VALUE_VALUE:
			{
				FE_value *array;
				if (REALLOCATE(array,*(FE_value **)previous_array,FE_value,allocate_number_of_values))
				{
					if (initialise_storage)
					{
						for (j = previous_number_of_values ; j < new_number_of_values ; j++)
						{
							array[j] = FE_VALUE_INITIALIZER;
						}
					}
					*(FE_value **)new_array = array;
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"reallocate_time_values_storage_array. Out of memory");
					return_code = 0;
				}
			} break;
			case FLT_VALUE:
			{
				float *array;
				if (REALLOCATE(array,*(float **)previous_array,float,allocate_number_of_values))
				{
					if (initialise_storage)
					{
						for (j = previous_number_of_values ; j < new_number_of_values ; j++)
						{
							array[j] = 0.0f;
						}
					}
					*(float **)new_array = array;
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"reallocate_time_values_storage_array. Out of memory");
					return_code = 0;
				}
			} break;
			case SHORT_VALUE:
			{
				short *array;
				if (REALLOCATE(array,*(short **)previous_array,short,allocate_number_of_values))
				{
					if (initialise_storage)
					{
						for (j = previous_number_of_values ; j < new_number_of_values ; j++)
						{
							array[j] = 0;
						}
					}
					*(short **)new_array = array;
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"reallocate_time_values_storage_array. Out of memory");
					return_code = 0;
				}
			} break;
			case INT_VALUE:
			{
				int *array;
				if (REALLOCATE(array,*(int **)previous_array,int,allocate_number_of_values))
				{
					if (initialise_storage)
					{
						for (j = previous_number_of_values ; j < new_number_of_values ; j++)
						{
							array[j] = 0;
						}
					}
					*(int **)new_array = array;
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"reallocate_time_values_storage_array. Out of memory");
					return_code = 0;
				}
			} break;
			case STRING_VALUE:
			{
				display_message(ERROR_MESSAGE,
					"reallocate_time_values_storage_array. String type not implemented for multiple times yet.");
				return_code = 0;
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"reallocate_time_values_storage_array. Invalid type");
				return_code = 0;
			} break;
		} /*switch (the_value_type) */
	}
	else
	{
		display_message(ERROR_MESSAGE,"reallocate_time_values_storage_array."
			"Invalid arguments");
		return_code = 0;
	}
	return (return_code);
}

int copy_value_storage_array(Value_storage *destination,
	enum Value_type value_type, struct FE_time_sequence *destination_time_sequence,
	struct FE_time_sequence *source_time_sequence,int number_of_values,
	Value_storage *source, int optimised_merge)
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
{
	enum FE_time_sequence_mapping time_sequence_mapping;
	int destination_number_of_times, i, return_code;

	ENTER(copy_value_storage_array);
	if (destination&&(0<number_of_values)&&source)
	{
		return_code=1;
		if (source_time_sequence || destination_time_sequence)
		{
			if (source_time_sequence && destination_time_sequence)
			{
				int value_size;
				Value_storage *src,*dest;

				dest = destination;
				src = source;
				value_size=get_Value_storage_size(value_type,
					destination_time_sequence);
				if (optimised_merge)
				{
					time_sequence_mapping =
						FE_time_sequences_mapping(source_time_sequence, destination_time_sequence);
				}
				else
				{
					time_sequence_mapping = FE_TIME_SEQUENCE_MAPPING_UNKNOWN;
				}
				switch (time_sequence_mapping)
				{
					case FE_TIME_SEQUENCE_MAPPING_IDENTICAL:
					case FE_TIME_SEQUENCE_MAPPING_APPEND:
					{
						destination_number_of_times = FE_time_sequence_get_number_of_times(destination_time_sequence);
						for (i=0;(i<number_of_values)&&return_code;i++)
						{
							reallocate_time_values_storage_array(value_type,
								destination_number_of_times, dest, src,
								/*initialise_storage*/0, /*previous_number_of_values*/0);
							*(void **)src = 0x0;
							dest += value_size;
							src += value_size;
						}
					} break;
					default:
					{
						/* Fallback default implementation */
						for (i=0;(i<number_of_values)&&return_code;i++)
						{
							if (!(allocate_time_values_storage_array(value_type,
										destination_time_sequence,dest,/*initialise_storage*/0)&&
									copy_time_sequence_values_storage_array(src,value_type,
										source_time_sequence,destination_time_sequence,dest)))
							{
								display_message(ERROR_MESSAGE,
									"copy_value_storage_array.  Failed to copy array");
								if (0<i)
								{
									/* free any arrays allocated to date */
									free_value_storage_array(destination,value_type,
										destination_time_sequence,i);
								}
									return_code = 0;
							}
							dest += value_size;
							src += value_size;
						}
					} break;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"copy_value_storage_array.  Copying time values to or from "
					"non time based values not implemented");
			}
		}
		else
		{
			switch (value_type)
			{
				case DOUBLE_VALUE:
				{
					double *src,*dest;

					src = (double *)source;
					dest = (double *)destination;
					for (i=0;i<number_of_values;i++)
					{
						*dest = *src;
						dest++;
						src++;
					}
				} break;
				case ELEMENT_XI_VALUE:
				{
					int i,j;
					Value_storage *src,*dest;

					src = source;
					dest = destination;
					for (i=0;i<number_of_values;i++)
					{
						/* copy accessed element pointer */
						if (*((cmzn_element **)src))
						{
							(*(cmzn_element **)dest)
								= ACCESS(FE_element)(*((cmzn_element **)src));
						}
						else
						{
							(*(cmzn_element **)dest) = (cmzn_element *)NULL;
						}
						dest += sizeof(cmzn_element *);
						src += sizeof(cmzn_element *);
						/* copy the xi location */
						for (j=0;j<MAXIMUM_ELEMENT_XI_DIMENSIONS;j++)
						{
							*((FE_value *)dest) = *((FE_value *)src);
							dest += sizeof(FE_value);
							src += sizeof(FE_value);
						}
					}
				} break;
				case FE_VALUE_VALUE:
				{
					FE_value *src,*dest;

					src = (FE_value *)source;
					dest = (FE_value *)destination;
					for (i=0;i<number_of_values;i++)
					{
						*dest = *src;
						dest++;
						src++;
					}
				} break;
				case FLT_VALUE:
				{
					float *src,*dest;

					src = (float *)source;
					dest = (float *)destination;
					for (i=0;i<number_of_values;i++)
					{
						*dest = *src;
						dest++;
						src++;
					}
				} break;
				case SHORT_VALUE:
				{
					display_message(ERROR_MESSAGE,"copy_value_storage_array.  "
						"SHORT_VALUE: Haven't written code yet. Beware pointer alignment problems!");
					return_code = 0;
				} break;
				case INT_VALUE:
				{
					int *src,*dest;

					src = (int *)source;
					dest = (int *)destination;
					for (i=0;i<number_of_values;i++)
					{
						*dest = *src;
						dest++;
						src++;
					}
				} break;
				case UNSIGNED_VALUE:
				{
					unsigned *src,*dest;

					src = (unsigned *)source;
					dest = (unsigned *)destination;
					for (i=0;i<number_of_values;i++)
					{
						*dest = *src;
						dest++;
						src++;
					}
				} break;
				case DOUBLE_ARRAY_VALUE:
				case FE_VALUE_ARRAY_VALUE:
				case FLT_ARRAY_VALUE:
				case SHORT_ARRAY_VALUE:
				case INT_ARRAY_VALUE:
				case UNSIGNED_ARRAY_VALUE:
				case STRING_VALUE:
				{
					int value_size;
					Value_storage *src,*dest;

					dest = destination;
					src = source;
					value_size=get_Value_storage_size(value_type,
						(struct FE_time_sequence *)NULL);
					for (i=0;(i<number_of_values)&&return_code;i++)
					{
						if (!allocate_and_copy_values_storage_array(src,value_type,dest))
						{
							display_message(ERROR_MESSAGE,
								"copy_value_storage_array.  Failed to copy array");
							if (0<i)
							{
								/* free any arrays allocated to date */
								free_value_storage_array(destination,value_type,
									(struct FE_time_sequence *)NULL,i);
							}
							return_code = 0;
						}
						dest += value_size;
						src += value_size;
					}
				} break;
				default:
				{
					display_message(ERROR_MESSAGE,
						"copy_value_storage_array.  Unknown value_type");
					return_code = 0;
				} break;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"copy_value_storage_array.  Invalid arguments");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* copy_value_storage_array */

int initialise_value_storage_array(Value_storage *values_storage,
	enum Value_type value_type, struct FE_time_sequence *time_sequence,
	int number_of_values)
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
{
	int i, j, return_code, size;
	Value_storage *new_value, *temp_values_storage;

	ENTER(initialise_value_storage_array);
	if (values_storage &&
		(size = get_Value_storage_size(value_type, time_sequence)) &&
		(0 < number_of_values))
	{
		return_code = 1;
		temp_values_storage = values_storage;
		if (time_sequence)
		{
			/* set pointers to NULL */
			switch (value_type)
			{
				case DOUBLE_VALUE:
				{
					for (i = number_of_values; 0 < i; i--)
					{
						*((double **)temp_values_storage) = (double *)NULL;
						temp_values_storage += size;
					}
				}	break;
				case FE_VALUE_VALUE:
				{
					for (i = number_of_values; 0 < i; i--)
					{
						*((FE_value **)temp_values_storage) = (FE_value *)NULL;
						temp_values_storage += size;
					}
				}	break;
				case FLT_VALUE:
				{
					for (i = number_of_values; 0 < i; i--)
					{
						*((float **)temp_values_storage) = (float *)NULL;
						temp_values_storage += size;
					}
				} break;
				case SHORT_VALUE:
				{
					for (i = number_of_values; 0 < i; i--)
					{
						*((short **)temp_values_storage) = (short *)NULL;
						temp_values_storage += size;
					}
				} break;
				case INT_VALUE:
				{
					for (i = number_of_values; 0 < i; i--)
					{
						*((int **)temp_values_storage) = (int *)NULL;
						temp_values_storage += size;
					}
				}	break;
				case UNSIGNED_VALUE:
				{
					for (i = number_of_values; 0 < i; i--)
					{
						*((unsigned **)temp_values_storage) = (unsigned *)NULL;
						temp_values_storage += size;
					}
				}	break;
				default:
				{
					display_message(ERROR_MESSAGE, "initialise_value_storage_array.  "
						"Value type does not support time_sequences");
					return_code = 0;
				} break;
			}
		}
		else
		{
			/* set values to zero */
			switch (value_type)
			{
				case DOUBLE_VALUE:
				{
					for (i = number_of_values; 0 < i; i--)
					{
						*((double *)temp_values_storage) = 0.0;
						temp_values_storage += size;
					}
				}	break;
				case ELEMENT_XI_VALUE:
				{
					for (i = number_of_values; 0 < i; i--)
					{
						new_value = temp_values_storage;
						*((cmzn_element **)new_value) = (cmzn_element *)NULL;
						new_value += sizeof(cmzn_element *);
						for (j = 0; j < MAXIMUM_ELEMENT_XI_DIMENSIONS; j++)
						{
							*((FE_value *)new_value) = 0;
							new_value += sizeof(FE_value);
						}
						temp_values_storage += size;
					}
				}	break;
				case FE_VALUE_VALUE:
				{
					for (i = number_of_values; 0 < i; i--)
					{
						*((FE_value *)temp_values_storage) = 0.0;
						temp_values_storage += size;
					}
				}	break;
				case FLT_VALUE:
				{
					for (i = number_of_values; 0 < i; i--)
					{
						*((float *)temp_values_storage) = 0.0;
						temp_values_storage += size;
					}
				} break;
				case SHORT_VALUE:
				{
					display_message(ERROR_MESSAGE," initialise_value_storage_array."
						"SHORT_VALUE. Code not written yet. Beware alignment problems ");
					return_code = 0;
				} break;
				case INT_VALUE:
				{
					for (i = number_of_values; 0 < i; i--)
					{
						*((int *)temp_values_storage) = 0;
						temp_values_storage += size;
					}
				}	break;
				case UNSIGNED_VALUE:
				{
					for (i = number_of_values; 0 < i; i--)
					{
						*((unsigned *)temp_values_storage) = 0;
						temp_values_storage += size;
					}
				}	break;
				case STRING_VALUE:
				{
					for (i = number_of_values; 0 < i; i--)
					{
						*((char **)temp_values_storage) = (char *)NULL;
						temp_values_storage += size;
					}
				} break;
				/* set number of array values to 0, array pointers to NULL */
				case DOUBLE_ARRAY_VALUE:
				{
					double **array_address;

					for (i = number_of_values; 0 < i; i--)
					{
						/* copy the number of array values (0!) to temp_values_storage */
						*((int *)temp_values_storage) = 0;
						/* copy the pointer to the array values (currently NULL), to
							 temp_values_storage*/
						array_address = (double **)(temp_values_storage + sizeof(int));
						*array_address = (double *)NULL;
						temp_values_storage += size;
					}
				} break;
				case FE_VALUE_ARRAY_VALUE:
				{
					FE_value **array_address;

					for (i = number_of_values; 0 < i; i--)
					{
						*((int *)temp_values_storage) = 0;
						array_address = (FE_value **)(temp_values_storage + sizeof(int));
						*array_address = (FE_value *)NULL;
						temp_values_storage += size;
					}
				} break;
				case FLT_ARRAY_VALUE:
				{
					float **array_address;

					for (i = number_of_values; 0 < i; i--)
					{
						*((int *)temp_values_storage) = 0;
						array_address = (float **)(temp_values_storage + sizeof(int));
						*array_address = (float *)NULL;
						temp_values_storage += size;
					}
				} break;
				case SHORT_ARRAY_VALUE:
				{
					short **array_address;

					for (i = number_of_values; 0 < i; i--)
					{
						*((int *)temp_values_storage) = 0;
						array_address = (short **)(temp_values_storage + sizeof(int));
						*array_address = (short *)NULL;
						temp_values_storage += size;
					}
				} break;
				case INT_ARRAY_VALUE:
				{
					int **array_address;

					for (i = number_of_values; 0 < i; i--)
					{
						*((int *)temp_values_storage) = 0;
						array_address = (int **)(temp_values_storage + sizeof(int));
						*array_address = (int *)NULL;
						temp_values_storage += size;
					}
				} break;
				case UNSIGNED_ARRAY_VALUE:
				{
					unsigned **array_address;

					for (i = number_of_values; 0 < i; i--)
					{
						*((int *)temp_values_storage) = 0;
						array_address = (unsigned **)(temp_values_storage + sizeof(int));
						*array_address = (unsigned *)NULL;
						temp_values_storage += size;
					}
				} break;
				default:
				{
					display_message(ERROR_MESSAGE,
						"initialise_value_storage_array.  Unknown value_type");
					return_code = 0;
				} break;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"initialise_value_storage_array.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* initialise_value_storage_array */

Value_storage *make_value_storage_array(enum Value_type value_type,
	struct FE_time_sequence *time_sequence, int number_of_values)
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
{
	int size, values_storage_size;
	Value_storage *values_storage;

	ENTER(make_value_storage_array);
	values_storage = (Value_storage *)NULL;
	if ((size = get_Value_storage_size(value_type ,time_sequence)) &&
		(0 < number_of_values))
	{
		values_storage_size = size*number_of_values;
		ADJUST_VALUE_STORAGE_SIZE(values_storage_size);
		if (ALLOCATE(values_storage, Value_storage, values_storage_size))
		{
			if (!initialise_value_storage_array(values_storage, value_type,
				time_sequence, number_of_values))
			{
				DEALLOCATE(values_storage);
				values_storage = (Value_storage *)NULL;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"make_value_storage_array.  Not enough memory");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"make_value_storage_array.  Invalid argument(s)");
	}
	LEAVE;

	return (values_storage);
} /* make_value_storage_array */
