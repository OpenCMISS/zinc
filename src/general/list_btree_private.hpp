/***************************************************************************//**
 * FILE : indexed_list_btree_private.hpp
 *
 * Implementation of indexed lists using cmzn_btree template.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (INDEXED_LIST_BTREE_PRIVATE_HPP)
#define INDEXED_LIST_BTREE_PRIVATE_HPP

#include "general/cmiss_btree.hpp"
extern "C" {
#include "general/list.h"
}

#define CMZN_BTREE( object_type )  cmzn_set_ ## object_type

#define FULL_DECLARE_INDEXED_LIST_BTREE_TYPE( object_type )

/*
Global functions
----------------
*/

#define DECLARE_CREATE_INDEXED_LIST_BTREE_FUNCTION( object_type ) \
PROTOTYPE_CREATE_LIST_FUNCTION(object_type) \
{ \
	return reinterpret_cast<struct LIST(object_type) *>(CMZN_BTREE(object_type)::create_independent()); \
}

#define DECLARE_CREATE_RELATED_INDEXED_LIST_BTREE_FUNCTION( object_type ) \
PROTOTYPE_CREATE_RELATED_LIST_FUNCTION(object_type) \
{ \
	CMZN_BTREE(object_type) *other = reinterpret_cast<CMZN_BTREE(object_type) *>(other_list); \
	return reinterpret_cast<struct LIST(object_type) *>(other->create_related()); \
}

#define DECLARE_DESTROY_INDEXED_LIST_BTREE_FUNCTION( object_type ) \
PROTOTYPE_DESTROY_LIST_FUNCTION(object_type) \
{ \
	if (list_address) \
	{ \
		CMZN_BTREE(object_type) **cmiss_btree_address = \
			reinterpret_cast<CMZN_BTREE(object_type) **>(list_address); \
		return CMZN_BTREE(object_type)::deaccess(cmiss_btree_address); \
	} \
	return 0; \
}

#define DECLARE_COPY_INDEXED_LIST_BTREE_FUNCTION( object_type ) \
PROTOTYPE_COPY_LIST_FUNCTION(object_type) \
{ \
	if (target_list && source_list) \
	{ \
		CMZN_BTREE(object_type) *target = reinterpret_cast<CMZN_BTREE(object_type) *>(target_list); \
		CMZN_BTREE(object_type) *source = reinterpret_cast<CMZN_BTREE(object_type) *>(source_list); \
		*target = *source; \
		if (target->size() == source->size()) \
			return 1; \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"COPY_LIST(" #object_type ").  Invalid argument(s)"); \
	} \
	return 0; \
}

#define DECLARE_REMOVE_OBJECT_FROM_INDEXED_LIST_BTREE_FUNCTION( object_type ) \
PROTOTYPE_REMOVE_OBJECT_FROM_LIST_FUNCTION(object_type) \
{ \
	if (object && list) \
	{ \
		CMZN_BTREE(object_type) *cmiss_btree = reinterpret_cast<CMZN_BTREE(object_type) *>(list); \
		return cmiss_btree->erase(object); \
	} \
	display_message(ERROR_MESSAGE, \
		"REMOVE_OBJECT_FROM_LIST(" #object_type ").  Invalid argument(s)"); \
	return 0; \
}

#define DECLARE_REMOVE_OBJECTS_FROM_INDEXED_LIST_BTREE_THAT_FUNCTION( object_type ) \
PROTOTYPE_REMOVE_OBJECTS_FROM_LIST_THAT_FUNCTION(object_type) \
{ \
	if (conditional && list) \
	{ \
		CMZN_BTREE(object_type) *cmiss_btree = reinterpret_cast<CMZN_BTREE(object_type) *>(list); \
		return cmiss_btree->erase_conditional(conditional, user_data); \
	} \
	display_message(ERROR_MESSAGE, \
		"REMOVE_OBJECTS_FROM_LIST_THAT" #object_type ").  Invalid argument(s)"); \
	return 0; \
}

#define DECLARE_REMOVE_ALL_OBJECTS_FROM_INDEXED_LIST_BTREE_FUNCTION( object_type ) \
PROTOTYPE_REMOVE_ALL_OBJECTS_FROM_LIST_FUNCTION(object_type) \
{ \
	if (list) \
	{ \
		CMZN_BTREE(object_type) *cmiss_btree = reinterpret_cast<CMZN_BTREE(object_type) *>(list); \
		cmiss_btree->clear(); \
		return 1; \
	} \
	display_message(ERROR_MESSAGE, \
		"REMOVE_ALL_OBJECTS_FROM_LIST" #object_type ").  Invalid argument(s)"); \
	return 0; \
}

#define DECLARE_ADD_OBJECT_TO_INDEXED_LIST_BTREE_FUNCTION( object_type ) \
PROTOTYPE_ADD_OBJECT_TO_LIST_FUNCTION( object_type ) \
{ \
	if (object && list) \
	{ \
		CMZN_BTREE(object_type) *cmiss_btree = reinterpret_cast<CMZN_BTREE(object_type) *>(list); \
		return cmiss_btree->insert(object); \
	} \
	display_message(ERROR_MESSAGE, \
		"ADD_OBJECT_TO_LIST(" #object_type ").  Invalid argument(s)"); \
	return 0; \
}

#define DECLARE_NUMBER_IN_INDEXED_LIST_BTREE_FUNCTION( object_type ) \
PROTOTYPE_NUMBER_IN_LIST_FUNCTION(object_type) \
{ \
	if (list) \
	{ \
		CMZN_BTREE(object_type) *cmiss_btree = reinterpret_cast<CMZN_BTREE(object_type) *>(list); \
		return cmiss_btree->size(); \
	} \
	display_message(ERROR_MESSAGE, \
		"NUMBER_IN_LIST(" #object_type ").  Invalid argument"); \
	return 0; \
}

#define DECLARE_IS_OBJECT_IN_INDEXED_LIST_BTREE_FUNCTION( object_type ) \
PROTOTYPE_IS_OBJECT_IN_LIST_FUNCTION(object_type) \
{ \
	if (list && object) \
	{ \
		CMZN_BTREE(object_type) *cmiss_btree = reinterpret_cast<CMZN_BTREE(object_type) *>(list); \
		return cmiss_btree->contains(object); \
	} \
	display_message(ERROR_MESSAGE, \
		"IS_OBJECT_IN_LIST(" #object_type ").  Invalid argument"); \
	return 0; \
}

#define DECLARE_FIRST_OBJECT_IN_INDEXED_LIST_BTREE_THAT_FUNCTION( object_type ) \
PROTOTYPE_FIRST_OBJECT_IN_LIST_THAT_FUNCTION(object_type) \
{ \
	if (list) \
	{ \
		CMZN_BTREE(object_type) *cmiss_btree = reinterpret_cast<CMZN_BTREE(object_type) *>(list); \
		return cmiss_btree->find_first_object_that(conditional, user_data); \
	} \
	display_message(ERROR_MESSAGE, \
		"FIRST_OBJECT_IN_LIST_THAT(" #object_type ").  Invalid argument(s)"); \
	return 0; \
}

#define DECLARE_FOR_EACH_OBJECT_IN_INDEXED_LIST_BTREE_FUNCTION( object_type ) \
PROTOTYPE_FOR_EACH_OBJECT_IN_LIST_FUNCTION(object_type) \
{ \
	if (list && iterator) \
	{ \
		CMZN_BTREE(object_type) *cmiss_btree = reinterpret_cast<CMZN_BTREE(object_type) *>(list); \
		return cmiss_btree->for_each_object(iterator, user_data); \
	} \
	display_message(ERROR_MESSAGE, \
		"FOR_EACH_OBJECT_IN_LIST(" #object_type ").  Invalid argument(s)"); \
	return 0; \
}

#define DECLARE_CREATE_INDEXED_LIST_BTREE_ITERATOR_FUNCTION( object_type , iterator_type ) \
PROTOTYPE_CREATE_LIST_ITERATOR_FUNCTION(object_type,iterator_type) \
{ \
	CMZN_BTREE(object_type) *cmiss_btree = reinterpret_cast<CMZN_BTREE(object_type) *>(list); \
	if (cmiss_btree) \
		return new iterator_type(cmiss_btree); \
	return 0; \
}

#define DECLARE_FIND_BY_IDENTIFIER_IN_INDEXED_LIST_BTREE_FUNCTION( object_type , \
	identifier , identifier_type ) \
PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(object_type,identifier, \
	identifier_type) \
{ \
	if (list) \
	{ \
		CMZN_BTREE(object_type) *cmiss_btree = reinterpret_cast<CMZN_BTREE(object_type) *>(list); \
		return cmiss_btree->find_object_by_identifier(identifier); \
	} \
	display_message(ERROR_MESSAGE, "FIND_BY_IDENTIFIER_IN_LIST(" #object_type \
		"," #identifier ").  Invalid argument"); \
	return 0; \
}

#define PROTOTYPE_INDEXED_LIST_BTREE_BEGIN_IDENTIFIER_CHANGE_FUNCTION( object_type , \
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

#define DECLARE_INDEXED_LIST_BTREE_BEGIN_IDENTIFIER_CHANGE_FUNCTION( object_type , \
	identifier ) \
PROTOTYPE_INDEXED_LIST_BTREE_BEGIN_IDENTIFIER_CHANGE_FUNCTION(object_type, \
	identifier) \
{ \
	if (list && object) \
	{ \
		CMZN_BTREE(object_type) *cmiss_btree = reinterpret_cast<CMZN_BTREE(object_type) *>(list); \
		return cmiss_btree->begin_identifier_change(object); \
	} \
	display_message(ERROR_MESSAGE, \
		"LIST_BEGIN_IDENTIFIER_CHANGE(" #object_type "," #identifier \
		").  Invalid argument(s)"); \
	return 0; \
}

#define PROTOTYPE_INDEXED_LIST_BTREE_END_IDENTIFIER_CHANGE_FUNCTION( \
	object_type , identifier ) \
void LIST_END_IDENTIFIER_CHANGE(object_type,identifier)( \
	struct LIST(object_type) *list) \
/***************************************************************************** \
Companion function to LIST_BEGIN_IDENTIFIER_CHANGE function. \
Re-adds the changed object to all the lists it was in. \
Should only be declared with manager functions. \
============================================================================*/

#define DECLARE_INDEXED_LIST_BTREE_END_IDENTIFIER_CHANGE_FUNCTION(object_type ,  \
	identifier ) \
PROTOTYPE_INDEXED_LIST_BTREE_END_IDENTIFIER_CHANGE_FUNCTION(object_type, \
	identifier) \
{ \
	if (list) \
	{ \
		CMZN_BTREE(object_type) *cmiss_btree = reinterpret_cast<CMZN_BTREE(object_type) *>(list); \
		cmiss_btree->end_identifier_change(); \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"LIST_BEGIN_IDENTIFIER_CHANGE(" #object_type "," #identifier \
			").  Invalid argument(s)"); \
	} \
}

#define LIST_BTREE_STATISTICS(object_type,list) \
{ \
	CMZN_BTREE(object_type) *cmiss_btree = reinterpret_cast<CMZN_BTREE(object_type) *>(list); \
	int stem_count = 0; \
	int leaf_count = 0; \
	int min_leaf_depth = 0; \
	int max_leaf_depth = 0; \
	double mean_leaf_depth = 0.0; \
	double mean_stem_occupancy = 0.0; \
	double mean_leaf_occupancy = 0.0; \
	cmiss_btree->get_statistics(stem_count, leaf_count, \
		min_leaf_depth, max_leaf_depth, mean_leaf_depth, \
		mean_stem_occupancy, mean_leaf_occupancy); \
	display_message(INFORMATION_MESSAGE, "  Size = %d\n", cmiss_btree->size()); \
	display_message(INFORMATION_MESSAGE, "  Stem count = %d\n", stem_count); \
	display_message(INFORMATION_MESSAGE, "  Leaf count = %d\n", leaf_count); \
	display_message(INFORMATION_MESSAGE, "  Min leaf depth = %d\n", min_leaf_depth); \
	display_message(INFORMATION_MESSAGE, "  Max leaf depth = %d\n", max_leaf_depth); \
	display_message(INFORMATION_MESSAGE, "  Mean leaf depth = %g\n", mean_leaf_depth); \
	display_message(INFORMATION_MESSAGE, "  Mean stem occupancy = %g\n", mean_stem_occupancy); \
	display_message(INFORMATION_MESSAGE, "  Mean leaf occupancy = %g\n", mean_leaf_occupancy); \
}

/* prototypes for private headers, eg. finite_element_private.h */
#define PROTOTYPE_INDEXED_LIST_BTREE_IDENTIFIER_CHANGE_FUNCTIONS( object_type , identifier ) \
PROTOTYPE_INDEXED_LIST_BTREE_BEGIN_IDENTIFIER_CHANGE_FUNCTION(object_type, identifier); \
PROTOTYPE_INDEXED_LIST_BTREE_END_IDENTIFIER_CHANGE_FUNCTION(object_type,identifier)

#define DECLARE_INDEXED_LIST_BTREE_IDENTIFIER_CHANGE_FUNCTIONS( object_type , identifier ) \
DECLARE_INDEXED_LIST_BTREE_BEGIN_IDENTIFIER_CHANGE_FUNCTION(object_type,identifier) \
DECLARE_INDEXED_LIST_BTREE_END_IDENTIFIER_CHANGE_FUNCTION(object_type,identifier)

#define DECLARE_INDEXED_LIST_BTREE_FUNCTIONS( object_type ) \
DECLARE_CREATE_INDEXED_LIST_BTREE_FUNCTION(object_type) \
DECLARE_CREATE_RELATED_INDEXED_LIST_BTREE_FUNCTION(object_type) \
DECLARE_DESTROY_INDEXED_LIST_BTREE_FUNCTION(object_type) \
DECLARE_COPY_INDEXED_LIST_BTREE_FUNCTION(object_type) \
DECLARE_REMOVE_OBJECT_FROM_INDEXED_LIST_BTREE_FUNCTION(object_type) \
DECLARE_REMOVE_OBJECTS_FROM_INDEXED_LIST_BTREE_THAT_FUNCTION(object_type) \
DECLARE_REMOVE_ALL_OBJECTS_FROM_INDEXED_LIST_BTREE_FUNCTION(object_type) \
DECLARE_ADD_OBJECT_TO_INDEXED_LIST_BTREE_FUNCTION(object_type) \
DECLARE_NUMBER_IN_INDEXED_LIST_BTREE_FUNCTION(object_type) \
DECLARE_IS_OBJECT_IN_INDEXED_LIST_BTREE_FUNCTION(object_type) \
DECLARE_FIRST_OBJECT_IN_INDEXED_LIST_BTREE_THAT_FUNCTION(object_type) \
DECLARE_FOR_EACH_OBJECT_IN_INDEXED_LIST_BTREE_FUNCTION(object_type)

#endif /* !defined (INDEXED_LIST_BTREE_PRIVATE_HPP) */
