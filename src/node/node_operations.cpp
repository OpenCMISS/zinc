/*******************************************************************************
FILE : node_operations.c

LAST MODIFIED : 17 January 2003

DESCRIPTION :
FE_node functions that utilise non finite element data structures and therefore
cannot reside in finite element modules.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stdlib.h>
#include <math.h>

#include "opencmiss/zinc/fieldcache.h"
#include "opencmiss/zinc/fieldconstant.h"
#include "opencmiss/zinc/fieldlogicaloperators.h"
#include "opencmiss/zinc/fieldmodule.h"
#include "opencmiss/zinc/fieldsubobjectgroup.h"
#include "opencmiss/zinc/fieldtime.h"
#include "opencmiss/zinc/nodeset.h"
#include "computed_field/computed_field_subobject_group.hpp"
#include "finite_element/finite_element_nodeset.hpp"
#include "finite_element/finite_element_region.h"
#include "general/debug.h"
#include "mesh/cmiss_node_private.hpp"
#include "node/node_operations.h"
#include "general/message.h"

/*
Global functions
----------------
*/

struct FE_node_values_number
/*******************************************************************************
LAST MODIFIED : 22 December 2000

DESCRIPTION :
Data for changing node identifiers.
==============================================================================*/
{
	struct FE_node *node;
	int number_of_values;
	FE_value *values;
	int new_number;
};

static int compare_FE_node_values_number_values(
	const void *node_values1_void, const void *node_values2_void)
/*******************************************************************************
LAST MODIFIED : 22 December 2000

DESCRIPTION :
Compares the values in <node_values1> and <node_values2> from the last to the
first, returning -1 as soon as a value in <node_values1> is less than its
counterpart in <node_values2>, or 1 if greater. 0 is returned if all values
are identival. Used as a compare function for qsort.
==============================================================================*/
{
	int i, number_of_values, return_code;
	struct FE_node_values_number *node_values1, *node_values2;

	ENTER(compare_FE_node_values_number_values);
	return_code = 0;
	if ((node_values1 = (struct FE_node_values_number *)node_values1_void) &&
		(node_values2 = (struct FE_node_values_number *)node_values2_void) &&
		(0 < (number_of_values = node_values1->number_of_values)) &&
		(number_of_values == node_values2->number_of_values))
	{
		for (i = number_of_values - 1; (!return_code) && (0 <= i); i--)
		{
			if (node_values1->values[i] < node_values2->values[i])
			{
				return_code = -1;
			}
			else if (node_values1->values[i] > node_values2->values[i])
			{
				return_code = 1;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"compare_FE_node_values_number_values.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* compare_FE_node_values_number_values */

struct FE_node_and_values_to_array_data
{
	cmzn_fieldcache_id field_cache;
	struct FE_node_values_number *node_values;
	struct Computed_field *sort_by_field;
	int number_of_values;
}; /* FE_node_and_values_to_array_data */

static int FE_node_and_values_to_array(struct FE_node *node,
	void *array_data_void)
{
	int return_code;
	struct FE_node_and_values_to_array_data *array_data;

	ENTER(FE_node_and_values_to_array);
	if (node && (array_data =
		(struct FE_node_and_values_to_array_data *)array_data_void) &&
		array_data->node_values)
	{
		return_code = 1;
		cmzn_fieldcache_set_node(array_data->field_cache, node);
		array_data->node_values->node = node;
		if (array_data->sort_by_field)
		{
			if ((0 == array_data->node_values->values) ||
				(CMZN_OK != cmzn_field_evaluate_real(array_data->sort_by_field, array_data->field_cache,
					array_data->number_of_values, array_data->node_values->values)))
			{
				display_message(ERROR_MESSAGE, "FE_node_and_values_to_array.  "
					"sort_by field could not be evaluated at node");
				return_code = 0;
			}
		}
		(array_data->node_values)++;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_node_and_values_to_array.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_node_and_values_to_array */

int cmzn_nodeset_change_node_identifiers(cmzn_nodeset_id nodeset,
	int node_offset, cmzn_field_id sort_by_field, FE_value time)
{
	int i, next_spare_node_number, number_of_nodes, number_of_values, return_code;
	struct FE_node *node_with_identifier;
	struct FE_node_and_values_to_array_data array_data;
	struct FE_node_values_number *node_values;

	if (nodeset)
	{
		FE_nodeset *fe_nodeset = cmzn_nodeset_get_FE_nodeset_internal(nodeset);
		FE_region *fe_region = fe_nodeset->get_FE_region();
		FE_region_begin_change(fe_region);
		return_code = 1;
		number_of_nodes = cmzn_nodeset_get_size(nodeset);
		if (0 < number_of_nodes)
		{
			cmzn_fieldmodule_id field_module;
			cmzn_fieldcache_id field_cache;
			if (sort_by_field)
			{
				number_of_values = cmzn_field_get_number_of_components(sort_by_field);
			}
			else
			{
				number_of_values = 0;
			}
			if (ALLOCATE(node_values, struct FE_node_values_number,
				number_of_nodes))
			{
				for (i = 0; i < number_of_nodes; i++)
				{
					node_values[i].number_of_values = number_of_values;
					node_values[i].values = (FE_value *)NULL;
				}
				if (sort_by_field)
				{
					for (i = 0; (i < number_of_nodes) && return_code; i++)
					{
						if (!ALLOCATE(node_values[i].values, FE_value, number_of_values))
						{
							display_message(ERROR_MESSAGE,
								"cmzn_nodeset_change_node_identifiers.  "
								"Not enough memory");
							return_code = 0;
						}
					}
				}
				if (return_code)
				{
					field_module = cmzn_region_get_fieldmodule(FE_region_get_cmzn_region(fe_region));
					field_cache = cmzn_fieldmodule_create_fieldcache(field_module);
					cmzn_fieldcache_set_time(field_cache, time);
					/* make a linear array of the nodes in the group in current order */
					array_data.field_cache = field_cache;
					array_data.node_values = node_values;
					array_data.sort_by_field = sort_by_field;
					array_data.number_of_values = number_of_values;
					if (!fe_nodeset->for_each_FE_node(FE_node_and_values_to_array, (void *)&array_data))
					{
						display_message(ERROR_MESSAGE,
							"cmzn_nodeset_change_node_identifiers.  "
							"Could not build node/field values array");
						return_code = 0;
					}
					cmzn_fieldcache_destroy(&field_cache);
					cmzn_fieldmodule_destroy(&field_module);
				}
				if (return_code)
				{
					if (sort_by_field)
					{
						/* sort by field values with higher components more significant */
						qsort(node_values, number_of_nodes,
							sizeof(struct FE_node_values_number),
							compare_FE_node_values_number_values);
						/* give the nodes sequential values starting at node_offset */
						for (i = 0; i < number_of_nodes; i++)
						{
							node_values[i].new_number = node_offset + i;
						}
					}
					else
					{
						/* offset node numbers by node_offset */
						for (i = 0; i < number_of_nodes; i++)
						{
							node_values[i].new_number =
								get_FE_node_identifier(node_values[i].node) + node_offset;
						}
					}
					/* check node numbers are positive and ascending */
					for (i = 0; (i < number_of_nodes) && return_code; i++)
					{
						if (0 >= node_values[i].new_number)
						{
							display_message(ERROR_MESSAGE,
								"cmzn_nodeset_change_node_identifiers.  "
								"node_offset would give negative node numbers");
							return_code = 0;
						}
						else if ((0 < i) &&
							(node_values[i].new_number <= node_values[i - 1].new_number))
						{
							display_message(ERROR_MESSAGE,
								"cmzn_nodeset_change_node_identifiers.  "
								"Node numbers are not strictly increasing");
							return_code = 0;
						}
					}
				}
				if (return_code)
				{
					/* check no new numbers are in use by nodes not in node_group */
					for (i = 0; (i < number_of_nodes) && return_code; i++)
					{
						if ((node_with_identifier = fe_nodeset->findNodeByIdentifier(
							node_values[i].new_number)) &&
							(!cmzn_nodeset_contains_node(nodeset, node_with_identifier)))
						{
							display_message(ERROR_MESSAGE,
								"cmzn_nodeset_change_node_identifiers.  "
								"Node using new number exists in master nodeset");
							return_code = 0;
						}
					}
				}
				if (return_code)
				{
					/* change identifiers */
					/* maintain next_spare_node_number to renumber nodes in same group
						 which already have the same number as the new_number */
					next_spare_node_number =
						node_values[number_of_nodes - 1].new_number + 1;
					for (i = 0; (i < number_of_nodes) && return_code; i++)
					{
						/* only modify if node doesn't already have correct identifier */
						if (cmzn_nodeset_contains_node(nodeset, node_values[i].node) &&
							(node_values[i].node != (node_with_identifier =
								fe_nodeset->findNodeByIdentifier(node_values[i].new_number))))
						{
							if (node_with_identifier) 
							{
								while ((struct FE_node *)NULL !=
									fe_nodeset->findNodeByIdentifier(next_spare_node_number))
								{
									next_spare_node_number++;
								}
								if (!fe_nodeset->change_FE_node_identifier(
									node_with_identifier, next_spare_node_number))
								{
									return_code = 0;
								}
							}
							if (!fe_nodeset->change_FE_node_identifier(
								node_values[i].node, node_values[i].new_number))
							{
								display_message(ERROR_MESSAGE,
									"cmzn_nodeset_change_node_identifiers.  "
									"Could not change node identifier");
								return_code = 0;
							}
						}
					}
				}
				for (i = 0; i < number_of_nodes; i++)
				{
					if (node_values[i].values)
					{
						DEALLOCATE(node_values[i].values);
					}
				}
				DEALLOCATE(node_values);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"cmzn_nodeset_change_node_identifiers.  Not enough memory");
				return_code = 0;
			}
		}
		FE_region_end_change(fe_region);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_nodeset_change_node_identifiers.  Invalid argument(s)");
		return_code = 0;
	}
	return (return_code);
}

cmzn_field_id cmzn_nodeset_create_conditional_field_from_identifier_ranges(
	cmzn_nodeset_id nodeset, struct Multi_range *identifierRanges)
{
	if (!(nodeset && identifierRanges))
	{
		display_message(ERROR_MESSAGE, "cmzn_nodeset_create_conditional_field_from_identifier_ranges.  Invalid argument(s)");
		return 0;
	}
	cmzn_fieldmodule *fieldmodule = cmzn_nodeset_get_fieldmodule(nodeset);
	cmzn_fieldmodule_begin_change(fieldmodule);
	cmzn_field *conditionalField = cmzn_fieldmodule_create_field_node_group(fieldmodule, nodeset);
	cmzn_field_node_group *nodeGroupField = cmzn_field_cast_node_group(conditionalField);
	Computed_field_node_group *nodeGroup = Computed_field_node_group_core_cast(nodeGroupField);
	if (nodeGroup)
	{
		const int number_of_ranges = Multi_range_get_number_of_ranges(identifierRanges);
		for (int i = 0; i < number_of_ranges; ++i)
		{
			int start, stop;
			Multi_range_get_range(identifierRanges, i, &start, &stop);
			if (CMZN_OK != nodeGroup->addObjectsInIdentifierRange(start, stop))
			{
				cmzn_field_destroy(&conditionalField);
				break;
			}
		}
	}
	else
		cmzn_field_destroy(&conditionalField);
	cmzn_field_node_group_destroy(&nodeGroupField);
	cmzn_fieldmodule_end_change(fieldmodule);
	cmzn_fieldmodule_destroy(&fieldmodule);
	if (!conditionalField)
		display_message(ERROR_MESSAGE, "cmzn_nodeset_create_conditional_field_from_identifier_ranges.  Failed");
	return conditionalField;
}

cmzn_field_id cmzn_nodeset_create_conditional_field_from_ranges_and_selection(
	cmzn_nodeset_id nodeset, struct Multi_range *identifierRanges,
	cmzn_field_id conditionalField1, cmzn_field_id conditionalField2,
	cmzn_field_id conditionalField3, FE_value time)
{
	if (!nodeset)
		return 0;
	bool time_varying = false;
	cmzn_field *returnField = 0;
	bool error = false;
	cmzn_fieldmodule *fieldmodule = cmzn_nodeset_get_fieldmodule(nodeset);
	cmzn_fieldmodule_begin_change(fieldmodule);
	if (identifierRanges && (Multi_range_get_number_of_ranges(identifierRanges) > 0)) // empty ranges mean not specified
	{
		returnField = cmzn_nodeset_create_conditional_field_from_identifier_ranges(nodeset, identifierRanges);
		if (!returnField)
			error = true;
	}
	cmzn_field *conditionalFields[3] = { conditionalField1, conditionalField2, conditionalField3 };
	for (int i = 0; i < 3; ++i)
	{
		if (conditionalFields[i])
		{
			if (Computed_field_has_multiple_times(conditionalFields[i]))
				time_varying = true;
			if (returnField)
			{
				cmzn_field *tmpField = returnField;
				returnField = cmzn_fieldmodule_create_field_and(fieldmodule, tmpField, conditionalFields[i]);
				cmzn_field_destroy(&tmpField);
				if (!returnField)
				{
					error = true;
					break;
				}
			}
			else
				returnField = cmzn_field_access(conditionalFields[i]);
		}
	}
	if (returnField)
	{
		if (time_varying)
		{
			cmzn_field *tmpField = returnField;
			cmzn_field *timeField = cmzn_fieldmodule_create_field_constant(fieldmodule, 1, &time);
			returnField = cmzn_fieldmodule_create_field_time_lookup(fieldmodule, tmpField, timeField);
			cmzn_field_destroy(&tmpField);
			if (!returnField)
				error = true;
			cmzn_field_destroy(&timeField);
		}
	}
	else
	{
		// create an always true conditional field
		const double one = 1;
		returnField = cmzn_fieldmodule_create_field_constant(fieldmodule, 1, &one);
		if (!returnField)
			error = true;
	}
	cmzn_fieldmodule_end_change(fieldmodule);
	cmzn_fieldmodule_destroy(&fieldmodule);
	if (error)
	{
		display_message(ERROR_MESSAGE, "cmzn_nodeset_create_conditional_field_from_ranges_and_selection.  Failed");
		cmzn_field_destroy(&returnField);
	}
	return returnField;
}
