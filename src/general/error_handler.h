/*******************************************************************************
FILE : error_handler.h

LAST MODIFIED : 8 August 2002

DESCRIPTION :
Error handling routines.
???DB.   Not using POSIX because I can do everything I need with ANSI.
???DB.  Hard to abstract because of the special requirements for setjmp and
longjmp .
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/* to get access to signal codes */
#include <signal.h>

/*
Global variables
----------------
*/
extern int signal_code;

/*
Global functions
----------------
*/
void new_error_handler(int sig);

