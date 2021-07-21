/*******************************************************************************
FILE : finite_element_to_iso_lines.c

LAST MODIFIED : 30 July 2001

DESCRIPTION :
Functions for computing, sorting and storing polylines of constant scalar field
value over 2-D elements.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <math.h>

#include "opencmiss/zinc/status.h"
#include "computed_field/computed_field.h"
#include "computed_field/field_cache.hpp"
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_to_iso_lines.h"
#include "finite_element/finite_element_to_graphics_object.h"
#include "general/debug.h"
#include "general/object.h"
#include "graphics/auxiliary_graphics_types.h"
#include "graphics/graphics_object.h"
#include "graphics/graphics_object.hpp"
#include "general/message.h"

#define CONTOUR_POLYLINE_REALLOCATE_SIZE 25

/*
Module types
------------
*/

struct Contour_polyline
{
	int number_of_points;
	Triple *point_list;
	ZnReal *data;
};

struct Contour_lines
{
	int allocated_polylines,number_of_data_components,number_of_polylines;
	struct Contour_polyline *polylines;
};

struct Contour_lines *CREATE(Contour_lines)(int number_of_data_components)
/*******************************************************************************
LAST MODIFIED : 25 January 2000

DESCRIPTION :
==============================================================================*/
{
	struct Contour_lines *contour_lines;

	ENTER(CREATE(Contour_lines));
	if (0<=number_of_data_components)
	{
		if (ALLOCATE(contour_lines,struct Contour_lines,1))
		{
			contour_lines->allocated_polylines=0;
			contour_lines->number_of_data_components=number_of_data_components;
			contour_lines->number_of_polylines=0;
			contour_lines->polylines=(struct Contour_polyline *)NULL;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"CREATE(Contour_lines).  Not enough memory");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CREATE(Contour_lines).  Invalid argument(s)");
		contour_lines=(struct Contour_lines *)NULL;
	}
	LEAVE;

	return (contour_lines);
} /* CREATE(Contour_lines) */

int DESTROY(Contour_lines)(struct Contour_lines **contour_lines_address)
/*******************************************************************************
LAST MODIFIED : 25 January 2000

DESCRIPTION :
==============================================================================*/
{
	int i,return_code;
	struct Contour_lines *contour_lines;

	ENTER(DESTROY(Contour_lines));
	if (contour_lines_address&&(contour_lines= *contour_lines_address))
	{
		if (contour_lines->polylines)
		{
			for (i=0;i<contour_lines->number_of_polylines;i++)
			{
				DEALLOCATE(contour_lines->polylines[i].point_list);
				if (0<contour_lines->number_of_data_components)
				{
					DEALLOCATE(contour_lines->polylines[i].data);
				}
			}
			DEALLOCATE(contour_lines->polylines);
		}
		DEALLOCATE(*contour_lines_address);
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(Contour_lines).  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Contour_lines) */

int Contour_lines_add_segment(struct Contour_lines *contour_lines,
	Triple point1,ZnReal *data1,Triple point2,ZnReal *data2)
/*******************************************************************************
LAST MODIFIED : 25 January 2000

DESCRIPTION :
Adds a single line segment to the <contour_lines>, from <point1> to <point2>.
If the contour_lines has number_of_data_components > 0, then that number
of components are expected to be supplied in <data1> and <data2>.
==============================================================================*/
{
	int i,return_code;
	struct Contour_polyline *polyline,*temp_polylines;

	ENTER(Contour_lines_add_segment);
	if (contour_lines&&point1&&point2&&
		((0==contour_lines->number_of_data_components)||(data1&&data2)))
	{
		return_code=1;
		if ((contour_lines->number_of_polylines+1) >
			contour_lines->allocated_polylines)
		{
			if (REALLOCATE(temp_polylines,contour_lines->polylines,
				struct Contour_polyline,
				contour_lines->allocated_polylines+CONTOUR_POLYLINE_REALLOCATE_SIZE))
			{
				contour_lines->polylines=temp_polylines;
				contour_lines->allocated_polylines += CONTOUR_POLYLINE_REALLOCATE_SIZE;
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Contour_lines_add_segment.  Could not add segment");
				return_code=0;
			}
		}
		if (return_code)
		{
			polyline= &(contour_lines->polylines[contour_lines->number_of_polylines]);
			polyline->number_of_points=2;
			polyline->point_list=(Triple *)NULL;
			polyline->data=(ZnReal *)NULL;
			if (ALLOCATE(polyline->point_list,Triple,2)&&
				((0==contour_lines->number_of_data_components)||
					ALLOCATE(polyline->data,ZnReal,
						2*contour_lines->number_of_data_components)))
			{
				for (i=0;i<3;i++)
				{
					polyline->point_list[0][i]=point1[i];
					polyline->point_list[1][i]=point2[i];
				}
				for (i=0;i<(contour_lines->number_of_data_components);i++)
				{
					polyline->data[i]=data1[i];
					polyline->data[contour_lines->number_of_data_components+i]=data2[i];
				}
				(contour_lines->number_of_polylines)++;
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Contour_lines_add_segment.  Could not fill segment");
				return_code=0;
				if (polyline->point_list)
				{
					DEALLOCATE(polyline->point_list);
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Contour_lines_add_segment.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Contour_lines_add_segment */

int Contour_lines_add_to_vertex_array(struct Contour_lines *contour_lines,
	struct Graphics_vertex_array *array,int object_name)
/*******************************************************************************
LAST MODIFIED : 1 February 2000

DESCRIPTION :
Converts the polylines in <contour_lines> to GT_polylines and adds them to
<graphics_object> at <time>.
==============================================================================*/
{
	int i,return_code;
	struct Contour_polyline *polyline;

	ENTER(Contour_lines_add_to_graphics_object);
	if (contour_lines&&array)
	{
		return_code=1;
		unsigned int vertex_start = array->get_number_of_vertices(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION);
		GLfloat *dataField = NULL;
		if (0 < contour_lines->number_of_data_components)
			dataField = new GLfloat[contour_lines->number_of_data_components];
		for (i=0;(i<contour_lines->number_of_polylines)&&return_code;i++)
		{
			polyline= &(contour_lines->polylines[i]);
			unsigned int number_of_vertices = (unsigned int)polyline->number_of_points;

			array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
				3, polyline->number_of_points, polyline->point_list[0]);
			ZnReal *original_data = polyline->data;
			for (int j = 0; j < polyline->number_of_points; j++)
			{
				if (contour_lines->number_of_data_components > 0)
				{
					CAST_TO_OTHER(dataField, original_data, GLfloat, contour_lines->number_of_data_components);
					array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
						contour_lines->number_of_data_components, 1, dataField);
					original_data += contour_lines->number_of_data_components;
				}
			}
			array->add_fast_search_id(object_name);
			array->add_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_OBJECT_ID,
				1, 1, &(object_name));
			int modificationRequired = 0;
			array->add_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_UPDATE_REQUIRED,
				1, 1, &modificationRequired);
			array->add_unsigned_integer_attribute(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
				1, 1, &number_of_vertices);
			array->add_unsigned_integer_attribute(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
				1, 1, &vertex_start);
			vertex_start += number_of_vertices;
		}
		if (0 != contour_lines->number_of_data_components)
			delete[] dataField;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Contour_lines_add_to_graphics_object.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Contour_lines_add_to_graphics_object */

int Contour_lines_add_lines_in_square(struct Contour_lines *contour_lines,
	FE_value iso_value,
	Triple corner1,FE_value scalar1,FE_value *data1,
	Triple corner2,FE_value scalar2,FE_value *data2,
	Triple corner3,FE_value scalar3,FE_value *data3,
	Triple corner4,FE_value scalar4,FE_value *data4)
/*******************************************************************************
LAST MODIFIED : 1 February 2000

DESCRIPTION :
Converts the polylines in <contour_lines> to GT_polylines and adds them to
<graphics_object> at <time>.
==============================================================================*/
{
	FE_value a,b,c,d,r,r1,r2,q;
	int i,intersect_1_4,number_of_intersections,return_code;
	Triple *point,points[4];
	ZnReal *data,*datum;

	ENTER(Contour_lines_add_lines_in_square);
	if (contour_lines&&corner1&&corner2&&corner3&&corner4&&
		((0==contour_lines->number_of_data_components)||
			(data1&&data2&&data3&&data4)))
	{
		return_code=1;
		number_of_intersections=0;
		data=(ZnReal *)NULL;
		if ((0==contour_lines->number_of_data_components)||
			ALLOCATE(data,ZnReal,4*contour_lines->number_of_data_components))
		{
			datum=data;
			/* intersection on 1-2 line */
			if (((scalar1 > iso_value)&&(scalar2 <= iso_value))||
				((scalar2 > iso_value)&&(scalar1 <= iso_value)))
			{
				r = (iso_value - scalar1)/(scalar2-scalar1);
				point = &(points[number_of_intersections]);
				q = 1.0 - r;
				for (i=0;i<3;i++)
				{
					(*point)[i] = GLfloat(q)*corner1[i] + GLfloat(r)*corner2[i];
				}
				if (data)
				{
					for (i=0;i<contour_lines->number_of_data_components;i++)
					{
						*datum = ZnReal(q*data1[i] + r*data2[i]);
						datum++;
					}
				}
				number_of_intersections++;
			}

			/* intersection on 3-4 line */
			if (((scalar3 > iso_value)&&(scalar4 <= iso_value))||
				((scalar4 > iso_value)&&(scalar3 <= iso_value)))
			{
				r = (iso_value - scalar3)/(scalar4-scalar3);
				point = &(points[number_of_intersections]);
				q = 1.0 - r;
				for (i=0;i<3;i++)
				{
					(*point)[i] = GLfloat(q)*corner3[i] + GLfloat(r)*corner4[i];
				}
				if (data)
				{
					for (i=0;i<contour_lines->number_of_data_components;i++)
					{
						*datum = ZnReal(q*data3[i] + r*data4[i]);
						datum++;
					}
				}
				number_of_intersections++;
			}

			/* intersection on 1-3 line */
			if (((scalar1 > iso_value)&&(scalar3 <= iso_value))||
				((scalar3 > iso_value)&&(scalar1 <= iso_value)))
			{
				r = (iso_value - scalar1)/(scalar3-scalar1);
				point = &(points[number_of_intersections]);
				q = 1.0 - r;
				for (i=0;i<3;i++)
				{
					(*point)[i] = GLfloat(q)*corner1[i] + GLfloat(r)*corner3[i];
				}
				if (data)
				{
					for (i=0;i<contour_lines->number_of_data_components;i++)
					{
						*datum = ZnReal(q*data1[i] + r*data3[i]);
						datum++;
					}
				}
				number_of_intersections++;
			}

			/* intersection on 2-4 line */
			if (((scalar2 > iso_value)&&(scalar4 <= iso_value))||
				((scalar4 > iso_value)&&(scalar2 <= iso_value)))
			{
				r = (iso_value - scalar2)/(scalar4-scalar2);
				point = &(points[number_of_intersections]);
				q = 1.0 - r;
				for (i=0;i<3;i++)
				{
					(*point)[i] = GLfloat(q)*corner2[i] + GLfloat(r)*corner4[i];
				}
				if (data)
				{
					for (i=0;i<contour_lines->number_of_data_components;i++)
					{
						*datum = ZnReal(q*data2[i] + r*data4[i]);
						datum++;
					}
				}
				number_of_intersections++;
			}
			switch (number_of_intersections)
			{
				case 0:
				{
					/* nothing to do */
				} break;
				case 2:
				{
					/* 1 line */
					return_code=Contour_lines_add_segment(contour_lines,points[0],data,
						points[1],data+contour_lines->number_of_data_components);
				} break;
				case 4:
				{
					/* 2 lines: tricky */
					/* Are there any intersections on the 1-4 line? Use fact that diagonal
						 line can be described by a quadratic */
					a = -scalar2 - scalar3;
					b = scalar4 - scalar1 + scalar2 + scalar3;
					c = scalar1 - iso_value;
					intersect_1_4 = 0;
					if (0.0 != a)
					{
						d = b*b - 4*a*c;
						if (0.0 <= d)
						{
							d = sqrt(d);
							r1 = (-b - d) / 2.0*a;
							r2 = (-b + d) / 2.0*a;
							if (((0.0 <= r1)&&(1.0 >= r1)) || ((0.0 <= r2)&&(1.0 >= r2)))
							{
								intersect_1_4 = 1;
							}
						}
					}
					else
					{
						/* linear; determine intersect */
						if (0.0 != b)
						{
							r1 = -c/b;
							if ((0.0 <= r1)&&(1.0 >= r1))
							{
								intersect_1_4 = 1;
							}
						}
					}
					if (intersect_1_4)
					{
						return_code=
							Contour_lines_add_segment(contour_lines,
								points[0],data,
								points[2],data+2*contour_lines->number_of_data_components)&&
							Contour_lines_add_segment(contour_lines,
								points[1],data+1*contour_lines->number_of_data_components,
								points[3],data+3*contour_lines->number_of_data_components);
					}
					else
					{
						return_code=
							Contour_lines_add_segment(contour_lines,
								points[0],data,
								points[3],data+3*contour_lines->number_of_data_components)&&
							Contour_lines_add_segment(contour_lines,
								points[1],data+1*contour_lines->number_of_data_components,
								points[2],data+2*contour_lines->number_of_data_components);
					}
				} break;
				default:
				{
					display_message(ERROR_MESSAGE,
						"Contour_lines_add_lines_in_square.  Invalid intersections");
					return_code=0;
				} break;
			}
			if (data)
			{
				DEALLOCATE(data);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Contour_lines_add_lines_in_square.  Not enough memory");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Contour_lines_add_lines_in_square.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Contour_lines_add_lines_in_square */

int Contour_lines_add_lines_in_triangle(struct Contour_lines *contour_lines,
	FE_value iso_value,
	Triple corner1,FE_value scalar1,FE_value *data1,
	Triple corner2,FE_value scalar2,FE_value *data2,
	Triple corner3,FE_value scalar3,FE_value *data3)
/*******************************************************************************
LAST MODIFIED : 1 February 2000

DESCRIPTION :
Converts the polylines in <contour_lines> to GT_polylines and adds them to
<graphics_object> at <time>.
==============================================================================*/
{
	FE_value r,q;
	int i,number_of_intersections,return_code;
	Triple *point,points[3];
	ZnReal *data,*datum;

	ENTER(Contour_lines_add_lines_in_triangle);
	if (contour_lines&&corner1&&corner2&&corner3&&
		((0==contour_lines->number_of_data_components)||
			(data1&&data2&&data3)))
	{
		return_code=1;
		number_of_intersections=0;
		data=(ZnReal *)NULL;
		if ((0==contour_lines->number_of_data_components)||
			ALLOCATE(data,ZnReal,3*contour_lines->number_of_data_components))
		{
			datum=data;
			/* intersection on 1-2 line */
			if (((scalar1 > iso_value)&&(scalar2 <= iso_value))||
				((scalar2 > iso_value)&&(scalar1 <= iso_value)))
			{
				r = (iso_value - scalar1)/(scalar2-scalar1);
				point = &(points[number_of_intersections]);
				q = 1.0 - r;
				for (i=0;i<3;i++)
				{
					(*point)[i] = GLfloat(q)*corner1[i] + GLfloat(r)*corner2[i];
				}
				if (data)
				{
					for (i=0;i<contour_lines->number_of_data_components;i++)
					{
						*datum = ZnReal(q*data1[i] + r*data2[i]);
						datum++;
					}
				}
				number_of_intersections++;
			}

			/* intersection on 2-3 line */
			if (((scalar2 > iso_value)&&(scalar3 <= iso_value))||
				((scalar3 > iso_value)&&(scalar2 <= iso_value)))
			{
				r = (iso_value - scalar2)/(scalar3-scalar2);
				point = &(points[number_of_intersections]);
				q = 1.0 - r;
				for (i=0;i<3;i++)
				{
					(*point)[i] = GLfloat(q)*corner2[i] + GLfloat(r)*corner3[i];
				}
				if (data)
				{
					for (i=0;i<contour_lines->number_of_data_components;i++)
					{
						*datum = ZnReal(q*data2[i] + r*data3[i]);
						datum++;
					}
				}
				number_of_intersections++;
			}

			/* intersection on 1-3 line */
			if (((scalar1 > iso_value)&&(scalar3 <= iso_value))||
				((scalar3 > iso_value)&&(scalar1 <= iso_value)))
			{
				r = (iso_value - scalar1)/(scalar3-scalar1);
				point = &(points[number_of_intersections]);
				q = 1.0 - r;
				for (i=0;i<3;i++)
				{
					(*point)[i] = GLfloat(q)*corner1[i] + GLfloat(r)*corner3[i];
				}
				if (data)
				{
					for (i=0;i<contour_lines->number_of_data_components;i++)
					{
						*datum = ZnReal(q*data1[i] + r*data3[i]);
						datum++;
					}
				}
				number_of_intersections++;
			}

			if (2 == number_of_intersections)
			{
				return_code=Contour_lines_add_segment(contour_lines,points[0],data,
					points[1],data+contour_lines->number_of_data_components);
			}
			else if (0 != number_of_intersections)
			{
				display_message(ERROR_MESSAGE,
					"Contour_lines_add_lines_in_triangle.  Invalid intersections");
				return_code=0;
			}
			if (data)
			{
				DEALLOCATE(data);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Contour_lines_add_lines_in_triangle.  Not enough memory");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Contour_lines_add_lines_in_triangle.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Contour_lines_add_lines_in_triangle */

int points_and_data_match(Triple point1,Triple point2,
	int number_of_data_components,ZnReal *data1,ZnReal *data2)
/*******************************************************************************
LAST MODIFIED : 1 February 2000

DESCRIPTION :
Links together polylines in the <contour_lines> with identical end points and
data values for more efficient storage and smoother rendering.
==============================================================================*/
{
	int i,return_code;

	ENTER(points_and_data_match);
	return_code=0;
	if (point1&&point2&&((0==number_of_data_components)||(data1&&data2)))
	{
		if ((point1[0]==point2[0])&&(point1[1]==point2[1])&&
			(point1[2]==point2[2]))
		{
			return_code=1;
			for (i=0;return_code&&(i<number_of_data_components);i++)
			{
				if (data1[i] != data2[i])
				{
					return_code=0;
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"points_and_data_match.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* points_and_data_match */

int Contour_lines_link_ends(struct Contour_lines *contour_lines)
/*******************************************************************************
LAST MODIFIED : 1 February 2000

DESCRIPTION :
Links together polylines in the <contour_lines> with identical end points and
data values for more efficient storage and smoother rendering.
==============================================================================*/
{
	int i,j,k,links_made,m,match11,match21,match22,
		number_of_data_components,return_code;
	ZnReal *data,*data_i1,*data_i2,*data_j1,*data_j2;
	struct Contour_polyline *polyline_i,*polyline_j;
	Triple *point_i1,*point_i2,*point_j1,*point_j2,*point_list;

	ENTER(Contour_lines_link_ends);
	if (contour_lines)
	{
		return_code=1;
		number_of_data_components=contour_lines->number_of_data_components;
		for (i=0;(i<contour_lines->number_of_polylines)&&return_code;i++)
		{
			polyline_i=&(contour_lines->polylines[i]);
			if (0 < polyline_i->number_of_points)
			{
				links_made=1;
				while (links_made)
				{
					point_i1=polyline_i->point_list;
					data_i1=polyline_i->data;
					point_i2=polyline_i->point_list+polyline_i->number_of_points-1;
					data_i2=polyline_i->data+contour_lines->number_of_data_components*
						(polyline_i->number_of_points-1);
					links_made=0;
					for (j=i+1;(j<contour_lines->number_of_polylines)&&(!links_made)&&
						return_code;j++)
					{
						polyline_j=&(contour_lines->polylines[j]);
						if (0 < polyline_j->number_of_points)
						{
							point_j1=polyline_j->point_list;
							point_j2=polyline_j->point_list+(polyline_j->number_of_points-1);
							data_j1=polyline_j->data;
							data_j2=polyline_j->data+contour_lines->number_of_data_components*
								(polyline_j->number_of_points-1);
							match11=0;
							match21=0;
							match22=0;
							if ((match11=points_and_data_match(*point_i1,*point_j1,
								number_of_data_components,data_i1,data_j1))||
								points_and_data_match(*point_i1,*point_j2,
									number_of_data_components,data_i1,data_j2)||
								(match21=points_and_data_match(*point_i2,*point_j1,
									number_of_data_components,data_i2,data_j1))||
								(match22=points_and_data_match(*point_i2,*point_j2,
									number_of_data_components,data_i2,data_j2)))
							{
								point_list=(Triple *)NULL;
								data=(ZnReal *)NULL;
								if (ALLOCATE(point_list,Triple,
									polyline_i->number_of_points+polyline_j->number_of_points-1)&&
									((0==number_of_data_components)||
										ALLOCATE(data,ZnReal,(polyline_i->number_of_points+
											polyline_j->number_of_points-1)*
											number_of_data_components)))
								{
									/* copy points and data from polyline_i in correct order */
									point_i1=point_list;
									data_i1=data;
									if (match21||match22)
									{
										point_i2=polyline_i->point_list;
										data_i2=polyline_i->data;
									}
									else
									{
										point_i2=polyline_i->point_list+
											polyline_i->number_of_points-1;
										data_i2=polyline_i->data+(polyline_i->number_of_points-1)*
											number_of_data_components;
									}
									for (k=0;k<polyline_i->number_of_points;k++)
									{
										for (m=0;m<3;m++)
										{
											(*point_i1)[m]=(*point_i2)[m];
										}
										for (m=0;m<number_of_data_components;m++)
										{
											data_i1[m] = data_i2[m];
										}
										point_i1++;
										data_i1 += number_of_data_components;
										if (match21||match22)
										{
											point_i2++;
											data_i2 += number_of_data_components;
										}
										else
										{
											point_i2--;
											data_i2 -= number_of_data_components;
										}
									}
									/* copy points and data from polyline_j in correct order */
									if (match11||match21)
									{
										point_i2=polyline_j->point_list+1;
										data_i2=polyline_j->data+number_of_data_components;
									}
									else
									{
										point_i2=polyline_j->point_list+
											polyline_j->number_of_points-2;
										data_i2=polyline_j->data+(polyline_j->number_of_points-2)*
											number_of_data_components;
									}
									for (k=1;k<polyline_j->number_of_points;k++)
									{
										for (m=0;m<3;m++)
										{
											(*point_i1)[m]=(*point_i2)[m];
										}
										for (m=0;m<number_of_data_components;m++)
										{
											data_i1[m] = data_i2[m];
										}
										point_i1++;
										data_i1 += number_of_data_components;
										if (match11||match21)
										{
											point_i2++;
											data_i2 += number_of_data_components;
										}
										else
										{
											point_i2--;
											data_i2 -= number_of_data_components;
										}
									}
									/* make polyline_i into combined polyline */
									DEALLOCATE(polyline_i->point_list);
									if (polyline_i->data)
									{
										DEALLOCATE(polyline_i->data);
									}
									polyline_i->point_list=point_list;
									polyline_i->data=data;
									polyline_i->number_of_points +=
										(polyline_j->number_of_points-1);
									/* copy last polyline into polyline_j */
									DEALLOCATE(polyline_j->point_list);
									if (polyline_j->data)
									{
										DEALLOCATE(polyline_j->data);
									}
									if (j<(contour_lines->number_of_polylines-1))
									{
										polyline_j->point_list=contour_lines->polylines
											[contour_lines->number_of_polylines-1].point_list;
										polyline_j->data=contour_lines->polylines
											[contour_lines->number_of_polylines-1].data;
									}
									contour_lines->number_of_polylines--;
									links_made++;
								}
								else
								{
									display_message(ERROR_MESSAGE,
										"Contour_lines_link_ends.  Not enough memory)");
									if (point_list)
									{
										DEALLOCATE(point_list);
									}
									return_code=0;
								}
							}
						}
					}
				} /* while (links_made) */
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Contour_lines_link_ends.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Contour_lines_link_ends */


int create_iso_lines_from_FE_element(struct FE_element *element,
	cmzn_fieldcache_id field_cache, struct Computed_field *coordinate_field,
	struct Computed_field *isoscalar_field, FE_value iso_value,
	struct Computed_field *data_field,int number_of_segments_in_xi1_requested,
	int number_of_segments_in_xi2_requested,struct FE_element *top_level_element,
	struct Graphics_vertex_array *array)
{
	enum Collapsed_element_type collapsed_element;
	enum FE_element_shape_type shape_type1;
	FE_value coordinates[3],*data,*datum,distance1,distance2,*scalar,*scalars,
		xi[2];
	gtPolygonType polygon_type;
	int adjusted_number_of_points_in_xi2,n_data_components,i,j,number_of_points,
		number_of_points_in_xi1,number_of_points_in_xi2,number_of_polygon_vertices,
		simplex_element,return_code;
	struct Contour_lines *contour_lines;
	Triple *point,*points;

	if (element && field_cache && (2==get_FE_element_dimension(element))&&
		(0<number_of_segments_in_xi1_requested)&&
		(0<number_of_segments_in_xi2_requested)&&coordinate_field&&
		(3>=cmzn_field_get_number_of_components(coordinate_field))&&
		isoscalar_field&&(1==cmzn_field_get_number_of_components(isoscalar_field))&&
		array)
	{
		if (data_field)
		{
			n_data_components=cmzn_field_get_number_of_components(data_field);
		}
		else
		{
			n_data_components=0;
		}
		/* clear coordinates and derivatives not set if coordinate field is not
			 3 component */
		coordinates[1]=0.0;
		coordinates[2]=0.0;
		return_code = get_surface_element_segmentation(element,
			number_of_segments_in_xi1_requested,number_of_segments_in_xi2_requested,
			&number_of_points_in_xi1,&number_of_points_in_xi2,
			&number_of_points,&number_of_polygon_vertices,&polygon_type,
			&collapsed_element,&shape_type1);
		simplex_element=(SIMPLEX_SHAPE== shape_type1);
		points=(Triple *)NULL;
		scalars=(FE_value *)NULL;
		data=(FE_value *)NULL;
		distance1=(FE_value)(number_of_points_in_xi1-1);
		distance2=(FE_value)(number_of_points_in_xi2-1);
		contour_lines=(struct Contour_lines *)NULL;
		unsigned int point_index = 0;
		if (ALLOCATE(points,Triple,number_of_points)&&
			ALLOCATE(scalars,FE_value,number_of_points)&&((!data_field)||
				ALLOCATE(data,FE_value,n_data_components*number_of_points))&&
			(contour_lines=CREATE(Contour_lines)(n_data_components)))
		{
			point=points;
			scalar=scalars;
			datum=data;
			for (i=0;(i<number_of_points_in_xi1)&&return_code;i++)
			{
				xi[0]=(FE_value)i / distance1;
				if (simplex_element)
				{
					adjusted_number_of_points_in_xi2=number_of_points_in_xi2-i;
				}
				else
				{
					adjusted_number_of_points_in_xi2=number_of_points_in_xi2;
				}
				for (j=0;(j<adjusted_number_of_points_in_xi2)&&return_code;j++)
				{
					xi[1]=(FE_value)j / distance2;
					if ((CMZN_OK == field_cache->setIndexedMeshLocation(point_index, element, xi, top_level_element)) &&
						(CMZN_OK == cmzn_field_evaluate_real(coordinate_field, field_cache, 3, coordinates)) &&
						(CMZN_OK == cmzn_field_evaluate_real(isoscalar_field, field_cache, 1, scalar)) &&
						((!data_field) || (CMZN_OK == cmzn_field_evaluate_real(data_field, field_cache, n_data_components, datum))))
					{
						(*point)[0]=GLfloat(coordinates[0]);
						(*point)[1]=GLfloat(coordinates[1]);
						(*point)[2]=GLfloat(coordinates[2]);
						point++;
						scalar++;
						datum += n_data_components;
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"create_iso_lines_from_FE_element.  Error calculating fields");
						return_code=0;
					}
					++point_index;
				}
			}
			/* perform contouring on the squares joining the points */
			point=points;
			scalar=scalars;
			datum=data;
			for (i=0;(i<(number_of_points_in_xi1-1))&&return_code;i++)
			{
				if (simplex_element)
				{
					adjusted_number_of_points_in_xi2=number_of_points_in_xi2-i;
				}
				else
				{
					adjusted_number_of_points_in_xi2=number_of_points_in_xi2;
				}
				for (j=0;(j<(adjusted_number_of_points_in_xi2-1))&&return_code;j++)
				{
					if (simplex_element&&(j==(adjusted_number_of_points_in_xi2-2)))
					{
						return_code=Contour_lines_add_lines_in_triangle(contour_lines,
							iso_value,
							*point,*scalar,datum,
							*(point+adjusted_number_of_points_in_xi2),
							*(scalar+adjusted_number_of_points_in_xi2),
							datum+adjusted_number_of_points_in_xi2*n_data_components,
							*(point+1),*(scalar+1),datum+n_data_components);
					}
					else
					{
						return_code=Contour_lines_add_lines_in_square(contour_lines,
							iso_value,
							*point,*scalar,datum,
							*(point+adjusted_number_of_points_in_xi2),
							*(scalar+adjusted_number_of_points_in_xi2),
							datum+adjusted_number_of_points_in_xi2*n_data_components,
							*(point+1),*(scalar+1),datum+n_data_components,
							*(point+adjusted_number_of_points_in_xi2+1),
							*(scalar+adjusted_number_of_points_in_xi2+1),
							datum+(adjusted_number_of_points_in_xi2+1)*n_data_components);
					}
					if (!return_code)
					{
						display_message(ERROR_MESSAGE,
							"create_iso_lines_from_FE_element.  Error getting contours");
					}
					point++;
					scalar++;
					datum += n_data_components;
				}
				point++;
				scalar++;
				datum += n_data_components;
			}
			if (return_code)
			{
				Contour_lines_link_ends(contour_lines);
				if (!Contour_lines_add_to_vertex_array(contour_lines,
						array, get_FE_element_index(element)))
				{
					display_message(ERROR_MESSAGE,"create_iso_lines_from_FE_element.  "
						"Could not add lines to graphics object");
					return_code=0;
				}
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"create_iso_lines_from_FE_element.  Not enough memory");
			return_code=0;
		}
		if (points)
		{
			DEALLOCATE(points);
		}
		if (data)
		{
			DEALLOCATE(data);
		}
		if (scalars)
		{
			DEALLOCATE(scalars);
		}
		if (contour_lines)
		{
			DESTROY(Contour_lines)(&contour_lines);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"create_iso_lines_from_FE_element.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
}
