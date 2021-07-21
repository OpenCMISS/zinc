/**
 * @file mesh.h
 *
 * The public interface to Zinc finite element meshes and related classes.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef CMZN_MESH_H__
#define CMZN_MESH_H__

#include "types/differentialoperatorid.h"
#include "types/elementid.h"
#include "types/elementbasisid.h"
#include "types/elementtemplateid.h"
#include "types/elementfieldtemplateid.h"
#include "types/fieldid.h"
#include "types/meshid.h"
#include "types/fieldmoduleid.h"

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
 * Get a handle to the default mesh of a given dimension. Cmgui is currently
 * limited to 1 mesh of each dimension from 1 to 3. These meshes have default
 * names of "mesh_Nd", where "N" is the dimension.
 *
 * @param fieldmodule  The field module the mesh belongs to.
 * @param dimension  The dimension of the mesh from 1 to 3.
 * @return  Handle to the mesh, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_mesh_id cmzn_fieldmodule_find_mesh_by_dimension(
	cmzn_fieldmodule_id fieldmodule, int dimension);

/**
 * Get a handle to a finite element mesh from its name. A mesh is the container
 * of elements of a fixed dimension. Valid names may be any element_group field,
 * or any of the following special names:
 * "mesh3d" = 3-D elements.
 * "mesh2d" = 2-D elements including faces of 3-D elements.
 * "mesh1d" = 1-D elements including faces (lines) of 2-D elements.
 * Note that the default names for element group fields created from a group
 * is GROUP_NAME.MESH_NAME, with mesh names as above.
 *
 * @param fieldmodule  The field module the mesh belongs to.
 * @param name  The name of the finite element mesh.
 * @return  Handle to the mesh, or NULL/invalid handle if not found or failed.
 */
ZINC_API cmzn_mesh_id cmzn_fieldmodule_find_mesh_by_name(
	cmzn_fieldmodule_id fieldmodule, const char *mesh_name);

/**
 * Returns a new handle to the mesh with reference count incremented.
 *
 * @param mesh  The mesh to obtain a new handle to.
 * @return  New handle to the mesh, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_mesh_id cmzn_mesh_access(cmzn_mesh_id mesh);

/**
 * Destroys this handle to the finite element mesh and sets it to NULL.
 * Internally this decrements the reference count.
 *
 * @param mesh_address  Address of handle to the mesh to destroy.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
ZINC_API int cmzn_mesh_destroy(cmzn_mesh_id *mesh_address);

/**
 * Returns whether the element is from the mesh.
 *
 * @param mesh  The mesh to query.
 * @param element  The element to query about.
 * @return  Boolean true if element is in the mesh, otherwise false.
 */
ZINC_API bool cmzn_mesh_contains_element(cmzn_mesh_id mesh,
	cmzn_element_id element);

/**
 * Create an element field template to describe parameter mapping and
 * interpolation of a scalar field / field component over elements of the mesh.
 * The new template defaults to NODE parameter mapping mode, with the standard
 * number of nodes for the element basis. For Lagrange, simplex and constant
 * basis function types there are as many nodes as basis functions and the
 * single parameter per node has VALUE label. For Hermite bases with multiple
 * functions per node, default parameters are expected in numerical order of
 * the node value label e.g. for bicubic Hermite the first four functions are
 * for the first node and expected in order: VALUE D_DS1 D_DS2 D2_DS1DS2.
 * By default, versions are initialised to 1, and no scaling is used.
 * In all cases local nodes cycle fastest in lowest element xi direction.
 *
 * @param mesh  Handle to the mesh the template works with.
 * @param elementbasis  The element basis which will be used for interpolation,
 * which must have the same dimension as the mesh.
 * @return  Handle to new element field template, or NULL/invalid handle on
 * failure.
 */
ZINC_API cmzn_elementfieldtemplate_id cmzn_mesh_create_elementfieldtemplate(
	cmzn_mesh_id mesh, cmzn_elementbasis_id elementbasis);

/**
 * Create a blank template from which new elements can be created in this mesh.
 * Also used for defining new fields over elements. The new element template
 * does not have a valid shape, which allows adding fields to existing elements
 * without changing shape, however a valid shape must be set to create new
 * elements with this element template.
 *
 * @param mesh  Handle to the mesh the template works with.
 * @return  Handle to new element template, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_elementtemplate_id cmzn_mesh_create_elementtemplate(
	cmzn_mesh_id mesh);

/**
 * Create a new element in this mesh with shape and fields described by the
 * element_template. Returns handle to new element.
 * @see cmzn_mesh_define_element
 *
 * @param mesh  Handle to the mesh to create the new element in.
 * @param identifier  Non-negative integer identifier of new element, or -1 to
 * automatically generate, starting from 1. Fails if supplied identifier already
 * used by an existing element.
 * @param element_template  Template describing element shape and fields to
 * define or undefine. Must be valid, with a valid shape.
 * @return  Handle to new element, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_element_id cmzn_mesh_create_element(cmzn_mesh_id mesh,
	int identifier, cmzn_elementtemplate_id element_template);

/**
 * Create an element iterator object for iterating through the elements in the
 * mesh which are ordered from lowest to highest identifier. The iterator
 * initially points at the position before the first element, so the first call
 * to the element iterator next method returns the first element and advances the
 * iterator.
 * Iterator becomes invalid if mesh is modified or any of its elements are
 * given new identifiers.
 * @see cmzn_elementiterator_next
 *
 * @param mesh  Handle to the mesh whose elements are to be iterated over.
 * @return  Handle to new element iterator, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_elementiterator_id cmzn_mesh_create_elementiterator(
	cmzn_mesh_id mesh);

/**
 * Create a new element in this mesh with shape and fields described by the
 * element_template.
 * @see cmzn_mesh_create_element
 *
 * @param mesh  Handle to the mesh to create the new element in.
 * @param identifier  Non-negative integer identifier of new element, or -1 to
 * automatically generate, starting from 1. Fails if supplied identifier already
 * used by an existing element.
 * @param element_template  Template describing element shape and fields to
 * define or undefine. Must be valid, with a valid shape.
 * @return  Result OK on success, any other value on failure.
 */
ZINC_API int cmzn_mesh_define_element(cmzn_mesh_id mesh, int identifier,
	cmzn_elementtemplate_id element_template);

/**
 * Destroy all elements in mesh, also removing them from any related groups.
 * All handles to the destroyed element become invalid.
 *
 * @param mesh  Handle to mesh to destroy elements from.
 * @return  Status CMZN_OK if all elements destroyed, any other value if failed.
 */
ZINC_API int cmzn_mesh_destroy_all_elements(cmzn_mesh_id mesh);

/**
 * Destroy the element if it is in the mesh. Removes element from any related
 * groups it is in. All handles to the destroyed element become invalid.
 *
 * @param mesh  Handle to the mesh whose element is to be destroyed.
 * @param element  The element to destroy.
 * @return  Status CMZN_OK if element is successfully destroyed, any other
 * value if failed.
 */
ZINC_API int cmzn_mesh_destroy_element(cmzn_mesh_id mesh, cmzn_element_id element);

/**
 * Destroy all elements in the mesh for which the conditional field is true i.e.
 * non-zero valued in element. These elements are removed from any related
 * groups they are in. All handles to removed elements become invalid.
 * Results are undefined if conditional field is not constant over element.
 * Note that group and element_group fields are valid conditional fields.
 *
 * @param mesh  Handle to the mesh to destroy elements from.
 * @param conditional_field  Field which if non-zero in the element indicates it
 * is to be destroyed.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
ZINC_API int cmzn_mesh_destroy_elements_conditional(cmzn_mesh_id mesh,
	cmzn_field_id conditional_field);

/**
 * Return a handle to the element in the mesh with this identifier.
 *
 * @param mesh  Handle to the mesh to find the element in.
 * @param identifier  Non-negative integer identifier of element.
 * @return  Handle to element, or NULL/invalid handle if not found or failed.
 */
ZINC_API cmzn_element_id cmzn_mesh_find_element_by_identifier(cmzn_mesh_id mesh,
	int identifier);

/**
 * Returns the differential operator giving a field derivative of the given
 * order with respect to the mesh's chart. The term identifies which of the
 * possible differential operator terms are available for the order and
 * dimension of the mesh.
 *
 * @param mesh  Handle to the mesh to get differential operator from.
 * @param order  The order of the derivative. Currently must be 1.
 * @param term  Which of the (dimensions)^order terms is required,
 * starting at 1, or -1 to evaluate all terms.
 * For order 1, the term corresponds to a element chart/xi direction.
 * @return  Handle to differential operator, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_differentialoperator_id cmzn_mesh_get_chart_differentialoperator(
	cmzn_mesh_id mesh, int order, int term);

/**
 * Returns the number of dimensions of the mesh.
 *
 * @param mesh  Handle to the mesh to query.
 * @return  dimension of mesh.
 */
ZINC_API int cmzn_mesh_get_dimension(cmzn_mesh_id mesh);

/**
 * Returns handle to field module for region this mesh belongs to.
 *
 * @param mesh  The mesh from which to obtain the field module.
 * @return  Handle to field module, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_fieldmodule_id cmzn_mesh_get_fieldmodule(cmzn_mesh_id mesh);

/**
 * Get the master mesh which owns the elements for this mesh. Can be the
 * same as the supplied mesh if it is a master.
 *
 * @param mesh  The mesh to query.
 * @return  Handle to the master mesh, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_mesh_id cmzn_mesh_get_master_mesh(cmzn_mesh_id mesh);

/**
 * Return the name of the mesh.
 *
 * @see cmzn_deallocate
 * @param mesh  The mesh whose name is requested.
 * @return  On success: allocated string containing mesh name. Up to caller to
 * free using cmzn_deallocate().
 */
ZINC_API char *cmzn_mesh_get_name(cmzn_mesh_id mesh);

/**
 * Return the number of elements in the mesh.
 *
 * @param mesh  Handle to the mesh to query.
 * @return  Number of elements in mesh.
 */
ZINC_API int cmzn_mesh_get_size(cmzn_mesh_id mesh);

/**
 * Check if two mesh handles refer to the same object.
 *
 * @param mesh1  The first mesh to match.
 * @param mesh2  The second mesh to match.
 * @return  Boolean true if the two meshes match, false if not.
 */
ZINC_API bool cmzn_mesh_match(cmzn_mesh_id mesh1, cmzn_mesh_id mesh2);

/**
 * If the mesh is a mesh group i.e. subset of elements from a master mesh,
 * get the mesh group specific interface for add/remove functions.
 *
 * @param field  The mesh to be cast.
 * @return  Handle to derived mesh group, or NULL/invalid handle if wrong type or failed.
 */
ZINC_API cmzn_mesh_group_id cmzn_mesh_cast_group(cmzn_mesh_id mesh);

/**
 * Destroys this handle to the mesh group and sets it to NULL.
 * Internally this decrements the reference count.
 *
 * @param mesh_group_address  Address of mesh group handle to destroy.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
ZINC_API int cmzn_mesh_group_destroy(cmzn_mesh_group_id *mesh_group_address);

/**
 * Cast mesh group back to its base mesh class.
 * IMPORTANT NOTE: Returned mesh does not have incremented reference count and
 * must not be destroyed. Use cmzn_mesh_access() to add a reference if
 * maintaining returned handle beyond the lifetime of the mesh_group.
 * Use this function to call base-class API, e.g.:
 * int size = cmzn_mesh_get_size(cmzn_mesh_group_base_cast(mesh_group);
 *
 * @param mesh_group  Handle to the mesh group to cast.
 * @return  Non-accessed handle to the mesh or NULL if failed.
 */
ZINC_C_INLINE cmzn_mesh_id cmzn_mesh_group_base_cast(
	cmzn_mesh_group_id mesh_group)
{
	return (cmzn_mesh_id)(mesh_group);
}

/**
 * Add specified element to mesh group.
 *
 * @param mesh_group  Handle to mesh group to modify.
 * @param element  Handle to element to add. Must be from the group's master mesh.
 * @return  Status CMZN_OK on success, CMZN_ERROR_ALREADY_EXISTS if element
 * was already in the group but otherwise successful, or any other value on
 * more serious failure.
 */
ZINC_API int cmzn_mesh_group_add_element(cmzn_mesh_group_id mesh_group,
	cmzn_element_id element);

/**
 * Ensure this mesh group contains all elements from the master mesh for which
 * the conditional field is true i.e. non-zero valued in the element.
 * Results are undefined if conditional field is not constant over element.
 * Note that group and element_group fields are valid conditional fields.
 *
 * @param mesh_group  Handle to the mesh group to add elements to.
 * @param conditional_field  Field which if non-zero in an element indicates it
 * is to be added.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
ZINC_API int cmzn_mesh_group_add_elements_conditional(cmzn_mesh_group_id mesh_group,
	cmzn_field_id conditional_field);

/**
 * Remove all elements from mesh group.
 *
 * @param mesh_group  Handle to mesh group to modify.
 * @return  Status CMZN_OK if all elements removed, any other value if failed.
 */
ZINC_API int cmzn_mesh_group_remove_all_elements(cmzn_mesh_group_id mesh_group);

/**
 * Remove specified element from mesh group.
 *
 * @param mesh_group  Handle to mesh group to modify.
 * @param element  Handle to element to remove.
 * @return  Status CMZN_OK on success, CMZN_ERROR_NOT_FOUND if element was not
 * in the group but otherwise successful, or any other value on more serious
 * failure.
 */
ZINC_API int cmzn_mesh_group_remove_element(cmzn_mesh_group_id mesh_group,
	cmzn_element_id element);

/**
 * Remove all elements from the mesh group for which the conditional field is
 * true i.e. non-zero valued in the element.
 * Results are undefined if conditional field is not constant over element.
 * Note that group and element_group fields are valid conditional fields.
 *
 * @param mesh_group  Handle to the mesh group to remove elements from.
 * @param conditional_field  Field which if non-zero in the element indicates it
 * is to be removed.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
ZINC_API int cmzn_mesh_group_remove_elements_conditional(cmzn_mesh_group_id mesh_group,
	cmzn_field_id conditional_field);

/**
 * Returns a new handle to the mesh changes with reference count incremented.
 *
 * @param meshchanges  The mesh changes to obtain a new handle to.
 * @return  New handle to mesh changes, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_meshchanges_id cmzn_meshchanges_access(
	cmzn_meshchanges_id meshchanges);

/**
 * Destroys this handle to the meshchanges and sets it to NULL.
 * Internally this decrements the reference count.
 *
 * @param meshchanges_address  Address of handle to the mesh changes to destroy.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
ZINC_API int cmzn_meshchanges_destroy(cmzn_meshchanges_id *meshchanges_address);

/**
 * Returns if and how the element has changed in the mesh changes. Note that
 * individual changes are not stored for each element, hence this function
 * returns the summary of all element changes if the element has changed, and
 * CHANGE_FLAG_NONE if not changed.
 *
 * @param meshchanges  The mesh changes to query.
 * @param element  The element to query about.
 * @return  Logical OR of change bit flags. See #cmzn_element_change_flag.
 */
ZINC_API cmzn_element_change_flags cmzn_meshchanges_get_element_change_flags(
	cmzn_meshchanges_id meshchanges, cmzn_element_id element);

/**
 * Returns the number of elements recorded as changed in the mesh changes, or
 * -1 if all considered changed, i.e. too many changes occurred to record.
 *
 * @param meshchanges  The mesh changes to query.
 * @return  The number of changes to elements, or -1 if all changed.
 */
ZINC_API int cmzn_meshchanges_get_number_of_changes(
	cmzn_meshchanges_id meshchanges);

/**
 * Returns logical OR of change flags for all elements in the mesh changes.
 *
 * @param meshchanges  The mesh changes to query.
 * @return  Logical OR of change bit flags. See #cmzn_element_change_flag.
 */
ZINC_API cmzn_element_change_flags cmzn_meshchanges_get_summary_element_change_flags(
	cmzn_meshchanges_id meshchanges);

#ifdef __cplusplus
}
#endif

#endif /* CMZN_MESH_H__ */
