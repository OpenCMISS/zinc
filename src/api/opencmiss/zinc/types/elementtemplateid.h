/**
 * @file elementtemplateid.h
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CMZN_ELEMENTTEMPLATEID_H__
#define CMZN_ELEMENTTEMPLATEID_H__

/**
 * @brief A description of element shape and field definitions.
 *
 * A description of element shape and field definitions (incl. basis, parameter
 * mappings), used as a template for creating new elements in a mesh, or merging
 * into an element to define additional fields on it.
 */
struct cmzn_elementtemplate;
typedef struct cmzn_elementtemplate *cmzn_elementtemplate_id;

#endif
