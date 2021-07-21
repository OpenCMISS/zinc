/*******************************************************************************
FILE : colour.c

LAST MODIFIED : 25 November 1999

DESCRIPTION :
Colour structures and support code.
???DB.  I'm not sure that this needs to be abstracted/formalized.
???DB.  Should convert to CREATE and DESTROY
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stdio.h>

#include "general/debug.h"
#include "graphics/colour.h"
#include "general/message.h"

/*
Global functions
----------------
*/
struct Colour *create_Colour(COLOUR_PRECISION red,COLOUR_PRECISION green,
	COLOUR_PRECISION blue)
/*******************************************************************************
LAST MODIFIED : 27 August 1996

DESCRIPTION :
Allocates memory and assigns field for a colour.
==============================================================================*/
{
	struct Colour *colour;

	ENTER(create_Colour);
	if (ALLOCATE(colour,struct Colour,1))
	{
		colour->red=red;
		colour->green=green;
		colour->blue=blue;
		colour->alpha=0.0;
	}
	else
	{
		display_message(ERROR_MESSAGE,"create_Colour.  Insufficient memory");
	}
	LEAVE;

	return (colour);
} /* create_Colour */

void destroy_Colour(struct Colour **colour_address)
/*******************************************************************************
LAST MODIFIED : 27 August 1996

DESCRIPTION :
Frees the memory for the colour and sets <*colour_address> to NULL.
==============================================================================*/
{
	ENTER(destroy_Colour);
	/* checking arguments */
	if (colour_address)
	{
		if (*colour_address)
		{
			DEALLOCATE(*colour_address);
			*colour_address=(struct Colour *)NULL;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"destroy_Colour.  Invalid argument");
	}
	LEAVE;
} /* destroy_Colour */

