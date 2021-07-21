/***************************************************************************//**
 * FILE : indexed_list_stl_private.hpp
 *
 * Implementation of indexed lists which delegates to an STL container.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (INDEXED_LIST_STL_PRIVATE_HPP)
#define INDEXED_LIST_STL_PRIVATE_HPP

#include "general/list.h"

#define CMZN_SET( object_type )  cmzn_set_ ## object_type

#define FULL_DECLARE_INDEXED_LIST_STL_TYPE( object_type )

/*
Global functions
----------------
*/

#define DECLARE_CREATE_INDEXED_LIST_STL_FUNCTION( object_type ) \
PROTOTYPE_CREATE_LIST_FUNCTION(object_type) \
{ \
	return reinterpret_cast<struct LIST(object_type) *>(CMZN_SET(object_type)::create_independent()); \
}

#define DECLARE_CREATE_RELATED_INDEXED_LIST_STL_FUNCTION( object_type ) \
PROTOTYPE_CREATE_RELATED_LIST_FUNCTION(object_type) \
{ \
	CMZN_SET(object_type) *other = reinterpret_cast<CMZN_SET(object_type) *>(other_list); \
	return reinterpret_cast<struct LIST(object_type) *>(other->create_related()); \
}

#define DECLARE_DESTROY_INDEXED_LIST_STL_FUNCTION( object_type ) \
PROTOTYPE_DESTROY_LIST_FUNCTION(object_type) \
{ \
	if (list_address) \
	{ \
		CMZN_SET(object_type) **cmiss_set_address = \
			reinterpret_cast<CMZN_SET(object_type) **>(list_address); \
		return CMZN_SET(object_type)::deaccess(cmiss_set_address); \
	} \
	return 0; \
}

#define DECLARE_COPY_INDEXED_LIST_STL_FUNCTION( object_type ) \
PROTOTYPE_COPY_LIST_FUNCTION(object_type) \
{ \
	int return_code; \
	if (target_list && source_list) \
	{ \
		CMZN_SET(object_type) *target = reinterpret_cast<CMZN_SET(object_type) *>(target_list); \
		CMZN_SET(object_type) *source = reinterpret_cast<CMZN_SET(object_type) *>(source_list); \
		*target = *source; \
		return_code = 1; \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"COPY_LIST(" #object_type ").  Invalid argument(s)"); \
		return_code = 0; \
	} \
	return (return_code); \
}

#define DECLARE_REMOVE_OBJECT_FROM_INDEXED_LIST_STL_FUNCTION( object_type ) \
PROTOTYPE_REMOVE_OBJECT_FROM_LIST_FUNCTION(object_type) \
{ \
	int return_code; \
	if (object && list) \
	{ \
		CMZN_SET(object_type) *cmiss_set = reinterpret_cast<CMZN_SET(object_type) *>(list); \
		return_code = (1 == cmiss_set->erase(object)); \
		if (!return_code) \
		{ \
			display_message(ERROR_MESSAGE, "REMOVE_OBJECT_FROM_LIST(" #object_type \
				").  Object is not in list"); \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"REMOVE_OBJECT_FROM_LIST(" #object_type ").  Invalid argument(s)"); \
		return_code=0; \
	} \
	return (return_code); \
}

#define DECLARE_REMOVE_OBJECTS_FROM_INDEXED_LIST_STL_THAT_FUNCTION( object_type ) \
PROTOTYPE_REMOVE_OBJECTS_FROM_LIST_THAT_FUNCTION(object_type) \
{ \
	int return_code; \
	if (conditional && list) \
	{ \
		CMZN_SET(object_type) *cmiss_set = reinterpret_cast<CMZN_SET(object_type) *>(list); \
		CMZN_SET(object_type)::iterator iter = cmiss_set->begin(); \
		while (iter != cmiss_set->end()) \
		{ \
			if ((conditional)(*iter,user_data)) \
			{ \
				CMZN_SET(object_type)::iterator tmp = iter; \
				++iter; \
				cmiss_set->erase(tmp); \
			} \
			else \
			{ \
				++iter; \
			} \
		} \
		return_code = 1; \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"REMOVE_OBJECTS_FROM_LIST_THAT" #object_type ").  Invalid argument(s)"); \
		return_code = 0; \
	} \
	return (return_code); \
}

#define DECLARE_REMOVE_ALL_OBJECTS_FROM_INDEXED_LIST_STL_FUNCTION( object_type ) \
PROTOTYPE_REMOVE_ALL_OBJECTS_FROM_LIST_FUNCTION(object_type) \
{ \
	int return_code; \
	if (list) \
	{ \
		CMZN_SET(object_type) *cmiss_set = reinterpret_cast<CMZN_SET(object_type) *>(list); \
		cmiss_set->clear(); \
		return_code = 1; \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"REMOVE_ALL_OBJECTS_FROM_LIST" #object_type ").  Invalid argument(s)"); \
		return_code = 0; \
	} \
	return (return_code); \
}

#define DECLARE_ADD_OBJECT_TO_INDEXED_LIST_STL_FUNCTION( object_type ) \
PROTOTYPE_ADD_OBJECT_TO_LIST_FUNCTION( object_type ) \
{ \
	int return_code; \
	if (object && list) \
	{ \
		CMZN_SET(object_type) *cmiss_set = reinterpret_cast<CMZN_SET(object_type) *>(list); \
		std::pair<CMZN_SET(object_type)::iterator,bool> result = cmiss_set->insert(object); \
		if (result.second) \
		{ \
			return_code = 1; \
		} \
		else \
		{ \
			display_message(ERROR_MESSAGE, "ADD_OBJECT_TO_LIST(" #object_type \
				").  Object is already at that index"); \
			return_code = 0; \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"ADD_OBJECT_TO_LIST(" #object_type ").  Invalid argument(s)"); \
		return_code = 0; \
	} \
	return (return_code); \
}

#define DECLARE_NUMBER_IN_INDEXED_LIST_STL_FUNCTION( object_type ) \
PROTOTYPE_NUMBER_IN_LIST_FUNCTION(object_type) \
{ \
	if (list) \
	{ \
		CMZN_SET(object_type) *cmiss_set = reinterpret_cast<CMZN_SET(object_type) *>(list); \
		return static_cast<int>(cmiss_set->size()); \
	} \
	display_message(ERROR_MESSAGE, \
		"NUMBER_IN_LIST(" #object_type ").  Invalid argument"); \
	return 0; \
}

#define DECLARE_IS_OBJECT_IN_INDEXED_LIST_STL_FUNCTION( object_type ) \
PROTOTYPE_IS_OBJECT_IN_LIST_FUNCTION(object_type) \
{ \
	int return_code = 0; \
	if (list && object) \
	{ \
		CMZN_SET(object_type) *cmiss_set = reinterpret_cast<CMZN_SET(object_type) *>(list); \
		CMZN_SET(object_type)::iterator iter = cmiss_set->find(object); \
		if (iter != cmiss_set->end()) \
		{ \
			if (*iter == object) \
			{ \
				return_code = 1; \
			} \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"IS_OBJECT_IN_LIST(" #object_type ").  Invalid argument"); \
	} \
	return (return_code); \
}

#define DECLARE_FIRST_OBJECT_IN_INDEXED_LIST_STL_THAT_FUNCTION( object_type ) \
PROTOTYPE_FIRST_OBJECT_IN_LIST_THAT_FUNCTION(object_type) \
{ \
	if (list) \
	{ \
		CMZN_SET(object_type) *cmiss_set = reinterpret_cast<CMZN_SET(object_type) *>(list); \
		for (CMZN_SET(object_type)::iterator iter = cmiss_set->begin(); iter != cmiss_set->end(); ++iter) \
		{ \
			if ((!conditional) || ((conditional)(*iter,user_data))) \
				return *iter; \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"FIRST_OBJECT_IN_LIST_THAT(" #object_type ").  Invalid argument(s)"); \
	} \
	return 0; \
}

#define DECLARE_FOR_EACH_OBJECT_IN_INDEXED_LIST_STL_FUNCTION( object_type ) \
PROTOTYPE_FOR_EACH_OBJECT_IN_LIST_FUNCTION(object_type) \
{ \
	int return_code = 1; \
	if (list && iterator) \
	{ \
		CMZN_SET(object_type) *cmiss_set = reinterpret_cast<CMZN_SET(object_type) *>(list); \
		for (CMZN_SET(object_type)::iterator iter = cmiss_set->begin(); iter != cmiss_set->end(); ++iter) \
		{ \
			if (!(iterator)(*iter, user_data)) \
			{ \
				return_code = 0; \
				break; \
			} \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"FOR_EACH_OBJECT_IN_LIST(" #object_type ").  Invalid argument(s)"); \
		return_code = 0; \
	} \
	return (return_code); \
}

#define DECLARE_CREATE_INDEXED_LIST_STL_ITERATOR_FUNCTION( object_type , iterator_type ) \
PROTOTYPE_CREATE_LIST_ITERATOR_FUNCTION(object_type,iterator_type) \
{ \
	CMZN_SET(object_type) *cmiss_set = reinterpret_cast<CMZN_SET(object_type) *>(list); \
	if (cmiss_set) \
		return iterator_type::create(cmiss_set); \
	return 0; \
}

#define DECLARE_FIND_BY_IDENTIFIER_IN_INDEXED_LIST_STL_FUNCTION( object_type , \
	identifier , identifier_type ) \
PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(object_type,identifier, \
	identifier_type) \
{ \
	struct object_type *object = 0; \
	if (list) \
	{ \
		object_type ## _identifier tmp(identifier); \
		CMZN_SET(object_type) *cmiss_set = reinterpret_cast<CMZN_SET(object_type) *>(list); \
		CMZN_SET(object_type)::iterator iter = cmiss_set->find(tmp.getPseudoObject()); \
		if (iter != cmiss_set->end()) \
			object = *iter; \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, "FIND_BY_IDENTIFIER_IN_LIST(" #object_type \
			"," #identifier ").  Invalid argument"); \
	} \
	return (object); \
}

#define PROTOTYPE_INDEXED_LIST_STL_BEGIN_IDENTIFIER_CHANGE_FUNCTION( object_type , \
	identifier ) \
int LIST_BEGIN_IDENTIFIER_CHANGE(object_type,identifier)( \
	struct LIST(object_type) *list, struct object_type *object) \
/***************************************************************************** \
MANAGER functions using indexed object lists must call this before modifying \
the identifier of any object, and afterwards call the companion function \
LIST_END_IDENTIFIER_CHANGE with the returned \
identifier_change_data. These functions temporarily remove the object from \
any list it is in, then re-add it later so it is in the correct indexed \
position. <object> is ACCESSed between these two functions. \
Should only be declared with manager functions. \
============================================================================*/

#define DECLARE_INDEXED_LIST_STL_BEGIN_IDENTIFIER_CHANGE_FUNCTION( object_type , \
	identifier ) \
PROTOTYPE_INDEXED_LIST_STL_BEGIN_IDENTIFIER_CHANGE_FUNCTION(object_type, \
	identifier) \
{ \
	if (list && object) \
	{ \
		CMZN_SET(object_type) *cmiss_set = reinterpret_cast<CMZN_SET(object_type) *>(list); \
		return cmiss_set->begin_identifier_change(object); \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"LIST_BEGIN_IDENTIFIER_CHANGE(" #object_type "," #identifier \
			").  Invalid argument(s)"); \
	} \
	return (0); \
}

#define PROTOTYPE_INDEXED_LIST_STL_END_IDENTIFIER_CHANGE_FUNCTION( \
	object_type , identifier ) \
void LIST_END_IDENTIFIER_CHANGE(object_type,identifier)( \
	struct LIST(object_type) *list) \
/***************************************************************************** \
Companion function to LIST_BEGIN_IDENTIFIER_CHANGE function. \
Re-adds the changed object to all the lists it was in. \
Should only be declared with manager functions. \
============================================================================*/

#define DECLARE_INDEXED_LIST_STL_END_IDENTIFIER_CHANGE_FUNCTION(object_type ,  \
	identifier ) \
PROTOTYPE_INDEXED_LIST_STL_END_IDENTIFIER_CHANGE_FUNCTION(object_type, \
	identifier) \
{ \
	if (list) \
	{ \
		CMZN_SET(object_type) *cmiss_set = reinterpret_cast<CMZN_SET(object_type) *>(list); \
		cmiss_set->end_identifier_change(); \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"LIST_BEGIN_IDENTIFIER_CHANGE(" #object_type "," #identifier \
			").  Invalid argument(s)"); \
	} \
}

/* prototypes for private headers, eg. finite_element_private.h */
#define PROTOTYPE_INDEXED_LIST_STL_IDENTIFIER_CHANGE_FUNCTIONS( object_type , identifier ) \
PROTOTYPE_INDEXED_LIST_STL_BEGIN_IDENTIFIER_CHANGE_FUNCTION(object_type, identifier); \
PROTOTYPE_INDEXED_LIST_STL_END_IDENTIFIER_CHANGE_FUNCTION(object_type,identifier)

#define DECLARE_INDEXED_LIST_STL_IDENTIFIER_CHANGE_FUNCTIONS( object_type , identifier ) \
DECLARE_INDEXED_LIST_STL_BEGIN_IDENTIFIER_CHANGE_FUNCTION(object_type,identifier) \
DECLARE_INDEXED_LIST_STL_END_IDENTIFIER_CHANGE_FUNCTION(object_type,identifier)

#define DECLARE_INDEXED_LIST_STL_FUNCTIONS( object_type ) \
DECLARE_CREATE_INDEXED_LIST_STL_FUNCTION(object_type) \
DECLARE_CREATE_RELATED_INDEXED_LIST_STL_FUNCTION(object_type) \
DECLARE_DESTROY_INDEXED_LIST_STL_FUNCTION(object_type) \
DECLARE_COPY_INDEXED_LIST_STL_FUNCTION(object_type) \
DECLARE_REMOVE_OBJECT_FROM_INDEXED_LIST_STL_FUNCTION(object_type) \
DECLARE_REMOVE_OBJECTS_FROM_INDEXED_LIST_STL_THAT_FUNCTION(object_type) \
DECLARE_REMOVE_ALL_OBJECTS_FROM_INDEXED_LIST_STL_FUNCTION(object_type) \
DECLARE_ADD_OBJECT_TO_INDEXED_LIST_STL_FUNCTION(object_type) \
DECLARE_NUMBER_IN_INDEXED_LIST_STL_FUNCTION(object_type) \
DECLARE_IS_OBJECT_IN_INDEXED_LIST_STL_FUNCTION(object_type) \
DECLARE_FIRST_OBJECT_IN_INDEXED_LIST_STL_THAT_FUNCTION(object_type) \
DECLARE_FOR_EACH_OBJECT_IN_INDEXED_LIST_STL_FUNCTION(object_type)

#endif /* !defined (INDEXED_LIST_STL_PRIVATE_HPP) */
