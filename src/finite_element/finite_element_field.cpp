/**
 * FILE : finite_element_field.cpp
 *
 * Internal class for field interpolated over finite elements.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "opencmiss/zinc/status.h"
#include "finite_element/finite_element_field_parameters.hpp"
#include "finite_element/finite_element_field_private.hpp"
#include "finite_element/finite_element_mesh.hpp"
#include "finite_element/finite_element_region_private.h"
#include "finite_element/finite_element_value_storage.hpp"
#include "general/change_log_private.h"
#include "general/debug.h"
#include "general/enumerator_private.hpp"
#include "general/indexed_list_stl_private.hpp"
#include "general/indexed_list_private.h"
#include "general/message.h"
#include "general/mystring.h"

/*
Module types
------------
*/

FULL_DECLARE_CHANGE_LOG_TYPES(FE_field);

/*
Module functions
----------------
*/

DECLARE_CHANGE_LOG_MODULE_FUNCTIONS(FE_field)

static char *get_automatic_component_name(char **component_names,
	int component_no)
/*******************************************************************************
LAST MODIFIED : 28 January 1999

DESCRIPTION :
Used internally by some FE_field functions. Returns an allocated string
containing the component name for <component_no> (starting at 0 for the first).
If <component_names> or component_names[component_no] are NULL a name consisting
of the value component_no+1 is created and returned.
If <component_names> is not NULL, the function assumes that component_no is
less than the number of names in this array.
It is up to the calling function to deallocate the returned string.
==============================================================================*/
{
	char *component_name,*source_name,temp_string[20];

	ENTER(get_automatic_component_name);
	if (0<=component_no)
	{
		if (component_names&&component_names[component_no])
		{
			source_name=component_names[component_no];
		}
		else
		{
			sprintf(temp_string,"%i",component_no+1);
			source_name=temp_string;
		}
		component_name = duplicate_string(source_name);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"get_automatic_component_name.  Invalid component_no");
		component_name=(char *)NULL;
	}
	LEAVE;

	return (component_name);
} /* get_automatic_component_name */

/*
Global functions
----------------
*/

FE_field::FE_field(const char *nameIn, struct FE_region *fe_regionIn) :
	name(duplicate_string(nameIn)),
	fe_region(fe_regionIn),
	fe_field_type(GENERAL_FE_FIELD),
	indexer_field(nullptr),
	number_of_indexed_values(0),
	cm_field_type(CM_GENERAL_FIELD),
	number_of_components(0),  // GRC really?
	component_names(nullptr),  // not allocated until we have custom names
	coordinate_system(),
	number_of_values(0),
	values_storage(nullptr),
	value_type(UNKNOWN_VALUE),
	element_xi_host_mesh(nullptr),
	fe_field_parameters(nullptr),
	number_of_wrappers(0),
	access_count(1)
{
	this->coordinate_system.type = UNKNOWN_COORDINATE_SYSTEM;
	for (int d = 0; d < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++d)
		this->meshFieldData[d] = nullptr;
	for (int i = 0; i < 2; ++i)
		this->embeddedNodeFields[i] = nullptr;
}

FE_field::~FE_field()
{
	if (0 != this->access_count)
	{
		display_message(ERROR_MESSAGE, "~FE_field.  Non-zero access_count (%d)", this->access_count);
		return;
	}
	if (this->element_xi_host_mesh)
	{
		for (int i = 0; i < 2; ++i)
			this->element_xi_host_mesh->removeEmbeddedNodeField(this->embeddedNodeFields[i]);
		FE_mesh::deaccess(this->element_xi_host_mesh);
	}
	for (int d = 0; d < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++d)
		delete this->meshFieldData[d];
	if (this->indexer_field)
		FE_field::deaccess(&this->indexer_field);
	if (this->values_storage)
	{
		/* free any arrays pointed to by field->values_storage */
		free_value_storage_array(this->values_storage, this->value_type,
			(struct FE_time_sequence *)NULL, this->number_of_values);
		/* free the global values */
		DEALLOCATE(this->values_storage);
	}
	if (this->name)
		DEALLOCATE(this->name);
	if (this->component_names)
	{
		for (int c = 0; c < this->number_of_components; ++c)
			DEALLOCATE(this->component_names[c]);
		DEALLOCATE(this->component_names);
	}
}

FE_field *FE_field::create(const char *nameIn, FE_region *fe_regionIn)
{
	if ((nameIn) && (fe_regionIn))
		return new FE_field(nameIn, fe_regionIn);
	display_message(ERROR_MESSAGE, "FE_field::create.  Invalid argument(s)");
	return nullptr;
}

int FE_field::deaccess(FE_field **fieldAddress)
{
	if (!((fieldAddress) && (*fieldAddress)))
		return 0;
	--((*fieldAddress)->access_count);
	if ((*fieldAddress)->access_count <= 0)
		delete *fieldAddress;
	*fieldAddress = nullptr;
	return 1;
}

int FE_field::setName(const char *nameIn)
{
	if (nameIn)
	{
		char *temp = duplicate_string(nameIn);
		if (temp)
		{
			DEALLOCATE(this->name);
			this->name = temp;
			return 1;
		}
		display_message(ERROR_MESSAGE, "FE_field::setName.  Failed");
	}
	else
	{
		display_message(ERROR_MESSAGE, "FE_field::setName.  Missing name");
	}
	return 0;
}

/**
 * Get definition of field on element, or higher dimension element it is a face
 * of, or a face of a face. If field is inherited, returns the xi coordinate
 * transformation to the higher dimensional element.
 * Can also force inheritance onto a specified face of the supplied element.
 * Function is recursive.
 *
 * @param element  The element we want to evaluate on.
 * @param inheritFaceNumber  If non-negative, inherit onto this face number
 * of element, as if the face element were supplied to this function.
 * @param topLevelElement  If supplied, forces field definition to be
 * obtained from it, otherwise field is undefined.
 * @param coordinateTransformation  If field is inherited from higher dimension
 * element, gives mapping of element coordinates to return element coordinates.
 * This is a matrix of dimension(return element) rows X dimension(element)+1
 * columns. This represents an affine transformation, b + A xi for calculating
 * the return element xi coordinates from those of element, where b is the
 * first column of the <coordinate_transformation> matrix. Caller must pass an
 * array of size sufficient for the expected dimensions, e.g.
 * MAXIMUM_ELEMENT_XI_DIMENSIONS*MAXIMUM_ELEMENT_XI_DIMENSIONS.
 * @return  The element the field is actually defined on, or 0 if not defined.
 * Return element either matches supplied element or is an ancestor of it.
 */
cmzn_element *FE_field::getOrInheritOnElement(cmzn_element *element,
	int inheritFaceNumber, cmzn_element *topLevelElement,
	FE_value *coordinateTransformation)
{
	const FE_mesh *mesh = element->getMesh();
	if (!mesh)
		return 0;
	const int dim = mesh->getDimension() - 1;
	// fast case of field directly defined on element:
	// check first component only
	// note this formerly only used topLevelElement for inheritance only
	// i.e. would return initial element if it had field directly defined on it
	const bool useDefinitionOnThisElement = ((!topLevelElement) || (element == topLevelElement))
		&& (0 != this->meshFieldData[dim])
		&& (this->meshFieldData[dim]->getComponentMeshfieldtemplate(0)->getElementEFTIndex(element->getIndex()) >= 0);
	if (useDefinitionOnThisElement && (inheritFaceNumber < 0))
		return element;

	FE_value parentCoordinateTransformation[MAXIMUM_ELEMENT_XI_DIMENSIONS*MAXIMUM_ELEMENT_XI_DIMENSIONS];
	cmzn_element *fieldElement = 0;
	cmzn_element *parent = 0;
	int faceNumber = inheritFaceNumber;
	if (inheritFaceNumber >= 0)
	{
		parent = element;
		if (useDefinitionOnThisElement)
			fieldElement = element;
		else // inherit onto this element, then onto specified face below to handle two coordinate transformations
			fieldElement = this->getOrInheritOnElement(parent, /*inheritFaceNumber*/-1, topLevelElement, parentCoordinateTransformation);
	}
	else
	{
		const FE_mesh *parentMesh = mesh->getParentMesh();
		if (!parentMesh)
			return 0;
		// try to inherit field from any of the element's parents
		const DsLabelIndex *parents;
		const int parentsCount = mesh->getElementParents(element->getIndex(), parents);
		for (int p = 0; p < parentsCount; ++p)
		{
			parent = parentMesh->getElement(parents[p]);
			fieldElement = this->getOrInheritOnElement(parent, /*inheritFaceNumber*/-1, topLevelElement, parentCoordinateTransformation);
			if (fieldElement)
			{
				faceNumber = parentMesh->getElementFaceNumber(parents[p], element->getIndex());
				break;
			}
		}
	}
	if (!fieldElement)
		return 0;
	FE_element_shape *parentShape = parent->getElementShape();
	const FE_value *faceToElement = get_FE_element_shape_face_to_element(parentShape, faceNumber);
	if (!faceToElement)
	{
		display_message(ERROR_MESSAGE, "FE_field::getOrInheritOnElement.  Missing parent shape or invalid face number");
		return 0;
	}
	const int parentDimension = get_FE_element_shape_dimension(parentShape);
	const int fieldElementDimension = fieldElement->getDimension();
	if (fieldElementDimension > parentDimension)
	{
		// incorporate the face to element map in the coordinate transformation
		const FE_value *parentValue = parentCoordinateTransformation;
		FE_value *faceValue = coordinateTransformation;
		const int faceDimension = parentDimension - 1;
		// this had used DOUBLE_FOR_DOT_PRODUCT, but FE_value will be at least double precision now
		FE_value sum;
		const FE_value *faceToElementValue;
		for (int i = fieldElementDimension; i > 0; --i)
		{
			/* calculate b entry for this row */
			sum = *parentValue;
			++parentValue;
			faceToElementValue = faceToElement;
			for (int k = parentDimension; k > 0; --k)
			{
				sum += (*parentValue)*(*faceToElementValue);
				++parentValue;
				faceToElementValue += parentDimension;
			}
			*faceValue = sum;
			faceValue++;
			/* calculate A entries for this row */
			for (int j = faceDimension; j > 0; --j)
			{
				++faceToElement;
				faceToElementValue = faceToElement;
				parentValue -= parentDimension;
				sum = 0.0;
				for (int k = parentDimension; k > 0; --k)
				{
					sum += (*parentValue)*(*faceToElementValue);
					++parentValue;
					faceToElementValue += parentDimension;
				}
				*faceValue = sum;
				++faceValue;
			}
			faceToElement -= faceDimension;
		}
	}
	else
	{
		// use the face to element map as the transformation
		memcpy(coordinateTransformation, faceToElement, parentDimension*parentDimension*sizeof(FE_value));
	}
	return fieldElement;
}

PROTOTYPE_ENUMERATOR_STRING_FUNCTION(CM_field_type)
{
	const char *enumerator_string;

	ENTER(ENUMERATOR_STRING(CM_field_type));
	switch (enumerator_value)
	{
		case CM_ANATOMICAL_FIELD:
		{
			enumerator_string = "anatomical";
		} break;
		case CM_COORDINATE_FIELD:
		{
			enumerator_string = "coordinate";
		} break;
		case CM_GENERAL_FIELD:
		{
			enumerator_string = "field";
		} break;
		default:
		{
			enumerator_string = (const char *)NULL;
		} break;
	}
	LEAVE;

	return (enumerator_string);
} /* ENUMERATOR_STRING(CM_field_type) */

DEFINE_DEFAULT_ENUMERATOR_FUNCTIONS(CM_field_type)

char *FE_field::getComponentName(int componentIndex) const
{
	if ((0 <= componentIndex) && (componentIndex < this->number_of_components))
		return get_automatic_component_name(this->component_names, componentIndex);
	return nullptr;
}

bool FE_field::setComponentName(int componentIndex, const char *componentName)
{
	if ((0 <= componentIndex) && (componentIndex < this->number_of_components) && (componentName))
	{
		const char *tempName = this->getComponentName(componentIndex);
		const bool change = strcmp(tempName, componentName);
		DEALLOCATE(tempName);
		if (!change)
			return true;
		char *tempComponentName = duplicate_string(componentName);
		if (tempComponentName)
		{
			/* component_names array may be non-existent if default names used */
			if (this->component_names)
			{
				if (this->component_names[componentIndex])
				{
					DEALLOCATE(this->component_names[componentIndex]);
				}
			}
			else
			{
				if (ALLOCATE(this->component_names, char *, this->number_of_components))
				{
					for (int i = 0; i < this->number_of_components; ++i)
						this->component_names[i] = nullptr;
				}
			}
			if (this->component_names)
			{
				this->component_names[componentIndex] = tempComponentName;
				return true;
			}
		}
		display_message(ERROR_MESSAGE, "FE_field::setComponentName.  Failed");
		return false;
	}
	display_message(ERROR_MESSAGE, "FE_field::setComponentName.  Invalid argument(s)");
	return false;
}

FE_field_parameters *FE_field::get_FE_field_parameters()
{
	if (this->fe_field_parameters)
		return this->fe_field_parameters->access();
	this->fe_field_parameters = FE_field_parameters::create(this);
	return this->fe_field_parameters;
}

void FE_field::list() const
{
	display_message(INFORMATION_MESSAGE, "field : %s\n", this->name);
	display_message(INFORMATION_MESSAGE, "  access count = %d\n", this->access_count);
	display_message(INFORMATION_MESSAGE, "  type = %s",
		ENUMERATOR_STRING(CM_field_type)(this->cm_field_type));
	display_message(INFORMATION_MESSAGE, "  coordinate system = %s",
		ENUMERATOR_STRING(Coordinate_system_type)(this->coordinate_system.type));
	const int number_of_components = this->number_of_components;
	display_message(INFORMATION_MESSAGE, ", #Components = %d\n", number_of_components);
	for (int c = 0; c < this->number_of_components; ++c)
	{
		char *component_name = this->getComponentName(c);
		if (component_name)
		{
			display_message(INFORMATION_MESSAGE, "    %s", component_name);
			DEALLOCATE(component_name);
		}
		/* display field based information*/
		if (this->number_of_values)
		{
			int count;

			display_message(INFORMATION_MESSAGE, "field based values: ");
			switch (this->value_type)
			{
			case FE_VALUE_VALUE:
			{
				display_message(INFORMATION_MESSAGE, "\n");
				/* output in columns if FE_VALUE_MAX_OUTPUT_COLUMNS > 0 */
				for (count = 0; count < this->number_of_values; count++)
				{
					display_message(INFORMATION_MESSAGE, " %" FE_VALUE_STRING,
						*((FE_value*)(this->values_storage + count*sizeof(FE_value))));
					if ((0<FE_VALUE_MAX_OUTPUT_COLUMNS)&&
						(0==((count+1) % FE_VALUE_MAX_OUTPUT_COLUMNS)))
					{
						display_message(INFORMATION_MESSAGE, "\n");
					}
				}
			} break;
			default:
			{
				display_message(INFORMATION_MESSAGE, "list_FE_field: "
					"Can't display that field value_type yet. Write the code!");
			} break;
			}	/* switch () */
		}
		display_message(INFORMATION_MESSAGE, "\n");
	}
}

int list_FE_field(struct FE_field *field, void *)
{
	if (field)
	{
		field->list();
		return 1;
	}
	display_message(ERROR_MESSAGE, "list_FE_field.  Invalid argument");
	return 0;
}

bool FE_field::hasMultipleTimes()
{
	return FE_region_FE_field_has_multiple_times(this->fe_region, this);
}

PROTOTYPE_ACCESS_OBJECT_FUNCTION(FE_field)
{
	if (object)
		return object->access();
	return 0;
}

PROTOTYPE_DEACCESS_OBJECT_FUNCTION(FE_field)
{
	return FE_field::deaccess(object_address);
}

PROTOTYPE_REACCESS_OBJECT_FUNCTION(FE_field)
{
	if (object_address)
	{
		if (new_object)
		{
			new_object->access();
		}
		if (*object_address)
		{
			FE_field::deaccess(object_address);
		}
		*object_address = new_object;
		return 1;
	}
	return 0;
}

DECLARE_INDEXED_LIST_STL_FUNCTIONS(FE_field)
DECLARE_FIND_BY_IDENTIFIER_IN_INDEXED_LIST_STL_FUNCTION(FE_field,name,const char *)
DECLARE_INDEXED_LIST_STL_IDENTIFIER_CHANGE_FUNCTIONS(FE_field,name)

DECLARE_CHANGE_LOG_FUNCTIONS(FE_field)

int FE_field::copyProperties(FE_field *source)
{
	if (!(this->fe_region && source && source->fe_region))
	{
		display_message(ERROR_MESSAGE,
			"FE_field::copyProperties.  Invalid argument(s)");
		return 0;
	}
	int return_code = 1;
	const bool externalMerge = this->fe_region != source->fe_region;
	char **component_names=(char **)NULL;
	if (source->component_names)
	{
		ALLOCATE(component_names, char *, source->number_of_components);
		if (!component_names)
			return false;
		for (int c = 0; c < source->number_of_components; ++c)
		{
			if (source->component_names[c])
			{
				component_names[c] = duplicate_string(source->component_names[c]);
				if (!component_names[c])
				{
					for (int c2 = 0; c2 < c; ++c)
						DEALLOCATE(component_names[c2]);
					DEALLOCATE(component_names);
					return_code = 0;
					break;
				}
			}
			else
			{
				component_names[c] = 0;
			}
		}
	}
	Value_storage *new_values_storage = 0;
	if (0<source->number_of_values)
	{
		if (!((new_values_storage =make_value_storage_array(source->value_type,
			(struct FE_time_sequence *)NULL,source->number_of_values))&&
			copy_value_storage_array(new_values_storage,source->value_type,
				(struct FE_time_sequence *)NULL,(struct FE_time_sequence *)NULL,
				source->number_of_values,source->values_storage, /*optimised_merge*/0)))
		{
			return_code=0;
		}
	}
	FE_field *indexer_field = 0;
	if (INDEXED_FE_FIELD == source->fe_field_type)
	{
		if (!source->indexer_field)
			return_code = 0;
		else
		{
			if (externalMerge)
			{
				indexer_field = FE_region_get_FE_field_from_name(this->fe_region, source->indexer_field->name);
				if (!indexer_field)
					return_code = 0;
			}
			else
			{
				indexer_field = source->indexer_field;
			}
		}
	}
	FE_mesh *hostMesh = nullptr;
	if (ELEMENT_XI_VALUE == source->value_type)
	{
		if (!source->element_xi_host_mesh)
		{
			display_message(ERROR_MESSAGE,
				"FE_field::copyProperties.  Source element:xi valued field %s does not have a host mesh", source->name);
			return_code = 0;
		}
		else if (externalMerge)
		{
			// find equivalent host mesh in destination FE_region
			// to be fixed in future when arbitrary meshes are allowed:
			hostMesh = FE_region_find_FE_mesh_by_dimension(this->fe_region, source->element_xi_host_mesh->getDimension());
			if (strcmp(hostMesh->getName(), source->element_xi_host_mesh->getName()) != 0)
			{
				display_message(ERROR_MESSAGE,
					"FE_field::copyProperties.  Cannot find destination host mesh named %s for merging 'element:xi' valued field %s. Needs to be implemented.",
					source->element_xi_host_mesh->getName(), source->name);
				return_code = 0;
			}
		}
		else
		{
			hostMesh = source->element_xi_host_mesh;
		}
	}
	if (return_code)
	{
		// don't want to change info if merging to external region. In all other cases should be same info anyway
		if (!externalMerge)
			this->set_FE_region(source->get_FE_region());
		if (this->cm_field_type != source->cm_field_type)
		{
			display_message(WARNING_MESSAGE, "Changing field %s CM type from %s to %s",
				source->name, ENUMERATOR_STRING(CM_field_type)(this->cm_field_type),
				ENUMERATOR_STRING(CM_field_type)(source->cm_field_type));
			this->cm_field_type = source->cm_field_type;
		}
		this->fe_field_type=source->fe_field_type;
		REACCESS(FE_field)(&(this->indexer_field), indexer_field);
		this->number_of_indexed_values=
			source->number_of_indexed_values;
		if (this->component_names)
		{
			for (int i = 0; i < this->number_of_components; i++)
			{
				if (this->component_names[i])
				{
					DEALLOCATE(this->component_names[i]);
				}
			}
			DEALLOCATE(this->component_names);
		}
		this->number_of_components=source->number_of_components;
		this->component_names=component_names;
		this->coordinate_system = source->coordinate_system;
		this->value_type=source->value_type;
		/*
		if (hostMesh)
			hostMesh->access();
		if (this->element_xi_host_mesh)
		{
			FE_mesh::deaccess(this->element_xi_host_mesh);
		}
		this->element_xi_host_mesh = hostMesh;
		*/
		if (hostMesh)
			this->setElementXiHostMesh(hostMesh);
		/* replace old values_storage with new */
		if (0<this->number_of_values)
		{
			free_value_storage_array(this->values_storage,
				this->value_type,(struct FE_time_sequence *)NULL,
				this->number_of_values);
			DEALLOCATE(this->values_storage);
		}
		this->number_of_values=source->number_of_values;
		this->values_storage= new_values_storage;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_field::copyProperties.  Invalid source field or could not copy dynamic contents");
	}
	if (!return_code)
	{
		if (component_names)
		{
			for (int i=0;i<source->number_of_components;i++)
			{
				if (component_names[i])
				{
					DEALLOCATE(component_names[i]);
				}
			}
			DEALLOCATE(component_names);
		}
		if (new_values_storage)
		{
			free_value_storage_array(new_values_storage,source->value_type,
				(struct FE_time_sequence *)NULL,source->number_of_values);
			DEALLOCATE(new_values_storage);
		}
	}
	return (return_code);
}

bool FE_fields_match_fundamental(struct FE_field *field1,
	struct FE_field *field2)
{
	if (!(field1 && field2))
	{
		display_message(ERROR_MESSAGE,
			"FE_fields_match_fundamental.  Missing field(s)");
		return false;
	}
	if (!((field1->value_type == field2->value_type)
		&& (field1->fe_field_type == field2->fe_field_type)
		&& (field1->number_of_components == field2->number_of_components)
		&& (0 != Coordinate_systems_match(&(field1->coordinate_system),
			&(field2->coordinate_system)))))
		return false;
	if (ELEMENT_XI_VALUE == field1->value_type)
	{
		if (!field1->element_xi_host_mesh)
		{
			display_message(ERROR_MESSAGE,
				"FE_fields_match_fundamental.  Source element xi field %s does not have a host mesh", field1->name);
			return false;
		}
		// This will need to be improved once multiple meshes or meshes from different regions are allowed:
		if (field1->element_xi_host_mesh->getDimension() != field2->element_xi_host_mesh->getDimension())
			return false;
	}
	return true;
}

bool FE_fields_match_exact(struct FE_field *field1, struct FE_field *field2)
{
	if (field1 && field2)
	{
		// does not match until proven so
		if ((0 == strcmp(field1->name, field2->name)) &&
			(field1->fe_field_type == field2->fe_field_type) &&
			((INDEXED_FE_FIELD != field1->fe_field_type) ||
				(field1->indexer_field && field2->indexer_field &&
					(0 == strcmp(field1->indexer_field->name,
						field2->indexer_field->name)) &&
					FE_fields_match_fundamental(field1->indexer_field,
						field2->indexer_field) &&
					(field1->number_of_indexed_values ==
						field2->number_of_indexed_values))) &&
			(field1->cm_field_type == field2->cm_field_type) &&
			Coordinate_systems_match(&(field1->coordinate_system),
				&(field2->coordinate_system)) &&
			(field1->value_type == field2->value_type) &&
			(field1->number_of_components == field2->number_of_components))
		{
			// matches until disproven
			// check component names match
			for (int i = field1->number_of_components; i <= 0; --i)
			{
				char *component_name1 = get_automatic_component_name(field1->component_names, i);
				char *component_name2 = get_automatic_component_name(field2->component_names, i);
				bool matching = (component_name1) && (component_name2) &&
					(0 == strcmp(component_name1, component_name2));
				DEALLOCATE(component_name2);
				DEALLOCATE(component_name1);
				if (!matching)
					return false;
			}
			return true;
		}
	}
	else
		display_message(ERROR_MESSAGE, "FE_fields_match_exact.  Missing field(s)");
	return false;
}

/**
 * List iterator function which fetches a field with the same name as <field>
 * from <field_list>. Returns 1 (true) if there is either no such field in the
 * list or the two fields return true for FE_fields_match_fundamental(),
 * otherwise returns 0 (false).
 */
int FE_field_can_be_merged_into_list(struct FE_field *field, void *field_list_void)
{
	struct LIST(FE_field) *field_list = reinterpret_cast<struct LIST(FE_field) *>(field_list_void);
	if (field && field_list)
	{
		FE_field *other_field = FIND_BY_IDENTIFIER_IN_LIST(FE_field,name)(field->getName(), field_list);
		if ((!(other_field)) || FE_fields_match_fundamental(field, other_field))
			return 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_field_can_be_merged_into_list.  Invalid argument(s)");
	}
	return 0;
}

bool FE_field::usesNonlinearBasis() const
{
	for (int d = 0; d < MAXIMUM_ELEMENT_XI_DIMENSIONS; ++d)
		if ((this->meshFieldData[d]) && this->meshFieldData[d]->usesNonLinearBasis())
			return true;
	return false;
}

int ensure_FE_field_is_in_list(struct FE_field *field, void *field_list_void)
/*******************************************************************************
LAST MODIFIED : 29 March 2006

DESCRIPTION :
Iterator function for adding <field> to <field_list> if not currently in it.
==============================================================================*/
{
	int return_code;
	struct LIST(FE_field) *field_list;

	ENTER(ensure_FE_field_is_in_list);
	if (field&&(field_list=(struct LIST(FE_field) *)field_list_void))
	{
		if (!IS_OBJECT_IN_LIST(FE_field)(field,field_list))
		{
			return_code=ADD_OBJECT_TO_LIST(FE_field)(field,field_list);
		}
		else
		{
			return_code=1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"ensure_FE_field_is_in_list.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* ensure_FE_field_is_in_list */

char *get_FE_field_component_name(struct FE_field *field, int component_no)
{
	if (field)
		return field->getComponentName(component_no);
	display_message(ERROR_MESSAGE, "get_FE_field_component_name.  Invalid argument(s)");
	return nullptr;
}

int set_FE_field_component_name(struct FE_field *field,int component_no,
	const char *component_name)
{
	if ((field) && (component_name))
		return field->setComponentName(component_no, component_name);
	display_message(ERROR_MESSAGE, "set_FE_field_component_name.  Invalid argument(s)");
	return 0;
}

int set_FE_field_coordinate_system(struct FE_field *field,
	const Coordinate_system *coordinate_system)
{
	if ((field) && (coordinate_system))
	{
		field->setCoordinateSystem(*coordinate_system);
		return 1;
	}
	display_message(ERROR_MESSAGE,
		"set_FE_field_coordinate_system.  Invalid argument(s)");
	return 0;
}

void FE_field::clearMeshFieldData(FE_mesh *mesh)
{
	if (mesh && (mesh->get_FE_region() == this->fe_region))
	{
		const int dim = mesh->getDimension() - 1;
		if (this->meshFieldData[dim])
		{
			delete this->meshFieldData[dim];
			this->meshFieldData[dim] = 0;
			this->fe_region->FE_field_change(this, CHANGE_LOG_RELATED_OBJECT_CHANGED(FE_field));
		}
	}
}

FE_mesh_field_data *FE_field::createMeshFieldData(FE_mesh *mesh)
{
	// GRC check field type general, real/int only?
	if (mesh && (mesh->get_FE_region() == this->fe_region))
	{
		const int dim = mesh->getDimension() - 1;
		if (!this->meshFieldData[dim])
			this->meshFieldData[dim] = FE_mesh_field_data::create(this, mesh);
		return this->meshFieldData[dim];
	}
	return nullptr;
}

int get_FE_field_number_of_components(struct FE_field *field)
{
	if (field)
		return field->getNumberOfComponents();
	display_message(ERROR_MESSAGE,
		"get_FE_field_number_of_components.  Missing field");
	return 0;
}

int set_FE_field_number_of_components(struct FE_field *field,
	int number_of_components)
/*******************************************************************************
LAST MODIFIED : 16 January 2002

DESCRIPTION :
Sets the number of components in the <field>. Automatically assumes names for
any new components. Clears/reallocates the values_storage for FE_field_types
that use them, eg. CONSTANT_FE_FIELD and INDEXED_FE_FIELD - but only if number
of components changes. If function fails the field is left exactly as it was.
Should only call this function for unmanaged fields.
ELEMENT_XI_VALUE, STRING_VALUE and URL_VALUE fields may only have 1 component.
==============================================================================*/
{
	char **component_names;
	int i,number_of_values,return_code;
	Value_storage *values_storage;

	ENTER(set_FE_field_number_of_components);
	if (field && (0<number_of_components) && ((1 == number_of_components) ||
		((field->value_type != ELEMENT_XI_VALUE) && (field->value_type != STRING_VALUE) &&
			(field->value_type != URL_VALUE))))
	{
		return_code=1;
		if (number_of_components != field->number_of_components)
		{
			/* 1. make dynamic allocations for number_of_components-specific data */
			component_names=(char **)NULL;
			if (field->component_names)
			{
				if (ALLOCATE(component_names,char *,number_of_components))
				{
					/* copy the old names, clear any new ones */
					for (i=0;i<number_of_components;i++)
					{
						if (i<field->number_of_components)
						{
							component_names[i]=field->component_names[i];
						}
						else
						{
							component_names[i]=(char *)NULL;
						}
					}
				}
				else
				{
					return_code=0;
				}
			}
			values_storage=(Value_storage *)NULL;
			number_of_values=0;
			switch (field->fe_field_type)
			{
				case CONSTANT_FE_FIELD:
				{
					number_of_values=number_of_components;
				} break;
				case GENERAL_FE_FIELD:
				{
					number_of_values=0;
				} break;
				case INDEXED_FE_FIELD:
				{
					number_of_values=field->number_of_indexed_values*number_of_components;
				} break;
				default:
				{
					return_code=0;
				} break;
			}
			if (number_of_values != field->number_of_values)
			{
				if (!(values_storage=make_value_storage_array(field->value_type,
					(struct FE_time_sequence *)NULL,number_of_values)))
				{
					return_code=0;
				}
			}
			if (return_code)
			{
				/* 2. free current number_of_components-specific data */
				if (field->component_names)
				{
					/* free component_names no longer used */
					for (i = number_of_components; i < field->number_of_components; i++)
					{
						if (field->component_names[i])
						{
							DEALLOCATE(field->component_names[i]);
						}
					}
					DEALLOCATE(field->component_names);
				}
				if (field->values_storage)
				{
					free_value_storage_array(field->values_storage,field->value_type,
						(struct FE_time_sequence *)NULL,field->number_of_values);
					DEALLOCATE(field->values_storage);
				}
				/* 3. establish the new number_of_components and associated data */
				field->number_of_components=number_of_components;
				field->component_names=component_names;
				field->values_storage=values_storage;
				field->number_of_values=number_of_values;
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"set_FE_field_number_of_components.  Not enough memory");
				DEALLOCATE(component_names);
				DEALLOCATE(values_storage);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"set_FE_field_number_of_components.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* set_FE_field_number_of_components */

enum CM_field_type get_FE_field_CM_field_type(struct FE_field *field)
{
	if (field)
		return field->get_CM_field_type();
	display_message(ERROR_MESSAGE, "get_FE_field_CM_field_type.  Invalid field");
	return CM_GENERAL_FIELD;
}

int set_FE_field_CM_field_type(struct FE_field *field,
	enum CM_field_type cm_field_type)
{
	if (field)
	{
		field->set_CM_field_type(cm_field_type);
		return 1;
	}
	display_message(ERROR_MESSAGE, "set_FE_field_CM_field_type.  Invalid argument(s)");
	return 0;
}

enum FE_field_type get_FE_field_FE_field_type(struct FE_field *field)
{
	if (field)
		return field->get_FE_field_type();
	display_message(ERROR_MESSAGE, "get_FE_field_FE_field_type.  Invalid field");
	return UNKNOWN_FE_FIELD;
}

int set_FE_field_type_constant(struct FE_field *field)
/*******************************************************************************
LAST MODIFIED : 1 September 1999

DESCRIPTION :
Converts the <field> to type CONSTANT_FE_FIELD.
Allocates and clears the values_storage of the field to fit
field->number_of_components of the current value_type.
If function fails the field is left exactly as it was.
Should only call this function for unmanaged fields.
==============================================================================*/
{
	int number_of_values, return_code;
	Value_storage *values_storage;

	ENTER(set_FE_field_type_constant);
	return_code = 0;
	if (field)
	{
		if (field->getValueType() == ELEMENT_XI_VALUE)
		{
			display_message(ERROR_MESSAGE,
				"set_FE_field_type_constant.  Not implemented for ELEMENT_XI_VALUE");
			return 0;
		}
		/* 1. make dynamic allocations for any new type-specific data */
		number_of_values = field->number_of_components;
		if (NULL != (values_storage = make_value_storage_array(field->value_type,
			(struct FE_time_sequence *)NULL, number_of_values)))
		{
			/* 2. free current type-specific data */
			if (field->values_storage)
			{
				free_value_storage_array(field->values_storage, field->value_type,
					(struct FE_time_sequence *)NULL, field->number_of_values);
				DEALLOCATE(field->values_storage);
			}
			REACCESS(FE_field)(&(field->indexer_field), NULL);
			field->number_of_indexed_values = 0;
			/* 3. establish the new type */
			field->fe_field_type = CONSTANT_FE_FIELD;
			field->values_storage = values_storage;
			field->number_of_values = number_of_values;
			return_code = 1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"set_FE_field_type_constant.  Could not allocate values_storage");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"set_FE_field_type_constant.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* set_FE_field_type_constant */

int set_FE_field_type_general(struct FE_field *field)
/*******************************************************************************
LAST MODIFIED : 1 September 1999

DESCRIPTION :
Converts the <field> to type GENERAL_FE_FIELD.
Frees any values_storage currently in use by the field.
If function fails the field is left exactly as it was.
Should only call this function for unmanaged fields.
==============================================================================*/
{
	int return_code;

	ENTER(set_FE_field_type_general);
	return_code = 0;
	if (field)
	{
		/* 1. make dynamic allocations for any new type-specific data */
		/* none */
		/* 2. free current type-specific data */
		if (field->values_storage)
		{
			free_value_storage_array(field->values_storage, field->value_type,
				(struct FE_time_sequence *)NULL, field->number_of_values);
			DEALLOCATE(field->values_storage);
		}
		REACCESS(FE_field)(&(field->indexer_field), NULL);
		field->number_of_indexed_values = 0;
		/* 3. establish the new type */
		field->fe_field_type = GENERAL_FE_FIELD;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"set_FE_field_type_general.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* set_FE_field_type_general */

int get_FE_field_type_indexed(struct FE_field *field,
	struct FE_field **indexer_field, int *number_of_indexed_values)
/*******************************************************************************
LAST MODIFIED : 1 September 1999

DESCRIPTION :
If the field is of type INDEXED_FE_FIELD, the indexer_field and
number_of_indexed_values it uses are returned - otherwise an error is reported.
Use function FE_field::get_FE_field_type to determine the field type.
==============================================================================*/
{
	int return_code;

	ENTER(get_FE_field_type_indexed);
	if (field&&(INDEXED_FE_FIELD==field->fe_field_type)&&indexer_field&&
		number_of_indexed_values)
	{
		*indexer_field = field->indexer_field;
		*number_of_indexed_values = field->number_of_indexed_values;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"get_FE_field_type_indexed.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* get_FE_field_type_indexed */

int set_FE_field_type_indexed(struct FE_field *field,
	struct FE_field *indexer_field, int number_of_indexed_values)
/*******************************************************************************
LAST MODIFIED : 2 September 1999

DESCRIPTION :
Converts the <field> to type INDEXED_FE_FIELD, indexed by the given
<indexer_field> and with the given <number_of_indexed_values>. The indexer_field
must return a single integer value to be valid.
Allocates and clears the values_storage of the field to fit
field->number_of_components x number_of_indexed_values of the current
value_type. If function fails the field is left exactly as it was.
Should only call this function for unmanaged fields.
==============================================================================*/
{
	int number_of_values, return_code;
	Value_storage *values_storage;

	ENTER(set_FE_field_type_indexed);
	return_code = 0;
	if (field&&indexer_field&&(0<number_of_indexed_values)&&
		(1==get_FE_field_number_of_components(indexer_field))&&
		(INT_VALUE==get_FE_field_value_type(indexer_field))&&
		/* and to avoid possible endless loops... */
		(indexer_field != field) &&
		(INDEXED_FE_FIELD != get_FE_field_FE_field_type(indexer_field)))
	{
		if (field->getValueType() == ELEMENT_XI_VALUE)
		{
			display_message(ERROR_MESSAGE,
				"set_FE_field_type_indexed.  Not implemented for ELEMENT_XI_VALUE");
			return 0;
		}
		/* 1. make dynamic allocations for any new type-specific data */
		number_of_values = field->number_of_components*number_of_indexed_values;
		if (NULL != (values_storage = make_value_storage_array(field->value_type,
			(struct FE_time_sequence *)NULL, number_of_values)))
		{
			/* 2. free current type-specific data */
			if (field->values_storage)
			{
				free_value_storage_array(field->values_storage, field->value_type,
					(struct FE_time_sequence *)NULL, field->number_of_values);
				DEALLOCATE(field->values_storage);
			}
			/* 3. establish the new type */
			field->fe_field_type = INDEXED_FE_FIELD;
			REACCESS(FE_field)(&(field->indexer_field), indexer_field);
			field->number_of_indexed_values = number_of_indexed_values;
			field->values_storage = values_storage;
			field->number_of_values = number_of_values;
			return_code = 1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"set_FE_field_type_indexed.  Could not allocate values_storage");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"set_FE_field_type_indexed.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* set_FE_field_type_indexed */

enum Value_type get_FE_field_value_type(struct FE_field *field)
{
	if (field)
		return field->getValueType();
	display_message(ERROR_MESSAGE, "get_FE_field_value_type.  Invalid field");
	return UNKNOWN_VALUE;
}

int set_FE_field_value_type(struct FE_field *field, enum Value_type value_type)
/*******************************************************************************
LAST MODIFIED : 1 September 1999

DESCRIPTION :
Sets the value_type of the <field>. Clears/reallocates the values_storage for
FE_field_types that use them, eg. CONSTANT_FE_FIELD and INDEXED_FE_FIELD - but
only if the value_type changes. If function fails the field is left exactly as
it was. Should only call this function for unmanaged fields.
ELEMENT_XI_VALUE, STRING_VALUE and URL_VALUE fields may only have 1 component.
==============================================================================*/
{
	int number_of_values, return_code;
	Value_storage *values_storage;

	ENTER(set_FE_field_value_type);
	if (field && ((1 >= field->number_of_components) ||
		((value_type != ELEMENT_XI_VALUE) && (value_type != STRING_VALUE) &&
		(value_type != URL_VALUE))))
	{
		return_code = 1;
		if (value_type != field->value_type)
		{
			/* 1. make dynamic allocations for value_type-specific data */
			values_storage = (Value_storage *)NULL;
			number_of_values = field->number_of_values;
			if (0!=number_of_values)
			{
				if (!(values_storage = make_value_storage_array(value_type,
					(struct FE_time_sequence *)NULL, number_of_values)))
				{
					return_code = 0;
				}
			}
			if (return_code)
			{
				/* 2. free current value_type-specific data */
				if (field->values_storage)
				{
					free_value_storage_array(field->values_storage, field->value_type,
						(struct FE_time_sequence *)NULL, field->number_of_values);
					DEALLOCATE(field->values_storage);
				}
				/* 3. establish the new value_type and associated data */
				field->value_type = value_type;
				field->values_storage = values_storage;
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"set_FE_field_value_type.  Not enough memory");
				DEALLOCATE(values_storage);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"set_FE_field_value_type.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
}/* set_FE_field_value_type */

int FE_field::setElementXiHostMesh(FE_mesh *hostMesh)
{
	if (!((this->value_type == ELEMENT_XI_VALUE) && (hostMesh)
		&& (hostMesh->get_FE_region() == this->fe_region))) // Current limitation of same region can be removed later
	{
		display_message(ERROR_MESSAGE, "FE_field::setElementXiHostMesh.  Invalid arguments");
		return CMZN_ERROR_ARGUMENT;
	}
	if (this->element_xi_host_mesh)
	{
		display_message(ERROR_MESSAGE, "FE_field::setElementXiHostMesh.  Host mesh is already set");
		return CMZN_ERROR_ALREADY_EXISTS;
	}
	this->element_xi_host_mesh = hostMesh->access();
	for (int i = 0; i < 2; ++i)
		this->embeddedNodeFields[i] = hostMesh->addEmbeddedNodeField(this, this->fe_region->nodesets[i]);
	return CMZN_OK;
}

FE_mesh_embedded_node_field *FE_field::getEmbeddedNodeField(FE_nodeset *nodeset) const
{
	if (this->value_type == ELEMENT_XI_VALUE)
		return this->embeddedNodeFields[(nodeset->getFieldDomainType() == CMZN_FIELD_DOMAIN_TYPE_NODES) ? 0 : 1];
	return nullptr;
}

int set_FE_field_string_value(struct FE_field *field, int value_number,
	char *string)
/*******************************************************************************
LAST MODIFIED : 22 September 1999

DESCRIPTION :
Copies and sets the <string> stored at <value_number> in the <field>.
<string> may be NULL.
==============================================================================*/
{
	int return_code, size;
	char *the_string, **string_address;

	ENTER(set_FE_field_string_value);
	return_code = 0;
	if (field && (field->value_type == STRING_VALUE) && (0<=value_number)&&(value_number<field->number_of_values))
	{
		/* get the pointer to the stored string */
		size = get_Value_storage_size(STRING_VALUE, (struct FE_time_sequence *)NULL);
		string_address = (char **)(field->values_storage+value_number*size);
		if (string)
		{
			/* reallocate the string currently there */
			if (REALLOCATE(the_string, *string_address, char, strlen(string)+1))
			{
				strcpy(the_string, string);
				*string_address = the_string;
				return_code = 1;
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"set_FE_field_string_value.  Not enough memory");
			}
		}
		else
		{
			/* NULL string; free the existing string */
			if (*string_address)
			{
				DEALLOCATE(*string_address);
			}
			return_code = 1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"set_FE_field_string_value.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* set_FE_field_string_value */

int set_FE_field_FE_value_value(struct FE_field *field, int number,
	FE_value value)
/*******************************************************************************
LAST MODIFIED : 2 September 1999

DESCRIPTION :
Sets the specified global FE_value <value> in <field>.
The <field> must be of type FE_VALUE_VALUE to have such values and
<number> must be within the range from FE_field::getNumberOfValues.
==============================================================================*/
{
	int return_code;

	ENTER(set_FE_field_FE_value_value);
	if (field&&(FE_VALUE_VALUE==field->value_type)&&field->values_storage&&
		(0<=number)&&(number<=field->number_of_values))
	{
		*((FE_value *)(field->values_storage+(number*sizeof(FE_value)))) = value;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"set_FE_field_FE_value_value.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* set_FE_field_FE_value_value */

int set_FE_field_int_value(struct FE_field *field, int number, int value)
/*******************************************************************************
LAST MODIFIED : 2 September 1999

DESCRIPTION :
Sets the specified global int <value> in <field>.
The <field> must be of type INT_VALUE to have such values and
<number> must be within the range from FE_field::getNumberOfValues.
==============================================================================*/
{
	int return_code;

	ENTER(set_FE_field_int_value);
	if (field&&(INT_VALUE==field->value_type)&&field->values_storage&&
		(0<=number)&&(number<=field->number_of_values))
	{
		*((int *)(field->values_storage+(number*sizeof(int)))) = value;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"set_FE_field_int_value.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* set_FE_field_int_value */

const char *get_FE_field_name(struct FE_field *field)
{
	if (field)
		return field->getName();
	display_message(ERROR_MESSAGE, "get_FE_field_name.  Invalid argument(s)");
	return nullptr;
}

int FE_field_is_1_component_integer(struct FE_field *field,void *dummy_void)
/*******************************************************************************
LAST MODIFIED : 18 May 2000

DESCRIPTION :
Conditional function returning true if <field> has exactly 1 component and a
value type of integer.
This type of field is used for storing eg. grid_point_number.
==============================================================================*/
{
	int return_code;

	ENTER(FE_field_is_1_component_integer);
	USE_PARAMETER(dummy_void);
	if (field)
	{
		return_code=(INT_VALUE==field->getValueType())&&
			(1 == field->getNumberOfComponents());
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_field_is_1_component_integer.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* FE_field_is_1_component_integer */

int FE_field_is_coordinate_field(struct FE_field *field,void *dummy_void)
{
	USE_PARAMETER(dummy_void);
	if (field && field->isTypeCoordinate())
		return 1;
	return 0;
}
