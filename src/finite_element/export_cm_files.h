/**
 * FILE : export_cm_files.h
 *
 * Functions for exporting finite element data to CMISS IP files.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (EXPORT_CM_FILES_H)
#define EXPORT_CM_FILES_H

#include <stdio.h>
#include "opencmiss/zinc/types/fieldid.h"
#include "opencmiss/zinc/types/fieldgroupid.h"
#include "opencmiss/zinc/types/regionid.h"

/*
Global/Public functions
-----------------------
*/

/**
 * Writes the set of <ipcoor_file>, <ipbase_file>, <ipnode_file> and
 * <ipelem_file> that defines elements of <field> in <region>.
 * The <ipmap_file> is optional, all the others are required.
 * @param region  Region from which to export.
 * @param group  Optional group within region to restrict output to.
 * @param field  Finite element field to export.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
int write_cm_files(FILE *ipcoor_file, FILE *ipbase_file,
	FILE *ipnode_file, FILE *ipelem_file, FILE *ipmap_file,
	cmzn_region *region, cmzn_field_group *group,
	cmzn_field *field);

#endif /* !defined (EXPORT_CM_FILES_H) */
