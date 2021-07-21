/*******************************************************************************
FILE : environment_map.h

LAST MODIFIED : 6 December 2004

DESCRIPTION :
The data structures used for representing environment maps.  Used by volume
textures.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (ENVIRONMENT_MAP_H)
#define ENVIRONMENT_MAP_H

#include "general/list.h"
#include "general/manager.h"
#include "general/object.h"
#include "graphics/material.h"

/*
Global types
------------
*/

struct Environment_map
/*******************************************************************************
LAST MODIFIED : 25 July 1995

DESCRIPTION :
The properties of a environment map.
==============================================================================*/
{
	/* the name of the environment map */
	const char *name;
	/* the graphical materials to use for each face of the cube */
	cmzn_material *face_material[6];

	/* after clearing in create, following to be modified only by manager */
	struct MANAGER(Environment_map) *manager;
	int manager_change_status;

	/* the number of structures that point to this environment map. The
		environment map cannot be destroyed while this is greater than 0 */
	int access_count;
}; /* struct Environment_map */

DECLARE_LIST_TYPES(Environment_map);

DECLARE_MANAGER_TYPES(Environment_map);

struct Modify_environment_map_data
/*******************************************************************************
LAST MODIFIED : 22 July 1996

DESCRIPTION :
==============================================================================*/
{
	struct MANAGER(Environment_map) *environment_map_manager;
	struct MANAGER(cmzn_material) *graphical_material_manager;
}; /* struct Modify_environment_map_data */

/*
Global functions
----------------
*/
struct Environment_map *CREATE(Environment_map)(const char *name);
/*******************************************************************************
LAST MODIFIED : 25 September 1995

DESCRIPTION :
Allocates memory and assigns fields for a environment map.  Adds the environment
map to the list of all environment maps.
==============================================================================*/

int DESTROY(Environment_map)(struct Environment_map **environment_map_address);
/*******************************************************************************
LAST MODIFIED : 25 September 1995

DESCRIPTION :
Frees the memory for the environment map and sets <*environment_map_address> to
NULL.
==============================================================================*/

PROTOTYPE_OBJECT_FUNCTIONS(Environment_map);
PROTOTYPE_GET_OBJECT_NAME_FUNCTION(Environment_map);

PROTOTYPE_LIST_FUNCTIONS(Environment_map);

PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(Environment_map,name,const char *);

PROTOTYPE_MANAGER_COPY_FUNCTIONS(Environment_map,name,const char *);

PROTOTYPE_MANAGER_FUNCTIONS(Environment_map);

PROTOTYPE_MANAGER_IDENTIFIER_FUNCTIONS(Environment_map,name,const char *);

int list_Environment_map(struct Environment_map *environment_map);
/*****************************************************************************
LAST MODIFIED : 21 September 1998

DESCRIPTION :
Write the properties of the <environment_map> to the command window.
============================================================================*/

int file_read_Environment_map_name(struct IO_stream *file,
	struct Environment_map **environment_map_address,
	struct MANAGER(Environment_map) *environment_map_manager);
/*******************************************************************************
LAST MODIFIED : 6 December 2004

DESCRIPTION :
Reads a environment map name from a <file>.  Searchs the list of all environment
maps for one with the specified name.  If one is not found a new one is created
with the specified name and the default properties.
==============================================================================*/
#endif
