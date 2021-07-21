/*******************************************************************************
FILE : child_process.h

LAST MODIFIED : 24 August 1999

DESCRIPTION :
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (CHILD_PROCESS_H)
#define CHILD_PROCESS_H

#include "general/object.h"

struct Child_process;

PROTOTYPE_OBJECT_FUNCTIONS(Child_process);
PROTOTYPE_GET_OBJECT_NAME_FUNCTION(Child_process);

struct Child_process *CREATE(Child_process)(char *filename);
/*******************************************************************************
LAST MODIFIED : 24 August 1999

DESCRIPTION :
==============================================================================*/

int DESTROY(Child_process)(struct Child_process **child_process);
/*******************************************************************************
LAST MODIFIED : 24 August 1999

DESCRIPTION :
Destroys a Child_process object
x==============================================================================*/

int Child_process_send_string_to_stdin(struct Child_process *child_process,
	char *string);
/*******************************************************************************
LAST MODIFIED : 26 August 1999

DESCRIPTION :
Sends a string to the stdin pipe of a Child_process.
==============================================================================*/

char *Child_process_get_line_from_stdout(struct Child_process *child_process,
	int timeout);
/*******************************************************************************
LAST MODIFIED : 26 August 1999

DESCRIPTION :
Waits on the pipe from the stdout of the child process until an end_of_line is
received and returns the character string for that line.  If the end_of_line
isn't recieved in the <timeout> limit then the function returns NULL.
==============================================================================*/
#endif /* !defined (CHILD_PROCESS_H) */

