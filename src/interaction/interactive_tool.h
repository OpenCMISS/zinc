/*******************************************************************************
FILE : interactive_tool.h

LAST MODIFIED : 18 July 2000

DESCRIPTION :
Active CMGUI tools should create a wrapper Interactive_tool that supplies a
consistent way of choosing which one is to receive input from a given input
device, and listing which ones are active.
Only certain such tools will be capable of receiving input events from scene
viewer/mouse input and other devices, but many more will follow and change the
content of the global selections and objects with text input.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (INTERACTIVE_TOOL_H)
#define INTERACTIVE_TOOL_H

#include "general/image_utilities.h"
#include "general/list.h"
#include "general/manager.h"
#include "general/object.h"
#include "graphics/colour.h"
#include "interaction/interactive_event.h"

/*
Global types
------------
*/
struct Graphics_buffer;
struct Graphics_window;

struct Interactive_tool;
/*******************************************************************************
LAST MODIFIED : 10 April 2000

DESCRIPTION :
Wrapper object for CMGUI tools giving them a name, a consistent interface for
sending input device events to, and can be put in a manager.
The contents of this object are private.
==============================================================================*/
struct MANAGER(Interactive_tool);

typedef void Interactive_event_handler(void *device_id,
	struct Interactive_event *event,void *user_data,
	struct Graphics_buffer *graphics_buffer);
typedef int Interactive_tool_bring_up_dialog_function(void *user_data,
	struct Graphics_window *graphics_window);
typedef void Interactive_tool_reset_function(void *user_data);
typedef int Interactive_tool_destroy_tool_data_function(
   void **interactive_tool_data_address);
typedef int Interactive_tool_copy_function(
	void *destination_tool, void *source_tool,
	struct MANAGER(Interactive_tool) *destination_tool_manager);

DECLARE_LIST_TYPES(Interactive_tool);
DECLARE_MANAGER_TYPES(Interactive_tool);

/*
Global functions
----------------
*/

struct Interactive_tool *CREATE(Interactive_tool)(const char *name,const char *display_name,
	const char *tool_type_name,
	Interactive_event_handler *interactive_event_handler,
	Interactive_tool_bring_up_dialog_function *bring_up_dialog_function,
	Interactive_tool_reset_function* reset_function,
   Interactive_tool_destroy_tool_data_function *destroy_tool_data_function,
   Interactive_tool_copy_function *copy_tool_function,
	void *tool_data);
/*******************************************************************************
LAST MODIFIED : 25 February 2008

DESCRIPTION :
Creates an Interactive_tool with the given <name> and <icon>. If an
<interactive_event_handler> is supplied it is called to pass on any input
events to the interactive_tool, with the <tool_data> passed as the third
parameter.
==============================================================================*/

int DESTROY(Interactive_tool)(
	struct Interactive_tool **interactive_tool_address);
/*******************************************************************************
LAST MODIFIED : 10 April 2000

DESCRIPTION :
Destroys the Interactive_tool.
==============================================================================*/

PROTOTYPE_OBJECT_FUNCTIONS(Interactive_tool);
PROTOTYPE_LIST_FUNCTIONS(Interactive_tool);
PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(Interactive_tool,name,const char *);
PROTOTYPE_GET_OBJECT_NAME_FUNCTION(Interactive_tool);

PROTOTYPE_MANAGER_COPY_FUNCTIONS(Interactive_tool,name,const char *);
PROTOTYPE_MANAGER_FUNCTIONS(Interactive_tool);
PROTOTYPE_MANAGER_IDENTIFIER_FUNCTIONS(Interactive_tool,name,const char *);

const char *Interactive_tool_get_display_name(
	struct Interactive_tool *interactive_tool);
/*******************************************************************************
LAST MODIFIED : 11 May 2000

DESCRIPTION :
Returns the display_name of the tool = what should be shown on widget/window
descriptions of the tool. The actual name generally uses underscore _ characters
instead of spaces for more terse commands.
Up to calling function to DEALLOCATE the returned copy of the display_name.
==============================================================================*/

const char *Interactive_tool_get_tool_type_name(
	struct Interactive_tool *interactive_tool);
/*******************************************************************************
LAST MODIFIED : 6 October 2000

DESCRIPTION :
Returns the static pointer to the string which belongs to the tool type.  Do
not DEALLOCATE this pointer.
==============================================================================*/

int Interactive_tool_handle_interactive_event(
	struct Interactive_tool *interactive_tool,void *device_id,
	struct Interactive_event *interactive_event,
	struct Graphics_buffer *graphics_buffer);
/*******************************************************************************
LAST MODIFIED : 18 November 2005

DESCRIPTION :
Passes the <interactive_event> from <device_id> to the tool wrapped by the
<interactive_tool> object.
==============================================================================*/

int Interactive_tool_copy(struct Interactive_tool *destination_interactive_tool,
   struct Interactive_tool *source_interactive_tool,
	struct MANAGER(Interactive_tool) *destination_tool_manager);
/*******************************************************************************
LAST MODIFIED : 29 March 2007

DESCRIPTION :
Copies the settings of the <source_interactive_tool> into either the
<destination_interactive_tool> if that is set or creates a copy of the tool
if <destination_interactive_tool> is NULL and adds that to the manager.
==============================================================================*/

int Interactive_tool_create_copy_iterator(struct Interactive_tool *interactive_tool,
   void *interactive_tool_manager_void);
/*******************************************************************************
LAST MODIFIED : 4 April 2007

DESCRIPTION :
An iterator which copies the <interactive_tool>, creating a new one of the same
type by calling Interactive_tool_copy and then adds this tool to the manager
pointed to by <interactive_tool_manager_void>.
==============================================================================*/

int Interactive_tool_bring_up_dialog(struct Interactive_tool *interactive_tool, struct Graphics_window *graphics_window);
/*******************************************************************************
LAST MODIFIED : 18 July 2000

DESCRIPTION :
If the bring_up_dialog function is defined for <interactive_tool> calls it to
bring up the dialog for changing its settings.
==============================================================================*/

void Interactive_tool_reset(struct Interactive_tool *interactive_tool);
/*******************************************************************************
LAST MODIFIED : 25 February 2008

DESCRIPTION :
Resets any current edits in the tool.
Call on old tool when a new tool is activated, esp. on asynchronous tool change.
==============================================================================*/

const char **interactive_tool_manager_get_tool_names(
	struct MANAGER(Interactive_tool) *interactive_tool_manager,
	int *number_of_tools,struct Interactive_tool *current_interactive_tool,
	const char **current_tool_name);
/*******************************************************************************
LAST MODIFIED : 13 June 2000

DESCRIPTION :
Returns an array of strings containing the names of the tools - suitable for
choosing in a text command. On success, also returns <*number_of_tools>,
and in <current_tool_name> the pointer to the tool_names
string for <current_interactive_tool>, or the first one if it is not found.
Up to calling function to deallocate the returned array AND the strings in it.
==============================================================================*/

#endif /* !defined (INTERACTIVE_TOOL_H) */
