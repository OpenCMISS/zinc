/******************************************************************************
FILE : mystring.c

LAST MODIFIED : 22 April 2004

DESCRIPTION :
Function definitions for some general purpose string functions.
???DB.  A merging of Greg's and my string.c 's
???DB.  Needs some more tidying up and rationalization.
???DB.  Need to tidy up writing error messages.
=============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cctype>

#include "general/debug.h"
#include "general/mystring.h"
/*???DB.  Would like to move memory management out of Greg's debug.h */
/*#include "mymemory.h"*/
#include "general/message.h"

/*
Functions
---------
*/

void cvt_string_to_ints(char *string, int array[100], int *number)
/*******************************************************************************
DESCRIPTION:
	Puts a string of delimited integers into an array.
==============================================================================*/
{
	char *stringptr, *ptr;

	*number = 0;
	stringptr = string;
	ptr = stringptr;
	while(*stringptr)                     /* step thru string of numbers */
	{
		if(!isdigit(*stringptr))            /* if it is a delimiter ...    */
		{
			*stringptr = 0;
			if(ptr != stringptr)
			{
				array[*number] = atoi(ptr);     /* ... create an integer       */
				(*number)++;
			}
			ptr = stringptr + 1;
		}
		stringptr++;
	}
	array[*number] = atoi(ptr);
	(*number)++;

	return;
} /* cvt_string_to_ints */

int is_abbrev(char *short_string,char *long_string,int min_length)
/*******************************************************************************
LAST MODIFIED : 27 December 1995

DESCRIPTION :
Does a case insensitive comparison to see if the <short_string> is an
abbreviation of the <long_string>.
==============================================================================*/
{
	char *long_c,*short_c;
	int return_code;

	ENTER(is_abbrev);
	if ((short_c=short_string)&&(long_c=long_string)&&(min_length>0))
	{
		if ((int)strlen(short_string)>=min_length)
		{
			return_code=1;
			while ((1==return_code)&&(*short_c)&&(*long_c))
			{
				if ((((*short_c>='a')&&(*short_c<='z')) ? (*short_c)-32 : *short_c)==
					(((*long_c>='a')&&(*long_c<='z')) ? (*long_c)-32 : *long_c))
				{
					short_c++;
					long_c++;
				}
				else
				{
					return_code=0;
				}
			}
		}
		else
		{
			return_code=0;
		}
	}
	else
	{
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* is_abbrev */

char *remove_before_first(char *string, char search)
/*******************************************************************************
DESCRIPTION:
==============================================================================*/
{
	char *ptr;

	ptr = string;
	/* Step forward until find the search character */
	while((*ptr != search) && (*ptr))
		ptr++;
	/*
	* If "ptr" is valid then we have found the search character, so return
	* a pointer to the next character.  Otherwise the search character was
	* not found, so return the whole string.
	*/
	if(*ptr)
	{
		ptr++;
		return(ptr);
	}
	else
		return(string);

} /* remove_before_first */


char *remove_before_last(char *string, char search)
/*******************************************************************************
DESCRIPTION:
==============================================================================*/
{
	char *ptr;

	ptr = string;
	/* Point to last character of the string */
	while(*ptr)
		ptr++;
	ptr--;
	/*
	* Step back until the end of string is at the search character or until
	* we pass the beginning of the string.  Return the following character.
	*/
	while(*ptr != search && ptr >= string)
		ptr--;
	ptr++;

	return(ptr);
} /* remove_before_last */


char *remove_after_first(char *string, char search)
/*******************************************************************************
DESCRIPTION:
==============================================================================*/
{
	char *ptr;

	/*
	* Find the first occurance of "search", and set the following character to
	* a null (terminating the string) and return the string.
	*/
	ptr = string;
	while((*ptr != search) && (*ptr))
		ptr++;
	*ptr = 0;

	return(string);
} /* remove_after_first */


char *remove_after_last(char *string, char search)
/*******************************************************************************
DESCRIPTION:
==============================================================================*/
{
	char *ptr;

	ptr = string;
	/* Point to last character of the string */
	while(*ptr)
		ptr++;
	ptr--;
	/*
	* Step back until the end of string is at the search character
	*/
	while(*ptr != search && ptr >= string)
		ptr--;
	/*
	* Terminate the string if we have found "search"
	*/
	if(ptr >= string)
		*ptr = 0;

	return(string);
} /* remove_after_last */

void trim_string_in_place(char *stringIn)
{
	if (!stringIn)
		return;
	char *source = stringIn;
	while (isspace(*source))
		++source;
	char *end = source + strlen(source) - 1;
	while ((end > source) && (isspace(*end)))
		--end;
	if (source == stringIn)
	{
		end[1] = '\0';
		return;
	}
	char *target = stringIn;
	while (source <= end)
	{
		*target = *source;
		++source;
		++target;
	}
	*target = '\0';
}

char *trim_string(char *stringIn)
{
	char *copy,*end,*start;
	int length;

	ENTER(trim_string);
	if (stringIn)
	{
		start= stringIn;
		end=start+strlen(stringIn)-1;
		/* remove leading white space */
		while ((start<=end)&&isspace(*start))
		{
			start++;
		}
		/* remove trailing white space */
		while ((start<=end)&&isspace(*end))
		{
			end--;
		}
		length=end-start+1;
		if (ALLOCATE(copy,char,length+1))
		{
			if (length>0)
			{
				strncpy(copy,start,length);
			}
			copy[length]='\0';
		}
		else
		{
			display_message(ERROR_MESSAGE,"trim_string.  Insufficient memory");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"trim_string.  Missing string");
		copy=(char *)NULL;
	}
	LEAVE;

	return (copy);
} /* trim_string */

char *upcase(char *string)
/*******************************************************************************
DESCRIPTION:
==============================================================================*/
{
	char *ptr;

	/*
	* For each character in the string, convert to uppercase
	*/
	ptr=string;
	while(*ptr)
	{
		*ptr = toupper(*ptr);
		ptr++;
	}

	return(string);
} /* upcase */

char *string_to_upper(char *string)
/******************************************************************************
LAST MODIFIED : 12 November 1990

DESCRIPTION :
=============================================================================*/
{
	int i;

	ENTER(string_to_upper);
	if (string)
	{
		for (i=strlen(string);i>0;)
		{
			i--;
			if (islower(string[i]))
			{
				string[i]=(char)toupper(string[i]);
			}
		}
	}
	LEAVE;

	return (string);
} /* string_to_upper */

int read_string(FILE *in,char *format,char **string_read)
/******************************************************************************
LAST MODIFIED : 17 May 1991

DESCRIPTION :
	A routine for reading in a single string.  It allocates the memory for the
string.  It uses fscanf:
1. the format string follows the format string for scanf, but is for only one %
	item and does not have the %.
2. if field width is not specified, 256 is used.
3. it reads from the stream <in>.
???DB.  What should be the return code if no characters are read (EOF) ?
=============================================================================*/
{
	int characters_read,format_len,return_code,working_string_len;
	char *working_format,*working_string;

	ENTER(read_string);
	/* check for valid parameters */
	if (in&&format&&string_read)
	{
		format_len=strlen(format);
		if (!strcmp(format,"s"))
		/* format is s */
		{
			return_code=1;
		}
		else
		{
			if ((format[0]=='[')&&(format[format_len-1]==']'))
			/* format is [ */
			{
				return_code=1;
			}
			else
			{
/*        print_message(1,"read_string.  Format is not s or [");*/
				display_message(WARNING_MESSAGE,"read_string.  Format is not s or [");
				return_code=0;
			}
		}
		if (return_code)
		{
			/* construct a format for reading the string and counting the characters
				read */
			if (ALLOCATE(working_format,char,format_len+6)&&
				ALLOCATE(working_string,char,1))
			{
				strcpy(working_format,"%80");
				strcat(working_format,format);
				strcat(working_format,"%n");
				/* read in string, allocating extra memory as required */
				working_string_len=0;
				working_string[0]='\0';
				characters_read=80;
				while (return_code&&!feof(in)&&(characters_read==80))
				{
					working_string_len += 80;
					if (REALLOCATE(working_string,working_string,char,
						working_string_len+1))
					{
						characters_read=0;
						if (EOF == fscanf(in,working_format,working_string+working_string_len-80,
							&characters_read))
						{
							display_message(WARNING_MESSAGE,
								"read_string.  Error reading from string");
							return_code = 0;
						}
					}
					else
					{
/*            print_message(1,*/
						display_message(WARNING_MESSAGE,
							"read_string.  Could not allocate memory for string");
						return_code=0;
					}
				}
				if (return_code)
				{
					if (working_string)
					{
						REALLOCATE(*string_read,working_string,char,
							strlen(working_string)+1);
					}
					else
					{
						*string_read=(char *)NULL;
					}
				}
				else
				{
					if (working_string)
					{
						DEALLOCATE(working_string);
					}
				}
				DEALLOCATE(working_format);
			}
			else
			{
/*        print_message(1,*/
				display_message(WARNING_MESSAGE,
					"read_string.  Could not allocate memory for working format");
				return_code=0;
			}
		}
	}
	else
	{
/*    print_message(1,"read_string.  Invalid argument(s)");*/
		display_message(WARNING_MESSAGE,"read_string.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return(return_code);
} /* read_string */

int assign_empty_string(char **string)
/******************************************************************************
LAST MODIFIED : 17 May 1992

DESCRIPTION :
	A routine for assigning an empty string.  Assumes that if the string has
been previously assigned, it was done with mymalloc.
=============================================================================*/
{
	int return_code;

	ENTER(assign_empty_string);
	/* if the string has been previously assigned */
	DEALLOCATE(*string);
	/* allocate memory for an empty string */
	ALLOCATE(*string,char,1);
	if (*string)
	{
		/* assign the empty string */
		*string[0]='\0';
		return_code=1;
	}
	else
	{
/*    return_code=print_message(1,"assign_empty_string.  Insufficient memory");*/
		return_code=
			display_message(WARNING_MESSAGE,"assign_empty_string.  Insufficient memory");
		return_code=0;
	}
	LEAVE;

	return(return_code);
} /* assign_empty_string */

char *remove_leading_trailing_blanks(char *in_string)
/******************************************************************************
LAST MODIFIED : 17 May 1992

DESCRIPTION :
=============================================================================*/
{
	int string_length,offset,i;
	char *out_string=NULL,stop;

	ENTER(remove_leading_trailing_blanks);
	/* strip trailing blanks */
	string_length=strlen(in_string)-1;
	stop=0;
	while ((string_length>=0)&&!stop)
	{
		if (in_string[string_length]==' ')
		{
			string_length--;
		}
		else
		{
			stop=1;
		}
	}
	string_length++;
	/* strip leading blanks */
	offset=0;
	stop=0;
	while ((offset<string_length)&&!stop)
	{
		if (in_string[offset]==' ')
		{
			offset++;
		}
		else
		{
			stop=1;
		}
	}
	string_length=string_length-offset;
	/* set up storage to for out_string */
	if (ALLOCATE(out_string,char,string_length+1))
	{
		/* copy in_string */
		for (i=0;i<string_length;i++)
		{
			out_string[i]=in_string[i+offset];
		}
		out_string[i]='\0';
	}
	LEAVE;

	return(out_string);
} /* remove_leading_trailing_blanks */

int append_string(char **string1,const char *string2,int *error)
/*******************************************************************************
LAST MODIFIED : 2 December 1998

DESCRIPTION :
Concatenates <string2> on to the end of <*string1> by reallocating <*string1> to
fit. <*string1> may start off as NULL or an existing allocated string.
If <*error> is 1, nothing is done; if an error occurs, <*error> is set to 1 and
<*string1> is deallocated. Repeated calls to this function after an error has
occurred thus do not add to the string, and do not result in further errors.
It is up to the calling function to deallocate the returned string.
==============================================================================*/
{
	char *new_string;
	int current_length;

	ENTER(append_string);
	if (string1&&string2&&error)
	{
		if (!(*error))
		{
			if (*string1)
			{
				current_length=strlen(*string1);
			}
			else
			{
				current_length=0;
			}
			if (REALLOCATE(new_string,*string1,char,current_length+strlen(string2)+1))
			{
				if (*string1)
				{
					strcat(new_string,string2);
				}
				else
				{
					strcpy(new_string,string2);
				}
				*string1=new_string;
			}
			else
			{
				display_message(ERROR_MESSAGE,"append_string.  Could not reallocate");
				*error=1;
				if (*string1)
				{
					DEALLOCATE(*string1);
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"append_string.  Invalid argument(s)");
		*error=1;
	}
	LEAVE;

	return (!(*error));
} /* append_string */

int has_suffix(char *string)
/*******************************************************************************
LAST MODIFIED : 25 November 1999

DESCRIPTION :
Returns true if <string> has a suffix present.
==============================================================================*/
{
	char *ptr;
	int has_suffix;

	ENTER(has_suffix);
	has_suffix = 0;
	ptr = string;

	/* Point to last character of the string */
	while(*ptr)
		ptr++;
	ptr--;
	/*
	* Step back until either a dot or slash is encountered
	*/
	while(*ptr != '.' && *ptr != '/' && *ptr != '\\'  && ptr > string)
		ptr--;

	/* if we found a dot before a slash or the beginning of the string
		there is a suffix present */
	if(*ptr == '.' && ptr > string)
		has_suffix = 1;

	LEAVE;
	return(has_suffix);
} /* has_suffix */

int check_suffix(char **string, const char *suffix)
/*******************************************************************************
LAST MODIFIED : 22 April 2004

DESCRIPTION :
Checks to see if the filename contains a suffix and if not appends the default
<suffix>
==============================================================================*/
{

	char *new_string;
	int return_code;

	ENTER(check_suffix);
	if (string&&(*string)&&suffix)
	{
		// check to see if string has a suffix
		// note we purposely do not check to see if the suffix matches the default
		if (has_suffix(*string)) {
			return_code = 1;
		}
		else {
			
			if (REALLOCATE(new_string, *string, char, strlen(*string)
				+strlen(suffix)+1))
			{
				*string = new_string;
				strcat(*string, suffix);
				return_code = 1;
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"compare_suffix.  Unable to REALLOCATE string");
				return_code = 0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"compare_suffix.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* compare_suffix */

char *duplicate_string(const char *source_string)
/*******************************************************************************
LAST MODIFIED : 2 December 1998

DESCRIPTION :
Returns an allocated copy of <source_string>, or NULL in case of error.
==============================================================================*/
{
	char *copy_of_string;

	ENTER(duplicate_string);
	if (source_string)
	{
		if (ALLOCATE(copy_of_string, char, strlen(source_string) + 1))
		{
			strcpy(copy_of_string,source_string);
		}
		else
		{
			display_message(ERROR_MESSAGE,"duplicate_string.  Not enough memory");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"duplicate_string.  Invalid argument(s)");
		copy_of_string=(char *)NULL;
	}
	LEAVE;

	return (copy_of_string);
} /* duplicate_string */

int string_matches_without_whitespace(char *input_string,const char *match_string)
/*******************************************************************************
LAST MODIFIED : 2 September 1999

DESCRIPTION :
Returns true if all the non-whitespace characters in <input_string> match those
in <match_string>. Whitespace characters (space,tab) are only allowed in
<input_string> where they appear in match_string.
==============================================================================*/
{
	char *s1;
	const char *s2;
	int return_code;

	ENTER(string_matches_without_whitespace);
	if ((s1=input_string)&&(s2=match_string))
	{
		return_code=1;
		while (return_code&&(*s1)&&(*s2))
		{
			if ((' '== (*s2))||('\t'== (*s2)))
			{
				while ((' '== (*s1))||('\t'== (*s1)))
				{
					s1++;
				}
				while ((' '== (*s2))||('\t'== (*s2)))
				{
					s2++;
				}
			}
			if ((*s1) != (*s2))
			{
				return_code=0;
			}
			if (*s1)
			{
				s1++;
			}
			if (*s2)
			{
				s2++;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"string_matches_without_whitespace.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* string_matches_without_whitespace */

static int reduce_fuzzy_string(char *reduced_string,const char *string)
/*******************************************************************************
LAST MODIFIED : 16 September 1998

DESCRIPTION :
Copies <string> to <reduced_string> converting to upper case and removing
whitespace, -'s and _'s.
???RC Removed filtering of dash and underline.
==============================================================================*/
{
	char *destination;
	const char *source;
	int return_code;

	ENTER(reduce_fuzzy_string);
	if ((source=string)&&(destination=reduced_string))
	{
		while (*source)
		{
			/* remove whitespace, -'s and _'s */
			/*???RC don't know why you want to exclude - and _. I had a case where
				I typed gfx create fibre_ (short for fibre_field) and it thought it was
				ambiguous since there is also a gfx create fibres commands. */
			if (!isspace(*source)/*&&(*source!='-')&&(*source!='_')*/)
			{
				*destination=toupper(*source);
				destination++;
			}
			source++;
		}
		/* terminate the string */
		*destination='\0';
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"reduce_fuzzy_string.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* reduce_fuzzy_string */

int fuzzy_string_compare(const char *first,const char *second)
/*******************************************************************************
LAST MODIFIED : 30 August 2000

DESCRIPTION :
This is a case insensitive compare disregarding certain characters (whitespace,
dashes and underscores).  For example, "Ambient Colour" matches the following:

"AMBIENT_COLOUR", "ambient_colour", "ambientColour", "Ambient_Colour",
"ambient-colour", "AmBiEnTcOlOuR", "Ambient-- Colour"

and a large set of even louder versions.

Both strings are first reduced, which removes whitespace and converts to upper
case. Returns 1 iff the reduced second string is at least as long as the
reduced first string and starts with the characters in the first.
==============================================================================*/
{
	char *first_reduced,*second_reduced;
	int compare_length,first_length,return_code,second_length;

	ENTER(fuzzy_string_compare);
	if (first&&second)
	{
		/*???Edouard.  Malloc()ing and free()ing space for every string compare is
			perhaps going to eat a few too many cycles in the final analysis. I think
			that the best idea would be to at some point in the future recode the
			following to compare the strings 'live' by reducing and skipping the chars
			one by one. I've done this before, but it's generally not all that
			clean-looking, so I'm trying to write a simple one first off */
		/* allocate memory */
		if (ALLOCATE(first_reduced,char,strlen(first)+1)&&
			ALLOCATE(second_reduced,char,strlen(second)+1))
		{
			/* reduce strings */
			if (reduce_fuzzy_string(first_reduced,first)&&
				reduce_fuzzy_string(second_reduced,second))
			{
				first_length=strlen(first_reduced);
				second_length=strlen(second_reduced);
				/* first reduced string must not be longer than second */
				if (first_length <= second_length)
				{
					compare_length=first_length;
					if (strncmp(first_reduced,second_reduced,compare_length))
					{
						return_code=0;
					}
					else
					{
						return_code=1;
					}
				}
				else
				{
					return_code=0;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,"fuzzy_string_compare.  Error reducing");
				return_code=0;
			}
			DEALLOCATE(first_reduced);
			DEALLOCATE(second_reduced);
		}
		else
		{
			DEALLOCATE(first_reduced);
			display_message(ERROR_MESSAGE,
				"fuzzy_string_compare.  Insufficient memory");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"fuzzy_string_compare.  Invalid arguments");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* fuzzy_string_compare */

int fuzzy_string_compare_same_length(const char *first,const char *second)
/*******************************************************************************
LAST MODIFIED : 14 August 1998

DESCRIPTION :
Same as fuzzy_string_compare except that the two reduced strings must be the
same length.
==============================================================================*/
{
	char *first_reduced,*second_reduced;
	int return_code;

	ENTER(fuzzy_string_compare_same_length);
	if (first&&second)
	{
		if (ALLOCATE(first_reduced,char,strlen(first)+1)&&
			ALLOCATE(second_reduced,char,strlen(second)+1))
		{
			/* reduce strings */
			if (reduce_fuzzy_string(first_reduced,first)&&
				reduce_fuzzy_string(second_reduced,second))
			{
				if (0==strcmp(first_reduced,second_reduced))
				{
					return_code=1;
				}
				else
				{
					return_code=0;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"fuzzy_string_compare_same_length.  Error reducing");
				return_code=0;
			}
			DEALLOCATE(first_reduced);
			DEALLOCATE(second_reduced);
		}
		else
		{
			DEALLOCATE(first_reduced);
			display_message(ERROR_MESSAGE,
				"fuzzy_string_compare_same_length.  Insufficient memory");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"fuzzy_string_compare_same_length.  Invalid arguments");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* fuzzy_string_compare_same_length */

int make_valid_token(char **token_address)
{
	char *new_token, *letter;
	int number_of_escapes, old_length, return_code, special_chars;

	ENTER(make_valid_token);
	if (token_address && (letter = *token_address))
	{
		return_code = 1;
		/* work out if string contains special characters */
		special_chars = 0;
		if ((0 == strlen(*token_address)) || (0 == strcmp(*token_address, "/")))
		{
			special_chars++;
		}
		number_of_escapes = 0;
		while (*letter)
		{
			if (isspace(*letter) || (',' == *letter) ||
				(';' == *letter) || ('=' == *letter) ||
				('#' == *letter))
			{
				special_chars++;
			}
			else if (('\"' == *letter) || ('\'' == *letter) ||
				('\\' == *letter) || ('$' == *letter))
			{
				special_chars++;
				number_of_escapes++;
			}
			letter++;
		}
		if (special_chars)
		{
			old_length = strlen(*token_address);
			if (REALLOCATE(new_token, *token_address, char,
				old_length + number_of_escapes + 3))
			{
				*token_address = new_token;
				letter = *token_address + old_length - 1;
				new_token = letter + number_of_escapes + 3;
				/* zero at end of string */
				*new_token = '\0';
				new_token--;
				/* final quote */
				*new_token = '\"';
				new_token--;
				while (letter >= *token_address)
				{
					*new_token = *letter;
					new_token--;
					if (('\"' == *letter) || ('\'' == *letter) ||
						('\\' == *letter) || ('$' == *letter))
					{
						/* add escape character \ */
						*new_token = '\\';
						new_token--;
					}
					letter--;
				}
				/* opening quote */
				*new_token = '\"';
			}
			else
			{
				display_message(ERROR_MESSAGE, "make_valid_token.  Not enough memory");
				return_code = 0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE, "make_valid_token.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* make_valid_token */

int getNumericalFormatStringSize(const char *formatString, int valuesCount)
{
	if (!formatString)
		return 0;
	int len = strlen(formatString);
	int size = len + 1;
	int numValues = 0;
	for (int i = 0; i < len; ++i)
	{
		if (formatString[i] == '%')
		{
			++i;
			if (formatString[i] == '%')
			{
				continue;
			}
			const char *length = formatString + i;
			const char *precision = 0;
			const char *c = length;
			while (isdigit(*c) || (*c=='+') || (*c=='-') || (*c=='.'))
			{
				++c;
				if (!precision && (*c=='.'))
				{
					precision = c;
				}
			}
			if ( (*c == 'e') || (*c == 'E')
				|| (*c == 'f') || (*c == 'F')
				|| (*c == 'g') || (*c == 'G'))
			{
				size += abs(atoi(length));
				if (precision)
				{
					size += atoi(precision);
				}
				size += 14; // "-0.000000e+000"
				if ((*c == 'f') || (*c == 'F'))
				{
					size += 309; // max double exponent!
				}
				++numValues;
			}
			else
			{
				return 0;
			}
		}
	}
	return (numValues == valuesCount) ? size : 0;
}

bool labels_match(const char *label1, const char *label2)
{
	return ((label1 == label2)
		|| ((0 == label1) && (label2[0] == '\0'))
		|| ((0 == label2) && (label1[0] == '\0'))
		|| (label1 && label2 && (0 == strcmp(label1, label2))));
}
