/**
 * FILE : computed_field_group.cpp
 * 
 * Implements a field for representing hierarchical groups, essentially a
 * predicate function returning true for parts of domains considered in the group,
 * and coordinating related subobject groups.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "opencmiss/zinc/fieldgroup.h"
#include "opencmiss/zinc/fieldsubobjectgroup.h"
#include "opencmiss/zinc/mesh.h"
#include "opencmiss/zinc/nodeset.h"
#include "opencmiss/zinc/scene.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_group.hpp"
#include "computed_field/computed_field_group_base.hpp"
#include "computed_field/computed_field_subobject_group.hpp"
#include "computed_field/computed_field_private.hpp"
#include "computed_field/field_module.hpp"
#if defined (USE_OPENCASCADE)
#include "graphics/scene.hpp"
#include "opencmiss/zinc/fieldcad.h"
#endif /* defined (USE_OPENCASCADE) */
#include "finite_element/finite_element_nodeset.hpp"
#include "finite_element/finite_element_region.h"
#include "general/debug.h"
#include "general/mystring.h"
#include "region/cmiss_region.hpp"
#include "general/message.h"
#include "mesh/cmiss_node_private.hpp"
#include "mesh/cmiss_element_private.hpp"
#if defined (USE_OPENCASCADE)
#include "cad/field_location.hpp"
#endif /* defined (USE_OPENCASCADE) */
#include <cstdlib>
#include <list>

char computed_field_group_type_string[] = "group";

Computed_field_group::Computed_field_group(cmzn_region *region)
	: Computed_field_group_base()
	, region(region)
	, contains_all(false)
	, subelementHandlingMode(CMZN_FIELD_GROUP_SUBELEMENT_HANDLING_MODE_NONE)
	, local_node_group(0)
	, local_data_group(0)
	, subregion_group_map()
{
	for (int i = 0; i < MAXIMUM_ELEMENT_XI_DIMENSIONS; i++)
		this->local_element_group[i] = 0;
}

Computed_field_group::~Computed_field_group()
{
	clearLocalNodeGroup(/*isData*/false);
	clearLocalNodeGroup(/*isData*/true);
	for (int i = 0; i < MAXIMUM_ELEMENT_XI_DIMENSIONS; i++)
		clearLocalElementGroup(i);
	for (Region_field_map_iterator iter = subregion_group_map.begin();
		iter != subregion_group_map.end(); ++iter)
	{
		cmzn_field_group_id subregion_group_field = iter->second;
		cmzn_field_group_destroy(&subregion_group_field);
	}
	std::map<Computed_field *, Computed_field *>::iterator it = domain_selection_group.begin();
	for (;it != domain_selection_group.end(); it++)
	{
		//cmzn_field_destroy(&(it->first)); don't destroy this it is not a reference just a key
		cmzn_field_destroy(&(it->second));
	}
}

void Computed_field_group::clearLocalElementGroup(int index)
{
	if (this->local_element_group[index])
	{
		Computed_field_subobject_group *subobject_group =
			static_cast<Computed_field_subobject_group *>(this->local_element_group[index]->core);
		subobject_group->clear();
		subobject_group->setOwnerGroup(0);
		check_subobject_group_dependency(subobject_group);
		cmzn_field_destroy(&this->local_element_group[index]);
	}
}

void Computed_field_group::setLocalElementGroup(int index, cmzn_field_element_group *element_group)
{
	clearLocalElementGroup(index);
	if (element_group)
	{
		Computed_field_element_group_core_cast(element_group)->setOwnerGroup(this);
		this->local_element_group[index] = cmzn_field_access(cmzn_field_element_group_base_cast(element_group));
	}
}

void Computed_field_group::clearLocalNodeGroup(bool isData)
{
	cmzn_field_id *node_group_field_address = (isData) ? &local_data_group : &local_node_group;
	if (*node_group_field_address)
	{
		Computed_field_subobject_group *subobject_group =
			static_cast<Computed_field_subobject_group *>((*node_group_field_address)->core);
		subobject_group->clear();
		subobject_group->setOwnerGroup(0);
		check_subobject_group_dependency(subobject_group);
		cmzn_field_destroy(node_group_field_address);
	}
}

void Computed_field_group::setLocalNodeGroup(bool isData, cmzn_field_node_group *node_group)
{
	clearLocalNodeGroup(isData);
	if (node_group)
	{
		Computed_field_node_group_core_cast(node_group)->setOwnerGroup(this);
		cmzn_field_id *node_group_field_address = (isData) ? &local_data_group : &local_node_group;
		*node_group_field_address = cmzn_field_access(cmzn_field_node_group_base_cast(node_group));
	}
}

inline Computed_field *Computed_field_cast(
	cmzn_field_group *group_field)
{
	return (reinterpret_cast<Computed_field*>(group_field));
}

inline Computed_field_group *Computed_field_group_core_cast(
	cmzn_field_group *group_field)
{
	return (static_cast<Computed_field_group*>(
		reinterpret_cast<Computed_field*>(group_field)->core));
}

/***************************************************************************//**
 * Compare the type specific data.
 */
int Computed_field_group::compare(Computed_field_core *other_core)
{
	int return_code;

	ENTER(Computed_field_group::compare);
	if (field && dynamic_cast<Computed_field_group*>(other_core))
	{
		return_code = 1;
	}
	else
	{
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_group::compare */

/***************************************************************************//**
 * Evaluates to 1 if domain location is in group, otherwise 0.
 */
int Computed_field_group::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
	valueCache.values[0] = 0.0;
	if (contains_all)
	{
		valueCache.values[0] = 1;
	}
#if defined (USE_OPENCASCADE)
	else if (dynamic_cast<Field_cad_geometry_location*>(location))
	{
		printf("=== Cad geometry field location\n");
	}
#endif /* defined (USE_OPENCASCADE) */
	else
	{
		const Field_location_element_xi *element_xi_location;
		if (cache.get_location_node())
		{
			if (local_node_group)
			{
				const RealFieldValueCache *sourceCache = RealFieldValueCache::cast(local_node_group->evaluate(cache));
				if (sourceCache)
				{
					valueCache.values[0] = sourceCache->values[0];
				}
			}
			if (local_data_group && (0.0 == valueCache.values[0]))
			{
				const RealFieldValueCache *sourceCache = RealFieldValueCache::cast(local_data_group->evaluate(cache));
				if (sourceCache)
				{
					valueCache.values[0] = sourceCache->values[0];
				}
			}
		}
		else if (element_xi_location = cache.get_location_element_xi())
		{
			int dimension = element_xi_location->get_element_dimension();
			cmzn_field_id subobject_group_field = get_element_group_field_private(dimension);
			if (subobject_group_field)
			{
				const RealFieldValueCache *sourceCache = RealFieldValueCache::cast(subobject_group_field->evaluate(cache));
				if (sourceCache)
				{
					valueCache.values[0] = sourceCache->values[0];
				}
			}
		}
	}
	return 1;
}

bool Computed_field_group::isEmptyLocal() const
{
	if (contains_all)
		return false;
	if (local_node_group && !isSubGroupEmpty(local_node_group->core))
		return false;
	if (local_data_group && !isSubGroupEmpty(local_data_group->core))
		return false;
	for (int i = 0; i < MAXIMUM_ELEMENT_XI_DIMENSIONS; i++)
	{
		if (this->local_element_group[i] && !isSubGroupEmpty(this->local_element_group[i]->core))
			return false;
	}
	std::map<Computed_field *, Computed_field *>::const_iterator it = domain_selection_group.begin();
	while (it != domain_selection_group.end())
	{
		Computed_field *subobject_group_field = it->second;
		if (!isSubGroupEmpty(subobject_group_field->core))
			return false;
		++it;
	}
	return true;
}

bool Computed_field_group::isEmptyNonLocal() const
{
	for (Region_field_map_const_iterator iter = subregion_group_map.begin();
		iter != subregion_group_map.end(); iter++)
	{
		cmzn_field_group_id subregion_group_field = iter->second;
		if (!isSubGroupEmpty(cmzn_field_group_base_cast(subregion_group_field)->core))
			return false;
	}
	return true;
}

int Computed_field_group::clearLocal()
{
	if (this->isEmptyLocal())
		return CMZN_OK;
	this->beginChange();
	// for efficiently, temporarily disable subelement handling
	cmzn_field_group_subelement_handling_mode oldSubelementHandlingMode = this->subelementHandlingMode;
	this->subelementHandlingMode = CMZN_FIELD_GROUP_SUBELEMENT_HANDLING_MODE_NONE;
	contains_all = false;
	clearLocalNodeGroup(/*isData*/false); 
	clearLocalNodeGroup(/*isData*/true);
	for (int i = 0; i < MAXIMUM_ELEMENT_XI_DIMENSIONS; i++)
		clearLocalElementGroup(i);
	change_detail.changeRemoveLocal();
	this->subelementHandlingMode = oldSubelementHandlingMode;
	this->field->setChanged();
	this->endChange();
	return CMZN_OK;
};

int Computed_field_group::clear()
{
	int return_code = 0;
	this->beginChange();
	for (Region_field_map_iterator iter = subregion_group_map.begin();
		iter != subregion_group_map.end(); iter++)
	{
		Computed_field_group *group_core = Computed_field_group_core_cast(iter->second);
		group_core->clear();
	}
	return_code = clearLocal();
	this->field->setChanged();
	this->endChange();
	return return_code;
};

int Computed_field_group::check_subobject_group_dependency(Computed_field_core *source_core)
{
	Computed_field_subobject_group *subobject_group = dynamic_cast<Computed_field_subobject_group *>(source_core);
	/* check_dependency method is not sufficient to determine a subobject group has changed or not for a group */
	if (subobject_group->check_dependency_for_group_special())
	{
		field->setChangedPrivate(MANAGER_CHANGE_FULL_RESULT(Computed_field));
		const cmzn_field_subobject_group_change_detail *subobject_group_change_detail =
			dynamic_cast<const cmzn_field_subobject_group_change_detail *>(source_core->get_change_detail());
		if (subobject_group_change_detail)
			change_detail.changeMergeLocal(subobject_group_change_detail->getChangeSummary());
	}
	return 1;
}

cmzn_field_change_detail *Computed_field_group::extract_change_detail()
{
	if (this->change_detail.getChangeSummary() == CMZN_FIELD_GROUP_CHANGE_NONE)
		return NULL;
	cmzn_field_hierarchical_group_change_detail *prior_change_detail =
		new cmzn_field_hierarchical_group_change_detail(change_detail);
#ifdef DEBUG_CODE
	{
		cmzn_region *region = Computed_field_get_region(field);
		char *path = cmzn_region_get_path(region);
		display_message(INFORMATION_MESSAGE, "Group %s%s change local %d non-local %d\n", path, field->name,
			prior_change_detail->getLocalChange(), prior_change_detail->getNonLocalChange());
		DEALLOCATE(path);
	}
#endif // DEBUG_CODE
	change_detail.clear();
	return prior_change_detail;
}

int Computed_field_group::check_dependency()
{
	if (field)
	{
		if (local_node_group)
			check_subobject_group_dependency(local_node_group->core);
		if (local_data_group)
			check_subobject_group_dependency(local_data_group->core);
		for (int i = 0; i < MAXIMUM_ELEMENT_XI_DIMENSIONS; i++)
		{
			if (this->local_element_group[i])
			{
				check_subobject_group_dependency(this->local_element_group[i]->core);
			}
		}
		std::map<Computed_field *, Computed_field *>::const_iterator it = domain_selection_group.begin();
		while (it != domain_selection_group.end())
		{
			Computed_field *subobject_group_field = it->second;
			check_subobject_group_dependency(subobject_group_field->core);
			++it;
		}
		return field->manager_change_status;
	}
	return MANAGER_CHANGE_NONE(Computed_field);
}

void Computed_field_group::propagate_hierarchical_field_changes(
	MANAGER_MESSAGE(Computed_field) *message)
{
	if (message)
	{
		for (Region_field_map_iterator iter = subregion_group_map.begin();
			iter != subregion_group_map.end(); iter++)
		{
			cmzn_field_group_id subregion_group = iter->second;
			// future optimisation: check subfield is from changed region
			const cmzn_field_change_detail *source_change_detail = NULL;
			int change = Computed_field_manager_message_get_object_change_and_detail(
				message, cmzn_field_group_base_cast(subregion_group), &source_change_detail);
			if (change != MANAGER_CHANGE_NONE(Computed_field))
			{
				if (source_change_detail)
				{
					const cmzn_field_group_base_change_detail *subregion_group_change_detail =
						dynamic_cast<const cmzn_field_group_base_change_detail *>(source_change_detail);
					if (subregion_group_change_detail)
					{
						const int subregion_group_change = subregion_group_change_detail->getChangeSummary();
						if (subregion_group_change != CMZN_FIELD_GROUP_CHANGE_NONE)
						{
							this->change_detail.changeMergeNonlocal(subregion_group_change);
							Computed_field_dependency_changed(field);
						}
					}
					else
					{
						display_message(WARNING_MESSAGE, "Sub-region group changes could not be propagated.");
					}
				}
				// we have found only possible subgroup for sub-region:
				break;
			}
		}
	}
}

/***************************************************************************//**
 * Writes type-specific details of the field to the console. 
 */
int Computed_field_group::list()
{
	int return_code;
	
	ENTER(List_Computed_field_group);
	if (field)
	{
		display_message(INFORMATION_MESSAGE, "    Region : ");
		if (region)
		{
			char *path = cmzn_region_get_path(region);
			display_message(INFORMATION_MESSAGE, "%s", path);
			DEALLOCATE(path);
		}
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_group.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_group */

int Computed_field_group::removeRegion(cmzn_region_id region)
{
	cmzn_field_group_id subgroup = getSubRegionGroup(region);
	if (subgroup)
	{
		Computed_field_group *group_core = Computed_field_group_core_cast(subgroup);
		group_core->clearLocal();
		cmzn_field_group_destroy(&subgroup);
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

bool Computed_field_group::containsRegion(cmzn_region_id region)
{
	bool result = false;
	cmzn_field_group_id subgroup = getSubRegionGroup(region);
	if (subgroup)
	{
		Computed_field_group *group_core = Computed_field_group_core_cast(subgroup);
		result = group_core->containsLocalRegion();
		cmzn_field_group_destroy(&subgroup);
	}
	return result;
}

int Computed_field_group::setSubelementHandlingMode(
	cmzn_field_group_subelement_handling_mode modeIn)
{
	if (modeIn == CMZN_FIELD_GROUP_SUBELEMENT_HANDLING_MODE_INVALID)
		return CMZN_ERROR_ARGUMENT;
	if (modeIn != this->subelementHandlingMode)
	{
		this->subelementHandlingMode = modeIn;
		this->setChanged();
	}
	// propagate down subregion group tree
	for (Region_field_map_iterator iter = this->subregion_group_map.begin();
		iter != this->subregion_group_map.end(); iter++)
	{
		cmzn_field_group_id subregion_group = iter->second;
		Computed_field_group_core_cast(subregion_group)->setSubelementHandlingMode(modeIn);
	}
	return CMZN_OK;
}

Computed_field_element_group *Computed_field_group::getElementGroupPrivate(FE_mesh *fe_mesh, bool create)
{
	if (!fe_mesh)
		return 0;
	const int dimension = fe_mesh->getDimension();
	if (this->local_element_group[dimension - 1])
		return static_cast<Computed_field_element_group *>(this->local_element_group[dimension - 1]->core);
	if (create)
	{
		cmzn_fieldmodule_id fieldmodule = cmzn_field_get_fieldmodule(this->field);
		cmzn_mesh_id mesh = cmzn_fieldmodule_find_mesh_by_dimension(fieldmodule, dimension);
		cmzn_field_element_group_id element_group = this->get_element_group(mesh);
		if (!element_group)
			element_group = this->create_element_group(mesh);
		Computed_field_element_group *elementGroupCore = Computed_field_element_group_core_cast(element_group);
		cmzn_field_element_group_destroy(&element_group);
		cmzn_mesh_destroy(&mesh);
		cmzn_fieldmodule_destroy(&fieldmodule);
		return elementGroupCore;
	}
	return 0;
}

Computed_field_node_group *Computed_field_group::getNodeGroupPrivate(cmzn_field_domain_type domain_type, bool create)
{
	switch (domain_type)
	{
	case CMZN_FIELD_DOMAIN_TYPE_NODES:
		if (this->local_node_group)
			return static_cast<Computed_field_node_group *>(this->local_node_group->core);
		break;
	case CMZN_FIELD_DOMAIN_TYPE_DATAPOINTS:
		if (this->local_data_group)
			return static_cast<Computed_field_node_group *>(this->local_data_group->core);
		break;
	default:
		display_message(ERROR_MESSAGE, "Computed_field_group::getNodeGroupPrivate.  Invalid domain_type");
		return 0;
		break;
	}
	if (create)
	{
		cmzn_fieldmodule_id fieldmodule = cmzn_field_get_fieldmodule(this->field);
		cmzn_nodeset_id nodeset = cmzn_fieldmodule_find_nodeset_by_field_domain_type(fieldmodule, domain_type);
		cmzn_field_node_group_id node_group = this->get_node_group(nodeset);
		if (!node_group)
			node_group = this->create_node_group(nodeset);
		Computed_field_node_group *nodeGroupCore = Computed_field_node_group_core_cast(node_group);
		cmzn_field_node_group_destroy(&node_group);
		cmzn_nodeset_destroy(&nodeset);
		cmzn_fieldmodule_destroy(&fieldmodule);
		return nodeGroupCore;
	}
	return 0;
}

cmzn_field_group_id Computed_field_group::getSubRegionGroup(cmzn_region_id subregion)
{
	cmzn_field_group_id subregion_group = NULL;
	if (region == subregion)
	{
		subregion_group = cmzn_field_cast_group(this->getField());
	}
	else
	{
		Region_field_map_iterator iter = subregion_group_map.find(subregion);
		if (iter != subregion_group_map.end())
		{
			subregion_group = iter->second;
			ACCESS(Computed_field)(cmzn_field_group_base_cast(subregion_group));
		}
		if (!subregion_group && !subregion_group_map.empty())
		{
			for (iter = subregion_group_map.begin(); iter != subregion_group_map.end(); iter++)
			{
				cmzn_field_group_id temp = iter->second;
				Computed_field_group *group_core = Computed_field_group_core_cast(temp);
				subregion_group = group_core->getSubRegionGroup(subregion);
				if (subregion_group)
					break;
			}
		}
	}
	return subregion_group;
}

cmzn_field_group_id Computed_field_group::createSubRegionGroup(cmzn_region_id subregion)
{
	cmzn_field_group_id subregion_group = NULL;
	if (cmzn_region_contains_subregion(region, subregion) && region != subregion)
	{
		cmzn_region_id parent_region = subregion->getParent();
		if (parent_region != region)
		{
			cmzn_field_group_id temp = getSubRegionGroup(subregion);
			if (!temp)
			{
				/* this will construct the hierarchy tree */
				temp = getSubRegionGroup(parent_region);
				if (!temp)
					temp = createSubRegionGroup(parent_region);
				if (temp)
				{
					Computed_field_group *group_core = Computed_field_group_core_cast(temp);
					subregion_group = group_core->createSubRegionGroup(subregion);
				}
			}
			if (temp)
				cmzn_field_group_destroy(&temp);
		}
		else // (parent_region == region)
		{
			Region_field_map_iterator pos = subregion_group_map.find(subregion);
			if (pos == subregion_group_map.end())
			{
				cmzn_fieldmodule_id fieldmodule = cmzn_region_get_fieldmodule(subregion);
				cmzn_field_id generic_field =
					cmzn_fieldmodule_find_field_by_name(fieldmodule, this->getField()->name);
				if (generic_field)
				{
					subregion_group = cmzn_field_cast_group(generic_field);
					// Not calling cmzn_field_set_managed(subregion_group, 0);
					cmzn_field_destroy(&generic_field);
				}
				if (!subregion_group)
				{
					cmzn_fieldmodule_begin_change(fieldmodule);
					subregion_group = reinterpret_cast<cmzn_field_group_id>(cmzn_fieldmodule_create_field_group(fieldmodule));
					cmzn_field_set_name(cmzn_field_group_base_cast(subregion_group), this->getField()->name);
					cmzn_field_group_set_subelement_handling_mode(subregion_group, this->subelementHandlingMode);
					cmzn_fieldmodule_end_change(fieldmodule);
				}
				cmzn_fieldmodule_destroy(&fieldmodule);
				cmzn_field_access(cmzn_field_group_base_cast(subregion_group));
				subregion_group_map.insert(std::make_pair(subregion, subregion_group));
			}
			// else already exists: fail
		}
	}
	return subregion_group;
}

cmzn_field_group_id Computed_field_group::getFirstNonEmptyGroup()
{
	if (!isEmptyLocal())
	{
		return cmzn_field_cast_group(this->getField());
	}
	if (!subregion_group_map.empty())
	{
		cmzn_field_group_id subregion_group = 0;
		Region_field_map_iterator iter;
		for (Region_field_map_iterator iter = subregion_group_map.begin();
			iter != subregion_group_map.end(); iter++)
		{
			cmzn_field_group_id temp = iter->second;
			Computed_field_group *group_core = Computed_field_group_core_cast(temp);
			subregion_group = group_core->getFirstNonEmptyGroup();
			if (subregion_group)
				return subregion_group;
		}
	}
	return 0;
}

char *Computed_field_group::get_standard_node_group_name(cmzn_nodeset_id master_nodeset)
{
	char *name = cmzn_field_get_name(this->getField());
	int error = 0;
	append_string(&name, ".", &error);
	char *nodeset_name = cmzn_nodeset_get_name(master_nodeset);
	append_string(&name, nodeset_name, &error);
	DEALLOCATE(nodeset_name);
	return name;
}

cmzn_field_node_group_id Computed_field_group::create_node_group(cmzn_nodeset_id nodeset)
{
	if (contains_all || (!nodeset))
		return 0;
	cmzn_field_node_group_id node_group = this->get_node_group(nodeset);
	if (node_group)
		cmzn_field_node_group_destroy(&node_group); // can't create if already exists
	else if (cmzn_nodeset_get_region_internal(nodeset) != this->region)
	{
		cmzn_field_group *subregionGroup = this->getSubRegionGroup(cmzn_nodeset_get_region_internal(nodeset));
		if (!subregionGroup)
			subregionGroup = this->createSubRegionGroup(cmzn_nodeset_get_region_internal(nodeset));
		if (subregionGroup)
		{
			node_group = Computed_field_group_core_cast(subregionGroup)->create_node_group(nodeset);
			cmzn_field_group_destroy(&subregionGroup);
		}
	}
	else
	{
		cmzn_nodeset_id master_nodeset = cmzn_nodeset_get_master_nodeset(nodeset);
		cmzn_fieldmodule_id field_module = cmzn_region_get_fieldmodule(region);
		cmzn_fieldmodule_begin_change(field_module);
		cmzn_field_id node_group_field = cmzn_fieldmodule_create_field_node_group(field_module, master_nodeset);
		if (node_group_field)
		{
			node_group = cmzn_field_cast_node_group(node_group_field);
			char *name = get_standard_node_group_name(master_nodeset);
			cmzn_field_set_name(node_group_field, name);
			DEALLOCATE(name);
			setLocalNodeGroup(cmzn_nodeset_is_data_internal(master_nodeset), node_group);
			cmzn_field_destroy(&node_group_field);
		}
		cmzn_fieldmodule_end_change(field_module);
		cmzn_fieldmodule_destroy(&field_module);
		cmzn_nodeset_destroy(&master_nodeset);
	}
	return (node_group);
}

cmzn_field_node_group_id Computed_field_group::get_node_group(cmzn_nodeset_id nodeset)
{
	if (contains_all || (!nodeset))
		return 0;
	cmzn_field_node_group_id node_group = 0;
	FE_nodeset *fe_nodeset = cmzn_nodeset_get_FE_nodeset_internal(nodeset);
	cmzn_region *nodeset_region = FE_region_get_cmzn_region(fe_nodeset->get_FE_region());
	if (nodeset_region != this->region)
	{
		cmzn_field_group *subregionGroup = this->getSubRegionGroup(nodeset_region);
		if (subregionGroup)
		{
			node_group = Computed_field_group_core_cast(subregionGroup)->get_node_group(nodeset);
			cmzn_field_group_destroy(&subregionGroup);
		}
	}
	else
	{
		bool use_data = cmzn_nodeset_is_data_internal(nodeset);
		if (use_data)
		{
			if (this->local_data_group)
				node_group = cmzn_field_cast_node_group(this->local_data_group);
		}
		else if (this->local_node_group)
			node_group = cmzn_field_cast_node_group(this->local_node_group);
		if (!node_group)
		{
			// find by name & check it is for same FE_nodeset
			cmzn_nodeset_id master_nodeset = cmzn_nodeset_get_master_nodeset(nodeset);
			cmzn_fieldmodule_id field_module = cmzn_region_get_fieldmodule(region);
			char *name = get_standard_node_group_name(master_nodeset);
			cmzn_field_id node_group_field = cmzn_fieldmodule_find_field_by_name(field_module, name);
			DEALLOCATE(name);
			node_group = cmzn_field_cast_node_group(node_group_field);
			if (node_group)
			{
				if (Computed_field_node_group_core_cast(node_group)->get_fe_nodeset() == fe_nodeset)
					setLocalNodeGroup(use_data, node_group);
				else // wrong nodeset
					cmzn_field_node_group_destroy(&node_group);
			}
			cmzn_field_destroy(&node_group_field);
			cmzn_fieldmodule_destroy(&field_module);
			cmzn_nodeset_destroy(&master_nodeset);
		}
	}
	return (node_group);
}

char *Computed_field_group::get_standard_element_group_name(cmzn_mesh_id master_mesh)
{
	char *name = cmzn_field_get_name(this->getField());
	int error = 0;
	append_string(&name, ".", &error);
	char *mesh_name = cmzn_mesh_get_name(master_mesh);
	append_string(&name, mesh_name, &error);
	DEALLOCATE(mesh_name);
	return name;
}

cmzn_field_element_group_id Computed_field_group::create_element_group(cmzn_mesh_id mesh)
{
	if (contains_all || (!mesh))
		return 0;
	cmzn_field_element_group_id element_group = get_element_group(mesh);
	if (element_group)
		cmzn_field_element_group_destroy(&element_group); // can't create if already exists
	else if (cmzn_mesh_get_region_internal(mesh) != this->region)
	{
		cmzn_field_group *subregionGroup = this->getSubRegionGroup(cmzn_mesh_get_region_internal(mesh));
		if (!subregionGroup)
			subregionGroup = this->createSubRegionGroup(cmzn_mesh_get_region_internal(mesh));
		if (subregionGroup)
		{
			element_group = Computed_field_group_core_cast(subregionGroup)->create_element_group(mesh);
			cmzn_field_group_destroy(&subregionGroup);
		}
	}
	else
	{
		cmzn_mesh_id master_mesh = cmzn_mesh_get_master_mesh(mesh);
		cmzn_fieldmodule_id field_module = cmzn_region_get_fieldmodule(region);
		cmzn_fieldmodule_begin_change(field_module);
		cmzn_field_id element_group_field = cmzn_fieldmodule_create_field_element_group(field_module, master_mesh);
		element_group = cmzn_field_cast_element_group(element_group_field);
		char *name = get_standard_element_group_name(master_mesh);
		cmzn_field_set_name(element_group_field, name);
		DEALLOCATE(name);
		this->setLocalElementGroup(cmzn_mesh_get_dimension(mesh) - 1, element_group);
		cmzn_field_destroy(&element_group_field);
		cmzn_fieldmodule_end_change(field_module);
		cmzn_fieldmodule_destroy(&field_module);
		cmzn_mesh_destroy(&master_mesh);
	}
	return (element_group);
}

cmzn_field_element_group_id Computed_field_group::get_element_group(cmzn_mesh_id mesh)
{
	if (contains_all || (!mesh))
		return 0;
	cmzn_field_element_group_id element_group = 0;
	if (cmzn_mesh_get_region_internal(mesh) != this->region)
	{
		cmzn_field_group *subregionGroup = this->getSubRegionGroup(cmzn_mesh_get_region_internal(mesh));
		if (subregionGroup)
		{
			element_group = Computed_field_group_core_cast(subregionGroup)->get_element_group(mesh);
			cmzn_field_group_destroy(&subregionGroup);
		}
	}
	else
	{
		int dimension = cmzn_mesh_get_dimension(mesh);
		if (this->local_element_group[dimension - 1])
			element_group = cmzn_field_cast_element_group(this->local_element_group[dimension - 1]);
		else
		{
			// find by name & check it is for same master mesh
			cmzn_mesh_id master_mesh = cmzn_mesh_get_master_mesh(mesh);
			cmzn_fieldmodule_id field_module = cmzn_region_get_fieldmodule(region);
			char *name = get_standard_element_group_name(master_mesh);
			cmzn_field_id element_group_field = cmzn_fieldmodule_find_field_by_name(field_module, name);
			DEALLOCATE(name);
			element_group = cmzn_field_cast_element_group(element_group_field);
			if (element_group)
			{
				if (cmzn_mesh_get_FE_mesh_internal(master_mesh) == 
						Computed_field_element_group_core_cast(element_group)->get_fe_mesh())
					this->setLocalElementGroup(dimension - 1, element_group);
				else // wrong mesh
					cmzn_field_element_group_destroy(&element_group);
			}
			cmzn_field_destroy(&element_group_field);
			cmzn_fieldmodule_destroy(&field_module);
			cmzn_mesh_destroy(&master_mesh);
		}
	}
	return (element_group);
}

cmzn_field_id Computed_field_group::get_subobject_group_for_domain(cmzn_field_id domain)
{
	Computed_field *field = NULL;
	std::map<Computed_field *, Computed_field *>::const_iterator it;
	it = domain_selection_group.find(domain);
	if (it != domain_selection_group.end())
	{
		field = it->second;
		cmzn_field_access(field);
	}

	return field;
}

#if defined (USE_OPENCASCADE)
cmzn_field_id Computed_field_group::create_cad_primitive_group(cmzn_field_cad_topology_id cad_topology_domain)
{
	Computed_field *field = NULL;
	if (cad_topology_domain)
	{
		const char *base_name = "cad_primitive_selection";
		const char *domain_field_name = cmzn_field_get_name(reinterpret_cast<cmzn_field_id>(cad_topology_domain));
		char *field_name = NULL;
		int error = 0;
		ALLOCATE(field_name, char, strlen(base_name)+strlen(domain_field_name)+2);
		field_name[0] = '\0';
		append_string(&field_name, base_name, &error);
		append_string(&field_name, "_", &error);
		append_string(&field_name, domain_field_name, &error);

		cmzn_fieldmodule_id field_module =
			cmzn_region_get_fieldmodule(region);
		cmzn_fieldmodule_set_field_name(field_module, field_name);
		field = cmzn_fieldmodule_create_field_cad_primitive_group_template(field_module);
		Computed_field *cad_topology_key = reinterpret_cast<cmzn_field_id>(cad_topology_domain);
		domain_selection_group.insert(std::pair<Computed_field *, Computed_field *>(cad_topology_key, field));

		cmzn_fieldmodule_destroy(&field_module);
		cmzn_field_access(field);
		DEALLOCATE(field_name);
	}
	else
	{
		display_message(ERROR_MESSAGE, "Computed_field_group::create_cad_primitive_group.  Invalid arguments\n");
	}

	return (field);
}

int Computed_field_group::clear_region_tree_cad_primitive()
{
	Region_field_map_iterator pos;
	int return_code = 1;
	cmzn_field_group_id group_field = NULL;
	std::map<Computed_field *, Computed_field *>::iterator it = domain_selection_group.begin();
	while (it != domain_selection_group.end())
	{
		cmzn_field_cad_primitive_group_template_id cad_primitive_group =
			cmzn_field_cast_cad_primitive_group_template(it->second);
		return_code = cmzn_field_cad_primitive_group_template_clear(cad_primitive_group);
		this->field->setChanged();
		//cmzn_field_id cad_primitive_group_field = reinterpret_cast<Computed_field*>(cad_primitive_group);
		cmzn_field_cad_primitive_group_template_destroy(&cad_primitive_group);
		cmzn_field_destroy(&it->second);
		domain_selection_group.erase(it++);
	}
	if (!subregion_group_map.empty())
	{
		for (pos = subregion_group_map.begin(); pos != subregion_group_map.end(); pos++)
		{
			group_field = pos->second;
			cmzn_field_group_clear_region_tree_cad_primitive(group_field);
		}
	}

	return (return_code);
}

#endif /* defined (USE_OPENCASCADE) */


int Computed_field_group::addRegion(struct cmzn_region *child_region)
{
	if (cmzn_region_contains_subregion(region, child_region))
	{
		cmzn_region_begin_hierarchical_change(region);
		cmzn_field_group_id subregion_group = getSubRegionGroup(child_region);
		if (!subregion_group)
			subregion_group = createSubRegionGroup(child_region);
		Computed_field_group *group_core = Computed_field_group_core_cast(subregion_group);
		group_core->addLocalRegion();
		cmzn_field_group_destroy(&subregion_group);
		cmzn_region_end_hierarchical_change(region);
		return CMZN_OK;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_group::addRegion.  Sub region is not a child region"
			"or part of the parent region");
	}
	return CMZN_ERROR_ARGUMENT;
}

const char* Computed_field_group::get_type_string()
{
	return (computed_field_group_type_string);
}

void Computed_field_group::remove_child_group(struct cmzn_region *child_region)
{
	Region_field_map_iterator iter = this->subregion_group_map.find(child_region);
	if (iter != this->subregion_group_map.end())
	{
		cmzn_field_group_id subregion_group_field = iter->second;
		Computed_field_group *group_core = Computed_field_group_core_cast(subregion_group_field);
		const bool nonEmptySubregionRemoved = !group_core->isEmpty();
		subregion_group_map.erase(iter);
		cmzn_field_group_destroy(&subregion_group_field);
		if (nonEmptySubregionRemoved)
		{
			this->change_detail.changeMergeNonlocal(CMZN_FIELD_GROUP_CHANGE_REMOVE);
			this->field->setChanged();
		}
	}
}

int Computed_field_group::remove_empty_subgroups()
{
	/* remove empty subobject groups */
	if (local_node_group)
	{
		Computed_field_group_base *group_base = static_cast<Computed_field_group_base *>(local_node_group->core);
		if (group_base->isEmpty())
			this->clearLocalNodeGroup(/*isData*/false);
	}
	if (local_data_group)
	{
		Computed_field_group_base *group_base = static_cast<Computed_field_group_base *>(local_data_group->core);
		if (group_base->isEmpty())
			this->clearLocalNodeGroup(/*isData*/true);
	}
	for (int i = 0; i < MAXIMUM_ELEMENT_XI_DIMENSIONS; i++)
	{
		if (this->local_element_group[i])
		{
			Computed_field_subobject_group *subobject_group = static_cast<Computed_field_subobject_group *>(
				this->local_element_group[i]->core);
			if (subobject_group->isEmpty())
				this->clearLocalElementGroup(i);
		}
	}
	/* remove empty subregion group */
	for (Region_field_map_iterator iter = subregion_group_map.begin();
		iter != subregion_group_map.end();)
	{
		cmzn_field_group_id subregion_group_field = iter->second;
		Computed_field_group *group_core = Computed_field_group_core_cast(subregion_group_field);
		group_core->remove_empty_subgroups();
		if (group_core->isEmpty())
		{
			// must transfer non-local changes now
			const int subregion_group_change = group_core->change_detail.getChangeSummary();
			this->change_detail.changeMergeNonlocal(subregion_group_change);
			subregion_group_map.erase(iter++);
			cmzn_field_group_destroy(&subregion_group_field);
		}
		else
		{
			++iter;
		}
	}
	if (this->change_detail.getChangeSummary() != CMZN_FIELD_GROUP_CHANGE_NONE)
		Computed_field_dependency_changed(this->field);
	return 1;
}

#ifdef OLD_CODE
// no longer used by cmgui, but may be restored
int Computed_field_group::clear_region_tree_node(int use_data)
{
	Region_field_map_iterator pos;
	int return_code = 1;
	cmzn_field_group_id group_field = NULL;
	if (!use_data && local_node_group)
	{
		cmzn_field_node_group_id node_group = cmzn_field_cast_node_group(local_node_group);
		cmzn_nodeset_group_id nodeset_group = cmzn_field_node_group_get_nodeset_group(node_group);
		return_code = cmzn_nodeset_group_remove_all_nodes(nodeset_group);
		cmzn_nodeset_group_destroy(&nodeset_group);
		check_subobject_group_dependency(local_node_group->core);
		this->field->setChanged();
		cmzn_field_node_group_destroy(&node_group);
	}
	if (use_data && local_data_group)
	{
		cmzn_field_node_group_id data_group = cmzn_field_cast_node_group(local_data_group);
		cmzn_nodeset_group_id nodeset_group = cmzn_field_node_group_get_nodeset_group(data_group);
		return_code = cmzn_nodeset_group_remove_all_nodes(nodeset_group);
		cmzn_nodeset_group_destroy(&nodeset_group);
		check_subobject_group_dependency(local_data_group->core);
		this->field->setChanged();
		cmzn_field_node_group_destroy(&data_group);
	}
	if (!subregion_group_map.empty())
	{
		for (pos = subregion_group_map.begin(); pos != subregion_group_map.end(); pos++)
		{
			group_field = pos->second;
			if (!use_data)
				cmzn_field_group_clear_region_tree_node(group_field);
			else
				cmzn_field_group_clear_region_tree_data(group_field);
		}
	}
	return (return_code);
}
#endif // OLD_CODE

int Computed_field_group::addLocalRegion()
{
	if (!this->contains_all)
	{
		this->contains_all = true;
		change_detail.changeAddLocal();
		this->field->setChanged();
	}
	return CMZN_OK;
}

int Computed_field_group::removeLocalRegion()
{
	if (this->contains_all)
	{
		this->contains_all = false;
		change_detail.changeRemoveLocal();
		this->field->setChanged();
	}
	return CMZN_OK;
}

bool Computed_field_group::containsLocalRegion()
{
	return contains_all;
}

#ifdef OLD_CODE
// no longer used by cmgui, but may be restored
int Computed_field_group::clear_region_tree_element()
{
	Region_field_map_iterator pos;
	int return_code = 1;
	for (int i = 0; i < MAXIMUM_ELEMENT_XI_DIMENSIONS; i++)
	{
		if (this->local_element_group[i])
		{
			cmzn_field_element_group_id element_group =
				cmzn_field_cast_element_group(this->local_element_group[i]);
			return_code = Computed_field_element_group_core_cast(element_group)->clear();
			check_subobject_group_dependency(this->local_element_group[i]->core);
			this->field->setChanged();
			cmzn_field_element_group_destroy(&element_group);
		}
	}
	if (!subregion_group_map.empty())
	{
		for (pos = subregion_group_map.begin(); pos != subregion_group_map.end(); pos++)
		{
			cmzn_field_group_clear_region_tree_element(pos->second);
		}
	}
	return (return_code);
}
#endif // OLD_CODE

int Computed_field_group::for_each_group_hiearchical(
	cmzn_field_group_iterator_function function, void *user_data)
{
	int return_code = 0;
	if (field)
	{
		// ensure group is accessed while user function is called so not destroyed
		cmzn_field_id access_field = cmzn_field_access(field);
		return_code = function(reinterpret_cast<cmzn_field_group_id>(field), user_data);
		cmzn_field_destroy(&access_field);
		if (return_code)
		{
			for (Region_field_map_iterator child_group_iter = subregion_group_map.begin();
				child_group_iter !=subregion_group_map.end(); ++child_group_iter)
			{
				Computed_field_group *child_group_core = Computed_field_group_core_cast(child_group_iter->second);
				if ((!child_group_core) ||
					(!child_group_core->for_each_group_hiearchical(function, user_data)))
				{
					return_code = 0;
					break;
				}
			}
		}
	}
	return return_code;
}

cmzn_field_group *cmzn_field_cast_group(cmzn_field_id field)
{

	if (field && dynamic_cast<Computed_field_group*>(field->core))
	{
		cmzn_field_access(field);
		return (reinterpret_cast<cmzn_field_group_id>(field));
	}
	else
	{
		return (NULL);
	}
}

cmzn_field_node_group_id cmzn_field_group_create_field_node_group(
	cmzn_field_group_id group, cmzn_nodeset_id nodeset)
{
	if (group)
	{
		Computed_field_group *group_core = Computed_field_group_core_cast(group);
		if (group_core)
			return group_core->create_node_group(nodeset);
	}
	return 0;
}

cmzn_field_node_group_id cmzn_field_group_get_field_node_group(
	cmzn_field_group_id group, cmzn_nodeset_id nodeset)
{
	if (group)
	{
		Computed_field_group *group_core = Computed_field_group_core_cast(group);
		if (group_core)
			return group_core->get_node_group(nodeset);
	}
	return 0;
}

cmzn_field_element_group_id cmzn_field_group_create_field_element_group(
	cmzn_field_group_id group, cmzn_mesh_id mesh)
{
	if (group)
	{
		Computed_field_group *group_core = Computed_field_group_core_cast(group);
		if (group_core)
			return group_core->create_element_group(mesh);
	}
	return 0;
}

cmzn_field_element_group_id cmzn_field_group_get_field_element_group(
	cmzn_field_group_id group, cmzn_mesh_id mesh)
{
	if (group)
	{
		Computed_field_group *group_core = Computed_field_group_core_cast(group);
		if (group_core)
			return group_core->get_element_group(mesh);
	}
	return 0;
}

#if defined (USE_OPENCASCADE)
cmzn_field_cad_primitive_group_template_id cmzn_field_group_create_cad_primitive_group(cmzn_field_group_id group, cmzn_field_cad_topology_id cad_topology_domain)
{
	cmzn_field_cad_primitive_group_template_id cad_primitive_group = NULL;
	if (group)
	{
		Computed_field_group *group_core =
			Computed_field_group_core_cast(group);
		if (group_core)
		{
			cmzn_field_id field = group_core->create_cad_primitive_group(cad_topology_domain);
			if (field != NULL)
			{
				cad_primitive_group = cmzn_field_cast_cad_primitive_group_template(field);
				cmzn_field_destroy(&field);
			}
		}
	}

	return cad_primitive_group;
}

cmzn_field_cad_primitive_group_template_id cmzn_field_group_get_cad_primitive_group(cmzn_field_group_id group, cmzn_field_cad_topology_id cad_topology_domain)
{
	cmzn_field_cad_primitive_group_template_id cad_primitive_group = NULL;
	if (group)
	{
		Computed_field_group *group_core =
			Computed_field_group_core_cast(group);
		if (group_core)
		{
			cmzn_field_id field = group_core->get_subobject_group_for_domain(reinterpret_cast< Computed_field * >(cad_topology_domain));//cad_primitive_group();
			if (field != NULL)
			{
				cad_primitive_group = cmzn_field_cast_cad_primitive_group_template(field);
				cmzn_field_destroy(&field);
			}
		}
	}

	return cad_primitive_group;
}

int cmzn_field_group_clear_region_tree_cad_primitive(cmzn_field_group_id group)
{
	int return_code = 0;
	if (group)
	{
		Computed_field_group *group_core =
			Computed_field_group_core_cast(group);
		if (group_core)
		{
			return_code = group_core->clear_region_tree_cad_primitive();
		}
	}
	return return_code;
}


#endif /* defined (USE_OPENCASCADE) */

cmzn_field_group_id cmzn_field_group_get_subregion_field_group(
	cmzn_field_group_id group, cmzn_region_id subregion)
{
	if (group)
	{
		Computed_field_group *group_core = Computed_field_group_core_cast(group);
		if (group_core)
			return group_core->getSubRegionGroup(subregion);
	}
	return 0;
}

cmzn_field_group_id cmzn_field_group_create_subregion_field_group(
	cmzn_field_group_id group, cmzn_region_id subregion)
{
	if (group)
	{
		Computed_field_group *group_core = Computed_field_group_core_cast(group);
		if (group_core)
			return group_core->createSubRegionGroup(subregion);
	}
	return 0;
}

#ifdef OLD_CODE
// no longer used by cmgui, but may be restored
int cmzn_field_group_clear_region_tree_node(cmzn_field_group_id group)
{
	int return_code = 0;
	if (group)
	{
		Computed_field_group *group_core =
			Computed_field_group_core_cast(group);
		if (group_core)
		{
			return_code = group_core->clear_region_tree_node(/*use_data*/0);
		}
	}
	return return_code;
}

int cmzn_field_group_clear_region_tree_data(cmzn_field_group_id group)
{
	int return_code = 0;
	if (group)
	{
		Computed_field_group *group_core =
			Computed_field_group_core_cast(group);
		if (group_core)
		{
			return_code = group_core->clear_region_tree_node(/*use_data*/1);
		}
	}
	return return_code;
}

int cmzn_field_group_clear_region_tree_element(cmzn_field_group_id group)
{
	int return_code = 0;
	if (group)
	{
		Computed_field_group *group_core =
			Computed_field_group_core_cast(group);
		if (group_core)
		{
			return_code = group_core->clear_region_tree_element();
		}
	}
	return return_code;
}
#endif // OLD_CODE

int cmzn_field_group_remove_empty_subgroups(cmzn_field_group_id group)
{
	if (group)
	{
		Computed_field_group *group_core =
			Computed_field_group_core_cast(group);
		if (group_core)
		{
			return group_core->remove_empty_subgroups();
		}
	}
	return 0;
}

cmzn_field_id cmzn_field_group_get_subobject_group_field_for_domain_field(cmzn_field_group_id group, cmzn_field_id domain)
{
	Computed_field *field = NULL;

	if (group)
	{
		Computed_field_group *group_core =
			Computed_field_group_core_cast(group);
		if (group_core)
		{
			field = group_core->get_subobject_group_for_domain(domain);
		}
	}

	return field;
}

int cmzn_field_group_for_each_group_hierarchical(cmzn_field_group_id group,
	cmzn_field_group_iterator_function function, void *user_data)
{
	int return_code = 0;
	if (group && function)
	{
		Computed_field_group *group_core = Computed_field_group_core_cast(group);
		if (group_core)
		{
			return group_core->for_each_group_hiearchical(function, user_data);
		}
	}
	return return_code;
}

int cmzn_field_group_destroy(cmzn_field_group_id *group_address)
{
	return cmzn_field_destroy(reinterpret_cast<cmzn_field_id *>(group_address));
}

cmzn_field *cmzn_fieldmodule_create_field_group(cmzn_fieldmodule_id fieldmodule)
{
	cmzn_field *field = nullptr;
	if (fieldmodule)
	{
		cmzn_region_id region = cmzn_fieldmodule_get_region(fieldmodule);
		field = Computed_field_create_generic(fieldmodule,
			/*check_source_field_regions*/false, 1,
			/*number_of_source_fields*/0, NULL,
			/*number_of_source_values*/0, NULL,
			new Computed_field_group(region));
		cmzn_region_destroy(&region);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_group.  Invalid argument(s)");
	}
	return (field);
}

int cmzn_field_group_clear(cmzn_field_group_id group)
{
	if (group)
	{
		Computed_field_group *group_core =
			Computed_field_group_core_cast(group);
		if (group_core)
		{
			return group_core->clear();
		}
	}
	return 0;
}

int cmzn_field_group_clear_local(cmzn_field_group_id group)
{
	if (group)
	{
		Computed_field_group *group_core =
			Computed_field_group_core_cast(group);
		if (group_core)
			return group_core->clearLocal();
	}
	return CMZN_ERROR_ARGUMENT;
}

bool cmzn_field_group_is_empty(cmzn_field_group_id group)
{
	if (group)
	{
		Computed_field_group *group_core = Computed_field_group_core_cast(group);
		if (group_core)
		{
			return group_core->isEmpty();
		}
	}
	return false;
}

bool cmzn_field_group_was_modified(cmzn_field_group_id group)
{
	if (group)
	{
		Computed_field_group *group_core = Computed_field_group_core_cast(group);
		if (group_core)
		{
			return group_core->wasModified();
		}
	}
	return false;
}

bool cmzn_field_group_is_empty_local(cmzn_field_group_id group)
{
	if (group)
	{
		Computed_field_group *group_core = Computed_field_group_core_cast(group);
		if (group_core)
		{
			return group_core->isEmptyLocal();
		}
	}
	return false;
}

int cmzn_field_group_add_local_region(cmzn_field_group_id group)
{
	if (group)
	{
		Computed_field_group *group_core = Computed_field_group_core_cast(group);
		if (group_core)
		{
			return group_core->addLocalRegion();
		}
	}
	return CMZN_ERROR_ARGUMENT;
}

bool cmzn_field_group_contains_local_region(cmzn_field_group_id group)
{
	if (group)
	{
		Computed_field_group *group_core = Computed_field_group_core_cast(group);
		if (group_core)
		{
			return group_core->containsLocalRegion();
		}
	}
	return false;
}

int cmzn_field_group_remove_local_region(cmzn_field_group_id group)
{
	if (group)
	{
		Computed_field_group *group_core = Computed_field_group_core_cast(group);
		if (group_core)
		{
			return group_core->removeLocalRegion();
		}
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_field_group_add_region(cmzn_field_group_id group, cmzn_region_id region)
{
	if (group && region)
	{
		Computed_field_group *group_core = Computed_field_group_core_cast(group);
		if (group_core)
		{
			return group_core->addRegion(region);
		}
	}
	return CMZN_ERROR_ARGUMENT;
}

bool cmzn_field_group_contains_region(cmzn_field_group_id group, cmzn_region_id region)
{
	if (group && region)
	{
		Computed_field_group *group_core = Computed_field_group_core_cast(group);
		if (group_core)
		{
			return group_core->containsRegion(region);
		}
	}
	return false;
}

int cmzn_field_group_remove_region(cmzn_field_group_id group, cmzn_region_id region)
{
	if (group && region)
	{
		Computed_field_group *group_core = Computed_field_group_core_cast(group);
		if (group_core)
			return group_core->removeRegion(region);
	}
	return CMZN_ERROR_ARGUMENT;
}

enum cmzn_field_group_subelement_handling_mode
	cmzn_field_group_get_subelement_handling_mode(cmzn_field_group_id group)
{
	if (group)
		return Computed_field_group_core_cast(group)->getSubelementHandlingMode();
	return CMZN_FIELD_GROUP_SUBELEMENT_HANDLING_MODE_INVALID;
}

int cmzn_field_group_set_subelement_handling_mode(cmzn_field_group_id group,
	enum cmzn_field_group_subelement_handling_mode mode)
{
	if (group)
		return Computed_field_group_core_cast(group)->setSubelementHandlingMode(mode);
	return CMZN_ERROR_ARGUMENT;
}

cmzn_field_group_id cmzn_field_group_get_first_non_empty_subregion_field_group(
	cmzn_field_group_id group)
{
	if (group)
	{
		Computed_field_group *group_core =
			Computed_field_group_core_cast(group);
		if (group_core)
		{
			return group_core->getFirstNonEmptyGroup();
		}
	}
	return 0;
}

int cmzn_field_is_type_group(cmzn_field_id field, void *dummy_void)
{
	int return_code;
	USE_PARAMETER(dummy_void);
	if (field)
	{
		if (dynamic_cast<Computed_field_group*>(field->core))
		{
			return_code = 1;
		}
		else
		{
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_field_is_type_group.  Missing field");
		return_code = 0;
	}

	return (return_code);
} /* cmzn_field_is_type_group */
