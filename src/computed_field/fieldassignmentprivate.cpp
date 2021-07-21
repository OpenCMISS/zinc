/**
 * FILE : fieldassignmentprivate.cpp
 * 
 * Implementation of field assignment class.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "opencmiss/zinc/field.h"
#include "opencmiss/zinc/fieldmodule.h"
#include "opencmiss/zinc/node.h"
#include "opencmiss/zinc/nodeset.h"
#include "computed_field/fieldassignmentprivate.hpp"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_update.h"
#include "mesh/cmiss_node_private.hpp"
#include "general/debug.h"

cmzn_fieldassignment::cmzn_fieldassignment(cmzn_field *targetFieldIn, cmzn_field *sourceFieldIn) :
	targetField(cmzn_field_access(targetFieldIn)),
	sourceField(cmzn_field_access(sourceFieldIn)),
	conditionalField(0),
	nodeset(0),
	access_count(1)
{
}

cmzn_fieldassignment::~cmzn_fieldassignment()
{
	cmzn_field_destroy(&this->targetField);
	cmzn_field_destroy(&this->sourceField);
	cmzn_field_destroy(&this->conditionalField);
	cmzn_nodeset_destroy(&this->nodeset);
}

cmzn_fieldassignment *cmzn_fieldassignment::create(cmzn_field *targetFieldIn, cmzn_field *sourceFieldIn)
{
	if ((Computed_field_get_region(sourceFieldIn) == Computed_field_get_region(targetFieldIn))
		&& (cmzn_field_get_value_type(sourceFieldIn) == cmzn_field_get_value_type(targetFieldIn))
		&& (cmzn_field_get_number_of_components(sourceFieldIn)
			== cmzn_field_get_number_of_components(targetFieldIn)))
	{
		return new cmzn_fieldassignment(targetFieldIn, sourceFieldIn);
	}
	display_message(ERROR_MESSAGE, "Field createFieldassignment:  Invalid or incompatible fields");
	return 0;
}

int cmzn_fieldassignment::assign()
{
	cmzn_fieldmodule *fm = cmzn_field_get_fieldmodule(this->targetField);
	bool useGroup = false;
	cmzn_nodeset *useNodeset = (this->nodeset) ? cmzn_nodeset_access(this->nodeset)
		: cmzn_fieldmodule_find_nodeset_by_field_domain_type(fm, CMZN_FIELD_DOMAIN_TYPE_NODES);
	const int result = cmzn_nodeset_assign_field_from_source(useNodeset,
		this->targetField, this->sourceField, this->conditionalField, /*time*/0.0);
	if ((result != CMZN_RESULT_OK)
		&& (result != CMZN_RESULT_WARNING_PART_DONE)
		&& (result != CMZN_RESULT_ERROR_NOT_FOUND))
	{
		display_message(ERROR_MESSAGE, "Fieldassignment assign:  Error assigning source field");
	}
	cmzn_nodeset_destroy(&useNodeset);
	cmzn_fieldmodule_destroy(&fm);
	return result;
}

int cmzn_fieldassignment::setConditionalField(cmzn_field *conditionalFieldIn)
{
	if (conditionalFieldIn)
	{
		if ((Computed_field_get_region(conditionalFieldIn) != Computed_field_get_region(this->targetField))
			|| (cmzn_field_get_value_type(conditionalFieldIn) != CMZN_FIELD_VALUE_TYPE_REAL)
			|| (cmzn_field_get_number_of_components(conditionalFieldIn) != 1))
		{
			display_message(ERROR_MESSAGE, "Fieldassignment setConditionalField:  Invalid or incompatible conditional field");
			return CMZN_RESULT_ERROR_ARGUMENT;
		}
		cmzn_field_access(conditionalFieldIn);
	}
	if (this->conditionalField)
	{
		cmzn_field_destroy(&this->conditionalField);
	}
	this->conditionalField = conditionalFieldIn;
	return CMZN_RESULT_OK;
}

int cmzn_fieldassignment::setNodeset(cmzn_nodeset *nodesetIn)
{
	if (nodesetIn)
	{
		if (cmzn_nodeset_get_region_internal(nodesetIn) != Computed_field_get_region(this->targetField))
		{
			display_message(ERROR_MESSAGE, "Fieldassignment setNodeset:  Invalid or incompatible nodeset");
			return CMZN_RESULT_ERROR_ARGUMENT;
		}
		cmzn_nodeset_access(nodesetIn);
	}
	if (this->nodeset)
	{
		cmzn_nodeset_destroy(&this->nodeset);
	}
	this->nodeset = nodesetIn;
	return CMZN_RESULT_OK;
}

/*
Global functions
----------------
*/

cmzn_fieldassignment_id cmzn_field_create_fieldassignment(
	cmzn_field_id targetField, cmzn_field_id sourceField)
{
	return cmzn_fieldassignment::create(targetField, sourceField);
}

cmzn_fieldassignment_id cmzn_fieldassignment_access(cmzn_fieldassignment_id fieldassignment)
{
	if (fieldassignment)
		return fieldassignment->access();
	return 0;
}

int cmzn_fieldassignment_destroy(cmzn_fieldassignment_id *fieldassignment_address)
{
	if (!fieldassignment_address)
		return CMZN_RESULT_ERROR_ARGUMENT;
	return cmzn_fieldassignment::deaccess(*fieldassignment_address);
}

int cmzn_fieldassignment_assign(cmzn_fieldassignment_id fieldassignment)
{
	if (fieldassignment)
		return fieldassignment->assign();
	return CMZN_RESULT_ERROR_ARGUMENT;
}

cmzn_field_id cmzn_fieldassignment_get_conditional_field(
	cmzn_fieldassignment_id fieldassignment)
{
	if (fieldassignment)
	{
		return fieldassignment->getConditionalField();
	}
	display_message(ERROR_MESSAGE, "Fieldassignment getSourceField:  Invalid field assignment object");
	return 0;
}

int cmzn_fieldassignment_set_conditional_field(
	cmzn_fieldassignment_id fieldassignment, cmzn_field_id conditionalField)
{
	if (fieldassignment)
	{
		return fieldassignment->setConditionalField(conditionalField);
	}
	display_message(ERROR_MESSAGE, "Fieldassignment setConditionalField:  Invalid field assignment object");
	return CMZN_RESULT_ERROR_ARGUMENT;
}

cmzn_nodeset_id cmzn_fieldassignment_get_nodeset(
	cmzn_fieldassignment_id fieldassignment)
{
	if (fieldassignment)
	{
		return fieldassignment->getNodeset();
	}
	display_message(ERROR_MESSAGE, "Fieldassignment getNodeset:  Invalid field assignment object");
	return 0;
}

int cmzn_fieldassignment_set_nodeset(
	cmzn_fieldassignment_id fieldassignment, cmzn_nodeset_id nodeset)
{
	if (fieldassignment)
	{
		return fieldassignment->setNodeset(nodeset);
	}
	display_message(ERROR_MESSAGE, "Fieldassignment setNodeset:  Invalid field assignment object");
	return CMZN_RESULT_ERROR_ARGUMENT;
}

cmzn_field_id cmzn_fieldassignment_get_source_field(
	cmzn_fieldassignment_id fieldassignment)
{
	if (fieldassignment)
	{
		return fieldassignment->getSourceField();
	}
	display_message(ERROR_MESSAGE, "Fieldassignment getSourceField:  Invalid field assignment object");
	return 0;
}

cmzn_field_id cmzn_fieldassignment_get_target_field(
	cmzn_fieldassignment_id fieldassignment)
{
	if (fieldassignment)
	{
		return fieldassignment->getTargetField();
	}
	display_message(ERROR_MESSAGE, "Fieldassignment getTargetField:  Invalid field assignment object");
	return 0;
}
