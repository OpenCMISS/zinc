/*******************************************************************************
FILE : photogrammetry.h

LAST MODIFIED : 15 April 1998

DESCRIPTION :
Routines for performing photogrammetry calculations.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (PHOTOGRAMMETRY_H)
#define PHOTOGRAMMETRY_H

/*
Global functions
----------------
*/
int point_3d_to_2d_view(double *t,double *pos3,double *pos2);
/*******************************************************************************
LAST MODIFIED : 25 February 1998

DESCRIPTION :
Returns the 2-D position of point <pos3> by transformation with matrix <t>.
==============================================================================*/

int point_pair_to_3d(double *p1,double *T1,double *p2,double *T2,double *b);
/*******************************************************************************
LAST MODIFIED : 28 January 1998

DESCRIPTION :
Performs the photogrammetry calculations for converting a pair of points on
two 2-D images with known transformation matrices into a 3-D position. p1 is an
x,y position on image 1, with the transformation from 3-D to the 2-D coordinates
in the image is achieved using the 4x3 matrix t1 as follows:
	[x y z 1].t1 = [p1x' p1y' h1],
where the true location on the 2-D image is found by the perspective division
from the homogeneous coordinates:
	p1x = p1x'/h1
	p1y = p1y'/h1.
Matching relationships can be developed for the the second image with p2, t2.
The following code uses the theory from F.I.Parke, K.Waters "Computer Facial
Animation", A.K.Peters Ltd. publ. pages 74-76 for determining the x,y,z position
given p1, t1, p2 and t2. The 3-D location of the point is returned in the
vector <b>.
???RC.  Could be generalised for N>1 images.
==============================================================================*/

int weighted_point_pair_to_3d(double *p1,double *t1,double w1,
	double *p2,double *t2,double w2,double *b);
/*******************************************************************************
LAST MODIFIED : 9 March 1998

DESCRIPTION :
Version of point_pair_to_3d allowing weighting on each pair of points to be
varied to reflect the confidence we have in the values.
For example, having the weighting very much higher on one point will mean its
equations will be very closely satisfied, while the others will be satisfied
as nearly as possible within the remaining degree of freedom.
==============================================================================*/

int photogrammetry_to_graphics_projection(double *t,double near,double far,
	double NDC_left,double NDC_bottom,double NDC_width,double NDC_height,
	double *modelview_matrix,double *projection_matrix,double *eye,
	double *lookat,double *up);
/*******************************************************************************
LAST MODIFIED : 14 February 1998

DESCRIPTION :
Extracts 4x4 modelview and projection matrices suitable for sending to a
Scene_viewer from the 4x3 photogrammetry projection matrix, t, combined with
<near> and <far> clipping plane information, and the origin <NDC_left>,
<NDC_bottom> and size <NDC_widthxNDC_height> of the region projected on to.
Also returns lookat parameters <eye> position, <lookat> point and <up> vector.
Requires for now that NDC_bottom and NDC_left are zero.
==============================================================================*/

int photogrammetry_project(double *t,double near,double far,
	double NDC_left,double NDC_bottom,double NDC_width,double NDC_height,
	double *pos3,double *win_pos);
/*******************************************************************************
LAST MODIFIED : 15 April 1998

DESCRIPTION :
Converts 3-D coordinate pos3 into its projection, win_pos on the window.
Note that the z-coordinate returned is 0.0 on the near plane and 1.0 on the far
plane. The x and y coordinates returned should match those returned by
point_3d_to_2d_view.
==============================================================================*/

int photogrammetry_unproject(double *t,double near,double far,
	double NDC_left,double NDC_bottom,double NDC_width,double NDC_height,
	double *pos2,double *near_pos,double *far_pos);
/*******************************************************************************
LAST MODIFIED : 9 March 1998

DESCRIPTION :
Converts 2-D coordinate pos2 into two 3-D locations on the near and far place
of the viewing volume.
==============================================================================*/
#endif /* !defined (PHOTOGRAMMETRY_H) */
