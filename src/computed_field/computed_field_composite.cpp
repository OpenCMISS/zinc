/*******************************************************************************
FILE : computed_field_composite.c

LAST MODIFIED : 18 May 2008

DESCRIPTION :
Implements a "composite" computed_field which converts fields, field components
and real values in any order into a single vector field.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "opencmiss/zinc/fieldmodule.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_composite.h"
#include "computed_field/computed_field_private.hpp"
#include "computed_field/computed_field_set.h"
#include "computed_field/field_module.hpp"
#include "general/debug.h"
#include "general/mystring.h"
#include "general/message.h"
#include <cstdlib>
#include <vector>

/*
Module types
------------
*/

struct Computed_field_component
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Used to specify a component of a Computed_field with function
set_Computed_field_component.
???RC Note that in its current use the field is NOT assumed to be accessed by
this structure in set_Computed_field_component.
==============================================================================*/
{
	cmzn_field *field;
	int component_no;
}; /* struct Computed_field_component */

class Computed_field_composite_package : public Computed_field_type_package
{
};

namespace {

const char computed_field_composite_type_string[] = "composite";

class Computed_field_composite : public Computed_field_core
{
public:
	/* following are allocated with enough space for the number of components
		 in the field. <source_field_numbers> are indices into the source fields
		 for the field, with <source_values_numbers> referring to component number,
		 with both arrays starting at 0. If <source_field_numbers> is -1 then
		 the <source_value_numbers> at the same index contains the index into the
		 source values for the field. */
	int *source_field_numbers;
	int *source_value_numbers;
	enum cmzn_field_type type;

	Computed_field_composite(int number_of_components,
		const int *source_field_numbers_in, const int *source_value_numbers_in) : Computed_field_core()
	{
		int i;
		source_field_numbers = new int[number_of_components];
		source_value_numbers = new int[number_of_components];
		for (i = 0 ; i < number_of_components ; i++)
		{
			source_field_numbers[i] = source_field_numbers_in[i];
			source_value_numbers[i] = source_value_numbers_in[i];
		}
		type = CMZN_FIELD_TYPE_COMPONENT;
	}

	~Computed_field_composite();

	char *get_source_string(int commands);

	// for component field only; index and source component index start at 0.
	// @return -1 if invalid argument
	int getSourceComponentIndex(int index)
	{
		if ((0 <= index) && (index < field->number_of_components))
			return this->source_value_numbers[index];
		return -1;
	}

	// for component field only; index and source component index start at 0.
	int setSourceComponentIndex(int index, int source_component_index)
	{
		if ((0 <= index) && (index < field->number_of_components) && (0 <= source_component_index) &&
			(source_component_index < cmzn_field_get_number_of_components(getSourceField(0))))
		{
			if (source_value_numbers[index] != source_component_index)
			{
				source_value_numbers[index] = source_component_index;
				this->field->setChanged();
			}
			return CMZN_OK;
		}
		return CMZN_ERROR_ARGUMENT;
	}

private:

	Computed_field_core* copy()
	{
		return (new Computed_field_composite(field->number_of_components,
			source_field_numbers, source_value_numbers));
	}

	const char* get_type_string()
	{
		return(computed_field_composite_type_string);
	}

	virtual enum cmzn_field_type get_type()
	{
		return type;
	}

	int compare(Computed_field_core* other_field);

	virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative);

	virtual int getDerivativeTreeOrder(const FieldDerivative& fieldDerivative)
	{
		// start at constant 0, increase to maximum source field order, if any
		int order = 0;
		for (int i = 0; i < this->field->number_of_source_fields; ++i)
		{
			const int sourceOrder = this->field->source_fields[i]->getDerivativeTreeOrder(fieldDerivative);
			if (sourceOrder > order)
				order = sourceOrder;
		}
		return order;
	}

	int list();

	char* get_command_string();

	virtual enum FieldAssignmentResult assign(cmzn_fieldcache& /*cache*/, RealFieldValueCache& /*valueCache*/);

	virtual int propagate_find_element_xi(cmzn_fieldcache& field_cache,
		const FE_value *values, int number_of_values,
		struct FE_element **element_address, FE_value *xi,
		cmzn_mesh_id mesh);
};

Computed_field_composite::~Computed_field_composite()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Clear the type specific data used by this type.
==============================================================================*/
{
	ENTER(Computed_field_composite::~Computed_field_composite);
	if (field)
	{
		if (source_field_numbers)
		{
			delete [] source_field_numbers;
		}
		if (source_value_numbers)
		{
			delete [] source_value_numbers;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_composite::~Computed_field_composite.  "
			"Invalid arguments.");
	}
	LEAVE;
} /* Computed_field_composite::~Computed_field_composite */

int Computed_field_composite::compare(Computed_field_core *other_core)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Compare the type specific data
==============================================================================*/
{
	int i, return_code;
	Computed_field_composite* other;

	ENTER(Computed_field_composite::compare);
	if (field && (other = dynamic_cast<Computed_field_composite*>(other_core)))
	{
		return_code=1;
		for (i=0;return_code&&(i<field->number_of_components);i++)
		{
			if ((source_field_numbers[i] !=
				other->source_field_numbers[i]) ||
				(source_value_numbers[i] !=
					other->source_value_numbers[i]))
			{
				return_code=0;
			}
		}
	}
	else
	{
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_composite::compare */

int Computed_field_composite::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache& valueCache = RealFieldValueCache::cast(inValueCache);
	int sourceFieldNumber = -1;
	const FE_value *sourceFieldValues;
	FE_value *targetValue = valueCache.values;
	for (int c = 0; c < field->number_of_components; ++c)
	{
		if (0 <= this->source_field_numbers[c])
		{
			if (sourceFieldNumber != this->source_field_numbers[c])
			{
				sourceFieldNumber = this->source_field_numbers[c];
				const RealFieldValueCache *sourceValueCache = RealFieldValueCache::cast(this->getSourceField(sourceFieldNumber)->evaluate(cache));
				if (!sourceValueCache)
					return 0;
				sourceFieldValues = sourceValueCache->values;
			}
			*targetValue = sourceFieldValues[source_value_numbers[c]];
		}
		else
		{
			*targetValue = this->field->source_values[source_value_numbers[c]];
		}
		++targetValue;
	}
	return 1;
}

int Computed_field_composite::evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
{
	DerivativeValueCache *derivativeCache = inValueCache.getDerivativeValueCache(fieldDerivative);
	FE_value *derivatives = derivativeCache->values;
	const int componentCount = field->number_of_components;
	const int termCount = derivativeCache->getTermCount();
	int sourceFieldNumber = -1;
	const DerivativeValueCache *sourceDerivativeValueCache = nullptr;
	for (int c = 0; c < componentCount; ++c)
	{
		if (0 <= this->source_field_numbers[c])
		{
			if (sourceFieldNumber != this->source_field_numbers[c])
			{
				sourceFieldNumber = this->source_field_numbers[c];
				sourceDerivativeValueCache = this->getSourceField(sourceFieldNumber)->evaluateDerivative(cache, fieldDerivative);
				if (!sourceDerivativeValueCache)
					return 0;
			}
			const FE_value *sourceDerivatives = sourceDerivativeValueCache->values + termCount*source_value_numbers[c];
			for (int d = 0; d < termCount; ++d)
				derivatives[d] = sourceDerivatives[d];
		}
		else
		{
			for (int d = 0; d < termCount; ++d)
				derivatives[d] = 0.0;
		}
		derivatives += termCount;
	}
	return 1;
}

enum FieldAssignmentResult Computed_field_composite::assign(cmzn_fieldcache& cache, RealFieldValueCache& valueCache)
{
	/* go through each source field, getting current values, changing values
		for components that are used in the composite field, then setting the
		whole field in one hit */
	enum FieldAssignmentResult result = FIELD_ASSIGNMENT_RESULT_ALL_VALUES_SET;
	for (int source_field_number = 0; source_field_number<field->number_of_source_fields; ++source_field_number)
	{
		cmzn_field *sourceField = this->getSourceField(source_field_number);
		if (!sourceField->evaluate(cache))
			return FIELD_ASSIGNMENT_RESULT_FAIL;
		// get non-const sourceCache to modify:
		RealFieldValueCache *sourceValueCache = RealFieldValueCache::cast(sourceField->getValueCache(cache));
		for (int i=0;i<field->number_of_components;i++)
		{
			if (source_field_numbers[i] == source_field_number)
			{
				sourceValueCache->values[source_value_numbers[i]] = valueCache.values[i];
			}
		}
		enum FieldAssignmentResult thisResult = sourceField->assign(cache, *sourceValueCache);
		if (thisResult == FIELD_ASSIGNMENT_RESULT_FAIL)
			return FIELD_ASSIGNMENT_RESULT_FAIL;
		if ((result == FIELD_ASSIGNMENT_RESULT_ALL_VALUES_SET) &&
			(thisResult == FIELD_ASSIGNMENT_RESULT_PARTIAL_VALUES_SET))
		{
			result = FIELD_ASSIGNMENT_RESULT_PARTIAL_VALUES_SET;
		}
	}
	// don't assign values to constants if assigning to cache
	if (!cache.assignInCacheOnly())
	{
		bool changed = false;
		for (int i = 0; i < field->number_of_components; i++)
		{
			if (-1 == source_field_numbers[i])
			{
				field->source_values[source_value_numbers[i]] = valueCache.values[i];
				changed = true;
			}
		}
		if (changed)
			this->field->setChanged();
	}
	return result;
}

int Computed_field_composite::propagate_find_element_xi(cmzn_fieldcache& field_cache,
	const FE_value *values, int number_of_values, struct FE_element **element_address,
	FE_value *xi, cmzn_mesh_id mesh)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Currently only tries to work if there is only one and exactly one source field.
Zero is used for any source field values that aren't set from the composite field.
==============================================================================*/
{
	int i, number_of_source_values, return_code, source_field_number;
	FE_value *source_values;

	ENTER(Computed_field_composite::propagate_find_element_xi);
	if (field && values && (number_of_values == field->number_of_components))
	{
		return_code=1;
		if (field->number_of_source_fields == 1)
		{
			source_field_number = 0;
			number_of_source_values = field->source_fields[source_field_number]->number_of_components;
			if (ALLOCATE(source_values,FE_value,number_of_source_values))
			{
				/* Put zero into every value initially */
				for (i=0;i<number_of_source_values;i++)
				{
					source_values[i] = 0.0;
				}
				for (i=0;i<field->number_of_components;i++)
				{
					if (source_field_number == source_field_numbers[i])
					{
						source_values[source_value_numbers[i]] = values[i];
					}
				}
				return_code = Computed_field_find_element_xi(
					field->source_fields[source_field_number], &field_cache, source_values,
					number_of_values, element_address, xi, mesh,
					/*propagate_field*/1, /*find_nearest*/0);
				DEALLOCATE(source_values);
			}
			else
			{
				return_code=0;
			}
			if (!return_code)
			{
				display_message(ERROR_MESSAGE,
					"Computed_field_composite::propagate_find_element_xi.  Failed");
				return_code=0;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Computed_field_composite::propagate_find_element_xi.  "
				"Unable to find element xi on a composite field involving more than one source field.");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_composite::propagate_find_element_xi.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_composite::propagate_find_element_xi */

char *Computed_field_composite::get_source_string(int commands)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Returns the field components and constants that make up the composite field in
a single string. If all components of a field are used consecutively, just the
field name is put in the final string, otherwise individual components appear.
Field/component names are made into valid tokens for commands.
If <commands> is set, field/components are made into valid tokens.
==============================================================================*/
{
	char *component_name, *source_string, tmp_string[40], *token;
	int error, i, j, source_number_of_components, token_error, whole_field;
	cmzn_field *source_field;

	ENTER(Computed_field_composite_get_source_string);
	source_string = (char *)NULL;
	if (field)
	{
		error = 0;
		for (i = 0; (i < field->number_of_components) && !error; i++)
		{
			if (0 < i)
			{
				append_string(&source_string, " ", &error);
			}
			if (0 <= source_field_numbers[i])
			{
				source_field = field->source_fields[source_field_numbers[i]];
				/* source field component */
				if (GET_NAME(Computed_field)(source_field, &token))
				{
					token_error = 0;
					source_number_of_components = source_field->number_of_components;
					whole_field = 1;
					for (j = 0; whole_field && (j < source_number_of_components); j++)
					{
						whole_field = ((i + j) < field->number_of_components) &&
							(j == source_value_numbers[i + j]);
					}
					if (whole_field)
					{
						i += source_number_of_components - 1;
					}
					else
					{
						component_name = cmzn_field_get_component_name(source_field, source_value_numbers[i] + 1);
						if (component_name != 0)
						{
							append_string(&token, ".", &token_error);
							append_string(&token, component_name, &token_error);
							DEALLOCATE(component_name);
						}
					}
					if (commands)
					{
						make_valid_token(&token);
					}
					append_string(&source_string, token, &error);
					DEALLOCATE(token);
				}
			}
			else
			{
				/* source value */
				sprintf(tmp_string, "%g",
					field->source_values[source_value_numbers[i]]);
				append_string(&source_string, tmp_string, &error);
			}
		}
		if (error)
		{
			display_message(ERROR_MESSAGE,
				"Computed_field_composite_get_source_string.  Invalid argument(s)");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_composite_get_source_string.  Invalid argument(s)");
	}
	LEAVE;

	return (source_string);
} /* Computed_field_composite_get_source_string */

int Computed_field_composite::list()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
==============================================================================*/
{
	char *source_string;
	int return_code;

	ENTER(List_Computed_field_composite);
	if (field)
	{
		source_string = get_source_string(/*commands*/0);
		if (source_string != 0)
		{
			display_message(INFORMATION_MESSAGE,"    source data : %s\n",
				source_string);
			DEALLOCATE(source_string);
			return_code = 1;
		}
		else
		{
			display_message(ERROR_MESSAGE,"list_Computed_field_composite.  Failed");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_composite.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_composite */

char *Computed_field_composite::get_command_string()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *source_string;
	int error;

	ENTER(Computed_field_composite::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		/* could restore constant/component types here too */
		append_string(&command_string,
			computed_field_composite_type_string, &error);
		append_string(&command_string, " ", &error);
		source_string = get_source_string(/*commands*/1);
		if (source_string != 0)
		{
			append_string(&command_string, source_string, &error);
			DEALLOCATE(source_string);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_composite::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_composite::get_command_string */

inline Computed_field_composite *Computed_field_composite_core_cast(
	cmzn_field_component_id composite_field)
{
	if (composite_field)
		return (static_cast<Computed_field_composite*>(
			reinterpret_cast<cmzn_field*>(composite_field)->core));
	return 0;
}

} //namespace

cmzn_field *cmzn_fieldmodule_create_field_composite(
	cmzn_fieldmodule *field_module,
	int number_of_components,
	int number_of_source_fields, cmzn_field **source_fields,
	int number_of_source_values, const double *source_values,
	const int *source_field_numbers, const int *source_value_numbers)
{
	int i, j, return_code, source_field_number, source_value_number;
	cmzn_field *field, *source_field;

	ENTER(cmzn_fieldmodule_create_field_composite);
	field = nullptr;
	if ((0<number_of_components) &&
		((0==number_of_source_fields) ||
			((0<number_of_source_fields) && source_fields)) &&
		((0==number_of_source_values) ||
			((0<number_of_source_values) && source_values)) &&
		source_field_numbers && source_value_numbers)
	{
		return_code=1;
		/* check all source fields exist and are not repeated */
		for (i=0;i<number_of_source_fields;i++)
		{
			if (source_fields[i] && source_fields[i]->isNumerical())
			{
				for (j=0;j<i;j++)
				{
					if (source_fields[j] == source_fields[i])
					{
						display_message(ERROR_MESSAGE,
							"cmzn_fieldmodule_create_field_composite.  Repeated source field");
						return_code=0;
					}
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"cmzn_fieldmodule_create_field_composite.  Missing or non-numerical source field");
				return 0;
			}
		}
		if (return_code)
		{
			/* Check source_field_numbers refer to existing source_fields or equal -1.
				 Check components or source_values exist for the source_value_numbers
				 depending on whether field component or value specified. Also
				 ensure that values and fields are first used in the order they
				 appear in the source_values and source_fields */
			source_field_number=0;
			source_value_number=0;
			for (i=0;i<number_of_components;i++)
			{
				if (-1 == source_field_numbers[i])
				{
					/* source value numbers must be non-repeating and consecutive */
					if (source_value_number != source_value_numbers[i])
					{
						display_message(ERROR_MESSAGE,
							"cmzn_fieldmodule_create_field_composite.  "
							"Source value numbers out of order");
						return_code=0;
					}
					source_value_number++;
				}
				else if ((0 <= source_field_numbers[i]) &&
					(source_field_numbers[i] < number_of_source_fields))
				{
					if (source_field_numbers[i] <= source_field_number)
					{
						if (source_field_numbers[i] == source_field_number)
						{
							source_field_number++;
						}
						source_field = source_fields[source_field_numbers[i]];
						if ((0 > source_value_numbers[i]) ||
							(source_value_numbers[i] >= source_field->number_of_components))
						{
							display_message(ERROR_MESSAGE,
								"cmzn_fieldmodule_create_field_composite.  "
								"Component %d is out of range for field %s",
								source_value_numbers[i],source_field->name);
							return_code=0;
						}
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"cmzn_fieldmodule_create_field_composite.  "
							"source fields not used in order");
						return_code=0;
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"cmzn_fieldmodule_create_field_composite.  "
						"Invalid source field number %d",source_field_numbers[i]);
					return_code=0;
				}
			}
			if (source_field_number < number_of_source_fields)
			{
				display_message(ERROR_MESSAGE,
					"cmzn_fieldmodule_create_field_composite.  Not all source fields used");
				return_code=0;
			}
			if (source_value_number < number_of_source_values)
			{
				display_message(ERROR_MESSAGE,
					"cmzn_fieldmodule_create_field_composite.  Not all source values used");
				return_code=0;
			}
		}
		if (return_code)
		{
			field = Computed_field_create_generic(field_module,
				/*check_source_field_regions*/true,
				number_of_components,
				number_of_source_fields, source_fields,
				number_of_source_values, source_values,
				new Computed_field_composite(number_of_components,
					source_field_numbers, source_value_numbers));
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_composite.  Invalid argument(s)");
	}
	LEAVE;

	return (field);
} /* cmzn_fieldmodule_create_field_composite */

int Computed_field_get_type_composite(cmzn_field *field,
	int *number_of_components,
	int *number_of_source_fields,cmzn_field ***source_fields,
	int *number_of_source_values, double **source_values,
	int **source_field_numbers,int **source_value_numbers)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
If the field is of type composite, its source data is returned.
Note that <source_fields> and <source_values> are ALLOCATED (or NULL if none),
and <source_field_numbers> and <source_value_numbers> are always allocated on
return, and it is therefore up to the calling function to deallocate.
Note returned fields are not allocated in arrays.
==============================================================================*/
{
	int i, return_code;
	Computed_field_composite* composite_core;

	ENTER(Computed_field_get_type_composite);
	if (field &&
		(composite_core = dynamic_cast<Computed_field_composite*>(field->core)) &&
		number_of_components &&
		number_of_source_fields && source_fields &&
		number_of_source_values && source_values &&
		source_field_numbers && source_value_numbers)
	{
		*number_of_components = field->number_of_components;
		*number_of_source_fields = field->number_of_source_fields;
		*source_fields = (cmzn_field **)NULL;
		*number_of_source_values = field->number_of_source_values;
		*source_values = (double *)NULL;
		*source_field_numbers = (int *)NULL;
		*source_value_numbers = (int *)NULL;
		if (((0 == field->number_of_source_fields) || ALLOCATE(*source_fields,
			cmzn_field *, field->number_of_source_fields)) &&
			((0 == field->number_of_source_values) || ALLOCATE(*source_values,
				double, field->number_of_source_values)) &&
			ALLOCATE(*source_field_numbers, int, field->number_of_components) &&
			ALLOCATE(*source_value_numbers, int, field->number_of_components))
		{
			for (i = 0; i < field->number_of_source_fields; i++)
			{
				(*source_fields)[i] = field->source_fields[i];
			}
			for (i = 0; i < field->number_of_source_values; i++)
			{
				(*source_values)[i] = field->source_values[i];
			}
			for (i = 0; i < field->number_of_components; i++)
			{
				(*source_field_numbers)[i] = composite_core->source_field_numbers[i];
				(*source_value_numbers)[i] = composite_core->source_value_numbers[i];
			}
			return_code = 1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Computed_field_get_type_composite.  Not enough memory");
			DEALLOCATE(*source_fields);
			DEALLOCATE(*source_values);
			DEALLOCATE(*source_field_numbers);
			DEALLOCATE(*source_value_numbers);
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_composite.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_composite */

cmzn_field *cmzn_fieldmodule_create_field_constant(
	struct cmzn_fieldmodule *field_module,
	int number_of_values, const double *values)
/*******************************************************************************
LAST MODIFIED : 15 May 2008

DESCRIPTION :
Creates a constructor for COMPUTED_FIELD_COMPOSITE which has constant
components of the <number_of_values> listed in <values> array.
Since a constant field performs a subset of the capabilities of the composite
field type but the latter is somewhat complicated to set up, this is a
convenience function for building a composite field which has <number_of_values>
<values>.
==============================================================================*/
{
	int i, *source_field_numbers, *source_value_numbers;
	cmzn_field *field = nullptr;

	ENTER(cmzn_fieldmodule_create_field_constant);
	if ((0 < number_of_values) && values)
	{
		ALLOCATE(source_field_numbers, int, number_of_values);
		ALLOCATE(source_value_numbers, int, number_of_values);
		if (source_field_numbers && source_value_numbers)
		{
			for (i = 0; i < number_of_values; i++)
			{
				source_field_numbers[i] = -1;
				source_value_numbers[i] = i;
			}
			field = cmzn_fieldmodule_create_field_composite(field_module,
				/*number_of_components*/number_of_values,
				/*number_of_source_fields*/0,
				/*source_fields*/(cmzn_field **)NULL,
				/*number_of_source_values*/number_of_values,
				/*source_values*/values,
				source_field_numbers, source_value_numbers);
			if (field && field->core)
			{
				Computed_field_composite *fieldComposite= static_cast<Computed_field_composite*>(
					field->core);
				fieldComposite->type = CMZN_FIELD_TYPE_CONSTANT;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"cmzn_fieldmodule_create_field_constant.  Not enough memory");
		}
		DEALLOCATE(source_field_numbers);
		DEALLOCATE(source_value_numbers);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_constant.  Invalid argument(s)");
	}
	LEAVE;

	return (field);
} /* cmzn_fieldmodule_create_field_constant */

FE_value *Computed_field_constant_get_values_storage(cmzn_field *field)
{
	if (Computed_field_is_constant(field))
		return field->source_values;
	return 0;
}

int Computed_field_is_constant(cmzn_field *field)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Returns true if the field is of type composite but with no source_fields, only
source_values.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_is_constant);
	if (field)
	{
		return_code =
			(dynamic_cast<Computed_field_composite*>(field->core)) &&
			(0 == field->number_of_source_fields);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_is_constant.  Missing field");
		return_code = 0;
	}

	return (return_code);
} /* Computed_field_is_constant */

int Computed_field_is_constant_scalar(cmzn_field *field,
	FE_value scalar)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Returns true if the field is of type composite but with no source_fields, only
a single source value, equal to <scalar>.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_is_constant_scalar);
	if (field)
	{
		return_code =
			(dynamic_cast<Computed_field_composite*>(field->core)) &&
			(0 == field->number_of_source_fields) &&
			(1 == field->number_of_components) &&
			(scalar == field->source_values[0]);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_is_constant_scalar.  Missing field");
		return_code = 0;
	}

	return (return_code);
} /* Computed_field_is_constant_scalar */

int Computed_field_is_component_wrapper(cmzn_field *field,
	void *field_component_void)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Returns true if the field is of type composite with a single component matching
the passed Computed_field_component.
==============================================================================*/
{
	Computed_field_composite *composite_core;
	int return_code;
	struct Computed_field_component *field_component;

	ENTER(Computed_field_is_component_wrapper);
	if (field && (field_component =
		(struct Computed_field_component *)field_component_void))
	{
		return_code =
			(composite_core = dynamic_cast<Computed_field_composite*>(field->core)) &&
			(1 == field->number_of_components) &&
			(1 == field->number_of_source_fields) &&
			(field_component->field == field->source_fields[0]) &&
			(0 == composite_core->source_field_numbers[0]) &&
			(field_component->component_no == composite_core->source_value_numbers[0]);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_is_component_wrapper.  Missing field");
		return_code=0;
	}

	return (return_code);
} /* Computed_field_is_component_wrapper */

cmzn_field *Computed_field_manager_get_component_wrapper(
	struct MANAGER(Computed_field) *computed_field_manager,
	cmzn_field *field, int component_number)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Returns a composite field that computes <component_number> of <field>. First
tries to find one in the manager that does this, otherwise makes one of name
'field.component_name', adds it to the manager and returns it.
Returned field is ACCESSed once.
==============================================================================*/
{
	char *component_field_name, *component_name;
	cmzn_field *component_field;
	struct Computed_field_component field_component;

	ENTER(Computed_field_manager_get_component_wrapper);
	component_field = (cmzn_field *)NULL;
	if (computed_field_manager && field && (0 <= component_number) &&
		(component_number < field->number_of_components))
	{
		field_component.field = field;
		field_component.component_no = component_number;
		/* try to find an existing wrapper for this component */
		component_field = FIRST_OBJECT_IN_MANAGER_THAT(Computed_field)(
			Computed_field_is_component_wrapper, &field_component,
			computed_field_manager);
		if (component_field != 0)
		{
			ACCESS(Computed_field)(component_field);
		}
		else
		{
			component_name = cmzn_field_get_component_name(field, component_number + 1);
			if (component_name != 0)
			{
				if (ALLOCATE(component_field_name, char,
					strlen(field->name) + strlen(component_name) + 2))
				{
					sprintf(component_field_name, "%s.%s", field->name, component_name);
					cmzn_region* region = Computed_field_get_region(field);
					cmzn_fieldmodule *field_module = cmzn_fieldmodule_create(region);
					cmzn_fieldmodule_set_field_name(field_module, component_field_name);
					component_field = cmzn_fieldmodule_create_field_component(field_module, field, component_number + 1);
					cmzn_fieldmodule_destroy(&field_module);
					DEALLOCATE(component_field_name);
				}
				DEALLOCATE(component_name);
			}
			if (!component_field)
			{
				display_message(WARNING_MESSAGE,
					"Computed_field_manager_get_component_wrapper.  Failed");
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_manager_get_component_wrapper.  Invalid argument(s)");
	}
	LEAVE;

	return (component_field);
} /* Computed_field_manager_get_component_wrapper */

cmzn_field *cmzn_fieldmodule_create_field_identity(
	struct cmzn_fieldmodule *field_module,
	cmzn_field *source_field)
/*******************************************************************************
LAST MODIFIED : 21 April 2008

DESCRIPTION :
Changes <field> into type composite with one input field, the <source_field>.
==============================================================================*/
{
	int i, number_of_values, *source_field_numbers,
		*source_value_numbers;
	cmzn_field *field = nullptr;

	ENTER(cmzn_fieldmodule_create_field_identity);
	if (source_field && source_field->isNumerical())
	{
		number_of_values = source_field->number_of_components;
		ALLOCATE(source_field_numbers, int, number_of_values);
		ALLOCATE(source_value_numbers, int, number_of_values);
		if (source_field_numbers && source_value_numbers)
		{
			for (i = 0; i < number_of_values; i++)
			{
				source_field_numbers[i] = 0;
				source_value_numbers[i] = i;
			}
			field =
				cmzn_fieldmodule_create_field_composite(field_module,
				/*number_of_components*/number_of_values,
				/*number_of_source_fields*/1, /*source_fields*/&source_field,
				/*number_of_source_values*/0, /*source_values*/(double *)NULL,
				source_field_numbers, source_value_numbers);
			if (field && field->core)
			{
				Computed_field_composite *fieldComposite= static_cast<Computed_field_composite*>(
					field->core);
				fieldComposite->type = CMZN_FIELD_TYPE_IDENTITY;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"cmzn_fieldmodule_create_field_identity.  Not enough memory");
		}
		DEALLOCATE(source_field_numbers);
		DEALLOCATE(source_value_numbers);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_identity.  Invalid argument(s)");
	}
	LEAVE;

	return (field);
} /* cmzn_fieldmodule_create_field_identity */

cmzn_field_id cmzn_fieldmodule_create_field_component(
	cmzn_fieldmodule_id field_module, cmzn_field_id source_field,
	int source_component_index)
{
	cmzn_field_id field = 0;
	if (source_field && source_field->isNumerical() && (0 < source_component_index) &&
		(source_component_index <= cmzn_field_get_number_of_components(source_field)))
	{
		const int source_field_number = 0;
		const int source_value_number = source_component_index - 1; // external numbering starts at 1
		field =
			cmzn_fieldmodule_create_field_composite(field_module,
			/*number_of_components*/1,
			/*number_of_source_fields*/1, /*source_fields*/&source_field,
			/*number_of_source_values*/0, /*source_values*/(double *)0,
			&source_field_number, &source_value_number);
	}
	return field;
}

cmzn_field_id cmzn_fieldmodule_create_field_component_multiple(
	cmzn_fieldmodule_id field_module, cmzn_field_id source_field,
	int source_component_indexes_count, const int *source_component_indexes_in)
{
	cmzn_field *field = 0;
	if (source_field && source_field->isNumerical() &&
		(0 < source_component_indexes_count) && (source_component_indexes_in))
	{
		const int source_number_of_components = cmzn_field_get_number_of_components(source_field);
		for (int i = 0; i < source_component_indexes_count; ++i)
		{
			if ((source_component_indexes_in[i] < 1) || 
					(source_component_indexes_in[i] > source_number_of_components))
				return 0;
		}
		int *source_field_numbers;
		int *source_value_numbers;
		ALLOCATE(source_field_numbers, int, source_component_indexes_count);
		ALLOCATE(source_value_numbers, int, source_component_indexes_count);
		if ((source_value_numbers) && (source_value_numbers))
		{
			for (int i = 0; i < source_component_indexes_count; ++i)
			{
				source_field_numbers[i] = 0;
				source_value_numbers[i] = source_component_indexes_in[i] - 1; // external numbering starts at 1
			}
			field = cmzn_fieldmodule_create_field_composite(field_module,
				/*number_of_components*/source_component_indexes_count,
				/*number_of_source_fields*/1, /*source_fields*/&source_field,
				/*number_of_source_values*/0, /*source_values*/(double *)0,
				source_field_numbers, source_value_numbers);
		}
		DEALLOCATE(source_field_numbers);
		DEALLOCATE(source_value_numbers);
	}
	return field;
}

cmzn_field_id cmzn_fieldmodule_create_field_concatenate(
	cmzn_fieldmodule_id fieldmodule, int number_of_source_fields,
	cmzn_field_id *source_fields)
{
	cmzn_field *field = 0;
	if ((fieldmodule) && (source_fields) && (number_of_source_fields > 0))
	{
		int number_of_components = 0;
		for (int i = 0; i < number_of_source_fields; ++i)
		{
			if (!((source_fields[i]) && source_fields[i]->isNumerical()))
			{
				display_message(ERROR_MESSAGE, "Fieldmodule createFieldConcatenate.  Invalid source field");
				return 0;
			}
			number_of_components += cmzn_field_get_number_of_components(source_fields[i]);
		}
		std::vector<cmzn_field *> merged_source_fields(number_of_source_fields);
		std::vector<int> source_field_numbers(number_of_components);
		std::vector<int> source_value_numbers(number_of_components);
		int k = 0;
		int merged_source_fields_count = 0;
		for (int i = 0; i < number_of_source_fields; i++)
		{
			bool newSourceField = true;
			int merged_source_field_index = 0;
			while (merged_source_field_index < merged_source_fields_count)
			{
				if (source_fields[i] == merged_source_fields[merged_source_field_index])
				{
					newSourceField = false;
					break;
				}
				++merged_source_field_index;
			}
			if (newSourceField)
			{
				merged_source_fields[merged_source_field_index] = source_fields[i];
				++merged_source_fields_count;
			}
			const int number_of_components_per_field =
				cmzn_field_get_number_of_components(source_fields[i]);
			for (int j = 0; j < number_of_components_per_field; j++)
			{
				source_field_numbers[k + j] = merged_source_field_index;
				source_value_numbers[k + j] = j;
			}
			k += number_of_components_per_field;
		}
		field = cmzn_fieldmodule_create_field_composite(fieldmodule,
			number_of_components,
			merged_source_fields_count, merged_source_fields.data(),
			/*number_of_source_values*/0, /*source_values*/(double *)NULL,
			source_field_numbers.data(), source_value_numbers.data());
		if (field && field->core)
		{
			Computed_field_composite *fieldComposite= static_cast<Computed_field_composite*>(
				field->core);
			fieldComposite->type = CMZN_FIELD_TYPE_CONCATENATE;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE, "Fieldmodule createFieldConcatenate.  Invalid argument(s)");
	}
	return(field);
}

cmzn_field_component *cmzn_field_cast_component(cmzn_field_id field)
{
	if ((field) && (dynamic_cast<Computed_field_composite*>(field->core)))
	{
		// ensure composite field has one source field and no values
		if ((1 == field->number_of_source_fields) && (0 == field->number_of_source_values))
		{
			cmzn_field_access(field);
			return (reinterpret_cast<cmzn_field_component_id>(field));
		}
	}
	return nullptr;
}

int cmzn_field_component_get_component_index(cmzn_field_component_id component)
{
	Computed_field_composite *composite_core = Computed_field_composite_core_cast(component);
	if (composite_core)
		return composite_core->getSourceComponentIndex(0) + 1;
	return 0;
}

int cmzn_field_component_set_component_index(cmzn_field_component_id component, int source_component_index)
{
	Computed_field_composite *composite_core = Computed_field_composite_core_cast(component);
	if (composite_core)
		return composite_core->setSourceComponentIndex(0, source_component_index - 1);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_field_component_get_source_component_index(
	cmzn_field_component_id component, int index)
{
	Computed_field_composite *composite_core = Computed_field_composite_core_cast(component);
	if (composite_core)
		return composite_core->getSourceComponentIndex(index - 1) + 1;
	return 0;
}

int cmzn_field_component_set_source_component_index(
	cmzn_field_component_id component, int index, int source_component_index)
{
	Computed_field_composite *composite_core = Computed_field_composite_core_cast(component);
	if (composite_core)
		return composite_core->setSourceComponentIndex(index - 1, source_component_index - 1);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_field_component_destroy(cmzn_field_component_id *component_address)
{
	return cmzn_field_destroy(
		reinterpret_cast<cmzn_field_id *>(component_address));
}

cmzn_field_constant_id cmzn_field_cast_constant(cmzn_field_id field)
{
	if ((field) && (dynamic_cast<Computed_field_composite*>(field->core)))
	{
		if ((0 == field->number_of_source_fields) && (0 < field->number_of_source_values))
		{
			cmzn_field_access(field);
			return (reinterpret_cast<cmzn_field_constant_id>(field));
		}
	}
	return nullptr;
}
