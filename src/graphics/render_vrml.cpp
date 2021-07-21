/*******************************************************************************
FILE : render_vrml.cpp

LAST MODIFIED : 20 March 2003

DESCRIPTION :
Renders gtObjects to VRML file
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "opencmiss/zinc/material.h"
#include "general/debug.h"
#include "general/list.h"
#include "general/list_private.h"
#include "general/object.h"
#include "general/mystring.h"
#include "graphics/glyph.hpp"
#include "graphics/graphics_object.h"
#include "graphics/material.h"
#include "graphics/render_vrml.h"
#include "graphics/spectrum.h"
#include "graphics/texture.h"
#include "region/cmiss_region.hpp"
#include "general/message.h"
#include "graphics/scene.hpp"
#include "graphics/graphics_object_private.hpp"

/*
Module types
------------
*/

struct VRML_prototype
/*******************************************************************************
LAST MODIFIED : 9 May 1999

DESCRIPTION :
Used to keep track of what objects have been defined.
PROTOtyping is very slow so has been phased out.
==============================================================================*/
{
	char *name;
	struct Texture *texture;
	cmzn_material *material;
	struct GT_object *graphics_object;
	/* time only needed for graphics_object prototype */
	ZnReal time;
	int access_count;
}; /* struct VRML_prototype */

DECLARE_LIST_TYPES(VRML_prototype);
FULL_DECLARE_LIST_TYPE(VRML_prototype);

/*
Module functions
----------------
*/

static double clamped_acos(double x)
/*******************************************************************************
LAST MODIFIED : 9 May 1999

DESCRIPTION :
Protects us from errors due to numbers being close but outside the range of
-1 to 1.
==============================================================================*/
{
	double result, tolerance;

	ENTER(clamped_acos);

	tolerance = 1e-3;
	if (x > 1.0)
	{
		if (x > 1.0 + tolerance)
		{
			display_message(WARNING_MESSAGE,
				"clamped_acos.  Value passed to acos is greater than 1.0 plus tolerance.");
		}
		result = 0.0;
	}
	else if (x < -1.0)
	{
		if (x < -1.0 - tolerance)
		{
			display_message(WARNING_MESSAGE,
				"clamped_acos.  Value passed to acos is less than -1.0 minus tolerance.");
		}
		result = PI;
	}
	else
	{
		result = acos(x);
	}

	LEAVE;

	return (result);
} /* clamped_acos */

static struct VRML_prototype *CREATE(VRML_prototype)(char *name,
	struct Texture *texture,
	cmzn_material *material,
	struct GT_object *graphics_object,ZnReal time)
/*******************************************************************************
LAST MODIFIED : 9 May 1999

DESCRIPTION :
Creates a VRML_prototype for the supplied object - only one of texture/material/
graphics_object may be set. The name must be unique for any object in the file.
Used for ensuring PROTOtypes for VRML objects are written in the right order
such that they exist at the time they are to be used.
==============================================================================*/
{
	struct VRML_prototype *vrml_prototype;

	ENTER(CREATE(VRML_prototype));
	if (name)
	{
		if (ALLOCATE(vrml_prototype,struct VRML_prototype,1)&&
			ALLOCATE(vrml_prototype->name,char,strlen(name)+1))
		{
			sprintf(vrml_prototype->name,"%s",name);
			/* do not ACCESS objects as vrml_prototypes are temporary structures */
			vrml_prototype->texture=texture;
			vrml_prototype->material=material;
			vrml_prototype->graphics_object=graphics_object;
			vrml_prototype->time=time;
			vrml_prototype->access_count=0;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"CREATE(VRML_prototype).  Not enough memory");
			DEALLOCATE(vrml_prototype);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CREATE(VRML_prototype).  Invalid argument(s)");
		vrml_prototype=(struct VRML_prototype *)NULL;
	}
	LEAVE;

	return (vrml_prototype);
} /* CREATE(VRML_prototype) */

static int DESTROY(VRML_prototype)(
	struct VRML_prototype **vrml_prototype_address)
/*******************************************************************************
LAST MODIFIED : 9 May 1999

DESCRIPTION :
Frees the memory for <**vrml_prototype_address> and sets
<*vrml_prototype_address> to NULL.
==============================================================================*/
{
	int return_code;

	ENTER(DESTROY(VRML_prototype));
	if (vrml_prototype_address&&(*vrml_prototype_address))
	{
		if (0==(*vrml_prototype_address)->access_count)
		{
			DEALLOCATE((*vrml_prototype_address)->name);
			DEALLOCATE(*vrml_prototype_address);
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,"DESTROY(VRML_prototype).  "
				"Non-zero access count of %d",(*vrml_prototype_address)->access_count);
			*vrml_prototype_address=(struct VRML_prototype *)NULL;
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(VRML_prototype).  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(VRML_prototype) */

DECLARE_OBJECT_FUNCTIONS(VRML_prototype)
DECLARE_LIST_FUNCTIONS(VRML_prototype)

static int VRML_prototype_is_identical(struct VRML_prototype *vrml_prototype,
	void *prototype_void)
/*******************************************************************************
LAST MODIFIED : 9 June 1999

DESCRIPTION :
Conditional function returning true if <vrml_prototype> refers to <material>.
Used with FIRST_OBJECT_IN_LIST_THAT to find if a request to prototype this
material is already in the list.
==============================================================================*/
{
	int return_code;
	struct VRML_prototype *prototype;

	ENTER(VRML_prototype_is_identical);

	if (vrml_prototype&&(prototype=(struct VRML_prototype *)prototype_void))
	{
		return_code=((vrml_prototype->material == prototype->material)
			&&(vrml_prototype->graphics_object == prototype->graphics_object)
			&&(vrml_prototype->texture == prototype->texture));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"VRML_prototype_is_identical.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* VRML_prototype_is_identical */

/* Forward declaration of write_graphics_object_vrml function for glyph_set */
static int write_graphics_object_vrml(FILE *vrml_file,
	struct GT_object *gt_object,ZnReal time,
	struct LIST(VRML_prototype) *vrml_prototype_list,
	int object_is_glyph,cmzn_material *default_material,
	int gt_object_already_prototyped);

static int write_texture_vrml(FILE *file,struct Texture *texture)
/*******************************************************************************
LAST MODIFIED : 11 February 2002

DESCRIPTION :
Writes VRML that defines the texture.
==============================================================================*/
{
	char *texture_name;
	ZnReal width, height, depth;
	int width_texels, height_texels, depth_texels, return_code;

	ENTER(write_texture_vrml);
	if (texture && file)
	{
		Texture_get_size(texture, &width_texels, &height_texels, &depth_texels);
		if (1 != depth_texels)
		{
			GET_NAME(Texture)(texture, &texture_name);
			display_message(WARNING_MESSAGE,
				"write_texture_vrml.  VRML97 file cannot use 3-D texture %s; "
				"file will refer to nonexistent 3-D image series template %s",
				texture_name, Texture_get_image_file_name(texture));
			DEALLOCATE(texture_name);
		}
		Texture_get_physical_size(texture, &width, &height, &depth);
		fprintf(file,"texture  ImageTexture\n{\n");
		fprintf(file,"  url \"%s\"\n",Texture_get_image_file_name(texture));
		fprintf(file,"} #ImageTexture\n");
		fprintf(file,"textureTransform  TextureTransform\n{\n");
		fprintf(file,"  translation 0 0\n" );
		fprintf(file,"  rotation 0\n" );
		fprintf(file,"  scale %f %f\n", 1./width , 1./height );
		fprintf(file,"  center	0.0 0.0\n" );
		fprintf(file,"} #TextureTransform\n");
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"write_texture_vrml.  Missing texture or FILE handle");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* write_texture_vrml */

static int write_material_node_vrml(FILE *vrml_file,
	cmzn_material *material,int emissive_only)
/*******************************************************************************
LAST MODIFIED : 5 May 1999

DESCRIPTION :
Writes a VRML Material node to <file> for the <material>. If <emissive_only> is
set, only the emissive material is output. This is used for lines and points
with no lighting/normals.
==============================================================================*/
{
	int return_code;
	MATERIAL_PRECISION alpha,shininess;
	struct Colour diffuse,ambient,specular,emissive;

	ENTER(write_material_node_vrml);
	if (vrml_file&&material)
	{
		fprintf(vrml_file,"Material {\n");
		if (emissive_only)
		{
			/* use the diffuse colour for emissive */
			if (Graphical_material_get_diffuse(material,&emissive))
			{
				fprintf(vrml_file,"  emissiveColor %f %f %f\n",
					emissive.red,emissive.green,emissive.blue);
			}
		}
		else
		{
			if (Graphical_material_get_diffuse(material,&diffuse))
			{
				fprintf(vrml_file,"  diffuseColor %f %f %f\n",
					diffuse.red,diffuse.green,diffuse.blue);
			}
			if (Graphical_material_get_ambient(material,&ambient))
			{
				fprintf(vrml_file,"  ambientIntensity %f\n",
					(ambient.red+ambient.green+ambient.blue)/3.0);
			}
			if (Graphical_material_get_emission(material,&emissive))
			{
				fprintf(vrml_file,"  emissiveColor %f %f %f\n",
					emissive.red,emissive.green,emissive.blue);
			}
			if (Graphical_material_get_specular(material,&specular))
			{
				fprintf(vrml_file,"  specularColor %f %f %f\n",
					specular.red,specular.green,specular.blue);
			}
			if (Graphical_material_get_alpha(material,&alpha))
			{
				fprintf(vrml_file,"  transparency %f\n",1.0-alpha);
			}
			if (Graphical_material_get_shininess(material,&shininess))
			{
				fprintf(vrml_file,"  shininess %f\n",shininess);
			}
		}
		fprintf(vrml_file,"} #Material\n");
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"write_material_node_vrml.  Missing material or FILE handle");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* write_material_node_vrml */

static int activate_material_vrml(FILE *vrml_file,
	cmzn_material *material,
	struct LIST(VRML_prototype) *vrml_prototype_list,
	int no_define,int emissive_only)
/*******************************************************************************
LAST MODIFIED : 9 May 1999

DESCRIPTION :
==============================================================================*/
{
	char *dot_pointer, *material_name;
	int return_code = 0;
	struct VRML_prototype *vrml_prototype;

	ENTER(activate_material_vrml);
	if (vrml_file&&material)
	{
		if (emissive_only || no_define || (!vrml_prototype_list))
		{
			return_code=write_material_node_vrml(vrml_file,material,emissive_only);
		}
		else
		{
			if (GET_NAME(cmzn_material)(material,&material_name))
			{
				/* Can't have . in a name */
				while (NULL != (dot_pointer = strchr(material_name, '.')))
				{
					*dot_pointer = '_';
				}
				vrml_prototype=CREATE(VRML_prototype)(material_name,(struct Texture *)NULL,
					material,(struct GT_object *)NULL,/*time*/0);
				if (FIRST_OBJECT_IN_LIST_THAT(VRML_prototype)(VRML_prototype_is_identical,
					(void *)vrml_prototype,vrml_prototype_list))
				{
					fprintf(vrml_file,"USE %s\n",material_name);
					DESTROY(VRML_prototype)(&vrml_prototype);
					return_code = 1;
				}
				else
				{
					fprintf(vrml_file,"DEF %s ",material_name);
					return_code=write_material_node_vrml(vrml_file,material,emissive_only);
					ADD_OBJECT_TO_LIST(VRML_prototype)(vrml_prototype,vrml_prototype_list);
					fprintf(vrml_file,"#END DEF %s\n",material_name);
				}
				DEALLOCATE(material_name);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"activate_material_vrml.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* activate_material_vrml */

static int spectrum_start_render_vrml(FILE *vrml_file,struct cmzn_spectrum *spectrum,
	cmzn_material *material)
/*******************************************************************************
LAST MODIFIED : 6 May 1999

DESCRIPTION :
Sets VRML file for rendering values on the current material.
==============================================================================*/
{
	int return_code;

	ENTER(spectrum_start_render_vrml);
	if (spectrum && material)
	{
		fprintf(vrml_file,"color Color{\n");
		fprintf(vrml_file,"  color [\n");
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"spectrum_start_render_vrml.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* spectrum_start_render_vrml */

static int spectrum_render_vrml_value(FILE *vrml_file,struct cmzn_spectrum *spectrum,
	cmzn_material *material,int number_of_data_components,GLfloat *data)
/*******************************************************************************
LAST MODIFIED : 6 May 1999

DESCRIPTION :
Writes VRML to represent the value 'data' in accordance with the spectrum.
==============================================================================*/
{
	int return_code;
	ZnReal rgba[4];

	ENTER(spectrum_render_vrml_value);
	if (spectrum&&material)
	{
		FE_value *feData = new FE_value[number_of_data_components];
		CAST_TO_FE_VALUE(feData,data,number_of_data_components);
		Spectrum_value_to_rgba(spectrum,number_of_data_components,feData,rgba);
		fprintf(vrml_file,"    %f %f %f,\n",rgba[0],rgba[1],rgba[2]);
		return_code=1;
		delete[] feData;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"spectrum_render_vrml_value.  Invalid arguments given.");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* spectrum_render_vrml_value */

static int spectrum_end_render_vrml(FILE *vrml_file,struct cmzn_spectrum *spectrum)
/*******************************************************************************
LAST MODIFIED : 6 May 1999

DESCRIPTION :
Resets the graphics state after rendering values on current material.
==============================================================================*/
{
	int return_code;

	ENTER(spectrum_end_render_vrml);
	if (spectrum)
	{
		Spectrum_end_value_to_rgba(spectrum);
		fprintf(vrml_file,"  ]\n");
		fprintf(vrml_file,"} #Color\n");
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"spectrum_end_render_vrml.  Invalid spectrum object");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* spectrum_end_render_vrml */

static int get_orthogonal_axes(ZnReal a1,ZnReal a2,ZnReal a3,
	ZnReal *b1,ZnReal *b2,ZnReal *b3,ZnReal *c1,ZnReal *c2,ZnReal *c3)
/*******************************************************************************
LAST MODIFIED : 11 May 1999

DESCRIPTION :
Given unit vector axis a=(a1,a2,a3), returns a pair of unit vectors
b=(b1,b2,b3) and c=(c1,c2,c3) such that a = b (x) c.
==============================================================================*/
{
	ZnReal length;
	int return_code;

	ENTER(get_orthogonal_axes);
	if (b1&&b2&&b3&&c1&&c2&&c3&&(0.00001>fabs(sqrt(a1*a1+a2*a2+a3*a3)-1.0)))
	{
		*b1=0.0;
		*b2=0.0;
		*b3=0.0;
		/* make b have 1.0 in the component with the least absolute value in a */
		if (fabs(a1) < fabs(a2))
		{
			if (fabs(a3) < fabs(a1))
			{
				*b3=1.0;
			}
			else
			{
				*b1=1.0;
			}
		}
		else
		{
			if (fabs(a3) < fabs(a2))
			{
				*b3=1.0;
			}
			else
			{
				*b2=1.0;
			}
		}
		/* set c = a (x) b */
		(*c1) = a2*(*b3) - a3*(*b2);
		(*c2) = a3*(*b1) - a1*(*b3);
		(*c3) = a1*(*b2) - a2*(*b1);
		/* normalize c */
		length = sqrt((*c1)*(*c1) + (*c2)*(*c2) + (*c3)*(*c3));
		(*c1) /= length;
		(*c2) /= length;
		(*c3) /= length;
		/* set b = c (x) a */
		*b1 = (*c2)*a3-(*c3)*a2;
		*b2 = (*c3)*a1-(*c1)*a3;
		*b3 = (*c1)*a2-(*c2)*a1;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"get_orthogonal_axes.  Invalid arguments");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* get_orthogonal_axes */

/**
 * Defines an object for the <glyph> and then draws that at <number_of_points>
 * points  given by the positions in <point_list> and oriented and scaled by
 * <axis1_list>, <axis2_list> and <axis3_list>.
 */
static int draw_glyph_set_vrml(FILE *vrml_file, GT_glyphset_vertex_buffers *glyph_set,
	Graphics_vertex_array *vertex_array,
	cmzn_material *material, struct cmzn_spectrum *spectrum, ZnReal time,
	struct LIST(VRML_prototype) *vrml_prototype_list)
{
	int return_code = 1;
	if (glyph_set && vertex_array)
	{
		int group = 0;
		unsigned int nodeset_index;
		unsigned int nodeset_count =
			vertex_array->get_number_of_vertices(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START);
		if (0 == nodeset_count)
			return 1;
		unsigned int position_values_per_vertex = 0, position_vertex_count = 0,
			data_values_per_vertex = 0, data_vertex_count = 0, axis1_values_per_vertex = 0,
			axis1_vertex_count = 0, axis2_values_per_vertex = 0, axis2_vertex_count = 0,
			axis3_values_per_vertex = 0, axis3_vertex_count = 0, scale_values_per_vertex = 0,
			scale_vertex_count = 0, label_per_vertex, label_count = 0;
		std::string *label_buffer = 0;
		GLfloat *position_buffer = 0, *data_buffer = 0, *axis1_buffer = 0,
			*axis2_buffer = 0, *axis3_buffer = 0, *scale_buffer = 0;
		vertex_array->get_float_vertex_buffer(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
			&position_buffer, &position_values_per_vertex, &position_vertex_count);
		vertex_array->get_float_vertex_buffer(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
			&data_buffer, &data_values_per_vertex, &data_vertex_count);
		vertex_array->get_float_vertex_buffer(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_AXIS1,
			&axis1_buffer, &axis1_values_per_vertex, &axis1_vertex_count);
		vertex_array->get_float_vertex_buffer(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_AXIS2,
			&axis2_buffer, &axis2_values_per_vertex, &axis2_vertex_count);
		vertex_array->get_float_vertex_buffer(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_AXIS3,
			&axis3_buffer, &axis3_values_per_vertex, &axis3_vertex_count);
		vertex_array->get_float_vertex_buffer(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_SCALE,
			&scale_buffer, &scale_values_per_vertex, &scale_vertex_count);
		vertex_array->get_string_vertex_buffer(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_LABEL,
			&label_buffer, &label_per_vertex, &label_count);
		const bool data_spectrum = (0 < data_values_per_vertex) && (0 != data_buffer) &&
			(0 != spectrum) && (0 != material);
		if (data_buffer && (!spectrum))
		{
			display_message(WARNING_MESSAGE,"draw_glyph_set_vrml.  Missing spectrum");
		}
		if (nodeset_count > 1)
		{
			fprintf(vrml_file,"Group {\n");
			fprintf(vrml_file,"  children [\n");
			group = 1;
		}
		for (nodeset_index = 0; nodeset_index < nodeset_count; nodeset_index++)
		{
			unsigned int index_start = 0, index_count = 0;
			vertex_array->get_unsigned_integer_attribute(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
				nodeset_index, 1, &index_start);
			vertex_array->get_unsigned_integer_attribute(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
				nodeset_index, 1, &index_count);
			GLfloat *position = position_buffer + position_values_per_vertex * index_start,
				*axis1 = axis1_buffer + axis1_values_per_vertex * index_start,
				*axis2 = axis2_buffer + axis2_values_per_vertex * index_start,
				*axis3 = axis3_buffer + axis3_values_per_vertex * index_start,
				*scale = scale_buffer + scale_values_per_vertex * index_start,
				*datum = data_buffer + data_values_per_vertex * index_start;
			std::string *labels = label_buffer + label_per_vertex * index_start;
			std::string *label = labels;
			Triple temp_axis1, temp_axis2, temp_axis3, temp_point;
			temp_point[0] = 0.0;
			temp_point[1] = 0.0;
			temp_point[2] = 0.0;
			ZnReal a1, a2, a3, a_angle, a_magnitude, ax1, ax2, ax3, b1, b2, b3, b_angle,
			bx1, bx2, bx3, c1, c2, c3, cx1, cx2, cx3, dp, j1, j2, j3,
			c_magnitude, s1, s2, s3, x = 0.0, y = 0.0, z = 0.0;
			int j, number_of_skew_glyph_axes, skewed_axes;
			unsigned int i;
			cmzn_material *material_copy;
			GT_object *glyph = glyph_set->glyph;
			cmzn_glyph_repeat_mode glyph_repeat_mode = glyph_set->glyph_repeat_mode;
			/* try to draw points and lines faster */
			cmzn_glyph_shape_type glyph_type = glyph ?
				GT_object_get_glyph_type(glyph) : CMZN_GLYPH_SHAPE_TYPE_NONE;
			if ((glyph_type == CMZN_GLYPH_SHAPE_TYPE_POINT) && (
				(glyph_repeat_mode == CMZN_GLYPH_REPEAT_MODE_NONE) ||
				(glyph_repeat_mode == CMZN_GLYPH_REPEAT_MODE_MIRROR)))
			{
				fprintf(vrml_file,"Shape {\n");
				fprintf(vrml_file,"  appearance\n");
				if (material)
				{
					fprintf(vrml_file,"Appearance {\n");
					fprintf(vrml_file,"  material\n");
					activate_material_vrml(vrml_file,material,
						(struct LIST(VRML_prototype) *)NULL,
						/*no_define_material*/0,/*emissive_only*/1);
					fprintf(vrml_file,"} #Appearance\n");
				}
				else
				{
					fprintf(vrml_file,"IS line_appearance\n");
				}
				fprintf(vrml_file,"  geometry PointSet {\n");
				fprintf(vrml_file,"    coord Coordinate {\n");
				fprintf(vrml_file,"      point [\n");
				for (i=0;i<index_count;i++)
				{
					fprintf(vrml_file,"        %f %f %f,\n",
						position[0], position[1],position[2]);
					position += position_values_per_vertex;
				}
				fprintf(vrml_file,"      ]\n");
				fprintf(vrml_file,"    }\n");
				if (data_spectrum)
				{
					spectrum_start_render_vrml(vrml_file,spectrum,material);
					for (i=0;i<index_count;i++)
					{
						spectrum_render_vrml_value(vrml_file,spectrum,material,
							data_values_per_vertex,datum+i*data_values_per_vertex);
					}
					spectrum_end_render_vrml(vrml_file, spectrum);
				}
				fprintf(vrml_file,"  } #Pointset\n");
				fprintf(vrml_file,"} #Shape\n");
			}
			else if ((glyph_type == CMZN_GLYPH_SHAPE_TYPE_LINE) &&
				(glyph_repeat_mode == CMZN_GLYPH_REPEAT_MODE_NONE))
			{
				fprintf(vrml_file,"Shape {\n");
				fprintf(vrml_file,"  appearance\n");
				if (material)
				{
					fprintf(vrml_file,"Appearance {\n");
					fprintf(vrml_file,"  material\n");
					activate_material_vrml(vrml_file,material,
						(struct LIST(VRML_prototype) *)NULL,
						/*no_define_material*/0,/*emissive_only*/1);
					fprintf(vrml_file,"} #Appearance\n");
				}
				else
				{
					fprintf(vrml_file,"IS line_appearance\n");
				}
				fprintf(vrml_file,"  geometry IndexedLineSet {\n");
				fprintf(vrml_file,"    coord Coordinate {\n");
				fprintf(vrml_file,"      point [\n");
				for (i=0;i<index_count;i++)
				{
					resolve_glyph_axes(glyph_repeat_mode, /*glyph_number*/0,
						glyph_set->base_size, glyph_set->scale_factors, glyph_set->offset,
						position, axis1, axis2, axis3, scale,
						temp_point, temp_axis1, temp_axis2, temp_axis3);
					x = temp_point[0];
					y = temp_point[1];
					z = temp_point[2];
					fprintf(vrml_file,"        %f %f %f,\n",x,y,z);
					x = temp_point[0] + temp_axis1[0];
					y = temp_point[1] + temp_axis1[1];
					z = temp_point[2] + temp_axis1[2];
					fprintf(vrml_file,"        %f %f %f,\n",x,y,z);
					position += position_values_per_vertex;
					axis1 += axis1_values_per_vertex;
					axis2 += axis2_values_per_vertex;
					axis3 += axis3_values_per_vertex;
					scale += scale_values_per_vertex;
				}
				fprintf(vrml_file,"      ]\n");
				fprintf(vrml_file,"    }\n");
				if (data_spectrum)
				{
					fprintf(vrml_file,"    colorPerVertex FALSE\n");
					spectrum_start_render_vrml(vrml_file,spectrum,material);
					for (i=0;i<index_count;i++)
					{
						spectrum_render_vrml_value(vrml_file,spectrum,material,
							data_values_per_vertex,datum+i*data_values_per_vertex);
					}
					spectrum_end_render_vrml(vrml_file, spectrum);
				}
				fprintf(vrml_file,"    coordIndex [\n");
				for (i=0;i<index_count;i++)
				{
					fprintf(vrml_file,"      %d,%d,-1,\n",2*i,2*i+1);
				}
				fprintf(vrml_file,"    ]\n");
				fprintf(vrml_file,"  } #IndexedLineSet\n");
				fprintf(vrml_file,"} #Shape\n");
			}
			else if ((glyph_type == CMZN_GLYPH_SHAPE_TYPE_CROSS) &&
				(glyph_repeat_mode == CMZN_GLYPH_REPEAT_MODE_NONE))
			{
				fprintf(vrml_file,"Shape {\n");
				fprintf(vrml_file,"  appearance\n");
				if (material)
				{
					fprintf(vrml_file,"Appearance {\n");
					fprintf(vrml_file,"  material\n");
					activate_material_vrml(vrml_file,material,
						(struct LIST(VRML_prototype) *)NULL,
						/*no_define_material*/0,/*emissive_only*/1);
					fprintf(vrml_file,"} #Appearance\n");
				}
				else
				{
					fprintf(vrml_file,"IS line_appearance\n");
				}
				fprintf(vrml_file,"  geometry IndexedLineSet {\n");
				fprintf(vrml_file,"    coord Coordinate {\n");
				fprintf(vrml_file,"      point [\n");
				for (i=0;i<index_count;i++)
				{
					resolve_glyph_axes(glyph_repeat_mode, /*glyph_number*/0,
						glyph_set->base_size, glyph_set->scale_factors, glyph_set->offset,
						position, axis1, axis2, axis3, scale,
						temp_point, temp_axis1, temp_axis2, temp_axis3);
					/* x-line */
					x = temp_point[0] - 0.5*temp_axis1[0];
					y = temp_point[1] - 0.5*temp_axis1[1];
					z = temp_point[2] - 0.5*temp_axis1[2];
					fprintf(vrml_file,"        %f %f %f,\n",x,y,z);
					x += temp_axis1[0];
					y += temp_axis1[1];
					z += temp_axis1[2];
					fprintf(vrml_file,"        %f %f %f,\n",x,y,z);
					/* y-line */
					x = temp_point[0] - 0.5*temp_axis2[0];
					y = temp_point[1] - 0.5*temp_axis2[1];
					z = temp_point[2] - 0.5*temp_axis2[2];
					fprintf(vrml_file,"        %f %f %f,\n",x,y,z);
					x += temp_axis2[0];
					y += temp_axis2[1];
					z += temp_axis2[2];
					fprintf(vrml_file,"        %f %f %f,\n",x,y,z);
					/* z-line */
					x = temp_point[0] - 0.5*temp_axis3[0];
					y = temp_point[1] - 0.5*temp_axis3[1];
					z = temp_point[2] - 0.5*temp_axis3[2];
					fprintf(vrml_file,"        %f %f %f,\n",x,y,z);
					x += temp_axis3[0];
					y += temp_axis3[1];
					z += temp_axis3[2];
					fprintf(vrml_file,"        %f %f %f,\n",x,y,z);
					position += position_values_per_vertex;
					axis1 += axis1_values_per_vertex;
					axis2 += axis2_values_per_vertex;
					axis3 += axis3_values_per_vertex;
					scale += scale_values_per_vertex;
				}
				fprintf(vrml_file,"      ]\n");
				fprintf(vrml_file,"    }\n");
				if (data_spectrum)
				{
					fprintf(vrml_file,"    colorPerVertex FALSE\n");
					spectrum_start_render_vrml(vrml_file,spectrum,material);
					for (i=0;i<index_count;i++)
					{
						for (j=0;j<3;j++)
						{
							spectrum_render_vrml_value(vrml_file,spectrum,material,
								data_values_per_vertex,datum+i*data_values_per_vertex);
						}
					}
					spectrum_end_render_vrml(vrml_file, spectrum);
				}
				fprintf(vrml_file,"    coordIndex [\n");
				for (i=0;i<index_count;i++)
				{
					fprintf(vrml_file,"      %d,%d,-1,%d,%d,-1,%d,%d,-1,\n",
						6*i,6*i+1,6*i+2,6*i+3,6*i+4,6*i+5);
				}
				fprintf(vrml_file,"    ]\n");
				fprintf(vrml_file,"  } #IndexedLineSet\n");
				fprintf(vrml_file,"} #Shape\n");
			}
			else if (glyph) // general case
			{
				if (data_spectrum)
				{
					material_copy=cmzn_material_create_private();
					cmzn_material_set_name(material_copy, "render_vrml_copy");
				}
				else
				{
					material_copy = (cmzn_material *)NULL;
				}
				number_of_skew_glyph_axes=0;
				const int number_of_glyphs =
					cmzn_glyph_repeat_mode_get_number_of_glyphs(glyph_repeat_mode);
				for (i = 0; i < index_count; i++)
				{
					for (int glyph_number = 0; glyph_number < number_of_glyphs; glyph_number++)
					{
						resolve_glyph_axes(glyph_repeat_mode, glyph_number,
							glyph_set->base_size, glyph_set->scale_factors, glyph_set->offset,
							position, axis1, axis2, axis3, scale,
							temp_point, temp_axis1, temp_axis2, temp_axis3);
						/* get the glyph centre as x, y and z */
						x = temp_point[0];
						y = temp_point[1];
						z = temp_point[2];
						/* store 3 axes in ax, bx and cx */
						ax1 = temp_axis1[0];
						ax2 = temp_axis1[1];
						ax3 = temp_axis1[2];
						bx1 = temp_axis2[0];
						bx2 = temp_axis2[1];
						bx3 = temp_axis2[2];
						cx1 = temp_axis3[0];
						cx2 = temp_axis3[1];
						cx3 = temp_axis3[2];
						/* get magnitudes of the glyph axes and normalise ax, bx and cx */
						if (0.0 < (s1 = sqrt(ax1*ax1 + ax2*ax2 + ax3*ax3)))
						{
							ax1 /= s1;
							ax2 /= s1;
							ax3 /= s1;
						}
						if (0.0 < (s2 = sqrt(bx1*bx1 + bx2*bx2 + bx3*bx3)))
						{
							bx1 /= s2;
							bx2 /= s2;
							bx3 /= s2;
						}
						if (0.0 < (s3 = sqrt(cx1*cx1 + cx2*cx2 + cx3*cx3)))
						{
							cx1 /= s3;
							cx2 /= s3;
							cx3 /= s3;
						}
						/* the three axes are either unit length or zero. Checks:
							If all three are non-zero, check that the axes are all mutually
						  orthogonal.
							If two are non-zero, make sure they are othogonal and get the
						  third from the cross product.
							If only one is non-zero, choose any two axes orthogonal to the
						  first as the remaining axes. */
						skewed_axes=0;
						if ((0.0<s1)&&(0.0<s2)&&(0.0<s3))
						{
							/* 3 non-zero axes: check dot product of axis1 (x) axis2 and
								axis2 (x) axis3 are both zero or thereabouts */
							if ((0.00001 < fabs(ax1*bx1 + ax2*bx2 + ax3*bx3))||
								(0.00001 < fabs(bx1*cx1 + bx2*cx2 + bx3*cx3)))
							{
								skewed_axes=1;
							}
						}
						else if (((0.0<s1)&&(0.0<s2)) ||
							((0.0<s2)&&(0.0<s3)) || ((0.0<s3)&&(0.0<s1)))
						{
							/* 2 non-zero axes: check their dot product is zero or
								thereabouts, and get third axis as their cross product. */
							if (0.0==s1)
							{
								if (0.00001 < fabs(bx1*cx1 + bx2*cx2 + bx3*cx3))
								{
									skewed_axes=1;
								}
								else
								{
									ax1 = bx2*cx3 - bx3*cx2;
									ax2 = bx3*cx1 - bx1*cx3;
									ax3 = bx1*cx2 - bx2*cx1;
								}
							}
							else if (0.0==s2)
							{
								if (0.00001 < fabs(cx1*ax1 + cx2*ax2 + cx3*ax3))
								{
									skewed_axes=1;
								}
								else
								{
									bx1 = cx2*ax3 - cx3*ax2;
									bx2 = cx3*ax1 - cx1*ax3;
									bx3 = cx1*ax2 - cx2*ax1;
								}
							}
							else /* if (0.0==s3) */
							{
								if (0.00001 < fabs(ax1*bx1 + ax2*bx2 + ax3*bx3))
								{
									skewed_axes=1;
								}
								else
								{
									cx1 = ax2*bx3 - ax3*bx2;
									cx2 = ax3*bx1 - ax1*bx3;
									cx3 = ax1*bx2 - ax2*bx1;
								}
							}
						}
						else if ((0.0<s1)||(0.0<s2)||(0<s3))
						{
							/* 1 non-zero axis: Get any two other orthogonal axes */
							if (0.0<s1)
							{
								get_orthogonal_axes(ax1,ax2,ax3,&bx1,&bx2,&bx3,&cx1,&cx2,&cx3);
							}
							else if (0.0<s2)
							{
								get_orthogonal_axes(bx1,bx2,bx3,&cx1,&cx2,&cx3,&ax1,&ax2,&ax3);
							}
							else /* if (0.0<s2) */
							{
								get_orthogonal_axes(cx1,cx2,cx3,&ax1,&ax2,&ax3,&bx1,&bx2,&bx3);
							}
						}
						else
						{
							/* All axes non-zero: Use default axes */
							ax1 = 1.0;
							bx2 = 1.0;
							cx3 = 1.0;
						}
						if (skewed_axes)
						{
							/* refuse to output the glyph if axes are skew */
							number_of_skew_glyph_axes++;
						}
						else
						{
							/* find angles - a_angle and b_angle - and axes - a and b - so
							that rotating by a_angle about a followed rotating by b_angle
							about b takes the x-axis to ax, the y-axis to bx and the z-axis
							to cx */
							/* a is perpendicular to the projection of ax onto the y-z plane
							(i) and a_angle is the angle between ax and the x axis.  a_angle
							must be between 0 and pi because the angle is in the plane
							defined by the x axis and i and ax must be in the non-negative
							i half of this plane */
							if (0.0 < (a_magnitude = sqrt(ax2*ax2 + ax3*ax3)))
							{
								a1 = 0.0;
								a2 = -ax3 / a_magnitude;
								a3 = ax2 / a_magnitude;
								a_angle = clamped_acos(ax1);
							}
							else
							{
								a1 = 0.0;
								a2 = 1.0;
								a3 = 0.0;
								if (0 > ax1)
								{
									a_angle = (ZnReal)PI;
								}
								else
								{
									a_angle = 0.0;
								}
							}
							/* rotating by a_angle about a takes the x-axis onto ax and the
							y-axis onto j */
							j1 = -sin(a_angle) * a3;
							j2 = (1.0 - cos(a_angle)) * a2 * a2 + cos(a_angle);
							j3 = (1.0 - cos(a_angle)) * a2 * a3;
							/* bx is perpendicular to ax, because they are orthonormal, and
							j is perpendiculat to ax because they are the images of the y-
							and x- axes under rotation.  So bx and j are in the plane
							perpendicular to ax */
							/* now rotate about ax (b) to get j onto bx */
							b1 = ax1;
							b2 = ax2;
							b3 = ax3;
							/* because j and bx are unit length cos(b_angle)=j dot b */
							b_angle = clamped_acos(j1*bx1 + j2*bx2 + j3*bx3);
							/* acos gives an angle between 0 and pi.  If j cross bx is in the
							same direction as b, then the angle is clockwise, if it is in
							the opposite direction then it is anti-clockwise.   */
							/*???DB.  Lose accuracy fairly badly here.  Need to have
							tolerance so that there are not different numbers of lines for
							ndiff */
#define ZERO_ROTATION_TOLERANCE 0.001
							/*							if (0.0 != b_angle)*/
							if (ZERO_ROTATION_TOLERANCE < fabs(b_angle))
							{
								/* get c = j1 (x) bx */
								c1 = j2*bx3 - j3*bx2;
								c2 = j3*bx1 - j1*bx3;
								c3 = j1*bx2 - j2*bx1;
								/* the magnitude of c is the absolute value of sin(b_angle)
								because j and bx are unit length.  For small theta,
								sin(theta) is approximately theta */
								c_magnitude = sqrt(c1*c1 + c2*c2 + c3*c3);
								if (ZERO_ROTATION_TOLERANCE < fabs(c_magnitude))
								{
									dp = b1*c1 + b2*c2 + b3*c3;
									if (dp < 0.0)
									{
										/* make clockwise */
										b_angle = -b_angle;
									}
								}
								else
								{
									/* b_angle is close to +/- PI (have already checked that
									b_angle isn't near 0 */
									b_angle = (ZnReal)PI;
								}
							}
							fprintf(vrml_file,"Transform {\n");
							fprintf(vrml_file,"  translation %f %f %f\n", x,y,z);
							/* if possible, try to avoid having two Transform nodes */
							/*							if ((0.0 != a_angle)&&(0.0 != b_angle))*/
							if ((ZERO_ROTATION_TOLERANCE < fabs(a_angle))&&
								(ZERO_ROTATION_TOLERANCE < fabs(b_angle)))
							{
								/* b-rotation must wrap a-rotation to occur after it */
								fprintf(vrml_file,"  rotation %f %f %f %f\n",b1,b2,b3,b_angle);
								fprintf(vrml_file,"  children [\n");
								fprintf(vrml_file,"    Transform {\n");
							}
							/*							if (0.0 != a_angle)*/
							if (ZERO_ROTATION_TOLERANCE < fabs(a_angle))
							{
								fprintf(vrml_file,"    rotation %f %f %f %f\n",a1,a2,a3,
									a_angle);
							}
							/*							else if (0.0 != b_angle)*/
							else if (ZERO_ROTATION_TOLERANCE < fabs(b_angle))
							{
								fprintf(vrml_file,"    rotation %f %f %f %f\n",b1,b2,b3,
									b_angle);
							}
							fprintf(vrml_file,"    scale   %f %f %f\n", s1,s2,s3);
							fprintf(vrml_file,"    children [\n");

							/* set the spectrum for this datum, if any */
							if (material_copy)
							{
								MANAGER_COPY_WITHOUT_IDENTIFIER(cmzn_material,name)
									(material_copy, material);
								Spectrum_render_value_on_material(spectrum,material_copy,
									data_values_per_vertex,datum+i*data_values_per_vertex);
								/*???RC temporary until we have a struct Glyph - actual glyph to use
								is at GT_object_get_next_object(glyph) when glyph is in mirror mode */
								/* note no DEF/USE when coloured by a spectrum as will always be
									different every time it is rendered */
								write_graphics_object_vrml(vrml_file, glyph, time,
									(struct LIST(VRML_prototype) *)NULL, /*object_is_glyph*/1,
									material_copy, /*gt_object_already_defined*/0);
							}
							else
							{
								write_graphics_object_vrml(vrml_file, glyph, time,
									vrml_prototype_list, /*object_is_glyph*/1,
									material, /*gt_object_already_defined*/0<i);
							}
							fprintf(vrml_file,"    ]\n");
							/*							if ((0.0 != a_angle)&&(0.0 != b_angle))*/
							if ((ZERO_ROTATION_TOLERANCE < fabs(a_angle))&&
								(ZERO_ROTATION_TOLERANCE < fabs(b_angle)))
							{
								fprintf(vrml_file,"    } #Transform\n");
								fprintf(vrml_file,"  ]\n");
							}
							fprintf(vrml_file,"} #Transform\n");
						}
					}
					position += position_values_per_vertex;
					axis1 += axis1_values_per_vertex;
					axis2 += axis2_values_per_vertex;
					axis3 += axis3_values_per_vertex;
					scale += scale_values_per_vertex;
				}
				if (0 < number_of_skew_glyph_axes)
				{
					display_message(WARNING_MESSAGE, "draw_glyph_set_vrml.  "
						"%d glyph(s) not rendered because they have skewed axes",
						number_of_skew_glyph_axes);
				}
				if (material_copy)
				{
					cmzn_material_destroy(&material_copy);
				}
			}
			/* output label at each point, if supplied */
			char **static_labels = 0;
			for (int labelNumber = 0; labelNumber < 3; ++labelNumber)
			{
				if (glyph_set->static_label_text[labelNumber])
				{
					static_labels = glyph_set->static_label_text;
					break;
				}
			}
			position = position_buffer + position_values_per_vertex * index_start;
			axis1 = axis1_buffer + axis1_values_per_vertex * index_start;
			axis2 = axis2_buffer + axis2_values_per_vertex * index_start;
			axis3 = axis3_buffer + axis3_values_per_vertex * index_start;
			scale = scale_buffer + scale_values_per_vertex * index_start;
			datum = data_buffer + data_values_per_vertex * index_start;
			if (label || static_labels)
			{
				position = position_buffer + position_values_per_vertex * index_start;
				if (data_spectrum)
				{
					material_copy=cmzn_material_create_private();
					cmzn_material_set_name(material_copy, "render_vrml_copy");
				}
				else
				{
					material_copy = (cmzn_material *)NULL;
				}
				const int number_of_glyphs =
					cmzn_glyph_repeat_mode_get_number_of_glyphs(glyph_repeat_mode);
				for (i = 0; i < index_count; i++)
				{
					for (int glyph_number = 0; glyph_number < number_of_glyphs; glyph_number++)
					{
						if (cmzn_glyph_repeat_mode_glyph_number_has_label(glyph_repeat_mode, glyph_number) &&
							((label && (glyph_number == 0)) || (static_labels && static_labels[glyph_number])))
						{
							resolve_glyph_axes(glyph_repeat_mode, glyph_number,
								glyph_set->base_size, glyph_set->scale_factors, glyph_set->offset,
								position, axis1, axis2, axis3, scale,
								temp_point, temp_axis1, temp_axis2, temp_axis3);
							if ((glyph_repeat_mode == CMZN_GLYPH_REPEAT_MODE_MIRROR) && ((scale)[0] < 0.0f))
							{
								for (int j = 0; j < 3; ++j)
								{
									temp_point[j] += (
										(1.0 - glyph_set->label_offset[0])*temp_axis1[j] +
										glyph_set->label_offset[1]*temp_axis2[j] +
										glyph_set->label_offset[2]*temp_axis3[j]);
								}
							}
							else
							{
								for (int j = 0; j < 3; ++j)
								{
									temp_point[j] += (
										glyph_set->label_offset[0]*temp_axis1[j] +
										glyph_set->label_offset[1]*temp_axis2[j] +
										glyph_set->label_offset[2]*temp_axis3[j]);
								}
							}
							x = temp_point[0];
							y = temp_point[1];
							z = temp_point[2];
							fprintf(vrml_file,"Transform {\n");
							fprintf(vrml_file,"  translation %f %f %f\n", x, y, z);
							fprintf(vrml_file,"  children [\n");
							fprintf(vrml_file,"    Billboard {\n");
							fprintf(vrml_file,"      axisOfRotation 0 0 0\n");
							fprintf(vrml_file,"      children [\n");
							fprintf(vrml_file,"Shape {\n");
							fprintf(vrml_file,"  appearance\n");
							if (material)
							{
								fprintf(vrml_file,"Appearance {\n");
								fprintf(vrml_file,"  material\n");
								if (material_copy)
								{
									MANAGER_COPY_WITHOUT_IDENTIFIER(cmzn_material,name)
										(material_copy, material);
									Spectrum_render_value_on_material(spectrum,material_copy,
										data_values_per_vertex,datum+i*data_values_per_vertex);
									activate_material_vrml(vrml_file,material_copy,
										(struct LIST(VRML_prototype) *)NULL,
										/*no_define*/1,/*emissive_only*/0);
									/*???RC text is drawn as a textured surface, hence */
								}
								else
								{
									activate_material_vrml(vrml_file,material,
										(struct LIST(VRML_prototype) *)NULL,
										/*no_define*/1,/*emissive_only*/0);
								}
								fprintf(vrml_file,"} #Appearance\n");
							}
							else
							{
								fprintf(vrml_file,"IS line_appearance\n");
							}
							fprintf(vrml_file,"  geometry Text {\n");
							fprintf(vrml_file,"    string [\n");
							char *text = 0;
							int error = 0;
							if (static_labels && static_labels[glyph_number])
							{
								append_string(&text, static_labels[glyph_number], &error);
							}
							if (label && (glyph_number == 0))
							{
								append_string(&text, label->c_str(), &error);
							}
							if (text)
							{
								make_valid_token(&text);
								if (('\"' == text[0]) || ('\'' == text[0]))
								{
									fprintf(vrml_file,"      %s\n", text);
								}
								else
								{
									fprintf(vrml_file,"      \"%s\"\n", text);
								}
								DEALLOCATE(text);
							}
							fprintf(vrml_file,"    ]\n");
							fprintf(vrml_file,"  } #Text\n");
							fprintf(vrml_file,"} #Shape\n");
							fprintf(vrml_file,"      ]\n");
							fprintf(vrml_file,"    } #Billboard\n");
							fprintf(vrml_file,"  ]\n");
							fprintf(vrml_file,"} #Transform\n");
						}
					}
					position += position_values_per_vertex;
					axis1 += axis1_values_per_vertex;
					axis2 += axis2_values_per_vertex;
					axis3 += axis3_values_per_vertex;
					scale += scale_values_per_vertex;
					label++;
				}
				if (material_copy)
				{
					cmzn_material_destroy(&material_copy);
				}
			}
		}
		if (group)
		{
			fprintf(vrml_file,"  ]\n");
			fprintf(vrml_file,"} #Group\n");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"draw_glyph_set_vrml. Invalid argument(s)");
		return_code = 0;
	}
	return (return_code);
}

static int draw_point_set_vrml(FILE *vrml_file,int n_pts, GLfloat *point_list,
	std::string *labels, gtMarkerType marker_type, int number_of_data_components,
	GLfloat *data, cmzn_material *material, struct cmzn_spectrum *spectrum)
{
	int i, return_code;
	cmzn_material *material_copy;

	if (vrml_file&&point_list&&(1<n_pts)&&
		((0==number_of_data_components)||(data&&material&&spectrum)))
	{
		GLfloat *point = point_list;
		switch(marker_type)
		{
			case g_POINT_MARKER:
			{
				fprintf(vrml_file,"Shape {\n");
				fprintf(vrml_file,"  appearance\n");
				if (material)
				{
					fprintf(vrml_file,"Appearance {\n");
					fprintf(vrml_file,"  material\n");
					activate_material_vrml(vrml_file,material,
						(struct LIST(VRML_prototype) *)NULL,
						/*no_define_material*/0,/*emissive_only*/1);
					fprintf(vrml_file,"} #Appearance\n");
				}
				else
				{
					fprintf(vrml_file,"IS line_appearance\n");
				}
				fprintf(vrml_file,"  geometry PointSet {\n");
				fprintf(vrml_file,"    coord Coordinate {\n");
				fprintf(vrml_file,"      point [\n");
				point=point_list;
				for (i=0;i<n_pts;i++)
				{
					fprintf(vrml_file,"        %f %f %f,\n",
						point[0], point[1], point[2]);
					point += 3;
				}
				fprintf(vrml_file,"      ]\n");
				fprintf(vrml_file,"    }\n");
				if (number_of_data_components && data && spectrum)
				{
					spectrum_start_render_vrml(vrml_file,spectrum,material);
					for (i=0;i<n_pts;i++)
					{
						spectrum_render_vrml_value(vrml_file,spectrum,material,
							number_of_data_components,data+i*number_of_data_components);
					}
					spectrum_end_render_vrml(vrml_file, spectrum);
				}
				fprintf(vrml_file,"  } #PointSet\n");
				fprintf(vrml_file,"} #Shape\n");
			} break;
			default:
			{
			} break;
		}
		if (material&&Graphical_material_get_texture(material))
		{
			display_message(INFORMATION_MESSAGE,"draw_point_set_vrml.  "
				"VRML does not allow a texture to be applied to a pointset");
		}
		if (labels)
		{
			std::string *label = labels;
			point=point_list;
			if (number_of_data_components && data && material && spectrum)
			{
				material_copy=cmzn_material_create_private();
				cmzn_material_set_name(material_copy, "render_vrml_copy");
			}
			else
			{
				material_copy = (cmzn_material *)NULL;
			}
			for (i=0;i<n_pts;i++)
			{
				fprintf(vrml_file,"Transform {\n");
				fprintf(vrml_file,"  translation %f %f %f\n", point[0], point[1], point[2]);
				fprintf(vrml_file,"  children [\n");
				fprintf(vrml_file,"    Billboard {\n");
				fprintf(vrml_file,"      axisOfRotation 0 0 0\n");
				fprintf(vrml_file,"      children [\n");
				fprintf(vrml_file,"Shape {\n");
				fprintf(vrml_file,"  appearance\n");
				if (material)
				{
					fprintf(vrml_file,"Appearance {\n");
					fprintf(vrml_file,"  material\n");
					if (material_copy)
					{
						MANAGER_COPY_WITHOUT_IDENTIFIER(cmzn_material,name)
							(material_copy, material);
						Spectrum_render_value_on_material(spectrum,material_copy,
							number_of_data_components,data+i*number_of_data_components);
						activate_material_vrml(vrml_file,material_copy,
							(struct LIST(VRML_prototype) *)NULL,
							/*no_define*/1,/*emissive_only*/0);
						/*???RC text is drawn as a textured surface, hence */
					}
					else
					{
						activate_material_vrml(vrml_file,material,
							(struct LIST(VRML_prototype) *)NULL,
							/*no_define*/1,/*emissive_only*/0);
					}
					fprintf(vrml_file,"}\n");
				}
				else
				{
					fprintf(vrml_file,"IS line_appearance\n");
				}
				fprintf(vrml_file,"  geometry Text {\n");
				fprintf(vrml_file,"    string [\n");
				fprintf(vrml_file,"      \"%s\"\n", label->c_str());
				fprintf(vrml_file,"    ]\n");
				fprintf(vrml_file,"  } #Text\n");
				fprintf(vrml_file,"} #Shape\n");
				fprintf(vrml_file,"      ]\n");
				fprintf(vrml_file,"    } #Billboard\n");
				fprintf(vrml_file,"  ]\n");
				fprintf(vrml_file,"} #Transform\n");
				point += 3;
				label++;
			}
			if (material_copy)
			{
				cmzn_material_destroy(&material_copy);
			}
		}
		return_code = 1;
	}
	else
	{
		if (1<n_pts)
		{
			display_message(ERROR_MESSAGE,
				"draw_point_set_vrml.  Invalid argument(s)");
			return_code=0;
		}
		else
		{
			return_code=1;
		}
	}

	return (return_code);
} /* draw_point_set_vrml */

static int draw_polyline_vrml(FILE *vrml_file,GLfloat *point_list,
	int number_of_data_components,GLfloat *data,
	cmzn_material *material,struct cmzn_spectrum *spectrum,int n_pts,
	enum GT_polyline_type polyline_type)
/*******************************************************************************
LAST MODIFIED : 9 May 1999

DESCRIPTION :
Writes VRML code to the file handle which represents the given
continuous polyline. If data or spectrum are NULL they are ignored.
==============================================================================*/
{
	int i,number_of_segments,return_code;

	ENTER(draw_polyline_vrml);
	const bool data_spectrum = (0 < number_of_data_components) && (0 != data) &&
		(0 != spectrum) && (0 != material);
	if (vrml_file&&point_list&&(1<n_pts))
	{
		return_code=1;
		switch (polyline_type)
		{
			case g_PLAIN:
			{
				number_of_segments=n_pts;
			} break;
			case g_PLAIN_DISCONTINUOUS:
			{
				/* n_pts = number of line segments in this case */
				number_of_segments=n_pts/2;
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"draw_polyline_vrml.  Unsupported polyline_type");
				return_code=0;
			}
		}
		if (return_code)
		{
			fprintf(vrml_file,"Shape {\n");
			fprintf(vrml_file,"  appearance\n");
			if (material)
			{
				fprintf(vrml_file,"Appearance {\n");
				fprintf(vrml_file,"  material\n");
				activate_material_vrml(vrml_file,material,
					(struct LIST(VRML_prototype) *)NULL,
					/*no_define_material*/0,/*emissive_only*/1);
				fprintf(vrml_file,"} #Appearance\n");
			}
			else
			{
				fprintf(vrml_file,"IS line_appearance\n");
			}
			fprintf(vrml_file,"  geometry IndexedLineSet {\n");
			fprintf(vrml_file,"    coord Coordinate {\n");
			fprintf(vrml_file,"      point [\n");
			for (i=0;i<n_pts;i++)
			{
				fprintf(vrml_file,"        %f %f %f,\n",point_list[i*3+0],
					point_list[i*3+1],point_list[i*3+2]);
			}
			fprintf(vrml_file,"      ]\n");
			fprintf(vrml_file,"    }\n");
			if (data_spectrum)
			{
				fprintf(vrml_file,"    colorPerVertex TRUE\n");
				spectrum_start_render_vrml(vrml_file,spectrum,material);
				for (i=0;i<n_pts;i++)
				{
					spectrum_render_vrml_value(vrml_file,spectrum,material,
						number_of_data_components,data+i*number_of_data_components);
				}
				spectrum_end_render_vrml(vrml_file, spectrum);
			}
			if (material&&Graphical_material_get_texture(material))
			{
				display_message(INFORMATION_MESSAGE,"draw_polyline_vrml.  "
					"VRML does not allow a texture to be applied to a line");
			}
			fprintf(vrml_file,"    coordIndex [\n");
			switch (polyline_type)
			{
				case g_PLAIN:
				{
					fprintf(vrml_file,"      ");
					for (i=0;i<number_of_segments;i++)
					{
						fprintf(vrml_file,"%d,",i);
					}
					fprintf(vrml_file,"-1\n");
				} break;
				case g_PLAIN_DISCONTINUOUS:
				{
					for (i=0;i<number_of_segments;i++)
					{
						fprintf(vrml_file,"      %d,%d,-1\n",2*i,2*i+1);
					}
				} break;
				default:
				{
				} break;
			}
			fprintf(vrml_file,"    ]\n");
			fprintf(vrml_file,"  } #IndexedLineSet\n");
			fprintf(vrml_file,"} #Shape\n");
		}
	}
	else
	{
		if (1<n_pts)
		{
			display_message(ERROR_MESSAGE,"draw_polyline_vrml.  Invalid argument(s)");
			return_code=0;
		}
		else
		{
			return_code=1;
		}
	}
	LEAVE;

	return (return_code);
} /* draw_polyline_vrml */

int draw_surface_vrml(FILE *vrml_file,
	Graphics_vertex_array *vertex_array,
	cmzn_material *material,struct cmzn_spectrum *spectrum,
	enum GT_surface_type surface_type,enum cmzn_graphics_render_polygon_mode render_polygon_mode,
	struct LIST(VRML_prototype) *vrml_prototype_list)
/*******************************************************************************
LAST MODIFIED : 9 July 1999

DESCRIPTION :
==============================================================================*/
{
	unsigned int i;
	int return_code = 1, group = 0;
	struct Texture *texture;
	if (vertex_array)
	{
		unsigned int surface_index;
		unsigned int surface_count =
			vertex_array->get_number_of_vertices(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START);
		GLfloat *position_buffer = 0, *data_buffer = 0, *normal_buffer = 0,
			*texture_coordinate0_buffer = 0;
		unsigned int position_values_per_vertex, position_vertex_count,
			data_values_per_vertex, data_vertex_count, normal_values_per_vertex,
			normal_vertex_count, texture_coordinate0_values_per_vertex,
			texture_coordinate0_vertex_count;
		if (0 == surface_count)
			return 1;

		vertex_array->get_float_vertex_buffer(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
			&position_buffer, &position_values_per_vertex, &position_vertex_count);
		vertex_array->get_float_vertex_buffer(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
			&data_buffer, &data_values_per_vertex, &data_vertex_count);
		vertex_array->get_float_vertex_buffer(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
			&normal_buffer, &normal_values_per_vertex, &normal_vertex_count);
		vertex_array->get_float_vertex_buffer(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_TEXTURE_COORDINATE_ZERO,
			&texture_coordinate0_buffer, &texture_coordinate0_values_per_vertex,
			&texture_coordinate0_vertex_count);
		const bool data_spectrum = (0 < data_values_per_vertex) && (0 != data_buffer) &&
			(0 != spectrum) && (0 != material);

		switch (surface_type)
		{
			case g_SHADED:
			case g_SHADED_TEXMAP:
			case g_SH_DISCONTINUOUS:
			case g_SH_DISCONTINUOUS_TEXMAP:
			{
			} break;
			default:
			{
				return 0;
			} break;
		}
		if (surface_count > 1)
		{
			fprintf(vrml_file,"Group {\n");
			fprintf(vrml_file,"  children [\n");
			group = 1;
		}

		fprintf(vrml_file,"Shape {\n");
		fprintf(vrml_file,"  appearance\n");
		fprintf(vrml_file,"Appearance {\n");
		fprintf(vrml_file,"  material\n");
		activate_material_vrml(vrml_file,material,
			vrml_prototype_list,
			/*no_define_material*/0,/*emissive_only*/0);
		texture = Graphical_material_get_texture(material);
		if (texture)
		{
			write_texture_vrml(vrml_file, texture);
		}
		fprintf(vrml_file,"}\n");
		if (CMZN_GRAPHICS_RENDER_POLYGON_MODE_WIREFRAME == render_polygon_mode)
		{
			fprintf(vrml_file,"  geometry IndexedLineSet {\n");
		}
		else
		{
			fprintf(vrml_file,"  geometry IndexedFaceSet {\n");
			fprintf(vrml_file,"    solid FALSE\n");
		}
		fprintf(vrml_file,"    coord Coordinate {\n");
		fprintf(vrml_file,"      point [\n");
		GLfloat *current_value = position_buffer;
		unsigned int number_to_fill = 3 - position_values_per_vertex;
		for (i=position_vertex_count;i>0;i--)
		{
			fprintf(vrml_file, "       ");
			for (unsigned int j = 0; j < position_values_per_vertex; j++)
			{
				fprintf(vrml_file," %f", current_value[j]);
			}
			for (unsigned int j = 0; j < number_to_fill; j++)
			{
				fprintf(vrml_file," 0.0");
			}
			current_value += position_values_per_vertex;
			fprintf(vrml_file,",\n");
		}
		fprintf(vrml_file,"      ]\n");
		fprintf(vrml_file,"    }\n");
		if (CMZN_GRAPHICS_RENDER_POLYGON_MODE_SHADED == render_polygon_mode)
		{
			current_value = normal_buffer;
			if (current_value)
			{
				fprintf(vrml_file,"    normal Normal {\n");
				fprintf(vrml_file,"      vector [\n");
				for (i=position_vertex_count;i>0;i--)
				{
					fprintf(vrml_file,"        %f %f %f,\n",current_value[0],current_value[1],
						current_value[2]);
					current_value += normal_values_per_vertex;
				}
				fprintf(vrml_file,"      ]\n");
				fprintf(vrml_file,"    }\n");
			}
		}
		if (data_spectrum)
		{
			spectrum_start_render_vrml(vrml_file,spectrum,material);
			for (i=0;i<position_vertex_count;i++)
			{
				spectrum_render_vrml_value(vrml_file,spectrum,material,
					data_values_per_vertex,data_buffer+data_values_per_vertex*i);
			}
			spectrum_end_render_vrml(vrml_file, spectrum);
		}
		/* texture coordinates */
		if (CMZN_GRAPHICS_RENDER_POLYGON_MODE_SHADED == render_polygon_mode)
		{
			current_value = texture_coordinate0_buffer;
			if (current_value)
			{
				fprintf(vrml_file,"    texCoord TextureCoordinate {\n");
				fprintf(vrml_file,"      point [\n");
				for (i=position_vertex_count;i>0;i--)
				{
					fprintf(vrml_file,"        %f %f,\n",current_value[0],current_value[1]);
					current_value += texture_coordinate0_values_per_vertex;
				}
				fprintf(vrml_file,"      ]\n");
				fprintf(vrml_file,"    }\n");
			}
		}
		unsigned int *index_vertex_buffer = 0, index_values_per_vertex = 0, index_vertex_count = 0;
		vertex_array->get_unsigned_integer_vertex_buffer(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_STRIP_INDEX_ARRAY,
			&index_vertex_buffer, &index_values_per_vertex,
			&index_vertex_count);
		fprintf(vrml_file,"    coordIndex [\n");

		for (surface_index = 0; surface_index < surface_count; surface_index++)
		{
			int object_name = 0;
			vertex_array->get_integer_attribute(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_OBJECT_ID,
				surface_index, 1, &object_name);
			if (object_name > -1)
			{
				if (index_vertex_buffer)
				{
					unsigned int number_of_strips = 0;
					unsigned int strip_start = 0;
					vertex_array->get_unsigned_integer_attribute(
						GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_STRIPS,
						surface_index, 1, &number_of_strips);
					vertex_array->get_unsigned_integer_attribute(
						GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_STRIP_START,
						surface_index, 1, &strip_start);
					for (i = 0; i < number_of_strips; i++)
					{
						unsigned int points_per_strip = 0;
						unsigned int index_start_for_strip = 0;
						vertex_array->get_unsigned_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_STRIP_INDEX_START,
							strip_start+i, 1, &index_start_for_strip);
						vertex_array->get_unsigned_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_POINTS_FOR_STRIP,
							strip_start+i, 1, &points_per_strip);
						unsigned int *indices = &index_vertex_buffer[index_start_for_strip];
						for (unsigned int j = 0; j < points_per_strip - 2; j++)
						{
							if (0 == (j % 2))
							{
								if (CMZN_GRAPHICS_RENDER_POLYGON_MODE_WIREFRAME == render_polygon_mode)
								{
									fprintf(vrml_file,"      %d,%d,%d,%d,-1\n",
										indices[j],indices[j+1], indices[j+2], indices[j]);
								}
								else
								{
									fprintf(vrml_file,"      %d,%d,%d,-1\n",
										indices[j], indices[j+1], indices[j+2]);
								}
							}
							else
							{
								if (CMZN_GRAPHICS_RENDER_POLYGON_MODE_WIREFRAME == render_polygon_mode)
								{
									fprintf(vrml_file,"      %d,%d,%d,%d,-1\n",
										indices[j+1],indices[j], indices[j+2], indices[j+1]);
								}
								else
								{
									fprintf(vrml_file,"      %d,%d,%d,-1\n",
										indices[j+1], indices[j], indices[j+2]);
								}
							}
						}
					}
				}
				else
				{
					unsigned int index_start, index_count;
					vertex_array->get_unsigned_integer_attribute(
						GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
						surface_index, 1, &index_start);
					vertex_array->get_unsigned_integer_attribute(
						GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
						surface_index, 1, &index_count);
					unsigned int number_of_triangles = index_count / 3;
					int current_index = index_start;
					for (i = 0; i < number_of_triangles; i++)
					{
						if (CMZN_GRAPHICS_RENDER_POLYGON_MODE_WIREFRAME == render_polygon_mode)
						{
							fprintf(vrml_file,"      %d,%d,%d,%d,-1\n",
								current_index,current_index+1, current_index+2, current_index);
						}
						else
						{
							fprintf(vrml_file,"      %d,%d,%d,-1\n",
								current_index, current_index+1, current_index+2);
						}
						current_index += 3;
					}
				}
			}
		}

		fprintf(vrml_file,"    ]\n");
		if (CMZN_GRAPHICS_RENDER_POLYGON_MODE_WIREFRAME == render_polygon_mode)
		{
			fprintf(vrml_file,"  } #IndexedLineSet\n");
		}
		else
		{
			fprintf(vrml_file,"  } #IndexedFaceSet\n");
		}
		fprintf(vrml_file,"} #Shape\n");
		if (group)
		{
			fprintf(vrml_file,"  ]\n");
			fprintf(vrml_file,"} #Group\n");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
				"draw_surface_vrml.  Invalid argument(s)");
		return_code = 0;
	}

	return (return_code);
} /* draw_surface_vrml */

int makevrml(FILE *vrml_file,gtObject *object,ZnReal time,
	struct LIST(VRML_prototype) *vrml_prototype_list)
/*******************************************************************************
LAST MODIFIED : 20 March 2003

DESCRIPTION :
Convert graphical object into API object.
Only writes the geometry field.
???RC Crap
==============================================================================*/
{
	int return_code = 1, group = 0;

	if (vrml_file&&object)
	{
		if (0 < object->number_of_times)
		{
			if (!object->primitive_lists)
			{
				display_message(ERROR_MESSAGE,
					"makevrml.  Invalid primitive_lists");
				return_code = 0;
			}
		}
		if ((0 < object->number_of_times) && return_code)
		{
			switch (GT_object_get_type(object))
			{
				case g_GLYPH_SET_VERTEX_BUFFERS:
				{
					struct GT_glyphset_vertex_buffers *glyph_set =
						object->primitive_lists->gt_glyphset_vertex_buffers;
					if (glyph_set)
					{
						draw_glyph_set_vrml(vrml_file, glyph_set,
							object->vertex_array,
							object->default_material, object->spectrum,
							time, vrml_prototype_list);
					}
					return_code=1;
				} break;
				case g_POINT_SET_VERTEX_BUFFERS:
				{
					struct GT_pointset_vertex_buffers *point_set =
						object->primitive_lists->gt_pointset_vertex_buffers;
					if (point_set)
					{
						GLfloat *position_buffer, *data_buffer;
						unsigned int position_values_per_vertex, position_vertex_count,
							data_values_per_vertex, data_vertex_count,
							label_per_vertex, label_count;

						position_buffer = 0;
						object->vertex_array->get_float_vertex_buffer(
							GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
							&position_buffer, &position_values_per_vertex, &position_vertex_count);

						data_buffer = 0;
						object->vertex_array->get_float_vertex_buffer(
							GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
							&data_buffer, &data_values_per_vertex, &data_vertex_count);

						std::string *label_buffer = 0;
						object->vertex_array->get_string_vertex_buffer(
							GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_LABEL,
							&label_buffer, &label_per_vertex, &label_count);

						draw_point_set_vrml(vrml_file,
							position_vertex_count, position_buffer,
							label_buffer, point_set->marker_type,
							data_values_per_vertex, data_buffer,
							object->default_material, object->spectrum);
						return_code=1;
					}
					else
					{
						display_message(ERROR_MESSAGE,"makevrml.  Missing point");
						return_code=0;
					}
				} break;
				case g_POLYLINE_VERTEX_BUFFERS:
				{
					GT_polyline_vertex_buffers *line;
					line = object->primitive_lists->gt_polyline_vertex_buffers;
					if (line)
					{
						unsigned int line_index;
						unsigned int line_count =
							object->vertex_array->get_number_of_vertices(
							GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START);

						GLfloat *position_buffer, *data_buffer, *normal_buffer;
						unsigned int position_values_per_vertex, position_vertex_count,
						data_values_per_vertex, data_vertex_count, normal_values_per_vertex,
						normal_vertex_count;

						if (line_count > 1)
						{
							fprintf(vrml_file,"Group {\n");
							fprintf(vrml_file,"  children [\n");
							group = 1;
						}

						position_buffer = 0;
						object->vertex_array->get_float_vertex_buffer(
							GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
							&position_buffer, &position_values_per_vertex, &position_vertex_count);

						data_buffer = 0;
						object->vertex_array->get_float_vertex_buffer(
							GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
							&data_buffer, &data_values_per_vertex, &data_vertex_count);

						normal_buffer = 0;
						object->vertex_array->get_float_vertex_buffer(
							GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
							&normal_buffer, &normal_values_per_vertex, &normal_vertex_count);

						for (line_index = 0; line_index < line_count; line_index++)
						{
							int object_name;
							object->vertex_array->get_integer_attribute(
								GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_VERTEX_ID,
								line_index, 1, &object_name);
							if (object_name > -1)
							{
								unsigned int index_start, index_count;
								GLfloat *position_vertex, *data_vertex;

								object->vertex_array->get_unsigned_integer_attribute(
									GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
									line_index, 1, &index_start);
								object->vertex_array->get_unsigned_integer_attribute(
									GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
									line_index, 1, &index_count);

								position_vertex = position_buffer +
									position_values_per_vertex * index_start;
								if (data_buffer)
								{
									data_vertex = data_buffer +
										data_values_per_vertex * index_start;
								}
								else
								{
									data_values_per_vertex = 0;
									data_vertex = NULL;
								}
								draw_polyline_vrml(vrml_file, position_vertex,
									data_values_per_vertex, data_vertex,
									object->default_material,object->spectrum,
									index_count, line->polyline_type);
							}
						}
						if (group)
						{
							fprintf(vrml_file,"  ]\n");
							fprintf(vrml_file,"} #Group\n");
						}
					}
					else
					{
						display_message(ERROR_MESSAGE,"makevrml.  Missing line");
						return_code=0;
					}
				} break;
				case g_SURFACE_VERTEX_BUFFERS:
				{
					struct GT_surface_vertex_buffers *surface =
						object->primitive_lists->gt_surface_vertex_buffers;
					if (surface)
					{
						draw_surface_vrml(vrml_file,
							object->vertex_array,
							object->default_material,object->spectrum,
							surface->surface_type,surface->render_polygon_mode,
							vrml_prototype_list);
						return_code=1;
					}
					else
					{
						display_message(ERROR_MESSAGE,"makevrml.  Missing surface");
						return_code=0;
					}
				} break;
				default:
				{
					display_message(ERROR_MESSAGE,"makevrml.  Invalid object type");
					return_code=0;
				} break;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"makevrml.  Missing object");
		return_code=0;
	}

	return (return_code);
} /* makevrml */

static int write_graphics_object_vrml(FILE *vrml_file,
	struct GT_object *gt_object,ZnReal time,
	struct LIST(VRML_prototype) *vrml_prototype_list,
	int object_is_glyph,cmzn_material *default_material,
	int gt_object_already_prototyped)
/*******************************************************************************
LAST MODIFIED : 21 June 2001

DESCRIPTION :
==============================================================================*/
{
	char *dot_pointer, *prototype_name = NULL, *material_name;
	int group, in_def, return_code;
	struct GT_object *temp_gt_object;
	struct VRML_prototype *vrml_prototype;
  /* DPN 21 June 2001 - Using fgetpos() like this is wrong! */
	long file_pointer,save_file_pointer = 0L;

	ENTER(write_graphics_object_vrml);
	if (vrml_file&&gt_object&&gt_object->name&&default_material)
	{
		group = 0;
		in_def = 0;
		return_code=1;

		const char *parsed_name = NULL;
		if (object_is_glyph)
			parsed_name = duplicate_string(gt_object->name);
		else
			parsed_name = duplicate_string(gt_object->name+1);
		char *temp_string = NULL;
		const char *num_string = "";
		if ((temp_string = (char *)strrchr(parsed_name, '/')))
		{
			num_string = temp_string+1;
			temp_string[0] = '\0';
		}
		while ((temp_string = (char *)strchr(parsed_name, '/')))
		{
			temp_string[0] = '_';
		}
		if (GET_NAME(cmzn_material)(default_material, &material_name)&&
			ALLOCATE(prototype_name,char,strlen(parsed_name)+strlen(num_string)+10+strlen(material_name)))
		{
			if (object_is_glyph)
			{
				sprintf(prototype_name,"glyph_%s%s_%s",parsed_name, num_string, material_name);
			}
			else
			{
				sprintf(prototype_name,"object_%s%s_%s",parsed_name, num_string, material_name);
			}
			/* Can't have certain characters (.: ) in a name */
			while ((dot_pointer = strchr(prototype_name, '.'))
				|| (dot_pointer = strchr(prototype_name, ' '))
				|| (dot_pointer = strchr(prototype_name, ':')))
			{
				*dot_pointer = '_';
			}
			temp_gt_object=gt_object;
			if (vrml_prototype_list)
			{
				vrml_prototype=CREATE(VRML_prototype)(prototype_name,(struct Texture *)NULL,
					default_material, gt_object,/*time*/0);
				if (gt_object_already_prototyped ||
					FIRST_OBJECT_IN_LIST_THAT(VRML_prototype)(VRML_prototype_is_identical,
					(void *)vrml_prototype,vrml_prototype_list))
				{
					fprintf(vrml_file,"USE %s\n", prototype_name);
					/* Don't write the objects */
					temp_gt_object=(struct GT_object *)NULL;
					DESTROY(VRML_prototype)(&vrml_prototype);
				}
				else
				{
					fprintf(vrml_file,"DEF %s\n", prototype_name);
					/* save file pointer so we can see if anything was output */
					save_file_pointer = ftell(vrml_file);
					in_def = 1;
					ADD_OBJECT_TO_LIST(VRML_prototype)(vrml_prototype,vrml_prototype_list);
				}
			}
			if (temp_gt_object)
			{
				if (GT_object_get_next_object(temp_gt_object))
				{
					fprintf(vrml_file,"Group {\n");
					fprintf(vrml_file,"  children [\n");
					group = 1;
				}
				while ((NULL != temp_gt_object)&&return_code)
				{
					if (temp_gt_object->default_material)
					{
						return_code=
							makevrml(vrml_file,temp_gt_object,time,vrml_prototype_list);
					}
					else
					{
						/* Temporarily set the correct material in objects that don't have one */
						temp_gt_object->default_material = default_material;
						return_code=
							makevrml(vrml_file,temp_gt_object,time,vrml_prototype_list);
						temp_gt_object->default_material = (cmzn_material *)NULL;
					}
					temp_gt_object=GT_object_get_next_object(temp_gt_object);
				}
				if (group)
				{
					fprintf(vrml_file,"  ]\n");
					fprintf(vrml_file,"} #Group\n");
				}
				if (in_def)
				{
					/* check if anything was output; if not, add a dummy node */
					file_pointer = ftell(vrml_file);
					if (file_pointer == save_file_pointer)
					{
						fprintf(vrml_file,"Group {\n");
						fprintf(vrml_file,"# Dummy group node for empty object\n");
						fprintf(vrml_file,"} #Group\n");
					}
					fprintf(vrml_file, "#END DEF %s\n", prototype_name);
				}
			}
			DEALLOCATE(prototype_name);
			DEALLOCATE(material_name);
		}
		DEALLOCATE(parsed_name);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"write_graphics_object_vrml.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* write_graphics_object_vrml */

struct Export_to_vrml_data
{
	FILE *vrml_file;
	cmzn_scene_id scene;
	cmzn_scenefilter_id filter;
	/* store materials, glyphs and gt_objects that have been DEFined already so
		 we can USE them again */
	struct LIST(VRML_prototype) *vrml_prototype_list;
}; /* struct Export_to_vrml_data */

static int graphics_object_export_to_vrml(struct GT_object *gt_object,
	double time,void *export_to_vrml_data_void)
/*******************************************************************************
LAST MODIFIED : 9 May 1999

DESCRIPTION :
Write the graphics object to the <file>.
???RC <time> is a double because that is expected for a
graphics_object_tree_iterator_function
==============================================================================*/
{
	FILE *vrml_file;
	int return_code = 0;
	struct Export_to_vrml_data *export_to_vrml_data;

	ENTER(graphics_object_export_to_vrml);
	if (gt_object&&(export_to_vrml_data=
		(struct Export_to_vrml_data *)export_to_vrml_data_void))
	{
		vrml_file=export_to_vrml_data->vrml_file;
		switch(GT_object_get_type(gt_object))
		{
			case g_POINT_SET_VERTEX_BUFFERS:
			case g_POLYLINE_VERTEX_BUFFERS:
			case g_GLYPH_SET_VERTEX_BUFFERS:
			case g_SURFACE_VERTEX_BUFFERS:
			{
				return_code=write_graphics_object_vrml(vrml_file,gt_object,time,
					export_to_vrml_data->vrml_prototype_list,
					/*object_is_glyph*/0,gt_object->default_material,
					/*gt_object_already_defined*/0);
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,"graphics_object_export_to_vrml.  "
					"The graphics object %s is of a type not yet supported",
					gt_object->name);
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"graphics_object_export_to_vrml.  "
			"Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* graphics_object_export_to_vrml */

/*
Global functions
----------------
*/

int export_to_vrml(char *file_name, cmzn_scene_id scene,
	cmzn_scenefilter_id filter)
/******************************************************************************
LAST MODIFIED : 19 October 2001

DESCRIPTION :
Renders the visible objects to a VRML file.
==============================================================================*/
{
	FILE *vrml_file;
	int return_code;
	struct Export_to_vrml_data export_to_vrml_data;

	if (file_name&&scene)
	{
		build_Scene(scene, filter);
		/* open file and add header */
		vrml_file=fopen(file_name,"w");
		if (vrml_file)
		{
			/* 1. Write the VRML header */
			/*???RC Add copyright message, source/how to contact UniServices??
				Treat this as Advertising... */
			fprintf(vrml_file,"#VRML V2.0 utf8\n# CMGUI VRML Generator\n");
			export_to_vrml_data.vrml_file=vrml_file;
			export_to_vrml_data.scene=scene;
			export_to_vrml_data.filter=filter;
			export_to_vrml_data.vrml_prototype_list=NULL;
			/* 2. Write scene graph */
			/* transform.... */
			/* draw objects */
			fprintf(vrml_file,"Group {\n");
			fprintf(vrml_file,"  children [\n");

			double centre[3], size[3];
			const int result = scene->getCoordinatesRangeCentreSize(filter, centre, size);
			if ((CMZN_OK == result) || (CMZN_ERROR_NOT_FOUND == result))
			{
				const double radius = sqrt(size[0]*size[0] + size[1]*size[1] + size[2]*size[2]);
				fprintf(vrml_file,"    Viewpoint {\n");
				fprintf(vrml_file,"      description \"default\"\n");
				fprintf(vrml_file,"      position %f %f %f\n",
					centre[0], centre[1], centre[2] + radius);
				fprintf(vrml_file,"    } #Viewpoint\n");
			}
			fprintf(vrml_file,"    NavigationInfo {\n");
			fprintf(vrml_file,"      type [\"EXAMINE\",\"ANY\"]\n");
			fprintf(vrml_file,"    } #NavigationInfo\n");
			export_to_vrml_data.vrml_prototype_list=
				CREATE(LIST(VRML_prototype))();
			return_code=for_each_graphics_object_in_scene_tree(scene, filter,
				graphics_object_export_to_vrml,(void *)&export_to_vrml_data);
			DESTROY(LIST(VRML_prototype))(&export_to_vrml_data.vrml_prototype_list);
			fprintf(vrml_file,"  ]\n");
			fprintf(vrml_file,"} #Group\n");
			/* set lights... */
			fclose(vrml_file);
			return_code = 1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"export_to_vrml.  Could not open vrml file");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"export_to_vrml.  Invalid argument(s)");
		return_code=0;
	}

	return (return_code);
} /* export_to_vrml */
