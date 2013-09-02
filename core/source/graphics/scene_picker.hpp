/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is cmgui.
 *
 * The Initial Developer of the Original Code is
 * Auckland Uniservices Ltd, Auckland, New Zealand.
 * Portions created by the Initial Developer are Copyright (C) 2013
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#if !defined (SCENE_PICKER_HPP)
#define SCENE_PICKER_HPP

#include <map>
#include "zinc/scenepicker.h"
#include "zinc/types/graphicid.h"
#include "zinc/types/graphicsfilterid.h"
#include "zinc/types/fieldgroupid.h"
#include "zinc/types/elementid.h"
#include "zinc/types/nodeid.h"
#include "zinc/types/sceneid.h"
#include "zinc/types/scenecoordinatesystem.h"
#include "zinc/types/scenepickerid.h"
#include "zinc/types/sceneviewerid.h"


typedef std::multimap<cmzn_region *, cmzn_element_id> Region_element_map;
typedef std::multimap<cmzn_region *, cmzn_node_id> Region_node_map;

enum cmzn_scene_picker_object_type
{
	CMZN_SCENE_PICKER_OBJECT_ANY = 0,
	CMZN_SCENE_PICKER_OBJECT_NODE = 1,
	CMZN_SCENE_PICKER_OBJECT_ELEMENT = 2,
	CMZN_SCENE_PICKER_OBJECT_DATA = 3
};

struct cmzn_scene_picker
{
private:
	struct Interaction_volume *interaction_volume;
	cmzn_scene_id top_scene;
	cmzn_scene_viewer_id scene_viewer;
	int centre_x, centre_y, size_x, size_y;
	enum cmzn_scene_coordinate_system coordinate_system;
	cmzn_graphics_filter_id filter;
	GLuint *select_buffer;
	int select_buffer_size;
	int number_of_hits;
	cmzn_graphics_filter_module_id filter_module;

	void updateViewerRectangle();

	int pickObjects();

	Region_node_map getPickedRegionSortedNodes(enum cmzn_scene_picker_object_type type);

	Region_element_map getPickedRegionSortedElements();

	void reset();

	/*provide a select buffer pointer and return the scene and graphic */
	int getSceneAndGraphic(GLuint *select_buffer_ptr,
		cmzn_scene_id *scene, cmzn_graphic_id *graphic);

public:

	int access_count;

	cmzn_scene_picker(cmzn_graphics_filter_module_id filter_module_in);

	~cmzn_scene_picker();

	cmzn_graphics_filter_id getGraphicsFilter();

	int setGraphicsFilter(cmzn_graphics_filter_id filter_in);

	cmzn_scene_id getScene();

	int setScene(cmzn_scene_id scene_in);

	int setSceneViewerRectangle(cmzn_scene_viewer_id scene_viewer_in,
		enum cmzn_scene_coordinate_system coordinate_system_in, double x1,
		double y1, double x2, double y2);

	int setInteractionVolume(struct Interaction_volume *interaction_volume_in);

	cmzn_element_id getNearestElement();

	cmzn_node_id getNearestNode(enum cmzn_scene_picker_object_type type);

	cmzn_graphic_id getNearestGraphic(enum cmzn_scene_picker_object_type type);

	inline cmzn_scene_picker *access()
	{
		++access_count;
		return this;
	}

	int addPickedElementsToGroup(cmzn_field_group_id group);

	int addPickedNodesToGroup(cmzn_field_group_id group,
		enum cmzn_scene_picker_object_type type);

};

PROTOTYPE_OBJECT_FUNCTIONS(cmzn_scene_picker);

cmzn_scene_picker_id cmzn_scene_picker_create(cmzn_graphics_filter_module_id filter_module);

int cmzn_scene_picker_set_interaction_volume(cmzn_scene_picker_id scene_picker,
	struct Interaction_volume *interaction_volume);

int cmzn_scene_picker_add_picked_data_to_group(cmzn_scene_picker_id scene_picker,
	cmzn_field_group_id group);

cmzn_graphic_id cmzn_scene_picker_get_nearest_data_graphic(cmzn_scene_picker_id scene_picker);

cmzn_node_id cmzn_scene_picker_get_nearest_data(cmzn_scene_picker_id scene_picker);

#endif /* (SCENE_PICKER_HPP) */
