#define MAT_CODE
#define BOOLEAN_TEST
/*******************************************************************************
FILE : mcubes.c

LAST MODIFIED : 11 April 2005

DESCRIPTION :
Modified Marching cubes isosurface generating algorithm based on David
Kenwright's work. See his PhD thesis (1993).  Two files are used to create the
look up table, MARCHG and MCUBES.  MCUBES contains a triangulated data version
for polygon drawing. These files have been generated independently using an
algorithm described in Kenwright. (arrays are shifted by -1)
???DB.  Changed load_mc_tables into a program (utilities/create_mcubes_arrays.c)
	that reads in marchg.dat and mcubes.dat and writes out static declarations
	and initializers for mctbl, ipntbl and idatbl .  Replaced load_mc_tables with
	these declarations.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#define ACC4 0.00000001
#define ACC3 0.0000000001
/* uniqueness of vertices */
#define ACC 0.0000001
#define ACC2 0.0000001
#define AREA_ACC 0.0001
#define INTERSECTION_ACC 0.000000001
#define BLENDED_COLOURX
#define MC_EPSILON 0.000001
/* temporary vertex list size (for checking single cells) */
#define MAX_CELL_VERTICES 512
#include <stdio.h>
#include <math.h>

/* #define DEBUG_CODE */

#include "opencmiss/zinc/zincconfigure.h"



#if defined (DEBUG_CODE)
#  if defined (UNIX)
#    include <sys/times.h>
#  endif
#endif /* defined (DEBUG_CODE) */

#include "general/debug.h"
#include "general/matrix_vector.h"
#include "general/indexed_list_private.h"
#include "graphics/volume_texture.h"
#include "graphics/mcubes.h"
#include "graphics/complex.h"
#include "graphics/laguer.h"
#include "graphics/texture_line.h"
#include "general/message.h"

/* module data */

/* arrays defining face/edge/node connectivity */

/* binary number to represent each mc case */
static int ibin[8]={1,2,4,8,16,32,64,128};

/* edge/node relationship */
static int nodarr[12][2]={{1,5},{2,6},{4,8},{3,7},{1,2},{2,4},
	{4,3},{3,1},{5,6},{6,8},{8,7},{7,5}};

/* face/node */
static int facnod[6][4]={{1,3,7,5},{2,4,8,6},{1,5,6,2},
	{3,7,8,4},{1,2,4,3},{5,6,8,7}};

/* adjacent cell info */
static int adjcel[6][4]={{3,4,5,6},{3,4,5,6},{1,2,5,6},{1,2,5,6},
	{1,2,3,4},{1,2,3,4}};

/* table to replace triangle with more smaller ones */
static int detail_table[8][13]=
	{
		{1, 0, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{2, 0, 3, 2, 3, 1, 2, 0, 0, 0, 0, 0, 0},
		{2, 0, 1, 4, 0, 4, 2, 0, 0, 0, 0, 0, 0},
		{3, 0, 3, 2, 3, 4, 2, 3, 1, 4, 0, 0, 0},
		{2, 0, 1, 5, 1, 2, 5, 0, 0, 0, 0, 0, 0},
		{3, 0, 3, 5, 3, 2, 5, 3, 1, 2, 0, 0, 0},
		{3, 0, 1, 5, 1, 4, 5, 4, 2, 5, 0, 0, 0},
		{4, 0, 3, 5, 3, 4, 5, 3, 1, 4, 4, 2, 5}
	};

/* triangulated marching cubes data */
static int mctbl[17][256]=
	{
		{
			0,0,0,0,0,0,5,0,0,5,0,0,0,0,0,0,0,0,3,0,
			1,0,5,0,0,5,3,0,1,0,3,0,0,3,0,0,0,3,5,0,
			2,5,0,0,2,3,0,0,0,0,0,0,1,0,5,0,2,5,0,0,
			2,2,1,0,0,1,0,1,0,0,5,0,4,5,4,4,0,0,0,0,
			0,0,3,0,0,0,5,0,4,5,4,4,0,0,3,0,6,6,6,6,
			6,6,6,6,6,6,6,6,6,6,6,6,0,0,0,0,0,0,5,0,
			4,5,4,4,2,2,0,0,0,0,2,2,4,4,5,4,0,5,0,0,
			0,0,0,0,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
			0,3,0,0,4,4,5,4,0,5,0,0,0,3,0,0,0,0,0,0,
			4,4,5,4,0,5,0,0,1,0,1,0,0,1,2,2,0,0,5,2,
			0,5,0,1,0,0,0,0,0,0,3,2,0,0,5,2,0,5,3,0,
			0,0,3,0,0,3,0,1,0,3,5,0,0,5,0,1,0,3,0,0,
			0,0,0,0,0,0,5,0,0,5,0,0,0,0,0,0
		},
		{
			0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,2,1,2,2,3,
			2,3,3,4,2,3,3,4,3,4,4,3,1,2,2,3,2,3,3,4,
			2,3,3,4,3,4,4,3,2,3,3,2,3,4,4,3,3,4,4,3,
			4,5,5,2,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,3,
			2,3,3,4,3,2,4,3,3,4,4,5,4,3,5,2,2,3,3,4,
			3,4,4,5,3,4,4,5,4,5,5,4,3,4,4,3,4,3,5,2,
			4,5,5,4,5,4,2,1,1,2,2,3,2,3,3,4,2,3,3,4,
			3,4,4,3,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,4,
			2,3,3,4,3,4,4,5,3,4,2,3,4,5,3,2,3,4,4,3,
			4,5,5,4,4,5,3,2,5,2,4,1,2,3,3,4,3,4,4,5,
			3,4,4,5,2,3,3,2,3,4,4,5,4,3,5,4,4,5,5,2,
			3,2,4,1,3,4,4,5,4,5,5,2,4,5,3,4,3,4,2,1,
			2,3,3,2,3,2,4,1,3,4,2,1,2,1,1,0
		},
		{
			0,1,2,1,4,1,2,2,3,1,2,1,3,1,2,1,1,5,1,2,
			1,4,1,2,1,3,1,2,1,3,1,2,2,1,5,1,2,2,4,1,
			2,1,3,1,2,2,3,1,1,2,1,6,4,2,4,4,3,3,1,3,
			1,2,1,3,4,1,2,4,7,1,2,1,3,1,4,4,3,1,3,1,
			1,4,2,6,1,5,2,2,3,3,1,3,1,3,1,2,2,1,4,4,
			2,2,5,1,2,1,4,4,2,2,3,1,1,2,1,4,1,2,1,6,
			3,2,1,8,1,2,1,3,3,1,2,3,3,3,2,3,6,1,2,1,
			4,1,2,1,1,3,1,3,1,3,1,3,1,5,1,2,1,4,1,2,
			2,1,3,1,4,1,4,1,2,1,5,1,2,6,4,1,1,2,1,8,
			4,7,4,3,1,2,1,7,1,2,1,4,3,1,2,1,12,1,2,1,
			4,1,7,1,6,1,5,1,4,4,2,8,1,3,2,6,1,8,1,2,
			6,5,1,2,9,1,6,1,2,5,7,1,9,1,5,1,2,5,5,1,
			1,5,4,3,2,2,1,3,1,2,1,4,1,2,1,0
		},
		{
			0,8,5,8,7,4,5,1,6,8,5,8,6,4,5,4,9,9,9,9,
			9,7,9,9,9,6,9,9,9,6,9,9,10,8,6,8,10,10,7,4,
			10,8,10,8,10,10,10,4,2,10,5,10,7,10,7,7,6,6,5,10,
			2,10,5,10,12,8,5,12,8,12,5,12,6,8,12,12,6,12,2,12,
			9,8,5,2,9,9,5,9,6,6,9,2,9,6,9,9,10,8,12,12,
			10,10,6,12,10,8,12,12,10,10,5,12,2,10,5,8,2,10,5,10,
			6,10,11,7,2,10,5,10,11,8,5,11,11,11,5,11,7,8,5,8,
			11,4,5,4,9,11,9,11,9,11,9,11,9,9,9,9,9,11,9,9,
			3,8,11,8,7,4,7,4,6,8,7,8,6,5,11,4,2,3,5,6,
			7,5,7,11,2,6,5,11,2,6,5,11,4,8,5,8,10,12,5,6,
			12,8,4,8,8,12,8,12,1,8,5,6,9,7,5,2,9,5,9,9,
			8,9,9,9,2,8,3,8,3,1,8,12,2,8,12,8,6,1,8,12,
			2,2,1,4,3,3,5,7,2,6,5,8,2,6,5,0
		},
		{
			0,5,6,2,8,5,6,6,7,5,3,2,4,6,3,2,12,8,12,12,
			12,5,12,6,12,7,12,12,12,12,12,12,9,5,9,6,9,9,8,9,
			9,5,9,7,9,9,4,9,12,5,10,8,8,12,8,6,7,7,7,12,
			12,12,12,4,11,5,6,11,11,5,6,6,7,5,11,11,8,5,11,2,
			4,5,6,8,11,7,6,6,7,7,4,7,11,11,8,3,9,5,11,11,
			9,9,9,11,9,5,11,11,9,9,8,11,10,11,4,6,8,11,6,7,
			7,11,4,3,10,11,8,11,10,5,6,10,10,10,6,10,10,5,10,2,
			8,11,10,11,12,10,12,10,12,10,12,10,12,8,12,8,12,10,12,12,
			9,5,9,6,8,5,8,6,9,5,9,7,9,1,9,9,12,5,12,3,
			8,4,8,12,6,7,7,8,12,5,12,12,10,5,6,2,3,10,6,2,
			10,5,12,2,10,10,12,2,9,5,6,2,10,5,6,9,10,9,4,10,
			1,6,8,10,3,5,4,6,7,12,12,9,6,5,9,9,8,12,9,9,
			4,3,5,6,7,5,6,6,6,7,4,7,8,5,8,0
		},
		{
			0,0,0,2,0,5,4,6,0,3,3,2,4,6,3,2,0,8,2,2,
			4,4,2,6,3,5,2,2,3,12,2,2,0,2,9,1,4,1,5,9,
			3,2,3,1,3,1,4,9,12,5,10,8,1,2,1,4,1,2,1,3,
			12,2,12,4,0,4,4,1,11,5,7,1,4,3,2,1,3,5,11,2,
			4,4,1,8,1,7,1,6,1,4,4,7,1,11,8,3,4,2,5,1,
			7,7,9,1,3,2,3,1,3,1,3,1,1,2,4,4,8,2,1,7,
			1,2,11,8,1,2,3,0,0,3,3,1,4,1,3,1,10,6,10,2,
			8,1,10,1,3,5,2,9,3,4,2,2,6,8,2,8,6,4,2,2,
			9,2,3,1,2,5,3,6,9,2,9,1,9,6,4,9,12,5,12,8,
			1,4,1,3,1,2,1,8,12,4,12,0,10,3,3,2,12,1,12,6,
			4,4,7,2,10,1,5,2,4,4,4,8,1,3,1,6,1,8,4,4,
			6,6,8,0,9,9,6,1,2,5,7,3,9,9,12,9,2,5,9,0,
			4,5,4,6,2,5,1,0,1,2,4,0,8,0,0,0
		},
		{
			0,0,0,8,0,4,7,1,0,6,5,8,6,4,5,4,0,9,5,12,
			7,5,5,9,6,9,5,12,6,6,5,12,0,10,6,6,7,4,6,4,
			6,10,9,7,6,4,10,4,2,10,5,10,2,12,5,6,2,10,7,12,
			2,12,5,10,0,12,12,8,8,12,8,6,12,6,5,8,8,12,2,12,
			9,5,9,2,11,9,9,9,9,8,9,2,11,6,9,9,12,10,6,8,
			8,5,6,11,6,10,10,8,6,12,8,11,10,11,5,6,2,11,6,10,
			2,11,1,3,10,11,10,0,0,11,11,8,7,4,11,4,7,7,5,8,
			11,11,5,11,11,9,5,12,11,7,5,9,7,9,5,9,8,10,5,12,
			3,3,9,6,3,4,11,4,6,6,7,7,6,1,9,4,2,3,5,3,
			2,5,5,12,6,7,7,11,2,11,5,0,4,4,4,8,3,10,10,1,
			10,12,12,8,8,10,12,12,9,5,1,2,10,5,9,9,10,9,9,8,
			1,9,9,0,3,2,4,6,7,12,12,7,6,2,5,8,8,12,8,0,
			2,3,5,4,7,3,6,0,6,7,5,0,2,0,0,0
		},
		{
			0,0,0,6,0,7,8,4,0,7,7,7,8,3,8,3,0,12,6,6,
			8,12,6,12,7,8,3,3,4,9,4,4,0,9,10,10,8,5,9,10,
			7,9,5,3,4,5,9,10,10,12,6,12,12,5,7,12,12,12,3,8,
			10,5,10,12,0,11,11,2,12,7,11,2,11,7,3,2,11,11,5,11,
			11,11,4,9,7,11,11,11,4,5,11,8,3,9,11,11,11,9,9,6,
			11,1,10,7,7,9,9,7,8,5,11,3,11,5,11,11,7,5,10,11,
			4,5,10,10,11,5,11,0,0,10,10,2,8,5,10,7,11,10,11,7,
			10,5,11,10,10,8,6,8,10,5,6,12,10,12,10,12,4,6,11,4,
			11,9,6,3,9,7,9,7,11,9,11,11,11,4,8,11,11,8,11,11,
			12,12,6,6,12,11,12,12,11,12,11,0,12,10,10,6,7,3,3,7,
			7,10,5,7,12,5,2,10,10,3,9,9,3,9,10,7,6,4,10,7,
			9,10,10,0,12,3,12,9,8,7,3,6,12,6,4,7,9,6,12,0,
			3,8,6,8,8,7,3,0,7,4,7,0,6,0,0,0
		},
		{
			0,0,0,0,0,0,0,4,0,0,0,2,0,6,3,0,0,0,0,6,
			0,12,4,6,0,8,3,3,4,9,4,2,0,0,0,1,0,5,9,10,
			0,3,3,1,4,5,4,10,0,5,1,0,12,5,7,12,12,2,1,3,
			3,5,10,0,0,0,0,2,0,7,11,6,0,4,3,2,11,5,11,2,
			0,11,4,8,1,0,1,6,4,4,2,8,1,9,8,0,0,4,9,1,
			11,7,7,1,4,3,3,1,3,5,11,1,1,5,11,11,7,5,1,0,
			4,5,10,8,1,5,0,0,0,0,0,2,0,5,4,1,0,10,11,2,
			8,5,11,1,0,8,3,9,4,4,3,2,10,6,10,2,6,4,2,2,
			0,9,6,1,9,2,3,1,11,9,0,1,11,6,8,0,11,8,11,8,
			12,12,1,6,12,2,12,0,11,0,11,0,0,10,10,3,12,1,12,7,
			7,4,5,2,0,5,2,0,4,3,4,8,1,3,1,7,1,4,4,0,
			6,0,8,0,12,9,6,9,2,7,3,0,12,9,4,9,9,6,0,0,
			0,8,4,0,2,0,1,0,1,2,0,0,0,0,0,0
		},
		{
			0,0,0,0,0,0,0,7,0,0,0,7,0,5,8,0,0,0,0,12,
			0,5,7,12,0,9,5,12,6,6,5,4,0,0,0,10,0,4,6,4,
			0,6,5,3,6,4,9,4,0,12,10,0,2,12,5,6,2,12,3,8,
			6,12,5,0,0,0,0,8,0,12,8,12,0,12,5,8,8,11,5,11,
			0,5,9,9,7,0,11,11,9,5,5,2,3,6,11,0,0,12,6,6,
			8,1,8,7,12,6,9,7,8,12,8,3,11,11,5,6,2,11,10,0,
			2,11,1,10,11,11,0,0,0,0,0,8,0,4,7,7,0,7,5,7,
			10,11,5,10,0,9,11,8,7,5,11,12,7,7,5,8,4,6,11,4,
			0,3,9,3,3,3,9,6,6,6,0,11,6,4,9,0,2,3,5,11,
			2,5,6,12,6,11,7,0,2,0,5,0,0,4,4,4,7,3,3,1,
			10,10,12,7,0,10,12,0,10,5,9,9,3,9,10,9,6,9,10,0,
			9,0,10,0,3,3,12,6,8,12,12,0,6,6,5,7,8,12,0,0,
			0,3,6,0,8,0,3,0,7,4,0,0,0,0,0,0
		},
		{
			0,0,0,0,0,0,0,6,0,0,0,3,0,1,4,0,0,0,0,8,
			0,9,8,7,0,12,7,7,8,5,8,3,0,0,0,9,0,7,10,7,
			0,7,7,9,8,6,8,3,0,8,12,0,10,4,6,10,10,5,12,7,
			4,4,3,0,0,0,0,6,0,11,12,7,0,11,7,7,12,6,12,3,
			0,9,11,4,8,0,7,7,11,11,3,9,8,5,3,0,0,11,10,9,
			12,12,11,6,11,7,5,3,11,6,12,9,4,4,6,10,10,7,11,0,
			11,4,5,4,8,3,0,0,0,0,0,6,0,7,8,6,0,11,7,11,
			6,10,8,2,0,12,10,2,8,12,10,6,11,10,11,7,11,5,10,11,
			0,11,5,9,11,9,6,9,7,11,0,9,4,11,5,0,3,11,6,12,
			11,2,12,7,7,12,11,0,6,0,8,0,0,12,12,10,8,5,7,12,
			6,7,2,10,0,6,10,0,3,10,10,4,8,10,3,10,7,10,7,0,
			10,0,5,0,4,12,5,3,9,3,9,0,7,12,7,12,12,9,0,0,
			0,4,3,0,1,0,8,0,4,5,0,0,0,0,0,0
		},
		{
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,7,0,0,0,7,0,3,2,0,0,0,0,0,0,0,0,7,
			0,0,0,9,0,6,8,0,0,0,0,0,0,5,1,0,0,5,12,0,
			4,5,3,0,0,0,0,0,0,0,0,7,0,0,0,2,0,6,12,0,
			0,0,0,4,0,0,1,0,0,11,3,8,8,0,8,0,0,0,0,9,
			0,7,11,1,0,4,3,1,11,6,10,9,0,5,11,0,7,0,1,0,
			11,5,10,4,8,5,0,0,0,0,0,0,0,0,0,1,0,0,0,2,
			0,5,11,0,0,0,0,2,0,12,4,6,0,10,11,2,6,4,11,2,
			0,0,0,9,0,9,6,9,0,11,0,0,4,6,0,0,0,8,11,0,
			11,12,12,7,12,2,0,0,11,0,11,0,0,0,0,10,0,5,12,7,
			0,7,2,10,0,0,0,0,0,10,4,4,8,0,1,7,1,4,7,0,
			0,0,5,0,0,12,5,9,9,3,3,0,12,12,0,12,0,6,0,0,
			0,0,0,0,0,0,8,0,0,5,0,0,0,0,0,0
		},
		{
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,12,0,0,0,12,0,12,4,0,0,0,0,0,0,0,0,10,
			0,0,0,3,0,4,9,0,0,0,0,0,0,4,7,0,0,12,3,0,
			6,4,5,0,0,0,0,0,0,0,0,12,0,0,0,7,0,11,5,0,
			0,0,0,9,0,0,7,0,0,5,5,9,3,0,3,0,0,0,0,6,
			0,12,8,6,0,12,5,3,8,12,9,3,0,4,6,0,10,0,11,0,
			2,4,5,10,11,3,0,0,0,0,0,0,0,0,0,6,0,0,0,11,
			0,10,8,0,0,0,0,8,0,5,7,12,0,7,5,7,11,5,5,11,
			0,0,0,3,0,3,9,6,0,6,0,0,6,11,0,0,0,11,6,0,
			2,2,6,12,7,12,0,0,6,0,8,0,0,0,0,4,0,3,7,12,
			0,10,12,7,0,0,0,0,0,5,10,9,3,0,3,10,7,10,10,0,
			0,0,10,0,0,3,12,3,8,12,9,0,7,6,0,7,0,9,0,0,
			0,0,0,0,0,0,3,0,0,4,0,0,0,0,0,0
		},
		{
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,4,0,0,0,8,0,4,3,0,0,0,0,0,0,0,0,6,
			0,0,0,10,0,3,5,0,0,0,0,0,0,7,12,0,0,8,10,0,
			8,6,4,0,0,0,0,0,0,0,0,11,0,0,0,3,0,3,8,0,
			0,0,0,11,0,0,8,0,0,9,7,4,6,0,2,0,0,0,0,10,
			0,11,12,10,0,11,7,9,12,11,5,10,0,8,10,0,11,0,7,0,
			10,8,3,11,3,6,0,0,0,0,0,0,0,0,0,2,0,0,0,10,
			0,6,4,0,0,0,0,6,0,9,8,4,0,11,7,10,10,9,4,10,
			0,0,0,11,0,11,5,11,0,7,0,0,8,9,0,0,0,12,3,0,
			3,11,3,4,11,8,0,0,4,0,4,0,0,0,0,12,0,7,8,3,
			0,6,10,4,0,0,0,0,0,9,3,10,7,0,8,3,4,7,2,0,
			0,0,2,0,0,4,9,12,12,2,6,0,4,7,0,4,0,2,0,0,
			0,0,0,0,0,0,7,0,0,8,0,0,0,0,0,0
		},
		{
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,6,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,4,0,0,8,0,0,0,0,0,
			0,0,0,1,0,0,0,9,0,6,10,0,0,0,0,0,0,0,1,0,
			0,3,3,0,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,6,0,0,0,10,0,4,4,0,
			0,0,0,0,0,0,0,11,0,0,0,0,0,6,0,0,0,0,0,0,
			0,11,12,0,0,2,0,0,4,0,0,0,0,0,0,0,0,0,0,3,
			0,0,0,10,0,0,0,0,0,0,0,4,0,0,8,0,0,7,7,0,
			0,0,0,0,0,0,0,12,0,2,6,0,0,12,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
		},
		{
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,4,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,9,0,0,2,0,0,0,0,0,
			0,0,0,10,0,0,0,3,0,11,5,0,0,0,0,0,0,0,7,0,
			0,6,5,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,7,0,9,5,0,
			0,0,0,0,0,0,0,6,0,0,0,0,0,9,0,0,0,0,0,0,
			0,2,3,0,0,8,0,0,6,0,0,0,0,0,0,0,0,0,0,12,
			0,0,0,4,0,0,0,0,0,0,0,10,0,0,3,0,0,10,2,0,
			0,0,0,0,0,0,0,3,0,12,9,0,0,7,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
		},
		{
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,3,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,11,0,0,5,0,0,0,0,0,
			0,0,0,9,0,0,0,10,0,3,3,0,0,0,0,0,0,0,8,0,
			0,7,7,0,6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,7,0,0,0,11,0,12,8,0,
			0,0,0,0,0,0,0,3,0,0,0,0,0,2,0,0,0,0,0,0,
			0,3,11,0,0,5,0,0,8,0,0,0,0,0,0,0,0,0,0,10,
			0,0,0,12,0,0,0,0,0,0,0,3,0,0,7,0,0,6,5,0,
			0,0,0,0,0,0,0,4,0,9,5,0,0,4,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
		}
	};

/* current cell node coords */
double xc[8],yc[8],zc[8];

/* scalar values at nodes */
double fc[MAX_SCALAR_FIELDS][8];

/* material indices at nodes */
double mc[8];

/* triangle edge relationships */
int triangle_edge_list[4][2]={ {0,1}, {1,2}, {2,0},{0,1} };

/* retriangulation list for complete cut off */
int retriangulation_list[8][7]={ {1,1,2,3,4,5,6},{2,4,2,6,2,3,6},
	{2,1,4,5,1,5,3},{1,5,3,6,2,3,1},{2,1,2,5,1,5,6},{1,4,2,5,1,2,3},
	{1,1,4,6,1,2,3},{1,1,2,3,4,5,6} };

static int n_clipped_triangles;
static struct Triangle *clipped_triangle_list[MAX_INTERSECTION];
struct Cell_fn cell_fn;

double cube[8][3]=
	{{0,0,0},{1,0,0},{0,1,0},{1,1,0},{0,0,1},{1,0,1},{0,1,1},{1,1,1}};

int debug_flag;
int debug_i=0,debug_j=0,debug_k=0;

/*
Module functions
----------------
*/

static int vectors_equal(double v1[3],double v2[3],double tolerance)
/*******************************************************************************
LAST MODIFIED : 18 February 1998

DESCRIPTION :
Compares difference of vectors to a tolerance. returns 1 if less than tolerance,
0 if greater.
==============================================================================*/
{
	double v[3];
	int i;

	for (i=0;i<3;i++)
	{
		v[i]=v1[i]-v2[i];
	}
	if (sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]) < tolerance)
	{
		return(1);
	}
	else
	{
		return(0);
	}
} /* vectors_equal */

static int shared_edge(double v1[3],double v2[3],double t2[3][3])
/*******************************************************************************
LAST MODIFIED : 17 July 1997

DESCRIPTION :
Checks for shared edge in MC triangulation
==============================================================================*/
{
	int m,n,return_code;

	ENTER(shared_edge);
	return_code=0;
	for (n=0;n<3;n++)
	{
		m=n+1;
		if (3==m)
		{
			m=0;
		}
		if (((v1[0]==t2[n][0])&&(v1[1]==t2[n][1])&&(v1[2]==t2[n][2])&&
			(v2[0]==t2[m][0])&&(v2[1]==t2[m][1])&&(v2[2]==t2[m][2]))||
			((v1[0]==t2[m][0])&&(v1[1]==t2[m][1])&&(v1[2]==t2[m][2])&&
			(v2[0]==t2[n][0])&&(v2[1]==t2[n][1])&&(v2[2]==t2[n][2])))
		{
			return_code=1;
		}
	}
	LEAVE;

	return (return_code);
} /* shared_edge */

static int internal_boundary(double v1[3],double v2[3],double xc[8],
	double yc[8],double zc[8],int i,int j,int k,int *detail_map,int mcnx,int mcny,
	int mcnz, int res)
/*******************************************************************************
LAST MODIFIED : 1 November 1999

DESCRIPTION :
Checks for shared boundary edge in mc triangulation
==============================================================================*/
{
	int return_code;

	ENTER(internal_boundary);
  USE_PARAMETER(mcnz);
	if (((v1[0]==xc[0])&&(v2[0]==xc[0])&&(res+1 <= detail_map[(i-1)+j*mcnx+k*mcnx*mcny]))||
		((v1[0]==xc[7])&&(v2[0]==xc[7])&&(res+1 <= detail_map[(i+1)+j*mcnx+k*mcnx*mcny]))||
		((v1[1]==yc[0])&&(v2[1]==yc[0])&&(res+1 <= detail_map[i+(j-1)*mcnx+k*mcnx*mcny]))||
		((v1[1]==yc[7])&&(v2[1]==yc[7])&&(res+1 <= detail_map[i+(j+1)*mcnx+k*mcnx*mcny]))||
		((v1[2]==zc[0])&&(v2[2]==zc[0])&&(res+1 <= detail_map[i+j*mcnx+(k-1)*mcnx*mcny]))||
		((v1[2]==zc[7])&&(v2[2]==zc[7])&&(res+1 <= detail_map[i+j*mcnx+(k+1)*mcnx*mcny])))
	{
		return_code=1;
	}
	else
	{
		return_code=0;
	}

	LEAVE;

	return (return_code);
} /* internal_boundary */

static int compile_mc_vertex_triangle_lists(
	struct MC_iso_surface *mc_iso_surface,int closed_surface)
/*******************************************************************************
LAST MODIFIED : 1 November 1999

DESCRIPTION :
==============================================================================*/
{
	int triangle_index,vertex_index;
	int x_min,x_max,y_min,y_max,z_min,z_max;
	int ii,jj,kk,mcnx,mcny;
	int i,j,k,m,n,index,n_triangles,n_cv,found_in_cell,prior_cell;
	struct MC_cell *mc_cell;
	struct MC_vertex *v;
	struct MC_vertex *cell_vertices[MAX_CELL_VERTICES];
	struct MC_triangle *triangle, **triangle_list;

	ENTER(compile_mc_vertex_triangle_lists);
	x_min=1;
	x_max=mc_iso_surface->dimension[0];
	y_min=1;
	y_max=mc_iso_surface->dimension[1];
	z_min=1;
	z_max=mc_iso_surface->dimension[2];
	if (closed_surface)
	{
		if (1==x_min)
		{
			x_min=0;
		}
		if (1==y_min)
		{
			y_min=0;
		}
		if (1==z_min)
		{
			z_min=0;
		}
		if (mc_iso_surface->dimension[0]==x_max)
		{
			x_max += 1;
		}
		if (mc_iso_surface->dimension[1]==y_max)
		{
			y_max += 1;
		}
		if (mc_iso_surface->dimension[2]==z_max)
		{
			z_max += 1;
		}
	}
/*???debug */
/*printf("Compiling triangle and vertex lists....\n");*/
	if (mc_iso_surface->compiled_vertex_list)
	{
		DEALLOCATE(mc_iso_surface->compiled_vertex_list);
	}
	if (mc_iso_surface->compiled_triangle_list)
	{
		DEALLOCATE(mc_iso_surface->compiled_triangle_list);
	}
	triangle_index=vertex_index=0;
	if (ALLOCATE(mc_iso_surface->compiled_vertex_list,struct MC_vertex *,
		mc_iso_surface->n_vertices)&&ALLOCATE(mc_iso_surface->
		compiled_triangle_list,struct MC_triangle *,mc_iso_surface->n_triangles))
	{
		mcnx=mc_iso_surface->dimension[0]+2;
		mcny=mc_iso_surface->dimension[1]+2;
		/*mcnz=mc_iso_surface->dimension[2]+2;*/
		for (index=0;index<mc_iso_surface->n_scalar_fields+6;index++)
		{
			for (kk=z_min;kk<=z_max;kk++)
			{
				for (jj=y_min;jj<=y_max;jj++)
				{
					for (ii=x_min;ii<=x_max;ii++)
					{
						/* step through non_null cells, add vertex lists of any new triangle
							vertices in cell or pointing to forward cells */
						i=ii+jj*mcnx+kk*mcnx*mcny;
						if (mc_iso_surface->mc_cells[i] !=NULL)
						{
							if (mc_iso_surface->mc_cells[i]->triangle_list[index] !=NULL)
							{
								mc_cell=mc_iso_surface->mc_cells[i];
								triangle_list=mc_cell->triangle_list[index];
								n_triangles=mc_cell->n_triangles[index];
								n_cv=0;
								for (k=0;k<MAX_CELL_VERTICES;k++)
								{
									cell_vertices[k]=NULL;
								}
								for (j=0;j<n_triangles;j++)
								{
									triangle=triangle_list[j];

									mc_iso_surface->compiled_triangle_list[triangle_index]=
										triangle;
									triangle->triangle_index=triangle_index;
									triangle_index++;
									/* step through vertices */
									for (n=0;n<3;n++)
									{
										v=triangle->vertices[n];
										found_in_cell=0;
										prior_cell=0;
										for (k=0;k<n_cv;k++)
										{
											if ((v==cell_vertices[k])&&!found_in_cell)
											{
												found_in_cell=1;
												if ((v->vertex_index>=0)&&
													(v->vertex_index<mc_iso_surface->n_vertices))
												{
													triangle->vertex_index[n]=v->vertex_index;
												}
/*???debug */
else
{
	printf("ERROR: (1)compile vt list: v->vertex_index=%d\n", v->vertex_index);
}
											}
										}
										if (!found_in_cell)
										{
											/* check prior cells through triangles */
											for (m=0;m<v->n_triangle_ptrs;m++)
											{
												/* if the pointed to cell is less than the current it
													must have been visited already.  Changed to
												   step fastest in z */
												if ((v->triangle_ptrs[m]->cell_ptr->index[0]+
													v->triangle_ptrs[m]->cell_ptr->index[1]*mcnx+
													v->triangle_ptrs[m]->cell_ptr->index[2]*mcnx*mcny<
													mc_cell->index[0]+mc_cell->index[1]*mcnx+
													mc_cell->index[2]*mcnx*mcny)&&!prior_cell)
												{
													prior_cell=1;
													/* check vertex index sensible */
													if ((v->vertex_index>=0)&&
														(v->vertex_index<mc_iso_surface->n_vertices))
													{
														triangle->vertex_index[n]=v->vertex_index;
													}
/*???debug */
else
{
	printf("ERROR: (2) compile vt list: v->vertex_index=%d\n", v->vertex_index);
	printf("v (%p)=%f %f %f\n",v,  v->coord[0], v->coord[1], v->coord[2]);
	printf("compare cell (%d %d %d)=%d to prev cell (%d %d %d)=%d\n",
		mc_cell->index[0],mc_cell->index[1], mc_cell->index[2],
		mc_cell->index[0]+mc_cell->index[1]*mcnx+mc_cell->index[2]*mcnx*mcny,
		v->triangle_ptrs[m]->cell_ptr->index[0],
		v->triangle_ptrs[m]->cell_ptr->index[1],
		v->triangle_ptrs[m]->cell_ptr->index[2],
		v->triangle_ptrs[m]->cell_ptr->index[0]+
		v->triangle_ptrs[m]->cell_ptr->index[1]*mcnx+
		v->triangle_ptrs[m]->cell_ptr->index[2]*mcnx*mcny);
}
												}
											}
										}
										if (!found_in_cell && !prior_cell)
										{
											/* store vertex */
											cell_vertices[n_cv]=v;
											if (n_cv < MAX_CELL_VERTICES)
											{
												n_cv++;
											}
											else
											{
												display_message(ERROR_MESSAGE,
							"compile_mc_vertex_triangle_lists.  Increase MAX_CELL_VERTICES");
/*???debug */
printf("MAX CELL VERTICES REACHED\n");
											}
											mc_iso_surface->compiled_vertex_list[vertex_index]=v;
/*???debug */
if (vertex_index > mc_iso_surface->n_vertices || vertex_index < 0)
{
	int mm;

	printf("VERTEX %d INDEX ERROR: mc_cell (%d %d %d) <%p>\n",vertex_index,
		ii, jj, kk, mc_iso_surface->mc_cells[i]);
	printf("v (%d %d %d) <%p>\n", ii, jj, kk, mc_iso_surface->mc_cells[i]);
	printf("v->n_triangle_ptrs=%d :", v->n_triangle_ptrs);
	for (mm=0;mm< v->n_triangle_ptrs;mm++)
	{
		printf(" %p (%d) : ", v->triangle_ptrs[mm],
			v->triangle_ptrs[mm]->triangle_index);
	}
	printf("\n");
	for (mm=0;mm< v->n_triangle_ptrs;mm++)
	{
		printf("(%d)  %d %d %d : ", v->triangle_ptrs[mm]->triangle_index,
			v->triangle_ptrs[mm]->cell_ptr->index[0],
		v->triangle_ptrs[mm]->cell_ptr->index[1],
		v->triangle_ptrs[mm]->cell_ptr->index[2]);
	}
/*???DB.  Not allowed.  Have to handle errors better */
/*	exit(0);*/
}
											v->vertex_index=vertex_index;
											triangle->vertex_index[n]=v->vertex_index;
											vertex_index++;

										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"compile_mc_vertex_triangle_lists.  Alloc failed");
	}
/*???debug */
/*printf("-------Compiled list of %d vertices and %d triangles---------\n",
	vertex_index,triangle_index);*/
	LEAVE;

	return (triangle_index);
} /* compile_mc_vertex_triangle_lists */

#if defined (USE_PARAMETER_ON) // ==++
struct MC_vertex *check_mc_vertex(struct MC_cell *mc_cell,ZnReal v[3],
	int a)
/*******************************************************************************
LAST MODIFIED : 26 March 1997

DESCRIPTION :
Compare vertices. If found return mc_vertex else return NULL.
==============================================================================*/
{
	int i, j, n_triangles, found;
	struct MC_triangle **triangle_list, *triangle;
	struct MC_vertex *mc_vertex,  *return_value;

	ENTER(check_mc_vertex);
	/* go through cell triangle vertices and compare */
	found=0;
	return_value=NULL;
	n_triangles=mc_cell->n_triangles[a];
	triangle_list=mc_cell->triangle_list[a];
	if (triangle_list)
	{
		for (i=0;i<n_triangles;i++)
		{
			if (!found)
			{
				triangle=triangle_list[i];
				for (j=0;j<3;j++)
				{
					if (!found)
					{
						mc_vertex=triangle->vertices[j];
						if ((fabs(mc_vertex->coord[0] - v[0]) < MC_EPSILON)&&
							(fabs(mc_vertex->coord[1] - v[1]) < MC_EPSILON)&&
							(fabs(mc_vertex->coord[2] - v[2]) < MC_EPSILON))
						{
							found=1;
							return_value=mc_vertex;
						}
					}
				}
			}
		}
	}
	LEAVE;

	return (return_value);
} /* check_mc_vertex */
#endif /* defined (USE_PARAMETER_ON) */

static void add_mc_triangle(int i,int j,int k,
	struct MC_iso_surface *mc_iso_surface,int a,ZnReal mc_vertices[3][3],int x_min,
	int x_max,int y_min,int y_max,int z_min,int z_max,int mcnx,int mcny,int mcnz,
	int n_scalar_fields)
/*******************************************************************************
LAST MODIFIED : 1 November 1999

DESCRIPTION :
Add mc_triangle to mc_cell list and add new vertex if unique
==============================================================================*/
{
	int n, nn, found;
	//int jj = 0, kk = 0;// <== \todo someone is intending to use these?

	/* pointers to vertices */
	struct MC_vertex *v[3];
	struct MC_triangle *triangle,**temp_triangle_ptrs,**temp_triangle_list;
	struct MC_cell *mc_cell;

	ENTER(add_mc_triangle);
	USE_PARAMETER(x_min);
	USE_PARAMETER(x_max);
	USE_PARAMETER(y_min);
	USE_PARAMETER(y_max);
	USE_PARAMETER(z_min);
	USE_PARAMETER(z_max);
	USE_PARAMETER(mcnz);
	USE_PARAMETER(n_scalar_fields);
	/* step through each vertex */
	for (n=0;n<3;n++)
	{
		found=0;
		//USE_PARAMETER(jj);
		//USE_PARAMETER(kk);
		//USE_PARAMETER(check_mc_vertex);
		if (!found)
		{
			if (ALLOCATE(v[n], struct MC_vertex, 1))
			{
				mc_iso_surface->n_vertices++;
				v[n]->n_triangle_ptrs=0;
				v[n]->triangle_ptrs=NULL;
				v[n]->vertex_index=-1;
				for (nn=0;nn<3;nn++)
				{
					v[n]->coord[nn]=mc_vertices[n][nn];
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,"add_mc_triangle.   Alloc vertex failed");
			}
		}
	}
	/* allocate triangle */
	if (ALLOCATE(triangle, struct MC_triangle,  1))
	{
		mc_iso_surface->n_triangles++;
		triangle->triangle_list_index=a;
		triangle->triangle_index= -1;
		triangle->vertex_index[0]= -1;
		triangle->vertex_index[1]= -1;
		triangle->vertex_index[2]= -1;
		for (n=0;n<3;n++)
		{
			triangle->vertices[n]=v[n];
			/* initialise structure */
			/* for each vertex assign back pointers */
			if (REALLOCATE(temp_triangle_ptrs,v[n]->triangle_ptrs,
				struct MC_triangle *, v[n]->n_triangle_ptrs+1))
			{
				temp_triangle_ptrs[v[n]->n_triangle_ptrs]=triangle;
				v[n]->n_triangle_ptrs++;
				v[n]->triangle_ptrs=temp_triangle_ptrs;
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"add_mc_triangle.   Alloc triangle_ptrs failed");
			}
		}
		triangle->cell_ptr=mc_iso_surface->mc_cells[i+mcnx*j+mcnx*mcny*k];
		if (triangle->cell_ptr==NULL)
		{
			display_message(ERROR_MESSAGE,"add_mc_triangles.  Cell=NULL");
/*???debug */
printf("ERROR: cell(%d %d %d)=NULL\n", i, j, k);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"add_mc_triangle:  Alloc triangle failed");
	}
	/* update cell list of triangles */
	mc_cell=mc_iso_surface->mc_cells[i+mcnx*j+mcnx*mcny*k];
	if (mc_cell)
	{
		if (REALLOCATE(temp_triangle_list,mc_cell->triangle_list[a],
			struct MC_triangle *, mc_cell->n_triangles[a]+1))
		{
			temp_triangle_list[mc_cell->n_triangles[a]]=triangle;
			mc_cell->triangle_list[a]=temp_triangle_list;
			mc_cell->n_triangles[a]++;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"add_mc_triangle.   Alloc triangle_list failed");
		}
	}
	LEAVE;
} /* add_mc_triangle */

static void destroy_mc_triangle(struct MC_triangle *triangle,
	struct MC_iso_surface *mc_iso_surface)
/*******************************************************************************
LAST MODIFIED : 28 August 1998

DESCRIPTION :
Destroy mc_triangle
==============================================================================*/
{
	int i, j, k, delete_index;
	struct MC_vertex *v;
	struct MC_triangle **temp_triangle_ptrs;

	ENTER(destroy_mc_triangle);
	/* delete vertex list links - if vertex shares no other triangles, delete
		it */
	for (i=0;i<3;i++)
	{
		v=triangle->vertices[i];
		/* search through vertex list and delete entry - if no other entries,
			delete vertex */
		delete_index = -1;
		for (j=0;j < v->n_triangle_ptrs; j++)
		{
			if (v->triangle_ptrs[j]==triangle)
			{
				delete_index=j;
			}
		}
		if (delete_index != -1)
		{
			if (1==v->n_triangle_ptrs)
			{
				mc_iso_surface->n_vertices--;
				/* delete vertex as this is the last one */
				DEALLOCATE(v->triangle_ptrs);
				DEALLOCATE(v);
			}
			else
			{
				/* copy shortened list minus current triangle */
				k=0;
				if (ALLOCATE(temp_triangle_ptrs,struct MC_triangle *,
						v->n_triangle_ptrs-1))
				{
					for (j=0;j<v->n_triangle_ptrs;j++)
					{
						/* leave out one to be deleted */
						if (j!=delete_index)
						{
							temp_triangle_ptrs[k]=v->triangle_ptrs[j];
							k++;
						}
					}
					DEALLOCATE(v->triangle_ptrs);
					v->triangle_ptrs=temp_triangle_ptrs;
					v->n_triangle_ptrs--;
				}
				else
				{
					display_message(ERROR_MESSAGE,"destroy_mc_triangle.  Alloc failed");
				}
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,"destroy_mc_triangle.  "
				"Triangle not found in one of its vertices triangle lists.");
		}
	}
	DEALLOCATE(triangle);
	LEAVE;
} /* destroy_mc_triangle */

static void destroy_mc_triangle_field_list(struct MC_cell *mc_cell,
	struct MC_triangle **triangle_list,int n_triangles,
	struct MC_iso_surface *mc_iso_surface)
/*******************************************************************************
LAST MODIFIED : 1 November 1999

DESCRIPTION :
Destroy triangle list for particular scalar field or face.
==============================================================================*/
{
	int i;

	ENTER(destroy_mc_triangle_field_list);
  USE_PARAMETER(mc_cell);
	if (triangle_list && n_triangles > 0)
	{
		for (i=0;i < n_triangles;i++)
		{
			if (triangle_list[i])
			{
				destroy_mc_triangle(triangle_list[i], mc_iso_surface);
				mc_iso_surface->n_triangles--;
			}
		}
		DEALLOCATE(triangle_list);
	}
	LEAVE;
} /* destroy_mc_triangle_field_list */

static double rqs(struct VT_scalar_field *scalar_field,int i,int j,int k)
/*******************************************************************************
LAST MODIFIED : 18 February 1998

DESCRIPTION :
Gives requested scalar value at a node in a volume_texture
==============================================================================*/
{
int nx,ny,nz;

/* adjust dimension for nodes (1 greater the cells) */
nx=scalar_field->dimension[0]+1;
ny=scalar_field->dimension[1]+1;
nz=scalar_field->dimension[2]+1;

/* if closed surface, indices will be out of range on border, which is effectively
covered in zeros for closure */

if ((i==-1) || (j==-1) || (k==-1) || (i==nx) || (j==ny) || (k==nz))
	{
	return(0.0);
	}
else
	{
	return(scalar_field->scalar[i + j*nx + k*nx*ny]);
	}
} /* rqs */

static void dblcon(struct VT_scalar_field *scalar_field,int nx,int ny,int nz,
	int ic,int jc,int kc,int iface,double cl,double *grads)
/*******************************************************************************
LAST MODIFIED : 2 May 2002

DESCRIPTION :
Sorts out the tricky 'double contour' cases - returns gradient in grads
==============================================================================*/
{
	int i,in,jn,kn;
	int icell;
	int border;
	double f1,f2,f3,f4,a,b,c,d;
	double fn[8];

	/* adjust nx etc if using virtual dim[n+1] cube for closed surfaces.
		 I am not sure about this -- Mark */
	if (scalar_field->dimension[0] !=nx) nx--;
	if (scalar_field->dimension[1] !=nx) ny--;
	if (scalar_field->dimension[2] !=nx) nz--;
	/*--------*/

	for (i = 1; i <= 4; i++)
	{
		/*determine cell indices of an adjacent cell */
		icell=adjcel[iface - 1][i - 1];
		in=ic;
		jn=jc;
		kn=kc;
		border=0;
		switch (icell)
		{
			case 1:
			{
				in = in - 1;
				if (in < 1)
				{
					border = 1;
				}
			} break;
			case 2:
			{
				in = in + 1;
				if (in > nx)
				{
					border = 1;
				}
			} break;
			case 3:
			{
				jn = jn - 1;
				if (jn < 1)
				{
					border = 1;
				}
			} break;
			case 4:
			{
				jn = jn + 1;
				if (jn > ny)
				{
					border = 1;
				}
			} break;
			case 5:
			{
				kn = kn - 1;
				if (kn < 1)
				{
					border = 1;
				}
			} break;
			case 6:
			{
				kn = kn + 1;
				if (kn > nz)
				{
					border = 1;
				}
			} break;
			default:
			{
				/* do nothing */
			} break;
		}
		if (!border)
		{
			/* get scalar values for given cell in fn here */
			fn[0]=rqs(scalar_field,in-1,jn-1,kn-1);
			fn[1]=rqs(scalar_field,in,jn-1,kn-1);
			fn[2]=rqs(scalar_field,in-1,jn,kn-1);
			fn[3]=rqs(scalar_field,in,jn,kn-1);
			fn[4]=rqs(scalar_field,in-1,jn-1,kn);
			fn[5]=rqs(scalar_field,in,jn-1,kn);
			fn[6]=rqs(scalar_field,in-1,jn,kn);
			fn[7]=rqs(scalar_field,in,jn,kn);

			f1 = fn[facnod[iface - 1][0] - 1];
			f2 = fn[facnod[iface - 1][1] - 1];
			f3 = fn[facnod[iface - 1][2] - 1];
			f4 = fn[facnod[iface - 1][3] - 1];

			if (!(  ((f1>cl) && (f3>cl) && (f2 < cl) && (f4 < cl)) ||
				((f1<cl) && (f3<cl) && (f2 > cl) && (f4 > cl)) ))
			{
				/* evaluate the sign of the gradient of the adjacent face */
				a=f1 - cl;
				b=f4 - f1;
				c=f2 - f1;
				d=f1 - f2 + f3 - f4;
				*grads=a*d - b*c;
				return;
			}
		}
	}
	/* all adjacent faces have double contours */
	*grads=1.0;
	return;
} /* dblcon */

static int rqc(struct VT_vector_field *coordinate_field,int i,int j,int k,
	double coord[3])
/*******************************************************************************
LAST MODIFIED : 21 December 1995

DESCRIPTION :
Gives requested coordinates of a node in a volume_texture returns 1 if value on
border
==============================================================================*/
{
	int nx,ny,nz,n;
	int is_border_face=0;
	int x1,x2,y1,y2,z1,z2;

	ENTER(rqc);
	/* these are variables which are used to deal with a virtual border (1 cell
		thick) used when calculating a closed surface isosurface. If the coordinate
		value does not exist, then one is created by extrapolating using the width
		of the cells at the edge of the real cube.  The formula
		coord(x)=2*coord(x1)-coord(x2)
		is used which produces coord(x) if x1=x2 and the desired extrapolation if
		not */
	x1=x2=i;
	y1=y2=j;
	z1=z2=k;
	nx=coordinate_field->dimension[0]+1;
	ny=coordinate_field->dimension[1]+1;
	nz=coordinate_field->dimension[2]+1;
	/* if closed surface, indices will be out of range on border.  To achieve a
		flat surface on the borders, the virtual border cells have no thickness and
		hence the coordinate values of the interior adjacent cells are returned */
	if (i==-1)
	{
		x1=0;x2=1;
		is_border_face=1;
	}
	if (j==-1)
	{
		y1=0;y2=1;
		is_border_face=1;
	}
	if (k==-1)
	{
	z1=0;z2=1;
	is_border_face=1;
	}
	if (i==nx)
	{
		x1=nx-1;x2=nx-2;
		is_border_face=1;
	}
	if (j==ny)
	{
		y1=ny-1;y2=ny-2;
		is_border_face=1;
	}
	if (k==nz)
	{
		z1=nz-1;z2=nz-2;
		is_border_face=1;
	}
	/* for time being, the coordinates are just for an evenly spaced rectangular
		grid.  This probably should be changed so irregular grids, or curvilinear
		grids by having a node coordinate field */
	/*???MS.  If border get the extra point by extrapolating the point from the
		internal values */
	for (n=0;n<3;n++)
	{
/*    coord[n]=val[n]/((double) texture->dimension[n]) *
			(texture->ximax[n]-texture->ximin[n]) + texture->ximin[n]; */
		coord[n]=2.0 * coordinate_field->vector[3*(x1 + y1*nx + z1*nx*ny)+n] -
			coordinate_field->vector[3*(x2 + y2*nx + z2*nx*ny)+n];
	}
	LEAVE;

	return(is_border_face);
} /* rqc */

static int scalar_interpolation(struct Triangle *triangle,struct Cell_fn *c_fn)
/*******************************************************************************
LAST MODIFIED : 18 February 1998

DESCRIPTION :
calculates interpolated scalar value for trilinear fn, using actual coordinates
returns 1 if unsuccessful
==============================================================================*/
{

int i;
double p_xi[3];
double *xc,*yc,*zc,*fn;


if (triangle && c_fn )
	{

	xc=c_fn->xc;
	yc=c_fn->yc;
	zc=c_fn->zc;
	fn=c_fn->fn;

	/* calculate xi parameters to perform trilinear
	interpolation in f space */

	for (i=0;i<3;i++)   /* vertices of trinangle */
		{
		if (xc[7]==xc[0])
			p_xi[0]=0.5;
		else
			p_xi[0]=(triangle->v[i][0]-xc[0])/(xc[7]-xc[0]);

		if (yc[7]==yc[0])
			p_xi[1]=0.5;
		else
			p_xi[1]=(triangle->v[i][1]-yc[0])/(yc[7]-yc[0]);

		if (zc[7]==zc[0])
			p_xi[2]=0.5;
		else
			p_xi[2]=(triangle->v[i][2]-zc[0])/(zc[7]-zc[0]);

		triangle->trilinear_int[i]=(1.0-p_xi[2])*((1.0-p_xi[1])*((1.0-p_xi[0])*fn[0]+p_xi[0]*fn[1])
			+ p_xi[1]*((1.0-p_xi[0])*fn[2]+p_xi[0]*fn[3]))
			+ p_xi[2]*((1.0-p_xi[1])*((1.0-p_xi[0])*fn[4]+p_xi[0]*fn[5])
			+ p_xi[1]*((1.0-p_xi[0])*fn[6]+p_xi[0]*fn[7]));
		}
	return(0);
	}
else
	{
	printf("ERROR: scalar interpolation , NULL values\n");
	return(1);
	}
} /* scalar_interpolation */

static struct Triangle *make_triangle(double vertices[3][3],int clip_history,
	int clip_history2)
/*******************************************************************************
LAST MODIFIED : 15 October 2001

DESCRIPTION :
Creates and sets up triangle data structure, including plane eqn parameters.
==============================================================================*/
{
	int i,j;
	double v1[3],v2[3];
	struct Triangle *triangle;

	ENTER(make_triangle);
	if (ALLOCATE(triangle,struct Triangle,1))
	{
		for (i=0;i<3;i++)
		{
			for (j=0;j<3;j++)
			{
				triangle->v[i][j]=vertices[i][j];
			}
			/* material at vertex */
		}
		for (i=0;i<3;i++)
		{
			v1[i]=triangle->v[1][i]-triangle->v[0][i];
			v2[i]=triangle->v[2][i]-triangle->v[0][i];
		}
		/* calculate normal */
		if (vectors_equal(v1, v2, AREA_ACC))
		{
			if (debug_flag)
				printf("** ERROR ** make_triangle : equal vectors, no cross product\n");
			DEALLOCATE(triangle);
			return(NULL);
		}
		else
		{
			cross_product3(v1, v2, triangle->n);
			/* ax + by + cz + d = 0 */
			triangle->d=-(triangle->n[0]*triangle->v[0][0]
				+triangle->n[1]*triangle->v[0][1]
				+triangle->n[2]*triangle->v[0][2]);
		}
		triangle->clip_history=clip_history;
		triangle->clip_history2=clip_history2;
		return(triangle);
	}
	else
	{
		printf("*** ERROR *** make_triangle: - memory allocate failed\n");
		return(NULL);
	}
	LEAVE;
}  /* make_triangle */

static void recurse_clip(struct Triangle *triangle,int n_clipping_triangles,
	struct Triangle **clip_triangles,int n_clip)
/*******************************************************************************
LAST MODIFIED : 18 February 1998

DESCRIPTION :
Clips a triangle against a clipping triangle, retrianglates it and recursively
clips the parts. If completely clipped after module global n_clipping_triangles,
resultant triangles are stored in module global clipped_triangle_list.
==============================================================================*/
{
	double cutoff=0;
	int i,j,k;
	struct Triangle *clip_triangle;
	double new_v[3][3];
	int new_m[3];
	double v1[3],v2[3],v3[3],va[3],vb[3],vc[3],q[3],sa,sb,sc;
	/* temporary vertices for splitting triangle */
	double temp_v[9][3],vertex_value[3];
	/* temp_v[9..={(t1) v1,2,3,4,5,6, (t2) v1,2,3}*/
	/* flag for whether edge clipped */
	int edge_count1,edge_count2;
	int clip_case;
	if (debug_flag)
		printf("in recurse_clip. level=%d\n",n_clip);

	if (triangle==NULL)
	{
		if (debug_flag)
			printf("recurse_clip : NULL triangle, aborting\n");
		return;
	}
	new_m[0] = 0;
	new_m[1] = 0;
	new_m[2] = 0;

	/* if no more clipping planes then done */
	/* examine triangle to see if not 1D */
	for (i=0;i<3;i++)
		vertex_value[i]=0;

	if (vectors_equal(triangle->v[0],triangle->v[1],AREA_ACC) || vectors_equal(triangle->v[1],triangle->v[2],AREA_ACC) ||
		vectors_equal(triangle->v[0],triangle->v[2],AREA_ACC) )
	{
		if (debug_flag)
			printf("****** triangle has no area,aborting  ***********\n");
		DEALLOCATE(triangle);
		return;
	}

	if (n_clip >=n_clipping_triangles)
	{
		if (debug_flag)
			printf("n_clip=n_clipping_triangles\n");
		if (debug_flag)
			printf("Triangle->clip_history=%d %d \n",triangle->clip_history,triangle->clip_history2);


		if (triangle->clip_history==-1)
		{
			/* means no clip or meaningful comparison has occurred*/
			/* store triangle in out_list depending on trilinear values at vertices */
			scalar_interpolation(triangle,&cell_fn);
			if (cell_fn.dir*(triangle->trilinear_int[0]+triangle->trilinear_int[1]
					 +triangle->trilinear_int[2])/3.0 > cell_fn.dir*cell_fn.isovalue)
			{
				clipped_triangle_list[n_clipped_triangles]=triangle;
				if (debug_flag)
					printf("++ triangle stored (t_int)=#%d ++++++\n",n_clipped_triangles);

				n_clipped_triangles++;
				return;
			}
			else
			{
				DEALLOCATE(triangle);
				return;
			}
		}

		if (triangle->clip_history==0)
		{
			/* store triangle in out_list and exit */
			clipped_triangle_list[n_clipped_triangles]=triangle;
			if (debug_flag)
				printf("++++++ triangle stored=#%d ++++++\n",n_clipped_triangles);
			if (debug_flag)
			{
				printf("vertices stored: ");
				for (j=0;j<3;j++)
					printf(" %f %f %f / ",triangle->v[j][0],triangle->v[j][1],triangle->v[j][2]);
				printf("\n");
			}

			n_clipped_triangles++;
			return;
		}
		else    /* it has had no intersections, but is below clipping surfaces */
		{
			if (debug_flag)
				printf("not stored\n");
			DEALLOCATE(triangle);
			return;
		}
	}
	else
	{
		/* check for intersection between triangle and clip_triangle */
		triangle->clip_history2=0;
		clip_triangle=clip_triangles[n_clip];

		if (clip_triangle==NULL)
		{
			/* go to next one */
			printf("CLIP_TRIANGLE=NULL\n");
			recurse_clip(triangle,n_clipping_triangles,clip_triangles,n_clip+1);
			return;
		}
		if (debug_flag)
			printf("nclip=%d\n",n_clip);

		for (i=0;i<3;i++)
		{
			for (j=0;j<3;j++)
			{
				temp_v[i][j]=triangle->v[i][j];
			}
		}

		edge_count1=0; edge_count2=0;
		/* go through edges on triangle */
		for (i=0;i<3;i++)
		{
			for (j=0;j<3;j++)
			{
				va[j]=triangle->v[triangle_edge_list[i][0]][j];
				vb[j]=triangle->v[triangle_edge_list[i][1]][j];
				vc[j]=triangle->v[triangle_edge_list[i+1][1]][j];
			}
			if (vectors_equal(va,vb,ACC3))
				printf("^^^^^^^^^^^^^^va=vb\n");

			if (debug_flag)
			{
				printf("va,vb,vc=%f %f %f | %f %f %f | %f %f %f\n",va[0],va[1],va[2],vb[0],vb[1],vb[2],vc[0],vc[1],vc[2]);
				printf("clip_triangle->d %f\n",clip_triangle->d);
				printf("clip_trinagle->n %f %f %f \n",clip_triangle->n[0],clip_triangle->n[1],clip_triangle->n[2]);
				printf("clip_trinagle->v : ");
				for (j=0;j<3;j++)
					printf(" %f %f %f | ",clip_triangle->v[j][0],clip_triangle->v[j][1],clip_triangle->v[j][2]);
				printf("\n");
			}

			/* test to see if on edge of cube */
			if ((va[0]==xc[0] && vb[0]==xc[0]) || (va[0]==xc[7] && vb[0]==xc[7])
				|| (va[1]==yc[0] && vb[1]==yc[0]) || (va[1]==yc[7] && vb[1]==yc[7])
				|| (va[2]==zc[0] && vb[2]==zc[0]) || (va[2]==zc[7] && vb[2]==zc[7]))
			{
				if (debug_flag)
					printf("<<<<<<<<<<<<<Edge on edge of cube>>>>>>>>>>>>>>>>>>\n");
				cutoff=-INTERSECTION_ACC;
			}
			else
				cutoff=INTERSECTION_ACC;
			/* sa=ax + by + cz + d, where x,y,z are coords in triangle
				and a,b,c,d are plane eqn values from the clipping triangle */
			sa=   clip_triangle->n[0]*va[0] + clip_triangle->n[1]*va[1]
				+ clip_triangle->n[2]*va[2] + clip_triangle->d;
			sb=   clip_triangle->n[0]*vb[0] + clip_triangle->n[1]*vb[1]
				+ clip_triangle->n[2]*vb[2] + clip_triangle->d;
			sc=  clip_triangle->n[0]*vc[0] + clip_triangle->n[1]*vc[1]
				+ clip_triangle->n[2]*vc[2] + clip_triangle->d;


			/* store vertex values to decide what parts of triangle to clip */
			vertex_value[i]=sa;
			if (sa==0 && sb==0 && sc==0)
			{
				if (debug_flag)
					printf("@@@@@@@@ sa=sb=sc=0 @@@@@@@@\n");
			}
			if (debug_flag)
				printf("sa,sb=%f %f\n",sa,sb);
			if ((sa >=0 && sb <=0) || (sa <=0 && sb >=0)) /* edges passes through plane of clipping poly */
			{
				if (sa==0 && sb==0) /* an edge of a clipping trinagle lies along the triangle */
				{
					if (debug_flag)
						printf("sa=sb=0 : treating as non_intersecting\n");
					if (sc > 0)   /* >=?? */
					{
						if (debug_flag)
							printf("sa=sb=0: above clip\n");
						triangle->clip_history=1;
						recurse_clip(triangle,n_clipping_triangles,clip_triangles,n_clip+1);
						return;
					}
					else
					{
						if (debug_flag)
							printf("sa=sb=0: above clip\n");
						triangle->clip_history=0;
						recurse_clip(triangle,n_clipping_triangles,clip_triangles,n_clip+1);
						return;
					}
				}
				for (j=0;j<3;j++)
				{
					/* q is the point of intersection of the edge and the clipping triangle plane */
					q[j]=va[j] + sa/(sa-sb) * (vb[j]-va[j]);
					v1[j]=clip_triangle->v[0][j] - q[j];
					v2[j]=clip_triangle->v[1][j] - q[j];
					v3[j]=clip_triangle->v[2][j] - q[j];
					/* store as intermediate vertices 3,4,5 (between 0,1,2) */
					temp_v[i+3][j]=q[j];
				}
				if (scalar_triple_product3(v1,v2,clip_triangle->n) >=cutoff /* intersection is inside triangle */
					&&  scalar_triple_product3(v2,v3,clip_triangle->n) >=cutoff
					&&  scalar_triple_product3(v3,v1,clip_triangle->n) >=cutoff)
				{
					if (debug_flag)
						printf("stp:  %f,%f,%f\n",
							scalar_triple_product3(v1,v2,clip_triangle->n),
							scalar_triple_product3(v2,v3,clip_triangle->n),
							scalar_triple_product3(v3,v1,clip_triangle->n));
					if (scalar_triple_product3(v1,v2,clip_triangle->n)==0
						||  scalar_triple_product3(v2,v3,clip_triangle->n)==0
						||  scalar_triple_product3(v3,v1,clip_triangle->n)==0)
					{
						if (debug_flag)
							printf("one of stp=0\n");
					}
					/* the point of intersection has position vector q */
					/* this is the case were the clip is on an edge of triangle,
						so this will become a new vertex */
					edge_count1++;
				}
			}  /* if */

		}  /* i */
		if (debug_flag)
			printf("edge_count1=%d\n",edge_count1);

		if (edge_count1 < 2)   /* swap triangles to find other edge which passes thru */
		{ /* (there can only be 2 so inly enter if necessary) */
			for (i=0;i<3;i++)
			{
				for (j=0;j<3;j++)
				{
					va[j]=clip_triangle->v[triangle_edge_list[i][0]][j];
					vb[j]=clip_triangle->v[triangle_edge_list[i][1]][j];
				}
				if ((va[0]==xc[0] && vb[0]==xc[0]) || (va[0]==xc[7] && vb[0]==xc[7])
					|| (va[1]==yc[0] && vb[1]==yc[0]) || (va[1]==yc[7] && vb[1]==yc[7])
					|| (va[2]==zc[0] && vb[2]==zc[0]) || (va[2]==zc[7] && vb[2]==zc[7]))
				{
					if (debug_flag)
						printf("<<<<<<<<<<<<<Edge on edge of cube>>>>>>>>>>>>>>>>>>\n");
					cutoff=-INTERSECTION_ACC;
				}
				else
					cutoff=INTERSECTION_ACC;

				/* sa=ax + by + cz + d, where x,y,z are coords in triangle
					and a,b,c,d are plane eqn values from the clipping triangle */
				sa=   triangle->n[0]*va[0] + triangle->n[1]*va[1]
					+ triangle->n[2]*va[2] + triangle->d;
				sb=   triangle->n[0]*vb[0] + triangle->n[1]*vb[1]
					+ triangle->n[2]*vb[2] + triangle->d;

				if ((sa >=0 && sb <=0) || (sa <=0 && sb >=0)) /* edges passes through plane of clipping poly */
				{
					if (debug_flag)
						printf("clip sa,sb=%f %f\n",sa,sb);
					if (sa==0 && sb==0) /* this means clip triangle edeg lies on normal trinagle*/
					{
						/*=> treat as intersecting, set edge_count2 > 0 */
						if (debug_flag)
							printf("clip sa=sb=0, treating as non intersecting \n");
						/*edge_count2++;*/
					}
					else
					{
						for (j=0;j<3;j++)
						{
							q[j]=va[j] + sa/(sa-sb) * (vb[j]-va[j]);
							v1[j]=triangle->v[0][j] - q[j];
							v2[j]=triangle->v[1][j] - q[j];
							v3[j]=triangle->v[2][j] - q[j];
						}
						if (debug_flag)
							printf("stp: clip : %f,%f,%f\n",
								scalar_triple_product3(v1,v2,triangle->n),
								scalar_triple_product3(v2,v3,triangle->n),
								scalar_triple_product3(v3,v1,triangle->n));
						if (scalar_triple_product3(v1,v2,triangle->n) >=cutoff  /* intersection is inside triangle */
							&& scalar_triple_product3(v2,v3,triangle->n) >=cutoff
							&& scalar_triple_product3(v3,v1,triangle->n) >=cutoff
							 )
						{
							if (debug_flag)

								/* the point of intersection has position vector q */
								/* this is the case were the clip is on an edge of triangle,
									so this will become a new vertex */
								if (scalar_triple_product3(v1,v2,triangle->n)==0
									||  scalar_triple_product3(v2,v3,triangle->n)==0
									||  scalar_triple_product3(v3,v1,triangle->n)==0)
								{
									if (debug_flag)
										printf("clip: one of stp=0\n");
								}

							edge_count2++;
						}
					} /* if */
				} /* if sa ,sb >, < 0 && || ... */
			} /* i */
		} /* if edge_count1 < 2 */
		if (debug_flag)
			printf("edge_count2=%d\n",edge_count2);


		if (debug_flag)
		{
			printf("temp_v: \n");
			for (j=0;j<6;j++)
			{
				for (k=0;k<3;k++)
				{
					printf(" %f ",temp_v[j][k]);
				}
				printf("\n");
			}
		}

		/* if edge_count=0=> no intersection, but surface may be completely clipped.
			check by comparing vertex positions to clipping planes. If clip_history > 0, means
			no intersection has been encountered, and it lies below the clipping planes, so therefore
			do not add it to out_list. If it is clipped by another plane, this status is removed */

		if (edge_count1 + edge_count2==0 )
		{
			/* check a vertex to see if under or over clip plane  v >0=> above cut plane=> maybe clip it off*/

			if (vertex_value[0]==0 && vertex_value[1]==0 && vertex_value[2]==0)
			{
				printf("!!!!!!!!! vertex values all equal zero !!!!!!!!!\n");
				/* treat as non_intersecting*/
				if (triangle->clip_history==-1)
				{
					triangle->clip_history=0;
				}


				recurse_clip(triangle,n_clipping_triangles,clip_triangles,n_clip+1);
				return;

			}
			if (vertex_value[0]>=0 && vertex_value[1]>=0 && vertex_value[2] >=0)
			{
				if (debug_flag)
					printf(" v.v >=0, non_intersecting (above clip plane)\n");

				/* only change if unaltered by any other polygon */
				if (triangle->clip_history==-1)
				{
					triangle->clip_history=1;
				}
				recurse_clip(triangle,n_clipping_triangles,clip_triangles,n_clip+1);
				return;
			}
			else
			{
				if (debug_flag)
					printf(" v.v < 0,non_intersecting (below clip plane)\n");

				if (triangle->clip_history==-1)
				{
					triangle->clip_history=0;
				}

				recurse_clip(triangle,n_clipping_triangles,clip_triangles,n_clip+1);
				return;
			}
		}
		else
		{
			if (debug_flag)
				printf("edge_count1+2 > 0 (=%d+%d)=> some kind of clip to occur\n",edge_count1,edge_count2);
			if (edge_count1 + edge_count2 < 2)  /* if non_sensible value, get out */
			{
				/*recurse_clip(triangle,n_clipping_triangles,clip_triangles,n_clip+1);
				  return;*/
			}
			triangle->clip_history=0 ;    /* some kind of clip to occur */

			/* check vertex_values. If two are below clipping plane the we get one resulting
				triangle. If one only then retriangulate resulting quadrilateral */
			clip_case=0;
			if (vertex_value[0] >=0)
				clip_case +=1;
			if (vertex_value[1] >=0)
				clip_case +=2;
			if (vertex_value[2] >=0)
				clip_case +=4;
			if (debug_flag)
				printf("vertex_values (%f,%f,%f) clip_case %d\n",vertex_value[0],vertex_value[1],vertex_value[2],clip_case);

			if (clip_case >6 || clip_case <1)
				if (debug_flag)
					printf("$$$$$$$$$$$ HEINIOUS ERROR DUDE - CLIP CASE !!!!\n");
			if (clip_case==7)
			{
				triangle->clip_history=1;
				recurse_clip(triangle,n_clipping_triangles,clip_triangles,n_clip+1);
				return;
			}
			if (clip_case==0)
			{
				triangle->clip_history=0;
				recurse_clip(triangle,n_clipping_triangles,clip_triangles,n_clip+1);
				return;
			}
			for (i=0;i<retriangulation_list[clip_case][0];i++)
			{
				for (j=0;j<3;j++)
				{
					for (k=0;k<3;k++)
					{
						new_v[j][k]=
							temp_v[retriangulation_list[clip_case][j+1+3*i]-1][k];
						/* +1 to avoid count bit, -1 as list has counting # indices */
					}
				}
				if (debug_flag)
					printf("below clip plane \n");

				recurse_clip(make_triangle(new_v,0,0),n_clipping_triangles,clip_triangles,n_clip+1);
			} /* i*/

			if (debug_flag)
				printf("processing part above clip plane\n");

			for (i=0;i<retriangulation_list[7-clip_case][0];i++)
			{
				for (j=0;j<3;j++)
				{
					for (k=0;k<3;k++)
					{
						/* take complement */
						new_v[j][k]=
							temp_v[retriangulation_list[7-clip_case][j+1+3*i]-1][k];
						/* +1 to avoid count bit, -1 as list has counting # indices */
					}
				}
				/* taint triangle as could be totally severed already */
				if (debug_flag)
					printf("above clip plane\n");
				if (debug_flag)
					printf("new_m=%d %d %d  (k=%d)\n",new_m[0],new_m[1],new_m[2],k);

				recurse_clip(make_triangle(new_v,1,1),n_clipping_triangles,clip_triangles,n_clip+1);
			}


			/* deallocate memory assoc with triangle */
			if (triangle)
				DEALLOCATE(triangle);

		} /* if edge_count */
	} /* if nclip */
} /* recurse_clip */

/*
Global functions
----------------
*/

int marching_cubes(struct VT_scalar_field **scalar_field,int n_scalar_fields,
	struct VT_vector_field *coordinate_field,
	struct MC_iso_surface *mc_iso_surface,
	double *isovalue,int closed_surface,int cutting_plane_on)
/*******************************************************************************
LAST MODIFIED : 28 October 2005

DESCRIPTION :
The modified marching cubes algorithm for constructing isosurfaces.  This
constructs an isosurface for specified value from a filtered volume texture.
The nodal values are obtained by a weighting of the surrounding cells.  These
are used to create an isosurface, and a polygon list is created.  Normals are
calculated using central or one sided differences at each node.  If
closed_surface, the iso surface generated contains closed surfaces along
intersections with the boundary.
==============================================================================*/
{
	int return_code;
	/*???debug */
#if defined (UNIX)
#if defined (DEBUG_CODE)
	struct tms buffer;
	double real_time1,real_time2,real_time3,real_time4,cpu_time1,cpu_time2,
		cpu_time3,cpu_time4;
#endif /* defined (DEBUG_CODE) */
	/* clock_ticks=100 */
#endif
	/* mc array size=2 bigger than standard array nx,ny,nz to account for closed surfaces */
	int mcnx, mcny, mcnz;
	int x_min, x_max, y_min, y_max, z_min, z_max;
	int list_index = 0;
	int a_count, a_count2;
	int a,b;
	int nx,ny,nz;
	int i,j,k,m,n,nn,ii;
	/* the mc case (1-256) */
	int icase[MAX_SCALAR_FIELDS];
	int iambig[MAX_SCALAR_FIELDS],npolyg[MAX_SCALAR_FIELDS];
	/* size of mesh */

	/* pointer to table values */
	int ipntr[MAX_SCALAR_FIELDS];
	int istep[MAX_SCALAR_FIELDS],iedge[MAX_SCALAR_FIELDS];
	/* nodes on either side of contour */
	int node1,node2;
	/* scalar value at ambiguous node */
	double cnrscl;
	double sdiff;
	double acc=0.000001;
	/* normalised contour value */
	double p;
	/* coords of contour/edge intersection */
	double xw,yw,zw;
	/* scalar gradients at nodes */
	/*static double dx[8],dy[8],dz[8];*/
	/* returned double contour gradient */
	double grads;
	/* triangle vertex variables */
	double v[6][3],v_out[MAX_SCALAR_FIELDS][MAX_INTERSECTION][3][3],
		v_out2[MAX_SCALAR_FIELDS][MAX_INTERSECTION][3][3];
	double coord[3];
	/* number of triangles after triangle clipped */
	int n_triangles[MAX_SCALAR_FIELDS],n_triangles2;
	int clip_flag;
	/* temp storage for polygons generated by iso surface routine
		before reordering as vertex/triangle list */

	/* lists specific to border faces */
	/*int n_xi_face_polys[6]={0,0,0,0,0,0};*/

	/* added detail */
	int edge[3],edge_total,ihmax,mm,n_temp_triangles,res;
	double h[3],hmax,temp_vout[MAX_INTERSECTION*4][3][3];
	struct Triangle **clip_triangles;
	struct Triangle *new_triangle;
	int border_flag=0;

	double maxc[3],minc[3],tempc;

	/* centre of projection for mapped textures */
	struct MC_cell *mc_cell;
	ZnReal mc_vertices[3][3];

	ENTER(marching_cubes);
#if defined (DEBUG_CODE)
	/*???debug */
	printf("enter marching_cubes\n");
#endif /* defined (DEBUG_CODE) */
	/* check arguments */
	if (scalar_field&&(*scalar_field)&&(0<n_scalar_fields)&&coordinate_field&&
		mc_iso_surface&&isovalue)
	{
#if defined (DEBUG_CODE)
		/*???debug */
		printf("iso_values %d:\n",n_scalar_fields);
		for (i=0;i<n_scalar_fields;i++)
		{
			printf("%d %lg\n",i,isovalue[i]);
		}
#endif /* defined (DEBUG_CODE) */
		return_code=1;
#if defined (DEBUG_CODE)
#if defined (UNIX)
		/*???debug */
		/* diagnostics */
		real_time1=((double)times(&buffer))/100.0;
		cpu_time1=((double)buffer.tms_utime)/100.0;
		printf("*************  real time=%lf    CPU time=%lf *************\n",
			real_time1,cpu_time1);
#endif /* defined (UNIX) */
#endif /* defined (DEBUG_CODE) */
		mc_iso_surface->n_scalar_fields=n_scalar_fields;
#if defined (DEBUG_CODE)
		/*???debug */
		printf("1: #MC TRIANGLES=%d  #MC VERTICES=%d\n",mc_iso_surface->n_triangles,
			mc_iso_surface->n_vertices);
#endif /* defined (DEBUG_CODE) */
		/* get nx,ny,nz values from field */
		mcnx=mc_iso_surface->dimension[0]+2;
		mcny=mc_iso_surface->dimension[1]+2;
		mcnz=mc_iso_surface->dimension[2]+2;
		nx=scalar_field[0]->dimension[0];
		ny=scalar_field[0]->dimension[1];
		nz=scalar_field[0]->dimension[2];
#if defined (DEBUG_CODE)
		/*???debug */
		printf("nx,ny,nz=%d, %d, %d\n",nx,ny,nz);
#endif /* defined (DEBUG_CODE) */

		/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!temp MC */
		if (closed_surface)
		{
			if (mc_iso_surface->active_block[0]<0)
			{
				mc_iso_surface->active_block[0]=0;
			}
			if (mc_iso_surface->active_block[1]>nx)
			{
				mc_iso_surface->active_block[1]=nx;
			}
			if (mc_iso_surface->active_block[2]<0)
			{
				mc_iso_surface->active_block[2]=0;
			}
			if (mc_iso_surface->active_block[3]>ny)
			{
				mc_iso_surface->active_block[3]=ny;
			}
			if (mc_iso_surface->active_block[4]<0)
			{
				mc_iso_surface->active_block[4]=0;
			}
			if (mc_iso_surface->active_block[5]>nz)
			{
				mc_iso_surface->active_block[5]=nz;
			}
		}
		else
		{
			if (mc_iso_surface->active_block[0]<=0)
			{
				mc_iso_surface->active_block[0]=1;
			}
			if (mc_iso_surface->active_block[1]>nx)
			{
				mc_iso_surface->active_block[1]=nx;
			}
			if (mc_iso_surface->active_block[2]<=0)
			{
				mc_iso_surface->active_block[2]=1;
			}
			if (mc_iso_surface->active_block[3]>ny)
			{
				mc_iso_surface->active_block[3]=ny;
			}
			if (mc_iso_surface->active_block[4]<=0)
			{
				mc_iso_surface->active_block[4]=1;
			}
			if (mc_iso_surface->active_block[5]>nz)
			{
				mc_iso_surface->active_block[5]=nz;
			}
		}
		/*printf("mc_active_block=(%d, %d), (%d, %d), (%d, %d)\n", mc_iso_surface->active_block[0],
		  mc_iso_surface->active_block[1], mc_iso_surface->active_block[2], mc_iso_surface->active_block[3],
		  mc_iso_surface->active_block[4], mc_iso_surface->active_block[5]);*/
		/*
		  mc_iso_surface->active_block[0]=1-closed_surface;
		  mc_iso_surface->active_block[1]=nx+closed_surface;
		  mc_iso_surface->active_block[2]=1-closed_surface;
		  mc_iso_surface->active_block[3]=ny+closed_surface;
		  mc_iso_surface->active_block[4]=1-closed_surface;
		  mc_iso_surface->active_block[5]=nz+closed_surface;
		*/

		if (coordinate_field->dimension[0] !=scalar_field[0]->dimension[0]
			|| coordinate_field->dimension[1] !=scalar_field[0]->dimension[1]
			|| coordinate_field->dimension[2] !=scalar_field[0]->dimension[2] )
		{
			printf("ERROR - coordinate field does not match scalar field\n");
		}

		if (cutting_plane_on)
		{
			for (i=0;i<n_scalar_fields;i++)
			{
				if (scalar_field[i]->dimension[0] !=scalar_field[0]->dimension[0]
					|| scalar_field[i]->dimension[1] !=scalar_field[0]->dimension[1]
					|| scalar_field[i]->dimension[2] !=scalar_field[0]->dimension[2] )
				{
					printf("ERROR - scalar field[%d] does not match scalar field[0]\n",i);
				}
			}
		}


		x_min=mc_iso_surface->active_block[0];
		x_max=mc_iso_surface->active_block[1];
		y_min=mc_iso_surface->active_block[2];
		y_max=mc_iso_surface->active_block[3];
		z_min=mc_iso_surface->active_block[4];
		z_max=mc_iso_surface->active_block[5];
		if (closed_surface)
		{
			if (1==x_min)
				x_min=0;
			if (1==y_min)
				y_min=0;
			if (1==z_min)
				z_min=0;
			if (nx==x_max)
				x_max +=1;
			if (ny==y_max)
				y_max +=1;
			if (nz==z_max)
				z_max +=1;
			nx++;
			ny++;
			nz++;
			/*x0=y0=z0=0;*/
		}

		/* store bounds of cube (for closed surface squishing) */
		if (closed_surface)
		{
			rqc(coordinate_field,nx-1,ny-1,nz-1,maxc);
			rqc(coordinate_field,0,0,0,minc);
			/* deal with different +ve axis dirn */
			for (i=0;i<3;i++)
			{
				/*???debug */
				/*printf("min=%g, max=%g\n",minc[i],maxc[i]);*/
				if (maxc[i] < minc[i])
				{
					tempc=minc[i];
					minc[i]=maxc[i];
					maxc[i]=tempc;
				}
			}
		}

		/* delete mc active block first */
		for (k=z_min;k<=z_max;k++)
		{
			for (j=y_min;j<=y_max;j++)
			{
				for (i=x_min;i<=x_max;i++)
				{
					/* clear mc_cell triangles and vertices! */
					if (mc_iso_surface->mc_cells !=NULL)
					{
						if (mc_iso_surface->mc_cells[i+j*mcnx+k*mcnx*mcny] !=NULL)
						{
							destroy_mc_triangle_list(mc_iso_surface->mc_cells[i+j*mcnx+k*mcnx*mcny],
								mc_iso_surface->mc_cells[i+j*mcnx+k*mcnx*mcny]->triangle_list,
								mc_iso_surface->mc_cells[i+j*mcnx+k*mcnx*mcny]->n_triangles,  n_scalar_fields, mc_iso_surface);
							mc_iso_surface->mc_cells[i+j*mcnx+k*mcnx*mcny]->triangle_list=NULL;
							mc_iso_surface->mc_cells[i+j*mcnx+k*mcnx*mcny]->n_triangles=NULL;
							mc_cell=mc_iso_surface->mc_cells[i+j*mcnx+k*mcnx*mcny];
							DEALLOCATE(mc_cell);
							mc_iso_surface->mc_cells[i+j*mcnx+k*mcnx*mcny]=NULL;
						}
					}
				}
			}
		}
#if defined (DEBUG_CODE)
		/*???debug */
		printf("2: #MC TRIANGLES=%d  #MC VERTICES=%d\n",mc_iso_surface->n_triangles,
			mc_iso_surface->n_vertices);
#endif /* defined (DEBUG_CODE) */
		if ((mc_iso_surface->n_triangles<0)||(mc_iso_surface->n_vertices<0))
		{
			display_message(ERROR_MESSAGE,"n_triangles or n_vertices < 0");
			mc_iso_surface->n_triangles=0;
			mc_iso_surface->n_vertices=0;
		}
		for (k=z_min;k<=z_max;k++)
		{
			for (j=y_min;j<=y_max;j++)
			{
				for (i=x_min;i<=x_max;i++)
				{
#if defined (DEBUG_CODE)
					/*???debug */
					if (i==debug_i && j==debug_j && k==debug_k)
					{
						debug_flag=1;
					}
					else
					{
						debug_flag=0;
					}
					if (debug_flag)
					{
						printf("----x,y,z=%d %d %d ----\n",i,j,k);
					}
#endif /* defined (DEBUG_CODE) */
					/* get coords of nodes & scalar values for current cell */
					/* get colours too? */
					n_triangles2=0;
					/* get scalars */
					for (n=0;n<n_scalar_fields;n++)
					{
						fc[n][0]=rqs(scalar_field[n],i-1,j-1,k-1);
						fc[n][1]=rqs(scalar_field[n],i,j-1,k-1);
						fc[n][2]=rqs(scalar_field[n],i-1,j,k-1);
						fc[n][3]=rqs(scalar_field[n],i,j,k-1);
						fc[n][4]=rqs(scalar_field[n],i-1,j-1,k);
						fc[n][5]=rqs(scalar_field[n],i,j-1,k);
						fc[n][6]=rqs(scalar_field[n],i-1,j,k);
						fc[n][7]=rqs(scalar_field[n],i,j,k);
#if defined (DEBUG_CODE)
						/*???debug */
						/* printf("%d %d %d %d  %g %g %g %g %g %g %g %g\n",i,j,k,n,fc[n][0], */
/* 							fc[n][1],fc[n][2],fc[n][3],fc[n][4],fc[n][5],fc[n][6],fc[n][7]); */
#endif /* defined (DEBUG_CODE) */
					}
					/* get coords */
					/* border flag
						0 : part of normal iso surface
						4 : face
						6 : edge
						7 : corner
						8 : 1D in one dirn */
					border_flag=0;
					if (rqc(coordinate_field,i-1,j-1,k-1,coord))
					{
						border_flag++;
					}
					xc[0]=coord[0];
					yc[0]=coord[1];
					zc[0]=coord[2];
					if (rqc(coordinate_field,i,j-1,k-1,coord))
					{
						border_flag++;
					}
					xc[1]=coord[0];
					yc[1]=coord[1];
					zc[1]=coord[2];
					if (rqc(coordinate_field,i-1,j,k-1,coord))
					{
						border_flag++;
					}
					xc[2]=coord[0];
					yc[2]=coord[1];
					zc[2]=coord[2];
					if (rqc(coordinate_field,i,j,k-1,coord))
					{
						border_flag++;
					}
					xc[3]=coord[0];
					yc[3]=coord[1];
					zc[3]=coord[2];
					if (rqc(coordinate_field,i-1,j-1,k,coord))
					{
						border_flag++;
					}
					xc[4]=coord[0];
					yc[4]=coord[1];
					zc[4]=coord[2];
					if (rqc(coordinate_field,i,j-1,k,coord))
					{
						border_flag++;
					}
					xc[5]=coord[0];
					yc[5]=coord[1];
					zc[5]=coord[2];
					if (rqc(coordinate_field,i-1,j,k,coord))
					{
						border_flag++;
					}
					xc[6]=coord[0];
					yc[6]=coord[1];
					zc[6]=coord[2];
					if (rqc(coordinate_field,i,j,k,coord))
					{
						border_flag++;
					}
					xc[7]=coord[0];
					yc[7]=coord[1];
					zc[7]=coord[2];
					/* calculate case number */
					for (m=0;m<n_scalar_fields;m++)
					{
						icase[m]=1;
						for (n=1;n<=8;n++)
						{
							if (fc[m][n -1]>isovalue[m])
							{
								icase[m] +=ibin[n -1];
							}
						}
					}
					/* look thru all clip fields */
					clip_flag=0;
					if ((icase[0]>1)&&(icase[0]<256))
					{
						/* The first scalar is the isosurface we are generating */
						clip_flag = 1;
					}
					for (m=1;m<n_scalar_fields;m++)
					{
						/* We are totally clipped by one of the other isosurfaces */
						if (icase[m]==1)
						{
							clip_flag=0;
						}
					}
					/* dont need to consider cases wholly inside or outside surface, or
						wholly clipped */
					/* if [ (not out & not in & not fully clipped) OR (in and partially
						clipped) ] */
					if (clip_flag)
					{
#if defined (DEBUG_CODE)
						/*???debug */
						/* printf("%d %d %d clip_flag\n",i,j,k); */
#endif /* defined (DEBUG_CODE) */
						/* see if cell has alternative polygon
							configurations */
						for (m=0;m<n_scalar_fields;m++)
						{
							iambig[m]=mctbl[1 -1][icase[m] -1];
							npolyg[m]=mctbl[2 -1][icase[m] -1];
							if (iambig[m]==0)
							{
								/* no double contours */
								/*index[m]=icase[m];*/
								ipntr[m]=3;
								istep[m]=1;
							}
							else
							{
								/*!!! check pointer stuff here */
								/* double contours */
								dblcon(scalar_field[m],nx,ny,nz,i,j,k,iambig[m],isovalue[m],
									&grads);
								cnrscl=fc[m][facnod[iambig[m] -1][1 -1]];
								if (((cnrscl<isovalue[m])&&(grads>0))||
									((cnrscl>isovalue[m])&&(grads<0)))
								{
									ipntr[m]=3;
									istep[m]=1;
								}
								else
								{
									icase[m]=257-icase[m];
									npolyg[m]=mctbl[2 -1][icase[m] -1];
									ipntr[m]=3*npolyg[m] + 2;
									istep[m]=-1;
								}
							}
						}
						/* get the edge indices and perform the interpolation. Iterate
							through all the polygons in the cell */
						n_triangles2=0;
						for (a=0;a<n_scalar_fields;a++)
						{
							n_triangles[a]=0;
							for (m=1;m<=npolyg[a];m++)
							{
								for (n=1;n<=3;n++)  /* x,y,z */
								{
									iedge[a]=mctbl[ipntr[a] -1][icase[a] -1];
									ipntr[a]=ipntr[a]+istep[a];
									node1=nodarr[iedge[a] -1][1 -1];
									node2=nodarr[iedge[a] -1][2 -1];
									sdiff=fc[a][node2 -1]-fc[a][node1 -1];
									if (fabs(sdiff)<acc)
									{
										p=0.5;
									}
									else
									{
										p=(isovalue[a] - fc[a][node1 -1])/sdiff;
									}
									if ((p<0.0)||(p>1.0))
									{
#if defined (DEBUG_CODE)
#endif /* defined (DEBUG_CODE) */
										/*???debug */
										printf("iso error in case %d\n",icase[a]);
										printf("p=%lf\n",p);
									}
									xw=xc[node1 -1]+p*(xc[node2 -1]-xc[node1 -1]);
									yw=yc[node1 -1]+p*(yc[node2 -1]-yc[node1 -1]);
									zw=zc[node1 -1]+p*(zc[node2 -1]-zc[node1 -1]);
									/* make polygons for rendering */
									v[n-1][0]=xw;
									v[n-1][1]=yw;
									v[n-1][2]=zw;
								} /*n*/
								/*???MS.  Should this go before the above? */
								if ((border_flag!=6)&&(border_flag!=7))
								{
									/* ignore infinitesimal corners & edges as these do not
										contribute to the image, and would stuff up the averages for
										the normals on the faces */
									for (n=0;n<3;n++)
									{
										for (nn=0;nn<3;nn++)
										{
											v_out2[a][n_triangles[a]][2-n][nn]=v[n][nn];
											v_out[a][n_triangles[a]][2-n][nn]= v[n][nn];
										}
									}
									/* only store if triangle has area */
									if (vectors_equal(v_out[a][n_triangles[a]][0],
											v_out[a][n_triangles[a]][1],AREA_ACC)||
										vectors_equal(v_out[a][n_triangles[a]][1],
											v_out[a][n_triangles[a]][2],AREA_ACC)||
										vectors_equal(v_out[a][n_triangles[a]][0],
											v_out[a][n_triangles[a]][2],AREA_ACC))
									{
#if defined (DEBUG_CODE)
#endif /* defined (DEBUG_CODE) */
										if (debug_flag)
										{
											printf("VECTORS EQUAL for triangle\n");
										}
									}
									else
									{
										n_triangles[a]++;
									}
								} /* if border_flag !=6 or 7 */
							} /*m*/
#if !defined (DO_NOT_ADD_DETAIL)
							/*???MS.  Detail begin */
							/***************************** add detail ************************************/
							if (mc_iso_surface->detail_map)
							{
								for (res=0;res<mc_iso_surface->detail_map[i+j*mcnx+k*mcnx*mcny];
									  res++)
								{
									n_temp_triangles=0;
									for (m=0;m<n_triangles[a];m++)
									{
										/* classify edges as internal (internal within cell) or
											boundary internal (internal across cells) */
										for (n=0;n<3;n++)
										{
											for (nn=0;nn<3;nn++)
											{
												v[n][nn]=v_out2[a][m][n][nn];
											}
											edge[n]=0;
										}
										/* compare edges v[0]_v[1],v[1]_v[2],v[2]_v[0] */
										/* 1. check internal */
										/* rotate so that the hypotenuse is always edge 0 */
										h[0]=0;
										h[1]=0;
										h[2]=0;
										hmax=0;
										ihmax=0;
										for (nn=0;nn<3;nn++)
										{
											h[0] += (v[1][nn]-v[0][nn])*(v[1][nn]-v[0][nn]);
											h[1] += (v[2][nn]-v[1][nn])*(v[2][nn]-v[1][nn]);
											h[2] += (v[0][nn]-v[2][nn])*(v[0][nn]-v[2][nn]);
										}
										for (n=0;n<3;n++)
										{
											if (h[n]>hmax)
											{
												hmax=h[n];
												ihmax=n;
											}
										}
										switch (ihmax)
										{
											case 1:
											{
												for (nn=0;nn<3;nn++)
												{
													v[0][nn]=v_out2[a][m][1][nn];
													v[1][nn]=v_out2[a][m][2][nn];
													v[2][nn]=v_out2[a][m][0][nn];
												}
											} break;
											case 2:
											{
												for (nn=0;nn<3;nn++)
												{
													v[0][nn]=v_out2[a][m][2][nn];
													v[1][nn]=v_out2[a][m][0][nn];
													v[2][nn]=v_out2[a][m][1][nn];
												}
											} break;
										}
										for (n=0;n<3;n++)
										{
											for (nn=0;nn<3;nn++)
											{
												v_out2[a][m][n][nn]=v[n][nn];
											}
										}
										for (mm=0;mm<n_triangles[a];mm++)
										{
											if (mm != m)
											{
												if (shared_edge(v[0],v[1],v_out2[a][mm]))
												{
													edge[0]=1;
												}
												if (shared_edge(v[1],v[2],v_out2[a][mm]))
												{
													edge[1]=1;
												}
												if (shared_edge(v[2],v[0],v_out2[a][mm]))
												{
													edge[2]=1;
												}
											}
										}
										/* 2. check iboundary internal */
										if (internal_boundary(v[0],v[1],xc,yc,zc,i,j,k,
												mc_iso_surface->detail_map,mcnx,mcny,mcnz,res))
										{
											edge[0]=1;
										}
										if (internal_boundary(v[1],v[2],xc,yc,zc,i,j,k,
												mc_iso_surface->detail_map,mcnx,mcny,mcnz,res))
										{
											edge[1]=1;
										}
										if (internal_boundary(v[2],v[0],xc,yc,zc,i,j,k,
												mc_iso_surface->detail_map,mcnx,mcny,mcnz,res))
										{
											edge[2]=1;
										}
										/* calculate detail table index */
										edge_total=edge[0]+2*edge[1]+4*edge[2];
										/* calculate new bisected vertices */
										for (nn=0;nn<3;nn++)
										{
											v[3][nn]=(v[0][nn]+v[1][nn])/2.0;
											v[4][nn]=(v[1][nn]+v[2][nn])/2.0;
											v[5][nn]=(v[2][nn]+v[0][nn])/2.0;
										}
										/* replace trinagles using table */
										for (mm=0;mm<detail_table[edge_total][0];mm++)
										{
											for (n=0;n<3;n++)
											{
												for (nn=0;nn<3;nn++)
												{
													temp_vout[n_temp_triangles][n][nn]=
														v[detail_table[edge_total][n+1+mm*3]][nn];
												}
											}
											n_temp_triangles++;
										}
									} /* m */
									for (m=0;m<n_temp_triangles;m++)
									{
										for (n=0;n<3;n++)
										{
											for (nn=0;nn<3;nn++)
											{
												v_out2[a][m][n][nn]=temp_vout[m][n][nn];
											}
										}
									}
									n_triangles[a]=n_temp_triangles;
								} /* detail */
							}
							/*???MS.  Detail end */
#endif /* !defined (DO_NOT_ADD_DETAIL) */
						} /* a */
						/* output in poly lists */
						if ((border_flag!=6)&&(border_flag!=7))
						{
							/* ignore infinitesimal corners & edges as these do not contribute
								to the image, and would stuff up the averages for the normals on
								the faces */
							/* we successively clip field a with fields b */
							/* allocate MC_cell triangle list structures */
							/* create mc_cell if not already created */
#if defined (DEBUG_CODE)
							/*???debug */
							/* printf("----------- Allocating MC_cell %d %d %d --------------\n", */
/* 								i,j,k); */
#endif /* defined (DEBUG_CODE) */
							if (mc_iso_surface->mc_cells[i+j*mcnx+k*mcnx*mcny] !=NULL)
							{
#if defined (DEBUG_CODE)
#endif /* defined (DEBUG_CODE) */
								/*???debug */
								printf("had to delete previous entry\n");
								/* deletes cells not deleted if closed surface on */
								destroy_mc_triangle_list(
									mc_iso_surface->mc_cells[i+j*mcnx+k*mcnx*mcny],
									mc_iso_surface->mc_cells[i+j*mcnx+k*mcnx*mcny]->triangle_list,
									mc_iso_surface->mc_cells[i+j*mcnx+k*mcnx*mcny]->n_triangles,
									n_scalar_fields,mc_iso_surface);
								mc_iso_surface->mc_cells[i+j*mcnx+k*mcnx*mcny]->triangle_list=
									NULL;
								mc_iso_surface->mc_cells[i+j*mcnx+k*mcnx*mcny]->n_triangles=
									NULL;
								mc_cell=mc_iso_surface->mc_cells[i+j*mcnx+k*mcnx*mcny];
								DEALLOCATE(mc_cell);
								mc_iso_surface->mc_cells[i+j*mcnx+k*mcnx*mcny]=NULL;
							}
							if (NULL==mc_iso_surface->mc_cells[i+j*mcnx+k*mcnx*mcny])
							{
								if (ALLOCATE(mc_cell, struct MC_cell, 1))
								{
									mc_iso_surface->mc_cells[i+j*mcnx+k*mcnx*mcny]=mc_cell;
								}
								else
								{
									display_message(ERROR_MESSAGE,"ALLOC failed for mc_cell");
									mc_iso_surface->mc_cells[i+j*mcnx+k*mcnx*mcny]=NULL;
								}
								if (mc_cell)
								{
									mc_cell->index[0]=i;
									mc_cell->index[1]=j;
									mc_cell->index[2]=k;
									/* cell(i,j,k)=mc_cells[i+j*mcnx+k*mcnx*mcny] */
									if (ALLOCATE(mc_cell->triangle_list,struct MC_triangle **,
											n_scalar_fields+6)&&ALLOCATE(mc_cell->n_triangles,int,
												n_scalar_fields+6))
									{
										for (ii=0;ii < n_scalar_fields+6;ii++)
										{
											mc_cell->n_triangles[ii]=0;
											mc_cell->triangle_list[ii]=NULL;
										}
									}
									else
									{
										display_message(ERROR_MESSAGE,
											"ALLOC failed for mc_cell triangle list");
									}
								}
							}
							else
							{
								display_message(ERROR_MESSAGE,"Delete failed for mc_cell");
							}
								a=0;
								n_triangles2=n_triangles[a];
								/* n_triangles[a] represents the polys before clipping,
									n_triangles2 after.  v_out2 contains the triangles after they
									have been clipped by each field */
								for (b=0;b<n_scalar_fields;b++)
								{
									if (a!=b)
									{
										if ((icase[a]>1)&&(icase[a]<256)&&(icase[b]>1)&&
											(icase[b]<256))
										{
											cell_fn.xc=xc;
											cell_fn.yc=yc;
											cell_fn.zc=zc;
											cell_fn.fn=fc[b];
											cell_fn.isovalue=isovalue[b];
											/* 1.0 ???? */        cell_fn.dir= 1.0;
											n_clipped_triangles=0;
											/* have intersecting isosurfaces in cell, so clip */
											/* assemble all clipping triangles in list */
											if ((0<n_triangles[b])&&ALLOCATE(clip_triangles,
													struct Triangle *,n_triangles[b]))
											{
												for (n=0;n<n_triangles[b];n++)
												{
													clip_triangles[n]=make_triangle(v_out[b][n],0,0);
												}
												for (n=0;n<n_triangles2;n++)
												{
													new_triangle=make_triangle(v_out2[a][n],-1,0);
													recurse_clip(new_triangle,n_triangles[b],
														clip_triangles,0);
												}
												for (n=0;n<n_triangles[b];n++)
												{
													if (clip_triangles[n])
													{
														DEALLOCATE(clip_triangles[n]);
													}
												}
												DEALLOCATE(clip_triangles);
											}
											/* outputs n_clipped_triangles into
												clipped_triangle_list */
#if defined (DEBUG_CODE)
#endif /* defined (DEBUG_CODE) */
											/*???debug */
											if (n_clipped_triangles >=MAX_INTERSECTION)
											{
												printf("#### ERROR n_clipped_triangles=%d ####\n",
													n_clipped_triangles);
											}
											n_triangles2=n_clipped_triangles;
											for (m=0;m<n_triangles2;m++)
											{
												for (n=0;n<3;n++)
												{
													for(nn=0;nn<3;nn++)
													{
														v_out2[a][m][n][nn]=
															clipped_triangle_list[m]->v[n][nn];
													}
												}
												if (clipped_triangle_list[m])
												{
													DEALLOCATE(clipped_triangle_list[m]);
												}
											}
										} /* if icase */
									} /* if a !=b */
								} /* b */
								/* choose which list to use */
								if ((border_flag==0)||(!closed_surface))
								{
									/* standard iso surface */
									list_index=a+6;
								}
								else
								{
									if ((border_flag==4)||(border_flag==8))
									{
										/* face or 1D */
										if (v[0][0]+v[1][0]+v[2][0]<3*minc[0])
										{
											/* x bottom face */
											list_index=0;
										}
										else
										{
											if (v[0][0]+v[1][0]+v[2][0]>3*maxc[0])
											{
												/* x top face */
												list_index=1;
											}
											else
											{
												if (v[0][1]+v[1][1]+v[2][1]<3*minc[1])
												{
													/* y bot */
													list_index=2;
												}
												else
												{
													if(v[0][1]+v[1][1]+v[2][1]>3*maxc[1] )
													{
														/* y top */
														list_index=3;
													}
													else
													{
														if (v[0][2]+v[1][2]+v[2][2]<3*minc[2])
														{
															/* z bot */
															list_index=4;
														}
														else
														{
															if (v[0][2]+v[1][2]+v[2][2]>3*maxc[2])
															{
																/* z top */
																list_index=5;
															}
															else
															{
#if defined (DEBUG_CODE)
#endif /* defined (DEBUG_CODE) */
																/*???debug */
																printf("****** ERROR : polygon does not lie on face\n");
															}
														}
													}
												}
											}
										}
									}
									else
									{
										printf("########## error border_flag=%d\n",border_flag);
									}
								}
								for (m=0;m<n_triangles2;m++)
								{
									for (n=0;n<3;n++)
									{
										for (nn=0;nn<3;nn++)
										{
											if (closed_surface)
											{
												/* squish to fit inside cube (for closed surface)*/
												if (v_out2[a][m][2-n][nn] > maxc[nn])
												{
													v_out2[a][m][2-n][nn]=maxc[nn];
												}
												if (v_out2[a][m][2-n][nn] < minc[nn])
												{
													v_out2[a][m][2-n][nn]=minc[nn];
												}
											}
											mc_vertices[n][nn]=(ZnReal) v_out2[a][m][2-n][nn];
										}
									}
									if (mc_iso_surface->mc_cells[i+j*mcnx+k*mcnx*mcny]==NULL)
									{
#if defined (DEBUG_CODE)
#endif /* defined (DEBUG_CODE) */
										/*???debug */
										printf("****** ERROR ****** cell (%d %d %d)=NULL\n",i,j,k);
									}
									add_mc_triangle(i,j,k,mc_iso_surface,list_index,mc_vertices,
										x_min,x_max,y_min,y_max,z_min,z_max,mcnx,mcny,mcnz,
										n_scalar_fields+6);
								} /* for m < n_triangles2 */
						} /* if border flag !=6 or 7*/
					} /*if clip_flag */
				} /*i*/
			} /*j*/
		} /*k*/
#if defined (DEBUG_CODE)
#if defined (UNIX)
		/*???debug */
		real_time2=((double)times(&buffer))/100.0;
		cpu_time2= ((double)buffer.tms_utime)/100.0;
#endif
#endif /* defined (DEBUG_CODE) */
#if defined (DEBUG_CODE)
		/*???debug */
		printf("3: #MC TRIANGLES=%d  #MC VERTICES=%d\n",
			mc_iso_surface->n_triangles,mc_iso_surface->n_vertices);
#endif /* defined (DEBUG_CODE) */
		if ((mc_iso_surface->n_triangles>0)&&((return_code=
					compile_mc_vertex_triangle_lists(mc_iso_surface,closed_surface))>0))
		{
			/* create vertex list */

#if defined (DEBUG_CODE)
#if defined (UNIX)
			real_time3=((double)times(&buffer))/100.0;
			cpu_time3= ((double)buffer.tms_utime)/100.0;
#endif
#endif /* defined (DEBUG_CODE) */

#ifdef CHECKLISTS
			printf("Checking lists....\n");
			for (i=0;i<mc_iso_surface->n_triangles;i++)
			{
				if (mc_iso_surface->compiled_triangle_list[i]->triangle_index < 0 ||
					mc_iso_surface->compiled_triangle_list[i]->triangle_index > mc_iso_surface->n_triangles)
				{
					printf ("Index error in triangle_list[%d], triangle_index=%d\n", i,
						mc_iso_surface->compiled_triangle_list[i]->triangle_index);
				}
			}
			for (i=0;i<mc_iso_surface->n_vertices;i++)
			{
				if (mc_iso_surface->compiled_vertex_list[i]->vertex_index < 0 ||
					mc_iso_surface->compiled_vertex_list[i]->vertex_index > mc_iso_surface->n_vertices)
				{
					printf ("Index error in vertex_list[%d], vertex_index=%d\n", i,
						mc_iso_surface->compiled_vertex_list[i]->vertex_index);
				}
			}
			for (i=0;i<mc_iso_surface->n_triangles;i++)
			{
				for(j=0;j<3;j++)
				{
					if (mc_iso_surface->compiled_triangle_list[i]->vertex_index[j] < 0 ||
						mc_iso_surface->compiled_triangle_list[i]->vertex_index[j] > mc_iso_surface->n_vertices)
					{
						printf ("Index error in compiled_triangle_list[%d]->vertex_index[%d]\n", i,j,
							mc_iso_surface->compiled_triangle_list[i]->vertex_index[j]);
					}
				}
			}

			/*printf("Done.\n");*/
#endif
			a_count=0;
			for (a=0;a<n_scalar_fields+6;a++)
			{
				a_count2=0;

				for (i=0;i<mcnx*mcny*mcnz;i++)
				{
					if (mc_iso_surface->mc_cells[i])
					{
						a_count +=mc_iso_surface->mc_cells[i]->n_triangles[a];
						a_count2 +=mc_iso_surface->mc_cells[i]->n_triangles[a];
					}
				}
				if (a < 6)
				{
					mc_iso_surface->xi_face_poly_index[a]=a_count;
					/*    printf("xi_face_poly_index[%d]=%d\n", a, a_count);*/
				}

				/*if (a>=6)
				  {
				  printf("# Triangles field[%d]=%d\n",a,  a_count2);
				  }*/
			}

#if defined (DEBUG_CODE)
#if defined (UNIX)
			/*???debug */
			real_time4=((double)times(&buffer))/100.0;
			cpu_time4= ((double)buffer.tms_utime)/100.0;
			/*printf("***************  Isosurface Timing Diagnostics **************\n");
			  printf("Marching cubes:			Real=%lf    CPU=%lf\n",real_time2-real_time1,cpu_time2-cpu_time1);
			  printf("Compile Vertex/Triangle lists:	Real=%lf    CPU=%lf\n",real_time3-real_time2,cpu_time3-cpu_time2);
			  printf("Normal calculation:		Real=%lf    CPU=%lf\n",real_time4-real_time3,cpu_time4-cpu_time3);
			  printf("_____________________________________________________________\n");
			  printf("Total time:            Real=%lf    CPU=%lf\n",real_time4-real_time1,cpu_time4-cpu_time1);
			  printf("Total number of polygons produced: %d\n",mc_iso_surface->n_triangles);
			  printf("Total number of vertices produced: %d\n",mc_iso_surface->n_vertices);
			  printf("************************************************************\n\n");*/
			/*
			  timefile=fopen("timefile","a");
			  fprintf(timefile,"# polys   t_mc  t_vtlist  t_normal  t_total\n");
			  fprintf(timefile,"%d %lf %lf %lf %lf\n",
			  iso_surface->n_triangles, cpu_time2-cpu_time1, cpu_time3-cpu_time2, cpu_time4-cpu_time3, cpu_time4-cpu_time1);
			  fclose(timefile);
			*/
#endif /* defined (UNIX) */
#endif /* defined (DEBUG_CODE) */

		}
		else
		{
#if defined (DEBUG_CODE)
			/*???debug */
			printf("WARNING: No polygons generated \n");
#endif /* defined (DEBUG_CODE) */
			if (mc_iso_surface->compiled_vertex_list)
			{
				DEALLOCATE(mc_iso_surface->compiled_vertex_list);
			}
			if (mc_iso_surface->compiled_triangle_list)
			{
				DEALLOCATE(mc_iso_surface->compiled_triangle_list);
			}
			mc_iso_surface->n_triangles=0;
			mc_iso_surface->n_vertices=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"marching_cubes.  Invalid argument(s)");
		return_code=0;
	}
#if defined (DEBUG_CODE)
	/*???debug */
	printf("leave marching_cubes\n");
#endif /* defined (DEBUG_CODE) */
	LEAVE;

	return (return_code);
} /* marching_cubes */

int update_scalars(struct VT_volume_texture *t)
/*******************************************************************************
LAST MODIFIED : 18 October 1997

DESCRIPTION :
Calculates scalar values at nodes from the average of the surrounding cell
values. Also calculates the gradient (using central differences) of the scalar
field. These values are then updated or stored as part of the volume_texture
structure.  1D arrays are used where A[i][j][k]=A'[i + j*dimi + k*dimi*dimj]
???MS.  I think i should put in a provision to edit node values - ie so this
routine cant touch the set nodes.
==============================================================================*/
{
	int return_code;
	int i,j,k,m,nx,ny,nz;
	int count;
	static double dist;
	double val[3],kfactor,sigmadist,sigmadist2;
	int incx,decx,incy,decy,incz,decz;
	struct VT_texture_node *node;
	double clipping = 0.0,scalar;
	struct VT_texture_curve *p;
	double point[3];

	ENTER(update_scalars);
	if (t)
	{
		return_code=1;
	nx=t->dimension[0]+1;
	ny=t->dimension[1]+1;
	nz=t->dimension[2]+1;
	kfactor=0.25;
	/* calculate coordinate field */
/* ???debug
printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!B4 update\n");
printVT(t);
*/
	if (t->grid_spacing)
	{
		for(k=0;k<nz;k++)
		{
			val[2]=t->grid_spacing[k+nx+ny];
			for (j=0;j<ny;j++)
			{
				val[1]=t->grid_spacing[j+nx];
				for (i=0;i<nx;i++)
				{
					val[0]=t->grid_spacing[i];
					for (m=0;m<3;m++)
					{
						t->coordinate_field->vector[3*(i+nx*j+nx*ny*k)+m]=
							val[m]*
							(t->ximax[m]-t->ximin[m]) + t->ximin[m];
				}
			}
		}
			}
	}
	else /* regular spacing */
	{
		for(k=0;k<nz;k++)
		{
			val[2]=(double) k;
			for (j=0;j<ny;j++)
			{
				val[1]=(double) j;
				for (i=0;i<nx;i++)
				{
					val[0]=(double) i;
					for (m=0;m<3;m++)
					{
						t->coordinate_field->vector[3*(i+nx*j+nx*ny*k)+m]=
							val[m]/((double) t->dimension[m])*
							(t->ximax[m]-t->ximin[m]) + t->ximin[m];
						/*???debug */
						/*printf("%g ",t->coordinate_field->vector[3*(i+nx*j+nx*ny*k)+m]);*/
					}
					/*???debug */
					/*printf("\n");*/
				}
			}
		}
	}
	for (k=0;k<nz;k++)
	{
		for (j=0;j<ny;j++)
		{
			for (i=0;i<nx;i++)
			{
				node=t->global_texture_node_list[ i + j*nx + k*nx*ny ];
				t->scalar_field->scalar[i + j*nx + k*nx*ny]=0;
				/* take average of cells surrounding node */
				/* check to see how many cells surrounding node */
				scalar=0.0;
				count=0;
				incx=incy=incz=decx=decy=decz=0;
				if (i < nx-1) incx=1;
				if (i > 0) decx=1;
				if (j < ny-1) incy=1;
				if (j > 0) decy=1;
				if (k < nz-1) incz=1;
				if (k > 0) decz=1;
				if (incx && incy && incz)
				{
					scalar +=(t->texture_cell_list[
						i + j*(nx-1) +k*(nx-1)*(ny-1)])->scalar_value;
					count++;
				}
				if (decx && decy && decz)
				{
					scalar +=(t->texture_cell_list[
						i-1 + (j-1)*(nx-1) +(k-1)*(nx-1)*(ny-1)])->scalar_value;
					count++;
				}
				if (incx && incy && decz)
				{
					scalar +=(t->texture_cell_list[
						i + j*(nx-1) +(k-1)*(nx-1)*(ny-1)])->scalar_value;
					count++;
				}
				if (incx && decy && decz)
				{
					scalar +=(t->texture_cell_list[
						i + (j-1)*(nx-1) +(k-1)*(nx-1)*(ny-1)])->scalar_value;
					count++;
				}
				if (incx && decy && incz)
				{
					scalar +=(t->texture_cell_list[
						i + (j-1)*(nx-1) +k*(nx-1)*(ny-1)])->scalar_value;
					count++;
				}
				if (decx && decy && incz)
				{
					scalar +=(t->texture_cell_list[
						i-1 + (j-1)*(nx-1) +k*(nx-1)*(ny-1)])->scalar_value;
					count++;
				}
				if (decx && incy && incz)
				{
					scalar +=(t->texture_cell_list[
						i-1 + j*(nx-1) +k*(nx-1)*(ny-1)])->scalar_value;
					count++;
				}
				if (decx && incy && decz)
				{
					scalar +=(t->texture_cell_list[
						i-1 + j*(nx-1) + (k-1)*(nx-1)*(ny-1)])->scalar_value;
					count++;
				}
				if (count==0)
				{
					printf("error - count=0\n");
					display_message(ERROR_MESSAGE,"marching cubes: count=0");

					scalar=1.0;
				}
				else
				{
					/*???DB.  Added option for reading in nodal scalars rather than
						calculating from cell values */
					if ((t->calculate_nodal_values)&&!(node->active))
					{
/*            scalar /=(double)count;*/
						scalar /=(double) count;
						node->scalar_value=scalar;
					}
					else
					{
						scalar=node->scalar_value;
						if (t->clipping_field_on)
						{
							clipping=node->clipping_fn_value;
						}
					}
				}
				t->scalar_field->scalar[i + j*nx + k*nx*ny]=scalar;
				/* define a clipping field in the same way as a scalar field */
				if (t->clipping_field_on)
				{
					t->clip_field->scalar[i + j*nx + k*nx*ny]=clipping;
				}


				/* put in potential contribution from texture_curves */
				for (m=0;m<3;m++)
				{
					point[m]=t->coordinate_field->vector[3*(i+nx*j+nx*ny*k)+m] ;
				}
				/* add contribution to point from each line segment */
				sigmadist=sigmadist2=0;
				for (p=*(t->texture_curve_list); p; p=p->ptrnext)
				{
					switch (p->type)
					{
						case 0:
						{
							dist= blob_segment_distance(0.75*kfactor,p->point1,p->point2,
								point,p->scalar_value[0],p->scalar_value[1]);
						} break;
						case 1:
						{
							dist= line_segment_distance(kfactor,p->point1,p->point2,point,
								p->scalar_value[0],
							p->scalar_value[1]);
						} break;
						case 2:
						{
							dist= curve_segment_distance(kfactor,p->point1,p->point3,
								p->point4,p->point2,point,p->scalar_value[0],
								p->scalar_value[1]);

						} break;
						case 3:
						{
							dist=0;
							t->scalar_field->scalar[i + j*nx + k*nx*ny] +=soft_object_distance(p->scalar_value[1],p->point1,
								point,p->scalar_value[0]);
						} break;
					}
					sigmadist +=dist;
					sigmadist2 +=dist*dist;
				}
				/* Vtotal=(Va + Vb + ..)/(Va^2 + Vb^2 + ...) */
				if (*(t->texture_curve_list) !=(struct VT_texture_curve *) NULL)
				{
					if (sigmadist2==0)
					{
						t->scalar_field->scalar[i + j*nx + k*nx*ny] += 0;
					}
					else if (t->scalar_field->scalar[i + j*nx + k*nx*ny] >0.0)
					{
						if (sigmadist/sigmadist2 < 1.0)
						{
							t->scalar_field->scalar[i + j*nx + k*nx*ny] +=
								1.0 - sigmadist/sigmadist2;
						}
						else
						{
							t->scalar_field->scalar[i + j*nx + k*nx*ny] +=0;
						}
					}
					else
					{
						t->scalar_field->scalar[i + j*nx + k*nx*ny] +=
							1.0 - sigmadist/sigmadist2;

					}
				}
				if (node->active)
				{
					/* if node active it dominates all */
					(t->scalar_field->scalar)[i+j*nx+k*nx*ny]=node->scalar_value;
				}
			}
		}
	}
/* debug
printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!after update\n");
printVT(t);
*/
	}
	else
	{
		display_message(ERROR_MESSAGE,"update_scalars.  Invalid argument(s)");
		return_code=1;
	}
	LEAVE;

	return (return_code);
} /* update_scalars */

void destroy_mc_triangle_list(struct MC_cell *mc_cell,
	struct MC_triangle ***triangle_list,int *n_triangles,int n_scalar_fields,
	struct MC_iso_surface *mc_iso_surface)
/*******************************************************************************
LAST MODIFIED : 1 November 1999

DESCRIPTION :
Destroy triangle list for a mc_cell, which includes (n_scalar_fields + 6)
independent triangle lists
==============================================================================*/
{
	int i;
	struct MC_triangle **field_list;

	ENTER(destroy_mc_triangle_list);
	if (triangle_list && n_triangles)
	{
		for (i=0;i < n_scalar_fields + 6;i++)
		{
			field_list=triangle_list[i];
			if (field_list&&n_triangles[i])
			{
				destroy_mc_triangle_field_list(mc_cell,field_list,n_triangles[i],
					mc_iso_surface);
			}
		}
		DEALLOCATE(n_triangles);
		DEALLOCATE(triangle_list);
	}
} /* destroy_mc_triangle_list */

void clean_mc_iso_surface(int n_scalar_fields,
	struct MC_iso_surface *mc_iso_surface)
/*******************************************************************************
LAST MODIFIED : 3 February 1998

DESCRIPTION :
==============================================================================*/
{
	int i;
	struct MC_cell *mc_cell;

	ENTER(clean_mc_iso_surface);
	/* clear mc_cell triangles and vertices! */
	if (mc_iso_surface->mc_cells !=NULL)
	{
		for (i=0;i<(mc_iso_surface->dimension[0]+2)*(mc_iso_surface->dimension[1]+2)*
				  (mc_iso_surface->dimension[2]+2);i++)
		{
			if (mc_iso_surface->mc_cells[i] !=NULL)
			{
				destroy_mc_triangle_list(mc_iso_surface->mc_cells[i],
					mc_iso_surface->mc_cells[i]->triangle_list,
					mc_iso_surface->mc_cells[i]->n_triangles,n_scalar_fields,
					mc_iso_surface);
				mc_iso_surface->mc_cells[i]->triangle_list=NULL;
				mc_iso_surface->mc_cells[i]->n_triangles=NULL;
				mc_cell=mc_iso_surface->mc_cells[i];
				DEALLOCATE(mc_cell);
				mc_iso_surface->mc_cells[i]=NULL;
			}
		}
		DEALLOCATE(mc_iso_surface->mc_cells);
	}
	else
	{
		/* SAB If there aren't any cells but there are compiled lists then
			the it may have been read in from a file and each triangle and
			vertex needs to be freed, I have reverse freed them so that the
			debug checking is as efficient as possible */
		if (mc_iso_surface->compiled_vertex_list)
		{
			for (i = 0 ; i < mc_iso_surface->n_vertices ; i++)
			{
				if(mc_iso_surface->compiled_vertex_list[mc_iso_surface->n_vertices-i-1]
					->triangle_ptrs)
				{
					DEALLOCATE(mc_iso_surface->compiled_vertex_list[mc_iso_surface->n_vertices-i-1]
						->triangle_ptrs);
				}
			}
		}
		if (mc_iso_surface->compiled_triangle_list)
		{
			for (i = 0 ; i < mc_iso_surface->n_triangles ; i++)
			{
				DEALLOCATE(mc_iso_surface->compiled_triangle_list[mc_iso_surface->n_triangles-i-1]);
			}
		}
		if (mc_iso_surface->compiled_vertex_list)
		{
			for (i = 0 ; i < mc_iso_surface->n_vertices ; i++)
			{
				DEALLOCATE(mc_iso_surface->compiled_vertex_list[mc_iso_surface->n_vertices-i-1]);
			}
		}
	}
	/*???SAB. Not sure about this but we need to free this somewhere.  Is this
		supposed to only tidy up rather than destroy? */
	DEALLOCATE(mc_iso_surface->compiled_triangle_list);
	DEALLOCATE(mc_iso_surface->compiled_vertex_list);
	DEALLOCATE(mc_iso_surface->detail_map);
	if (mc_iso_surface->deform)
	{
		DEALLOCATE(mc_iso_surface->deform);
	}
	mc_iso_surface->n_vertices=0;
	mc_iso_surface->n_triangles=0;
} /* clean_mc_iso_surface */

