/***************************************************************************//**
 * FILE : scene_json_export.hpp
 *
 * The interface to scene_json_export.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (SCENE_JSON_EXPORT_HPP)
#define SCENE_JSON_EXPORT_HPP

#include "opencmiss/zinc/scene.h"
#include "opencmiss/zinc/scene.hpp"
#include "jsoncpp/json.h"
#include <string>

class SceneJsonExport
{

public:

	SceneJsonExport(cmzn_scene_id scene_in) :	scene(cmzn_scene_access(scene_in))
	{  }

	std::string getExportString();

private:
	OpenCMISS::Zinc::Scene scene;
	Json::Value root;
};

#endif
