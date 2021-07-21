/**
 * @file node.hpp
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef CMZN_NODE_HPP__
#define CMZN_NODE_HPP__

#include "opencmiss/zinc/node.h"

namespace OpenCMISS
{
namespace Zinc
{

class Nodeset;
class Nodetemplate;

class Node
{
private:

	cmzn_node_id id;

public:

	Node() : id(0)
	{ }

	// takes ownership of C handle, responsibility for destroying it
	explicit Node(cmzn_node_id node_id) : id(node_id)
	{ }

	Node(const Node& node) :
		id(cmzn_node_access(node.id))
	{ }

	/**
	 * Node change bit flags returned by Nodesetchanges query functions.
	 */
	enum ChangeFlag
	{
		CHANGE_FLAG_NONE = CMZN_NODE_CHANGE_FLAG_NONE,
		CHANGE_FLAG_ADD = CMZN_NODE_CHANGE_FLAG_ADD,
		CHANGE_FLAG_REMOVE = CMZN_NODE_CHANGE_FLAG_REMOVE,
		CHANGE_FLAG_IDENTIFIER = CMZN_NODE_CHANGE_FLAG_IDENTIFIER,
		CHANGE_FLAG_DEFINITION = CMZN_NODE_CHANGE_FLAG_DEFINITION,
		CHANGE_FLAG_FIELD = CMZN_NODE_CHANGE_FLAG_FIELD
	};
	
	/**
	 * Type for passing logical OR of #ChangeFlag
	 */
	typedef int ChangeFlags;

	enum ValueLabel
	{
		VALUE_LABEL_INVALID = CMZN_NODE_VALUE_LABEL_INVALID,
		VALUE_LABEL_VALUE = CMZN_NODE_VALUE_LABEL_VALUE,                /*!< literal field value */
		VALUE_LABEL_D_DS1 = CMZN_NODE_VALUE_LABEL_D_DS1,                /*!< derivative w.r.t. arc length S1 */
		VALUE_LABEL_D_DS2 = CMZN_NODE_VALUE_LABEL_D_DS2,                /*!< derivative w.r.t. arc length S2 */
		VALUE_LABEL_D2_DS1DS2 = CMZN_NODE_VALUE_LABEL_D2_DS1DS2,        /*!< cross derivative w.r.t. arc lengths S1,S2 */
		VALUE_LABEL_D_DS3 = CMZN_NODE_VALUE_LABEL_D_DS3,                /*!< derivative w.r.t. arc length S3 */
		VALUE_LABEL_D2_DS1DS3 = CMZN_NODE_VALUE_LABEL_D2_DS1DS3,        /*!< cross derivative w.r.t. arc lengths S1,S3 */
		VALUE_LABEL_D2_DS2DS3 = CMZN_NODE_VALUE_LABEL_D2_DS2DS3,        /*!< cross derivative w.r.t. arc lengths S2,S3 */
		VALUE_LABEL_D3_DS1DS2DS3 = CMZN_NODE_VALUE_LABEL_D3_DS1DS2DS3,  /*!< triple cross derivative w.r.t. arc lengths S1,S2,S3 */
	};

	Node& operator=(const Node& node)
	{
		cmzn_node_id temp_id = cmzn_node_access(node.id);
		if (0 != id)
		{
			cmzn_node_destroy(&id);
		}
		id = temp_id;
		return *this;
	}

	~Node()
	{
		if (0 != id)
		{
			cmzn_node_destroy(&id);
		}
	}

	bool isValid() const
	{
		return (0 != id);
	}

	cmzn_node_id getId() const
	{
		return id;
	}

	int getIdentifier()
	{
		return cmzn_node_get_identifier(id);
	}

	int setIdentifier(int identifier)
	{
		return cmzn_node_set_identifier(id, identifier);
	}

	inline Nodeset getNodeset() const;

	int merge(const Nodetemplate& nodeTemplate);

};

inline bool operator==(const Node& a, const Node& b)
{
	return a.getId() == b.getId();
}

class Nodeiterator
{
private:

	cmzn_nodeiterator_id id;

public:

	Nodeiterator() : id(0)
	{ }

	// takes ownership of C handle, responsibility for destroying it
	explicit Nodeiterator(cmzn_nodeiterator_id node_iterator_id) :
		id(node_iterator_id)
	{ }

	Nodeiterator(const Nodeiterator& nodeIterator) :
		id(cmzn_nodeiterator_access(nodeIterator.id))
	{ }

	Nodeiterator& operator=(const Nodeiterator& nodeIterator)
	{
		cmzn_nodeiterator_id temp_id = cmzn_nodeiterator_access(nodeIterator.id);
		if (0 != id)
		{
			cmzn_nodeiterator_destroy(&id);
		}
		id = temp_id;
		return *this;
	}

	~Nodeiterator()
	{
		if (0 != id)
		{
			cmzn_nodeiterator_destroy(&id);
		}
	}

	bool isValid() const
	{
		return (0 != id);
	}

	Node next()
	{
		return Node(cmzn_nodeiterator_next(id));
	}
};

}  // namespace Zinc
}

#endif // CMZN_NODE_HPP__
