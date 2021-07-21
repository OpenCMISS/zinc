/***************************************************************************//**
 * FILE : graphics_json_export.cpp
 *
 * The definition to graphics_json_export.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

//#include "opencmiss/zinc/spectrum.h"
//#include "opencmiss/zinc/spectrum.hpp"
#include "general/debug.h"
#include "graphics/graphics_json_export.hpp"
#include "graphics/graphics.h"

void GraphicsJsonExport::addEntries()
{
	root.clear();
	cmzn_graphics_id c_graphics = graphics.getId();
	char *name = graphics.getName();
	Json::Value graphicsSettings;
	if (name)
	{
		graphicsSettings["id"] = name;
		DEALLOCATE(name);
	}
	enum cmzn_graphics_type type = cmzn_graphics_get_type(c_graphics);
	char *type_string = cmzn_graphics_type_enum_to_string(type);
	graphicsSettings["Type"] = type_string;
	ioTypeEntries(graphicsSettings);
	ioGeneralEntries(graphicsSettings);
	ioAttributesEntries(graphicsSettings);
	if (order > 0)
	{
		char order_string[5];
		sprintf(order_string, "%d", order);
		root[order_string] = graphicsSettings;
	}
	else
	{
		root = graphicsSettings;
	}

	if (type_string)
		DEALLOCATE(type_string);
}

std::string GraphicsJsonExport::getExportString()
{
	std::string returned_string;
	if (graphics.isValid())
	{
		this->addEntries();
		returned_string = Json::StyledWriter().write(root);
	}
	return returned_string;
}

Json::Value *GraphicsJsonExport::exportJsonValue()
{
	if (graphics.isValid())
	{
		this->addEntries();
		return &root;
	}
	return 0;
}
