/*******************************************************************************
FILE : render_stl.h

LAST MODIFIED : 3 July 2008

DESCRIPTION :
Renders gtObjects to STL stereolithography file.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (RENDERSTL_H)
#define RENDERSTL_H

struct cmzn_scene;
struct cmzn_scenefilter;
/*
Global functions
----------------
*/

/**************************************************************************//**
 * Renders the visible objects to an STL file.
 * 
 * @param file_name The name of the file to write to.
 * @param scene The scene to output
 * @param filter The filter on scene
 * @return 1 on success, 0 on failure
 */
int export_to_stl(char *file_name, cmzn_scene *scene, cmzn_scenefilter *filter);

#endif /* !defined (RENDERSTL_H) */
