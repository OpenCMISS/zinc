/*******************************************************************************
FILE : time.h

LAST MODIFIED : 26 June 2002

DESCRIPTION :
Defines the gettimeofday and relevant structure for UNIX and WIN32_SYSTEM
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (GENERAL_TIME_H) /* Distinguish general/time.h and time/time.h */
#define GENERAL_TIME_H


#include "opencmiss/zinc/zincconfigure.h"


#if defined (UNIX) /* switch (OPERATING_SYSTEM) */
#include <sys/time.h>
#include <sys/times.h>
#elif defined (WIN32_SYSTEM) /* switch (OPERATING_SYSTEM) */
#if defined (_MSC_VER)
	#ifndef _CRTDBG_MAP_ALLOC
		#define _CRTDBG_MAP_ALLOC
	#endif
	#include <stdlib.h>
	#include <crtdbg.h>
#endif /* defined (_MSC_VER) */
//#define WINDOWS_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int gettimeofday(struct timeval *time, void *timezone);

#ifdef __cplusplus
}
#endif /* __cplusplus */

typedef long clock_t;
struct tms 
{
	/* The times function in cmgui is just used to get a timestamp at
		the moment, if more than this is required it will need to be 
		implemented in the c function as well as added to this structure */
	int dummy;
};
clock_t times(struct tms *buffer);
#endif /* switch (OPERATING_SYSTEM) */

#endif /* !defined (GENERAL_TIME_H) */
