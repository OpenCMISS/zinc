/**
 * @file element.h
 *
 * The public interface to Zinc finite elements and related classes.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef CMZN_ELEMENT_H__
#define CMZN_ELEMENT_H__

#include "types/differentialoperatorid.h"
#include "types/elementid.h"
#include "types/elementtemplateid.h"
#include "types/elementfieldtemplateid.h"
#include "types/fieldid.h"
#include "types/fieldmoduleid.h"
#include "types/meshid.h"
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
 * Convert a short name into an enum if the name matches any of the members in
 * the enum.
 *
 * @param string  string of the short enumerator name
 * @return  the correct enum type if a match is found.
 */
ZINC_API enum cmzn_element_shape_type cmzn_element_shape_type_enum_from_string(
	const char *string);

/**
 * Return an allocated short name of the enum type from the provided enum.
 * User must call cmzn_deallocate to destroy the successfully returned string.
 *
 * @param type  enum to be converted into string
 * @return  an allocated string which stored the short name of the enum.
 */
ZINC_API char *cmzn_element_shape_type_enum_to_string(enum cmzn_element_shape_type type);

/**
 * Returns a new handle to the element iterator with reference count incremented.
 *
 * @param mesh  The element iterator to obtain a new handle to.
 * @return  New handle to element iterator, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_elementiterator_id cmzn_elementiterator_access(
	cmzn_elementiterator_id element_iterator);

/**
 * Destroys this handle to the element_iterator and sets it to NULL.
 *
 * @param element_iterator_address  Address of handle to element_iterator to
 * destroy.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
ZINC_API int cmzn_elementiterator_destroy(
	cmzn_elementiterator_id *element_iterator_address);

/**
 * Returns a handle to the next element in the container being iterated over
 * then advances the iterator position. The caller is required to destroy the
 * returned element handle.
 *
 * @param element_iterator  Element iterator to query and advance.
 * @return  Handle to next element, or NULL/invalid handle if none or failed.
 */
ZINC_API cmzn_element_id cmzn_elementiterator_next(
	cmzn_elementiterator_id element_iterator);

/**
 * Returns a new handle to the element with reference count incremented.
 *
 * @param element  Handle to an element.
 * @return  New handle to the element, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_element_id cmzn_element_access(cmzn_element_id element);

/**
 * Destroys this handle to the element and sets it to NULL.
 * Internally this decrements the reference count.
 *
 * @param element_address  Address of handle to the element to destroy.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
ZINC_API int cmzn_element_destroy(cmzn_element_id *element_address);

/**
 * Returns the number of dimensions of the element's chart.
 *
 * @param element  The element to query.
 * @return  The dimension.
 */
ZINC_API int cmzn_element_get_dimension(cmzn_element_id element);

/**
 * Gets the element field template defining field component on element, or
 * all field components for special homogeneous case with component -1.
 *
 * @param element  The element to query.
 * @param field  The field to query. May be finite element type only.
 * @param componentNumber  The component to query from 1 to number of
 * components, or -1 to request element field template used by all components
 * which fails if not homogeous i.e. if components have different templates.
 * @return  Handle to element field template, or NULL/invalid handle if field
 * not defined or on failure including special component -1 is used and field
 * is not homogeneous over components in element.
 */
ZINC_API cmzn_elementfieldtemplate_id cmzn_element_get_elementfieldtemplate(
	cmzn_element_id element, cmzn_field_id field, int componentNumber);

/**
 * Returns the non-negative integer uniquely identifying the element in its
 * mesh.
 *
 * @param element  The element to query.
 * @return  The non-negative integer identifier of the element, or a negative
 * value if element is invalid.
 */
ZINC_API int cmzn_element_get_identifier(cmzn_element_id element);

/**
 * Set an integer uniquely identifying the element in its mesh.
 *
 * @param element  The element to query.
 * @param identifier  unique identifier to be set for the element
 *
 * @return  CMZN_OK if set the identifier successfully,
 * 	any other value on fail.
 */
ZINC_API int cmzn_element_set_identifier(cmzn_element_id element, int identifier);

/**
 * Get the mesh which owns this element.
 *
 * @param element  The element to query.
 * @return  Handle to the owning mesh, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_mesh_id cmzn_element_get_mesh(cmzn_element_id element);

/**
 * Gets a local node used by element field template in element.
 * 
 * @param element  The element to query.
 * @param eft  The element field template to get the node for.
 * @param localNodeIndex  The local index of the node to get, starting at 1.
 * @return  Handle to valid node, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_node_id cmzn_element_get_node(cmzn_element_id element,
	cmzn_elementfieldtemplate_id eft, int localNodeIndex);

/**
 * Sets a local node used by element field template in element. Note if any
 * node-type scale factors are indexed by a node being set or changed, new
 * scale factors will be discovered or created for it, and any old scale
 * factors will be released. Note that any existing scale factors discovered
 * for a node keep their current values, and newly created scale factors values
 * are assigned the current value for the scale factor on the former node.
 * 
 * @param element  The element to modify.
 * @param eft  The element field template to set the node for.
 * @param localNodeIndex  The local index of the node to set, starting at 1.
 * @param node  The global node to set. Note once a local node index has
 * a valid node, it is not permitted to remove it.
 * @return  Result OK on success, any other value on failure.
 */
ZINC_API int cmzn_element_set_node(cmzn_element_id element,
	cmzn_elementfieldtemplate_id eft, int localNodeIndex, cmzn_node_id node);

/**
 * Sets all the local nodes used by the given element field template in this
 * element by their identifiers. The nodes are found by identifier in the
 * nodeset for the elements' owning mesh. It is an error if any node with a
 * valid identifier is not found.
 * Note it is illegal to clear a node that has already been set. Also, if any
 * node-type scale factors are indexed by a node being set or changed, new
 * scale factors will be discovered or created for it, and any old scale
 * factors will be released. Note that any existing scale factors discovered
 * for a node keep their current values, and newly created scale factors values
 * are assigned the current value for the scale factor on the former node.
 * 
 * @param element  The element to modify.
 * @param eft  The element field template to set nodes for.
 * @param identifiersCount  The size of the identifiers array. Must equal the
 * number of nodes in the element field template.
 * @param identifiersIn  The array of node identifiers to set.
 * @return  Result OK on success, any other value on failure.
 */
ZINC_API int cmzn_element_set_nodes_by_identifier(cmzn_element_id element,
	cmzn_elementfieldtemplate_id eft, int identifiersCount,
	const int *identifiersIn);

/**
 * Get a scale factor for the element field template in element.
 * 
 * @param element  The element to query.
 * @param eft  The element field template to get the scale factor for.
 * @param localScaleFactorIndex  The local index of the scale factor to get,
 * starting at 1.
 * @param valueOut  Address of variable/memory which on success is set to the
 * scale factor value.
 * @return  Result OK on success, any other value on failure.
 */
ZINC_API int cmzn_element_get_scale_factor(cmzn_element_id element,
	cmzn_elementfieldtemplate_id eft, int localScaleFactorIndex, double *valueOut);

/**
 * Set a scale factor for the element field template in element.
 * 
 * @param element  The element to modify.
 * @param eft  The element field template to set the scale factor for.
 * @param localScaleFactorIndex  The local index of the scale factor to set,
 * starting at 1.
 * @param value  The scale factor value to set.
 * @return  Result OK on success, any other value on failure.
 */
ZINC_API int cmzn_element_set_scale_factor(cmzn_element_id element,
	cmzn_elementfieldtemplate_id eft, int localScaleFactorIndex, double value);

/**
 * Gets all scale factors used by the given element field template in this
 * element.
 * 
 * @param element  The element to query.
 * @param eft  The element field template to get scale factors for.
 * @param valuesCount  The size of the values array. Must equal the number of
 * scale factors in the element field template.
 * @param valuesOut  Array to fill with the scale factors.
 * @return  Result OK on success, any other value on failure.
 */
ZINC_API int cmzn_element_get_scale_factors(cmzn_element_id element,
	cmzn_elementfieldtemplate_id eft, int valuesCount, double *valuesOut);

/**
 * Sets all scale factors used by the given element field template in this
 * element.
 * 
 * @param element  The element to modify.
 * @param eft  The element field template to set scale factors for.
 * @param valuesCount  The size of the values array. Must equal the number of
 * scale factors in the element field template.
 * @param valuesIn  The array of scale factors to set.
 * @return  Result OK on success, any other value on failure.
 */
ZINC_API int cmzn_element_set_scale_factors(cmzn_element_id element,
	cmzn_elementfieldtemplate_id eft, int valuesCount, const double *valuesIn);

/**
 * Gets the shape type of the element. Note that legacy meshes may return an
 * unknown shape type for certain custom element shapes e.g. polygon shapes.
 * It is intended that future revisions of the API will offer more detailed
 * shape query and modification functions.
 *
 * @param element  Element to query.
 * @return  The element's shape type.
 */
ZINC_API enum cmzn_element_shape_type cmzn_element_get_shape_type(
	cmzn_element_id element);

/**
 * Modifies the element to define or undefine fields as described in the
 * element template, and possibly change its shape.
 *
 * @param element  The element to modify.
 * @param elementtemplate  Template describing fields to define or undefine.
 * Must be created for this mesh and valid. If the element template has a valid
 * shape, it will be set for the element; if it has an invalid shape, the
 * element will keep its current shape.
 * @return  Result OK on success, any other value on failure.
 */
ZINC_API int cmzn_element_merge(cmzn_element_id element,
	cmzn_elementtemplate_id elementtemplate);

#ifdef __cplusplus
}
#endif

#endif /* CMZN_ELEMENT_H__ */
