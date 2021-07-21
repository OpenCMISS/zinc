/***************************************************************************//**
 * FILE : cmiss_node_private.hpp
 *
 * Private header file of cmzn_node, cmzn_nodeset.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (CMZN_NODE_PRIVATE_HPP)
#define CMZN_NODE_PRIVATE_HPP

#include "opencmiss/zinc/node.h"
#include "opencmiss/zinc/fieldsubobjectgroup.h"
#include "finite_element/finite_element.h"
#include "datastore/labelschangelog.hpp"

struct cmzn_fieldmoduleevent;
struct FE_region;
class FE_nodeset;

class cmzn_node_value_label_conversion
{
public:
	static const char *to_string(enum cmzn_node_value_label label)
	{
		const char *enum_string = nullptr;
		switch (label)
		{
		case CMZN_NODE_VALUE_LABEL_VALUE:
			enum_string = "VALUE";
			break;
		case CMZN_NODE_VALUE_LABEL_D_DS1:
			enum_string = "D_DS1";
			break;
		case CMZN_NODE_VALUE_LABEL_D_DS2:
			enum_string = "D_DS2";
			break;
		case CMZN_NODE_VALUE_LABEL_D_DS3:
			enum_string = "D_DS3";
			break;
		case CMZN_NODE_VALUE_LABEL_D2_DS1DS2:
			enum_string = "D2_DS1DS2";
			break;
		case CMZN_NODE_VALUE_LABEL_D2_DS1DS3:
			enum_string = "D2_DS1DS3";
			break;
		case CMZN_NODE_VALUE_LABEL_D2_DS2DS3:
			enum_string = "D2_DS2DS3";
			break;
		case CMZN_NODE_VALUE_LABEL_D3_DS1DS2DS3:
			enum_string = "D3_DS1DS2DS3";
			break;
		default:
			break;
		}
		return enum_string;
	}
};

/***************************************************************************//**
 * Ensures all nodes of the supplied element are in this nodeset_group.
 * Candidate for external API.
 *
 * @param nodeset_group  The nodeset group to add nodes to. Must be a subgroup
 * for the master nodeset expected to own the element's nodes.
 * @param element  The element whose nodes are to be added. Face elements
 * inherit nodes from parent elements via field mappings.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
int cmzn_nodeset_group_add_element_nodes(
	cmzn_nodeset_group_id nodeset_group, cmzn_element_id element);

/***************************************************************************//**
 * Ensures all nodes of the supplied element are not in this nodeset_group.
 * Candidate for external API.
 *
 * @param nodeset_group  The nodeset group to remove nodes from. Must be a
 * subgroup for the master nodeset expected to own the element's nodes.
 * @param element  The element whose nodes are to be removed. Face elements
 * inherit nodes from parent elements via field mappings.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
int cmzn_nodeset_group_remove_element_nodes(
	cmzn_nodeset_group_id nodeset_group, cmzn_element_id element);

/** Internal use only
 * @return non-accessed fe_nodeset for this nodeset. Note this is the master
 * list of all nodes, even if called on a nodeset_group.
 */
FE_nodeset *cmzn_nodeset_get_FE_nodeset_internal(cmzn_nodeset_id nodeset);

/** Internal use only
 * @return non-accessed fe_region for this nodeset.
 */
FE_region *cmzn_nodeset_get_FE_region_internal(cmzn_nodeset_id nodeset);

/** Internal use only
 * @return non-accessed region for this nodeset.
 */
cmzn_region_id cmzn_nodeset_get_region_internal(cmzn_nodeset_id nodeset);

/** Internal use only.
 * @return non-accessed node group field for this nodeset, if any.
 */
cmzn_field_node_group *cmzn_nodeset_get_node_group_field_internal(cmzn_nodeset_id nodeset);

/** Internal use only
 * @return  True if nodeset represents data points.
 */
bool cmzn_nodeset_is_data_internal(cmzn_nodeset_id nodeset);

/**
 * If the name is of the form GROUP_NAME.NODESET_NAME. Create a nodeset group.
 * For internal use in command migration only.
 *
 * @param field_module  The field module the nodeset belongs to.
 * @param name  The name of the nodeset: GROUP_NAME.{nodes|datapoints}.
 * @return  Handle to the nodeset, or NULL if error, name already in use or no
 * such nodeset name.
 */
cmzn_nodeset_group_id cmzn_fieldmodule_create_nodeset_group_from_name_internal(
	cmzn_fieldmodule_id field_module, const char *nodeset_group_name);

struct cmzn_nodesetchanges
{
private:
	cmzn_fieldmoduleevent *event; // accessed
	DsLabelsChangeLog *changeLog; // Accessed from object obtained from correct mesh
	int access_count;

	cmzn_nodesetchanges(cmzn_fieldmoduleevent *eventIn, cmzn_nodeset *nodesetIn);
	~cmzn_nodesetchanges();

public:

	static cmzn_nodesetchanges *create(cmzn_fieldmoduleevent *eventIn, cmzn_nodeset *nodesetIn);

	cmzn_nodesetchanges *access()
	{
		++(this->access_count);
		return this;
	}

	static int deaccess(cmzn_nodesetchanges* &nodesetchanges);

	cmzn_node_change_flags getNodeChangeFlags(cmzn_node *node);

	int getNumberOfChanges()
	{
		if (this->changeLog->isAllChange())
			return -1;
		return this->changeLog->getChangeCount();
	}

	cmzn_node_change_flags getSummaryNodeChangeFlags()
	{
		return this->changeLog->getChangeSummary();
	}
};

#endif /* !defined (CMZN_NODE_PRIVATE_HPP) */
