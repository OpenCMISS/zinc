/*******************************************************************************
FILE : finite_element_conversion.cpp

LAST MODIFIED : 5 April 2006

DESCRIPTION :
Functions for converting one finite_element representation to another.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/* for IGES */
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "opencmiss/zinc/core.h"
#include "opencmiss/zinc/element.h"
#include "opencmiss/zinc/elementbasis.h"
#include "opencmiss/zinc/elementtemplate.h"
#include "opencmiss/zinc/field.h"
#include "opencmiss/zinc/fieldcache.h"
#include "opencmiss/zinc/fieldfiniteelement.h"
#include "opencmiss/zinc/fieldmodule.h"
#include "opencmiss/zinc/mesh.h"
#include "opencmiss/zinc/node.h"
#include "opencmiss/zinc/nodeset.h"
#include "opencmiss/zinc/nodetemplate.h"
#include "opencmiss/zinc/region.h"
#include "opencmiss/zinc/status.h"
#include "computed_field/computed_field.h"
#include "computed_field/field_cache.hpp"
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_conversion.h"
#include "general/debug.h"
#include "general/octree.h"
#include "general/enumerator_private.hpp"
#include "general/message.h"
#include "general/mystring.h"

/*
Module types
------------
*/

const int HERMITE_2D_NUMBER_OF_NODES = 4;
const int BICUBIC_NUMBER_OF_NODES = 16;
const int TRILINEAR_NUMBER_OF_NODES = 8;
const int TRIQUADRATIC_NUMBER_OF_NODES = 27;
const int TRICUBIC_NUMBER_OF_NODES = 64;
const int MAX_NUMBER_OF_NODES = 64;

const FE_value_triple destination_xi_bicubic_hermite[HERMITE_2D_NUMBER_OF_NODES] =
{
	{ 0.0, 0.0, 0.0 },{ 1.0, 0.0, 0.0 },{ 0.0, 1.0, 0.0 },{ 1.0, 1.0, 0.0 }
};

const FE_value_triple destination_xi_bicubic[BICUBIC_NUMBER_OF_NODES] =
{
	{ 0.0, 0.0    , 0.0 },{ 1.0/3.0, 0.0    , 0.0 },{ 2.0/3.0, 0.0    , 0.0 },{ 1.0, 0.0    , 0.0 },
	{ 0.0, 1.0/3.0, 0.0 },{ 1.0/3.0, 1.0/3.0, 0.0 },{ 2.0/3.0, 1.0/3.0, 0.0 },{ 1.0, 1.0/3.0, 0.0 },
	{ 0.0, 2.0/3.0, 0.0 },{ 1.0/3.0, 2.0/3.0, 0.0 },{ 2.0/3.0, 2.0/3.0, 0.0 },{ 1.0, 2.0/3.0, 0.0 },
	{ 0.0, 1.0    , 0.0 },{ 1.0/3.0, 1.0    , 0.0 },{ 2.0/3.0, 1.0    , 0.0 },{ 1.0, 1.0    , 0.0 },
};

const FE_value_triple destination_xi_trilinear[TRILINEAR_NUMBER_OF_NODES] =
{
	{ 0.0, 0.0, 0.0 },{ 1.0, 0.0, 0.0 },{ 0.0, 1.0, 0.0 },{ 1.0, 1.0, 0.0 },
	{ 0.0, 0.0, 1.0 },{ 1.0, 0.0, 1.0 },{ 0.0, 1.0, 1.0 },{ 1.0, 1.0, 1.0 }
};

const FE_value_triple destination_xi_triquadratic[TRIQUADRATIC_NUMBER_OF_NODES] =
{
	{ 0.0, 0.0, 0.0 },{ 0.5, 0.0, 0.0 },{ 1.0, 0.0, 0.0 },
	{ 0.0, 0.5, 0.0 },{ 0.5, 0.5, 0.0 },{ 1.0, 0.5, 0.0 },
	{ 0.0, 1.0, 0.0 },{ 0.5, 1.0, 0.0 },{ 1.0, 1.0, 0.0 },

	{ 0.0, 0.0, 0.5 },{ 0.5, 0.0, 0.5 },{ 1.0, 0.0, 0.5 },
	{ 0.0, 0.5, 0.5 },{ 0.5, 0.5, 0.5 },{ 1.0, 0.5, 0.5 },
	{ 0.0, 1.0, 0.5 },{ 0.5, 1.0, 0.5 },{ 1.0, 1.0, 0.5 },

	{ 0.0, 0.0, 1.0 },{ 0.5, 0.0, 1.0 },{ 1.0, 0.0, 1.0 },
	{ 0.0, 0.5, 1.0 },{ 0.5, 0.5, 1.0 },{ 1.0, 0.5, 1.0 },
	{ 0.0, 1.0, 1.0 },{ 0.5, 1.0, 1.0 },{ 1.0, 1.0, 1.0 },
};

const FE_value_triple destination_xi_tricubic[TRICUBIC_NUMBER_OF_NODES] =
{
	{ 0.0, 0.0    , 0.0     },{ 1.0/3.0, 0.0    , 0.0     },{ 2.0/3.0, 0.0    , 0.0     },{ 1.0, 0.0    , 0.0     },
	{ 0.0, 1.0/3.0, 0.0     },{ 1.0/3.0, 1.0/3.0, 0.0     },{ 2.0/3.0, 1.0/3.0, 0.0     },{ 1.0, 1.0/3.0, 0.0     },
	{ 0.0, 2.0/3.0, 0.0     },{ 1.0/3.0, 2.0/3.0, 0.0     },{ 2.0/3.0, 2.0/3.0, 0.0     },{ 1.0, 2.0/3.0, 0.0     },
	{ 0.0, 1.0    , 0.0     },{ 1.0/3.0, 1.0    , 0.0     },{ 2.0/3.0, 1.0    , 0.0     },{ 1.0, 1.0    , 0.0     },

	{ 0.0, 0.0    , 1.0/3.0 },{ 1.0/3.0, 0.0    , 1.0/3.0 },{ 2.0/3.0, 0.0    , 1.0/3.0 },{ 1.0, 0.0    , 1.0/3.0 },
	{ 0.0, 1.0/3.0, 1.0/3.0 },{ 1.0/3.0, 1.0/3.0, 1.0/3.0 },{ 2.0/3.0, 1.0/3.0, 1.0/3.0 },{ 1.0, 1.0/3.0, 1.0/3.0 },
	{ 0.0, 2.0/3.0, 1.0/3.0 },{ 1.0/3.0, 2.0/3.0, 1.0/3.0 },{ 2.0/3.0, 2.0/3.0, 1.0/3.0 },{ 1.0, 2.0/3.0, 1.0/3.0 },
	{ 0.0, 1.0    , 1.0/3.0 },{ 1.0/3.0, 1.0    , 1.0/3.0 },{ 2.0/3.0, 1.0    , 1.0/3.0 },{ 1.0, 1.0    , 1.0/3.0 },

	{ 0.0, 0.0    , 2.0/3.0 },{ 1.0/3.0, 0.0    , 2.0/3.0 },{ 2.0/3.0, 0.0    , 2.0/3.0 },{ 1.0, 0.0    , 2.0/3.0 },
	{ 0.0, 1.0/3.0, 2.0/3.0 },{ 1.0/3.0, 1.0/3.0, 2.0/3.0 },{ 2.0/3.0, 1.0/3.0, 2.0/3.0 },{ 1.0, 1.0/3.0, 2.0/3.0 },
	{ 0.0, 2.0/3.0, 2.0/3.0 },{ 1.0/3.0, 2.0/3.0, 2.0/3.0 },{ 2.0/3.0, 2.0/3.0, 2.0/3.0 },{ 1.0, 2.0/3.0, 2.0/3.0 },
	{ 0.0, 1.0    , 2.0/3.0 },{ 1.0/3.0, 1.0    , 2.0/3.0 },{ 2.0/3.0, 1.0    , 2.0/3.0 },{ 1.0, 1.0    , 2.0/3.0 },

	{ 0.0, 0.0    , 1.0     },{ 1.0/3.0, 0.0    , 1.0     },{ 2.0/3.0, 0.0    , 1.0     },{ 1.0, 0.0    , 1.0     },
	{ 0.0, 1.0/3.0, 1.0     },{ 1.0/3.0, 1.0/3.0, 1.0     },{ 2.0/3.0, 1.0/3.0, 1.0     },{ 1.0, 1.0/3.0, 1.0     },
	{ 0.0, 2.0/3.0, 1.0     },{ 1.0/3.0, 2.0/3.0, 1.0     },{ 2.0/3.0, 2.0/3.0, 1.0     },{ 1.0, 2.0/3.0, 1.0     },
	{ 0.0, 1.0    , 1.0     },{ 1.0/3.0, 1.0    , 1.0     },{ 2.0/3.0, 1.0    , 1.0     },{ 1.0, 1.0    , 1.0     },
};

struct Convert_finite_elements_data
{
	enum Convert_finite_elements_mode mode;
	const int mode_dimension;
	cmzn_region_id source_region;
	cmzn_fieldmodule_id source_fieldmodule;
	cmzn_mesh_id source_mesh;
	cmzn_fieldcache_id source_fieldcache;
	Element_refinement refinement;
	FE_value tolerance;
	struct Octree *octree;
	struct LIST(Octree_object) *nearby_nodes;
	cmzn_region_id destination_region;
	cmzn_fieldmodule_id destination_fieldmodule;
	cmzn_nodeset_id destination_nodeset;
	cmzn_mesh_id destination_mesh;
	cmzn_fieldcache_id destination_fieldcache;
	cmzn_nodetemplate_id nodetemplate;
	cmzn_elementtemplate_id elementtemplate;
	int number_of_fields;
	cmzn_field_id *source_fields;
	cmzn_field_finite_element_id *destination_fields;
	int maximum_number_of_components;
	FE_value *temporary_values;
	int subelement_count;
	FE_value delta_xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	int number_of_local_nodes;
	const FE_value_triple *destination_xi;

	Convert_finite_elements_data(cmzn_region_id source_regionIn,
			Convert_finite_elements_mode modeIn,
			Element_refinement refinementIn, FE_value toleranceIn,
			cmzn_region_id destination_regionIn) :
		mode(modeIn),
		mode_dimension(((CONVERT_TO_FINITE_ELEMENTS_HERMITE_2D_PRODUCT == this->mode)
			|| (CONVERT_TO_FINITE_ELEMENTS_BICUBIC == this->mode)) ? 2 : 3),
		source_region(cmzn_region_access(source_regionIn)),
		source_fieldmodule(cmzn_region_get_fieldmodule(source_region)),
		source_mesh(cmzn_fieldmodule_find_mesh_by_dimension(source_fieldmodule, this->mode_dimension)),
		source_fieldcache(cmzn_fieldmodule_create_fieldcache(source_fieldmodule)),
		refinement(refinementIn),
		tolerance(toleranceIn),
		octree(CREATE(Octree)()),
		nearby_nodes(CREATE(LIST(Octree_object))()),
		destination_region(cmzn_region_access(destination_regionIn)),
		destination_fieldmodule(cmzn_region_get_fieldmodule(destination_region)),
		destination_nodeset(cmzn_fieldmodule_find_nodeset_by_field_domain_type(destination_fieldmodule, CMZN_FIELD_DOMAIN_TYPE_NODES)),
		destination_mesh(cmzn_fieldmodule_find_mesh_by_dimension(destination_fieldmodule, this->mode_dimension)),
		destination_fieldcache(cmzn_fieldmodule_create_fieldcache(destination_fieldmodule)),
		nodetemplate(0),
		elementtemplate(0),
		number_of_fields(0),
		source_fields(0),
		destination_fields(0),
		maximum_number_of_components(0),
		temporary_values((FE_value *)NULL),
		subelement_count(1),
		number_of_local_nodes(0),
		destination_xi(0)
	{
		for (int i = 0; i < this->mode_dimension; i++)
		{
			this->subelement_count *= this->refinement.count[i];
			delta_xi[i] = 1.0 / (FE_value)(this->refinement.count[i]);
		}

		switch (this->mode)
		{
		case CONVERT_TO_FINITE_ELEMENTS_HERMITE_2D_PRODUCT:
			this->number_of_local_nodes = HERMITE_2D_NUMBER_OF_NODES;
			this->destination_xi = destination_xi_bicubic_hermite;
			break;
		case CONVERT_TO_FINITE_ELEMENTS_BICUBIC:
			this->number_of_local_nodes = BICUBIC_NUMBER_OF_NODES;
			this->destination_xi = destination_xi_bicubic;
			break;
		case CONVERT_TO_FINITE_ELEMENTS_TRILINEAR:
			this->number_of_local_nodes = TRILINEAR_NUMBER_OF_NODES;
			this->destination_xi = destination_xi_trilinear;
			break;
		case CONVERT_TO_FINITE_ELEMENTS_TRIQUADRATIC:
			this->number_of_local_nodes = TRIQUADRATIC_NUMBER_OF_NODES;
			this->destination_xi = destination_xi_triquadratic;
			break;
		case CONVERT_TO_FINITE_ELEMENTS_TRICUBIC:
			this->number_of_local_nodes = TRICUBIC_NUMBER_OF_NODES;
			this->destination_xi = destination_xi_tricubic;
			break;
		case CONVERT_TO_FINITE_ELEMENTS_MODE_UNSPECIFIED:
			break;
		}
		cmzn_fieldmodule_begin_change(this->destination_fieldmodule);
	}

	~Convert_finite_elements_data()
	{
		cmzn_fieldmodule_end_change(this->destination_fieldmodule);
		cmzn_elementtemplate_destroy(&this->elementtemplate);
		cmzn_nodetemplate_destroy(&this->nodetemplate);
		if (this->temporary_values)
			DEALLOCATE(this->temporary_values);
		DESTROY(LIST(Octree_object))(&this->nearby_nodes);
		DESTROY(Octree)(&this->octree);
		if (this->destination_fields)
		{
			for (int i = 0; i < this->number_of_fields; i++)
				cmzn_field_finite_element_destroy(&(this->destination_fields[i]));
			DEALLOCATE(this->destination_fields);
		}
		cmzn_fieldcache_destroy(&this->destination_fieldcache);
		cmzn_mesh_destroy(&this->destination_mesh);
		cmzn_nodeset_destroy(&this->destination_nodeset);
		cmzn_fieldmodule_destroy(&this->destination_fieldmodule);
		cmzn_region_destroy(&this->destination_region);
		cmzn_fieldcache_destroy(&this->source_fieldcache);
		cmzn_mesh_destroy(&this->source_mesh);
		cmzn_fieldmodule_destroy(&this->source_fieldmodule);
		cmzn_region_destroy(&this->source_region);
	}

	int setFields(int sourceFieldsCount, cmzn_field_id *sourceFieldsIn);

	cmzn_node_id getNearestNode(FE_value *coordinates)
	{
		Octree_add_objects_near_coordinate_to_list(octree,
			/*dimension*/3, coordinates, tolerance, nearby_nodes);
		if (0 == NUMBER_IN_LIST(Octree_object)(nearby_nodes))
		{
			return NULL;
		}
		struct Octree_object *nearest_octree_object =
			Octree_object_list_get_nearest(nearby_nodes, coordinates);
		REMOVE_ALL_OBJECTS_FROM_LIST(Octree_object)(nearby_nodes);
		return static_cast<cmzn_node_id>(Octree_object_get_user_data(nearest_octree_object));
	}

	int addNode(FE_value *coordinates, cmzn_node_id node)
	{
		Octree_object *octree_node = CREATE(Octree_object)(/*dimension*/3, coordinates);
		Octree_object_set_user_data(octree_node, static_cast<void *>(node));
		return Octree_add_object(octree, octree_node);
	}

	int convertSubelement(cmzn_element_id element, int subelement_number);

	int convert();
}; /* struct Convert_finite_elements_data */

/** Set the source fields to convert. This ensures matching destination fields exist.
  * Also ensures first source field is an appropriate coordinate field for dimension.
  * Keeps pointer to passed fields array; caller must ensure it remains in existence.
  * @return  Result OK on success, any other value on failure. */
int Convert_finite_elements_data::setFields(int sourceFieldsCount, cmzn_field_id *sourceFieldsIn)
{
	if ((sourceFieldsCount < 1) || (!sourceFieldsIn))
		return CMZN_ERROR_ARGUMENT;
	cmzn_field_id source_coordinate_field = sourceFieldsIn[0];
	const int coordinate_component_count = cmzn_field_get_number_of_components(source_coordinate_field);
	if ((coordinate_component_count < this->mode_dimension) || (coordinate_component_count > 3))
	{
		display_message(ERROR_MESSAGE, "convert elements:  invalid first/coordinate field.");
		return CMZN_ERROR_ARGUMENT;
	}

	this->maximum_number_of_components = 0; // grows in iteration below

	ALLOCATE(this->destination_fields, cmzn_field_finite_element_id, sourceFieldsCount);
	if (!this->destination_fields)
		return CMZN_ERROR_MEMORY;
	int result = CMZN_OK;
	for (int f = 0; f < sourceFieldsCount; ++f)
	{
		cmzn_field_id source_field = sourceFieldsIn[f];
		char *name = cmzn_field_get_name(source_field);
		if (cmzn_field_get_value_type(source_field) != CMZN_FIELD_VALUE_TYPE_REAL)
		{
			display_message(INFORMATION_MESSAGE,
				"convert elements: cannot convert field %s with non-real value type", name);
			result = CMZN_ERROR_NOT_IMPLEMENTED;
		}
		const int number_of_components = cmzn_field_get_number_of_components(source_field);
		if (number_of_components > this->maximum_number_of_components)
			this->maximum_number_of_components = number_of_components;
		cmzn_field_id destination_field = cmzn_fieldmodule_find_field_by_name(this->destination_fieldmodule, name);
		cmzn_field_finite_element_id destination_field_finite_element = 0;
		if (destination_field)
		{
			if (cmzn_field_get_number_of_components(destination_field) != number_of_components)
			{
				display_message(INFORMATION_MESSAGE,
					"convert elements: existing destination field %s has wrong number of components", name);
				result = CMZN_ERROR_INCOMPATIBLE_DATA;
			}
			else if (0 == (destination_field_finite_element = cmzn_field_cast_finite_element(destination_field)))
			{
				display_message(INFORMATION_MESSAGE,
					"convert elements: existing destination field %s is not finite element type", name);
				result = CMZN_ERROR_INCOMPATIBLE_DATA;
			}
		}
		else
		{
			destination_field = cmzn_fieldmodule_create_field_finite_element(this->destination_fieldmodule, number_of_components);
			cmzn_field_set_name(destination_field, name);
			cmzn_field_set_managed(destination_field, true);
			destination_field_finite_element = cmzn_field_cast_finite_element(destination_field);
			if (!destination_field_finite_element)
			{
				result = CMZN_ERROR_GENERAL;
			}
			else
			{
				if (cmzn_field_is_type_coordinate(source_field))
					cmzn_field_set_type_coordinate(destination_field, true);
				for (int c = 0; c < number_of_components; ++c)
				{
					char *component_name = cmzn_field_get_component_name(source_field, c + 1);
					cmzn_field_set_component_name(destination_field, c + 1, component_name);
					cmzn_deallocate(component_name);
				}
				const cmzn_field_coordinate_system_type coordinate_system_type =
					cmzn_field_get_coordinate_system_type(source_field);
				cmzn_field_set_coordinate_system_type(destination_field, coordinate_system_type);
				const double focus = cmzn_field_get_coordinate_system_focus(source_field);
				cmzn_field_set_coordinate_system_focus(destination_field, focus);
			}
		}
		this->destination_fields[f] = destination_field_finite_element;
		cmzn_field_destroy(&destination_field);
		cmzn_deallocate(name);
	}
	this->number_of_fields = sourceFieldsCount;
	this->source_fields = sourceFieldsIn;
	return result;
}

/**
 * Converts the specified subelement refinement of element with the current
 * mode and adds it to data->destination_fe_field.
 */
int Convert_finite_elements_data::convertSubelement(cmzn_element_id element,
	int subelement_number)
{
	FE_value base_xi[MAXIMUM_ELEMENT_XI_DIMENSIONS], *values, *derivatives,
		*nodal_values, source_xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];

	int inner_subelement_count = 1;
	for (int d = 0; d < mode_dimension; d++)
	{
		int e = (subelement_number / inner_subelement_count) % refinement.count[d];
		base_xi[d] = (FE_value)e / (FE_value)refinement.count[d];
		inner_subelement_count *= refinement.count[d];
	}
	cmzn_node_id nodes[MAX_NUMBER_OF_NODES]; // not accessed
	switch (mode)
	{
		case CONVERT_TO_FINITE_ELEMENTS_HERMITE_2D_PRODUCT:
		{
			FE_value destination_xi[HERMITE_2D_NUMBER_OF_NODES][2] =
				{{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}};
			int return_code = 1;
			for (int n = 0 ; n < HERMITE_2D_NUMBER_OF_NODES; ++n)
			{
				cmzn_node_id node = cmzn_nodeset_create_node(this->destination_nodeset, -1, this->nodetemplate);
				nodes[n] = node;
				if (!node || (CMZN_OK != cmzn_elementtemplate_set_node(this->elementtemplate, n + 1, node)))
				{
					display_message(ERROR_MESSAGE, "convert elements.  Failed to create or set element node.");
					return_code = 0;
				}
				cmzn_node_destroy(&node);
			}
			if (return_code == 0)
				return return_code;
			for (int f = 0 ; f < number_of_fields; ++f)
			{
				cmzn_field_id source_field = this->source_fields[f];
				cmzn_field_finite_element_id destination_field = this->destination_fields[f];
				const int number_of_components = cmzn_field_get_number_of_components(source_field);
				const int number_of_values = 4 * number_of_components;
				values = this->temporary_values;
				derivatives = this->temporary_values + number_of_components;
				nodal_values = this->temporary_values + number_of_values;
				for (int n = 0; n < this->number_of_local_nodes; ++n)
				{
					for (int d = 0; d < mode_dimension; d++)
					{
						source_xi[d] = base_xi[d] + destination_xi[n][d]*delta_xi[d];
					}
					if ((CMZN_OK == source_fieldcache->setMeshLocation(element, source_xi)) &&
						(CMZN_OK == cmzn_field_evaluate_real_with_derivatives(source_field, source_fieldcache, number_of_components, values,
							mode_dimension, derivatives)))
					{
						/* Reorder the separate lists of values and derivatives into
							a single mixed list */
						for (int k = 0 ; k < number_of_components ; k++)
						{
							nodal_values[  number_of_components + k] = delta_xi[0]*derivatives[2*k    ];
							nodal_values[2*number_of_components + k] = delta_xi[1]*derivatives[2*k + 1];
							// cannot determine cross derivative from first derivatives, so use zero:
							nodal_values[3*number_of_components + k] = 0.0;
						}
						if ((CMZN_OK != cmzn_fieldcache_set_node(this->destination_fieldcache, nodes[n]))
							|| (CMZN_OK != cmzn_field_finite_element_set_node_parameters(destination_field,
								this->destination_fieldcache, -1, CMZN_NODE_VALUE_LABEL_VALUE, /*version*/1, number_of_components, values))
							|| (CMZN_OK != cmzn_field_finite_element_set_node_parameters(destination_field,
								this->destination_fieldcache, -1, CMZN_NODE_VALUE_LABEL_D_DS1, /*version*/1, number_of_components, nodal_values + number_of_components))
							|| (CMZN_OK != cmzn_field_finite_element_set_node_parameters(destination_field,
								this->destination_fieldcache, -1, CMZN_NODE_VALUE_LABEL_D_DS2, /*version*/1, number_of_components, nodal_values + 2*number_of_components))
							|| (CMZN_OK != cmzn_field_finite_element_set_node_parameters(destination_field,
								this->destination_fieldcache, -1, CMZN_NODE_VALUE_LABEL_D2_DS1DS2, /*version*/1, number_of_components, nodal_values + 3*number_of_components)))
						{
							display_message(ERROR_MESSAGE, "convert elements.  Failed to set node field parameters.");
							return 0;
						}
					}
					else
					{
						display_message(ERROR_MESSAGE,"convert elements.  Field not defined.");
						return 0;
					}
				}
			}
			if (CMZN_OK != cmzn_mesh_define_element(this->destination_mesh, -1, this->elementtemplate))
			{
				display_message(ERROR_MESSAGE, "convert elements:  Failed to create element.");
				return 0;
			}
		} break;
		case CONVERT_TO_FINITE_ELEMENTS_BICUBIC:
		case CONVERT_TO_FINITE_ELEMENTS_TRILINEAR:
		case CONVERT_TO_FINITE_ELEMENTS_TRIQUADRATIC:
		case CONVERT_TO_FINITE_ELEMENTS_TRICUBIC:
		{
			for (int f = 0 ; f < number_of_fields ; ++f)
			{
				cmzn_field_id source_field = this->source_fields[f];
				cmzn_field_finite_element_id destination_field = this->destination_fields[f];
				const int number_of_components = cmzn_field_get_number_of_components(source_field);
				values = temporary_values;
				for (int n = 0; n < this->number_of_local_nodes; ++n)
				{
					for (int d = 0; d < mode_dimension; d++)
					{
						source_xi[d] = base_xi[d] + destination_xi[n][d]*delta_xi[d];
					}
					if ((CMZN_OK == source_fieldcache->setMeshLocation(element, source_xi)) &&
						(CMZN_OK == cmzn_field_evaluate_real(source_field, source_fieldcache, number_of_components, values)))
					{
						bool setFieldValues = true;
						if (f==0)
						{
							int return_code = 1;
							// assuming first field is coordinate field, find node with same field value
							cmzn_node_id node = getNearestNode(values);
							if (node)
							{
								cmzn_node_access(node);
								setFieldValues = false;
							}
							else
							{
								node = cmzn_nodeset_create_node(this->destination_nodeset, -1, this->nodetemplate);
								if (node)
								{
									this->addNode(values, node);
								}
								else
								{
									display_message(ERROR_MESSAGE, "convert elements.  Failed to create node.");
									return_code = 0;
								}
							}
							if (CMZN_OK != cmzn_elementtemplate_set_node(this->elementtemplate, n + 1, node))
							{
								display_message(ERROR_MESSAGE, "convert elements.  Failed to set element node.");
								return_code = 0;
							}
							nodes[n] = node;
							cmzn_node_destroy(&node);
							if (return_code == 0)
								return return_code;
						}
						if (setFieldValues)
						{
							if ((CMZN_OK != cmzn_fieldcache_set_node(this->destination_fieldcache, nodes[n]))
								|| (CMZN_OK != cmzn_field_finite_element_set_node_parameters(destination_field,
									this->destination_fieldcache, -1, CMZN_NODE_VALUE_LABEL_VALUE, /*version*/1, number_of_components, values)))
							{
								display_message(ERROR_MESSAGE, "convert elements:  Failed to set node field parameters.");
								return 0;
							}
						}
					}
					else
					{
						display_message(ERROR_MESSAGE,"convert elements:   Field not defined.");
						return 0;
					}
				}
			}
			if (CMZN_OK != cmzn_mesh_define_element(this->destination_mesh, -1, this->elementtemplate))
			{
				display_message(ERROR_MESSAGE, "convert elements:  Failed to create element.");
				return 0;
			}
		} break;
		default:
		{
		}
	}
	return 1;
}

/** Constructs destination elements for each subelement in the source mesh.
  *@return  1 on success, 0 on failure. */
int Convert_finite_elements_data::convert()
{
	cmzn_elementiterator_id iter = cmzn_mesh_create_elementiterator(this->source_mesh);
	if (!iter)
		return 0;
	int return_code = 1;
	cmzn_element_id element;
	while (0 != (element = cmzn_elementiterator_next_non_access(iter)))
	{
		for (int i = 0; (i < this->subelement_count) && return_code; i++)
		{
			if (!this->convertSubelement(element, i))
			{
				return_code = 0;
				break;
			}
		}
	}
	cmzn_elementiterator_destroy(&iter);
	return return_code;
}

/*
Global functions
----------------
*/

PROTOTYPE_ENUMERATOR_STRING_FUNCTION(Convert_finite_elements_mode)
{
	const char *enumerator_string = 0;

	ENTER(ENUMERATOR_STRING(Convert_finite_elements_mode));
	switch (enumerator_value)
	{
		case CONVERT_TO_FINITE_ELEMENTS_HERMITE_2D_PRODUCT:
		{
			enumerator_string = "convert_hermite_2D_product_elements";
		} break;
		case CONVERT_TO_FINITE_ELEMENTS_BICUBIC:
		{
			enumerator_string = "convert_bicubic";
		} break;
		case CONVERT_TO_FINITE_ELEMENTS_TRILINEAR:
		{
			enumerator_string = "convert_trilinear";
		} break;
		case CONVERT_TO_FINITE_ELEMENTS_TRIQUADRATIC:
		{
			enumerator_string = "convert_triquadratic";
		} break;
		case CONVERT_TO_FINITE_ELEMENTS_TRICUBIC:
		{
			enumerator_string = "convert_tricubic";
		} break;
		case CONVERT_TO_FINITE_ELEMENTS_MODE_UNSPECIFIED:
		{
			enumerator_string = (const char *)NULL;
		} break;
	}
	LEAVE;

	return (enumerator_string);
} /* ENUMERATOR_STRING(Convert_finite_elements_mode) */

DEFINE_DEFAULT_ENUMERATOR_FUNCTIONS(Convert_finite_elements_mode)

int finite_element_conversion(cmzn_region_id source_region,
	cmzn_region_id destination_region,
	enum Convert_finite_elements_mode mode,
	int number_of_source_fields, cmzn_field_id *source_fields,
	struct Element_refinement refinement, FE_value tolerance)
{
	if (!(source_region && destination_region && (0 < number_of_source_fields)
		&& (source_fields) && (*source_fields) && (tolerance >= 0.0)))
	{
		display_message(ERROR_MESSAGE, "convert elements.  Invalid arguments.");
		return 0;
	}

	Convert_finite_elements_data data(source_region,
		mode, refinement, tolerance, destination_region);
	int result = data.setFields(number_of_source_fields, source_fields);
	if (result != CMZN_OK)
		return 0;

	/* Set up data */
	const int nodeIndexesCount = data.number_of_local_nodes;
	int nodeIndexes[MAX_NUMBER_OF_NODES];
	for (int i = 0; i < nodeIndexesCount; ++i)
		nodeIndexes[i] = i + 1;

	cmzn_element_shape_type elementShapeType = CMZN_ELEMENT_SHAPE_TYPE_INVALID;
	switch (mode)
	{
	case CONVERT_TO_FINITE_ELEMENTS_HERMITE_2D_PRODUCT:
	case CONVERT_TO_FINITE_ELEMENTS_BICUBIC:
		elementShapeType = CMZN_ELEMENT_SHAPE_TYPE_SQUARE;
		break;
	case CONVERT_TO_FINITE_ELEMENTS_TRILINEAR:
	case CONVERT_TO_FINITE_ELEMENTS_TRIQUADRATIC:
	case CONVERT_TO_FINITE_ELEMENTS_TRICUBIC:
		elementShapeType = CMZN_ELEMENT_SHAPE_TYPE_CUBE;
		break;
	default:
		display_message(ERROR_MESSAGE, "convert elements:  Invalid or unimplemented conversion mode.");
		return 0;
		break;
	}

	switch (mode)
	{
		case CONVERT_TO_FINITE_ELEMENTS_HERMITE_2D_PRODUCT:
		{
			ALLOCATE(data.temporary_values, FE_value, /*dofs*/2*4*data.maximum_number_of_components);
			if (!data.temporary_values)
			{
				display_message(ERROR_MESSAGE, "convert elements:  Unable to allocate temporary values.");
				return 0;
			}
			data.nodetemplate = cmzn_nodeset_create_nodetemplate(data.destination_nodeset);
			if (!data.nodetemplate)
			{
				display_message(ERROR_MESSAGE, "convert elements:  Failed to create Hermite node template.");
				return 0;
			}
			for (int f = 0; f < data.number_of_fields; ++f)
			{
				cmzn_field_id destination_field = cmzn_field_finite_element_base_cast(data.destination_fields[f]);
				if ((CMZN_OK != cmzn_nodetemplate_define_field(data.nodetemplate, destination_field))
					|| (CMZN_OK != cmzn_nodetemplate_set_value_number_of_versions(data.nodetemplate, destination_field, -1, CMZN_NODE_VALUE_LABEL_D_DS1, 1))
					|| (CMZN_OK != cmzn_nodetemplate_set_value_number_of_versions(data.nodetemplate, destination_field, -1, CMZN_NODE_VALUE_LABEL_D_DS2, 1))
					|| (CMZN_OK != cmzn_nodetemplate_set_value_number_of_versions(data.nodetemplate, destination_field, -1, CMZN_NODE_VALUE_LABEL_D2_DS1DS2, 1)))
				{
					display_message(ERROR_MESSAGE, "convert elements:  Failed to define Hermite node template field");
					return 0;
				}
			}
			data.elementtemplate = cmzn_mesh_create_elementtemplate(data.destination_mesh);
			if (!(data.elementtemplate)
				|| (CMZN_OK != cmzn_elementtemplate_set_element_shape_type(data.elementtemplate, elementShapeType))
				|| (CMZN_OK != cmzn_elementtemplate_set_number_of_nodes(data.elementtemplate, 4)))
			{
				display_message(ERROR_MESSAGE, "convert elements:  Failed to create Hermite element template.");
				return 0;
			}
			cmzn_elementbasis_id elementbasis = cmzn_fieldmodule_create_elementbasis(data.destination_fieldmodule, 2, CMZN_ELEMENTBASIS_FUNCTION_TYPE_CUBIC_HERMITE);
			bool success = true;
			for (int f = 0; f < data.number_of_fields; ++f)
			{
				cmzn_field_id destination_field = cmzn_field_finite_element_base_cast(data.destination_fields[f]);
				if (CMZN_OK != cmzn_elementtemplate_define_field_simple_nodal(data.elementtemplate,
					destination_field, -1, elementbasis, nodeIndexesCount, nodeIndexes))
				{
					display_message(ERROR_MESSAGE, "convert elements:  Failed to define Hermite element template field");
					success = false;
					break;
				}
			}
			cmzn_elementbasis_destroy(&elementbasis);
			if (!success)
				return 0;
		} break;
		case CONVERT_TO_FINITE_ELEMENTS_BICUBIC:
		case CONVERT_TO_FINITE_ELEMENTS_TRILINEAR:
		case CONVERT_TO_FINITE_ELEMENTS_TRIQUADRATIC:
		case CONVERT_TO_FINITE_ELEMENTS_TRICUBIC:
		{
			ALLOCATE(data.temporary_values, FE_value, /*dofs*/2*data.maximum_number_of_components);
			if (!data.temporary_values)
			{
				display_message(ERROR_MESSAGE, "convert elements:  Unable to allocate temporary values.");
				return 0;
			}
			data.nodetemplate = cmzn_nodeset_create_nodetemplate(data.destination_nodeset);
			if (!data.nodetemplate)
			{
				display_message(ERROR_MESSAGE, "convert elements:  Failed to create Lagrange node template.");
				return 0;
			}
			for (int f = 0; f < data.number_of_fields; ++f)
			{
				if (CMZN_OK != cmzn_nodetemplate_define_field(data.nodetemplate, cmzn_field_finite_element_base_cast(data.destination_fields[f])))
				{
					display_message(ERROR_MESSAGE, "convert elements:  Failed to define Lagrange node template field");
					return 0;
				}
			}
			data.elementtemplate = cmzn_mesh_create_elementtemplate(data.destination_mesh);
			if (!(data.elementtemplate)
				|| (CMZN_OK != cmzn_elementtemplate_set_element_shape_type(data.elementtemplate, elementShapeType))
				|| (CMZN_OK != cmzn_elementtemplate_set_number_of_nodes(data.elementtemplate, nodeIndexesCount)))
			{
				display_message(ERROR_MESSAGE, "convert elements:  Failed to create Lagrange element template.");
				return 0;
			}
			const cmzn_elementbasis_function_type basis_function_type =
				(mode == CONVERT_TO_FINITE_ELEMENTS_BICUBIC) ? CMZN_ELEMENTBASIS_FUNCTION_TYPE_CUBIC_LAGRANGE :
				(mode == CONVERT_TO_FINITE_ELEMENTS_TRILINEAR) ? CMZN_ELEMENTBASIS_FUNCTION_TYPE_LINEAR_LAGRANGE :
				(mode == CONVERT_TO_FINITE_ELEMENTS_TRIQUADRATIC) ? CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_LAGRANGE :
				(mode == CONVERT_TO_FINITE_ELEMENTS_TRICUBIC) ? CMZN_ELEMENTBASIS_FUNCTION_TYPE_CUBIC_LAGRANGE :
				CMZN_ELEMENTBASIS_FUNCTION_TYPE_INVALID;
			cmzn_elementbasis_id elementbasis = cmzn_fieldmodule_create_elementbasis(data.destination_fieldmodule, data.mode_dimension, basis_function_type);
			bool success = true;
			for (int f = 0; f < data.number_of_fields; ++f)
			{
				cmzn_field_id destination_field = cmzn_field_finite_element_base_cast(data.destination_fields[f]);
				if (CMZN_OK != cmzn_elementtemplate_define_field_simple_nodal(data.elementtemplate,
					destination_field, -1, elementbasis, nodeIndexesCount, nodeIndexes))
				{
					display_message(ERROR_MESSAGE, "convert elements:  Failed to define Lagrange element template field");
					success = false;
					break;
				}
			}
			cmzn_elementbasis_destroy(&elementbasis);
			if (!success)
				return 0;
		} break;
		default:
		{
			display_message(ERROR_MESSAGE,"convert elements:  Invalid or unimplemented conversion mode.");
			return 0;
		} break;
	}
	return data.convert();
}
