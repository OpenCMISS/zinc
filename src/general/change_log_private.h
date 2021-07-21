/*******************************************************************************
FILE : change_log_private.h

LAST MODIFIED : 15 April 2003

DESCRIPTION :
Structure for storing information about objects added, removed or modified
with and/or without identifier. Used for change messages throughout Cmgui.
Internally the objects are stored in a list indexed by the address of the
object, and containing an enumerator that describes the change. Objects in the
change_log are accessed so that removed objects exist until after clients
get to hear about their imminent demise. Change enumerator logic is slightly
involved -- refer to function CHANGE_LOG_ADD for details.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (CHANGE_LOG_PRIVATE_H)
#define CHANGE_LOG_PRIVATE_H

#include "general/change_log.h"
#include "general/compare.h"
#include "general/indexed_list_private.h"
#include "general/object.h"

/*
Macros
======
*/

/*
Local types
-----------
*/

#if ! defined (SHORT_NAMES)
#define CHANGE_LOG_ENTRY( object_type ) change_log_entry_ ## object_type
#else
#define CHANGE_LOG_ENTRY( object_type ) cle_ ## object_type
#endif

#define FULL_DECLARE_CHANGE_LOG_ENTRY_TYPE( object_type ) \
struct CHANGE_LOG_ENTRY(object_type) \
/***************************************************************************** \
LAST MODIFIED : 9 December 2002 \
\
DESCRIPTION : \
Record of an object and what has changed about it, used internally by the \
change_log. \
============================================================================*/ \
{ \
	/* ACCESSed pointer to changed object */ \
	struct object_type *the_object; \
	/* what has changed about the object */ \
	int change; \
	/* access_count for using list macros */ \
	int access_count; \
}; /* struct CHANGE_LOG_ENTRY(object_type) */ \
\
DECLARE_LIST_TYPES(CHANGE_LOG_ENTRY(object_type)); \
\
FULL_DECLARE_INDEXED_LIST_TYPE(CHANGE_LOG_ENTRY(object_type))

#define FULL_DECLARE_CHANGE_LOG_TYPE( object_type ) \
DECLARE_CHANGE_LOG_TYPE(object_type) \
/***************************************************************************** \
LAST MODIFIED : 4 February 2003 \
\
DESCRIPTION : \
The structure for the change_log. \
???RC could typedef the entry_list to be the change log structure, but may \
find more info is needed later so leave for now. \
============================================================================*/ \
{ \
	/* bitwise OR of all change enumerators in entry_list */ \
	int change_summary; \
	/* bitwise OR of change enumerators set with all change, indicating all objects have changed in this way */ \
	int all_change; \
	/* the list of changed objects with change descriptions */ \
	struct LIST(CHANGE_LOG_ENTRY(object_type)) *entry_list; \
	/* the list of global objects */ \
	struct LIST(object_type) *object_list; \
} /* struct CHANGE_LOG(object_type) */

/*
Module functions
----------------
*/

#define DESTROY_CHANGE_LOG_ENTRY( object_type ) \
	DESTROY(CHANGE_LOG_ENTRY(object_type))

#define DECLARE_DESTROY_CHANGE_LOG_ENTRY_FUNCTION( object_type ) \
static int DESTROY_CHANGE_LOG_ENTRY(object_type)( \
	struct CHANGE_LOG_ENTRY(object_type) **change_log_entry_address) \
/***************************************************************************** \
LAST MODIFIED : 9 December 2002 \
\
DESCRIPTION : \
Note the CREATE for CHANGE_LOG_ENTRY is in the ADD function. \
============================================================================*/ \
{ \
	int return_code; \
	struct CHANGE_LOG_ENTRY(object_type) *change_log_entry; \
\
	ENTER(DESTROY_CHANGE_LOG_ENTRY(object_type)); \
	if (change_log_entry_address && \
		(change_log_entry = *change_log_entry_address)) \
	{ \
		if (0 == change_log_entry->access_count) \
		{ \
			DEACCESS(object_type)(&(change_log_entry->the_object)); \
			DEALLOCATE(*change_log_entry_address); \
			return_code = 1; \
		} \
		else \
		{ \
			return_code = 0; \
		} \
		*change_log_entry_address = (struct CHANGE_LOG_ENTRY(object_type) *)NULL; \
	} \
	else \
	{ \
		return_code = 0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* DESTROY_CHANGE_LOG_ENTRY(object_type) */

#define CHANGE_LOG_ITERATOR_DATA( object_type ) change_log_iterator_data_ ## object_type

#define CHANGE_LOG_ITERATOR( object_type ) change_log_iterator_ ## object_type

#define DEFINE_CHANGE_LOG_ITERATOR_DATA_AND_FUNCTION( object_type ) \
struct CHANGE_LOG_ITERATOR_DATA(object_type) \
{ \
	CHANGE_LOG_ITERATOR_FUNCTION(object_type) *iterator_function; \
	void *user_data; \
}; \
\
static int CHANGE_LOG_ITERATOR(object_type)( \
	struct CHANGE_LOG_ENTRY(object_type) *entry, void *data_void) \
/***************************************************************************** \
LAST MODIFIED : 9 December 2002 \
\
DESCRIPTION : \
Calls, for the object in <entry>, the iterator_function with user_data from the \
<data>. Returns its return value to the caller of this function. \
<data_void> points at a struct CHANGE_LOG_ITERATOR_DATA(object_type). \
============================================================================*/ \
{ \
	int return_code; \
	struct CHANGE_LOG_ITERATOR_DATA(object_type) *data; \
\
	ENTER(CHANGE_LOG_ITERATOR(object_type)); \
	if (entry && \
		(data = (struct CHANGE_LOG_ITERATOR_DATA(object_type) *)data_void) && \
		data->iterator_function) \
	{ \
		return_code = (data->iterator_function)(entry->the_object, entry->change, \
			data->user_data); \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"CHANGE_LOG_ITERATOR(" #object_type ").  Invalid argument(s)"); \
		return_code = 0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* CHANGE_LOG_ITERATOR(object_type) */

/*
Global functions
----------------
*/

#define DECLARE_CREATE_CHANGE_LOG_FUNCTION( object_type ) \
PROTOTYPE_CREATE_CHANGE_LOG_FUNCTION(object_type) \
{ \
	struct CHANGE_LOG(object_type) *change_log; \
\
	ENTER(CREATE_CHANGE_LOG(object_type)); \
	change_log = (struct CHANGE_LOG(object_type) *)NULL; \
	if (object_list) \
	{ \
		if (ALLOCATE(change_log, struct CHANGE_LOG(object_type), 1)) \
		{ \
			change_log->change_summary = CHANGE_LOG_OBJECT_UNCHANGED(object_type); \
			change_log->all_change = CHANGE_LOG_OBJECT_UNCHANGED(object_type); \
			change_log->entry_list = CREATE_LIST(CHANGE_LOG_ENTRY(object_type))(); \
			change_log->object_list = object_list; \
			if (!(change_log->entry_list)) \
			{ \
				display_message(ERROR_MESSAGE, "CREATE_CHANGE_LOG(" #object_type \
					").  Could not create entry_list"); \
				DEALLOCATE(change_log); \
				change_log = (struct CHANGE_LOG(object_type) *)NULL; \
			} \
		} \
		else \
		{ \
			display_message(ERROR_MESSAGE, \
				"CREATE_CHANGE_LOG(" #object_type ").  Could not allocate log"); \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"CREATE_CHANGE_LOG(" #object_type ").  Could not allocate log"); \
	} \
	LEAVE; \
\
	return (change_log); \
} /* CREATE_CHANGE_LOG(object_type) */

#define DECLARE_DESTROY_CHANGE_LOG_FUNCTION( object_type ) \
PROTOTYPE_DESTROY_CHANGE_LOG_FUNCTION(object_type) \
/***************************************************************************** \
LAST MODIFIED : 9 December 2002 \
\
DESCRIPTION : \
Destroys the <**change_log> and sets <*change_log> to NULL. \
============================================================================*/ \
{ \
	int return_code; \
	struct CHANGE_LOG(object_type) *change_log; \
\
	ENTER(DESTROY_CHANGE_LOG(object_type)); \
	if (change_log_address && (NULL != (change_log = *change_log_address))) \
	{ \
		return_code = 1; \
		DESTROY_LIST(CHANGE_LOG_ENTRY(object_type))(&(change_log->entry_list)); \
		DEALLOCATE(*change_log_address); \
		*change_log_address = (struct CHANGE_LOG(object_type) *)NULL; \
	} \
	else \
	{ \
		return_code = 0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* DESTROY_CHANGE_LOG(object_type) */

#define DECLARE_CHANGE_LOG_CLEAR_FUNCTION( object_type ) \
PROTOTYPE_CHANGE_LOG_CLEAR_FUNCTION(object_type) \
/***************************************************************************** \
LAST MODIFIED : 20 February 2003 \
\
DESCRIPTION : \
Clears all entries/flags in the change_log. \
============================================================================*/ \
{ \
	int return_code; \
\
	ENTER(CHANGE_LOG_CLEAR(object_type)); \
	if (change_log) \
	{ \
		change_log->change_summary = CHANGE_LOG_OBJECT_UNCHANGED(object_type); \
		change_log->all_change = CHANGE_LOG_OBJECT_UNCHANGED(object_type); \
		REMOVE_ALL_OBJECTS_FROM_LIST(CHANGE_LOG_ENTRY(object_type))( \
			change_log->entry_list); \
		return_code = 1; \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"CHANGE_LOG_CLEAR(" #object_type ").  Invalid argument(s)"); \
		return_code = 0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* CHANGE_LOG_CLEAR(object_type) */

#define DECLARE_CHANGE_LOG_ALL_CHANGE_FUNCTION( object_type ) \
PROTOTYPE_CHANGE_LOG_ALL_CHANGE_FUNCTION(object_type) \
{ \
	int return_code; \
	if ((change_log) && !(change & (CHANGE_LOG_OBJECT_ADDED(object_type) | CHANGE_LOG_OBJECT_REMOVED(object_type)))) \
	{ \
		change_log->change_summary |= change; \
		change_log->all_change |= change; \
		return_code = 1; \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"CHANGE_LOG_ALL_CHANGE(" #object_type ").  Invalid argument(s)"); \
		return_code = 0; \
	} \
	return (return_code); \
}

#define DECLARE_CHANGE_LOG_MERGE_ALL_CHANGE_INTO_OBJECT_FUNCTION( object_type ) \
/** \
 * Merge all change status into individual object changes, for use prior \
 * to sending change message. \
 */ \
static int object_type ## _change_log_merge_all_change_into_object( \
	struct object_type *object, void *change_log_void) \
{ \
	struct CHANGE_LOG(object_type) *change_log = (struct CHANGE_LOG(object_type) *)change_log_void; \
	return CHANGE_LOG_OBJECT_CHANGE(object_type)(change_log, object, change_log->all_change); \
}

#define DECLARE_CHANGE_LOG_MERGE_ALL_CHANGE_FUNCTION( object_type ) \
PROTOTYPE_CHANGE_LOG_MERGE_ALL_CHANGE_FUNCTION(object_type) \
{ \
	if (change_log) \
	{ \
		if (change_log->all_change != CHANGE_LOG_OBJECT_UNCHANGED(object_type)) \
		{ \
			return FOR_EACH_OBJECT_IN_LIST(object_type)( \
				object_type ## _change_log_merge_all_change_into_object, \
				(void *)change_log, change_log->object_list); \
		} \
		return 1; \
	} \
	return 0; \
}

#define DECLARE_CHANGE_LOG_OBJECT_CHANGE_FUNCTION( object_type ) \
PROTOTYPE_CHANGE_LOG_OBJECT_CHANGE_FUNCTION(object_type) \
/***************************************************************************** \
LAST MODIFIED : 15 April 2003 \
\
DESCRIPTION : \
Tells the <change_log> that <object> has undergone the <change>. \
============================================================================*/ \
{ \
	int return_code; \
	struct CHANGE_LOG_ENTRY(object_type) *entry; \
\
	ENTER(CHANGE_LOG_OBJECT_CHANGE(object_type)); \
	if (change_log && object && (change != CHANGE_LOG_OBJECT_UNCHANGED(object_type))) \
	{ \
		return_code = 1; \
		change_log->change_summary |= change; \
		entry = FIND_BY_IDENTIFIER_IN_LIST(CHANGE_LOG_ENTRY(object_type), \
			the_object)(object, change_log->entry_list); \
		if (NULL != entry) \
		{ \
			switch (change) \
			{ \
				case CHANGE_LOG_OBJECT_ADDED(object_type): \
				{ \
					if (entry->change == CHANGE_LOG_OBJECT_REMOVED(object_type)) \
					{ \
						/* adding after removed is OK but assume it was modified */ \
						entry->change = CHANGE_LOG_OBJECT_CHANGED(object_type); \
					} \
					else \
					{ \
						entry->change = CHANGE_LOG_OBJECT_ADDED(object_type); \
					} \
				} break; \
				case CHANGE_LOG_OBJECT_REMOVED(object_type): \
				{ \
					if (0 != (entry->change & CHANGE_LOG_OBJECT_ADDED(object_type))) \
					{ \
						/* if just been added, no change needs to be noted */ \
						REMOVE_OBJECT_FROM_LIST(CHANGE_LOG_ENTRY(object_type))(entry, \
							change_log->entry_list); \
					} \
					else \
					{ \
						/* REMOVED bit only ever set on its own */ \
						entry->change = CHANGE_LOG_OBJECT_REMOVED(object_type); \
					} \
				} break; \
				default: /* all other bit combinations */ \
				{ \
					/* no change if object removed */ \
					if (entry->change != CHANGE_LOG_OBJECT_REMOVED(object_type)) \
					{ \
						/* if added, only related changes can be merged in */ \
						if (0 != (entry->change & CHANGE_LOG_OBJECT_ADDED(object_type))) \
							entry->change |= (change & CHANGE_LOG_RELATED_OBJECT_CHANGED(object_type)); \
						else \
							entry->change |= change; \
					} \
				} break; \
			} \
		} \
		else \
		{ \
			/* create a new CHANGE_LOG_ENTRY */ \
			/* note CREATE CHANGE_LOG_ENTRY incorporated in this function */ \
			if (ALLOCATE(entry, struct CHANGE_LOG_ENTRY(object_type), 1)) \
			{ \
				/* objects ACCESSed internally to ensure not destroyed */ \
				entry->the_object = ACCESS(object_type)(object); \
				entry->change = change; \
				entry->access_count = 0; \
				if (!ADD_OBJECT_TO_LIST(CHANGE_LOG_ENTRY(object_type))(entry, \
					change_log->entry_list)) \
				{ \
					display_message(ERROR_MESSAGE, \
						"CHANGE_LOG_OBJECT_CHANGE(" #object_type \
						").  Could not add change entry to list"); \
					DESTROY(CHANGE_LOG_ENTRY(object_type))(&entry); \
					return_code = 0; \
				} \
			} \
			else \
			{ \
				display_message(ERROR_MESSAGE, \
					"CHANGE_LOG_OBJECT_CHANGE(" #object_type \
					").  Could not add create change entry"); \
				return_code = 0; \
			} \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"CHANGE_LOG_OBJECT_CHANGE(" #object_type ").  Invalid argument(s)"); \
		return_code = 0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* CHANGE_LOG_OBJECT_CHANGE(object_type) */

#define DECLARE_CHANGE_LOG_GET_CHANGE_SUMMARY_FUNCTION( object_type ) \
PROTOTYPE_CHANGE_LOG_GET_CHANGE_SUMMARY_FUNCTION(object_type) \
/***************************************************************************** \
LAST MODIFIED : 4 February 2003 \
\
DESCRIPTION : \
Returns a bitwise OR of all the changes enumerators in the change_log. Check \
against a particular change by bitwise ANDing with ADD, REMOVE etc. \
============================================================================*/ \
{ \
	int return_code; \
\
	ENTER(CHANGE_LOG_GET_CHANGE_SUMMARY(object_type)); \
	if (change_log && change_summary_address) \
	{ \
		*change_summary_address = change_log->change_summary; \
		return_code = 1; \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"CHANGE_LOG_GET_CHANGE_SUMMARY(" #object_type \
			").  Invalid argument(s)"); \
		return_code = 0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* CHANGE_LOG_GET_CHANGE_SUMMARY(object_type) */

#define DECLARE_CHANGE_LOG_QUERY_FUNCTION( object_type ) \
PROTOTYPE_CHANGE_LOG_QUERY_FUNCTION(object_type) \
/***************************************************************************** \
LAST MODIFIED : 20 February 2003 \
\
DESCRIPTION : \
Returns in <change_address> the status of <object> from <change_log>. \
Note that it is possible for the change to be a union of any of \
OBJECT_ADDED, OBJECT_IDENTIFIER_CHANGED and OBJECT_NOT_IDENTIFIER_CHANGED, \
meaning that any of them may have occured. Hence, the calling function should \
test by performing logical AND against any or all of these states. \
The OBJECT_REMOVED bit always occurs alone. \
Unchanged objects are returned as OBJECT_UNCHANGED. \
============================================================================*/ \
{ \
	int return_code; \
\
	ENTER(CHANGE_LOG_QUERY(object_type)); \
	if (change_log && object && change_address) \
	{ \
		int change = change_log->all_change; \
		struct CHANGE_LOG_ENTRY(object_type) *entry = \
			FIND_BY_IDENTIFIER_IN_LIST(CHANGE_LOG_ENTRY(object_type),the_object)(object, change_log->entry_list); \
		if (0 != entry) \
		{ \
			change = entry->change; \
			/* removed is always on its own */ \
			if (entry->change != CHANGE_LOG_OBJECT_REMOVED(object_type)) \
			{ \
				if (0 != (entry->change & CHANGE_LOG_OBJECT_ADDED(object_type))) \
					change |= (change_log->all_change & CHANGE_LOG_RELATED_OBJECT_CHANGED(object_type)); \
				else \
					change |= change_log->all_change; \
			} \
		} \
		*change_address = change; \
		return_code = 1; \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"CHANGE_LOG_QUERY(" #object_type ").  Invalid argument(s)"); \
		return_code = 0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* CHANGE_LOG_QUERY(object_type) */

#define DECLARE_CHANGE_LOG_FOR_EACH_OBJECT_FUNCTION( object_type ) \
PROTOTYPE_CHANGE_LOG_FOR_EACH_OBJECT_FUNCTION(object_type) \
/***************************************************************************** \
LAST MODIFIED : 9 December 2002 \
\
DESCRIPTION : \
Calls the <iterator_function> with <user_data> for each object in \
<change_log>. \
Note the special format of the CHANGE_LOG_ITERATOR_FUNCTION(object_type), \
which has an additional middle argument of the object's change status. \
============================================================================*/ \
{ \
	int return_code; \
	struct CHANGE_LOG_ITERATOR_DATA(object_type) data; \
\
	ENTER(CHANGE_LOG_FOR_EACH_OBJECT(object_type)); \
	if (change_log && iterator_function) \
	{ \
		data.iterator_function = iterator_function; \
		data.user_data = user_data; \
		return_code = FOR_EACH_OBJECT_IN_LIST(CHANGE_LOG_ENTRY(object_type))( \
			CHANGE_LOG_ITERATOR(object_type), (void *)&data, \
			change_log->entry_list); \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"CHANGE_LOG_FOR_EACH_OBJECT(" #object_type \
			").  Invalid argument(s)"); \
		return_code = 0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* CHANGE_LOG_FOR_EACH_OBJECT(object_type) */

#define DECLARE_CHANGE_LOG_PROPAGATE_PARENT_CHANGE_SUMMARY_FUNCTION( object_type ) \
PROTOTYPE_CHANGE_LOG_PROPAGATE_PARENT_CHANGE_SUMMARY_FUNCTION(object_type) \
{ \
	if (change_log) \
		change_log->change_summary |= CHANGE_LOG_RELATED_OBJECT_CHANGED(object_type); \
}

#define FULL_DECLARE_CHANGE_LOG_TYPES( object_type ) \
FULL_DECLARE_CHANGE_LOG_ENTRY_TYPE(object_type); \
FULL_DECLARE_CHANGE_LOG_TYPE(object_type)

#define DECLARE_CHANGE_LOG_MODULE_FUNCTIONS( object_type ) \
PROTOTYPE_OBJECT_FUNCTIONS(CHANGE_LOG_ENTRY(object_type)); \
PROTOTYPE_LIST_FUNCTIONS(CHANGE_LOG_ENTRY(object_type)); \
DECLARE_DESTROY_CHANGE_LOG_ENTRY_FUNCTION(object_type) \
DECLARE_OBJECT_FUNCTIONS(CHANGE_LOG_ENTRY(object_type)) \
DECLARE_INDEXED_LIST_MODULE_FUNCTIONS(CHANGE_LOG_ENTRY(object_type), \
	the_object, struct object_type *, compare_pointer) \
DECLARE_INDEXED_LIST_FUNCTIONS(CHANGE_LOG_ENTRY(object_type)) \
DECLARE_FIND_BY_IDENTIFIER_IN_INDEXED_LIST_FUNCTION( \
	CHANGE_LOG_ENTRY(object_type), the_object, struct object_type *, \
	compare_pointer) \
DECLARE_CHANGE_LOG_MERGE_ALL_CHANGE_INTO_OBJECT_FUNCTION(object_type) \
DEFINE_CHANGE_LOG_ITERATOR_DATA_AND_FUNCTION(object_type)

#define DECLARE_CHANGE_LOG_FUNCTIONS( object_type ) \
DECLARE_CREATE_CHANGE_LOG_FUNCTION(object_type) \
DECLARE_DESTROY_CHANGE_LOG_FUNCTION(object_type) \
DECLARE_CHANGE_LOG_CLEAR_FUNCTION(object_type) \
DECLARE_CHANGE_LOG_ALL_CHANGE_FUNCTION(object_type) \
DECLARE_CHANGE_LOG_MERGE_ALL_CHANGE_FUNCTION(object_type) \
DECLARE_CHANGE_LOG_OBJECT_CHANGE_FUNCTION(object_type) \
DECLARE_CHANGE_LOG_GET_CHANGE_SUMMARY_FUNCTION(object_type) \
DECLARE_CHANGE_LOG_QUERY_FUNCTION(object_type) \
DECLARE_CHANGE_LOG_FOR_EACH_OBJECT_FUNCTION(object_type) \
DECLARE_CHANGE_LOG_PROPAGATE_PARENT_CHANGE_SUMMARY_FUNCTION(object_type)

#endif /* !defined (CHANGE_LOG_PRIVATE_H) */
