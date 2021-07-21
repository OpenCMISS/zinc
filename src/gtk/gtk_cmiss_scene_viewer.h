/*******************************************************************************
FILE : gtk_cmiss_scene_viewer.h

LAST MODIFIED : 23 September 2002

DESCRIPTION :
The gtk interface to the cmzn_scene_viewer object for rendering cmiss
scenes.  This creates a GtkWidget that represents the scene viewer and allows
the cmzn_scene_viewer to be integrated with other Gtk widgets.  To control
the scene viewer get the contained cmzn_scene_viewer object and use its api.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef __GTK_CMZN_SCENE_VIEWER_H__
#define __GTK_CMZN_SCENE_VIEWER_H__

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include "api/cmiss_scene_viewer.h"


#if GTK_MAJOR_VERSION >= 2

G_BEGIN_DECLS

#define GTK_TYPE_CMZN_SCENE_VIEWER \
   (gtk_cmiss_scene_viewer_get_type())
#define GTK_CMZN_SCENE_VIEWER(obj) \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_CMZN_SCENE_VIEWER, GtkcmznSceneViewer))
#define GTK_CMZN_SCENE_VIEWER_CLASS(klass) \
   (G_TYPE_CHECK_CLASS_CAST (klass, GTK_TYPE_CMZN_SCENE_VIEWER, GtkcmznSceneViewerClass))
#define GTK_IS_CMZN_SCENE_VIEWER(obj) \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_CMZN_SCENE_VIEWER))
#define GTK_IS_CMZN_SCENE_VIEWER_CLASS(klass) \
   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_CMZN_SCENE_VIEWER))
#define GTK_CMZN_SCENE_VIEWER_GET_CLASS(obj) \
   (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_CMZN_SCENE_VIEWER, GtkcmznSceneViewer))

#else /* GTK_MAJOR_VERSION >= 2 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GTK_TYPE_CMZN_SCENE_VIEWER \
   (gtk_cmiss_scene_viewer_get_type())
#define GTK_CMZN_SCENE_VIEWER(obj) \
   (GTK_CHECK_CAST ((obj), GTK_TYPE_CMZN_SCENE_VIEWER, GtkcmznSceneViewer))
#define GTK_CMZN_SCENE_VIEWER_CLASS(klass) \
   (GTK_CHECK_CLASS_CAST (klass, GTK_TYPE_CMZN_SCENE_VIEWER, GtkcmznSceneViewerClass))
#define GTK_IS_CMZN_SCENE_VIEWER(obj) \
   (GTK_CHECK_TYPE ((obj), GTK_TYPE_CMZN_SCENE_VIEWER))
#define GTK_IS_CMZN_SCENE_VIEWER_CLASS(klass) \
   (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_CMZN_SCENE_VIEWER))

#endif /* GTK_MAJOR_VERSION >= 2 */

typedef struct _GtkcmznSceneViewer GtkcmznSceneViewer;
/*******************************************************************************
LAST MODIFIED : 9 September 2002

DESCRIPTION :
==============================================================================*/

typedef struct _GtkcmznSceneViewerClass GtkcmznSceneViewerClass;
/*******************************************************************************
LAST MODIFIED : 9 September 2002

DESCRIPTION :
==============================================================================*/

struct _GtkcmznSceneViewer
/*******************************************************************************
LAST MODIFIED : 9 September 2002

DESCRIPTION :
==============================================================================*/
{
	GtkBin bin;

	cmzn_scene_viewer_id cmiss_scene_viewer;
	struct cmzn_scene_viewer_module* cmiss_scene_viewer_module;
};

struct _GtkcmznSceneViewerClass
/*******************************************************************************
LAST MODIFIED : 9 September 2002

DESCRIPTION :
==============================================================================*/
{
	GtkBinClass parent_class;
};

#if GTK_MAJOR_VERSION >= 2
GType gtk_cmiss_scene_viewer_get_type(void);
/*******************************************************************************
LAST MODIFIED : 23 September 2002

DESCRIPTION :
==============================================================================*/
#else /* GTK_MAJOR_VERSION >= 2 */
GtkType gtk_cmiss_scene_viewer_get_type(void);
/*******************************************************************************
LAST MODIFIED : 23 September 2002

DESCRIPTION :
==============================================================================*/
#endif /* GTK_MAJOR_VERSION >= 2 */

GtkWidget* gtk_cmiss_scene_viewer_new(
	struct cmzn_scene_viewer_module *scene_viewer_module);
/*******************************************************************************
LAST MODIFIED : 9 September 2002

DESCRIPTION :
Create a new gtk_cmiss_scene_viewer widget with all the default settings.
To modify this object get the cmzn_scene_viewer handle and then interact with
that.
==============================================================================*/

cmzn_scene_viewer_id gtk_cmiss_scene_viewer_get_cmiss_scene_viewer(
	GtkcmznSceneViewer *gtk_cmiss_scene_viewer);
/*******************************************************************************
LAST MODIFIED : 10 September 2002

DESCRIPTION:
Returns a handle to the cmzn_scene_viewer object which this
<gtk_cmiss_scene_viewer> wraps.  This handle can then be used to interact with
the scene_viewer.
==============================================================================*/

#if GTK_MAJOR_VERSION >= 2
G_END_DECLS
#else /* GTK_MAJOR_VERSION >= 2 */
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* GTK_MAJOR_VERSION >= 2 */
#endif /* __GTK_CMZN_SCENE_VIEWER_H__ */
