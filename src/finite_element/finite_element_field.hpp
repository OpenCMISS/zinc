/**
 * FILE : finite_element_field.hpp
 *
 * Internal class for field interpolated over finite elements.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (FINITE_ELEMENT_FIELD_HPP)
#define FINITE_ELEMENT_FIELD_HPP

#include "opencmiss/zinc/types/elementid.h"
#include "finite_element/finite_element_mesh.hpp"
#include "general/change_log.h"
#include "general/enumerator.h"
#include "general/geometry.h"
#include "general/value.h"
#include "general/list.h"

/*
Global types
------------
*/

/**
 * FE_field maintains pointer to owning FE_region.
 */
struct FE_region;

class FE_field_parameters;

/** Information about what the field represents physically.
 * It is derived from how fields are used in cm, but does not correspond to a
 * field type in cm or identify fields in cm.
 * Note: the first value will be 0 by the ANSI standard, with each subsequent entry
 * incremented by 1. This pattern is expected by the ENUMERATOR macros.  Must
 * ensure the ENUMERATOR_STRING function returns a string for each value here. */
enum CM_field_type
{
	CM_ANATOMICAL_FIELD,
	CM_COORDINATE_FIELD,
	CM_GENERAL_FIELD
};

/** Legacy internal storage/function type for FE_field */
enum FE_field_type
{
	CONSTANT_FE_FIELD, /* fixed values */
	INDEXED_FE_FIELD,  /* indexed set of fixed values */
	GENERAL_FE_FIELD,  /* values held in nodes, elements */
	UNKNOWN_FE_FIELD
};

/** Internal class for field interpolated over finite elements. */
struct FE_field
{
	friend struct FE_region;
	friend bool FE_fields_match_fundamental(struct FE_field *field1,
		struct FE_field *field2);
	friend bool FE_fields_match_exact(struct FE_field *field1, struct FE_field *field2);
	friend int set_FE_field_number_of_components(struct FE_field *field,
		int number_of_components);
	friend int set_FE_field_type_constant(struct FE_field *field);
	friend int set_FE_field_type_general(struct FE_field *field);
	friend int get_FE_field_type_indexed(struct FE_field *field,
		struct FE_field **indexer_field, int *number_of_indexed_values);
	friend int set_FE_field_type_indexed(struct FE_field *field,
		struct FE_field *indexer_field, int number_of_indexed_values);
	friend int set_FE_field_value_type(struct FE_field *field, enum Value_type value_type);
	friend int set_FE_field_string_value(struct FE_field *field, int value_number,
		char *string);
	friend int set_FE_field_FE_value_value(struct FE_field *field, int number,
		FE_value value);
	friend int set_FE_field_int_value(struct FE_field *field, int number, int value);

protected:
	/* the name of the field, which is its identifier */
	const char *name;

private:
	FE_region *fe_region;  // owning FE_region, not accessed
	enum CM_field_type cm_field_type;  // anatomical/coordinate/general
	enum FE_field_type fe_field_type;  // constant/general/indexed
	/* following two for INDEXED_FE_FIELD only */
	struct FE_field *indexer_field;
	int number_of_indexed_values;
	/* the number of components (allows vector valued fields) */
	int number_of_components;
	/* the names of the different vector components */
	char **component_names;
	/* the coordinate system for the vector */
	Coordinate_system coordinate_system;
	/* the number of global values/derivatives that are stored with the field */
	int number_of_values;
	/* the type of the values returned by the field */
	enum Value_type value_type;
	// with element_xi_host_mesh, embedded node field data for 0=nodes, 1=datapoints
	FE_mesh_embedded_node_field *embeddedNodeFields[2];
	// for value_type == ELEMENT_XI_VALUE, accessed host mesh
	// or nullptr if not yet determined from legacy input
	FE_mesh *element_xi_host_mesh;
	/* array of global values/derivatives that are stored with the field.
	 * The actual values can be extracted using the <value_type> */
	Value_storage *values_storage;

	// field definition and data on FE_mesh[dimension - 1]
	// Future: limit to being defined on a single mesh; requires change to current usage
	FE_mesh_field_data *meshFieldData[MAXIMUM_ELEMENT_XI_DIMENSIONS];

	// non-accessed handle to object indexing parameters, when exists
	// clients access it, and it accesses this FE_field
	FE_field_parameters *fe_field_parameters;

	/* the number of computed fields wrapping this FE_field */
	int number_of_wrappers;
	/* the number of structures that point to this field.  The field cannot be
		destroyed while this is greater than 0 */
	int access_count;

protected:

	// used only by FE_field_identifier pseudo class
	FE_field() :
		name(nullptr),
		indexer_field(nullptr),
		component_names(nullptr),
		element_xi_host_mesh(nullptr),
		values_storage(nullptr),
		fe_field_parameters(nullptr),
		access_count(0)
	{
		for (int d = 0; d < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++d)
			this->meshFieldData[d] = nullptr;
	}

	FE_field(const char *nameIn, struct FE_region *fe_regionIn);

	~FE_field();

private:

	// called on merge or to clear when owning FE_region destroyed
	void set_FE_region(FE_region *fe_regionIn)
	{
		this->fe_region = fe_regionIn;
	}

public:

	static FE_field *create(const char *nameIn, FE_region *fe_regionIn);

	inline FE_field *access()
	{
		++access_count;
		return this;
	}

	static int deaccess(FE_field **fieldAddress);

	int getAccessCount() const
	{
		return this->access_count;
	}

	FE_region *get_FE_region() const
	{
		return this->fe_region;
	}

	const char *getName() const
	{
		return this->name;
	}

	/** Set the name of the <field>.
	 * Only call this function for unmanaged fields.
	 * All others should use FE_region_set_FE_field_name. */
	int setName(const char *nameIn);

	int getNumberOfComponents() const
	{
		return this->number_of_components;
	}

	bool isTypeCoordinate() const
	{
		return (CM_COORDINATE_FIELD == this->cm_field_type)
			&& (FE_VALUE_VALUE == this->value_type)
			&& (1 <= this->number_of_components)
			&& (3 >= this->number_of_components);
	}

	cmzn_element *getOrInheritOnElement(cmzn_element *element,
		int inheritFaceNumber, cmzn_element *topLevelElement,
		FE_value *coordinateTransformation);

	/**
	 * @param componentIndex  Component index starting at 0
	 * @return  Allocated component name */
	char *getComponentName(int componentIndex) const;

	/** @return  true on success, otherwise false */
	bool setComponentName(int componentIndex, const char *componentName);

	/* @return  Accessed handle to existing or new field parameters for field */
	FE_field_parameters *get_FE_field_parameters();

	/** Only to be called by ~FE_field_parameters */
	void clear_FE_field_parameters()
	{
		this->fe_field_parameters = nullptr;
	}

	/** Writes a text description of field to console/logger */
	void list() const;

	/** Returns true if field varies with time on any nodes */
	bool hasMultipleTimes();

	/**
	 * Copies the field definition from source to destination, except for name,
	 * mesh field data and node definition. Assumes source field has been
	 * proven compatible with destination including that source indexer field has
	 * already been merged.
	 * If source and destination fields belong to different FE_regions, substitute
	 * local indexer_field and element_xi_host_mesh if appropriate.
	 * Only for use by FE_region. */
	int copyProperties(FE_field *source);

	/** Record that a Computed_field_finite_element is wrapping this FE_field.
	 * @return  New number of wrappers */
	int addWrapper()
	{
		return ++(this->number_of_wrappers);
	}

	/** Record that a Computed_field_finite_element is no longer wrapping this FE_field.
	 * @return  The number of wrappers remaining. */
	int removeWrapper()
	{
		return --(this->number_of_wrappers);
	}

	bool usesNonlinearBasis() const;

	const Coordinate_system& getCoordinateSystem() const
	{
		return this->coordinate_system;
	}

	void setCoordinateSystem(const Coordinate_system& coordinateSystemIn)
	{
		this->coordinate_system = coordinateSystemIn;
	}

	/** Ensure field is not defined on mesh
	  * Assumes called with FE_region change caching on; records change but doesn't notify */
	void clearMeshFieldData(FE_mesh *mesh);

	/** Note: must get no field data from getMeshFieldData first!
	 * @return  New mesh field data defining field over the given mesh, or nullptr if failed.
	 * @param mesh  The mesh to create data for. Must be from same region as field.
	 */
	FE_mesh_field_data *createMeshFieldData(FE_mesh *mesh);

	/** @return  Mesh field data defining field over the given mesh, or nullptr if none.
	 * @param field  The field to create data for.
	 * @param mesh  The mesh to create data for. Must be from same region as field.
	 */
	FE_mesh_field_data *getMeshFieldData(const FE_mesh *mesh) const
	{
		if (mesh && (mesh->get_FE_region() == this->fe_region))
			return this->meshFieldData[mesh->getDimension() - 1];
		return nullptr;
	}

	/** Get number of global field parameters */
	int getNumberOfValues() const
	{
		return this->number_of_values;
	}

	CM_field_type get_CM_field_type() const
	{
		return this->cm_field_type;
	}

	void set_CM_field_type(CM_field_type cm_field_typeIn)
	{
		this->cm_field_type = cm_field_typeIn;
	}

	FE_field_type get_FE_field_type() const
	{
		return this->fe_field_type;
	}

	Value_type getValueType() const
	{
		return this->value_type;
	}

	/** If the FE_field has value_type ELEMENT_XI_VALUE, this returns the
	 * host mesh the embedded locations are within, or nullptr if not yet set (legacy
	 * input only).
	 * @return  Host mesh (not accessed) or nullptr if none.
	 */
	FE_mesh *getElementXiHostMesh() const
	{
		return this->element_xi_host_mesh;
	}

	/** If the FE_field has value_type ELEMENT_XI_VALUE, sets the host mesh the
	 * embedded locations are within.
	 * @param hostMesh  The host mesh to set.
	 * @return  Standard result code.
	 */
	int setElementXiHostMesh(FE_mesh *hostMesh);

	/**
	 * If field is of value_type ELEMENT_XI_VALUE and element_xi_host_mesh is set,
	 * returns pointer to embedded node information for it in host mesh, otherwise
	 * nullptr.
	 * @param nodeset  Nodeset from same region. Not checked.
	 **/
	FE_mesh_embedded_node_field *getEmbeddedNodeField(FE_nodeset *nodeset) const;

	FE_field *getIndexerField() const
	{
		return this->indexer_field;
	}

	int getNumberOfIndexedValues() const
	{
		return this->number_of_indexed_values;
	}

	/** Client must index only to NumberOfValues */
	const int *getIntValues() const
	{
		if ((this->values_storage) && (this->value_type == INT_VALUE))
			return reinterpret_cast<int *>(this->values_storage);
		return nullptr;
	}

	/** Client must index only to NumberOfValues */
	const FE_value *getRealValues() const
	{
		if ((this->values_storage) && (this->value_type == FE_VALUE_VALUE))
			return reinterpret_cast<FE_value *>(this->values_storage);
		return nullptr;
	}

	/** Client must index only to NumberOfValues */
	const char **getStringValues() const
	{
		if ((this->values_storage) && (this->value_type == STRING_VALUE))
			return reinterpret_cast<const char **>(this->values_storage);
		return nullptr;
	}

	/** Template version without type check.
	 * Client must index only to NumberOfValues */
	template <typename VALUE_TYPE>
	void getValues(const VALUE_TYPE*& values) const
	{
		values = reinterpret_cast<const VALUE_TYPE *>(this->values_storage);
	}

};

DECLARE_LIST_TYPES(FE_field);

DECLARE_CHANGE_LOG_TYPES(FE_field);

struct Set_FE_field_conditional_data
/*******************************************************************************
LAST MODIFIED : 3 December 2002

DESCRIPTION :
User data structure passed to set_FE_field_conditional, containing the
fe_field_list and the optional conditional_function (and
conditional_function_user_data) for selecting a field out of a subset of the
fields in the list.
==============================================================================*/
{
	LIST_CONDITIONAL_FUNCTION(FE_field) *conditional_function;
	void *conditional_function_user_data;
	struct LIST(FE_field) *fe_field_list;
}; /* struct Set_FE_field_conditional_data */

/*
Global functions
----------------
*/

PROTOTYPE_ENUMERATOR_FUNCTIONS(CM_field_type);

PROTOTYPE_OBJECT_FUNCTIONS(FE_field);

PROTOTYPE_LIST_FUNCTIONS(FE_field);

PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(FE_field,name,const char *);

PROTOTYPE_CHANGE_LOG_FUNCTIONS(FE_field);

/**
 * Returns true if <field1> and <field2> have the same fundamental definition,
 * namely they:
 * 1. Have the same value type
 * 2. Have the same fe field type (general, indexed etc.)
 * 3. Have the same number of components
 * 4. Have the same coordinate system
 * If so, they can be merged without affecting the rest of the model.
 * Other attributes such as cm field type (field, coordinate, anatomical) are
 * not considered fundamental. The name is also not compared.
 * Must ensure this function fits with FE_fields_match_exact.
 */
bool FE_fields_match_fundamental(struct FE_field *field1,
	struct FE_field *field2);

/**
 * Returns true if <field1> and <field2> have exactly the same definition,
 * comparing all attributes.
 * @see FE_fields_match_fundamental
 */
bool FE_fields_match_exact(struct FE_field *field1, struct FE_field *field2);

/**
 * List iterator function which fetches a field with the same name as <field>
 * from <field_list>. Returns 1 (true) if there is either no such field in the
 * list or the two fields return true for FE_fields_match_fundamental(),
 * otherwise returns 0 (false).
 */
int FE_field_can_be_merged_into_list(struct FE_field *field, void *field_list_void);

/**
 * Return true if any basis functions used by the field is non-linear i.e.
 * quadratic, cubic, Fourier etc.
 */
bool FE_field_uses_non_linear_basis(struct FE_field *fe_field);

struct FE_field *find_first_time_field_at_FE_node(struct FE_node *node);
/*******************************************************************************
LAST MODIFIED : 9 June 1999

Find the first time based field at a node
==============================================================================*/

int ensure_FE_field_is_in_list(struct FE_field *field, void *field_list_void);
/*******************************************************************************
LAST MODIFIED : 29 March 2006

DESCRIPTION :
Iterator function for adding <field> to <field_list> if not currently in it.
==============================================================================*/

char *get_FE_field_component_name(struct FE_field *field,int component_no);
/*******************************************************************************
LAST MODIFIED : 10 May 2000

DESCRIPTION :
Returns the name of component <component_no> of <field>. If no name is stored
for the component, a string comprising the value component_no+1 is returned.
Up to calling function to DEALLOCATE the returned string.
==============================================================================*/

int set_FE_field_component_name(struct FE_field *field,int component_no,
	const char *component_name);
/*******************************************************************************
LAST MODIFIED : 10 May 2000

DESCRIPTION :
Sets the name of component <component_no> of <field>. Only sets name if it is
different from that already returned for field to preserve default names if can.
==============================================================================*/

int set_FE_field_coordinate_system(struct FE_field *field,
	const Coordinate_system *coordinate_system);
/*******************************************************************************
LAST MODIFIED : 28 January 1999

DESCRIPTION :
Sets the coordinate system of the <field>.
==============================================================================*/

int get_FE_field_number_of_components(struct FE_field *field);
/*******************************************************************************
LAST MODIFIED : 16 November 1998

DESCRIPTION :
Returns the number of components for the <field>.
==============================================================================*/

int set_FE_field_number_of_components(struct FE_field *field,
	int number_of_components);
/*******************************************************************************
LAST MODIFIED : 1 September 1999

DESCRIPTION :
Sets the number of components in the <field>. Automatically assumes names for
any new components. Clears/reallocates the values_storage for FE_field_types
that use them, eg. CONSTANT_FE_FIELD and INDEXED_FE_FIELD - but only if number
of components changes. If function fails the field is left exactly as it was.
Should only call this function for unmanaged fields.
ELEMENT_XI_VALUE, STRING_VALUE and URL_VALUE fields may only have 1 component.
==============================================================================*/

enum CM_field_type get_FE_field_CM_field_type(struct FE_field *field);
/*******************************************************************************
LAST MODIFIED : 31 August 1999

DESCRIPTION :
Returns the CM_field_type of the <field>.
==============================================================================*/

int set_FE_field_CM_field_type(struct FE_field *field,
	enum CM_field_type cm_field_type);
/*******************************************************************************
LAST MODIFIED : 31 August 1999

DESCRIPTION :
Sets the CM_field_type of the <field>.
Should only call this function for unmanaged fields.
==============================================================================*/

enum FE_field_type get_FE_field_FE_field_type(struct FE_field *field);
/*******************************************************************************
LAST MODIFIED : 31 August 1999

DESCRIPTION :
Returns the FE_field_type for the <field>.
==============================================================================*/

int set_FE_field_type_constant(struct FE_field *field);
/*******************************************************************************
LAST MODIFIED : 1 September 1999

DESCRIPTION :
Converts the <field> to type CONSTANT_FE_FIELD.
Allocates and clears the values_storage of the field to fit
field->number_of_components of the current value_type.
If function fails the field is left exactly as it was.
Should only call this function for unmanaged fields.
==============================================================================*/

int set_FE_field_type_general(struct FE_field *field);
/*******************************************************************************
LAST MODIFIED : 1 September 1999

DESCRIPTION :
Converts the <field> to type GENERAL_FE_FIELD.
Frees any values_storage currently in use by the field.
If function fails the field is left exactly as it was.
Should only call this function for unmanaged fields.
==============================================================================*/

int get_FE_field_type_indexed(struct FE_field *field,
	struct FE_field **indexer_field,int *number_of_indexed_values);
/*******************************************************************************
LAST MODIFIED : 1 September 1999

DESCRIPTION :
If the field is of type INDEXED_FE_FIELD, the indexer_field and
number_of_indexed_values it uses are returned - otherwise an error is reported.
Use function get_FE_field_FE_field_type to determine the field type.
==============================================================================*/

int set_FE_field_type_indexed(struct FE_field *field,
	struct FE_field *indexer_field,int number_of_indexed_values);
/*******************************************************************************
LAST MODIFIED : 1 September 1999

DESCRIPTION :
Converts the <field> to type INDEXED_FE_FIELD, indexed by the given
<indexer_field> and with the given <number_of_indexed_values>. The indexer_field
must return a single integer value to be valid.
Allocates and clears the values_storage of the field to fit
field->number_of_components x number_of_indexed_values of the current
value_type. If function fails the field is left exactly as it was.
Should only call this function for unmanaged fields.
==============================================================================*/

enum Value_type get_FE_field_value_type(struct FE_field *field);
/*******************************************************************************
LAST MODIFIED : 19 February 1999

DESCRIPTION :
Returns the value_type of the <field>.
==============================================================================*/

int set_FE_field_value_type(struct FE_field *field,enum Value_type value_type);
/*******************************************************************************
LAST MODIFIED : 1 September 1999

DESCRIPTION :
Sets the value_type of the <field>. Clears/reallocates the values_storage for
FE_field_types that use them, eg. CONSTANT_FE_FIELD and INDEXED_FE_FIELD - but
only if the value_type changes. If function fails the field is left exactly as
it was. Should only call this function for unmanaged fields.
ELEMENT_XI_VALUE, STRING_VALUE and URL_VALUE fields may only have 1 component.
=========================================================================*/

int set_FE_field_string_value(struct FE_field *field, int value_number,
	char *string);
/*******************************************************************************
LAST MODIFIED : 22 September 1999

DESCRIPTION :
Copies and sets the <string> stored at <value_number> in the <field>.
<string> may be NULL.
==============================================================================*/

int set_FE_field_FE_value_value(struct FE_field *field, int number,
	FE_value value);
/*******************************************************************************
LAST MODIFIED : 2 September 1999

DESCRIPTION :
Sets the specified global FE_value <value> in <field>.
The <field> must be of type FE_VALUE_VALUE to have such values and
<number> must be within the range from FE_field::getNumberOfValues.
==============================================================================*/

int set_FE_field_int_value(struct FE_field *field, int number, int value);
/*******************************************************************************
LAST MODIFIED : 2 September 1999

DESCRIPTION :
Sets the specified global int <value> in <field>.
The <field> must be of type INT_VALUE to have such values and
<number> must be within the range from FE_field::getNumberOfValues.
==============================================================================*/

const char *get_FE_field_name(struct FE_field *field);
/*******************************************************************************
LAST MODIFIED : 19 February 1999

DESCRIPTION :
Returns a pointer to the name for the <field>.
Should only call this function for unmanaged fields.
==============================================================================*/

int FE_field_is_1_component_integer(struct FE_field *field,void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 18 May 2000

DESCRIPTION :
Conditional function returning true if <field> has exactly 1 component and a
value type of integer.
This type of field is used for storing eg. grid_point_number.
==============================================================================*/

int FE_field_is_coordinate_field(struct FE_field *field,void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 18 May 2000

DESCRIPTION :
Conditional function returning true if the <field> is a coodinate field
(defined by having a CM_field_type of coordinate) has a Value_type of
FE_VALUE_VALUE and has from 1 to 3 components.
==============================================================================*/

#endif /* !defined (FINITE_ELEMENT_FIELD_HPP) */
