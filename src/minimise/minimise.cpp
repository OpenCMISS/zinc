/**
 * FILE : minimise.cpp
 *
 * Optimisation/minimisation routines.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <math.h>
#include "opencmiss/zinc/field.h"
#include "opencmiss/zinc/fieldmodule.h"
#include "opencmiss/zinc/optimisation.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_composite.h"
#include "computed_field/computed_field_set.h"
#include "computed_field/computed_field_finite_element.h"
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_region.h"
#include "general/callback_private.h"
#include "general/compare.h"
#include "general/debug.h"
#include "general/indexed_list_private.h"
#include "general/object.h"
#include "general/message.h"
#include "general/mystring.h"
#include "minimise/minimise.h"
#include "general/enumerator_private.hpp"

PROTOTYPE_ENUMERATOR_STRING_FUNCTION(cmzn_optimisation_method)
{
	const char *enumerator_string = 0;
	switch (enumerator_value)
	{
	case CMZN_OPTIMISATION_METHOD_QUASI_NEWTON:
		enumerator_string = "QUASI_NEWTON";
		break;
	case CMZN_OPTIMISATION_METHOD_LEAST_SQUARES_QUASI_NEWTON:
		enumerator_string = "LEAST_SQUARES_QUASI_NEWTON";
		break;
	case CMZN_OPTIMISATION_METHOD_NEWTON:
		enumerator_string = "NEWTON";
		break;
	default:
		break;
	}
	return enumerator_string;
}

DEFINE_DEFAULT_ENUMERATOR_FUNCTIONS( cmzn_optimisation_method);

