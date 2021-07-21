/*******************************************************************************
FILE : integration.h

LAST MODIFIED : 26 December 2002

DESCRIPTION :
Structures and functions for integerating a computed field over a group of
elements.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (INTEGRATION_H)
#define INTEGRATION_H

#include "computed_field/computed_field.h"
#include "finite_element/finite_element.h"
#include "general/object.h"

/*
Global types
------------
*/
struct Integration_scheme;
/*******************************************************************************
LAST MODIFIED : 26 December 2002

DESCRIPTION :
An object which when given an element will return the weights and abscissae to
be used when integrating over the element.

???DB.  cm has weights and abscissae for each basis.  A computed field may use
	FE fields with different bases for the same element - if weights and abscissae
	were stored with basis it wouldn't be clear which to use
???DB.  May want to have many ways of setting the weights and abscissae for
	elements eg shape, basis for a particular field.  Start with always same
==============================================================================*/

/*
Global functions
----------------
*/
struct Integration_scheme *CREATE(Integration_scheme)(char *name,
	struct FE_basis *basis);
/*******************************************************************************
LAST MODIFIED : 26 December 2002

DESCRIPTION :
Creates an integration scheme with the given <name> that will integrate the
<basis> exactly.  Unless further information is added, the created scheme will
return the weights and abscissae for <basis> for every element.  The <basis>
also defines the dimension of the integration scheme.
==============================================================================*/

int DESTROY(Integration_scheme)(struct Integration_scheme **scheme_address);
/*******************************************************************************
LAST MODIFIED : 26 December 2002

DESCRIPTION :
Frees memory/deaccess objects in scheme at <*scheme_address>.
==============================================================================*/

int integrate(struct Computed_field *field, struct FE_region *domain,
	struct Integration_scheme *scheme,FE_value time,FE_value *result);
/*******************************************************************************
LAST MODIFIED : 26 December 2002

DESCRIPTION :
Calculates the integral of the <field> over the <domain> at the specified <time>
using the given <scheme>.  The <result> array needs to be big enough to hold one
value for each component of the <field>.
==============================================================================*/

#endif /* !defined (INTEGRATION_H) */
