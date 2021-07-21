/**
 * @file node.h
 *
 * The public interface to finite element nodes.
 * Nodes are also used for other point domains e.g. data points.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef CMZN_NODE_H__
#define CMZN_NODE_H__

#include "types/nodeid.h"
#include "types/nodesetid.h"
#include "types/nodetemplateid.h"

#include "opencmiss/zinc/zincsharedobject.h"

/*
Global functions
----------------
*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Convert a short name into an enum if the name matches any of the members in
 * the enum.
 *
 * @param string  string of the short enumerator name
 * @return  the correct enum type if a match is found.
 */
ZINC_API enum cmzn_node_value_label cmzn_node_value_label_enum_from_string(
	const char *string);

/**
 * Return an allocated short name of the enum type from the provided enum.
 * User must call cmzn_deallocate to destroy the successfully returned string.
 *
 * @param type  enum to be converted into string
 * @return  an allocated string which stored the short name of the enum.
 */
ZINC_API char *cmzn_node_value_label_enum_to_string(enum cmzn_node_value_label type);

/**
 * Returns a new handle to the node iterator with reference count incremented.
 *
 * @param node_iterator  The node iterator to obtain a new handle to.
 * @return  New handle to the node iterator, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_nodeiterator_id cmzn_nodeiterator_access(
	cmzn_nodeiterator_id node_iterator);

/**
 * Destroys this handle to the node_iterator and sets it to NULL.
 *
 * @param node_iterator_address  Address of handle to node_iterator to destroy.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
ZINC_API int cmzn_nodeiterator_destroy(cmzn_nodeiterator_id *node_iterator_address);

/**
 * Returns a handle to the next node in the container being iterated over then
 * advances the iterator position. The caller is required to destroy the
 * returned node handle.
 *
 * @param node_iterator  Node iterator to query and advance.
 * @return  Handle to the next node, or NULL/invalid handle if none remaining or failed.
 */
ZINC_API cmzn_node_id cmzn_nodeiterator_next(cmzn_nodeiterator_id node_iterator);

/**
 * Returns a new handle to the node with reference count incremented.
 *
 * @param node  The node to obtain a new handle to.
 * @return  New handle to the node, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_node_id cmzn_node_access(cmzn_node_id node);

/**
 * Destroys this handle to the node and sets it to NULL.
 * Internally this decrements the reference count.
 *
 * @param node_address  Address of handle to the node to destroy.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
ZINC_API int cmzn_node_destroy(cmzn_node_id *node_address);

/**
 * Returns the non-negative integer uniquely identifying the node in its
 * nodeset.
 *
 * @param node  The node to query.
 * @return  The non-negative integer identifier of the node, or a negative
 * value if node is invalid.
 */
ZINC_API int cmzn_node_get_identifier(cmzn_node_id node);

/**
 * Set an integer uniquely identifying the node in its nodeset.
 *
 * @param node  The node to query.
 * @param identifier  unique identifier to be set for the node
 *
 * @return  CMZN_OK if set the identifier successfully,
 * 	any other value on fail.
 */
ZINC_API int cmzn_node_set_identifier(cmzn_node_id node, int identifier);

/**
 * Get the nodeset which owns this node.
 *
 * @param node  The node to query.
 * @return  Handle to the owning nodeset, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_nodeset_id cmzn_node_get_nodeset(cmzn_node_id node);

/**
 * Modifies the node to define fields as described in the node_template.
 *
 * @param node  The node to modify.
 * @param node_template  Template containing node field descriptions.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
ZINC_API int cmzn_node_merge(cmzn_node_id node, cmzn_nodetemplate_id node_template);

#ifdef __cplusplus
}
#endif

#endif // CMZN_NODE_H__
