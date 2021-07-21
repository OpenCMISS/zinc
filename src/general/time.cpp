/*******************************************************************************
FILE : time.c

LAST MODIFIED : 26 June 2002

DESCRIPTION :
Defines the gettimeofday and relevant structure for UNIX and WIN32_SYSTEM
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "opencmiss/zinc/zincconfigure.h"


#if defined (WIN32_SYSTEM)
#if defined (_MSC_VER)
	//#define WINDOWS_LEAN_AND_MEAN
	#ifndef _CRTDBG_MAP_ALLOC
		#define _CRTDBG_MAP_ALLOC
	#endif
	#include <stdlib.h>
	#include <crtdbg.h>
#endif /* defined (_MSC_VER) */
//#define WINDOWS_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "general/time.h"
#include "general/debug.h"

/*
int gettimeofday(struct timeval *time, void *timezone)
{
	USE_PARAMETER(timezone);

	unsigned int gettime;
  
	gettime = timeGetTime();
	time->tv_sec = gettime / 1000;
	time->tv_usec = (gettime - (time->tv_sec*1000)) * 1000;
	return 0;
}
*/

clock_t times(struct tms *buffer)
{
	USE_PARAMETER(buffer);
	clock_t gettime;
  
	gettime = timeGetTime();
	return gettime;
}
#else /* defined (WIN32_SYSTEM) */
/* Declare something so that this file will compile */
int dummy;
#endif /* defined (WIN32_SYSTEM) */
