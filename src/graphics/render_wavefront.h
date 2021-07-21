/*******************************************************************************
FILE : render_wavefront.h

LAST MODIFIED : 20 October 1998

DESCRIPTION :
Renders gtObjects to WAVEFRONT file
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (render_wavefront_H)
#define render_wavefront_H

struct cmzn_scene;
struct cmzn_scenefilter;

/*
Global functions
----------------
*/
int export_to_wavefront(char *file_name, cmzn_scene *scene,
	cmzn_scenefilter *filter, int full_comments);
/******************************************************************************
LAST MODIFIED : 19 October 1998

DESCRIPTION :
Renders the visible objects to Wavefront object files.
==============================================================================*/
#endif /* !defined (render_wavefront_H) */
