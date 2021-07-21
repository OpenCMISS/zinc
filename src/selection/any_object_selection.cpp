/*******************************************************************************
FILE : any_object_selection.c

LAST MODIFIED : 23 August 2000

DESCRIPTION :
Global store of selected any_objects for group actions and highlighting.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stdio.h>
#include "general/callback_private.h"
#include "general/debug.h"
#include "selection/any_object_selection.h"
#include "general/message.h"

/*
Module types
------------
*/

FULL_DECLARE_CMZN_CALLBACK_TYPES(Any_object_selection_change, \
	struct Any_object_selection *,struct Any_object_selection_changes *);

struct Any_object_selection
/*******************************************************************************
LAST MODIFIED : 23 August 2000

DESCRIPTION :
Global store of selected any_objects for group actions and highlighting.
==============================================================================*/
{
	/* flag indicating whether the cache is on */
	int cache;
	/* list of all any_objects currently selected */
	struct LIST(Any_object) *any_object_list;
	/* list of all any_objects selected since the last update; includes those
		 already selected; excludes those subsequently unselected */
	struct LIST(Any_object) *newly_selected_any_object_list;
	/* list of all any_objects unselected since the last update; excludes those
		 not already selected; excludes those subsequently selected */
	struct LIST(Any_object) *newly_unselected_any_object_list;
	/* list of callbacks requested by other objects when selction changes */
	struct LIST(CMZN_CALLBACK_ITEM(Any_object_selection_change)) *change_callback_list;
}; /* struct Any_object_selection */

/*
Module functions
----------------
*/

DEFINE_CMZN_CALLBACK_MODULE_FUNCTIONS(Any_object_selection_change, void)

DEFINE_CMZN_CALLBACK_FUNCTIONS(Any_object_selection_change, \
	struct Any_object_selection *,struct Any_object_selection_changes *)

static int Any_object_selection_update(
	struct Any_object_selection *any_object_selection)
/*******************************************************************************
LAST MODIFIED : 23 August 2000

DESCRIPTION :
Tells the clients of the <any_object_selection> that objects have been selected
or unselected since the last update callback. Avoids sending message if caching
is on or if no such changes have occurred.
==============================================================================*/
{
	int return_code;
	struct Any_object_selection_changes changes;

	ENTER(Any_object_selection_update);
	if (any_object_selection)
	{
		if (!any_object_selection->cache)
		{
			/* only update if something changed */
			if ((0<NUMBER_IN_LIST(Any_object)(
				any_object_selection->newly_selected_any_object_list)) ||
				(0<NUMBER_IN_LIST(Any_object)(
					any_object_selection->newly_unselected_any_object_list)))
			{
				/* construct the call data */
				changes.newly_selected_any_object_list=
					any_object_selection->newly_selected_any_object_list;
				changes.newly_unselected_any_object_list=
					any_object_selection->newly_unselected_any_object_list;
				/* send the callbacks */
				CMZN_CALLBACK_LIST_CALL(Any_object_selection_change)(
					any_object_selection->change_callback_list,
					any_object_selection,&changes);
				/* clear the newly selected and unselected lists */
				REMOVE_ALL_OBJECTS_FROM_LIST(Any_object)(
					any_object_selection->newly_selected_any_object_list);
				REMOVE_ALL_OBJECTS_FROM_LIST(Any_object)(
					any_object_selection->newly_unselected_any_object_list);
			}
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Any_object_selection_update.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Any_object_selection_update */

/*
Global functions
----------------
*/

struct Any_object_selection *CREATE(Any_object_selection)(void)
/*******************************************************************************
LAST MODIFIED : 23 August 2000

DESCRIPTION :
Creates the global store of selected any_objects for group actions and
highlighting.
==============================================================================*/
{
	struct Any_object_selection *any_object_selection;

	ENTER(CREATE(Any_object_selection));
	if (ALLOCATE(any_object_selection,struct Any_object_selection,1))
	{
		any_object_selection->cache=0;
		any_object_selection->any_object_list=CREATE(LIST(Any_object))();
		any_object_selection->newly_selected_any_object_list=
			CREATE(LIST(Any_object))();
		any_object_selection->newly_unselected_any_object_list=
			CREATE(LIST(Any_object))();
		any_object_selection->change_callback_list=
			CREATE(LIST(CMZN_CALLBACK_ITEM(Any_object_selection_change)))();
		if (!(any_object_selection->any_object_list&&
			any_object_selection->newly_selected_any_object_list&&
			any_object_selection->newly_unselected_any_object_list&&
			any_object_selection->change_callback_list))
		{
			display_message(ERROR_MESSAGE,
				"CREATE(Any_object_selection).  Could not create lists");
			DESTROY(Any_object_selection)(&any_object_selection);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CREATE(Any_object_selection).  Not enough memory");
	}
	LEAVE;

	return (any_object_selection);
} /* CREATE(Any_object_selection) */

int DESTROY(Any_object_selection)(
	struct Any_object_selection **any_object_selection_address)
/*******************************************************************************
LAST MODIFIED : 23 August 2000

DESCRIPTION :
Destroys the Any_object_selection.
==============================================================================*/
{
	int return_code;
	struct Any_object_selection *any_object_selection;

	ENTER(DESTROY(Any_object_selection));
	if (any_object_selection_address&&
		(any_object_selection= *any_object_selection_address))
	{
		DESTROY(LIST(Any_object))(&(any_object_selection->any_object_list));
		DESTROY(LIST(Any_object))(
			&(any_object_selection->newly_selected_any_object_list));
		DESTROY(LIST(Any_object))(
			&(any_object_selection->newly_unselected_any_object_list));
		DESTROY(LIST(CMZN_CALLBACK_ITEM(Any_object_selection_change)))(
			&(any_object_selection->change_callback_list));
		DEALLOCATE(*any_object_selection_address);
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(Any_object_selection).  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Any_object_selection) */

int Any_object_selection_add_callback(
	struct Any_object_selection *any_object_selection,
	CMZN_CALLBACK_FUNCTION(Any_object_selection_change) *function,void *user_data)
/*******************************************************************************
LAST MODIFIED : 23 August 2000

DESCRIPTION :
Adds a callback to <any_object_selection> so that when it changes <function> is
called with <user_data>. <function> has 3 arguments, a struct
Any_object_selection, a struct Any_object_selection_changes and the
void *user_data.
==============================================================================*/
{
	int return_code;

	ENTER(Any_object_selection_add_callback);
	if (any_object_selection&&function)
	{
		if (CMZN_CALLBACK_LIST_ADD_CALLBACK(Any_object_selection_change)(
			any_object_selection->change_callback_list,function,user_data))
		{
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Any_object_selection_add_callback.  Could not add callback");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Any_object_selection_add_callback.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Any_object_selection_add_callback */

int Any_object_selection_remove_callback(
	struct Any_object_selection *any_object_selection,
	CMZN_CALLBACK_FUNCTION(Any_object_selection_change) *function,void *user_data)
/*******************************************************************************
LAST MODIFIED : 23 August 2000

DESCRIPTION :
Removes the callback calling <function> with <user_data> from
<any_object_selection>.
==============================================================================*/
{
	int return_code;

	ENTER(Any_object_selection_remove_callback);
	if (any_object_selection&&function)
	{
		if (CMZN_CALLBACK_LIST_REMOVE_CALLBACK(Any_object_selection_change)(
			any_object_selection->change_callback_list,function,user_data))
		{
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Any_object_selection_remove_callback.  Could not remove callback");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Any_object_selection_remove_callback.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Any_object_selection_remove_callback */

int Any_object_selection_begin_cache(
	struct Any_object_selection *any_object_selection)
/*******************************************************************************
LAST MODIFIED : 23 August 2000

DESCRIPTION :
Starts caching for changes in the <any_object_selection>. No update callbacks
will occur until Any_object_selection_end_cache is called.
==============================================================================*/
{
	int return_code;

	ENTER(Any_object_selection_begin_cache);
	if (any_object_selection)
	{
		if (any_object_selection->cache)
		{
			display_message(ERROR_MESSAGE,
				"Any_object_selection_begin_cache.  Cache already enabled");
			return_code=0;
		}
		else
		{
			any_object_selection->cache=1;
			return_code=1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Any_object_selection_begin_cache.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Any_object_selection_begin_cache */

int Any_object_selection_end_cache(
	struct Any_object_selection *any_object_selection)
/*******************************************************************************
LAST MODIFIED : 23 August 2000

DESCRIPTION :
Ends caching for selected list changes in the <selection>. Calls
Any_object_selection_update to inform clients of the changes that took place
while caching was on.
==============================================================================*/
{
	int return_code;

	ENTER(Any_object_selection_end_cache);
	if (any_object_selection)
	{
		if (any_object_selection->cache)
		{
			any_object_selection->cache=0;
			Any_object_selection_update(any_object_selection);
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Any_object_selection_end_cache.  Cache not enabled");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Any_object_selection_end_cache.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Any_object_selection_end_cache */

int Any_object_selection_clear(
	struct Any_object_selection *any_object_selection)
/*******************************************************************************
LAST MODIFIED : 23 August 2000

DESCRIPTION :
Clears all the selected any_objects in the <any_object_selection>.
==============================================================================*/
{
	int return_code;

	ENTER(Any_object_selection_clear);
	if (any_object_selection)
	{
		FOR_EACH_OBJECT_IN_LIST(Any_object)(ensure_Any_object_is_in_list,
			(void *)any_object_selection->newly_unselected_any_object_list,
			any_object_selection->any_object_list);
		REMOVE_ALL_OBJECTS_FROM_LIST(Any_object)(
			any_object_selection->any_object_list);
		REMOVE_ALL_OBJECTS_FROM_LIST(Any_object)(
			any_object_selection->newly_selected_any_object_list);
		Any_object_selection_update(any_object_selection);
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Any_object_selection_clear.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Any_object_selection_clear */

int Any_object_selection_select_any_object(
	struct Any_object_selection *any_object_selection,
	struct Any_object *any_object)
/*******************************************************************************
LAST MODIFIED : 23 August 2000

DESCRIPTION :
Ensures <any_object> is in <any_object_selection>. Even if it is already
selected, <any_object> is added to the newly_selected_any_object_list and
removed from the newly_unselected_any_object_list if it is currently there.
Calls Any_object_selection_update.
==============================================================================*/
{
	int return_code;

	ENTER(Any_object_selection_select_any_object);
	if (any_object_selection&&any_object)
	{
		return_code=ensure_Any_object_is_in_list(any_object,
			(void *)any_object_selection->any_object_list);
		ensure_Any_object_is_in_list(any_object,
			(void *)any_object_selection->newly_selected_any_object_list);
		ensure_Any_object_is_not_in_list(any_object,
			(void *)any_object_selection->newly_unselected_any_object_list);
		Any_object_selection_update(any_object_selection);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Any_object_selection_select_any_object.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Any_object_selection_select_any_object */

int Any_object_select_in_Any_object_selection(struct Any_object *any_object,
	void *any_object_selection_void)
/*******************************************************************************
LAST MODIFIED : 23 August 2000

DESCRIPTION :
Any_object iterator version of Any_object_selection_select_any_object.
==============================================================================*/
{
	int return_code;

	ENTER(Any_object_select_in_Any_object_selection);
	return_code=Any_object_selection_select_any_object(
		(struct Any_object_selection *)any_object_selection_void,any_object);
	LEAVE;

	return (return_code);
} /* Any_object_select_in_Any_object_selection */

int Any_object_selection_unselect_any_object(
	struct Any_object_selection *any_object_selection,
	struct Any_object *any_object)
/*******************************************************************************
LAST MODIFIED : 23 August 2000

DESCRIPTION :
If <any_object> is in <any_object_selection> it is removed and added to the
newly_unselected_any_object_list. Note it is added to this list if not currently
selected. Also ensures it is removed from the newly_selected_any_object_list if
it is currently there. Calls Any_object_selection_update.
==============================================================================*/
{
	int return_code;

	ENTER(Any_object_selection_unselect_any_object);
	if (any_object_selection&&any_object)
	{
		if (IS_OBJECT_IN_LIST(Any_object)(any_object,
			any_object_selection->any_object_list))
		{
			ADD_OBJECT_TO_LIST(Any_object)(any_object,
				any_object_selection->newly_unselected_any_object_list);
			ensure_Any_object_is_not_in_list(any_object,
				(void *)any_object_selection->newly_selected_any_object_list);
			return_code=REMOVE_OBJECT_FROM_LIST(Any_object)(any_object,
				any_object_selection->any_object_list);
			Any_object_selection_update(any_object_selection);
		}
		else
		{
			return_code=1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Any_object_selection_unselect_any_object.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Any_object_selection_unselect_any_object */

int Any_object_unselect_in_Any_object_selection(struct Any_object *any_object,
	void *any_object_selection_void)
/*******************************************************************************
LAST MODIFIED : 23 August 2000

DESCRIPTION :
Any_object iterator version of Any_object_selection_unselect_any_object.
==============================================================================*/
{
	int return_code;

	ENTER(Any_object_unselect_in_Any_object_selection);
	return_code=Any_object_selection_unselect_any_object(
		(struct Any_object_selection *)any_object_selection_void,any_object);
	LEAVE;

	return (return_code);
} /* Any_object_unselect_in_Any_object_selection */

struct LIST(Any_object) *Any_object_selection_get_any_object_list(
	struct Any_object_selection *any_object_selection)
/*******************************************************************************
LAST MODIFIED : 23 August 2000

DESCRIPTION :
Returns the list containing the any_objects currently in <any_object_selection>.
RETURNED LIST IS NOT TO BE MODIFIED.
==============================================================================*/
{
	struct LIST(Any_object) *any_object_list;

	ENTER(Any_object_selection_get_any_object_list);
	if (any_object_selection)
	{
		any_object_list=any_object_selection->any_object_list;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Any_object_selection_get_any_object_list.  Invalid argument(s)");
		any_object_list=(struct LIST(Any_object) *)NULL;
	}
	LEAVE;

	return (any_object_list);
} /* Any_object_selection_get_any_object_list */

int Any_object_selection_is_any_object_selected(
	struct Any_object_selection *any_object_selection,
	struct Any_object *any_object)
/*******************************************************************************
LAST MODIFIED : 23 August 2000

DESCRIPTION :
Returns true if the <any_object> is in the <any_object_selection>.
==============================================================================*/
{
	int return_code;

	ENTER(Any_object_selection_is_any_object_selected);
	if (any_object_selection&&any_object)
	{
		if (IS_OBJECT_IN_LIST(Any_object)(any_object,
			any_object_selection->any_object_list))
		{
			return_code=1;
		}
		else
		{
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Any_object_selection_is_any_object_selected.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Any_object_selection_is_any_object_selected */
