/*******************************************************************************
FILE : computed_field_time.c

LAST MODIFIED : 25 August 2006

DESCRIPTION :
Implements a number of basic component wise operations on computed fields.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <math.h>
#include "opencmiss/zinc/fieldtime.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_private.hpp"
#include "computed_field/computed_field_set.h"
#include "general/debug.h"
#include "general/mystring.h"
#include "time/time.h"
#include "general/message.h"
#include "computed_field/computed_field_time.h"

class Computed_field_time_package : public Computed_field_type_package
{
public:
	struct cmzn_timekeeper *time_keeper;
};

namespace {

char computed_field_time_lookup_type_string[] = "time_lookup";

class Computed_field_time_lookup : public Computed_field_core
{
public:
	Computed_field_time_lookup() : Computed_field_core()
	{
	};

private:
	Computed_field_core *copy()
	{
		return new Computed_field_time_lookup();
	}

	const char *get_type_string()
	{
		return(computed_field_time_lookup_type_string);
	}

	virtual enum cmzn_field_type get_type()
	{
		return CMZN_FIELD_TYPE_TIME_LOOKUP;
	}

	int compare(Computed_field_core* other_field)
	{
		if (dynamic_cast<Computed_field_time_lookup*>(other_field))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	virtual FieldValueCache *createValueCache(cmzn_fieldcache& fieldCache)
	{
		RealFieldValueCache *valueCache = new RealFieldValueCache(field->number_of_components);
		valueCache->getOrCreatePrivateExtraCache(Computed_field_get_region(field));
		return valueCache;
	}

	virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
	{
		return this->evaluateDerivativeFiniteDifference(cache, inValueCache, fieldDerivative);
	}

	int list();

	char* get_command_string();

	int has_multiple_times();
};

int Computed_field_time_lookup::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	const RealFieldValueCache *timeValueCache = RealFieldValueCache::cast(getSourceField(1)->evaluate(cache));
	if (timeValueCache)
	{
		RealFieldValueCache& valueCache = RealFieldValueCache::cast(inValueCache);
		cmzn_fieldcache& extraCache = *valueCache.getExtraCache();
		extraCache.copyLocation(cache);
		extraCache.setTime(timeValueCache->values[0]);
		const RealFieldValueCache *sourceValueCache = RealFieldValueCache::cast(getSourceField(0)->evaluate(extraCache));
		if (sourceValueCache)
		{
			valueCache.copyValues(*sourceValueCache);
			return 1;
		}
	}
	return 0;
}

int Computed_field_time_lookup::list()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_time_lookup);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    field : %s\n",
			field->source_fields[0]->name);
		display_message(INFORMATION_MESSAGE,
			"    time field : %s\n",
			field->source_fields[1]->name);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_time_lookup.  Invalid field");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_time_lookup */

char *Computed_field_time_lookup::get_command_string()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name;
	int error;

	ENTER(Computed_field_time_lookup::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_time_lookup_type_string, &error);
		append_string(&command_string, " field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
		append_string(&command_string, " time_field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[1], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, " ", &error);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_time_lookup::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_time_lookup::get_command_string */

int Computed_field_time_lookup::has_multiple_times ()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Returns 1 if the time_field source_field has multiple times.  The times of the
actual source field are controlled by this time field and so changes in the
global time do not matter.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_default::has_multiple_times);
	if (field && (2 == field->number_of_source_fields))
	{
		return_code=0;
		if (Computed_field_has_multiple_times(field->source_fields[1]))
		{
			return_code=1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_time_lookup::has_multiple_times.  "
			"Invalid arguments.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_time_lookup::has_multiple_times */

} //namespace

cmzn_field_id cmzn_fieldmodule_create_field_time_lookup(
	struct cmzn_fieldmodule *field_module,
	struct Computed_field *source_field, struct Computed_field *time_field)
{
	struct Computed_field *field = NULL;
	if (field_module && source_field && time_field &&
		(1 == time_field->number_of_components))
	{
		Computed_field *source_fields[2];
		source_fields[0] = source_field;
		source_fields[1] = time_field;
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field->number_of_components,
			/*number_of_source_fields*/2, source_fields,
			/*number_of_source_values*/0, NULL,
			new Computed_field_time_lookup());
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_time_lookup.  Invalid argument(s)");
	}

	return (field);
}

int Computed_field_get_type_time_lookup(struct Computed_field *field,
	struct Computed_field **source_field, struct Computed_field **time_field)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_TIME_LOOKUP, the
<source_field> and <time_field> used by it are returned.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_get_type_time_lookup);
	if (field&&(dynamic_cast<Computed_field_time_lookup*>(field->core)))
	{
		*source_field = field->source_fields[0];
		*time_field = field->source_fields[1];
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_time_lookup.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_time_lookup */

namespace {

char computed_field_time_value_type_string[] = "time_value";

class Computed_field_time_value : public Computed_field_core
{
public:

	Time_object *time_object;

	Computed_field_time_value(cmzn_timekeeper* time_keeper) :
		Computed_field_core(),
		time_object(NULL)
	{
		time_object = Time_object_create_regular(
			/*update_frequency*/10.0, /*time_offset*/0.0);
		if (!time_keeper->addTimeObject(time_object))
		{
			DEACCESS(Time_object)(&time_object);
		}
	};

	virtual bool attach_to_field(Computed_field *parent)
	{
		if (Computed_field_core::attach_to_field(parent))
		{
			if (time_object && Time_object_set_name(time_object, parent->name))
			{
				return true;
			}
		}
		return false;
	}

	~Computed_field_time_value()
	{
		DEACCESS(Time_object)(&time_object);
	};

private:
	Computed_field_core *copy()
	{
		return new Computed_field_time_value(
			Time_object_get_timekeeper(time_object));
	}

	const char *get_type_string()
	{
		return(computed_field_time_value_type_string);
	}

	virtual enum cmzn_field_type get_type()
	{
		return CMZN_FIELD_TYPE_TIME_VALUE;
	}

	int compare(Computed_field_core* other_field);

	virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative);

	int list();

	char* get_command_string();

	int has_multiple_times();
};

int Computed_field_time_value::compare(Computed_field_core *other_core)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
==============================================================================*/
{
	Computed_field_time_value* other;
	int return_code;

	ENTER(Computed_field_time_value::compare);
	if (field && (other = dynamic_cast<Computed_field_time_value*>(other_core)))
	{
		if (Time_object_get_timekeeper(time_object) ==
			Time_object_get_timekeeper(other->time_object))
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
} /* Computed_field_time_value::compare */

int Computed_field_time_value::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
	valueCache.values[0] = (Time_object_get_timekeeper(time_object))->getTime();
	return 1;
}

int Computed_field_time_value::evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
{
	// spatial derivatives are zero (review when time derivatives are added)
	inValueCache.getDerivativeValueCache(fieldDerivative)->zeroValues();
	return 1;
}

int Computed_field_time_value::list()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_time_value);
	if (field)
	{
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_time_value.  Invalid field");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_time_value */

char *Computed_field_time_value::get_command_string()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string;
	int error;

	ENTER(Computed_field_time_value::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_time_value_type_string, &error);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_time_value::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_time_value::get_command_string */

int Computed_field_time_value::has_multiple_times()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Always has multiple times.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_time_value::has_multiple_times);
	if (field)
	{
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_time_value::has_multiple_times.  "
			"Invalid arguments.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_time_value::has_multiple_times */

} //namespace

cmzn_field_id cmzn_fieldmodule_create_field_time_value(
	struct cmzn_fieldmodule *field_module, struct cmzn_timekeeper *timekeeper)
{
	struct Computed_field *field = NULL;
	if (field_module && timekeeper)
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			/*number_of_components*/1,
			/*number_of_source_fields*/0, NULL,
			/*number_of_source_values*/0, NULL,
			new Computed_field_time_value(timekeeper));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_time_value.  Invalid argument(s)");
	}

	return (field);
}

