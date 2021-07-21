/*******************************************************************************
FILE : any_object_selection.h

LAST MODIFIED : 23 August 2000

DESCRIPTION :
Global store of selected any_objects for group actions and highlighting.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (ANY_OBJECT_SELECTION_H)
#define ANY_OBJECT_SELECTION_H

#include "general/any_object.h"
#include "general/callback.h"
#include "general/object.h"

/*
Global types
------------
*/

struct Any_object_selection;
/*******************************************************************************
LAST MODIFIED : 23 August 2000

DESCRIPTION :
Global store of selected any_objects for group actions and highlighting.
The contents of this object are private.
==============================================================================*/

struct Any_object_selection_changes
/*******************************************************************************
LAST MODIFIED : 23 August 2000

DESCRIPTION :
Data broadcast with callbacks from <Any_object_selection> describing the changes.
==============================================================================*/
{
	/* list of any_objects selected since the last update callback - includes those
     already selected; excludes those subsequently unselected */
	struct LIST(Any_object) *newly_selected_any_object_list;
	/* list of any_objects unselected since the last update callback - excludes
		 both those already unselected and those subsequently reselected */
	struct LIST(Any_object) *newly_unselected_any_object_list;
}; /* struct Any_object_selection_changes */

DECLARE_CMZN_CALLBACK_TYPES(Any_object_selection_change, \
	struct Any_object_selection *,struct Any_object_selection_changes *, void);

/*
Global functions
----------------
*/
struct Any_object_selection *CREATE(Any_object_selection)(void);
/*******************************************************************************
LAST MODIFIED : 23 August 2000

DESCRIPTION :
Creates the global store of selected any_objects for group actions and
highlighting.
==============================================================================*/

int DESTROY(Any_object_selection)(
	struct Any_object_selection **any_object_selection_address);
/*******************************************************************************
LAST MODIFIED : 23 August 2000

DESCRIPTION :
Destroys the Any_object_selection.
==============================================================================*/

int Any_object_selection_add_callback(
	struct Any_object_selection *any_object_selection,
	CMZN_CALLBACK_FUNCTION(Any_object_selection_change) *function,void *user_data);
/*******************************************************************************
LAST MODIFIED : 23 August 2000

DESCRIPTION :
Adds a callback to <any_object_selection> so that when it changes <function> is
called with <user_data>. <function> has 3 arguments, a struct
Any_object_selection, a struct Any_object_selection_changes and the
void *user_data.
==============================================================================*/

int Any_object_selection_remove_callback(
	struct Any_object_selection *any_object_selection,
	CMZN_CALLBACK_FUNCTION(Any_object_selection_change) *function,void *user_data);
/*******************************************************************************
LAST MODIFIED : 23 August 2000

DESCRIPTION :
Removes the callback calling <function> with <user_data> from
<any_object_selection>.
==============================================================================*/

int Any_object_selection_begin_cache(
	struct Any_object_selection *any_object_selection);
/*******************************************************************************
LAST MODIFIED : 23 August 2000

DESCRIPTION :
Starts caching for changes in the <any_object_selection>. No update callbacks
will occur until Any_object_selection_end_cache is called.
==============================================================================*/

int Any_object_selection_end_cache(
	struct Any_object_selection *any_object_selection);
/*******************************************************************************
LAST MODIFIED : 23 August 2000

DESCRIPTION :
Ends caching for selected list changes in the <selection>. Calls
Any_object_selection_update to inform clients of the changes that took place
while caching was on.
==============================================================================*/

int Any_object_selection_clear(
	struct Any_object_selection *any_object_selection);
/*******************************************************************************
LAST MODIFIED : 23 August 2000

DESCRIPTION :
Clears all the selected any_objects in the <any_object_selection>.
==============================================================================*/

int Any_object_selection_select_any_object(
	struct Any_object_selection *any_object_selection,
	struct Any_object *any_object);
/*******************************************************************************
LAST MODIFIED : 23 August 2000

DESCRIPTION :
Ensures <any_object> is in <any_object_selection>. Even if it is already
selected, <any_object> is added to the newly_selected_any_object_list and
removed from the newly_unselected_any_object_list if it is currently there.
Calls Any_object_selection_update.
==============================================================================*/

int Any_object_select_in_Any_object_selection(struct Any_object *any_object,
	void *any_object_selection_void);
/*******************************************************************************
LAST MODIFIED : 23 August 2000

DESCRIPTION :
Any_object iterator version of Any_object_selection_select_any_object.
==============================================================================*/

int Any_object_selection_unselect_any_object(
	struct Any_object_selection *any_object_selection,
	struct Any_object *any_object);
/*******************************************************************************
LAST MODIFIED : 23 August 2000

DESCRIPTION :
If <any_object> is in <any_object_selection> it is removed and added to the
newly_unselected_any_object_list. Note it is added to this list if not currently
selected. Also ensures it is removed from the newly_selected_any_object_list if
it is currently there. Calls Any_object_selection_update.
==============================================================================*/

int Any_object_unselect_in_Any_object_selection(struct Any_object *any_object,
	void *any_object_selection_void);
/*******************************************************************************
LAST MODIFIED : 23 August 2000

DESCRIPTION :
Any_object iterator version of Any_object_selection_unselect_any_object.
==============================================================================*/

struct LIST(Any_object) *Any_object_selection_get_any_object_list(
	struct Any_object_selection *any_object_selection);
/*******************************************************************************
LAST MODIFIED : 23 August 2000

DESCRIPTION :
Returns the list containing the any_objects currently in <any_object_selection>.
RETURNED LIST IS NOT TO BE MODIFIED.
==============================================================================*/

int Any_object_selection_is_any_object_selected(
	struct Any_object_selection *any_object_selection,
	struct Any_object *any_object);
/*******************************************************************************
LAST MODIFIED : 23 August 2000

DESCRIPTION :
Returns true if the <any_object> is in the <any_object_selection>.
==============================================================================*/

#endif /* !defined (ANY_OBJECT_SELECTION_H) */
