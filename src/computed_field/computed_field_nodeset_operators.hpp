/***************************************************************************//**
 * FILE : computed_field_nodeset_operators.hpp
 *
 * Field operators that act on a nodeset.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (COMPUTED_FIELD_NODESET_OPERATORS_HPP)
#define COMPUTED_FIELD_NODESET_OPERATORS_HPP

/** @return 1 if field is stored mesh location type suitable for using as an
 * element evaluation map in nodeset operators */
int cmzn_field_is_valid_nodeset_operator_element_map(cmzn_field_id field, void *);

#endif /* !defined (COMPUTED_FIELD_NODESET_OPERATORS_HPP) */
