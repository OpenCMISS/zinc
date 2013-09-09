/**
 * FILE : fieldarithmeticoperators.h
 *
 * The public interface to the cmzn_fields that perform arithmetic operators.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef CMZN_FIELDARITHMETICOPERATORS_H__
#define CMZN_FIELDARITHMETICOPERATORS_H__

#include "types/fieldid.h"
#include "types/fieldmoduleid.h"

#include "zinc/zincsharedobject.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a field where the which calculates the components of source_field_one
 * raised to the power of the components in source_field_two.
 * Automatic scalar broadcast will apply, see field.h.
 *
 * @param field_module  Region field module which will own new field.
 * @param source_field_one First input field
 * @param source_field_two Second input field
 * @return Newly created field
 */
ZINC_API cmzn_field_id cmzn_field_module_create_power(cmzn_field_module_id field_module,
	cmzn_field_id source_field_one,
	cmzn_field_id source_field_two);

/**
 * Creates a field where the which multiplies the components of source_field_one and
 * source_field_two.
 * Automatic scalar broadcast will apply, see field.h.
 *
 * @param field_module  Region field module which will own new field.
 * @param source_field_one First input field
 * @param source_field_two Second input field
 * @return Newly created field
 */
ZINC_API cmzn_field_id cmzn_field_module_create_multiply(cmzn_field_module_id field_module,
	cmzn_field_id source_field_one,
	cmzn_field_id source_field_two);

/**
 * Creates a field where the which divides the components of source_field_one
 * by source_field_two.
 * Automatic scalar broadcast will apply, see field.h.
 *
 * @param field_module  Region field module which will own new field.
 * @param source_field_one First input field
 * @param source_field_two Second input field
 * @return Newly created field
 */
ZINC_API cmzn_field_id cmzn_field_module_create_divide(cmzn_field_module_id field_module,
	cmzn_field_id source_field_one,
	cmzn_field_id source_field_two);

/**
 * Creates a field where the which adds the components of source_field_one and
 * source_field_two.
 * Automatic scalar broadcast will apply, see field.h.
 *
 * @param field_module  Region field module which will own new field.
 * @param source_field_one First input field
 * @param source_field_two Second input field
 * @return Newly created field
 */
ZINC_API cmzn_field_id cmzn_field_module_create_add(cmzn_field_module_id field_module,
	cmzn_field_id source_field_one,
	cmzn_field_id source_field_two);

/**
 * Creates a field where the which subtracts the components of source_field_two
 * from source_field_one.
 * Automatic scalar broadcast will apply, see field.h.
 *
 * @param field_module  Region field module which will own new field.
 * @param source_field_one First input field
 * @param source_field_two Second input field
 * @return Newly created field
 */
ZINC_API cmzn_field_id cmzn_field_module_create_subtract(cmzn_field_module_id field_module,
	cmzn_field_id source_field_one,
	cmzn_field_id source_field_two);

/**
 * Creates a field where the field components are the natural logarithm
 * of each component in the source field.
 *
 * @param field_module  Region field module which will own new field.
 * @param source_field The input field
 * @return Newly created field
 */
ZINC_API cmzn_field_id cmzn_field_module_create_log(cmzn_field_module_id field_module,
	cmzn_field_id source_field);

/**
 * Creates a field where the field components are the square root
 * of each component in the source field.
 *
 * @param field_module  Region field module which will own new field.
 * @param source_field The input field
 * @return Newly created field
 */
ZINC_API cmzn_field_id cmzn_field_module_create_sqrt(cmzn_field_module_id field_module,
	cmzn_field_id source_field);

/**
 * Creates a field where the field components are the natural exponent
 * of each component in the source field.
 *
 * @param field_module  Region field module which will own new field.
 * @param source_field The input field
 * @return Newly created field
 */
ZINC_API cmzn_field_id cmzn_field_module_create_exp(cmzn_field_module_id field_module,
	cmzn_field_id source_field);

/**
 * Creates a field where the field components are the absolute value of each
 * component in the source field.
 *
 * @param field_module  Region field module which will own new field.
 * @param source_field The input field
 * @return Newly created field
 */
ZINC_API cmzn_field_id cmzn_field_module_create_abs(cmzn_field_module_id field_module,
	cmzn_field_id source_field);

#ifdef __cplusplus
}
#endif

#endif /* CMZN_FIELDARITHMETICOPERATORS_H__ */
