/*******************************************************************************
FILE : snake.c

LAST MODIFIED : 29 March 2006

DESCRIPTION :
Functions for making a snake of 1-D cubic Hermite elements from a chain of
data points.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <math.h>
#include <stdio.h>

#include "opencmiss/zinc/elementbasis.h"
#include "opencmiss/zinc/elementtemplate.h"
#include "opencmiss/zinc/fieldcache.h"
#include "opencmiss/zinc/fieldfiniteelement.h"
#include "opencmiss/zinc/fieldmodule.h"
#include "opencmiss/zinc/mesh.h"
#include "opencmiss/zinc/nodeset.h"
#include "opencmiss/zinc/nodetemplate.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_finite_element.h"
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_mesh.hpp"
#include "finite_element/finite_element_nodeset.hpp"
#include "finite_element/finite_element_region.h"
#include "finite_element/snake.h"
#include "general/debug.h"
#include "general/matrix_vector.h"
#include "general/message.h"

/*
Module Constants
----------------
*/

/* gauss point positions and weights are on [-0.5, 0.5]. Both taken from CM */
double gauss_point[4] = { -0.4305681557970260, -0.1699905217924280, 0.1699905217924280, 0.4305681557970260 };
double gauss_weight[4] = { 0.1739274225687270, 0.3260725774312730, 0.3260725774312730, 0.1739274225687270 };

/*
Module functions
----------------
*/

struct FE_node_accumulate_length_data
/*******************************************************************************
LAST MODIFIED : 3 May 2006

DESCRIPTION :
User data for FE_node_accumulate_length function.
<lengths> must point to as many FE_values as there are nodes passed to the
function, while <coordinates> must have space for
number_of_coordinate_components per node.
Set node_number to zero before passing.
==============================================================================*/
{
	cmzn_fieldcache_id field_cache;
	FE_value *fitting_field_values;
	FE_value *weights;
	/* Space to evaluate the coordinate field and keep the previous value */
	FE_value *coordinates;
	FE_value *lengths;
	int node_number;
	struct Computed_field *coordinate_field;
	struct Computed_field *weight_field;
	struct LIST(FE_field) *fe_field_list;
	struct FE_node *current_node;
};

static int FE_field_evaluate_snake_position(struct FE_field *field,
	void *accumulate_data_void)
/*******************************************************************************
LAST MODIFIED : 29 March 2006

DESCRIPTION :
Evaluates the <field> into the <fitting_field_values>.
==============================================================================*/
{
	int i, number_of_components, return_code;
	struct FE_node_accumulate_length_data *accumulate_data;

	ENTER(FE_node_accumulate_length);
	if (field && (accumulate_data =
		(struct FE_node_accumulate_length_data *)accumulate_data_void))
	{
		return_code = 1;
		number_of_components = get_FE_field_number_of_components(field);
		for (i = 0; (i < number_of_components) && return_code; i++)
		{
			const int result = get_FE_nodal_FE_value_value(accumulate_data->current_node,
				field, /*component_number*/i, CMZN_NODE_VALUE_LABEL_VALUE, /*version*/0,
				/*time*/0, accumulate_data->fitting_field_values);
			if (CMZN_OK == result)
			{
				accumulate_data->fitting_field_values++;
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"FE_field_evaluate_snake_position.  Field %s component not defined at node or data %d",
					get_FE_field_name(field), 
					get_FE_node_identifier(accumulate_data->current_node));
				return_code = 0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_field_evaluate_snake_position.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_field_evaluate_snake_position */

static int FE_node_accumulate_length(struct FE_node *node,
	struct FE_node_accumulate_length_data *accumulate_data)
/*******************************************************************************
LAST MODIFIED : 3 May 2006

DESCRIPTION :
Calculates the coordinates and length from the first node.
==============================================================================*/
{
	double sum;
	FE_value *coordinates, distance, *lengths;
	int i, node_number, number_of_components, return_code;
	struct Computed_field *coordinate_field;

	ENTER(FE_node_accumulate_length);
	if ((node) && (accumulate_data) &&
		(coordinates = accumulate_data->coordinates) &&
		(lengths = accumulate_data->lengths) &&
		(0 <= (node_number = accumulate_data->node_number)) &&
		(coordinate_field = accumulate_data->coordinate_field) &&
		(1 < (number_of_components =
			cmzn_field_get_number_of_components(coordinate_field))))
	{
		accumulate_data->current_node = node;
		return_code = FOR_EACH_OBJECT_IN_LIST(FE_field)(
			FE_field_evaluate_snake_position, (void *)accumulate_data,
			accumulate_data->fe_field_list);

		cmzn_fieldcache_set_node(accumulate_data->field_cache, node);
		if (return_code)
		{
			if ((CMZN_OK == cmzn_field_evaluate_real(coordinate_field, accumulate_data->field_cache, number_of_components, coordinates)))
			{
				if (0 == node_number)
				{
					lengths[0] = 0.0;
					for (i = 0; i < number_of_components; i++)
					{
						coordinates[i + number_of_components] = coordinates[i];
					}
				}
				else
				{
					sum = 0.0;
					for (i = 0; i < number_of_components; i++)
					{
						distance = (coordinates[i] - coordinates[i + number_of_components]);
						coordinates[i + number_of_components] = coordinates[i];
						sum += distance*distance;
					}
					distance = sqrt(sum);
					lengths[node_number] = lengths[node_number - 1] + distance;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"FE_node_accumulate_length.  Unable to evaluate coordinate field.");
				return_code = 0;
			}
		}
		if (return_code && accumulate_data->weight_field)
		{
			if (CMZN_OK != cmzn_field_evaluate_real(accumulate_data->weight_field,
				accumulate_data->field_cache, /*number_of_values*/1, &accumulate_data->weights[node_number]))
			{
				display_message(ERROR_MESSAGE, "FE_node_accumulate_length.  "
					"Unable to evaluate weight field.");
				return_code = 0;
			}
		}
		accumulate_data->node_number++;
		accumulate_data->current_node = (struct FE_node *)NULL;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_node_accumulate_length.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_node_accumulate_length */

int calculate_Hermite_basis_1d(double xi, double *phi)
/*******************************************************************************
LAST MODIFIED : 9 May 2001

DESCRIPTION :
Returns 1-D Hermite basis functions at <xi> in [0.0, 1.0].
<phi> must point to space for 4 doubles, where, on return:
<phi>[0] multiplies the position at the start of the curve.
<phi>[1] multiplies the derivative at the start of the curve.
<phi>[2] multiplies the position at the end of the curve.
<phi>[3] multiplies the derivative at the end of the curve.
==============================================================================*/
{
	double xi_2, xi_3;
	int return_code;

	ENTER(calculate_Hermite_basis_1d);
	if ((0.0 <= xi) && (1.0 >= xi) && phi)
	{
		xi_2 = xi*xi;
		xi_3 = xi_2*xi;
		phi[0] = 2.0*xi_3 - 3.0*xi_2 + 1.0;
		phi[1] = xi_3 - 2.0*xi_2 + xi;
		phi[2] = -2.0*xi_3 + 3.0*xi_2;
		phi[3] = xi_3 - xi_2;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"calculate_Hermite_basis_1d.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* calculate_Hermite_basis_1d */

int calculate_Hermite_basis_1d_derivatives(double xi, double *dphi_dxi)
/*******************************************************************************
LAST MODIFIED : 14 May 2001

DESCRIPTION :
Returns first derivatives of 1-D Hermite basis functions at <xi> in [0.0, 1.0].
<dphi_dxi> must point to space for 4 doubles, where, on return:
<dphi_dxi>[0] multiplies the position at the start of the curve.
<dphi_dxi>[1] multiplies the derivative at the start of the curve.
<dphi_dxi>[2] multiplies the position at the end of the curve.
<dphi_dxi>[3] multiplies the derivative at the end of the curve.
==============================================================================*/
{
	double xi_2;
	int return_code;

	ENTER(calculate_Hermite_basis_1d_derivatives);
	if ((0.0 <= xi) && (1.0 >= xi) && dphi_dxi)
	{
		xi_2 = xi*xi;
		dphi_dxi[0] = 6.0*(xi_2 - xi);
		dphi_dxi[1] = 3.0*xi_2 - 4.0*xi + 1;
		dphi_dxi[2] = -dphi_dxi[0];
		dphi_dxi[3] = 3.0*xi_2 - 2.0*xi;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"calculate_Hermite_basis_1d_derivatives.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* calculate_Hermite_basis_1d_derivatives */

int calculate_Hermite_basis_1d_second_derivatives(double xi, double *d2phi_dxi2)
/*******************************************************************************
LAST MODIFIED : 14 May 2001

DESCRIPTION :
Returns second derivatives of 1-D Hermite basis functions at <xi> in [0.0, 1.0].
<d2phi_dxi2> must point to space for 4 doubles, where, on return:
<d2phi_dxi2>[0] multiplies the position at the start of the curve.
<d2phi_dxi2>[1] multiplies the derivative at the start of the curve.
<d2phi_dxi2>[2] multiplies the position at the end of the curve.
<d2phi_dxi2>[3] multiplies the derivative at the end of the curve.
==============================================================================*/
{
	int return_code;

	ENTER(calculate_Hermite_basis_1d_second_derivatives);
	if ((0.0 <= xi) && (1.0 >= xi) && d2phi_dxi2)
	{
		d2phi_dxi2[0] = 12.0*xi - 6.0;
		d2phi_dxi2[1] = 6.0*xi - 4.0;
		d2phi_dxi2[2] = -d2phi_dxi2[0];
		d2phi_dxi2[3] = 6.0*xi - 2.0;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"calculate_Hermite_basis_1d_second_derivatives.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* calculate_Hermite_basis_1d_second_derivatives */

struct FE_field_initialise_array_data
{
	int number_of_components;
	int fe_field_index;
	struct FE_field **fe_field_array;
}; /* struct FE_field_initialise_array_data */

static int FE_field_initialise_array(struct FE_field *field,
	void *fe_field_initialise_array_data_void)
/*******************************************************************************
LAST MODIFIED : 29 March 2006

DESCRIPTION :
Iterator function for adding <field> to <field_list> if not currently in it.
==============================================================================*/
{
	int return_code;
	struct FE_field_initialise_array_data *data;

	ENTER(FE_field_initialise_array);
	if (field && (data = (struct FE_field_initialise_array_data *)fe_field_initialise_array_data_void))
	{
		data->number_of_components += get_FE_field_number_of_components(field);
		data->fe_field_array[data->fe_field_index] = field;
		data->fe_field_index++;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_field_initialise_array.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* FE_field_initialise_array */

/*
Global functions
----------------
*/

int create_FE_element_snake_from_data_points(
	struct FE_region *fe_region, struct Computed_field *coordinate_field,
	struct Computed_field *weight_field,
	int number_of_fitting_fields, struct Computed_field **fitting_fields,
	cmzn_nodeset_id data_nodeset,
	int number_of_elements, FE_value density_factor, FE_value stiffness,
	cmzn_nodeset_group_id nodeset_group, cmzn_mesh_group_id mesh_group)
{
	double d, d2phi_dxi2[4] = {0.0, 0.0, 0.0, 0.0}, d2phi_dxi2_m, dxi_ds, dxi_ds_4, double_stiffness,
		double_xi, *force_vectors = NULL, phi[4], phi_m, *pos, *stiffness_matrix  = NULL,
		*stiffness_offset, weight;
	FE_value *coordinates = NULL, *fitting_field_values = NULL, density_multiplier,
		length_multiplier, *lengths = NULL, *value, *weights = NULL, xi;
	int component, element_number, i, *indx = NULL, j, m, n, 
		number_of_components, number_of_coordinate_components, number_of_data,
		number_of_rows, return_code, row, start_row;
	struct FE_element *element;
	struct FE_node **nodes;
	struct FE_node_accumulate_length_data accumulate_data;
	struct FE_field_initialise_array_data initialise_array_data;
	struct LIST(FE_field) *fe_field_list;
	cmzn_fieldmodule_id field_module;
	cmzn_fieldcache_id field_cache;

	ENTER(create_FE_element_snake_from_data_points);
	if (fe_region && coordinate_field && 
		(number_of_fitting_fields > 0) && fitting_fields && data_nodeset &&
		(0 < number_of_elements) &&
		(0.0 <= density_factor) && (1.0 >= density_factor) &&
		(0.0 <= (double_stiffness = (double)stiffness)))
	{
		return_code = 1;
		field_module = cmzn_field_get_fieldmodule(coordinate_field);
		field_cache = cmzn_fieldmodule_create_fieldcache(field_module);
		cmzn_nodeset_id nodeset = (nodeset_group) ? cmzn_nodeset_access(cmzn_nodeset_group_base_cast(nodeset_group)) :
			cmzn_fieldmodule_find_nodeset_by_field_domain_type(field_module, CMZN_FIELD_DOMAIN_TYPE_NODES);
		cmzn_mesh_id mesh = (mesh_group) ? cmzn_mesh_access(cmzn_mesh_group_base_cast(mesh_group)) :
			cmzn_fieldmodule_find_mesh_by_dimension(field_module, 1);
		fe_field_list = Computed_field_array_get_defining_FE_field_list(
			number_of_fitting_fields, fitting_fields);
		const int number_of_fe_fields = NUMBER_IN_LIST(FE_field)(fe_field_list);
		cmzn_field_finite_element_id *define_field_array;
		ALLOCATE(define_field_array, cmzn_field_finite_element_id, number_of_fe_fields);
		if (define_field_array)
		{
			for (int i = 0; i < number_of_fe_fields; ++i)
				define_field_array[i] = 0;
		}
		FE_field **fe_field_array;
		ALLOCATE(fe_field_array, struct FE_field *, number_of_fe_fields);
		if (fe_field_array && define_field_array && nodeset)
		{
			initialise_array_data.number_of_components = 0;
			initialise_array_data.fe_field_index = 0;
			initialise_array_data.fe_field_array = fe_field_array;
			FOR_EACH_OBJECT_IN_LIST(FE_field)(FE_field_initialise_array,
				(void *)&initialise_array_data, fe_field_list);
			for (int i = 0; i < number_of_fe_fields; ++i)
			{
				cmzn_field_id field = cmzn_fieldmodule_find_field_by_name(field_module, get_FE_field_name(fe_field_array[i]));
				define_field_array[i] = cmzn_field_cast_finite_element(field);
				cmzn_field_destroy(&field);
			}
			number_of_components = initialise_array_data.number_of_components;

			number_of_coordinate_components = cmzn_field_get_number_of_components(coordinate_field);

			fitting_field_values = (FE_value *)NULL;
			lengths = (FE_value *)NULL;
			coordinates = (FE_value *)NULL;
			weights = (FE_value *)NULL;
			/* 1. Make table of lengths from first data point up to last */
			if (0 < number_of_components)
			{
				if (1 < (number_of_data = cmzn_nodeset_get_size(data_nodeset)))
				{
					if (ALLOCATE(lengths, FE_value, number_of_data) &&
						ALLOCATE(fitting_field_values, FE_value, number_of_components*number_of_data) &&
						ALLOCATE(coordinates, FE_value, 2 * number_of_coordinate_components)
						&& (!weight_field || ALLOCATE(weights, FE_value, number_of_data)))
					{
						accumulate_data.field_cache = field_cache;
						accumulate_data.fitting_field_values = fitting_field_values;
						accumulate_data.lengths = lengths;
						accumulate_data.coordinates = coordinates;
						accumulate_data.weights = weights;
						accumulate_data.node_number = 0;
						accumulate_data.coordinate_field = coordinate_field;
						accumulate_data.weight_field = weight_field;
						accumulate_data.fe_field_list = fe_field_list;
						cmzn_nodeiterator *iter = cmzn_nodeset_create_nodeiterator(data_nodeset);
						cmzn_node *node;
						while (0 != (node = cmzn_nodeiterator_next_non_access(iter)))
						{
							if (!FE_node_accumulate_length(node, &accumulate_data))
							{
								display_message(ERROR_MESSAGE,
									"create_FE_element_snake_from_data_points.  Could not calculate lengths");
								return_code = 0;
								break;
							}
						}
						if (return_code)
						{
							if (0.0 < lengths[number_of_data - 1])
							{
								if ((0.0 >= stiffness) &&
									((2 + number_of_elements*2) > number_of_data))
								{
									display_message(ERROR_MESSAGE,
										"create_FE_element_snake_from_data_points.  "
										"Not enough data; add more data or a stiffness");
									return_code = 0;
								}
							}
							else
							{
								display_message(ERROR_MESSAGE,
									"create_FE_element_snake_from_data_points.  Zero length");
								return_code = 0;
							}
						}
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"create_FE_element_snake_from_data_points.  "
							"Could not allocate lengths");
						return_code = 0;
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"create_FE_element_snake_from_data_points.  "
						"Not enough data points");
					return_code = 0;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"create_FE_element_snake_from_data_points.  "
					"No values to fit for defining FE_fields of fitting_fields list.");
				return_code = 0;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"create_FE_element_snake_from_data_points.  "
				"Unable to allocate fe_field array.");
			return_code = 0;
		}
		if (return_code)
		{
#if defined (DEBUG_CODE)
			/*???debug*/
			for (i = 0; i < number_of_data; i++)
			{
				printf("length[%d] = %f\n", i, lengths[i]);
			}
#endif /* defined (DEBUG_CODE) */
			/* convert lengths array into combined element:xi coordinates. The integer
				 part of the number is the element number, the fractional part is Xi */
			/* density_factor controls the density of elements along the snake,
				 together with the overall length and density of data points. Values:
				 0.0 = elements/xi are evenly spaced along the lengths;
				 1.0 = elements/xi are evenly spaced along the point numbers.
				 Values from 0.0 change the behaviour continuously between the two */
			length_multiplier = (FE_value)number_of_elements /
				accumulate_data.lengths[number_of_data - 1];
			density_multiplier =
				(FE_value)number_of_elements / (FE_value)(number_of_data - 1);
			for (i = 0; i < number_of_data; i++)
			{
				lengths[i] = (1.0 - density_factor)*lengths[i]*length_multiplier +
					density_factor*(FE_value)i*density_multiplier;
			}
#if defined (DEBUG_CODE)
			/*???debug*/
			for (i = 0; i < number_of_data; i++)
			{
				printf("element:xi[%d] = %f\n", i, accumulate_data.lengths[i]);
			}
#endif /* defined (DEBUG_CODE) */
			/* allocate stiffness_matrix with (number_of_elements + 1)*2 rows*columns,
				 and the force_vectors with (number_of_elements + 1)*2 rows for each
				 coordinate component. The factor or 2 adds the derivative DOFs for
				 Hermite basis functions.
				 Note that the same stiffness_matrix is used to fit each coordinate
				 component, but there is one force/RHS vector per component.
				 The indx array is required for LU_decompose */
			number_of_rows = 2*(number_of_elements + 1);
			if (ALLOCATE(stiffness_matrix, double, number_of_rows*number_of_rows) &&
				ALLOCATE(force_vectors, double, number_of_components*number_of_rows) &&
				ALLOCATE(indx, int, number_of_rows))
			{
				/* clear the stiffness_matrix and force_vectors */
				pos = stiffness_matrix;
				for (i = number_of_rows*number_of_rows; 0 < i; i--)
				{
					*pos = 0.0;
					pos++;
				}
				pos = force_vectors;
				for (i = number_of_components*number_of_rows; 0 < i; i--)
				{
					*pos = 0.0;
					pos++;
				}
				/* assemble the stiffness matrix and force_vectors */
				value = fitting_field_values;
				weight = 1.0;
				for (i = 0; i < number_of_data; i++)
				{
					element_number = (int)lengths[i];
					/* handle points at the end of the line specially */
					if (element_number >= number_of_elements)
					{
						element_number = number_of_elements - 1;
						xi = 1.0;
					}
					else
					{
						xi = lengths[i] - element_number;
					}
					start_row = element_number*2;
					calculate_Hermite_basis_1d(xi, phi);
					if (weight_field)
					{
						weight = weights[i];
					}
					for (m = 0; m < 4; m++)
					{
						phi_m = phi[m];
						row = start_row + m;
						stiffness_offset =
							stiffness_matrix + row*number_of_rows + start_row;
						for (n = 0; n < 4; n++)
						{
							stiffness_offset[n] += weight * phi_m * phi[n];
						}
						for (n = 0; n < number_of_components; n++)
						{
							force_vectors[n*number_of_rows + row] += 
								weight * phi_m * value[n];
						}
					}
					value += number_of_components;
				}
				/* add stiffness penalising second derivatives, if non-zero */
				if (0.0 < double_stiffness)
				{
					dxi_ds = (double)number_of_elements /
						(double)lengths[number_of_data - 1];
					dxi_ds_4 = dxi_ds*dxi_ds*dxi_ds*dxi_ds;
					for (element_number = 0; element_number < number_of_elements;
						element_number++)
					{
						start_row = element_number*2;
						for (i = 0; i < 4; i++)
						{
							weight = dxi_ds_4*double_stiffness*gauss_weight[i];
							double_xi = gauss_point[i] + 0.5;
							calculate_Hermite_basis_1d_second_derivatives(double_xi,
								d2phi_dxi2);
							for (m = 0; m < 4; m++)
							{
								d2phi_dxi2_m = d2phi_dxi2[m]*weight;
								row = start_row + m;
								stiffness_offset =
									stiffness_matrix + row*number_of_rows + start_row;
								for (n = 0; n < 4; n++)
								{
									stiffness_offset[n] += d2phi_dxi2_m*d2phi_dxi2[n];
								}
							}
						}
					}
				}
#if defined (DEBUG_CODE)
				/*???debug*/
				printf("Stiffness Matrix:\n");
				print_matrix(number_of_rows,number_of_rows,stiffness_matrix,"%8.4f");
				for (n = 0; n < number_of_components; n++)
				{
					printf("Force Vector %d:\n", n + 1);
					print_matrix(number_of_rows,1,
						force_vectors + n*number_of_rows,"%8.4f");
				}
#endif /* defined (DEBUG_CODE) */
				if (LU_decompose(number_of_rows, stiffness_matrix, indx, &d,/*singular_tolerance*/1.0e-12))
				{
					for (n = 0; (n < number_of_components) && return_code; n++)
					{
						if (!LU_backsubstitute(number_of_rows, stiffness_matrix, indx,
							force_vectors + n*number_of_rows))
						{
							display_message(ERROR_MESSAGE,
								"create_FE_element_snake_from_data_points.  "
								"Could not get solution for force_vector %d", n + 1);
							return_code = 0;
						}
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"create_FE_element_snake_from_data_points.  "
						"Singular stiffness matrix");
					return_code = 0;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"create_FE_element_snake_from_data_points.  "
					"Could not allocate stiffness matrix and force vector for fitting");
				return_code = 0;
			}
		}
		if (return_code)
		{
			FE_region_begin_change(fe_region);
#if defined (DEBUG_CODE)
			/*???debug*/
			for (n = 0; n < number_of_components; n++)
			{
				printf("Solution: Coordinate %d:\n", n + 1);
				print_matrix(number_of_rows,1,
					force_vectors + n*number_of_rows,"%8.4f");
			}
#endif /* defined (DEBUG_CODE) */
			nodes = (struct FE_node **)NULL;
			if (ALLOCATE(nodes, struct FE_node *, number_of_elements + 1))
			{
				// clear nodes so safe to DEACCESS later
				for (j = 0; j <= number_of_elements; j++)
				{
					nodes[j] = (struct FE_node *)NULL;
				}
				// create a template node interpolating the target fields with 1-D Hermite basis
				cmzn_nodetemplate_id nodetemplate = cmzn_nodeset_create_nodetemplate(nodeset);
				if (nodetemplate)
				{
					for (n = 0 ; return_code && (n < number_of_fe_fields) ; n++)
					{
						cmzn_field_id define_field = cmzn_field_finite_element_base_cast(define_field_array[n]);
						if ((CMZN_OK != cmzn_nodetemplate_define_field(nodetemplate, define_field))
							|| (CMZN_OK != cmzn_nodetemplate_set_value_number_of_versions(nodetemplate,
								define_field, -1, CMZN_NODE_VALUE_LABEL_D_DS1, 1)))
						{
							display_message(ERROR_MESSAGE,
								"create_FE_element_snake_from_data_points.  Could not define field on node template");
							return_code = 0;
							break;
						}
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"create_FE_element_snake_from_data_points.  Could not create node template");
					return_code = 0;
				}
				/* create the nodes in the snake as copies of the template, access them
				   in the nodes array, set their coordinates and derivatives and add
				   them to the manager and node_group */
				cmzn_fieldcache_id cache = cmzn_fieldmodule_create_fieldcache(field_module);
				for (j = 0; (j <= number_of_elements) && return_code; j++)
				{
					nodes[j] = cmzn_nodeset_create_node(nodeset, -1, nodetemplate);
					if (nodes[j])
					{
						cmzn_fieldcache_set_node(cache, nodes[j]);
						/* set the coordinate and derivatives */
						component = 0;
						for (n = 0; (n < number_of_fe_fields) && return_code; n++)
						{
							cmzn_field_id define_field = cmzn_field_finite_element_base_cast(define_field_array[n]);
							int componentCount = cmzn_field_get_number_of_components(define_field);
							for (i = 0; i < componentCount; ++i)
							{
								if ((CMZN_OK != cmzn_field_finite_element_set_node_parameters(define_field_array[n],
									cache, i + 1, CMZN_NODE_VALUE_LABEL_VALUE, 1, 1, &force_vectors[component*number_of_rows + j*2]))
									|| (CMZN_OK != cmzn_field_finite_element_set_node_parameters(define_field_array[n],
										cache, i + 1, CMZN_NODE_VALUE_LABEL_D_DS1, 1, 1, &force_vectors[component*number_of_rows + j*2 + 1])))
								{
									display_message(ERROR_MESSAGE,
										"create_FE_element_snake_from_data_points.  Failed to set node parameters");
									return_code = 0;
									break;
								}
								++component;
							}
						}
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"create_FE_element_snake_from_data_points.  Failed to create node from template");
						return_code = 0;
					}
				}
				if (return_code)
				{
					cmzn_elementtemplate_id elementtemplate = cmzn_mesh_create_elementtemplate(mesh);
					const int nodeIndexes[] = { 1, 2 };
					cmzn_elementbasis_id basis = cmzn_fieldmodule_create_elementbasis(field_module, 1, CMZN_ELEMENTBASIS_FUNCTION_TYPE_CUBIC_HERMITE);
					if ((elementtemplate)
						&& cmzn_elementtemplate_set_element_shape_type(elementtemplate, CMZN_ELEMENT_SHAPE_TYPE_LINE)
						&& cmzn_elementtemplate_set_number_of_nodes(elementtemplate, 2))
					{
						for (int i = 0; i < number_of_fe_fields; ++i)
						{
							cmzn_field_id define_field = cmzn_field_finite_element_base_cast(define_field_array[i]);
							if (CMZN_OK != cmzn_elementtemplate_define_field_simple_nodal(elementtemplate,
								define_field, -1, basis, 2, nodeIndexes))
							{
								display_message(ERROR_MESSAGE,
									"create_FE_element_snake_from_data_points.  Could not define field on element template");
								return_code = 0;
								break;
							}
						}
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"create_FE_element_snake_from_data_points.  Could not create element template");
						return_code = 0;
					}
					if (return_code)
					{
						for (j = 0; (j < number_of_elements) && return_code; j++)
						{
							if ((CMZN_OK != cmzn_elementtemplate_set_node(elementtemplate, 1, nodes[j]))
								|| (CMZN_OK != cmzn_elementtemplate_set_node(elementtemplate, 2, nodes[j + 1])))
							{
								display_message(ERROR_MESSAGE,
									"create_FE_element_snake_from_data_points.   Failed to set element nodes");
								return_code = 0;
							}
							else
							{
								element = cmzn_mesh_create_element(mesh, -1, elementtemplate);
								if (!element)
								{
									display_message(ERROR_MESSAGE,
										"create_FE_element_snake_from_data_points.   Failed to create element");
									return_code = 0;
								}
								cmzn_element_destroy(&element);
							}
						}
					}
					cmzn_elementbasis_destroy(&basis);
					cmzn_elementtemplate_destroy(&elementtemplate);
				}
				for (int j = 0; j <= number_of_elements; ++j)
					cmzn_node_destroy(&nodes[j]);
				DEALLOCATE(nodes);
				cmzn_fieldcache_destroy(&cache);
				cmzn_nodetemplate_destroy(&nodetemplate);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"create_FE_element_snake_from_data_points.  "
					"Could not allocate nodes array");
				return_code = 0;
			}
			FE_region_end_change(fe_region);
		}
		if (fitting_field_values)
		{
			DEALLOCATE(fitting_field_values);
		}
		if (lengths)
		{
			DEALLOCATE(lengths);
		}
		if (coordinates)
		{
			DEALLOCATE(coordinates);
		}
		if (weights)
		{
			DEALLOCATE(weights);
		}
		if (stiffness_matrix)
		{
			DEALLOCATE(stiffness_matrix);
		}
		if (force_vectors)
		{
			DEALLOCATE(force_vectors);
		}
		if (indx)
		{
			DEALLOCATE(indx);
		}
		DESTROY(LIST(FE_field))(&fe_field_list);
		if (fe_field_array)
			DEALLOCATE(fe_field_array);
		if (define_field_array)
		{
			for (int i = 0; i < number_of_fe_fields; ++i)
				cmzn_field_finite_element_destroy(&define_field_array[i]);
			DEALLOCATE(define_field_array);
		}
		cmzn_nodeset_destroy(&nodeset);
		cmzn_mesh_destroy(&mesh);
		cmzn_fieldcache_destroy(&field_cache);
		cmzn_fieldmodule_destroy(&field_module);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"create_FE_element_snake_from_data_points.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* create_FE_element_snake_from_data_points */
