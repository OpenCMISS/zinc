/*******************************************************************************
FILE : computed_field_conditional.cpp

LAST MODIFIED : 16 May 2008

DESCRIPTION :
Implements a number of basic component wise operations on computed fields.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <math.h>
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_private.hpp"
#include "computed_field/computed_field_set.h"
#include "general/debug.h"
#include "general/mystring.h"
#include "general/message.h"
#include "computed_field/computed_field_conditional.h"

class Computed_field_conditional_package : public Computed_field_type_package
{
};

namespace {

char computed_field_if_type_string[] = "if";

class Computed_field_if : public Computed_field_core
{
	cmzn_field_value_type value_type;

public:
	Computed_field_if() : Computed_field_core()
		, value_type(CMZN_FIELD_VALUE_TYPE_INVALID)
	{
	}

	virtual bool attach_to_field(cmzn_field *parent)
	{
		if (Computed_field_core::attach_to_field(parent))
		{
			if (cmzn_field_get_value_type(parent->source_fields[1]) ==
				cmzn_field_get_value_type(parent->source_fields[2]))
			{
				value_type = cmzn_field_get_value_type(parent->source_fields[1]);
				return true;
			}
		}
		return false;
	}

private:
	Computed_field_core *copy()
	{
		return new Computed_field_if();
	}

	const char *get_type_string()
	{
		return(computed_field_if_type_string);
	}

	virtual enum cmzn_field_type get_type()
	{
		return CMZN_FIELD_TYPE_IF;
	}

	virtual FieldValueCache *createValueCache(cmzn_fieldcache& /*fieldCache*/)
	{
		if (value_type == CMZN_FIELD_VALUE_TYPE_REAL)
			return new RealFieldValueCache(field->number_of_components);
		else if (value_type == CMZN_FIELD_VALUE_TYPE_STRING)
			return new StringFieldValueCache();
		else if (value_type == CMZN_FIELD_VALUE_TYPE_MESH_LOCATION)
			return new MeshLocationFieldValueCache();

		return 0;
	}

	int compare(Computed_field_core* other_field)
	{
		if (dynamic_cast<Computed_field_if*>(other_field))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative);

	int list();

	char* get_command_string();
};

int Computed_field_if::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	cmzn_field *switchField = getSourceField(0);
	const RealFieldValueCache *source1Cache = RealFieldValueCache::cast(switchField->evaluate(cache));
	if (source1Cache)
	{
		// Work out whether we need to evaluate source field 2, 3 or both
		bool evaluateField2 = false;
		bool evaluateField3 = false;
		for (int i = 0; i < switchField->number_of_components; ++i)
			if (source1Cache->values[i])
				evaluateField2 = true;
			else
				evaluateField3 = true;
		if (this->value_type == CMZN_FIELD_VALUE_TYPE_REAL)
		{
			RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
			const RealFieldValueCache *source2Cache = evaluateField2 ? RealFieldValueCache::cast(getSourceField(1)->evaluate(cache)) : nullptr;
			const RealFieldValueCache *source3Cache = evaluateField3 ? RealFieldValueCache::cast(getSourceField(2)->evaluate(cache)) : nullptr;
			if (((!evaluateField2) || source2Cache) && ((!evaluateField3) || source3Cache))
			{
				const FE_value *sourceValues = nullptr;
				for (int i = 0; i < field->number_of_components; ++i)
				{
					if (i < switchField->number_of_components)
						sourceValues = (source1Cache->values[i]) ? source2Cache->values : source3Cache->values;
					valueCache.values[i] = sourceValues[i];
				}
				return 1;
			}
		}
		else if (this->value_type == CMZN_FIELD_VALUE_TYPE_STRING)
		{
			// can only be scalar
			StringFieldValueCache &valueCache = StringFieldValueCache::cast(inValueCache);
			const StringFieldValueCache *useSourceCache = evaluateField2 ?
				StringFieldValueCache::cast(getSourceField(1)->evaluate(cache)) :
				StringFieldValueCache::cast(getSourceField(2)->evaluate(cache));
			if (useSourceCache)
			{
				valueCache.setString(useSourceCache->stringValue);
				return 1;
			}
		}
		else if (this->value_type == CMZN_FIELD_VALUE_TYPE_MESH_LOCATION)
		{
		}
	}
	return 0;
}

int Computed_field_if::evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
{
	if (this->value_type != CMZN_FIELD_VALUE_TYPE_REAL)
		return 0;
	cmzn_field *switchField = getSourceField(0);
	const RealFieldValueCache *source1Cache = RealFieldValueCache::cast(switchField->evaluate(cache));
	if (source1Cache)
	{
		// Work out whether we need to evaluate source field 2, 3 or both
		bool evaluateField2 = false;
		bool evaluateField3 = false;
		for (int i = 0; i < switchField->number_of_components; ++i)
			if (source1Cache->values[i])
				evaluateField2 = true;
			else
				evaluateField3 = true;
		const DerivativeValueCache *source2DerivativeCache = evaluateField2 ? getSourceField(1)->evaluateDerivative(cache, fieldDerivative) : nullptr;
		const DerivativeValueCache *source3DerivativeCache = evaluateField3 ? getSourceField(2)->evaluateDerivative(cache, fieldDerivative) : nullptr;
		if (((!evaluateField2) || source2DerivativeCache) && ((!evaluateField3) || source3DerivativeCache))
		{
			DerivativeValueCache *derivativeCache = inValueCache.getDerivativeValueCache(fieldDerivative);
			FE_value *derivatives = derivativeCache->values;
			const int componentCount = this->field->number_of_components;
			const int termCount = derivativeCache->getTermCount();
			const FE_value *sourceDerivatives = nullptr;
			for (int i = 0; i < componentCount; ++i)
			{
				if (i < switchField->number_of_components)
					sourceDerivatives = (source1Cache->values[i]) ? source2DerivativeCache->values : source3DerivativeCache->values;
				FE_value *componentDerivatives = derivatives + i*termCount;
				const FE_value *componentSourceDerivatives = sourceDerivatives + i*termCount;
				for (int j = 0; j < termCount; ++j)
					componentDerivatives[j] = componentSourceDerivatives[j];
			}
			return 1;
		}
	}
	return 0;
}

int Computed_field_if::list()
/*******************************************************************************
LAST MODIFIED : 27 July 2007

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_if);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    source fields : %s %s\n",field->source_fields[0]->name,
			field->source_fields[1]->name, field->source_fields[2]->name);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_if.  Invalid field");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_if */

char *Computed_field_if::get_command_string()
/*******************************************************************************
LAST MODIFIED : 27 July 2007

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name;
	int error;

	ENTER(Computed_field_if::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_if_type_string, &error);
		append_string(&command_string, " fields ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
		if (GET_NAME(Computed_field)(field->source_fields[1], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, " ", &error);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
		if (GET_NAME(Computed_field)(field->source_fields[2], &field_name))
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
			"Computed_field_if::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_if::get_command_string */

} //namespace

cmzn_field_id cmzn_fieldmodule_create_field_if(cmzn_fieldmodule_id field_module,
	cmzn_field_id source_field_one,
	cmzn_field_id source_field_two,
	cmzn_field_id source_field_three)
{
	cmzn_field *field = nullptr;
	if (source_field_one && source_field_one->isNumerical() &&
		source_field_two && source_field_three &&
		((source_field_one->number_of_components == 1) ||
		(source_field_one->number_of_components ==
			source_field_two->number_of_components)) &&
		(source_field_two->number_of_components ==
			source_field_three->number_of_components))
	{
		cmzn_field *source_fields[3];
		source_fields[0] = source_field_one;
		source_fields[1] = source_field_two;
		source_fields[2] = source_field_three;
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field_two->number_of_components,
			/*number_of_source_fields*/3, source_fields,
			/*number_of_source_values*/0, NULL,
			new Computed_field_if());
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_if.  Invalid argument(s)");
	}
	return (field);
}

int Computed_field_get_type_if(cmzn_field *field,
	cmzn_field **source_field_one,
	cmzn_field **source_field_two,
	cmzn_field **source_field_three)
/*******************************************************************************
LAST MODIFIED : 27 July 2007

DESCRIPTION :
If the field is of type COMPUTED_FIELD_IF, the 
<source_field_one>, <source_field_two> and <source_field_three> used by it
are returned.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_get_type_if);
	if (field&&(dynamic_cast<Computed_field_if*>(field->core)))
	{
		*source_field_one = field->source_fields[0];
		*source_field_two = field->source_fields[1];
		*source_field_three = field->source_fields[2];
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_if.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_if */

