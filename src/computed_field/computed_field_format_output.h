/*******************************************************************************
FILE : computed_field_format_output.h

LAST MODIFIED : 14 December 2010

DESCRIPTION :
Implements computed fields that control the format_output behaviour.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (COMPUTED_FIELD_FORMAT_OUTPUT_H)
#define COMPUTED_FIELD_FORMAT_OUTPUT_H

/*****************************************************************************//**
 * Creates a string field with the components of the source field rendered using
 * the format_string. 
 * 
 * @param field_module  Region field module which will own new field.
 * @param source_field  The field values to be rendered to a string
 * @param format_string  The C style rendering string that converts the ZnReal values
 * of the source_field to string.  Some checking of the format_string is done to ensure
 * it matches the number of values in the source field.
 * @return Newly created field
 */
cmzn_field *cmzn_fieldmodule_create_field_format_output(
	cmzn_fieldmodule *fieldmodule,
	cmzn_field *source_field, char *format_string);

#endif /* !defined (COMPUTED_FIELD_FORMAT_OUTPUT_H) */
