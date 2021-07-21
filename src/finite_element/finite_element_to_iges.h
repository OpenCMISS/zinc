/*******************************************************************************
FILE : finite_element_to_iges.h

LAST MODIFIED : 4 March 2003

DESCRIPTION :
Functions for building IGES information from finite elements and exporting
to file.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (FINITE_ELEMENT_TO_IGES_H)
#define FINITE_ELEMENT_TO_IGES_H

/*
Global functions
----------------
*/

struct cmzn_region;
struct cmzn_field;

/**
 * Write bicubic elements to an IGES file.
 */
int export_to_iges(char *file_name, struct cmzn_region *region,
	char *region_path, struct cmzn_field *field);

#endif /* !defined (FINITE_ELEMENT_TO_IGES_H) */
