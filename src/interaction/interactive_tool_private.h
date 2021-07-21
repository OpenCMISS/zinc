/*******************************************************************************
FILE : interactive_tool_private.h

LAST MODIFIED : 6 October 2000

DESCRIPTION :
Privileged functions that different types of Interactive Tool are allowed to
use.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (INTERACTIVE_TOOL_PRIVATE_H)
#define INTERACTIVE_TOOL_PRIVATE_H

void *Interactive_tool_get_tool_data(
	struct Interactive_tool *interactive_tool);
/*******************************************************************************
LAST MODIFIED : 6 October 2000

DESCRIPTION :
Priveleged function for objects of type Interactive tool to get their own
type specific data.
==============================================================================*/

#endif /* !defined (INTERACTIVE_TOOL_PRIVATE_H) */
