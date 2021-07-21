/*******************************************************************************
FILE : auxiliary_graphics_types.c

LAST MODIFIED : 3 May 2001

DESCRIPTION :
Structures and enumerated types needed to produce graphics primitives but not
specific to any of them. Examples are:
- struct Element_discretization: stores the number of segments used to
represent curvesin three xi-directions;
- Triple;
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "general/debug.h"
#include "general/random.h"
#include "graphics/auxiliary_graphics_types.h"
#include "general/message.h"
#include "general/mystring.h"
#include "general/enumerator_private.hpp"

/*
Global functions
----------------
*/

PROTOTYPE_ENUMERATOR_STRING_FUNCTION(cmzn_graphics_select_mode)
{
	const char *enumerator_string;

	ENTER(ENUMERATOR_STRING(cmzn_graphics_select_mode));
	switch (enumerator_value)
	{
		case CMZN_GRAPHICS_SELECT_MODE_ON:
		{
			enumerator_string = "select_on";
		} break;
		case CMZN_GRAPHICS_SELECT_MODE_OFF:
		{
			enumerator_string = "no_select";
		} break;
		case CMZN_GRAPHICS_SELECT_MODE_DRAW_SELECTED:
		{
			enumerator_string = "draw_selected";
		} break;
		case CMZN_GRAPHICS_SELECT_MODE_DRAW_UNSELECTED:
		{
			enumerator_string = "draw_unselected";
		} break;
		default:
		{
			enumerator_string = (const char *)NULL;
		} break;
	}
	LEAVE;

	return (enumerator_string);
} /* ENUMERATOR_STRING(cmzn_graphics_select_mode) */

DEFINE_DEFAULT_ENUMERATOR_FUNCTIONS(cmzn_graphics_select_mode)

int check_Element_discretization(struct Element_discretization
	*element_discretization)
/*******************************************************************************
LAST MODIFIED : 11 December 1998

DESCRIPTION :
Ensures the <element_discretization> is within the bounds of the minimum of 1
and the maximum read in from the defaults.
???DB.  Changed the lower bound to 1 because also used for elements.
???DB.  Need to make consistent.
???RC user_interface argument not checked as may not be needed in
read_Element_discretization_defaults().
==============================================================================*/
{
	int discretization_change,return_code;
	struct Element_discretization initial;

	ENTER(check_Element_discretization);
	if (element_discretization)
	{
		discretization_change=0;
		initial.number_in_xi1=element_discretization->number_in_xi1;
		initial.number_in_xi2=element_discretization->number_in_xi2;
		initial.number_in_xi3=element_discretization->number_in_xi3;
		if (1 > element_discretization->number_in_xi1)
		{
			element_discretization->number_in_xi1=1;
			discretization_change=1;
		}
		if (1 > element_discretization->number_in_xi2)
		{
			element_discretization->number_in_xi2=1;
			discretization_change=1;
		}
		if (1 > element_discretization->number_in_xi3)
		{
			element_discretization->number_in_xi3=1;
			discretization_change=1;
		}
		if (discretization_change)
		{
			display_message(WARNING_MESSAGE,
				"Element discretization values must be at least 1\n"
				"%d*%d*%d changed to %d*%d*%d",
				initial.number_in_xi1,initial.number_in_xi2,initial.number_in_xi3,
				element_discretization->number_in_xi1,
				element_discretization->number_in_xi2,
				element_discretization->number_in_xi3);
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"check_Element_discretization.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return return_code;
} /* check_Element_discretization */

PROTOTYPE_ENUMERATOR_STRING_FUNCTION(cmzn_graphicslineattributes_shape_type)
{
	switch (enumerator_value)
	{
		case CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_LINE:
		{
			return "line";
		} break;
		case CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_RIBBON:
		{
			return "ribbon";
		} break;
		case CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_CIRCLE_EXTRUSION:
		{
			return "circle_extrusion";
		} break;
		case CMZN_GRAPHICSLINEATTRIBUTES_SHAPE_TYPE_SQUARE_EXTRUSION:
		{
			return "square_extrusion";
		} break;
		default:
		{
			// do nothing
		} break;
	}
	return 0;
}

DEFINE_DEFAULT_ENUMERATOR_FUNCTIONS(cmzn_graphicslineattributes_shape_type)

PROTOTYPE_ENUMERATOR_STRING_FUNCTION(cmzn_graphics_streamlines_track_direction)
{
	switch (enumerator_value)
	{
		case CMZN_GRAPHICS_STREAMLINES_TRACK_DIRECTION_FORWARD:
		{
			return "forward_track";
		} break;
		case CMZN_GRAPHICS_STREAMLINES_TRACK_DIRECTION_REVERSE:
		{
			return "reverse_track";
		} break;
		default:
		{
			// do nothing
		} break;
	}
	return 0;
}

DEFINE_DEFAULT_ENUMERATOR_FUNCTIONS(cmzn_graphics_streamlines_track_direction)

PROTOTYPE_ENUMERATOR_STRING_FUNCTION(cmzn_graphics_streamlines_colour_data_type)
{
	const char *enumerator_string;

	switch (enumerator_value)
	{
		case CMZN_GRAPHICS_STREAMLINES_COLOUR_DATA_TYPE_FIELD:
		{
			enumerator_string = "field_scalar";
		} break;
		case CMZN_GRAPHICS_STREAMLINES_COLOUR_DATA_TYPE_MAGNITUDE:
		{
			enumerator_string = "magnitude_scalar";
		} break;
		case CMZN_GRAPHICS_STREAMLINES_COLOUR_DATA_TYPE_TRAVEL_TIME:
		{
			enumerator_string = "travel_scalar";
		} break;
		default:
		{
			enumerator_string = (const char *)NULL;
		} break;
	}
	LEAVE;

	return (enumerator_string);
}

DEFINE_DEFAULT_ENUMERATOR_FUNCTIONS(cmzn_graphics_streamlines_colour_data_type)

PROTOTYPE_ENUMERATOR_STRING_FUNCTION(cmzn_graphics_render_polygon_mode)
{
	const char *enumerator_string;

	ENTER(ENUMERATOR_STRING(cmzn_graphics_render_polygon_mode));
	switch (enumerator_value)
	{
		case CMZN_GRAPHICS_RENDER_POLYGON_MODE_SHADED:
		{
			enumerator_string = "render_shaded";
		} break;
		case CMZN_GRAPHICS_RENDER_POLYGON_MODE_WIREFRAME:
		{
			enumerator_string = "render_wireframe";
		} break;
		default:
		{
			enumerator_string = (const char *)NULL;
		} break;
	}
	LEAVE;

	return (enumerator_string);
}

DEFINE_DEFAULT_ENUMERATOR_FUNCTIONS(cmzn_graphics_render_polygon_mode)
