/*******************************************************************************
FILE : computed_field_image_resample.h

LAST MODIFIED : 7 March 2007

DESCRIPTION :
Field that changes the native resolution of a computed field.
Image processing fields use the native resolution to determine their image size.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (COMPUTED_FIELD_IMAGE_RESAMPLE_H)
#define COMPUTED_FIELD_IMAGE_RESAMPLE_H

struct cmzn_field_image_resample;
typedef struct cmzn_field_image_resample * cmzn_field_image_resample_id;

/***************************************************************************//**
 * Create a field which resamples the image in source field, and overrides the
 * sizes that will be used for subsequent image processing operations.
 * The texture_coordinate field could also be overridden.  The minimums and
 * maximums are not implemented at all, which would allow a windowing, and could
 * also be overridden here.
 * 
 * @param field_module  Region field module which will own new field.
 * @return  Newly created field.
 */
Computed_field *cmzn_fieldmodule_create_field_image_resample(
	struct cmzn_fieldmodule *field_module,
	struct Computed_field *source_field, int dimension, int *sizes);

ZINC_C_INLINE cmzn_field_id cmzn_field_image_resample_base_cast(
	cmzn_field_image_resample_id image_resample)
{
	return (cmzn_field_id)(image_resample);
}

int cmzn_field_image_resample_destroy(cmzn_field_image_resample_id *image_resample_address);

cmzn_field_image_resample_id cmzn_field_cast_image_resample(cmzn_field_id field);

int cmzn_field_image_resample_set_input_coordinates_minimum(cmzn_field_image_resample_id image_resample,
	int dimension, const double *input_coordinates_minimum);

int cmzn_field_image_resample_set_input_coordinates_maximum(cmzn_field_image_resample_id image_resample,
	int dimension, const double *input_coordinates_maximum);

int cmzn_field_image_resample_set_lookup_coordinates_minimum(cmzn_field_image_resample_id image_resample,
	int dimension, const double *lookup_coordinates_minimum);

int cmzn_field_image_resample_set_lookup_coordinates_maximum(cmzn_field_image_resample_id image_resample,
	int dimension, const double *lookup_coordinates_maximum);

int cmzn_field_image_resample_get_input_coordinates_minimum(cmzn_field_image_resample_id image_resample,
	int dimension, double *input_coordinates_minimum);

int cmzn_field_image_resample_get_input_coordinates_maximum(cmzn_field_image_resample_id image_resample,
	int dimension, double *input_coordinates_maximum);

int cmzn_field_image_resample_get_lookup_coordinates_minimum(cmzn_field_image_resample_id image_resample,
	int dimension, double *lookup_coordinates_minimum);

int cmzn_field_image_resample_get_lookup_coordinates_maximum(cmzn_field_image_resample_id image_resample,
	int dimension, double *lookup_coordinates_maximum);

#endif /* !defined (COMPUTED_FIELD_IMAGE_RESAMPLE_H) */
