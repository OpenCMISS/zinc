/*******************************************************************************
FILE : machine.h

LAST MODIFIED : 18 February 1997

DESCRIPTION :
Definitions of machine specific stuff.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (MACHINE_H)
#define MACHINE_H

#include "opencmiss/zinc/zincsharedobject.h"

#include "general/object.h"

/*
Global types
------------
*/
enum Machine_type
/*******************************************************************************
LAST MODIFIED : 20 December 1996

DESCRIPTION :
Different types of machines supported.
==============================================================================*/
{
	MACHINE_UNKNOWN,
	MACHINE_UNIX,
	MACHINE_VMS,
	MACHINE_WINDOWS
}; /* enum Machine_type */

struct Machine_information
/*******************************************************************************
LAST MODIFIED : 20 December 1996

DESCRIPTION :
Different types of machines supported.
==============================================================================*/
{
	char *name;
	enum Machine_type type;
	int num_processors;
	char **processor_types;
}; /* struct Machine_information */

/*
Global functions
----------------
*/
struct Machine_information *CREATE(Machine_information)(void);
/*******************************************************************************
LAST MODIFIED : 20 December 1996

DESCRIPTION :
Creates a machine information structure.
???GMH.  Perhaps this could be extended to tell it a machine to interrogate?
==============================================================================*/

int DESTROY(Machine_information)(
	struct Machine_information **machine_information_address);
/*******************************************************************************
LAST MODIFIED : 20 December 1996

DESCRIPTION :
Creates a machine information structure.
???GMH.  Perhaps this could be extended to tell it a machine to interrogate?
==============================================================================*/

#endif /* !defined (MACHINE_H) */
