/*******************************************************************************
FILE : octree.c

LAST MODIFIED : 8 December 2005

DESCRIPTION :
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <math.h>

#include "general/debug.h"
#include "general/octree.h"
#include "general/list_private.h"
#include "general/message.h"

/*
Module types
------------
*/

#define OCTREE_DIMENSION (3)
#define OCTREE_2_POWER_DIMENSION (8)
#define OCTREE_THRESHOLD (20)
static int octree_binary_flags[OCTREE_DIMENSION] = {1, 2, 4};

struct Octree
{
	struct Octree_branch *tree;

#if defined (NEW_CODE)
	/* Maybe keep a cache of the last visited octree object so
		we can start looking there next time */
	struct Octree_object *last_visited;
#endif /* defined (NEW_CODE) */
};

struct Octree_branch;

struct Octree_branch
{
	FE_value *coordinate_minimums;
	FE_value *coordinate_maximums;

	/* The branch contains either a number_of_branches = 2^dimension array 
		of further branches or it contains an object list */
	int number_of_branches;
	struct Octree_branch **branch_array;

	/* This is the list of actual coordinates.  This should be NULL if
		there is a branch list */
	struct LIST(Octree_object) *object_list;

	int access_count;
};

struct Octree_object
{
	FE_value *coordinates;
	void *user_data;

	int access_count;
};

FULL_DECLARE_LIST_TYPE(Octree_object);

/*
Module functions
----------------
*/

static struct Octree_branch *CREATE(Octree_branch)(void)
/*******************************************************************************
LAST MODIFIED : 20 October 2005

DESCRIPTION :
Creates an Octree which will contain Octree_objects.
==============================================================================*/
{
	struct Octree_branch *octree_branch;

	ENTER(CREATE(Octree_branch));
	if (ALLOCATE(octree_branch, struct Octree_branch, 1) &&
		ALLOCATE(octree_branch->coordinate_minimums, FE_value, OCTREE_DIMENSION)  &&
		ALLOCATE(octree_branch->coordinate_maximums, FE_value, OCTREE_DIMENSION))
	{
		octree_branch->branch_array = (struct Octree_branch **)NULL;
		octree_branch->object_list = CREATE(LIST(Octree_object))();
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CREATE(Octree_branch).  Unable to allocate arrays");
		octree_branch = (struct Octree_branch *)NULL;
	}
	LEAVE;

	return (octree_branch);
} /* CREATE(Octree_branch) */

static int DESTROY(Octree_branch)(struct Octree_branch **octree_branch_address)
/*******************************************************************************
LAST MODIFIED : 20 October 2005

DESCRIPTION :
Destroys an octree.
==============================================================================*/
{
	int i, return_code;
	struct Octree_branch *octree_branch;

	ENTER(DESTROY(Octree));
	if (octree_branch_address && (octree_branch = *octree_branch_address))
	{
		DEALLOCATE(octree_branch->coordinate_minimums);
		DEALLOCATE(octree_branch->coordinate_maximums);
		if (octree_branch->branch_array)
		{			
			for (i = 0 ; i < octree_branch->number_of_branches ; i++)
			{
				DESTROY(Octree_branch)(octree_branch->branch_array + i);
			}
			DEALLOCATE(octree_branch->branch_array);
		}
		if (octree_branch->object_list)
		{			
			DESTROY(LIST(Octree_object))(&octree_branch->object_list);
		}
		DEALLOCATE(*octree_branch_address);
		return_code=1;
	}
	else
	{
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Octree_branch) */

static int Octree_branch_split(struct Octree_branch *branch)
/*******************************************************************************
LAST MODIFIED : 21 October 2005

DESCRIPTION :
Splits an octree branch so that it no longer contains an object list but
instead an array of new branches with the appropriate objects in them.
==============================================================================*/
{
	FE_value coordinate_means[OCTREE_DIMENSION];
	int i, j, return_code;
	struct Octree_branch *new_branch;
	struct Octree_object *object;

	ENTER(Octree_branch_split);
	if (branch)
	{
		if ((!branch->branch_array) && branch->object_list)
		{
			branch->number_of_branches = OCTREE_2_POWER_DIMENSION;
			if (ALLOCATE(branch->branch_array, struct Octree_branch *,
					OCTREE_2_POWER_DIMENSION))
			{
				for (j = 0 ; j < OCTREE_DIMENSION ; j++)
				{
					coordinate_means[j] = (branch->coordinate_minimums[j] + 
						branch->coordinate_maximums[j]) / 2.0;
				}
				for (i = 0 ; i < OCTREE_2_POWER_DIMENSION ; i++)
				{
					new_branch = CREATE(Octree_branch)();
					for (j = 0 ; j < OCTREE_DIMENSION ; j++)
					{
						if (i & octree_binary_flags[j])
						{
							new_branch->coordinate_minimums[j] = 
								coordinate_means[j];
							new_branch->coordinate_maximums[j] = 
								branch->coordinate_maximums[j];
						}
						else
						{
							new_branch->coordinate_minimums[j] = 
								branch->coordinate_minimums[j];
							new_branch->coordinate_maximums[j] = 
								coordinate_means[j];
						}
					}
					branch->branch_array[i] = new_branch;
				}
				while (NULL != (object = FIRST_OBJECT_IN_LIST_THAT(Octree_object)(
					(LIST_CONDITIONAL_FUNCTION(Octree_object) *)NULL, (void *)NULL,
					branch->object_list)))
				{
					i = 0;
					for (j = 0 ; j < OCTREE_DIMENSION ; j++)
					{
						if (object->coordinates[j] > coordinate_means[j])
						{
							i += octree_binary_flags[j];
						}
					}
					ADD_OBJECT_TO_LIST(Octree_object)(object, branch->branch_array[i]->object_list);
					REMOVE_OBJECT_FROM_LIST(Octree_object)(object, branch->object_list);
				}
				DESTROY_LIST(Octree_object)(&branch->object_list);
			}
			else
			{
				display_message(ERROR_MESSAGE,"Octree_branch_split.  "
					"Unable to allocate branch array.");
				return_code=0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Octree_branch_split.  "
			"Invalid arguments");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Octree_branch_split */

static int Octree_branch_expand(struct Octree_branch **trunk,
	struct Octree_object *object)
/*******************************************************************************
LAST MODIFIED : 21 October 2005

DESCRIPTION :
Expands the range of an octree specified by <trunk> so that it includes the 
coordinates of the specified <object>.  If the trunk contains no branches
then this is trivial.  Otherwise the octree must expand itself by 
doubling and adding more and more branches.
==============================================================================*/
{
	int contained, existing_tree_index, i, j, return_code;
	struct Octree_branch *new_branch, *new_trunk;

	ENTER(Octree_branch_expand);

	if (trunk && *trunk && object)
	{
		return_code = 1;
		if ((*trunk)->branch_array)
		{
			/* Need to add branches doubling the existing one until we enclose the 
				missing value */
			contained = 0;
			while (!contained)
			{
				new_trunk = CREATE(Octree_branch)();
				DESTROY(LIST(Octree_object))(&new_trunk->object_list);
				new_trunk->number_of_branches = OCTREE_2_POWER_DIMENSION;
				if (ALLOCATE(new_trunk->branch_array, struct Octree_branch *,
						OCTREE_2_POWER_DIMENSION))
				{
					existing_tree_index = 0;
					for (j = 0 ; j < OCTREE_DIMENSION ; j++)
					{
						/* Work out where in the array the existing tree lies,
							test against the means of these boundaries so that 
							if we expand multiple times about a coordinate dimension that
							is contained it will remain relatively near the centre. */
						if (object->coordinates[j] < (((*trunk)->coordinate_minimums[j] + 
							(*trunk)->coordinate_maximums[j]) / 2.0))
						{
							existing_tree_index |= octree_binary_flags[j];
							new_trunk->coordinate_minimums[j] = 
								2 * (*trunk)->coordinate_minimums[j]
								- (*trunk)->coordinate_maximums[j];
							new_trunk->coordinate_maximums[j] = 
								(*trunk)->coordinate_maximums[j];
						}
						else
						{
							new_trunk->coordinate_minimums[j] = 
								(*trunk)->coordinate_minimums[j];
							new_trunk->coordinate_maximums[j] = 
								2 * (*trunk)->coordinate_maximums[j]
								- (*trunk)->coordinate_minimums[j];
						}
					}
					for (i = 0 ; i < OCTREE_2_POWER_DIMENSION ; i++)
					{
						if (existing_tree_index == i)
						{
							new_trunk->branch_array[i] = *trunk;
						}
						else
						{
							new_branch = CREATE(Octree_branch)();
							for (j = 0 ; j < OCTREE_DIMENSION ; j++)
							{
								if (existing_tree_index & octree_binary_flags[j])
								{
									if (i & octree_binary_flags[j])
									{
										new_branch->coordinate_minimums[j] = 
											(*trunk)->coordinate_minimums[j];
										new_branch->coordinate_maximums[j] = 
											(*trunk)->coordinate_maximums[j];
									}
									else
									{
										new_branch->coordinate_minimums[j] = 
										  new_trunk->coordinate_minimums[j];
										new_branch->coordinate_maximums[j] = 
											(*trunk)->coordinate_minimums[j];
									}
								}
								else
								{
									if (i & octree_binary_flags[j])
									{
										new_branch->coordinate_minimums[j] = 
											(*trunk)->coordinate_maximums[j];
										new_branch->coordinate_maximums[j] = 
											new_trunk->coordinate_maximums[j];
									}
									else
									{
										new_branch->coordinate_minimums[j] = 
											(*trunk)->coordinate_minimums[j];
										new_branch->coordinate_maximums[j] = 
											(*trunk)->coordinate_maximums[j];
									}
								}
							}
							new_trunk->branch_array[i] = new_branch;
						}
					}
				}
				*trunk = new_trunk;

				contained = 1;
				i = 0;
				while (contained && (i < OCTREE_DIMENSION))
				{
					if (object->coordinates[i] > (*trunk)->coordinate_maximums[i])
					{
						contained = 0;
					}
					if (object->coordinates[i] < (*trunk)->coordinate_minimums[i])
					{
						contained = 0;
					}
					i++;
				}
			}
		}
		else
		{
			/* Only a single branch so this is easy, just expand it */
			for (i = 0 ; i < OCTREE_DIMENSION ; i++)
			{
				if (object->coordinates[i] > (*trunk)->coordinate_maximums[i])
				{
					(*trunk)->coordinate_maximums[i] = object->coordinates[i];
				}
				if (object->coordinates[i] < (*trunk)->coordinate_minimums[i])
				{
					(*trunk)->coordinate_minimums[i] = object->coordinates[i];
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Octree_branch_expand.  "
			"Invalid arguments");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Octree_branch_expand */

static int Octree_branch_add_object(struct Octree_branch *branch,
	struct Octree_object *object)
/*******************************************************************************
LAST MODIFIED : 21 October 2005

DESCRIPTION :
Adds the specified <object> into the <octree>
==============================================================================*/
{
	int i, j, return_code = 0;

	ENTER(Octree_branch_add_object);
	if (branch && object)
	{
		if (branch->branch_array)
		{
			/* This branch has branches */
			i = 0;
			for (j = 0 ; j < OCTREE_DIMENSION ; j++)
			{
				/* The middle points are stored in the maximums for the first subbranch */
				if (object->coordinates[j] > branch->branch_array[0]->coordinate_maximums[j])
				{
					i += octree_binary_flags[j];
				}
			}
			Octree_branch_add_object(branch->branch_array[i], object);
		}
		else
		{
			ADD_OBJECT_TO_LIST(Octree_object)(object, branch->object_list);
			if (NUMBER_IN_LIST(Octree_object)(branch->object_list) > OCTREE_THRESHOLD)
			{
				Octree_branch_split(branch);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Octree_branch_add_object.  "
			"Invalid arguments");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Octree_branch_add_object */

struct Octree_find_data
{
	FE_value *coordinates;
	FE_value radius;
	FE_value radius_squared;
	struct LIST(Octree_object) *object_list;
};

static int Octree_object_add_objects_near_coordinate_to_list(
	struct Octree_object *object, void *data_void)
/*******************************************************************************
LAST MODIFIED : 26 October 2005

DESCRIPTION :
==============================================================================*/
{
	double distance;
	int i, return_code;
	struct Octree_find_data *data;

	ENTER(Octree_object_add_objects_near_coordinate_to_list);
	if (object && (data = (struct Octree_find_data *)data_void))
	{
		distance = 0.0;
		for (i = 0 ; i < OCTREE_DIMENSION ; i++)
		{
			distance += (object->coordinates[i] - data->coordinates[i])
				* (object->coordinates[i] - data->coordinates[i]);
		}
		if (distance <= data->radius_squared)
		{
			ADD_OBJECT_TO_LIST(Octree_object)(object, data->object_list);
		}
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"Octree_object_add_objects_near_coordinate_to_list.  "
			"Invalid arguments");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Octree_object_add_objects_near_coordinate_to_list */

static int Octree_branch_add_objects_near_coordinate_to_list(
	struct Octree_branch *branch, struct Octree_find_data *data)
/*******************************************************************************
LAST MODIFIED : 26 October 2005

DESCRIPTION :
==============================================================================*/
{
	int call_branch, i, j, return_code;

	ENTER(Octree_branch_add_objects_near_coordinate_to_list);
	if (branch && data)
	{
		return_code = 1;
		if (branch->branch_array)
		{
			for (i = 0 ; i < OCTREE_2_POWER_DIMENSION ; i++)
			{
				call_branch = 1;
				for (j = 0 ; call_branch && (j < OCTREE_DIMENSION) ; j++)
				{
					if (branch->branch_array[i]->coordinate_minimums[j] > 
						data->coordinates[j] + data->radius)
					{
						call_branch = 0;
					}
					if (branch->branch_array[i]->coordinate_maximums[j] < 
						data->coordinates[j] - data->radius)
					{
						call_branch = 0;
					}
				}
				if (call_branch)
				{
					return_code = Octree_branch_add_objects_near_coordinate_to_list(
						branch->branch_array[i], data);
				}
			}
		}
		else
		{
			return_code = FOR_EACH_OBJECT_IN_LIST(Octree_object)(
				Octree_object_add_objects_near_coordinate_to_list,
				(void *)data, branch->object_list);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Octree_branch_add_objects_near_coordinate_to_list.  "
			"Invalid arguments");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Octree_branch_add_objects_near_coordinate_to_list */

/*
Global functions
----------------
*/

DECLARE_OBJECT_FUNCTIONS(Octree_object)
DECLARE_LIST_FUNCTIONS(Octree_object)

struct Octree *CREATE(Octree)(void)
/*******************************************************************************
LAST MODIFIED : 20 October 2005

DESCRIPTION :
Creates an Octree which will contain Octree_objects.
==============================================================================*/
{
	struct Octree *octree;

	ENTER(CREATE(Octree));
	if (ALLOCATE(octree, struct Octree, 1))
	{
		octree->tree = (struct Octree_branch *)NULL;
#if defined (NEW_CODE)
		/* Maybe keep a cache of the last visited octree object so
			we can start looking there next time */
		octree->last_visited = (struct Octree_branch *)NULL;
#endif /* defined (NEW_CODE) */
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CREATE(Octree).  Unable to allocate arrays");
		octree = (struct Octree *)NULL;
	}
	LEAVE;

	return (octree);
} /* CREATE(Octree) */

int DESTROY(Octree)(struct Octree **octree_address)
/*******************************************************************************
LAST MODIFIED : 20 October 2005

DESCRIPTION :
Destroys an octree.
==============================================================================*/
{
	int return_code;
	struct Octree *octree;

	ENTER(DESTROY(Octree));
	if (octree_address && (octree = *octree_address))
	{
		if (octree->tree)
		{
			DESTROY(Octree_branch)(&octree->tree);
		}
		DEALLOCATE(*octree_address);
		return_code=1;
	}
	else
	{
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Octree) */

int Octree_add_object(struct Octree *octree, struct Octree_object *object)
/*******************************************************************************
LAST MODIFIED : 21 October 2005

DESCRIPTION :
Adds the specified <object> into the <octree>
==============================================================================*/
{
	FE_value delta, maximum_width, sizes[OCTREE_DIMENSION];
	int contained, i, return_code;

	ENTER(Octree_add_object);
	if (octree && object)
	{
		return_code = 1;
		if (!octree->tree)
		{
			octree->tree = CREATE(Octree_branch)();
			for (i = 0 ; i < OCTREE_DIMENSION ; i++)
			{
				octree->tree->coordinate_minimums[i] = object->coordinates[i];
				octree->tree->coordinate_maximums[i] = object->coordinates[i];
			}
		}
		contained = 1;
		i = 0;
		while (contained && (i < OCTREE_DIMENSION))
		{
			if (object->coordinates[i] > octree->tree->coordinate_maximums[i])
			{
				contained = 0;
			}
			if (object->coordinates[i] < octree->tree->coordinate_minimums[i])
			{
				contained = 0;
			}
			i++;
		}
		if (!contained)
		{
			return_code = Octree_branch_expand(&octree->tree, object);
		}
		if (octree->tree->object_list && 
			(NUMBER_IN_LIST(Octree_object)(octree->tree->object_list) == OCTREE_THRESHOLD))
		{
			/* When it is full regularise the shape of this first branch before we split
				so that we don't have any very thin or zero widths */
			sizes[0] = octree->tree->coordinate_maximums[0] - octree->tree->coordinate_maximums[0];
			maximum_width = sizes[0];
			for (i = 1 ; i < OCTREE_DIMENSION ; i++)
			{
				sizes[i] = octree->tree->coordinate_maximums[i] - octree->tree->coordinate_maximums[i];
				if (sizes[i] > maximum_width)
				{
					maximum_width = sizes[i];
				}
			}
			if (maximum_width < 1e-5)
			{
				maximum_width = 1.0;
			}
			for (i = 0 ; i < OCTREE_DIMENSION ; i++)
			{
				delta = (maximum_width - sizes[i]) / 2.0;
				octree->tree->coordinate_minimums[i] -= delta;
				octree->tree->coordinate_maximums[i] += delta;
			}
		}
		if (return_code)
		{
			return_code = Octree_branch_add_object(octree->tree, object);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Octree_add_object.  "
			"Invalid arguments");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Octree_add_object */

int Octree_add_objects_near_coordinate_to_list(struct Octree *octree, 
	int dimension, FE_value *coordinates, FE_value radius, 
	struct LIST(Octree_object) *object_list)
/*******************************************************************************
LAST MODIFIED : 20 October 2005

DESCRIPTION :
Examines the <octree> for any objects closer than <radius> in an L2 norm sense
to the specified <coordinate> and adds these to the <object_list>.
==============================================================================*/
{
	int return_code;
	struct Octree_find_data data;

	ENTER(Octree_add_object);
	if (octree && coordinates && (dimension == OCTREE_DIMENSION) && object_list)
	{
		if (octree->tree)
		{
			data.coordinates = coordinates;
			data.radius = radius;
			data.radius_squared = radius * radius;
			data.object_list = object_list;
			Octree_branch_add_objects_near_coordinate_to_list(octree->tree,
				&data);
		}
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"Octree_add_objects_near_coordinate_to_list.  "
			"Invalid arguments");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Octree_add_objects_near_coordinate_to_list */

struct Octree_object *CREATE(Octree_object)(int dimension, FE_value *coordinates)
/*******************************************************************************
LAST MODIFIED : 21 October 2005

DESCRIPTION :
Creates an Octree_object having the specified <coordinates>.
==============================================================================*/
{
	int i;
	struct Octree_object *object;

	ENTER(CREATE(Octree_object));
	if (coordinates && (dimension == OCTREE_DIMENSION))
	{
		if (ALLOCATE(object, struct Octree_object, 1)
			&& ALLOCATE(object->coordinates, FE_value, OCTREE_DIMENSION))
		{
			object->user_data = NULL;
			object->access_count = 0;
			for (i = 0 ; i < OCTREE_DIMENSION ; i++)
			{
				object->coordinates[i] = coordinates[i];
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"CREATE(Octree_object).  Unable to allocate arrays");
			object = (struct Octree_object *)NULL;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CREATE(Octree_object).  Invalid coordinates or dimension.");
		object = (struct Octree_object *)NULL;
	}
	LEAVE;

	return (object);
} /* CREATE(Octree_object) */

int Octree_object_set_user_data(struct Octree_object *object, void *user_data)
/*******************************************************************************
LAST MODIFIED : 21 October 2005

DESCRIPTION :
Sets the <user_data> pointer associated with an octree object.
==============================================================================*/
{
	int return_code;

	ENTER(Octree_object_set_user_data);
	if (object)
	{
		object->user_data = user_data;
		return_code=1;
	}
	else
	{
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Octree_object_set_user_data */

void *Octree_object_get_user_data(struct Octree_object *object)
/*******************************************************************************
LAST MODIFIED : 21 October 2005

DESCRIPTION :
Gets the <user_data> pointer associated with an octree object.
==============================================================================*/
{
	void *user_data;

	ENTER(Octree_object_get_user_data);
	if (object)
	{
		user_data = object->user_data;
	}
	else
	{
		user_data = NULL;
	}
	LEAVE;

	return (user_data);
} /* Octree_object_get_user_data */

int DESTROY(Octree_object)(struct Octree_object **object_address)
/*******************************************************************************
LAST MODIFIED : 21 October 2005

DESCRIPTION :
Destroys an octree.
==============================================================================*/
{
	int return_code;
	struct Octree_object *object;

	ENTER(DESTROY(Octree));
	if (object_address && (object = *object_address))
	{
		DEALLOCATE(object->coordinates);
		DEALLOCATE(*object_address);
		return_code=1;
	}
	else
	{
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Octree_object) */

struct Octree_object_nearest_data
{
	FE_value coordinates[3];
	struct Octree_object *object;
	FE_value distance;
};

static int Octree_object_nearest_iterator(
	struct Octree_object *octree_object, void *Octree_object_nearest_data_void)
{
	FE_value distance, temp;
	int i, return_code;
	struct Octree_object_nearest_data *nearest_data;

	ENTER(Octree_object_nearest_iterator);
	distance = 0.0;
	nearest_data = (struct Octree_object_nearest_data *)Octree_object_nearest_data_void;
	if (octree_object && nearest_data)
	{
		return_code = 1;
		for (i = 0 ; i < OCTREE_DIMENSION ; i++)
		{
			temp = octree_object->coordinates[i] - nearest_data->coordinates[i];
			distance += temp*temp;
		}
		distance = sqrt(distance);
		if ((NULL == nearest_data->object) || (distance < nearest_data->distance))
		{
			nearest_data->object = octree_object;
			nearest_data->distance = distance;
		}
	}
	else
	{
		return_code = 0;
	}
	LEAVE;

	return return_code;
}

struct Octree_object *Octree_object_list_get_nearest(
	struct LIST(Octree_object) *object_list, FE_value *coordinates)
{
	struct Octree_object_nearest_data nearest_data;

	ENTER(Octree_object_list_get_nearest);
	nearest_data.object = NULL;
	if (object_list && coordinates)
	{
		nearest_data.coordinates[0] = coordinates[0];
		nearest_data.coordinates[1] = coordinates[1];
		nearest_data.coordinates[2] = coordinates[2];
		nearest_data.distance = 0.0;
		FOR_EACH_OBJECT_IN_LIST(Octree_object)(
			Octree_object_nearest_iterator, (void *)&nearest_data, object_list);
	}
	return nearest_data.object;
}
