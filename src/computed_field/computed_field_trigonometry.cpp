/*******************************************************************************
FILE : computed_field_trigonometry.c

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
#include "computed_field/computed_field_trigonometry.h"

class Computed_field_trigonometry_package : public Computed_field_type_package
{
};

namespace {

const char computed_field_sin_type_string[] = "sin";

class Computed_field_sin : public Computed_field_core
{
public:
	Computed_field_sin() : Computed_field_core()
	{
	};

private:
	Computed_field_core *copy()
	{
		return new Computed_field_sin();
	}

	const char *get_type_string()
	{
		return(computed_field_sin_type_string);
	}

	int compare(Computed_field_core* other_field)
	{
		if (dynamic_cast<Computed_field_sin*>(other_field))
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

int Computed_field_sin::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	const RealFieldValueCache *sourceCache = RealFieldValueCache::cast(getSourceField(0)->evaluate(cache));
	if (sourceCache)
	{
		RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
		for (int i = 0; i < field->number_of_components; ++i)
			valueCache.values[i] = sin(sourceCache->values[i]);
		return 1;
	}
	return 0;
}

int Computed_field_sin::evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
{
	if (fieldDerivative.getTotalOrder() > 1)
		return this->evaluateDerivativeFiniteDifference(cache, inValueCache, fieldDerivative);
	const RealFieldValueCache *sourceCache = RealFieldValueCache::cast(getSourceField(0)->evaluateDerivativeTree(cache, fieldDerivative));
	if (sourceCache)
	{
		const FE_value *sourceDerivatives = sourceCache->getDerivativeValueCache(fieldDerivative)->values;
		DerivativeValueCache *derivativeCache = inValueCache.getDerivativeValueCache(fieldDerivative);
		FE_value *derivatives = derivativeCache->values;
		const int componentCount = this->field->number_of_components;
		const int termCount = derivativeCache->getTermCount();
		for (int i = 0 ; i < componentCount; ++i)
		{
			// d(sin u)/dx = cos u * du/dx
			const FE_value cos_u = cos(sourceCache->values[i]);
			for (int j = 0 ; j < termCount; ++j)
				derivatives[j] = cos_u * sourceDerivatives[j];
			derivatives += termCount;
			sourceDerivatives += termCount;
		}
		return 1;
	}
	return 0;
}

int Computed_field_sin::list()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_sin);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    source field : %s\n",field->source_fields[0]->name);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_sin.  Invalid field");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_sin */

char *Computed_field_sin::get_command_string()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name;
	int error;

	ENTER(Computed_field_sin::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_sin_type_string, &error);
		append_string(&command_string, " field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_sin::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_sin::get_command_string */

} //namespace

cmzn_field *cmzn_fieldmodule_create_field_sin(
	struct cmzn_fieldmodule *field_module,
	cmzn_field *source_field)
{
	cmzn_field_id field = 0;
	if (source_field && source_field->isNumerical())
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field->number_of_components,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_sin());
	}
	return (field);
}

int Computed_field_get_type_sin(cmzn_field *field,
	cmzn_field **source_field)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_SIN, the
<source_field> used by it are returned.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_get_type_sin);
	if (field&&(dynamic_cast<Computed_field_sin*>(field->core)))
	{
		*source_field = field->source_fields[0];
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_sin.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_sin */

namespace {

const char computed_field_cos_type_string[] = "cos";

class Computed_field_cos : public Computed_field_core
{
public:
	Computed_field_cos() : Computed_field_core()
	{
	};

private:
	Computed_field_core *copy()
	{
		return new Computed_field_cos();
	}

	const char *get_type_string()
	{
		return(computed_field_cos_type_string);
	}

	int compare(Computed_field_core* other_field)
	{
		if (dynamic_cast<Computed_field_cos*>(other_field))
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

int Computed_field_cos::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	const RealFieldValueCache *sourceCache = RealFieldValueCache::cast(getSourceField(0)->evaluate(cache));
	if (sourceCache)
	{
		RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
		for (int i = 0 ; i < field->number_of_components ; i++)
			valueCache.values[i] = cos(sourceCache->values[i]);
		return 1;
	}
	return 0;
}

int Computed_field_cos::evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
{
	if (fieldDerivative.getTotalOrder() > 1)
		return this->evaluateDerivativeFiniteDifference(cache, inValueCache, fieldDerivative);
	const RealFieldValueCache *sourceCache = RealFieldValueCache::cast(getSourceField(0)->evaluateDerivativeTree(cache, fieldDerivative));
	if (sourceCache)
	{
		const FE_value *sourceDerivatives = sourceCache->getDerivativeValueCache(fieldDerivative)->values;
		DerivativeValueCache *derivativeCache = inValueCache.getDerivativeValueCache(fieldDerivative);
		FE_value *derivatives = derivativeCache->values;
		const int componentCount = this->field->number_of_components;
		const int termCount = derivativeCache->getTermCount();
		for (int i = 0; i < componentCount; ++i)
		{
			// d(cos u)/dx = -sin u * du/dx
			const FE_value _sin_u = -sin(sourceCache->values[i]);
			for (int j = 0; j < termCount; ++j)
				derivatives[j] = _sin_u * sourceDerivatives[j];
			derivatives += termCount;
			sourceDerivatives += termCount;
		}
		return 1;
	}
	return 0;
}

int Computed_field_cos::list()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_cos);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    source field : %s\n",field->source_fields[0]->name);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_cos.  Invalid field");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_cos */

char *Computed_field_cos::get_command_string()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name;
	int error;

	ENTER(Computed_field_cos::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_cos_type_string, &error);
		append_string(&command_string, " field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_cos::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_cos::get_command_string */

} //namespace

cmzn_field *cmzn_fieldmodule_create_field_cos(
	struct cmzn_fieldmodule *field_module,
	cmzn_field *source_field)
{
	cmzn_field_id field = 0;
	if (source_field && source_field->isNumerical())
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field->number_of_components,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_cos());
	}
	return (field);
}

int Computed_field_get_type_cos(cmzn_field *field,
	cmzn_field **source_field)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_COS, the
<source_field> used by it are returned.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_get_type_cos);
	if (field&&(dynamic_cast<Computed_field_cos*>(field->core)))
	{
		*source_field = field->source_fields[0];
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_cos.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_cos */

namespace {

const char computed_field_tan_type_string[] = "tan";

class Computed_field_tan : public Computed_field_core
{
public:
	Computed_field_tan() : Computed_field_core()
	{
	};

private:
	Computed_field_core *copy()
	{
		return new Computed_field_tan();
	}

	const char *get_type_string()
	{
		return(computed_field_tan_type_string);
	}

	int compare(Computed_field_core* other_field)
	{
		if (dynamic_cast<Computed_field_tan*>(other_field))
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

int Computed_field_tan::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	const RealFieldValueCache *sourceCache = RealFieldValueCache::cast(getSourceField(0)->evaluate(cache));
	if (sourceCache)
	{
		RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
		for (int i = 0 ; i < field->number_of_components ; i++)
			valueCache.values[i] = tan(sourceCache->values[i]);
		return 1;
	}
	return 0;
}

int Computed_field_tan::evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
{
	if (fieldDerivative.getTotalOrder() > 1)
		return this->evaluateDerivativeFiniteDifference(cache, inValueCache, fieldDerivative);
	const RealFieldValueCache *sourceCache = RealFieldValueCache::cast(getSourceField(0)->evaluateDerivativeTree(cache, fieldDerivative));
	if (sourceCache)
	{
		const FE_value *sourceDerivatives = sourceCache->getDerivativeValueCache(fieldDerivative)->values;
		DerivativeValueCache *derivativeCache = inValueCache.getDerivativeValueCache(fieldDerivative);
		FE_value *derivatives = derivativeCache->values;
		const int componentCount = this->field->number_of_components;
		const int termCount = derivativeCache->getTermCount();
		for (int i = 0; i < componentCount; ++i)
		{
			// d(tan u)/dx = sec^2 u * du/dx
			const FE_value cos_u = cos(sourceCache->values[i]);
			const FE_value sec2_u = 1.0 / (cos_u*cos_u);
			for (int j = 0; j < termCount; ++j)
				derivatives[j] = sec2_u * sourceDerivatives[j];
			derivatives += termCount;
			sourceDerivatives += termCount;
		}
		return 1;
	}
	return 0;
}
int Computed_field_tan::list()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_tan);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    source field : %s\n",field->source_fields[0]->name);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_tan.  Invalid field");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_tan */

char *Computed_field_tan::get_command_string()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name;
	int error;

	ENTER(Computed_field_tan::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_tan_type_string, &error);
		append_string(&command_string, " field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_tan::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_tan::get_command_string */

} //namespace

cmzn_field *cmzn_fieldmodule_create_field_tan(
	struct cmzn_fieldmodule *field_module,
	cmzn_field *source_field)
{
	cmzn_field_id field = 0;
	if (source_field && source_field->isNumerical())
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field->number_of_components,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_tan());
	}
	return (field);
}

int Computed_field_get_type_tan(cmzn_field *field,
	cmzn_field **source_field)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_TAN, the
<source_field> used by it are returned.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_get_type_tan);
	if (field&&(dynamic_cast<Computed_field_tan*>(field->core)))
	{
		*source_field = field->source_fields[0];
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_tan.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_tan */

namespace {

const char computed_field_asin_type_string[] = "asin";

class Computed_field_asin : public Computed_field_core
{
public:
	Computed_field_asin() : Computed_field_core()
	{
	};

private:
	Computed_field_core *copy()
	{
		return new Computed_field_asin();
	}

	const char *get_type_string()
	{
		return(computed_field_asin_type_string);
	}

	int compare(Computed_field_core* other_field)
	{
		if (dynamic_cast<Computed_field_asin*>(other_field))
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

int Computed_field_asin::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	const RealFieldValueCache *sourceCache = RealFieldValueCache::cast(getSourceField(0)->evaluate(cache));
	if (sourceCache)
	{
		RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
		for (int i = 0; i < field->number_of_components; i++)
			valueCache.values[i] = asin(sourceCache->values[i]);
		return 1;
	}
	return 0;
}

int Computed_field_asin::evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
{
	if (fieldDerivative.getTotalOrder() > 1)
		return this->evaluateDerivativeFiniteDifference(cache, inValueCache, fieldDerivative);
	const RealFieldValueCache *sourceCache = RealFieldValueCache::cast(getSourceField(0)->evaluateDerivativeTree(cache, fieldDerivative));
	if (sourceCache)
	{
		const FE_value *sourceDerivatives = sourceCache->getDerivativeValueCache(fieldDerivative)->values;
		DerivativeValueCache *derivativeCache = inValueCache.getDerivativeValueCache(fieldDerivative);
		FE_value *derivatives = derivativeCache->values;
		const int componentCount = this->field->number_of_components;
		const int termCount = derivativeCache->getTermCount();
		for (int i = 0; i < componentCount; ++i)
		{
			// d(asin u)/dx = 1.0/sqrt(1.0 - u^2) * du/dx
			// avoid division by zero - make derivative zero there
			const FE_value u = sourceCache->values[i];
			const FE_value one__sqrt_1_u2 = (u != 1.0) ? 1.0/sqrt(1.0 - u*u) : 0.0;
			for (int j = 0; j < termCount; ++j)
				derivatives[j] = one__sqrt_1_u2 * sourceDerivatives[j];
			derivatives += termCount;
			sourceDerivatives += termCount;
		}
		return 1;
	}
	return 0;
}

int Computed_field_asin::list()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_asin);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    source field : %s\n",field->source_fields[0]->name);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_asin.  Invalid field");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_asin */

char *Computed_field_asin::get_command_string()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name;
	int error;

	ENTER(Computed_field_asin::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_asin_type_string, &error);
		append_string(&command_string, " field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_asin::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_asin::get_command_string */

} //namespace

cmzn_field *cmzn_fieldmodule_create_field_asin(
	struct cmzn_fieldmodule *field_module,
	cmzn_field *source_field)
{
	cmzn_field_id field = 0;
	if (source_field && source_field->isNumerical())
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field->number_of_components,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_asin());
	}
	return (field);
}

int Computed_field_get_type_asin(cmzn_field *field,
	cmzn_field **source_field)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_ASIN, the
<source_field> used by it are returned.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_get_type_asin);
	if (field&&(dynamic_cast<Computed_field_asin*>(field->core)))
	{
		*source_field = field->source_fields[0];
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_asin.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_asin */

namespace {

const char computed_field_acos_type_string[] = "acos";

class Computed_field_acos : public Computed_field_core
{
public:
	Computed_field_acos() : Computed_field_core()
	{
	};

private:
	Computed_field_core *copy()
	{
		return new Computed_field_acos();
	}

	const char *get_type_string()
	{
		return(computed_field_acos_type_string);
	}

	int compare(Computed_field_core* other_field)
	{
		if (dynamic_cast<Computed_field_acos*>(other_field))
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

int Computed_field_acos::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	const RealFieldValueCache *sourceCache = RealFieldValueCache::cast(getSourceField(0)->evaluate(cache));
	if (sourceCache)
	{
		RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
		for (int i = 0; i < field->number_of_components; i++)
			valueCache.values[i] = acos(sourceCache->values[i]);
		return 1;
	}
	return 0;
}

int Computed_field_acos::evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
{
	if (fieldDerivative.getTotalOrder() > 1)
		return this->evaluateDerivativeFiniteDifference(cache, inValueCache, fieldDerivative);
	const RealFieldValueCache *sourceCache = RealFieldValueCache::cast(getSourceField(0)->evaluateDerivativeTree(cache, fieldDerivative));
	if (sourceCache)
	{
		const FE_value *sourceDerivatives = sourceCache->getDerivativeValueCache(fieldDerivative)->values;
		DerivativeValueCache *derivativeCache = inValueCache.getDerivativeValueCache(fieldDerivative);
		FE_value *derivatives = derivativeCache->values;
		const int componentCount = this->field->number_of_components;
		const int termCount = derivativeCache->getTermCount();
		for (int i = 0; i < componentCount; ++i)
		{
			// d(acos u)/dx = -1.0/sqrt(1.0 - u^2) * du/dx
			// avoid division by zero - make derivative zero there
			const FE_value u = sourceCache->values[i];
			const FE_value _one__sqrt_1_u2 = (u != 1.0) ? -1.0/sqrt(1.0 - u*u) : 0.0;
			for (int j = 0; j < termCount; ++j)
				derivatives[j] = _one__sqrt_1_u2 * sourceDerivatives[j];
			derivatives += termCount;
			sourceDerivatives += termCount;
		}
		return 1;
	}
	return 0;
}

int Computed_field_acos::list()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_acos);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    source field : %s\n",field->source_fields[0]->name);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_acos.  Invalid field");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_acos */

char *Computed_field_acos::get_command_string()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name;
	int error;

	ENTER(Computed_field_acos::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_acos_type_string, &error);
		append_string(&command_string, " field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_acos::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_acos::get_command_string */

} //namespace

cmzn_field *cmzn_fieldmodule_create_field_acos(
		struct cmzn_fieldmodule *field_module,
	cmzn_field *source_field)
{
	cmzn_field_id field = 0;
	if (source_field && source_field->isNumerical())
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field->number_of_components,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_acos());
	}
	return (field);
}

int Computed_field_get_type_acos(cmzn_field *field,
	cmzn_field **source_field)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_ACOS, the
<source_field> used by it are returned.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_get_type_acos);
	if (field&&(dynamic_cast<Computed_field_acos*>(field->core)))
	{
		*source_field = field->source_fields[0];
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_acos.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_acos */

namespace {

const char computed_field_atan_type_string[] = "atan";

class Computed_field_atan : public Computed_field_core
{
public:
	Computed_field_atan() : Computed_field_core()
	{
	};

private:
	Computed_field_core *copy()
	{
		return new Computed_field_atan();
	}

	const char *get_type_string()
	{
		return(computed_field_atan_type_string);
	}

	int compare(Computed_field_core* other_field)
	{
		if (dynamic_cast<Computed_field_atan*>(other_field))
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

int Computed_field_atan::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	const RealFieldValueCache *sourceCache = RealFieldValueCache::cast(getSourceField(0)->evaluate(cache));
	if (sourceCache)
	{
		RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
		for (int i = 0; i < field->number_of_components; i++)
			valueCache.values[i] = atan(sourceCache->values[i]);
		return 1;
	}
	return 0;
}

int Computed_field_atan::evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
{
	if (fieldDerivative.getTotalOrder() > 1)
		return this->evaluateDerivativeFiniteDifference(cache, inValueCache, fieldDerivative);
	const RealFieldValueCache *sourceCache = RealFieldValueCache::cast(getSourceField(0)->evaluateDerivativeTree(cache, fieldDerivative));
	if (sourceCache)
	{
		const FE_value *sourceDerivatives = sourceCache->getDerivativeValueCache(fieldDerivative)->values;
		DerivativeValueCache *derivativeCache = inValueCache.getDerivativeValueCache(fieldDerivative);
		FE_value *derivatives = derivativeCache->values;
		const int componentCount = this->field->number_of_components;
		const int termCount = derivativeCache->getTermCount();
		for (int i = 0; i < componentCount; ++i)
		{
			// d(atan u)/dx = 1.0/(1.0 + u^2) * du/dx
			const FE_value u = sourceCache->values[i];
			const FE_value one__1_u2 = 1.0/(1.0 + u*u);
			for (int j = 0; j < termCount; ++j)
				derivatives[j] = one__1_u2 * sourceDerivatives[j];
			derivatives += termCount;
			sourceDerivatives += termCount;
		}
		return 1;
	}
	return 0;
}

int Computed_field_atan::list()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_atan);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    source field : %s\n",field->source_fields[0]->name);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_atan.  Invalid field");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_atan */

char *Computed_field_atan::get_command_string()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name;
	int error;

	ENTER(Computed_field_atan::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_atan_type_string, &error);
		append_string(&command_string, " field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_atan::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_atan::get_command_string */

} //namespace

cmzn_field *cmzn_fieldmodule_create_field_atan(
		struct cmzn_fieldmodule *field_module,
	cmzn_field *source_field)
{
	cmzn_field_id field = 0;
	if (source_field && source_field->isNumerical())
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field->number_of_components,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_atan());
	}
	return (field);
}

int Computed_field_get_type_atan(cmzn_field *field,
	cmzn_field **source_field)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_ATAN, the
<source_field> used by it are returned.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_get_type_atan);
	if (field&&(dynamic_cast<Computed_field_atan*>(field->core)))
	{
		*source_field = field->source_fields[0];
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_atan.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_atan */

namespace {

const char computed_field_atan2_type_string[] = "atan2";

class Computed_field_atan2 : public Computed_field_core
{
public:
	Computed_field_atan2() : Computed_field_core()
	{
	};

private:
	Computed_field_core *copy()
	{
		return new Computed_field_atan2();
	}

	const char *get_type_string()
	{
		return(computed_field_atan2_type_string);
	}

	int compare(Computed_field_core* other_field)
	{
		if (dynamic_cast<Computed_field_atan2*>(other_field))
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

int Computed_field_atan2::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	const RealFieldValueCache *source1Cache = RealFieldValueCache::cast(getSourceField(0)->evaluate(cache));
	const RealFieldValueCache *source2Cache = RealFieldValueCache::cast(getSourceField(1)->evaluate(cache));
	if (source1Cache && source2Cache)
	{
		RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
		for (int i = 0 ; i < field->number_of_components ; i++)
			valueCache.values[i] = atan2(source1Cache->values[i], source2Cache->values[i]);
		return 1;
	}
	return 0;
}

int Computed_field_atan2::evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
{
	if (fieldDerivative.getTotalOrder() > 1)
		return this->evaluateDerivativeFiniteDifference(cache, inValueCache, fieldDerivative);
	const RealFieldValueCache *source1Cache = RealFieldValueCache::cast(getSourceField(0)->evaluateDerivativeTree(cache, fieldDerivative));
	const RealFieldValueCache *source2Cache = RealFieldValueCache::cast(getSourceField(1)->evaluateDerivativeTree(cache, fieldDerivative));
	if (source1Cache && source2Cache)
	{
		const FE_value *source1Derivatives = source1Cache->getDerivativeValueCache(fieldDerivative)->values;
		const FE_value *source2Derivatives = source2Cache->getDerivativeValueCache(fieldDerivative)->values;
		DerivativeValueCache *derivativeCache = inValueCache.getDerivativeValueCache(fieldDerivative);
		FE_value *derivatives = derivativeCache->values;
		const int componentCount = this->field->number_of_components;
		const int termCount = derivativeCache->getTermCount();
		for (int i = 0; i < componentCount; ++i)
		{
			// d(atan (u/v))/dx =  ( v * du/dx - u * dv/dx ) / ( u^2 + v^2 )
			const FE_value u = source1Cache->values[i];
			const FE_value v = source2Cache->values[i];
			const FE_value u2_v2 = u*u + v*v;
			const FE_value u__u2_v2 = u / u2_v2;
			const FE_value v__u2_v2 = v / u2_v2;
			for (int j = 0; j < termCount; ++j)
				derivatives[j] = v__u2_v2 * source1Derivatives[j] - u__u2_v2 * source2Derivatives[j];
			derivatives += termCount;
			source1Derivatives += termCount;
			source2Derivatives += termCount;
		}
		return 1;
	}
	return 0;
}

int Computed_field_atan2::list()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_atan2);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    source fields : %s %s\n",field->source_fields[0]->name,
			field->source_fields[1]->name);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_atan2.  Invalid field");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_atan2 */

char *Computed_field_atan2::get_command_string()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name;
	int error;

	ENTER(Computed_field_atan2::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_atan2_type_string, &error);
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
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_atan2::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_atan2::get_command_string */

} //namespace

cmzn_field *cmzn_fieldmodule_create_field_atan2(
	struct cmzn_fieldmodule *field_module,
	cmzn_field *source_field_one,
	cmzn_field *source_field_two)
{
	cmzn_field *field = NULL;
	if (source_field_one && source_field_one->isNumerical() &&
		source_field_two && source_field_two->isNumerical() &&
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
			new Computed_field_atan2());
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_atan2.  Invalid argument(s)");
	}

	return (field);
}

int Computed_field_get_type_atan2(cmzn_field *field,
	cmzn_field **source_field_one,
	cmzn_field **source_field_two)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_ATAN2, the
<source_field_one> and <source_field_two> used by it are returned.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_get_type_atan2);
	if (field&&(dynamic_cast<Computed_field_atan2*>(field->core)))
	{
		*source_field_one = field->source_fields[0];
		*source_field_two = field->source_fields[1];
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_atan2.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_atan2 */

