/***************************************************************************//**
 * FILE : scene_json_export.cpp
 *
 * The definition to graphics_json_export.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "general/debug.h"
#include "opencmiss/zinc/graphics.hpp"
#include "description_io/graphics_json_export.hpp"
#include "description_io/scene_json_export.hpp"
#include "graphics/scene.hpp"

std::string SceneJsonExport::getExportString()
{
	std::string returned_string;

	Json::Value scene_settings;
	bool visibility = scene.getVisibilityFlag();
	root["VisibilityFlag"] = visibility;
	Json::Value graphics_settings;
	OpenCMISS::Zinc::Graphics graphics = scene.getFirstGraphics();
	while (graphics.isValid())
	{
		GraphicsJsonExport graphicsJsonExport = GraphicsJsonExport(graphics, -1);
		Json::Value *tempValue = graphicsJsonExport.exportJsonValue();
		graphics_settings.append(*tempValue);
		graphics = scene.getNextGraphics(graphics);
	}
	root["Graphics"] = graphics_settings;
	if (this->scene.hasTransformation())
	{
		OpenCMISS::Zinc::Field field = this->scene.getTransformationField();
		if (field.isValid())
		{
			char *fieldName = field.getName();
			root["TransformationField"] = fieldName;
			DEALLOCATE(fieldName);
		}
		else
		{
			double values[16];
			this->scene.getTransformationMatrix(values);
			Json::Value& v = root["TransformationMatrix"];
			v.resize(16);
			for (int i = 0; i < 16; ++i)
				v[i] = values[i];
		}
	}
	returned_string = Json::StyledWriter().write(root);

	return returned_string;
}
