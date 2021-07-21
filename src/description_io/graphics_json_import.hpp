/***************************************************************************//**
 * FILE : graphics_json_import.hpp
 *
 * The interface to graphics_json_import.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (GRAPHICS_JSON_IMPORT_HPP)
#define GRAPHICS_JSON_IMPORT_HPP

#include "opencmiss/zinc/graphics.h"
#include "opencmiss/zinc/graphics.hpp"
#include "description_io/graphics_json_io.hpp"
#include "jsoncpp/json.h"
#include <string>

class GraphicsJsonImport : GraphicsJsonIO
{

public:

	/*
	 * With order set to positive integer, order will be use as key.
	 */
	GraphicsJsonImport(cmzn_graphics_id graphics_in, Json::Value &graphicsJsonIn) :
		GraphicsJsonIO(graphics_in, GraphicsJsonIO::IO_MODE_IMPORT), graphicsJson(graphicsJsonIn)
	{  }

	GraphicsJsonImport(const OpenCMISS::Zinc::Graphics &graphics_in, Json::Value &graphicsJsonIn) :
		GraphicsJsonIO(graphics_in, GraphicsJsonIO::IO_MODE_IMPORT), graphicsJson(graphicsJsonIn)
	{	}

	int import();

private:

	Json::Value& graphicsJson;

};

#endif
