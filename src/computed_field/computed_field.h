/*******************************************************************************
FILE : computed_field.h

LAST MODIFIED : 18 April 2005

DESCRIPTION :
A cmzn_field is an abstraction of an FE_field. For each FE_field there is
a wrapper cmzn_field automatically generated that can be called on to
evaluate the field in an element or node. The interface for evaluating
Computed_fields is much simpler than for FE_field, since they hide details of
evaluation caches, for example. Their main benefit is in
allowing new types of fields to be defined as functions of other fields and
source information, such as scale, offset, magnitude, gradient,
coordinate transformations etc., thus providing cmgui with the ability to
provide customised features without cluttering dialogs such as the graphical
element editor. Also, finite_element_to_graphics_object is greatly simplified
because it can assume all coordinate and other fields are in rectangular
cartesian coordinates - if they are not already, the rest of the program can
make a simple wrapper computed_field effecting this change. A positive
consequence of this change is that this file should in time contain the only
code for invoking coordinate transformations in the entire program.

In addition to these functional benefits, computed fields cache the last
position they were evaluated at and the values and derivatives of the field at
that point so that if values at that point are requested again they can be
immediately returned. This allows functions using a number of fields, some
possibly depending on each other to get the maximum benefit out of values that
have already been calculated, without requiring complex logic for determining
if a value is already known.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (COMPUTED_FIELD_H)
#define COMPUTED_FIELD_H

#include "opencmiss/zinc/types/meshid.h"
#include "opencmiss/zinc/field.h"
#include "finite_element/finite_element.h"
#include "general/geometry.h"
#include "general/list.h"
#include "general/manager.h"
#include "general/object.h"
#include "general/message.h"

struct cmzn_region;

/*
The Public interface to cmzn_field is contained in
zinc/field.h however most of the functions come directly from
this module.  So that these functions match the public declarations the
functions are given their public names.
*/

/* Convert legacy object type name to use external field object type name */
#define Computed_field cmzn_field

enum cmzn_field_type
{
	CMZN_FIELD_TYPE_INVALID = 0,
	/*field alias */
	CMZN_FIELD_TYPE_ALIAS = 1,
	/* field arithmetic operators */
	CMZN_FIELD_TYPE_ADD = 2,
	CMZN_FIELD_TYPE_POWER = 3,
	CMZN_FIELD_TYPE_MULTIPLY = 4,
	CMZN_FIELD_TYPE_DIVIDE = 5,
	CMZN_FIELD_TYPE_SUBTRACT = 6,
	CMZN_FIELD_TYPE_LOG = 7,
	CMZN_FIELD_TYPE_SQRT = 8,
	CMZN_FIELD_TYPE_EXP = 9,
	CMZN_FIELD_TYPE_ABS = 10,
	/* field composite */
	CMZN_FIELD_TYPE_IDENTITY = 11,
	CMZN_FIELD_TYPE_COMPONENT = 12,
	CMZN_FIELD_TYPE_CONCATENATE = 13,
	/*field conditional */
	CMZN_FIELD_TYPE_IF = 14,
	/*field constant */
	CMZN_FIELD_TYPE_CONSTANT = 15,
	CMZN_FIELD_TYPE_STRING_CONSTANT = 16,
	/*field coordinate transformation */
	CMZN_FIELD_TYPE_COORDINATE_TRANSFORMATION = 17,
	CMZN_FIELD_TYPE_VECTOR_COORDINATE_TRANSFORMATION = 18,
	/*field derivatives */
	CMZN_FIELD_TYPE_DERIVATIVE = 19,
	CMZN_FIELD_TYPE_CURL = 20,
	CMZN_FIELD_TYPE_DIVERGENCE = 21,
	CMZN_FIELD_TYPE_GRADIENT = 22,
	/*field fibre */
	CMZN_FIELD_TYPE_FIBRE_AXES = 23,
	/*field finite element*/
	CMZN_FIELD_TYPE_EDGE_DISCONTINUITY = 24,
	CMZN_FIELD_TYPE_EMBEDDED = 25,
	CMZN_FIELD_TYPE_NODE_VALUE = 26,
	CMZN_FIELD_TYPE_STORED_STRING = 27,
	CMZN_FIELD_TYPE_IS_EXTERIOR = 28,
	CMZN_FIELD_TYPE_IS_ON_FACE = 29,
	/*field logical operators*/
	CMZN_FIELD_TYPE_AND = 30,
	CMZN_FIELD_TYPE_EQUAL_TO = 31,
	CMZN_FIELD_TYPE_GREATER_THAN = 32,
	CMZN_FIELD_TYPE_IS_DEFINED = 33,
	CMZN_FIELD_TYPE_LESS_THAN = 34,
	CMZN_FIELD_TYPE_OR = 35,
	CMZN_FIELD_TYPE_NOT = 36,
	CMZN_FIELD_TYPE_XOR = 37,
	/*field matrix operators*/
	CMZN_FIELD_TYPE_DETERMINANT = 38,
	CMZN_FIELD_TYPE_EIGENVALUES = 39,
	CMZN_FIELD_TYPE_EIGENVECTORS = 40,
	CMZN_FIELD_TYPE_MATRIX_INVERT = 41,
	CMZN_FIELD_TYPE_MATRIX_MULTIPLY = 42,
	CMZN_FIELD_TYPE_PROJECTION = 43,
	CMZN_FIELD_TYPE_TRANSPOSE = 44,
	/*field time */
	CMZN_FIELD_TYPE_TIME_LOOKUP = 45,
	/* field trigonometry */
	CMZN_FIELD_TYPE_SIN = 46,
	CMZN_FIELD_TYPE_COS = 47,
	CMZN_FIELD_TYPE_TAN = 48,
	CMZN_FIELD_TYPE_ASIN = 49,
	CMZN_FIELD_TYPE_ACOS = 50,
	CMZN_FIELD_TYPE_ATAN = 51,
	CMZN_FIELD_TYPE_ATAN2 = 52,
	/*field vector operators */
	CMZN_FIELD_TYPE_CROSS_PRODUCT = 53,
	CMZN_FIELD_TYPE_DOT_PRODUCT = 54,
	CMZN_FIELD_TYPE_MAGNITUDE = 55,
	CMZN_FIELD_TYPE_NORMALISE = 56,
	CMZN_FIELD_TYPE_SUM_COMPONENTS = 57,
	/*field finite element */
	CMZN_FIELD_TYPE_FINITE_ELEMENT = 58,
	/*field time */
	CMZN_FIELD_TYPE_TIME_VALUE = 59,
	CMZN_FIELD_TYPE_STORED_MESH_LOCATION = 60,
	CMZN_FIELD_TYPE_FIND_MESH_LOCATION = 61
};


/*
Global types
------------
*/

DECLARE_LIST_TYPES(cmzn_field);

DECLARE_MANAGER_TYPES(cmzn_field);


struct List_Computed_field_commands_data
{
	const char *command_prefix;
	int listed_fields;
	struct LIST(cmzn_field) *computed_field_list;
	struct MANAGER(cmzn_field) *computed_field_manager;
}; /* struct List_Computed_field_commands_data */

struct Computed_field_conditional_data
{
	struct cmzn_field *conditional_field;
	FE_value time;
}; /* struct Computed_field_conditional_data */

/***************************************************************************//**
 * Stores data and settings needed to create field, including the region to add
 * it to.
 */
struct cmzn_fieldmodule;

/*
Global functions
----------------
*/
PROTOTYPE_OBJECT_FUNCTIONS(cmzn_field);
PROTOTYPE_GET_OBJECT_NAME_FUNCTION(cmzn_field);

PROTOTYPE_LIST_FUNCTIONS(cmzn_field);
PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(cmzn_field,name,const char *);

/**
 * Get non-allocated field name. Internal use only, for error messages etc.
 *
 * @param field  Field to get name of.
 * @return  Internal name of field. Use immediately and do not deallocate.
 */
const char *cmzn_field_get_name_internal(cmzn_field_id field);

/**
 * Set a unique name for field, concatenating first and second parts with a unique
 * number if needed. Internal use only.
 *
 * @param field  Field to set name of.
 * @param first  First part of name.
 * @param second  Second part of name.
 * @return  Standard result code.
 */
int cmzn_field_set_name_unique_concatentate(cmzn_field_id field,
	const char *first, const char *second);

/***************************************************************************//**
 * Internal variant of public cmzn_fielditerator_next() which does not access
 * the returned field, for more efficient if less safe usage.
 *
 * @param iterator  Field iterator to query and advance.
 * @return  Non-accessed pointer to the next field, or NULL if none remaining.
 */
cmzn_field_id cmzn_fielditerator_next_non_access(
	cmzn_fielditerator_id iterator);

PROTOTYPE_MANAGER_COPY_FUNCTIONS(cmzn_field,name,const char *);
PROTOTYPE_MANAGER_FUNCTIONS(cmzn_field);
PROTOTYPE_MANAGER_IDENTIFIER_WITHOUT_MODIFY_FUNCTIONS(cmzn_field,name,const char *);

/***************************************************************************//**
 * Returns true if <field> can be calculated in <element>. If the field depends
 * on any other fields, this function is recursively called for them.
 * Warning: inefficient as field cache is created and destroyed with each call.
 * For new code, prefer managing your own field cache and directly using
 * cmzn_field_id_defined_at_location.
 */
int Computed_field_is_defined_in_element(struct cmzn_field *field,
	struct FE_element *element);

/***************************************************************************//**
 * List iterator version of Computed_field_is_defined_in_element.
 * Returns true if <field> can be calculated in <element>.
 * Warning: inefficient as field cache is created and destroyed with each call.
 * For new code, prefer managing your own field cache and directly using
 * cmzn_field_id_defined_at_location.
 */
int Computed_field_is_defined_in_element_conditional(
	struct cmzn_field *field,void *element_void);

/***************************************************************************//**
 * Returns true if <field> can be calculated in <node>. If the field depends
 * on any other fields, this function is recursively called for them.
 * Warning: inefficient as field cache is created and destroyed with each call.
 * For new code, prefer managing your own field cache and directly using
 * cmzn_field_id_defined_at_location.
 */
int Computed_field_is_defined_at_node(struct cmzn_field *field,
	struct FE_node *node);

/***************************************************************************//**
 * List iterator version of Computed_field_is_defined_at_node.
 * Returns true if <field> can be calculated in <element>.
 * Warning: inefficient as field cache is created and destroyed with each call.
 * For new code, prefer managing your own field cache and directly using
 * cmzn_field_id_defined_at_location.
 */
int Computed_field_is_defined_at_node_conditional(struct cmzn_field *field,
	void *node_void);

/***************************************************************************//**
 * List conditional function returning true if field value type is
 * CMZN_FIELD_VALUE_TYPE_STRING.
 */
int Computed_field_has_string_value_type(struct cmzn_field *field,
	void *dummy_void);

int Computed_field_for_each_ancestor(struct cmzn_field *field,
	LIST_ITERATOR_FUNCTION(cmzn_field) *iterator_function, void *user_data);
/*******************************************************************************
LAST MODIFIED : 2 April 2003

DESCRIPTION :
For <field> and all of its source Computed_fields, calls <iterator_function>
with <user_data>. Iteration stops if a single iterator_function call returns 0.
==============================================================================*/

/***************************************************************************//**
 * Prescribes a location in an element for field evaluation or assignment with
 * the cache. Variant with optional prescribed parent element.
 * Note: replaces any other spatial location in cache (e.g. node.) but time
 * is unchanged.
 *
 * @param cache  The field cache to set the location in.
 * @param element  The element the location is in. Must belong to same region
 * as cache.
 * @param number_of_chart_coordinates  The size of the chart_coordinates array,
 * checked to be not less than the element dimension.
 * @param chart_coordinates  Location in element chart. Value is not checked;
 * caller is responsible for supplying locations within the bounds of the
 * element shape.
 * @param top_level_element  Optional highest dimensional parent element to
 * inherit fields from.
 * @return  1 on success, 0 on failure.
 */
int cmzn_fieldcache_set_mesh_location_with_parent(
	cmzn_fieldcache_id cache, cmzn_element_id element,
	int number_of_chart_coordinates, const double *chart_coordinates,
	cmzn_element_id top_level_element);

/***************************************************************************//**
 * Internal function which if set means subsequent values are assigned into the
 * cache only, not into field values. Subsequent evaluations return this and
 * other fields as if the values had been set in the field.
 * Remember to clear flag after experimental evaluation with assigned values.
 * Warning: only a single assign works with the current code.
 * Changing the cache location clears these temporary cached values.
 *
 * @param cache  Store of location to assign at and intermediate field values.
 * @param assign_to_cache  The new state of the assign-to-cache-only flag.
 * @return  1 on success, 0 on failure.
 */
int cmzn_fieldcache_set_assign_in_cache(cmzn_fieldcache_id cache, int assign_in_cache);

/**
 * Internal function.
 * Note: no warnings if not evaluated so can be used for is_defined
 *
 * @return  true if any field component value is non zero with a small tolerance,
 * otherwise false.
 */
bool cmzn_field_evaluate_boolean(cmzn_field_id field,
	cmzn_fieldcache_id cache);

/***************************************************************************//**
 * Temporary function to allow first derivatives to be evaluated with value.
 * @param number_of_derivatives  Expected number of derivatives for domain.
 * @param derivatives  Array of size number_of_values*number_of_derivatives.
 * IMPORTANT: Not approved for external API!
 * @deprecated
 * Try to remove its use as soon as possible.
 */
int cmzn_field_evaluate_real_with_derivatives(cmzn_field_id field,
	cmzn_fieldcache_id cache, int number_of_values, double *values,
	int number_of_derivatives, double *derivatives);

int Computed_field_get_native_discretization_in_element(
	struct cmzn_field *field,struct FE_element *element,int *number_in_xi);
/*******************************************************************************
LAST MODIFIED : 28 October 1999

DESCRIPTION :
If the <field> or its source field is grid-based in <element>, returns in
<number_in_xi> the numbers of finite difference cells in each xi-direction of
<element>. Note that this number is one less than the number of grid points in
each direction. <number_in_xi> should be allocated with at least as much space
as the number of dimensions in <element>, but is assumed to have no more than
MAXIMUM_ELEMENT_XI_DIMENSIONS so that
int number_in_xi[MAXIMUM_ELEMENT_XI_DIMENSIONS] can be passed to this function.
Returns 0 with no errors if the field is not grid-based.
==============================================================================*/

int Computed_field_get_native_resolution(struct cmzn_field *field,
		int *dimension, int **sizes,
	struct cmzn_field **texture_coordinate_field);
/*******************************************************************************
LAST MODIFIED : 03 February 2005

DESCRIPTION :
Gets the <dimension>, <sizes>, <minimums>, <maximums> and <texture_coordinate_field> from
the <field>. These parameters will be used in image processing.

==============================================================================*/

struct Coordinate_system *Computed_field_get_coordinate_system(
	struct cmzn_field *field);
/*******************************************************************************
LAST MODIFIED : 19 January 1999

DESCRIPTION :
Returns the coordinate system <field> is to be interpreted under. See function
Computed_field_set_coordinate_system for further details.
==============================================================================*/

int Computed_field_set_coordinate_system(struct cmzn_field *field,
	const Coordinate_system *coordinate_system);
/*******************************************************************************
LAST MODIFIED : 19 January 1999

DESCRIPTION :
Sets the coordinate system <field> is to be interpreted under. Note the careful
choice of words here: the coordinate system merely tells the rest of the program
what needs to be done to transform the field values into any other coordinate
system. It does not have to be "correct" for the values in the field, eg. you
can describe prolate spheroidal values as RC to "open out" the heart model.
<focus> must be given with prolate and oblate spheroidal coordinate systems.
==============================================================================*/

const char *Computed_field_get_type_string(struct cmzn_field *field);
/*******************************************************************************
LAST MODIFIED : 24 January 2002

DESCRIPTION :
Returns the string which identifies the type.
The calling function must not deallocate the returned string.
==============================================================================*/

int Computed_field_has_3_components(struct cmzn_field *field,
	void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 10 March 1999

DESCRIPTION :
Conditional function returning true if <field> has exactly three components.
==============================================================================*/

int Computed_field_has_4_components(struct cmzn_field *field,
	void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 10 March 1999

DESCRIPTION :
Conditional function returning true if <field> has exactly four components.
==============================================================================*/

int Computed_field_has_16_components(struct cmzn_field *field,
	void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 21 February 2008

DESCRIPTION :
Iterator/conditional function returning true if <field> has exactly sixteen
components - useful for selecting transformation matrix.
==============================================================================*/

int Computed_field_has_up_to_4_numerical_components(
	struct cmzn_field *field,void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 23 May 2000

DESCRIPTION :
Conditional function returning true if <field> has up to 4 components and they
are numerical - useful for selecting vector/coordinate fields.
==============================================================================*/

int Computed_field_has_n_components(struct cmzn_field *field,
	void *components_ptr_void);
/*******************************************************************************
LAST MODIFIED : 11 July 2000

DESCRIPTION :
Iterator/conditional function returning true if <field> has the same number of
components as that specified by <components_ptr_void>.
==============================================================================*/

int Computed_field_has_at_least_2_components(struct cmzn_field *field,
	void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 16 March 1999

DESCRIPTION :
Iterator/conditional function returning true if <field> has at least 2 components.
==============================================================================*/

/*******************************************************************************
 * @return  1 if field returns mesh_location value type.
 */
int Computed_field_has_value_type_mesh_location(struct cmzn_field *field,
	void *dummy_void);

int Computed_field_has_numerical_components(struct cmzn_field *field,
	void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 23 May 2000

DESCRIPTION :
Conditional function returning true if <field> returns numerical components.
Note that whether the numbers were integer, FE_value or double, they may be
returned as FE_value when evaluated.
==============================================================================*/

int Computed_field_is_scalar(struct cmzn_field *field,void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 23 May 2000

DESCRIPTION :
Conditional function returning true if <field> has 1 component and it is
numerical.
==============================================================================*/

int Computed_field_has_up_to_3_numerical_components(
	struct cmzn_field *field,void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 23 May 2000

DESCRIPTION :
Conditional function returning true if <field> has up to 3 components and they
are numerical - useful for selecting vector/coordinate fields.
==============================================================================*/

int Computed_field_has_up_to_4_numerical_components(
	struct cmzn_field *field,void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 23 May 2000

DESCRIPTION :
Conditional function returning true if <field> has up to 4 components and they
are numerical - useful for selecting vector/coordinate fields.
==============================================================================*/

int Computed_field_has_multiple_times(struct cmzn_field *field);
/*******************************************************************************
LAST MODIFIED : 22 November 2001

DESCRIPTION :
Conditional function returning true if <field> depends on time.
==============================================================================*/

int Computed_field_is_orientation_scale_capable(struct cmzn_field *field,
	void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 12 February 1999

DESCRIPTION :
Conditional function returning true if the field can be used to orient or scale
glyphs. Generally, this means it has 1,2,3,4,6 or 9 components, where:
1 = scalar (no vector, isotropic scaling).
2 = 1 2-D vector (2nd axis is normal in plane, 3rd is out of 2-D plane);
3 = 1 3-D vector (orthogonal 2nd and 3rd axes are arbitrarily chosen);
4 = 2 2-D vectors (3rd axis taken as out of 2-D plane);
6 = 2 3-D vectors (3rd axis found from cross product);
9 = 3 3-D vectors = complete definition of 3 axes.
???RC.  Include coordinate system in test?
==============================================================================*/

int Computed_field_is_stream_vector_capable(struct cmzn_field *field,
	void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 15 March 1999

DESCRIPTION :
Conditional function returning true if the field is suitable for 3-D streamline
tracking. This means it has either 3, 6 or 9 components (with 3 components per
vector), or has a FIBRE coordinate_system, meaning it can be wrapped to produce
9-component fibre_axes.  Also now supports 2 components for use with a 2
component coordinate field.
The number of components controls how the field is interpreted:
3 = 1 3-D vector (lateral direction and normal worked out from curl of field);
6 = 2 3-D vectors (2nd vector is lateral direction. Stream ribbon normal found
	from cross product);
9 = 3 3-D vectors (2nd vector is lateral direction; 3rd vector is stream ribbon
	normal).
==============================================================================*/

/***************************************************************************//**
 * Find location in mesh or element where the field has same or nearest value to
 * the prescribed values.
 *
 * @param field  The field whose values need to match.
 * @param field_cache  Field cache to perform evaluations to find mesh location.
 * Set time in the field cache to search for field values at particular times.
 * @param values  Array of values to match or get nearest to. Implementation
 * promises to copy this, hence can pass a pointer to field cache values.
 * @param number_of_values  The size of the values array, must equal the number
 * of components of field.
 * @param element_address  Address to return element in. If mesh is omitted,
 * must point at a single element to search.
 * @param xi  Array of same dimension as mesh or element to return chart
 * coordinates in.
 * @param mesh  The mesh to search over. Can be omitted if element specified.
 * @param propagate_to_source  If this is set, find_nearest is not set, and
 * field implements a propagate_find_element_xi function this is called to undo
 * its field calculation and resume the search on its source field. This can
 * result in less computation, but can fail if the source field is multi-valued,
 * a common case being when it is in a polar coordinate system since valid
 * values may be a multiple of 2*PI out.
 * @param find_nearest  Set to 1 to find location of nearest field value, or 0
 * to find exact match.
 * @return  1 if search carried out without error including when no element is
 * found, or 0 if failed.
 */
int Computed_field_find_element_xi(struct cmzn_field *field,
	cmzn_fieldcache_id field_cache, const FE_value *values,
	int number_of_values, struct FE_element **element_address, FE_value *xi,
	cmzn_mesh_id mesh, int propagate_to_source, int find_nearest);

int Computed_field_is_find_element_xi_capable(struct cmzn_field *field,
	void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 16 June 2000

DESCRIPTION :
This function returns true if the <field> can find element and xi given
a set of values.
==============================================================================*/

/* Returns true if all fields are defined in the same way at the two elements. */
bool equivalent_computed_fields_at_elements(struct FE_element *element_1,
	struct FE_element *element_2);

int equivalent_computed_fields_at_nodes(struct FE_node *node_1,
	struct FE_node *node_2);
/*******************************************************************************
LAST MODIFIED : 23 May 2000

DESCRIPTION :
Returns true if all fields are defined in the same way at the two nodes.
==============================================================================*/

int list_Computed_field(struct cmzn_field *field,void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 25 January 1999

DESCRIPTION :
Writes the properties of the <field> to the command window.
==============================================================================*/

int write_Computed_field_commands_to_comfile(struct cmzn_field *field,
	 void *command_prefix_void);
/*******************************************************************************
LAST MODIFIED : 10 August 2007

DESCRIPTION :
Writes the commands needed to reproduce <field> to the com file.
==============================================================================*/

int write_Computed_field_commands_if_managed_source_fields_in_list_to_comfile(
	 struct cmzn_field *field, void *list_commands_data_void);
/*******************************************************************************
LAST MODIFIED : 10 August 2007

DESCRIPTION :
Calls list_Computed_field_commands if the field is not already in the list,
has no source fields, or all its source fields are either not managed or
already in the list. If the field is listed, it is added to the list.
Ensures field command list comes out in the order they need to be created.
Note, must be cycled through as many times as it takes till listed_fields -> 0.
Second argument is a struct List_Computed_field_commands_data.
==============================================================================*/

int list_Computed_field_name(struct cmzn_field *field,void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 4 February 1999

DESCRIPTION :
Lists a single line about a computed field including just name, number of
components, coordinate system and type.
==============================================================================*/

int Computed_field_can_be_destroyed(struct cmzn_field *field);
/*******************************************************************************
LAST MODIFIED : 10 May 2000

DESCRIPTION :
Returns true if the <field> is only accessed once - assumed by its manager. If
it is of type COMPUTED_FIELD_FINITE_ELEMENT further tests that its fe_field can
be destroyed, assuming it is only accessed by this field and its manager.
==============================================================================*/

/***************************************************************************//**
 * Returns the region which this field belongs to, if any.
 *
 * @param field  The field.
 * @return  A handle to the owning region if in one, or NULL if none.
 */
struct cmzn_region *Computed_field_get_region(struct cmzn_field *field);

/**
 * Returns true if the field is a finite element coordinate type field and
 * composed of three or less components, false otherwise.
 *
 * @param field the field to determine whether or not it is a coordinate field
 * @return 1 if the field is a coordinate field, 0 otherwise
 */
int Computed_field_is_coordinate_field(struct cmzn_field *field, void *not_in_use);

/*******************************************************************************
 * Returns the domains of the given field by recursively searching through the
 * field's source fields
 *
 * @param field  The field to find the domain of
 * @param domain_field_list  A handle to the list of domains for the field
 * @return  1 on success, 0 otherwise
 */
int Computed_field_get_domain( struct cmzn_field *field,
	struct LIST(cmzn_field) *domain_field_list );

/*******************************************************************************
 * Returns true if the field value varies non-linearly over its domain.
 * If applicable, e.g. for coordinate fields, callers should first determine
 * whether the coordinate system varies non-linearly with screen space since
 * that is a less expensive query.
 * Note that an image-based field is linear if it is interpolated linearly.
 *
 * @see Coordinate_system_type_is_non_linear
 * @param field  The field to query.
 * @return  1 if non-linear, 0 if linear.
 */
int Computed_field_is_non_linear(struct cmzn_field *field);

/**
 * @return  Allocated field name unused by any other field in field module.
 */
char *cmzn_fieldmodule_get_unique_field_name(
	struct cmzn_fieldmodule *fieldmodule);

/***************************************************************************//**
 * Returns true if field is not a source field of other.
 *
 * @param field  The field.
 * @return  1 if field is not a source field of others otherwise 0.
 */
int Computed_field_is_not_source_field_of_others(struct cmzn_field *field);

#if defined (DEBUG_CODE)
/**
 * Returns the access count for the supplied field.  This function is only
 * available when DEBUG_CODE is defined.
 *
 * @param field the field to query the access count from
 * @return the access count
 */
int cmzn_field_get_access_count(cmzn_field_id field);
#endif /* defined (DEBUG_CODE) */

enum cmzn_field_type cmzn_field_type_enum_from_string(const char *string);

char *cmzn_field_type_enum_to_string(enum cmzn_field_type type);

enum cmzn_field_type cmzn_field_type_enum_from_class_name(const char *string);

char *cmzn_field_type_enum_to_class_name(enum cmzn_field_type type);

enum cmzn_field_type cmzn_field_get_type(cmzn_field_id field);

#endif /* !defined (COMPUTED_FIELD_H) */
