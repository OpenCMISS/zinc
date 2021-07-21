/**
 * FILE : finite_element.h
 *
 * Internal header for finite element data structures and functions.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (FINITE_ELEMENT_H)
#define FINITE_ELEMENT_H

#include "opencmiss/zinc/types/elementid.h"
#include "opencmiss/zinc/types/fieldid.h"
#include "opencmiss/zinc/types/nodeid.h"
#include "finite_element/finite_element_basis.hpp"
#include "finite_element/finite_element_field.hpp"
#include "finite_element/finite_element_nodeset.hpp"
#include "finite_element/finite_element_shape.hpp"
#include "finite_element/finite_element_time.h"
#include "datastore/labels.hpp"
#include "general/enumerator.h"
#include "general/geometry.h"
#include "general/list.h"
#include "general/manager.h"
#include "general/object.h"
#include "general/value.h"

/*
Global types
------------
*/

class DsLabelsGroup;
class DsLabelsChangeLog;

class FE_node_field_template;

struct cmzn_element;
#define FE_element cmzn_element
/*******************************************************************************
LAST MODIFIED : 9 October 2002

DESCRIPTION :
A region in space with functions defined on the region.  The region is
parameterized and the functions are known in terms of the parameterized
variables.
==============================================================================*/

PROTOTYPE_ENUMERATOR_FUNCTIONS(cmzn_element_face_type);

PROTOTYPE_ENUMERATOR_FUNCTIONS(cmzn_element_point_sampling_mode);

PROTOTYPE_ENUMERATOR_FUNCTIONS(cmzn_element_quadrature_rule);

typedef int (FE_node_field_iterator_function)(struct FE_node *node, \
	struct FE_field *field,void *user_data);

typedef int (FE_element_field_iterator_function)(struct FE_element *element, \
	struct FE_field *field,void *user_data);

struct FE_field_order_info;
/*******************************************************************************
LAST MODIFIED : 4 September 2001

DESCRIPTION :
Stores a list of fields in the order they are added.
The contents of this object are private.
==============================================================================*/

PROTOTYPE_ENUMERATOR_FUNCTIONS(cmzn_field_domain_type);

/*
Global functions
----------------
*/

/** Define a field at a node. Does not assign values.
  * @param componentTemplates  Pointer to array of node field templates. These
  * are copied for internal use and valuesOffset set appropriately.
  * @param timeSequence  Optional time sequence for time-varying parameters. */
int define_FE_field_at_node(cmzn_node *node, FE_field *field,
	const FE_node_field_template *componentTemplatesIn,
	struct FE_time_sequence *time_sequence);

/**
 * Removes definition of <field> at <node>. If field is of type GENERAL_FE_FIELD
 * then removes values storage for it and shifts values storage for all subsequent
 * fields down.
 * Note: Must ensure that the node field is not in-use by any elements before it
 * is undefined!
 * @return  Result code OK if undefined CMZN_NOT_FOUND if field undefined or
 * any other error if failed.
 */
int undefine_FE_field_at_node(struct FE_node *node,struct FE_field *field);

int for_FE_field_at_node(struct FE_field *field,
	FE_node_field_iterator_function *iterator,void *user_data,
	struct FE_node *node);
/*******************************************************************************
LAST MODIFIED : 30 October 1998

DESCRIPTION :
If an <iterator> is supplied and the <field> is defined at the <node> then the
result of the <iterator> is returned.  Otherwise, if an <iterator> is not
supplied and the <field> is defined at the <node> then a non-zero is returned.
Otherwise, zero is returned.
???DB.  Multiple behaviour dangerous ?
==============================================================================*/

int for_each_FE_field_at_node(FE_node_field_iterator_function *iterator,
	void *user_data,struct FE_node *node);
/*******************************************************************************
LAST MODIFIED : 30 October 1998

DESCRIPTION :
Calls the <iterator> for each field defined at the <node> until the <iterator>
returns 0 or it runs out of fields.  Returns the result of the last <iterator>
called.
==============================================================================*/

/***************************************************************************//**
 * Calls the <iterator> for each field defined at the <node> in alphabetical
 * order and with indexer fields prioritised so they are defined before the
 * fields indexed by them. Runs until the <iterator> returns 0 or it runs out
 * of fields.  Returns the result of the last <iterator>.
 * Note: expensive
 */
int for_each_FE_field_at_node_alphabetical_indexer_priority(
	FE_node_field_iterator_function *iterator,void *user_data,
	struct FE_node *node);

int FE_node_has_FE_field_values(struct FE_node *node);
/*******************************************************************************
LAST MODIFIED : 24 September 1999

DESCRIPTION :
Returns true if any single field defined at <node> has values stored with
the field.
==============================================================================*/

/**
 * Get the number of elements referencing the supplied node
 */
int FE_node_get_element_usage_count(struct FE_node *node);

/**
 * Returns the FE_nodeset that <node> belongs to.
 */
FE_nodeset *FE_node_get_FE_nodeset(struct FE_node *node);

int FE_node_to_node_string(struct FE_node *node, char **string_address);
/*****************************************************************************
LAST MODIFIED : 20 March 2003

DESCRIPTION :
Returns an allocated <string> of the identifier of <node>.
============================================================================*/

int equivalent_FE_field_at_nodes(struct FE_field *field,struct FE_node *node_1,
	struct FE_node *node_2);
/*******************************************************************************
LAST MODIFIED : 30 October 1998

DESCRIPTION :
Returns true if the <field> is defined in the same way at the two nodes.
==============================================================================*/

int equivalent_FE_fields_at_nodes(struct FE_node *node_1,
	struct FE_node *node_2);
/*******************************************************************************
LAST MODIFIED : 23 May 2000

DESCRIPTION :
Returns true if all fields are defined in the same way at the two nodes.
==============================================================================*/

/** Get node field parameters for valueLabel, version at time.
  * If valueLabel is VALUE all components must be defined, otherwise at least
  * one component, and undefined components are returned with value 0.
  * @param componentNumber  Component number starting at 0, or negative to get
  * all components. 
  * @return  Result OK on full success, WARNING_PART_DONE if only some components
  * have parameters, ERROR_NOT_FOUND if field not defined or none of the requested
  * components have parameters, otherwise any other error code. */
#define PROTOTYPE_GET_FE_NODAL_VALUE_FUNCTION( value_typename, value_enum ) \
int get_FE_nodal_ ## value_typename ## _value(cmzn_node *node, FE_field *field, \
	int componentNumber, cmzn_node_value_label valueLabel, int version, \
	FE_value time, value_typename *valuesOut);

/** Set node field parameters for valueLabel, version at time.
  * If valueLabel is VALUE all components must be defined, otherwise at least
  * one component, and undefined components are returned with value 0.
  * @param componentNumber  Field component number starting at 0, or negative to
  * set all component values.
  * @return  Result OK on full success, WARNING_PART_DONE if only some components
  * have parameters (and which were set), ERROR_NOT_FOUND if field not defined
  * or none of the requested components have parameters, otherwise any other error code. */
#define PROTOTYPE_SET_FE_NODAL_VALUE_FUNCTION( value_typename, value_enum ) \
int set_FE_nodal_ ## value_typename ## _value(cmzn_node *node,  FE_field *field, \
	int componentNumber, cmzn_node_value_label valueLabel, int version, \
	FE_value time, const value_typename *valuesIn);

/** Get address of node field parameter for valueLabel, version at time.
  * @param componentNumber  Field component number starting at 0.
  * @param valueAddress  On success, address of node parameter. Will be invalid
  * if the node is modified so it should only be used temporarily.
  * @return  1 on success, otherwise 0. */
#define PROTOTYPE_GET_FE_NODAL_VALUE_STORAGE_FUNCTION( value_typename, value_enum ) \
int get_FE_nodal_ ## value_typename ## _storage(cmzn_node *node, FE_field *field, \
	int componentNumber, cmzn_node_value_label valueLabel, int version, \
	FE_value time, value_typename **valueAddress);

#define PROTOTYPE_FE_NODAL_VALUE_FUNCTIONS( value_type , value_enum ) \
PROTOTYPE_GET_FE_NODAL_VALUE_FUNCTION(value_type,value_enum) \
PROTOTYPE_SET_FE_NODAL_VALUE_FUNCTION(value_type,value_enum) \
PROTOTYPE_GET_FE_NODAL_VALUE_STORAGE_FUNCTION(value_type,value_enum)

PROTOTYPE_FE_NODAL_VALUE_FUNCTIONS(FE_value, FE_VALUE_VALUE)
PROTOTYPE_FE_NODAL_VALUE_FUNCTIONS(double, DOUBLE_VALUE)
PROTOTYPE_FE_NODAL_VALUE_FUNCTIONS(float, FLOAT_VALUE)
PROTOTYPE_FE_NODAL_VALUE_FUNCTIONS(int, INT_VALUE)
PROTOTYPE_FE_NODAL_VALUE_FUNCTIONS(short, SHORT_VALUE)

/**
 * Gets an element:xi value stored at a node.
 * Note doesn't support value labels and versions.
 */
int get_FE_nodal_element_xi_value(struct FE_node *node,
	FE_field *field, int component_number,
	cmzn_element **element, FE_value *xi);

/**
 * Sets a particular element_xi_value (<version>, <type>) for the field
 * <component> at the <node>.
 * Important: only call this for global node, element, field all from the same
 * region. Requires careful use if called during external node merge.
 * If not followed, reverse maps from element to nodes are messed up.
 */
int set_FE_nodal_element_xi_value(struct FE_node *node,
	FE_field *field, int component_number,
	cmzn_element *element, const FE_value *xi);

/** Returns a copy of the string for field component at the <node. 
  * Up to the calling function to DEALLOCATE the returned string.
  * @param stringOut  On success, set to allocated string value, or 0 if no
  * string. */
int get_FE_nodal_string_value(struct FE_node *node,
	FE_field *field, int componentNumber, char **stringOut);

/** Copies and sets the string value of field component at node.
  * @param stringIn  May be NULL. */
int set_FE_nodal_string_value(struct FE_node *node,
	FE_field *field, int componentNumber, const char *stringIn);

/** Get as a string the node field component with valueLabel, version at time.
  * @return  Allocated string, or 0 if faile. */
char *get_FE_nodal_value_as_string(struct FE_node *node, FE_field *field,
	int componentNumber, cmzn_node_value_label valueLabel, int version,
	FE_value time);

int FE_node_is_in_Multi_range(struct FE_node *node,void *multi_range_void);
/*******************************************************************************
LAST MODIFIED : 4 July 2000

DESCRIPTION :
Conditional function returning true if <node> identifier is in the
<multi_range>.
==============================================================================*/

int FE_node_is_not_in_Multi_range(struct FE_node *node,void *multi_range_void);
/*******************************************************************************
LAST MODIFIED : 4 July 2000

DESCRIPTION :
Conditional function returning true if <node> identifier is NOT in the
<multi_range>.
==============================================================================*/

int add_FE_node_number_to_Multi_range(struct FE_node *node,
	void *multi_range_void);
/*******************************************************************************
LAST MODIFIED : 20 February 2000

DESCRIPTION :
Iterator function for adding the number of <node> to <multi_range>.
==============================================================================*/

/**
 * Clears any embedded locations from nodes in nodeset for fields in
 * field_list. This is to avoid circular dependencies which prevent clean-up.
 * Note to handle merging from a separate region where both the source and
 * target region reference the same nodes, only embedded locations in nodes
 * belonging to the supplied nodeset are cleared.
 * @param hostMesh  Optional host mesh to limit to.
 */
int FE_nodeset_clear_embedded_locations(FE_nodeset *nodeset,
	struct LIST(FE_field) *field_list, FE_mesh *hostMesh = nullptr);

/**
 * @return  true if definition of fields in source node are compatible with
 * target node.
 */
bool FE_node_can_merge(struct FE_node *targetNode, struct FE_node *sourceNode);

/**
 * Get all parameters for field component at node and time.
 * Parameter order cycles slowest for derivative / value label, with versions
 * for a given value label consecutive.
 *
 * @param node  The node which stores the field values.
 * @param field  The fields whose values are to be returned from.
 * @param componentNumber  The component number starting at 0.
 * @param time  The time to get values at. If node field is not time-varying,
 * this argument is ignored. If time varying, parameters are interpolated to
 * in-between times, or if out of range the parameters at minimum or maximum
 * time are returned.
 * @param valuesCount  The size of the values array. Must be at least as big
 * as number of values for the component.
 * @param valuesOut  Array to receive node field values.
 * @return  Result OK on success, any other value on failure.
 */
int cmzn_node_get_field_component_FE_value_values(cmzn_node *node,
	FE_field *field, int componentNumber, FE_value time, int valuesCount,
	FE_value *valuesOut);

/**
 * Set all parameters for field component at node and time.
 * Parameter order cycles slowest for derivative / value label, with versions
 * for a given value label consecutive.
 *
 * @param node  The node which stores the field values.
 * @param field  The fields whose values are to be set.
 * @param componentNumber  The component number starting at 0.
 * @param time  The time to set values at, ignored for non-time-varying field.
 * For a time-varying field, must exactly equal a time at which parameters are
 * stored.
 * @param valuesCount  The size of the values array. Must match number of
 * values for the component.
 * @param valuesIn  The array of values to set.
 * @return  Result OK on success, any other value on failure.
 */
int cmzn_node_set_field_component_FE_value_values(cmzn_node *node,
	FE_field *field, int componentNumber, FE_value time, int valuesCount,
	const FE_value *valuesIn);

/**
 * Add increment to all parameters for field component at node and time.
 * Parameter order cycles slowest for derivative / value label, with versions
 * for a given value label consecutive.
 *
 * @param node  The node which stores the field values.
 * @param field  The fields whose values are to be set.
 * @param componentNumber  The component number starting at 0.
 * @param time  The time to set values at, ignored for non-time-varying field.
 * For a time-varying field, must exactly equal a time at which parameters are
 * stored.
 * @param valuesCount  The size of the values array. Must match number of
 * values for the component.
 * @param valuesIn  The array of values to add.
 * @return  Result OK on success, any other value on failure.
 */
int cmzn_node_add_field_component_FE_value_values(cmzn_node *node,
	FE_field *field, int componentNumber, FE_value time, int valuesCount,
	const FE_value *valuesIn);

/**
 * Get all parameters for field component at node and time. Integer variant.
 * Parameter order cycles slowest for derivative / value label, with versions
 * for a given value label consecutive.
 *
 * @param node  The node which stores the field values.
 * @param field  The fields whose values are to be returned from.
 * @param componentNumber  The component number starting at 0.
 * @param time  The time to get values at. If node field is not time-varying,
 * this argument is ignored. If time varying, parameters are interpolated to
 * in-between times, or if out of range the parameters at minimum or maximum
 * time are returned.
 * @param valuesCount  The size of the values array. Must be at least as big
 * as number of values for the component.
 * @param valuesOut  Array to receive node field values.
 * @return  Result OK on success, any other value on failure.
 */
int cmzn_node_get_field_component_int_values(cmzn_node *node,
	FE_field *field, int componentNumber, FE_value time, int valuesCount,
	int *valuesOut);

/**
 * Set all parameters for field component at node and time. Integer variant.
 * Parameter order cycles slowest for derivative / value label, with versions
 * for a given value label consecutive.
 *
 * @param node  The node which stores the field values.
 * @param field  The fields whose values are to be set.
 * @param componentNumber  The component number starting at 0.
 * @param time  The time to set values at, ignored for non-time-varying field.
 * For a time-varying field, must exactly equal a time at which parameters are
 * stored.
 * @param valuesCount  The size of the values array. Must match number of
 * values for the component.
 * @param valuesIn  The array of values to set.
 * @return  Result OK on success, any other value on failure.
 */
int cmzn_node_set_field_component_int_values(cmzn_node *node,
	FE_field *field, int componentNumber, FE_value time, int valuesCount,
	const int *valuesIn);

/**
 * Assigns all parameters for the field at the node, taken from sparse arrays.
 * Works around current limitations of node fields (that they must have VALUE
 * parameters for all components, and the same number of versions for all
 * values/derivatives in a component). Values not found in the sparse 
 * Note this is not time-aware yet.
 * @param field  The field whose parameters are to be assigned.
 * @param node  The node at which the parameters are stored.
 * @param arraySize  The full size of the values and valueExists arrays, equal
 * to componentsSize*derivativesSize*versionsSize
 * @param values  The parameters to set.
 * @param valueExists  Sparsity array; 1 if value set, 0 if not.
 * @param valuesCount  The number of valid parameters in the sparse array.
 * @param componentsSize  Number of components.
 * @param componentsOffset  Offset in array to next component for same
 * derivative and version.
 * @param derivativesSize  Number of derivatives. Derivatives are indexed in
 * order of cmzn_node_value_label, index 0 = VALUE.
 * @param derivativesOffset  Offset in array to next derivative for same
 * component and version.
 * @param versionsSize  Number of versions.
 * @param versionsOffset  Offset in array to next version for same
 * component and derivative.
 */
int FE_field_assign_node_parameters_sparse_FE_value(FE_field *field, FE_node *node,
	int arraySize, const FE_value *values, const int *valueExists, int valuesCount,
	int componentsSize, int componentsOffset,
	int derivativesSize, int derivativesOffset,
	int versionsSize, int versionsOffset);

int get_FE_node_number_of_values(struct FE_node *node);
/*******************************************************************************
LAST MODIFIED : 29 October 1998

DESCRIPTION :
Returns the number of values stored at the <node>.
==============================================================================*/

/**
 * @return  The non-negative node identifier, otherwise
 * DS_LABEL_IDENTIFIER_INVALID on error or if node is orphaned from nodeset.
 */
int get_FE_node_identifier(struct FE_node *node);

/***************************************************************************//**
 * Writes to the console the node identifier and details of the fields and
 * parameters defined there.
 */
int list_FE_node(struct FE_node *node);

/***************************************************************************//**
 * Internal variant of public cmzn_nodeiterator_next() which does not access
 * the returned node, for more efficient if less safe usage.
 *
 * @param node_iterator  Node iterator to query and advance.
 * @return  Non-accessed pointer to the next node, or NULL if none remaining.
 */
cmzn_node_id cmzn_nodeiterator_next_non_access(
	cmzn_nodeiterator_id node_iterator);

int calculate_grid_field_offsets(int element_dimension,
	int top_level_element_dimension, const int *top_level_number_in_xi,
	FE_value *element_to_top_level,int *number_in_xi,int *base_grid_offset,
	int *grid_offset_in_xi);
/*******************************************************************************
LAST MODIFIED : 13 June 2000

DESCRIPTION :
Calculates the factors for converting a grid position on a element of
<element_dimension> to a top_level_element of <top_level_element_dimension>
with <top_level_number_in_xi>, given affine transformation
<element_to_top_level> which has as many rows as <top_level_element_dimension>
and 1 more column than <element_dimension>, converting xi from element to
top_level as follows:
top_level_xi = b + A xi, with b the first column.
The <number_in_xi> of the element is returned, as is the <base_grid_offset> and
the <grid_offset_in_xi> which make up the grid point number conversion:
eg. top_level_grid_point_number = base_grid_offset +
grid_offset_in_xi[i]*grid_number_in_xi[i] (i summed over element_dimension).
Sets values appropriately if element_dimension = top_level_element_dimension.
==============================================================================*/

/**
 * The function allocates an array, <*element_field_nodes_array_address> to store the
 * pointers to the ACCESS'd element nodes.  Components that are not node-based are
 * ignored.  The element nodes are ordered by increasing xi (fastest in xi1, next
 * fastest in xi2 and so on).
 * Beware: for general parameter maps, only returns first node affecting a given
 * element parameter. Also, in general this information is insufficient to determine
 * a common face since it may differ by versions or general maps, but it is
 * currently used for this purpose.
 * The optional <top_level_element> forces inheritance from it as needed.
 * NB.  The nodes need to be DEACCESS'd before the nodes array is DEALLOCATE'd.
 * @param face_number  If non-negative, calculate nodes for face number of
 * element, as if the face element were supplied to this function.
 * @param field  The field to calculate nodes for, or pass NULL to calculate
 * nodes for the first coordinate field defined at element, provided it can
 * be inherited from the optional top_level_element.
 * @return  Result OK on success, ERROR_NOT_FOUND if element has no field or
 * no nodes so cannot be found, otherwise any other error.
 */
int calculate_FE_element_field_nodes(struct FE_element *element,
	int face_number, struct FE_field *field,
	int *number_of_element_field_nodes_address,
	struct FE_node ***element_field_nodes_array_address,
	struct FE_element *top_level_element);

PROTOTYPE_OBJECT_FUNCTIONS(FE_element);

/**
 * Returns the list of <adjacent_elements> not including <element> which share
 * the face indicated by <face_number>.
 * Limitation: does not handle element being connected to itself.
 * On success, caller must DEALLOCATE element array.
 * @return  CMZN_OK on success, any error on failure.
 */
int adjacent_FE_element(struct FE_element *element,
	int face_number, int *number_of_adjacent_elements,
	struct FE_element ***adjacent_elements);

/**
 * Returns true if <field> is equivalently listed in the field information for
 * <element_1> and <element_2>. If neither element has field information or if they
 * do but the field is not defined in either, this is also equivalent.
 */
bool equivalent_FE_field_in_elements(struct FE_field *field,
	struct FE_element *element_1, struct FE_element *element_2);

/**
 * @return  True if all fields are defined in the same way at the two elements.
 * A prerequisite is that they must be valid elements on the same mesh.
 */
bool equivalent_FE_fields_in_elements(struct FE_element *element_1,
	struct FE_element *element_2);

/**
 * @return  The dimension of the element, or 0 if it cannot be determined.
 * @deprecated  Use element->getDimension() instead.
 */
int get_FE_element_dimension(struct FE_element *element);

/**
 * @return  The non-negative element identifier, otherwise
 * DS_LABEL_IDENTIFIER_INVALID on error or if not a true element in the mesh.
 * @deprecated  Use element->getIdentifier() instead.
 */
DsLabelIdentifier get_FE_element_identifier(struct FE_element *element);

/**
 * @return  The non-negative element index within the mesh otherwise
 * DS_LABEL_INDEX_INVALID on error or if not a true element in the mesh.
 * @deprecated  Use element->getIndex() instead.
 */
DsLabelIndex get_FE_element_index(struct FE_element *element);

/** @return  First coordinate field defined on element or inherited from
  * parent. Note inheritence is in same order as FE_field::getOrInheritOnElement */
struct FE_field *FE_element_get_default_coordinate_field(struct FE_element *element);

/** @return  The number of fields defined at <element>.
  * Does not include fields inherited from parent elements. */
int get_FE_element_number_of_fields(struct FE_element *element);

/**
 * Returns the <shape> of the <element>, if any. Invalid elements (in process
 * of being constructed or destroyed, or orphaned) have no shape.
 * @deprecated Use element->getElementShape()
 */
FE_element_shape *get_FE_element_shape(struct FE_element *element);

/**
 * Returns the <face_element> for face <face_number> of <element>, where NULL
 * means there is no face. Element must have a shape and face.
 * Use is discouraged; add new element/mesh API if needed.
 */
struct FE_element *get_FE_element_face(struct FE_element *element, int face_number);

/***************************************************************************//**
 * Calls the <iterator> for each field defined at the <element> in alphabetical
 * order and with indexer fields prioritised so they are defined before the
 * fields indexed by them. Runs until the <iterator> returns 0 or it runs out
 * of fields.  Returns the result of the last <iterator>.
 * Note: expensive
 */
int for_each_FE_field_at_element_alphabetical_indexer_priority(
	FE_element_field_iterator_function *iterator,void *user_data,
	struct FE_element *element);

/***************************************************************************//**
 * Conditional function returning true if <element> number is in the
 * multi range.
 * @param multi_range_void  A struct Multi_range *.
 */
int FE_element_number_is_in_Multi_range(struct FE_element *element,
	void *multi_range_void);

/***************************************************************************//**
 * Iterator function which adds the element number into the multi range.
 * @param multi_range_void  A struct Multi_range *.
 */
int FE_element_add_number_to_Multi_range(
	struct FE_element *element, void *multi_range_void);

/***************************************************************************//**
 * Internal variant of public cmzn_elementiterator_next() which does not
 * access the returned element, for more efficient if less safe usage.
 *
 * @param element_iterator  Element iterator to query and advance.
 * @return  Non-accessed pointer to the next element, or NULL if none remaining.
 */
cmzn_element_id cmzn_elementiterator_next_non_access(
	cmzn_elementiterator_id element_iterator);

/**
 * Returns true if element is a top_level parent of other_element.
 */
bool FE_element_is_top_level_parent_of_element(
	struct FE_element *element, struct FE_element *other_element);

/**
 * Returns the/a top level [ultimate parent] element for <element>. If supplied,
 * the function attempts to verify that the <check_top_level_element> is in
 * fact a valid top_level_element for <element>, otherwise it tries to find one
 * with the specified face, if supplied.
 *
 * If the returned element is different to <element> (ie. is of higher dimension),
 * then this function also fills the matrix <element_to_top_level> with values for
 * converting the xi coordinates in <element> to those in the returned element.
 * <element_to_top_level> should be preallocated to store at least nine FE_values.
 *
 * The use of the <element_to_top_level> matrix is similar to <face_to_element> in
 * FE_element_shape - in fact it is either a copy of it, or calculated from it.
 * It gives the transformation xi(top_level) = b + A xi(element), where b is in the
 * first column of the matrix, and the rest of the matrix is A. Its size depends
 * on the dimension of element:top_level, ie.,
 * 1:2 First 4 values, in form of 2 row X 2 column matrix, used only.
 * 1:3 First 6 values, in form of 3 row X 2 column matrix, used only.
 * 2:3 First 9 values, in form of 3 row X 3 column matrix, used only.
 * NOTE: recursive to handle 1-D to 3-D case.
 */
struct FE_element *FE_element_get_top_level_element_conversion(
	struct FE_element *element,struct FE_element *check_top_level_element,
	cmzn_element_face_type specified_face, FE_value *element_to_top_level);

int FE_element_get_top_level_element_and_xi(struct FE_element *element,
	const FE_value *xi, int element_dimension,
	struct FE_element **top_level_element, FE_value *top_level_xi,
	int *top_level_element_dimension);
/*******************************************************************************
LAST MODIFIED : 13 March 2003

DESCRIPTION :
Finds the <top_level_element>, <top_level_xi> and <top_level_element_dimension>
for the given <element> and <xi>.  If <top_level_element> is already set it
is checked and the <top_level_xi> calculated.
==============================================================================*/

/**
 * Returns in <number_in_xi> the equivalent discretization of <element> for its
 * position - element, face or line - in <top_level_element>. Uses
 * <element_to_top_level> array for line/face conversion as returned by
 * FE_element_get_top_level_element_conversion.
 * <number_in_xi> must have space at least MAXIMUM_ELEMENT_XI_DIMENSIONS
 * integers, as remaining values up to this size are cleared to zero.
 */
int get_FE_element_discretization_from_top_level(struct FE_element *element,
	int *number_in_xi, struct FE_element *top_level_element,
	int *top_level_number_in_xi, FE_value *element_to_top_level);

/**
 * Returns the discretization in <number_in_xi> for displaying graphics over
 * <element>, using the suggested <*top_level_element> if any.
 * If <native_discretization_field> is defined over the
 * element and is grid-based, it's native discretization is used in preference
 * to the <top_level_number_in_xi>.
 * <*top_level_element> can be NULL; final element used will be returned.
 * <top_level_number_in_xi> should be set by the caller as it will be used if
 * there is no native_discretization field or it is not defined over the element;
 * in either case the top_level_number_in_xi used is returned.
 */
int get_FE_element_discretization(struct FE_element *element,
	cmzn_element_face_type face, struct FE_field *native_discretization_field,
	int *top_level_number_in_xi,struct FE_element **top_level_element,
	int *number_in_xi);

/**
 * Checks if the element is 2-D and exterior i.e. a face of exactly one parent
 * element, and if so whether the standard element/face mapping gives it an
 * inward normal.
 *
 * @param element  The element to test.
 * @return  True if element is an exterior face with inward normal, otherwise false.
 */
bool FE_element_is_exterior_face_with_inward_normal(struct FE_element *element);

/**
 * Add nodes used by this element, including inherited from parent elements, to
 * the supplied labels group.
 * Caller must ensure the group was created for the appropriate FE_nodeset.
 * Note node inheritance is currently only for default coordinate field.
 * @return  CMZN_OK on success, any other value on failure.
 */
int cmzn_element_add_nodes_to_labels_group(cmzn_element *element, DsLabelsGroup &nodeLabelsGroup);

/**
 * Remove nodes used by this element, including inherited from parent elements,
 * from the supplied labels group.
 * Caller must ensure the group was created for the appropriate FE_nodeset.
 * Note node inheritance is currently only for default coordinate field.
 * @return CMZN_OK on success, any other value on failure.
 */
int cmzn_element_remove_nodes_from_labels_group(cmzn_element *element, DsLabelsGroup &nodeLabelsGroup);

/**
 * Conditional function.
 * @return  1 if element is top-level i.e. has no parents, otherwise 0.
 */
int FE_element_is_top_level(struct FE_element *element,void *dummy_void);

/** @return  True if the <field> is defined on the <element>, directly or
  * inherited from any parent element it is a face of.
 */
bool FE_field_is_defined_in_element(struct FE_field *field,
	struct FE_element *element);

/**
 * Returns true if <field> is defined in <element> directly, not inherited from
 * a parent element it is a face of.
 */
bool FE_field_is_defined_in_element_not_inherited(struct FE_field *field,
	struct FE_element *element);

/** @return  True if any component of field is grid-based in element. */
bool FE_element_field_is_grid_based(struct FE_element *element,
	struct FE_field *field);

/** @return  True if any field component defined on element is grid-based. */
bool FE_element_has_grid_based_fields(struct FE_element *element);

/**
 * Partner function to FE_element_smooth_FE_field. Averages node derivatives.
 * Assigns the nodal first derivatives of fe_field at time in node with the
 * values from node_accumulate_fe_field divided by the value of the
 * element_count_fe_field, handling multiple versions.
 * Finally undefines node_accumulate_fe_field and element_count_fe_field.
 * @return  1 on success, 0 on failure.
 */
int FE_node_smooth_FE_field(struct FE_node *node, struct FE_field *fe_field,
	FE_value time, struct FE_field *node_accumulate_fe_field,
	struct FE_field *element_count_fe_field);

/**
 * For each node contributing to <fe_field> in <element>, accumulates delta
 * coordinates along each element edge are accumulated.
 *
 * @param node_accumulate_fe_field  Temporary field for accumulating node
 * values. Must have same number of components as fe_field.
 * @param element_count_fe_field  Field to store number of elements accumulated
 * at node, for final averaging.
 *
 * After making calls to this function for all the intended elements, call
 * FE_node_smooth_FE_field for each node to divide the accumulated derivatives
 * by the number of elements they are over and to undefine the
 * node_accumulate_fe_field and element_count_fe_field.
 *
 * Sets all scale factors used for <fe_field> to 1.0.
 *
 * Notes:
 * Only works for "line" shapes with Hermite basis functions.
 * - <element> shold not be global = not merged into an FE_region.
 * - <fe_field> should be of type FE_VALUE_VALUE.
 * - returns 1 without errors if fe_field is not defined on this element or the
 *   element has no field information, or the field cannot be smoothed.
 * - handles multiple versions at nodes.
 * - may not handle obscure tensor product combinations such as linear in xi1,
 *   cubic Hermite in xi2, since indexing of element parameters assumes order:
 *   value d/dxi1 d/dxi2 d2/dxi1dxi2 d/dxi3 d2/dxi1dxi3 d2/dxi2dxi3 d3/dxi1dxi2dxi3
 */
bool FE_element_smooth_FE_field(struct FE_element *element,
	struct FE_field *fe_field, FE_value time, 
	struct FE_field *node_accumulate_fe_field,
	struct FE_field *element_count_fe_field);

struct FE_field_order_info *CREATE(FE_field_order_info)(void);
/*******************************************************************************
LAST MODIFIED : 4 September 2001

DESCRIPTION
Frees them memory used by field_order_info.
==============================================================================*/

int DESTROY(FE_field_order_info)(
	struct FE_field_order_info **field_order_info_address);
/*******************************************************************************
LAST MODIFIED : 4 September 2001

Frees memory for FE_field_order_info
==============================================================================*/

int add_FE_field_order_info_field(
	struct FE_field_order_info *field_order_info, struct FE_field *field);
/*******************************************************************************
LAST MODIFIED : 4 September 2001

DESCRIPTION :
Adds <field> to the end of the list of fields in <field_order_info>.
==============================================================================*/

/***************************************************************************//**
 * FE_field iterator for calling add_FE_field_order_info_field.
 */
int FE_field_add_to_FE_field_order_info(struct FE_field *field,
	void *field_order_info_void);

/***************************************************************************//**
 * Ensures indexer fields precede fields they index in FE_field_order_info.
 */
int FE_field_order_info_prioritise_indexer_fields(
	struct FE_field_order_info *field_order_info);

int clear_FE_field_order_info(struct FE_field_order_info *field_order_info);
/*******************************************************************************
LAST MODIFIED : 4 September 2001

DESCRIPTION :
Clears the fields from <field_order_info>.
==============================================================================*/

int get_FE_field_order_info_number_of_fields(
	struct FE_field_order_info *field_order_info);
/*******************************************************************************
LAST MODIFIED : 13 July 1999

DESCRIPTION :
Gets the <field_order_info> number_of_fields
==============================================================================*/

struct FE_field *get_FE_field_order_info_field(
	struct FE_field_order_info *field_order_info,int field_number);
/*******************************************************************************
LAST MODIFIED : 13 July 1999

DESCRIPTION :
Gets the <field_order_info> field at the specified field_number
==============================================================================*/

/**
 * Returns the number of permutations known for the changing to the adjacent
 * element at face <face_number>.  It would be better to extend FE_element_shape
 * to take account of rotations and flipping.
 */
int FE_element_get_number_of_change_to_adjacent_element_permutations(
	struct FE_element *element, FE_value *xi, int face_number);

/**
 * Steps into the adjacent element through face <face_number>, updating the
 * <element_address> location.
 * If <xi> is not NULL then the <xi_face> coordinates are converted to an xi
 * location in the new element.
 * If <increment> is not NULL then it is converted into an equvalent increment
 * in the new element.
 * <permutation> is used to resolve the possible rotation and flipping of the
 * local face xi coordinates between the two parents.
 * The shape mapping from parents are reused for all elements of the same shape
 * and do not take into account the relative orientation of the parents.  It would
 * be better to do so.  The range of possible permutations is from 0 to the value
 * returned from FE_element_get_number_of_change_to_adjacent_element_permutations.
 */
int FE_element_change_to_adjacent_element(struct FE_element **element_address,
	FE_value *xi, FE_value *increment, int *face_number, FE_value *xi_face,
	int permutation);

/**
 * Adds the <increment> to <xi>.  If this moves <xi> outside of the element, then
 * if an adjacent element is found then the element and xi location are changed
 * to this element and the stepping continues using the remaining increment.  If
 * no adjacent element is found then the <xi> will be on the element boundary and
 * the <increment> will contain the fraction of the increment not used.
 */
int FE_element_xi_increment(struct FE_element **element_address,FE_value *xi,
	FE_value *increment);

enum cmzn_field_domain_type cmzn_field_domain_type_enum_from_string(
	const char *string);

char *cmzn_field_domain_type_enum_to_string(enum cmzn_field_domain_type type);

enum cmzn_element_face_type cmzn_element_face_type_enum_from_string(
	const char *string);

char *cmzn_element_face_type_enum_to_string(enum cmzn_element_face_type type);

enum cmzn_element_point_sampling_mode cmzn_element_point_sampling_mode_enum_from_string(
	const char *string);

char *cmzn_element_point_sampling_mode_enum_to_string(
	enum cmzn_element_point_sampling_mode mode);


#endif /* !defined (FINITE_ELEMENT_H) */
