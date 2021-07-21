/***************************************************************************//**
 * FILE : spectrum_json_io.hpp
 *
 * The interface to spectrum_json_io.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (SPECTRUM_JSON_IO_HPP)
#define SPECTRUM_JSON_IO_HPP

#include "opencmiss/zinc/spectrum.hpp"
#include <string>
#include "jsoncpp/json.h"

enum IOMode
{
	IO_MODE_INVALID = 0,
	IO_MODE_IMPORT = 1,
	IO_MODE_EXPORT= 2
};

/*
 * Class to import/export attributes into/from spectrum.
 */
class SpectrumcomponentJsonIO
{

public:

	SpectrumcomponentJsonIO(cmzn_spectrumcomponent_id spectrumcomponent_in, IOMode mode_in) :
		spectrumcomponent(cmzn_spectrumcomponent_access(spectrumcomponent_in)), mode(mode_in)
	{  }

	SpectrumcomponentJsonIO(const OpenCMISS::Zinc::Spectrumcomponent spectrumcomponent_in, IOMode mode_in) :
		spectrumcomponent(spectrumcomponent_in), mode(mode_in)
	{	}

	void ioEntries(Json::Value &componentSettings);

protected:

	OpenCMISS::Zinc::Spectrumcomponent spectrumcomponent;
	IOMode mode;

	void ioBoolEntries(Json::Value &componentSettings);

	void ioEnumEntries(Json::Value &componentSettings);

	void ioParameterEntries(Json::Value &componentSettings);

};

/*
 * Class to import attributes into spectrum.
 */
class SpectrumJsonIO
{

public:

	SpectrumJsonIO(cmzn_spectrum_id spectrum_in, IOMode mode_in) :
		spectrum(cmzn_spectrum_access(spectrum_in)), mode(mode_in)
	{  }

	SpectrumJsonIO(const OpenCMISS::Zinc::Spectrum spectrum_in, IOMode mode_in) :
		spectrum(spectrum_in), mode(mode_in)
	{	}

	void ioEntries(Json::Value &spectrumSettings);

private:
	OpenCMISS::Zinc::Spectrum spectrum;
	IOMode mode;

};

/*
 * Class to import attributes into spectrum module.
 */
class SpectrummoduleJsonImport
{

private:
	OpenCMISS::Zinc::Spectrummodule spectrummodule;

public:

	SpectrummoduleJsonImport(cmzn_spectrummodule_id spectrummodule_in) :
		spectrummodule(cmzn_spectrummodule_access(spectrummodule_in))
	{  }

	int import(const std::string &jsonString);

	void importSpectrum(Json::Value &spectrumSettings);
};

/*
 * Class to export attributes from spectrum module.
 */
class SpectrummoduleJsonExport
{
private:
	OpenCMISS::Zinc::Spectrummodule spectrummodule;

public:

	SpectrummoduleJsonExport(cmzn_spectrummodule_id spectrummodule_in) :
		spectrummodule(cmzn_spectrummodule_access(spectrummodule_in))
	{  }

	std::string getExportString();
};

char *cmzn_spectrumcomponent_scale_type_enum_to_string(
	enum cmzn_spectrumcomponent_scale_type scale_type);

enum cmzn_spectrumcomponent_scale_type cmzn_spectrumcomponent_scale_type_enum_from_string(
	const char *string);

#endif
