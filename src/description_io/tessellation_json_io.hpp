/***************************************************************************//**
 * FILE : tessellation_json_io.hpp
 *
 * The interface to tessellation_json_io.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (TESSELLATION_JSON_IO_HPP)
#define TESSELLATION_JSON_IO_HPP

#include "opencmiss/zinc/tessellation.hpp"
#include <string>
#include "jsoncpp/json.h"

enum IOMode
{
	IO_MODE_INVALID = 0,
	IO_MODE_IMPORT = 1,
	IO_MODE_EXPORT= 2
};

/*
 * Class to import attributes into tessellation.
 */
class TessellationJsonIO
{

public:

	TessellationJsonIO(cmzn_tessellation_id tessellation_in, IOMode mode_in) :
		tessellation(cmzn_tessellation_access(tessellation_in)), mode(mode_in)
	{  }

	TessellationJsonIO(const OpenCMISS::Zinc::Tessellation tessellation_in, IOMode mode_in) :
		tessellation(tessellation_in), mode(mode_in)
	{	}

	void ioEntries(Json::Value &tessellationSettings);

private:
	OpenCMISS::Zinc::Tessellation tessellation;
	IOMode mode;

};

/*
 * Class to import attributes into tessellation module.
 */
class TessellationmoduleJsonImport
{

private:
	OpenCMISS::Zinc::Tessellationmodule tessellationmodule;

public:

	TessellationmoduleJsonImport(cmzn_tessellationmodule_id tessellationmodule_in) :
		tessellationmodule(cmzn_tessellationmodule_access(tessellationmodule_in))
	{  }

	int import(const std::string &jsonString);

	void importTessellation(Json::Value &tessellationSettings);
};

/*
 * Class to export attributes from tessellation module.
 */
class TessellationmoduleJsonExport
{
private:
	OpenCMISS::Zinc::Tessellationmodule tessellationmodule;

public:

	TessellationmoduleJsonExport(cmzn_tessellationmodule_id tessellationmodule_in) :
		tessellationmodule(cmzn_tessellationmodule_access(tessellationmodule_in))
	{  }

	std::string getExportString();
};

#endif
