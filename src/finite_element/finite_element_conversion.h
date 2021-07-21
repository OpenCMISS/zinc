/*******************************************************************************
FILE : finite_element_conversion.h

LAST MODIFIED : 4 March 2003

DESCRIPTION :
Functions for building IGES information from finite elements and exporting
to file.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (FINITE_ELEMENT_CONVERSION_H)
#define FINITE_ELEMENT_CONVERSION_H

#include "general/enumerator.h"
#include "general/value.h"
#include "opencmiss/zinc/types/regionid.h"
#include "opencmiss/zinc/types/fieldid.h"
#include "finite_element/finite_element_constants.hpp"

/*
Global types
------------
*/

/**************************************************************************//**
 * Mode and basis of finite element conversion.
 */
enum Convert_finite_elements_mode
{
	/* Convert the surface elements to topologically square cubic hermite 2D elements. */
	CONVERT_TO_FINITE_ELEMENTS_HERMITE_2D_PRODUCT,
	CONVERT_TO_FINITE_ELEMENTS_BICUBIC,
	CONVERT_TO_FINITE_ELEMENTS_TRILINEAR,
	CONVERT_TO_FINITE_ELEMENTS_TRIQUADRATIC,
	CONVERT_TO_FINITE_ELEMENTS_TRICUBIC,
	CONVERT_TO_FINITE_ELEMENTS_MODE_UNSPECIFIED
};

PROTOTYPE_ENUMERATOR_FUNCTIONS(Convert_finite_elements_mode)

struct Element_refinement
{
	int count[MAXIMUM_ELEMENT_XI_DIMENSIONS];
};

/*
Global functions
----------------
*/

/**
 * Convert the finite_elements in source_region to new finite_elements
 * in destination_region according to the mode defining the fields
 * in source fields.
 */
int finite_element_conversion(cmzn_region_id source_region,
	cmzn_region_id destination_region,
	enum Convert_finite_elements_mode mode,
	int number_of_source_fields, cmzn_field_id *source_fields,
	struct Element_refinement refinement, FE_value tolerance);

#endif /* !defined (FINITE_ELEMENT_CONVERSION_H) */
