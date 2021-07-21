/*******************************************************************************
FILE : computed_field_function.h

LAST MODIFIED : 31 March 2008

DESCRIPTION :
Implements a "function" computed_field which returns the values of
<result_field> with respect to the <source_field> values 
being the inputs for <reference_field>.
The sequence of operations <reference_field> to <result_field> 
become a function operating on the input <source_field> values.
which converts fields, field components
and real values in any order into a single vector field.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (COMPUTED_FIELD_FUNCTION_H)
#define COMPUTED_FIELD_FUNCTION_H

/***************************************************************************//**
 * Converts a "function" type field which returns the values of <result_field>
 * with respect to the <source_field> values being the inputs for
 * <reference_field>.
 * The sequence of operations <reference_field> to <result_field>
 * becomes a function operating on the input <source_field> values.
 * Either the number of components in the <source_field> and <reference_field>
 * should be the same, and then the number of components of this <field>
 * will be the same as the number of components in the <result_field>,
 * or if the <reference_field> and <result_field> are scalar then the
 * function operation will be applied as many times as required for each
 * component in the <source_field> and then this <field> will have as many
 * components as the <source_field>.
 */
cmzn_field *cmzn_fieldmodule_create_field_function(
	struct cmzn_fieldmodule *fieldmodule,
	cmzn_field *source_field, cmzn_field *result_field,
	cmzn_field *reference_field);

int Computed_field_get_type_function(struct Computed_field *field,
	struct Computed_field **source_field, struct Computed_field **result_field,
	struct Computed_field **reference_field);
/*******************************************************************************
LAST MODIFIED : 31 March 2008

DESCRIPTION :
If the field is of type COMPUTED_FIELD_FUNCTION, the function returns the three
fields which define the field.
==============================================================================*/

#endif /* !defined (COMPUTED_FIELD_FUNCTION_H) */
