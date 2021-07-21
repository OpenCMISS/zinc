/*******************************************************************************
FILE : auxiliary_graphics_types.h

LAST MODIFIED : 12 March 2002

DESCRIPTION :
Structures and enumerated types needed to produce graphics primitives but not
specific to any of them. Examples are:
- struct Element_discretization: stores the number of segments used to
represent curves in three xi-directions;
- Triple;
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (AUXILIARY_GRAPHICS_TYPES_H)
#define AUXILIARY_GRAPHICS_TYPES_H

#include "opencmiss/zinc/zincconfigure.h"
#include "opencmiss/zinc/types/graphicsid.h"

#include "general/enumerator.h"
//#if defined (WIN32_SYSTEM)
//#define NOMINMAX
//#include <windows.h>
//#endif /* defined (WIN32_SYSTEM) || defined (CYGWIN) */
#if defined (USE_GLEW)
#include <GL/glew.h>
#endif
/*
Global types
------------
*/

struct Element_discretization
/*******************************************************************************
LAST MODIFIED : 23 November 2001

DESCRIPTION :
Stores the number of segment used in drawing each side of an element.
Controls quality of curve scene.
==============================================================================*/
{
	int number_in_xi1,number_in_xi2,number_in_xi3;
}; /* struct Element_discretization */

typedef GLfloat Triple[3];

enum Graphics_compile_status
/*******************************************************************************
LAST MODIFIED : 12 March 2002

DESCRIPTION :
Graphical objects use this enumeration to flag their compile status, which
can indicate either no compilation is required, child graphics objects need
compiling, or the object and its children need compilation.
Currently applied only to OpenGL display lists, but relevant to other
intermediate storage systems.
==============================================================================*/
{
	GRAPHICS_COMPILED,
	CHILD_GRAPHICS_NOT_COMPILED,
	GRAPHICS_NOT_COMPILED
}; /* enum Graphics_compile_status */

/*
Global functions
----------------
*/

PROTOTYPE_ENUMERATOR_FUNCTIONS(cmzn_graphics_select_mode);

PROTOTYPE_ENUMERATOR_FUNCTIONS(cmzn_graphics_render_polygon_mode);

PROTOTYPE_ENUMERATOR_FUNCTIONS(cmzn_graphicslineattributes_shape_type);

PROTOTYPE_ENUMERATOR_FUNCTIONS(cmzn_graphics_streamlines_track_direction);

PROTOTYPE_ENUMERATOR_FUNCTIONS(cmzn_graphics_streamlines_colour_data_type);

int check_Circle_discretization(int *circle_discretization);

int check_Element_discretization(struct Element_discretization
	*element_discretization);

#endif /* AUXILIARY_GRAPHICS_TYPES_H */
