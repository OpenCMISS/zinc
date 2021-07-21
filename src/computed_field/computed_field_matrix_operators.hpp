/***************************************************************************//**
 * FILE : computed_field_matrix_operators.hpp
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (COMPUTED_FIELD_MATRIX_OPERATORS_HPP)
#define COMPUTED_FIELD_MATRIX_OPERATORS_HPP

int cmzn_field_matrix_multiply_get_number_of_rows(cmzn_field_id field);

int cmzn_field_transpose_get_source_number_of_rows(cmzn_field_id field);

#endif /* !defined (COMPUTED_FIELD_MATRIX_OPERATORS_HPP) */
