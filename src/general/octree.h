/*******************************************************************************
FILE : octree.h

LAST MODIFIED : 20 October 2005

DESCRIPTION :
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (OCTREE_H)
#define OCTREE_H

#include "general/object.h"
#include "general/list.h"
#include "general/value.h"

/*
Macros
======
*/

/*
Global types
------------
*/

struct Octree;

struct Octree_object;

DECLARE_LIST_TYPES(Octree_object);

/*
Global functions
----------------
*/

PROTOTYPE_LIST_FUNCTIONS(Octree_object);

struct Octree *CREATE(Octree)(void);
/*******************************************************************************
LAST MODIFIED : 20 October 2005

DESCRIPTION :
Creates an Octree which will contain Octree_objects.
==============================================================================*/

int DESTROY(Octree)(struct Octree **octree_address);
/*******************************************************************************
LAST MODIFIED : 20 October 2005

DESCRIPTION :
Destroys an octree.
==============================================================================*/

int Octree_add_object(struct Octree *octree, struct Octree_object *object);
/*******************************************************************************
LAST MODIFIED : 20 October 2005

DESCRIPTION :
Adds the specified <object> into the <octree>
==============================================================================*/

int Octree_add_objects_near_coordinate_to_list(struct Octree *octree, 
	int dimension, FE_value *coordinate, FE_value radius, 
	struct LIST(Octree_object) *object_list);
/*******************************************************************************
LAST MODIFIED : 20 October 2005

DESCRIPTION :
Examines the <octree> for any objects closer than <radius> in an L2 norm sense
to the specified <coordinate> and adds these to the <object_list>.
==============================================================================*/

struct Octree_object *CREATE(Octree_object)(int dimension, FE_value *coordinates);
/*******************************************************************************
LAST MODIFIED : 20 October 2005

DESCRIPTION :
Creates an Octree_object having the specified <coordinates> in <dimension> space.
==============================================================================*/

int Octree_object_set_user_data(struct Octree_object *object, void *user_data);
/*******************************************************************************
LAST MODIFIED : 20 October 2005

DESCRIPTION :
Sets the <user_data> pointer associated with an octree object.
==============================================================================*/

void *Octree_object_get_user_data(struct Octree_object *object);
/*******************************************************************************
LAST MODIFIED : 20 October 2005

DESCRIPTION :
Gets the <user_data> pointer associated with an octree object.
==============================================================================*/

int DESTROY(Octree_object)(struct Octree_object **octree_address);
/*******************************************************************************
LAST MODIFIED : 20 October 2005

DESCRIPTION :
Destroys an octree.
==============================================================================*/

/***************************************************************************//**
 * @return Octree_object from <object_list> with coordinates nearest to supplied
 * <coordinates>, or NULL if empty.
 */
struct Octree_object *Octree_object_list_get_nearest(
	struct LIST(Octree_object) *object_list, FE_value *coordinates);

#endif /* !defined (OCTREE_H) */
