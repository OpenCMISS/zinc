/***************************************************************************//**
 * FILE : field_json_io.hpp
 *
 * The interface to field_json_io.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (FIELD_JSON_IO_HPP)
#define FIELD_JSON_IO_HPP

#include "opencmiss/zinc/field.h"
#include "opencmiss/zinc/field.hpp"
#include "opencmiss/zinc/fieldmodule.h"
#include "opencmiss/zinc/fieldmodule.hpp"
#include "description_io/fieldmodule_json_io.hpp"
#include "jsoncpp/json.h"
#include <string>

OpenCMISS::Zinc::Field importTypeSpecificField(
	OpenCMISS::Zinc::Fieldmodule &fieldmodule, Json::Value &fieldSettings,
	FieldmoduleJsonImport *jsonImport);

class FieldJsonIO
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

	FieldJsonIO(cmzn_field_id field_in, cmzn_fieldmodule_id fieldmodule_in, IOMode mode_in) :
		field(cmzn_field_access(field_in)), fieldmodule(cmzn_fieldmodule_access(fieldmodule_in)),
		mode(mode_in)
	{  }

	FieldJsonIO(const OpenCMISS::Zinc::Field &field_in,
		const OpenCMISS::Zinc::Fieldmodule &fieldmodule_in, IOMode mode_in) :
		field(field_in), fieldmodule(fieldmodule_in), mode(mode_in)
	{	}

	/* serialise/deserialise fields definition from json format */
	void ioEntries(Json::Value &typeSettings);

	/* methods to customise output based on field type */
	void exportTypeSpecificParameters(Json::Value &fieldSettings);

	void ioFiniteElementEntries(Json::Value &fieldSettings, Json::Value &typeSettings);

protected:

	OpenCMISS::Zinc::Field field;
	OpenCMISS::Zinc::Fieldmodule fieldmodule;
	IOMode mode;

};

#endif

