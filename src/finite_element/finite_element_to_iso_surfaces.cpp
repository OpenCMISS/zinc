/***************************************************************************//**
 * FILE : finite_element_to_iso_surfaces.cpp
 *
 * Functions for creating graphical iso-surfaces from finite element fields.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <list>
#include <map>
#include "opencmiss/zinc/differentialoperator.h"
#include "opencmiss/zinc/fieldcache.h"
#include "opencmiss/zinc/mesh.h"
#include "opencmiss/zinc/status.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_wrappers.h"
#include "computed_field/field_cache.hpp"
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_discretization.h"
#include "finite_element/finite_element_to_graphics_object.h"
#include "finite_element/finite_element_to_iso_surfaces.h"
#include "general/debug.h"
#include "general/matrix_vector.h"
#include "graphics/graphics_object.hpp"
#include "graphics/volume_texture.h"
#include "general/message.h"

/*
Global types
------------
*/

struct Iso_surface_specification
{
public:
	double first_iso_value, *iso_values, iso_value_range, last_iso_value;
	int number_of_data_components, number_of_iso_values;
	struct Computed_field *coordinate_field, *data_field, *scalar_field,
		*texture_coordinate_field;

public:
	double get_iso_value(int iso_value_number) const;
};

/*
Module types and functions
--------------------------
*/

inline double Iso_surface_specification::get_iso_value(int iso_value_number) const
{
	if (NULL != iso_values)
	{
		return iso_values[iso_value_number];
	}
	return (first_iso_value + (double)iso_value_number * iso_value_range);
}

namespace {

class Point_index
{
public:
	bool exact_xi;
	int i, j, k;
	FE_value xi1, xi2, xi3;
	double scalar;

	Point_index() :
		exact_xi(false), i(0), j(0), k(0)
	{
	}

	Point_index(int i, int j, int k) :
		exact_xi(false), i(i), j(j), k(k)
	{
	}

	Point_index(FE_value xi[3], double scalar) :
		exact_xi(true), xi1(xi[0]), xi2(xi[1]), xi3(xi[2]), scalar(scalar)
	{
	}

	void set(int in_i, int in_j, int in_k)
	{
		exact_xi = false;
		i = in_i;
		j = in_j;
		k = in_k;
	}

	bool operator<(const Point_index& other) const
	{
		// index < exact_xi
		if (exact_xi)
		{
			if (other.exact_xi)
			{
				return ((xi1 < other.xi1) || ((xi1 == other.xi1) &&
					((xi2 < other.xi2) || ((xi2 == other.xi2) && (xi3 < other.xi3)))));
			}
			return false;
		}
		else if (other.exact_xi)
		{
			return true;
		}
		return ((i < other.i) || ((i == other.i) &&
			((j < other.j) || ((j == other.j) && (k < other.k)))));
	}
};

class Point_index_pair
{
public:
	Point_index pa;
	Point_index pb;

	Point_index_pair(const Point_index& in_pa, const Point_index& in_pb)
	{
		// ensure pa < pb
		if (in_pa < in_pb)
		{
			pa = in_pa;
			pb = in_pb;
		}
		else
		{
			pa = in_pb;
			pb = in_pa;
		}
	}
};

class Point_index_pair_compare
{
public:
	bool operator() (const Point_index_pair &pp1, const Point_index_pair &pp2) const
	{
		return ((pp1.pa < pp2.pa) || ((!(pp2.pa < pp1.pa)) && (pp1.pb < pp2.pb)));
	}
};

class Iso_vertex
{
public:
	FE_value xi[3];
	FE_value coordinates[3];
	FE_value normal[3];
	FE_value texture_coordinates[3];
	FE_value *data;

	Iso_vertex() :
		data(NULL)
	{
		xi[0] = 0.0;
		xi[1] = 0.0;
		xi[2] = 0.0;
		coordinates[0] = 0.0;
		coordinates[1] = 0.0;
		coordinates[2] = 0.0;
		normal[0] = 0.0;
		normal[1] = 0.0;
		normal[2] = 0.0;
		texture_coordinates[0] = 0.0;
		texture_coordinates[1] = 0.0;
		texture_coordinates[2] = 0.0;
	}

	~Iso_vertex()
	{
		delete[] data;
	}

	/** takes ownership of supplied data array */
	void set_data(FE_value *in_data)
	{
		// note: not expecting to set twice
		data = in_data;
	}

private:
	// following methods are private and undefined to prevent copy
	Iso_vertex(const Iso_vertex& in_vertex);

	void operator=(const Iso_vertex& in_vertex);
};

inline FE_value square_distance3(const FE_value *c1, const FE_value *c2)
{
	FE_value offset0 = c2[0] - c1[0];
	FE_value offset1 = c2[1] - c1[1];
	FE_value offset2 = c2[2] - c1[2];
	return (offset0*offset0 + offset1*offset1 + offset2*offset2);
}

typedef std::map<Point_index_pair, Iso_vertex*, Point_index_pair_compare> Iso_vertex_map;
typedef std::map<Point_index_pair, Iso_vertex*, Point_index_pair_compare>::iterator Iso_vertex_map_iterator;
typedef std::map<Point_index_pair, Iso_vertex*, Point_index_pair_compare>::const_iterator Iso_vertex_map_const_iterator;

class Iso_triangle
{
public:
	const Iso_vertex *v1, *v2, *v3;

	Iso_triangle(const Iso_vertex *v1, const Iso_vertex *v2, const Iso_vertex *v3) :
		v1(v1), v2(v2), v3(v3)
	{
	}
};

typedef std::list<Iso_triangle*> Iso_triangle_list;
typedef std::list<Iso_triangle*>::iterator Iso_triangle_list_iterator;
typedef std::list<Iso_triangle*>::const_iterator Iso_triangle_list_const_iterator;

class Iso_mesh
{
public:
	Iso_vertex_map vertex_map;
	Iso_triangle_list triangle_list;

	Iso_mesh() :
		vertex_map(Point_index_pair_compare()),
		triangle_list()
	{
	}

	~Iso_mesh()
	{
		for (Iso_triangle_list_iterator triangle_iter = triangle_list.begin(); triangle_iter != triangle_list.end(); triangle_iter++)
		{
			delete (*triangle_iter);
		}
		for (Iso_vertex_map_iterator vertex_iter = vertex_map.begin(); vertex_iter != vertex_map.end(); vertex_iter++)
		{
			Iso_vertex *vertex = vertex_iter->second;
			delete (vertex);
		}
	}

	const Iso_vertex *get_vertex(const Point_index_pair& pp) const
	{
		Iso_vertex_map_const_iterator iter = vertex_map.find(pp);
		if (iter != vertex_map.end())
		{
			return (iter->second);
		}
		return NULL;
	}

	/** transfers ownership of vertex to mesh */
	const Iso_vertex *add_vertex(const Point_index_pair& pp, Iso_vertex* vertex)
	{
		vertex_map[pp] = vertex;
		return vertex;
	}

	/**
	 *      3
	 *     / \
	 *    /   \
	 *   1-----2
	 */
	void add_triangle(const Iso_vertex *v1,	const Iso_vertex *v2,
		const Iso_vertex *v3)
	{
		// ignore degenerate triangles
		if ((0.0 == square_distance3(v1->coordinates, v2->coordinates)) ||
			(0.0 == square_distance3(v2->coordinates, v3->coordinates)) ||
			(0.0 == square_distance3(v3->coordinates, v1->coordinates)))
		{
			return;
		}
		triangle_list.push_back(new Iso_triangle(v1, v2, v3));
	}

	void add_triangle(const Iso_vertex *v1,	const Iso_vertex *v2,
		const Iso_vertex *v3, bool reverse_winding)
	{
		if (reverse_winding)
		{
			add_triangle(v3, v2, v1);
		}
		else
		{
			add_triangle(v1, v2, v3);
		}
	}

	/**
	 *   4-----3
	 *   |     |
	 *   |     |
	 *   1-----2
	 */
	void add_quadrilateral(const Iso_vertex *v1,	const Iso_vertex *v2,
		const Iso_vertex *v3, const Iso_vertex *v4)
	{
		// cut quadrilateral across the shortest diagonal
		if (square_distance3(v1->coordinates, v3->coordinates) <
			square_distance3(v2->coordinates, v4->coordinates))
		{
			add_triangle(v1, v2, v3);
			add_triangle(v1, v3, v4);
		}
		else
		{
			add_triangle(v1, v2, v4);
			add_triangle(v2, v3, v4);
		}
	}

	void add_quadrilateral(const Iso_vertex *v1,	const Iso_vertex *v2,
		const Iso_vertex *v3, const Iso_vertex *v4, bool reverse_winding)
	{
		if (reverse_winding)
		{
			add_quadrilateral(v4, v3, v2, v1);
		}
		else
		{
			add_quadrilateral(v1, v2, v3, v4);
		}
	}

	/**
	 *   5----4
	 *   |     \
	 *   |      3
	 *   |     /
	 *   1----2
	 */
	void add_pentagon(const Iso_vertex *v1,	const Iso_vertex *v2,
		const Iso_vertex *v3, const Iso_vertex *v4, const Iso_vertex *v5)
	{
		// cut pentagon across the shortest diagonal
		FE_value distance13 = square_distance3(v1->coordinates, v3->coordinates);
		FE_value distance24 = square_distance3(v2->coordinates, v4->coordinates);
		FE_value distance35 = square_distance3(v3->coordinates, v5->coordinates);
		FE_value distance41 = square_distance3(v4->coordinates, v1->coordinates);
		FE_value distance52 = square_distance3(v5->coordinates, v2->coordinates);
		if ((distance13 < distance24) && (distance13 < distance35) &&
			(distance13 < distance41) && (distance13 < distance52))
		{
			add_triangle(v1, v2, v3);
			add_quadrilateral(v1, v3, v4, v5);
		}
		else if ((distance24 < distance35) && (distance24 < distance41) &&
			(distance24 < distance52))
		{
			add_triangle(v2, v3, v4);
			add_quadrilateral(v1, v2, v4, v5);
		}
		else if ((distance35 < distance41) && (distance35 < distance52))
		{
			add_triangle(v3, v4, v5);
			add_quadrilateral(v1, v2, v3, v5);
		}
		else if (distance41 < distance52)
		{
			add_triangle(v1, v4, v5);
			add_quadrilateral(v1, v2, v3, v4);
		}
		else
		{
			add_triangle(v1, v2, v5);
			add_quadrilateral(v2, v3, v4, v5);
		}
	}

	void add_pentagon(const Iso_vertex *v1,	const Iso_vertex *v2,
		const Iso_vertex *v3, const Iso_vertex *v4, const Iso_vertex *v5,
		bool reverse_winding)
	{
		if (reverse_winding)
		{
			add_pentagon(v5, v4, v3, v2, v1);
		}
		else
		{
			add_pentagon(v1, v2, v3, v4, v5);
		}
	}

	/**
	 *    5----4
	 *   /      \
	 *  6        3
	 *   \      /
	 *    1----2
	 */
	void add_hexagon(const Iso_vertex *v1,	const Iso_vertex *v2, const Iso_vertex *v3,
		const Iso_vertex *v4, const Iso_vertex *v5, const Iso_vertex *v6)
	{
		// cut hexagon into two quads across the shortest opposite diagonal
		FE_value distance14 = square_distance3(v1->coordinates, v4->coordinates);
		FE_value distance25 = square_distance3(v2->coordinates, v5->coordinates);
		FE_value distance36 = square_distance3(v3->coordinates, v6->coordinates);
		if ((distance14 < distance25) && (distance14 < distance36))
		{
			add_quadrilateral(v1, v2, v3, v4);
			add_quadrilateral(v1, v4, v5, v6);
		}
		else if (distance25 < distance36)
		{
			add_quadrilateral(v1, v2, v5, v6);
			add_quadrilateral(v2, v3, v4, v5);
		}
		else
		{
			add_quadrilateral(v1, v2, v3, v6);
			add_quadrilateral(v3, v4, v5, v6);
		}
	}

private:
	// declared but not defined so illegal:
	Iso_mesh(const Iso_mesh& in_Iso_mesh);

	// declared but not defined so illegal:
	void operator=(const Iso_mesh& in_Iso_mesh);
};

class Marching_cube
{
	unsigned char cases[256][10];

public:
	Marching_cube();

	unsigned char final_case(unsigned char unrotated_case)
	{
		return cases[unrotated_case][0];
	}

	unsigned char rotated_vertex(unsigned char unrotated_case, unsigned char unrotated_vertex)
	{
		return cases[unrotated_case][unrotated_vertex + 1];
	}

	bool inverse(unsigned char unrotated_case)
	{
		return (0 != cases[unrotated_case][9]);
	}
};

Marching_cube::Marching_cube()
{
	// all cube orientations = 4 rotations of 6 cube faces
	static const unsigned char cube_orientation[24][8] =
	{
			{ 0, 1, 2, 3, 4, 5, 6, 7 },
			{ 2, 3, 6, 7, 0, 1, 4, 5 },
			{ 6, 7, 4, 5, 2, 3, 0, 1 },
			{ 4, 5, 0, 1, 6, 7, 2, 3 },

			{ 3, 2, 1, 0, 7, 6, 5, 4 },
			{ 1, 0, 5, 4, 3, 2, 7, 6 },
			{ 5, 4, 7, 6, 1, 0, 3, 2 },
			{ 7, 6, 3, 2, 5, 4, 1, 0 },

			{ 1, 3, 0, 2, 5, 7, 4, 6 },
			{ 0, 2, 4, 6, 1, 3, 5, 7 },
			{ 4, 6, 5, 7, 0, 2, 1, 3 },
			{ 5, 7, 1, 3, 4, 6, 0, 2 },

			{ 2, 0, 3, 1, 6, 4, 7, 5 },
			{ 3, 1, 7, 5, 2, 0, 6, 4 },
			{ 7, 5, 6, 4, 3, 1, 2, 0 },
			{ 6, 4, 2, 0, 7, 5, 3, 1 },

			{ 0, 4, 1, 5, 2, 6, 3, 7 },
			{ 1, 5, 3, 7, 0, 4, 2, 6 },
			{ 3, 7, 2, 6, 1, 5, 0, 4 },
			{ 2, 6, 0, 4, 3, 7, 1, 5 },

			{ 5, 1, 4, 0, 7, 3, 6, 2 },
			{ 4, 0, 6, 2, 5, 1, 7, 3 },
			{ 6, 2, 7, 3, 4, 0, 5, 1 },
			{ 7, 3, 5, 1, 6, 2, 4, 0 }
	};
	// final crossing cases; second byte = 1 means also handle inverse
	static const unsigned char final_cases[14][2] =
	{
			{   1, 1 },
			{   3, 1 },
			{   7, 1 },
			{   9, 1 },
			{  15, 0 },
			{  23, 0 },
			{  39, 0 },
			{  41, 1 },
			{  67, 1 },
			{  71, 0 },
			{ 105, 0 },
			{ 129, 1 },
			{ 135, 0 },
			{ 195, 0 }
	};
	static const unsigned char power2[8] =	{ 1, 2, 4, 8, 16, 32, 64, 128 };

	ENTER(Marching_cube::Marching_cube);
	memset(cases, 0, sizeof(cases));
	for (int f = 0; f < 14; f++)
	{
		unsigned char final_case = final_cases[f][0];
		for (unsigned char inverse = 0; inverse <= final_cases[f][1]; inverse++)
		{
			unsigned char test_case = inverse ? ( final_case ^ 0xFF) : final_case;
			for (unsigned char orient = 0; orient < 24; orient++)
			{
				unsigned char unrotated_case = 0;
				unsigned char bit = 1;
				unsigned char point;
				for (point = 0; point < 8; point++)
				{
					if (test_case & bit)
					{
						unrotated_case += power2[cube_orientation[orient][point]];
					}
					bit <<= 1;
				}
				if (0 == cases[unrotated_case][0])
				{
					cases[unrotated_case][0] = final_case;
					for (point = 0; point < 8; point++)
					{
						cases[unrotated_case][point+1] = cube_orientation[orient][point];
					}
					cases[unrotated_case][9] = inverse;
				}
			}
		}
	}
#if defined (TEST_CODE)
	display_message(INFORMATION_MESSAGE, "In Marching_cube::Marching_cube\n");
	unsigned char final_case_count[14][2];
	memset(final_case_count, 0, sizeof(final_case_count));
	for (int i = 0; i < 256; i++)
	{
		for (int j = 0; j < 14; j++)
		{
			if (cases[i][0] == final_cases[j][0])
			{
				++(final_case_count[j][ cases[i][9] ]);
			}
		}
		display_message(INFORMATION_MESSAGE, "case[%d] -> %d  : %d %d %d %d %d %d %d %d : %d\n",
			i,
			cases[i][0], cases[i][1], cases[i][2], cases[i][3], cases[i][4],
			cases[i][5], cases[i][6], cases[i][7], cases[i][8], cases[i][9]);
	}
	display_message(INFORMATION_MESSAGE, "\nCube case summary:\n");
	int total_cases = 2;
	for (int j = 0; j < 14; j++)
	{
		display_message(INFORMATION_MESSAGE, "case[%3d] : %2d , %2d\n",
			final_cases[j][0], final_case_count[j][0], final_case_count[j][1]);
		total_cases += final_case_count[j][0] + final_case_count[j][1];
	}
	display_message(INFORMATION_MESSAGE, "Total cases = %d\n", total_cases);
#endif /* defined (TEST_CODE) */
	LEAVE;
}

typedef std::map<int, Iso_mesh*> Iso_mesh_map;
typedef std::map<int, Iso_mesh*>::iterator Iso_mesh_map_iterator;
typedef std::map<int, Iso_mesh*>::const_iterator Iso_mesh_map_const_iterator;

class Isosurface_builder
{
private:
	FE_element *element;
	cmzn_fieldcache_id field_cache;
	cmzn_mesh_id mesh;
	int number_in_xi1, number_in_xi2, number_in_xi3, number_of_polygon_vertices;
	const Iso_surface_specification& specification;
	FE_value delta_xi1, delta_xi2, delta_xi3;
	const int number_of_iso_values;
	int current_iso_value_number;
	double current_iso_value;
	Computed_field *coordinate_field, *data_field, *scalar_field,
		*texture_coordinate_field;
	const int number_of_data_components;
	int plane_size;
	double *plane_scalars;
	bool cube, polygon12, polygon13, polygon23, simplex12, simplex13, simplex23,
		tetrahedron;
	Iso_mesh_map mesh_map;
	int last_mesh_number;
	Iso_mesh *last_mesh;

public:
	Isosurface_builder(FE_element *element, cmzn_fieldcache_id field_cache, cmzn_mesh_id mesh,
		int requested_number_in_xi1, int requested_number_in_xi2, int requested_number_in_xi3,
		const Iso_surface_specification& specification);

	~Isosurface_builder();

	int sweep();

	int fill_graphics(struct Graphics_vertex_array *array);

	void add_vertex_array_entries(struct Graphics_vertex_array *array,
		enum Graphics_vertex_array_attribute_type type, int number_of_components,
		const FE_value *values1, const FE_value *values2, const FE_value *values3);

	void replace_vertex_array_entries(struct Graphics_vertex_array *array,
		enum Graphics_vertex_array_attribute_type type, int number_of_components,
		const FE_value *values1, const FE_value *values2, const FE_value *values3,
		unsigned int insert_vertex_location);

private:

	Iso_mesh& get_mesh()
	{
		if (current_iso_value_number != last_mesh_number)
		{
			last_mesh_number = current_iso_value_number;
			Iso_mesh_map_iterator iter = mesh_map.find(current_iso_value_number);
			if (iter != mesh_map.end())
			{
				last_mesh = iter->second;
			}
			else
			{
				last_mesh = new Iso_mesh();
				mesh_map[last_mesh_number] = last_mesh;
			}
		}
		return *last_mesh;
	}

	const Iso_vertex *compute_line_crossing(const Point_index_pair& pp);

	const Iso_vertex *get_line_crossing(const Point_index_pair& pp)
	{
		const Iso_vertex *vertex = get_mesh().get_vertex(pp);
		if (NULL == vertex)
		{
			vertex = compute_line_crossing(pp);
		}
		return (vertex);
	}

	bool reverse_winding();

	double get_scalar(const Point_index& p) const
	{
		if (p.exact_xi)
		{
			return p.scalar;
		}
		return plane_scalars[(p.k & 1)*plane_size + p.j*(number_in_xi1 + 1) + p.i];
	}

	void set_scalar(int i, int j, int k, double scalar_value)
	{
		plane_scalars[(k & 1)*plane_size + j*(number_in_xi1 + 1) + i] = scalar_value;
	}

	/***************************************************************************//**
	 * Evaluate the scalar field at the centre of the quad and return true if it
	 * exceeds current iso-value.
	 * @param p1, p2, p3, p4  Point indexes of ambiguous quad corners.
	 * @return  true if scalar at quad centre is greater than current iso-value.
	 */
	bool ambiguous_quad_resolves_over(const Point_index& p1, const Point_index& p2,
		const Point_index& p3, const Point_index& p4)
	{
		FE_value scalar_FE_value, xi[3], xi1[3], xi2[3], xi3[3], xi4[3];
		get_xi(p1, xi1);
		get_xi(p2, xi2);
		get_xi(p3, xi3);
		get_xi(p4, xi4);
		xi[0] = 0.25*(xi1[0] + xi2[0] + xi3[0] + xi4[0]);
		xi[1] = 0.25*(xi1[1] + xi2[1] + xi3[1] + xi4[1]);
		xi[2] = 0.25*(xi1[2] + xi2[2] + xi3[2] + xi4[2]);
		field_cache->setMeshLocation(element, xi);
		cmzn_field_evaluate_real(scalar_field, field_cache, 1, &scalar_FE_value);
		return ((double)scalar_FE_value > current_iso_value);
	}

	void get_xi(const Point_index& p, FE_value *xi) const
	{
		if (p.exact_xi)
		{
			xi[0] = p.xi1;
			xi[1] = p.xi2;
			xi[2] = p.xi3;
		}
		else
		{
			xi[0] = p.i * delta_xi1;
			xi[1] = p.j * delta_xi2;
			xi[2] = p.k * delta_xi3;
		}
	}

	void get_cell_centre_xi(int i, int j, int k, FE_value *xi) const
	{
		xi[0] = (i + 0.5) * delta_xi1;
		xi[1] = (j + 0.5) * delta_xi2;
		xi[2] = (k + 0.5) * delta_xi3;
	}

	void cross_tetrahedron(const Point_index& p1,
		const Point_index& p2, const Point_index& p3, const Point_index& p4);

	int cross_cube(int i, int j, int k);

	int cross_octahedron(int i, int j, int k);

	int cross_pyramid(
		const Point_index& p0, const Point_index& p1, const Point_index& p2,
		const Point_index& p3, const Point_index& p4);

	int cross_triangle_wedge(
		const Point_index& p0, const Point_index& p1, const Point_index& p2,
		const Point_index& p3, const Point_index& p4, const Point_index& p5);

};

Isosurface_builder::Isosurface_builder(FE_element *element, cmzn_fieldcache_id field_cache,
	cmzn_mesh_id mesh, int requested_number_in_xi1, int requested_number_in_xi2, int requested_number_in_xi3,
	const Iso_surface_specification& specification) :
		element(element),
		field_cache(field_cache),
		mesh(mesh),
		number_in_xi1(requested_number_in_xi1),
		number_in_xi2(requested_number_in_xi2),
		number_in_xi3(requested_number_in_xi3),
		number_of_polygon_vertices(1),
		specification(specification),
		number_of_iso_values(specification.number_of_iso_values),
		coordinate_field(specification.coordinate_field),
		data_field(specification.data_field),
		scalar_field(specification.scalar_field),
		texture_coordinate_field(specification.texture_coordinate_field),
		number_of_data_components(specification.number_of_data_components),
		last_mesh_number(-1)
{
	enum FE_element_shape_type shape_type1, shape_type2, shape_type3;
	FE_element_shape *element_shape = get_FE_element_shape(element);
	get_FE_element_shape_xi_shape_type(element_shape, /*xi_number*/0, &shape_type1);
	get_FE_element_shape_xi_shape_type(element_shape, /*xi_number*/1, &shape_type2);
	get_FE_element_shape_xi_shape_type(element_shape, /*xi_number*/2, &shape_type3);

	simplex12 = (SIMPLEX_SHAPE == shape_type1) && (SIMPLEX_SHAPE == shape_type2);
	simplex13 = (SIMPLEX_SHAPE == shape_type3) && (SIMPLEX_SHAPE == shape_type1);
	simplex23 = (SIMPLEX_SHAPE == shape_type2) && (SIMPLEX_SHAPE == shape_type3);
	tetrahedron = simplex12 && simplex23;
	polygon12 = (POLYGON_SHAPE == shape_type1) && (POLYGON_SHAPE == shape_type2);
	polygon13 = (POLYGON_SHAPE == shape_type1) && (POLYGON_SHAPE == shape_type3);
	polygon23 = (POLYGON_SHAPE == shape_type2) && (POLYGON_SHAPE == shape_type3);
	cube = (!simplex12) && (!simplex13) && (!simplex23);

	if (simplex12)
	{
		number_in_xi2 = number_in_xi1;
	}
	if (simplex13)
	{
		number_in_xi3 = number_in_xi1;
	}
	if (simplex23)
	{
		number_in_xi3 = number_in_xi2;
	}

	if (polygon12 || polygon13 || polygon23)
	{
		int linked_xi1 = (polygon12 || polygon13) ? 0 : 1;
		int linked_xi2 = polygon12 ? 1 : 2;
		get_FE_element_shape_xi_linkage_number(element_shape,
			linked_xi1, linked_xi2, &number_of_polygon_vertices);
		if (0 == linked_xi1)
		{
			number_in_xi1 = (1 + (number_in_xi1 - 1)/number_of_polygon_vertices)
				* number_of_polygon_vertices;
			// better: number_in_xi1 *= number_of_polygon_vertices;
		}
		else
		{
			number_in_xi2 = (1 + (number_in_xi2 - 1)/number_of_polygon_vertices)
				* number_of_polygon_vertices;
			// better: number_in_xi2 *= number_of_polygon_vertices;
		}
	}

	// precalculate scalings from index to xi
	delta_xi1 = 1.0 / number_in_xi1;
	delta_xi2 = 1.0 / number_in_xi2;
	delta_xi3 = 1.0 / number_in_xi3;
	plane_size = (number_in_xi1 + 1)*(number_in_xi2 + 1);
	plane_scalars = new double[2*plane_size];
}

Isosurface_builder::~Isosurface_builder()
{
	ENTER(Isosurface_builder::~Isosurface_builder);
	for (Iso_mesh_map_iterator mesh_iter = mesh_map.begin(); mesh_iter != mesh_map.end(); mesh_iter++)
	{
		Iso_mesh *mesh = mesh_iter->second;
		delete mesh;
	}
	delete[] plane_scalars;
	LEAVE;
}

const Iso_vertex *Isosurface_builder::compute_line_crossing(
	const Point_index_pair& pp)
{
	ENTER(Isosurface_builder::compute_line_crossing);
	Iso_vertex *vertex = new Iso_vertex();
	double scalar_a = get_scalar(pp.pa);
	double scalar_b = get_scalar(pp.pb);
	double r = (current_iso_value - scalar_a) / (scalar_b - scalar_a);
	double inverse_r = 1.0 - r;
	FE_value xi_a[3], xi_b[3];
	get_xi(pp.pa, xi_a);
	get_xi(pp.pb, xi_b);
	vertex->xi[0] = xi_a[0]*inverse_r + xi_b[0]*r;
	vertex->xi[1] = xi_a[1]*inverse_r + xi_b[1]*r;
	vertex->xi[2] = xi_a[2]*inverse_r + xi_b[2]*r;
	// future: option to iterate to get exact xi crossing for non-linear fields
	this->field_cache->setMeshLocation(element, vertex->xi);
	cmzn_field_evaluate_real(coordinate_field, field_cache, 3, vertex->coordinates);
	if (NULL != texture_coordinate_field)
	{
		cmzn_field_evaluate_real(texture_coordinate_field, field_cache, 3, vertex->texture_coordinates);
	}
	if (NULL != data_field)
	{
		FE_value *data = new FE_value[number_of_data_components];
		cmzn_field_evaluate_real(data_field, field_cache, number_of_data_components, data);
		vertex->set_data(data);
	}
	const Iso_vertex* new_vertex = get_mesh().add_vertex(pp, vertex);
	LEAVE;

	return (new_vertex);
}

/***************************************************************************//**
 * @param p0..p3  Point index locations of tetrahedron vertices with winding:
 *
 *       3---2
 *      / \.'|
 *     / .'\ |
 *    /.'   \|
 *   0-------1
 */
void Isosurface_builder::cross_tetrahedron(const Point_index& p0,
	const Point_index& p1, const Point_index& p2, const Point_index& p3)
{
	// first column is final case in standard orientation
	// columns 1-4 are indexes of initial vertices in order to rotate to final case
	// column 5 is 1 for inverse case: reverse polygon winding
	static const unsigned char tet_case[16][6] =
	{
		{ 0, 0, 0, 0, 0, 0 }, //  0
		{ 1, 0, 1, 2, 3, 0 }, //  1 : 0 over = standard case
		{ 1, 1, 0, 3, 2, 0 }, //  2
		{ 3, 0, 1, 2, 3, 0 }, //  3 : 0,1 over = standard case
		{ 1, 2, 0, 1, 3, 0 }, //  4
		{ 3, 0, 2, 3, 1, 0 }, //  5
		{ 3, 1, 2, 0, 3, 0 }, //  6
		{ 1, 3, 0, 2, 1, 1 }, //  7
		{ 1, 3, 0, 2, 1, 0 }, //  8
		{ 3, 0, 3, 1, 2, 0 }, //  9
		{ 3, 1, 3, 2, 0, 0 }, // 10
		{ 1, 2, 0, 1, 3, 1 }, // 11
		{ 3, 2, 3, 0, 1, 0 }, // 12
		{ 1, 1, 0, 3, 2, 1 }, // 13
		{ 1, 0, 1, 2, 3, 1 }, // 14 : 1,2,3 over = case 1 inverse
		{ 0, 0, 0, 0, 0, 0 }  // 15
	};

	ENTER(Isosurface_builder::cross_tetrahedron);
	int unrotated_case =
		((get_scalar(p0) > current_iso_value) ? 1 : 0) +
		((get_scalar(p1) > current_iso_value) ? 2 : 0) +
		((get_scalar(p2) > current_iso_value) ? 4 : 0) +
		((get_scalar(p3) > current_iso_value) ? 8 : 0);
	int final_case = tet_case[unrotated_case][0];
	if (0 < final_case)
	{
		Point_index mp[4] = { p0, p1, p2, p3 };
		Point_index mp0 = mp[tet_case[unrotated_case][1]];
		Point_index mp1 = mp[tet_case[unrotated_case][2]];
		Point_index mp2 = mp[tet_case[unrotated_case][3]];
		Point_index mp3 = mp[tet_case[unrotated_case][4]];
		const Iso_vertex *v1, *v2, *v3, *v4;
		Iso_mesh& mesh = get_mesh();
		switch (final_case)
		{
		case 1: // 0 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp1));
			v2 = get_line_crossing(Point_index_pair(mp0, mp3));
			v3 = get_line_crossing(Point_index_pair(mp0, mp2));
			mesh.add_triangle(v1, v2, v3, /*inverse*/(0 != tet_case[unrotated_case][5]));
			break;
		case 3: // 0, 1 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp0, mp3));
			v2 = get_line_crossing(Point_index_pair(mp0, mp2));
			v3 = get_line_crossing(Point_index_pair(mp1, mp2));
			v4 = get_line_crossing(Point_index_pair(mp1, mp3));
			mesh.add_quadrilateral(v1, v2, v3, v4);
			break;
		default:
			display_message(ERROR_MESSAGE,
				"Isosurface_builder::cross_tetrahedron.  Unknown case %d (unrotated %d)", final_case, unrotated_case);
			break;
		}
	}
	LEAVE;
}

static Marching_cube mcube;

/***************************************************************************//**
 * @param i, j, k  Index location of unit cube, vertices with following winding:
 *
 *     6-------7
 *    /:      /|
 *   4-------5 |
 *   | :     | |
 *   | 2- - -|-3
 *   |:      |/
 *   0-------1
 */
inline int Isosurface_builder::cross_cube(int i, int j, int k)
{
	ENTER(Isosurface_builder::cross_cube);
	int return_code = 1;
	Point_index p[8];
	p[0].set(i    , j    , k    );
	p[1].set(i + 1, j    , k    );
	p[2].set(i    , j + 1, k    );
	p[3].set(i + 1, j + 1, k    );
	p[4].set(i    , j    , k + 1);
	p[5].set(i + 1, j    , k + 1);
	p[6].set(i    , j + 1, k + 1);
	p[7].set(i + 1, j + 1, k + 1);
	int unrotated_case =
		((get_scalar(p[0]) > current_iso_value) ?   1 : 0) +
		((get_scalar(p[1]) > current_iso_value) ?   2 : 0) +
		((get_scalar(p[2]) > current_iso_value) ?   4 : 0) +
		((get_scalar(p[3]) > current_iso_value) ?   8 : 0) +
		((get_scalar(p[4]) > current_iso_value) ?  16 : 0) +
		((get_scalar(p[5]) > current_iso_value) ?  32 : 0) +
		((get_scalar(p[6]) > current_iso_value) ?  64 : 0) +
		((get_scalar(p[7]) > current_iso_value) ? 128 : 0);
	int final_case = mcube.final_case(unrotated_case);
	if (0 < final_case)
	{
		const bool inverse = mcube.inverse(unrotated_case);
		Point_index mp0 = p[mcube.rotated_vertex(unrotated_case, 0)];
		Point_index mp1 = p[mcube.rotated_vertex(unrotated_case, 1)];
		Point_index mp2 = p[mcube.rotated_vertex(unrotated_case, 2)];
		Point_index mp3 = p[mcube.rotated_vertex(unrotated_case, 3)];
		Point_index mp4 = p[mcube.rotated_vertex(unrotated_case, 4)];
		Point_index mp5 = p[mcube.rotated_vertex(unrotated_case, 5)];
		Point_index mp6 = p[mcube.rotated_vertex(unrotated_case, 6)];
		Point_index mp7 = p[mcube.rotated_vertex(unrotated_case, 7)];
		const Iso_vertex *v1, *v2, *v3, *v4, *v5, *v6, *v7, *v8;
		Iso_mesh& mesh = get_mesh();
		switch (final_case)
		{
		case 1: // 0 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp1));
			v2 = get_line_crossing(Point_index_pair(mp0, mp4));
			v3 = get_line_crossing(Point_index_pair(mp0, mp2));
			mesh.add_triangle(v1, v2, v3, inverse);
			break;
		case 3: // 0,1 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp4));
			v2 = get_line_crossing(Point_index_pair(mp0, mp2));
			v3 = get_line_crossing(Point_index_pair(mp1, mp3));
			v4 = get_line_crossing(Point_index_pair(mp1, mp5));
			mesh.add_quadrilateral(v1, v2, v3, v4, inverse);
			break;
		case 7: // 0,1,2 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp4));
			v2 = get_line_crossing(Point_index_pair(mp2, mp6));
			v3 = get_line_crossing(Point_index_pair(mp2, mp3));
			v4 = get_line_crossing(Point_index_pair(mp1, mp3));
			v5 = get_line_crossing(Point_index_pair(mp1, mp5));
			mesh.add_pentagon(v1, v2, v3, v4, v5, inverse);
			break;
		case 9: // 0,3 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp1));
			v2 = get_line_crossing(Point_index_pair(mp0, mp4));
			v3 = get_line_crossing(Point_index_pair(mp0, mp2));
			v4 = get_line_crossing(Point_index_pair(mp3, mp1));
			v5 = get_line_crossing(Point_index_pair(mp3, mp2));
			v6 = get_line_crossing(Point_index_pair(mp3, mp7));
			if (ambiguous_quad_resolves_over(mp0, mp1, mp2, mp3) != inverse)
			{
				mesh.add_quadrilateral(v1, v2, v6, v4, inverse);
				mesh.add_quadrilateral(v2, v3, v5, v6, inverse);
			}
			else
			{
				mesh.add_triangle(v1, v2, v3, inverse);
				mesh.add_triangle(v4, v5, v6, inverse);
			}
			break;
		case 15: // 0,1,2,3 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp0, mp4));
			v2 = get_line_crossing(Point_index_pair(mp2, mp6));
			v3 = get_line_crossing(Point_index_pair(mp3, mp7));
			v4 = get_line_crossing(Point_index_pair(mp1, mp5));
			mesh.add_quadrilateral(v1, v2, v3, v4);
			break;
		case 23: // 0,1,2,4 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp1, mp3));
			v2 = get_line_crossing(Point_index_pair(mp1, mp5));
			v3 = get_line_crossing(Point_index_pair(mp4, mp5));
			v4 = get_line_crossing(Point_index_pair(mp4, mp6));
			v5 = get_line_crossing(Point_index_pair(mp2, mp6));
			v6 = get_line_crossing(Point_index_pair(mp2, mp3));
			mesh.add_hexagon(v1, v2, v3, v4, v5, v6);
			break;
		case 39: // 0,1,2,5 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp0, mp4));
			v2 = get_line_crossing(Point_index_pair(mp2, mp6));
			v3 = get_line_crossing(Point_index_pair(mp2, mp3));
			v4 = get_line_crossing(Point_index_pair(mp1, mp3));
			v5 = get_line_crossing(Point_index_pair(mp5, mp7));
			v6 = get_line_crossing(Point_index_pair(mp5, mp4));
			mesh.add_hexagon(v1, v2, v3, v4, v5, v6);
			break;
		case 67: // 0,1,6 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp4));
			v2 = get_line_crossing(Point_index_pair(mp0, mp2));
			v3 = get_line_crossing(Point_index_pair(mp1, mp3));
			v4 = get_line_crossing(Point_index_pair(mp1, mp5));
			v5 = get_line_crossing(Point_index_pair(mp6, mp2));
			v6 = get_line_crossing(Point_index_pair(mp6, mp4));
			v7 = get_line_crossing(Point_index_pair(mp6, mp7));
			if (ambiguous_quad_resolves_over(mp0, mp2, mp4, mp6) != inverse)
			{
				// extra interior point:
				v8 = get_line_crossing(Point_index_pair(mp0, mp7));
				mesh.add_triangle(v8, v4, v1, inverse);
				mesh.add_triangle(v8, v3, v4, inverse);
				mesh.add_triangle(v8, v2, v3, inverse);
				mesh.add_quadrilateral(v8, v1, v6, v7, inverse);
				mesh.add_quadrilateral(v8, v7, v5, v2, inverse);
			}
			else
			{
				mesh.add_quadrilateral(v1, v2, v3, v4, inverse);
				mesh.add_triangle(v5, v6, v7, inverse);
			}
			break;
		case 71: // 0,1,2,6 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp0, mp4));
			v2 = get_line_crossing(Point_index_pair(mp6, mp4));
			v3 = get_line_crossing(Point_index_pair(mp6, mp7));
			v4 = get_line_crossing(Point_index_pair(mp2, mp3));
			v5 = get_line_crossing(Point_index_pair(mp1, mp3));
			v6 = get_line_crossing(Point_index_pair(mp1, mp5));
			mesh.add_hexagon(v1, v2, v3, v4, v5, v6);
			break;
		case 129: // 0,7 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp1));
			v2 = get_line_crossing(Point_index_pair(mp0, mp4));
			v3 = get_line_crossing(Point_index_pair(mp0, mp2));
			v4 = get_line_crossing(Point_index_pair(mp7, mp3));
			v5 = get_line_crossing(Point_index_pair(mp7, mp6));
			v6 = get_line_crossing(Point_index_pair(mp7, mp5));
			mesh.add_triangle(v1, v2, v3, inverse);
			mesh.add_triangle(v4, v5, v6, inverse);
			break;
		case 41: // 0,3,5 > iso_value
		case 105: // 0,3,5,6 > iso_value
		case 135: // 0,1,2,7 > iso_value
		case 195: // 0,1,6,7 > iso_value
		{
			// Cases with multiple ambiguous double contour faces:
			// divide into 6 pyramids with new centre point sample
			FE_value xi_c[3], scalar_FE_value;
			get_cell_centre_xi(i, j, k, xi_c);
			this->field_cache->setMeshLocation(element, xi_c);
			cmzn_field_evaluate_real(scalar_field, field_cache, 1, &scalar_FE_value);
			Point_index pc(xi_c,  static_cast<double>(scalar_FE_value));
			cross_pyramid(p[0], p[2], p[4], p[6], pc);
			cross_pyramid(p[3], p[1], p[7], p[5], pc);
			cross_pyramid(p[1], p[0], p[5], p[4], pc);
			cross_pyramid(p[2], p[3], p[6], p[7], pc);
			cross_pyramid(p[0], p[1], p[2], p[3], pc);
			cross_pyramid(p[5], p[4], p[7], p[6], pc);
			break;
		}
		default:
			display_message(ERROR_MESSAGE,
				"Isosurface_builder::cross_cube.  Unknown case %d (unrotated %d)", final_case, unrotated_case);
			return_code = 0;
			break;
		}
	}
	LEAVE;

	return (return_code);
}

/***************************************************************************//**
 * @param i, j, k  Index location origin A of unit cube containing octahedron,
 * vertices 0..5 used with following numbering and winding:
 *
 *     5-------B
 *    /:      /|
 *   3-------4 |
 *   | :     | |
 *   | 1- - -|-2
 *   |:      |/
 *   A-------0
 *
 * Octahedron is cube with A and B corner tetrahedra removed.
 */
int Isosurface_builder::cross_octahedron(int i, int j, int k)
{
	// first column is final case in standard orientation
	// columns 1-6 are indexes of initial vertices in order to rotate to final case
	// column 7 is 1 for inverse case: reverse polygon winding
	static const unsigned char oct_case[64][8] =
	{
		{  0, 0, 0, 0, 0, 0, 0, 0 }, //  0
		{  1, 0, 1, 2, 3, 4, 5, 0 }, //  1 : 0 over = standard case
		{  1, 1, 2, 0, 5, 3, 4, 0 }, //  2
		{  3, 0, 1, 2, 3, 4, 5, 0 }, //  3 : 0,1 over (adj) = standard case
		{  1, 2, 0, 1, 4, 5, 3, 0 }, //  4
		{  3, 0, 2, 4, 1, 3, 5, 0 }, //  5
		{  3, 1, 2, 0, 5, 3, 4, 0 }, //  6
		{  7, 0, 1, 2, 3, 4, 5, 0 }, //  7 : 0,1,2 over (adj) = standard case
		{  1, 3, 1, 0, 5, 4, 2, 0 }, //  8
		{  3, 0, 3, 1, 4, 2, 5, 0 }, //  9
		{  3, 1, 3, 5, 0, 2, 4, 0 }, // 10
		{  7, 0, 3, 1, 4, 2, 5, 0 }, // 11
		{ 33, 2, 0, 1, 4, 5, 3, 0 }, // 12
		{ 35, 2, 0, 1, 4, 5, 3, 0 }, // 13
		{ 35, 2, 1, 5, 0, 4, 3, 0 }, // 14
		{  3, 4, 5, 3, 2, 0, 1, 1 }, // 15
		{  1, 4, 0, 2, 3, 5, 1, 0 }, // 16
		{  3, 0, 4, 3, 2, 1, 5, 0 }, // 17
		{ 33, 1, 2, 0, 5, 3, 4, 0 }, // 18
		{ 35, 1, 0, 3, 2, 5, 4, 0 }, // 19
		{  3, 2, 4, 0, 5, 1, 3, 0 }, // 20
		{  7, 0, 2, 4, 1, 3, 5, 0 }, // 21
		{ 35, 1, 2, 0, 5, 3, 4, 0 }, // 22
		{  3, 3, 5, 1, 4, 0, 2, 1 }, // 23
		{  3, 3, 4, 5, 0, 1, 2, 0 }, // 24
		{  7, 0, 4, 3, 2, 1, 5, 0 }, // 25
		{ 35, 1, 3, 5, 0, 2, 4, 0 }, // 26
		{  3, 2, 5, 4, 1, 0, 3, 1 }, // 27
		{ 35, 2, 4, 0, 5, 1, 3, 0 }, // 28
		{  3, 1, 5, 2, 3, 0, 4, 1 }, // 29
		{ 33, 0, 1, 2, 3, 4, 5, 1 }, // 30 : 1,2,3,4 over = case 33 inverse
		{  1, 5, 2, 1, 4, 3, 0, 1 }, // 31
		{  1, 5, 2, 1, 4, 3, 0, 0 }, // 32
		{ 33, 0, 1, 2, 3, 4, 5, 0 }, // 33 : 0,5 over (opp) = standard case
		{  3, 1, 5, 2, 3, 0, 4, 0 }, // 34
		{ 35, 0, 1, 2, 3, 4, 5, 0 }, // 35 : 0,1,5 over (opp) = standard case
		{  3, 2, 5, 4, 1, 0, 3, 0 }, // 36
		{ 35, 0, 2, 4, 1, 3, 5, 0 }, // 37
		{  7, 1, 5, 2, 3, 0, 4, 0 }, // 38
		{  3, 3, 4, 5, 0, 1, 2, 1 }, // 39
		{  3, 3, 5, 1, 4, 0, 2, 0 }, // 40
		{ 35, 0, 3, 1, 4, 2, 5, 0 }, // 41
		{  7, 1, 3, 5, 0, 2, 4, 0 }, // 42
		{  3, 2, 4, 0, 5, 1, 3, 1 }, // 43
		{ 35, 2, 5, 4, 1, 0, 3, 0 }, // 44
		{ 33, 1, 2, 0, 5, 3, 4, 1 }, // 45
		{  3, 0, 4, 3, 2, 1, 5, 1 }, // 46
		{  1, 4, 0, 2, 3, 5, 1, 1 }, // 47
		{  3, 4, 5, 3, 2, 0, 1, 0 }, // 48
		{ 35, 0, 4, 3, 2, 1, 5, 0 }, // 49
		{ 35, 1, 5, 2, 3, 0, 4, 0 }, // 50
		{ 33, 2, 0, 1, 4, 5, 3, 1 }, // 51
		{  7, 2, 5, 4, 1, 0, 3, 0 }, // 52
		{  3, 1, 3, 5, 0, 2, 4, 1 }, // 53
		{  3, 0, 3, 1, 4, 2, 5, 1 }, // 54
		{  1, 3, 1, 0, 5, 4, 2, 1 }, // 55
		{  7, 3, 4, 5, 0, 1, 2, 0 }, // 56
		{  3, 1, 2, 0, 5, 3, 4, 1 }, // 57
		{  3, 0, 2, 4, 1, 3, 5, 1 }, // 58
		{  1, 2, 0, 1, 4, 5, 3, 1 }, // 59
		{  3, 0, 1, 2, 3, 4, 5, 1 }, // 60 : 2,3,4,5 over = case 3 inverse
		{  1, 1, 2, 0, 5, 3, 4, 1 }, // 61
		{  1, 0, 1, 2, 3, 4, 5, 1 }, // 62 : 1,2,3,4,5 over = case 1 inverse
		{  0, 0, 0, 0, 0, 0, 0, 0 }  // 63
	};

	ENTER(Isosurface_builder::cross_octahedron);
	int return_code = 1;
	Point_index p[6];
	p[0].set(i + 1, j    , k    );
	p[1].set(i    , j + 1, k    );
	p[2].set(i + 1, j + 1, k    );
	p[3].set(i    , j    , k + 1);
	p[4].set(i + 1, j    , k + 1);
	p[5].set(i    , j + 1, k + 1);
	int unrotated_case =
		((get_scalar(p[0]) > current_iso_value) ?  1 : 0) +
		((get_scalar(p[1]) > current_iso_value) ?  2 : 0) +
		((get_scalar(p[2]) > current_iso_value) ?  4 : 0) +
		((get_scalar(p[3]) > current_iso_value) ?  8 : 0) +
		((get_scalar(p[4]) > current_iso_value) ? 16 : 0) +
		((get_scalar(p[5]) > current_iso_value) ? 32 : 0);
	int final_case = oct_case[unrotated_case][0];
	if (0 < final_case)
	{
		const bool inverse = (0 != oct_case[unrotated_case][7]);
		Point_index mp0 = p[oct_case[unrotated_case][1]];
		Point_index mp1 = p[oct_case[unrotated_case][2]];
		Point_index mp2 = p[oct_case[unrotated_case][3]];
		Point_index mp3 = p[oct_case[unrotated_case][4]];
		Point_index mp4 = p[oct_case[unrotated_case][5]];
		Point_index mp5 = p[oct_case[unrotated_case][6]];
		const Iso_vertex *v1, *v2, *v3, *v4, *v5, *v6;
		Iso_mesh& mesh = get_mesh();
		switch (final_case)
		{
		case 1: // 0 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp1));
			v2 = get_line_crossing(Point_index_pair(mp0, mp2));
			v3 = get_line_crossing(Point_index_pair(mp0, mp4));
			v4 = get_line_crossing(Point_index_pair(mp0, mp3));
			mesh.add_quadrilateral(v1, v2, v3, v4, inverse);
			break;
		case 3: // 0,1 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp2));
			v2 = get_line_crossing(Point_index_pair(mp0, mp4));
			v3 = get_line_crossing(Point_index_pair(mp0, mp3));
			v4 = get_line_crossing(Point_index_pair(mp1, mp3));
			v5 = get_line_crossing(Point_index_pair(mp1, mp5));
			v6 = get_line_crossing(Point_index_pair(mp1, mp2));
			mesh.add_quadrilateral(v1, v2, v5, v6, inverse);
			mesh.add_quadrilateral(v2, v3, v4, v5, inverse);
			break;
		case 7: // 0,1,2 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp0, mp4));
			v2 = get_line_crossing(Point_index_pair(mp0, mp3));
			v3 = get_line_crossing(Point_index_pair(mp1, mp3));
			v4 = get_line_crossing(Point_index_pair(mp1, mp5));
			v5 = get_line_crossing(Point_index_pair(mp2, mp5));
			v6 = get_line_crossing(Point_index_pair(mp2, mp4));
			mesh.add_hexagon(v1, v2, v3, v4, v5, v6);
			break;
		case 33: // 0,5 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp1));
			v2 = get_line_crossing(Point_index_pair(mp0, mp2));
			v3 = get_line_crossing(Point_index_pair(mp0, mp4));
			v4 = get_line_crossing(Point_index_pair(mp0, mp3));
			mesh.add_quadrilateral(v1, v2, v3, v4, inverse);
			v1 = get_line_crossing(Point_index_pair(mp1, mp5));
			v2 = get_line_crossing(Point_index_pair(mp3, mp5));
			v3 = get_line_crossing(Point_index_pair(mp4, mp5));
			v4 = get_line_crossing(Point_index_pair(mp2, mp5));
			mesh.add_quadrilateral(v1, v2, v3, v4, inverse);
			break;
		case 35:
			// 0,1,5 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp0, mp2));
			v2 = get_line_crossing(Point_index_pair(mp0, mp4));
			v3 = get_line_crossing(Point_index_pair(mp4, mp5));
			v4 = get_line_crossing(Point_index_pair(mp2, mp5));
			v5 = get_line_crossing(Point_index_pair(mp1, mp2));
			mesh.add_pentagon(v1, v2, v3, v4, v5);
			v1 = v3;
			//v2 = v2;
			v3 = get_line_crossing(Point_index_pair(mp0, mp3));
			v4 = get_line_crossing(Point_index_pair(mp1, mp3));
			v5 = get_line_crossing(Point_index_pair(mp3, mp5));
			mesh.add_pentagon(v1, v2, v3, v4, v5);
			break;
		default:
			display_message(ERROR_MESSAGE,
				"Isosurface_builder::cross_octahedron.  Unknown case %d (unrotated %d)", final_case, unrotated_case);
			return_code = 0;
			break;
		}
	}
	LEAVE;

	return (return_code);
}

/***************************************************************************//**
 * @param p0..p4  Point index locations of pyramid vertices with winding:
 *
 *        4\
 *       /.\ \
 *      /.  \  \
 *     /2 . .\. 3
 *    /.      \/
 *   0--------1
 */
int Isosurface_builder::cross_pyramid(
	const Point_index& p0, const Point_index& p1, const Point_index& p2,
	const Point_index& p3, const Point_index& p4)
{
	// first column is final case in standard orientation
	// columns 1-5 are indexes of initial vertices in order to rotate to final case
	// column 6 is 1 for inverse case: reverse polygon winding
	static const unsigned char pyr_case[32][7] =
	{
		{  0, 0, 0, 0, 0, 0, 0 }, //  0
		{  1, 0, 1, 2, 3, 4, 0 }, //  1 : 0 over = standard case
		{  1, 1, 3, 0, 2, 4, 0 }, //  2
		{  3, 0, 1, 2, 3, 4, 0 }, //  3 : 0,1 over = standard case
		{  1, 2, 0, 3, 1, 4, 0 }, //  4
		{  3, 2, 0, 3, 1, 4, 0 }, //  5
		{  9, 1, 3, 0, 2, 4, 0 }, //  6
		{  7, 0, 1, 2, 3, 4, 0 }, //  7 : 0,1,2 over = standard case
		{  1, 3, 2, 1, 0, 4, 0 }, //  8
		{  9, 0, 1, 2, 3, 4, 0 }, //  9 : 0,3 over = standard case
		{  3, 1, 3, 0, 2, 4, 0 }, // 10
		{  7, 1, 3, 0, 2, 4, 0 }, // 11
		{  3, 3, 2, 1, 0, 4, 0 }, // 12
		{  7, 2, 0, 3, 1, 4, 0 }, // 13
		{  7, 3, 2, 1, 0, 4, 0 }, // 14
		{ 15, 0, 1, 2, 3, 4, 0 }, // 15 : 0,1,2,3 over = standard case
		{ 15, 0, 1, 2, 3, 4, 1 }, // 16 : 4 over = case 15 inverse
		{  7, 3, 2, 1, 0, 4, 1 }, // 17
		{  7, 2, 0, 3, 1, 4, 1 }, // 18
		{  3, 3, 2, 1, 0, 4, 1 }, // 19
		{  7, 1, 3, 0, 2, 4, 1 }, // 20
		{  3, 1, 3, 0, 2, 4, 1 }, // 21
		{  9, 0, 1, 2, 3, 4, 1 }, // 22 : 1,2,4 over = case 9 inverse
		{  1, 3, 2, 1, 0, 4, 1 }, // 23
		{  7, 0, 1, 2, 3, 4, 1 }, // 24 : 3,4 over = case 7 inverse
		{  9, 1, 3, 0, 2, 4, 1 }, // 25
		{  3, 2, 0, 3, 1, 4, 1 }, // 26
		{  1, 2, 0, 3, 1, 4, 1 }, // 27
		{  3, 0, 1, 2, 3, 4, 1 }, // 28 : 2,3,4 over = case 3 inverse
		{  1, 1, 3, 0, 2, 4, 1 }, // 29
		{  1, 0, 1, 2, 3, 4, 1 }, // 30 : 1,2,3,4 over = case 1 inverse
		{  0, 0, 0, 0, 0, 0, 0 }  // 31
	};

	ENTER(Isosurface_builder::cross_pyramid);
	int return_code = 1;
	int unrotated_case =
		((get_scalar(p0) > current_iso_value) ?  1 : 0) +
		((get_scalar(p1) > current_iso_value) ?  2 : 0) +
		((get_scalar(p2) > current_iso_value) ?  4 : 0) +
		((get_scalar(p3) > current_iso_value) ?  8 : 0) +
		((get_scalar(p4) > current_iso_value) ? 16 : 0);
	int final_case = pyr_case[unrotated_case][0];
	if (0 < final_case)
	{
		const bool inverse = (0 != pyr_case[unrotated_case][6]);
		Point_index mp[5] = { p0, p1, p2, p3, p4 };
		Point_index mp0 = mp[pyr_case[unrotated_case][1]];
		Point_index mp1 = mp[pyr_case[unrotated_case][2]];
		Point_index mp2 = mp[pyr_case[unrotated_case][3]];
		Point_index mp3 = mp[pyr_case[unrotated_case][4]];
		Point_index mp4 = mp[pyr_case[unrotated_case][5]];
		const Iso_vertex *v1, *v2, *v3, *v4, *v5, *v6;
		Iso_mesh& mesh = get_mesh();
		switch (final_case)
		{
		case 1: // 0 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp1));
			v2 = get_line_crossing(Point_index_pair(mp0, mp4));
			v3 = get_line_crossing(Point_index_pair(mp0, mp2));
			mesh.add_triangle(v1, v2, v3, inverse);
			break;
		case 3: // 0,1 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp4));
			v2 = get_line_crossing(Point_index_pair(mp0, mp2));
			v3 = get_line_crossing(Point_index_pair(mp1, mp3));
			v4 = get_line_crossing(Point_index_pair(mp1, mp4));
			mesh.add_quadrilateral(v1, v2, v3, v4, inverse);
			break;
		case 7: // 0,1,2 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp4));
			v2 = get_line_crossing(Point_index_pair(mp2, mp4));
			v3 = get_line_crossing(Point_index_pair(mp2, mp3));
			v4 = get_line_crossing(Point_index_pair(mp1, mp3));
			v5 = get_line_crossing(Point_index_pair(mp1, mp4));
			mesh.add_pentagon(v1, v2, v3, v4, v5, inverse);
			break;
		case 9: // 0,3 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp1));
			v2 = get_line_crossing(Point_index_pair(mp0, mp4));
			v3 = get_line_crossing(Point_index_pair(mp0, mp2));
			v4 = get_line_crossing(Point_index_pair(mp3, mp2));
			v5 = get_line_crossing(Point_index_pair(mp3, mp4));
			v6 = get_line_crossing(Point_index_pair(mp3, mp1));
			if (ambiguous_quad_resolves_over(mp0, mp1, mp2, mp3) != inverse)
			{
				mesh.add_quadrilateral(v1, v2, v5, v6, inverse);
				mesh.add_quadrilateral(v2, v3, v4, v5, inverse);
			}
			else
			{
				mesh.add_triangle(v1, v2, v3, inverse);
				mesh.add_triangle(v4, v5, v6, inverse);
			}
			break;
		case 15: // 0,1,2,3 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp4));
			v2 = get_line_crossing(Point_index_pair(mp2, mp4));
			v3 = get_line_crossing(Point_index_pair(mp3, mp4));
			v4 = get_line_crossing(Point_index_pair(mp1, mp4));
			mesh.add_quadrilateral(v1, v2, v3, v4, inverse);
			break;
		default:
			display_message(ERROR_MESSAGE,
				"Isosurface_builder::cross_pyramid.  Unknown case %d (unrotated %d)", final_case, unrotated_case);
			return_code = 0;
			break;
		}
	}
	LEAVE;

	return (return_code);
}

/**
 *      3_____5
 *      |\   /|
 *      | \ / |
 *      |  4  |
 *      0..|..2
 *       \ | /
 *        \|/
 *         1
 */
int Isosurface_builder::cross_triangle_wedge(
	const Point_index& p0, const Point_index& p1, const Point_index& p2,
	const Point_index& p3, const Point_index& p4, const Point_index& p5)
{
	// first column is final case in standard orientation
	// columns 1-6 are indexes of initial vertices in order to rotate to final case
	// column 7 is 1 for inverse case: reverse polygon winding
	static const unsigned char tri_case[64][8] =
	{
		{  0, 0, 0, 0, 0, 0, 0, 0 }, //  0
		{  1, 0, 1, 2, 3, 4, 5, 0 }, //  1 : 0 over = standard case
		{  1, 1, 2, 0, 4, 5, 3, 0 }, //  2
		{  3, 0, 1, 2, 3, 4, 5, 0 }, //  3 : 0,1 over = standard case
		{  1, 2, 0, 1, 5, 3, 4, 0 }, //  4
		{  3, 2, 0, 1, 5, 3, 4, 0 }, //  5
		{  3, 1, 2, 0, 4, 5, 3, 0 }, //  6
		{  7, 0, 1, 2, 3, 4, 5, 0 }, //  7 : 0,1,2 over = standard case
		{  1, 3, 5, 4, 0, 2, 1, 0 }, //  8
		{  9, 0, 1, 2, 3, 4, 5, 0 }, //  9 : 0,3 over = standard case
		{ 33, 1, 2, 0, 4, 5, 3, 0 }, // 10
		{ 11, 0, 1, 2, 3, 4, 5, 0 }, // 11 : 0,1,3 over = standard case
		{ 17, 2, 0, 1, 5, 3, 4, 0 }, // 12
		{ 19, 2, 0, 1, 5, 3, 4, 0 }, // 13
		{ 35, 1, 2, 0, 4, 5, 3, 0 }, // 14
		{  3, 5, 4, 3, 2, 1, 0, 1 }, // 15
		{  1, 4, 3, 5, 1, 0, 2, 0 }, // 16
		{ 17, 0, 1, 2, 3, 4, 5, 0 }, // 17 : 0,4 over = standard case
		{  9, 1, 2, 0, 4, 5, 3, 0 }, // 18
		{ 19, 0, 1, 2, 3, 4, 5, 0 }, // 19 : 0,1,4 over = standard case
		{ 33, 2, 0, 1, 5, 3, 4, 0 }, // 20
		{ 35, 2, 0, 1, 5, 3, 4, 0 }, // 21
		{ 11, 1, 2, 0, 4, 5, 3, 0 }, // 22
		{  3, 3, 5, 4, 0, 2, 1, 1 }, // 23
		{  3, 4, 3, 5, 1, 0, 2, 0 }, // 24
		{ 19, 4, 3, 5, 1, 0, 2, 0 }, // 25
		{ 11, 4, 3, 5, 1, 0, 2, 0 }, // 26
		{  9, 2, 0, 1, 5, 3, 4, 1 }, // 27
		{ 35, 4, 3, 5, 1, 0, 2, 0 }, // 28
		{ 17, 1, 2, 0, 4, 5, 3, 1 }, // 29
		{ 33, 0, 1, 2, 3, 4, 5, 1 }, // 30 : 1,2,3,4 over = case 33 inverse
		{  1, 5, 4, 3, 2, 1, 0, 1 }, // 31
		{  1, 5, 4, 3, 2, 1, 0, 0 }, // 32
		{ 33, 0, 1, 2, 3, 4, 5, 0 }, // 33 : 0,5 over = standard case
		{ 17, 1, 2, 0, 4, 5, 3, 0 }, // 34
		{ 35, 0, 1, 2, 3, 4, 5, 0 }, // 35 : 0,1,5 over = standard case
		{  9, 2, 0, 1, 5, 3, 4, 0 }, // 36
		{ 11, 2, 0, 1, 5, 3, 4, 0 }, // 37
		{ 19, 1, 2, 0, 4, 5, 3, 0 }, // 38
		{  3, 4, 3, 5, 1, 0, 2, 1 }, // 39
		{  3, 3, 5, 4, 0, 2, 1, 0 }, // 40
		{ 11, 3, 5, 4, 0, 2, 1, 0 }, // 41
		{ 35, 3, 5, 4, 0, 2, 1, 0 }, // 42
		{ 33, 2, 0, 1, 5, 3, 4, 1 }, // 43
		{ 19, 3, 5, 4, 0, 2, 1, 0 }, // 44
		{  9, 1, 2, 0, 4, 5, 3, 1 }, // 45
		{ 17, 0, 1, 2, 3, 4, 5, 1 }, // 46 : 1,2,3,5 over = case 17 inverse
		{  1, 4, 3, 5, 1, 0, 2, 1 }, // 47
		{  3, 5, 4, 3, 2, 1, 0, 0 }, // 48
		{ 35, 5, 4, 3, 2, 1, 0, 0 }, // 49
		{ 19, 5, 4, 3, 2, 1, 0, 0 }, // 50
		{ 17, 2, 0, 1, 5, 3, 4, 1 }, // 51
		{ 11, 5, 4, 3, 2, 1, 0, 0 }, // 52
		{ 33, 1, 2, 0, 4, 5, 3, 1 }, // 53
		{  9, 0, 1, 2, 3, 4, 5, 1 }, // 54 : 1,2,4,5 over = case 9 inverse
		{  1, 3, 5, 4, 0, 2, 1, 1 }, // 55
		{  7, 5, 4, 3, 2, 1, 0, 0 }, // 56
		{  3, 1, 2, 0, 4, 5, 3, 1 }, // 57
		{  3, 2, 0, 1, 5, 3, 4, 1 }, // 58
		{  1, 2, 0, 1, 5, 3, 4, 1 }, // 59
		{  3, 0, 1, 2, 3, 4, 5, 1 }, // 60 : 2,3,4,5 over = case 3 inverse
		{  1, 1, 2, 0, 4, 5, 3, 1 }, // 61
		{  1, 0, 1, 2, 3, 4, 5, 1 }, // 62 : 1,2,3,4,5 over = case 1 inverse
		{  0, 0, 0, 0, 0, 0, 0, 0 }  // 63
	};

	ENTER(Isosurface_builder::cross_triangle_wedge);
	int return_code = 1;
	int unrotated_case =
		((get_scalar(p0) > current_iso_value) ?  1 : 0) +
		((get_scalar(p1) > current_iso_value) ?  2 : 0) +
		((get_scalar(p2) > current_iso_value) ?  4 : 0) +
		((get_scalar(p3) > current_iso_value) ?  8 : 0) +
		((get_scalar(p4) > current_iso_value) ? 16 : 0) +
		((get_scalar(p5) > current_iso_value) ? 32 : 0);
	int final_case = tri_case[unrotated_case][0];
	if (0 < final_case)
	{
		const bool inverse = (0 != tri_case[unrotated_case][7]);
		Point_index mp[6] = { p0, p1, p2, p3, p4, p5 };
		Point_index mp0 = mp[tri_case[unrotated_case][1]];
		Point_index mp1 = mp[tri_case[unrotated_case][2]];
		Point_index mp2 = mp[tri_case[unrotated_case][3]];
		Point_index mp3 = mp[tri_case[unrotated_case][4]];
		Point_index mp4 = mp[tri_case[unrotated_case][5]];
		Point_index mp5 = mp[tri_case[unrotated_case][6]];
		const Iso_vertex *v1, *v2, *v3, *v4, *v5, *v6;
		Iso_mesh& mesh = get_mesh();
		switch (final_case)
		{
		case 1: // 0 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp1));
			v2 = get_line_crossing(Point_index_pair(mp0, mp3));
			v3 = get_line_crossing(Point_index_pair(mp0, mp2));
			mesh.add_triangle(v1, v2, v3, inverse);
			break;
		case 3: // 0,1 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp3));
			v2 = get_line_crossing(Point_index_pair(mp0, mp2));
			v3 = get_line_crossing(Point_index_pair(mp1, mp2));
			v4 = get_line_crossing(Point_index_pair(mp1, mp4));
			mesh.add_quadrilateral(v1, v2, v3, v4, inverse);
			break;
		case 7: // 0,1,2 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp0, mp3));
			v2 = get_line_crossing(Point_index_pair(mp2, mp5));
			v3 = get_line_crossing(Point_index_pair(mp1, mp4));
			mesh.add_triangle(v1, v2, v3);
			break;
		case 9: // 0,3 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp2));
			v2 = get_line_crossing(Point_index_pair(mp0, mp1));
			v3 = get_line_crossing(Point_index_pair(mp3, mp4));
			v4 = get_line_crossing(Point_index_pair(mp3, mp5));
			mesh.add_quadrilateral(v1, v2, v3, v4, inverse);
			break;
		case 11: // 0,1,3 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp0, mp2));
			v2 = get_line_crossing(Point_index_pair(mp1, mp2));
			v3 = get_line_crossing(Point_index_pair(mp1, mp4));
			v4 = get_line_crossing(Point_index_pair(mp3, mp4));
			v5 = get_line_crossing(Point_index_pair(mp3, mp5));
			mesh.add_pentagon(v1, v2, v3, v4, v5);
			break;
		case 17: // 0,4 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp3));
			v2 = get_line_crossing(Point_index_pair(mp0, mp2));
			v3 = get_line_crossing(Point_index_pair(mp0, mp1));
			v4 = get_line_crossing(Point_index_pair(mp4, mp1));
			v5 = get_line_crossing(Point_index_pair(mp4, mp5));
			v6 = get_line_crossing(Point_index_pair(mp4, mp3));
			if (ambiguous_quad_resolves_over(mp0, mp1, mp3, mp4) != inverse)
			{
				mesh.add_quadrilateral(v1, v2, v5, v6, inverse);
				mesh.add_quadrilateral(v2, v3, v4, v5, inverse);
			}
			else
			{
				mesh.add_triangle(v1, v2, v3, inverse);
				mesh.add_triangle(v4, v5, v6, inverse);
			}
			break;
		case 19: // 0,1,4 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp0, mp3));
			v2 = get_line_crossing(Point_index_pair(mp0, mp2));
			v3 = get_line_crossing(Point_index_pair(mp1, mp2));
			v4 = get_line_crossing(Point_index_pair(mp4, mp5));
			v5 = get_line_crossing(Point_index_pair(mp4, mp3));
			mesh.add_pentagon(v1, v2, v3, v4, v5);
			break;
		case 33: // 0,5 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp2));
			v2 = get_line_crossing(Point_index_pair(mp0, mp1));
			v3 = get_line_crossing(Point_index_pair(mp0, mp3));
			v4 = get_line_crossing(Point_index_pair(mp5, mp3));
			v5 = get_line_crossing(Point_index_pair(mp5, mp4));
			v6 = get_line_crossing(Point_index_pair(mp5, mp2));
			if (ambiguous_quad_resolves_over(mp0, mp2, mp3, mp5) != inverse)
			{
				mesh.add_quadrilateral(v1, v2, v5, v6, inverse);
				mesh.add_quadrilateral(v2, v3, v4, v5, inverse);
			}
			else
			{
				mesh.add_triangle(v1, v2, v3, inverse);
				mesh.add_triangle(v4, v5, v6, inverse);
			}
			break;
		case 35: // 0,1,5 > iso_value
		{
			// Double ambiguous double contour faces:
			// divide into 2 tetrahedra plus 3 pyramids with new centre point sample
			// first get centre point = average of vertex xi coordinates
			FE_value scalar_FE_value, xi[3], xi_c[3];
			xi_c[0] = 0.0;
			xi_c[1] = 0.0;
			xi_c[2] = 0.0;
			for (int i = 0; i < 6; i++)
			{
				get_xi(mp[i], xi);
				xi_c[0] += xi[0];
				xi_c[1] += xi[1];
				xi_c[2] += xi[2];
			}
			xi_c[0] /= 6.0;
			xi_c[1] /= 6.0;
			xi_c[2] /= 6.0;
			this->field_cache->setMeshLocation(element, xi_c);
			cmzn_field_evaluate_real(scalar_field, field_cache, 1, &scalar_FE_value);
			Point_index pc(xi_c,  static_cast<double>(scalar_FE_value));
			cross_tetrahedron(p0, p1, p2, pc);
			cross_pyramid(p0, p3, p1, p4, pc);
			cross_pyramid(p1, p4, p2, p5, pc);
			cross_pyramid(p2, p5, p0, p3, pc);
			cross_tetrahedron(p3, p5, p4, pc);
			break;
		}
		default:
			display_message(ERROR_MESSAGE,
				"Isosurface_builder::cross_triangle_wedge.  Unknown case %d (unrotated %d)", final_case, unrotated_case);
			return_code = 0;
			break;
		}
	}
	LEAVE;

	return (return_code);
}

int Isosurface_builder::sweep()
{
	ENTER(Isosurface_builder::sweep);
	int return_code = 1;
	double scalar_value;
	FE_value scalar_FE_value, xi[3];

	unsigned int point_index = 0;
	for (int k = number_in_xi3; (k >= 0) && return_code; k--)
	{
		xi[2] = k*delta_xi3;
		int mod_number_in_xi2 = number_in_xi2;
		if (simplex23)
		{
			mod_number_in_xi2 -= k; // assume number_in_xi equal on linked axes
		}

		for (int j = mod_number_in_xi2; (j >= 0) && return_code; j--)
		{
			xi[1] = j*delta_xi2;
			int mod_number_in_xi1 = number_in_xi1;
			if (simplex12)
			{
				mod_number_in_xi1 -= j;
			}
			if (simplex13)
			{
				mod_number_in_xi1 -= k;
			}

			for (int i = mod_number_in_xi1; i >= 0; i--)
			{
				xi[0] = i*delta_xi1;
				if ((CMZN_OK == this->field_cache->setIndexedMeshLocation(point_index, element, xi)) &&
					(CMZN_OK == cmzn_field_evaluate_real(scalar_field, field_cache, 1, &scalar_FE_value)))
				{
					scalar_value = static_cast<double>(scalar_FE_value);
					set_scalar(i, j, k, scalar_value);

					for (int v = 0; v < number_of_iso_values; v++)
					{
						current_iso_value_number = v;
						current_iso_value = specification.get_iso_value(v);

						if ((i < mod_number_in_xi1) && (j < mod_number_in_xi2) &&
							(k < number_in_xi3))
						{
							if (cube)
							{
								cross_cube(i, j, k);
							}
							else if (tetrahedron)
							{
								if (i < (mod_number_in_xi1 - 1))
								{
									if (i < (mod_number_in_xi1 - 2))
									{
										cross_tetrahedron(
											Point_index(i + 1, j + 1, k    ),
											Point_index(i    , j + 1, k + 1),
											Point_index(i + 1, j    , k + 1),
											Point_index(i + 1, j + 1, k + 1));
									}
									cross_octahedron(i, j, k);
								}
								cross_tetrahedron(
									Point_index(i, j, k),
									Point_index(i + 1, j, k),
									Point_index(i, j + 1, k),
									Point_index(i, j, k + 1));
							}
							else if (simplex12)
							{
								if (i < (mod_number_in_xi1 - 1))
								{
									cross_triangle_wedge(
										Point_index(i + 1, j, k    ), Point_index(i + 1, j + 1, k    ), Point_index(i, j + 1, k    ),
										Point_index(i + 1, j, k + 1), Point_index(i + 1, j + 1, k + 1), Point_index(i, j + 1, k + 1));
								}
								cross_triangle_wedge(
									Point_index(i, j, k    ), Point_index(i + 1, j, k    ), Point_index(i, j + 1, k    ),
									Point_index(i, j, k + 1), Point_index(i + 1, j, k + 1), Point_index(i, j + 1, k + 1));
							}
							else if (simplex23)
							{
								if (j < (mod_number_in_xi2 - 1))
								{
									cross_triangle_wedge(
										Point_index(i    , j + 1, k), Point_index(i    , j + 1, k + 1), Point_index(i    , j, k + 1),
										Point_index(i + 1, j + 1, k), Point_index(i + 1, j + 1, k + 1), Point_index(i + 1, j, k + 1));
								}
								cross_triangle_wedge(
									Point_index(i    , j, k), Point_index(i    , j + 1, k), Point_index(i    , j, k + 1),
									Point_index(i + 1, j, k), Point_index(i + 1, j + 1, k), Point_index(i + 1, j, k + 1));
							}
							else if (simplex13)
							{
								if (i < (mod_number_in_xi1 - 1))
								{
									cross_triangle_wedge(
										Point_index(i + 1, j    , k), Point_index(i, j    , k + 1), Point_index(i + 1, j    , k + 1),
										Point_index(i + 1, j + 1, k), Point_index(i, j + 1, k + 1), Point_index(i + 1, j + 1, k + 1));
								}
								cross_triangle_wedge(
									Point_index(i, j    , k), Point_index(i, j    , k + 1), Point_index(i + 1, j    , k),
									Point_index(i, j + 1, k), Point_index(i, j + 1, k + 1), Point_index(i + 1, j + 1, k));
							}
						}
					}
				}
				else
				{
					return_code = 0;
					break;
				}
				++point_index;
			}
		}
	}
	LEAVE;

	return (return_code);
}

bool Isosurface_builder::reverse_winding()
{
	FE_value result[3], winding_coordinate_derivative1[3],
		winding_coordinate_derivative2[3], winding_coordinate_derivative3[3];
	FE_value_triple *xi_points;
	int number_in_xi[3], number_of_xi_points_created[3];

	/* Determine whether xi forms a LH or RH coordinate system in this element */
	bool reverse_winding = false;
	FE_element_shape *shape = get_FE_element_shape(element);
	number_in_xi[0] = 1;
	number_in_xi[1] = 1;
	number_in_xi[2] = 1;
	if (FE_element_shape_get_xi_points_cell_centres(shape,
			number_in_xi, number_of_xi_points_created, &xi_points))
	{
		cmzn_differentialoperator_id d_dxi1 = cmzn_mesh_get_chart_differentialoperator(mesh, /*order*/1, 1);
		cmzn_differentialoperator_id d_dxi2 = cmzn_mesh_get_chart_differentialoperator(mesh, /*order*/1, 2);
		cmzn_differentialoperator_id d_dxi3 = cmzn_mesh_get_chart_differentialoperator(mesh, /*order*/1, 3);
		if ((CMZN_OK == this->field_cache->setMeshLocation(element, xi_points[0])) &&
			(CMZN_OK == cmzn_field_evaluate_derivative(coordinate_field, d_dxi1, field_cache, 3, winding_coordinate_derivative1)) &&
			(CMZN_OK == cmzn_field_evaluate_derivative(coordinate_field, d_dxi2, field_cache, 3, winding_coordinate_derivative2)) &&
			(CMZN_OK == cmzn_field_evaluate_derivative(coordinate_field, d_dxi3, field_cache, 3, winding_coordinate_derivative3)))
		{
			cross_product_FE_value_vector3(winding_coordinate_derivative1, winding_coordinate_derivative2, result);
			if ((result[0] * winding_coordinate_derivative3[0] +
				 result[1] * winding_coordinate_derivative3[1] +
				 result[2] * winding_coordinate_derivative3[2]) < 0)
			{
				reverse_winding = true;
			}
		}
		DEALLOCATE(xi_points);
		cmzn_differentialoperator_destroy(&d_dxi1);
		cmzn_differentialoperator_destroy(&d_dxi2);
		cmzn_differentialoperator_destroy(&d_dxi3);
	}
	return (reverse_winding);
}

inline void Isosurface_builder::add_vertex_array_entries(struct Graphics_vertex_array *array,
	enum Graphics_vertex_array_attribute_type type, int number_of_components,
	const FE_value *values1, const FE_value *values2, const FE_value *values3)
{
	GLfloat *floatField = new GLfloat[number_of_components];
	CAST_TO_OTHER(floatField,values1, GLfloat, number_of_components);
	array->add_float_attribute(type,
		number_of_components, 1, floatField);
	CAST_TO_OTHER(floatField,values2, GLfloat, number_of_components);
	array->add_float_attribute(type,
		number_of_components, 1, floatField);
	CAST_TO_OTHER(floatField,values3, GLfloat, number_of_components);
	array->add_float_attribute(type,
		number_of_components, 1, floatField);
	delete[] floatField;

}

inline void Isosurface_builder::replace_vertex_array_entries(struct Graphics_vertex_array *array,
	enum Graphics_vertex_array_attribute_type type, int number_of_components,
	const FE_value *values1, const FE_value *values2, const FE_value *values3,
	unsigned int insert_vertex_location)
{
	GLfloat *floatField = new GLfloat[number_of_components];
	CAST_TO_OTHER(floatField,values1, GLfloat, number_of_components);
	array->replace_float_vertex_buffer_at_position(type,
		insert_vertex_location, number_of_components, 1, floatField);
	CAST_TO_OTHER(floatField,values2, GLfloat, number_of_components);
	array->replace_float_vertex_buffer_at_position(type,
		insert_vertex_location+1, number_of_components, 1, floatField);
	CAST_TO_OTHER(floatField,values3, GLfloat, number_of_components);
	array->replace_float_vertex_buffer_at_position(type,
		insert_vertex_location+2, number_of_components, 1, floatField);
	delete[] floatField;
}

/* replace graphics as it already exist be the locations should be updated/expanded */
int Isosurface_builder::fill_graphics(struct Graphics_vertex_array *array)
{
	int return_code = 1;
	bool reverse = reverse_winding();
	unsigned int vertex_start = array->get_number_of_vertices(
		GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION);
	unsigned int total_number_of_triangles_to_add = 0;
	int polygonType = (int)g_TRIANGLE;
	const DsLabelIndex index = get_FE_element_index(element);
	int current_location = 0, number_of_locations = 0, *locations = 0;
	/* get all surface entries of this elements */
	array->get_all_fast_search_id_locations(index,
		&number_of_locations, &locations);
	int vertex_location = 0;
	unsigned int number_of_available_vertices = 0, insert_vertex_location = 0;
	GLfloat place_holder[3];

	total_number_of_triangles_to_add = 0;
	for (Iso_mesh_map_const_iterator mesh_iter = mesh_map.begin();
		mesh_iter != mesh_map.end(); mesh_iter++)
	{
		Iso_mesh& mesh = *(mesh_iter->second);
		const Iso_triangle_list& triangle_list = mesh.triangle_list;
		const size_t number_of_triangles = triangle_list.size();
		if (0 < number_of_triangles)
		{
			for (Iso_triangle_list_const_iterator triangle_iter = triangle_list.begin();
				triangle_iter != triangle_list.end(); triangle_iter++)
			{
				/* get and refill invalidated vertices if available, if no more available,
				 * the total_number_of_triangles_to_add will increment by 1 for each triangle
				 * and added as another surface entry */
				if ((number_of_available_vertices < 3) && (number_of_locations > 0) &&
					(number_of_locations > current_location)	&& locations)
				{
					vertex_location = locations[current_location];
					array->get_unsigned_integer_attribute(
						GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
						vertex_location, 1, &number_of_available_vertices);
					array->get_unsigned_integer_attribute(
						GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
						vertex_location, 1, &insert_vertex_location);
					current_location++;
					/*validate these vertices */
					array->replace_integer_vertex_buffer_at_position(
						GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_OBJECT_ID, vertex_location, 1, 1,
						&index);
					int updated = 0;
					array->replace_integer_vertex_buffer_at_position(
						GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_UPDATE_REQUIRED,
						vertex_location, 1, 1, &updated);
				}

				const Iso_triangle *triangle = *triangle_iter;
				const Iso_vertex* v1 = triangle->v1;
				const Iso_vertex* v2 = reverse ? triangle->v3 : triangle->v2;
				const Iso_vertex* v3 = reverse ? triangle->v2 : triangle->v3;

				if (number_of_available_vertices < 3)
				{
					add_vertex_array_entries(array, GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
						3, &(v1->coordinates[0]), &(v2->coordinates[0]), &(v3->coordinates[0]));
				}
				else
				{
					replace_vertex_array_entries(array, GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
						3, &(v1->coordinates[0]), &(v2->coordinates[0]), &(v3->coordinates[0]),
						insert_vertex_location);
					CAST_TO_OTHER(place_holder,v3->coordinates,GLfloat,3);
				}

				// calculate facet normal:
				FE_value axis1[3], axis2[3], facet_normal[3];
				axis1[0] = v2->coordinates[0] - v1->coordinates[0];
				axis1[1] = v2->coordinates[1] - v1->coordinates[1];
				axis1[2] = v2->coordinates[2] - v1->coordinates[2];
				axis2[0] = v3->coordinates[0] - v1->coordinates[0];
				axis2[1] = v3->coordinates[1] - v1->coordinates[1];
				axis2[2] = v3->coordinates[2] - v1->coordinates[2];
				cross_product_FE_value_vector3(axis1, axis2, facet_normal);
				normalize_FE_value3(facet_normal);
				if (number_of_available_vertices < 3)
				{
					add_vertex_array_entries(array, GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
						3, &(facet_normal[0]), &(facet_normal[0]), &(facet_normal[0]));
				}
				else
				{
					replace_vertex_array_entries(array, GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
						3, &(facet_normal[0]), &(facet_normal[0]), &(facet_normal[0]),
						insert_vertex_location);
				}
				if (0 != texture_coordinate_field)
				{
					if (number_of_available_vertices < 3)
					{
						add_vertex_array_entries(array, GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_TEXTURE_COORDINATE_ZERO,
							3, &(v1->texture_coordinates[0]), &(v2->texture_coordinates[0]), &(v3->texture_coordinates[0]));
					}
					else
					{
						replace_vertex_array_entries(array, GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_TEXTURE_COORDINATE_ZERO,
							3, &(v1->texture_coordinates[0]), &(v2->texture_coordinates[0]), &(v3->texture_coordinates[0]),
							insert_vertex_location);
					}
				}
				if (0 != data_field)
				{
					if (number_of_available_vertices < 3)
					{
						add_vertex_array_entries(array, GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
							number_of_data_components, &(v1->data[0]),
							&(v2->data[0]), &(v3->data[0]));
					}
					else
					{
						replace_vertex_array_entries(array, GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
							number_of_data_components, &(v1->data[0]),
							&(v2->data[0]), &(v3->data[0]),
							insert_vertex_location);
					}
				}
				if (number_of_available_vertices < 3)
				{
					total_number_of_triangles_to_add++;
				}
				else
				{
					number_of_available_vertices -= 3;
					insert_vertex_location += 3;
				}
			}
		}
	}
	if (number_of_available_vertices > 0)
	{
		/* fill the remaining available_vertices with the last vertex location */
		for (unsigned int j = 0; j < number_of_available_vertices; j++)
		{
			array->replace_float_vertex_buffer_at_position(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
				insert_vertex_location+j, 3, 1, place_holder);
		}
	}
	if (total_number_of_triangles_to_add > 0)
	{
		unsigned int number_of_vertices = total_number_of_triangles_to_add * 3;
		unsigned int number_of_xi1 = total_number_of_triangles_to_add;
		unsigned int number_of_xi2 = 3;
		array->add_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_OBJECT_ID,
			1, 1, &index);
		array->add_fast_search_id(index);
		int modificationRequired = 0;
		array->add_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_UPDATE_REQUIRED,
			1, 1, &modificationRequired);
		array->add_unsigned_integer_attribute(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
			1, 1, &number_of_vertices);
		array->add_unsigned_integer_attribute(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
			1, 1, &vertex_start);
		array->add_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POLYGON,
			1, 1, &polygonType);
		array->add_unsigned_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_XI1,
			1, 1, &number_of_xi1);
		array->add_unsigned_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_XI2,
			1, 1, &number_of_xi2);
	}
	if (locations)
		delete[] locations;
	return (return_code);
}

} // anonymous namespace

/*
Global functions
----------------
*/

struct Iso_surface_specification *Iso_surface_specification_create(
	int number_of_iso_values, const double *iso_values,
	double first_iso_value, double last_iso_value,
	struct Computed_field *coordinate_field, struct Computed_field *data_field,
	struct Computed_field *scalar_field,
	struct Computed_field *texture_coordinate_field)
{
	ENTER(Iso_surface_specification_create);
	struct Iso_surface_specification *specification = NULL;
	if (Computed_field_has_3_components(coordinate_field, NULL) &&
		(0 <= number_of_iso_values) &&
		(1 == cmzn_field_get_number_of_components(scalar_field)) &&
		((NULL == texture_coordinate_field) ||
			(3 >= cmzn_field_get_number_of_components(texture_coordinate_field))))
	{
		ALLOCATE(specification, struct Iso_surface_specification, 1);
		if (NULL != specification)
		{
			specification->number_of_iso_values = number_of_iso_values;
			specification->coordinate_field = ACCESS(Computed_field)(coordinate_field);
			specification->data_field =
				(NULL != data_field) ? ACCESS(Computed_field)(data_field) : NULL;
			specification->scalar_field = ACCESS(Computed_field)(scalar_field);
			specification->texture_coordinate_field =
				(NULL != texture_coordinate_field) ? ACCESS(Computed_field)(texture_coordinate_field) : NULL;
			specification->number_of_data_components = (NULL != data_field) ?
				cmzn_field_get_number_of_components(data_field) : 0;
			specification->iso_values = NULL;
			specification->first_iso_value = first_iso_value;
			specification->last_iso_value = last_iso_value;
			specification->iso_value_range = 0;
			if (NULL != iso_values)
			{
				if (0 < number_of_iso_values)
				{
					specification->iso_values = new double[number_of_iso_values];
					memcpy(specification->iso_values, iso_values, number_of_iso_values*sizeof(double));
				}
			}
			else if (1 < number_of_iso_values)
			{
				specification->iso_value_range = (last_iso_value - first_iso_value)
					/ (double)(number_of_iso_values - 1);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Iso_surface_specification_create.  Invalid argument(s)");
	}
	LEAVE;

	return (specification);
}

int Iso_surface_specification_destroy(
	struct Iso_surface_specification **specification_address)
{
	int return_code;
	struct Iso_surface_specification *specification;

	ENTER(Iso_surface_specification_destroy);
	if ((NULL != specification_address) &&
		(NULL != (specification = *specification_address)))
	{
		delete[] specification->iso_values;
		DEACCESS(Computed_field)(&specification->coordinate_field);
		REACCESS(Computed_field)(&specification->data_field, NULL);
		DEACCESS(Computed_field)(&specification->scalar_field);
		REACCESS(Computed_field)(&specification->texture_coordinate_field, NULL);
		DEALLOCATE(*specification_address);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Iso_surface_specification_destroy.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
}

int create_iso_surfaces_from_FE_element(struct FE_element *element,
	cmzn_fieldcache_id field_cache, cmzn_mesh_id mesh,
	struct Graphics_vertex_array *array,
	int *number_in_xi, struct Iso_surface_specification *specification)
{
	ENTER(create_iso_surfaces_from_FE_element);
	int return_code = 0;
	if ((NULL != element) && field_cache && mesh && array && (3 == get_FE_element_dimension(element)) &&
		(NULL != number_in_xi) &&
		(0 < number_in_xi[0]) && (0 < number_in_xi[1]) && (0 < number_in_xi[2]) &&
		(NULL != specification) && array)
	{
		int replaceRequired = 0;
		/* find if vertex already in the array */
		int vertex_location = array->find_first_fast_search_id_location(get_FE_element_index(element));
		/* vertex found in array, check if update is required */
		if (vertex_location >= 0)
		{
			array->get_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_UPDATE_REQUIRED,
				vertex_location, 1, &replaceRequired);
		}
		/* Vertices not available or should be repalced/expanded */
		if (vertex_location < 0 || replaceRequired)
		{
			Isosurface_builder iso_builder(element, field_cache, mesh,
				number_in_xi[0], number_in_xi[1], number_in_xi[2], *specification);
			return_code = iso_builder.sweep();
			if (return_code)
			{
				return_code = iso_builder.fill_graphics(array);
			}
		}
		else /* do not waste time calculating existing valid graphics */
		{
			return_code = 1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"create_iso_surfaces_from_FE_element.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* create_iso_surfaces_from_FE_element */

