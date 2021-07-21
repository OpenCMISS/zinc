/*******************************************************************************
FILE : list_private.h

LAST MODIFIED : 16 October 2002

DESCRIPTION :
Macros for declaring standard list types and declaring standard list functions.
This file contains the details of the internal workings of the list and should
be included in the module that declares the list for a particular object type,
but should not be visible to modules that use lists for a particular object
type.  This allows changes to the internal structure to be made without
affecting other parts of the program.

Also see indexed_list_private.h .
???DB.  What if want to use different internal structures within same module ?
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (LIST_PRIVATE_H)
#define LIST_PRIVATE_H

#include "general/list.h"

/*
Macros
======
*/

/*
Local types
-----------
*/
#if ! defined (SHORT_NAMES)
#define LIST_ITEM( object_type )  list_item_ ## object_type
#else
#define LIST_ITEM( object_type )  li ## object_type
#endif

#define FULL_DECLARE_LIST_TYPE( object_type ) \
struct LIST_ITEM(object_type) \
/***************************************************************************** \
LAST MODIFIED : 24 September 1995 \
\
DESCRIPTION : \
An item in a object_type list. \
============================================================================*/ \
{ \
	/* pointer to the object */ \
	struct object_type *object; \
	/* the next item in the list */ \
	struct LIST_ITEM(object_type) *next; \
}; /* struct LIST_ITEM(object_type) */ \
\
DECLARE_LIST_TYPE(object_type) \
/***************************************************************************** \
LAST MODIFIED : 17 July 1995 \
\
DESCRIPTION : \
The object_type list structure. \
============================================================================*/ \
{ \
	/* the number of objects in the list */ \
	int count; \
	/* the start and end of the list */ \
	struct LIST_ITEM(object_type) *head,*tail; \
} /* struct LIST(object_type) */

/*
Global functions
----------------
*/
#define DECLARE_CREATE_LIST_FUNCTION( object_type ) \
PROTOTYPE_CREATE_LIST_FUNCTION(object_type) \
{ \
	struct LIST(object_type) *list; \
\
	ENTER(CREATE_LIST(object_type)); \
	if (ALLOCATE(list,struct LIST(object_type),1)) \
	{ \
		list->count=0; \
		list->head=(struct LIST_ITEM(object_type) *)NULL; \
		list->tail=(struct LIST_ITEM(object_type) *)NULL; \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"CREATE_LIST(" #object_type ").  Insufficient memory"); \
	} \
	LEAVE; \
\
	return (list); \
} /* CREATE_LIST(object_type) */

#define DECLARE_CREATE_RELATED_LIST_FUNCTION( object_type ) \
PROTOTYPE_CREATE_RELATED_LIST_FUNCTION(object_type) \
{ \
	USE_PARAMETER(other_list); \
	return CREATE_LIST(object_type)(); \
}

#define DECLARE_DESTROY_LIST_FUNCTION( object_type ) \
PROTOTYPE_DESTROY_LIST_FUNCTION(object_type) \
{ \
	int return_code; \
	struct LIST_ITEM(object_type) *item,*remove; \
\
	ENTER(DESTROY_LIST(object_type)); \
	if (list_address) \
	{ \
		if (*list_address) \
		{ \
			item=(*list_address)->head; \
			while (item) \
			{ \
				remove=item; \
				item=item->next; \
				DEACCESS(object_type)(&(remove->object)); \
				DEALLOCATE(remove); \
			} \
			DEALLOCATE(*list_address); \
		} \
		return_code=1; \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"DESTROY_LIST(" #object_type ").  Invalid argument"); \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* DESTROY_LIST(object_type) */

#define DECLARE_COPY_LIST_FUNCTION( object_type ) \
PROTOTYPE_COPY_LIST_FUNCTION(object_type) \
{ \
	int count,return_code; \
	struct LIST_ITEM(object_type) **add,*item,*remove,*tail; \
\
	ENTER(COPY_LIST(object_type)); \
	if (target_list&&source_list) \
	{ \
		/* remove the current contents of the <target_list> */ \
		item=target_list->head; \
		while (item) \
		{ \
			remove=item; \
			item=item->next; \
			DEACCESS(object_type)(&(remove->object)); \
			DEALLOCATE(remove); \
		} \
		count=0; \
		/* copy the contents of the <source_list> to the <target_list> */ \
		return_code=1; \
		item=source_list->head; \
		add= &(target_list->head); \
		tail=(struct LIST_ITEM(object_type) *)NULL; \
		while (return_code&&item) \
		{ \
			if (ALLOCATE(tail,struct LIST_ITEM(object_type),1)) \
			{ \
				*add=tail; \
				tail->object=ACCESS(object_type)(item->object); \
				add= &(tail->next); \
				item=item->next; \
				count++; \
			} \
			else \
			{ \
				display_message(ERROR_MESSAGE, \
					"COPY_LIST(" #object_type ").  Could not allocate list item"); \
				return_code=0; \
			} \
		} \
		*add=(struct LIST_ITEM(object_type) *)NULL; \
		target_list->count=count; \
		target_list->tail=tail; \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"COPY_LIST(" #object_type ").  Invalid argument(s)"); \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* COPY_LIST(object_type) */

#define DECLARE_ADD_OBJECT_TO_LIST_FUNCTION( object_type ) \
PROTOTYPE_ADD_OBJECT_TO_LIST_FUNCTION( object_type ) \
{ \
	int return_code; \
	struct LIST_ITEM(object_type) *add_item; \
\
	ENTER(ADD_OBJECT_TO_LIST(object_type)); \
	if (object && list) \
	{ \
		/* allocate memory for list item */ \
		if (ALLOCATE(add_item, struct LIST_ITEM(object_type), 1)) \
		{ \
			add_item->object = ACCESS(object_type)(object); \
			add_item->next = (struct LIST_ITEM(object_type) *)NULL; \
			if (list->tail) \
			{ \
				(list->tail)->next = add_item; \
				list->tail = add_item; \
			} \
			else \
			{ \
				list->head = add_item; \
				list->tail = add_item; \
			} \
			(list->count)++; \
			return_code = 1; \
		} \
		else \
		{ \
			display_message(ERROR_MESSAGE, "ADD_OBJECT_TO_LIST(" #object_type \
				").  Could not allocate memory for list item"); \
			return_code = 0; \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"ADD_OBJECT_TO_LIST(" #object_type ").  Invalid argument(s)"); \
		return_code = 0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* ADD_OBJECT_TO_LIST(object_type) */

#define DECLARE_ADD_OBJECT_TO_FRONT_OF_LIST_FUNCTION( object_type ) \
PROTOTYPE_ADD_OBJECT_TO_FRONT_OF_LIST_FUNCTION( object_type ) \
{ \
	int return_code; \
	struct LIST_ITEM(object_type) *add_item; \
\
	ENTER(ADD_OBJECT_TO_FRONT_OF_LIST(object_type)); \
	if (object && list) \
	{ \
		/* allocate memory for list item */ \
		if (ALLOCATE(add_item, struct LIST_ITEM(object_type), 1)) \
		{ \
			add_item->object = ACCESS(object_type)(object); \
			add_item->next = (struct LIST_ITEM(object_type) *)NULL; \
			if (list->head) \
			{ \
				add_item->next = list->head; \
				list->head = add_item; \
			} \
			else \
			{ \
				list->head = add_item; \
				list->tail = add_item; \
			} \
			(list->count)++; \
			return_code = 1; \
		} \
		else \
		{ \
			display_message(ERROR_MESSAGE, "ADD_OBJECT_TO_FRONT_OF_LIST(" #object_type \
				").  Could not allocate memory for list item"); \
			return_code = 0; \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"ADD_OBJECT_TO_FRONT_OF_LIST(" #object_type ").  Invalid argument(s)"); \
		return_code = 0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* ADD_OBJECT_TO_FRONT_OF_LIST(object_type) */

#define DECLARE_REMOVE_OBJECT_FROM_LIST_FUNCTION( object_type ) \
PROTOTYPE_REMOVE_OBJECT_FROM_LIST_FUNCTION(object_type) \
{ \
	int return_code; \
	struct LIST_ITEM(object_type) *item, *previous_item; \
\
	ENTER(REMOVE_OBJECT_FROM_LIST(object_type)); \
	if (object && list) \
	{ \
		/* search for the object in the list */ \
		item = list->head; \
		previous_item = (struct LIST_ITEM(object_type) *)NULL; \
		while (item && (object != item->object)) \
		{ \
			previous_item = item; \
			item = item->next; \
		} \
		if (item) \
		{ \
			/* remove the object from the list */ \
			if (item == list->head) \
			{ \
				list->head = item->next; \
			} \
			else \
			{ \
				previous_item->next = item->next; \
			} \
			if (item == list->tail) \
			{ \
				list->tail = previous_item; \
			} \
			return_code = DEACCESS(object_type)(&(item->object)); \
			DEALLOCATE(item); \
			(list->count)--; \
			return_code = 1; \
		} \
		else \
		{ \
			display_message(ERROR_MESSAGE, \
				"REMOVE_OBJECT_FROM_LIST(" #object_type ").  Could not find object"); \
			return_code = 0; \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"REMOVE_OBJECT_FROM_LIST(" #object_type ").  Invalid argument(s)"); \
		return_code = 0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* REMOVE_OBJECT_FROM_LIST(object_type) */

#define DECLARE_REMOVE_ALL_OBJECTS_FROM_LIST_FUNCTION( object_type ) \
PROTOTYPE_REMOVE_ALL_OBJECTS_FROM_LIST_FUNCTION(object_type) \
{ \
	int return_code; \
	struct LIST_ITEM(object_type) *item,*remove; \
\
	ENTER(REMOVE_ALL_OBJECTS_FROM_LIST(object_type)); \
	if (list) \
	{ \
		item=list->head; \
		while (item) \
		{ \
			remove=item; \
			item=item->next; \
			DEACCESS(object_type)(&(remove->object)); \
			DEALLOCATE(remove); \
		} \
		list->count=0; \
		list->head=(struct LIST_ITEM(object_type) *)NULL; \
		list->tail=(struct LIST_ITEM(object_type) *)NULL; \
		return_code=1; \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"REMOVE_ALL_OBJECTS_FROM_LIST" #object_type ").  Invalid argument(s)"); \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* REMOVE_ALL_OBJECTS_FROM_LIST(object_type) */

#define DECLARE_REMOVE_OBJECTS_FROM_LIST_THAT_FUNCTION( object_type ) \
PROTOTYPE_REMOVE_OBJECTS_FROM_LIST_THAT_FUNCTION(object_type) \
{ \
	int return_code; \
	struct LIST_ITEM(object_type) *item,*remove,*previous_item; \
\
	ENTER(REMOVE_OBJECTS_FROM_LIST_THAT(object_type)); \
	if (list&&conditional) \
	{ \
		item=list->head; \
		previous_item=(struct LIST_ITEM(object_type) *)NULL; \
		while (item) \
		{ \
      if(conditional(item->object,user_data)) \
      { \
        remove=item; \
  			if (item==list->head) \
  			{ \
	  			list->head=item->next; \
	  		} \
		  	else \
  			{ \
	  			previous_item->next=item->next; \
		  	} \
  			if (item==list->tail) \
	  		{ \
		  		list->tail=previous_item; \
  			} \
			  item=item->next; \
           DEACCESS(object_type)(&(remove->object)); \
			  DEALLOCATE(remove); \
			  (list->count)--; \
      } \
      else \
      { \
			  previous_item=item; \
			  item=item->next; \
      } \
		} \
		return_code=1; \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"REMOVE_OBJECTS_FROM_LIST_THAT" #object_type ").  Invalid argument(s)"); \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* REMOVE_OBJECTS_FROM_LIST_THAT(object_type) */

#define DECLARE_NUMBER_IN_LIST_FUNCTION( object_type ) \
PROTOTYPE_NUMBER_IN_LIST_FUNCTION(object_type) \
{ \
	int count; \
\
	ENTER(NUMBER_IN_LIST(object_type)); \
	if (list) \
	{ \
		count=list->count; \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"NUMBER_IN_LIST(" #object_type ").  Invalid argument"); \
		count=0; \
	} \
	LEAVE; \
\
	return (count); \
} /* NUMBER_IN_LIST(object_type) */

#define DECLARE_IS_OBJECT_IN_LIST_FUNCTION( object_type ) \
PROTOTYPE_IS_OBJECT_IN_LIST_FUNCTION(object_type) \
{ \
	int return_code; \
	struct LIST_ITEM(object_type) *item; \
\
	ENTER(IS_OBJECT_IN_LIST(object_type)); \
	if (list) \
	{ \
		item=list->head; \
		return_code=0; \
		/* search through the list for the object */ \
		while (item&&!return_code) \
		{ \
			if (object==item->object) \
			{ \
				return_code=1; \
			} \
			item=item->next; \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"IS_OBJECT_IN_LIST(" #object_type ").  Invalid argument"); \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* IS_OBJECT_IN_LIST(object_type) */

#define DECLARE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION( object_type , \
	identifier, identifier_type , compare_function ) \
PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(object_type,identifier, \
	identifier_type) \
{ \
	struct object_type *object; \
	struct LIST_ITEM(object_type) *item; \
\
	ENTER(FIND_BY_IDENTIFIER_IN_LIST(object_type,identifier)); \
	if (list) \
	{ \
		/* search through the list for a object with the specified identifier */ \
		item=list->head; \
		object=(struct object_type *)NULL; \
		while (item&&!object) \
		{ \
			if ((object=item->object)&&compare_function(identifier, \
				object->identifier)) \
			{ \
				object=(struct object_type *)NULL; \
			} \
			item=item->next; \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE,"FIND_BY_IDENTIFIER_IN_LIST(" #object_type \
			"," #identifier ").  Invalid argument"); \
		object=(struct object_type *)NULL; \
	} \
	LEAVE; \
\
	return (object); \
} /* FIND_BY_IDENTIFIER_IN_LIST(object_type,identifier) */

#define DECLARE_FIRST_OBJECT_IN_LIST_THAT_FUNCTION( object_type ) \
PROTOTYPE_FIRST_OBJECT_IN_LIST_THAT_FUNCTION(object_type) \
{ \
	struct object_type *object; \
	struct LIST_ITEM(object_type) *item; \
\
	ENTER(FIRST_OBJECT_IN_LIST_THAT(object_type)); \
	/* check the arguments */ \
	if (list) \
	{ \
		if (conditional) \
		{ \
			item=list->head; \
			object=(struct object_type *)NULL; \
			while (item&&!object) \
			{ \
				if ((conditional)(item->object,user_data)) \
				{ \
					object=item->object; \
				} \
				item=item->next; \
			} \
		} \
		else \
		{ \
			if (list->head) \
			{ \
				object=list->head->object; \
			} \
			else \
			{ \
				object=(struct object_type *)NULL; \
			} \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"FIRST_OBJECT_IN_LIST_THAT(" #object_type ").  Invalid argument(s)"); \
		object=(struct object_type *)NULL; \
	} \
	LEAVE; \
\
	return (object); \
} /* FIRST_OBJECT_IN_LIST_THAT(object_type) */

#define DECLARE_FOR_EACH_OBJECT_IN_LIST_FUNCTION( object_type ) \
PROTOTYPE_FOR_EACH_OBJECT_IN_LIST_FUNCTION(object_type) \
{ \
	int return_code; \
	struct object_type *object; \
	struct LIST_ITEM(object_type) *item; \
\
	ENTER(FOR_EACH_OBJECT_IN_LIST(object_type)); \
	/* check the arguments */ \
	if (list&&iterator) \
	{ \
		item=list->head; \
		return_code=1; \
		while (item&&return_code) \
		{ \
			object=item->object; \
			item=item->next; \
			return_code=(iterator)(object,user_data); \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"FOR_EACH_OBJECT_IN_LIST(" #object_type ").  Invalid argument(s)"); \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* FOR_EACH_OBJECT_IN_LIST(object_type) */

#if ! defined (SHORT_NAMES)
#define LIST_IDENTIFIER_CHANGE_DATA( object_type , identifier ) \
	list_identifier_change_data_ ## object_type ## identifier
#else
#define LIST_IDENTIFIER_CHANGE_DATA( object_type , identifier ) \
	licd ## object_type ## identifier
#endif

#define DECLARE_LIST_IDENTIFIER_CHANGE_DATA( object_type , identifier ) \
struct LIST_IDENTIFIER_CHANGE_DATA(object_type,identifier) \
/***************************************************************************** \
LAST MODIFIED : 21 December 2000 \
\
DESCRIPTION : \
Data structure used by LIST_BEGIN/END_IDENTIFIER_CHANGE functions. \
Should only be declared with manager functions. \
============================================================================*/ \
{ \
	void *dummy; \
} /* struct LIST_IDENTIFIER_CHANGE_DATA(object_type,identifier) */

#define DECLARE_LIST_BEGIN_IDENTIFIER_CHANGE_FUNCTION( object_type , \
	identifier ) \
static struct LIST_IDENTIFIER_CHANGE_DATA(object_type,identifier) \
	*LIST_BEGIN_IDENTIFIER_CHANGE(object_type,identifier) ( \
	struct object_type *object) \
/***************************************************************************** \
LAST MODIFIED : 21 December 2000 \
\
DESCRIPTION : \
Dummy version of equivalent function used by indexed lists. \
Should only be declared with manager functions. \
============================================================================*/ \
{ \
	struct LIST_IDENTIFIER_CHANGE_DATA(object_type,identifier) \
		*identifier_change_data; \
\
	ENTER(LIST_BEGIN_IDENTIFIER_CHANGE(object_type,identifier)); \
	if (object) \
	{ \
		if (ALLOCATE(identifier_change_data, \
			struct LIST_IDENTIFIER_CHANGE_DATA(object_type, identifier), 1)) \
		{ \
			identifier_change_data->dummy = (void *)NULL; \
		} \
		else \
		{ \
			display_message(ERROR_MESSAGE, \
				"LIST_BEGIN_IDENTIFIER_CHANGE(" #object_type "," #identifier \
				").  Not enough memory"); \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"LIST_BEGIN_IDENTIFIER_CHANGE(" #object_type "," #identifier \
			").  Invalid argument(s)"); \
		identifier_change_data = \
			(struct LIST_IDENTIFIER_CHANGE_DATA(object_type,identifier) *)NULL; \
	} \
	LEAVE; \
\
	return (identifier_change_data); \
} /* LIST_BEGIN_IDENTIFIER_CHANGE(object_type,identifier) */

#define DECLARE_LIST_END_IDENTIFIER_CHANGE_FUNCTION( \
	object_type , identifier ) \
static int LIST_END_IDENTIFIER_CHANGE(object_type,identifier)( \
	struct LIST_IDENTIFIER_CHANGE_DATA(object_type,identifier) \
		**identifier_change_data_address) \
/***************************************************************************** \
LAST MODIFIED : 21 December 2000 \
\
DESCRIPTION : \
Companion function to LIST_BEGIN_IDENTIFIER_CHANGE function. \
Should only be declared with manager functions. \
============================================================================*/ \
{ \
	int return_code; \
\
	ENTER(LIST_END_IDENTIFIER_CHANGE(object_type,identifier)); \
	if (identifier_change_data_address && *identifier_change_data_address) \
	{ \
		DEALLOCATE(*identifier_change_data_address); \
		*identifier_change_data_address = \
			(struct LIST_IDENTIFIER_CHANGE_DATA(object_type,identifier) *)NULL; \
		return_code = 1;					\
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"LIST_END_IDENTIFIER_CHANGE(" #object_type "," #identifier \
			").  Invalid argument(s)"); \
		return_code = 0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* LIST_END_IDENTIFIER_CHANGE(object_type,identifier) */

/* following only need to be declared when used with managers */
#define DECLARE_LIST_IDENTIFIER_CHANGE_FUNCTIONS( object_type , identifier ) \
DECLARE_LIST_IDENTIFIER_CHANGE_DATA(object_type,identifier); \
DECLARE_LIST_BEGIN_IDENTIFIER_CHANGE_FUNCTION(object_type,identifier) \
DECLARE_LIST_END_IDENTIFIER_CHANGE_FUNCTION(object_type,identifier)

/* DECLARE_ADD_OBJECT_TO_FRONT_OF_LIST_FUNCTION(object_type); is not
	automatically included as it is unavailable to INDEXED lists so
	it is up to the definer of the list to PROTOTYPE and DECLARE it 
	if appropriate/required. */

#define DECLARE_LIST_FUNCTIONS( object_type ) \
DECLARE_CREATE_LIST_FUNCTION(object_type) \
DECLARE_CREATE_RELATED_LIST_FUNCTION(object_type) \
DECLARE_DESTROY_LIST_FUNCTION(object_type) \
DECLARE_COPY_LIST_FUNCTION(object_type) \
DECLARE_REMOVE_OBJECT_FROM_LIST_FUNCTION(object_type) \
DECLARE_REMOVE_ALL_OBJECTS_FROM_LIST_FUNCTION(object_type) \
DECLARE_REMOVE_OBJECTS_FROM_LIST_THAT_FUNCTION(object_type) \
DECLARE_ADD_OBJECT_TO_LIST_FUNCTION(object_type) \
DECLARE_NUMBER_IN_LIST_FUNCTION(object_type) \
DECLARE_IS_OBJECT_IN_LIST_FUNCTION(object_type) \
DECLARE_FIRST_OBJECT_IN_LIST_THAT_FUNCTION(object_type) \
DECLARE_FOR_EACH_OBJECT_IN_LIST_FUNCTION(object_type)

#endif /* !defined (LIST_PRIVATE_H) */
