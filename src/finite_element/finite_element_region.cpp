/*******************************************************************************
FILE : finite_element_region.cpp

LAST MODIFIED : 8 August 2006

DESCRIPTION :
Object comprising a single finite element mesh including nodes, elements and
finite element fields defined on or interpolated over them.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cstdlib>
#include <cstdio>
#include <vector>
#include "opencmiss/zinc/element.h"
#include "opencmiss/zinc/node.h"
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_field_private.hpp"
#include "finite_element/finite_element_mesh.hpp"
#include "finite_element/finite_element_nodeset.hpp"
#include "finite_element/finite_element_region.h"
#include "finite_element/finite_element_region_private.h"
#include "general/callback_private.h"
#include "general/compare.h"
#include "general/debug.h"
#include "general/indexed_list_private.h"
#include "general/mystring.h"
#include "general/object.h"
#include "region/cmiss_region.hpp"
#include "general/message.h"

/*
Module types
------------
*/

FE_region_changes::FE_region_changes(struct FE_region *fe_regionIn) :
	fe_region(ACCESS(FE_region)(fe_regionIn)),
	access_count(1)
{
	this->fe_field_changes = fe_region->extractFieldChangeLog();
	for (int n = 0; n < 2; ++n)
		this->nodeChangeLogs[n] = fe_region->nodesets[n]->extractChangeLog();
	// when extracting element change logs, propagate field change summary flags
	// (RELATED flag) from nodes and parent elements
	const int nodeChangeSummary = this->nodeChangeLogs[0]->getChangeSummary();
	bool parentChange = 0 != (nodeChangeSummary & DS_LABEL_CHANGE_TYPE_RELATED);
	bool parentAllChange = this->nodeChangeLogs[0]->isAllChange();
	for (int dim = MAXIMUM_ELEMENT_XI_DIMENSIONS - 1; 0 <= dim; --dim)
	{
		this->elementChangeLogs[dim] = fe_region->meshes[dim]->extractChangeLog();
		if (parentChange)
		{
			if (parentAllChange)
				this->elementChangeLogs[dim]->setAllChange(DS_LABEL_CHANGE_TYPE_RELATED);
			else
				this->elementChangeLogs[dim]->setChange(DS_LABEL_CHANGE_TYPE_RELATED);
		}
		else
		{
			int elementChanges = this->elementChangeLogs[dim]->getChangeSummary();
			parentChange = 0 != (elementChanges & DS_LABEL_CHANGE_TYPE_RELATED);
		}
		if (parentChange && !parentAllChange)
			parentAllChange = this->elementChangeLogs[dim]->isAllChange();
		// following flag is used for propagation of field changes from
		// nodes, and on to face elements
		this->propagatedToDimension[dim] = false;
	}
}

FE_region_changes::~FE_region_changes()
{
	DESTROY(CHANGE_LOG(FE_field))(&(this->fe_field_changes));
	for (int n = 0; n < 2; ++n)
		cmzn::Deaccess(this->nodeChangeLogs[n]);
	for (int dim = 0; dim < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++dim)
		cmzn::Deaccess(this->elementChangeLogs[dim]);
	DEACCESS(FE_region)(&(this->fe_region));
}

/** If not already done, propagate field changes from nodes, or higher
  * dimension elements to faces of this dimension.
  * @param dimension   Mesh dimension from 1 to MAXIMUM_ELEMENT_XI_DIMENSIONS */
bool FE_region_changes::propagateToDimension(int dimension)
{
	if ((dimension < 1) || (dimension > MAXIMUM_ELEMENT_XI_DIMENSIONS))
	{
		display_message(INFORMATION_MESSAGE, "FE_region_changes::propagateToDimension.  Invalid dimension");
		return false;
	}
	if (this->propagatedToDimension[dimension - 1])
		return true;
	if (!this->elementChangeLogs[dimension - 1]->isAllChange())
	{
		FE_mesh *mesh = FE_region_find_FE_mesh_by_dimension(this->fe_region, dimension);
		if (mesh)
		{
			// propagate from parent element changes, node changes, both, or neither
			DsLabelsChangeLog *changeLog = this->elementChangeLogs[dimension - 1];
			DsLabelsGroup *group = changeLog->getLabelsGroup();
			const DsLabelsGroup *parentGroup = 0;
			if (dimension < MAXIMUM_ELEMENT_XI_DIMENSIONS)
			{
				this->propagateToDimension(dimension + 1);
				const DsLabelsChangeLog *parentChangeLog = this->elementChangeLogs[dimension];
				if (parentChangeLog->getChangeSummary() != DS_LABEL_CHANGE_TYPE_NONE)
					parentGroup = parentChangeLog->getLabelsGroup();
			}
			const DsLabelsChangeLog *nodeChangeLog = this->nodeChangeLogs[0];
			const DsLabelsGroup *nodeGroup = 0;
			if (nodeChangeLog->getChangeSummary() != DS_LABEL_CHANGE_TYPE_NONE)
				nodeGroup = nodeChangeLog->getLabelsGroup();
			if (parentGroup || nodeGroup)
			{
				bool relatedChange = (0 != (changeLog->getChangeSummary() & DS_LABEL_CHANGE_TYPE_RELATED));
				const DsLabelIndex elementIndexLimit = mesh->getLabelsIndexSize();
				for (DsLabelIndex elementIndex = 0; elementIndex < elementIndexLimit; ++elementIndex)
				{
					if (relatedChange && group->hasIndex(elementIndex))
						continue; // optimisation only works after relatedChange is set
					if (mesh->getElementIdentifier(elementIndex) == DS_LABEL_IDENTIFIER_INVALID)
						continue; // no element at index, normal if elements have been removed
					if ((parentGroup && mesh->elementHasParentInGroup(elementIndex, *parentGroup))
						|| (nodeGroup && mesh->elementHasNodeInGroup(elementIndex, *nodeGroup)))
					{
						group->setIndex(elementIndex, true);
						relatedChange = true;
					}
				}
				if (relatedChange)
					this->elementChangeLogs[dimension - 1]->setChange(DS_LABEL_CHANGE_TYPE_RELATED);
			}
		}
	}
	this->propagatedToDimension[dimension - 1] = true;
	return true;
}

void FE_region::clearCachedChanges()
{
	CHANGE_LOG_CLEAR(FE_field)(this->fe_field_changes);
	for (int n = 0; n < 2; ++n)
		this->nodesets[n]->clearChangeLog();
	for (int dim = 0; dim < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++dim)
		this->meshes[dim]->clearChangeLog();
}

/**
 * Tells parent region about changes to fields, nodes and elements.
 * No messages sent if change level is positive, or no changes have been made.
 */
void FE_region::update()
{
	if (this->change_level <= 0)
	{
		// note this only informs region of change; change logs are extracted
		// on demand when computed field manager change is sent to region
		if (this->cmiss_region)
		{
			// only inform if fields, nodes or elements changed
			int fieldChangeSummary = 0;
			CHANGE_LOG_GET_CHANGE_SUMMARY(FE_field)(this->fe_field_changes, &fieldChangeSummary);
			bool changed = (fieldChangeSummary != 0);
			if (!changed)
				for (int n = 0; n < 2; ++n)
				{
					const int nodeChangeSummary = this->nodesets[n]->getChangeLog()->getChangeSummary();
					if (nodeChangeSummary != 0)
					{
						changed = true;
						break;
					}
				}
			if (!changed)
				for (int dim = 0; dim < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++dim)
				{
					const int elementChangeSummary = this->meshes[dim]->getChangeLog()->getChangeSummary();
					if (elementChangeSummary != 0)
					{
						changed = true;
						break;
					}
				}
			if (changed)
				this->cmiss_region->FeRegionChange();
		}
	}
}

cmzn_fielditerator *FE_region::create_fielditerator()
{
	return this->cmiss_region->createFielditerator();
}

/*
Module functions
----------------
*/

FE_region_bases_and_shapes::FE_region_bases_and_shapes() :
	basis_manager(CREATE(MANAGER(FE_basis))()),
	element_shape_list(CREATE(LIST(FE_element_shape))()),
	access_count(1)
{
}

FE_region_bases_and_shapes::~FE_region_bases_and_shapes()
{
	DESTROY(MANAGER(FE_basis))(&this->basis_manager);
	DESTROY(LIST(FE_element_shape))(&this->element_shape_list);
}

FE_region::FE_region(cmzn_region *region, FE_region *base_fe_region) :
	cmiss_region(region),
	fe_time(CREATE(FE_time_sequence_package)()),
	fe_field_list(CREATE(LIST(FE_field))()),
	bases_and_shapes(base_fe_region ? base_fe_region->bases_and_shapes->access() : FE_region_bases_and_shapes::create()),
	change_level(0),
	fe_field_changes(0),
	informed_make_cmiss_number_field(false),
	informed_make_xi_field(false),
	access_count(1)
{
	this->createFieldChangeLog();
	for (int n = 0; n < 2; ++n)
		this->nodesets[n] = FE_nodeset::create(this);
	this->nodesets[0]->setFieldDomainType(CMZN_FIELD_DOMAIN_TYPE_NODES);
	this->nodesets[1]->setFieldDomainType(CMZN_FIELD_DOMAIN_TYPE_DATAPOINTS);

	for (int dimension = 1; dimension <= MAXIMUM_ELEMENT_XI_DIMENSIONS; ++dimension)
	{
		this->meshes[dimension - 1] = FE_mesh::create(this, dimension);
		this->meshes[dimension - 1]->setNodeset(this->nodesets[0]);
	}
	for (int dimension = 2; dimension <= MAXIMUM_ELEMENT_XI_DIMENSIONS; ++dimension)
		this->meshes[dimension - 1]->setFaceMesh(this->meshes[dimension - 2]);
	for (int dimension = 1; dimension < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++dimension)
		this->meshes[dimension - 1]->setParentMesh(this->meshes[dimension]);
}

FE_region::~FE_region()
{
	if (0 != this->access_count)
		display_message(ERROR_MESSAGE, "~FE_region.  Non-zero access count");
	if (0 != this->change_level)
		display_message(WARNING_MESSAGE, "~FE_region.  Non-zero change_level %d", this->change_level);

	this->change_level = 1; // so no notifications

	// detach first to clean up some dynamic data and remove pointers back to FE_region
	for (int n = 0; n < 2; ++n)
		this->nodesets[n]->detach_from_FE_region();
	for (int dimension = MAXIMUM_ELEMENT_XI_DIMENSIONS; 0 < dimension; --dimension)
		this->meshes[dimension - 1]->detach_from_FE_region();

	// then deaccess to destroy handle
	for (int n = 0; n < 2; ++n)
		FE_nodeset::deaccess(this->nodesets[n]);
	for (int dimension = MAXIMUM_ELEMENT_XI_DIMENSIONS; 0 < dimension; --dimension)
		FE_mesh::deaccess(this->meshes[dimension - 1]);

	// remove FE_fields' pointers to this region
	cmzn_set_FE_field *fields = reinterpret_cast<cmzn_set_FE_field*>(this->fe_field_list);
	for (cmzn_set_FE_field::iterator field_iter = fields->begin(); field_iter != fields->end(); ++field_iter)
		(*field_iter)->set_FE_region(nullptr);

	FE_region_bases_and_shapes::deaccess(this->bases_and_shapes);

	DESTROY(LIST(FE_field))(&(this->fe_field_list));
	DESTROY(FE_time_sequence_package)(&(this->fe_time));

	DESTROY(CHANGE_LOG(FE_field))(&(this->fe_field_changes));
}

void FE_region::clearRegionPrivate()
{
	this->cmiss_region = nullptr;
}

/** Private: assumes current change log pointer is null or invalid */
void FE_region::createFieldChangeLog()
{
	this->fe_field_changes = CREATE(CHANGE_LOG(FE_field))(this->fe_field_list);
	if (!this->fe_field_changes)
		display_message(ERROR_MESSAGE, "FE_region::createFieldChangeLog.  Failed to create change log");
}

struct CHANGE_LOG(FE_field) *FE_region::extractFieldChangeLog()
{
	struct CHANGE_LOG(FE_field) *returnChangeLog = this->fe_field_changes;
	CHANGE_LOG_MERGE_ALL_CHANGE(FE_field)(returnChangeLog);
	this->createFieldChangeLog();
	return returnChangeLog;
}

/*
Global functions
----------------
*/

/**
 * Frees the memory for the FE_region and sets <*fe_region_address> to NULL.
 */
static int DESTROY(FE_region)(struct FE_region **fe_region_address)
{
	if (fe_region_address)
	{
		delete *fe_region_address;
		fe_region_address = 0;
		return 1;
	}
	return 0;
}

DECLARE_OBJECT_FUNCTIONS(FE_region)

int FE_region_begin_change(struct FE_region *fe_region)
{
	if (fe_region)
	{
		++(fe_region->change_level);
		return 1;
	}
	return 0;
}

int FE_region_end_change(struct FE_region *fe_region)
{
	if (fe_region)
	{
		if (0 < fe_region->change_level)
		{
			--(fe_region->change_level);
			fe_region->update();
			return 1;
		}
		else
		{
			display_message(ERROR_MESSAGE, "FE_region_end_change.  Change not enabled");
		}
	}
	return 0;
}

bool FE_field_has_cached_changes(FE_field *fe_field)
{
	FE_region *fe_region;
	if (fe_field && (fe_region = fe_field->get_FE_region()) && (fe_region->change_level))
	{
		int change = 0;
		CHANGE_LOG_QUERY(FE_field)(fe_region->fe_field_changes, fe_field, &change);
		if (change & (CHANGE_LOG_OBJECT_NOT_IDENTIFIER_CHANGED(FE_field) | 
			CHANGE_LOG_RELATED_OBJECT_CHANGED(FE_field)))
			return true;
	}
	return false;
}

int FE_region_clear(struct FE_region *fe_region)
// This could be made faster.
{
	int return_code;
	struct FE_field *fe_field;

	ENTER(FE_region_clear);
	if (fe_region)
	{
		return_code = 1;
		FE_region_begin_change(fe_region);
		for (int dim = 0; dim < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++dim)
		{
			fe_region->meshes[dim]->clear();
		}
		for (int n = 0; n < 2; ++n)
		{
			fe_region->nodesets[n]->clear();
		}

		if (fe_region->fe_field_list)
		{
			while (return_code && (fe_field = FIRST_OBJECT_IN_LIST_THAT(FE_field)(
				(LIST_CONDITIONAL_FUNCTION(FE_field) *)NULL, (void *)NULL,
				fe_region->fe_field_list)))
			{
				return_code = FE_region_remove_FE_field(fe_region, fe_field);
			}
		}
		if (!return_code)
		{
			display_message(ERROR_MESSAGE, "FE_region_clear.  Failed");
			return_code = 0;
		}
		FE_region_end_change(fe_region);
	}
	else
	{
		display_message(ERROR_MESSAGE, "FE_region_clear.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
}

struct FE_field *FE_region_get_FE_field_with_general_properties(
	struct FE_region *fe_region, const char *name, enum Value_type value_type,
	int number_of_components)
{
	FE_field *fe_field = nullptr;
	if (fe_region && name && (0 < number_of_components))
	{
		fe_field = FIND_BY_IDENTIFIER_IN_LIST(FE_field,name)(name,
			fe_region->fe_field_list);
		if (fe_field)
		{
			if ((get_FE_field_FE_field_type(fe_field) == GENERAL_FE_FIELD) &&
				(get_FE_field_value_type(fe_field) == value_type) &&
				(get_FE_field_number_of_components(fe_field) == number_of_components))
			{
				fe_field->access();
			}
			else
			{
				display_message(ERROR_MESSAGE, "FE_region_get_FE_field_with_general_properties.  "
					"Existing field '%s' has different properties", name);
				fe_field = nullptr;
			}
		}
		else
		{
			fe_field = FE_field::create(name, fe_region);
			if (!(set_FE_field_value_type(fe_field, value_type) &&
				set_FE_field_number_of_components(fe_field, number_of_components) &&
				set_FE_field_type_general(fe_field) &&
				FE_region_merge_FE_field(fe_region, fe_field)))
			{
				FE_field::deaccess(&fe_field);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_FE_field_with_general_properties.  Invalid argument(s)");
	}
	return (fe_field);
}

struct FE_field *FE_region_merge_FE_field(struct FE_region *fe_region,
	struct FE_field *fe_field)
{
	struct FE_field *merged_fe_field = 0;
	if (fe_region && fe_field)
	{
		if (fe_field->get_FE_region() == fe_region)
		{
			merged_fe_field = FIND_BY_IDENTIFIER_IN_LIST(FE_field,name)(
				get_FE_field_name(fe_field), fe_region->fe_field_list);
			if (merged_fe_field)
			{
				/* no change needs to be noted if fields are exactly the same */
				if (!FE_fields_match_exact(merged_fe_field, fe_field))
				{
					/* can only change fundamentals -- number of components, value type
						 if merged_fe_field is not accessed by any other objects */
					if ((1 == merged_fe_field->getAccessCount()) ||
						FE_fields_match_fundamental(merged_fe_field, fe_field))
					{
						if (merged_fe_field->copyProperties(fe_field))
						{
#if defined (DEBUG_CODE)
					/*???debug*/printf("FE_region_merge_FE_field: %p OBJECT_NOT_IDENTIFIER_CHANGED field %p\n",fe_region,merged_fe_field);
#endif /* defined (DEBUG_CODE) */
							fe_region->FE_field_change(merged_fe_field, CHANGE_LOG_OBJECT_NOT_IDENTIFIER_CHANGED(FE_field));
							fe_region->update();
						}
						else
						{
							display_message(ERROR_MESSAGE,
								"FE_region_merge_FE_field.  Could not modify field");
							merged_fe_field = (struct FE_field *)NULL;
						}
					}
					else
					{
						display_message(ERROR_MESSAGE, "FE_region_merge_FE_field.  "
							"Existing field named %s is different",
							get_FE_field_name(merged_fe_field));
						merged_fe_field = (struct FE_field *)NULL;
					}
				}
			}
			else
			{
				if (ADD_OBJECT_TO_LIST(FE_field)(fe_field, fe_region->fe_field_list))
				{
					merged_fe_field = fe_field;
#if defined (DEBUG_CODE)
					/*???debug*/printf("FE_region_merge_FE_field: %p ADD field %p\n",fe_region,merged_fe_field);
#endif /* defined (DEBUG_CODE) */
					fe_region->FE_field_change(merged_fe_field, CHANGE_LOG_OBJECT_ADDED(FE_field));
					fe_region->update();
				}
				else
				{
					display_message(ERROR_MESSAGE, "FE_region_merge_FE_field.  "
						"Could not add field %s", get_FE_field_name(fe_field));
				}
			}
		}
		else
		{
			display_message(ERROR_MESSAGE, "FE_region_merge_FE_field.  "
					"Field '%s' is not compatible with this finite element region",
				get_FE_field_name(fe_field));
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_merge_FE_field.  Invalid argument(s)");
	}
	return (merged_fe_field);
}

bool FE_region_is_FE_field_in_use(struct FE_region *fe_region,
	struct FE_field *fe_field)
{
	if (fe_region && fe_field)
	{
		if (IS_OBJECT_IN_LIST(FE_field)(fe_field, fe_region->fe_field_list))
		{
			for (int n = 0; n < 2; ++n)
				if (fe_region->nodesets[n]->is_FE_field_in_use(fe_field))
					return true;
			for (int dim = 0; dim < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++dim)
			{
				FE_mesh_field_data *meshFieldData = fe_field->getMeshFieldData(fe_region->meshes[dim]);
				if ((meshFieldData) && meshFieldData->isDefinedOnElements())
					return true;
			}
		}
		else
		{
			struct FE_region *referenced_fe_region = fe_field->get_FE_region();
			if ((referenced_fe_region != NULL) && (referenced_fe_region != fe_region))
			{
				display_message(ERROR_MESSAGE,
					"FE_region_is_FE_field_in_use.  Field %s is from another region", fe_field->getName());
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_is_FE_field_in_use.  Invalid argument(s)");
	}
	return false;
}

int FE_region_remove_FE_field(struct FE_region *fe_region,
	struct FE_field *fe_field)
/*******************************************************************************
LAST MODIFIED : 13 May 2003

DESCRIPTION :
Removes <fe_field> from <fe_region>.
Fields can only be removed if not defined on any nodes and element in
<fe_region>.
==============================================================================*/
{
	int return_code;

	ENTER(FE_region_remove_FE_field);
	return_code = 0;
	if (fe_region && fe_field)
	{
		if (IS_OBJECT_IN_LIST(FE_field)(fe_field, fe_region->fe_field_list))
		{
			if (FE_region_is_FE_field_in_use(fe_region, fe_field))
			{
				display_message(ERROR_MESSAGE,
					"FE_region_remove_FE_field.  Field is in use in region");
				/*???RC Could undefine it at nodes and elements */
			}
			else
			{
				/* access field in case it is only accessed here */
				ACCESS(FE_field)(fe_field);
				return_code = REMOVE_OBJECT_FROM_LIST(FE_field)(fe_field,
					fe_region->fe_field_list);
				if (return_code)
				{
					fe_region->FE_field_change(fe_field, CHANGE_LOG_OBJECT_REMOVED(FE_field));
					fe_region->update();
				}
				DEACCESS(FE_field)(&fe_field);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_region_remove_FE_field.  Field %p is not in region %p",
				fe_field, fe_region);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_remove_FE_field.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* FE_region_remove_FE_field */

struct CHANGE_LOG(FE_field) *FE_region_get_FE_field_changes(struct FE_region *fe_region)
{
	if (fe_region)
		return fe_region->fe_field_changes;
	return 0;
}

struct FE_field *FE_region_get_FE_field_from_name(struct FE_region *fe_region,
	const char *field_name)
{
	if (fe_region && field_name)
		return FIND_BY_IDENTIFIER_IN_LIST(FE_field,name)(field_name, fe_region->fe_field_list);
	return 0;
}

int FE_region_set_FE_field_name(struct FE_region *fe_region,
	struct FE_field *field, const char *new_name)
{
	int return_code;

	ENTER(FE_region_set_FE_field_name);
	if (fe_region && field && new_name)
	{
		if (FE_region_contains_FE_field(fe_region, field))
		{
			return_code = 1;
			if (FE_region_get_FE_field_from_name(fe_region, new_name))
			{
				display_message(ERROR_MESSAGE, "FE_region_set_FE_field_name.  "
					"Field named \"%s\" already exists in this FE_region.", new_name);
				return_code = 0;
			}
			else
			{
				// this temporarily removes the object from all indexed lists
				if (LIST_BEGIN_IDENTIFIER_CHANGE(FE_field,name)(
					fe_region->fe_field_list, field))
				{
					return_code = field->setName(new_name);
					LIST_END_IDENTIFIER_CHANGE(FE_field,name)(fe_region->fe_field_list);
					if (return_code)
					{
						fe_region->FE_field_change(field, CHANGE_LOG_OBJECT_IDENTIFIER_CHANGED(FE_field));
						fe_region->update();
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"FE_region_set_FE_field_name.  "
						"Could not safely change identifier in indexed lists");
					return_code = 0;
				}
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_region_set_FE_field_name.  Field is not from this region");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_set_FE_field_name.  Invalid argument(s)");
		return_code = 0;
	}
	return (return_code);
}

bool FE_region_contains_FE_field(struct FE_region *fe_region,
	struct FE_field *field)
{
	if (fe_region)
		return (0 != IS_OBJECT_IN_LIST(FE_field)(field, fe_region->fe_field_list));
	return false;
}

struct FE_field *FE_region_get_first_FE_field_that(struct FE_region *fe_region,
	LIST_CONDITIONAL_FUNCTION(FE_field) *conditional_function,
	void *user_data_void)
{
	if (fe_region)
		return FIRST_OBJECT_IN_LIST_THAT(FE_field)(conditional_function,
			user_data_void, fe_region->fe_field_list);
	return 0;
}

int FE_region_for_each_FE_field(struct FE_region *fe_region,
	LIST_ITERATOR_FUNCTION(FE_field) iterator_function, void *user_data)
{
	if (fe_region && iterator_function)
		return FOR_EACH_OBJECT_IN_LIST(FE_field)(iterator_function,
			user_data, fe_region->fe_field_list);
	return 0;
}

int FE_region_get_number_of_FE_fields(struct FE_region *fe_region)
{
	if (fe_region)
		return NUMBER_IN_LIST(FE_field)(fe_region->fe_field_list);
	return 0;
}

int FE_region_FE_field_has_multiple_times(struct FE_region *fe_region,
	struct FE_field *fe_field)
{
	if (fe_region && fe_field)
	{
		// currently only node fields can be time aware
		for (int n = 0; n < 2; ++n)
		{
			if (fe_region->nodesets[n]->FE_field_has_multiple_times(fe_field))
				return 1;
		}
	}
	return 0;
}

struct FE_field *FE_region_get_default_coordinate_FE_field(struct FE_region *fe_region)
{
	if (fe_region)
		return FE_region_get_first_FE_field_that(fe_region, FE_field_is_coordinate_field, (void *)NULL);
	return 0;
}

struct MANAGER(FE_basis) *FE_region_get_basis_manager(
	struct FE_region *fe_region)
{
	if (fe_region)
		return fe_region->bases_and_shapes->getBasisManager();
	return 0;
}

struct LIST(FE_field) *FE_region_get_FE_field_list(struct FE_region *fe_region)
{
	if (fe_region)
		return fe_region->fe_field_list;
	return 0;
}

struct LIST(FE_element_shape) *FE_region_get_FE_element_shape_list(
	struct FE_region *fe_region)
{
	if (fe_region)
		return fe_region->bases_and_shapes->getElementShapeList();
	return 0;
}

struct FE_element *FE_region_get_top_level_FE_element_from_identifier(
	struct FE_region *fe_region, int identifier)
{
	if (fe_region && (0 <= identifier))
	{
		for (int dimension = MAXIMUM_ELEMENT_XI_DIMENSIONS; 1 <= dimension; --dimension)
		{
			FE_element *element = fe_region->meshes[dimension - 1]->findElementByIdentifier(identifier);
			if (element && FE_element_is_top_level(element, (void *)NULL))
				return element;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_top_level_FE_element_from_identifier.  Invalid argument(s)");
	}
	return 0;
}

FE_nodeset *FE_region_find_FE_nodeset_by_field_domain_type(
	struct FE_region *fe_region, enum cmzn_field_domain_type domain_type)
{
	if (fe_region)
	{
		if (domain_type == CMZN_FIELD_DOMAIN_TYPE_NODES)
			return fe_region->nodesets[0];
		else if (domain_type == CMZN_FIELD_DOMAIN_TYPE_DATAPOINTS)
			return fe_region->nodesets[1];
	}
	return 0;
}

FE_mesh *FE_region_find_FE_mesh_by_dimension(
	struct FE_region *fe_region, int dimension)
{
	if (fe_region && (0 < dimension) && (dimension <= MAXIMUM_ELEMENT_XI_DIMENSIONS))
		return fe_region->meshes[dimension - 1];
	return 0;
}

int FE_region_get_number_of_FE_elements_all_dimensions(struct FE_region *fe_region)
{
	int number_of_elements = 0;

	ENTER(FE_region_get_number_of_FE_elements_all_dimensions);
	if (fe_region)
	{
		for (int dim = 0; dim < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++dim)
		{
			number_of_elements += fe_region->meshes[dim]->getSize();
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_number_of_FE_elements_all_dimensions.  Invalid argument(s)");
	}
	LEAVE;

	return (number_of_elements);
}

int FE_region_get_number_of_FE_elements_of_dimension(
	struct FE_region *fe_region, int dimension)
{
	if (fe_region && (1 <= dimension) && (dimension <= MAXIMUM_ELEMENT_XI_DIMENSIONS))
	{
		return fe_region->meshes[dimension - 1]->getSize();
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_number_of_FE_elements_of_dimension.  Invalid argument(s)");
	}
	return 0;
}

int FE_region_get_highest_dimension(struct FE_region *fe_region)
{
	int highest_dimension = MAXIMUM_ELEMENT_XI_DIMENSIONS;
	while (highest_dimension &&
		(fe_region->meshes[highest_dimension - 1]->getSize() == 0))
	{
		--highest_dimension;
	}
	return highest_dimension;
}

int FE_region_begin_define_faces(struct FE_region *fe_region)
{
	if (fe_region)
	{
		for (int dimension = 1; dimension <= MAXIMUM_ELEMENT_XI_DIMENSIONS; ++dimension)
		{
			int result = fe_region->meshes[dimension - 1]->begin_define_faces();
			if (result != CMZN_OK)
			{
				if (result == CMZN_ERROR_ALREADY_EXISTS)
					break;
				FE_region_end_define_faces(fe_region);
				return 0;
			}
		}
		return 1;
	}
	else
	{
		display_message(ERROR_MESSAGE, "FE_region_begin_define_faces.  Invalid argument(s)");
	}
	return 0;
}

int FE_region_end_define_faces(struct FE_region *fe_region)
{
	if (fe_region)
	{
		for (int dimension = 1; dimension <= MAXIMUM_ELEMENT_XI_DIMENSIONS; ++dimension)
			fe_region->meshes[dimension - 1]->end_define_faces();
		return 1;
	}
	else
	{
		display_message(ERROR_MESSAGE, "FE_region_end_define_faces.  Invalid argument(s)");
	}
	return 0;
}

int FE_region_define_faces(struct FE_region *fe_region)
{
	int return_code = CMZN_OK;
	if (fe_region)
	{
		if (FE_region_get_highest_dimension(fe_region) > 1)
		{
			FE_region_begin_change(fe_region);
			FE_region_begin_define_faces(fe_region);
			for (int dimension = MAXIMUM_ELEMENT_XI_DIMENSIONS; 2 <= dimension; --dimension)
			{
				const int result = fe_region->meshes[dimension - 1]->define_faces();
				if (result != CMZN_OK)
				{
					return_code = result;
					if (result == CMZN_WARNING_PART_DONE)
					{
						continue;
					}
					break;
				}
			}
			FE_region_end_define_faces(fe_region);
			FE_region_end_change(fe_region);
			if (return_code == CMZN_WARNING_PART_DONE)
			{
				display_message(WARNING_MESSAGE, "Fieldmodule defineAllFaces.  Could not define faces for some elements due to missing field or nodes");
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE, "FE_region_define_faces.  Invalid argument(s)");
		return_code = CMZN_ERROR_ARGUMENT;
	}
	return return_code;
}

struct FE_element *FE_region_get_first_FE_element_that(
	struct FE_region *fe_region,
	LIST_CONDITIONAL_FUNCTION(FE_element) *conditional_function,
	void *user_data_void)
{
	if (fe_region)
	{
		for (int dimension = MAXIMUM_ELEMENT_XI_DIMENSIONS; (1 <= dimension); --dimension)
		{
			FE_element *element = fe_region->meshes[dimension - 1]->get_first_FE_element_that(conditional_function, user_data_void);
			if (element)
				return element;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_get_first_FE_element_that.  Invalid argument(s)");
	}
	return 0;
}

int FE_region_for_each_FE_element(struct FE_region *fe_region,
	LIST_ITERATOR_FUNCTION(FE_element) iterator_function, void *user_data)
{
	if (fe_region && iterator_function)
	{
		for (int dimension = MAXIMUM_ELEMENT_XI_DIMENSIONS; 1 <= dimension; --dimension)
		{
			if (!fe_region->meshes[dimension - 1]->for_each_FE_element(iterator_function, user_data))
				return 0;
		}
		return 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_for_each_FE_element.  Invalid argument(s)");
	}
	return 0;
}

int FE_region_smooth_FE_field(struct FE_region *fe_region,
	struct FE_field *fe_field, FE_value time)
{
	int return_code = 1;
	if (fe_region && fe_field && (get_FE_field_value_type(fe_field) == FE_VALUE_VALUE))
	{
		if (IS_OBJECT_IN_LIST(FE_field)(fe_field, fe_region->fe_field_list))
		{
			// use highest dimension non-empty element list
			int dimension = FE_region_get_highest_dimension(fe_region);
			if (dimension)
			{
				FE_region_begin_change(fe_region);

				const int componentCount = fe_field->getNumberOfComponents();

				// create field for accumulating node values for averaging
				FE_field *node_accumulate_fe_field = FE_region_get_FE_field_with_general_properties(
					fe_region, "cmzn_smooth_node_accumulate", FE_VALUE_VALUE, componentCount);

				/* create a field to store an integer value per component of fe_field */
				FE_field *element_count_fe_field = FE_region_get_FE_field_with_general_properties(
					fe_region, "cmzn_smooth_element_count", INT_VALUE, componentCount);

				FE_mesh *fe_mesh = fe_region->meshes[dimension - 1];
				cmzn_elementiterator *elementIter = fe_mesh->createElementiterator();

				fe_region->FE_field_change(fe_field, CHANGE_LOG_RELATED_OBJECT_CHANGED(FE_field));
				FE_mesh_field_data *meshFieldData = fe_field->getMeshFieldData(fe_mesh);

				if ((node_accumulate_fe_field) && (element_count_fe_field) && (elementIter) && (meshFieldData))
				{
					FE_element *element;
					const int componentCount = get_FE_field_number_of_components(fe_field);
					while (0 != (element = cmzn_elementiterator_next_non_access(elementIter)))
					{
						/* skip elements without field defined appropriately */
						bool definedAndNodeBased = true;
						for (int c = 0; c < componentCount; ++c)
						{
							FE_mesh_field_template *mft = meshFieldData->getComponentMeshfieldtemplate(c);
							FE_element_field_template *eft = mft->getElementfieldtemplate(element->getIndex());
							if (!((eft) && (eft->getParameterMappingMode() == CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE)))
							{
								definedAndNodeBased = false;
								break;
							}
						}
						if (definedAndNodeBased)
						{
							if (FE_element_smooth_FE_field(element, fe_field, time, node_accumulate_fe_field, element_count_fe_field))
							{
								fe_mesh->elementFieldChange(get_FE_element_index(element), DS_LABEL_CHANGE_TYPE_RELATED, fe_field);  // redundant?
							}
							else
							{
								return_code = 0;
								break;
							}
						}
					}
				}
				else
				{
					if (!meshFieldData)
						display_message(ERROR_MESSAGE, "FE_region_smooth_FE_field.  FE_field is not defined on mesh");
					return_code = 0;
				}
				cmzn_elementiterator_destroy(&elementIter);

				FE_nodeset *fe_nodeset = FE_region_find_FE_nodeset_by_field_domain_type(fe_region,
					CMZN_FIELD_DOMAIN_TYPE_NODES);
				cmzn_nodeiterator *nodeIter = fe_nodeset->createNodeiterator();
				if (!nodeIter)
					return_code = 0;
				cmzn_node *node = 0;
				while ((0 != (node = cmzn_nodeiterator_next_non_access(nodeIter))))
				{
					if (node->getNodeField(fe_field))
					{
						if (FE_node_smooth_FE_field(node, fe_field, time, node_accumulate_fe_field, element_count_fe_field))
							fe_nodeset->nodeFieldChange(node, fe_field);  // redundant, probably
						else
						{
							return_code = 0;
							break;
						}
					}
				}
				cmzn_nodeiterator_destroy(&nodeIter);

				FE_field::deaccess(&element_count_fe_field);
				FE_field::deaccess(&node_accumulate_fe_field);

				FE_region_end_change(fe_region);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_region_smooth_FE_field.  FE_field is not from this region");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_smooth_FE_field.  Invalid argument(s)");
		return_code = 0;
	}
	return (return_code);
}

struct FE_time_sequence *FE_region_get_FE_time_sequence_matching_series(
	struct FE_region *fe_region, int number_of_times, const FE_value *times)
{
	if (fe_region)
		return get_FE_time_sequence_matching_time_series(fe_region->fe_time, number_of_times, times);
	return 0;
}

struct FE_time_sequence *FE_region_get_FE_time_sequence_merging_two_time_series(
	struct FE_region *fe_region, struct FE_time_sequence *time_sequence_one,
	struct FE_time_sequence *time_sequence_two)
{
	if (fe_region)
		return get_FE_time_sequence_merging_two_time_series(fe_region->fe_time,
			time_sequence_one, time_sequence_two);
	return 0;
}

struct FE_basis *FE_region_get_FE_basis_matching_basis_type(
	struct FE_region *fe_region, int *basis_type)
{
	if (fe_region && basis_type)
		return make_FE_basis(basis_type, fe_region->bases_and_shapes->getBasisManager());
	return 0;
}

struct FE_basis *FE_region_get_constant_FE_basis_of_dimension(
	struct FE_region *fe_region, int dimension)
{
	if ((!fe_region) || (dimension < 1) || (dimension > MAXIMUM_ELEMENT_XI_DIMENSIONS))
		return 0;
	int basisType[1 + MAXIMUM_ELEMENT_XI_DIMENSIONS*MAXIMUM_ELEMENT_XI_DIMENSIONS];
	basisType[0] = dimension;
	int *type = basisType + 1;
	for (int d = 0; d < dimension; ++d)
	{
		*type = FE_BASIS_CONSTANT;
		++type;
		for (int i = d + 1; i < dimension; ++i)
		{
			*type = NO_RELATION;
			++type;
		}
	}
	return make_FE_basis(basisType, fe_region->bases_and_shapes->getBasisManager());
}

struct cmzn_region *FE_region_get_cmzn_region(struct FE_region *fe_region)
{
	if (fe_region)
		return fe_region->cmiss_region;
	return 0;
}

/**
 * Finds or creates a new field in FE_region matching the supplied one,
 * converting all indexer fields and host meshes.
 * FE_field iterator assumed to be called from FE_region_merge, and
 * assumes that FE_region_can_merge has passed.
 */
static int FE_field_merge_into_FE_region(struct FE_field *sourceField,
	void *fe_region_void)
{
	FE_region *fe_region = reinterpret_cast<struct FE_region *>(fe_region_void);
	if (!(sourceField && fe_region))
	{
		display_message(ERROR_MESSAGE, "FE_field_merge_into_FE_region.  Invalid argument(s)");
		return 0;
	}
	// if target field exists, assume it has passed FE_region_can_merge
	const char *fieldName = get_FE_field_name(sourceField);
	FE_field *targetField = FE_region_get_FE_field_from_name(fe_region, fieldName);
	if (targetField)
		return 1;

	if (INDEXED_FE_FIELD == get_FE_field_FE_field_type(sourceField))
	{
		// must merge indexer field first
		FE_field *indexerField;
		int number_of_indexed_values = 0;
		if (!(get_FE_field_type_indexed(sourceField, &indexerField, &number_of_indexed_values)
			&& FE_field_merge_into_FE_region(indexerField, fe_region_void)))
			return 0;
	}
	targetField = FE_field::create(fieldName, fe_region);
	// don't want to be warned about this changing, so set here:
	targetField->set_CM_field_type(sourceField->get_CM_field_type());
	if (!(targetField->copyProperties(sourceField) &&
		ADD_OBJECT_TO_LIST(FE_field)(targetField, fe_region->fe_field_list)))
	{
		display_message(ERROR_MESSAGE,
			"FE_field_merge_into_FE_region.  Failed to create field %s for target region", fieldName);
		FE_field::deaccess(&targetField);
		return 0;
	}
	fe_region->FE_field_change(targetField, CHANGE_LOG_OBJECT_ADDED(FE_field));
	FE_field::deaccess(&targetField);
	fe_region->update();
	return 1;
}

bool FE_region_can_merge(struct FE_region *target_fe_region,
	struct FE_region *source_fe_region)
{
	if (!source_fe_region)
		return false;

	if (target_fe_region)
	{
		// check fields of the same name have compatible definitions
		if (!FOR_EACH_OBJECT_IN_LIST(FE_field)(FE_field_can_be_merged_into_list,
			(void *)target_fe_region->fe_field_list, source_fe_region->fe_field_list))
		{
			display_message(ERROR_MESSAGE,
				"Cannot merge field(s) into region due to incompatible definition");
			return false;
		}

		// check nodes (nodesets)
		for (int n = 0; n < 2; ++n)
		{
			if (!target_fe_region->nodesets[n]->canMerge(*(source_fe_region->nodesets[n])))
				return false;
		}
	}

	// check/convert finite element field parameter mappings from indexes to derivatives & versions
	// conversion must happen with or without a target_fe_region
	FE_nodeset *target_fe_nodeset = (target_fe_region) ? target_fe_region->nodesets[0] : 0;
	for (int dimension = MAXIMUM_ELEMENT_XI_DIMENSIONS; (0 < dimension); --dimension)
	{
		FE_mesh *source_fe_mesh = source_fe_region->meshes[dimension - 1];
		if (!source_fe_mesh->checkConvertLegacyNodeParameters(target_fe_nodeset))
		{
			display_message(ERROR_MESSAGE,
				"Cannot merge field(s) into region as cannot migrate element field mapping indexes to derivatives/versions");
			return false;
		}
	}

	if (target_fe_region)
	{
		// check elements match in shape and faces
		for (int dimension = MAXIMUM_ELEMENT_XI_DIMENSIONS; (0 < dimension); --dimension)
		{
			FE_mesh *source_fe_mesh = source_fe_region->meshes[dimension - 1];
			FE_mesh *target_fe_mesh = target_fe_region->meshes[dimension - 1];
			if (!target_fe_mesh->canMerge(*source_fe_mesh))
				return false;
		}
	}

	return true;
}

int FE_region_merge(struct FE_region *target_fe_region,
	struct FE_region *source_fe_region)
{
	int return_code = 1;
	if (target_fe_region && source_fe_region)
	{
		FE_region_begin_change(target_fe_region);

		// merge fields
		if (!FOR_EACH_OBJECT_IN_LIST(FE_field)(
			FE_field_merge_into_FE_region, (void *)target_fe_region,
			source_fe_region->fe_field_list))
		{
			display_message(ERROR_MESSAGE, "FE_region_merge.  Could not merge fields");
			return_code = 0;
		}

		// merge meshes part 1: elements (so defined for element:xi node fields)
		if (return_code)
		{
			// merge meshes from lowest to highest dimension so faces are merged before parent
			for (int dim = 0; dim < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++dim)
				if (!target_fe_region->meshes[dim]->mergePart1Elements(*(source_fe_region->meshes[dim])))
					return_code = 0;
		}

		// merge nodesets: nodes and fields
		if (return_code)
		{
			for (int n = 0; n < 2; ++n)
				if (!target_fe_region->nodesets[n]->merge(*(source_fe_region->nodesets[n])))
					return_code = 0;
		}

		// merge meshes part 2: fields
		if (return_code)
		{
			// merge meshes from lowest to highest dimension so faces are merged before parent
			for (int dim = 0; dim < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++dim)
				if (!target_fe_region->meshes[dim]->mergePart2Fields(*(source_fe_region->meshes[dim])))
					return_code = 0;
		}

		FE_region_end_change(target_fe_region);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_region_merge.  Invalid argument(s)");
		return_code = 0;
	}
	return (return_code);
}

bool FE_region_need_add_cmiss_number_field(struct FE_region *fe_region)
{
	if (fe_region && (!fe_region->informed_make_cmiss_number_field))
	{
		for (int n = 0; n < 2; ++n)
			if (fe_region->nodesets[n]->getSize())
			{
				fe_region->informed_make_cmiss_number_field = true;
				return true;
			}
		if (FE_region_get_number_of_FE_elements_all_dimensions(fe_region))
		{
			fe_region->informed_make_cmiss_number_field = true;
			return true;
		}
	}
	return false;
}

bool FE_region_need_add_xi_field(struct FE_region *fe_region)
{
	if (fe_region && (!fe_region->informed_make_xi_field) &&
		FE_region_get_number_of_FE_elements_all_dimensions(fe_region))
	{
		fe_region->informed_make_xi_field = true;
		return true;
	}
	return false;
}

void FE_region_list_btree_statistics(struct FE_region *fe_region)
{
	if (fe_region)
	{
		for (int n = 0; n < 2; ++n)
			fe_region->nodesets[n]->list_btree_statistics();
		for (int dimension = 1; dimension <= MAXIMUM_ELEMENT_XI_DIMENSIONS; ++dimension)
			fe_region->meshes[dimension - 1]->list_btree_statistics();
	}
}
