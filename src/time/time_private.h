/*******************************************************************************
FILE : time_private.h

LAST MODIFIED : 16 Apr 2009

DESCRIPTION :
This provides an object which supplies a concept of time to Cmgui
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (TIME_TIME_PRIVATE_H) /* Distinguish general/time.h and time/time.h */
#define TIME_TIME_PRIVATE_H

#include "time/time.h"
#include "time/time_keeper.hpp"

/***************************************************************************//**
 * Private function to set and access a pointer to time keeper in time object.
 *
 * @param time  The time object to be modified.
 * @param time_keeper  The time keeper to be set to the time object.
 * @return  1 if successfully set the time keeper, otherwise 0.
 */
int Time_object_set_timekeeper(struct Time_object *time,
	struct cmzn_timekeeper *time_keeper);

#endif  /* !defined (TIME_TIME_PRIVATE_H) */
