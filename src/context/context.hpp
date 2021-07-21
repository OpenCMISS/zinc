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

#if !defined (CONTEXT_H)
#define CONTEXT_H

#include <list>
#include "opencmiss/zinc/context.h"
#include "opencmiss/zinc/status.h"
#include "general/message_log.hpp"
#include "general/manager.h"

struct cmzn_graphics_module;

struct cmzn_context
{
	friend struct Element_point_ranges_selection *cmzn_context_get_element_point_ranges_selection(
		cmzn_context *context);
	friend struct IO_stream_package *cmzn_context_get_default_IO_stream_package(
		cmzn_context *context);
private:
	const char *id;
	cmzn_logger *logger;
	cmzn_region *defaultRegion;
	struct Element_point_ranges_selection *element_point_ranges_selection;
	//-- struct Event_dispatcher *event_dispatcher;
	struct IO_stream_package *io_stream_package;
	cmzn_timekeepermodule *timekeepermodule;
	std::list<cmzn_region *> allRegions; // list of all regions created for context, not accessed
	cmzn_graphics_module *graphics_module;
	int access_count;

	cmzn_context(const char *idIn);
	~cmzn_context();

public:

	inline cmzn_context *access()
	{
		++access_count;
		return this;
	}

	static inline int deaccess(cmzn_context*& context)
	{
		if (context)
		{
			--(context->access_count);
			if (context->access_count <= 0)
				delete context;
			context = 0;
			return CMZN_OK;
		}
		return CMZN_ERROR_ARGUMENT;
	}

	static cmzn_context *create(const char *id);

	cmzn_region *createRegion();

	void removeRegion(cmzn_region *region);

	cmzn_graphics_module *getGraphicsmodule()
	{
		return this->graphics_module;
	}

	/** Get any region from context from which to copy FE_region information */
	cmzn_region *getBaseRegion() const
	{
		return (this->allRegions.size() > 0) ? this->allRegions.front() : nullptr;
	}

	/** Get default region or nullptr if none */
	cmzn_region *getDefaultRegion() const
	{
		return this->defaultRegion;
	}

	/** Set default region if you wish context to manage it */
    int setDefaultRegion(cmzn_region *regionIn);

	cmzn_logger *getLogger() const
	{
		return this->logger;
	}

	const std::list<cmzn_region *>& getRegionsList() const
	{
		return this->allRegions;
	}

	cmzn_timekeepermodule *getTimekeepermodule() const
	{
		return this->timekeepermodule;
	}
	
};

/***************************************************************************//**
 * Return the element point ranges selection in context.
 *
 * @param context  Pointer to a cmiss_context object.
 * @return  the Element_point_ranges_selection if successfully, otherwise NULL.
 */
struct Element_point_ranges_selection *cmzn_context_get_element_point_ranges_selection(
	cmzn_context *context);

/***************************************************************************//**
 * Return the IO_stream_package in context. Used by Cmgui only.
 *
 * @param context  Pointer to a cmiss_context object.
 * @return  the default IO_stream_package if successfully, otherwise NULL.
 */
struct IO_stream_package *cmzn_context_get_default_IO_stream_package(
	cmzn_context *context);

#endif /* !defined (CONTEXT_H) */
