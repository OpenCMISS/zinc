/*******************************************************************************
FILE : mcubes.h

LAST MODIFIED : 11 April 2005

DESCRIPTION :
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (MCUBES_H)
#define MCUBES_H

#include "graphics/volume_texture.h"

/*
Global constants
----------------
*/
/* max # of polys to be put out from clip for one cell */
#define MAX_INTERSECTION 64  /* 36 */

/*
Global types
------------
*/
struct Triangle
{
	/* vertex */
	double v[3][3];
	/* normal */
	double n[3];
	/* clipping plane eqn parameter */
	double d;
	/* material at vertex */
	int mat[3];
	int clip_history;
	int clip_history2;
	/* store trilinear fn at vertices for deciding non_intersecting clips */
	double trilinear_int[3];
}; /* struct Triangle */

/* structure to represent certain cell values for use in interpolating */
struct Cell_fn
{
	double *xc;
	double *yc;
	double *zc;
	double *fn;
	double isovalue;
	/* the direction of the isosurface 1 out, -1 in */
	double dir;
}; /* struct Cell_fn */

/*
Global functions
----------------
*/

int marching_cubes(struct VT_scalar_field **scalar_field,int n_scalar_fields,
	struct VT_vector_field *coordinate_field,
	struct MC_iso_surface *mc_iso_surface,
	double *isovalue,int closed_surface,int cutting_plane_on);
/*******************************************************************************
LAST MODIFIED : 28 October 2005

DESCRIPTION :
The modified marching cubes algorithm for constructing isosurfaces.  This
constructs an isosurface for specified value from a filtered volume texture.
The nodal values are obtained by a weighting of the surrounding cells. These are
used to create an isosurface, and a polygon list is created.  Normals are
calculated using central or one sided differences at each node.  If
closed_surface, the iso surface generated contains closed surfaces along
intersections with the boundary.
==============================================================================*/

int update_scalars(struct VT_volume_texture *t);
/*******************************************************************************
LAST MODIFIED : 27 October 2005

DESCRIPTION :
Calculates scalar values at nodes from the average of the surrounding cell
values. Also calculates the gradient (using central differences) of the scalar
field. These values are then updated or stored as part of the volume_texture
structure.

???MS.  I think i should put in a provision to edit node values - ie so this
routine cant touch the set nodes.
==============================================================================*/

void print_texture(struct VT_volume_texture *texture);
/*******************************************************************************
LAST MODIFIED : 18 February 1998

DESCRIPTION :
Prints volume_texure scalar info into a file
==============================================================================*/

void destroy_mc_triangle_list(struct MC_cell *mc_cell,
	struct MC_triangle ***triangle_list,int *n_triangles,int n_scalar_fields, 
	struct MC_iso_surface *mc_iso_surface);
/*******************************************************************************
LAST MODIFIED : 24 February 1997

DESCRIPTION :
Destroy triangle list for a mc_cell, which includes (n_scalar_fields + 6)
independent triangle lists
==============================================================================*/

void clean_mc_iso_surface(int n_scalar_fields, 
    struct MC_iso_surface *mc_iso_surface);
/*******************************************************************************
LAST MODIFIED : 24 February 1997

DESCRIPTION : 
==============================================================================*/

int calculate_detail_map(struct VT_volume_texture *texture,
	struct MC_iso_surface *mc_iso_surface);
/*******************************************************************************
LAST MODIFIED : 17 July 1997

DESCRIPTION :
Fills mcubes detail map 0: leave same 1+: increase trianglular resolution 
==============================================================================*/
#endif /* !defined (MCUBES_H) */
