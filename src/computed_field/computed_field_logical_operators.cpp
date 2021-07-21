/**
 * FILE : computed_field_logical_operators.cpp
 *
 * Implements a number of logical operations on computed fields.
 * Following operators are defined: AND, OR, XOR, NOT, EQUAL_TO, LESS_THAN,
 * GREATER_THAN, IS_DEFINED.
 */
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
#include "computed_field/computed_field_logical_operators.h"

class Computed_field_logical_operators_package : public Computed_field_type_package
{
};

namespace {

const char computed_field_or_type_string[] = "or";

class Computed_field_or : public Computed_field_core
{
public:
	Computed_field_or() : Computed_field_core()
	{
	};

private:
	Computed_field_core *copy()
	{
		return new Computed_field_or();
	}

	const char *get_type_string()
	{
		return(computed_field_or_type_string);
	}

	virtual enum cmzn_field_type get_type()
	{
		return CMZN_FIELD_TYPE_OR;
	}

	int compare(Computed_field_core* other_field)
	{
		if (dynamic_cast<Computed_field_or*>(other_field))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
	{
		inValueCache.getDerivativeValueCache(fieldDerivative)->zeroValues();
		return 1;
	}

};

int Computed_field_or::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
	const RealFieldValueCache *source1Cache = RealFieldValueCache::cast(getSourceField(0)->evaluate(cache));
	const RealFieldValueCache *source2Cache = RealFieldValueCache::cast(getSourceField(1)->evaluate(cache));
	if (source1Cache && source2Cache)
	{
		for (int i = 0 ; i < field->number_of_components ; i++)
		{
			valueCache.values[i] = (0.0 != source1Cache->values[i]) || (0.0 != source2Cache->values[i]);
		}
		return 1;
	}
	return 0;
}

} //namespace

Computed_field *cmzn_fieldmodule_create_field_or(
	struct cmzn_fieldmodule *field_module,
	struct Computed_field *source_field_one,
	struct Computed_field *source_field_two)
{
	Computed_field *field = NULL;
	/* Access and broadcast before checking components match,
		the local source_field_one and source_field_two will
		get replaced if necessary. */
	ACCESS(Computed_field)(source_field_one);
	ACCESS(Computed_field)(source_field_two);
	if (field_module && source_field_one && source_field_one->isNumerical() &&
		source_field_two && source_field_two->isNumerical() &&
		Computed_field_broadcast_field_components(field_module,
			&source_field_one, &source_field_two) &&
		(source_field_one->number_of_components ==
			source_field_two->number_of_components))
	{
		Computed_field *source_fields[2];
		source_fields[0] = source_field_one;
		source_fields[1] = source_field_two;
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field_one->number_of_components,
			/*number_of_source_fields*/2, source_fields,
			/*number_of_source_values*/0, NULL,
			new Computed_field_or());
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_or.  Invalid argument(s)");
	}
	DEACCESS(Computed_field)(&source_field_one);
	DEACCESS(Computed_field)(&source_field_two);

	return (field);
}

int Computed_field_get_type_or(struct Computed_field *field,
	struct Computed_field **source_field_one,
	struct Computed_field **source_field_two)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_XYZ, the
<source_field>  used by it are returned.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_get_type_or);
	if (field&&(dynamic_cast<Computed_field_or*>(field->core)))
	{
		*source_field_one = field->source_fields[0];
		*source_field_two = field->source_fields[1];
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_or.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_or */

namespace {

const char computed_field_and_type_string[] = "and";

class Computed_field_and : public Computed_field_core
{
public:
	Computed_field_and() : Computed_field_core()
	{
	};

private:
	Computed_field_core *copy()
	{
		return new Computed_field_and();
	}

	const char *get_type_string()
	{
		return(computed_field_and_type_string);
	}

	virtual enum cmzn_field_type get_type()
	{
		return CMZN_FIELD_TYPE_AND;
	}

	int compare(Computed_field_core* other_field)
	{
		if (dynamic_cast<Computed_field_and*>(other_field))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
	{
		inValueCache.getDerivativeValueCache(fieldDerivative)->zeroValues();
		return 1;
	}

};

int Computed_field_and::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
	const RealFieldValueCache *source1Cache = RealFieldValueCache::cast(getSourceField(0)->evaluate(cache));
	const RealFieldValueCache *source2Cache = RealFieldValueCache::cast(getSourceField(1)->evaluate(cache));
	if (source1Cache && source2Cache)
	{
		for (int i = 0 ; i < field->number_of_components ; i++)
		{
			valueCache.values[i] = (0.0 != source1Cache->values[i]) && (0.0 != source2Cache->values[i]);
		}
		return 1;
	}
	return 0;
}

} //namespace

Computed_field *cmzn_fieldmodule_create_field_and(
	struct cmzn_fieldmodule *field_module,
	struct Computed_field *source_field_one,
	struct Computed_field *source_field_two)
{
	Computed_field *field = NULL;
	/* Access and broadcast before checking components match,
		the local source_field_one and source_field_two will
		get replaced if necessary. */
	ACCESS(Computed_field)(source_field_one);
	ACCESS(Computed_field)(source_field_two);
	if (field_module && source_field_one && source_field_one->isNumerical() &&
		source_field_two && source_field_two->isNumerical() &&
		Computed_field_broadcast_field_components(field_module,
			&source_field_one, &source_field_two) &&
		(source_field_one->number_of_components ==
			source_field_two->number_of_components))
	{
		Computed_field *source_fields[2];
		source_fields[0] = source_field_one;
		source_fields[1] = source_field_two;
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field_one->number_of_components,
			/*number_of_source_fields*/2, source_fields,
			/*number_of_source_values*/0, NULL,
			new Computed_field_and());
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_or.  Invalid argument(s)");
	}
	DEACCESS(Computed_field)(&source_field_one);
	DEACCESS(Computed_field)(&source_field_two);

	return (field);
}

int Computed_field_get_type_and(struct Computed_field *field,
	struct Computed_field **source_field_one,
	struct Computed_field **source_field_two)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_AND, the
<source_field>  used by it are returned.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_get_type_and);
	if (field&&(dynamic_cast<Computed_field_and*>(field->core)))
	{
		*source_field_one = field->source_fields[0];
		*source_field_two = field->source_fields[1];
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_and.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_and */

namespace {

const char computed_field_xor_type_string[] = "xor";

class Computed_field_xor : public Computed_field_core
{
public:
	Computed_field_xor() : Computed_field_core()
	{
	};

private:
	Computed_field_core *copy()
	{
		return new Computed_field_xor();
	}

	const char *get_type_string()
	{
		return(computed_field_xor_type_string);
	}

	virtual enum cmzn_field_type get_type()
	{
		return CMZN_FIELD_TYPE_XOR;
	}

	int compare(Computed_field_core* other_field)
	{
		if (dynamic_cast<Computed_field_xor*>(other_field))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
	{
		inValueCache.getDerivativeValueCache(fieldDerivative)->zeroValues();
		return 1;
	}

};

int Computed_field_xor::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
	const RealFieldValueCache *source1Cache = RealFieldValueCache::cast(getSourceField(0)->evaluate(cache));
	const RealFieldValueCache *source2Cache = RealFieldValueCache::cast(getSourceField(1)->evaluate(cache));
	if (source1Cache && source2Cache)
	{
		for (int i = 0 ; i < field->number_of_components ; i++)
		{
			if (0.0 != source1Cache->values[i])
			{
				valueCache.values[i] = (0.0 == source2Cache->values[i]);
			}
			else
			{
				valueCache.values[i] = (0.0 != source2Cache->values[i]);
			}
		}
		return 1;
	}
	return 0;
}

} //namespace

Computed_field *cmzn_fieldmodule_create_field_xor(
	struct cmzn_fieldmodule *field_module,
	struct Computed_field *source_field_one,
	struct Computed_field *source_field_two)
{
	Computed_field *field = NULL;
	/* Access and broadcast before checking components match,
		the local source_field_one and source_field_two will
		get replaced if necessary. */
	ACCESS(Computed_field)(source_field_one);
	ACCESS(Computed_field)(source_field_two);
	if (field_module && source_field_one && source_field_one->isNumerical() &&
		source_field_two && source_field_two->isNumerical() &&
		Computed_field_broadcast_field_components(field_module,
			&source_field_one, &source_field_two) &&
		(source_field_one->number_of_components ==
			source_field_two->number_of_components))
	{
		Computed_field *source_fields[2];
		source_fields[0] = source_field_one;
		source_fields[1] = source_field_two;
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field_one->number_of_components,
			/*number_of_source_fields*/2, source_fields,
			/*number_of_source_values*/0, NULL,
			new Computed_field_xor());
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_or.  Invalid argument(s)");
	}
	DEACCESS(Computed_field)(&source_field_one);
	DEACCESS(Computed_field)(&source_field_two);

	return (field);
}

int Computed_field_get_type_xor(struct Computed_field *field,
	struct Computed_field **source_field_one,
	struct Computed_field **source_field_two)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_XOR, the
<source_field>  used by it are returned.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_get_type_xor);
	if (field&&(dynamic_cast<Computed_field_xor*>(field->core)))
	{
		*source_field_one = field->source_fields[0];
		*source_field_two = field->source_fields[1];
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_xor.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_xor */

namespace {

const char computed_field_equal_to_type_string[] = "equal_to";

class Computed_field_equal_to : public Computed_field_core
{
public:
	Computed_field_equal_to() : Computed_field_core()
	{
	};

private:
	Computed_field_core *copy()
	{
		return new Computed_field_equal_to();
	}

	const char *get_type_string()
	{
		return(computed_field_equal_to_type_string);
	}

	virtual enum cmzn_field_type get_type()
	{
		return CMZN_FIELD_TYPE_EQUAL_TO;
	}

	int compare(Computed_field_core* other_field)
	{
		if (dynamic_cast<Computed_field_equal_to*>(other_field))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
	{
		inValueCache.getDerivativeValueCache(fieldDerivative)->zeroValues();
		return 1;
	}
};

int Computed_field_equal_to::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
	switch (cmzn_field_get_value_type(getSourceField(0)))
	{
		case CMZN_FIELD_VALUE_TYPE_REAL:
		{
			const RealFieldValueCache *source1Cache = RealFieldValueCache::cast(getSourceField(0)->evaluate(cache));
			const RealFieldValueCache *source2Cache = RealFieldValueCache::cast(getSourceField(1)->evaluate(cache));
			if (source1Cache && source2Cache)
			{
				for (int i = 0 ; i < field->number_of_components ; i++)
				{
					valueCache.values[i] = (source1Cache->values[i] == source2Cache->values[i]) ? 1.0 : 0.0;
				}
				return 1;
			}
		} break;
		case CMZN_FIELD_VALUE_TYPE_STRING:
		{
			const StringFieldValueCache *source1Cache = StringFieldValueCache::cast(getSourceField(0)->evaluate(cache));
			const StringFieldValueCache *source2Cache = StringFieldValueCache::cast(getSourceField(1)->evaluate(cache));
			if (source1Cache && source2Cache)
			{
				FE_value result = (FE_value)(0 == strcmp(source1Cache->stringValue, source2Cache->stringValue));
				// should only be one component if string arguments
				for (int i = 0 ; i < field->number_of_components ; i++)
				{
					valueCache.values[i] = result;
				}
				return 1;
			}
		} break;
		default:
		{
			// unsupported type
		} break;
	}
	return 0;
}

} //namespace

Computed_field *cmzn_fieldmodule_create_field_equal_to(
	struct cmzn_fieldmodule *field_module,
	struct Computed_field *source_field_one,
	struct Computed_field *source_field_two)
{
	Computed_field *field = NULL;
	/* Access and broadcast before checking components match,
		the local source_field_one and source_field_two will
		get replaced if necessary. */
	ACCESS(Computed_field)(source_field_one);
	ACCESS(Computed_field)(source_field_two);
	if (field_module && source_field_one && source_field_two &&
		(cmzn_field_get_value_type(source_field_one) == cmzn_field_get_value_type(source_field_two)) &&
		Computed_field_broadcast_field_components(field_module,
			&source_field_one, &source_field_two) &&
		(source_field_one->number_of_components ==
			source_field_two->number_of_components))
	{
		Computed_field *source_fields[2];
		source_fields[0] = source_field_one;
		source_fields[1] = source_field_two;
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field_one->number_of_components,
			/*number_of_source_fields*/2, source_fields,
			/*number_of_source_values*/0, NULL,
			new Computed_field_equal_to());
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_or.  Invalid argument(s)");
	}
	DEACCESS(Computed_field)(&source_field_one);
	DEACCESS(Computed_field)(&source_field_two);

	return (field);
}

int Computed_field_get_type_equal_to(struct Computed_field *field,
	struct Computed_field **source_field_one,
	struct Computed_field **source_field_two)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_EQUAL_TO, the
<source_field>  used by it are returned.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_get_type_equal_to);
	if (field&&(dynamic_cast<Computed_field_equal_to*>(field->core)))
	{
		*source_field_one = field->source_fields[0];
		*source_field_two = field->source_fields[1];
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_equal_to.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_equal_to */

namespace {

const char computed_field_less_than_type_string[] = "less_than";

class Computed_field_less_than : public Computed_field_core
{
public:
	Computed_field_less_than() : Computed_field_core()
	{
	};

private:
	Computed_field_core *copy()
	{
		return new Computed_field_less_than();
	}

	const char *get_type_string()
	{
		return(computed_field_less_than_type_string);
	}

	virtual enum cmzn_field_type get_type()
	{
		return CMZN_FIELD_TYPE_LESS_THAN;
	}

	int compare(Computed_field_core* other_field)
	{
		if (dynamic_cast<Computed_field_less_than*>(other_field))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
	{
		inValueCache.getDerivativeValueCache(fieldDerivative)->zeroValues();
		return 1;
	}

};

int Computed_field_less_than::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
	const RealFieldValueCache *source1Cache = RealFieldValueCache::cast(getSourceField(0)->evaluate(cache));
	const RealFieldValueCache *source2Cache = RealFieldValueCache::cast(getSourceField(1)->evaluate(cache));
	if (source1Cache && source2Cache)
	{
		for (int i = 0 ; i < field->number_of_components ; i++)
		{
			valueCache.values[i] = (source1Cache->values[i] < source2Cache->values[i]) ? 1.0 : 0.0;
		}
		return 1;
	}
	return 0;
}

} //namespace

cmzn_field_id cmzn_fieldmodule_create_field_less_than(
	cmzn_fieldmodule_id field_module,
	cmzn_field_id source_field_one, cmzn_field_id source_field_two)
{
	cmzn_field *field = NULL;
	/* Access and broadcast before checking components match,
		the local source_field_one and source_field_two will
		get replaced if necessary. */
	ACCESS(cmzn_field)(source_field_one);
	ACCESS(cmzn_field)(source_field_two);
	if (field_module && source_field_one  && source_field_one->isNumerical() &&
		source_field_two && source_field_two->isNumerical() &&
		Computed_field_broadcast_field_components(field_module,
			&source_field_one, &source_field_two) &&
		(source_field_one->number_of_components ==
			source_field_two->number_of_components))
	{
		cmzn_field *source_fields[2];
		source_fields[0] = source_field_one;
		source_fields[1] = source_field_two;
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field_one->number_of_components,
			/*number_of_source_fields*/2, source_fields,
			/*number_of_source_values*/0, NULL,
			new Computed_field_less_than());
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_less_than.  Invalid argument(s)");
	}
	DEACCESS(cmzn_field)(&source_field_one);
	DEACCESS(cmzn_field)(&source_field_two);
	return (field);
}

int Computed_field_get_type_less_than(struct Computed_field *field,
	struct Computed_field **source_field_one,
	struct Computed_field **source_field_two)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_LESS_THAN, the
<source_field>  used by it are returned.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_get_type_less_than);
	if (field&&(dynamic_cast<Computed_field_less_than*>(field->core)))
	{
		*source_field_one = field->source_fields[0];
		*source_field_two = field->source_fields[1];
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_less_than.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_less_than */

namespace {

const char computed_field_greater_than_type_string[] = "greater_than";

class Computed_field_greater_than : public Computed_field_core
{
public:
	Computed_field_greater_than() : Computed_field_core()
	{
	};

private:
	Computed_field_core *copy()
	{
		return new Computed_field_greater_than();
	}

	const char *get_type_string()
	{
		return(computed_field_greater_than_type_string);
	}

	virtual enum cmzn_field_type get_type()
	{
		return CMZN_FIELD_TYPE_GREATER_THAN;
	}

	int compare(Computed_field_core* other_field)
	{
		if (dynamic_cast<Computed_field_greater_than*>(other_field))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
	{
		inValueCache.getDerivativeValueCache(fieldDerivative)->zeroValues();
		return 1;
	}

};

int Computed_field_greater_than::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
	const RealFieldValueCache *source1Cache = RealFieldValueCache::cast(getSourceField(0)->evaluate(cache));
	const RealFieldValueCache *source2Cache = RealFieldValueCache::cast(getSourceField(1)->evaluate(cache));
	if (source1Cache && source2Cache)
	{
		for (int i = 0 ; i < field->number_of_components ; i++)
		{
			valueCache.values[i] = (source1Cache->values[i] > source2Cache->values[i]) ? 1.0 : 0.0;
		}
		return 1;
	}
	return 0;
}

} //namespace

cmzn_field_id cmzn_fieldmodule_create_field_greater_than(
	cmzn_fieldmodule_id field_module,
	cmzn_field_id source_field_one, cmzn_field_id source_field_two)
{
	cmzn_field *field = NULL;
	/* Access and broadcast before checking components match,
		the local source_field_one and source_field_two will
		get replaced if necessary. */
	ACCESS(cmzn_field)(source_field_one);
	ACCESS(cmzn_field)(source_field_two);
	if (field_module && source_field_one  && source_field_one->isNumerical() &&
		source_field_two && source_field_two->isNumerical() &&
		Computed_field_broadcast_field_components(field_module,
			&source_field_one, &source_field_two) &&
		(source_field_one->number_of_components ==
			source_field_two->number_of_components))
	{
		cmzn_field *source_fields[2];
		source_fields[0] = source_field_one;
		source_fields[1] = source_field_two;
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field_one->number_of_components,
			/*number_of_source_fields*/2, source_fields,
			/*number_of_source_values*/0, NULL,
			new Computed_field_greater_than());
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_greater_than.  Invalid argument(s)");
	}
	DEACCESS(cmzn_field)(&source_field_one);
	DEACCESS(cmzn_field)(&source_field_two);
	return (field);
}

int Computed_field_get_type_greater_than(struct Computed_field *field,
	struct Computed_field **source_field_one,
	struct Computed_field **source_field_two)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_GREATER_THAN, the
<source_field>  used by it are returned.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_get_type_greater_than);
	if (field&&(dynamic_cast<Computed_field_greater_than*>(field->core)))
	{
		*source_field_one = field->source_fields[0];
		*source_field_two = field->source_fields[1];
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_greater_than.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_greater_than */

namespace {

const char computed_field_is_defined_type_string[] = "is_defined";

class Computed_field_is_defined : public Computed_field_core
{
public:
	Computed_field_is_defined() : Computed_field_core()
	{
	};

private:
	Computed_field_core *copy()
	{
		return new Computed_field_is_defined();
	}

	const char *get_type_string()
	{
		return(computed_field_is_defined_type_string);
	}

	virtual enum cmzn_field_type get_type()
	{
		return CMZN_FIELD_TYPE_IS_DEFINED;
	}

	int compare(Computed_field_core* other_field)
	{
		if (dynamic_cast<Computed_field_is_defined*>(other_field))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
	{
		inValueCache.getDerivativeValueCache(fieldDerivative)->zeroValues();
		return 1;
	}

	virtual bool is_defined_at_location(cmzn_fieldcache&)
	{
		return true;  // field can be evaluated everywhere to true or false
	}

};

int Computed_field_is_defined::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache& valueCache = RealFieldValueCache::cast(inValueCache);
	valueCache.values[0] = getSourceField(0)->core->is_defined_at_location(cache);
	return 1;
}

} //namespace

cmzn_field_id cmzn_fieldmodule_create_field_is_defined(
	cmzn_fieldmodule_id field_module, cmzn_field_id source_field)
{
	return Computed_field_create_generic(field_module,
		/*check_source_field_regions*/true,
		/*number_of_components*/1,
		/*number_of_source_fields*/1, &source_field,
		/*number_of_source_values*/0, NULL,
		new Computed_field_is_defined());
}

int Computed_field_get_type_is_defined(struct Computed_field *field,
	struct Computed_field **source_field)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_IS_DEFINED, the
<source_field>  used by it are returned.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_get_type_is_defined);
	if (field&&(dynamic_cast<Computed_field_is_defined*>(field->core)))
	{
		*source_field = field->source_fields[0];
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_is_defined.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_is_defined */

namespace {

const char computed_field_not_type_string[] = "not";

class Computed_field_not : public Computed_field_core
{
public:
	Computed_field_not() : Computed_field_core()
	{
	};

private:
	Computed_field_core *copy()
	{
		return new Computed_field_not();
	}

	const char *get_type_string()
	{
		return (computed_field_not_type_string);
	}

	virtual enum cmzn_field_type get_type()
	{
		return CMZN_FIELD_TYPE_NOT;
	}

	int compare(Computed_field_core* other_field)
	{
		return (0 != dynamic_cast<Computed_field_not*>(other_field));
	}

	virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
	{
		inValueCache.getDerivativeValueCache(fieldDerivative)->zeroValues();
		return 1;
	}

};

int Computed_field_not::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
	const RealFieldValueCache *sourceCache = RealFieldValueCache::cast(getSourceField(0)->evaluate(cache));
	if (sourceCache)
	{
		for (int i = 0 ; i < field->number_of_components ; i++)
		{
			valueCache.values[i] = (0.0 != sourceCache->values[i]) ? 0.0 : 1.0;
		}
		return 1;
	}
	return 0;
}

} //namespace

cmzn_field_id cmzn_fieldmodule_create_field_not(cmzn_fieldmodule_id field_module,
	cmzn_field_id source_field)
{
	cmzn_field_id field = 0;
	if (source_field && source_field->isNumerical())
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field->number_of_components,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_not());
	}
	return field;
}

