/**
 * FILE : finite_element.cpp
 *
 * Finite element data structures and functions.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <vector>

#include "opencmiss/zinc/element.h"
#include "opencmiss/zinc/node.h"
#include "opencmiss/zinc/status.h"
#include "general/indexed_list_stl_private.hpp"
#include <math.h>
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_field_evaluation.hpp"
#include "finite_element/finite_element_field_private.hpp"
#include "finite_element/finite_element_mesh.hpp"
#include "finite_element/finite_element_nodeset.hpp"
#include "finite_element/finite_element_private.h"
#include "finite_element/finite_element_region_private.h"
#include "finite_element/finite_element_value_storage.hpp"
#include "general/compare.h"
#include "general/debug.h"
#include "general/enumerator_private.hpp"
#include "general/enumerator_conversion.hpp"
#include "general/indexed_list_private.h"
#include "general/list_private.h"
#include "general/multi_range.h"
#include "general/mystring.h"
#include "general/object.h"
#include "general/value.h"
#include "general/message.h"

/*
Module types
------------
*/

FULL_DECLARE_INDEXED_LIST_TYPE(FE_node_field);

FULL_DECLARE_LIST_TYPE(FE_node_field_info);

/**
 * @see struct FE_element_type_node_sequence.
 */
struct FE_element_type_node_sequence_identifier
{
	/* node_numbers must be ordered from lowest to highest to work! */
	int *node_numbers,number_of_nodes;
};

struct FE_element_type_node_sequence
/*******************************************************************************
LAST MODIFIED : 30 April 1999

DESCRIPTION :
Structure for storing an element with its identifier being its cm_type and the
number and list - in ascending order - of the nodes referred to by the default
coordinate field of the element. Indexed lists, indexed using function
compare_FE_element_type_node_sequence_identifier ensure that recalling a line or
face with the same nodes is extremely rapid. FE_mesh
uses them to find faces and lines for elements without them, if they exist.

???RC Can only match faces correctly for coordinate fields with standard node
to element maps and no versions. A grid-based coordinate field would fail
utterly since it has no nodes. A possible future solution for all cases is to
match the geometry exactly either by using the FE_element_field_evaluation
(coefficients of the monomial basis functions), although there is a problem with
xi-directions not matching up, or actual centre positions of the face being a
trivial rejection, narrowing down to a single face or list of faces to compare
against.
==============================================================================*/
{
	struct FE_element_type_node_sequence_identifier identifier;
	struct FE_element *element;
	int dimension; // can be different from element dimension if created with face number
	int access_count;
}; /* struct FE_element_line_face_node_sequence */

FULL_DECLARE_INDEXED_LIST_TYPE(FE_element_type_node_sequence);

struct FE_node_field_iterator_and_data
{
	FE_node_field_iterator_function *iterator;
	struct FE_node *node;
	void *user_data;
}; /* struct FE_node_field_iterator_and_data */

struct FE_element_field_iterator_and_data
{
	FE_element_field_iterator_function *iterator;
	struct FE_element *element;
	void *user_data;
}; /* struct FE_element_field_iterator_and_data */

struct FE_field_order_info
/*******************************************************************************
LAST MODIFIED : 4 September 2001

DESCRIPTION :
Stores a list of fields in the order they are added.
???RC move up in file.
==============================================================================*/
{
	int allocated_number_of_fields, number_of_fields;
	struct FE_field **fields;
}; /* FE_field_order_info */

/*
Module functions
----------------
*/

/**
 * Returns true if <node_field_1> and <node_field_2> are equivalent.
 * Note that the component value pointers do not have to match since the
 * position of the data in the values_storage can be different.
 * Use with caution!
 * @param compare_field_and_time_sequence  If set, compare field pointer
 * and time sequence, otherwise ignore differences.
 * @param compare_component_value_offset  If set, compare the value
 * offsets into the node values_storage, otherwise ignore differences.
 */
static bool FE_node_fields_match(struct FE_node_field *node_field_1,
	struct FE_node_field *node_field_2, bool compare_field_and_time_sequence,
	bool compare_component_value_offset)
{
	if (!((node_field_1) && (node_field_2)))
	{
		display_message(ERROR_MESSAGE, "FE_node_fields_match.  Invalid argument(s)");
		return false;
	}
	if ((compare_field_and_time_sequence)
		&& ((node_field_1->field != node_field_2->field)
			|| (node_field_1->time_sequence != node_field_2->time_sequence)))
	{
		return false;
	}
	const int componentCount = node_field_1->field->getNumberOfComponents();
	if (node_field_2->field->getNumberOfComponents() != componentCount)
	{
		return false;
	}
	for (int c = 0; c < componentCount; ++c)
	{
		if (!(node_field_1->components[c].matches(node_field_2->components[c], compare_component_value_offset)))
		{
			return false;
		}
	}
	return true;
}

int FE_node_field_is_in_list(struct FE_node_field *node_field,
	void *node_field_list_void)
{
	int return_code;
	struct FE_node_field *node_field_2;
	struct LIST(FE_node_field) *node_field_list;

	ENTER(FE_node_field_is_in_list);
	return_code = 0;
	if (node_field && (node_field->field) &&
		(node_field_list = (struct LIST(FE_node_field) *)node_field_list_void))
	{
		if ((node_field_2 = FIND_BY_IDENTIFIER_IN_LIST(FE_node_field,field)(
			node_field->field, node_field_list)) &&
			FE_node_fields_match(node_field, node_field_2,
				/*compare_field_and_time_sequence*/true, /*compare_component_value*/true))
		{
			return_code = 1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_node_field_is_in_list.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* FE_node_field_is_in_list */

static int FE_node_field_set_FE_time_sequence(struct FE_node_field *node_field,
	struct FE_time_sequence *time_sequence)
/*******************************************************************************
LAST MODIFIED : 13 December 2005

DESCRIPTION :
Sets the fe_time_sequence for this object.  If this FE_node_field is being
accessed more than once it will fail as there will be other Node_field_infos
and therefore nodes that would then have mismatched node_fields and values_storage.
Should only be doing this if the Node_field_info that this belongs to is only
being used by one node otherwise you would have to update the values_storage
for all nodes using the Node_field_info.
==============================================================================*/
{
	int return_code;

	ENTER(FE_node_field_set_FE_time_sequence);
	if (node_field && (node_field->access_count < 2))
	{
		REACCESS(FE_time_sequence)(&(node_field->time_sequence), time_sequence);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_node_field_set_FE_time_sequence.  Invalid arguments");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_node_field_set_FE_time_sequence */

static int FE_node_field_add_values_storage_size(
	struct FE_node_field *node_field, void *values_storage_size_void)
/*******************************************************************************
LAST MODIFIED : 14 September 2000

DESCRIPTION :
If <node_field> represents a GENERAL_FE_FIELD then its required values_storage
in the node, adjusted for word alignment, is added on to <*values_storage_size>.
==============================================================================*/
{
	int return_code,this_values_storage_size,*values_storage_size;
	struct FE_field *field;

	ENTER(FE_node_field_add_values_storage_size);
	if (node_field&&(field=node_field->field)&&
		(values_storage_size = (int *)values_storage_size_void))
	{
		if (GENERAL_FE_FIELD==field->get_FE_field_type())
		{
			this_values_storage_size = node_field->getTotalValuesCount()*
				get_Value_storage_size(field->getValueType(), node_field->time_sequence);
			ADJUST_VALUE_STORAGE_SIZE(this_values_storage_size);
			(*values_storage_size) += this_values_storage_size;
		}
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_node_field_add_values_storage_size.  Invalid argument");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_node_field_add_values_storage_size */

int FE_node_field_free_values_storage_arrays(
	struct FE_node_field *node_field, void *start_of_values_storage_void)
/*******************************************************************************
LAST MODIFIED: 9 March 2005

DESCRIPTION:
Frees accesses and dynamically allocated memory in <start_of_values_storage>
for the FE_node_field.
The <start_of_the_values_storage> address is passed so that the this function
can be called from an interator, the <node> is not passed so this function can
be used to deallocate parts of any values_storage.  The <values_storage> can
be NULL if there are no GENERAL_FE_FIELDs defined on the node.
Only certain value types, eg. arrays, strings, element_xi require this.
==============================================================================*/
{
	enum Value_type value_type;
	int number_of_values,return_code;
	Value_storage *start_of_values_storage, *values_storage;

	ENTER(FE_node_field_free_values_storage_arrays);
	if (node_field&&node_field->field)
	{
		return_code = 1;
		/* only general fields have node_field_components and can have
			 values_storage at the node */
		if (GENERAL_FE_FIELD==node_field->field->get_FE_field_type())
		{
			value_type = node_field->field->getValueType();
			const int componentCount = node_field->field->getNumberOfComponents();
			for (int c = 0; c < componentCount; ++c)
			{
				const FE_node_field_template *component = node_field->getComponent(c);
				if (NULL != (start_of_values_storage=(Value_storage *)start_of_values_storage_void))
				{
					values_storage = start_of_values_storage + component->getValuesOffset();
					number_of_values = component->getTotalValuesCount();
					free_value_storage_array(values_storage,value_type,
						node_field->time_sequence,number_of_values);
					component++;
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"FE_node_field_free_values_storage_arrays. Invalid values storage");
					return_code = 0;
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_node_field_free_values_storage_arrays. Invalid arguments");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_node_field_free_values_storage_arrays */

struct FE_node_field_merge_values_storage_data
{
	Value_storage *new_values_storage;
	struct LIST(FE_node_field) *old_node_field_list;
	Value_storage *old_values_storage;
	struct LIST(FE_node_field) *add_node_field_list;
	Value_storage *add_values_storage;
	int optimised_merge;
}; /* FE_node_field_merge_values_storage_data */

static int FE_node_field_merge_values_storage(
	struct FE_node_field *new_node_field,	void *copy_data_void)
/*******************************************************************************
LAST MODIFIED: 4 March 2005

DESCRIPTION:
If <new_node_field> uses values storage then:

... when <add_node_field_list> and <add_values_storage> provided:
Finds the equivalent node field in the <old_node_field_list> or
<add_node_field_list>, and copies values giving precedence to the latter.
If the node fields have times, the time arrays are allocated once, then the
old values are copied followed by the add values to correctly merge the times.

... when <add_node_field_list> and <add_values_storage> not provided:
Copies the values for <new_node_field> into <new_values_storage> from the
<old_values_storage> with the equivalent node field in <old_node_field_list>.

... when <new_values_storage> is not provided then the values described by
<add_node_field_list> are copied from the <add_values_storage> into the
corresponding places in the <old_values_storage>.

Notes:
Assumes <new_values_storage> is already allocated to the appropriate size.
Assumes the only differences between equivalent node fields are in time version;
no checks on this are made here.
Assumes component nodal values are consecutive and start at first component.
<copy_data_void> points at a struct FE_node_field_merge_values_storage_data.
==============================================================================*/
{
	enum Value_type value_type;
	int number_of_values, return_code;
	struct FE_field *field;
	struct FE_node_field *add_node_field, *old_node_field;
	struct FE_node_field_merge_values_storage_data *copy_data;
	Value_storage *destination, *source;

	ENTER(FE_node_field_merge_values_storage);
	if (new_node_field && (field = new_node_field->field) && (copy_data =
		(struct FE_node_field_merge_values_storage_data *)copy_data_void))
	{
		return_code = 1;
		/* only GENERAL_FE_FIELD has values stored with node */
		if (GENERAL_FE_FIELD == field->get_FE_field_type())
		{
			old_node_field = FIND_BY_IDENTIFIER_IN_LIST(FE_node_field,field)(
				field, copy_data->old_node_field_list);
			if (copy_data->add_node_field_list)
			{
				add_node_field = FIND_BY_IDENTIFIER_IN_LIST(FE_node_field,field)(
					field, copy_data->add_node_field_list);
			}
			else
			{
				add_node_field = (struct FE_node_field *)NULL;
			}
			if (copy_data->new_values_storage)
			{
				/* Merging into a new values storage */
				if (old_node_field || add_node_field)
				{
					/* destination in new_values_storage according to new_node_field */
					destination =
						copy_data->new_values_storage + new_node_field->components->getValuesOffset();
					value_type = field->getValueType();
					number_of_values = new_node_field->getTotalValuesCount();
					if ((!add_node_field) ||
						(old_node_field && new_node_field->time_sequence))
					{
						/* source in old_values_storage according to old_node_field */
						if (copy_data->old_values_storage && old_node_field->components)
						{
							source = copy_data->old_values_storage +
								old_node_field->components->getValuesOffset();
							return_code = copy_value_storage_array(destination, value_type,
								new_node_field->time_sequence, old_node_field->time_sequence,
								number_of_values, source, copy_data->optimised_merge);
						}
						else
						{
							return_code = 0;
						}
					}
					if (return_code && add_node_field)
					{
						/* source in add_values_storage according to add_node_field */
						if (copy_data->add_values_storage && add_node_field->components)
						{
							source = copy_data->add_values_storage +
								add_node_field->components->getValuesOffset();
							if (old_node_field && new_node_field->time_sequence)
							{
								return_code = copy_time_sequence_values_storage_arrays(
									destination, value_type, new_node_field->time_sequence,
									add_node_field->time_sequence, number_of_values, source);
							}
							else
							{
								return_code = copy_value_storage_array(destination, value_type,
									new_node_field->time_sequence, add_node_field->time_sequence,
									number_of_values, source, copy_data->optimised_merge);
							}
						}
						else
						{
							return_code = 0;
						}
					}
				}
				else
				{
					display_message(ERROR_MESSAGE, "FE_node_field_merge_values_storage.  "
						"Could not find equivalent existing node field");
					return_code = 0;
				}
				if (!return_code)
				{
					display_message(ERROR_MESSAGE,
						"FE_node_field_merge_values_storage.  Unable to copy values");
					return_code = 0;
				}
			}
			else
			{
				/* Replacing values in the existing values storage by copying
					them from the add_node_storage to the old_node_storage.
					(old_node_field and new_node_field should be the same) */
				if (old_node_field && (old_node_field == new_node_field))
				{
					if (add_node_field)
					{
						destination =
							copy_data->old_values_storage + old_node_field->components->getValuesOffset();
						value_type = field->getValueType();
						number_of_values = new_node_field->getTotalValuesCount();
						if (copy_data->add_values_storage && add_node_field->components)
						{
							source = copy_data->add_values_storage +
								add_node_field->components->getValuesOffset();
							if (old_node_field->time_sequence)
							{
								return_code = copy_time_sequence_values_storage_arrays(
									destination, value_type, old_node_field->time_sequence,
									add_node_field->time_sequence, number_of_values, source);
							}
							else
							{
								/* Release the storage of the old values.  The pointer is from the
									start of the values storage to make it work as a node_field iterator. */
								FE_node_field_free_values_storage_arrays(old_node_field,
									(void *)copy_data->old_values_storage);
								return_code = copy_value_storage_array(destination, value_type,
									old_node_field->time_sequence, add_node_field->time_sequence,
									number_of_values, source, copy_data->optimised_merge);
							}
						}
						else
						{
							display_message(ERROR_MESSAGE,
								"FE_node_field_merge_values_storage.  Unable to merge values");
							return_code = 0;
						}
					}
					/* else do nothing as we are leaving this field alone */
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"FE_node_field_merge_values_storage.  "
						"Unable to find corresponding node fields when updating values.");
					return_code = 0;
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_node_field_merge_values_storage.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_node_field_merge_values_storage */

static int merge_FE_node_values_storage(struct FE_node *node,
	Value_storage *values_storage,
	struct LIST(FE_node_field) *new_node_field_list,
	struct FE_node *add_node, int optimised_merge)
/*******************************************************************************
LAST MODIFIED: 4 March 2005

DESCRIPTION:
For each field in <new_node_field_list> requiring values storage, finds the
equivalent node field in either <node>, <add_node> or both. If only one of
<node> or <add_node> contains an equivalent node field, those values are
copied. If there is an equivalent node field in both, behaviour depends on
whether the node fields have times:
* if the node fields have no times, values are taken from <add_node>.
* if the node fields have times, the times arrays are allocated, then the
values at times in <node> are copied, followed by those in <add_node>.
Hence, the values in <add_node> take preference over those in <node>.
If <values_storage> is not provided then the fields described in the
<new_node_field_list> are looked for in the <add_node> and if found the
values are copied into the existing values storage in <node>.
Notes:
* there must be an equivalent node field at either <node> or <add_node>;
* <add_node> is optional and used only by merge_FE_node. If NULL then a node
field must be found in <node>;
* Values_storage, when provided, must already be allocated to the appropriate
size but is not assumed to contain any information prior to being filled here;
* Any objects or arrays referenced in the values_storage are accessed or
allocated in the new <values_storage> so <node> and <add_node> are unchanged.
* It is up to the calling function to have checked that the node fields in
<node>, <add_node> and <new_node_field_list> are compatible.
==============================================================================*/
{
	int return_code;
	struct FE_node_field_merge_values_storage_data copy_data;

	ENTER(merge_FE_node_values_storage);
	if (node && node->fields && new_node_field_list &&
		((!add_node) || add_node->fields))
	{
		copy_data.new_values_storage = values_storage;
		copy_data.old_node_field_list = node->fields->node_field_list;
		copy_data.old_values_storage = node->values_storage;
		if (add_node)
		{
			copy_data.add_node_field_list = add_node->fields->node_field_list;
			copy_data.add_values_storage = add_node->values_storage;
		}
		else
		{
			copy_data.add_node_field_list = (struct LIST(FE_node_field) *)NULL;
			copy_data.add_values_storage = (Value_storage *)NULL;
		}
		copy_data.optimised_merge = optimised_merge;
		return_code = FOR_EACH_OBJECT_IN_LIST(FE_node_field)(
			FE_node_field_merge_values_storage, (void *)(&copy_data),
			new_node_field_list);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"merge_FE_node_values_storage.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* merge_FE_node_values_storage */

int get_FE_node_field_list_values_storage_size(
	struct LIST(FE_node_field) *node_field_list)
{
	int values_storage_size;

	ENTER(get_FE_node_field_list_values_storage_size);
	values_storage_size=0;
	if (node_field_list)
	{
		FOR_EACH_OBJECT_IN_LIST(FE_node_field)(
			FE_node_field_add_values_storage_size,
			(void *)&values_storage_size,node_field_list);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"get_FE_node_field_list_values_storage_size.  Invalid argument");
	}
	LEAVE;

	return (values_storage_size);
} /* get_FE_node_field_list_values_storage_size */

int allocate_and_copy_FE_node_values_storage(struct FE_node *node,
	Value_storage **values_storage)
{
	int return_code,size;
	Value_storage *dest_values_storage;

	ENTER(allocate_and_copy_FE_node_values_storage);
	if (node)
	{
		return_code = 1;

		if (node->fields)
		{
			size = get_FE_node_field_list_values_storage_size(
				node->fields->node_field_list);
			if (size)
			{
				if (ALLOCATE(dest_values_storage,Value_storage,size))
				{
					return_code = merge_FE_node_values_storage(node, dest_values_storage,
						node->fields->node_field_list, (struct FE_node *)NULL,
						/*optimised_merge*/0);
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"allocate_and_copy_FE_node_values_storage.  Not enough memory");
					dest_values_storage = (Value_storage *)NULL;
					return_code = 0;
				}
			}
			else /* no fields, nothing to copy */
			{
				dest_values_storage = (Value_storage *)NULL;
			}
		}
		else /* no fields, nothing to copy */
		{
		 dest_values_storage = (Value_storage *)NULL;
		}
		*values_storage = dest_values_storage;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"allocate_and_copy_FE_node_values_storage.  Invalid arguments");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* allocate_and_copy_FE_node_values_storage */

/**
 * Sorting function for FE_node_field and FE_element_field lists.
 * Returns:
 * -1 if field_1 < field_2
 *  0 if field_1 = field_2
 *  1 if field_1 > field_2
 * Note: formerly used strcmp(field_1->getName(),field_2->getName()) to order by field
 * name alphabetically, but this made it nearly impossible to rename FE_fields.
 */
static int compare_FE_field(struct FE_field *field_1,struct FE_field *field_2)
{
	if (field_1 < field_2)
	{
		return -1;
	}
	else if (field_1 > field_2)
	{
		return 1;
	}
	return 0;
}

FE_node_field::FE_node_field(struct FE_field *fieldIn) :
	field(ACCESS(FE_field)(fieldIn)),
	components(new FE_node_field_template[fieldIn->getNumberOfComponents()]),
	time_sequence(0),
	access_count(0)
{
}

FE_node_field::~FE_node_field()
{
	DEACCESS(FE_field)(&(this->field));
	delete[] this->components;
	if (this->time_sequence)
	{
		DEACCESS(FE_time_sequence)(&(this->time_sequence));
	}
}

FE_node_field *FE_node_field::create(struct FE_field *fieldIn)
{
	if (!fieldIn)
	{
		display_message(ERROR_MESSAGE, "FE_node_field::create.  Missing field");
		return 0;
	}
	FE_node_field *node_field = new FE_node_field(fieldIn);
	if (!((node_field) && (node_field->components)))
	{
		display_message(ERROR_MESSAGE, "FE_node_field::create.  Failed.");
		delete node_field;
		return 0;
	}
	return node_field;
}

FE_node_field *FE_node_field::clone(const FE_node_field& source, int deltaValuesOffset)
{
	FE_node_field *node_field = FE_node_field::create(source.field);
	if (!node_field)
	{
		return 0;
	}
	if (source.time_sequence)
	{
		node_field->time_sequence = ACCESS(FE_time_sequence)(source.time_sequence);
	}
	if (GENERAL_FE_FIELD != source.field->get_FE_field_type())
	{
		// though component->valuesOffset is currently irrelevant for these fields,
		// keep it unchanged for future use
		deltaValuesOffset = 0;
	}
	const int componentCount = get_FE_field_number_of_components(source.field);
	for (int c = 0; c < componentCount; ++c)
	{
		node_field->components[c] = source.components[c];
		node_field->components[c].addValuesOffset(deltaValuesOffset);
	}
	return node_field;
}

int FE_node_field::getValueMaximumVersionsCount(cmzn_node_value_label valueLabel) const
{
	int maximumVersionsCount = 0;
	const int componentCount = get_FE_field_number_of_components(this->field);
	for (int c = 0; c < componentCount; ++c)
	{
		const int versionsCount = this->components[c].getValueNumberOfVersions(valueLabel);
		if (versionsCount > maximumVersionsCount)
		{
			maximumVersionsCount = versionsCount;
		}
	}
	return maximumVersionsCount;
}

int FE_node_field::getValueMinimumVersionsCount(cmzn_node_value_label valueLabel) const
{
	int minimumVersionsCount = 0;
	const int componentCount = get_FE_field_number_of_components(this->field);
	for (int c = 0; c < componentCount; ++c)
	{
		const int versionsCount = this->components[c].getValueNumberOfVersions(valueLabel);
		if (versionsCount == 0)
		{
			return 0;
		}
		if ((c == 0) || (versionsCount < minimumVersionsCount))
		{
			minimumVersionsCount = versionsCount;
		}
	}
	return minimumVersionsCount;
}

int FE_node_field::getMaximumDerivativeNumber() const
{
	int maximumDerivativeNumber = 0;
	const int componentCount = this->field->getNumberOfComponents();
	for (int c = 0; c < componentCount; ++c)
	{
		const int componentMaximumDerivativeNumber = this->components[c].getMaximumDerivativeNumber();
		if (componentMaximumDerivativeNumber > maximumDerivativeNumber)
		{
			maximumDerivativeNumber = componentMaximumDerivativeNumber;
		}
	}
	return maximumDerivativeNumber;
}

void FE_node_field::expandHighestDerivativeAndVersion(int& highestDerivative, int& highestVersion) const
{
	const int componentCount = this->field->getNumberOfComponents();
	for (int c = 0; c < componentCount; ++c)
	{
		const FE_node_field_template *nft = this->getComponent(c);
		const int valueLabelsCount = nft->getValueLabelsCount();
		for (int d = 0; d < valueLabelsCount; ++d)
		{
			const cmzn_node_value_label valueLabel = nft->getValueLabelAtIndex(d);
			const int derivative = (valueLabel - CMZN_NODE_VALUE_LABEL_VALUE) + 1;
			if (derivative > highestDerivative)
				highestDerivative = derivative;
			const int versionsCount = nft->getVersionsCountAtIndex(d);
			if (versionsCount > highestVersion)
				highestVersion = versionsCount;
		}
	}
}

PROTOTYPE_ACCESS_OBJECT_FUNCTION(FE_node_field)
{
	if (object)
	{
		return object->access();
	}
	display_message(ERROR_MESSAGE, "ACCESS(FE_node_field).  Invalid argument");
	return 0;
}

PROTOTYPE_DEACCESS_OBJECT_FUNCTION(FE_node_field)
{
	if (object_address)
	{
		FE_node_field::deaccess(*object_address);
		return 1;
	}
	return 0;
}

PROTOTYPE_REACCESS_OBJECT_FUNCTION(FE_node_field)
{
	if (object_address)
	{
		if (new_object)
		{
			new_object->access();
		}
		FE_node_field::deaccess(*object_address);
		*object_address = new_object;
		return 1;
	}
	return 0;
}

DECLARE_INDEXED_LIST_MODULE_FUNCTIONS(FE_node_field, field, \
	struct FE_field *, compare_FE_field)

DECLARE_INDEXED_LIST_FUNCTIONS(FE_node_field)

DECLARE_FIND_BY_IDENTIFIER_IN_INDEXED_LIST_FUNCTION(FE_node_field, field, \
	struct FE_field *, compare_FE_field)

struct FE_node_field_copy_with_equivalent_field_data
/*******************************************************************************
LAST MODIFIED : 27 November 2002

DESCRIPTION :
Data for passing to FE_node_field_copy_with_equivalent_field.
==============================================================================*/
{
	struct FE_time_sequence_package *fe_time;
	struct LIST(FE_field) *fe_field_list;
	struct LIST(FE_node_field) *node_field_list;
};

/**
 * Creates a copy of <node_field> using the same named field in <fe_field_list>
 * and adds it to <node_field_list>. Checks fields are equivalent.
 * Also creates an equivalent FE_time_sequence in the new <fe_time> for this
 * node field.
 * <data_void> points at a struct FE_node_field_copy_with_equivalent_field_data.
 */
static int FE_node_field_copy_with_equivalent_field(
	struct FE_node_field *node_field, void *data_void)
{
	int return_code;
	struct FE_node_field_copy_with_equivalent_field_data *data;

	if (node_field && node_field->field &&
		(data = (struct FE_node_field_copy_with_equivalent_field_data *)data_void))
	{
		return_code = 1;
		struct FE_field *equivalent_field = FIND_BY_IDENTIFIER_IN_LIST(FE_field, name)(
			node_field->field->getName(), data->fe_field_list);
		if (equivalent_field)
		{
			if (FE_fields_match_exact(node_field->field, equivalent_field))
			{
				struct FE_node_field *copy_node_field = FE_node_field::clone(*node_field);
				if (copy_node_field)
				{
					copy_node_field->setField(equivalent_field);
					if (node_field->time_sequence)
					{
						// following is not accessed:
						FE_time_sequence *copy_time_sequence = get_FE_time_sequence_matching_FE_time_sequence(data->fe_time,
							node_field->time_sequence);
						if (copy_time_sequence)
						{
							FE_node_field_set_FE_time_sequence(copy_node_field, copy_time_sequence);
						}
						else
						{
							return_code = 0;
						}
					}
					if (return_code)
					{
						if (!ADD_OBJECT_TO_LIST(FE_node_field)(copy_node_field,
							data->node_field_list))
						{
							return_code = 0;
						}
					}
					if (!return_code)
					{
						display_message(ERROR_MESSAGE,
							"FE_node_field_copy_with_equivalent_field.  Could not copy node field");
						delete copy_node_field;
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"FE_node_field_copy_with_equivalent_field.  "
						"Could not create node field");
					return_code = 0;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"FE_node_field_copy_with_equivalent_field.  Fields not equivalent");
				return_code = 0;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_node_field_copy_with_equivalent_field.  No equivalent field");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_node_field_copy_with_equivalent_field.  Invalid argument(s)");
		return_code = 0;
	}
	return (return_code);
}

struct LIST(FE_node_field) *
	FE_node_field_list_clone_with_FE_field_list(
		struct LIST(FE_node_field) *node_field_list,
		struct LIST(FE_field) *fe_field_list, struct FE_time_sequence_package *fe_time)
/*******************************************************************************
LAST MODIFIED : 6 May 2003

DESCRIPTION :
Returns a new FE_node_field list that is identical to <node_field_list>
except that it references equivalent same-name fields from <fe_field_list> and
uses FE_time_sequences in <fe_time>.
It is an error if an equivalent FE_field is not found.
==============================================================================*/
{
	struct LIST(FE_node_field) *return_node_field_list;
	struct FE_node_field_copy_with_equivalent_field_data data;

	ENTER(FE_node_field_list_clone_with_FE_field_list);
	return_node_field_list = (struct LIST(FE_node_field) *)NULL;
	if (node_field_list && fe_field_list && fe_time)
	{
		data.fe_time = fe_time;
		data.fe_field_list = fe_field_list;
		data.node_field_list = CREATE(LIST(FE_node_field))();
		if (FOR_EACH_OBJECT_IN_LIST(FE_node_field)(
			FE_node_field_copy_with_equivalent_field, (void *)&data,
			node_field_list))
		{
			return_node_field_list = data.node_field_list;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_node_field_list_clone_with_FE_field_list.  Failed");
			DESTROY(LIST(FE_node_field))(&data.node_field_list);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_node_field_list_clone_with_FE_field_list.  Invalid argument(s)");
	}
	LEAVE;

	return (return_node_field_list);
} /* FE_node_field_list_clone_with_FE_field_list */

int FE_node_field_info_add_node_field(
	struct FE_node_field_info *fe_node_field_info,
	struct FE_node_field *new_node_field, int new_number_of_values)
/*******************************************************************************
LAST MODIFIED : 24 August 2005

DESCRIPTION :
Adds the <new_node_field> to the list in the <fe_node_field_info> and updates the
<new_number_of_values>.  This should only be done if object requesting the change
is known to be the only object using this field info.
==============================================================================*/
{
	int return_code;

	ENTER(FE_node_field_info_add_node_field);
	if (fe_node_field_info)
	{
		if (ADD_OBJECT_TO_LIST(FE_node_field)(new_node_field,
				fe_node_field_info->node_field_list))
		{
			fe_node_field_info->number_of_values = new_number_of_values;
			FE_node_field_add_values_storage_size(
				new_node_field, (void *)&fe_node_field_info->values_storage_size);
			return_code = 1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_node_field_info_add_node_field.  Unable to add field to list");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_node_field_info_add_node_field.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_node_field_info_add_node_field */

/**
 * Compare functions for listing struct FE_element_type_node_sequence in order:
 * - number_of_nodes changing slowest;
 * - node numbers in numerical order (nodes must be in ascending order).
 * Returns values like strcmp:
 * -1 = identifier 1 < identifier 2
 * 0 = identifier 1 = identifier 2
 * 1 = identifier 1 > identifier 2
 */
static int compare_FE_element_type_node_sequence_identifier(
	FE_element_type_node_sequence_identifier &identifier1,
	FE_element_type_node_sequence_identifier &identifier2)
{
	/* 1. compare number_of_nodes */
	int number_of_nodes = identifier1.number_of_nodes;
	if (number_of_nodes < identifier2.number_of_nodes)
	{
		return -1;
	}
	else if (number_of_nodes > identifier2.number_of_nodes)
	{
		return 1;
	}
	else
	{
		/* 2. compare node_numbers - assumed in ascending order */
		for (int i = 0; i < number_of_nodes; ++i)
		{
			if (identifier1.node_numbers[i] < identifier2.node_numbers[i])
			{
				return -1;
			}
			else if (identifier1.node_numbers[i] > identifier2.node_numbers[i])
			{
				return 1;
			}
		}
	}
	return 0;
}

struct FE_element_type_node_sequence *CREATE(FE_element_type_node_sequence)(
	int &result, struct FE_element *element, int face_number)
{
	result = CMZN_OK;
	int i,node_number,*node_numbers,number_of_nodes,placed,j,k;
	struct FE_element_type_node_sequence *element_type_node_sequence;
	struct FE_node **nodes_in_element;

	if (element)
	{
		/* get list of nodes used by default coordinate field in element/face */
		result = calculate_FE_element_field_nodes(element, face_number,
			(struct FE_field *)NULL, &number_of_nodes, &nodes_in_element,
			/*top_level_element*/(struct FE_element *)NULL);
		if ((CMZN_OK == result) && (0 < number_of_nodes))
		{
			if (ALLOCATE(element_type_node_sequence, struct FE_element_type_node_sequence, 1) &&
				ALLOCATE(node_numbers, int, number_of_nodes))
			{
				element_type_node_sequence->identifier.number_of_nodes = number_of_nodes;
				element_type_node_sequence->identifier.node_numbers = node_numbers;
				element_type_node_sequence->element=ACCESS(FE_element)(element);
				element_type_node_sequence->dimension = element->getDimension();
				if (face_number >= 0)
					--(element_type_node_sequence->dimension);
				element_type_node_sequence->access_count=0;
				/* put the nodes in the identifier in ascending order */
				for (i=0;element_type_node_sequence&&(i<number_of_nodes);i++)
				{
					node_number=(nodes_in_element[i])->getIdentifier();
					node_numbers[i]=node_number;
					/* SAB Reenabled the matching of differently ordered faces as
						detecting the continuity correctly is more important than the
						problems with lines matching to different nodes when
						inheriting from different parents.
						OLDCOMMENT
						We do not want to sort these node numbers as the order of the nodes
						determines which ordering the faces are in and if these do not match
						we will get the lines matched to different pairs of the nodes. */
					placed=0;
					for (j=0;(!placed)&&(j<i);j++)
					{
						if (node_number<node_numbers[j])
						{
							/* make space for the new number */
							for (k=i;j<k;k--)
							{
								node_numbers[k]=node_numbers[k-1];
							}
							node_numbers[j]=node_number;
							placed=1;
						}
					}
					if (!placed)
					{
						node_numbers[i]=node_number;
					}
				}
#if defined (DEBUG_CODE)
				/*???debug*/
				printf("FE_element_type_node_sequence  %d-D element %d has nodes: ",
					element->getDimension(), element->getIdentifier());
				for (i=0;i<number_of_nodes;i++)
				{
					printf(" %d",node_numbers[i]);
				}
				printf("\n");
#endif /* defined (DEBUG_CODE) */
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"CREATE(FE_element_type_node_sequence).  Not enough memory");
				DEALLOCATE(element_type_node_sequence);
			}
			for (i=0;i<number_of_nodes;i++)
			{
				cmzn_node::deaccess(nodes_in_element[i]);
			}
			DEALLOCATE(nodes_in_element);
		}
		else
		{
			if (CMZN_ERROR_NOT_FOUND != result)
			{
				display_message(ERROR_MESSAGE, "CREATE(FE_element_type_node_sequence).  Failed to get nodes in element");
			}
			element_type_node_sequence=(struct FE_element_type_node_sequence *)NULL;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CREATE(FE_element_type_node_sequence).  Invalid argument(s)");
		element_type_node_sequence=(struct FE_element_type_node_sequence *)NULL;
		result = CMZN_ERROR_ARGUMENT;
	}
	return (element_type_node_sequence);
}

int DESTROY(FE_element_type_node_sequence)(
	struct FE_element_type_node_sequence **element_type_node_sequence_address)
/*******************************************************************************
LAST MODIFIED : 13 February 2003

DESCRIPTION :
Cleans up memory used by the FE_element_type_node_sequence.
==============================================================================*/
{
	int return_code;
	struct FE_element_type_node_sequence *element_type_node_sequence;

	ENTER(DESTROY(FE_element_type_node_sequence));
	if (element_type_node_sequence_address&&
		(element_type_node_sequence= *element_type_node_sequence_address))
	{
		if (0==element_type_node_sequence->access_count)
		{
			/* must deaccess element */
			DEACCESS(FE_element)(&(element_type_node_sequence->element));
			DEALLOCATE(element_type_node_sequence->identifier.node_numbers);
			DEALLOCATE(*element_type_node_sequence_address);
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"DESTROY(FE_element_type_node_sequence).  Non-zero access count of %d",
				element_type_node_sequence->access_count);
			*element_type_node_sequence_address=
				(struct FE_element_type_node_sequence *)NULL;
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(FE_element_type_node_sequence).  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(FE_element_type_node_sequence) */

DECLARE_OBJECT_FUNCTIONS(FE_element_type_node_sequence)

DECLARE_INDEXED_LIST_MODULE_FUNCTIONS(FE_element_type_node_sequence, \
	identifier, FE_element_type_node_sequence_identifier&, \
	compare_FE_element_type_node_sequence_identifier)

DECLARE_INDEXED_LIST_FUNCTIONS(FE_element_type_node_sequence)

DECLARE_FIND_BY_IDENTIFIER_IN_INDEXED_LIST_FUNCTION( \
	FE_element_type_node_sequence,identifier, \
	FE_element_type_node_sequence_identifier&, \
	compare_FE_element_type_node_sequence_identifier)

int FE_element_type_node_sequence_is_collapsed(
	struct FE_element_type_node_sequence *element_type_node_sequence)
/*******************************************************************************
LAST MODIFIED : 13 February 2003

DESCRIPTION :
Returns true if the <element_type_node_sequence> represents an element that
has collapsed, ie, is a face with <= 2 unique nodes, or a line with 1 unique
node.
???RC Note that repeated nodes in the face/line are not put in the node_numbers
array twice, facilitating the simple logic in this function. If they were, then
this function would have to be modified extensively.
==============================================================================*/
{
	int return_code;

	ENTER(FE_element_type_node_sequence_is_collapsed);
	if (element_type_node_sequence)
	{
		return_code=((2 == element_type_node_sequence->dimension)&&
			(2 >= element_type_node_sequence->identifier.number_of_nodes))||
			((1 == element_type_node_sequence->dimension)&&
				(1 == element_type_node_sequence->identifier.number_of_nodes));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_element_type_node_sequence_is_collapsed.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* FE_element_type_node_sequence_is_collapsed */

struct FE_element *FE_element_type_node_sequence_get_FE_element(
	struct FE_element_type_node_sequence *element_type_node_sequence)
{
	if (element_type_node_sequence)
		return element_type_node_sequence->element;
	return 0;
}

void FE_element_type_node_sequence_set_FE_element(
	struct FE_element_type_node_sequence *element_type_node_sequence,
	struct FE_element *element)
{
	if (element_type_node_sequence)
		REACCESS(FE_element)(&element_type_node_sequence->element, element);
}

FE_element_type_node_sequence *FE_element_type_node_sequence_list_find_match(
	struct LIST(FE_element_type_node_sequence) *element_type_node_sequence_list,
	FE_element_type_node_sequence *element_type_node_sequence)
{
	if (element_type_node_sequence_list && element_type_node_sequence)
	{
		return FIND_BY_IDENTIFIER_IN_LIST(FE_element_type_node_sequence, identifier)(
			element_type_node_sequence->identifier, element_type_node_sequence_list);
	}
	return 0;
}

/** Increases the values count by the number of values for node field */
static int count_nodal_values(struct FE_node_field *node_field,
	void *values_count_void)
{
	if (!((node_field) && (values_count_void)))
	{
		display_message(ERROR_MESSAGE, "count_nodal_values.  Invalid argument(s)");
		return 0;
	}
	*(static_cast<int *>(values_count_void)) += node_field->getTotalValuesCount();
	return 1;
}

/** Increases the values size by the number of values for node field
  * times the size of the node field value type. */
static int count_nodal_size(struct FE_node_field *node_field,
	void *values_size_void)
{
	if (!((node_field) && (values_size_void)))
	{
		display_message(ERROR_MESSAGE, "count_nodal_size.  Invalid argument(s)");
		return 0;
	}
	const int valueTypeSize = get_Value_storage_size(node_field->field->getValueType(),
		node_field->time_sequence);
	*(static_cast<int *>(values_size_void)) += node_field->getTotalValuesCount()*valueTypeSize;
	return 1;
}

struct Merge_FE_node_field_into_list_data
{
	int requires_merged_storage;
	int values_storage_size;
	int number_of_values;
	struct LIST(FE_node_field) *list;
}; /* struct Merge_FE_node_field_into_list_data */

/**
 * Merge the node field into the list.
 * If there is already a node_field for the field in <node_field>, checks the two
 * are compatible and adds any new times that may be in the new field.
 * If <node_field> introduces a new field to the list, it is added to the list
 * with value offset to the <value_size> which it subsequently increases to fit
 * the new data added for this node field. <number_of_values> is also increased
 * appropriately.
 */
static int merge_FE_node_field_into_list(struct FE_node_field *node_field,
	void *merge_FE_node_field_into_list_data)
{
	auto merge_data = static_cast<Merge_FE_node_field_into_list_data *>(merge_FE_node_field_into_list_data);
	if (!((node_field) && (merge_data)))
	{
		display_message(ERROR_MESSAGE, "merge_FE_node_field_into_list.  Invalid argument(s)");
		return 0;
	}
	int return_code = 1;
	FE_field *field = node_field->field;
	const int componentCount = field->getNumberOfComponents();
	/* check if the node field is in the list */
	FE_node_field *existing_node_field = FIND_BY_IDENTIFIER_IN_LIST(FE_node_field, field)(field, merge_data->list);
	if (existing_node_field)
	{
		// check that the components match
		for (int c = 0; c < componentCount; ++c)
		{
			if (!existing_node_field->getComponent(c)->matches(*node_field->getComponent(c)))
			{
				display_message(ERROR_MESSAGE, "merge_FE_node_field_into_list.  Node field does not match");
				return_code = 0;
				break;
			}
		}
		if (return_code)
		{
			/* if the new node_field is a time based node field merge it in */
			if (node_field->time_sequence)
			{
				if (existing_node_field->time_sequence)
				{
					// Merge time sequences in new node field
					FE_time_sequence *merged_time_sequence = FE_region_get_FE_time_sequence_merging_two_time_series(
						field->get_FE_region(), node_field->time_sequence, existing_node_field->time_sequence);
					if (merged_time_sequence)
					{
						if (compare_FE_time_sequence(merged_time_sequence, existing_node_field->time_sequence))
						{
							/* Time sequences are different */
							merge_data->requires_merged_storage = 1;
							FE_node_field *new_node_field = FE_node_field::clone(*existing_node_field);
							REACCESS(FE_time_sequence)(&(new_node_field->time_sequence),
								merged_time_sequence);
							if (!(REMOVE_OBJECT_FROM_LIST(FE_node_field)(existing_node_field, merge_data->list)
								&& ADD_OBJECT_TO_LIST(FE_node_field)(new_node_field, merge_data->list)))
							{
								display_message(ERROR_MESSAGE, "merge_FE_node_field_into_list.  "
									"Unable to replace node_field in merged list.");
								return_code = 0;
							}
						}
						else
						{
							/* Can continue with the existing time_sequence */
						}
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"merge_FE_node_field_into_list.  Unable to merge time arrays.");
						return_code = 0;
					}
				}
				else
				{
					// Not implemented. Values storage size will change.
					display_message(ERROR_MESSAGE, "merge_FE_node_field_into_list.  "
						"Overwriting non time-varying field with time-varying field is not implemented");
					return_code = 0;
				}
			}
			else
			{
				if (existing_node_field->time_sequence)
				{
					// Not implemented. Values storage size will change.
					display_message(ERROR_MESSAGE, "merge_FE_node_field_into_list.  "
						"Overwriting time-varying field with non time-varying field is not implemented");
					return_code = 0;
				}
			}
		}
	}
	else
	{
		/* the node field is not already in the list */
		/* create a new node field with modified offsets */
		merge_data->requires_merged_storage = 1;
		FE_node_field *new_node_field = FE_node_field::create(field);
		if (new_node_field)
		{
			FE_node_field_set_FE_time_sequence(new_node_field, node_field->time_sequence);
			const int valueTypeSize = get_Value_storage_size(node_field->field->getValueType(), node_field->time_sequence);
			int new_values_storage_size = 0;
			for (int c = 0; c < componentCount; ++c)
			{
				const FE_node_field_template &sourceComponent = *(node_field->getComponent(c));
				new_node_field->components[c] = sourceComponent;
				new_node_field->components[c].setValuesOffset(merge_data->values_storage_size + new_values_storage_size);
				new_values_storage_size += sourceComponent.getTotalValuesCount()*valueTypeSize;
				merge_data->number_of_values += sourceComponent.getTotalValuesCount();
			}
			if (return_code)
			{
				ADJUST_VALUE_STORAGE_SIZE(new_values_storage_size);
				merge_data->values_storage_size += new_values_storage_size;
				if (!ADD_OBJECT_TO_LIST(FE_node_field)(new_node_field, merge_data->list))
				{
					delete new_node_field;
					new_node_field = 0;
					return_code = 0;
				}
			}
			else
			{
				delete new_node_field;
				new_node_field = 0;
			}
		}
		else
		{
			return_code = 0;
		}
	}
	return return_code;
}

#if !defined (WINDOWS_DEV_FLAG)
/***************************************************************************//**
 * Outputs details of how field is defined at node.
 */
static int list_FE_node_field(struct FE_node *node, struct FE_field *field,
	void *dummy_void)
{
	USE_PARAMETER(dummy_void);
	int return_code = 1;
	if (node && field)
	{
		const FE_node_field *node_field = node->getNodeField(field);
		if (node_field)
		{
			const int valueTypeSize = get_Value_storage_size(node_field->field->getValueType(), node_field->time_sequence);
			const int componentCount = field->getNumberOfComponents();
			display_message(INFORMATION_MESSAGE,"  %s",field->getName());
			const char *type_string;
			if (NULL != (type_string=ENUMERATOR_STRING(CM_field_type)(field->get_CM_field_type())))
			{
				display_message(INFORMATION_MESSAGE,", %s",type_string);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"list_FE_node_field.  Invalid CM field type");
				return_code=0;
			}
			if (NULL != (type_string=ENUMERATOR_STRING(Coordinate_system_type)(
				field->getCoordinateSystem().type)))
			{
				display_message(INFORMATION_MESSAGE,", %s",type_string);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"list_FE_node_field.  Invalid field coordinate system");
				return_code=0;
			}
			display_message(INFORMATION_MESSAGE, ", #Components=%d\n", componentCount);
			const int number_of_times = (node_field->time_sequence) ?
				FE_time_sequence_get_number_of_times(node_field->time_sequence) : 1;
			FE_value time;
			for (int time_index = 0; return_code && (time_index < number_of_times); ++time_index)
			{
				if (node_field->time_sequence)
				{
					FE_time_sequence_get_time_for_index(node_field->time_sequence,
						time_index, &time);
					display_message(INFORMATION_MESSAGE,"   Time: %g\n", time);
				}
				for (int c = 0; (c < componentCount) && return_code; ++c)
				{
					const FE_node_field_template &component = *(node_field->getComponent(c));
					char *componentName = get_FE_field_component_name(field, c);
					if (componentName)
					{
						display_message(INFORMATION_MESSAGE,"    %s.  ", componentName);
						DEALLOCATE(componentName);
					}
					if (field->getNumberOfValues())
					{
						/* display field based information */
						const int valuePerComponent = field->getNumberOfValues() / field->getNumberOfComponents();
						display_message(INFORMATION_MESSAGE,"field based values: ");
						switch (field->getValueType())
						{
							case FE_VALUE_VALUE:
							{
								display_message(INFORMATION_MESSAGE, "\n    ");
								const FE_value *values = field->getRealValues() + c*valuePerComponent;
								/* output in columns if FE_VALUE_MAX_OUTPUT_COLUMNS > 0 */
								for (int v = 0; v < valuePerComponent; ++v)
								{
									display_message(INFORMATION_MESSAGE, " %" FE_VALUE_STRING, values[v]);
									if ((v == (valuePerComponent - 1)) ||
										((0 < FE_VALUE_MAX_OUTPUT_COLUMNS) &&
										(0 == ((v + 1) % FE_VALUE_MAX_OUTPUT_COLUMNS))))
									{
										if (v < (valuePerComponent - 1))
										{
											display_message(INFORMATION_MESSAGE, "\n    ");
										}
										else
										{
											display_message(INFORMATION_MESSAGE, "\n");
										}
									}
								}
							} break;
							case INT_VALUE:
							{
								display_message(INFORMATION_MESSAGE, "\n    ");
								const int *values = field->getIntValues() + c*valuePerComponent;
								/* output in columns if FE_VALUE_MAX_OUTPUT_COLUMNS > 0 */
								for (int v = 0; v < valuePerComponent; ++v)
								{
									display_message(INFORMATION_MESSAGE, " %d", values[v]);
									if ((v == (valuePerComponent - 1)) ||
										((0 < FE_VALUE_MAX_OUTPUT_COLUMNS) &&
										(0 == ((v + 1) % FE_VALUE_MAX_OUTPUT_COLUMNS))))
									{
										if (v < (valuePerComponent - 1))
										{
											display_message(INFORMATION_MESSAGE, "\n    ");
										}
										else
										{
											display_message(INFORMATION_MESSAGE, "\n");
										}
									}
								}
							} break;
							case STRING_VALUE:
							{
								const char **fieldValues = field->getStringValues();
								display_message(INFORMATION_MESSAGE, "\n");
								display_message(INFORMATION_MESSAGE, "    ");
								// should only be one component, so this should work
								for (int v = 0; v < field->getNumberOfValues(); ++v)
								{
									if ((fieldValues) && (fieldValues[v]))
									{
										char *s = duplicate_string(fieldValues[v]);
										make_valid_token(&s);
										display_message(INFORMATION_MESSAGE, " %s", s);
										DEALLOCATE(s);
									}
									else
									{
										display_message(INFORMATION_MESSAGE, " none");
									}
								}
								display_message(INFORMATION_MESSAGE, "\n");
							} break;
							default:
							{
								display_message(INFORMATION_MESSAGE, "list_FE_node_field.  "
									"Can't display that field value_type yet.  Write the code!");
							} break;
						}
					}
					else if (node->values_storage)
					{
						/* display node based information*/
						Value_storage *values_storage = node->values_storage + component.getValuesOffset();
						const int valueLabelsCount = component.getValueLabelsCount();
						for (int d = 0; d < valueLabelsCount; ++d)
						{
							cmzn_node_value_label valueLabel = component.getValueLabelAtIndex(d);
							const int versionsCount = component.getVersionsCountAtIndex(d);
							for (int v = 0; v < versionsCount; ++v)
							{
								display_message(INFORMATION_MESSAGE, "%s", ENUMERATOR_STRING(cmzn_node_value_label)(valueLabel));
								if (versionsCount > 1)
								{
									display_message(INFORMATION_MESSAGE, "(%d)", v + 1);
								}
								display_message(INFORMATION_MESSAGE, "=");
								if (node_field->time_sequence)
								{
									switch (field->getValueType())
									{
									case FE_VALUE_VALUE:
									{
										display_message(INFORMATION_MESSAGE, "%g",
											*(*((FE_value**)values_storage) + time_index));
									} break;
									case INT_VALUE:
									{
										display_message(INFORMATION_MESSAGE, "%d",
											*(*((int **)values_storage) + time_index));
									} break;
									default:
									{
										display_message(INFORMATION_MESSAGE, "list_FE_node_field: "
											"Can't display times for this value type");
									} break;
									}/* switch*/
								}
								else /* (node_field->time_sequence) */
								{
									switch (field->getValueType())
									{
									case FE_VALUE_VALUE:
									{
										display_message(INFORMATION_MESSAGE, "%g",
											*((FE_value*)values_storage));
									} break;
									case ELEMENT_XI_VALUE:
									{
										cmzn_element *embedding_element = *((struct FE_element **)values_storage);
										if (embedding_element)
										{
											int xi_dimension = embedding_element->getDimension();
											display_message(INFORMATION_MESSAGE, "%d-D %d xi",
												xi_dimension, embedding_element->getIdentifier());
											Value_storage *value = values_storage+sizeof(struct FE_element *);
											for (int xi_index = 0; xi_index<xi_dimension; xi_index++)
											{
												display_message(INFORMATION_MESSAGE, " %g",
													*((FE_value *)value));
												value += sizeof(FE_value);
											}
										}
										else
										{
											display_message(INFORMATION_MESSAGE, "UNDEFINED");
										}
									} break;
									case INT_VALUE:
									{
										display_message(INFORMATION_MESSAGE, "%d",
											*((int *)values_storage));
									} break;
									case STRING_VALUE:
									{
										char *string_value;
										if (get_FE_nodal_string_value(node, field, c, &string_value))
										{
											display_message(INFORMATION_MESSAGE, string_value);
											DEALLOCATE(string_value);
										}
										else
										{
											display_message(ERROR_MESSAGE,
												"list_FE_node_field.  Could not get string value");
											return_code = 0;
										}
									} break;
									default:
									{
										display_message(INFORMATION_MESSAGE, "list_FE_node_field: "
											"Can't display that Value_type yet. Write the code!");
									} break;
									}/* switch*/
								} /* (node_field->time_sequence) */
								values_storage += valueTypeSize;
								if (v < (versionsCount - 1))
								{
									display_message(INFORMATION_MESSAGE, ", ");
								}
							}
							if (d < (valueLabelsCount - 1))
							{
								display_message(INFORMATION_MESSAGE, ", ");
							}
						}
						display_message(INFORMATION_MESSAGE, "\n");
					}
					else
					{
						/* missing nodal values only an error if no field based values
							either */
						if (!(field->getNumberOfValues()))
						{
							display_message(ERROR_MESSAGE,
								"list_FE_node_field.  Missing nodal values");
							return_code=0;
						}
					}
				} /* component */
			} /* time_index */
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"list_FE_node_field.  Field %s is not defined at node %d",
				field->getName(), node->getIdentifier());
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"list_FE_node_field.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* list_FE_node_field */
#endif /* !defined (WINDOWS_DEV_FLAG) */

/**
 * The standard function for mapping global parameters to get the local element
 * parameters weighting the basis in the element field template.
 * Uses relative offsets into nodal values array in standard and general node to
 * element maps. Absolute offset for start of field component is obtained from
 * the node_field_component for the field at the node.
 * Does not check arguments as called internally.
 *
 * @param field  The field to get values for.
 * @param componentNumber  The component of the field to get values for, >= 0.
 * @param eft  Element field template describing parameter mapping and basis.
 * @param element  The element to get values for.
 * @param time  The time at which to get parameter values.
 * @param nodeset  The nodeset owning any node indexes mapped.
 * @param elementValues  Pre-allocated array big enough for the number of basis
 * functions used by the EFT basis. Note this is a reference and will be
 * reallocated if basis uses a blending function. Either way, caller is
 * required to deallocate.
 * @param scaleFactors  Pointer to element scale factors.
 * @return  Number of values calculated or 0 if error.
 */
int global_to_element_map_values(FE_field *field, int componentNumber,
	const FE_element_field_template *eft, cmzn_element *element, FE_value time,
	const FE_nodeset *nodeset, const FE_value *scaleFactors, FE_value*& elementValues)
{
	if (eft->getParameterMappingMode() != CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE)
	{
		display_message(ERROR_MESSAGE, "global_to_element_map_values.  "
			"Only implemented for node parameter, first found on field %s component %d at element %d.",
			field->getName(), componentNumber + 1, element->getIdentifier());
		return 0;
	}
	if (field->getValueType() != FE_VALUE_VALUE)
	{
		display_message(ERROR_MESSAGE, "global_to_element_map_values.  Field %s is not FE_value type, not implemented", field->getName());
		return 0;
	}
	FE_mesh *mesh = element->getMesh();
	FE_mesh_element_field_template_data *meshEFTData = mesh->getElementfieldtemplateData(eft->getIndexInMesh());
	FE_basis *basis = eft->getBasis();

	const DsLabelIndex elementIndex = element->getIndex();
	const DsLabelIndex *nodeIndexes = meshEFTData->getElementNodeIndexes(elementIndex);
	if (!nodeIndexes)
	{
		display_message(ERROR_MESSAGE, "global_to_element_map_values.  "
			"Missing local-to-global node map for field %s component %d at element %d.",
			field->getName(), componentNumber + 1, element->getIdentifier());
		return 0;
	}
	const int scaleFactorCount = eft->getNumberOfLocalScaleFactors();
	if ((scaleFactorCount) && (!scaleFactors))
	{
		display_message(ERROR_MESSAGE, "global_to_element_map_values.  "
			"Element %d is missing scale factors for field %s component %d.",
			element->getIdentifier(), field->getName(), componentNumber + 1);
		return 0;
	}

	const int basisFunctionCount = eft->getNumberOfFunctions();
	int lastLocalNodeIndex = -1;
	FE_node *node = 0;
	// Cache last node_field_info since expensive to find and probably same as last node
	// If same node_field_info, then same node field template
	FE_node_field_info *node_field_info = 0;
	const FE_node_field_template *nft = 0;
	FE_time_sequence *time_sequence = 0;
	int time_index_one, time_index_two;
	FE_value time_xi;
	int tt = 0; // total term, increments up to eft->totalTermCount
	int tts = 0; // total term scaling, increments up to eft->totalLocalScaleFactorIndexes
	for (int f = 0; f < basisFunctionCount; ++f)
	{
		FE_value termSum = 0.0;
		const int termCount = eft->termCounts[f];
		for (int t = 0; t < termCount; ++t)
		{
			const int localNodeIndex = eft->localNodeIndexes[tt];
			if (localNodeIndex != lastLocalNodeIndex)
			{
				const DsLabelIndex nodeIndex = nodeIndexes[localNodeIndex];
				node = nodeset->getNode(nodeIndex);
				if (!node)
				{
					display_message(ERROR_MESSAGE, "global_to_element_map_values.  "
						"Missing node for field %s component %d in element %d, function %d term %d.",
						field->getName(), componentNumber + 1, element->getIdentifier(), f + 1, t + 1);
					return 0;
				}
				if (node_field_info != node->fields)
				{
					const FE_node_field *node_field = node->getNodeField(field);
					if (!node_field)
					{
						display_message(ERROR_MESSAGE, "global_to_element_map_values.  "
							"Cannot evaluate field %s component %d in element %d because it is not defined at node %d",
							field->getName(), componentNumber + 1, element->getIdentifier(), node->getIdentifier());
						return 0;
					}
					node_field_info = node->fields;
					nft = node_field->getComponent(componentNumber);
					if ((node_field->time_sequence != time_sequence) &&
						(time_sequence = node_field->time_sequence))
					{
						FE_time_sequence_get_interpolation_for_time(time_sequence,
							time, &time_index_one, &time_index_two, &time_xi);
					}
				}
				lastLocalNodeIndex = localNodeIndex;
			}
			const int valueIndex = nft->getValueIndex(eft->nodeValueLabels[tt], eft->nodeVersions[tt]);
			if (valueIndex < 0)
			{
				display_message(ERROR_MESSAGE, "global_to_element_map_values.  "
					"Parameter '%s' version %d not found for field %s component %d at node %d, used from element %d",
					ENUMERATOR_STRING(cmzn_node_value_label)(eft->nodeValueLabels[tt]), eft->nodeVersions[tt] + 1,
					field->getName(), componentNumber + 1, node->getIdentifier(), element->getIdentifier());
				return 0;
			}
			FE_value termValue;
			if (time_sequence)
			{
				// get address of field component parameters in node
				const FE_value *timeValues = *(reinterpret_cast<FE_value **>(node->values_storage + nft->valuesOffset) + valueIndex);
				termValue = (1.0 - time_xi)*timeValues[time_index_one] + time_xi*timeValues[time_index_two];
			}
			else
			{
				termValue = reinterpret_cast<FE_value *>(node->values_storage + nft->valuesOffset)[valueIndex];
			}
			if (scaleFactorCount)
			{
				const int termScaleFactorCount = eft->termScaleFactorCounts[tt];
				for (int s = 0; s < termScaleFactorCount; ++s)
				{
					termValue *= scaleFactors[eft->localScaleFactorIndexes[tts]];
					++tts;
				}
			}
			termSum += termValue;
			++tt;
		}
		elementValues[f] = termSum;
	}
	if (eft->getLegacyModifyThetaMode() != FE_BASIS_MODIFY_THETA_MODE_INVALID)
	{
		if (!FE_basis_modify_theta_in_xi1(eft->basis, eft->getLegacyModifyThetaMode(), elementValues))
		{
			display_message(ERROR_MESSAGE, "global_to_element_map_values.  Error modifying element values");
			return 0;
		}
	}
	const int blendedElementValuesCount = FE_basis_get_number_of_blended_functions(basis);
	if (blendedElementValuesCount > 0)
	{
		FE_value *blendedElementValues = FE_basis_get_blended_element_values(basis, elementValues);
		if (!blendedElementValues)
		{
			display_message(ERROR_MESSAGE,
				"global_to_element_map_values.  Could not allocate memory for blended values");
			return 0;
		}
		DEALLOCATE(elementValues);
		elementValues = blendedElementValues;
		return blendedElementValuesCount;
	}
	return basisFunctionCount;
}

/**
 * The standard function for calculating the nodes used for a field component.
 * Determines a vector of nodes contributing to each basis function.
 * Limitation: Only returns the first node contributing to each basis parameter
 * for general maps from multiple nodes.
 * Does not check arguments as called internally.
 *
 * @param field  The field to get values for.
 * @param componentNumber  The component of the field to get values for, >= 0.
 * @param eft  Element field template describing parameter mapping and basis.
 * @param element  The element to get values for.
 * @param nodeset  The nodeset owning any node indexes mapped.
 * @param basisNodeCount  On success, the number of node indexes returned.
 * @param basisNodeIndexes  On successful return, allocated to indexes of nodes.
 * Up to caller to deallocate.
 * @return  Result OK on success, ERROR_NOT_FOUND if no nodes found, otherwise
 * any other error code.
 */
int global_to_element_map_nodes(FE_field *field, int componentNumber,
	const FE_element_field_template *eft, cmzn_element *element,
	int &basisNodeCount, DsLabelIndex *&basisNodeIndexes)
{
	if (eft->getParameterMappingMode() != CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE)
	{
		display_message(ERROR_MESSAGE, "global_to_element_map_nodes.  "
			"Only implemented for node parameter, first found on field %s component %d at element %d.",
			field->getName(), componentNumber + 1, element->getIdentifier());
		return CMZN_ERROR_ARGUMENT;
	}
	const FE_mesh *mesh = element->getMesh();
	const FE_mesh_element_field_template_data *meshEFTData = mesh->getElementfieldtemplateData(eft->getIndexInMesh());

	const DsLabelIndex elementIndex = element->getIndex();
	const DsLabelIndex *nodeIndexes = meshEFTData->getElementNodeIndexes(elementIndex);
	if (!nodeIndexes)
	{
		//display_message(ERROR_MESSAGE, "global_to_element_map_nodes.  "
		//	"Missing local-to-global node map for field %s component %d at element %d.",
		//	field->getName(), componentNumber + 1, element->getIdentifier());
		return CMZN_ERROR_NOT_FOUND;
	}
	const int basisFunctionCount = eft->getNumberOfFunctions();
	basisNodeIndexes = new DsLabelIndex[basisFunctionCount];
	if (!basisNodeIndexes)
	{
		return CMZN_ERROR_MEMORY;
	}
	int tt = 0; // total term, increments up to eft->totalTermCount
	for (int f = 0; f < basisFunctionCount; ++f)
	{
		const int termCount = eft->termCounts[f];
		if (0 < termCount)
		{
			basisNodeIndexes[f] = nodeIndexes[eft->localNodeIndexes[tt]];
			tt += termCount;
		}
		else
		{
			basisNodeIndexes[f] = DS_LABEL_INDEX_INVALID; // no terms = 'zero' parameter
		}
	}
	basisNodeCount = basisFunctionCount;
	return CMZN_OK;
}

static int for_FE_field_at_node_iterator(struct FE_node_field *node_field,
	void *iterator_and_data_void)
/*******************************************************************************
LAST MODIFIED : 21 September 1999

DESCRIPTION :
FE_node_field iterator for for_each_FE_field_at_node.
==============================================================================*/
{
	int return_code;
	struct FE_node_field_iterator_and_data *iterator_and_data;

	ENTER(for_FE_field_at_node_iterator);
	if (node_field&&(iterator_and_data=
		(struct FE_node_field_iterator_and_data *)iterator_and_data_void)&&
		iterator_and_data->iterator)
	{
		return_code=(iterator_and_data->iterator)(iterator_and_data->node,
			node_field->field,iterator_and_data->user_data);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"for_FE_field_at_node_iterator.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* for_FE_field_at_node_iterator */

/**
 * Get pointer to the nodal values_storage and the matching time_sequence
 * for the given field, component, value label, version at node.
 * Writes no error message if field not defined or parameter does not exist,
 * hence can be used to determine if defined/exists.
 * @param version  Version number starting at 0.
 * @return  Result OK on success, ERROR_NOT_FOUND if field or parameter
 * not found, otherwise any other error.
 */
int find_FE_nodal_values_storage_dest(cmzn_node *node, FE_field *field,
	int componentNumber, cmzn_node_value_label valueLabel, int version,
	Value_storage*& valuesStorage, FE_time_sequence*& time_sequence)
{
	if (!(node && field && (0 <= componentNumber)
		&& (componentNumber < field->getNumberOfComponents()) && (0 <= version)
		&& (GENERAL_FE_FIELD == field->get_FE_field_type())))
	{
		display_message(ERROR_MESSAGE, "find_FE_nodal_values_storage_dest.  Invalid argument(s)");
		return CMZN_ERROR_ARGUMENT;
	}
	FE_node_field *node_field = node->getNodeField(field);
	if (!node_field)
	{
		return CMZN_ERROR_NOT_FOUND;
	}
	const FE_node_field_template *nft = node_field->getComponent(componentNumber);
	const int valueIndex = nft->getValueIndex(valueLabel, version);
	if (valueIndex < 0)
	{
		return CMZN_ERROR_NOT_FOUND;
	}
	if (!node->values_storage)
	{
		display_message(ERROR_MESSAGE, "find_FE_nodal_values_storage_dest.  Node has no values storage");
		return CMZN_ERROR_GENERAL;
	}
	const int valueTypeSize = get_Value_storage_size(field->getValueType(), node_field->time_sequence);
	valuesStorage = node->values_storage + nft->getValuesOffset() + valueIndex*valueTypeSize;
	time_sequence = node_field->time_sequence;
	return CMZN_OK;
}

/*
Global functions
----------------
*/

PROTOTYPE_ENUMERATOR_STRING_FUNCTION(cmzn_element_face_type)
{
	switch (enumerator_value)
	{
		case CMZN_ELEMENT_FACE_TYPE_INVALID:
			break;
		case CMZN_ELEMENT_FACE_TYPE_ALL:
			return "all";
			break;
		case CMZN_ELEMENT_FACE_TYPE_ANY_FACE:
			return "any_face";
			break;
		case CMZN_ELEMENT_FACE_TYPE_NO_FACE:
			return "no_face";
			break;
		case CMZN_ELEMENT_FACE_TYPE_XI1_0:
			return "xi1_0";
			break;
		case CMZN_ELEMENT_FACE_TYPE_XI1_1:
			return "xi1_1";
			break;
		case CMZN_ELEMENT_FACE_TYPE_XI2_0:
			return "xi2_0";
			break;
		case CMZN_ELEMENT_FACE_TYPE_XI2_1:
			return "xi2_1";
			break;
		case CMZN_ELEMENT_FACE_TYPE_XI3_0:
			return "xi3_0";
			break;
		case CMZN_ELEMENT_FACE_TYPE_XI3_1:
			return "xi3_1";
			break;
	}
	return 0;
}

class cmzn_element_face_type_conversion
{
public:
	static const char *to_string(enum cmzn_element_face_type type)
	{
		const char *enum_string = 0;
		switch (type)
		{
		case CMZN_ELEMENT_FACE_TYPE_INVALID:
			break;
		case CMZN_ELEMENT_FACE_TYPE_ALL:
			enum_string = "ALL";
			break;
		case CMZN_ELEMENT_FACE_TYPE_ANY_FACE:
			enum_string = "ANY_FACE";
			break;
		case CMZN_ELEMENT_FACE_TYPE_NO_FACE:
			enum_string = "NO_FACE";
			break;
		case CMZN_ELEMENT_FACE_TYPE_XI1_0:
			enum_string = "XI1_0";
			break;
		case CMZN_ELEMENT_FACE_TYPE_XI1_1:
			enum_string = "XI1_1";
			break;
		case CMZN_ELEMENT_FACE_TYPE_XI2_0:
			enum_string = "XI2_0";
			break;
		case CMZN_ELEMENT_FACE_TYPE_XI2_1:
			enum_string = "XI2_1";
			break;
		case CMZN_ELEMENT_FACE_TYPE_XI3_0:
			enum_string = "XI3_0";
			break;
		case CMZN_ELEMENT_FACE_TYPE_XI3_1:
			enum_string = "XI3_1";
			break;
		}
		return enum_string;
	}
};

enum cmzn_element_face_type cmzn_element_face_type_enum_from_string(
	const char *string)
{
	return string_to_enum<enum cmzn_element_face_type,
		cmzn_element_face_type_conversion>(string);
}

char *cmzn_element_face_type_enum_to_string(enum cmzn_element_face_type type)
{
	const char *string = cmzn_element_face_type_conversion::to_string(type);
	return (string ? duplicate_string(string) : 0);
}

PROTOTYPE_ENUMERATOR_STRING_FUNCTION(cmzn_element_point_sampling_mode)
{
	switch (enumerator_value)
	{
		case CMZN_ELEMENT_POINT_SAMPLING_MODE_CELL_CENTRES:
			return "cell_centres";
			break;
		case CMZN_ELEMENT_POINT_SAMPLING_MODE_CELL_CORNERS:
			return "cell_corners";
			break;
		case CMZN_ELEMENT_POINT_SAMPLING_MODE_CELL_POISSON:
			return "cell_poisson";
			break;
		case CMZN_ELEMENT_POINT_SAMPLING_MODE_SET_LOCATION:
			return "set_location";
			break;
		case CMZN_ELEMENT_POINT_SAMPLING_MODE_GAUSSIAN_QUADRATURE:
			return "gaussian_quadrature";
			break;
		case CMZN_ELEMENT_POINT_SAMPLING_MODE_INVALID:
			break;
	}
	return 0;
}

DEFINE_DEFAULT_ENUMERATOR_FUNCTIONS(cmzn_element_point_sampling_mode)

class cmzn_element_point_sampling_mode_conversion
{
public:
	static const char *to_string(enum cmzn_element_point_sampling_mode mode)
	{
		const char *enum_string = 0;
		switch (mode)
		{
		case CMZN_ELEMENT_POINT_SAMPLING_MODE_CELL_CENTRES:
			enum_string = "CELL_CENTRES";
			break;
		case CMZN_ELEMENT_POINT_SAMPLING_MODE_CELL_CORNERS:
			enum_string = "CELL_CORNERS";
			break;
		case CMZN_ELEMENT_POINT_SAMPLING_MODE_CELL_POISSON:
			enum_string = "CELL_POISSON";
			break;
		case CMZN_ELEMENT_POINT_SAMPLING_MODE_SET_LOCATION:
			enum_string = "SET_LOCATION";
			break;
		case CMZN_ELEMENT_POINT_SAMPLING_MODE_GAUSSIAN_QUADRATURE:
			enum_string = "GAUSSIAN_QUADRATURE";
			break;
		default:
			break;
		}
		return enum_string;
	}
};

enum cmzn_element_point_sampling_mode cmzn_element_point_sampling_mode_enum_from_string(
	const char *string)
{
	return string_to_enum<enum cmzn_element_point_sampling_mode,
		cmzn_element_point_sampling_mode_conversion>(string);
}

char *cmzn_element_point_sampling_mode_enum_to_string(enum cmzn_element_point_sampling_mode mode)
{
	const char *string = cmzn_element_point_sampling_mode_conversion::to_string(mode);
	return (string ? duplicate_string(string) : 0);
}

PROTOTYPE_ENUMERATOR_STRING_FUNCTION(cmzn_element_quadrature_rule)
{
	switch (enumerator_value)
	{
		case CMZN_ELEMENT_QUADRATURE_RULE_GAUSSIAN:
			return "gaussian_quadrature";
			break;
		case CMZN_ELEMENT_QUADRATURE_RULE_MIDPOINT:
			return "midpoint_quadrature";
			break;
		case CMZN_ELEMENT_QUADRATURE_RULE_INVALID:
			break;
	}
	return 0;
}

DEFINE_DEFAULT_ENUMERATOR_FUNCTIONS(cmzn_element_quadrature_rule)

/** Important: check other enumerator functions work when adding new values.
 * They assume enums are powers of 2 */
PROTOTYPE_ENUMERATOR_STRING_FUNCTION(cmzn_field_domain_type)
{
	switch (enumerator_value)
	{
	case CMZN_FIELD_DOMAIN_TYPE_POINT:
		return "domain_point";
		break;
	case CMZN_FIELD_DOMAIN_TYPE_NODES:
		return "domain_nodes";
		break;
	case CMZN_FIELD_DOMAIN_TYPE_DATAPOINTS:
		return "domain_datapoints";
		break;
	case CMZN_FIELD_DOMAIN_TYPE_MESH1D:
		return "domain_mesh1d";
		break;
	case CMZN_FIELD_DOMAIN_TYPE_MESH2D:
		return "domain_mesh2d";
		break;
	case CMZN_FIELD_DOMAIN_TYPE_MESH3D:
		return "domain_mesh3d";
		break;
	case CMZN_FIELD_DOMAIN_TYPE_MESH_HIGHEST_DIMENSION:
		return "domain_mesh_highest_dimension";
		break;
	case CMZN_FIELD_DOMAIN_TYPE_INVALID:
		break;
	}
	return 0;
}


/** Note: assumes valid enums are powers of 2, starting at 1 */
int STRING_TO_ENUMERATOR(cmzn_field_domain_type)(const char *enumerator_string,
	enum cmzn_field_domain_type *enumerator_value_address)
{
	if (enumerator_string && enumerator_value_address)
	{
		enum cmzn_field_domain_type value = static_cast<cmzn_field_domain_type>(1);
		const char *valid_string;
		while (0 != (valid_string = ENUMERATOR_STRING(cmzn_field_domain_type)(value)))
		{
			if (fuzzy_string_compare_same_length(enumerator_string, valid_string))
			{
				*enumerator_value_address = value;
				return 1;
			}
			value = static_cast<cmzn_field_domain_type>(2*value);
		}
	}
	return 0;
}


/** Note: assumes valid enums are powers of 2, starting at 1 */
const char **ENUMERATOR_GET_VALID_STRINGS(cmzn_field_domain_type)(
	int *number_of_valid_strings,
	ENUMERATOR_CONDITIONAL_FUNCTION(cmzn_field_domain_type) conditional_function,
	void *user_data)
{
	*number_of_valid_strings = 0;
	const char **valid_strings;

	ALLOCATE(valid_strings, const char *, 64); // bits in a 64-bit integer, to be safe
	enum cmzn_field_domain_type value = static_cast<cmzn_field_domain_type>(1);
	const char *valid_string;
	while (0 != (valid_string = ENUMERATOR_STRING(cmzn_field_domain_type)(value)))
	{
		if ((0 == conditional_function) || (conditional_function(value, user_data)))
		{
			valid_strings[*number_of_valid_strings] = valid_string;
			++(*number_of_valid_strings);
		}
		value = static_cast<cmzn_field_domain_type>(2*value);
	}
	return valid_strings;
}

class cmzn_field_domain_type_conversion
{
public:
	static const char *to_string(enum cmzn_field_domain_type type)
	{
		const char *enum_string = 0;
		switch (type)
		{
		case CMZN_FIELD_DOMAIN_TYPE_INVALID:
			break;
		case CMZN_FIELD_DOMAIN_TYPE_POINT:
			enum_string = "POINT";
			break;
		case CMZN_FIELD_DOMAIN_TYPE_NODES:
			enum_string = "NODES";
			break;
		case CMZN_FIELD_DOMAIN_TYPE_DATAPOINTS:
			enum_string = "DATAPOINTS";
			break;
		case CMZN_FIELD_DOMAIN_TYPE_MESH1D:
			enum_string = "MESH1D";
			break;
		case CMZN_FIELD_DOMAIN_TYPE_MESH2D:
			enum_string = "MESH2D";
			break;
		case CMZN_FIELD_DOMAIN_TYPE_MESH3D:
			enum_string = "MESH3D";
			break;
		case CMZN_FIELD_DOMAIN_TYPE_MESH_HIGHEST_DIMENSION:
			enum_string = "MESH_HIGHEST_DIMENSION";
			break;
		}
		return enum_string;
	}
};

enum cmzn_field_domain_type cmzn_field_domain_type_enum_from_string(
	const char *string)
{
	return string_to_enum_bitshift<enum cmzn_field_domain_type,
		cmzn_field_domain_type_conversion>(string);
}

char *cmzn_field_domain_type_enum_to_string(
	enum cmzn_field_domain_type type)
{
	const char *string = cmzn_field_domain_type_conversion::to_string(type);
	return (string ? duplicate_string(string) : 0);
}

/**
 * Updates the pointer to <node_field_info_address> to point to a node_field info
 * which appends to the fields in <node_field_info_address> one <new_node_field>.
 * The node_field_info returned in <node_field_info_address> will be for the
 * <new_number_of_values>.
 * The <fe_nodeset> maintains an internal list of these structures so they can be
 * shared between nodes.  This function allows a fast path when adding a single
 * field.  If the node_field passed in is only referenced by one external object
 * then it is assumed that this function can modify it rather than copying.  If
 * there are more references then the object is copied and then modified.
 * This function handles the access and deaccess of the <node_field_info_address>
 * as if it is just updating then there is nothing to do.
 */
int FE_nodeset_get_FE_node_field_info_adding_new_times(
	FE_nodeset *fe_nodeset, struct FE_node_field_info **node_field_info_address,
	struct FE_node_field *new_node_field)
{
	int return_code;
	struct FE_node_field_info *existing_node_field_info;
	struct LIST(FE_node_field) *node_field_list;

	if (fe_nodeset && node_field_info_address &&
		(existing_node_field_info = *node_field_info_address))
	{
		FE_node_field *node_field_copy;
		return_code = 1;
		FE_node_field *node_field = existing_node_field_info->getNodeField(new_node_field->field);
		if (node_field)
		{
			if (existing_node_field_info->usedOnce())
			{
				if (node_field->access_count > 1)
				{
					/* Need to copy this node_field */
					node_field_copy = FE_node_field::clone(*node_field);
					REMOVE_OBJECT_FROM_LIST(FE_node_field)(node_field,
						existing_node_field_info->node_field_list);
					/* Update the time sequence */
					FE_node_field_set_FE_time_sequence(node_field_copy, new_node_field->time_sequence);
					ADD_OBJECT_TO_LIST(FE_node_field)(node_field_copy,
						existing_node_field_info->node_field_list);
				}
				else
				{
					FE_node_field_set_FE_time_sequence(node_field, new_node_field->time_sequence);
				}
				/* Should check there isn't a node_field equivalent to this modified one
					already in the list, and if there is use that instead */
			}
			else
			{
				/* Need to copy after all */
				node_field_list = CREATE_LIST(FE_node_field)();
				if (COPY_LIST(FE_node_field)(node_field_list, existing_node_field_info->node_field_list))
				{
					/* Find the correct node field in the new list */
					if (NULL != (node_field = FIND_BY_IDENTIFIER_IN_LIST(FE_node_field,field)(
						new_node_field->field, node_field_list)))
					{
						node_field_copy = FE_node_field::clone(*node_field);
						REMOVE_OBJECT_FROM_LIST(FE_node_field)(node_field, node_field_list);
						/* Update the time sequence */
						FE_node_field_set_FE_time_sequence(node_field_copy, new_node_field->time_sequence);
						ADD_OBJECT_TO_LIST(FE_node_field)(node_field_copy, node_field_list);
						/* create the new node information, number_of_values has not changed */
						struct FE_node_field_info *new_node_field_info = fe_nodeset->get_FE_node_field_info(
							existing_node_field_info->number_of_values, node_field_list);
						if (0 != new_node_field_info)
						{
							if (*node_field_info_address)
								FE_node_field_info::deaccess(*node_field_info_address);
							*node_field_info_address = new_node_field_info;
						}
					}
				}
				DESTROY(LIST(FE_node_field))(&node_field_list);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_nodeset_get_FE_node_field_info_adding_new_times.  Field not already defined.");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_nodeset_get_FE_node_field_info_adding_new_times.  Invalid argument(s)");
		return_code = 0;
	}
	return (return_code);
}

int define_FE_field_at_node(cmzn_node *node, FE_field *field,
	const FE_node_field_template *componentTemplatesIn,
	struct FE_time_sequence *time_sequence)
{
	FE_region *fe_region;
	FE_node_field_info *node_field_info;
	if (!(node && field && (fe_region = field->get_FE_region())
		&& (node_field_info = node->fields) && (node_field_info->nodeset)
		&& (node_field_info->nodeset->get_FE_region() == fe_region)
		&& (componentTemplatesIn)))
	{
		display_message(ERROR_MESSAGE, "define_FE_field_at_node.  Invalid argument(s)");
		return 0;
	}
	FE_node_field *node_field = FE_node_field::create(field);
	if (!node_field)
	{
		display_message(ERROR_MESSAGE, "define_FE_field_at_node.  Could not create node_field");
		return 0;
	}
	/* access now, deaccess at end to clean up if fails */
	node_field->access();
	int return_code = 1;
	const int componentCount = get_FE_field_number_of_components(field);
	Value_type value_type = field->getValueType();
	const int valueTypeSize = get_Value_storage_size(value_type, time_sequence);
	if (time_sequence)
	{
		FE_node_field_set_FE_time_sequence(node_field, time_sequence);
	}
	int new_values_storage_size = 0;
	int number_of_values = 0;
	for (int c = 0; c < componentCount; ++c)
	{
		// GRC following was previously only done for if (GENERAL_FE_FIELD == field->get_FE_field_type())
		node_field->components[c] = componentTemplatesIn[c];
		if (GENERAL_FE_FIELD == field->get_FE_field_type())
		{
			node_field->components[c].setValuesOffset(node_field_info->values_storage_size + new_values_storage_size);
			new_values_storage_size += componentTemplatesIn[c].getTotalValuesCount()*valueTypeSize;
			number_of_values += componentTemplatesIn[c].getTotalValuesCount();
		}
	}
	FE_node_field *existing_node_field = FIND_BY_IDENTIFIER_IN_LIST(FE_node_field, field)(
		field, node_field_info->node_field_list);
	if (existing_node_field)
	{
		FE_time_sequence *existing_time_sequence = (existing_node_field->time_sequence) ?
			ACCESS(FE_time_sequence)(existing_node_field->time_sequence) : 0;
		/* Check node fields are consistent or we are only adding times */
		/* Need to copy node field list in case it is modified  */
		struct LIST(FE_node_field) *new_node_field_list = CREATE_LIST(FE_node_field)();
		if (COPY_LIST(FE_node_field)(new_node_field_list, node_field_info->node_field_list))
		{
			Merge_FE_node_field_into_list_data merge_data;
			merge_data.requires_merged_storage = 0;
			merge_data.values_storage_size = 0;
			merge_data.number_of_values = node_field_info->number_of_values;
			merge_data.list = new_node_field_list;
			if (merge_FE_node_field_into_list(node_field, (void *)(&merge_data)))
			{
				if (merge_data.requires_merged_storage)
				{
					/* Time sequences are different or it would have failed */
					/* get the node_field_info for the new list */
					FE_node_field *merged_node_field = FIND_BY_IDENTIFIER_IN_LIST(FE_node_field,field)(
						field, new_node_field_list);
					FE_nodeset_get_FE_node_field_info_adding_new_times(
						node_field_info->nodeset, &node->fields,
						merged_node_field);
					FE_node_field *new_node_field = FIND_BY_IDENTIFIER_IN_LIST(FE_node_field,field)(
						field, node->fields->node_field_list);
					/* Update values storage, similar to copy_value_storage_array but
						we are updating existing storage and need to initialise the new values */
					/* Offsets must not have changed so we can use the existing_node_field */
					Value_storage *storage;
					storage = node->values_storage + existing_node_field->getComponent(0)->getValuesOffset();
					enum FE_time_sequence_mapping time_sequence_mapping =
						FE_time_sequences_mapping(existing_time_sequence, new_node_field->time_sequence);
					switch (time_sequence_mapping)
					{
						case FE_TIME_SEQUENCE_MAPPING_IDENTICAL:
						{
							/* Do nothing, in fact we shouldn't get here as the merge_data.requires_merged_storage
								should be false in this case */
						} break;
						case FE_TIME_SEQUENCE_MAPPING_APPEND:
						{
							const int previous_number_of_times = FE_time_sequence_get_number_of_times(existing_time_sequence);
							const int new_number_of_times = FE_time_sequence_get_number_of_times(new_node_field->time_sequence);
							for (int i = 0; i < number_of_values; ++i)
							{
								reallocate_time_values_storage_array(value_type,
									new_number_of_times, storage, storage,
									/*initialise_storage*/1, previous_number_of_times);
								storage += valueTypeSize;
							}
						} break;
						default:
						{
							/* Need a temporary pointer as we will be allocating the new
								memory, copying the existing values and then replacing the
								pointer with the new value */
							void *temp_storage;

							/* Fallback default implementation */
							for (int i = 0; (i < number_of_values) && return_code; ++i)
							{
								if (!(allocate_time_values_storage_array(value_type,
										new_node_field->time_sequence,(Value_storage *)&temp_storage,/*initialise_storage*/1) &&
									copy_time_sequence_values_storage_array(storage, value_type,
										existing_time_sequence, new_node_field->time_sequence, (Value_storage *)&temp_storage)))
								{
									display_message(ERROR_MESSAGE, "define_FE_field_at_node.  Failed to copy time array");
									return_code = 0;
								}
								/* Must free the src array now otherwise we will lose any reference to it */
								free_value_storage_array(storage, value_type, existing_time_sequence, 1);
								/* Update the value storage with the new pointer */
								*(void **)storage = temp_storage;
								storage += valueTypeSize;
							}
						} break;
					}
				}
				/* else existing time sequence includes new times so do nothing */
			}
			else
			{
				display_message(ERROR_MESSAGE, "define_FE_field_at_node.  "
					"Field already defined incompatibly at node.");
			}
		}
		if (existing_time_sequence)
		{
			DEACCESS(FE_time_sequence)(&existing_time_sequence);
		}
	}
	else
	{
		const int existing_values_storage_size = node_field_info->values_storage_size;
		if (node_field_info->nodeset->get_FE_node_field_info_adding_new_field(
			&node_field_info, node_field, node_field_info->number_of_values + number_of_values))
		{
			if (GENERAL_FE_FIELD == field->get_FE_field_type())
			{
				ADJUST_VALUE_STORAGE_SIZE(new_values_storage_size);
				Value_storage *new_value;
				if (REALLOCATE(new_value, node->values_storage, Value_storage,
					node_field_info->values_storage_size + new_values_storage_size))
				{
					node->values_storage = new_value;
					/* initialize new values */
					new_value += existing_values_storage_size;
					for (int i = number_of_values ; i > 0 ; i--)
					{
						if (time_sequence)
						{
							allocate_time_values_storage_array(value_type,
								time_sequence, new_value, /*initialise_storage*/1);
							new_value += valueTypeSize;
						}
						else
						{
							switch (value_type)
							{
								case ELEMENT_XI_VALUE:
								{
									*((struct FE_element **)new_value) =
										(struct FE_element *)NULL;
									new_value += sizeof(struct FE_element *);
									for (int j = 0; j < MAXIMUM_ELEMENT_XI_DIMENSIONS; j++)
									{
										*((FE_value *)new_value) = FE_VALUE_INITIALIZER;
										new_value += sizeof(FE_value);
									}
								} break;
								case FE_VALUE_VALUE:
								{
									*((FE_value *)new_value) = FE_VALUE_INITIALIZER;
									new_value += valueTypeSize;
								} break;
								case UNSIGNED_VALUE:
								{
									*((unsigned *)new_value) = 0;
									new_value += valueTypeSize;
								} break;
								case INT_VALUE:
								{
									*((int *)new_value) = 0;
									new_value += valueTypeSize;
								} break;
								case DOUBLE_VALUE:
								{
									*((double *)new_value) = 0;
									new_value += valueTypeSize;
								} break;
								case FLT_VALUE:
								{
									*((float *)new_value) = 0;
									new_value += valueTypeSize;
								} break;
								case SHORT_VALUE:
								{
									display_message(ERROR_MESSAGE,
										"define_FE_field_at_node.  SHORT_VALUE: Code not "
										"written yet. Beware alignmemt problems!");
									return_code = 0;
								} break;
								case DOUBLE_ARRAY_VALUE:
								{
									double *array = (double *)NULL;
									double **array_address;
									int zero = 0;
									/* copy number of array values to values_storage*/
									*((int *)new_value) = zero;
									/* copy pointer to array values to values_storage */
									array_address =
										(double **)(new_value + sizeof(int));
									*array_address = array;
									new_value += valueTypeSize;
								} break;
								case FE_VALUE_ARRAY_VALUE:
								{
									FE_value *array = (FE_value *)NULL;
									FE_value **array_address;
									int zero = 0;
									*((int *)new_value) = zero;
									array_address =
										(FE_value **)(new_value + sizeof(int));
									*array_address = array;
									new_value += valueTypeSize;
								} break;
								case FLT_ARRAY_VALUE:
								{
									float *array = 0;
									float **array_address;
									int zero = 0;
									*((int *)new_value) = zero;
									array_address = (float **)(new_value + sizeof(int));
									*array_address = array;
									new_value += valueTypeSize;
								} break;
								case SHORT_ARRAY_VALUE:
								{
									short *array = (short *)NULL;
									short **array_address;
									int zero = 0;
									*((int *)new_value) = zero;
									array_address = (short **)(new_value + sizeof(int));
									*array_address = array;
									new_value += valueTypeSize;
								} break;
								case  INT_ARRAY_VALUE:
								{
									int *array = (int *)NULL;
									int **array_address;
									int zero = 0;
									*((int *)new_value) = zero;
									array_address = (int **)(new_value + sizeof(int));
									*array_address = array;
									new_value += valueTypeSize;
								} break;
								case  UNSIGNED_ARRAY_VALUE:
								{
									unsigned *array = (unsigned *)NULL;
									unsigned **array_address;
									int zero = 0;
									*((int *)new_value) = zero;
									array_address =
										(unsigned **)(new_value + sizeof(int));
									*array_address = array;
									new_value += valueTypeSize;
								} break;
								case STRING_VALUE:
								{
									char **string_address;

									string_address = (char **)(new_value);
									*string_address = (char *)NULL;
									new_value += valueTypeSize;
								} break;
								default:
								{
									display_message(ERROR_MESSAGE,
										"define_FE_field_at_node.  Unsupported value_type");
									return_code = 0;
								} break;
							}
						}
					}
				}
				else
				{
					display_message(ERROR_MESSAGE, "define_FE_field_at_node.  "
						"Could not reallocate nodal values");
					return_code = 0;
				}
			}
			if (return_code)
			{
				/* already handled the accessing in FE_nodeset::get_FE_node_field_info_adding_new_field */
				node->fields = node_field_info;
			}
			else
			{
				FE_node_field_info::deaccess(node_field_info);
			}
		}
	}
	DEACCESS(FE_node_field)(&node_field);
	return (return_code);
}

struct FE_node_field_add_to_list_with_exclusion_data
{
	int value_exclusion_length, value_exclusion_start;
	struct FE_node_field *excluded_node_field;
	struct LIST(FE_node_field) *node_field_list;
};

static int FE_node_field_add_to_list_with_exclusion(
	struct FE_node_field *node_field,void *exclusion_data_void)
/*******************************************************************************
LAST MODIFIED : 14 September 2000

DESCRIPTION :
If <node_field> is before the excluded_node_field, it is added to the list.
If <node_field> is the excluded_node_field, it is ignored.
If <node_field> is after the excluded_node_field, a copy of it is made with
a new value reduced by the <value_exclusion_length>.
==============================================================================*/
{
	int return_code;
	struct FE_node_field_add_to_list_with_exclusion_data *exclusion_data;

	ENTER(FE_node_field_add_to_list_with_exclusion);
	if (node_field && (exclusion_data=
		(struct FE_node_field_add_to_list_with_exclusion_data *)
		exclusion_data_void))
	{
		return_code=1;
		if (node_field != exclusion_data->excluded_node_field)
		{
			if ((GENERAL_FE_FIELD == node_field->field->get_FE_field_type()) &&
				(node_field->getComponent(0)->getValuesOffset() > exclusion_data->value_exclusion_start))
			{
				/* create copy of node_field with component->value reduced
					 by value_exclusion_length */
				FE_node_field *offset_node_field = FE_node_field::clone(*node_field, -exclusion_data->value_exclusion_length);
				if (offset_node_field)
				{
					if (!ADD_OBJECT_TO_LIST(FE_node_field)(offset_node_field,
						exclusion_data->node_field_list))
					{
						delete[] offset_node_field;
						return_code=0;
					}
				}
				else
				{
					return_code=0;
				}
			}
			else
			{
				return_code=ADD_OBJECT_TO_LIST(FE_node_field)(node_field,
					exclusion_data->node_field_list);
			}
			if (!return_code)
			{
				display_message(ERROR_MESSAGE,
					"FE_node_field_add_to_list_with_exclusion.  Failed");
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_node_field_add_to_list_with_exclusion.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* FE_node_field_add_to_list_with_exclusion */

int undefine_FE_field_at_node(struct FE_node *node, struct FE_field *field)
{
	int bytes_to_copy,field_number_of_values,return_code;
	struct FE_node_field *node_field;
	struct FE_node_field_add_to_list_with_exclusion_data exclusion_data;
	struct FE_node_field_info *existing_node_field_info;
	struct FE_region *fe_region;
	Value_storage *values_storage;

	if (node && field && (fe_region = field->get_FE_region()) &&
		(existing_node_field_info = node->fields) &&
		(existing_node_field_info->nodeset) &&
		(existing_node_field_info->nodeset->get_FE_region() == fe_region))
	{
		/* check if the field is defined at the node */
		if (NULL != (node_field = FIND_BY_IDENTIFIER_IN_LIST(FE_node_field,field)(field,
			existing_node_field_info->node_field_list)))
		{
			field_number_of_values = node_field->getTotalValuesCount();
			if (GENERAL_FE_FIELD == field->get_FE_field_type())
			{
				exclusion_data.value_exclusion_start = node_field->getComponent(0)->getValuesOffset();
				exclusion_data.value_exclusion_length = field_number_of_values *
					get_Value_storage_size(node_field->field->getValueType(),
						node_field->time_sequence);
				/* adjust size for proper word alignment in memory */
				ADJUST_VALUE_STORAGE_SIZE(exclusion_data.value_exclusion_length);
			}
			else
			{
				exclusion_data.value_exclusion_start =
					existing_node_field_info->values_storage_size;
				exclusion_data.value_exclusion_length = 0;
			}
			exclusion_data.excluded_node_field = node_field;
			exclusion_data.node_field_list = CREATE(LIST(FE_node_field))();
			if (FOR_EACH_OBJECT_IN_LIST(FE_node_field)(
				FE_node_field_add_to_list_with_exclusion, (void *)&exclusion_data,
				existing_node_field_info->node_field_list))
			{
				struct FE_node_field_info *new_node_field_info =
					existing_node_field_info->nodeset->get_FE_node_field_info(
						existing_node_field_info->number_of_values - field_number_of_values,
						exclusion_data.node_field_list);
				if (0 != new_node_field_info)
				{
					return_code = CMZN_OK;
					if (0 < exclusion_data.value_exclusion_length)
					{
						/* free arrays, embedded locations */
						FE_node_field_free_values_storage_arrays(node_field, (void*)node->values_storage);
						/* copy values_storage after the removed field */
						bytes_to_copy = existing_node_field_info->values_storage_size -
							(exclusion_data.value_exclusion_start +
								exclusion_data.value_exclusion_length);
						if (0<bytes_to_copy)
						{
							/* use memmove instead of memcpy as memory blocks overlap */
							memmove(node->values_storage+exclusion_data.value_exclusion_start,
								node->values_storage+exclusion_data.value_exclusion_start+
								exclusion_data.value_exclusion_length,bytes_to_copy);
						}
						if (0 == new_node_field_info->values_storage_size)
						{
							DEALLOCATE(node->values_storage); // avoids warning about zero size
						}
						else if (REALLOCATE(values_storage,node->values_storage,Value_storage,
							new_node_field_info->values_storage_size))
						{
							node->values_storage=values_storage;
						}
						else
						{
							display_message(ERROR_MESSAGE, "undefine_FE_field_at_node.  Reallocate failed");
							return_code = CMZN_ERROR_MEMORY;
						}
					}
					FE_node_field_info::deaccess(node->fields);
					node->fields = new_node_field_info;
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"undefine_FE_field_at_node.  Could not create node field info");
					return_code= CMZN_ERROR_GENERAL;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"undefine_FE_field_at_node.  Could not copy node field list");
				return_code=CMZN_ERROR_GENERAL;
			}
			DESTROY(LIST(FE_node_field))(&exclusion_data.node_field_list);
		}
		else
		{
			//display_message(WARNING_MESSAGE,
			//	"undefine_FE_field_at_node.  Field %s is not defined at node %d",
			//	field->getName(),node->getIdentifier());
			return_code = CMZN_ERROR_NOT_FOUND;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"undefine_FE_field_at_node.  Invalid argument(s)");
		return_code=CMZN_ERROR_ARGUMENT;
	}
	LEAVE;

	return (return_code);
}

int for_FE_field_at_node(struct FE_field *field,
	FE_node_field_iterator_function *iterator,void *user_data,
	struct FE_node *node)
/*******************************************************************************
LAST MODIFIED : 10 February 1999

DESCRIPTION :
If an <iterator> is supplied and the <field> is defined at the <node> then the
result of the <iterator> is returned.  Otherwise, if an <iterator> is not
supplied and the <field> is defined at the <node> then a non-zero is returned.
Otherwise, zero is returned.
???DB.  Multiple behaviour dangerous ?
==============================================================================*/
{
	int return_code;
	struct FE_node_field *node_field;
	struct FE_node_field_iterator_and_data iterator_and_data;

	ENTER(for_FE_field_at_node);
	return_code=0;
	if (field&&node&&(node->fields))
	{
		node_field=FIND_BY_IDENTIFIER_IN_LIST(FE_node_field,field)(field,
			node->fields->node_field_list);

		if (node_field)
		{
			if (iterator)
			{
				iterator_and_data.iterator=iterator;
				iterator_and_data.user_data=user_data;
				iterator_and_data.node=node;
				return_code=for_FE_field_at_node_iterator(node_field,
					&iterator_and_data);
			}
			else
			{
				return_code=1;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"for_FE_field_at_node.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* for_FE_field_at_node */

int for_each_FE_field_at_node(FE_node_field_iterator_function *iterator,
	void *user_data,struct FE_node *node)
/*******************************************************************************
LAST MODIFIED : 10 February 1999

DESCRIPTION :
Calls the <iterator> for each field defined at the <node> until the <iterator>
returns 0 or it runs out of fields.  Returns the result of the last <iterator>
called.
==============================================================================*/
{
	int return_code;
	struct FE_node_field_iterator_and_data iterator_and_data;

	ENTER(for_each_FE_field_at_node);
	return_code=0;
	if (iterator&&node&&(node->fields))
	{
		iterator_and_data.iterator=iterator;
		iterator_and_data.user_data=user_data;
		iterator_and_data.node=node;
		return_code=FOR_EACH_OBJECT_IN_LIST(FE_node_field)(
			for_FE_field_at_node_iterator,&iterator_and_data,
			node->fields->node_field_list);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"for_each_FE_field_at_node.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* for_each_FE_field_at_node */

int for_each_FE_field_at_node_alphabetical_indexer_priority(
	FE_node_field_iterator_function *iterator,void *user_data,
	struct FE_node *node)
{
	int i, number_of_fields, return_code;
	struct FE_field *field;
	struct FE_field_order_info *field_order_info;
	struct FE_node_field *node_field;

	ENTER(for_each_FE_field_at_node_alphabetical_indexer_priority);
	return_code = 0;
	if (iterator && node && node->fields)
	{
		// get list of all fields in default alphabetical order
		field_order_info = CREATE(FE_field_order_info)();
		FE_region *fe_region = node->fields->nodeset->get_FE_region();
		return_code = FE_region_for_each_FE_field(fe_region,
			FE_field_add_to_FE_field_order_info, (void *)field_order_info);
		FE_field_order_info_prioritise_indexer_fields(field_order_info);
		number_of_fields = get_FE_field_order_info_number_of_fields(field_order_info);
		for (i = 0; i < number_of_fields; i++)
		{
			field = get_FE_field_order_info_field(field_order_info, i);
			node_field = FIND_BY_IDENTIFIER_IN_LIST(FE_node_field,field)(field,
				node->fields->node_field_list);
			if (node_field)
			{
				return_code = (iterator)(node, field, user_data);
			}
		}
		DESTROY(FE_field_order_info)(&field_order_info);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"for_each_FE_field_at_node_alphabetical_indexer_priority.  "
			"Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
}

static int FE_node_field_has_FE_field_values(
	struct FE_node_field *node_field,void *dummy)
/*******************************************************************************
LAST MODIFIED: 19 October 1999

DESCRIPTION:
Returns true if <node_field> has a field with values_storage.
==============================================================================*/
{
	int return_code;

	ENTER(FE_node_field_has_FE_field_values);
	USE_PARAMETER(dummy);
	if (node_field&&node_field->field)
	{
		return_code=(0<node_field->field->getNumberOfValues());
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_node_field_has_FE_field_values.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_node_field_has_FE_field_values */

int FE_node_has_FE_field_values(struct FE_node *node)
/*******************************************************************************
LAST MODIFIED : 24 September 1999

DESCRIPTION :
Returns true if any single field defined at <node> has values stored with
the field.
==============================================================================*/
{
	int return_code;

	ENTER(FE_node_has_FE_field_values);
	if (node&&node->fields)
	{
		return_code=((struct FE_node_field *)NULL !=
			FIRST_OBJECT_IN_LIST_THAT(FE_node_field)(
				FE_node_field_has_FE_field_values,(void *)NULL,
				node->fields->node_field_list));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_node_has_FE_field_values.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* FE_node_has_FE_field_values */

int FE_node_get_element_usage_count(struct FE_node *node)
{
	if (node)
		return node->getElementUsageCount();
	return 0;
}

FE_nodeset *FE_node_get_FE_nodeset(struct FE_node *node)
{
	if (node)
		return node->getNodeset();
	return 0;
}

int FE_node_to_node_string(struct FE_node *node, char **string_address)
/*****************************************************************************
LAST MODIFIED : 20 March 2003

DESCRIPTION :
Returns an allocated <string> of the identifier of <node>.
============================================================================*/
{
	char tmp_string[50];
	int return_code;

	ENTER(FE_node_to_node_string);
	if (node && string_address)
	{
		sprintf(tmp_string, "%d", get_FE_node_identifier(node));
		*string_address = duplicate_string(tmp_string);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_node_to_node_string.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_node_to_node_string */

int equivalent_FE_field_at_nodes(struct FE_field *field,struct FE_node *node_1,
	struct FE_node *node_2)
/*******************************************************************************
LAST MODIFIED : 26 February 2003

DESCRIPTION :
Returns non-zero if the <field> is defined in the same way at the two nodes.
Note this will also return true if <field> is not defined at both nodes.
==============================================================================*/
{
	int return_code;
	struct FE_node_field *node_field_1, *node_field_2;

	ENTER(equivalent_FE_field_at_nodes);
	return_code = 0;
	if (field && node_1 && node_2)
	{
		if (node_1->fields == node_2->fields)
		{
			return_code = 1;
		}
		else
		{
			node_field_1 = FIND_BY_IDENTIFIER_IN_LIST(FE_node_field,field)(field,
				node_1->fields->node_field_list);
			node_field_2 = FIND_BY_IDENTIFIER_IN_LIST(FE_node_field,field)(field,
				node_2->fields->node_field_list);
			return_code = ((!node_field_1) && (!node_field_2)) || (
				node_field_1 && node_field_2 &&
				FE_node_fields_match(node_field_1, node_field_2,
					/*compare_field_and_time_sequence*/true, /*compare_component_value*/false));
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"equivalent_FE_field_at_nodes.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* equivalent_FE_field_at_nodes */

int equivalent_FE_fields_at_nodes(struct FE_node *node_1,
	struct FE_node *node_2)
/*******************************************************************************
LAST MODIFIED : 23 May 2000

DESCRIPTION :
Returns true if all fields are defined in the same way at the two nodes.
==============================================================================*/
{
	int return_code;

	ENTER(equivalent_FE_fields_at_nodes);
	return_code=0;
	if (node_1&&(node_1->fields)&&node_2&&(node_2->fields)&&
		(node_1->fields==node_2->fields))
	{
		return_code=1;
	}
	else
	{
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* equivalent_FE_fields_at_nodes */

/**
 * Get node field parameters for valueLabel, version at time.
 * Note if parameters only exist for some of the requested components, absent
 * parameters are set to zero, and WARNING_PART_DONE is returned.
 * @param componentNumber  Field component number starting at 0, or negative to
 * get all component values.
 * @param valuesOut  Array to receive values of size expected for componentNumber.
 * @return  Result OK on full success, WARNING_PART_DONE if only some components
 * have parameters, ERROR_NOT_FOUND if field not defined or none of the requested
 * components have parameters, otherwise any other error code.
 */
template <typename VALUE_TYPE> int cmzn_node_get_field_parameters(
	cmzn_node *node, FE_field *field, int componentNumber,
	cmzn_node_value_label valueLabel, int version, FE_value time, VALUE_TYPE *valuesOut)
{
	if (!(node && node->fields && field && (componentNumber < field->getNumberOfComponents()) && (0 <= version) && valuesOut))
	{
		display_message(ERROR_MESSAGE, "cmzn_node_get_field_parameters<VALUE_TYPE>.  Invalid arguments node %d field %s",
			(node) ? node->getIdentifier() : -1, field ? field->getName() : "UNKNOWN");
		return CMZN_ERROR_ARGUMENT;
	}
	const FE_node_field *node_field = FIND_BY_IDENTIFIER_IN_LIST(FE_node_field, field)(
		field, node->fields->node_field_list);
	if (!node_field)
	{
		return CMZN_ERROR_NOT_FOUND;
	}
	const int componentStart = (componentNumber < 0) ? 0 : componentNumber;
	const int componentLimit = (componentNumber < 0) ? field->getNumberOfComponents() : componentNumber + 1;
	VALUE_TYPE *valueOut = valuesOut;
	switch (field->get_FE_field_type())
	{
	case GENERAL_FE_FIELD:
	{
		if (!node->values_storage)
		{
			display_message(ERROR_MESSAGE, "cmzn_node_get_field_parameters<VALUE_TYPE>.  Node has no values storage");
			return CMZN_ERROR_GENERAL;
		}
		int time_index_one = 0, time_index_two = 0;
		FE_value time_xi = 0.0;
		if (node_field->time_sequence)
		{
			FE_time_sequence_get_interpolation_for_time(node_field->time_sequence,
				time, &time_index_one, &time_index_two, &time_xi);
		}
		const FE_value one_minus_time_xi = 1.0 - time_xi;
		const int valueTypeSize = get_Value_storage_size(field->getValueType(), node_field->time_sequence);
		int componentsFound = 0;
		for (int c = componentStart; c < componentLimit; ++c)
		{
			const FE_node_field_template *nft = node_field->getComponent(c);
			const int valueIndex = nft->getValueIndex(valueLabel, version);
			if (valueIndex < 0)
			{
				*valueOut = 0;
			}
			else
			{
				Value_storage *valuesStorage = node->values_storage + nft->getValuesOffset() + valueIndex*valueTypeSize;
				if (node_field->time_sequence)
				{
					const VALUE_TYPE *timeValues = *((VALUE_TYPE **)valuesStorage);
					*valueOut = timeValues[time_index_one]*one_minus_time_xi + timeValues[time_index_two]*time_xi;
				}
				else
				{
					*valueOut = *((VALUE_TYPE *)valuesStorage);
				}
				++componentsFound;
			}
			++valueOut;
		}
		if (0 == componentsFound)
		{
			return CMZN_ERROR_NOT_FOUND;
		}
		if (componentsFound < (componentLimit - componentStart))
		{
			return CMZN_WARNING_PART_DONE;
		}
		return CMZN_OK;
	} break;
	case CONSTANT_FE_FIELD:
	{
		const VALUE_TYPE *fieldValues;
		field->getValues(fieldValues);
		for (int c = componentStart; c < componentLimit; ++c)
		{
			*valueOut = fieldValues[c];
			++valueOut;
		}
		return CMZN_OK;
	} break;
	case INDEXED_FE_FIELD:
	{
		int index;
		if (cmzn_node_get_field_parameters(node, field->getIndexerField(),
			/*componentNumber*/0, CMZN_NODE_VALUE_LABEL_VALUE, /*version*/0, time, &index))
		{
			/* index numbers start at 1 */
			const int indexedValuesCount = field->getNumberOfIndexedValues();
			if ((1 <= index) && (index <= indexedValuesCount))
			{
				const VALUE_TYPE *fieldValues;
				field->getValues(fieldValues);
				for (int c = componentStart; c < componentLimit; ++c)
				{
					*valueOut = fieldValues[indexedValuesCount*c + index - 1];
					++valueOut;
				}
				return CMZN_OK;
			}
			else
			{	
				display_message(ERROR_MESSAGE,"cmzn_node_get_field_parameters<VALUE_TYPE>.  "
					"Index field %s gave out-of-range index %d for field %s at node %d",
					field->getIndexerField()->getName(), index, field->getName(), node->getIdentifier());
			}
		}
	} break;
	default:
	{
		display_message(ERROR_MESSAGE,
			"cmzn_node_get_field_parameters<VALUE_TYPE>.  Unknown FE_field_type");
	} break;
	}
	return CMZN_ERROR_GENERAL;
}

/**
 * Set node field parameters for valueLabel, version at time.
 * Note if parameters only exist for some of the requested components, sets
 * those that do exist and returns WARNING_PART_DONE.
 * Notifies of node field changes.
 * @param componentNumber  Field component number starting at 0, or negative to
 * set all component values.
 * @param valuesIn  Array of in-values of size expected for componentNumber.
 * @return  Result OK on full success, WARNING_PART_DONE if only some components
 * have parameters (and which were set), ERROR_NOT_FOUND if field not defined
 * or none of the requested components have parameters, otherwise any other error code.
 */
template <typename VALUE_TYPE> int cmzn_node_set_field_parameters(
	cmzn_node *node, FE_field *field, int componentNumber,
	cmzn_node_value_label valueLabel, int version, FE_value time, const VALUE_TYPE *valuesIn)
{
	if (!(node && node->fields && field && (componentNumber < field->getNumberOfComponents()) && (0 <= version) && valuesIn))
	{
		display_message(ERROR_MESSAGE, "cmzn_node_set_field_parameters<VALUE_TYPE>.  Invalid arguments node %d field %s",
			(node) ? node->getIdentifier() : -1, field ? field->getName() : "UNKNOWN");
		return CMZN_ERROR_ARGUMENT;
	}
	const FE_node_field *node_field = FIND_BY_IDENTIFIER_IN_LIST(FE_node_field, field)(
		field, node->fields->node_field_list);
	if (!node_field)
	{
		return CMZN_ERROR_NOT_FOUND;
	}
	if (field->get_FE_field_type() != GENERAL_FE_FIELD)
	{
		display_message(ERROR_MESSAGE, "cmzn_node_set_field_parameters<VALUE_TYPE>.  Node %d field %s is not GENERAL type",
			node->getIdentifier(), field->getName());
		return CMZN_ERROR_NOT_IMPLEMENTED;
	}
	if (!node->values_storage)
	{
		display_message(ERROR_MESSAGE, "cmzn_node_set_field_parameters<VALUE_TYPE>.  Node has no values storage");
		return CMZN_ERROR_GENERAL;
	}
	const int componentStart = (componentNumber < 0) ? 0 : componentNumber;
	const int componentLimit = (componentNumber < 0) ? field->getNumberOfComponents() : componentNumber + 1;
	const VALUE_TYPE *valueIn = valuesIn;
	int time_index = 0;
	if ((node_field->time_sequence) && !FE_time_sequence_get_index_for_time(node_field->time_sequence, time, &time_index))
	{
		display_message(ERROR_MESSAGE, "cmzn_node_set_field_parameters<VALUE_TYPE>.  "
			"Node %d field %s has no parameters at time %g", node->getIdentifier(), field->getName(), time);
		return CMZN_ERROR_ARGUMENT;
	}
	const int valueTypeSize = get_Value_storage_size(field->getValueType(), node_field->time_sequence);
	int componentsFound = 0;
	for (int c = componentStart; c < componentLimit; ++c)
	{
		const FE_node_field_template *nft = node_field->getComponent(c);
		const int valueIndex = nft->getValueIndex(valueLabel, version);
		if (valueIndex >= 0)
		{
			Value_storage *valuesStorage = node->values_storage + nft->getValuesOffset() + valueIndex*valueTypeSize;
			if (node_field->time_sequence)
			{
				VALUE_TYPE *timeValues = *((VALUE_TYPE **)valuesStorage);
				timeValues[time_index] = *valueIn;
			}
			else
			{
				*((VALUE_TYPE *)valuesStorage) = *valueIn;
			}
			++componentsFound;
		}
		++valueIn;
	}
	if (0 == componentsFound)
	{
		return CMZN_ERROR_NOT_FOUND;
	}
	// notify of changes, but only for valid nodes (i.e. not template nodes)
	if (node->getIndex() >= 0)
	{
		node->fields->nodeset->nodeFieldChange(node, field);
	}
	if (componentsFound < (componentLimit - componentStart))
	{
		return CMZN_WARNING_PART_DONE;
	}
	return CMZN_OK;
}

#define INSTANTIATE_GET_FE_NODAL_VALUE_FUNCTION( value_typename, value_enum ) \
int get_FE_nodal_ ## value_typename ## _value(cmzn_node *node, FE_field *field, \
	int componentNumber, cmzn_node_value_label valueLabel, int version, \
	FE_value time, value_typename *valuesOut) \
{ \
	if (!(field && (field->getValueType() == value_enum))) \
	{ \
		display_message(ERROR_MESSAGE, \
			"get_FE_nodal_" #value_typename "_value.  Invalid arguments"); \
		return CMZN_ERROR_ARGUMENT; \
	} \
	return cmzn_node_get_field_parameters(node, field, componentNumber, valueLabel, version, time, valuesOut); \
}

#define INSTANTIATE_SET_FE_NODAL_VALUE_FUNCTION( value_typename, value_enum ) \
int set_FE_nodal_ ## value_typename ## _value(cmzn_node *node, FE_field *field, \
	int componentNumber, cmzn_node_value_label valueLabel, int version, \
	FE_value time, const value_typename *valuesIn) \
{ \
	if (!(field && (field->getValueType() == value_enum))) \
	{ \
		display_message(ERROR_MESSAGE, \
			"set_FE_nodal_" #value_typename "_value.  Invalid arguments"); \
		return CMZN_ERROR_ARGUMENT; \
	} \
	return cmzn_node_set_field_parameters(node, field, componentNumber, valueLabel, version, time, valuesIn); \
}

#define INSTANTIATE_GET_FE_NODAL_VALUE_STORAGE_FUNCTION( value_typename, value_enum ) \
int get_FE_nodal_ ## value_typename ## _storage(cmzn_node *node, FE_field *field, \
	int componentNumber, cmzn_node_value_label valueLabel, int version, \
	FE_value time, value_typename **valueAddress) \
{ \
	int return_code = CMZN_OK; \
	if (node && field && (0 <= componentNumber) && \
		(componentNumber < field->getNumberOfComponents()) && (0<=version)) \
	{ \
		struct FE_time_sequence *time_sequence; \
		Value_storage *valuesStorage = NULL; \
		return_code = find_FE_nodal_values_storage_dest(node, field, componentNumber, \
			valueLabel, version, valuesStorage, time_sequence); \
		if (CMZN_OK == return_code) \
		{ \
			if (time_sequence) \
			{ \
				int time_index; \
				if (FE_time_sequence_get_index_for_time(time_sequence, time, &time_index)) \
				{ \
					value_typename *timeValues = *((value_typename **)valuesStorage); \
					*valueAddress = &(timeValues[time_index]); \
				} \
				else \
				{ \
					display_message(ERROR_MESSAGE,"get_FE_nodal_" #value_typename "_storage.  " \
						"Time value for time %g not defined at this node.", time); \
					return_code = CMZN_ERROR_ARGUMENT; \
				} \
			} \
			else \
			{ \
				/* copy in the value */ \
				*valueAddress = (value_typename *)valuesStorage; \
			} \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"get_FE_nodal_" #value_typename "_storage.  Invalid argument(s)"); \
		return_code = CMZN_ERROR_ARGUMENT; \
	} \
	return (return_code); \
}

#define INSTANTIATE_FE_NODAL_VALUE_FUNCTIONS( value_typename , value_enum ) \
INSTANTIATE_GET_FE_NODAL_VALUE_FUNCTION(value_typename,value_enum) \
INSTANTIATE_SET_FE_NODAL_VALUE_FUNCTION(value_typename,value_enum) \
INSTANTIATE_GET_FE_NODAL_VALUE_STORAGE_FUNCTION(value_typename,value_enum)

INSTANTIATE_FE_NODAL_VALUE_FUNCTIONS( FE_value , FE_VALUE_VALUE )
INSTANTIATE_FE_NODAL_VALUE_FUNCTIONS( double , DOUBLE_VALUE )
INSTANTIATE_FE_NODAL_VALUE_FUNCTIONS( float , FLT_VALUE )
INSTANTIATE_FE_NODAL_VALUE_FUNCTIONS( int , INT_VALUE )
INSTANTIATE_FE_NODAL_VALUE_FUNCTIONS( short , SHORT_VALUE )

int get_FE_nodal_element_xi_value(struct FE_node *node,
	FE_field *field, int component_number,
	cmzn_element **element, FE_value *xi)
{
	int return_code = 0;
	if (node && field && (field->getValueType() == ELEMENT_XI_VALUE)
		&& (0 <= component_number)&& (component_number < field->getNumberOfComponents())
		&& (element) &&(xi))
	{
		Value_storage *valuesStorage = 0;
		switch (field->get_FE_field_type())
		{
			case CONSTANT_FE_FIELD:
			{
				display_message(ERROR_MESSAGE, "get_FE_nodal_element_xi_value.  Field %s CONSTANT ELEMENT_XI value is not supported", field->getName());
				return_code = 0;
			} break;
			case GENERAL_FE_FIELD:
			{
				struct FE_time_sequence *time_sequence;
				if (CMZN_OK == find_FE_nodal_values_storage_dest(node,field,component_number,
					CMZN_NODE_VALUE_LABEL_VALUE, /*version*/0, valuesStorage, time_sequence))
				{
					return_code = 1;
				}
				else
				{
					display_message(ERROR_MESSAGE, "get_FE_nodal_element_xi_value.  Field %s is not defined", field->getName());
				}
			} break;
			case INDEXED_FE_FIELD:
			{
				display_message(ERROR_MESSAGE, "get_FE_nodal_element_xi_value.  Field %s INDEXED ELEMENT_XI value is not supported", field->getName());
				return_code = 0;
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"get_FE_nodal_element_xi_value.  Unknown FE_field_type");
			} break;
		}
		if (return_code && valuesStorage)
		{
			*element = *((cmzn_element **)valuesStorage);
			valuesStorage += sizeof(struct FE_element *);
			if (*element)
			{
				const int dimension = (*element)->getDimension();
				for (int i = 0; i < dimension; i++)
				{
					xi[i] = *((FE_value *)valuesStorage);
					valuesStorage += sizeof(FE_value);
				}
			}
		}
		else
		{
			if (return_code)
			{
				display_message(ERROR_MESSAGE,
					"get_FE_nodal_element_xi_value.  No values storage");
				return_code=0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"get_FE_nodal_element_xi_value.  Invalid argument(s)");
	}
	return (return_code);
}

int set_FE_nodal_element_xi_value(struct FE_node *node,
	FE_field *field, int component_number,
	cmzn_element *element, const FE_value *xi)
{
	int return_code = 0;
	int dimension = 0;
	// now require host mesh to be set; legacy inputs must ensure set with first element
	if (node && field && (field->getValueType() == ELEMENT_XI_VALUE) && (field->getElementXiHostMesh()) &&
		(0 <= component_number) && (component_number < field->getNumberOfComponents()) &&
		((!element) || ((0 < (dimension = element->getDimension())) && xi)))
	{
		if (element && (!field->getElementXiHostMesh()->containsElement(element)))
		{
			display_message(ERROR_MESSAGE, "set_FE_nodal_element_xi_value.  %d-D element %d is not from host mest %s for field %s ",
				dimension, element->getIdentifier(), field->getElementXiHostMesh()->getName(), field->getName());
			return 0;
		}
		Value_storage *valuesStorage = 0;
		FE_time_sequence *time_sequence = 0;
		/* get the values storage */
		if (CMZN_OK == find_FE_nodal_values_storage_dest(node, field, component_number,
			CMZN_NODE_VALUE_LABEL_VALUE, /*version*/0, valuesStorage, time_sequence))
		{
			/* copy in the element_xi_value */
			cmzn_element** elementAddress = reinterpret_cast<cmzn_element**>(valuesStorage);
			cmzn_element *oldElement = *elementAddress;
			if (element != oldElement)
			{
				FE_mesh_embedded_node_field *embeddedNodeField = field->getEmbeddedNodeField(node->getNodeset());
				if (oldElement)
				{
					cmzn_element::deaccess(*elementAddress);
					embeddedNodeField->removeNode(oldElement->getIndex(), node->getIndex());
				}
				if (element)
				{
					*elementAddress = element->access();
					embeddedNodeField->addNode(element->getIndex(), node->getIndex());
				}
			}
			FE_value *xiStored = reinterpret_cast<FE_value*>(elementAddress + 1);
			for (int i = 0; i < dimension; ++i)
				xiStored[i] = xi[i];
			// set spare xi values to zero
			for (int i = dimension; i < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++i)
				xiStored[i] = 0.0;
			// notify of changes, but only for valid nodes (i.e. not template nodes)
			if (node->getIndex() >= 0)
			{
				node->fields->nodeset->nodeFieldChange(node, field);
			}
			return_code=1;
		}
		else
		{
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE, "set_FE_nodal_element_xi_value.  Invalid argument(s)");
	}
	return (return_code);
}

int get_FE_nodal_string_value(struct FE_node *node,
	FE_field *field, int componentNumber, char **stringOut)
{
	int return_code = 0;
	if (node && field && (field->getValueType() == STRING_VALUE) && (0 <= componentNumber)
		&& (componentNumber < field->getNumberOfComponents()) && stringOut)
	{
		const char **stringValues = nullptr;
		switch (field->get_FE_field_type())
		{
		case CONSTANT_FE_FIELD:
		{
			// should only be 1 component number for string values...
			stringValues = field->getStringValues() + componentNumber;
			return_code = 1;
		} break;
		case GENERAL_FE_FIELD:
		{
			Value_storage *valuesStorage;
			struct FE_time_sequence *time_sequence;
			if (CMZN_OK == find_FE_nodal_values_storage_dest(node, field, componentNumber,
				CMZN_NODE_VALUE_LABEL_VALUE, /*version*/0, valuesStorage, time_sequence))
			{
				stringValues = reinterpret_cast<const char **>(valuesStorage);
				return_code = 1;
			}
		} break;
		case INDEXED_FE_FIELD:
		{
			int index;
			if (cmzn_node_get_field_parameters(node, field->getIndexerField(),
				/*componentNumber*/0, CMZN_NODE_VALUE_LABEL_VALUE, /*version*/0,
				/*time*/0, &index))
			{
				/* index numbers start at 1 */
				const int indexedValuesCount = field->getNumberOfIndexedValues();
				if ((1 <= index) && (index <= indexedValuesCount))
				{
					stringValues = field->getStringValues() +
						(indexedValuesCount*componentNumber + index - 1);
					return_code = 1;
				}
				else
				{
					display_message(ERROR_MESSAGE, "get_FE_nodal_string_value.  "
						"Index field %s gave out-of-range index %d in field %s",
						field->getIndexerField()->getName(), index, field->getName());
				}
			}
			else
			{
				display_message(ERROR_MESSAGE, "get_FE_nodal_string_value.  "
					"Field %s, indexed by %s not defined at node %",
					field->getName(), field->getIndexerField()->getName(), node->getIdentifier());
				return_code = 0;
			}
		} break;
		default:
		{
			display_message(ERROR_MESSAGE,
				"get_FE_nodal_string_value.  Unknown FE_field_type");
		} break;
		}
		if (return_code && stringValues)
		{
			const char *theString = *stringValues;
			if (theString)
			{
				if (ALLOCATE(*stringOut, char, strlen(theString) + 1))
				{
					strcpy(*stringOut, theString);
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"get_FE_nodal_string_value.  Not enough memory");
					return_code = 0;
				}
			}
			else
			{
				/* no string, so successfully return NULL */
				*stringOut = (char *)NULL;
			}
		}
		else
		{
			if (return_code)
			{
				display_message(ERROR_MESSAGE,
					"get_FE_nodal_element_xi_value.  No values storage");
				return_code = 0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE, "get_FE_nodal_string_value.  Invalid argument(s)");
	}
	return (return_code);
}

int set_FE_nodal_string_value(struct FE_node *node,
	FE_field *field, int componentNumber, const char *stringIn)
{
	int return_code = 0;
	if (node && field && (field->getValueType() == STRING_VALUE) && (0 <= componentNumber) &&
		(componentNumber < field->getNumberOfComponents()))
	{
		Value_storage *valuesStorage = 0;
		struct FE_time_sequence *time_sequence;
		if (CMZN_OK == find_FE_nodal_values_storage_dest(node, field, componentNumber,
			CMZN_NODE_VALUE_LABEL_VALUE, /*version*/0, valuesStorage, time_sequence))
		{
			/* get the pointer to the stored string */
			char **stringAddress = (char **)(valuesStorage);
			if (stringIn)
			{
				/* reallocate the string currently there */
				char *theString;
				if (REALLOCATE(theString, *stringAddress, char, strlen(stringIn) + 1))
				{
					strcpy(theString, stringIn);
					*stringAddress = theString;
					// notify of changes, but only for valid nodes (i.e. not template nodes)
					if (node->getIndex() >= 0)
					{
						node->fields->nodeset->nodeFieldChange(node, field);
					}
					return_code = 1;
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"set_FE_nodal_string_value.  Not enough memory");
				}
			}
			else
			{
				/* NULL string; free the existing string */
				if (*stringAddress)
				{
					DEALLOCATE(*stringAddress);
				}
				return_code = 1;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE, "set_FE_nodal_string_value.  Field %s is not defined", field->getName());
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"set_FE_nodal_string_value.  Invalid argument(s)");
	}
	return (return_code);
}

char *get_FE_nodal_value_as_string(struct FE_node *node, FE_field *field,
	int componentNumber, cmzn_node_value_label valueLabel, int version,
	FE_value time)
{
	if (!(node && field && (0 <= componentNumber)&&
		(componentNumber < field->getNumberOfComponents()) && (0 <= version)))
	{
		display_message(ERROR_MESSAGE, "get_FE_nodal_value_as_string.  Invalid argument(s)");
		return 0;
	}
	char temp_string[40];
	char *returnString = 0;
	switch (field->getValueType())
	{
	case ELEMENT_XI_VALUE:
	{
		cmzn_element *element;
		FE_value xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
		if (get_FE_nodal_element_xi_value(node, field, componentNumber, &element, xi) && element)
		{
			const int dimension = element->getDimension();
			int error = 0;
			if (dimension == 1)
				append_string(&returnString, "L", &error);
			else if (dimension == 2)
				append_string(&returnString, "F", &error);
			else
				append_string(&returnString, "E", &error);
			sprintf(temp_string, " %d", element->getIdentifier());
			append_string(&returnString, temp_string, &error);
			for (int d = 0; d < dimension; ++d)
			{
				sprintf(temp_string, " %g", xi[d]);
				append_string(&returnString, temp_string, &error);
			}
		}
	} break;
	case FE_VALUE_VALUE:
	{
		FE_value value;
		if (cmzn_node_get_field_parameters(node, field, componentNumber, valueLabel, version, time, &value))
		{
			sprintf(temp_string, "%g", value);
			returnString = duplicate_string(temp_string);
		}
	} break;
	case INT_VALUE:
	{
		int value;
		if (cmzn_node_get_field_parameters(node, field, componentNumber, valueLabel, version, time, &value))
		{
			sprintf(temp_string, "%d", value);
			returnString = duplicate_string(temp_string);
		}
	} break;
	case STRING_VALUE:
	{
		get_FE_nodal_string_value(node, field, componentNumber, &returnString);
	} break;
	default:
	{
		display_message(ERROR_MESSAGE,
			"get_FE_nodal_value_as_string.  Unknown value type %s",
			Value_type_string(field->getValueType()));
	} break;
	}
	return returnString;
}

int FE_node_is_in_Multi_range(struct FE_node *node,void *multi_range_void)
/*******************************************************************************
LAST MODIFIED : 4 July 2000

DESCRIPTION :
Conditional function returning true if <node> identifier is in the
<multi_range>.
==============================================================================*/
{
	int return_code;
	struct Multi_range *multi_range;

	ENTER(FE_node_is_in_Multi_range);
	if (node&&(multi_range=(struct Multi_range *)multi_range_void))
	{
		return_code=
			Multi_range_is_value_in_range(multi_range,node->getIdentifier());
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_node_is_in_Multi_range.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* FE_node_is_in_Multi_range */

int FE_node_is_not_in_Multi_range(struct FE_node *node,void *multi_range_void)
/*******************************************************************************
LAST MODIFIED : 4 July 2000

DESCRIPTION :
Conditional function returning true if <node> identifier is NOT in the
<multi_range>.
==============================================================================*/
{
	int return_code;
	struct Multi_range *multi_range;

	ENTER(FE_node_is_not_in_Multi_range);
	if (node&&(multi_range=(struct Multi_range *)multi_range_void))
	{
		return_code=
			!Multi_range_is_value_in_range(multi_range,node->getIdentifier());
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_node_is_not_in_Multi_range.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* FE_node_is_not_in_Multi_range */

int add_FE_node_number_to_Multi_range(struct FE_node *node,
	void *multi_range_void)
/*******************************************************************************
LAST MODIFIED : 20 February 2000

DESCRIPTION :
Iterator function for adding the number of <node> to <multi_range>.
==============================================================================*/
{
	int node_number,return_code;
	struct Multi_range *multi_range;

	ENTER(add_FE_node_number_to_Multi_range);
	if (node&&(multi_range=(struct Multi_range *)multi_range_void))
	{
		node_number=get_FE_node_identifier(node);
		return_code=Multi_range_add_range(multi_range,node_number,node_number);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"add_FE_node_number_to_Multi_range.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* add_FE_node_number_to_Multi_range */

int FE_nodeset_clear_embedded_locations(FE_nodeset *fe_nodeset,
	struct LIST(FE_field) *field_list, FE_mesh *hostMesh)
{
	if (!field_list || !fe_nodeset)
		return 0;
	cmzn_set_FE_field *fields = reinterpret_cast<cmzn_set_FE_field*>(field_list);
	FE_value xi[MAXIMUM_ELEMENT_XI_DIMENSIONS] = { 0.0, 0.0, 0.0 };
	for (cmzn_set_FE_field::iterator field_iter = fields->begin(); field_iter != fields->end(); ++field_iter)
	{
		FE_field *field = *field_iter;
		if ((ELEMENT_XI_VALUE == field->getValueType()) &&
			(GENERAL_FE_FIELD == field->get_FE_field_type()) &&
			((!hostMesh) || (field->getElementXiHostMesh() == hostMesh)))
		{
			cmzn_nodeiterator *node_iter = fe_nodeset->createNodeiterator();
			cmzn_node_id node = 0;
			// inefficient to loop over all nodes; set_FE_nodal_element_xi_value silently fails if field not defined
			while (0 != (node = node_iter->nextNode()))
			{
				// don't clear embedded locations on nodes now owned by a different nodeset
				// as happens when merging from a separate region
				if (node->fields->nodeset == fe_nodeset)
					set_FE_nodal_element_xi_value(node, field, /*component*/0, /*element*/nullptr, /*xi*/nullptr);
			}
			cmzn_nodeiterator_destroy(&node_iter);
		}
	}
	return 1;
}

static int FE_node_field_has_field_with_name(
	struct FE_node_field *node_field, void *field_name_void)
/*******************************************************************************
LAST MODIFIED : 14 November 2002

DESCRIPTION :
Returns true if the name of the field in <node_field> matches <field_name>.
==============================================================================*/
{
	int return_code;

	ENTER(FE_node_field_has_field_with_name);
	return_code = 0;
	if (node_field && node_field->field && field_name_void)
	{
		if (0 == strcmp(node_field->field->getName(), (char *)field_name_void))
		{
			return_code = 1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_node_field_has_field_with_name.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* FE_node_field_has_field_with_name */

static int FE_node_field_can_be_merged(struct FE_node_field *node_field,
	void *node_field_list_void)
/*******************************************************************************
LAST MODIFIED : 6 May 2003

DESCRIPTION :
Fetches a node_field with field of the same name as that in <node_field> from
<node_field_list>. Returns true if there is either no such node_field in the
list or the two node_fields are identically defined apart from the field itself.
Checks first that the FE_fields match.
==============================================================================*/
{
	int return_code;
	struct FE_node_field *other_node_field;
	struct LIST(FE_node_field) *node_field_list;

	ENTER(FE_node_field_can_be_merged);
	return_code = 0;
	if (node_field && node_field->field &&
		(node_field_list = (struct LIST(FE_node_field) *)node_field_list_void))
	{
		if (NULL != (other_node_field = FIRST_OBJECT_IN_LIST_THAT(FE_node_field)(
			FE_node_field_has_field_with_name, (void *)node_field->field->getName(),
			node_field_list)))
		{
			if (FE_fields_match_exact(node_field->field, other_node_field->field))
			{
				if (FE_node_fields_match(node_field, other_node_field,
					/*compare_field_and_time_sequence*/false, /*compare_component_value*/false))
				{
					return_code = 1;
				}
			}
		}
		else
		{
			return_code = 1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_node_field_can_be_merged.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* FE_node_field_can_be_merged */

bool FE_node_can_merge(struct FE_node *targetNode, struct FE_node *sourceNode)
{
	if (targetNode && targetNode->fields && sourceNode && sourceNode->fields)
	{
		if (FOR_EACH_OBJECT_IN_LIST(FE_node_field)(
			FE_node_field_can_be_merged,
			(void *)targetNode->fields->node_field_list,
			sourceNode->fields->node_field_list))
		{
			return true;
		}
	}
	return false;
}

PROTOTYPE_ENUMERATOR_STRING_FUNCTION(cmzn_node_value_label)
{
	switch (enumerator_value)
	{
		case CMZN_NODE_VALUE_LABEL_VALUE:
		{
			return "value";
		} break;
		case CMZN_NODE_VALUE_LABEL_D_DS1:
		{
			return "d/ds1";
		} break;
		case CMZN_NODE_VALUE_LABEL_D_DS2:
		{
			return "d/ds2";
		} break;
		case CMZN_NODE_VALUE_LABEL_D2_DS1DS2:
		{
			return "d2/ds1ds2";
		} break;
		case CMZN_NODE_VALUE_LABEL_D_DS3:
		{
			return "d/ds3";
		} break;
		case CMZN_NODE_VALUE_LABEL_D2_DS1DS3:
		{
			return "d2/ds1ds3";
		} break;
		case CMZN_NODE_VALUE_LABEL_D2_DS2DS3:
		{
			return "d2/ds2ds3";
		} break;
		case CMZN_NODE_VALUE_LABEL_D3_DS1DS2DS3:
		{
			return "d3/ds1ds2ds3";
		} break;
		case CMZN_NODE_VALUE_LABEL_INVALID:
		{
		} break;
	}
	return 0;
}

DEFINE_DEFAULT_ENUMERATOR_FUNCTIONS(cmzn_node_value_label)

template <typename VALUE_TYPE> int cmzn_node_get_field_component_values(
	cmzn_node *node, FE_field *field, int componentNumber, FE_value time,
	int valuesCount, VALUE_TYPE *valuesOut)
{
	if (!(node && node->fields && node->values_storage
		&& (0 <= componentNumber) && (componentNumber < field->getNumberOfComponents()) && (valuesOut)))
	{
		display_message(ERROR_MESSAGE, "cmzn_node_get_field_component_values<VALUE_TYPE>.  Invalid arguments");
		return CMZN_ERROR_ARGUMENT;
	}
	FE_node_field *node_field = node->getNodeField(field);
	if (!node_field)
	{
		display_message(ERROR_MESSAGE, "cmzn_node_get_field_component_values<VALUE_TYPE>.  Field %s is not defined at node %d",
			field->getName(), node->getIdentifier());
		return CMZN_ERROR_NOT_FOUND;
	}
	const FE_node_field_template &nft = *(node_field->getComponent(componentNumber));
	const int totalValuesCount = nft.getTotalValuesCount();
	if (totalValuesCount > valuesCount)
	{
		display_message(ERROR_MESSAGE, "set_FE_nodal_field_FE_value_values.  "
			"Field %s component %d at node %d has %d values, values array size %d is too small",
			field->getName(), componentNumber + 1, node->getIdentifier(), totalValuesCount, valuesCount);
		return CMZN_ERROR_ARGUMENT;
	}
	VALUE_TYPE *dest = valuesOut;
	if (node_field->time_sequence)
	{
		int time_index_one, time_index_two;
		FE_value time_xi = 0.0;
		FE_time_sequence_get_interpolation_for_time(node_field->time_sequence, time, &time_index_one,
			&time_index_two, &time_xi);
		const FE_value one_minus_time_xi = 1.0 - time_xi;
		const int valueTypeSize = get_Value_storage_size(field->getValueType(), node_field->time_sequence);
		const Value_storage *value_storage = node->values_storage + nft.getValuesOffset();
		for (int j = 0; j < totalValuesCount; ++j)
		{
			const VALUE_TYPE *source = *(const VALUE_TYPE **)(value_storage);
			if ((time_xi != 0.0) && time_index_one != time_index_two)
			{
				*dest = source[time_index_one]*one_minus_time_xi + source[time_index_two]*time_xi;
			}
			else
			{
				*dest = source[time_index_one];
			}
			value_storage += valueTypeSize;
			++dest;
		}
	}
	else
	{
		const VALUE_TYPE *source = (VALUE_TYPE *)(node->values_storage + nft.getValuesOffset());
		for (int j = 0; j < totalValuesCount; ++j)
		{
			*dest = *source;
			++source;
			++dest;
		}
	}
	return CMZN_OK;
}

template <typename VALUE_TYPE> int cmzn_node_add_field_component_values(
	cmzn_node *node, FE_field *field, int componentNumber, FE_value time,
	int valuesCount, const VALUE_TYPE *valuesIn)
{
	if (!(node && node->fields && node->values_storage
		&& (0 <= componentNumber) && (componentNumber < field->getNumberOfComponents()) && (valuesIn)))
	{
		display_message(ERROR_MESSAGE, "cmzn_node_add_field_component_values<VALUE_TYPE>.  Invalid arguments");
		return CMZN_ERROR_ARGUMENT;
	}
	FE_node_field *node_field = node->getNodeField(field);
	if (!node_field)
	{
		display_message(ERROR_MESSAGE,
			"cmzn_node_add_field_component_values<VALUE_TYPE>.  Field %s is not defined at node %d", field->getName(), node->getIdentifier());
		return CMZN_ERROR_NOT_FOUND;
	}
	const FE_node_field_template &nft = *(node_field->getComponent(componentNumber));
	const int totalValuesCount = nft.getTotalValuesCount();
	if (totalValuesCount != valuesCount)
	{
		display_message(ERROR_MESSAGE, "cmzn_node_add_field_component_values<VALUE_TYPE>.  Field %s component %d at node %d has %d values, %d are supplied",
			field->getName(), componentNumber + 1, node->getIdentifier(), totalValuesCount, valuesCount);
		return CMZN_ERROR_ARGUMENT;
	}
	const VALUE_TYPE *source = valuesIn;
	if (node_field->time_sequence)
	{
		int time_index = 0;
		if (!FE_time_sequence_get_index_for_time(node_field->time_sequence, time, &time_index))
		{
			display_message(ERROR_MESSAGE,
				"cmzn_node_add_field_component_values<VALUE_TYPE>.  Field %s does not store parameters at time %g", field->getName(), time);
			return CMZN_ERROR_ARGUMENT;
		}
		VALUE_TYPE **destArray = (VALUE_TYPE **)(node->values_storage + nft.getValuesOffset());
		for (int j = 0; j < totalValuesCount; ++j)
		{
			(*destArray)[time_index] += *source;
			++destArray;
			++source;
		}
	}
	else
	{
		VALUE_TYPE *dest = (VALUE_TYPE *)(node->values_storage + nft.getValuesOffset());
		for (int j = 0; j < totalValuesCount; ++j)
		{
			*dest += *source;
			++dest;
			++source;
		}
	}
	// notify of changes, but only for valid nodes (i.e. not template nodes)
	if (node->getIndex() >= 0)
	{
		node->fields->nodeset->nodeFieldChange(node, field);
	}
	return CMZN_OK;
}

template <typename VALUE_TYPE> int cmzn_node_set_field_component_values(
	cmzn_node *node, FE_field *field, int componentNumber, FE_value time,
	int valuesCount, const VALUE_TYPE *valuesIn)
{
	if (!(node && node->fields && node->values_storage
		&& (0 <= componentNumber) && (componentNumber < field->getNumberOfComponents()) && (valuesIn)))
	{
		display_message(ERROR_MESSAGE, "cmzn_node_set_field_component_values<VALUE_TYPE>.  Invalid arguments");
		return CMZN_ERROR_ARGUMENT;
	}
	FE_node_field *node_field = node->getNodeField(field);
	if (!node_field)
	{
		display_message(ERROR_MESSAGE,
			"cmzn_node_set_field_component_values<VALUE_TYPE>.  Field %s is not defined at node %d", field->getName(), node->getIdentifier());
		return CMZN_ERROR_NOT_FOUND;
	}
	const FE_node_field_template &nft = *(node_field->getComponent(componentNumber));
	const int totalValuesCount = nft.getTotalValuesCount();
	if (totalValuesCount != valuesCount)
	{
		display_message(ERROR_MESSAGE, "cmzn_node_set_field_component_values<VALUE_TYPE>.  Field %s component %d at node %d has %d values, %d are supplied",
			field->getName(), componentNumber + 1, node->getIdentifier(), totalValuesCount, valuesCount);
		return CMZN_ERROR_ARGUMENT;
	}
	const VALUE_TYPE *source = valuesIn;
	if (node_field->time_sequence)
	{
		int time_index = 0;
		if (!FE_time_sequence_get_index_for_time(node_field->time_sequence, time, &time_index))
		{
			display_message(ERROR_MESSAGE,
				"cmzn_node_set_field_component_values<VALUE_TYPE>.  Field %s does not store parameters at time %g", field->getName(), time);
			return CMZN_ERROR_ARGUMENT;
		}
		VALUE_TYPE **destArray = (VALUE_TYPE **)(node->values_storage + nft.getValuesOffset());
		for (int j = 0; j < totalValuesCount; ++j)
		{
			(*destArray)[time_index] = *source;
			++destArray;
			++source;
		}
	}
	else
	{
		VALUE_TYPE *dest = (VALUE_TYPE *)(node->values_storage + nft.getValuesOffset());
		for (int j = 0; j < totalValuesCount; ++j)
		{
			*dest = *source;
			++dest;
			++source;
		}
	}
	// notify of changes, but only for valid nodes (i.e. not template nodes)
	if (node->getIndex() >= 0)
	{
		node->fields->nodeset->nodeFieldChange(node, field);
	}
	return CMZN_OK;
}

int cmzn_node_get_field_component_FE_value_values(cmzn_node *node,
	FE_field *field, int componentNumber, FE_value time, int valuesCount,
	FE_value *valuesOut)
{
	if (!(field && (field->getValueType() == FE_VALUE_VALUE)))
	{
		display_message(ERROR_MESSAGE, "cmzn_node_get_field_component_FE_value_values.  Invalid arguments");
		return CMZN_ERROR_ARGUMENT;
	}
	return cmzn_node_get_field_component_values(node, field, componentNumber, time, valuesCount, valuesOut);
}

int cmzn_node_add_field_component_FE_value_values(cmzn_node *node,
	FE_field *field, int componentNumber, FE_value time, int valuesCount,
	const FE_value *valuesIn)
{
	if (!(field && (field->getValueType() == FE_VALUE_VALUE)))
	{
		display_message(ERROR_MESSAGE, "cmzn_node_add_field_component_FE_value_values.  Invalid arguments");
		return CMZN_ERROR_ARGUMENT;
	}
	return cmzn_node_add_field_component_values(node, field, componentNumber, time, valuesCount, valuesIn);
}

int cmzn_node_set_field_component_FE_value_values(cmzn_node *node,
	FE_field *field, int componentNumber, FE_value time, int valuesCount,
	const FE_value *valuesIn)
{
	if (!(field && (field->getValueType() == FE_VALUE_VALUE)))
	{
		display_message(ERROR_MESSAGE, "cmzn_node_set_field_component_FE_value_values.  Invalid arguments");
		return CMZN_ERROR_ARGUMENT;
	}
	return cmzn_node_set_field_component_values(node, field, componentNumber, time, valuesCount, valuesIn);
}

int cmzn_node_get_field_component_int_values(cmzn_node *node,
	FE_field *field, int componentNumber, FE_value time, int valuesCount,
	int *valuesOut)
{
	if (!(field && (field->getValueType() == INT_VALUE)))
	{
		display_message(ERROR_MESSAGE, "cmzn_node_get_field_component_int_values.  Invalid arguments");
		return CMZN_ERROR_ARGUMENT;
	}
	return cmzn_node_get_field_component_values(node, field, componentNumber, time, valuesCount, valuesOut);
}

int cmzn_node_set_field_component_int_values(cmzn_node *node,
	FE_field *field, int componentNumber, FE_value time, int valuesCount,
	const int *valuesIn)
{
	if (!(field && (field->getValueType() == INT_VALUE)))
	{
		display_message(ERROR_MESSAGE, "cmzn_node_set_field_component_int_values.  Invalid arguments");
		return CMZN_ERROR_ARGUMENT;
	}
	return cmzn_node_set_field_component_values(node, field, componentNumber, time, valuesCount, valuesIn);
}

int FE_field_assign_node_parameters_sparse_FE_value(FE_field *field, FE_node *node,
	int arraySize, const FE_value *values, const int *valueExists, int valuesCount,
	int componentsSize, int componentsOffset,
	int derivativesSize, int derivativesOffset,
	int versionsSize, int versionsOffset)
{
	if (!(field && node && (FE_VALUE_VALUE == field->getValueType()) &&
		(0 < arraySize) && values && valueExists && (0 < valuesCount) &&
		(componentsSize == field->getNumberOfComponents()) &&
		(componentsSize*derivativesSize*versionsSize == arraySize)))
	{
		display_message(ERROR_MESSAGE, "FE_node_assign_FE_value_parameters_sparse.  Invalid arguments");
		return CMZN_ERROR_ARGUMENT;
	}
	FE_node_field *node_field = FIND_BY_IDENTIFIER_IN_LIST(FE_node_field, field)(field, node->fields->node_field_list);
	if (!node_field)
	{
		display_message(ERROR_MESSAGE, "FE_node_assign_FE_value_parameters_sparse.  Field %s is not defined at node %d",
			field->getName(), node->getIdentifier());
		return CMZN_ERROR_NOT_FOUND;
	}
	if (node_field->time_sequence)
	{
		display_message(ERROR_MESSAGE, "FE_node_assign_FE_value_parameters_sparse.  Field %s at node %d is time-varying; case is not implemented",
			field->getName(), node->getIdentifier());
		return CMZN_ERROR_NOT_IMPLEMENTED;
	}
	if (node_field->getTotalValuesCount() != valuesCount)
	{
		display_message(ERROR_MESSAGE, "FE_node_assign_FE_value_parameters_sparse.  "
			"Number of values supplied (%d) does not match number expected by node field (%d)",
			valuesCount, node_field->getTotalValuesCount());
		return CMZN_ERROR_ARGUMENT;
	}
	int oc = 0;
	for (int c = 0; c < componentsSize; ++c)
	{
		const FE_node_field_template &nft = *(node_field->getComponent(c));
		FE_value *target = reinterpret_cast<FE_value *>(node->values_storage + nft.getValuesOffset());
		const int valueLabelsCount = nft.getValueLabelsCount();
		for (int d = 0; d < valueLabelsCount; ++d)
		{
			const cmzn_node_value_label valueLabel = nft.getValueLabelAtIndex(d);
			int ov = oc + derivativesOffset*(valueLabel - CMZN_NODE_VALUE_LABEL_VALUE);
			const int versionsCount = nft.getVersionsCountAtIndex(d);
			for (int v = 0; v < versionsCount; ++v)
			{
				if (!valueExists[ov])
				{
					display_message(ERROR_MESSAGE, "FE_node_assign_FE_value_parameters_sparse.  "
						"Field %s at node %d has different sparsity", field->getName(), node->getIdentifier());
					return CMZN_ERROR_INCOMPATIBLE_DATA;
				}
				*target = values[ov];
				++target;
				ov += versionsOffset;
			}
		}
		oc += componentsOffset;
	}
	return CMZN_OK;
}

int get_FE_node_number_of_values(struct FE_node *node)
/*******************************************************************************
LAST MODIFIED : 5 November 1998

DESCRIPTION :
Returns the number of values stored at the <node>.
==============================================================================*/
{
	int number_of_values;

	ENTER(get_FE_node_number_of_values);
	if (node&&(node->fields))
	{
		number_of_values=node->fields->number_of_values;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"get_FE_node_number_of_values.  Invalid node");
		number_of_values=0;
	}
	LEAVE;

	return (number_of_values);
} /* get_FE_node_number_of_values */

int get_FE_node_identifier(struct FE_node *node)
{
	if (node)
		return node->getIdentifier();
	return DS_LABEL_IDENTIFIER_INVALID;
}

/** If node_field is defined differently in the node, undefine it there so it
 * can be redefined. This is done because merge code cannot yet handle changing
 * the definition, but undefine + redefine works. */
static int FE_node_field_undefine_nonmatching_FE_field_at_node(FE_node_field *node_field, void *node_void)
{
	cmzn_node *node = static_cast<cmzn_node *>(node_void);
	FE_field *field = node_field->field;
	FE_node_field *existing_node_field = node->getNodeField(field);
	if (existing_node_field)
	{
		const int componentCount = field->getNumberOfComponents();
		// undefine if any component does not match
		for (int c = 0; c < componentCount; ++c)
			if (!existing_node_field->getComponent(c)->matches(*node_field->getComponent(c)))
				return (CMZN_OK == undefine_FE_field_at_node(node, field)) ? 1 : 0;
	}
	return 1;
}

int merge_FE_node(cmzn_node *destination, cmzn_node *source, int optimised_merge)
{
	int number_of_values, values_storage_size, return_code;
	struct FE_node_field_info *destination_fields, *source_fields;
	FE_nodeset *fe_nodeset;
	struct LIST(FE_node_field) *node_field_list;
	struct Merge_FE_node_field_into_list_data merge_data;
	Value_storage *values_storage;

	ENTER(merge_FE_node);
	if (destination && (destination_fields = destination->fields) &&
		(fe_nodeset = destination_fields->nodeset) &&
		source && (source_fields = source->fields) &&
		(source_fields->nodeset == fe_nodeset))
	{
		return_code = 1;
		// undefine fields on destination which are being redefined differently in merge
		if (!(FOR_EACH_OBJECT_IN_LIST(FE_node_field)(FE_node_field_undefine_nonmatching_FE_field_at_node,
			static_cast<void *>(destination), source_fields->node_field_list)))
		{
			display_message(ERROR_MESSAGE, "merge_FE_node.  Failed to undefine field(s) being redefined");
			return 0;
		}
		// this will have changed if fields were undefined:
		destination_fields = destination->fields;
		/* construct a node field list containing the fields from destination */
		node_field_list = CREATE_LIST(FE_node_field)();
		if (COPY_LIST(FE_node_field)(node_field_list,
			destination_fields->node_field_list))
		{
			/* sum the values_storage_size and number_of_values */
			number_of_values = 0;
			values_storage_size = 0;
			if (FOR_EACH_OBJECT_IN_LIST(FE_node_field)(count_nodal_size,
				(void *)(&values_storage_size), node_field_list) &&
				FOR_EACH_OBJECT_IN_LIST(FE_node_field)(count_nodal_values,
					(void *)(&number_of_values), node_field_list))
			{
				/* include the new information */
				merge_data.requires_merged_storage = 0;
				merge_data.values_storage_size = values_storage_size;
				merge_data.number_of_values = number_of_values;
				/* node field list */
				merge_data.list = node_field_list;
				if (FOR_EACH_OBJECT_IN_LIST(FE_node_field)(
					merge_FE_node_field_into_list, (void *)(&merge_data),
					source_fields->node_field_list))
				{
					if (!merge_data.requires_merged_storage)
					{
						/* Don't need to reallocate memory as we are only overwriting
							existing values */
						merge_FE_node_values_storage(destination, (Value_storage *)NULL,
							node_field_list, source, optimised_merge);
					}
					else
					{
						number_of_values = merge_data.number_of_values;
						values_storage_size = merge_data.values_storage_size;
						values_storage = (Value_storage *)NULL;
						/* allocate the new values storage and fill it with values from the
							destination and the source, favouring the latter but merging all
							time arrays */
						if ((0 == values_storage_size) ||
							(ALLOCATE(values_storage, Value_storage, values_storage_size) &&
								merge_FE_node_values_storage(destination, values_storage,
									node_field_list, source, optimised_merge)))
						{
							/* create a node field info for the combined list */
							struct FE_node_field_info *fe_node_field_info =
								fe_nodeset->get_FE_node_field_info(number_of_values, node_field_list);
							if (0 != fe_node_field_info)
							{
								/* clean up old destination values_storage */
								if (destination->values_storage)
								{
									if (destination_fields)
									{
										FOR_EACH_OBJECT_IN_LIST(FE_node_field)(
											FE_node_field_free_values_storage_arrays,
											(void *)destination->values_storage,
											destination_fields->node_field_list);
									}
									DEALLOCATE(destination->values_storage);
								}
								/* insert new fields and values_storage */
								FE_node_field_info::deaccess(destination->fields);
								destination->fields = fe_node_field_info;
								destination->values_storage = values_storage;
							}
							else
							{
								display_message(ERROR_MESSAGE,
									"merge_FE_node.  Could not get node field info");
								/* do not bother to clean up dynamic contents of values_storage */
								DEALLOCATE(values_storage);
								return_code = 0;
							}
						}
						else
						{
							display_message(ERROR_MESSAGE,
								"merge_FE_node.  Could copy values_storage");
							/* cannot clean up dynamic contents of values_storage */
							DEALLOCATE(values_storage);
							return_code = 0;
						}
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"merge_FE_node.  Error merging node field list");
					return_code = 0;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"merge_FE_node.  Error counting nodal values");
				return_code = 0;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"merge_FE_node.  Could not copy node field list");
			return_code = 0;
		}
		DESTROY(LIST(FE_node_field))(&node_field_list);
	}
	else
	{
		display_message(ERROR_MESSAGE, "merge_FE_node.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* merge_FE_node */

#if !defined (WINDOWS_DEV_FLAG)
int list_FE_node(struct FE_node *node)
{
	int return_code;

	ENTER(list_FE_node);
	if (node)
	{
		return_code=1;
		/* write the number */
		display_message(INFORMATION_MESSAGE,"node : %d\n",node->getIdentifier());
		/* write the field information */
		for_each_FE_field_at_node_alphabetical_indexer_priority(
			list_FE_node_field, (void *)NULL, node);
#if defined (DEBUG_CODE)
		/*???debug*/
		display_message(INFORMATION_MESSAGE,"  access count = %d\n",
			node->access_count);
#endif /* defined (DEBUG_CODE) */
	}
	else
	{
		display_message(ERROR_MESSAGE,"list_FE_node.  Invalid argument");
		return_code=0;
	}
	LEAVE;

	return (return_code);
}
#endif /* !defined (WINDOWS_DEV_FLAG) */

cmzn_nodeiterator_id cmzn_nodeiterator_access(cmzn_nodeiterator_id node_iterator)
{
	if (node_iterator)
		return cmzn::Access(node_iterator);
	return 0;
}

int cmzn_nodeiterator_destroy(cmzn_nodeiterator_id *node_iterator_address)
{
	if (node_iterator_address)
	{
		cmzn::Deaccess(*node_iterator_address);
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_node_id cmzn_nodeiterator_next(cmzn_nodeiterator_id node_iterator)
{
	if (node_iterator)
	{
		cmzn_node *node = node_iterator->nextNode();
		if (node)
			node->access();
		return node;
	}
	return 0;
}

cmzn_node_id cmzn_nodeiterator_next_non_access(cmzn_nodeiterator_id node_iterator)
{
	if (node_iterator)
		return node_iterator->nextNode();
	return 0;
}

int calculate_grid_field_offsets(int element_dimension,
	int top_level_element_dimension, const int *top_level_number_in_xi,
	FE_value *element_to_top_level,int *number_in_xi,int *base_grid_offset,
	int *grid_offset_in_xi)
/*******************************************************************************
LAST MODIFIED : 13 June 2000

DESCRIPTION :
Calculates the factors for converting a grid position on a element of
<element_dimension> to a top_level_element of <top_level_element_dimension>
with <top_level_number_in_xi>, given affine transformation
<element_to_top_level> which has as many rows as <top_level_element_dimension>
and 1 more column than <element_dimension>, converting xi from element to
top_level as follows:
top_level_xi = b + A xi, with b the first column.
The <number_in_xi> of the element is returned, as is the <base_grid_offset> and
the <grid_offset_in_xi> which make up the grid point number conversion:
eg. top_level_grid_point_number = base_grid_offset +
grid_offset_in_xi[i]*grid_number_in_xi[i] (i summed over element_dimension).
Sets values appropriately if element_dimension = top_level_element_dimension.
==============================================================================*/
{
	FE_value *temp_element_to_top_level;
	int i, next_offset, return_code,
		top_level_grid_offset_in_xi[MAXIMUM_ELEMENT_XI_DIMENSIONS], xi_number;

	ENTER(calculate_grid_field_offsets);
	if ((0<element_dimension)&&(element_dimension<=top_level_element_dimension)&&
		(top_level_element_dimension<=MAXIMUM_ELEMENT_XI_DIMENSIONS)&&
		top_level_number_in_xi&&((element_dimension==top_level_element_dimension)||
			element_to_top_level)&&number_in_xi&&base_grid_offset&&grid_offset_in_xi)
	{
		return_code=1;
		/* clear offsets */
		*base_grid_offset = 0;
		for (i=0;i<element_dimension;i++)
		{
			grid_offset_in_xi[i]=0;
		}
		/* calculate offset in grid_point_number for adjacent points in each xi
			 direction on the top_level_element */
		next_offset = 1;
		for (i = 0; i < top_level_element_dimension; i++)
		{
			if (top_level_number_in_xi[i] > 0)
			{
				top_level_grid_offset_in_xi[i] = next_offset;
				next_offset *= top_level_number_in_xi[i] + 1;
			}
			else
			{
				/* zero offset means linear interpolation gives same result as constant */
				top_level_grid_offset_in_xi[i] = 0;
			}
		}
		if (element_dimension == top_level_element_dimension)
		{
			for (i=0;i<top_level_element_dimension;i++)
			{
				grid_offset_in_xi[i]=top_level_grid_offset_in_xi[i];
				number_in_xi[i]=top_level_number_in_xi[i];
			}
		}
		else
		{
			temp_element_to_top_level=element_to_top_level;
			for (i=0;i<top_level_element_dimension;i++)
			{
				/* a number in the first column indicates either xi decreasing
					 or the direction this is a face/line on */
				if (*temp_element_to_top_level)
				{
					*base_grid_offset +=
						top_level_number_in_xi[i]*top_level_grid_offset_in_xi[i];
				}
				/* find out how (if at all) element xi changes with this
					 field_element xi */
				for (xi_number=0;xi_number<element_dimension;xi_number++)
				{
					if (temp_element_to_top_level[xi_number+1])
					{
						number_in_xi[xi_number] = top_level_number_in_xi[i];
						if (0<temp_element_to_top_level[xi_number+1])
						{
							grid_offset_in_xi[xi_number] = top_level_grid_offset_in_xi[i];
						}
						else
						{
							grid_offset_in_xi[xi_number] = -top_level_grid_offset_in_xi[i];
						}
					}
				}
				temp_element_to_top_level += (element_dimension+1);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"calculate_grid_field_offsets.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* calculate_grid_field_offsets */

PROTOTYPE_ACCESS_OBJECT_FUNCTION(cmzn_element)
{
	if (object)
		return object->access();
	return 0;
}

PROTOTYPE_DEACCESS_OBJECT_FUNCTION(cmzn_element)
{
	if (object_address)
		return cmzn_element::deaccess(*object_address);
	return CMZN_ERROR_ARGUMENT;
}

PROTOTYPE_REACCESS_OBJECT_FUNCTION(cmzn_element)
{
	if (object_address)
		cmzn_element::reaccess(*object_address, new_object);
	return CMZN_ERROR_ARGUMENT;
}

cmzn_elementiterator_id cmzn_elementiterator_access(cmzn_elementiterator_id element_iterator)
{
	if (element_iterator)
		return cmzn::Access(element_iterator);
	return 0;
}

int cmzn_elementiterator_destroy(cmzn_elementiterator_id *element_iterator_address)
{
	if (element_iterator_address)
	{
		cmzn::Deaccess(*element_iterator_address);
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_element_id cmzn_elementiterator_next(cmzn_elementiterator_id element_iterator)
{
	if (element_iterator)
	{
		cmzn_element *element = element_iterator->nextElement();
		if (element)
			element->access();
		return element;
	}
	return 0;
}

cmzn_element_id cmzn_elementiterator_next_non_access(cmzn_elementiterator_id element_iterator)
{
	if (element_iterator)
		return element_iterator->nextElement();
	return 0;
}

int adjacent_FE_element(struct FE_element *element,
	int face_number, int *number_of_adjacent_elements,
	struct FE_element ***adjacent_elements)
{
	int return_code = CMZN_OK;
	FE_mesh *mesh;
	if (element && (mesh = element->getMesh()))
	{
		int j = 0;
		FE_mesh *faceMesh = mesh->getFaceMesh();
		const DsLabelIndex elementIndex = element->getIndex();
		DsLabelIndex faceIndex;
		if ((faceMesh) && (0 <= (faceIndex = mesh->getElementFace(elementIndex, face_number))))
		{
			const DsLabelIndex *parents;
			const int parentsCount = faceMesh->getElementParents(faceIndex, parents);
			if (ALLOCATE(*adjacent_elements, struct FE_element *, parentsCount))
			{
				for (int p = 0; p < parentsCount; ++p)
					if (parents[p] != elementIndex)
					{
						(*adjacent_elements)[j] = mesh->getElement(parents[p]);
						++j;
					}
			}
			else
			{
				display_message(ERROR_MESSAGE, "adjacent_FE_element.  Unable to allocate array");
				return_code = CMZN_ERROR_MEMORY;
			}
		}
		*number_of_adjacent_elements = j;
	}
	else
	{
		display_message(ERROR_MESSAGE, "adjacent_FE_element.  Invalid argument(s)");
		return_code = CMZN_ERROR_ARGUMENT;
	}
	return return_code;
}

int calculate_FE_element_field_nodes(struct FE_element *element,
	int face_number, struct FE_field *field,
	int *number_of_element_field_nodes_address,
	struct FE_node ***element_field_nodes_array_address,
	struct FE_element *top_level_element)
{
	if (!((element) && (number_of_element_field_nodes_address) &&
		(element_field_nodes_array_address)))
	{
		display_message(ERROR_MESSAGE, "calculate_FE_element_field_nodes.  Invalid argument(s)");
		return CMZN_ERROR_ARGUMENT;
	}
	if (!field)
	{
		field = FE_element_get_default_coordinate_field(top_level_element ? top_level_element : element);
		if (!field)
		{
			display_message(ERROR_MESSAGE, "calculate_FE_element_field_nodes.  No default coordinate field");
			return CMZN_ERROR_ARGUMENT;
		}
	}
	// retrieve the field element from which this element inherits the field
	// and calculate the affine transformation from the element xi coordinates
	// to the xi coordinates for the field element */
	FE_value coordinate_transformation[MAXIMUM_ELEMENT_XI_DIMENSIONS*MAXIMUM_ELEMENT_XI_DIMENSIONS];
	cmzn_element *fieldElement = field->getOrInheritOnElement(element,
		face_number, top_level_element, coordinate_transformation);
	if (!fieldElement)
	{
		if (field)
		{
			display_message(ERROR_MESSAGE,
				"calculate_FE_element_field_nodes.  %s not defined for %d-D element %d",
				field->getName(), element->getDimension(), element->getIdentifier());
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"calculate_FE_element_field_nodes.  No coordinate fields defined for %d-D element %d",
				element->getDimension(), element->getIdentifier());
		}
		return CMZN_ERROR_NOT_FOUND;
	}
	const FE_mesh *mesh = fieldElement->getMesh();
	if (!mesh)
	{
		display_message(ERROR_MESSAGE, "calculate_FE_element_field_nodes.  Invalid element");
		return CMZN_ERROR_NOT_FOUND;
	}
	const FE_nodeset *nodeset = mesh->getNodeset();
	if (!nodeset)
	{
		display_message(ERROR_MESSAGE, "calculate_FE_element_field_nodes.  No nodeset, invalid mesh");
		return CMZN_ERROR_NOT_FOUND;
	}

	int return_code = CMZN_OK;
	FE_value *blending_matrix, *combined_blending_matrix, *transformation;
	int i, *inherited_basis_arguments, j, k,
		number_of_inherited_values, number_of_blended_values;

	int number_of_element_field_nodes = 0;
	struct FE_node **element_field_nodes_array = 0;
	int elementDimension = element->getDimension();
	if (face_number >= 0)
		--elementDimension;
	const int fieldElementDimension = fieldElement->getDimension();
	const FE_mesh_field_data *meshFieldData = field->getMeshFieldData(mesh);
	const int number_of_components = field->getNumberOfComponents();
	struct FE_basis *previous_basis = 0;
	int previous_number_of_element_values = -1;
	DsLabelIndex *element_value, *element_values = 0;
	DsLabelIndex *previous_element_values = 0;
	const DsLabelIndex fieldElementIndex = fieldElement->getIndex();
	for (int component_number = 0; (component_number < number_of_components) && (return_code == CMZN_OK); ++component_number)
	{
		const FE_element_field_template *componentEFT = meshFieldData->getComponentMeshfieldtemplate(component_number)->getElementfieldtemplate(fieldElementIndex);
		if (componentEFT->getParameterMappingMode() == CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE)
		{
			int number_of_element_values = 0;
			return_code = global_to_element_map_nodes(field, component_number,
				componentEFT, fieldElement, number_of_element_values, element_values);
			if (CMZN_OK != return_code)
			{
				if (CMZN_ERROR_NOT_FOUND != return_code)
				{
					display_message(ERROR_MESSAGE, "calculate_FE_element_field_nodes.  Failed to map element nodes");
				}
				break;
			}
			if (0 == number_of_element_values)
			{
				continue;
			}
			FE_basis *basis = componentEFT->getBasis();
			if ((i=number_of_element_values)==
				previous_number_of_element_values)
			{
				i--;
				while ((i>=0)&&(element_values[i]==previous_element_values[i]))
				{
					i--;
				}
			}
			if ((i>=0)||(basis!=previous_basis))
			{
				DEALLOCATE(previous_element_values);
				previous_element_values=element_values;
				previous_number_of_element_values=number_of_element_values;
				previous_basis=basis;
				Standard_basis_function *standard_basis_function;
				if (fieldElementDimension == elementDimension)
				{
					blending_matrix = 0;
					inherited_basis_arguments = 0;
				}
				else if (calculate_standard_basis_transformation(basis,
					coordinate_transformation, elementDimension,
					&inherited_basis_arguments, &number_of_inherited_values,
					&standard_basis_function, &blending_matrix))
				{
					number_of_blended_values = FE_basis_get_number_of_blended_functions(basis);
					if (number_of_blended_values > 0)
					{
						combined_blending_matrix = FE_basis_calculate_combined_blending_matrix(basis,
							number_of_blended_values, number_of_inherited_values, blending_matrix);
						DEALLOCATE(blending_matrix);
						blending_matrix = combined_blending_matrix;
						if (!blending_matrix)
						{
							display_message(ERROR_MESSAGE,
								"calculate_FE_element_field_nodes.  Could not allocate combined_blending_matrix");
							return_code = CMZN_ERROR_MEMORY;
						}
					}
				}
				else
				{
					return_code = CMZN_ERROR_GENERAL;
				}
				if (CMZN_OK == return_code)
				{
					transformation=blending_matrix;
					element_value=element_values;
					i=number_of_element_values;
					while ((i > 0) && (CMZN_OK == return_code))
					{
						bool add;
						if (transformation)
						{
							add = false;
							j = number_of_inherited_values;
							while (!add&&(j>0))
							{
								if (1.e-8<fabs(*transformation))
								{
									add = true;
								}
								transformation++;
								j--;
							}
							transformation += j;
						}
						else
						{
							add = true;
						}
						if (add)
						{
							struct FE_node *node = nodeset->getNode(*element_value);
							if (node)
							{
								k = 0;
								while ((k<number_of_element_field_nodes)&&
									(node != element_field_nodes_array[k]))
								{
									k++;
								}
								if (k>=number_of_element_field_nodes)
								{
									struct FE_node **temp_element_field_nodes_array;
									if (REALLOCATE(temp_element_field_nodes_array,
										element_field_nodes_array, struct FE_node *,
										number_of_element_field_nodes+1))
									{
										element_field_nodes_array = temp_element_field_nodes_array;
										element_field_nodes_array[number_of_element_field_nodes] = node->access();
										number_of_element_field_nodes++;
									}
									else
									{
										display_message(ERROR_MESSAGE,
											"calculate_FE_element_field_nodes.  "
											"Could not REALLOCATE element_field_nodes_array");
										return_code = CMZN_ERROR_MEMORY;
									}
								}
							}
						}
						element_value++;
						i--;
					}
				}
				if (blending_matrix)
					DEALLOCATE(blending_matrix);
				if (inherited_basis_arguments)
					DEALLOCATE(inherited_basis_arguments);
			}
			else
			{
				DEALLOCATE(element_values);
			}
		}
	}
	DEALLOCATE(previous_element_values);
	if (CMZN_OK == return_code)
	{
		*number_of_element_field_nodes_address=number_of_element_field_nodes;
		if (number_of_element_field_nodes)
		{
			*element_field_nodes_array_address = element_field_nodes_array;
		}
		else
		{
			DEALLOCATE(element_field_nodes_array);
			*element_field_nodes_array_address = 0;
			return_code = CMZN_ERROR_NOT_FOUND;
		}
	}
	else
	{
		*number_of_element_field_nodes_address = 0;
		*element_field_nodes_array_address = 0;
		for (i=0;i<number_of_element_field_nodes;i++)
		{
			cmzn_node::deaccess(element_field_nodes_array[i]);
		}
		DEALLOCATE(element_field_nodes_array);
	}
	return (return_code);
}

bool equivalent_FE_field_in_elements(struct FE_field *field,
	struct FE_element *element_1, struct FE_element *element_2)
{
	if (!(field && element_1 && element_2))
		return false;
	FE_mesh *mesh = element_1->getMesh();
	if ((element_2->getMesh() != mesh) || (!mesh))
		return false;
	const FE_mesh_field_data *meshFieldData = field->getMeshFieldData(mesh);
	if (!meshFieldData)
		return true; // field not defined on any elements in mesh, hence equivalent
	const int componentCount = field->getNumberOfComponents();
	for (int c = 0; c < componentCount; ++c)
	{
		const FE_mesh_field_template *mft = meshFieldData->getComponentMeshfieldtemplate(c);
		if (mft->getElementEFTIndex(element_1->getIndex()) != mft->getElementEFTIndex(element_2->getIndex()))
			return false;
	}
	return true;
}

bool equivalent_FE_fields_in_elements(struct FE_element *element_1,
	struct FE_element *element_2)
{
	if (!(element_1 && element_2))
		return false;
	FE_mesh *mesh = element_1->getMesh();
	if ((element_2->getMesh() != mesh) || (!mesh))
		return false;
	return mesh->equivalentFieldsInElements(element_1->getIndex(), element_2->getIndex());
}

int get_FE_element_dimension(struct FE_element *element)
{
	if (element)
		return element->getDimension();
	display_message(ERROR_MESSAGE, "get_FE_element_dimension.  Invalid element");
	return 0;
}

DsLabelIdentifier get_FE_element_identifier(struct FE_element *element)
{
	if (element)
		return element->getIdentifier();
	return DS_LABEL_IDENTIFIER_INVALID;
}

DsLabelIndex get_FE_element_index(struct FE_element *element)
{
	if (element)
		return element->getIndex();
	return DS_LABEL_INDEX_INVALID;
}

FE_field *FE_element_get_default_coordinate_field(struct FE_element *element)
{
	if (!(element && element->getMesh()))
		return 0;
	CMZN_SET(FE_field) *fields = reinterpret_cast<CMZN_SET(FE_field) *>(element->getMesh()->get_FE_region()->fe_field_list);
	for (CMZN_SET(FE_field)::iterator iter = fields->begin(); iter != fields->end(); ++iter)
	{
		FE_field *field = *iter;
		if (field->isTypeCoordinate() && FE_field_is_defined_in_element(field, element))
			return field;
	}
	return 0;
}

int get_FE_element_number_of_fields(struct FE_element *element)
{
	int fieldCount = 0;
	const FE_mesh *mesh;
	if ((element) && (mesh = element->getMesh()))
	{
		const DsLabelIndex elementIndex = element->getIndex();
		CMZN_SET(FE_field) *fields = reinterpret_cast<CMZN_SET(FE_field) *>(element->getMesh()->get_FE_region()->fe_field_list);
		for (CMZN_SET(FE_field)::iterator iter = fields->begin(); iter != fields->end(); ++iter)
		{
			const FE_mesh_field_data *meshFieldData = (*iter)->getMeshFieldData(mesh);
			if ((meshFieldData) && (meshFieldData->getComponentMeshfieldtemplate(0)->getElementEFTIndex(elementIndex) >= 0))
				++fieldCount;
		}
	}
	return fieldCount;
}

FE_element_shape *get_FE_element_shape(struct FE_element *element)
{
	if (element)
		return element->getElementShape();
	display_message(ERROR_MESSAGE,"get_FE_element_shape.  Invalid element");
	return 0;
}

struct FE_element *get_FE_element_face(struct FE_element *element, int face_number)
{
	const FE_mesh *mesh;
	if (element && (mesh = element->getMesh()))
	{
		const FE_mesh::ElementShapeFaces *shapeFaces = mesh->getElementShapeFacesConst(element->getIndex());
		FE_mesh *faceMesh = mesh->getFaceMesh();
		if ((shapeFaces) && (0 <= face_number) && (face_number < shapeFaces->getFaceCount()) && (faceMesh))
		{
			const DsLabelIndex *faces = shapeFaces->getElementFaces(element->getIndex());
			if (faces)
				return faceMesh->getElement(faces[face_number]);
			return 0;
		}
	}
	display_message(ERROR_MESSAGE, "get_FE_element_face.  Invalid argument(s)");
	return 0;
}

int for_each_FE_field_at_element_alphabetical_indexer_priority(
	FE_element_field_iterator_function *iterator,void *user_data,
	struct FE_element *element)
{
	int i, number_of_fields, return_code;
	struct FE_field *field;
	struct FE_field_order_info *field_order_info;
	struct FE_region *fe_region;

	ENTER(for_each_FE_field_at_element_alphabetical_indexer_priority);
	return_code = 0;
	if (iterator && element && element->getMesh())
	{
		// get list of all fields in default alphabetical order
		field_order_info = CREATE(FE_field_order_info)();
		fe_region = element->getMesh()->get_FE_region();
		return_code = FE_region_for_each_FE_field(fe_region,
			FE_field_add_to_FE_field_order_info, (void *)field_order_info);
		FE_field_order_info_prioritise_indexer_fields(field_order_info);
		number_of_fields = get_FE_field_order_info_number_of_fields(field_order_info);
		for (i = 0; i < number_of_fields; i++)
		{
			field = get_FE_field_order_info_field(field_order_info, i);
			if (FE_field_is_defined_in_element_not_inherited(field, element))
				return_code = (iterator)(element, field, user_data);
		}
		DESTROY(FE_field_order_info)(&field_order_info);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"for_each_FE_field_at_element_alphabetical_indexer_priority.  "
			"Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
}

int FE_element_number_is_in_Multi_range(struct FE_element *element,
	void *multi_range_void)
{
	int return_code;
	struct Multi_range *multi_range = (struct Multi_range *)multi_range_void;
	if (element && multi_range)
	{
		return_code = Multi_range_is_value_in_range(multi_range,
			element->getIdentifier());
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_element_number_is_in_Multi_range.  Invalid argument(s)");
		return_code = 0;
	}
	return (return_code);
}

int FE_element_add_number_to_Multi_range(
	struct FE_element *element, void *multi_range_void)
{
	int return_code;
	struct Multi_range *multi_range = (struct Multi_range *)multi_range_void;
	if (element && multi_range)
	{
		const DsLabelIdentifier identifier = element->getIdentifier();
		return_code = Multi_range_add_range(multi_range, identifier, identifier);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_element_add_number_to_Multi_range.   Invalid argument(s)");
		return_code = 0;
	}
	return (return_code);
}

bool FE_element_is_top_level_parent_of_element(
	struct FE_element *element, struct FE_element *other_element)
{
	if (element && element->getMesh() && other_element && other_element->getMesh())
	{
		if ((element->getMesh()->getElementParentsCount(element->getIndex()) == 0)
			&& (element->getMesh()->isElementAncestor(element->getIndex(),
				other_element->getMesh(), other_element->getIndex())))
			return true;
	}
	else
		display_message(ERROR_MESSAGE, "FE_element_is_top_level_parent_of_element.  Invalid argument(s)");
	return false;
}

struct FE_element *FE_element_get_top_level_element_conversion(
	struct FE_element *element,struct FE_element *check_top_level_element,
	cmzn_element_face_type specified_face, FE_value *element_to_top_level)
{
	struct FE_element *top_level_element;
	if (element && element->getMesh() && element_to_top_level)
	{
		FE_mesh *parentMesh = element->getMesh()->getParentMesh();
		const DsLabelIndex *parents;
		int parentsCount;
		if ((!parentMesh) || (0 == (parentsCount = element->getMesh()->getElementParents(element->getIndex(), parents))))
		{
			/* no parents */
			top_level_element = element;
		}
		else
		{
			DsLabelIndex parentIndex = DS_LABEL_INDEX_INVALID;
			if (check_top_level_element)
			{
				FE_mesh *topLevelMesh = check_top_level_element->getMesh();
				const DsLabelIndex checkTopLevelElementIndex = check_top_level_element->getIndex();
				for (int p = 0; p < parentsCount; ++p)
				{
					if (((parentMesh == topLevelMesh) && (parents[p] == checkTopLevelElementIndex)) ||
						topLevelMesh->isElementAncestor(checkTopLevelElementIndex, parentMesh, parents[p]))
					{
						parentIndex = parents[p];
						break;
					}
				}
			}
			if ((parentIndex < 0) && (CMZN_ELEMENT_FACE_TYPE_XI1_0 <= specified_face))
				parentIndex = element->getMesh()->getElementParentOnFace(element->getIndex(), specified_face);
			if (parentIndex < 0)
				parentIndex = parents[0];
			FE_element *parent = parentMesh->getElement(parentIndex);

			int face_number;
			FE_element_shape *parent_shape = parentMesh->getElementShape(parentIndex);
			const FE_value *face_to_element = 0;
			if ((parent_shape) &&
				(0 <= (face_number = parentMesh->getElementFaceNumber(parentIndex, element->getIndex()))) &&
				(top_level_element = FE_element_get_top_level_element_conversion(
					parent, check_top_level_element, specified_face, element_to_top_level)) &&
				(face_to_element = get_FE_element_shape_face_to_element(parent_shape, face_number)))
			{
				const int size = top_level_element->getDimension();
				if (parent == top_level_element)
				{
					/* element_to_top_level = face_to_element of appropriate face */
					for (int i = size*size - 1; 0 <= i; --i)
					{
						element_to_top_level[i] = face_to_element[i];
					}
				}
				else
				{
					/* multiply face_to_element of top_level_element (currently in
						 element_to_top_level) by face_to_element of parent */
					/* this is the 1:3 case */
					for (int i = 0; i < size; ++i)
					{
						element_to_top_level[i*2  ] = element_to_top_level[i*size] +
							element_to_top_level[i*size+1]*face_to_element[0]+
							element_to_top_level[i*size+2]*face_to_element[2];
						element_to_top_level[i*2+1] =
							element_to_top_level[i*size+1]*face_to_element[1]+
							element_to_top_level[i*size+2]*face_to_element[3];
					}
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"FE_element_get_top_level_element_conversion.  Invalid parent");
				top_level_element=(struct FE_element *)NULL;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_element_get_top_level_element_conversion.  Invalid argument(s)");
		top_level_element=(struct FE_element *)NULL;
	}
	return (top_level_element);
}

int FE_element_get_top_level_element_and_xi(struct FE_element *element,
	const FE_value *xi, int element_dimension,
	struct FE_element **top_level_element, FE_value *top_level_xi,
	int *top_level_element_dimension)
/*******************************************************************************
LAST MODIFIED : 7 May 2003

DESCRIPTION :
Finds the <top_level_element>, <top_level_xi> and <top_level_element_dimension>
for the given <element> and <xi>.  If <top_level_element> is already set it
is checked and the <top_level_xi> calculated.
==============================================================================*/
{
	FE_value element_to_top_level[9];
	int i,j,k,return_code;

	ENTER(FE_element_get_top_level_element_and_xi);
	if (element && element->getMesh() && xi && top_level_element && top_level_xi &&
		top_level_element_dimension)
	{
		return_code = 1;
		if (element->getMesh()->getElementParentsCount(element->getIndex()) == 0)
		{
			*top_level_element = element;
			for (i=0;i<element_dimension;i++)
			{
				top_level_xi[i]=xi[i];
			}
			/* do not set element_to_top_level */
			*top_level_element_dimension=element_dimension;
		}
		else
		{
			/* check or get top_level element and xi coordinates for it */
			if (NULL != (*top_level_element = FE_element_get_top_level_element_conversion(
				element,*top_level_element,
				CMZN_ELEMENT_FACE_TYPE_ALL, element_to_top_level)))
			{
				/* convert xi to top_level_xi */
				*top_level_element_dimension = (*top_level_element)->getDimension();
				for (j=0;j<*top_level_element_dimension;j++)
				{
					top_level_xi[j] = element_to_top_level[j*(element_dimension+1)];
					for (k=0;k<element_dimension;k++)
					{
						top_level_xi[j] +=
							element_to_top_level[j*(element_dimension+1)+k+1]*xi[k];
					}
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"FE_element_get_top_level_element_and_xi.  "
					"No top-level element found to evaluate on");
				return_code=0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_element_get_top_level_element_and_xi.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* FE_element_get_top_level_element_and_xi */

int get_FE_element_discretization_from_top_level(struct FE_element *element,
	int *number_in_xi,struct FE_element *top_level_element,
	int *top_level_number_in_xi,FE_value *element_to_top_level)
{
	int dimension,i,j,maximum_number_in_xi,return_code,top_level_dimension;

	ENTER(get_FE_element_discretization_from_top_level);
	if (element&&number_in_xi&&top_level_element&&top_level_number_in_xi)
	{
		return_code=1;
		dimension = element->getDimension();
		if (top_level_element==element)
		{
			for (i=0;i<dimension;i++)
			{
				number_in_xi[i]=top_level_number_in_xi[i];
			}
		}
		else if (element_to_top_level)
		{
			top_level_dimension = top_level_element->getDimension();
			/* use largest number_in_xi of any linked xi directions */
			for (i=0;(i<dimension)&&return_code;i++)
			{
				maximum_number_in_xi=0;
				number_in_xi[i]=0;
				for (j=0;j<top_level_dimension;j++)
				{
					if (0.0!=fabs(element_to_top_level[j*(dimension+1)+i+1]))
					{
						if (top_level_number_in_xi[j]>maximum_number_in_xi)
						{
							maximum_number_in_xi=top_level_number_in_xi[j];
						}
					}
				}
				number_in_xi[i] = maximum_number_in_xi;
				if (0==maximum_number_in_xi)
				{
					display_message(ERROR_MESSAGE,
						"get_FE_element_discretization_from_top_level.  "
						"Could not get discretization");
					return_code=0;
				}
			}
			for (i=dimension;i<MAXIMUM_ELEMENT_XI_DIMENSIONS;i++)
			{
				number_in_xi[i]=0;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"get_FE_element_discretization_from_top_level.  "
				"Missing element_to_top_level matrix");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"get_FE_element_discretization_from_top_level.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* get_FE_element_discretization_from_top_level */

int get_FE_element_discretization(struct FE_element *element,
	cmzn_element_face_type face, struct FE_field *native_discretization_field,
	int *top_level_number_in_xi,struct FE_element **top_level_element,
	int *number_in_xi)
{
	FE_value element_to_top_level[9];
	int return_code;

	ENTER(get_FE_element_discretization);
	if (element&&top_level_number_in_xi&&top_level_element&&number_in_xi)
	{
		if (NULL != (*top_level_element = FE_element_get_top_level_element_conversion(
			element, *top_level_element, face, element_to_top_level)))
		{
			/* get the discretization requested for top-level element, from native
				 discretization field if not NULL and is element based in element */
			if (native_discretization_field)
			{
				FE_mesh *mesh = (*top_level_element)->getMesh();
				const int topDimension = mesh->getDimension();
				const FE_mesh_field_data *meshFieldData = native_discretization_field->getMeshFieldData(mesh);
				if (meshFieldData)
				{
					// use first grid-based field component
					for (int c = 0; c < native_discretization_field->getNumberOfComponents(); ++c)
					{
						const FE_mesh_field_template *mft = meshFieldData->getComponentMeshfieldtemplate(c);
						const FE_element_field_template *eft = mft->getElementfieldtemplate((*top_level_element)->getIndex());
						const int *gridNumberInXi = eft->getLegacyGridNumberInXi();
						if (gridNumberInXi) // only if legacy grid; other element-based do not multiply
						{
							for (int d = 0; d < topDimension; ++d)
								top_level_number_in_xi[d] *= gridNumberInXi[d];
							break;
						}
					}
				}
			}
			if (get_FE_element_discretization_from_top_level(element,number_in_xi,
				*top_level_element,top_level_number_in_xi,element_to_top_level))
			{
				return_code=1;
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"get_FE_element_discretization.  Error getting discretization");
				return_code=0;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,"get_FE_element_discretization.  "
				"Error getting top_level_element");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"get_FE_element_discretization.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* get_FE_element_discretization */

bool FE_element_is_exterior_face_with_inward_normal(struct FE_element *element)
{
	if (element && element->getMesh())
	{
		FE_mesh *parentMesh = element->getMesh()->getParentMesh();
		const DsLabelIndex *parents;
		if ((parentMesh) && (parentMesh->getDimension() == 3) &&
			(1 == element->getMesh()->getElementParents(element->getIndex(), parents)))
		{
			if (FE_element_shape_face_has_inward_normal(parentMesh->getElementShape(parents[0]),
					parentMesh->getElementFaceNumber(parents[0], element->getIndex())))
				return true;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_element_is_exterior_face_with_inward_normal.  Invalid argument(s)");
	}
	return false;
}

int cmzn_element_add_nodes_to_labels_group(cmzn_element *element, DsLabelsGroup &nodeLabelsGroup)
{
	if (!(element && element->getMesh()))
		return CMZN_ERROR_ARGUMENT;
	if (!element->getMesh()->addElementNodesToGroup(element->getIndex(), nodeLabelsGroup))
		return CMZN_ERROR_GENERAL;
	int return_code = CMZN_OK;
	if (element->getMesh()->getElementParentsCount(element->getIndex()) > 0)
	{
		int number_of_element_field_nodes;
		cmzn_node_id *element_field_nodes_array;
		return_code = calculate_FE_element_field_nodes(element, /*face_number*/-1, (struct FE_field *)NULL,
			&number_of_element_field_nodes, &element_field_nodes_array,
			/*top_level_element*/(struct FE_element *)NULL);
		if (return_code == CMZN_OK)
		{
			for (int i = 0; i < number_of_element_field_nodes; i++)
			{
				FE_node *node = element_field_nodes_array[i];
				if (node && (node->getIndex() >= 0))
				{
					const int result = nodeLabelsGroup.setIndex(node->getIndex(), true);
					if ((result != CMZN_OK) && (result != CMZN_ERROR_ALREADY_EXISTS))
						return_code = result; // don't break as need to deaccess nodes
				}
				cmzn_node_destroy(&node);
			}
			DEALLOCATE(element_field_nodes_array);
		}
		else if (return_code == CMZN_ERROR_NOT_FOUND)
		{
			return_code = CMZN_OK;
		}
	}
	return return_code;
}

int cmzn_element_remove_nodes_from_labels_group(cmzn_element *element, DsLabelsGroup &nodeLabelsGroup)
{
	if (!(element && element->getMesh()))
		return CMZN_ERROR_ARGUMENT;
	if (!element->getMesh()->removeElementNodesFromGroup(element->getIndex(), nodeLabelsGroup))
		return CMZN_ERROR_GENERAL;
	int return_code = CMZN_OK;
	if (element->getMesh()->getElementParentsCount(element->getIndex()) > 0)
	{
		int number_of_element_field_nodes;
		cmzn_node_id *element_field_nodes_array;
		return_code = calculate_FE_element_field_nodes(element, /*face_number*/-1, (struct FE_field *)NULL,
			&number_of_element_field_nodes, &element_field_nodes_array,
			/*top_level_element*/(struct FE_element *)NULL);
		if (return_code == CMZN_OK)
		{
			for (int i = 0; i < number_of_element_field_nodes; i++)
			{
				FE_node *node = element_field_nodes_array[i];
				if (node && (node->getIndex() >= 0))
				{
					const int result = nodeLabelsGroup.setIndex(node->getIndex(), false);
					if ((result != CMZN_OK) && (result != CMZN_ERROR_NOT_FOUND))
						return_code = result; // don't break as need to deaccess nodes
				}
				cmzn_node_destroy(&node);
			}
			DEALLOCATE(element_field_nodes_array);
		}
		else if (return_code == CMZN_ERROR_NOT_FOUND)
		{
			return_code = CMZN_OK;
		}
	}
	return return_code;
}

int FE_element_is_top_level(struct FE_element *element,void *dummy_void)
{
	USE_PARAMETER(dummy_void);
	if (element && (element->getMesh()))
		return (0 == element->getMesh()->getElementParentsCount(element->getIndex()));
	return 0;
}

bool FE_field_is_defined_in_element(struct FE_field *field,
	struct FE_element *element)
{
	const FE_mesh *mesh;
	if (!((field) && (element) && (mesh = element->getMesh())))
	{
		display_message(ERROR_MESSAGE, "FE_field_is_defined_in_element.  Invalid argument(s)");
		return false;
	}
	FE_mesh_field_data *meshFieldData = field->getMeshFieldData(mesh);
	if (meshFieldData)
	{
		// check only first component
		const FE_mesh_field_template *mft = meshFieldData->getComponentMeshfieldtemplate(0);
		if (mft->getElementEFTIndex(element->getIndex()) >= 0)
			return true;
	}
	const FE_mesh *parentMesh = mesh->getParentMesh();
	if (parentMesh)
	{
		const DsLabelIndex *parents;
		const int parentsCount = mesh->getElementParents(element->getIndex(), parents);
		for (int p = 0; p < parentsCount; ++p)
			if (FE_field_is_defined_in_element(field, parentMesh->getElement(parents[p])))
				return true;
	}
	return false;
}

bool FE_field_is_defined_in_element_not_inherited(struct FE_field *field,
	struct FE_element *element)
{
	const FE_mesh *mesh;
	if (!((field) && (element) && (mesh = element->getMesh())))
		return false;
	FE_mesh_field_data *meshFieldData = field->getMeshFieldData(mesh);
	if (!meshFieldData)
		return false; // not defined on any elements of mesh
	// check only first component
	const FE_mesh_field_template *mft = meshFieldData->getComponentMeshfieldtemplate(0);
	return mft->getElementEFTIndex(element->getIndex()) >= 0;
}

bool FE_element_field_is_grid_based(struct FE_element *element,
	struct FE_field *field)
{
	const FE_mesh *mesh;
	if (!((field) && (element) && (mesh = element->getMesh())))
	{
		display_message(ERROR_MESSAGE, "FE_element_field_is_grid_based.  Invalid argument(s)");
		return false;
	}
	FE_mesh_field_data *meshFieldData = field->getMeshFieldData(mesh);
	if (!meshFieldData)
		return false; // not defined on any elements of mesh
	for (int c = 0; c < field->getNumberOfComponents(); ++c)
	{
		const FE_mesh_field_template *mft = meshFieldData->getComponentMeshfieldtemplate(c);
		const FE_element_field_template *eft = mft->getElementfieldtemplate(element->getIndex());
		if (!eft)
			return false;
		if ((eft->getNumberOfElementDOFs() > 0) && (0 != eft->getLegacyGridNumberInXi()))
			return true;
	}
	return false;
}

bool FE_element_has_grid_based_fields(struct FE_element *element)
{
	if (!(element && element->getMesh()))
	{
		display_message(ERROR_MESSAGE, "FE_element_has_grid_based_fields.  Invalid argument(s)");
		return false;
	}
	CMZN_SET(FE_field) *fields = reinterpret_cast<CMZN_SET(FE_field) *>(element->getMesh()->get_FE_region()->fe_field_list);
	for (CMZN_SET(FE_field)::iterator iter = fields->begin(); iter != fields->end(); ++iter)
	{
		if (FE_element_field_is_grid_based(element, *iter))
			return true;
	}
	return false;
}

int FE_node_smooth_FE_field(struct FE_node *node, struct FE_field *fe_field,
	FE_value time, struct FE_field *node_accumulate_fe_field,
	struct FE_field *element_count_fe_field)
{
	const cmzn_node_value_label firstDerivativeValueLabels[3] =
	{
		CMZN_NODE_VALUE_LABEL_D_DS1, CMZN_NODE_VALUE_LABEL_D_DS2, CMZN_NODE_VALUE_LABEL_D_DS3
	};
	int return_code;
	if (node && fe_field && (fe_field->getValueType() == FE_VALUE_VALUE)
		&& node_accumulate_fe_field && (node_accumulate_fe_field->getValueType() == FE_VALUE_VALUE)
		&& element_count_fe_field && (element_count_fe_field->getValueType() == INT_VALUE))
	{
		const FE_node_field *node_field = node->getNodeField(fe_field);
		FE_value value;
		int count;
		return_code = 1;
		const int componentCount = fe_field->getNumberOfComponents();
		for (int c = 0; (c < componentCount) && return_code; ++c)
		{
			const FE_node_field_template &nft = *(node_field->getComponent(c));
			for (int d = 0; (d < 3) && return_code; ++d)
			{
				const cmzn_node_value_label valueLabel = firstDerivativeValueLabels[d];
				const int versionsCount = nft.getValueNumberOfVersions(valueLabel);
				for (int v = 0; v < versionsCount; ++v)
				{
					if (cmzn_node_get_field_parameters(node, node_accumulate_fe_field, c, valueLabel, v, time, &value) &&
						cmzn_node_get_field_parameters(node, element_count_fe_field, c, valueLabel, v, time, &count))
					{
						if (0 < count)
						{
							const FE_value newValue = value/count;
							if (!cmzn_node_set_field_parameters(node, fe_field, c, valueLabel, v, time, &newValue))
							{
								return_code = 0;
								break;
							}
						}
					}
					else
					{
						return_code = 0;
						break;
					}
				}
			}
		}
		undefine_FE_field_at_node(node, node_accumulate_fe_field);
		undefine_FE_field_at_node(node, element_count_fe_field);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_node_smooth_FE_field.  Invalid argument(s)");
		return_code = 0;
	}
	return (return_code);
}

/**
 * Used by FE_element_smooth_FE_field.
 * Adds <delta> to the identified quantity in <fe_field> and increments the
 * integer counter for the corresponding quantity in <count_fe_field>.
 */
static int FE_node_field_component_accumulate_value(struct FE_node *node,
	struct FE_field *fe_field, struct FE_field *count_fe_field,
	int componentNumber, cmzn_node_value_label valueLabel, int version,
	FE_value time, FE_value delta)
{
	if (!(node && fe_field && count_fe_field && (0 <= componentNumber) &&
		(componentNumber <= get_FE_field_number_of_components(fe_field)) &&
		(0 <= version)))
	{
		display_message(ERROR_MESSAGE, "FE_node_field_component_accumulate_value.  Invalid argument(s)");
		return 0;
	}
	FE_value value;
	if (cmzn_node_get_field_parameters(node, fe_field, componentNumber, valueLabel, version, time, &value))
	{
		const FE_value newValue = value + delta;
		if (!(cmzn_node_set_field_parameters(node, fe_field, componentNumber, valueLabel, version, time, &newValue)))
		{
			display_message(ERROR_MESSAGE, "FE_node_field_component_accumulate_value.  Failed to set field value");
			return 0;
		}
		int int_value;
		if (!cmzn_node_get_field_parameters(node, count_fe_field, componentNumber, valueLabel, version, time, &int_value))
		{
			display_message(ERROR_MESSAGE, "FE_node_field_component_accumulate_value.  Failed to get count");
			return 0;
		}
		const int newIntValue = int_value + 1;
		if (!(cmzn_node_set_field_parameters(node, count_fe_field, componentNumber, valueLabel, version, time, &newIntValue)))
		{
			display_message(ERROR_MESSAGE, "FE_node_field_component_accumulate_value.  Failed to set count");
			return 0;
		}
	}
	return 1;
}

class FE_element_accumulate_node_values
{
	FE_element *element;
	const FE_element_field_template *eft;
	FE_nodeset *nodeset;
	const DsLabelIndex *nodeIndexes;
	FE_field *fe_field, *node_accumulate_fe_field, *element_count_fe_field;
	int component_number;
	FE_value time;
	FE_value *component_values;

public:
	FE_element_accumulate_node_values(FE_element *elementIn,
			const FE_element_field_template *eftIn,
			FE_nodeset *nodesetIn,
			const DsLabelIndex *nodeIndexesIn, FE_field *fe_fieldIn,
			FE_field *node_accumulate_fe_fieldIn,
			FE_field *element_count_fe_fieldIn,
			int component_numberIn, FE_value timeIn, FE_value *component_valuesIn) :
		element(elementIn),
		eft(eftIn),
		nodeset(nodesetIn),
		nodeIndexes(nodeIndexesIn),
		fe_field(fe_fieldIn),
		node_accumulate_fe_field(node_accumulate_fe_fieldIn),
		element_count_fe_field(element_count_fe_fieldIn),
		component_number(component_numberIn),
		time(timeIn),
		component_values(component_valuesIn)
	{
	}

	/**
	 * @param xiIndex  Element chart xi index starting at 0.
	 * @param basisNode1  Basis corner/end local node index for edge node 1.
	 * @param basisNode2  Basis corner/end local node index for edge node 2.
	 */
	void accumulate_edge(int xiIndex, int basisNode1, int basisNode2)
	{
		const cmzn_node_value_label basisNodeValueLabel =
			((xiIndex == 0) ? CMZN_NODE_VALUE_LABEL_D_DS1 : (xiIndex == 1) ? CMZN_NODE_VALUE_LABEL_D_DS2 : CMZN_NODE_VALUE_LABEL_D_DS3);
		const FE_value delta = this->component_values[basisNode2] - this->component_values[basisNode1];
		for (int n = 0; n < 2; ++n)
		{
			const int basisNode = (n == 0) ? basisNode1 : basisNode2;
			const int functionNumber = FE_basis_get_function_number_from_node_index_and_derivative(eft->getBasis(),
				basisNode, basisNodeValueLabel);
			if (functionNumber < 0)
				continue; // derivative not used for basis at node
			const int termCount = this->eft->getFunctionNumberOfTerms(functionNumber);
			if (0 == termCount)
				continue; // permanent zero derivative
			// only use first term, using more is not implemented
			const int term = 0;
			const int termLocalNodeIndex = this->eft->getTermLocalNodeIndex(functionNumber, term);
			FE_node *node = this->nodeset->getNode(this->nodeIndexes[termLocalNodeIndex]);
			if (node)
			{
				FE_node_field_component_accumulate_value(node,
					this->node_accumulate_fe_field, this->element_count_fe_field, this->component_number,
					this->eft->getTermNodeValueLabel(functionNumber, term),
					this->eft->getTermNodeVersion(functionNumber, term),
					this->time, delta);
			}
		}
	}
};

template <typename ObjectType> class SelfDestruct
{
	typedef int (DestroyFunction)(ObjectType &objectAddress);
	ObjectType& object;
	DestroyFunction *destroyFunction;

public:
	SelfDestruct(ObjectType& objectIn, DestroyFunction *destroyFunctionIn) :
		object(objectIn),
		destroyFunction(destroyFunctionIn)
	{
	}

	~SelfDestruct()
	{
		(this->destroyFunction)(object);
	}
};

bool FE_element_smooth_FE_field(struct FE_element *element,
	struct FE_field *fe_field, FE_value time,
	struct FE_field *node_accumulate_fe_field,
	struct FE_field *element_count_fe_field)
{
	FE_element_shape *element_shape = element->getElementShape();
	const int componentCount = get_FE_field_number_of_components(fe_field);
	if (!(element_shape && fe_field && node_accumulate_fe_field && element_count_fe_field &&
		(FE_VALUE_VALUE == fe_field->getValueType()) &&
		(INT_VALUE == element_count_fe_field->getValueType()) &&
		(get_FE_field_number_of_components(element_count_fe_field) ==
			componentCount)))
	{
		display_message(ERROR_MESSAGE, "FE_element_smooth_FE_field.  Invalid argument(s)");
		return false;
	}
	if (!FE_element_shape_is_line(element_shape))
		return true; // not implemented for these shapes, or nothing to do
	FE_mesh *mesh = element->getMesh();
	if (!mesh)
	{
		display_message(ERROR_MESSAGE, "FE_element_smooth_FE_field.  Invalid element");
		return false;
	}
	FE_nodeset *nodeset = mesh->getNodeset();
	if (!nodeset)
	{
		display_message(ERROR_MESSAGE, "FE_element_smooth_FE_field.  No nodeset");
		return false;
	}
	const FE_mesh_field_data *meshFieldData = fe_field->getMeshFieldData(mesh);
	if (!meshFieldData)
	{
		display_message(ERROR_MESSAGE, "FE_element_smooth_FE_field.  Field not defined on mesh");
		return false; // field not defined on any elements of mesh
	}
	const int dimension = element->getDimension();
	const int basisNodeCount =
		(1 == dimension) ? 2 :
		(2 == dimension) ? 4 :
		(3 == dimension) ? 8 :
		0;
	FE_value component_value[8], xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	FE_element_field_evaluation *fe_element_field_evaluation = FE_element_field_evaluation::create();
	SelfDestruct<FE_element_field_evaluation *> sd1(fe_element_field_evaluation, FE_element_field_evaluation::deaccess);
	// need to calculate field values to evaluate field in element corners,
	// ensuring optional modify function, version mapping etc. applied
	int return_code = 1;
	if (!fe_element_field_evaluation->calculate_values(fe_field, element, time))
	{
		display_message(ERROR_MESSAGE,
			"FE_element_smooth_FE_field.  Could not calculate element field values");
		return 0;
	}
	Standard_basis_function_evaluation basis_function_evaluation;
	for (int componentNumber = 0; componentNumber < componentCount; ++componentNumber)
	{
		const FE_mesh_field_template *mft = meshFieldData->getComponentMeshfieldtemplate(componentNumber);
		const FE_element_field_template *eft = mft->getElementfieldtemplate(element->getIndex());
		if (!eft)
			return true; // field not defined on element
		if (eft->getParameterMappingMode() != CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE)
			continue;
		if (FE_basis_get_number_of_nodes(eft->getBasis()) != basisNodeCount)
			continue; // some cases not implemented e.g. Hermite * quadratic Lagrange
		FE_mesh_element_field_template_data *meshEFTData = mesh->getElementfieldtemplateData(eft->getIndexInMesh());
		const DsLabelIndex *nodeIndexes = meshEFTData->getElementNodeIndexes(element->getIndex());
		if (!nodeIndexes)
		{
			display_message(ERROR_MESSAGE, "FE_element_smooth_FE_field.  "
				"Missing local-to-global node map for field %s component %d at element %d.",
				fe_field->getName(), componentNumber + 1, element->getIdentifier());
			return false;
		}
		const int localNodeCount = eft->getNumberOfLocalNodes();
		for (int n = 0; n < localNodeCount; ++n)
		{
			FE_node *node = nodeset->getNode(nodeIndexes[n]);
			if (!node)
			{
				display_message(WARNING_MESSAGE, "FE_element_smooth_FE_field.  Element %d is missing a node",
					element->getIdentifier());
				continue;
			}
			FE_node_field *node_field = node->getNodeField(fe_field);
			if (!node_field)
			{
				display_message(ERROR_MESSAGE, "FE_element_smooth_FE_field.  Field not defined at node %d used by element %d",
					node->getIdentifier(), element->getIdentifier());
				return false;
			}
			if (!(node->getNodeField(node_accumulate_fe_field)))
			{
				// define node_accumulate_fe_field and element_count_fe_field identically to fe_field at node
				// note: node field DOFs are zeroed by define_FE_field_at_node
				if (!(define_FE_field_at_node(node, node_accumulate_fe_field, node_field->components, (struct FE_time_sequence *)NULL)
					&& define_FE_field_at_node(node, element_count_fe_field, node_field->components, (struct FE_time_sequence *)NULL)))
				{
					display_message(ERROR_MESSAGE, "FE_element_smooth_FE_field.  Could not define temporary fields at node");
					return false;
				}
			}
		}
		/* set unit scale factors */
		// GRC A bit brutal, this does not take into account how they are used
		const int localScaleFactorCount = eft->getNumberOfLocalScaleFactors();
		if (localScaleFactorCount)
		{
			std::vector<FE_value> scaleFactors(localScaleFactorCount, 1.0);
			if (CMZN_OK != meshEFTData->setElementScaleFactors(element->getIndex(), scaleFactors.data()))
			{
				display_message(ERROR_MESSAGE, "FE_element_smooth_FE_field.  Failed to set unit scale factors");
				return false;
			}
		}
		// get element corner/end values
		for (int n = 0; n < basisNodeCount; n++)
		{
			xi[0] = (FE_value)(n & 1);
			xi[1] = (FE_value)((n & 2)/2);
			xi[2] = (FE_value)((n & 4)/4);
			basis_function_evaluation.invalidate();
			if (!fe_element_field_evaluation->evaluate_real(componentNumber,
				xi, basis_function_evaluation, /*derivative_order*/0,
				/*parameter_derivative_order*/0, &(component_value[n])))
			{
				display_message(ERROR_MESSAGE,
					"FE_element_smooth_FE_field.  Could not calculate element field");
				return 0;
			}
		}
		if (!return_code)
			return 0;

		FE_element_accumulate_node_values element_accumulate_node_values(element,
			eft, nodeset, nodeIndexes, fe_field, node_accumulate_fe_field,
			element_count_fe_field, componentNumber, time, component_value);
		element_accumulate_node_values.accumulate_edge(/*xi*/0, 0, 1);
		if (1 < dimension)
		{
			element_accumulate_node_values.accumulate_edge(/*xi*/0, 2, 3);
			element_accumulate_node_values.accumulate_edge(/*xi*/1, 0, 2);
			element_accumulate_node_values.accumulate_edge(/*xi*/1, 1, 3);
			if (2 < dimension)
			{
				element_accumulate_node_values.accumulate_edge(/*xi*/0, 4, 5);
				element_accumulate_node_values.accumulate_edge(/*xi*/0, 6, 7);
				element_accumulate_node_values.accumulate_edge(/*xi*/1, 4, 6);
				element_accumulate_node_values.accumulate_edge(/*xi*/1, 5, 7);
				element_accumulate_node_values.accumulate_edge(/*xi*/2, 0, 4);
				element_accumulate_node_values.accumulate_edge(/*xi*/2, 1, 5);
				element_accumulate_node_values.accumulate_edge(/*xi*/2, 2, 6);
				element_accumulate_node_values.accumulate_edge(/*xi*/2, 3, 7);
			}
		}
	}
	return true;
}

struct FE_field_order_info *CREATE(FE_field_order_info)(void)
/*******************************************************************************
LAST MODIFIED : 4 September 2001

DESCRIPTION :
Creates an empty FE_field_order_info structure.
==============================================================================*/
{
	struct FE_field_order_info *field_order_info;

	ENTER(CREATE(FE_field_order_info));
	if (ALLOCATE(field_order_info,struct FE_field_order_info,1))
	{
		field_order_info->allocated_number_of_fields = 0;
		field_order_info->number_of_fields = 0;
		field_order_info->fields = (struct FE_field **)NULL;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CREATE(FE_field_order_info).  Not enough memory");
	}
	LEAVE;

	return (field_order_info);
} /* CREATE(FE_field_order_info) */

int DESTROY(FE_field_order_info)(
	struct FE_field_order_info **field_order_info_address)
/*******************************************************************************
LAST MODIFIED : 4 September 2001

DESCRIPTION
Frees them memory used by field_order_info.
==============================================================================*/
{
	int return_code, i;
	struct FE_field_order_info *field_order_info;

	ENTER(DESTROY(FE_field_order_info));
	if ((field_order_info_address) &&
		(field_order_info = *field_order_info_address))
	{
		if (field_order_info->fields)
		{
			for (i = 0; i < field_order_info->number_of_fields; i++)
			{
				DEACCESS(FE_field)(&(field_order_info->fields[i]));
			}
			DEALLOCATE(field_order_info->fields);
		}
		DEALLOCATE(*field_order_info_address);
		return_code = 1;
	}
	else
	{
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(FE_field_order_info) */

int add_FE_field_order_info_field(
	struct FE_field_order_info *field_order_info, struct FE_field *field)
/*******************************************************************************
LAST MODIFIED : 4 September 2001

DESCRIPTION :
Adds <field> to the end of the list of fields in <field_order_info>.
==============================================================================*/
{
#define FE_FIELD_ORDER_INFO_ALLOCATE_SIZE 10
	int return_code;
	struct FE_field **temp_fields;

	ENTER(add_FE_field_order_info_field);
	if (field_order_info && field)
	{
		return_code = 1;
		if (field_order_info->number_of_fields ==
			field_order_info->allocated_number_of_fields)
		{
			field_order_info->allocated_number_of_fields +=
				FE_FIELD_ORDER_INFO_ALLOCATE_SIZE;
			if (REALLOCATE(temp_fields, field_order_info->fields, struct FE_field *,
				field_order_info->allocated_number_of_fields))
			{
				field_order_info->fields = temp_fields;
			}
			else
			{
				field_order_info->allocated_number_of_fields -=
					FE_FIELD_ORDER_INFO_ALLOCATE_SIZE;
				display_message(ERROR_MESSAGE,
					"add_FE_field_order_info_field.  Not enough memory");
				return_code = 0;
			}
		}
		if (return_code)
		{
			field_order_info->fields[field_order_info->number_of_fields] =
				ACCESS(FE_field)(field);
			field_order_info->number_of_fields++;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"add_FE_field_order_info_field.  Invalid argument");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* add_FE_field_order_info_field */

int FE_field_add_to_FE_field_order_info(struct FE_field *field,
	void *field_order_info_void)
{
	return (add_FE_field_order_info_field(
		(struct FE_field_order_info *)field_order_info_void, field));
}

int FE_field_order_info_prioritise_indexer_fields(
	struct FE_field_order_info *field_order_info)
{
	int i, j, k, number_of_indexed_values,return_code;
	struct FE_field *indexer_field;

	return_code = 0;
	if (field_order_info)
	{
		for (i = 0; i < field_order_info->number_of_fields; i++)
		{
			indexer_field = NULL;
			if ((INDEXED_FE_FIELD ==
				get_FE_field_FE_field_type(field_order_info->fields[i])) &&
				get_FE_field_type_indexed(field_order_info->fields[i], &indexer_field,
					/*ignored*/&number_of_indexed_values))
			{
				for (j = i + 1; j < field_order_info->number_of_fields; j++)
				{
					if (field_order_info->fields[j] == indexer_field)
					{
						for (k = j; k > i; k--)
						{
							field_order_info->fields[k] = field_order_info->fields[k - 1];
						}
						field_order_info->fields[i] = indexer_field;
						break;
					}
				}
			}
		}
		return_code = 1;
	}
	return return_code;
}

int clear_FE_field_order_info(struct FE_field_order_info *field_order_info)
/*******************************************************************************
LAST MODIFIED : 4 September 2001

DESCRIPTION :
Clears the fields from <field_order_info>.
==============================================================================*/
{
	int i, return_code;

	ENTER(clear_FE_field_order_info_field);
	if (field_order_info)
	{
		for (i = 0; i < field_order_info->number_of_fields; i++)
		{
			DEACCESS(FE_field)(&(field_order_info->fields[i]));
		}
		field_order_info->number_of_fields = 0;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"clear_FE_field_order_info_field.  Invalid argument");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* clear_FE_field_order_field */

int get_FE_field_order_info_number_of_fields(
	struct FE_field_order_info *field_order_info)
/*******************************************************************************
LAST MODIFIED : 13 July 1999

DESCRIPTION :
Gets the <field_order_info> number_of_fields
==============================================================================*/
{
	int number_of_fields;

	ENTER(get_FE_field_order_info_number_of_fields);
	if (field_order_info)
	{
		number_of_fields=field_order_info->number_of_fields;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"get_FE_field_order_info_number_of_fields.  Invalid argument");
		number_of_fields=0;
	}
	LEAVE;

	return (number_of_fields);
} /* get_FE_field_order_info_number_of_fields */

struct FE_field *get_FE_field_order_info_field(
	struct FE_field_order_info *field_order_info,int field_number)
/*******************************************************************************
LAST MODIFIED : 4 September 2001

DESCRIPTION :
Gets the <field_order_info> field at the specified field_number.
==============================================================================*/
{
	struct FE_field *field;

	ENTER(get_FE_field_order_info_field);
	if (field_order_info &&
		(field_number <= field_order_info->number_of_fields))
	{
		field=field_order_info->fields[field_number];
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"get_FE_field_order_info_field.  Invalid argument(s)");
		field = (struct FE_field *)NULL;
	}
	LEAVE;

	return (field);
} /* get_FE_field_order_field */

int FE_element_get_number_of_change_to_adjacent_element_permutations(
	struct FE_element *element, FE_value *xi, int face_number)
{
	int number_of_permutations;

	USE_PARAMETER(xi);
	FE_mesh *faceMesh, *fe_mesh;
	if ((element) && (fe_mesh = element->getMesh()) &&
		(faceMesh = fe_mesh->getFaceMesh()))
	{
		number_of_permutations = 1;
		DsLabelIndex faceIndex = fe_mesh->getElementFace(element->getIndex(), face_number);
		if (faceIndex >= 0)
		{
			number_of_permutations = 1;
			switch (faceMesh->getDimension())
			{
				case 1:
				{
					number_of_permutations = 2;
				} break;
				case 2:
				{
					// doesn't yet support all [8] permutations for square faces...
					// need to implement in FE_element_change_to_adjacent_element
					FE_element_shape *face_shape = faceMesh->getElementShape(faceIndex);
					if (FE_element_shape_is_triangle(face_shape))
					{
						number_of_permutations = 6;
					}
				} break;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"FE_element_get_number_of_change_to_adjacent_element_permutations.  "
			"Invalid argument(s).");
		number_of_permutations = 0;
	}
	return (number_of_permutations);
}

int FE_element_change_to_adjacent_element(struct FE_element **element_address,
	FE_value *xi, FE_value *increment, int *face_number, FE_value *xi_face,
	int permutation)
{
	int return_code = 0;
	int dimension = 0;
	struct FE_element *element;
	FE_element_shape *element_shape;
	FE_mesh *faceMesh, *fe_mesh;
	if ((element_address) && (element = *element_address) &&
		(fe_mesh = element->getMesh()) &&
		(faceMesh = fe_mesh->getFaceMesh()) &&
		(0 != (element_shape = element->getElementShape())) &&
		(0 < (dimension = get_FE_element_shape_dimension(element_shape))))
	{
		int new_face_number;
		// following checks *face_number is valid
		DsLabelIndex newElementIndex = fe_mesh->getElementFirstNeighbour(element->getIndex(), *face_number, new_face_number);
		if (newElementIndex < 0)
		{
			/* no adjacent element found */
			*face_number = -1;
			return_code = 1;
		}
		else
		{
			DsLabelIndex faceIndex = fe_mesh->getElementFace(element->getIndex(), *face_number);
			FE_element_shape *face_shape = faceMesh->getElementShape(faceIndex);
			FE_element *new_element = fe_mesh->getElement(newElementIndex);
			if (face_shape && new_element)
			{
				FE_value temp_increment[MAXIMUM_ELEMENT_XI_DIMENSIONS];
				FE_value face_normal[MAXIMUM_ELEMENT_XI_DIMENSIONS];
				FE_value local_xi_face[MAXIMUM_ELEMENT_XI_DIMENSIONS - 1];
				/* change xi and increment into element coordinates */
				FE_element_shape *new_element_shape = get_FE_element_shape(new_element);
				if ((new_element_shape) && (0 <= new_face_number))
				{
					return_code = 1;
					if (xi)
					{
						for (int j = 0 ; j < dimension - 1 ; j++)
							local_xi_face[j] = xi_face[j];
						if (permutation > 0)
						{
							/* Try rotating the face_xi coordinates */
							/* Only implementing the cases required so far and
								enumerated by FE_element_change_get_number_of_permutations */
							switch (dimension - 1)
							{
								case 1:
								{
									if (FE_element_shape_is_line(face_shape))
									{
										local_xi_face[0] = 1.0 - xi_face[0];
									}
								} break;
								case 2:
								{
									if (FE_element_shape_is_triangle(face_shape))
									{
										switch (permutation)
										{
											case 1:
											{
												local_xi_face[0] = xi_face[1];
												local_xi_face[1] = 1.0 - xi_face[0] - xi_face[1];
											} break;
											case 2:
											{
												local_xi_face[0] = 1.0 - xi_face[0] - xi_face[1];
												local_xi_face[1] = xi_face[0];
											} break;
											case 3:
											{
												local_xi_face[0] = xi_face[1];
												local_xi_face[1] = xi_face[0];
											} break;
											case 4:
											{
												local_xi_face[0] = xi_face[0];
												local_xi_face[1] = 1.0 - xi_face[0] - xi_face[1];
											} break;
											case 5:
											{
												local_xi_face[0] = 1.0 - xi_face[0] - xi_face[1];
												local_xi_face[1] = xi_face[1];
											} break;
										}
									}
								} break;
							}
						}
						const FE_value *face_to_element = get_FE_element_shape_face_to_element(new_element_shape, new_face_number);
						for (int i=0;i<dimension;i++)
						{
							xi[i]= *face_to_element;
							face_to_element++;
							for (int j=0;j<dimension-1;j++)
							{
								xi[i] += (*face_to_element)*local_xi_face[j];
								face_to_element++;
							}
						}
					}
					if (increment)
					{
						/* convert increment into face+normal coordinates */
						const FE_value *face_to_element = get_FE_element_shape_face_to_element(element_shape, *face_number);
						return_code = FE_element_shape_calculate_face_xi_normal(element_shape,
							*face_number, face_normal);
						if (return_code)
						{
							double dot_product;
							for (int i=0;i<dimension;i++)
							{
								temp_increment[i]=increment[i];
							}
							for (int i=1;i<dimension;i++)
							{
								dot_product=(double)0;
								for (int j=0;j<dimension;j++)
								{
									dot_product += (double)(temp_increment[j])*
										(double)(face_to_element[j*dimension+i]);
								}
								increment[i-1]=(FE_value)dot_product;
							}
							dot_product=(double)0;
							for (int i=0;i<dimension;i++)
							{
								dot_product += (double)(temp_increment[i])*(double)(face_normal[i]);
							}
							increment[dimension-1]=(FE_value)dot_product;

							/* Convert this back to an increment in the new element */
							return_code = FE_element_shape_calculate_face_xi_normal(new_element_shape,
								new_face_number, face_normal);
							if (return_code)
							{
								for (int i=0;i<dimension;i++)
								{
									temp_increment[i]=increment[i];
								}
								const FE_value *face_to_element = get_FE_element_shape_face_to_element(new_element_shape, new_face_number);
								for (int i=0;i<dimension;i++)
								{
									increment[i]=temp_increment[dimension-1]*face_normal[i];
									face_to_element++;
									for (int j=0;j<dimension-1;j++)
									{
										increment[i] += (*face_to_element)*temp_increment[j];
										face_to_element++;
									}
								}
							}
							else
							{
								display_message(ERROR_MESSAGE,"FE_element_change_to_adjacent_element.  "
									"Unable to calculate face_normal for new element and face");
							}
						}
						else
						{
							display_message(ERROR_MESSAGE,"FE_element_change_to_adjacent_element.  "
								"Unable to calculate face_normal for old element and face");
						}
					}
					if (return_code)
					{
						*element_address=new_element;
						*face_number=new_face_number;
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,"FE_element_change_to_adjacent_element.  "
						"Invalid new element shape or face number");
					return_code = 0;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,"FE_element_change_to_adjacent_element.  "
					"Invalid new element or element shape");
				return_code = 0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"FE_element_change_to_adjacent_element.  "
			"Invalid argument(s).  %p %p %d %p %p %d %p",element_address,
			element_address ? *element_address : 0, dimension,
			xi,increment,*face_number,xi_face);
		return_code = 0;
	}
	return (return_code);
}

int FE_element_xi_increment(struct FE_element **element_address,FE_value *xi,
	FE_value *increment)
{
	FE_value fraction;
	int face_number,i,return_code;
	struct FE_element *element;

	ENTER(FE_element_xi_increment);
	return_code=0;
	int dimension = 0;
	FE_element_shape *element_shape;
	if (element_address && (element= *element_address) &&
		(0 != (element_shape = get_FE_element_shape(element))) &&
		(0 < (dimension = get_FE_element_shape_dimension(element_shape))) && xi && increment)
	{
		// working space:
		FE_value xi_face[MAXIMUM_ELEMENT_XI_DIMENSIONS];
		FE_value local_xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
		FE_value local_increment[MAXIMUM_ELEMENT_XI_DIMENSIONS];
		for (i = 0 ; i < dimension ; i++)
		{
			local_xi[i] = xi[i];
			local_increment[i] = increment[i];
		}
		fraction = 0.0;
		return_code = 1;
		/* Continue stepping until a step within an element is able to do
			all of the remaining increment or there is no adjacent element */
		while (return_code && (fraction != 1.0))
		{
			return_code = FE_element_shape_xi_increment(element_shape,
				local_xi, local_increment, &fraction, &face_number, xi_face);
			if (return_code && (fraction < 1.0))
			{
				return_code = FE_element_change_to_adjacent_element(&element,
					local_xi, local_increment, &face_number, xi_face, /*permutation*/0);
				element_shape = get_FE_element_shape(element);
				if (0 == element_shape)
					return_code = 0;
				if (face_number == -1)
				{
					/* No adjacent face could be found, so stop */
					fraction = 1.0;
					break;
				}
			}
		}
		if (return_code)
		{
			*element_address=element;
			for (i = 0 ; i < dimension ; i++)
			{
				xi[i] = local_xi[i];
				increment[i] = local_increment[i];
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"FE_element_xi_increment.  "
			"Invalid argument(s).  %p %p %d %p %p",element_address,
			element_address ? *element_address : 0, dimension,
			xi,increment);
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* FE_element_xi_increment */

int cmzn_node_set_identifier(cmzn_node_id node, int identifier)
{
	if (node)
	{
		FE_nodeset *nodeset = node->getNodeset();
		if (nodeset)
			return nodeset->change_FE_node_identifier(node, identifier);
	}
	return CMZN_ERROR_ARGUMENT;
}
