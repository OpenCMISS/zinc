/*******************************************************************************
FILE : machine.c

LAST MODIFIED : 8 August 2002

DESCRIPTION :
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "opencmiss/zinc/zincconfigure.h"


#if defined (UNIX)
#include <sys/utsname.h>
#endif /* UNIX */
#if defined (VMS)
#include <stdlib.h>
#endif /* defined (VMS) */
#include "general/machine.h"
#include "general/debug.h"
#include "general/message.h"

/*
Global functions
----------------
*/
struct Machine_information *CREATE(Machine_information)(void)
/*******************************************************************************
LAST MODIFIED : 3 March 1997

DESCRIPTION :
Creates a machine information structure.
???GMH.  Perhaps this could be extended to tell it a machine to interrogate?
==============================================================================*/
{
	int name_length;
	struct Machine_information *return_struct;
#if defined (UNIX)
	struct utsname local_information;
#endif /* defined (UNIX) */
#if defined (VMS)
	char *node_name,*temp_char;
#endif /* defined (VMS) */

	ENTER(CREATE(Machine_information));
	if (ALLOCATE(return_struct,struct Machine_information,1))
	{
		/* determine what machine we are */
		return_struct->name = NULL;
		name_length=0;
#if defined (UNIX)
		if (-1==uname(&local_information))
		{
			display_message(WARNING_MESSAGE,"CREATE(Machine_information).  %s",
				"Could not determine local machine name");
			/* NULL terminate the string */
			local_information.nodename[0]='\0';
			name_length=0;
		}
		else
		{
			name_length=strlen(local_information.nodename);
		}
#endif /* defined (UNIX) */
#if defined (VMS)
		if (node_name=getenv("sys$node"))
		{
			if (temp_char=strchr(node_name,':'))
			{
				name_length=temp_char-node_name;
			}
			else
			{
				name_length=strlen(node_name);
			}
		}
#endif /* defined (VMS) */
		if (ALLOCATE(return_struct->name,char,name_length+1))
		{
#if defined (UNIX)
			strcpy(return_struct->name,local_information.nodename);
#endif /* UNIX */
#if defined (VMS)
			strncpy(return_struct->name,node_name,name_length);
#endif /* VMS */
			(return_struct->name)[name_length]='\0';
			return_struct->type=MACHINE_UNKNOWN;
#if defined (UNIX)
			return_struct->type=MACHINE_UNIX;
#endif /* UNIX */
#if defined (VMS)
			return_struct->type=MACHINE_VMS;
#endif /* VMS */
#if defined (WIN32_SYSTEM)
			return_struct->type=MACHINE_WINDOWS;
#endif /* WIN32_SYSTEM */
			return_struct->num_processors=1;
			return_struct->processor_types=(char **)NULL;
		}
		else
		{
			display_message(ERROR_MESSAGE,"CREATE(Machine_information).  %s",
				"Could not allocate memory name");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"CREATE(Machine_information).  %s",
			"Could not allocate memory");
	}
	LEAVE;

	return return_struct;
} /* CREATE(Machine_information) */

int DESTROY(Machine_information)(
	struct Machine_information **machine_information_address)
/*******************************************************************************
LAST MODIFIED : 18 February 1997

DESCRIPTION :
Creates a machine information structure.
???GMH.  Perhaps this could be extended to tell it a machine to interrogate?
==============================================================================*/
{
	struct Machine_information *machine_information;
	int return_code;

	ENTER(DESTROY(Machine_information));
	return_code=0;
	if (machine_information_address&&
		(machine_information=*machine_information_address))
	{
		DEALLOCATE(machine_information->name);
		DEALLOCATE(*machine_information_address);
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"DESTROYMachine_information).  %s",
			"Invalid arguments");
	}
	LEAVE;

	return return_code;
} /* DESTROY(Machine_information) */

