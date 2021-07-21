/*
 * computed_field_cad_colour.h
 *
 *  Created on: 12-Oct-2009
 *      Author: hsorby
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef CAD_COMPUTED_FIELD_CAD_COLOUR_H_
#define CAD_COMPUTED_FIELD_CAD_COLOUR_H_

extern "C" {
#include "api/cmiss_field.h"
}
#include "cad/geometricshape.h"

//#define Computed_field_create_cad_colour cmzn_field_create_cad_colour

struct cmzn_field_cad_colour;

typedef struct cmzn_field_cad_colour *cmzn_field_cad_colour_id;

cmzn_field_id Computed_field_module_create_cad_colour(cmzn_field_module_id field_module, cmzn_field_id field);

cmzn_field_cad_colour_id cmzn_field_cast_cad_colour( cmzn_field_id cad_colour_field );

int cmzn_field_is_cad_colour( cmzn_field_id field, void *not_in_use );

#endif /* CAD_COMPUTED_FIELD_CAD_COLOUR_H_ */
