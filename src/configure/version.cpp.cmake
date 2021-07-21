# /*OpenCMISS-Zinc
# *
# * This Source Code Form is subject to the terms of the Mozilla Public
# * License, v. 2.0. If a copy of the MPL was not distributed with this
# * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "opencmiss/zinc/status.h"
#include "opencmiss/zinc/context.h"
#include "general/mystring.h"
#include <string.h>

static const int ZINC_MAJOR_VERSION = @Zinc_VERSION_MAJOR@;
static const int ZINC_MINOR_VERSION = @Zinc_VERSION_MINOR@;
static const int ZINC_PATCH_VERSION = @Zinc_VERSION_PATCH@;
static const char* ZINC_REVISION = "@ZINC_REVISION_LONG@";
static const char* ZINC_BUILD_TYPE = "@CMAKE_BUILD_TYPE_LOWER@";

int cmzn_context_get_version(cmzn_context_id context, int *version_out)
{
	if (context && version_out)
	{
		version_out[0] = ZINC_MAJOR_VERSION;
		version_out[1] = ZINC_MINOR_VERSION;
		version_out[2] = ZINC_PATCH_VERSION;
		return CMZN_OK;
	}

	return CMZN_ERROR_ARGUMENT;
}

const char* cmzn_context_get_revision(cmzn_context_id context)
{
	if (context)
	{
		return ZINC_REVISION;
	}

	return 0;
}

char *cmzn_context_get_version_string(cmzn_context_id context)
{
	if (context)
	{
		char *version_string = new char[1000];
		sprintf(version_string, "%d.%d.%d",
			ZINC_MAJOR_VERSION, ZINC_MINOR_VERSION, ZINC_PATCH_VERSION);
		if (0 == strcmp(ZINC_BUILD_TYPE, "debug"))
		{
			sprintf(version_string, "%s.Debug",	version_string);
		}
		char *output_string = duplicate_string(version_string);
		delete[] version_string;
		return output_string;
	}

	return 0;
}
