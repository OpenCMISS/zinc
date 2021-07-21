/**
 * FILE: scene_coordinate_system.hpp
 */ 
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef SCENE_COORDINATE_SYSTEM_HPP
#define SCENE_COORDINATE_SYSTEM_HPP

#include "opencmiss/zinc/types/scenecoordinatesystem.h"
#include "general/enumerator.h"

PROTOTYPE_ENUMERATOR_FUNCTIONS(cmzn_scenecoordinatesystem);

/*******************************************************************************
 * Returns the orthographic viewport bounds for 'device' scene coordinate
 * systems.
 *
 * @param coordinate_system  The scene coordinate system. World, local and
 * other 'non-device' coordinate systems return error.
 * @param viewport_width  Viewport width in pixels.
 * @param viewport_height  Viewport height in pixels.
 * @param left  Address to return coordinate of left of left-most pixel.
 * @param right  Address to return coordinate of right of right-most pixel.
 * @param bottom  Address to return coordinate of bottom of bottom-most pixel.
 * @param top  Address to return coordinate of top of top-most pixel.
 * @return  1 on success, 0 on failure.
 */
int cmzn_scenecoordinatesystem_get_viewport(
	enum cmzn_scenecoordinatesystem coordinate_system,
	double viewport_width, double viewport_height,
	double *left, double *right, double *bottom, double *top);

/*******************************************************************************
 * Returns true if the coordinate_system is window-relative, which currently
 * determines whether it is drawn as an overlay.
 *
 * @param coordinate_system  The scene coordinate system.
 * @return  1 if window-relative, 0 if not.
 */
int cmzn_scenecoordinatesystem_is_window_relative(
	enum cmzn_scenecoordinatesystem coordinate_system);

#endif /* SCENE_COORDINATE_SYSTEM_HPP */
