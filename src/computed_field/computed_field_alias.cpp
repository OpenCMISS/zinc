/*****************************************************************************//**
 * FILE : computed_field_alias.cpp
 *
 * Implements a cmiss field which is an alias for another field, commonly from a
 * different region to make it available locally.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stdlib.h>
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_alias.h"
#include "computed_field/computed_field_private.hpp"
#include "computed_field/computed_field_set.h"
#include "general/debug.h"
#include "general/mystring.h"
#include "region/cmiss_region.hpp"
#include "general/message.h"

/*
Module types
------------
*/
namespace {

const char computed_field_alias_type_string[] = "alias";

class Computed_field_alias : public Computed_field_core
{
public:
	void *other_field_manager_callback_id;

	Computed_field_alias() : Computed_field_core(),
		other_field_manager_callback_id(NULL)
	{
	}

	virtual bool attach_to_field(Computed_field *parent)
	{
		if (Computed_field_core::attach_to_field(parent))
		{
			check_alias_from_other_manager();
			return true;
		}
		return false;
	}

	~Computed_field_alias()
	{
		if (other_field_manager_callback_id)
		{
			if (field && (field->number_of_source_fields > 0) && field->source_fields && original_field())
			{
				if (original_field()->manager)
					MANAGER_DEREGISTER(Computed_field)(other_field_manager_callback_id,
						original_field()->manager);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"~Computed_field_alias.  Computed_field source_fields removed before core. Can't get manager of aliased field to end callbacks.");
			}
		}
	}

private:
	inline Computed_field *original_field(void) { return field->source_fields[0]; }

	static void other_field_manager_change(
		MANAGER_MESSAGE(Computed_field) *message, void *alias_field_core_void);

	void check_alias_from_other_manager(void);

	Computed_field_core* copy()
	{
		Computed_field_alias* core = new Computed_field_alias();
		return (core);
	};

	const char* get_type_string()
	{
		return (computed_field_alias_type_string);
	}

	virtual enum cmzn_field_type get_type()
	{
		return CMZN_FIELD_TYPE_ALIAS;
	}

	int compare(Computed_field_core* other_field);

	virtual FieldValueCache *createValueCache(cmzn_fieldcache& /*fieldCache*/)
	{
		RealFieldValueCache *valueCache = new RealFieldValueCache(field->number_of_components);
		cmzn_region_id otherRegion = Computed_field_get_region(getSourceField(0));
		if (otherRegion != Computed_field_get_region(field))
		{
			// @TODO: share extraCache with other alias fields in cache referencing otherRegion
			valueCache->getOrCreatePrivateExtraCache(otherRegion);
		}
		return valueCache;
	}

	virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative);

	int list();

	char* get_command_string();

	virtual enum FieldAssignmentResult assign(cmzn_fieldcache& /*cache*/, RealFieldValueCache& /*valueCache*/);

	void field_is_managed(void)
	{
		check_alias_from_other_manager();
	}
};

/***************************************************************************//**
 * Callback for changes in the field manager owning original_field.
 * If this field depends on the change, propagate to this manager as a change to
 * this field.
 */
void Computed_field_alias::other_field_manager_change(
	struct MANAGER_MESSAGE(Computed_field) *message, void *alias_field_core_void)
{
	Computed_field_alias *alias_field_core =
		reinterpret_cast<Computed_field_alias *>(alias_field_core_void);
	Computed_field *field;

	if (message && alias_field_core && (field = alias_field_core->field) &&
		(field->number_of_source_fields > 0) && field->source_fields)
	{
		int change = MANAGER_MESSAGE_GET_OBJECT_CHANGE(Computed_field)(message,
			alias_field_core->original_field());
		if (change & MANAGER_CHANGE_RESULT(Computed_field))
		{
			Computed_field_dependency_changed(field);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_alias::other_field_manager_change.  Invalid argument(s)");
	}
	LEAVE;
} /* Computed_field_alias::other_field_manager_change */

/***************************************************************************//**
 * If original_field is from a different manager to this field, request
 * manager messages to propagate changes to this manager.
 */
void Computed_field_alias::check_alias_from_other_manager(void)
{
	ENTER(Computed_field_alias::check_alias_from_other_manager);
	if (!other_field_manager_callback_id)
	{
		if (field && (field->number_of_source_fields > 0) && field->source_fields &&
			original_field() && original_field()->manager)
		{
			if (field->manager && (field->manager != original_field()->manager))
			{
				// alias from another region: set up manager callbacks
				other_field_manager_callback_id = MANAGER_REGISTER(Computed_field)(
					other_field_manager_change, (void *)this, original_field()->manager);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Computed_field_alias::check_alias_from_other_manager.  Invalid source_fields array.");
		}
	}
	LEAVE;
} /* Computed_field_alias::check_alias_from_other_manager */

/***************************************************************************//**
 * Compare the type specific data.
 */
int Computed_field_alias::compare(Computed_field_core *other_core)
{
	int return_code;

	ENTER(Computed_field_alias::compare);
	if (field && dynamic_cast<Computed_field_alias*>(other_core))
	{
		return_code = 1;
	}
	else
	{
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_alias::compare */

int Computed_field_alias::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
	cmzn_fieldcache *extraCache = valueCache.getExtraCache();
	const RealFieldValueCache *sourceCache = nullptr;
	if (extraCache)
	{
		// exists only if aliasing field from separate region
		extraCache->copyLocation(cache);
		sourceCache = RealFieldValueCache::cast(getSourceField(0)->evaluate(*extraCache));
	}
	else
	{
		sourceCache = RealFieldValueCache::cast(getSourceField(0)->evaluate(cache));
	}
	if (sourceCache)
	{
		valueCache.copyValues(*sourceCache);
		return 1;
	}
	return 0;
}

/** Alias field is currently guaranteed to be numerical */
int Computed_field_alias::evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
{
	RealFieldValueCache& valueCache = RealFieldValueCache::cast(inValueCache);
	DerivativeValueCache& derivativeValueCache = *valueCache.getDerivativeValueCache(fieldDerivative);
	cmzn_fieldcache *extraCache = valueCache.getExtraCache();
	if (extraCache)
	{
		// not implemented for other region - don't allow FieldDerivative for another region at this point
		return 0;
	}
	const DerivativeValueCache *sourceDerivativeValueCache = this->getSourceField(0)->evaluateDerivative(cache, fieldDerivative);
	if (sourceDerivativeValueCache)
	{
		derivativeValueCache.copyValues(*sourceDerivativeValueCache);
		return 1;
	}
	return 0;
}

/***************************************************************************//**
 * Sets values of the original field at the supplied location.
 */
enum FieldAssignmentResult Computed_field_alias::assign(cmzn_fieldcache& cache, RealFieldValueCache& valueCache)
{
	cmzn_fieldcache *extraCache = valueCache.getExtraCache();
	RealFieldValueCache *sourceCache = 0;
	if (extraCache)
	{
		// exists only if aliasing field from separate region
		extraCache->copyLocation(cache);
		sourceCache = RealFieldValueCache::cast(getSourceField(0)->getValueCache(*extraCache));
	}
	else
	{
		sourceCache = RealFieldValueCache::cast(getSourceField(0)->getValueCache(cache));
	}
	sourceCache->setValues(valueCache.values);
	return getSourceField(0)->assign(extraCache ? *extraCache : cache, *sourceCache);
}

/***************************************************************************//**
 * Writes type-specific details of the field to the console.
 */
int Computed_field_alias::list()
{
	char *field_name;
	int return_code;

	ENTER(List_Computed_field_alias);
	if (field)
	{
		display_message(INFORMATION_MESSAGE, "    Original field : ");
		if (original_field()->manager != field->manager)
		{
			char *path = cmzn_region_get_path(Computed_field_get_region(original_field()));
			display_message(INFORMATION_MESSAGE, "%s", path);
			DEALLOCATE(path);
		}
		if (GET_NAME(Computed_field)(original_field(), &field_name))
		{
			make_valid_token(&field_name);
			display_message(INFORMATION_MESSAGE, "%s\n", field_name);
			DEALLOCATE(field_name);
		}
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_alias.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_alias */

/***************************************************************************//**
 * Returns allocated command string for reproducing this field. Includes type.
 */
char *Computed_field_alias::get_command_string()
{
	char *command_string, *field_name;
	int error;

	ENTER(Computed_field_alias::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string, computed_field_alias_type_string, &error);
		append_string(&command_string, " field ", &error);
		if (original_field()->manager != field->manager)
		{
			char *path = cmzn_region_get_path(Computed_field_get_region(original_field()));
			append_string(&command_string, path, &error);
			DEALLOCATE(path);
		}
		if (GET_NAME(Computed_field)(original_field(), &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_alias::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_alias::get_command_string */

} //namespace

Computed_field *cmzn_fieldmodule_create_field_alias(cmzn_fieldmodule_id field_module,
	Computed_field *original_field)
{
	cmzn_field_id field = 0;
	// @TODO Generalise to non-numeric types by adding createValueCache and modifying evaluate methods
	if (original_field && original_field->isNumerical())
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/false, original_field->number_of_components,
			/*number_of_source_fields*/1, &original_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_alias());
	}
	return (field);
}

