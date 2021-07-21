/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if defined (BUILD_WITH_CMAKE)
#include "configure/cmgui_configure.h"
#endif /* defined (BUILD_WITH_CMAKE) */

#include "cad/cad_element.h"

extern "C" {
#include "user_interface/message.h"
}

const char *Cad_primitive_type_string(Cad_primitive_type type)
{
	const char *return_string;

	switch (type)
	{
		case Cad_primitive_INVALID:
		{
			return_string="invalid";
		} break;
		case Cad_primitive_SURFACE:
		{
			return_string="surface";
		} break;
		case Cad_primitive_CURVE:
		{
			return_string="curve";
		} break;
		case Cad_primitive_POINT:
		{
			return_string="point";
		} break;
		default:
		{
			display_message(ERROR_MESSAGE,
				"Cad_primitive_type_string.  Unknown CAD element type");
			return_string = static_cast<const char *>(0);
		} break;
	}

	return (return_string);
}

