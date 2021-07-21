/*******************************************************************************
FILE : manager.h

LAST MODIFIED : 5 March 2002

DESCRIPTION :
Managers oversee the creation, deletion and modification of global objects -
such as object_types, lights and cameras.  All registered clients of the manager
are sent a message when an object has changed.

This file contains macros for (forward) declaring global manager types and
prototyping global manager functions.  Full declarations are not given, so as to
hide the internal workings and allow them to be changed without affecting other
parts of the program.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (MANAGER_H)
#define MANAGER_H

#include "general/list.h"

/*
Macros
======
*/

/*
Global types
------------
*/
#define MANAGER_( object_type )  manager_ ## object_type
#define MANAGER( object_type )  MANAGER_(object_type)

#define DECLARE_MANAGER_TYPE( object_type ) \
struct MANAGER(object_type)

#define MANAGER_CHANGE_( object_type )  manager_change_ ## object_type
#define MANAGER_CHANGE( object_type )  MANAGER_CHANGE_(object_type)

#define MANAGER_CHANGE_NONE_( object_type )  manager_change_none_ ## object_type
#define MANAGER_CHANGE_NONE( object_type )  MANAGER_CHANGE_NONE_(object_type)

#define MANAGER_CHANGE_ADD_( object_type )  manager_change_add_ ## object_type
#define MANAGER_CHANGE_ADD( object_type )  MANAGER_CHANGE_ADD_(object_type)

#define MANAGER_CHANGE_REMOVE_( object_type ) \
	manager_change_remove_ ## object_type
#define MANAGER_CHANGE_REMOVE( object_type ) \
	MANAGER_CHANGE_REMOVE_(object_type)

#define MANAGER_CHANGE_IDENTIFIER_( object_type ) \
	manager_change_identifier_ ## object_type
#define MANAGER_CHANGE_IDENTIFIER( object_type ) \
	MANAGER_CHANGE_IDENTIFIER_(object_type)

#define MANAGER_CHANGE_DEFINITION_( object_type ) \
	manager_change_definition_ ## object_type
#define MANAGER_CHANGE_DEFINITION( object_type ) \
	MANAGER_CHANGE_DEFINITION_(object_type)

#define MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER_( object_type ) \
	manager_change_object_not_identifier_ ## object_type
#define MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER( object_type ) \
	MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER_(object_type)

#define MANAGER_CHANGE_FULL_RESULT_( object_type ) \
	manager_change_full_result_ ## object_type
#define MANAGER_CHANGE_FULL_RESULT( object_type ) \
	MANAGER_CHANGE_FULL_RESULT_(object_type)

#define MANAGER_CHANGE_PARTIAL_RESULT_( object_type ) \
	manager_change_partial_result_ ## object_type
#define MANAGER_CHANGE_PARTIAL_RESULT( object_type ) \
	MANAGER_CHANGE_PARTIAL_RESULT_(object_type)

#define MANAGER_CHANGE_RESULT_( object_type ) \
	manager_change_result_ ## object_type
#define MANAGER_CHANGE_RESULT( object_type ) \
	MANAGER_CHANGE_RESULT_(object_type)

#define DECLARE_MANAGER_CHANGE_TYPE( object_type ) \
enum MANAGER_CHANGE(object_type) \
/*************************************************************************//** \
 * Bit values of object changes sent in MANAGER_MESSAGEs to clients. \
 * Each object change can be either ADD, REMOVE or bitwise OR of other flags. \
 * Message change summary is a bitwise OR of all individual object changes. \
 */ \
{ \
	MANAGER_CHANGE_NONE(object_type) = 0, \
	/*!< object not changed */ \
	\
	MANAGER_CHANGE_ADD(object_type) = 1, \
	/*!< object added to the manager */ \
	\
	MANAGER_CHANGE_REMOVE(object_type) = 2, \
	/*!< object removed from the manager */ \
	\
	MANAGER_CHANGE_IDENTIFIER(object_type) = 4, \
	/*!< object identifier changed in the manager */ \
	\
	MANAGER_CHANGE_DEFINITION(object_type) = 8, \
	/*!< object contents but not identifier changed in the manager */ \
	\
	MANAGER_CHANGE_FULL_RESULT(object_type) = 16, \
	/*!< full change to the resulting output of the object */ \
	\
	MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER(object_type) = ( \
		MANAGER_CHANGE_DEFINITION(object_type) | \
		MANAGER_CHANGE_FULL_RESULT(object_type)), \
	/*!< object contents but not identifier changed in the manager, affecting result */ \
	\
	MANAGER_CHANGE_PARTIAL_RESULT(object_type) = 32, \
	/*!< object is a dependency of another object that has changed */ \
	\
	MANAGER_CHANGE_RESULT(object_type) = ( \
		MANAGER_CHANGE_FULL_RESULT(object_type) | \
		MANAGER_CHANGE_PARTIAL_RESULT(object_type)), \
	/*!< bitwise OR of object change flags affecting result */ \
}

#define MANAGER_MESSAGE_( object_type )  manager_message_ ## object_type
#define MANAGER_MESSAGE( object_type )  MANAGER_MESSAGE_(object_type)

/*************************************************************************//**
 * Object sent with manager messages, detailing changes to managed objects.
 */
#define DECLARE_MANAGER_MESSAGE_TYPE( object_type ) \
struct MANAGER_MESSAGE(object_type)

#define MANAGER_CALLBACK_FUNCTION_( object_type ) \
	manager_callback_function_ ## object_type
#define MANAGER_CALLBACK_FUNCTION( object_type ) \
	MANAGER_CALLBACK_FUNCTION_(object_type)

#define DECLARE_MANAGER_CALLBACK_FUNCTION( object_type ) \
typedef void (MANAGER_CALLBACK_FUNCTION(object_type)) \
	(struct MANAGER_MESSAGE(object_type) *message,void *user_data)

#define MANAGER_CONDITIONAL_FUNCTION_( object_type ) \
	manager_conditional_function_ ## object_type
#define MANAGER_CONDITIONAL_FUNCTION( object_type ) \
	MANAGER_CONDITIONAL_FUNCTION_(object_type)

#define DECLARE_MANAGER_CONDITIONAL_FUNCTION( object_type ) \
typedef int (MANAGER_CONDITIONAL_FUNCTION(object_type)) \
	(struct object_type *object,void *user_data)

#define MANAGER_ITERATOR_FUNCTION_( object_type ) \
	manager_iterator_function_ ## object_type
#define MANAGER_ITERATOR_FUNCTION( object_type ) \
	MANAGER_ITERATOR_FUNCTION_(object_type)

#define DECLARE_MANAGER_ITERATOR_FUNCTION( object_type ) \
typedef int (MANAGER_ITERATOR_FUNCTION(object_type)) \
	(struct object_type *object,void *user_data)

/*
Global functions
----------------
*/
#define CREATE_MANAGER( object_type )  CREATE(MANAGER(object_type))

#define PROTOTYPE_CREATE_MANAGER_FUNCTION( object_type ) \
struct MANAGER(object_type) *CREATE_MANAGER(object_type)(void) \
/***************************************************************************** \
LAST MODIFIED : 27 September 1995 \
\
DESCRIPTION : \
Creates and returns a new manager. \
==============================================================================*/

#define DESTROY_MANAGER( object_type )  DESTROY(MANAGER(object_type))

#define PROTOTYPE_DESTROY_MANAGER_FUNCTION( object_type ) \
int DESTROY_MANAGER(object_type)( \
	struct MANAGER(object_type) **manager_address) \
/***************************************************************************** \
LAST MODIFIED : 27 September 1995 \
\
DESCRIPTION : \
Destroys the <**manager> and sets <*manager> to NULL. \
==============================================================================*/

#define MANAGER_CREATE_LIST_( object_type ) \
	manager_create_list_ ## object_type
#define MANAGER_CREATE_LIST( object_type ) \
	MANAGER_CREATE_LIST_(object_type)

#define PROTOTYPE_MANAGER_CREATE_LIST_FUNCTION( object_type ) \
struct LIST(object_type) *MANAGER_CREATE_LIST(object_type)( \
	struct MANAGER(object_type) *manager) \
/***************************************************************************** \
Creates an empty list for adding objects from this manager. This list is \
related to the manager's own list meaning it is updated automatically when \
object identifiers change in the manager. \
==============================================================================*/

#define REMOVE_OBJECT_FROM_MANAGER_( object_type ) \
	manager_remove_object_ ## object_type
#define REMOVE_OBJECT_FROM_MANAGER( object_type ) \
	REMOVE_OBJECT_FROM_MANAGER_(object_type)

#define PROTOTYPE_REMOVE_OBJECT_FROM_MANAGER_FUNCTION( object_type ) \
int REMOVE_OBJECT_FROM_MANAGER(object_type)(struct object_type *object, \
	struct MANAGER(object_type) *manager) \
/***************************************************************************** \
LAST MODIFIED : 28 September 1995 \
\
DESCRIPTION : \
Deletes an <object> from the <manager>. If the <manager> is locked or the \
operation cannot be performed, it returns zero otherwise a non-zero is \
returned. \
==============================================================================*/

#define REMOVE_ALL_OBJECTS_FROM_MANAGER_( object_type ) \
	manager_remove_all_objects_ ## object_type
#define REMOVE_ALL_OBJECTS_FROM_MANAGER( object_type ) \
	REMOVE_ALL_OBJECTS_FROM_MANAGER_(object_type)

#define PROTOTYPE_REMOVE_ALL_OBJECTS_FROM_MANAGER_FUNCTION( object_type ) \
int REMOVE_ALL_OBJECTS_FROM_MANAGER(object_type)( \
	struct MANAGER(object_type) *manager) \
/***************************************************************************** \
LAST MODIFIED : 16 September 1996 \
\
DESCRIPTION : \
Removes all the objects from the <manager>. If the <manager> is locked or the \
operation cannot be performed, it returns zero otherwise a non-zero is \
returned. \
==============================================================================*/

#define ADD_OBJECT_TO_MANAGER_( object_type ) \
	manager_add_object_ ## object_type
#define ADD_OBJECT_TO_MANAGER( object_type ) \
	ADD_OBJECT_TO_MANAGER_(object_type)

#define PROTOTYPE_ADD_OBJECT_TO_MANAGER_FUNCTION( object_type ) \
int ADD_OBJECT_TO_MANAGER(object_type)(struct object_type *object, \
	struct MANAGER(object_type) *manager) \
/***************************************************************************** \
LAST MODIFIED : 28 September 1995 \
\
DESCRIPTION : \
Adds a <object> to the <manager>. If the <manager> is locked or the \
operation cannot be performed, it returns zero, otherwise a non-zero is \
returned. \
==============================================================================*/

#define NUMBER_IN_MANAGER_( object_type )  manager_number_ ## object_type
#define NUMBER_IN_MANAGER( object_type )  NUMBER_IN_MANAGER_(object_type)

#define PROTOTYPE_NUMBER_IN_MANAGER_FUNCTION( object_type ) \
int NUMBER_IN_MANAGER(object_type)(struct MANAGER(object_type) *manager) \
/***************************************************************************** \
LAST MODIFIED : 21 March 1996 \
\
DESCRIPTION : \
Returns the number of items in the <manager>. \
==============================================================================*/

#define MANAGER_MODIFY_( object_type , identifier_field_name ) \
	manager_modify_ ## object_type ## identifier_field_name
#define MANAGER_MODIFY( object_type , identifier_field_name ) \
	MANAGER_MODIFY_(object_type,identifier_field_name)

#define PROTOTYPE_MANAGER_MODIFY_FUNCTION( object_type , \
	identifier_field_name ) \
int MANAGER_MODIFY(object_type,identifier_field_name)( \
	struct object_type *object, struct object_type *new_data, \
	struct MANAGER(object_type) *manager) \
/***************************************************************************** \
LAST MODIFIED : 18 May 2001 \
\
DESCRIPTION : \
Copies the <new_data> to the <object>, including the new identifier.  If the \
<manager> is locked or the operation cannot be performed, it returns zero \
otherwise it returns a non-zero. \
==============================================================================*/

#define MANAGER_MODIFY_NOT_IDENTIFIER_( object_type , identifier_field_name ) \
	manager_modify_not_identifier_ ## object_type ## identifier_field_name
#define MANAGER_MODIFY_NOT_IDENTIFIER( object_type , identifier_field_name ) \
	MANAGER_MODIFY_NOT_IDENTIFIER_(object_type,identifier_field_name)

#define PROTOTYPE_MANAGER_MODIFY_NOT_IDENTIFIER_FUNCTION( object_type , \
	identifier_field_name ) \
int MANAGER_MODIFY_NOT_IDENTIFIER(object_type,identifier_field_name)( \
	struct object_type *object,struct object_type *new_data, \
	struct MANAGER(object_type) *manager) \
/***************************************************************************** \
LAST MODIFIED : 28 September 1995 \
\
DESCRIPTION : \
Copies the data from <new_data> to <object>, but does not change the \
identifier.  If the <manager> is locked or the operation cannot be performed, \
it returns zero, otherwise a non-zero is returned. \
==============================================================================*/

#define MANAGER_MODIFY_IDENTIFIER_( object_type , identifier_field_name ) \
	manager_modify_identifier_ ## object_type ## identifier_field_name
#define MANAGER_MODIFY_IDENTIFIER( object_type , identifier_field_name ) \
	MANAGER_MODIFY_IDENTIFIER_(object_type,identifier_field_name)

#define PROTOTYPE_MANAGER_MODIFY_IDENTIFIER_FUNCTION( object_type , \
	identifier_field_name , identifier_type ) \
int MANAGER_MODIFY_IDENTIFIER(object_type,identifier_field_name)( \
	struct object_type *object,identifier_type new_identifier, \
	struct MANAGER(object_type) *manager) \
/***************************************************************************** \
LAST MODIFIED : 28 September 1995 \
\
DESCRIPTION : \
Changes the identifier of the <object>  to the <new_identifier>.  If the \
<manager> is locked or the operation cannot be performed, it returns zero, \
otherwise a non-zero is returned. \
==============================================================================*/

#define MANAGER_REGISTER_( object_type )  manager_register_ ## object_type
#define MANAGER_REGISTER( object_type )  MANAGER_REGISTER_(object_type)

#define PROTOTYPE_MANAGER_REGISTER_FUNCTION( object_type ) \
void *MANAGER_REGISTER(object_type)( \
	MANAGER_CALLBACK_FUNCTION(object_type) *callback,void *user_data, \
	struct MANAGER(object_type) *manager) \
/***************************************************************************** \
LAST MODIFIED : 28 September 1995 \
\
DESCRIPTION : \
Registers a <callback> and <user_data> to be called when objects in the \
<manager> are changed.  It returns the id to be used when the callback is \
deregistered. \
==============================================================================*/

#define MANAGER_DEREGISTER_( object_type )  manager_deregister_ ## object_type
#define MANAGER_DEREGISTER( object_type )  MANAGER_DEREGISTER_(object_type)

#define PROTOTYPE_MANAGER_DEREGISTER_FUNCTION( object_type ) \
int MANAGER_DEREGISTER(object_type)(void *callback_id, \
	struct MANAGER(object_type) *manager) \
/***************************************************************************** \
LAST MODIFIED : 28 September 1995 \
\
DESCRIPTION : \
Deregisters the callback with the specified <callback_id> from the <manager>. \
==============================================================================*/

#define IS_MANAGED_( object_type )  is_managed_ ## object_type
#define IS_MANAGED( object_type )  IS_MANAGED_(object_type)

#define PROTOTYPE_IS_MANAGED_FUNCTION( object_type ) \
int IS_MANAGED(object_type)(struct object_type *object, \
	struct MANAGER(object_type) *manager) \
/***************************************************************************** \
LAST MODIFIED : 28 September 1995 \
\
DESCRIPTION : \
Returns a non-zero if the <object> is managed by the <manager>. \
==============================================================================*/

#define MANAGED_OBJECT_NOT_IN_USE_( object_type ) \
	managed_object_not_in_use_ ## object_type
#define MANAGED_OBJECT_NOT_IN_USE( object_type ) \
	MANAGED_OBJECT_NOT_IN_USE_(object_type)

#define PROTOTYPE_MANAGED_OBJECT_NOT_IN_USE_FUNCTION( object_type ) \
int MANAGED_OBJECT_NOT_IN_USE(object_type)(struct object_type *object, \
	struct MANAGER(object_type) *manager) \
/***************************************************************************** \
LAST MODIFIED : 18 January 2002 \
\
DESCRIPTION : \
Returns true if <object> is only accessed by the manager or other managed \
objects. In general, a true result is sufficient to indicate the object may be \
removed from the manager or modified. \
==============================================================================*/

#define FIND_BY_IDENTIFIER_IN_MANAGER_( object_type , identifier ) \
	manager_find_by_identifier_ ## object_type ## identifier
#define FIND_BY_IDENTIFIER_IN_MANAGER( object_type , identifier ) \
	FIND_BY_IDENTIFIER_IN_MANAGER_(object_type,identifier)

#define PROTOTYPE_FIND_BY_IDENTIFIER_IN_MANAGER_FUNCTION( object_type , \
	identifier , identifier_type ) \
struct object_type *FIND_BY_IDENTIFIER_IN_MANAGER(object_type,identifier)( \
	identifier_type identifier,struct MANAGER(object_type) *manager) \
/***************************************************************************** \
LAST MODIFIED : 28 September 1995 \
\
DESCRIPTION : \
Returns the object with the specified <identifier> from the <manager>. \
==============================================================================*/

#define FIRST_OBJECT_IN_MANAGER_THAT_( object_type ) \
	manager_first_that_ ## object_type
#define FIRST_OBJECT_IN_MANAGER_THAT( object_type ) \
	FIRST_OBJECT_IN_MANAGER_THAT_(object_type)

#define PROTOTYPE_FIRST_OBJECT_IN_MANAGER_THAT_FUNCTION( object_type ) \
struct object_type *FIRST_OBJECT_IN_MANAGER_THAT(object_type)( \
	MANAGER_CONDITIONAL_FUNCTION(object_type) *conditional,void *user_data, \
	struct MANAGER(object_type) *manager) \
/***************************************************************************** \
LAST MODIFIED : 28 September 1995 \
\
DESCRIPTION : \
If <conditional> is not NULL, the "first" object in the <manager> that \
<conditional> returns true is returned, otherwise the "first" object in the \
<list> is returned. \
==============================================================================*/

#define FOR_EACH_OBJECT_IN_MANAGER_( object_type ) \
	manager_for_each_ ## object_type
#define FOR_EACH_OBJECT_IN_MANAGER( object_type ) \
	FOR_EACH_OBJECT_IN_MANAGER_(object_type)

#define PROTOTYPE_FOR_EACH_OBJECT_IN_MANAGER_FUNCTION( object_type ) \
int FOR_EACH_OBJECT_IN_MANAGER(object_type)( \
	MANAGER_ITERATOR_FUNCTION(object_type) *iterator,void *user_data, \
	struct MANAGER(object_type) *manager) \
/***************************************************************************** \
LAST MODIFIED : 28 September 1995 \
\
DESCRIPTION : \
Calls <iterator> for each object  being managed by the <manager>. \
==============================================================================*/

#define MANAGED_OBJECT_CHANGE_( object_type )  managed_object_change_ ## object_type
#define MANAGED_OBJECT_CHANGE( object_type )  MANAGED_OBJECT_CHANGE_(object_type)
 
#define PROTOTYPE_MANAGED_OBJECT_CHANGE_FUNCTION( object_type ) \
int MANAGED_OBJECT_CHANGE(object_type)(struct object_type *object, \
	int change) \
/*************************************************************************//** \
 * Call after changing a managed object to record the change. \
 * If change cache is off, sends manager message to clients. \
 * Does nothing if the object is not managed. \
 * Do not use to set ADD or REMOVE changes. \
 * @param change  Logical OR of enum MANAGER_CHANGE(object_type) values.
 */

#define MANAGER_BEGIN_CACHE_( object_type )  manager_begin_cache_ ## object_type
#define MANAGER_BEGIN_CACHE( object_type )  MANAGER_BEGIN_CACHE_(object_type)

#define PROTOTYPE_MANAGER_BEGIN_CACHE_FUNCTION( object_type ) \
int MANAGER_BEGIN_CACHE(object_type)(struct MANAGER(object_type) *manager) \
/***************************************************************************** \
LAST MODIFIED : 28 September 1995 \
\
DESCRIPTION : \
Supresses updates from this point. \
==============================================================================*/

#define MANAGER_END_CACHE_( object_type )  manager_end_cache_ ## object_type
#define MANAGER_END_CACHE( object_type )  MANAGER_END_CACHE_(object_type)

#define PROTOTYPE_MANAGER_END_CACHE_FUNCTION( object_type ) \
int MANAGER_END_CACHE(object_type)(struct MANAGER(object_type) *manager) \
/***************************************************************************** \
LAST MODIFIED : 28 September 1995 \
\
DESCRIPTION : \
Performs a global update. \
==============================================================================*/

#define MANAGER_EXTERNAL_CHANGE_( object_type ) manager_force_update ## object_type
#define MANAGER_EXTERNAL_CHANGE( object_type ) MANAGER_EXTERNAL_CHANGE_(object_type)

/**
 * Forces a global update when change cache is [next] zero so dependency checks
 * on external objects can be made by individual managed objects.
 */
#define PROTOTYPE_MANAGER_EXTERNAL_CHANGE_FUNCTION( object_type ) \
void MANAGER_EXTERNAL_CHANGE(object_type)(struct MANAGER(object_type) *manager)

#define MANAGER_MESSAGE_ACCESS_( object_type ) manager_message_access_ ## object_type
#define MANAGER_MESSAGE_ACCESS( object_type ) MANAGER_MESSAGE_ACCESS_(object_type)

/** Increment reference count on the message */
#define PROTOTYPE_MANAGER_MESSAGE_ACCESS_FUNCTION( object_type ) \
struct MANAGER_MESSAGE(object_type) *MANAGER_MESSAGE_ACCESS(object_type)( \
	struct MANAGER_MESSAGE(object_type) *message) \

#define MANAGER_MESSAGE_DEACCESS_( object_type ) manager_message_deaccess_ ## object_type
#define MANAGER_MESSAGE_DEACCESS( object_type ) MANAGER_MESSAGE_DEACCESS_(object_type)

/** Decrement reference count on the message, set to NULL */
#define PROTOTYPE_MANAGER_MESSAGE_DEACCESS_FUNCTION( object_type ) \
void MANAGER_MESSAGE_DEACCESS(object_type)( \
	struct MANAGER_MESSAGE(object_type) **message_address) \

#define MANAGER_MESSAGE_GET_CHANGE_SUMMARY_( object_type ) \
	manager_message_get_change_summary_ ## object_type
#define MANAGER_MESSAGE_GET_CHANGE_SUMMARY( object_type ) \
	MANAGER_MESSAGE_GET_CHANGE_SUMMARY_(object_type)

#define PROTOTYPE_MANAGER_MESSAGE_GET_CHANGE_SUMMARY_FUNCTION( object_type ) \
int MANAGER_MESSAGE_GET_CHANGE_SUMMARY(object_type)( \
	struct MANAGER_MESSAGE(object_type) *message) \
/*************************************************************************//** \
 * Gets a summary of all the changes to objects in the message, a bitwise OR \
 * of all individual object changes. \
 */

#define MANAGER_MESSAGE_GET_OBJECT_CHANGE_( object_type ) \
	manager_message_get_object_change_ ## object_type
#define MANAGER_MESSAGE_GET_OBJECT_CHANGE( object_type ) \
	MANAGER_MESSAGE_GET_OBJECT_CHANGE_(object_type)

#define PROTOTYPE_MANAGER_MESSAGE_GET_OBJECT_CHANGE_FUNCTION( object_type ) \
int MANAGER_MESSAGE_GET_OBJECT_CHANGE(object_type)( \
	struct MANAGER_MESSAGE(object_type) *message, struct object_type *object) \
/*************************************************************************//** \
 * Gets the change flags for the object from message. \
 */

#define MANAGER_MESSAGE_GET_CHANGE_LIST_( object_type ) \
	manager_message_get_change_list_ ## object_type
#define MANAGER_MESSAGE_GET_CHANGE_LIST( object_type ) \
	MANAGER_MESSAGE_GET_CHANGE_LIST_(object_type)

#define PROTOTYPE_MANAGER_MESSAGE_GET_CHANGE_LIST_FUNCTION( object_type ) \
struct LIST(object_type) *MANAGER_MESSAGE_GET_CHANGE_LIST(object_type)( \
	struct MANAGER_MESSAGE(object_type) *message, int change_mask) \
/*************************************************************************//** \
 * Creates and returns a list of objects with any change flags from the \
 * change_mask set. No need to check change_summary against change_mask first \
 * as this is done internally to return NULL if no changes. \
 * Up to caller to destroy any returned list.
 * @return  New list of objects with changes from change mask or NULL if none. \
 */

#define MANAGER_MESSAGE_HAS_CHANGED_OBJECT_THAT_( object_type ) \
	manager_message_has_changed_object_that_ ## object_type
#define MANAGER_MESSAGE_HAS_CHANGED_OBJECT_THAT( object_type ) \
	MANAGER_MESSAGE_HAS_CHANGED_OBJECT_THAT_(object_type)

#define PROTOTYPE_MANAGER_MESSAGE_HAS_CHANGED_OBJECT_THAT_FUNCTION( object_type ) \
int MANAGER_MESSAGE_HAS_CHANGED_OBJECT_THAT(object_type)( \
	struct MANAGER_MESSAGE(object_type) *message, int change_mask, \
	MANAGER_CONDITIONAL_FUNCTION(object_type) *conditional_function, \
	void *user_data) \
/*************************************************************************//** \
 * Returns true if there is a changed object listed in the message which \
 * matches any change flags in the change_mask and returns true for the \
 * supplied conditional function with user_data. \
 */

#define MANAGER_COPY_WITH_IDENTIFIER_( object_type , identifier_field_name ) \
	manager_copy_with_identifier_ ## object_type ## identifier_field_name
#define MANAGER_COPY_WITH_IDENTIFIER( object_type , identifier_field_name ) \
	MANAGER_COPY_WITH_IDENTIFIER_(object_type,identifier_field_name)

#define PROTOTYPE_MANAGER_COPY_WITH_IDENTIFIER_FUNCTION( object_type , \
	identifier_field_name ) \
int MANAGER_COPY_WITH_IDENTIFIER(object_type,identifier_field_name)( \
	struct object_type *destination,struct object_type *source) \
/***************************************************************************** \
LAST MODIFIED : 30 September 1995 \
\
DESCRIPTION : \
Copies all the contents of <source> to <destination>.  Used by the manager. \
==============================================================================*/

#define MANAGER_COPY_WITHOUT_IDENTIFIER_( object_type , \
	identifier_field_name )  manager_copy_without_identifier_ ## object_type \
	## identifier_field_name
#define MANAGER_COPY_WITHOUT_IDENTIFIER( object_type , identifier_field_name ) \
	MANAGER_COPY_WITHOUT_IDENTIFIER_(object_type,identifier_field_name)

#define PROTOTYPE_MANAGER_COPY_WITHOUT_IDENTIFIER_FUNCTION( object_type , \
	identifier_field_name ) \
int MANAGER_COPY_WITHOUT_IDENTIFIER(object_type,identifier_field_name)( \
	struct object_type *destination,struct object_type *source) \
/***************************************************************************** \
LAST MODIFIED : 30 September 1995 \
\
DESCRIPTION : \
Copies all the contents, except the identifier, of <source> to <destination>. \
Used by the manager. \
==============================================================================*/

#define MANAGER_COPY_IDENTIFIER_( object_type , identifier_field_name ) \
	manager_copy_identifier_ ## object_type ## identifier_field_name
#define MANAGER_COPY_IDENTIFIER( object_type , identifier_field_name ) \
	MANAGER_COPY_IDENTIFIER_(object_type,identifier_field_name)

#define PROTOTYPE_MANAGER_COPY_IDENTIFIER_FUNCTION( object_type , \
	identifier_field_name , identifier_type ) \
int MANAGER_COPY_IDENTIFIER(object_type,identifier_field_name)( \
	struct object_type *destination,identifier_type identifier_field_name) \
/***************************************************************************** \
LAST MODIFIED : 30 September 1995 \
\
DESCRIPTION : \
Copies the <identifier_field_name> to <destination>.  Used by the manager. \
==============================================================================*/

#define MANAGER_CLASS_(object_type) manager_class_ ## object_type
#define MANAGER_CLASS(object_type) MANAGER_CLASS_(object_type)
/***************************************************************************** \
LAST MODIFIED : 9 February 2007 \
\
DESCRIPTION : \
Wraps the existing Manager functionality and types into a class. \
==============================================================================*/
#define DEFINE_MANAGER_CLASS(object_type) \
class MANAGER_CLASS(object_type) \
{\
public: \
	typedef MANAGER(object_type) Manager_type; \
	typedef MANAGER_MESSAGE(object_type) Manager_message_type; \
	typedef MANAGER_CONDITIONAL_FUNCTION(object_type) Manager_conditional_function; \
	typedef LIST_CONDITIONAL_FUNCTION(object_type) List_conditional_function; \
	static const enum MANAGER_CHANGE(object_type) Manager_change_none = MANAGER_CHANGE_NONE(object_type); \
	static const enum MANAGER_CHANGE(object_type) Manager_change_add = MANAGER_CHANGE_ADD(object_type); \
	static const enum MANAGER_CHANGE(object_type) Manager_change_remove = MANAGER_CHANGE_REMOVE(object_type); \
	static const enum MANAGER_CHANGE(object_type) Manager_change_identifier = MANAGER_CHANGE_IDENTIFIER(object_type); \
	static const enum MANAGER_CHANGE(object_type) Manager_change_definition = MANAGER_CHANGE_DEFINITION(object_type); \
	static const enum MANAGER_CHANGE(object_type) Manager_change_object_not_identifier = MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER(object_type); \
	static const enum MANAGER_CHANGE(object_type) Manager_change_full_result = MANAGER_CHANGE_FULL_RESULT(object_type); \
	static const enum MANAGER_CHANGE(object_type) Manager_change_partial_result = MANAGER_CHANGE_PARTIAL_RESULT(object_type); \
	static const enum MANAGER_CHANGE(object_type) Manager_change_result = MANAGER_CHANGE_RESULT(object_type); \
\
	MANAGER(object_type) *manager; \
\
   MANAGER_CLASS(object_type)(MANAGER(object_type) *manager) : manager(manager) \
	{ \
	} \
\
	inline void *register_callback(MANAGER_CALLBACK_FUNCTION(object_type) *callback, \
		void *user_data) \
	{ \
		return MANAGER_REGISTER(object_type)(callback, user_data, manager); \
	} \
\
	inline int deregister_callback(void *callback_id) \
	{ \
		return MANAGER_DEREGISTER(object_type)(callback_id, manager); \
	} \
\
	inline int number_in_manager() \
	{ \
		return NUMBER_IN_MANAGER(object_type)(manager); \
	} \
\
	inline int for_each_object_in_manager(	\
		MANAGER_ITERATOR_FUNCTION(object_type) *iterator, void *user_data) \
	{ \
		return FOR_EACH_OBJECT_IN_MANAGER(object_type)( \
			iterator, user_data, manager); \
	} \
\
	inline object_type *first_object_in_list_that(	\
	  LIST_CONDITIONAL_FUNCTION(object_type) *conditional, void *user_data, \
	  struct LIST(object_type) *list) \
	{ \
		return FIRST_OBJECT_IN_LIST_THAT(object_type)( \
			conditional, user_data, list); \
	} \
\
	inline int get_object_name(object_type *object, char **name) \
	{ \
		return GET_NAME(object_type)(object, name); \
	} \
\
	inline int manager_message_get_change_summary(Manager_message_type *message) \
	{ \
		return MANAGER_MESSAGE_GET_CHANGE_SUMMARY(object_type)(message); \
	} \
\
	inline int manager_message_has_changed_object_that( \
		Manager_message_type *message, int change_mask, \
		Manager_conditional_function *conditional_function, void *user_data) \
	{ \
		return MANAGER_MESSAGE_HAS_CHANGED_OBJECT_THAT(object_type)( \
			message, change_mask, conditional_function, user_data); \
	} \
\
}; /* MANAGER_CLASS(object_type) */

#define DECLARE_MANAGER_TYPES( object_type ) \
DECLARE_MANAGER_CHANGE_TYPE(object_type); \
DECLARE_MANAGER_MESSAGE_TYPE(object_type); \
DECLARE_MANAGER_CALLBACK_FUNCTION(object_type); \
DECLARE_MANAGER_TYPE(object_type); \
DECLARE_MANAGER_CONDITIONAL_FUNCTION(object_type); \
DECLARE_MANAGER_ITERATOR_FUNCTION(object_type)

#define PROTOTYPE_MANAGER_FUNCTIONS( object_type ) \
PROTOTYPE_CREATE_MANAGER_FUNCTION(object_type); \
PROTOTYPE_DESTROY_MANAGER_FUNCTION(object_type); \
PROTOTYPE_MANAGER_CREATE_LIST_FUNCTION(object_type); \
PROTOTYPE_REMOVE_OBJECT_FROM_MANAGER_FUNCTION(object_type); \
PROTOTYPE_REMOVE_ALL_OBJECTS_FROM_MANAGER_FUNCTION(object_type); \
PROTOTYPE_NUMBER_IN_MANAGER_FUNCTION(object_type); \
PROTOTYPE_MANAGER_REGISTER_FUNCTION(object_type); \
PROTOTYPE_MANAGER_DEREGISTER_FUNCTION(object_type); \
PROTOTYPE_IS_MANAGED_FUNCTION(object_type); \
PROTOTYPE_MANAGED_OBJECT_NOT_IN_USE_FUNCTION(object_type); \
PROTOTYPE_FIRST_OBJECT_IN_MANAGER_THAT_FUNCTION(object_type); \
PROTOTYPE_FOR_EACH_OBJECT_IN_MANAGER_FUNCTION(object_type); \
PROTOTYPE_MANAGED_OBJECT_CHANGE_FUNCTION(object_type); \
PROTOTYPE_MANAGER_BEGIN_CACHE_FUNCTION(object_type); \
PROTOTYPE_MANAGER_END_CACHE_FUNCTION(object_type); \
PROTOTYPE_MANAGER_EXTERNAL_CHANGE_FUNCTION(object_type); \
PROTOTYPE_MANAGER_MESSAGE_ACCESS_FUNCTION(object_type); \
PROTOTYPE_MANAGER_MESSAGE_DEACCESS_FUNCTION(object_type); \
PROTOTYPE_MANAGER_MESSAGE_GET_CHANGE_SUMMARY_FUNCTION(object_type); \
PROTOTYPE_MANAGER_MESSAGE_GET_OBJECT_CHANGE_FUNCTION(object_type); \
PROTOTYPE_MANAGER_MESSAGE_GET_CHANGE_LIST_FUNCTION(object_type); \
PROTOTYPE_MANAGER_MESSAGE_HAS_CHANGED_OBJECT_THAT_FUNCTION(object_type)

#define PROTOTYPE_MANAGER_IDENTIFIER_WITHOUT_MODIFY_FUNCTIONS( object_type , \
	identifier_field_name , identifier_type ) \
PROTOTYPE_ADD_OBJECT_TO_MANAGER_FUNCTION(object_type); \
PROTOTYPE_FIND_BY_IDENTIFIER_IN_MANAGER_FUNCTION(object_type, \
	identifier_field_name,identifier_type)

#define PROTOTYPE_MANAGER_IDENTIFIER_FUNCTIONS( object_type , \
	identifier_field_name , identifier_type ) \
PROTOTYPE_MANAGER_IDENTIFIER_WITHOUT_MODIFY_FUNCTIONS(object_type, \
	identifier_field_name,identifier_type); \
PROTOTYPE_MANAGER_MODIFY_FUNCTION(object_type,identifier_field_name); \
PROTOTYPE_MANAGER_MODIFY_NOT_IDENTIFIER_FUNCTION(object_type, \
	identifier_field_name); \
PROTOTYPE_MANAGER_MODIFY_IDENTIFIER_FUNCTION(object_type, \
	identifier_field_name,identifier_type)

#define PROTOTYPE_MANAGER_COPY_FUNCTIONS( object_type , \
	identifier_field_name , identifier_type ) \
PROTOTYPE_MANAGER_COPY_WITHOUT_IDENTIFIER_FUNCTION(object_type, \
	identifier_field_name); \
PROTOTYPE_MANAGER_COPY_IDENTIFIER_FUNCTION(object_type, \
	identifier_field_name,identifier_type); \
PROTOTYPE_MANAGER_COPY_WITH_IDENTIFIER_FUNCTION(object_type, \
	identifier_field_name)

#define DECLARE_MANAGER_TYPES_AND_PROTOTYPE_FUNCTIONS( object_type ) \
DECLARE_MANAGER_TYPES(object_type); \
PROTOTYPE_MANAGER_FUNCTIONS(object_type)
#endif
