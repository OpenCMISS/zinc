/*******************************************************************************
FILE : computed_field_update.c

LAST MODIFIED : 24 August 2006

DESCRIPTION :
Functions for updating values of one computed field from those of another.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "opencmiss/zinc/fieldcache.h"
#include "opencmiss/zinc/fieldderivatives.h"
#include "opencmiss/zinc/fieldfiniteelement.h"
#include "opencmiss/zinc/fieldmodule.h"
#include "opencmiss/zinc/mesh.h"
#include "opencmiss/zinc/nodeset.h"
#include "opencmiss/zinc/status.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_finite_element.h"
#include "computed_field/computed_field_update.h"
#include "computed_field/field_cache.hpp"
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_private.h"
#include "finite_element/finite_element_region.h"
#include "finite_element/finite_element_discretization.h"
#include "general/debug.h"
#include "general/message.h"
#include "mesh/cmiss_element_private.hpp"
#include "mesh/cmiss_node_private.hpp"

int cmzn_nodeset_assign_field_from_source(
	cmzn_nodeset_id nodeset, cmzn_field_id destination_field,
	cmzn_field_id source_field, cmzn_field_id conditional_field,
	FE_value time)
{
	int return_code = CMZN_OK;
	if (nodeset && destination_field && source_field)
	{
		cmzn_field_value_type value_type = cmzn_field_get_value_type(destination_field);
		if (value_type == CMZN_FIELD_VALUE_TYPE_MESH_LOCATION)
		{
			// element_xi fields used to allow locations in multiple meshes, but are now
			// restricted to a single host mesh. This must be discovered if necessary.
			// This only happens when read from legacy EX format or created in Cmgui.
			const int result = cmzn_field_discover_element_xi_host_mesh_from_source(destination_field, source_field);
			if (CMZN_OK != result)
			{
				return result;
			}
		}
		const int componentCount = cmzn_field_get_number_of_components(destination_field);
		// can always evaluate to a string value
		if ((value_type == CMZN_FIELD_VALUE_TYPE_STRING) ||
			((cmzn_field_get_number_of_components(source_field) == componentCount) &&
				(cmzn_field_get_value_type(source_field) == value_type)))
		{
			cmzn_fieldmodule_id fieldmodule = cmzn_field_get_fieldmodule(destination_field);
			cmzn_fieldmodule_begin_change(fieldmodule);

			FE_field *feField = 0;
			Computed_field_get_type_finite_element(destination_field, &feField);
			cmzn_field *dx_dX = 0;
			// if source and destination are both finite element fields, can efficiently transfer parameters
			FE_field *sourceFeField = 0;
			Computed_field_get_type_finite_element(source_field, &sourceFeField);
			if ((feField) && (!sourceFeField) && (value_type == CMZN_FIELD_VALUE_TYPE_REAL)
				&& (1 < componentCount) && (componentCount <= 3)) // 1-3 coordinate components is current restriction on gradient field
			{
				// if source field is a function of destination feField, can numerically calculate derivatives
				struct LIST(FE_field) *definingFeFieldlist = Computed_field_get_defining_FE_field_list(source_field);
				if (IS_OBJECT_IN_LIST(FE_field)(feField, definingFeFieldlist))
				{
					// gradient field works by finite different at nodes; use this to get F = dx/dX to transform derivatives
					dx_dX = cmzn_fieldmodule_create_field_gradient(fieldmodule, source_field, destination_field);
					if (!dx_dX)
					{
						display_message(WARNING_MESSAGE, "Fieldassignment assign.  Cannot evaluate gradient needed for derivatives");
					}
				}
				DESTROY(LIST(FE_field))(&definingFeFieldlist);
			}
			cmzn_fieldcache_id fieldcache = cmzn_fieldmodule_create_fieldcache(fieldmodule);
			FE_value *values = new FE_value[componentCount*2];
			FE_value *values2 = values + componentCount;
			// all fields evaluated at same time so set once
			cmzn_fieldcache_set_time(fieldcache, time);
			cmzn_nodeiterator_id iterator = cmzn_nodeset_create_nodeiterator(nodeset);
			cmzn_node_id node = 0;
			int selected_count = 0;
			int success_count = 0;
			while ((return_code == CMZN_OK) && (0 != (node = cmzn_nodeiterator_next_non_access(iterator))))
			{
				cmzn_fieldcache_set_node(fieldcache, node);
				if ((!conditional_field) || cmzn_field_evaluate_boolean(conditional_field, fieldcache))
				{
					if ((cmzn_field_is_defined_at_location(destination_field, fieldcache)))
					{
						switch (value_type)
						{
						case CMZN_FIELD_VALUE_TYPE_MESH_LOCATION:
							{
								FE_value xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
								cmzn_element_id element = cmzn_field_evaluate_mesh_location(
									source_field, fieldcache, MAXIMUM_ELEMENT_XI_DIMENSIONS, xi);
								if (element)
								{
									if ((CMZN_OK == cmzn_field_assign_mesh_location(destination_field, fieldcache,
										element, MAXIMUM_ELEMENT_XI_DIMENSIONS, xi)))
									{
										++success_count;
									}
									cmzn_element_destroy(&element);
								}
							} break;
						case CMZN_FIELD_VALUE_TYPE_REAL:
							{
								if (feField)
								{
									const FE_node_field *node_field = node->getNodeField(feField);
									if (node_field)
									{
										int assign_count = 0;
										int result = CMZN_OK;
										const int maximumDerivativeNumber = node_field->getMaximumDerivativeNumber();
										if (sourceFeField)
										{
											// special case for assigning finite element parameters directly
											const FE_node_field *source_node_field = node->getNodeField(sourceFeField);
											if (source_node_field)
											{
												for (int d = 0; d <= maximumDerivativeNumber; ++d)
												{
													const cmzn_node_value_label valueLabel = static_cast<cmzn_node_value_label>(CMZN_NODE_VALUE_LABEL_VALUE + d);
													const int versionsCount = node_field->getValueMaximumVersionsCount(valueLabel);
													for (int v = 0; v < versionsCount; ++v)
													{
														result = get_FE_nodal_FE_value_value(node, sourceFeField, /*componentNumber*/-1, valueLabel, v, time, values);
														if (result == CMZN_OK)
														{
															result = set_FE_nodal_FE_value_value(node, feField, /*componentNumber*/-1, valueLabel, v, time, values);
															if ((result == CMZN_OK) || (result == CMZN_WARNING_PART_DONE))
															{
																result = CMZN_OK;
																++assign_count;
															}
															else if (result == CMZN_ERROR_NOT_FOUND)
															{
																result = CMZN_OK;
															}
															else
															{
																break;
															}
														}
														else if (result == CMZN_WARNING_PART_DONE)
														{
															// assign components individually
															for (int c = 0; c < componentCount; ++c)
															{
																if (v < source_node_field->getComponent(c)->getValueNumberOfVersions(valueLabel))
																{
																	result = set_FE_nodal_FE_value_value(node, feField, c, valueLabel, v, time, values + c);
																	if (result == CMZN_OK)
																	{
																		++assign_count;
																	}
																	else if (result == CMZN_ERROR_NOT_FOUND)
																	{
																		result = CMZN_OK;
																	}
																	else
																	{
																		break;
																	}
																}
															}
															if (result != CMZN_OK)
															{
																break;
															}
														}
													}
													if (result != CMZN_OK)
													{
														break;
													}
												}
												if (result != CMZN_OK)
												{
													display_message(ERROR_MESSAGE,
														"cmzn_nodeset_assign_field_from_source.  Failed to evaluate or assign from finite element node field.");
													return_code = result;
												}
											}
										}
										else
										{
											const int FSize = componentCount*componentCount;
											int valueVersionsCount = node_field->getValueMaximumVersionsCount(CMZN_NODE_VALUE_LABEL_VALUE);
											// buffer for storing values for all versions for value and F = dx/dX for transforming derivatives
											FE_value *valuesBuffer = new double[(componentCount*2 + FSize)*valueVersionsCount];
											FE_value *coordinateArrays = valuesBuffer;
											FE_value *sourceArrays = coordinateArrays + componentCount*valueVersionsCount;
											FE_value *FArrays = sourceArrays + componentCount*valueVersionsCount;
											if (!valuesBuffer)
											{
												display_message(ERROR_MESSAGE,
													"cmzn_nodeset_assign_field_from_source.  Failed to allocate real valuesBuffer.");
												return_code = result = CMZN_ERROR_MEMORY;
											}
											int evaluateValueVersionsCount = 0;
											if (return_code == CMZN_OK)
											{
												// get destination coordinate value versions
												for (int v = 0; v < valueVersionsCount; ++v)
												{
													result = get_FE_nodal_FE_value_value(node, feField, /*componentNumber*/-1,
														CMZN_NODE_VALUE_LABEL_VALUE, v, time, coordinateArrays + v*componentCount);
													if ((result == CMZN_WARNING_PART_DONE) && (v > 0))
													{
														// fall back to version 1 for undefined components
														for (int c = 0; c < componentCount; ++c)
														{
															const FE_node_field_template *component = node_field->getComponent(c);
															if (component->getValueNumberOfVersions(CMZN_NODE_VALUE_LABEL_VALUE) < (v + 1))
															{
																(coordinateArrays + v*componentCount)[c] = coordinateArrays[c];
															}
														}
													}
													else if (result != CMZN_OK)
													{
														break;
													}
													++evaluateValueVersionsCount;
												}
											}
											int FCount = 0;
											const bool evaluateDerivatives = (maximumDerivativeNumber > 0) && (dx_dX);
											if ((evaluateValueVersionsCount > 0) && (return_code == CMZN_OK))
											{
												// evaluate dx_dX before assigning value as original value is used in finite difference calculation
												for (int v = 0; v < evaluateValueVersionsCount; ++v)
												{
													if (v > 0)
													{
														fieldcache->setAssignInCacheOnly(true);
														cmzn_field_assign_real(destination_field, fieldcache, componentCount, coordinateArrays + v*componentCount);
													}
													if (CMZN_OK != cmzn_field_evaluate_real(source_field, fieldcache, componentCount, sourceArrays + v*componentCount))
													{
														evaluateValueVersionsCount = v;
														break;
													}
													if (evaluateDerivatives && (CMZN_OK == cmzn_field_evaluate_real(dx_dX, fieldcache, FSize, FArrays + v*FSize)))
													{
														++FCount;
													}
												}
												fieldcache->setAssignInCacheOnly(false);
												// evaluate source values and assign to destination
												for (int v = 0; v < valueVersionsCount; ++v)
												{
													const FE_value *sourceValues = (v < evaluateValueVersionsCount) ? sourceArrays + v*componentCount : sourceArrays;
													result = set_FE_nodal_FE_value_value(node, feField, /*componentNumber*/-1,
														CMZN_NODE_VALUE_LABEL_VALUE, v, time, sourceValues);
													if ((result == CMZN_OK) || (result == CMZN_WARNING_PART_DONE))
													{
														result = CMZN_OK;
														++assign_count;
													}
													else
													{
														break;
													}
												}
												if ((FCount > 0) && (result == CMZN_OK))
												{
													for (int d = 1; d <= maximumDerivativeNumber; ++d)
													{
														const cmzn_node_value_label valueLabel = static_cast<cmzn_node_value_label>(CMZN_NODE_VALUE_LABEL_VALUE + d);
														const int derivativeVersionsCount = node_field->getValueMaximumVersionsCount(valueLabel);
														for (int v = 0; v < derivativeVersionsCount; ++v)
														{
															result = get_FE_nodal_FE_value_value(node, feField, /*componentNumber*/-1, valueLabel, v, time, values);
															if ((result == CMZN_OK) || (result == CMZN_WARNING_PART_DONE))
															{
																// transform derivative by F = dx/dX
																// assume same version of value & F as derivative, falling back to version 1
																// may need to provide control in future
																const FE_value *F = (v < FCount) ? FArrays + v*FSize : FArrays;
																for (int c2 = 0; c2 < componentCount; ++c2)
																{
																	const FE_value *f = F + c2*componentCount;
																	double sum = 0.0;
																	for (int c = 0; c < componentCount; ++c)
																	{
																		sum += f[c]*values[c];
																	}
																	values2[c2] = sum;
																}
																result = set_FE_nodal_FE_value_value(node, feField, /*componentNumber*/-1,
																	valueLabel, v, time, values2);
																if ((result == CMZN_OK) || (result == CMZN_WARNING_PART_DONE))
																{
																	result = CMZN_OK;
																	++assign_count;
																}
																else
																{
																	break;
																}
															}
														}
														if (result != CMZN_OK)
														{
															break;
														}
													}
												}
											}
											delete[] valuesBuffer;
										}
										if ((assign_count > 0) && (result == CMZN_OK))
										{
											++success_count;
										}
									}
								}
								else if ((CMZN_OK == cmzn_field_evaluate_real(source_field, fieldcache, componentCount, values))
									&& (CMZN_OK == cmzn_field_assign_real(destination_field, fieldcache, componentCount, values)))
								{
									++success_count;
								}
							} break;
						case CMZN_FIELD_VALUE_TYPE_STRING:
							{
								char *string_value = cmzn_field_evaluate_string(source_field, fieldcache);
								if (string_value)
								{
									if ((CMZN_OK == cmzn_field_assign_string(destination_field, fieldcache, string_value)))
									{
										++success_count;
									}
									DEALLOCATE(string_value);
								}
							} break;
						default:
							{
								display_message(ERROR_MESSAGE,
									"cmzn_nodeset_assign_field_from_source.  Unsupported value type.");
								return_code = CMZN_ERROR_NOT_IMPLEMENTED;
							} break;
						}
					}
					++selected_count;
				}
			}
			cmzn_nodeiterator_destroy(&iterator);
			if (success_count != selected_count)
			{
				display_message(WARNING_MESSAGE,
					"cmzn_nodeset_assign_field_from_source.  "
					"Only able to set values for %d nodes out of %d\n"
					"  Either source field isn't defined at node "
					"or destination field could not be set.",
					success_count, selected_count);
				if (success_count > 0)
				{
					return_code = CMZN_WARNING_PART_DONE;
				}
				else
				{
					return_code = CMZN_ERROR_NOT_FOUND;
				}
			}
			delete[] values;
			cmzn_fieldcache_destroy(&fieldcache);
			if (dx_dX)
			{
				cmzn_field_destroy(&dx_dX);
			}
			cmzn_fieldmodule_end_change(fieldmodule);
			cmzn_fieldmodule_destroy(&fieldmodule);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"cmzn_nodeset_assign_field_from_source.  "
				"Value type and number of components in source and destination fields must match.");
			return_code = CMZN_ERROR_ARGUMENT;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_nodeset_assign_field_from_source.  Invalid argument(s)");
		return_code = CMZN_ERROR_ARGUMENT;
	}
	return (return_code);
}

struct cmzn_element_assign_grid_field_from_source_data
{
	cmzn_fieldcache_id field_cache;
	int selected_count, success_count, xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	struct Computed_field *source_field;
	struct Computed_field *destination_field;
	struct Element_point_ranges_selection *element_point_ranges_selection;
	struct Computed_field *group_field;
};

int cmzn_element_assign_grid_field_from_source_sub(
	cmzn_element_id element, cmzn_element_assign_grid_field_from_source_data *data)
{
	FE_value *values, xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	int can_select_individual_points, destination_field_is_grid_based,
		element_selected, grid_point_number, i, maximum_element_point_number,
		number_of_ranges, return_code, start, stop;
	struct Element_point_ranges *element_point_ranges;
	struct Element_point_ranges_identifier element_point_ranges_identifier;
	struct Multi_range *selected_ranges;

	ENTER(cmzn_element_assign_grid_field_from_source_sub);
	if (element && data)
	{
		int componentCount = cmzn_field_get_number_of_components(data->source_field);
		element_point_ranges = (struct Element_point_ranges *)NULL;
		return_code = 1;
		if (Computed_field_get_native_discretization_in_element(
			data->destination_field, element,
			element_point_ranges_identifier.number_in_xi))
		{
			destination_field_is_grid_based = 1;
		}
		else
		{
			destination_field_is_grid_based = 0;
		}
		if (data->group_field)
		{
			if ((CMZN_OK == cmzn_fieldcache_set_element(data->field_cache, element)) &&
				cmzn_field_evaluate_boolean(data->group_field, data->field_cache))
			{
				element_selected = 1;
				can_select_individual_points = 0;
			}
			else
			{
				element_selected = 0;
				can_select_individual_points = 1;
			}
		}
		else
		{
			element_selected = 1;
			can_select_individual_points = 1;
		}
		if (destination_field_is_grid_based)
		{
			if (can_select_individual_points &&
				data->element_point_ranges_selection)
			{
				element_point_ranges_identifier.element = element;
				element_point_ranges_identifier.top_level_element = element;
				element_point_ranges_identifier.sampling_mode = CMZN_ELEMENT_POINT_SAMPLING_MODE_CELL_CORNERS;
				/* already set the number_in_xi, above */
				if (0 != (element_point_ranges = ACCESS(Element_point_ranges)(
							FIND_BY_IDENTIFIER_IN_LIST(
							Element_point_ranges, identifier)(&element_point_ranges_identifier,
								Element_point_ranges_selection_get_element_point_ranges_list(
									data->element_point_ranges_selection)))))
				{
					element_selected = 1;
				}
				else
				{
					element_selected = 0;
				}
			}
			else
			{
				element_point_ranges_identifier.element = element;
				element_point_ranges_identifier.top_level_element = element;
				element_point_ranges_identifier.sampling_mode = CMZN_ELEMENT_POINT_SAMPLING_MODE_CELL_CORNERS;
				/* already set the number_in_xi, above */
				element_point_ranges = ACCESS(Element_point_ranges)(
					CREATE(Element_point_ranges)(
						&element_point_ranges_identifier));
				FE_element_get_xi_points(element, 
					element_point_ranges_identifier.sampling_mode,
					element_point_ranges_identifier.number_in_xi,
					element_point_ranges_identifier.exact_xi,
					(cmzn_fieldcache_id)0,
					/*coordinate_field*/(struct Computed_field *)NULL,
					/*density_field*/(struct Computed_field *)NULL,
					&maximum_element_point_number,
					/*xi_points_address*/(FE_value_triple **)NULL);
				Element_point_ranges_add_range(element_point_ranges,
					0, maximum_element_point_number - 1);
			}
		}
		if (element_selected)
		{
			data->selected_count++;
			if (destination_field_is_grid_based &&
				(CMZN_OK == cmzn_fieldcache_set_element(data->field_cache, element)) &&
				cmzn_field_is_defined_at_location(data->source_field, data->field_cache) &&
				ALLOCATE(values, FE_value, componentCount))
			{
				if (element_point_ranges)
				{
					selected_ranges = 
						Element_point_ranges_get_ranges(element_point_ranges);
						
					number_of_ranges =
						Multi_range_get_number_of_ranges(selected_ranges);
					for (i = 0; i < number_of_ranges; i++)
					{
						if (Multi_range_get_range(selected_ranges, i, &start, &stop))
						{
							for (grid_point_number = start ; grid_point_number <= stop ; grid_point_number++)
							{
								if (FE_element_get_numbered_xi_point(
											element, element_point_ranges_identifier.sampling_mode,
											element_point_ranges_identifier.number_in_xi, element_point_ranges_identifier.exact_xi,
											(cmzn_fieldcache_id)0,
											/*coordinate_field*/(struct Computed_field *)NULL,
											/*density_field*/(struct Computed_field *)NULL,
											grid_point_number, xi))
								{
									if ((CMZN_OK == data->field_cache->setMeshLocation(element, xi)) &&
										(CMZN_OK == cmzn_field_evaluate_real(data->source_field,
											data->field_cache, componentCount, values)))
									{
										cmzn_field_assign_real(data->destination_field,
											data->field_cache, componentCount, values);
									}
								}
							}
						}
					}
				}
				data->success_count++;
				DEALLOCATE(values);
			}
		}
		if (element_point_ranges)
		{
			DEACCESS(Element_point_ranges)(&element_point_ranges);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_element_assign_grid_field_from_source_sub.  "
			"Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_element_assign_grid_field_from_source_sub */

int cmzn_mesh_assign_grid_field_from_source(
	cmzn_mesh_id mesh, cmzn_field_id destination_field,
	cmzn_field_id source_field, cmzn_field_id conditional_field,
	struct Element_point_ranges_selection *element_point_ranges_selection,
	FE_value time)
{
	int return_code = 1;
	if (mesh && destination_field && source_field)
	{
		if (cmzn_field_get_number_of_components(source_field) ==
			 cmzn_field_get_number_of_components(destination_field))
		{
			cmzn_region_id region = cmzn_mesh_get_region_internal(mesh);
			cmzn_fieldmodule_id fieldmodule = cmzn_region_get_fieldmodule(region);
			cmzn_fieldmodule_begin_change(fieldmodule);
			cmzn_fieldcache_id field_cache = cmzn_fieldmodule_create_fieldcache(fieldmodule);
			cmzn_fieldcache_set_time(field_cache, time);
			cmzn_element_assign_grid_field_from_source_data data;
			data.field_cache = field_cache;
			data.source_field = source_field;
			data.destination_field = destination_field;
			data.element_point_ranges_selection = element_point_ranges_selection;
			data.group_field = conditional_field;
			data.selected_count = 0;
			data.success_count = 0;
			cmzn_elementiterator_id iter = cmzn_mesh_create_elementiterator(mesh);
			cmzn_element_id element = 0;
			while (0 != (element = cmzn_elementiterator_next_non_access(iter)))
			{
				if (!cmzn_element_assign_grid_field_from_source_sub(element, &data))
				{
					return_code = 0;
					break;
				}
			}
			cmzn_elementiterator_destroy(&iter);
			if (data.success_count != data.selected_count)
			{
				display_message(ERROR_MESSAGE,
					"cmzn_mesh_assign_grid_field_from_source."
					"  Only able to set values for %d elements out of %d\n"
					"  Either source field isn't defined in element "
					"or destination field could not be set.",
					data.success_count, data.selected_count);
				return_code = 0;
			}
			cmzn_fieldcache_destroy(&field_cache);
			cmzn_fieldmodule_end_change(fieldmodule);
			cmzn_fieldmodule_destroy(&fieldmodule);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"cmzn_mesh_assign_grid_field_from_source.  "
				"Number of components in source and destination fields must match.");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_mesh_assign_grid_field_from_source.  "
			"Invalid argument(s)");
		return_code = 0;
	}
	return (return_code);
}
