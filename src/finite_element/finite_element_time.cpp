/*******************************************************************************
FILE : finite_element_time.c

LAST MODIFIED : 27 November 2002

DESCRIPTION :
Representing time in finite elements.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <math.h>

#include "opencmiss/zinc/timesequence.h"
#include "opencmiss/zinc/status.h"
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_time.h"
#include "general/debug.h"
#include "general/indexed_list_private.h"
#include "general/list_private.h"
#include "general/manager_private.h"
#include "general/message.h"

/*
Module types
------------
*/

struct FE_time_sequence_package
/*******************************************************************************
LAST MODIFIED : 9 November 2001

DESCRIPTION :
==============================================================================*/
{
	struct MANAGER(FE_time_sequence) *fe_time_sequence_manager;

	int access_count;
}; /* struct FE_time_sequence_package */

struct FE_time_sequence
/*******************************************************************************
LAST MODIFIED : 9 November 2001

DESCRIPTION :
==============================================================================*/
{
	enum FE_time_sequence_type type;

	/* For FE_TIME_SEQUENCE */
	int number_of_times;
	FE_value *times;

	/* A pointer to itself so that we can make the INDEX functions work with
		multiple parts of the object as the identifier */
	struct FE_time_sequence *self;

	/* after clearing in create, following to be modified only by manager */
	struct MANAGER(FE_time_sequence) *manager;
	int manager_change_status;

	int access_count;
}; /* struct FE_time_sequence */

FULL_DECLARE_INDEXED_LIST_TYPE(FE_time_sequence);

FULL_DECLARE_MANAGER_TYPE(FE_time_sequence);

/*
Module functions
----------------
*/
DECLARE_OBJECT_FUNCTIONS(FE_time_sequence_package)

DECLARE_INDEXED_LIST_MODULE_FUNCTIONS(FE_time_sequence,self,struct FE_time_sequence *,
	compare_FE_time_sequence)

DECLARE_LOCAL_MANAGER_FUNCTIONS(FE_time_sequence)

/*
Global functions
----------------
*/

int compare_FE_time_sequence(struct FE_time_sequence *fe_time_sequence_1,
	struct FE_time_sequence *fe_time_sequence_2)
/*******************************************************************************
LAST MODIFIED : 17 November 2004

DESCRIPTION :
Returns -1 if fe_time_sequence_1 < fe_time_sequence_2,
0 if fe_time_sequence_1 = fe_time_sequence_2 and 1 if
fe_time_sequence_1 > fe_time_sequence_2.
==============================================================================*/
{
	int return_code;

	ENTER(compare_FE_time_sequence);
	if (fe_time_sequence_1 && fe_time_sequence_2)
	{
		if (fe_time_sequence_1->type == fe_time_sequence_2->type)
		{
			switch(fe_time_sequence_1->type)
			{
				case FE_TIME_SEQUENCE:
				{
					if (fe_time_sequence_1->number_of_times ==
						fe_time_sequence_2->number_of_times)
					{
						/* Using a char based memcmp for speed.  I think that
							it is OK to misrepresent these FE_values as chars
							as so long as we have a consistent order it is OK,
							if we wanted different representations of the same
							number to match we would have to compare FE_values
							instead. */
						return_code = memcmp(fe_time_sequence_1->times,
							fe_time_sequence_2->times, fe_time_sequence_1->number_of_times
							* sizeof(FE_value));
						if (return_code != 0)
						{
							/* Convert to -1 or 1 in case it is something else */
							if (return_code > 0)
							{
								return_code = 1;
							}
							else
							{
								return_code = -1;
							}
						}
					}
					else
					{
						if (fe_time_sequence_1->number_of_times >
							fe_time_sequence_2->number_of_times)
						{
							return_code = 1;
						}
						else
						{
							return_code = -1;
						}
					}
				} break;
				default:
				{
					display_message(ERROR_MESSAGE,
						"compare_FE_time_sequence.  Unimplemented FE_time_sequence type");
					return_code = 0;
				} break;
			}
		}
		else
		{
			if (fe_time_sequence_1->type > fe_time_sequence_2->type)
			{
				return_code = 1;
			}
			else
			{
				return_code = -1;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"compare_FE_time_sequence.  Invalid arguments");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* compare_FE_time_sequence */

struct FE_time_sequence_package *CREATE(FE_time_sequence_package)(void)
/*******************************************************************************
LAST MODIFIED : 9 November 2001

DESCRIPTION :
Creates FE_time_sequence_package.
==============================================================================*/
{
	struct FE_time_sequence_package *fe_time;

	ENTER(CREATE(FE_time_sequence_package));

	if (ALLOCATE(fe_time, struct FE_time_sequence_package,1) &&
		(fe_time->fe_time_sequence_manager = CREATE(MANAGER(FE_time_sequence))()))
	{
		fe_time->access_count=0;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CREATE(FE_time_sequence_package).  Not enough memory");
		DEALLOCATE(fe_time);
	}
	LEAVE;

	return (fe_time);
} /* CREATE(FE_time_sequence_package) */

int DESTROY(FE_time_sequence_package)(struct FE_time_sequence_package **fe_time_address)
/*******************************************************************************
LAST MODIFIED : 9 November 2001

DESCRIPTION :
Frees memory/deaccess objects in FE_time_sequence_package at <*fe_time_address>.
==============================================================================*/
{
	int return_code;
	struct FE_time_sequence_package *fe_time;

	ENTER(DESTROY(FE_time_sequence_package));
	if (fe_time_address&&(fe_time= *fe_time_address))
	{
		if (0 >= fe_time->access_count)
		{
			DESTROY(MANAGER(FE_time_sequence))(&(fe_time->fe_time_sequence_manager));
			DEALLOCATE(*fe_time_address);
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"DESTROY(FE_time_sequence_package).  Positive access_count");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(FE_time_sequence_package).  Missing fe_time");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(FE_time_sequence_package) */

struct FE_time_sequence *CREATE(FE_time_sequence)(void)
/*******************************************************************************
LAST MODIFIED : 9 November 2001

DESCRIPTION :
Creates a basic FE_time_sequence.
==============================================================================*/
{
	struct FE_time_sequence *fe_time_sequence;

	ENTER(CREATE(FE_time_sequence));
	if (ALLOCATE(fe_time_sequence,struct FE_time_sequence,1))
	{
		/* initialise all members of computed_fe_time_sequence */
		fe_time_sequence->type = FE_TIME_SEQUENCE;
		fe_time_sequence->number_of_times = 0;
		fe_time_sequence->times = (FE_value *)NULL;

		fe_time_sequence->self = fe_time_sequence;

		fe_time_sequence->manager = (struct MANAGER(FE_time_sequence) *)NULL;
		fe_time_sequence->manager_change_status = MANAGER_CHANGE_NONE(FE_time_sequence);

		fe_time_sequence->access_count=0;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CREATE(FE_time_sequence).  Not enough memory");
		DEALLOCATE(fe_time_sequence);
	}
	LEAVE;

	return (fe_time_sequence);
} /* CREATE(FE_time_sequence) */

int DESTROY(FE_time_sequence)(struct FE_time_sequence **fe_time_sequence_address)
/*******************************************************************************
LAST MODIFIED : 9 November 2001

DESCRIPTION :
Frees memory/deaccess objects in fe_time_sequence at <*fe_time_sequence_address>.
==============================================================================*/
{
	int return_code;
	struct FE_time_sequence *fe_time_sequence;

	ENTER(DESTROY(FE_time_sequence));
	if (fe_time_sequence_address&&(fe_time_sequence= *fe_time_sequence_address))
	{
		if (0 >= fe_time_sequence->access_count)
		{
			if (fe_time_sequence->times)
			{
				DEALLOCATE(fe_time_sequence->times);
			}
			DEALLOCATE(*fe_time_sequence_address);
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"DESTROY(FE_time_sequence).  Positive access_count");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(FE_time_sequence).  Missing fe_time_sequence");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(FE_time_sequence) */

DECLARE_OBJECT_FUNCTIONS(FE_time_sequence)

DECLARE_INDEXED_LIST_FUNCTIONS(FE_time_sequence)

DECLARE_FIND_BY_IDENTIFIER_IN_INDEXED_LIST_FUNCTION(FE_time_sequence,self,
	struct FE_time_sequence *, compare_FE_time_sequence)

DECLARE_INDEXED_LIST_IDENTIFIER_CHANGE_FUNCTIONS(FE_time_sequence,self)

PROTOTYPE_MANAGER_COPY_WITH_IDENTIFIER_FUNCTION(FE_time_sequence,self)
{
	int return_code;

	ENTER(MANAGER_COPY_WITH_IDENTIFIER(FE_time_sequence,self));
	USE_PARAMETER(destination);
	USE_PARAMETER(source);
	display_message(ERROR_MESSAGE,
		"MANAGER_COPY_WITH_IDENTIFIER(FE_time_sequence,self).  "
		"You cannot do this on an FE_time_sequence, make a new one");
	return_code=0;
	LEAVE;

	return (return_code);
} /* MANAGER_COPY_WITH_IDENTIFIER(FE_time_sequence,name) */

PROTOTYPE_MANAGER_COPY_WITHOUT_IDENTIFIER_FUNCTION(FE_time_sequence,self)
/*******************************************************************************
LAST MODIFIED : 9 November 2001

DESCRIPTION :
Do not allow copy if:
- it creates a self-referencing fe_time_sequence (one that depends on itself) which will
  result in an infinite loop;
- it changes the number of components of a fe_time_sequence in use;
==============================================================================*/
{
	int return_code;

	ENTER(MANAGER_COPY_WITHOUT_IDENTIFIER(FE_time_sequence,self));
	USE_PARAMETER(destination);
	USE_PARAMETER(source);
	display_message(ERROR_MESSAGE,
		"MANAGER_COPY_WITHOUT_IDENTIFIER(FE_time_sequence,self).  "
		"You cannot do this on an FE_time_sequence, make a new one");
	return_code=0;
	LEAVE;

	return (return_code);
} /* MANAGER_COPY_WITHOUT_IDENTIFIER(FE_time_sequence,name) */

PROTOTYPE_MANAGER_COPY_IDENTIFIER_FUNCTION(FE_time_sequence,self,struct FE_time_sequence *)
{
	int return_code;

	ENTER(MANAGER_COPY_IDENTIFIER(FE_time_sequence,self));
	USE_PARAMETER(destination);
	USE_PARAMETER(self);
	display_message(ERROR_MESSAGE,
		"MANAGER_COPY_IDENTIFIER(FE_time_sequence,self).  "
		"You cannot do this on an FE_time_sequence, make a new one");
	return_code=0;
	LEAVE;

	return (return_code);
} /* MANAGER_COPY_IDENTIFIER(FE_time_sequence,name) */

DECLARE_MANAGER_FUNCTIONS(FE_time_sequence,manager)

DECLARE_DEFAULT_MANAGED_OBJECT_NOT_IN_USE_FUNCTION(FE_time_sequence,manager)

DECLARE_MANAGER_IDENTIFIER_FUNCTIONS(FE_time_sequence,self,struct FE_time_sequence *,manager)

int FE_time_sequence_get_number_of_times(
	struct FE_time_sequence *fe_time_sequence)
/*******************************************************************************
LAST MODIFIED : 15 November 2001

DESCRIPTION :
Returns the number of times that a particular FE_time_sequence references to.
==============================================================================*/
{
	int number_of_times;

	ENTER(FE_time_sequence_get_number_of_times);

	if (fe_time_sequence)
	{
		number_of_times = fe_time_sequence->number_of_times;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_time_sequence_get_number_of_times.  Invalid arguments");
		number_of_times = 0;
	}
	LEAVE;

	return (number_of_times);
} /* FE_time_sequence_get_number_of_times */

int FE_time_sequence_get_index_for_time(
	struct FE_time_sequence *fe_time_sequence, FE_value time, int *time_index)
/*******************************************************************************
LAST MODIFIED : 16 November 2001

DESCRIPTION :
Returns the integer <time_index> into the time array contained in this version
that corresponds to the <time>.  Returns 0 if that exact time is not found
and 1 if it is.
==============================================================================*/
{
	int return_code, time_index_one, time_index_two;
	FE_value xi;
	const FE_value xi_tolerance = 1e-5;


	ENTER(FE_time_sequence_get_index_for_time);

	if (fe_time_sequence)
	{
		return_code = 0;
		if (FE_time_sequence_get_interpolation_for_time(fe_time_sequence,
			time, &time_index_one, &time_index_two, &xi))
		{
			if (xi < (0.0 + xi_tolerance))
			{
				*time_index = time_index_one;
				return_code = 1;
			}
			else if ((1.0 - xi_tolerance) < xi)
			{
				*time_index = time_index_two;
				return_code = 1;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_time_sequence_get_index_for_time.  Invalid arguments");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_time_sequence_get_index_for_time */

int FE_time_sequence_get_interpolation_for_time(
	struct FE_time_sequence *fe_time_sequence, FE_value time, int *time_index_one,
	int *time_index_two, FE_value *xi)
/*******************************************************************************
LAST MODIFIED : 20 November 2001

DESCRIPTION :
Returns the two integers <time_index_one> and <time_index_two> which index into
the time array bracketing the supplied <time>, the <xi> value is set between 0
and 1 to indicate what fraction of the way between <time_index_one> and
<time_index_two> the value is found.  Returns 0 if time is outside the range
of the time index array however sets <time_index_one> and <time_index_two> to
be either the minimum or maximum value as appropriate.
==============================================================================*/
{
	int array_index,done,index_high,index_low,number_of_times,return_code,step;
	FE_value first_time,last_time,this_time,fe_value_index,time_high,time_low;

	ENTER(FE_time_sequence_get_index_for_time);

	if (fe_time_sequence)
	{
		return_code = 0;
		number_of_times = fe_time_sequence->number_of_times;

		first_time = fe_time_sequence->times[0];
		last_time = fe_time_sequence->times[number_of_times-1];
		time_low=0;
		time_high=0;
		if ((time >= first_time) && (time <= last_time))
		{
			/*Initial est. of the array index, assuming times evenly spaced, no gaps */
			/*This assumption and hence estimate is true for most signal files. */
			if (last_time>first_time)
			{
				fe_value_index=((time-first_time)/(last_time-first_time))*(number_of_times-1);
				array_index = static_cast<int>(fe_value_index+0.5);/*round float to nearest int */;
			}
			else
			{
				array_index = 0;
			}
			done=0;
			index_low=0;
			index_high=number_of_times-1;
			/* do binary search for <time>'s array index. Also look at time of */
			/* adjacent array element, as index estimate may be slightly off due to*/
			/* rounding error. This avoids unnecessarily long search from end of array */
			while(!done)
			{
				if ((array_index >= 0) && (array_index < number_of_times))
				{
					this_time = fe_time_sequence->times[array_index];
					if (this_time>time)
					{
						index_high=array_index;
						if (array_index>0)
						{
							/* get adjacent array element*/
							time_low = fe_time_sequence->times[array_index - 1];
							/* are we between elements?*/
							if (time_low<time)
							{
								index_low=array_index-1;
								return_code=1;
								done=1;
							}
							else
							{
								time_low=0;
							}
						}
						else
						{
							/* can't get lower adjacent array element when array_index=0. Finished*/
							time_low = fe_time_sequence->times[array_index];
							index_low=array_index;
							return_code=1;
							done=1;
						}
					}
					else if (this_time<time)
					{
						index_low=array_index;
						if (array_index<(number_of_times-1))
						{
							/* get adjacent array element*/
							time_high = fe_time_sequence->times[array_index + 1];
							/* are we between elements?*/
							if (time_high>time)
							{
								index_high=array_index+1;
								return_code=1;
								done=1;
							}
							else
							{
								time_high=0;
							}
						}
						else
						{
							/* can't get higher adjacent array element when */
							/*array_index=(number_of_times-1). Finished*/
							time_high = fe_time_sequence->times[array_index];
							index_high=array_index;
							return_code=1;
							done=1;
						}
					}
					else /* (this_time == time) */
					{
						index_low=array_index;
						index_high=array_index;
						time_high=this_time;
						time_low=this_time;
						return_code=1;
						done=1;
					}
					if (!done)
					{
						step=(index_high-index_low)/2;
						/* No exact match, can't subdivide further, must do interpolation.*/
						if (step==0)
						{
							done=1;
							return_code=1;
						}
						else
						{
							array_index=index_low+step;
						}

					}/* if (!done)	*/
				}
				else
				{
					if (array_index < 0)
					{
						/* Before start of times, don't write an error as
							we want the calling routine to decide how to treat this */
						done = 1;
						index_low = 0;
						index_high = 0;
						return_code = 0;
					}
					else
					{
						/* After end of times, don't write an error as
							we want the calling routine to decide how to treat this */
						done = 1;
						index_low = number_of_times-1;
						index_high = number_of_times-1;
						return_code = 0;
					}
				}
			}	/* while(!done)	*/
		}
		else
		{
			/* Outside range of sequence so return_code is zero but we
			 still set the indices */
			return_code = 0;
			if (time < first_time)
			{
				index_low = 0;
				index_high = 0;
			}
			else
			{
				index_low = number_of_times - 1;
				index_high = number_of_times - 1;
			}
		}
		/* index_low and index_high should now be adjacent */
		if (!time_low)
		{
			time_low = fe_time_sequence->times[index_low];
		}
		if (!time_high)
		{
			time_high = fe_time_sequence->times[index_high];
		}
		*time_index_one = index_low;
		*time_index_two = index_high;
		if (time_high > time_low)
		{
			*xi = (time - time_low) / (time_high - time_low);
		}
		else
		{
			*xi = 0.0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_time_sequence_get_interpolation_for_time.  "
			"Invalid arguments time out of range");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* FE_time_sequence_get_interpolation_for_time */

int FE_time_sequence_get_nearest_time_index_for_time(
	struct FE_time_sequence *fe_time_sequence, FE_value time)
{
	int time_index_one, time_index_two;
	FE_value xi;
	if (FE_time_sequence_get_interpolation_for_time(fe_time_sequence, time, &time_index_one,
		&time_index_two, &xi))
	{
		return (xi < 0.5) ? time_index_one : time_index_two;
	}
	return 0;
}

int FE_time_sequence_get_time_for_index(
	struct FE_time_sequence *fe_time_sequence, int time_index, FE_value *time)
/*******************************************************************************
LAST MODIFIED : 15 November 2001

DESCRIPTION :
If the <time_index> is valid returns the corresponding <time>.
==============================================================================*/
{
	int return_code;

	ENTER(FE_time_sequence_get_time_for_index);

	if (fe_time_sequence)
	{
		if ((time_index >= 0) && (time_index < fe_time_sequence->number_of_times))
		{
			*time = fe_time_sequence->times[time_index];
			return_code = 1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_time_sequence_get_time_for_index.  Time index out of range");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_time_sequence_get_time_for_index.  Invalid arguments");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
}

int FE_time_sequence_set_time_and_index(
	struct FE_time_sequence *fe_time_sequence, int time_index, FE_value time)
/*******************************************************************************
LAST MODIFIED : 18 November 2004

DESCRIPTION :
Sets the <time> for the given <time_index> in the <fe_time_sequence>.  This
should only be done for unmanaged time sequences (as otherwise this sequence
may be shared by many other objects which are not expecting changes).
If the sequence does not have as many times as the <time_index> then it will
be expanded and the unspecified times also set to <time>.
==============================================================================*/
{
	FE_value *new_times;
	int i, return_code;

	ENTER(FE_time_sequence_set_time_and_index);

	if (fe_time_sequence)
	{
		if (time_index >= 0)
		{
			return_code = CMZN_OK;
			if (time_index >= fe_time_sequence->number_of_times)
			{
				if (REALLOCATE(new_times, fe_time_sequence->times,
					FE_value, time_index + 1))
				{
					fe_time_sequence->times = new_times;
					for (i = fe_time_sequence->number_of_times ;
						i <= time_index ; i++)
					{
						new_times[i] = time;
					}
					fe_time_sequence->number_of_times = time_index + 1;
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"FE_time_sequence_set_time_and_index.  Unable to reallocate times");
					return_code = CMZN_ERROR_MEMORY;
				}
			}
			else
			{
				fe_time_sequence->times[time_index] = time;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_time_sequence_set_time_and_index.  Time index out of range");
			return_code = CMZN_ERROR_ARGUMENT;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_time_sequence_set_time_and_index.  Invalid arguments");
		return_code = CMZN_ERROR_ARGUMENT;
	}
	LEAVE;

	return (return_code);
} /* FE_time_sequence_set_time_and_index */

struct FE_time_sequence *get_FE_time_sequence_matching_FE_time_sequence(
	struct FE_time_sequence_package *fe_time, struct FE_time_sequence *source_fe_time_sequence)
/*******************************************************************************
LAST MODIFIED : 27 November 2002

DESCRIPTION :
Searches <fe_time> for a FE_time_sequence matching <source_fe_time_sequence>.
If no equivalent fe_time_sequence is found one is created in <fe_time> and
returned.
==============================================================================*/
{
	int return_code;
	struct FE_time_sequence *fe_time_sequence;

	ENTER(get_FE_time_sequence_matching_FE_time_sequence);
	if (fe_time && fe_time->fe_time_sequence_manager && source_fe_time_sequence)
	{
		/* first try to find a matching fe_time_sequence in the manager */
		if (!(fe_time_sequence = FIND_BY_IDENTIFIER_IN_MANAGER(FE_time_sequence,self)(
			source_fe_time_sequence, fe_time->fe_time_sequence_manager)))
		{
			if (NULL != (fe_time_sequence = CREATE(FE_time_sequence)()))
			{
				return_code = 1;
				switch (source_fe_time_sequence->type)
				{
					case FE_TIME_SEQUENCE:
					{
						fe_time_sequence->type = FE_TIME_SEQUENCE;
						fe_time_sequence->number_of_times =
							source_fe_time_sequence->number_of_times;
						fe_time_sequence->times = (FE_value *)NULL;
						if (ALLOCATE(fe_time_sequence->times, FE_value,
							source_fe_time_sequence->number_of_times))
						{
							memcpy(fe_time_sequence->times, source_fe_time_sequence->times,
								source_fe_time_sequence->number_of_times*sizeof(FE_value));
							if (!ADD_OBJECT_TO_MANAGER(FE_time_sequence)(fe_time_sequence,
								fe_time->fe_time_sequence_manager))
							{
								return_code = 0;
							}
						}
						else
						{
							return_code = 0;
						}
					} break;
					default:
					{
						display_message(ERROR_MESSAGE,
							"get_FE_time_sequence_matching_FE_time_sequence.  "
							"Unimplemented FE_time_sequence type");
						return_code = 0;
					} break;
				}
				if (!return_code)
				{
					display_message(ERROR_MESSAGE,
						"get_FE_time_sequence_matching_FE_time_sequence.  "
						"Could not copy contents of FE_time_sequence");
					DESTROY(FE_time_sequence)(&fe_time_sequence);
					fe_time_sequence = (struct FE_time_sequence *)NULL;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"get_FE_time_sequence_matching_FE_time_sequence.  "
					"Could not create FE_time_sequence");
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"get_FE_time_sequence_matching_FE_time_sequence.  Invalid argument(s)");
		fe_time_sequence = (struct FE_time_sequence *)NULL;
	}
	LEAVE;

	return (fe_time_sequence);
} /* get_FE_time_sequence_matching_FE_time_sequence */

struct FE_time_sequence *get_FE_time_sequence_matching_time_series(
	struct FE_time_sequence_package *fe_time, int number_of_times, const FE_value *times)
/*******************************************************************************
LAST MODIFIED : 9 November 2001

DESCRIPTION :
Searches <fe_time> for a fe_time_sequence which has the time series specified.
If no equivalent fe_time_sequence is found one is created and returned.
==============================================================================*/
{
	struct FE_time_sequence *fe_time_sequence, *local_fe_time_sequence;

	ENTER(get_FE_time_sequence_matching_time_series);
	fe_time_sequence=(struct FE_time_sequence *)NULL;
	if (fe_time&&fe_time->fe_time_sequence_manager&&(0 < number_of_times)&&times)
	{
		/* Create a FE_time_sequence into which we will poke a reference to this
			number of times and times array so that we can look for another one the
			same.  I really want to avoid copying the array unnecessarily but if
			the CREATE(FE_time_sequence) routine was changed to allocate the times array
			by default then this would leak.*/
		local_fe_time_sequence = CREATE(FE_time_sequence)();
		local_fe_time_sequence->type = FE_TIME_SEQUENCE;
		local_fe_time_sequence->number_of_times = number_of_times;
		local_fe_time_sequence->times = const_cast<FE_value*>(times);
		/* search the manager for a fe_time_sequence of that name */
		if (NULL != (fe_time_sequence=
			FIND_BY_IDENTIFIER_IN_MANAGER(FE_time_sequence,self)(local_fe_time_sequence,
				fe_time->fe_time_sequence_manager)))
		{
			/* Found so get rid of our local one. */
			local_fe_time_sequence->number_of_times = 0;
			local_fe_time_sequence->times = (FE_value *)NULL;
			DESTROY(FE_time_sequence)(&local_fe_time_sequence);
		}
		else
		{
			/* Finish establishing our local one correctly. */
			local_fe_time_sequence->times = (FE_value *)NULL;
			if (ALLOCATE(local_fe_time_sequence->times, FE_value, number_of_times))
			{
				memcpy(local_fe_time_sequence->times, times, number_of_times *
					sizeof(FE_value));
				if (ADD_OBJECT_TO_MANAGER(FE_time_sequence)(local_fe_time_sequence,
					fe_time->fe_time_sequence_manager))
				{
					fe_time_sequence = local_fe_time_sequence;
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"get_FE_time_sequence_matching_time_series.  "
						"Unable to add object to manager");
					fe_time_sequence=(struct FE_time_sequence *)NULL;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"get_FE_time_sequence_matching_time_series.  "
					"Unable to allocate memory");
				fe_time_sequence=(struct FE_time_sequence *)NULL;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"get_FE_time_sequence_matching_time_series.  Invalid argument(s)");
		fe_time_sequence=(struct FE_time_sequence *)NULL;
	}
	LEAVE;

	return (fe_time_sequence);
} /* get_FE_time_sequence_matching_time_series */

struct FE_time_sequence *get_FE_time_sequence_merging_two_time_series(
	struct FE_time_sequence_package *fe_time, struct FE_time_sequence *time_sequence_one,
	struct FE_time_sequence *time_sequence_two)
/*******************************************************************************
LAST MODIFIED : 20 November 2001

DESCRIPTION :
Searches <fe_time> for a fe_time_sequence which has the list of times formed
by merging the two time_sequences supplied.
==============================================================================*/
{
	int i, maximum_number_of_times, number_of_times, time_found, time_index;
	FE_value *end_one, *end_two, *index_one, *index_two, *merged_index, *times;
	struct FE_time_sequence *fe_time_sequence;
#define TIME_SEQUENCE_MERGING_SMALL_COUNT (10)

	ENTER(get_FE_time_sequence_merging_two_time_series);
	fe_time_sequence=(struct FE_time_sequence *)NULL;
	if (fe_time&&fe_time->fe_time_sequence_manager&&time_sequence_one&&
		time_sequence_two)
	{
		if (time_sequence_one == time_sequence_two)
		{
			fe_time_sequence = time_sequence_one;
		}
		else
		{
			/* Make a fast path if there only a few values, by checking to see if they
				are a subset of the other sequence */
			if (time_sequence_one->number_of_times <= time_sequence_two->number_of_times)
			{
				if (time_sequence_one->number_of_times < TIME_SEQUENCE_MERGING_SMALL_COUNT)
				{
					i = 0;
					time_found = 1;
					while (time_found && (i < time_sequence_one->number_of_times))
					{
						time_found = FE_time_sequence_get_index_for_time(
							time_sequence_two, time_sequence_one->times[i], &time_index);
						i++;
					}
					if (time_found)
					{
						fe_time_sequence = time_sequence_two;
					}
				}
			}
			else
			{
				if (time_sequence_two->number_of_times < TIME_SEQUENCE_MERGING_SMALL_COUNT)
				{
					i = 0;
					time_found = 1;
					while (time_found && (i < time_sequence_two->number_of_times))
					{
						time_found = FE_time_sequence_get_index_for_time(
							time_sequence_one, time_sequence_two->times[i], &time_index);
						i++;
					}
					if (time_found)
					{
						fe_time_sequence = time_sequence_one;
					}
				}
			}
			if (!fe_time_sequence)
			{
				maximum_number_of_times = time_sequence_one->number_of_times +
					time_sequence_two->number_of_times;
				if (ALLOCATE(times, FE_value, maximum_number_of_times))
				{
					number_of_times = 0;
					merged_index = times;
					index_one = time_sequence_one->times;
					index_two = time_sequence_two->times;
					end_one = time_sequence_one->times + time_sequence_one->number_of_times;
					end_two = time_sequence_two->times + time_sequence_two->number_of_times;

					while ((index_one < end_one) && (index_two < end_two))
					{
						if (*index_one < *index_two)
						{
							*merged_index = *index_one;
							index_one++;
							number_of_times++;
							merged_index++;
						}
						else if (*index_one > *index_two)
						{
							*merged_index = *index_two;
							index_two++;
							number_of_times++;
							merged_index++;
						}
						else
						{
							*merged_index = *index_one;
							index_one++;
							index_two++;
							number_of_times++;
							merged_index++;
						}
					}
					if (index_one < end_one)
					{
						/* Copy over the end of array one */
						while(index_one < end_one)
						{
							*merged_index = *index_one;
							index_one++;
							number_of_times++;
							merged_index++;
						}
					}
					else if (index_two < end_two)
					{
						/* Copy over the end of array two */
						while(index_two < end_two)
						{
							*merged_index = *index_two;
							index_two++;
							number_of_times++;
							merged_index++;
						}
					}

					fe_time_sequence = get_FE_time_sequence_matching_time_series(
						fe_time, number_of_times, times);

					DEALLOCATE(times);
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"get_FE_time_sequence_merging_two_time_series.  "
						"Could not ALLOCATE temporary time array.");
					fe_time_sequence=(struct FE_time_sequence *)NULL;
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"get_FE_time_sequence_merging_two_time_series.  Invalid argument(s)");
		fe_time_sequence=(struct FE_time_sequence *)NULL;
	}
	LEAVE;

	return (fe_time_sequence);
} /* get_FE_time_sequence_merging_two_time_series */

enum FE_time_sequence_mapping FE_time_sequences_mapping(
	struct FE_time_sequence *source_sequence,
	struct FE_time_sequence *destination_sequence)
/*******************************************************************************
LAST MODIFIED : 13 July 2005

DESCRIPTION :
Attempts to deduce a mapping that goes from the <source_sequence> to the
<destination_sequence> and returns an enumerator describing that mapping.
If the change isn't recognised then it will return
FE_TIME_SEQUENCE_MAPPING_UNKNOWN;
==============================================================================*/
{
	enum FE_time_sequence_mapping mapping;

	ENTER(FE_time_sequences_mapping);
	if (source_sequence && destination_sequence)
	{
		/* Could add some caching to this test by keeping a list of known mappings
			with the source and/or destination sequence, however need to be careful that
			the source or destination sequence hasn't changed on us */
		mapping = FE_TIME_SEQUENCE_MAPPING_UNKNOWN;
		/* Using a char based memcmp for speed.  I think that
			it is OK to misrepresent these FE_values as chars
			as so long as we have a consistent order it is OK,
			if we wanted different representations of the same
			number to match we would have to compare FE_values
			instead. */
		if ((source_sequence->number_of_times == destination_sequence->number_of_times)
			&& !memcmp(source_sequence->times, destination_sequence->times,
				source_sequence->number_of_times * sizeof(FE_value)))
		{
			mapping = FE_TIME_SEQUENCE_MAPPING_IDENTICAL;
		}
		else if ((source_sequence->number_of_times < destination_sequence->number_of_times)
			&& !memcmp(source_sequence->times, destination_sequence->times,
				source_sequence->number_of_times * sizeof(FE_value)))
		{
			mapping = FE_TIME_SEQUENCE_MAPPING_APPEND;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_time_sequences_mapping.  Invalid argument(s)");
		mapping = FE_TIME_SEQUENCE_MAPPING_UNKNOWN;
	}
	LEAVE;

	return (mapping);
} /* FE_time_sequences_mapping */

int FE_time_sequence_package_has_FE_time_sequence(struct FE_time_sequence_package *fe_time,
	struct FE_time_sequence *fe_time_sequence)
/*******************************************************************************
LAST MODIFIED : 12 November 2004

DESCRIPTION :
Returns true if <fe_time> contains the <fe_time_sequence>.
==============================================================================*/
{
	int return_code;

	ENTER(FE_time_has_FE_time_sequence);
	if (fe_time && fe_time_sequence)
	{
		if (FIND_BY_IDENTIFIER_IN_MANAGER(FE_time_sequence,self)(fe_time_sequence,
			fe_time->fe_time_sequence_manager))
		{
			return_code = 1;
		}
		else
		{
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_time_has_FE_time_sequence.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_time_has_FE_time_sequence */

int FE_time_sequence_is_in_use(struct FE_time_sequence *fe_time_sequence)
{
	if (NULL == fe_time_sequence)
		return 0;
	/* basic implementation assumes object is accessed by region and caller */
	return (fe_time_sequence->access_count > 2);
}

cmzn_timesequence_id cmzn_timesequence_access(
	cmzn_timesequence_id timesequence)
{
	if (timesequence)
		return (cmzn_timesequence_id)(ACCESS(FE_time_sequence)(
			(struct FE_time_sequence *)timesequence));
	return 0;
}

int cmzn_timesequence_destroy(cmzn_timesequence_id *timesequence_address)
{
	return DEACCESS(FE_time_sequence)(
		(struct FE_time_sequence **)timesequence_address);
}

int cmzn_timesequence_get_number_of_times(
	cmzn_timesequence_id timesequence)
{
	return FE_time_sequence_get_number_of_times(
		reinterpret_cast<struct FE_time_sequence *>(timesequence));
}

double cmzn_timesequence_get_time(cmzn_timesequence_id timesequence,
	int time_index)
{
	FE_value value = 0;
	struct FE_time_sequence *fe_timesequence =
		reinterpret_cast<struct FE_time_sequence *>(timesequence);
	FE_time_sequence_get_time_for_index(fe_timesequence, time_index - 1, &value);
	return static_cast<double>(value);
}

int cmzn_timesequence_set_time(cmzn_timesequence_id timesequence,
	int time_index, double time)
{
	struct FE_time_sequence *fe_timesequence =
		reinterpret_cast<struct FE_time_sequence *>(timesequence);
	if (FE_time_sequence_is_in_use(fe_timesequence))
	{
		display_message(ERROR_MESSAGE, "cmzn_timesequence_set_time.  "
			"Cannot modify time sequence while in use");
		return CMZN_ERROR_ARGUMENT;
	}
	return FE_time_sequence_set_time_and_index(fe_timesequence, time_index - 1, time);
}
