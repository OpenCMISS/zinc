/*******************************************************************************
FILE : render_vrml.c

LAST MODIFIED : 5 January 1998

DESCRIPTION :
Renders gtObjects to VRML file
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (render_vrml_H)
#define render_vrml_H

#include "opencmiss/zinc/types/sceneid.h"
#include "opencmiss/zinc/types/scenefilterid.h"

/*
Global functions
----------------
*/
int export_to_vrml(char *file_name, cmzn_scene_id scene,
	cmzn_scenefilter_id filter);
/******************************************************************************
LAST MODIFIED : 5 January 1998

DESCRIPTION :
Renders the visible objects to a VRML file.
==============================================================================*/
#endif /* !defined (render_vrml_H) */
