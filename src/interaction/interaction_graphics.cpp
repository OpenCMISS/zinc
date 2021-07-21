/*******************************************************************************
FILE : interaction_graphics.c

LAST MODIFIED : 10 July 2000

DESCRIPTION :
Functions for building graphics assisting interaction, eg. rubber-band effect.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stdio.h>
#include "general/debug.h"
#include "interaction/interaction_graphics.h"
#include "graphics/graphics_object.hpp"
#include "general/message.h"

/*
Module functions
----------------
*/

int Interaction_volume_make_polyline_extents(
	struct Interaction_volume *interaction_volume,
	struct GT_object *graphics_object)
{
	ZnReal model_vertex[8][3];
	/* use 0.999999 to avoid clipping problems for frustums */
	static ZnReal normalised_vertex[8][3]=
	{
		{-0.9999,-0.9999,-0.9999},
		{-0.9999,-0.9999, 0.9999},
		{-0.9999, 0.9999,-0.9999},
		{-0.9999, 0.9999, 0.9999},
		{ 0.9999,-0.9999,-0.9999},
		{ 0.9999,-0.9999, 0.9999},
		{ 0.9999, 0.9999,-0.9999},
		{ 0.9999, 0.9999, 0.9999}
	};
	int i,return_code;
	struct GT_polyline_vertex_buffers *polyline;
	Triple *point,*points;

	if (interaction_volume&&graphics_object&&
		(g_POLYLINE_VERTEX_BUFFERS==GT_object_get_type(graphics_object)))
	{
		return_code=1;
		/* get the 8 vertices of the interaction_volume frustum */
		for (i=0;(i<8)&&return_code;i++)
		{
			return_code=Interaction_volume_normalised_to_model_coordinates(
				interaction_volume,normalised_vertex[i],model_vertex[i]);
		}
		if (return_code)
		{
			/* make a polyline big enough for the 12 line segment edges of the box */
			if (ALLOCATE(points,Triple,24)&&
				(polyline=CREATE(GT_polyline_vertex_buffers)(g_PLAIN_DISCONTINUOUS, 0)))
			{
				point=points;
				/* lines spanning axis 1 */
				for (i=0;i<4;i++)
				{
					(*point)[0]=(GLfloat)model_vertex[i][0];
					(*point)[1]=(GLfloat)model_vertex[i][1];
					(*point)[2]=(GLfloat)model_vertex[i][2];
					point++;
					(*point)[0]=(GLfloat)model_vertex[i+4][0];
					(*point)[1]=(GLfloat)model_vertex[i+4][1];
					(*point)[2]=(GLfloat)model_vertex[i+4][2];
					point++;
				}
				/* lines spanning axis 2 */
				for (i=0;i<2;i++)
				{
					(*point)[0]=(GLfloat)model_vertex[i*4][0];
					(*point)[1]=(GLfloat)model_vertex[i*4][1];
					(*point)[2]=(GLfloat)model_vertex[i*4][2];
					point++;
					(*point)[0]=(GLfloat)model_vertex[i*4+2][0];
					(*point)[1]=(GLfloat)model_vertex[i*4+2][1];
					(*point)[2]=(GLfloat)model_vertex[i*4+2][2];
					point++;
					(*point)[0]=(GLfloat)model_vertex[i*4+1][0];
					(*point)[1]=(GLfloat)model_vertex[i*4+1][1];
					(*point)[2]=(GLfloat)model_vertex[i*4+1][2];
					point++;
					(*point)[0]=(GLfloat)model_vertex[i*4+3][0];
					(*point)[1]=(GLfloat)model_vertex[i*4+3][1];
					(*point)[2]=(GLfloat)model_vertex[i*4+3][2];
					point++;
				}
				/* lines spanning axis 3 */
				for (i=0;i<8;i++)
				{
					(*point)[0]=(GLfloat)model_vertex[i][0];
					(*point)[1]=(GLfloat)model_vertex[i][1];
					(*point)[2]=(GLfloat)model_vertex[i][2];
					point++;
				}
				/* put all graphics in at time 0 */
				struct Graphics_vertex_array *array = GT_object_get_vertex_set(
					graphics_object);
				GT_object_clear_primitives(graphics_object);
				fill_line_graphics_vertex_array(array,	24, points, 0, 0, 0);
				if (!GT_OBJECT_ADD(GT_polyline_vertex_buffers)(graphics_object, polyline))
				{
					display_message(ERROR_MESSAGE,
						"Interaction_volume_make_polyline_extents.  "
						"Could not add primitive");
					return_code=0;
					DESTROY(GT_polyline_vertex_buffers)(&polyline);
				}
				DEALLOCATE(points);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Interaction_volume_make_polyline_extents.  Not enough memory)");
				DEALLOCATE(points);
				return_code=0;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Interaction_volume_make_polyline_extents.  Invalid volume");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Interaction_volume_make_polyline_extents.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Interaction_volume_make_polyline_extents */

