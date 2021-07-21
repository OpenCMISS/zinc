/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

extern "C"
{
#include "computed_field/computed_field.h"
#include "general/debug.h"
}
#include "cad/element_identifier.h"
#include "cad/computed_field_cad_topology.h"


cmzn_cad_identifier::cmzn_cad_identifier(cmzn_field_cad_topology_id cad_topology, Cad_primitive_identifier cad_identifier)
	: cad_topology(cad_topology)
	, identifier(cad_identifier)
{
	cmzn_field_cad_topology_access(this->cad_topology);
	//DEBUG_PRINT("Constructor %p %d\n", this, cmzn_field_get_access_count(reinterpret_cast<cmzn_field_id>(this->cad_topology)));
}

cmzn_cad_identifier::~cmzn_cad_identifier()
{
	//DEBUG_PRINT("Destructor %p %d\n", this, cmzn_field_get_access_count(reinterpret_cast<cmzn_field_id>(this->cad_topology))-1);
	cmzn_field_cad_topology_destroy(&(this->cad_topology));
}

cmzn_cad_identifier::cmzn_cad_identifier(const cmzn_cad_identifier& cad_identifier)
{
	this->cad_topology = cad_identifier.cad_topology;
	cmzn_field_cad_topology_access(this->cad_topology);
	//DEBUG_PRINT("Copy constructor %d\n", cmzn_field_get_access_count(reinterpret_cast<cmzn_field_id>(this->cad_topology)));
	this->identifier = cad_identifier.identifier;
}

cmzn_cad_identifier& cmzn_cad_identifier::operator=(const cmzn_cad_identifier& source)
{
	//DEBUG_PRINT("Assignment operator\n");
	cmzn_cad_identifier_id result = new cmzn_cad_identifier(source);
	return (*result);
}

bool cmzn_cad_identifier::operator==(const cmzn_cad_identifier& other) const
{
	bool result = false;
	//DEBUG_PRINT("Comparison operator\n");
	if (this->cad_topology == other.cad_topology &&
		this->identifier.number == other.identifier.number &&
		this->identifier.type == other.identifier.type)
		result = true;

	return result;
}

bool cmzn_cad_identifier::operator!=(const cmzn_cad_identifier& other) const
{
	return !(*this == other);
}




