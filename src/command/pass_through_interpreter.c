/*******************************************************************************
FILE : pass_through_interpreter.c

LAST MODIFIED : 18 April 2007

DESCRIPTION :
Just passes commands through without interpretation
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if defined (BUILD_WITH_CMAKE)
#include "configure/cmgui_configure.h"
#endif /* defined (BUILD_WITH_CMAKE) */

#define PCRE_STATIC
#include <pcre.h>
#include "command/pass_through_interpreter.h"
#include "general/debug.h"

#if defined (USE_PARAMETER_ON)
#define USE_PARAMETER_PTI(dummy) use_parameter_pti(0,dummy)

void use_parameter_pti(int dummy, ... )
/*******************************************************************************
LAST MODIFIED : 26 November 2001

DESCRIPTION :
Definition of function which is called in the development stage (when
USE_PARAMETER_ON is defined) to swallow unused parameters to functions which
would otherwise cause compiler warnings. For example, parameter <dummy_void>
is swallowed with the call USE_PARAMETER_PTI(dummy_void); at the start of function.
==============================================================================*/
{
	if (dummy)
	{
	}
} /* use_parameter_pti */
#else /* defined (USE_PARAMETER_ON) */
#define USE_PARAMETER_PTI(dummy)
#endif /* defined (USE_PARAMETER_ON) */

struct Interpreter 
/*******************************************************************************
LAST MODIFIED : 18 April 2007

DESCRIPTION :
Internal data storage for this interpreter.
==============================================================================*/
{
	int number_of_variables;
	char **variable_names, **variable_values;
	pcre *variable_assignment_regex;
	pcre *variable_substitute_regex;
}; /* struct Interpreter */

char *interpreter_strndup(const char *string, const unsigned int maximum_length)
{
	char *return_string;
	unsigned int length = 0;
	while (('\0' != string[length]) && (length < maximum_length))
	{
		length++;
	}
	return_string = malloc(length + 1);
	memcpy(return_string, string, length);
	return_string[length] = '\0';
	return (return_string);
}

void create_interpreter_(int argc, char **argv, const char *initial_comfile, 
	 struct Interpreter **interpreter, int *status)
/*******************************************************************************
LAST MODIFIED : 16 January 2007

DESCRIPTION:
Creates the interpreter for processing commands.
==============================================================================*/
{
	const char *pcre_error_string;
	int pcre_error_offset, return_code;

  USE_PARAMETER_PTI(argc);
  USE_PARAMETER_PTI(argv);
  USE_PARAMETER_PTI(initial_comfile);

  return_code = 1;

  if ( (*interpreter = (struct Interpreter *)malloc(sizeof (struct Interpreter))) )
  {

 	  (*interpreter)->number_of_variables = 0; 
 	  (*interpreter)->variable_names = (char **)NULL; 
 	  (*interpreter)->variable_values = (char **)NULL; 

	  {
		  char assignment_regex[] = "^\\s*\\$([\\w][\\w\\d_]*)\\s*"
			  "=\\s*((\\d+)|[\"']([^\"']*)[\"\'])\\s*;?\\s*$";
		  if (!((*interpreter)->variable_assignment_regex = 
			  pcre_compile(assignment_regex,
			  /*options*/0, &pcre_error_string, &pcre_error_offset,
			  /*table_ptr*/(unsigned char *)NULL)))
		  {
			  printf("PCRE REGEX error %s\n", pcre_error_string);
		  }
#if defined (DEBUG_CODE)
		  printf("Assignment regex: %s\n", assignment_regex);
#endif /* defined (DEBUG_CODE) */
	  }

	  {
		  char substitute_regex[] = "\\$(\\w[\\w\\d_]*|{\\w[\\w\\d_]*})";
		  if (!((*interpreter)->variable_substitute_regex =
			  pcre_compile(substitute_regex,
			  /*options*/0, &pcre_error_string, &pcre_error_offset,
			  /*table_ptr*/(unsigned char *)NULL)))
		  {
			  printf("PCRE REGEX error %s\n", pcre_error_string);
		  }
#if defined (DEBUG_CODE)
		  printf("Substitute regex: %s\n", substitute_regex);
#endif /* defined (DEBUG_CODE) */
	  }
  }
  else
  {
	  *interpreter = (struct Interpreter *)NULL;
	  return_code = 0;
  }
  
  *status = return_code;
}

void destroy_interpreter_(struct Interpreter *interpreter, int *status)
/*******************************************************************************
LAST MODIFIED : 16 January 2007

DESCRIPTION:
==============================================================================*/
{
	int i;

	 if (interpreter)
	 {
		 pcre_free(interpreter->variable_assignment_regex);
		 pcre_free(interpreter->variable_substitute_regex);

		 if (interpreter->variable_names && 
			 interpreter->variable_values)
		 {
			 for (i = 0 ; i < interpreter->number_of_variables ; i++)
			 {
				 free(interpreter->variable_names[i]);
				 free(interpreter->variable_values[i]);
			 }
			 free(interpreter->variable_names);
			 free(interpreter->variable_values);
		 }
		 
		 free (interpreter);
		 *status = 1;
	 }
	 else
	 {
		 *status = 0;
	 }
}

void interpreter_set_display_message_function_(struct Interpreter *interpreter, 
	 Interpreter_display_message_function *function, int *status)
/*******************************************************************************
LAST MODIFIED : 16 January 2007

DESCRIPTION:
Sets the function that will be called whenever the Interpreter wants to report
information.
==============================================================================*/
{
	USE_PARAMETER_PTI(interpreter);
	USE_PARAMETER_PTI(function);
	*status = 1;
}

void redirect_interpreter_output_(struct Interpreter *interpreter, int *status)
/*******************************************************************************
LAST MODIFIED : 16 January 2007

DESCRIPTION:
This redirects the output from stdout to a pipe so that the handle_output
routine can write this to the command window.
==============================================================================*/
{
	USE_PARAMETER_PTI(interpreter);
  *status = 1;
}

void interpret_command_(struct Interpreter *interpreter, const char *command_string, 
	 void *user_data, int *quit,
  execute_command_function_type execute_command_function, int *status)
/*******************************************************************************
LAST MODIFIED : 16 January 2007

DESCRIPTION:
Takes a <command_string>, replaces simple variables and sends the command
back to the main program.
==============================================================================*/
{
	char *current_string, *new_name, *new_value = NULL, *temporary_string = NULL;
	int i, match, match_value_length, return_code;
	int matches[10 * 3];

	if (interpreter)
	{
		if (0 < pcre_exec(interpreter->variable_assignment_regex,
				/*extra_data*/NULL, command_string, 
				strlen(command_string), /*offset*/0, /*options*/0,
				matches, sizeof(matches)/sizeof(int)))
		{
#if defined (DEBUG_CODE)
			printf(" VARIABLE DEF MATCH %d %d\n%s\n",
				matches[0], matches[1], command_string);
#endif /* defined (DEBUG_CODE) */
			if ((interpreter->variable_names = (char **)realloc(
					 interpreter->variable_names, 
					 (interpreter->number_of_variables + 1) * sizeof(char *)))
				&& (interpreter->variable_values = (char **)realloc(
					 interpreter->variable_values, 
					 (interpreter->number_of_variables + 1) * sizeof(char *))))
			{
				new_name = interpreter_strndup(command_string + matches[2],
					matches[3] - matches[2]);

				if (-1 != matches[6])
				{
					new_value = interpreter_strndup(command_string + matches[6],
						matches[7] - matches[6]);
				}
				else if (-1 != matches[8])
				{
					new_value = interpreter_strndup (command_string + matches[8],
						matches[9] - matches[8]);
				}
				match = -1;
				for (i = 0 ; (-1 == match) &&
						  (i < interpreter->number_of_variables) ; i++)
				{
					if (!strcmp(interpreter->variable_names[i], new_name))
					{
						free(new_name);
						free(interpreter->variable_values[i]);
						interpreter->variable_values[i] = new_value;
						match = i;
						/* Won't worry about extra pointer on arrays as it
							will get used next time */
					}
				}
				if (-1 == match)
				{
					interpreter->variable_names[interpreter->number_of_variables] =
						new_name;
					interpreter->variable_values[interpreter->number_of_variables] =
						new_value;
#if defined (DEBUG_CODE)
					printf ("  new variable %s = %s\n", new_name, new_value);
#endif /* defined (DEBUG_CODE) */
					interpreter->number_of_variables++;
				}
			}
			else
			{
				printf("interpret_command: malloc failed\n");
			}
			return_code = 1;
		}
		else
		{
			current_string = (char *)(command_string);
			return_code = 1;
			while (return_code && (0 < pcre_exec(interpreter->variable_substitute_regex,
				/*extra_data*/NULL, current_string, 
				strlen(current_string), /*offset*/0, /*options*/0,
				matches, sizeof(matches)/sizeof(int))))
			{
#if defined (DEBUG_CODE)
				printf(" MATCH %d %d %s\n",
				       matches[0], matches[1], current_string);
#endif /* defined (DEBUG_CODE) */
				match = -1;
				for (i = 0 ; (match == -1) && (i < interpreter->number_of_variables) ; i++)
				{
#if defined (DEBUG_CODE)
					printf("Comparing %d characters %s  %s\n",
					       (int)strlen(interpreter->variable_names[i]),
						command_string + matches[0] + 1,
						interpreter->variable_names[i]);
#endif /* defined (DEBUG_CODE) */
					if (!strncmp(command_string + matches[0] + 1,
							interpreter->variable_names[i],
							strlen(interpreter->variable_names[i])))
					{
						match = i;
					}
				}
				if (-1 != match)
				{
					match_value_length = strlen(interpreter->variable_values[match]);
					temporary_string = (char *)realloc(temporary_string,
						strlen(current_string) + match_value_length);
					if (current_string == command_string)
					{
						/* Copying from the command_string */
						memcpy(temporary_string, current_string, matches[0]);
						memcpy(
							temporary_string + matches[0] + match_value_length,
							current_string + matches[1],
							strlen(current_string + matches[1]) + 1);
						current_string = temporary_string;
					}
					else
					{
						/* The location of the string may have shifted */
						current_string = temporary_string;
						/* Shift the second part of the string first in case
							we are overlapping */
						memmove(temporary_string + matches[0] +
							match_value_length,
							temporary_string + matches[1],
							strlen(temporary_string + matches[1]) + 1);
							
					}
					memcpy(current_string + matches[0],
						interpreter->variable_values[match],
						match_value_length);
				}
				else
				{
					return_code = 0;
				}
			}
			if (return_code)
			{
				(execute_command_function)(current_string, 
					user_data, quit, &return_code);
			}
			else
			{
				printf("Error sustituting variable\n");
			}
			if (temporary_string)
			{
				free(temporary_string);
			}
		}
	}
	else
	{
		 return_code=0;
	}
	
	*status = return_code;
} /* interpret_command_ */

void interpreter_evaluate_string_(struct Interpreter *interpreter, 
	 char *expression, char **result, int *status)
/*******************************************************************************
LAST MODIFIED : 16 January 2007

DESCRIPTION:
NOT_IMPLEMENTED
==============================================================================*/
{
	USE_PARAMETER_PTI(interpreter);
	USE_PARAMETER_PTI(expression);
	*result = (char *)NULL;
	*status = 0;
} /* interpreter_evaluate_string_ */

void interpreter_set_string_(struct Interpreter *interpreter, 
	 const char *variable_name, const char *value, int *status)
/*******************************************************************************
LAST MODIFIED : 16 January 2007

DESCRIPTION:
NOT_IMPLEMENTED
==============================================================================*/
{
	USE_PARAMETER_PTI(interpreter);
	USE_PARAMETER_PTI(variable_name);
	USE_PARAMETER_PTI(value);
	*status = 0;
} /* interpreter_set_string_ */

void interpreter_set_pointer_(struct Interpreter *interpreter,
	 const char *variable_name, const char *class_name, void *value,int *status)
/*******************************************************************************
LAST MODIFIED : 16 January 2007

DESCRIPTION:
NOT_IMPLEMENTED
==============================================================================*/
{
	USE_PARAMETER_PTI(interpreter);
	USE_PARAMETER_PTI(variable_name);
	USE_PARAMETER_PTI(class_name);
	USE_PARAMETER_PTI(value);
	*status = 0;
} /* interpreter_set_string_ */

