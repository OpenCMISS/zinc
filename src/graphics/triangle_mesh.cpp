/*****************************************************************************//**
 * FILE : triangle_mesh.cpp
 * 
 * Class for representing a simple linear triangular mesh with facilities for
 * merging vertices whose coordinates are within a tolerance.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "general/debug.h"
#include "general/message.h"
#include "graphics/auxiliary_graphics_types.h"
#include "triangle_mesh.hpp"

void Triangle_vertex::list() const
{
	
	display_message(INFORMATION_MESSAGE, "identifier %i coords   %g,%g,%g\n",
		identifier, coordinates[0],coordinates[1],coordinates[2]);
}

void Mesh_triangle::get_vertexes(const Triangle_vertex **vertex1,
	const Triangle_vertex **vertex2, const Triangle_vertex **vertex3) const
{
	*vertex1 = trivertex1;
	*vertex2 = trivertex2;
	*vertex3 = trivertex3;
}

void Mesh_triangle::list() const
{
	trivertex1->list();
	trivertex2->list();
	trivertex3->list();
}

bool Triangle_vertex_compare::operator() (const Triangle_vertex *v1, const Triangle_vertex *v2) const
{
	if (v1->coordinates[2] < (v2->coordinates[2] - tolerance))
	{
		return true;
	}
	else if (v1->coordinates[2] > (v2->coordinates[2] + tolerance))
	{
		return false;
	}
	else
	{
		if (v1->coordinates[1] < (v2->coordinates[1] - tolerance))
		{
			return true;
		}
		else if (v1->coordinates[1] > (v2->coordinates[1] + tolerance))
		{
			return false;
		}
		else
		{
			if (v1->coordinates[0] < (v2->coordinates[0] - tolerance))
			{
				return true;
			}
			else
			{
				return false;
			}
		}
	}
}

Triangle_mesh::~Triangle_mesh()
{
	for (Mesh_triangle_list_iterator triangle_iter = triangle_list.begin(); triangle_iter != triangle_list.end(); triangle_iter++)
	{
		Mesh_triangle *triangle = (*triangle_iter);
		delete triangle;
	}
	for (Triangle_vertex_set_iterator vertex_iter = vertex_set.begin(); vertex_iter != vertex_set.end(); vertex_iter++)
	{
		Triangle_vertex *vertex = (*vertex_iter);
		delete vertex;
	}
	DESTROY(LIST(Octree_object))(&nearby_vertex);
	DESTROY(Octree)(&octree);
}

const Triangle_vertex *Triangle_mesh::add_vertex(const Triple coordinates)
{
	Triangle_vertex *vertex_in_set = NULL;
	FE_value coordinates_FEValue[3];
	coordinates_FEValue[0] = (FE_value)coordinates[0];
	coordinates_FEValue[1] = (FE_value)coordinates[1];
	coordinates_FEValue[2] = (FE_value)coordinates[2];
	Octree_add_objects_near_coordinate_to_list(octree,
		/*dimension*/3, coordinates_FEValue, /*tolerance*/ 0.000001, nearby_vertex);
	if (0 == NUMBER_IN_LIST(Octree_object)(nearby_vertex))
	{
		vertex_in_set = new Triangle_vertex(coordinates_FEValue);
		vertex_in_set->set_identifier(vertex_set.size() + 1);
		Octree_object *octree_vertex = CREATE(Octree_object)(/*dimension*/3, coordinates_FEValue);
		Octree_object_set_user_data(octree_vertex, (void *)vertex_in_set);
		Octree_add_object(octree, octree_vertex);
		vertex_set.insert(vertex_in_set);
	}
	else
	{
		struct Octree_object *nearest_octree_object =
			Octree_object_list_get_nearest(nearby_vertex, coordinates_FEValue);
		REMOVE_ALL_OBJECTS_FROM_LIST(Octree_object)(nearby_vertex);
		vertex_in_set = (Triangle_vertex *)Octree_object_get_user_data(nearest_octree_object);
	}

	return vertex_in_set;
}

const Mesh_triangle *Triangle_mesh::add_triangle(const Triangle_vertex *vertex1,
	const Triangle_vertex *vertex2, const Triangle_vertex *vertex3)
{
	// ignore degenerate triangles:
	if ((vertex1 == vertex2) || (vertex2 == vertex3) || (vertex3 == vertex1))
	{
		return NULL;
	}
	Mesh_triangle *triangle = new Mesh_triangle(vertex1, vertex2, vertex3);
	triangle_list.push_back(triangle);

	return triangle;
}

void Triangle_mesh::add_quadrilateral(const Triangle_vertex *v1, const Triangle_vertex *v2,
	const Triangle_vertex *v3, const Triangle_vertex *v4)
{
	GLfloat centre[3];
	centre[0] = 0.25*(v1->coordinates[0] + v2->coordinates[0] + v3->coordinates[0] + v4->coordinates[0]);
	centre[1] = 0.25*(v1->coordinates[1] + v2->coordinates[1] + v3->coordinates[1] + v4->coordinates[1]);
	centre[2] = 0.25*(v1->coordinates[2] + v2->coordinates[2] + v3->coordinates[2] + v4->coordinates[2]);
	const Triangle_vertex *vc = add_vertex(centre);
	add_triangle(v1, v2, vc);
	add_triangle(v2, v4, vc);
	add_triangle(v4, v3, vc);
	add_triangle(v3, v1, vc);
}

void Triangle_mesh::set_vertex_identifiers(size_t first_identifier)
{
	size_t i = first_identifier;
	for (Triangle_vertex_set_iterator iter = vertex_set.begin(); iter != vertex_set.end(); iter++)
	{
		(*iter)->set_identifier(i);
		i++;
	}
}

void Triangle_mesh::list() const
{
	int i = 0;
	display_message(INFORMATION_MESSAGE, "Set contents:\n");
	for (Mesh_triangle_list_const_iterator iter = triangle_list.begin(); iter != triangle_list.end(); iter++)
	{
		display_message(INFORMATION_MESSAGE, "Triangle[%d] : ",i);
		(*iter)->list();
		i++;
	}
}
