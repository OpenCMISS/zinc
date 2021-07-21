/*******************************************************************************
FILE : child_process.c

LAST MODIFIED : 24 August 1999

DESCRIPTION :
This provides an object which interfaces between a child_process and Cmgui
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "opencmiss/zinc/zincconfigure.h"


#if defined (UNIX)
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#endif /* defined (UNIX) */
#include "general/debug.h"
#include "general/object.h"
#include "general/message.h"
#include "general/child_process.h"

struct Child_process
{
	char *name;
#if defined (UNIX)
	pid_t process_id;
	int stdin_filedes;
	int stdout_filedes;
#endif /* defined (UNIX) */

	int access_count;
};

DECLARE_OBJECT_FUNCTIONS(Child_process)
DECLARE_DEFAULT_GET_OBJECT_NAME_FUNCTION(Child_process)

struct Child_process *CREATE(Child_process)(char *filename)
/*******************************************************************************
LAST MODIFIED : 26 August 1999

DESCRIPTION :
==============================================================================*/
{
#if defined (UNIX)
	int stdin_filedes[2], stdout_filedes[2];
	pid_t process_id;
#endif /* defined (UNIX) */
	struct Child_process *child_process;

	ENTER(CREATE(Child_process));

	if(filename)
	{
#if defined (UNIX)
		if (ALLOCATE(child_process, struct Child_process, 1) &&
			ALLOCATE(child_process->name, char, strlen(filename) + 1))
		{
			strcpy(child_process->name, filename);
			child_process->access_count = 0;
			if ((!pipe(stdin_filedes)) && (!pipe(stdout_filedes)))
			{
				if (-1 != (process_id = fork()))
				{
					if (process_id)
					{
						/* Parent process comes here */
						child_process->process_id = process_id;
						child_process->stdin_filedes = stdin_filedes[1];
						child_process->stdout_filedes = stdout_filedes[0];
					}
					else
					{
						/* Child process comes here */
						/* Remap the stdin and stdout */
						dup2(stdin_filedes[0],STDIN_FILENO);
						dup2(stdout_filedes[1],STDOUT_FILENO);
						/* Execute the filename */
						execlp(filename, filename, NULL);
						/* The execlp should never return as the process gets overlayed */
						display_message(ERROR_MESSAGE,"CREATE(Child_process). Exec error!");
						exit(EXIT_FAILURE);
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,"CREATE(Child_process). Unable to fork child process");
					DEALLOCATE(child_process);
					DEALLOCATE(child_process->name);
					child_process = (struct Child_process *)NULL;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,"CREATE(Child_process). Unable to create pipes");
				DEALLOCATE(child_process);
				DEALLOCATE(child_process->name);
				child_process = (struct Child_process *)NULL;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,"CREATE(Child_process). Unable to allocate structure");
			child_process = (struct Child_process *)NULL;
		}
#else /* defined (UNIX) */
		display_message(ERROR_MESSAGE,"CREATE(Child_process).  "
			"Not implemented outside UNIX yet.");
		child_process = (struct Child_process *)NULL;
#endif /* defined (UNIX) */
	}
	else
	{
		display_message(ERROR_MESSAGE,"CREATE(Child_process). Invalid arguments");
		child_process = (struct Child_process *)NULL;
	}
	LEAVE;

	return (child_process);
} /* CREATE(Child_process) */

int Child_process_send_string_to_stdin(struct Child_process *child_process,
	char *string)
/*******************************************************************************
LAST MODIFIED : 27 August 1999

DESCRIPTION :
Sends a string to the stdin pipe of a Child_process.
==============================================================================*/
{
#if defined (UNIX)
	int length;
#endif /* defined (UNIX) */
	int return_code;

	ENTER(Child_process_send_string_to_stdin);
	if (child_process)
	{
#if defined (UNIX)
		length = strlen(string);
		if (-1 == write(child_process->stdin_filedes, (void *)string, length))
		{
			display_message(ERROR_MESSAGE,"Child_process_send_string_to_stdin.  "
				"Error writing child_process result.");
		   return_code = 0;
		}
		else
		{
			return_code=1;
		}
#else /* defined (UNIX) */
		USE_PARAMETER(string);
		display_message(ERROR_MESSAGE,"Child_process_send_string_to_stdin.  "
			"Not implemented outside UNIX yet.");
		return_code=0;
#endif /* defined (UNIX) */
	}
	else
	{
		display_message(ERROR_MESSAGE,"Child_process_send_string_to_stdin.  "
			"Missing child process");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Child_process_send_string_to_stdin */

char *Child_process_get_line_from_stdout(struct Child_process *child_process,
	int timeout)
/*******************************************************************************
LAST MODIFIED : 26 August 1999

DESCRIPTION :
Waits on the pipe from the stdout of the child process until an end_of_line is
received and returns the character string for that line.  If the end_of_line
isn't recieved in the <timeout> limit then the function returns NULL.
==============================================================================*/
{
	char *return_string;
#if defined (UNIX)
#define BLOCKSIZE (100)
	char last_char, *new_string;
	fd_set readfds;
	int flags, index, string_size;
	ssize_t number_read;
	struct timeval timeout_struct;
#endif /* defined (UNIX) */

	ENTER(Child_process_get_line_from_stdout);
	if (child_process)
	{
#if defined (UNIX)
		FD_ZERO(&readfds);
		FD_SET(child_process->stdout_filedes, &readfds);
		string_size = 2 * BLOCKSIZE;
		last_char = 0xff;
		index = 0;
		if (ALLOCATE(return_string, char, 2 * BLOCKSIZE))
		{
			while((last_char != '\n') && return_string)
			{
				if (index + BLOCKSIZE > string_size)
				{
					if(REALLOCATE(new_string, return_string, char,
						index + 2 * BLOCKSIZE))
					{
						return_string = new_string;
						string_size = index + 2 * BLOCKSIZE;
					}
					else
					{
						display_message(ERROR_MESSAGE,"Child_process_get_line_from_stdout."
							"  Unable to allocate string");
						DEALLOCATE(return_string);
						return_string = (char *)NULL;
					}
				}
				if (return_string)
				{
					flags = fcntl (child_process->stdout_filedes, F_GETFL);
					/*???DB.  Wouldn't compile at home.  O_NDELAY is equivalent to
						FNDELAY */
/*					flags &= FNDELAY;*/
					flags &= O_NDELAY;
					fcntl (child_process->stdout_filedes, F_SETFL, flags);
					timeout_struct.tv_sec = timeout;
					timeout_struct.tv_usec = 10000;
					if (select(FD_SETSIZE, &readfds, NULL, NULL, &timeout_struct))
					{
						number_read = read(child_process->stdout_filedes,
							(void *)(return_string + index), BLOCKSIZE);
						while((last_char != '\n') && number_read)
						{
							last_char = *(return_string + index);
							number_read--;
							index++;
						}
					}
					else
					{
						display_message(ERROR_MESSAGE,"Child_process_get_line_from_stdout."
							"  Timed out waiting for response from child");
						DEALLOCATE(return_string);
						return_string = (char *)NULL;
					}
				}
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,"Child_process_get_line_from_stdout."
				"  Unable to allocate string");
			return_string = (char *)NULL;
		}
#else /* defined (UNIX) */
		USE_PARAMETER(timeout);
		display_message(ERROR_MESSAGE,"Child_process_get_line_from_stdout.  "
			"Not implemented outside UNIX yet.");
		return_string = (char *)NULL;
#endif /* defined (UNIX) */
	}
	else
	{
		display_message(ERROR_MESSAGE,"Child_process_get_line_from_stdout.  "
			"Missing child_process");
		return_string = (char *)NULL;
	}
	LEAVE;

	return (return_string);
} /* Child_process_get_line_from_stdout */

int DESTROY(Child_process)(struct Child_process **child_process)
/*******************************************************************************
LAST MODIFIED : 24 August 1999

DESCRIPTION :
Destroys a Child_process object
x==============================================================================*/
{
	int return_code;

	ENTER(DESTROY(Child_process));

	if (child_process && *child_process)
	{
		return_code=1;

		DEALLOCATE((*child_process)->name);
		DEALLOCATE(*child_process);
		*child_process = (struct Child_process *)NULL;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(Child_process).  Missing child_process object");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Child_process) */

