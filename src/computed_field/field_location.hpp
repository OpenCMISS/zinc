/**
 * FILE : field_location.hpp
 *
 * Location at which to evaluate a field.
 * These are low level transient objects used internally to evaluate fields.
 * May contain object pointers, but does not access them for thread safety.
 * Client using them must monitor for changes and clear as needed.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (__FIELD_LOCATION_HPP__)
#define __FIELD_LOCATION_HPP__

#include "opencmiss/zinc/types/elementid.h"
#include "opencmiss/zinc/types/fieldid.h"
#include "opencmiss/zinc/types/nodeid.h"
#include "finite_element/finite_element_mesh.hpp"
#include "finite_element/finite_element_basis.hpp"
#include "general/value.h"

class Field_location_element_xi;
class Field_location_field_values;
class Field_location_node;
class Field_location_time;

class Field_location
{
public:
	// enumeration of all derived types 
	enum Type
	{
		TYPE_INVALID = 0,
		TYPE_ELEMENT_XI = 1,
		TYPE_FIELD_VALUES = 2,
		TYPE_NODE = 3,
		TYPE_TIME = 4
	};

private:
	Field_location();  // not implemented for abstract base class

protected:
	Type type;  // store for efficient type switching
	FE_value time;

	Field_location(Type type_in) :
		type(type_in),
		time(0.0)
	{}

public:

	// abstract virtual destructor declaration
	virtual ~Field_location() = 0;

	FE_value get_time() const
	{
		return this->time;
	}

	void set_time(FE_value time_in)
	{
		this->time = time_in;
	}

	Type get_type() const
	{
		return this->type;
	}

	/** @return  Pointer to element xi location, or nullptr if not this type of location */
	inline const Field_location_element_xi *cast_element_xi() const;

	/** @return  Pointer to field values location, or nullptr if not this type of location */
	inline const Field_location_field_values *cast_field_values() const;

	/** @return  Pointer to node location, or nullptr if not this type of location */
	inline const Field_location_node *cast_node() const;

	/** @return Pointer to time location, or nullptr if not this type of location */
	inline const Field_location_time *cast_time() const;

};

class Field_location_element_xi : public Field_location
{
private:
	cmzn_element *element;  // not accessed
	int element_dimension;
	FE_value xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	cmzn_element *top_level_element;  // not accessed
	mutable Standard_basis_function_evaluation basis_function_evaluation;

public:

	Field_location_element_xi() :
		Field_location(TYPE_ELEMENT_XI),
		element(0),
		element_dimension(0),
		top_level_element(0)
	{
		for (int i = 0; i < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++i)
			xi[i] = 0.0;
	}

	/** Set element and optional top level element to inherit fields from, at default xi.
	 * @param element  Element pointer. Client must ensure valid.
	 * @param top_level_element  Field element pointer, or 0 for default. */
	void set_element(cmzn_element *element_in, cmzn_element *top_level_element_in = 0)
	{
		this->element = element_in;
		this->element_dimension = element_in->getDimension();
		this->top_level_element = top_level_element_in;
		for (int i = 0; i < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++i)
			xi[i] = 0.0;
		this->basis_function_evaluation.invalidate();
	}

	/** Set element xi location with optional top level element to inherit fields from.
	 * @param element_in  Element pointer. Client must ensure exists while pointer held.
	 * @param xi_in  Array of xi coordinates. Client must check valid!
	 * @param top_level_element_in  Field element pointer, or 0 for default. Client must ensure exists while pointer held */
	void set_element_xi(cmzn_element *element_in, const FE_value *xi_in, cmzn_element *top_level_element_in = 0)
	{
		this->element = element_in;
		const int element_dimension_in = element_in->getDimension();
		bool same_xi = (this->element_dimension == element_dimension_in);
		this->element_dimension = element_dimension_in;
		for (int i = 0; i < element_dimension_in; ++i)
		{
			if (same_xi && (this->xi[i] != xi_in[i]))
				same_xi = false;
			xi[i] = xi_in[i];
		}
		this->top_level_element = top_level_element_in;
		if (!same_xi)
			this->basis_function_evaluation.invalidate();
	}

	int get_element_dimension() const
	{
		return this->element_dimension;
	}

	cmzn_element *get_element() const
	{
		return element;
	}

	const FE_value *get_xi() const
	{
		return xi;
	}

	cmzn_element *get_top_level_element() const
	{
		return top_level_element;
	}

	/** This is only for evaluating basis functions at this xi location. */
	Standard_basis_function_evaluation &get_basis_function_evaluation() const
	{
		return this->basis_function_evaluation;
	}

};

const Field_location_element_xi *Field_location::cast_element_xi() const
{
	if (this->get_type() == TYPE_ELEMENT_XI)
		return static_cast<const Field_location_element_xi *>(this);
	return nullptr;
}

/** A location represented by values of a single field */
class Field_location_field_values : public Field_location
{
private:
	cmzn_field *field;  // not accessed
	FE_value *values;
	int number_of_values;
	int number_of_values_allocated;

public:
	Field_location_field_values() :
		Field_location(TYPE_FIELD_VALUES),
		field(0),
		values(0),
		number_of_values(0),
		number_of_values_allocated(0)
	{
	}

	virtual ~Field_location_field_values()
	{
		delete[] this->values;
	}

	/** Set field and values to evaluate at.
	 * @param field_in  Client must ensure exists while pointer held.
	 * @param number_of_values  Size of values_in, equal to number of field components. Client must ensure correct.
	 * @param values_in  Array of real coordinates. Client must ensure valid. */
	void set_field_values(cmzn_field *field_in,
		int number_of_values_in, const FE_value* values_in);

	cmzn_field *get_field() const
	{
		return this->field;
	}

	int get_number_of_values() const
	{
		return number_of_values;
	}

	const FE_value *get_values() const
	{
		return values;
	}

};

const Field_location_field_values *Field_location::cast_field_values() const
{
	if (this->get_type() == TYPE_FIELD_VALUES)
		return static_cast<const Field_location_field_values *>(this);
	return nullptr;
}

class Field_location_node : public Field_location
{
private:
	cmzn_node *node;  // not accessed
	cmzn_element *host_element;  // optional; not accessed

public:
	Field_location_node() :
		Field_location(TYPE_NODE),
		node(nullptr),
		host_element(nullptr)
	{
	}

	cmzn_node *get_node() const
	{
		return node;
	}

	/** Set node location.
	 * @param node_in  Node pointer. Client must ensure exists while pointer held. */
	void set_node(cmzn_node *node_in)
	{
		this->node = node_in;
		this->host_element = nullptr;
	}

	/** Set node with host element the node is embedded in.
	 * @param node_in  Node pointer. Client must ensure exists while pointer held.
	 * @param element_in  Element pointer. Client must ensure exists while pointer held */
	void set_node_with_host_element(cmzn_node *node_in, cmzn_element *element_in)
	{
		this->node = node_in;
		this->host_element = element_in;
	}

	cmzn_element *get_host_element() const
	{
		return this->host_element;
	}

};

const Field_location_node *Field_location::cast_node() const
{
	if (this->get_type() == TYPE_NODE)
		return static_cast<const Field_location_node *>(this);
	return nullptr;
}

class Field_location_time : public Field_location
{
public:
	Field_location_time():
		Field_location(TYPE_TIME)
	{
	}
};

const Field_location_time *Field_location::cast_time() const
{
	if (this->get_type() == TYPE_TIME)
		return static_cast<const Field_location_time *>(this);
	return nullptr;
}

#endif /* !defined (__FIELD_LOCATION_HPP__) */
