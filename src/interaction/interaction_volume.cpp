/*******************************************************************************
FILE : interaction_volume.c

LAST MODIFIED : 26 November 2001

DESCRIPTION :
Structure representing volume of space and centre interacted on by input
devices, and used by scenes for picking graphics.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <math.h>
#include <stdio.h>
#include "general/debug.h"
#include "general/matrix_vector.h"
#include "interaction/interaction_volume.h"
#include "general/message.h"

/*
Module types
------------
*/

enum Interaction_volume_type
{
	INTERACTION_VOLUME_CENTRED_BOX, /* box of given size with centre point */
	INTERACTION_VOLUME_RAY_FRUSTUM, /* view frustum with central near->far ray */
	INTERACTION_VOLUME_UNKNOWN_TYPE
};

struct Interaction_volume
/*******************************************************************************
LAST MODIFIED : 3 April 2000

DESCRIPTION :
Structure representing volume of space and centre interacted on by input
devices, and used by scenes for picking graphics.
==============================================================================*/
{
	enum Interaction_volume_type type;
	/* the following are 4x4 transformation matrices get from model to world
		 = modelview, and world to normalised coordinates = projection_matrix. They
		 are calculated for a particular Interaction_volume_type */
	double inverse_transformation[16],modelview_matrix[16],projection_matrix[16],
		transformation_matrix[16];
	/* flags indicating if the above matrices are calculated */
	int inverse_transformation_calculated,inverse_transformation_index[4],
		modelview_matrix_calculated,projection_matrix_calculated,
		transformation_matrix_calculated;
	union
	{
		struct
		{
			double centre_x,centre_y,centre_z,size_x,size_y,size_z;
		} centred_box;
		struct
		{
			double centre_x,centre_y,modelview_matrix[16],projection_matrix[16],
				size_x,size_y,viewport_bottom,viewport_left,
				viewport_width,viewport_height;
		} ray_frustum;
	} data;
	int access_count;
}; /* struct Interaction_volume */

/*
Module functions
----------------
*/

static struct Interaction_volume *CREATE(Interaction_volume)(void)
/*******************************************************************************
LAST MODIFIED : 3 April 2000

DESCRIPTION :
Private create function for Interaction_volume. Allocates structure and sets all
defaults common to each Interaction_volume_type. Initialisation must be
completed by type-specific create function.
==============================================================================*/
{
	struct Interaction_volume *interaction_volume;

	ENTER(CREATE(Interaction_volume));
	if (ALLOCATE(interaction_volume,struct Interaction_volume,1))
	{
		interaction_volume->type=INTERACTION_VOLUME_UNKNOWN_TYPE;
		interaction_volume->inverse_transformation_calculated=0;
		interaction_volume->modelview_matrix_calculated=0;
		interaction_volume->projection_matrix_calculated=0;
		interaction_volume->transformation_matrix_calculated=0;
		interaction_volume->access_count=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CREATE(Interaction_volume).  Not enough memory");
	}
	LEAVE;

	return (interaction_volume);
} /* CREATE(Interaction_volume) */

int Interaction_volume_calculate_modelview_matrix(
	struct Interaction_volume *interaction_volume)
/*******************************************************************************
LAST MODIFIED : 3 April 2000

DESCRIPTION :
Returns a 4x4 <modelview matrix> for <interaction_volume>. Note that values
go across rows fastest, and the matrix is appropriate for premultiplying
model coordinates to give world coordinates, i.e.:
  {wx wy wz wh}T = {Modelview_matrix}{mx my mz mh}T
It must therefore be transposed to be used by OpenGL.
==============================================================================*/
{
	int i,return_code;

	ENTER(Interaction_volume_calculate_modelview_matrix);
	if (interaction_volume)
	{
		return_code=1;
		switch (interaction_volume->type)
		{
			case INTERACTION_VOLUME_CENTRED_BOX:
			{
				identity_matrix4(interaction_volume->modelview_matrix);
				interaction_volume->modelview_matrix_calculated=1;
			} break;
			case INTERACTION_VOLUME_RAY_FRUSTUM:
			{
				/* use the modelview_matrix for the view */
				for (i=0;i<16;i++)
				{
					interaction_volume->modelview_matrix[i]=
						interaction_volume->data.ray_frustum.modelview_matrix[i];
				}
				interaction_volume->modelview_matrix_calculated=1;
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"Interaction_volume_calculate_modelview_matrix.  "
					"Unknown Interaction_volume_type");
				return_code=0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Interaction_volume_calculate_modelview_matrix.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Interaction_volume_calculate_modelview_matrix */

int Interaction_volume_calculate_projection_matrix(
	struct Interaction_volume *interaction_volume)
/*******************************************************************************
LAST MODIFIED : 3 April 2000

DESCRIPTION :
Returns a 4x4 <projection matrix> for <interaction_volume>. Note that values
go across rows fastest, and the matrix is appropriate for premultiplying
world coordinates to give viewport coordinates, i.e.:
  {vx vy vz vh}T = {Projection_matrix}{wx wy wz wh}T
Note that the projected viewing volume ranges from -1 to +1 in each coordinate,
incl. -1 on near to +1 on far clipping plane.
It must therefore be transposed to be used by OpenGL.
==============================================================================*/
{
	double temp_matrix[16];
	int return_code;

	ENTER(Interaction_volume_calculate_projection_matrix);
	if (interaction_volume)
	{
		return_code=1;
		switch (interaction_volume->type)
		{
			case INTERACTION_VOLUME_CENTRED_BOX:
			{
				identity_matrix4(interaction_volume->projection_matrix);
				/* scale */
				interaction_volume->projection_matrix[0] =
					2.0 / interaction_volume->data.centred_box.size_x;
				interaction_volume->projection_matrix[5] =
					2.0 / interaction_volume->data.centred_box.size_y;
				interaction_volume->projection_matrix[10] =
					2.0 / interaction_volume->data.centred_box.size_z;
				/* re-centre */
				interaction_volume->projection_matrix[4] =
					-2.0*interaction_volume->data.centred_box.centre_x /
					interaction_volume->data.centred_box.size_x;
				interaction_volume->projection_matrix[8] =
					-2.0*interaction_volume->data.centred_box.centre_y /
					interaction_volume->data.centred_box.size_y;
				interaction_volume->projection_matrix[12] =
					-2.0*interaction_volume->data.centred_box.centre_z /
					interaction_volume->data.centred_box.size_z;
				interaction_volume->projection_matrix_calculated=1;
			} break;
			case INTERACTION_VOLUME_RAY_FRUSTUM:
			{
				identity_matrix4(temp_matrix);
				/* scale */
				temp_matrix[0] = interaction_volume->data.ray_frustum.viewport_width /
					interaction_volume->data.ray_frustum.size_x;
				temp_matrix[5] = interaction_volume->data.ray_frustum.viewport_height /
					interaction_volume->data.ray_frustum.size_y;
				/* re-centre */
				temp_matrix[3] =
					(2.0*(interaction_volume->data.ray_frustum.viewport_left -
						interaction_volume->data.ray_frustum.centre_x) +
						interaction_volume->data.ray_frustum.viewport_width) /
					interaction_volume->data.ray_frustum.size_x;
				temp_matrix[7] =
					(2.0*(interaction_volume->data.ray_frustum.viewport_bottom -
						interaction_volume->data.ray_frustum.centre_y) +
						interaction_volume->data.ray_frustum.viewport_height) /
					interaction_volume->data.ray_frustum.size_y;
				return_code = multiply_matrix(4,4,4,temp_matrix,
					interaction_volume->data.ray_frustum.projection_matrix,
					interaction_volume->projection_matrix);
				if (return_code)
				{
					interaction_volume->projection_matrix_calculated=1;
				}
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"Interaction_volume_calculate_projection_matrix.  "
					"Unknown Interaction_volume_type");
				return_code=0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Interaction_volume_calculate_projection_matrix.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Interaction_volume_calculate_projection_matrix */

int Interaction_volume_calculate_transformation_matrix(
	struct Interaction_volume *interaction_volume)
/*******************************************************************************
LAST MODIFIED : 27 April 2000

DESCRIPTION :
Calculates total transformation matrix = projection_matrix x modelview_matrix
for <interaction_volume>, used for transforming model coordinates into
normalised coordinates.
==============================================================================*/
{
	int return_code;

	ENTER(Interaction_volume_calculate_transformation_matrix);
	if (interaction_volume)
	{
		if ((interaction_volume->modelview_matrix_calculated ||
			Interaction_volume_calculate_modelview_matrix(interaction_volume))&&
			(interaction_volume->projection_matrix_calculated ||
				Interaction_volume_calculate_projection_matrix(interaction_volume))&&
			multiply_matrix(4,4,4,interaction_volume->projection_matrix,
				interaction_volume->modelview_matrix,
				interaction_volume->transformation_matrix))
		{
			interaction_volume->transformation_matrix_calculated=1;
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Interaction_volume_calculate_transformation_matrix.  Failed");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Interaction_volume_calculate_transformation_matrix.  "
			"Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Interaction_volume_calculate_transformation_matrix */

int Interaction_volume_calculate_inverse_transformation(
	struct Interaction_volume *interaction_volume)
/*******************************************************************************
LAST MODIFIED : 3 April 2000

DESCRIPTION :
Calculates total transformation matrix = projection_matrix x modelview_matrix
for <interaction_volume>, used for transforming model coordinates into
normalised coordinates.
==============================================================================*/
{
	double d;
	int return_code;

	ENTER(Interaction_volume_calculate_inverse_transformation);
	if (interaction_volume)
	{
		if ((interaction_volume->transformation_matrix_calculated ||
			Interaction_volume_calculate_transformation_matrix(interaction_volume))&&
			copy_matrix(4,4,interaction_volume->transformation_matrix,
				interaction_volume->inverse_transformation)&&
			LU_decompose(4,interaction_volume->inverse_transformation,
				interaction_volume->inverse_transformation_index,&d,/*singular_tolerance*/1.0e-12))
		{
			interaction_volume->inverse_transformation_calculated=1;
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Interaction_volume_calculate_inverse_transformation.  Failed");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Interaction_volume_calculate_inverse_transformation.  "
			"Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Interaction_volume_calculate_inverse_transformation */

/*
Global functions
----------------
*/

struct Interaction_volume *create_Interaction_volume_bounding_box(
	struct Interaction_volume *interaction_volume1,
	struct Interaction_volume *interaction_volume2)
/*******************************************************************************
LAST MODIFIED : 26 November 2001

DESCRIPTION :
Creates an Interaction_volume between the centres of <interaction_volume1> and
<interaction_volume2> that forms a bounding box suitable for rubber-banding.
Note that the two interaction volumes must presently be of the same type and
in the case of ray_frustum type, should have the same modelview matrix,
projection_matrix and viewport.
The returned Interaction_volume will be of the same type as those volumes input,
and will span between their centres.
==============================================================================*/
{
	double centre_x,centre_y,centre_z,size_x,size_y,size_z;
	int matching_projections;
	struct Interaction_volume *interaction_volume;

	ENTER(create_Interaction_volume_bounding_box);
	interaction_volume=(struct Interaction_volume *)NULL;
	if (interaction_volume1&&interaction_volume2)
	{
		if (interaction_volume1->type==interaction_volume2->type)
		{
			switch (interaction_volume1->type)
			{
				case INTERACTION_VOLUME_CENTRED_BOX:
				{
					centre_x=0.5*(interaction_volume1->data.centred_box.centre_x +
						interaction_volume2->data.centred_box.centre_x);
					centre_y=0.5*(interaction_volume1->data.centred_box.centre_y +
						interaction_volume2->data.centred_box.centre_y);
					centre_z=0.5*(interaction_volume1->data.centred_box.centre_z +
						interaction_volume2->data.centred_box.centre_z);
					size_x=fabs(interaction_volume2->data.centred_box.centre_x -
						interaction_volume1->data.centred_box.centre_x);
					size_y=fabs(interaction_volume2->data.centred_box.centre_y -
						interaction_volume1->data.centred_box.centre_y);
					size_z=fabs(interaction_volume2->data.centred_box.centre_z -
							interaction_volume1->data.centred_box.centre_z);
					/* handle case where pointer is back where it started */
					if (0.0 >= size_x)
					{
						size_x = 0.0001*interaction_volume1->data.centred_box.size_x;
					}
					if (0.0 >= size_y)
					{
						size_y = 0.0001*interaction_volume1->data.centred_box.size_y;
					}
					if (0.0 >= size_z)
					{
						size_z = 0.0001*interaction_volume1->data.centred_box.size_z;
					}
					interaction_volume=create_Interaction_volume_centred_box(
						centre_x,centre_y,centre_z,size_x,size_y,size_z);
				} break;
				case INTERACTION_VOLUME_RAY_FRUSTUM:
				{
					/* check projections match */
					matching_projections=1;
#if defined (DEBUG_CODE)
					{
						int i;
						/* SAB Removed this check as we just want to use the 
							 second one and spinning scene viewers cause this to
							 happen all the time */
						for (i=0;i<16;i++)
						{
							if ((interaction_volume1->data.ray_frustum.modelview_matrix[i] !=
								interaction_volume2->data.ray_frustum.modelview_matrix[i]) ||
								(interaction_volume1->data.ray_frustum.projection_matrix[i] !=
									interaction_volume2->data.ray_frustum.projection_matrix[i]))
							{
								matching_projections=0;
							}
						}
					}
#endif /* defined (DEBUG_CODE) */
					if ((interaction_volume1->data.ray_frustum.viewport_left !=
						interaction_volume2->data.ray_frustum.viewport_left)||
						(interaction_volume1->data.ray_frustum.viewport_width !=
							interaction_volume2->data.ray_frustum.viewport_width)||
						(interaction_volume1->data.ray_frustum.viewport_bottom !=
							interaction_volume2->data.ray_frustum.viewport_bottom)||
						(interaction_volume1->data.ray_frustum.viewport_height !=
							interaction_volume2->data.ray_frustum.viewport_height))
					{
						matching_projections=0;
					}
					if (matching_projections)
					{
						centre_x=0.5*(interaction_volume1->data.ray_frustum.centre_x +
							interaction_volume2->data.ray_frustum.centre_x);
						centre_y=0.5*(interaction_volume1->data.ray_frustum.centre_y +
							interaction_volume2->data.ray_frustum.centre_y);
						size_x=fabs(interaction_volume2->data.ray_frustum.centre_x -
							interaction_volume1->data.ray_frustum.centre_x);
						size_y=fabs(interaction_volume2->data.ray_frustum.centre_y -
							interaction_volume1->data.ray_frustum.centre_y);
						/* handle case where pointer is back where it started */
						if (0.0 >= size_x)
						{
							size_x = 0.0001*interaction_volume2->data.ray_frustum.size_x;
						}
						if (0.0 >= size_y)
						{
							size_y = 0.0001*interaction_volume2->data.ray_frustum.size_y;
						}
						interaction_volume=create_Interaction_volume_ray_frustum(
							interaction_volume2->data.ray_frustum.modelview_matrix,
							interaction_volume2->data.ray_frustum.projection_matrix,
							interaction_volume2->data.ray_frustum.viewport_left,
							interaction_volume2->data.ray_frustum.viewport_bottom,
							interaction_volume2->data.ray_frustum.viewport_width,
							interaction_volume2->data.ray_frustum.viewport_height,
							centre_x,centre_y,size_x,size_y);
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"create_Interaction_volume_bounding_box.  "
							"Ray_frustums have different projections");
					}
				} break;
				default:
				{
					display_message(ERROR_MESSAGE,
						"create_Interaction_volume_bounding_box.  "
						"Unknown Interaction_volume_type");
				} break;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,"create_Interaction_volume_bounding_box.  "
				"Interaction_volume_types do not match");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"create_Interaction_volume_bounding_box.  Invalid argument(s)");
	}
	LEAVE;

	return (interaction_volume);
} /* create_Interaction_volume_bounding_box */

struct Interaction_volume *create_Interaction_volume_centred_box(
	double centre_x,double centre_y,double centre_z,double size_x,double size_y,
	double size_z)
/*******************************************************************************
LAST MODIFIED : 3 April 2000

DESCRIPTION :
Creates an Interaction_volume at the given centre with the given size in each
direction.
==============================================================================*/
{
	struct Interaction_volume *interaction_volume;

	ENTER(create_Interaction_volume_centred_box);
	interaction_volume=(struct Interaction_volume *)NULL;
	if ((0.0 < size_x)&&(0.0 < size_y)&&(0.0 < size_z))
	{
		if (NULL != (interaction_volume=CREATE(Interaction_volume)()))
		{
			interaction_volume->type=INTERACTION_VOLUME_CENTRED_BOX;
			interaction_volume->data.centred_box.centre_x=centre_x;
			interaction_volume->data.centred_box.centre_y=centre_y;
			interaction_volume->data.centred_box.centre_z=centre_z;
			interaction_volume->data.centred_box.size_x=size_x;
			interaction_volume->data.centred_box.size_y=size_y;
			interaction_volume->data.centred_box.size_z=size_z;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"create_Interaction_volume_centred_box.  Not enough memory");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"create_Interaction_volume_centred_box.  Invalid box size");
	}
	LEAVE;

	return (interaction_volume);
} /* create_Interaction_volume_centred_box */

struct Interaction_volume *create_Interaction_volume_ray_frustum(
	double modelview_matrix[16],double projection_matrix[16],
	double viewport_left,double viewport_bottom,
	double viewport_width,double viewport_height,
	double centre_x,double centre_y,
	double size_x,double size_y)
/*******************************************************************************
LAST MODIFIED : 3 April 2000

DESCRIPTION :
Creates an Interaction_volume ray_frustum. The combined modelview and projection
matrices represent the basic view of scene viewer, while the viewport, centre
and size define what part of the 2d view seen by the viewer - once depth is
compressed - that interaction is taking place on.
==============================================================================*/
{
	int i;
	struct Interaction_volume *interaction_volume;

	ENTER(create_Interaction_volume_ray_frustum);
	interaction_volume=(struct Interaction_volume *)NULL;
	if ((0.0 < viewport_width)&&(0.0 < viewport_height)&&
		(0.0 < size_x)&&(0.0 < size_y))
	{
		if (NULL != (interaction_volume=CREATE(Interaction_volume)()))
		{
			interaction_volume->type=INTERACTION_VOLUME_RAY_FRUSTUM;
			for (i=0;i<16;i++)
			{
				interaction_volume->data.ray_frustum.modelview_matrix[i]=
					modelview_matrix[i];
				interaction_volume->data.ray_frustum.projection_matrix[i]=
					projection_matrix[i];
			}
			interaction_volume->data.ray_frustum.viewport_left=viewport_left;
			interaction_volume->data.ray_frustum.viewport_bottom=viewport_bottom;
			interaction_volume->data.ray_frustum.viewport_width=viewport_width;
			interaction_volume->data.ray_frustum.viewport_height=viewport_height;
			interaction_volume->data.ray_frustum.centre_x=centre_x;
			interaction_volume->data.ray_frustum.centre_y=centre_y;
			interaction_volume->data.ray_frustum.size_x=size_x;
			interaction_volume->data.ray_frustum.size_y=size_y;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"create_Interaction_volume_ray_frustum.  Not enough memory");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"create_Interaction_volume_ray_frustum.  Invalid viewport");
	}
	LEAVE;

	return (interaction_volume);
} /* create_Interaction_volume_ray_frustum */

int DESTROY(Interaction_volume)(
	struct Interaction_volume **interaction_volume_address)
/*******************************************************************************
LAST MODIFIED : 30 March 2000

DESCRIPTION :
Destroys the Interaction_volume.
==============================================================================*/
{
	int return_code;
	struct Interaction_volume *interaction_volume;

	ENTER(DESTROY(Interaction_volume));
	if (interaction_volume_address&&
		(interaction_volume= *interaction_volume_address))
	{
		if (0 >= interaction_volume->access_count)
		{
			DEALLOCATE(*interaction_volume_address);
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"DESTROY(Interaction_volume).  Non-zero access count!");
			*interaction_volume_address=(struct Interaction_volume *)NULL;
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(Interaction_volume).  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Interaction_volume) */

DECLARE_OBJECT_FUNCTIONS(Interaction_volume)

double Interaction_volume_get_closeness_from_normalised_z(
	struct Interaction_volume *interaction_volume,double normalised_z)
/*******************************************************************************
LAST MODIFIED : 30 March 2000

DESCRIPTION :
The projection_matrix maps part of real space onto normalised device coordinates
that vary from -1 to +1 in each direction x, y and z. These are left handed, so
that when projected onto an area of the screen:
x = -1 at the left of the viewport, +1 at the right;
y = -1 at the bottom of the viewport, +1 at the top;
z = -1 at the near clipping plane of the viewport, +1 at the far clipping plane.
This follows how OpenGL works.
The viewport transformation in OpenGL converts x and y into pixel coordinates in
the window, while z lies within the values given in glDepthRange(near,far).
Normally near=0.0 and far=1.0; these are the values used by Scene_viewers. When
OpenGL picking with the interaction_volume is performed, it will return depths
for each picked object that should be converted with:

  normalised_z=2.0*(depth-near_depth)/(far_depth-near_depth) - 1.0;

This function should then be called to convert <normalised_z> into a measure of
closeness appropriate to the <interaction_volume> type, which can be used to
work out the closest object to the pointer, where a lower value is closer.
==============================================================================*/
{
	double closeness;

	ENTER(Interaction_volume_get_closeness_from_depth);
	/* in case of error, closeness is well outside normalised coordinates */
	closeness=1000.0;
	if (interaction_volume)
	{
		switch (interaction_volume->type)
		{
			case INTERACTION_VOLUME_CENTRED_BOX:
			{
				closeness=fabs(normalised_z);
			} break;
			case INTERACTION_VOLUME_RAY_FRUSTUM:
			{
				closeness=normalised_z;
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"Interaction_volume_get_closeness_from_depth.  "
					"Unknown Interaction_volume_type");
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Interaction_volume_get_closeness_from_depth.  Invalid argument(s)");
	}
	LEAVE;

	return (closeness);
} /* Interaction_volume_get_closeness_from_depth */

int Interaction_volume_get_distance_to_point(
	struct Interaction_volume *interaction_volume,double *point,double *distance)
/*******************************************************************************
LAST MODIFIED : 30 March 2000

DESCRIPTION :
Calculates the distance in real coordinates from the "centre" of the
<interaction_volume> to the <point>. The centre to be used depends on
<interaction_volume> type as appropriate, esp. for ray_frustum, where closeness
to the viewer is paramount.
==============================================================================*/
{
	double normalised_centre[3],centre[3];
	int return_code;

	ENTER(Interaction_volume_get_distance_to_point);
	if (interaction_volume&&point&&distance)
	{
		return_code=1;
		switch (interaction_volume->type)
		{
			case INTERACTION_VOLUME_CENTRED_BOX:
			{
				/* use the actual centre of the box */
				centre[0]=interaction_volume->data.centred_box.centre_x;
				centre[1]=interaction_volume->data.centred_box.centre_y;
				centre[2]=interaction_volume->data.centred_box.centre_z;
			} break;
			case INTERACTION_VOLUME_RAY_FRUSTUM:
			{
				/* use the centre of the near plane */
				normalised_centre[0]=0.0;
				normalised_centre[1]=0.0;
				normalised_centre[2]=-1.0;
				return_code=Interaction_volume_normalised_to_model_coordinates(
					interaction_volume,normalised_centre,centre);
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"Interaction_volume_get_closeness_from_depth.  "
					"Unknown Interaction_volume_type");
				return_code=0;
			} break;
		}
		if (return_code)
		{
			centre[0] -= point[0];
			centre[1] -= point[1];
			centre[2] -= point[2];
			*distance=
				sqrt(centre[0]*centre[0] + centre[1]*centre[1] + centre[2]*centre[2]);
		}
		else
		{
			*distance=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Interaction_volume_get_distance_to_point.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Interaction_volume_get_distance_to_point */

int Interaction_volume_get_modelview_matrix(
	struct Interaction_volume *interaction_volume,double *modelview_matrix)
/*******************************************************************************
LAST MODIFIED : 3 April 2000

DESCRIPTION :
Returns a 4x4 <modelview matrix> for <interaction_volume>. Note that values
go across rows fastest, and the matrix is appropriate for premultiplying
model coordinates to give world coordinates, i.e.:
	<wx wy wz wh>T = <Modelview_matrix><mx my mz mh>T
It must therefore be transposed to be used by OpenGL.
==============================================================================*/
{
	int i,return_code;

	ENTER(Interaction_volume_get_modelview_matrix);
	if (interaction_volume&&modelview_matrix)
	{
		if (interaction_volume->modelview_matrix_calculated ||
			Interaction_volume_calculate_modelview_matrix(interaction_volume))
		{
			for (i=0;i<16;i++)
			{
				modelview_matrix[i]=interaction_volume->modelview_matrix[i];
			}
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Interaction_volume_get_modelview_matrix.  Could not calculate matrix");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Interaction_volume_get_modelview_matrix.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Interaction_volume_get_modelview_matrix */

int Interaction_volume_get_placement_point(
	struct Interaction_volume *interaction_volume,double *point,
	Interation_volume_constraint_function constraint_function,
	void *constraint_function_data)
/*******************************************************************************
LAST MODIFIED : 21 March 2005

DESCRIPTION :
Returns the point in the <interaction_volume> most appropriate for placing
an object. This is normally the centre, but in the case of ray_frustum it is
arbitratily chosen to be the centre of the view frustum on the ray.
In future this function may be just one of many for enquiring about the centre
abilities of the Interaction_volume, which may enable such features as placement
at the intersection of a ray and a surface/manifold in space.
If a <constraint_function> is supplied then a location will be passed to this
function, which is expected to adjust it to satisfy the constraints that it
represents.  This function will then adjust the location and call the
<constraint_function> iteratively until either the two locations converge or
it gives up.
==============================================================================*/
{
	double constraint_change[3], iteration_change[3], normalised_point[3], 
		previous_point[3];
	FE_value fe_value_point[3];
	int converged, return_code, steps;

	ENTER(Interaction_volume_get_placement_point);
	if (interaction_volume&&point)
	{
		return_code=1;
		switch (interaction_volume->type)
		{
			case INTERACTION_VOLUME_CENTRED_BOX:
			{
				point[0]=interaction_volume->data.centred_box.centre_x;
				point[1]=interaction_volume->data.centred_box.centre_y;
				point[2]=interaction_volume->data.centred_box.centre_z;
			} break;
			case INTERACTION_VOLUME_RAY_FRUSTUM:
			{
				/* return point in centre of view frustum on the ray */
				normalised_point[0]=0.0;
				normalised_point[1]=0.0;
				normalised_point[2]=0.0;
				return_code=Interaction_volume_normalised_to_model_coordinates(
					interaction_volume,normalised_point,point);
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"Interaction_volume_get_placement_point.  "
					"Unknown Interaction_volume_type");
				return_code=0;
			} break;
		}
		if (constraint_function)
		{
			converged = 0;
			steps = 0;
			while (return_code && !converged)
			{
				previous_point[0] = point[0];
				previous_point[1] = point[1];
				previous_point[2] = point[2];

 				fe_value_point[0] = point[0];
 				fe_value_point[1] = point[1];
 				fe_value_point[2] = point[2];
				(*constraint_function)(fe_value_point, constraint_function_data);

				constraint_change[0] = point[0] - fe_value_point[0];
				constraint_change[1] = point[1] - fe_value_point[1];
				constraint_change[2] = point[2] - fe_value_point[2];

				/* The constraint function works with FE_values so we need a 
					relatively large convergence tolerance. */
				if (norm3(constraint_change) < 1.0e-4)
				{
					converged = 1;
				}
				else
				{
					point[0] = fe_value_point[0];
					point[1] = fe_value_point[1];
					point[2] = fe_value_point[2];
					steps++;
					if (steps > 10000)
					{
						return_code = 0;
					}
					/* Move the current location back to the centre of the ray */
					Interaction_volume_model_to_normalised_coordinates(interaction_volume,
						point, normalised_point);
					/* Set to the center and convert back to model coordinates */
					Interaction_volume_centred_normalised_to_model_coordinates(
						interaction_volume, normalised_point, point);

					iteration_change[0] = point[0] - previous_point[0];
					iteration_change[1] = point[1] - previous_point[1];
					iteration_change[2] = point[2] - previous_point[2];

					/* A tighter check, see if we aren't improving at all, then bail */
					if (norm3(iteration_change) < 1.0e-6)
					{
						return_code = 0;
					}
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Interaction_volume_get_placement_point.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Interaction_volume_get_placement_point */

int Interaction_volume_get_projection_matrix(
	struct Interaction_volume *interaction_volume,double *projection_matrix)
/*******************************************************************************
LAST MODIFIED : 3 April 2000

DESCRIPTION :
Returns a 4x4 <projection matrix> for <interaction_volume>. Note that values
go across rows fastest, and the matrix is appropriate for premultiplying
world coordinates to give normalised coordinates, i.e.:
	<wx wy wz wh>T = <Projection_matrix><mx my mz mh>T
It must therefore be transposed to be used by OpenGL.

The projection_matrix maps part of real space onto normalised device coordinates
that vary from -1 to +1 in each direction x, y and z. These are left handed, so
that when projected onto an area of the screen:
x = -1 at the left of the viewport, +1 at the right;
y = -1 at the bottom of the viewport, +1 at the top;
z = -1 at the near clipping plane of the viewport, +1 at the far clipping plane.
This follows how OpenGL works. The normalised device coordinates returned by
this function refer to the space of the interaction_volume itself. This is not
to be confused with the volume of space refered to by the projection_matrix
parameter for ray_frustum type which is yet to undergo a viewport/picking
transformation to give the matrix returned here.
==============================================================================*/
{
	int i,return_code;

	ENTER(Interaction_volume_get_projection_matrix);
	if (interaction_volume&&projection_matrix)
	{
		if (interaction_volume->projection_matrix_calculated ||
			Interaction_volume_calculate_projection_matrix(interaction_volume))
		{
			for (i=0;i<16;i++)
			{
				projection_matrix[i]=interaction_volume->projection_matrix[i];
			}
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Interaction_volume_get_projection_matrix.  "
				"Could not calculate matrix");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Interaction_volume_get_projection_matrix.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Interaction_volume_get_projection_matrix */

int Interaction_volume_model_to_normalised_coordinates(
	struct Interaction_volume *interaction_volume,double *model_coordinates,
	double *normalised_coordinates)
/*******************************************************************************
LAST MODIFIED : 6 April 2000

DESCRIPTION :
Transforms <model_coordinates> to <normalised_coordinates> for the
<interaction_volume>, ranging from -1 to +1 in each direction.
==============================================================================*/
{
	double vector1[4],vector2[4];
	int return_code;

	ENTER(Interaction_volume_model_to_normalised_coordinates);
	if (interaction_volume&&model_coordinates&&normalised_coordinates)
	{
		vector1[0]=model_coordinates[0];
		vector1[1]=model_coordinates[1];
		vector1[2]=model_coordinates[2];
		vector1[3]=1.0;
		if ((interaction_volume->transformation_matrix_calculated ||
			Interaction_volume_calculate_transformation_matrix(interaction_volume))&&
			multiply_matrix(4,4,1,interaction_volume->transformation_matrix,
				vector1,vector2)&&(0.0 != vector2[3]))
		{
			normalised_coordinates[0]=vector2[0]/vector2[3];
			normalised_coordinates[1]=vector2[1]/vector2[3];
			normalised_coordinates[2]=vector2[2]/vector2[3];
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Interaction_volume_model_to_normalised_coordinates.  Failed");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Interaction_volume_model_to_normalised_coordinates.  "
			"Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Interaction_volume_model_to_normalised_coordinates */

int Interaction_volume_normalised_to_model_coordinates(
	struct Interaction_volume *interaction_volume,double *normalised_coordinates,
	double *model_coordinates)
/*******************************************************************************
LAST MODIFIED : 3 April 2000

DESCRIPTION :
Transforms <normalised_coordinates> to <model_coordinates> for the
<interaction_volume>, ranging from -1 to +1 in each direction.
==============================================================================*/
{
	double vector[4];
	int return_code;

	ENTER(Interaction_volume_normalised_to_model_coordinates);
	if (interaction_volume&&normalised_coordinates&&model_coordinates)
	{
		vector[0]=normalised_coordinates[0];
		vector[1]=normalised_coordinates[1];
		vector[2]=normalised_coordinates[2];
		vector[3]=1.0;
		if ((interaction_volume->inverse_transformation_calculated ||
			Interaction_volume_calculate_inverse_transformation(interaction_volume))&&
			LU_backsubstitute(4,interaction_volume->inverse_transformation,
				interaction_volume->inverse_transformation_index,vector)&&
			(0.0 != vector[3]))
		{
			model_coordinates[0]=vector[0]/vector[3];
			model_coordinates[1]=vector[1]/vector[3];
			model_coordinates[2]=vector[2]/vector[3];
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Interaction_volume_normalised_to_model_coordinates.  Failed");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Interaction_volume_normalised_to_model_coordinates.  "
			"Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Interaction_volume_normalised_to_model_coordinates */

int Interaction_volume_centred_normalised_to_model_coordinates(
	struct Interaction_volume *interaction_volume,double *normalised_coordinates,
	double *model_coordinates)
/*******************************************************************************
LAST MODIFIED : 3 April 2000

DESCRIPTION :
Similar to Interaction_volume_normalised_to_model_coordinates, but returns a
position modified to be on the centre indicated by the <interaction_volume>
type.
==============================================================================*/
{
	double centred_normalised_coordinates[3];
	int return_code;

	ENTER(Interaction_volume_centred_normalised_to_model_coordinates);
	if (interaction_volume&&normalised_coordinates&&model_coordinates)
	{
		return_code=1;
		switch (interaction_volume->type)
		{
			case INTERACTION_VOLUME_CENTRED_BOX:
			{
				/* return the centre of the box */
				model_coordinates[0]=interaction_volume->data.centred_box.centre_x;
				model_coordinates[1]=interaction_volume->data.centred_box.centre_y;
				model_coordinates[2]=interaction_volume->data.centred_box.centre_z;
			} break;
			case INTERACTION_VOLUME_RAY_FRUSTUM:
			{
				/* move normalised x and y to centre of frustum */
				centred_normalised_coordinates[0]=0.0;
				centred_normalised_coordinates[1]=0.0;
				centred_normalised_coordinates[2]=normalised_coordinates[2];
				return_code=Interaction_volume_normalised_to_model_coordinates(
					interaction_volume,centred_normalised_coordinates,model_coordinates);
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"Interaction_volume_centred_normalised_to_model_coordinates.  "
					"Unknown Interaction_volume_type");
				return_code=0;
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Interaction_volume_centred_normalised_to_model_coordinates.  "
			"Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Interaction_volume_centred_normalised_to_model_coordinates */
