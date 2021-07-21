/*******************************************************************************
FILE : computed_field_coordinate.c

LAST MODIFIED : 24 August 2006

DESCRIPTION :
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stdio.h>
#include <math.h>
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_private.hpp"
#include "computed_field/computed_field_coordinate.h"
#include "computed_field/computed_field_finite_element.h"
#include "computed_field/computed_field_set.h"
#include "computed_field/computed_field_wrappers.h"
#include "general/debug.h"
#include "general/mystring.h"
#include "general/message.h"

namespace {

char computed_field_coordinate_transformation_type_string[] = "coordinate_transformation";

class Computed_field_coordinate_transformation : public Computed_field_core
{
public:
	Computed_field_coordinate_transformation() : Computed_field_core()
	{
	};

private:
	Computed_field_core *copy()
	{
		return new Computed_field_coordinate_transformation();
	}

	const char *get_type_string()
	{
		return(computed_field_coordinate_transformation_type_string);
	}

	virtual enum cmzn_field_type get_type()
	{
		return CMZN_FIELD_TYPE_COORDINATE_TRANSFORMATION;
	}

	int compare(Computed_field_core* other_field)
	{
		if (dynamic_cast<Computed_field_coordinate_transformation*>(other_field))
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

	virtual enum FieldAssignmentResult assign(cmzn_fieldcache& cache, RealFieldValueCache& valueCache);

	virtual int propagate_find_element_xi(cmzn_fieldcache& field_cache,
		const FE_value *values, int number_of_values,
		struct FE_element **element_address, FE_value *xi,
		cmzn_mesh_id mesh);
};

int Computed_field_coordinate_transformation::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	cmzn_field_id sourceField = getSourceField(0);
	const RealFieldValueCache *sourceCache = RealFieldValueCache::cast(sourceField->evaluate(cache));
	if (sourceCache)
	{
		RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
		if (convert_Coordinate_system(&(sourceField->coordinate_system),
			sourceField->number_of_components, sourceCache->values,
			&(field->coordinate_system), field->number_of_components, valueCache.values,
			/*jacobian*/nullptr))
		{
			return 1;
		}
	}
	return 0;
}

int Computed_field_coordinate_transformation::evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
{
	if (fieldDerivative.getTotalOrder() > 1)
		return this->evaluateDerivativeFiniteDifference(cache, inValueCache, fieldDerivative);
	cmzn_field_id sourceField = getSourceField(0);
	const RealFieldValueCache *sourceCache = RealFieldValueCache::cast(sourceField->evaluateDerivativeTree(cache, fieldDerivative));
	if (sourceCache)
	{
		RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
		FE_value dx_dX[9];
		const int sourceComponentCount = sourceField->number_of_components;
		if (convert_Coordinate_system(&(sourceField->coordinate_system),
			sourceComponentCount, sourceCache->values,
			&(field->coordinate_system), field->number_of_components, valueCache.values,
			dx_dX))
		{
			const DerivativeValueCache *sourceDerivativeCache = sourceCache->getDerivativeValueCache(fieldDerivative);
			DerivativeValueCache *derivativeCache = inValueCache.getDerivativeValueCache(fieldDerivative);
			derivativeCache->zeroValues();
			FE_value *derivatives = derivativeCache->values;
			const int componentCount = this->field->number_of_components;
			const int termCount = derivativeCache->getTermCount();
			for (int i = 0; i < componentCount; ++i)
			{
				const FE_value *sourceDerivatives = sourceDerivativeCache->values;
				for (int k = 0; k < sourceComponentCount; ++k)
				{
					const FE_value dx_dXik = dx_dX[i*3 + k];
					for (int j = 0; j < termCount; ++j)
						derivatives[j] += dx_dXik * sourceDerivatives[j];
					sourceDerivatives += termCount;
				}
				derivatives += termCount;
			}
			return 1;
		}
	}
	return 0;
}

enum FieldAssignmentResult Computed_field_coordinate_transformation::assign(cmzn_fieldcache& cache, RealFieldValueCache& valueCache)
{
	cmzn_field_id sourceField = getSourceField(0);
	RealFieldValueCache *sourceCache = RealFieldValueCache::cast(sourceField->getValueCache(cache));
	if (convert_Coordinate_system(&(field->coordinate_system), /*number_of_components*/3, valueCache.values,
		&(sourceField->coordinate_system), sourceField->number_of_components, sourceCache->values,
		/*jacobian*/(FE_value *)NULL))
	{
		return getSourceField(0)->assign(cache, *sourceCache);
	}
	return FIELD_ASSIGNMENT_RESULT_FAIL;
}

int Computed_field_coordinate_transformation::propagate_find_element_xi(cmzn_fieldcache& field_cache,
	const FE_value *values, int number_of_values, struct FE_element **element_address,
	FE_value *xi, cmzn_mesh_id mesh)
{
	int return_code;

	ENTER(Computed_field_coordinate_transformation::propagate_find_element_xi);
	if (field && values && (field->number_of_components == number_of_values))
	{
		FE_value source_field_coordinates[3];

		/* convert this fields values back into source coordinate system */
		return_code=convert_Coordinate_system(&(field->coordinate_system),
			number_of_values,values, &(field->source_fields[0]->coordinate_system),
			field->source_fields[0]->number_of_components, source_field_coordinates,
			/*jacobian*/(FE_value *)NULL) && Computed_field_find_element_xi(
			field->source_fields[0], &field_cache, source_field_coordinates,
			field->source_fields[0]->number_of_components, element_address,
			xi, mesh, /*propagate_field*/1,
			/*find_nearest_location*/0);
		if (!return_code)
		{
			display_message(ERROR_MESSAGE,
				"Computed_field_coordinate_transformation::propagate_find_element_xi.  "
				"Could not set coordinate_transformation field %s at node",field->name);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_coordinate_transformation::propagate_find_element_xi.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_coordinate_transformation::propagate_find_element_xi */

int Computed_field_coordinate_transformation::list(
	)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_coordinate_transformation);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    source field : %s\n",field->source_fields[0]->name);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_coordinate_transformation.  Invalid arguments.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_coordinate_transformation */

char *Computed_field_coordinate_transformation::get_command_string(
	)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name;
	int error;

	ENTER(Computed_field_coordinate_transformation::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_coordinate_transformation_type_string, &error);
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
			"Computed_field_coordinate_transformation::get_command_string.  "
			"Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_coordinate_transformation::get_command_string */

} //namespace

struct Computed_field *cmzn_fieldmodule_create_field_coordinate_transformation(
	struct cmzn_fieldmodule *field_module,
	struct Computed_field *source_field)
{
	cmzn_field_id field = 0;
	if (source_field && source_field->isNumerical())
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			/*number_of_components*/3,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_coordinate_transformation());
	}
	return (field);
}

int Computed_field_get_type_coordinate_transformation(struct Computed_field *field,
	struct Computed_field **source_field)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_COORDINATE_TRANSFORMATION, the
<source_field> used by it is returned.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_get_type_coordinate_transformation);
	if (field && (dynamic_cast<Computed_field_coordinate_transformation*>(field->core)) && source_field)
	{
		*source_field = field->source_fields[0];
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_coordinate_transformation.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_coordinate_transformation */

namespace {

char computed_field_vector_coordinate_transformation_type_string[] = "vector_coordinate_transformation";

class Computed_field_vector_coordinate_transformation : public Computed_field_core
{
public:
	Computed_field_vector_coordinate_transformation() : Computed_field_core()
	{
	};

private:
	Computed_field_core *copy()
	{
		return new Computed_field_vector_coordinate_transformation();
	}

	const char *get_type_string()
	{
		return(computed_field_vector_coordinate_transformation_type_string);
	}

	virtual enum cmzn_field_type get_type()
	{
		return CMZN_FIELD_TYPE_VECTOR_COORDINATE_TRANSFORMATION;
	}

	int compare(Computed_field_core* other_field)
	{
		if (dynamic_cast<Computed_field_vector_coordinate_transformation*>(other_field))
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
		return this->evaluateDerivativeFiniteDifference(cache, inValueCache, fieldDerivative);
	}

	int list();

	char* get_command_string();

	virtual enum FieldAssignmentResult assign(cmzn_fieldcache& cache, RealFieldValueCache& valueCache);
};

/***************************************************************************//**
 * Transforms 1, 2 or 3 vectors at a current coordinate position to a different
 * coordinate system.
 * The function uses the jacobian between the old coordinate system and the new
 * one at the coordinate position to get the new vector. Hence, derivatives of
 * the converted vectors are not available.
 */
int Computed_field_vector_coordinate_transformation::evaluate(
	cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	cmzn_field_id vectorField = getSourceField(0);
	cmzn_field_id coordinateField = getSourceField(1);
	const RealFieldValueCache *vectorValueCache = RealFieldValueCache::cast(vectorField->evaluate(cache));
	const RealFieldValueCache *coordinateValueCache = RealFieldValueCache::cast(coordinateField->evaluate(cache));
	if (vectorValueCache && coordinateValueCache)
	{
		RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);

		FE_value cx[3],jacobian[9],*source,sum,x[3];
		int coordinates_per_vector;
		if (convert_Coordinate_system(&(coordinateField->coordinate_system),
			coordinateField->number_of_components, coordinateValueCache->values,
			&(vectorField->coordinate_system), 3, cx, /*jacobian*/(FE_value *)NULL) &&
			convert_Coordinate_system(&(vectorField->coordinate_system),
				3, cx, &(field->coordinate_system), 3, x, jacobian))
		{
			int number_of_vectors = field->number_of_components/3;
			coordinates_per_vector = vectorField->number_of_components/number_of_vectors;
			source = vectorValueCache->values;
			for (int i=0;i<number_of_vectors;i++)
			{
				for (int j=0;j<3;j++)
				{
					sum=0.0;
					for (int k=0;k<coordinates_per_vector;k++)
					{
						sum += jacobian[j*3+k]*source[i*coordinates_per_vector+k];
					}
					valueCache.values[i*3+j]=sum;
				}
			}
			return 1;
		}
	}
	return 0;
}

enum FieldAssignmentResult Computed_field_vector_coordinate_transformation::assign(cmzn_fieldcache& cache, RealFieldValueCache& valueCache)
{
	cmzn_field_id vectorField = getSourceField(0);
	cmzn_field_id coordinateField = getSourceField(1);
	const RealFieldValueCache *coordinateValueCache = RealFieldValueCache::cast(coordinateField->evaluate(cache));
	if (coordinateValueCache)
	{
		FE_value cx[3];
		if (convert_Coordinate_system(&(coordinateField->coordinate_system),
			coordinateField->number_of_components, coordinateValueCache->values,
			&(vectorField->coordinate_system), 3, cx, /*jacobian*/(FE_value *)NULL))
		{
			FE_value new_cx[3], jacobian[9];
			/* need jacobian at current coordinate position for
				converting to coordinate system of source vector field
				(=source_fields[0]) */
			if (convert_Coordinate_system(&(field->coordinate_system), 3, cx,
				&(vectorField->coordinate_system), 3, new_cx, jacobian))
			{
				RealFieldValueCache *vectorValueCache = RealFieldValueCache::cast(vectorField->getValueCache(cache));
				const int number_of_vectors = field->number_of_components/3;
				const int coordinates_per_vector = vectorField->number_of_components/number_of_vectors;
				for (int i=0;i<number_of_vectors;i++)
				{
					for (int m=0;m<coordinates_per_vector;m++)
					{
						FE_value sum=0.0;
						for (int k=0;k<3;k++)
						{
							sum += jacobian[m*3+k]*valueCache.values[(i*3+k)];
						}
						vectorValueCache->values[i*coordinates_per_vector+m]=sum;
					}
				}
				return vectorField->assign(cache, *vectorValueCache);
			}
		}
	}
	return FIELD_ASSIGNMENT_RESULT_FAIL;
}

int Computed_field_vector_coordinate_transformation::list()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_vector_coordinate_transformation);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    vector field : %s\n",field->source_fields[0]->name);
		display_message(INFORMATION_MESSAGE,
			"    coordinate field : %s\n",field->source_fields[1]->name);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_vector_coordinate_transformation.  Invalid arguments.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_vector_coordinate_transformation */

char *Computed_field_vector_coordinate_transformation::get_command_string()
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name;
	int error;

	ENTER(Computed_field_vector_coordinate_transformation::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_vector_coordinate_transformation_type_string, &error);
		append_string(&command_string, " vector ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
		append_string(&command_string, " coordinate ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[1], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_vector_coordinate_transformation::get_command_string.  "
			"Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_vector_coordinate_transformation::get_command_string */

} //namespace

struct Computed_field *cmzn_fieldmodule_create_field_vector_coordinate_transformation(
	struct cmzn_fieldmodule *field_module,
	struct Computed_field *vector_field, struct Computed_field *coordinate_field)
{
	Computed_field *field = NULL;
	if (field_module && vector_field && coordinate_field&&
		Computed_field_is_orientation_scale_capable(vector_field, (void *)NULL) &&
		Computed_field_has_up_to_3_numerical_components(coordinate_field,
			(void *)NULL))
	{
		int number_of_components = 0;
		if (3 >= vector_field->number_of_components)
		{
			number_of_components = 3;
		}
		else if (6 >= vector_field->number_of_components)
		{
			number_of_components = 6;
		}
		else
		{
			number_of_components = 9;
		}
		Computed_field *source_fields[2];
		source_fields[0] = vector_field;
		source_fields[1] = coordinate_field;

		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			number_of_components,
			/*number_of_source_fields*/2, source_fields,
			/*number_of_source_values*/0, NULL,
			new Computed_field_vector_coordinate_transformation());
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_vector_coordinate_transformation.  Invalid argument(s)");
	}

	return (field);
}

int Computed_field_get_type_vector_coordinate_transformation(struct Computed_field *field,
	struct Computed_field **vector_field,
	struct Computed_field **coordinate_field)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_VECTOR_COORDINATE_TRANSFORMATION, the
<vector_field> and <coordinate_field> used by it is returned.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_get_type_vector_coordinate_transformation);
	if (field &&
		(dynamic_cast<Computed_field_vector_coordinate_transformation*>(field->core)) &&
		vector_field && coordinate_field)
	{
		*vector_field = field->source_fields[0];
		*coordinate_field = field->source_fields[1];
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_vector_coordinate_transformation.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_vector_coordinate_transformation */

