/*******************************************************************************
FILE : interaction_volume.h

LAST MODIFIED : 3 April 2000

DESCRIPTION :
Structure representing volume of space and centre interacted on by input
devices, and used by scenes for picking graphics.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (INTERACTION_VOLUME_H)
#define INTERACTION_VOLUME_H

#include "general/object.h"
#include "general/value.h"

struct Interaction_volume;
/*******************************************************************************
LAST MODIFIED : 30 March 2000

DESCRIPTION :
Structure representing volume of space and centre interacted on by input
devices, and used by scenes for picking graphics.
The contents of this object are private.
==============================================================================*/

typedef int (*Interation_volume_constraint_function)(FE_value *point,
	void *void_data);

/*
Global functions
----------------
*/

struct Interaction_volume *create_Interaction_volume_centred_box(
	double centre_x,double centre_y,double centre_z,double size_x,double size_y,
	double size_z);
/*******************************************************************************
LAST MODIFIED : 30 March 2000

DESCRIPTION :
Creates an Interaction_volume at the given centre with the given size in each
direction.
==============================================================================*/

struct Interaction_volume *create_Interaction_volume_ray_frustum(
	double modelview_matrix[16],double projection_matrix[16],
	double viewport_left,double viewport_bottom,
	double viewport_width,double viewport_height,
	double centre_x,double centre_y,
	double size_x,double size_y);
/*******************************************************************************
LAST MODIFIED : 3 April 2000

DESCRIPTION :
Creates an Interaction_volume ray_frustum. The combined modelview and projection
matrices represent the basic view of scene viewer, while the viewport, centre
and size define what part of the 2d view seen by the viewer - once depth is
compressed - that interaction is taking place on.
==============================================================================*/

struct Interaction_volume *create_Interaction_volume_bounding_box(
	struct Interaction_volume *interaction_volume1,
	struct Interaction_volume *interaction_volume2);
/*******************************************************************************
LAST MODIFIED : 30 March 2000

DESCRIPTION :
Creates an Interaction_volume between the centres of <interaction_volume1> and
<interaction_volume2> that forms a bounding box suitable for rubber-banding.
Note that the two interaction volumes must presently be of the same type and
in the case of ray_frustum type, should have the same modelview matrix,
projection_matrix and viewport.
The returned Interaction_volume will be of the same type as those volumes input,
and will span between their centres.
==============================================================================*/

int DESTROY(Interaction_volume)(
	struct Interaction_volume **interaction_volume_address);
/*******************************************************************************
LAST MODIFIED : 30 March 2000

DESCRIPTION :
Destroys the Interaction_volume.
==============================================================================*/

PROTOTYPE_OBJECT_FUNCTIONS(Interaction_volume);

double Interaction_volume_get_closeness_from_normalised_z(
	struct Interaction_volume *interaction_volume,double normalised_z);
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

int Interaction_volume_get_distance_to_point(
	struct Interaction_volume *interaction_volume,double *point,double *distance);
/*******************************************************************************
LAST MODIFIED : 30 March 2000

DESCRIPTION :
Calculates the distance in real coordinates from the "centre" of the
<interaction_volume> to the <point>. The centre to be used depends on
<interaction_volume> type as appropriate, esp. for ray_frustum, where closeness
to the viewer is paramount.
==============================================================================*/

int Interaction_volume_get_modelview_matrix(
	struct Interaction_volume *interaction_volume,double *modelview_matrix);
/*******************************************************************************
LAST MODIFIED : 3 April 2000

DESCRIPTION :
Returns a 4x4 <modelview matrix> for <interaction_volume>. Note that values
go across rows fastest, and the matrix is appropriate for premultiplying
model coordinates to give world coordinates, i.e.:
	<wx wy wz wh>T = <Modelview_matrix><mx my mz mh>T
It must therefore be transposed to be used by OpenGL.
==============================================================================*/

int Interaction_volume_get_placement_point(
	struct Interaction_volume *interaction_volume,double *point,
	Interation_volume_constraint_function constraint_function,
	void *constraint_function_data);
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

int Interaction_volume_get_projection_matrix(
	struct Interaction_volume *interaction_volume,double *projection_matrix);
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

int Interaction_volume_model_to_normalised_coordinates(
	struct Interaction_volume *interaction_volume,double *model_coordinates,
	double *normalised_coordinates);
/*******************************************************************************
LAST MODIFIED : 3 April 2000

DESCRIPTION :
Transforms <model_coordinates> to <normalised_coordinates> for the
<interaction_volume>, ranging from -1 to +1 in each direction.
==============================================================================*/

int Interaction_volume_normalised_to_model_coordinates(
	struct Interaction_volume *interaction_volume,double *normalised_coordinates,
	double *model_coordinates);
/*******************************************************************************
LAST MODIFIED : 3 April 2000

DESCRIPTION :
Transforms <normalised_coordinates> to <model_coordinates> for the
<interaction_volume>, ranging from -1 to +1 in each direction.
==============================================================================*/

int Interaction_volume_centred_normalised_to_model_coordinates(
	struct Interaction_volume *interaction_volume,double *normalised_coordinates,
	double *model_coordinates);
/*******************************************************************************
LAST MODIFIED : 3 April 2000

DESCRIPTION :
Similar to Interaction_volume_normalised_to_model_coordinates, but returns a
position modified to be on the centre indicated by the <interaction_volume>
type.
==============================================================================*/
#endif /* !defined (INTERACTION_VOLUME_H) */
