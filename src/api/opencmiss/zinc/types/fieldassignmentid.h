/**
 * @file fieldassignmentid.h
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CMZN_FIELDASSIGNMENTID_H__
#define CMZN_FIELDASSIGNMENTID_H__

/**
 * @brief  Object for assigning values of a field from a source field.
 *
 * Object performing assignment of a source field to a target field so that the
 * target field will afterwards give as near as possible the same values over
 * its domain as the source.
 * Beware this object may not handle all cases, notably element-based fields.
 * It is initially designed to assign node parameters, and normally only the
 * node field value is assigned to.
 * If the target field is real-valued finite element type then three special
 * cases are handled:
 * 1. If the source field is finite element type, derivatives and versions
 * are copied directly. Only derivatives/versions present in both source and
 * target field at the node are evaluated and assigned.
 * 2. If the source field is a function of the target field, it attempts to
 * assign appropriately to different value versions and derivatives, latter
 * by computing the gradient of the source w.r.t. target. In both cases only
 * simple functions of target field value are currently expected to work.
 * 3. If the source field is not a function of the target field, the source
 * field values are simply evaluated and assigned to all versions of the value
 * parameter of the target field at the node.
 */
struct cmzn_fieldassignment;
typedef struct cmzn_fieldassignment *cmzn_fieldassignment_id;

#endif
