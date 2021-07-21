/**
 * FILE : finite_element_nodeset.cpp
 *
 * Class defining a domain consisting of a set of nodes.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cstdlib>
#include <cstdio>
#include <vector>
#include "opencmiss/zinc/node.h"
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_mesh.hpp"
#include "finite_element/finite_element_nodeset.hpp"
#include "finite_element/finite_element_region.h"
#include "finite_element/finite_element_private.h"
#include "finite_element/finite_element_region_private.h"
#include "general/debug.h"
#include "general/message.h"
#include "general/mystring.h"
#include "general/object.h"

FE_node_field_info::FE_node_field_info(FE_nodeset *nodesetIn, struct LIST(FE_node_field) *nodeFieldListIn,
	int numberOfValuesIn) :
	number_of_values(numberOfValuesIn),
	values_storage_size(get_FE_node_field_list_values_storage_size(nodeFieldListIn)),
	node_field_list(nodeFieldListIn),
	nodeset(nodesetIn),
	access_count(1)
{
}

FE_node_field_info::~FE_node_field_info()
{
	if (0 == this->access_count)
	{
		if (this->nodeset)
			this->nodeset->remove_FE_node_field_info(this);
		DESTROY(LIST(FE_node_field))(&(this->node_field_list));
	}
	else
	{
		display_message(ERROR_MESSAGE, "~FE_node_field_info.  Non-zero access count");
	}
}

FE_node_field_info *FE_node_field_info::create(FE_nodeset *nodesetIn, struct LIST(FE_node_field) *nodeFieldListIn,
	int numberOfValuesIn)
{
	struct LIST(FE_node_field) *newNodeFieldList = CREATE_LIST(FE_node_field)();
	if ((!newNodeFieldList) ||
		(nodeFieldListIn) && (!COPY_LIST(FE_node_field)(newNodeFieldList, nodeFieldListIn)))
	{
		display_message(ERROR_MESSAGE, "FE_node_field_info::create.  Failed to copy node field lists");
		return nullptr;
	}
	return new FE_node_field_info(nodesetIn, newNodeFieldList, numberOfValuesIn);
}

bool FE_node_field_info::hasMatchingNodeFieldList(struct LIST(FE_node_field) *nodeFieldListIn) const
{
	if (NUMBER_IN_LIST(FE_node_field)(this->node_field_list) ==
		NUMBER_IN_LIST(FE_node_field)(nodeFieldListIn))
	{
		if (FOR_EACH_OBJECT_IN_LIST(FE_node_field)(FE_node_field_is_in_list,
			(void *)(this->node_field_list), nodeFieldListIn))
		{
			return true;
		}
	}
	return false;
}

namespace {

	/** Log field in <node_field> as RELATED_OBJECT_CHANGED in FE_region. */
	int FE_node_field_log_FE_field_change_related(
		struct FE_node_field *node_field, void *fe_region_void)
	{
		static_cast<FE_region *>(fe_region_void)->FE_field_change_related(node_field->field, CHANGE_LOG_RELATED_OBJECT_CHANGED(FE_field));
		return 1;
	}

}

void FE_node_field_info::logFieldsChangeRelated(FE_region *fe_region) const
{
	FOR_EACH_OBJECT_IN_LIST(FE_node_field)(
		FE_node_field_log_FE_field_change_related, static_cast<void *>(fe_region),
		this->node_field_list);
}

cmzn_node::~cmzn_node()
{
	// all nodes should be either templates with invalid index,
	// or have been invalidated by nodeset prior to being destroyed
	if (0 != this->access_count)
	{
		display_message(ERROR_MESSAGE,
			"cmzn_node::~cmzn_node.  Node has non-zero access count %d", this->access_count);
	}
	else if (DS_LABEL_IDENTIFIER_INVALID != this->index)
	{
		display_message(ERROR_MESSAGE,
			"cmzn_node::~cmzn_node.  Node has not been invalidated. Index = %d", this->index);
	}
	else
	{
		if (this->fields) // for template nodes only
			this->invalidate();
	}
}

int FE_node_field_invalidate(FE_node_field *nodeField, void *nodeVoid)
{
	cmzn_node *node = static_cast<cmzn_node*>(nodeVoid);
	if (nodeField->field->getValueType() == ELEMENT_XI_VALUE)
		return set_FE_nodal_element_xi_value(node, nodeField->field, /*component_number*/0, /*element*/nullptr, /*xi*/nullptr);
	return FE_node_field_free_values_storage_arrays(nodeField, node->values_storage);
}

void cmzn_node::invalidate()
{
	if (this->fields)
	{
		FOR_EACH_OBJECT_IN_LIST(FE_node_field)(
			FE_node_field_invalidate, static_cast<void*>(this), this->fields->node_field_list);
		FE_node_field_info::deaccess(this->fields);
	}
	DEALLOCATE(this->values_storage);
	this->index = DS_LABEL_INDEX_INVALID;
}

cmzn_node* cmzn_node::createFromTemplate(DsLabelIndex index, cmzn_node *template_node)
{
	// Assumes DS_LABEL_INDEX_INVALID == -1
	if ((index < DS_LABEL_INDEX_INVALID) || (nullptr == template_node) || (nullptr == template_node->fields))
	{
		display_message(ERROR_MESSAGE, "cmzn_node::createFromTemplate.  Invalid argument(s)");
		return nullptr;
	}
	cmzn_node *node = new cmzn_node();
	node->index = index;
	node->fields = template_node->fields->access();
	if (template_node->values_storage)
	{
		if (!allocate_and_copy_FE_node_values_storage(template_node, &node->values_storage))
		{
			display_message(ERROR_MESSAGE,
				"cmzn_node::createFromTemplate.  Could not copy values from template node");
			/* values_storage may be corrupt, so clear it */
			node->values_storage = nullptr;
			cmzn_node::deaccess(node);
		}
	}
	return node;
}

FE_node_template::FE_node_template(FE_nodeset *nodeset_in, struct FE_node_field_info *node_field_info) :
	cmzn::RefCounted(),
	nodeset(nodeset_in->access()),
	template_node(cmzn_node::createTemplate(node_field_info))
{
}

FE_node_template::FE_node_template(FE_nodeset *nodeset_in, cmzn_node *node) :
	cmzn::RefCounted(),
	nodeset(nodeset_in->access()),
	template_node(cmzn_node::createFromTemplate(DS_LABEL_INDEX_INVALID, node))
{
}

FE_node_template::~FE_node_template()
{
	FE_nodeset::deaccess(this->nodeset);
	cmzn_node::deaccess(this->template_node);
}

FE_nodeset::FE_nodeset(FE_region *fe_region) :
	fe_region(fe_region),
	domainType(CMZN_FIELD_DOMAIN_TYPE_INVALID),
	last_fe_node_field_info(0),
	changeLog(0),
	activeNodeIterators(0),
	access_count(1)
{
	this->createChangeLog();
}

FE_nodeset::~FE_nodeset()
{
	cmzn::Deaccess(this->changeLog);
	this->last_fe_node_field_info = 0;

	// remove pointers to this FE_nodeset as destroying
	cmzn_nodeiterator *nodeIterator = this->activeNodeIterators;
	while (nodeIterator)
	{
		nodeIterator->invalidate();
		nodeIterator = nodeIterator->nextIterator;
	}

	this->clear();

	for (std::list<FE_node_field_info*>::iterator iter = this->node_field_info_list.begin();
		iter != this->node_field_info_list.end(); ++iter)
	{
		(*iter)->nodeset = nullptr;
	}
}

/** Private: assumes current change log pointer is null or invalid */
void FE_nodeset::createChangeLog()
{
	cmzn::Deaccess(this->changeLog);
	this->changeLog = DsLabelsChangeLog::create(&this->labels);
	if (!this->changeLog)
		display_message(ERROR_MESSAGE, "FE_nodeset::createChangeLog.  Failed to create change log");
	this->last_fe_node_field_info = 0;
}

DsLabelsChangeLog *FE_nodeset::extractChangeLog()
{
	// take access count of changelog when extracting
	DsLabelsChangeLog *returnChangeLog = this->changeLog;
	this->changeLog = nullptr;
	this->createChangeLog();
	return returnChangeLog;
}

/**
 * Returns a struct FE_node_field_info for the supplied <fe_node_field_list>
 * with <number_of_values>. The fe_nodeset maintains an internal list of these
 * structures so they can be shared between nodes.
 * If <node_field_list> is omitted, an empty list is assumed.
 * @return  Accessed FE_node_field_info, or 0 on error.
 */
struct FE_node_field_info *FE_nodeset::get_FE_node_field_info(
	int number_of_values, struct LIST(FE_node_field) *fe_node_field_list)
{
	struct FE_node_field_info *existing_fe_node_field_info = nullptr;
	for (std::list<FE_node_field_info*>::iterator iter = this->node_field_info_list.begin();
		iter != this->node_field_info_list.end(); ++iter)
	{
		if (fe_node_field_list)
		{
			if ((*iter)->hasMatchingNodeFieldList(fe_node_field_list))
			{
				existing_fe_node_field_info = *iter;
				break;
			}
		}
		else if (0 == NUMBER_IN_LIST(FE_node_field)((*iter)->node_field_list))
		{
			existing_fe_node_field_info = *iter;
			break;
		}
	}
	struct FE_node_field_info *fe_node_field_info = nullptr;
	if (existing_fe_node_field_info)
	{
		const int existing_number_of_values = existing_fe_node_field_info->getNumberOfValues();
		if (number_of_values == existing_number_of_values)
		{
			fe_node_field_info = existing_fe_node_field_info->access();
		}
		else
		{
			display_message(ERROR_MESSAGE, "FE_nodeset::get_FE_node_field_info.  "
				"Existing node field information has %d values, not %d requested",
				existing_number_of_values, number_of_values);
		}
	}
	else
	{
		fe_node_field_info = FE_node_field_info::create(this,
			fe_node_field_list, number_of_values);
		if (fe_node_field_info)
		{
			this->node_field_info_list.push_back(fe_node_field_info);
		}
		else
		{
			display_message(ERROR_MESSAGE, "FE_nodeset::get_FE_node_field_info.  "
				"Could not create node field information");
		}
	}
	return (fe_node_field_info);
}

/**
 * Updates the pointer to <node_field_info_address> to point to a node_field info
 * which appends to the fields in <node_field_info_address> one <new_node_field>.
 * The node_field_info returned in <node_field_info_address> will be for the
 * <new_number_of_values>.
 * The nodeset maintains an internal list of these structures so they can be
 * shared between nodes.  This function allows a fast path when adding a single
 * field.  If the node_field passed in is only referenced by one external object
 * then it is assumed that this function can modify it rather than copying.  If
 * there are more references then the object is copied and then modified.
 * This function handles the access and deaccess of the <node_field_info_address>
 * as if it is just updating then there is nothing to do.
 */
int FE_nodeset::get_FE_node_field_info_adding_new_field(
	struct FE_node_field_info **node_field_info_address,
	struct FE_node_field *new_node_field, int new_number_of_values)
{
	int return_code;
	struct FE_node_field_info *existing_node_field_info, *new_node_field_info;

	if (node_field_info_address &&
		(nullptr != (existing_node_field_info = *node_field_info_address)))
	{
		return_code = 1;
		if (existing_node_field_info->usedOnce())
		{
			FE_node_field_info_add_node_field(existing_node_field_info,
				new_node_field, new_number_of_values);
			for (std::list<FE_node_field_info*>::iterator iter = this->node_field_info_list.begin();
				iter != this->node_field_info_list.end(); ++iter)
			{
				if ((*iter)->hasMatchingNodeFieldList(existing_node_field_info->node_field_list))
				{
					new_node_field_info = *iter;
					break;
				}
			}
			if (new_node_field_info)
			{
				FE_node_field_info::reaccess(*node_field_info_address, new_node_field_info);
			}
		}
		else
		{
			/* Need to copy after all */
			struct LIST(FE_node_field) *node_field_list = CREATE_LIST(FE_node_field)();
			if (COPY_LIST(FE_node_field)(node_field_list, existing_node_field_info->node_field_list))
			{
				/* add the new node field */
				if (ADD_OBJECT_TO_LIST(FE_node_field)(new_node_field, node_field_list))
				{
					/* create the new node information */
					new_node_field_info = this->get_FE_node_field_info(new_number_of_values, node_field_list);
					if (new_node_field_info)
					{
						if (*node_field_info_address)
							FE_node_field_info::deaccess(*node_field_info_address);
						*node_field_info_address = new_node_field_info;
					}
				}
			}
			DESTROY(LIST(FE_node_field))(&node_field_list);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_nodeset::get_FE_node_field_info_adding_new_field.  Invalid argument(s)");
		return_code = 0;
	}
	return (return_code);
}

/**
 * Returns a clone of <fe_node_field_info> that belongs to nodeset and
 * uses equivalent FE_fields and FE_time_sequences from <fe_region>.
 * Used to merge nodes from other FE_regions into this nodeset.
 * Returned object has an incremented access count that the caller takes over.
 * It is an error if an equivalent/same name FE_field is not found in nodeset.
 */
struct FE_node_field_info *FE_nodeset::clone_FE_node_field_info(
	struct FE_node_field_info *fe_node_field_info)
{
	struct FE_node_field_info *clone_fe_node_field_info = 0;
	if (fe_node_field_info)
	{
		struct LIST(FE_node_field) *fe_node_field_list = FE_node_field_list_clone_with_FE_field_list(
			fe_node_field_info->node_field_list,
			fe_region->fe_field_list, fe_region->fe_time);
		if (fe_node_field_list)
		{
			clone_fe_node_field_info = this->get_FE_node_field_info(
				fe_node_field_info->getNumberOfValues(),
				fe_node_field_list);
			DESTROY(LIST(FE_node_field))(&fe_node_field_list);
		}
		if (!clone_fe_node_field_info)
		{
			display_message(ERROR_MESSAGE,
				"FE_nodeset::clone_FE_node_field_info.  Failed");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_nodeset::clone_FE_node_field_info.  Invalid argument(s)");
	}
	return (clone_fe_node_field_info);
}

/** 
 * Provided EXCLUSIVELY for the use of DEACCESS and REACCESS functions.
 * Called when the access_count of <fe_node_field_info> drops to 1 so that
 * <fe_region> can destroy FE_node_field_info not in use.
 */
void FE_nodeset::remove_FE_node_field_info(struct FE_node_field_info *fe_node_field_info)
{
	if (fe_node_field_info == this->last_fe_node_field_info)
		this->last_fe_node_field_info = 0;
	this->node_field_info_list.remove(fe_node_field_info);
}

/** Assumes called by FE_region destructor, and change notification is disabled. */
void FE_nodeset::detach_from_FE_region()
{
	// clear embedded locations to avoid circular dependencies which prevent cleanup
	FE_nodeset_clear_embedded_locations(this, this->fe_region->fe_field_list);
	this->fe_region = 0;
}

/** Remove iterator from linked list in this nodeset */
void FE_nodeset::removeNodeiterator(cmzn_nodeiterator *iterator)
{
	if (iterator == this->activeNodeIterators)
		this->activeNodeIterators = iterator->nextIterator;
	else
	{
		cmzn_nodeiterator *prevIterator = this->activeNodeIterators;
		while (prevIterator && (prevIterator->nextIterator != iterator))
			prevIterator = prevIterator->nextIterator;
		if (prevIterator)
			prevIterator->nextIterator = iterator->nextIterator;
		else
			display_message(ERROR_MESSAGE, "FE_nodeset::removeNodeiterator.  Iterator not in linked list");
	}
	iterator->nextIterator = 0;
}

/**
* Create a node iterator object for iterating through the nodes of the
* nodeset. The iterator initially points at the position before the first node.
* @param labelsGroup  Optional group to iterate over.
* @return  Handle to node iterator at position before first, or NULL if error.
*/
cmzn_nodeiterator *FE_nodeset::createNodeiterator(DsLabelsGroup *labelsGroup)
{
	DsLabelIterator *labelIterator = labelsGroup ? labelsGroup->createLabelIterator() : this->labels.createLabelIterator();
	if (!labelIterator)
		return 0;
	cmzn_nodeiterator *iterator = new cmzn_nodeiterator(this, labelIterator);
	if (iterator)
	{
		iterator->nextIterator = this->activeNodeIterators;
		this->activeNodeIterators = iterator;
	}
	else
		cmzn::Deaccess(labelIterator);
	return iterator;
}

DsLabelsGroup *FE_nodeset::createLabelsGroup()
{
	return DsLabelsGroup::create(&this->labels); // GRC dodgy taking address here
}

/**
* Call this to mark node with the supplied change.
* Notifies change to clients of FE_region.
*/
void FE_nodeset::nodeChange(DsLabelIndex nodeIndex, int change)
{
	if (this->fe_region && this->changeLog)
	{
		this->changeLog->setIndexChange(nodeIndex, change);
		this->fe_region->FE_region_change();
		this->fe_region->update();
	}
}

/**
 * Call this to mark node with the supplied change, logging field changes
 * from the field_info_node in the fe_region.
 * Notifies change to clients of FE_region.
 * When a node is added or removed, the same node is used for <nodeIndex> and
 * <field_info_node>. For changes to the contents of <node>, <field_info_node>
 * should contain the changed fields, consistent with merging it into <node>.
*/
void FE_nodeset::nodeChange(DsLabelIndex nodeIndex, int change, cmzn_node *field_info_node)
{
	if (this->fe_region && this->changeLog && field_info_node)
	{
		this->changeLog->setIndexChange(nodeIndex, change);
		// for efficiency, the following marks field changes only if field info is different from last
		struct FE_node_field_info *temp_fe_node_field_info = field_info_node->getNodeFieldInfo();
		if (temp_fe_node_field_info != this->last_fe_node_field_info)
		{
			this->last_fe_node_field_info = temp_fe_node_field_info;
			temp_fe_node_field_info->logFieldsChangeRelated(this->fe_region);
		}
		this->fe_region->FE_region_change();
		this->fe_region->update();
	}
}

/**
 * Call this instead of nodeChange when exactly one field, <fe_field> of
 * <node> has changed.
 */
void FE_nodeset::nodeFieldChange(cmzn_node *node, FE_field *fe_field)
{
	if (this->fe_region && this->changeLog)
	{
		this->changeLog->setIndexChange(node->getIndex(), DS_LABEL_CHANGE_TYPE_RELATED);
		fe_region->FE_field_change(fe_field, CHANGE_LOG_RELATED_OBJECT_CHANGED(FE_field));
		this->fe_region->update();
	}
}

bool FE_nodeset::is_FE_field_in_use(struct FE_field *fe_field)
{
	for (std::list<FE_node_field_info*>::iterator iter = this->node_field_info_list.begin();
		iter != this->node_field_info_list.end(); ++iter)
	{
		if ((*iter)->getNodeField(fe_field))
		{
			// since nodes may still exist in the change_log,
			// must now check that no remaining nodes use fe_field
			// for now, if there are nodes then fe_field is in use
			if (0 < this->getSize())
				return true;
		}
	}
	return false;
}

// @return  Number of element references to node at nodeIndex
int FE_nodeset::getElementUsageCount(DsLabelIndex nodeIndex) const
{
	if (nodeIndex >= 0)
	{
		ElementUsageCountType *elementUsageCountAddress = this->elementUsageCount.getAddress(nodeIndex);
		if (elementUsageCountAddress)
			return static_cast<int>(*elementUsageCountAddress);
	}
	return 0;
}

// call when start using a node in an element, so we know it's in use
void FE_nodeset::incrementElementUsageCount(DsLabelIndex nodeIndex)
{
	if (nodeIndex >= 0)
	{
		ElementUsageCountType *elementUsageCountAddress = this->elementUsageCount.getAddress(nodeIndex);
		if (elementUsageCountAddress)
			++(*elementUsageCountAddress);
		else
			this->elementUsageCount.setValue(nodeIndex, 1);
	}
}

// call when end using a node in an element, to record no longer in use
void FE_nodeset::decrementElementUsageCount(DsLabelIndex nodeIndex)
{
	if (nodeIndex >= 0)
	{
		ElementUsageCountType *elementUsageCountAddress = this->elementUsageCount.getAddress(nodeIndex);
		if (elementUsageCountAddress)
		{
			if (*elementUsageCountAddress > 0)
				--(*elementUsageCountAddress);
			else
				display_message(ERROR_MESSAGE, "FE_nodeset::decrementElementUsageCount.  Count is already 0");
		}
		else
			display_message(ERROR_MESSAGE, "FE_nodeset::decrementElementUsageCount.  Missing count");
	}
}

/** @return  Name of nodeset, not to be freed. Currently restricted to
  * "nodes" or "datapoints". */
const char *FE_nodeset::getName() const
{
	if (this->domainType == CMZN_FIELD_DOMAIN_TYPE_NODES)
		return "nodes";
	if (this->domainType == CMZN_FIELD_DOMAIN_TYPE_DATAPOINTS)
		return "datapoints";
	return 0;
}

// Only to be called by FE_region_clear, or when all nodeset already removed
// to reclaim memory in labels and mapped arrays
void FE_nodeset::clear()
{
	if (0 < this->labels.getSize())
	{
		const DsLabelIndex indexLimit = this->labels.getIndexSize();
		cmzn_node *node;
		for (DsLabelIndex index = 0; index < indexLimit; ++index)
		{
			if (this->fe_nodes.getValue(index, node) && (node))
			{
				// must invalidate nodes in case elements holding on to them
				// BUT! Don't invalidate nodes that have been merged into another region
				if (FE_node_get_FE_nodeset(node) == this)
					node->invalidate();
				cmzn_node::deaccess(node);
			}
		}
	}
	this->fe_nodes.clear();
	this->labels.clear();
}

int FE_nodeset::change_FE_node_identifier(cmzn_node *node, DsLabelIdentifier new_identifier)
{
	if ((FE_node_get_FE_nodeset(node) == this) && (new_identifier >= 0))
	{
		const DsLabelIndex nodeIndex = node->getIndex();
		const DsLabelIdentifier currentIdentifier = this->getNodeIdentifier(nodeIndex);
		if (currentIdentifier >= 0)
		{
			int return_code = this->labels.setIdentifier(nodeIndex, new_identifier);
			if (return_code == CMZN_OK)
				this->nodeIdentifierChange(node);
			else if (return_code == CMZN_ERROR_ALREADY_EXISTS)
				display_message(ERROR_MESSAGE, "FE_nodeset::change_FE_node_identifier.  Identifier %d is already used in nodeset",
					new_identifier);
			else
				display_message(ERROR_MESSAGE, "FE_nodeset::change_FE_node_identifier.  Failed to set label identifier");
			return return_code;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_nodeset::change_FE_node_identifier.  node is not in this nodeset");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_nodeset::change_FE_node_identifier.  Invalid argument(s)");
	}
	return CMZN_ERROR_ARGUMENT;
}

/** Creates a template that is a copy of the existing node */
FE_node_template *FE_nodeset::create_FE_node_template(cmzn_node *node)
{
	if (FE_node_get_FE_nodeset(node) != this)
		return 0;
	FE_node_template *node_template = new FE_node_template(this, node);
	return node_template;
}

FE_node_template *FE_nodeset::create_FE_node_template()
{
	FE_node_template *node_template = new FE_node_template(this,
		this->get_FE_node_field_info(/*number_of_values*/0, (struct LIST(FE_node_field) *)NULL));
	return node_template;
}

/**
 * Convenience function returning an existing node with the identifier from
 * the nodeset, or if none found or if identifier is -1, a new node with
 * with the identifier (or the first available identifier if -1).
 * It is expected that the calling function has wrapped calls to this function
 * with FE_region_begin/end_change.
 * @return  Accessed node, or 0 on error.
 */
cmzn_node *FE_nodeset::get_or_create_FE_node_with_identifier(DsLabelIdentifier identifier)
{
	cmzn_node *node = 0;
	if (-1 <= identifier)
	{
		if (identifier >= 0)
			node = this->findNodeByIdentifier(identifier);
		if (node)
		{
			node->access();
		}
		else
		{
			FE_node_template *node_template = this->create_FE_node_template();
			node = this->create_FE_node(identifier, node_template);
			cmzn::Deaccess(node_template);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_nodeset::get_or_create_FE_node_with_identifier.  Invalid argument(s)");
	}
	return (node);
}

/**
* Checks the node_template is compatible with nodeset & that there is no
* existing node of supplied identifier, then creates node of that
* identifier as a copy of node_template and adds it to the nodeset.
*
* @param identifier  Non-negative integer identifier of new node, or -1 to
* automatically generate (starting at 1). Fails if supplied identifier already
* used by an existing node.
* @return  Accessed node, or 0 on error.
*/
cmzn_node *FE_nodeset::create_FE_node(DsLabelIdentifier identifier, FE_node_template *node_template)
{
	cmzn_node *new_node = 0;
	if ((-1 <= identifier) && node_template)
	{
		if (node_template->nodeset == this)
		{
			DsLabelIndex nodeIndex = (identifier < 0) ? this->labels.createLabel() : this->labels.createLabel(identifier);
			if (nodeIndex >= 0)
			{
				new_node = cmzn_node::createFromTemplate(nodeIndex, node_template->get_template_node());
				if ((new_node) && this->fe_nodes.setValue(nodeIndex, new_node))
				{
					new_node->access();
					this->nodeAddedChange(new_node);
				}
				else
				{
					display_message(ERROR_MESSAGE, "FE_nodeset::create_FE_node.  Failed to add node to list.");
					cmzn_node::deaccess(new_node);
					this->labels.removeLabel(nodeIndex);
				}
			}
			else
			{
				if (this->labels.findLabelByIdentifier(identifier) >= 0)
					display_message(ERROR_MESSAGE, "FE_nodeset::create_FE_node.  Identifier %d is already used in nodeset.",
						identifier);
				else
					display_message(ERROR_MESSAGE, "FE_nodeset::create_FE_node.  Could not create label");
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_nodeset::create_FE_node.  node template is incompatible with nodeset");
		}
	}
	return (new_node);
}

int FE_nodeset::merge_FE_node_template(cmzn_node *destination, FE_node_template *fe_node_template)
{
	if (fe_node_template
		&& (fe_node_template->nodeset == this)
		&& (FE_node_get_FE_nodeset(destination) == this))
	{
		if (::merge_FE_node(destination, fe_node_template->get_template_node()))
		{
			this->nodeChange(destination->getIndex(), DS_LABEL_CHANGE_TYPE_RELATED, fe_node_template->get_template_node());
			return CMZN_OK;
		}
		return CMZN_ERROR_GENERAL;
	}
	display_message(ERROR_MESSAGE, "Node merge.  Invalid argument(s)");
	return CMZN_ERROR_ARGUMENT;
}

/**
 *@return  Result code OK if undefined CMZN_NOT_FOUND if field undefined or
 *any other error if failed.
 */
int FE_nodeset::undefineFieldAtNode(cmzn_node *node, struct FE_field *fe_field)
{
	int result = ::undefine_FE_field_at_node(node, fe_field);
	if (result == CMZN_OK)
		this->nodeFieldChange(node, fe_field);
	return result;
}

/**
 * Calls <iterator_function> with <user_data> for each cmzn_node in <region>.
 */
int FE_nodeset::for_each_FE_node(LIST_ITERATOR_FUNCTION(cmzn_node) iterator_function, void *user_data_void)
{
	DsLabelIterator *iter = this->labels.createLabelIterator();
	if (!iter)
		return 0;
	int return_code = 1;
	DsLabelIndex nodeIndex;
	cmzn_node *node;
	while ((nodeIndex = iter->nextIndex()) != DS_LABEL_INDEX_INVALID)
	{
		node = this->getNode(nodeIndex);
		if (!node)
		{
			display_message(ERROR_MESSAGE, "FE_nodeset::for_each_FE_node.  No node at index");
			return_code = 0;
			break;
		}
		if (!iterator_function(node, user_data_void))
		{
			return_code = 0;
			break;
		}
	}
	cmzn::Deaccess(iter);
	return return_code;
}

/**
 * Removes node from the nodeset if not in use by elements.
 * Private function which avoids checking whether node is in nodeset.
 * @return OK on success ERROR_IN_USE if in use by element
 */
int FE_nodeset::remove_FE_node_private(cmzn_node *node)
{
	const DsLabelIndex nodeIndex = node->getIndex();
	if (this->getElementUsageCount(nodeIndex) > 0)
		return CMZN_ERROR_IN_USE;
	// must notify of change before invalidating node otherwise has no fields
	// assumes within begin/end change
	this->nodeRemovedChange(node);
	// clear cmzn_node entry but deaccess at end of this function
	this->fe_nodes.setValue(nodeIndex, 0);
	node->invalidate();
	this->labels.removeLabel(nodeIndex);
	cmzn_node::deaccess(node);
	return CMZN_OK;
}

/**
 * Destroys node if not in use by elements.
 * @return OK on success ERROR_IN_USE if in use by element,
 * or other error if more serious failure.
 */
int FE_nodeset::destroyNode(cmzn_node *node)
{
	if (this->containsNode(node))
	{
		FE_region_begin_change(this->fe_region);
		const int result = this->remove_FE_node_private(node);
		if (0 == this->labels.getSize())
			this->clear();
		FE_region_end_change(this->fe_region);
		return result;
	}
	display_message(ERROR_MESSAGE, "FE_nodeset::destroyNode.  Invalid argument(s)");
	return CMZN_ERROR_ARGUMENT;
}

/**
 * Destroy all nodes not in use by elements.
 * @return OK on success ERROR_IN_USE if any not removed because in use by
 * elements, or other error if more serious failure.
 */
int FE_nodeset::destroyAllNodes()
{
	int return_code = CMZN_OK;
	FE_region_begin_change(this->fe_region);
	// can't use an iterator as invalidated when node removed
	const DsLabelIndex indexLimit = this->labels.getIndexSize();
	cmzn_node *node;
	const bool contiguous = this->labels.isContiguous();
	for (DsLabelIndex index = 0; index < indexLimit; ++index)
	{
		// must handle holes left in identifier array by deleted nodes
		if (contiguous || (DS_LABEL_IDENTIFIER_INVALID != this->getNodeIdentifier(index)))
		{
			node = this->getNode(index);
			if (!node)
			{
				display_message(WARNING_MESSAGE, "FE_nodeset::destroyAllNodes.  No node at index");
				continue;
			}
			int result = this->remove_FE_node_private(node);
			if (result != CMZN_OK)
			{
				return_code = result;
				if (result != CMZN_ERROR_IN_USE)
					break;
			}
		}
	}
	if (0 == this->labels.getSize())
		this->clear();
	FE_region_end_change(this->fe_region);
	return (return_code);
}

/**
 * Destroy all the nodes in labelsGroup not in use by elements.
 * @return OK on success ERROR_IN_USE if any not removed because in use by
 * elements, or other error if more serious failure.
 */
int FE_nodeset::destroyNodesInGroup(DsLabelsGroup& labelsGroup)
{
	int return_code = CMZN_OK;
	FE_region_begin_change(this->fe_region);
	// can't use an iterator as invalidated when node removed
	DsLabelIndex index = -1; // DS_LABEL_INDEX_INVALID
	cmzn_node *node;
	while (labelsGroup.incrementIndex(index))
	{
		node = this->getNode(index);
		if (!node)
		{
			display_message(WARNING_MESSAGE, "FE_nodeset::destroyNodesInGroup.  No node at index");
			continue;
		}
		int result = this->remove_FE_node_private(node);
		if (result != CMZN_OK)
		{
			return_code = result;
			if (result != CMZN_ERROR_IN_USE)
				break;
		}
	}
	if (0 == this->labels.getSize())
		this->clear();
	FE_region_end_change(this->fe_region);
	return (return_code);
}

int FE_nodeset::get_last_FE_node_identifier()
{
	cmzn_nodeiterator *iter = this->createNodeiterator();
	cmzn_node *node = 0;
	cmzn_node *last_node = 0;
	while ((0 != (node = cmzn_nodeiterator_next_non_access(iter))))
	{
		last_node = node;
	}
	cmzn_nodeiterator_destroy(&iter);
	if (last_node)
		return get_FE_node_identifier(node);
	return 0;
}

bool FE_nodeset::FE_field_has_multiple_times(struct FE_field *fe_field) const
{
	for (std::list<FE_node_field_info*>::const_iterator iter = this->node_field_info_list.begin();
		iter != this->node_field_info_list.end(); ++iter)
	{
		const FE_node_field *node_field = (*iter)->getNodeField(fe_field);
		if (node_field)
		{
			if (node_field->time_sequence)
				return true;
			break;
		}
	}
	return false;
}

void FE_nodeset::list_btree_statistics()
{
	if (this->labels.getSize() > 0)
	{
		if (this->domainType == CMZN_FIELD_DOMAIN_TYPE_NODES)
			display_message(INFORMATION_MESSAGE, "Nodes:\n");
		else if (this->domainType == CMZN_FIELD_DOMAIN_TYPE_DATAPOINTS)
			display_message(INFORMATION_MESSAGE, "Datapoints:\n");
		else
			display_message(INFORMATION_MESSAGE, "General nodeset:\n");
		this->labels.list_storage_details();
	}
}

/**
 * Data for passing to FE_nodeset::merge_FE_node_external.
 */
struct FE_nodeset::Merge_FE_node_external_data
{
	FE_nodeset &source;
	FE_region *target_fe_region; // not accessed; needed to convert embedded locations
	/* use following array and number to build up matching pairs of old node
	   field info what they become in the global_fe_region.
	   Note these are ACCESSed */
	struct FE_node_field_info **matching_node_field_info;
	int number_of_matching_node_field_info;
	// arrays of embedded element_xi fields in pairs for source and target region
	std::vector<FE_field *> sourceTargetEmbeddedFields;
	// working space for pairs of source and target host elements for embedded nodes
	std::vector<cmzn_element*> sourceTargetHostElements;
	// working space for xi coordinates for embedded nodes
	std::vector<FE_value> hostXi;

	Merge_FE_node_external_data(FE_nodeset &sourceIn, FE_region *target_fe_region_in) :
		source(sourceIn),
		target_fe_region(target_fe_region_in),
		matching_node_field_info(0),
		number_of_matching_node_field_info(0)
	{
	}

	~Merge_FE_node_external_data()
	{
		if (this->matching_node_field_info)
		{
			for (int i = 2*this->number_of_matching_node_field_info - 1; 0 <= i; --i)
				FE_node_field_info::deaccess(this->matching_node_field_info[i]);
			DEALLOCATE(this->matching_node_field_info);
		}
	}

	bool addEmbeddedField(struct FE_field *sourceEmbeddedField)
	{
		FE_field *targetEmbeddedField = FE_region_get_FE_field_from_name(this->target_fe_region, get_FE_field_name(sourceEmbeddedField));
		if (!targetEmbeddedField)
			return false;
		this->sourceTargetEmbeddedFields.push_back(sourceEmbeddedField);
		this->sourceTargetEmbeddedFields.push_back(targetEmbeddedField);
		this->sourceTargetHostElements.resize(this->sourceTargetHostElements.size() + 2);
		this->hostXi.resize(this->hostXi.size() + MAXIMUM_ELEMENT_XI_DIMENSIONS);
		return true;
	}

	/**
	 * If <field> is embedded, ie. returns element_xi, adds it to the embedded_field
	 * array in the FE_nodeset::Merge_FE_node_external_data.
	 * @param merge_data_void  A struct FE_nodeset::Merge_FE_node_external_data *.
	 */
	static int addEmbeddedFieldIterator(struct FE_field *field,
		void *merge_data_void)
	{
		FE_nodeset::Merge_FE_node_external_data *merge_data =
			static_cast<FE_nodeset::Merge_FE_node_external_data *>(merge_data_void);
		if (ELEMENT_XI_VALUE == get_FE_field_value_type(field))
		{
			if (!merge_data->addEmbeddedField(field))
				return 0;
		}
		return 1;
	}

	bool findEmbeddedFields()
	{
		if (FOR_EACH_OBJECT_IN_LIST(FE_field)(addEmbeddedFieldIterator,
			(void *)this, source.get_FE_region()->fe_field_list))
		{
			return true;
		}
		display_message(ERROR_MESSAGE,
			"FE_nodeset::Merge_FE_node_external_data::findEmbeddedFields.  Could not find embedded fields");
		return false;
	}

	/**
	 * @return  Target node field info if match, 0 if don't have a match for source.
	 */
	FE_node_field_info *get_matching_FE_node_field_info(FE_node_field_info *source_node_field_info)
	{
		FE_node_field_info **matching_node_field_info = this->matching_node_field_info;
		for (int i = 0; i < this->number_of_matching_node_field_info; ++i)
		{
			if (*matching_node_field_info == source_node_field_info)
				return *(matching_node_field_info + 1);
			matching_node_field_info += 2;
		}
		return 0;
	}

	/**
	 * Record match between source_node_field_info and target_node_field_info.
	 * @return  true on success.
	 */
	bool add_matching_FE_node_field_info(
		FE_node_field_info *source_node_field_info, FE_node_field_info *target_node_field_info)
	{
		FE_node_field_info **matching_node_field_info;
		if (REALLOCATE(matching_node_field_info,
			this->matching_node_field_info, struct FE_node_field_info *,
			2*(this->number_of_matching_node_field_info + 1)))
		{
			matching_node_field_info[this->number_of_matching_node_field_info*2] =
				source_node_field_info->access();
			matching_node_field_info[this->number_of_matching_node_field_info*2 + 1] =
				target_node_field_info->access();
			this->matching_node_field_info = matching_node_field_info;
			++(this->number_of_matching_node_field_info);
			return true;
		}
		display_message(ERROR_MESSAGE,
			"FE_nodeset::Merge_FE_node_external_data::add_matching_FE_node_field_info.  Failed");
		return false;
	}

};

/**
 * Merge node from another nodeset, used when reading models from files into
 * temporary regions.
 * Before merging, substitutes into node an appropriate node field info
 * for this nodeset, and corresponding global elements for current elements in
 * embedded element:xi fields.
 * Since this changes information in the node the caller is required to
 * destroy the source nodeset immediately after calling this function on any
 * nodes from it. Operations such as findNodeByIdentifier will no longer
 * work as the node is given a new index for this nodeset.
 * @param node  Node from another region to merge in.
 */
int FE_nodeset::merge_FE_node_external(cmzn_node *node,
	Merge_FE_node_external_data &data)
{
	struct FE_node_field_info *sourceNodeFieldInfo = node->getNodeFieldInfo();
	if (!sourceNodeFieldInfo)
	{
		display_message(ERROR_MESSAGE, "FE_nodeset::merge_FE_node_external.  Invalid node");
		return 0;
	}

	// get pairs of source & target host elements for this node and clear source element during merge
	// target element is set later on target node so reverse element->node maps are updated
	const size_t embeddedFieldSize = data.sourceTargetEmbeddedFields.size();
	for (size_t f = 0; f < embeddedFieldSize; f += 2)
	{
		cmzn_element *sourceElement = nullptr;
		cmzn_element *targetElement = nullptr;
		FE_field *sourceEmbeddedField = data.sourceTargetEmbeddedFields[f];
		if (node->getNodeField(sourceEmbeddedField))
		{
			FE_value* xi = data.hostXi.data() + (f/2)*MAXIMUM_ELEMENT_XI_DIMENSIONS;  // stored for later
			// embedded fields have 1 component, just a VALUE (no derivatives) and no versions
			if (get_FE_nodal_element_xi_value(node, sourceEmbeddedField, /*component_number*/0, &sourceElement, xi))
			{
				if (sourceElement)
				{
					FE_field *targetEmbeddedField = data.sourceTargetEmbeddedFields[f + 1];
					FE_mesh *targetHostMesh = targetEmbeddedField->getElementXiHostMesh();
					targetElement = targetHostMesh->findElementByIdentifier(sourceElement->getIdentifier());
					// target element should exist as FE_mesh::mergePart1Elements should have been called first
					if (!targetElement)
					{
						display_message(ERROR_MESSAGE,
							"FE_nodeset::merge_FE_node_external.  Missing global element for embedded node.");
						return 0;
					}
					// clear source element for merge; xi has been stored separately for setting later
					set_FE_nodal_element_xi_value(node, sourceEmbeddedField, /*component_number*/0, /*element*/nullptr, /*xi*/nullptr);
				}
				else
				{
					// hack to guarantee target is set later; so can read a blank location over an element location
					sourceElement += 1;
				}
			}
		}
		data.sourceTargetHostElements[f] = sourceElement;
		data.sourceTargetHostElements[f + 1] = targetElement;
	}

	// get target node index from existing global node, or new index if none and set
	const DsLabelIndex sourceNodeIndex = node->getIndex();
	const DsLabelIdentifier identifier = node->getIdentifier();
	cmzn_node *globalNode = this->findNodeByIdentifier(identifier);
	const DsLabelIndex targetNodeIndex = (globalNode) ? globalNode->getIndex() :
		this->labels.createLabel(identifier);
	if (targetNodeIndex < 0)
	{
		display_message(ERROR_MESSAGE, "FE_nodeset::merge_FE_node_external.  Failed to get node index.");
		return 0;
	}
	int return_code = 1;
	// set target index; temporarily if merging into global node
	node->setIndex(targetNodeIndex);

	// convert node to use an equivalent node field info for target nodeset
	FE_node_field_info *targetNodeFieldInfo = data.get_matching_FE_node_field_info(sourceNodeFieldInfo);
	if (targetNodeFieldInfo)
	{
		targetNodeFieldInfo->access();
	}
	else
	{
		targetNodeFieldInfo = this->clone_FE_node_field_info(sourceNodeFieldInfo);
		if (!((targetNodeFieldInfo) && data.add_matching_FE_node_field_info(sourceNodeFieldInfo, targetNodeFieldInfo)))
		{
			display_message(ERROR_MESSAGE,
				"FE_nodeset::merge_FE_node_external.  Could not clone node field info");
			return_code = 0;
		}
	}
	if (globalNode)
		sourceNodeFieldInfo->access();  // so it stays around to restore later
	node->setNodeFieldInfo(targetNodeFieldInfo);

	// merge
	if (globalNode)
	{
		if (::merge_FE_node(globalNode, node, /*optimised_merge*/1))
		{
			this->nodeChange(globalNode->getIndex(), DS_LABEL_CHANGE_TYPE_RELATED, node);
		}
		else
		{
			return_code = 0;
		}
		// must restore the previous information for clean-up, don't care about old host elements
		node->setNodeFieldInfo(sourceNodeFieldInfo);
		FE_node_field_info::deaccess(sourceNodeFieldInfo);
		node->setIndex(sourceNodeIndex);
	}
	else
	{
		// transfer ownership of source node to this target nodeset
		// node will not be invalidated when source nodeset is cleared; see: FE_nodeset::clear();
		if (this->fe_nodes.setValue(targetNodeIndex, node))
		{
			node->access();
			this->nodeAddedChange(node);
		}
		else
		{
			display_message(ERROR_MESSAGE, "FE_nodeset::merge_FE_node_external.  Failed to add node to list.");
			this->labels.removeLabel(targetNodeIndex);
			return_code = 0;
		}
	}
	FE_node_field_info::deaccess(targetNodeFieldInfo);

	// set target host elements for embedded locations
	// if there is no global node, the incoming node is used directly
	cmzn_node *targetNode = (globalNode) ? globalNode : node;
	for (size_t f = 0; f < embeddedFieldSize; f += 2)
	{
		cmzn_element *sourceElement = data.sourceTargetHostElements[f];
		cmzn_element *targetElement = data.sourceTargetHostElements[f + 1];
		if (targetElement != sourceElement)
		{
			FE_value* xi = data.hostXi.data() + (f/2)*MAXIMUM_ELEMENT_XI_DIMENSIONS;  // stored from earlier
			FE_field *targetEmbeddedField = data.sourceTargetEmbeddedFields[f + 1];
			set_FE_nodal_element_xi_value(targetNode, targetEmbeddedField, /*component_number*/0, targetElement, xi);
		}
	}

	if (!return_code)
		display_message(ERROR_MESSAGE, "FE_nodeset::merge_FE_node_external.  Failed");
	return return_code;
}

void FE_nodeset::getHighestNodeFieldDerivativeAndVersion(FE_field *field,
	int& highestDerivative, int& highestVersion) const
{
	highestDerivative = 0;
	highestVersion = 0;
	for (std::list<FE_node_field_info*>::const_iterator iter = this->node_field_info_list.begin();
		iter != this->node_field_info_list.end(); ++iter)
	{
		const FE_node_field *node_field = (*iter)->getNodeField(field);
		if (node_field)
			node_field->expandHighestDerivativeAndVersion(highestDerivative, highestVersion);
	}
}

/*
 * Check that definition of fields in nodes of source_fe_region match that
 * of equivalent fields in equivalent nodes of global_fe_region
 */
bool FE_nodeset::canMerge(FE_nodeset &source)
{
	return true;  // no limitations if field definitions have been matched
#ifdef OLD_CODE
	bool result = true;

	/* for efficiency, pairs of FE_node_field_info from the source and target nodes
	   are stored if compatible to avoid checks later */
	int number_of_compatible_node_field_info = 0;
	struct FE_node_field_info **compatible_node_field_info = 0; // stores pairs: source, target

	struct FE_node_field_info **infoAddress, *sourceInfo, *targetInfo;
	cmzn_nodeiterator *iter = source.createNodeiterator();
	cmzn_node *sourceNode;
	while (0 != (sourceNode = cmzn_nodeiterator_next_non_access(iter)))
	{
		const DsLabelIdentifier identifier = get_FE_node_identifier(sourceNode);
		cmzn_node *targetNode = this->findNodeByIdentifier(identifier);
		if (!targetNode)
			continue;
		// fast path: check if the node_field_info have already been proven compatible */
		sourceInfo = sourceNode->getNodeFieldInfo();
		targetInfo = targetNode->getNodeFieldInfo();
		infoAddress = compatible_node_field_info;
		bool compatible = false;
		for (int i = 0; i < number_of_compatible_node_field_info; ++i)
		{
			if ((*infoAddress == sourceInfo) && (*(infoAddress + 1) == targetInfo))	
			{
				compatible = true;
				break;
			}
			infoAddress += 2;
		}
		if (compatible)
			continue;
		// slow full check:
		if (!FE_node_can_merge(targetNode, sourceNode))
		{
			display_message(ERROR_MESSAGE, "FE_nodeset::canMerge.  Nodes are not compatible");
			result = false;
			break;
		}
		/* store combination of source and target info for fast check in future */
		if (REALLOCATE(infoAddress, compatible_node_field_info,
			struct FE_node_field_info *,  2*(number_of_compatible_node_field_info + 1)))
		{
			compatible_node_field_info = infoAddress;
			compatible_node_field_info[number_of_compatible_node_field_info*2] = sourceInfo;
			compatible_node_field_info[number_of_compatible_node_field_info*2 + 1] = targetInfo;
			++number_of_compatible_node_field_info;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_nodeset::canMerge.  Could not reallocate compatible_node_field_info");
			result = false;
			break;
		}
	}
	cmzn_nodeiterator_destroy(&iter);
	if (compatible_node_field_info)
		DEALLOCATE(compatible_node_field_info);
	return result;
#endif
}

int FE_nodeset::merge(FE_nodeset &source)
{
	Merge_FE_node_external_data merge_data(source, this->fe_region);
	if (!merge_data.findEmbeddedFields())
		return 0;
	int return_code = 1;
	cmzn_nodeiterator *iter = source.createNodeiterator();
	cmzn_node *node;
	while (0 != (node = cmzn_nodeiterator_next_non_access(iter)))
	{
		if (!this->merge_FE_node_external(node, merge_data))
		{
			display_message(ERROR_MESSAGE, "FE_nodeset::merge.  Could not merge node");
			return_code = 0;
			break;
		}
	}
	cmzn_nodeiterator_destroy(&iter);
	return return_code;
}
