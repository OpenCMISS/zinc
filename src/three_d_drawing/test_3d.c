/*******************************************************************************
FILE : test_3d.c

LAST MODIFIED : 17 August 1998

DESCRIPTION :
For testing the 3-D drawing widget.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stdio.h>
#if defined (BUILD_WITH_CMAKE)
#include "configure/configure.h"
#endif /* defined (BUILD_WITH_CMAKE) */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <Xm/Form.h>
#if defined (GL_API)
#include <gl/gl.h>
#endif
#if defined (OPENGL_API)
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#define WHITE 0
#define BLUE 1
#endif
#if defined (PEXLIB_API)
#if defined (IBM)
#include <X11/PEX5/PEXlib.h>
#endif
#if defined (VAX)
#include <PEXlib.h>
#endif
#endif

#include "ThreeDDraw.h"

static void initialize_callback(Widget widget,XtPointer user_data,
	XtPointer call_data)
/*******************************************************************************
LAST MODIFIED : 11 February 1995

DESCRIPTION :
==============================================================================*/
{
#if defined (OPENGL_API) || defined (GL_API)
	Colormap colour_map;
	Window window;
	X3dBufferColourMode colour_mode;
	Display *display;
	XColor colour;
#endif

/*???debug */
printf("initialize_callback\n");
#if defined (OPENGL_API) || defined (GL_API)
	if (widget&&(window=XtWindow(widget)))
	{
		XtVaGetValues(widget,
			X3dNbufferColourMode,&colour_mode,
			XtNcolormap,&colour_map,
			NULL);
		if (X3dCOLOUR_INDEX_MODE==colour_mode)
		{
			/* set up colours for colour index mode */
			if (None!=colour_map)
			{
				/*???DB.  GL doesn't seem to set up predefined colours and mapcolor
					doesn't work for double buffering */
				display=XtDisplay(widget);
				XParseColor(display,colour_map,"white",&colour);
				colour.pixel=(Pixel)WHITE;
				colour.flags=DoRed|DoGreen|DoBlue;
				XStoreColor(display,colour_map,&colour);
				XParseColor(display,colour_map,"blue",&colour);
				colour.pixel=(Pixel)BLUE;
				colour.flags=DoRed|DoGreen|DoBlue;
				XStoreColor(display,colour_map,&colour);
			}
			else
			{
				printf("initialize_callback.  Missing colour map\n");
			}
		}
	}
	else
	{
		printf("initialize_callback.  Missing widget or widget window\n");
	}
#endif
} /* initialize_callback */

static void expose_callback(Widget widget,XtPointer user_data,
	XtPointer call_data)
/*******************************************************************************
LAST MODIFIED : 11 February 1995

DESCRIPTION :
==============================================================================*/
{
	X3dBufferColourMode colour_mode;
	Window window;

	/* check arguments */
	if (widget)
	{
		/* check for window */
		if (window=XtWindow(widget))
		{
			/* simple drawing */
#if defined (GL_API)
			float white_vec[3]={1.,1.,1.};
			float blue_vec[3]={0.,0.,1.};
			long vert1[2]={200,200};
			long vert2[2]={200,400};
			long vert3[2]={400,400};
			long vert4[2]={400,200};

			ortho2(100.5,500.5,100.5,500.5);
			XtVaGetValues(widget,
				X3dNbufferColourMode,&colour_mode,
				NULL);
			switch (colour_mode)
			{
				case X3dCOLOUR_INDEX_MODE:
				{
					color(WHITE);
				} break;
				case X3dCOLOUR_RGB_MODE:
				{
					c3f(blue_vec);
				} break;
			}
			clear();
			switch (colour_mode)
			{
				case X3dCOLOUR_INDEX_MODE:
				{
					color(BLUE);
				} break;
				case X3dCOLOUR_RGB_MODE:
				{
					c3f(white_vec);
				} break;
			}
			bgnline();
			v2i(vert1);
			v2i(vert2);
			v2i(vert3);
			v2i(vert4);
			v2i(vert1);
			endline();
#endif
#if defined (OPENGL_API)
			float white_vec[3]={1.,1.,1.};
			float red_vec[3]={1.,0.,0.};
			float green_vec[3]={0.,1.,0.};
			int vert1[2]={200,200};
			int vert2[2]={200,400};
			int vert3[2]={400,300};
			int vert4[2]={400,200};
			/*#define COLOUR_TABLE*/
#if defined COLOUR_TABLE
			/* Experimenting with a colour table,
			 can't make it do anything even though it doesn't generate
			 errors! */
		
			if ( !glIsEnabled(GL_COLOR_TABLE_SGI))
			{
				static unsigned char table[256];
				/*static unsigned char table[96]
					= {0, 0, 0, 
					0, 0, 0,
					0, 0, 0, 
					200, 200, 200,
					0, 0, 0, 
					200, 200, 200,
					0, 0, 0, 
					200, 200, 200,
					0, 0, 0, 
					200, 200, 200,
					0, 0, 0, 
					200, 200, 200,
					0, 0, 0, 
					200, 200, 200,
					0, 0, 0, 
					200, 200, 200,
					0, 0, 0, 
					200, 200, 200,
					0, 0, 0, 
					200, 200, 200,
					0, 0, 0, 
					200, 200, 200,
					0, 0, 0, 
					200, 200, 200,
					0, 0, 0, 
					200, 200, 200,
					0, 0, 0, 
					200, 200, 200,
					0, 0, 0, 
					200, 200, 200,
					0, 0, 0, 
					200, 200, 200};*/
				float scales[4] = {1, 1, 1, 1};
				float bias[4] = {0, 0, 0, 0};
				GLenum error;
				int i;
				for (i = 0; i < 256; i++)
				{
					table[i] = 255-i;
				}

				while (error = glGetError())
				{
					printf("GL ERROR 0: %s\n",gluErrorString(error));
				}
				glColorTableSGI( GL_COLOR_TABLE_SGI, GL_RGBA8_EXT,
					256, GL_LUMINANCE, GL_UNSIGNED_BYTE, table); 
				while (error = glGetError())
				{
					printf("GL ERROR 1: %s\n",gluErrorString(error));
				}
				glColorTableParameterfvSGI(GL_COLOR_TABLE_SGI,
					GL_COLOR_TABLE_SCALE_SGI, scales);
				glColorTableParameterfvSGI(GL_COLOR_TABLE_SGI,
					GL_COLOR_TABLE_BIAS_SGI, bias);
				glEnable( GL_COLOR_TABLE_SGI );
				while (error = glGetError())
				{
					printf("GL ERROR 2: %s\n",gluErrorString(error));
				}
				glGetColorTableSGI(GL_COLOR_TABLE_SGI, GL_RGB, GL_UNSIGNED_BYTE,
					(GLvoid *)table);
				if ( glIsEnabled(GL_COLOR_TABLE_SGI))
				{
					printf("Table %d %d %d   %d %d %d\n",
						table[0], table[1], table[2],
						table[3], table[4], table[5]);
				}
			}
#endif /* defined COLOUR_TABLE */
			glLoadIdentity();
			gluOrtho2D(100.5,500.5,100.5,500.5);
			XtVaGetValues(widget,
				X3dNbufferColourMode,&colour_mode,
				NULL);
			switch (colour_mode)
			{
				case X3dCOLOUR_INDEX_MODE:
				{
					glClearIndex((GLfloat)WHITE);
				} break;
				case X3dCOLOUR_RGB_MODE:
				{
					glClearColor(0.,0.,1.,1.);
				} break;
			}
			glClear(GL_COLOR_BUFFER_BIT);
			switch (colour_mode)
			{
				case X3dCOLOUR_INDEX_MODE:
				{
					glIndexf((GLfloat)BLUE);
				} break;
				case X3dCOLOUR_RGB_MODE:
				{
					glColor3fv(white_vec);
				} break;
			}
			glBegin(GL_LINE_STRIP);
			glVertex2iv(vert1);
			if ( colour_mode == X3dCOLOUR_RGB_MODE )
			{
				glColor3fv(red_vec);
			}
			glVertex2iv(vert2);
			if ( colour_mode == X3dCOLOUR_RGB_MODE )
			{
				glColor3fv(green_vec);
			}
			glVertex2iv(vert3);
			glVertex2iv(vert4);
			glVertex2iv(vert1);
			glEnd();
#endif
#if defined (PEXLIB_API)
			Display *display;
			PEXColor colour;
			PEXCoord points[]=
			{
				{0.2,0.2,0.0},
				{0.4,0.7,0.0},
				{0.6,0.3,0.0},
				{0.8,0.8,0.0},
			};
			PEXRenderer renderer;
			X3dThreeDDrawCallbackStruct *callback;

			if (callback=(X3dThreeDDrawCallbackStruct *)call_data)
			{
				display=XtDisplay(widget);
				XtVaGetValues(widget,
					X3dNbufferColourMode,&colour_mode,
					X3dNrenderingContext,&renderer,
					NULL);
				XClearWindow(display,window);
					/*???DB.  Still need to set background pixel */
				PEXBeginRendering(display,window,renderer);
				switch (colour_mode)
				{
					case X3dCOLOUR_INDEX_MODE:
					{
						/*???DB.  Temp while sorting out colour index mode */
						colour.rgb.red=0.0;
						colour.rgb.green=0.0;
						colour.rgb.blue=1.0;
						PEXSetLineColor(display,renderer,PEXOCRender,PEXColorTypeRGB,
							&colour);
					} break;
					case X3dCOLOUR_RGB_MODE:
					{
						colour.rgb.red=1.0;
						colour.rgb.green=1.0;
						colour.rgb.blue=1.0;
						PEXSetLineColor(display,renderer,PEXOCRender,PEXColorTypeRGB,
							&colour);
					} break;
				}
				PEXPolyline(display,renderer,PEXOCRender,4,points);
				PEXEndRendering(display,renderer,True);
			}
			else
			{
				printf("expose_callback.  Missing call_data\n");
			}
#endif
		}
		else
		{
			printf("expose_callback.  No window\n");
		}
	}
	else
	{
		printf("expose_callback.  Missing widget\n");
	}
} /* expose_callback */

static void input_callback(Widget widget,XtPointer user_data,
	XtPointer call_data)
/*******************************************************************************
LAST MODIFIED : 17 August 1998

DESCRIPTION :
==============================================================================*/
{
	XButtonEvent *event;
	X3dThreeDDrawCallbackStruct *callback;

	if (callback=(X3dThreeDDrawCallbackStruct *)call_data)
	{
		if (X3dCR_INPUT==callback->reason)
		{
			if ((callback->event)&&((ButtonPress==callback->event->type)||
				(ButtonRelease==callback->event->type)))
			{
				event= &(callback->event->xbutton);
				if (ButtonPress==callback->event->type)
				{
					printf("button press at %d %d\n",event->x,event->y);
				}
				else
				{
					printf("button release at %d %d\n",event->x,event->y);
				}
			}
			else
			{
				if ((callback->event)&&((KeyPress==callback->event->type)||
					(KeyRelease==callback->event->type)))
				{
					printf("key press/release\n");
				}
				else
				{
					printf("input_callback.  Invalid X event\n");
				}
			}
		}
		else
		{
			printf("input_callback.  Invalid reason\n");
		}
	}
	else
	{
		printf("input_callback.  Missing call_data\n");
	}
} /* input_callback */

main(int argc,char **argv)
/*******************************************************************************
LAST MODIFIED : 4 February 1995

DESCRIPTION :
==============================================================================*/
{
	Display *display;
	Widget form_1,form_2,shell_1,shell_2,three_d_drawing_1,three_d_drawing_2;
	XtAppContext application_context;

	XtToolkitInitialize();
	application_context=XtCreateApplicationContext();
	if (display=XtOpenDisplay(application_context,NULL,"test_3d","Test_3d",NULL,0,
		&argc,argv))
	{
		shell_1=XtVaAppCreateShell("test_3d_1","Test_3d",
			applicationShellWidgetClass,display,
			XtNallowShellResize,True,
			NULL);
		form_1=XtVaCreateWidget("form_1",xmFormWidgetClass,
			shell_1,
			NULL);
		XtManageChild(form_1);
		three_d_drawing_1=XtVaCreateWidget("drawing_1",threeDDrawingWidgetClass,
			form_1,
			XtNx,0,
			XtNy,0,
			XtNwidth,500,
			XtNheight,500,
			XmNbottomAttachment,XmATTACH_FORM,
			XmNleftAttachment,XmATTACH_FORM,
			XmNrightAttachment,XmATTACH_FORM,
			XmNtopAttachment,XmATTACH_FORM,
			X3dNbufferColourMode,X3dCOLOUR_RGB_MODE,
			/*      X3dNbufferingMode,X3dDOUBLE_BUFFERING,*/
			NULL);
		XtAddCallback(three_d_drawing_1,X3dNinitializeCallback,initialize_callback,
			(XtPointer)NULL);
		XtAddCallback(three_d_drawing_1,X3dNexposeCallback,expose_callback,
			(XtPointer)NULL);
/*    XtAddCallback(three_d_drawing_1,X3dNresizeCallback,expose_callback,
			(XtPointer)NULL);*/
		XtAddCallback(three_d_drawing_1,X3dNinputCallback,input_callback,
			(XtPointer)NULL);
		XtManageChild(three_d_drawing_1);
		XtRealizeWidget(shell_1);
		shell_2=XtVaAppCreateShell("test_3d_2","Test_3d",
			topLevelShellWidgetClass,display,
			XtNallowShellResize,True,
			NULL);
		form_2=XtVaCreateWidget("form_2",xmFormWidgetClass,
			shell_2,
			NULL);
		XtManageChild(form_2);
		three_d_drawing_2=XtVaCreateWidget("drawing_2",threeDDrawingWidgetClass,
			form_2,
			XtNx,0,
			XtNy,0,
			XtNwidth,500,
			XtNheight,500,
			XmNbottomAttachment,XmATTACH_FORM,
			XmNleftAttachment,XmATTACH_FORM,
			XmNrightAttachment,XmATTACH_FORM,
			XmNtopAttachment,XmATTACH_FORM,
			NULL);
		XtAddCallback(three_d_drawing_2,X3dNinitializeCallback,initialize_callback,
			(XtPointer)NULL);
		XtAddCallback(three_d_drawing_2,X3dNexposeCallback,expose_callback,
			(XtPointer)NULL);
/*    XtAddCallback(three_d_drawing_2,X3dNresizeCallback,expose_callback,
			(XtPointer)NULL);*/
		XtAddCallback(three_d_drawing_2,X3dNinputCallback,input_callback,
			(XtPointer)NULL);
		XtManageChild(three_d_drawing_2);
		XtRealizeWidget(shell_2);
		XtAppMainLoop(application_context);
	}
	else
	{
		printf("Could not open display\n");
	}
} /* main */
