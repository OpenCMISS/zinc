/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "graphics/scene.hpp"
#include "graphics/selection.hpp"
#include "general/message.h"

int cmzn_selectionevent::deaccess(cmzn_selectionevent* &event)
{
	if (event)
	{
		--(event->access_count);
		if (event->access_count <= 0)
			delete event;
		event = 0;
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_selectionnotifier::cmzn_selectionnotifier(cmzn_scene *scene) :
	scene(scene),
	function(0),
	user_data(0),
	access_count(1)
{
	scene->addSelectionnotifier(this);
}

cmzn_selectionnotifier::~cmzn_selectionnotifier()
{
}

int cmzn_selectionnotifier::deaccess(cmzn_selectionnotifier* &notifier)
{
	if (notifier)
	{
		--(notifier->access_count);
		if (notifier->access_count <= 0)
			delete notifier;
		else if ((1 == notifier->access_count) && notifier->scene)
			notifier->scene->removeSelectionnotifier(notifier);
		notifier = 0;
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_selectionnotifier::setCallback(cmzn_selectionnotifier_callback_function function_in,
	void *user_data_in)
{
	if (!function_in)
		return CMZN_ERROR_ARGUMENT;
	this->function = function_in;
	this->user_data = user_data_in;
	return CMZN_OK;
}

void cmzn_selectionnotifier::clearCallback()
{
	this->function = 0;
	this->user_data = 0;
}

void cmzn_selectionnotifier::sceneDestroyed()
{
	this->scene = 0;
	this->clearCallback();
}

void *cmzn_selectionnotifier_get_callback_user_data(
	cmzn_selectionnotifier_id selectionnotifier)
{
	if (selectionnotifier)
	{
		return selectionnotifier->getUserData();
	}

	return 0;
}

int cmzn_selectionnotifier_set_callback(cmzn_selectionnotifier_id selectionnotifier,
	cmzn_selectionnotifier_callback_function function_in, void *user_data_in)
{
	if (selectionnotifier && function_in)
		return selectionnotifier->setCallback(function_in, user_data_in);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_selectionnotifier_clear_callback(cmzn_selectionnotifier_id selectionnotifier)
{
	if (selectionnotifier)
	{
		selectionnotifier->clearCallback();
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_selectionnotifier_id cmzn_selectionnotifier_access(
	cmzn_selectionnotifier_id selectionnotifier)
{
	if (selectionnotifier)
		return selectionnotifier->access();
	return 0;
}

int cmzn_selectionnotifier_destroy(cmzn_selectionnotifier_id *selectionnotifier_address)
{
	return cmzn_selectionnotifier::deaccess(*selectionnotifier_address);
}

cmzn_selectionevent_id cmzn_selectionevent_access(
	cmzn_selectionevent_id selectionevent)
{
	if (selectionevent)
		return selectionevent->access();
	return 0;
}

int cmzn_selectionevent_destroy(cmzn_selectionevent_id *selectionevent_address)
{
	return cmzn_selectionevent::deaccess(*selectionevent_address);
}

cmzn_selectionevent_change_flags cmzn_selectionevent_get_change_flags(cmzn_selectionevent_id selectionevent)
{
	return selectionevent->changeFlags;
}
