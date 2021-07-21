/*******************************************************************************
FILE : computed_field_histogram_image_filter.h

LAST MODIFIED : 25 March 2008

DESCRIPTION :
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (COMPUTED_FIELD_HISTOGRAM_IMAGE_FILTER_H)
#define COMPUTED_FIELD_HISTOGRAM_IMAGE_FILTER_H

#include "opencmiss/zinc/field.h"

int cmzn_field_get_type_histogram_image_filter(struct Computed_field *field,
	struct Computed_field **source_field, int **numberOfBins, double *marginalScale,
	double **histogramMinimum, double **histogramMaximum);
/*******************************************************************************
If the field is of type COMPUTED_FIELD_HISTOGRAM_IMAGE_FILTER, the source_field and histogram_image_filter
used by it are returned - otherwise an error is reported.
==============================================================================*/

#endif /* !defined (COMPUTED_FIELD_HISTOGRAM_IMAGE_FILTER_H) */
