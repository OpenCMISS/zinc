/*******************************************************************************
FILE : compare.c

LAST MODIFIED :  15 March 1999

DESCRIPTION :
Functions for comparing data types (analogous to strcmp).  Functions used for
managers and lists.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stddef.h>

#include "general/compare.h"
#include "general/debug.h"

/*
Global functions
----------------
*/
int compare_int(int int_1,int int_2)
/*******************************************************************************
LAST MODIFIED : 27 April 1995

DESCRIPTION :
Returns -1 if int_1 < int_2, 0 if int_1 = int_2 and 1 if int_1 > int_2.
==============================================================================*/
{
	int return_code;

	ENTER(compare_int);
	if (int_1<int_2)
	{
		return_code= -1;
	}
	else
	{
		if (int_1>int_2)
		{
			return_code=1;
		}
		else
		{
			return_code=0;
		}
	}
	LEAVE;

	return (return_code);
} /* compare_int */

int compare_pointer(void *pointer_1,void *pointer_2)
/*******************************************************************************
LAST MODIFIED : 15 March 1999

DESCRIPTION :
Returns -1 if pointer_1 < pointer_2, 0 if pointer_1 = pointer_2 
and 1 if pointer_1 > pointer_2.
==============================================================================*/
{
	int return_code;

	ENTER(compare_pointer);
	if (pointer_1<pointer_2)
	{
		return_code= -1;
	}
	else
	{
		if (pointer_1>pointer_2)
		{
			return_code=1;
		}
		else
		{
			return_code=0;
		}
	}
	LEAVE;

	return (return_code);
} /* compare_pointer */

int compare_double(double double_1, double double_2)
/*******************************************************************************
LAST MODIFIED : 15 March 1999

DESCRIPTION :
Returns -1 if double_1 < double_2, 0 if double_1 = double_2 
and 1 if double_1 > double_2.
==============================================================================*/
{
	int return_code;

	ENTER(compare_double);
	if (double_1<double_2)
	{
		return_code= -1;
	}
	else
	{
		if (double_1>double_2)
		{
			return_code=1;
		}
		else
		{
			return_code=0;
		}
	}
	LEAVE;

	return (return_code);
} /* compare_double */
