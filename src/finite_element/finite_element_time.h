/*******************************************************************************
FILE : finite_element.h

LAST MODIFIED : 12 November 2004

DESCRIPTION :
Representing time in finite elements.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (FINITE_ELEMENT_TIME_H)
#define FINITE_ELEMENT_TIME_H

#include "general/list.h"
#include "general/manager.h"
#include "general/object.h"
#include "general/value.h"

/*
Global types
------------
*/

enum FE_time_sequence_type
/******************************************************************************
LAST MODIFIED : 9 November 2001

DESCRIPTION :
==============================================================================*/
{
	FE_TIME_SEQUENCE
};

struct FE_time_sequence_package;
/*******************************************************************************
LAST MODIFIED : 9 November 2001

DESCRIPTION :
struct FE_time_sequence_package is private.
==============================================================================*/

struct FE_time_sequence;
/*******************************************************************************
LAST MODIFIED : 9 November 2001

DESCRIPTION :
struct FE_time_sequence is private.
==============================================================================*/

DECLARE_LIST_TYPES(FE_time_sequence);

DECLARE_MANAGER_TYPES(FE_time_sequence);

enum FE_time_sequence_mapping 
/*******************************************************************************
LAST MODIFIED : 9 November 2001

DESCRIPTION :
Some simple mappings between time_sequences that can be used to accelerate
copying and merging operations.
==============================================================================*/
{
	FE_TIME_SEQUENCE_MAPPING_UNKNOWN,
	FE_TIME_SEQUENCE_MAPPING_IDENTICAL,
	FE_TIME_SEQUENCE_MAPPING_APPEND
};

/*
Global functions
----------------
*/
int compare_FE_time_sequence(struct FE_time_sequence *fe_time_sequence_1,
	struct FE_time_sequence *fe_time_sequence_2);
/*******************************************************************************
LAST MODIFIED : 17 November 2004

DESCRIPTION :
Returns -1 if fe_time_sequence_1 < fe_time_sequence_2, 
0 if fe_time_sequence_1 = fe_time_sequence_2 and 1 if
fe_time_sequence_1 > fe_time_sequence_2.
==============================================================================*/

struct FE_time_sequence_package *CREATE(FE_time_sequence_package)(void);
/*******************************************************************************
LAST MODIFIED : 9 November 2001

DESCRIPTION :
Creates FE_time_sequence_package.
==============================================================================*/

int DESTROY(FE_time_sequence_package)(struct FE_time_sequence_package **fe_time_address);
/*******************************************************************************
LAST MODIFIED : 9 November 2001

DESCRIPTION :
Frees memory/deaccess objects in FE_time_sequence_package at <*field_address>.
==============================================================================*/

struct FE_time_sequence *CREATE(FE_time_sequence)(void);
/*******************************************************************************
LAST MODIFIED : 9 November 2001

DESCRIPTION :
Creates a basic FE_time_sequence.
==============================================================================*/

int DESTROY(FE_time_sequence)(struct FE_time_sequence **fe_time_sequence_address);
/*******************************************************************************
LAST MODIFIED : 9 November 2001

DESCRIPTION :
Frees memory/deaccess objects in FE_time_sequence at <*field_address>.
==============================================================================*/

PROTOTYPE_OBJECT_FUNCTIONS(FE_time_sequence_package);

PROTOTYPE_OBJECT_FUNCTIONS(FE_time_sequence);

PROTOTYPE_LIST_FUNCTIONS(FE_time_sequence);
PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(FE_time_sequence,name,const char *);

PROTOTYPE_MANAGER_COPY_FUNCTIONS(FE_time_sequence,name,const char *);
PROTOTYPE_MANAGER_FUNCTIONS(FE_time_sequence);
PROTOTYPE_MANAGER_IDENTIFIER_FUNCTIONS(FE_time_sequence,name,const char *);

struct FE_time_sequence *get_FE_time_sequence_matching_FE_time_sequence(
	struct FE_time_sequence_package *fe_time, struct FE_time_sequence *source_fe_time_sequence);
/*******************************************************************************
LAST MODIFIED : 27 November 2002

DESCRIPTION :
Searches <fe_time> for a FE_time_sequence matching <source_fe_time_sequence>.
If no equivalent fe_time_sequence is found one is created in <fe_time> and
returned.
==============================================================================*/

struct FE_time_sequence *get_FE_time_sequence_matching_time_series(
	struct FE_time_sequence_package *fe_time, int number_of_times, const FE_value *times);
/*******************************************************************************
LAST MODIFIED : 9 November 2001

DESCRIPTION :
Searches <fe_time> for a fe_time_sequence which has the time series specified. 
If no equivalent fe_time_sequence is found one is created and returned.
==============================================================================*/

struct FE_time_sequence *get_FE_time_sequence_merging_two_time_series(
	struct FE_time_sequence_package *fe_time, struct FE_time_sequence *time_sequence_one,
	struct FE_time_sequence *time_sequence_two);
/*******************************************************************************
LAST MODIFIED : 20 November 2001

DESCRIPTION :
Searches <fe_time> for a fe_time_sequence which has the list of times formed
by merging the two time_sequences supplied.
==============================================================================*/

int FE_time_sequence_get_number_of_times(
	struct FE_time_sequence *fe_time_sequence);
/*******************************************************************************
LAST MODIFIED : 15 November 2001

DESCRIPTION :
Returns the number of times that a particular FE_time_sequence references to.
==============================================================================*/

int FE_time_sequence_get_index_for_time(
	struct FE_time_sequence *fe_time_sequence, FE_value time, int *time_index);
/*******************************************************************************
LAST MODIFIED : 16 November 2001

DESCRIPTION :
Returns the integer <time_index> into the time array contained in this version 
that corresponds to the <time>.  Returns 0 if that exact time is not found
and 1 if it is.
==============================================================================*/

int FE_time_sequence_get_time_for_index(
	struct FE_time_sequence *fe_time_sequence, int time_index, FE_value *time);
/*******************************************************************************
LAST MODIFIED : 15 November 2001

DESCRIPTION :
If the <time_index> is valid returns the corresponding <time>.
==============================================================================*/

int FE_time_sequence_set_time_and_index(
	struct FE_time_sequence *fe_time_sequence, int time_index, FE_value time);
/*******************************************************************************
LAST MODIFIED : 18 November 2004

DESCRIPTION :
Sets the <time> for the given <time_index> in the <fe_time_sequence>.  This 
should only be done for unmanaged time sequences (as otherwise this sequence
may be shared by many other objects which are not expecting changes).
If the sequence does not have as many times as the <time_index> then it will
be expanded and the unspecified times also set to <time>.
==============================================================================*/

int FE_time_sequence_get_interpolation_for_time(
	struct FE_time_sequence *fe_time_sequence, FE_value time, int *time_index_one,
	int *time_index_two, FE_value *xi);
/*******************************************************************************
LAST MODIFIED : 20 November 2001

DESCRIPTION :
Returns the two integers <time_index_one> and <time_index_two> which index into
the time array bracketing the supplied <time>, the <xi> value is set between 0
and 1 to indicate what fraction of the way between <time_index_one> and 
<time_index_two> the value is found.  Returns 0 if time is outside the range
of the time index array.
==============================================================================*/

/** @return  Nearest time index to time for time sequence */
int FE_time_sequence_get_nearest_time_index_for_time(
	struct FE_time_sequence *fe_time_sequence, FE_value time);

enum FE_time_sequence_mapping FE_time_sequences_mapping(
	struct FE_time_sequence *source_sequence,
	struct FE_time_sequence *destination_sequence);
/*******************************************************************************
LAST MODIFIED : 13 July 2005

DESCRIPTION :
Attempts to deduce a mapping that goes from the <source_sequence> to the 
<destination_sequence> and returns an enumerator describing that mapping.
If the change isn't recognised then it will return 
FE_TIME_SEQUENCE_MAPPING_UNKNOWN;
==============================================================================*/

int FE_time_sequence_package_has_FE_time_sequence(
	struct FE_time_sequence_package *fe_time_sequence_package,
	struct FE_time_sequence *fe_time_sequence);
/*******************************************************************************
LAST MODIFIED : 12 November 2004

DESCRIPTION :
Returns true if <fe_time_sequence_package> contains the <fe_time_sequence>.
==============================================================================*/

/***************************************************************************//**
 * @return  1 if the time sequence is in use by other objects and should thus
 * not be modified, or 0 if it is not in use.
 */
int FE_time_sequence_is_in_use(struct FE_time_sequence *fe_time_sequence);

#endif /* !defined (FINITE_ELEMENT_TIME_H) */
