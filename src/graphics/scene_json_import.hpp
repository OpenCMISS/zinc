/***************************************************************************//**
 * FILE : scene_json_import.hpp
 *
 * The interface to scene_json_export.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (SCENE_JSON_IMPORT_HPP)
#define SCENE_JSON_IMPORT_HPP

#include "opencmiss/zinc/scene.h"
#include "opencmiss/zinc/scene.hpp"
#include <string>
#include "jsoncpp/json.h"

class SceneJsonImport
{

public:

	SceneJsonImport(cmzn_scene_id scene_in, int overwrite_in) :
		scene(cmzn_scene_access(scene_in)),
		overwrite(overwrite_in)
	{  }

	int import(std::string &jsonString);

	void importGraphics(Json::Value &graphicsJson);

private:
	OpenCMISS::Zinc::Scene scene;
	Json::Value root;
	int overwrite;
};

#endif
