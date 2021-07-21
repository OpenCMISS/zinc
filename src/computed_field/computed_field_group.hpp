/**
 *FILE : computed_field_group.hpp
 *
 * Implements a "group" computed_field which group regions, 
 * node and data point component.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMPUTED_FIELD_GROUP_HPP)
#define COMPUTED_FIELD_GROUP_HPP

#include "opencmiss/zinc/fieldmodule.h"
#include "opencmiss/zinc/fieldgroup.h"
#include "computed_field/computed_field_group_base.hpp"
#include <map>

struct cmzn_field_hierarchical_group_change_detail : public cmzn_field_group_base_change_detail
{
private:
	int localChangeSummary;
	int nonlocalChangeSummary;

public:
	cmzn_field_hierarchical_group_change_detail() :
		localChangeSummary(CMZN_FIELD_GROUP_CHANGE_NONE),
		nonlocalChangeSummary(CMZN_FIELD_GROUP_CHANGE_NONE)
	{
	}

	void clear()
	{
		localChangeSummary = CMZN_FIELD_GROUP_CHANGE_NONE;
		nonlocalChangeSummary = CMZN_FIELD_GROUP_CHANGE_NONE;
	}

	virtual int getChangeSummary() const
	{
		return localChangeSummary | nonlocalChangeSummary;
	}

	int getLocalChangeSummary() const
	{
		return localChangeSummary;
	}

	int getNonlocalChangeSummary() const
	{
		return nonlocalChangeSummary;
	}

	/** Inform local group has been added, but wasn't before */
	void changeAddLocal()
	{
		localChangeSummary |= CMZN_FIELD_GROUP_CHANGE_ADD;
	}

	/** Inform local group has had part removed */
	void changeRemoveLocal()
	{
		localChangeSummary |= CMZN_FIELD_GROUP_CHANGE_REMOVE;
	}

	void changeMergeLocal(int inChangeSummary)
	{
		localChangeSummary |= inChangeSummary;
	}

	void changeMergeNonlocal(int inChangeSummary)
	{
		nonlocalChangeSummary |= inChangeSummary;
	}
};

typedef std::map<cmzn_region_id, cmzn_field_group_id> Region_field_map;
typedef std::map<cmzn_region_id, cmzn_field_group_id>::iterator Region_field_map_iterator;
typedef std::map<cmzn_region_id, cmzn_field_group_id>::const_iterator Region_field_map_const_iterator;

typedef int (*cmzn_field_group_iterator_function)(cmzn_field_group_id,void *);

class Computed_field_element_group;
class Computed_field_node_group;

class Computed_field_group : public Computed_field_group_base
{
private:
	cmzn_field_hierarchical_group_change_detail change_detail;
	cmzn_region *region;
	bool contains_all;
	cmzn_field_group_subelement_handling_mode subelementHandlingMode;
	Computed_field *local_node_group, *local_data_group, *local_element_group[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	std::map<Computed_field *, Computed_field *> domain_selection_group;
	Region_field_map subregion_group_map;

	void clearLocalElementGroup(int index);
	void setLocalElementGroup(int index, cmzn_field_element_group *element_group);
	void clearLocalNodeGroup(bool isData);
	void setLocalNodeGroup(bool isData, cmzn_field_node_group *node_group);

public:

	Computed_field_group(cmzn_region *region);

	virtual ~Computed_field_group();

	/** @return allocated name for node group for master_nodeset */
	char *get_standard_node_group_name(cmzn_nodeset_id master_nodeset);

	cmzn_field_node_group_id create_node_group(cmzn_nodeset_id nodeset);

	cmzn_field_node_group_id get_node_group(cmzn_nodeset_id nodeset);

	/** @return allocated name for element group for master_mesh */
	char *get_standard_element_group_name(cmzn_mesh_id master_mesh);

	cmzn_field_element_group_id create_element_group(cmzn_mesh_id mesh);

	cmzn_field_element_group_id get_element_group(cmzn_mesh_id mesh);

	cmzn_field_id get_subobject_group_for_domain(cmzn_field_id domain);

#if defined (USE_OPENCASCADE)
	cmzn_field_id create_cad_primitive_group(cmzn_field_cad_topology_id cad_topology_domain);

	int clear_region_tree_cad_primitive();
#endif /*defined (USE_OPENCASCADE) */

	cmzn_field_group_id getSubRegionGroup(cmzn_region_id subregion);

	cmzn_field_group_id createSubRegionGroup(cmzn_region_id subregion);

	cmzn_field_group_id getFirstNonEmptyGroup();

#ifdef OLD_CODE
// no longer used by cmgui, but may be restored
	int clear_region_tree_node(int use_data);

	int clear_region_tree_element();
#endif // OLD_CODE

	int for_each_group_hiearchical(cmzn_field_group_iterator_function function, void *user_data);

	int remove_empty_subgroups();

	virtual cmzn_field_change_detail *extract_change_detail();

	virtual int check_dependency();

	virtual void propagate_hierarchical_field_changes(MANAGER_MESSAGE(Computed_field) *message);

	virtual void subregionRemoved(cmzn_region *subregion)
	{
		this->remove_child_group(subregion);
	}

	bool isEmptyLocal() const;

	virtual bool isEmpty() const
	{
		return (isEmptyLocal() && isEmptyNonLocal());
	}

	bool wasModified() const
	{
		return this->change_detail.getChangeSummary() != CMZN_FIELD_GROUP_CHANGE_NONE;
	}

	virtual int clear();

	int clearLocal();

	int addLocalRegion();

	int removeLocalRegion();

	bool containsLocalRegion();

	int addRegion(struct cmzn_region *child_region);

	int removeRegion(struct cmzn_region *region);

	bool containsRegion(struct cmzn_region *region);

	cmzn_field_group_subelement_handling_mode getSubelementHandlingMode() const
	{
		return this->subelementHandlingMode;
	}

	int setSubelementHandlingMode(cmzn_field_group_subelement_handling_mode mode);

	// for use by subobject group only. Assumes within begin/end change if creating
	Computed_field_element_group *getElementGroupPrivate(FE_mesh *fe_mesh, bool create = false);

	// for use by subobject group only. Assumes within begin/end change if creating
	Computed_field_node_group *getNodeGroupPrivate(cmzn_field_domain_type domain_type, bool create = false);

private:

	Computed_field_core* copy()
	{
		Computed_field_group *core = new Computed_field_group(region);
		core->contains_all = this->contains_all;
		return (core);
	};

	virtual const char* get_type_string();

	void remove_child_group(struct cmzn_region *child_region);

	int compare(Computed_field_core* other_field);

	virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
	{
		inValueCache.getDerivativeValueCache(fieldDerivative)->zeroValues();
		return 1;
	}

	int list();

	cmzn_field_id get_element_group_field_private(int dimension)
	{
		if ((dimension > 0) && (dimension <= MAXIMUM_ELEMENT_XI_DIMENSIONS))
			return this->local_element_group[dimension - 1];
		return 0;
	}

	int getSubgroupLocal();

	int add_region_tree(struct cmzn_region *region_tree);

	int remove_region(struct cmzn_region *child_region);

	int remove_region_tree(struct cmzn_region *child_region);

	int contain_region_tree(struct cmzn_region *child_region);

	inline int isSubGroupEmpty(Computed_field_core *source_core) const
	{
		Computed_field_group_base *group_base = dynamic_cast<Computed_field_group_base *>(source_core);
		if (group_base)
		{
			return group_base->isEmpty();
		}
		display_message(ERROR_MESSAGE,
			"Computed_field_group::isSubGroupEmpty.  Subgroup not derived from Computed_field_group_base");
		return 0;
	}

	bool isEmptyNonLocal() const;

	int check_subobject_group_dependency(Computed_field_core *source_core);

};

/*****************************************************************************//**
 * A convenience function which calls the supplied function for this group and
 * each descendant group throughout the region hierarchy.
 *
 * @param group  group field.
 * @param function  Pointer to the function to be called for each group field.
 * @param user_data  Void pointer to user data to pass to each function.
 * @return 1 on success, 0 on failure.
 */
int cmzn_field_group_for_each_group_hierarchical(cmzn_field_group_id group,
	cmzn_field_group_iterator_function function, void *user_data);

#ifdef OLD_CODE
// no longer used by cmgui, but may be restored
int cmzn_field_group_clear_region_tree_node(cmzn_field_group_id group);

int cmzn_field_group_clear_region_tree_data(cmzn_field_group_id group);

int cmzn_field_group_clear_region_tree_element(cmzn_field_group_id group);
#endif // OLD_CODE

int cmzn_field_is_type_group(cmzn_field_id field, void *dummy_void);

bool cmzn_field_group_was_modified(cmzn_field_group_id group);

#endif /* !defined (COMPUTED_FIELD_GROUP_HPP) */
