/*******************************************************************************
FILE : colour_editor_wx.cpp

LAST MODIFIED : 6 November 2007

DESCRIPTION :
Creates a window that allows the user to create a colour.  Each colour component
ranges between 0-1.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
extern "C"
{
#include <float.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>
#include "general/debug.h"
#include "material/material_editor_wx.h"
#include "user_interface/message.h"
#include "user_interface/user_interface.h"
}
#include "colour/colour_editor_wx.hpp"

COLOUR_PRECISION min(COLOUR_PRECISION *data,int number)
/*******************************************************************************
LAST MODIFIED : 12 June 1994

DESCRIPTION :
Finds the minimum of the passed values.
==============================================================================*/
{
	COLOUR_PRECISION return_value;
	int i;

	ENTER(min);
	return_value=FLT_MAX;
	for (i=0;i<number;i++)
	{
		if (return_value>=data[i])
		{
			return_value=data[i];
		}
	}
	LEAVE;

	return (return_value);
} /* min */

COLOUR_PRECISION max(COLOUR_PRECISION *data,int number)
/*******************************************************************************
LAST MODIFIED : 12 June 1994

DESCRIPTION :
Finds the maximum of the passed values.
==============================================================================*/
{
	COLOUR_PRECISION return_value;
	int i;

	ENTER(max);
	return_value=-FLT_MAX;
	for (i=0;i<number;i++)
	{
		if (return_value<=data[i])
		{
			return_value=data[i];
		}
	}
	LEAVE;

	return (return_value);
} /* max */

Colour_editor::Colour_editor(wxPanel* parent, const char *panel_name, enum Colour_editor_mode mode,
	 struct Colour *colour, void *material_editor_temp)
{
	 colour_editor_panel = parent;
	 material_editor_void = material_editor_temp;
	 title = new wxStaticText(colour_editor_panel, -1, wxT(panel_name), 
			 wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE);
	 const wxString colour_mode_choices[] = {
			wxT("RGB"),
			wxT("HSV"),
			wxT("CMV")
	 };

	USE_PARAMETER(colour);
	 colour_text_choice[0][0] = wxT("Red");
	 colour_text_choice[0][1] = wxT("Green");
	 colour_text_choice[0][2] = wxT("Blue");
	 colour_text_choice[1][0] = wxT("Hue");
	 colour_text_choice[1][1] = wxT("Saturation");
	 colour_text_choice[1][2] = wxT("Value");
	 colour_text_choice[2][0] = wxT("Cyan");
	 colour_text_choice[2][1] = wxT("Magenta");
	 colour_text_choice[2][2] = wxT("Yellow");

    colour_mode_choice = new wxChoice(colour_editor_panel, 
			 -1, wxDefaultPosition, wxSize(-1,26), 3, colour_mode_choices, 0);
    colour_mode_choice->SetSelection(0);
    colour_palette_panel = new wxPanel(colour_editor_panel, -1);
    colour_editor_colour_text_1 = new wxStaticText(colour_editor_panel, -1, colour_text_choice[0][0],wxPoint(-1,-1),wxSize(-1,20));
    colour_editor_text_ctrl_1 = new wxTextCtrl(colour_editor_panel, -1, wxT("0.0000"),wxPoint(-1,-1),wxSize(-1,20), wxTE_PROCESS_ENTER);
    colour_editor_slider_1 = new wxSlider(colour_editor_panel, -1, 0, 0, 100, wxDefaultPosition, 
			 wxSize(-1,20), wxSL_HORIZONTAL);
    colour_editor_colour_text_2 = new wxStaticText(colour_editor_panel, -1, colour_text_choice[0][1],wxPoint(-1,-1),wxSize(-1,20));
    colour_editor_text_ctrl_2 = new wxTextCtrl(colour_editor_panel, -1, wxT("0.0000"),wxPoint(-1,-1),wxSize(-1,20), wxTE_PROCESS_ENTER);
    colour_editor_slider_2 = new wxSlider(colour_editor_panel, -1, 0, 0, 100, wxDefaultPosition, 
			 wxSize(-1,20), wxSL_HORIZONTAL);
    colour_editor_colour_text_3 = new wxStaticText(colour_editor_panel, -1, colour_text_choice[0][2],wxPoint(-1,-1),wxSize(-1,20));
    colour_editor_text_ctrl_3 = new wxTextCtrl(colour_editor_panel, -1, wxT("0.0000"),wxPoint(-1,-1),wxSize(-1,20), wxTE_PROCESS_ENTER);
    colour_editor_slider_3 = new wxSlider(colour_editor_panel, -1, 0, 0, 100, wxDefaultPosition, 
			 wxSize(-1,20), wxSL_HORIZONTAL);
    colour_editor_staticbox = new wxStaticBox(colour_editor_panel, -1, wxT(""));
	 current.red = 0;
	 current.green = 0;
	 current.blue = 0;
	 current_mode = mode;
	 return_mode = mode;

	 colour_editor_text_ctrl_1->SetClientData((void *)this);
	 colour_editor_text_ctrl_1->Connect(wxEVT_COMMAND_TEXT_ENTER, 
			wxCommandEventHandler(Colour_editor::OnColourEditorTextEntered));
	 colour_editor_text_ctrl_1->Connect(wxEVT_KILL_FOCUS, 
			wxCommandEventHandler(Colour_editor::OnColourEditorTextEntered));

	 colour_editor_text_ctrl_2->SetClientData((void *)this);
	 colour_editor_text_ctrl_2->Connect(wxEVT_COMMAND_TEXT_ENTER, 
			wxCommandEventHandler(Colour_editor::OnColourEditorTextEntered));
	 colour_editor_text_ctrl_2->Connect(wxEVT_KILL_FOCUS, 
			wxCommandEventHandler(Colour_editor::OnColourEditorTextEntered));

	 colour_editor_text_ctrl_3->SetClientData((void *)this);
	 colour_editor_text_ctrl_3->Connect(wxEVT_COMMAND_TEXT_ENTER, 
			wxCommandEventHandler(Colour_editor::OnColourEditorTextEntered));
	 colour_editor_text_ctrl_3->Connect(wxEVT_KILL_FOCUS, 
			wxCommandEventHandler(Colour_editor::OnColourEditorTextEntered));

	 colour_editor_slider_1->SetClientData((void *)this);
	 colour_editor_slider_1->Connect(wxEVT_SCROLL_THUMBTRACK, 
			wxCommandEventHandler(Colour_editor::OnColourEditorSliderChanged));
	 colour_editor_slider_1->Connect(wxEVT_SCROLL_PAGEDOWN, 
			wxCommandEventHandler(Colour_editor::OnColourEditorSliderChanged));
	 colour_editor_slider_1->Connect(wxEVT_SCROLL_PAGEUP, 
			wxCommandEventHandler(Colour_editor::OnColourEditorSliderChanged));

	 colour_editor_slider_2->SetClientData((void *)this);
	 colour_editor_slider_2->Connect(wxEVT_SCROLL_THUMBTRACK,
			wxCommandEventHandler(Colour_editor::OnColourEditorSliderChanged));
	 colour_editor_slider_2->Connect(wxEVT_SCROLL_PAGEDOWN, 
			wxCommandEventHandler(Colour_editor::OnColourEditorSliderChanged));
	 colour_editor_slider_2->Connect(wxEVT_SCROLL_PAGEUP, 
			wxCommandEventHandler(Colour_editor::OnColourEditorSliderChanged));

	 colour_editor_slider_3->SetClientData((void *)this);
	 colour_editor_slider_3->Connect(wxEVT_SCROLL_THUMBTRACK, 
			wxCommandEventHandler(Colour_editor::OnColourEditorSliderChanged));
	 colour_editor_slider_3->Connect(wxEVT_SCROLL_PAGEDOWN, 
			wxCommandEventHandler(Colour_editor::OnColourEditorSliderChanged));
	 colour_editor_slider_3->Connect(wxEVT_SCROLL_PAGEUP, 
			wxCommandEventHandler(Colour_editor::OnColourEditorSliderChanged));


	 colour_mode_choice->SetClientData((void *)this);
	 colour_mode_choice->Connect(wxEVT_COMMAND_CHOICE_SELECTED,
			wxCommandEventHandler(Colour_editor::OnColourEditorColourModeChoiceChanged));

    do_layout();
}

void Colour_editor::do_layout()
/*******************************************************************************
LAST MODIFIED : 29 November 2007

DESCRIPTION :
Set up the widgets.
==============================================================================*/
{
    // begin wxGlade: MyFrame::do_layout
    wxBoxSizer* sizer_1 = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* sizer_2 = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* sizer_3 = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* sizer_5 = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* sizer_9 = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* sizer_8 = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* sizer_7 = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* sizer_6 = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* sizer_4 = new wxBoxSizer(wxVERTICAL);
    wxStaticBoxSizer* sizer_10 = new wxStaticBoxSizer(colour_editor_staticbox, wxHORIZONTAL);
    sizer_2->Add(title, 0, wxALIGN_CENTER_HORIZONTAL|wxADJUST_MINSIZE, 0);
    sizer_4->Add(colour_mode_choice, 0, wxALIGN_CENTER_HORIZONTAL|wxADJUST_MINSIZE, 0);
    sizer_10->Add(colour_palette_panel, 1, wxEXPAND, 0);
    sizer_4->Add(sizer_10, 1, wxEXPAND, 0);
    sizer_3->Add(sizer_4, 1, wxEXPAND, 0);
    sizer_6->Add(colour_editor_colour_text_1, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);
    sizer_6->Add(colour_editor_text_ctrl_1, 0, wxRIGHT|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);
    sizer_5->Add(sizer_6, 1, wxEXPAND, 0);
    sizer_7->Add(colour_editor_slider_1, 1, wxEXPAND|wxADJUST_MINSIZE, 0);
    sizer_5->Add(sizer_7, 1, wxEXPAND, 0);
    sizer_8->Add(colour_editor_colour_text_2, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);
    sizer_8->Add(colour_editor_text_ctrl_2, 0, wxRIGHT|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);
    sizer_5->Add(sizer_8, 1, wxEXPAND, 0);
    sizer_5->Add(colour_editor_slider_2, 1, wxEXPAND|wxADJUST_MINSIZE, 0);
    sizer_9->Add(colour_editor_colour_text_3, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);
    sizer_9->Add(colour_editor_text_ctrl_3, 0, wxRIGHT|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);
    sizer_5->Add(sizer_9, 1, wxEXPAND, 0);
    sizer_5->Add(colour_editor_slider_3, 1, wxEXPAND|wxADJUST_MINSIZE, 0);
    sizer_3->Add(sizer_5, 1, wxEXPAND, 0);
    sizer_2->Add(sizer_3, 1, wxEXPAND, 0);
    colour_editor_panel->SetAutoLayout(true);
    colour_editor_panel->SetSizer(sizer_2);
    sizer_2->Fit(colour_editor_panel);
    sizer_2->SetSizeHints(colour_editor_panel);
    sizer_1->Add(colour_editor_panel, 1, wxEXPAND, 0);
    colour_editor_panel->SetAutoLayout(true);
    colour_editor_panel->SetSizer(sizer_1);
    sizer_1->Fit(colour_editor_panel);
    sizer_1->SetSizeHints(colour_editor_panel);
    colour_editor_panel->Layout();
}

void Colour_editor::set_properties()
{
    colour_mode_choice->SetSelection(0);
}

void Colour_editor::colour_editor_wx_update()
{
	 ENTER(Colour_editor::colour_editor_wx_update);

	 material_editor_update_colour(material_editor_void);

	 LEAVE;
}

void Colour_editor::colour_editor_wx_get_colour_from_interface_textctrl(struct Colour *colour)
/*******************************************************************************
LAST MODIFIED : 29 November 2007

DESCRIPTION :
Get the colour struct from the text control of the interface.
==============================================================================*/
{
	 ENTER(Colour_editor::colour_editor_wx_get_colour_from_interface);

	 char *text;
	 float temp;
	 text = (char*)colour_editor_text_ctrl_1->GetValue().mb_str(wxConvUTF8);
	 if (text)
	 {
			sscanf(text,"%f",&temp);
			colour->red = temp;
	 }
	 else
	 {
			display_message(ERROR_MESSAGE,
				 "Colour_editor::colour_editor_wx_get_colour_from_interface. Missing colour editor text ");
	 }
	 text = NULL;
	 text = (char*)colour_editor_text_ctrl_2->GetValue().mb_str(wxConvUTF8);
	 if (text)
	 {
			sscanf(text,"%f",&temp);
			colour->green = temp;
	 }
	 else
	 {
			display_message(ERROR_MESSAGE,
				 "Colour_editor::colour_editor_wx_get_colour_from_interface. Missing colour editor text ");
	 }
	 text = NULL;
	 text = (char*)colour_editor_text_ctrl_3->GetValue().mb_str(wxConvUTF8);
	 if (text)
	 {
			sscanf(text,"%f",&temp);
			colour->blue = temp;
	 }
	 else
	 {
			display_message(ERROR_MESSAGE,
				 "Colour_editor::colour_editor_wx_get_colour_from_interface. Missing colour editor text ");
	 }

	 LEAVE;
}

void Colour_editor::OnColourEditorTextEntered(wxCommandEvent& event)
/*******************************************************************************
LAST MODIFIED : 29 November 2007

DESCRIPTION :
Process enter text event. Eventhandler somehow does not return any information of the colour
editor class although this function is inside the class. Therefore, I store the colour
editor inside the control object and get the control object whenever an event is
provoked then use this colour editor to do the settings.
==============================================================================*/
{
	 ENTER(Colour_editor::OnColourEditorTextEntered);

	 wxTextCtrl *temp_object = (wxTextCtrl *)event.GetEventObject();
	 Colour temp_colour;
	 Colour_editor *temp_editor = (Colour_editor *)temp_object->GetClientData();
	 temp_editor->colour_editor_wx_get_colour_from_interface_textctrl(&temp_colour);
	 temp_editor->colour_editor_wx_set_colour(&temp_colour);
	 temp_editor->colour_editor_wx_update();
	 LEAVE;
}

void Colour_editor::colour_editor_wx_get_colour_from_interface_slider(struct Colour *colour)
/*******************************************************************************
LAST MODIFIED : 29 November 2007

DESCRIPTION :
Get the colour struct from the slider of the interface.
==============================================================================*/
{
	 ENTER(Colour_editor::colour_editor_wx_get_colour_from_interface_slider);

	 int value;
	 float temp;
	 
	 value = colour_editor_slider_1->GetValue();
	 temp = (float)value/(float)100.0;
	 colour->red = temp;

	 value = colour_editor_slider_2->GetValue();
	 temp = (float)value/(float)100.0;
	 colour->green = temp;

	 value = colour_editor_slider_3->GetValue();
	 temp = (float)value/(float)100.0;
	 colour->blue = temp;

	 LEAVE;
}

void Colour_editor::OnColourEditorSliderChanged(wxCommandEvent& event)
/*******************************************************************************
LAST MODIFIED : 29 November 2007

DESCRIPTION :
Process scroll event. Eventhandler somehow does not return any information of the colour
editor class although this function is inside the class. Therefore, I store the colour
editor inside the control object and get the control object whenever an event is
provoked then use this colour editor to do the settings.
==============================================================================*/
{
	 ENTER(Colour_editor::OnColourEditorSliderChanged);

	 wxSlider *temp_object = (wxSlider *)event.GetEventObject();
	 Colour temp_colour;
	 Colour_editor *temp_editor = (Colour_editor *)temp_object->GetClientData();
	 temp_editor->colour_editor_wx_get_colour_from_interface_slider(&temp_colour);
	 temp_editor->colour_editor_wx_set_colour(&temp_colour);
	 temp_editor->colour_editor_wx_update();
	 LEAVE;
}

void Colour_editor::colour_editor_wx_change_mode_from_choice()
{
	 ENTER(Colour_editor::colour_editor_wx_change_mode_from_choice);
	 
	 int selection;
	 struct Colour temp_colour;

	 selection = colour_mode_choice->GetCurrentSelection();
	 if (selection == 0)
	 {
			return_mode = COLOUR_EDITOR_RGB;
			colour_editor_colour_text_1->SetLabel(colour_text_choice[0][0]);
			colour_editor_colour_text_2->SetLabel(colour_text_choice[0][1]);
			colour_editor_colour_text_3->SetLabel(colour_text_choice[0][2]);
	 }
	 else if (selection == 1)
	 {
			return_mode = COLOUR_EDITOR_HSV;
			colour_editor_colour_text_1->SetLabel(colour_text_choice[1][0]);
			colour_editor_colour_text_2->SetLabel(colour_text_choice[1][1]);
			colour_editor_colour_text_3->SetLabel(colour_text_choice[1][2]);
	 }
	 else if (selection == 2)
	 {
			return_mode = COLOUR_EDITOR_CMY;
			colour_editor_colour_text_1->SetLabel(colour_text_choice[2][0]);
			colour_editor_colour_text_2->SetLabel(colour_text_choice[2][1]);
			colour_editor_colour_text_3->SetLabel(colour_text_choice[2][2]);
	 }
	 temp_colour = current;
	 colour_editor_wx_conversion(current_mode, return_mode, &temp_colour, &current);
	 current_mode = return_mode;

	 LEAVE;
}

void Colour_editor::OnColourEditorColourModeChoiceChanged(wxCommandEvent& event)
/*******************************************************************************
LAST MODIFIED : 29 November 2007

DESCRIPTION :
Process scroll event. Eventhandler somehow does not return any information of the colour
editor class although this function is inside the class. Therefore, I store the colour
editor inside the control object and get the control object whenever an event is
provoked then use this colour editor to do the settings.
==============================================================================*/
{
	 ENTER(Colour_editor::OnColourEditorColourModeChoice);

	 wxChoice *temp_object = (wxChoice *)event.GetEventObject();
	 Colour_editor *temp_editor = (Colour_editor *)temp_object->GetClientData();
	 int i;

	 temp_editor->colour_editor_wx_change_mode_from_choice();
	 for ( i =0; i < 3; i++)
	 {
			temp_editor->colour_editor_wx_update_value(i);
	 }

	 LEAVE;
}

void Colour_editor::colour_editor_wx_conversion(enum Colour_editor_mode old_mode,
	 enum Colour_editor_mode new_mode,struct Colour *old_data,
	 struct Colour *new_data)
/*******************************************************************************
LAST MODIFIED : 13 November 2007

DESCRIPTION :
Converts between the different colour_editor modes.  Algorithms taken from
Foley and van Damm p592.
==============================================================================*/
{
	 ENTER(Colour_editor::colour_editor_wx_conversion);

	 COLOUR_PRECISION temp_delta,temp_f,temp_old0,temp_p = 0,temp_q = 0,temp_t = 0,temp_max,
			temp_min;
	 int temp_i = 0;
	 
	 if (new_mode==old_mode)
	 {
			new_data->red=old_data->red;
			new_data->green=old_data->green;
			new_data->blue=old_data->blue;
	 }
	 else
	 {
			/* use nested case statements here as in future we may have more than
				 two colour_editor types. */
			switch (old_mode)
			{
				 case COLOUR_EDITOR_RGB:
				 {
						switch (new_mode)
						{
							 case COLOUR_EDITOR_HSV:
							 {
									temp_min=min((COLOUR_PRECISION *)old_data,3);
									temp_max=max((COLOUR_PRECISION *)old_data,3);
									new_data->blue=temp_max;
									if (temp_max)
									{
										 new_data->green=(temp_max-temp_min)/temp_max;
									}
									else
									{
										 new_data->green=0;
									}
									if (new_data->green)
									{
										 temp_delta=temp_max-temp_min;
										 if (old_data->red==temp_max)
										 {
												new_data->red=(old_data->green-old_data->blue)/
													 temp_delta;
										 }
										 else if (old_data->green==temp_max)
										 {
												new_data->red=2+(old_data->blue-old_data->red)/
													 temp_delta;
										 }
										 else if (old_data->blue==temp_max)
										 {
												new_data->red=4+(old_data->red-old_data->green)/
													 temp_delta;
										 }
										 if (new_data->red<0)
										 {
												new_data->red +=6;
										 }
										 new_data->red /=6; /* bring it down into the range 0-1 */
									}
									else
									{
										 new_data->red=0; /* dont use undefined */
									}
							} break;
							case COLOUR_EDITOR_CMY:
							{
									new_data->red=1.0-old_data->red;
									new_data->green=1.0-old_data->green;
									new_data->blue=1.0-old_data->blue;
							} break;
							case COLOUR_EDITOR_RGB:
							{
								/* Do nothing */
							} break;
						}
				} break;
				case COLOUR_EDITOR_HSV:
				{
						switch (new_mode)
						{
							case COLOUR_EDITOR_RGB:
							{
									if (old_data->green==0)
									{
										 new_data->red=old_data->blue;
										 new_data->green=old_data->blue;
										 new_data->blue=old_data->blue;
									}
									else
									{
										temp_old0=old_data->red;
										if (temp_old0==1)
										{
											temp_old0=0;
										}
										temp_old0 *=6;
										temp_i= (int)floor(temp_old0);
										temp_f=temp_old0-temp_i;
										temp_p=old_data->blue*(1-old_data->green);
										temp_q=old_data->blue*(1-(old_data->green*temp_f));
										temp_t=old_data->blue*(1-(old_data->green*(1-temp_f)));
										switch (temp_i)
										{
											case 0:
											{
												new_data->red=old_data->blue;
												new_data->green=temp_t;
												new_data->blue=temp_p;
											} break;
											case 1:
											{
												new_data->red=temp_q;
												new_data->green=old_data->blue;
												new_data->blue=temp_p;
											} break;
											case 2:
											{
													new_data->red=temp_p;
													new_data->green=old_data->blue;
													new_data->blue=temp_t;
											} break;
											case 3:
											{
												new_data->red=temp_p;
												new_data->green=temp_q;
												new_data->blue=old_data->blue;
											} break;
											case 4:
											{
												new_data->red=temp_t;
												new_data->green=temp_p;
												new_data->blue=old_data->blue;
											} break;
											case 5:
											{
												new_data->red=old_data->blue;
												new_data->green=temp_p;
												new_data->blue=temp_q;
											} break;
										}
									}
							} break;
							case COLOUR_EDITOR_CMY:
							{
								if (old_data->green==0)
								{
									new_data->red=1-old_data->blue;
									new_data->green=1-old_data->blue;
									new_data->blue=1-old_data->blue;
								}
								else
								{
									temp_old0=old_data->red;
									if (temp_old0==1)
									{
										temp_old0=0;
									}
									temp_old0 *=6;
									temp_i= (int)floor(temp_old0);
									temp_f=temp_old0-temp_i;
									temp_p=old_data->blue*(1-old_data->green);
									temp_q=old_data->blue*(1-(old_data->green*temp_f));
									temp_t=old_data->blue*(1-(old_data->green*(1-temp_f)));
								}
								switch (temp_i)
								{
									case 0:
									{
										new_data->red=1-old_data->blue;
										new_data->green=1-temp_t;
										new_data->blue=1-temp_p;
									} break;
									case 1:
									{
										new_data->red=1-temp_q;
										new_data->green=1-old_data->blue;
										new_data->blue=1-temp_p;
									} break;
									case 2:
									{
										new_data->red=1-temp_p;
										new_data->green=1-old_data->blue;
										new_data->blue=1-temp_t;
									} break;
									case 3:
									{
										new_data->red=1-temp_p;
										new_data->green=1-temp_q;
										new_data->blue=1-old_data->blue;
									} break;
									case 4:
									{
										new_data->red=1-temp_t;
										new_data->green=1-temp_p;
										new_data->blue=1-old_data->blue;
									} break;
									case 5:
									{
										new_data->red=1-old_data->blue;
										new_data->green=1-temp_p;
										new_data->blue=1-temp_q;
									} break;
								}
							} break;
							case COLOUR_EDITOR_HSV:
							{
								/* Do nothing */
							} break;
						}
				} break;
				case COLOUR_EDITOR_CMY:
				{
					switch (new_mode)
					{
						case COLOUR_EDITOR_RGB:
						{
							new_data->red=1.0-old_data->red;
							new_data->green=1.0-old_data->green;
							new_data->blue=1.0-old_data->blue;
						} break;
						case COLOUR_EDITOR_HSV:
						{
							temp_max=1-min((COLOUR_PRECISION *)old_data,3);
							temp_min=1.0-max((COLOUR_PRECISION *)old_data,3);
							new_data->blue=temp_max;
							if (temp_max)
							{
								new_data->green=(temp_max-temp_min)/temp_max;
							}
							else
							{
								new_data->green=0;
							}
							if (new_data->green)
							{
								temp_delta=temp_max-temp_min;
								if ((1.0-old_data->red)==temp_max)
								{
									new_data->red=-(old_data->green-old_data->blue)/
										 temp_delta;
								}
								else if ((1.0-old_data->green)==temp_max)
								{
									new_data->red=2-(old_data->blue-old_data->red)/
										 temp_delta;
								}
								else if ((1.0-old_data->blue)==temp_max)
								{
									new_data->red=4-(old_data->red-old_data->green)/
										 temp_delta;
								}
								if (new_data->red<0)
								{
									new_data->red +=6;
								}
								new_data->red /=6; /* bring it down into the range 0-1 */
								}
								else
								{
									new_data->red=0; /* dont use undefined */
								}
						} break;
						case COLOUR_EDITOR_CMY:
						{
							/* Do nothing */
						} break;
					}
				} break;
			}
	}
	 LEAVE;
} /* colour_editor_conversion */

void Colour_editor::colour_editor_wx_update_value(
	int item_num)
/*******************************************************************************
LAST MODIFIED : 13 November 2007

DESCRIPTION :
Makes the numeric value and the slider agree for the particular combo.
==============================================================================*/
{
	 ENTER(Colour_editor::colour_editor_wx_update_value);

	 char temp_str[20];
	 COLOUR_PRECISION temp;
	 int slider_value;
	 
	 switch (item_num)
	 {
			case 0:
			{
				 sprintf(temp_str,COLOUR_NUM_FORMAT,current.red);
				 colour_editor_text_ctrl_1->ChangeValue(temp_str);
				 temp = current.red *100;
				 slider_value = (int)(temp+0.5);
				 colour_editor_slider_1->SetValue(slider_value);
			} break;
			case 1:
			{
				 sprintf(temp_str,COLOUR_NUM_FORMAT,current.green);
				 colour_editor_text_ctrl_2->ChangeValue(temp_str);
				 temp = current.green *100;
				 slider_value = (int)(temp+0.5);
				 colour_editor_slider_2->SetValue(slider_value);
			} break;
			case 2:
			{
				 sprintf(temp_str,COLOUR_NUM_FORMAT,current.blue);
				 colour_editor_text_ctrl_3->ChangeValue(temp_str);
				 temp = current.blue *100;
				 slider_value = (int)(temp+0.5);
				 colour_editor_slider_3->SetValue(slider_value);
			} break;	
	 }
	 LEAVE;	
} /* Colour_editor::colour_editor_wx_update_value */

void Colour_editor::colour_editor_wx_update_panel_colour()
{
	 ENTER(Colour_editor::colour_editor_wx_update_panel_colour);

	 Colour temp_colour;
	 colour_editor_wx_conversion(current_mode, COLOUR_EDITOR_RGB, &current, &temp_colour);
	 float red = temp_colour.red * (float)255.0;
	 float green = temp_colour.green * (float)255.0;
	 float blue = temp_colour.blue * (float)255.0;
	 const wxColour panel_colour = wxColour((unsigned char)red, (unsigned char)green,(unsigned char)blue,255);
	 colour_palette_panel->SetOwnBackgroundColour(panel_colour);

	 LEAVE;
}

Colour Colour_editor::colour_editor_wx_get_colour()
/*******************************************************************************
LAST MODIFIED : 30 November 2007

DESCRIPTION :
Get current colour.
==============================================================================*/
{
	 Colour temp_colour;

	 colour_editor_wx_conversion(current_mode,COLOUR_EDITOR_RGB,
			&current, &temp_colour);

	 return (temp_colour);
}

int Colour_editor::colour_editor_wx_set_colour(struct Colour *colour)
/*******************************************************************************
LAST MODIFIED : 13 November 2007

DESCRIPTION :
Copies <colour> into the value in the <colour_editor_widget>.
==============================================================================*/
{	 
	 ENTER(Colour_editor::colour_editor_wx_set_colour);

	 int i, return_code;
	 
	 if ((colour_editor_text_ctrl_1) && (colour_editor_text_ctrl_2) &&
			(colour_editor_text_ctrl_3))
	 {
			colour_editor_panel->Freeze();
			if (colour)
			{
				 colour_editor_wx_conversion(return_mode, current_mode,colour, &current);
			}
			for ( i =0; i < 3; i++)
			{
				 colour_editor_wx_update_value(i);
			}
			colour_editor_wx_update_panel_colour();
			colour_editor_panel->Thaw();
			colour_editor_panel->Update();
			return_code=1;
	 } 
	 else
	 {
			display_message(ERROR_MESSAGE,
				 "Colour_editor::colour_editor_wx_set_colour.  Missing widget");
			return_code=0;
	 }

	 LEAVE;
 	 return (return_code);
} /* Colour_editor::colour_editor_wx_set_colour */
