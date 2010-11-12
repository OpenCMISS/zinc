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
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is cmgui.
 *
 * The Initial Developer of the Original Code is
 * Auckland Uniservices Ltd, Auckland, New Zealand.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
#if !defined (MANAGER_PRIVATE_H)
#define MANAGER_PRIVATE_H

#include "general/manager.h"

/*
Macros
======
*/

/*
Local types
-----------
*/

#define MANAGER_MESSAGE_OBJECT_CHANGE( object_type ) \
	manager_message_object_change ## object_type

#define MANAGER_CALLBACK_ITEM( object_type ) \
	manager_callback_item_ ## object_type

#define FULL_DECLARE_MANAGER_TYPE_WITH_OWNER( object_type , owner_type ) \
struct MANAGER_MESSAGE_OBJECT_CHANGE(object_type) \
/*************************************************************************//** \
 * Internal record of a single managed object change. \
 */ \
{ \
	struct object_type *object; \
	int change; /* bits set from enum MANAGER_CHANGE(object_type) */ \
	void *detail; /* optional detail of subobject changes */ \
}; \
\
struct MANAGER_MESSAGE(object_type) \
/*************************************************************************//** \
 * A message that will be sent when one of more of the objects being managed \
 * has changed. \
 */ \
{ \
	int change_summary; /* bitwise OR of all object changes */ \
	int number_of_changed_objects; /* size of following array */ \
	struct MANAGER_MESSAGE_OBJECT_CHANGE(object_type) *object_changes; \
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
DECLARE_MANAGER_TYPE(object_type) \
/*************************************************************************//** \
 * The structure for the manager. \
 */ \
{ \
	/* the list of objects in the manager */ \
	struct LIST(object_type) *object_list; \
	/* the list of callbacks invoked when manager has changed */ \
	struct MANAGER_CALLBACK_ITEM(object_type) *callback_list; \
	int locked; \
	/* the list of objects changed since the last update message */ \
	struct LIST(object_type) *changed_object_list; \
	/* pointer to owning object which exists for lifetime of this manager, if any */ \
	owner_type *owner; \
	/* flag indicating whether caching is on */ \
	int cache; \
}; /* struct MANAGER(object_type) */

#define FULL_DECLARE_MANAGER_TYPE( object_type ) \
FULL_DECLARE_MANAGER_TYPE_WITH_OWNER(object_type, void)

/*
Local functions
---------------
*/

/*************************************************************************//**
 * Redefine following before calling DECLARE_LOCAL_MANAGER_FUNCTIONS to mark
 * objects dependent on changed objects as having a dependency change, e.g.:
 * #undef MANAGER_UPDATE_DEPENDENCIES
 * #define MANAGER_UPDATE_DEPENDENCIES( Computed_field , manager ) \
 *   Computed_field_manager_update_dependencies(manager);
 * (Only needed for certain objects.)
 */
#define MANAGER_UPDATE_DEPENDENCIES( object_type, manager )

#define MANAGER_UPDATE( object_type )  manager_update_ ## object_type

#define DECLARE_MANAGER_UPDATE_FUNCTION( object_type ) \
static void MANAGER_UPDATE(object_type)(struct MANAGER(object_type) *manager) \
/*************************************************************************//** \
 * Send a manager message listing all changes to date of the <change> type in \
 * the <changed_object_list> of <manager> to registered clients. \
 * Once <change> and <changed_object_list> are put in the <message>, they are \
 * cleared from the manager, then the message is sent. \
 * Copies the message, leaving the message in the manager blank before sending \
 * since it is sometimes possible to have messages sent while others are out. \
 */ \
{ \
	ENTER(MANAGER_UPDATE(object_type)); \
	if (manager) \
	{ \
		if (0 < NUMBER_IN_LIST(object_type)(manager->changed_object_list)) \
		{ \
			int i, number_of_changed_objects; \
			struct MANAGER_CALLBACK_ITEM(object_type) *item; \
			struct MANAGER_MESSAGE(object_type) message; \
			struct MANAGER_MESSAGE_OBJECT_CHANGE(object_type) *object_change; \
			\
			/* Optional stage: added objects change by dependency */ \
			MANAGER_UPDATE_DEPENDENCIES(object_type, manager) \
			/* prepare message, transferring change details from objects to it */ \
			number_of_changed_objects = \
				NUMBER_IN_LIST(object_type)(manager->changed_object_list); \
			if (ALLOCATE(message.object_changes, \
				struct MANAGER_MESSAGE_OBJECT_CHANGE(object_type), \
					number_of_changed_objects)) \
			{ \
				message.change_summary = MANAGER_CHANGE_NONE(object_type); \
				message.number_of_changed_objects = number_of_changed_objects; \
				object_change = message.object_changes; \
				for (i = 0; i < number_of_changed_objects; i++) \
				{ \
					object_change->object = ACCESS(object_type)( \
						FIRST_OBJECT_IN_LIST_THAT(object_type)( \
							(LIST_CONDITIONAL_FUNCTION(object_type) *)NULL, (void *)NULL, \
							manager->changed_object_list)); \
					object_change->change = object_change->object->manager_change_status; \
					object_change->object->manager_change_status = \
						MANAGER_CHANGE_NONE(object_type); \
					REMOVE_OBJECT_FROM_LIST(object_type)(object_change->object, \
						manager->changed_object_list); \
					object_change->detail = NULL; \
					message.change_summary |= object_change->change; \
					object_change++; \
				} \
				/* send message to clients */ \
				item = manager->callback_list; \
				while (item) \
				{ \
					(item->callback)(&message, item->user_data); \
					item = item->next; \
				} \
				/* clean up message */ \
				object_change = message.object_changes; \
				for (i = 0; i < number_of_changed_objects; i++) \
				{ \
					DEACCESS(object_type)(&(object_change->object)); \
					object_change++; \
				} \
				DEALLOCATE(message.object_changes); \
			} \
			else \
			{ \
				display_message(ERROR_MESSAGE, \
					"MANAGER_UPDATE(" #object_type ").  Could not build message"); \
			} \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"MANAGER_UPDATE(" #object_type ").  Invalid argument(s)"); \
	} \
	LEAVE; \
} /* MANAGER_UPDATE(object_type) */

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
		manager->changed_object_list = CREATE_LIST(object_type)(); \
		if (manager->object_list && manager->changed_object_list) \
		{ \
			manager->callback_list = \
				(struct MANAGER_CALLBACK_ITEM(object_type) *)NULL; \
			manager->locked = 0; \
			manager->cache = 0; \
		} \
		else \
		{ \
			display_message(ERROR_MESSAGE, \
				"MANAGER_CREATE(" #object_type ").  Could not create object lists"); \
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
					if (object->manager_change_status == MANAGER_CHANGE_ADD(object_type)) \
					{ \
						/* send no change message for objects added and removed while caching */ \
						object->manager_change_status = MANAGER_CHANGE_NONE(object_type); \
						REMOVE_OBJECT_FROM_LIST(object_type)(object, manager->changed_object_list); \
					} \
					else \
					{ \
						if (object->manager_change_status == MANAGER_CHANGE_NONE(object_type)) \
						{ \
							/* changed_object_list ACCESSes removed objects until message sent */ \
							ADD_OBJECT_TO_LIST(object_type)(object, manager->changed_object_list); \
						} \
						object->manager_change_status = MANAGER_CHANGE_REMOVE(object_type); \
					} \
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
								MANAGER_CHANGE_OBJECT(object_type)); \
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
the manager's object_list and changed_object_list. \
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

#define DECLARE_MANAGER_MESSAGE_GET_CHANGE_SUMMARY_FUNCTION( object_type ) \
PROTOTYPE_MANAGER_MESSAGE_GET_CHANGE_SUMMARY_FUNCTION(object_type) \
{ \
	if (message) \
	{ \
		return (message->change_summary); \
	} \
	return (MANAGER_CHANGE_NONE(object_type)); \
}

#define DECLARE_MANAGER_MESSAGE_GET_OBJECT_CHANGE_FUNCTION( object_type ) \
PROTOTYPE_MANAGER_MESSAGE_GET_OBJECT_CHANGE_FUNCTION(object_type) \
{ \
	if (message && object) \
	{ \
		int i; \
		struct MANAGER_MESSAGE_OBJECT_CHANGE(object_type) *object_change; \
		\
		object_change = message->object_changes; \
		for (i = message->number_of_changed_objects; 0 < i; i--) \
		{ \
			if (object == object_change->object) \
			{ \
				return (object_change->change); \
			} \
			object_change++; \
		} \
	} \
	return (MANAGER_CHANGE_NONE(object_type)); \
}

#define DECLARE_MANAGER_MESSAGE_GET_CHANGE_LIST_FUNCTION( object_type ) \
PROTOTYPE_MANAGER_MESSAGE_GET_CHANGE_LIST_FUNCTION(object_type) \
{ \
	int i; \
	struct LIST(object_type) *object_list; \
	struct MANAGER_MESSAGE_OBJECT_CHANGE(object_type) *object_change; \
	\
	object_list = (struct LIST(object_type) *)NULL; \
	if (message) \
	{ \
		if (message->change_summary & change_mask) \
		{ \
			object_list = CREATE(LIST(object_type))(); \
			object_change = message->object_changes; \
			for (i = message->number_of_changed_objects; 0 < i; i--) \
			{ \
				if (object_change->change & change_mask) \
				{ \
					ADD_OBJECT_TO_LIST(object_type)(object_change->object, object_list); \
				} \
				object_change++; \
			} \
		} \
	} \
	return (object_list); \
}

#define DECLARE_MANAGER_MESSAGE_HAS_CHANGED_OBJECT_THAT_FUNCTION( object_type ) \
PROTOTYPE_MANAGER_MESSAGE_HAS_CHANGED_OBJECT_THAT_FUNCTION(object_type) \
{ \
	int i, return_code; \
	struct MANAGER_MESSAGE_OBJECT_CHANGE(object_type) *object_change; \
	\
	return_code = 0; \
	if (message && conditional_function) \
	{ \
		object_change = message->object_changes; \
		for (i = message->number_of_changed_objects; 0 < i; i--) \
		{ \
			if ((object_change->change & change_mask) && \
				(conditional_function)(object_change->object, user_data)) \
			{ \
				return_code = 1; \
				break; \
			} \
			object_change++; \
		} \
	} \
	return (return_code); \
}

#define MANAGER_GET_OWNER(object_type)  manager_get_owner_ ## object_type

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

#define MANAGER_SET_OWNER(object_type)  manager_set_owner_ ## object_type

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
DECLARE_MANAGER_UPDATE_FUNCTION(object_type) \
DECLARE_MANAGER_FIND_CLIENT_FUNCTION(object_type) \
DECLARE_MANAGED_OBJECT_NOT_IN_USE_CONDITIONAL_FUNCTION(object_type)

/* NOTE: MUST clear the objects pointer to manager in its CREATE function! */
#define DECLARE_MANAGER_FUNCTIONS( object_type , object_manager ) \
DECLARE_CREATE_MANAGER_FUNCTION(object_type) \
DECLARE_OBJECT_CLEAR_MANAGER_FUNCTION(object_type,object_manager) \
DECLARE_DESTROY_MANAGER_FUNCTION(object_type,object_manager) \
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
