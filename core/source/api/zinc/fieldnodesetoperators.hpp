/***************************************************************************//**
 * FILE : fieldnodesetoperators.hpp
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef CMZN_FIELDNODESETOPERATORS_HPP__
#define CMZN_FIELDNODESETOPERATORS_HPP__

#include "zinc/fieldnodesetoperators.h"
#include "zinc/field.hpp"
#include "zinc/node.hpp"

namespace OpenCMISS
{
namespace Zinc
{

class FieldNodesetSum : public Field
{
private:
	// takes ownership of C handle, responsibility for destroying it
	explicit FieldNodesetSum(cmzn_field_id field_id) : Field(field_id)
	{	}

	friend FieldNodesetSum FieldModule::createNodesetSum(Field& sourceField, Nodeset& nodeset);

public:

	FieldNodesetSum() : Field(0)
	{	}

};

class FieldNodesetMean : public Field
{
private:
	// takes ownership of C handle, responsibility for destroying it
	explicit FieldNodesetMean(cmzn_field_id field_id) : Field(field_id)
	{	}

	friend FieldNodesetMean FieldModule::createNodesetMean(Field& sourceField,
		Nodeset& nodeset);

public:

	FieldNodesetMean() : Field(0)
	{	}

};

class FieldNodesetSumSquares : public Field
{
private:
	// takes ownership of C handle, responsibility for destroying it
	explicit FieldNodesetSumSquares(cmzn_field_id field_id) : Field(field_id)
	{	}

	friend FieldNodesetSumSquares FieldModule::createNodesetSumSquares(
		Field& sourceField, Nodeset& nodeset);

public:

	FieldNodesetSumSquares() : Field(0)
	{	}

};

class FieldNodesetMeanSquares : public Field
{
private:
	// takes ownership of C handle, responsibility for destroying it
	explicit FieldNodesetMeanSquares(cmzn_field_id field_id) : Field(field_id)
	{	}

	friend FieldNodesetMeanSquares FieldModule::createNodesetMeanSquares(
		Field& sourceField, Nodeset& nodeset);

public:

	FieldNodesetMeanSquares() : Field(0)
	{	}

};

class FieldNodesetMinimum : public Field
{
private:
	// takes ownership of C handle, responsibility for destroying it
	explicit FieldNodesetMinimum(cmzn_field_id field_id) : Field(field_id)
	{	}

	friend FieldNodesetMinimum FieldModule::createNodesetMinimum(
		Field& sourceField, Nodeset& nodeset);

public:

	FieldNodesetMinimum() : Field(0)
	{	}

};

class FieldNodesetMaximum : public Field
{
private:
	// takes ownership of C handle, responsibility for destroying it
	explicit FieldNodesetMaximum(cmzn_field_id field_id) : Field(field_id)
	{	}

	friend FieldNodesetMaximum FieldModule::createNodesetMaximum(
		Field& sourceField, Nodeset& nodeset);

public:

	FieldNodesetMaximum() : Field(0)
	{	}

};

inline FieldNodesetSum FieldModule::createNodesetSum(Field& sourceField, Nodeset& nodeset)
{
	return FieldNodesetSum(cmzn_field_module_create_nodeset_sum(id,
		sourceField.getId(), nodeset.getId()));
}

inline FieldNodesetMean FieldModule::createNodesetMean(Field& sourceField, Nodeset& nodeset)
{
	return FieldNodesetMean(cmzn_field_module_create_nodeset_mean(id,
		sourceField.getId(), nodeset.getId()));
}

inline FieldNodesetSumSquares FieldModule::createNodesetSumSquares(
	Field& sourceField, Nodeset& nodeset)
{
	return FieldNodesetSumSquares(cmzn_field_module_create_nodeset_sum_squares(id,
		sourceField.getId(), nodeset.getId()));
}

inline FieldNodesetMeanSquares FieldModule::createNodesetMeanSquares(
	Field& sourceField, Nodeset& nodeset)
{
	return FieldNodesetMeanSquares(cmzn_field_module_create_nodeset_mean_squares(id,
		sourceField.getId(), nodeset.getId()));
}

inline FieldNodesetMinimum FieldModule::createNodesetMinimum(
	Field& sourceField, Nodeset& nodeset)
{
	return FieldNodesetMinimum(cmzn_field_module_create_nodeset_minimum(id,
		sourceField.getId(), nodeset.getId()));
}

inline FieldNodesetMaximum FieldModule::createNodesetMaximum(
	Field& sourceField, Nodeset& nodeset)
{
	return FieldNodesetMaximum(cmzn_field_module_create_nodeset_maximum(id,
		sourceField.getId(), nodeset.getId()));
}

}  // namespace Zinc
}

#endif
