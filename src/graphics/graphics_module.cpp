/**
 * FILE : graphics_module.cpp
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "opencmiss/zinc/glyph.h"
#include "opencmiss/zinc/material.h"
#include "opencmiss/zinc/scene.h"
#include "opencmiss/zinc/timekeeper.h"
#include "general/debug.h"
#include "general/object.h"
#include "general/mystring.h"
#include "graphics/font.h"
#include "graphics/glyph.hpp"
#include "graphics/graphics_object.h"
#include "graphics/material.h"
#include "graphics/scene.hpp"
#include "graphics/scene_viewer.h"
#include "graphics/shader.hpp"
#include "graphics/shader_program.hpp"
#include "graphics/shader_uniforms.hpp"
#include "graphics/spectrum.h"
#include "graphics/graphics_module.hpp"
#include "graphics/scenefilter.hpp"
#include "graphics/tessellation.hpp"
#include "region/cmiss_region.hpp"
#include "general/message.h"
#include <list>

namespace {

/**
 * Callback for changes in the glyph manager.
 * Informs all scenes about the changes.
 */
void cmzn_graphics_module_glyph_manager_callback(
	struct MANAGER_MESSAGE(cmzn_glyph) *message, void *graphics_module_void)
{
	cmzn_graphics_module *graphics_module = reinterpret_cast<cmzn_graphics_module *>(graphics_module_void);
	if (message && graphics_module)
	{
		int change_summary = MANAGER_MESSAGE_GET_CHANGE_SUMMARY(cmzn_glyph)(message);
		if (change_summary & MANAGER_CHANGE_RESULT(cmzn_glyph))
		{
			const std::list<cmzn_region*>& regionsList = graphics_module->getContext()->getRegionsList();
			std::list<cmzn_region*>::const_iterator regionIter;
			for (regionIter = regionsList.begin(); regionIter != regionsList.end(); ++regionIter)
			{
				cmzn_region *region = *regionIter;
				if (cmzn_region_is_root(region))
				{
					cmzn_scene *scene = region->getScene();
					cmzn_scene_glyph_change(scene, message);
				}
			}
		}
	}
}

/**
 * Callback for changes in the material manager.
 * Informs all scenes about the changes.
 */
void cmzn_graphics_module_material_manager_callback(
	struct MANAGER_MESSAGE(cmzn_material) *message, void *graphics_module_void)
{
	cmzn_graphics_module *graphics_module = reinterpret_cast<cmzn_graphics_module *>(graphics_module_void);
	if (message && graphics_module)
	{
		int change_summary = MANAGER_MESSAGE_GET_CHANGE_SUMMARY(cmzn_material)(message);
		if (change_summary & MANAGER_CHANGE_RESULT(cmzn_material))
		{
			const std::list<cmzn_region*>& regionsList = graphics_module->getContext()->getRegionsList();
			std::list<cmzn_region*>::const_iterator regionIter;
			for (regionIter = regionsList.begin(); regionIter != regionsList.end(); ++regionIter)
			{
				cmzn_region *region = *regionIter;
				if (cmzn_region_is_root(region))
				{
					cmzn_scene *scene = region->getScene();
					cmzn_scene_material_change(scene, message);
				}
			}
		}
	}
}

/**
 * Callback for changes in the spectrum manager.
 * Informs all scenes about the changes.
 */
void cmzn_graphics_module_spectrum_manager_callback(
	struct MANAGER_MESSAGE(cmzn_spectrum) *message, void *graphics_module_void)
{
	cmzn_graphics_module *graphics_module = reinterpret_cast<cmzn_graphics_module *>(graphics_module_void);
	if (message && graphics_module)
	{
		int change_summary = MANAGER_MESSAGE_GET_CHANGE_SUMMARY(cmzn_spectrum)(message);
		if (change_summary & MANAGER_CHANGE_RESULT(cmzn_spectrum))
		{
			// update colour_bar glyphs, if any
			graphics_module->glyphmodule->beginChange();
			cmzn_set_cmzn_glyph *glyphList = graphics_module->glyphmodule->getGlyphListPrivate();
			for (cmzn_set_cmzn_glyph::iterator iter = glyphList->begin(); iter != glyphList->end(); ++iter)
			{
				cmzn_glyph *glyph = *iter;
				glyph->spectrumChange(message);
			}
			graphics_module->glyphmodule->endChange();
			const std::list<cmzn_region*>& regionsList = graphics_module->getContext()->getRegionsList();
			std::list<cmzn_region*>::const_iterator regionIter;
			for (regionIter = regionsList.begin(); regionIter != regionsList.end(); ++regionIter)
			{
				cmzn_region *region = *regionIter;
				if (cmzn_region_is_root(region))
				{
					cmzn_scene *scene = region->getScene();
					cmzn_scene_spectrum_change(scene, message);
				}
			}
		}
	}
}

void cmzn_graphics_module_shaderprogram_manager_callback(
	struct MANAGER_MESSAGE(cmzn_shaderprogram) *message, void *graphics_module_void)
{
	cmzn_graphics_module *graphics_module = reinterpret_cast<cmzn_graphics_module *>(graphics_module_void);
	if (message && graphics_module)
	{
		int change_summary = MANAGER_MESSAGE_GET_CHANGE_SUMMARY(cmzn_shaderprogram)(message);
		if (change_summary & MANAGER_CHANGE_RESULT(cmzn_shaderprogram))
		{
			FOR_EACH_OBJECT_IN_MANAGER(cmzn_material)(
				cmzn_material_shaderprogram_changed, (void *)message,
				cmzn_materialmodule_get_manager(graphics_module->materialmodule));
		}
	}
}

void cmzn_graphics_module_shaderuniforms_manager_callback(
	struct MANAGER_MESSAGE(cmzn_shaderuniforms) *message, void *graphics_module_void)
{
	cmzn_graphics_module *graphics_module = reinterpret_cast<cmzn_graphics_module *>(graphics_module_void);
	if (message && graphics_module)
	{
		int change_summary = MANAGER_MESSAGE_GET_CHANGE_SUMMARY(cmzn_shaderuniforms)(message);
		if (change_summary & MANAGER_CHANGE_RESULT(cmzn_shaderuniforms))
		{
			FOR_EACH_OBJECT_IN_MANAGER(cmzn_material)(
				cmzn_material_shaderuniforms_changed, (void *)message,
				cmzn_materialmodule_get_manager(graphics_module->materialmodule));
		}
	}
}

/**
 * Callback for changes in the tessellation manager.
 * Informs all scenes about the changes.
 */
void cmzn_graphics_module_tessellation_manager_callback(
	struct MANAGER_MESSAGE(cmzn_tessellation) *message, void *graphics_module_void)
{
	cmzn_graphics_module *graphics_module = reinterpret_cast<cmzn_graphics_module *>(graphics_module_void);
	if (message && graphics_module)
	{
		int change_summary = MANAGER_MESSAGE_GET_CHANGE_SUMMARY(cmzn_tessellation)(message);
		if (change_summary & MANAGER_CHANGE_RESULT(cmzn_tessellation))
		{
			const std::list<cmzn_region*>& regionsList = graphics_module->getContext()->getRegionsList();
			std::list<cmzn_region*>::const_iterator regionIter;
			for (regionIter = regionsList.begin(); regionIter != regionsList.end(); ++regionIter)
			{
				cmzn_region *region = *regionIter;
				if (cmzn_region_is_root(region))
				{
					cmzn_scene *scene = region->getScene();
					cmzn_scene_tessellation_change(scene, message);
				}
			}
		}
	}
}

/**
 * Callback for changes in the font manager.
 * Informs all scenes about the changes.
 */
void cmzn_graphics_module_font_manager_callback(
	struct MANAGER_MESSAGE(cmzn_font) *message, void *graphics_module_void)
{
	cmzn_graphics_module *graphics_module = reinterpret_cast<cmzn_graphics_module *>(graphics_module_void);
	if (message && graphics_module)
	{
		int change_summary = MANAGER_MESSAGE_GET_CHANGE_SUMMARY(cmzn_font)(message);
		if (change_summary & MANAGER_CHANGE_RESULT(cmzn_font))
		{
			const std::list<cmzn_region*>& regionsList = graphics_module->getContext()->getRegionsList();
			std::list<cmzn_region*>::const_iterator regionIter;
			for (regionIter = regionsList.begin(); regionIter != regionsList.end(); ++regionIter)
			{
				cmzn_region *region = *regionIter;
				if (cmzn_region_is_root(region))
				{
					cmzn_scene *scene = region->getScene();
					cmzn_scene_font_change(scene, message);
				}
			}
		}
	}
}

}

cmzn_graphics_module::cmzn_graphics_module(cmzn_context *contextIn) :
	context(contextIn),
	fontmodule(cmzn_fontmodule_create()),
	font_manager_callback_id(MANAGER_REGISTER(cmzn_font)(cmzn_graphics_module_font_manager_callback,
		(void *)this, cmzn_fontmodule_get_manager(this->fontmodule))),
	spectrummodule(cmzn_spectrummodule_create()),
	spectrum_manager_callback_id(MANAGER_REGISTER(cmzn_spectrum)(cmzn_graphics_module_spectrum_manager_callback,
	(void *)this, cmzn_spectrummodule_get_manager(this->spectrummodule))),
	materialmodule(cmzn_materialmodule_create(cmzn_spectrummodule_get_manager(this->spectrummodule))),
	material_manager_callback_id(MANAGER_REGISTER(cmzn_material)(cmzn_graphics_module_material_manager_callback,
		(void *)this, cmzn_materialmodule_get_manager(this->materialmodule))),
	glyphmodule(cmzn_glyphmodule_create(this->materialmodule)),
	glyph_manager_callback_id(MANAGER_REGISTER(cmzn_glyph)(cmzn_graphics_module_glyph_manager_callback,
		(void *)this, cmzn_glyphmodule_get_manager(this->glyphmodule))),
	lightmodule(cmzn_lightmodule_create()),
	scenefiltermodule(cmzn_scenefiltermodule_create()),
	sceneviewermodule(nullptr),  // created on demand
	shadermodule(cmzn_shadermodule_create()),
	shaderprogram_manager_callback_id(MANAGER_REGISTER(cmzn_shaderprogram)(cmzn_graphics_module_shaderprogram_manager_callback,
		(void *)this, cmzn_shadermodule_get_program_manager(this->shadermodule))),
	shaderuniforms_manager_callback_id(MANAGER_REGISTER(cmzn_shaderuniforms)(cmzn_graphics_module_shaderuniforms_manager_callback,
		(void *)this, cmzn_shadermodule_get_uniforms_manager(this->shadermodule))),
	tessellationmodule(cmzn_tessellationmodule_create()),
	tessellation_manager_callback_id(MANAGER_REGISTER(cmzn_tessellation)(cmzn_graphics_module_tessellation_manager_callback,
		(void *)this, cmzn_tessellationmodule_get_manager(this->tessellationmodule)))
{
}

cmzn_graphics_module::~cmzn_graphics_module()
{
	MANAGER_DEREGISTER(cmzn_glyph)(
		this->glyph_manager_callback_id,
		cmzn_glyphmodule_get_manager(this->glyphmodule));
	MANAGER_DEREGISTER(cmzn_material)(
		this->material_manager_callback_id,
		cmzn_materialmodule_get_manager(this->materialmodule));
	MANAGER_DEREGISTER(cmzn_shaderprogram)(
		this->shaderprogram_manager_callback_id,
		cmzn_shadermodule_get_program_manager(this->shadermodule));
	MANAGER_DEREGISTER(cmzn_shaderuniforms)(
		this->shaderuniforms_manager_callback_id,
		cmzn_shadermodule_get_uniforms_manager(this->shadermodule));
	MANAGER_DEREGISTER(cmzn_spectrum)(
		this->spectrum_manager_callback_id, cmzn_spectrummodule_get_manager(this->spectrummodule));
	MANAGER_DEREGISTER(cmzn_tessellation)(
		this->tessellation_manager_callback_id,
		cmzn_tessellationmodule_get_manager(this->tessellationmodule));
	MANAGER_DEREGISTER(cmzn_font)(
		this->font_manager_callback_id,
		cmzn_fontmodule_get_manager(this->fontmodule));
	/* This will remove all callbacks used by the scene_viewer projection_field callback */
	cmzn_glyphmodule_destroy(&this->glyphmodule);
	if (this->lightmodule)
		cmzn_lightmodule_destroy(&this->lightmodule);
	if (this->shadermodule)
		cmzn_shadermodule_destroy(&this->shadermodule);
	if (this->spectrummodule)
		cmzn_spectrummodule_destroy(&this->spectrummodule);
	if (this->fontmodule)
		cmzn_fontmodule_destroy(&this->fontmodule);
	if (this->materialmodule)
		cmzn_materialmodule_destroy(&this->materialmodule);
	if (this->scenefiltermodule)
		cmzn_scenefiltermodule_destroy(&this->scenefiltermodule);
	if (this->tessellationmodule)
		cmzn_tessellationmodule_destroy(&this->tessellationmodule);
	if (this->sceneviewermodule)
		cmzn_sceneviewermodule_destroy(&this->sceneviewermodule);
}

cmzn_graphics_module *cmzn_graphics_module::create(cmzn_context *contextIn)
{
	if (!contextIn)
	{
		display_message(ERROR_MESSAGE, "cmzn_graphics_module::create.  Missing context");
		return nullptr;
	}
	cmzn_graphics_module *module = new cmzn_graphics_module(contextIn);
	if ((module) &&
		(module->context) &&
		(module->fontmodule) &&
		(module->font_manager_callback_id) &&
		(module->spectrummodule) &&
		(module->spectrum_manager_callback_id) &&
		(module->materialmodule) &&
		(module->material_manager_callback_id) &&
		(module->glyphmodule) &&
		(module->glyph_manager_callback_id) &&
		(module->lightmodule) &&
		(module->scenefiltermodule) &&
		// created on demand: (module->sceneviewermodule) &&
		(module->shadermodule) &&
		(module->shaderprogram_manager_callback_id) &&
		(module->shaderuniforms_manager_callback_id) &&
		(module->tessellationmodule) &&
		(module->tessellation_manager_callback_id))
	{
		return module;
	}
	display_message(ERROR_MESSAGE, "cmzn_graphics_module::create.  Failed");
	delete module;
	return nullptr;
}

struct cmzn_materialmodule *cmzn_graphics_module_get_materialmodule(
	struct cmzn_graphics_module *graphics_module)
{
	if (graphics_module && graphics_module->materialmodule)
	{
		return cmzn_materialmodule_access(graphics_module->materialmodule);
	}

	return nullptr;
}

struct MANAGER(cmzn_spectrum) *cmzn_graphics_module_get_spectrum_manager(
	struct cmzn_graphics_module *graphics_module)
{
	if (graphics_module && graphics_module->spectrummodule)
	{
		return cmzn_spectrummodule_get_manager(graphics_module->spectrummodule);
	}

	return NULL;
}

cmzn_spectrummodule_id cmzn_graphics_module_get_spectrummodule(
	struct cmzn_graphics_module *graphics_module)
{
	if (graphics_module && graphics_module->spectrummodule)
	{
		return cmzn_spectrummodule_access(graphics_module->spectrummodule);
	}
	return nullptr;
}

struct cmzn_font *cmzn_graphics_module_get_default_font(
	struct cmzn_graphics_module *graphics_module)
{
	if (graphics_module && graphics_module->fontmodule)
	{
		return cmzn_fontmodule_get_default_font(graphics_module->fontmodule);
	}
	return nullptr;
}

cmzn_fontmodule_id cmzn_graphics_module_get_fontmodule(
	struct cmzn_graphics_module *graphics_module)
{
	if (graphics_module && graphics_module->fontmodule)
	{
		return cmzn_fontmodule_access(graphics_module->fontmodule);
	}
	return nullptr;
}

struct MANAGER(cmzn_font) *cmzn_graphics_module_get_font_manager(
	struct cmzn_graphics_module *graphics_module)
{
	if (graphics_module && graphics_module->fontmodule)
	{
		return cmzn_fontmodule_get_manager(graphics_module->fontmodule);
	}
	return nullptr;
}

cmzn_glyphmodule_id cmzn_graphics_module_get_glyphmodule(
	cmzn_graphics_module * graphics_module)
{
	if (graphics_module)
	{
		cmzn_glyphmodule_access(graphics_module->glyphmodule);
		return graphics_module->glyphmodule;
	}
	return nullptr;
}

cmzn_sceneviewermodule_id cmzn_graphics_module_get_sceneviewermodule(
	cmzn_graphics_module * graphics_module)
{
	cmzn_sceneviewermodule *sceneviewermodule = NULL;
	if (graphics_module)
	{
		if (!graphics_module->sceneviewermodule)
		{
			cmzn_light *default_light =
				cmzn_lightmodule_get_default_light(graphics_module->lightmodule);
			cmzn_light *default_ambient_light =
				cmzn_lightmodule_get_default_ambient_light(graphics_module->lightmodule);
			Colour default_background_colour;
			default_background_colour.red = 0.0;
			default_background_colour.green = 0.0;
			default_background_colour.blue = 0.0;
			default_background_colour.alpha = 1.0;
			cmzn_scenefiltermodule_id filterModule = cmzn_graphics_module_get_scenefiltermodule(graphics_module);
			graphics_module->sceneviewermodule = cmzn_sceneviewermodule::create(
				&default_background_colour,
				graphics_module->lightmodule, default_light,
				default_ambient_light,
				filterModule);
			cmzn_scenefiltermodule_destroy(&filterModule);
			cmzn_light_destroy(&default_ambient_light);
			cmzn_light_destroy(&default_light);
		}
		sceneviewermodule = cmzn_sceneviewermodule_access(graphics_module->sceneviewermodule);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_graphics_module_get_sceneviewermodule.  "
			"Missing context");
	}
	return sceneviewermodule;
}

cmzn_shadermodule_id cmzn_graphics_module_get_shadermodule(
	struct cmzn_graphics_module *graphics_module)
{
	if (graphics_module && graphics_module->shadermodule)
	{
		return cmzn_shadermodule_access(graphics_module->shadermodule);
	}
	return nullptr;
}

cmzn_tessellationmodule_id cmzn_graphics_module_get_tessellationmodule(
	struct cmzn_graphics_module *graphics_module)
{
	if (graphics_module && graphics_module->tessellationmodule)
	{
		return cmzn_tessellationmodule_access(graphics_module->tessellationmodule);
	}
	return nullptr;
}

cmzn_timekeepermodule *cmzn_graphics_module_get_timekeepermodule(
	struct cmzn_graphics_module *graphics_module)
{
	if (graphics_module && graphics_module->context)
	{
		return graphics_module->context->getTimekeepermodule()->access();
	}
	return nullptr;
}

cmzn_scenefiltermodule_id cmzn_graphics_module_get_scenefiltermodule(
	struct cmzn_graphics_module *graphics_module)
{
	if (graphics_module && graphics_module->scenefiltermodule)
	{
		return cmzn_scenefiltermodule_access(graphics_module->scenefiltermodule);
	}
	return nullptr;
}

struct MANAGER(cmzn_scenefilter) *cmzn_graphics_module_get_filter_manager(
		struct cmzn_graphics_module *graphics_module)
{
	if (graphics_module && graphics_module->scenefiltermodule)
	{
		return cmzn_scenefiltermodule_get_manager(graphics_module->scenefiltermodule);
	}
	return nullptr;
}

cmzn_lightmodule *cmzn_graphics_module_get_lightmodule(
	struct cmzn_graphics_module *graphics_module)
{
	if (graphics_module && graphics_module->lightmodule)
	{
		return cmzn_lightmodule_access(graphics_module->lightmodule);
	}

	return nullptr;
}
