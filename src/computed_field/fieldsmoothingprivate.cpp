/**
 * FILE : fieldsmoothingprivate.cpp
 * 
 * Implementation of field smoothing class.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "opencmiss/zinc/field.h"
#include "computed_field/computed_field_finite_element.h"
#include "computed_field/fieldsmoothingprivate.hpp"
#include "finite_element/finite_element_region.h"
#include "general/debug.h"

cmzn_fieldsmoothing::cmzn_fieldsmoothing(cmzn_fieldmodule_id fieldmodule) :
	algorithm(CMZN_FIELDSMOOTHING_ALGORITHM_AVERAGE_DELTA_DERIVATIVES_UNSCALED),
	time(0.0),
	access_count(1)
{
	USE_PARAMETER(fieldmodule);
}

cmzn_fieldsmoothing::~cmzn_fieldsmoothing()
{
}

/*
Global functions
----------------
*/

cmzn_fieldsmoothing_id cmzn_fieldmodule_create_fieldsmoothing(
	cmzn_fieldmodule_id fieldmodule)
{
	if (fieldmodule)
		return new cmzn_fieldsmoothing(fieldmodule);
	return 0;
}

cmzn_fieldsmoothing_id cmzn_fieldsmoothing_access(cmzn_fieldsmoothing_id fieldsmoothing)
{
	if (fieldsmoothing)
		return fieldsmoothing->access();
	return 0;
}

int cmzn_fieldsmoothing_destroy(cmzn_fieldsmoothing_id *fieldsmoothing_address)
{
	if (!fieldsmoothing_address)
		return CMZN_ERROR_ARGUMENT;
	return cmzn_fieldsmoothing::deaccess(*fieldsmoothing_address);
}

int cmzn_fieldsmoothing_set_algorithm(cmzn_fieldsmoothing_id fieldsmoothing,
	cmzn_fieldsmoothing_algorithm algorithm)
{
	if (!fieldsmoothing)
		return CMZN_ERROR_ARGUMENT;
	return fieldsmoothing->setAlgorithm(algorithm);
}

int cmzn_fieldsmoothing_set_time(cmzn_fieldsmoothing_id fieldsmoothing, double time)
{
	if (!fieldsmoothing)
		return CMZN_ERROR_ARGUMENT;
	fieldsmoothing->setTime(time);
	return CMZN_OK;
}

int cmzn_field_smooth(cmzn_field_id field, cmzn_fieldsmoothing_id fieldsmoothing)
{
	if (!(field && fieldsmoothing))
		return CMZN_ERROR_ARGUMENT;
	FE_field *fe_field;
	Computed_field_get_type_finite_element(field, &fe_field);
	if (!fe_field)
		return CMZN_ERROR_ARGUMENT;
	if (!FE_region_smooth_FE_field(fe_field->get_FE_region(), fe_field,
		fieldsmoothing->getTime()))
		return CMZN_ERROR_GENERAL;
	return CMZN_OK;
}
