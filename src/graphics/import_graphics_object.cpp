/*******************************************************************************
FILE : import_graphics_object.c

LAST MODIFIED : 19 March 2003

DESCRIPTION :
Functions for reading graphics object data from a file.
???DB.  7 June 1994.  Merged GTTEXT into GTPOINT and GTPOINTSET and added a
	marker type and a marker size
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "opencmiss/zinc/material.h"
#include "general/debug.h"
#include "general/io_stream.h"
#include "general/matrix_vector.h"
#include "general/mystring.h"
#include "graphics/glyph.hpp"
#include "graphics/material.h"
#include "graphics/graphics_library.h"
#include "graphics/graphics_object.h"
#include "graphics/graphics_object.hpp"
#include "graphics/import_graphics_object.h"
#include "graphics/volume_texture.h"
#include "general/message.h"

/*
Module functions
----------------
*/

/*
Global functions
----------------
*/

static int file_read_GT_object_type(struct IO_stream *file,
	enum GT_object_type *object_type)
/*******************************************************************************
LAST MODIFIED : 13 August 1998

DESCRIPTION :
Reads an object type, as a string, from a <file> and translates it into the
enumerated <object_type>.
==============================================================================*/
{
	char *type_string;
	int return_code;

	if (file&&object_type)
	{
		if (IO_stream_read_string(file,"s",&type_string))
		{
			*object_type = g_OBJECT_TYPE_INVALID;
			return_code = 1;
			if (0 == (strcmp("SURFACE", type_string)))
			{
				*object_type = g_SURFACE_VERTEX_BUFFERS;
			}
			else if (0 == (strcmp("POLYLINE", type_string)))
			{
				*object_type = g_POLYLINE_VERTEX_BUFFERS;
			}
			else if (0 == (strcmp("POINT", type_string)))
			{
				*object_type = g_POINT_VERTEX_BUFFERS;
			}
			else if (0 == (strcmp("POINTSET", type_string)))
			{
				*object_type = g_POINT_SET_VERTEX_BUFFERS;
			}
			else
			{
				return_code = get_GT_object_type_from_string(type_string,object_type);
			}
		}
		else
		{
			return_code=0;
			display_message(ERROR_MESSAGE,
				"file_read_GT_object_type.  Error reading object type string");
		}
	}
	else
	{
		return_code=0;
		display_message(ERROR_MESSAGE,
			"file_read_GT_object_type.  Invalid argument(s)");
	}

	return (return_code);
} /* file_read_GT_object_type */

static int file_read_GT_polyline_type(struct IO_stream *file,
	enum GT_polyline_type *polyline_type)
/*******************************************************************************
LAST MODIFIED : 13 August 1998

DESCRIPTION :
Reads an polyline type, as a string, from a <file> and translates it into the
enumerated <polyline_type>.
==============================================================================*/
{
	char *type_string;
	int return_code;

	ENTER(file_read_GT_polyline_type);
	if (file&&polyline_type)
	{
		if (IO_stream_read_string(file,"s",&type_string))
		{
			return_code=get_GT_polyline_type_from_string(type_string,polyline_type);
			DEALLOCATE(type_string);
		}
		else
		{
			return_code=0;
			display_message(ERROR_MESSAGE,
				"file_read_GT_polyline_type.  Error reading polyline type string");
		}
	}
	else
	{
		return_code=0;
		display_message(ERROR_MESSAGE,
			"file_read_GT_polyline_type.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* file_read_GT_polyline_type */

static int file_read_GT_surface_type(struct IO_stream *file,
	enum GT_surface_type *surface_type)
/*******************************************************************************
LAST MODIFIED : 13 August 1998

DESCRIPTION :
Reads an surface type, as a string, from a <file> and translates it into the
enumerated <surface_type>.
==============================================================================*/
{
	char *type_string;
	int return_code;

	ENTER(file_read_GT_surface_type);
	if (file&&surface_type)
	{
		if (IO_stream_read_string(file,"s",&type_string))
		{
			return_code=get_GT_surface_type_from_string(type_string,surface_type);
			DEALLOCATE(type_string);
		}
		else
		{
			return_code=0;
			display_message(ERROR_MESSAGE,
				"file_read_GT_surface_type.  Error reading surface type string");
		}
	}
	else
	{
		return_code=0;
		display_message(ERROR_MESSAGE,
			"file_read_GT_surface_type.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* file_read_GT_surface_type */

/*
Global functions
----------------
*/

int file_read_graphics_objects(char *file_name,
	struct IO_stream_package *io_stream_package,
	struct cmzn_materialmodule *materialmodule,
	struct cmzn_glyphmodule *glyphmodule)
/*******************************************************************************
LAST MODIFIED : 19 March 2003

DESCRIPTION :
???RC.  26 June 97.  Added nurbs_type for NURBS. NURBSMORPH are NURBS with
	nurbs_type=1
==============================================================================*/
{
	int return_code;
	gtObject *obj;
	char objname[100];
	int dummy, i,j,k;
	int n_data_components, npts1,npts2;
	Triple *pointlist,*normallist,*texturelist;
	enum GT_surface_type surface_type;
	enum GT_polyline_type polyline_type;
	GLfloat *data;
	enum GT_object_type object_type;
	gtMatrix transform;
	gtTransformType transtype;
	cmzn_material *object_material;
	struct IO_stream *stream;
	int version;
	double time;

	cmzn_glyphmodule_begin_change(glyphmodule);
#if defined (DEBUG_CODE)
	/*???debug*/
	printf("ENTER(file_read_graphics_objects)\n");
#endif /* defined (DEBUG_CODE) */
	if (file_name)
	{
		/* files without a header default to 1 */
		version=1;
		if ((stream = CREATE(IO_stream)(io_stream_package))
			&& (IO_stream_open_for_read(stream, file_name)))
		{
			return_code=1;
			while (return_code&&(IO_stream_scan(stream,"%s",objname)!=EOF)&&
				(strcmp(objname,"END_OF_FILE")))
			{
#if defined (DEBUG_CODE)
				/*???debug*/
				printf("\nReading object: %s\n",objname);
				display_message(INFORMATION_MESSAGE,"Reading object: %s\n",objname);
#endif /* defined (DEBUG_CODE) */
				if (fuzzy_string_compare(objname,"CMISS"))
				{
					/* version information */
					if (EOF==(IO_stream_scan(stream,"%s",objname))||
						(!fuzzy_string_compare(objname,"exobj")))
					{
						display_message(WARNING_MESSAGE,
							"file_read_graphics_objects.  Header does not have exobj "
							"keyword\nValid header is 'CMISS exobj File Version 3'\n");
					}
					if ((EOF==IO_stream_scan(stream,"%s",objname))||
						(!fuzzy_string_compare(objname,"file")))
					{
						display_message(WARNING_MESSAGE,
							"file_read_graphics_objects.  Header does not have file keyword\n"
							"Valid header is 'CMISS exobj File Version 3'\n");
					}
					if ((EOF==IO_stream_scan(stream,"%s",objname))||
						(!fuzzy_string_compare(objname,"version")))
					{
						display_message(WARNING_MESSAGE,
							"file_read_graphics_objects.  Header does not have version "
							"keyword\nValid header is 'CMISS exobj File Version 2'\n");
					}
					if (EOF==IO_stream_scan(stream,"%d",&version))
					{
						display_message(ERROR_MESSAGE,
							"file_read_graphics_objects. Unable to read version");
						return_code=0;
					}
					IO_stream_scan(stream,"%s",objname);
				}
				if (return_code)
				{
					/* read the object type */
					if (file_read_GT_object_type(stream,&object_type))
					{
						if(version < 3)
						{
							IO_stream_scan(stream,"%d",&dummy);
							IO_stream_scan(stream,"%d",&dummy);
							time = 0;
							display_message(WARNING_MESSAGE,
								"file_read_graphics_objects.  Activity type and default attribute are obsolete, values ignored\n");
						}
						else
						{
							IO_stream_scan(stream,"%lf", &time);
						}
						file_read_Graphical_material_name(stream,&object_material,
							cmzn_materialmodule_get_manager(materialmodule));
						if (version<2)
						{
							transtype=g_ID;
							for (i=0;i<4;i++)
							{
								for(j=0;j<4;j++)
								{
									IO_stream_scan(stream,"%f",&(transform[i][j]));
									if (((i==j) && (transform[i][j] != 1)) ||
										((i != j) && (transform[i][j] != 0)))
									{
										transtype = g_NOT_ID;
									}
								}
							}
							if(transtype == g_NOT_ID)
							{
								display_message(WARNING_MESSAGE,
									"file_read_graphics_objects.  Transformation in .exobj file "
									"is now obsolete\nIt will be ignored!\n"
									"Instead add the following command to your comfile after the"
									"object has been drawn.\n"
									"gfx set transformation name %s %f %f %f %f %f %f %f %f %f "
									"%f %f %f %f %f %f %f\n",
									objname,transform[0][0],transform[0][1],transform[0][2],
									transform[0][3],transform[1][0],transform[1][1],
									transform[1][2],transform[1][3],transform[2][0],
									transform[2][1],transform[2][2],transform[2][3],
									transform[3][0],transform[3][1],transform[3][2],
									transform[3][3]);
							}
						}
						cmzn_glyph_id glyph = cmzn_glyphmodule_find_glyph_by_name(glyphmodule, objname);
						obj = 0;
						cmzn_glyph_static *glyphStatic = dynamic_cast<cmzn_glyph_static*>(glyph);
						if (glyphStatic)
						{
							obj = glyphStatic->getGraphicsObject();
						}
						else if (glyph)
						{
							display_message(ERROR_MESSAGE, "Read graphics object file: Cannot modify non-static glyph '%s'", objname);
							cmzn_glyph_destroy(&glyph);
							return_code = 0;
						}
						if (obj)
						{
							// Note: time is now ignored
							return_code = GT_object_clear_primitives(obj);
						}
						else if (return_code)
						{
							if (object_type == g_POINT_VERTEX_BUFFERS)
							{
								obj=CREATE(GT_object)(objname,g_POINT_SET_VERTEX_BUFFERS,object_material);
							}
							else
							{
								obj=CREATE(GT_object)(objname,object_type,object_material);
							}
							glyph = cmzn_glyphmodule_create_glyph_static(glyphmodule, obj);
							if (glyph)
							{
								glyph->setName(objname);
								glyph->setManaged(true);
							}
							else
							{
								return_code = 0;
							}
						}
						if (return_code)
						{
						switch (object_type)
						{
							case g_POINT_VERTEX_BUFFERS:
							{
								/*???DB.  Check allocation */
								ALLOCATE(pointlist,Triple,1);
								for (i=0;i<3;i++)
								{
									IO_stream_scan(stream,"%f",&((*pointlist)[i]));
								}
								/*???DB.  Merging GTTEXT into GTPOINT and GTPOINTSET */
								GT_pointset_vertex_buffers *pointset = CREATE(GT_pointset_vertex_buffers)(0, g_PLUS_MARKER, 1.0);
								fill_pointset_graphics_vertex_array(GT_object_get_vertex_set(obj),
									1, pointlist, 0, /*n_data_components*/0,(GLfloat *)NULL);
								GT_OBJECT_ADD(GT_pointset_vertex_buffers)(obj,pointset);
								DEALLOCATE(pointlist);
							} break;
							case g_POINT_SET_VERTEX_BUFFERS:
							{
								IO_stream_scan(stream,"%d",&npts1);
								/*???DB.  Check allocation */
								ALLOCATE(pointlist,Triple,npts1);
								for (j=0;j<npts1;j++)
								{
									for (i=0;i<3;i++)
									{
										IO_stream_scan(stream,"%f",&(pointlist[j][i]));
									}
								}
								/*???DB.  Merging GTTEXT into GTPOINT and GTPOINTSET */
								GT_pointset_vertex_buffers *pointset = CREATE(GT_pointset_vertex_buffers)(0, g_PLUS_MARKER, 1.0);
								fill_pointset_graphics_vertex_array(GT_object_get_vertex_set(obj),
									npts1, pointlist, 0, /*n_data_components*/0,(GLfloat *)NULL);
								GT_OBJECT_ADD(GT_pointset_vertex_buffers)(obj, pointset);
								DEALLOCATE(pointlist);
							} break;
							case g_POLYLINE_VERTEX_BUFFERS:
							{
								/*IO_stream_scan(stream,"%d",&polyline_type);*/
								if (file_read_GT_polyline_type(stream,&polyline_type))
								{
									IO_stream_scan(stream,"%d",&npts1);
									/* must clear the following since passed directly to
										 CREATE(GT_surface) unless specifically allocated */
									pointlist=(Triple *)NULL;
									normallist = (Triple *)NULL;
									data = 0;
									unsigned int number_of_points = 0;
									switch (polyline_type)
									{
										case g_PLAIN:
										{
											ALLOCATE(pointlist,Triple,npts1);
											for (i=0;i<npts1;i++)
											{
												for (j=0;j<3;j++)
												{
													IO_stream_scan(stream,"%f",&(pointlist[i][j]));
												}
											}
											number_of_points = npts1;
										} break;
										case g_PLAIN_DISCONTINUOUS:
										{
											ALLOCATE(pointlist,Triple,2*npts1);
											for (i=0;i<2*npts1;i++)
											{
												for (j=0;j<3;j++)
												{
													IO_stream_scan(stream,"%f",&(pointlist[i][j]));
												}
											}
											number_of_points = 2*npts1;
										} break;
										case g_NORMAL_DISCONTINUOUS:
										{
											ALLOCATE(pointlist,Triple,2*npts1);
											ALLOCATE(normallist,Triple,2*npts1);
											for (i=0;i<2*npts1;i++)
											{
												for (j=0;j<3;j++)
												{
													IO_stream_scan(stream,"%f",&(pointlist[i][j]));
												}
												for (j=0;j<3;j++)
												{
													IO_stream_scan(stream,"%f",&(normallist[i][j]));
												}
											}
											number_of_points = 2*npts1;
										} break;
										case g_NORMAL:
										{
											ALLOCATE(pointlist,Triple,npts1);
											ALLOCATE(normallist,Triple,npts1);
											for (i=0;i<npts1;i++)
											{
												for (j=0;j<3;j++)
												{
													IO_stream_scan(stream,"%f",&(pointlist[i][j]));
												}
												for (j=0;j<3;j++)
												{
													IO_stream_scan(stream,"%f",&(normallist[i][j]));
												}
											}
											number_of_points = npts1;
										} break;
										default:
										{
										} break;
									}
									GT_polyline_vertex_buffers *polyline = CREATE(GT_polyline_vertex_buffers)(polyline_type, 1);
									fill_line_graphics_vertex_array(GT_object_get_vertex_set(obj),
										number_of_points, pointlist, normallist,
										/*n_data_components*/0,(GLfloat *)NULL);
									GT_OBJECT_ADD(GT_polyline_vertex_buffers)(obj, polyline);
									if (pointlist)
									{
										DEALLOCATE(pointlist);
									}
									if (normallist)
									{
										DEALLOCATE(normallist);
									}
								}
								else
								{
									display_message(ERROR_MESSAGE,
										"file_read_graphics_objects.  Unknown polyline type");
									return_code=0;
								}
							} break;
							case g_SURFACE_VERTEX_BUFFERS:
							{
								/*IO_stream_scan(stream,"%d",&surface_type);*/
								if (file_read_GT_surface_type(stream,&surface_type))
								{
									IO_stream_scan(stream,"%d",&n_data_components);
									IO_stream_scan(stream,"%d",&npts1);
									IO_stream_scan(stream,"%d",&npts2);
									/* must clear the following since passed directly to
										 CREATE(GT_surface) unless specifically allocated */
									pointlist=(Triple *)NULL;
									normallist=(Triple *)NULL;
									texturelist=(Triple *)NULL;
									data=0;
									/* Note: A Discontinuous surface is set up differently from
										the more economical continuous surface in which points and
										normals are shared between adjacent polys in an array
										[npts1][npts2][2]. The discontinuous surface is made from
										discrete polygons in an array [npolys][npoints/poly][2]. */
									switch (surface_type)
									{
										case g_SH_DISCONTINUOUS:
										case g_SH_DISCONTINUOUS_STRIP:
										{
#if defined (DEBUG_CODE)
											/*???debug */
											printf("g_SH_DISCONTINUOUS\n");
#endif /* defined (DEBUG_CODE) */
											ALLOCATE(pointlist,Triple,npts1*npts2);
											ALLOCATE(normallist,Triple,npts1*npts2);
											for (i=0;i<npts1;i++)
											{
												for (j=0;j<npts2;j++)
												{
													for (k=0;k<3;k++)
													{
														IO_stream_scan(stream,"%f",&(pointlist[j+npts2 * i][k]));
													}
													for (k=0;k<3;k++)
													{
														IO_stream_scan(stream,"%f",&(normallist[j+npts2*i][k]));
													}
												}
											}
											if (n_data_components)
											{
												ALLOCATE(data,GLfloat,n_data_components*npts1*npts2);
												for (i=0;i<npts1;i++)
												{
													for (j=0;j<npts2;j++)
													{
														for (k=0;k<n_data_components;k++)
														{
															IO_stream_scan(stream,"%f",&(data[k+n_data_components*(j+npts2*i)]));
														}
													}
												}
											}
										} break;
										default:      /* continuous types */
										{
											/* Only reads points, and normals, No texture coordinates */
#if defined (DEBUG_CODE)
											/*???debug */
											printf("default\n");
#endif /* defined (DEBUG_CODE) */
											ALLOCATE(pointlist,Triple,npts1*npts2);
											ALLOCATE(normallist,Triple,npts1*npts2);
											for (i=0;i<npts1;i++)
											{
												for (j=0;j<npts2;j++)
												{
													for (k=0;k<3;k++)
													{
														IO_stream_scan(stream,"%f",&(pointlist[i+npts1 * j][k]));
													}
													for (k=0;k<3;k++)
													{
														IO_stream_scan(stream,"%f",
															&(normallist[i+npts1 * j][k]));
													}
												}
											}
											if (n_data_components)
											{
												ALLOCATE(data,GLfloat,n_data_components*npts1*npts2);
												for (i=0;i<npts1;i++)
												{
													for (j=0;j<npts2;j++)
													{
														for (k=0;k<n_data_components;k++)
														{
															IO_stream_scan(stream,"%f",&(data[k+n_data_components*(i+npts2*j)]));
														}
													}
												}
											}
										} break;
									}
									struct GT_surface_vertex_buffers *surface =
										CREATE(GT_surface_vertex_buffers)(surface_type,
											CMZN_GRAPHICS_RENDER_POLYGON_MODE_SHADED);
									fill_surface_graphics_vertex_array(GT_object_get_vertex_set(obj),
										g_TRIANGLE, npts1, npts2,
										pointlist,normallist, /*tangentlist*/(Triple *)NULL,
										texturelist, n_data_components, data);
									GT_OBJECT_ADD(GT_surface_vertex_buffers)(obj,surface);
									if (pointlist)
									{
										DEALLOCATE(pointlist);
									}
									if (normallist)
									{
										DEALLOCATE(normallist);
									}
									if (texturelist)
									{
										DEALLOCATE(texturelist);
									}
									if (data)
									{
										DEALLOCATE(data);
									}
								}
								else
								{
									display_message(ERROR_MESSAGE,
										"file_read_graphics_objects.  Unknown surface type");
									return_code=0;
								}
							} break;
							default:
							{
								display_message(ERROR_MESSAGE,"file_read_graphics_objects.  "
									"Cannot read in objects of type '%s'",
									get_GT_object_type_string(object_type));
								return_code=0;
							} break;
						} /* switch (object_type) */
						}
						if (return_code)
						{
							glyph->changed();
						}
						if (obj)
						{
							DEACCESS(GT_object)(&obj);
						}
						cmzn_glyph_destroy(&glyph);
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"file_read_graphics_objects.  Unknown object type");
						return_code=0;
					}
				}
			}
			IO_stream_close(stream);
			DESTROY(IO_stream)(&stream);
		}
		else
		{
			return_code=0;
			display_message(ERROR_MESSAGE,
				"file_read_graphics_objects.  Could not open file");
		}
	}
	else
	{
		return_code=0;
		display_message(ERROR_MESSAGE,
			"file_read_graphics_objects.  Invalid argument(s)");
	}
	cmzn_glyphmodule_end_change(glyphmodule);

	return (return_code);
} /* file_read_graphics_objects */

#define MAX_OBJ_VERTICES (128)

int file_read_surface_graphics_object_from_obj(char *file_name,
	struct IO_stream_package *io_stream_package,
	char *graphics_object_name, enum cmzn_graphics_render_polygon_mode render_polygon_mode,
	ZnReal time, struct cmzn_materialmodule *materialmodule,
	struct cmzn_glyphmodule *glyphmodule)
{
	char face_word[MAX_OBJ_VERTICES][128], objname[100], *text, *word, matname[128];
	FE_value rmag, result[3], vector1[3], vector2[3], vectorsum[3], vertex0[3],
		vertex1[3], vertex2[3];
	ZnReal *new_coordinate_vertices = 0, *coordinate_vertices = 0,
		*new_normal_vertices, *normal_vertices, *new_texture_vertices, *texture_vertices;
	int face_index,face_vertex[MAX_OBJ_VERTICES][3], i, j, k, line_number,
		number_of_triangles,  number_of_vertices, n_face_vertices,  n_obj_coordinate_vertices,
		n_obj_normal_vertices, n_obj_texture_vertices, return_code,
		*vertex_reindex, warning_multiple_normals;
	gtObject *new_obj, *next_obj, *obj;
	cmzn_material *scanned_material;
	GT_surface_vertex_buffers *surfaces = 0;
	struct IO_stream *file;
	struct VT_iso_vertex *vertex, **vertex_list;
	struct VT_iso_triangle *triangle, **triangle_list;

	cmzn_glyphmodule_begin_change(glyphmodule);
	return_code = 1;
	if (file_name && materialmodule)
	{
		file = CREATE(IO_stream)(io_stream_package);
		if (file && (IO_stream_open_for_read(file, file_name)))
		{
			if(graphics_object_name)
			{
				sprintf(objname, "%s", graphics_object_name);
			}
			else
			{
				sprintf(objname, "%s", file_name);
			}
			cmzn_glyph_id glyph = cmzn_glyphmodule_find_glyph_by_name(glyphmodule, objname);
			obj = 0;
			cmzn_glyph_static *glyphStatic = dynamic_cast<cmzn_glyph_static*>(glyph);
			if (glyphStatic)
			{
				obj = glyphStatic->getGraphicsObject();
			}
			else if (glyph)
			{
				display_message(ERROR_MESSAGE, "Read wavefront obj file: Cannot modify non-static glyph '%s'", objname);
				cmzn_glyph_destroy(&glyph);
				return_code = 0;
			}
			if (obj)
			{
				next_obj = obj;
				while (next_obj)
				{
					if (g_SURFACE_VERTEX_BUFFERS==GT_object_get_type(next_obj))
					{
						// Note: time is now ignored
						return_code = GT_object_clear_primitives(next_obj);
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"Object of different type named '%s' already exists",
							objname);
						return_code=0;
					}
					next_obj = GT_object_get_next_object(next_obj);
				}
			}
			else if (return_code)
			{
				obj = CREATE(GT_object)(objname, g_SURFACE_VERTEX_BUFFERS, NULL);
				glyph = cmzn_glyphmodule_create_glyph_static(glyphmodule, obj);
				if (glyph)
				{
					glyph->setName(objname);
					glyph->setManaged(true);
				}
				else
				{
					return_code = 0;
				}
			}

			if (return_code)
			{
				warning_multiple_normals = 0;
				/* default material is NULL so that it gets controlled by the graphics_object */
				scanned_material=(cmzn_material *)NULL;

				line_number = 0;
				number_of_vertices = 0;
				number_of_triangles = 0;
				n_obj_coordinate_vertices = 0;
				n_obj_texture_vertices = 0;
				n_obj_normal_vertices = 0;
				vertex_list = (struct VT_iso_vertex **)NULL;
				triangle_list = (struct VT_iso_triangle **)NULL;
				coordinate_vertices = (ZnReal *)NULL;
				texture_vertices = (ZnReal *)NULL;
				normal_vertices = (ZnReal *)NULL;
				vertex_reindex = (int *)NULL;

				while ((!IO_stream_end_of_stream(file))&&
					IO_stream_read_string(file,"[^\n]",&text)&&
					IO_stream_getc(file))
				{
					line_number++;

					/* parse line */
					word=strtok(text," \t\n");
					if (word)
					{
						if (0==strcmp(word,"#"))
						{
							/* Comment */
						}
						else if (0==strcmp(word,"g"))
						{
							/* Group */
						}
						else if (0==strcmp(word,"g\n"))
						{
							/* Group default */
						}
						else if (0==strcmp(word,"s"))
						{
							/* Smooth */
						}
						else if (0==strcmp(word,"usemtl"))
						{
							word=strtok(NULL," \n");
							if (word)
							{
								if (number_of_vertices && vertex_list &&
									number_of_triangles && triangle_list)
								{
									surfaces = CREATE(GT_surface_vertex_buffers)(g_SH_DISCONTINUOUS_TEXMAP,
										render_polygon_mode);
									struct Graphics_vertex_array *array = GT_object_get_vertex_set(obj);
									GLfloat floatField[3];
									unsigned int total_number_of_vertices = number_of_triangles * 3;
									unsigned int vertex_start = 0;
									int polygonType = (int)g_TRIANGLE;
									array->add_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POLYGON,
										1, 1, &polygonType);
									for (int k = 0; k < number_of_triangles; k++)
									{
										for (int l = 0; l < 3; l++)
										{
											CAST_TO_OTHER(floatField,triangle_list[k]->vertices[l]->coordinates,GLfloat,3);
											array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
												3, 1, floatField);
											CAST_TO_OTHER(floatField,triangle_list[k]->vertices[l]->normal,GLfloat,3);
											array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
												3, 1, floatField);
										}
									}
									array->add_unsigned_integer_attribute(
										GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
										1, 1, &total_number_of_vertices);
									array->add_unsigned_integer_attribute(
										GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
										1, 1, &vertex_start);
									if (surfaces)
									{
										return_code = GT_OBJECT_ADD(GT_surface_vertex_buffers)(obj, surfaces);
									}
									else
									{
										display_message(ERROR_MESSAGE,
											"file_read_surface_graphics_object_from_obj.  "
											"Unable create GT_surface_vertex_buffers when changing material");
										return_code=0;
									}
								}
								else
								{
									if (vertex_list)
									{
										DEALLOCATE(vertex_list);
									}
									if (triangle_list)
									{
										DEALLOCATE(triangle_list);
									}
								}
								number_of_vertices = 0;
								number_of_triangles = 0;
								vertex_list = (struct VT_iso_vertex **)NULL;
								triangle_list = (struct VT_iso_triangle **)NULL;

								/* print material name */
								strncpy(matname, word, strlen(word));
								matname[strlen(word)] = 0;
								scanned_material=FIND_BY_IDENTIFIER_IN_MANAGER(
									cmzn_material,name)(matname,
										cmzn_materialmodule_get_manager(materialmodule));
								if (scanned_material)
									ACCESS(cmzn_material)(scanned_material);
								if (!(scanned_material ||
										fuzzy_string_compare_same_length(matname,"NONE")))
								{
									scanned_material = cmzn_material_create_private();
									cmzn_material_set_name(scanned_material, matname);
									if(scanned_material &&
										(ADD_OBJECT_TO_MANAGER(cmzn_material)
											(scanned_material, cmzn_materialmodule_get_manager(materialmodule))))
									{
										cmzn_material_set_managed(scanned_material, true);
									}
									else
									{
										scanned_material = (cmzn_material *)NULL;
									}
								}
								new_obj = GT_object_get_next_object(obj);
								if (new_obj)
								{
									/* Could check that the materials match although I don't know
										what to do if they don't */
									ACCESS(GT_object)(new_obj);
								}
								else
								{
									/* Make a new obj using the new material */
									new_obj = CREATE(GT_object)(objname, g_SURFACE_VERTEX_BUFFERS, scanned_material);
									GT_object_set_next_object(obj, new_obj);
								}
								REACCESS(GT_object)(&obj, new_obj);
								DEACCESS(GT_object)(&new_obj);
								if (scanned_material)
								{
									cmzn_material_destroy(&scanned_material);
								}
							}
							else
							{
								printf("Error: Missing material\n");
							}
						}
						else if (0==strcmp(word,"mtllib"))
						{
							/* Material library */
						}
						else if (0==strcmp(word, "v"))
						{
							/* process vertices */
							if(REALLOCATE(new_coordinate_vertices, coordinate_vertices, ZnReal,
									3 * (n_obj_coordinate_vertices + 1)) &&
								REALLOCATE(vertex_reindex, vertex_reindex, int,
									(n_obj_coordinate_vertices + 1)))
							{
								coordinate_vertices = new_coordinate_vertices;

								vertex_reindex[n_obj_coordinate_vertices] = -1;
								new_coordinate_vertices = coordinate_vertices + 3 * n_obj_coordinate_vertices;
								i=0;
								while (NULL != (word=strtok(NULL," ")))
								{
									new_coordinate_vertices[i]=atof(word);
									i++;
								}
								while (i < 3)
								{
									new_coordinate_vertices[i]=0;
									i++;
								}
								n_obj_coordinate_vertices++;
							}
						}
						else if (0==strcmp(word,"vt"))
						{
							/* process texture vertices */
							if(REALLOCATE(new_texture_vertices, texture_vertices, ZnReal,
									3 * (n_obj_texture_vertices + 1)))
							{
								texture_vertices = new_texture_vertices;

								new_texture_vertices = texture_vertices + 3 * n_obj_texture_vertices;
								i=0;
								while (NULL != (word=strtok(NULL," ")))
								{
									new_texture_vertices[i]=atof(word);
									i++;
								}
								while (i < 3)
								{
									new_texture_vertices[i]=0;
									i++;
								}
								n_obj_texture_vertices++;
							}
						}
						else if (0==strcmp(word,"vn"))
						{
							/* process vertex normals */
							if(REALLOCATE(new_normal_vertices, normal_vertices, ZnReal,
									3 * (n_obj_normal_vertices + 1)))
							{
								normal_vertices = new_normal_vertices;

								new_normal_vertices = normal_vertices + 3 * n_obj_normal_vertices;
								i=0;
								while (NULL != (word=strtok(NULL," ")))
								{
									new_normal_vertices[i]=atof(word);
									i++;
								}
								while (i < 3)
								{
									new_normal_vertices[i]=0;
									i++;
								}

								n_obj_normal_vertices++;
							}
						}
						else if (0==strcmp(word,"f"))
						{
							/* process faces */
							/* SAB This should be improved, it thinks that there is
								an extra face when there is a blank on the end of a
								line */
							n_face_vertices=0;
							while (NULL != (word=strtok(NULL," ")))
							{
								if (n_face_vertices<MAX_OBJ_VERTICES)
								{
									strcpy(face_word[n_face_vertices],word);
								}
								n_face_vertices++;
							}
							if (n_face_vertices>MAX_OBJ_VERTICES)
							{
								n_face_vertices=MAX_OBJ_VERTICES;
								display_message(ERROR_MESSAGE,
									"read_volume_texture_from_file.  Exceeded MAX_OBJ_VERTICES");
								/*???debug */
								printf("Error: exceeded MAX_OBJ_VERTICES\n");
							}
							for (i=0;i<n_face_vertices;i++)
							{
								face_vertex[i][0]=0;
								face_vertex[i][1]=0;
								face_vertex[i][2]=0;
								if (!strstr(face_word[i],"/"))
								{
									face_vertex[i][0]=atoi(face_word[i]);
								}
								else
								{
									if (strstr(face_word[i],"//"))
									{
										if (NULL != (word=strtok(face_word[i],"//")))
										{
											face_vertex[i][0]=atoi(word);
											if(NULL != (word=strtok(NULL,"//")))
											{
												face_vertex[i][1]=0;
												face_vertex[i][2]=atoi(word);
												word=strtok(NULL,"/");
											}
										}
										else
										{
											face_vertex[i][0]=atoi(face_word[i]);
										}
									}
									else
									{
										if (NULL != (word=strtok(face_word[i], "/")))
										{
											face_vertex[i][0]=atoi(word);
											if (NULL != (word=strtok(NULL,"/")))
											{
												face_vertex[i][1]=atoi(word);
												if(NULL != (word=strtok(NULL,"/")))
												{
													face_vertex[i][2]=atoi(word);
												}
											}
										}
										else
										{
											face_vertex[i][0]=atoi(face_word[i]);
										}
									}
								}
							}
							if (n_face_vertices >= 3)
							{
								for (k=1;k<n_face_vertices-1;k++)
								{
									/* fan from face_vertex[0] */
									if ((triangle = CREATE(VT_iso_triangle)()) &&
										REALLOCATE(triangle_list, triangle_list, struct VT_iso_triangle *, number_of_triangles + 1))
									{
										triangle_list[number_of_triangles] = triangle;
										triangle->index = number_of_triangles;
										for (i=0;i<3;i++)
										{
											/* NB: we subtract 1 from the obj file
												vertex index for consistency with C
												arrays */
											if(i==0)
											{
												face_index = 0;
											}
											else
											{
												face_index = i+k-1;
											}
											if((face_vertex[face_index][0] > 0) &&
												(face_vertex[face_index][0] <= n_obj_coordinate_vertices))
											{
												/* Only generating new vertices when they are used.
													Could also add code where coordinate vertices are duplicated
													to support multiple texture coordinates or normals */
												if (-1 == vertex_reindex[face_vertex[face_index][0] - 1])
												{
													/* Create a new vertex */
													if ((vertex = CREATE(VT_iso_vertex)()) &&
														REALLOCATE(vertex_list, vertex_list, struct VT_iso_vertex *, number_of_vertices + 1))
													{
														vertex_list[number_of_vertices] = vertex;
														vertex->index = number_of_vertices;

														vertex->coordinates[0] =
															coordinate_vertices[3 * (face_vertex[face_index][0] - 1)];
														vertex->coordinates[1] =
															coordinate_vertices[3 * (face_vertex[face_index][0] - 1) + 1];
														vertex->coordinates[2] =
															coordinate_vertices[3 * (face_vertex[face_index][0] - 1) + 2];

														vertex_reindex[face_vertex[face_index][0] - 1] = number_of_vertices;

														number_of_vertices++;
													}
													else
													{
														display_message(WARNING_MESSAGE,
															"read_volume_texture_from_file: "
															" Unable to create vertex.");
														return_code = 0;
													}
												}
												else
												{
													vertex = vertex_list[vertex_reindex[face_vertex[face_index][0] - 1]];
												}

												triangle->vertices[i] = vertex;

												if (REALLOCATE(vertex->triangles, vertex->triangles,
														struct VT_iso_triangle *, vertex->number_of_triangles + 1))
												{
													vertex->triangles[vertex->number_of_triangles] =
														triangle;
													vertex->number_of_triangles++;
												}
												else
												{
													display_message(WARNING_MESSAGE,
														"read_volume_texture_from_file: "
														" Unable to reallocate triangle list");
													return_code = 0;
												}
											}
											else
											{
												display_message(WARNING_MESSAGE,
													"read_volume_texture_from_file: vertex"
													" %d not defined when used", face_vertex[face_index][0]);
												vertex = (struct VT_iso_vertex *)NULL;
												return_code = 0;
											}

											if(vertex && face_vertex[face_index][1])
											{
												if((face_vertex[face_index][1] > 0) &&
													(face_vertex[face_index][1] <= n_obj_texture_vertices))
												{
													vertex->texture_coordinates[0] =
														texture_vertices[3 * (face_vertex[face_index][1] - 1)];
													vertex->texture_coordinates[1] =
														texture_vertices[3 * (face_vertex[face_index][1] - 1) + 1];
													vertex->texture_coordinates[2] =
														texture_vertices[3 * (face_vertex[face_index][1] - 1) + 2];
												}
												else
												{
													display_message(WARNING_MESSAGE,
														"read_volume_texture_from_file: texture vertex"
														" %d not defined when used", face_vertex[face_index][1]);
													return_code = 0;

												}
											}

											if(vertex && face_vertex[face_index][2])
											{
												if((face_vertex[face_index][2] > 0) &&
													(face_vertex[face_index][2] <= n_obj_normal_vertices))
												{
													if(vertex->normal[0] || vertex->normal[1]
														|| vertex->normal[2])
													{
														if ((vertex->normal[0] !=
																normal_vertices[3 * (face_vertex[face_index][2] - 1)])
															|| (vertex->normal[1] !=
																normal_vertices[3 * (face_vertex[face_index][2] - 1) + 1])
															|| (vertex->normal[2] !=
																normal_vertices[3 * (face_vertex[face_index][2] - 1) + 2]))
														{
															if(!warning_multiple_normals)
															{
																display_message(WARNING_MESSAGE,
																	"read_volume_texture_from_file: multiple normals defined"
																	" for vertices, first normals used");
																warning_multiple_normals = 1;
															}
														}
													}
													else
													{
														vertex->normal[0] = normal_vertices[3 * (face_vertex[face_index][2] - 1)];
														vertex->normal[1] = normal_vertices[3 * (face_vertex[face_index][2] - 1) + 1];
														vertex->normal[2] = normal_vertices[3 * (face_vertex[face_index][2] - 1) + 2];
													}
												}
												else
												{
													display_message(WARNING_MESSAGE,
														"read_volume_texture_from_file: normal vertex"
														" %d not defined when used", face_vertex[face_index][1]);
													return_code = 0;
												}
											}
										}
										number_of_triangles++;
									}
									else
									{
										display_message(ERROR_MESSAGE,
											"read_volume_texture_from_file: Could not allocate vertex");
										return_code = 0;
									}
								}
							}
							else
							{
								display_message(WARNING_MESSAGE,
									"Less than 3 vertices in face");
							}
						}
					}
					DEALLOCATE(text);
				}

				if (coordinate_vertices)
				{
					DEALLOCATE(coordinate_vertices);
				}
				if (vertex_reindex)
				{
					DEALLOCATE(vertex_reindex);
				}
				if (normal_vertices)
				{
					DEALLOCATE(normal_vertices);
				}
				if (texture_vertices)
				{
					DEALLOCATE(texture_vertices);
				}
			}
			if (return_code)
			{
				if (0 == n_obj_normal_vertices)
				{
					/* now calculate vertex normals in cartesian space by
						averaging normals of surrounding faces */
					for (i=0;i<number_of_vertices;i++)
					{
						for (k=0;k<3;k++)
						{
							vectorsum[k]=0;
						}
						for (j=0;j<vertex_list[i]->number_of_triangles;j++)
						{
							triangle=vertex_list[i]->triangles[j];
							for (k=0;k<3;k++)
							{
								vertex0[k]=triangle->vertices[0]->coordinates[k];
								vertex1[k]=triangle->vertices[1]->coordinates[k];
								vertex2[k]=triangle->vertices[2]->coordinates[k];
								vector1[k]=vertex1[k]-vertex0[k];
								vector2[k]=vertex2[k]-vertex0[k];
							}
							cross_product_FE_value_vector3(vector1, vector2, result);
							normalize_FE_value3(result);
							for (k=0;k<3;k++)
							{
								vectorsum[k] += result[k];
							}
						}
						/* now set normal as the average & normalize */
						rmag=sqrt((double)(vectorsum[0]*vectorsum[0]+
								vectorsum[1]*vectorsum[1]+vectorsum[2]*vectorsum[2]));
						if (rmag < FE_VALUE_ZERO_TOLERANCE)
						{
							/* If the magnitude is zero then just copy the normal values */
							rmag = 1.0;
						}
						for (k=0;k<3;k++)
						{
							vertex_list[i]->normal[k] = vectorsum[k]/rmag;
						}
					}
				}
			}
			if (return_code)
			{
				surfaces = CREATE(GT_surface_vertex_buffers)(
					g_SH_DISCONTINUOUS_TEXMAP, render_polygon_mode);
				struct Graphics_vertex_array *array = GT_object_get_vertex_set(obj);
				GLfloat floatField[3];
				unsigned int total_number_of_vertices = number_of_triangles * 3;
				unsigned int vertex_start = 0;
				int polygonType = (int)g_TRIANGLE;
				array->add_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POLYGON,
					1, 1, &polygonType);
				for (int k = 0; k < number_of_triangles; k++)
				{
					for (int l = 0; l < 3; l++)
					{
						CAST_TO_OTHER(floatField,triangle_list[k]->vertices[l]->coordinates,GLfloat,3);
						array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
							3, 1, floatField);
						CAST_TO_OTHER(floatField,triangle_list[k]->vertices[l]->normal,GLfloat,3);
						array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
							3, 1, floatField);
					}
				}
				array->add_unsigned_integer_attribute(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
					1, 1, &total_number_of_vertices);
				array->add_unsigned_integer_attribute(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
					1, 1, &vertex_start);
				if (surfaces)
				{
					return_code = GT_OBJECT_ADD(GT_surface_vertex_buffers)(obj, surfaces);
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"file_read_surface_graphics_object_from_obj.  "
						"Unable create GT_surface_vertex_buffers");
					return_code=0;
				}
				if (return_code)
				{
					glyph->changed();
				}
			}
			DEACCESS(GT_object)(&obj);
			cmzn_glyph_destroy(&glyph);
			IO_stream_close(file);
			DESTROY(IO_stream)(&file);
		}
		else
		{
			return_code=0;
			display_message(ERROR_MESSAGE,
				"file_read_surface_graphics_object_from_obj.  Unable to open file %s", file_name);
		}
	}
	else
	{
		return_code=0;
		display_message(ERROR_MESSAGE,
			"file_read_surface_graphics_object_from_obj.  Invalid argument(s)");
	}
	cmzn_glyphmodule_end_change(glyphmodule);
	LEAVE;

	return (return_code);
}
