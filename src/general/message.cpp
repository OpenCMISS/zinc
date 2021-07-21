/*******************************************************************************
FILE : message.c

LAST MODIFIED : 5 November 2000

DESCRIPTION :
Declaration of functions for displaying messages.
???GMH.  Should all the printf('s be fprintf(stderr,'s?
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#if defined (WIN32_USER_INTERFACE) || defined (_MSC_VER)
//#define WINDOWS_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif /* defined (WIN32_USER_INTERFACE) */

#include "general/debug.h"
#include "general/message.h"

/*
Module variables
----------------
*/
static Display_message_function
	*display_any_message_function=(Display_message_function *)NULL;
static void	*display_message_data = (void *)NULL;

#define MESSAGE_STRING_SIZE 1000
static char message_string[MESSAGE_STRING_SIZE];

static bool display_message_on_console = false;

/*
Global functions
----------------
*/

void set_display_message_on_console(bool display_flag)
{
	display_message_on_console = display_flag;
}

int set_display_message_function(Display_message_function *display_message_function,void *data)
/*******************************************************************************
LAST MODIFIED : 11 June 1999

DESCRIPTION :
A function for setting the <display_message_function> to be used for displaying
a message of the specified <message_type>.
==============================================================================*/
{
	display_any_message_function=display_message_function;
	display_message_data=data;
	return 1;
} /* set_display_message_function */

int display_message_string(enum Message_type message_type,
	const char *the_string)
{
	int return_code = 0;

	if (!the_string)
		return 0;

	if (display_any_message_function)
	{
		return_code=(*display_any_message_function)(the_string,	message_type,
			display_message_data);
	}
	else if (display_message_on_console)
	{
	switch (message_type)
	{
		case ERROR_MESSAGE:
		{
#if defined (WIN32_USER_INTERFACE)
/*			{
				CHAR szBuf[80];
				DWORD dw = GetLastError();

				sprintf(szBuf, "ERROR: %s: GetLastError returned %u\n",
					the_string, dw);
					
				MessageBox(NULL, szBuf, "Error", MB_OK);
			}
			return_code = 1;*/
			return_code=printf("ERROR: %s\n",the_string);
#else /* defined (WIN32_USER_INTERFACE) */
			return_code=printf("ERROR: %s\n",the_string);
#if defined (_MSC_VER)
			OutputDebugString("ERROR: ");
#endif /* _MSC_VER */
#endif /* defined (WIN32_USER_INTERFACE) */
		} break;
		case INFORMATION_MESSAGE:
		{
			/* make sure we don't interpret % characters by printing the string */
#if defined (WIN32_USER_INTERFACE)
/*			{
				CHAR szBuf[80];
				DWORD dw = GetLastError();

				sprintf(szBuf, "%s\n",
					the_string, dw);

				MessageBox(NULL, szBuf, "Information", MB_OK);
			}
			return_code = 1;*/
			return_code=printf("%s",the_string);
#else /* defined (WIN32_USER_INTERFACE) */
			return_code=printf("%s",the_string);
#endif /* defined (WIN32_USER_INTERFACE) */
		} break;
		case WARNING_MESSAGE:
		{
#if defined (WIN32_USER_INTERFACE)
/*			{
				CHAR szBuf[80];
				DWORD dw = GetLastError();

				sprintf(szBuf, "WARNING: %s: GetLastError returned %u\n",
					the_string, dw);

				MessageBox(NULL, szBuf, "Warning", MB_OK);
			}
			return_code = 1;*/
			return_code=printf("WARNING: %s\n",the_string);
#else /* defined (WIN32_USER_INTERFACE) */
			return_code=printf("WARNING: %s\n",the_string);
#if defined (_MSC_VER)
			OutputDebugString("WARNING: ");
#endif /* _MSC_VER */
#endif /* defined (WIN32_USER_INTERFACE) */
		} break;
		default:
		{
#if defined (WIN32_USER_INTERFACE)
/*			{ 
				CHAR szBuf[80]; 
				DWORD dw = GetLastError(); 

				sprintf(szBuf, "UNKNOWN: %s: GetLastError returned %u\n", 
					the_string, dw);

				MessageBox(NULL, szBuf, "Error", MB_OK); 
			} 
			return_code = 1;*/
			return_code=printf("UNKNOWN: %s\n",the_string);
#else /* defined (WIN32_USER_INTERFACE) */
			return_code=printf("UNKNOWN: %s\n",the_string);
#if defined (_MSC_VER)
			OutputDebugString("UNKNOWN: ");
#endif /* _MSC_VER */
#endif /* defined (WIN32_USER_INTERFACE) */
		} break;
	}
	}
#if defined (_MSC_VER)
	OutputDebugString(the_string);
	OutputDebugString("\n");
#endif /* _MSC_VER */
	LEAVE;

	return (return_code);
}

int display_message(enum Message_type message_type,const char *format, ... )
/*******************************************************************************
LAST MODIFIED : 15 September 2008

DESCRIPTION :
A function for displaying a message of the specified <message_type>.  The printf
form of arguments is used.
==============================================================================*/
{
	int return_code;
	va_list ap;

	va_start(ap,format);
	message_string[MESSAGE_STRING_SIZE-1] = '\0';
	return_code=vsnprintf(message_string,MESSAGE_STRING_SIZE-1,format,ap);
	if (return_code >= (MESSAGE_STRING_SIZE-1))
	{
		char error_string[100];
		sprintf(error_string,"Overflow of message_string.  "
			"Following is truncated to %d characters:",MESSAGE_STRING_SIZE-1);
		if (display_any_message_function)
		{
			return_code=(*display_any_message_function)(error_string, ERROR_MESSAGE,
				display_message_data);
		}
		else
		{
			return_code=printf("ERROR: %s\n",error_string);
		}
	}
	if (!display_message_string(message_type, message_string))
	{
		return_code = 0;
	}
	va_end(ap);

	return (return_code);
} /* display_message */

int write_message_to_file(enum Message_type message_type,const char *format, ... )
/*******************************************************************************
LAST MODIFIED : 15 September 2008

DESCRIPTION :
A function for writing out commands to com file.
==============================================================================*/
{
	int return_code;
	va_list ap;
	FILE *com_file;
	ENTER(write_message_to_file);
	va_start(ap,format);
/*	return_code=vsnprintf(message_string,MESSAGE_STRING_SIZE,format,ap);*/
	return_code=vsprintf(message_string,format,ap);
	if (return_code >= (MESSAGE_STRING_SIZE-1))
	{
		char error_string[100];
		sprintf(error_string,"Overflow of message_string.  "
			"Following is truncated to %d characters:",return_code);
		if (display_any_message_function)
		{
			return_code=(*display_any_message_function)(error_string, ERROR_MESSAGE,
				display_message_data);
		}
		else
		{
			return_code=printf("ERROR: %s\n",error_string);
		}
	}
	switch (message_type)
	{
		 case INFORMATION_MESSAGE:
		 {
				if (NULL != (com_file = fopen("temp_file_com.com", "a")))
				{
#if defined (WIN32_USER_INTERFACE)
					 fprintf(com_file, "%s", message_string);
#else /* defined (WIN32_USER_INTERFACE) */
					 fprintf(com_file, "%s", message_string);
#endif /* defined (WIN32_USER_INTERFACE) */
					 fclose(com_file);
				}
		 } break;
		 default:
		 {
#if defined (WIN32_USER_INTERFACE)
				/*			{ 
					CHAR szBuf[80]; 
					DWORD dw = GetLastError(); 
					
					sprintf(szBuf, "UNKNOWN: %s: GetLastError returned %u\n", 
					message_string, dw); 
					
					MessageBox(NULL, szBuf, "Error", MB_OK); 
					} 
					return_code = 1;*/
				return_code=printf("UNKNOWN: %s\n",message_string);
#else /* defined (WIN32_USER_INTERFACE) */
				return_code=printf("UNKNOWN: %s\n",message_string);
#endif /* defined (WIN32_USER_INTERFACE) */
		 } break;
	}
	va_end(ap);
	LEAVE;

	return (return_code);
} /* display_message */

