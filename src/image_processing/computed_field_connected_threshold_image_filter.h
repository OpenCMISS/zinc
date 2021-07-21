/*******************************************************************************
FILE : computed_field_connected_threshold_filter.h

LAST MODIFIED : 16 July 2007

DESCRIPTION :
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (COMPUTED_FIELD_CONNECTED_THRESHOLD_IMAGE_FILTER_H)
#define COMPUTED_FIELD_CONNECTED_THRESHOLD_IMAGE_FILTER_H

#include "opencmiss/zinc/field.h"

int cmzn_field_get_type_connected_threshold_image_filter(struct Computed_field *field,
	struct Computed_field **source_field,
  double *lower_threshold, double *upper_threshold, double *replace_value,
	int *num_seed_points, int *dimension, double **seed_points);
/*******************************************************************************
LAST MODIFIED : 16 July 2007

DESCRIPTION :
If the field is of type COMPUTED_FIELD_CONNECTED_THRESHOLD_IMAGE_FILTER, the source_field and connected_threshold_image_filter
used by it are returned - otherwise an error is reported.
==============================================================================*/

#endif /* !defined (COMPUTED_FIELD_CONNECTED_THRESHOLD_IMAGE_FILTER_H) */
