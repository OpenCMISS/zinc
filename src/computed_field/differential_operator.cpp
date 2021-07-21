/***************************************************************************//**
 * FILE : differential_operator.cpp
 *
 * Internal implementation for class representing a differential operator that
 * can be applied to a field to obtain a derivative.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "computed_field/differential_operator.hpp"
#include "opencmiss/zinc/status.h"

cmzn_differentialoperator::cmzn_differentialoperator(FieldDerivative *fieldDerivativeIn, int termIn) :
	fieldDerivative(fieldDerivativeIn->access()),
	term(termIn),
	access_count(1)
{
}

cmzn_differentialoperator::~cmzn_differentialoperator()
{
	FieldDerivative::deaccess(this->fieldDerivative);
}

cmzn_differentialoperator* cmzn_differentialoperator::create(FieldDerivative *fieldDerivativeIn, int termIn)
{
	if ((!fieldDerivativeIn)
		|| ((!fieldDerivativeIn->isMeshOnly()) && (termIn >= 0))
		|| (fieldDerivativeIn->isMeshOnly() && (termIn >= fieldDerivativeIn->getMeshTermCount())))
	{
		display_message(ERROR_MESSAGE, "cmzn_differentialoperator::create.  Invalid arguments");
		return nullptr;
	}
	return new cmzn_differentialoperator(fieldDerivativeIn, termIn);
}

cmzn_differentialoperator_id cmzn_differentialoperator_access(
	cmzn_differentialoperator_id differential_operator)
{
	if (differential_operator)
		return differential_operator->access();
	return 0;
}

int cmzn_differentialoperator_destroy(
	cmzn_differentialoperator_id *differential_operator_address)
{
	if (differential_operator_address)
		return cmzn_differentialoperator::deaccess(*differential_operator_address);
	return CMZN_ERROR_ARGUMENT;
}
