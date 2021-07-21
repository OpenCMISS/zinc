/*******************************************************************************
FILE : pass_through_interpreter.h

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

enum Message_type
/*******************************************************************************
LAST MODIFIED : 18 April 2007

DESCRIPTION :
The different message types.
==============================================================================*/
{
	ERROR_MESSAGE,
	INFORMATION_MESSAGE,
	WARNING_MESSAGE
}; /* enum Message_type */

typedef void (*execute_command_function_type)(const char *, void *, int *, int *);

typedef int (Interpreter_display_message_function)(enum Message_type message_type,
	const char *format, ... );

struct Interpreter;
/*******************************************************************************
LAST MODIFIED : 18 April 2007

DESCRIPTION :
Internal data storage for this interpreter.
==============================================================================*/

void create_interpreter_(int argc, char **argv, const char *initial_comfile, 
	struct Interpreter **interpreter, int *status);
/*******************************************************************************
LAST MODIFIED : 16 January 2007

DESCRIPTION:
Creates the interpreter for processing commands.
==============================================================================*/

void destroy_interpreter_(struct Interpreter *interpreter, int *status);
/*******************************************************************************
LAST MODIFIED : 16 January 2007

DESCRIPTION:
==============================================================================*/

void interpreter_set_display_message_function_(struct Interpreter *interpreter, 
	Interpreter_display_message_function *function, int *status);
/*******************************************************************************
LAST MODIFIED : 16 January 2007

DESCRIPTION:
Sets the function that will be called whenever the Interpreter wants to report
information.
==============================================================================*/

void redirect_interpreter_output_(struct Interpreter *interpreter, int *status);
/*******************************************************************************
LAST MODIFIED : 16 January 2007

DESCRIPTION:
This redirects the output from stdout to a pipe so that the handle_output
routine can write this to the command window.
==============================================================================*/

void interpret_command_(struct Interpreter *interpreter, const char *command_string, 
	 void *user_data, int *quit,
	execute_command_function_type execute_command_function, int *status);
/*******************************************************************************
LAST MODIFIED : 16 January 2007

DESCRIPTION:
Takes a <command_string>, replaces simple variables and sends the command
back to the main program.
==============================================================================*/

void interpreter_evaluate_string_(struct Interpreter *interpreter, 
	char *expression, char **result, int *status);
/*******************************************************************************
LAST MODIFIED : 16 January 2007

DESCRIPTION:
NOT_IMPLEMENTED
==============================================================================*/

void interpreter_set_string_(struct Interpreter *interpreter, 
	const char *variable_name, const char *value, int *status);
/*******************************************************************************
LAST MODIFIED : 16 January 2007

DESCRIPTION:
NOT_IMPLEMENTED
==============================================================================*/

void interpreter_set_pointer_(struct Interpreter *interpreter,
	const char *variable_name, const char *class_name, void *value,int *status);
/*******************************************************************************
LAST MODIFIED : 16 January 2007

DESCRIPTION:
NOT_IMPLEMENTED
==============================================================================*/
