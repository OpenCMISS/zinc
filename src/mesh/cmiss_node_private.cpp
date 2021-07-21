/***************************************************************************//**
 * FILE : cmiss_node_private.cpp
 *
 * Implementation of public interface to cmzn_node.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdarg.h>
#include "opencmiss/zinc/fieldfiniteelement.h"
#include "opencmiss/zinc/fieldmodule.h"
#include "opencmiss/zinc/node.h"
#include "opencmiss/zinc/nodeset.h"
#include "opencmiss/zinc/nodetemplate.h"
#include "opencmiss/zinc/timesequence.h"
#include "opencmiss/zinc/status.h"
#include "computed_field/computed_field_finite_element.h"
#include "computed_field/computed_field_private.hpp"
#include "computed_field/computed_field_subobject_group.hpp"
#include "computed_field/field_module.hpp"
#include "general/debug.h"
#include "general/mystring.h"
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_nodeset.hpp"
#include "finite_element/finite_element_private.h"
#include "finite_element/finite_element_region.h"
#include "finite_element/node_field_template.hpp"
#include "general/message.h"
#include "general/enumerator_conversion.hpp"
#include "mesh/cmiss_node_private.hpp"
#include "node/node_operations.h"
#include <vector>

/*
Global types
------------
*/

/*============================================================================*/

namespace {

class cmzn_node_field
{
	FE_field *fe_field;
	std::vector<FE_node_field_template> componentNodefieldtemplates;
	FE_time_sequence *timesequence;

public:

	cmzn_node_field(FE_field *fe_field) :
		fe_field(ACCESS(FE_field)(fe_field)),
		componentNodefieldtemplates(get_FE_field_number_of_components(fe_field)),
		timesequence(NULL)
	{
	}

	~cmzn_node_field()
	{
		if (timesequence)
			DEACCESS(FE_time_sequence)(&timesequence);
		DEACCESS(FE_field)(&fe_field);
	}

	/** Set up node field with a copy of node field in use in node.
	  * @return  Result OK on success, any other value on failure. Client should clean up on failure. */
	int cloneNodeField(cmzn_node *node)
	{
		const FE_node_field *node_field = node->getNodeField(this->fe_field);
		if (!node_field)
		{
			return CMZN_ERROR_NOT_FOUND;
		}
		const int componentCount = get_FE_field_number_of_components(this->fe_field);
		for (int c = 0; c < componentCount; ++c)
		{
			this->componentNodefieldtemplates[c] = *(node_field->getComponent(c));
		}
		if (node_field->time_sequence)
		{
			this->setTimesequence(node_field->time_sequence);
		}
		return CMZN_OK;
	}

	/** note: does not ACCESS */
	cmzn_timesequence_id getTimesequence()
	{
		return reinterpret_cast<cmzn_timesequence_id>(timesequence);
	}

	int setTimesequence(FE_time_sequence *in_timesequence)
	{
		return REACCESS(FE_time_sequence)(&timesequence, in_timesequence);
	}

	int defineAtNode(FE_node *node)
	{
		return define_FE_field_at_node(node, this->fe_field, this->componentNodefieldtemplates.data(),
			this->timesequence);
	}

	int getValueNumberOfVersions(int componentNumber, cmzn_node_value_label valueLabel)
	{
		const int componentCount = get_FE_field_number_of_components(this->fe_field);
		if ((valueLabel < CMZN_NODE_VALUE_LABEL_VALUE)
			|| (valueLabel > CMZN_NODE_VALUE_LABEL_D3_DS1DS2DS3)
			|| (componentNumber < -1)
			|| (componentNumber == 0)
			|| (componentNumber > componentCount))
		{
			display_message(ERROR_MESSAGE, "Nodetemplate getValueNumberOfVersions.  Invalid arguments");
			return -1;
		}
		if (componentNumber == -1)
		{
			const int versionsCount = this->componentNodefieldtemplates[0].getValueNumberOfVersions(valueLabel);
			for (int c = 1; c < componentCount; ++c)
			{
				if (this->componentNodefieldtemplates[c].getValueNumberOfVersions(valueLabel) != versionsCount)
				{
					return -1; // not consistent over components
				}
			}
			return versionsCount;
		}
		return this->componentNodefieldtemplates[componentNumber - 1].getValueNumberOfVersions(valueLabel);
	}

	/** @param componentNumber  From 1 to number of components or -1 to set for all
	  * @param numberOfVersions  Number of versions > 1, or 0 to undefine. */
	int setValueNumberOfVersions(int componentNumber, cmzn_node_value_label valueLabel, int numberOfVersions)
	{
		const int componentCount = get_FE_field_number_of_components(this->fe_field);
		if ((valueLabel < CMZN_NODE_VALUE_LABEL_VALUE)
			|| (valueLabel > CMZN_NODE_VALUE_LABEL_D3_DS1DS2DS3)
			|| (componentNumber < -1)
			|| (componentNumber == 0)
			|| (componentNumber > componentCount)
			|| (numberOfVersions < 0))
		{
			display_message(ERROR_MESSAGE, "Nodetemplate setValueNumberOfVersions.  Invalid arguments");
			return CMZN_ERROR_ARGUMENT;
		}
		int first = 0;
		int limit = get_FE_field_number_of_components(fe_field);
		if (componentNumber > 0)
		{
			first = componentNumber - 1;
			limit = componentNumber;
		}
		for (int c = first; c < limit; ++c)
		{
			const int result = this->componentNodefieldtemplates[c].setValueNumberOfVersions(valueLabel, numberOfVersions);
			if (result != CMZN_OK)
			{
				if (((numberOfVersions == 0) && (result != CMZN_ERROR_NOT_FOUND))
					|| ((numberOfVersions > 0) && (result != CMZN_ERROR_ALREADY_EXISTS)))
				{
					display_message(ERROR_MESSAGE, "Nodetemplate setValueNumberOfVersions.  Failed");
					return result;
				}
			}
		}
		return CMZN_OK;
	}		

	FE_field *getFeField() const { return fe_field; }
};

}

/*============================================================================*/

struct cmzn_nodetemplate
{
private:
	FE_nodeset *fe_nodeset;
	FE_node_template *fe_node_template;
	std::vector<cmzn_node_field*> fields;
	std::vector<FE_field*> undefine_fields; // ACCESSed
	int access_count;

public:
	cmzn_nodetemplate(FE_nodeset *fe_nodeset_in) :
		fe_nodeset(fe_nodeset_in->access()),
		fe_node_template(0),
		access_count(1)
	{
	}

	cmzn_nodetemplate_id access()
	{
		++access_count;
		return this;
	}

	static int deaccess(cmzn_nodetemplate_id &node_template)
	{
		if (!node_template)
			return CMZN_ERROR_ARGUMENT;
		--(node_template->access_count);
		if (node_template->access_count <= 0)
			delete node_template;
		node_template = 0;
		return CMZN_OK;
	}

	int defineField(cmzn_field_id field)
	{
		int result = this->checkValidFieldForDefine(field);
		if (result != CMZN_OK)
		{
			display_message(ERROR_MESSAGE, "Nodetemplate defineField.  Invalid field");
			return result;
		}
		this->invalidate();
		FE_field *fe_field = 0;
		Computed_field_get_type_finite_element(field, &fe_field);
		cmzn_node_field *node_field = this->createNodeField(fe_field);
		if (!node_field)
		{
			this->removeDefineField(fe_field);
			return CMZN_ERROR_GENERAL;
		}
		// External behaviour defaults to having VALUE defined, which must removed if not wanted
		result = node_field->setValueNumberOfVersions(-1, CMZN_NODE_VALUE_LABEL_VALUE, 1);
		if (result != CMZN_OK)
		{
			return result;
		}
		return CMZN_OK;
	}

	int defineFieldFromNode(cmzn_field_id field, cmzn_node_id node)
	{
		if (!((node) && this->fe_nodeset->containsNode(node)))
		{
			display_message(ERROR_MESSAGE, "Nodetemplate defineFieldFromNode.  Invalid node");
			return CMZN_ERROR_ARGUMENT;
		}
		int result = this->checkValidFieldForDefine(field);
		if (result != CMZN_OK)
		{
			display_message(ERROR_MESSAGE, "Nodetemplate defineFieldFromNode.  Invalid field");
			return result;
		}
		this->invalidate();
		FE_field *fe_field = 0;
		Computed_field_get_type_finite_element(field, &fe_field);
		if (!(node->getNodeField(fe_field)))
		{
			this->removeDefineField(fe_field);
			this->removeUndefineField(fe_field);
			return CMZN_ERROR_NOT_FOUND;
		}
		cmzn_node_field *node_field = this->createNodeField(fe_field);
		if (!node_field)
		{
			return CMZN_ERROR_GENERAL;
		}
		result = node_field->cloneNodeField(node);
		if (result != CMZN_OK)
		{
			this->removeDefineField(fe_field);
			return result;
		}
		return CMZN_OK;
	}

	cmzn_timesequence_id getTimesequence(cmzn_field_id field)
	{
		cmzn_field_finite_element_id finite_element_field = cmzn_field_cast_finite_element(field);
		if (!finite_element_field)
			return 0;
		cmzn_field_finite_element_destroy(&finite_element_field);
		FE_field *fe_field = NULL;
		Computed_field_get_type_finite_element(field, &fe_field);
		cmzn_node_field *node_field = getNodeField(fe_field);
		if (!node_field)
			return 0;
		cmzn_timesequence_id timeSequence = node_field->getTimesequence();
		if (timeSequence)
		{
			cmzn_timesequence_access(timeSequence);
		}
		return timeSequence;
	}

	int setTimesequence(cmzn_field_id field, cmzn_timesequence_id timesequence)
	{
		cmzn_field_finite_element_id finite_element_field = cmzn_field_cast_finite_element(field);
		if (!finite_element_field)
		{
			display_message(ERROR_MESSAGE,
				"cmzn_nodetemplate_set_timesequence.  Field must be real finite_element type");
			return CMZN_ERROR_ARGUMENT;
		}
		cmzn_field_finite_element_destroy(&finite_element_field);
		FE_field *fe_field = NULL;
		Computed_field_get_type_finite_element(field, &fe_field);
		cmzn_node_field *node_field = getNodeField(fe_field);
		if (!node_field)
			return CMZN_ERROR_NOT_FOUND;
		this->invalidate();
		return node_field->setTimesequence(reinterpret_cast<struct FE_time_sequence *>(timesequence));
	}

	int getValueNumberOfVersions(cmzn_field_id field, int componentNumber,
		enum cmzn_node_value_label nodeValueLabel)
	{
		cmzn_field_finite_element_id finite_element_field = cmzn_field_cast_finite_element(field);
		if (!finite_element_field)
		{
			return -1;
		}
		cmzn_field_finite_element_destroy(&finite_element_field);
		FE_field *fe_field = NULL;
		Computed_field_get_type_finite_element(field, &fe_field);
		cmzn_node_field *node_field = this->getNodeField(fe_field);
		if (!node_field)
		{
			return -1;
		}
		return node_field->getValueNumberOfVersions(componentNumber, nodeValueLabel);
	}

	int setValueNumberOfVersions(cmzn_field_id field, int componentNumber,
		cmzn_node_value_label nodeValueLabel, int numberOfVersions)
	{
		cmzn_field_finite_element_id finite_element_field = cmzn_field_cast_finite_element(field);
		if (!finite_element_field)
		{
			display_message(ERROR_MESSAGE,
				"cmzn_nodetemplate_set_value_number_of_versions.  Field must be real finite element type");
			return CMZN_ERROR_ARGUMENT;
		}
		cmzn_field_finite_element_destroy(&finite_element_field);
		FE_field *fe_field = NULL;
		Computed_field_get_type_finite_element(field, &fe_field);
		cmzn_node_field *node_field = this->getNodeField(fe_field);
		if (!node_field)
			return CMZN_ERROR_NOT_FOUND;
		return node_field->setValueNumberOfVersions(componentNumber, nodeValueLabel, numberOfVersions);
	}

	int removeField(cmzn_field_id field)
	{
		int result = this->checkValidFieldForDefine(field);
		if (result != CMZN_OK)
			return result;
		FE_field *fe_field = NULL;
		Computed_field_get_type_finite_element(field, &fe_field);
		this->invalidate();
		if (this->removeDefineField(fe_field) || this->removeUndefineField(fe_field))
			return CMZN_OK;
		return CMZN_ERROR_NOT_FOUND;
	}

	int undefineField(cmzn_field_id field)
	{
		int result = this->checkValidFieldForDefine(field);
		if (result != CMZN_OK)
			return result;
		FE_field *fe_field = NULL;
		Computed_field_get_type_finite_element(field, &fe_field);
		this->invalidate();
		setUndefineNodeField(fe_field);
		return CMZN_OK;
	}

	int validate()
	{
		if (this->fe_node_template)
			return 1;
		this->fe_node_template = this->fe_nodeset->create_FE_node_template();
		for (unsigned int i = 0; i < fields.size(); i++)
		{
			if (!fields[i]->defineAtNode(this->fe_node_template->get_template_node()))
			{
				cmzn::Deaccess(this->fe_node_template);
				break;
			}
		}
		if (!this->fe_node_template)
		{
			display_message(ERROR_MESSAGE,
				"cmzn_nodetemplate_validate.  Failed to create fe_node_template");
			return 0;
		}
		return 1;
	}

	// can be made more efficient
	int mergeIntoNode(cmzn_node_id node)
	{
		FE_nodeset *target_fe_nodeset = FE_node_get_FE_nodeset(node);
		if (target_fe_nodeset == this->fe_nodeset)
		{
			if (this->validate())
			{
				int return_code = CMZN_OK;
				if (0 < undefine_fields.size())
				{
					FE_region_begin_change(this->fe_nodeset->get_FE_region());
					for (unsigned int i = 0; i < undefine_fields.size(); i++)
					{
						int result = this->fe_nodeset->undefineFieldAtNode(node, undefine_fields[i]);
						if ((result != CMZN_OK) && (result != CMZN_ERROR_NOT_FOUND))
						{
							return_code = result;
							break;
						}
					}
				}
				if ((return_code == CMZN_OK) && (0 < fields.size()))
					return_code = this->fe_nodeset->merge_FE_node_template(node, this->fe_node_template);
				if (0 < undefine_fields.size())
					FE_region_end_change(this->fe_nodeset->get_FE_region());
				return return_code;
			}
			else
				display_message(ERROR_MESSAGE, "cmzn_node_merge.  Node template is not valid");
		}
		else
			display_message(ERROR_MESSAGE, "cmzn_node_merge.  Incompatible template");
		return CMZN_ERROR_ARGUMENT;
	}

	FE_node_template *get_FE_node_template() { return this->fe_node_template; }

private:
	~cmzn_nodetemplate()
	{
		for (unsigned int i = 0; i < fields.size(); i++)
		{
			delete fields[i];
		}
		for (unsigned int i = 0; i < undefine_fields.size(); i++)
		{
			DEACCESS(FE_field)(&(undefine_fields[i]));
		}
		cmzn::Deaccess(this->fe_node_template);
		FE_nodeset::deaccess(fe_nodeset);
	}

	cmzn_node_field *getNodeField(FE_field *fe_field)
	{
		for (unsigned int i = 0; i < fields.size(); ++i)
			if (fields[i]->getFeField() == fe_field)
				return fields[i];
		return 0;
	}

	bool removeDefineField(FE_field *fe_field)
	{
		for (std::vector<cmzn_node_field*>::iterator iter = this->fields.begin(); iter != this->fields.end(); ++iter)
			if ((*iter)->getFeField() == fe_field)
			{
				delete *iter;
				this->fields.erase(iter);
				return true;
			}
		return false;
	}

	/** Replaces any existing define or undefine field for fe_field */
	cmzn_node_field *createNodeField(FE_field *fe_field)
	{
		cmzn_node_field *node_field = new cmzn_node_field(fe_field);
		// replace existing field in fields list
		for (unsigned int i = 0; i < fields.size(); ++i)
			if (this->fields[i]->getFeField() == fe_field)
			{
				delete fields[i];
				fields[i] = node_field;
				return node_field;
			}
		this->removeUndefineField(fe_field);
		fields.push_back(node_field);
		return node_field;
	}

	bool isUndefineNodeField(FE_field *fe_field)
	{
		for (unsigned int i = 0; i < undefine_fields.size(); ++i)
			if (undefine_fields[i] == fe_field)
				return true;
		return false;
	}

	bool removeUndefineField(FE_field *fe_field)
	{
		for (std::vector<FE_field*>::iterator iter = this->undefine_fields.begin(); iter != this->undefine_fields.end(); ++iter)
			if ((*iter) == fe_field)
			{
				DEACCESS(FE_field)(&fe_field);
				this->undefine_fields.erase(iter);
				return true;
			}
		return false;
	}

	/** Replaces any existing define or undefine field for fe_field */
	void setUndefineNodeField(FE_field *fe_field)
	{
		// if already in undefine list, nothing to do
		for (unsigned int i = 0; i < this->undefine_fields.size(); ++i)
			if (this->undefine_fields[i] == fe_field)
				return;
		this->removeDefineField(fe_field);
		ACCESS(FE_field)(fe_field);
		this->undefine_fields.push_back(fe_field);
	}

	void invalidate()
	{
		cmzn::Deaccess(this->fe_node_template);
	}

	int checkValidFieldForDefine(cmzn_field_id field)
	{
		FE_field *fe_field = 0;
		Computed_field_get_type_finite_element(field, &fe_field);
		if (!fe_field)
			return CMZN_ERROR_ARGUMENT;
		if (fe_field->get_FE_region() != this->fe_nodeset->get_FE_region())
			return CMZN_ERROR_INCOMPATIBLE_DATA;
		return CMZN_OK;
	}
};

/*============================================================================*/

struct cmzn_nodeset
{
protected:
	FE_nodeset *fe_nodeset;
	cmzn_field_node_group_id group;
	int access_count;

	cmzn_nodeset(cmzn_field_node_group_id group) :
		fe_nodeset(Computed_field_node_group_core_cast(group)->get_fe_nodeset()->access()),
		group(group),
		access_count(1)
	{
		// GRC cmzn_field_node_group_access missing:
		cmzn_field_access(cmzn_field_node_group_base_cast(group));
	}

public:
	cmzn_nodeset(FE_nodeset *fe_nodeset_in) :
		fe_nodeset(fe_nodeset_in->access()),
		group(0),
		access_count(1)
	{
	}

	cmzn_nodeset_id access()
	{
		++access_count;
		return this;
	}

	static int deaccess(cmzn_nodeset_id &nodeset)
	{
		if (!nodeset)
			return CMZN_ERROR_ARGUMENT;
		--(nodeset->access_count);
		if (nodeset->access_count <= 0)
			delete nodeset;
		nodeset = 0;
		return CMZN_OK;
	}

	bool containsNode(cmzn_node_id node)
	{
		if (group)
			return Computed_field_node_group_core_cast(group)->containsObject(node);
		return this->fe_nodeset->containsNode(node);
	}

	cmzn_node_id createNode(int identifier,
		cmzn_nodetemplate_id node_template)
	{
		cmzn_node_id node = 0;
		if (node_template->validate())
		{
			if (group)
				FE_region_begin_change(this->fe_nodeset->get_FE_region());
			node = this->fe_nodeset->create_FE_node(identifier, node_template->get_FE_node_template());
			if (group)
			{
				Computed_field_node_group_core_cast(group)->addObject(node);
				FE_region_end_change(this->fe_nodeset->get_FE_region());
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"cmzn_nodeset_create_node.  Node template is not valid");
		}
		return node;
	}

	cmzn_nodetemplate_id createNodetemplate()
	{
		return new cmzn_nodetemplate(this->fe_nodeset);
	}

	cmzn_nodeiterator_id createNodeiterator()
	{
		if (group)
			return Computed_field_node_group_core_cast(group)->createNodeiterator();
		return this->fe_nodeset->createNodeiterator();
	}

	int destroyAllNodes()
	{
		if (this->group)
		{
			Computed_field_node_group *node_group = Computed_field_node_group_core_cast(this->group);
			return this->fe_nodeset->destroyNodesInGroup(node_group->getLabelsGroup());
		}
		return this->fe_nodeset->destroyAllNodes();
	}

	int destroyNode(cmzn_node_id node)
	{
		if (this->containsNode(node))
			return this->fe_nodeset->destroyNode(node);
		return 0;
	}

	int destroyNodesConditional(cmzn_field_id conditional_field)
	{
		if (!conditional_field)
			return CMZN_ERROR_ARGUMENT;
		DsLabelsGroup *labelsGroup = this->fe_nodeset->createLabelsGroup();
		if (labelsGroup)
		{
			cmzn_region_id region = FE_region_get_cmzn_region(this->fe_nodeset->get_FE_region());
			cmzn_fieldmodule_id fieldmodule = cmzn_region_get_fieldmodule(region);
			cmzn_fieldcache_id cache = cmzn_fieldmodule_create_fieldcache(fieldmodule);
			cmzn_nodeiterator_id iterator = this->createNodeiterator();
			cmzn_node_id node = 0;
			while (0 != (node = cmzn_nodeiterator_next_non_access(iterator)))
			{
				cmzn_fieldcache_set_node(cache, node);
				if (cmzn_field_evaluate_boolean(conditional_field, cache))
					labelsGroup->setIndex(node->getIndex(), true);
			}
			cmzn::Deaccess(iterator);
			cmzn_fieldcache_destroy(&cache);
			cmzn_fieldmodule_destroy(&fieldmodule);
			int return_code = this->fe_nodeset->destroyNodesInGroup(*labelsGroup);
			cmzn::Deaccess(labelsGroup);
			return return_code;
		}
		return CMZN_ERROR_GENERAL;
	}


	cmzn_node_id findNodeByIdentifier(int identifier) const
	{
		cmzn_node_id node = 0;
		if (group)
			node = Computed_field_node_group_core_cast(group)->findNodeByIdentifier(identifier);
		else
			node = this->fe_nodeset->findNodeByIdentifier(identifier);
		if (node)
			node->access();
		return node;
	}

	FE_nodeset *getFeNodeset()
	{
		return this->fe_nodeset;
	}

	FE_region *getFeRegion() const { return fe_nodeset->get_FE_region(); }

	/** @return  Allocated name */
	char *getName()
	{
		if (group)
			return cmzn_field_get_name(cmzn_field_node_group_base_cast(group));
		else
			return duplicate_string(this->fe_nodeset->getName());
		return 0;
	}

	cmzn_nodeset_id getMaster()
	{
		if (!isGroup())
			return access();
		return new cmzn_nodeset(this->fe_nodeset);
	}

	int getSize() const
	{
		if (group)
			return Computed_field_node_group_core_cast(group)->getSize();
		return this->fe_nodeset->getSize();
	}

	int isGroup()
	{
		return (0 != group);
	}

	// if nodeset is a group, return the node group field, otherwise return 0
	cmzn_field_node_group *getGroupField()
	{
		return this->group;
	}

	bool match(cmzn_nodeset& other_nodeset)
	{
		return ((fe_nodeset == other_nodeset.fe_nodeset) &&
			(group == other_nodeset.group));
	}

protected:
	~cmzn_nodeset()
	{
		if (group)
			cmzn_field_node_group_destroy(&group);
		FE_nodeset::deaccess(fe_nodeset);
	}

};

struct cmzn_nodeset_group : public cmzn_nodeset
{
public:

	cmzn_nodeset_group(cmzn_field_node_group_id group) :
		cmzn_nodeset(group)
	{
	}

	int addNode(cmzn_node_id node)
	{
		return Computed_field_node_group_core_cast(group)->addObject(node);
	}

	int addNodesConditional(cmzn_field_id conditional_field)
	{
		return Computed_field_node_group_core_cast(group)->addNodesConditional(conditional_field);
	}

	int removeAllNodes()
	{
		return Computed_field_node_group_core_cast(group)->clear();
	}

	int removeNode(cmzn_node_id node)
	{
		return Computed_field_node_group_core_cast(group)->removeObject(node);
	}

	int removeNodesConditional(cmzn_field_id conditional_field)
	{
		return Computed_field_node_group_core_cast(group)->removeNodesConditional(conditional_field);
	}

	int addElementNodes(cmzn_element_id element)
	{
		return Computed_field_node_group_core_cast(group)->addElementNodes(element);
	}

	int removeElementNodes(cmzn_element_id element)
	{
		return Computed_field_node_group_core_cast(group)->removeElementNodes(element);
	}

};

/*
Global functions
----------------
*/

cmzn_nodeset_id cmzn_fieldmodule_find_nodeset_by_field_domain_type(
	cmzn_fieldmodule_id fieldmodule, enum cmzn_field_domain_type domain_type)
{
	cmzn_region_id region = cmzn_fieldmodule_get_region_internal(fieldmodule);
	FE_nodeset *fe_nodeset = FE_region_find_FE_nodeset_by_field_domain_type(
		region->get_FE_region(), domain_type);
	if (fe_nodeset)
		return new cmzn_nodeset(fe_nodeset);
	return 0;
}

cmzn_nodeset_id cmzn_fieldmodule_find_nodeset_by_name(
	cmzn_fieldmodule_id fieldmodule, const char *nodeset_name)
{
	cmzn_nodeset_id nodeset = 0;
	if (fieldmodule && nodeset_name)
	{
		cmzn_field_id field = cmzn_fieldmodule_find_field_by_name(fieldmodule, nodeset_name);
		if (field)
		{
			cmzn_field_node_group_id node_group_field = cmzn_field_cast_node_group(field);
			if (node_group_field)
			{
				nodeset = cmzn_nodeset_group_base_cast(cmzn_field_node_group_get_nodeset_group(node_group_field));
				cmzn_field_node_group_destroy(&node_group_field);
			}
			cmzn_field_destroy(&field);
		}
		else
		{
			if (0 == strcmp(nodeset_name, "nodes"))
			{
				nodeset = cmzn_fieldmodule_find_nodeset_by_field_domain_type(fieldmodule, CMZN_FIELD_DOMAIN_TYPE_NODES);
			}
			else if (0 == strcmp(nodeset_name, "datapoints"))
			{
				nodeset = cmzn_fieldmodule_find_nodeset_by_field_domain_type(fieldmodule, CMZN_FIELD_DOMAIN_TYPE_DATAPOINTS);
			}
		}
	}
	return nodeset;
}

cmzn_nodeset_id cmzn_nodeset_access(cmzn_nodeset_id nodeset)
{
	if (nodeset)
		return nodeset->access();
	return 0;
}

int cmzn_nodeset_destroy(cmzn_nodeset_id *nodeset_address)
{
	if (nodeset_address)
		return cmzn_nodeset::deaccess(*nodeset_address);
	return CMZN_ERROR_ARGUMENT;
}

bool cmzn_nodeset_contains_node(cmzn_nodeset_id nodeset, cmzn_node_id node)
{
	if (nodeset && node)
		return nodeset->containsNode(node);
	return false;
}

cmzn_nodetemplate_id cmzn_nodeset_create_nodetemplate(
	cmzn_nodeset_id nodeset)
{
	if (nodeset)
		return nodeset->createNodetemplate();
	return 0;
}

cmzn_node_id cmzn_nodeset_create_node(cmzn_nodeset_id nodeset,
	int identifier, cmzn_nodetemplate_id node_template)
{
	if (nodeset && node_template)
		return nodeset->createNode(identifier, node_template);
	return 0;
}

cmzn_nodeiterator_id cmzn_nodeset_create_nodeiterator(
	cmzn_nodeset_id nodeset)
{
	if (nodeset)
		return nodeset->createNodeiterator();
	return 0;
}

cmzn_node_id cmzn_nodeset_find_node_by_identifier(cmzn_nodeset_id nodeset,
	int identifier)
{
	if (nodeset)
		return nodeset->findNodeByIdentifier(identifier);
	return 0;
}

char *cmzn_nodeset_get_name(cmzn_nodeset_id nodeset)
{
	if (nodeset)
		return nodeset->getName();
	return 0;
}

int cmzn_nodeset_get_size(cmzn_nodeset_id nodeset)
{
	if (nodeset)
		return nodeset->getSize();
	return 0;
}

int cmzn_nodeset_destroy_all_nodes(cmzn_nodeset_id nodeset)
{
	if (nodeset)
		return nodeset->destroyAllNodes();
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_nodeset_destroy_node(cmzn_nodeset_id nodeset, cmzn_node_id node)
{
	if (nodeset && node)
		return nodeset->destroyNode(node);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_nodeset_destroy_nodes_conditional(cmzn_nodeset_id nodeset,
	cmzn_field_id conditional_field)
{
	if (nodeset && conditional_field)
		return nodeset->destroyNodesConditional(conditional_field);
	return CMZN_ERROR_ARGUMENT;
}

cmzn_fieldmodule_id cmzn_nodeset_get_fieldmodule(cmzn_nodeset_id nodeset)
{
	if (nodeset)
	{
		cmzn_region *region = FE_region_get_cmzn_region(nodeset->getFeRegion());
		return cmzn_fieldmodule_create(region);
	}
	return 0;
}

cmzn_nodeset_id cmzn_nodeset_get_master_nodeset(cmzn_nodeset_id nodeset)
{
	if (nodeset)
		return nodeset->getMaster();
	return 0;
}

bool cmzn_nodeset_match(cmzn_nodeset_id nodeset1, cmzn_nodeset_id nodeset2)
{
	return (nodeset1 && nodeset2 && nodeset1->match(*nodeset2));
}

cmzn_nodeset_group_id cmzn_nodeset_cast_group(cmzn_nodeset_id nodeset)
{
	if (nodeset && nodeset->isGroup())
		return static_cast<cmzn_nodeset_group_id>(nodeset->access());
	return 0;
}

int cmzn_nodeset_group_destroy(cmzn_nodeset_group_id *nodeset_group_address)
{
	if (nodeset_group_address)
		return cmzn_nodeset::deaccess(*(reinterpret_cast<cmzn_nodeset_id*>(nodeset_group_address)));
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_nodeset_group_add_node(cmzn_nodeset_group_id nodeset_group, cmzn_node_id node)
{
	if (nodeset_group)
		return nodeset_group->addNode(node);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_nodeset_group_add_nodes_conditional(
	cmzn_nodeset_group_id nodeset_group, cmzn_field_id conditional_field)
{
	if (nodeset_group)
		return nodeset_group->addNodesConditional(conditional_field);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_nodeset_group_remove_all_nodes(cmzn_nodeset_group_id nodeset_group)
{
	if (nodeset_group)
		return nodeset_group->removeAllNodes();
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_nodeset_group_remove_node(cmzn_nodeset_group_id nodeset_group, cmzn_node_id node)
{
	if (nodeset_group)
		return nodeset_group->removeNode(node);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_nodeset_group_remove_nodes_conditional(cmzn_nodeset_group_id nodeset_group,
	cmzn_field_id conditional_field)
{
	if (nodeset_group)
		return nodeset_group->removeNodesConditional(conditional_field);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_nodeset_group_add_element_nodes(
	cmzn_nodeset_group_id nodeset_group, cmzn_element_id element)
{
	if (nodeset_group)
		return nodeset_group->addElementNodes(element);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_nodeset_group_remove_element_nodes(
	cmzn_nodeset_group_id nodeset_group, cmzn_element_id element)
{
	if (nodeset_group)
		return nodeset_group->removeElementNodes(element);
	return CMZN_ERROR_ARGUMENT;
}

cmzn_nodeset_group_id cmzn_field_node_group_get_nodeset_group(
	cmzn_field_node_group_id node_group)
{
	if (node_group)
		return new cmzn_nodeset_group(node_group);
	return 0;
}

FE_nodeset *cmzn_nodeset_get_FE_nodeset_internal(cmzn_nodeset_id nodeset)
{
	if (nodeset)
		return nodeset->getFeNodeset();
	return 0;
}

FE_region *cmzn_nodeset_get_FE_region_internal(cmzn_nodeset_id nodeset)
{
	if (nodeset)
		return nodeset->getFeRegion();
	return 0;
}

cmzn_region_id cmzn_nodeset_get_region_internal(cmzn_nodeset_id nodeset)
{
	if (nodeset)
		return FE_region_get_cmzn_region(nodeset->getFeRegion());
	return 0;
}

cmzn_field_node_group *cmzn_nodeset_get_node_group_field_internal(cmzn_nodeset_id nodeset)
{
	if (nodeset)
		return nodeset->getGroupField();
	return 0;
}

bool cmzn_nodeset_is_data_internal(cmzn_nodeset_id nodeset)
{
	if (nodeset)
		return (nodeset->getFeNodeset()->getFieldDomainType() == CMZN_FIELD_DOMAIN_TYPE_DATAPOINTS);
	return false;
}

cmzn_nodeset_group_id cmzn_fieldmodule_create_nodeset_group_from_name_internal(
	cmzn_fieldmodule_id fieldmodule, const char *nodeset_group_name)
{
	cmzn_nodeset_group_id nodeset_group = 0;
	if (fieldmodule && nodeset_group_name)
	{
		cmzn_field_id existing_field = cmzn_fieldmodule_find_field_by_name(fieldmodule, nodeset_group_name);
		if (existing_field)
		{
			cmzn_field_destroy(&existing_field);
		}
		else
		{
			char *group_name = duplicate_string(nodeset_group_name);
			char *nodeset_name = strrchr(group_name, '.');
			if (nodeset_name)
			{
				*nodeset_name = '\0';
				++nodeset_name;
				cmzn_nodeset_id master_nodeset = cmzn_fieldmodule_find_nodeset_by_name(fieldmodule, nodeset_name);
				cmzn_field_id field = cmzn_fieldmodule_find_field_by_name(fieldmodule, group_name);
				cmzn_field_group_id group = cmzn_field_cast_group(field);
				cmzn_field_node_group_id node_group = cmzn_field_group_create_field_node_group(group, master_nodeset);
				nodeset_group = cmzn_field_node_group_get_nodeset_group(node_group);
				cmzn_field_node_group_destroy(&node_group);
				cmzn_field_group_destroy(&group);
				cmzn_field_destroy(&field);
				cmzn_nodeset_destroy(&master_nodeset);
			}
			DEALLOCATE(group_name);
		}
	}
	return nodeset_group;
}

cmzn_nodetemplate_id cmzn_nodetemplate_access(cmzn_nodetemplate_id node_template)
{
	if (node_template)
		return node_template->access();
	return 0;
}

int cmzn_nodetemplate_destroy(cmzn_nodetemplate_id *node_template_address)
{
	if (node_template_address)
		return cmzn_nodetemplate::deaccess(*node_template_address);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_nodetemplate_define_field(cmzn_nodetemplate_id node_template,
	cmzn_field_id field)
{
	if (node_template)
		return node_template->defineField(field);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_nodetemplate_define_field_from_node(
	cmzn_nodetemplate_id node_template, cmzn_field_id field,
	cmzn_node_id node)
{
	if (node_template)
		return node_template->defineFieldFromNode(field, node);
	return CMZN_ERROR_ARGUMENT;
}

cmzn_timesequence_id cmzn_nodetemplate_get_timesequence(
	cmzn_nodetemplate_id node_template, cmzn_field_id field)
{
	if (node_template && field)
		return node_template->getTimesequence(field);
	return 0;
}

int cmzn_nodetemplate_set_timesequence(
	cmzn_nodetemplate_id node_template, cmzn_field_id field,
	cmzn_timesequence_id timesequence)
{
	if (node_template && field && timesequence)
		return node_template->setTimesequence(field, timesequence);
	return 0;
}

int cmzn_nodetemplate_get_value_number_of_versions(
	cmzn_nodetemplate_id node_template, cmzn_field_id field, int component_number,
	enum cmzn_node_value_label node_value_label)
{
	if (node_template)
		return node_template->getValueNumberOfVersions(field, component_number, node_value_label);
	return -1;
}

int cmzn_nodetemplate_set_value_number_of_versions(
	cmzn_nodetemplate_id node_template, cmzn_field_id field, int component_number,
	enum cmzn_node_value_label node_value_label, int number_of_versions)
{
	if (node_template)
		return node_template->setValueNumberOfVersions(field, component_number, node_value_label, number_of_versions);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_nodetemplate_remove_field(cmzn_nodetemplate_id node_template,
	cmzn_field_id field)
{
	if (node_template)
		return node_template->removeField(field);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_nodetemplate_undefine_field(cmzn_nodetemplate_id node_template,
	cmzn_field_id field)
{
	if (node_template)
		return node_template->undefineField(field);
	return CMZN_ERROR_ARGUMENT;
}

cmzn_node_id cmzn_node_access(cmzn_node_id node)
{
	if (node)
		return node->access();
	return 0;
}

int cmzn_node_destroy(cmzn_node_id *node_address)
{
	if (node_address && *node_address)
	{
		cmzn_node::deaccess(*node_address);
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_node_get_identifier(cmzn_node_id node)
{
	return get_FE_node_identifier(node);
}

cmzn_nodeset_id cmzn_node_get_nodeset(cmzn_node_id node)
{
	FE_nodeset *fe_nodeset = FE_node_get_FE_nodeset(node);
	if (fe_nodeset)
		return new cmzn_nodeset(fe_nodeset);
	return 0;
}

int cmzn_node_merge(cmzn_node_id node, cmzn_nodetemplate_id node_template)
{
	if (node && node_template)
		return node_template->mergeIntoNode(node);
	return 0;
}

enum cmzn_node_value_label cmzn_node_value_label_enum_from_string(
	const char *string)
{
	return string_to_enum<enum cmzn_node_value_label, cmzn_node_value_label_conversion>(string);
}

char *cmzn_node_value_label_enum_to_string(enum cmzn_node_value_label label)
{
	const char *label_string = cmzn_node_value_label_conversion::to_string(label);
	return (label_string ? duplicate_string(label_string) : 0);
}

cmzn_nodesetchanges::cmzn_nodesetchanges(cmzn_fieldmoduleevent *eventIn, cmzn_nodeset *nodesetIn) :
	event(eventIn->access()),
	changeLog(event->getFeRegionChanges()->getNodeChangeLog(
		cmzn_nodeset_get_FE_nodeset_internal(nodesetIn)->getFieldDomainType())),
	access_count(1)
{
}

cmzn_nodesetchanges::~cmzn_nodesetchanges()
{
	cmzn_fieldmoduleevent::deaccess(this->event);
}

cmzn_nodesetchanges *cmzn_nodesetchanges::create(cmzn_fieldmoduleevent *eventIn, cmzn_nodeset *nodesetIn)
{
	if (eventIn && (eventIn->getFeRegionChanges()) && nodesetIn &&
		(eventIn->getRegion()->get_FE_region() == cmzn_nodeset_get_FE_region_internal(nodesetIn)))
		return new cmzn_nodesetchanges(eventIn, nodesetIn);
	return 0;
}

int cmzn_nodesetchanges::deaccess(cmzn_nodesetchanges* &nodesetchanges)
{
	if (nodesetchanges)
	{
		--(nodesetchanges->access_count);
		if (nodesetchanges->access_count <= 0)
			delete nodesetchanges;
		nodesetchanges = 0;
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_node_change_flags cmzn_nodesetchanges::getNodeChangeFlags(cmzn_node *node)
{
	cmzn_node_change_flags change = CMZN_NODE_CHANGE_FLAG_NONE;
	if (node)
	{
		if (this->changeLog->isIndexChange(node->getIndex()))
			change = this->changeLog->getChangeSummary();
	}
	return change;
}

cmzn_nodesetchanges_id cmzn_nodesetchanges_access(
	cmzn_nodesetchanges_id nodesetchanges)
{
	if (nodesetchanges)
		return nodesetchanges->access();
	return 0;
}

int cmzn_nodesetchanges_destroy(cmzn_nodesetchanges_id *nodesetchanges_address)
{
	if (nodesetchanges_address)
		return cmzn_nodesetchanges::deaccess(*nodesetchanges_address);
	return CMZN_ERROR_ARGUMENT;
}

cmzn_node_change_flags cmzn_nodesetchanges_get_node_change_flags(
	cmzn_nodesetchanges_id nodesetchanges, cmzn_node_id node)
{
	if (nodesetchanges && node)
		return nodesetchanges->getNodeChangeFlags(node);
	return CMZN_NODE_CHANGE_FLAG_NONE;
}

int cmzn_nodesetchanges_get_number_of_changes(
	cmzn_nodesetchanges_id nodesetchanges)
{
	if (nodesetchanges)
		return nodesetchanges->getNumberOfChanges();
	return 0;
}

cmzn_node_change_flags cmzn_nodesetchanges_get_summary_node_change_flags(
	cmzn_nodesetchanges_id nodesetchanges)
{
	if (nodesetchanges)
		return nodesetchanges->getSummaryNodeChangeFlags();
	return CMZN_NODE_CHANGE_FLAG_NONE;
}
