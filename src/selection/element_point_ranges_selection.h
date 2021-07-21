/*******************************************************************************
FILE : element_point_ranges_selection.h

LAST MODIFIED : 18 May 2000

DESCRIPTION :
Global store of selected element_point_ranges for group actions and
highlighting.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (ELEMENT_POINT_RANGES_SELECTION_H)
#define ELEMENT_POINT_RANGES_SELECTION_H

#include "general/callback.h"
#include "general/object.h"
#include "graphics/element_point_ranges.h"

/*
Global types
------------
*/

struct Element_point_ranges_selection;
/*******************************************************************************
LAST MODIFIED : 27 March 2000

DESCRIPTION :
Global store of selected element_point_ranges for group actions and
highlighting.
The contents of this object are private.
==============================================================================*/

struct Element_point_ranges_selection_changes
/*******************************************************************************
LAST MODIFIED : 27 March 2000

DESCRIPTION :
Data broadcast with callbacks from <Element_point_ranges_selection> describing
the changes.
==============================================================================*/
{
	/* list of element_point_ranges selected since the last update callback -
		 includes those already selected; excludes those subsequently unselected */
	struct LIST(Element_point_ranges) *newly_selected_element_point_ranges_list;
	/* list of element_point_ranges unselected since the last update callback -
		 excludes both those already unselected and those subsequently reselected */
	struct LIST(Element_point_ranges) *newly_unselected_element_point_ranges_list;
}; /* struct Element_point_ranges_selection_changes */

DECLARE_CMZN_CALLBACK_TYPES(Element_point_ranges_selection_change, \
	struct Element_point_ranges_selection *, \
	struct Element_point_ranges_selection_changes *, void);

struct FE_element_select_grid_field_ranges_data
/*******************************************************************************
LAST MODIFIED : 18 May 2000

DESCRIPTION :
Data for passing to FE_element_[un]select_grid_field_ranges.
==============================================================================*/
{
	struct Element_point_ranges_selection *element_point_ranges_selection;
	struct FE_field *grid_field;
	struct Multi_range *ranges;
};

/*
Global functions
----------------
*/
struct Element_point_ranges_selection *CREATE(Element_point_ranges_selection)(
	void);
/*******************************************************************************
LAST MODIFIED : 27 March 2000

DESCRIPTION :
Creates the global store of selected element_point_ranges for group actions and
highlighting.
==============================================================================*/

int DESTROY(Element_point_ranges_selection)(
	struct Element_point_ranges_selection
	**element_point_ranges_selection_address);
/*******************************************************************************
LAST MODIFIED : 27 March 2000

DESCRIPTION :
Destroys the Element_point_ranges_selection.
==============================================================================*/

int Element_point_ranges_selection_add_callback(
	struct Element_point_ranges_selection *element_point_ranges_selection,
	CMZN_CALLBACK_FUNCTION(Element_point_ranges_selection_change) *function,
	void *user_data);
/*******************************************************************************
LAST MODIFIED : 27 March 2000

DESCRIPTION :
Adds a callback to <element_point_ranges_selection> so that when it changes
<function> is called with <user_data>. <function> has 3 arguments, a
struct Element_point_ranges_selection, a
struct Element_point_ranges_selection_changes and the void *user_data.
==============================================================================*/

int Element_point_ranges_selection_remove_callback(
	struct Element_point_ranges_selection *element_point_ranges_selection,
	CMZN_CALLBACK_FUNCTION(Element_point_ranges_selection_change) *function,
	void *user_data);
/*******************************************************************************
LAST MODIFIED : 27 March 2000

DESCRIPTION :
Removes the callback calling <function> with <user_data> from
<element_point_ranges_selection>.
==============================================================================*/

int Element_point_ranges_selection_begin_cache(
	struct Element_point_ranges_selection *element_point_ranges_selection);
/*******************************************************************************
LAST MODIFIED : 27 March 2000

DESCRIPTION :
Starts caching for changes in the <element_point_ranges_selection>. No update
callbacks will occur until Element_point_ranges_selection_end_cache is called.
==============================================================================*/

int Element_point_ranges_selection_end_cache(
	struct Element_point_ranges_selection *element_point_ranges_selection);
/*******************************************************************************
LAST MODIFIED : 27 March 2000

DESCRIPTION :
Ends caching for selected list changes in the <selection>. Calls
Element_point_ranges_selection_update to inform clients of the changes that
took place while caching was on.
==============================================================================*/

int Element_point_ranges_selection_clear(
	struct Element_point_ranges_selection *element_point_ranges_selection);
/*******************************************************************************
LAST MODIFIED : 27 March 2000

DESCRIPTION :
Clears all the selected element_point_ranges in the
<element_point_ranges_selection>.
==============================================================================*/

int Element_point_ranges_selection_select_element_point_ranges(
	struct Element_point_ranges_selection *element_point_ranges_selection,
	struct Element_point_ranges *element_point_ranges);
/*******************************************************************************
LAST MODIFIED : 27 March 2000

DESCRIPTION :
Ensures <element_point_ranges> is in <element_point_ranges_selection>. Even if
it is already selected, <element_point_ranges> is added to the
newly_selected_element_point_ranges_list and removed from the
newly_unselected_element_point_ranges_list if it is currently there.
Calls Element_point_ranges_selection_update.
==============================================================================*/

int Element_point_ranges_select(
	struct Element_point_ranges *element_point_ranges,
	void *element_point_ranges_selection_void);
/*******************************************************************************
LAST MODIFIED : 26 May 2000

DESCRIPTION :
Element_point_ranges iterator version of
Element_point_ranges_selection_select_element_point_ranges
==============================================================================*/

int Element_point_ranges_selection_unselect_element_point_ranges(
	struct Element_point_ranges_selection *element_point_ranges_selection,
	struct Element_point_ranges *element_point_ranges);
/*******************************************************************************
LAST MODIFIED : 27 March 2000

DESCRIPTION :
If <element_point_ranges> is in <element_point_ranges_selection> it is removed
and added to the newly_unselected_element_point_ranges_list. Note it is added to
this list if not currently selected. Also ensures it is removed from the
newly_selected_element_point_ranges_list if it is currently there. Calls
Element_point_ranges_selection_update.
==============================================================================*/

int Element_point_ranges_unselect(
	struct Element_point_ranges *element_point_ranges,
	void *element_point_ranges_selection_void);
/*******************************************************************************
LAST MODIFIED : 26 May 2000

DESCRIPTION :
Element_point_ranges iterator version of
Element_point_ranges_selection_unselect_element_point_ranges
==============================================================================*/

struct LIST(Element_point_ranges)
	*Element_point_ranges_selection_get_element_point_ranges_list(
		struct Element_point_ranges_selection *element_point_ranges_selection);
/*******************************************************************************
LAST MODIFIED : 27 March 2000

DESCRIPTION :
Returns the list containing the element_point_ranges currently in
<element_point_ranges_selection>.
RETURNED LIST AND OBJECTS IN IT ARE NOT TO BE MODIFIED.
==============================================================================*/

int Element_point_ranges_selection_is_element_point_ranges_selected(
	struct Element_point_ranges_selection *element_point_ranges_selection,
	struct Element_point_ranges *element_point_ranges);
/*******************************************************************************
LAST MODIFIED : 27 March 2000

DESCRIPTION :
Returns true if <element_point_ranges> overlaps any of the
selected_element_point_ranges in <element_point_ranges_selection>.
==============================================================================*/

#endif /* !defined (ELEMENT_POINT_RANGES_SELECTION_H) */
