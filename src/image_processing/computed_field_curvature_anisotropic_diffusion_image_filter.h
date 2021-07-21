/*******************************************************************************
FILE : computed_field_curvature_anisotropic_diffusion_image_filter.h

LAST MODIFIED : 15 Dec 2006

DESCRIPTION :
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (computed_field_curvature_anisotropic_diffusion_image_filter_H)
#define computed_field_curvature_anisotropic_diffusion_image_filter_H

#include "opencmiss/zinc/field.h"

int cmzn_field_get_type_curvature_anisotropic_diffusion_image_filter(struct Computed_field *field,
	struct Computed_field **source_field, double *timeStep, double *conductance, int *numIterations);
/*******************************************************************************
LAST MODIFIED : 18 Nov 2006

DESCRIPTION :
If the field is of type computed_field_curvature_anisotropic_diffusion_image_filter, the source_field and curvature_anisotropic_diffusion_image_filter
used by it are returned - otherwise an error is reported.
==============================================================================*/

#endif /* !defined (computed_field_curvature_anisotropic_diffusion_image_filter_H) */
