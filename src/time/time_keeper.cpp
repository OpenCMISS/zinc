/*******************************************************************************
FILE : time_keeper.c

LAST MODIFIED : 21 January 2003

DESCRIPTION :
This object defines a relationship between a bunch of time objects, keeps them
in sync and allows control such as play, rewind and fast forward.
This is intended to be multithreaded......
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <math.h>
#include <stdio.h>

#include "description_io/timekeeper_json_io.hpp"
#include "opencmiss/zinc/status.h"
#include "opencmiss/zinc/timekeeper.h"
#include "general/debug.h"
#include "general/list_private.h"
#include "general/mystring.h"
#include "general/object.h"
#include "general/time.h"
#include "general/message.h"
#include "time/time.h"
#include "time/time_keeper.hpp"
#include "time/time_private.h"
#include "general/enumerator_conversion.hpp"

cmzn_timekeeper::cmzn_timekeeper():
	name(NULL),
	time(0.0),
	time_object_info_list(NULL),
	minimum(0.0), maximum(0.0),
	access_count(1)
{
}

cmzn_timekeeper::~cmzn_timekeeper()
{
	struct Time_object_info *object_info = this->time_object_info_list;
	struct Time_object_info *next = 0;
	while(object_info)
	{
		if (object_info->time_object)
		{
			Time_object_set_timekeeper(object_info->time_object,
				(struct cmzn_timekeeper *)NULL);
			object_info->time_object = (struct Time_object *)NULL;
		}
		next = object_info->next;
		DEALLOCATE(object_info);
		object_info = next;
	}

	if (name)
	{
		DEALLOCATE(name);
	}
}

bool cmzn_timekeeper::setName(const char *name_in)
{
	char *new_name = duplicate_string(name_in);
	if (!new_name)
		return false;
	if (name)
		DEALLOCATE(name);
	name = new_name;
	return true;
}

char *cmzn_timekeeper::getName()
{
	char *name_out = NULL;
	if (name)
	{
		name_out = duplicate_string(name);
	}

	return name_out;
}

int cmzn_timekeeper::addTimeObject(struct Time_object *time_object)
{
	int return_code = 0;
	struct Time_object_info *object_info, *previous;

	if (time_object)
	{
		if (!Time_object_get_timekeeper(time_object))
		{
			if(ALLOCATE(object_info, struct Time_object_info, 1))
			{
				object_info->time_object = time_object;
				Time_object_set_current_time_privileged(time_object, time);
				Time_object_notify_clients_privileged(time_object);
				object_info->next = (struct Time_object_info *)NULL;
				if(time_object_info_list)
				{
					previous = time_object_info_list;
					while(previous->next)
					{
						previous = previous->next;
					}
					previous->next = object_info;
				}
				else
				{
					time_object_info_list = object_info;
				}
				Time_object_set_timekeeper(time_object, this);
				return_code=1;
			}

		}
	}

	return (return_code);
}

int cmzn_timekeeper::removeTimeObject(struct Time_object *time_object)
{
	int return_code = 0;
	struct Time_object_info *object_info, *previous = NULL;

	if (time_object)
	{
		if (this == Time_object_get_timekeeper(time_object))
		{
			return_code = 0;
			object_info = time_object_info_list;
			while(!return_code && object_info)
			{
				if (object_info->time_object == time_object)
				{
					if (object_info == time_object_info_list)
					{
						time_object_info_list = object_info->next;
					}
					else
					{
						previous->next = object_info->next;
					}
					Time_object_set_timekeeper(object_info->time_object, NULL);
					DEALLOCATE(object_info);
					return_code = 1;
				}
				else
				{
					previous = object_info;
					object_info = object_info->next;
				}
			}
		}
	}
	else
	{
		return_code=0;
	}

	return (return_code);
}

double cmzn_timekeeper::getTime()
{
	return time;
}

double cmzn_timekeeper::getMinimum()
{
	return minimum;
}

int cmzn_timekeeper::setMinimum(double minimum_in)
{
	minimum = minimum_in;

	return 1;
}

double cmzn_timekeeper::getMaximum()
{
	return maximum;
}

int cmzn_timekeeper::setMaximum(double maximum_in)
{
	maximum = maximum_in;

	return 1;
}

int cmzn_timekeeper::setTime(double new_time)
{
	static int recursive_check = 0;
	struct Time_object_info *object_info;

	if(!recursive_check)
	{
		recursive_check = 1;
		time = new_time;
		/* Update the times in all the clients and then get them to call their clients */
		object_info = time_object_info_list;
		while(object_info)
		{
			Time_object_set_current_time_privileged(object_info->time_object, new_time);
			object_info = object_info->next;
		}
		object_info = time_object_info_list;
		while(object_info)
		{
			Time_object_notify_clients_privileged(object_info->time_object);
			object_info = object_info->next;
		}
		recursive_check = 0;
	}
	return CMZN_OK;
}

double cmzn_timekeeper::getNextCallbackTime(enum cmzn_timekeeper_play_direction direction)
{
	struct Time_object_info *object_info;

	object_info = time_object_info_list;
	double next_callback_time = 0.0, closet_callback_time = 0.0;
	int closet_time_set = 0;
	while (object_info)
	{
		next_callback_time = cmzn_timenotifier_get_next_callback_time(object_info->time_object,
			direction);
		if (closet_time_set == 0)
		{
			closet_time_set = 1;
			closet_callback_time = next_callback_time;
		}
		else
		{
			if (direction == CMZN_TIMEKEEPER_PLAY_DIRECTION_FORWARD)
			{
				if ((closet_callback_time - time) >  (next_callback_time - time))
				{
					closet_callback_time = next_callback_time;
				}
			}
			else if (direction == CMZN_TIMEKEEPER_PLAY_DIRECTION_REVERSE)
			{
				if ((time - closet_callback_time) >  (time - next_callback_time))
				{
					closet_callback_time = next_callback_time;
				}
			}
		}
		object_info = object_info->next;
	}
	if ((direction == CMZN_TIMEKEEPER_PLAY_DIRECTION_FORWARD) &&
		(closet_callback_time > maximum))
	{
		closet_callback_time = minimum + closet_callback_time - maximum;
	}
	else if ((direction == CMZN_TIMEKEEPER_PLAY_DIRECTION_REVERSE) &&
		(closet_callback_time < minimum))
	{
		closet_callback_time = maximum - (minimum - closet_callback_time);
	}

	return closet_callback_time;
}

int cmzn_timekeeper::hasTimeObject()
{
	 if (time_object_info_list)
	 {
		 return 1;
	 }

	return 0;
}

struct Time_object_info *cmzn_timekeeper::getObjectInfo()
{
	return time_object_info_list;
}

void cmzn_timekeeper::setTimeQuiet(double new_time)
{
	time = new_time;
}

int DESTROY(cmzn_timekeeper)(struct cmzn_timekeeper **timekeeper_address)
{
	int return_code = 0;

	if (timekeeper_address && (*timekeeper_address))
	{
		delete *timekeeper_address;
		*timekeeper_address = NULL;
		return_code = 1;
	}

	return (return_code);
}

DECLARE_OBJECT_FUNCTIONS(cmzn_timekeeper)

cmzn_timenotifier_id cmzn_timekeeper_create_timenotifier_regular(
	cmzn_timekeeper_id timekeeper, double update_frequency, double time_offset)
{
	cmzn_timenotifier_id timenotifier = NULL;
	if (timekeeper)
	{
		timenotifier = Time_object_create_regular(update_frequency, time_offset);
		if (timenotifier)
		{
			if (!timekeeper->addTimeObject(timenotifier))
			{
				cmzn_timenotifier_destroy(&timenotifier);
			}
		}
	}
	return timenotifier;
}

cmzn_timekeeper_id cmzn_timekeeper_access(cmzn_timekeeper_id timekeeper)
{
	if (timekeeper)
		return timekeeper->access();
	return 0;
}

int cmzn_timekeeper_destroy(cmzn_timekeeper_id *timekeeper_address)
{
	return (DEACCESS(cmzn_timekeeper)(timekeeper_address));
}

double cmzn_timekeeper_get_maximum_time(cmzn_timekeeper_id timekeeper)
{
	if (timekeeper)
		return timekeeper->getMaximum();
	return 0;
}

int cmzn_timekeeper_set_maximum_time(cmzn_timekeeper_id timekeeper, double maximum_time)
{
	if (timekeeper)
		return timekeeper->setMaximum(maximum_time);
	return CMZN_ERROR_ARGUMENT;
}

double cmzn_timekeeper_get_minimum_time(cmzn_timekeeper_id timekeeper)
{
	if (timekeeper)
		return timekeeper->getMinimum();
	return 0;
}

int cmzn_timekeeper_set_minimum_time(cmzn_timekeeper_id timekeeper, double minimum_time)
{
	if (timekeeper)
		return timekeeper->setMinimum(minimum_time);
	return CMZN_ERROR_ARGUMENT;
}

double cmzn_timekeeper_get_time(cmzn_timekeeper_id timekeeper)
{
	if (timekeeper)
		return timekeeper->getTime();
	return 0;
}

double cmzn_timekeeper_get_next_callback_time(cmzn_timekeeper_id timekeeper,
	enum cmzn_timekeeper_play_direction direction)
{
	if (timekeeper)
		return timekeeper->getNextCallbackTime(direction);
	return 0.0;
}


int cmzn_timekeeper_set_time(cmzn_timekeeper_id timekeeper, double time)
{
	if (timekeeper)
		return timekeeper->setTime(time);
	return CMZN_ERROR_ARGUMENT;
}

cmzn_timekeepermodule *cmzn_timekeepermodule::create()
{
	cmzn_timekeepermodule *timekeepermodule = new cmzn_timekeepermodule();
	if ((timekeepermodule) && (timekeepermodule->default_timekeeper))
		return timekeepermodule;
	display_message(ERROR_MESSAGE, "Zinc.  Failed to create Timekeepermodule");
	delete timekeepermodule;
	return 0;
}

cmzn_timekeepermodule_id cmzn_timekeepermodule_access(
	cmzn_timekeepermodule_id timekeepermodule)
{
	if (timekeepermodule)
		return timekeepermodule->access();
	return 0;
}

int cmzn_timekeepermodule_destroy(cmzn_timekeepermodule_id *timekeepermodule_address)
{
	if (timekeepermodule_address)
		return cmzn_timekeepermodule::deaccess(*timekeepermodule_address);
	return CMZN_ERROR_ARGUMENT;
}

cmzn_timekeeper_id cmzn_timekeepermodule_get_default_timekeeper(
	cmzn_timekeepermodule_id timekeepermodule)
{
	if (timekeepermodule)
		return timekeepermodule->getDefaultTimekeeper()->access();
	return 0;
}

int cmzn_timekeepermodule_read_description(cmzn_timekeepermodule_id timekeepermodule,
	const char *description)
{
	if (timekeepermodule && description)
	{
		TimekeepermoduleJsonImport jsonImport(timekeepermodule);
		std::string inputString(description);
		return jsonImport.import(inputString);
	}
	return CMZN_ERROR_ARGUMENT;
}

char *cmzn_timekeepermodule_write_description(cmzn_timekeepermodule_id timekeepermodule)
{
	if (timekeepermodule)
	{
		TimekeepermoduleJsonExport jsonExport(timekeepermodule);
		return duplicate_string(jsonExport.getExportString().c_str());
	}
	return 0;
}
