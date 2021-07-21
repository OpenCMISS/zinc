/*******************************************************************************
FILE : texture_line.h

LAST MODIFIED : 30 August 1996

DESCRIPTION :
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (TEXTURE_LINE_H)
#define TEXTURE_LINE_H

/*
Global functions
----------------
*/
struct VT_texture_curve *get_curve_from_list(struct VT_texture_curve **ptrfirst,
	struct VT_texture_curve *curve);
/*******************************************************************************
LAST MODIFIED : 30 August 1996

DESCRIPTION :
==============================================================================*/

int add_curve_to_list(struct VT_texture_curve **ptrfirst,
	struct VT_texture_curve *newcurve);
/*******************************************************************************
LAST MODIFIED : 30 August 1996

DESCRIPTION :
==============================================================================*/

int remove_curve_from_list(struct VT_texture_curve **ptrfirst,
	struct VT_texture_curve *curve);
/*******************************************************************************
LAST MODIFIED : 31 July 1996

DESCRIPTION :
==============================================================================*/

double line_segment_potential(double k,double r1,double r2,double L,double q1,
	double q2);
/*******************************************************************************
LAST MODIFIED : 30 August 1996

DESCRIPTION :
Calculates potential at a point from a line segment (length L) of charge (q1,q2)
(charge/unit length) with q1,q2 being distances r1,r2 from the point
respectively. (The charge/unit length varies linearly from q1 to q2)
==============================================================================*/

double line_segment_distance(double k,double *p1,double *p2,double *p,double q1,
	double q2);
/*******************************************************************************
LAST MODIFIED : 30 August 1996

DESCRIPTION :
Calculates distance potential at a point p from a line segment p1-p2 of charge
(q1,q2)
==============================================================================*/

double blob_segment_distance(double k,double *p1,double *p2,double *p,double q1,
	double q2);
/*******************************************************************************
LAST MODIFIED : 30 August 1996

DESCRIPTION :
Calculates distance potential at a point p from a blob segment p1-p2 of charge
(q1,q2)
==============================================================================*/

double curve_segment_distance(double k,double *p1,double *p2,double *p3,
	double *p4,double *p,double q1,double q2);
/*******************************************************************************
LAST MODIFIED : 30 August 1996

DESCRIPTION :
Calculates distance potential at a point p from a curve segment defined by
p1-p2, slope p3,p4 with charge(q1,q2) the distance Q(t)-p is minimized by
translating to the origin and solving the minimum sum Q(t)*Q(t) =>
0 = sum Q'(t).Q(t)
==============================================================================*/

double soft_object_distance(double WR,double *p1,double *p,double q1);
/*******************************************************************************
LAST MODIFIED : 4 March 1997

DESCRIPTION :
==============================================================================*/
#endif
