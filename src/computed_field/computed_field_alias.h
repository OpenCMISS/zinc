/*****************************************************************************//**
 * FILE : computed_field_alias.h
 *
 * Implements a cmiss field which is an alias for another field, commonly from a
 * different region to make it available locally.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (COMPUTED_FIELD_ALIAS_H)
#define COMPUTED_FIELD_ALIAS_H

#include "opencmiss/zinc/fieldalias.h"

/*****************************************************************************//**
 * Sets up command data for alias field.
 *
 * @param computed_field_package  Container for command data.
 * @param root_region  Root region for getting paths to fields in other regions.
 * @return 1 on success, 0 on failure.
 */
int Computed_field_register_type_alias(
	struct Computed_field_package *computed_field_package,
	struct cmzn_region *root_region);

#endif /* !defined (COMPUTED_FIELD_ALIAS_H) */
