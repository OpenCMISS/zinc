/**
 * @file fieldassignment.hpp
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CMZN_FIELDASSIGNMENT_HPP__
#define CMZN_FIELDASSIGNMENT_HPP__

#include "opencmiss/zinc/fieldassignment.h"
#include "opencmiss/zinc/field.hpp"
#include "opencmiss/zinc/nodeset.hpp"

namespace OpenCMISS
{
namespace Zinc
{

class Fieldassignment
{
protected:
	cmzn_fieldassignment_id id;

public:

	Fieldassignment() :
		id(0)
	{
	}

	// takes ownership of C handle, responsibility for destroying it
	explicit Fieldassignment(cmzn_fieldassignment_id fieldassignment_id) :
		id(fieldassignment_id)
	{
	}

	Fieldassignment(const Fieldassignment& fieldassignment) :
		id(cmzn_fieldassignment_access(fieldassignment.id))
	{
	}

	Fieldassignment& operator=(const Fieldassignment& fieldassignment)
	{
		cmzn_fieldassignment_id temp_id = cmzn_fieldassignment_access(fieldassignment.id);
		if (0 != this->id)
		{
			cmzn_fieldassignment_destroy(&this->id);
		}
		this->id = temp_id;
		return *this;
	}

	~Fieldassignment()
	{
		if (0 != this->id)
		{
			cmzn_fieldassignment_destroy(&this->id);
		}
	}

	bool isValid() const
	{
		return (0 != this->id);
	}

	cmzn_fieldassignment_id getId() const
	{
		return id;
	}

	int assign()
	{
		return cmzn_fieldassignment_assign(this->id);
	}

	Field getConditionalField() const
	{
		return Field(cmzn_fieldassignment_get_conditional_field(this->id));
	}

	int setConditionalField(const Field& conditionalField)
	{
		return cmzn_fieldassignment_set_conditional_field(this->id, conditionalField.getId());
	}

	Nodeset getNodeset() const
	{
		return Nodeset(cmzn_fieldassignment_get_nodeset(this->id));
	}

	int setNodeset(const Nodeset& nodeset)
	{
		return cmzn_fieldassignment_set_nodeset(this->id, nodeset.getId());
	}

	Field getSourceField() const
	{
		return Field(cmzn_fieldassignment_get_source_field(this->id));
	}

	Field getTargetField() const
	{
		return Field(cmzn_fieldassignment_get_target_field(this->id));
	}
};

inline Fieldassignment Field::createFieldassignment(const Field& sourceField)
{
	return Fieldassignment(cmzn_field_create_fieldassignment(this->getId(), sourceField.getId()));
}

}  // namespace Zinc
}

#endif /* CMZN_FIELDASSIGNMENT_HPP__ */
