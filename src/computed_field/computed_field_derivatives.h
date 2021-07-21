/*******************************************************************************
FILE : computed_field_derivatives.h

LAST MODIFIED : 1 November 2000

DESCRIPTION :
Implements computed_fields for calculating various derivative quantities such
as derivatives w.r.t. Xi, gradient, curl, divergence etc.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (COMPUTED_FIELD_DERIVATIVES_H)
#define COMPUTED_FIELD_DERIVATIVES_H

#include "opencmiss/zinc/fieldderivatives.h"

int cmzn_field_derivative_get_xi_index(cmzn_field_id field);

#endif /* !defined (COMPUTED_FIELD_DERIVATIVES_H) */
