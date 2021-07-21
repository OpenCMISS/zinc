/**
 * FILE: scene_coordinate_system.cpp
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string.h>
#include "general/debug.h"
#include "general/mystring.h"
#include "general/message.h"
#include "general/enumerator_conversion.hpp"
#include "general/enumerator_private.hpp"
#include "graphics/scene_coordinate_system.hpp"

class cmzn_scenecoordinatesystem_conversion
{
public:
	static const char *to_string(enum cmzn_scenecoordinatesystem system)
	{
		const char *enum_string = 0;
		switch (system)
		{
		case CMZN_SCENECOORDINATESYSTEM_LOCAL:
			enum_string = "LOCAL";
			break;
		case CMZN_SCENECOORDINATESYSTEM_WORLD:
			enum_string = "WORLD";
			break;
		case CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FILL:
			enum_string = "NORMALISED_WINDOW_FILL";
			break;
		case CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FIT_CENTRE:
			enum_string = "NORMALISED_WINDOW_FIT_CENTRE";
			break;
		case CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FIT_LEFT:
			enum_string = "NORMALISED_WINDOW_FIT_LEFT";
			break;
		case CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FIT_RIGHT:
			enum_string = "NORMALISED_WINDOW_FIT_RIGHT";
			break;
		case CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FIT_BOTTOM:
			enum_string = "NORMALISED_WINDOW_FIT_BOTTOM";
			break;
		case CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FIT_TOP:
			enum_string = "NORMALISED_WINDOW_FIT_TOP";
			break;
		case CMZN_SCENECOORDINATESYSTEM_WINDOW_PIXEL_BOTTOM_LEFT:
			enum_string = "WINDOW_PIXEL_BOTTOM_LEFT";
			break;
		case CMZN_SCENECOORDINATESYSTEM_WINDOW_PIXEL_TOP_LEFT:
			enum_string = "WINDOW_PIXEL_TOP_LEFT";
			break;
		default:
			break;
		}
		return enum_string;
	}
};

enum cmzn_scenecoordinatesystem	cmzn_scenecoordinatesystem_enum_from_string(
	const char *string)
{
	return string_to_enum<enum cmzn_scenecoordinatesystem,
		cmzn_scenecoordinatesystem_conversion>(string);
}

char *cmzn_scenecoordinatesystem_enum_to_string(
	enum cmzn_scenecoordinatesystem coordinate_system)
{
	const char *system_string = cmzn_scenecoordinatesystem_conversion::to_string(coordinate_system);
	return (system_string ? duplicate_string(system_string) : 0);
}

PROTOTYPE_ENUMERATOR_STRING_FUNCTION(cmzn_scenecoordinatesystem)
{
	return cmzn_scenecoordinatesystem_conversion::to_string(enumerator_value);
}

DEFINE_DEFAULT_ENUMERATOR_FUNCTIONS(cmzn_scenecoordinatesystem)

int cmzn_scenecoordinatesystem_get_viewport(
	enum cmzn_scenecoordinatesystem coordinate_system,
	double viewport_width, double viewport_height,
	double *left, double *right, double *bottom, double *top)
{
	if (!((viewport_width > 0.0) && (viewport_height > 0.0) && left && right && bottom && top))
		return 0;
	*left = -1.0;
	*right = 1.0;
	*bottom = -1.0;
	*top = 1.0;
	int return_code = 1;
	switch (coordinate_system)
	{
	case CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FILL:
		// do nothing
		break;
	case CMZN_SCENECOORDINATESYSTEM_WINDOW_PIXEL_BOTTOM_LEFT:
		*left = -0.5;
		*right = (double)(int)(viewport_width - 0.5);
		*bottom = -0.5;
		*top = (double)(int)(viewport_height - 0.5);
		break;
	case CMZN_SCENECOORDINATESYSTEM_WINDOW_PIXEL_TOP_LEFT:
		*left = -0.5;
		*right = (double)(int)(viewport_width - 0.5);
		*bottom = (double)(int)(-viewport_height + 0.5);
		*top = 0.5;
		break;
	default:
		if (viewport_width > viewport_height)
		{
			double width_to_height_ratio = viewport_width/viewport_height;
			switch (coordinate_system)
			{
				case CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FIT_LEFT:
					*right = 2.0*width_to_height_ratio - 1.0;
					break;
				case CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FIT_RIGHT:
					*left = -2.0*width_to_height_ratio + 1.0;
					break;
				case CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FIT_CENTRE:
				case CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FIT_TOP:
				case CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FIT_BOTTOM:
					*left = -width_to_height_ratio;
					*right = width_to_height_ratio;
					break;
				default:
					return_code = 0;
					break;
			}
		}
		else
		{
			double height_to_width_ratio = viewport_height / viewport_width;
			switch (coordinate_system)
			{
				case CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FIT_CENTRE:
				case CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FIT_LEFT:
				case CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FIT_RIGHT:
					*bottom = -height_to_width_ratio;
					*top = height_to_width_ratio;
					break;
				case CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FIT_TOP:
					*bottom = -2.0*height_to_width_ratio + 1.0;
					break;
				case CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FIT_BOTTOM:
					*top = 2.0*height_to_width_ratio - 1.0;
					break;
				default:
					return_code = 0;
					break;
			}
		}
		break;
	}
	if (!return_code)
	{
		display_message(ERROR_MESSAGE,
			"cmzn_scenecoordinatesystem_get_viewport.  Unsupported coordinate system");
	}
	return return_code;
}

/*******************************************************************************
 * Returns true if the coordinate_system is window-relative, which currently
 * determines whether it is drawn as an overlay.
 *
 * @param coordinate_system  The scene coordinate system.
 * @return  1 if window-relative, 0 if not.
 */
int cmzn_scenecoordinatesystem_is_window_relative(
	enum cmzn_scenecoordinatesystem coordinate_system)
{
	return
		(coordinate_system == CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FILL) ||
		(coordinate_system == CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FIT_CENTRE) ||
		(coordinate_system == CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FIT_LEFT) ||
		(coordinate_system == CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FIT_RIGHT) ||
		(coordinate_system == CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FIT_TOP) ||
		(coordinate_system == CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FIT_BOTTOM) ||
		(coordinate_system == CMZN_SCENECOORDINATESYSTEM_WINDOW_PIXEL_BOTTOM_LEFT) ||
		(coordinate_system == CMZN_SCENECOORDINATESYSTEM_WINDOW_PIXEL_TOP_LEFT);
}
