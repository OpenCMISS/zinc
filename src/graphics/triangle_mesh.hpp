/***************************************************************************//**
 * FILE : triangle_mesh.hpp
 *
 * Class for representing a simple linear triangular mesh with facilities for
 * merging vertices whose coordinates are within a tolerance.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (TRIANGLE_MESH)
#define TRIANGLE_MESH

#include <list>
#include <set>

#include "general/octree.h"
#include "graphics/auxiliary_graphics_types.h"

class Triangle_vertex
{
private:
	ZnReal coordinates[3];
	size_t identifier;

	friend class Triangle_vertex_compare;
	friend class Mesh_triangle;
	friend class Triangle_mesh;

public:
	Triangle_vertex(const ZnReal *in_coordinates) :
		identifier(0)
	{
		coordinates[0] = in_coordinates[0];
		coordinates[1] = in_coordinates[1];
		coordinates[2] = in_coordinates[2];
	}
	
	~Triangle_vertex()
	{
	}

	void get_coordinates(ZnReal *coord1, ZnReal *coord2, ZnReal *coord3) const
	{
		*coord1 = coordinates[0];
		*coord2 = coordinates[1];
		*coord3 = coordinates[2];
	}

	void get_coordinates(double outValues3[3]) const
	{
		outValues3[0] = coordinates[0];
		outValues3[1] = coordinates[1];
		outValues3[2] = coordinates[2];
	}

	void set_identifier(size_t in_identifier)
	{
		identifier = in_identifier;
	}
	
	size_t get_identifier() const
	{
		return identifier;
	}
	
	void list() const;
};

/***************************************************************************//**
 * Functor for comparing/sorting Triangle vertex based on coordinates but with
 * an allowed tolerance.
 */
class Triangle_vertex_compare
{
private:
	ZnReal tolerance;

public:
	Triangle_vertex_compare(ZnReal in_tolerance) :
		tolerance(in_tolerance)
	{	
	}

	bool operator() (const Triangle_vertex *v1, const Triangle_vertex *v2) const;
};

typedef std::set<Triangle_vertex*,Triangle_vertex_compare> Triangle_vertex_set;
typedef std::set<Triangle_vertex*,Triangle_vertex_compare>::iterator Triangle_vertex_set_iterator;
typedef std::set<Triangle_vertex*,Triangle_vertex_compare>::const_iterator Triangle_vertex_set_const_iterator;

class Mesh_triangle
{
private:
	const Triangle_vertex *trivertex1;
	const Triangle_vertex *trivertex2;
	const Triangle_vertex *trivertex3;
	
public:
	Mesh_triangle(const Triangle_vertex *in_vertex1,
		const Triangle_vertex *in_vertex2, const Triangle_vertex *in_vertex3) :
		trivertex1(in_vertex1), trivertex2(in_vertex2), trivertex3(in_vertex3)
	{
	}
	
	void get_vertexes(const Triangle_vertex **vertex1,
		const Triangle_vertex **vertex2, const Triangle_vertex **vertex3) const;
	
	void list() const;
};

typedef std::list<Mesh_triangle*> Mesh_triangle_list;
typedef std::list<Mesh_triangle*>::iterator Mesh_triangle_list_iterator;
typedef std::list<Mesh_triangle*>::const_iterator Mesh_triangle_list_const_iterator;

class Triangle_mesh
{
private:
	Triangle_vertex_compare compare;
	Triangle_vertex_set vertex_set;
	Mesh_triangle_list triangle_list;
	struct Octree *octree;
	struct LIST(Octree_object) *nearby_vertex;
	
public:
	Triangle_mesh(ZnReal tolerance) :
		compare(tolerance),
		vertex_set(compare),
		triangle_list(),
		octree(CREATE(Octree)()),
		nearby_vertex(CREATE(LIST(Octree_object))())
	{
	}

	~Triangle_mesh();
	
	/***************************************************************************//**
	 * Either finds an existing vertex within the tolerance of the supplied
	 * coordinates, or creates one.
	 *
	 * @param coordinates  Pointer to 3 GLfloat values giving x, y, z coordinates. 
	 * @return  Pointer to const vertex with supplied coordinates or within mesh
	 * tolerance thereof.
	 */
	const Triangle_vertex *add_vertex(const Triple coordinates);

	/***************************************************************************//**
	 * Adds a triangle to the mesh. Degenerate triangles - with repeared vertices
	 * are ignored. Triangle vertices must have anticlockwise winding for normal out
	 * of 2-D screen/page:
	 *   3
	 *   | \
	 *   1--2
	 *   
	 * @return  On success, const pointer to new triangle, or NULL on failure or
	 * degenerate triangle.
	 */
	const Mesh_triangle *add_triangle(const Triangle_vertex *v1,
		const Triangle_vertex *v2, const Triangle_vertex *v3);
	
	void add_triangle_coordinates(const Triple c1, const Triple c2, const Triple c3)
	{
		add_triangle(add_vertex(c1), add_vertex(c2), add_vertex(c3));
	}

	/***************************************************************************//**
	 * Adds a quadrilateral symmetrically split into four triangles meeting at a new
	 * centre vertex at the average of the coordinates of the four corner vertices.
	 * @param v1,v2,v3,v4  Vertices in following order (for normal towards reader):
	 *   3---4
	 *   |\ /|
	 *   | c |
	 *   |/ \|
	 *   1---2
	 */
	void add_quadrilateral(const Triangle_vertex *v1, const Triangle_vertex *v2,
		const Triangle_vertex *v3, const Triangle_vertex *v4);

	void add_quadrilateral_coordinates(const Triple c1, const Triple c2,
		const Triple c3, const Triple c4)
	{
		add_quadrilateral(add_vertex(c1), add_vertex(c2), add_vertex(c3), add_vertex(c4));
	}

	void set_vertex_identifiers(size_t first_identifier);
	
	const Triangle_vertex_set& get_vertex_set() const
	{
		return vertex_set;
	}

	const Mesh_triangle_list& get_triangle_list() const
	{
		return triangle_list;
	}
	
	void list() const;

private:
	// declared but not defined so illegal:
	Triangle_mesh(const Triangle_mesh& in_triangle_mesh);

	// declared but not defined so illegal:
	void operator=(const Triangle_mesh& in_triangle_mesh);
};

#endif /* !defined (TRIANGLE_MESH) */
