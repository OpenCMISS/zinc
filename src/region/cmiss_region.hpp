/***************************************************************************//**
 * FILE : cmiss_region.h
 *
 * Definition of cmzn_region, container of fields for representing model data,
 * and child regions for building hierarchical models.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (CMZN_REGION_H)
#define CMZN_REGION_H

#include "opencmiss/zinc/types/contextid.h"
#include "opencmiss/zinc/types/regionid.h"
#include "computed_field/computed_field.h"
#include "computed_field/field_derivative.hpp"
#include "general/callback.h"
#include "general/object.h"
#include <list>


/*
Global constants
----------------
*/

/* separator character for cmzn_region in path strings */
#define CMZN_REGION_PATH_SEPARATOR_CHAR '/'
#define CMZN_REGION_PATH_SEPARATOR_STRING "/"

/*
Global types
------------
*/

/** Data broadcast with callbacks from <cmzn_region> describing the changes. */
class cmzn_region_changes
{
public:
	/* true if the name of this region has changed */
	int name_changed;
	/* true if children added, removed or reordered in cmzn_region */
	int children_changed;
	/* if a single child has been added (and none removed) it is indicated here */
	struct cmzn_region *child_added;  // currently region responsibility to reference count
	/* if a single child has been removed (and none added) it is indicated here */
	struct cmzn_region *child_removed;  // currently region responsibility to reference count

	cmzn_region_changes() :
		name_changed(0),
		children_changed(0),
		child_added(nullptr),
		child_removed(nullptr)
	{
	}

	void clear()
	{
		this->name_changed = 0;
		this->children_changed = 0;
		this->child_added = nullptr;
		this->child_removed = nullptr;
	}
};

DECLARE_CMZN_CALLBACK_TYPES(cmzn_region_change, \
	struct cmzn_region *, cmzn_region_changes *, void);

typedef std::list<cmzn_fieldmodulenotifier *> cmzn_fieldmodulenotifier_list;

struct cmzn_scene;

/**
 * Hierarchical constainer for local fields and Scene and child regions.
 */
struct cmzn_region
{
	friend struct cmzn_context;

private:
	char *name;
	/* non-accessed pointer to context region belongs to */
	cmzn_context *context;
	/* non-accessed pointer to parent region, or NULL if root */
	cmzn_region *parent;
	/* accessed first child and next sibling for building region tree */
	cmzn_region *first_child, *next_sibling;
	/* non-access pointer to previous sibling, if any */
	cmzn_region *previous_sibling;

	/* fields owned by this region (or master) */
	struct MANAGER(Computed_field) *field_manager;
	void *field_manager_callback_id;
	struct FE_region *fe_region;
	int field_cache_size; // 1 more than highest field cache index given out
	// all field caches currently in use for this region, for clearing
	// when fields changed, and adding value caches for new fields.
	std::list<cmzn_fieldcache_id> field_caches;
	std::vector<FieldDerivative *> fieldDerivatives;

	// Scene gives visualisation of region content
	cmzn_scene *scene;

	// incremented if any fields are modified in the region so field caches
	// can detect if their values are invalid.
	int fieldModifyCounter;

	/* increment/decrement change_level to nest changes. Message sent when zero */
	int change_level;
	/* number of hierarchical changes in progress on this region tree. A region's
	 * change_level will have one increment per ancestor hierarchical_change_level.
	 * Must be tracked to safely transfer when re-parenting regions */
	int hierarchical_change_level;
	cmzn_region_changes changes;
	/* list of change callbacks */
	struct LIST(CMZN_CALLBACK_ITEM(cmzn_region_change)) *change_callback_list;

	// list of notifiers which receive field module callbacks
	cmzn_fieldmodulenotifier_list notifier_list;

	/* number of objects using this region */
	int access_count;

	cmzn_region(cmzn_context* contextIn);

	~cmzn_region();

	/** Called only by context->createRegion */
	static cmzn_region *create(cmzn_context* contextIn);

	/** Clear context pointer; called only when context is being destroyed. */
	void clearContext()
	{
		this->context = nullptr;
	}

	void detachFields();

	static void Computed_field_change(struct MANAGER_MESSAGE(Computed_field) *message, void *region_void);

	void updateClients();

public:

	inline cmzn_region *access()
	{
		++(this->access_count);
		return this;
	}

	static int deaccess(cmzn_region* &region)
	{
		if (!region)
			return CMZN_ERROR_ARGUMENT;
		--(region->access_count);
		if (region->access_count <= 0)
			delete region;
		region = nullptr;
		return CMZN_OK;
	}

	static void reaccess(cmzn_region* &region, cmzn_region *newRegion)
	{
		if (newRegion)
			++(newRegion->access_count);
		if ((region) && (--(region->access_count) == 0))
			delete region;
		region = newRegion;
	}

	// all code which modifies values of fields must call this to ensure
	// field caches are recalculated
	inline void setFieldModify()
	{
		++(this->fieldModifyCounter);
	}

	// field caches store current value when calculating at a new location
	// and if the region value changes then cache is invalid
	inline int getFieldModifyCounter() const
	{
		return this->fieldModifyCounter;
	}

	void beginChangeFields();

	void endChangeFields();

	void beginChange()
	{
		++(this->change_level);
		this->beginChangeFields();
	}

	// clear cached changes to prevent notifications about them
	void clearCachedChanges();

	int endChange();

	int getSumHierarchicalChangeLevel() const;

	void treeChange(int delta_change_level);

	void beginHierarchicalChange()
	{
		++(this->hierarchical_change_level);
		this->treeChange(+1);
	}

	void endHierarchicalChange()
	{
		--(this->hierarchical_change_level);
		this->treeChange(-1);
	}

	int addCallback(CMZN_CALLBACK_FUNCTION(cmzn_region_change) *function, void *user_data);

	int removeCallback(CMZN_CALLBACK_FUNCTION(cmzn_region_change) *function, void *user_data);

	/**
	 * Returns pointer to context this region was created for. Can be NULL if
	 * context is destroyed already during clean-up.
	 * @return  Non-accessed pointer to cmzn_context or NULL if none/cleared.
	 */
	cmzn_context *getContext() const
	{
		return this->context;
	}

	struct MANAGER(Computed_field) *getFieldManager() const
	{
		return this->field_manager;
	}

	FE_region *get_FE_region() const
	{
		return this->fe_region;
	}

	/**
	 * Callback for changes to FE_region attached to region.
	 * Updates definitions of Computed_field wrappers for changed FE_fields in the
	 * region.
	 * Ensures region has cmiss_number and xi fields, at the appropriate time.
	 * Triggers computed field changes if not already changed.
	 * @private  Should only be called from finite_element_region.cpp
	 */
	void FeRegionChange();

	cmzn_region *findChildByName(const char *name) const;

	cmzn_region *findSubregionAtPath(const char *path) const;

	cmzn_region *createSubregion(const char *path);

	cmzn_fielditerator *createFielditerator() const;

	/** Called only by Field constructor code to assign cache index */
	int addField(cmzn_field *field);

	/** Called only by FieldDerivative to assign cache index */
	void addFieldDerivative(FieldDerivative *fieldDerivative);

	/** Called only by FieldDerivative to remove cache index */
	void removeFieldDerivative(FieldDerivative *fieldDerivative);

	/** Returns size of field cache array to fit all assigned cache indexes. */
	int getFieldcacheSize() const
	{
		return this->field_cache_size;
	}

	/** Called only by Fieldcache constructor.
	 * Adds cache to the list of caches for this region. Region needs this list to
	 * add new value caches for any fields created while the cache exists.
	 */
	void addFieldcache(cmzn_fieldcache *fieldcache)
	{
		if (fieldcache)
			this->field_caches.push_back(fieldcache);
	}

	/** Called only by Fieldcache destructor.
	 * Removes cache from the list of caches for this region.
	 */
	void removeFieldcache(cmzn_fieldcache *fieldcache)
	{
		if (fieldcache)
			this->field_caches.remove(fieldcache);
	}

	/**
	 * Private function for clearing field value caches for field in all caches
	 * listed in region.
	 */
	void clearFieldValueCaches(cmzn_field *field);

	cmzn_field *findFieldByName(const char *fieldName) const
	{
		if (!fieldName)
			return nullptr;
		return FIND_BY_IDENTIFIER_IN_MANAGER(Computed_field, name)(
			(char *)fieldName, this->field_manager);
	}

	const char *getName() const
	{
		return this->name;
	}

	int setName(const char *name);

	cmzn_region *getParent() const
	{
		return this->parent;
	}

	/** @return allocated string /path/to/region/ */
	char *getPath() const;

	char *getRelativePath(cmzn_region *other_region) const;

	cmzn_region *getFirstChild() const
	{
		return this->first_child;
	}

	cmzn_region *getNextSibling() const
	{
		return this->next_sibling;
	}

	cmzn_region *getPreviousSibling() const
	{
		return this->previous_sibling;
	}

	int appendChild(cmzn_region *new_child)
	{
		return this->insertChildBefore(new_child, nullptr);
	}

	int insertChildBefore(cmzn_region *new_child, cmzn_region *ref_child);

	int removeChild(cmzn_region *old_child);

	bool containsSubregion(cmzn_region *subregion) const;

	void addFieldmodulenotifier(cmzn_fieldmodulenotifier *notifier);

	void removeFieldmodulenotifier(cmzn_fieldmodulenotifier *notifier);

	/** @return  Non-accessed pointer to scene for region, or nullptr if none */
	cmzn_scene *getScene() const
	{
		return this->scene;
	}

	/** Only to be called by region or context destructors */
	void detachScene();

};

/*
Global functions
----------------
*/

PROTOTYPE_OBJECT_FUNCTIONS(cmzn_region);

/***************************************************************************//**
 * Remove all nodes, elements, data and finite element fields from this region.
 *
 * @param region  The region to clear the fields from. Must not be a group.
 * @return  1 on success, 0 if no region supplied.
 */
int cmzn_region_clear_finite_elements(struct cmzn_region *region);

/***************************************************************************//**
 * Returns FE_region for this cmzn_region.
 */
struct FE_region *cmzn_region_get_FE_region(struct cmzn_region *region);

/***************************************************************************//**
 * Returns the field manager for this region.
 */
struct MANAGER(Computed_field) *cmzn_region_get_Computed_field_manager(
	struct cmzn_region *region);

int cmzn_region_add_callback(struct cmzn_region *region,
	CMZN_CALLBACK_FUNCTION(cmzn_region_change) *function, void *user_data);
/*******************************************************************************
LAST MODIFIED : 2 December 2002

DESCRIPTION :
Adds a callback to <region> so that when it changes <function> is called with
<user_data>. <function> has 3 arguments, a struct cmzn_region *, a
cmzn_region_changes * and the void *user_data.
==============================================================================*/

int cmzn_region_remove_callback(struct cmzn_region *region,
	CMZN_CALLBACK_FUNCTION(cmzn_region_change) *function, void *user_data);
/*******************************************************************************
LAST MODIFIED : 2 December 2002

DESCRIPTION :
Removes the callback calling <function> with <user_data> from <region>.
==============================================================================*/

/***************************************************************************//**
 * Allocates and returns the path to the root_region ("/").
 *
 * @return  Allocated string "/".
 */
char *cmzn_region_get_root_region_path(void);

/***************************************************************************//**
 * Returns the full path name from the root region to this region. Path name
 * always begins and ends with the CMZN_REGION_PATH_SEPARATOR_CHAR '/'.
 *
 * @param region  The region whose path is requested.
 * @return  On success: allocated string containing full region path.
 */
char *cmzn_region_get_path(struct cmzn_region *region);

/***************************************************************************//**
 * Returns the relative path name to this region from other_region. Path name
 * always begins and ends with the CMZN_REGION_PATH_SEPARATOR_CHAR '/'.
 *
 * @param region  The region whose path is requested.
 * @param other_region  The region the path is relative to.
 * @return  On success: allocated string containing relative region path; on
 * failure: NULL, including case when region is not within other_region.
 */
char *cmzn_region_get_relative_path(struct cmzn_region *region,
	struct cmzn_region *other_region);

/*******************************************************************************
 * Internal only. External API is cmzn_fieldmodule_find_field_by_name.
 * @return  Accessed handle to field of given name, or NULL if none.
 */
cmzn_field_id cmzn_region_find_field_by_name(cmzn_region_id region,
	const char *field_name);

/***************************************************************************//**
 * Deprecated legacy version of cmzn_region_find_subregion_at_path returning
 * non-ACCESSed region as final argument.
 *
 * @param region  The region to search.
 * @param path  The directory-style path to the subregion.
 * @param subregion_address  Address to put region at path. Set to NULL if no
 * region is identified.
 * @return  1 if region found, 0 otherwise.
 */
int cmzn_region_get_region_from_path_deprecated(struct cmzn_region *region,
	const char *path, struct cmzn_region **subregion_address);

/**
 * Returns a reference to the root region of this region.
 *
 * @param region  The region.
 * @return  Accessed reference to root region, or NULL if none.
 */
struct cmzn_region *cmzn_region_get_root(struct cmzn_region *region);

/**
 * Returns true if region has no parent.
 */
bool cmzn_region_is_root(struct cmzn_region *region);

/***************************************************************************//**
 * Separates a region/path/name into the region plus region-path and remainder
 * string containing text from the first unrecognized child region name.
 *
 * Examples:
 * "fibres" or "/fibres" -> root_region, "" and "fibres" if fibres was not a
 *     child region of the root region
 * "heart/fibres" or "/heart/fibres" -> heart region, "heart" and "fibres" if
 *     heart region has no child region called fibres
 * "body/heart" -> heart region, "body/heart" and NULL name if root region
 *     contains body region contains heart region
 * "heart/bob/fred/" -> region heart, "heart" and "bob/fred" if region heart
 *     has no child region called bob
 *
 * @param root_region the starting region for path
 * @path string the input path
 * @param region_address on success, points to region partially matched by path
 * @param region_path_address on success, returns allocated string path to the
 *   returned region, stripped of leading and trailing region path separators
 * @param remainder_address on success, returns pointer to allocated remainder
 *   of path stripped of leading and trailing region path separators, or NULL
 *   if all of path was resolved
 * @return 1 on success, 0 on failure
 */
int cmzn_region_get_partial_region_path(struct cmzn_region *root_region,
	const char *path, struct cmzn_region **region_address,
	char **region_path_address,	char **remainder_address);

int cmzn_region_list(struct cmzn_region *region,
	int indent, int indent_increment);
/*******************************************************************************
LAST MODIFIED : 5 March 2003

DESCRIPTION :
Lists the cmzn_region hierarchy starting from <region>. Contents are listed
indented from the left margin by <indent> spaces; this is incremented by
<indent_increment> for each child level.
==============================================================================*/

/**
 * Check that fields and other object definitions in source region are properly
 * defined and compatible with definitions in target region.
 * Converts legacy field representations e.g. read from older EX files, hence
 * source region can be modified, and function fails if conversion is not
 * possible.
 * Successful from this function is prerequisite for calling cmzn_region_merge.
 * @see cmzn_region_merge
 * @param target_region  Optional target/global region to check compatibility
 * with. Omit to confirm conversion of legacy field representations only.
 * Not modified.
 * @param source_region  Source region to check. Can be modified.
 * @return  True if compatible and conversions successful, false if failed or
 * source region is missing.
 */
bool cmzn_region_can_merge(cmzn_region_id target_region,
	cmzn_region_id source_region);

/**
 * Merge fields and other objects from source region tree into target region,
 * transferring objects in some cases for efficiency.
 * @see cmzn_region_can_merge
 * @param target_region  Target / global region to merge into.
 * @param source_region  Source region to merge from. Modified and left in an
 * in an unusable state by this function as some of its objects, hence must be
 * destroyed after calling.
 * @return  1 on success, 0 on failure.
 */
int cmzn_region_merge(cmzn_region_id target_region, cmzn_region_id source_region);

/** Called only by ~FieldDerivative.
 * Add the field derivative to the list in the region and assign its unique
 * cache index.
 * NOTE: Throws an exception on any failure.
 * @return  Accessed field derivative or nullptr if failed.
 */
void cmzn_region_add_field_derivative(cmzn_region *region,
	FieldDerivative *fieldDerivative);

/** Called only by ~FieldDerivative */
void cmzn_region_remove_field_derivative(cmzn_region *region, FieldDerivative *field_derivative);

#endif /* !defined (CMZN_REGION_H) */
