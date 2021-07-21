/*******************************************************************************
FILE : cmiss_region_write_info.h

LAST MODIFIED : 16 May 2003

DESCRIPTION :
Data structure shared by several region export modules.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (CMZN_REGION_WRITE_INFO_H)
#define CMZN_REGION_WRITE_INFO_H

#include "region/cmiss_region.hpp"
#include "general/list.h"
#include "general/object.h"

/*
Module types
------------
*/

enum cmzn_region_write_status
{
	CMZN_REGION_NOT_WRITTEN,
	CMZN_REGION_DECLARED,
	CMZN_REGION_WRITTEN
};

struct cmzn_region_write_info;
/*******************************************************************************
LAST MODIFIED : 16 May 2003

DESCRIPTION :
Data structure shared by several region export modules.
==============================================================================*/

DECLARE_LIST_TYPES(cmzn_region_write_info);

/*
Module functions
----------------
*/

struct cmzn_region_write_info *CREATE(cmzn_region_write_info)(void);
/*******************************************************************************
LAST MODIFIED : 16 May 2003

DESCRIPTION :
==============================================================================*/

int DESTROY(cmzn_region_write_info)(
	struct cmzn_region_write_info **write_info_address);
/*******************************************************************************
LAST MODIFIED : 16 May 2003

DESCRIPTION :
==============================================================================*/

PROTOTYPE_OBJECT_FUNCTIONS(cmzn_region_write_info);

PROTOTYPE_LIST_FUNCTIONS(cmzn_region_write_info);

PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(cmzn_region_write_info, \
	region, struct cmzn_region *);

int set_cmzn_region_write_info(
	struct LIST(cmzn_region_write_info) *write_info_list,
	struct cmzn_region *region, enum cmzn_region_write_status write_status,
	char *path);
/*******************************************************************************
LAST MODIFIED : 16 May 2003

DESCRIPTION :
==============================================================================*/

int get_cmzn_region_write_info(
	struct LIST(cmzn_region_write_info) *write_info_list,
	struct cmzn_region *region,
	enum cmzn_region_write_status *write_status_address,
	char **path_address);
/*******************************************************************************
LAST MODIFIED : 16 May 2003

DESCRIPTION :
The returned path is not to be deallocated.
==============================================================================*/

#endif /* !defined (CMZN_REGION_WRITE_INFO_H) */
