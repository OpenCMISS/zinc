/***************************************************************************//**
 * FILE : generate_mesh_netgen.h
 *
 * Convenience functions for making simple finite element fields.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (__CMZN_GENERATE_MESH_NETGEN_H)
#define __CMZN_GENERATE_MESH_NETGEN_H

struct Generate_netgen_parameters;

/***************************************************************************//**
 * Calls create_netgen_parameters for creating a parameter set for netgen.
 * 
 * @param para  Pointer to a parameter set for netgen.
 * @return  1 if creating is successful
 *   0 if creating is failed
 */
struct Generate_netgen_parameters * create_netgen_parameters();

/***************************************************************************//**
 * Calls release_netgen_parameters for releasing a parameter set for netgen.
 * 
 * @param para  Pointer to a parameter set for netgen.
 * @return  1 if releasing is successful
 *   0 if releasing is failed
 */
int release_netgen_parameters(struct Generate_netgen_parameters *para);

/***************************************************************************//**
 * Calls set_netgen_parameters_secondorder for setting the secondorder switch
 * 
 * @param para Pointer to a parameter set for netgen.
 * @param secondorder is switch for secondorder
 * @return  1 if setting is successful
 *   0 if setting is failed
 */
int set_netgen_parameters_secondorder(struct Generate_netgen_parameters *para, int secondorder);

/***************************************************************************//**
 * Calls set_netgen_parameters_trimesh for setting the triangular surface mesh 
 * 
 * @param para  Pointer to a parameter set for netgen.
 * @param trimesh  Pointer to a triangular surface mesh for netgen.
 * @return  1 if setting is successful
 *   0 if setting is failed
 */
int set_netgen_parameters_trimesh(struct Generate_netgen_parameters *para, void *trimesh);

/***************************************************************************//**
 * Calls set_netgen_parameters_trimesh for setting the fineness of volumetric mesh
 * 
 * @param para  Pointer to a parameter set for netgen.
 * @param fineness  The fineness of volumetric mesh
 * @return  1 if setting is successful
 *   0 if setting is failed
 */
int set_netgen_parameters_fineness(struct Generate_netgen_parameters *para, double fineness);

/***************************************************************************//**
 * Calls set_netgen_parameters_maxh for setting the maximum length of volumetric element 
 * 
 * @param para  Pointer to a parameter set for netgen.
 * @param maxh  The maximum length of volumetric element
 * @return  1 if setting is successful
 *   0 if setting is failed
 */
int set_netgen_parameters_maxh(struct Generate_netgen_parameters *para, double maxh);

/***************************************************************************//**
 * Calls generate_mesh_netgen for invoking netgen and will visualize the 3D mesh automatically.
 * 
 * @param region  The region to create the mesh in.
 * @param netgen_para_void  Pointer to parameters for the specification of meshing. It comes with 
 *   such information:pointer to the triangular surface mesh, parameters to control the quality of
 *   volume mesh 
 * @return  1 if meshing is successful
 *   0 if meshing is failed
 */
int generate_mesh_netgen(cmzn_region *region, void *netgen_para_void);

/***************************************************************************//**
 * Calls set_netgen_parameters_meshsize_filename for setting mesh size file name
 * 
 * @param para  Pointer to a parameter set for netgen.
 * @param meshsize_filename  is mesh size file name
 * @return  1 if setting is successful
 *   0 if setting is failed
 */
int set_netgen_parameters_meshsize_filename(struct Generate_netgen_parameters *para,
	const char* meshsize_filename);
#endif /* __CMZN_GENERATE_MESH_NETGEN_H */

