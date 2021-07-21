/**
 * @file fieldnodesetoperators.hpp
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef CMZN_FIELDNODESETOPERATORS_HPP__
#define CMZN_FIELDNODESETOPERATORS_HPP__

#include "opencmiss/zinc/fieldnodesetoperators.h"
#include "opencmiss/zinc/field.hpp"
#include "opencmiss/zinc/fieldmodule.hpp"
#include "opencmiss/zinc/node.hpp"

namespace OpenCMISS
{
namespace Zinc
{

class FieldNodesetOperator : public Field
{
	inline cmzn_field_nodeset_operator_id getDerivedId() const
	{
		return reinterpret_cast<cmzn_field_nodeset_operator_id>(id);
	}

public:

	FieldNodesetOperator() :
		Field()
	{
	}

	// takes ownership of C handle, responsibility for destroying it
	explicit FieldNodesetOperator(cmzn_field_nodeset_operator_id field_nodeset_operator_id) :
		Field(reinterpret_cast<cmzn_field_id>(field_nodeset_operator_id))
	{
	}

	Field getElementMapField() const
	{
		return Field(cmzn_field_nodeset_operator_get_element_map_field(this->getDerivedId()));
	}
		
	int setElementMapField(const Field& elementMapField)
	{
		return cmzn_field_nodeset_operator_set_element_map_field(
			this->getDerivedId(), elementMapField.getId());
	}

};

class FieldNodesetSum : public FieldNodesetOperator
{
private:
	// takes ownership of C handle, responsibility for destroying it
	explicit FieldNodesetSum(cmzn_field_nodeset_operator_id field_nodeset_operator_id) :
		FieldNodesetOperator(field_nodeset_operator_id)
	{	}

	friend FieldNodesetSum Fieldmodule::createFieldNodesetSum(const Field& sourceField, const Nodeset& nodeset);

public:

	FieldNodesetSum() : FieldNodesetOperator()
	{	}

};

class FieldNodesetMean : public FieldNodesetOperator
{
private:
	// takes ownership of C handle, responsibility for destroying it
	explicit FieldNodesetMean(cmzn_field_nodeset_operator_id field_nodeset_operator_id) :
		FieldNodesetOperator(field_nodeset_operator_id)
	{	}

	friend FieldNodesetMean Fieldmodule::createFieldNodesetMean(const Field& sourceField,
		const Nodeset& nodeset);

public:

	FieldNodesetMean() : FieldNodesetOperator()
	{	}

};

class FieldNodesetSumSquares : public FieldNodesetOperator
{
private:
	// takes ownership of C handle, responsibility for destroying it
	explicit FieldNodesetSumSquares(cmzn_field_nodeset_operator_id field_nodeset_operator_id) :
		FieldNodesetOperator(field_nodeset_operator_id)
	{	}

	friend FieldNodesetSumSquares Fieldmodule::createFieldNodesetSumSquares(
		const Field& sourceField, const Nodeset& nodeset);

public:

	FieldNodesetSumSquares() : FieldNodesetOperator()
	{	}

};

class FieldNodesetMeanSquares : public FieldNodesetOperator
{
private:
	// takes ownership of C handle, responsibility for destroying it
	explicit FieldNodesetMeanSquares(cmzn_field_nodeset_operator_id field_nodeset_operator_id) :
		FieldNodesetOperator(field_nodeset_operator_id)
	{	}

	friend FieldNodesetMeanSquares Fieldmodule::createFieldNodesetMeanSquares(
		const Field& sourceField, const Nodeset& nodeset);

public:

	FieldNodesetMeanSquares() : FieldNodesetOperator()
	{	}

};

class FieldNodesetMinimum : public FieldNodesetOperator
{
private:
	// takes ownership of C handle, responsibility for destroying it
	explicit FieldNodesetMinimum(cmzn_field_nodeset_operator_id field_nodeset_operator_id) :
		FieldNodesetOperator(field_nodeset_operator_id)
	{	}

	friend FieldNodesetMinimum Fieldmodule::createFieldNodesetMinimum(
		const Field& sourceField, const Nodeset& nodeset);

public:

	FieldNodesetMinimum() : FieldNodesetOperator()
	{	}

};

class FieldNodesetMaximum : public FieldNodesetOperator
{
private:
	// takes ownership of C handle, responsibility for destroying it
	explicit FieldNodesetMaximum(cmzn_field_nodeset_operator_id field_nodeset_operator_id) :
		FieldNodesetOperator(field_nodeset_operator_id)
	{	}

	friend FieldNodesetMaximum Fieldmodule::createFieldNodesetMaximum(
		const Field& sourceField, const Nodeset& nodeset);

public:

	FieldNodesetMaximum() : FieldNodesetOperator()
	{	}

};

inline FieldNodesetOperator Field::castNodesetOperator()
{
	return FieldNodesetOperator(cmzn_field_cast_nodeset_operator(this->id));
}

inline FieldNodesetSum Fieldmodule::createFieldNodesetSum(const Field& sourceField, const Nodeset& nodeset)
{
	return FieldNodesetSum(reinterpret_cast<cmzn_field_nodeset_operator_id>(
		cmzn_fieldmodule_create_field_nodeset_sum(this->id, sourceField.getId(), nodeset.getId())));
}

inline FieldNodesetMean Fieldmodule::createFieldNodesetMean(const Field& sourceField, const Nodeset& nodeset)
{
	return FieldNodesetMean(reinterpret_cast<cmzn_field_nodeset_operator_id>(
		cmzn_fieldmodule_create_field_nodeset_mean(this->id, sourceField.getId(), nodeset.getId())));
}

inline FieldNodesetSumSquares Fieldmodule::createFieldNodesetSumSquares(
	const Field& sourceField, const Nodeset& nodeset)
{
	return FieldNodesetSumSquares(reinterpret_cast<cmzn_field_nodeset_operator_id>(
		cmzn_fieldmodule_create_field_nodeset_sum_squares(this->id, sourceField.getId(), nodeset.getId())));
}

inline FieldNodesetMeanSquares Fieldmodule::createFieldNodesetMeanSquares(
	const Field& sourceField, const Nodeset& nodeset)
{
	return FieldNodesetMeanSquares(reinterpret_cast<cmzn_field_nodeset_operator_id>(
		cmzn_fieldmodule_create_field_nodeset_mean_squares(this->id, sourceField.getId(), nodeset.getId())));
}

inline FieldNodesetMinimum Fieldmodule::createFieldNodesetMinimum(
	const Field& sourceField, const Nodeset& nodeset)
{
	return FieldNodesetMinimum(reinterpret_cast<cmzn_field_nodeset_operator_id>(
		cmzn_fieldmodule_create_field_nodeset_minimum(this->id, sourceField.getId(), nodeset.getId())));
}

inline FieldNodesetMaximum Fieldmodule::createFieldNodesetMaximum(
	const Field& sourceField, const Nodeset& nodeset)
{
	return FieldNodesetMaximum(reinterpret_cast<cmzn_field_nodeset_operator_id>(
		cmzn_fieldmodule_create_field_nodeset_maximum(this->id, sourceField.getId(), nodeset.getId())));
}

}  // namespace Zinc
}

#endif
