/*******************************************************************************
FILE: em_cmgui.h

LAST MODIFIED : 25 Febuary 1998

DESCRIPTION :
Header file for EM stuff for DB and cmgui.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (EM_CMGUI_H)
#define EM_CMGUI_H


struct EM_Object
/*******************************************************************************
LAST MODIFIED : 25 February 1998

DESCRIPTION : Contains the data read in from the EM basis file.
	A = U.w.V^T
==============================================================================*/
{
	double *u;
	double *w;
	double *v;
	double mode_one_std;
	int *index;
	int n_modes;
	int n_nodes;
	int m;
	int *minimum_nodes;
	int number_of_minimum_nodes;
}; /* struct EM_Object */


/*******************************************************************************
LAST MODIFIED : 27 Febuary 1998

DESCRIPTION :
Allocate the object that contains the EM data.
==============================================================================*/
struct EM_Object *create_EM_Object(int n_modes,int n_nodes);

/*******************************************************************************
LAST MODIFIED : 27 Febuary 1998

DESCRIPTION :
Destroy the object that contains the EM data.
==============================================================================*/
void destroy_EM_Object(struct EM_Object **em_object);

/*******************************************************************************
LAST MODIFIED : 26 Febuary 1998

DESCRIPTION :
Write out a basis data file.
==============================================================================*/
void EM_write_basis(char *filename,struct EM_Object *em_object);

struct EM_Object *EM_read_basis(char *filename,
	struct IO_stream_package *io_stream_package, struct EM_Object **em_object,
	int *node_index_list, int number_in_node_list);
/*******************************************************************************
LAST MODIFIED : 28 March 2007

DESCRIPTION :
Read in a file containing a basis function. Creates a EM_Object and returns
it, deallocating the old em_object and pointing it to the new one.
The <node_index_list> and <number_in_node_list> are required when the basis_file
is a version two file as these basis files do not include information about
the corresponding nodes.
==============================================================================*/

/*******************************************************************************
LAST MODIFIED : 26 Febuary 1998

DESCRIPTION :
Returns the number of modes.
==============================================================================*/
int EM_number_of_modes(struct EM_Object *em_object);

/*******************************************************************************
LAST MODIFIED : 26 Febuary 1998

DESCRIPTION :
Returns the number of nodes.
==============================================================================*/
int EM_number_of_nodes(struct EM_Object *em_object);

/*******************************************************************************
LAST MODIFIED : 26 Febuary 1998

DESCRIPTION :
Reconstructs a face from a EM_Object and a set of weights. Returns a
pointer to a vector of double, of size 3*n_nodes, contains the x,y,z
coordinates of the reconstructed face, in order x1,y1,z1,x2,y2,z2, ...
Weights must be of size n_modes.
==============================================================================*/
double *EM_reconstruct(double *weights,struct EM_Object *em_object);

/*******************************************************************************
LAST MODIFIED : 26 Febuary 1998

DESCRIPTION :
Returns the max/min weights for the em data. The arrays max and min must be
of size n_modes.
==============================================================================*/
void EM_get_weights(double *max,double *min,struct EM_Object *em_object);

/*******************************************************************************
LAST MODIFIED : 27 Febuary 1998

DESCRIPTION :
Projects a face onto a basis function. x must be of size n_nodes*3, and
contains the coordinates of the face to be deconstructed, in the order
x1,y1,z1,x2,y2,z2,.... index contains the node numbers for face points.
An array of size n_modes is allocated and returned. This contains the
weights for each face mode.
==============================================================================*/
double *EM_project(double *x,int *index,struct EM_Object *em_object);

/*******************************************************************************
LAST MODIFIED : 14 April 1998

DESCRIPTION :
Calculate the mean value of the V*w for mode one, it is then used as a
reference offset.
==============================================================================*/
double EM_standard_mode_one(struct EM_Object *em_object);

int EM_calculate_minimum_nodeset(struct EM_Object *em_object, int number_of_modes);
/*******************************************************************************
LAST MODIFIED : 7 September 1999

DESCRIPTION :
Finds a independent set of nodes that are sufficient to distinguish the first
<number_of_modes> modes in the EM
==============================================================================*/
#endif
