/**
 * glyph_colour_bar.cpp
 *
 * Derived glyph type which creates a spectrum colour_bar which automatically
 * updates when the spectrum changes.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cmath>
#include "opencmiss/zinc/status.h"
#include "general/debug.h"
#include "general/mystring.h"
#include "graphics/glyph_colour_bar.hpp"
#include "graphics/graphics_vertex_array.hpp"
#include "graphics/spectrum.h"
#include "general/message.h"

/*
Global functions
----------------
*/

/**
Creates a coloured bar with annotation for displaying the scale of <spectrum>.
First makes a graphics object named <name> consisting of a cylinder of size
<bar_length>, <bar_radius> centred at <bar_centre> and aligned with the
<bar_axis>, combining the <bar_material> with the <spectrum>. The bar is
coloured by the current range of the spectrum. An extra <extend_length> is added
on to each end of the bar and given the colour the spectrum returns outside its
range at that end.
The <side_axis> must not be collinear with <bar_axis> and is used to calculate
points around the bar and the direction of the <label_divisions>+1 ticks at which
spectrum values are written.
Attached to the bar graphics_object are two graphics objects using the
<tick_label_material>, one containing the ticks, the other the labels. The
labels are written using the <number_format>.
The side_axis is made to be orthogonal to the bar_axis, and both are made unit
length by this function.
@return  Accessed graphics object or 0 on error.
*/
GT_object *create_Spectrum_colour_bar(
	const char *name,struct cmzn_spectrum *spectrum,int component_number,
	const double barCentre[3], const double barAxis[3], const double sideAxis[3],
	GLfloat extend_length,
	int label_divisions,GLfloat tick_length,const char *number_format,
	cmzn_material *bar_material,
	cmzn_material *tick_label_material,
	struct cmzn_font *font)
{
	char **labels;
	ZnReal cos_theta,extend_fraction,half_final_length,length_factor,
		sin_theta,spectrum_factor,spectrum_minimum,spectrum_maximum,theta,
		unit_factor;
	int allocated_labels,i,j,number_of_ticks,points_along_bar,points_around_bar,
		return_code;
	ZnReal spectrum_value;
	GLfloat *data,*datum;
	struct GT_pointset_vertex_buffers *pointset;
	struct GT_polyline_vertex_buffers *polyline;
	struct GT_surface_vertex_buffers *surface;
	Triple base,centre,front_axis,*point,*points,*normal,*normalpoints,
		scaled_axis,scaled_front,scaled_side;

	GT_object *bar_graphics_object = 0;
	int numberStringSize = getNumericalFormatStringSize(number_format, 1);
	if (name && spectrum && (0 < numberStringSize))
	{
		char *numberString = new char[numberStringSize];
		return_code=1;
		GT_object *tick_graphics_object = 0;
		GT_object *label_graphics_object = 0;
		/* calculate and get range of spectrum */
		Spectrum_calculate_range(spectrum);
		spectrum_minimum=cmzn_spectrum_get_minimum(spectrum);
		spectrum_maximum=cmzn_spectrum_get_maximum(spectrum);
		/* get orthogonal unit vectors along bar_axis, side and front */
		double bar_centre[3] = { barCentre[0], barCentre[1], barCentre[2] };
		double bar_axis[3] = { barAxis[0], barAxis[1], barAxis[2] };
		double side_axis[3] = { sideAxis[0], sideAxis[1], sideAxis[2] };
		double bar_length = sqrt(bar_axis[0]*bar_axis[0]+bar_axis[1]*bar_axis[1]+bar_axis[2]*bar_axis[2]);
		double bar_radius = 0.5*sqrt(side_axis[0]*side_axis[0]+side_axis[1]*side_axis[1]+side_axis[2]*side_axis[2]);
		if (0.0 < bar_length)
		{
			bar_axis[0] /= bar_length;
			bar_axis[1] /= bar_length;
			bar_axis[2] /= bar_length;
			/* get front = unit cross product of bar_axis and side_axis */
			front_axis[0]=bar_axis[1]*side_axis[2] - bar_axis[2]*side_axis[1];
			front_axis[1]=bar_axis[2]*side_axis[0] - bar_axis[0]*side_axis[2];
			front_axis[2]=bar_axis[0]*side_axis[1] - bar_axis[1]*side_axis[0];
			double magnitude = sqrt(front_axis[0]*front_axis[0]+
				front_axis[1]*front_axis[1]+front_axis[2]*front_axis[2]);
			if (0.0 <  magnitude)
			{
				front_axis[0] /= magnitude;
				front_axis[1] /= magnitude;
				front_axis[2] /= magnitude;
				/* now get side_axis = front_axis (x) bar_axis */
				side_axis[0]=front_axis[1]*bar_axis[2] - front_axis[2]*bar_axis[1];
				side_axis[1]=front_axis[2]*bar_axis[0] - front_axis[0]*bar_axis[2];
				side_axis[2]=front_axis[0]*bar_axis[1] - front_axis[1]*bar_axis[0];
			}
			else
			{
				display_message(ERROR_MESSAGE,"create_Spectrum_colour_bar.  "
					"side axis (tick direction) is in-line with bar axis");
				return_code=0;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"create_Spectrum_colour_bar.  Invalid bar axis");
			return_code=0;
		}
		if (return_code)
		{
			bar_graphics_object = CREATE(GT_object)(name,g_SURFACE_VERTEX_BUFFERS,bar_material);
			tick_graphics_object = CREATE(GT_object)("ticks",g_POLYLINE_VERTEX_BUFFERS,tick_label_material);
			label_graphics_object = CREATE(GT_object)("labels",g_POINT_SET_VERTEX_BUFFERS,tick_label_material);
			if (bar_graphics_object && tick_graphics_object && label_graphics_object)
			{
				GT_object_set_next_object(bar_graphics_object, tick_graphics_object);
				GT_object_set_next_object(tick_graphics_object, label_graphics_object);
				return_code=1;
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"create_Spectrum_colour_bar.  Could not create graphics objects");
				return_code=0;
			}
		}
		if (return_code)
		{
			/* create the colour bar */
			points=(Triple *)NULL;
			data=0;
			surface=(struct GT_surface_vertex_buffers *)NULL;
			points_along_bar = 109;
			points_around_bar = 25;
			if (ALLOCATE(points,Triple,points_along_bar*points_around_bar)&&
				ALLOCATE(normalpoints,Triple,points_along_bar*points_around_bar)&&
				ALLOCATE(data,GLfloat,(component_number + 1)
					*points_along_bar*points_around_bar))
			{
				extend_fraction=extend_length/bar_length;
				half_final_length=0.5*bar_length+extend_length;
				scaled_axis[0]=half_final_length*bar_axis[0];
				scaled_axis[1]=half_final_length*bar_axis[1];
				scaled_axis[2]=half_final_length*bar_axis[2];
				point=points;
				normal=normalpoints;
				datum=data + component_number;
				for (i=0;i<points_along_bar;i++)
				{
					/* get factor ranging from 0.0 to 1.0 over extended length */
					unit_factor=(GLfloat)i/(GLfloat)(points_along_bar-1);
					/* get factor ranging from -1 to +1 along bar */
					length_factor=2.0*unit_factor-1.0;
					/* get centre of bar for ith point */
					centre[0]=bar_centre[0] + length_factor*scaled_axis[0];
					centre[1]=bar_centre[1] + length_factor*scaled_axis[1];
					centre[2]=bar_centre[2] + length_factor*scaled_axis[2];
					/* get factor ranging from 0 to 1 over the bar_length and extending
						 outside that range in the extend_length */
					spectrum_factor=unit_factor*(1.0+2.0*extend_fraction)-extend_fraction;
					/* interpolate to get spectrum value */
					spectrum_value = (GLfloat)((1.0-spectrum_factor)*spectrum_minimum +
						spectrum_factor*spectrum_maximum);
					for (j=0;j<points_around_bar;j++)
					{
						theta=(GLfloat) j * 2.0 * PI / (GLfloat)(points_around_bar-1);
						cos_theta=cos(theta);
						sin_theta=sin(theta);
						scaled_side[0]=cos_theta*side_axis[0];
						scaled_side[1]=cos_theta*side_axis[1];
						scaled_side[2]=cos_theta*side_axis[2];
						scaled_front[0]=sin_theta*front_axis[0];
						scaled_front[1]=sin_theta*front_axis[1];
						scaled_front[2]=sin_theta*front_axis[2];
						/* set point */
						(*point)[0]=centre[0]+bar_radius*(scaled_side[0]+scaled_front[0]);
						(*point)[1]=centre[1]+bar_radius*(scaled_side[1]+scaled_front[1]);
						(*point)[2]=centre[2]+bar_radius*(scaled_side[2]+scaled_front[2]);
						point++;
						/* set normal */
						(*normal)[0]=scaled_side[0]+scaled_front[0];
						(*normal)[1]=scaled_side[1]+scaled_front[1];
						(*normal)[2]=scaled_side[2]+scaled_front[2];
						normal++;
						/* set data */
						*datum=spectrum_value;
						datum+=(component_number + 1);
					}
				}
				surface = CREATE(GT_surface_vertex_buffers)(
					g_SHADED,CMZN_GRAPHICS_RENDER_POLYGON_MODE_SHADED);
				return_code = ((fill_surface_graphics_vertex_array(GT_object_get_vertex_set(bar_graphics_object),
						g_TRIANGLE, points_around_bar, points_along_bar,
						points,normalpoints,/*tangentpoints*/(Triple *)NULL,
						 /*texturepoints*/(Triple *)NULL, (component_number + 1), data)) &&
						 (set_GT_object_default_material(bar_graphics_object,bar_material)) &&
						 (set_GT_object_Spectrum(bar_graphics_object,spectrum))&&
						 (GT_OBJECT_ADD(GT_surface_vertex_buffers)(bar_graphics_object, surface)));
				DEALLOCATE(points);
				DEALLOCATE(normalpoints);
				DEALLOCATE(data);
			}
			else
			{
				return_code=0;
			}
			if (!return_code)
			{
				display_message(ERROR_MESSAGE,
					"create_Spectrum_colour_bar.  Could not build spectrum bar");
				if (surface)
				{
					DESTROY(GT_surface_vertex_buffers)(&surface);
				}
			}
		}
		/* get values for calculating tick and label positions */
		number_of_ticks=label_divisions+1;
		scaled_axis[0]=bar_length*bar_axis[0];
		scaled_axis[1]=bar_length*bar_axis[1];
		scaled_axis[2]=bar_length*bar_axis[2];
		base[0]=bar_centre[0]-0.5*scaled_axis[0]+bar_radius*side_axis[0];
		base[1]=bar_centre[1]-0.5*scaled_axis[1]+bar_radius*side_axis[1];
		base[2]=bar_centre[2]-0.5*scaled_axis[2]+bar_radius*side_axis[2];
		scaled_side[0]=tick_length*side_axis[0];
		scaled_side[1]=tick_length*side_axis[1];
		scaled_side[2]=tick_length*side_axis[2];
		if (return_code)
		{
			/* create the scale ticks */
			points=(Triple *)NULL;
			polyline=(struct GT_polyline_vertex_buffers *)NULL;
			if (ALLOCATE(points,Triple,2*number_of_ticks))
			{
				point=points;
				for (i=0;i<number_of_ticks;i++)
				{
					/* get factor ranging from 0.0 to 1.0 over bar_length */
					unit_factor=(GLfloat)i/(GLfloat)(number_of_ticks-1);
					(*point)[0]=base[0]+unit_factor*scaled_axis[0];
					(*point)[1]=base[1]+unit_factor*scaled_axis[1];
					(*point)[2]=base[2]+unit_factor*scaled_axis[2];
					(*(point+1))[0]=(*point)[0]+scaled_side[0];
					(*(point+1))[1]=(*point)[1]+scaled_side[1];
					(*(point+1))[2]=(*point)[2]+scaled_side[2];
					point += 2;
				}
				polyline = CREATE(GT_polyline_vertex_buffers)(g_PLAIN_DISCONTINUOUS, 1);
				return_code=((fill_line_graphics_vertex_array(GT_object_get_vertex_set(tick_graphics_object),
						(unsigned int)(number_of_ticks * 2),points,/*normalpoints*/(Triple *)NULL,
						/*n_data_components*/0,(GLfloat *)NULL))&&
					(set_GT_object_default_material(tick_graphics_object,
						tick_label_material)) &&
					(GT_OBJECT_ADD(GT_polyline_vertex_buffers)(tick_graphics_object,polyline)));
					DEALLOCATE(points);
			}
			else
			{
				return_code=0;
			}
			if (!return_code)
			{
				display_message(ERROR_MESSAGE,
					"create_Spectrum_colour_bar.  Could not build scale ticks");
				if (polyline)
				{
					DESTROY(GT_polyline_vertex_buffers)(&polyline);
				}
			}
		}
		/* labels are drawn at the ends of the ticks */
		base[0] += scaled_side[0];
		base[1] += scaled_side[1];
		base[2] += scaled_side[2];
		if (return_code)
		{
			/* create the scale labels */
			points=(Triple *)NULL;
			labels=(char **)NULL;
			pointset=(struct GT_pointset_vertex_buffers *)NULL;
			allocated_labels=0;
			if (ALLOCATE(points,Triple,number_of_ticks)&&
				ALLOCATE(labels,char *,number_of_ticks))
			{
				point=points;
				for (i=0;return_code&&(i<number_of_ticks);i++)
				{
					/* get factor ranging from 0.0 to 1.0 over bar_length */
					unit_factor=(GLfloat)i/(GLfloat)(number_of_ticks-1);
					(*point)[0]=base[0]+unit_factor*scaled_axis[0];
					(*point)[1]=base[1]+unit_factor*scaled_axis[1];
					(*point)[2]=base[2]+unit_factor*scaled_axis[2];
					point++;
					/* interpolate to get spectrum value */
					spectrum_value=(GLfloat)((1.0-unit_factor)*spectrum_minimum +
						unit_factor*spectrum_maximum);
					snprintf(numberString, numberStringSize-1, number_format, spectrum_value);
					if (ALLOCATE(labels[i],char,static_cast<int>(strlen(numberString)+1)))
					{
						allocated_labels++;
						strcpy(labels[i],numberString);
					}
					else
					{
						return_code=0;
					}
				}
				GT_pointset_vertex_buffers *pointsets = CREATE(GT_pointset_vertex_buffers)(font, g_NO_MARKER, 0.0);
				return_code=((fill_pointset_graphics_vertex_array(GT_object_get_vertex_set(label_graphics_object),
					(unsigned int)number_of_ticks, points, labels, /*n_data_components*/0,(GLfloat *)NULL))&&
					(set_GT_object_default_material(label_graphics_object, tick_label_material)) &&
					(GT_OBJECT_ADD(GT_pointset_vertex_buffers)(label_graphics_object,pointsets)));
				DEALLOCATE(points);
				for (i=0;i<allocated_labels;i++)
				{
					DEALLOCATE(labels[i]);
				}
				DEALLOCATE(labels);
			}
			else
			{
				return_code=0;
			}
			if (!return_code)
			{
				display_message(ERROR_MESSAGE,
					"create_Spectrum_colour_bar.  Could not build scale labels");
				if (pointset)
				{
					DESTROY(GT_pointset_vertex_buffers)(&pointset);
				}
			}
			if (!return_code)
			{
				DEACCESS(GT_object)(&bar_graphics_object);
			}
		}
		if (bar_graphics_object)
		{
			DEACCESS(GT_object)(&tick_graphics_object);
			DEACCESS(GT_object)(&label_graphics_object);
		}
		delete[] numberString;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"create_Spectrum_colour_bar.  Invalid argument(s)");
		return_code=0;
	}
	return (bar_graphics_object);
}

cmzn_glyph_colour_bar::cmzn_glyph_colour_bar(cmzn_spectrum *spectrumIn) :
	spectrum(cmzn_spectrum_access(spectrumIn)),
	graphicsObject(0),
	extendLength(0.05),
	labelMaterial(0),
	numberFormat(duplicate_string("%+.4e")),
	labelDivisions(10),
	tickLength(0.05)
{
	axis[0] = 0.0;
	axis[1] = 1.0;
	axis[2] = 0.0;
	centre[0] = 0.0;
	centre[1] = 0.0;
	centre[2] = 0.0;
	sideAxis[0] = 0.1;
	sideAxis[1] = 0.0;
	sideAxis[2] = 0.0;
}

cmzn_glyph_colour_bar::~cmzn_glyph_colour_bar()
{
	cmzn_spectrum_destroy(&spectrum);
	if (graphicsObject)
	{
		DEACCESS(GT_object)(&graphicsObject);
	}
	if (numberFormat)
	{
		DEALLOCATE(this->numberFormat);
	}
	cmzn_material_destroy(&labelMaterial);
}

GT_object *cmzn_glyph_colour_bar::getGraphicsObject(cmzn_tessellation *tessellation,
	cmzn_material *material, cmzn_font *font)
{
	USE_PARAMETER(tessellation);
	if (this->graphicsObject)
	{
		if ((material != get_GT_object_default_material(this->graphicsObject)) ||
			(font != get_GT_object_font(GT_object_get_next_object(GT_object_get_next_object(this->graphicsObject)))))
		{
			DEACCESS(GT_object)(&this->graphicsObject);
		}
	}
	if (!this->graphicsObject)
	{
		const int componentNumber = 0;
		this->graphicsObject = create_Spectrum_colour_bar("colour_bar", this->spectrum,
			componentNumber, this->centre, this->axis, this->sideAxis,
			this->extendLength, this->labelDivisions, this->tickLength, this->numberFormat,
			/*barMaterial*/material,
			this->labelMaterial ? this->labelMaterial : material,
			font);
	}
	return ACCESS(GT_object)(this->graphicsObject);
}

int cmzn_glyph_colour_bar::getAxis(int valuesCount, double *valuesOut) const
{
	if ((0 < valuesCount) && valuesOut)
	{
		const int count = (valuesCount > 3) ? 3 : valuesCount;
		for (int i = 0; i < count; ++i)
		{
			valuesOut[i] = this->axis[i];
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_glyph_colour_bar::setAxis(int valuesCount, const double *valuesIn)
{
	if ((0 < valuesCount) && valuesIn)
	{
		bool changed = false;
		for (int i = 0; i < 3; ++i)
		{
			double value = (i < valuesCount) ? valuesIn[i] : 0.0;
			if (this->axis[i] != value)
			{
				this->axis[i] = value;
				changed = true;
			}
		}
		if (changed)
		{
			this->invalidate();
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_glyph_colour_bar::getCentre(int valuesCount, double *valuesOut) const
{
	if ((0 < valuesCount) && valuesOut)
	{
		const int count = (valuesCount > 3) ? 3 : valuesCount;
		for (int i = 0; i < count; ++i)
		{
			valuesOut[i] = this->centre[i];
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_glyph_colour_bar::setCentre(int valuesCount, const double *valuesIn)
{
	if ((0 < valuesCount) && valuesIn)
	{
		bool changed = false;
		for (int i = 0; i < 3; ++i)
		{
			double value = (i < valuesCount) ? valuesIn[i] : 0.0;
			if (this->centre[i] != value)
			{
				this->centre[i] = value;
				changed = true;
			}
		}
		if (changed)
		{
			this->invalidate();
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_glyph_colour_bar::setExtendLength(double extendLengthIn)
{
	if (extendLengthIn >= 0.0)
	{
		if (extendLengthIn != this->extendLength)
		{
			this->extendLength = extendLengthIn;
			this->invalidate();
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_glyph_colour_bar::setLabelDivisions(int labelDivisionsIn)
{
	if (labelDivisionsIn > 0)
	{
		if (labelDivisionsIn != this->labelDivisions)
		{
			this->labelDivisions = labelDivisionsIn;
			this->invalidate();
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_glyph_colour_bar::setLabelMaterial(cmzn_material_id material)
{
	if (material != this->labelMaterial)
	{
		REACCESS(cmzn_material)(&(this->labelMaterial), material);
		this->invalidate();
	}
	return CMZN_OK;
}

char *cmzn_glyph_colour_bar::getNumberFormat() const
{
	return duplicate_string(this->numberFormat);
}

int cmzn_glyph_colour_bar::setNumberFormat(const char *numberFormatIn)
{
	int size = getNumericalFormatStringSize(numberFormatIn, 1);
	if ((size > 0) && (size < 500))
	{
		if (0 != strcmp(numberFormatIn, this->numberFormat))
		{
			DEALLOCATE(this->numberFormat);
			this->numberFormat = duplicate_string(numberFormatIn);
			this->invalidate();
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_glyph_colour_bar::getSideAxis(int valuesCount, double *valuesOut) const
{
	if ((0 < valuesCount) && valuesOut)
	{
		const int count = (valuesCount > 3) ? 3 : valuesCount;
		for (int i = 0; i < count; ++i)
		{
			valuesOut[i] = this->sideAxis[i];
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_glyph_colour_bar::setSideAxis(int valuesCount, const double *valuesIn)
{
	if ((0 < valuesCount) && valuesIn)
	{
		bool changed = false;
		for (int i = 0; i < 3; ++i)
		{
			double value = (i < valuesCount) ? valuesIn[i] : 0.0;
			if (this->sideAxis[i] != value)
			{
				this->sideAxis[i] = value;
				changed = true;
			}
		}
		if (changed)
		{
			this->invalidate();
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_glyph_colour_bar::setTickLength(double tickLengthIn)
{
	if (tickLengthIn >= 0.0)
	{
		if (tickLengthIn != this->tickLength)
		{
			this->tickLength = tickLengthIn;
			this->invalidate();
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

void cmzn_glyph_colour_bar::fontChange()
{
	this->invalidate();
}

void cmzn_glyph_colour_bar::materialChange(struct MANAGER_MESSAGE(cmzn_material) *message)
{
	if (this->labelMaterial)
	{
		int change_flags = MANAGER_MESSAGE_GET_OBJECT_CHANGE(cmzn_material)(message, this->labelMaterial);
		if (change_flags & MANAGER_CHANGE_RESULT(cmzn_material))
		{
			this->invalidate();
		}
	}
}

void cmzn_glyph_colour_bar::spectrumChange(struct MANAGER_MESSAGE(cmzn_spectrum) *message)
{
	int change_flags = MANAGER_MESSAGE_GET_OBJECT_CHANGE(cmzn_spectrum)(message, this->spectrum);
	if (change_flags & MANAGER_CHANGE_RESULT(cmzn_spectrum))
	{
		this->invalidate();
	}
}

cmzn_glyph_id cmzn_glyphmodule_create_glyph_colour_bar(
	cmzn_glyphmodule_id glyphmodule, cmzn_spectrum_id spectrum)
{
	if (glyphmodule)
	{
		cmzn_glyph_colour_bar_id colour_bar = cmzn_glyph_colour_bar::create(spectrum);
		if (colour_bar)
		{
			glyphmodule->addGlyph(colour_bar);
			return colour_bar;
		}
	}
	return 0;
}

cmzn_glyph_colour_bar_id cmzn_glyph_cast_colour_bar(cmzn_glyph_id glyph)
{
	if (glyph && (dynamic_cast<cmzn_glyph_colour_bar*>(glyph)))
	{
		glyph->access();
		return (reinterpret_cast<cmzn_glyph_colour_bar_id>(glyph));
	}
	return 0;
}

int cmzn_glyph_colour_bar_destroy(cmzn_glyph_colour_bar_id *colour_bar_address)
{
	return cmzn_glyph_destroy(reinterpret_cast<cmzn_glyph_id *>(colour_bar_address));
}

int cmzn_glyph_colour_bar_get_axis(
	cmzn_glyph_colour_bar_id colour_bar, int valuesCount, double *valuesOut)
{
	if (colour_bar)
		return colour_bar->getAxis(valuesCount, valuesOut);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_glyph_colour_bar_set_axis(
	cmzn_glyph_colour_bar_id colour_bar, int valuesCount, const double *valuesIn)
{
	if (colour_bar)
		return colour_bar->setAxis(valuesCount, valuesIn);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_glyph_colour_bar_get_centre(
	cmzn_glyph_colour_bar_id colour_bar, int valuesCount, double *valuesOut)
{
	if (colour_bar)
		return colour_bar->getCentre(valuesCount, valuesOut);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_glyph_colour_bar_set_centre(
	cmzn_glyph_colour_bar_id colour_bar, int valuesCount, const double *valuesIn)
{
	if (colour_bar)
		return colour_bar->setCentre(valuesCount, valuesIn);
	return CMZN_ERROR_ARGUMENT;
}

double cmzn_glyph_colour_bar_get_extend_length(
	cmzn_glyph_colour_bar_id colour_bar)
{
	if (colour_bar)
		return colour_bar->getExtendLength();
	return 0.0;
}

int cmzn_glyph_colour_bar_set_extend_length(
	cmzn_glyph_colour_bar_id colour_bar, double extendLength)
{
	if (colour_bar)
		return colour_bar->setExtendLength(extendLength);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_glyph_colour_bar_get_label_divisions(
	cmzn_glyph_colour_bar_id colour_bar)
{
	if (colour_bar)
		return colour_bar->getLabelDivisions();
	return 0;
}

int cmzn_glyph_colour_bar_set_label_divisions(
	cmzn_glyph_colour_bar_id colour_bar, int labelDivisions)
{
	if (colour_bar)
		return colour_bar->setLabelDivisions(labelDivisions);
	return CMZN_ERROR_ARGUMENT;
}

cmzn_material_id cmzn_glyph_colour_bar_get_label_material(
	cmzn_glyph_colour_bar_id colour_bar)
{
	if (colour_bar)
		return colour_bar->getLabelMaterial();
	return 0;
}

int cmzn_glyph_colour_bar_set_label_material(
	cmzn_glyph_colour_bar_id colour_bar, cmzn_material_id material)
{
	if (colour_bar)
		return colour_bar->setLabelMaterial(material);
	return CMZN_ERROR_ARGUMENT;
}

char *cmzn_glyph_colour_bar_get_number_format(
	cmzn_glyph_colour_bar_id colour_bar)
{
	if (colour_bar)
		return colour_bar->getNumberFormat();
	return 0;
}

int cmzn_glyph_colour_bar_set_number_format(
	cmzn_glyph_colour_bar_id colour_bar, const char *numberFormat)
{
	if (colour_bar)
		return colour_bar->setNumberFormat(numberFormat);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_glyph_colour_bar_get_side_axis(
	cmzn_glyph_colour_bar_id colour_bar, int valuesCount, double *valuesOut)
{
	if (colour_bar)
		return colour_bar->getSideAxis(valuesCount, valuesOut);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_glyph_colour_bar_set_side_axis(
	cmzn_glyph_colour_bar_id colour_bar, int valuesCount, const double *valuesIn)
{
	if (colour_bar)
		return colour_bar->setSideAxis(valuesCount, valuesIn);
	return CMZN_ERROR_ARGUMENT;
}

cmzn_spectrum_id cmzn_glyph_colour_bar_get_spectrum(
	cmzn_glyph_colour_bar_id colour_bar)
{
	if (colour_bar)
		return cmzn_spectrum_access(colour_bar->getSpectrum());
	return 0;
}

double cmzn_glyph_colour_bar_get_tick_length(
	cmzn_glyph_colour_bar_id colour_bar)
{
	if (colour_bar)
		return colour_bar->getTickLength();
	return 0.0;
}

ZINC_API int cmzn_glyph_colour_bar_set_tick_length(
	cmzn_glyph_colour_bar_id colour_bar, double tickLength)
{
	if (colour_bar)
		return colour_bar->setTickLength(tickLength);
	return CMZN_ERROR_ARGUMENT;
}
