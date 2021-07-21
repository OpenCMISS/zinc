/**
 * FILE : finite_element_nodeset.hpp
 *
 * Class defining a domain consisting of a set of nodes.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (FINITE_ELEMENT_NODESET_HPP)
#define FINITE_ELEMENT_NODESET_HPP

#include "opencmiss/zinc/types/fieldid.h"  // for cmzn_field_domain_type
#include "opencmiss/zinc/types/nodeid.h"
#include "opencmiss/zinc/status.h"
#include "datastore/labels.hpp"
#include "datastore/labelschangelog.hpp"
#include "datastore/maparray.hpp"
#include "general/block_array.hpp"
#include "general/enumerator.h"
#include "general/list.h"
#include <list>

class FE_nodeset;
struct FE_field;
struct FE_node_field;
struct FE_region;
struct FE_time_sequence;

DECLARE_LIST_TYPES(FE_node_field);

PROTOTYPE_LIST_FUNCTIONS(FE_node_field);

PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(FE_node_field, field, \
	struct FE_field *);

/**
 * Describes how fields are defined for a node.
 * Stores pointer back to owning FE_nodeset.
 */
struct FE_node_field_info
{
	// Following are public only to support legacy functions in finite_element.cpp; no new code should use them!
public:

	/* the total number of values and derivatives */
	/*???RC not convinced number_of_values needs to be in this structure */
	int number_of_values;

	/* the size of the data in node->values->storage */
	int values_storage_size;

	/* list of the node fields */
	struct LIST(FE_node_field) *node_field_list;

	/* the FE_nodeset this FE_node_field_info and all nodes using it belong to */
	FE_nodeset *nodeset;

	/* the number of structures that point to this node field information.  The
		node field information cannot be destroyed while this is greater than 0 */
	int access_count;

	/** takes ownership of fe_node_field_listIn */
	FE_node_field_info(FE_nodeset *nodesetIn, struct LIST(FE_node_field) *nodeFieldListIn,
		int numberOfValuesIn);

	~FE_node_field_info();

public:

	/** @param node_field_listIn  FE_node_field list to copy, or nullptr to use empty list. */
	static FE_node_field_info *create(FE_nodeset *nodesetIn, struct LIST(FE_node_field) *nodeFieldListIn,
		int numberOfValuesIn);

	inline FE_node_field_info *access()
	{
		++access_count;
		return this;
	}

	static inline int deaccess(FE_node_field_info* &info)
	{
		if (info)
		{
			if (--(info->access_count) == 0)
				delete info;
			info = nullptr;
			return CMZN_OK;
		}
		return CMZN_ERROR_ARGUMENT;
	}

	static inline void reaccess(FE_node_field_info* &info, FE_node_field_info *newInfo)
	{
		if (newInfo)
			++(newInfo->access_count);
		if ((info) && (--(info->access_count) == 0))
			delete info;
		info = newInfo;
	}

	inline int getAccessCount() const
	{
		return this->access_count;
	}

	bool usedOnce() const
		/**
		 * @return  True if the access count indicates info is being used
		 * by only one node.
		 */
	{
		return this->access_count == 1;
	}

	/** @return  Definition of field at node, or nullptr if none. */
	const FE_node_field *getNodeField(FE_field *field) const
	{
		return FIND_BY_IDENTIFIER_IN_LIST(FE_node_field, field)(field, this->node_field_list);
	}

	/** @return  Definition of field at node, or nullptr if none. */
	FE_node_field *getNodeField(FE_field *field)
	{
		return FIND_BY_IDENTIFIER_IN_LIST(FE_node_field, field)(field, this->node_field_list);
	}

	/** @return  True if member node_field_list has exactly the same FE_node_fields as
	 * node_field_listIn. */
	bool hasMatchingNodeFieldList(struct LIST(FE_node_field) *nodeFieldListIn) const;

	int getNumberOfValues() const
	{
		return this->number_of_values;
	}

	/**
	 * Marks each FE_field in <fe_node_field_info> as RELATED_OBJECT_CHANGED
	 * in FE_region.
	 */
	void logFieldsChangeRelated(FE_region *fe_region) const;

};

struct cmzn_node
{
	// Following are public only to support legacy functions in finite_element.cpp; no new code should use them!
public:

	/* index into nodeset labels, maps to unique identifier */
	DsLabelIndex index;

	/** the number of structures that point to this node.  The node cannot be
	 * destroyed while this is greater than 0 */
	int access_count;

	/* the fields defined at the node */
	struct FE_node_field_info *fields;

	/* the global values and derivatives for the fields defined at the node */
	Value_storage *values_storage;

	/**
	 * Set the index of the node in owning nodeset. Used only by FE_nodeset when
	 * merging nodes from another region's nodeset.
	 * @param index  The new index, non-negative. Value is not checked due
	 * to use by privileged caller.
	 */
	void setIndex(DsLabelIndex indexIn)
	{
		this->index = indexIn;
	}

	cmzn_node() :
		index(DS_LABEL_INDEX_INVALID),
		access_count(1),
		fields(nullptr),
		values_storage(nullptr)
	{
	}

	~cmzn_node();

	/**
	 * Clear content of node and disconnect it from owning nodeset.
	 * Use when removing node from nodeset or deleting nodeset to safely orphan any
	 * externally accessed nodes.
	 */
	void invalidate();

	/** @return  Node field info. Must not be modified. */
	struct FE_node_field_info *getNodeFieldInfo() const
	{
		return this->fields;
	}

	/** Changes the FE_node_field_info at <node> to <fe_node_field_info>.
	 * Note it is very important that the old and the new FE_node_field_info structures
	 * describe the same data layout in the nodal values_storage!
	 * Private function only to be called by FE_region when merging FE_regions! */
	void setNodeFieldInfo(FE_node_field_info *nodeFieldInfoIn)
	{
		FE_node_field_info::reaccess(this->fields, nodeFieldInfoIn);
	}

public:

	/**
	 * Special constructor for a non-global node to be used as a template for
	 * creating other nodes in the nodeset for node_field_info.
	 * @param node_field_info  A struct FE_node_field_info linking to the
	 * nodeset. Should have no fields. Caller MUST supply valid pointer.
	 * @return  Accessed node, or nullptr on error. Do not merge into FE_nodeset!
	 */
	static cmzn_node* createTemplate(FE_node_field_info *node_field_info)
	{
		cmzn_node *node = new cmzn_node();
		node->fields = node_field_info->access();
		return node;
	}

	/**
	 * Construct node with the specified index, copying the supplied template node
	 * i.e. with all fields and values from it.
	 * The returned node is ready to be added into the FE_nodeset the
	 * template node was created by.
	 * @param index  Index of node in nodeset, or DS_LABEL_INDEX_INVALID if used
	 * as a non-global template node i.e. when called from
	 * FE_node_template::FE_node_template().
	 * @param template_node  node to copy.
	 * @return  Accessed node, or nullptr on error.
	 */
	static cmzn_node* createFromTemplate(DsLabelIndex index, cmzn_node *template_node);

	inline cmzn_node *access()
	{
		++access_count;
		return this;
	}

	static inline int deaccess(cmzn_node* &node)
	{
		if (node)
		{
			if (--(node->access_count) == 0)
				delete node;
			node = nullptr;
			return CMZN_OK;
		}
		return CMZN_ERROR_ARGUMENT;
	}

	static inline void reaccess(cmzn_node* &node, cmzn_node *newNode)
	{
		if (newNode)
			++(newNode->access_count);
		if ((node) && (--(node->access_count) == 0))
			delete node;
		node = newNode;
	}

	inline int getAccessCount() const
	{
		return this->access_count;
	}

	inline DsLabelIdentifier getIdentifier() const;

	inline DsLabelIndex getIndex() const
	{
		return this->index;
	}

	inline int getElementUsageCount() const;

	inline void incrementElementUsageCount();

	inline void decrementElementUsageCount();

	/** @return  Non-accessed nodeset, or nullptr if invalid. */
	inline FE_nodeset *getNodeset() const
	{
		if (this->fields)
			return this->fields->nodeset;
		return nullptr;
	}

	/** @return  Definition of field at node, or nullptr if none. */
	const FE_node_field *getNodeField(FE_field *field) const
	{
		if (this->fields)
			return this->fields->getNodeField(field);
		return nullptr;
	}

	/** @return  Definition of field at node, or nullptr if none. */
	FE_node_field *getNodeField(FE_field *field)
	{
		if (this->fields)
			return this->fields->getNodeField(field);
		return nullptr;
	}

};

// Legacy node type
#define FE_node cmzn_node

DECLARE_LIST_CONDITIONAL_FUNCTION(cmzn_node);
DECLARE_LIST_ITERATOR_FUNCTION(cmzn_node);

PROTOTYPE_ENUMERATOR_FUNCTIONS(cmzn_node_value_label);

/**
* Template for creating a new node in the given FE_nodeset
* @see FE_nodeset::create_FE_node()
*/
class FE_node_template : public cmzn::RefCounted
{
	friend class FE_nodeset;

	FE_nodeset *nodeset; // Note: accessed: prevents nodeset from being destroyed
	cmzn_node *template_node;

	FE_node_template(FE_nodeset *nodeset_in, struct FE_node_field_info *node_field_info);

	/** Creates a template copying from an existing node */
	FE_node_template(FE_nodeset *nodeset_in, cmzn_node *node);

	~FE_node_template();

	FE_node_template(); // not implemented
	FE_node_template(const FE_node_template&); // not implemented

public:

	FE_nodeset *get_nodeset() const
	{
		return this->nodeset;
	}

	cmzn_node *get_template_node() const
	{
		return this->template_node;
	}
};

/**
 * A set of nodes/datapoints in the FE_region.
 */
class FE_nodeset
{
	FE_region *fe_region; // not accessed
	cmzn_field_domain_type domainType;

	DsLabels labels; // node identifiers

	// map labels -> cmzn_node (accessed)
	block_array<DsLabelIndex, cmzn_node*> fe_nodes;

	// number of element references to node, so can't destroy while in use
	typedef unsigned short ElementUsageCountType; // internal use only
	block_array<DsLabelIndex, ElementUsageCountType> elementUsageCount;

	std::list<FE_node_field_info*> node_field_info_list;
	struct FE_node_field_info *last_fe_node_field_info;

	// log of nodes added, removed or otherwise changed
	DsLabelsChangeLog *changeLog;

	// list of node iterators to invalidate when nodeset destroyed
	cmzn_nodeiterator *activeNodeIterators;

	int access_count;

	FE_nodeset(FE_region *fe_region);

	~FE_nodeset();

	void createChangeLog();

	int remove_FE_node_private(cmzn_node *node);

	struct Merge_FE_node_external_data;
	int merge_FE_node_external(cmzn_node *node,
		Merge_FE_node_external_data &data);

public:

	static FE_nodeset *create(FE_region *fe_region)
	{
		if (fe_region)
			return new FE_nodeset(fe_region);
		return 0;
	}

	void detach_from_FE_region();

	FE_nodeset *access()
	{
		++(this->access_count);
		return this;
	}

	static void deaccess(FE_nodeset *&nodeset)
	{
		if (nodeset)
		{
			--(nodeset->access_count);
			if (nodeset->access_count <= 0)
				delete nodeset;
			nodeset = nullptr;
		}
	}

	void clearChangeLog()
	{
		this->createChangeLog();
	}

	// in following change is a logical OR of values from enum DsLabelChangeType
	void nodeChange(DsLabelIndex nodeIndex, int change);
	void nodeChange(DsLabelIndex nodeIndex, int change, cmzn_node *field_info_node);
	void nodeFieldChange(cmzn_node *node, FE_field *fe_field);

	void nodeIdentifierChange(cmzn_node *node)
	{
		this->nodeChange(node->getIndex(), DS_LABEL_CHANGE_TYPE_IDENTIFIER);
	}

	void nodeAddedChange(cmzn_node *node)
	{
		this->nodeChange(node->getIndex(), DS_LABEL_CHANGE_TYPE_ADD, node);
	}

	void nodeRemovedChange(cmzn_node *node)
	{
		this->nodeChange(node->getIndex(), DS_LABEL_CHANGE_TYPE_REMOVE, node);
	}

	FE_region *get_FE_region() const
	{
		return this->fe_region;
	}

	cmzn_field_domain_type getFieldDomainType() const
	{
		return this->domainType;
	}

	void setFieldDomainType(cmzn_field_domain_type domainTypeIn)
	{
		this->domainType = domainTypeIn;
		this->labels.setName(this->getName());
	}

	const char *getName() const;

	const DsLabels& getLabels() const
	{
		return this->labels;
	}

	void clear();

	/** @return Accessed changes */
	DsLabelsChangeLog *extractChangeLog();

	/** @retrun non-Accessed changes */
	DsLabelsChangeLog *getChangeLog()
	{
		return this->changeLog;
	}

	struct FE_node_field_info *get_FE_node_field_info(int number_of_values,
		struct LIST(FE_node_field) *fe_node_field_list);

	int get_FE_node_field_info_adding_new_field(
		struct FE_node_field_info **node_field_info_address,
		struct FE_node_field *new_node_field, int new_number_of_values);

	struct FE_node_field_info *clone_FE_node_field_info(
		struct FE_node_field_info *fe_node_field_info);

	void remove_FE_node_field_info(struct FE_node_field_info *fe_node_field_info);

	bool is_FE_field_in_use(struct FE_field *fe_field);

	int getElementUsageCount(DsLabelIndex nodeIndex) const;
	void incrementElementUsageCount(DsLabelIndex nodeIndex);
	void decrementElementUsageCount(DsLabelIndex nodeIndex);

	/** get size i.e. number of nodes in nodeset */
	int getSize() const
	{
		return this->labels.getSize();
	}

	inline DsLabelIdentifier getNodeIdentifier(DsLabelIndex nodeIndex) const
	{
		return this->labels.getIdentifier(nodeIndex);
	}

	/** @return  Non-accessed node object at index */
	inline cmzn_node *getNode(DsLabelIndex nodeIndex) const
	{
		cmzn_node *node = 0;
		if (nodeIndex >= 0)
			this->fe_nodes.getValue(nodeIndex, node);
		return node;
	}

	DsLabelIdentifier get_next_FE_node_identifier(DsLabelIdentifier start_identifier)
	{
		return this->labels.getFirstFreeIdentifier(start_identifier);
	}

	void list_btree_statistics();

	bool containsNode(cmzn_node *node) const
	{
		return (node) ? (node->getNodeset() == this) && (node->getIndex() >= 0) : false;
	}

	DsLabelIndex findIndexByIdentifier(DsLabelIdentifier identifier) const
	{
		return this->labels.findLabelByIdentifier(identifier);
	}

	/** @return  Non-accessed node */
	cmzn_node *findNodeByIdentifier(DsLabelIdentifier identifier) const
	{
		return this->getNode(this->labels.findLabelByIdentifier(identifier));
	}

	void removeNodeiterator(cmzn_nodeiterator *iterator); // private, but needed by cmzn_nodeiterator

	cmzn_nodeiterator *createNodeiterator(DsLabelsGroup *labelsGroup = 0);

	int for_each_FE_node(LIST_ITERATOR_FUNCTION(cmzn_node) iterator_function, void *user_data_void);

	DsLabelsGroup *createLabelsGroup();

	int change_FE_node_identifier(cmzn_node *node, DsLabelIdentifier new_identifier);

	FE_node_template *create_FE_node_template(cmzn_node *node);

	FE_node_template *create_FE_node_template();

	cmzn_node *get_or_create_FE_node_with_identifier(DsLabelIdentifier identifier);

	cmzn_node *create_FE_node(DsLabelIdentifier identifier, FE_node_template *node_template);

	int merge_FE_node_template(struct cmzn_node *destination, FE_node_template *fe_node_template);

	int undefineFieldAtNode(struct cmzn_node *node, struct FE_field *fe_field);

	int destroyNode(struct cmzn_node *node);

	int destroyAllNodes();

	int destroyNodesInGroup(DsLabelsGroup& labelsGroup);

	int get_last_FE_node_identifier();

	bool FE_field_has_multiple_times(struct FE_field *fe_field) const;

	void getHighestNodeFieldDerivativeAndVersion(FE_field *field,
		int& highestDerivative, int& highestVersion) const;

	bool canMerge(FE_nodeset &source);

	int merge(FE_nodeset &source);
};

inline DsLabelIdentifier cmzn_node::getIdentifier() const
{
	if (this->fields)
		return this->fields->nodeset->getNodeIdentifier(this->index);
	return DS_LABEL_IDENTIFIER_INVALID;
}

inline int cmzn_node::getElementUsageCount() const
{
	if (this->fields)
		return this->fields->nodeset->getElementUsageCount(this->index);
	return 0;
}

inline void cmzn_node::incrementElementUsageCount()
{
	if (this->fields)
		this->fields->nodeset->incrementElementUsageCount(this->index);
}

inline void cmzn_node::decrementElementUsageCount()
{
	if (this->fields)
		this->fields->nodeset->decrementElementUsageCount(this->index);
}

struct cmzn_nodeiterator : public cmzn::RefCounted
{
	friend class FE_nodeset;

private:
	FE_nodeset *fe_nodeset;
	DsLabelIterator *iter;
	cmzn_nodeiterator *nextIterator; // for linked list of active iterators in FE_nodeset

	// takes ownership of iter_in access count
	cmzn_nodeiterator(FE_nodeset *fe_nodeset_in, DsLabelIterator *iter_in) :
		fe_nodeset(fe_nodeset_in),
		iter(iter_in),
		nextIterator(0)
	{
	}

	virtual ~cmzn_nodeiterator()
	{
		if (this->fe_nodeset)
			this->fe_nodeset->removeNodeiterator(this);
		cmzn::Deaccess(this->iter);
	}

	void invalidate()
	{
		// sufficient to clear fe_nodeset as only called from FE_nodeset destructor
		this->fe_nodeset = 0;
	}

	template<class REFCOUNTED>
	friend inline void cmzn::Deaccess(REFCOUNTED*& nodeIterator);

public:

	/** @return  Non-accessed node or 0 if iteration ended or iterator invalidated */
	cmzn_node *nextNode()
	{
		if (this->fe_nodeset)
		{
			const DsLabelIndex nodeIndex = this->iter->nextIndex();
			return this->fe_nodeset->getNode(nodeIndex);
		}
		return 0;
	}

	void setIndex(DsLabelIndex index)
	{
		this->iter->setIndex(index);
	}
};

#endif /* !defined (FINITE_node_NODESET_HPP) */
