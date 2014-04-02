/**
 * @file fieldfiniteelement.h
 *
 * Implements fields interpolated over finite element meshes with
 * parameters indexed by nodes.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef CMZN_FIELDFINITEELEMENT_H__
#define CMZN_FIELDFINITEELEMENT_H__

#include "types/elementid.h"
#include "types/fieldid.h"
#include "types/fieldfiniteelementid.h"
#include "types/fieldmoduleid.h"
#include "types/nodeid.h"

#include "zinc/zincsharedobject.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a real-valued finite_element field which can be interpolated over a
 * finite element mesh with parameters indexed by nodes.
 *
 * @param field_module  Region field module which will own new field.
 * @param number_of_components  The number of components for the new field.
 * @return  Handle to new field, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_field_id cmzn_fieldmodule_create_field_finite_element(
	cmzn_fieldmodule_id field_module, int number_of_components);

/**
 * If the field is real-valued interpolated finite element then this function
 * returns the finite_element specific representation, otherwise returns NULL.
 * Caller is responsible for destroying the returned derived field handle.
 *
 * @param field  The field to be cast.
 * @return  Handle to derived finite element field, or NULL/invalid handle if wrong type or failed.
 */
ZINC_API cmzn_field_finite_element_id cmzn_field_cast_finite_element(cmzn_field_id field);

/**
 * Cast finite element field back to its base field and return the field.
 * IMPORTANT NOTE: Returned field does not have incremented reference count and
 * must not be destroyed. Use cmzn_field_access() to add a reference if
 * maintaining returned handle beyond the lifetime of the derived field.
 * Use this function to call base-class API, e.g.:
 * cmzn_field_set_name(cmzn_field_derived_base_cast(derived_field), "bob");
 *
 * @param finite_element_field  Handle to the finite element field to cast.
 * @return  Non-accessed handle to the base field or NULL if failed.
 */
ZINC_C_INLINE cmzn_field_id cmzn_field_finite_element_base_cast(
	cmzn_field_finite_element_id finite_element_field)
{
	return (cmzn_field_id)(finite_element_field);
}

/**
 * Destroys handle to the finite_element field (and sets it to NULL).
 * Internally this decrements the reference count.
 *
 * @param finite_element_field_address  Address of handle to the field to
 * 		destroy.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
ZINC_API int cmzn_field_finite_element_destroy(
	cmzn_field_finite_element_id *finite_element_field_address);

/**
 * Creates a field returning a value of a source field at an embedded location.
 * The new field has the same value type as the source field.
 *
 * @param field_module  Region field module which will own new field.
 * @param source_field  Field to evaluate at the embedded location. Currently
 * restricted to having numerical values.
 * @param embedded_location_field  Field returning an embedded location, i.e.
 * find_mesh_location or stored mesh location fields.
 * @return  Handle to new field, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_field_id cmzn_fieldmodule_create_field_embedded(
	cmzn_fieldmodule_id field_module, cmzn_field_id source_field,
	cmzn_field_id embedded_location_field);

/**
 * Creates a field returning the location in a mesh at which the calculated
 * source_field value equals the mesh_field value. Type-specific functions allow
 * the search to find the nearest value and set a conditional field.
 *
 * @param field_module  Region field module which will own new field.
 * @param source_field  Source field whose value is to be searched for. Must have
 * the same number of numerical components as the mesh_field, and at least as
 * many as mesh dimension.
 * @param mesh_field  Field defined over the mesh which is to be matched with
 * source_field. Must have the same number of numerical components as the
 * source_field, and at least as many as mesh dimension.
 * @param mesh  The mesh to find locations in.
 * @return  Handle to new field, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_field_id cmzn_fieldmodule_create_field_find_mesh_location(
	cmzn_fieldmodule_id field_module, cmzn_field_id source_field,
	cmzn_field_id mesh_field, cmzn_mesh_id mesh);

/**
 * If the field is of type find_mesh_location then this function returns the
 * find_mesh_location specific representation, otherwise returns NULL.
 * Caller is responsible for destroying the returned derived field handle.
 *
 * @param field  The field to be cast.
 * @return  Handle to derived find mesh location field, or NULL/invalid handle if wrong type or failed.
 */
ZINC_API cmzn_field_find_mesh_location_id cmzn_field_cast_find_mesh_location(
	cmzn_field_id field);

/**
 * Cast find_mesh_location field back to its base field and return the field.
 * IMPORTANT NOTE: Returned field does not have incremented reference count and
 * must not be destroyed. Use cmzn_field_access() to add a reference if
 * maintaining returned handle beyond the lifetime of the derived field.
 * Use this function to call base-class API, e.g.:
 * cmzn_field_set_name(cmzn_field_derived_base_cast(derived_field), "bob");
 *
 * @param find_mesh_location_field  Handle to the find_mesh_location field to cast.
 * @return  Non-accessed handle to the base field or NULL if failed.
 */
ZINC_C_INLINE cmzn_field_id cmzn_field_find_mesh_location_base_cast(
	cmzn_field_find_mesh_location_id find_mesh_location_field)
{
	return (cmzn_field_id)(find_mesh_location_field);
}

/**
 * Destroys handle to the find_mesh_location field and sets it to NULL.
 * Internally this decrements the reference count.
 *
 * @param find_mesh_location_field_address  Address of handle to the field to
 * 		destroy.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
ZINC_API int cmzn_field_find_mesh_location_destroy(
	cmzn_field_find_mesh_location_id *find_mesh_location_field_address);

/**
 * Returns the mesh the find_mesh_location field is to find locations in.
 *
 * @param find_mesh_location_field  The field to query.
 * @return  Handle to the mesh, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_mesh_id cmzn_field_find_mesh_location_get_mesh(
	cmzn_field_find_mesh_location_id find_mesh_location_field);

/**
 * Convert a short name into an enum if the name matches any of the members in
 * the enum.
 *
 * @param string  string of the short enumerator name
 * @return  the correct enum type if a match is found.
 */
ZINC_API enum cmzn_field_find_mesh_location_search_mode
	cmzn_field_find_mesh_location_search_mode_enum_from_string(const char *string);

/**
 * Return an allocated short name of the enum type from the provided enum.
 * User must call cmzn_deallocate to destroy the successfully returned string.
 *
 * @param format  enum to be converted into string
 * @return  an allocated string which stored the short name of the enum.
 */
ZINC_API char *cmzn_field_find_mesh_location_search_mode_enum_to_string(
	enum cmzn_field_find_mesh_location_search_mode mode);

/**
 * Gets the search mode for the find_mesh_location field: whether finding
 * location with exact or nearest value.
 *
 * @param find_mesh_location_field  The field to query.
 * @return  The search mode.
 */
ZINC_API enum cmzn_field_find_mesh_location_search_mode
	cmzn_field_find_mesh_location_get_search_mode(
		cmzn_field_find_mesh_location_id find_mesh_location_field);

/**
 * Sets the search mode for the find_mesh_location field: whether finding
 * location with exact or nearest value.
 *
 * @param find_mesh_location_field  The field to modify.
 * @param search_mode  The search mode to set.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
ZINC_API int cmzn_field_find_mesh_location_set_search_mode(
	cmzn_field_find_mesh_location_id find_mesh_location_field,
	enum cmzn_field_find_mesh_location_search_mode search_mode);

/**
 * Creates a field which represents and returns node values/derivatives.
 *
 * @param field_module  Region field module which will own new field.
 * @param field  The field for which the nodal values are stored, this
 * 	must be a finite element field.
 * @param node_value_label  The label of the node value/derivative to return.
 * @param version_number  The version number of the value or derivative to
 * return, starting from 1.
 * @return  Handle to new field, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_field_id cmzn_fieldmodule_create_field_node_value(
	cmzn_fieldmodule_id field_module, cmzn_field_id field,
	enum cmzn_node_value_label node_value_label, int version_number);

/**
 * Creates a field which stores and returns mesh location values at nodes.
 *
 * @param field_module  Region field module which will own new field.
 * @param mesh  The mesh for which locations are stored.
 * @return  Handle to new field, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_field_id cmzn_fieldmodule_create_field_stored_mesh_location(
	cmzn_fieldmodule_id field_module, cmzn_mesh_id mesh);

/**
 * If the field is stored_mesh_location type, return type-specific handle to it.
 * Caller is responsible for destroying the returned derived field handle.
 *
 * @param field  The field to be cast.
 * @return  Handle to derived stored mesh location field, or NULL/invalid handle if wrong type or failed.
 */
ZINC_API cmzn_field_stored_mesh_location_id cmzn_field_cast_stored_mesh_location(cmzn_field_id field);

/**
 * Cast stored_mesh_location field back to its base field and return the field.
 * IMPORTANT NOTE: Returned field does not have incremented reference count and
 * must not be destroyed. Use cmzn_field_access() to add a reference if
 * maintaining returned handle beyond the lifetime of the derived field.
 * Use this function to call base-class API, e.g.:
 * cmzn_field_set_name(cmzn_field_derived_base_cast(derived_field), "bob");
 *
 * @param stored_mesh_location_field  Handle to the field to cast.
 * @return  Non-accessed handle to the base field or NULL if failed.
 */
ZINC_C_INLINE cmzn_field_id cmzn_field_stored_mesh_location_base_cast(
	cmzn_field_stored_mesh_location_id stored_mesh_location_field)
{
	return (cmzn_field_id)(stored_mesh_location_field);
}

/**
 * Destroys handle to the stored_mesh_location field (and sets it to
 * NULL). Internally this decrements the reference count.
 *
 * @param stored_mesh_location_field_address  Address of handle to the field to
 * 		destroy.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
ZINC_API int cmzn_field_stored_mesh_location_destroy(
	cmzn_field_stored_mesh_location_id *stored_mesh_location_field_address);

/**
 * Creates a field which stores and returns string values at nodes.
 *
 * @param field_module  Region field module which will own new field.
 * @return  Handle to new field, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_field_id cmzn_fieldmodule_create_field_stored_string(
	cmzn_fieldmodule_id field_module);

/**
 * If the field is stored_string type, return type-specific handle to it.
 * Caller is responsible for destroying the returned derived field handle.
 *
 * @param field  The field to be cast.
 * @return  Handle to derived stored string field, or NULL/invalid handle if wrong type or failed.
 */
ZINC_API cmzn_field_stored_string_id cmzn_field_cast_stored_string(cmzn_field_id field);

/**
 * Cast stored_string field back to its base field and return the field.
 * IMPORTANT NOTE: Returned field does not have incremented reference count and
 * must not be destroyed. Use cmzn_field_access() to add a reference if
 * maintaining returned handle beyond the lifetime of the derived field.
 * Use this function to call base-class API, e.g.:
 * cmzn_field_set_name(cmzn_field_derived_base_cast(derived_field), "bob");
 *
 * @param stored_string_field  Handle to the field to cast.
 * @return  Non-accessed handle to the base field or NULL if failed.
 */
ZINC_C_INLINE cmzn_field_id cmzn_field_stored_string_base_cast(
	cmzn_field_stored_string_id stored_string_field)
{
	return (cmzn_field_id)(stored_string_field);
}

/**
 * Destroys handle to the stored_string field (and sets it to NULL).
 * Internally this decrements the reference count.
 *
 * @param stored_string_field_address  Address of handle to the field to
 * 		destroy.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
ZINC_API int cmzn_field_stored_string_destroy(
	cmzn_field_stored_string_id *stored_string_field_address);

#ifdef __cplusplus
}
#endif

#endif
