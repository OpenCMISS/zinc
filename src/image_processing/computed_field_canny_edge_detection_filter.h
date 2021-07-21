/*******************************************************************************
FILE : computed_field_canny_edge_detection_filter.h

LAST MODIFIED : 9 September 2006

DESCRIPTION :
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (computed_field_canny_edge_detection_filter_H)
#define computed_field_canny_edge_detection_filter_H

#include "opencmiss/zinc/field.h"

int cmzn_field_get_type_canny_edge_detection_image_filter(struct Computed_field *field,
      struct Computed_field **source_field, double *variance, double *maximumError,
      double *upperThreshold, double *lowerThreshold);
/*******************************************************************************
LAST MODIFIED : 9 September 2006

DESCRIPTION :
If the field is of type computed_field_canny_edge_detection_filter, the source_field and canny_edge_detection_image_filter
used by it are returned - otherwise an error is reported.
==============================================================================*/

#endif /* !defined (computed_field_canny_edge_detection_filter_H) */
