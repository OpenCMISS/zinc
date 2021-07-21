/**
 * FILE : enumerator_conversion.hpp
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (ENUMERATOR_CONVERSION_HPP)
#define ENUMERATOR_CONVERSION_HPP

#include <cstring>

template <typename enum_type, class enum_conversion>
	enum_type string_to_enum(const char *string)
{
	for (int i = 1; ; ++i)
	{
		enum_type enum_value = static_cast<enum_type>(i);
		const char *enum_string = enum_conversion::to_string(enum_value);
		if (!enum_string)
			break;
		if (0 == strcmp(enum_string, string))
			return enum_value;
	}
	return static_cast<enum_type>(0);
}

template <typename enum_type, class enum_conversion>
	enum_type string_to_enum_bitshift(const char *string)
{
	for (int i = 1; ; i <<= 1)
	{
		enum_type enum_value = static_cast<enum_type>(i);
		const char *enum_string = enum_conversion::to_string(enum_value);
		if (!enum_string)
			break;
		if (0 == strcmp(enum_string, string))
			return enum_value;
	}
	return static_cast<enum_type>(0);
}

#endif /* ENUMERATOR_CONVERSION_HPP */
