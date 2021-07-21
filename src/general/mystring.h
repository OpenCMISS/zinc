/*******************************************************************************
FILE : mystring.h

LAST MODIFIED : 29 April 2003

DESCRIPTION :
Function prototypes for some general purpose string functions.
???DB.  A merging of Greg's and my string.h 's
???DB.  Needs some more tidying up and rationalization.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (MYSTRING_H)
#define MYSTRING_H

#include <stddef.h>
#include <stdio.h>

#if defined (_MSC_VER)
/* Alias the MSVC versions of these functions to the unix names.
 * However be aware that if the MSVC version overflows it will not
 * be null terminated, so I tend to use 1 less than the lenght and
 * just add a null always.
 */
#define snprintf _snprintf
#endif // defined (_MSC_VER)

/*
Functions
---------
*/
void  cvt_string_to_ints(char *string, int array[100], int *number);

int is_abbrev(char *short_string,char *long_string,int min_length);
/*******************************************************************************
LAST MODIFIED : 18 May 1993

DESCRIPTION :
Does a case insensitive comparison to see if the <short_string> is an
abbreviation of the <long_string>.
==============================================================================*/

char *remove_before_first(char *string, char search);
char *remove_before_last(char *string, char search);
char *remove_after_first(char *string, char search);
char *remove_after_last(char *string, char search);

/** Removes leading and trailing white space from stringIn. */
void trim_string_in_place(char *stringIn);

/**
 * Returns a copy of the stringIn with the leading and trailing white space
 * removed.  NB Memory is allocated by this function for the copy.
 */
char *trim_string(const char *stringIn);

char *upcase(char *string);

char *string_to_upper(char *string);
/*******************************************************************************
LAST MODIFIED : 12 November 1990

DESCRIPTION :
==============================================================================*/

int read_string(FILE *in,char *format,char **string_read);
/*******************************************************************************
LAST MODIFIED : 6 May 1991

DESCRIPTION :
	A routine for reading in a single string.  It allocates the memory for the
string.  It uses fscanf:
1. the format string follows the format string for scanf, but is for only one %
	item and does not have the %.
2. if field width is not specified, 256 is used.
3. it reads from the stream <in>.
==============================================================================*/

int assign_empty_string(char **string);
/*******************************************************************************
LAST MODIFIED : 26 May 1990

DESCRIPTION :
	A routine for assigning an empty string.  Assumes that if the string has
been previously assigned, it was done with malloc.
==============================================================================*/

char *remove_leading_trailing_blanks(char *in_string);
/*******************************************************************************
LAST MODIFIED : 1 June 1990

DESCRIPTION :
==============================================================================*/

int append_string(char **string1,const char *string2,int *error);
/*******************************************************************************
LAST MODIFIED : 1 September 1998

DESCRIPTION :
Concatenates <string2> on to the end of <*string1> by reallocating <*string1> to
fit. <*string1> may start off as NULL or an existing allocated string.
If <*error> is 1, nothing is done; if an error occurs, <*error> is set to 1 and
<*string1> is deallocated. Repeated calls to this function after an error has
occurred thus do not add to the string, and do not result in further errors.
It is up to the calling function to deallocate the returned string.
==============================================================================*/

int check_suffix(char **string, const char *suffix);
/*******************************************************************************
LAST MODIFIED : 24 September 1998

DESCRIPTION :
Compares the file extension of the string.  If the string given has the
supplied suffix (or the universal suffix .cmiss) then the function returns
without changing anything.  Otherwise the string is REALLOCATED and the suffix
added to the end of the string.
==============================================================================*/

char *duplicate_string(const char *source_string);
/*******************************************************************************
LAST MODIFIED : 2 December 1998

DESCRIPTION :
Returns an allocated copy of <source_string>, or NULL in case of error.
==============================================================================*/

int string_matches_without_whitespace(char *input_string,const char *match_string);
/*******************************************************************************
LAST MODIFIED : 2 September 1999

DESCRIPTION :
Returns true if all the non-whitespace characters in <input_string> match those
in <match_string>. Whitespace characters (space,tab) are only allowed in
<input_string> where they appear in match_string.
==============================================================================*/

int fuzzy_string_compare(const char *first,const char *second);
/*******************************************************************************
LAST MODIFIED : 21 June 1999

DESCRIPTION :
This is a case insensitive compare disregarding certain characters (whitespace,
dashes and underscores).  For example, "Ambient Colour" matches the following:

"AMBIENT_COLOUR", "ambient_colour", "ambientColour", "Ambient_Colour",
"ambient-colour", "AmBiEnTcOlOuR", "Ambient-- Colour"

and a large set of even louder versions. The strings are compared up to the
length of the shortest of first and second.

Returns 1 if the strings match, 0 if they do not.
==============================================================================*/

int fuzzy_string_compare_same_length(const char *first,const char *second);
/*******************************************************************************
LAST MODIFIED : 14 August 1998

DESCRIPTION :
Same as fuzzy_string_compare except that the two reduced strings must be the
same length.
==============================================================================*/

/**
 * If the string pointed to by <token_address> contains any special characters
 * such that, if parsed, it would not be read in its entirety as a single token,
 * this function reallocates and redefines the string so that it is surrounded
 * by quotes. Any quotes in the original string are escaped by preceding with
 * a backslash \, and each backslash is written as "\\".
 * Special characters include token separators (whitespace|,|;|=), comment
 * characters (!/#) and the quote marks themselves ("|').
 * NOTE: the string pointed to by <token_address> must be non-static and allowed
 * to be reallocated.
 * @see extract_token().
 */
int make_valid_token(char **token_address);

/**
 * @param formatString  C-style printf format string for floating point output.
 * @param valuesCount  Number of floating point values to match, at least 1.
 * @return  Conservative estimate of size of string needed to write numbers
 * with printf using format_string. Supports format characters:
 * f, F, e, E and g, G. Includes space for trailing zero.
 * Return 0 if missing format string, unrecognised format characters or wrong
 * number of values.
 */
int getNumericalFormatStringSize(const char *formatString, int valuesCount);

/** String equality that handles null strings and empty strings */
bool labels_match(const char *label1, const char *label2);

#endif /* !defined (MYSTRING_H) */
