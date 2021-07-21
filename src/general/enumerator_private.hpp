/*******************************************************************************
FILE : enumerator_private.h

LAST MODIFIED : 19 March 2001

DESCRIPTION :
Definitions of macro functions for handling enumerators.
Useful for automatic menu/command/dialog widget generation.
Usage:
1. #include enumerator.h in the .h file you are defining an enumerator in.
2. Call the prototypes for the types and functions you wish to use. Prototype
all of them with PROTOTYPE_ENUMERATOR_FUNCTIONS(enumerator type).
3. #include enumerator_private.h in the .c file matching your .h file above.
4. Write an ENUMERATOR_STRING function for your enumerator type.
5. Call the default function definition macros for any other functions you wish
to use; note they assume the range of enumerated values starts at 0 and
increments by 1 to the last value which has an ENUMERATOR_STRING. If your enum
declaration does not match this pattern, you will have to write the contents
of these functions yourself.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (ENUMERATOR_PRIVATE_H)
#define ENUMERATOR_PRIVATE_H

#include "general/enumerator.h"

/*
Macros
======
*/

/*
Global functions
----------------
*/

#define DEFINE_DEFAULT_ENUMERATOR_GET_VALID_STRINGS_FUNCTION( enumerator_type )\
PROTOTYPE_ENUMERATOR_GET_VALID_STRINGS_FUNCTION(enumerator_type) \
/***************************************************************************** \
LAST MODIFIED : 19 March 2001 \
\
DESCRIPTION : \
Returns an allocated array containing the ENUMERATOR_STRINGs for all valid \
enumerator values in the enumerator_type. The optional <conditional_function> \
and <user_data> allow reduced selections of strings to be returned. \
Note it is up to the calling function to deallocate the returned array, but \
the strings it contains must NOT be deallocated. \
Default version assumes all valid enumerator values are sequential from 0. \
============================================================================*/ \
{ \
	const char *enumerator_string, **valid_strings; \
	int enumerator_value, first_enumerator_value, i;		       \
\
	ENTER(ENUMERATOR_GET_VALID_STRINGS(enumerator_type)); \
	valid_strings = (const char **)NULL; \
	if (number_of_valid_strings) \
	{ \
		*number_of_valid_strings = 0; \
		/* valid modes are from 0 or 1 to the last one with a string */ \
		first_enumerator_value = 0; \
		if (NULL == ENUMERATOR_STRING(enumerator_type)( \
			static_cast<enum enumerator_type >(first_enumerator_value))) \
		{ \
			first_enumerator_value = static_cast<enum enumerator_type>(1); \
		} \
		for (enumerator_value = first_enumerator_value; \
			ENUMERATOR_STRING(enumerator_type)(static_cast<enum enumerator_type >(enumerator_value)); \
				++enumerator_value)		\
		{ \
			if ((NULL == conditional_function) || \
			    conditional_function(static_cast<enum enumerator_type >(enumerator_value), user_data)) \
			{ \
				++(*number_of_valid_strings); \
			} \
		} \
		if ((0 == *number_of_valid_strings) || \
			ALLOCATE(valid_strings, const char *, *number_of_valid_strings)) \
		{ \
			i = 0; \
			for (enumerator_value = first_enumerator_value; ((NULL != (enumerator_string = \
				ENUMERATOR_STRING(enumerator_type)(static_cast<enum enumerator_type >(enumerator_value))))); \
					++enumerator_value)		\
			{ \
				if ((NULL == conditional_function) || \
				    conditional_function(static_cast<enum enumerator_type >(enumerator_value), user_data)) \
				{ \
					valid_strings[i] = enumerator_string; \
					i++; \
				} \
			} \
		} \
		else \
		{ \
			display_message(ERROR_MESSAGE, \
				"ENUMERATOR_GET_VALID_STRINGS(" #enumerator_type \
				").  Not enough memory"); \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"ENUMERATOR_GET_VALID_STRINGS(" #enumerator_type \
			").  Invalid argument(s)"); \
	} \
	LEAVE; \
\
	return (valid_strings); \
} /* ENUMERATOR_GET_VALID_STRINGS(enumerator_type) */

#define DEFINE_DEFAULT_STRING_TO_ENUMERATOR_FUNCTION( enumerator_type ) \
PROTOTYPE_STRING_TO_ENUMERATOR_FUNCTION(enumerator_type) \
/***************************************************************************** \
LAST MODIFIED : 16 March 2001 \
\
DESCRIPTION : \
If <enumerator_string> matches the ENUMERATOR_STRING(enumerator_type) for any \
enumerator_value, returns the enumerator_value in *<enumerator_value_address>, \
with a true return_code. Otherwise a false return_code is returned, without  \
an error message, for the calling function to handle. \
Default version assumes all valid enumerator values are sequential from 0. \
============================================================================*/ \
{ \
	const char *other_enumerator_string = NULL; \
	int enumerator_value, return_code;	\
\
	ENTER(STRING_TO_ENUMERATOR(enumerator_type)); \
	if (enumerator_string && enumerator_value_address) \
	{ \
		enumerator_value = (enum enumerator_type)0; \
		/* valid modes are from 0 or 1 to the last one with a string */ \
		if (NULL == ENUMERATOR_STRING(enumerator_type)( \
			static_cast<enum enumerator_type >(enumerator_value))) \
		{ \
			enumerator_value = static_cast<enum enumerator_type>(1); \
		} \
		while ((other_enumerator_string = \
			ENUMERATOR_STRING(enumerator_type)(static_cast<enum enumerator_type>(enumerator_value))) && \
			(!fuzzy_string_compare_same_length(enumerator_string, \
				other_enumerator_string))) \
		{ \
			++enumerator_value; \
		} \
		if (other_enumerator_string) \
		{ \
			*enumerator_value_address = static_cast<enum enumerator_type>(enumerator_value); \
			return_code = 1; \
		} \
		else \
		{ \
			return_code = 0; \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"STRING_TO_ENUMERATOR(" #enumerator_type ").  Invalid argument(s)"); \
		return_code = 0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* STRING_TO_ENUMERATOR(enumerator_type) */

#define DEFINE_DEFAULT_ENUMERATOR_PREFIX_INCREMENT_OPERATOR( enumerator_type ) \
enumerator_type& operator++(enumerator_type& orig) \
{ \
	orig = static_cast<enumerator_type>(orig + 1); \
	return orig; \
}

#define DEFINE_DEFAULT_ENUMERATOR_POSTFIX_INCREMENT_OPERATOR( enumerator_type ) \
enumerator_type operator++(enumerator_type& orig, int) \
{ \
	enumerator_type rVal = orig; \
	++orig; \
	return rVal; \
}


#define set_enumerator_macro(enumerator_type) set_enumerator ## enumerator_type

#define DEFINE_DEFAULT_ENUMERATOR_FUNCTIONS( enumerator_type ) \
DEFINE_DEFAULT_ENUMERATOR_PREFIX_INCREMENT_OPERATOR( enumerator_type ) \
DEFINE_DEFAULT_ENUMERATOR_POSTFIX_INCREMENT_OPERATOR( enumerator_type ) \
DEFINE_DEFAULT_ENUMERATOR_GET_VALID_STRINGS_FUNCTION(enumerator_type) \
DEFINE_DEFAULT_STRING_TO_ENUMERATOR_FUNCTION(enumerator_type)

#endif /* !defined (ENUMERATOR_PRIVATE_H) */
