/*******************************************************************************
FILE : computed_field_gradientMagnitudeRecursiveGaussianImageFilter.h

LAST MODIFIED : 31 May 2001

DESCRIPTION :
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (COMPUTED_FIELD_GRADIENT_MAGNITUDE_RECURSIVE_GAUSSIAN_IMAGE_FILTER_H)
#define COMPUTED_FIELD_GRADIENT_MAGNITUDE_RECURSIVE_GAUSSIAN_IMAGE_FILTER_H

#include "opencmiss/zinc/field.h"

int cmzn_field_get_type_gradient_magnitude_recursive_gaussian_image_filter(struct Computed_field *field,
	struct Computed_field **source_field, double *sigma);
/*******************************************************************************
LAST MODIFIED : 18 Nov 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_GRADIENT_MAGNITUDE_RECURSIVE_GAUSSIAN_IMAGE_FILTER, the source_field and gradient_magnitude_recursive_gaussian_image_filter
used by it are returned - otherwise an error is reported.
==============================================================================*/

#endif /* !defined (COMPUTED_FIELD_GRADIENT_MAGNITUDE_RECURSIVE_GAUSSIAN_IMAGE_FILTER_H) */
