/**
 * @file nodetemplate.hpp
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef CMZN_NODETEMPLATE_HPP__
#define CMZN_NODETEMPLATE_HPP__

#include "opencmiss/zinc/nodetemplate.h"
#include "opencmiss/zinc/field.hpp"
#include "opencmiss/zinc/node.hpp"
#include "opencmiss/zinc/timesequence.hpp"

namespace OpenCMISS
{
namespace Zinc
{

class Nodetemplate
{
private:

	cmzn_nodetemplate_id id;

public:

	Nodetemplate() : id(0)
	{ }

	// takes ownership of C handle, responsibility for destroying it
	explicit Nodetemplate(cmzn_nodetemplate_id node_template_id) :
		id(node_template_id)
	{ }

	Nodetemplate(const Nodetemplate& nodeTemplate) :
		id(cmzn_nodetemplate_access(nodeTemplate.id))
	{ }

	Nodetemplate& operator=(const Nodetemplate& nodeTemplate)
	{
		cmzn_nodetemplate_id temp_id = cmzn_nodetemplate_access(nodeTemplate.id);
		if (0 != id)
		{
			cmzn_nodetemplate_destroy(&id);
		}
		id = temp_id;
		return *this;
	}

	~Nodetemplate()
	{
		if (0 != id)
		{
			cmzn_nodetemplate_destroy(&id);
		}
	}

	bool isValid() const
	{
		return (0 != id);
	}

	cmzn_nodetemplate_id getId() const
	{
		return id;
	}

	int defineField(const Field& field)
	{
		return cmzn_nodetemplate_define_field(id, field.getId());
	}

	int defineFieldFromNode(const Field& field, const Node& node)
	{
		return cmzn_nodetemplate_define_field_from_node(id, field.getId(), node.getId());
	}

	Timesequence getTimesequence(const Field& field)
	{
		return Timesequence(cmzn_nodetemplate_get_timesequence(id, field.getId()));
	}

	int setTimesequence(const Field& field, const Timesequence& timesequence)
	{
		return cmzn_nodetemplate_set_timesequence(id, field.getId(), timesequence.getId());
	}

	int getValueNumberOfVersions(const Field& field, int componentNumber,
		Node::ValueLabel valueLabel)
	{
		return cmzn_nodetemplate_get_value_number_of_versions(id, field.getId(),
			componentNumber, static_cast<cmzn_node_value_label>(valueLabel));
	}

	int setValueNumberOfVersions(const Field& field, int componentNumber,
		Node::ValueLabel valueLabel, int numberOfVersions)
	{
		return cmzn_nodetemplate_set_value_number_of_versions(id, field.getId(),
			componentNumber, static_cast<cmzn_node_value_label>(valueLabel), numberOfVersions);
	}

	int removeField(const Field& field)
	{
		return cmzn_nodetemplate_remove_field(id, field.getId());
	}
	int undefineField(const Field& field)
	{
		return cmzn_nodetemplate_undefine_field(id, field.getId());
	}
};

inline int Node::merge(const Nodetemplate& nodeTemplate)
{
	return cmzn_node_merge(id, nodeTemplate.getId());
}

}  // namespace Zinc
}

#endif // CMZN_NODETEMPLATE_HPP__
