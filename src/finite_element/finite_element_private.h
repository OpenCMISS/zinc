/*******************************************************************************
FILE : finite_element_private.h

LAST MODIFIED : 30 May 2003

DESCRIPTION :
Private interfaces to finite element objects to be included only by privileged
modules such as finite_element_region.c.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (FINITE_ELEMENT_PRIVATE_H)
#define FINITE_ELEMENT_PRIVATE_H

#include "opencmiss/zinc/types/nodeid.h"
#include "opencmiss/zinc/status.h"
#include "finite_element/finite_element.h"
#include "finite_element/node_field_template.hpp"
#include "general/indexed_list_private.h"
#include "general/indexed_list_stl_private.hpp"
#include "general/list.h"
#include "general/object.h"

/*
Global types
------------
*/

struct FE_node_field_info;
/*******************************************************************************
LAST MODIFIED : 4 October 2002

DESCRIPTION :
Structure describing how fields and their values and derivatives are stored in
cmzn_node structures.
==============================================================================*/

DECLARE_LIST_TYPES(FE_node_field_info);

struct FE_element_type_node_sequence_identifier;
/*******************************************************************************
LAST MODIFIED : 13 February 2003

DESCRIPTION :
See struct FE_element_type_node_sequence.
==============================================================================*/

struct FE_element_type_node_sequence;
/*******************************************************************************
LAST MODIFIED : 13 February 2003

DESCRIPTION :
Structure for storing an element with its identifier being its cm_type and the
number and list - in ascending order - of the nodes referred to by the default
coordinate field of the element. Indexed lists, indexed using function
compare_FE_element_type_node_sequence_identifier ensure that recalling a line or
face with the same nodes is extremely rapid. FE_mesh
uses them to find faces and lines for elements without them, if they exist.
==============================================================================*/

DECLARE_LIST_TYPES(FE_element_type_node_sequence);

/*
Private functions
-----------------
*/

/**
 * Describes storage of global values and derivatives for a field at a node.
 * Public only to allow efficient conversion of legacy node DOF indexes.
 * @see FE_mesh::checkConvertLegacyNodeParameters
 */
struct FE_node_field
{
	/* the field which this accesses values and derivatives for */
	struct FE_field *field;
	/* an array with <number_of_components> node field templates */
	FE_node_field_template *components;
	/* the time dependence of all components below this point,
	   if it is non-NULL then every value storage must be an array of
	   values that matches this fe_time_sequence */
	struct FE_time_sequence *time_sequence;
	/* the number of structures that point to this node field.  The node field
		cannot be destroyed while this is greater than 0 */
	int access_count;

	const FE_node_field_template *getComponent(int componentIndex) const
	{
		return &this->components[componentIndex];
	}

	FE_node_field(struct FE_field *fieldIn);

	~FE_node_field();

	static FE_node_field *create(struct FE_field *fieldIn);

	/** Clone node field, optionally offseting all component valuesOffsets.
	  * For non-GENERAL_FE_FIELD types, the valuesOffset is not changed.*/
	static FE_node_field *clone(const FE_node_field& source, int deltaValuesOffset = 0);

	FE_node_field *access()
	{
		++this->access_count;
		return this;
	}

	static int deaccess(FE_node_field* &node_field)
	{
		if (!node_field)
		{
			return CMZN_ERROR_ARGUMENT;
		}
		--(node_field->access_count);
		if (node_field->access_count <= 0)
		{
			delete node_field;
		}
		node_field = 0;
		return CMZN_OK;
	}

	void setField(FE_field *fieldIn)
	{
		REACCESS(FE_field)(&this->field, fieldIn);
	}

	/** @return  True if field is multi component with all components defined identically */
	bool isHomogeneousMultiComponent() const
	{
		const int componentCount = this->field->getNumberOfComponents();
		if (componentCount <= 1)
		{
			return false;
		}
		for (int c = 1; c < componentCount; ++c)
		{
			if (!(this->components[c].matches(this->components[0])))
			{
				return false;
			}
		}
		return true;
	}

	/** Returns the maximum total number of field parameters for any component */
	int getMaximumComponentTotalValuesCount() const
	{
		int maximumComponentTotalValuesCount = 0;
		const int componentCount = this->field->getNumberOfComponents();
		for (int c = 0; c < componentCount; ++c)
		{
			if (this->components[c].getTotalValuesCount() > maximumComponentTotalValuesCount)
			{
				maximumComponentTotalValuesCount = this->components[c].getTotalValuesCount();
			}
		}
		return maximumComponentTotalValuesCount;
	}

	/** Returns the total number of field parameters for all components */
	int getTotalValuesCount() const
	{
		int totalValuesCount = 0;
		const int componentCount = this->field->getNumberOfComponents();
		for (int c = 0; c < componentCount; ++c)
		{
			totalValuesCount += this->components[c].getTotalValuesCount();
		}
		return totalValuesCount;
	}

	/** @return  Maximum number of versions stored for valueLabel in any component */
	int getValueMaximumVersionsCount(cmzn_node_value_label valueLabel) const;

	/** @return  Minimum number of versions stored for valueLabel over all components */
	int getValueMinimumVersionsCount(cmzn_node_value_label valueLabel) const;

	/** @return  Highest numerical value of value label above VALUE for any component */
	int getMaximumDerivativeNumber() const;

	/** Increase highestDerivative and highestVersion if higher in this node field */
	void expandHighestDerivativeAndVersion(int& highestDerivative, int& highestVersion) const;

};

struct LIST(FE_node_field) *FE_node_field_list_clone_with_FE_field_list(
	struct LIST(FE_node_field) *node_field_list,
	struct LIST(FE_field) *fe_field_list,
	struct FE_time_sequence_package *fe_time_sequence_package);
/*******************************************************************************
LAST MODIFIED : 24 February 2003

DESCRIPTION :
Returns a new FE_node_field list that is identical to <node_field_list>
except that it references equivalent same-name fields from <fe_field_list> and
uses FE_time_sequences in <fe_time_sequence_package>.
It is an error if an equivalent FE_field is not found.
==============================================================================*/

/**
 * @return  true if a node field exactly matching <node_field> is found in
 * <node_field_list>.
 */
int FE_node_field_is_in_list(struct FE_node_field *node_field,
	void *node_field_list_void);

/**
 * Frees accesses and dynamically allocated memory in <start_of_values_storage>
 * for the FE_node_field.
 * The <start_of_the_values_storage> address is passed so that the this function
 * can be called from an interator, the <node> is not passed so this function can
 * be used to deallocate parts of any values_storage.  The <values_storage> can
 * be NULL if there are no GENERAL_FE_FIELDs defined on the node.
 * Only certain value types, eg. arrays, strings, element_xi require this.
 */
int FE_node_field_free_values_storage_arrays(
	struct FE_node_field *node_field, void *start_of_values_storage_void);

/**
 * @return  The size, in bytes, of the data in the nodal values storage owned
 * by the all the node_fields in node_field_list.
 */
int get_FE_node_field_list_values_storage_size(
	struct LIST(FE_node_field) *node_field_list);

/**
 * Allocates values_storage to the same size as node->values_storage.
 * Copies the node->values_storage to values_storage. Also allocates and copies
 * any arrays in node->values_storage.
 * Note that values_storage contains no information about the value_type(s) or the
 * number of values of the data in it. You must refer to the FE_node/FE_field to
 * get this.
 * The calling function is responsible for deallocating values_storage,
 * and any arrays in values_storage.
 */
int allocate_and_copy_FE_node_values_storage(struct FE_node *node,
	Value_storage **values_storage);

int FE_node_field_info_add_node_field(
	struct FE_node_field_info *fe_node_field_info, 
	struct FE_node_field *new_node_field, int new_number_of_values);
/*******************************************************************************
LAST MODIFIED : 24 August 2005

DESCRIPTION :
Adds the <new_node_field> to the list in the <fe_node_field_info> and updates the
<new_number_of_values>.  This should only be done if object requesting the change
is known to be the only object using this field info.
==============================================================================*/

PROTOTYPE_INDEXED_LIST_STL_IDENTIFIER_CHANGE_FUNCTIONS(cmzn_node,cm_node_identifier);

/**
 * Merges the fields from <source> into <destination>. Existing fields in the
 * <destination> keep the same node field description as before with new field
 * storage following them. Where existing fields in <destination> are passed in
 * <source>, values from <source> take precedence, but the node field structure
 * remains unchanged.
 * Function is atomic; <destination> is unchanged if <source> cannot be merged.
 * @param optimised_merge  If non-zero, time arrays are allowed to be
 * transferred from source instead of copying. Use ONLY for external region merge
 * during I/O.
 */
int merge_FE_node(cmzn_node *destination, cmzn_node *source, int optimised_merge = 0);

/**
 * Create structure storing an element with its identifier being its cm_type
 * and the number and list - in ascending order - of the nodes referred to by
 * the default coordinate field of the element. Indexed lists, indexed using
 * function compare_FE_element_type_node_sequence_identifier ensure that
 * recalling a line or face with the same nodes is extremely rapid.
 * FE_mesh uses them to find faces and lines for elements without them,
 * if they exist.
 * @param face_number  If non-negative, calculate nodes for face number of
 * element, as if the face element were supplied to this function. If so, the
 * returned structure accesses the supplied element. It is assumed this is
 * only used when creating new faces, and the face should be set as soon as
 * created.
 * @param result  Result OK on success, otherwise ERROR_NOT_FOUND if
 * nodes not obtainable, otherwise any other error.
 * @return  On success, newly created sequence.
 */
struct FE_element_type_node_sequence *CREATE(FE_element_type_node_sequence)(
	int &result, struct FE_element *element, int face_number = -1);

int DESTROY(FE_element_type_node_sequence)(
	struct FE_element_type_node_sequence **element_type_node_sequence_address);
/*******************************************************************************
LAST MODIFIED : 13 February 2003

DESCRIPTION :
Cleans up memory used by the FE_element_type_node_sequence.
==============================================================================*/

int FE_element_type_node_sequence_is_collapsed(
	struct FE_element_type_node_sequence *element_type_node_sequence);
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

/**
 * Gets the FE_element from the <element_type_node_sequence>.
 */
struct FE_element *FE_element_type_node_sequence_get_FE_element(
	struct FE_element_type_node_sequence *element_type_node_sequence);

/**
 * Sets the FE_element in the <element_type_node_sequence>. Use only to
 * substitute face element when sequence was created from face number of
 * parent element.
 */
void FE_element_type_node_sequence_set_FE_element(
	struct FE_element_type_node_sequence *element_type_node_sequence,
	struct FE_element *element);

/**
 * Returns existing FE_element_type_node_sequence from list with matching
 * identifier to supplied one.
 */
FE_element_type_node_sequence *FE_element_type_node_sequence_list_find_match(
	struct LIST(FE_element_type_node_sequence) *element_type_node_sequence_list,
	FE_element_type_node_sequence *element_type_node_sequence);

PROTOTYPE_OBJECT_FUNCTIONS(FE_element_type_node_sequence);

PROTOTYPE_LIST_FUNCTIONS(FE_element_type_node_sequence);

PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(FE_element_type_node_sequence, \
	identifier, FE_element_type_node_sequence_identifier&);

#endif /* !defined (FINITE_ELEMENT_PRIVATE_H) */
