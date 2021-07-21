/**
 * FILE : fieldassignmentprivate.hpp
 * 
 * Implementation of field assignment class.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (CMZN_FIELDASSIGNMENTPRIVATE_HPP)
#define CMZN_FIELDASSIGNMENTPRIVATE_HPP

#include "opencmiss/zinc/field.h"
#include "opencmiss/zinc/fieldassignment.h"
#include "opencmiss/zinc/nodeset.h"
#include "opencmiss/zinc/result.h"

struct cmzn_fieldassignment
{
private:
	cmzn_field *targetField;
	cmzn_field *sourceField;
	cmzn_field *conditionalField;
	cmzn_nodeset *nodeset;
	int access_count;

	cmzn_fieldassignment(cmzn_field *targetFieldIn, cmzn_field *sourceFieldIn);

	~cmzn_fieldassignment();

public:

	static cmzn_fieldassignment *create(cmzn_field *targetFieldIn, cmzn_field *sourceFieldIn);

	cmzn_fieldassignment_id access()
	{
		++access_count;
		return this;
	}

	static int deaccess(cmzn_fieldassignment_id &fieldassignment)
	{
		if (!fieldassignment)
			return CMZN_RESULT_ERROR_ARGUMENT;
		--(fieldassignment->access_count);
		if (fieldassignment->access_count <= 0)
			delete fieldassignment;
		fieldassignment = 0;
		return CMZN_RESULT_OK;
	}

	int assign();

	cmzn_field *getConditionalField() const
	{
		if (this->conditionalField)
		{
			return cmzn_field_access(this->conditionalField);
		}
		return 0;
	}

	int setConditionalField(cmzn_field *conditionalFieldIn);

	cmzn_nodeset *getNodeset() const
	{
		if (this->nodeset)
		{
			return cmzn_nodeset_access(this->nodeset);
		}
		return 0;
	}

	int setNodeset(cmzn_nodeset *nodesetIn);

	cmzn_field *getSourceField() const
	{
		return cmzn_field_access(this->sourceField);
	}

	cmzn_field *getTargetField() const
	{
		return cmzn_field_access(this->targetField);
	}
};

#endif /* !defined (CMZN_FIELDASSIGNMENTPRIVATE_HPP) */
