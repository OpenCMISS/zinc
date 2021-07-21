/***************************************************************************//**
 * context.cpp
 *
 * The main root structure of cmgui.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <cstdlib>
#include "opencmiss/zinc/fieldgroup.h"
#include "configure/version.h"
#include "context/context.hpp"
#include "general/debug.h"
#include "general/mystring.h"
#include "general/object.h"
#include "graphics/scene_viewer.h"
#include "graphics/graphics_module.hpp"
#include "graphics/scene.hpp"
#include "region/cmiss_region.hpp"
#include "opencmiss/zinc/timekeeper.h"

cmzn_context::cmzn_context(const char *idIn) :
	id(duplicate_string(idIn)),
	logger(cmzn_logger::create()),
	defaultRegion(0),
	element_point_ranges_selection(0),
	io_stream_package(0),
	timekeepermodule(cmzn_timekeepermodule::create()),
	graphics_module(cmzn_graphics_module::create(this)),
	access_count(1)
{
}

cmzn_context::~cmzn_context()
{
	if (this->id)
		DEALLOCATE(this->id);
	if (this->defaultRegion)
		cmzn_region::deaccess(this->defaultRegion);
	// clear regions' fields and pointers to this context
	for (std::list<cmzn_region*>::iterator iter = this->allRegions.begin(); iter != this->allRegions.end(); ++iter)
	{
		cmzn_region *region = *iter;
		// detach scenes from all regions before destroying graphics module
		// this also releases fields in use by graphics
		region->detachScene();
		region->detachFields();
		region->clearContext();
	}
	delete this->graphics_module;
	if (this->element_point_ranges_selection)
		DESTROY(Element_point_ranges_selection)(&this->element_point_ranges_selection);
	if (this->io_stream_package)
		DESTROY(IO_stream_package)(&this->io_stream_package);
	cmzn_timekeepermodule::deaccess(this->timekeepermodule);

	/* Write out any memory blocks still ALLOCATED when MEMORY_CHECKING is
		on.  When MEMORY_CHECKING is off this function does nothing */
	list_memory(/*count_number*/0, /*show_pointers*/0, /*increment_counter*/0,
		/*show_structures*/1);
}

cmzn_context *cmzn_context::create(const char *id)
{
	cmzn_context *context = new cmzn_context(id);
	if ((context) && (context->id) && (context->logger) && (context->timekeepermodule) && (context->graphics_module))
		return context;
	display_message(ERROR_MESSAGE, "Zinc.  Failed to create Context");
	delete context;
	return 0;
}

cmzn_region *cmzn_context::createRegion()
{
	// all regions within context share element shapes and bases
	cmzn_region *region = cmzn_region::create(this);
	if (region)
		this->allRegions.push_back(region);
	return region;
}

void cmzn_context::removeRegion(cmzn_region *region)
{
	std::list<cmzn_region*>::iterator iter = std::find(this->allRegions.begin(), this->allRegions.end(), region);
	if (iter != this->allRegions.end())
	{
		region->clearContext();  // not really needed as removeRegion only called by region destructor.
		this->allRegions.erase(iter);
	}
}

int cmzn_context::setDefaultRegion(cmzn_region *regionIn)
{
	if (regionIn && (regionIn->getContext() != this))
	{
		display_message(ERROR_MESSAGE, "Zinc Context setDefaultRegion():  Region is from a different context");
		return CMZN_ERROR_ARGUMENT_CONTEXT;
	}
	cmzn_region::reaccess(this->defaultRegion, regionIn);
	return CMZN_OK;
}

cmzn_context *cmzn_context_create(const char *id)
{
	return cmzn_context::create(id);
}

cmzn_context *cmzn_context_access(cmzn_context *context)
{
	if (context)
		return context->access();
	return 0;
}

int cmzn_context_destroy(cmzn_context **context_address)
{
	if (context_address)
		return cmzn_context::deaccess(*context_address);
	return CMZN_ERROR_ARGUMENT;
}

struct cmzn_region *cmzn_context_get_default_region(cmzn_context *context)
{
	if (!context)
	{
		display_message(ERROR_MESSAGE, "Zinc Context getDefaultRegion():  Missing context");
		return nullptr;
	}
	cmzn_region *defaultRegion = context->getDefaultRegion();
	if (defaultRegion)
	{
		defaultRegion->access();
	}
	else
	{
		defaultRegion = context->createRegion();
		context->setDefaultRegion(defaultRegion);
	}
	return defaultRegion;
}

int cmzn_context_set_default_region(cmzn_context_id context,
	cmzn_region_id region)
{
	if (context)
		return context->setDefaultRegion(region);
	display_message(ERROR_MESSAGE, "Zinc Context setDefaultRegion():  Missing context");
	return CMZN_ERROR_ARGUMENT;
}

struct cmzn_region *cmzn_context_create_region(cmzn_context *context)
{
	if (context)
		return context->createRegion();
	display_message(ERROR_MESSAGE, "Zinc Context createRegion():  Missing context");
	return 0;
}

struct Element_point_ranges_selection *cmzn_context_get_element_point_ranges_selection(
	cmzn_context *context)
{
	struct Element_point_ranges_selection *element_point_ranges_selection = NULL;
	if (context)
	{
		if (!context->element_point_ranges_selection)
		{
			context->element_point_ranges_selection = CREATE(Element_point_ranges_selection)();
		}
		element_point_ranges_selection = context->element_point_ranges_selection;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_context_get_element_point_ranges_selection.  Missing context.");
	}
	return element_point_ranges_selection;
}

struct IO_stream_package *cmzn_context_get_default_IO_stream_package(
	cmzn_context *context)
{
	struct IO_stream_package *io_stream_package = NULL;
	if (context)
	{
		if (!context->io_stream_package)
		{
			context->io_stream_package = CREATE(IO_stream_package)();
		}
		io_stream_package = context->io_stream_package;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_context_get_default_IO_stream_package.  Missing context.");
	}

	return io_stream_package;
}

cmzn_timekeepermodule_id cmzn_context_get_timekeepermodule(cmzn_context_id context)
{
	if (context)
		return context->getTimekeepermodule()->access();
	return 0;
}

cmzn_sceneviewermodule_id cmzn_context_get_sceneviewermodule(
	cmzn_context_id context)
{
	if (context)
		return cmzn_graphics_module_get_sceneviewermodule(context->getGraphicsmodule());
	return nullptr;
}

cmzn_lightmodule_id cmzn_context_get_lightmodule(
	cmzn_context_id context)
{
	if (context)
		return cmzn_graphics_module_get_lightmodule(context->getGraphicsmodule());
	return nullptr;
}

cmzn_materialmodule_id cmzn_context_get_materialmodule(
	cmzn_context_id context)
{
	if (context)
		return cmzn_graphics_module_get_materialmodule(context->getGraphicsmodule());
	return nullptr;
}

cmzn_scenefiltermodule_id cmzn_context_get_scenefiltermodule(
	cmzn_context_id context)
{
	if (context)
		return cmzn_graphics_module_get_scenefiltermodule(context->getGraphicsmodule());
	return nullptr;
}

cmzn_fontmodule_id cmzn_context_get_fontmodule(
	cmzn_context_id context)
{
	if (context)
		return cmzn_graphics_module_get_fontmodule(context->getGraphicsmodule());
	return nullptr;
}

cmzn_tessellationmodule_id cmzn_context_get_tessellationmodule(
	cmzn_context_id context)
{
	if (context)
		return cmzn_graphics_module_get_tessellationmodule(context->getGraphicsmodule());
	return nullptr;
}

cmzn_shadermodule_id cmzn_context_get_shadermodule(
	cmzn_context_id context)
{
	if (context)
		return cmzn_graphics_module_get_shadermodule(context->getGraphicsmodule());
	return nullptr;
}

cmzn_spectrummodule_id cmzn_context_get_spectrummodule(
	cmzn_context_id context)
{
	if (context)
		return cmzn_graphics_module_get_spectrummodule(context->getGraphicsmodule());
	return nullptr;
}

cmzn_glyphmodule_id cmzn_context_get_glyphmodule(
	cmzn_context_id context)
{
	if (context)
		return cmzn_graphics_module_get_glyphmodule(context->getGraphicsmodule());
	return nullptr;
}

cmzn_logger_id cmzn_context_get_logger(cmzn_context_id context)
{
	if (context)
	{
		return cmzn_logger_access(context->getLogger());
	}
	return nullptr;
}
