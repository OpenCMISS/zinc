/***************************************************************************//**
 * FILE : fieldmodule_json_io.cpp
 *
 * The definition to fieldmodule_json_io.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string.h>
#include "computed_field/computed_field.h"
#include "description_io/field_json_io.hpp"
#include "description_io/fieldmodule_json_io.hpp"
#include "general/debug.h"
#include "opencmiss/zinc/field.hpp"
#include "opencmiss/zinc/status.h"

OpenCMISS::Zinc::Field FieldmoduleJsonImport::importField(Json::Value &fieldSettings)
{
	OpenCMISS::Zinc::Field field(0);
	field = fieldmodule.findFieldByName(fieldSettings["Name"].asCString());
	if (!field.isValid())
	{
		field = importTypeSpecificField(fieldmodule, fieldSettings, this);
		FieldJsonIO(field, fieldmodule, FieldJsonIO::IO_MODE_IMPORT).ioEntries(fieldSettings);
	}

	return field;

	/*
	OpenCMISS::Zinc::Field field = fieldmodule.findFieldByName(fieldName);
	if (!field.isValid())
	{
		spectrum = spectrummodule.createSpectrum();
		spectrum.setName(spectrumName);
	}
	FieldJsonIO(field, fieldmodule, IO_MODE_IMPORT).ioEntries(fieldSettings);
	*/
}

void FieldmoduleJsonImport::setManaged(Json::Value &fieldSettings)
{
	OpenCMISS::Zinc::Field field(0);
	field = fieldmodule.findFieldByName(fieldSettings["Name"].asCString());
	if (field.isValid())
	{
		if (fieldSettings["IsManaged"].isBool())
		{
			field.setManaged(fieldSettings["IsManaged"].asBool());
		}
	}
}

OpenCMISS::Zinc::Field FieldmoduleJsonImport::getFieldByName(const char *field_name)
{
	OpenCMISS::Zinc::Field field = fieldmodule.findFieldByName(field_name);
	if (!field.isValid())
	{
		unsigned int index = 0;
		while (index < fieldsList.size() || field.isValid())
		{
			Json::Value fieldJson = fieldsList[index];
			if (fieldJson["Name"].isString())
			{
				if ((0 == strcmp(fieldJson["Name"].asCString(), field_name)))
				{
					 field = this->importField(fieldJson);
					break;
				}
			}
			++index;
		}
	}

	return field;
}

int FieldmoduleJsonImport::import(const std::string &jsonString)
{
	int return_code = CMZN_ERROR_ARGUMENT;
	std::string returned_string;
	Json::Value root;

	if (Json::Reader().parse(jsonString, root, true))
	{
		fieldmodule.beginChange();
		if (root.isObject())
		{
			fieldsList = root["Fields"];
			for (unsigned int index = 0; index < fieldsList.size(); ++index )
			{
				importField(fieldsList[index]);
			}
		}
		return_code = CMZN_OK;
		fieldmodule.endChange();
	}

	return return_code;
}

std::string FieldmoduleJsonExport::getExportString()
{
	Json::Value root;

	OpenCMISS::Zinc::Fielditerator fielditerator =
		fieldmodule.createFielditerator();
	OpenCMISS::Zinc::Field field = fielditerator.next();
	while (field.isValid())
	{
		if (cmzn_field_get_type(field.getId()) != CMZN_FIELD_TYPE_INVALID)
		{
			Json::Value fieldSettings;
			FieldJsonIO(field, fieldmodule, FieldJsonIO::IO_MODE_EXPORT).ioEntries(fieldSettings);
			root["Fields"].append(fieldSettings);
		}
		field = fielditerator.next();
	}

	return Json::StyledWriter().write(root);
}
