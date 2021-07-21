/**
 * FILE : export_cm_files.c
 *
 * Functions for exporting finite element data to CMISS IP files.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stdio.h>

#include "opencmiss/zinc/element.h"
#include "opencmiss/zinc/fieldmodule.h"
#include "opencmiss/zinc/fieldgroup.h"
#include "opencmiss/zinc/fieldsubobjectgroup.h"
#include "opencmiss/zinc/mesh.h"
#include "opencmiss/zinc/node.h"
#include "opencmiss/zinc/nodeset.h"
#include "opencmiss/zinc/region.h"
#include "opencmiss/zinc/status.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_finite_element.h"
#include "finite_element/export_cm_files.h"
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_mesh.hpp"
#include "finite_element/finite_element_nodeset.hpp"
#include "finite_element/finite_element_private.h"
#include "finite_element/finite_element_region.h"
#include "general/debug.h"
#include "general/geometry.h"
#include "general/message.h"
#include "region/cmiss_region.hpp"

/*
Module types
------------
*/

struct write_cm_files_data
{
	int number_of_bases;
	struct FE_basis **basis_array;
}; /* struct write_cm_files_data */

static int write_ipcoor_file(FILE *ipcoor_file, struct FE_field *field)
/*******************************************************************************
LAST MODIFIED : 22 March 2006

DESCRIPTION :
Writes text for an <ipcoor_file> to support <field>.
==============================================================================*/
{
	int return_code;

	ENTER(write_ipcoor_file);
	if (ipcoor_file && field)
	{
		return_code = 1;
		fprintf(ipcoor_file, " CMISS Version 1.21 ipcoor File Version 1\n");
		fprintf(ipcoor_file, " Heading: cmgui generated file\n\n");
		fprintf(ipcoor_file, " The global coordinates for region 1 are [1]:\n");
		fprintf(ipcoor_file, "   (1) rectangular cartesian (x,y,z)\n");
		fprintf(ipcoor_file, "   (2) cylindrical polar (r,theta,z)\n");
		fprintf(ipcoor_file, "   (3) spherical polar (r,theta,phi)\n");
		fprintf(ipcoor_file, "   (4) prolate spheroidal (lambda,mu,theta)\n");
		fprintf(ipcoor_file, "   (5) oblate  spheroidal (lambda,mu,theta)\n");
		const Coordinate_system& coordinate_system = field->getCoordinateSystem();
		switch (coordinate_system.type)
		{
			case RECTANGULAR_CARTESIAN:
			{
				fprintf(ipcoor_file, "    1\n");
			} break;
			case CYLINDRICAL_POLAR:
			{
				fprintf(ipcoor_file, "    2\n");
			} break;
			default:
			{
				display_message(ERROR_MESSAGE, "write_ipcoor_file.  "
					"Coordinate system %s not implemented yet.", Coordinate_system_string(&coordinate_system));
				return_code = 0;
			} break;
		}
		fprintf(ipcoor_file, " Enter the number of global coordinates [3]: %d\n",
			get_FE_field_number_of_components(field));
		fprintf(ipcoor_file, " Do you want to specify another coord. system for dependent variables [N]? N\n");

		if (coordinate_system.type == CYLINDRICAL_POLAR)
		{
			fprintf(ipcoor_file, " Specify whether radial interpolation is in [1]:\n"
				"   (1) r\n"
				"   (2) r^2\n"
				"    1\n");
		}
		fprintf(ipcoor_file, " Enter x,y,z origin of coords relative to region 0 [0,0,0]:0.00000D+00  0.00000D+00  0.00000D+00\n");
		fprintf(ipcoor_file, " Are there any non-standard mappings [N]? N\n");
	}
	else
	{
		display_message(ERROR_MESSAGE, "write_ipcoor_file.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* write_ipcoor_file */

struct FE_element_field_add_basis_data
{
	int number_of_components;
	struct FE_field *field;
	struct FE_region *fe_region;
	struct LIST(FE_basis) *basis_types;
}; /* struct FE_element_field_add_basis_data */

/**
 * FE_element iterator which adds the FE_basis representing data->field to the
 * data->basis_list if it isn't there already.
 */
static int FE_element_field_add_basis_to_list(
	struct FE_element *element, FE_element_field_add_basis_data *data)
{
	int return_code;
	if (element && data)
	{
		int basis_type_array[4], dimension, xi1, xi2;
		struct FE_basis *face_basis, *fe_basis;
		return_code = 1;
		FE_mesh_field_data *meshFieldData = data->field->getMeshFieldData(element->getMesh());
		if (!meshFieldData)
		{
			return 1; // not defined on mesh
		}
		for (int i = 0 ; return_code && (i < data->number_of_components) ; i++)
		{
			FE_mesh_field_template *mft = meshFieldData->getComponentMeshfieldtemplate(i);
			FE_element_field_template *eft = mft->getElementfieldtemplate(element->getIndex());
			if (!eft)
				return 1; // not defined on element
			fe_basis = eft->getBasis();
			if (!IS_OBJECT_IN_LIST(FE_basis)(fe_basis, data->basis_types))
			{
				return_code = ADD_OBJECT_TO_LIST(FE_basis)(fe_basis, data->basis_types);
				FE_basis_get_dimension(fe_basis, &dimension);
				if (dimension == 3)
				{
					/* Add the face bases into the list */
					basis_type_array[0] = /*dimension*/2;
					basis_type_array[2] = /*off diagonal*/NO_RELATION;
					for (xi1 = 0 ; xi1 < dimension ; xi1++)
					{
						FE_basis_get_xi_basis_type(fe_basis, xi1, reinterpret_cast<FE_basis_type *>(&basis_type_array[1]));
						for (xi2 = xi1 + 1 ; xi2 < dimension ; xi2++)
						{
							FE_basis_get_xi_basis_type(fe_basis, xi2,  reinterpret_cast<FE_basis_type *>(&basis_type_array[3]));
							face_basis = make_FE_basis(basis_type_array,
								FE_region_get_basis_manager(data->fe_region));
							if (!IS_OBJECT_IN_LIST(FE_basis)(face_basis, data->basis_types))
							{
								return_code = ADD_OBJECT_TO_LIST(FE_basis)(face_basis,
									data->basis_types);
							}
						}
					}
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_element_field_add_basis_to_list.  Invalid argument(s)");
		return_code = 0;
	}
	return (return_code);
}

/**
 * Writes text for an <ipbase_file> to support <field>.
 */
static int write_ipbase_file(FILE *ipbase_file, cmzn_region *region,
	cmzn_field_group *group, struct FE_field *field, struct write_cm_files_data *data)
{
	enum FE_basis_type basis_type;
	int any_derivatives, basis_number, dimension, finished,
		has_derivatives[MAXIMUM_ELEMENT_XI_DIMENSIONS], interpolant_index,
		node_flags[MAXIMUM_ELEMENT_XI_DIMENSIONS], number_of_gauss_points,
		number_of_nodes[MAXIMUM_ELEMENT_XI_DIMENSIONS], return_code, xi_number;
	struct FE_basis *basis;

	if (ipbase_file && field)
	{
		return_code = 1;

		FE_element_field_add_basis_data add_basis_data;
		add_basis_data.basis_types = CREATE(LIST(FE_basis))();
		add_basis_data.field = field;
		add_basis_data.fe_region = region->get_FE_region();
		add_basis_data.number_of_components = get_FE_field_number_of_components(field);

		cmzn_fieldmodule *fieldmodule = cmzn_region_get_fieldmodule(region);
		for (dimension = MAXIMUM_ELEMENT_XI_DIMENSIONS; 0 < dimension; --dimension)
		{
			cmzn_mesh *mesh = cmzn_fieldmodule_find_mesh_by_dimension(fieldmodule, dimension);
			cmzn_elementiterator *iterator = 0;
			if (group)
			{
				cmzn_field_element_group *element_group = cmzn_field_group_get_field_element_group(group, mesh);
				if (element_group)
				{
					cmzn_mesh_group *mesh_group = cmzn_field_element_group_get_mesh_group(element_group);
					iterator = cmzn_mesh_create_elementiterator(cmzn_mesh_group_base_cast(mesh_group));
					cmzn_mesh_group_destroy(&mesh_group);
					cmzn_field_element_group_destroy(&element_group);
				}
			}
			else
				iterator = cmzn_mesh_create_elementiterator(mesh);
			if (iterator)
			{
				cmzn_element *element;
				while (0 != (element = cmzn_elementiterator_next_non_access(iterator)))
				{
					if (!FE_element_field_add_basis_to_list(element, &add_basis_data))
					{
						return_code = 0;
						break;
					}
				}
				cmzn_elementiterator_destroy(&iterator);
			}
			cmzn_mesh_destroy(&mesh);
		}
		cmzn_fieldmodule_destroy(&fieldmodule);

		data->number_of_bases = NUMBER_IN_LIST(FE_basis)(add_basis_data.basis_types);

		/* Need an array so that we have an integer index for each basis */
		ALLOCATE(data->basis_array, struct FE_basis *, data->number_of_bases);

		fprintf(ipbase_file, " CMISS Version 1.21 ipbase File Version 2\n");
		fprintf(ipbase_file, " Heading: cmgui generated file\n\n");
		fprintf(ipbase_file, " Enter the number of types of basis function [1]: %d\n\n",
			data->number_of_bases);

		basis_number = 0;
		while (NULL != (basis = FIRST_OBJECT_IN_LIST_THAT(FE_basis)(
			(LIST_CONDITIONAL_FUNCTION(FE_basis) *)NULL, (void *)NULL,
			add_basis_data.basis_types)))
		{
			/* Put it in the array */
			data->basis_array[basis_number] = basis;

			fprintf(ipbase_file, " For basis function type %d the type of nodal interpolation is [1]:\n",
				basis_number + 1);
			fprintf(ipbase_file, "   (0) Auxiliary basis only\n");
			fprintf(ipbase_file, "   (1) Lagrange/Hermite tensor prod\n");
			fprintf(ipbase_file, "   (2) Simplex/Serendipity/Sector\n");
			fprintf(ipbase_file, "   (3) B-spline tensor product\n");
			fprintf(ipbase_file, "   (4) Fourier Series/Lagrange/Hermite tensor prod\n");
			fprintf(ipbase_file, "   (5) Boundary Element Lagrange/Hermite tensor pr.\n");
			fprintf(ipbase_file, "   (6) Boundary Element Simplex/Serendipity/Sector\n");
			fprintf(ipbase_file, "   (7) Extended Lagrange (multigrid collocation)\n");
			fprintf(ipbase_file, "    1\n");

			FE_basis_get_dimension(basis, &dimension);
			fprintf(ipbase_file, " Enter the number of Xi-coordinates [1]: %d\n\n", dimension);

			any_derivatives = 0;
			for (xi_number = 0 ; return_code && (xi_number < dimension) ; xi_number++)
			{
				FE_basis_get_xi_basis_type(basis, xi_number, &basis_type);
				has_derivatives[xi_number] = 0;
				switch (basis_type)
				{
					case LINEAR_LAGRANGE:
					{
						interpolant_index = 1;
						number_of_gauss_points = 2;
						number_of_nodes[xi_number] = 2;
					} break;
					case QUADRATIC_LAGRANGE:
					{
						interpolant_index = 2;
						number_of_gauss_points = 3;
						number_of_nodes[xi_number] = 3;
					} break;
					case CUBIC_LAGRANGE:
					{
						interpolant_index = 3;
						number_of_gauss_points = 4;
						number_of_nodes[xi_number] = 4;
					} break;
					case CUBIC_HERMITE:
					{
						interpolant_index = 5;
						number_of_gauss_points = 4;
						number_of_nodes[xi_number] = 2;
						has_derivatives[xi_number] = 1;
						any_derivatives = 1;
					} break;
					default:
					{
						display_message(ERROR_MESSAGE, "write_ipcoor_file.  "
							"Basis type %s not implemented or supported.", FE_basis_type_string(basis_type));
						interpolant_index = 0;
						number_of_gauss_points = 0;
						return_code = 0;
					}
				}
				fprintf(ipbase_file, " The interpolant in the Xi(%d) direction is [1]:\n",
					xi_number + 1);
				fprintf(ipbase_file, "   (1) Linear Lagrange\n");
				fprintf(ipbase_file, "   (2) Quadratic Lagrange\n");
				fprintf(ipbase_file, "   (3) Cubic Lagrange\n");
				fprintf(ipbase_file, "   (4) Quadratic Hermite\n");
				fprintf(ipbase_file, "   (5) Cubic Hermite\n");
				fprintf(ipbase_file, "    %d\n", interpolant_index);

				fprintf(ipbase_file, " Enter the number of Gauss points in the Xi(%d) direction [2]: %d\n",
					xi_number + 1, number_of_gauss_points);
			}
			if (return_code)
			{
				if (any_derivatives)
				{
					fprintf(ipbase_file, " Do you want to set cross derivatives to zero [N]? N\n");
				}
				fprintf(ipbase_file, " Enter the node position indices []:");
				for (xi_number = 0 ; xi_number < dimension ; xi_number++)
				{
					node_flags[xi_number] = 1;
				}
				finished = 0;
				while (!finished)
				{
					for (xi_number = 0 ; xi_number < dimension ; xi_number++)
					{
						fprintf(ipbase_file, " %d", node_flags[xi_number]);
					}
					node_flags[0]++;
					for (xi_number = 0 ; (node_flags[xi_number] > number_of_nodes[xi_number]) && (xi_number < dimension) ; xi_number++)
					{
						node_flags[xi_number] = 1;
						if ((xi_number + 1) < dimension)
							node_flags[xi_number + 1]++;
					}
					if (xi_number == dimension)
					{
						finished = 1;
					}
				}
				fprintf(ipbase_file, "\n");
				if (any_derivatives)
				{
					fprintf(ipbase_file, " Enter the derivative order indices []: ");
					for (xi_number = 0 ; xi_number < dimension ; xi_number++)
					{
						node_flags[xi_number] = 1;
					}
					finished = 0;
					while (!finished)
					{
						for (xi_number = 0 ; xi_number < dimension ; xi_number++)
						{
							if (has_derivatives[xi_number])
							{
								fprintf(ipbase_file, " %d", node_flags[xi_number]);
							}
						}
						node_flags[0]++;
						for (xi_number = 0 ; (node_flags[xi_number] > (2 * has_derivatives[xi_number])) && (xi_number < dimension) ;
							  xi_number++)
						{
							node_flags[xi_number] = 1;
							if ((xi_number + 1) < dimension)
								node_flags[xi_number + 1]++;
						}
						if (xi_number == dimension)
						{
							finished = 1;
						}
					}
					fprintf(ipbase_file, "\n");
				}
				fprintf(ipbase_file, " Enter the number of auxiliary element parameters [0]:  0\n\n");
				if (any_derivatives)
				{
					fprintf(ipbase_file, " For basis function type %d scale factors are [6]:\n",
						basis_number + 1);
					fprintf(ipbase_file, "   (1) Unit\n");
					fprintf(ipbase_file, "   (2) Read in - Element based\n");
					fprintf(ipbase_file, "   (3) Read in - Node based\n");
					fprintf(ipbase_file, "   (4) Calculated from angle change\n");
					fprintf(ipbase_file, "   (5) Calculated from arc length\n");
					fprintf(ipbase_file, "   (6) Calculated from arithmetic mean arc length\n");
					fprintf(ipbase_file, "   (7) Calculated from harmonic mean arc length\n");
					fprintf(ipbase_file, "    1\n");
				}
			}

			basis_number++;
			REMOVE_OBJECT_FROM_LIST(FE_basis)(basis, add_basis_data.basis_types);
		}
		DESTROY(LIST(FE_basis))(&add_basis_data.basis_types);

	}
	else
	{
		display_message(ERROR_MESSAGE, "write_ipbase_file.  Invalid argument(s)");
		return_code = 0;
	}
	return (return_code);
}

struct FE_node_write_cm_check_node_values_data
{
	int number_of_nodes;
	struct FE_field *field;
	int number_of_components;
	int number_of_derivatives[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	int has_versions[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	int number_of_nodes_with_versions;
	int maximum_number_of_derivatives;
	FILE *ipnode_file;
	FILE *ipmap_file;
}; /* struct FE_node_write_cm_check_node_values_data */

/**
 * Counts how many nodes have the field defined and checks that the nodes
 * for which the field is defined are consistent in the number of derivatives
 * for each component.
 * Also sets a flag if any of components of the field have versions.
 */
static int FE_node_write_cm_check_node_values(
	struct FE_node *node, FE_node_write_cm_check_node_values_data *data)
{
	int return_code = 1;
	if (node && data)
	{
		bool has_versions = false;
		const FE_node_field *node_field = node->getNodeField(data->field);
		if (node_field)
		{
			if (data->number_of_nodes)
			{
				/* Check for consistent number of derivatives */
				for (int c = 0 ; c < data->number_of_components ; ++c)
				{
					const FE_node_field_template *nft = node_field->getComponent(c);
					if (data->number_of_derivatives[c] != nft->getMaximumDerivativeNumber())
					{
						display_message(ERROR_MESSAGE, "FE_node_write_cm_check_node_values.  "
							"Node %d has inconsistent numbers of derivatives / value labels.",
							get_FE_node_identifier(node));
						return_code = 0;
					}
					if (1 != nft->getMaximumVersionsCount())
					{
						++(data->has_versions[c]);
						has_versions = true;
					}
				}
			}
			else
			{
				/* This is the first node */
				for (int c = 0; c < data->number_of_components; ++c)
				{
					const FE_node_field_template *nft = node_field->getComponent(c);
					data->number_of_derivatives[c] = nft->getMaximumDerivativeNumber();
					if (1 != nft->getMaximumVersionsCount())
					{
						++(data->has_versions[c]);
						has_versions = true;
					}
				}
			}
			++(data->number_of_nodes);
			if (has_versions)
			{
				++(data->number_of_nodes_with_versions);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_node_write_cm_check_node_values.  Missing element_field");
		return_code = 0;
	}
	return (return_code);
}

/**
 * Writes the node to an ipnode file.
 */
static int write_cm_FE_node(
	struct FE_node *node, FE_node_write_cm_check_node_values_data *data)
{
	const char *value_strings[] = {" 1", " 2", "s 1 & 2", " 3", "s 1 & 3", "s 2 & 3",
		"s 1, 2 & 3"};
	const cmzn_node_value_label derivativeValueLabels[] =
	{
		CMZN_NODE_VALUE_LABEL_D_DS1,
		CMZN_NODE_VALUE_LABEL_D_DS2,
		CMZN_NODE_VALUE_LABEL_D2_DS1DS2,
		CMZN_NODE_VALUE_LABEL_D_DS3,
		CMZN_NODE_VALUE_LABEL_D2_DS1DS3,
		CMZN_NODE_VALUE_LABEL_D2_DS2DS3,
		CMZN_NODE_VALUE_LABEL_D3_DS1DS2DS3
	};
	FE_value value;
	int j, k, number_of_versions, return_code;

	if (node && data)
	{
		return_code = 1;
		const FE_node_field *node_field = node->getNodeField(data->field);
		if (node_field)
		{
			fprintf(data->ipnode_file, " Node number [    1]:     %d\n",
				get_FE_node_identifier(node));
			for (int c = 0; c < data->number_of_components; ++c)
			{
				const FE_node_field_template *nft = node_field->getComponent(c);
				if (data->has_versions[c])
				{
					number_of_versions = nft->getMaximumVersionsCount();
					fprintf(data->ipnode_file, " The number of versions for nj=%d is [1]:  %d\n",
						c + 1, number_of_versions);
				}
				else
				{
					number_of_versions = 1;
				}
				for (j = 0 ; j < number_of_versions ; j++)
				{
					if (1 < number_of_versions)
					{
						fprintf(data->ipnode_file, " For version number%2d:\n", j + 1);
					}
					get_FE_nodal_FE_value_value(node, data->field, c,
						CMZN_NODE_VALUE_LABEL_VALUE, /*version*/j, /*time*/0.0, &value);
					fprintf(data->ipnode_file, " The Xj(%d) coordinate is [ 0.00000E+00]:  %le\n",
						c + 1, value);
					for (k = 0 ; k < data->number_of_derivatives[c] ; k++)
					{
						get_FE_nodal_FE_value_value(node, data->field, c, 
							derivativeValueLabels[k], /*version*/j, /*time*/0.0, &value);
						fprintf(data->ipnode_file, " The derivative wrt direction%s is [ 0.00000E+00]:  %le\n",
							value_strings[k], value);
					}
				}
			}
			fprintf(data->ipnode_file, "\n");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE, "write_cm_FE_node.  Invalid argument(s)");
		return_code = 0;
	}
	return (return_code);
}

/**
 * Writes the node to an ipmap file.
 */
static int write_cm_FE_nodal_mapping(
	struct FE_node *node, FE_node_write_cm_check_node_values_data *data)
{
	const char *value_strings[] = {" 1", " 2", "s 1 & 2", " 3", "s 1 & 3", "s 2 & 3",
		"s 1, 2 & 3"};
	const cmzn_node_value_label derivativeValueLabels[] =
	{
		CMZN_NODE_VALUE_LABEL_D_DS1,
		CMZN_NODE_VALUE_LABEL_D_DS2,
		CMZN_NODE_VALUE_LABEL_D2_DS1DS2,
		CMZN_NODE_VALUE_LABEL_D_DS3,
		CMZN_NODE_VALUE_LABEL_D2_DS1DS3,
		CMZN_NODE_VALUE_LABEL_D2_DS2DS3,
		CMZN_NODE_VALUE_LABEL_D3_DS1DS2DS3
	};
	FE_value diff, sum, *values = NULL;
	int inverse_match, inverse_match_version,
		j, k, m, n, map_derivatives, match_version, match, node_number,
		return_code;

	if (node && data)
	{
		return_code = 1;
		map_derivatives = 1;
		const FE_node_field *node_field = node->getNodeField(data->field);
		if (node_field)
		{
			// Does this node have any versions?
			// We can only apply our rules for derivatives if all the components
			// have the same number of versions
			int number_of_versions = -1;
			for (int c = 0; c < data->number_of_components; ++c)
			{
				const FE_node_field_template *nft = node_field->getComponent(c);
				const int component_number_of_versions = nft->getMaximumVersionsCount();
				if (c == 0)
				{
					number_of_versions = component_number_of_versions;
				}
				else if (number_of_versions != component_number_of_versions)
				{
					map_derivatives = 0;
					/* Just see if we have any versions and if so map positions */
					if (number_of_versions < component_number_of_versions)
					{
						number_of_versions = component_number_of_versions;
					}
				}
			}

			if (1 < number_of_versions)
			{
				node_number = get_FE_node_identifier(node);
				fprintf(data->ipmap_file, " Node number [    1]:     %d\n",
					node_number);

				if (map_derivatives)
				{
					/* Store the values for all versions and components and derivatives */
					ALLOCATE(values, FE_value, number_of_versions *
						data->number_of_components * data->maximum_number_of_derivatives);

					for (int c = 0 ; c < data->number_of_components ; ++c)
					{
						for (j = 0 ; j < number_of_versions ; j++)
						{
							for (k = 0 ; k < data->number_of_derivatives[c] ; k++)
							{
								get_FE_nodal_FE_value_value(node, data->field, /*component_number*/c,
									derivativeValueLabels[k], /*version*/j, /*time*/0.0, values +
									j * data->number_of_components * data->maximum_number_of_derivatives
									+ c * data->maximum_number_of_derivatives + k);
							}
							for ( ; k < data->maximum_number_of_derivatives ; k++)
							{
								values[j * data->number_of_components * data->maximum_number_of_derivatives
									+ c * data->maximum_number_of_derivatives + k] = 0.0;
							}
						}
					}
				}

				for (int c = 0; c < data->number_of_components; ++c)
				{
					const FE_node_field_template *nft = node_field->getComponent(c);
					fprintf(data->ipmap_file, " For the Xj(%d) coordinate:\n", c + 1);
					const int component_number_of_versions = nft->getMaximumVersionsCount();

					/* We don't map out the first version */
					fprintf(data->ipmap_file, " For version number%2d:\n", 1);

					fprintf(data->ipmap_file, " Is the nodal position mapped out [N]: N\n");

					for (k = 0 ; k < data->number_of_derivatives[c] ; k++)
					{
						fprintf(data->ipmap_file, " Is the derivative wrt direction%s is mapped out [N]: N\n",
							value_strings[k]);
					}

					/* Other versions match always for the position and then
						if the derivatives are the same */
					for (j = 1 ; j < component_number_of_versions ; j++)
					{
						fprintf(data->ipmap_file, " For version number%2d:\n", j + 1);

						fprintf(data->ipmap_file, " Is the nodal position mapped out [N]: Y\n");
						fprintf(data->ipmap_file, " Enter node, version, direction, derivative numbers to map to [1,1,1,1]: %d 1 %d 1\n", node_number, c + 1);
						fprintf(data->ipmap_file, " Enter the mapping coefficient [1]: 0.10000E+01\n");

						for (k = 0 ; k < data->number_of_derivatives[c] ; k++)
						{
							if (map_derivatives &&
								/* Only try for first deriavites */
								((k == 0) || (k == 1) || (k == 3)))
							{
								/* Look back to see if there is any derivative in the same
									or opposite direction */
								match_version = 0;
								inverse_match_version = 0;
								for (m = 0 ; !match_version && !inverse_match_version &&
									 (m < j) ; m++)
								{
									/* Check all components, this means we are doing
										this check repeatedly for each component unnecessarily
										but that is easier than storing the mappings */
									match = 1;
									inverse_match = 1;
									n = 0;
									while ((match || inverse_match)
										&& n < data->number_of_components)
									{
										diff = values[/*this version*/j *
											data->number_of_components *
											data->maximum_number_of_derivatives +
											n * data->maximum_number_of_derivatives + k]
											- values[/*other version*/m *
											data->number_of_components *
											data->maximum_number_of_derivatives +
												n * data->maximum_number_of_derivatives + k];
										sum = values[/*this version*/j *
											data->number_of_components *
											data->maximum_number_of_derivatives +
											n * data->maximum_number_of_derivatives + k]
											+ values[/*other version*/m *
											data->number_of_components *
											data->maximum_number_of_derivatives +
												n * data->maximum_number_of_derivatives + k];
										if ((diff > 1e-8) || (diff < -1e-8))
										{
											match = 0;
										}
										if ((sum > 1e-8) || (sum < -1e-8))
										{
											inverse_match = 0;
										}
										n++;
									}
									if (match)
									{
										match_version = m + 1;
									}
									if (inverse_match)
									{
										inverse_match_version = m + 1;
									}
								}
								if (match_version)
								{
									fprintf(data->ipmap_file, " Is the derivative wrt direction%s is mapped out [N]: Y\n",
										value_strings[k]);
									fprintf(data->ipmap_file, " Enter node, version, direction, derivative numbers to map to [1,1,1,1]: %d %d %d %d\n",
										node_number, match_version,
										c + 1, k + 2);  /* k + 1 for nodal value + 1 for array start at 1 */
									fprintf(data->ipmap_file, " Enter the mapping coefficient [1]: 0.10000E+01\n");
								}
								else if (inverse_match_version)
								{
									fprintf(data->ipmap_file, " Is the derivative wrt direction%s is mapped out [N]: Y\n",
											  value_strings[k]);
									fprintf(data->ipmap_file, " Enter node, version, direction, derivative numbers to map to [1,1,1,1]: %d %d %d %d\n",
										node_number, inverse_match_version,
										c + 1, k + 2);
									fprintf(data->ipmap_file, " Enter the mapping coefficient [1]: -0.10000E+01\n");
								}
								else
								{
									fprintf(data->ipmap_file, " Is the derivative wrt direction%s is mapped out [N]: N\n",
										value_strings[k]);
								}
							}
							else
							{
								fprintf(data->ipmap_file, " Is the derivative wrt direction%s is mapped out [N]: N\n",
									value_strings[k]);
							}
						}
					}
				}
				fprintf(data->ipmap_file, "\n");
				if (map_derivatives)
				{
					DEALLOCATE(values);
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"write_cm_FE_nodal_mapping.  Missing element_field");
		return_code = 0;
	}
	return (return_code);
}

/**
 * Writes text for an <ipnode_file> to support <field>, with optional <ipmap>.
 */
static int write_ipnode_file(FILE *ipnode_file, FILE *ipmap_file,
	cmzn_region *region, cmzn_field_group *group, struct FE_field *field)
{
	int return_code = 1;
	if (ipnode_file && region && field)
	{
		int i;
		fprintf(ipnode_file, " CMISS Version 1.21 ipnode File Version 2\n");
		fprintf(ipnode_file, " Heading: cmgui generated file\n\n");

		struct FE_node_write_cm_check_node_values_data cm_node_data;
		cm_node_data.number_of_nodes = 0;
		cm_node_data.field = field;
		cm_node_data.number_of_components = get_FE_field_number_of_components(field);
		for (i = 0 ; i < cm_node_data.number_of_components ; i++)
		{
			cm_node_data.number_of_derivatives[i] = -1;
			cm_node_data.has_versions[i] = 0;
		}
		cm_node_data.number_of_nodes_with_versions = 0;
		cm_node_data.maximum_number_of_derivatives = 0;
		cm_node_data.ipnode_file = ipnode_file;
		cm_node_data.ipmap_file = ipmap_file;

		cmzn_fieldmodule *fieldmodule = cmzn_region_get_fieldmodule(region);
		cmzn_nodeset *nodeset = cmzn_fieldmodule_find_nodeset_by_field_domain_type(fieldmodule, CMZN_FIELD_DOMAIN_TYPE_NODES);
		cmzn_fieldmodule_destroy(&fieldmodule);
		if (group)
		{
			cmzn_field_node_group *node_group = cmzn_field_group_get_field_node_group(group, nodeset);
			cmzn_nodeset_destroy(&nodeset);
			if (node_group)
			{
				nodeset = cmzn_nodeset_group_base_cast(cmzn_field_node_group_get_nodeset_group(node_group));
				cmzn_field_node_group_destroy(&node_group);
			}
		}

		cmzn_nodeiterator *iterator;
		cmzn_node *node;

		if (nodeset)
		{
			iterator = cmzn_nodeset_create_nodeiterator(nodeset);
			if (iterator)
			{
				while (0 != (node = cmzn_nodeiterator_next_non_access(iterator)))
				{
					if (!FE_node_write_cm_check_node_values(node, &cm_node_data))
					{
						return_code = 0;
						break;
					}
				}
				cmzn_nodeiterator_destroy(&iterator);
			}
			else
				return_code = 0;
		}

		if (return_code)
		{
			for (i = 0 ; i < cm_node_data.number_of_components ; i++)
			{
				if (cm_node_data.number_of_derivatives[i] >
					cm_node_data.maximum_number_of_derivatives)
				{
					cm_node_data.maximum_number_of_derivatives =
						cm_node_data.number_of_derivatives[i];
				}
			}

			fprintf(ipnode_file, " The number of nodes is [1]: %d\n",
				cm_node_data.number_of_nodes);
			fprintf(ipnode_file, " Number of coordinates [3]: %d\n",
				cm_node_data.number_of_components);
			for (i = 0 ; i < cm_node_data.number_of_components ; i++)
			{
				if (cm_node_data.has_versions[i])
				{
					fprintf(ipnode_file, " Do you want prompting for different versions of nj=%d [N]? Y\n",
						i + 1);
				}
				else
				{
					fprintf(ipnode_file, " Do you want prompting for different versions of nj=%d [N]? N\n",
						i + 1);
				}
			}
			for (i = 0 ; i < cm_node_data.number_of_components ; i++)
			{
				fprintf(ipnode_file, " The number of derivatives for coordinate %d is [0]: %d\n",
					i + 1, cm_node_data.number_of_derivatives[i]);
			}

			if (nodeset)
			{
				iterator = cmzn_nodeset_create_nodeiterator(nodeset);
				while (0 != (node = cmzn_nodeiterator_next_non_access(iterator)))
				{
					if (!write_cm_FE_node(node, &cm_node_data))
					{
						return_code = 0;
						break;
					}
				}
				cmzn_nodeiterator_destroy(&iterator);
			}

			if (ipmap_file)
			{
				fprintf(ipmap_file, " CMISS Version 2.0  ipmap File Version 1\n");
				fprintf(ipmap_file, " Heading: cmgui generated file\n\n");

				fprintf(ipmap_file, " Define node position mapping [N]? y\n");
				fprintf(ipmap_file, " The number of nodes with special mappings is [    1]: %d\n",
					cm_node_data.number_of_nodes_with_versions);

				if (nodeset)
				{
					iterator = cmzn_nodeset_create_nodeiterator(nodeset);
					while (0 != (node = cmzn_nodeiterator_next_non_access(iterator)))
					{
						if (!write_cm_FE_nodal_mapping(node, &cm_node_data))
						{
							return_code = 0;
							break;
						}
					}
					cmzn_nodeiterator_destroy(&iterator);
				}
			}
		}
		else
		{
			display_message(ERROR_MESSAGE, "write_ipnode_file.  "
				"Nodes do not have the required consistency to write an ipnode file.");
			return_code = 0;
		}
		cmzn_nodeset_destroy(&nodeset);
	}
	else
	{
		display_message(ERROR_MESSAGE, "write_ipnode_file.  Invalid argument(s)");
		return_code = 0;
	}
	return (return_code);
}

struct FE_element_write_cm_check_element_values_data
{
	int number_of_elements;
	struct FE_field *field;
	int number_of_components;
	int number_of_bases;
	struct FE_basis **basis_array;
	FILE *ipelem_file;
}; /* struct FE_element_write_cm_check_element_values_data */

/**
 * Counts how many elements have the field defined.
 */
static int FE_element_write_cm_check_element_values(
	struct FE_element *element, FE_element_write_cm_check_element_values_data *data)
{
	if (data && (FE_field_is_defined_in_element_not_inherited(data->field, element)))
		++(data->number_of_elements);
	return 1;
}

/**
 * Writes the element to an ipelem file.
 */
static int write_cm_FE_element(
	struct FE_element *element, FE_element_write_cm_check_element_values_data *data)
{
	int return_code;
	if (element && element->getMesh() && data)
	{
		return_code = 1;
		FE_mesh *mesh = element->getMesh();
		FE_nodeset *nodeset = mesh->getNodeset();
		FE_mesh_field_data *meshFieldData = data->field->getMeshFieldData(mesh);
		const int componentCount = get_FE_field_number_of_components(data->field);
		if (!meshFieldData)
		{
			return 1; // not defined on mesh
		}
		if (FE_field_is_defined_in_element_not_inherited(data->field, element))
		{
			FE_basis *fe_basis;
			int element_identifier = element->getIdentifier();
			fprintf(data->ipelem_file, " Element number [    1]:     %d\n",
				element_identifier);
			const int dimension = element->getDimension();

			fprintf(data->ipelem_file, " The number of geometric Xj-coordinates is [3]: %d\n",
				data->number_of_components);
			for (int c = 0 ; c < data->number_of_components ; c++)
			{
				FE_mesh_field_template *mft = meshFieldData->getComponentMeshfieldtemplate(c);
				FE_element_field_template *eft = mft->getElementfieldtemplate(element->getIndex());
				fe_basis = eft->getBasis();
				int basis_number = 0;
				while ((data->basis_array[basis_number] != fe_basis) && (basis_number < data->number_of_bases))
				{
					basis_number++;
				}
				fprintf(data->ipelem_file, " The basis function type for geometric variable %d is [1]:  %d\n",
					c + 1, basis_number + 1);
			}
			int basis_dimension;
			bool first_basis = true;
			for (int b = 0 ; b < data->number_of_bases ; b++)
			{
				FE_basis_get_dimension(data->basis_array[b], &basis_dimension);
				if (dimension == basis_dimension)
				{
					/* Use the first component as it has a basis, could try and match
						which component actually used this basis above */

					const FE_mesh_field_template *mft = meshFieldData->getComponentMeshfieldtemplate(0);
					const FE_element_field_template *eft = mft->getElementfieldtemplate(element->getIndex());
					const FE_mesh_element_field_template_data *meshEFTData = mesh->getElementfieldtemplateData(eft);
					const DsLabelIndex *nodeIndexes = meshEFTData->getElementNodeIndexes(element->getIndex());
					fe_basis = eft->getBasis();
					const int number_of_basis_nodes = FE_basis_get_number_of_nodes(fe_basis);
					if (0 < number_of_basis_nodes)
					{
						if (first_basis)
						{
							fprintf(data->ipelem_file, " Enter the %d global numbers for basis %d:",
								number_of_basis_nodes, b + 1);
						}
						else
						{
							fprintf(data->ipelem_file, " Enter the %d numbers for basis %d [prev]:",
								number_of_basis_nodes, b + 1);
						}
						int *node_number_array, *number_of_versions_array, *versions_array;
						ALLOCATE(number_of_versions_array, int, number_of_basis_nodes);
						ALLOCATE(node_number_array, int, number_of_basis_nodes);
						ALLOCATE(versions_array, int, number_of_basis_nodes);
						bool warnGeneralLinearMap = false;
						bool warnMixedVersions = false;
						for (int j = 0 ; j < number_of_basis_nodes; j++)
						{
							const int number_of_nodal_values = FE_basis_get_number_of_functions_per_node(fe_basis, j);
							int localNodeIndex = -1;
							int version = -1;
							for (int v = 0; v < number_of_nodal_values; ++v)
							{
								const int functionNumber = FE_basis_get_function_number_from_node_function(fe_basis, j, v);
								const int termCount = eft->getFunctionNumberOfTerms(functionNumber);
								if (termCount > 1)
									warnGeneralLinearMap = true;
								for (int t = 0; t < termCount; ++t)
								{
									if (localNodeIndex == -1)
										localNodeIndex = eft->getTermLocalNodeIndex(functionNumber, t);
									else if (eft->getTermLocalNodeIndex(functionNumber, t) != localNodeIndex)
										warnGeneralLinearMap = true;
									if (version == -1)
										version = eft->getTermNodeVersion(functionNumber, t);
									else if (eft->getTermNodeVersion(functionNumber, t) != version)
										warnMixedVersions = true;
								}
							}
							FE_node *node;
							if (nodeIndexes && (localNodeIndex >= 0))
							{
								node_number_array[j] = nodeset->getNodeIdentifier(nodeIndexes[localNodeIndex]);
								node = nodeset->getNode(nodeIndexes[localNodeIndex]);
							}
							else
							{
								node_number_array[j] = -1;
								node = 0;
							}
							const FE_node_field *node_field = node->getNodeField(data->field);
							if (node_field)
							{
								const FE_node_field_template *nft = node_field->getComponent(/*component*/0);
								number_of_versions_array[j] = nft->getMaximumVersionsCount();
							}
							else
							{
								number_of_versions_array[j] = 0;
							}
							versions_array[j] = version;

							fprintf(data->ipelem_file, " %d", node_number_array[j]);
						}
						if (warnGeneralLinearMap)
						{
							display_message(WARNING_MESSAGE,
								"Element %d field %s uses a general linear map which is not supported in ipelem format; using first term only.",
								cmzn_element_get_identifier(element), get_FE_field_name(data->field));
						}
						if (warnMixedVersions)
						{
							display_message(WARNING_MESSAGE,
								"Element %d field %s gets mixed versions from nodes which is not supported in ipelem format; using first value found.",
								cmzn_element_get_identifier(element), get_FE_field_name(data->field));
						}
						fprintf(data->ipelem_file, "\n");
						if (first_basis)
						{
							for (int j = 0 ; j < number_of_basis_nodes ; j++)
							{
								if (1 < number_of_versions_array[j])
								{
									int occurrences = 1; // need occurrence number if repeated nodes
									for (int k = j - 1; 0 <= k; --k)
									{
										if (node_number_array[k] == node_number_array[j])
											++occurrences;
									}
									/* Required to specify version to use for each different coordinate,
										just specify the same versions for now. */
									for (int c = 0; c < componentCount; ++c)
									{
										fprintf(data->ipelem_file,
											" The version number for occurrence  %d of node %5d"
											", njj=%d is [ 1]: %d\n", occurrences, node_number_array[j],
											c + 1, versions_array[j] + 1);
									}
								}
							}
							first_basis = false;
						}
						DEALLOCATE(number_of_versions_array);
						DEALLOCATE(node_number_array);
						DEALLOCATE(versions_array);
					}
				}
			}
			fprintf(data->ipelem_file, "\n");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"write_cm_FE_element.  Missing element_field");
		return_code = 0;
	}
	return (return_code);
}

/**
 * Writes text for an <ipelem_file> to support <field>.
 */
static int write_ipelem_file(FILE *ipelem_file,
	cmzn_region *region, cmzn_field_group *group, struct FE_field *field,
	struct write_cm_files_data *data)
{
	int return_code = 1;
	if (ipelem_file && region && field)
	{
		fprintf(ipelem_file, " CMISS Version 1.21 ipelem File Version 2\n");
		fprintf(ipelem_file, " Heading: cmgui generated file\n\n");

		struct FE_element_write_cm_check_element_values_data cm_element_data;
		cm_element_data.number_of_elements = 0;
		cm_element_data.field = field;
		cm_element_data.number_of_components = get_FE_field_number_of_components(field);
		cm_element_data.number_of_bases = data->number_of_bases;
		cm_element_data.basis_array = data->basis_array;
		cm_element_data.ipelem_file = ipelem_file;

		cmzn_fieldmodule *fieldmodule = cmzn_region_get_fieldmodule(region);
		// stage 1 = check, stage 2 = write
		for (int stage = 1; stage <= 2; ++stage)
		{
			if (stage == 2)
			{
				fprintf(ipelem_file, " The number of elements is [1]: %d\n\n",
					cm_element_data.number_of_elements);
			}
			for (int dimension = MAXIMUM_ELEMENT_XI_DIMENSIONS; 0 < dimension; --dimension)
			{
				cmzn_mesh *mesh = cmzn_fieldmodule_find_mesh_by_dimension(fieldmodule, dimension);
				cmzn_elementiterator *iterator = 0;
				if (group)
				{
					cmzn_field_element_group *element_group = cmzn_field_group_get_field_element_group(group, mesh);
					if (element_group)
					{
						cmzn_mesh_group *mesh_group = cmzn_field_element_group_get_mesh_group(element_group);
						iterator = cmzn_mesh_create_elementiterator(cmzn_mesh_group_base_cast(mesh_group));
						cmzn_mesh_group_destroy(&mesh_group);
						cmzn_field_element_group_destroy(&element_group);
					}
				}
				else
					iterator = cmzn_mesh_create_elementiterator(mesh);
				if (iterator)
				{
					cmzn_element *element;
					while (0 != (element = cmzn_elementiterator_next_non_access(iterator)))
					{
						if (stage == 1)
							return_code = FE_element_write_cm_check_element_values(element, &cm_element_data);
						else
							return_code = write_cm_FE_element(element, &cm_element_data);
						if (!return_code)
						{
							if (stage == 1)
							{
								display_message(ERROR_MESSAGE, "write_ipelem_file.  "
									"Elements do not have the required consistency to write an ipelem file.");
								return_code = 0;
							}
							break;
						}
					}
					cmzn_elementiterator_destroy(&iterator);
				}
			}
		}
		cmzn_fieldmodule_destroy(&fieldmodule);
	}
	else
	{
		display_message(ERROR_MESSAGE, "write_ipelem_file.  Invalid argument(s)");
		return_code = 0;
	}
	return (return_code);
}

/*
Global functions
----------------
*/

int write_cm_files(FILE *ipcoor_file, FILE *ipbase_file,
	FILE *ipnode_file, FILE *ipelem_file, FILE *ipmap_file,
	cmzn_region *region, cmzn_field_group *group,
	cmzn_field *field)
{
	int return_code;
	FE_field *fe_field = 0;
	Computed_field_get_type_finite_element(field, &fe_field);
	if (ipcoor_file && ipbase_file && ipnode_file && ipelem_file && region && fe_field)
	{
		write_cm_files_data data;
		data.number_of_bases = 0;
		data.basis_array = (struct FE_basis **)NULL;

		return_code = write_ipcoor_file(ipcoor_file, fe_field);
		if (return_code)
		{
			return_code = write_ipbase_file(ipbase_file, region, group, fe_field, &data);
		}
		if (return_code)
		{
			return_code = write_ipnode_file(ipnode_file, ipmap_file,
				region, group, fe_field);
		}
		if (return_code)
		{
			return_code = write_ipelem_file(ipelem_file, region, group, fe_field, &data);
		}
		if (!return_code)
		{
			display_message(ERROR_MESSAGE, "write_cm_FE_region.  Failed");
			return_code = 0;
		}
		if (data.basis_array)
			DEALLOCATE(data.basis_array);
		if (return_code)
			return_code = CMZN_OK;
		else
			return_code = CMZN_ERROR_GENERAL;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"write_cm_files.  Invalid argument(s)");
		return_code = CMZN_ERROR_ARGUMENT;
	}
	return (return_code);
}
