/*******************************************************************************
FILE : cmiss_region_write_info.c

LAST MODIFIED : 16 May 2003

DESCRIPTION :
Data structure shared by several region export modules.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stdio.h>

#include "general/compare.h"
#include "general/debug.h"
#include "general/list.h"
#include "general/indexed_list_private.h"
#include "general/mystring.h"
#include "general/object.h"
#include "region/cmiss_region.hpp"
#include "region/cmiss_region_write_info.h"
#include "general/message.h"

/*
Module types
------------
*/

struct cmzn_region_write_info
/*******************************************************************************
LAST MODIFIED : 16 May 2003

DESCRIPTION :
Data structure shared by several region export modules.
==============================================================================*/
{
	struct cmzn_region *region;
	enum cmzn_region_write_status status;
	char *path;
	int access_count;
};

FULL_DECLARE_INDEXED_LIST_TYPE(cmzn_region_write_info);

/*
Module functions
----------------
*/

struct cmzn_region_write_info *CREATE(cmzn_region_write_info)(void)
/*******************************************************************************
LAST MODIFIED : 16 May 2003

DESCRIPTION :
==============================================================================*/
{
	struct cmzn_region_write_info *write_info;

	ENTER(CREATE(cmzn_region_write_info));
	if (ALLOCATE(write_info, struct cmzn_region_write_info, 1))
	{
		write_info->region = (struct cmzn_region *)NULL;
		write_info->status = CMZN_REGION_NOT_WRITTEN;
		write_info->path = (char *)NULL;
		write_info->access_count = 0;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CREATE(cmzn_region_write_info).  Invalid argument(s)");
	}
	LEAVE;

	return (write_info);
} /* CREATE(cmzn_region_write_info) */

int DESTROY(cmzn_region_write_info)(
	struct cmzn_region_write_info **write_info_address)
/*******************************************************************************
LAST MODIFIED : 16 May 2003

DESCRIPTION :
==============================================================================*/
{
	int return_code;
	struct cmzn_region_write_info *write_info;

	ENTER(DESTROY(cmzn_region_write_info));
	if (write_info_address && (write_info = *write_info_address))
	{
		if (write_info->region)
		{
			DEACCESS(cmzn_region)(&(write_info->region));
		}
		if (write_info->path)
		{
			DEALLOCATE(write_info->path);
		}
		DEALLOCATE(*write_info_address);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(cmzn_region_write_info).  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(cmzn_region_write_info) */

DECLARE_OBJECT_FUNCTIONS(cmzn_region_write_info)

DECLARE_INDEXED_LIST_MODULE_FUNCTIONS(cmzn_region_write_info, region, \
	struct cmzn_region *, compare_pointer)

DECLARE_INDEXED_LIST_FUNCTIONS(cmzn_region_write_info)

DECLARE_FIND_BY_IDENTIFIER_IN_INDEXED_LIST_FUNCTION(cmzn_region_write_info, \
	region, struct cmzn_region *, compare_pointer)

int set_cmzn_region_write_info(
	struct LIST(cmzn_region_write_info) *write_info_list,
	struct cmzn_region *region, enum cmzn_region_write_status write_status,
	char *path)
/*******************************************************************************
LAST MODIFIED : 16 May 2003

DESCRIPTION :
==============================================================================*/
{
	int return_code;
	struct cmzn_region_write_info *write_info;

	ENTER(set_cmzn_region_write_info);
	if (write_info_list && region && path &&
		((CMZN_REGION_DECLARED == write_status) ||
			(CMZN_REGION_WRITTEN == write_status)))
	{
		return_code = 1;
		if (NULL != (write_info = FIND_BY_IDENTIFIER_IN_LIST(cmzn_region_write_info,region)(
			region, write_info_list)))
		{
			/* only need to handle updating from DECLARED to WRITTEN */
			if (CMZN_REGION_DECLARED == write_info->status)
			{
				write_info->status = write_status;
			}
		}
		else
		{
			if (NULL != (write_info = CREATE(cmzn_region_write_info)()))
			{
				write_info->region = ACCESS(cmzn_region)(region);
				write_info->status = write_status;
				write_info->path = duplicate_string(path);
				if (!(write_info->path && ADD_OBJECT_TO_LIST(cmzn_region_write_info)(
					write_info,	write_info_list)))
				{
					DESTROY(cmzn_region_write_info)(&write_info);
					return_code = 0;
				}
			}
			else
			{
				return_code = 0;
			}
			if (!return_code)
			{
				display_message(ERROR_MESSAGE,
					"set_cmzn_region_write_info.  Could not set info");
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"set_cmzn_region_write_info.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* set_cmzn_region_write_info */

int get_cmzn_region_write_info(
	struct LIST(cmzn_region_write_info) *write_info_list,
	struct cmzn_region *region,
	enum cmzn_region_write_status *write_status_address,
	char **path_address)
/*******************************************************************************
LAST MODIFIED : 16 May 2003

DESCRIPTION :
The returned path is not to be deallocated.
==============================================================================*/
{
	int return_code;
	struct cmzn_region_write_info *write_info;

	ENTER(get_cmzn_region_write_info);
	if (write_info_list && region && write_status_address && path_address)
	{
		return_code = 1;
		if (NULL != (write_info = FIND_BY_IDENTIFIER_IN_LIST(cmzn_region_write_info,region)(
			region, write_info_list)))
		{
			*write_status_address = write_info->status;
			*path_address = write_info->path;
		}
		else
		{
			*write_status_address = CMZN_REGION_NOT_WRITTEN;
			*path_address = (char *)NULL;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"get_cmzn_region_write_info.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* get_cmzn_region_write_info */
