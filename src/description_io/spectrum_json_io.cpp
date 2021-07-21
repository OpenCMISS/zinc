/***************************************************************************//**
 * FILE : spectrum_json_io.cpp
 *
 * The definition to spectrum_json_io.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "description_io/spectrum_json_io.hpp"
#include "general/debug.h"
#include "opencmiss/zinc/spectrum.hpp"
#include "opencmiss/zinc/status.h"

void SpectrumcomponentJsonIO::ioEntries(Json::Value &componentSettings)
{
	ioBoolEntries(componentSettings);
	ioEnumEntries(componentSettings);
	ioParameterEntries(componentSettings);
}

void SpectrumcomponentJsonIO::ioBoolEntries(Json::Value &componentSettings)
{
	if (mode == IO_MODE_EXPORT)
	{
		componentSettings["Active"] = spectrumcomponent.isActive();
		componentSettings["ColourReverse"] = spectrumcomponent.isColourReverse();
		componentSettings["ExtendAbove"] = spectrumcomponent.isExtendAbove();
		componentSettings["ExtendBelow"] = spectrumcomponent.isExtendBelow();
		componentSettings["FixMaximum"] = spectrumcomponent.isFixMaximum();
		componentSettings["FixMinimum"] = spectrumcomponent.isFixMinimum();
	}
	else
	{
		if (componentSettings["Active"].isBool())
			spectrumcomponent.setActive(componentSettings["Active"].asBool());
		if (componentSettings["ColourReverse"].isBool())
			spectrumcomponent.setColourReverse(componentSettings["ColourReverse"].asBool());
		if (componentSettings["ExtendAbove"].isBool())
			spectrumcomponent.setExtendAbove(componentSettings["ExtendAbove"].asBool());
		if (componentSettings["ExtendBelow"].isBool())
			spectrumcomponent.setExtendBelow(componentSettings["ExtendBelow"].asBool());
		if (componentSettings["FixMinimum"].isBool())
			spectrumcomponent.setFixMinimum(componentSettings["FixMinimum"].asBool());
		if (componentSettings["FixMaximum"].isBool())
			spectrumcomponent.setFixMaximum(componentSettings["FixMaximum"].asBool());
	}
}

void SpectrumcomponentJsonIO::ioEnumEntries(Json::Value &componentSettings)
{
	if (mode == IO_MODE_EXPORT)
	{
		char *return_string = cmzn_spectrumcomponent_colour_mapping_type_enum_to_string(
			cmzn_spectrumcomponent_get_colour_mapping_type(spectrumcomponent.getId()));
		componentSettings["ColourMappingType"] = return_string;
		DEALLOCATE(return_string);
		return_string = cmzn_spectrumcomponent_scale_type_enum_to_string(
			cmzn_spectrumcomponent_get_scale_type(spectrumcomponent.getId()));
		componentSettings["ScaleType"] = return_string;
		DEALLOCATE(return_string);
	}
	else
	{
		if (componentSettings["ColourMappingType"].isString())
			cmzn_spectrumcomponent_set_colour_mapping_type(spectrumcomponent.getId(),
				cmzn_spectrumcomponent_colour_mapping_type_enum_from_string(
					componentSettings["ColourMappingType"].asCString()));
		if (componentSettings["ScaleType"].isString())
			cmzn_spectrumcomponent_set_scale_type(spectrumcomponent.getId(),
				cmzn_spectrumcomponent_scale_type_enum_from_string(
					componentSettings["ScaleType"].asCString()));
	}
}

void SpectrumcomponentJsonIO::ioParameterEntries(Json::Value &componentSettings)
{
	if (mode == IO_MODE_EXPORT)
	{
		componentSettings["RangeMinimum"] = spectrumcomponent.getRangeMinimum();
		componentSettings["RangeMaximum"] = spectrumcomponent.getRangeMaximum();
		componentSettings["ColourMinimum"] = spectrumcomponent.getColourMinimum();
		componentSettings["ColourMaximum"] = spectrumcomponent.getColourMaximum();
		componentSettings["StepValue"] = spectrumcomponent.getStepValue();
		componentSettings["Exaggeration"] = spectrumcomponent.getExaggeration();
		componentSettings["BandedRatio"] = spectrumcomponent.getBandedRatio();
		componentSettings["FieldComponent"] = spectrumcomponent.getFieldComponent();
		componentSettings["NumberOfBands"] = spectrumcomponent.getNumberOfBands();
	}
	else
	{
		if (componentSettings["RangeMinimum"].isDouble())
			spectrumcomponent.setRangeMinimum(componentSettings["RangeMinimum"].asDouble());
		if (componentSettings["RangeMaximum"].isDouble())
			spectrumcomponent.setRangeMaximum(componentSettings["RangeMaximum"].asDouble());
		if (componentSettings["ColourMinimum"].isDouble())
			spectrumcomponent.setColourMinimum(componentSettings["ColourMinimum"].asDouble());
		if (componentSettings["ColourMaximum"].isDouble())
			spectrumcomponent.setColourMaximum(componentSettings["ColourMaximum"].asDouble());
		if (componentSettings["StepValue"].isDouble())
			spectrumcomponent.setStepValue(componentSettings["StepValue"].asDouble());
		if (componentSettings["Exaggeration"].isDouble())
			spectrumcomponent.setExaggeration(componentSettings["Exaggeration"].asDouble());
		if (componentSettings["BandedRatio"].isDouble())
			spectrumcomponent.setBandedRatio(componentSettings["BandedRatio"].asDouble());
		if (componentSettings["FieldComponent"].isInt())
			spectrumcomponent.setFieldComponent(componentSettings["FieldComponent"].asDouble());
		if (componentSettings["NumberOfBands"].isInt())
			spectrumcomponent.setNumberOfBands(componentSettings["NumberOfBands"].asDouble());
	}
}

void SpectrumJsonIO::ioEntries(Json::Value &spectrumSettings)
{
	if (mode == IO_MODE_EXPORT)
	{
		char *name = spectrum.getName();
		spectrumSettings["Name"] = name;
		DEALLOCATE(name);
		spectrumSettings["MaterialOverwrite"] = spectrum.isMaterialOverwrite();
		OpenCMISS::Zinc::Spectrumcomponent spectrumcomponent = spectrum.getFirstSpectrumcomponent();
		while (spectrumcomponent.isValid())
		{
			Json::Value componentSettings;
			SpectrumcomponentJsonIO(spectrumcomponent, IO_MODE_EXPORT).ioEntries(componentSettings);
			spectrumSettings["Components"].append(componentSettings);
			spectrumcomponent = spectrum.getNextSpectrumcomponent(spectrumcomponent);
		}
	}
	else
	{
		spectrum.beginChange();
		if (spectrumSettings["Name"].isString())
		{
			spectrum.setName(spectrumSettings["Name"].asCString());
		}
		if (spectrumSettings["MaterialOverwrite"].isBool())
		{
			spectrum.setMaterialOverwrite(spectrumSettings["MaterialOverwrite"].asBool());
		}
		spectrum.removeAllSpectrumcomponents();
		Json::Value componentSettings = spectrumSettings["Components"];
		for (unsigned int index = 0; index < componentSettings.size(); ++index )
		{
			OpenCMISS::Zinc::Spectrumcomponent spectrumcomponent = spectrum.createSpectrumcomponent();
			SpectrumcomponentJsonIO(spectrumcomponent, IO_MODE_IMPORT).ioEntries(componentSettings[index]);
		}
		spectrum.setManaged(true);
		spectrum.endChange();
	}
}

int SpectrummoduleJsonImport::import(const std::string &jsonString)
{
	int return_code = CMZN_ERROR_ARGUMENT;
	std::string returned_string;
	Json::Value root;

	if (Json::Reader().parse(jsonString, root, true))
	{
		spectrummodule.beginChange();
		if (root.isObject())
		{
			Json::Value spectrumJson = root["Spectrums"];
			for (unsigned int index = 0; index < spectrumJson.size(); ++index )
			{
				importSpectrum(spectrumJson[index]);
			}
		}
		if (root["DefaultSpectrum"].isString())
		{
			spectrummodule.setDefaultSpectrum(spectrummodule.findSpectrumByName(
				root["DefaultSpectrum"].asCString()));
		}
		return_code = CMZN_OK;
		spectrummodule.endChange();
	}

	return return_code;
}

void SpectrummoduleJsonImport::importSpectrum(Json::Value &spectrumSettings)
{
	const char *spectrumName = spectrumSettings["Name"].asCString();
	OpenCMISS::Zinc::Spectrum spectrum = spectrummodule.findSpectrumByName(spectrumName);
	if (!spectrum.isValid())
	{
		spectrum = spectrummodule.createSpectrum();
		spectrum.setName(spectrumName);
	}
	SpectrumJsonIO(spectrum, IO_MODE_IMPORT).ioEntries(spectrumSettings);
}

std::string SpectrummoduleJsonExport::getExportString()
{
	Json::Value root;

	OpenCMISS::Zinc::Spectrumiterator spectrumiterator =
		spectrummodule.createSpectrumiterator();
	OpenCMISS::Zinc::Spectrum spectrum = spectrumiterator.next();
	while (spectrum.isValid())
	{
		Json::Value spectrumSettings;
		SpectrumJsonIO(spectrum, IO_MODE_EXPORT).ioEntries(spectrumSettings);
		root["Spectrums"].append(spectrumSettings);
		spectrum = spectrumiterator.next();
	}
	char *defaultSpectrumName = spectrummodule.getDefaultSpectrum().getName();
	root["DefaultSpectrum"] = defaultSpectrumName;
	DEALLOCATE(defaultSpectrumName);

	return Json::StyledWriter().write(root);
}
