/*****************************************************************************
FILE : generate_mesh_netgen.cpp

DESCRIPTION :
This interface is used to call netgen internally inside CMGUI. CMGUI will 
generate triangular surface and export the surface into netgen. After netgen returns
the volume mesh, CMGUI will visualize them to the user.

==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include "opencmiss/zinc/element.h"
#include "opencmiss/zinc/elementbasis.h"
#include "opencmiss/zinc/elementtemplate.h"
#include "opencmiss/zinc/fieldcache.h"
#include "opencmiss/zinc/fieldmodule.h"
#include "opencmiss/zinc/fieldfiniteelement.h"
#include "opencmiss/zinc/mesh.h"
#include "opencmiss/zinc/nodeset.h"
#include "opencmiss/zinc/nodetemplate.h"
#include "opencmiss/zinc/region.h"
#include "general/debug.h"
#include "general/mystring.h"
#include "finite_element/generate_mesh_netgen.h"
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_mesh.hpp"
#include "finite_element/finite_element_nodeset.hpp"
#include "finite_element/finite_element_region.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_wrappers.h"
#include "graphics/triangle_mesh.hpp"
#include "mesh/cmiss_node_private.hpp"


namespace nglib {
#include <nglib.h>
}
using namespace nglib;
// Netgen Meshing Parameters class

struct Generate_netgen_parameters
{   
	double maxh;
	double fineness;
	int secondorder;
	Triangle_mesh *trimesh; 
	char* meshsize_filename;
};/*struct Generate_netgen_parameters*/

int set_netgen_parameters_trimesh(struct Generate_netgen_parameters *para, void *trimesh)
{
	ENTER(set_netgen_parameters_trimesh);
	int return_code=0;
	Triangle_mesh* tri_mesh_tmp;

	if(para==NULL) return return_code;
	if(trimesh==NULL) return return_code;
	tri_mesh_tmp=(Triangle_mesh*)trimesh;
	para->trimesh=tri_mesh_tmp;
	return_code=1;

	LEAVE;
	return return_code;
}

int set_netgen_parameters_fineness(struct Generate_netgen_parameters *para, double fineness)
{
	ENTER(set_netgen_parameters_fineness);
	int return_code=0;

	if(para==NULL) return return_code;

	para->fineness=fineness;
	return_code=1;

	LEAVE;
	return return_code;
}

int set_netgen_parameters_maxh(struct Generate_netgen_parameters *para, double maxh)
{
	ENTER(set_netgen_parameters_maxh);
	int return_code=0;

	if(para==NULL) return return_code;

	para->maxh=maxh;
	return_code=1;

	LEAVE;
	return return_code;
}

int set_netgen_parameters_secondorder(struct Generate_netgen_parameters *para, int secondorder)
{
	ENTER(set_netgen_parameters_secondorder);
	int return_code=0;

	if(para==NULL) return return_code;

	para->secondorder=secondorder;
	return_code=1;

	LEAVE;
	return return_code;
}

int set_netgen_parameters_meshsize_filename(struct Generate_netgen_parameters *para,
	const char* meshsize_filename)
{
	int return_code=0;

	ENTER(set_netgen_parameters_meshsize_filename);
	if(para && meshsize_filename)
	{
		para->meshsize_filename=duplicate_string(meshsize_filename);
		return_code=1;
	}
	LEAVE;

	return return_code;
}

/***************************************************************************//**
																			 * brief Locally restrict the mesh element size at the given point
																			 *
																			 * Unlike the function #Ng_RestrictMeshSizeGlobal, this function 
																			 * allows the user to locally restrict the maximum allowable mesh 
																			 * size at a given point.
																			 *
																			 * The point is specified via its three cartesian co-ordinates.
																			 *
																			 * <b>Note</b>: This function only limits the <b>Maximum</b> size 
																			 * of the elements around the specified point.
																			 *
																			 * @param mesh  Pointer to an existing Netgen Mesh structure of 
																			 *               type #Ng_Mesh
																			 * @param p  Pointer to an Array of type <i>double</i>, containing 
																			 *             the three co-ordinates of the point in the form: \n
																			 *             - p[0] = X co-ordinate
																			 *             - p[1] = Y co-ordinate
																			 *             - p[2] = Z co-ordinate
																			 * @param h  Variable of type <i>double</i>, specifying the maximum
																			 *             allowable mesh size at that point
																			 */
int set_netgen_restrict_meshsizepoint(Ng_Mesh *mesh, double *p, double h)
{
	ENTER(set_netgen_restrict_meshsizepoint);
	int return_code=0;

	if(mesh)
	{
		Ng_RestrictMeshSizePoint(mesh, p,h);
		return_code=1;
	}

	LEAVE;
	return return_code;
}

/***************************************************************************//**
																			 *
																			 *  Similar to the function set_netgen_restrict_meshsize_Point, this function 
																			 *  allows the size of elements within a mesh to be locally limited.
																			 *
																			 *  However, rather than limit the mesh size at a single point, this 
																			 *  utility restricts the local mesh size within a 3D Box region, specified 
																			 *  via the co-ordinates of the two diagonally opposite points of a cuboid.
																			 *
																			 *  <b>Note</b>: This function only limits the <b>Maximum</b> size 
																			 *  of the elements within the specified region.
																			 *
																			 *  @param mesh Pointer to an existing Netgen Mesh structure of 
																			 *              type #Ng_Mesh
																			 *  @param pmin Pointer to an Array of type <i>double</i>, containing 
																			 *             the three co-ordinates of the first point of the cuboid: \n
																			 *              - pmin[0] = X co-ordinate
																			 *              - pmin[1] = Y co-ordinate
																			 *              - pmin[2] = Z co-ordinate
																			 *  @param pmax Pointer to an Array of type <i>double</i>, containing 
																			 *              the three co-ordinates of the opposite point of the 
																			 *              cuboid: \n

																			 *              - pmax[0] = X co-ordinate
																			 *              - pmax[1] = Y co-ordinate
																			 *              - pmax[2] = Z co-ordinate
																			 *  @param h    Variable of type <i>double</i>, specifying the maximum
																			 *              allowable mesh size at that point
																			 */
int set_netgen_restrict_meshsizebox(Ng_Mesh *mesh, double *pmin, double *pmax, double h)
{
	ENTER(set_netgen_restrict_meshsizebox);
	int return_code=0;

	if(mesh)
	{     
		Ng_RestrictMeshSizeBox(mesh, pmin, pmax,h);
		return_code=1;
	}
	LEAVE;
	return return_code;
}

struct Generate_netgen_parameters * create_netgen_parameters()
{
	ENTER(create_netgen_parameters);
	struct Generate_netgen_parameters *para;

	para= new struct Generate_netgen_parameters();

	LEAVE;
	return para;
}

int release_netgen_parameters(struct Generate_netgen_parameters *para)
{
	int return_code=0;

	ENTER(release_netgen_parameters);
	if(para)
	{
		if (para->meshsize_filename)
		{
			DEALLOCATE(para->meshsize_filename);
		}
		delete para;
		return_code=1;
	}
	LEAVE;

	return return_code;
}

int generate_mesh_netgen(cmzn_region *region, void *netgen_para_void)
{
	Generate_netgen_parameters *generate_netgen_para = static_cast<Generate_netgen_parameters *>(netgen_para_void);
	Triangle_mesh *trimesh=generate_netgen_para->trimesh;
	int return_code;
	Ng_Mesh * mesh;
	Ng_STL_Geometry * geom;
	Ng_Meshing_Parameters *mp=new Ng_Meshing_Parameters();
	mp->maxh=generate_netgen_para->maxh;
	mp->fineness=generate_netgen_para->fineness;
	mp->secondorder=generate_netgen_para->secondorder;
	mp->meshsize_filename=generate_netgen_para->meshsize_filename;

	Ng_Init();
	geom=Ng_STL_NewGeometry(); 

	const Mesh_triangle_list  triangle_list = trimesh->get_triangle_list();
	Mesh_triangle_list_const_iterator triangle_iter;

	ZnReal coord1[3], coord2[3],coord3[3];
	//ZnReal dcoord1[3], dcoord2[3], dcoord3[3];

	const Triangle_vertex *vertex1, *vertex2, *vertex3;
	for (triangle_iter = triangle_list.begin(); triangle_iter!=triangle_list.end(); ++triangle_iter)
	{
		(*triangle_iter)->get_vertexes(&vertex1/*point 1*/,&vertex2/*point 2*/,&vertex3/*point 3*/);
		vertex1->get_coordinates(coord1, coord1+1,coord1+2);
		vertex2->get_coordinates(coord2, coord2+1,coord2+2);
		vertex3->get_coordinates(coord3, coord3+1,coord3+2);
		//dcoord1[0] = (double)coord1[0];
		//dcoord1[1] = (double)coord1[1];
		//dcoord1[2] = (double)coord1[2];
		//dcoord2[0] = (double)coord2[0];
		//dcoord2[1] = (double)coord2[1];
		//dcoord2[2] = (double)coord2[2];
		//dcoord3[0] = (double)coord3[0];
		//dcoord3[1] = (double)coord3[1];
		//dcoord3[2] = (double)coord3[2];    
		Ng_STL_AddTriangle(geom/*new geometry*/, coord1/*point 1*/, coord2/*point 2*/, coord3/*point 3*/);
	}

	return_code=Ng_STL_InitSTLGeometry(geom);
	if(return_code!=NG_OK)
	{
		Ng_Exit();
		return 0;
	}
	mesh = Ng_NewMesh (); 
	return_code=Ng_STL_MakeEdges(geom, mesh, mp);
	if(return_code!=NG_OK)
	{
		Ng_Exit();
		return 0;
	}

	return_code=Ng_STL_GenerateSurfaceMesh(geom, mesh, mp);
	if(return_code!=NG_OK)
	{
		Ng_Exit();
		return 0;
	}

	try
	{
		return_code=Ng_GenerateVolumeMesh(mesh,mp);
	}
	catch (...)
	{
		display_message(ERROR_MESSAGE, "Netgen exception occurred - aborting");
		return_code = NG_ERROR;
	}
	if(return_code!=NG_OK)
	{
		Ng_Exit();
		return 0;
	}

	cmzn_fieldmodule_id fieldmodule = cmzn_region_get_fieldmodule(region);
	cmzn_fieldmodule_begin_change(fieldmodule);

	/* create a 3-D coordinate field */
	cmzn_field_id coordinate_field = cmzn_fieldmodule_find_field_by_name(fieldmodule, "coordinates");
	if (coordinate_field)
	{
		cmzn_field_finite_element_id fe_field = cmzn_field_cast_finite_element(coordinate_field);
		if ((!fe_field) ||
			(3 != cmzn_field_get_number_of_components(coordinate_field)) ||
			(!cmzn_field_is_type_coordinate(coordinate_field)) ||
			(CMZN_FIELD_COORDINATE_SYSTEM_TYPE_RECTANGULAR_CARTESIAN !=
				cmzn_field_get_coordinate_system_type(coordinate_field)))
		{
			cmzn_field_destroy(&coordinate_field);
		}
		cmzn_field_finite_element_destroy(&fe_field);
	}
	if (!coordinate_field)
	{
		coordinate_field = cmzn_fieldmodule_create_field_finite_element(fieldmodule, 3);
		cmzn_field_set_name(coordinate_field, "coordinates");
		cmzn_field_set_component_name(coordinate_field, 1, "x");
		cmzn_field_set_component_name(coordinate_field, 2, "y");
		cmzn_field_set_component_name(coordinate_field, 3, "z");
		cmzn_field_set_managed(coordinate_field, true);
		cmzn_field_set_type_coordinate(coordinate_field, true);
	}

	/* create and fill nodes */
	cmzn_nodeset_id nodeset = cmzn_fieldmodule_find_nodeset_by_field_domain_type(fieldmodule, CMZN_FIELD_DOMAIN_TYPE_NODES);
	cmzn_nodetemplate_id nodetemplate = cmzn_nodeset_create_nodetemplate(nodeset);
	cmzn_nodetemplate_define_field(nodetemplate, coordinate_field);
	cmzn_fieldcache_id fieldcache = cmzn_fieldmodule_create_fieldcache(fieldmodule);

	FE_nodeset *fe_nodeset = cmzn_nodeset_get_FE_nodeset_internal(nodeset);
	const int number_of_nodes = Ng_GetNP(mesh);
	double coordinates[3];
	int initial_identifier = fe_nodeset->get_last_FE_node_identifier() + 1;
	int i;
	for (i = 0; i < number_of_nodes; i++)
	{
		Ng_GetPoint (mesh, i+1/*index in netgen starts from 1*/, coordinates);
		cmzn_node_id node = cmzn_nodeset_create_node(nodeset, initial_identifier + i, nodetemplate);
		cmzn_fieldcache_set_node(fieldcache, node);
		cmzn_field_assign_real(coordinate_field, fieldcache, 3, coordinates);
		cmzn_node_destroy(&node);
	}
	cmzn_fieldcache_destroy(&fieldcache);
	cmzn_nodetemplate_destroy(&nodetemplate);

	// establish mode which automates creation of shared faces
	FE_region_begin_define_faces(region->get_FE_region());
	FE_mesh *fe_mesh = FE_region_find_FE_mesh_by_dimension(region->get_FE_region(), 3);

	cmzn_mesh_id cmesh = cmzn_fieldmodule_find_mesh_by_dimension(fieldmodule, 3);
	cmzn_elementtemplate_id elementtemplate = cmzn_mesh_create_elementtemplate(cmesh);
	cmzn_elementtemplate_set_element_shape_type(elementtemplate, CMZN_ELEMENT_SHAPE_TYPE_TETRAHEDRON);
	cmzn_elementtemplate_set_number_of_nodes(elementtemplate, 4);
	cmzn_elementbasis_id elementbasis = cmzn_fieldmodule_create_elementbasis(fieldmodule, 3,
		CMZN_ELEMENTBASIS_FUNCTION_TYPE_LINEAR_SIMPLEX);
	int local_node_indexes[4] = { 1, 2, 3, 4 };
	cmzn_elementtemplate_define_field_simple_nodal(elementtemplate, coordinate_field, /*component_number*/-1,
		elementbasis, 4, local_node_indexes);

	const int number_of_elements = Ng_GetNE(mesh);
	int nodal_idx[4];
	for (i = 0 ; i < number_of_elements; i++)
	{
		Ng_GetVolumeElement (mesh, i+1, nodal_idx);
		/* netgen tet node order gives a left handed element coordinate system,
		 * hence swap node indices 2 and 3 */
		cmzn_elementtemplate_set_node(elementtemplate, 1, cmzn_nodeset_find_node_by_identifier(nodeset, nodal_idx[0] + initial_identifier - 1));
		cmzn_elementtemplate_set_node(elementtemplate, 2, cmzn_nodeset_find_node_by_identifier(nodeset, nodal_idx[1] + initial_identifier - 1));
		cmzn_elementtemplate_set_node(elementtemplate, 3, cmzn_nodeset_find_node_by_identifier(nodeset, nodal_idx[3] + initial_identifier - 1));
		cmzn_elementtemplate_set_node(elementtemplate, 4, cmzn_nodeset_find_node_by_identifier(nodeset, nodal_idx[2] + initial_identifier - 1));
		cmzn_element_id element = cmzn_mesh_create_element(cmesh, /*identifier*/-1, elementtemplate);
		fe_mesh->defineElementFaces(get_FE_element_index(element));
		cmzn_element_destroy(&element);
	}
	cmzn_elementbasis_destroy(&elementbasis);
	cmzn_elementtemplate_destroy(&elementtemplate);
	cmzn_mesh_destroy(&cmesh);

	cmzn_nodeset_destroy(&nodeset);
	cmzn_field_destroy(&coordinate_field);

	FE_region_end_define_faces(region->get_FE_region());

	cmzn_fieldmodule_end_change(fieldmodule);

	if (mesh)
		Ng_DeleteMesh (mesh); 
	Ng_Exit();
	return return_code;
}
