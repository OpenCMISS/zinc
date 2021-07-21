/***************************************************************************//**
 * FILE : computed_field_group_base.hpp
 * 
 * Common base class for group fields & common group change detail types.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMPUTED_FIELD_GROUP_BASE_HPP)
#define COMPUTED_FIELD_GROUP_BASE_HPP

#include "computed_field/computed_field_private.hpp"

/**
 * Flags summarising changes to a group.
 */
enum cmzn_field_group_change_type
{
	CMZN_FIELD_GROUP_CHANGE_NONE = 0,  /*!< no change */
	CMZN_FIELD_GROUP_CHANGE_ADD = 1,   /*!< one or more objects added */
	CMZN_FIELD_GROUP_CHANGE_REMOVE = 2 /*!< one or more objects removed */
};

/**
 * Base class for all group field change details.
 */
struct cmzn_field_group_base_change_detail : public cmzn_field_change_detail
{
	/**
	 * Override to return overall summary of changes to group.
	 * @return Logical OR of enum cmzn_field_group_change_type values.
	 */
	virtual int getChangeSummary() const = 0;
};

/***************************************************************************//**
 * Base class for all group fields.
 */
class Computed_field_group_base : public Computed_field_core
{
public:

	Computed_field_group_base() :
		Computed_field_core()
	{
	}

	virtual char *get_command_string()
	{
		return 0;
	}

	virtual bool isEmpty() const = 0;

	virtual int clear() = 0;

	virtual bool is_purely_function_of_field(cmzn_field *other_field)
	{
		return (this->field == other_field);
	}
};

#endif /* COMPUTED_FIELD_GROUP_BASE_HPP */

