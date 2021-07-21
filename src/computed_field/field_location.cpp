/**
 * FILE : field_location.hpp
 *
 * Location at which to evaluate a field.
 * These are low level transient objects used internally to evaluate fields.
 * May contain object pointers, but does not access them for thread safety.
 * Client using them must monitor for changes and clear as needed.
 */
 /* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "computed_field/field_location.hpp"

Field_location::~Field_location() {} /* Declaration of pure virtual destructor */

void Field_location_field_values::set_field_values(cmzn_field *field_in,
	int number_of_values_in, const FE_value* values_in)
{
	if (number_of_values_in > this->number_of_values_allocated)
	{
		delete[] this->values;
		this->values = new FE_value[number_of_values_in];
		this->number_of_values_allocated = number_of_values_in;
	}
	this->field = field_in;
	for (int i = 0; i < number_of_values_in; ++i)
		this->values[i] = values_in[i];
	this->number_of_values = number_of_values_in;
}
