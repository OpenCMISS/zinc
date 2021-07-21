/**
 * @file fieldparameters.hpp
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CMZN_FIELDPARAMETERS_HPP__
#define CMZN_FIELDPARAMETERS_HPP__

#include "opencmiss/zinc/differentialoperator.hpp"
#include "opencmiss/zinc/element.hpp"
#include "opencmiss/zinc/fieldparameters.h"
#include "opencmiss/zinc/field.hpp"

namespace OpenCMISS
{
namespace Zinc
{

class Fieldparameters
{
protected:
	cmzn_fieldparameters_id id;

public:

	Fieldparameters() :
		id(0)
	{
	}

	// takes ownership of C handle, responsibility for destroying it
	explicit Fieldparameters(cmzn_fieldparameters_id fieldparameters_id) :
		id(fieldparameters_id)
	{
	}

	Fieldparameters(const Fieldparameters& fieldparameters) :
		id(cmzn_fieldparameters_access(fieldparameters.id))
	{
	}

	Fieldparameters& operator=(const Fieldparameters& fieldparameters)
	{
		cmzn_fieldparameters_id temp_id = cmzn_fieldparameters_access(fieldparameters.id);
		if (0 != this->id)
		{
			cmzn_fieldparameters_destroy(&this->id);
		}
		this->id = temp_id;
		return *this;
	}

	~Fieldparameters()
	{
		if (0 != this->id)
		{
			cmzn_fieldparameters_destroy(&this->id);
		}
	}

	bool isValid() const
	{
		return (0 != this->id);
	}

	cmzn_fieldparameters_id getId() const
	{
		return id;
	}

	Differentialoperator getDerivativeOperator(int order)
	{
		return Differentialoperator(cmzn_fieldparameters_get_derivative_operator(this->id, order));
	}

	int getElementParameterIndexes(const Element& element, int valuesCount, int *valuesOut)
	{
		return cmzn_fieldparameters_get_element_parameter_indexes(this->id, element.getId(), valuesCount, valuesOut);
	}

	int getElementParameterIndexesZero(const Element& element, int valuesCount, int *valuesOut)
	{
		return cmzn_fieldparameters_get_element_parameter_indexes_zero(this->id, element.getId(), valuesCount, valuesOut);
	}

	Field getField() const
	{
		return Field(cmzn_fieldparameters_get_field(this->id));
	}

	int getNumberOfElementParameters(const Element& element)
	{
		return cmzn_fieldparameters_get_number_of_element_parameters(this->id, element.getId());
	}

	int getNumberOfParameters()
	{
		return cmzn_fieldparameters_get_number_of_parameters(this->id);
	}

	int addParameters(int valuesCount, const double *valuesIn)
	{
		return cmzn_fieldparameters_add_parameters(this->id, valuesCount, valuesIn);
	}

	int getParameters(int valuesCount, double *valuesOut)
	{
		return cmzn_fieldparameters_get_parameters(this->id, valuesCount, valuesOut);
	}

	int setParameters(int valuesCount, const double *valuesIn)
	{
		return cmzn_fieldparameters_set_parameters(this->id, valuesCount, valuesIn);
	}
};

inline Fieldparameters Field::getFieldparameters()
{
	return Fieldparameters(cmzn_field_get_fieldparameters(this->getId()));
}

}  // namespace Zinc
}

#endif /* CMZN_FIELDPARAMETERS_HPP__ */
