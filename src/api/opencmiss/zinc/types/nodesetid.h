/**
 * @file nodesetid.h
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CMZN_NODESETID_H__
#define CMZN_NODESETID_H__

/**
 * @brief A set of nodes or points.
 *
 * A set of nodes or points, equivalent to a zero-dimensional mesh.
 */
struct cmzn_nodeset;
typedef struct cmzn_nodeset *cmzn_nodeset_id;

/**
 * @brief A subset of a master nodeset.
 *
 * A specialised nodeset consisting of a subset of nodes from a master nodeset.
 */
struct cmzn_nodeset_group;
typedef struct cmzn_nodeset_group *cmzn_nodeset_group_id;

/**
* @brief Object describing changes to a nodeset in a fieldmoduleevent.
*
* Object describing changes to a nodeset in a fieldmoduleevent
*/
struct cmzn_nodesetchanges;
typedef struct cmzn_nodesetchanges *cmzn_nodesetchanges_id;

#endif
