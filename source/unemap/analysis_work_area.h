/*******************************************************************************
FILE : analysis_work_area.h

LAST MODIFIED : 11 February 2000

DESCRIPTION :
==============================================================================*/
#if !defined (ANALYSIS_WORK_AREA_H)
#define ANALYSIS_WORK_AREA_H

#include <stddef.h>
#if defined (MOTIF)
#include <X11/Intrinsic.h>
#include <X11/Xlib.h>
#include <Xm/Xm.h>
#endif /* defined (MOTIF) */
#include "time/time.h"
#include "time/time_keeper.h"
#include "unemap/analysis_window.h"
#include "unemap/mapping_window.h"
#include "unemap/mapping_work_area.h"
#include "unemap/rig.h"
#include "unemap/trace_window.h"

/*
Global types
------------
*/
struct Analysis_work_area
/*******************************************************************************
LAST MODIFIED : 11 February 2000

DESCRIPTION :
==============================================================================*/
{
	Widget activation;
	struct Analysis_window *window;
	Widget window_shell;
	struct Trace_window *trace;
	struct Mapping_work_area *mapping_work_area;
	struct Mapping_window *mapping_window;
	/* raw rig contains the data that has been read from the file and rig contains
		the processed (filtered, beat averaged, ...) data */
	struct Rig *raw_rig,*rig;
	struct Signal_drawing_package *signal_drawing_package;
	int datum;
	struct Device **highlight;
#if defined (UNEMAP_USE_NODES)
	struct FE_node *highlight_rig_node;
#endif /* defined (UNEMAP_USE_NODES) */
	char calculate_events;
	enum Event_detection_objective objective;
	enum Event_detection_algorithm detection;
	int event_number,number_of_events;
	int potential_time;
	int minimum_separation,threshold;
	float level;
	enum Datum_type datum_type;
	enum Edit_order edit_order;
	enum Signal_order signal_order;
	enum Map_type map_type;
	int map_type_changed; /* flag 1/0 true/false */
	struct File_open_data *bard_signal_file_data,*cardiomapp_signal_file_data,
		*neurosoft_signal_file_data;
	/* the <search_interval_divisions> mark the internal boundaries between beats
		(<number_of_events>-1 values).  If <search_interval_divisions> is NULL then
		the beats are assumed to be of equal length */
	int end_search_interval,*search_interval_divisions,start_search_interval;
	struct Signal_drawing_information *signal_drawing_information;
	struct Map_drawing_information *map_drawing_information;
	struct User_interface *user_interface;
	int pointer_sensitivity;
	char *configuration_file_extension,*postscript_file_extension;
	/* user settings */
	char *events_file_extension;
	int average_width;
	Pixel identifying_colour;
	/* these flags ensure that when the next update occurs the XOR graphics are
		drawn correctly */
	enum Analysis_window_update_flags analysis_update_flags;
	enum Trace_window_update_flags trace_update_flags;
	/* the potential_time_object is controlled by the time_keeper */
	struct Time_object *potential_time_object;
	struct Time_keeper *time_keeper;
	/* the datum_time_object is internal and handles events between the different
		parts of the analysis work area but is not input to a timekeeper */
	struct Time_object *datum_time_object;
	struct Unemap_package *unemap_package;
}; /* struct Analysis_work_area */

/*
Global functions
----------------
*/
int highlight_analysis_device(unsigned int multiple_selection,
	struct Device **device,int *device_number,int *electrode_number,
	int *auxiliary_number,struct Analysis_work_area *analysis);
/*******************************************************************************
LAST MODIFIED : 20 April 1998

DESCRIPTION :
If the highlight is part of a multiple selection
then
  If the <device> is not highlighted
  then
    highlight it and make it THE highlighted device for the analysis work area
  else
    if it is the only highlighted device
    then
      do nothing
    else
      dehighlight it
      if it is THE highlighted device for the analysis work area
      then
        make the first highlighted device THE highlighted device
else
  highlight it and dehighlight all other devices
  make it THE highlighted device for the analysis work area
==============================================================================*/

#if defined (UNEMAP_USE_NODES) 
/* need to alter to use callbacks, cf highlight_analysis_device*/
int highlight_analysis_device_node(unsigned int multiple_selection,
	struct FE_node *device_node,	int *device_number,int *electrode_number,
	int *auxiliary_number,struct Analysis_work_area *analysis);
/*******************************************************************************
LAST MODIFIED : 5 September 2000

DESCRIPTION :
If the highlight is part of a multiple selection
then
  If the <device_node> is not highlighted
  then
    highlight it and make it THE highlighted device for the analysis work area
  else
    if it is the only highlighted device
    then
      do nothing
    else
      dehighlight it
      if it is THE highlighted device for the analysis work area
      then
        make the first highlighted device THE highlighted device
else
  highlight it and dehighlight all other devices
  make it THE highlighted device for the analysis work area

cf highlight_analysis_device

==============================================================================*/
#endif /* defined (UNEMAP_USE_NODES)  */

void set_mapping_analysis_region(Widget widget,XtPointer analysis_work_area,
	XtPointer call_data);
/*******************************************************************************
LAST MODIFIED : 16 September 1992

DESCRIPTION :
Called when a new rig region is selected from the mapping window in the
analysis work area.
==============================================================================*/

void analysis_select_auxiliary_drawi(Widget widget,
	XtPointer analysis_work_area,XtPointer call_data);
/*******************************************************************************
LAST MODIFIED : 4 July 1992

DESCRIPTION :
The callback for selecting a device in the analysis work area (signals drawing
area, mapping drawing area, colour bar or auxiliary devices drawing area).
==============================================================================*/

void analysis_select_map_drawing_are(Widget widget,
	XtPointer analysis_work_area,XtPointer call_data);
/*******************************************************************************
LAST MODIFIED : 4 July 1992

DESCRIPTION :
The callback for selecting a device in the analysis work area (signals drawing
area, mapping drawing area, colour bar or auxiliary devices drawing area).
==============================================================================*/

#if defined (NEW_CODE)
#if defined (UNEMAP_USE_NODES)
void rig_node_group_node_selection_change(struct FE_node_selection *node_selection,
	struct FE_node_selection_changes *changes,void *data);
/*******************************************************************************
LAST MODIFIED : 6 September 2000

DESCRIPTION :
Callback for change in the  node selection. Checks to see if nodes is are in
of the rig_node group. If are, highlights them
==============================================================================*/
#endif /* defined (UNEMAP_USE_NODES) */
#endif /* defined (NEW_CODE) */

int create_analysis_work_area(struct Analysis_work_area *analysis,
	Widget activation,Widget parent,int pointer_sensitivity,
	char *signal_file_extension_read,char *signal_file_extension_write,
	char *postscript_file_extension,char *configuration_file_extension,
	Pixel identifying_colour,
	struct Map_drawing_information *map_drawing_information,
	struct User_interface *user_interface, struct Time_keeper *time_keeper,
	struct Unemap_package *package);
/*******************************************************************************
LAST MODIFIED : 30 April 1999

DESCRIPTION :
Creates the windows associated with the analysis work area.
???Allocate memory for the work area ?
???system_window_structure ?
???associate_mapping_acquisition ?
==============================================================================*/

void close_analysis_work_area(Widget widget,
	XtPointer analysis_work_area,XtPointer call_data);
/*******************************************************************************
LAST MODIFIED : 21 June 1997

DESCRIPTION :
Closes the windows associated with the analysis work area.
==============================================================================*/
#endif
