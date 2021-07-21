/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (SELECTION_HPP)
#define SELECTION_HPP

#include "opencmiss/zinc/scene.h"
#include "opencmiss/zinc/selection.h"

/**
 * @param group_change_type  Logical OR of cmzn_field_group_change_type enums.
 * @return  Equivalent logical OR of cmzn_selectionevent_change_type enums.
 */
inline int cmzn_field_group_change_type_to_selectionevent_change_type(int group_change_type)
{
	// currently the same values used
	return group_change_type;
}

struct cmzn_selectionevent
{
	cmzn_selectionevent_change_flags changeFlags;
	int access_count;

	cmzn_selectionevent() :
		changeFlags(CMZN_SELECTIONEVENT_CHANGE_FLAG_NONE),
		access_count(1)
	{
	}

	~cmzn_selectionevent()
	{
	}

	cmzn_selectionevent *access()
	{
		++(this->access_count);
		return this;
	}

	static int deaccess(cmzn_selectionevent* &event);

};

struct cmzn_selectionnotifier
{
private:
	cmzn_scene_id scene; // owning scene: not accessed
	cmzn_selectionnotifier_callback_function function;
	void *user_data;
	int access_count;

	cmzn_selectionnotifier(cmzn_scene *scene);

	~cmzn_selectionnotifier();

public:

	/** private: external code must use cmzn_scene_create_selectionnotifier */
	static cmzn_selectionnotifier *create(cmzn_scene *scene)
	{
		return new cmzn_selectionnotifier(scene);
	}

	cmzn_selectionnotifier *access()
	{
		++(this->access_count);
		return this;
	}

	static int deaccess(cmzn_selectionnotifier* &notifier);

	void *getUserData()
	{
		return user_data;
	}

	int setCallback(cmzn_selectionnotifier_callback_function function_in,
		void *user_data_in);

	void clearCallback();

	void sceneDestroyed();

	void notify(cmzn_selectionevent *event)
	{
		if (this->function && event)
			(this->function)(event, this->user_data);
	}

};

#endif /* (SELECTION_HPP) */
