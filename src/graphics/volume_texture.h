/*******************************************************************************
FILE : volume_texture.h

LAST MODIFIED : 6 December 2004

DESCRIPTION :
Contains data structures for the 3d volumetric textures to be mapped onto
finite elements.  A volume texture is a group of texture 'elements' defined as
lists of texture 'nodes'. Material values may be associated either with
elements, or with nodes.  The texture nodes are arranged as a 3d lattice in xi1,
xi2, xi3 space.
???DB.  Why are doubles used ?
???DB.  Get rid of index's ?
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (VOLUME_TEXTURE_H)
#define VOLUME_TEXTURE_H

#include <stdio.h>
#include "general/io_stream.h"
#include "general/list.h"
#include "general/manager.h"
#include "general/object.h"
#include "graphics/material.h"
#include "graphics/environment_map.h"

/*
Module constants
----------------
*/
/*???DB.  Shouldn't use.  Determine number required and allocate */
#define MAX_SCALAR_FIELDS 5
#define MAXLINES 100
#define ISOPOLYMAX 10000
#define MAXPTRS 256
#define MAX_MATERIALS 100

/*
Module types
------------
*/
struct MC_vertex
/*******************************************************************************
LAST MODIFIED : 19 February 1998

DESCRIPTION :
Unique Vertex structure for isosurface - can be shared by triangles in adjacent
MC_cells.
==============================================================================*/
{
	/* identifiers for creating array */
	int vertex_index;
	/* vertex position */
	ZnReal coord[3];
	/* back pointers to triangles containing vertices */
	int n_triangle_ptrs;
	struct MC_triangle **triangle_ptrs;
}; /* struct MC_vertex */

struct MC_triangle
/*******************************************************************************
LAST MODIFIED : 24 February 1997
DESCRIPTION : Triangle structure for isosurface. Points to shared vertices.
Contains material and texture map information for each triangle as vertices
can be shared by adjacent cells with differing materials.
==============================================================================*/
{
	/* pointers to vertices (ordering important for normal calculation) */
	struct MC_vertex *vertices[3];
	/* identifiers for creating array */
	int triangle_index;
	int vertex_index[3];
	/* index describing which MC_cell triangle list this
	triangle belongs to */
	int triangle_list_index;
	/* back pointers to cell containing triangle vertices */
	struct MC_cell *cell_ptr;
}; /* struct MC_triangle */

struct MC_cell
/*******************************************************************************
LAST MODIFIED : 24 February 1997
DESCRIPTION : Cell based structure representing marching cubes based isosurface.
Each cell contains independent sets of triangular polygons associated with
different scalar files or faces.
==============================================================================*/
{
	/* index (i,j,k) = MC_cells[ i + j*nx + k*nx*ny ] */
	int index[3];
	/* pointers to triangles */
	/* Each scalar field and face has its own list of triangles which do not
	share vertices and normals, or material information, so there are
	n independent lists allocated for each cell */
	int *n_triangles;
	struct MC_triangle ***triangle_list;
}; /* struct MC_cell */

struct MC_iso_surface
/*******************************************************************************
LAST MODIFIED : 18 May 1998

DESCRIPTION :
Isosurface created from MC_cell list, which points to the vertices and triangles
for transformation and rendering
==============================================================================*/
{
	int dimension[3];
	/* Local editing: perform marching cubes only on cells i,j,k =
	active_block[0,1] x active_block[2,3] x active_block[4,5] */
	int active_block[6];
	/* list of mc_cells[dimension[0]*dim..[1]*dim..[2]]  */
	/* this can be discarded after editing has finished */
	struct MC_cell **mc_cells;
	/* list of cells to be detailed (higher resolution triangulation) */
	int *detail_map;
	/* this represents the number of scalar fields to create the isosurface */
	int n_scalar_fields;
	/* the compiled list of pointers to unique vertices and triangles
		which is sent to the renderer */
	int n_vertices;
	int n_triangles;
	struct MC_vertex **compiled_vertex_list;
	struct MC_triangle **compiled_triangle_list;
	/* index to allow separation of isosurface closed faces (so that if closed
		faces are internal in a group of adjacent elements they need not be
		drawn) */
	int xi_face_poly_index[6];
	/* list of deformable vertices (i.e. material points) if NULL (default), all
		points are treated as material points */
	int *deform;
}; /* struct MC_iso_surface */

struct Clipping
/*******************************************************************************
LAST MODIFIED : 25 July 1995

DESCRIPTION :
==============================================================================*/
{
	ZnReal (*function)(ZnReal *,ZnReal *);
	ZnReal *parameters;
}; /* struct Clipping */

struct VT_scalar_field
/*******************************************************************************
LAST MODIFIED : 22 November 1994

DESCRIPTION :
==============================================================================*/
{
	int dimension[3];
	ZnReal *scalar;
}; /* struct VT_scalar_field */

struct VT_vector_field
/*******************************************************************************
LAST MODIFIED : 22 November 1994

DESCRIPTION :
==============================================================================*/
{
	/* nx ny nz */
	int dimension[3];
	/* the vectors are stord in a 1D array, hence component (n) of the vector at
		grid point[i,j,k] is vector[3*(i + j*nx + k*nx*ny) + n ] */
	ZnReal *vector;
}; /* struct VT_vector_field */

struct VT_iso_triangle;

struct VT_iso_vertex
/*******************************************************************************
LAST MODIFIED : 21 October 1997

DESCRIPTION :
==============================================================================*/
{
	ZnReal coordinates[3];
	ZnReal normal[3];
	ZnReal texture_coordinates[3];

	int number_of_triangles;
	struct VT_iso_triangle **triangles;

	ZnReal *data;

	/* Integer index indicating where this vertex is in the list, useful
		for serialising */
	int index;
}; /* struct VT_iso_vertex */

struct VT_iso_triangle
{
	struct VT_iso_vertex *vertices[3];
	/* Integer index indicating where this triangle is in the list, useful
		for serialising */
	int index;
};

struct VT_texture_curve
/*******************************************************************************
LAST MODIFIED : 22 November 1994

DESCRIPTION :
???DB.  list item type.
==============================================================================*/
{
	/* type 0:ellipsoid    1:line     2:bezier curve */
	int type;
	/* a bezier curve segment for field generation purposes */
	int index;
	/* potential value at nodes */
	ZnReal scalar_value[2];
	/* the xi coordinates of each point defining the segment */
	/* the line passes from p1 to p4, with p2 and p3 defining the
	slope at each node in bezier fashion */
	ZnReal point1[3];
	ZnReal point2[3];
	ZnReal point3[3];
	ZnReal point4[3];
	struct VT_texture_curve *ptrnext;
}; /* struct VT_iso_surface */

struct VT_texture_node
/*******************************************************************************
LAST MODIFIED : 8 January 1998

DESCRIPTION :
==============================================================================*/
{
	int index;

	/* the isosurface value at the node */
	ZnReal scalar_value;
	/* the clipping function value */
	ZnReal clipping_fn_value;
	/* the gradient of the scalar field at each node */
	ZnReal scalar_gradient[3];
	/* if active flag, use scalar value directly */
	int active;
	/* nodes may be used to represent FE nodes for mesh generation in this
		capacity the texture_node is used to create groups and represent split
		FE nodes for creating slits in the mesh */
	/* type 0: no slits (normal), 1: slit in xi1, 2: slit in xi2,
		3: slit in xi1 & xi2, 4: slit in xi3, 5: slit in xi1 & xi3
		6: slit in xi2 &xi3, 7: slit in xi1 & x2 & x3 */
	int node_type;
	/* the cmiss identifier for each number - accessed by
		cm_node_identifier[node_type] */
	int cm_node_identifier[8];
}; /* struct VT_texture_node */

struct VT_texture_cell
/*******************************************************************************
LAST MODIFIED : 17 July 1997

DESCRIPTION :
==============================================================================*/
{
	int index;
	/* the scalar value for isosurface construction */
	ZnReal scalar_value;
}; /* struct VT_texture_cell */

struct VT_node_group
/*******************************************************************************
LAST MODIFIED : 14 January 1998

DESCRIPTION :

==============================================================================*/
{
	char *name;
	/* type 0: no fields, 1: slit, 2: scalar_field, 3: vector_field */
	int type;
	int n_nodes;
	/* nodes are referenced by their position in the global node array, and if the
		index is greater than nx*ny*nz then it refers to one of the split nodes for
		a slit (type = index modulo nx*ny*nz) */
	int *nodes;
}; /* struct VT_node_group */

struct VT_volume_texture
/*******************************************************************************
LAST MODIFIED : 8 January 1998

DESCRIPTION :
???DB.  Make some of the pointers sub-structures ?
???DB.  Added option for reading in nodal scalars rather than calculating from
	cell values
==============================================================================*/
{
	int index;
	char *name;
	/* the name of the file the volume texture was read from */
	char *file_name;
	/* xi range */
	ZnReal ximin[3],ximax[3];
	/* scales and offsets for the volume texture in the xi directions
		SAB It is not clear that these are generally useful or exactly
	   what effect they will have over multiple elements*/
	ZnReal scale_xi[3], offset_xi[3];
	/* discretization/resolution in xi1,2,3 directions */
	int dimension[3];
	/* grid spacing for irregular grids - consists of
	a 1-D array [(dimension[0]+1)+(dimension[1]+1)+(dimension[2]+1)]
	of normalized spacing values [0,1]). If NULL then regular spacing */
	ZnReal *grid_spacing;
	struct VT_texture_curve **texture_curve_list;
	struct VT_texture_cell **texture_cell_list;
	/* list of the global nodes of the texture */
	struct VT_texture_node **global_texture_node_list;
	/* the scalar field for creating the isosurface */
	struct VT_scalar_field *scalar_field;
	/* the scalar field for the clipping isosurface */
	struct VT_scalar_field *clip_field;
	struct VT_scalar_field *clip_field2;
	/* the set of coordinates corresponding to the scalar values */
	struct VT_vector_field *coordinate_field;
	/* the iso_surface representing this texture */
	struct MC_iso_surface *mc_iso_surface;
	/* the material for the isosurface */

	/* the centre of projection coordinates for any mapped texture */

	/* for 2d texturemapping */

	/* the index if a projection function has a set of alternative
	textures for different faces */


	ZnReal isovalue;
	int hollow_mode_on;
	ZnReal hollow_isovalue;
	/* cutting field allows the value of the function to be calculated in
		conjuction with the isoscalar.  It cannot be used in conjunction with the
		cutting plane and uses the same cut_isovalue and VT_scalar_field
		clip_field */
	int clipping_field_on;
	int cutting_plane_on;
	ZnReal cut_isovalue;
	ZnReal cutting_plane[4];
	int closed_surface;
	ZnReal decimation_threshold;
	/*???DB.  Added option for reading in nodal scalars rather than calculating
		from cell values */
	int calculate_nodal_values;
	/* only recalculate if the scalar field has changed: 1 = recalulate, 0 = do
		not recalulate */
	int recalculate;
	/* if a Alias/Wavefront .obj file is stored as an isosurface we must disable
		any volume functions and set disable_volume_functions = 1 */
	int disable_volume_functions;

	/* number of groups to which the node belongs (used to create exnode files) */
	int n_groups;
	struct VT_node_group **node_groups;

	/* after clearing in create, following to be modified only by manager */
	struct MANAGER(VT_volume_texture) *manager;
	int manager_change_status;
	
	int access_count;
}; /* struct VT_volume_texture */

DECLARE_LIST_TYPES(VT_volume_texture);

DECLARE_MANAGER_TYPES(VT_volume_texture);

struct Modify_VT_volume_texture_data
/*******************************************************************************
LAST MODIFIED : 6 December 2004

DESCRIPTION :
==============================================================================*/
{
	struct IO_stream_package *io_stream_package;
	struct MANAGER(Environment_map) *environment_map_manager;
	struct MANAGER(cmzn_material) *graphical_material_manager;
	struct MANAGER(VT_volume_texture) *volume_texture_manager;
	char **example_directory_address;
}; /* struct Modify_VT_volume_texture_data */

/*
Global functions
----------------
*/
struct VT_volume_texture *CREATE(VT_volume_texture)(const char *name);
/*******************************************************************************
LAST MODIFIED : 25 September 1995

DESCRIPTION :
Allocates memory and assigns fields for a volume texture.  Adds the texture to
the list of all volume textures.
???DB.  Trimming name ?
???DB.  Check if it already exists ?  Retrieve if it does already exist ?
==============================================================================*/

int DESTROY(VT_volume_texture)(struct VT_volume_texture **texture_address);
/*******************************************************************************
LAST MODIFIED : 25 September 1995

DESCRIPTION :
Frees the memory for the volume texture and sets <*texture_address> to NULL.
==============================================================================*/

PROTOTYPE_OBJECT_FUNCTIONS(VT_volume_texture);
PROTOTYPE_GET_OBJECT_NAME_FUNCTION(VT_volume_texture);

PROTOTYPE_LIST_FUNCTIONS(VT_volume_texture);

PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(VT_volume_texture,name,const char *);

PROTOTYPE_MANAGER_COPY_FUNCTIONS(VT_volume_texture,name,const char *);

PROTOTYPE_MANAGER_FUNCTIONS(VT_volume_texture);

PROTOTYPE_MANAGER_IDENTIFIER_FUNCTIONS(VT_volume_texture,name,const char *);

void generate_isosurface(struct VT_volume_texture *texture);
/*******************************************************************************
LAST MODIFIED : 13 November 1994

DESCRIPTION :
Creates/Updates isosurface for basic volume_texture
==============================================================================*/

struct Clipping *CREATE(Clipping)(void);
/*******************************************************************************
LAST MODIFIED : 26 January 1996

DESCRIPTION :
Allocates memory for a clipping.
==============================================================================*/

int DESTROY(Clipping)(struct Clipping **clipping_address);
/*******************************************************************************
LAST MODIFIED : 26 January 1996

DESCRIPTION :
Frees the memory for the clipping and sets <*clipping_address> to NULL.
==============================================================================*/

struct VT_iso_vertex *CREATE(VT_iso_vertex)(void);
/*******************************************************************************
LAST MODIFIED : 9 November 2005

DESCRIPTION :
==============================================================================*/

int DESTROY(VT_iso_vertex)(struct VT_iso_vertex **vertex_address);
/*******************************************************************************
LAST MODIFIED : 9 November 2005

DESCRIPTION :
Frees the memory for the volume vertex and sets <*vertex_address> to NULL.
==============================================================================*/

/***************************************************************************//**
 * Creates and fills a VT_iso_vertex with the supplied values.
 * @param coordinates  Array of 3 coordinates. Compulsory.
 * @param normal  Array of 3 normal components. Compulsory.
 * @param texture_coordinates  Optional pointer to up to 3 texture coordinates.
 * @param n_data_components  Number of data components to follow.
 * @param data  Pointer to n_data_components data values for vertex.
 * @param return  Newly created and set VT_iso_vertex.
 */
struct VT_iso_vertex *VT_iso_vertex_create_and_set(
	GLfloat *coordinates, GLfloat *normal, GLfloat *texture_coordinates,
	int n_data_components, GLfloat *data);

int VT_iso_vertex_normalise_normal(struct VT_iso_vertex *vertex);
/*******************************************************************************
LAST MODIFIED : 28 October 2005

DESCRIPTION :
Normalisess the normal for the vertex.
==============================================================================*/

struct VT_iso_triangle *CREATE(VT_iso_triangle)(void);
/*******************************************************************************
LAST MODIFIED : 9 November 2005

DESCRIPTION :
==============================================================================*/

int DESTROY(VT_iso_triangle)(struct VT_iso_triangle **triangle_address);
/*******************************************************************************
LAST MODIFIED : 9 November 2005

DESCRIPTION :
Frees the memory for the volume triangle and sets <*triangle_address> to NULL.
==============================================================================*/

/***************************************************************************//**
 * Creates and fills a VT_iso_triangle using the supplied values. Adds back
 * references to triangle from each vertex.
 * @param vertices  Array of 3 VT_iso_vertex. 
 * @param return  Newly created and set VT_iso_triangle.
 */
struct VT_iso_triangle *VT_iso_triangle_create_and_set(
	struct VT_iso_vertex **vertices);

#endif
