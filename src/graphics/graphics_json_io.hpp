/***************************************************************************//**
 * FILE : graphics_json_io.hpp
 *
 * The interface to graphics_json_io.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (GRAPHICS_JSON_IO_HPP)
#define GRAPHICS_JSON_IO_HPP

#include "opencmiss/zinc/graphics.h"
#include "opencmiss/zinc/graphics.hpp"
#include "jsoncpp/json.h"
#include <string>

class GraphicsJsonIO
{

public:

	/*
	 * With order set to positive integer, order will be use as key.
	 */

	enum IOMode
	{
		IO_MODE_INVALID = 0,
		IO_MODE_IMPORT = 1,
		IO_MODE_EXPORT= 2
	};

	GraphicsJsonIO(cmzn_graphics_id graphics_in, IOMode mode_in) :
		graphics(cmzn_graphics_access(graphics_in)), mode(mode_in)
	{  }

	GraphicsJsonIO(const OpenCMISS::Zinc::Graphics &graphics_in, IOMode mode_in) :
		graphics(graphics_in), mode(mode_in)
	{	}

protected:

	OpenCMISS::Zinc::Graphics graphics;
	int order;
	IOMode mode;

	void ioEntries();

	void ioGeneralEntries(Json::Value &graphicsSettings);

	void ioGeneralBoolEntries(Json::Value &graphicsSettings);

	void ioGeneralDobuleEntries(Json::Value &graphicsSettings);

	void ioGeneralEnumEntries(Json::Value &graphicsSettings);

	void ioGeneralFieldEntries(Json::Value &graphicsSettings);

	void ioGeneralObjectEntries(Json::Value &graphicsSettings);

	void ioAttributesEntries(Json::Value &graphicsSettings);

	void ioLineAttributesEntries(Json::Value &graphicsSettings);

	void ioPointAttributesEntries(Json::Value &graphicsSettings);

	void ioSamplingAttributesEntries(Json::Value &graphicsSettings);

	void ioContoursEntries(Json::Value &graphicsSettings);

	void ioLinesEntries(Json::Value &graphicsSettings);

	void ioPointsEntries(Json::Value &graphicsSettings);

	void ioStreamlinesEntries(Json::Value &graphicsSettings);

	void ioSurfacesEntries(Json::Value &graphicsSettings);

	void ioTypeEntries(Json::Value &graphicsSettings);

};

#endif

