/*******************************************************************************
FILE : time.c

LAST MODIFIED : 25 November 1999

DESCRIPTION :
This provides an object which supplies a concept of time to Cmgui
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <math.h>
#include <stdio.h>

#include "opencmiss/zinc/status.h"
#include "general/debug.h"
#include "general/object.h"
#include "general/message.h"
#include "time/time.h"
#include "time/time_keeper.hpp"

int cmzn_timenotifierevent::deaccess(cmzn_timenotifierevent* &event)
{
	if (event)
	{
		--(event->access_count);
		if (event->access_count <= 0)
			delete event;
		event = 0;
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_timenotifierevent_id cmzn_timenotifierevent_access(
	cmzn_timenotifierevent_id timenotifierevent)
{
	if (timenotifierevent)
		return timenotifierevent->access();
	return 0;
}

int cmzn_timenotifierevent_destroy(cmzn_timenotifierevent_id *timenotifierevent_address)
{
	return cmzn_timenotifierevent::deaccess(*timenotifierevent_address);
}

double cmzn_timenotifierevent_get_time(cmzn_timenotifierevent_id timenotifierevent)
{
	return timenotifierevent->time;
}

enum Time_object_type
{
	TIME_OBJECT_REGULAR,
	TIME_OBJECT_NEXT_TIME_FUNCTION
};

struct Time_object
{
	char *name;
	double current_time;
	double update_frequency;
	double time_offset;
	enum Time_object_type type;
	cmzn_timenotifier_callback callback_function;
	void *callback_user_data;
	struct cmzn_timekeeper *time_keeper;
	Time_object_next_time_function next_time_function;
	void *next_time_user_data;

	int access_count;
};

int DESTROY(Time_object)(struct Time_object **time)
/*******************************************************************************
LAST MODIFIED : 29 September 1998

DESCRIPTION :
Destroys a Time_object object
==============================================================================*/
{
	int return_code;

	if (time && *time)
	{
		return_code=1;

		if((*time)->time_keeper)
		{
			(*time)->time_keeper->removeTimeObject(*time);
		}

		(*time)->callback_function = 0;
		(*time)->callback_user_data = 0;

		DEALLOCATE((*time)->name);
		DEALLOCATE(*time);
		*time = (struct Time_object *)NULL;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(Time_object).  Missing time object");
		return_code=0;
	}

	return (return_code);
} /* DESTROY(Time_object) */

DECLARE_OBJECT_FUNCTIONS(Time_object)
DECLARE_DEFAULT_GET_OBJECT_NAME_FUNCTION(Time_object)

struct Time_object *CREATE(Time_object)(void)
/*******************************************************************************
LAST MODIFIED : 29 September 1998

DESCRIPTION :
==============================================================================*/
{
	struct Time_object *time;

	ENTER(CREATE(Time_object));

	if (ALLOCATE(time, struct Time_object, 1))
	{
		time->name = (char *)NULL;
		time->current_time = 0.0;
		time->time_keeper = (struct cmzn_timekeeper *)NULL;
		time->callback_function = 0;
		time->callback_user_data = 0;
		time->update_frequency = 10.0;
		time->time_offset = 0.0;
		/* after setting the time notifier type in the type specific constructor,
			 it should not be allowed to change it later. This can be enforced when 
			 other types are added. */
		time->type = TIME_OBJECT_REGULAR;
		time->next_time_function = (Time_object_next_time_function)NULL;
		time->next_time_user_data = NULL;
		time->access_count = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"CREATE(Time_object). Unable to allocate buffer structure");
		time = (struct Time_object *)NULL;
	}

	LEAVE;

	return (time);
} /* CREATE(Time_object) */

struct Time_object *Time_object_create_regular(double update_frequency, 
	double time_offset)
{
	struct Time_object *time;

	ENTER(Time_object_create_regular);
	if (NULL != (time = CREATE(Time_object)()))
	{
		time->update_frequency = update_frequency;
		time->time_offset = time_offset;
		time->type = TIME_OBJECT_REGULAR;
	}
	else
	{
		time = (struct Time_object *)NULL;
	}
	LEAVE;
	
	return (time);
}

int Time_object_set_name(struct Time_object *time, const char *name)
{
	int return_code;
	char *temp_name;
	
	ENTER(Time_object_set_name);
	if(time && name)
	{
		if (REALLOCATE(temp_name, time->name, char, strlen(name) + 1))
		{
			time->name = temp_name;
			strcpy(time->name, name);
			return_code = 1;
		}
		else
		{
			display_message(ERROR_MESSAGE,"Time_object_set_name. "
				"Unable to reallocate memory for name.");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Time_object_set_name. Invalid arguments");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Time_object_set_name */

double cmzn_timenotifier_get_time(cmzn_timenotifier_id timenotifier)
{
	if (timenotifier)
		return timenotifier->current_time;
	return 0.0;
}

int Time_object_check_valid_callback_time(struct Time_object *time_object,
	double time,enum cmzn_timekeeper_play_direction play_direction)
{
	int return_code = 0;

	if (time_object)
	{
		switch(time_object->type)
		{
			default:
			{
				switch(play_direction)
				{
					case CMZN_TIMEKEEPER_PLAY_DIRECTION_FORWARD:
					{
						/* minus the current time by a faction of the update frequency
							 then workout the closest callback time */
						if (time == (1.0 + 
								floor((time - 1.0 / time_object->update_frequency) *
								time_object->update_frequency)) /
								time_object->update_frequency)
						{
							return_code = 1;
						}
					} break;
					case CMZN_TIMEKEEPER_PLAY_DIRECTION_REVERSE:
					{
						/* add the current time by a faction of the update frequency
							 then workout the closest callback time */
						if (time == (1.0 + 
								ceil((time + 1.0 / time_object->update_frequency) *
								time_object->update_frequency)) /
								time_object->update_frequency)
						{
							return_code = 1;
						}
					} break;
					default:
					{
						return_code=0;
					} break;
				}
			} break;
			case TIME_OBJECT_NEXT_TIME_FUNCTION:
			{
				if(time_object->next_time_function)
				{
					if (time == (time_object->next_time_function)(time, play_direction,
							time_object->next_time_user_data))
					{
						return_code = 1;
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"Time_object_check_valid_callback_time.  type TIME_OBJECT_NEXT_TIME_FUNCTION but no function");
					return_code=0;
				}
			} break;
		}
	}
	else
	{
		return_code=0;
	}

	return (return_code);
}

double cmzn_timenotifier_get_next_callback_time_private(cmzn_timenotifier_id timenotifier,
	double curren_time, enum cmzn_timekeeper_play_direction play_direction)
{
	double return_code;

	if (timenotifier)
	{
		switch(timenotifier->type)
		{
			default:
			{
				switch(play_direction)
				{
					case CMZN_TIMEKEEPER_PLAY_DIRECTION_FORWARD:
					{
						return_code = timenotifier->time_offset + (1.0 +
							floor((curren_time - timenotifier->time_offset) *
								timenotifier->update_frequency)) / timenotifier->update_frequency;
					} break;
					case CMZN_TIMEKEEPER_PLAY_DIRECTION_REVERSE:
					{
						return_code = timenotifier->time_offset + (-1.0 +
							ceil((curren_time -  timenotifier->time_offset) * timenotifier->update_frequency)) /
							timenotifier->update_frequency;
					} break;
					default:
					{
						return_code=0;
					} break;
				}
			} break;
			case TIME_OBJECT_NEXT_TIME_FUNCTION:
			{
				if(timenotifier->next_time_function)
				{
					return_code = (timenotifier->next_time_function)(curren_time, play_direction,
						timenotifier->next_time_user_data);
				}
				else
				{
					return_code=0;
				}
			} break;
		}
	}
	else
	{
		return_code=0;
	}

	return (return_code);
}

double cmzn_timenotifier_get_next_callback_time(cmzn_timenotifier_id timenotifier,
	enum cmzn_timekeeper_play_direction play_direction)
{
	if (timenotifier)
	{
		return cmzn_timenotifier_get_next_callback_time_private(timenotifier,
			timenotifier->current_time, play_direction);
	}
	return 0.0;
}


int Time_object_set_current_time_privileged(struct Time_object *time,
	double new_time)
/*******************************************************************************
LAST MODIFIED : 17 January 2002

DESCRIPTION :
This routine allows the timekeeper to explicitly set the time.
Separated Time_object_notify_clients_privileged so that all the clients
can be updated with the new time before any of them call back to their clients.
Users of a time object that is controlled by a timekeeper should set the time 
through the timekeeper.
==============================================================================*/
{
	int return_code;

	ENTER(Time_object_set_current_time_privileged);

	if (time)
	{
		time->current_time = new_time;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Time_object_set_current_time_privileged. Invalid time object");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Time_object_set_current_time_privileged */

int Time_object_notify_clients_privileged(struct Time_object *time)
/*******************************************************************************
LAST MODIFIED : 17 January 2002

DESCRIPTION :
This routine allows the timekeeper to tell the time_object to notify its clients.
Separated off from Time_object_set_current_time_privileged so that all the clients
can be updated with the new time before any of them call back to their clients.
Users of a time object that is controlled by a timekeeper should set the time 
through the timekeeper.
==============================================================================*/
{
	int return_code;

	if (time && time->callback_function)
	{
		cmzn_timenotifierevent_id event = new cmzn_timenotifierevent();
		event->time = time->current_time;
		(time->callback_function)(event,time->callback_user_data);
		cmzn_timenotifierevent_destroy(&event);
		return_code = 1;
	}
	else
	{
		return_code=0;
	}

	return (return_code);
} /* Time_object_set_current_time_privileged */

cmzn_timenotifier_regular_id cmzn_timenotifier_cast_regular(
	cmzn_timenotifier_id timenotifier)
{
	if (timenotifier && (TIME_OBJECT_REGULAR == timenotifier->type))
	{
		cmzn_timenotifier_access(timenotifier);
		return reinterpret_cast<cmzn_timenotifier_regular_id>(timenotifier);
	}
	return 0;
}

int cmzn_timenotifier_regular_destroy(
	cmzn_timenotifier_regular_id *timenotifier_regular_address)
{
	return cmzn_timenotifier_destroy(reinterpret_cast<cmzn_timenotifier_id *>(timenotifier_regular_address));
}

int cmzn_timenotifier_regular_set_frequency(
	cmzn_timenotifier_regular_id timenotifier_regular, double frequency)
{
	if (timenotifier_regular && (frequency > 0.0))
	{
		cmzn_timenotifier *timenotifier = reinterpret_cast<cmzn_timenotifier*>(timenotifier_regular);
		timenotifier->update_frequency = frequency;
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_timenotifier_regular_set_offset(
	cmzn_timenotifier_regular_id timenotifier_regular, double time_offset)
{
	if (timenotifier_regular)
	{
		cmzn_timenotifier *timenotifier = reinterpret_cast<cmzn_timenotifier*>(timenotifier_regular);
		timenotifier->time_offset = time_offset;
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int Time_object_set_next_time_function(struct Time_object *time,
	Time_object_next_time_function next_time_function, void *user_data)
/*******************************************************************************
LAST MODIFIED : 25 November 1999

DESCRIPTION :
By setting this function when the time_keeper requests the update_frequency is
not used.  Instead the next_time_function is called to evaluate the next valid
time.
==============================================================================*/
{
	int return_code;

	ENTER(Time_object_set_next_time_function);
	if (time && next_time_function)
	{
		time->type = TIME_OBJECT_NEXT_TIME_FUNCTION;
		time->next_time_function = next_time_function;
		time->next_time_user_data = user_data;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Time_object_set_next_time_function. Invalid arguments");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Time_object_set_next_time_function */

struct cmzn_timekeeper *Time_object_get_timekeeper(struct Time_object *time)
/*******************************************************************************
LAST MODIFIED : 29 September 1998

DESCRIPTION :
==============================================================================*/
{
	struct cmzn_timekeeper *return_code;

	ENTER(Time_object_get_timekeeper);

	if (time)
	{
		return_code = time->time_keeper;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Time_object_get_timekeeper. Invalid time object");
		return_code=(struct cmzn_timekeeper *)NULL;
	}
	LEAVE;

	return (return_code);
} /* Time_object_get_timekeeper */

int Time_object_set_timekeeper(struct Time_object *time,
	struct cmzn_timekeeper *time_keeper)
/*******************************************************************************
LAST MODIFIED : 29 September 1998

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(Time_object_set_timekeeper);

	if (time)
	{
		return_code = REACCESS(cmzn_timekeeper)(&(time->time_keeper), time_keeper);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Time_object_set_timekeeper. Invalid time object");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Time_object_set_timekeeper */

void *cmzn_timenotifier_get_callback_user_data(cmzn_timenotifier_id timenotifier)
{
	if (timenotifier)
	{
		return timenotifier->callback_user_data;
	}

	return 0;
}

int cmzn_timenotifier_set_callback(cmzn_timenotifier_id timenotifier,
	cmzn_timenotifier_callback function, void *user_data_in)
{
	if (timenotifier && function)
	{
		timenotifier->callback_function = function;
		timenotifier->callback_user_data = user_data_in;
		return CMZN_OK;
	}

	return CMZN_ERROR_ARGUMENT;
}

int cmzn_timenotifier_clear_callback(cmzn_timenotifier_id timenotifier)
{
	if (timenotifier)
	{
		timenotifier->callback_function = 0;
		timenotifier->callback_user_data = 0;
		return CMZN_OK;
	}

	return CMZN_ERROR_ARGUMENT;
}

cmzn_timenotifier_id cmzn_timenotifier_access(cmzn_timenotifier_id timenotifier)
{
	if (timenotifier)
		return ACCESS(Time_object)(timenotifier);
	return 0;
}

int cmzn_timenotifier_destroy(cmzn_timenotifier_id *timenotifier_address)
{
	if (timenotifier_address && *timenotifier_address)
	{
		DEACCESS(Time_object)(timenotifier_address);
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}
