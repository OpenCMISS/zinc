/*******************************************************************************
FILE : interaction_graphics.h

LAST MODIFIED : 10 July 2000

DESCRIPTION :
Functions for building graphics assisting interaction, eg. rubber-band effect.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (INTERACTION_GRAPHICS_H)
#define INTERACTION_GRAPHICS_H

#include "graphics/graphics_object.h"
#include "interaction/interaction_volume.h"

/*
Global functions
----------------
*/

int Interaction_volume_make_polyline_extents(
	struct Interaction_volume *interaction_volume,
	struct GT_object *graphics_object);
/*******************************************************************************
LAST MODIFIED : 10 July 2000

DESCRIPTION :
Fills <graphics_object> - of type g_POLYLINE with lines marking the box
enclosing the <interaction volume>, used for rubber-banding. Lines are put at
time 0 in the graphics object; any other primitives at that time are cleared.
==============================================================================*/

#endif /* !defined (INTERACTION_GRAPHICS_H) */
