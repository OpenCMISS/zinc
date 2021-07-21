/**
 * FILE : finite_element_field.hpp
 *
 * Internal class for field interpolated over finite elements.
 * Private header to share struct definition.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (FINITE_ELEMENT_FIELD_PRIVATE_HPP)
#define FINITE_ELEMENT_FIELD_PRIVATE_HPP

#include "opencmiss/zinc/types/elementid.h"
#include <opencmiss/zinc/zincconfigure.h>
#include "finite_element/finite_element_field.hpp"
#include "finite_element/finite_element_private.h"
#include "general/cmiss_set.hpp"
#include "general/geometry.h"
#include "general/indexed_list_stl_private.hpp"
#include "general/value.h"

/*
Global types
------------
*/

/* Only to be used from FIND_BY_IDENTIFIER_IN_INDEXED_LIST_STL function
 * Creates a pseudo object with name identifier suitable for finding
 * objects by identifier with cmzn_set.
 */
class FE_field_identifier : private FE_field
{
public:
	FE_field_identifier(const char *name)
	{
		this->name = name;
	}

	~FE_field_identifier()
	{
		this->name = nullptr;
	}

	FE_field *getPseudoObject()
	{
		return this;
	}
};

/** functor for ordering cmzn_set<FE_field> by field name */
struct FE_field_compare_name
{
	inline bool operator() (const FE_field* field1, const FE_field* field2) const
	{
		return strcmp(field1->getName(), field2->getName()) < 0;
	}
};

typedef cmzn_set<FE_field *,FE_field_compare_name> cmzn_set_FE_field;

/*
Private functions
-----------------
*/

PROTOTYPE_INDEXED_LIST_STL_IDENTIFIER_CHANGE_FUNCTIONS(FE_field,name);

#endif /* !defined (FINITE_ELEMENT_FIELD_PRIVATE_HPP) */
