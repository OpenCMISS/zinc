/***************************************************************************//**
 * FILE : timekeeper_json_io.hpp
 *
 * The interface to timekeeper_json_io.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (TIMEKEPER_JSON_IO_HPP)
#define TIMEKEPER_JSON_IO_HPP

#include "opencmiss/zinc/timekeeper.hpp"
#include <string>
#include "jsoncpp/json.h"

enum IOMode
{
	IO_MODE_INVALID = 0,
	IO_MODE_IMPORT = 1,
	IO_MODE_EXPORT= 2
};

/*
 * Class to import attributes into timekeeper.
 */
class TimekeeperJsonIO
{

public:

	TimekeeperJsonIO(cmzn_timekeeper_id timekeeper_in, IOMode mode_in) :
		timekeeper(cmzn_timekeeper_access(timekeeper_in)), mode(mode_in)
	{  }

	TimekeeperJsonIO(const OpenCMISS::Zinc::Timekeeper timekeeper_in, IOMode mode_in) :
		timekeeper(timekeeper_in), mode(mode_in)
	{	}

	void ioEntries(Json::Value &timekeeperSettings);

private:
	OpenCMISS::Zinc::Timekeeper timekeeper;
	IOMode mode;
};

/*
 * Class to import attributes into timekeeper module.
 */
class TimekeepermoduleJsonImport
{

private:
	OpenCMISS::Zinc::Timekeepermodule timekeepermodule;

public:

	TimekeepermoduleJsonImport(cmzn_timekeepermodule_id timekeepermodule_in) :
		timekeepermodule(cmzn_timekeepermodule_access(timekeepermodule_in))
	{  }

	int import(const std::string &jsonString);

	void importTimekeeper(Json::Value &timekeeperSettings);
};

/*
 * Class to export attributes from timekeeper module.
 */
class TimekeepermoduleJsonExport
{
private:
	OpenCMISS::Zinc::Timekeepermodule timekeepermodule;

public:

	TimekeepermoduleJsonExport(cmzn_timekeepermodule_id timekeepermodule_in) :
		timekeepermodule(cmzn_timekeepermodule_access(timekeepermodule_in))
	{  }

	std::string getExportString();
};

#endif
