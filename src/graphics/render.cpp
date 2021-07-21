/***************************************************************************//**
 * render.cpp
 * Rendering calls - Non API specific.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stdio.h>
#include <math.h>
#include "opencmiss/zinc/scenefilter.h"
#include "general/debug.h"
#include "graphics/auxiliary_graphics_types.h"
#include "graphics/graphics_library.h"
#include "graphics/font.h"
#include "graphics/graphics_object.h"
#include "graphics/mcubes.h"
#include "graphics/spectrum.h"
#include "graphics/tile_graphics_objects.h"
#include "general/message.h"
#include "graphics/scene.hpp"
#include "graphics/graphics_object_private.hpp"
#include "graphics/material.hpp"
#include "graphics/render.hpp"

/****************** Render_graphics_compile_members **********************/

int Render_graphics_compile_members::Scene_compile(cmzn_scene *scene, cmzn_scenefilter *scenefilter)
{
	set_Scene(scene);
	setScenefilter(scenefilter);
	return cmzn_scene_compile_tree(scene, this);
}

int Render_graphics_compile_members::cmzn_scene_compile(cmzn_scene *scene)
{
	return cmzn_scene_compile_scene(scene, this);
}

int Render_graphics_compile_members::cmzn_scene_compile_members(cmzn_scene *scene)
{
	return cmzn_scene_compile_graphics(scene, this, /*force_rebuild*/0);
}
