/*******************************************************************************
FILE : interactive_event.h

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
#if !defined (INTERACTIVE_EVENT_H)
#define INTERACTIVE_EVENT_H

#include "general/object.h"
#include "interaction/interaction_volume.h"
#include "graphics/scene.hpp"

/*
Global constants
----------------
*/
#define INTERACTIVE_EVENT_MODIFIER_SHIFT   1
#define INTERACTIVE_EVENT_MODIFIER_CONTROL 2
#define INTERACTIVE_EVENT_MODIFIER_ALT     4

/*
Global types
------------
*/

enum Interactive_event_type
{
	INTERACTIVE_EVENT_BUTTON_PRESS,
	INTERACTIVE_EVENT_MOTION_NOTIFY,
	INTERACTIVE_EVENT_BUTTON_RELEASE
};

struct Interactive_event;
/*******************************************************************************
LAST MODIFIED : 10 April 2000

DESCRIPTION :
Structure describing an interactive event (eg. button press at point in space).
ACCESS this object for as long as you need to keep it; it is not modifiable.
The contents of this object are private.
==============================================================================*/

/*
Global functions
----------------
*/

struct Interactive_event *CREATE(Interactive_event)(
	enum Interactive_event_type type,int button_number,int input_modifier,
	struct Interaction_volume *interaction_volume, cmzn_scene *scene);
/*******************************************************************************
LAST MODIFIED : 6 May 2005

DESCRIPTION :
Creates an Interactive_event. Note that the scene is optional.
Also, both this object and the <interaction_volume> may be accessed to keep as long as
necessary - they are not modifiable.
==============================================================================*/

int DESTROY(Interactive_event)(
	struct Interactive_event **interactive_event_address);
/*******************************************************************************
LAST MODIFIED : 10 April 2000

DESCRIPTION :
Destroys the Interactive_event.
==============================================================================*/

PROTOTYPE_OBJECT_FUNCTIONS(Interactive_event);

int Interactive_event_get_button_number(
	struct Interactive_event *interactive_event);
/*******************************************************************************
LAST MODIFIED : 10 April 2000

DESCRIPTION :
Returns the number of the button involved in the INTERACTIVE_EVENT_BUTTON_PRESS
or INTERACTIVE_EVENT_BUTTON_RELEASE event. Return value is arbitrary for other
events.
==============================================================================*/

int Interactive_event_get_input_modifier(
	struct Interactive_event *interactive_event);
/*******************************************************************************
LAST MODIFIED : 10 April 2000

DESCRIPTION :
Returns flags indicating the state of the shift, control and alt keys during the
even - use logical OR with INTERACTIVE_EVENT_MODIFIER_SHIFT etc.
==============================================================================*/

struct Interaction_volume *Interactive_event_get_interaction_volume(
	struct Interactive_event *interactive_event);
/*******************************************************************************
LAST MODIFIED : 10 April 2000

DESCRIPTION :
Returns the centre/volume of space involved in the <interactive_event>. Note
that the returned volume may be accessed and kept beyond the lifespan of the
event itself since it is not modifiable.
==============================================================================*/

cmzn_scene *Interactive_event_get_scene(
	struct Interactive_event *interactive_event);
/*******************************************************************************
LAST MODIFIED : 10 April 2000

DESCRIPTION :
Returns the optional scene associated with the interaction_volume for the
<interactive_event>. Can then be used for picking.
==============================================================================*/

enum Interactive_event_type Interactive_event_get_type(
	struct Interactive_event *interactive_event);
/*******************************************************************************
LAST MODIFIED : 10 April 2000

DESCRIPTION :
Returns whether the event is of type INTERACTIVE_EVENT_BUTTON_PRESS,
INTERACTIVE_EVENT_MOTION_NOTIFY, INTERACTIVE_EVENT_BUTTON_RELEASE.
==============================================================================*/

#endif /* !defined (INTERACTIVE_EVENT_H) */
