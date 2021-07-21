/***************************************************************************//**
 * time_keeper.hpp
 *
 * Declaration of time keeper classes and functions.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef TIME_KEEPER_HPP
#define TIME_KEEPER_HPP

#include "opencmiss/zinc/status.h"
#include "opencmiss/zinc/timekeeper.h"
#include "general/mystring.h"
#include "general/object.h"
#include "general/debug.h"

PROTOTYPE_OBJECT_FUNCTIONS(cmzn_timekeeper);

#define Time_object cmzn_timenotifier

struct Time_object;

struct Time_object_info
{
	struct Time_object *time_object;
	double next_callback_due;
	struct Time_object_info *next;
};

struct cmzn_timekeeper
{
private:
	const char *name;
	double time;
	struct Time_object_info *time_object_info_list;
	double minimum, maximum;

public:

	int access_count;

	cmzn_timekeeper();

	virtual ~cmzn_timekeeper();

	inline cmzn_timekeeper *access()
	{
		++access_count;
		return this;
	}

	static inline int deaccess(cmzn_timekeeper **time_keeper_address)
	{
		return DEACCESS(cmzn_timekeeper)(time_keeper_address);
	}

	bool setName(const char *name_in);

	char *getName();

	int addTimeObject(struct Time_object *time_object);

	int removeTimeObject(struct Time_object *time_object);

	double getTime();

	double getMinimum();

	int setMinimum(double minimum_in);

	double getMaximum();

	int setMaximum(double maximum_in);

	int setTime(double new_time);

	int hasTimeObject();

	struct Time_object_info *getObjectInfo();

	void setTimeQuiet(double new_time);

	double getNextCallbackTime(cmzn_timekeeper_play_direction direction);
};

/** Designed for supporting multiple timekeepers in future.
 * Currently only holds default timekeeper
 */
struct cmzn_timekeepermodule
{
private:
	cmzn_timekeeper *default_timekeeper;
	int access_count;

	cmzn_timekeepermodule() :
		default_timekeeper(new cmzn_timekeeper()),
		access_count(1)
	{
	}

	~cmzn_timekeepermodule()
	{
		cmzn_timekeeper::deaccess(&(this->default_timekeeper));
	}

public:

	static cmzn_timekeepermodule *create();

	cmzn_timekeepermodule *access()
	{
		++(this->access_count);
		return this;
	}

	static int deaccess(cmzn_timekeepermodule* &timekeepermodule)
	{
		if (timekeepermodule)
		{
			--(timekeepermodule->access_count);
			if (timekeepermodule->access_count <= 0)
				delete timekeepermodule;
			timekeepermodule = 0;
			return CMZN_OK;
		}
		return CMZN_ERROR_ARGUMENT;
	}

	/** @return  Non-accessed default timekeeper */
	cmzn_timekeeper *getDefaultTimekeeper()
	{
		return this->default_timekeeper;
	}
};

#endif
