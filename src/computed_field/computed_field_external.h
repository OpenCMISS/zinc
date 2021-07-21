/*******************************************************************************
FILE : computed_field_external.h

LAST MODIFIED : 23 January 2002

DESCRIPTION :
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (COMPUTED_FIELD_EXTERNAL_H)
#define COMPUTED_FIELD_EXTERNAL_H

int Computed_field_set_type_external(struct Computed_field *field,
	char *filename, int timeout,
	int number_of_source_values, FE_value *source_values,
	int number_of_source_fields, struct Computed_field **source_fields);
/*******************************************************************************
LAST MODIFIED : 23 January 2002

DESCRIPTION :
Converts <field> to type COMPUTED_FIELD_EXTERNAL.
<source_fields> must point to an array of <number_of_sources> pointers to
Computed_fields. The resulting field will have as many
components as <number_of_sources> * components + number_of_source_values.
If function fails, field is guaranteed to be unchanged from its original state,
although its cache may be lost.
==============================================================================*/

int Computed_field_get_type_external(struct Computed_field *field,
	char **filename, int *timeout,
	int *number_of_source_values, FE_value **source_values,
	int *number_of_source_fields,struct Computed_field ***source_fields);
/*******************************************************************************
LAST MODIFIED : 23 January 2002

DESCRIPTION :
If the field is of type COMPUTED_FIELD_EXTERNAL, the function allocates and
returns in <**source_fields> an array containing the <number_of_sources> source
fields making up the composite field - otherwise an error is reported.
It is up to the calling function to DEALLOCATE the returned array. Note that the
fields in the returned array are not ACCESSed.
==============================================================================*/

#endif /* !defined (COMPUTED_FIELD_EXTERNAL_H) */
