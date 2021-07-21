/**
 * @file nodetemplateid.h
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CMZN_NODETEMPLATEID_H__
#define CMZN_NODETEMPLATEID_H__

/**
 * @brief A description of field parameters to define at a node.
 *
 * A description of field parameters to define at a node (incl. value/derivative
 * types, versions), used as a template for creating new nodes in a nodeset, or
 * merging into a node to define or undefine fields on it.
 */
struct cmzn_nodetemplate;
typedef struct cmzn_nodetemplate *cmzn_nodetemplate_id;

#endif
