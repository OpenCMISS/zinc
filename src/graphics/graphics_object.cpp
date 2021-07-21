/*******************************************************************************
FILE : graphics_object.cpp

DESCRIPTION :
gtObject/gtWindow management routines.
???DB.  Used to be gtutil.c
???DB.  7 June 1994.  Merged GTTEXT into GTPOINT and GTPOINTSET and added a
	marker type and a marker size
???DB.  11 January 1997.  Added pointers to the nodes in a GTPOINTSET.  This is
	a temporary measure to allow the graphical_node_editor to work (will be
	replaced by the graphical FE_node). See below.
???RC.  Changed above nodes to integer names for	OpenGL picking.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <math.h>
#include "opencmiss/zinc/zincconfigure.h"

#include "opencmiss/zinc/field.h"
#include "opencmiss/zinc/fieldsubobjectgroup.h"
#include "opencmiss/zinc/material.h"
#include "opencmiss/zinc/status.h"
#include "general/debug.h"
#include "general/mystring.h"
#include "general/object.h"
#include "general/octree.h"
#include "graphics/auxiliary_graphics_types.h"
#include "graphics/font.h"
#include "graphics/glyph.hpp"
#include "graphics/graphics_object.h"
#include "graphics/material.h"
#include "graphics/spectrum.h"
#include "graphics/volume_texture.h"
#include "general/message.h"
#if defined (USE_OPENCASCADE)
#include "opencmiss/zinc/fieldcad.h"
#endif /* defined (USE_OPENCASCADE) */
#include "graphics/render_gl.h"
#include "graphics/graphics_object.hpp"
#include "graphics/graphics_object_highlight.hpp"
#include "graphics/graphics_object_private.hpp"

/*
Module types
------------
*/

static int GT_object_get_time_number(struct GT_object *graphics_object,
	ZnReal time)
/*******************************************************************************
LAST MODIFIED : 18 June 1998

DESCRIPTION :
The graphics object stores a list of times at which primitives/key frames are
kept. This function returns the index into this array that <time> is at,
starting with 1 for the first time, whereas 0 is returned if the time is not
found or there is an error.
==============================================================================*/
{
	ZnReal *times;
	int time_number;

	ENTER(GT_object_get_time_number);
	if (graphics_object)
	{
		if (0<(time_number=graphics_object->number_of_times))
		{
			times=graphics_object->times;
			if (times)
			{
				times += time_number-1;
				while ((time_number>0)&&(time< *times))
				{
					times--;
					time_number--;
				}
				if ((time_number>0)&&(time != *times))
				{
					time_number=0;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"GT_object_get_time_number.  Invalid times array");
				time_number=0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"GT_object_get_time_number.  Invalid arguments");
		time_number=0;
	}
	LEAVE;

	return (time_number);
} /* GT_object_get_time_number */

#if ! defined (SHORT_NAMES)
#define GT_OBJECT_REMOVE_PRIMITIVES_AT_TIME_NUMBER(primitive_type) \
	GT_object_remove_primitives_at_time_number_ ## primitive_type
#else
#define GT_OBJECT_REMOVE_PRIMITIVES_AT_TIME_NUMBER(primitive_type) \
	gorptn_ ## primitive_type
#endif

#define DECLARE_GT_OBJECT_REMOVE_PRIMITIVES_AT_TIME_NUMBER_FUNCTION( \
	primitive_type,gt_object_type,primitive_var) \
static int GT_OBJECT_REMOVE_PRIMITIVES_AT_TIME_NUMBER(primitive_type)( \
	struct GT_object *graphics_object, int time_number, \
	GT_object_primitive_object_name_conditional_function *conditional_function, \
	void *user_data) \
/***************************************************************************** \
LAST MODIFIED : 17 March 2003 \
\
DESCRIPTION : \
Removes all primitives from <graphics_object> at <time_number>. \
The optional <conditional_function> allows a subset of the primitives to \
be removed. This function is called with the object_name integer associated \
with each primitive plus the void *<user_data> supplied here. A true result \
from the conditional_function causes the primitive to be removed. \
============================================================================*/ \
{ \
	ZnReal *times; \
	int i, return_code; \
	struct primitive_type *last_primitive, *primitive, **primitive_ptr; \
	union GT_primitive_list *primitive_list; \
\
	ENTER(GT_OBJECT_REMOVE_PRIMITIVES_AT_TIME_NUMBER(primitive_type)); \
	if (graphics_object && (gt_object_type == graphics_object->object_type) && \
		(0 < time_number) && (time_number <= graphics_object->number_of_times)) \
	{ \
		times = graphics_object->times; \
		if (times) \
		{ \
			if (graphics_object->primitive_lists) \
			{ \
				primitive_list = graphics_object->primitive_lists + time_number - 1; \
				last_primitive = (struct primitive_type *)NULL; \
				primitive_ptr = &(primitive_list->primitive_var.first); \
				/* remove primitives at this time */ \
				while (NULL != (primitive = *primitive_ptr)) \
				{ \
					if ((!conditional_function) || \
						(conditional_function)(primitive->object_name, user_data)) \
					{ \
						*primitive_ptr = primitive->ptrnext; \
						DESTROY(primitive_type)(&primitive); \
					} \
					else \
					{ \
						last_primitive = primitive; \
						primitive_ptr = &(primitive->ptrnext); \
					} \
				} \
				primitive_list->primitive_var.last = last_primitive; \
				/* must remove time if no primitives left there */ \
				if ((struct primitive_type *)NULL == primitive_list->primitive_var.first) \
				{ \
					times += time_number - 1; \
					/* Shift following times and primitives down */ \
					for (i = (graphics_object->number_of_times)-time_number; i > 0; i--) \
					{ \
						*times = *(times + 1); \
						times++; \
						primitive_list->primitive_var.first = primitive_list[1].primitive_var.first; \
						primitive_list->primitive_var.last = primitive_list[1].primitive_var.last; \
						primitive_ptr++; \
					} \
					graphics_object->number_of_times--; \
					/* do not reallocate to make times and primitive_var arrays smaller \
						 since this function is most often called by DESTROY(GT_object), \
						 or just before primitives are to be added again at the same time. \
						 However, must deallocate if there are no times left so DESTROY \
						 function can rely on this function to clean up. */ \
					if (0 == graphics_object->number_of_times) \
					{ \
						DEALLOCATE(graphics_object->times); \
						graphics_object->times = (ZnReal *)NULL; \
						DEALLOCATE(graphics_object->primitive_lists); \
					} \
				} \
				return_code = 1; \
			} \
			else \
			{ \
				display_message(ERROR_MESSAGE, \
					"GT_OBJECT_REMOVE_PRIMITIVES_AT_TIME_NUMBER(" \
					#primitive_type ").  Invalid primitive lists"); \
				return_code = 0; \
			} \
		} \
		else \
		{ \
			display_message(ERROR_MESSAGE, \
				"GT_OBJECT_REMOVE_PRIMITIVES_AT_TIME_NUMBER(" \
				#primitive_type ").  Invalid times array"); \
			return_code = 0; \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"GT_OBJECT_REMOVE_PRIMITIVES_AT_TIME_NUMBER(" \
			#primitive_type ").  Invalid arguments"); \
		return_code = 0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* GT_OBJECT_REMOVE_PRIMITIVES_AT_TIME_NUMBER(primitive_type) */

#define DECLARE_GT_OBJECT_REMOVE_PRIMITIVES_AT_TIME_NUMBER_AUXILIARY_FUNCTION( \
	primitive_type,gt_object_type,primitive_var) \
int GT_OBJECT_REMOVE_PRIMITIVES_AT_TIME_NUMBER(primitive_type)( \
	struct GT_object *graphics_object, int time_number, \
	GT_object_primitive_object_name_conditional_function *conditional_function, \
	void *user_data) \
/***************************************************************************** \
LAST MODIFIED : 17 March 2003 \
\
DESCRIPTION : \
Version of GT_OBJECT_REMOVE_PRIMITIVES_AT_TIME_NUMBER for primitives with \
both an <object_name> and an <auxiliary_object_name> that may satisfy the \
conditional_function. \
============================================================================*/ \
{ \
	ZnReal *times; \
	int i, return_code; \
	struct primitive_type *last_primitive, *primitive, **primitive_ptr; \
	union GT_primitive_list *primitive_list; \
\
	ENTER(GT_OBJECT_REMOVE_PRIMITIVES_AT_TIME_NUMBER(primitive_type)); \
	if (graphics_object && (gt_object_type == graphics_object->object_type) && \
		(0 < time_number) && (time_number <= graphics_object->number_of_times)) \
	{ \
		times = graphics_object->times; \
		if (times) \
		{ \
			if (graphics_object->primitive_lists) \
			{ \
				primitive_list = graphics_object->primitive_lists + time_number - 1; \
				last_primitive = (struct primitive_type *)NULL; \
				primitive_ptr = &(primitive_list->primitive_var.first); \
				/* remove primitives at this time */ \
				while (NULL != (primitive = *primitive_ptr)) \
				{ \
					if ((!conditional_function) || \
						(conditional_function)(primitive->object_name, user_data) || \
						(conditional_function)(primitive->auxiliary_object_name, \
							user_data)) \
					{ \
						*primitive_ptr = primitive->ptrnext; \
						DESTROY(primitive_type)(&primitive); \
					} \
					else \
					{ \
						last_primitive = primitive; \
						primitive_ptr = &(primitive->ptrnext); \
					} \
				} \
				primitive_list->primitive_var.last = last_primitive; \
				/* must remove time if no primitives left there */ \
				if ((struct primitive_type *)NULL == primitive_list->primitive_var.first) \
				{ \
					times += time_number - 1; \
					/* Shift following times and primitives down */ \
					for (i = (graphics_object->number_of_times)-time_number; i > 0; i--) \
					{ \
						*times = *(times + 1); \
						times++; \
						primitive_list->primitive_var.first = primitive_list[1].primitive_var.first; \
						primitive_list->primitive_var.last = primitive_list[1].primitive_var.last; \
						primitive_ptr++; \
					} \
					graphics_object->number_of_times--; \
					/* do not reallocate to make times and primitive_var arrays smaller \
						 since this function is most often called by DESTROY(GT_object), \
						 or just before primitives are to be added again at the same time. \
						 However, must deallocate if there are no times left so DESTROY \
						 function can rely on this function to clean up. */ \
					if (0 == graphics_object->number_of_times) \
					{ \
						DEALLOCATE(graphics_object->times); \
						graphics_object->times = (ZnReal *)NULL; \
						DEALLOCATE(graphics_object->primitive_lists); \
					} \
				} \
				return_code = 1; \
			} \
			else \
			{ \
				display_message(ERROR_MESSAGE, \
					"GT_OBJECT_REMOVE_PRIMITIVES_AT_TIME_NUMBER(" \
					#primitive_type ").  Invalid primitive lists"); \
				return_code = 0; \
			} \
		} \
		else \
		{ \
			display_message(ERROR_MESSAGE, \
				"GT_OBJECT_REMOVE_PRIMITIVES_AT_TIME_NUMBER(" \
				#primitive_type ").  Invalid times array"); \
			return_code = 0; \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"GT_OBJECT_REMOVE_PRIMITIVES_AT_TIME_NUMBER(" \
			#primitive_type ").  Invalid arguments"); \
		return_code = 0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* GT_OBJECT_REMOVE_PRIMITIVES_AT_TIME_NUMBER(primitive_type) */

static int GT_object_mark_vertex_array_primitives_changes(struct GT_object *object,
	DsLabelsChangeLog *changeLog)
{
	switch (object->object_type)
	{
		case g_POLYLINE_VERTEX_BUFFERS:
		case g_SURFACE_VERTEX_BUFFERS:
		case g_GLYPH_SET_VERTEX_BUFFERS:
		{
			if (object->vertex_array)
			{
				int *value_buffer = 0;
				unsigned int values_per_vertex = 0, vertex_count = 0;
				if (object->vertex_array->get_integer_vertex_buffer(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_OBJECT_ID, &value_buffer, &values_per_vertex,
					&vertex_count) &&  value_buffer && vertex_count)
				{
					int object_name = 0;
					int modified_required = 1;
					for (unsigned int i = 0; i < vertex_count; i++)
					{
						object_name = value_buffer[i];
						if (changeLog->isIndexChange(object_name))
						{
							object->vertex_array->replace_integer_vertex_buffer_at_position(
								GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_UPDATE_REQUIRED, i, 1, 1,
								&modified_required);
							int invalid_id = -1;
							/* setting object id here to -1, marking it as invalid, removed object
								* will not be drawn and modified object will be modified and given
								* correctly during object compilation.*/
							object->vertex_array->replace_integer_vertex_buffer_at_position(
								GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_OBJECT_ID, i, 1, 1,
								&invalid_id);
						}
					}
				}
			}
		} break;
		default:
		{
		} break;
	}
	return 1;
}

/**
 * Removes all primitives at graphics object but leaves vertex arrays intact.
 */
static int GT_object_destroy_primitives(struct GT_object *graphics_object)
{
	int return_code = 0;
	if (graphics_object)
	{
		switch (graphics_object->object_type)
		{
			case g_POINT_SET_VERTEX_BUFFERS:
			case g_POLYLINE_VERTEX_BUFFERS:
			case g_SURFACE_VERTEX_BUFFERS:
			case g_GLYPH_SET_VERTEX_BUFFERS:
			{
				if (graphics_object->number_of_times &&
					graphics_object->primitive_lists)
				{
					graphics_object->number_of_times = 0;
					if (g_POINT_SET_VERTEX_BUFFERS == graphics_object->object_type)
						DESTROY(GT_pointset_vertex_buffers)(
							&graphics_object->primitive_lists->gt_pointset_vertex_buffers);
					else if (g_POLYLINE_VERTEX_BUFFERS == graphics_object->object_type)
						DESTROY(GT_polyline_vertex_buffers)(
							&graphics_object->primitive_lists->gt_polyline_vertex_buffers);
					else if (g_SURFACE_VERTEX_BUFFERS == graphics_object->object_type)
						DESTROY(GT_surface_vertex_buffers)(
							&graphics_object->primitive_lists->gt_surface_vertex_buffers);
					else if (g_GLYPH_SET_VERTEX_BUFFERS == graphics_object->object_type)
						DESTROY(GT_glyphset_vertex_buffers)(
							&graphics_object->primitive_lists->gt_glyphset_vertex_buffers);
					DEALLOCATE(graphics_object->primitive_lists);
					DEALLOCATE(graphics_object->times);
				}
				return_code = 1;
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"GT_object_destroy_primitives.  Unknown object type");
				return_code = 0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"GT_object_destroy_primitives.  Invalid arguments");
		return_code = 0;
	}
	return (return_code);
}

/*
Global functions
----------------
*/

enum GT_object_type GT_object_get_type(struct GT_object *gt_object)
/*******************************************************************************
LAST MODIFIED : 17 June 2004

DESCRIPTION :
Returns the object type from the gt_object.
==============================================================================*/
{
	enum GT_object_type type;

	ENTER(GT_object_get_type);
	if (gt_object)
	{
		type = gt_object->object_type;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"GT_object_get_type.  Invalid argument(s)");
		type = g_OBJECT_TYPE_INVALID;
	}
	LEAVE;

	return (type);
} /* GT_object_get_type */

struct GT_object *GT_object_get_next_object(struct GT_object *gt_object)
/*******************************************************************************
LAST MODIFIED : 17 June 2004

DESCRIPTION :
Returns the next object from the gt_object.
==============================================================================*/
{
	struct GT_object *next;

	ENTER(GT_object_get_next_object);
	if (gt_object)
	{
		next = gt_object->nextobject;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"GT_object_get_next_object.  Invalid argument(s)");
		next = (struct GT_object *)NULL;
	}
	LEAVE;

	return (next);
} /* GT_object_get_next_object */

int GT_object_set_next_object(struct GT_object *gt_object,
	struct GT_object *next_object)
/*******************************************************************************
LAST MODIFIED : 17 June 2004

DESCRIPTION :
Sets the next object for the gt_object.
==============================================================================*/
{
	int return_code;

	ENTER(GT_object_set_next_object);
	if (gt_object)
	{
		REACCESS(GT_object)(&gt_object->nextobject, next_object);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"GT_object_set_next_object.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* GT_object_set_next_object */

const char *get_GT_object_type_string(enum GT_object_type object_type)
/*******************************************************************************
LAST MODIFIED : 13 August 1998

DESCRIPTION :
Returns a string describing the object type, suitable for writing to file
(and reinterpreting it later).
==============================================================================*/
{
	const char *type_string;

	ENTER(get_GT_object_type_string);
	switch (object_type)
	{
		case g_POLYLINE_VERTEX_BUFFERS:
		{
			type_string="POLYLINE_VERTEX_BUFFERS";
		} break;
		case g_GLYPH_SET_VERTEX_BUFFERS:
		{
			type_string="GLYPH_SET_VERTEX_BUFFERS";
		} break;
		case g_POINT_SET_VERTEX_BUFFERS:
		{
			type_string="POINT_SET_VERTEX_BUFFERS";
		} break;
		case g_SURFACE_VERTEX_BUFFERS:
		{
			type_string="SURFACE_VERTEX_BUFFERS";
		} break;
		case g_POINT_VERTEX_BUFFERS:
		{
			type_string="POINT_SET_VERTEX_BUFFERS";
		} break;
		default:
		{
			display_message(ERROR_MESSAGE,
				"get_GT_object_type_string.  Unknown object type");
			type_string=(const char *)NULL;
		}
	}
	LEAVE;

	return type_string;
} /* get_GT_object_type_string */

int get_GT_object_type_from_string(char *type_string,
	enum GT_object_type *object_type)
/*******************************************************************************
LAST MODIFIED : 14 August 1998

DESCRIPTION :
Returns the object type from the string produced by function
get_GT_object_type_string. For compatibility, also supports converting old
enumerator numbers (as text) into the new enumerator values, with a warning.
==============================================================================*/
{
	const char *compare_type_string;
	int return_code;
	enum GT_object_type temp_obj_type;

	ENTER(get_GT_object_type_from_string);
	if (type_string&&object_type)
	{
		return_code=1;
		temp_obj_type=g_OBJECT_TYPE_BEFORE_FIRST;
		temp_obj_type = (enum GT_object_type)((int)temp_obj_type + 1);
		while ((temp_obj_type<g_OBJECT_TYPE_AFTER_LAST)&&
			(compare_type_string=get_GT_object_type_string(temp_obj_type))&&
			(!fuzzy_string_compare_same_length(compare_type_string,type_string)))
		{
			temp_obj_type = (enum GT_object_type)((int)temp_obj_type + 1);
		}
		if (g_OBJECT_TYPE_AFTER_LAST==temp_obj_type)
		{
			display_message(ERROR_MESSAGE,"get_GT_object_type_from_string.  "
				"Object type string '%s' not recognized",type_string);
			return_code=0;
		}
		else
		{
			*object_type=temp_obj_type;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"get_GT_object_type_from_string.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* get_GT_object_type_from_string */

const char *get_GT_polyline_type_string(enum GT_polyline_type polyline_type)
/*******************************************************************************
LAST MODIFIED : 13 August 1998

DESCRIPTION :
Returns a string describing the polyline type, suitable for writing to file
(and reinterpreting it later).
==============================================================================*/
{
	const char *type_string;

	ENTER(get_GT_polyline_type_string);
	switch (polyline_type)
	{
		case g_PLAIN:
		{
			type_string="PLAIN";
		} break;
		case g_NORMAL:
		{
			type_string="NORMAL";
		} break;
		case g_PLAIN_DISCONTINUOUS:
		{
			type_string="PLAIN_DISCONTINUOUS";
		} break;
		case g_NORMAL_DISCONTINUOUS:
		{
			type_string="NORMAL_DISCONTINUOUS";
		} break;
		default:
		{
			display_message(ERROR_MESSAGE,
				"get_GT_polyline_type_string.  Unknown polyline type");
			type_string=(const char *)NULL;
		}
	}
	LEAVE;

	return type_string;
} /* get_GT_polyline_type_string */

int get_GT_polyline_type_from_string(char *type_string,
	enum GT_polyline_type *polyline_type)
/*******************************************************************************
LAST MODIFIED : 14 August 1998

DESCRIPTION :
Returns the polyline type from the string produced by function
get_GT_polyline_type_string. For compatibility, also supports converting old
enumerator numbers (as text) into the new enumerator values, with a warning.
==============================================================================*/
{
	const char *compare_type_string;
	int return_code;
	enum GT_polyline_type temp_poly_type;

	ENTER(get_GT_polyline_type_from_string);
	if (type_string&&polyline_type)
	{
		return_code=1;
		temp_poly_type=g_POLYLINE_TYPE_BEFORE_FIRST;
		temp_poly_type = (enum GT_polyline_type)((int)temp_poly_type + 1);
		while ((temp_poly_type<g_POLYLINE_TYPE_AFTER_LAST)&&
			(compare_type_string=get_GT_polyline_type_string(temp_poly_type))&&
			(!fuzzy_string_compare_same_length(compare_type_string,type_string)))
		{
			temp_poly_type = (enum GT_polyline_type)((int)temp_poly_type + 1);
		}
		if (g_POLYLINE_TYPE_AFTER_LAST==temp_poly_type)
		{
			/* Previously, polyline types were read in from .exgobj files as numbers
				 matching the enumerated values. This is unacceptable now that more
				 types have been added. For compatibility, however, the original numbers
				 are read in to the correct types with a warning message */
			if (fuzzy_string_compare("0",type_string))
			{
				*polyline_type=g_PLAIN;
			}
			else if (fuzzy_string_compare("1",type_string))
			{
				*polyline_type=g_NORMAL;
			}
			else if (fuzzy_string_compare("2",type_string))
			{
				*polyline_type=g_PLAIN_DISCONTINUOUS;
			}
			else if (fuzzy_string_compare("3",type_string))
			{
				*polyline_type=g_NORMAL_DISCONTINUOUS;
			}
			else
			{
				display_message(ERROR_MESSAGE,"get_GT_polyline_type_from_string.  "
					"Polyline type string '%s' not recognized",type_string);
				return_code=0;
			}
			if (return_code)
			{
				display_message(WARNING_MESSAGE,
					"Old style polyline type '%s' should be updated to '%s'",
					type_string,get_GT_polyline_type_string(*polyline_type));
			}
		}
		else
		{
			*polyline_type=temp_poly_type;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"get_GT_polyline_type_from_string.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* get_GT_polyline_type_from_string */

const char *get_GT_surface_type_string(enum GT_surface_type surface_type)
/*******************************************************************************
LAST MODIFIED : 13 August 1998

DESCRIPTION :
Returns a string describing the surface type, suitable for writing to file
(and reinterpreting it later).
==============================================================================*/
{
	const char *type_string;

	ENTER(get_GT_surface_type_string);
	switch (surface_type)
	{
		case g_SHADED:
		{
			type_string="SHADED";
		} break;
		case g_SH_DISCONTINUOUS:
		{
			type_string="SH_DISCONTINUOUS";
		} break;
		case g_SHADED_TEXMAP:
		{
			type_string="SHADED_TEXMAP";
		} break;
		case g_SH_DISCONTINUOUS_TEXMAP:
		{
			type_string="SH_DISCONTINUOUS_TEXMAP";
		} break;
		case g_SH_DISCONTINUOUS_STRIP:
		{
			type_string="SH_DISCONTINUOUS_STRIP";
		} break;
		case g_SH_DISCONTINUOUS_STRIP_TEXMAP:
		{
			type_string="SH_DISCONTINUOUS_STRIP_TEXMAP";
		} break;
		default:
		{
			display_message(ERROR_MESSAGE,
				"get_GT_surface_type_string.  Unknown surface type");
			type_string=(const char *)NULL;
		}
	}
	LEAVE;

	return type_string;
} /* get_GT_surface_type_string */

int get_GT_surface_type_from_string(char *type_string,
	enum GT_surface_type *surface_type)
/*******************************************************************************
LAST MODIFIED : 14 August 1998

DESCRIPTION :
Returns the surface type from the string produced by function
get_GT_surface_type_string. For compatibility, also supports converting old
enumerator numbers (as text) into the new enumerator values, with a warning.
==============================================================================*/
{
	const char *compare_type_string;
	int return_code;
	enum GT_surface_type temp_surf_type;

	ENTER(get_GT_surface_type_from_string);
	if (type_string&&surface_type)
	{
		return_code=1;
		temp_surf_type=g_SURFACE_TYPE_BEFORE_FIRST;
		temp_surf_type = (enum GT_surface_type)((int)temp_surf_type + 1);
		while ((temp_surf_type<g_SURFACE_TYPE_AFTER_LAST)&&
			(compare_type_string=get_GT_surface_type_string(temp_surf_type))&&
			(!fuzzy_string_compare_same_length(compare_type_string,type_string)))
		{
			temp_surf_type = (enum GT_surface_type)((int)temp_surf_type + 1);
		}
		if (g_SURFACE_TYPE_AFTER_LAST==temp_surf_type)
		{
			/* Previously, surface types were read in from .exgobj files as numbers
				 matching the enumerated values. This is unacceptable now that more
				 types have been added. For compatibility, however, the original numbers
				 are read in to the correct types with a warning message */
			if (fuzzy_string_compare("0",type_string))
			{
				*surface_type=g_SHADED;
			}
			else if (fuzzy_string_compare("3",type_string))
			{
				*surface_type=g_SH_DISCONTINUOUS;
			}
			else if (fuzzy_string_compare("6",type_string))
			{
				*surface_type=g_SHADED_TEXMAP;
			}
			else if (fuzzy_string_compare("7",type_string))
			{
				*surface_type=g_SH_DISCONTINUOUS_TEXMAP;
			}
			else
			{
				display_message(ERROR_MESSAGE,"get_GT_surface_type_from_string.  "
					"Surface type string '%s' not recognized",type_string);
				return_code=0;
			}
			if (return_code)
			{
				display_message(WARNING_MESSAGE,
					"Old style surface type '%s' should be updated to '%s'",
					type_string,get_GT_surface_type_string(*surface_type));
			}
		}
		else
		{
			*surface_type=temp_surf_type;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"get_GT_surface_type_from_string.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* get_GT_surface_type_from_string */

int GT_glyphset_vertex_buffers_setup(GT_glyphset_vertex_buffers *glyphset,
	struct GT_object *glyph, enum cmzn_glyph_repeat_mode glyph_repeat_mode,
	Triple base_size, Triple scale_factors, Triple offset, cmzn_font_id font,
	Triple label_offset, char *static_label_text[3],
	int label_bounds_dimension, int label_bounds_components)
{
	if (glyphset)
	{
		if (glyph)
			glyphset->glyph = ACCESS(GT_object)(glyph);
		if (font)
			glyphset->font = ACCESS(cmzn_font)(font);
		for (int i = 0; i < 3; ++i)
		{
			glyphset->base_size[i] = base_size[i];
			glyphset->scale_factors[i] = scale_factors[i];
			glyphset->offset[i] = offset[i];
			glyphset->label_offset[i] = label_offset[i];
			glyphset->static_label_text[i] =
			(static_label_text && static_label_text[i]) ? duplicate_string(static_label_text[i]) : 0;
		}
		glyphset->label_bounds_dimension = label_bounds_dimension;
		glyphset->label_bounds_components = label_bounds_components;
		glyphset->glyph_repeat_mode = glyph_repeat_mode;
		return 1;
	}
	return 0;
}

/***************************************************************************//**
 * Creates the shared scene information for a GT_polyline_vertex_buffers.
 */
GT_polyline_vertex_buffers *CREATE(GT_polyline_vertex_buffers)(
	GT_polyline_type polyline_type, int line_width)
{
	struct GT_polyline_vertex_buffers *polyline;

	if (ALLOCATE(polyline,struct GT_polyline_vertex_buffers,1))
	{
		polyline->polyline_type=polyline_type;
		polyline->line_width=line_width;
	}
	else
	{
		display_message(ERROR_MESSAGE,"CREATE(GT_polyline_vertex_buffers).  Not enough memory");
	}

	return (polyline);
} /* CREATE(GT_polyline_vertex_buffers) */

/***************************************************************************//**
 * Destroys the polyline.
 */
int DESTROY(GT_polyline_vertex_buffers)(GT_polyline_vertex_buffers **polyline)
{
	int return_code;

	ENTER(DESTROY(GT_polyline_vertex_buffers));
	if (polyline && *polyline)
	{
		DEALLOCATE(*polyline);
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"DESTROY(GT_polyline_vertex_buffers).  "
			"Invalid argument");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(GT_polyline_vertex_buffers) */

/***************************************************************************//**
 * Creates the shared rendition information for a GT_polyline_vertex_buffers.
 */
GT_surface_vertex_buffers *CREATE(GT_surface_vertex_buffers)(
	GT_surface_type surface_type,  enum cmzn_graphics_render_polygon_mode render_polygon_mode)
{
	struct GT_surface_vertex_buffers *surface;

	ENTER(CREATE(GT_surface_vertex_buffers));
	if (ALLOCATE(surface,struct GT_surface_vertex_buffers,1))
	{
		surface->surface_type=surface_type;
		surface->render_polygon_mode=render_polygon_mode;
	}
	else
	{
		display_message(ERROR_MESSAGE,"CREATE(GT_polyline_vertex_buffers).  Not enough memory");
	}
	LEAVE;

	return (surface);
}

/***************************************************************************//**
 * Destroys the polyline.
 */
int DESTROY(GT_surface_vertex_buffers)(GT_surface_vertex_buffers **surface)
{
	int return_code;

	ENTER(DESTROY(GT_surface_vertex_buffers));
	if (surface && *surface)
	{
		DEALLOCATE(*surface);
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"DESTROY(GT_surface_vertex_buffers).  "
			"Invalid argument");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(GT_polyline_vertex_buffers) */

GT_glyphset_vertex_buffers *CREATE(GT_glyphset_vertex_buffers)()
{
	struct GT_glyphset_vertex_buffers *glyphset;

	if (ALLOCATE(glyphset,struct GT_glyphset_vertex_buffers,1))
	{
		glyphset->glyph = (GT_object *)NULL;
		glyphset->font = (cmzn_font *)NULL;
		for (int i = 0; i < 3; ++i)
		{
			glyphset->base_size[i] = 0;
			glyphset->scale_factors[i] = 0;
			glyphset->offset[i] = 0;
			glyphset->label_offset[i] = 0;
			glyphset->static_label_text[i] = 0;
		}
		glyphset->label_bounds_dimension = 0;
		glyphset->label_bounds_components = 0;
	}
	else
	{
		display_message(ERROR_MESSAGE,"CREATE(GT_glyphset_vertex_buffers).  Not enough memory");
	}

	return (glyphset);
} /* CREATE(GT_glyphset_vertex_buffers) */

GT_pointset_vertex_buffers *CREATE(GT_pointset_vertex_buffers)(
	struct cmzn_font *font, gtMarkerType marker_type,
	ZnReal marker_size)
{
	struct GT_pointset_vertex_buffers *pointset;

	if (ALLOCATE(pointset,struct GT_pointset_vertex_buffers,1))
	{
		if (font)
			pointset->font = ACCESS(cmzn_font)(font);
		else
			pointset->font = (cmzn_font *)NULL;
		pointset->marker_type=marker_type;
		pointset->marker_size=marker_size;
	}
	else
	{
		display_message(ERROR_MESSAGE,"CREATE(GT_pointset_vertex_buffers).  Not enough memory");
	}

	return (pointset);
} /* CREATE(GT_pointset_vertex_buffers) */

int DESTROY(GT_glyphset_vertex_buffers)(GT_glyphset_vertex_buffers **glyphset_address)
{
	int return_code = 0;
	GT_glyphset_vertex_buffers *glyphset = 0;
	if (glyphset_address && (glyphset = (*glyphset_address)))
	{
		DEACCESS(GT_object)(&(glyphset->glyph));
		for (int i = 0; i < 3; i++)
		{
			if (glyphset->static_label_text[i])
			{
				DEALLOCATE(glyphset->static_label_text[i]);
			}
		}
		if (glyphset->font)
		{
			DEACCESS(cmzn_font)(&(glyphset->font));
		}
		if (glyphset->glyph)
			 DEACCESS(GT_object)(&(glyphset->glyph));
		if (glyphset->font)
			 DEACCESS(cmzn_font)(&(glyphset->font));
		DEALLOCATE(*glyphset_address);
		return_code=1;
	}

	return (return_code);
} /* DESTROY(GT_glyphset_vertex_buffers) */

int DESTROY(GT_pointset_vertex_buffers)(GT_pointset_vertex_buffers **pointset)
{
	int return_code;

	if (pointset && *pointset)
	{
		if ((*pointset)->font)
			DEACCESS(cmzn_font)(&((*pointset)->font));
		DEALLOCATE(*pointset);
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"DESTROY(GT_pointset_vertex_buffers).  "
			"Invalid argument");
		return_code=0;
	}

	return (return_code);
} /* DESTROY(GT_pointset_vertex_buffers) */

struct GT_object *CREATE(GT_object)(const char *name,enum GT_object_type object_type,
	cmzn_material *default_material)
{
	struct GT_object *object;
	int return_code;

	ENTER(CREATE(GT_object));
	if (name)
	{
		if (ALLOCATE(object, gtObject, 1) &&
			(object->name = duplicate_string(name)))
		{
			object->select_mode=CMZN_GRAPHICS_SELECT_MODE_OFF;
			object->times = (ZnReal *)NULL;
			object->primitive_lists = (union GT_primitive_list *)NULL;
			object->glyph_labels_function = (Graphics_object_glyph_labels_function)NULL;
			object->glyph_type = CMZN_GLYPH_SHAPE_TYPE_INVALID;
			object->texture_tiling = (struct Texture_tiling *)NULL;
			object->vertex_array = (Graphics_vertex_array *)NULL;
			object->access_count = 1;
			return_code = 1;
			switch (object_type)
			{
				case g_SURFACE_VERTEX_BUFFERS:
				case g_POLYLINE_VERTEX_BUFFERS:
				case g_GLYPH_SET_VERTEX_BUFFERS:
				case g_POINT_SET_VERTEX_BUFFERS:
				{
					object->vertex_array = new Graphics_vertex_array
						(GRAPHICS_VERTEX_ARRAY_TYPE_FLOAT_SEPARATE_DRAW_ARRAYS);
				} break;
				default:
				{
					display_message(ERROR_MESSAGE,
						"CREATE(GT_object).  Unknown object type");
					return_code= -1;
				} break;
			}
			if (0<return_code)
			{
#if defined (OPENGL_API)
				object->buffer_binding = 0;
				object->display_list = 0;
				object->position_vertex_buffer_object = 0;
				object->position_values_per_vertex = 0;
				object->colour_vertex_buffer_object = 0;
				object->colour_values_per_vertex = 0;
				object->normal_vertex_buffer_object = 0;
				object->texture_coordinate0_vertex_buffer_object = 0;
				object->tangent_vertex_buffer_object = 0;
				object->index_vertex_buffer_object = 0;
				object->vertex_array_object = 0;
				object->multipass_width = 0;
				object->multipass_height = 0;
				object->multipass_vertex_buffer_object = 0;
				object->multipass_frame_buffer_object = 0;
				object->multipass_frame_buffer_texture = 0;
#endif /* defined (OPENGL_API) */
				object->compile_status = GRAPHICS_NOT_COMPILED;
				object->object_type=object_type;
				if (default_material)
				{
					object->default_material=cmzn_material_access(default_material);
				}
				else
				{
					object->default_material=(cmzn_material *)NULL;
				}
				object->selected_material=(cmzn_material *)NULL;
				object->secondary_material=(cmzn_material *)NULL;
				object->nextobject=(gtObject *)NULL;
				object->spectrum=(struct cmzn_spectrum *)NULL;
				object->number_of_times=0;
				object->render_line_width = 0.0; // not set: inherit from current state
				object->render_point_size = 0.0; // not set: inherit from current state
			}
			else
			{
				if (0==return_code)
				{
					display_message(ERROR_MESSAGE,
						"CREATE(GT_object).  Insufficient memory");
				}
				DEALLOCATE(object->name);
				DEALLOCATE(object);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,"CREATE(GT_object).  Insufficient memory");
			if (object)
			{
				if (object->name)
				{
					DEALLOCATE(object->name);
				}
				DEALLOCATE(object);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"CREATE(GT_object).  Invalid argument(s)");
		object=(gtObject *)NULL;
	}
	LEAVE;

	return (object);
}

/**
 * Frees the memory for the fields of <**object>, frees the memory for <**object>
 * and sets <*object> to NULL.
 */
static int DESTROY(GT_object)(struct GT_object **object_ptr)
{
	int return_code;
	struct GT_object *object;

	ENTER(DESTROY(GT_object));
	if (object_ptr&&(object= *object_ptr))
	{
		if (0!=object->access_count)
		{
			display_message(ERROR_MESSAGE,
				"DESTROY(GT_object).  Access count = %d",object->access_count);
			return_code=0;
		}
		else
		{
			GT_object_destroy_primitives(object);
			if (object->vertex_array)
				object->vertex_array->clear_buffers();
			DEALLOCATE(object->name);
			if (object->default_material)
			{
				cmzn_material_destroy(&(object->default_material));
			}
			if (object->selected_material)
			{
				cmzn_material_destroy(&(object->selected_material));
			}
			if (object->secondary_material)
			{
				cmzn_material_destroy(&(object->secondary_material));
			}
			if (object->spectrum)
			{
				DEACCESS(cmzn_spectrum)(&(object->spectrum));
			}
			if (object->vertex_array)
			{
				delete object->vertex_array;
			}
			if (object->texture_tiling)
			{
				DEACCESS(Texture_tiling)(&object->texture_tiling);
			}
#if defined (OPENGL_API)
			if (object->display_list)
			{
				glDeleteLists(object->display_list,1);
			}
#if defined (GL_VERSION_1_5)
			/* Assume this function is available the vertex buffer object
			 * values are set */
			if (object->position_vertex_buffer_object)
			{
				glDeleteBuffers(1, &object->position_vertex_buffer_object);
			}
			if (object->colour_vertex_buffer_object)
			{
				glDeleteBuffers(1, &object->colour_vertex_buffer_object);
			}
			if (object->normal_vertex_buffer_object)
			{
				glDeleteBuffers(1, &object->normal_vertex_buffer_object);
			}
			if (object->texture_coordinate0_vertex_buffer_object)
			{
				glDeleteBuffers(1, &object->texture_coordinate0_vertex_buffer_object);
			}
			if (object->tangent_vertex_buffer_object)
			{
				glDeleteBuffers(1, &object->tangent_vertex_buffer_object);
			}
			if (object->index_vertex_buffer_object)
			{
				glDeleteBuffers(1, &object->index_vertex_buffer_object);
			}
			if (object->multipass_vertex_buffer_object)
			{
				glDeleteBuffers(1, &object->multipass_vertex_buffer_object);
			}
			if (object->multipass_frame_buffer_object)
			{
#if defined GL_EXT_framebuffer_object
				glDeleteFramebuffersEXT(1, &object->multipass_frame_buffer_object);
#endif // defined GL_EXT_framebuffer_object
			}
			if (object->multipass_frame_buffer_texture)
			{
				glDeleteTextures(1, &object->multipass_frame_buffer_texture);
			}
#endif /* defined (GL_VERSION_1_5) */
#if defined (GL_VERSION_3_0)  && (NOT_IN_MESA_HEADER_YET)
			if (object->vertex_array_object)
			{
				/* Assume this is safe as the vertex array object is set */
				glDeleteVertexArrays(1, &object->vertex_array_object);
			}
#endif /* defined (GL_VERSION_3_0) */
#endif /* defined (OPENGL_API) */
			/* DEACCESS ptrnext so that objects attached in linked-list may be
				 destroyed. Note that this means they should have been accessed! */
			if (object->nextobject)
			{
				DEACCESS(GT_object)(&(object->nextobject));
			}
			DEALLOCATE(*object_ptr);
			return_code=1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"DESTROY(GT_object).  Invalid arguments");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(GT_object) */

DECLARE_OBJECT_FUNCTIONS(GT_object)

DECLARE_DEFAULT_GET_OBJECT_NAME_FUNCTION(GT_object)

void GT_object_changed(struct GT_object *graphics_object)
{
	while (graphics_object)
	{
		graphics_object->compile_status = GRAPHICS_NOT_COMPILED;
		graphics_object = graphics_object->nextobject;
	}
}

int GT_object_Graphical_material_change(struct GT_object *graphics_object,
	struct LIST(cmzn_material) *changed_material_list)
/*******************************************************************************
LAST MODIFIED : 20 March 2002

DESCRIPTION :
Tells the <graphics_object> that the materials in the <changed_material_list>
have changed. If any of these materials are used in any graphics object,
changes the compile_status to CHILD_GRAPHICS_NOT_COMPILED and
informs clients of the need to recompile and redraw. Note that if a spectrum is
in use the more expensive GRAPHICS_NOT_COMPILED status is necessarily set.
Note: Passing a NULL <changed_material_list> indicates the equivalent of a
change to any material in use in the linked graphics objects.
==============================================================================*/
{
	int return_code;

	ENTER(GT_object_Graphical_material_change);
	if (graphics_object)
	{
		while (graphics_object)
		{
			if ((graphics_object->default_material &&
				((!changed_material_list) || IS_OBJECT_IN_LIST(cmzn_material)(
					graphics_object->default_material, changed_material_list))) ||
				(graphics_object->selected_material &&
					((!changed_material_list) || IS_OBJECT_IN_LIST(cmzn_material)(
						graphics_object->selected_material, changed_material_list))) ||
				(graphics_object->secondary_material &&
					((!changed_material_list) || IS_OBJECT_IN_LIST(cmzn_material)(
						graphics_object->secondary_material, changed_material_list))))
			{
				if (graphics_object->spectrum)
				{
					/* need to rebuild display list when spectrum in use */
					graphics_object->compile_status = GRAPHICS_NOT_COMPILED;
				}
				else if (((graphics_object->object_type == g_POLYLINE_VERTEX_BUFFERS) ||
					(graphics_object->object_type == g_SURFACE_VERTEX_BUFFERS) ||
					(graphics_object->object_type == g_POINT_SET_VERTEX_BUFFERS) ||
					(graphics_object->object_type == g_GLYPH_SET_VERTEX_BUFFERS)) &&
					(graphics_object->secondary_material))
				{
					/* vertex positions are calculated from the secondary material so need to be updated */
					graphics_object->compile_status = GRAPHICS_NOT_COMPILED;
				}
				else
				{
					if (GRAPHICS_NOT_COMPILED != graphics_object->compile_status)
					{
						graphics_object->compile_status = CHILD_GRAPHICS_NOT_COMPILED;
					}
				}
			}
			graphics_object = graphics_object->nextobject;
		}
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"GT_object_Graphical_material_change.  Invalid graphics object");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* GT_object_Graphical_material_change */

int GT_object_Spectrum_change(struct GT_object *graphics_object,
	struct LIST(cmzn_spectrum) *changed_spectrum_list)
/*******************************************************************************
LAST MODIFIED : 12 March 2002

DESCRIPTION :
Tells the <graphics_object> that the spectrums in the <changed_spectrum_list>
have changed. If any of these spectrums are used in any graphics object,
changes the compile_status to GRAPHICS_NOT_COMPILED and
informs clients of the need to recompile and redraw.
Note: Passing a NULL <changed_spectrum_list> indicates the equivalent of a
change to any spectrum in use in the linked graphics objects.
==============================================================================*/
{
	int return_code;

	ENTER(GT_object_Spectrum_change);
	if (graphics_object)
	{
		while (graphics_object)
		{
			if (graphics_object->spectrum &&
				((!changed_spectrum_list) || IS_OBJECT_IN_LIST(cmzn_spectrum)(
					graphics_object->spectrum, changed_spectrum_list)))
			{
				/* need to rebuild display list when spectrum in use */
				graphics_object->compile_status = GRAPHICS_NOT_COMPILED;
			}
			graphics_object = graphics_object->nextobject;
		}
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"GT_object_Spectrum_change.  Invalid graphics object");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* GT_object_Spectrum_change */

int GT_object_has_time(struct GT_object *graphics_object,ZnReal time)
/*******************************************************************************
LAST MODIFIED : 19 June 1998

DESCRIPTION :
Returns 1 if the time parameter is used by the graphics_object.
==============================================================================*/
{
	int return_code;

	ENTER(GT_object_has_time);
	/* check arguments */
	if (graphics_object)
	{
		return_code=(0<GT_object_get_time_number(graphics_object,time));
	}
	else
	{
		display_message(ERROR_MESSAGE,"GT_object_has_time.  Invalid arguments");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* GT_object_has_time */

int GT_object_get_number_of_times(struct GT_object *graphics_object)
{
	if (graphics_object)
		return graphics_object->number_of_times;
	return 0;
}

void GT_object_time_change(struct GT_object *graphics_object)
{
	if (graphics_object)
	{
		//if ((g_GLYPH_SET == graphics_object->object_type) && graphics_object->primitive_lists)
		//{
			// assume only one time and one glyph
		//	GT_glyph_set *glyph_set = graphics_object->primitive_lists[0].gt_glyph_set.first;
		//	GT_object_time_change(glyph_set->glyph);
		//}
	}
	if (1 < graphics_object->number_of_times)
		GT_object_changed(graphics_object);
}

Graphics_vertex_array *
	GT_object_get_vertex_set(GT_object *graphics_object)
{
	Graphics_vertex_array *set;

	ENTER(GT_object_get_vertex_set);
	if (graphics_object)
	{
		set=graphics_object->vertex_array;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"GT_object_get_vertex_set.  Invalid arguments");
		set = (Graphics_vertex_array *)NULL;
	}
	LEAVE;

	return (set);
} /* GT_object_get_vertex_set */

ZnReal GT_object_get_time(struct GT_object *graphics_object,int time_number)
/*******************************************************************************
LAST MODIFIED : 18 June 1998

DESCRIPTION :
Returns the time at <time_number> from the graphics_object.
Note that time numbers range from 1 to number_of_times.
==============================================================================*/
{
	ZnReal return_time,*times;

	ENTER(GT_object_get_time);
	if (graphics_object)
	{
		if ((0 <= time_number)&&(time_number < graphics_object->number_of_times))
		{
			times=graphics_object->times;
			if (times)
			{
				return_time=times[time_number-1];
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"GT_object_get_time.  Invalid times array");
				return_time=0.0;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"GT_object_get_time.  Time number out of range");
/*???debug */
printf("GT_object_get_time.  Time number out of range in GT_object %s\n",
	graphics_object->name);
			return_time=0.0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"GT_object_get_time.  Invalid argument(s)");
		return_time=0.0;
	}
	LEAVE;

	return (return_time);
} /* GT_object_get_time */

ZnReal GT_object_get_nearest_time(struct GT_object *graphics_object,ZnReal time)
/*******************************************************************************
LAST MODIFIED : 7 July 1998

DESCRIPTION :
Returns the nearest time to <time> in <graphics_object> at which graphics
primitives are called.
NOTE: presently finds the nearest time that is *lower* than <time>. When all
routines updated to use this, may be changed to get actual nearest time.
???RC modularise; get it to call a routine get_nearest time_number
???RC need another version to handle morphing/proportions?
==============================================================================*/
{
	ZnReal return_time,*times;
	int time_number;

	ENTER(GT_object_get_nearest_time);
	if (graphics_object)
	{
		if (0<(time_number=graphics_object->number_of_times))
		{
			times=graphics_object->times;
			if (times)
			{
				time_number--;
				times += time_number;
				while ((time_number>0)&&(time< *times))
				{
					times--;
					time_number--;
				}
				return_time=times[time_number];
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"GT_object_get_nearest_time.  Invalid times array");
				return_time=0.0;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"GT_object_get_nearest_time.  No times defined for graphics object");
			return_time=0.0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"GT_object_get_nearest_time.  Invalid argument(s)");
		return_time=0.0;
	}
	LEAVE;

	return (return_time);
} /* GT_object_get_nearest_time */

int get_graphics_object_range(struct GT_object *graphics_object,
	void *graphics_object_range_void)
/*******************************************************************************
LAST MODIFIED : 17 March 2003

DESCRIPTION :
Returns the range of the coordinates in <graphics_object> or 0 if object empty
or error occurs. First should be set to 1 outside routine. Several calls to
this routine for differing graphics objects (without settings first in between)
will produce the range of all the graphics objects.
???RC only does some object types.
???RC this could be cleaned up by getting primitives to find their own range.
==============================================================================*/
{
	GLfloat *maximum,*minimum;
	int *first, j, number_of_times = 0,	return_code = 0;
	struct Graphics_object_range_struct *graphics_object_range;
	union GT_primitive_list *primitive_list;

	if (graphics_object&&(graphics_object_range=
		(struct Graphics_object_range_struct *)graphics_object_range_void)&&
		(maximum = graphics_object_range->maximum)&&
		(minimum = graphics_object_range->minimum)&&
		(first=&(graphics_object_range->first)))
	{
		return_code = 1;
		number_of_times = graphics_object->number_of_times;
		if ((0 == number_of_times) || graphics_object->primitive_lists)
		{
			for (j = 0; (j < number_of_times) && return_code; j++)
			{
				primitive_list = graphics_object->primitive_lists + j;
				switch (graphics_object->object_type)
				{
					case g_GLYPH_SET_VERTEX_BUFFERS:
					{
						struct GT_glyphset_vertex_buffers *gt_glyphset_vertex_buffers =
							 primitive_list->gt_glyphset_vertex_buffers;
						unsigned int position_values_per_vertex = 0, position_vertex_count = 0,
							axis1_values_per_vertex = 0, axis1_vertex_count = 0,
							axis2_values_per_vertex = 0, axis2_vertex_count = 0,
							axis3_values_per_vertex = 0, axis3_vertex_count = 0,
							scale_values_per_vertex = 0, scale_vertex_count = 0;
						GLfloat *position_buffer = 0, *axis1_buffer = 0,
							*axis2_buffer = 0, *axis3_buffer = 0, *scale_buffer = 0;
						graphics_object->vertex_array->get_float_vertex_buffer(
							GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
							&position_buffer, &position_values_per_vertex, &position_vertex_count);
						graphics_object->vertex_array->get_float_vertex_buffer(
							GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_AXIS1,
							&axis1_buffer, &axis1_values_per_vertex, &axis1_vertex_count);
						graphics_object->vertex_array->get_float_vertex_buffer(
							GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_AXIS2,
							&axis2_buffer, &axis2_values_per_vertex, &axis2_vertex_count);
						graphics_object->vertex_array->get_float_vertex_buffer(
							GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_AXIS3,
							&axis3_buffer, &axis3_values_per_vertex, &axis3_vertex_count);
						graphics_object->vertex_array->get_float_vertex_buffer(
							GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_SCALE,
							&scale_buffer, &scale_values_per_vertex, &scale_vertex_count);
						Triple temp_axis1, temp_axis2, temp_axis3, temp_point;
						for (unsigned int i = 0; i < position_vertex_count; ++i)
						{
							resolve_glyph_axes(gt_glyphset_vertex_buffers->glyph_repeat_mode, /*glyph_number*/0,
								gt_glyphset_vertex_buffers->base_size, gt_glyphset_vertex_buffers->scale_factors,
								gt_glyphset_vertex_buffers->offset,
								position_buffer, axis1_buffer, axis2_buffer, axis3_buffer, scale_buffer,
								temp_point, temp_axis1, temp_axis2, temp_axis3);
							if (*first)
							{
								for (int k = 0; k < 3; ++k)
								{
									maximum[k] = minimum[k] = temp_point[k];
								}
								*first = 0;
							}
							else
							{
								for (int k = 0; k < 3; ++k)
								{
									if (temp_point[k] < minimum[k])
									{
										minimum[k] = temp_point[k];
									}
									else if (temp_point[k] > maximum[k])
									{
										maximum[k] = temp_point[k];
									}
								}
							}
							position_buffer += position_values_per_vertex;
							axis1_buffer += axis1_values_per_vertex;
							axis2_buffer += axis2_values_per_vertex;
							axis3_buffer += axis3_values_per_vertex;
							scale_buffer += scale_values_per_vertex;
						}
					}break;
					case g_SURFACE_VERTEX_BUFFERS:
					case g_POLYLINE_VERTEX_BUFFERS:
					case g_POINT_SET_VERTEX_BUFFERS:
					{
						GLfloat *vertex_buffer;
						unsigned int values_per_vertex, vertex_count;
						if (graphics_object->vertex_array->get_float_vertex_buffer(
							GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
							&vertex_buffer, &values_per_vertex, &vertex_count))
						{
							if (*first)
							{
								for (unsigned int i = 0 ; i < values_per_vertex ; i++)
									minimum[i] = maximum[i] = vertex_buffer[i];
								--vertex_count;
								vertex_buffer += values_per_vertex;
								*first=0;
							}
							while (vertex_count>0)
							{
								for (unsigned int i = 0 ; i < values_per_vertex ; i++)
								{
									if (vertex_buffer[i] < minimum[i])
										minimum[i] = vertex_buffer[i];
									else if (vertex_buffer[i] > maximum[i])
										maximum[i] = vertex_buffer[i];
								}
								--vertex_count;
								vertex_buffer += values_per_vertex;
							}
						}
					} break;
					default:
					{
						display_message(ERROR_MESSAGE,
							"get_graphics_object_range.  Invalid graphics object type");
						return_code = 0;
					} break;
				}
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"get_graphics_object_range.  Invalid primitive_lists");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"get_graphics_object_range.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* get_graphics_object_range */

int get_graphics_object_data_range(struct GT_object *graphics_object,
	Graphics_object_data_range *range)
{
	if (graphics_object && range)
	{
		int number_of_times = graphics_object->number_of_times;
		if ((0 == number_of_times) || graphics_object->primitive_lists)
		{
			for (int j = 0; j < number_of_times; ++j)
			{
				switch (graphics_object->object_type)
				{
					case g_SURFACE_VERTEX_BUFFERS:
					case g_POLYLINE_VERTEX_BUFFERS:
					case g_GLYPH_SET_VERTEX_BUFFERS:
					case g_POINT_SET_VERTEX_BUFFERS:
					{
						GLfloat *vertex_buffer = NULL;
						unsigned int values_per_vertex = 0, vertex_count = 0;
						graphics_object->vertex_array->get_float_vertex_buffer(
							GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
							&vertex_buffer, &values_per_vertex, &vertex_count);
						while (vertex_count>0)
						{
							range->addValues(values_per_vertex, vertex_buffer);
							--vertex_count;
							vertex_buffer += values_per_vertex;
						}
					} break;
					default:
					{
						display_message(ERROR_MESSAGE,
							"get_graphics_object_data_range.  "
							"Invalid graphics object type");
						return 0;
					} break;
				}
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"get_graphics_object_data_range.  Invalid primitive_lists");
			return 0;
		}
		return 1;
	}
	return 0;
}

int get_graphics_object_time_range(struct GT_object *graphics_object,
	void *graphics_object_time_range_void)
/*******************************************************************************
LAST MODIFIED : 19 June 1998

DESCRIPTION :
Enlarges the minimum and maximum time range by that of the graphics_object.
==============================================================================*/
{
	ZnReal *times;
	int return_code;
	struct Graphics_object_time_range_struct *graphics_object_time_range;

	ENTER(get_graphics_object_time_range);
	if (graphics_object&&(graphics_object_time_range=(struct
		Graphics_object_time_range_struct *)graphics_object_time_range_void))
	{
		return_code=1;
		if (0 < graphics_object->number_of_times)
		{
			times=graphics_object->times;
			if (times)
			{
				if (graphics_object_time_range->first)
				{
					graphics_object_time_range->minimum=times[0];
					graphics_object_time_range->maximum=
						times[graphics_object->number_of_times-1];
					graphics_object_time_range->first=0;
				}
				else
				{
					if (times[0] < graphics_object_time_range->minimum)
					{
						graphics_object_time_range->minimum = times[0];
					}
					if (times[graphics_object->number_of_times-1] >
						graphics_object_time_range->maximum)
					{
						graphics_object_time_range->maximum =
							times[graphics_object->number_of_times-1];
					}
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"GT_object_get_time_range.  Invalid times array");
				return_code=0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"get_graphics_object_time_range.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* get_graphics_object_time_range */

#define DECLARE_GT_OBJECT_ADD_FUNCTION(primitive_type, \
	gt_object_type,primitive_var) \
PROTOTYPE_GT_OBJECT_ADD_FUNCTION(primitive_type) \
/***************************************************************************** \
LAST MODIFIED : 20 May 2003 \
\
DESCRIPTION : \
Adds <primitive> to <graphics_object> at <time>, creating the new time if it \
does not already exist. If the <primitive> is NULL an empty time is added if \
there is not already one. <primitive> is a NULL-terminated linked-list. \
============================================================================*/ \
{ \
	ZnReal *times; \
	int return_code, time_number, i; \
	union GT_primitive_list *primitive_list; \
\
	ENTER(GT_OBJECT_ADD(primitive_type)); \
	if (graphics_object && (gt_object_type == graphics_object->object_type)) \
	{ \
		return_code = 1; \
		time_number= \
			GT_object_get_less_than_or_equal_time_number(graphics_object,time); \
		if ((0 < time_number) && (time==graphics_object->times[time_number-1])) \
		{ \
			primitive_list = graphics_object->primitive_lists + time_number - 1; \
		} \
		else \
		{ \
			/* make a new time & primitive_list */ \
			if (REALLOCATE(times, graphics_object->times, ZnReal, \
				graphics_object->number_of_times + 1)) \
			{ \
				graphics_object->times = times; \
				if (REALLOCATE(primitive_list, (graphics_object->primitive_lists), \
					union GT_primitive_list, graphics_object->number_of_times + 1)) \
				{ \
					(graphics_object->primitive_lists) = primitive_list; \
					times += graphics_object->number_of_times; \
					primitive_list += graphics_object->number_of_times; \
					(graphics_object->number_of_times)++; \
					for (i = (graphics_object->number_of_times)-time_number-1; i>0; i--) \
					{ \
						times--; \
						times[1] = times[0]; \
						primitive_list--; \
						primitive_list[1].primitive_var.first = primitive_list->primitive_var.first; \
						primitive_list[1].primitive_var.last = primitive_list->primitive_var.last; \
					} \
					*times = time; \
					primitive_list->primitive_var.first = (struct primitive_type *)NULL; \
					primitive_list->primitive_var.last = (struct primitive_type *)NULL; \
				} \
				else \
				{ \
					return_code = 0; \
				} \
			} \
			else \
			{ \
				return_code = 0; \
			} \
		} \
		if (return_code) \
		{ \
			/* note this adds primitives to the end of the list; used to be start */ \
			if (primitive_list->primitive_var.last) \
			{ \
				primitive_list->primitive_var.last->ptrnext = primitive; \
			} \
			else \
			{ \
				primitive_list->primitive_var.first = primitive; \
				primitive_list->primitive_var.last = primitive; \
			} \
			/* advance last pointer to last primitive in linked-list */ \
			if (primitive_list->primitive_var.last) \
			{ \
				while (primitive_list->primitive_var.last->ptrnext) \
				{ \
					primitive_list->primitive_var.last = \
						primitive_list->primitive_var.last->ptrnext; \
				} \
			} \
			GT_object_changed(graphics_object); \
		} \
		else \
		{ \
			display_message(ERROR_MESSAGE, \
				"GT_OBJECT_ADD(" #primitive_type ").  Failed"); \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"GT_OBJECT_ADD(" #primitive_type ").  Invalid arguments"); \
		return_code = 0; \
	} \
	LEAVE; \
\
	return (return_code); \
} /* GT_OBJECT_ADD(primitive_type) */

int GT_OBJECT_ADD(GT_polyline_vertex_buffers)(
	struct GT_object *graphics_object, struct GT_polyline_vertex_buffers *primitive)
{
	int return_code = 0;

	if (graphics_object && (g_POLYLINE_VERTEX_BUFFERS == graphics_object->object_type) &&
		!graphics_object->primitive_lists)
	{
		if (ALLOCATE(graphics_object->primitive_lists, union GT_primitive_list, 1) &&
			ALLOCATE(graphics_object->times, ZnReal, 1))
		{
			graphics_object->primitive_lists->gt_polyline_vertex_buffers = primitive;
			graphics_object->times[0] = 0.0;
			graphics_object->number_of_times = 1;
			graphics_object->buffer_binding = 1;
			return_code = 1;
		}
	}
	else
	{
		return_code = 0;
	}
	return (return_code);
}

int GT_OBJECT_ADD(GT_surface_vertex_buffers)(
	struct GT_object *graphics_object, struct GT_surface_vertex_buffers *primitive)
{
	int return_code = 0;

	if (graphics_object && (g_SURFACE_VERTEX_BUFFERS == graphics_object->object_type) &&
		!graphics_object->primitive_lists)
	{
		if (ALLOCATE(graphics_object->primitive_lists, union GT_primitive_list, 1) &&
			ALLOCATE(graphics_object->times, ZnReal, 1))
		{
			graphics_object->primitive_lists->gt_surface_vertex_buffers = primitive;
			graphics_object->times[0] = 0.0;
			graphics_object->number_of_times = 1;
			graphics_object->buffer_binding = 1;
			return_code = 1;
		}
	}
	else
	{
		return_code = 0;
	}
	return (return_code);
}

int GT_OBJECT_ADD(GT_glyphset_vertex_buffers)(
	struct GT_object *graphics_object, struct GT_glyphset_vertex_buffers *primitive)
{
	int return_code = 0;

	if (graphics_object && (g_GLYPH_SET_VERTEX_BUFFERS == graphics_object->object_type) &&
		(!graphics_object->primitive_lists))
	{
		if (ALLOCATE(graphics_object->primitive_lists, union GT_primitive_list, 1) &&
			ALLOCATE(graphics_object->times, ZnReal, 1))
		{
			graphics_object->primitive_lists->gt_glyphset_vertex_buffers = primitive;
			graphics_object->times[0] = 0.0;
			graphics_object->number_of_times = 1;
			graphics_object->buffer_binding = 1;
			return_code = 1;
		}
	}
	else
	{
		return_code = 0;
	}
	return (return_code);
}

int GT_OBJECT_ADD(GT_pointset_vertex_buffers)(
	struct GT_object *graphics_object, struct GT_pointset_vertex_buffers *primitive)
{
	int return_code = 0;

	if (graphics_object && (g_POINT_SET_VERTEX_BUFFERS == graphics_object->object_type) &&
		!graphics_object->primitive_lists)
	{
		if (ALLOCATE(graphics_object->primitive_lists, union GT_primitive_list, 1) &&
			ALLOCATE(graphics_object->times, ZnReal, 1))
		{
			graphics_object->primitive_lists->gt_pointset_vertex_buffers = primitive;
			graphics_object->times[0] = 0.0;
			graphics_object->number_of_times = 1;
			graphics_object->buffer_binding = 1;
			return_code = 1;
		}
	}
	else
	{
		return_code = 0;
	}
	return (return_code);
}

void GT_object_reset_buffer_binding(struct GT_object *graphics_object)
{
	if (graphics_object)
		graphics_object->buffer_binding = 1;
}

#define DECLARE_GT_OBJECT_GET_FUNCTION(primitive_type, \
	gt_object_type,primitive_var) \
PROTOTYPE_GT_OBJECT_GET_FUNCTION(primitive_type) \
/***************************************************************************** \
LAST MODIFIED : 17 March 2003 \
\
DESCRIPTION : \
Returns pointer to the primitive at the given time in graphics_object. \
============================================================================*/ \
{ \
	struct primitive_type *primitive; \
	int time_number; \
\
	ENTER(GT_OBJECT_GET(primitive_type)); \
	primitive = (struct primitive_type *)NULL; \
	if (graphics_object&&(gt_object_type==graphics_object->object_type)) \
	{ \
		if (0 < (time_number = GT_object_get_time_number(graphics_object,time))) \
		{ \
			if (graphics_object->primitive_lists) \
			{ \
				primitive = graphics_object->primitive_lists[time_number - 1]. \
					primitive_var.first; \
			} \
			else \
			{ \
				display_message(ERROR_MESSAGE, \
					"GT_OBJECT_GET(" #primitive_type ").  Invalid primitive_lists"); \
			} \
		} \
		else \
		{ \
			display_message(ERROR_MESSAGE, \
				"GT_OBJECT_GET(" #primitive_type ").  No primitives at time %g",time); \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"GT_OBJECT_GET(" #primitive_type ").  Invalid arguments"); \
	} \
	LEAVE; \
\
	return (primitive); \
} /* GT_OBJECT_GET(primitive_type) */

struct GT_glyphset_vertex_buffers *GT_object_get_GT_glyphset_vertex_buffers(
	struct GT_object *graphics_object)
{
	if (graphics_object && (g_GLYPH_SET_VERTEX_BUFFERS == graphics_object->object_type))
	{
		if (graphics_object->primitive_lists)
			return graphics_object->primitive_lists->gt_glyphset_vertex_buffers;
	}
	else
		display_message(ERROR_MESSAGE, "GT_object_get_GT_glyphset_vertex_buffers.  Invalid arguments");
	return 0;
}

int GT_object_invalidate_selected_primitives(struct GT_object *graphics_object,
	DsLabelsChangeLog *changeLog)
{
	if (!(graphics_object && changeLog))
		return 0;
	switch (graphics_object->object_type)
	{
		case g_POINT_SET_VERTEX_BUFFERS:
			return 1;
			break;
		case g_POLYLINE_VERTEX_BUFFERS:
		case g_SURFACE_VERTEX_BUFFERS:
		case g_GLYPH_SET_VERTEX_BUFFERS:
			GT_object_destroy_primitives(graphics_object);
			GT_object_mark_vertex_array_primitives_changes(graphics_object, changeLog);
			GT_object_changed(graphics_object);
			return 1;
			break;
		default:
			display_message(ERROR_MESSAGE, "GT_object_invalidate_selected_primitives.  Unknown object type");
			break;
	}
	return 0;
}

int GT_object_clear_primitives(struct GT_object *graphics_object)
{
	if (graphics_object)
	{
		GT_object_destroy_primitives(graphics_object);
		if (graphics_object->vertex_array)
			graphics_object->vertex_array->clear_buffers();
		GT_object_changed(graphics_object);
		return 1;
	}
	display_message(ERROR_MESSAGE, "GT_object_clear_primitives.  Invalid argument(s)");
	return 0;
}

#if ! defined (SHORT_NAMES)
#define GT_OBJECT_TRANSFER_PRIMITIVES_AT_TIME_NUMBER_(primitive_type) \
	GT_object_transfer_primitives_at_time_number_ ## primitive_type
#else
#define GT_OBJECT_TRANSFER_PRIMITIVES_AT_TIME_NUMBER_(primitive_type) \
	gotptn_ ## primitive_type
#endif
#define GT_OBJECT_TRANSFER_PRIMITIVES_AT_TIME_NUMBER(primitive_type) \
	GT_OBJECT_TRANSFER_PRIMITIVES_AT_TIME_NUMBER_(primitive_type)

#define DECLARE_GT_OBJECT_TRANSFER_PRIMITIVES_AT_TIME_NUMBER_FUNCTION( \
	primitive_type,gt_object_type,primitive_var) \
static int GT_OBJECT_TRANSFER_PRIMITIVES_AT_TIME_NUMBER(primitive_type)( \
	struct GT_object *destination, struct GT_object *source, \
	int time_number, ZnReal time) \
/***************************************************************************** \
LAST MODIFIED : 18 March 2003 \
\
DESCRIPTION : \
Transfers the primitives stored at <time_number> in <source> to <time> in \
<destination>. Primitives are added after any in <destination> at <time>. \
============================================================================*/ \
{ \
	int return_code; \
	struct primitive_type *primitive; \
	union GT_primitive_list *primitive_list; \
\
	ENTER(GT_OBJECT_TRANSFER_PRIMITIVES_AT_TIME_NUMBER(primitive_type)); \
	return_code = 0; \
	if (destination && (gt_object_type == destination->object_type) && \
		source && (gt_object_type == source->object_type) && \
		source->primitive_lists && \
		(0 < time_number) && (time_number <= source->number_of_times)) \
	{ \
		primitive_list = source->primitive_lists + time_number - 1; \
		primitive = primitive_list->primitive_var.first; \
		if (GT_OBJECT_ADD(primitive_type)(destination, time, primitive)) \
		{ \
			primitive_list->primitive_var.first = (struct primitive_type *)NULL; \
			primitive_list->primitive_var.last = (struct primitive_type *)NULL; \
			GT_object_changed(source); \
			return_code = 1; \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"GT_OBJECT_TRANSFER_PRIMITIVES_AT_TIME_NUMBER(" #primitive_type \
			").  Invalid arguments"); \
	} \
	LEAVE; \
\
	return (return_code); \
} /* GT_OBJECT_TRANSFER_PRIMITIVES_AT_TIME_NUMBER(primitive_type) */

#define DECLARE_GT_OBJECT_EXTRACT_FIRST_PRIMITIVES_AT_TIME_FUNCTION( \
	primitive_type,gt_object_type,primitive_var) \
PROTOTYPE_GT_OBJECT_EXTRACT_FIRST_PRIMITIVES_AT_TIME_FUNCTION( \
	primitive_type) \
/***************************************************************************** \
LAST MODIFIED : 18 March 2003 \
\
DESCRIPTION : \
Returns the first primitives in <graphics_object> at <time> that have the \
given <object_name>, or NULL if there are no primitives or none with the name. \
The extracted primitives are returned in a linked-list. \
============================================================================*/ \
{ \
	int time_number; \
	struct primitive_type *last_primitive, *primitive; \
	union GT_primitive_list *primitive_list; \
\
	ENTER(GT_OBJECT_EXTRACT_FIRST_PRIMITIVES_AT_TIME(primitive_type)); \
	primitive = (struct primitive_type *)NULL; \
	if (graphics_object && (gt_object_type == graphics_object->object_type)) \
	{ \
		if (graphics_object->primitive_lists && \
			(0 < (time_number = GT_object_get_time_number(graphics_object, time)))) \
		{ \
			primitive_list = graphics_object->primitive_lists + time_number - 1; \
			primitive = primitive_list->primitive_var.first; \
			if (primitive) \
			{ \
				if (primitive->object_name == object_name) \
				{ \
					last_primitive = primitive; \
					while ((last_primitive->ptrnext) && \
						(last_primitive->ptrnext->object_name == object_name)) \
					{ \
						last_primitive = last_primitive->ptrnext; \
					} \
					primitive_list->primitive_var.first = last_primitive->ptrnext; \
					if (last_primitive->ptrnext) \
					{ \
						last_primitive->ptrnext = (struct primitive_type *)NULL; \
					} \
					else \
					{ \
						primitive_list->primitive_var.last = \
							(struct primitive_type *)NULL; \
					} \
					GT_object_changed(graphics_object); \
				} \
				else \
				{ \
					primitive = (struct primitive_type *)NULL; \
				} \
			} \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"GT_OBJECT_EXTRACT_FIRST_PRIMITIVES_AT_TIME(" #primitive_type \
			").  Invalid arguments"); \
	} \
	LEAVE; \
\
	return (primitive); \
} /* GT_OBJECT_EXTRACT_FIRST_PRIMITIVES_AT_TIME(primitive_type) */

#define DECLARE_GT_OBJECT_EXTRACT_FIRST_PRIMITIVES_AT_TIME_AUXILIARY_FUNCTION( \
	primitive_type,gt_object_type,primitive_var) \
PROTOTYPE_GT_OBJECT_EXTRACT_FIRST_PRIMITIVES_AT_TIME_FUNCTION( \
	primitive_type) \
/***************************************************************************** \
LAST MODIFIED : 18 March 2003 \
\
DESCRIPTION : \
Returns the first primitives in <graphics_object> at <time> that have the \
given <object_name>, or NULL if there are no primitives or none with the name. \
The extracted primitives are returned in a linked-list. \
Version for objects using the auxiliary_object_name in place of object_name. \
============================================================================*/ \
{ \
	int time_number; \
	struct primitive_type *last_primitive, *primitive; \
	union GT_primitive_list *primitive_list; \
\
	ENTER(GT_OBJECT_EXTRACT_FIRST_PRIMITIVES_AT_TIME_AUXILIARY(primitive_type)); \
	primitive = (struct primitive_type *)NULL; \
	if (graphics_object && (gt_object_type == graphics_object->object_type)) \
	{ \
		if (graphics_object->primitive_lists && \
			(0 < (time_number = GT_object_get_time_number(graphics_object, time)))) \
		{ \
			primitive_list = graphics_object->primitive_lists + time_number - 1; \
			primitive = primitive_list->primitive_var.first; \
			if (primitive) \
			{ \
				if (primitive->auxiliary_object_name == object_name) \
				{ \
					last_primitive = primitive; \
					while ((last_primitive->ptrnext) && \
						(last_primitive->ptrnext->auxiliary_object_name == object_name)) \
					{ \
						last_primitive = last_primitive->ptrnext; \
					} \
					primitive_list->primitive_var.first = last_primitive->ptrnext; \
					if (last_primitive->ptrnext) \
					{ \
						last_primitive->ptrnext = (struct primitive_type *)NULL; \
					} \
					else \
					{ \
						primitive_list->primitive_var.last = \
							(struct primitive_type *)NULL; \
					} \
					GT_object_changed(graphics_object); \
				} \
				else \
				{ \
					primitive = (struct primitive_type *)NULL; \
				} \
			} \
		} \
	} \
	else \
	{ \
		display_message(ERROR_MESSAGE, \
			"GT_OBJECT_EXTRACT_FIRST_PRIMITIVES_AT_TIME_AUXILIARY(" #primitive_type \
			").  Invalid arguments"); \
	} \
	LEAVE; \
\
	return (primitive); \
} /* GT_OBJECT_EXTRACT_FIRST_PRIMITIVES_AT_TIME_AUXILIARY(primitive_type) */

enum cmzn_graphics_select_mode GT_object_get_select_mode(
	struct GT_object *graphics_object)
{
	if (graphics_object)
		return graphics_object->select_mode;
	return CMZN_GRAPHICS_SELECT_MODE_INVALID;
}

int GT_object_set_select_mode(struct GT_object *graphics_object,
	enum cmzn_graphics_select_mode select_mode)
{
	if (graphics_object && (0 != ENUMERATOR_STRING(cmzn_graphics_select_mode)(select_mode)))
	{
		if (select_mode != graphics_object->select_mode)
		{
			graphics_object->select_mode=select_mode;
			GT_object_changed(graphics_object);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

Graphics_object_glyph_labels_function Graphics_object_get_glyph_labels_function(
	struct GT_object *graphics_object)
/*******************************************************************************
LAST MODIFIED : 19 September 2005

DESCRIPTION :
Gets the glyph_labels_function of the <graphics_object>.
This function enables a custom, per compile, labelling for a graphics object
==============================================================================*/
{
	Graphics_object_glyph_labels_function glyph_labels_function;

	ENTER(GT_object_get_glyph_labels_function);
	if (graphics_object)
	{
		glyph_labels_function = graphics_object->glyph_labels_function;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"GT_object_set_glyph_labels_function.  Invalid argument(s)");
		glyph_labels_function = 0;
	}
	LEAVE;

	return (glyph_labels_function);
} /* GT_object_get_glyph_labels_function */

int Graphics_object_set_glyph_labels_function(struct GT_object *graphics_object,
	Graphics_object_glyph_labels_function glyph_labels_function)
/*******************************************************************************
LAST MODIFIED : 19 September 2005

DESCRIPTION :
Sets the glyph_labels_function of the <graphics_object>.
This function enables a custom, per compile, labelling for a graphics object
==============================================================================*/
{
	int return_code;

	ENTER(GT_object_set_glyph_labels_function);
	if (graphics_object)
	{
		graphics_object->glyph_labels_function = glyph_labels_function;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"GT_object_set_glyph_labels_function.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* GT_object_set_glyph_labels_function */

double get_GT_object_render_line_width(struct GT_object *graphics_object)
{
	if (graphics_object)
		return graphics_object->render_line_width;
	return 0.0;
}

int set_GT_object_render_line_width(struct GT_object *graphics_object,
	double width)
{
	if (graphics_object && (width >= 0.0))
	{
		if (graphics_object->render_line_width != width)
		{
			graphics_object->render_line_width = width;
			GT_object_changed(graphics_object);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

double get_GT_object_render_point_size(struct GT_object *graphics_object)
{
	if (graphics_object)
		return graphics_object->render_point_size;
	return 0.0;
}

int set_GT_object_render_point_size(struct GT_object *graphics_object,
	double size)
{
	if (graphics_object && (size >= 0.0))
	{
		if (graphics_object->render_point_size != size)
		{
			graphics_object->render_point_size = size;
			GT_object_changed(graphics_object);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_material *get_GT_object_default_material
	(struct GT_object *graphics_object)
/*******************************************************************************
LAST MODIFIED : 4 June 1999

DESCRIPTION :
Gets the default_material of a GT_object.
==============================================================================*/
{
	cmzn_material *material;

	ENTER(get_GT_object_default_material);
	if (graphics_object)
	{
		material = graphics_object->default_material;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"get_GT_object_default_material.  Invalid graphics object");
		material = (cmzn_material *)NULL;
	}
	LEAVE;

	return (material);
} /* get_GT_object_default_material */

int set_GT_object_default_material(struct GT_object *graphics_object,
	cmzn_material *material)
/*******************************************************************************
LAST MODIFIED : 20 February 2000

DESCRIPTION :
Sets the default_material of a GT_object.
==============================================================================*/
{
	int return_code;

	ENTER(set_GT_object_default_material);
	if (graphics_object)
	{
		/* SAB Allow NULL material as this is what is required in glyphs where
			the material changes with the data */
		if (material != graphics_object->default_material)
		{
			REACCESS(cmzn_material)(&(graphics_object->default_material),
				material);
			GT_object_changed(graphics_object);
		}
	  return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"set_GT_object_default_material.  Invalid graphics object");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* set_GT_object_default_material */

cmzn_material *get_GT_object_secondary_material
	(struct GT_object *graphics_object)
/*******************************************************************************
LAST MODIFIED : 4 June 1999

DESCRIPTION :
Gets the secondary_material of a GT_object.
==============================================================================*/
{
	cmzn_material *material;

	ENTER(get_GT_object_secondary_material);
	if (graphics_object)
	{
		material = graphics_object->secondary_material;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"get_GT_object_secondary_material.  Invalid graphics object");
		material = (cmzn_material *)NULL;
	}
	LEAVE;

	return (material);
} /* get_GT_object_secondary_material */

int set_GT_object_secondary_material(struct GT_object *graphics_object,
	cmzn_material *material)
/*******************************************************************************
LAST MODIFIED : 20 February 2000

DESCRIPTION :
Sets the secondary_material of a GT_object.
==============================================================================*/
{
	int return_code;

	ENTER(set_GT_object_secondary_material);
	if (graphics_object)
	{
		/* SAB Allow NULL material as this is what is required in glyphs where
			the material changes with the data */
		if (material != graphics_object->secondary_material)
		{
			REACCESS(cmzn_material)(&(graphics_object->secondary_material),
				material);
			GT_object_changed(graphics_object);
		}
	  return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"set_GT_object_secondary_material.  Invalid graphics object");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* set_GT_object_secondary_material */

int GT_object_set_name(struct GT_object *gt_object, const char *name)
{
	if (gt_object)
	{
		if (gt_object->name)
			DEALLOCATE(gt_object->name);
		gt_object->name = name ? duplicate_string(name) : 0;
		return 1;
	}
	return 0;
}

cmzn_material *get_GT_object_selected_material
	(struct GT_object *graphics_object)
/*******************************************************************************
LAST MODIFIED : 18 February 2000

DESCRIPTION :
Gets the selected_material of a GT_object.
==============================================================================*/
{
	cmzn_material *material;

	ENTER(get_GT_object_selected_material);
	if (graphics_object)
	{
		material = graphics_object->selected_material;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"get_GT_object_selected_material.  Invalid graphics object");
		material = (cmzn_material *)NULL;
	}
	LEAVE;

	return (material);
} /* get_GT_object_selected_material */

int set_GT_object_selected_material(struct GT_object *graphics_object,
	cmzn_material *material)
/*******************************************************************************
LAST MODIFIED : 20 February 2000

DESCRIPTION :
Sets the selected_material of a GT_object.
==============================================================================*/
{
	int return_code;

	ENTER(set_GT_object_selected_material);
	if (graphics_object)
	{
		if (material)
		{
			if (material != graphics_object->selected_material)
			{
				REACCESS(cmzn_material)(&(graphics_object->selected_material),
					material);
				GT_object_changed(graphics_object);
			}
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"set_GT_object_selected_material.  Invalid material object");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"set_GT_object_selected_material.  Invalid graphics object");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* set_GT_object_selected_material */

int set_GT_object_Spectrum(struct GT_object *graphics_object,
	struct cmzn_spectrum *spectrum)
{
	int return_code;
	if (graphics_object)
	{
		if (spectrum != graphics_object->spectrum)
		{
			REACCESS(cmzn_spectrum)(&graphics_object->spectrum, spectrum);
			GT_object_changed(graphics_object);
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"set_GT_object_Spectrum.  Invalid graphics object");
		return_code=0;
	}
	return (return_code);
}

struct cmzn_spectrum *get_GT_object_spectrum(struct GT_object *graphics_object)
{
	struct cmzn_spectrum *spectrum;
	if (graphics_object)
	{
		spectrum = graphics_object->spectrum;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"get_GT_object_spectrum.  Invalid graphics object");
		spectrum = (struct cmzn_spectrum *)NULL;
	}
	return (spectrum);
}

struct cmzn_font *get_GT_object_font(struct GT_object *graphics_object)
{
	if (graphics_object)
	{
		if ((g_GLYPH_SET_VERTEX_BUFFERS == graphics_object->object_type) && graphics_object->primitive_lists)
		{
			GT_glyphset_vertex_buffers *glyph_set = graphics_object->primitive_lists->gt_glyphset_vertex_buffers;
			if (glyph_set)
				return glyph_set->font;
		}
		else if ((g_POINT_SET_VERTEX_BUFFERS == graphics_object->object_type) && graphics_object->primitive_lists)
		{
			GT_pointset_vertex_buffers *point_set = graphics_object->primitive_lists->gt_pointset_vertex_buffers;
			if (point_set)
				return point_set->font;
		}
	}
	return 0;
}

int set_GT_object_font(struct GT_object *graphics_object,
	struct cmzn_font *font)
{
	int return_code = 0;
	if (graphics_object)
	{
		if ((g_GLYPH_SET_VERTEX_BUFFERS == graphics_object->object_type) && graphics_object->primitive_lists)
		{
			// assume only one time
			GT_glyphset_vertex_buffers *glyph_set = graphics_object->primitive_lists->gt_glyphset_vertex_buffers;
			if (glyph_set && (glyph_set->font != font))
			{
				REACCESS(cmzn_font)(&glyph_set->font, font);
				GT_object_changed(graphics_object);
			}
		}
		else if ((g_POINT_SET_VERTEX_BUFFERS == graphics_object->object_type) && graphics_object->primitive_lists)
		{
			GT_pointset_vertex_buffers *point_set = graphics_object->primitive_lists->gt_pointset_vertex_buffers;
			if (point_set && (point_set->font != font))
			{
				REACCESS(cmzn_font)(&point_set->font, font);
				GT_object_changed(graphics_object);
			}
		}
		return_code = 1;
	}
	return (return_code);
}

struct GT_object *get_GT_object_glyph(struct GT_object *graphics_object)
{
	if (graphics_object && (g_GLYPH_SET_VERTEX_BUFFERS == graphics_object->object_type) &&
		graphics_object->primitive_lists)
	{
		GT_glyphset_vertex_buffers *glyph_set = graphics_object->primitive_lists[0].gt_glyphset_vertex_buffers;
		if (glyph_set)
		{
			return glyph_set->glyph;
		}
	}
	return 0;
}

int set_GT_object_glyph(struct GT_object *graphics_object,
	struct GT_object *glyph)
{
	int return_code = 0;
	if (graphics_object)
	{
		if ((g_GLYPH_SET_VERTEX_BUFFERS == graphics_object->object_type) &&
			graphics_object->primitive_lists)
		{
			// assume only one time
			GT_glyphset_vertex_buffers *glyph_set = graphics_object->primitive_lists[0].gt_glyphset_vertex_buffers;
			if (glyph_set && (glyph_set->glyph != glyph))
			{
				REACCESS(GT_object)(&glyph_set->glyph, glyph);
				GT_object_changed(graphics_object);
			}
		}
		return_code = 1;
	}
	return (return_code);
}

int set_GT_object_glyph_repeat_mode(struct GT_object *graphics_object,
	enum cmzn_glyph_repeat_mode glyph_repeat_mode)
{
	int return_code = 0;
	if (graphics_object)
	{
		if ((g_GLYPH_SET_VERTEX_BUFFERS == graphics_object->object_type) && graphics_object->primitive_lists)
		{
			// assume only one time
			GT_glyphset_vertex_buffers *glyph_set = graphics_object->primitive_lists->gt_glyphset_vertex_buffers;
			if (glyph_set && (glyph_set->glyph_repeat_mode != glyph_repeat_mode))
			{
				glyph_set->glyph_repeat_mode = glyph_repeat_mode;
				GT_object_changed(graphics_object);
			}
		}
		return_code = 1;
	}
	return (return_code);
}

int set_GT_object_glyph_base_size(struct GT_object *graphics_object,
	Triple base_size)
{
	int return_code = 0;
	if (graphics_object)
	{
		if ((g_GLYPH_SET_VERTEX_BUFFERS == graphics_object->object_type) && graphics_object->primitive_lists)
		{
			// assume only one time
			GT_glyphset_vertex_buffers *glyph_set = graphics_object->primitive_lists->gt_glyphset_vertex_buffers;
			if (glyph_set && (
				(glyph_set->base_size[0] != base_size[0]) ||
				(glyph_set->base_size[1] != base_size[1]) ||
				(glyph_set->base_size[2] != base_size[2])))
			{
				glyph_set->base_size[0] = base_size[0];
				glyph_set->base_size[1] = base_size[1];
				glyph_set->base_size[2] = base_size[2];
				GT_object_changed(graphics_object);
			}
		}
		return_code = 1;
	}
	return (return_code);
}

int set_GT_object_glyph_scale_factors(struct GT_object *graphics_object,
	Triple scale_factors)
{
	int return_code = 0;
	if (graphics_object)
	{
		if ((g_GLYPH_SET_VERTEX_BUFFERS == graphics_object->object_type) && graphics_object->primitive_lists)
		{
			// assume only one time
			GT_glyphset_vertex_buffers *glyph_set = graphics_object->primitive_lists->gt_glyphset_vertex_buffers;
			if (glyph_set && (
				(glyph_set->scale_factors[0] != scale_factors[0]) ||
				(glyph_set->scale_factors[1] != scale_factors[1]) ||
				(glyph_set->scale_factors[2] != scale_factors[2])))
			{
				glyph_set->scale_factors[0] = scale_factors[0];
				glyph_set->scale_factors[1] = scale_factors[1];
				glyph_set->scale_factors[2] = scale_factors[2];
				GT_object_changed(graphics_object);
			}
		}
		return_code = 1;
	}
	return (return_code);
}

int set_GT_object_glyph_offset(struct GT_object *graphics_object,
	Triple offset)
{
	int return_code = 0;
	if (graphics_object)
	{
		if ((g_GLYPH_SET_VERTEX_BUFFERS == graphics_object->object_type) && graphics_object->primitive_lists)
		{
			// assume only one time
			GT_glyphset_vertex_buffers *glyph_set = graphics_object->primitive_lists->gt_glyphset_vertex_buffers;
			if (glyph_set && (
				(glyph_set->offset[0] != offset[0]) ||
				(glyph_set->offset[1] != offset[1]) ||
				(glyph_set->offset[2] != offset[2])))
			{
				glyph_set->offset[0] = offset[0];
				glyph_set->offset[1] = offset[1];
				glyph_set->offset[2] = offset[2];
				GT_object_changed(graphics_object);
			}
		}
		return_code = 1;
	}
	return (return_code);
}

int set_GT_object_glyph_label_offset(struct GT_object *graphics_object,
	Triple label_offset)
{
	int return_code = 0;
	if (graphics_object)
	{
		if ((g_GLYPH_SET_VERTEX_BUFFERS == graphics_object->object_type) && graphics_object->primitive_lists)
		{
			// assume only one time
			GT_glyphset_vertex_buffers *glyph_set = graphics_object->primitive_lists->gt_glyphset_vertex_buffers;
			if (glyph_set && (
				(glyph_set->label_offset[0] != label_offset[0]) ||
				(glyph_set->label_offset[1] != label_offset[1]) ||
				(glyph_set->label_offset[2] != label_offset[2])))
			{
				glyph_set->label_offset[0] = label_offset[0];
				glyph_set->label_offset[1] = label_offset[1];
				glyph_set->label_offset[2] = label_offset[2];
				GT_object_changed(graphics_object);
			}
		}
		return_code = 1;
	}
	return (return_code);
}

int set_GT_object_glyph_label_text(struct GT_object *graphics_object,
	char *static_label_text[3])
{
	int return_code = 0;
	if (graphics_object && static_label_text)
	{
		if ((g_GLYPH_SET_VERTEX_BUFFERS == graphics_object->object_type) && graphics_object->primitive_lists)
		{
			// assume only one time
			GT_glyphset_vertex_buffers *glyph_set = graphics_object->primitive_lists->gt_glyphset_vertex_buffers;
			if (glyph_set)
			{
				bool different = false;
				for (int i = 0; i < 3; ++i)
				{
					if ((static_label_text[i] != glyph_set->static_label_text[i]) && (
						(0 == static_label_text[i]) || (0 == glyph_set->static_label_text[i]) ||
						(0 != strcmp(static_label_text[i], glyph_set->static_label_text[i]))))
					{
						different = true;
						break;
					}
				}
				if (different)
				{
					for (int i = 0; i < 3; ++i)
					{
						if (glyph_set->static_label_text[i])
						{
							DEALLOCATE(glyph_set->static_label_text[i]);
						}
						glyph_set->static_label_text[i] =
							static_label_text[i] ? duplicate_string(static_label_text[i]) : 0;\
					}
					GT_object_changed(graphics_object);
				}
			}
		}
		return_code = 1;
	}
	return (return_code);
}

int set_GT_object_render_polygon_mode(struct GT_object *graphics_object,
	enum cmzn_graphics_render_polygon_mode render_polygon_mode)
{
	int return_code = 0;
	if (graphics_object)
	{
		if ((g_SURFACE_VERTEX_BUFFERS == graphics_object->object_type) && graphics_object->primitive_lists)
		{
			// assume only one time
			GT_surface_vertex_buffers *surface = graphics_object->primitive_lists[0].gt_surface_vertex_buffers;
			if (surface && (surface->render_polygon_mode != render_polygon_mode))
			{
				surface->render_polygon_mode = render_polygon_mode;
				GT_object_changed(graphics_object);
			}
		}
		return_code = 1;
	}
	return (return_code);
}

int GT_object_list_contents(struct GT_object *graphics_object,void *dummy_void)
/*******************************************************************************
LAST MODIFIED : 14 August 1998

DESCRIPTION :
Writes out information contained in <graphics_object> including its name and
type.
==============================================================================*/
{
	char *material_name;
	int return_code;

	ENTER(GT_object_list_contents);
	if (graphics_object && (!dummy_void))
	{
		if (graphics_object->name)
		{
			return_code=1;
			display_message(INFORMATION_MESSAGE,graphics_object->name);
			display_message(INFORMATION_MESSAGE," = %s",
				get_GT_object_type_string(graphics_object->object_type));
			if (graphics_object->default_material&&GET_NAME(cmzn_material)(
				graphics_object->default_material,&material_name))
			{
				display_message(INFORMATION_MESSAGE," material %s",
					material_name);
				DEALLOCATE(material_name);
			}
			display_message(INFORMATION_MESSAGE,"; access_count=%d\n",
				graphics_object->access_count);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"GT_object_list_contents.  Missing graphics object name");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"GT_object_list_contents.  Invalid graphics object");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* GT_object_list_contents */

enum cmzn_glyph_shape_type GT_object_get_glyph_type(
	struct GT_object *gt_object)
{
	if (gt_object)
		return gt_object->glyph_type;
	return CMZN_GLYPH_SHAPE_TYPE_INVALID;
}

int GT_object_set_glyph_type(struct GT_object *gt_object,
	enum cmzn_glyph_shape_type glyph_type)
{
	if (gt_object)
	{
		gt_object->glyph_type = glyph_type;
		return 1;
	}
	return 0;
}
\

#if 0
int GT_object_get_overlay(struct GT_object *graphics_object)
{
	return graphics_object->overlay;
}

int set_GT_object_overlay(struct GT_object *graphics_object, int overlay)
{
	int return_code;

	ENTER(set_GT_object_overlay);
	if (graphics_object)
	{
		if (overlay != graphics_object->overlay)
		{
			graphics_object->overlay = overlay;
			GT_object_changed(graphics_object);
		}
	  return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"set_GT_object_overlay.  Invalid graphics object");
		return_code=0;
	}
	LEAVE;

	return (return_code);
}
#endif
