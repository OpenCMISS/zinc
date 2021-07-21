/*******************************************************************************
FILE : object.h

LAST MODIFIED : 29 January 2003

DESCRIPTION :
Macros for prototyping standard object functions and declaring standard object
functions.

TO DO :
16 May 1997 (After making select into a template class)
----------
1 Split into private and public
2 Adding name/identifier ? (ie objects always have an identifier)
3 Move copy/modify's from manager ?
4 Should objects have a type label ?

3 December 1997
---------------
1 Have to do something to make sure that objects aren't destroyed twice.  Check
	if access_count<0 ?
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (OBJECT_H)
#define OBJECT_H

#include <string.h>
/* this is needed for the default copy object method, which uses memcpy */

/*
Macros
------
*/
#if ! defined (SHORT_NAMES)
#define CREATE_( object_type )  create_ ## object_type
#else
#define CREATE_( object_type )  c ## object_type
#endif
#define CREATE( object_type )  CREATE_(object_type)
	/*???DB.  CREATE is also defined in /usr/lib/X11/extensions/XI.h */

#if ! defined (SHORT_NAMES)
#define DESTROY_( object_type )  destroy_ ## object_type
#else
#define DESTROY_( object_type )  d ## object_type
#endif
#define DESTROY( object_type )  DESTROY_(object_type)

#define PROTOTYPE_DEFAULT_DESTROY_OBJECT_FUNCTION( object_type ) \
int DESTROY(object_type)(struct object_type **object_address) \
/***************************************************************************** \
LAST MODIFIED : 24 September 1995 \
\
DESCRIPTION : \
Destroys the object.  Note:  will not function correctly if the object \
contains fields that must be destroyed separately.  Sets *object_address to \
NULL \
==============================================================================*/

#define DECLARE_DEFAULT_DESTROY_OBJECT_FUNCTION( object_type ) \
PROTOTYPE_DEFAULT_DESTROY_OBJECT_FUNCTION(object_type) \
{ \
	int return_code; \
\
	ENTER(DESTROY(object_type)); \
	if (object_address&&(*object_address)) \
	{ \
		DEALLOCATE(*object_address); \
		*object_address = (struct object_type *)NULL; \
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

#if ! defined (SHORT_NAMES)
#define COPY_( object_type )  copy_ ## object_type
#else
#define COPY_( object_type )  o ## object_type
#endif
#define COPY( object_type )  COPY_(object_type)

#define PROTOTYPE_COPY_OBJECT_FUNCTION( object_type ) \
int COPY(object_type)(struct object_type *destination, \
	struct object_type *source) \
/***************************************************************************** \
LAST MODIFIED : 18 January 1996 \
\
DESCRIPTION : \
Copies the object contents from source to destination. \
==============================================================================*/

#define DECLARE_DEFAULT_COPY_OBJECT_FUNCTION( object_type ) \
PROTOTYPE_COPY_OBJECT_FUNCTION(object_type) \
/***************************************************************************** \
LAST MODIFIED : 18 January 1996 \
\
DESCRIPTION : \
Copies the object contents from source to destination.  This just does an \
element by element copy, and will not function correctly if the object \
contains pointers to fields that must be copied separately. \
============================================================================*/ \
{ \
	int return_code,save_access_count; \
\
	ENTER(COPY(object_type)); \
	return_code=1; \
	save_access_count=destination->access_count; \
	memcpy(destination,source,sizeof(struct object_type)); \
	destination->access_count=save_access_count; \
	LEAVE; \
\
	return (return_code); \
} /* COPY(object_type) */

#if ! defined (SHORT_NAMES)
#define GET_NAME_( object_type )  get_name_ ## object_type
#else
#define GET_NAME_( object_type )  gn ## object_type
#endif
#define GET_NAME( object_type )  GET_NAME_(object_type)

#define PROTOTYPE_GET_OBJECT_NAME_FUNCTION( object_type ) \
int GET_NAME(object_type)(struct object_type *object, \
	char **name_ptr) \
/***************************************************************************** \
LAST MODIFIED : 23 October 1997 \
\
DESCRIPTION : \
Forms a string out of the objects identifier. Used to get the name of objects \
whose members are private. Needed for creating choose, select and other dialog \
macros. Up to the calling routine to deallocate the returned char string! \
???RC Specify identifier as macro argument? More than one identifier possible? \
==============================================================================*/

#define DECLARE_DEFAULT_GET_OBJECT_NAME_FUNCTION( object_type ) \
PROTOTYPE_GET_OBJECT_NAME_FUNCTION(object_type) \
/***************************************************************************** \
LAST MODIFIED : 23 October 1997 \
\
DESCRIPTION : \
Forms a string out of the objects identifier. Used to get the name of objects \
whose members are private. Needed for creating choose, select and other dialog \
macros. Up to the calling routine to deallocate the returned char string! \
This version assumes the object identifier is "char *name". \
============================================================================*/ \
{ \
	int return_code; \
 \
	ENTER(GET_NAME(object_type)); \
	if (object&&name_ptr) \
	{ \
		if (ALLOCATE(*name_ptr,char,(unsigned long int)strlen(object->name)+1)) \
		{ \
			strcpy(*name_ptr,object->name); \
			return_code=1; \
		} \
		else \
		{ \
			display_message(ERROR_MESSAGE, \
				"GET_NAME(" #object_type ").  Could not allocate space for name"); \
			return_code=0; \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"GET_NAME(" #object_type ").  Invalid argument(s)"); \
		return_code=0; \
	} \
	LEAVE; \
	\
	return (return_code); \
} /* GET_NAME(object_type) */

#if ! defined (SHORT_NAMES)
#define ACCESS_( object_type )  access_ ## object_type
#else
#define ACCESS_( object_type )  a ## object_type
#endif
#define ACCESS( object_type )  ACCESS_(object_type)

#define PROTOTYPE_ACCESS_OBJECT_FUNCTION( object_type ) \
struct object_type *ACCESS(object_type)(struct object_type *object) \
/***************************************************************************** \
LAST MODIFIED : 24 September 1995 \
\
DESCRIPTION : \
Increases the <access_count> for the <object> by 1.  Returns the <object>. \
==============================================================================*/

#define DECLARE_ACCESS_OBJECT_FUNCTION( object_type ) \
PROTOTYPE_ACCESS_OBJECT_FUNCTION(object_type) \
{ \
	ENTER(ACCESS(object_type)); \
	if (object) \
	{ \
		(object->access_count)++; \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"ACCESS(" #object_type ").  Invalid argument"); \
	} \
	LEAVE; \
\
	return (object); \
} /* ACCESS(object_type) */

#if ! defined (SHORT_NAMES)
#define DEACCESS_( object_type )  deaccess_ ## object_type
#else
#define DEACCESS_( object_type )  s ## object_type
#endif
#define DEACCESS( object_type )  DEACCESS_(object_type)

#define PROTOTYPE_DEACCESS_OBJECT_FUNCTION( object_type ) \
int DEACCESS(object_type)(struct object_type **object_address) \
/***************************************************************************** \
LAST MODIFIED : 14 October 2002 \
\
DESCRIPTION : \
Decreases the <access_count> of the object by 1. If the access_count becomes \
less than or equal to 0, the object is destroyed. \
In all cases, sets <*object_address> to NULL. \
Note that the REACCESS function will also destroy objects if it reduces their \
access_count below 1. \
==============================================================================*/

#define DECLARE_DEACCESS_OBJECT_FUNCTION( object_type ) \
PROTOTYPE_DEACCESS_OBJECT_FUNCTION(object_type) \
{ \
	int return_code; \
	struct object_type *object; \
\
	ENTER(DEACCESS(object_type)); \
	if (object_address && (object = *object_address)) \
	{ \
		(object->access_count)--; \
		if (object->access_count <= 0) \
		{ \
			return_code = DESTROY(object_type)(object_address); \
		} \
		else \
		{ \
			return_code = 1; \
		} \
		*object_address = (struct object_type *)NULL; \
	} \
	else \
	{ \
		return_code = 0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* DEACCESS(object_type) */

#if ! defined (SHORT_NAMES)
#define REACCESS_( object_type )  reaccess_ ## object_type
#else
#define REACCESS_( object_type )  r ## object_type
#endif
#define REACCESS( object_type )  REACCESS_(object_type)

#define PROTOTYPE_REACCESS_OBJECT_FUNCTION( object_type ) \
int REACCESS(object_type)(struct object_type **object_address, \
	struct object_type *new_object) \
/***************************************************************************** \
LAST MODIFIED : 14 October 2002 \
\
DESCRIPTION : \
Makes <*object_address> point to <new_object> with safe ACCESS transfer. \
If <new_object> is not NULL, its access_count is first increased by 1. \
If <*object_address> is not NULL, its access_count is decreased by 1, and if \
it is now less than or equal to 0, the object is destroyed. \
Finally, <*object_address> is set to <new_object>.
==============================================================================*/

#define DECLARE_REACCESS_OBJECT_FUNCTION( object_type ) \
PROTOTYPE_REACCESS_OBJECT_FUNCTION(object_type) \
{ \
	int return_code; \
	struct object_type *current_object; \
\
	ENTER(REACCESS(object_type)); \
	if (object_address) \
	{ \
		return_code = 1; \
		if (new_object) \
		{ \
			/* access the new object */ \
			(new_object->access_count)++; \
		} \
		if (NULL != (current_object = *object_address)) \
		{ \
			/* deaccess the current object */ \
			(current_object->access_count)--; \
			if (current_object->access_count <= 0) \
			{ \
				DESTROY(object_type)(object_address); \
			} \
		} \
		/* point to the new object */ \
		*object_address = new_object; \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"REACCESS(" #object_type ").  Invalid argument"); \
		return_code = 0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* REACCESS(object_type) */

#define PROTOTYPE_OBJECT_FUNCTIONS( object_type ) \
PROTOTYPE_ACCESS_OBJECT_FUNCTION(object_type); \
PROTOTYPE_DEACCESS_OBJECT_FUNCTION(object_type); \
PROTOTYPE_REACCESS_OBJECT_FUNCTION(object_type)

#define DECLARE_OBJECT_FUNCTIONS( object_type ) \
DECLARE_ACCESS_OBJECT_FUNCTION(object_type) \
DECLARE_DEACCESS_OBJECT_FUNCTION(object_type) \
DECLARE_REACCESS_OBJECT_FUNCTION(object_type)

#endif
