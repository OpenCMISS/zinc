/*******************************************************************************
FILE: em_cmgui.c

LAST MODIFIED : 26 April 1998

DESCRIPTION :
EM stuff for DB and cmgui.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "general/debug.h"
#include "general/io_stream.h"
#include "general/message.h"
#include "emoter/em_cmgui.h"

const char magic[] = "em basis data";


/*******************************************************************************
LAST MODIFIED : 27 Febuary January 1998

DESCRIPTION :
Allocate the object that contains the EM data.
==============================================================================*/
struct EM_Object *create_EM_Object(int n_modes,int n_nodes)
{
	struct EM_Object *return_code, *em_object;

	ENTER(create_EM_Object);

	if (ALLOCATE(em_object,struct EM_Object,1))
	{
		em_object->n_modes = n_modes;
		em_object->n_nodes = n_nodes;
		em_object->m = n_nodes*3;
		em_object->mode_one_std = 0;
		em_object->minimum_nodes = (int *)NULL;
		em_object->number_of_minimum_nodes = 0;
		if (ALLOCATE(em_object->u,double,(em_object->m)*(em_object->n_modes)))
		{
			if (ALLOCATE(em_object->w,double,(em_object->n_modes)))
			{
				if (ALLOCATE(em_object->v,double,
					(em_object->n_modes)*(em_object->n_modes)))
				{
					if (ALLOCATE(em_object->index,int,(em_object->n_nodes)))
					{
						return_code = em_object;
					}
					else
					{
						DEALLOCATE(em_object->v);
						DEALLOCATE(em_object->w);
						DEALLOCATE(em_object->u);
						DEALLOCATE(em_object);
						display_message(ERROR_MESSAGE,"alloc_EM_Object: unable to allocate"
							" memory for em_object->index");
						return_code = NULL;
					}
				}
				else
				{
					DEALLOCATE(em_object->w);
					DEALLOCATE(em_object->u);
					DEALLOCATE(em_object);
					display_message(ERROR_MESSAGE,"alloc_EM_Object: unable to allocate "
						"memory for em_object->v");
					return_code = NULL;
				}
			}
			else
			{
				DEALLOCATE(em_object->u);
				DEALLOCATE(em_object);
				display_message(ERROR_MESSAGE,"alloc_EM_Object: unable to allocate "
					"memory for em_object->w");
				return_code = NULL;
			}
		}
		else
		{
			DEALLOCATE(em_object);
			display_message(ERROR_MESSAGE,"alloc_EM_Object: unable to allocate "
				"memory for em_object->u");
			return_code = NULL;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"alloc_EM_Object: unable to allocate "
			"memory for em_object");
		return_code = NULL;

	}
	LEAVE;

	return return_code;
}


/*******************************************************************************
LAST MODIFIED : 27 Febuary 1998

DESCRIPTION :
Destroy the object that contains the EM data.
==============================================================================*/
void destroy_EM_Object(struct EM_Object **em_object)
{
	ENTER(destroy_EM_Object);

	if (*em_object)
	{
	if ( (*em_object)->minimum_nodes )
	{
		DEALLOCATE( (*em_object)->minimum_nodes );
	}
		DEALLOCATE((*em_object)->index);
		DEALLOCATE((*em_object)->v);
		DEALLOCATE((*em_object)->w);
		DEALLOCATE((*em_object)->u);
		DEALLOCATE((*em_object));
	}

	LEAVE;
}


/*******************************************************************************
LAST MODIFIED : 26 Febuary 1998

DESCRIPTION :
Create a basis data file.
==============================================================================*/
void EM_write_basis(char *filename,struct EM_Object *em_object)
{
	int return_code;
	FILE* file;

	ENTER(write_basis);

	file = fopen(filename,"w");

	return_code = 1;
	
	if (1 != fwrite(magic,sizeof(magic),1,file))
	{
	   return_code = 0;
	}
	if (1 != fwrite(&(em_object->m),sizeof(int),1,file))
	{
	   return_code = 0;
	}		
	if (1 != fwrite(&(em_object->n_modes),sizeof(int),1,file))
	{
	   return_code = 0;
	}		
	if (0 == fwrite(em_object->index,sizeof(int),em_object->n_nodes,file))
	{
	   return_code = 0;
	}		
	if (0 == fwrite(em_object->u,sizeof(double),em_object->m*em_object->n_modes,file))
	{
	   return_code = 0;
	}		
	if (0 == fwrite(em_object->w,sizeof(double),em_object->n_modes,file))
	{
	   return_code = 0;
	}		
	if (0 == fwrite(em_object->v,
		sizeof(double),em_object->n_modes*em_object->n_modes, file))
	{
	   return_code = 0;
	}		

	fclose(file);
	if (!return_code)
	{
		display_message(ERROR_MESSAGE,"EM_write_basis.  Error writing file %s",
			filename);
	}
	
	LEAVE;
}

double EM_standard_mode_one(struct EM_Object *em_object)
/*******************************************************************************
LAST MODIFIED : 14 April 1998

DESCRIPTION :
Calculate the mean value of the V*w for mode one, it is then used as a
reference offset.
==============================================================================*/
{
	int i;
	double sum;

	if ( !em_object->mode_one_std )
	{
		sum = 0.0;
		for (i=0;i<em_object->n_modes;i++)
		{
			sum += em_object->v[i*em_object->n_modes];
		}
		sum *= em_object->w[0]/((double) em_object->n_modes);
		em_object->mode_one_std = sum;
	}
	return( em_object->mode_one_std );
}

struct EM_Object *EM_read_basis(char *filename,
	struct IO_stream_package *io_stream_package, struct EM_Object **em_object,
	int *node_index_list, int number_in_node_list)
/*******************************************************************************
LAST MODIFIED : 28 March 2007

DESCRIPTION :
Read in a file containing a basis function. Creates a EM_Object and returns
it, deallocating the old em_object and pointing it to the new one.
The <node_index_list> and <number_in_node_list> are required when the basis_file
is a version two file as these basis files do not include information about
the corresponding nodes.
==============================================================================*/
{
#define MAGICSIZE (13)
	char buff[MAGICSIZE + 1];
	char magic2[] = "em basis 2.0\n";
	struct IO_stream *stream;
	int i,j,n_modes,n_nodes;
	struct EM_Object *ptr;

	ENTER(EM_read_basis);
	ptr=(struct EM_Object *)NULL;
	/* check arguments */
	if (filename&&em_object)
	{
		if ((stream = CREATE(IO_stream)(io_stream_package))
			&& IO_stream_open_for_read(stream, filename))
		{
			IO_stream_fread(stream, buff,MAGICSIZE,1);
			buff[MAGICSIZE] = 0;
			if (0==strncmp(buff,magic,MAGICSIZE))
			{
				IO_stream_fread(stream, buff,1,1); /* The string NULL termination */
				IO_stream_fread(stream, &n_nodes,sizeof(int),1);
				IO_stream_fread(stream, &n_modes,sizeof(int),1);
				n_nodes=n_nodes/3;
				if (NULL != (ptr = create_EM_Object(n_modes,n_nodes)))
				{
					IO_stream_fread(stream, ptr->index,sizeof(int),   n_nodes);
					IO_stream_fread(stream, ptr->u,    sizeof(double),n_nodes*3*n_modes);
					IO_stream_fread(stream, ptr->w,    sizeof(double),n_modes);
					IO_stream_fread(stream, ptr->v,    sizeof(double),n_modes*n_modes);
					destroy_EM_Object(em_object);
					*em_object=ptr;
					ptr->minimum_nodes=(int *)NULL;
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"EM_read_basis.  not enough memory to allocate basis_object");
				}
			}
			/* SAB Don't compare the last character to support windows files */
			else if (strncmp(buff,magic2,MAGICSIZE-1) == 0)
			{
				/* SAB Read one more character if the previous one was a \r to support windows files */
				if (buff[MAGICSIZE-1] == '\r')
				{
					IO_stream_fread(stream, buff,1,1);					
				}

				/* Comment/title line */
				IO_stream_scan(stream, "%*[^\n]%*[\n]");
				 
				IO_stream_scan(stream, "%d%d", &n_nodes, &n_modes);
				n_nodes=n_nodes/3;
				if (NULL != (ptr = create_EM_Object(n_modes,n_nodes)))
				{
					for (i=0;i<n_modes;i++)
					{
						for (j=0;j<n_nodes * 3;j++)
						{
							IO_stream_scan(stream, "%lf", ptr->u + i * n_nodes * 3 + j);
						}
						ptr->w[i] = 1.0;
						ptr->v[i * n_modes] = 1.0;
						for (j=1;j<n_modes;j++)
						{
								ptr->v[i * n_modes + j] = 0.0;
						}
						ptr->v[i * n_modes + i] = 1.0;
					}
					if (number_in_node_list >= n_nodes)
					{
						for (i=0;i<n_nodes;i++)
						{
							ptr->index[i] = node_index_list[i];
						}
						destroy_EM_Object(em_object);
						*em_object=ptr;
						ptr->minimum_nodes=(int *)NULL;
					}
					else
					{
						destroy_EM_Object(&ptr);
						display_message(ERROR_MESSAGE,
							"EM_read_basis.  A node index group is required when loading a version 2 basis file to indicate active nodes\n"
							"   number nodes in basis: %d  number of nodes in index group %d\n", n_nodes, number_in_node_list);
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"EM_read_basis.  not enough memory to allocate basis_object");
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"EM_read_basis.  \"%s\" isn't a basis file",filename);
			}
			IO_stream_close(stream);
			DESTROY(IO_stream)(&stream);
		}
		else
		{
			display_message(ERROR_MESSAGE,"EM_read_basis.  Could not open file %s",
				filename);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"EM_read_basis.  Invalid argument(s)");
	}
	LEAVE;

	return (ptr);
} /* EM_read_basis */

/*******************************************************************************
LAST MODIFIED : 26 Febuary 1998

DESCRIPTION :
Returns the number of modes.
==============================================================================*/
int EM_number_of_modes(struct EM_Object *em_object)
{
	int return_code;

	ENTER(EM_number_of_modes);

	if (em_object)
	{
		return_code = em_object->n_modes;
	}
	else
	{
		display_message(ERROR_MESSAGE,"EM_number_of_modes: em_object not allocated");
		return_code = 0;
	}

	LEAVE;

	return return_code;
}

/*******************************************************************************
LAST MODIFIED : 26 Febuary 1998

DESCRIPTION :
Returns the number of nodes.
==============================================================================*/
int EM_number_of_nodes(struct EM_Object *em_object)
{
	int return_code;

	ENTER(EM_number_of_nodes);

	if (em_object)
	{
		return_code = em_object->n_nodes;
	}
	else
	{
		display_message(ERROR_MESSAGE,"EM_number_of_nodes: em_object not allocated");
		return_code = 0;
	}

	LEAVE;

	return return_code;
}

/*******************************************************************************
LAST MODIFIED : 26 Febuary 1998

DESCRIPTION :
Reconstructs a face from a EM_Object and a set of weights. Returns a
pointer to a vector of double, of size 3*n_nodes, contains the x,y,z
coordinates of the reconstructed face, in order x1,y1,z1,x2,y2,z2, ...
Weights must be of size n_modes.
==============================================================================*/
double *EM_reconstruct(double *weights,struct EM_Object *em_object)
{
	int i,j;
	double *ptr;

	ENTER(EM_reconstruct);

	/* allocate memory */

	if (ALLOCATE(ptr,double,(em_object->n_nodes)*3))
	{

		/* construct a face */

		for (i=0;i<em_object->m;i++)
		{
			ptr[i] = 0.0;

			for (j=0;j<em_object->n_modes;j++)
			{
				ptr[i] += em_object->u[(i)+(j*(em_object->m))]*weights[j];
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"EM_reconstruct: cannot allocate memory");
	}

	LEAVE;

	return ptr;
}


/*******************************************************************************
LAST MODIFIED : 26 Febuary 1998

DESCRIPTION :
Returns the max/min weights for the em data. The arrays max and min must be
of size n_modes.
==============================================================================*/
void EM_get_weights(double *max,double *min,struct EM_Object *em_object)
{
	int i,j;

	ENTER(EM_get_weights);

	/* the first value is the average of the major mode */

	max[0] = 0.0;
	for (i=0;i<em_object->n_modes;i++)
	{
		max[0] += em_object->v[i*em_object->n_modes];
	}
	max[0] *= em_object->w[0]/((double) em_object->n_modes);
	min[0] = max[0];

	/* the other values are the max/min values of the pertibations */

	for (i=1;i<em_object->n_modes;i++)
	{
		max[i] = em_object->v[(i)];
		min[i] = em_object->v[(i)];

		for (j=1;j<em_object->n_modes;j++)
		{
			double d = em_object->v[(j*(em_object->n_modes))+i];

			max[i] = max[i] > d ? max[i] : d;
			min[i] = min[i] < d ? min[i] : d;
		}
		max[i] *= em_object->w[i];
		min[i] *= em_object->w[i];
	}

	LEAVE;
}


/*******************************************************************************
LAST MODIFIED : 26 Febuary 1998

DESCRIPTION :
Deconstruct a face. x must be of size n_nodes*3, and contains the coordinates
of the face to be deconstructed, in order x1,y1,z1,x2,y2,z2,.... index contains
the node numbers for face points.
An array of size n_modes is allocated and returned. This contains the
weights for each face mode.
==============================================================================*/
double *EM_project(double *x,int *index,struct EM_Object *em_object)
{
	int i,j,k,node_loc;
	double *ptr;

	ENTER(EM_get_weights);

	/* allocate memory */

	if (ALLOCATE(ptr,double,(em_object->n_modes)))
	{

		/* deconstruct the face */

		for (i=0;i<em_object->n_modes;i++)
		{
			ptr[i] = 0.0;
			for (j=0;j<em_object->n_nodes;j++)
			{
				/* find the corresponding node location in the em data set
					to account for cases where the node number order varies between
					the em basis file data and the coordinate data entered in
					x and index */

				node_loc = -1;
				for (k=0;k<em_object->n_nodes;k++)
				{
					if (index[j] == em_object->index[k])
					{
						node_loc = k;
					}
				}
				if (node_loc == -1)
				{
					display_message(ERROR_MESSAGE,"EM_deconstruct: index's dont match."
						" Cannot find index %d\n",index[j]);
				}

				for (k=0;k<3;k++)
				{
					ptr[i] += em_object->u[(i*(em_object->m))+3*node_loc+k]*x[3*j+k];
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"EM_deconstruct: cannot allocate memory");
	}

	LEAVE;

	return ptr;
}

int EM_calculate_minimum_nodeset(struct EM_Object *em_object, int number_of_modes)
/*******************************************************************************
LAST MODIFIED : 7 September 1999

DESCRIPTION :
Finds a independent set of nodes that are sufficient to distinguish the first
<number_of_modes> modes in the EM
==============================================================================*/
{
	double greatest_value, *ucopy, factor;
	float coordinate_tolerance = 1e-5f;
	FILE *node_file;
	int i, j, *free_coordinate, mode, *nodeset, best_coordinate = 0, return_code;

	ENTER(EM_calculate_minimum_nodeset);

	if ( em_object )
	{
		em_object->number_of_minimum_nodes = 0;
		if ( ALLOCATE( nodeset, int, number_of_modes) &&
			ALLOCATE(ucopy,double,(em_object->m)*(em_object->n_modes)) &&
			ALLOCATE(free_coordinate,int,(em_object->m)))
		{
			if ( NULL != (node_file = fopen("minimum_set.exnode", "w")))
			{
				return_code = 1;
				em_object->minimum_nodes = nodeset;
				fprintf( node_file, "Group name: minimum_set\n #Fields=0\n");
				memcpy( ucopy, em_object->u, sizeof(double) *
					(em_object->m)*(em_object->n_modes));
				for ( i = 0 ; i < em_object->m ; i++ )
				{
					free_coordinate[i] = 1;
				}
				for ( mode = 0 ; mode < number_of_modes ; mode++ )
				{
					greatest_value = 0;
					for ( i = 0 ; i < em_object->m ; i++ )
					{
						if ( free_coordinate[i] &&
							(fabs( ucopy[mode * em_object->m + i] ) > greatest_value) )
						{
							greatest_value = fabs( ucopy[mode * em_object->m + i]);
							best_coordinate = i;
						}
					}
					if ( greatest_value >= coordinate_tolerance )
					{
						free_coordinate[best_coordinate] = 0;
						nodeset[em_object->number_of_minimum_nodes] = em_object->index[best_coordinate / 3];
						/* Debug
							printf("Minimum set   node %d coordinate %d in %d (%d)\n",
							em_object->index[best_coordinate / 3], best_coordinate % 3,
							best_coordinate / 3, best_coordinate);
						*/
						fprintf(node_file, "Node: %d\n", em_object->index[best_coordinate / 3]);
						(em_object->number_of_minimum_nodes)++;

						/* Now factor this coordinate direction from all the other coordinates */
						for ( i = 0 ; i < em_object->m ; i++ )
						{
							if ( free_coordinate[i] )
							{
								factor = ucopy[mode * em_object->m + i]
									/ ucopy[mode * em_object->m + best_coordinate];
								for ( j = mode ; j < em_object->n_modes ; j++ )
								{
									ucopy[j * em_object->m + i] -= factor *
										ucopy[j * em_object->m + best_coordinate];
								}
							}
						}

					}
					else
					{
						/* SAB To flag an node shape that a node could not be found for */
						nodeset[em_object->number_of_minimum_nodes] = -1;
						(em_object->number_of_minimum_nodes)++;
						display_message(WARNING_MESSAGE,"EM_calculate_minimum_nodeset: Unable to find node coordinate for mode %d\n", mode + 1);
					}
				}
				fclose(node_file);
			}
			else
			{
				display_message(ERROR_MESSAGE,"EM_calculate_minimum_nodeset: Unable to write minimum_nodeset file");
				return_code = 0;
				DEALLOCATE( nodeset );
			}
			DEALLOCATE( ucopy );
			DEALLOCATE( free_coordinate );
		}
		else
		{
			display_message(ERROR_MESSAGE,"EM_calculate_minimum_nodeset: cannot allocate memory");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"EM_calculate_minimum_nodeset: em_object not found");
		return_code = 0;
	}

	LEAVE;

	return( return_code );
} /* EM_calculate_minimum_nodeset */
