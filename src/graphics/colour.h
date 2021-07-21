/*******************************************************************************
FILE : colour.h

LAST MODIFIED : 18 June 1996

DESCRIPTION :
Colour structures and support code.
???DB.  I'm not sure that colour needs to be abstracted/formalized.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined( COLOUR_H )
#define COLOUR_H

#include "opencmiss/zinc/zincconfigure.h"

#define COLOUR_VECTOR(colour_struct_ptr) \
((COLOUR_PRECISION *)colour_struct_ptr)
/*
Global types
------------
*/
struct Colour
/*******************************************************************************
LAST MODIFIED : 9 November 1994

DESCRIPTION :
==============================================================================*/
{
	COLOUR_PRECISION blue, green, red, alpha;
}; /* struct Colour */

/*
Global functions
----------------
*/
struct Colour *create_Colour(COLOUR_PRECISION red,COLOUR_PRECISION green,
	COLOUR_PRECISION blue);
/*******************************************************************************
LAST MODIFIED : 11 November 1994

DESCRIPTION :
Allocates memory and assigns field for a colour.
==============================================================================*/

void destroy_Colour(struct Colour **colour_address);
/*******************************************************************************
LAST MODIFIED : 11 November 1994

DESCRIPTION :
Frees the memory for the colour and sets <*colour_address> to NULL.
==============================================================================*/

#endif
