/**
 * @file elementtemplate.h
 *
 * The public interface to Zinc element templates.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef CMZN_ELEMENTTEMPLATE_H__
#define CMZN_ELEMENTTEMPLATE_H__

#include "types/elementid.h"
#include "types/elementbasisid.h"
#include "types/elementfieldtemplateid.h"
#include "types/elementtemplateid.h"
#include "types/fieldid.h"
#include "types/nodeid.h"

#include "opencmiss/zinc/zincsharedobject.h"

/*
Global types
------------
*/

#ifdef __cplusplus
extern "C" {
#endif

/*
Global functions
----------------
*/

/**
 * Returns a new handle to the element template with reference count incremented.
 *
 * @param elementtemplate  The element template to obtain a new handle to.
 * @return  New handle to element template, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_elementtemplate_id cmzn_elementtemplate_access(
	cmzn_elementtemplate_id elementtemplate);

/**
 * Destroys this handle to the element template and sets it to NULL.
 * Internally this decrements the reference count.
 *
 * @param element_template_address  Address of handle to element_template
 * to destroy.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
ZINC_API int cmzn_elementtemplate_destroy(
	cmzn_elementtemplate_id *element_template_address);

/**
 * Get the current element shape type set in the element template.
 *
 * @param element_template  Element template to query.
 * @return  The element shape type, or INVALID if not set or error.
 */
ZINC_API enum cmzn_element_shape_type cmzn_elementtemplate_get_element_shape_type(
	cmzn_elementtemplate_id element_template);

/**
 * Set the element shape to a standard element shape type. The shape must have
 * the same dimension as the mesh from which the element template was created.
 * Beware that face mappings are lost if shape changes are merged into elements.
 *
 * @param element_template  Element template to modify.
 * @param shape_type  Standard element shapes enumerated value. Note can be
 * INVALID which means the shape is not set when merged into an element, but
 * new elements cannot be created unless they have a valid shape.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
ZINC_API int cmzn_elementtemplate_set_element_shape_type(
	cmzn_elementtemplate_id element_template, enum cmzn_element_shape_type shape_type);

/**
 * Gets the number of local nodes this element_template can address.
 *
 * @param element_template  Element template to query.
 * @return  The number of local nodes, or 0 on error.
 */
ZINC_API int cmzn_elementtemplate_get_number_of_nodes(
	cmzn_elementtemplate_id element_template);

/**
 * Sets the number of local nodes this element_template can address. This must
 * be done before defining fields that index them.
 * This number cannot be reduced.
 * \note DEPRECATED. Use element field template functions instead.
 *
 * @param element_template  Element template to modify.
 * @param number_of_nodes  The number of nodes.
 * @return  Status CMZN_OK on success, otherwise CMZN_ERROR_ARGUMENT.
 */
ZINC_API int cmzn_elementtemplate_set_number_of_nodes(
	cmzn_elementtemplate_id element_template, int number_of_nodes);

/**
 * Define the field component(s) on the element template using the
 * element field template. The element template is not valid until
 * all components are defined for all fields.
 *
 * @param elementtemplate  Element template to modify.
 * @param field  The field to define. May be finite element type only.
 * @param component_number  The component to define from 1 to the number of
 * field components, or -1 to define all components.
 * @param eft  The element field template. Must be for mesh this element
 * template was created from.
 * @return  Result OK on success, any other value on failure.
 */
ZINC_API int cmzn_elementtemplate_define_field(
	cmzn_elementtemplate_id elementtemplate, cmzn_field_id field,
	int component_number, cmzn_elementfieldtemplate_id eft);

/**
 * Defines per element constant field or field component in the
 * element_template.
 * \note DEPRECATED. Use element field template and define field function instead.
 * @see cmzn_elementtemplate_define_field
 *
 * @param element_template  Element template to modify.
 * @param field  The field to define. May be finite element type only.
 * @param component_number  The component to define from 1 to the number of
 * field components, or -1 to define all components.
 *
 * @return  Status CMZN_OK on success, any other value on failure.
 */
ZINC_API int cmzn_elementtemplate_define_field_element_constant(
	cmzn_elementtemplate_id element_template,
	cmzn_field_id field, int component_number);

/**
 * Defines a nodally interpolated element field or field component in the
 * element template. For Lagrange, simplex and constant basis function types
 * there are as many nodes as basis functions and the single parameter per
 * node has VALUE label. For Hermite bases with multiple functions per node,
 * default parameters are expected in numerical order of the node value label
 * e.g. for bicubic Hermite the first four functions are for the first node and
 * expected in order: VALUE D_DS1 D_DS2 D2_DS1DS2.
 * By default, versions are initialised to 1, and no scaling is used.
 * In all cases local nodes cycle fastest in lowest element xi direction.
 * Do not use in combination with new general define field function.
 * \note DEPRECATED. Use element field template and define field function instead.
 * @see cmzn_elementtemplate_define_field
 *
 * @param elementtemplate  Element template to modify.
 * @param field  The field to define. May be finite element type only.
 * @param component_number  The component to define from 1 to the number of
 * field components, or -1 to define all components with identical basis and
 * nodal mappings.
 * @param basis  The element basis to use for all field components.
 * @param basis_number_of_nodes  The number of nodes indexed by the basis,
 * equals the size of the local_node_indexes array.
 * @param local_node_indexes  Array containing the local node indexes of the
 * nodes from which element field parameters are mapped, which range from 1 to
 * the number of nodes set for the element_template. Local nodes are ordered
 * by lowest xi coordinate varying fastest, e.g. for biquadratic Lagrange:
 * xi = (0,0), (0.5,0), (1,0), (0,0.5), (0.5,0.5) ...
 * @return  Status CMZN_OK on success, any other value on failure.
 */
ZINC_API int cmzn_elementtemplate_define_field_simple_nodal(
	cmzn_elementtemplate_id elementtemplate,
	cmzn_field_id field,  int component_number,
	cmzn_elementbasis_id basis, int basis_number_of_nodes,
	const int *local_node_indexes);

/**
 * For a field defined by node interpolation, sets the node value label (value
 * or derivative) mapped to one function parameter in the element basis.
 * Note that any values set by this function are reset if the field is
 * re-defined at the node.
 * Only to be used after define field simple nodal.
 * \note DEPRECATED. Use element field template instead.
 *
 * @param elementtemplate  Element template to modify.
 * @param field  The field to modify mapping for.
 * @param component_number  The component to modify mapping for, from 1 to the
 * number of field components, or -1 to set all components identically.
 * @param basis_node_index  The basis node index starting at 1, up to number
 * used by basis.
 * @see cmzn_elementbasis_get_number_of_nodes
 * @param node_function_index  The function index starting at 1, up to the
 * number per node in the element basis.
 * @see cmzn_elementbasis_get_number_of_functions_per_node
 * @param node_value_label  The node value label to set.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
ZINC_API int cmzn_elementtemplate_set_map_node_value_label(
	cmzn_elementtemplate_id elementtemplate, cmzn_field_id field,
	int component_number, int basis_node_index, int node_function_index,
	enum cmzn_node_value_label node_value_label);

/**
 * For a field defined by node interpolation, sets the version number of the
 * value mapped to one function parameter in the element basis.
 * Note that any values set by this function are reset if the field is
 * re-defined at the node.
 * Only to be used after define field simple nodal.
 * \note DEPRECATED. Use element field template instead.
 *
 * @param elementtemplate  Element template to modify.
 * @param field  The field to modify mapping for.
 * @param component_number  The component to modify mapping for, from 1 to the
 * number of field components, or -1 to set all components identically.
 * @param basis_node_index  The basis node index starting at 1, up to number
 * used by basis.
 * @see cmzn_elementbasis_get_number_of_nodes
 * @param node_function_index  The function index starting at 1, up to the
 * number per node in the element basis.
 * @see cmzn_elementbasis_get_number_of_functions_per_node
 * @param node_version  The version number to set, starting at 1.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
ZINC_API int cmzn_elementtemplate_set_map_node_version(
	cmzn_elementtemplate_id elementtemplate, cmzn_field_id field,
	int component_number, int basis_node_index, int node_function_index,
	int version_number);

/**
 * Gets the global node at a given local node index in the element_template.
 * \note DEPRECATED. Only used with deprecated define field simple nodal.
 *
 * @param element_template  Element template to query.
 * @param local_node_index  The index from 1 to number of nodes in template.
 * @return  Handle to global node, or NULL/invalid handle if none or failed.
 */
ZINC_API cmzn_node_id cmzn_elementtemplate_get_node(
	cmzn_elementtemplate_id element_template, int local_node_index);

/**
 * Sets the global node at a given local node index in the element_template.
 * \note DEPRECATED. Set nodes directly for the element and element field
 * template instead.
 *
 * @param element_template  Element template to modify.
 * @param local_node_index  The index from 1 to number of nodes in template.
 * @param node  The global node to set at that index.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
ZINC_API int cmzn_elementtemplate_set_node(cmzn_elementtemplate_id element_template,
	int local_node_index, cmzn_node_id node);

/**
 * Removes field from list of fields to define or undefine in element template.
 *
 * @param elementtemplate  Element template to modify.
 * @param field  The field to remove. May be finite element type only.
 * @return  Result OK on success, any other value on failure.
 */
ZINC_API int cmzn_elementtemplate_remove_field(
	cmzn_elementtemplate_id elementtemplate, cmzn_field_id field);

/**
 * Marks field to be undefined when next merged into an existing element. Has
 * no effect on newly created elements. Removes field from define list if
 * present.
 *
 * @param elementtemplate  Element template to modify.
 * @param field  The field to undefine. May be finite element type only.
 * @return  Result OK on success, any other value on failure.
 */
ZINC_API int cmzn_elementtemplate_undefine_field(
	cmzn_elementtemplate_id elementtemplate, cmzn_field_id field);

#ifdef __cplusplus
}
#endif

#endif /* CMZN_ELEMENTTEMPLATE_H__ */
