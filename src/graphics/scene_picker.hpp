/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (SCENE_PICKER_HPP)
#define SCENE_PICKER_HPP

#include <map>
#include "opencmiss/zinc/scenepicker.h"
#include "opencmiss/zinc/types/graphicsid.h"
#include "opencmiss/zinc/types/scenefilterid.h"
#include "opencmiss/zinc/types/fieldgroupid.h"
#include "opencmiss/zinc/types/elementid.h"
#include "opencmiss/zinc/types/nodeid.h"
#include "opencmiss/zinc/types/sceneid.h"
#include "opencmiss/zinc/types/scenecoordinatesystem.h"
#include "opencmiss/zinc/types/scenepickerid.h"
#include "opencmiss/zinc/types/sceneviewerid.h"

enum cmzn_scenepicker_object_type
{
	CMZN_SCENEPICKER_OBJECT_ANY = 0,
	CMZN_SCENEPICKER_OBJECT_NODE = 1,
	CMZN_SCENEPICKER_OBJECT_ELEMENT = 2
};

struct cmzn_scenepicker
{
private:
	struct Interaction_volume *interaction_volume;
	cmzn_scene_id top_scene;
	cmzn_sceneviewer_id scene_viewer;
	int centre_x, centre_y, size_x, size_y;
	enum cmzn_scenecoordinatesystem coordinate_system;
	cmzn_scenefilter_id filter;
	GLuint *select_buffer;
	int select_buffer_size;
	int number_of_hits;
	cmzn_scenefiltermodule_id filter_module;

	void updateViewerRectangle();

	int pickObjects();

	void reset();

	/*provide a select buffer pointer and return the scene and graphics */
	int getSceneAndGraphics(GLuint *select_buffer_ptr,
		cmzn_scene_id *scene, cmzn_graphics_id *graphics);

public:

	int access_count;

	cmzn_scenepicker(cmzn_scenefiltermodule_id filter_module_in);

	~cmzn_scenepicker();

	cmzn_scenefilter_id getScenefilter();

	int setScenefilter(cmzn_scenefilter_id filter_in);

	cmzn_scene_id getScene();

	int setScene(cmzn_scene_id scene_in);

	int setSceneviewerRectangle(cmzn_sceneviewer_id scene_viewer_in,
		enum cmzn_scenecoordinatesystem coordinate_system_in, double x1,
		double y1, double x2, double y2);

	int setInteractionVolume(struct Interaction_volume *interaction_volume_in);

	cmzn_element_id getNearestElement();

	cmzn_node_id getNearestNode();

	cmzn_graphics_id getNearestGraphics(enum cmzn_scenepicker_object_type type);

	inline cmzn_scenepicker *access()
	{
		++access_count;
		return this;
	}

	int addPickedElementsToFieldGroup(cmzn_field_group_id group);

	int addPickedNodesToFieldGroup(cmzn_field_group_id group);

	int getPickingVolumeCentre(double *coordinateValuesOut3);

};

PROTOTYPE_OBJECT_FUNCTIONS(cmzn_scenepicker);

cmzn_scenepicker_id cmzn_scenepicker_create(cmzn_scenefiltermodule_id filter_module);

int cmzn_scenepicker_set_interaction_volume(cmzn_scenepicker_id scenepicker,
	struct Interaction_volume *interaction_volume);

#endif /* (SCENE_PICKER_HPP) */
