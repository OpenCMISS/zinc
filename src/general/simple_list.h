/*******************************************************************************
FILE : simple_list.h

LAST MODIFIED : 17 June 1996

DESCRIPTION :
A simple_list is just like a list except that it is simple - ie it does not
contain pointers to things that need to be destroyed individually.
This file contains the types, prototypes and definitions required to use
simple_list's.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (SIMPLE_LIST_H)
#define SIMPLE_LIST_H
#include "general/list.h"
/*
Macros
------
*/
#define PROTOTYPE_SIMPLE_LIST_CREATE_OBJECT_FUNCTION( object_type ) \
struct object_type *CREATE(object_type)(void) \
/***************************************************************************** \
LAST MODIFIED : 24 September 1995 \
\
DESCRIPTION : \
Creates the object by allocating the structure. \
==============================================================================*/

#define DECLARE_SIMPLE_LIST_CREATE_OBJECT_FUNCTION( object_type ) \
PROTOTYPE_SIMPLE_LIST_CREATE_OBJECT_FUNCTION(object_type) \
{ \
	struct object_type *object; \
\
	ENTER(CREATE(object_type)); \
	if (!ALLOCATE(object,struct object_type,1)) \
	{ \
		display_message(ERROR_MESSAGE, \
			"CREATE(" #object_type ").  Could not allocate structure"); \
	} \
	LEAVE; \
\
	return (object); \
} /* CREATE(object_type) */

#define PROTOTYPE_SIMPLE_LIST_DESTROY_OBJECT_FUNCTION( object_type ) \
int DESTROY(object_type)(struct object_type **object_address) \
/***************************************************************************** \
LAST MODIFIED : 2 November 1995 \
\
DESCRIPTION : \
Destroys the object. \
==============================================================================*/

#define DECLARE_SIMPLE_LIST_DESTROY_OBJECT_FUNCTION( object_type ) \
PROTOTYPE_SIMPLE_LIST_DESTROY_OBJECT_FUNCTION(object_type) \
{ \
	int return_code; \
\
	ENTER(DESTROY(object_type)); \
	if ((object_address)&&(*object_address)) \
	{ \
		/* free the memory */ \
		DEALLOCATE(*object_address); \
		return_code = 1; \
	} \
	else \
	{ \
		return_code = 0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* DESTROY(object_type) */

#define PROTOTYPE_SIMPLE_LIST_ACCESS_OBJECT_FUNCTION( object_type ) \
struct object_type *ACCESS(object_type)(struct object_type *object) \
/***************************************************************************** \
LAST MODIFIED : 24 September 1995 \
\
DESCRIPTION : \
Returns the <object>. \
==============================================================================*/

#define DECLARE_SIMPLE_LIST_ACCESS_OBJECT_FUNCTION( object_type ) \
PROTOTYPE_SIMPLE_LIST_ACCESS_OBJECT_FUNCTION(object_type) \
{ \
	ENTER(ACCESS(object_type)); \
	if (!object) \
	{ \
		display_message(ERROR_MESSAGE, \
			"ACCESS(" #object_type ").  Invalid argument"); \
	} \
	LEAVE; \
\
	return (object); \
} /* ACCESS(object_type) */

#define PROTOTYPE_SIMPLE_LIST_DEACCESS_OBJECT_FUNCTION( object_type ) \
int DEACCESS(object_type)(struct object_type **object_address) \
/***************************************************************************** \
LAST MODIFIED : 2 November 1995 \
\
DESCRIPTION : \
Destroys the object. \
==============================================================================*/

#define DECLARE_SIMPLE_LIST_DEACCESS_OBJECT_FUNCTION( object_type ) \
PROTOTYPE_SIMPLE_LIST_DEACCESS_OBJECT_FUNCTION(object_type) \
{ \
	int return_code; \
\
	ENTER(DEACCESS(object_type)); \
	if (object_address&&(*object_address)) \
	{ \
		return_code=DESTROY(object_type)(object_address); \
	} \
	else \
	{ \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* DEACCESS(object_type) */

#define PROTOTYPE_SIMPLE_LIST_OBJECT_FUNCTIONS( object_type ) \
PROTOTYPE_SIMPLE_LIST_CREATE_OBJECT_FUNCTION(object_type); \
PROTOTYPE_SIMPLE_LIST_DESTROY_OBJECT_FUNCTION(object_type); \
PROTOTYPE_SIMPLE_LIST_ACCESS_OBJECT_FUNCTION(object_type); \
PROTOTYPE_SIMPLE_LIST_DEACCESS_OBJECT_FUNCTION(object_type)

#define DECLARE_SIMPLE_LIST_OBJECT_FUNCTIONS( object_type ) \
DECLARE_SIMPLE_LIST_CREATE_OBJECT_FUNCTION(object_type) \
DECLARE_SIMPLE_LIST_DESTROY_OBJECT_FUNCTION(object_type) \
DECLARE_SIMPLE_LIST_ACCESS_OBJECT_FUNCTION(object_type) \
DECLARE_SIMPLE_LIST_DEACCESS_OBJECT_FUNCTION(object_type)
#endif

