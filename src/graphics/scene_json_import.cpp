/***************************************************************************//**
 * FILE : scene_json_import.cpp
 *
 * The definition to scene_json_import.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "general/debug.h"
#include "opencmiss/zinc/graphics.hpp"
#include "graphics/graphics_json_import.hpp"
#include "graphics/scene_json_import.hpp"
#include "graphics/scene.h"
#include "opencmiss/zinc/status.h"

int SceneJsonImport::import(std::string &jsonString)
{
	int return_code = CMZN_ERROR_ARGUMENT;
	std::string returned_string;
	Json::Value root;

	if (Json::Reader().parse(jsonString, root, true))
	{
		if (overwrite == 1)
		{
			scene.removeAllGraphics();
		}
		if (root.isObject())
		{
			for (unsigned int index = 1; index < (root.size() + 1); ++index )
			{
				char temp_id[10];
				sprintf(temp_id, "%d", index);
				Json::Value graphicsJson = root[temp_id];
				importGraphics(graphicsJson);
			}
		}

		return_code = CMZN_OK;
	}

	return return_code;
}

void SceneJsonImport::importGraphics(Json::Value &graphicsJson)
{
	std::string typeString = graphicsJson["Type"].asString();
	enum OpenCMISS::Zinc::Graphics::Type graphicsType =  static_cast<OpenCMISS::Zinc::Graphics::Type>(
		cmzn_graphics_type_enum_from_string(typeString.c_str()));
	OpenCMISS::Zinc::Graphics graphics = scene.createGraphics(graphicsType);
	GraphicsJsonImport(graphics, graphicsJson).import();
}
