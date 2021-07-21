/*******************************************************************************
FILE : math.h

LAST MODIFIED : 26 June 2002

DESCRIPTION :
Defines the finite function for UNIX and WIN32_SYSTEM
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (MATH_H)
#define MATH_H

#include "opencmiss/zinc/zincconfigure.h"

#if defined (SGI)
/* For finite so that we can check for Nans which some compilers seem
	to accept as valid in an fscanf */
#include <ieeefp.h>
#endif /* defined (SGI) */
#if defined (WIN32_SYSTEM)
#include <float.h>
#define finite _finite
#endif /* defined (WIN32_SYSTEM) */

#endif /* !defined (MATH_H) */
