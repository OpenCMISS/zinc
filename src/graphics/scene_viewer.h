/*******************************************************************************
FILE : scene_viewer.h

LAST MODIFIED : 6 December 2006

DESCRIPTION :
Three_D_drawing derivative for viewing a Scene from an arbitrary position.
The scene viewer has the following modes for handling user input:
SCENE_VIEWER_NO_INPUT ignores any input, leaving it up to the owner of the
scene viewer to set viewing parameters.
SCENE_VIEWER_SELECT performs OpenGL picking and returns the picked objects
to the scene via a callback, along with mouse button press and motion
information in a view-independent format.
SCENE_VIEWER_TRANSFORM allows the view of the scene to be changed by tumbling,
translating and zooming with mouse button press and motion events.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (SCENE_VIEWER_H)
#define SCENE_VIEWER_H

#include "opencmiss/zinc/types/scenecoordinatesystem.h"
#include "opencmiss/zinc/sceneviewer.h"
#include "general/callback.h"
#include "general/enumerator.h"
#include "general/image_utilities.h"
#include "general/object.h"
#include "graphics/colour.h"
#include "graphics/light.hpp"
#include "graphics/render_gl.h"
#include <list>

struct Graphics_buffer;
#define Graphics_buffer_input cmzn_sceneviewerinput
#define Graphics_buffer_input_event_type cmzn_sceneviewerinput_event_type

struct cmzn_sceneviewerevent
{
	cmzn_sceneviewerevent_change_flags changeFlags;
	int access_count;

	cmzn_sceneviewerevent() :
		changeFlags(CMZN_SCENEVIEWEREVENT_CHANGE_FLAG_NONE),
		access_count(1)
	{
	}

	~cmzn_sceneviewerevent()
	{
	}

	cmzn_sceneviewerevent *access()
	{
		++(this->access_count);
		return this;
	}

	static int deaccess(cmzn_sceneviewerevent* &event);

};

struct cmzn_sceneviewernotifier
{
private:
	cmzn_sceneviewer_id sceneviewer; // owning region: not accessed
	cmzn_sceneviewernotifier_callback_function function;
	void *user_data;
	int access_count;

	cmzn_sceneviewernotifier(cmzn_sceneviewer *sceneviewer);

	~cmzn_sceneviewernotifier();

public:

	/** private: external code must use cmzn_sceneviewer_create_sceneviewernotifier */
	static cmzn_sceneviewernotifier *create(cmzn_sceneviewer *sceneviewer)
	{
		if (sceneviewer)
			return new cmzn_sceneviewernotifier(sceneviewer);
		return 0;
	}

	cmzn_sceneviewernotifier *access()
	{
		++(this->access_count);
		return this;
	}

	static int deaccess(cmzn_sceneviewernotifier* &notifier);

	int setCallback(cmzn_sceneviewernotifier_callback_function function_in,
		void *user_data_in);

	void *getUserData()
	{
		return this->user_data;
	}

	void clearCallback();

	void sceneviewerDestroyed();

	void notify(cmzn_sceneviewerevent *event)
	{
		if (this->function && event)
			(this->function)(event, this->user_data);
	}

};

typedef std::list<cmzn_sceneviewernotifier *> cmzn_sceneviewernotifier_list;

struct cmzn_sceneviewermodule;

/*
The cmzn_sceneviewer which is Public is currently the same object as the
cmgui internal Scene_viewer.  The Public interface is contained in
zinc/sceneviewer.h however most of the functions come directly from
this module.  So that these functions match the public declarations the
struct Scene_viewer is declared to be the same as cmzn_sceneviewer here
and the functions given their public names.
*/
/* Convert the type */
#define Scene_viewer cmzn_sceneviewer

/* Convert the functions that have identical interfaces */
#define Scene_viewer_get_depth_of_field cmzn_sceneviewer_get_depth_of_field
#define Scene_viewer_set_depth_of_field cmzn_sceneviewer_set_depth_of_field
#define Scene_viewer_get_freespin_tumble_angle cmzn_sceneviewer_get_freespin_tumble_angle
#define Scene_viewer_set_freespin_tumble_angle cmzn_sceneviewer_set_freespin_tumble_angle
#define Scene_viewer_get_freespin_tumble_axis cmzn_sceneviewer_get_freespin_tumble_axis
#define Scene_viewer_start_freespin cmzn_sceneviewer_start_freespin
#define Scene_viewer_stop_animations cmzn_sceneviewer_stop_animations
#define Scene_viewer_get_viewing_volume cmzn_sceneviewer_get_viewing_volume
#define Scene_viewer_set_viewing_volume cmzn_sceneviewer_set_viewing_volume
#define Scene_viewer_set_background_texture_info cmzn_sceneviewer_set_background_texture_info
#define Scene_viewer_get_frame_count cmzn_sceneviewer_get_frame_count

/*
Global types
------------
*/

#include "opencmiss/zinc/sceneviewerinput.h"

#define MAX_CLIP_PLANES (6)
#define SCENE_VIEWER_PICK_SIZE 7.0

struct Scene_viewer_rendering_data;
/*******************************************************************************
LAST MODIFIED : 11 April 2003

DESCRIPTION :
Private rendering information.
==============================================================================*/

enum Scene_viewer_input_mode
/*******************************************************************************
LAST MODIFIED : 6 October 2000

DESCRIPTION :
==============================================================================*/
{
	SCENE_VIEWER_NO_INPUT_OR_DRAW,
	SCENE_VIEWER_UPDATE_ON_CLICK,
	SCENE_VIEWER_NO_INPUT,
	SCENE_VIEWER_SELECT,
	SCENE_VIEWER_TRANSFORM
};

enum Scene_viewer_buffering_mode
/*******************************************************************************
LAST MODIFIED : 19 September 2002

DESCRIPTION :
Be sure to implement any new modes in Scene_viewer_buffering_mode_string.
==============================================================================*/
{
	SCENE_VIEWER_PIXEL_BUFFER,
	SCENE_VIEWER_SINGLE_BUFFER,
	SCENE_VIEWER_DOUBLE_BUFFER
};

enum Scene_viewer_projection_mode
/*******************************************************************************
LAST MODIFIED : 16 September 2002

DESCRIPTION :
Specifies the sort of projection matrix used to render the 3D scene.
==============================================================================*/
{
	SCENE_VIEWER_PARALLEL,
	SCENE_VIEWER_PERSPECTIVE,
	SCENE_VIEWER_CUSTOM
};

enum Scene_viewer_drag_mode
{
	SV_DRAG_NOTHING,
	SV_DRAG_TUMBLE,
	SV_DRAG_TRANSLATE,
	SV_DRAG_ZOOM,
	SV_DRAG_FLY
};

struct cmzn_sceneviewermodule
/*******************************************************************************
LAST MODIFIED : 19 January 2007

DESCRIPTION:
The default data used to create cmzn_sceneviewers.
==============================================================================*/
{
	struct Graphics_buffer_package *graphics_buffer_package;
	struct Colour background_colour;
	//struct MANAGER(Interactive_tool) *interactive_tool_manager;
	cmzn_lightmodule *lightModule;
	struct cmzn_light *default_light;
	struct cmzn_light *default_ambient_light;
	cmzn_scenefiltermodule_id filterModule;
	//-- struct User_interface *user_interface;
	/* List of scene_viewers created with this package,
		generally all scene_viewers that are not in graphics windows */
	struct LIST(Scene_viewer) *scene_viewer_list;
	struct LIST(CMZN_CALLBACK_ITEM(cmzn_sceneviewermodule_callback))
		*destroy_callback_list;
	void *scenefilter_manager_callback_id;
	void *light_manager_callback_id;

private:
	int access_count;

	cmzn_sceneviewermodule(struct Colour *background_colourIn,
		cmzn_lightmodule *lightmoduleIn, struct cmzn_light *default_lightIn,
		struct cmzn_light *default_ambient_lightIn,
		cmzn_scenefiltermodule_id scenefiltermoduleIn);

	~cmzn_sceneviewermodule();

public:

	static cmzn_sceneviewermodule *create(struct Colour *background_colourIn,
		cmzn_lightmodule *lightmoduleIn, struct cmzn_light *default_lightIn,
		struct cmzn_light *default_ambient_lightIn,
		cmzn_scenefiltermodule_id scenefiltermoduleIn);

	cmzn_sceneviewermodule *access()
	{
		++(this->access_count);
		return this;
	}

	static int deaccess(cmzn_sceneviewermodule* &sceneviewermodule)
	{
		if (sceneviewermodule)
		{
			--(sceneviewermodule->access_count);
			if (sceneviewermodule->access_count <= 0)
				delete sceneviewermodule;
			sceneviewermodule = 0;
			return CMZN_OK;
		}
		return CMZN_ERROR_ARGUMENT;
	}


	double getDefaultBackgroundColourAlpha() const
	{
		return this->background_colour.alpha;
	}

	int setDefaultBackgroundColourAlpha(double alpha)
	{
		this->background_colour.alpha = alpha;
		return CMZN_OK;
	}

	int getDefaultBackgroundColourRGB(double *valuesOut3) const;

	int setDefaultBackgroundColourRGB(const double *valuesIn3);

	int getDefaultBackgroundColourRGBA(double *valuesOut4) const;

	int setDefaultBackgroundColourRGBA(const double *valuesIn4);

};

struct Scene_viewer_image_texture
{
	struct Texture *texture;
	struct MANAGER(Computed_field) *manager;
	cmzn_field_image_id field;
	void *callback_id;
	struct Scene_viewer *scene_viewer;
};

struct cmzn_sceneviewerinput
{
	int access_count;
	enum cmzn_sceneviewerinput_event_type type;
	int button_number;
	int key_code;
	int position_x;
	int position_y;
	/* flags indicating the state of the shift, control and alt keys - use
	 * logical OR with CMZN_SCENEVIEWERINPUT_MODIFIER_FLAG_SHIFT etc. */
	cmzn_sceneviewerinput_modifier_flags modifiers;
};

struct cmzn_sceneviewer
{
	int access_count;
	/* The buffer into which this scene viewer is rendering */
	struct Graphics_buffer *graphics_buffer;
	enum Scene_viewer_input_mode input_mode;
	/* following flag forces the scene_viewer temporarily into transform mode
		 when the control key is held down */
	int temporary_transform_mode;
	/* scene to be viewed */
	/* The projection mode. PARALLEL and PERSPECTIVE projections get their
		 modelview matrix using gluLookat, and their projection matrix from the
		 viewing volume. CUSTOM projection requires both matrices to be read-in */
	enum Scene_viewer_projection_mode projection_mode;
	/* Viewing transformation defined by eye pos, look-at point and up-vector */
	double eyex,eyey,eyez;
	double lookatx,lookaty,lookatz;
	double upx,upy,upz;
	/* Viewing volume for PARALLEL and PERSPECTIVE projections. */
	double left,right,bottom,top,near_plane,far_plane;
	/* Scale factors for controlling how rate of translate, tumble and zoom
		 transformations in relation to mouse movements. Setting a value to
		 zero turns off that transform capability. */
	double translate_rate,tumble_rate,zoom_rate;
	/* For CUSTOM projection only: 4X4 projection and modelview matrices.
		These are now stored internally in OpenGL format.
		ie. m0 m4 m8 m12 would be the first row, m1 m5 m9 m13 the second, etc. */
	double projection_matrix[16],modelview_matrix[16],
		window_projection_matrix[16];
	/* The projection matrix, whether set directly for CUSTOM projection or
		 calculated for PARALLEL and PERSPECTIVE projections using the viewing
		 volume, converts 3-D positions into Normalized Device Coordinates (NDCs) in
		 a cube from -1 to +1 in each direction.  In the z (depth) direction the
		 values from -1 (=near_plane plane) to +1 (=far plane) are already where we want
		 and need no further processing.  In general, however, the real x,y size and
		 origin in user coordinates are needed to display the image in an
		 undistorted manner. The following NDC_ variables are used for this purpose.
		 In a CUSTOM projection they must be read-in. PARALLEL and PERSPECTIVE
		 projections calculate them from the viewing volume.  Note that these values
		 must be given in user coordinates. */
	double NDC_left,NDC_top,NDC_width,NDC_height;
	/* The viewport mode specifies whether the NDCs, adjusted to the aspect
		 ratio from NDC_width/NDC_height are made as large as possible in the
		 physical viewport (RELATIVE_VIEWPORT), or whether an exact mapping from
		 user coordinates to pixels is used (ABSOLUTE_VIEWPORT), or whether the
		 aspect ratio from NDC_width/NDC_height is ignored and the NDCs are made
		 as large as possible(DISTORTING_RELATIVE_VIEWPORT).
	*/
	enum cmzn_sceneviewer_viewport_mode viewport_mode;
	/* Specifies the offset and scale of user coordinates in the physical
		 viewport, by supplying the user coordinate of the top,left position in
		 and the number of pixels plotted for a change of 1 in user units. Note
		 that these are in no way restricted to integer values.
		 ???RC.  Write how to handle y increasing down the screen? */
	double user_viewport_left,user_viewport_top,user_viewport_pixels_per_unit_x,
		user_viewport_pixels_per_unit_y;
	/* specifies the quality of transparency rendering */
	enum cmzn_sceneviewer_transparency_mode transparency_mode;
	/* number of layers used in layered transparency mode */
	int transparency_layers;
	/* When an ABSOLUTE_VIEWPORT is used the following values specify the
		 position and scale of the image relative to user coordinates. In the
		 RELATIVE_VIEWPORT and DISTORTING_RELATIVE_VIEWPORT modes, these values
		 are ignored and the image is
		 drawn behind the normalized device coordinate range.
		 ???RC.  Allow texture to be cropped as well? */
	double bk_texture_left,bk_texture_top,bk_texture_width,
		bk_texture_height,bk_texture_max_pixels_per_polygon;
	int bk_texture_undistort_on;
	/* non-ambient lights in this list are oriented relative to the viewer */
	struct LIST(cmzn_light) *list_of_lights;
	/* managers and callback IDs for automatic updates */
	/* For interpreting mouse events */
	cmzn_sceneviewer_interact_mode interact_mode;
	enum Scene_viewer_drag_mode drag_mode;
	int previous_pointer_x, previous_pointer_y;
	/* kept tumble axis and angle for spinning scene viewer */
	double tumble_axis[3], tumble_angle;
	int tumble_active;
	/* Keep track of debt owed to near and far plane when in fly mode */
	double near_plane_fly_debt, far_plane_fly_debt;
	/* background */
	struct Colour background_colour;
	enum Scene_viewer_buffering_mode buffering_mode;
	enum cmzn_sceneviewer_stereo_mode stereo_mode;
	int pixel_height,pixel_width,update_pixel_image;
	void *pixel_data;
	int antialias;
	bool perturb_lines;
	cmzn_sceneviewer_blending_mode blending_mode;
	double depth_of_field;  /* depth_of_field, 0 == infinite */
	double focal_depth;
	/* flag indicating that the viewer should swap buffers at the next
		 appropriate point */
	int swap_buffers;
	/* Flag that indicates the update includes a change of the projection matrices */
	int transform_flag;
	int awaken;
	/* Clip planes */
	char clip_planes_enable[MAX_CLIP_PLANES];
	double clip_planes[MAX_CLIP_PLANES * 4];
	/* The distance between the two stereo views in world space */
	double stereo_eye_spacing;
	/* Special persistent data for order independent transparency */
	struct cmzn_sceneviewer_transparency_order_independent_data
	   *order_independent_transparency_data;
	/* The connection to the systems user interface system */
	//-- struct User_interface *user_interface;
#if defined (WIN32_SYSTEM)
	/* Clear twice, if set then the glClear in the background will be called
		twice, which appears to work around a rendering bug on ATI windows driver 6.14.0010.6706 */
	int clear_twice_flag;
#endif /* defined (WIN32_SYSTEM) */
	/* Keeps a counter of the frame redraws */
	unsigned int frame_count;
	Scene_viewer_image_texture image_texture;
	cmzn_scenefilter_id filter;
	cmzn_scene_id scene;
	cmzn_sceneviewernotifier_list *notifier_list;
	cmzn_sceneviewermodule *module;
	int cache;
	int changes;
	// if true (default) then back surfaces are lit with reversed normals
	// if false then back surfaces are only lit by ambient lighting
	bool lightingTwoSided;
	// if true, angle of view from the eye is used to give better lighting, at more expense
	// if false (default) infinite lighting is assumed
	bool lightingLocalViewer;

	double getBackgroundColourAlpha() const
	{
		return this->background_colour.alpha;
	}

	int setBackgroundColourAlpha(double alpha);

	int getBackgroundColourRGB(double *valuesOut3) const;

	int setBackgroundColourRGB(const double *valuesIn3);

	int getBackgroundColourRGBA(double *valuesOut4) const;

	int setBackgroundColourRGBA(const double *valuesIn4);

	bool isLightingLocalViewer()
	{
		return this->lightingLocalViewer;
	}

	void setLightingLocalViewer(bool value);

	bool isLightingTwoSided()
	{
		return this->lightingTwoSided;
	}

	void setLightingTwoSided(bool value);

	/**
	 * @param  localToWorldTransformationMatrix  Optional.
	 * @return CMZN_OK on success, any other error on failure
	 */
	int getTransformationMatrix(
		enum cmzn_scenecoordinatesystem fromCoordinateSystem,
		enum cmzn_scenecoordinatesystem toCoordinateSystem,
		const gtMatrix *localToWorldTransformationMatrix,
		double *transformationMatrix16);

	void beginChange()
	{
		++(this->cache);
	}

	void endChange()
	{
		--(this->cache);
		if ((0 == this->cache) && (this->changes))
			this->notifyClients();
	}

	/** Notify registered clients of change in the scene viewer */
	void notifyClients();

	/** Mark changed repaint required. Notify clients unless caching changes. */
	void setChangedRepaint()
	{
		this->changes |= CMZN_SCENEVIEWEREVENT_CHANGE_FLAG_REPAINT_REQUIRED;
		if (0 == this->cache)
			this->notifyClients();
	}

	void setChangedTransform()
	{
		this->changes |= (CMZN_SCENEVIEWEREVENT_CHANGE_FLAG_TRANSFORM |
			CMZN_SCENEVIEWEREVENT_CHANGE_FLAG_REPAINT_REQUIRED);
		if (0 == this->cache)
			this->notifyClients();
	}

	void setChangedTransformOnly()
	{
		this->changes |= CMZN_SCENEVIEWEREVENT_CHANGE_FLAG_TRANSFORM;
		if (0 == this->cache)
			this->notifyClients();
	}

}; /* struct cmzn_sceneviewer */

DECLARE_CMZN_CALLBACK_TYPES(cmzn_sceneviewermodule_callback, \
	struct cmzn_sceneviewermodule *, void *, void);

DECLARE_LIST_TYPES(Scene_viewer);
PROTOTYPE_LIST_FUNCTIONS(Scene_viewer);

/*
Global functions
----------------
*/

int cmzn_sceneviewermodule_add_destroy_callback(struct cmzn_sceneviewermodule *sceneviewermodule,
	CMZN_CALLBACK_FUNCTION(cmzn_sceneviewermodule_callback) *function,void *user_data);
/*******************************************************************************
LAST MODIFIED : 24 January 2007

DESCRIPTION :
Adds a callback to the <sceneviewermodule> that is called back before the scene
viewer is destroyed.
==============================================================================*/

int cmzn_sceneviewermodule_remove_destroy_callback(struct cmzn_sceneviewermodule *sceneviewermodule,
	CMZN_CALLBACK_FUNCTION(cmzn_sceneviewermodule_callback) *function,void *user_data);
/*******************************************************************************
LAST MODIFIED : 24 January 2007

DESCRIPTION :
Removes the callback calling <function> with <user_data> from
<sceneviewermodule>.
==============================================================================*/

struct Graphics_buffer_package *cmzn_sceneviewermodule_get_graphics_buffer_package(
	struct cmzn_sceneviewermodule *sceneviewermodule);
/*******************************************************************************
LAST MODIFIED : 19 January 2007

DESCRIPTION :
==============================================================================*/

struct Scene_viewer *CREATE(Scene_viewer)(struct Graphics_buffer *graphics_buffer,
	struct Colour *background_colour,
	struct cmzn_light *default_light,
	struct cmzn_light *default_ambient_light,
	cmzn_scenefilter_id filter);
/*******************************************************************************
LAST MODIFIED : 19 September 2002

DESCRIPTION :
Creates a Scene_viewer in the widget <parent> to display <scene>.
Note: the parent must be an XmForm since form constraints will be applied.
If any of light_manager or scene_manager.
are supplied, the scene_viewer will automatically redraw in response to changes
of objects from these managers that are in use by the scene_viewer. Redraws are
performed in idle time so that multiple redraws are avoided.
==============================================================================*/

int DESTROY(Scene_viewer)(struct Scene_viewer **scene_viewer_address);
/*******************************************************************************
LAST MODIFIED : 18 November 1998

DESCRIPTION :
Closes the scene_viewer and disposes of the scene_viewer data structure.
==============================================================================*/

int Scene_viewer_awaken(struct Scene_viewer *scene_viewer);
/*******************************************************************************
LAST MODIFIED : 29 September 2000

DESCRIPTION :
Restores manager callbacks of previously inactive scene_viewer. Must call after
Scene_viewer_sleep to restore normal activity.
==============================================================================*/

int Scene_viewer_get_freespin_tumble_angle(struct Scene_viewer *scene_viewer,
	double *tumble_angle);
/*******************************************************************************
LAST MODIFIED : 9 October 2003

DESCRIPTION :
Gets the <scene_viewer> tumble angle.
==============================================================================*/

int Scene_viewer_set_freespin_tumble_angle(struct Scene_viewer *scene_viewer,
	double tumble_angle);
/*******************************************************************************
LAST MODIFIED : 17 February 2005

DESCRIPTION :
Sets the <scene_viewer> tumble angle.
==============================================================================*/

int Scene_viewer_get_freespin_tumble_axis(struct Scene_viewer *scene_viewer,
	double *tumble_axis);
/*******************************************************************************
LAST MODIFIED : 9 October 2003

DESCRIPTION :
Gets the <scene_viewer> tumble axis.  The <tumble_axis> is the vector
about which the scene is turning relative to its lookat point.
==============================================================================*/

int Scene_viewer_start_freespin(struct Scene_viewer *scene_viewer,
	double *tumble_axis, double tumble_angle);
/*******************************************************************************
LAST MODIFIED : 10 September 2003

DESCRIPTION :
Sets the <scene_viewer> spinning in idle time.  The <tumble_axis> is the vector
about which the scene is turning relative to its lookat point and the
<tumble_angle> controls how much it turns on each redraw.
==============================================================================*/

int Scene_viewer_stop_animations(struct Scene_viewer *scene_viewer);
/*******************************************************************************
LAST MODIFIED : 29 September 2000

DESCRIPTION :
Tells the <scene_viewer> to stop all automatic informations that it produces,
eg. automatic tumble.
==============================================================================*/

int Scene_viewer_sleep(struct Scene_viewer *scene_viewer);
/*******************************************************************************
LAST MODIFIED : 29 September 2000

DESCRIPTION :
Turns off any pending automatic tumbles or redraws in idle time, and removes
any manager callbacks to minimise impact of inactive scene_viewer on rest of
program. Must call Scene_viewer_awaken to restore manager callbacks.
Must call this in DESTROY function.
==============================================================================*/

int Scene_viewer_get_depth_of_field(struct Scene_viewer *scene_viewer,
	double *depth_of_field, double *focal_depth);
/*******************************************************************************
LAST MODIFIED : 5 December 2006

DESCRIPTION :
==============================================================================*/

int Scene_viewer_set_depth_of_field(struct Scene_viewer *scene_viewer,
	double depth_of_field, double focal_depth);
/*******************************************************************************
LAST MODIFIED : 5 December 2006

DESCRIPTION :
Set a simulated <depth_of_field> for the scene_viewer.
If <depth_of_field> is 0, then this is disabled, essentially an infinite depth.
Otherwise, <depth_of_field> is a normalised length in z space, so 1 is a
significant value, 0.1 is a small value causing significant distortion.
The <focal_depth> is depth in normalised device coordinates, -1 at near plane
and +1 at far plane.  At this <focal_depth> the image is in focus no matter
how small the <depth_of_field>.
==============================================================================*/

struct Texture *Scene_viewer_get_background_texture(
	struct Scene_viewer *scene_viewer);
/*******************************************************************************
LAST MODIFIED : 21 January 1998

DESCRIPTION :
Retrieves the Scene_viewer's background_texture. Note that NULL is the valid
return if there is no background texture.
==============================================================================*/

int Scene_viewer_get_background_texture_info(struct Scene_viewer *scene_viewer,
	double *bk_texture_left,double *bk_texture_top,
	double *bk_texture_width,double *bk_texture_height,
	int *bk_texture_undistort_on,double *bk_texture_max_pixels_per_polygon);
/*******************************************************************************
LAST MODIFIED : 28 September 1999

DESCRIPTION :
See Scene_viewer_set_background_texture_info for meaning of return values.
==============================================================================*/

int Scene_viewer_set_background_texture_info(struct Scene_viewer *scene_viewer,
	double bk_texture_left,double bk_texture_top,
	double bk_texture_width,double bk_texture_height,
	int bk_texture_undistort_on,double bk_texture_max_pixels_per_polygon);
/*******************************************************************************
LAST MODIFIED : 28 September 1999

DESCRIPTION :
If there is a background_texture in the scene_viewer, these values specify the
top,left corner, in user coordinates, where it will be displayed, while the
next two parameters specify the size it will have in these coordinates.
If the bk_texture_undistort_on flag is set, radial distortion parameters from the background texture are un-distorted when the
texture is displayed. It does this by drawing it as a collection of polygons;
the last parameter controls the size of polygons used to do this.
==============================================================================*/

enum Scene_viewer_buffering_mode Scene_viewer_get_buffering_mode(
	struct Scene_viewer *scene_viewer);
/*******************************************************************************
LAST MODIFIED : 15 October 1998

DESCRIPTION :
Returns the buffer mode - single_buffer/double_buffer - of the Scene_viewer.
==============================================================================*/

enum Scene_viewer_input_mode Scene_viewer_get_input_mode(
	struct Scene_viewer *scene_viewer);
/*******************************************************************************
LAST MODIFIED : 6 October 1998

DESCRIPTION :
Returns the input_mode of the Scene_viewer.
==============================================================================*/

int Scene_viewer_set_input_mode(struct Scene_viewer *scene_viewer,
	enum Scene_viewer_input_mode input_mode);
/*******************************************************************************
LAST MODIFIED : 19 December 1997

DESCRIPTION :
Sets the input_mode of the Scene_viewer.
==============================================================================*/

int cmzn_sceneviewer_has_light_in_list(struct Scene_viewer *scene_viewer,
	struct LIST(cmzn_light) *light_list);
/*******************************************************************************
LAST MODIFIED : 30 May 2001

DESCRIPTION :
Returns true if the list_of_lights in <Scene> intersects <light_list>.
==============================================================================*/

int Scene_viewer_add_clip_plane(struct Scene_viewer *scene_viewer,
	double A, double B, double C, double D);
/*******************************************************************************
LAST MODIFIED : 12 December 2000

DESCRIPTION :
Sets a clip plane that defines a plane in Modelview space, (Ax+By+Cz=D).
==============================================================================*/

int Scene_viewer_remove_clip_plane(struct Scene_viewer *scene_viewer,
	double A, double B, double C, double D);
/*******************************************************************************
LAST MODIFIED : 12 December 2000

DESCRIPTION :
Removes a clip plane that defines a plane in Modelview space, fails if the
exact plane isn't defined as a clip plane.
==============================================================================*/

int Scene_viewer_set_lookat_parameters(struct Scene_viewer *scene_viewer,
	double eyex,double eyey,double eyez,
	double lookatx,double lookaty,double lookatz,
	double upx,double upy,double upz);
/*******************************************************************************
LAST MODIFIED : 18 November 1997

DESCRIPTION :
Sets the view direction and orientation of the Scene_viewer.
==============================================================================*/

double Scene_viewer_get_stereo_eye_spacing(struct Scene_viewer *scene_viewer);
/*******************************************************************************
LAST MODIFIED : 13 August 2002

DESCRIPTION :
Returns the Scene_viewer stereo_eye_spacing.
==============================================================================*/

int Scene_viewer_set_stereo_eye_spacing(struct Scene_viewer *scene_viewer,
	double stereo_eye_spacing);
/*******************************************************************************
LAST MODIFIED : 13 August 2002

DESCRIPTION :
Sets the Scene_viewer stereo_eye_spacing.
==============================================================================*/

int Scene_viewer_get_modelview_matrix(struct Scene_viewer *scene_viewer,
	double modelview_matrix[16]);
/*******************************************************************************
LAST MODIFIED : 21 January 1998

DESCRIPTION :
Reads the modelview matrix used if the SCENE_VIEWER_CUSTOM projection is in
effect. The format of the matrix is as in Scene_viewer_set_modelview_matrix.
==============================================================================*/

int Scene_viewer_set_modelview_matrix(struct Scene_viewer *scene_viewer,
	double modelview_matrix[16]);
/*******************************************************************************
LAST MODIFIED : 21 January 1998

DESCRIPTION :
Sets the modelview matrix used if the SCENE_VIEWER_CUSTOM projection is in
effect. The 4X4 matrix is stored in an array of 16 double values, with values
consecutive across rows, eg:
[x'] = |  m0  m1  m2  m3 |.[x]
[y']   |  m4  m5  m6  m7 | [y]
[z']   |  m8  m9 m10 m11 | [z]
[w']   | m12 m13 m14 m15 | [w]
==============================================================================*/

int Scene_viewer_get_NDC_info(struct Scene_viewer *scene_viewer,
	double *NDC_left,double *NDC_top,double *NDC_width,double *NDC_height);
/*******************************************************************************
LAST MODIFIED : 21 January 1998

DESCRIPTION :
Gets the NDC_info from the scene_viewer - see Scene_viewer_set_NDC_info.
==============================================================================*/

int Scene_viewer_set_NDC_info(struct Scene_viewer *scene_viewer,
	double NDC_left,double NDC_top,double NDC_width,double NDC_height);
/*******************************************************************************
LAST MODIFIED : 21 January 1998

DESCRIPTION :
The projection matrix converts the viewing volume into Normalised Device
Coordinates - NDCs - which range from -1.0 to +1.0 in each axis. However, the
shape of this area in the x,y plane of the screen will not be square in the
general case. The NDC_width, NDC_height, NDC_top and NDC_left values describe
the physical dimensions of the NDC cube, used to draw the image on the screen
without distortion. In RELATIVE viewport_mode, only the ratio of NDC_width to
NDC_height is important. In ABSOLUTE viewport_mode, the top and left values
are used to position the intended viewing volume in user coordinates.
==============================================================================*/

/**
 * Gets matrix transforming coordinate system to
 * CMZN_SCENECOORDINATESYSTEM_NORMALISED_WINDOW_FILL
 * Note this is a right-handed coordinate system with each coordinate on [-1,+1]
 * and farthest z = -1, nearest at z = +1. Compare with OpenGL normalised device
 * coordinates which reverse z so are left-handed.
 * @param local_transformation_matrix  Optional local to world transformation.
 */
int Scene_viewer_get_transformation_to_window(struct Scene_viewer *scene_viewer,
	enum cmzn_scenecoordinatesystem coordinate_system,
	const gtMatrix *local_transformation_matrix, double *projection);

int Scene_viewer_get_projection_mode(struct Scene_viewer *scene_viewer,
	enum Scene_viewer_projection_mode *projection_mode);
/*******************************************************************************
LAST MODIFIED : 17 September 2002

DESCRIPTION :
Returns the projection mode - parallel/perspective - of the Scene_viewer.
==============================================================================*/

int Scene_viewer_set_projection_mode(struct Scene_viewer *scene_viewer,
	enum Scene_viewer_projection_mode projection_mode);
/*******************************************************************************
LAST MODIFIED : 21 November 1997

DESCRIPTION :
Sets the projection mode - parallel/perspective - of the Scene_viewer.
==============================================================================*/

int Scene_viewer_get_projection_matrix(struct Scene_viewer *scene_viewer,
	double projection_matrix[16]);
/*******************************************************************************
LAST MODIFIED : 21 January 1998

DESCRIPTION :
Reads the projection matrix used if the SCENE_VIEWER_CUSTOM projection is in
effect. The format of the matrix is as in Scene_viewer_set_projection_matrix.
==============================================================================*/

int Scene_viewer_set_projection_matrix(struct Scene_viewer *scene_viewer,
	double projection_matrix[16]);
/*******************************************************************************
LAST MODIFIED : 21 January 1998

DESCRIPTION :
Sets the projection matrix used if the SCENE_VIEWER_CUSTOM projection is in
effect. The 4X4 matrix is stored in an array of 16 double values, with values
consecutive across rows, eg:
[x'] = |  m0  m1  m2  m3 |.[x]
[y']   |  m4  m5  m6  m7 | [y]
[z']   |  m8  m9 m10 m11 | [z]
[w']   | m12 m13 m14 m15 | [w]
==============================================================================*/

int Scene_viewer_get_horizontal_view_angle(struct Scene_viewer *scene_viewer,
	double *horizontal_view_angle);
/*******************************************************************************
LAST MODIFIED : 6 April 2001

DESCRIPTION :
Gets the horizontal view angle, in radians, of the <scene_viewer>.
View angle is measured across the normalized device coordinates - NDCs.
For PARALLEL and PERSPECTIVE projection modes only.
==============================================================================*/

int Scene_viewer_get_vertical_view_angle(struct Scene_viewer *scene_viewer,
	double *vertical_view_angle);
/*******************************************************************************
LAST MODIFIED : 6 April 2001

DESCRIPTION :
Gets the vertical view angle, in radians, of the <scene_viewer>.
View angle is measured across the normalized device coordinates - NDCs.
For PARALLEL and PERSPECTIVE projection modes only.
==============================================================================*/

/**
 * Adjusts the viewing parameters of scene_viewer so that it is looking at
 * the centre of a sphere of the given radius with the given view_angle.
 * The function also adjusts the far clipping plane to be clip_distance behind
 * the interest point, and the near plane to by the minimum of clip_distance or
 * eye_distance*0.95 in front of it.
 */
int Scene_viewer_set_view_simple(struct Scene_viewer *scene_viewer,
	double centre_x,double centre_y,double centre_z,double radius,
	double view_angle,double clip_distance);

int Scene_viewer_get_viewing_volume(struct Scene_viewer *scene_viewer,
	double *left,double *right,double *bottom,double *top,double *near,
	double *far);
/*******************************************************************************
LAST MODIFIED : 18 November 1997

DESCRIPTION :
Gets the viewing volume of the Scene_viewer.
==============================================================================*/

/***************************************************************************//**
 * This function handle cases when scene viewer or/and print out images is/are
 * is non square. This calculation will help cmgui to determine
 * the required view volume and NDC width and height for correct image
 * output.
 *
 * @param scene_viewer  The target scene viewer where the values will be
 *   calculated for.
 * @param target_width  width of the print out images.
 * @param target_height  height of the print out images.
 * @param source_width  width of the scene viewer.
 * @param source_height  height of the scene viewer.
 * @param left  Pointer to a double, which value will be written with the
 *   evaluated location of the left of the view volume.
 * @param right  Pointer to a double, which value will be written with the
 *   evaluated location of the right of the view volume.
 * @param bottom  Pointer to a double, which value will be written with the
 *   evaluated location of the bottom of the view volume.
 * @param top  Pointer to a double, which value will be written with the
 *   evaluated location of the top of the view volume.
 * @param scaled_NDC_width  Pointer to a double, which value will be written with
 *   the rescaled NDC width
  * @param scaled_NDC_height Pointer to a double, which value will be written with
 *   the rescaled NDC height
 * @return  1 if successfully get the view volume and NDC info, otherwise 0.
 */
int Scene_viewer_get_viewing_volume_and_NDC_info_for_specified_size(struct Scene_viewer *scene_viewer,
	int target_width, int target_height, int source_width, int source_height, double *left,
	double *right, double *bottom, double *top, double *scaled_NDC_width, double *scaled_NDC_height);

int Scene_viewer_set_viewing_volume(struct Scene_viewer *scene_viewer,
	double left,double right,double bottom,double top,double near,double far);
/*******************************************************************************
LAST MODIFIED : 15 December 1997

DESCRIPTION :
Sets the viewing volume of the Scene_viewer. Unless the viewing volume is the
same shape as the window, taking into account the aspect, the Scene_viewer will
enlarge it to maintain the desired aspect ratio. Hence, the values specified
represent the minimum viewing volume. The left, right, bottom and top values
are at the lookat point, not on the near plane as OpenGL assumes. This gives a
similar sized viewing_volume for both parallel and perspective projections.
The viewing volume can be made unsymmetric to create special effects such as
rendering a higher resolution image in parts.
==============================================================================*/

int Scene_viewer_get_viewport_info(struct Scene_viewer *scene_viewer,
	double *viewport_left,double *viewport_top,double *viewport_pixels_per_unit_x,
	double *viewport_pixels_per_unit_y);
/*******************************************************************************
LAST MODIFIED : 21 January 1998

DESCRIPTION :
See Scene_viewer_set_viewport_info for explanation of the values returned.
==============================================================================*/

int Scene_viewer_set_viewport_info(struct Scene_viewer *scene_viewer,
	double viewport_left,double viewport_top,double viewport_pixels_per_unit_x,
	double viewport_pixels_per_unit_y);
/*******************************************************************************
LAST MODIFIED : 21 January 1998

DESCRIPTION :
The 4 parameters of this routine define the user coordinate system in the
physical viewport. <viewport_left> and <viewport_top> define the exact value
of the user's x and y coordinate at these edges of the viewport.
Note that these are real numbers, and that they refer to the location at the
top-left of the top-left pixel.
???RC Check the above!
The remaining two values specify the scale in pixels per 1 unit of user
coordinates in the x and y direction.
The parameters to the Scene_viewer_set_background_texture_info and
Scene_viewer_set_NDC_info routines are to be given in the user coordinate
system established here. Furthermore, by adjusting the viewport_left and
viewport_top, the image can be translated, while changing the number of
pixels per unit enables zooming to be achieved.
???RC Later: send user coordinates with mouse events.
???RC How to handle y axis pointing down?
==============================================================================*/

struct Graphics_buffer *cmzn_sceneviewer_get_graphics_buffer(cmzn_sceneviewer_id scene_viewer);

int Scene_viewer_get_viewport_size(struct Scene_viewer *scene_viewer,
	int *width, int *height);
/*******************************************************************************
LAST MODIFIED : 2 July 2002

DESCRIPTION :
Returns the width and height of the Scene_viewers drawing area.
==============================================================================*/

int Scene_viewer_get_window_projection_matrix(struct Scene_viewer *scene_viewer,
	double window_projection_matrix[16]);
/*******************************************************************************
LAST MODIFIED : 24 March 1998

DESCRIPTION :
Returns the actual projection matrix applied to fill the window.
==============================================================================*/

/**
 * Translate sceneviewer eye and lookat by supplied offset vector.
 * @param scene_viewer  The scene viewer to modify.
 * @param offset  Offset vector to apply.
 * @return  CMZN_OK on success, otherwise CMZN_ERROR_ARGUMENT.
 */
int Scene_viewer_translate(struct Scene_viewer *scene_viewer, const double offset[3]);

/**
 * Rotates the eye by angle radians about axis vector bound to the scene viewer
 * lookat point. Up vector is also reoriented to remain normal to the
 * eye-to-lookat direction. Rotation is in a right-handed sense about axis.
 * @param scene_viewer  The scene viewer to modify.
 * @param axis  Vector giving axis of rotation. Does not need to be unit vector
 * as copied and normalised before use in function.
 * @param angle  Rotation angle in radians.
 * @return  CMZN_OK on success, otherwise CMZN_ERROR_ARGUMENT.
 */
int Scene_viewer_rotate_about_lookat_point(struct Scene_viewer *scene_viewer,
	const double axis[3], double angle);

int for_each_cmzn_light_in_Scene_viewer(struct Scene_viewer *scene_viewer,
	LIST_ITERATOR_FUNCTION(cmzn_light) *iterator_function,void *user_data);
/*******************************************************************************
LAST MODIFIED : 18 December 1997

DESCRIPTION :
Allows clients of the <scene_viewer> to perform functions with the lights in it.
The most common task will to list the lights in the scene with show_cmzn_light.
==============================================================================*/

int Scene_viewer_render_scene_in_viewport(struct Scene_viewer *scene_viewer,
	int left, int bottom, int right, int top);
/*******************************************************************************
LAST MODIFIED : 23 July 1999

DESCRIPTION :
Called to redraw the Scene_viewer scene after changes in the display lists or
viewing transformations.  Uses the specified viewport to draw into (unless
all the dimensions are zero).
==============================================================================*/

int Scene_viewer_render_scene_in_viewport_with_overrides(
	struct Scene_viewer *scene_viewer, int left, int bottom, int right, int top,
	int antialias, int transparency_layers, int drawing_offscreen);
/*******************************************************************************
LAST MODIFIED : 11 December 2002

DESCRIPTION :
Called to redraw the Scene_viewer scene after changes in the display lists or
viewing transformations.  Uses the specified viewport to draw into (unless
all the dimensions are zero).  If non_zero then the supplied <antialias> and
<transparency_layers> are used for just this render.
==============================================================================*/

struct Cmgui_image *Scene_viewer_get_image(struct Scene_viewer *scene_viewer,
	int force_onscreen, int preferred_width, int preferred_height,
	int preferred_antialias, int preferred_transparency_layers,
	enum Texture_storage_type storage);
/*******************************************************************************
LAST MODIFIED : 18 September 2002

DESCRIPTION :
Creates and returns a Cmgui_image from the image in <scene_viewer>, usually for
writing. The image has a single depth plane and is in RGBA format.
Up to the calling function to DESTROY the returned Cmgui_image.
If <preferred_width>, <preferred_height>, <preferred_antialias> or
<preferred_transparency_layers> are non zero then they attempt to override the
default values for just this call.
If <force_onscreen> is set then the pixels are grabbed directly from the window
display and the <preferred_width> and <preferred_height> are ignored.
Currently limited to 1 byte per component -- may want to improve for HPC.
==============================================================================*/

int Scene_viewer_set_update_pixel_image(struct Scene_viewer *scene_viewer);
/*******************************************************************************
LAST MODIFIED : 18 May 1998

DESCRIPTION :
Sets a flag so that the redraw will necessarily fully render the scene in
pixel buffer mode
==============================================================================*/

int Scene_viewer_set_pixel_image(struct Scene_viewer *scene_viewer,
	int width,int height,void *data);
/*******************************************************************************
LAST MODIFIED : 18 May 1998

DESCRIPTION :
Sets the RGB data in a scene viewer when buffer type
is SCENE_VIEWER_PIXEL_BUFFER.  The data is copied into the internal buffer.
It is expected to be byte sized values for each of Red Green and Blue only.
==============================================================================*/

int Scene_viewer_get_pixel_image(struct Scene_viewer *scene_viewer,
	int *width,int *height,void **data);
/*******************************************************************************
LAST MODIFIED : 18 May 1998

DESCRIPTION :
Returns RGB data grabbed from the scene viewer when buffer type
is SCENE_VIEWER_PIXEL_BUFFER.  The data is handed directly so it should
be used immediately and not DEALLOCATED.  It is expected to be byte sized
values for each of Red Green and Blue only.
==============================================================================*/

int Scene_viewer_viewport_zoom(struct Scene_viewer *scene_viewer,
	double zoom_ratio);
/*******************************************************************************
LAST MODIFIED : 16 February 1998

DESCRIPTION :
Scales of the absolute image while keeping the same centre point.
==============================================================================*/

int Scene_viewer_default_input_callback(struct Scene_viewer *scene_viewer,
	struct Graphics_buffer_input *input, void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 11 September 2007

DESCRIPTION :
The callback for mouse or keyboard input in the Scene_viewer window. The
resulting behaviour depends on the <scene_viewer> input_mode. In Transform mode
mouse clicks and drags are converted to transformation; in Select mode OpenGL
picking is performed with picked objects and mouse click and drag information
returned to the scene.
==============================================================================*/

//-- int Scene_viewer_add_input_callback(struct Scene_viewer *scene_viewer,
//-- 	CMZN_CALLBACK_FUNCTION(Scene_viewer_input_callback) *function,
//-- 	void *user_data, int add_first);
/*******************************************************************************
LAST MODIFIED : 11 September 2007

DESCRIPTION :
Adds callback that will be activated each time input is received by the
scene_viewer.
If <add_first> is true (non zero) then this callback will be added to the
front of the list.
When a callback event is generated the list is processed as long as each
callback function returns true, so to stop processing and not call any more
of the callbacks registered after your handler then return false.
==============================================================================*/

//-- int Scene_viewer_remove_input_callback(struct Scene_viewer *scene_viewer,
//-- 	CMZN_CALLBACK_FUNCTION(Scene_viewer_input_callback) *function,
//-- 	void *user_data);
/*******************************************************************************
LAST MODIFIED : 2 July 2002

DESCRIPTION :
Removes the callback calling <function> with <user_data> from
<scene_viewer>.
==============================================================================*/

PROTOTYPE_ENUMERATOR_FUNCTIONS(cmzn_sceneviewer_blending_mode);

const char *Scene_viewer_buffering_mode_string(
	enum Scene_viewer_buffering_mode buffering_mode);
/*******************************************************************************
LAST MODIFIED : 14 October 1998

DESCRIPTION :
Returns a string label for the <buffering_mode>.
NOTE: Calling function must not deallocate returned string.
==============================================================================*/

const char *Scene_viewer_input_mode_string(
	enum Scene_viewer_input_mode input_mode);
/*******************************************************************************
LAST MODIFIED : 14 October 1998

DESCRIPTION :
Returns a string label for the <input_mode>.
NOTE: Calling function must not deallocate returned string.
==============================================================================*/

const char *Scene_viewer_projection_mode_string(
	enum Scene_viewer_projection_mode projection_mode);
/*******************************************************************************
LAST MODIFIED : 14 October 1998

DESCRIPTION :
Returns a string label for the <projection_mode>.
NOTE: Calling function must not deallocate returned string.
==============================================================================*/

const char *cmzn_sceneviewer_transparency_mode_string(
	enum cmzn_sceneviewer_transparency_mode transparency_mode);
/*******************************************************************************
LAST MODIFIED : 23 November 1998

DESCRIPTION :
Returns a string label for the <transparency_mode>.
NOTE: Calling function must not deallocate returned string.
==============================================================================*/

/**
 * Returns a string label for the <viewport_mode>.
 * NOTE: Calling function must not deallocate returned string.
 */
const char *cmzn_sceneviewer_viewport_mode_string(
	enum cmzn_sceneviewer_viewport_mode viewport_mode);

int Scene_viewer_call_next_renderer(
	struct Scene_viewer_rendering_data *rendering_data);
/*******************************************************************************
LAST MODIFIED : 11 April 2003

DESCRIPTION :
Used by rendering functions to call the rest of the rendering callstack.
==============================================================================*/

struct Graphics_buffer *Scene_viewer_get_graphics_buffer(
	struct Scene_viewer *scene_viewer);
/*******************************************************************************
LAST MODIFIED : 12 May 2004

DESCRIPTION :
Gets the <graphics_buffer> used for 3D graphics in the scene_viewer.
==============================================================================*/

int Scene_viewer_get_frame_pixels(struct Scene_viewer *scene_viewer,
	enum Texture_storage_type storage, int *width, int *height,
	int preferred_antialias, int preferred_transparency_layers,
	unsigned char **frame_data, int force_onscreen);
/*******************************************************************************
LAST MODIFIED : 18 September 2002

DESCRIPTION :
Returns the contents of the scene viewer as pixels.  <width> and <height>
will be respected if the window is drawn offscreen and they are non zero,
otherwise they are set in accordance with current size of the scene viewer.
If <preferred_antialias> or <preferred_transparency_layers> are non zero then they
attempt to override the default values for just this call.
If <force_onscreen> is non zero then the pixels will always be grabbed from the
scene viewer on screen.
==============================================================================*/

unsigned int Scene_viewer_get_frame_count(struct Scene_viewer *scene_viewer);
/*******************************************************************************
LAST MODIFIED : 24 April 2008

DESCRIPTION :
Returns a count of the number of scene viewer redraws.
==============================================================================*/

cmzn_field_image_id Scene_viewer_get_background_image_field(
	struct Scene_viewer *scene_viewer);

int Scene_viewer_set_background_image_field(
	struct Scene_viewer *scene_viewer, cmzn_field_image_id image_field);

Render_graphics_opengl *Scene_viewer_rendering_data_get_renderer(
	Scene_viewer_rendering_data *rendering_data);

int Scene_viewer_input_transform(struct Scene_viewer *scene_viewer,
	struct Graphics_buffer_input *input);

int Scene_viewer_scene_change(cmzn_sceneviewer_id scene_viewer);

struct Scene_viewer *create_Scene_viewer_from_module(
	struct Graphics_buffer *graphics_buffer,
	struct cmzn_sceneviewermodule *sceneviewermodule);

#endif /* !defined (SCENE_VIEWER_H) */
