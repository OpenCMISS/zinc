/*******************************************************************************
FILE : message.h

LAST MODIFIED : 11 June 1999

DESCRIPTION :
Global types and function prototypes for displaying messages.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (MESSAGE_H)
#define MESSAGE_H

#include "opencmiss/zinc/zincsharedobject.h"

/*
Global types
------------
*/

#ifndef MESSAGE_TYPE_ENUM
#define MESSAGE_TYPE_ENUM
enum Message_type
/*******************************************************************************
LAST MODIFIED : 31 May 1996

DESCRIPTION :
The different message types.
==============================================================================*/
{
	ERROR_MESSAGE = 0,
	WARNING_MESSAGE = 1,
	INFORMATION_MESSAGE = 2
}; /* enum Message_type */

#endif

typedef int (Display_message_function)(const char *,enum Message_type, void *);
/*******************************************************************************
LAST MODIFIED : 11 June 1999

DESCRIPTION :
The type of a function for displaying message strings.
==============================================================================*/

/*
Global functions
----------------
*/
void set_display_message_on_console(bool display_flag);

int set_display_message_function(Display_message_function *display_message_function,
	void *data);
/*******************************************************************************
LAST MODIFIED : 11 June 1999

DESCRIPTION :
A function for setting the <display_message_function> to be used for displaying
a message of the specified <message_type>.
==============================================================================*/

/***************************************************************************//**
 * Displays the string as a message of the specified <message_type>.
 * Unlike display_message(), not limited to fixed buffer size.
 */
int display_message_string(enum Message_type message_type,
	const char *the_string);

int display_message(enum Message_type message_type,const char *format, ... );
/*******************************************************************************
LAST MODIFIED : 15 September 2008

DESCRIPTION :
A function for displaying a message of the specified <message_type>.  The printf
form of arguments is used.
==============================================================================*/

int write_message_to_file(enum Message_type message_type,const char *format, ... );
/*******************************************************************************
LAST MODIFIED : 15 September 2008

DESCRIPTION :
A function for writing out commands to com file.
==============================================================================*/
#endif /* !defined (MESSAGE_H) */
