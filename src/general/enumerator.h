/*******************************************************************************
FILE : enumerator.h

LAST MODIFIED : 16 March 2001

DESCRIPTION :
Prototypes for macro functions for handling enumerators.
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
#if !defined (ENUMERATOR_H)
#define ENUMERATOR_H

/*
Macros
======
*/

/*
Global types
------------
*/

#if ! defined (SHORT_NAMES)
#define ENUMERATOR_CONDITIONAL_FUNCTION( enumerator_type ) \
	enumerator_conditional_function_ ## enumerator_type
#else
#define ENUMERATOR_CONDITIONAL_FUNCTION( enumerator_type ) \
	ecf_ ## enumerator_type
#endif

#define DECLARE_ENUMERATOR_CONDITIONAL_FUNCTION_TYPE( enumerator_type ) \
typedef int (ENUMERATOR_CONDITIONAL_FUNCTION(enumerator_type)) \
	(enum enumerator_type enumerator_value, void *user_data)

/*
Global functions
----------------
*/

#if ! defined (SHORT_NAMES)
#define ENUMERATOR_STRING( enumerator_type ) \
	enumerator_string_ ## enumerator_type
#else
#define ENUMERATOR_STRING( enumerator_type ) es_ ## enumerator_type
#endif

#define PROTOTYPE_ENUMERATOR_STRING_FUNCTION( enumerator_type ) \
const char *ENUMERATOR_STRING(enumerator_type)( \
  enum enumerator_type enumerator_value)
/*******************************************************************************
LAST MODIFIED : 16 March 2001

DESCRIPTION :
Returns a static string describing each <enumerator_value>, suitable for file
and command parsing and for display in menus. Note that a pointer to a literal
string will be returned so it must not be deallocated!
Must always write the definition of this function for each enumeration as the
string given for each value will usually be custom. Note it must return a NULL
string for unrecognized enumerator values WITHOUT an error message.
==============================================================================*/

#if ! defined (SHORT_NAMES)
#define ENUMERATOR_GET_VALID_STRINGS( enumerator_type ) \
	enumerator_get_valid_strings_ ## enumerator_type
#else
#define ENUMERATOR_GET_VALID_STRINGS( enumerator_type ) egvs_ ## enumerator_type
#endif

#define PROTOTYPE_ENUMERATOR_GET_VALID_STRINGS_FUNCTION( enumerator_type ) \
const char **ENUMERATOR_GET_VALID_STRINGS(enumerator_type)( \
	int *number_of_valid_strings, \
	ENUMERATOR_CONDITIONAL_FUNCTION(enumerator_type) conditional_function, \
	void *user_data)
/*******************************************************************************
LAST MODIFIED : 16 March 2001

DESCRIPTION :
Returns an allocated array containing the ENUMERATOR_STRINGs for all valid
enumerator values in the enumerator_type. The optional <conditional_function>
and <user_data> allow reduced selections of strings to be returned.
Note it is up to the calling function to deallocate the returned array, but the
strings it contains must NOT be deallocated.
==============================================================================*/

#if ! defined (SHORT_NAMES)
#define STRING_TO_ENUMERATOR( enumerator_type ) \
	string_to_enumerator_ ## enumerator_type
#else
#define STRING_TO_ENUMERATOR( enumerator_type ) ste_ ## enumerator_type
#endif

#define PROTOTYPE_STRING_TO_ENUMERATOR_FUNCTION( enumerator_type ) \
int STRING_TO_ENUMERATOR(enumerator_type)(const char *enumerator_string, \
	enum enumerator_type *enumerator_value_address)
/*******************************************************************************
LAST MODIFIED : 16 March 2001

DESCRIPTION :
If <enumerator_string> matches the ENUMERATOR_STRING(enumerator_type) for any
enumerator_value, returns the enumerator_value in *<enumerator_value_address>,
with a true return_code. Otherwise a false return_code is returned, without 
an error message, for the calling function to handle.
==============================================================================*/

#define ENUMERATOR_TYPE_CLASS(enumerator_type) enumerator_class_ ## enumerator_type
/***************************************************************************** \
LAST MODIFIED : 8 March 2007 \
\
DESCRIPTION : \
Wraps the existing Enumerator functionality and types into a class. \
==============================================================================*/
#define DEFINE_ENUMERATOR_TYPE_CLASS(enumerator_type) \
class ENUMERATOR_TYPE_CLASS(enumerator_type) \
{\
public: \
	typedef enum enumerator_type Enumerator_type; \
	typedef ENUMERATOR_CONDITIONAL_FUNCTION(enumerator_type) Conditional_function; \
\
   ENUMERATOR_TYPE_CLASS(enumerator_type)() \
	{ \
	} \
\
	inline const char* value_to_string(enum enumerator_type enumerator_value) \
	{ \
		return ENUMERATOR_STRING(enumerator_type)(enumerator_value); \
	} \
\
   inline const char **get_valid_strings(int *number_of_valid_strings, \
		ENUMERATOR_CONDITIONAL_FUNCTION(enumerator_type) conditional_function, \
		void *user_data) \
	{ \
		return ENUMERATOR_GET_VALID_STRINGS(enumerator_type)(number_of_valid_strings, \
			conditional_function, user_data); \
	} \
\
	inline int string_to_value(const char *string, enum enumerator_type *enumerator_value) \
	{ \
		return STRING_TO_ENUMERATOR(enumerator_type)(string, enumerator_value);	\
	} \
\
}; /* ENUMERATOR_TYPE_CLASS(enumerator_type) */

#define PROTOTYPE_ENUMERATOR_FUNCTIONS( enumerator_type ) \
DECLARE_ENUMERATOR_CONDITIONAL_FUNCTION_TYPE(enumerator_type); \
PROTOTYPE_ENUMERATOR_GET_VALID_STRINGS_FUNCTION(enumerator_type); \
PROTOTYPE_ENUMERATOR_STRING_FUNCTION(enumerator_type); \
PROTOTYPE_STRING_TO_ENUMERATOR_FUNCTION(enumerator_type);

#endif /* !defined (ENUMERATOR_H) */
