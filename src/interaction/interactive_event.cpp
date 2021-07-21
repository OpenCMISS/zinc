/*******************************************************************************
FILE : interactive_event.cpp

LAST MODIFIED : 6 May 2005

DESCRIPTION :
Structure describing an interactive event containing:
- type: press, move or release
- button number
- modifier keys: shift, ctrl, alt
- Interaction_volume
- Scene
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "general/debug.h"
#include "graphics/scene.hpp"
#include "interaction/interactive_event.h"
#include "general/message.h"

/*
Module types
------------
*/

struct Interactive_event
/*******************************************************************************
LAST MODIFIED : 10 April 2000

DESCRIPTION :
Structure describing an interactive event (eg. button press at point in space).
ACCESS this object for as long as you need to keep it; it is not modifiable.
==============================================================================*/
{
	enum Interactive_event_type type;
	int button_number;
	/* flags indicating the state of the shift, control and alt keys - use
		 logical OR with INTERACTIVE_EVENT_MODIFIER_SHIFT etc. */
	int input_modifier;
	struct Interaction_volume *interaction_volume;
	cmzn_scene_id scene;
	int access_count;
}; /* struct Interactive_event */

/*
Global functions
----------------
*/

struct Interactive_event *CREATE(Interactive_event)(
	enum Interactive_event_type type,int button_number,int input_modifier,
	struct Interaction_volume *interaction_volume,cmzn_scene_id scene)
/*******************************************************************************
LAST MODIFIED : 6 May 2005

DESCRIPTION :
Creates an Interactive_event. Note that the scene is optional.
Also, both this object and the <interaction_volume> may be accessed to keep as long as
necessary - they are not modifiable.
==============================================================================*/
{
	struct Interactive_event *interactive_event = NULL;

	ENTER(CREATE(Interactive_event));
	if (interaction_volume)
	{
		if (ALLOCATE(interactive_event,struct Interactive_event,1))
		{
			interactive_event->type=type;
			interactive_event->button_number=button_number;
			interactive_event->input_modifier=input_modifier;
			interactive_event->interaction_volume=
				ACCESS(Interaction_volume)(interaction_volume);
			if (scene)
			{
				interactive_event->scene=cmzn_scene_access(scene);
			}
			else
			{
				interactive_event->scene=(cmzn_scene_id)NULL;
			}
			interactive_event->access_count=0;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"CREATE(Interactive_event).  Not enough memory");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CREATE(Interactive_event).  Invalid argument(s)");
	}
	LEAVE;

	return (interactive_event);
} /* CREATE(Interactive_event) */

int DESTROY(Interactive_event)(
	struct Interactive_event **interactive_event_address)
/*******************************************************************************
LAST MODIFIED : 10 April 2000

DESCRIPTION :
Destroys the Interactive_event.
==============================================================================*/
{
	int return_code;
	struct Interactive_event *interactive_event;

	ENTER(DESTROY(Interactive_event));
	if (interactive_event_address&&
		(interactive_event= *interactive_event_address))
	{
		if (0==interactive_event->access_count)
		{
			DEACCESS(Interaction_volume)(&(interactive_event->interaction_volume));
			if (interactive_event->scene)
			{
				cmzn_scene_destroy(&(interactive_event->scene));
			}
			DEALLOCATE(*interactive_event_address);
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"DESTROY(Interactive_event).  Non-zero access count!");
			*interactive_event_address=(struct Interactive_event *)NULL;
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(Interactive_event).  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Interactive_event) */

DECLARE_OBJECT_FUNCTIONS(Interactive_event)

int Interactive_event_get_button_number(
	struct Interactive_event *interactive_event)
/*******************************************************************************
LAST MODIFIED : 10 April 2000

DESCRIPTION :
Returns the number of the button involved in the INTERACTIVE_EVENT_BUTTON_PRESS
or INTERACTIVE_EVENT_BUTTON_RELEASE event. Return value is arbitrary for other
events.
==============================================================================*/
{
	int button_number;

	ENTER(Interactive_event_get_button_number);
	if (interactive_event)
	{
		button_number=interactive_event->button_number;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Interactive_event_get_button_number.  Invalid argument(s)");
		button_number=0;
	}
	LEAVE;

	return (button_number);
} /* Interactive_event_get_button_number */

int Interactive_event_get_input_modifier(
	struct Interactive_event *interactive_event)
/*******************************************************************************
LAST MODIFIED : 10 April 2000

DESCRIPTION :
Returns flags indicating the state of the shift, control and alt keys during the
even - use logical OR with INTERACTIVE_EVENT_MODIFIER_SHIFT etc.
==============================================================================*/
{
	int input_modifier;

	ENTER(Interactive_event_get_input_modifier);
	if (interactive_event)
	{
		input_modifier=interactive_event->input_modifier;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Interactive_event_get_input_modifier.  Invalid argument(s)");
		input_modifier=0;
	}
	LEAVE;

	return (input_modifier);
} /* Interactive_event_get_input_modifier */

struct Interaction_volume *Interactive_event_get_interaction_volume(
	struct Interactive_event *interactive_event)
/*******************************************************************************
LAST MODIFIED : 10 April 2000

DESCRIPTION :
Returns the centre/volume of space involved in the <interactive_event>. Note
that the returned volume may be accessed and kept beyond the lifespan of the
event itself since it is not modifiable.
==============================================================================*/
{
	struct Interaction_volume *interaction_volume;

	ENTER(Interactive_event_get_interaction_volume);
	if (interactive_event)
	{
		interaction_volume=interactive_event->interaction_volume;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Interactive_event_get_interaction_volume.  Invalid argument(s)");
		interaction_volume=(struct Interaction_volume *)NULL;
	}
	LEAVE;

	return (interaction_volume);
} /* Interactive_event_get_interaction_volume */

cmzn_scene_id Interactive_event_get_scene(
	struct Interactive_event *interactive_event)
/*******************************************************************************
LAST MODIFIED : 10 April 2000

DESCRIPTION :
Returns the optional scene associated with the interaction_volume for the
<interactive_event>. Can then be used for picking.
==============================================================================*/
{
	if (interactive_event)
	{
		return interactive_event->scene;
	}

	return 0;
} /* Interactive_event_get_scene */

enum Interactive_event_type Interactive_event_get_type(
	struct Interactive_event *interactive_event)
/*******************************************************************************
LAST MODIFIED : 10 April 2000

DESCRIPTION :
Returns whether the event is of type INTERACTIVE_EVENT_BUTTON_PRESS,
INTERACTIVE_EVENT_MOTION_NOTIFY, INTERACTIVE_EVENT_BUTTON_RELEASE.
==============================================================================*/
{
	enum Interactive_event_type type;

	ENTER(Interactive_event_get_type);
	if (interactive_event)
	{
		type=interactive_event->type;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Interactive_event_get_type.  Invalid argument(s)");
		type=INTERACTIVE_EVENT_BUTTON_PRESS;
	}
	LEAVE;

	return (type);
} /* Interactive_event_get_type */


