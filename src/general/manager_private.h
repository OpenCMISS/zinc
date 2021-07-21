/*******************************************************************************
FILE : manager_private.h

LAST MODIFIED : 4 May 2010

DESCRIPTION :
Managers oversee the creation, deletion and modification of global objects -
such as object_types, lights and cameras.  All registered clients of the manager
are sent a message when an object has changed.

This file contains the details of the internal workings of the manager and
should be included in the module that declares the manager for a particular
object type.  This allows changes to the internal structure to be made without
affecting other parts of the program.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (MANAGER_PRIVATE_H)
#define MANAGER_PRIVATE_H

#include "general/manager.h"
#include <vector>

/*
Macros
======
*/

/*
Local types
-----------
*/

#define MANAGER_MESSAGE_OBJECT_CHANGE_( object_type ) \
	manager_message_object_change ## object_type
#define MANAGER_MESSAGE_OBJECT_CHANGE( object_type ) \
	MANAGER_MESSAGE_OBJECT_CHANGE_(object_type)

#define MANAGER_CALLBACK_ITEM_( object_type ) \
	manager_callback_item_ ## object_type
#define MANAGER_CALLBACK_ITEM( object_type ) \
	MANAGER_CALLBACK_ITEM_(object_type)

#define MANAGER_EXTRACT_CHANGE_DETAIL_( object_type )  manager_extract_change_detail_ ## object_type
#define MANAGER_EXTRACT_CHANGE_DETAIL( object_type )  MANAGER_EXTRACT_CHANGE_DETAIL_(object_type)

#define MANAGER_CLEANUP_CHANGE_DETAIL_( object_type )  manager_cleanup_change_detail_ ## object_type
#define MANAGER_CLEANUP_CHANGE_DETAIL( object_type )  MANAGER_CLEANUP_CHANGE_DETAIL_(object_type)

#define FULL_DECLARE_MANAGER_TYPE_WITH_OWNER( object_type , owner_type , change_detail_type ) \
change_detail_type MANAGER_EXTRACT_CHANGE_DETAIL(object_type)(struct object_type *object); \
void MANAGER_CLEANUP_CHANGE_DETAIL(object_type)(change_detail_type *change_detail_address); \
struct MANAGER_MESSAGE_OBJECT_CHANGE(object_type) \
/** \
 * Internal record of a single managed object change. \
 */ \
{ \
	struct object_type *object; \
	int change; /* bits set from enum MANAGER_CHANGE(object_type) */ \
	change_detail_type detail; /* optional details of sub-object changes */ \
\
	MANAGER_MESSAGE_OBJECT_CHANGE(object_type)() : \
		object(0), \
		change(MANAGER_CHANGE_NONE(object_type)), \
		detail(0) \
	{ \
	} \
\
	MANAGER_MESSAGE_OBJECT_CHANGE(object_type)(struct object_type *objectIn) : \
		object(ACCESS(object_type)(objectIn)), \
		change(object->manager_change_status), \
		detail(MANAGER_EXTRACT_CHANGE_DETAIL(object_type)(object)) \
	{ \
	} \
\
	~MANAGER_MESSAGE_OBJECT_CHANGE(object_type)() \
	{ \
		MANAGER_CLEANUP_CHANGE_DETAIL(object_type)(&(this->detail)); \
		DEACCESS(object_type)(&(this->object)); \
	} \
}; \
\
struct MANAGER_MESSAGE(object_type) \
/** \
 * A message that will be sent when one of more of the objects being managed \
 * has been added, removed or changed. \
 */ \
{ \
	int change_summary; /* bitwise OR of all object changes */ \
	std::vector<MANAGER_MESSAGE_OBJECT_CHANGE(object_type)*> object_changes; \
	int access_count; \
\
private: \
	MANAGER_MESSAGE(object_type)() : \
		change_summary(MANAGER_CHANGE_NONE(object_type)), \
		access_count(1) \
	{ \
	} \
\
	~MANAGER_MESSAGE(object_type)() \
	{ \
		for (std::vector<MANAGER_MESSAGE_OBJECT_CHANGE(object_type)*>::iterator iter = object_changes.begin(); \
			iter != object_changes.end(); ++iter) \
		{ \
			MANAGER_MESSAGE_OBJECT_CHANGE(object_type) *object_change = *iter; \
			delete object_change; \
		} \
	} \
\
public: \
	static struct MANAGER_MESSAGE(object_type) *create() \
	{ \
		return new MANAGER_MESSAGE(object_type)(); \
	} \
\
	struct MANAGER_MESSAGE(object_type) *access() \
	{ \
		++(this->access_count); \
		return this; \
	} \
\
	static void deaccess(struct MANAGER_MESSAGE(object_type) *&message) \
	{ \
		if (message) \
		{ \
			--(message->access_count); \
			if (message->access_count <= 0) \
				delete message; \
			message = 0; \
		} \
	} \
\
	void addObjectChange(struct object_type *object) \
	{ \
		if (object) \
			this->object_changes.push_back(new MANAGER_MESSAGE_OBJECT_CHANGE(object_type)(object)); \
	} \
\
	int getObjectChangeFlags(struct object_type *object) \
	{ \
		size_t number_of_changed_objects = this->object_changes.size(); \
		struct MANAGER_MESSAGE_OBJECT_CHANGE(object_type) *object_change; \
		for (size_t i = 0; i < number_of_changed_objects; ++i) \
		{ \
			object_change = this->object_changes[i]; \
			if (object == object_change->object) \
				return (object_change->change); \
		} \
		return MANAGER_CHANGE_NONE(object_type); \
	} \
\
	int getObjectChangeFlagsAndDetail(struct object_type *object, const change_detail_type *detail_address) \
	{ \
		if (detail_address) \
		{ \
			size_t number_of_changed_objects = this->object_changes.size(); \
			struct MANAGER_MESSAGE_OBJECT_CHANGE(object_type) *object_change; \
			for (size_t i = 0; i < number_of_changed_objects; ++i) \
			{ \
				object_change = this->object_changes[i]; \
				if (object == object_change->object) \
				{ \
					*detail_address = object_change->detail; \
					return (object_change->change); \
				} \
			} \
			*detail_address = 0; \
		} \
		return MANAGER_CHANGE_NONE(object_type); \
	} \
\
}; /* MANAGER_MESSAGE(object_type) */ \
\
struct MANAGER_CALLBACK_ITEM(object_type) \
/*************************************************************************//** \
 * An item in the manager's list of callbacks to make when changes are made to \
 * the objects being managed \
 */ \
{ \
	MANAGER_CALLBACK_FUNCTION(object_type) *callback; \
	void *user_data; \
	struct MANAGER_CALLBACK_ITEM(object_type) *next; \
}; /* struct MANAGER_CALLBACK_ITEM(object_type) */ \
\
/** \
 * The structure for the manager. \
 */ \
DECLARE_MANAGER_TYPE(object_type) \
{ \
	/* the list of objects in the manager */ \
	struct LIST(object_type) *object_list; \
	/* the list of callbacks invoked when manager has changed */ \
	struct MANAGER_CALLBACK_ITEM(object_type) *callback_list; \
	int locked; \
	/* lists of objects added/changed OR removed since the last update message; \
	 * separate so can add new object with same identifier as one removed. */ \
	struct LIST(object_type) *changed_object_list; \
	struct LIST(object_type) *removed_object_list; \
	/* pointer to owning object which exists for lifetime of this manager, if any */ \
	owner_type *owner; \
	bool external_change; \
	/* flag indicating whether caching is on */ \
	int cache; \
}; /* struct MANAGER(object_type) */

#define FULL_DECLARE_MANAGER_TYPE( object_type ) \
FULL_DECLARE_MANAGER_TYPE_WITH_OWNER(object_type, void, void *)

/*
Local functions
---------------
*/

#define MANAGER_UPDATE_DEPENDENCIES_( object_type )  manager_update_dependencies_ ## object_type
#define MANAGER_UPDATE_DEPENDENCIES( object_type )  MANAGER_UPDATE_DEPENDENCIES_(object_type)

/**
 * Called before manager messages are sent out.
 * Override if objects depend on one another so dependent objects are marked as
 * MANAGER_CHANGE_FULL_RESULT. Examples uses include field, scenefilter.
 */
#define DECLARE_DEFAULT_MANAGER_UPDATE_DEPENDENCIES_FUNCTION( object_type ) \
inline void MANAGER_UPDATE_DEPENDENCIES(object_type)(struct MANAGER(object_type) *manager) \
{ \
	USE_PARAMETER(manager); \
}

/**
 * Default function for no per-object change detail. Does nothing.
 * Override to extract change details from object, if any.
 * Must be cleaned up by MANAGER_CLEANUP_CHANGE_DETAIL, see below.
 * Example uses include field, tessellation.
 */
#define DECLARE_DEFAULT_MANAGER_EXTRACT_CHANGE_DETAIL_FUNCTION( object_type ) \
inline void * MANAGER_EXTRACT_CHANGE_DETAIL(object_type)(struct object_type *object) \
{ \
	USE_PARAMETER(object); \
	return 0; \
}

/**
 * Default function for no per-object change detail. Does nothing.
 * Override to clean up change detail object.
 * Example uses include field, tessellation.
 */
#define DECLARE_DEFAULT_MANAGER_CLEANUP_CHANGE_DETAIL_FUNCTION( object_type ) \
inline void MANAGER_CLEANUP_CHANGE_DETAIL(object_type)(void **change_detail_address) \
{ \
	USE_PARAMETER(change_detail_address); \
}

#define MANAGER_UPDATE_( object_type )  manager_update_ ## object_type
#define MANAGER_UPDATE( object_type )  MANAGER_UPDATE_(object_type)

#define DECLARE_MANAGER_UPDATE_FUNCTION( object_type ) \
static void MANAGER_UPDATE(object_type)(struct MANAGER(object_type) *manager) \
/** \
 * Send a manager message to registered clients about changes to objects in \
 * the manager's changed_object_list and removed_object_list, if any. \
 * Change information is copied out of the manager and objects before the \
 * message is sent. \
 */ \
{ \
	if (manager) \
	{ \
		int number_of_changed_objects = NUMBER_IN_LIST(object_type)(manager->changed_object_list); \
		int number_of_removed_objects = NUMBER_IN_LIST(object_type)(manager->removed_object_list); \
		if (number_of_changed_objects || number_of_removed_objects || manager->external_change) \
		{ \
			MANAGER_UPDATE_DEPENDENCIES(object_type)(manager); \
			manager->external_change = false; \
			number_of_changed_objects = NUMBER_IN_LIST(object_type)(manager->changed_object_list); \
			struct MANAGER_MESSAGE(object_type) *message = MANAGER_MESSAGE(object_type)::create(); \
			if (message) \
			{ \
				for (int i = 0; i < number_of_changed_objects; i++) \
				{ \
					struct object_type *object = FIRST_OBJECT_IN_LIST_THAT(object_type)( \
						(LIST_CONDITIONAL_FUNCTION(object_type) *)NULL, (void *)NULL, manager->changed_object_list); \
					message->addObjectChange(object); \
					message->change_summary |= object->manager_change_status; \
					object->manager_change_status = MANAGER_CHANGE_NONE(object_type); \
					REMOVE_OBJECT_FROM_LIST(object_type)(object, manager->changed_object_list); \
				} \
				for (int i = 0; i < number_of_removed_objects; i++) \
				{ \
					struct object_type *object = FIRST_OBJECT_IN_LIST_THAT(object_type)( \
						(LIST_CONDITIONAL_FUNCTION(object_type) *)NULL, (void *)NULL, manager->removed_object_list); \
					message->addObjectChange(object); \
					message->change_summary |= object->manager_change_status; \
					object->manager_change_status = MANAGER_CHANGE_NONE(object_type); \
					REMOVE_OBJECT_FROM_LIST(object_type)(object, manager->removed_object_list); \
				} \
				/* send message to clients */ \
				struct MANAGER_CALLBACK_ITEM(object_type) *item = manager->callback_list; \
				while (item) \
				{ \
					(item->callback)(message, item->user_data); \
					item = item->next; \
				} \
			} \
			else \
			{ \
				display_message(ERROR_MESSAGE, \
					"MANAGER_UPDATE(" #object_type ").  Could not build message"); \
			} \
			message->deaccess(message); \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"MANAGER_UPDATE(" #object_type ").  Invalid argument(s)"); \
	} \
}

#define MANAGER_FIND_CLIENT( object_type )  manager_find_client ## object_type

#define DECLARE_MANAGER_FIND_CLIENT_FUNCTION( object_type ) \
struct MANAGER_CALLBACK_ITEM(object_type) *MANAGER_FIND_CLIENT(object_type)( \
	void *callback_id,struct MANAGER(object_type) *manager) \
/***************************************************************************** \
LAST MODIFIED : 27 September 1995 \
\
DESCRIPTION : \
Finds a client based on its callback id. \
???DB.  Is this needed ? \
============================================================================*/ \
{ \
	struct MANAGER_CALLBACK_ITEM(object_type) *item,*return_callback; \
\
	ENTER(MANAGER_FIND_CLIENT(object_type)); \
	if (manager&&callback_id) \
	{ \
		item=manager->callback_list; \
		return_callback=(struct MANAGER_CALLBACK_ITEM(object_type) *)NULL; \
		while (item&&!return_callback) \
		{ \
			if (callback_id==item) \
			{ \
				return_callback=item; \
			} \
			else \
			{ \
				item=item->next; \
			} \
		} \
		if (!return_callback) \
		{ \
			display_message(ERROR_MESSAGE, \
				"MANAGER_FIND_CLIENT(" #object_type ").  Could not find client"); \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"MANAGER_FIND_CLIENT(" #object_type ").  Invalid argument(s)"); \
		return_callback=(struct MANAGER_CALLBACK_ITEM(object_type) *)NULL; \
	} \
	LEAVE; \
\
	return (return_callback); \
} /* MANAGER_FIND_CLIENT(object_type) */

#define MANAGED_OBJECT_NOT_IN_USE_CONDITIONAL( object_type ) \
	managed_object_not_in_use_conditional_ ## object_type

#define DECLARE_MANAGED_OBJECT_NOT_IN_USE_CONDITIONAL_FUNCTION( object_type ) \
static int MANAGED_OBJECT_NOT_IN_USE_CONDITIONAL(object_type)( \
	struct object_type *object, void *manager_void) \
/***************************************************************************** \
LAST MODIFIED : 18 January 2002 \
\
DESCRIPTION : \
List conditional version of MANAGED_OBJECT_NOT_IN_USE function. \
============================================================================*/ \
{ \
	int return_code; \
\
	ENTER(MANAGED_OBJECT_NOT_IN_USE_CONDITIONAL(object_type)); \
	return_code = MANAGED_OBJECT_NOT_IN_USE(object_type)(object, \
		(struct MANAGER(object_type) *)manager_void); \
	LEAVE; \
\
	return (return_code); \
} /* MANAGED_OBJECT_NOT_IN_USE_CONDITIONAL(object_type) */

#define OBJECT_CLEAR_MANAGER( object_type ) \
	object_clear_manager ## object_type

#define DECLARE_OBJECT_CLEAR_MANAGER_FUNCTION( object_type , object_manager ) \
static int OBJECT_CLEAR_MANAGER(object_type)(struct object_type *object, \
	void *dummy_user_data) \
/***************************************************************************** \
LAST MODIFIED : 10 August 2000 \
\
DESCRIPTION : \
Remove the reference to the manager from the object \
============================================================================*/ \
{ \
	int return_code; \
\
	ENTER(OBJECT_CLEAR_MANAGER(object_type)); \
   USE_PARAMETER(dummy_user_data); \
	if (object) \
	{ \
		object->object_manager = (struct MANAGER(object_type) *)NULL; \
		return_code = 1; \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"OBJECT_CLEAR_MANAGER(" #object_type ").  Missing object"); \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* OBJECT_CLEAR_MANAGER(object_type) */

/*
Global functions
----------------
*/
#define DECLARE_CREATE_MANAGER_FUNCTION( object_type ) \
PROTOTYPE_CREATE_MANAGER_FUNCTION(object_type) \
{ \
	struct MANAGER(object_type) *manager; \
\
	ENTER(CREATE_MANAGER(object_type)); \
	if (ALLOCATE(manager, struct MANAGER(object_type), 1)) \
	{ \
		manager->object_list = CREATE_LIST(object_type)(); \
		manager->changed_object_list = CREATE_RELATED_LIST(object_type)(manager->object_list); \
		manager->removed_object_list = CREATE_RELATED_LIST(object_type)(manager->object_list); \
		if (manager->object_list && manager->changed_object_list && manager->removed_object_list) \
		{ \
			manager->callback_list = \
				(struct MANAGER_CALLBACK_ITEM(object_type) *)NULL; \
			manager->locked = 0; \
			manager->external_change = false; \
			manager->cache = 0; \
		} \
		else \
		{ \
			display_message(ERROR_MESSAGE, \
				"MANAGER_CREATE(" #object_type ").  Could not create object lists"); \
			DESTROY(LIST(object_type))(&(manager->removed_object_list)); \
			DESTROY(LIST(object_type))(&(manager->changed_object_list)); \
			DESTROY(LIST(object_type))(&(manager->object_list)); \
			DEALLOCATE(manager); \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"MANAGER_CREATE(" #object_type ").  Could not allocate memory"); \
	} \
	LEAVE; \
\
	return (manager); \
} /* MANAGER_CREATE(object_type) */

#define DECLARE_DESTROY_MANAGER_FUNCTION( object_type , \
	object_manager ) \
PROTOTYPE_DESTROY_MANAGER_FUNCTION(object_type) \
{ \
	int return_code; \
	struct MANAGER(object_type) *manager; \
	struct MANAGER_CALLBACK_ITEM(object_type) *current,*next; \
\
	ENTER(DESTROY_MANAGER(object_type)); \
	if (manager_address && (manager= *manager_address)) \
	{ \
		return_code = 1; \
		/* warn if cache should has not to zero */ \
		if (0 != manager->cache) \
		{ \
			display_message(ERROR_MESSAGE, "DESTROY(MANAGER(" #object_type \
				")).  manager->cache = %d != 0", manager->cache); \
		} \
		DESTROY_LIST(object_type)(&(manager->changed_object_list)); \
		DESTROY_LIST(object_type)(&(manager->removed_object_list)); \
		/* remove the manager_pointer from each object */ \
		FOR_EACH_OBJECT_IN_LIST(object_type)(OBJECT_CLEAR_MANAGER(object_type), \
			(void *)NULL, manager->object_list); \
		/* destroy the list of objects in the manager */ \
		DESTROY_LIST(object_type)(&(manager->object_list)); \
		/* destroy the callback list after the list of objects \ 
			to handle Computed_fields getting manager callbacks */ \
		current=manager->callback_list; \
		while (current) \
		{ \
			next = current->next; \
			DEALLOCATE(current); \
			current = next; \
		} \
		DEALLOCATE(manager); \
	} \
	else \
	{ \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* DESTROY_MANAGER(object_type) */

#define DECLARE_MANAGER_CREATE_LIST_FUNCTION( object_type ) \
PROTOTYPE_MANAGER_CREATE_LIST_FUNCTION(object_type) \
{ \
	struct LIST(object_type) *list; \
\
	ENTER(MANAGER_CREATE_LIST(object_type)); \
	list = NULL; \
	if (manager) \
	{ \
		list = CREATE_RELATED_LIST(object_type)(manager->object_list); \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"MANAGER_CREATE_LIST(" #object_type ").  Missing manager"); \
	} \
	LEAVE; \
\
	return (list); \
}

#define DECLARE_ADD_OBJECT_TO_MANAGER_FUNCTION( object_type , \
	identifier_field_name , object_manager ) \
PROTOTYPE_ADD_OBJECT_TO_MANAGER_FUNCTION(object_type) \
{ \
	int return_code; \
\
	ENTER(ADD_OBJECT_TO_MANAGER(object_type)); \
	if (manager && object) \
	{ \
		if (!object->object_manager) \
		{ \
			if (!(manager->locked)) \
			{ \
				/* can only add if new identifier not already in use in manager */ \
				if ((struct object_type *)NULL == \
					FIND_BY_IDENTIFIER_IN_LIST(object_type, identifier_field_name)( \
						object->identifier_field_name, manager->object_list)) \
				{ \
					/* object_list will access the object */ \
					if (ADD_OBJECT_TO_LIST(object_type)(object, manager->object_list)) \
					{ \
						/* object keeps a pointer to its manager */ \
						object->object_manager = manager; \
						object->manager_change_status = MANAGER_CHANGE_ADD(object_type); \
						ADD_OBJECT_TO_LIST(object_type)(object, manager->changed_object_list); \
						if (!manager->cache) \
						{ \
							MANAGER_UPDATE(object_type)(manager); \
						} \
						return_code = 1; \
					} \
					else \
					{ \
						display_message(ERROR_MESSAGE, \
							"ADD_OBJECT_TO_MANAGER(" #object_type \
							").  Could not add object to list"); \
						return_code = 0; \
					} \
				} \
				else \
				{ \
					display_message(ERROR_MESSAGE, \
						"ADD_OBJECT_TO_MANAGER(" #object_type \
						").  Object with that identifier already in manager"); \
					return_code = 0; \
				} \
			} \
			else \
			{ \
				display_message(WARNING_MESSAGE, \
					"ADD_OBJECT_TO_MANAGER(" #object_type \
					").  Manager locked"); \
				return_code = 0; \
			} \
		} \
		else \
		{ \
			display_message(ERROR_MESSAGE, \
				"ADD_OBJECT_TO_MANAGER(" #object_type \
				").  Object already managed"); \
			return_code = 0; \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"ADD_OBJECT_TO_MANAGER(" #object_type \
			").  Invalid argument(s)"); \
		return_code = 0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* ADD_OBJECT_TO_MANAGER(object_type) */

#define DECLARE_REMOVE_OBJECT_FROM_MANAGER_FUNCTION( \
	object_type , object_manager ) \
PROTOTYPE_REMOVE_OBJECT_FROM_MANAGER_FUNCTION(object_type) \
{ \
	int return_code; \
\
	ENTER(REMOVE_OBJECT_FROM_MANAGER(object_type)); \
	if (manager && object) \
	{ \
		if (object->object_manager == manager) \
		{ \
			if (!(manager->locked)) \
			{ \
				if (MANAGED_OBJECT_NOT_IN_USE(object_type)(object, manager)) \
				{ \
					/* clear manager pointer first so list DEACCESS doesn't remove from manager again */ \
					object->object_manager = (struct MANAGER(object_type) *)NULL; \
					if (object->manager_change_status != MANAGER_CHANGE_NONE(object_type)) \
					{ \
						REMOVE_OBJECT_FROM_LIST(object_type)(object, manager->changed_object_list); \
					} \
					/* do not inform clients about objects added and removed during caching */ \
					if (object->manager_change_status != MANAGER_CHANGE_ADD(object_type)) \
					{ \
						/* removed_object_list ACCESSes removed objects until message sent */ \
						ADD_OBJECT_TO_LIST(object_type)(object, manager->removed_object_list); \
					} \
					object->manager_change_status = MANAGER_CHANGE_REMOVE(object_type); \
					return_code = REMOVE_OBJECT_FROM_LIST(object_type)(object, manager->object_list); \
					if (!manager->cache) \
					{ \
						MANAGER_UPDATE(object_type)(manager); \
					} \
				} \
				else \
				{ \
					display_message(ERROR_MESSAGE, \
						"REMOVE_OBJECT_FROM_MANAGER(" #object_type ").  Object is in use"); \
					return_code = 0; \
				} \
			} \
			else \
			{ \
				display_message(WARNING_MESSAGE, \
					"REMOVE_OBJECT_FROM_MANAGER(" #object_type ").  Manager locked"); \
				return_code = 0; \
			} \
		} \
		else \
		{ \
			display_message(WARNING_MESSAGE, \
				"REMOVE_OBJECT_FROM_MANAGER(" #object_type ").  Object is not in this manager"); \
			return_code = 0; \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"REMOVE_OBJECT_FROM_MANAGER(" #object_type ").  Invalid argument(s)"); \
		return_code = 0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* REMOVE_OBJECT_FROM_MANAGER(object_type) */

#define DECLARE_REMOVE_ALL_OBJECTS_FROM_MANAGER_FUNCTION( \
	object_type , object_manager ) \
PROTOTYPE_REMOVE_ALL_OBJECTS_FROM_MANAGER_FUNCTION(object_type) \
{ \
	int return_code; \
	struct object_type *object; \
\
	ENTER(REMOVE_ALL_OBJECTS_FROM_MANAGER(object_type)); \
	if (manager) \
	{ \
		if (!(manager->locked)) \
		{ \
			return_code = 1; \
			MANAGER_BEGIN_CACHE(object_type)(manager); \
			while (return_code && (object = FIRST_OBJECT_IN_LIST_THAT(object_type)( \
				MANAGED_OBJECT_NOT_IN_USE_CONDITIONAL(object_type), (void *)manager, \
				manager->object_list))) \
			{ \
				return_code = REMOVE_OBJECT_FROM_MANAGER(object_type)(object, manager); \
			} \
			if (0 != NUMBER_IN_MANAGER(object_type)(manager)) \
			{ \
				display_message(ERROR_MESSAGE, \
					"REMOVE_ALL_OBJECTS_FROM_MANAGER(" #object_type \
					").  %d items could not be removed", \
					NUMBER_IN_MANAGER(object_type)(manager)); \
				return_code = 0; \
			} \
			MANAGER_END_CACHE(object_type)(manager); \
		} \
		else \
		{ \
			display_message(WARNING_MESSAGE, \
				"REMOVE_ALL_OBJECTS_FROM_MANAGER(" #object_type ").  Manager locked"); \
			return_code = 0; \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"REMOVE_ALL_OBJECTS_FROM_MANAGER(" #object_type \
			").  Invalid argument(s)"); \
		return_code = 0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* REMOVE_ALL_OBJECTS_FROM_MANAGER(object_type) */

#define DECLARE_NUMBER_IN_MANAGER_FUNCTION( object_type ) \
PROTOTYPE_NUMBER_IN_MANAGER_FUNCTION(object_type) \
{ \
	int return_code; \
\
	ENTER(NUMBER_IN_MANAGER(object_type)); \
	if (manager) \
	{ \
		if (!(manager->locked)) \
		{ \
			return_code = NUMBER_IN_LIST(object_type)(manager->object_list); \
		} \
		else \
		{ \
			display_message(WARNING_MESSAGE, \
				"NUMBER_IN_MANAGER(" #object_type ").  Manager is locked"); \
			return_code = 0; \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"NUMBER_IN_MANAGER(" #object_type ").  Invalid argument(s)"); \
		return_code = 0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* NUMBER_IN_MANAGER(object_type) */

#define DECLARE_MANAGER_MODIFY_FUNCTION( object_type , identifier_field_name ) \
PROTOTYPE_MANAGER_MODIFY_FUNCTION(object_type,identifier_field_name) \
{ \
	int return_code; \
	struct LIST_IDENTIFIER_CHANGE_DATA(object_type, \
		identifier_field_name) *identifier_change_data; \
	struct object_type *tmp_object; \
\
	ENTER(MANAGER_MODIFY(object_type,identifier_field_name)); \
	if (manager && object && new_data && (object != new_data)) \
	{ \
		if (!(manager->locked)) \
		{ \
			if (IS_OBJECT_IN_LIST(object_type)(object, manager->object_list)) \
			{ \
				tmp_object = FIND_BY_IDENTIFIER_IN_LIST(object_type, identifier_field_name)( \
					new_data->identifier_field_name, manager->object_list); \
				if (tmp_object != object) \
				{ \
					display_message(ERROR_MESSAGE, \
						"MANAGER_MODIFY(" #object_type "," #identifier_field_name \
						").  Identifier of source object is already used in manager"); \
					return_code = 0; \
				} \
				else \
				{ \
					identifier_change_data = LIST_BEGIN_IDENTIFIER_CHANGE( \
						object_type, identifier_field_name)(object); \
					if (identifier_change_data) \
					{ \
						return_code = MANAGER_COPY_WITH_IDENTIFIER(object_type, \
							identifier_field_name)(object, new_data); \
						if (!return_code) \
						{ \
							display_message(ERROR_MESSAGE, \
								"MANAGER_MODIFY(" #object_type "," #identifier_field_name \
								").  Could not copy object"); \
						} \
						if (!LIST_END_IDENTIFIER_CHANGE(object_type, \
							identifier_field_name)(&identifier_change_data)) \
						{ \
							display_message(ERROR_MESSAGE, \
								"MANAGER_MODIFY(" #object_type "," #identifier_field_name \
								").  Could not restore object to all indexed lists"); \
						} \
						/* notify clients AFTER object restored to indexed lists */ \
						if (return_code) \
						{ \
							MANAGED_OBJECT_CHANGE(object_type)(object, \
								MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER(object_type) | \
								MANAGER_CHANGE_IDENTIFIER(object_type)); \
						} \
					} \
					else \
					{ \
						display_message(ERROR_MESSAGE, \
							"MANAGER_MODIFY(" #object_type "," #identifier_field_name \
							").  Could not safely change identifier in indexed lists"); \
						return_code = 0; \
					} \
				} \
			} \
			else \
			{ \
				display_message(ERROR_MESSAGE, \
					"MANAGER_MODIFY(" #object_type "," #identifier_field_name \
					").  Object is not managed"); \
				return_code = 0; \
			} \
		} \
		else \
		{ \
			display_message(WARNING_MESSAGE, \
				"MANAGER_MODIFY(" #object_type "," #identifier_field_name \
				").  Manager locked"); \
			return_code = 0; \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"MANAGER_MODIFY(" #object_type "," #identifier_field_name \
			").  Invalid argument(s)"); \
		return_code = 0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* MANAGER_MODIFY(object_type,identifier_field_name) */

#define DECLARE_MANAGER_MODIFY_NOT_IDENTIFIER_FUNCTION( object_type , \
	identifier_field_name ) \
PROTOTYPE_MANAGER_MODIFY_NOT_IDENTIFIER_FUNCTION(object_type, \
	identifier_field_name) \
{ \
	int return_code; \
\
	ENTER(MANAGER_MODIFY_NOT_IDENTIFIER(object_type,identifier_field_name)); \
	if (manager && object && new_data) \
	{ \
		if (!(manager->locked)) \
		{ \
			if (IS_OBJECT_IN_LIST(object_type)(object,manager->object_list)) \
			{ \
				return_code = MANAGER_COPY_WITHOUT_IDENTIFIER(object_type, \
					identifier_field_name)(object, new_data); \
				if (return_code) \
				{ \
					MANAGED_OBJECT_CHANGE(object_type)(object, \
						MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER(object_type)); \
				} \
				else  \
				{ \
					display_message(ERROR_MESSAGE, \
						"MANAGER_MODIFY_NOT_IDENTIFIER(" #object_type "," \
						#identifier_field_name ").  Could not copy object"); \
					return_code = 0; \
				} \
			} \
			else \
			{ \
				display_message(ERROR_MESSAGE, \
					"MANAGER_MODIFY_NOT_IDENTIFIER(" #object_type "," \
					#identifier_field_name ").  Object is not managed"); \
				return_code = 0; \
			} \
		} \
		else \
		{ \
			display_message(WARNING_MESSAGE, \
				"MANAGER_MODIFY_NOT_IDENTIFIER(" #object_type "," \
				#identifier_field_name ").  Manager is locked"); \
			return_code = 0; \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"MANAGER_MODIFY_NOT_IDENTIFIER(" #object_type "," \
			#identifier_field_name ").  Invalid argument(s)"); \
		return_code = 0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* MANAGER_MODIFY_NOT_IDENTIFIER(object_type,identifier_field_name) */

#define DECLARE_MANAGER_MODIFY_IDENTIFIER_FUNCTION( object_type , \
	identifier_field_name , identifier_type ) \
PROTOTYPE_MANAGER_MODIFY_IDENTIFIER_FUNCTION(object_type, \
	identifier_field_name,identifier_type) \
{ \
	int return_code; \
	struct LIST_IDENTIFIER_CHANGE_DATA(object_type, \
		identifier_field_name) *identifier_change_data; \
	struct object_type *tmp_object; \
\
	ENTER(MANAGER_MODIFY_IDENTIFIER(object_type,identifier_field_name)); \
	if (manager && object && new_identifier) \
	{ \
		if (!(manager->locked)) \
		{ \
			if (IS_OBJECT_IN_LIST(object_type)(object,manager->object_list)) \
			{ \
				tmp_object = FIND_BY_IDENTIFIER_IN_LIST(object_type, identifier_field_name)( \
					new_identifier, manager->object_list); \
				if (NULL != tmp_object) \
				{ \
					if (tmp_object == object) \
					{ \
						/* don't need to copy identifier of object over itself */ \
						return_code = 1; \
					} \
					else \
					{ \
						display_message(ERROR_MESSAGE,"MANAGER_MODIFY_IDENTIFIER(" \
							#object_type "," #identifier_field_name \
							").  Identifier is already used in manager"); \
						return_code = 0; \
					} \
				} \
				else \
				{ \
					identifier_change_data = LIST_BEGIN_IDENTIFIER_CHANGE( \
						object_type, identifier_field_name)(object); \
					if (identifier_change_data) \
					{ \
						return_code = MANAGER_COPY_IDENTIFIER(object_type, \
							identifier_field_name)(object, new_identifier); \
						if (!return_code) \
						{ \
							display_message(ERROR_MESSAGE, \
								"MANAGER_MODIFY_IDENTIFIER(" #object_type "," \
								#identifier_field_name ").  Could not copy identifier"); \
						} \
						if (!LIST_END_IDENTIFIER_CHANGE(object_type, \
							identifier_field_name)(&identifier_change_data)) \
						{ \
							display_message(ERROR_MESSAGE, \
								"MANAGER_MODIFY_IDENTIFIER(" #object_type "," \
								#identifier_field_name \
								").  Could not restore object to all indexed lists"); \
						} \
						/* notify clients AFTER object restored to indexed lists */ \
						if (return_code) \
						{ \
							MANAGED_OBJECT_CHANGE(object_type)(object, \
								MANAGER_CHANGE_IDENTIFIER(object_type)); \
						} \
					} \
					else  \
					{ \
						display_message(ERROR_MESSAGE, \
							"MANAGER_MODIFY_IDENTIFIER(" #object_type "," \
							#identifier_field_name \
							").  Could not safely change identifier in indexed lists"); \
						return_code = 0; \
					} \
				} \
			} \
			else \
			{ \
				display_message(ERROR_MESSAGE, \
					"MANAGER_MODIFY_IDENTIFIER(" #object_type "," #identifier_field_name \
					").  Object is not managed"); \
				return_code=0; \
			} \
		} \
		else \
		{ \
			display_message(WARNING_MESSAGE, \
				"MANAGER_MODIFY_IDENTIFIER(" #object_type "," #identifier_field_name \
				").  Manager is locked"); \
			return_code=0; \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"MANAGER_MODIFY_IDENTIFIER(" #object_type "," #identifier_field_name \
			").  Invalid argument(s)"); \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* MANAGER_MODIFY_IDENTIFIER(object_type,identifier_field_name) */

#define DECLARE_MANAGER_REGISTER_FUNCTION( object_type ) \
PROTOTYPE_MANAGER_REGISTER_FUNCTION(object_type) \
{ \
	struct MANAGER_CALLBACK_ITEM(object_type) *new_callback; \
	void *callback_id; \
\
	ENTER(MANAGER_REGISTER(object_type)); \
	if (manager&&callback) \
	{ \
		if (ALLOCATE(new_callback,struct MANAGER_CALLBACK_ITEM(object_type),1)) \
		{ \
			new_callback->next=manager->callback_list; \
			new_callback->callback=callback; \
			new_callback->user_data=user_data; \
			manager->callback_list=new_callback; \
			callback_id=(void *)new_callback; \
		} \
		else \
		{ \
			display_message(ERROR_MESSAGE, \
			"MANAGER_REGISTER(" #object_type ").  Could not allocate new callback"); \
			callback_id=(void *)NULL; \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"MANAGER_REGISTER(" #object_type ").  Invalid argument(s)"); \
		callback_id=(void *)NULL; \
	} \
	LEAVE; \
\
	return (callback_id); \
} /* MANAGER_REGISTER(object_type) */

#define DECLARE_MANAGER_DEREGISTER_FUNCTION( object_type ) \
PROTOTYPE_MANAGER_DEREGISTER_FUNCTION(object_type) \
{ \
	int return_code; \
	struct MANAGER_CALLBACK_ITEM(object_type) *item,**item_address; \
\
	ENTER(MANAGER_DEREGISTER(object_type)); \
	if (manager&&callback_id) \
	{ \
		item_address= &(manager->callback_list); \
		while ((*item_address)&&(*item_address!=callback_id)) \
		{ \
				item_address= &((*item_address)->next); \
		} \
		if (NULL != (item= *item_address)) \
		{ \
			/* found it */ \
			*item_address=item->next; \
			DEALLOCATE(item); \
			return_code=1; \
		} \
		else \
		{ \
			display_message(ERROR_MESSAGE, \
				"MANAGER_DEREGISTER(" #object_type ").  Could not find callback"); \
			return_code=0; \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"MANAGER_DEREGISTER(" #object_type ").  Invalid argument(s)"); \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* MANAGER_DEREGISTER(object_type) */

#define DECLARE_IS_MANAGED_FUNCTION( object_type ) \
PROTOTYPE_IS_MANAGED_FUNCTION(object_type) \
{ \
	int return_code; \
\
	ENTER(IS_MANAGED(object_type)); \
	if (manager&&object) \
	{ \
		if (!(manager->locked)) \
		{ \
			return_code=IS_OBJECT_IN_LIST(object_type)(object,manager->object_list); \
		} \
		else \
		{ \
			display_message(WARNING_MESSAGE, \
				"IS_MANAGED(" #object_type ").  Manager is locked"); \
			return_code=0; \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"IS_MANAGED(" #object_type ").  Invalid argument(s)"); \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* IS_MANAGED(object_type) */

#define DECLARE_DEFAULT_MANAGED_OBJECT_NOT_IN_USE_FUNCTION( object_type , object_manager ) \
PROTOTYPE_MANAGED_OBJECT_NOT_IN_USE_FUNCTION(object_type) \
/***************************************************************************** \
LAST MODIFIED : 21 January 2002 \
\
DESCRIPTION : \
Returns true if <object> is only accessed by the manager or other managed \
objects. In general, a true result is sufficient to indicate the object may be \
removed from the manager or modified. \
This default version may be used for any object that cannot be accessed by \
other objects in the manager. It only checks the object is accessed just by \
the manager's object_list and changed_object_list or removed_object_list. \
FE_element type requires a special version due to face/parent accessing. \
============================================================================*/ \
{ \
	int return_code; \
\
	ENTER(MANAGED_OBJECT_NOT_IN_USE(object_type)); \
	return_code = 0; \
	if (manager && object) \
	{ \
		if (manager == object->object_manager) \
		{ \
			if ((1 == object->access_count) || \
				((MANAGER_CHANGE_NONE(object_type) != object->manager_change_status) && \
				 (2 == object->access_count))) \
			{ \
				return_code = 1; \
			} \
		} \
		else \
		{ \
			display_message(WARNING_MESSAGE, \
				"MANAGED_OBJECT_NOT_IN_USE(" #object_type \
				").  Object is not in this manager"); \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"MANAGED_OBJECT_NOT_IN_USE(" #object_type ").  Invalid argument(s)"); \
	} \
	LEAVE; \
\
	return (return_code); \
} /* MANAGED_OBJECT_NOT_IN_USE(object_type) */

#define DECLARE_FIND_BY_IDENTIFIER_IN_MANAGER_FUNCTION( object_type , \
	identifier , identifier_type ) \
PROTOTYPE_FIND_BY_IDENTIFIER_IN_MANAGER_FUNCTION(object_type,identifier, \
	identifier_type) \
{ \
	struct object_type *object; \
\
	ENTER(FIND_BY_IDENTIFIER_IN_MANAGER(object_type,identifier)); \
	if (manager) \
	{ \
		if (!(manager->locked)) \
		{ \
			object=FIND_BY_IDENTIFIER_IN_LIST(object_type,identifier)( \
				identifier,manager->object_list); \
		} \
		else \
		{ \
			display_message(WARNING_MESSAGE, \
				"FIND_BY_IDENTIFIER_IN_LIST(" #object_type "," #identifier \
				").  Manager is locked"); \
			object=(struct object_type *)NULL; \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"FIND_BY_IDENTIFIER_IN_LIST(" #object_type "," #identifier \
			").  Invalid argument(s)"); \
		object=(struct object_type *)NULL; \
	} \
	LEAVE; \
\
	return (object); \
} /* FIND_BY_IDENTIFIER_IN_MANAGER(object_type,identifier) */

#define DECLARE_FIRST_OBJECT_IN_MANAGER_THAT_FUNCTION( object_type ) \
PROTOTYPE_FIRST_OBJECT_IN_MANAGER_THAT_FUNCTION(object_type) \
{ \
	struct object_type *object; \
\
	ENTER(FIRST_OBJECT_IN_MANAGER_THAT(object_type)); \
	if (manager) \
	{ \
		object=FIRST_OBJECT_IN_LIST_THAT(object_type)(conditional,user_data, \
			manager->object_list); \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"FIRST_OBJECT_IN_MANAGER_THAT(" #object_type ").  Invalid argument(s)"); \
		object=(struct object_type *)NULL; \
	} \
	LEAVE; \
\
	return (object); \
} /* FIRST_OBJECT_IN_MANAGER_THAT(object_type) */

#define DECLARE_FOR_EACH_OBJECT_IN_MANAGER_FUNCTION( object_type ) \
PROTOTYPE_FOR_EACH_OBJECT_IN_MANAGER_FUNCTION(object_type) \
{ \
	int return_code; \
\
	ENTER(FOR_EACH_OBJECT_IN_MANAGER(object_type)); \
	if (manager&&iterator) \
	{ \
		return_code=FOR_EACH_OBJECT_IN_LIST(object_type)(iterator,user_data, \
			manager->object_list); \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"FOR_EACH_OBJECT_IN_MANAGER(" #object_type ").  Invalid argument(s)"); \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* FOR_EACH_OBJECT_IN_MANAGER(object_type) */

#define DECLARE_MANAGED_OBJECT_CHANGE_FUNCTION( object_type , object_manager ) \
PROTOTYPE_MANAGED_OBJECT_CHANGE_FUNCTION(object_type) \
{ \
	int return_code; \
\
	ENTER(MANAGED_OBJECT_CHANGE(object_type)); \
	if (object) \
	{ \
		if (object->object_manager) \
		{ \
			return_code = 1; \
			if (0 == (object->manager_change_status & MANAGER_CHANGE_ADD(object_type))) \
			{ \
				if (object->manager_change_status == MANAGER_CHANGE_NONE(object_type)) \
				{ \
					ADD_OBJECT_TO_LIST(object_type)(object, \
						object->object_manager->changed_object_list); \
				} \
				object->manager_change_status |= change; \
			} \
			if (!object->object_manager->cache) \
			{ \
				MANAGER_UPDATE(object_type)(object->object_manager); \
			} \
		} \
		else \
		{ \
			return_code = 0; \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"MANAGED_OBJECT_CHANGE(" #object_type ").  Invalid argument(s)"); \
		return_code = 0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* MANAGED_OBJECT_CHANGE(object_type) */
			
#define DECLARE_MANAGER_BEGIN_CACHE_FUNCTION( object_type ) \
PROTOTYPE_MANAGER_BEGIN_CACHE_FUNCTION(object_type) \
{ \
	int return_code; \
\
	ENTER(MANAGER_BEGIN_CACHE(object_type)); \
	if (manager) \
	{ \
		/* increment cache so we can safely nest caching */ \
		(manager->cache)++; \
		return_code = 1; \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"MANAGER_BEGIN_CACHE(" #object_type ").  Invalid argument"); \
		return_code = 0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* MANAGER_BEGIN_CACHE(object_type) */

#define DECLARE_MANAGER_END_CACHE_FUNCTION( object_type ) \
PROTOTYPE_MANAGER_END_CACHE_FUNCTION(object_type) \
{ \
	int return_code; \
\
	ENTER(MANAGER_END_CACHE(object_type)); \
	if (manager) \
	{ \
		if (0 < manager->cache) \
		{ \
			/* decrement cache so we can safely nest caching */ \
			(manager->cache)--; \
			if (0 == manager->cache) \
			{ \
				/* once cache has run out, inform clients of all changes to date */ \
				MANAGER_UPDATE(object_type)(manager); \
			} \
			return_code = 1; \
		} \
		else \
		{ \
			display_message(ERROR_MESSAGE, \
				"MANAGER_END_CACHE(" #object_type ").  Caching not enabled"); \
			return_code = 0; \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"MANAGER_END_CACHE(" #object_type ").  Invalid argument"); \
		return_code = 0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* MANAGER_END_CACHE(object_type) */

#define DECLARE_MANAGER_EXTERNAL_CHANGE_FUNCTION( object_type ) \
PROTOTYPE_MANAGER_EXTERNAL_CHANGE_FUNCTION( object_type ) \
{ \
	if (manager) \
	{ \
		manager->external_change = true; \
		if (!manager->cache) \
			MANAGER_UPDATE(object_type)(manager); \
	} \
}

#define DECLARE_MANAGER_MESSAGE_ACCESS_FUNCTION( object_type ) \
PROTOTYPE_MANAGER_MESSAGE_ACCESS_FUNCTION( object_type )  \
{ \
	if (message) \
		return message->access(); \
	return 0; \
}

#define DECLARE_MANAGER_MESSAGE_DEACCESS_FUNCTION( object_type ) \
PROTOTYPE_MANAGER_MESSAGE_DEACCESS_FUNCTION( object_type )  \
{ \
	if (message_address) \
		MANAGER_MESSAGE(object_type)::deaccess(*message_address); \
}

#define DECLARE_MANAGER_MESSAGE_GET_CHANGE_SUMMARY_FUNCTION( object_type ) \
PROTOTYPE_MANAGER_MESSAGE_GET_CHANGE_SUMMARY_FUNCTION(object_type) \
{ \
	if (message) \
		return (message->change_summary); \
	return (MANAGER_CHANGE_NONE(object_type)); \
}

#define DECLARE_MANAGER_MESSAGE_GET_OBJECT_CHANGE_FUNCTION( object_type ) \
PROTOTYPE_MANAGER_MESSAGE_GET_OBJECT_CHANGE_FUNCTION(object_type) \
{ \
	if (message) \
		return message->getObjectChangeFlags(object); \
	return MANAGER_CHANGE_NONE(object_type); \
}

#define DECLARE_MANAGER_MESSAGE_GET_CHANGE_LIST_FUNCTION( object_type ) \
PROTOTYPE_MANAGER_MESSAGE_GET_CHANGE_LIST_FUNCTION(object_type) \
{ \
	struct LIST(object_type) *object_list = 0; \
	if (message) \
	{ \
		if (message->change_summary & change_mask) \
		{ \
			object_list = CREATE(LIST(object_type))(); \
			size_t number_of_changed_objects = message->object_changes.size(); \
			struct MANAGER_MESSAGE_OBJECT_CHANGE(object_type) *object_change; \
			for (size_t i = 0; i < number_of_changed_objects; ++i) \
			{ \
				object_change = message->object_changes[i]; \
				if (object_change->change & change_mask) \
					ADD_OBJECT_TO_LIST(object_type)(object_change->object, object_list); \
			} \
		} \
	} \
	return (object_list); \
}

#define DECLARE_MANAGER_MESSAGE_HAS_CHANGED_OBJECT_THAT_FUNCTION( object_type ) \
PROTOTYPE_MANAGER_MESSAGE_HAS_CHANGED_OBJECT_THAT_FUNCTION(object_type) \
{ \
	if (message && conditional_function) \
	{ \
		size_t number_of_changed_objects = message->object_changes.size(); \
		struct MANAGER_MESSAGE_OBJECT_CHANGE(object_type) *object_change; \
		for (size_t i = 0; i < number_of_changed_objects; ++i) \
		{ \
			object_change = message->object_changes[i]; \
			if ((object_change->change & change_mask) && \
				(conditional_function)(object_change->object, user_data)) \
				return 1; \
		} \
	} \
	return 0; \
}

#define MANAGER_GET_OWNER_(object_type)  manager_get_owner_ ## object_type
#define MANAGER_GET_OWNER(object_type)  MANAGER_GET_OWNER_(object_type)

#define PROTOTYPE_MANAGER_GET_OWNER_FUNCTION( object_type, owner_type ) \
owner_type *MANAGER_GET_OWNER(object_type)(struct MANAGER(object_type) *manager) \
/***************************************************************************** \
LAST MODIFIED : 5 March 2009 \
\
DESCRIPTION : \
Private function to get the owning object for this manager. \
==============================================================================*/

#define DECLARE_MANAGER_GET_OWNER_FUNCTION( object_type , owner_type ) \
PROTOTYPE_MANAGER_GET_OWNER_FUNCTION(object_type,owner_type) \
{ \
	owner_type *owner; \
\
	ENTER(MANAGER_GET_OWNER(object_type)); \
	if (manager) \
	{ \
		owner = manager->owner; \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"MANAGER_GET_OWNER(" #object_type ").  Missing manager"); \
		owner = (owner_type *)NULL; \
	} \
	LEAVE; \
\
	return (owner); \
} /* MANAGER_GET_OWNER(object_type) */

#define MANAGER_SET_OWNER_(object_type)  manager_set_owner_ ## object_type
#define MANAGER_SET_OWNER(object_type)  MANAGER_SET_OWNER_(object_type)

#define PROTOTYPE_MANAGER_SET_OWNER_FUNCTION( object_type, owner_type ) \
int MANAGER_SET_OWNER(object_type)(struct MANAGER(object_type) *manager, \
	owner_type *owner) \
/***************************************************************************** \
LAST MODIFIED : 5 March 2009 \
\
DESCRIPTION : \
Private function to set the owning object for this manager. \
==============================================================================*/

#define DECLARE_MANAGER_SET_OWNER_FUNCTION( object_type , owner_type ) \
PROTOTYPE_MANAGER_SET_OWNER_FUNCTION(object_type,owner_type) \
{ \
	int return_code; \
\
	ENTER(MANAGER_SET_OWNER(object_type)); \
	if (manager && owner) \
	{ \
		manager->owner = owner; \
		return_code = 1; \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"MANAGER_SET_OWNER(" #object_type ").  Missing manager"); \
		return_code = 0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* MANAGER_SET_OWNER(object_type) */

#define PROTOTYPE_MANAGER_OWNER_FUNCTIONS(object_type, owner_type) \
PROTOTYPE_MANAGER_GET_OWNER_FUNCTION(object_type, owner_type); \
PROTOTYPE_MANAGER_SET_OWNER_FUNCTION(object_type, owner_type)

#define DECLARE_MANAGER_OWNER_FUNCTIONS(object_type, owner_type) \
DECLARE_MANAGER_GET_OWNER_FUNCTION(object_type, owner_type) \
DECLARE_MANAGER_SET_OWNER_FUNCTION(object_type, owner_type)

#define DECLARE_LOCAL_MANAGER_FUNCTIONS(object_type) \
DECLARE_DEFAULT_MANAGER_UPDATE_DEPENDENCIES_FUNCTION(object_type) \
DECLARE_DEFAULT_MANAGER_EXTRACT_CHANGE_DETAIL_FUNCTION(object_type) \
DECLARE_DEFAULT_MANAGER_CLEANUP_CHANGE_DETAIL_FUNCTION(object_type) \
DECLARE_MANAGER_UPDATE_FUNCTION(object_type) \
DECLARE_MANAGER_FIND_CLIENT_FUNCTION(object_type) \
DECLARE_MANAGED_OBJECT_NOT_IN_USE_CONDITIONAL_FUNCTION(object_type)

/* NOTE: MUST clear the objects pointer to manager in its CREATE function! */
#define DECLARE_MANAGER_FUNCTIONS( object_type , object_manager ) \
DECLARE_CREATE_MANAGER_FUNCTION(object_type) \
DECLARE_OBJECT_CLEAR_MANAGER_FUNCTION(object_type,object_manager) \
DECLARE_DESTROY_MANAGER_FUNCTION(object_type,object_manager) \
DECLARE_MANAGER_CREATE_LIST_FUNCTION(object_type) \
DECLARE_REMOVE_OBJECT_FROM_MANAGER_FUNCTION(object_type,object_manager) \
DECLARE_REMOVE_ALL_OBJECTS_FROM_MANAGER_FUNCTION(object_type,object_manager) \
DECLARE_NUMBER_IN_MANAGER_FUNCTION(object_type) \
DECLARE_MANAGER_REGISTER_FUNCTION(object_type) \
DECLARE_MANAGER_DEREGISTER_FUNCTION(object_type) \
DECLARE_IS_MANAGED_FUNCTION(object_type) \
DECLARE_FIRST_OBJECT_IN_MANAGER_THAT_FUNCTION(object_type) \
DECLARE_FOR_EACH_OBJECT_IN_MANAGER_FUNCTION(object_type) \
DECLARE_MANAGED_OBJECT_CHANGE_FUNCTION(object_type,object_manager) \
DECLARE_MANAGER_BEGIN_CACHE_FUNCTION(object_type) \
DECLARE_MANAGER_END_CACHE_FUNCTION(object_type) \
DECLARE_MANAGER_EXTERNAL_CHANGE_FUNCTION(object_type) \
DECLARE_MANAGER_MESSAGE_ACCESS_FUNCTION(object_type) \
DECLARE_MANAGER_MESSAGE_DEACCESS_FUNCTION(object_type) \
DECLARE_MANAGER_MESSAGE_GET_CHANGE_SUMMARY_FUNCTION(object_type) \
DECLARE_MANAGER_MESSAGE_GET_OBJECT_CHANGE_FUNCTION(object_type) \
DECLARE_MANAGER_MESSAGE_GET_CHANGE_LIST_FUNCTION(object_type) \
DECLARE_MANAGER_MESSAGE_HAS_CHANGED_OBJECT_THAT_FUNCTION(object_type)

#define DECLARE_MANAGER_IDENTIFIER_WITHOUT_MODIFY_FUNCTIONS( \
	object_type , identifier_field_name , identifier_type , object_manager ) \
DECLARE_ADD_OBJECT_TO_MANAGER_FUNCTION(object_type, \
	identifier_field_name, object_manager) \
DECLARE_FIND_BY_IDENTIFIER_IN_MANAGER_FUNCTION(object_type, \
	identifier_field_name,identifier_type)

#define DECLARE_MANAGER_IDENTIFIER_FUNCTIONS( \
	object_type , identifier_field_name , identifier_type , object_manager ) \
DECLARE_MANAGER_IDENTIFIER_WITHOUT_MODIFY_FUNCTIONS( \
	object_type , identifier_field_name , identifier_type , object_manager ) \
DECLARE_MANAGER_MODIFY_FUNCTION(object_type,identifier_field_name) \
DECLARE_MANAGER_MODIFY_NOT_IDENTIFIER_FUNCTION(object_type, \
	identifier_field_name) \
DECLARE_MANAGER_MODIFY_IDENTIFIER_FUNCTION(object_type,identifier_field_name, \
	identifier_type)

#endif /* !defined (MANAGER_PRIVATE_H) */
