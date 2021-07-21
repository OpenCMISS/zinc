/*******************************************************************************
FILE : computed_field_binary_threshold_image_filter.h

LAST MODIFIED : 16 May 2008

DESCRIPTION :
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (COMPUTED_FIELD_BINARY_THRESHOLD_IMAGE_FILTER_H)
#define COMPUTED_FIELD_BINARY_THRESHOLD_IMAGE_FILTER_H

#include "opencmiss/zinc/field.h"
#include "opencmiss/zinc/fieldimageprocessing.h"

/*****************************************************************************//**
 * If the field is of type COMPUTED_FIELD_BINARY_THRESHOLD_IMAGE_FILTER,
 * the source_field and thresholds used by it are returned -
 * otherwise an error is reported.
 *
 * @return Return code indicating success (1) or failure (0)
*/
int cmzn_field_get_type_binary_threshold_image_filter(struct Computed_field *field,
	struct Computed_field **source_field, double *lower_threshold,
	double *upper_threshold);

#endif /* !defined (COMPUTED_FIELD_BINARY_THRESHOLD_IMAGE_FILTER_H) */
