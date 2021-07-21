/*******************************************************************************
FILE : computed_field_image_filter.cpp

LAST MODIFIED : 9 September 2006

DESCRIPTION :
Class used for wrapping itk filters
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "opencmiss/zinc/fieldimageprocessing.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_private.hpp"
#include "image_processing/computed_field_image_filter.h"
#include "general/debug.h"
#include "general/mystring.h"
#include "general/message.h"

namespace CMZN {
int computed_field_image_filter::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
	return functor->update_and_evaluate_filter(cache, valueCache);
}

} // namespace CMZN
