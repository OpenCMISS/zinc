/*******************************************************************************
FILE : data_grabber.h

LAST MODIFIED : 18 February 1997

DESCRIPTION :
==============================================================================*/
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
 * Portions created by the Initial Developer are Copyright (C) 2005
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
#if !defined (DATA_GRABBER_H)
#define DATA_GRABBER_H

#include <Xm/Xm.h>
#include "general/callback_motif.h"
#include "general/simple_list.h"
#include "dof3/dof3.h"

#define DATA_GRABBER_PRECISION double
#define DATA_GRABBER_PRECISION_STRING "lf"
#define DATA_GRABBER_NUM_FORMAT "%6.4" DATA_GRABBER_PRECISION_STRING
#define DATA_GRABBER_STRING_SIZE 1000
/*
Global Types
------------
*/
enum DG_mode
/*******************************************************************************
LAST MODIFIED : 25 September 1995

DESCRIPTION :
The components of the position that are required by the user.
==============================================================================*/
{
	DATA_GRABBER_POSITION=1,
	DATA_GRABBER_NORMAL=2,
	DATA_GRABBER_TANGENT=4
}; /* DG_mode */

enum DG_data_type
/*******************************************************************************
LAST MODIFIED : 25 September 1995

DESCRIPTION :
Contains the different types of callbacks that are valid for the data_grabber
control widget.
==============================================================================*/
{
	DATA_GRABBER_UPDATE_CB,
	DATA_GRABBER_SELECT_CB,
	DATA_GRABBER_DATA,
	DATA_GRABBER_MODE
}; /* DG_callback_type */

#define DATA_GRABBER_NUM_CALLBACKS 2

struct DG_data
/*******************************************************************************
LAST MODIFIED : 25 September 1995

DESCRIPTION :
Contains the offset for the position, tangent and normal vectors.
==============================================================================*/
{
	struct Dof3_data position,tangent,normal;
}; /* DG_data */

struct DG_calib_data
/*******************************************************************************
LAST MODIFIED : 25 September 1995

DESCRIPTION :
Contains data for one of the calibration points.
==============================================================================*/
{
	int list_number;
	Gmatrix direction;
	struct Dof3_data position;
}; /* DG_calib_data */

DECLARE_LIST_TYPES(DG_calib_data);

struct DG_calib_struct
/*******************************************************************************
LAST MODIFIED : 25 September 1995

DESCRIPTION :
Contains information for each of the calibration windows that pop up.
==============================================================================*/
{
	int last_number;
	struct LIST(DG_calib_data) *data_list;
	struct Dof3_data global,offset;
	Widget control,scroll,control_clear,control_delete,control_record,
		control_solve;
	Widget dialog,widget;
}; /* DG_calib_struct */

struct DG_struct
/*******************************************************************************
LAST MODIFIED : 25 September 1995

DESCRIPTION :
Contains information required by the data_grabber control dialog.
==============================================================================*/
{
	int button_state,mode,calib_visible,streaming_mode;
	float streaming_distance;
	Gmatrix last_direction;
	struct Callback_data callback_array[DATA_GRABBER_NUM_CALLBACKS];
	struct DG_data current_value,calib_value;
	struct DG_calib_struct calib_struct[3];
	struct Dof3_data last_position,previous_streaming;
	Widget menu_bar,calib_form[3],calib_button[3],calib_label[3],data_form[3],
		data_label[3],data_rowcol,calib_rowcol,calib_menu_button,
		calib_frame,input_module_widget,calib_rc_form[3],data_rc_form[3],
	   stream_distance_entry, stream_distance_text;
	Widget calib_widget[3],data_widget[3];
	Widget widget_parent,widget,*widget_address;
}; /* DG_struct */

/*
UIL Identifiers
---------------
*/
#define data_grabber_menu_bar_ID        1
#define data_grabber_calib_frame_ID            11
#define data_grabber_calib_rowcol_ID            2
#define data_grabber_calib_form_ID            3
#define data_grabber_calib_button_ID            4
#define data_grabber_calib_label_ID            5
#define data_grabber_data_rowcol_ID             6
#define data_grabber_data_form_ID             7
#define data_grabber_data_label_ID             8
#define data_grabber_calib_menu_button_ID                      10
#define data_grabber_calib_control_ID                      12
#define data_grabber_calib_scroll_ID                      13
#define data_grabber_calib_cont_record_ID                      14
#define data_grabber_calib_cont_solve_ID                      15
#define data_grabber_calib_cont_delete_ID                      16
#define data_grabber_calib_cont_clear_ID                      17

/*
Global Functions
---------------
*/
Widget create_data_grabber_widget(Widget *data_grabber_widget,Widget parent,
	int mode);
/*******************************************************************************
LAST MODIFIED : 25 September 1995

DESCRIPTION :
Creates a data_grabber widget that will receive data from an output device, and
send data back to the a caller.
==============================================================================*/

int data_grabber_set_data(Widget data_grabber_widget,
	enum DG_data_type data_type,void *data);
/*******************************************************************************
LAST MODIFIED : 25 September 1995

DESCRIPTION :
Changes a data item of the input widget.
==============================================================================*/

void *data_grabber_get_data(Widget data_grabber_widget,
	enum DG_data_type data_type);
/*******************************************************************************
LAST MODIFIED : 25 September 1995

DESCRIPTION :
Returns a pointer to a data item of the input widget.
==============================================================================*/
#endif
