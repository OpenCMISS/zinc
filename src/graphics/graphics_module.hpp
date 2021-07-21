/*******************************************************************************
FILE : graphics_module.h

==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (GRAPHICS_MODULE_H)
#define GRAPHICS_MODULE_H

#include "opencmiss/zinc/font.h"
#include "opencmiss/zinc/scenefilter.h"
#include "opencmiss/zinc/shader.h"
#include "general/object.h"
#include "context/context.hpp"
#include "graphics/font.h"
#include "graphics/graphics_object.h"
#include "graphics/material.h"
#include "graphics/spectrum.h"
#include "graphics/light.hpp"
#include "graphics/scene.hpp"
#include "region/cmiss_region.hpp"
#include "selection/element_point_ranges_selection.h"
#include "time/time_keeper.hpp"


/** Members are public only for legacy C functions. Create methods to access in any new code */
struct cmzn_graphics_module
{
	/* attribute managers and defaults: */
	cmzn_context *context;  // owning context, not accessed
	cmzn_fontmodule_id fontmodule;
	void *font_manager_callback_id;
	// need to construct spectrummodule before materialmodule
	cmzn_spectrummodule_id spectrummodule;
	void *spectrum_manager_callback_id;
	// need to construct materialmodule before glyphmodule
	cmzn_materialmodule *materialmodule;
	void *material_manager_callback_id;
	struct cmzn_glyphmodule *glyphmodule;
	void *glyph_manager_callback_id;
	cmzn_lightmodule *lightmodule;
	cmzn_scenefiltermodule *scenefiltermodule;
	cmzn_sceneviewermodule_id sceneviewermodule;
	cmzn_shadermodule_id shadermodule;
	void *shaderprogram_manager_callback_id;
	void *shaderuniforms_manager_callback_id;
	cmzn_tessellationmodule_id tessellationmodule;
	void *tessellation_manager_callback_id;

public:

	cmzn_graphics_module(cmzn_context *contextIn);

public:

	~cmzn_graphics_module();

	static cmzn_graphics_module *create(cmzn_context *contextIn);

	cmzn_context *getContext() const
	{
		return this->context;
	}
};

/***************************************************************************//**
 * Return the light module in graphics module.
 *
 * @param graphics_module  Pointer to a Graphics_module object.
 * @return  the light module in graphics module if successfully called,
 *    otherwise NULL.
 */
struct cmzn_lightmodule *cmzn_graphics_module_get_lightmodule(
	struct cmzn_graphics_module *graphics_module);

/***************************************************************************//**
 * Return the spectrum manager in graphics module.
 *
 * @param graphics_module  Pointer to a Graphics_module object.
 * @return  the spectrum manager in graphics module if successfully called,
 *    otherwise NULL.
 */
struct MANAGER(cmzn_spectrum) *cmzn_graphics_module_get_spectrum_manager(
	struct cmzn_graphics_module *graphics_module);

/***************************************************************************//**
 * Return the default font in graphics module.
 *
 * @param graphics_module  Pointer to a Graphics_module object.
 * @return  the default font if successfully called, otherwise NULL.
 */
struct cmzn_font *cmzn_graphics_module_get_default_font(
	struct cmzn_graphics_module *graphics_module);

/**
 * Return the timekeeper module in graphics module.
 *
 * @param graphics_module  Pointer to a Graphics_module object.
 * @return  the timekeeper module in graphics module if successfully called,
 *    otherwise NULL. Up to caller to destroy.
 */
cmzn_timekeepermodule *cmzn_graphics_module_get_timekeepermodule(
	struct cmzn_graphics_module *graphics_module);

/**
 * Return the manager of scenefilter objects in graphics module.
 *
 * @param graphics_module  Pointer to a Graphics_module object.
 * @return  the manager of scenefilter in graphics module if successfully called,
 *    otherwise NULL.
 */
struct MANAGER(cmzn_scenefilter) *cmzn_graphics_module_get_scenefilter_manager(
		struct cmzn_graphics_module *graphics_module);

struct MANAGER(cmzn_font) *cmzn_graphics_module_get_font_manager(
	struct cmzn_graphics_module *graphics_module);

/**
 * Returns a handle to the scene viewer module.
 *
 * @param graphics_module  The graphics module to request the module from.
 * @return The scene viewer module if successfully called, otherwise NULL.
 */
cmzn_sceneviewermodule_id cmzn_graphics_module_get_sceneviewermodule(
	cmzn_graphics_module * graphics_module);

/**
 * Get the glyph module which stores static graphics for visualising points,
 * vectors, axes etc. Note on startup no glyphs are defined and glyph module
 * functions need to be called to set up standard glyphs.
 *
 * @param graphics_module  The graphics module to request manager from.
 * @return  Handle to the glyph module, or 0 on error. Up to caller to destroy.
 */
cmzn_glyphmodule_id cmzn_graphics_module_get_glyphmodule(
	cmzn_graphics_module * graphics_module);

/**
* Get the shader module which stores shaderuniforms and shaderprogram objects.
*
* @param graphics_module  The graphics module to request module from.
* @return  Handle to the shader module, or 0 on error. Up to caller to destroy.
*/
cmzn_shadermodule_id cmzn_graphics_module_get_shadermodule(
	cmzn_graphics_module * graphics_module);

/**
* Get the spectrum module which stores spectrum objects.
*
* @param graphics_module  The graphics module to request module from.
* @return  Handle to the spectrum module, or 0 on error. Up to caller to destroy.
*/
cmzn_spectrummodule_id cmzn_graphics_module_get_spectrummodule(
	cmzn_graphics_module * graphics_module);

/**
* Get the tessellation module which stores tessellation objects.
*
* @param graphics_module  The graphics module to request module from.
* @return  Handle to the tesselation module, or 0 on error. Up to caller to destroy.
*/
cmzn_tessellationmodule_id cmzn_graphics_module_get_tessellationmodule(
	cmzn_graphics_module * graphics_module);

/**
* Get the scene filter module which stores scenefilter objects.
*
* @param graphics_module  The graphics module to request module from.
* @return  Handle to the scene filter module, or 0 on error. Up to caller to destroy.
*/
cmzn_scenefiltermodule_id cmzn_graphics_module_get_scenefiltermodule(
	cmzn_graphics_module * graphics_module);

/**
* Get the font module which stores font object.
*
* @param graphics_module  The graphics module to request module from.
* @return  Handle to the font module, or 0 on error. Up to caller to destroy.
*/
cmzn_fontmodule_id cmzn_graphics_module_get_fontmodule(
	cmzn_graphics_module * graphics_module);

/***************************************************************************//**
 * Return the material module in graphics module.
 *
 * @param graphics_module  Pointer to a Graphics_module object.
 * @return  the material pacakage in graphics module if successfully called,
 *    otherwise NULL.
 */
cmzn_materialmodule_id cmzn_graphics_module_get_materialmodule(
	struct cmzn_graphics_module *graphics_module);

#endif /* !defined (GRAPHICS_MODULE_H) */
