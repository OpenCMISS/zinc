/*******************************************************************************
FILE : renderbinarywavefront.c

LAST MODIFIED : 20 March 2003

DESCRIPTION :
Renders gtObjects to Wavefront OBJ file
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "general/debug.h"
#include "graphics/graphics_object.h"
#include "graphics/material.h"
#include "graphics/renderwavefront.h"
#include "graphics/renderbinarywavefront.h"
#include "graphics/spectrum.h"
#include "user_interface/message.h"
#include "graphics/graphics_object_private.h"

/*
Module types
------------
*/
struct vertex
/*******************************************************************************
LAST MODIFIED : 18 May 1998

DESCRIPTION :
This is the data structure defined by the plugin written by Patrick Palmer for MAYA
==============================================================================*/
{
	float x, y, z, pad;
};

struct Binary_wavefront_data
/*******************************************************************************
LAST MODIFIED : 18 May 1998

DESCRIPTION :
This is the data structure defined by the plugin written by Patrick Palmer for MAYA
==============================================================================*/
{
	int number_of_vertices;
	struct vertex *vertices;
	int number_of_polygons;
	int *polygons;                     /* Number of connections for this polygon */
	int number_of_polygon_connections; /* Total number of connections for all polygons */
	int *connections;
	int number_of_texture_coordinates;
	float *u_texture_coordinates;
	float *v_texture_coordinates;
	int number_of_texture_connections;
	int *texture_connections;
	int number_of_groups; /* Not generating any groups yet so this is zero */
};

/*
Module functions
----------------
*/
static int activate_material_wavefront(struct Binary_wavefront_data *data,
	struct Graphical_material *material)
/*******************************************************************************
LAST MODIFIED : 7 January 1998

DESCRIPTION :
Writes Wavefront object file that defines the material
==============================================================================*/
{
	int return_code;

	ENTER(activate_material_wavefront);
	if (material&&data)
	{
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"activate_material_wavefront.  Missing material or data");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* activate_material_wavefront */

#if defined (OLD_CODE)
static int spectrum_start_renderwavefront(struct Binary_wavefront_data *data, struct cmzn_spectrum *spectrum, struct Graphical_material *material)
/*******************************************************************************
LAST MODIFIED : 1 October 1997

DESCRIPTION :
Sets WAVEFRONT file for rendering values on the current material.
==============================================================================*/
{
	int return_code;
	ENTER(spectrum_start_renderwavefront);

	USE_PARAMETER(data);
	USE_PARAMETER(material);
	if ( spectrum )
	{
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
							"spectrum_start_renderwavefront.  Invalid spectrum object.");
		return_code=0;
	}

	LEAVE;
	return (return_code);
} /* spectrum_start_renderwavefront */

static int spectrum_renderwavefront_value(struct Binary_wavefront_data *data, struct cmzn_spectrum *spectrum,
	struct Graphical_material *material, float data_value)
/*******************************************************************************
LAST MODIFIED :  1 October 1997

DESCRIPTION :
Writes WAVEFRONT to represent the value 'data' in
accordance with the spectrum.
==============================================================================*/
{
	int return_code;

	ENTER(spectrum_renderwavefront_value);
	USE_PARAMETER(data);
	USE_PARAMETER(data_value);
	if ( spectrum && material )
	{
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
				"spectrum_renderwavefront_value.  Invalid arguments given.");
		return_code = 0;
	}

	LEAVE;
	return (return_code);
} /* spectrum_renderwavefront_value */
#endif /* defined (OLD_CODE) */

#ifdef MERGE_TIMES
static int spectrum_renderwavefront_merge(struct Binary_wavefront_data *data, struct cmzn_spectrum *spectrum,
				struct Graphical_material *material,
				int merge_number, float *data)
/*******************************************************************************
LAST MODIFIED : 1 October 1997

DESCRIPTION :
Writes wavefront to represent the vector of values 'data'
in accordance with the spectrum.
==============================================================================*/
{
	enum Spectrum_type type;
	float alpha,blue,green,red;
	int return_code;

	ENTER(spectrum_renderwavefront_merge);

	if ( spectrum )
	{
		get_Spectrum_type(spectrum,&type);
		if (MERGE_RGB_SPECTRUM==type)
		{
			if ( merge_number > 2 && data )
			{
				display_message(ERROR_MESSAGE,
									"spectrum_renderwavefront_merge. Not yet implemented.");
				return_code = 1;
			}
			else
			{
				display_message(ERROR_MESSAGE,
									"spectrum_renderwavefront_merge.  Require three merge times.");
				return_code=0;
			}
		}
		else
		{
		display_message(ERROR_MESSAGE,
							"spectrum_renderwavefront_merge.  Invalid spectrum type for merge.");
		return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
							"spectrum_renderwavefront_merge.  Invalid spectrum object.");
		return_code=0;
	}

	LEAVE;
	return (return_code);
} /* spectrum_renderwavefront_merge */
#endif

#if defined (OLD_CODE)
static int spectrum_end_renderwavefront(struct Binary_wavefront_data *data, struct cmzn_spectrum *spectrum)
/*******************************************************************************
LAST MODIFIED : 1 October 1997

DESCRIPTION :
Resets the graphics state after rendering values on current material.
==============================================================================*/
{
	int return_code;

	ENTER(spectrum_end_renderGL);
	USE_PARAMETER(data);
	if ( spectrum )
	{
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
							"spectrum_end_renderwavefront.  Invalid spectrum object.");
		return_code=0;
	}

	LEAVE;
	return (return_code);
} /* spectrum_end_renderwavefront */
#endif /* defined (OLD_CODE) */

static int draw_surface_wavefront(struct Binary_wavefront_data *data, Triple *surfpts,
	Triple *normalpts, Triple *texturepts,
	struct Graphical_material *material, int npts1,int npts2)
/*******************************************************************************
LAST MODIFIED : 1 October 1997

DESCRIPTION :
==============================================================================*/
{
	float *u_texture_index, *v_texture_index;
	int i,j,nptsij,return_code, *connection_index, *polygon_index, *texture_index;
	int index;
	struct vertex *point_index;

	ENTER(draw_surface_texmapwavefront);
	USE_PARAMETER(normalpts);
	if (surfpts&&(1<npts1)&&(1<npts2))
	{
		activate_material_wavefront( data, material );

		nptsij = npts1 * npts2;
		REALLOCATE(data->vertices, data->vertices, struct vertex,
			data->number_of_vertices + nptsij );
		point_index = data->vertices + data->number_of_vertices;
		for (i=0;i<npts1;i++)
		{
			for (j=0;j<npts2;j++)
			{
				point_index->x = surfpts[i+npts1*j][0];
				point_index->y = surfpts[i+npts1*j][1];
				point_index->z = surfpts[i+npts1*j][2];
				point_index->pad = 1;
				point_index++;
			}
		}
		REALLOCATE(data->u_texture_coordinates, data->u_texture_coordinates,
			float, data->number_of_texture_coordinates + nptsij);
		REALLOCATE(data->v_texture_coordinates, data->v_texture_coordinates,
			float, data->number_of_texture_coordinates + nptsij);
		u_texture_index = data->u_texture_coordinates + data->number_of_texture_coordinates;
		v_texture_index = data->v_texture_coordinates + data->number_of_texture_coordinates;
		for (i=0;i<npts1;i++)
		{
			for (j=0;j<npts2;j++)
			{
				*u_texture_index = texturepts[i+npts1*j][0];
				*v_texture_index = texturepts[i+npts1*j][1];
				u_texture_index++;
				v_texture_index++;
			}
		}
		
		REALLOCATE(data->connections, data->connections, int,
			data->number_of_polygon_connections + 4 * (npts1-1) * (npts2-1));
		connection_index = data->connections + data->number_of_polygon_connections;
		data->number_of_polygon_connections += 4 * (npts1-1) * (npts2-1);

		REALLOCATE(data->texture_connections, data->texture_connections, int,
			data->number_of_texture_connections + 4 * (npts1-1) * (npts2-1));
		texture_index = data->texture_connections + data->number_of_texture_connections;
		data->number_of_texture_connections += 4 * (npts1-1) * (npts2-1);
		
		REALLOCATE(data->polygons, data->polygons, int,
			data->number_of_polygons + (npts1-1) * (npts2-1));
		polygon_index = data->polygons + data->number_of_polygons;
		data->number_of_polygons += (npts1-1) * (npts2-1);

		index = 0;
		for (i=0;i<npts1-1;i++)
		{
			for (j=0;j<npts2-1;j++)
			{
				*polygon_index = 4;
				polygon_index++;

				*connection_index = data->number_of_vertices + index;
				*texture_index = data->number_of_texture_coordinates + index;
				connection_index++;
				texture_index++;
				*connection_index = data->number_of_vertices + index + 1;
				*texture_index = data->number_of_texture_coordinates + index + 1;
				connection_index++;
				texture_index++;
				*connection_index = data->number_of_vertices + index + npts2 + 1;
				*texture_index = data->number_of_texture_coordinates + index + npts2 + 1;
				connection_index++;
				texture_index++;
				*connection_index = data->number_of_vertices + index + npts2;
				*texture_index = data->number_of_texture_coordinates + index + npts2;
				connection_index++;
				texture_index++;

				index++;
			}
			index++;
		}
		data->number_of_vertices += nptsij;
		data->number_of_texture_coordinates += nptsij;
		return_code=1;
	}
	else
	{
		if ((1<npts1)&&(1<npts2))
		{
			display_message(ERROR_MESSAGE,
				"draw_surface_texmapwavefront.  Invalid argument(s)");
			return_code=0;
		}
		else
		{
			return_code=1;
		}
	}
	LEAVE;

	return (return_code);
} /* draw_surface_texmapwavefront */

static int make_binary_wavefront(struct Binary_wavefront_data *data,
	int full_write,
	gtObject *object, float time)
/*******************************************************************************
LAST MODIFIED : 20 March 2003

DESCRIPTION :
Convert graphical object into Wavefront object file.
==============================================================================*/
{
	float proportion, *times;
	int itime, number_of_times, return_code;
	union GT_primitive_list *primitive_list1, *primitive_list2;

	ENTER(make_binary_wavefront);
	return_code = 1;
	if (object)
	{
		number_of_times = object->number_of_times;
		if (0 < number_of_times)
		{
			itime = number_of_times;
			if ((itime>1)&&(times=object->times))
			{
				itime--;
				times += itime;
				if (time>= *times)
				{
					proportion=0;
				}
				else
				{
					while ((itime>0)&&(time< *times))
					{
						itime--;
						times--;
					}
					if (time< *times)
					{
						proportion=0;
					}
					else
					{
						proportion=times[1]-times[0];
						if (proportion>0)
						{
							proportion=time-times[0]/proportion;
						}
						else
						{
							proportion=0;
						}
					}
				}
			}
			else
			{
				itime=0;
				proportion=0;
			}
			if (object->primitive_lists &&
				(primitive_list1 = object->primitive_lists + itime))
			{
				if (proportion > 0)
				{
					if (!(primitive_list2 = object->primitive_lists + itime + 1))
					{
						display_message(ERROR_MESSAGE,
							"make_binary_wavefront.  Invalid primitive_list");
						return_code = 0;
					}
				}
				else
				{
					primitive_list2 = (union GT_primitive_list *)NULL;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"make_binary_wavefront.  Invalid primitive_lists");
				return_code = 0;
			}
		}
		if ((0 < number_of_times) && return_code)
		{
			switch (GT_object_get_type(object))
			{
				default:
				{
					display_message(ERROR_MESSAGE,"make_binary_wavefront.  Invalid object type");
					return_code=0;
				} break;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"make_binary_wavefront.  Missing object");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* make_binary_wavefront */

