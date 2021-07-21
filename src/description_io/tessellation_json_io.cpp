/***************************************************************************//**
 * FILE : tessellation_json_io.cpp
 *
 * The definition to tessellation_json_io.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "description_io/tessellation_json_io.hpp"
#include "general/debug.h"
#include "opencmiss/zinc/tessellation.hpp"
#include "opencmiss/zinc/status.h"

void TessellationJsonIO::ioEntries(Json::Value &tessellationSettings)
{
	if (mode == IO_MODE_EXPORT)
	{
		char *name = tessellation.getName();
		tessellationSettings["Name"] = name;
		DEALLOCATE(name);
		tessellationSettings["CircleDivisions"] = tessellation.getCircleDivisions();
		int valuesCount = tessellation.getMinimumDivisions(0, 0);
		int *intValues = new int[valuesCount];
		tessellation.getMinimumDivisions(valuesCount, intValues);
		for (int i = 0; i < valuesCount; i++)
		{
			tessellationSettings["MinimumDivisions"].append(intValues[i]);
		}
		delete[] intValues;
		valuesCount = tessellation.getRefinementFactors(0, 0);
		intValues = new int[valuesCount];
		tessellation.getRefinementFactors(valuesCount, intValues);
		for (int i = 0; i < valuesCount; i++)
		{
			tessellationSettings["RefinementFactors"].append(intValues[i]);
		}
		delete[] intValues;
	}
	else
	{
		if (tessellationSettings["Name"].isString())
		{
			tessellation.setName(tessellationSettings["Name"].asCString());
		}
		if (tessellationSettings["CircleDivisions"].isInt())
		{
			tessellation.setCircleDivisions(tessellationSettings["CircleDivisions"].asInt());
		}
		if (tessellationSettings["MinimumDivisions"].isArray())
		{
			int *intValues = new int[tessellationSettings["MinimumDivisions"].size()];
			for (unsigned int i = 0; i < tessellationSettings["MinimumDivisions"].size(); i++)
			{
				intValues[i] = tessellationSettings["MinimumDivisions"][i].asInt();
			}
			tessellation.setMinimumDivisions(tessellationSettings["MinimumDivisions"].size(),
				intValues);
			delete[] intValues;
			intValues = new int[tessellationSettings["RefinementFactors"].size()];
			for (unsigned int i = 0; i < tessellationSettings["RefinementFactors"].size(); i++)
			{
				intValues[i] = tessellationSettings["RefinementFactors"][i].asInt();
			}
			tessellation.setRefinementFactors(tessellationSettings["RefinementFactors"].size(),
				intValues);
			delete[] intValues;
		}
		tessellation.setManaged(true);
	}
}

int TessellationmoduleJsonImport::import(const std::string &jsonString)
{
	int return_code = CMZN_ERROR_ARGUMENT;
	std::string returned_string;
	Json::Value root;

	if (Json::Reader().parse(jsonString, root, true))
	{
		tessellationmodule.beginChange();
		if (root.isObject())
		{
			Json::Value tessellationJson = root["Tessellations"];
			for (unsigned int index = 0; index < tessellationJson.size(); ++index )
			{
				importTessellation(tessellationJson[index]);
			}
		}
		if (root["DefaultTessellation"].isString())
		{
			tessellationmodule.setDefaultTessellation(tessellationmodule.findTessellationByName(
				root["DefaultTessellation"].asCString()));
		}
		if (root["DefaultPointsTessellation"].isString())
		{
			tessellationmodule.setDefaultPointsTessellation(tessellationmodule.findTessellationByName(
				root["DefaultPointsTessellation"].asCString()));
		}
		return_code = CMZN_OK;
		tessellationmodule.endChange();
	}

	return return_code;
}

void TessellationmoduleJsonImport::importTessellation(Json::Value &tessellationSettings)
{
	const char *tessellationName = tessellationSettings["Name"].asCString();
	OpenCMISS::Zinc::Tessellation tessellation = tessellationmodule.findTessellationByName(tessellationName);
	if (!tessellation.isValid())
	{
		tessellation = tessellationmodule.createTessellation();
		tessellation.setName(tessellationName);
	}
	TessellationJsonIO(tessellation, IO_MODE_IMPORT).ioEntries(tessellationSettings);
}

std::string TessellationmoduleJsonExport::getExportString()
{
	Json::Value root;

	OpenCMISS::Zinc::Tessellationiterator tessellationiterator =
		tessellationmodule.createTessellationiterator();
	OpenCMISS::Zinc::Tessellation tessellation = tessellationiterator.next();
	while (tessellation.isValid())
	{
		Json::Value tessellationSettings;
		TessellationJsonIO(tessellation, IO_MODE_EXPORT).ioEntries(tessellationSettings);
		root["Tessellations"].append(tessellationSettings);
		tessellation = tessellationiterator.next();
	}
	char *defaultTessellationName = tessellationmodule.getDefaultTessellation().getName();
	root["DefaultTessellation"] = defaultTessellationName;
	DEALLOCATE(defaultTessellationName);
	char *defaultPointsTessellationName = tessellationmodule.getDefaultPointsTessellation().getName();
	root["DefaultPointsTessellation"] = defaultPointsTessellationName;
	DEALLOCATE(defaultPointsTessellationName);

	return Json::StyledWriter().write(root);
}
