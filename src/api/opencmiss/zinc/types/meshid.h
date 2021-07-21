/**
 * @file meshid.h
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CMZN_MESHID_H__
#define CMZN_MESHID_H__

/**
 * @brief A finite element mesh consisting of a set of elements of fixed dimension.
 *
 * A finite element mesh consisting of a set of elements of fixed dimension.
 * Note that Zinc elements are not iso-parametric, meaning each field must
 * be individually defined on them, specifying the basis and parameter mapping.
 */
struct cmzn_mesh;
typedef struct cmzn_mesh *cmzn_mesh_id;

/**
 * @brief A subset of a master mesh.
 *
 * A specialised mesh consisting of a subset of elements from a master mesh.
 */
struct cmzn_mesh_group;
typedef struct cmzn_mesh_group *cmzn_mesh_group_id;

/**
 * @brief Object describing changes to a mesh in a fieldmoduleevent
 *
 * Object describing changes to a mesh in a fieldmoduleevent
 */
struct cmzn_meshchanges;
typedef struct cmzn_meshchanges *cmzn_meshchanges_id;

#endif
