/**
 * @file fieldnodesetoperatorsid.h
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CMZN_FIELDNODESETOPERATORSID_H__
#define CMZN_FIELDNODESETOPERATORSID_H__

/**
 * @brief  Base class of fields performing sum and other operators over a nodeset.
 *
 * Base class of fields performing sum, mean, minimum, maximum operators over a
 * a nodeset. These also have the option to evaluate at an element by
 * limiting the summation to nodes from the nodeset that have embedded
 * locations in the element provided by a stored mesh location field.
 */
struct cmzn_field_nodeset_operator;
typedef struct cmzn_field_nodeset_operator *cmzn_field_nodeset_operator_id;

#endif
