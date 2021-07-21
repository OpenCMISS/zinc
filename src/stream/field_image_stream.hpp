/***************************************************************************//**
 * FILE : cmzn_field_image_stream.hpp
 *
 * The private interface to cmzn_field_image_stream.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (CMZN_REGION_STREAM_HPP)
#define CMZN_REGION_STREAM_HPP

#include "opencmiss/zinc/field.h"
#include "opencmiss/zinc/fieldimage.h"
#include "general/image_utilities.h"
#include "stream/stream_private.hpp"

struct cmzn_streaminformation_image : cmzn_streaminformation
{
public:

	cmzn_streaminformation_image(cmzn_field_image_id image_field_in) :
		image_field(image_field_in)
	{
		cmzn_field_access(cmzn_field_image_base_cast(image_field_in));
		image_information = CREATE(Cmgui_image_information)();
	}

	virtual ~cmzn_streaminformation_image()
	{
		cmzn_field_image_destroy(&image_field);
		DESTROY(Cmgui_image_information)(&image_information);
	}

	Cmgui_image_information *getImageInformation()
	{
		return image_information;
	}

private:
	cmzn_field_image_id image_field;
	struct Cmgui_image_information *image_information;
};


#endif /* CMZN_REGION_STREAM_HPP */
