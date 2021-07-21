/*******************************************************************************
FILE : list.h

LAST MODIFIED : 25 April 2003

DESCRIPTION :
Macros for forward declaring standard list types and prototyping standard list
functions.  Full declarations are not given, so as to hide the internal workings
and allow them to be changed without affecting other parts of the program.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (LIST_H)
#define LIST_H

#include "general/object.h"

/*
Macros
======
*/

/*
Global types
------------
*/
#if ! defined (SHORT_NAMES)
#define LIST_( object_type )  list_ ## object_type
#else
#define LIST_( object_type )  l ## object_type
#endif
#define LIST( object_type )  LIST_(object_type)

/*???DB.  Should this be typedef'd and be a pointer ? */
#define DECLARE_LIST_TYPE( object_type ) \
struct LIST(object_type)

#if ! defined (SHORT_NAMES)
#define LIST_CONDITIONAL_FUNCTION_( object_type ) \
	list_conditional_function_ ## object_type
#else
#define LIST_CONDITIONAL_FUNCTION_( object_type )  lc ## object_type
#endif
#define LIST_CONDITIONAL_FUNCTION( object_type ) \
	LIST_CONDITIONAL_FUNCTION_(object_type)

#define DECLARE_LIST_CONDITIONAL_FUNCTION( object_type ) \
typedef int (LIST_CONDITIONAL_FUNCTION(object_type)) \
	(struct object_type *object,void *user_data)

#if ! defined (SHORT_NAMES)
#define LIST_ITERATOR_FUNCTION_( object_type ) \
	list_iterator_function_ ## object_type
#else
#define LIST_ITERATOR_FUNCTION_( object_type )  li ## object_type
#endif
#define LIST_ITERATOR_FUNCTION( object_type ) \
	LIST_ITERATOR_FUNCTION_(object_type)

#define DECLARE_LIST_ITERATOR_FUNCTION( object_type ) \
typedef int (LIST_ITERATOR_FUNCTION(object_type)) \
	(struct object_type *object,void *user_data)

/*
Global functions
----------------
*/
/*???DB.  What about ordering ? */
#define CREATE_LIST( object_type )  CREATE(LIST(object_type))

#define PROTOTYPE_CREATE_LIST_FUNCTION( object_type ) \
struct LIST(object_type) *CREATE_LIST(object_type)(void) \
/***************************************************************************** \
LAST MODIFIED : 24 September 1995 \
\
DESCRIPTION : \
Creates a object_type list. \
==============================================================================*/

#define CREATE_RELATED_LIST_( object_type ) list_create_related_ ## object_type
#define CREATE_RELATED_LIST( object_type ) CREATE_RELATED_LIST_(object_type)

#define PROTOTYPE_CREATE_RELATED_LIST_FUNCTION( object_type ) \
struct LIST(object_type) *CREATE_RELATED_LIST(object_type)( \
	struct LIST(object_type) *other_list) \
/***************************************************************************** \
Creates a list of object_type which is to contain related objects to the \
supplied <other_list>. By making lists related, object renaming of indexed \
lists / sets can be safely and efficiently done. \
==============================================================================*/

#define DESTROY_LIST( object_type )  DESTROY(LIST(object_type))

#define PROTOTYPE_DESTROY_LIST_FUNCTION( object_type ) \
int DESTROY_LIST(object_type)(struct LIST(object_type) **list_address) \
/***************************************************************************** \
LAST MODIFIED : 24 September 1995 \
\
DESCRIPTION : \
Destroys the object_type list pointed to by <**list_address> and sets \
<*list_address> to NULL. \
==============================================================================*/

#define COPY_LIST( object_type )  COPY(LIST(object_type))

#define PROTOTYPE_COPY_LIST_FUNCTION( object_type ) \
int COPY_LIST(object_type)(struct LIST(object_type) *target_list, \
	struct LIST(object_type) *source_list) \
/***************************************************************************** \
LAST MODIFIED : 24 September 1995 \
\
DESCRIPTION : \
Replaces the contents of the <target_list> with those of the <source_list>. \
==============================================================================*/

#if ! defined (SHORT_NAMES)
#define REMOVE_OBJECT_FROM_LIST_( object_type ) \
	list_remove_object_ ## object_type
#else
#define REMOVE_OBJECT_FROM_LIST_( object_type )  rl ## object_type
#endif
#define REMOVE_OBJECT_FROM_LIST( object_type ) \
	REMOVE_OBJECT_FROM_LIST_(object_type)

#define PROTOTYPE_REMOVE_OBJECT_FROM_LIST_FUNCTION( object_type ) \
int REMOVE_OBJECT_FROM_LIST(object_type)(struct object_type *object, \
	struct LIST(object_type) *list) \
/***************************************************************************** \
LAST MODIFIED : 24 September 1995 \
\
DESCRIPTION : \
Removes the <object> from the <list>. \
==============================================================================*/

#if ! defined (SHORT_NAMES)
#define REMOVE_ALL_OBJECTS_FROM_LIST_( object_type )  \
	list_remove_all_objects_ ## object_type
#else
#define REMOVE_ALL_OBJECTS_FROM_LIST_( object_type )  ql ## object_type
#endif
#define REMOVE_ALL_OBJECTS_FROM_LIST( object_type ) \
	REMOVE_ALL_OBJECTS_FROM_LIST_(object_type)

#define PROTOTYPE_REMOVE_ALL_OBJECTS_FROM_LIST_FUNCTION( object_type ) \
int REMOVE_ALL_OBJECTS_FROM_LIST(object_type)(struct LIST(object_type) *list) \
/***************************************************************************** \
LAST MODIFIED : 19 February 1996 \
\
DESCRIPTION : \
Removes all objects from the list \
==============================================================================*/

#if ! defined (SHORT_NAMES)
#define REMOVE_OBJECTS_FROM_LIST_THAT_( object_type )  \
	list_remove_objects_that_ ## object_type
#else
#define REMOVE_OBJECTS_FROM_LIST_THAT_( object_type )  sl ## object_type
#endif
#define REMOVE_OBJECTS_FROM_LIST_THAT( object_type ) \
	REMOVE_OBJECTS_FROM_LIST_THAT_(object_type)

#define PROTOTYPE_REMOVE_OBJECTS_FROM_LIST_THAT_FUNCTION( object_type ) \
int REMOVE_OBJECTS_FROM_LIST_THAT(object_type)( \
	LIST_CONDITIONAL_FUNCTION(object_type) *conditional,void *user_data, \
	struct LIST(object_type) *list) \
/***************************************************************************** \
LAST MODIFIED : 14 November 1996 \
\
DESCRIPTION : \
Removes all objects from the list that <conditional> returns true for \
==============================================================================*/

#if ! defined (SHORT_NAMES)
#define ADD_OBJECT_TO_LIST_( object_type )  list_add_object_ ## object_type
#else
#define ADD_OBJECT_TO_LIST_( object_type )  pl ## object_type
#endif
#define ADD_OBJECT_TO_LIST( object_type )  ADD_OBJECT_TO_LIST_(object_type)

#define PROTOTYPE_ADD_OBJECT_TO_LIST_FUNCTION( object_type ) \
int ADD_OBJECT_TO_LIST(object_type)(struct object_type *object, \
	struct LIST(object_type) *list) \
/***************************************************************************** \
LAST MODIFIED : 24 September 1995 \
\
DESCRIPTION : \
Add the <object> to the <list>. \
==============================================================================*/

#if ! defined (SHORT_NAMES)
#define ADD_OBJECT_TO_FRONT_OF_LIST_( object_type )  list_add_object_to_front ## object_type
#else
#define ADD_OBJECT_TO_FRONT_OF_LIST_( object_type )  pl ## object_type
#endif
#define ADD_OBJECT_TO_FRONT_OF_LIST( object_type )  ADD_OBJECT_TO_FRONT_OF_LIST_(object_type)

#define PROTOTYPE_ADD_OBJECT_TO_FRONT_OF_LIST_FUNCTION( object_type ) \
int ADD_OBJECT_TO_FRONT_OF_LIST(object_type)(struct object_type *object, \
	struct LIST(object_type) *list) \
/***************************************************************************** \
LAST MODIFIED : 11 September 2007 \
\
DESCRIPTION : \
Add the <object> to the front of <list>. \
==============================================================================*/

#if ! defined (SHORT_NAMES)
#define NUMBER_IN_LIST_( object_type )  list_number_ ## object_type
#else
#define NUMBER_IN_LIST_( object_type )  nl ## object_type
#endif
#define NUMBER_IN_LIST( object_type )  NUMBER_IN_LIST_(object_type)

#define PROTOTYPE_NUMBER_IN_LIST_FUNCTION( object_type ) \
int NUMBER_IN_LIST(object_type)(struct LIST(object_type) *list) \
/***************************************************************************** \
LAST MODIFIED : 24 September 1995 \
\
DESCRIPTION : \
Returns the number of items in the <list>. \
==============================================================================*/

#if ! defined (SHORT_NAMES)
#define IS_OBJECT_IN_LIST_( object_type )  list_in_list_ ## object_type
#else
#define IS_OBJECT_IN_LIST_( object_type )  il ## object_type
#endif
#define IS_OBJECT_IN_LIST( object_type )  IS_OBJECT_IN_LIST_(object_type)

#define PROTOTYPE_IS_OBJECT_IN_LIST_FUNCTION( object_type ) \
int IS_OBJECT_IN_LIST(object_type)(struct object_type *object, \
	struct LIST(object_type) *list) \
/***************************************************************************** \
LAST MODIFIED : 24 September 1995 \
\
DESCRIPTION : \
Returns true if the <object> is in the <list>. \
???DB.  Is this needed ? \
==============================================================================*/

#if ! defined (SHORT_NAMES)
#define FIND_BY_IDENTIFIER_IN_LIST_( object_type , identifier ) \
	list_find_by_identifier_ ## object_type ## identifier
#else
#define FIND_BY_IDENTIFIER_IN_LIST_( object_type , identifier ) \
	fl ## object_type ## identifier
#endif
#define FIND_BY_IDENTIFIER_IN_LIST( object_type , identifier ) \
	FIND_BY_IDENTIFIER_IN_LIST_(object_type,identifier)

#define PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION( object_type , \
	identifier, identifier_type ) \
struct object_type *FIND_BY_IDENTIFIER_IN_LIST(object_type,identifier)( \
	identifier_type identifier,struct LIST(object_type) *list) \
/***************************************************************************** \
LAST MODIFIED : 24 September 1995 \
\
DESCRIPTION : \
Searchs the <list> for the object with the specified <identifier> and returns \
the address of the object. \
==============================================================================*/

#if ! defined (SHORT_NAMES)
#define FIRST_OBJECT_IN_LIST_THAT_( object_type ) \
	list_first_that_ ## object_type
#else
#define FIRST_OBJECT_IN_LIST_THAT_( object_type )  tl ## object_type
#endif
#define FIRST_OBJECT_IN_LIST_THAT( object_type ) \
	FIRST_OBJECT_IN_LIST_THAT_(object_type)

#define PROTOTYPE_FIRST_OBJECT_IN_LIST_THAT_FUNCTION( object_type ) \
struct object_type *FIRST_OBJECT_IN_LIST_THAT(object_type)( \
	LIST_CONDITIONAL_FUNCTION(object_type) *conditional,void *user_data, \
	struct LIST(object_type) *list) \
/***************************************************************************** \
LAST MODIFIED : 24 September 1995 \
\
DESCRIPTION : \
If <conditional> is not NULL, the "first" object in the <list> that \
<conditional> returns true is returned, otherwise the "first" object in the \
<list> is returned. \
==============================================================================*/

#if ! defined (SHORT_NAMES)
#define FOR_EACH_OBJECT_IN_LIST_( object_type )  list_for_each_ ## object_type
#else
#define FOR_EACH_OBJECT_IN_LIST_( object_type )  el ## object_type
#endif
#define FOR_EACH_OBJECT_IN_LIST( object_type ) \
	FOR_EACH_OBJECT_IN_LIST_(object_type)

#define PROTOTYPE_FOR_EACH_OBJECT_IN_LIST_FUNCTION( object_type ) \
int FOR_EACH_OBJECT_IN_LIST(object_type)( \
	LIST_ITERATOR_FUNCTION(object_type) *iterator,void *user_data, \
	struct LIST(object_type) *list) \
/***************************************************************************** \
LAST MODIFIED : 23 September 1995 \
\
DESCRIPTION : \
Calls <iterator> for each object in the <list>. \
==============================================================================*/

#define CREATE_LIST_ITERATOR_( object_type ) \
	create_list_iterator_ ## object_type
#define CREATE_LIST_ITERATOR( object_type ) \
	CREATE_LIST_ITERATOR_(object_type)

#define PROTOTYPE_CREATE_LIST_ITERATOR_FUNCTION( object_type , iterator_type ) \
struct iterator_type *CREATE_LIST_ITERATOR(object_type)( \
	struct LIST(object_type) *list) \
/*************************************************************************//** \
 * Returns an iterator object set to the start of the list. \
 * Optional prototype for list implementations supporting iterator objects. \
 */

#define LIST_BEGIN_IDENTIFIER_CHANGE_( object_type, identifier ) \
	list_begin_identifier_change_ ## object_type ## identifier
#define LIST_BEGIN_IDENTIFIER_CHANGE( object_type, identifier ) \
	LIST_BEGIN_IDENTIFIER_CHANGE_(object_type, identifier)

#define PROTOTYPE_INDEXED_LIST_BEGIN_IDENTIFIER_CHANGE_FUNCTION( object_type , \
	identifier ) \
struct LIST_IDENTIFIER_CHANGE_DATA(object_type,identifier) \
	*LIST_BEGIN_IDENTIFIER_CHANGE(object_type,identifier) ( \
	struct object_type *object) \
/***************************************************************************** \
LAST MODIFIED : 13 February 2003 \
\
DESCRIPTION : \
MANAGER functions using indexed object lists must call this before modifying \
the identifier of any object, and afterwards call the companion function \
LIST_END_IDENTIFIER_CHANGE with the returned \
identifier_change_data. These functions temporarily remove the object from \
any list it is in, then re-add it later so it is in the correct indexed \
position. <object> is ACCESSed between these two functions. \
Should only be declared with manager functions. \
============================================================================*/

#define LIST_END_IDENTIFIER_CHANGE_( object_type, identifier ) \
	list_end_identifier_change_ ## object_type ## identifier
#define LIST_END_IDENTIFIER_CHANGE( object_type, identifier ) \
	LIST_END_IDENTIFIER_CHANGE_(object_type, identifier)

#define PROTOTYPE_INDEXED_LIST_END_IDENTIFIER_CHANGE_FUNCTION( \
	object_type , identifier ) \
int LIST_END_IDENTIFIER_CHANGE(object_type,identifier)( \
	struct LIST_IDENTIFIER_CHANGE_DATA(object_type,identifier) \
		**identifier_change_data_address) \
/***************************************************************************** \
LAST MODIFIED : 13 February 2003 \
\
DESCRIPTION : \
Companion function to LIST_BEGIN_IDENTIFIER_CHANGE function. \
Re-adds the changed object to all the lists it was in. \
Should only be declared with manager functions. \
============================================================================*/

#define LIST_CLASS(object_type) list_class_ ## object_type
/***************************************************************************** \
LAST MODIFIED : 9 February 2007 \
\
DESCRIPTION : \
Wraps the existing Manager functionality and types into a class. \
==============================================================================*/
#define DEFINE_LIST_CLASS(object_type) \
class LIST_CLASS(object_type) \
{\
public: \
	typedef LIST(object_type) List_type; \
	typedef LIST_CONDITIONAL_FUNCTION(object_type) List_conditional_function; \
\
	LIST(object_type) *list; \
\
   LIST_CLASS(object_type)(LIST(object_type) *list) : list(list) \
	{ \
	} \
\
	inline int number_in_list() \
	{ \
		return NUMBER_IN_LIST(object_type)(list); \
	} \
\
	inline int for_each_object_in_list(	\
		LIST_ITERATOR_FUNCTION(object_type) *iterator, void *user_data) \
	{ \
		return FOR_EACH_OBJECT_IN_LIST(object_type)( \
			iterator, user_data, list); \
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
		return GET_NAME(object_type)(object, name);		\
	} \
\
}; /* LIST_CLASS(object_type) */

#define DECLARE_LIST_TYPES( object_type ) \
DECLARE_LIST_TYPE(object_type); \
DECLARE_LIST_CONDITIONAL_FUNCTION(object_type); \
DECLARE_LIST_ITERATOR_FUNCTION(object_type)

/* PROTOTYPE_ADD_OBJECT_TO_FRONT_OF_LIST_FUNCTION(object_type); is not
	automatically included as it is unavailable to INDEXED lists so
	it is up to the definer of the list to PROTOTYPE and DECLARE it 
	if appropriate/required. */

#define PROTOTYPE_LIST_FUNCTIONS( object_type ) \
PROTOTYPE_CREATE_LIST_FUNCTION(object_type); \
PROTOTYPE_CREATE_RELATED_LIST_FUNCTION(object_type); \
PROTOTYPE_DESTROY_LIST_FUNCTION(object_type); \
PROTOTYPE_COPY_LIST_FUNCTION(object_type); \
PROTOTYPE_REMOVE_OBJECT_FROM_LIST_FUNCTION(object_type); \
PROTOTYPE_REMOVE_ALL_OBJECTS_FROM_LIST_FUNCTION(object_type); \
PROTOTYPE_REMOVE_OBJECTS_FROM_LIST_THAT_FUNCTION(object_type); \
PROTOTYPE_ADD_OBJECT_TO_LIST_FUNCTION(object_type); \
PROTOTYPE_NUMBER_IN_LIST_FUNCTION(object_type); \
PROTOTYPE_IS_OBJECT_IN_LIST_FUNCTION(object_type); \
PROTOTYPE_FIRST_OBJECT_IN_LIST_THAT_FUNCTION(object_type); \
PROTOTYPE_FOR_EACH_OBJECT_IN_LIST_FUNCTION(object_type)
#endif
