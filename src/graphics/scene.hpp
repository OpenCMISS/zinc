/*******************************************************************************
FILE : scene.h

==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (SCENE_H)
#define SCENE_H

#include <list>
#include <map>
#include <string>
#include "opencmiss/zinc/scene.h"
#include "computed_field/computed_field.h"
#include "general/manager.h"
#include "general/object.h"
#include "graphics/graphics.h"
#include "graphics/graphics_library.h"
#include "context/context.hpp"
#include "region/cmiss_region.hpp"
#include "opencmiss/zinc/types/timenotifierid.h"
#include "general/enumerator_private.hpp"

typedef std::list<cmzn_selectionnotifier *> cmzn_selectionnotifier_list;

typedef std::map<cmzn_field *, std::pair<cmzn_field *, int> > SceneCoordinateFieldWrapperMap;
	// int = usage count
typedef std::map<std::pair<cmzn_field *,cmzn_field*>, std::pair<cmzn_field *, int> > SceneVectorFieldWrapperMap;
	// pair of fields are in order vector, coordinate field; int = usage count

/**
 * Structure for maintaining a graphical scene of region.
 */
struct cmzn_scene
{
	/* the region being drawn */
	struct cmzn_region *region;
	cmzn_fieldmodulenotifier *fieldmodulenotifier;
	/* settings shared by whole scene */
	// legacy general settings used as defaults for new graphics
	struct Computed_field *default_coordinate_field;
	int *element_divisions;
	int element_divisions_size;
	int circle_discretization;
	/* list of objects interested in changes to the cmzn_scene */
	struct cmzn_scene_callback_data *update_callback_list;
	struct LIST(cmzn_graphics) *list_of_graphics;
	/* level of cache in effect */
	int cache;
	int changed; /**< true if scene has changed and haven't updated clients */
	bool visibility_flag;
	// transformation data
	bool transformationActive;
	bool transformationMatrixColumnMajor; // default false = indexes across rows fastest; true = OpenGL-style column major
	double transformationMatrix[16];
	cmzn_field *transformationField;
	struct cmzn_graphics_module *graphics_module;
	cmzn_timenotifier_id time_notifier;
	/* callback list for transformation changes */
	struct LIST(CMZN_CALLBACK_ITEM(cmzn_scene_transformation)) *transformation_callback_list;
	struct LIST(CMZN_CALLBACK_ITEM(cmzn_scene_top_region_change)) *top_region_change_callback_list;
	unsigned int picking_name;
	cmzn_field_group_id selection_group;
	bool selectionChanged;
	cmzn_selectionnotifier_list *selectionnotifier_list;
	bool editorCopy; // is this a temporary scene for Cmgui Scene editor?
	/* for accessing objects */
	int access_count;

private:
	SceneCoordinateFieldWrapperMap coordinateFieldWrappers;
	SceneVectorFieldWrapperMap vectorFieldWrappers;

private:
	cmzn_scene(cmzn_region *regionIn, cmzn_graphics_module *graphicsmoduleIn);

	~cmzn_scene();

	void transformationChange();

	void updateTimeDependence();

public:

	/** Deaccess any fields used by scene and its graphics. */
	void detachFields();

	/** Remove references to external objects and callbacks from them.
	  * This function is called when:
	  * - owning region is being destroyed
	  * - context is being destroyed */
	void detachFromOwner();

	/**
	 * @return Non-accessed scene, or nullptr on failure.
	 */
	static cmzn_scene *create(cmzn_region *regionIn, cmzn_graphics_module *graphicsmoduleIn);

	/** Special copy constructor making an independent copy of scene with its
	 * graphics without graphics objects. Avoids setting up change callbacks etc.
	 * @return Accessed scene, or nullptr on failure.
	 */
	static cmzn_scene *createCopy(const cmzn_scene& source);

	inline cmzn_scene *access()
	{
		++(this->access_count);
		return this;
	}

	static inline int deaccess(cmzn_scene* &scene)
	{
		if (scene)
		{
			if (--(scene->access_count) == 0)
				delete scene;
			scene = 0;
			return CMZN_OK;
		}
		return CMZN_ERROR_ARGUMENT;
	}

	static inline void reaccess(cmzn_scene* &scene, cmzn_scene *newScene)
	{
		if (newScene)
			++(newScene->access_count);
		if ((scene) && (--(scene->access_count) == 0))
			delete scene;
		scene = newScene;
	}

	void addSelectionnotifier(cmzn_selectionnotifier *selectionnotifier);

	void removeSelectionnotifier(cmzn_selectionnotifier *selectionnotifier);

	/** Register that the coordinate field is in use by graphics.
	 * Ensures a wrapper RC coordinate field exists for it. */
	void registerCoordinateField(cmzn_field *coordinateField);

	/** Remove wrapping for this instance of coordinate field. */
	void deregisterCoordinateField(cmzn_field *coordinateField);

	/** Get the wrapper for coordinate field; will be itself if RC.
	 * @return  Non-accessed field. */
	cmzn_field *getCoordinateFieldWrapper(cmzn_field *coordinateField);

	/** Register that the vector field is in use with coordinate field in graphics.
	* Ensures a wrapper RC vector field exists for it. Note it is required that coordinateField has a wrapper! */
	void registerVectorField(cmzn_field *vectorField, cmzn_field *coordinateField);

	/** Remove wrapping for this instance of vector and coordinate field. */
	void deregisterVectorField(cmzn_field *vectorField, cmzn_field *coordinateField);

	/** Get the wrapper for vector field with coordinate field; will be itself if both are RC.
	 * @return  Non-accessed field. */
	cmzn_field *getVectorFieldWrapper(cmzn_field *vectorField, cmzn_field *coordinateField);

	/** Check/ensure registered coordinate and vector fields have appropriate wrappers.
	 * Call if definition of fields changed. */
	void refreshFieldWrappers();

	/** Evaluate transformation matrix from field at current time
	  * @return  Result OK on success, any other value on failure */
	int evaluateTransformationMatrixFromField();

	bool isEditorCopy() const
	{
		return this->editorCopy;
	}

	cmzn_graphics_module *getGraphicsmodule() const
	{
		return this->graphics_module;
	}

	/** @return  Non-accessed pointer to parent scene, or nullptr if none */
	cmzn_scene *getParent() const
	{
		cmzn_region *parentRegion = (this->region) ? this->region->getParent() : nullptr;
		return (parentRegion) ? parentRegion->getScene() : nullptr;
	}

	/** @return  Non-accessed timekeeper for scene, or 0 if failed */
	cmzn_timekeeper *getTimekeeper();

	void clearTransformation();

	bool isTransformationActive() const
	{
		return this->transformationActive;
	}

	bool isTransformationMatrixColumnMajor() const
	{
		return this->transformationMatrixColumnMajor;
	}

	void setTransformationMatrixColumnMajor(bool columnMajor);

	/** @return  Non-accessed transformation field or NULL if none */
	cmzn_field *getTransformationField() const
	{
		return this->transformationField;
	}

	int setTransformationField(cmzn_field *transformationFieldIn);

	int getTransformationMatrix(double *valuesOut16);

	int getTransformationMatrixRowMajor(double *valuesOut16);

	int setTransformationMatrix(const double *valuesIn16);

	/** Fill matrix4x4 with row major transformation matrix from parent coordinate
	  * system of topScene to local coordinate system of this scene.
	  * @return  OK if valid matrix returned, NOT_FOUND if no transformation, any
	  * other value if failed. */
	int getTotalTransformationMatrix(cmzn_scene* topScene, double* matrix4x4);

	int getCoordinatesRange(cmzn_scenefilter *filter, double *minimumValuesOut3,
		double *maximumValuesOut3);

	/** Returns graphics coordinates range as centre, size. Zeros them if any error. */
	int getCoordinatesRangeCentreSize(cmzn_scenefilter *filter, double *centre3,
		double *size3);

	void timeChange(cmzn_timenotifierevent_id timenotifierevent);

	/** Notify registered clients of change in the scene */
	void notifyClients();

	void beginChange()
	{
		++(this->cache);
	}

	void endChange()
	{
		--(this->cache);
		if ((0 == this->cache) && this->changed)
			this->notifyClients();
	}

	/** Mark scene as changed, needing rebuild and notify clients unless caching changed. 
	  * For internal use only; called by changed graphics to owning scene. */
	void setChanged()
	{
		this->changed = 1;
		if (0 == this->cache)
			this->notifyClients();
	}

	void notifySelectionevent(cmzn_selectionevent_id selectionevent);

	void processFieldmoduleevent(cmzn_fieldmoduleevent *event);

}; /* struct cmzn_scene */

struct MANAGER_MESSAGE(cmzn_tessellation);

typedef int(*cmzn_scene_callback)(struct cmzn_scene *scene,
	void *user_data);

DECLARE_CMZN_CALLBACK_TYPES(cmzn_scene_transformation, struct cmzn_scene *, \
	void *, void);

DECLARE_CMZN_CALLBACK_TYPES(cmzn_scene_top_region_change, struct cmzn_scene *, \
	struct cmzn_scene *, void);

/***************************************************************************//**
 * Wrapper for accessing the list of graphics in <cmzn_scene>.
 * @param scene target for that scene
 * @param conditional_function conditional function for the list
 * @param data void pointer to data to pass into the conditional function
 * @return Return the first graphics that fullfill the conditional function
 */
struct cmzn_graphics *cmzn_scene_get_first_graphics_with_condition(
	struct cmzn_scene *scene,
	LIST_CONDITIONAL_FUNCTION(cmzn_graphics) *conditional_function,
	void *data);

/***************************************************************************//**
 * Adds a callback routine which is called whenever a cmzn_scene is aware of
 * changes.
 */
int cmzn_scene_add_callback(struct cmzn_scene *scene,
	cmzn_scene_callback callback, void *user_data);

/***************************************************************************//**
 * Removes a callback which was added previously
 */
int cmzn_scene_remove_callback(struct cmzn_scene *scene,
	cmzn_scene_callback callback, void *user_data);

/***************************************************************************//**
 * Attempt to guess which field is the most appropriate to use as a coordinate
 * field for graphics.
 * @param scene  The scene whose graphics need a coordinate field.
 * @param domain_type  Type of domain to get coordinate field for. Not used
 * currently.
 * @return non-accessed field
 */
cmzn_field_id cmzn_scene_guess_coordinate_field(
	struct cmzn_scene *scene, cmzn_field_domain_type domain_type);

/***************************************************************************//**
 * Iterates through every material used by the scene.
 */
int cmzn_scene_for_each_material(struct cmzn_scene *scene,
	MANAGER_ITERATOR_FUNCTION(cmzn_material) *iterator_function,
	void *user_data);

/***************************************************************************//**
 * Lists the general graphics defined for <scene> - as a
 * set of commands that can be used to reproduce the groups appearance. The
 * <command_prefix> should generally contain "gfx modify g_element" while the
 * optional <command_suffix> may describe the scene (eg. "scene default").
 * Note the command prefix is expected to contain the name of the region.
 */
int cmzn_scene_list_commands(struct cmzn_scene *scene,
	const char *command_prefix, const char *command_suffix);

/**
 * Private method for informing scene of glyph manager changes.
 * Propagates changes hierarchically to child scenes to minimise messages.
 * Should only be called by cmzn_graphics_module.
 */
void cmzn_scene_glyph_change(struct cmzn_scene *scene,
	struct MANAGER_MESSAGE(cmzn_glyph) *manager_message);

/***************************************************************************//**
 * Private method for informing scene of material manager changes.
 * Should only be called by cmzn_graphics_module.
 */
void cmzn_scene_material_change(struct cmzn_scene *scene,
	struct MANAGER_MESSAGE(cmzn_material) *manager_message);

/***************************************************************************//**
 * Private method for informing scene of spectrum manager changes.
 * Should only be called by cmzn_graphics_module.
 */
void cmzn_scene_spectrum_change(struct cmzn_scene *scene,
	struct MANAGER_MESSAGE(cmzn_spectrum) *manager_message);

/***************************************************************************//**
 * Private method for informing scene of tessellation manager changes.
 * Should only be called by cmzn_graphics_module.
 */
void cmzn_scene_tessellation_change(struct cmzn_scene *scene,
	struct MANAGER_MESSAGE(cmzn_tessellation) *manager_message);

/***************************************************************************//**
 * Private method for informing scene of font manager changes.
 * Should only be called by cmzn_graphics_module.
 */
void cmzn_scene_font_change(struct cmzn_scene *scene,
	struct MANAGER_MESSAGE(cmzn_font) *manager_message);

int for_each_child_scene_in_scene_tree(
	struct cmzn_scene *scene,
	int (*cmzn_scene_tree_iterator_function)(struct cmzn_scene *scene,
		void *user_data),	void *user_data);

/***************************************************************************//**
 * Returns the position of <graphics> in <scene>.
 */
int cmzn_scene_get_graphics_position(
	struct cmzn_scene *scene,
	struct cmzn_graphics *graphics);

/***************************************************************************//**
 * Returns true if <scene1> and <scene2> match in
 * main attributes of scene, graphics etc. such that they would produce
 * the same graphics.
 */
int cmzn_scenes_match(struct cmzn_scene *scene1,
	struct cmzn_scene *scene2);

int for_each_graphics_in_cmzn_scene(struct cmzn_scene *scene,
	int (*cmzn_scene_graphics_iterator_function)(struct cmzn_graphics *graphics,
		void *user_data),	void *user_data);

/***************************************************************************//**
 * Get region of scene. Not accessed.
 */
struct cmzn_region *cmzn_scene_get_region_internal(struct cmzn_scene *scene);

/***************************************************************************//**
 *Copies the cmzn_scene contents from source to destination, keeping any
 *graphics objects from the destination that will not change with the new graphics
 *from source. Used to apply the changed cmzn_scene from the editor to the
 *actual cmzn_scene.
 */
int cmzn_scene_modify(struct cmzn_scene *destination,
	struct cmzn_scene *source);

int cmzn_scene_add_transformation_callback(struct cmzn_scene *scene,
	CMZN_CALLBACK_FUNCTION(cmzn_scene_transformation) *function, void *user_data);

int cmzn_scene_remove_transformation_callback(
	struct cmzn_scene *scene,
	CMZN_CALLBACK_FUNCTION(cmzn_scene_transformation) *function, void *user_data);

PROTOTYPE_ACCESS_OBJECT_FUNCTION(cmzn_scene);
PROTOTYPE_DEACCESS_OBJECT_FUNCTION(cmzn_scene);

/***************************************************************************//**
 * Remove selection groups from scene tree if they are empty.
 */
void cmzn_scene_flush_tree_selections(cmzn_scene_id scene);

/***************************************************************************//**
 * Set default graphics attributes depending on type, e.g. tessellation,
 * materials, etc.
 */
int cmzn_scene_set_minimum_graphics_defaults(struct cmzn_scene *scene,
	struct cmzn_graphics *graphics);

/***************************************************************************//**
 * Adds the <graphics> to <scene> at the given <position>, where 1 is
 * the top of the list (rendered first), and values less than 1 or greater than the
 * last position in the list cause the graphics to be added at its end, with a
 * position one greater than the last.
 *
 * @param scene  The handle to the scene.
 * @param graphics  The handle to the cmiss graphics which will be added to the
 *   scene.
 * @param pos  The position to put the target graphics to.
 * @return  Returns 1 if successfully add graphics to scene at pos, otherwise
 *   returns 0.
 */
int cmzn_scene_add_graphics(cmzn_scene_id scene, cmzn_graphics_id graphics, int pos);

int list_cmzn_scene_transformation_commands(struct cmzn_scene *scene,
	void *command_prefix_void);

int list_cmzn_scene_transformation(struct cmzn_scene *scene);

/***************************************************************************//**
 * Returns the status of the inherited visibility flag of the scene.
 * This function returns 0 if the specified scene or any of its parents along
 * the path has the visibility flag set to 0 otherwise return 1;
 *
 * @param scene  The handle to the scene.
 * @return  1 if scene and all its parent are visible, otherwise 0.
 */
int cmzn_scene_is_visible_hierarchical(cmzn_scene_id scene);

/***************************************************************************//**
 * Get graphics at position <pos> in the internal graphics list of scene.
 *
 * @param scene  The handle to the scene of which the graphics is located.
 * @param pos  The position of the graphics, starting from 0.
 * @return  returns the found graphics if found at pos, otherwise returns NULL.
 */
cmzn_graphics_id cmzn_scene_get_graphics_at_position(
	cmzn_scene_id scene, int pos);

cmzn_field_id cmzn_scene_get_selection_group_private_for_highlighting(
	cmzn_scene_id scene);

int cmzn_region_modify_scene(struct cmzn_region *region,
	struct cmzn_graphics *graphics, int delete_flag, int position);

/**
 * Get legacy default_coordinate_field set in cmgui command
 * "gfx modify g_element general".
 * @return  Non-accessed field, or 0 if none.
 */
cmzn_field *cmzn_scene_get_default_coordinate_field(cmzn_scene *scene);

/**
 * Set legacy default_coordinate_field from cmgui command
 * "gfx modify g_element general".
 */
int cmzn_scene_set_default_coordinate_field(cmzn_scene *scene,
	cmzn_field *default_coordinate_field);

////  new APIs
int cmzn_scene_add_total_transformation_callback(struct cmzn_scene *child_scene,
	cmzn_scene_id scene, CMZN_CALLBACK_FUNCTION(cmzn_scene_transformation) *function,
	CMZN_CALLBACK_FUNCTION(cmzn_scene_top_region_change) *region_change_function,
	void *user_data);

int cmzn_scene_remove_total_transformation_callback(struct cmzn_scene *child_scene,
	cmzn_scene_id scene, CMZN_CALLBACK_FUNCTION(cmzn_scene_transformation) *function,
	CMZN_CALLBACK_FUNCTION(cmzn_scene_top_region_change) *region_change_function,
	void *user_data);

int cmzn_scene_notify_scene_viewer_callback(struct cmzn_scene *scene,
	void *scene_viewer_void);

/**
 * Get gtMatrix containing the total transformation from the parent coordinate
 * system for the top scene to scene, or NULL if none set.
 *
 * @param scene  Scene to query.
 * @param top_scene  pointer to the the top scene.
 * @return  Allocated gtMatrix if transformation in effect, NULL if failed or
 * no transformation is set.
 */
gtMatrix *cmzn_scene_get_total_transformation(struct cmzn_scene *scene,
	struct cmzn_scene *top_scene);

typedef int(*graphics_object_tree_iterator_function)(
	struct GT_object *graphics_object, double time, void *user_data);

int for_each_graphics_object_in_scene_tree(cmzn_scene_id scene,
	cmzn_scenefilter_id filter, graphics_object_tree_iterator_function iterator_function,
	void *user_data);

int Scene_export_region_graphics_object(cmzn_scene *scene,
	cmzn_region *region, const char *graphics_name, cmzn_scenefilter_id filter,
	graphics_object_tree_iterator_function iterator_function, void *user_data);

cmzn_scene *cmzn_scene_get_child_of_picking_name(cmzn_scene *scene, int position);

int cmzn_scene_triggers_top_region_change_callback(
	struct cmzn_scene *scene);

PROTOTYPE_ENUMERATOR_FUNCTIONS(cmzn_streaminformation_scene_io_data_type);

enum cmzn_streaminformation_scene_io_data_type
	cmzn_streaminformation_scene_io_data_type_enum_from_string(const char *string);

char *cmzn_streaminformation_scene_io_data_type_enum_to_string(
	enum cmzn_streaminformation_scene_io_data_type mode);

int Scene_render_threejs(cmzn_scene_id scene,
	cmzn_scenefilter_id scenefilter, const char *filename,
	int number_of_time_steps, double begin_time, double end_time,
	cmzn_streaminformation_scene_io_data_type export_mode,
	int *number_of_entries, std::string **output_string,
	int morphColours, int morphNormals, int morphVertices,
	int numberOfFiles, char **file_names, int isInline);

int Scene_render_webgl(cmzn_scene_id scene,
	cmzn_scenefilter_id scenefilter, const char *name_prefix);

int Scene_get_number_of_graphics_with_type_in_tree(
	cmzn_scene_id scene, cmzn_scenefilter_id scenefilter, enum cmzn_graphics_type type);

/* this include surfaces graphics and line graphics with surfaces (cylinder)*/
int Scene_get_number_of_graphics_with_surface_vertices_in_tree(cmzn_scene_id scene,
	cmzn_scenefilter_id scenefilter);

/* this include graphics with  line primitves*/
int Scene_get_number_of_graphics_with_line_vertices_in_tree(cmzn_scene_id scene,
	cmzn_scenefilter_id scenefilter);

/* Only glyphs with surfaces are compatible at this moment */
int Scene_get_number_of_exportable_glyph_resources(cmzn_scene_id scene,
	cmzn_scenefilter_id scenefilter);

class Render_graphics_compile_members;
class Render_graphics_opengl;

int cmzn_scene_compile_scene(cmzn_scene *scene,
	Render_graphics_compile_members *renderer);

int cmzn_scene_compile_graphics(cmzn_scene *scene,
	Render_graphics_compile_members *renderer, int force_rebuild);

int execute_scene_exporter_output(struct cmzn_scene *scene,
	Render_graphics_opengl *renderer);

int execute_scene_threejs_output(struct cmzn_scene *scene,
	Render_graphics_opengl *renderer);

int execute_cmzn_scene(cmzn_scene *scene,
	Render_graphics_opengl *renderer);

int cmzn_scene_graphics_render_opengl(struct cmzn_scene *scene,
	Render_graphics_opengl *renderer);

int cmzn_scene_render_child_scene(struct cmzn_scene *scene,
	Render_graphics_opengl *renderer);

int Scene_render_opengl(cmzn_scene *scene, Render_graphics_opengl *renderer);

int cmzn_scene_compile_tree(cmzn_scene *scene,
	Render_graphics_compile_members *renderer);

int build_Scene(cmzn_scene *scene, cmzn_scenefilter *filter);

#endif /* !defined (SCENE_H) */

