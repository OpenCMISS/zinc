/*******************************************************************************
FILE : computed_field_lookup.cpp

LAST MODIFIED : 25 July 2007

Defines fields for looking up values at given locations.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <math.h>
#include "opencmiss/zinc/fieldfiniteelement.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_private.hpp"
#include "computed_field/computed_field_set.h"
#include "computed_field/computed_field_finite_element.h"
#include "computed_field/field_module.hpp"
#include "finite_element/finite_element_nodeset.hpp"
#include "finite_element/finite_element_region.h"
#include "finite_element/finite_element_time.h"
#include "region/cmiss_region.hpp"
#include "general/debug.h"
#include "general/mystring.h"
#include "general/message.h"
#include "computed_field/computed_field_lookup.h"
#include "graphics/quaternion.hpp"

class Computed_field_lookup_package : public Computed_field_type_package
{
public:
	struct cmzn_region *root_region;
};

namespace {

const char computed_field_nodal_lookup_type_string[] = "nodal_lookup";

class Computed_field_nodal_lookup : public Computed_field_core
{
public:
	FE_node *lookup_node;

	Computed_field_nodal_lookup(FE_node *lookup_nodeIn) :
		Computed_field_core(),
		lookup_node(lookup_nodeIn->access())
	{
	}

	virtual bool attach_to_field(Computed_field *parent)
	{
		if (Computed_field_core::attach_to_field(parent))
		{
			FE_nodeset *fe_nodeset = FE_node_get_FE_nodeset(this->lookup_node);
			FE_region *fe_region = fe_nodeset->get_FE_region();
			if (Computed_field_get_region(parent->source_fields[0])->get_FE_region() == fe_region)
				return true;
		}
		return false;
	}

	~Computed_field_nodal_lookup();

private:
	Computed_field_core *copy();

	const char *get_type_string()
	{
		return(computed_field_nodal_lookup_type_string);
	}

	int compare(Computed_field_core* other_field);

	int list();

	char* get_command_string();

	virtual bool is_defined_at_location(cmzn_fieldcache& cache);

	virtual FieldValueCache *createValueCache(cmzn_fieldcache& fieldCache)
	{
		RealFieldValueCache *valueCache = new RealFieldValueCache(field->number_of_components);
		valueCache->getOrCreateSharedExtraCache(fieldCache);
		return valueCache;
	}

	virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
	{
		return this->evaluateDerivativeFiniteDifference(cache, inValueCache, fieldDerivative);
	}

	int has_multiple_times()
	{
		return 1;
	}

	virtual int check_dependency();

};

Computed_field_nodal_lookup::~Computed_field_nodal_lookup()
{
	cmzn_node::deaccess(lookup_node);
}

Computed_field_core* Computed_field_nodal_lookup::copy()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Copy the type specific data used by this type.
==============================================================================*/
{
	Computed_field_nodal_lookup* core =
		new Computed_field_nodal_lookup(lookup_node);

	return (core);
} /* Computed_field_compose::copy */

int Computed_field_nodal_lookup::compare(Computed_field_core *other_core)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Compare the type specific data.
==============================================================================*/
{
	Computed_field_nodal_lookup* other;
	int return_code;

	ENTER(Computed_field_nodal_lookup::compare);
	if (field && (other = dynamic_cast<Computed_field_nodal_lookup*>(other_core)))
	{
		if (lookup_node == other->lookup_node)
		{
			return_code = 1;
		}
		else
		{
			return_code = 0;
		}
	}
	else
	{
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_nodal_lookup::compare */

bool Computed_field_nodal_lookup::is_defined_at_location(cmzn_fieldcache& cache)
{
	FieldValueCache &inValueCache = *(field->getValueCache(cache));
	cmzn_fieldcache& extraCache = *(inValueCache.getExtraCache());
	extraCache.setNode(this->lookup_node);  // must set node as extraCache is shared
	extraCache.setTime(cache.getTime());
	return getSourceField(0)->core->is_defined_at_location(extraCache);
}

int Computed_field_nodal_lookup::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache& valueCache = RealFieldValueCache::cast(inValueCache);
	cmzn_fieldcache& extraCache = *valueCache.getExtraCache();
	extraCache.setNode(this->lookup_node);  // must set node as extraCache is shared
	extraCache.setTime(cache.getTime());
	const RealFieldValueCache *sourceValueCache = RealFieldValueCache::cast(getSourceField(0)->evaluate(extraCache));
	if (sourceValueCache)
	{
		valueCache.copyValues(*sourceValueCache);
		return 1;
	}
	return 0;
}

int Computed_field_nodal_lookup::list()
{
	int return_code;

	ENTER(List_Computed_field_time_nodal_lookup);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    field : %s\n",
			field->source_fields[0]->name);
		display_message(INFORMATION_MESSAGE,
			"    node : %d\n",
			get_FE_node_identifier(lookup_node));
		FE_nodeset *fe_nodeset = FE_node_get_FE_nodeset(lookup_node);
		display_message(INFORMATION_MESSAGE, "    nodeset: %s\n",
			(CMZN_FIELD_DOMAIN_TYPE_NODES == fe_nodeset->getFieldDomainType()) ? "nodes\n" : "datapoints\n");
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_nodal_lookup.  Invalid field");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_nodal_lookup */

char *Computed_field_nodal_lookup::get_command_string()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field.
==============================================================================*/
{
	char *command_string, *field_name, node_id[10];
	int error,node_number;

	ENTER(Computed_field_time_nodal_lookup::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_nodal_lookup_type_string, &error);
		append_string(&command_string, " field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
		FE_nodeset *fe_nodeset = FE_node_get_FE_nodeset(lookup_node);
		if (CMZN_FIELD_DOMAIN_TYPE_NODES == fe_nodeset->getFieldDomainType())
		{
			append_string(&command_string, " nodeset nodes ", &error);
		}
		else
		{
			append_string(&command_string, " nodeset datapoints ", &error);
		}
		append_string(&command_string, " node ", &error);
		node_number = get_FE_node_identifier(lookup_node);
		sprintf(node_id,"%d",node_number);
		append_string(&command_string, " ", &error);
		append_string(&command_string, node_id, &error);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_nodal_lookup::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_nodal_lookup::get_command_string */

int Computed_field_nodal_lookup::check_dependency()
{
	if (field)
	{
		if (0 == (field->manager_change_status & MANAGER_CHANGE_FULL_RESULT(Computed_field)))
		{
			int source_change_status = field->source_fields[0]->core->check_dependency();
			if (source_change_status & MANAGER_CHANGE_FULL_RESULT(Computed_field))
				field->setChangedPrivate(MANAGER_CHANGE_FULL_RESULT(Computed_field));
			else if (source_change_status & MANAGER_CHANGE_PARTIAL_RESULT(Computed_field))
			{
				FE_nodeset *fe_nodeset = FE_node_get_FE_nodeset(this->lookup_node);
				DsLabelsChangeLog *nodeChangeLog = fe_nodeset->getChangeLog();
				if (nodeChangeLog->isIndexChange(this->lookup_node->getIndex()))
					field->setChangedPrivate(MANAGER_CHANGE_FULL_RESULT(Computed_field));
			}
		}
		return field->manager_change_status;
	}
	return MANAGER_CHANGE_NONE(Computed_field);
}

} //namespace

cmzn_field_id cmzn_fieldmodule_create_field_node_lookup(
	cmzn_fieldmodule_id field_module, cmzn_field_id source_field, cmzn_node_id lookup_node)
{
	cmzn_field *field = nullptr;
	if (source_field && source_field->isNumerical() && lookup_node &&
		(FE_node_get_FE_nodeset(lookup_node)->get_FE_region() ==
			cmzn_fieldmodule_get_region_internal(field_module)->get_FE_region()))
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field->number_of_components,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_nodal_lookup(lookup_node));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_node_lookup.  Invalid argument(s)");
	}
	return (field);
}

int Computed_field_get_type_nodal_lookup(struct Computed_field *field,
  struct Computed_field **source_field, struct FE_node **lookup_node)
{
	Computed_field_nodal_lookup* core;
	int return_code = 0;

	ENTER(Computed_field_get_type_nodal_lookup);
	if (field && (NULL != (core =
		dynamic_cast<Computed_field_nodal_lookup*>(field->core))) &&
		source_field && lookup_node)
	{
		*source_field = field->source_fields[0];
		*lookup_node = core->lookup_node;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_nodal_lookup.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_nodal_lookup */

namespace {

const char computed_field_quaternion_SLERP_type_string[] = "quaternion_SLERP";

class Computed_field_quaternion_SLERP : public Computed_field_core
{
public:
	FE_node *nodal_lookup_node;

	Computed_field_quaternion_SLERP(
		FE_node *nodal_lookup_node) :
		Computed_field_core(),
		nodal_lookup_node(nodal_lookup_node->access())
	{
	};

	virtual bool attach_to_field(Computed_field *parent)
	{
		if (Computed_field_core::attach_to_field(parent))
		{
			FE_nodeset *fe_nodeset = FE_node_get_FE_nodeset(this->nodal_lookup_node);
			FE_region *fe_region = fe_nodeset->get_FE_region();
			if (Computed_field_get_region(parent->source_fields[0])->get_FE_region() == fe_region)
				return true;
		}
		return false;
	}

	~Computed_field_quaternion_SLERP();

private:

	 Computed_field_core *copy();

	 const char *get_type_string()
	 {
			return(computed_field_quaternion_SLERP_type_string);
	 }

	int compare(Computed_field_core* other_field);

	virtual bool is_defined_at_location(cmzn_fieldcache& cache);

	virtual FieldValueCache *createValueCache(cmzn_fieldcache& fieldCache)
	{
		RealFieldValueCache *valueCache = new RealFieldValueCache(field->number_of_components);
		valueCache->getOrCreateSharedExtraCache(fieldCache);
		return valueCache;
	}

	virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
	{
		return this->evaluateDerivativeFiniteDifference(cache, inValueCache, fieldDerivative);
	}

	int list();

	char* get_command_string();

	int has_multiple_times()
	{
		return 1;
	}

	virtual int check_dependency();

};

Computed_field_quaternion_SLERP::~Computed_field_quaternion_SLERP()
{
	cmzn_node::deaccess((nodal_lookup_node));
}

Computed_field_core* Computed_field_quaternion_SLERP::copy()
/*******************************************************************************
LAST MODIFIED : 18 October 2007

DESCRIPTION :
Copy the type specific data used by this type.
==============================================================================*/
{
	Computed_field_quaternion_SLERP* core = new Computed_field_quaternion_SLERP(
		nodal_lookup_node);

	return (core);
} /* Computed_field_quaternion_SLERP::copy */

int Computed_field_quaternion_SLERP::compare(Computed_field_core *other_core)
/*******************************************************************************
LAST MODIFIED : 18 October 2007

DESCRIPTION :
Compare the type specific data.
==============================================================================*/
{
	Computed_field_quaternion_SLERP* other;
	int return_code;

	ENTER(Computed_field_quaternion_SLERP::compare);
	if (field && (other = dynamic_cast<Computed_field_quaternion_SLERP*>(other_core)))
	{
		if (nodal_lookup_node == other->nodal_lookup_node)
		{
			return_code = 1;
		}
		else
		{
			return_code = 0;
		}
	}
	else
	{
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_quaternion_SLERP::compare */

bool Computed_field_quaternion_SLERP::is_defined_at_location(cmzn_fieldcache& cache)
{
	FieldValueCache &inValueCache = *(field->getValueCache(cache));
	cmzn_fieldcache& extraCache = *(inValueCache.getExtraCache());
	extraCache.setNode(this->nodal_lookup_node);  // must set node as extraCache is shared
	extraCache.setTime(cache.getTime());
	return getSourceField(0)->core->is_defined_at_location(extraCache);
}

int Computed_field_quaternion_SLERP::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache& valueCache = RealFieldValueCache::cast(inValueCache);
	cmzn_fieldcache& extraCache = *(valueCache.getExtraCache());
	const FE_value time = cache.getTime();
	extraCache.setNode(this->nodal_lookup_node);  // must set node as extraCache is shared
	//t is the normalised time scaled from 0 to 1
	FE_time_sequence *time_sequence = Computed_field_get_FE_node_field_FE_time_sequence(
		getSourceField(0), nodal_lookup_node);
	if (time_sequence)
	{
		FE_value xi, lower_time, upper_time;
		int time_index_one, time_index_two;
		FE_time_sequence_get_interpolation_for_time(
			time_sequence, time, &time_index_one,
			&time_index_two, &xi);
		FE_time_sequence_get_time_for_index(
			time_sequence, time_index_one, &lower_time);
		FE_time_sequence_get_time_for_index(
			time_sequence, time_index_two, &upper_time);
		FE_value normalised_t = xi;
		// get the starting quaternion
		extraCache.setTime(lower_time);
		const RealFieldValueCache *sourceValueCache = RealFieldValueCache::cast(getSourceField(0)->evaluate(extraCache));
		FE_value old_w = sourceValueCache->values[0];
		FE_value old_x = sourceValueCache->values[1];
		FE_value old_y = sourceValueCache->values[2];
		FE_value old_z = sourceValueCache->values[3];
		// get the last quaternion
		extraCache.setTime(upper_time);
		getSourceField(0)->evaluate(extraCache);
		FE_value w = sourceValueCache->values[0];
		FE_value x = sourceValueCache->values[1];
		FE_value y = sourceValueCache->values[2];
		FE_value z = sourceValueCache->values[3];

		Quaternion from(old_w, old_x, old_y, old_z);
		Quaternion to(w, x, y, z);
		Quaternion current;
		from.normalise();
		to.normalise();
		current.interpolated_with_SLERP(from, to, normalised_t);
		double quaternion_component[4];
		current.get(quaternion_component);
		for (int i = 0; i < 4; ++i)
		{
			valueCache.values[i] = quaternion_component[i];
		}
		return 1;
	}
	else
	{
		display_message(ERROR_MESSAGE, "Computed_field_quaternion::evaluate.  time sequence is missing.");
	}
	return 0;
}

int Computed_field_quaternion_SLERP::list()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_quaternion_SLERP);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    field : %s\n",
			field->source_fields[0]->name);
		display_message(INFORMATION_MESSAGE,
			"    node : %d\n",
			get_FE_node_identifier(nodal_lookup_node));
		FE_nodeset *fe_nodeset = FE_node_get_FE_nodeset(nodal_lookup_node);
		display_message(INFORMATION_MESSAGE, "    nodeset: %s\n",
			(CMZN_FIELD_DOMAIN_TYPE_NODES == fe_nodeset->getFieldDomainType()) ? "nodes\n" : "datapoints\n");
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_quaternion_SLERP.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_quaternion_SLERP */

char *Computed_field_quaternion_SLERP::get_command_string()
/*******************************************************************************
LAST MODIFIED : 5 October 2007

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name, node_id[10];
	int error, node_number;

	ENTER(Computed_field_quaternion_SLERP::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_quaternion_SLERP_type_string, &error);
		append_string(&command_string, " field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			 make_valid_token(&field_name);
			 append_string(&command_string, field_name, &error);
			 DEALLOCATE(field_name);
		}
		FE_nodeset *fe_nodeset = FE_node_get_FE_nodeset(nodal_lookup_node);
		if (CMZN_FIELD_DOMAIN_TYPE_NODES == fe_nodeset->getFieldDomainType())
		{
			append_string(&command_string, " nodeset nodes ", &error);
		}
		else
		{
			append_string(&command_string, " nodeset datapoints ", &error);
		}
		append_string(&command_string, " node ", &error);
		node_number = get_FE_node_identifier(nodal_lookup_node);
		sprintf(node_id,"%d",node_number);
		append_string(&command_string, " ", &error);
		append_string(&command_string, node_id, &error);
	}
	else
	{
		 display_message(ERROR_MESSAGE,
				"Computed_field_quaternion_SLERP::get_command_string.  "
				"Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_quaternion_SLERP::get_command_string */

int Computed_field_quaternion_SLERP::check_dependency()
{
	if (field)
	{
		if (0 == (field->manager_change_status & MANAGER_CHANGE_FULL_RESULT(Computed_field)))
		{
			int source_change_status = field->source_fields[0]->core->check_dependency();
			if (source_change_status & MANAGER_CHANGE_FULL_RESULT(Computed_field))
				field->setChangedPrivate(MANAGER_CHANGE_FULL_RESULT(Computed_field));
			else if (source_change_status & MANAGER_CHANGE_PARTIAL_RESULT(Computed_field))
			{
				FE_nodeset *fe_nodeset = FE_node_get_FE_nodeset(this->nodal_lookup_node);
				DsLabelsChangeLog *nodeChangeLog = fe_nodeset->getChangeLog();
				if (nodeChangeLog->isIndexChange(this->nodal_lookup_node->getIndex()))
					field->setChangedPrivate(MANAGER_CHANGE_FULL_RESULT(Computed_field));
			}
		}
		return field->manager_change_status;
	}
	return MANAGER_CHANGE_NONE(Computed_field);
}

} //namespace

/***************************************************************************//**
 * Create field that perfoms a quaternion SLERP interpolation where time
 * variation of quaternion parameters are held in a node.
 * <source_field> must have 4 components.
 */
cmzn_field *cmzn_fieldmodule_create_field_quaternion_SLERP(
	cmzn_fieldmodule_id fieldmodule, cmzn_field_id source_field,
	cmzn_node_id quaternion_SLERP_node)
{
	cmzn_field *field = nullptr;
	if (source_field && (4 == source_field->number_of_components) &&
		quaternion_SLERP_node &&
			(FE_node_get_FE_nodeset(quaternion_SLERP_node)->get_FE_region() ==
			cmzn_fieldmodule_get_region_internal(fieldmodule)->get_FE_region()))
	{
		field = Computed_field_create_generic(fieldmodule,
			/*check_source_field_regions*/true,
			source_field->number_of_components,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_quaternion_SLERP(quaternion_SLERP_node));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_quaternion_SLERP.  Invalid argument(s)");
	}

	return (field);
}

int Computed_field_get_type_quaternion_SLERP(struct Computed_field *field,
	 struct Computed_field **quaternion_SLERP_field,
	 struct FE_node **lookup_node)
/*******************************************************************************
LAST MODIFIED : 10 October 2007

DESCRIPTION :
If the field is of type COMPUTED_FIELD_QUATERNION_SLERP, the function returns the source
<quaternion_SLERP_field>, <quaternion_SLERP_region>, and the <quaternion_SLERP_node_identifier>.
Note that nothing returned has been ACCESSed.
==============================================================================*/
{
	 Computed_field_quaternion_SLERP* core;
	 int return_code;

	ENTER(Computed_field_get_type_quatenions_SLERP);
	if (field && (core = dynamic_cast<Computed_field_quaternion_SLERP*>(field->core)))
	{
		 *quaternion_SLERP_field = field->source_fields[0];
		 *lookup_node = core->nodal_lookup_node;
		 return_code = 1;
	}
	else
	{
		 display_message(ERROR_MESSAGE,
				"Computed_field_get_type_nodal_lookup.  Invalid argument(s)");
		 return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_quaternion_SLERP */

