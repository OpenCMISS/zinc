/*******************************************************************************
FILE : callback_private.h

LAST MODIFIED : 23 March 2000

DESCRIPTION :
Macro definition for lists of callbacks between objects.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "general/callback.h"
#include "general/debug.h"
#include "general/list_private.h"
#include "general/message.h"

/*
Module types
------------
*/

#define FULL_DECLARE_CMZN_CALLBACK_TYPE( callback_type ) \
struct CMZN_CALLBACK_ITEM(callback_type) \
/***************************************************************************** \
LAST MODIFIED : 20 March 2000 \
\
DESCRIPTION : \
A callback. \
============================================================================*/ \
{ \
   CMZN_CALLBACK_FUNCTION(callback_type) *function; \
	void *user_data; \
	int access_count; \
} /* CMZN_CALLBACK_ITEM(callback_type) */

#if ! defined (SHORT_NAMES)
#define CMZN_CALLBACK_OBJECT_AND_CALL_DATA( callback_type ) \
	callback_object_and_call_data_ ## callback_type
#else
#define CMZN_CALLBACK_OBJECT_AND_CALL_DATA( callback_type ) \
  cbocd_ ## callback_type
#endif

#define FULL_DECLARE_CMZN_CALLBACK_OBJECT_AND_CALL_DATA_TYPE( callback_type , \
  object_type , call_data_type) \
struct CMZN_CALLBACK_OBJECT_AND_CALL_DATA(callback_type) \
/***************************************************************************** \
LAST MODIFIED : 20 March 2000 \
\
DESCRIPTION : \
Additional data - object and call_data structures sent with the callback. \
============================================================================*/ \
{ \
  object_type object; \
  call_data_type call_data; \
} /* struct CMZN_CALLBACK_OBJECT_AND_CALL_DATA(callback_type) */

/*
Module functions
----------------
*/

#define DEFINE_CREATE_CMZN_CALLBACK_FUNCTION( callback_type ) \
struct CMZN_CALLBACK_ITEM(callback_type) *CREATE(CMZN_CALLBACK_ITEM(callback_type))( \
	CMZN_CALLBACK_FUNCTION(callback_type) *function,void *user_data) \
/***************************************************************************** \
LAST MODIFIED : 20 March 2000 \
\
DESCRIPTION : \
Creates a callback containing <function> and <user_data>. \
============================================================================*/ \
{ \
	struct CMZN_CALLBACK_ITEM(callback_type) *callback; \
\
	ENTER(CREATE(CMZN_CALLBACK_ITEM(callback_type))); \
	if (function) \
	{ \
		if (ALLOCATE(callback,struct CMZN_CALLBACK_ITEM(callback_type),1)) \
		{ \
			callback->function=function; \
			callback->user_data=user_data; \
			callback->access_count=0; \
		} \
		else \
		{ \
			display_message(ERROR_MESSAGE, \
				"CREATE(CMZN_CALLBACK_ITEM(" #callback_type ")).  Not enough memory"); \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"CREATE(CMZN_CALLBACK_ITEM(" #callback_type ")).  Invalid argument(s)"); \
		callback=(struct CMZN_CALLBACK_ITEM(callback_type) *)NULL; \
	} \
	LEAVE; \
\
	return (callback); \
} /* CREATE(CMZN_CALLBACK_ITEM(callback_type)) */

#define DEFINE_DESTROY_CMZN_CALLBACK_FUNCTION( callback_type ) \
	int DESTROY(CMZN_CALLBACK_ITEM(callback_type))( \
	struct CMZN_CALLBACK_ITEM(callback_type) **callback_address) \
/***************************************************************************** \
LAST MODIFIED : 20 March 2000 \
\
DESCRIPTION : \
Destroys the callback at <*callback_address>. \
============================================================================*/ \
{ \
  int return_code; \
\
	ENTER(DESTROY(CMZN_CALLBACK_ITEM(callback_type))); \
	if (callback_address&&(*callback_address)) \
	{ \
		DEALLOCATE(*callback_address); \
		return_code=1; \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"DESTROY(CMZN_CALLBACK_ITEM(" #callback_type ")).  Invalid argument(s)"); \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* DESTROY(CMZN_CALLBACK_ITEM(callback_type)) */

#if ! defined (SHORT_NAMES)
#define CMZN_CALLBACK_CALL( callback_type ) callback_call_ ## callback_type
#else
#define CMZN_CALLBACK_CALL( callback_type ) cbc_ ## callback_type
#endif

#define DEFINE_CMZN_CALLBACK_CALL_FUNCTIONvoid( callback_type ) \
	int CMZN_CALLBACK_CALL(callback_type)( \
	struct CMZN_CALLBACK_ITEM(callback_type) *callback,void *callback_data_void) \
/***************************************************************************** \
LAST MODIFIED : 11 September 2007 \
\
DESCRIPTION : \
Sends <callback> with the object and call_data in <callback_data>. \
Version for callback_function_return_type == void.
============================================================================*/ \
{ \
	int return_code; \
  struct CMZN_CALLBACK_OBJECT_AND_CALL_DATA(callback_type) *callback_data; \
\
	ENTER(CMZN_CALLBACK_CALL(callback_type)); \
	if (callback&&callback->function&&(NULL != (callback_data= \
		(struct CMZN_CALLBACK_OBJECT_AND_CALL_DATA(callback_type) *) \
				callback_data_void))&&callback_data->object) \
	{ \
		(callback->function)(callback_data->object, \
			callback_data->call_data,callback->user_data); \
		return_code=1; \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"CMZN_CALLBACK_CALL(" #callback_type ").  Invalid argument(s)"); \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* CMZN_CALLBACK_CALL(callback_type) */ \

#define DEFINE_CMZN_CALLBACK_CALL_FUNCTIONint( callback_type ) \
	int CMZN_CALLBACK_CALL(callback_type)( \
	struct CMZN_CALLBACK_ITEM(callback_type) *callback,void *callback_data_void) \
/***************************************************************************** \
LAST MODIFIED : 11 September 2007 \
\
DESCRIPTION : \
Sends <callback> with the object and call_data in <callback_data>. \
Version for callback_function_return_type == int.
============================================================================*/ \
{ \
	int return_code; \
  struct CMZN_CALLBACK_OBJECT_AND_CALL_DATA(callback_type) *callback_data; \
\
	ENTER(CMZN_CALLBACK_CALL(callback_type)); \
	if (callback&&callback->function&&(NULL != (callback_data= \
		(struct CMZN_CALLBACK_OBJECT_AND_CALL_DATA(callback_type) *) \
				callback_data_void))&&callback_data->object) \
	{ \
		return_code=(callback->function)(callback_data->object, \
			callback_data->call_data,callback->user_data); \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"CMZN_CALLBACK_CALL(" #callback_type ").  Invalid argument(s)"); \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* CMZN_CALLBACK_CALL(callback_type) */ \

#if ! defined (SHORT_NAMES)
#define CMZN_CALLBACK_MATCHES( callback_type ) callback_matches_ ## callback_type
#else
#define CMZN_CALLBACK_MATCHES( callback_type ) cbm_ ## callback_type
#endif

#define DEFINE_CMZN_CALLBACK_MATCHES_FUNCTION( callback_type ) \
	int CMZN_CALLBACK_MATCHES(callback_type)( \
	struct CMZN_CALLBACK_ITEM(callback_type) *callback,void *other_callback_void) \
/***************************************************************************** \
LAST MODIFIED : 20 March 2000 \
\
DESCRIPTION : \
Returns true if the \
Sends <callback> with the object and call_data in <callback_data>. \
============================================================================*/ \
{ \
	int return_code; \
  struct CMZN_CALLBACK_ITEM(callback_type) *other_callback; \
\
	ENTER(CMZN_CALLBACK_MATCHES(callback_type)); \
	if ((callback&& (NULL != (other_callback= \
					(struct CMZN_CALLBACK_ITEM(callback_type) *)other_callback_void)))) \
	{ \
		return_code=((callback->function == other_callback->function)&& \
			(callback->user_data == other_callback->user_data)); \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"CMZN_CALLBACK_MATCHES(" #callback_type ").  Invalid argument(s)"); \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* CMZN_CALLBACK_MATCHES(callback_type) */ \

/*
Global functions
----------------
*/

#define DEFINE_CMZN_CALLBACK_LIST_CALL_FUNCTION( callback_type , object_type , \
	call_data_type ) \
PROTOTYPE_CMZN_CALLBACK_LIST_CALL_FUNCTION(callback_type,object_type,call_data_type)\
/***************************************************************************** \
LAST MODIFIED : 20 March 2000 \
\
DESCRIPTION : \
Calls every callback in <callback_list> with <object> and <call_data>. \
============================================================================*/ \
{ \
	int return_code; \
	struct CMZN_CALLBACK_OBJECT_AND_CALL_DATA(callback_type) callback_data; \
\
	ENTER(CMZN_CALLBACK_LIST_CALL(callback_type)); \
	if (callback_list&&object) \
	{ \
		callback_data.object=object; \
		callback_data.call_data=call_data; \
		return_code=FOR_EACH_OBJECT_IN_LIST(CMZN_CALLBACK_ITEM(callback_type))( \
			CMZN_CALLBACK_CALL(callback_type),(void *)&callback_data,callback_list); \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"CMZN_CALLBACK_LIST_CALL(" #callback_type ").  Invalid argument(s)"); \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* CMZN_CALLBACK_LIST_CALL(callback_type) */

#define DEFINE_CMZN_CALLBACK_LIST_ADD_CALLBACK_FUNCTION( callback_type ) \
PROTOTYPE_CMZN_CALLBACK_LIST_ADD_CALLBACK_FUNCTION(callback_type) \
/***************************************************************************** \
LAST MODIFIED : 20 May 2003 \
\
DESCRIPTION : \
Adds a callback = <function> + <user_data> to end of <callback_list>. \
============================================================================*/ \
{ \
	int return_code; \
	struct CMZN_CALLBACK_ITEM(callback_type) *callback; \
\
	ENTER(CMZN_CALLBACK_LIST_ADD_CALLBACK(callback_type)); \
	if (callback_list && function) \
	{ \
		if (NULL != (callback = \
				CREATE(CMZN_CALLBACK_ITEM(callback_type))(function, user_data))) \
		{ \
			if (FIRST_OBJECT_IN_LIST_THAT(CMZN_CALLBACK_ITEM(callback_type))( \
				CMZN_CALLBACK_MATCHES(callback_type), (void *)callback, \
				callback_list)) \
			{ \
				/* we already have an equivalent callback, so destroy new one */ \
				DESTROY(CMZN_CALLBACK_ITEM(callback_type))(&callback); \
				return_code = 1; \
			} \
			else \
			{ \
				if (ADD_OBJECT_TO_LIST(CMZN_CALLBACK_ITEM(callback_type))( \
					callback,callback_list)) \
				{ \
					return_code=1; \
				} \
				else \
				{ \
					display_message(ERROR_MESSAGE,"CMZN_CALLBACK_LIST_ADD_CALLBACK(" \
						#callback_type ").  Could not add callback to list"); \
					DESTROY(CMZN_CALLBACK_ITEM(callback_type))(&callback); \
					return_code=0; \
				} \
			} \
		} \
		else \
		{ \
			display_message(ERROR_MESSAGE,"CMZN_CALLBACK_LIST_ADD_CALLBACK(" \
				#callback_type ").  Could not create callback"); \
			return_code=0; \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE,"CMZN_CALLBACK_LIST_ADD_CALLBACK(" \
			#callback_type ").  Invalid argument(s)"); \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* CMZN_CALLBACK_LIST_ADD_CALLBACK(callback_type) */

#define DEFINE_CMZN_CALLBACK_LIST_ADD_CALLBACK_TO_FRONT_FUNCTION( callback_type ) \
PROTOTYPE_CMZN_CALLBACK_LIST_ADD_CALLBACK_TO_FRONT_FUNCTION(callback_type) \
/***************************************************************************** \
LAST MODIFIED : 11 September 2007 \
\
DESCRIPTION : \
Adds a callback = <function> + <user_data> to front of <callback_list>. \
============================================================================*/ \
{ \
	int return_code; \
	struct CMZN_CALLBACK_ITEM(callback_type) *callback; \
\
	ENTER(CMZN_CALLBACK_LIST_ADD_CALLBACK_TO_FRONT(callback_type)); \
	if (callback_list && function) \
	{ \
		if (NULL != (callback = \
				CREATE(CMZN_CALLBACK_ITEM(callback_type))(function, user_data))) \
		{ \
			if (FIRST_OBJECT_IN_LIST_THAT(CMZN_CALLBACK_ITEM(callback_type))( \
				CMZN_CALLBACK_MATCHES(callback_type), (void *)callback, \
				callback_list)) \
			{ \
				/* we already have an equivalent callback, so destroy new one */ \
				DESTROY(CMZN_CALLBACK_ITEM(callback_type))(&callback); \
				return_code = 1; \
			} \
			else \
			{ \
				if (ADD_OBJECT_TO_FRONT_OF_LIST(CMZN_CALLBACK_ITEM(callback_type))( \
					callback,callback_list)) \
				{ \
					return_code=1; \
				} \
				else \
				{ \
					display_message(ERROR_MESSAGE,"CMZN_CALLBACK_LIST_ADD_CALLBACK_TO_FRONT(" \
						#callback_type ").  Could not add callback to list"); \
					DESTROY(CMZN_CALLBACK_ITEM(callback_type))(&callback); \
					return_code=0; \
				} \
			} \
		} \
		else \
		{ \
			display_message(ERROR_MESSAGE,"CMZN_CALLBACK_LIST_ADD_CALLBACK_TO_FRONT(" \
				#callback_type ").  Could not create callback"); \
			return_code=0; \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE,"CMZN_CALLBACK_LIST_ADD_CALLBACK_TO_FRONT(" \
			#callback_type ").  Invalid argument(s)"); \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* CMZN_CALLBACK_LIST_ADD_CALLBACK_TO_FRONT(callback_type) */

#define DEFINE_CMZN_CALLBACK_LIST_REMOVE_CALLBACK_FUNCTION( callback_type ) \
PROTOTYPE_CMZN_CALLBACK_LIST_REMOVE_CALLBACK_FUNCTION(callback_type) \
/***************************************************************************** \
LAST MODIFIED : 20 March 2000 \
\
DESCRIPTION : \
Removes a callback = <function> + <user_data> from <callback_list>. \
============================================================================*/ \
{ \
	int return_code = 0; \
	struct CMZN_CALLBACK_ITEM(callback_type) callback,*existing_callback = NULL; \
\
	ENTER(CMZN_CALLBACK_LIST_REMOVE_CALLBACK(callback_type)); \
	if (callback_list&&function) \
	{ \
		callback.function=function; \
		callback.user_data=user_data; \
		if (NULL != (existing_callback= \
			FIRST_OBJECT_IN_LIST_THAT(CMZN_CALLBACK_ITEM(callback_type))( \
				CMZN_CALLBACK_MATCHES(callback_type),(void *)&callback,callback_list))) \
		{ \
			if (existing_callback->access_count == 1) \
			{ \
				if (REMOVE_OBJECT_FROM_LIST(CMZN_CALLBACK_ITEM(callback_type))( \
					existing_callback,callback_list)) \
				{ \
					return_code=1; \
				} \
				else \
				{ \
					display_message(ERROR_MESSAGE,"CMZN_CALLBACK_LIST_REMOVE_CALLBACK(" \
						#callback_type ").  Could not remove callback from list"); \
					return_code=0; \
				} \
			} \
			else \
			{ \
				DEACCESS(CMZN_CALLBACK_ITEM(callback_type))(&existing_callback); \
				return_code = 1; \
			} \
		} \
		else \
		{ \
			display_message(ERROR_MESSAGE,"CMZN_CALLBACK_LIST_REMOVE_CALLBACK(" \
				#callback_type ").  Could not find callback in list"); \
			return_code=0; \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE,"CMZN_CALLBACK_LIST_REMOVE_CALLBACK(" \
			#callback_type ").  Invalid argument(s)"); \
		return_code=0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* CMZN_CALLBACK_LIST_REMOVE_CALLBACK(callback_type) */

#define FULL_DECLARE_CMZN_CALLBACK_TYPES( callback_type , object_type , call_data_type ) \
	FULL_DECLARE_CMZN_CALLBACK_TYPE(callback_type); \
	FULL_DECLARE_LIST_TYPE(CMZN_CALLBACK_ITEM(callback_type)); \
	FULL_DECLARE_CMZN_CALLBACK_OBJECT_AND_CALL_DATA_TYPE(callback_type,object_type, call_data_type)

#define DEFINE_CMZN_CALLBACK_MODULE_FUNCTIONS( callback_type , callback_function_return_type ) \
	DEFINE_CREATE_CMZN_CALLBACK_FUNCTION(callback_type) \
	DEFINE_DESTROY_CMZN_CALLBACK_FUNCTION(callback_type) \
	DECLARE_OBJECT_FUNCTIONS(CMZN_CALLBACK_ITEM(callback_type)) \
	DEFINE_CMZN_CALLBACK_CALL_FUNCTION ## callback_function_return_type(callback_type) \
	DEFINE_CMZN_CALLBACK_MATCHES_FUNCTION(callback_type)

#define DEFINE_CMZN_CALLBACK_FUNCTIONS( callback_type , object_type , \
		call_data_type ) \
	DECLARE_LIST_FUNCTIONS(CMZN_CALLBACK_ITEM(callback_type)) \
	DECLARE_ADD_OBJECT_TO_FRONT_OF_LIST_FUNCTION(CMZN_CALLBACK_ITEM(callback_type)) \
	DEFINE_CMZN_CALLBACK_LIST_CALL_FUNCTION( callback_type , object_type , \
		call_data_type ) \
	DEFINE_CMZN_CALLBACK_LIST_ADD_CALLBACK_FUNCTION(callback_type) \
	DEFINE_CMZN_CALLBACK_LIST_ADD_CALLBACK_TO_FRONT_FUNCTION(callback_type) \
	DEFINE_CMZN_CALLBACK_LIST_REMOVE_CALLBACK_FUNCTION(callback_type)

