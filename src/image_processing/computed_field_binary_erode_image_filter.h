/*******************************************************************************
FILE : computed_field_binary_erode_image_filter.h

LAST MODIFIED : 13 July 2007

DESCRIPTION :
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (COMPUTED_FIELD_BINARYERODEFILTER_H)
#define COMPUTED_FIELD_BINARYERODEFILTER_H

#include "opencmiss/zinc/field.h"

int cmzn_field_get_type_binary_erode_image_filter(struct Computed_field *field,
	struct Computed_field **source_field,
	int *radius, double *erode_value);
/*******************************************************************************
LAST MODIFIED : 13 July 2007

DESCRIPTION :
If the field is of type COMPUTED_FIELD_BINARYERODEFILTER, the source_field and binary_erode_image_filter
used by it are returned - otherwise an error is reported.
==============================================================================*/

#endif /* !defined (COMPUTED_FIELD_BINARYERODEFILTER_H) */
