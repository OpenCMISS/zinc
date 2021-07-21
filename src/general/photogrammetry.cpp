/*******************************************************************************
FILE : photogrammetry.c

LAST MODIFIED : 26 November 2001

DESCRIPTION :
Routines for performing photogrammetry calculations.

Note that all matrices and vectors used here will be stored or expected in the
format specified in general/matrix_vector.h.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stdio.h>

#include "general/debug.h"
#include "graphics/graphics_library.h"
#include "general/matrix_vector.h"
#include "general/photogrammetry.h"
#include "general/message.h"

/*
Module functions
----------------
*/

/*
Global functions
----------------
*/
int point_3d_to_2d_view(double *t,double *pos3,double *pos2)
/*******************************************************************************
LAST MODIFIED : 25 February 1998

DESCRIPTION :
Returns the 2-D position of point <pos3> by transformation with matrix <t>.
==============================================================================*/
{
	double p[4],r[3];
	int return_code;

	ENTER(point_3d_to_2d_view);
	if (t&&pos3&&pos2)
	{
		/* create homogeneous coordinate */
		p[0]=pos3[0];
		p[1]=pos3[1];
		p[2]=pos3[2];
		p[3]=1.0;
		return_code = multiply_matrix(1,4,3,p,t,r);
		if (return_code)
		{
			if (0.0 != r[2])
			{
				pos2[0]=r[0]/r[2];
				pos2[1]=r[1]/r[2];
			}
			else
			{
				return_code=0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"point_3d_to_2d_view.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* point_3d_to_2d_view */

int point_pair_to_3d(double *p1,double *t1,double *p2,double *t2,double *b)
/*******************************************************************************
LAST MODIFIED : 26 November 2001

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
{
	double a1[12]/*4x3*/,a1t[12]/*3x4*/,a[9]/*3x3*/,b1[4]/*4x1*/,d;
	int return_code, indx[3];

	ENTER(point_pair_to_3d);
	if (p1&&t1&&p2&&t2&&b)
	{
#if defined (DEBUG_CODE)
		printf("\n>>> ENTER: point_pair_to_3d <<<\n");
		printf("p1:\n");
		print_matrix(2,1,p1," %12.7f");
		printf("t1:\n");
		print_matrix(4,3,t1," %12.7f");
		printf("p2:\n");
		print_matrix(2,1,p2," %12.7f");
		printf("t2:\n");
		print_matrix(4,3,t2," %12.7f");
#endif /* defined (DEBUG_CODE) */

		a1[ 0] = t1[0]-p1[0]*t1[2];
		a1[ 1] = t1[3]-p1[0]*t1[5];
		a1[ 2] = t1[6]-p1[0]*t1[8];

		a1[ 3] = t1[1]-p1[1]*t1[2];
		a1[ 4] = t1[4]-p1[1]*t1[5];
		a1[ 5] = t1[7]-p1[1]*t1[8];

		a1[ 6] = t2[0]-p2[0]*t2[2];
		a1[ 7] = t2[3]-p2[0]*t2[5];
		a1[ 8] = t2[6]-p2[0]*t2[8];

		a1[ 9] = t2[1]-p2[1]*t2[2];
		a1[10] = t2[4]-p2[1]*t2[5];
		a1[11] = t2[7]-p2[1]*t2[8];

#if defined (DEBUG_CODE)
		printf("a1:\n");
		print_matrix(4,3,a1," %12.7f");
#endif /* defined (DEBUG_CODE) */

		/* fill vector b1 */
		b1[0] = p1[0]*t1[11]-t1[9];
		b1[1] = p1[1]*t1[11]-t1[10];

		b1[2] = p2[0]*t2[11]-t2[9];
		b1[3] = p2[1]*t2[11]-t2[10];
		/* printf("b1:\n"); print_matrix(4,1,b1," %12.7f"); */

#if defined (DEBUG_CODE)
		/* a1(t)a1 . x = a1(t) b1 */
		transpose_matrix(4,3,a1,a1t);
		printf("a1t:\n");
		print_matrix(3,4,a1t," %12.7f");

		multiply_matrix(3,4,3,a1t,a1,a);
		printf("a:\n");
		print_matrix(3,3,a," %12.7f");

		multiply_matrix(3,4,1,a1t,b1,b);
		printf("b:\n");
		print_matrix(3,1,b," %12.7f");
		/* solve a.x = b */
		LU_decompose(3,a,indx,&d,1.0e-12);
		LU_backsubstitute(3,a,indx,b);
		printf("Solution:\n");
		print_matrix(3,1,b," %12.7f");
		return_code=1;
#endif /* defined (DEBUG_CODE) */

		return_code=
			/* a1(t)a1 . x = a1(t) b1 */
			(transpose_matrix(4,3,a1,a1t)&&
			multiply_matrix(3,4,3,a1t,a1,a)&&
			multiply_matrix(3,4,1,a1t,b1,b)&&
			/* solve a.x = b */
			LU_decompose(3,a,indx,&d,/*singular_tolerance*/1.0e-12)&&
			LU_backsubstitute(3,a,indx,b));
	}
	else
	{
		display_message(ERROR_MESSAGE,"point_pair_to_3d.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* point_pair_to_3d */

int weighted_point_pair_to_3d(double *p1,double *t1,double w1,
	double *p2,double *t2,double w2,double *b)
/*******************************************************************************
LAST MODIFIED : 26 November 2001

DESCRIPTION :
Version of point_pair_to_3d allowing weighting on each pair of points to be
varied to reflect the confidence we have in the values.
For example, having the weighting very much higher on one point will mean its
equations will be very closely satisfied, while the others will be satisfied
as nearly as possible within the remaining degree of freedom.
???RC.  Have this as the default version?
==============================================================================*/
{
	double a1[12]/*4x3*/,a1t[12]/*3x4*/,a[9]/*3x3*/,b1[4]/*4x1*/,d;
	int return_code, indx[3];

	ENTER(weighted_point_pair_to_3d);
	if (p1&&t1&&(0<w1)&&p2&&t2&&(0<w2)&&b)
	{
#if defined (DEBUG_CODE)
		printf("\n>>> ENTER: point_pair_to_3d <<<\n");
		printf("p1:\n");
		print_matrix(2,1,p1," %12.7f");
		printf("t1:\n");
		print_matrix(4,3,t1," %12.7f");
		printf("p2:\n");
		print_matrix(2,1,p2," %12.7f");
		printf("t2:\n");
		print_matrix(4,3,t2," %12.7f");
#endif /* defined (DEBUG_CODE) */

		a1[ 0] = w1*(t1[0]-p1[0]*t1[2]);
		a1[ 1] = w1*(t1[3]-p1[0]*t1[5]);
		a1[ 2] = w1*(t1[6]-p1[0]*t1[8]);

		a1[ 3] = w1*(t1[1]-p1[1]*t1[2]);
		a1[ 4] = w1*(t1[4]-p1[1]*t1[5]);
		a1[ 5] = w1*(t1[7]-p1[1]*t1[8]);

		a1[ 6] = w2*(t2[0]-p2[0]*t2[2]);
		a1[ 7] = w2*(t2[3]-p2[0]*t2[5]);
		a1[ 8] = w2*(t2[6]-p2[0]*t2[8]);

		a1[ 9] = w2*(t2[1]-p2[1]*t2[2]);
		a1[10] = w2*(t2[4]-p2[1]*t2[5]);
		a1[11] = w2*(t2[7]-p2[1]*t2[8]);

#if defined (DEBUG_CODE)
		printf("a1:\n");
		print_matrix(4,3,a1," %12.7f");
#endif /* defined (DEBUG_CODE) */

		/* fill vector b1 */
		b1[0] = w1*(p1[0]*t1[11]-t1[9]);
		b1[1] = w1*(p1[1]*t1[11]-t1[10]);

		b1[2] = w2*(p2[0]*t2[11]-t2[9]);
		b1[3] = w2*(p2[1]*t2[11]-t2[10]);

#if defined (DEBUG_CODE)
		printf("b1:\n");
		print_matrix(4,1,b1," %12.7f");
#endif /* defined (DEBUG_CODE) */

		return_code=
			/* a1(t)a1 . x = a1(t) b1 */
			(transpose_matrix(4,3,a1,a1t)&&
			multiply_matrix(3,4,3,a1t,a1,a)&&
			multiply_matrix(3,4,1,a1t,b1,b)&&
			/* solve a.x = b */
			LU_decompose(3,a,indx,&d,/*singular_tolerance*/1.0e-12)&&
			LU_backsubstitute(3,a,indx,b));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"weighted_point_pair_to_3d.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* weighted_point_pair_to_3d */

int photogrammetry_to_graphics_projection(double *t,double near_plane,double far_plane,
	double NDC_left,double NDC_bottom,double NDC_width,double NDC_height,
	double *modelview_matrix,double *projection_matrix,double *eye,
	double *lookat,double *up)
/*******************************************************************************
LAST MODIFIED : 26 November 2001

DESCRIPTION :
Extracts 4x4 modelview and projection matrices suitable for sending to a
Scene_viewer from the 4x3 photogrammetry projection matrix, t, combined with
<near_plane> and <far_plane> clipping plane information, and the origin <NDC_left>,
<NDC_bottom> and size <NDC_widthxNDC_height> of the region projected on to.
Also returns lookat parameters <eye> position, <lookat> point and <up> vector.
Requires for now that NDC_bottom and NDC_left are zero.
==============================================================================*/
{
	double eye_distance, xnorm, ynorm, znorm, S14, S24, S33, S34, S44,
		eye_matrix[9], d;
	int return_code, tmpindx[3];

	ENTER(photogrammetry_to_graphics_projection);
	if (t&&(0<near_plane)&&(near_plane<far_plane)&&(0!=NDC_width)&&(0!=NDC_height)&&
		projection_matrix&&modelview_matrix&&eye&&lookat&&up)
	{
#if defined (DEBUG_CODE)
		{
			double tmp3[9], tmpb[3], tmpd;
			/* quick test of matrix solver: It worked! */
			/* Hand calculations give x=7/39,y=27/39,z=4/39 */
			tmp3[0]=5;
			tmp3[1]=2;
			tmp3[2]=7;
			tmp3[3]=1;
			tmp3[4]=0;
			tmp3[5]=8;
			tmp3[6]=4;
			tmp3[7]=3;
			tmp3[8]=2;
			tmpb[0]=3;
			tmpb[1]=1;
			tmpb[2]=3;
			printf("Matrix:\n");
			print_matrix(3,3,tmp3," %8.5f");
			printf("RHS:\n");
			print_matrix(3,1,tmpb," %12.7f");
			LU_decompose(3,tmp3,tmpindx,&tmpd,1.0e-12);
			LU_backsubstitute(3,tmp3,tmpindx,tmpb);
			printf("Solution:\n");
			print_matrix(3,1,tmpb," %12.7f");
			printf("\n");
		}
#endif /* defined (DEBUG_CODE) */

		/* the eye position is the point where x', y' and h are all zero */
		eye_matrix[0]=t[0];
		eye_matrix[1]=t[3];
		eye_matrix[2]=t[6];
		eye_matrix[3]=t[1];
		eye_matrix[4]=t[4];
		eye_matrix[5]=t[7];
		eye_matrix[6]=t[2];
		eye_matrix[7]=t[5];
		eye_matrix[8]=t[8];
		LU_decompose(3,eye_matrix,tmpindx,&d,/*singular_tolerance*/1.0e-12);
		eye[0]=-t[9];
		eye[1]=-t[10];
		eye[2]=-t[11];
		LU_backsubstitute(3,eye_matrix,tmpindx,eye);

#if defined (DEBUG_CODE)
		{
			double p1[2], p2[2];
			/* can also find eye position using photogrammetry calculations with 2
				 different points on the same image - not as efficient, though */
			p1[0]=NDC_left;
			p1[1]=NDC_bottom;
			p2[0]=NDC_left+NDC_width;
			p2[1]=NDC_bottom+NDC_height;
			point_pair_to_3d(p1,t,p2,t,eye);

			printf("Eye position:\n");
			print_matrix(1,3,eye," %14.7f");
		}
#endif /* defined (DEBUG_CODE) */

		/* form modelview matrix from unit vector axes in t */
		modelview_matrix[ 0]= t[0];
		modelview_matrix[ 1]= t[3];
		modelview_matrix[ 2]= t[6];
		xnorm=normalize3(&modelview_matrix[0]);
		modelview_matrix[ 4]= t[1];
		modelview_matrix[ 5]= t[4];
		modelview_matrix[ 6]= t[7];
		ynorm=normalize3(&modelview_matrix[4]);
		modelview_matrix[ 8]=-t[2];
		modelview_matrix[ 9]=-t[5];
		modelview_matrix[10]=-t[8];
		znorm=normalize3(&modelview_matrix[8]);

		/* incorporate eye position as origin of axes in modelview matrix */
		modelview_matrix[ 3]=-dot_product3(&modelview_matrix[0],eye);
		modelview_matrix[ 7]=-dot_product3(&modelview_matrix[4],eye);
		modelview_matrix[11]=-dot_product3(&modelview_matrix[8],eye);
		modelview_matrix[12]=0.0;
		modelview_matrix[13]=0.0;
		modelview_matrix[14]=0.0;
		modelview_matrix[15]=1.0;

		/* get lookat point halfway between near_plane and far_plane along -z direction */
		eye_distance=0.5*(near_plane+far_plane);
		lookat[0]=eye[0]-eye_distance*modelview_matrix[8];
		lookat[1]=eye[1]-eye_distance*modelview_matrix[9];
		lookat[2]=eye[2]-eye_distance*modelview_matrix[10];
		/* up vector is second row of modelview_matrix = y direction */
		up[0]=modelview_matrix[4];
		up[1]=modelview_matrix[5];
		up[2]=modelview_matrix[6];

		/* projection matrix has values only on main diagonal and at [1,4], */
		/* [2,4], [3,4], [4,3] */
		/* Get unknown components of following matrix S, such that */
		/* original t matrix with new 3rd row = S x modelview_matrix */
		/* S = | xnorm   0     0    S14  |  */
		/*     |   0   ynorm   0    S24  |  */
		/*     |   0     0    S33   S34  |  */
		/*     |   0     0  -znorm  S44  |  */
		S14=t[9]-xnorm*modelview_matrix[3];
		S24=t[10]-ynorm*modelview_matrix[7];
		S44=t[11]+znorm*modelview_matrix[11];
		/* The range of projected z-coordinates is from -1 on the near plane to */
		/* +1 on the far plane. Remembering that the z-coordinate also undergoes */
		/* division by the homogeneous coordinate, we have on these two planes: */
		/*   -1 = (S33*-near_plane + S34) / (znorm*near_plane + S44)   */
		/*   +1 = (S33*-far_plane  + S34) / (znorm*far_plane  + S44)   */
		/* which, can be solved for S33 and S34: */
		S33=(-znorm*(near_plane+far_plane)-2*S44)/(far_plane-near_plane);
		S34=(-znorm*(near_plane+far_plane)-2*S44)*near_plane/(far_plane-near_plane)-znorm*near_plane-S44;

#if defined (DEBUG_CODE)
		{
			double tmp_result[16];
			/* test! */
			printf("T:\n");
			print_matrix(4,3,t," %12.7f");
			projection_matrix[0]=xnorm;
			projection_matrix[1]=0.0;
			projection_matrix[2]=0.0;
			projection_matrix[3]=S14;
			projection_matrix[4]=0.0;
			projection_matrix[5]=ynorm;
			projection_matrix[6]=0.0;
			projection_matrix[7]=S24;
			projection_matrix[8]=0.0;
			projection_matrix[9]=0.0;
			projection_matrix[10]=S33;
			projection_matrix[11]=S34;
			projection_matrix[12]=0.0;
			projection_matrix[13]=0.0;
			projection_matrix[14]=-znorm;
			projection_matrix[15]=S44;
			printf("modelview matrix:\n");
			print_matrix(4,4,modelview_matrix," %12.7f");
			printf("projection matrix:\n");
			print_matrix(4,4,projection_matrix," %12.7f");
			multiply_matrix(4,4,4,projection_matrix,modelview_matrix,tmp_result);
			printf("result:\n");
			print_matrix(4,4,tmp_result," %12.7f");
		}
#endif /* defined (DEBUG_CODE) */

		/* Final projection matrix is S premultiplied by the following matrix A */
		/* to give the viewing volume in Normalized Device Coordinates: */
		/* A = | 2/NDC_width  0       0  -1-(2/NDC_width)*NDC_left    | */
		/*     |      0  2/NDC_height 0  -1-(2/NDC_height)*NDC_bottom | */
		/*     |      0       0       1       0                       | */
		/*     |      0       0       0       1                       | */
		/* x-direction: */
		projection_matrix[0]=(2.0/NDC_width)*xnorm;
		projection_matrix[1]=0.0;
		projection_matrix[2]=(1.0+2.0/NDC_width*NDC_left)*znorm;
		projection_matrix[3]=
			(2.0/NDC_width)*S14-(1.0+2.0/NDC_width*NDC_left)*S44;
		/* y-direction: */
		projection_matrix[4]=0.0;
		projection_matrix[5]=(2.0/NDC_height)*ynorm;
		projection_matrix[6]=(1.0+2.0/NDC_height*NDC_bottom)*znorm;
		projection_matrix[7]=
			(2.0/NDC_height)*S24-(1.0+2.0/NDC_height*NDC_bottom)*S44;
		/* z-direction: */
		projection_matrix[8]=0.0;
		projection_matrix[9]=0.0;
		projection_matrix[10]=S33;
		projection_matrix[11]=S34;
		/* homogeneous coordinate, h: */
		projection_matrix[12]=0.0;
		projection_matrix[13]=0.0;
		projection_matrix[14]=-znorm;
		projection_matrix[15]=S44;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"photogrammetry_to_graphics_projection.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* photogrammetry_to_graphics_projection */

int photogrammetry_project(double *t,double near_plane,double far_plane,
	double NDC_left,double NDC_bottom,double NDC_width,double NDC_height,
	double *pos3,double *win_pos)
/*******************************************************************************
LAST MODIFIED : 15 April 1998

DESCRIPTION :
Converts 3-D coordinate pos3 into its projection, win_pos on the window.
Note that the z-coordinate returned is 0.0 on the near plane and 1.0 on the far_plane
plane. The x and y coordinates returned should match those returned by
point_3d_to_2d_view.
==============================================================================*/
{
	int return_code,i,j;
	GLdouble modelview_matrix[16],projection_matrix[16],
		obj_x,obj_y,obj_z,win_x,win_y,win_z;
	GLint viewport[4];
	double modelview_matrixT[16],projection_matrixT[16],eye[3],lookat[3],up[3];

	ENTER(photogrammetry_project);
	if (t&&pos3&&win_pos)
	{
		return_code = photogrammetry_to_graphics_projection(t,near_plane,far_plane,
			NDC_left,NDC_bottom,NDC_width,NDC_height,
			modelview_matrixT,projection_matrixT,eye,lookat,up);
		if (return_code)
		{
			viewport[0]=0;
			viewport[1]=0;
			viewport[2]=1;
			viewport[3]=1;
			/* transpose our matrices */
			for (i=0;i<4;i++)
			{
				for (j=0;j<4;j++)
				{
					modelview_matrix[i*4+j]=modelview_matrixT[j*4+i];
					projection_matrix[i*4+j]=projection_matrixT[j*4+i];
				}
			}
			/* convert our 2-D coordinates into the above viewport */
			obj_x=(GLdouble)pos3[0];
			obj_y=(GLdouble)pos3[1];
			obj_z=(GLdouble)pos3[2];
			/* for OpenGL window z coordinates, 0.0=near_plane, 1.0=far_plane */
			if (GL_TRUE==gluProject(obj_x,obj_y,obj_z,
				modelview_matrix,projection_matrix,viewport,&win_x,&win_y,&win_z))
			{
				win_pos[0]=NDC_width*win_x+NDC_left;
				win_pos[1]=NDC_height*win_y+NDC_bottom;
				win_pos[2]=(double)win_z;
			}
			else
			{
				return_code=0;
			}
		}
		if (!return_code)
		{
			display_message(ERROR_MESSAGE,
				"photogrammetry_project.  Unable to project");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"photogrammetry_project.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* photogrammetry_project */

int photogrammetry_unproject(double *t,double near_plane,double far_plane,
	double NDC_left,double NDC_bottom,double NDC_width,double NDC_height,
	double *pos2,double *near_pos,double *far_pos)
/*******************************************************************************
LAST MODIFIED : 9 March 1998

DESCRIPTION :
Converts 2-D coordinate pos2 into two 3-D locations on the near_plane and far planes
of the viewing volume.
==============================================================================*/
{
	int return_code,i,j;
	GLdouble modelview_matrix[16],projection_matrix[16],
		obj_x,obj_y,obj_z,win_x,win_y;
	GLint viewport[4];
	double modelview_matrixT[16],projection_matrixT[16],eye[3],lookat[3],up[3];

	ENTER(photogrammetry_unproject);
	if (t&&pos2&&near_pos&&far_pos)
	{
		return_code = photogrammetry_to_graphics_projection(t,near_plane,far_plane,
			NDC_left,NDC_bottom,NDC_width,NDC_height,
			modelview_matrixT,projection_matrixT,eye,lookat,up);
		if (return_code)
		{
			viewport[0]=0;
			viewport[1]=0;
			viewport[2]=1;
			viewport[3]=1;
			/* transpose our matrices */
			for (i=0;i<4;i++)
			{
				for (j=0;j<4;j++)
				{
					modelview_matrix[i*4+j]=modelview_matrixT[j*4+i];
					projection_matrix[i*4+j]=projection_matrixT[j*4+i];
				}
			}
			/* convert our 2-D coordinates into the above viewport */
			win_x=(pos2[0]-NDC_left)/NDC_width;
			win_y=(pos2[1]-NDC_bottom)/NDC_height;
			return_code=0;
			/* for OpenGL window z coordinates, 0.0=near_plane, 1.0=far_plane */
			if (GL_TRUE==gluUnProject(win_x,win_y,0.0,
				modelview_matrix,projection_matrix,viewport,&obj_x,&obj_y,&obj_z))
			{
				near_pos[0]=(double)obj_x;
				near_pos[1]=(double)obj_y;
				near_pos[2]=(double)obj_z;
				if (GL_TRUE==gluUnProject(win_x,win_y,1.0,
					modelview_matrix,projection_matrix,viewport,&obj_x,&obj_y,&obj_z))
				{
					far_pos[0]=(double)obj_x;
					far_pos[1]=(double)obj_y;
					far_pos[2]=(double)obj_z;
					return_code=1;
				}
			}
		}
		if (!return_code)
		{
			display_message(ERROR_MESSAGE,
				"photogrammetry_unproject.  Unable to unproject");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"photogrammetry_unproject.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* photogrammetry_unproject */
