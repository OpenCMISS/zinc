/**
 * FILE : fieldml_common.cpp
 * 
 * Common data and classes for FieldML 0.5 I/O.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cstring>
#include "field_io/fieldml_common.hpp"
#include "finite_element/finite_element_basis.hpp"
#include "general/message.h"
#include "general/mystring.h"


enum cmzn_element_shape_type getElementShapeFromFieldmlName(const char *shapeName)
{
	for (int i = 0; i < numLibraryShapes; i++)
		if (0 == strcmp(shapeName, libraryShapes[i].fieldmlName))
			return libraryShapes[i].shape_type;
	display_message(ERROR_MESSAGE, "FieldML:  Unrecognised FieldML shape evaluator %s", shapeName);
	return CMZN_ELEMENT_SHAPE_TYPE_INVALID;
}

const char *getFieldmlNameFromElementShape(enum cmzn_element_shape_type shapeType)
{
	for (int i = 0; i < numLibraryShapes; i++)
		if (shapeType == libraryShapes[i].shape_type)
			return libraryShapes[i].fieldmlName;
	display_message(ERROR_MESSAGE, "FieldML:  Unrecognised element shape %d", shapeType);
	return 0;
}

bool filename_has_FieldML_extension(const char *filename)
{
	const char *extension_start = strrchr(filename, '.');
	if (extension_start && fuzzy_string_compare_same_length(extension_start, ".fieldml"))
		return true;
	return false;
}
