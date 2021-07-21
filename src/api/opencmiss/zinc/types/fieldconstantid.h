/**
 * @file fieldconstantid.h
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CMZN_FIELDCONSTANTID_H__
#define CMZN_FIELDCONSTANTID_H__

/**
 * @brief A field returning a real constant with one or more components.
 *
 * A field returning one or more real constant values.
 */
struct cmzn_field_constant;
typedef struct cmzn_field_constant *cmzn_field_constant_id;

/**
 * @brief A field returning a constant string.
 *
 * A field returning a single constant string.
 */
struct cmzn_field_string_constant;
typedef struct cmzn_field_string_constant *cmzn_field_string_constant_id;

#endif
