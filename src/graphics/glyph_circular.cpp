/**
 * FILE : glyph_circular.hpp
 *
 * Internal header for glyphs with circular features which are drawn with
 * tessellation circle divisions.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <cmath>
#include "opencmiss/zinc/status.h"
#include "opencmiss/zinc/tessellation.h"
#include "general/debug.h"
#include "general/message.h"
#include "graphics/glyph_circular.hpp"
#include "graphics/graphics_object_private.hpp"

/*
Module functions
----------------
*/

/**
 * Adds vertices and normals for a tube/cone/anulus/disc stretching from
 * position x1 with radius r1 to position x2 with radius r2. The axis of the
 * cylinder is parallel with the x axis and its centre is at cy,cz. If
 * <primary_axis> is 1, the above is the case, if its value is 2, x->y, y->z
 * and z->x, and a further permutation if <primary_axis> is 3. Values other than
 * 1, 2 or 3 are taken as 1.
 * The vertices and normals are added to create a single quadrilateral strip
 * suitable for using with GT_surfaces of type g_SH_DISCONTINUOUS_STRIP.
 */
int construct_tube(int number_of_segments_around,ZnReal x1,ZnReal r1,
	ZnReal x2,ZnReal r2,ZnReal cy,ZnReal cz,int primary_axis,Triple *vertex_list,
	Triple *normal_list)
{
	ZnReal longitudinal_normal,normal_angle,radial_normal,theta,y,z;
	int j,ix,iy,iz,return_code;
	Triple *normal,*vertex;

	ENTER(construct_tube);
	if ((2<number_of_segments_around)&&((x1 != x2)||(r1 != r2))&&
		vertex_list&&normal_list)
	{
		return_code=1;
		switch (primary_axis)
		{
			case 2:
			{
				ix=1;
				iy=2;
				iz=0;
			} break;
			case 3:
			{
				ix=2;
				iy=0;
				iz=1;
			} break;
			default:
			{
				ix=0;
				iy=1;
				iz=2;
			} break;
		}
		vertex=vertex_list;
		normal=normal_list;
		/* get radial and longitudinal components of surface normals */
		normal_angle=atan2(r2-r1,x2-x1);
		radial_normal=cos(normal_angle);
		longitudinal_normal=-sin(normal_angle);
		for (j=0;j <= number_of_segments_around;j++)
		{
			theta=2.0*PI*(ZnReal)j/(ZnReal)number_of_segments_around;
			y = sin(theta);
			z = cos(theta);
			(*vertex)[ix] = x1;
			(*vertex)[iy] = cy+r1*y;
			(*vertex)[iz] = cz+r1*z;
			vertex++;
			(*vertex)[ix] = x2;
			(*vertex)[iy] = cy+r2*y;
			(*vertex)[iz] = cz+r2*z;
			vertex++;
			y *= radial_normal;
			z *= radial_normal;
			(*normal)[ix] = longitudinal_normal;
			(*normal)[iy] = y;
			(*normal)[iz] = z;
			normal++;
			(*normal)[ix] = longitudinal_normal;
			(*normal)[iy] = y;
			(*normal)[iz] = z;
			normal++;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"construct_tube.  Invalid argument(s)");
		return_code=0;
	}
	return (return_code);
}

struct GT_object *create_GT_object_arrow_solid(const char *name, int primary_axis,
	int number_of_segments_around,ZnReal shaft_length,ZnReal shaft_radius,
	ZnReal cone_radius)
{
	ZnReal r1 = 0.0, r2 = 0.0, x1 = 0.0, x2 = 0.0;
	int i;
	struct GT_object *glyph = 0;
	Triple *points = NULL, *normalpoints = NULL;
	if (name&&(2<number_of_segments_around)&&(0<shaft_radius)&&(1>shaft_radius)&&
		(0<shaft_length)&&(1>shaft_length))
	{
		glyph=CREATE(GT_object)(name, g_SURFACE_VERTEX_BUFFERS,	(cmzn_material *)NULL);
		GT_surface_vertex_buffers *surface = CREATE(GT_surface_vertex_buffers)(
			g_SHADED_TEXMAP,CMZN_GRAPHICS_RENDER_POLYGON_MODE_SHADED);
		for (i=0;(i<4)&&glyph;i++)
		{
			if (ALLOCATE(points,Triple,2*(number_of_segments_around+1))&&
				ALLOCATE(normalpoints,Triple,2*(number_of_segments_around+1)))
			{
				switch (i)
				{
					case 0:
					{
						/* base of shaft */
						x1=0.0;
						r1=0.0;
						x2=0.0;
						r2=shaft_radius;
					} break;
					case 1:
					{
						/* shaft */
						x1=0.0;
						r1=shaft_radius;
						x2=shaft_length;
						r2=shaft_radius;
					} break;
					case 2:
					{
						/* base of head */
						x1=shaft_length;
						r1=shaft_radius;
						x2=shaft_length;
						r2=cone_radius;
					} break;
					case 3:
					{
						/* head */
						x1=shaft_length;
						r1=cone_radius;
						x2=1.0;
						r2=0.0;
					} break;
				}
				if (!construct_tube(number_of_segments_around,x1,r1,x2,r2,0.0,0.0,
					primary_axis,points,normalpoints))
				{
					DEALLOCATE(points);
					DEALLOCATE(normalpoints);
				}
			}
			if (!(points&&(fill_surface_graphics_vertex_array(GT_object_get_vertex_set(glyph),
					g_TRIANGLE, 2, number_of_segments_around+1,
					points,normalpoints,/*tangentpoints*/(Triple *)NULL,
					 /*texturepoints*/(Triple *)NULL, g_NO_DATA, (GLfloat *)NULL))))
			{
				DEACCESS(GT_object)(&glyph);
			}
			DEALLOCATE(points);
			DEALLOCATE(normalpoints);
		}

		if (glyph)
		{
			GT_OBJECT_ADD(GT_surface_vertex_buffers)(glyph, surface);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"create_GT_object_arrow_solid.  Error creating glyph");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"create_GT_object_arrow_solid.  Invalid argument(s)");
	}
	return (glyph);
}

/**
 * Creates a graphics object named <name> resembling a cone with the given
 * <number_of_segments_around>. The base of the cone is at <0,0,0> while its head
 * lies at <1,0,0>. The radius of the cone is 0.5 at its base.
 */
struct GT_object *create_GT_object_cone(const char *name,int number_of_segments_around)
{
	struct GT_object *glyph = 0;
	Triple *points,*normalpoints;

	if (name&&(2<number_of_segments_around))
	{
		if (ALLOCATE(points,Triple,2*(number_of_segments_around+1))&&
			ALLOCATE(normalpoints,Triple,2*(number_of_segments_around+1)))
		{
			construct_tube(number_of_segments_around, 0.0, 0.5, 1.0, 0.0, 0.0, 0.0, 1,
				points,normalpoints);
			glyph=CREATE(GT_object)(name, g_SURFACE_VERTEX_BUFFERS,	(cmzn_material *)NULL);
			GT_surface_vertex_buffers *surface = CREATE(GT_surface_vertex_buffers)(
				g_SHADED_TEXMAP,CMZN_GRAPHICS_RENDER_POLYGON_MODE_SHADED);
			if (fill_surface_graphics_vertex_array(GT_object_get_vertex_set(glyph),
				g_TRIANGLE, 2, number_of_segments_around+1,
				points,normalpoints,/*tangentpoints*/(Triple *)NULL,
				 /*texturepoints*/(Triple *)NULL, g_NO_DATA, (GLfloat *)NULL))
			{
				GT_OBJECT_ADD(GT_surface_vertex_buffers)(glyph, surface);
			}
			else
			{
				DESTROY(GT_surface_vertex_buffers)(&surface);
				DEACCESS(GT_object)(&glyph);
			}
			DEALLOCATE(points);
			DEALLOCATE(normalpoints);
		}
		if (!glyph)
		{
			display_message(ERROR_MESSAGE,"create_GT_object_cone.  Error creating glyph");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"create_GT_object_cone.  Invalid argument(s)");
	}
	return (glyph);
}

/**
 * Creates a graphics object named <name> resembling a cone with the given
 * <number_of_segments_around>. The base of the cone is at <0,0,0> while its head
 * lies at <1,0,0>. The radius of the cone is 0.5 at its base.  This cone has a
 * solid base.
 */
struct GT_object *create_GT_object_cone_solid(const char *name,int number_of_segments_around)
{
	struct GT_object *glyph = 0;
	Triple *points,*normalpoints;
	int return_code = 1;

	if (name&&(2<number_of_segments_around))
	{
		glyph=CREATE(GT_object)(name,g_SURFACE_VERTEX_BUFFERS,	(cmzn_material *)NULL);
		GT_surface_vertex_buffers *surface = CREATE(GT_surface_vertex_buffers)(
			g_SHADED_TEXMAP,CMZN_GRAPHICS_RENDER_POLYGON_MODE_SHADED);
		if (glyph)
		{
			if (ALLOCATE(points,Triple,2*(number_of_segments_around+1))&&
				ALLOCATE(normalpoints,Triple,2*(number_of_segments_around+1)))
			{
				construct_tube(number_of_segments_around, 0.0, 0.5, 1.0, 0.0, 0.0, 0.0, 1,
					points,normalpoints);
				if (return_code && ( 0 == fill_surface_graphics_vertex_array(GT_object_get_vertex_set(glyph),
					g_TRIANGLE, 2, number_of_segments_around+1,
					points, normalpoints,/*tangentpoints*/(Triple *)NULL, /*texturepoints*/(Triple *)NULL,
					g_NO_DATA, (GLfloat *)NULL)))
				{
					return_code = 0;
				}
				DEALLOCATE(points);
				DEALLOCATE(normalpoints);
			}
			if (ALLOCATE(points,Triple,2*(number_of_segments_around+1))&&
				ALLOCATE(normalpoints,Triple,2*(number_of_segments_around+1)))
			{
				construct_tube(number_of_segments_around, 0.0, 0.0, 0.0, 0.5, 0.0, 0.0, 1,
					points,normalpoints);
				if (return_code && ( 0 == fill_surface_graphics_vertex_array(GT_object_get_vertex_set(glyph),
					g_TRIANGLE, 2, number_of_segments_around+1,
					points, normalpoints,/*tangentpoints*/(Triple *)NULL, /*texturepoints*/(Triple *)NULL,
					g_NO_DATA, (GLfloat *)NULL)))
				{
					return_code = 0;
				}
				DEALLOCATE(points);
				DEALLOCATE(normalpoints);
			}
			if (return_code)
			{
				GT_OBJECT_ADD(GT_surface_vertex_buffers)(glyph, surface);
			}
			else
			{
				DESTROY(GT_surface_vertex_buffers)(&surface);
				DEACCESS(GT_object)(&glyph);
			}
		}
		if (!glyph)
		{
			display_message(ERROR_MESSAGE,"create_GT_object_cone_solid.  Error creating glyph");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"create_GT_object_cone_solid.  Invalid argument(s)");
	}
	return (glyph);
}

/**
 * Creates a graphics object named <name> resembling a cylinder with the given
 * <number_of_segments_around>. The cylinder is centred at (0.5,0,0) and its axis
 * lies in the direction <1,0,0>. It fits into the unit cube spanning from
 * (0,-0.5,-0.5) to (0,+0.5,+0.5).
 */
struct GT_object *create_GT_object_cylinder(const char *name,int number_of_segments_around)
{
	struct GT_object *glyph = 0;
	Triple *points,*normalpoints;

	if (name&&(2<number_of_segments_around))
	{
		if (ALLOCATE(points,Triple,2*(number_of_segments_around+1))&&
			ALLOCATE(normalpoints,Triple,2*(number_of_segments_around+1)))
		{
			construct_tube(number_of_segments_around,0.0,0.5,1.0,0.5,0.0,0.0,1,
				points,normalpoints);
			glyph=CREATE(GT_object)(name, g_SURFACE_VERTEX_BUFFERS,	(cmzn_material *)NULL);
			GT_surface_vertex_buffers *surface = CREATE(GT_surface_vertex_buffers)(
				g_SHADED_TEXMAP,CMZN_GRAPHICS_RENDER_POLYGON_MODE_SHADED);
			if (fill_surface_graphics_vertex_array(GT_object_get_vertex_set(glyph),
				g_TRIANGLE, 2, number_of_segments_around+1,
				points,normalpoints,/*tangentpoints*/(Triple *)NULL,
				 /*texturepoints*/(Triple *)NULL, g_NO_DATA, (GLfloat *)NULL))
			{
				GT_OBJECT_ADD(GT_surface_vertex_buffers)(glyph, surface);
			}
			else
			{
				DESTROY(GT_surface_vertex_buffers)(&surface);
				DEACCESS(GT_object)(&glyph);
			}
			DEALLOCATE(points);
			DEALLOCATE(normalpoints);
		}
		if (!glyph)
		{
			display_message(ERROR_MESSAGE,
				"create_GT_object_cylinder.  Error creating glyph");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"create_GT_object_cylinder.  Invalid argument(s)");
	}
	return (glyph);
}

/**
 * Creates a graphics object named <name> resembling a cylinder with the given
 * <number_of_segments_around>. The cylinder is centred at (0.5,0,0) and its axis
 * lies in the direction <1,0,0>. It fits into the unit cube spanning from
 * (0,-0.5,-0.5) to (0,+0.5,+0.5).  This cylinder has its ends covered over.
 */
struct GT_object *create_GT_object_cylinder_solid(const char *name,int number_of_segments_around)
{
	struct GT_object *glyph = 0;
	Triple *points,*normalpoints;
	int return_code = 1;

	if (name&&(2<number_of_segments_around))
	{
		glyph=CREATE(GT_object)(name,g_SURFACE_VERTEX_BUFFERS, (cmzn_material *)NULL);
		GT_surface_vertex_buffers *surface = CREATE(GT_surface_vertex_buffers)(
			g_SHADED_TEXMAP,CMZN_GRAPHICS_RENDER_POLYGON_MODE_SHADED);
		if (glyph && surface)
		{
			if (ALLOCATE(points,Triple,2*(number_of_segments_around+1))&&
				ALLOCATE(normalpoints,Triple,2*(number_of_segments_around+1)))
			{
				construct_tube(number_of_segments_around,0.0,0.5,1.0,0.5,0.0,0.0,1,
					points,normalpoints);
				if (return_code && (0 == fill_surface_graphics_vertex_array(GT_object_get_vertex_set(glyph),
					g_TRIANGLE, 2, number_of_segments_around+1,
					points, normalpoints,/*tangentpoints*/(Triple *)NULL, /*texturepoints*/(Triple *)NULL,
					g_NO_DATA, (GLfloat *)NULL)))
				{
					return_code = 0;
				}
				DEALLOCATE(points);
				DEALLOCATE(normalpoints);
			}
			/* Cover over the ends */
			if (ALLOCATE(points,Triple,2*(number_of_segments_around+1))&&
				ALLOCATE(normalpoints,Triple,2*(number_of_segments_around+1)))
			{
				construct_tube(number_of_segments_around,0.0,0.0,0.0,0.5,0.0,0.0,1,
					points,normalpoints);
				if (return_code && ( 0 == fill_surface_graphics_vertex_array(GT_object_get_vertex_set(glyph),
					g_TRIANGLE, 2, number_of_segments_around+1,
					points, normalpoints,/*tangentpoints*/(Triple *)NULL, /*texturepoints*/(Triple *)NULL,
					g_NO_DATA, (GLfloat *)NULL)))
				{
					return_code = 0;
				}
				DEALLOCATE(points);
				DEALLOCATE(normalpoints);
			}
			if (ALLOCATE(points,Triple,2*(number_of_segments_around+1))&&
				ALLOCATE(normalpoints,Triple,2*(number_of_segments_around+1)))
			{
				construct_tube(number_of_segments_around,1.0,0.0,1.0,0.5,0.0,0.0,1,
					points,normalpoints);
				if (return_code && ( 0 == fill_surface_graphics_vertex_array(GT_object_get_vertex_set(glyph),
					g_TRIANGLE, 2, number_of_segments_around+1,
					points, normalpoints,/*tangentpoints*/(Triple *)NULL, /*texturepoints*/(Triple *)NULL,
					g_NO_DATA, (GLfloat *)NULL)))
				{
					return_code = 0;
				}
				DEALLOCATE(points);
				DEALLOCATE(normalpoints);
			}
			if (return_code)
			{
				GT_OBJECT_ADD(GT_surface_vertex_buffers)(glyph, surface);
			}
			else
			{
				DESTROY(GT_surface_vertex_buffers)(&surface);
				DEACCESS(GT_object)(&glyph);
			}
		}
		if (!glyph)
		{
			display_message(ERROR_MESSAGE,
				"create_GT_object_cylinder_solid.  Error creating glyph");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"create_GT_object_cylinder_solid.  Invalid argument(s)");
	}
	return (glyph);
}

struct GT_object *create_GT_object_sphere(const char *name,int number_of_segments_around,
	int number_of_segments_down)
{
	ZnReal longitudinal_normal,phi,radial_normal,theta,x,y,z;
	int i,j;
	struct GT_object *glyph = 0;
	Triple *normal,*normalpoints,*points,*vertex;

	if (name&&(2<number_of_segments_around)&&(1<number_of_segments_down))
	{
		if (ALLOCATE(points,Triple,
			(number_of_segments_down+1)*(number_of_segments_around+1))&&
			ALLOCATE(normalpoints,Triple,(number_of_segments_down+1)*
				(number_of_segments_around+1)))
		{
			/*vertex=points;
				normal=points+(number_of_segments_down+1)*(number_of_segments_around+1);*/
			for (i=0;i <= number_of_segments_down;i++)
			{
				phi=PI*(ZnReal)i/(ZnReal)number_of_segments_down;
				x=-0.5*cos(phi);
				radial_normal=sin(phi);
				longitudinal_normal=2*x;
				/*printf("x=%g l=%g r=%g\n",x,longitudinal_normal,radial_normal);*/
				vertex=points+i;
				normal=normalpoints+i;
				for (j=0;j <= number_of_segments_around;j++)
				{
					theta=2.0*PI*(ZnReal)j/(ZnReal)number_of_segments_around;
					y = radial_normal*sin(theta);
					z = radial_normal*cos(theta);
					(*vertex)[0] = x;
					(*vertex)[1] = 0.5*y;
					(*vertex)[2] = 0.5*z;
					vertex += (number_of_segments_down+1);
					(*normal)[0] = longitudinal_normal;
					(*normal)[1] = y;
					(*normal)[2] = z;
					normal += (number_of_segments_down+1);
				}
			}
			glyph=CREATE(GT_object)(name, g_SURFACE_VERTEX_BUFFERS,	(cmzn_material *)NULL);
			GT_surface_vertex_buffers *surface = CREATE(GT_surface_vertex_buffers)(
				g_SHADED_TEXMAP,CMZN_GRAPHICS_RENDER_POLYGON_MODE_SHADED);
			if (fill_surface_graphics_vertex_array(GT_object_get_vertex_set(glyph),
				g_TRIANGLE, number_of_segments_down+1,number_of_segments_around+1,
				points,normalpoints,/*tangentpoints*/(Triple *)NULL,
				 /*texturepoints*/(Triple *)NULL, g_NO_DATA, (GLfloat *)NULL))
			{
				GT_OBJECT_ADD(GT_surface_vertex_buffers)(glyph, surface);
			}
			else
			{
				DESTROY(GT_surface_vertex_buffers)(&surface);
				DEACCESS(GT_object)(&glyph);
			}
			DEALLOCATE(points);
			DEALLOCATE(normalpoints);
		}
		if (!glyph)
		{
			display_message(ERROR_MESSAGE,
				"create_GT_object_sphere.  Error creating glyph");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"create_GT_object_sphere.  Invalid argument(s)");
	}
	return (glyph);
}

GT_object *cmzn_glyph_circular::getGraphicsObject(cmzn_tessellation *tessellation, cmzn_material *, cmzn_font *)
{
	const int circleDivisions = cmzn_tessellation_get_circle_divisions(tessellation);
	const size_t size = this->objects.size();
	for (size_t i = 0; i < size; ++i)
	{
		if (objects[i].circleDivisions == circleDivisions)
		{
			return ACCESS(GT_object)(objects[i].graphicsObject);
		}
	}
	GT_object *graphicsObject = this->createGraphicsObject(circleDivisions);
	if (!graphicsObject)
		return 0;
	GraphicsObjectDivisions entry(circleDivisions, graphicsObject);
	// replace first unused graphics object if possible
	for (size_t i = 0; i < size; ++i)
	{
		if (objects[i].graphicsObject->access_count == 1)
		{
			objects[i] = entry;
			return graphicsObject;
		}
	}
	objects.push_back(entry);
	return graphicsObject;
}

GT_object *cmzn_glyph_arrow_solid::createGraphicsObject(int circleDivisions)
{
	return create_GT_object_arrow_solid("arrow_solid", /*primary_axis*/1, circleDivisions,
		/*shaft_length*/1.0 - this->headLength, /*shaft_radius*/this->shaftThickness*0.5, /*cone_radius*/0.5);
}

GT_object *cmzn_glyph_cone::createGraphicsObject(int circleDivisions)
{
	return create_GT_object_cone("cone", circleDivisions);
}

GT_object *cmzn_glyph_cone_solid::createGraphicsObject(int circleDivisions)
{
	return create_GT_object_cone_solid("cone_solid", circleDivisions);
}

GT_object *cmzn_glyph_cylinder::createGraphicsObject(int circleDivisions)
{
	return create_GT_object_cylinder("cylinder", circleDivisions);
}

GT_object *cmzn_glyph_cylinder_solid::createGraphicsObject(int circleDivisions)
{
	return create_GT_object_cylinder_solid("cylinder_solid", circleDivisions);
}

GT_object *cmzn_glyph_sphere::createGraphicsObject(int circleDivisions)
{
	return create_GT_object_sphere("sphere", circleDivisions, (circleDivisions + 1)/2);
}

/*
Global functions
----------------
*/
