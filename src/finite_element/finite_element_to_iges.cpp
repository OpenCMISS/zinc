/*******************************************************************************
FILE : finite_element_to_iges.c

LAST MODIFIED : 4 March 2003

DESCRIPTION :
Functions for building IGES information from finite elements and exporting
to file.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/* for IGES */
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>

#include "opencmiss/zinc/core.h"
#include "opencmiss/zinc/fieldcache.h"
#include "opencmiss/zinc/fieldmodule.h"
#include "opencmiss/zinc/mesh.h"
#include "opencmiss/zinc/node.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_wrappers.h"
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_conversion.h"
#include "finite_element/finite_element_field_evaluation.hpp"
#include "finite_element/finite_element_mesh.hpp"
#include "finite_element/finite_element_nodeset.hpp"
#include "finite_element/finite_element_region.h"
#include "finite_element/finite_element_to_iges.h"
#include "mesh/cmiss_element_private.hpp"
#include "general/debug.h"
#include "general/message.h"

/*
Global functions
----------------
*/

struct IGES_entity_info
/******************************************************************************
LAST MODIFIED : 7 June 2002

DESCRIPTION :
==============================================================================*/
{
	int element_dimension;
	int element_identifier;
	int directory_pointer,parameter_pointer,type;
	struct
	{
		char label[9];
		int color,form,label_display_associativity,level,line_font_pattern,
			line_weight,parameter_line_count,subscript_number,structure,
			transformation_matrix,view;
		struct
		{
			int blank_status,entity_use_flag,hierarchy,subordinate_entity_switch;
		} status;
	} directory;
	union
	{
		struct
		{
			/* composite curve entity */
			int *directory_pointers,number_of_entities;
		} type_102;
		struct
		{
			/* line entity */
			FE_value end[3],start[3];
		} type_110;
		struct
		{
			/* parametric spline curve entity */
			int degree_of_continuity,n,number_of_dimensions,spline_type;
			/*???DB.  Assuming 1 cubic curve */
			FE_value tu[2],x[4],y[4],z[4];
		} type_112;
		struct
		{
			/* parametric spline surface entity */
			int m,n,patch_type,spline_boundary_type;
			/*???DB.  Assuming 1 bicubic surface */
			FE_value tu[2],tv[2],x[16],y[16],z[16];
		} type_114;
		struct
		{
			/* curve on parametric surface entity */
			int how_curve_created,material_curve_directory_pointer,
				preferred_representation,surface_directory_pointer,
				world_curve_directory_pointer;
		} type_142;
		struct
		{
			/* trimmed parametric surface entity */
			int *inner_boundary_directory_pointers,number_of_inner_boundary_curves,
				outer_boundary_directory_pointer,outer_boundary_type,
				surface_directory_pointer;
		} type_144;
	} parameter;
	struct IGES_entity_info *next;
}; /* struct IGES_entity_info */

#define NUMBER_OF_NODES (16)

struct Get_iges_entity_info_data
{
	cmzn_region_id source_region;
	cmzn_fieldmodule_id source_fieldmodule;
	cmzn_mesh_id source_mesh;
	cmzn_field_id source_field;
	cmzn_fieldcache_id source_fieldcache;
	cmzn_region_id destination_region;
	cmzn_fieldmodule_id destination_fieldmodule;
	cmzn_mesh_id destination_mesh;
	FE_field *destination_fe_field;
	struct IGES_entity_info *head,*tail;
	cmzn_node_id nodes[NUMBER_OF_NODES];
	int extra_line_number;

public:
	Get_iges_entity_info_data(cmzn_region_id source_regionIn, cmzn_field_id fieldIn) :
		source_region(cmzn_region_access(source_regionIn)),
		source_fieldmodule(cmzn_region_get_fieldmodule(this->source_region)),
		source_mesh(cmzn_fieldmodule_find_mesh_by_dimension(this->source_fieldmodule, 2)),
		source_field(cmzn_field_get_coordinate_field_wrapper(fieldIn)),
		source_fieldcache(cmzn_fieldmodule_create_fieldcache(this->source_fieldmodule)),
		destination_region(cmzn_region_create_region(this->source_region)),
		destination_fieldmodule(cmzn_region_get_fieldmodule(this->destination_region)),
		destination_mesh(cmzn_fieldmodule_find_mesh_by_dimension(this->destination_fieldmodule, 2)),
		destination_fe_field(0),
		head(0),
		tail(0),
		extra_line_number(999000) // Start somewhere random although we will still check that it doesn't conflict
	{
		for (int i = 0; i < NUMBER_OF_NODES; i++)
			this->nodes[i] = 0;

		struct Element_refinement refinement = { 1, 1, 1 };
		const double tolerance = 1.0E-6;
		// All 2-D elements are resampled into bicubic Lagrange for iges export.
		// Future: convert only top-level or exterior elements
		// Formerly, this code ensured the destination elements had the same element and face identifiers as the source
		// It doesn't do this at the moment; may need to reimplement that.
		cmzn_fieldmodule_begin_change(this->destination_fieldmodule);
		if (!finite_element_conversion(this->source_region,
			this->destination_region, CONVERT_TO_FINITE_ELEMENTS_BICUBIC,
			/*number_of_source_fields*/1, &this->source_field,
			refinement, tolerance))
		{
			display_message(ERROR_MESSAGE, "export iges:  Failed to convert surface elements to bicubic");
		}
		else
		{
			cmzn_fieldmodule_define_all_faces(this->destination_fieldmodule);
			char *name = cmzn_field_get_name(this->source_field);
			this->destination_fe_field = FE_region_get_FE_field_from_name(this->destination_region->get_FE_region(), name);
			cmzn_deallocate(name);
			name = 0;
			cmzn_elementiterator *iter = cmzn_mesh_create_elementiterator(this->destination_mesh);
			cmzn_element *element;
			FE_mesh *feMesh = cmzn_mesh_get_FE_mesh_internal(this->destination_mesh);
			while (0 != (element = cmzn_elementiterator_next_non_access(iter)))
			{
				if (1 >= feMesh->getElementParentsCount(element->getIndex()))
					this->get_iges_entity_info(element);
			}
			cmzn_elementiterator_destroy(&iter);
		}
		cmzn_fieldmodule_end_change(this->destination_fieldmodule);
	}

	~Get_iges_entity_info_data()
	{
		for (int i = 0; i < NUMBER_OF_NODES; i++)
			cmzn_node_destroy(&this->nodes[i]);
		cmzn_mesh_destroy(&this->destination_mesh);
		cmzn_fieldmodule_destroy(&this->destination_fieldmodule);
		cmzn_region_destroy(&this->destination_region);
		cmzn_fieldcache_destroy(&this->source_fieldcache);
		cmzn_field_destroy(&this->source_field);
		cmzn_mesh_destroy(&this->source_mesh);
		cmzn_fieldmodule_destroy(&this->source_fieldmodule);
		cmzn_region_destroy(&this->source_region);
	}

	int get_iges_entity_info(struct FE_element *element);
};

static struct IGES_entity_info *create_iges_entity_info(
	struct FE_element *element,struct IGES_entity_info **head,
	struct IGES_entity_info **tail)
/******************************************************************************
LAST MODIFIED : 6 June 2002

DESCRIPTION :
==============================================================================*/
{
	int directory_pointer,parameter_pointer;
	struct IGES_entity_info *entity;

	ENTER(create_iges_entity_info);
	entity=(struct IGES_entity_info *)NULL;
	if (element&&head&&tail)
	{
		if (ALLOCATE(entity,struct IGES_entity_info,1))
		{
			/* put into list */
			if ((*head)&&(*tail))
			{
				directory_pointer=(*tail)->directory_pointer+2;
				parameter_pointer=(*tail)->parameter_pointer+
					((*tail)->directory).parameter_line_count;
				(*tail)->next=entity;
				*tail=entity;
			}
			else
			{
				directory_pointer=1;
				parameter_pointer=1;
				*head=entity;
				*tail=entity;
			}
			entity->next=(struct IGES_entity_info *)NULL;
			/* fill in values */
			entity->element_dimension = get_FE_element_dimension(element);
			entity->element_identifier = get_FE_element_identifier(element);
			entity->type=0;
			entity->directory_pointer=directory_pointer;
			entity->parameter_pointer=parameter_pointer;
			(entity->directory).structure=0;
			(entity->directory).line_font_pattern=0;
			(entity->directory).level=0;
			(entity->directory).view=0;
			(entity->directory).transformation_matrix=0;
			(entity->directory).label_display_associativity=0;
			(entity->directory).status.blank_status=0;
			(entity->directory).status.subordinate_entity_switch=0;
			(entity->directory).status.entity_use_flag=0;
			(entity->directory).status.hierarchy=0;
			(entity->directory).line_weight=0;
			(entity->directory).color=0;
			(entity->directory).parameter_line_count=0;
			(entity->directory).form=0;
			((entity->directory).label)[0]='\0';
			(entity->directory).subscript_number=0;
		}
	}
	LEAVE;

	return (entity);
} /* create_iges_entity_info */

int Get_iges_entity_info_data::get_iges_entity_info(struct FE_element *element)
{
	FE_value *destination, *source, *source_ptr;
	int *faces_material,*faces_world,i,j,k,material_curve_directory_pointer,
		monomial_x[1 + MAXIMUM_ELEMENT_XI_DIMENSIONS],
		monomial_y[1 + MAXIMUM_ELEMENT_XI_DIMENSIONS],
		monomial_z[1 + MAXIMUM_ELEMENT_XI_DIMENSIONS],
		number_of_component_values, number_of_faces,
		outer_boundary_directory_pointer,*reorder_faces,return_code,
		surface_directory_pointer,world_curve_directory_pointer;
	struct FE_element *face;
	struct FE_field *coordinate_field;
	struct IGES_entity_info *entity,*surface_entity;

	ENTER(get_iges_entity_info);
	return_code=0;
	if (element)
	{
		return_code=1;
		FE_element_field_evaluation *coordinate_element_field_values = 0;
		FE_mesh *fe_mesh = element->getMesh();
		const DsLabelIndex elementIndex = get_FE_element_index(element);
		if ((fe_mesh) && (2 == fe_mesh->getDimension()) &&
			(1 >= fe_mesh->getElementParentsCount(elementIndex)) &&
			(coordinate_field = this->destination_fe_field) &&
			(3==get_FE_field_number_of_components(coordinate_field))&&
			(coordinate_element_field_values = FE_element_field_evaluation::create()) &&
			coordinate_element_field_values->calculate_values(coordinate_field, element, /*time*/0.0) &&
			coordinate_element_field_values->get_monomial_component_info(
				/*component_number*/0, monomial_x) &&
			(2 == monomial_x[0]) && (monomial_x[1] <= 3) && (monomial_x[2] <= 3) &&
			coordinate_element_field_values->get_monomial_component_info(
				/*component_number*/1, monomial_y) &&
			(2 == monomial_y[0]) && (monomial_y[1] <= 3) && (monomial_y[2] <= 3) &&
			coordinate_element_field_values->get_monomial_component_info(
				/*component_number*/2, monomial_z) &&
			(2 == monomial_z[0]) && (monomial_z[1] <= 3) && (monomial_z[2] <= 3))
		{
			surface_entity=create_iges_entity_info(element,&(this->head),
				&(this->tail));
			if (surface_entity != NULL)
			{
				surface_directory_pointer=surface_entity->directory_pointer;
				/* blanked */
				(surface_entity->directory).status.blank_status=1;
				/* physically dependent */
				(surface_entity->directory).status.subordinate_entity_switch=1;
				/* parametric spline surface entity */
				surface_entity->type=114;
				/* use one line for integer flags, one line for the breakpoints and
					4 values per line for coefficients */
				(surface_entity->directory).parameter_line_count=14;
				/* cubic */
				(surface_entity->parameter).type_114.spline_boundary_type=3;
				/* cartesian product */
				(surface_entity->parameter).type_114.patch_type=1;
				(surface_entity->parameter).type_114.m=1;
				(surface_entity->parameter).type_114.n=1;
				((surface_entity->parameter).type_114.tu)[0]=0.;
				((surface_entity->parameter).type_114.tu)[1]=1.;
				((surface_entity->parameter).type_114.tv)[0]=0.;
				((surface_entity->parameter).type_114.tv)[1]=1.;
				if (coordinate_element_field_values->get_component_values(
					/*component_number*/0, &number_of_component_values, &source) &&
					(0 < number_of_component_values) && source)
				{
					source_ptr = source;
					destination=(surface_entity->parameter).type_114.x;
					k=0;
					for (j=0;j<=monomial_x[2];j++)
					{
						for (i=0;i<=monomial_x[1];i++)
						{
							*destination= *source_ptr;
							destination++;
							source_ptr++;
							k++;
						}
						while (i<=3)
						{
							*destination=(ZnReal)0;
							destination++;
							k++;
							i++;
						}
					}
					while (k<16)
					{
						*destination=(ZnReal)0;
						destination++;
						k++;
					}
					DEALLOCATE(source);
				}
				if (coordinate_element_field_values->get_component_values(
					/*component_number*/1, &number_of_component_values, &source) &&
					(0 < number_of_component_values) && source)
				{
					source_ptr = source;
					destination=(surface_entity->parameter).type_114.y;
					k=0;
					for (j=0;j<=monomial_y[2];j++)
					{
						for (i=0;i<=monomial_y[1];i++)
						{
							*destination= *source_ptr;
							destination++;
							source_ptr++;
							k++;
						}
						while (i<=3)
						{
							*destination=(ZnReal)0;
							destination++;
							k++;
							i++;
						}
					}
					while (k<16)
					{
						*destination=(ZnReal)0;
						destination++;
						k++;
					}
					DEALLOCATE(source);
				}
				if (coordinate_element_field_values->get_component_values(
					/*component_number*/2, &number_of_component_values, &source) &&
					(0 < number_of_component_values) && source)
				{
					source_ptr = source;
					destination=(surface_entity->parameter).type_114.z;
					k=0;
					for (j=0;j<=monomial_z[2];j++)
					{
						for (i=0;i<=monomial_z[1];i++)
						{
							*destination= *source_ptr;
							destination++;
							source_ptr++;
							k++;
						}
						while (i<=3)
						{
							*destination=(ZnReal)0;
							destination++;
							k++;
							i++;
						}
					}
					while (k<16)
					{
						*destination=(ZnReal)0;
						destination++;
						k++;
					}
					DEALLOCATE(source);
				}
				/* add information for edges so that can be joined together */
				FE_element_shape *element_shape = get_FE_element_shape(element);
				number_of_faces = FE_element_shape_get_number_of_faces(element_shape);
				if (4 == number_of_faces)
				{
					ALLOCATE(faces_material,int,number_of_faces);
					ALLOCATE(faces_world,int,number_of_faces);
					ALLOCATE(reorder_faces,int,number_of_faces);
					if (faces_material&&faces_world)
					{
						/* reorder the faces to go around the surface (not cmiss way) */
						reorder_faces[0]=0;
						reorder_faces[1]=2;
						reorder_faces[2]=3;
						reorder_faces[3]=1;
						i=0;
						while (return_code&&(i<number_of_faces))
						{
							face = get_FE_element_face(element, i);
							if (face)
							{
								int face_dimension = get_FE_element_dimension(face);
								int face_identifier = get_FE_element_identifier(face);
								/* create an entity for the edge in material coordinates */
								entity = create_iges_entity_info(face,&(this->head),
									&(this->tail));
								if (entity != NULL)
								{
									faces_material[reorder_faces[i]]=entity->directory_pointer;
									/* blanked */
									(entity->directory).status.blank_status=1;
									/* physically dependent */
									(entity->directory).status.subordinate_entity_switch=1;
									/* 2d parametric.  In xi coordinates */
									(entity->directory).status.entity_use_flag=5;
									/* line entity */
									entity->type=110;
									/* use one line for type, one line for start and one line
										for end */
									(entity->directory).parameter_line_count=3;
									/* cmiss order is xi1=0, xi1=1, xi2=0, xi2=1 */
									switch (i)
									{
										case 0:
										{
											/* xi1=0 */
											((entity->parameter).type_110.start)[0]=0.;
											((entity->parameter).type_110.start)[1]=0.;
											((entity->parameter).type_110.end)[0]=0.;
											((entity->parameter).type_110.end)[1]=1.;
										} break;
										case 1:
										{
											/* xi1=1 */
											((entity->parameter).type_110.start)[0]=1.;
											((entity->parameter).type_110.start)[1]=0.;
											((entity->parameter).type_110.end)[0]=1.;
											((entity->parameter).type_110.end)[1]=1.;
										} break;
										case 2:
										{
											/* xi2=0 */
											((entity->parameter).type_110.start)[0]=0.;
											((entity->parameter).type_110.start)[1]=0.;
											((entity->parameter).type_110.end)[0]=1.;
											((entity->parameter).type_110.end)[1]=0.;
										} break;
										case 3:
										{
											/* xi2=1 */
											((entity->parameter).type_110.start)[0]=0.;
											((entity->parameter).type_110.start)[1]=1.;
											((entity->parameter).type_110.end)[0]=1.;
											((entity->parameter).type_110.end)[1]=1.;
										} break;
									}
									((entity->parameter).type_110.start)[2]=0.;
									((entity->parameter).type_110.end)[2]=0.;
									entity=this->head;
									while (entity && !((face_dimension == entity->element_dimension) &&
										(face_identifier == entity->element_identifier) &&
										(112 == entity->type)))
									{
										entity=entity->next;
									}
									if (entity)
									{
										faces_world[reorder_faces[i]]=entity->directory_pointer;
									}
									else
									{
										/* create an entity for the edge in material coordinates */
										entity=create_iges_entity_info(face,&(this->head),
											&(this->tail));
										if (entity != NULL)
										{
											faces_world[reorder_faces[i]]=entity->directory_pointer;
											/* blanked */
											(entity->directory).status.blank_status=1;
											/* physically dependent */
											(entity->directory).status.subordinate_entity_switch=1;
											/* parametric spline curve entity */
											entity->type=112;
											/* use one line for integer flags, one line for the
												breakpoints and 4 values per line for coefficients */
											(entity->directory).parameter_line_count=5;
											/* cubic */
											(entity->parameter).type_112.spline_type=3;
											/* value and slope continuous at breakpoints */
											(entity->parameter).type_112.degree_of_continuity=1;
											(entity->parameter).type_112.number_of_dimensions=3;
											(entity->parameter).type_112.n=1;
											((entity->parameter).type_112.tu)[0]=0.;
											((entity->parameter).type_112.tu)[1]=1.;
											switch (i)
											{
												case 0:
												{
													/* xi1=0 */
													for (j=0;j<4;j++)
													{
														((entity->parameter).type_112.x)[j]=
															((surface_entity->parameter).type_114.x)[4*j];
														((entity->parameter).type_112.y)[j]=
															((surface_entity->parameter).type_114.y)[4*j];
														((entity->parameter).type_112.z)[j]=
															((surface_entity->parameter).type_114.z)[4*j];
													}
												} break;
												case 1:
												{
													/* xi1=1 */
													for (j=0;j<4;j++)
													{
														((entity->parameter).type_112.x)[j]=
															((surface_entity->parameter).type_114.x)[4*j];
														((entity->parameter).type_112.y)[j]=
															((surface_entity->parameter).type_114.y)[4*j];
														((entity->parameter).type_112.z)[j]=
															((surface_entity->parameter).type_114.z)[4*j];
														for (k=1;k<4;k++)
														{
															((entity->parameter).type_112.x)[j] +=
																((surface_entity->parameter).type_114.x)[4*j+k];
															((entity->parameter).type_112.y)[j] +=
																((surface_entity->parameter).type_114.y)[4*j+k];
															((entity->parameter).type_112.z)[j] +=
																((surface_entity->parameter).type_114.z)[4*j+k];
														}
													}
												} break;
												case 2:
												{
													/* xi2=0 */
													for (j=0;j<4;j++)
													{
														((entity->parameter).type_112.x)[j]=
															((surface_entity->parameter).type_114.x)[j];
														((entity->parameter).type_112.y)[j]=
															((surface_entity->parameter).type_114.y)[j];
														((entity->parameter).type_112.z)[j]=
															((surface_entity->parameter).type_114.z)[j];
													}
												} break;
												case 3:
												{
													/* xi2=1 */
													for (j=0;j<4;j++)
													{
														((entity->parameter).type_112.x)[j]=
															((surface_entity->parameter).type_114.x)[j];
														((entity->parameter).type_112.y)[j]=
															((surface_entity->parameter).type_114.y)[j];
														((entity->parameter).type_112.z)[j]=
															((surface_entity->parameter).type_114.z)[j];
														for (k=4;k<16;k += 4)
														{
															((entity->parameter).type_112.x)[j] +=
																((surface_entity->parameter).type_114.x)[j+k];
															((entity->parameter).type_112.y)[j] +=
																((surface_entity->parameter).type_114.y)[j+k];
															((entity->parameter).type_112.z)[j] +=
																((surface_entity->parameter).type_114.z)[j+k];
														}
													}
												} break;
											}
										}
										else
										{
											return_code=0;
										}
									}
								}
								else
								{
									return_code=0;
								}
							}
							i++;
						}
						if (return_code)
						{
							/* create a composite curve entity to describe the boundary of
								the element in material coordinates */
							if ((entity=create_iges_entity_info(element,&(this->head),
								&(this->tail)))&&ALLOCATE((entity->parameter).type_102.
								directory_pointers,int,number_of_faces))
							{
								material_curve_directory_pointer=entity->directory_pointer;
								/* blanked */
								(entity->directory).status.blank_status=1;
								/* physically dependent */
								(entity->directory).status.subordinate_entity_switch=1;
								/* 2d parametric.  In xi coordinates */
								(entity->directory).status.entity_use_flag=5;
								/* composite curve entity */
								entity->type=102;
								(entity->directory).parameter_line_count=1;
								(entity->parameter).type_102.number_of_entities=number_of_faces;
								for (j=0;j<number_of_faces;j++)
								{
									((entity->parameter).type_102.directory_pointers)[j]=
										faces_material[j];
								}
								/* create a composite curve entity to describe the boundary of
									the element in world coordinates */
								if ((entity=create_iges_entity_info(element,&(this->head),
									&(this->tail)))&&ALLOCATE((entity->parameter).type_102.
									directory_pointers,int,number_of_faces))
								{
									world_curve_directory_pointer=entity->directory_pointer;
									/* blanked */
									(entity->directory).status.blank_status=1;
									/* physically dependent */
									(entity->directory).status.subordinate_entity_switch=1;
									/* composite curve entity */
									entity->type=102;
									(entity->directory).parameter_line_count=1;
									(entity->parameter).type_102.number_of_entities=
										number_of_faces;
									for (j=0;j<number_of_faces;j++)
									{
										((entity->parameter).type_102.directory_pointers)[j]=
											faces_world[j];
									}
									/* combine the world and material boundary descriptions */
									entity=create_iges_entity_info(element,&(this->head),
										&(this->tail));
									if (entity != NULL)
									{
										outer_boundary_directory_pointer=entity->directory_pointer;
										/* blanked */
										(entity->directory).status.blank_status=1;
										/* physically dependent */
										(entity->directory).status.subordinate_entity_switch=1;
										/* 2d parametric.  In xi coordinates */
										(entity->directory).status.entity_use_flag=5;
										/* curve on parametric surface entity */
										entity->type=142;
										(entity->directory).parameter_line_count=1;
										/* projection of a given curve on the surface */
										(entity->parameter).type_142.how_curve_created=1;
										(entity->parameter).type_142.surface_directory_pointer=
											surface_directory_pointer;
										(entity->parameter).type_142.
											material_curve_directory_pointer=
											material_curve_directory_pointer;
										(entity->parameter).type_142.world_curve_directory_pointer=
											world_curve_directory_pointer;
										/* material specification is preferred */
										(entity->parameter).type_142.preferred_representation=1;
										/* create the trimmed surface that can be stitched
											together */
										entity=create_iges_entity_info(element,
											&(this->head),&(this->tail));
										if (entity != NULL)
										{
											/* trimmed parametric surface entity */
											entity->type=144;
											(entity->directory).parameter_line_count=1;
											(entity->parameter).type_144.surface_directory_pointer=
												surface_directory_pointer;
											/* outer boundary is the one specified (not the boundary
												of <surface_directory_pointer> */
											(entity->parameter).type_144.outer_boundary_type=1;
											(entity->parameter).type_144.
												outer_boundary_directory_pointer=
												outer_boundary_directory_pointer;
											(entity->parameter).type_144.
												number_of_inner_boundary_curves=0;
											(entity->parameter).type_144.
												inner_boundary_directory_pointers=(int *)NULL;
										}
										else
										{
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
									return_code=0;
								}
							}
							else
							{
								return_code=0;
							}
						}
					}
					else
					{
						return_code=0;
					}
					DEALLOCATE(reorder_faces);
					DEALLOCATE(faces_world);
					DEALLOCATE(faces_material);
				}
			}
			else
			{
				return_code=0;
			}
		}
		if (coordinate_element_field_values)
			FE_element_field_evaluation::deaccess(coordinate_element_field_values);
	}
	LEAVE;

	return (return_code);
} /* get_iges_entity_info */

/*
Global functions
----------------
*/

struct Write_iges_parameter_data_data
{
	FILE *iges;
	int count;
}; /* struct Write_iges_parameter_data_data */

int export_to_iges(char *file_name, struct cmzn_region *region,
	char *region_path, struct cmzn_field *field)
{
	char *out_string, numeric_string[20], *temp_string, time_string[14];
	FILE *iges;
	int count, i, global_count, length, out_length, parameter_pointer,
		return_code;
	struct IGES_entity_info *entity;
	time_t coded_time;
	struct tm *time_struct;
	cmzn_fieldmodule_id field_module;
	cmzn_fieldcache_id field_cache;

	ENTER(export_to_iges);
	return_code=0;
	USE_PARAMETER(field);
	if (file_name && region && region_path)
	{
		iges = fopen(file_name,"w");
		if (iges != NULL)
		{
			return_code=1;
			/* write IGES header */
			/* start section */
			fprintf(iges,"%-72sS      1\n","cmgui");
			/* global section */
			global_count=0;
			out_string=(char *)NULL;
			out_length=0;
#define WRITE_STRING_PARAMETER( parameter ) \
{ \
	length = static_cast<int>(strlen(parameter)); \
	if (length>0) \
	{ \
		length += (int)ceil(log10((double)length))+2; \
		if (REALLOCATE(temp_string,out_string,char,out_length+length+2)) \
		{ \
			out_string=temp_string; \
			sprintf(out_string+out_length,"%dH",static_cast<int>(strlen(parameter))); \
			strcat(out_string,parameter); \
			strcat(out_string,","); \
			out_length = static_cast<int>(strlen(out_string)); \
			while (out_length>72) \
			{ \
				global_count++; \
				fprintf(iges,"%.72sG%7d\n",out_string,global_count); \
				out_length -= 72; \
				temp_string=out_string; \
				for (i=0;i<=out_length;i++) \
				{ \
					*temp_string=temp_string[72]; \
					temp_string++; \
				} \
			} \
		} \
	} \
	else \
	{ \
		if (REALLOCATE(temp_string,out_string,char,out_length+2)) \
		{ \
			out_string=temp_string; \
			strcat(out_string,","); \
			out_length=static_cast<int>(strlen(out_string)); \
		} \
	} \
}
#define WRITE_INTEGER_PARAMETER( parameter ) \
{ \
	sprintf(numeric_string,"%d",parameter); \
	length = static_cast<int>(strlen(numeric_string) + 1); \
	if (REALLOCATE(temp_string,out_string,char,out_length+length+1)) \
	{ \
		out_string=temp_string; \
		if (out_length+length>72) \
		{ \
			global_count++; \
			fprintf(iges,"%-72sG%7d\n",out_string,global_count); \
			out_string[0]='\0'; \
			out_length=0; \
		} \
		strcat(out_string,numeric_string); \
		strcat(out_string,","); \
		out_length = static_cast<int>(strlen(out_string)); \
	} \
}
#define WRITE_REAL_PARAMETER( parameter ) \
{ \
	sprintf(numeric_string,"%.6e",parameter); \
	length=static_cast<int>(strlen(numeric_string) + 1); \
	if (REALLOCATE(temp_string,out_string,char,out_length+length+1)) \
	{ \
		out_string=temp_string; \
		if (out_length+length>72) \
		{ \
			global_count++; \
			fprintf(iges,"%-72sG%7d\n",out_string,global_count); \
			out_string[0]='\0'; \
			out_length=0; \
		} \
		strcat(out_string,numeric_string); \
		strcat(out_string,","); \
		out_length = static_cast<int>(strlen(out_string)); \
	} \
}
			/* parameter delimiter */
			WRITE_STRING_PARAMETER(",");
			/* record delimiter */
			WRITE_STRING_PARAMETER(";");
			/* product identification from sending system */
			WRITE_STRING_PARAMETER(region_path);
			/* file name */
			WRITE_STRING_PARAMETER(file_name);
			/* system ID */
			WRITE_STRING_PARAMETER("cmgui");
			/* version */
			WRITE_STRING_PARAMETER("unknown");
			/* number of binary bits for integer representation */
			WRITE_INTEGER_PARAMETER((int)(8*sizeof(int)));
			/* maximum power of ten representable in a single precision floating point
				number on the sending system */
			WRITE_INTEGER_PARAMETER(FLT_MAX_10_EXP);
			/* number of significant digits in a single precision floating point
				number on the sending system */
			WRITE_INTEGER_PARAMETER(FLT_DIG);
			/* maximum power of ten representable in a double precision floating point
				number on the sending system */
			WRITE_INTEGER_PARAMETER(DBL_MAX_10_EXP);
			/* number of significant digits in a double precision floating point
				number on the sending system */
			WRITE_INTEGER_PARAMETER(DBL_DIG);
			/* product identification for the receiving system */
			WRITE_STRING_PARAMETER(region_path);
			/* model space scale (example:  .125 indicates a ratio of 1 unit model
				space to 8 units real world) */
			WRITE_REAL_PARAMETER(1.);
			/* unit flag.  millimetres */
			WRITE_INTEGER_PARAMETER(2);
			/* units abreviation */
			WRITE_STRING_PARAMETER("MM");
			/* maximum number of line weight gradations */
			WRITE_INTEGER_PARAMETER(1);
			/* width of maximum line weight in units */
			WRITE_REAL_PARAMETER(0.125);
			/* date & time of exchange file generation  YYMMDD.HHNNSS */
			time(&coded_time);
			time_struct=localtime(&coded_time);
			sprintf(time_string,"%02d%02d%02d.%02d%02d%02d",
				(time_struct->tm_year)%100,time_struct->tm_mday,(time_struct->tm_mon)+1,
				time_struct->tm_hour,time_struct->tm_min,time_struct->tm_sec);
			WRITE_STRING_PARAMETER(time_string);
			/* minimum user-intended resolution or granularity of the model expressed
				in units */
			WRITE_REAL_PARAMETER(1.e-8);
			/* approximate maximum coordinate value occurring in the model expressed
				in units */
			WRITE_REAL_PARAMETER(1.e4);
			/* name of author */
			WRITE_STRING_PARAMETER("");
			/* author's organization */
			WRITE_STRING_PARAMETER("");
			/* integer value corresponding to the version of the Specification used to
				create this file */
			WRITE_INTEGER_PARAMETER(5);
			/* drafting standard in compliance to which the data encoded in this file
				was generated */
			WRITE_INTEGER_PARAMETER(0);
			/* date and time the model was created or last modified, whichever
				occurred last, YYMMDD.HHNNSS */
			WRITE_STRING_PARAMETER("");
			global_count++;
			out_string[strlen(out_string)-1]=';';
			fprintf(iges,"%-72sG%7d\n",out_string,global_count);
			/* get entity information */
			field_module = cmzn_field_get_fieldmodule(field);
			field_cache = cmzn_fieldmodule_create_fieldcache(field_module);
			// convert surface elements to bicubic in temporary region
			Get_iges_entity_info_data iges_entity_info_data(region, field);
			/* We no longer require the template element and nodes */
			/* directory entry section */
			entity=iges_entity_info_data.head;
			while (entity)
			{
				fprintf(iges,"%8d%8d%8d%8d%8d%8d%8d%8d%02d%02d%02d%02dD%7d\n",
					entity->type,entity->parameter_pointer,
					(entity->directory).structure,(entity->directory).line_font_pattern,
					(entity->directory).level,(entity->directory).view,
					(entity->directory).transformation_matrix,
					(entity->directory).label_display_associativity,
					(entity->directory).status.blank_status,
					(entity->directory).status.subordinate_entity_switch,
					(entity->directory).status.entity_use_flag,
					(entity->directory).status.hierarchy,
					entity->directory_pointer);
				fprintf(iges,"%8d%8d%8d%8d%8d%8s%8s%8s%8dD%7d\n",
					entity->type,(entity->directory).line_weight,
					(entity->directory).color,(entity->directory).parameter_line_count,
					(entity->directory).form," "," ",(entity->directory).label,
					(entity->directory).subscript_number,(entity->directory_pointer)+1);
				entity=entity->next;
			}
			/* parameter data section */
			// write to string then output and pad with spaces.
			// earlier code used fprintf %n specifier which is disabled by default on windows
			char tmps[200];
			entity=iges_entity_info_data.head;
			while (entity)
			{
				switch (entity->type)
				{
					case 102:
					{
						/* composite curve entity */
						parameter_pointer=entity->parameter_pointer;
						sprintf(tmps,"%d,%d",entity->type,
							(entity->parameter).type_102.number_of_entities);
						for (i=0;i<(entity->parameter).type_102.number_of_entities;i++)
						{
							char tmps2[50];
							sprintf(tmps2, ",%d",
								((entity->parameter).type_102.directory_pointers)[i]);
							strcat(tmps, tmps2);
						}
						strcat(tmps, ";");
						count = 64 - static_cast<int>(strlen(tmps));
						fprintf(iges,"%s%*s%8dP%7d\n",tmps,count," ",
							entity->directory_pointer,parameter_pointer);
						parameter_pointer++;
					} break;
					case 110:
					{
						/* line entity */
						parameter_pointer=entity->parameter_pointer;
						sprintf(tmps,"%d,",entity->type);
						count = 64 - static_cast<int>(strlen(tmps));
						fprintf(iges,"%s%*s%8dP%7d\n",tmps,count," ",
							entity->directory_pointer,parameter_pointer);
						parameter_pointer++;
						sprintf(tmps,"%.6e,%.6e,%.6e,",
							((entity->parameter).type_110.start)[0],
							((entity->parameter).type_110.start)[1],
							((entity->parameter).type_110.start)[2]);
						count = 64 - static_cast<int>(strlen(tmps));
						fprintf(iges,"%s%*s%8dP%7d\n",tmps,count," ",
							entity->directory_pointer,parameter_pointer);
						sprintf(tmps,"%.6e,%.6e,%.6e;",
							((entity->parameter).type_110.end)[0],
							((entity->parameter).type_110.end)[1],
							((entity->parameter).type_110.end)[2]);
						count = 64 - static_cast<int>(strlen(tmps));
						fprintf(iges,"%s%*s%8dP%7d\n",tmps,count," ",
							entity->directory_pointer,parameter_pointer);
					} break;
					case 112:
					{
						/* parametric spline curve entity */
						parameter_pointer=entity->parameter_pointer;
						sprintf(tmps,"%d,%d,%d,%d,%d,",
							entity->type,(entity->parameter).type_112.spline_type,
							(entity->parameter).type_112.degree_of_continuity,
							(entity->parameter).type_112.number_of_dimensions,
							(entity->parameter).type_112.n);
						count = 64 - static_cast<int>(strlen(tmps));
						fprintf(iges,"%s%*s%8dP%7d\n",tmps,count," ",
							entity->directory_pointer,parameter_pointer);
						parameter_pointer++;
						sprintf(tmps,"%.6e,%.6e,",
							((entity->parameter).type_112.tu)[0],
							((entity->parameter).type_112.tu)[1]);
						count = 64 - static_cast<int>(strlen(tmps));
						fprintf(iges,"%s%*s%8dP%7d\n",tmps,count," ",
							entity->directory_pointer,parameter_pointer);
						parameter_pointer++;
						sprintf(tmps,"%.6e,%.6e,%.6e,%.6e,",
							((entity->parameter).type_112.x)[0],
							((entity->parameter).type_112.x)[1],
							((entity->parameter).type_112.x)[2],
							((entity->parameter).type_112.x)[3]);
						count = 64 - static_cast<int>(strlen(tmps));
						fprintf(iges,"%s%*s%8dP%7d\n",tmps,count," ",
							entity->directory_pointer,parameter_pointer);
						parameter_pointer++;
						sprintf(tmps,"%.6e,%.6e,%.6e,%.6e,",
							((entity->parameter).type_112.y)[0],
							((entity->parameter).type_112.y)[1],
							((entity->parameter).type_112.y)[2],
							((entity->parameter).type_112.y)[3]);
						count = 64 - static_cast<int>(strlen(tmps));
						fprintf(iges,"%s%*s%8dP%7d\n",tmps,count," ",
							entity->directory_pointer,parameter_pointer);
						parameter_pointer++;
						sprintf(tmps,"%.6e,%.6e,%.6e,%.6e;",
							((entity->parameter).type_112.z)[0],
							((entity->parameter).type_112.z)[1],
							((entity->parameter).type_112.z)[2],
							((entity->parameter).type_112.z)[3]);
						count = 64 - static_cast<int>(strlen(tmps));
						fprintf(iges,"%s%*s%8dP%7d\n",tmps,count," ",
							entity->directory_pointer,parameter_pointer);
						parameter_pointer++;
					} break;
					case 114:
					{
						/* parametric spline surface entity */
						parameter_pointer=entity->parameter_pointer;
						sprintf(tmps, "%d,%d,%d,%d,%d,",
							entity->type, (entity->parameter).type_114.spline_boundary_type,
							(entity->parameter).type_114.patch_type,
							(entity->parameter).type_114.m, (entity->parameter).type_114.n);
						count = 64 - static_cast<int>(strlen(tmps));
						fprintf(iges,"%s%*s%8dP%7d\n",tmps,count," ",
							entity->directory_pointer,parameter_pointer);
						parameter_pointer++;
						sprintf(tmps,"%.6e,%.6e,%.6e,%.6e,",
							((entity->parameter).type_114.tu)[0],
							((entity->parameter).type_114.tu)[1],
							((entity->parameter).type_114.tv)[0],
							((entity->parameter).type_114.tv)[1]);
						count = 64 - static_cast<int>(strlen(tmps));
						fprintf(iges,"%s%*s%8dP%7d\n",tmps,count," ",
							entity->directory_pointer,parameter_pointer);
						parameter_pointer++;
						for (i=0;i<16;i += 4)
						{
							sprintf(tmps,"%.6e,%.6e,%.6e,%.6e,",
								((entity->parameter).type_114.x)[i],
								((entity->parameter).type_114.x)[i+1],
								((entity->parameter).type_114.x)[i+2],
								((entity->parameter).type_114.x)[i+3]);
							count = 64 - static_cast<int>(strlen(tmps));
							fprintf(iges,"%s%*s%8dP%7d\n",tmps,count," ",
								entity->directory_pointer,parameter_pointer);
							parameter_pointer++;
						}
						for (i=0;i<16;i += 4)
						{
							sprintf(tmps,"%.6e,%.6e,%.6e,%.6e,",
								((entity->parameter).type_114.y)[i],
								((entity->parameter).type_114.y)[i+1],
								((entity->parameter).type_114.y)[i+2],
								((entity->parameter).type_114.y)[i+3]);
							count = 64 - static_cast<int>(strlen(tmps));
							fprintf(iges,"%s%*s%8dP%7d\n",tmps,count," ",
								entity->directory_pointer,parameter_pointer);
							parameter_pointer++;
						}
						for (i=0;i<16;i += 4)
						{
							sprintf(tmps, "%.6e,%.6e,%.6e,%.6e%c",
								((entity->parameter).type_114.z)[i],
								((entity->parameter).type_114.z)[i+1],
								((entity->parameter).type_114.z)[i+2],
								((entity->parameter).type_114.z)[i+3],
								(12==i) ? ';' : ',');
							count = 64 - static_cast<int>(strlen(tmps));
							fprintf(iges,"%s%*s%8dP%7d\n",tmps,count," ",
								entity->directory_pointer,parameter_pointer);
							parameter_pointer++;
						}
					} break;
					case 142:
					{
						/* curve on parametric surface entity */
						parameter_pointer=entity->parameter_pointer;
						sprintf(tmps,"%d,%d,%d,%d,%d,%d;",entity->type,
							(entity->parameter).type_142.how_curve_created,
							(entity->parameter).type_142.surface_directory_pointer,
							(entity->parameter).type_142.material_curve_directory_pointer,
							(entity->parameter).type_142.world_curve_directory_pointer,
							(entity->parameter).type_142.preferred_representation);
						count = 64 - static_cast<int>(strlen(tmps));
						fprintf(iges,"%s%*s%8dP%7d\n",tmps,count," ",
							entity->directory_pointer,parameter_pointer);
						parameter_pointer++;
					} break;
					case 144:
					{
						/* trimmed parametric surface entity */
						parameter_pointer=entity->parameter_pointer;
						sprintf(tmps,"%d,%d,%d,%d,%d",entity->type,
							(entity->parameter).type_144.surface_directory_pointer,
							(entity->parameter).type_144.outer_boundary_type,
							(entity->parameter).type_144.number_of_inner_boundary_curves,
							(entity->parameter).type_144.outer_boundary_directory_pointer);
						for (i=0;
							i<(entity->parameter).type_144.number_of_inner_boundary_curves;
							i++)
						{
							char tmps2[50];
							sprintf(tmps2,",%d",((entity->parameter).type_144.
								inner_boundary_directory_pointers)[i]);
							strcat(tmps, tmps2);
						}
						strcat(tmps, ";");
						count = 64 - static_cast<int>(strlen(tmps));
						fprintf(iges,"%s%*s%8dP%7d\n",tmps,count," ",
							entity->directory_pointer,parameter_pointer);
						parameter_pointer++;
					} break;
				}
				entity=entity->next;
			}
			/* terminate section */
			entity=iges_entity_info_data.tail;
			if (entity)
			{
				fprintf(iges,"S%7dG%7dD%7dP%7d%40sT%7d\n",1,global_count,
					(entity->directory_pointer)+1,(entity->parameter_pointer)+
					((entity->directory).parameter_line_count)-1," ",1);
			}
			while (iges_entity_info_data.head)
			{
				entity=iges_entity_info_data.head;
				iges_entity_info_data.head=entity->next;
				DEALLOCATE(entity);
			}
			fclose(iges);
			cmzn_region_destroy(&iges_entity_info_data.destination_region);
			cmzn_fieldcache_destroy(&field_cache);
			cmzn_fieldmodule_destroy(&field_module);
		}
		else
		{
			display_message(ERROR_MESSAGE,"gfx_export_iges.  Could not open %s",
				file_name);
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"gfx_export_iges.  "
			"Invalid argument(s).  %p %p %p",file_name,region,region_path);
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* export_to_iges */
