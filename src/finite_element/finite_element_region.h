/*******************************************************************************
FILE : finite_element_region.h

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
#if !defined (FINITE_ELEMENT_REGION_H)
#define FINITE_ELEMENT_REGION_H

#include "finite_element/finite_element.h"
#include "general/callback.h"
#include "general/change_log.h"
#include "general/object.h"
#include "region/cmiss_region.hpp"

/*
Global types
------------
*/

/**
 * Internal object containing finite element fields, nodes, elements, datapoints.
 */
struct FE_region;

class DsLabelsChangeLog;

/**
 * Structure describing FE_region field, node and element changes, for passing
 * back to owner region.
 */
class FE_region_changes
{
	struct FE_region *fe_region; // accessed
	struct CHANGE_LOG(FE_field) *fe_field_changes;
	DsLabelsChangeLog *elementChangeLogs[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	// following are set once field changes propagated to given dimension so not redone
	bool propagatedToDimension[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	DsLabelsChangeLog *nodeChangeLogs[2]; // nodes and datapoints
	int access_count;

	FE_region_changes(struct FE_region *fe_region);
	~FE_region_changes();

public:

	static FE_region_changes *create(struct FE_region *fe_region)
	{
		if (fe_region)
			return new FE_region_changes(fe_region);
		return 0;
	}

	FE_region_changes *access()
	{
		++(this->access_count);
		return this;
	}

	static void deaccess(FE_region_changes *&changes)
	{
		if (changes)
		{
			--(changes->access_count);
			if (changes->access_count <= 0)
				delete changes;
			changes = 0;
		}
	}

	DsLabelsChangeLog *getElementChangeLog(int dimension)
	{
		if ((1 <= dimension) && (dimension <= MAXIMUM_ELEMENT_XI_DIMENSIONS))
			return this->elementChangeLogs[dimension - 1];
		return 0;
	}

	struct CHANGE_LOG(FE_field) *getFieldChanges()
	{
		return this->fe_field_changes;
	}

	DsLabelsChangeLog *getNodeChangeLog(cmzn_field_domain_type domainType)
	{
		if (CMZN_FIELD_DOMAIN_TYPE_NODES == domainType)
			return this->nodeChangeLogs[0];
		if (CMZN_FIELD_DOMAIN_TYPE_DATAPOINTS == domainType)
			return this->nodeChangeLogs[1];
		return 0;
	}

	FE_region *get_FE_region() const
	{
		return this->fe_region;
	}

	bool propagateToDimension(int dimension);

};

/*
Global macros
-------------
*/

#define FE_region_FE_object_method_class( object_type ) FE_region_FE_object_method_class_FE_ ## object_type \

#define DEFINE_FE_region_FE_object_method_class( object_type )	\
class FE_region_FE_object_method_class( object_type ) \
{ \
 public: \
	 typedef int change_log_change_object_type; \
	 typedef struct CHANGE_LOG(FE_ ## object_type) change_log_object_type;	\
	 static const enum CHANGE_LOG_CHANGE(FE_ ## object_type) change_log_object_unchanged = \
			CHANGE_LOG_OBJECT_UNCHANGED(FE_ ## object_type);	\
	 static const enum CHANGE_LOG_CHANGE(FE_ ## object_type) change_log_object_added = \
			CHANGE_LOG_OBJECT_ADDED(FE_ ## object_type); \
	 static const enum CHANGE_LOG_CHANGE(FE_ ## object_type) change_log_object_removed = \
			CHANGE_LOG_OBJECT_REMOVED(FE_ ## object_type); \
	 static const enum CHANGE_LOG_CHANGE(FE_ ## object_type) change_log_object_identifier_changed = \
			CHANGE_LOG_OBJECT_IDENTIFIER_CHANGED(FE_ ## object_type);	\
	 static const enum CHANGE_LOG_CHANGE(FE_ ## object_type) change_log_object_not_identifier_changed = \
			CHANGE_LOG_OBJECT_NOT_IDENTIFIER_CHANGED(FE_ ## object_type); \
	 static const enum CHANGE_LOG_CHANGE(FE_ ## object_type) change_log_object_changed = \
			CHANGE_LOG_OBJECT_CHANGED(FE_ ## object_type); \
	 static const enum CHANGE_LOG_CHANGE(FE_ ## object_type) change_log_related_object_changed = \
			CHANGE_LOG_RELATED_OBJECT_CHANGED(FE_ ## object_type); \
	 static inline FE_ ## object_type* string_to_object(struct FE_region *fe_region, const char *string) \
	 { \
			return FE_region_ ## object_type ## _string_to_FE_ ## object_type(fe_region, string); \
	 } \
\
	static inline FE_ ## object_type* get_first_object_that(struct FE_region *fe_region, \
			 LIST_CONDITIONAL_FUNCTION(FE_ ## object_type) *conditional_function, \
			 void *user_data_void) \
	 { \
			return FE_region_get_first_FE_ ## object_type ## _that(fe_region, \
				 conditional_function, user_data_void); \
	 } \
\
	 static inline int FE_region_contains_object(struct FE_region *fe_region, struct FE_## object_type *object) \
	 { \
			return FE_region_contains_FE_ ## object_type(fe_region, object); \
	 } \
\
	 static inline int FE_object_to_string(struct FE_## object_type *object, char **string) \
	 {	\
			return FE_ ##object_type ## _to_ ##object_type## _string(object, string); \
	 } \
\
	 static inline change_log_object_type *get_object_changes(struct FE_region_changes *changes) \
	 { \
			return changes->fe_ ## object_type ## _changes; \
	 } \
\
	 static inline int change_log_query(struct CHANGE_LOG(FE_ ##object_type) *change_log, \
			struct FE_ ##object_type *object, int *change_address )	\
	 { \
			return CHANGE_LOG_QUERY(FE_ ##object_type)( \
				 change_log, object, change_address); \
	 } \
\
};

/*
Global functions
----------------
*/

PROTOTYPE_OBJECT_FUNCTIONS(FE_region);

/**
 * Increments the change counter in <region>. No update callbacks will occur until
 * change counter is restored to zero by calls to FE_region_end_change.
 */
int FE_region_begin_change(struct FE_region *fe_region);

/**
 * Decrements the change counter in <region>. No update callbacks occur until
 * the change counter is restored to zero by this function.
 */
int FE_region_end_change(struct FE_region *fe_region);

/**
 * Return true if the fe_field's owning fe_region is currently caching changes
 * and this fe_field has changes that affect its values.
 */
bool FE_field_has_cached_changes(FE_field *fe_field);

/**
 * Removes all the fields, nodes and elements from <fe_region>.
 * Note this function uses FE_region_begin/end_change so it sends a single change
 * message if not already in the middle of changes.
 */
int FE_region_clear(struct FE_region *fe_region);

/**
 * Internal function for getting pointer to field change log.
 * Warning; do not hold beyond immediate function as logs are constantly
 * recreated.
 */
struct CHANGE_LOG(FE_field) *FE_region_get_FE_field_changes(
	struct FE_region *fe_region);

/**
 * @return  Non-accessed field of that name, or 0 without error if not found.
 */
struct FE_field *FE_region_get_FE_field_from_name(struct FE_region *fe_region,
	const char *field_name);

/**
 * Safely change the name of field in fe_region to new_name, and inform clients.
 *
 * @param fe_region  The owning region of the field.
 * @param field  The field to rename.
 * @param new_name  The new name of the field. Must be unique in FE_region.
 * @return  1 on success, 0 if failed.
 */
int FE_region_set_FE_field_name(struct FE_region *fe_region,
	struct FE_field *field, const char *new_name);

/**
 * @returns  Boolean true if field is in/from fe_region, otherwise false.
 */
bool FE_region_contains_FE_field(struct FE_region *fe_region,
	struct FE_field *field);

/**
 * Returns the first field in <fe_region> that satisfies <conditional_function>
 * with <user_data_void>.
 * A NULL <conditional_function> returns the first FE_field in <fe_region>, if any.
 * @return  Non-accessed field, or 0 if not found.
 */
struct FE_field *FE_region_get_first_FE_field_that(struct FE_region *fe_region,
	LIST_CONDITIONAL_FUNCTION(FE_field) *conditional_function,
	void *user_data_void);

/**
 * Calls <iterator_function> with <user_data> for each FE_field in <region>.
 */
int FE_region_for_each_FE_field(struct FE_region *fe_region,
	LIST_ITERATOR_FUNCTION(FE_field) iterator_function, void *user_data);

/**
 * Returns the number of FE_fields in <fe_region>.
 */
int FE_region_get_number_of_FE_fields(struct FE_region *fe_region);

/***************************************************************************//**
 * Returns an FE_field with the given <name> merged into <fe_region> and with
 * the given properties. If a field of the given <name> already exists, checks
 * that it has the same properties -- if not an error is reported. If no field
 * of that name exists, one is created and FE_region_merge_FE_field called for
 * it. Hence, this function may result in change messages being sent, so use
 * begin/end change if several calls are to be made.
 * Field must be of FE_field_type GENERAL_FE_FIELD.
 * Field properties other than those listed are ignored in comparisons and
 * left at their default values.
 * @return  Accessed FE_field or nullptr if failed.
 */
struct FE_field *FE_region_get_FE_field_with_general_properties(
	struct FE_region *fe_region, const char *name, enum Value_type value_type,
	int number_of_components);

/**
 * Checks <fe_field> is compatible with <fe_region> and any existing FE_field
 * using the same identifier, then merges it into <fe_region>.
 * If no FE_field of the same identifier exists in FE_region, <fe_field> is added
 * to <fe_region> and returned by this function, otherwise changes are merged into
 * the existing FE_field and it is returned.
 * @return  Non-accessed field or 0 if error.
 */
struct FE_field *FE_region_merge_FE_field(struct FE_region *fe_region,
	struct FE_field *fe_field);

/**
 * Returns true if field is defined on any nodes or element in fe_region.
 */
bool FE_region_is_FE_field_in_use(struct FE_region *fe_region,
	struct FE_field *fe_field);

int FE_region_remove_FE_field(struct FE_region *fe_region,
	struct FE_field *fe_field);
/*******************************************************************************
LAST MODIFIED : 3 March 2003

DESCRIPTION :
Removes <fe_field> from <fe_region>.
Fields can only be removed if not defined on any nodes and element in
<fe_region>.
==============================================================================*/

struct Set_FE_field_conditional_FE_region_data
/*******************************************************************************
LAST MODIFIED : 27 February 2003

DESCRIPTION :
User data structure passed to set_FE_field_conditional_FE_region.
==============================================================================*/
{
	LIST_CONDITIONAL_FUNCTION(FE_field) *conditional_function;
	void *user_data;
	struct FE_region *fe_region;
}; /* struct Set_FE_field_conditional_FE_region_data */

/**
 * Returns true if field is defined with multiple times on any node or element.
 */
int FE_region_FE_field_has_multiple_times(struct FE_region *fe_region,
	struct FE_field *fe_field);

/**
 * Returns an <fe_field> which the <fe_region> considers to be its default
 * coordinate field. Generally the first field with the type_coordinate flag
 * set found alphabetically.
 */
struct FE_field *FE_region_get_default_coordinate_FE_field(
	struct FE_region *fe_region);

/**
 * Returns the FE_basis_manager used for bases in this <fe_region>.
 */
struct MANAGER(FE_basis) *FE_region_get_basis_manager(
	struct FE_region *fe_region);

struct LIST(FE_field) *FE_region_get_FE_field_list(struct FE_region *fe_region);

/**
 * Returns the LIST of FE_element_shapes used for elements in this <fe_region>.
 */
struct LIST(FE_element_shape) *FE_region_get_FE_element_shape_list(
	struct FE_region *fe_region);

/***************************************************************************//**
 * Returns the top-level element of the highest dimension with the supplied
 * identifier in fe_region, or NULL without error if none.
 */
struct FE_element *FE_region_get_top_level_FE_element_from_identifier(
	struct FE_region *fe_region, int identifier);

/**
 * @return  Non-accessed FE_nodeset or 0 if not found.
 */
FE_nodeset *FE_region_find_FE_nodeset_by_field_domain_type(
	struct FE_region *fe_region, enum cmzn_field_domain_type domain_type);

/**
 * @return  Non-accessed FE_mesh or 0 if not found.
 */
FE_mesh *FE_region_find_FE_mesh_by_dimension(
	struct FE_region *fe_region, int dimension);

/***************************************************************************//**
 * Returns the number of elements of all dimensions in <fe_region>.
 * Prefer FE_region_get_number_of_FE_elements_of_dimension.
 */
int FE_region_get_number_of_FE_elements_all_dimensions(
	struct FE_region *fe_region);

/***************************************************************************//**
 * Returns the number of elements of given dimensions in <fe_region>.
 */
int FE_region_get_number_of_FE_elements_of_dimension(
	struct FE_region *fe_region, int dimension);

/***************************************************************************//**
 * @return  The highest dimension of element in the region, or zero if none.
 */
int FE_region_get_highest_dimension(struct FE_region *fe_region);

/**
 * Sets up <fe_region> to automatically define faces on any elements merged into
 * it, and their faces recursively.
 * Call FE_region_end_define_faces as soon as face definition is finished.
 * Always put face definition calls between calls to begin_change/end_change.
 */
int FE_region_begin_define_faces(struct FE_region *fe_region);

/**
 * Ends face definition in <fe_region>. Cleans up internal caches.
 */
int FE_region_end_define_faces(struct FE_region *fe_region);

/**
 * Ensures for elements of every dimension > 1 that there are face and line
 * elements of lower dimension in the region.
 * Requires a coordinate field to be defined to work.
 * @return  Result OK on success, WARNING_PART_DONE if failed for some
 * elements due to absent nodes, otherwise any other error.
 */
int FE_region_define_faces(struct FE_region *fe_region);

/**
 * Returns the first element in <fe_region> that satisfies
 * <conditional_function> with <user_data_void>.
 * A NULL <conditional_function> returns the first FE_element in <fe_region>,
 * if any.
 * This version iterates over elements of all dimensions from highest to lowest.
 */
struct FE_element *FE_region_get_first_FE_element_that(
	struct FE_region *fe_region,
	LIST_CONDITIONAL_FUNCTION(FE_element) *conditional_function,
	void *user_data_void);

/**
 * Calls <iterator_function> with <user_data> for each FE_element in <region>
 * of all dimensions from highest to lowest.
 */
int FE_region_for_each_FE_element(struct FE_region *fe_region,
	LIST_ITERATOR_FUNCTION(FE_element) iterator_function, void *user_data);

/**
 * Smooths node-based <fe_field> over its nodes and elements in <fe_region>.
 */
int FE_region_smooth_FE_field(struct FE_region *fe_region,
	struct FE_field *fe_field, FE_value time);

/**
 * Finds or creates a struct FE_time_sequence in <fe_region> with the given
 * <number_of_times> and <times>.
 */
struct FE_time_sequence *FE_region_get_FE_time_sequence_matching_series(
	struct FE_region *fe_region, int number_of_times, const FE_value *times);

/**
 * Finds or creates a struct FE_time_sequence in <fe_region> which has the list
 * of times formed by merging the two time_sequences supplied.
 */
struct FE_time_sequence *FE_region_get_FE_time_sequence_merging_two_time_series(
	struct FE_region *fe_region, struct FE_time_sequence *time_sequence_one,
	struct FE_time_sequence *time_sequence_two);

/**
 * Finds or creates a struct FE_basis in <fe_region> with the given basis_type.
 * @return  Non-accessed basis.
 */
struct FE_basis *FE_region_get_FE_basis_matching_basis_type(
	struct FE_region *fe_region, int *basis_type);

/**
 * Finds or creates a struct FE_basis in <fe_region> for constant interpolation
 * of the given dimension.
 * @return  Non-accessed basis.
 */
struct FE_basis *FE_region_get_constant_FE_basis_of_dimension(
	struct FE_region *fe_region, int dimension);

/**
 * @return  The cmzn_region containing this fe_region. Not accessed.
 */
struct cmzn_region *FE_region_get_cmzn_region(struct FE_region *fe_region);

/**
 * Check that fields and other object definitions in source FE_region are
 * properly defined and compatible with definitions in target FE_region.
 * Converts legacy field representations e.g. read from older EX files, hence
 * source_fe_region can be modified, and function fails if conversion is not
 * possible.
 * @param target_fe_region  Optional target/global FE_region to check
 * compatibility with. Omit to confirm conversion of legacy field
 * representations only.
 * @param source_fe_region  Source region to check. Can be modified.
 * @return  True if compatible and conversions successful, false if failed or
 * source region is missing.
 */
bool FE_region_can_merge(struct FE_region *target_fe_region,
	struct FE_region *source_fe_region);

/**
 * Merges into <target_fe_region> the fields, nodes and elements from
 * <source_fe_region>. Note that <source_fe_region> is left in a polluted state
 * containing objects that partly belong to the <target_fe_region> and partly to
 * itself. Currently it needs to be left around for the remainder of the merge
 * up and down the region graph, but it needs to be destroyed as soon as possible.
 */
int FE_region_merge(struct FE_region *target_fe_region,
	struct FE_region *source_fe_region);

/***************************************************************************//**
 * List statistics about btree structures storing a region's nodes and elements.
 */
void FE_region_list_btree_statistics(struct FE_region *fe_region);

#endif /* !defined (FINITE_ELEMENT_REGION_H) */
