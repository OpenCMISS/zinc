/*******************************************************************************
FILE : time.h

LAST MODIFIED : 28 December 1999

DESCRIPTION :
This provides an object which supplies a concept of time to Cmgui
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (TIME_TIME_H) /* Distinguish general/time.h and time/time.h */
#define TIME_TIME_H

#include "opencmiss/zinc/timenotifier.h"
#include "general/object.h"
#include "time/time_keeper.hpp"

#define Time_object cmzn_timenotifier

struct Time_object;


struct cmzn_timenotifierevent
{
	double time;
	int access_count;

	cmzn_timenotifierevent() :
		time(0.0),
		access_count(1)
	{
	}

	~cmzn_timenotifierevent()
	{
	}

	cmzn_timenotifierevent *access()
	{
		++(this->access_count);
		return this;
	}

	static int deaccess(cmzn_timenotifierevent* &event);

};

typedef double (*Time_object_next_time_function)(double time_after,
	enum cmzn_timekeeper_play_direction play_direction, void *user_data);

PROTOTYPE_OBJECT_FUNCTIONS(Time_object);
PROTOTYPE_GET_OBJECT_NAME_FUNCTION(Time_object);

/***************************************************************************//**
 * Create a time object with regular update time.
 *
 * @param update_frequency  The number of times which time notifier will receive
 *    callback per unit of time in the time keeper.
 * @param time_offset  the offset value for the time object to receive time
 *    callback.
 * @return  The time object if successfully create a time object otherwise
 *    NULL.
 */
struct Time_object *Time_object_create_regular(double update_frequency,
	double time_offset);

/***************************************************************************//**
 * Set the name of time object.
 *
 * @param time  The time object to set the name for.
 * @param name  The name to be set for time object.
 * @return  1 if successfully set name for time object, otherwise 0.
 */
int Time_object_set_name(struct Time_object *time, const char *name);

/***************************************************************************//**
 * Check either the time specified is a valid callback time or not.
 *
 * @return  1 if the specified time is a valid callback time, otherwise 0.
 */
int Time_object_check_valid_callback_time(struct Time_object *time_object,
	double time,enum cmzn_timekeeper_play_direction play_direction);

double cmzn_timenotifier_get_next_callback_time(cmzn_timenotifier_id timenotifier,
	double curren_time, enum cmzn_timekeeper_play_direction play_direction);
/*******************************************************************************
LAST MODIFIED : 9 December 1998

DESCRIPTION :
==============================================================================*/

int Time_object_set_current_time_privileged(struct Time_object *time,
	double new_time);
/*******************************************************************************
LAST MODIFIED : 17 January 2002

DESCRIPTION :
This routine allows the timekeeper to explicitly set the time.
Separated Time_object_notify_clients_privileged so that all the clients
can be updated with the new time before any of them call back to their clients.
Users of a time object that is controlled by a timekeeper should set the time
through the timekeeper.
==============================================================================*/

int Time_object_notify_clients_privileged(struct Time_object *time);
/*******************************************************************************
LAST MODIFIED : 17 January 2002

DESCRIPTION :
This routine allows the timekeeper to tell the time_object to notify its clients.
Separated off from Time_object_set_current_time_privileged so that all the clients
can be updated with the new time before any of them call back to their clients.
Users of a time object that is controlled by a timekeeper should set the time
through the timekeeper.
==============================================================================*/

int Time_object_set_next_time_function(struct Time_object *time,
	Time_object_next_time_function next_time_function,void *user_data);
/*******************************************************************************
LAST MODIFIED : 18 December 1998

DESCRIPTION :
By setting this function when the time_keeper requests the update_frequency is
not used.  Instead the next_time_function is called to evaluate the next valid
time.
==============================================================================*/

struct cmzn_timekeeper *Time_object_get_timekeeper(struct Time_object *time);
/*******************************************************************************
LAST MODIFIED : 29 September 1998

DESCRIPTION :
==============================================================================*/

double cmzn_timenotifier_get_next_callback_time_private(
	cmzn_timenotifier_id timenotifier, double curren_time,
	enum cmzn_timekeeper_play_direction play_direction);

#endif /* !defined (TIME_TIME_H) */
