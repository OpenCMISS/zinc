/***************************************************************************//**
 * graphics.h
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (CMZN_GRAPHICS_H)
#define CMZN_GRAPHICS_H

#include <ctime>
#include "opencmiss/zinc/fieldgroup.h"
#include "opencmiss/zinc/graphics.h"
#include "opencmiss/zinc/types/scenefilterid.h"
#include "opencmiss/zinc/types/fieldmoduleid.h"
#include "computed_field/computed_field.h"
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_to_graphics_object.h"
#include "graphics/auxiliary_graphics_types.h"
#include "graphics/element_point_ranges.h"
#include "graphics/font.h"
#include "graphics/graphics_object.h"
#include "general/enumerator.h"
#include "general/list.h"
#include "graphics/material.h"
#include "graphics/spectrum.h"

struct cmzn_graphicspointattributes;
struct cmzn_graphicslineattributes;

struct cmzn_graphics
/*******************************************************************************
LAST MODIFIED : 14 March 2003

DESCRIPTION :
Stores one group of settings for a single line/surface/etc. part of the
finite element group scene.
==============================================================================*/
{
	/* position identifier for ordering settings in list */
	int position;

	/* scene which owns this graphics */
	struct cmzn_scene *scene;

	/* name for identifying settings */
	const char *name;

// 	/* geometry settings */
// 	/* for all graphics types */
	enum cmzn_graphics_type graphics_type;
	struct Computed_field *subgroup_field;
	struct Computed_field *coordinate_field;
	enum cmzn_graphics_select_mode select_mode;
	enum cmzn_field_domain_type domain_type;

	/* for 1-D and 2-D elements only */
	bool exterior;
	/* face number is from -1 to 5, where -1 is none/all, 0 is xi1=0, 1 is xi1=1,
		 2 is xi2=0 etc. */
	cmzn_element_face_type face;
	/* For surfaces only at the moment */
	struct Computed_field *texture_coordinate_field;

	/* line attributes */
	enum cmzn_graphicslineattributes_shape_type line_shape;
	FE_value line_base_size[2];
	FE_value line_scale_factors[2];
	cmzn_field_id line_orientation_scale_field;

	/* for contours only */
	struct Computed_field *isoscalar_field;
	int number_of_isovalues;
	/* If the isovalues array is set then these values are used,
		otherwise number_of_isovalues values are distributed from
		first_isovalue to last_isovalue including these values for n>1 */
	double *isovalues, first_isovalue, last_isovalue,
		decimation_threshold;

	/* point attributes */
	cmzn_glyph *glyph;
	enum cmzn_glyph_repeat_mode glyph_repeat_mode;
	FE_value point_offset[3];
	FE_value point_base_size[3];
	FE_value point_scale_factors[3];
	FE_value label_offset[3];
	char *label_text[3];
	struct Computed_field *point_orientation_scale_field;
	struct Computed_field *signed_scale_field;
	struct Computed_field *label_field;
	struct Computed_field *label_density_field;

	// for element sampling: element points, streamlines
	cmzn_element_point_sampling_mode sampling_mode;
	struct Computed_field *sample_density_field;
	FE_value sample_location[3];

	// for tessellating and sampling elements
	struct cmzn_tessellation *tessellation;
	struct cmzn_field *tessellation_field;

	int overlay_flag;
	int overlay_order;
	/* for settings starting in a particular element */
	struct FE_element *seed_element;

	/* streamlines */
	struct Computed_field *stream_vector_field;
	enum cmzn_graphics_streamlines_track_direction streamlines_track_direction;
	FE_value streamline_length;
	enum cmzn_graphics_streamlines_colour_data_type streamlines_colour_data_type;
	/* streamline seed nodeset and field giving mesh location */
	cmzn_nodeset_id seed_nodeset;
	struct Computed_field *seed_node_mesh_location_field;

	/* appearance settings */
	/* for all graphics types */
	bool visibility_flag;
	cmzn_material *material, *selected_material,
		*secondary_material;
	struct Computed_field *data_field;
	struct cmzn_spectrum *spectrum;
	int autorange_spectrum_flag;
	/* for glyphsets */
	struct cmzn_font *font;
	/* for surfaces */
	enum cmzn_graphics_render_polygon_mode render_polygon_mode;
	/* for rendering lines in GL, positive value; default 1.0 */
	double render_line_width;
	/* for rendering points in GL, positive value; default 1.0 */
	double render_point_size;

	/* rendering information */
	/* the graphics_object generated for this settings */
	struct GT_object *graphics_object;
	/* flag indicating the graphics_object needs rebuilding */
	int graphics_changed;
	/* for incremental build: last completed element index to start after (or before first if INVALID) */
	DsLabelIndex incrementalBuildIndex;
	/* flag indicating that selected graphics have changed */
	int selected_graphics_changed;
	/* flag indicating that this settings needs to be regenerated when time changes */
	bool timeDependent;
	enum cmzn_scenecoordinatesystem coordinate_system;
// 	/* for accessing objects */
	int access_count;

	inline cmzn_graphics *access()
	{
		++access_count;
		return this;
	}

	static inline int deaccess(cmzn_graphics **graphics_address);

	/** Update internal flag to match whether any graphics field, glyph or other
	  * member is time dependent */
	void updateTimeDependence();

	bool dataFieldIsTimeDependent() const
	{
		return (0 != this->data_field)
			&& Computed_field_has_multiple_times(this->data_field);
	}

	bool pointGlyphScalingIsTimeDependent() const
	{
		if ((this->point_orientation_scale_field)
			&& Computed_field_has_multiple_times(this->point_orientation_scale_field))
			return true;
		if ((this->signed_scale_field)
			&& Computed_field_has_multiple_times(this->signed_scale_field))
			return true;
		return false;
	}

	bool coordinateFieldIsTimeDependent() const
	{
		return (0 != this->coordinate_field)
			&& Computed_field_has_multiple_times(this->coordinate_field);
	}

	bool subgroupFieldIsTimeDependent() const
	{
		return (0 != this->subgroup_field)
			&& Computed_field_has_multiple_times(this->subgroup_field);
	}

	bool isoscalarFieldIsTimeDependent() const
	{
		return (0 != this->isoscalar_field)
			&& Computed_field_has_multiple_times(this->isoscalar_field);
	}

	/** @return  Non-accessed scene. Can be NULL if editor copy. */
	cmzn_scene *getScene() const
	{
		return this->scene;
	}
};

struct cmzn_graphics_module;

typedef enum cmzn_graphics_type cmzn_graphics_type_enum;

PROTOTYPE_ENUMERATOR_FUNCTIONS(cmzn_graphics_type);

enum cmzn_graphics_string_details
{
  GRAPHICS_STRING_GEOMETRY,
	GRAPHICS_STRING_COMPLETE,
	GRAPHICS_STRING_COMPLETE_PLUS
}; /* enum cmzn_graphics_string_details */

/***************************************************************************//**
 * Data for formating output with GT_element_settings_list_contents function.
 */
struct cmzn_graphics_list_data
{
	const char *line_prefix,*line_suffix;
	enum cmzn_graphics_string_details graphics_string_detail;
}; /* cmzn_graphics_list_data */

struct cmzn_graphics_range
{
	struct Graphics_object_range_struct *graphics_object_range;
	cmzn_scenefilter_id filter;
	enum cmzn_scenecoordinatesystem coordinate_system;
};

/**
 * Tracks the time spent building graphics in this increment so it can be stopped
 * when enough time elapses to just keep the client UI responsive. This is used
 * primarily to avoid very large models or large tessellation divisions from
 * causing zinc to seemingly lock up.
 */
class GraphicsIncrementalBuild
{
private:
	clock_t startClock; // process clock ticks when this object created
	clock_t clockLimit; // limit on work to do in incremental build, in clock units
	bool moreWorkToDo; // set once increment done, but more work to do i.e. another increment needed

public:

	/**
	 * @param timeLimitIn  Target duration of incremental update, in seconds.
	 */
	GraphicsIncrementalBuild(double timeLimitIn = 1.0) :
		startClock(clock()),
		clockLimit(static_cast<clock_t>(timeLimitIn*CLOCKS_PER_SEC)),
		moreWorkToDo(false)
	{
		if (timeLimitIn <= 0.0)
			clockLimit = 0;
		else if (clockLimit == 0)
			clockLimit = 1; // ensure at least 1 clock unit
	}

	~GraphicsIncrementalBuild()
	{}

	/**
	 * Returns true if elapsed time exceeds time limit for this incremental build.
	 * Note every few hours (~59 in Windows) the clock cycles to negative in which
	 * case the increment goes on indefinitely. This is acceptable as don't want
	 * this call to take any longer.
	 */
	bool incrementDone() const
	{
		return (clock() - this->startClock) > clockLimit;
	}

	/**
	 * Query whether further incremental build(s) needed.
	 */
	bool isMoreWorkToDo() const
	{
		return this->moreWorkToDo;
	}

	/**
	 * Record that not all graphics are completely built, so another incremental build is needed.
	 */
	void setMoreWorkToDo()
	{
		this->moreWorkToDo = true;
	}
};

struct cmzn_graphics_to_graphics_object_data
{
	cmzn_fieldcache_id field_cache;
	/* graphics object names are preceded by this */
	const char *name_prefix;
	/* default_rc_coordinate_field to use if NULL in any settings */
	struct Computed_field *rc_coordinate_field,
		*wrapper_orientation_scale_field,*wrapper_stream_vector_field,
		*selection_group_field;
	GT_object *glyph_gt_object;
	struct cmzn_region *region;
	cmzn_fieldmodule_id field_module;
	struct FE_region *fe_region;
	// master mesh being converted into graphics:
	cmzn_mesh_id master_mesh;
	// different from master mesh if iterating over group; can ignore subgroup_field if set:
	cmzn_mesh_id iteration_mesh;
	FE_value time;
	GraphicsIncrementalBuild *incrementalBuild;
	/* flag indicating that graphics_objects be built for all visible settings
		 currently without them */
	int build_graphics;
	/** The number of components in the data field */
	int number_of_data_values;
	/** A buffer allocated large enough for evaluating the data field */
	FE_value *data_copy_buffer;

	struct Iso_surface_specification *iso_surface_specification;
	struct cmzn_scenefilter *scenefilter;
	/* additional values for passing to element_to_graphics_object */
	struct cmzn_graphics *graphics;
	int top_level_number_in_xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
};

struct cmzn_graphics_field_change_data
{
	cmzn_fieldmoduleevent *event;
	bool selection_changed;
};

DECLARE_LIST_TYPES(cmzn_graphics);

/***************************************************************************//**
 * Created with access_count = 1.
 */
struct cmzn_graphics *CREATE(cmzn_graphics)(
	enum cmzn_graphics_type graphics_type);

int DESTROY(cmzn_graphics)(struct cmzn_graphics **graphicss_address);

/***************************************************************************//**
 * Adds the <graphics> to <list_of_graphics> at the given <position>, where 1 is
 * the top of the list (rendered first), and values less than 1 or greater than the
 * last position in the list cause the graphics to be added at its end, with a
 * position one greater than the last.
 * @param graphics The graphics to be added
 * @param position Given position of the item
 * @param list_of_graphics The list of graphics
 * @return If successfully add graphics to list of graphics returns 1, otherwise 0
 */
int cmzn_graphics_add_to_list(struct cmzn_graphics *graphics,
	int position,struct LIST(cmzn_graphics) *list_of_graphics);

/***************************************************************************//**
 * Removes the <graphics> from <list_of_graphics> and decrements the position
 * of all subsequent graphics.
 */
int cmzn_graphics_remove_from_list(struct cmzn_graphics *graphics,
	struct LIST(cmzn_graphics) *list_of_graphics);

/***************************************************************************//**
 * Changes the contents of <graphics> to match <new_graphics>, with no change in
 * position in <list_of_graphics>.
 */
int cmzn_graphics_modify_in_list(struct cmzn_graphics *graphics,
	struct cmzn_graphics *new_graphics,
	struct LIST(cmzn_graphics) *list_of_graphics);

PROTOTYPE_OBJECT_FUNCTIONS(cmzn_graphics);
PROTOTYPE_LIST_FUNCTIONS(cmzn_graphics);
PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(cmzn_graphics,position,int);

int cmzn_graphics::deaccess(cmzn_graphics **graphics_address)
{
	return DEACCESS(cmzn_graphics)(graphics_address);
}

/***************************************************************************//**
 * Returns a string summarising the graphics type, name and subset field.
 * @param graphics  The graphics.
 * @return  Allocated string.
 */
char *cmzn_graphics_get_summary_string(struct cmzn_graphics *graphics);

/***************************************************************************//**
 * Returns a string describing the graphics, suitable for entry into the command
 * line. Parameter <graphics_detail> selects whether appearance graphics are
 * included in the string. User must remember to DEALLOCATE the name afterwards.
 * ???RC When we have editing of graphics, rather than creating from scratch each
 * time as we do now with our text commands, we must ensure that defaults are
 * restored by commands generated by this string, eg. must have coordinate NONE
 * if no coordinate field. Currently only write if we have a field.
 * @param graphics The cmiss graphics
 * @param graphics_detail given detail of the string
 * @return the cmzn_graphics_string
 */
char *cmzn_graphics_string(struct cmzn_graphics *graphics,
	enum cmzn_graphics_string_details graphics_detail);

/***************************************************************************//**
 * @return  1 if both graphics and its scene visibility flags are set,
 * otherwise 0.
 */
int cmzn_graphics_and_scene_visibility_flags_is_set(struct cmzn_graphics *graphics);

/***************************************************************************//**
 * @return  1 if graphics is one of the graphical representations of region,
 * otherwise 0.
 */
int cmzn_graphics_is_from_region_hierarchical(struct cmzn_graphics *graphics, struct cmzn_region *region);

/**
 * Returns true if graphics is generated from elements and they are pickable via it.
 */
bool cmzn_graphics_selects_elements(struct cmzn_graphics *graphics);

/**
 * Returns the dimension of the domain used in the graphics. Note for domain
 * type CMZN_DOMAIN_MESH_HIGHEST_DIMENSION the region is checked for the
 * highest dimension.
 * @param graphics  The graphics to query.
 * @return  The dimension of the graphics domain, or -1 on error.
 */
int cmzn_graphics_get_domain_dimension(struct cmzn_graphics *graphics);

#if defined (USE_OPENCASCADE)
/**
 * Returns 1 if the graphics are output with names that identify
 * the elements they are calculated from.  Otherwise it returns 0.
 */
int cmzn_graphics_selects_cad_primitives(struct cmzn_graphics *graphics);
#endif /* defined (USE_OPENCASCADE) */

int cmzn_graphics_to_graphics_object(
	struct cmzn_graphics *graphics,void *graphics_to_object_data_void);

/***************************************************************************//**
 * If the settings visibility flag is set and it has a graphics_object, the
 * graphics_object is compiled.
 * @param graphics The graphics to be edit
 * @param renderer_void Void pointer to the renderer
 * @return If successfully compile visible setting returns 1, else 0
 */
int cmzn_graphics_compile_visible_graphics(
	struct cmzn_graphics *graphics, void *renderer_void);

/***************************************************************************//**
 * If the settings visibility flag is set and it has a graphics_object, the
 * graphics_object is executed, while the position of the settings in the list
 * is put out as a name to identify the object in OpenGL picking.
 * @param graphics The graphics to be edit
 * @param dummy_void void pointer to NULL
 * @return If successfully execute visible graphics returns 1, else 0
 */
int cmzn_graphics_execute_visible_graphics(
	struct cmzn_graphics *graphics, void *renderer_void);

int cmzn_graphics_get_visible_graphics_object_range(
	struct cmzn_graphics *graphics,void *graphics_range_void);

struct GT_object *cmzn_graphics_get_graphics_object(
	struct cmzn_graphics *graphics);

/**
 * Fills the top_level_number_in_xi array with the discretization computed for
 * the graphics taking into account the tessellation and non-linearity of the
 * coordinate field.
 *
 * @param max_dimensions  Size of supplied top_level_number_in_xi array.
 * @param top_level_number_in_xi  Array to receive values.
 */
int cmzn_graphics_get_top_level_number_in_xi(struct cmzn_graphics *graphics,
	int max_dimensions, int *top_level_number_in_xi);

/**
 * Get iso surface decimation threshold.
 */
double cmzn_graphics_contours_get_decimation_threshold(
	cmzn_graphics_contours_id contours);

/**
 * Set iso surface decimation threshold, controlling polygon reduction.
 * Converts iso surface into a voltex first.
 * @return  Status CMZN_OK on success, otherwise CMZN_ERROR_ARGUMENT.
 */
int cmzn_graphics_contours_set_decimation_threshold(
	cmzn_graphics_contours_id contours, double decimation_threshold);

/***************************************************************************//**
 * For graphics starting in a particular element.
 */
struct FE_element *cmzn_graphics_get_seed_element(
	struct cmzn_graphics *graphics);

int cmzn_graphics_set_seed_element(struct cmzn_graphics *graphics,
	struct FE_element *seed_element);

/***************************************************************************//**
 * Copies the graphics contents from source to destination.
 * Notes:
 * destination->access_count is not changed by COPY.
 * graphics_object is NOT copied; destination->graphics_object is cleared.
 * @param destination Copy the graphics to this address
 * @param source the source cmiss graphics
 * @return If successfully copy the graphics returns 1, else 0
 */
int cmzn_graphics_copy_without_graphics_object(
	struct cmzn_graphics *destination, struct cmzn_graphics *source);

/***************************************************************************//**
 * Graphics iterator function returning true if <graphics> has the
 * specified <name>.  If the graphics doesn't have a name then the position
 * number is converted to a string and that is compared to the supplied <name>.
 * @param graphics The graphics
 * @param name_void void pointer to name
 * @return If graphics has name <name> returns 1, else 0
 */
int cmzn_graphics_has_name(struct cmzn_graphics *graphics,
	void *name_void);

/***************************************************************************//**
 * cmzn_graphics list conditional function returning 1 if graphics has the
 * name.
 */
int cmzn_graphics_same_name(struct cmzn_graphics *graphics,
	void *name_void);

/***************************************************************************//**
 * Writes out the <graphics> as a text string in the command window with the
 * <graphics_string_detail>, <line_prefix> and <line_suffix> given in the
 * <list_data>.
 */
int cmzn_graphics_list_contents(struct cmzn_graphics *graphics,
	void *list_data_void);

/***************************************************************************//**
 * Returns the position of <graphics> in <list_of_grpahic>.
 */
int cmzn_graphics_get_position_in_list(
	struct cmzn_graphics *graphics,
	struct LIST(cmzn_graphics) *list_of_graphics);

/***************************************************************************//**
 * Returns true if <graphics1> and <graphics2> would produce identical graphics.
 */
int cmzn_graphics_match(struct cmzn_graphics *graphics1,
	struct cmzn_graphics *graphics2);

/***************************************************************************//**
 * cmzn_graphics iterator function for copying a list_of_graphics.
 * Makes a copy of the graphics and puts it in the list_of_graphics.
 */
int cmzn_graphics_copy_and_put_in_list(
	struct cmzn_graphics *graphics,void *list_of_graphics_void);

/***************************************************************************//**
 *Returns 1 if the graphics are of the specified graphics_type.
 */
int cmzn_graphics_type_matches(struct cmzn_graphics *graphics,
	void *graphics_type_void);

/***************************************************************************//**
 * If <graphics> does not already have a graphics object, this function attempts
 * to find graphics in <list_of_graphics> which differ only trivially in material,
 * spectrum etc. AND have a graphics object. If such a graphics is found, the
 * graphics_object is moved from the matching graphics and put in <graphics>, while
 * any trivial differences are fixed up in the graphics_obejct.
 */
int cmzn_graphics_extract_graphics_object_from_list(
	struct cmzn_graphics *graphics,void *list_of_graphics_void);

/***************************************************************************//**
 * Same as cmzn_graphics_same_non_trivial except <graphics> must also have
 * a graphics_object. Used for getting graphics objects from previous graphics
 * that are the same except for trivial differences such as the material and
 * spectrum which can be changed in the graphics object to match the new graphics .
 */
int cmzn_graphics_same_non_trivial_with_graphics_object(
	struct cmzn_graphics *graphics,void *second_graphics_void);

/***************************************************************************//**
 * Gets the graphics's stored name, or generates one based on its position.
 * @return  Allocated string.
 */
char *cmzn_graphics_get_name_internal(struct cmzn_graphics *graphics);

int cmzn_graphics_time_change(
	struct cmzn_graphics *graphics,void *dummy_void);

/** List iterator function for updating current dependence of graphics on time.
  * @param time_dependent_bool_void  Void pointer to a bool, set to True if time dependent. */
int cmzn_graphics_update_time_dependence(struct cmzn_graphics *graphics, void *time_dependent_bool_void);

/**
 * Pass fieldmoduleevent and selection change to graphics
 * @param change_data_void  A struct cmzn_graphics_field_change_data.
 */
int cmzn_graphics_field_change(struct cmzn_graphics *graphics,
	void *change_data_void);

/**
 * Inform graphics of changes in the glyph manager. Marks graphics for redraw if
 * uses a changed glyph, and propagates change to owning scene.
 * @param manager_message_void  A struct MANAGER_MESSAGE(cmzn_glyph) *.
 */
int cmzn_graphics_glyph_change(struct cmzn_graphics *graphics,
	void *manager_message_void);

/***************************************************************************//**
 * Inform graphics of changes in the material manager. Marks affected
 * graphics for rebuilding and sets flag for informing clients of scene.
 * Note: only graphics combining a material with data/spectrum are updated;
 * pure material changes do not require update.
 *
 * @param material_manager_message_void  MANAGER_MESSAGE(Material).
 */
int cmzn_material_change(struct cmzn_graphics *graphics,
	void *material_change_data_void);

/***************************************************************************//**
 * Inform graphics of changes in the spectrum manager. Marks affected
 * graphics for rebuilding and sets flag for informing clients of scene.
 *
 * @param spectrum_manager_message_void  MANAGER_MESSAGE(cmzn_spectrum).
 */
int cmzn_graphics_spectrum_change(struct cmzn_graphics *graphics,
	void *spectrum_manager_message_void);

/***************************************************************************//**
 * Inform graphics of changes in the tessellation manager. Marks affected
 * graphics for rebuilding and sets flag for informing clients of scene.
 *
 * @param tessellation_manager_message_void  Pointer to
 * struct MANAGER_MESSAGE(cmzn_tessellation).
 */
int cmzn_graphics_tessellation_change(struct cmzn_graphics *graphics,
	void *tessellation_manager_message_void);

/***************************************************************************//**
 * Inform graphics of changes in the font manager. Marks affected
 * graphics for rebuilding and sets flag for informing clients of scene.
 *
 * @param font_manager_message_void  Pointer to
 * struct MANAGER_MESSAGE(cmzn_font).
 */
int cmzn_graphics_font_change(struct cmzn_graphics *graphics,
	void *font_manager_message_void);

/* Overlay disabled
int cmzn_graphics_enable_overlay(struct cmzn_graphics *graphics, int flag);

int cmzn_graphics_is_overlay(struct cmzn_graphics *graphics);

int cmzn_graphics_set_overlay_order(struct cmzn_graphics *graphics, int order);

int cmzn_graphics_get_overlay_order(struct cmzn_graphics *graphics);
*/

/***************************************************************************//**
 * This function will deaccess any computed fields being used by graphics, this
 * should only be called from cmzn_scene::detachFields.
 *
 * @param graphics  pointer to the graphics.
 * @return Return 1 if successfully detach fields from graphics otherwise 0.
 */
int cmzn_graphics_detach_fields(struct cmzn_graphics *graphics, void *dummy_void);

int cmzn_graphics_set_scene_private(struct cmzn_graphics *graphics,
	struct cmzn_scene *scene);

int cmzn_graphics_selected_element_points_change(
	struct cmzn_graphics *graphics,void *dummy_void);

/***************************************************************************//**
 * A function to call set_scene_private but with a void pointer to the
 * scene passing into the function for list macro.
 *
 * @param graphics  pointer to the graphics.
 * @param scene_void  void pointer to the scene.
 * @return Return 1 if successfully set the graphics.
 */
int cmzn_graphics_set_scene_for_list_private(
		struct cmzn_graphics *graphics, void *scene_void);

int cmzn_graphics_update_selected(struct cmzn_graphics *graphics, void *dummy_void);

void cmzn_graphics_flag_glyph_has_changed(struct cmzn_graphics *graphics);

int cmzn_graphics_set_renderer_highlight_functor(struct cmzn_graphics *graphics, void *renderer_void);

int cmzn_graphics_remove_renderer_highlight_functor(struct cmzn_graphics *graphics,
	void *renderer_void);

int cmzn_graphics_flag_for_full_rebuild(
	struct cmzn_graphics *graphics,void *dummy_void);

enum GT_object_type cmzn_graphics_get_graphics_object_type(struct cmzn_graphics *graphics);

struct GT_object *cmzn_graphics_copy_graphics_object(struct cmzn_graphics *graphics);

enum cmzn_graphics_select_mode cmzn_graphics_select_mode_enum_from_string(
	const char *string);

char *cmzn_graphics_select_mode_enum_to_string(enum cmzn_graphics_select_mode type);

enum cmzn_graphicslineattributes_shape_type cmzn_graphicslineattributes_shape_type_enum_from_string(const char *string);

char *cmzn_graphicslineattributes_shape_type_enum_to_string(enum cmzn_graphicslineattributes_shape_type type);

enum cmzn_graphics_streamlines_track_direction cmzn_graphics_streamlines_track_direction_enum_from_string(
	const char *string);

char *cmzn_graphics_streamlines_track_direction_enum_to_string(
	enum cmzn_graphics_streamlines_track_direction direction);

enum cmzn_graphics_streamlines_colour_data_type cmzn_graphics_streamlines_colour_data_type_enum_from_string(
	const char *string);

char *cmzn_graphics_streamlines_colour_data_type_enum_to_string(
	enum cmzn_graphics_streamlines_colour_data_type type);

bool cmzn_graphicspointattributes_contain_surfaces(cmzn_graphicspointattributes_id point_attributes);

#endif
