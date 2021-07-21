/**
 * @file fieldconstant.hpp
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef CMZN_FIELDCONSTANT_HPP__
#define CMZN_FIELDCONSTANT_HPP__

#include "opencmiss/zinc/fieldconstant.h"
#include "opencmiss/zinc/field.hpp"
#include "opencmiss/zinc/fieldmodule.hpp"

namespace OpenCMISS
{
namespace Zinc
{

class FieldConstant : public Field
{
public:

	FieldConstant() : Field(0)
	{ }

	// takes ownership of C handle, responsibility for destroying it
	explicit FieldConstant(cmzn_field_constant_id field_constant_id) :
		Field(reinterpret_cast<cmzn_field_id>(field_constant_id))
	{ }

};

inline FieldConstant Field::castConstant()
{
	return FieldConstant(cmzn_field_cast_constant(this->id));
}

inline FieldConstant Fieldmodule::createFieldConstant(int valuesCount, const double *valuesIn)
{
	return FieldConstant(reinterpret_cast<cmzn_field_constant_id>(
		cmzn_fieldmodule_create_field_constant(this->id, valuesCount, valuesIn)));
}

class FieldStringConstant : public Field
{
public:

	FieldStringConstant() : Field(0)
	{ }

	// takes ownership of C handle, responsibility for destroying it
	explicit FieldStringConstant(cmzn_field_string_constant_id field_string_constant_id) :
		Field(reinterpret_cast<cmzn_field_id>(field_string_constant_id))
	{ }

};

inline FieldStringConstant Field::castStringConstant()
{
	return FieldStringConstant(cmzn_field_cast_string_constant(this->id));
}

inline FieldStringConstant Fieldmodule::createFieldStringConstant(const char *stringConstant)
{
	return FieldStringConstant(reinterpret_cast<cmzn_field_string_constant_id>(
		cmzn_fieldmodule_create_field_string_constant(this->id, stringConstant)));
}

}  // namespace Zinc
}

#endif
