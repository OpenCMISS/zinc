/**
 * @file fieldcad.h
 * 
 * The public interface to zinc fields which wrap cad entities.
 * Warning: prototype!
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CMZN_FIELDCAD_H__
#define CMZN_FIELDCAD_H__

#include "types/field_id.h"
#include "types/field_module_id.h"
#include "types/region_id.h"
#include "types/field_cad_id.h"
#include "types/field_group_id.h"

int cmzn_region_import_cad_file(cmzn_region_id region, const char *file_name);

struct cmzn_cad_identifier;
typedef struct cmzn_cad_identifier *cmzn_cad_identifier_id;

struct cmzn_field_cad_primitive_group_template;
typedef struct cmzn_field_cad_primitive_group_template *cmzn_field_cad_primitive_group_template_id;

cmzn_field_id cmzn_field_module_create_cad_primitive_group_template(cmzn_field_module_id field_module);

int cmzn_field_cad_primitive_group_template_destroy(cmzn_field_cad_primitive_group_template_id *cad_primitive_group_address);

cmzn_field_cad_primitive_group_template_id cmzn_field_cast_cad_primitive_group_template(cmzn_field_id field);

int cmzn_field_cad_primitive_group_template_add_cad_primitive(cmzn_field_cad_primitive_group_template_id cad_primitive_group,
	cmzn_cad_identifier_id cad_primitive);

int cmzn_field_cad_primitive_group_template_remove_cad_primitive(cmzn_field_cad_primitive_group_template_id cad_primitive_group,
	cmzn_cad_identifier_id cad_primitive);

int cmzn_field_cad_primitive_group_template_clear(cmzn_field_cad_primitive_group_template_id cad_primitive_group);

int cmzn_field_cad_primitive_group_template_is_cad_primitive_selected(
	cmzn_field_cad_primitive_group_template_id cad_primitive_group, cmzn_cad_identifier_id cad_primitive);

cmzn_field_cad_primitive_group_template_id cmzn_field_group_create_cad_primitive_group(cmzn_field_group_id group, cmzn_field_cad_topology_id cad_topology_domain);

cmzn_field_cad_primitive_group_template_id cmzn_field_group_get_cad_primitive_group(cmzn_field_group_id group, cmzn_field_cad_topology_id cad_topology_domain);

int cmzn_field_group_clear_region_tree_cad_primitive(cmzn_field_group_id group);

cmzn_cad_identifier_id cmzn_field_cad_primitive_group_template_get_first_cad_primitive(
	cmzn_field_cad_primitive_group_template_id cad_primitive_group);

cmzn_cad_identifier_id cmzn_field_cad_primitive_group_template_get_next_cad_primitive(
	cmzn_field_cad_primitive_group_template_id cad_primitive_group);


#endif /* #ifndef CMZN_FIELDCAD_H */

