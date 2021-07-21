/*******************************************************************************
FILE : volume_texture.c

LAST MODIFIED : 11 November 2005

DESCRIPTION :
Contains data structures for the 3d volumetric textures to be mapped onto
finite elements.  A volume texture is a group of texture 'elements' defined as
lists of texture 'nodes'.  Material values may be associated either with
elements, or with nodes.  The texture nodes are arranged as a 3d lattice in xi1,
xi2, xi3 space.
???DB.  OPENGL ?
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "general/debug.h"
#include "general/indexed_list_private.h"
#include "general/io_stream.h"
#include "general/manager_private.h"
#include "general/matrix_vector.h"
#include "general/mystring.h"
#include "general/object.h"
#include "graphics/graphics_library.h"
#include "graphics/material.h"
#include "graphics/mcubes.h"
#include "graphics/texture_line.h"
#include "graphics/volume_texture.h"
#include "general/message.h"

#define MAX_OBJ_VERTICES 100
#define OBJ_V_EPSILON 0.0000001

/*
Module types
------------
*/
FULL_DECLARE_INDEXED_LIST_TYPE(VT_volume_texture);

FULL_DECLARE_MANAGER_TYPE(VT_volume_texture);

/*
Module functions
----------------
*/
DECLARE_INDEXED_LIST_MODULE_FUNCTIONS(VT_volume_texture,name,const char *,strcmp)

DECLARE_LOCAL_MANAGER_FUNCTIONS(VT_volume_texture)

/*???Mark.  temp clip function for testing */
#if defined (OLDCLIP)
double plane(double *parameters)
{
	double a = 1.0;
	double b = 1.0;
	double c = 1.0;
	double d = -1.0;
	double result;

	result=a*parameters[0] + b*parameters[1] + c*parameters[2] + d;

	return(result);
} /* plane */

/* Use these values to clip the heart in half */
double a = 0.0;
double b = 0.0;
double c = 1.0;
double d = 0.75;/*.25*/
#endif

double plane(double *position,double *parameters)
{
	double result;

	result=position[0]*parameters[0] + position[1]*parameters[1] +
		position[2]*parameters[2] + parameters[3];

	return (result);
}

/*
Global functions
----------------
*/
struct VT_volume_texture *CREATE(VT_volume_texture)(const char *name)
/*******************************************************************************
LAST MODIFIED : 15 October 1998

DESCRIPTION :
Allocates memory and assigns fields for a volume texture.  Adds the texture to
the list of all volume textures.
???DB.  Trimming name ?
???DB.  Check if it already exists ?  Retrieve if it does already exist ?
???DB.  Added option for reading in nodal scalars rather than calculating from
	cell values
==============================================================================*/
{
	struct VT_volume_texture *texture;

	ENTER(CREATE(VT_volume_texture));
	/* allocate memory for structure */
	if (ALLOCATE(texture,struct VT_volume_texture,1)&&
		ALLOCATE(texture->scalar_field,struct VT_scalar_field,1)&&
		ALLOCATE(texture->clip_field,struct VT_scalar_field,1)&&
		ALLOCATE(texture->clip_field2,struct VT_scalar_field,1)&&
		ALLOCATE(texture->coordinate_field,struct VT_vector_field,1)&&
		ALLOCATE(texture->texture_curve_list,struct VT_texture_curve *,1))
	{
		if (name)
		{
			if (ALLOCATE(texture->name,char,strlen(name)+1))
			{
				strcpy(texture->name,name);
			}
		}
		else
		{
			if (ALLOCATE(texture->name,char,1))
			{
				*(texture->name)='\0';
			}
		}
		if (texture->name)
		{
			texture->access_count=0;
			texture->manager = (struct MANAGER(VT_volume_texture) *)NULL;
			texture->manager_change_status = MANAGER_CHANGE_NONE(VT_volume_texture);
			(texture->dimension)[0]= -1;
			(texture->dimension)[1]= -1;
			(texture->dimension)[2]= -1;
			(texture->ximin)[0]=0;
			(texture->ximin)[1]=0;
			(texture->ximin)[2]=0;
			(texture->ximax)[0]=1;
			(texture->ximax)[1]=1;
			(texture->ximax)[2]=1;
			texture->scale_xi[0]=1;
			texture->scale_xi[1]=1;
			texture->scale_xi[2]=1;
			texture->offset_xi[0]=0;
			texture->offset_xi[1]=0;
			texture->offset_xi[2]=0;
			texture->n_groups=0;
			texture->node_groups=(struct VT_node_group **)NULL;
			texture->grid_spacing=(double *)NULL;
			texture->file_name=(char *)NULL;
			*(texture->texture_curve_list)=(struct VT_texture_curve *)NULL;
			texture->texture_cell_list=(struct VT_texture_cell **)NULL;
			texture->global_texture_node_list=(struct VT_texture_node **)NULL;
			(texture->scalar_field->dimension)[0]= -1;
			(texture->scalar_field->dimension)[1]= -1;
			(texture->scalar_field->dimension)[2]= -1;
			texture->scalar_field->scalar=(double *)NULL;
			(texture->clip_field->dimension)[0]= -1;
			(texture->clip_field->dimension)[1]= -1;
			(texture->clip_field->dimension)[2]= -1;
			texture->clip_field->scalar=(double *)NULL;
			(texture->clip_field2->dimension)[0]= -1;
			(texture->clip_field2->dimension)[1]= -1;
			(texture->clip_field2->dimension)[2]= -1;
			texture->clip_field2->scalar=(double *)NULL;
			(texture->coordinate_field->dimension)[0]= -1;
			(texture->coordinate_field->dimension)[1]= -1;
			(texture->coordinate_field->dimension)[2]= -1;
			texture->coordinate_field->vector=(double *)NULL;
			texture->mc_iso_surface=(struct MC_iso_surface *)NULL;
			/*???DB.  Added option for reading in nodal scalars rather than
				calculating from cell values */
			texture->calculate_nodal_values=1;
			texture->recalculate=1;
			texture->isovalue=0;
			texture->hollow_mode_on=0;
			texture->hollow_isovalue=0;
			texture->clipping_field_on=0;
			texture->cutting_plane_on=0;
			texture->cut_isovalue=0;
			texture->closed_surface=0;
			texture->decimation_threshold=0.0;
			texture->disable_volume_functions=0;
			/*???DB.  Should do more initialization of values */
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"CREATE(VT_volume_texture).  Insufficient memory for name");
			DEALLOCATE(texture->coordinate_field);
			DEALLOCATE(texture->clip_field2);
			DEALLOCATE(texture->clip_field);
			DEALLOCATE(texture->scalar_field);
			DEALLOCATE(texture);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CREATE(VT_volume_texture).  Insufficient memory for structure");
		if (texture)
		{
			if (texture->scalar_field)
			{
				if (texture->clip_field)
				{
					if (texture->clip_field2)
					{
						DEALLOCATE(texture->coordinate_field);
						DEALLOCATE(texture->clip_field2);
					}
					DEALLOCATE(texture->clip_field);
				}
				DEALLOCATE(texture->scalar_field);
			}
			DEALLOCATE(texture);
		}
	}
	LEAVE;

	return (texture);
} /* CREATE(VT_volume_texture) */

int DESTROY(VT_volume_texture)(struct VT_volume_texture **texture_address)
/*******************************************************************************
LAST MODIFIED : 18 May 1998

DESCRIPTION :
Frees the memory for the volume texture and sets <*texture_address> to NULL.
==============================================================================*/
{
	int i,number_of_scalar_fields,return_code;
	struct VT_node_group *node_group;
	struct VT_texture_curve *curve,*curve_next;
	struct VT_volume_texture *texture;

	ENTER(DESTROY(VT_volume_texture));
	if (texture_address)
	{
		texture= *texture_address;
		if (texture)
		{
			if (texture->access_count<=0)
			{
				DEALLOCATE(texture->name);
				if (texture->texture_curve_list)
				{
					curve= *(texture->texture_curve_list);
					while (curve)
					{
						curve_next=curve->ptrnext;
						DEALLOCATE(curve);
						curve=curve_next;
					}
					DEALLOCATE(texture->texture_curve_list);
				}
				if (texture->texture_cell_list)
				{
					DEALLOCATE(*texture->texture_cell_list);
					DEALLOCATE(texture->texture_cell_list);
				}
				if (texture->global_texture_node_list)
				{
					DEALLOCATE(*texture->global_texture_node_list);
					DEALLOCATE(texture->global_texture_node_list);
				}
				if (texture->node_groups)
				{
					for (i=texture->n_groups;i>0;i--)
					{
						node_group=texture->node_groups[i];
						DEALLOCATE(node_group->nodes);
						DEALLOCATE(node_group);
					}
					DEALLOCATE(texture->node_groups);
					texture->n_groups = 0;
				}
				if (texture->mc_iso_surface)
				{
					number_of_scalar_fields=0;
					if (texture->scalar_field)
					{
						number_of_scalar_fields++;
					}
					if (texture->hollow_mode_on)
					{
						number_of_scalar_fields++;
					}
					if (texture->clip_field)
					{
/*???SAB.						number_of_scalar_fields++;*/

					}
					clean_mc_iso_surface(number_of_scalar_fields,texture->mc_iso_surface);
				}
				if (texture->scalar_field)
				{
					DEALLOCATE(texture->scalar_field->scalar);
					DEALLOCATE(texture->scalar_field);
				}
				if (texture->clip_field)
				{
					DEALLOCATE(texture->clip_field->scalar);
					DEALLOCATE(texture->clip_field);
				}
				if (texture->clip_field2)
				{
					DEALLOCATE(texture->clip_field2->scalar);
					DEALLOCATE(texture->clip_field2);
				}
				if (texture->coordinate_field)
				{
					DEALLOCATE(texture->coordinate_field->vector);
					DEALLOCATE(texture->coordinate_field);
				}
				if (texture->grid_spacing)
				{
					DEALLOCATE(texture->grid_spacing);
				}
				DEALLOCATE(texture->mc_iso_surface);
				/*???DB.  accessing and deaccessing iso_poly_material ? */
				DEALLOCATE(*texture_address);
				return_code=1;
			}
			else
			{
				return_code=0;
			}
		}
		else
		{
			return_code=1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(VT_volume_texture).  Invalid argument");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(VT_volume_texture) */

DECLARE_OBJECT_FUNCTIONS(VT_volume_texture)
DECLARE_DEFAULT_GET_OBJECT_NAME_FUNCTION(VT_volume_texture)

DECLARE_INDEXED_LIST_FUNCTIONS(VT_volume_texture)

DECLARE_FIND_BY_IDENTIFIER_IN_INDEXED_LIST_FUNCTION(VT_volume_texture,name,const char *,
	strcmp)

DECLARE_INDEXED_LIST_IDENTIFIER_CHANGE_FUNCTIONS(VT_volume_texture,name)

PROTOTYPE_MANAGER_COPY_WITH_IDENTIFIER_FUNCTION(VT_volume_texture,name)
{
	char *name;
	int return_code;

	ENTER(MANAGER_COPY_WITH_IDENTIFIER(VT_volume_texture,name));
	/* check arguments */
	if (source&&destination)
	{
		if (source->name)
		{
			if (ALLOCATE(name,char,strlen(source->name)+1))
			{
				strcpy(name,source->name);
				return_code=1;
			}
			else
			{
				display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITH_IDENTIFIER(VT_volume_texture,name).  Insufficient memory for name");
				return_code=0;
			}
		}
		else
		{
			name=(char *)NULL;
			return_code=1;
		}
		if (return_code)
		{
			return_code = MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name)(
				destination, source);
			if (return_code)
			{
				/* copy values */
				DEALLOCATE(destination->name);
				destination->name=name;
			}
			else
			{
				DEALLOCATE(name);
				display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITH_IDENTIFIER(VT_volume_texture,name).  Could not copy without identifier");
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITH_IDENTIFIER(VT_volume_texture,name).  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* MANAGER_COPY_WITH_IDENTIFIER(VT_volume_texture,name) */

PROTOTYPE_MANAGER_COPY_WITHOUT_IDENTIFIER_FUNCTION(VT_volume_texture,name)
/*******************************************************************************
LAST MODIFIED : 29 September 1998

DESCRIPTION :
Copies volume texture from source to destination.
Syntax: MANAGER_COPY_WITH_IDENTIFIER(VT_volume_texture,name)(destination,source)
???RC Previously did not clean up existing fields of destination, if set.
==============================================================================*/
{
	char *file_name;
	double *destination_double,*grid_spacing,*source_double;
	int *destination_int,*group_nodes,i,j,k,l,m,n_cells,n_groups,n_mc_cells,
		n_nodes,n_points,n_triangle_lists = 0,return_code,*source_deform,
		*source_int,maxk;
	struct MC_cell *cell,**cells,*source_cell,**source_cells;
	struct MC_iso_surface *iso_surface,*source_iso_surface;
	struct MC_triangle **compiled_triangle,**source_compiled_triangle,
		*source_triangle,**source_triangle_ptr,*triangle,**triangle_ptr;
	struct MC_vertex **compiled_vertex,**source_compiled_vertex,
		*source_vertex,*vertex;
	struct VT_node_group **destination_node_groups,**node_groups,
		**source_node_groups;
	struct VT_scalar_field *clip_field,*clip_field2,*scalar_field;
	struct VT_texture_cell **destination_texture_cell,*destination_cell_block,
		**source_texture_cell,**texture_cell_list;
	struct VT_texture_curve *destination_curve,*source_curve,**texture_curve_list;
	struct VT_texture_node **destination_node,*destination_node_block,
		**source_node,**texture_node_list;
	struct VT_vector_field *coordinate_field;

	ENTER(MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name));
/*???debug */
	/*printf("enter MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name)\n");*/
	/* check arguments */
	if (source&&destination)
	{
		return_code=1;
		file_name=(char *)NULL;
		texture_curve_list=(struct VT_texture_curve **)NULL;
		texture_cell_list=(struct VT_texture_cell **)NULL;
		texture_node_list=(struct VT_texture_node **)NULL;
		scalar_field=(struct VT_scalar_field *)NULL;
		clip_field=(struct VT_scalar_field *)NULL;
		clip_field2=(struct VT_scalar_field *)NULL;
		coordinate_field=(struct VT_vector_field *)NULL;
		iso_surface=(struct MC_iso_surface *)NULL;
		grid_spacing=(double *)NULL;
		node_groups=(struct VT_node_group **)NULL;

		if (source->file_name)
		{
			if (ALLOCATE(file_name,char,strlen(source->file_name)+1))
			{
				strcpy(file_name,source->file_name);
			}
			else
			{
				display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name).  Insufficient memory for image file name");
				return_code=0;
			}
		}

		if (return_code)
		{
			/* texture curve list */
			texture_curve_list=source->texture_curve_list;
			if (texture_curve_list)
			{
				source_curve= *texture_curve_list;
				if (ALLOCATE(texture_curve_list,struct VT_texture_curve *,1))
				{
					if (source_curve)
					{
						if (ALLOCATE(destination_curve,struct VT_texture_curve,1))
						{
							*texture_curve_list=destination_curve;
							do
							{
								destination_curve->type=source_curve->type;
								destination_curve->index=source_curve->index;
								(destination_curve->scalar_value)[0]=
									(source_curve->scalar_value)[0];
								(destination_curve->scalar_value)[1]=
									(source_curve->scalar_value)[1];
								(destination_curve->point1)[0]=(source_curve->point1)[0];
								(destination_curve->point1)[1]=(source_curve->point1)[1];
								(destination_curve->point1)[2]=(source_curve->point1)[2];
								(destination_curve->point2)[0]=(source_curve->point2)[0];
								(destination_curve->point2)[1]=(source_curve->point2)[1];
								(destination_curve->point2)[2]=(source_curve->point2)[2];
								(destination_curve->point3)[0]=(source_curve->point3)[0];
								(destination_curve->point3)[1]=(source_curve->point3)[1];
								(destination_curve->point3)[2]=(source_curve->point3)[2];
								(destination_curve->point4)[0]=(source_curve->point4)[0];
								(destination_curve->point4)[1]=(source_curve->point4)[1];
								(destination_curve->point4)[2]=(source_curve->point4)[2];
								destination_curve->ptrnext=(struct VT_texture_curve *)NULL;
								source_curve=source_curve->ptrnext;
								if (source_curve)
								{
									if (ALLOCATE(destination_curve->ptrnext,
										struct VT_texture_curve,1))
									{
										destination_curve=destination_curve->ptrnext;
									}
									else
									{
										display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name).  Insufficient memory for texture curve");
										return_code=0;
									}
								}
							}
							while (return_code&&source_curve);
						}
						else
						{
							display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name).  Insufficient memory for texture curve");
							return_code=0;
							DEALLOCATE(texture_curve_list);
						}
					}
					else
					{
						*texture_curve_list=(struct VT_texture_curve *)NULL;
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name).  Insufficient memory for texture curve list");
					return_code=0;
				}
			}
			if (return_code)
			{
				/* texture cell list */
				source_texture_cell=source->texture_cell_list;
				if (source_texture_cell)
				{
					n_cells=(source->dimension[0])*(source->dimension[1])*
						(source->dimension[2]);
					if (ALLOCATE(texture_cell_list,struct VT_texture_cell *,n_cells))
					{
						destination_texture_cell=texture_cell_list;
						i=n_cells;
						while (i>0)
						{
							*destination_texture_cell=(struct VT_texture_cell *)NULL;
							destination_texture_cell++;
							i--;
						}
						destination_texture_cell=texture_cell_list;
						i=0;
						if (ALLOCATE(destination_cell_block,struct VT_texture_cell,n_cells))
						{
							while (return_code&&(i<n_cells))
							{
								if (*source_texture_cell)
								{
									*destination_texture_cell = destination_cell_block+i;
									(*destination_texture_cell)->index=
										(*source_texture_cell)->index;
									(*destination_texture_cell)->scalar_value=
										(*source_texture_cell)->scalar_value;
								}
								else
								{
									display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name).  Insufficient memory for texture cell");
									return_code=0;
								}
								source_texture_cell++;
								destination_texture_cell++;
								i++;
							}
						}
					}
					else
					{
						display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name).  Insufficient memory for texture cell list");
						return_code=0;
					}
				}
				if (return_code)
				{
					/* node_groups */
					source_node_groups=source->node_groups;
					if (source_node_groups)
					{
						n_groups=source->n_groups;
						if (ALLOCATE(node_groups,struct VT_node_group *,n_groups))
						{
							destination_node_groups=node_groups;
							i=n_groups;
							while (i>0)
							{
								*destination_node_groups=(struct VT_node_group *)NULL;
								destination_node_groups++;
								i--;
							}
							destination_node_groups=node_groups;
							i=n_groups;
							while (return_code&&(i>0))
							{
								if (*source_node_groups)
								{
									if (ALLOCATE(*destination_node_groups,struct VT_node_group,1))
									{
										(*destination_node_groups)->n_nodes=
											(*source_node_groups)->n_nodes;
										if (ALLOCATE((*destination_node_groups)->name,char,
											strlen((*source_node_groups)->name)+1))
										{
											strcpy((*destination_node_groups)->name,
												(*source_node_groups)->name);
											if ((ALLOCATE(group_nodes,int,
												(*source_node_groups)->n_nodes)))
											{
												(*destination_node_groups)->nodes=group_nodes;
												for (j=0;j<(*source_node_groups)->n_nodes;j++)
												{
													(*destination_node_groups)->nodes[j]=
														(*source_node_groups)->nodes[j];
												}
											}
											else
											{
												display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name).  Insufficient memory for texture node group nodes 3");
												DEALLOCATE(*destination_node_groups);
												DEALLOCATE((*destination_node_groups)->name);
												return_code=0;
											}
										}
										else
										{
											display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name).  Insufficient memory for texture node_group 2");
											DEALLOCATE(*destination_node_groups);
											return_code=0;
										}
									}
									else
									{
										display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name).  Insufficient memory for texture node_group 1");
										return_code=0;
									}
								}
								else
								{
									*destination_node_groups=(struct VT_node_group *)NULL;
								}
								source_node_groups++;
								destination_node_groups++;
								i--;
							}
							if (!return_code)
							{
								i++;
								destination_node_groups--;
								while (i<n_groups)
								{
									i++;
									destination_node_groups--;
									if (*destination_node_groups)
									{
										DEALLOCATE((*destination_node_groups)->nodes);
										DEALLOCATE((*destination_node_groups)->name);
										DEALLOCATE(*destination_node_groups);
									}
								}
							}
						}
						else
						{
							display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name).  Insufficient memory for texture node groups");
							return_code=0;
						}
					}
				}
				if (return_code)
				{
					/* global node list */
					source_node=source->global_texture_node_list;
					if (source_node)
					{
						n_nodes=(source->dimension[0]+1)*(source->dimension[1]+1)*
							(source->dimension[2]+1);
						if (ALLOCATE(texture_node_list,struct VT_texture_node *,n_nodes))
						{
							destination_node=texture_node_list;
							i=n_nodes;
							while (i>0)
							{
								*destination_node=(struct VT_texture_node *)NULL;
								destination_node++;
								i--;
							}
							destination_node=texture_node_list;
							i=0;
							if (ALLOCATE(destination_node_block,struct VT_texture_node,n_nodes))
							{
								while (return_code&&(i<n_nodes))
								{
									if (*source_node)
									{
										*destination_node = destination_node_block+i;
										(*destination_node)->index=(*source_node)->index;
										((*destination_node)->scalar_gradient)[0]=
											((*source_node)->scalar_gradient)[0];
										((*destination_node)->scalar_gradient)[1]=
											((*source_node)->scalar_gradient)[1];
										((*destination_node)->scalar_gradient)[2]=
											((*source_node)->scalar_gradient)[2];
										(*destination_node)->clipping_fn_value=
											(*source_node)->clipping_fn_value;
										(*destination_node)->scalar_value=
											(*source_node)->scalar_value;
										(*destination_node)->active=(*source_node)->active;
										(*destination_node)->node_type=(*source_node)->node_type;
										for (j=0;j<8;j++)
										{
											(*destination_node)->cm_node_identifier[j]=
												(*source_node)->cm_node_identifier[j];
										}
									}
									source_node++;
									destination_node++;
									i++;
								}
							}
							else
							{
								display_message(ERROR_MESSAGE,
									"MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name).  Insufficient memory for texture node");
								return_code=0;
							}
						}
						else
						{
							display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name).  Insufficient memory for texture node list");
							return_code=0;
						}
					}
					if (return_code)
					{
						/* scalar field */
						if (source->scalar_field)
						{
							if (ALLOCATE(scalar_field,struct VT_scalar_field,1))
							{
								(scalar_field->dimension)[0]=
									(source->scalar_field->dimension)[0];
								(scalar_field->dimension)[1]=
									(source->scalar_field->dimension)[1];
								(scalar_field->dimension)[2]=
									(source->scalar_field->dimension)[2];
								if ((0<(scalar_field->dimension)[0])&&
									(0<(scalar_field->dimension)[1])&&
									(0<(scalar_field->dimension)[2]))
								{
									n_points=((scalar_field->dimension)[0]+1)*
										((scalar_field->dimension)[1]+1)*
										((scalar_field->dimension)[2]+1);
									if (ALLOCATE(scalar_field->scalar,double,n_points))
									{
										destination_double=scalar_field->scalar;
										source_double=source->scalar_field->scalar;
										for (i=n_points;i>0;i--)
										{
											*destination_double= *source_double;
											destination_double++;
											source_double++;
										}
									}
									else
									{
										display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name).  Insufficient memory for scalar field values");
										return_code=0;
									}
								}
								else
								{
									scalar_field->scalar=(double *)NULL;
								}
							}
							else
							{
								display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name).  Insufficient memory for scalar field");
								return_code=0;
							}
						}
						if (return_code)
						{
							/* clip field */
							if (source->clip_field)
							{
								if (ALLOCATE(clip_field,struct VT_scalar_field,1))
								{
									(clip_field->dimension)[0]=(source->clip_field->dimension)[0];
									(clip_field->dimension)[1]=(source->clip_field->dimension)[1];
									(clip_field->dimension)[2]=(source->clip_field->dimension)[2];
									if ((0<(clip_field->dimension)[0])&&
										(0<(clip_field->dimension)[1])&&
										(0<(clip_field->dimension)[2]))
									{
										n_points=((clip_field->dimension)[0]+1)*
											((clip_field->dimension)[1]+1)*
											((clip_field->dimension)[2]+1);
										if (ALLOCATE(clip_field->scalar,double,n_points))
										{
											destination_double=clip_field->scalar;
											source_double=source->clip_field->scalar;
											for (i=n_points;i>0;i--)
											{
												*destination_double= *source_double;
												destination_double++;
												source_double++;
											}
										}
										else
										{
											display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name).  Insufficient memory for clip field values");
											return_code=0;
										}
									}
									else
									{
										clip_field->scalar=(double *)NULL;
									}
								}
								else
								{
									display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name).  Insufficient memory for clip field");
									return_code=0;
								}
							}
							if (return_code)
							{
								/* clip field 2 */
								if (source->clip_field2)
								{
									if (ALLOCATE(clip_field2,struct VT_scalar_field,1))
									{
										(clip_field2->dimension)[0]=
											(source->clip_field2->dimension)[0];
										(clip_field2->dimension)[1]=
											(source->clip_field2->dimension)[1];
										(clip_field2->dimension)[2]=
											(source->clip_field2->dimension)[2];
										if ((0<(clip_field2->dimension)[0])&&
											(0<(clip_field2->dimension)[1])&&
											(0<(clip_field2->dimension)[2]))
										{
											n_points=((clip_field2->dimension)[0]+1)*
												((clip_field2->dimension)[1]+1)*
												((clip_field2->dimension)[2]+1);
											if (ALLOCATE(clip_field2->scalar,double,n_points))
											{
												destination_double=clip_field2->scalar;
												source_double=source->clip_field2->scalar;
												for (i=n_points;i>0;i--)
												{
													*destination_double= *source_double;
													destination_double++;
													source_double++;
												}
											}
											else
											{
												display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name).  Insufficient memory for clip field 2 values");
												return_code=0;
											}
										}
										else
										{
											clip_field2->scalar=(double *)NULL;
										}
									}
									else
									{
										display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name).  Insufficient memory for clip field 2");
										return_code=0;
									}
								}
								if (return_code)
								{
									/* coordinate field */
									if (source->coordinate_field)
									{
										if (ALLOCATE(coordinate_field,struct VT_vector_field,1))
										{
											(coordinate_field->dimension)[0]=
												(source->coordinate_field->dimension)[0];
											(coordinate_field->dimension)[1]=
												(source->coordinate_field->dimension)[1];
											(coordinate_field->dimension)[2]=
												(source->coordinate_field->dimension)[2];
											if ((0<(coordinate_field->dimension)[0])&&
												(0<(coordinate_field->dimension)[1])&&
												(0<(coordinate_field->dimension)[2]))
											{
												n_points=((coordinate_field->dimension)[0]+1)*
													((coordinate_field->dimension)[1]+1)*
													((coordinate_field->dimension)[2]+1);
												if (ALLOCATE(coordinate_field->vector,double,
													3*n_points))
												{
													destination_double=coordinate_field->vector;
													source_double=source->coordinate_field->vector;
													for (i=3*n_points;i>0;i--)
													{
														*destination_double= *source_double;
														destination_double++;
														source_double++;
													}
												}
												else
												{
													display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name).  Insufficient memory for coordinate field values");
													return_code=0;
												}
											}
											else
											{
												coordinate_field->vector=(double *)NULL;
											}
										}
										else
										{
											display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name).  Insufficient memory for coordinate field");
											return_code=0;
										}
									}
									if (return_code)
									{
										/* iso surface */
										source_iso_surface=source->mc_iso_surface;
										if (source_iso_surface)
										{
											if (ALLOCATE(iso_surface,struct MC_iso_surface,1))
											{
												(iso_surface->dimension)[0]=
													(source_iso_surface->dimension)[0];
												(iso_surface->dimension)[1]=
													(source_iso_surface->dimension)[1];
												(iso_surface->dimension)[2]=
													(source_iso_surface->dimension)[2];
												(iso_surface->active_block)[0]=
													(source_iso_surface->active_block)[0];
												(iso_surface->active_block)[1]=
													(source_iso_surface->active_block)[1];
												(iso_surface->active_block)[2]=
													(source_iso_surface->active_block)[2];
												(iso_surface->active_block)[3]=
													(source_iso_surface->active_block)[3];
												(iso_surface->active_block)[4]=
													(source_iso_surface->active_block)[4];
												(iso_surface->active_block)[5]=
													(source_iso_surface->active_block)[5];
												iso_surface->n_scalar_fields=
													source_iso_surface->n_scalar_fields;
												iso_surface->n_vertices=source_iso_surface->n_vertices;
												iso_surface->n_triangles=
													source_iso_surface->n_triangles;
												(iso_surface->xi_face_poly_index)[0]=
													(source_iso_surface->xi_face_poly_index)[0];
												(iso_surface->xi_face_poly_index)[1]=
													(source_iso_surface->xi_face_poly_index)[1];
												(iso_surface->xi_face_poly_index)[2]=
													(source_iso_surface->xi_face_poly_index)[2];
												(iso_surface->xi_face_poly_index)[3]=
													(source_iso_surface->xi_face_poly_index)[3];
												(iso_surface->xi_face_poly_index)[4]=
													(source_iso_surface->xi_face_poly_index)[4];
												(iso_surface->xi_face_poly_index)[5]=
													(source_iso_surface->xi_face_poly_index)[5];

												/* 1. copy over deformable vertices if list not NULL */
												source_deform=source_iso_surface->deform;
												if (source_deform)
												{
													if (ALLOCATE(iso_surface->deform,int,
														source_iso_surface->n_vertices))
													{
														for (i=0;i<source_iso_surface->n_vertices;i++)
														{
															iso_surface->deform[i]=source_deform[i];
														}
													}
													else
													{
														display_message(ERROR_MESSAGE,
															"MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name).  "
															"Could not copy deform field");
														return_code=0;
													}
												}
												else
												{
													iso_surface->deform = NULL;
												}

												/* 2. Copy the compiled vertex list, if any */
												if (return_code&&(0<source_iso_surface->n_vertices)&&
													(source_compiled_vertex=source_iso_surface->compiled_vertex_list))
												{
													iso_surface->n_vertices=source_iso_surface->n_vertices;
													if (ALLOCATE(iso_surface->compiled_vertex_list,
														struct MC_vertex *,iso_surface->n_vertices))
													{
														compiled_vertex=iso_surface->compiled_vertex_list;
														for (i=0;return_code&&(i<iso_surface->n_vertices);i++)
														{
															source_vertex= *source_compiled_vertex;
															source_compiled_vertex++;
															/* create a copy of the source vertex */
															if (ALLOCATE(vertex,struct MC_vertex,1)&&
																ALLOCATE(vertex->triangle_ptrs,struct MC_triangle *,
																	source_vertex->n_triangle_ptrs))
															{
																*compiled_vertex=vertex;
																compiled_vertex++;
																vertex->vertex_index=source_vertex->vertex_index;
																(vertex->coord)[0]=(source_vertex->coord)[0];
																(vertex->coord)[1]=(source_vertex->coord)[1];
																(vertex->coord)[2]=(source_vertex->coord)[2];
																vertex->n_triangle_ptrs=source_vertex->n_triangle_ptrs;
																/* set the following once the triangles are copied */
																for (m=0;m < vertex->n_triangle_ptrs;m++)
																{
																	(vertex->triangle_ptrs)[m]=NULL;
																}
															}
															else
															{
																return_code=0;
																if (vertex)
																{
																	DEALLOCATE(vertex);
																}
																/* only the first i vertices need cleaning up */
																iso_surface->n_vertices=i;
															}
														}
													}
													else
													{
														return_code=0;
													}
													if (!return_code)
													{
														display_message(ERROR_MESSAGE,
															"MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name).  "
															"Could not copy iso_surface compiled vertex list");
													}
												}
												else
												{
													iso_surface->compiled_vertex_list=(struct MC_vertex **)NULL;
													iso_surface->n_vertices=0;
												}

												/* 3. Copy the compiled triangle list, if any */
												if (return_code&&(0<source_iso_surface->n_triangles)&&
													(source_compiled_triangle=
														source_iso_surface->compiled_triangle_list))
												{
													iso_surface->n_triangles=source_iso_surface->n_triangles;
													if (ALLOCATE(iso_surface->compiled_triangle_list,
														struct MC_triangle *,iso_surface->n_triangles))
													{
														compiled_triangle=iso_surface->compiled_triangle_list;
														for (i=0;return_code&&(i<iso_surface->n_triangles);i++)
														{
															source_triangle= *source_compiled_triangle;
															source_compiled_triangle++;
															/* create a copy of the source triangle */
															if (ALLOCATE(triangle,struct MC_triangle,1))
															{
																*compiled_triangle=triangle;
																compiled_triangle++;
																triangle->triangle_index=source_triangle->triangle_index;
																for (j=0;j<3;j++)
																{
																	(triangle->vertex_index)[j]=
																		(source_triangle->vertex_index)[j];
																	(triangle->vertices)[j]=
																		(iso_surface->compiled_vertex_list)
																		[(triangle->vertex_index)[j]];
																}
																triangle->triangle_list_index=
																	source_triangle->triangle_list_index;
																/* set the following once cells are copied */
																triangle->cell_ptr=(struct MC_cell *)NULL;
															}
															else
															{
																return_code=0;
																/* only the first i triangles need cleaning up */
																iso_surface->n_triangles=i;
															}
														}
													}
													else
													{
														return_code=0;
													}
													if (!return_code)
													{
														display_message(ERROR_MESSAGE,
															"MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name).  "
															"Could not copy iso_surface compiled triangle list");
													}
												}
												else
												{
													iso_surface->compiled_triangle_list=(struct MC_triangle **)NULL;
													iso_surface->n_triangles=0;
												}

												/* 4. Get vertex->triangle_ptrs to point at correct triangles */
												if (return_code&&
													(compiled_vertex=iso_surface->compiled_vertex_list)&&
													(source_compiled_vertex=source_iso_surface->compiled_vertex_list))
												{
													for (i=0;return_code&&(i<iso_surface->n_vertices);i++)
													{
														vertex= *compiled_vertex;
														compiled_vertex++;
														source_vertex= *source_compiled_vertex;
														source_compiled_vertex++;
														for (m=0;m < vertex->n_triangle_ptrs;m++)
														{
															(vertex->triangle_ptrs)[m]=
																(iso_surface->compiled_triangle_list)
																	[((source_vertex->triangle_ptrs)[m])->triangle_index];
														}
													}
												}

												/* 5. Copy the marching cubes cells */
												if (return_code&&(source_cells=source_iso_surface->mc_cells))
												{
													n_mc_cells=
														((iso_surface->dimension)[0]+2)*
														((iso_surface->dimension)[1]+2)*
														((iso_surface->dimension)[2]+2);
													if (ALLOCATE(iso_surface->mc_cells,
														struct MC_cell *,n_mc_cells))
													{
														cells=iso_surface->mc_cells;
														n_triangle_lists=(iso_surface->n_scalar_fields)+6;
														for (i=0;return_code&&(i<n_mc_cells);i++)
														{
															source_cell = *source_cells;
															source_cells++;
															if (source_cell)
															{
																if (ALLOCATE(cell,struct MC_cell,1)&&
																	(ALLOCATE(cell->n_triangles,int,n_triangle_lists))&&
																	ALLOCATE(cell->triangle_list,
																		struct MC_triangle **,n_triangle_lists))
																{
																	(cell->index)[0]=(source_cell->index)[0];
																	(cell->index)[1]=(source_cell->index)[1];
																	(cell->index)[2]=(source_cell->index)[2];
																	for (j=0;return_code&&(j<n_triangle_lists);j++)
																	{
																		maxk=(cell->n_triangles)[j]=
																			(source_cell->n_triangles)[j];
																		if ((source_cell->triangle_list)[j])
																		{
																			if (ALLOCATE((cell->triangle_list)[j],
																				struct MC_triangle *,maxk))
																			{
																				triangle_ptr=(cell->triangle_list)[j];
																				source_triangle_ptr=(source_cell->triangle_list)[j];
																				for (k=0;return_code&&(k<maxk);k++)
																				{
																					if (iso_surface->compiled_triangle_list&&
																						(source_triangle = *source_triangle_ptr)&&
																						(triangle=(iso_surface->compiled_triangle_list)
																							[source_triangle->triangle_index]))
																					{
																						source_triangle_ptr++;
																						/* get triangle to point back at cell */
																						triangle->cell_ptr = cell;
																						*triangle_ptr = triangle;
																						triangle_ptr++;
																					}
																					else
																					{
																						DEALLOCATE((cell->triangle_list)[j]);
																						return_code=0;
																					}
																				}
																			}
																			else
																			{
																				return_code=0;
																			}
																			if (!return_code)
																			{
																				/* clean up triangle lists allocated up to now */
																				for (k=0;k<j;k++)
																				{
																					if ((cell->triangle_list)[k])
																					{
																						DEALLOCATE((cell->triangle_list)[k]);
																					}
																				}
																			}
																		}
																		else
																		{
																			(cell->triangle_list)[j]=
																				(struct MC_triangle **)NULL;
																			(cell->n_triangles)[j]=0;
																		}
																	}
																}
																else
																{
																	return_code=0;
																}
																if (!return_code)
																{
																	if (cell)
																	{
																		if (cell->n_triangles)
																		{
																			if (cell->triangle_list)
																			{
																				DEALLOCATE(cell->triangle_list);
																			}
																			DEALLOCATE(cell->n_triangles);
																		}
																		DEALLOCATE(cell);
																	}
																	return_code=0;
																}
															}
															else /* if (source_cell) */
															{
																cell=(struct MC_cell *)NULL;
															}
															*cells = cell;
															cells++;
															if (!return_code)
															{
																/* error; only the first i cells need cleaning up */
																n_mc_cells=i;
															}
														}
													}
													else
													{
														return_code=0;
													}
													if (!return_code)
													{
														display_message(ERROR_MESSAGE,
															"MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name).  "
															"Could not copy iso_surface cell list");
													}
												}
												else
												{
													iso_surface->mc_cells=(struct MC_cell **)NULL;
													n_mc_cells=0;
												}

												/* 6. Copy detail map - for variable cell triangulation */
												if (return_code&&source_iso_surface->detail_map)
												{
													/* n_mc_cells already set above if return_code not 0 */
													if (ALLOCATE(iso_surface->detail_map,int,n_mc_cells))
													{
														destination_int=iso_surface->detail_map;
														source_int=source_iso_surface->detail_map;
														for (i=0;i<n_mc_cells;i++)
														{
															*destination_int= *source_int;
															destination_int++;
															source_int++;
														}
													}
													else
													{
														display_message(ERROR_MESSAGE,
															"MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name).  "
															"Insufficient memory for detail map");
														return_code=0;
													}
												}
												else
												{
													iso_surface->detail_map=(int *)NULL;
												}
#if defined (DEBUG_CODE)
												/*???debug*/
												printf("Vertices:\n");
												for (i=0;i<iso_surface->n_vertices;i++)
												{
													vertex=iso_surface->compiled_vertex_list[i];
													source_vertex=source_iso_surface->compiled_vertex_list[i];
													if ((vertex->coord[0] != source_vertex->coord[0])||
														(vertex->coord[1] != source_vertex->coord[1])||
														(vertex->coord[2] != source_vertex->coord[2]))
													{
														printf("vertex %i (%i/%i) coords do not match\n",i,
															vertex->vertex_index,source_vertex->vertex_index);
													}
												}
												printf("Triangles:\n");
												for (i=0;i<iso_surface->n_triangles;i++)
												{
													triangle=iso_surface->compiled_triangle_list[i];
													source_triangle=source_iso_surface->compiled_triangle_list[i];
													if (
														((triangle->vertices[0])->vertex_index !=
															(source_triangle->vertices[0])->vertex_index)||
														((triangle->vertices[1])->vertex_index !=
															(source_triangle->vertices[1])->vertex_index)||
														((triangle->vertices[2])->vertex_index !=
															(source_triangle->vertices[2])->vertex_index))
													{
														printf("  v %i: %i / %i\n",i,source_triangle->triangle_index,
															triangle->triangle_index);
													}
													cell=triangle->cell_ptr;
													source_cell=source_triangle->cell_ptr;
													if (cell&&source_cell)
													{
														if (
															(cell->index[0] != source_cell->index[0])||
															(cell->index[1] != source_cell->index[1])||
															(cell->index[2] != source_cell->index[2]))
														{
															printf("  cell %i indices differ: %d %d %d / %d %d %d\n",i,
																cell->index[0],cell->index[1],cell->index[2],
																source_cell->index[0],
																source_cell->index[1],
																source_cell->index[2]);
														}
													}
													else
													{
														printf("  cell %i NULL! %p %p",i,cell,source_cell);
													}
												}
#endif /* defined (DEBUG_CODE) */

												/* 7. Clean up iso_surface in case of error */
												if (!return_code)
												{
													/* detail map */
													if (iso_surface->detail_map)
													{
														DEALLOCATE(iso_surface->detail_map);
													}
													/* cells */
													cells=iso_surface->mc_cells;
													if (cells)
													{
														n_triangle_lists=(iso_surface->n_scalar_fields)+6;
														for (i=0;i<n_mc_cells;i++)
														{
															cell= *cells;
															if (cell)
															{
																for (j=0;j<n_triangle_lists;j++)
																{
																	if ((cell->triangle_list)[j])
																	{
																		DEALLOCATE((cell->triangle_list)[j]);
																	}
																}
																DEALLOCATE(cell->n_triangles);
																DEALLOCATE(cell->triangle_list);
																DEALLOCATE(*cells);
															}
															cells++;
														}
														DEALLOCATE(iso_surface->mc_cells);
													}
													/* triangles */
													compiled_triangle=iso_surface->compiled_triangle_list;
													if (compiled_triangle)
													{
														for (i=0;i<iso_surface->n_triangles;i++)
														{
															DEALLOCATE(*compiled_triangle);
															compiled_triangle++;
														}
														DEALLOCATE(iso_surface->compiled_triangle_list);
													}
													/* vertices */
													compiled_vertex=iso_surface->compiled_vertex_list;
													if (compiled_vertex)
													{
														for (i=0;i<iso_surface->n_vertices;i++)
														{
															DEALLOCATE((*compiled_vertex)->triangle_ptrs);
															DEALLOCATE(*compiled_vertex);
															compiled_vertex++;
														}
														DEALLOCATE(iso_surface->compiled_vertex_list);
													}
													/* deformable vertices */
													if (iso_surface->deform)
													{
														DEALLOCATE(iso_surface->deform);
													}
												}
											}
											else
											{
												return_code=0;
											}
											if (!return_code)
											{
												display_message(ERROR_MESSAGE,
													"MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name).  "
													"Could not copy iso_surface");
											}
										}
										else
										{
											iso_surface=(struct MC_iso_surface *)NULL;
										}

										if (return_code)
										{
											grid_spacing=source->grid_spacing;
											if (grid_spacing)
											{
												i=(source->dimension)[0]+(source->dimension)[1]+
													(source->dimension)[2]+3;
												if (ALLOCATE(grid_spacing,double,i))
												{
													source_double=source->grid_spacing;
													destination_double=grid_spacing;
													while (i>0)
													{
														*destination_double= *source_double;
														destination_double++;
														source_double++;
														i--;
													}
												}
												else
												{
													display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name).  Insufficient memory for grid spacing");
													return_code=0;
												}
											}
											if (return_code)
											{
												/* copy values */
												/*???RC Was not clearing existing fields of destination.
													Now cleaning it up as we go. This is begging for
													destroy functions to do this. */
												/* file_name */
												DEALLOCATE(destination->file_name);
												destination->file_name=file_name;

												/* texture_curve_list */
												if (destination->texture_curve_list)
												{
													/*printf("destroy texture_curve_list\n");*/
													source_curve= *(destination->texture_curve_list);
													while (NULL != (destination_curve=source_curve))
													{
														source_curve=source_curve->ptrnext;
														DEALLOCATE(destination_curve);
													}
													DEALLOCATE(destination->texture_curve_list);
												}
												destination->texture_curve_list=texture_curve_list;

												/* texture_cell_list */
												if (destination->texture_cell_list)
												{
													/*printf("destroy texture_cell_list\n");*/
													DEALLOCATE(*destination->texture_cell_list);
													DEALLOCATE(destination->texture_cell_list);
												}
												destination->texture_cell_list=texture_cell_list;

												/* texture_node_list */
												if (destination->global_texture_node_list)
												{
													/*printf("destroy texture_node_list\n");*/
													destination_node=destination->global_texture_node_list;
													DEALLOCATE(*destination->global_texture_node_list);
													DEALLOCATE(destination->global_texture_node_list);
												}
												destination->global_texture_node_list=texture_node_list;

												/* scalar_field */
												if (destination->scalar_field)
												{
													/*printf("destroy scalar_field\n");*/
													DEALLOCATE(destination->scalar_field->scalar);
													DEALLOCATE(destination->scalar_field);
												}

												destination->scalar_field=scalar_field;
												/* clip_field */
												if (destination->clip_field)
												{
													/*printf("destroy clip_field\n");*/
													DEALLOCATE(destination->clip_field->scalar);
													DEALLOCATE(destination->clip_field);
												}
												destination->clip_field=clip_field;

												/* clip_field2 */
												if (destination->clip_field2)
												{
													/*printf("destroy clip_field2\n");*/
													DEALLOCATE(destination->clip_field2->scalar);
													DEALLOCATE(destination->clip_field2);
												}
												destination->clip_field2=clip_field2;

												/* coordinate_field */
												if (destination->coordinate_field)
												{
													/*printf("destroy coordinate_field\n");*/
													DEALLOCATE(destination->coordinate_field->vector);
													DEALLOCATE(destination->coordinate_field);
												}
												destination->coordinate_field=coordinate_field;

												/* mc_iso_surface */
												if (destination->mc_iso_surface)
												{
													/*printf("destroy mc_iso_surface\n");*/
													clean_mc_iso_surface(
														destination->mc_iso_surface->n_scalar_fields,
														destination->mc_iso_surface);
													DEALLOCATE(destination->mc_iso_surface);
												}
												destination->mc_iso_surface=iso_surface;

												/*???RC these parameters copied after dynamic members
													deallocated, since they need information such as
													original destination->dimension */
												destination->index=source->index;
												(destination->ximin)[0]=(source->ximin)[0];
												(destination->ximin)[1]=(source->ximin)[1];
												(destination->ximin)[2]=(source->ximin)[2];
												(destination->ximax)[0]=(source->ximax)[0];
												(destination->ximax)[1]=(source->ximax)[1];
												(destination->ximax)[2]=(source->ximax)[2];
												(destination->dimension)[0]=(source->dimension)[0];
												(destination->dimension)[1]=(source->dimension)[1];
												(destination->dimension)[2]=(source->dimension)[2];
												destination->grid_spacing=grid_spacing;

												destination->isovalue=source->isovalue;
												destination->hollow_mode_on=source->hollow_mode_on;
												destination->hollow_isovalue=source->hollow_isovalue;
												destination->clipping_field_on=
													source->clipping_field_on;
												destination->cutting_plane_on=source->cutting_plane_on;
												destination->disable_volume_functions=
													source->disable_volume_functions;
												destination->cut_isovalue=source->cut_isovalue;
												(destination->cutting_plane)[0]=
													(source->cutting_plane)[0];
												(destination->cutting_plane)[1]=
													(source->cutting_plane)[1];
												(destination->cutting_plane)[2]=
													(source->cutting_plane)[2];
												(destination->cutting_plane)[3]=
													(source->cutting_plane)[3];
												destination->closed_surface=source->closed_surface;
												destination->decimation_threshold=source->decimation_threshold;
												destination->calculate_nodal_values=
													source->calculate_nodal_values;
												destination->recalculate=source->recalculate;
											}
										}
									} /* if (return_code) */
								}
							}
						}
					}
				}
			}
		}
		if (!return_code)
		{
			/* free allocated memory */
			DEALLOCATE(file_name);
			if (texture_curve_list)
			{
				source_curve= *texture_curve_list;
				while (NULL != (destination_curve=source_curve))
				{
					source_curve=source_curve->ptrnext;
					DEALLOCATE(destination_curve);
				}
				DEALLOCATE(texture_curve_list);
			}
			if (texture_cell_list)
			{
				destination_texture_cell=texture_cell_list;
				DEALLOCATE(*texture_cell_list);
				DEALLOCATE(texture_cell_list);
			}
			if (texture_node_list)
			{
				DEALLOCATE(*texture_node_list);
				DEALLOCATE(texture_node_list);
			}
			if (scalar_field)
			{
				DEALLOCATE(scalar_field->scalar);
				DEALLOCATE(scalar_field);
			}
			if (clip_field)
			{
				DEALLOCATE(clip_field->scalar);
				DEALLOCATE(clip_field);
			}
			if (clip_field2)
			{
				DEALLOCATE(clip_field2->scalar);
				DEALLOCATE(clip_field2);
			}
			if (coordinate_field)
			{
				DEALLOCATE(coordinate_field->vector);
				DEALLOCATE(coordinate_field);
			}
			if (iso_surface)
			{
				if (iso_surface->mc_cells)
				{
					i=((iso_surface->dimension)[0]+2)*
						((iso_surface->dimension)[1]+2)*
						((iso_surface->dimension)[2]+2);
					cells=(iso_surface->mc_cells)+i;
					while (i<0)
					{
						cells--;
						cell= *cells;
						if (cell)
						{
							j=n_triangle_lists;
							while (j>0)
							{
								j--;
								if ((cell->triangle_list)[j])
								{
									k=(cell->n_triangles)[j];
									triangle_ptr=
										((cell->triangle_list)[j])+k;
									while (k>0)
									{
										triangle_ptr--;
										triangle= *triangle_ptr;
										k--;
										l=3;
										while (l>0)
										{
											l--;
											vertex=(triangle->vertices)[l];
											if (triangle==(vertex->
												triangle_ptrs)[0])
											{
												DEALLOCATE(vertex->
													triangle_ptrs);
												DEALLOCATE(vertex);
											}
										}
									}
								}
							}
							DEALLOCATE(cell->n_triangles);
							DEALLOCATE(cell->triangle_list);
							DEALLOCATE(*cells);
						}
						i++;
					}
					DEALLOCATE(iso_surface->compiled_vertex_list);
					DEALLOCATE(iso_surface->compiled_triangle_list);
					DEALLOCATE(iso_surface->mc_cells);
				}
				DEALLOCATE(iso_surface->detail_map);
				DEALLOCATE(iso_surface);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name).  Invalid argument(s)");
		return_code=0;
	}
/*???debug */
	/*printf("leave MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name) %d\n",
		return_code);*/
	LEAVE;

	return (return_code);
} /* MANAGER_COPY_WITHOUT_IDENTIFIER(VT_volume_texture,name) */

PROTOTYPE_MANAGER_COPY_IDENTIFIER_FUNCTION(VT_volume_texture,name,const char *)
{
	char *destination_name = NULL;
	int return_code;

	ENTER(MANAGER_COPY_IDENTIFIER(VT_volume_texture,name));
	/* check arguments */
	if (name&&destination)
	{
		if (name)
		{
			if (ALLOCATE(destination_name,char,strlen(name)+1))

			{
				strcpy(destination_name,name);
				return_code=1;
			}
			else
			{
				display_message(ERROR_MESSAGE,
			"MANAGER_COPY_IDENTIFIER(VT_volume_texture,name).  Insufficient memory");
				return_code=0;
			}
		}
		else
		{
			name=(char *)NULL;
			return_code=1;
		}
		if (return_code)
		{
			/* copy name */
			DEALLOCATE(destination->name);
			destination->name=destination_name;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"MANAGER_COPY_IDENTIFIER(VT_volume_texture,name).  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* MANAGER_COPY_IDENTIFIER(VT_volume_texture,name) */

DECLARE_MANAGER_FUNCTIONS(VT_volume_texture,manager)

DECLARE_DEFAULT_MANAGED_OBJECT_NOT_IN_USE_FUNCTION(VT_volume_texture,manager)

DECLARE_MANAGER_IDENTIFIER_FUNCTIONS(VT_volume_texture,name,const char *,manager)

int list_VT_volume_texture(struct VT_volume_texture *texture)
/*******************************************************************************
LAST MODIFIED : 21 September 1998

DESCRIPTION :
Writes the properties of the <texture> to the command window.
==============================================================================*/
{
	char line[80];
	int return_code;

	ENTER(list_VT_volume_texture);
	/* check the arguments */
	if (texture)
	{
		return_code=1;
		/* write the name */
		display_message(INFORMATION_MESSAGE,"volume_texture : ");
		display_message(INFORMATION_MESSAGE,texture->name);
		display_message(INFORMATION_MESSAGE,"\n");
		/* write the name of the file */
		display_message(INFORMATION_MESSAGE,"  file name : ");
		if (texture->file_name)
		{
			display_message(INFORMATION_MESSAGE,texture->file_name);
		}
		display_message(INFORMATION_MESSAGE,"\n");
		sprintf(line,"%.3g <= xi1 <= %.3g, %d divisions\n",(texture->ximin)[0],
			(texture->ximax)[0],(texture->dimension)[0]);
		display_message(INFORMATION_MESSAGE,line);
		sprintf(line,"%.3g <= xi2 <= %.3g, %d divisions\n",(texture->ximin)[1],
			(texture->ximax)[1],(texture->dimension)[1]);
		display_message(INFORMATION_MESSAGE,line);
		sprintf(line,"%.3g <= xi3 <= %.3g, %d divisions\n",(texture->ximin)[2],
			(texture->ximax)[2],(texture->dimension)[2]);
		display_message(INFORMATION_MESSAGE,line);
		display_message(INFORMATION_MESSAGE,"iso_surface %p\n", texture->mc_iso_surface);
		display_message(INFORMATION_MESSAGE,"access_count = %d\n",
			texture->access_count);
	}
	else
	{
		display_message(ERROR_MESSAGE,"list_VT_volume_texture.  Invalid argument");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* list_VT_volume_texture */

void generate_isosurface(struct VT_volume_texture *texture)
/*******************************************************************************
LAST MODIFIED : 7 April 1997

DESCRIPTION :
Creates/Updates isosurface for basic volume_texture
==============================================================================*/
{
	int i;
	int n_scalar_fields;
	struct VT_scalar_field *scalar_field_list[MAX_SCALAR_FIELDS];
	double isovalue_list[MAX_SCALAR_FIELDS];

	ENTER(generate_isosurface);
	/* calculate scalar field using filter */
	update_scalars(texture);
	scalar_field_list[0] = texture->scalar_field;
	isovalue_list[0] = texture->isovalue;
	if (texture->hollow_mode_on)
	{
		/* should combine clip field and this field in some way */
		for (i=(texture->clip_field2->dimension[0]+1)*
			(texture->clip_field2->dimension[1]+1)*
			(texture->clip_field2->dimension[2]+1)-1;i>=0;i--)
		{
			texture->clip_field2->scalar[i] = 1.0-texture->scalar_field->scalar[i];
		}
	}
	n_scalar_fields = 1;
	if (texture->cutting_plane_on)
	{
		scalar_field_list[n_scalar_fields] = texture->clip_field;
		isovalue_list[n_scalar_fields] = texture->cutting_plane[3];
		n_scalar_fields++;
	}
	if (texture->hollow_mode_on)
	{
		scalar_field_list[n_scalar_fields] = texture->clip_field2;
		isovalue_list[n_scalar_fields] = texture->hollow_isovalue*texture->isovalue;
		n_scalar_fields++;
	}
/*???debug */
/*printf("Marching cubes: isovalue = %lf : n_scalar_fields = %d : mode [hollow=%d] [closed = %d] [clip = %d]\n",
	texture->isovalue,n_scalar_fields,texture->hollow_mode_on,
	texture->closed_surface,texture->cutting_plane_on);*/
	/* create isosurface */
/*???DB.  Why does the iso value have to be between 0 and 1 ? */
/*  if (texture->isovalue >= 0.0 && texture->isovalue <= 1.0)*/
	{
		/* ( cutting plane[3] = clip isovalue ) */
		if (NULL==texture->mc_iso_surface)
		{
			display_message(ERROR_MESSAGE,
				"generate_isosurface.  mc_iso_surface = NULL");
		}
		marching_cubes(scalar_field_list,n_scalar_fields,texture->coordinate_field,
			texture->mc_iso_surface,isovalue_list,texture->closed_surface,
			(texture->cutting_plane_on)||(texture->hollow_mode_on));
	}
/*???debug */
/*else
{
	printf("error : isovalue is %f\n",texture->isovalue);
}*/
	LEAVE;
} /* generate_isosurface */

struct Clipping *CREATE(Clipping)(void)
/*******************************************************************************
LAST MODIFIED : 26 January 1996

DESCRIPTION :
Allocates memory for a clipping.
==============================================================================*/
{
	struct Clipping *clipping;

	ENTER(CREATE(Clipping));
	if (ALLOCATE(clipping,struct Clipping,1))
	{
		clipping->function=NULL;
		clipping->parameters=NULL;
	}
	else
	{
		display_message(ERROR_MESSAGE, "CREATE(Clipping).  Insufficient memory");
	}
	LEAVE;

	return (clipping);
} /* CREATE(Clipping) */

int DESTROY(Clipping)(struct Clipping **clipping_address)
/*******************************************************************************
LAST MODIFIED : 26 January 1996

DESCRIPTION :
Frees the memory for the clipping and sets <*clipping_address> to NULL.
==============================================================================*/
{
	int return_code;
	struct Clipping *clipping;

	ENTER(DESTROY(Clipping));
	if (clipping_address)
	{
		clipping= *clipping_address;
		if (clipping)
		{
			DEALLOCATE(clipping->parameters);
			DEALLOCATE(*clipping_address);
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"DESTROY(Clipping).  Invalid argument");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Clipping) */

struct VT_iso_vertex *CREATE(VT_iso_vertex)(void)
/*******************************************************************************
LAST MODIFIED : 9 November 2005

DESCRIPTION :
==============================================================================*/
{
	int k;
	struct VT_iso_vertex *vertex;

	ENTER(CREATE(VT_iso_vertex));

	/* allocate memory for structure */
	if (ALLOCATE(vertex,struct VT_iso_vertex,1))
	{
		for (k = 0 ; k < 3 ; k++)
		{
			vertex->coordinates[k] = 0.0;
			vertex->normal[k] = 0.0;
			vertex->texture_coordinates[k] = 0.0;
		}
		vertex->number_of_triangles = 0;
		vertex->triangles = (struct VT_iso_triangle **)NULL;
		vertex->data = (ZnReal *)NULL;
		vertex->index = 0;
	}
	else
	{
		display_message(ERROR_MESSAGE, "CREATE(VT_iso_vertex).  "
			"Unable to allocate memory for structure.");
	}
	LEAVE;

	return (vertex);
} /* CREATE(VT_iso_vertex) */

int DESTROY(VT_iso_vertex)(struct VT_iso_vertex **vertex_address)
/*******************************************************************************
LAST MODIFIED : 9 November 2005

DESCRIPTION :
Frees the memory for the volume vertex and sets <*vertex_address> to NULL.
==============================================================================*/
{
	int return_code = 0;
	struct VT_iso_vertex *vertex;

	ENTER(DESTROY(VT_iso_vertex));
	if (vertex_address && (vertex = *vertex_address))
	{
		if (vertex->triangles)
		{
			DEALLOCATE(vertex->triangles);
		}
		if (vertex->data)
		{
			DEALLOCATE(vertex->data);
		}
		DEALLOCATE(*vertex_address);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(VT_iso_vertex).  Invalid argument");
	}
	LEAVE;

	return (return_code);
} /* DESTROY(VT_iso_vertex) */

struct VT_iso_vertex *VT_iso_vertex_create_and_set(
	GLfloat *coordinates, GLfloat *normal, GLfloat *texture_coordinates,
	int n_data_components, GLfloat *data)
{
	int i;
	struct VT_iso_vertex *vertex;

	vertex = CREATE(VT_iso_vertex)();
	if ((NULL != vertex) && (NULL != coordinates) && (NULL != normal) &&
		((0 == n_data_components) || (NULL != data)))
	{
		for (i = 0; i < 3; i++)
		{
			vertex->coordinates[i] = coordinates[i];
			vertex->normal[i] = normal[i];
			if (NULL != texture_coordinates)
			{
				vertex->texture_coordinates[i] = texture_coordinates[i];
			}
		}
		if (n_data_components)
		{
			ALLOCATE(vertex->data, ZnReal, n_data_components);
			if (vertex->data)
			{
				for (i = 0; i < n_data_components; i++)
				{
					vertex->data[i] = data[i];
				}
			}
			else
			{
				DESTROY(VT_iso_vertex)(&vertex);
			}
		}
	}
	else
	{
		DESTROY(VT_iso_vertex)(&vertex);
	}

	return (vertex);
}

int VT_iso_vertex_normalise_normal(struct VT_iso_vertex *vertex)
/*******************************************************************************
LAST MODIFIED : 28 October 2005

DESCRIPTION :
Normalises the normal for the vertex.
==============================================================================*/
{
	int return_code = 0;

	ENTER(VT_iso_vertex_calculate_normal);
	if (vertex)
	{
		normalize3(vertex->normal);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"VT_iso_vertex_calculate_normal.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* VT_iso_vertex_calculate_normal */

struct VT_iso_triangle *CREATE(VT_iso_triangle)(void)
/*******************************************************************************
LAST MODIFIED : 9 November 2005

DESCRIPTION :
==============================================================================*/
{
	struct VT_iso_triangle *triangle;

	ENTER(CREATE(VT_iso_triangle));

	/* allocate memory for structure */
	if (ALLOCATE(triangle,struct VT_iso_triangle,1))
	{
		triangle->vertices[0] = (struct VT_iso_vertex *)NULL;
		triangle->vertices[1] = (struct VT_iso_vertex *)NULL;
		triangle->vertices[2] = (struct VT_iso_vertex *)NULL;
		triangle->index = 0;
	}
	else
	{
		display_message(ERROR_MESSAGE, "CREATE(VT_iso_triangle).  "
			"Unable to allocate memory for structure.");
	}
	LEAVE;

	return (triangle);
} /* CREATE(VT_iso_triangle) */

int DESTROY(VT_iso_triangle)(struct VT_iso_triangle **triangle_address)
/*******************************************************************************
LAST MODIFIED : 9 November 2005

DESCRIPTION :
Frees the memory for the volume triangle and sets <*triangle_address> to NULL.
==============================================================================*/
{
	int return_code = 0;

	ENTER(DESTROY(VT_iso_triangle));
	if (triangle_address)
	{
		DEALLOCATE(*triangle_address);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(VT_iso_triangle).  Invalid argument");
	}
	LEAVE;

	return (return_code);
} /* DESTROY(VT_iso_triangle) */

struct VT_iso_triangle *VT_iso_triangle_create_and_set(
	struct VT_iso_vertex **vertices)
{
	int i;
	struct VT_iso_triangle *triangle;
	struct VT_iso_triangle **temp_vt_triangles;
	struct VT_iso_vertex *vertex;

	triangle = CREATE(VT_iso_triangle)();
	if ((NULL != triangle) && (NULL != vertices))
	{
		for (i = 0; i < 3; i++)
		{
			vertex = vertices[i];
			if (NULL != vertex)
			{
				triangle->vertices[i] = vertex;
				temp_vt_triangles = NULL;
				if (REALLOCATE(temp_vt_triangles, vertex->triangles,
					struct VT_iso_triangle *, vertex->number_of_triangles+1))
				{
					vertex->triangles = temp_vt_triangles;
					vertex->triangles[vertex->number_of_triangles] = triangle;
					vertex->number_of_triangles++;
				}
				else
				{
					DESTROY(VT_iso_triangle)(&triangle);
					break;
				}
			}
			else
			{
				DESTROY(VT_iso_triangle)(&triangle);
				break;
			}
		}
	}
	else
	{
		DESTROY(VT_iso_triangle)(&triangle);
	}

	return (triangle);
}
