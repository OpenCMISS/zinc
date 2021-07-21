/*******************************************************************************
FILE : computed_field_derivative_image_filter.h

LAST MODIFIED : 31 May 2001

DESCRIPTION :
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (computed_field_derivative_image_filter_H)
#define computed_field_derivative_image_filter_H

#include "opencmiss/zinc/field.h"

/***************************************************************************//**
 * Creates a field performing ITK derivative image filter on scalar source field
 * image. Sets number of components to same number as <source_field>.
 */
struct Computed_field *cmzn_fieldmodule_create_field_imagefilter_derivative(
	struct cmzn_fieldmodule *field_module,
	struct Computed_field *source_field, int order, int direction);

int cmzn_field_get_type_derivative_image_filter(struct Computed_field *field,
	struct Computed_field **source_field, int *order, int *direction);
/*******************************************************************************
LAST MODIFIED : 18 Nov 2006

DESCRIPTION :
If the field is of type FIELD_DERIVATIVEIMAGEFILTER, the source_field and derivative_image_filter
used by it are returned - otherwise an error is reported.
WARNING: To be deprecated.
==============================================================================*/

#endif /* !defined (computed_field_derivative_image_filter_H) */
