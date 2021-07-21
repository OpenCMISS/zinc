/*******************************************************************************
FILE : callback.h

LAST MODIFIED : 23 March 2000

DESCRIPTION :
Macro definition for lists of callbacks between objects.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (CALLBACK_H)
#define CALLBACK_H
#include "general/list.h"
#include "general/object.h"

#if defined (WIN32_USER_INTERFACE)
//#define WINDOWS_LEAN_AND_MEAN
#if !defined (NOMINMAX)
#define NOMINMAX
#endif
#include <windows.h>
#define UWM_IDLE	(WM_APP + 1)
#define UWM_NETWORK     (WM_APP + 2)
#endif /* (WIN32_USER_INTERFACE) */

/*
Global Types
------------
*/

#define CMZN_CALLBACK_FUNCTION( callback_type ) callback_function_ ## callback_type

#define TYPEDEF_CMZN_CALLBACK_FUNCTION( callback_type , object_type , \
	call_data_type , callback_function_return_type ) \
typedef callback_function_return_type CMZN_CALLBACK_FUNCTION(callback_type)(object_type,call_data_type,void *)

#define CMZN_CALLBACK_FUNCTION_RETURN_TYPE( callback_type ) callback_function_return_type__ ## callback_type
	
#define DECLARE_CMZN_CALLBACK_FUNCTION_RETURN_TYPE( callback_type , callback_function_return_type ) callback_function_return_type

#define CMZN_CALLBACK_ITEM( callback_type ) callback_item_ ## callback_type

#define DECLARE_CMZN_CALLBACK_TYPE( callback_type ) \
struct CMZN_CALLBACK_ITEM(callback_type) \

/*
Global functions
----------------
*/

#define CMZN_CALLBACK_LIST_CALL( callback_type ) callback_list_call_ ## callback_type

#define PROTOTYPE_CMZN_CALLBACK_LIST_CALL_FUNCTION( callback_type , object_type , \
	call_data_type ) \
int CMZN_CALLBACK_LIST_CALL(callback_type)( \
	struct LIST(CMZN_CALLBACK_ITEM(callback_type)) *callback_list, \
	object_type object,call_data_type call_data)
/*******************************************************************************
LAST MODIFIED : 20 March 2000

DESCRIPTION :
Calls every callback in <callback_list> with <object> and <call_data>.
==============================================================================*/

#define CMZN_CALLBACK_LIST_ADD_CALLBACK( callback_type ) \
	callback_list_add_callback_ ## callback_type

#define PROTOTYPE_CMZN_CALLBACK_LIST_ADD_CALLBACK_FUNCTION(callback_type ) \
int CMZN_CALLBACK_LIST_ADD_CALLBACK(callback_type)( \
	struct LIST(CMZN_CALLBACK_ITEM(callback_type)) *callback_list, \
	CMZN_CALLBACK_FUNCTION(callback_type) *function,void *user_data)
/*******************************************************************************
LAST MODIFIED : 20 March 2000

DESCRIPTION :
Adds a callback = <function> + <user_data> to the end of <callback_list>.
==============================================================================*/

#define CMZN_CALLBACK_LIST_ADD_CALLBACK_TO_FRONT( callback_type ) \
	callback_list_add_callback_to_front_ ## callback_type

#define PROTOTYPE_CMZN_CALLBACK_LIST_ADD_CALLBACK_TO_FRONT_FUNCTION(callback_type ) \
int CMZN_CALLBACK_LIST_ADD_CALLBACK_TO_FRONT(callback_type)( \
	struct LIST(CMZN_CALLBACK_ITEM(callback_type)) *callback_list, \
	CMZN_CALLBACK_FUNCTION(callback_type) *function,void *user_data)
/*******************************************************************************
LAST MODIFIED : 11 September 2007

DESCRIPTION :
Adds a callback = <function> + <user_data> to the front of <callback_list>.
==============================================================================*/

#define CMZN_CALLBACK_LIST_REMOVE_CALLBACK( callback_type ) \
	callback_list_remove_callback_ ## callback_type

#define PROTOTYPE_CMZN_CALLBACK_LIST_REMOVE_CALLBACK_FUNCTION(callback_type) \
int CMZN_CALLBACK_LIST_REMOVE_CALLBACK(callback_type)( \
	struct LIST(CMZN_CALLBACK_ITEM(callback_type)) *callback_list, \
	CMZN_CALLBACK_FUNCTION(callback_type) *function,void *user_data)
/*******************************************************************************
LAST MODIFIED : 20 March 2000

DESCRIPTION :
Removes a callback = <function> + <user_data> from <callback_list>.
=============================================================================*/

#define DECLARE_CMZN_CALLBACK_TYPES( callback_type , object_type , call_data_type , callback_function_return_type ) \
	TYPEDEF_CMZN_CALLBACK_FUNCTION(callback_type,object_type,call_data_type,callback_function_return_type); \
DECLARE_CMZN_CALLBACK_TYPE(callback_type); \
DECLARE_LIST_TYPES(CMZN_CALLBACK_ITEM(callback_type))

/* note that most modules will not need to prototype functions in headers as
	 will use the following in wrappers or module functions */
#define PROTOTYPE_CMZN_CALLBACK_FUNCTIONS( callback_type , object_type , \
	call_data_type ) \
PROTOTYPE_LIST_FUNCTIONS(CMZN_CALLBACK_ITEM(callback_type)); \
PROTOTYPE_ADD_OBJECT_TO_FRONT_OF_LIST_FUNCTION(CMZN_CALLBACK_ITEM(callback_type)); \
PROTOTYPE_CMZN_CALLBACK_LIST_CALL_FUNCTION(callback_type,object_type, \
	call_data_type); \
PROTOTYPE_CMZN_CALLBACK_LIST_ADD_CALLBACK_FUNCTION(callback_type); \
PROTOTYPE_CMZN_CALLBACK_LIST_REMOVE_CALLBACK_FUNCTION(callback_type)

#endif /* !defined (CALLBACK_H) */
