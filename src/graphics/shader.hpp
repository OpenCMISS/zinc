/***************************************************************************//**
 * shaders.hpp
 *
 * Objects for controlling shaders program used in Zinc.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (SHADERS_HPP)
#define SHADERS_HPP

#include "opencmiss/zinc/shader.h"
#include "general/list.h"
#include "general/manager.h"
#include "general/object.h"

struct cmzn_graphics_module;

/**
 * Create and return a handle to a new shader module.
 * Private; only to be called from graphics_module.
 *
 * @return  Handle to the newly created shader module if successful,
 * otherwise NULL.
 */
cmzn_shadermodule_id cmzn_shadermodule_create();

struct MANAGER(cmzn_shaderuniforms) *cmzn_shadermodule_get_uniforms_manager(
		cmzn_shadermodule_id shadermodule);

struct MANAGER(cmzn_shaderprogram) *cmzn_shadermodule_get_program_manager(
		cmzn_shadermodule_id shadermodule);

cmzn_shaderprogram_id cmzn_shadermodule_find_shaderprogram_by_type(
		cmzn_shadermodule_id shadermodule, int type);

#endif
