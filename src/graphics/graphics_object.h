/**
 * FILE : graphics_object.h
 *
 * Private interfaces to Graphical object data structures.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (GRAPHICS_OBJECT_H)
#define GRAPHICS_OBJECT_H


#include "opencmiss/zinc/zincconfigure.h"
#include "opencmiss/zinc/types/glyphid.h"
#include "opencmiss/zinc/graphics.h"
#include "general/geometry.h"
#include "graphics/auxiliary_graphics_types.h"
#include "graphics/material.h"
#include "graphics/spectrum.h"
#include "graphics/volume_texture.h"
#if defined (USE_OPENCASCADE)
#include "opencmiss/zinc/fieldcad.h"
#endif /* defined (USE_OPENCASCADE) */

/*
Global types
------------
*/

struct cmzn_font;
struct cmzn_scene;
class DsLabelsChangeLog;

enum GT_object_type
/*******************************************************************************
LAST MODIFIED : 13 August 1998

DESCRIPTION :
Must ensure all the types defined here are handled by function
get_GT_object_type_string.
Have members BEFORE_FIRST and AFTEGraphics_object_range_structR_LAST to enable iterating through the list
without knowing which order the types are in.
==============================================================================*/
{
	g_OBJECT_TYPE_INVALID,
	g_OBJECT_TYPE_BEFORE_FIRST,
	g_POLYLINE_VERTEX_BUFFERS,
	g_SURFACE_VERTEX_BUFFERS,
	g_GLYPH_SET_VERTEX_BUFFERS,
	g_POINT_SET_VERTEX_BUFFERS,
	g_POINT_VERTEX_BUFFERS,
	g_OBJECT_TYPE_AFTER_LAST
};

typedef enum
/*******************************************************************************
LAST MODIFIED : 26 June 1998

DESCRIPTION :
==============================================================================*/
{
	g_GENERAL_POLYGON = 1,
	g_QUADRILATERAL = 2, /* Deprecated, no longer supported in latest version  of OpenGL. */
	g_TRIANGLE = 3
} gtPolygonType;

enum GT_surface_type
/*******************************************************************************
LAST MODIFIED : 13 August 1998

DESCRIPTION :
Must ensure all the types defined here are handled by functions:
  get_GT_surface_type_string      (in: graphics_object.c)
  get_GT_surface_type_from_string (in: graphics_object.c)
  makegtobj                       (in: makegtobj.c)
Have members BEFORE_FIRST and AFTER_LAST to enable iterating through the list
without knowing which order the types are in.
==============================================================================*/
{
	g_SURFACE_TYPE_INVALID,
	g_SURFACE_TYPE_BEFORE_FIRST,
	g_SHADED, /* old 0 */
	g_SH_DISCONTINUOUS, /* old 3 */
	g_SHADED_TEXMAP, /* old 6 */
	g_SH_DISCONTINUOUS_TEXMAP, /* old 7 */
	g_SH_DISCONTINUOUS_STRIP,
	g_SH_DISCONTINUOUS_STRIP_TEXMAP,
	g_SURFACE_TYPE_AFTER_LAST
};

enum GT_polyline_type
/*******************************************************************************
LAST MODIFIED : 13 August 1998

DESCRIPTION :
Must ensure all the types defined here are handled by functions:
  get_GT_polyline_type_string      (in: graphics_object.c)
  get_GT_polyline_type_from_string (in: graphics_object.c)
  makegtobj                        (in: makegtobj.c)
  file_read_graphics_objects       (in: import_graphics_object.c)
Have members BEFORE_FIRST and AFTER_LAST to enable iterating through the list
without knowing which order the types are in.
==============================================================================*/
{
	g_POLYLINE_TYPE_INVALID,
	g_POLYLINE_TYPE_BEFORE_FIRST,
	g_PLAIN, /* old 0 */
	g_NORMAL, /* old 1 */
	g_PLAIN_DISCONTINUOUS, /* old 2 */
	g_NORMAL_DISCONTINUOUS, /* old 3 */
	g_POLYLINE_TYPE_AFTER_LAST
}; /* enum GT_polyline_type */

typedef enum
/*******************************************************************************
LAST MODIFIED : 6 July 1998

DESCRIPTION :
If the gtTransformType of a graphics object is g_NOT_ID it will use its
transformation matrix, otherwise no matrix manipulations are output with it.
Note that if the gtInheritanceType of the graphics object is g_CHILD, the
transformation step is bypassed anyway.
==============================================================================*/
{
	g_ID,
	g_NOT_ID
} gtTransformType;

typedef enum
/*******************************************************************************
LAST MODIFIED : 16 February 1996

DESCRIPTION :
==============================================================================*/
{
	g_NO_DATA,
	g_SCALAR,
	g_TWO_COMPONENTS,
	g_VECTOR,
	g_VECTOR4
} ZnRealType;

typedef enum
/*******************************************************************************
LAST MODIFIED : 23 February 1998

DESCRIPTION :
==============================================================================*/
{
	g_NO_MARKER,
	g_POINT_MARKER,
	g_PLUS_MARKER,
	g_DERIVATIVE_MARKER
} gtMarkerType;

struct GT_polyline_vertex_buffers;
struct GT_surface_vertex_buffers;
struct GT_glyphset_vertex_buffers;
struct GT_pointset_vertex_buffers;

struct GT_object;

typedef struct GT_object gtObject;

/**
 * Structure for storing range of coordinates in graphics.
 * Initialised with first flag set; range is only valid if first is cleared.
 */
struct Graphics_object_range_struct
{
	int first;
	Triple maximum, minimum;

	Graphics_object_range_struct() :
		first(1)
	{
		// zero ranges to handle fewer than 3 coordinates
		for (int i = 0; i < 3; ++i)
			this->maximum[i] = this->minimum[i] = 0.0;
	}
};

/**
 * Class for storing range of data in one or several graphics objects.
 * Set valuesCount to the number of ranges requested, and supply both
 * minimumValues and maximumValues with that size.
 * After use, maxRanges contains the number of ranges that are obtainable
 * which can be more or less than number requested.
 */
class Graphics_object_data_range
{
	int maxRanges;
	int valuesCount;
	double *minimumValues, *maximumValues;

public:
	Graphics_object_data_range(int valuesCount, double *minimumValues, double *maximumValues) :
		maxRanges(0),
		valuesCount(valuesCount),
		minimumValues(minimumValues),
		maximumValues(maximumValues)
	{
	}

	/** caller must ensure valuesCount > 0 and values is non-zero */
	template <typename RealType> void addValues(int valuesCountIn, RealType *valuesIn)
	{
		const int count = (valuesCountIn > this->valuesCount) ? this->valuesCount : valuesCountIn;
		if (valuesCountIn > this->maxRanges)
		{
			for (int i = this->maxRanges; i < count; ++i)
			{
				this->minimumValues[i] = this->maximumValues[i] = static_cast<double>(valuesIn[i]);
			}
			this->maxRanges = valuesCountIn;
		}
		double value;
		for (int i = 0; i < count; ++i)
		{
			value = static_cast<double>(valuesIn[i]);
			if (value < this->minimumValues[i])
			{
				this->minimumValues[i] = value;
			}
			else if (value > this->maximumValues[i])
			{
				this->maximumValues[i] = value;
			}
		}
	}

	int getMaxRanges() const
	{
		return this->maxRanges;
	}
};

struct Graphics_object_time_range_struct
/*******************************************************************************
LAST MODIFIED : 6 August 1997

DESCRIPTION :
Structure for storing range of time in one or several graphics objects.
Set first=1 before calling time range routines. Only if first==0 afterwards is
the time range valid.
==============================================================================*/
{
	int first;
	ZnReal minimum,maximum;
}; /* Graphics_object_time_range_struct */

/*
Global functions
----------------
*/

enum GT_object_type GT_object_get_type(struct GT_object *gt_object);
/*******************************************************************************
LAST MODIFIED : 17 June 2004

DESCRIPTION :
Returns the object type from the gt_object.
==============================================================================*/

struct GT_object *GT_object_get_next_object(struct GT_object *gt_object);
/*******************************************************************************
LAST MODIFIED : 17 June 2004

DESCRIPTION :
Returns the next object from the gt_object.
==============================================================================*/

int GT_object_set_next_object(struct GT_object *gt_object,
	struct GT_object *next_object);
/*******************************************************************************
LAST MODIFIED : 17 June 2004

DESCRIPTION :
Sets the next object for the gt_object.
==============================================================================*/

const char *get_GT_object_type_string(enum GT_object_type object_type);
/*******************************************************************************
LAST MODIFIED : 13 August 1998

DESCRIPTION :
Returns a string describing the object type, suitable for writing to file
(and reinterpreting it later).
==============================================================================*/

int get_GT_object_type_from_string(char *type_string,
	enum GT_object_type *object_type);
/*******************************************************************************
LAST MODIFIED : 13 August 1998

DESCRIPTION :
Returns the object type from the string produced by function
get_GT_object_type_string. For compatibility, also supports converting old
enumerator numbers (as text) into the new enumerator values, with a warning.
==============================================================================*/

const char *get_GT_polyline_type_string(enum GT_polyline_type polyline_type);
/*******************************************************************************
LAST MODIFIED : 13 August 1998

DESCRIPTION :
Returns a string describing the polyline type, suitable for writing to file
(and reinterpreting it later).
==============================================================================*/

int get_GT_polyline_type_from_string(char *type_string,
	enum GT_polyline_type *polyline_type);
/*******************************************************************************
LAST MODIFIED : 13 August 1998

DESCRIPTION :
Returns the polyline type from the string produced by function
get_GT_polyline_type_string. For compatibility, also supports converting old
enumerator numbers (as text) into the new enumerator values, with a warning.
==============================================================================*/

const char *get_GT_surface_type_string(enum GT_surface_type surface_type);
/*******************************************************************************
LAST MODIFIED : 13 August 1998

DESCRIPTION :
Returns a string describing the surface type, suitable for writing to file
(and reinterpreting it later).
==============================================================================*/

int get_GT_surface_type_from_string(char *type_string,
	enum GT_surface_type *surface_type);
/*******************************************************************************
LAST MODIFIED : 13 August 1998

DESCRIPTION :
Returns the surface type from the string produced by function
get_GT_surface_type_string. For compatibility, also supports converting old
enumerator numbers (as text) into the new enumerator values, with a warning.
==============================================================================*/

PROTOTYPE_OBJECT_FUNCTIONS(GT_object);
PROTOTYPE_GET_OBJECT_NAME_FUNCTION(GT_object);


int DESTROY(GT_polyline_vertex_buffers)(struct GT_polyline_vertex_buffers **polyline);
int DESTROY(GT_surface_vertex_buffers)(struct GT_surface_vertex_buffers **surface);
int DESTROY(GT_glyphset_vertex_buffers)(struct GT_glyphset_vertex_buffers **glyphset);
int DESTROY(GT_pointset_vertex_buffers)(struct GT_pointset_vertex_buffers **glyphset);

int GT_glyphset_vertex_buffers_setup(GT_glyphset_vertex_buffers *glyphset,
	struct GT_object *glyph, enum cmzn_glyph_repeat_mode glyph_repeat_mode,
	Triple base_size, Triple scale_factors, Triple offset, cmzn_font_id font,
	Triple label_offset, char *static_label_text[3],
	int label_bounds_dimension, int label_bounds_components);

/***************************************************************************//**
 * Creates the shared scene information for a GT_polyline_vertex_buffers.
 */
struct GT_polyline_vertex_buffers *CREATE(GT_polyline_vertex_buffers)(
	enum GT_polyline_type polyline_type, int line_width);

struct GT_surface_vertex_buffers *CREATE(GT_surface_vertex_buffers)(
	enum GT_surface_type surface_type, enum cmzn_graphics_render_polygon_mode render_polygon_mode);

struct GT_glyphset_vertex_buffers *CREATE(GT_glyphset_vertex_buffers)();

struct GT_pointset_vertex_buffers *CREATE(GT_pointset_vertex_buffers)(
	struct cmzn_font *font, gtMarkerType marker_type,
	ZnReal marker_size);

int GT_surface_set_integer_identifier(struct GT_surface *surface,
	int identifier);
/*******************************************************************************
LAST MODIFIED : 18 June 2004

DESCRIPTION :
Sets the integer identifier used by the graphics to distinguish this object.
==============================================================================*/

/**
 * Allocates memory and assigns fields for a graphics object.
 * Object is returned with an access_count of 1.
 * Use DEACCESS(GT_object) to destroy.
 */
struct GT_object *CREATE(GT_object)(const char *name,enum GT_object_type object_type,
	cmzn_material *default_material);

/**
 * Mark graphics object as changed so it is recompiled at next redraw.
 */
void GT_object_changed(struct GT_object *graphics_object);

int GT_object_Graphical_material_change(struct GT_object *graphics_object,
	struct LIST(cmzn_material) *changed_material_list);
/*******************************************************************************
LAST MODIFIED : 12 March 2002

DESCRIPTION :
Tells the <graphics_object> that the materials in the <changed_material_list>
have changed. If any of these materials are used in any graphics object,
changes the compile_status to CHILD_GRAPHICS_NOT_COMPILED and
informs clients of the need to recompile and redraw. Note that if a spectrum is
in use the more expensive GRAPHICS_NOT_COMPILED status is necessarily set.
Note: Passing a NULL <changed_material_list> indicates the equivalent of a
change to any material in use in the linked graphics objects.
==============================================================================*/

int GT_object_Spectrum_change(struct GT_object *graphics_object,
	struct LIST(cmzn_spectrum) *changed_spectrum_list);
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

int GT_object_has_time(struct GT_object *graphics_object,ZnReal time);
/*******************************************************************************
LAST MODIFIED : 26 June 1997

DESCRIPTION :
Returns 1 if the time parameter is used by the graphics_object.
==============================================================================*/

/***************************************************************************//**
 * Returns the vertex buffer set if the graphics_object has one.
 */
struct Graphics_vertex_array *GT_object_get_vertex_set(struct GT_object *graphics_object);

/**
 * Returns the number of times/primitive lists in the graphics_object.
 */
int GT_object_get_number_of_times(struct GT_object *graphics_object);

/**
 * Mark for recompilation if graphics object has multiple times,
 * or any glyph it uses have multiple times.
 */
void GT_object_time_change(struct GT_object *graphics_object);

ZnReal GT_object_get_time(struct GT_object *graphics_object,int time_no);
/*******************************************************************************
LAST MODIFIED : 18 June 1998

DESCRIPTION :
Returns the time at <time_no> from the graphics_object.
Note that time numbers range from 1 to number_of_times.
==============================================================================*/

ZnReal GT_object_get_nearest_time(struct GT_object *graphics_object,ZnReal time);
/*******************************************************************************
LAST MODIFIED : 7 August 1997

DESCRIPTION :
Returns the nearest time to <time> in <graphics_object> at which graphics
primitives are called.
NOTE: presently finds the nearest time that is *lower* than <time>. When all
routines updated to use this, may be changed to get actual nearest time.
==============================================================================*/

int get_graphics_object_range(struct GT_object *graphics_object,
	void *graphics_object_range_void);
/*******************************************************************************
LAST MODIFIED : 8 August 1997

DESCRIPTION :
Returns the range of the coordinates in <graphics_object> or 0 if object empty
or error occurs. First should be set to 1 outside routine. Several calls to
this routine for differing graphics objects (without settings first in between)
will produce the range of all the graphics objects.
???RC only does some object types.
==============================================================================*/

/**
 * Expand the range to include the data range of this graphics object.
 * @see Graphics_object_data_range
 */
int get_graphics_object_data_range(struct GT_object *graphics_object,
	Graphics_object_data_range *range);

int get_graphics_object_time_range(struct GT_object *graphics_object,
	void *graphics_object_time_range_void);
/*******************************************************************************
LAST MODIFIED : 8 August 1997

DESCRIPTION :
Enlarges the minimum and maximum time range by that of the graphics_object.
==============================================================================*/

#if ! defined (SHORT_NAMES)
#define GT_OBJECT_ADD_(primitive_type) GT_object_add_ ## primitive_type
#else
#define GT_OBJECT_ADD_(primitive_type) goa_ ## primitive_type
#endif
#define GT_OBJECT_ADD(primitive_type) GT_OBJECT_ADD_(primitive_type)

#define PROTOTYPE_GT_OBJECT_ADD_FUNCTION(primitive_type) \
int GT_OBJECT_ADD(primitive_type)( \
	struct GT_object *graphics_object, \
	ZnReal time,struct primitive_type *primitive) \
/***************************************************************************** \
LAST MODIFIED : 17 March 2003 \
\
DESCRIPTION : \
Adds <primitive> to <graphics_object> at <time>, creating the new time if it \
does not already exist. If the <primitive> is NULL an empty time is added if \
there is not already one. <primitive> is a NULL-terminated linked-list. \
============================================================================*/ \

/*************************************************************************//**
 * Adds <primitive> to <graphics_object>.  There can be only one time for this
 * type
 */
int GT_OBJECT_ADD(GT_polyline_vertex_buffers)(
	struct GT_object *graphics_object, struct GT_polyline_vertex_buffers *primitive);

int GT_OBJECT_ADD(GT_surface_vertex_buffers)(
	struct GT_object *graphics_object, struct GT_surface_vertex_buffers *primitive);

int GT_OBJECT_ADD(GT_glyphset_vertex_buffers)(
	struct GT_object *graphics_object, struct GT_glyphset_vertex_buffers *primitive);

int GT_OBJECT_ADD(GT_pointset_vertex_buffers)(
	struct GT_object *graphics_object, struct GT_pointset_vertex_buffers *primitive);

/** When incrementally building graphics object with vertex arrays, call this to rebind a larger buffer each time */
void GT_object_reset_buffer_binding(struct GT_object *graphics_object);

#if ! defined (SHORT_NAMES)
#define GT_OBJECT_GET_(primitive_type) GT_object_get_ ## primitive_type
#else
#define GT_OBJECT_GET_(primitive_type) gog_ ## primitive_type
#endif
#define GT_OBJECT_GET(primitive_type) GT_OBJECT_GET_(primitive_type)

#define PROTOTYPE_GT_OBJECT_GET_FUNCTION(primitive_type) \
struct primitive_type *GT_OBJECT_GET(primitive_type)( \
	struct GT_object *graphics_object,ZnReal time) \
/***************************************************************************** \
LAST MODIFIED : 19 June 1997 \
\
DESCRIPTION : \
Returns pointer to the primitive at the given time in graphics_object. \
???RC only used in spectrum_editor.c and should be replaced.
============================================================================*/

/** Get the vertex buffer primitive iff graphics object is of corresponding type */
struct GT_glyphset_vertex_buffers *GT_object_get_GT_glyphset_vertex_buffers(
	struct GT_object *graphics_object);

typedef int (GT_object_primitive_object_name_conditional_function) \
	(int object_name, void *user_data);

/*
 * Mark primitives in the graphics object whose object name integer index
 * is marked as changed in the changeLog.
 * This means those objects' primitives must be rebuilt, but others are kept.
 */
int GT_object_invalidate_selected_primitives(struct GT_object *graphics_object,
	DsLabelsChangeLog *changeLog);

/**
 * Clears all primitives and vertext arrays from graphics object.
 */
int GT_object_clear_primitives(struct GT_object *graphics_object);

#if ! defined (SHORT_NAMES)
#define GT_OBJECT_EXTRACT_FIRST_PRIMITIVES_AT_TIME_(primitive_type) \
	GT_object_extract_first_primitives_at_time_ ## primitive_type
#else
#define GT_OBJECT_EXTRACT_FIRST_PRIMITIVES_AT_TIME_(primitive_type) \
	goefpt_ ## primitive_type
#endif
#define GT_OBJECT_EXTRACT_FIRST_PRIMITIVES_AT_TIME(primitive_type) \
	GT_OBJECT_EXTRACT_FIRST_PRIMITIVES_AT_TIME_(primitive_type)

#define PROTOTYPE_GT_OBJECT_EXTRACT_FIRST_PRIMITIVES_AT_TIME_FUNCTION( \
	primitive_type) \
struct primitive_type *GT_OBJECT_EXTRACT_FIRST_PRIMITIVES_AT_TIME( \
	primitive_type)(struct GT_object *graphics_object, \
	ZnReal time, int object_name) \
/***************************************************************************** \
LAST MODIFIED : 18 March 2003 \
\
DESCRIPTION : \
Returns the first primitives in <graphics_object> at <time> that have the \
given <object_name>, or NULL if there are no primitives or none with the name. \
The extracted primitives are returned in a linked-list. \
If the primitive type has an auxiliary_object_name, it is matched, not the \
object_name. \
============================================================================*/ \

/**
 * Gets the mode controlling how graphics are drawn depending on whether the
 * primitive is selected.
 */
enum cmzn_graphics_select_mode GT_object_get_select_mode(
	struct GT_object *graphics_object);

/**
 * Sets the mode controlling how graphics are drawn depending on whether the
 * primitive is selected, which is when the highlight_function is true.
 * For DRAW_SELECTED and DRAW_UNSELECTED only primitives meeting the
 * criteria are in the graphics object, so it only affects which material is
 * used in rendering.
 * @return  On success CMZN_OK, otherwise CMZN_ERROR_ARGUMENT.
 */
int GT_object_set_select_mode(struct GT_object *graphics_object,
	enum cmzn_graphics_select_mode select_mode);

/**
 * Gets the width of lines rendered with GL, in pixels.
 *
 * @param graphics_object  The graphics object to query.
 * @return  Line width in pixels, or 0.0 if unset or error.
 */
double get_GT_object_render_line_width(struct GT_object *graphics_object);

/**
 * Sets the width of lines rendered with GL, in pixels.
 * Default value is 0.0 meaning inherit width from current state
 * i.e. do not override.
 *
 * @param graphics_object  The graphics object to modify.
 * @param width  Line width in pixels, or 0.0 to use width from current state.
 * @return  Status CMZN_OK on success, otherwise CMZN_ERROR_ARGUMENT.
 */
int set_GT_object_render_line_width(struct GT_object *graphics_object,
	double width);

/**
 * Gets the size of points rendered with GL, in pixels.
 *
 * @param graphics_object  The graphics object to query.
 * @return  Point size in pixels, or 0.0 if unset or error.
 */
double get_GT_object_render_point_size(struct GT_object *graphics_object);

/**
 * Sets the size of points rendered with GL, in pixels.
 * Default value is 0.0 meaning inherit size from current state
 * i.e. do not override.
 *
 * @param graphics_object  The graphics object to modify.
 * @param size  Point size in pixels, or 0.0 to use size from current state.
 * @return  Status CMZN_OK on success, otherwise CMZN_ERROR_ARGUMENT.
 */
int set_GT_object_render_point_size(struct GT_object *graphics_object,
	double size);

cmzn_material *get_GT_object_default_material(
	struct GT_object *graphics_object);
/*******************************************************************************
LAST MODIFIED : 4 June 1999

DESCRIPTION :
Gets the default_material of a GT_object.
==============================================================================*/

int set_GT_object_default_material(struct GT_object *graphics_object,
	cmzn_material *material);
/*******************************************************************************
LAST MODIFIED : 11 June 1998

DESCRIPTION :
Sets the default_material of a GT_object.
==============================================================================*/

cmzn_material *get_GT_object_secondary_material(
	struct GT_object *graphics_object);
/*******************************************************************************
LAST MODIFIED : 30 September 2005

DESCRIPTION :
Gets the secondary_material of a GT_object.
==============================================================================*/

int set_GT_object_secondary_material(struct GT_object *graphics_object,
	cmzn_material *material);
/*******************************************************************************
LAST MODIFIED : 30 September 2005

DESCRIPTION :
Sets the secondary_material of a GT_object.
==============================================================================*/

int GT_object_set_name(struct GT_object *graphics_object, const char *name);
/*******************************************************************************
LAST MODIFIED : 30 April 2003

DESCRIPTION :
Changes the name of <graphics_object> to a copy of <name>.
==============================================================================*/

cmzn_material *get_GT_object_selected_material(
	struct GT_object *graphics_object);
/*******************************************************************************
LAST MODIFIED : 18 February 2000

DESCRIPTION :
Gets the selected_material of a GT_object.
==============================================================================*/

int set_GT_object_selected_material(struct GT_object *graphics_object,
	cmzn_material *material);
/*******************************************************************************
LAST MODIFIED : 18 February 2000

DESCRIPTION :
Sets the selected_material of a GT_object.
==============================================================================*/

/**
 * Sets the spectrum of a GT_object.
 */
int set_GT_object_Spectrum(struct GT_object *graphics_object,
	struct cmzn_spectrum *spectrum);

/**
 * Gets the spectrum of a GT_object.
 */
struct cmzn_spectrum *get_GT_object_spectrum(struct GT_object *graphics_object);

struct cmzn_font;

/**
 * Gets the font used by the first GT_glyphset_vertex_buffer in the graphics object, if any.
 */
struct cmzn_font *get_GT_object_font(struct GT_object *graphics_object);

/**
 * Sets the font of all GT_glyphset_vertex_buffer primitives in a GT_object.
 */
int set_GT_object_font(struct GT_object *graphics_object,
	struct cmzn_font *font);

/**
 * Gets the glyph used by the first GT_glyphset_vertex_buffer in the graphics object, if any.
 * @return  Glyph GT_object, non-accessed, or NULL on failure.
 */
struct GT_object *get_GT_object_glyph(struct GT_object *graphics_object);

/**
 * Sets the glyph of all GT_glyphset_vertex_buffer primitives in a GT_object.
 */
int set_GT_object_glyph(struct GT_object *graphics_object,
	struct GT_object *glyph);

/**
 * Sets the glyph repeat mode for all glyph_sets in the GT_object.
 */
int set_GT_object_glyph_repeat_mode(struct GT_object *graphics_object,
	enum cmzn_glyph_repeat_mode glyph_repeat_mode);

/**
 * Sets the glyph base size for all glyph_sets in the GT_object.
 */
int set_GT_object_glyph_base_size(struct GT_object *graphics_object,
	Triple base_size);

/**
 * Sets the glyph scale factors for all glyph_sets in the GT_object.
 */
int set_GT_object_glyph_scale_factors(struct GT_object *graphics_object,
	Triple scale_factors);

/**
 * Sets the glyph offset for all glyph_sets in the GT_object.
 */
int set_GT_object_glyph_offset(struct GT_object *graphics_object,
	Triple offset);

/**
 * Sets the glyph label offset for all glyph_sets in the GT_object.
 */
int set_GT_object_glyph_label_offset(struct GT_object *graphics_object,
	Triple label_offset);

/**
 * Sets static labels for all glyph_sets in the GT_object.
 */
int set_GT_object_glyph_label_text(struct GT_object *graphics_object,
	char *static_label_text[3]);

/**
 * Sets the polygon render mode for any primitives which support it in the
 * GT_object.
 */
int set_GT_object_render_polygon_mode(struct GT_object *graphics_object,
	enum cmzn_graphics_render_polygon_mode render_polygon_mode);

int GT_object_list_contents(struct GT_object *graphics_object,void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 5 January 1998

DESCRIPTION :
Writes out information contained in <graphics_object> including its name and
type.
==============================================================================*/

int expand_spectrum_range_with_graphics_object(
	struct GT_object *graphics_object,void *spectrum_void);
/*******************************************************************************
LAST MODIFIED : 24 November 1997

DESCRIPTION :
Ensures the <spectrum> maximum and minimum is at least large enough to include
the range of data values in <graphics_object>.
==============================================================================*/

/**
 * Internal function for getting type enum for quickly matching standard glyphs for
 * point, line, cross, for fast alternative rendering
 */
enum cmzn_glyph_shape_type GT_object_get_glyph_type(
	struct GT_object *gt_object);

/**
 * Internal function for setting type enum for quickly matching standard glyphs for
 * point, line, cross, for fast alternative rendering
 */
int GT_object_set_glyph_type(struct GT_object *gt_object,
	enum cmzn_glyph_shape_type glyph_type);


#endif /* !defined (GRAPHICS_OBJECT_H) */
