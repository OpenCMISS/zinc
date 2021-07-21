/**
 * @file elementtemplate.hpp
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CMZN_ELEMENTTEMPLATE_HPP__
#define CMZN_ELEMENTTEMPLATE_HPP__

#include "opencmiss/zinc/elementtemplate.h"
#include "opencmiss/zinc/element.hpp"
#include "opencmiss/zinc/elementfieldtemplate.hpp"
#include "opencmiss/zinc/field.hpp"
#include "opencmiss/zinc/node.hpp"

namespace OpenCMISS
{
namespace Zinc
{

class Elementtemplate
{
private:

	cmzn_elementtemplate_id id;

public:

	Elementtemplate() : id(0)
	{ }

	// takes ownership of C handle, responsibility for destroying it
	explicit Elementtemplate(cmzn_elementtemplate_id element_template_id) :
		id(element_template_id)
	{ }

	Elementtemplate(const Elementtemplate& elementTemplate) :
		id(cmzn_elementtemplate_access(elementTemplate.id))
	{ }

	Elementtemplate& operator=(const Elementtemplate& elementTemplate)
	{
		cmzn_elementtemplate_id temp_id = cmzn_elementtemplate_access(elementTemplate.id);
		if (0 != id)
		{
			cmzn_elementtemplate_destroy(&id);
		}
		id = temp_id;
		return *this;
	}

	~Elementtemplate()
	{
		if (0 != id)
		{
			cmzn_elementtemplate_destroy(&id);
		}
	}

	bool isValid() const
	{
		return (0 != id);
	}

	cmzn_elementtemplate_id getId() const
	{
		return id;
	}

	enum Element::ShapeType getElementShapeType()
	{
		return static_cast<Element::ShapeType>(cmzn_elementtemplate_get_element_shape_type(id));
	}

	int setElementShapeType(enum Element::ShapeType shapeType)
	{
		return cmzn_elementtemplate_set_element_shape_type(id,
			static_cast<cmzn_element_shape_type>(shapeType));
	}

	int getNumberOfNodes()
	{
		return cmzn_elementtemplate_get_number_of_nodes(id);
	}

	int setNumberOfNodes(int numberOfNodes)
	{
		return cmzn_elementtemplate_set_number_of_nodes(id, numberOfNodes);
	}

	int defineField(const Field& field, int componentNumber, const Elementfieldtemplate& eft)
	{
		return cmzn_elementtemplate_define_field(this->id, field.getId(),
			componentNumber, eft.getId());
	}

	int defineFieldElementConstant(const Field& field, int componentNumber)
	{
		return cmzn_elementtemplate_define_field_element_constant(
			id, field.getId(), componentNumber);
	}

	int defineFieldSimpleNodal(const Field& field, int componentNumber,
		const Elementbasis& basis, int nodeIndexesCount, const int *nodeIndexesIn)
	{
		return cmzn_elementtemplate_define_field_simple_nodal(
			id, field.getId(), componentNumber, basis.getId(),
			nodeIndexesCount, nodeIndexesIn);
	}

	int setMapNodeValueLabel(const Field& field, int componentNumber,
		int basisNodeIndex, int nodeFunctionIndex, Node::ValueLabel nodeValueLabel)
	{
		return cmzn_elementtemplate_set_map_node_value_label(id, field.getId(),
			componentNumber, basisNodeIndex, nodeFunctionIndex,
			static_cast<cmzn_node_value_label>(nodeValueLabel));
	}

	int setMapNodeVersion(const Field& field, int componentNumber,
		int basisNodeIndex, int nodeFunctionIndex, int versionNumber)
	{
		return cmzn_elementtemplate_set_map_node_version(id, field.getId(),
			componentNumber, basisNodeIndex, nodeFunctionIndex, versionNumber);
	}

	Node getNode(int localNodeIndex)
	{
		return Node(cmzn_elementtemplate_get_node(id, localNodeIndex));
	}

	int setNode(int localNodeIndex, const Node& node)
	{
		return cmzn_elementtemplate_set_node(id, localNodeIndex, node.getId());
	}

	int removeField(const Field& field)
	{
		return cmzn_elementtemplate_remove_field(this->id, field.getId());
	}

	int undefineField(const Field& field)
	{
		return cmzn_elementtemplate_undefine_field(this->id, field.getId());
	}
};

inline int Element::merge(const Elementtemplate& elementTemplate)
{
	return cmzn_element_merge(id, elementTemplate.getId());
}

}  // namespace Zinc
}

#endif /* CMZN_ELEMENTTEMPLATE_HPP__ */
