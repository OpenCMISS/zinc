/*******************************************************************************
 * material.hpp
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (MATERIAL_HPP)
#define MATERIAL_HPP

#include "general/callback_class.hpp"
#include "opencmiss/zinc/material.h"

class Render_graphics_opengl;

struct Material_order_independent_transparency
/*******************************************************************************
LAST MODIFIED : 2 May 2005

DESCRIPTION :
Data for compiling materials specially for order independent transparency.
==============================================================================*/
{
	int layer;
	Render_graphics_opengl *renderer;
}; /* struct Material_order_independent_transparency */

int Material_render_opengl(cmzn_material *material,
	Render_graphics_opengl *renderer);

int Material_compile_members_opengl(cmzn_material *material,
	Render_graphics_opengl *renderer);

int Material_compile_opengl_display_list(cmzn_material *material,
	Callback_base< cmzn_material* > *execute_function,
	Render_graphics_opengl *renderer);

int Material_execute_opengl_display_list(cmzn_material *material,
	Render_graphics_opengl *renderer);

#endif // MATERIAL_HPP
