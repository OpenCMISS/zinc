/**
 * @file fieldparametersid.h
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CMZN_FIELDPARAMETERSID_H__
#define CMZN_FIELDPARAMETERSID_H__

/**
 * @brief Interface to parameters of a field.
 *
 * Interface to parameters of a field allowing solvers to index them,
 * and to access derivative operators with respect to element parameters.
 * Note that only node-based field parameters are supported at present.
 */
struct cmzn_fieldparameters;
typedef struct cmzn_fieldparameters *cmzn_fieldparameters_id;

#endif
