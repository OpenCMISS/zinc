/*******************************************************************************
FILE : computed_field_fastMarchingImageFilter.h

LAST MODIFIED : 31 May 2001

DESCRIPTION :
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (COMPUTED_FIELD_FAST_MARCHING_IMAGE_FILTER_H)
#define COMPUTED_FIELD_FAST_MARCHING_IMAGE_FILTER_H

#include "opencmiss/zinc/field.h"

int cmzn_field_get_type_fast_marching_image_filter(struct Computed_field *field,
	struct Computed_field **source_field,double *stopping_value,
  int *num_seed_points, int *dimension, double **seed_points, double **seed_values, 
  int **output_size);
/*******************************************************************************
LAST MODIFIED : 18 Nov 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_FAST_MARCHING_IMAGE_FILTER, the source_field and fast_marching_image_filter
used by it are returned - otherwise an error is reported.
==============================================================================*/


/***************************************************************************//**
 * Creates a field performing ITK fast marching image filter on scalar source field
 * image. Sets number of components to same number as <source_field>.
 */
cmzn_field_id cmzn_fieldmodule_create_field_imagefilter_fast_marching(
	cmzn_fieldmodule_id field_module, cmzn_field_id source_field,
	double stopping_value, int num_seed_points, int dimension,
	const double *seed_points, const double *seed_values, const int *output_size);

#endif /* !defined (COMPUTED_FIELD_FAST_MARCHING_IMAGE_FILTER_H) */
