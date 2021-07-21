/*******************************************************************************
FILE : texture.c

LAST MODIFIED : 31 March 2008

DESCRIPTION :
The functions for manipulating graphical textures.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "opencmiss/zinc/zincconfigure.h"
#include "opencmiss/zinc/result.h"
#include "opencmiss/zinc/status.h"
#if defined (WIN32_SYSTEM)
#define _USE_MATH_DEFINES
#endif // defined (WIN32_SYSTEM)
#include <math.h>
#if defined (WIN32_SYSTEM)
#if (defined(_MSC_VER) && (_MSC_VER < 1700))
// Added in Visual Studio 2012
double log2(double value)
{
	return log(value) / M_LN2;
}
#endif
#if (defined(_MSC_VER) && (_MSC_VER < 1800))
// Added in Visual Studio 2013
long lround(double d)
{
	return (long)(d>0 ? d+0.5 : ceil(d-0.5));
}
#endif
#endif // defined (WIN32_SYSTEM)
#include "general/debug.h"
#include "general/image_utilities.h"
#include "general/indexed_list_private.h"
#include "general/myio.h"
#include "general/mystring.h"
#include "general/object.h"
#include "graphics/light.hpp"
#include "graphics/graphics_library.h"
#include "graphics/auxiliary_graphics_types.h"
#include "graphics/texture.h"
#include "general/message.h"
#include "general/enumerator_private.hpp"
#include "graphics/texture.hpp"
#include "graphics/render_gl.h"

/*
Module types
------------
*/

struct Texture_property
/*******************************************************************************
LAST MODIFIED : 25 October 2007

DESCRIPTION :
Stores a Texture_property, name and value pair.
==============================================================================*/
{
	const char *name;
	const char *value;
	int access_count;
};

/***************************************************************************//**
 * Flag for controlling the compile and execute stages of the texture
 */
enum Texture_compile_state
{
	/* Everything needs to be compiled */
	TEXTURE_COMPILE_STATE_NOT_COMPILED,
	 /** The display list is up to date, implies the texture object is compiled too. */
	TEXTURE_COMPILE_STATE_DISPLAY_LIST_COMPILED,
	/** The texture object data need to be updated but the display list doesn't. */
	TEXTURE_COMPILE_STATE_TEXTURE_OBJECT_UPDATE_REQUIRED,
	/** The texture object is up to date but the display list is not. */
	TEXTURE_COMPILE_STATE_TEXTURE_OBJECT_COMPILED
};

typedef enum
{
	CMZN_TEXTURE_LUMINANCE_SIZE = 1
} cmzn_texture_graphics_parameter_id;

DECLARE_LIST_TYPES(Texture_property);

PROTOTYPE_OBJECT_FUNCTIONS(Texture_property);
PROTOTYPE_LIST_FUNCTIONS(Texture_property);
PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(Texture_property,name,const char *);

struct Texture
/*******************************************************************************
LAST MODIFIED : 25 October 2007

DESCRIPTION :
The properties of a graphical texture.
???DB.  GL separates the texture from the texture environment (for performance
	reasons ?).  I do not think that this is "natural", especially because only
	certain environment types can be used with certain numbers of components.  The
	same index is used for environment and texture.
==============================================================================*/
{
	/* the name of the texture */
	const char *name;
	/* texture dimension: 1,2 or 3, automatically determined from image files,
		 ie. 2 if it has one texel depth, 1 if it has one texel height */
	int dimension;
	/* the range of texture coordinates in model units */
	double depth, height, width;
	/* image distortion parameters in the physical size of the image, where 0,0
		 refers to the left, bottom corner and width, height is the right, top.
		 Only pure radial distortion in physical space is supported. Note that these
		 are just a convenient storage space for these values: the texture display
		 list does not need to be updated if these change. */
	GLfloat distortion_centre_x,distortion_centre_y,distortion_factor_k1;
	/* the name of the file the image was read from */
		/*???DB.  May get rid of this when able to "draw" the image in response to
			a list texture command */
		/*???RC.  Keep it for working out if we can reuse already loaded textures */
	char *image_file_name;
	/* file number pattern and ranges for 3-D textures */
	char *file_number_pattern;
	int start_file_number, stop_file_number, file_number_increment;
	enum Texture_storage_type storage;
	/* number of 8-bit bytes per component */
	int number_of_bytes_per_component;
	/* array of 4-byte row-aligned unsigned chars containing the texel data.  The
		 texels fill the image from left to right, bottom to top.  Each row of texel
		 information must be 4-byte aligned (end of row byte padded) */
		/*???DB.  OpenGL allows greater choice, but this will not be used */
	unsigned char *image;
	/* OpenGL requires the width and height of textures to be in powers of 2.
		Hence, only the original width x height contains useful image data */
	/* stored image size in texels */
	int depth_texels, height_texels, width_texels;
	/* original image size in texels */
	int original_depth_texels,original_height_texels,original_width_texels;
	/* actually rendered size in texels (zero until rendered) */
	int rendered_depth_texels, rendered_height_texels,rendered_width_texels;
	/* cropping of image in file */
	int crop_bottom_margin,crop_height,crop_left_margin,crop_width;
	/* texture display options */
	enum Texture_combine_mode combine_mode;
	enum Texture_compression_mode compression_mode;
	enum Texture_filter_mode filter_mode;
	enum Texture_wrap_mode wrap_mode;
	struct Colour combine_colour;
	GLfloat combine_alpha;
	GLfloat mipmap_level_of_detail_bias;
	/* following controls how a texture is downsampled to fit texture hardware */
	enum Texture_resize_filter_mode resize_filter_mode;

	struct Graphics_buffer *graphics_buffer;

#if defined (OPENGL_API)
	GLuint display_list;
	GLuint texture_id;
#endif /* defined (OPENGL_API) */
	/* flag to say if the display list is up to date */
	Texture_compile_state display_list_current;
	/* the number of structures that point to this texture.  The texture
		cannot be destroyed while this is greater than 0 */

	/* Texture_tiling */
	int allow_texture_tiling;  /* Is texture tiling allowed for this texture */
	struct Texture_tiling *texture_tiling;  /* This structure is non-null if a texture tiling has been applied */

	/* Store the properties */
	struct LIST(Texture_property) *property_list;

	int access_count;
}; /* struct Texture */
/*
Module functions
----------------
*/

#if defined (OPENGL_API)
static GLenum Texture_get_target_enum(Texture *texture)
{
	GLenum texture_target = GL_TEXTURE_1D;

	switch (texture->dimension)
	{
		case 1:
		{
			texture_target = GL_TEXTURE_1D;
		} break;
		case 2:
		{
			texture_target = GL_TEXTURE_2D;
		} break;
		case 3:
		{
	#if defined (GL_VERSION_1_2) || defined (GL_EXT_texture3D)
			if (
	#  if defined (GL_VERSION_1_2)
				Graphics_library_check_extension(GL_VERSION_1_2)
	#    if defined (GL_EXT_texture3D)
				||
	#    endif /* defined (GL_EXT_texture3D) */
	#  endif /* defined (GL_VERSION_1_2) */
	#  if defined (GL_EXT_texture3D)
				Graphics_library_check_extension(GL_EXT_texture3D)
	#  endif /* defined (GL_EXT_texture3D) */
				)
			{
				texture_target = GL_TEXTURE_3D;
			}
			else
			{
	#endif /* defined (GL_VERSION_1_2) || defined (GL_EXT_texture3D) */
				display_message(ERROR_MESSAGE,
					"Texture_get_target_enum.  "
					"3D textures not supported on this display.");
				texture_target=0;
	#if defined (GL_VERSION_1_2) || defined (GL_EXT_texture3D)
			}
	#endif /* defined (GL_VERSION_1_2) || defined (GL_EXT_texture3D) */
		} break;
	}
	return (texture_target);
}
#endif // defined (OPENGL_API)

static struct Texture_property *CREATE(Texture_property)(
	const char *name, const char *value)
/*******************************************************************************
LAST MODIFIED : 25 October 2007

DESCRIPTION :
==============================================================================*/
{
	const char *name_copy, *value_copy;
	struct Texture_property *property;

	ENTER(CREATE(Texture_property));
	if (ALLOCATE(property, struct Texture_property, 1) &&
		(name_copy = duplicate_string(name)) &&
		(value_copy = duplicate_string(value)))
	{
		property->name = name_copy;
		property->value = value_copy;
		property->access_count = 0;
	}
	else
	{
		display_message(ERROR_MESSAGE,"CREATE(Texture_property).  "
			"Unable to allocate memory for property list structure");
		property = (struct Texture_property *)NULL;
	}
	LEAVE;

	return (property);
} /* CREATE(Texture_property) */

int DESTROY(Texture_property)(struct Texture_property **property_address)
/*******************************************************************************
LAST MODIFIED : 25 October 2007

DESCRIPTION :
==============================================================================*/
{
	int return_code;
	struct Texture_property *property;

	ENTER(DESTROY(Texture_property));
	if (property_address && (property = *property_address))
	{
		if (property->access_count <= 0)
		{
			if (property->name)
			{
				DEALLOCATE(property->name);
			}
			if (property->value)
			{
				DEALLOCATE(property->value);
			}
			DEALLOCATE(*property_address);
			return_code = 1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"DESTROY(Texture_property).  Destroy called when access count > 0.");
			*property_address = (struct Texture_property *)NULL;
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(Texture_property).  Invalid arguments.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Texture_property) */

DECLARE_OBJECT_FUNCTIONS(Texture_property)
FULL_DECLARE_INDEXED_LIST_TYPE(Texture_property);
DECLARE_INDEXED_LIST_MODULE_FUNCTIONS(Texture_property,name,const char *,strcmp)
DECLARE_INDEXED_LIST_FUNCTIONS(Texture_property)
DECLARE_FIND_BY_IDENTIFIER_IN_INDEXED_LIST_FUNCTION(Texture_property,name,
	const char *, strcmp)

#if defined (OPENGL_API)
static struct Texture_tiling *CREATE(Texture_tiling)(int dimension)
/*******************************************************************************
LAST MODIFIED : 23 November 2007

DESCRIPTION :
==============================================================================*/
{
	struct Texture_tiling *texture_tiling;

	ENTER(CREATE(Texture_tiling));
	if (ALLOCATE(texture_tiling, struct Texture_tiling, 1) &&
		ALLOCATE(texture_tiling->texture_tiles, int, dimension) &&
		ALLOCATE(texture_tiling->tile_size, int, dimension) &&
		ALLOCATE(texture_tiling->tile_coordinate_range, ZnReal,
			dimension) &&
		ALLOCATE(texture_tiling->image_coordinate_range, ZnReal,
			dimension) &&
		ALLOCATE(texture_tiling->coordinate_scaling, ZnReal,
			dimension))
	{
		texture_tiling->total_tiles = 0;
		texture_tiling->dimension = dimension;
		texture_tiling->texture_ids = (unsigned int *)NULL;
		texture_tiling->overlap = 0;
		texture_tiling->texture_target = 0;
		texture_tiling->access_count = 0;
	}
	else
	{
		display_message(ERROR_MESSAGE,"CREATE(Texture_tiling).  "
			"Unable to allocate memory for texture_tiling list structure");
		texture_tiling = (struct Texture_tiling *)NULL;
	}
	LEAVE;

	return (texture_tiling);
} /* CREATE(Texture_tiling) */
#endif /* defined (OPENGL_API) */

static int DESTROY(Texture_tiling)(struct Texture_tiling **texture_tiling_address)
/*******************************************************************************
LAST MODIFIED : 22 November 2007

DESCRIPTION :
==============================================================================*/
{
	int return_code;
	struct Texture_tiling *texture_tiling;

	ENTER(DESTROY(Texture_tiling));
	if (texture_tiling_address && (texture_tiling = *texture_tiling_address))
	{
		if (texture_tiling->texture_ids)
		{
#if defined (OPENGL_API)
			glDeleteTextures(texture_tiling->total_tiles, texture_tiling->texture_ids);
#endif /* defined (OPENGL_API) */
			DEALLOCATE(texture_tiling->texture_ids);
		}
		DEALLOCATE(texture_tiling->tile_size);
		DEALLOCATE(texture_tiling->texture_tiles);
		DEALLOCATE(texture_tiling->tile_coordinate_range);
		DEALLOCATE(texture_tiling->image_coordinate_range);
		DEALLOCATE(texture_tiling->coordinate_scaling);
		DEALLOCATE(*texture_tiling_address);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(Texture_tiling).  Invalid arguments.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Texture_tiling) */

DECLARE_OBJECT_FUNCTIONS(Texture_tiling)


int Texture_storage_type_get_number_of_components(
	enum Texture_storage_type storage)
/*******************************************************************************
LAST MODIFIED : 28 February 2002

DESCRIPTION :
Returns the number of components used per texel for <storage> type.
==============================================================================*/
{
	int return_code;

	ENTER(Texture_storage_type_get_number_of_components);
	switch (storage)
	{
		case TEXTURE_LUMINANCE:
		{
			return_code = 1;
		} break;
		case TEXTURE_LUMINANCE_ALPHA:
		{
			return_code = 2;
		} break;
		case TEXTURE_RGB:
		case TEXTURE_BGR:
		{
			return_code = 3;
		} break;
		case TEXTURE_RGBA:
		case TEXTURE_ABGR:
		{
			return_code = 4;
		} break;
		default:
		{
			display_message(ERROR_MESSAGE,
				"Texture_storage_type_get_number_of_components.  "
				"Texture storage type unknown");
			return_code = 0;
		} break;
	}
	LEAVE;

	return (return_code);
} /* Texture_get_number_of_bytes_per_texel_from_storage_type */

#if defined (OPENGL_API)
static int Texture_get_type_and_format_from_storage_type(
	enum Texture_storage_type storage,int number_of_bytes_per_component,
	GLenum *type,GLenum *format)
/*******************************************************************************
LAST MODIFIED : 25 August 1999

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(Texture_get_type_and_format_from_storage_type);
	return_code=1;
	switch (storage)
	{
		case TEXTURE_LUMINANCE:
		{
			if (number_of_bytes_per_component == 2)
			{
				*format=GL_LUMINANCE;
			}
			else
			{
				*format=GL_LUMINANCE;
			}
		} break;
		case TEXTURE_LUMINANCE_ALPHA:
		{
			if (number_of_bytes_per_component == 2)
			{
				*format=GL_LUMINANCE16_ALPHA16;
			}
			else
			{
				*format=GL_LUMINANCE_ALPHA;
			}
		} break;
		case TEXTURE_RGB:
		{
			*format=GL_RGB;
		} break;
		case TEXTURE_BGR:
		{
			*format=GL_BGR;
		} break;
		case TEXTURE_RGBA:
		{
			*format=GL_RGBA;
		} break;
		case TEXTURE_ABGR:
		{
#if defined (GL_EXT_abgr)
			*format=GL_ABGR_EXT;
#else /* defined (GL_EXT_abgr) */
			display_message(ERROR_MESSAGE,"Texture_get_type_and_format_from_storage_type.  abgr storage not supported in this compilation");
			return_code=0;
#endif /* defined (GL_EXT_abgr) */
		} break;
		default:
		{
			display_message(ERROR_MESSAGE,
				"Texture_get_type_and_format_from_storage_type."
				"  Texture storage type unknown");
			return_code=0;
		} break;
	}
	if (return_code)
	{
		switch(number_of_bytes_per_component)
		{
			case 1:
			{
				*type=GL_UNSIGNED_BYTE;
			} break;
			case 2:
			{
				*type=GL_UNSIGNED_SHORT;
			} break;
			case 4:
			{
				*type=GL_FLOAT;
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"Texture_get_type_and_format_from_storage_type."
					"  Must be one or two bytes per component");
				return_code=0;
			} break;
		}
	}
	LEAVE;

	return (return_code);
} /* Texture_get_type_and_format_from_storage_type */
#endif /* defined (OPENGL_API) */

#if defined (OPENGL_API)
static int Texture_get_hardware_storage_format(
	enum Texture_compression_mode compression_mode, int number_of_components,
	int number_of_bytes_per_component)
/*******************************************************************************
LAST MODIFIED : 8 August 2002

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(Texture_get_hardware_storage_format);
	return_code=1;
	switch (compression_mode)
	{
		case TEXTURE_UNCOMPRESSED:
		{
			switch (number_of_components)
			{
				case 1:
				{
					if (number_of_bytes_per_component == 2)
						return_code = GL_LUMINANCE16;
					else
						return_code = number_of_components;
				} break;
				case 2:
				{
					if (number_of_bytes_per_component == 2)
						return_code = GL_LUMINANCE16_ALPHA16;
					else
						return_code = number_of_components;
				} break;
				case 3:
				{
					return_code = number_of_components;
				} break;
				case 4:
				{
					return_code = number_of_components;
				} break;
				default:
				{
					display_message(WARNING_MESSAGE, "Texture_get_hardware_storage_format.  "
						"Texture compression not supported for this number of components.");
					return_code = number_of_components;
				} break;
			}
		} break;
		case TEXTURE_COMPRESSED_UNSPECIFIED:
		{
#if defined (GL_ARB_texture_compression)
			if (Graphics_library_check_extension(GL_ARB_texture_compression))
			{
				switch (number_of_components)
				{
					case 1:
					{
						return_code = GL_COMPRESSED_LUMINANCE_ARB;
					} break;
					case 2:
					{
						return_code = GL_COMPRESSED_LUMINANCE_ALPHA_ARB;
					} break;
					case 3:
					{
						return_code = GL_COMPRESSED_RGB_ARB;
					} break;
					case 4:
					{
						return_code = GL_COMPRESSED_RGBA_ARB;
					} break;
					default:
					{
						display_message(WARNING_MESSAGE, "Texture_get_hardware_storage_format.  "
							"Texture compression not supported for this number of components.");
						return_code = number_of_components;
					} break;
				}
			}
			else
			{
#endif /* defined (GL_ARB_texture_compression) */
				display_message(WARNING_MESSAGE, "Texture_get_hardware_storage_format.  "
					"Texture compression not supported on this hardware.");
				return_code = number_of_components;
#if defined (GL_ARB_texture_compression)
			}
#endif /* defined (GL_ARB_texture_compression) */
		} break;
		default:
		{
			display_message(ERROR_MESSAGE, "Texture_get_hardware_storage_format.  "
				"Invalid texture compression, using uncompressed.");
			return_code = number_of_components;
		} break;
	}
	LEAVE;

	return (return_code);
} /* Texture_get_hardware_storage_format */
#endif /* defined (OPENGL_API) */

#if defined (OPENGL_API)
static int direct_render_Texture_environment(struct Texture *texture)
/*******************************************************************************
LAST MODIFIED : 11 February 2002

DESCRIPTION :
Directly outputs the commands that set the environment for this <texture>.
This is separated from the commands for the texture itself as these are the
only commands required when the texture is compiled and bound with the
GL_EXT_texture_object extension.
==============================================================================*/
{
	int return_code;
#if defined (OPENGL_API)
	GLfloat values[4];
	GLint number_of_components;
	gtMatrix texture_coordinate_transform;
#endif /* defined (OPENGL_API) */

	ENTER(direct_render_Texture_environment);
	return_code=0;
	if (texture)
	{
		return_code=1;
#if defined (OPENGL_API)
		texture_coordinate_transform[0][0] = texture->original_width_texels/
			(texture->width_texels*texture->width);
		texture_coordinate_transform[1][0]=0.;
		texture_coordinate_transform[2][0]=0.;
		texture_coordinate_transform[3][0]=0.;
		texture_coordinate_transform[0][1]=0.;
		texture_coordinate_transform[1][1] = texture->original_height_texels/
			(texture->height_texels*texture->height);
		texture_coordinate_transform[2][1]=0.;
		texture_coordinate_transform[3][1]=0.;
		texture_coordinate_transform[0][2]=0.;
		texture_coordinate_transform[1][2]=0.;
		texture_coordinate_transform[2][2] = texture->original_depth_texels/
			(texture->depth_texels*texture->depth);
		texture_coordinate_transform[3][2]=0.;
		texture_coordinate_transform[0][3]=0.;
		texture_coordinate_transform[1][3]=0.;
		texture_coordinate_transform[2][3]=0.;
		texture_coordinate_transform[3][3]=1.;
		/* set the texture coordinate transformation */
		glMatrixMode(GL_TEXTURE);
		wrapperLoadCurrentMatrix(&texture_coordinate_transform);
		glMatrixMode(GL_MODELVIEW);
		number_of_components = Texture_storage_type_get_number_of_components(texture->storage);
		values[0]=(texture->combine_colour).red;
		values[1]=(texture->combine_colour).green;
		values[2]=(texture->combine_colour).blue;
		values[3]=texture->combine_alpha;
		/* specify how the texture is combined */
		switch (texture->combine_mode)
		{
			case TEXTURE_BLEND:
			{
				glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_BLEND);
			} break;
			case TEXTURE_DECAL:
			{
				if (number_of_components<3)
				{
					glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
				}
				else
				{
					glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
				}
			} break;
			case TEXTURE_MODULATE:
			{
				glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
			} break;
			case TEXTURE_ADD:
			{
#if defined (GL_VERSION_1_3)
				if (Graphics_library_check_extension(GL_VERSION_1_3))
				{
					glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_ADD);
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"direct_render_Texture_environment.  "
						"Texture environment combine mode ADD requires OpenGL 1.3 or better "
						"which is not provided on this display.");
					return_code=0;
				}
#else /* defined (GL_VERSION_1_3) */
				display_message(ERROR_MESSAGE,
					"direct_render_Texture_environment.  "
					"Texture environment combine mode ADD requires OpenGL 1.3 or better "
					"which was not compiled into this executable.");
					return_code=0;
#endif /* defined (GL_VERSION_1_3) */
			} break;
			case TEXTURE_ADD_SIGNED:
			{
#if defined (GL_VERSION_1_3)
				if (Graphics_library_check_extension(GL_VERSION_1_3))
				{
					glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE);
					glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB,GL_ADD_SIGNED);
					glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA,GL_ADD_SIGNED);
					glTexEnvf(GL_TEXTURE_ENV,GL_RGB_SCALE,1.0);
					glTexEnvi(GL_TEXTURE_ENV,GL_SRC0_RGB,GL_TEXTURE);
					glTexEnvi(GL_TEXTURE_ENV,GL_SRC1_RGB,GL_PREVIOUS);
					glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_RGB,GL_SRC_COLOR);
					glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_RGB,GL_SRC_COLOR);
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"direct_render_Texture_environment.  "
						"Texture environment combine mode ADD_SIGNED requires OpenGL 1.3 or better "
						"which is not provided on this display.");
					return_code=0;
				}
#else /* defined (GL_VERSION_1_3) */
				display_message(ERROR_MESSAGE,
					"direct_render_Texture_environment.  "
					"Texture environment combine mode ADD_SIGNED requires OpenGL 1.3 or better "
					"which was not compiled into this executable.");
					return_code=0;
#endif /* defined (GL_VERSION_1_3) */
			} break;
			case TEXTURE_MODULATE_SCALE_4:
			{
#if defined (GL_VERSION_1_3)
				if (Graphics_library_check_extension(GL_VERSION_1_3))
				{
					/* Same as GL_MODULATE with a 4 times scaling */
					glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE);
					glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB,GL_MODULATE);
					glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA,GL_MODULATE);
					glTexEnvf(GL_TEXTURE_ENV,GL_RGB_SCALE,4.0);
					glTexEnvi(GL_TEXTURE_ENV,GL_SRC0_RGB,GL_TEXTURE);
					glTexEnvi(GL_TEXTURE_ENV,GL_SRC1_RGB,GL_PREVIOUS);
					glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_RGB,GL_SRC_COLOR);
					glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_RGB,GL_SRC_COLOR);
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"direct_render_Texture_environment.  "
						"Texture environment combine mode MODULATE_SCALE_4 requires OpenGL 1.3 or better "
						"which is not provided on this display.");
					return_code=0;
				}
#else /* defined (GL_VERSION_1_3) */
				display_message(ERROR_MESSAGE,
					"direct_render_Texture_environment.  "
					"Texture environment combine mode MODULATE_SCALE_4 requires OpenGL 1.3 or better "
					"which was not compiled into this executable.");
					return_code=0;
#endif /* defined (GL_VERSION_1_3) */
			} break;
			case TEXTURE_BLEND_SCALE_4:
			{
#if defined (GL_VERSION_1_3)
				if (Graphics_library_check_extension(GL_VERSION_1_3))
				{
					/* Same as GL_BLEND for RGB texture with a 4 times scaling */
					glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE);
					glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB,GL_INTERPOLATE);
					glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA,GL_MODULATE);
					glTexEnvf(GL_TEXTURE_ENV,GL_RGB_SCALE,4.0);
					glTexEnvi(GL_TEXTURE_ENV,GL_SRC0_RGB,GL_CONSTANT);
					glTexEnvi(GL_TEXTURE_ENV,GL_SRC1_RGB,GL_PREVIOUS);
					glTexEnvi(GL_TEXTURE_ENV,GL_SRC2_RGB,GL_TEXTURE);
					glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_RGB,GL_SRC_COLOR);
					glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_RGB,GL_SRC_COLOR);
					glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND2_RGB,GL_SRC_COLOR);
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"direct_render_Texture_environment.  "
						"Texture environment combine mode MODULATE_SCALE_4 requires OpenGL 1.3 or better "
						"which is not provided on this display.");
					return_code=0;
				}
#else /* defined (GL_VERSION_1_3) */
				display_message(ERROR_MESSAGE,
					"direct_render_Texture_environment.  "
					"Texture environment combine mode MODULATE_SCALE_4 requires OpenGL 1.3 or better "
					"which was not compiled into this executable.");
					return_code=0;
#endif /* defined (GL_VERSION_1_3) */
			} break;
			case TEXTURE_ADD_SCALE_4:
			{
#if defined (GL_VERSION_1_3)
				if (Graphics_library_check_extension(GL_VERSION_1_3))
				{
					glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE);
					glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB,GL_ADD);
					glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA,GL_MODULATE);
					glTexEnvf(GL_TEXTURE_ENV,GL_RGB_SCALE,4.0);
					glTexEnvi(GL_TEXTURE_ENV,GL_SRC0_RGB,GL_PREVIOUS);
					glTexEnvi(GL_TEXTURE_ENV,GL_SRC1_RGB,GL_TEXTURE);
					glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_RGB,GL_SRC_COLOR);
					glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_RGB,GL_SRC_COLOR);
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"direct_render_Texture_environment.  "
						"Texture environment combine mode MODULATE_SCALE_4 requires OpenGL 1.3 or better "
						"which is not provided on this display.");
					return_code=0;
				}
#else /* defined (GL_VERSION_1_3) */
				display_message(ERROR_MESSAGE,
					"direct_render_Texture_environment.  "
					"Texture environment combine mode MODULATE_SCALE_4 requires OpenGL 1.3 or better "
					"which was not compiled into this executable.");
					return_code=0;
#endif /* defined (GL_VERSION_1_3) */
			} break;
			case TEXTURE_SUBTRACT_SCALE_4:
			{
#if defined (GL_VERSION_1_3)
				if (Graphics_library_check_extension(GL_VERSION_1_3))
				{
					glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE);
					glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB,GL_SUBTRACT);
					glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA,GL_MODULATE);
					glTexEnvf(GL_TEXTURE_ENV,GL_RGB_SCALE,4.0);
					glTexEnvi(GL_TEXTURE_ENV,GL_SRC0_RGB,GL_PREVIOUS);
					glTexEnvi(GL_TEXTURE_ENV,GL_SRC1_RGB,GL_TEXTURE);
					glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_RGB,GL_SRC_COLOR);
					glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_RGB,GL_SRC_COLOR);
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"direct_render_Texture_environment.  "
						"Texture environment combine mode MODULATE_SCALE_4 requires OpenGL 1.3 or better "
						"which is not provided on this display.");
					return_code=0;
				}
#else /* defined (GL_VERSION_1_3) */
				display_message(ERROR_MESSAGE,
					"direct_render_Texture_environment.  "
					"Texture environment combine mode MODULATE_SCALE_4 requires OpenGL 1.3 or better "
					"which was not compiled into this executable.");
					return_code=0;
#endif /* defined (GL_VERSION_1_3) */
			} break;
			case TEXTURE_SUBTRACT:
			{
#if defined (GL_VERSION_1_3)
				if (Graphics_library_check_extension(GL_VERSION_1_3))
				{
					glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE);
					glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB,GL_SUBTRACT);
					glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA,GL_MODULATE);
					glTexEnvi(GL_TEXTURE_ENV,GL_SRC0_RGB,GL_PREVIOUS);
					glTexEnvi(GL_TEXTURE_ENV,GL_SRC1_RGB,GL_TEXTURE);
					glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_RGB,GL_SRC_COLOR);
					glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_RGB,GL_SRC_COLOR);
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"direct_render_Texture_environment.  "
						"Texture environment combine mode MODULATE_SCALE_4 requires OpenGL 1.3 or better "
						"which is not provided on this display.");
					return_code=0;
				}
#else /* defined (GL_VERSION_1_3) */
				display_message(ERROR_MESSAGE,
					"direct_render_Texture_environment.  "
					"Texture environment combine mode MODULATE_SCALE_4 requires OpenGL 1.3 or better "
					"which was not compiled into this executable.");
					return_code=0;
#endif /* defined (GL_VERSION_1_3) */
			} break;
			case TEXTURE_INVERT_ADD_SCALE_4:
			{
#if defined (GL_VERSION_1_3)
				if (Graphics_library_check_extension(GL_VERSION_1_3))
				{
					glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE);
					glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB,GL_ADD);
					glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA,GL_MODULATE);
					glTexEnvf(GL_TEXTURE_ENV,GL_RGB_SCALE,4.0);
					glTexEnvi(GL_TEXTURE_ENV,GL_SRC0_RGB,GL_PREVIOUS);
					glTexEnvi(GL_TEXTURE_ENV,GL_SRC1_RGB,GL_TEXTURE);
					glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_RGB,GL_ONE_MINUS_SRC_COLOR);
					glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_RGB,GL_SRC_COLOR);
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"direct_render_Texture_environment.  "
						"Texture environment combine mode MODULATE_SCALE_4 requires OpenGL 1.3 or better "
						"which is not provided on this display.");
					return_code=0;
				}
#else /* defined (GL_VERSION_1_3) */
				display_message(ERROR_MESSAGE,
					"direct_render_Texture_environment.  "
					"Texture environment combine mode MODULATE_SCALE_4 requires OpenGL 1.3 or better "
					"which was not compiled into this executable.");
					return_code=0;
#endif /* defined (GL_VERSION_1_3) */
			} break;
			case TEXTURE_INVERT_SUBTRACT_SCALE_4:
			{
#if defined (GL_VERSION_1_3)
				if (Graphics_library_check_extension(GL_VERSION_1_3))
				{
					glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE);
					glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB,GL_SUBTRACT);
					glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA,GL_MODULATE);
					glTexEnvf(GL_TEXTURE_ENV,GL_RGB_SCALE,4.0);
					glTexEnvi(GL_TEXTURE_ENV,GL_SRC0_RGB,GL_PREVIOUS);
					glTexEnvi(GL_TEXTURE_ENV,GL_SRC1_RGB,GL_TEXTURE);
					glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_RGB,GL_ONE_MINUS_SRC_COLOR);
					glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_RGB,GL_SRC_COLOR);
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"direct_render_Texture_environment.  "
						"Texture environment combine mode MODULATE_SCALE_4 requires OpenGL 1.3 or better "
						"which is not provided on this display.");
					return_code=0;
				}
#else /* defined (GL_VERSION_1_3) */
				display_message(ERROR_MESSAGE,
					"direct_render_Texture_environment.  "
					"Texture environment combine mode MODULATE_SCALE_4 requires OpenGL 1.3 or better "
					"which was not compiled into this executable.");
					return_code=0;
#endif /* defined (GL_VERSION_1_3) */
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"direct_render_Texture_environment.  "
					"Unknown texture environment combine mode.");
					return_code=0;
			} break;
		}
		glTexEnvfv(GL_TEXTURE_ENV,GL_TEXTURE_ENV_COLOR,values);
		switch (texture->dimension)
		{
			case 1:
			{
				glEnable(GL_TEXTURE_1D);
			} break;
			case 2:
			{
				glEnable(GL_TEXTURE_2D);
			} break;
			case 3:
			{
#if defined (GL_VERSION_1_2) || defined (GL_EXT_texture3D)
				if (
#  if defined (GL_VERSION_1_2)
					Graphics_library_check_extension(GL_VERSION_1_2)
#    if defined (GL_EXT_texture3D)
					||
#    endif /* defined (GL_EXT_texture3D) */
#  endif /* defined (GL_VERSION_1_2) */
#  if defined (GL_EXT_texture3D)
					Graphics_library_check_extension(GL_EXT_texture3D)
#  endif /* defined (GL_EXT_texture3D) */
					)
				{
					/* Note that while to strictly satisfy the GL_EXT_texture3D
						all these uses should have the EXT delimiter the SGI
						OpenGL is version 1.1 with GL_EXT_texture3D but supplies
						all the 1.2 compliant symbols, so the code is simpler with
						just one version */
					glEnable(GL_TEXTURE_3D);
				}
				else
				{
#endif /* defined (GL_VERSION_1_2) || defined (GL_EXT_texture3D)*/
					display_message(ERROR_MESSAGE,
						"direct_render_Texture_environment.  "
						"3D textures not supported on this display.");
					return_code=0;
#if defined (GL_VERSION_1_2) || defined (GL_EXT_texture3D)
				}
#endif /* defined (GL_VERSION_1_2) || (GL_EXT_texture3D)*/
			} break;
		}
#endif /* defined (OPENGL_API) */
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"direct_render_Texture_environment.  Missing texture");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* direct_render_Texture_environment */
#endif /* defined (OPENGL_API) */

#if defined (OPENGL_API)
static int Texture_get_hardware_reduction_or_tiling(struct Texture *texture,
   int *reductions, int &allow_texture_tiling, struct Texture_tiling **texture_tiling)
/*******************************************************************************
LAST MODIFIED : 19 November 2007

DESCRIPTION :
Queries the graphics hardware to determine how much the texture size must be
reduced to fit the available space. Returns the reduction factors <reductions>
which must be an array equal in size to the dimension of the texture.
The function returns 0 if the texture cannot be loaded at all, 1 if no
reduction is required, 2 if the texture can be loaded if reduced according
to the <reductions> or 3 if the texture can be loaded if broken into multiple
tiles (and <texture_tiling> wasn't NULL.
==============================================================================*/
{
#if defined (OPENGL_API)
	char *cmiss_max_texture_size;
	int max_texture_size_int, next_reduction, pixel_size = 0, return_code;
	GLenum format = GL_COLOR_INDEX, type = GL_INT;
	GLint max_texture_size, number_of_components, test_width,
		hardware_texture_format;
#endif /* defined (OPENGL_API) */

	ENTER(Texture_get_hardware_reduction_or_tiling);
	if (texture)
	{
#if defined (OPENGL_API)
		number_of_components =
			Texture_storage_type_get_number_of_components(texture->storage);
		Texture_get_type_and_format_from_storage_type(texture->storage,
			texture->number_of_bytes_per_component, &type, &format);
		hardware_texture_format = Texture_get_hardware_storage_format(
			texture->compression_mode, number_of_components,
			texture->number_of_bytes_per_component);

		max_texture_size = 0;
		if (texture->dimension > 2)
		{
			cmiss_max_texture_size = getenv("CMZN_MAX_3D_TEXTURE_SIZE");
			if (cmiss_max_texture_size)
			{
				if (sscanf(cmiss_max_texture_size, "%d", &max_texture_size_int))
				{
					max_texture_size = max_texture_size_int;
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"Texture_get_hardware_reduction_or_tiling.  "
						"Unable to parse environment variable CMZN_MAX_3D_TEXTURE_SIZE: %s",
						cmiss_max_texture_size);
				}
			}
			else
			{
				glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max_texture_size);
			}
		}
		else
		{
#if defined (WIN32_SYSTEM)
		  char env_buffer[1024];
		  cmiss_max_texture_size = NULL;
		  if (GetEnvironmentVariable("CMZN_MAX_TEXTURE_SIZE",
			  env_buffer, sizeof(env_buffer))
			  && (cmiss_max_texture_size = env_buffer))
#else /* defined (WIN32_SYSTEM) */
		  cmiss_max_texture_size = getenv("CMZN_MAX_TEXTURE_SIZE");
		  if (cmiss_max_texture_size)
#endif /* defined (WIN32_SYSTEM) */
		  {
				if (sscanf(cmiss_max_texture_size, "%d", &max_texture_size_int))
				{
					max_texture_size = max_texture_size_int;
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"Texture_get_hardware_reduction_or_tiling.  "
						"Unable to parse environment variable CMZN_MAX_TEXTURE_SIZE: %s",
						cmiss_max_texture_size);
				}
			}
			else
			{
				glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
			}
		}

		if (max_texture_size > 0)
		{
			test_width = 0;
			return_code = 1;
			next_reduction = 0;
			if (texture->dimension > 0)
			{
				reductions[0] = 1;
				while ((texture->width_texels / reductions[0]) > max_texture_size)
				{
					reductions[0] *= 2;
					return_code = 2;
				}
			}
			if (texture->dimension > 1)
			{
				reductions[1] = 1;
				while ((texture->height_texels / reductions[1]) > max_texture_size)
				{
					reductions[1] *= 2;
					return_code = 2;
				}
			}
			if (texture->dimension > 2)
			{
				reductions[2] = 1;
				while ((texture->depth_texels / reductions[2]) > max_texture_size)
				{
					reductions[2] *= 2;
					return_code = 2;
				}
			}
			while ((test_width == 0) && return_code)
			{
#if defined (DEBUG_CODE)
				printf ("texture_dimension %d\n", texture->dimension);
				printf ("hardware_texture_format %d\n", hardware_texture_format);
				printf ("format %d\n", format);
				printf ("type %d\n", type);
				if (texture->dimension > 0)
				{
					printf ("width %d\n", texture->width_texels);
					printf ("reductions[0] %d\n", reductions[0]);
				}
				if (texture->dimension > 1)
				{
					printf ("height %d\n", texture->height_texels);
					printf ("reductions[1] %d\n", reductions[1]);
				}
				if (texture->dimension > 2)
				{
					printf ("depth %d\n", texture->depth_texels);
					printf ("reductions[2] %d\n", reductions[2]);
				}
				printf ("next_reduction %d\n\n", next_reduction);
#endif /* defined (DEBUG_CODE) */
				switch (texture->dimension)
				{
					case 1:
					{
						glTexImage1D(GL_PROXY_TEXTURE_1D, (GLint)0, hardware_texture_format,
							(GLint)(texture->width_texels/reductions[0]), (GLint)0,
							format, type, (GLvoid *)(texture->image));
						glGetTexLevelParameteriv(GL_PROXY_TEXTURE_1D, (GLint)0,
							GL_TEXTURE_WIDTH, &test_width);
					} break;
					case 2:
					{
						glTexImage2D(GL_PROXY_TEXTURE_2D, (GLint)0, hardware_texture_format,
							(GLint)(texture->width_texels/reductions[0]),
							(GLint)(texture->height_texels/reductions[1]), (GLint)0,
							format, type, (GLvoid *)(texture->image));
						glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, (GLint)0,
							GL_TEXTURE_WIDTH, &test_width);
					} break;
					case 3:
					{
#if defined (GL_VERSION_1_2) || defined (GL_EXT_texture3D)
						if (
#  if defined (GL_VERSION_1_2)
							Graphics_library_check_extension(GL_VERSION_1_2)
#    if defined (GL_EXT_texture3D)
							||
#    endif /* defined (GL_EXT_texture3D) */
#  endif /* defined (GL_VERSION_1_2) */
#  if defined (GL_EXT_texture3D)
							Graphics_library_check_extension(GL_EXT_texture3D)
#  endif /* defined (GL_EXT_texture3D) */
							)
						{
							glTexImage3D(GL_PROXY_TEXTURE_3D, (GLint)0, hardware_texture_format,
								(GLint)(texture->width_texels/reductions[0]),
								(GLint)(texture->height_texels/reductions[1]),
								(GLint)(texture->depth_texels/reductions[2]), (GLint)0,
								format, type, NULL);
							glGetTexLevelParameteriv(GL_PROXY_TEXTURE_3D, (GLint)0,
								GL_TEXTURE_WIDTH, &test_width);
						}
						else
						{
#endif /* defined (GL_VERSION_1_2) || defined (GL_EXT_texture3D) */
							display_message(ERROR_MESSAGE,
								"Texture_get_hardware_reduction_or_tiling.  "
								"3D textures not supported on this display.");
							return_code=0;
#if defined (GL_VERSION_1_2) || defined (GL_EXT_texture3D)
						}
#endif /* defined (GL_VERSION_1_2) || defined (GL_EXT_texture3D) */
					} break;
					default:
					{
						display_message(ERROR_MESSAGE,
							"Texture_get_hardware_reduction_or_tiling.  Invalid texture dimension");
						return_code = 0;
					} break;
				}
				if (0 == test_width)
				{
					/* Halve the texture in one dimension */
					reductions[next_reduction] *= 2;
					return_code = 2;
					switch (next_reduction)
					{
						case 0:
						{
							pixel_size = texture->width_texels;
						} break;
						case 1:
						{
							pixel_size = texture->height_texels;
						} break;
						case 2:
						{
							pixel_size = texture->depth_texels;
						} break;
					}
					if (reductions[next_reduction] >= pixel_size)
					{
						/* The proxy texture says that the texture will not load,
							this is probably a faulty OpenGL implementation,
							try the max texture size instead. */
						return_code = 0;
					}
					next_reduction++;
					if (next_reduction == texture->dimension)
					{
						next_reduction = 0;
					}
				}
			}
#if defined (DEBUG_CODE)
			{
				int i;
				printf(" texture.c : max_texture_size %d\n", max_texture_size);
				for (i = 0 ; i < texture->dimension ; i++)
				{
					printf(" texture.c : reduction[%d] = %d\n", i, reductions[i]);
				}
				printf("\n");
			}
#endif /* defined (DEBUG_CODE) */
			if (texture->texture_tiling)
			{
				DEACCESS(Texture_tiling)(&texture->texture_tiling);
			}
			if ((2 == return_code) && texture->allow_texture_tiling &&
				/* Allowed according to the renderer (compilation_context)*/
				allow_texture_tiling)
			{
				int i, number_of_tiles;

				/* Tile instead of reduce (initially assume we can load in
					multiple textures of the proxy size which isn't necessarily
					true, need to test that too). */
				return_code = 3;
				*texture_tiling = CREATE(Texture_tiling)(texture->dimension);
				(*texture_tiling)->texture_target = Texture_get_target_enum(texture);
				switch (texture->filter_mode)
				{
					/* These overlaps are now added to each tile, so the total overlap will
					 * be twice these numbers. */
					case TEXTURE_LINEAR_FILTER:
					{
						(*texture_tiling)->overlap = 1;
					} break;
					case TEXTURE_LINEAR_MIPMAP_NEAREST_FILTER:
					case TEXTURE_LINEAR_MIPMAP_LINEAR_FILTER:
					{
						/* If we are mipmapping then the edge pixels can depend on many more
						 * of the border pixels, depending on how much you have zoomed out.
						 * If we are edge clamping this isn't really a problem.  If we aren't
						 * edge clamping then we need to increase the edge clamp in our tile.
						 * 30 is an arbitrary amount of zooming out.
						 */
						if (texture->wrap_mode == TEXTURE_CLAMP_EDGE_WRAP)
						{
							(*texture_tiling)->overlap = 1;
						}
						else
						{
							(*texture_tiling)->overlap = 15;
						}
					} break;
					default:
					{
						(*texture_tiling)->overlap = 0;
					} break;
				}
				for (i = 0 ; i < texture->dimension ; i++)
				{
					(*texture_tiling)->texture_tiles[i] = reductions[i];
					reductions[i] = 1;
				}
				if (texture->dimension > 0)
				{
					(*texture_tiling)->tile_size[0] =
						texture->width_texels/(*texture_tiling)->texture_tiles[0];
					(*texture_tiling)->tile_coordinate_range[0] =
						texture->width /(double)(*texture_tiling)->texture_tiles[0]
						* ((double)texture->width_texels /
						(double)texture->original_width_texels);
					(*texture_tiling)->coordinate_scaling[0] =
						(*texture_tiling)->texture_tiles[0];
					while (((*texture_tiling)->texture_tiles[0] > 1) &&
						(ceil((double)texture->width_texels/
						(double)((*texture_tiling)->tile_size[0] - 2 * (*texture_tiling)->overlap))
						> (*texture_tiling)->texture_tiles[0]))
					{
						(*texture_tiling)->texture_tiles[0]++;
					}
					(*texture_tiling)->image_coordinate_range[0] =
						texture->width;
				}
				if (texture->dimension > 1)
				{
					(*texture_tiling)->tile_size[1] =
						texture->height_texels/(*texture_tiling)->texture_tiles[1];
					(*texture_tiling)->tile_coordinate_range[1] =
						texture->height/(double)(*texture_tiling)->texture_tiles[1]
						* ((double)texture->height_texels /
						(double)texture->original_height_texels);
					(*texture_tiling)->coordinate_scaling[1] =
						(*texture_tiling)->texture_tiles[1];
					while (((*texture_tiling)->texture_tiles[1] > 1) &&
						(ceil((double)texture->height_texels/
						(double)((*texture_tiling)->tile_size[1] - 2 * (*texture_tiling)->overlap))
						> (*texture_tiling)->texture_tiles[1]))
					{
						(*texture_tiling)->texture_tiles[1]++;
					}
					(*texture_tiling)->image_coordinate_range[1] =
						texture->height;
				}
				if (texture->dimension > 2)
				{
					(*texture_tiling)->tile_size[2] =
						texture->depth_texels/(*texture_tiling)->texture_tiles[2];
					(*texture_tiling)->tile_coordinate_range[2] =
						texture->depth/(double)(*texture_tiling)->texture_tiles[2]
						* ((double)texture->depth_texels /
						(double)texture->original_depth_texels);
					(*texture_tiling)->coordinate_scaling[2] =
						(*texture_tiling)->texture_tiles[2];
					while (((*texture_tiling)->texture_tiles[2] > 1) &&
						(ceil((double)texture->depth_texels/
						(double)((*texture_tiling)->tile_size[2] - 2 * (*texture_tiling)->overlap))
						>= (*texture_tiling)->texture_tiles[2]))
					{
						(*texture_tiling)->texture_tiles[2]++;
					}
					(*texture_tiling)->image_coordinate_range[2] =
						texture->depth;
				}
				number_of_tiles = 1;
				for (i = 0 ; i < texture->dimension ; i++)
				{
					number_of_tiles *= (*texture_tiling)->texture_tiles[i];
				}
				(*texture_tiling)->total_tiles = number_of_tiles;
				ALLOCATE((*texture_tiling)->texture_ids, unsigned int, number_of_tiles);
				glGenTextures(number_of_tiles, (*texture_tiling)->texture_ids);
				texture->texture_tiling = ACCESS(Texture_tiling)(*texture_tiling);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Texture_get_hardware_reduction_or_tiling.  "
				"Maximum texture size less than 1");
			return_code=0;
		}

#else /* defined (OPENGL_API) */
		display_message(ERROR_MESSAGE,
			"Texture_get_hardware_reduction_or_tiling.  Only implemented for OpenGL");
		return_code = 0;
#endif /* defined (OPENGL_API) */
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_get_hardware_reduction_or_tiling.  Missing texture");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Texture_get_hardware_reduction_or_tiling */
#endif /* defined (OPENGL_API)*/

#if defined (OPENGL_API)
/***************************************************************************//**
 * Returns an resized image representing a scaling of the <source_image>
 *
 * @param texture The source_image and returned image are assumed to
 * be stored consistently with the format described by this texture
 * @param source_image The incoming image data downsampled to calculate the resized image
 * @param source_width_texels source_image sizes
 * @param source_height_texels source_image sizes
 * @param source_depth_texels source_image sizes
 * @param width_texels Desired returned image sizes
 * @param height_texels Desired returned image sizes
 * @param depth_texels Desired returned image sizes
 * @param resize_filter Filtering mode
 * @return Returns allocated new memory containing image if successful, otherwise NULL.
 */
static unsigned char *Texture_get_resized_image(struct Texture *texture,
	unsigned char *source_image, int source_width_texels,
	int source_height_texels, int source_depth_texels,
	int width_texels, int height_texels, int depth_texels,
	enum Texture_resize_filter_mode resize_filter)
{
	double accumulator, width_reduction, height_reduction, depth_reduction, number_of_accumulated_pixels;
	unsigned char *destination, *i_base, *image, *j_base, *source, *source2;
	GLfloat i_factor, j_factor, k_factor;
	int a, b, bytes_per_pixel, c,
		destination_row_width_bytes, i, j, k, n, number_of_bytes_per_component, number_of_components,
		padding_bytes, return_code, source_row_width_bytes;

	ENTER(Texture_get_resized_image);
	image = (unsigned char *)NULL;
	if (texture && (0 < width_texels) && (0 < height_texels) &&
		(0 < depth_texels))
	{
		return_code = 1;
		number_of_components =
			Texture_storage_type_get_number_of_components(texture->storage);
		number_of_bytes_per_component = texture->number_of_bytes_per_component;
		bytes_per_pixel = number_of_bytes_per_component * number_of_components;
		source_row_width_bytes = 4*((source_width_texels*bytes_per_pixel + 3)/4);
		destination_row_width_bytes = 4*((width_texels*bytes_per_pixel + 3)/4);
		if (ALLOCATE(image, unsigned char,
			depth_texels*height_texels*destination_row_width_bytes))
		{
			switch (resize_filter)
			{
				case TEXTURE_RESIZE_LINEAR_FILTER:
				{
					width_reduction = (double)source_width_texels / (double)width_texels;
					height_reduction = (double)source_height_texels / (double)height_texels;
					depth_reduction = (double)source_depth_texels / (double)depth_texels;
					number_of_accumulated_pixels =
						width_reduction*height_reduction*depth_reduction;
					source = source_image;
					destination = image;
					padding_bytes = (destination_row_width_bytes -
						width_texels*bytes_per_pixel);
					switch (number_of_bytes_per_component)
					{
						case 1:
						{
							for (i = 0; i < depth_texels; i++)
							{
								double source_depth_start = (double)i * depth_reduction;
								double source_depth_end = (double)(i + 1) * depth_reduction;
								int a_start = (int)floor(source_depth_start);
								int a_end = (int)ceil(source_depth_end);
								double xi_a_start = 1.0 - (source_depth_start - (double)a_start);
								double xi_a_end =  1.0 - ((double)a_end - source_depth_end);
								for (j = 0; j < height_texels; j++)
								{
									double source_height_start = (double)j * height_reduction;
									double source_height_end = (double)(j + 1) * height_reduction;
									int b_start = (int)floor(source_height_start);
									int b_end = (int)ceil(source_height_end);
									double xi_b_start = 1.0 - (source_height_start - (double)b_start);
									double xi_b_end =  1.0 - ((double)b_end - source_height_end);
									for (k = 0; k < width_texels; k++)
									{
										double source_width_start = (double)k * width_reduction;
										double source_width_end = (double)(k + 1) * width_reduction;
										int c_start = (int)floor(source_width_start);
										int c_end = (int)ceil(source_width_end);
										double xi_c_start = 1.0 - (source_width_start - (double)c_start);
										double xi_c_end =  1.0 - ((double)c_end - source_width_end);
										source = source_image + bytes_per_pixel * c_start +
											source_row_width_bytes * (b_start + source_height_texels * a_start);
										for (n = 0; n < number_of_components; n++)
										{
											source2 = source + n;
											accumulator = 0.0;
											for (a = a_start; a < a_end; a++)
											{
												double xi_a;
												if (a == a_start)
													xi_a = xi_a_start;
												else if (a == a_end - 1)
													xi_a = xi_a_end;
												else
													xi_a = 1.0;
												for (b = b_start; b < b_end; b++)
												{
													double xi_b;
													if (b == b_start)
														xi_b = xi_a * xi_b_start;
													else if (b == b_end - 1)
														xi_b = xi_a * xi_b_end;
													else
														xi_b = xi_a;
													for (c = c_start; c < c_end; c++)
													{
														double xi_c;
														if (c == c_start)
															xi_c = xi_b * xi_c_start;
														else if (c == c_end - 1)
															xi_c = xi_b * xi_c_end;
														else
															xi_c = xi_b;
														accumulator += *source2 * xi_c;
														source2 += bytes_per_pixel;
													}
													source2 += source_row_width_bytes - (c_end - c_start)*bytes_per_pixel;
												}
												source2 += (source_height_texels - (b_end - b_start)) *
													source_row_width_bytes;
											}
											*destination = lround(accumulator / number_of_accumulated_pixels);
											destination++;
#if defined (DEBUG_CODE)
//											printf(" %lf-%lf(%d-%d) %lf-%lf(%d-%d) %lf-%lf(%d-%d)  axi %lf %lf bxi %lf %lf cxi %lf %lf\n       number %lf %lf accumulation %lf\n",
//													source_depth_start, source_depth_end, a_start, a_end,
//													source_height_start, source_height_end, b_start, b_end,
//													source_width_start, source_width_end, c_start, c_end,
//													xi_a_start, xi_a_end, xi_b_start, xi_b_end, xi_c_start, xi_c_end,
//													number_of_accumulated_pixels, xi_accumulator, accumulator);
#endif // defined (DEBUG_CODE)
										}
									}
									if (0 < padding_bytes)
									{
										memset((void *)destination, 0, padding_bytes);
										destination += padding_bytes;
									}
								}
							}
						} break;
						case 2:
						{
							for (i = 0; i < depth_texels; i++)
							{
								double source_depth_start = (double)i * depth_reduction;
								double source_depth_end = (double)(i + 1) * depth_reduction;
								int a_start = (int)floor(source_depth_start);
								int a_end = (int)ceil(source_depth_end);
								double xi_a_start = 1.0 - (source_depth_start - (double)a_start);
								double xi_a_end =  1.0 - ((double)a_end - source_depth_end);
								for (j = 0; j < height_texels; j++)
								{
									double source_height_start = (double)j * height_reduction;
									double source_height_end = (double)(j + 1) * height_reduction;
									int b_start = (int)floor(source_height_start);
									int b_end = (int)ceil(source_height_end);
									double xi_b_start = 1.0 - (source_height_start - (double)b_start);
									double xi_b_end =  1.0 - ((double)b_end - source_height_end);
									for (k = 0; k < width_texels; k++)
									{
										double source_width_start = (double)k * width_reduction;
										double source_width_end = (double)(k + 1) * width_reduction;
										int c_start = (int)floor(source_width_start);
										int c_end = (int)ceil(source_width_end);
										double xi_c_start = 1.0 - (source_width_start - (double)c_start);
										double xi_c_end =  1.0 - ((double)c_end - source_width_end);
										source = source_image + bytes_per_pixel * c_start +
											source_row_width_bytes * (b_start + source_height_texels * a_start);
										for (n = 0; n < number_of_components; n++)
										{
											source2 = source + 2*n;
											accumulator = 0.0;
											for (a = a_start; a < a_end; a++)
											{
												double xi_a;
												if (a == a_start)
													xi_a = xi_a_start;
												else if (a == a_end - 1)
													xi_a = xi_a_end;
												else
													xi_a = 1.0;
												for (b = b_start; b < b_end; b++)
												{
													double xi_b;
													if (b == b_start)
														xi_b = xi_a * xi_b_start;
													else if (b == b_end - 1)
														xi_b = xi_a * xi_b_end;
													else
														xi_b = xi_a;
													for (c = c_start; c < c_end; c++)
													{
														double xi_c;
														if (c == c_start)
															xi_c = xi_b * xi_c_start;
														else if (c == c_end - 1)
															xi_c = xi_b * xi_c_end;
														else
															xi_c = xi_b;
														accumulator += (double)*((unsigned short *)source2) * xi_c;
														source2 += bytes_per_pixel;
													}
													source2 += source_row_width_bytes - (c_end - c_start)*bytes_per_pixel;
												}
												source2 += (source_height_texels - (b_end - b_start)) *
													source_row_width_bytes;
											}
											*((unsigned short *)destination) =
												lround(accumulator / number_of_accumulated_pixels);
											destination+=2;
										}
									}
									if (0 < padding_bytes)
									{
										memset((void *)destination, 0, padding_bytes);
										destination += padding_bytes;
									}
								}
							}
						} break;
						default:
						{
							display_message(ERROR_MESSAGE, "Texture_get_resized_image.  "
								"Only 1 or 2 bytes per component supported");
							return_code = 0;
						} break;
					}
				} break;
				case TEXTURE_RESIZE_NEAREST_FILTER:
				{
					i_factor = source_depth_texels / depth_texels;
					j_factor = source_height_texels / height_texels;
					k_factor = source_width_texels / width_texels;
					padding_bytes = (destination_row_width_bytes -
						width_texels*bytes_per_pixel);
					destination = image;
					for (i = 0; i < depth_texels; i++)
					{
						i_base = source_image + (long int)((0.5 + (double)i)*(double)i_factor) *
							source_height_texels * source_row_width_bytes;
						for (j = 0; j < height_texels; j++)
						{
							j_base = i_base + (long int)((0.5 + (double)j)*(double)j_factor) *
								source_row_width_bytes;
							for (k = 0; k < width_texels; k++)
							{
								source = j_base +
									(long int)((0.5 + (double)k)*(double)k_factor) * bytes_per_pixel;
								for (n = 0; n < bytes_per_pixel; n++)
								{
									*destination = *source;
									destination++;
									source++;
								}
							}
							if (0 < padding_bytes)
							{
								memset((void *)destination, 0, padding_bytes);
								destination += padding_bytes;
							}
						}
					}
				} break;
				default:
				{
					display_message(ERROR_MESSAGE,
						"Texture_get_resized_image.  Unknown resize filter mode");
					return_code = 0;
				} break;
			}
			if (!return_code)
			{
				DEALLOCATE(image);
				image = (unsigned char *)NULL;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Texture_get_resized_image.  Not enough memory for image");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_get_resized_image.  Missing texture");
	}
	LEAVE;

	return (image);
} /* Texture_get_resized_image */
#endif /* defined (OPENGL_API) */

#if defined (OPENGL_API)
static int Texture_expand_to_power_of_two(struct Texture *texture)
/*******************************************************************************
LAST MODIFIED : 18 July 2006

DESCRIPTION :
To support older OpenGL implementations we need to be able to resize texture
storage to power of two size in each direction.
==============================================================================*/
{
	static unsigned char fill_byte = 0;
	int bytes_per_pixel, k, j, number_of_components, original_padded_width_bytes,
		padded_width_bytes, return_code, width, height, depth;
	unsigned char *destination, *source, *texture_image;

	ENTER(Texture_expand_to_power_of_two);
	if (texture && (number_of_components =
			Texture_storage_type_get_number_of_components(texture->storage)))
	{
		return_code = 1;
		/* Must expand texture to be a power of two */
		width = 1;
		while (width < texture->width_texels)
		{
			width *= 2;
		}
		/* ensure height is a power of 2 */
		height = 1;
		while (height < texture->height_texels)
		{
			height *= 2;
		}
		/* ensure depth is a power of 2 */
		depth = 1;
		while (depth < texture->depth_texels)
		{
			depth *= 2;
		}
		if ((width != texture->width_texels) ||
			(height != texture->height_texels) ||
			(depth != texture->depth_texels))
		{
			switch (texture->dimension)
			{
				case 1:
				{
					display_message(WARNING_MESSAGE,
						"image width is not a power of 2.  "
						"Extending (%d) to (%d)", texture->width_texels, width);
				} break;
				case 2:
				{
					display_message(WARNING_MESSAGE,
						"image width and/or height not powers of 2.  "
						"Extending (%d,%d) to (%d,%d)",
						texture->width_texels, texture->height_texels,
						width, height);
				} break;
				case 3:
				{
					display_message(WARNING_MESSAGE,
						"image width, height and/or depth not powers of 2.  "
						"Extending (%d,%d,%d) to (%d,%d,%d)",
						texture->width_texels, texture->height_texels, texture->depth_texels,
						width, height, depth);
				} break;
			}
			bytes_per_pixel = number_of_components * texture->number_of_bytes_per_component;
			original_padded_width_bytes = 4*((texture->width_texels*bytes_per_pixel + 3)/4);
			padded_width_bytes = 4*((width*bytes_per_pixel + 3)/4);
			if (REALLOCATE(texture_image, texture->image, unsigned char,
					depth*height*padded_width_bytes))
			{
				source = texture_image + original_padded_width_bytes *
					texture->height_texels * texture->depth_texels;

				destination = texture_image + padded_width_bytes *
					height * depth;

				/* Start at the end to try and reduce overlaps */
				if (depth > texture->depth_texels)
				{
					destination -= padded_width_bytes * height *
						(depth - texture->depth_texels);
					memset(destination, fill_byte,
						padded_width_bytes * height *
						(depth - texture->depth_texels));
				}
				for (k = texture->depth_texels - 1; k >= 0 ; k--)
				{
					if (height > texture->height_texels)
					{
						destination -= padded_width_bytes *
							(height - texture->height_texels);
						memset(destination, fill_byte,
							padded_width_bytes * (height - texture->height_texels));
					}

					for (j = texture->height_texels - 1; j >= 0 ; j--)
					{
						source -= original_padded_width_bytes;
						destination -= padded_width_bytes;

						/* Use memmove as we may be overlapping */
						memmove(destination, source, original_padded_width_bytes);

						if (padded_width_bytes > original_padded_width_bytes)
						{
							/* Set the rest of the row to empty */
							memset(destination + original_padded_width_bytes,
								fill_byte, padded_width_bytes - original_padded_width_bytes);
						}
					}
				}

				texture->image = texture_image;
				texture->width_texels = width;
				texture->height_texels = height;
				texture->depth_texels = depth;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_expand_to_power_of_two.  Missing texture");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Texture_expand_to_power_of_two */
#endif /* defined (OPENGL_API)*/

#if defined (OPENGL_API)
static int Texture_get_image_tile(struct Texture *texture,
	unsigned char **tile_image, struct Texture_tiling *texture_tiling,
	int tile_number)
/*******************************************************************************
LAST MODIFIED : 19 November 2007

DESCRIPTION :
Creates a sub_image based on <tile_number> and <texture_tiling> which
is a part of the total texture.  If <tile_image> initially points at a NULL
then it is allocated, otherwise it is assumed large enough for the image tile
and overwritten.
==============================================================================*/
{
	int bytes_per_pixel, copy_row_width_bytes = 0, end_x = 0, end_y = 0, end_z = 0, i,
		number_of_components, number_of_bytes_per_component,
		pad_x_start = 0, pad_x_end = 0, pad_y_start = 0, pad_y_end = 0,
		pad_z_start = 0, pad_z_end = 0, return_code, source_row_width_bytes,
		tile_row_width_bytes, start_x = 0, start_y = 0, start_z = 0,
		tile_x, tile_y, tile_z, y, z;
	Triple cropped_size;
	unsigned char *destination, *destination_y_start, *destination_z_start, *source;

	ENTER(Texture_get_image_tile);
	if (texture && texture_tiling)
	{
		return_code = 1;
		number_of_components =
			Texture_storage_type_get_number_of_components(texture->storage);
		number_of_bytes_per_component = texture->number_of_bytes_per_component;
		bytes_per_pixel =
			number_of_bytes_per_component * number_of_components;
		source_row_width_bytes = 4*((texture->width_texels*bytes_per_pixel + 3)/4);
		cropped_size[0] = texture_tiling->tile_size[0]
			- 2 * texture_tiling->overlap;
		for (i = 1 ; i < texture_tiling->dimension ; i++)
		{
			cropped_size[i] = texture_tiling->tile_size[i]
				- 2 * texture_tiling->overlap;
		}
		tile_row_width_bytes =
			4*((texture_tiling->tile_size[0]*bytes_per_pixel + 3)/4);
		if (!*tile_image)
		{
			unsigned int tile_size = tile_row_width_bytes;
			for (i = 1 ; i < texture_tiling->dimension ; i++)
			{
				tile_size *= texture_tiling->tile_size[i];
			}
			if (!ALLOCATE(*tile_image, unsigned char,
					tile_size))
			{
				display_message(ERROR_MESSAGE,
					"Texture_get_image_tiling.  Unable to allocate tile memory.");
				return_code = 0;
			}
		}
		if (return_code)
		{
			if (texture_tiling->texture_tiles[0] > 1)
			{
				tile_x = tile_number % texture_tiling->texture_tiles[0];
				start_x = (int)(tile_x * cropped_size[0] - texture_tiling->overlap);
				end_x = start_x + texture_tiling->tile_size[0];
				if (start_x < 0)
				{
					pad_x_start = -start_x;
					start_x = 0;
				}
				else
				{
					pad_x_start = 0;
				}
				if (end_x > texture->width_texels)
				{
					pad_x_end = end_x - texture->width_texels;
					end_x = texture->width_texels;
					copy_row_width_bytes = (end_x - start_x)*bytes_per_pixel;
				}
				else
				{
					pad_x_end = 0;
					if (pad_x_start == 0)
					{
						copy_row_width_bytes = tile_row_width_bytes;
					}
					else
					{
						copy_row_width_bytes = (end_x - start_x)*bytes_per_pixel;
					}
				}
			}
			else
			{
				tile_x = 0;
				start_x = 0;
				end_x = texture->width_texels;
				pad_x_start = 0;
				pad_x_end = 0;
				copy_row_width_bytes = tile_row_width_bytes;
			}
			if ((texture_tiling->dimension > 1) && (texture_tiling->texture_tiles[1] > 1))
			{
				tile_y = (tile_number / texture_tiling->texture_tiles[0])
					% texture_tiling->texture_tiles[1];
				start_y = (int)(tile_y * cropped_size[1] - texture_tiling->overlap);
				end_y = start_y + texture_tiling->tile_size[1];
				if (start_y < 0)
				{
					pad_y_start = -start_y;
					start_y = 0;
				}
				else
				{
					pad_y_start = 0;
				}
				if (end_y > texture->height_texels)
				{
					pad_y_end = end_y - texture->height_texels;
					end_y = texture->height_texels;
				}
				else
				{
					pad_y_end = 0;
				}
			}
			else
			{
				tile_y = 0;
				start_y = 0;
				end_y = texture->height_texels;
				pad_y_start = 0;
				pad_y_end = 0;
			}
			if ((texture_tiling->dimension > 2) && (texture_tiling->texture_tiles[2] > 1))
			{
				tile_z = (tile_number / (texture_tiling->texture_tiles[0] *
						texture_tiling->texture_tiles[1]))
					% texture_tiling->texture_tiles[2];
				start_z = (int)(tile_z * cropped_size[2] - texture_tiling->overlap);
				end_z = start_z + texture_tiling->tile_size[2];
				if (start_z < 0)
				{
					pad_z_start = -start_z;
					start_z = 0;
				}
				else
				{
					pad_z_start = 0;
				}
				if (end_z > texture->depth_texels)
				{
					pad_z_end = end_z - texture->depth_texels;
					end_z = texture->depth_texels;
				}
				else
				{
					pad_z_end = 0;
				}
			}
			else
			{
				tile_z = 0;
				start_z = 0;
				end_z = texture->depth_texels;
				pad_z_start = 0;
				pad_z_end = 0;
			}
#if defined (DEBUG_CODE)
			printf ("Rendering tile %d, tile_x %d, tile_y %d, tile_z %d\n",
				tile_number, tile_x, tile_y, tile_z);
#endif /* defined (DEBUG_CODE) */
		}
		source = texture->image + source_row_width_bytes *
			(start_y + start_z * texture->height_texels) +
			start_x * bytes_per_pixel;
		destination = *tile_image;
		destination_z_start = destination;
		destination += pad_z_start * texture_tiling->tile_size[1] * tile_row_width_bytes;
		for (z = start_z ; z < end_z ; z++)
		{
			destination_y_start = destination;
			destination += pad_y_start * tile_row_width_bytes;
			for (y = start_y ; y < end_y ; y++)
			{
				for (i = 0 ; i < pad_x_start ; i++)
				{
					/* Repeat the first pixel, edge_clamping */
					memcpy(destination, source, bytes_per_pixel);
					destination += bytes_per_pixel;
				}
				memcpy(destination, source, copy_row_width_bytes);
				destination += copy_row_width_bytes;
				for (i = 0 ; i < pad_x_end ; i++)
				{
					/* Repeat the last pixel, edge_clamping */
					memcpy(destination, source + copy_row_width_bytes - bytes_per_pixel, bytes_per_pixel);
					destination += bytes_per_pixel;
				}
				source += source_row_width_bytes;
				{
					int row_padding = tile_row_width_bytes - (copy_row_width_bytes + (pad_x_start + pad_x_end) * bytes_per_pixel);
					memset(destination, 0, row_padding);
					destination += row_padding;
				}
			}
			if (pad_y_start > 0)
			{
				unsigned char *destination_first_row = destination_y_start + tile_row_width_bytes * pad_y_start;
				for (i = 0 ; i < pad_y_start ; i++)
				{
					/* Repeat the first row, edge_clamping,
					 * easier to do after we have already padded the row so copy from destination */
					memcpy(destination_y_start, destination_first_row,	tile_row_width_bytes);
					destination_y_start += tile_row_width_bytes;
				}
			}
			for (i = 0 ; i < pad_y_end ; i++)
			{
				/* Repeat the last row, edge_clamping, copy from destination */
				unsigned char *destination_last_row = destination - tile_row_width_bytes;
				memcpy(destination, destination_last_row, tile_row_width_bytes);
				destination += tile_row_width_bytes;
			}
			source += source_row_width_bytes * (texture->height_texels -
				texture_tiling->tile_size[1]);
#if defined (DEBUG_CODE)
			unsigned char *expected_destination = *tile_image + tile_row_width_bytes * texture_tiling->tile_size[1] * (z - start_z + 1);
			if (destination != expected_destination)
			{
				printf("destination mismatch %p %p %d\n", destination, expected_destination,
					destination - expected_destination);
			}
#endif // defined (DEBUG_CODE)
		}
		for (i = 0 ; i < pad_z_start ; i++)
		{
			/* Repeat the first row, edge_clamping,
			 * easier to do after we have already padded the row so copy from destination */
			unsigned char *destination_first_depth = destination_z_start +
				tile_row_width_bytes * texture_tiling->tile_size[1] * pad_z_start;
			memcpy(destination_z_start, destination_first_depth,
				tile_row_width_bytes * texture_tiling->tile_size[1]);
			destination_z_start += tile_row_width_bytes * texture_tiling->tile_size[1];
		}
		for (i = 0 ; i < pad_z_end ; i++)
		{
			/* Repeat the last row, edge_clamping, copy from destination */
			unsigned char *destination_last_depth = destination - tile_row_width_bytes * texture_tiling->tile_size[1];
			memcpy(destination, destination_last_depth, tile_row_width_bytes * texture_tiling->tile_size[1]);
			destination += tile_row_width_bytes * texture_tiling->tile_size[1];
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_get_image_tile.  Missing texture or tiling.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Texture_get_image_tile */
#endif /* defined (OPENGL_API)*/

#if defined (OPENGL_API)
static int Texture_activate_texture_target_environment(struct Texture *texture,
	GLenum texture_target)
/*******************************************************************************
LAST MODIFIED : 28 March 2008

DESCRIPTION :
Separating this out so we can set it for each activated tile.
==============================================================================*/
{
	int return_code;
	GLfloat values[4];

	ENTER(Texture_activate_texture_target_environment);
	return_code = 1;
	if (texture)
	{
		switch (texture->wrap_mode)
		{
			case TEXTURE_CLAMP_WRAP:
			{
				glTexParameteri(texture_target,GL_TEXTURE_WRAP_S,GL_CLAMP);
				glTexParameteri(texture_target,GL_TEXTURE_WRAP_T,GL_CLAMP);
#if defined (GL_VERSION_1_2) || defined (GL_EXT_texture3D)
				glTexParameteri(texture_target,GL_TEXTURE_WRAP_R,GL_CLAMP);
#endif /* defined (GL_VERSION_1_2) || defined (GL_EXT_texture3D) */
			} break;
			case TEXTURE_REPEAT_WRAP:
			{
				glTexParameteri(texture_target,GL_TEXTURE_WRAP_S,GL_REPEAT);
				glTexParameteri(texture_target,GL_TEXTURE_WRAP_T,GL_REPEAT);
#if defined (GL_VERSION_1_2) || defined (GL_EXT_texture3D)
				glTexParameteri(texture_target,GL_TEXTURE_WRAP_R,GL_REPEAT);
#endif /* defined (GL_VERSION_1_2) || defined (GL_EXT_texture3D) */
			} break;
			case TEXTURE_CLAMP_EDGE_WRAP:
			{
#if defined (GL_VERSION_1_2)
				if (Graphics_library_check_extension(GL_VERSION_1_2))
				{
					glTexParameteri(texture_target,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
					glTexParameteri(texture_target,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
					glTexParameteri(texture_target,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE);
				}
				else
				{
					display_message(ERROR_MESSAGE, "Texture_activate_texture_target_environment.  "
						"Texture wrap mode %s not supported on this hardware.",
						ENUMERATOR_STRING(Texture_wrap_mode)(texture->wrap_mode));
					return_code = 0;
				}
#else /* defined (GL_VERSION_1_2) */
				display_message(ERROR_MESSAGE, "Texture_activate_texture_target_environment.  "
					"Texture wrap mode %s was not compiled into this executable.",
					ENUMERATOR_STRING(Texture_wrap_mode)(texture->wrap_mode));
				return_code = 0;
#endif /* defined (GL_VERSION_1_2) */
			} break;
			case TEXTURE_CLAMP_BORDER_WRAP:
			{
#if defined (GL_VERSION_1_3)
				if (Graphics_library_check_extension(GL_VERSION_1_3))
				{
					glTexParameteri(texture_target,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_BORDER);
					glTexParameteri(texture_target,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_BORDER);
					glTexParameteri(texture_target,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_BORDER);
				}
				else
				{
					display_message(ERROR_MESSAGE, "Texture_activate_texture_target_environment.  "
						"Texture wrap mode %s not supported on this hardware.",
						ENUMERATOR_STRING(Texture_wrap_mode)(texture->wrap_mode));
					return_code = 0;
				}
#else /* defined (GL_VERSION_1_3) */
				display_message(ERROR_MESSAGE, "Texture_activate_texture_target_environment.  "
					"Texture wrap mode %s was not compiled into this executable.",
					ENUMERATOR_STRING(Texture_wrap_mode)(texture->wrap_mode));
				return_code = 0;
#endif /* defined (GL_VERSION_1_3) */
			} break;
			case TEXTURE_MIRRORED_REPEAT_WRAP:
			{
#if defined (GL_VERSION_1_4)
				if (Graphics_library_check_extension(GL_VERSION_1_4))
				{
					glTexParameteri(texture_target,GL_TEXTURE_WRAP_S,GL_MIRRORED_REPEAT);
					glTexParameteri(texture_target,GL_TEXTURE_WRAP_T,GL_MIRRORED_REPEAT);
					glTexParameteri(texture_target,GL_TEXTURE_WRAP_R,GL_MIRRORED_REPEAT);
				}
				else
				{
					display_message(ERROR_MESSAGE, "Texture_activate_texture_target_environment.  "
						"Texture wrap mode %s not supported on this hardware.",
						ENUMERATOR_STRING(Texture_wrap_mode)(texture->wrap_mode));
					return_code = 0;
				}
#else /* defined (GL_VERSION_1_4) */
				display_message(ERROR_MESSAGE, "Texture_activate_texture_target_environment.  "
					"Texture wrap mode %s was not compiled into this executable.",
					ENUMERATOR_STRING(Texture_wrap_mode)(texture->wrap_mode));
				return_code = 0;
#endif /* defined (GL_VERSION_1_4) */
			} break;
		}
		switch (texture->filter_mode)
		{
			case TEXTURE_LINEAR_FILTER:
			{
				glTexParameteri(texture_target,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
				glTexParameteri(texture_target,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
			} break;
			case TEXTURE_NEAREST_FILTER:
			{
				glTexParameteri(texture_target,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
				glTexParameteri(texture_target,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
			} break;
			case TEXTURE_LINEAR_MIPMAP_LINEAR_FILTER:
			{
#if defined (GL_VERSION_1_4)
				/* The GL_GENERATE_MIPMAP extension was promoted to core in OpenGL 1.4.
				 * (but has been deprecated for manual calculation with glGenerateMipmap
				 * but I haven't changed right now as that seems less supported)
				 * We check for the previous extension name at runtime and if not available
				 * then fallback to software mipmap generation.
				 */
				glTexParameteri(texture_target,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
				glTexParameteri(texture_target,GL_TEXTURE_MIN_FILTER,
					GL_LINEAR_MIPMAP_LINEAR);
				glTexParameterf(texture_target,GL_TEXTURE_LOD_BIAS, texture->mipmap_level_of_detail_bias);
				if (Graphics_library_check_extension(GL_SGIS_generate_mipmap))
				{
					glTexParameteri(texture_target,GL_GENERATE_MIPMAP,GL_TRUE);
				}
#else /* defined (GL_VERSION_1_4) */
				display_message(ERROR_MESSAGE, "Texture_activate_texture_target_environment.  "
					"Texture filter mode %s was not compiled into this executable.",
					ENUMERATOR_STRING(Texture_filter_mode)(texture->filter_mode));
				return_code = 0;
#endif /* defined (GL_VERSION_1_4) */
			} break;
			case TEXTURE_LINEAR_MIPMAP_NEAREST_FILTER:
			{
#if defined (GL_VERSION_1_4)
				glTexParameteri(texture_target,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
				glTexParameteri(texture_target,GL_TEXTURE_MIN_FILTER,
					GL_LINEAR_MIPMAP_NEAREST);
				glTexParameterf(texture_target,GL_TEXTURE_LOD_BIAS, texture->mipmap_level_of_detail_bias);
				if (Graphics_library_check_extension(GL_SGIS_generate_mipmap))
				{
					glTexParameteri(texture_target,GL_GENERATE_MIPMAP,GL_TRUE);
				}
#else /* defined (GL_VERSION_1_4) */
				display_message(ERROR_MESSAGE, "Texture_activate_texture_target_environment.  "
					"Texture filter mode %s was not compiled into this executable.",
					ENUMERATOR_STRING(Texture_filter_mode)(texture->filter_mode));
				return_code = 0;
#endif /* defined (GL_VERSION_1_4) */
			} break;
			case TEXTURE_NEAREST_MIPMAP_NEAREST_FILTER:
			{
#if defined (GL_VERSION_1_4)
				glTexParameteri(texture_target,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
				glTexParameteri(texture_target,GL_TEXTURE_MIN_FILTER,
					GL_NEAREST_MIPMAP_NEAREST);
				glTexParameterf(texture_target,GL_TEXTURE_LOD_BIAS, texture->mipmap_level_of_detail_bias);
				if (Graphics_library_check_extension(GL_SGIS_generate_mipmap))
				{
					glTexParameteri(texture_target,GL_GENERATE_MIPMAP,GL_TRUE);
				}
#else /* defined (GL_VERSION_1_4) */
				display_message(ERROR_MESSAGE, "Texture_activate_texture_target_environment.  "
					"Texture filter mode %s was not compiled into this executable.",
					ENUMERATOR_STRING(Texture_filter_mode)(texture->filter_mode));
				return_code = 0;
#endif /* defined (GL_VERSION_1_4) */
			} break;
		}
		values[0]=(texture->combine_colour).red;
		values[1]=(texture->combine_colour).green;
		values[2]=(texture->combine_colour).blue;
		values[3]=texture->combine_alpha;
		glTexParameterfv(texture_target,GL_TEXTURE_BORDER_COLOR,values);
	}
	else
	{
		display_message(ERROR_MESSAGE,"Texture_activate_texture_target_environment.  Missing texture");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Texture_activate_texture_target_environment */
#endif /* defined (OPENGL_API) */

#if defined (OPENGL_API)
/***************************************************************************//**
 * Calculates and loads software mipmap levels for the texture.
 *
 * @param texture The currently active texture for which mipmaps should be loaded.
 * @param renderer  The currently operating renderer.
 * @return Returns 1 if successful.
 */
static int Texture_generate_software_mipmaps(struct Texture *texture,
	Render_graphics_opengl *renderer, unsigned char *source_image,
	int render_width, int render_height, int render_depth,
	int hardware_storage_format, GLenum format, GLenum type)
{
	int return_code;

	if (texture && renderer)
	{
		int max_size;
		unsigned int number_of_levels;

		max_size = render_width;
		if ((texture->dimension > 1) && render_height > max_size)
		{
			max_size = render_height;
		}
		if ((texture->dimension > 2) && render_depth > max_size)
		{
			max_size = render_depth;
		}

		number_of_levels = (unsigned int)floor(log2(max_size)) + 1;

#if defined (DEBUG_CODE)
		printf("Max size %d levels %u\n", max_size, number_of_levels);
#endif // defined (DEBUG_CODE)

		unsigned char *previous_mipmap_image = source_image;
		int previous_mipmap_width = render_width;
		int previous_mipmap_height = render_height;
		int previous_mipmap_depth = render_depth;


		double scaling = 1.0;
		unsigned int level;
		for (level = 1 ; (NULL != previous_mipmap_image) && (level < number_of_levels) ; level++)
		{
			scaling *= 2.0;
			int level_width = (int)floor((double)render_width / scaling);
			if (level_width < 1)
				level_width = 1;

			int level_height = (int)floor((double)render_height / scaling);
			if (level_height < 1)
				level_height = 1;

			int level_depth = (int)floor((double)render_depth / scaling);
			if (level_depth < 1)
				level_depth = 1;

#if defined (DEBUG_CODE)
			printf(" Level %u (%d,%d,%d)\n", level, level_width, level_height, level_depth);
#endif // defined (DEBUG_CODE)

			unsigned char *mipmap_image = Texture_get_resized_image(texture,
				previous_mipmap_image, previous_mipmap_width, previous_mipmap_height, previous_mipmap_depth,
				level_width, level_height, level_depth, TEXTURE_RESIZE_LINEAR_FILTER);

			if (mipmap_image)
			{
				switch (texture->dimension)
				{
					case 1:
					{
						glTexImage1D(GL_TEXTURE_1D, level,
							(GLint)hardware_storage_format,
							(GLint)level_width, (GLint)0,
							format, type, (GLvoid *)mipmap_image);
					} break;
					case 2:
					{
						glTexImage2D(GL_TEXTURE_2D, level,
							(GLint)hardware_storage_format,
							(GLint)level_width,
							(GLint)level_height, (GLint)0,
							format, type, (GLvoid *)mipmap_image);
					} break;
					case 3:
					{
	#if defined (GL_VERSION_1_2) || defined (GL_EXT_texture3D)
						if (
	#  if defined (GL_VERSION_1_2)
							Graphics_library_check_extension(GL_VERSION_1_2)
	#    if defined (GL_EXT_texture3D)
							||
	#    endif /* defined (GL_EXT_texture3D) */
	#  endif /* defined (GL_VERSION_1_2) */
	#  if defined (GL_EXT_texture3D)
							Graphics_library_check_extension(GL_EXT_texture3D)
	#  endif /* defined (GL_EXT_texture3D) */
							)
						{
							glTexImage3D(GL_TEXTURE_3D, level,
								(GLint)hardware_storage_format,
								(GLint)level_width,
								(GLint)level_height,
								(GLint)level_depth, (GLint)0,
								format, type, (GLvoid *)mipmap_image);
						}
						else
						{
	#endif /* defined (GL_VERSION_1_2) || defined (GL_EXT_texture3D) */
							display_message(ERROR_MESSAGE,"direct_render_Texture.  "
								"3D textures not supported on this display.");
							return_code=0;
	#if defined (GL_VERSION_1_2) || defined (GL_EXT_texture3D)
						}
	#endif /* defined (GL_VERSION_1_2) || defined (GL_EXT_texture3D) */
					} break;
				}
			}

			if (previous_mipmap_image != source_image)
			{
				DEALLOCATE(previous_mipmap_image);
			}
			previous_mipmap_image = mipmap_image;
			previous_mipmap_width = level_width;
			previous_mipmap_height = level_height;
			previous_mipmap_depth = level_depth;
		}
		if (previous_mipmap_image)
		{
			return_code = 1;
			if (previous_mipmap_image != source_image)
			{
				DEALLOCATE(previous_mipmap_image);
			}
		}
		else
		{
			return_code = 0;
		}
	}
	else
	{
		return_code = 0;
	}

	return (return_code);
} /* Texture_generate_software_mipmaps */
#endif /* defined (OPENGL_API) */

#if defined (OPENGL_API)
static int direct_render_Texture(struct Texture *texture,
	Render_graphics_opengl *renderer)
/*******************************************************************************
LAST MODIFIED : 19 November 2007

DESCRIPTION :
Directly outputs the commands setting up the <texture>.
==============================================================================*/
{
	int  return_code;
	int hardware_storage_format, i, number_of_components, number_of_tiles = 0,
		reduction_flag,reductions[3],
		render_tile_width = 0, render_tile_height = 0, render_tile_depth = 0;
	GLenum format = GL_COLOR_INDEX, texture_target, type = GL_INT;
	unsigned char *reduced_image, *rendered_image;

	ENTER(direct_render_Texture);
	return_code = 1;
	if (texture)
	{
		rendered_image = (unsigned char *)NULL;
		texture_target = Texture_get_target_enum(texture);
		glEnable(texture_target);
		Texture_activate_texture_target_environment(texture,
			texture_target);
#if defined (GL_ARB_texture_non_power_of_two)
		if (!Graphics_library_check_extension(GL_ARB_texture_non_power_of_two))
		{
#endif /* defined (GL_ARB_texture_non_power_of_two) */
			Texture_expand_to_power_of_two(texture);
#if defined (GL_ARB_texture_non_power_of_two)
		}
#endif /* defined (GL_ARB_texture_non_power_of_two) */

		/* make each row of the image start on a 4-byte boundary */
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		Texture_get_type_and_format_from_storage_type(texture->storage,
			texture->number_of_bytes_per_component, &type, &format);
		number_of_components =
			Texture_storage_type_get_number_of_components(texture->storage);
		hardware_storage_format = Texture_get_hardware_storage_format
			(texture->compression_mode, number_of_components,
			texture->number_of_bytes_per_component);

		if (0 < (reduction_flag = Texture_get_hardware_reduction_or_tiling(texture,
			 reductions, renderer->allow_texture_tiling, &renderer->texture_tiling)))
		{
			reduced_image = (unsigned char *)NULL;
			switch(reduction_flag)
			{
				case 1:
				{
					rendered_image = texture->image;
					texture->rendered_width_texels = texture->width_texels;
					texture->rendered_height_texels = texture->height_texels;
					texture->rendered_depth_texels = texture->depth_texels;
					render_tile_width = texture->width_texels;
					render_tile_height = texture->height_texels;
					render_tile_depth = texture->depth_texels;
					number_of_tiles = 1;
				} break;
				case 2:
				{
					switch (texture->dimension)
					{
						case 1:
						{
							texture->rendered_width_texels =
								texture->width_texels / reductions[0];
							texture->rendered_height_texels = 1;
							texture->rendered_depth_texels = 1;
							display_message(WARNING_MESSAGE,
								"1-D image %s is too large for this display.  "
								"Reducing width from %d to %d for display only",
								texture->name,
								texture->width_texels,
								texture->rendered_width_texels);
						} break;
						case 2:
						{
							texture->rendered_width_texels =
								texture->width_texels / reductions[0];
							texture->rendered_height_texels =
								texture->height_texels / reductions[1];
							texture->rendered_depth_texels = 1;
							display_message(WARNING_MESSAGE,
								"Image %s is too large for this display.  "
								"Reducing (%d,%d) to (%d,%d) for display only",
								texture->name,
								texture->width_texels, texture->height_texels,
								texture->rendered_width_texels,
								texture->rendered_height_texels);
						} break;
						case 3:
						{
							texture->rendered_width_texels =
								texture->width_texels / reductions[0];
							texture->rendered_height_texels =
								texture->height_texels / reductions[1];
							texture->rendered_depth_texels =
								texture->depth_texels / reductions[2];
							display_message(WARNING_MESSAGE,
								"3-D image %s is too large for this display.  "
								"Reducing (%d,%d,%d) to (%d,%d,%d) for display only",
								texture->name,
								texture->width_texels, texture->height_texels,
								texture->depth_texels,
								texture->rendered_width_texels,
								texture->rendered_height_texels,
								texture->rendered_depth_texels);
						} break;
						default:
						{
							display_message(ERROR_MESSAGE,
								"direct_render_Texture.  Invalid texture dimension");
							return_code = 0;
						}
					}
					if (return_code)
					{
						reduced_image = Texture_get_resized_image(texture,
							texture->image, texture->width_texels, texture->height_texels, texture->depth_texels,
							texture->rendered_width_texels, texture->rendered_height_texels,
							texture->rendered_depth_texels, texture->resize_filter_mode);
						if (reduced_image)
						{
							rendered_image = reduced_image;
						}
						else
						{
							display_message(ERROR_MESSAGE, "direct_render_Texture.  "
								"Could not reduce texture size for display");
							return_code = 0;
						}
						render_tile_width = texture->rendered_width_texels;
						render_tile_height = texture->rendered_height_texels;
						render_tile_depth = texture->rendered_depth_texels;
					}
					number_of_tiles = 1;
				} break;
				case 3:
				{
					number_of_tiles = 1;
					for (i = 0 ; i < renderer->texture_tiling->dimension ; i++)
					{
						number_of_tiles *= renderer->texture_tiling->texture_tiles[i];
					}
					render_tile_width = renderer->texture_tiling->tile_size[0];
					if (texture->dimension > 1)
					{
						render_tile_height = renderer->texture_tiling->tile_size[1];
					}
					else
					{
						render_tile_height = 1;
					}
					if (texture->dimension > 2)
					{
						render_tile_depth = renderer->texture_tiling->tile_size[2];
					}
					else
					{
						render_tile_depth = 1;
					}
				} break;
			}
			if (return_code)
			{
				for (i = 0 ; return_code && (i < number_of_tiles) ; i++)
				{
					if (3 == reduction_flag)
					{
						Texture_get_image_tile(texture, &reduced_image,
							renderer->texture_tiling, /*tile_number*/i);
						glBindTexture(texture_target,
							renderer->texture_tiling->texture_ids[i]);
						rendered_image = reduced_image;
						glEnable(texture_target);

						Texture_activate_texture_target_environment(texture,
							texture_target);
					}
					switch (texture->dimension)
					{
						case 1:
						{
							glTexImage1D(GL_TEXTURE_1D, (GLint)0,
								(GLint)hardware_storage_format,
								(GLint)render_tile_width, (GLint)0,
								format, type, (GLvoid *)rendered_image);
						} break;
						case 2:
						{
							glTexImage2D(GL_TEXTURE_2D, (GLint)0,
								(GLint)hardware_storage_format,
								(GLint)render_tile_width,
								(GLint)render_tile_height, (GLint)0,
								format, type, (GLvoid *)rendered_image);
						} break;
						case 3:
						{
#if defined (GL_VERSION_1_2) || defined (GL_EXT_texture3D)
							if (
#  if defined (GL_VERSION_1_2)
								Graphics_library_check_extension(GL_VERSION_1_2)
#    if defined (GL_EXT_texture3D)
								||
#    endif /* defined (GL_EXT_texture3D) */
#  endif /* defined (GL_VERSION_1_2) */
#  if defined (GL_EXT_texture3D)
								Graphics_library_check_extension(GL_EXT_texture3D)
#  endif /* defined (GL_EXT_texture3D) */
								)
							{
								glTexImage3D(GL_TEXTURE_3D, (GLint)0,
									(GLint)hardware_storage_format,
									(GLint)render_tile_width,
									(GLint)render_tile_height,
									(GLint)render_tile_depth, (GLint)0,
									format, type, (GLvoid *)rendered_image);
							}
							else
							{
#endif /* defined (GL_VERSION_1_2) || defined (GL_EXT_texture3D) */
								display_message(ERROR_MESSAGE,"direct_render_Texture.  "
									"3D textures not supported on this display.");
								return_code=0;
#if defined (GL_VERSION_1_2) || defined (GL_EXT_texture3D)
							}
#endif /* defined (GL_VERSION_1_2) || defined (GL_EXT_texture3D) */
						} break;
					}
#if defined (GL_VERSION_1_4)
					switch (texture->filter_mode)
					{
						case TEXTURE_LINEAR_MIPMAP_NEAREST_FILTER:
						case TEXTURE_LINEAR_MIPMAP_LINEAR_FILTER:
						{
							if (!Graphics_library_check_extension(GL_SGIS_generate_mipmap))
							{
								Texture_generate_software_mipmaps(texture, renderer, rendered_image,
									render_tile_width, render_tile_height, render_tile_depth,
									hardware_storage_format, format, type);
							}
						} break;
						default:
						{
							/* Do nothing */
						}
					}
#endif /* defined (GL_VERSION_1_4) */
				}
			}
			if (reduced_image)
			{
				DEALLOCATE(reduced_image);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Not enough hardware memory resources for texture %s",
				texture->name);
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"direct_render_Texture.  Missing texture");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* direct_render_Texture */
#endif /* defined (OPENGL_API) */

/*
Global functions
----------------
*/

PROTOTYPE_ENUMERATOR_STRING_FUNCTION(Texture_combine_mode)
{
	const char *enumerator_string;

	ENTER(ENUMERATOR_STRING(Texture_combine_mode));
	switch (enumerator_value)
	{
		case TEXTURE_BLEND:
		{
			enumerator_string = "blend";
		} break;
		case TEXTURE_DECAL:
		{
			enumerator_string = "decal";
		} break;
		case TEXTURE_MODULATE:
		{
			enumerator_string = "modulate";
		} break;
		case TEXTURE_ADD:
		{
			enumerator_string = "add";
		} break;
		case TEXTURE_ADD_SIGNED:
		{
			enumerator_string = "signed_add";
		} break;
		case TEXTURE_MODULATE_SCALE_4:
		{
			enumerator_string = "scale_4_modulate";
		} break;
		case TEXTURE_BLEND_SCALE_4:
		{
			enumerator_string = "scale_4_blend";
		} break;
		case TEXTURE_SUBTRACT:
		{
			enumerator_string = "subtract";
		} break;
		case TEXTURE_ADD_SCALE_4:
		{
			enumerator_string = "scale_4_add";
		} break;
		case TEXTURE_SUBTRACT_SCALE_4:
		{
			enumerator_string = "scale_4_subtract";
		} break;
		case TEXTURE_INVERT_ADD_SCALE_4:
		{
			enumerator_string = "invert_scale_4_add";
		} break;
		case TEXTURE_INVERT_SUBTRACT_SCALE_4:
		{
			enumerator_string = "invert_scale_4_subtract";
		} break;
		default:
		{
			enumerator_string = (const char *)NULL;
		} break;
	}
	LEAVE;

	return (enumerator_string);
} /* ENUMERATOR_STRING(Texture_combine_mode) */

DEFINE_DEFAULT_ENUMERATOR_FUNCTIONS(Texture_combine_mode)

PROTOTYPE_ENUMERATOR_STRING_FUNCTION(Texture_compression_mode)
{
	const char *enumerator_string;

	ENTER(ENUMERATOR_STRING(Texture_compression_mode));
	switch (enumerator_value)
	{
		case TEXTURE_UNCOMPRESSED:
		{
			enumerator_string = "uncompressed";
		} break;
		case TEXTURE_COMPRESSED_UNSPECIFIED:
		{
			enumerator_string = "compressed_unspecified";
		} break;
		default:
		{
			enumerator_string = (const char *)NULL;
		} break;
	}
	LEAVE;

	return (enumerator_string);
} /* ENUMERATOR_STRING(Texture_compression_mode) */

DEFINE_DEFAULT_ENUMERATOR_FUNCTIONS(Texture_compression_mode)

PROTOTYPE_ENUMERATOR_STRING_FUNCTION(Texture_filter_mode)
{
	const char *enumerator_string;

	ENTER(ENUMERATOR_STRING(Texture_filter_mode));
	switch (enumerator_value)
	{
		case TEXTURE_LINEAR_FILTER:
		{
			enumerator_string = "linear_filter";
		} break;
		case TEXTURE_NEAREST_FILTER:
		{
			enumerator_string = "nearest_filter";
		} break;
		/* Need to prepend these strings with something
			so that they don't conflict with abbreviations of
			the old values above */
		case TEXTURE_LINEAR_MIPMAP_LINEAR_FILTER:
		{
			enumerator_string = "filter_linear_mipmap_linear";
		} break;
		case TEXTURE_LINEAR_MIPMAP_NEAREST_FILTER:
		{
			enumerator_string = "filter_linear_mipmap_nearest";
		} break;
		case TEXTURE_NEAREST_MIPMAP_NEAREST_FILTER:
		{
			enumerator_string = "filter_nearest_mipmap_nearest";
		} break;
		default:
		{
			enumerator_string = (const char *)NULL;
		} break;
	}
	LEAVE;

	return (enumerator_string);
} /* ENUMERATOR_STRING(Texture_filter_mode) */

DEFINE_DEFAULT_ENUMERATOR_FUNCTIONS(Texture_filter_mode)

PROTOTYPE_ENUMERATOR_STRING_FUNCTION(Texture_resize_filter_mode)
{
	const char *enumerator_string;

	ENTER(ENUMERATOR_STRING(Texture_resize_filter_mode));
	switch (enumerator_value)
	{
		case TEXTURE_RESIZE_LINEAR_FILTER:
		{
			enumerator_string = "resize_linear_filter";
		} break;
		case TEXTURE_RESIZE_NEAREST_FILTER:
		{
			enumerator_string = "resize_nearest_filter";
		} break;
		default:
		{
			enumerator_string = (const char *)NULL;
		} break;
	}
	LEAVE;

	return (enumerator_string);
} /* ENUMERATOR_STRING(Texture_resize_filter_mode) */

DEFINE_DEFAULT_ENUMERATOR_FUNCTIONS(Texture_resize_filter_mode)

int Texture_storage_type_is_user_selectable(
	enum Texture_storage_type storage, void *dummy)
/*******************************************************************************
LAST MODIFIED : 28 February 2002

DESCRIPTION :
Returns true if <storage> is one of the basic storage types that users can
select, ie. not a digital media buffer.
==============================================================================*/
{
	int return_code

	ENTER(Texture_storage_type_is_user_selectable);
	USE_PARAMETER(dummy);
	return_code =
		(TEXTURE_LUMINANCE == storage) ||
		(TEXTURE_LUMINANCE_ALPHA == storage) ||
		(TEXTURE_RGB == storage) ||
		(TEXTURE_RGBA == storage) ||
		(TEXTURE_ABGR == storage);
	LEAVE;

	return (return_code);
} /* Texture_storage_type_is_user_selectable */

PROTOTYPE_ENUMERATOR_STRING_FUNCTION(Texture_storage_type)
{
	const char *enumerator_string;

	ENTER(ENUMERATOR_STRING(Texture_storage_type));
	switch (enumerator_value)
	{
		case TEXTURE_LUMINANCE:
		{
			enumerator_string = "i";
		} break;
		case TEXTURE_LUMINANCE_ALPHA:
		{
			enumerator_string = "ia";
		} break;
		case TEXTURE_RGB:
		{
			enumerator_string = "rgb";
		} break;
		case TEXTURE_RGBA:
		{
			enumerator_string = "rgba";
		} break;
		case TEXTURE_ABGR:
		{
			enumerator_string = "abgr";
		} break;
		default:
		{
			enumerator_string = (const char *)NULL;
		} break;
	}
	LEAVE;

	return (enumerator_string);
} /* ENUMERATOR_STRING(Texture_storage_type) */

DEFINE_DEFAULT_ENUMERATOR_FUNCTIONS(Texture_storage_type)

PROTOTYPE_ENUMERATOR_STRING_FUNCTION(Texture_wrap_mode)
{
	const char *enumerator_string;

	ENTER(ENUMERATOR_STRING(Texture_wrap_mode));
	switch (enumerator_value)
	{
		case TEXTURE_CLAMP_WRAP:
		{
			enumerator_string = "clamp_wrap";
		} break;
		case TEXTURE_REPEAT_WRAP:
		{
			enumerator_string = "repeat_wrap";
		} break;
		case TEXTURE_CLAMP_EDGE_WRAP:
		{
			enumerator_string = "edge_clamp_wrap";
		} break;
		case TEXTURE_CLAMP_BORDER_WRAP:
		{
			enumerator_string = "border_clamp_wrap";
		} break;
		case TEXTURE_MIRRORED_REPEAT_WRAP:
		{
			enumerator_string = "mirrored_repeat_wrap";
		} break;
		default:
		{
			enumerator_string = (const char *)NULL;
		} break;
	}
	LEAVE;

	return (enumerator_string);
} /* ENUMERATOR_STRING(Texture_wrap_mode) */

DEFINE_DEFAULT_ENUMERATOR_FUNCTIONS(Texture_wrap_mode)

struct Texture *CREATE(Texture)(const char *name)
/*******************************************************************************
LAST MODIFIED : 12 February 2002

DESCRIPTION :
Allocates memory and assigns fields for a texture.  Adds the texture to the list
of all textures.
???DB.  Trimming name ?
???DB.  Check if it already exists ?  Retrieve if it does already exist ?
==============================================================================*/
{
	struct Texture *texture;

	ENTER(CREATE(Texture));
	/* allocate memory for structure */
	if (ALLOCATE(texture, struct Texture, 1)&&
		ALLOCATE(texture->image, unsigned char, 4))
	{
		if (name)
		{
			if (ALLOCATE(texture->name,char,strlen(name)+1))
			{
				strcpy((char *)texture->name,name);
			}
		}
		else
		{
			if (ALLOCATE(texture->name,char,1))
			{
				*((char *)texture->name)='\0';
			}
		}
		if (texture->name)
		{
			/* texture defaults to 1-D since it is a single texel */
			texture->dimension = 1;
			texture->depth = 1.0;
			texture->height = 1.0;
			texture->width = 1.0;
			/* assign image description fields */
			texture->image_file_name = (char *)NULL;
			/* file number pattern and ranges for 3-D textures */
			texture->file_number_pattern = (char *)NULL;
			texture->start_file_number = 0;
			texture->stop_file_number = 0;
			texture->file_number_increment = 0;
			texture->storage=TEXTURE_RGBA;
			texture->number_of_bytes_per_component=1;
			texture->depth_texels=1;
			texture->height_texels=1;
			texture->width_texels=1;
			texture->original_depth_texels=1;
			texture->original_height_texels=1;
			texture->original_width_texels=1;
			texture->rendered_depth_texels=0;
			texture->rendered_height_texels=0;
			texture->rendered_width_texels=0;
			texture->distortion_centre_x=0.0;
			texture->distortion_centre_y=0.0;
			texture->distortion_factor_k1=0.0;
			(texture->image)[0]=0xFF;
			(texture->image)[1]=0xFF;
			(texture->image)[2]=0xFF;
			(texture->image)[3]=0xFF;
			texture->combine_mode=TEXTURE_DECAL;
			texture->compression_mode=TEXTURE_UNCOMPRESSED;
			texture->filter_mode=TEXTURE_NEAREST_FILTER;
			texture->resize_filter_mode=TEXTURE_RESIZE_NEAREST_FILTER;
			texture->wrap_mode=TEXTURE_REPEAT_WRAP;
			(texture->combine_colour).red=0.;
			(texture->combine_colour).green=0.;
			(texture->combine_colour).blue=0.;
			texture->combine_alpha=0.;
			texture->mipmap_level_of_detail_bias=0.;
			texture->graphics_buffer = (struct Graphics_buffer *)NULL;
#if defined (OPENGL_API)
			texture->display_list=0;
			texture->texture_id = 0;
#endif /* defined (OPENGL_API) */
			/* Don't enable texture tiling by default as it doesn't work for all our primitives yet. */
			texture->allow_texture_tiling = 0;
			texture->texture_tiling = (struct Texture_tiling *)NULL;
			texture->display_list_current= TEXTURE_COMPILE_STATE_NOT_COMPILED;
			texture->property_list = (struct LIST(Texture_property) *)NULL;
			texture->access_count=0;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"CREATE(Texture).  Insufficient memory for name");
			DEALLOCATE(texture->image);
			DEALLOCATE(texture);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CREATE(Texture).  Insufficient memory for structure");
		DEALLOCATE(texture);
	}
	LEAVE;

	return (texture);
} /* CREATE(Texture) */

int DESTROY(Texture)(struct Texture **texture_address)
/*******************************************************************************
LAST MODIFIED : 7 February 2002

DESCRIPTION :
Frees the memory for the texture and sets <*texture_address> to NULL.
==============================================================================*/
{
	int return_code;
	struct Texture *texture;

	ENTER(DESTROY(Texture));
	if (texture_address)
	{
		texture= *texture_address;
		if (texture)
		{
			if (texture->access_count<=0)
			{
				if (texture->texture_tiling)
				{
					DEACCESS(Texture_tiling)(&texture->texture_tiling);
				}
#if defined (GRAPHICS_BUFFER_USE_BUFFERS)
				if(texture->graphics_buffer)
				{
					DEACCESS(Graphics_buffer)(&(texture->graphics_buffer));
					return_code=0;
				}
#endif /* defined (GRAPHICS_BUFFER_USE_BUFFERS) */
#if defined (OPENGL_API)
				if (texture->display_list)
				{
					glDeleteLists(texture->display_list,1);
				}
				if (texture->texture_id)
				{
					glDeleteTextures(1, &(texture->texture_id));
				}
#endif /* defined (OPENGL_API) */
				DEALLOCATE(texture->name);
				if (texture->image_file_name)
				{
					DEALLOCATE(texture->image_file_name);
				}
				if (texture->file_number_pattern)
				{
					DEALLOCATE(texture->file_number_pattern);
				}
				DEALLOCATE(texture->image);
				if (texture->property_list)
				{
					DESTROY(LIST(Texture_property))(&texture->property_list);
				}
				DEALLOCATE(*texture_address);
				return_code=1;
			}
			else
			{
				return_code=0;
			}
		}
		else
		{
			return_code=1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"DESTROY(Texture).  Invalid argument");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Texture) */

DECLARE_OBJECT_FUNCTIONS(Texture)
DECLARE_DEFAULT_GET_OBJECT_NAME_FUNCTION(Texture)

int Texture_copy_display_attributes(const struct Texture *source, struct Texture *destination)
{
	if (source && destination)
	{
		destination->height = source->height;
		destination->width = source->width;
		destination->depth = source->depth;
		destination->combine_mode = source->combine_mode;
		destination->compression_mode = source->compression_mode;
		destination->filter_mode = source->filter_mode;
		destination->wrap_mode = source->wrap_mode;
		(destination->combine_colour).red = (source->combine_colour).red;
		(destination->combine_colour).green = (source->combine_colour).green;
		(destination->combine_colour).blue = (source->combine_colour).blue;
		destination->combine_alpha = source->combine_alpha;
		destination->allow_texture_tiling = source->allow_texture_tiling;
		if (destination->texture_tiling)
			DESTROY(Texture_tiling)(&(destination->texture_tiling));
		destination->mipmap_level_of_detail_bias = source->mipmap_level_of_detail_bias;
		destination->resize_filter_mode = source->resize_filter_mode;
		destination->display_list_current = TEXTURE_COMPILE_STATE_NOT_COMPILED;
		return CMZN_RESULT_OK;
	}
	display_message(ERROR_MESSAGE, "Texture_copy_display_attributes.  Invalid argument(s)");
	return CMZN_RESULT_ERROR_ARGUMENT;
}

/**
 * Copy all the texture attributes that describe source files.
 * None of these affect the image output.
 * @return  Result code.
 */
static int Texture_copy_file_attributes(const struct Texture *source, struct Texture *destination)
{
	if (source && destination)
	{
		destination->distortion_centre_x = source->distortion_centre_x;
		destination->distortion_centre_y = source->distortion_centre_y;
		destination->distortion_factor_k1 = source->distortion_factor_k1;
		if (destination->image_file_name)
			DEALLOCATE(destination->image_file_name);
		destination->image_file_name = source->image_file_name ? duplicate_string(source->image_file_name) : 0;
		if (destination->file_number_pattern)
			DEALLOCATE(destination->file_number_pattern);
		destination->file_number_pattern = source->file_number_pattern ? duplicate_string(source->file_number_pattern) : 0;
		destination->start_file_number = source->start_file_number;
		destination->stop_file_number = source->stop_file_number;
		destination->file_number_increment = source->file_number_increment;
		destination->crop_bottom_margin = source->crop_bottom_margin;
		destination->crop_left_margin = source->crop_left_margin;
		destination->crop_width = source->crop_width;
		destination->crop_height = source->crop_height;
		if (source->property_list)
		{
			if (destination->property_list)
				REMOVE_ALL_OBJECTS_FROM_LIST(Texture_property)(destination->property_list);
			else
				destination->property_list = CREATE(LIST(Texture_property))();
			COPY_LIST(Texture_property)(destination->property_list, source->property_list);
		}
		else
		{
			if (destination->property_list)
				DESTROY(LIST(Texture_property))(&destination->property_list);
		}
		return CMZN_RESULT_OK;
	}
	else
	{
		display_message(ERROR_MESSAGE, "Texture_copy_file_attributes.  Invalid argument(s)");
	}
	return CMZN_RESULT_ERROR_ARGUMENT;
}

int Texture_copy_image(const struct Texture *source, struct Texture *destination)
{
	int result = CMZN_RESULT_OK;
	if (source && destination)
	{
		const int number_of_components = Texture_storage_type_get_number_of_components(source->storage);
		const int image_size = source->depth_texels * source->height_texels * 4 *
			((source->width_texels * number_of_components *
				source->number_of_bytes_per_component + 3)/4);
		unsigned char *destination_image;
		if ((0 < image_size) && REALLOCATE(destination_image,
			destination->image, unsigned char, image_size))
		{
			destination->image = destination_image;
			/* use memcpy to copy the image data - should be fastest method */
			memcpy((void *)destination->image, (void *)source->image, image_size);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Texture_copy_image.  Insufficient memory for image");
			result = CMZN_RESULT_ERROR_MEMORY;
		}
		if (result == CMZN_RESULT_OK)
		{
			destination->dimension = source->dimension;
			destination->storage = source->storage;
			destination->number_of_bytes_per_component = source->number_of_bytes_per_component;
			destination->original_width_texels = source->original_width_texels;
			destination->original_height_texels = source->original_height_texels;
			destination->original_depth_texels = source->original_depth_texels;
			/* Not copying the rendered texel size as this is based on the
			   actual size sent to OpenGL */
			destination->width_texels = source->width_texels;
			destination->height_texels = source->height_texels;
			destination->depth_texels = source->depth_texels;
			destination->display_list_current = TEXTURE_COMPILE_STATE_NOT_COMPILED;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE, "Texture_copy_image. Invalid argument(s)");
		result = CMZN_RESULT_ERROR_ARGUMENT;
	}
	return result;
}

int Texture_copy_without_identifier(const struct Texture *source, struct Texture *destination)
{
	int result = Texture_copy_image(source, destination);
	if (result == CMZN_RESULT_OK)
	{
		Texture_copy_display_attributes(source, destination);
		Texture_copy_file_attributes(source, destination);
	}
	return result;
}

int Texture_get_combine_alpha(struct Texture *texture,ZnReal *alpha)
/*******************************************************************************
LAST MODIFIED : 13 February 1998

DESCRIPTION :
Returns the alpha value used for combining the texture.
==============================================================================*/
{
	int return_code;

	ENTER(Texture_get_combine_alpha);
	if (texture&&alpha)
	{
		*alpha=texture->combine_alpha;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_get_combine_alpha.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Texture_get_combine_alpha */

int Texture_set_combine_alpha(struct Texture *texture,ZnReal alpha)
/*******************************************************************************
LAST MODIFIED : 13 February 1998

DESCRIPTION :
Sets the alpha value used for combining the texture.
==============================================================================*/
{
	int return_code;

	ENTER(Texture_set_combine_alpha);
	if (texture&&(0.0<=alpha)&&(1.0>=alpha))
	{
		if (alpha != texture->combine_alpha)
		{
			texture->combine_alpha=alpha;
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_set_combine_alpha.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Texture_set_combine_alpha */

int Texture_get_combine_colour(struct Texture *texture,struct Colour *colour)
/*******************************************************************************
LAST MODIFIED : 13 February 1998

DESCRIPTION :
Returns the colour to be combined with the texture in blending combine mode.
==============================================================================*/
{
	int return_code;

	ENTER(Texture_get_combine_colour);
	if (texture&&colour)
	{
		colour->red=texture->combine_colour.red;
		colour->green=texture->combine_colour.green;
		colour->blue=texture->combine_colour.blue;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_get_combine_colour.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Texture_get_combine_colour */

int Texture_set_combine_colour(struct Texture *texture,struct Colour *colour)
/*******************************************************************************
LAST MODIFIED : 13 February 1998

DESCRIPTION :
Sets the colour to be combined with the texture in blending combine mode.
==============================================================================*/
{
	int return_code;

	ENTER(Texture_set_combine_colour);
	if (texture&&colour)
	{
		texture->combine_colour.red=colour->red;
		texture->combine_colour.green=colour->green;
		texture->combine_colour.blue=colour->blue;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_set_combine_colour.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Texture_set_combine_colour */

enum Texture_combine_mode Texture_get_combine_mode(struct Texture *texture)
/*******************************************************************************
LAST MODIFIED : 11 October 2000

DESCRIPTION :
Returns how the texture is combined with the material: blend, decal or modulate.
==============================================================================*/
{
	enum Texture_combine_mode combine_mode = TEXTURE_BLEND;

	ENTER(Texture_get_combine_mode);
	if (texture)
	{
		combine_mode = texture->combine_mode;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_get_combine_mode.  Invalid argument(s)");
	}
	LEAVE;

	return (combine_mode);
} /* Texture_get_combine_mode */

int Texture_set_combine_mode(struct Texture *texture,
	enum Texture_combine_mode combine_mode)
/*******************************************************************************
LAST MODIFIED : 16 October 2007

DESCRIPTION :
Sets how the texture is combined with the material: blend, decal or modulate.
==============================================================================*/
{
	int return_code;

	ENTER(Texture_set_combine_mode);
	if (texture)
	{
		return_code = 1;
		if (texture->combine_mode != combine_mode)
		{
			/* Check if supported on this platform */
			switch (combine_mode)
			{
				case TEXTURE_BLEND:
				case TEXTURE_DECAL:
				case TEXTURE_MODULATE:
				{
					/* Do nothing, in all versions */
				} break;
				case TEXTURE_ADD:
				case TEXTURE_ADD_SIGNED:
				case TEXTURE_SUBTRACT:
				case TEXTURE_MODULATE_SCALE_4:
				case TEXTURE_BLEND_SCALE_4:
				case TEXTURE_ADD_SCALE_4:
				case TEXTURE_SUBTRACT_SCALE_4:
				case TEXTURE_INVERT_ADD_SCALE_4:
				case TEXTURE_INVERT_SUBTRACT_SCALE_4:
				{
#if defined (GL_VERSION_1_3)
					if (!Graphics_library_tentative_check_extension(GL_VERSION_1_3))
					{
						display_message(ERROR_MESSAGE,
							"Texture_set_combine_mode.  "
							"Texture combine mode %s requires OpenGL 1.3 which is not available on this display.",
						ENUMERATOR_STRING(Texture_combine_mode)(combine_mode));
						return_code = 0;
					}
#else /* defined (GL_VERSION_1_3) */
					display_message(ERROR_MESSAGE,
						"Texture_set_combine_mode.  "
						"Texture combine mode %s requires OpenGL 1.3 which was not compiled into this executable.",
						ENUMERATOR_STRING(Texture_combine_mode)(combine_mode));
					return_code = 0;
#endif /* defined (GL_VERSION_1_3) */
				} break;
				default:
				{
					display_message(ERROR_MESSAGE,
						"Texture_set_combine_mode.  Unknown texture combine mode.");
					return_code=0;
				} break;
			}
			if (return_code)
			{
				texture->combine_mode = combine_mode;
				texture->display_list_current = TEXTURE_COMPILE_STATE_NOT_COMPILED;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_set_combine_mode.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Texture_set_combine_mode */

int Texture_get_mipmap_level_of_detail_bias(struct Texture *texture,ZnReal *bias)
{
	int return_code;

	ENTER(Texture_get_mipmap_level_of_detail_bias);
	if (texture&&bias)
	{
		*bias=texture->mipmap_level_of_detail_bias;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_get_mipmap_level_of_detail_bias.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Texture_get_mipmap_level_of_detail_bias */

int Texture_set_mipmap_level_of_detail_bias(struct Texture *texture,ZnReal bias)
{
	int return_code;

	ENTER(Texture_set_mipmap_level_of_detail_bias);
	if (texture)
	{
		if (bias != texture->mipmap_level_of_detail_bias)
		{
			texture->mipmap_level_of_detail_bias=bias;
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_set_mipmap_level_of_detail_bias.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Texture_set_mipmap_level_of_detail_bias */

enum Texture_compression_mode Texture_get_compression_mode(struct Texture *texture)
/*******************************************************************************
LAST MODIFIED : 8 August 2002

DESCRIPTION :
Returns how the texture is compressed.
==============================================================================*/
{
	enum Texture_compression_mode compression_mode = TEXTURE_UNCOMPRESSED;

	ENTER(Texture_get_compression_mode);
	if (texture)
	{
		compression_mode = texture->compression_mode;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_get_compression_mode.  Invalid argument(s)");
	}
	LEAVE;

	return (compression_mode);
} /* Texture_get_compression_mode */

int Texture_set_compression_mode(struct Texture *texture,
	enum Texture_compression_mode compression_mode)
/*******************************************************************************
LAST MODIFIED : 8 August 2002

DESCRIPTION :
Sets how the texture is compressed.
==============================================================================*/
{
	int return_code;

	ENTER(Texture_set_compression_mode);
	if (texture&&((TEXTURE_UNCOMPRESSED==compression_mode)||
		(TEXTURE_COMPRESSED_UNSPECIFIED==compression_mode)))
	{
		if (compression_mode != texture->compression_mode)
		{
			texture->compression_mode = compression_mode;
			texture->display_list_current = TEXTURE_COMPILE_STATE_NOT_COMPILED;
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_set_compression_mode.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Texture_set_compression_mode */

enum Texture_filter_mode Texture_get_filter_mode(struct Texture *texture)
/*******************************************************************************
LAST MODIFIED : 11 October 2000

DESCRIPTION :
Returns the texture filter: linear or nearest.
==============================================================================*/
{
	enum Texture_filter_mode filter_mode = TEXTURE_LINEAR_FILTER;

	ENTER(Texture_get_filter_mode);
	if (texture)
	{
		filter_mode = texture->filter_mode;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_get_filter_mode.  Invalid argument(s)");
	}
	LEAVE;

	return (filter_mode);
} /* Texture_get_filter_mode */

int Texture_set_filter_mode(struct Texture *texture,
	enum Texture_filter_mode filter_mode)
/*******************************************************************************
LAST MODIFIED : 13 February 1998

DESCRIPTION :
Sets the texture filter: linear or nearest.
==============================================================================*/
{
	const char *type_string;
	int return_code;

	ENTER(Texture_set_filter_mode);
	if (texture)
	{
		if ((TEXTURE_LINEAR_FILTER==filter_mode)||
			(TEXTURE_NEAREST_FILTER==filter_mode)
#if defined (GL_VERSION_1_4)
			/* Cannot do the runtime test here as the graphics may not be
				initialised yet or we may just want to evaluate the texture as a field */
			|| (TEXTURE_LINEAR_MIPMAP_LINEAR_FILTER==filter_mode)
			|| (TEXTURE_LINEAR_MIPMAP_NEAREST_FILTER==filter_mode)
			|| (TEXTURE_NEAREST_MIPMAP_NEAREST_FILTER==filter_mode)
#endif /* defined (GL_VERSION_1_4) */
			 )
		{
			if (filter_mode != texture->filter_mode)
			{
				texture->filter_mode = filter_mode;
				texture->display_list_current = TEXTURE_COMPILE_STATE_NOT_COMPILED;
			}
			return_code=1;
		}
		else
		{
			type_string = ENUMERATOR_STRING(Texture_filter_mode)(filter_mode);
			if (type_string)
			{
				display_message(ERROR_MESSAGE,  "Texture_set_filter_mode.  "
					"Texture filter mode %s was not compiled into this executable.", type_string);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Texture_set_filter_mode.  Invalid filter type.");
			}
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_set_filter_mode.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Texture_set_filter_mode */

enum Texture_resize_filter_mode Texture_get_resize_filter_mode(
	struct Texture *texture)
/*******************************************************************************
LAST MODIFIED : 28 February 2002

DESCRIPTION :
Returns the texture filter: linear or nearest.
==============================================================================*/
{
	enum Texture_resize_filter_mode resize_filter_mode = TEXTURE_RESIZE_LINEAR_FILTER;

	ENTER(Texture_get_resize_filter_mode);
	if (texture)
	{
		resize_filter_mode = texture->resize_filter_mode;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_get_resize_filter_mode.  Invalid argument(s)");
	}
	LEAVE;

	return (resize_filter_mode);
} /* Texture_get_resize_filter_mode */

int Texture_set_resize_filter_mode(struct Texture *texture,
	enum Texture_resize_filter_mode resize_filter_mode)
/*******************************************************************************
LAST MODIFIED : 28 February 2002

DESCRIPTION :
Sets the texture filter: linear or nearest.
==============================================================================*/
{
	int return_code;

	ENTER(Texture_set_resize_filter_mode);
	if (texture&&((TEXTURE_RESIZE_LINEAR_FILTER==resize_filter_mode)||
		(TEXTURE_RESIZE_NEAREST_FILTER==resize_filter_mode)))
	{
		if (resize_filter_mode != texture->resize_filter_mode)
		{
			texture->resize_filter_mode = resize_filter_mode;
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_set_resize_filter_mode.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Texture_set_resize_filter_mode */

int Texture_allocate_image(struct Texture *texture,
	int width, int height, int depth, enum Texture_storage_type storage,
	int number_of_bytes_per_component, const char *source_name)
{
	int bytes_per_pixel, dimension, padded_width_bytes, number_of_components,
		return_code;

	ENTER(Texture_allocate_image);
	if (texture && (0 < width) && (0 < height) && (0 < depth) &&
		(0 < (number_of_components =
			Texture_storage_type_get_number_of_components(storage))) &&
		((1 == number_of_bytes_per_component) ||
			(2 == number_of_bytes_per_component)))
	{
		return_code = 1;
		if (1 < depth)
		{
			dimension = 3;
		}
		else if (1 < height)
		{
			dimension = 2;
		}
		else
		{
			dimension = 1;
		}
		bytes_per_pixel = number_of_components * number_of_bytes_per_component;
		padded_width_bytes = 4*((width*bytes_per_pixel + 3)/4);

		// avoid allocation if already correct size
		if ((texture->original_width_texels != width) ||
			(texture->original_height_texels != height) ||
			(texture->original_depth_texels != depth) ||
			(Texture_storage_type_get_number_of_components(texture->storage) !=
				Texture_storage_type_get_number_of_components(storage)) ||
			(texture->number_of_bytes_per_component != number_of_bytes_per_component))
		{
			unsigned char *texture_image;
			if (REALLOCATE(texture_image, texture->image, unsigned char,
				depth*height*padded_width_bytes))
			{
				texture->image = texture_image;
				/* fill the image with zeros */
				memset(texture_image, 0, depth*height*padded_width_bytes);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Texture_allocate_image.  Could not reallocate texture image");
				return_code = 0;
			}
		}
		if (return_code)
		{
			/* assign values in the texture */
			texture->dimension = dimension;
			texture->storage = storage;
			texture->number_of_bytes_per_component =
				number_of_bytes_per_component;
			/* original size is intended to specify useful part of texture */
			texture->original_width_texels = width;
			texture->original_height_texels = height;
			texture->original_depth_texels = depth;
			texture->width_texels = width;
			texture->height_texels = height;
			texture->depth_texels = depth;
			if (texture->image_file_name)
				DEALLOCATE(texture->image_file_name);
			texture->image_file_name = source_name ? duplicate_string(source_name) : 0;
			texture->file_number_pattern = (char *)NULL;
			texture->start_file_number = 0;
			texture->stop_file_number = 0;
			texture->file_number_increment = 0;
			texture->crop_left_margin = 0;
			texture->crop_bottom_margin = 0;
			texture->crop_width = 0;
			texture->crop_height = 0;
			/* display list needs to be compiled again */
			texture->display_list_current = TEXTURE_COMPILE_STATE_NOT_COMPILED;
			return_code = 1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_allocate_image.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Texture_allocate_image */

enum Texture_storage_type Texture_get_storage_type(struct Texture *texture)
{
	if (texture)
		return texture->storage;
	return TEXTURE_STORAGE_TYPE_INVALID;
}


int Texture_set_storage_type(struct Texture *texture, enum Texture_storage_type storage_type)
{
	if (texture && (0 == texture->image_file_name))
	{
		texture->storage = storage_type;
		return CMZN_OK;
	}
	return CMZN_RESULT_ERROR_ARGUMENT;
}

struct Cmgui_image *Texture_get_image(struct Texture *texture)
/*******************************************************************************
LAST MODIFIED : 4 March 2002

DESCRIPTION :
Creates and returns a Cmgui_image from the image in <texture>, usually for
writing. Depth planes are stored as subimages of the returned structure.
Up to the calling function to DESTROY the returned Cmgui_image.
==============================================================================*/
{
	int bytes_per_pixel, i, number_of_components, return_code, width_bytes;
	unsigned char *source;
	struct Cmgui_image *cmgui_image, *next_cmgui_image;

	ENTER(Texture_get_image);
	cmgui_image = (struct Cmgui_image *)NULL;
	if (texture && (0 < (number_of_components =
		Texture_storage_type_get_number_of_components(texture->storage))) &&
		(0 < (bytes_per_pixel =
			number_of_components*texture->number_of_bytes_per_component)))
	{
		return_code = 1;
		width_bytes = 4*((texture->width_texels*bytes_per_pixel + 3)/4);
		source = texture->image;
		for (i = 0; (i < texture->original_depth_texels) && return_code; i++)
		{
			next_cmgui_image = Cmgui_image_constitute(
				texture->original_width_texels, texture->original_height_texels,
				number_of_components, texture->number_of_bytes_per_component,
				width_bytes, source);
			if (next_cmgui_image)
			{
				if (cmgui_image)
				{
					/* following "swallows" next_cmgui_image */
					if (!Cmgui_image_append(cmgui_image, &next_cmgui_image))
					{
						display_message(ERROR_MESSAGE,
							"Texture_get_image.  Could not append image");
						return_code = 0;
					}
				}
				else
				{
					/* first image */
					cmgui_image = next_cmgui_image;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Texture_get_image.  Could not constitute image");
				return_code = 0;
			}
			source += texture->height_texels*width_bytes;
		}
		if (!return_code)
		{
			if (cmgui_image)
			{
				DESTROY(Cmgui_image)(&cmgui_image);
				cmgui_image = (struct Cmgui_image *)NULL;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE, "Texture_get_image.  Invalid texture");
	}
	LEAVE;

	return (cmgui_image);
} /* Texture_get_image */

int Texture_set_image(struct Texture *texture,
	struct Cmgui_image *cmgui_image,
	const char *image_file_name, const char *file_number_pattern,
	int start_file_number, int stop_file_number, int file_number_increment,
	int	crop_left, int crop_bottom, int crop_width, int crop_height)
/*******************************************************************************
LAST MODIFIED : 14 March 2002

DESCRIPTION :
Puts <cmgui_image> into <texture>. If the <cmgui_image> contains
more than one image, these are put together into a 3-D texture. All other
parameters are merely recorded with the texture to be able to reproduce the
command for it later. The exception is the crop parameters, which must
which are used to cut the image size if <crop_width> and <crop_height> are
positive. Cropping is not available in the depth direction.
==============================================================================*/
{
	static unsigned char fill_byte = 0;
	enum Texture_storage_type storage;
	int bytes_per_pixel, dimension, padded_width_bytes,
		texture_bottom = 0, texture_left = 0,
		image_height, image_width, k, number_of_bytes_per_component,
		number_of_components, number_of_images,
		return_code, texture_depth, texture_height = 0, texture_width = 0;
	unsigned char *texture_image;
	unsigned char *destination;

	ENTER(Texture_set_image);
	if (texture && cmgui_image &&
		(0 < (image_width = Cmgui_image_get_width(cmgui_image))) &&
		(0 < (image_height = Cmgui_image_get_height(cmgui_image))) &&
		(0 < (number_of_components =
			Cmgui_image_get_number_of_components(cmgui_image))) &&
		(0 < (number_of_bytes_per_component =
			Cmgui_image_get_number_of_bytes_per_component(cmgui_image))) &&
		(0 < (number_of_images = Cmgui_image_get_number_of_images(cmgui_image))))
	{
		return_code = 1;
		if ((0 == crop_left) && (0 == crop_width) &&
			(0 == crop_bottom) && (0 == crop_height))
		{
			texture_width = image_width;
			texture_height = image_height;
			texture_left = 0;
			texture_bottom = 0;
		}
		else if ((0 <= crop_left) && (0 < crop_width) &&
			(crop_left + crop_width <= image_width) &&
			(0 <= crop_bottom) && (0 < crop_height) &&
			(crop_bottom + crop_height <= image_height))
		{
			texture_width = crop_width;
			texture_height = crop_height;
			texture_left = crop_left;
			texture_bottom = crop_bottom;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Texture_set_image.  Invalid cropping parameters");
			return_code = 0;
		}
		texture_depth = number_of_images;
		switch (number_of_components)
		{
			case 1:
			{
				storage = TEXTURE_LUMINANCE;
			} break;
			case 2:
			{
				storage = TEXTURE_LUMINANCE_ALPHA;
			} break;
			case 3:
			{
				storage = TEXTURE_RGB;
			} break;
			case 4:
			{
				storage = TEXTURE_RGBA;
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"Texture_set_image.  Invalid number_of_components");
				return_code = 0;
			} break;
		}
		if (return_code)
		{
			if (1 < texture_depth)
			{
				dimension = 3;
			}
			else if (1 < texture_height)
			{
				dimension = 2;
			}
			else
			{
				dimension = 1;
			}
			/* ensure texture images are row aligned to 4-byte boundary */
			bytes_per_pixel = number_of_components * number_of_bytes_per_component;
			padded_width_bytes =
				4*((texture_width*bytes_per_pixel + 3)/4);
			if (ALLOCATE(texture_image, unsigned char,
				texture_depth*texture_height*padded_width_bytes))
			{
				destination = texture_image;
				for (k = 0; (k < texture_depth) && return_code; k++)
				{
					/* fill image from bottom to top */
					return_code = Cmgui_image_dispatch(cmgui_image, /*image_number*/k,
						texture_left, texture_bottom, texture_width, texture_height,
						padded_width_bytes, /*number_of_fill_bytes*/1, &fill_byte,
									   0,destination);
					destination += padded_width_bytes * texture_height;
				}

				/* assign values in the texture */
				texture->dimension = dimension;
				texture->storage = storage;
				texture->number_of_bytes_per_component =
					number_of_bytes_per_component;
				/* original size is intended to specify useful part of texture */
				texture->original_width_texels = texture_width;
				texture->original_height_texels = texture_height;
				texture->original_depth_texels = texture_depth;
				texture->width_texels = texture_width;
				texture->height_texels = texture_height;
				texture->depth_texels = texture_depth;
				DEALLOCATE(texture->image);
				texture->image = texture_image;
				if (texture->image_file_name)
				{
					DEALLOCATE(texture->image_file_name);
				}
				if (image_file_name)
				{
					texture->image_file_name = duplicate_string(image_file_name);
				}
				else
				{
					texture->image_file_name = (char *)NULL;
				}
				if (texture->file_number_pattern)
				{
					DEALLOCATE(texture->file_number_pattern);
				}
				if (file_number_pattern)
				{
					texture->file_number_pattern =
						duplicate_string(file_number_pattern);
				}
				else
				{
					texture->file_number_pattern = (char *)NULL;
				}
				texture->start_file_number = start_file_number;
				texture->stop_file_number = stop_file_number;
				texture->file_number_increment = file_number_increment;
				texture->crop_left_margin = crop_left;
				texture->crop_bottom_margin = crop_bottom;
				texture->crop_width = crop_width;
				texture->crop_height = crop_height;
				/* display list needs to be compiled again */
				texture->display_list_current = TEXTURE_COMPILE_STATE_NOT_COMPILED;
				return_code = 1;
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Texture_set_image.  Could not allocate texture image");
				return_code = 0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE, "Texture_set_image.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Texture_set_image */

int Texture_set_image_block(struct Texture *texture,
	int left, int bottom, int width, int height, int depth_plane,
	int source_width_bytes, unsigned char *source_pixels)
/*******************************************************************************
LAST MODIFIED : 21 August 2002

DESCRIPTION :
Over-writes a block of texels in <texture>. The block is one texel in depth
and is placed in the given <depth_plane> which ranges from 0 to one
less than depth_texels. The block is placed offset from the left by <left> and
from the bottom by <bottom> and is <width>*<height> in size. The block must be
wholly within the texture width_texels and height_texels.
Image data is taken from <source_pixels> which is expected to match the
texture's current storage type and number_of_bytes_per_component. Source pixels
are expected in rows from bottom to top, and within rows from left to right.
The <source_width_bytes> is the offset between row data in <source_pixels>
and must be at least width*number_of_components*number_of_bytes_per_component
in size.
==============================================================================*/
{
	int bytes_per_pixel, copy_width, i, number_of_components, return_code,
		width_bytes;
	unsigned char *destination, *source;
	ENTER(Texture_set_image_block);
	if (texture && (0 <= left) && (0 < width) &&
		(left + width <= texture->width_texels) &&
		(0 <= bottom) && (0 < height) &&
		(bottom + height <= texture->height_texels) &&
		(0 <= depth_plane) && (depth_plane < texture->depth_texels) &&
		(0 < (number_of_components =
			Texture_storage_type_get_number_of_components(texture->storage))) &&
		(0 < (bytes_per_pixel =
			number_of_components*texture->number_of_bytes_per_component)) &&
		(width*bytes_per_pixel <= source_width_bytes) &&
		source_pixels)
	{
		width_bytes = 4*((texture->width_texels*bytes_per_pixel + 3)/4);
		copy_width = width*bytes_per_pixel;
		destination = texture->image +
			(depth_plane*texture->height_texels + bottom)*width_bytes +
			left*bytes_per_pixel;
		source = source_pixels;
		for (i = 0; i < height; i++)
		{
			memcpy(destination, source, copy_width);
			destination += width_bytes;
			source += source_width_bytes;
		}
		/* display list needs to be compiled again */
		texture->display_list_current = TEXTURE_COMPILE_STATE_NOT_COMPILED;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_set_image_block.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Texture_set_image_block */

int Texture_get_image_block(struct Texture *texture,
	const void **buffer_out, unsigned int *buffer_length_out)
{
	int bytes_per_pixel = 0, number_of_components = 0;
	if (buffer_out)
	{
		if (texture && (texture->width_texels > 0) &&  (texture->height_texels > 0) &&
			(texture->depth_texels > 0) &&
			(0 < (number_of_components =
				Texture_storage_type_get_number_of_components(texture->storage))) &&
			(0 < (bytes_per_pixel =
				number_of_components*texture->number_of_bytes_per_component)))
		{
			int source_width_bytes = texture->width_texels * bytes_per_pixel;
			int total_size = texture->depth_texels*texture->height_texels*source_width_bytes;
			*buffer_out = static_cast<void *>(texture->image);
			*buffer_length_out = (unsigned int)total_size;
			return CMZN_OK;
		}
		else
		{
			*buffer_out = 0;
			*buffer_length_out = 0;
		}
	}

	return CMZN_RESULT_ERROR_ARGUMENT;
}

int Texture_fill_image_block(struct Texture *texture, unsigned char *source_pixels,
	unsigned int buffer_length)
{
	int bytes_per_pixel = 0, number_of_components = 0;
	if (source_pixels && texture && (texture->width_texels > 0) &&  (texture->height_texels > 0) &&
		(texture->depth_texels > 0) &&
		(0 < (number_of_components =
			Texture_storage_type_get_number_of_components(texture->storage))) &&
		(0 < (bytes_per_pixel =
			number_of_components*texture->number_of_bytes_per_component)))
	{
		int source_width_bytes = texture->width_texels * bytes_per_pixel;
		int total_size = texture->depth_texels*texture->height_texels*source_width_bytes;
		if (total_size == (int)buffer_length)
		{
			// Only set the image if image_file_name is not set as the iamge is not from source
			// or files/memory.
			if (texture->image_file_name == 0)
			{
				Texture_allocate_image(texture,
					texture->width_texels, texture->height_texels, texture->depth_texels,
					texture->storage, texture->number_of_bytes_per_component, "user_buffer");
			}
			if (strcmp(texture->image_file_name, "user_buffer") == 0)
			{
				memcpy(texture->image, source_pixels, buffer_length);
				texture->display_list_current = TEXTURE_COMPILE_STATE_NOT_COMPILED;
				return CMZN_OK;
			}
		}
	}

	return CMZN_RESULT_ERROR_ARGUMENT;
}

int Texture_add_image(struct Texture *texture,
	struct Cmgui_image *cmgui_image,
	int crop_left, int crop_bottom, int crop_width, int crop_height)
/*******************************************************************************
LAST MODIFIED : 14 February 2003

DESCRIPTION :
Adds <cmgui_image> into <texture> making a 3D image from 2D images.
==============================================================================*/
{
	static unsigned char fill_byte = 0;
	int allocate_texture_depth, bytes_per_pixel, dimension, texture_bottom = 0,
		texture_left = 0, i, image_height, image_width, k, number_of_bytes_per_component,
		number_of_components, number_of_images, return_code;
	long int padded_width_bytes,
		texture_depth, texture_height = 0, texture_width = 0;
	unsigned char *texture_image;
	unsigned char *destination;

	ENTER(Texture_set_image);
	if (texture && cmgui_image &&
		(0 < (image_width = Cmgui_image_get_width(cmgui_image))) &&
		(0 < (image_height = Cmgui_image_get_height(cmgui_image))) &&
		(0 < (number_of_components =
			Cmgui_image_get_number_of_components(cmgui_image))) &&
		(0 < (number_of_bytes_per_component =
			Cmgui_image_get_number_of_bytes_per_component(cmgui_image))) &&
		(0 < (number_of_images = Cmgui_image_get_number_of_images(cmgui_image))))
	{
		return_code = 1;
		if ((0 == crop_left) && (0 == crop_width) &&
			(0 == crop_bottom) && (0 == crop_height))
		{
			texture_width = image_width;
			texture_height = image_height;
			texture_left = 0;
			texture_bottom = 0;
		}
		else if ((0 <= crop_left) && (0 < crop_width) &&
			(crop_left + crop_width <= image_width) &&
			(0 <= crop_bottom) && (0 < crop_height) &&
			(crop_bottom + crop_height <= image_height))
		{
			texture_width = crop_width;
			texture_height = crop_height;
			texture_left = crop_left;
			texture_bottom = crop_bottom;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Texture_add_image.  Invalid cropping parameters");
			return_code = 0;
		}
		texture_depth = number_of_images + texture->original_depth_texels;
		if (number_of_components != Texture_storage_type_get_number_of_components
			(texture->storage))
		{
			display_message(ERROR_MESSAGE, "Texture_add_image.  "
				"Number of components in new images are not consistent with existing texture.");
			return_code = 0;
		}
		if (texture_width != texture->original_width_texels)
		{
			display_message(ERROR_MESSAGE, "Texture_add_image.  "
				"Width of new images are not consistent with existing texture.");
			return_code = 0;
		}
		if (texture_height != texture->original_height_texels)
		{
			display_message(ERROR_MESSAGE, "Texture_add_image.  "
				"Height of new images are not consistent with existing texture.");
			return_code = 0;
		}
		if (return_code)
		{
			texture_width = texture->width_texels;
			texture_height = texture->height_texels;
			if (1 < texture_depth)
			{
				dimension = 3;
			}
			else if (1 < texture_height)
			{
				dimension = 2;
			}
			else
			{
				dimension = 1;
			}
			/* ensure texture images are row aligned to 4-byte boundary */
			bytes_per_pixel = number_of_components * number_of_bytes_per_component;
			padded_width_bytes =
				4*((texture_width*bytes_per_pixel + 3)/4);
			/* Allocate depth in blocks.  Maybe I should just allow the malloc
				implementation to do this, however this also worked around a bug
				that was arising in valgrind where valgrind would crash on the
				realloc for some of the examples. */
			allocate_texture_depth = 5 * ((texture_depth + 4) / 5);
			if (REALLOCATE(texture_image, texture->image, unsigned char,
					allocate_texture_depth*texture_height*
					padded_width_bytes))
			{
				destination = texture_image + (long int)texture->original_depth_texels *
					padded_width_bytes * texture_height;
				i = 0;
				for (k = texture->original_depth_texels ;
					  (k < texture_depth) && return_code; k++)
				{
					/* fill image from bottom to top */
					return_code = Cmgui_image_dispatch(cmgui_image, /*image_number*/i,
						texture_left, texture_bottom, texture_width, texture_height,
						padded_width_bytes, /*number_of_fill_bytes*/1, &fill_byte,
									   0, destination);
					i++;
					destination += padded_width_bytes * texture_height;
				}

				/* assign values in the texture */
				texture->image = texture_image;
				texture->dimension = dimension;
				/* original size is intended to specify useful part of texture */
				texture->original_width_texels = texture_width;
				texture->original_height_texels = texture_height;
				texture->original_depth_texels = texture_depth;
				texture->width_texels = texture_width;
				texture->height_texels = texture_height;
				texture->depth_texels = texture_depth;
				/* display list needs to be compiled again */
				texture->display_list_current = TEXTURE_COMPILE_STATE_NOT_COMPILED;
				return_code = 1;
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Texture_add_image.  Could not allocate texture image");
				return_code = 0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE, "Texture_add_image.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Texture_add_image */

int Texture_get_raw_pixel_values(struct Texture *texture,int x,int y,int z,
	unsigned char *values)
/*******************************************************************************
LAST MODIFIED : 21 March 2002

DESCRIPTION :
Returns the byte values in the texture at x,y,z.
==============================================================================*/
{
	int i,number_of_bytes, return_code,row_width_bytes;
	unsigned char *pixel_ptr;

	ENTER(Texture_get_raw_pixel_values);
	if (texture&&(0<=x)&&(x<texture->original_width_texels)&&
		(0<=y)&&(y<texture->original_height_texels)&&
		(0<=z)&&(z<texture->original_depth_texels)&&values)
	{
		number_of_bytes = Texture_storage_type_get_number_of_components(texture->storage)
			* texture->number_of_bytes_per_component;
		row_width_bytes=
			((int)(texture->width_texels*number_of_bytes+3)/4)*4;
		pixel_ptr=(unsigned char *)(texture->image);
		pixel_ptr += ((z*texture->height_texels + y)*row_width_bytes+x*number_of_bytes);
		for (i=0;i<number_of_bytes;i++)
		{
			values[i]=pixel_ptr[i];
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_get_raw_pixel_values.  Invalid arguments");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Texture_get_raw_pixel_values */

int Texture_get_pixel_values(struct Texture *texture,
	ZnReal x, ZnReal y, ZnReal z, ZnReal *values)
/*******************************************************************************
LAST MODIFIED : 28 August 2002

DESCRIPTION :
Returns the byte values in the texture using the texture coordinates relative
to the physical size.  Each texel is assumed to apply exactly
at its centre and the filter_mode used to determine whether the pixels are
interpolated or not.  When closer than half a texel to a boundary the colour
is constant from the half texel location to the edge.
==============================================================================*/
{
	ZnReal local_xi[3], max_v, pos[3], weight, weight_i, weight_j, weight_k, v;
	int bytes_per_pixel, component_max, dimension, i, in_border, j, k,
		max_i, max_j, max_k, n, number_of_bytes_per_component,
		number_of_components, original_size[3], return_code, row_width_bytes,
		size[3];
	long int high_offset[3], low_offset[3], offset, offset_i, offset_j, offset_k,
		v_i, x_i, y_i, z_i;
	unsigned char *pixel_ptr;
	unsigned short short_value;

	ENTER(Texture_get_pixel_values);
	if (texture && values)
	{
		return_code = 1;
		number_of_components =
			Texture_storage_type_get_number_of_components(texture->storage);
		number_of_bytes_per_component = texture->number_of_bytes_per_component;
		bytes_per_pixel = number_of_components*number_of_bytes_per_component;
		row_width_bytes=
			((int)(texture->width_texels*bytes_per_pixel+3)/4)*4;

		in_border = 0;
		switch (texture->wrap_mode)
		{
			/* SAB As far as I can tell we had actually implemented clamp_to_edge for
				normal clamp, so it is the same. It also behaves differently to the
			   OpenGL implementation where it uses the original_sizes.  Would be
				able to simplify this if we allowed non-power-of-2 textures. */
			case TEXTURE_CLAMP_WRAP:
			case TEXTURE_CLAMP_EDGE_WRAP:
			{
				if ((x < 0.0) || (texture->original_width_texels <= 1))
					x = 0.0;
				else if (x > texture->width)
					x = texture->original_width_texels;
				else
					x *= ((ZnReal)texture->original_width_texels / texture->width);

				if ((y < 0.0) || (texture->original_height_texels <= 1))
					y = 0.0;
				else if (y > texture->height)
					y = texture->original_height_texels;
				else
					y *= ((ZnReal)texture->original_height_texels / texture->height);

				if ((z < 0.0) || (texture->original_depth_texels <= 1))
					z = 0.0;
				else if (z > texture->depth)
					z = texture->original_depth_texels;
				else
					z *= ((ZnReal)texture->original_depth_texels / texture->depth);
			} break;
			case TEXTURE_CLAMP_BORDER_WRAP:
			{
				/* Technically we should be merging to the border using the
					current filter, so this is correct for nearest but the colour
					should blend to the border colour 1/2 a pixel outside the texture
					for linear.  We are also doing clamp to edge for the values inside
					the texture range rather than blending to the border colour. */
				if ((x < 0)||(x > texture->width))
				{
					x = 0.0;
					in_border = 1;
				}
				else if (texture->original_width_texels <= 1)
					x = 0.0;
				else
					x *= ((double)texture->original_width_texels / texture->width);

				if ((y < 0)||(y > texture->height))
				{
					y = 0.0;
					in_border = 1;
				}
				else if (texture->original_height_texels <= 1)
					y = 0.0;
				else
					y *= ((double)texture->original_height_texels / texture->height);

				if ((z < 0)||(z > texture->depth))
				{
					z = 0.0;
					in_border = 1;
				}
				else if (texture->original_depth_texels <= 1)
					z = 0.0;
				else
					z *= ((double)texture->original_depth_texels / texture->depth);
			} break;
			case TEXTURE_REPEAT_WRAP:
			{
				/* make x, y and z range from 0.0 to 1.0 over full texture size */
				if (texture->original_width_texels <= 1)
					x = 0.0;
				else
				{
					x *= ((double)texture->original_width_texels /
						(double)texture->width_texels) / texture->width;
					x -= floor(x);
					x *= (double)(texture->width_texels);
				}

				if (texture->original_height_texels <= 1)
					y = 0.0;
				else
				{
					y *= ((double)texture->original_height_texels /
						(double)texture->height_texels) / texture->height;
					y -= floor(y);
					y *= (double)(texture->height_texels);
				}

				if (texture->original_depth_texels <= 1)
					z = 0.0;
				else
				{
					z *= ((double)texture->original_depth_texels /
						(double)texture->depth_texels) / texture->depth;
					z -= floor(z);
					z *= (double)(texture->depth_texels);
				}
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"Texture_get_pixel_values.  Unknown wrap type");
				return_code = 0;
			} break;
		}
		if (2 == number_of_bytes_per_component)
		{
			component_max = 65535;
		}
		else
		{
			component_max = 255;
		}
		if (!in_border)
		{
			switch (texture->filter_mode)
			{
				case TEXTURE_LINEAR_FILTER:
				case TEXTURE_LINEAR_MIPMAP_NEAREST_FILTER:
				case TEXTURE_LINEAR_MIPMAP_LINEAR_FILTER:
				{
					dimension = texture->dimension;
					pos[0] = x;
					pos[1] = y;
					pos[2] = z;
					size[0] = texture->width_texels;
					size[1] = texture->height_texels;
					size[2] = texture->depth_texels;
					offset = bytes_per_pixel;
					switch (texture->wrap_mode)
					{
						case TEXTURE_CLAMP_BORDER_WRAP:
						/* We should handle this correctly as a border clamp, but lets do something anyway */
						case TEXTURE_CLAMP_WRAP:
						case TEXTURE_CLAMP_EDGE_WRAP:
						{
							/* note we clamp to the original size; not the power-of-2 */
							original_size[0] = texture->original_width_texels;
							original_size[1] = texture->original_height_texels;
							original_size[2] = texture->original_depth_texels;
							offset = bytes_per_pixel;
							for (i = 0; i < dimension; i++)
							{
								max_v = (double)original_size[i] - 0.5;
								v = pos[i];
								if ((0.5 <= v) && (v < max_v))
								{
									v_i = (long int)(v - 0.5);
									local_xi[i] = v - 0.5 - (double)v_i;
									low_offset[i] = v_i*offset;
									high_offset[i] = (v_i + 1)*offset;
								}
								else
								{
									/* I think this implements clamp to edge? */
									low_offset[i] = (long int)(original_size[i] - 1)*offset;
									high_offset[i] = 0;
									if (v < 0.5)
									{
										local_xi[i] = 1.0;
									}
									else
									{
										local_xi[i] = 0.0;
									}
								}
								if (i == 0)
								{
									offset = row_width_bytes;
								}
								else
								{
									offset *= (long int)size[i];
								}
							}
						} break;
						case TEXTURE_MIRRORED_REPEAT_WRAP:
						/* We should handle this correctly as a mirror, but lets do something anyway */
						case TEXTURE_REPEAT_WRAP:
						{
							for (i = 0; i < dimension; i++)
							{
								max_v = (double)size[i] - 0.5;
								v = pos[i];
								if ((0.5 <= v) && (v < max_v))
								{
									v_i = (long int)(v - 0.5);
									local_xi[i] = v - 0.5 - (double)v_i;
									low_offset[i] = v_i*offset;
									high_offset[i] = (v_i + 1)*offset;
								}
								else
								{
									low_offset[i] = (long int)(size[i] - 1)*offset;
									high_offset[i] = 0;
									if (v < 0.5)
									{
										local_xi[i] = v + 0.5;
									}
									else
									{
										local_xi[i] = v - max_v;
									}
								}
								if (i == 0)
								{
									offset = row_width_bytes;
								}
								else
								{
									offset *= (long int)size[i];
								}
							}
						} break;
					}

					max_i = 2;
					if (1 < dimension)
					{
						max_j = 2;
					}
					else
					{
						max_j = 1;
					}
					if (2 < dimension)
					{
						max_k = 2;
					}
					else
					{
						max_k = 1;
					}

					for (i = 0; i < number_of_components; i++)
					{
						values[i] = 0.0;
					}

					for (k = 0; k < max_k; k++)
					{
						if (2 < dimension)
						{
							if (0 == k)
							{
								weight_k = (1.0 - local_xi[2]);
								offset_k = low_offset[2];
							}
							else
							{
								weight_k = local_xi[2];
								offset_k = high_offset[2];
							}
						}
						else
						{
							weight_k = 1.0;
							offset_k = 0;
						}
						for (j = 0; j < max_j; j++)
						{
							if (1 < dimension)
							{
								if (0 == j)
								{
									weight_j = weight_k*(1.0 - local_xi[1]);
									offset_j = offset_k + low_offset[1];
								}
								else
								{
									weight_j = weight_k*local_xi[1];
									offset_j = offset_k + high_offset[1];
								}
							}
							else
							{
								weight_j = 1.0;
								offset_j = 0;
							}
							for (i = 0; i < max_i; i++)
							{
								if (0 == i)
								{
									weight_i = weight_j*(1.0 - local_xi[0]);
									offset_i = offset_j + low_offset[0];
								}
								else
								{
									weight_i = weight_j*local_xi[0];
									offset_i = offset_j + high_offset[0];
								}
								pixel_ptr = texture->image + offset_i;
								weight = weight_i / component_max;
								for (n = 0; n < number_of_components; n++)
								{
									if (2 == number_of_bytes_per_component)
									{
#if (1234==BYTE_ORDER)
										short_value =
											(((unsigned short)(*(pixel_ptr + 1))) << 8) + (*pixel_ptr);
#else /* (1234==BYTE_ORDER) */
										short_value =
											(((unsigned short)(*pixel_ptr)) << 8) + (*(pixel_ptr + 1));
#endif /* (1234==BYTE_ORDER) */
										values[n] += (double)short_value * weight;
									}
									else
									{
										values[n] += (double)(*pixel_ptr) * weight;
									}
									pixel_ptr += number_of_bytes_per_component;
								}
							}
						}
					}
				} break;
				case TEXTURE_NEAREST_FILTER:
				case TEXTURE_NEAREST_MIPMAP_NEAREST_FILTER:
				{
					x_i = (int)x;
					y_i = (int)y;
					z_i = (int)z;
					if (TEXTURE_CLAMP_WRAP == texture->wrap_mode)
					{
						/* fix problem of value being exactly on upper boundary */
						if (x_i == texture->original_width_texels)
						{
							x_i--;
						}
						if (y_i == texture->original_height_texels)
						{
							y_i--;
						}
						if (z_i == texture->original_depth_texels)
						{
							z_i--;
						}
					}
					offset = (z_i*(long int)texture->height_texels + y_i)*(long int)row_width_bytes +
						x_i*(long int)bytes_per_pixel;
					pixel_ptr = texture->image + offset;
					for (n = 0; n < number_of_components; n++)
					{
						if (2 == number_of_bytes_per_component)
						{
#if (1234==BYTE_ORDER)
							short_value =
								(((unsigned short)(*(pixel_ptr + 1))) << 8) + (*pixel_ptr);
#else /* (1234==BYTE_ORDER) */
							short_value =
								(((unsigned short)(*pixel_ptr)) << 8) + (*(pixel_ptr + 1));
#endif /* (1234==BYTE_ORDER) */
							values[n] = (double)short_value / component_max;
						}
						else
						{
							values[n] = (double)(*pixel_ptr) / component_max;
						}
						pixel_ptr += number_of_bytes_per_component;
					}
				} break;
				default:
				{
					display_message(ERROR_MESSAGE,
						"Texture_get_pixel_values.  Unknown filter type");
					return_code = 0;
				}
			}
		}
		else
		{
			/* Use border colour */
			switch (texture->storage)
			{
				case TEXTURE_LUMINANCE:
				{
					/* Just use the red colour to be efficient */
					values[0] = (texture->combine_colour).red;
				} break;
				case TEXTURE_LUMINANCE_ALPHA:
				{
					values[0] = (texture->combine_colour).red;
					values[1] = texture->combine_alpha;
				} break;
				case TEXTURE_RGB:
				{
					values[0] = (texture->combine_colour).red;
					values[1] = (texture->combine_colour).green;
					values[2] = (texture->combine_colour).blue;
				} break;
				case TEXTURE_RGBA:
				{
					values[0] = (texture->combine_colour).red;
					values[1] = (texture->combine_colour).green;
					values[2] = (texture->combine_colour).blue;
					values[3] = texture->combine_alpha;
				} break;
				default:
				{
					display_message(ERROR_MESSAGE,  "Texture_get_pixel_values.  "
						"Border code not implemented for texture storage.");
					return_code = 0;
				} break;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_get_pixel_values.  Invalid arguments");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Texture_get_pixel_values */

char *Texture_get_image_file_name(struct Texture *texture)
/*******************************************************************************
LAST MODIFIED : 8 February 2002

DESCRIPTION :
Returns the name of the file from which the current texture image was read.
Note: returned file_number_pattern may be NULL if not set yet.
User must not modify the returned value!
==============================================================================*/
{
	char *image_file_name;

	ENTER(Texture_get_image_file_name);
	if (texture)
	{
		image_file_name = texture->image_file_name;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_get_image_file_name.  Missing texture");
		image_file_name = (char *)NULL;
	}
	LEAVE;

	return (image_file_name);
} /* Texture_get_image_file_name */

char *Texture_get_file_number_pattern(struct Texture *texture)
/*******************************************************************************
LAST MODIFIED : 8 February 2002

DESCRIPTION :
Returns the file number pattern substituted in the image_file_name with the
image_file_number when 3-D image series are read in.
Note: returned file_number_pattern may be NULL if not set yet.
User must not modify the returned value!
==============================================================================*/
{
	char *file_number_pattern;

	ENTER(Texture_get_file_number_pattern);
	if (texture)
	{
		file_number_pattern = texture->file_number_pattern;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_get_file_number_pattern.  Missing texture");
		file_number_pattern = (char *)NULL;
	}
	LEAVE;

	return (file_number_pattern);
} /* Texture_get_file_number_pattern */

int Texture_get_file_number_series(struct Texture *texture,
	int *start_file_number, int *stop_file_number, int *file_number_increment)
/*******************************************************************************
LAST MODIFIED : 20 February 2002

DESCRIPTION :
Returns the start, stop and increment of file_numbers which together with the
image_file_name and file_number_pattern determine the files read in for
3-D textures. The returned values are meaningless if the texture is not
three dimensional = depth_texels greater than 1.
==============================================================================*/
{
	int return_code;

	ENTER(Texture_get_file_number_series);
	if (texture && start_file_number && stop_file_number && file_number_increment)
	{
		*start_file_number = texture->start_file_number;
		*stop_file_number = texture->stop_file_number;
		*file_number_increment = texture->file_number_increment;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_get_file_number_series.  Missing texture");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Texture_get_file_number_series */

int Texture_uses_image_file_name(struct Texture *texture,
	void *image_file_name_void)
/*******************************************************************************
LAST MODIFIED : 13 March 1998

DESCRIPTION :
Returns true if <texture> contains the image from file <image_file_name>.
==============================================================================*/
{
	char *image_file_name;
	int return_code;

	ENTER(Texture_uses_image_file_name);
	if (texture&&(image_file_name=(char *)image_file_name_void))
	{
		if (texture->image_file_name)
		{
			return_code=(0==strcmp(texture->image_file_name,image_file_name));
		}
		else
		{
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_uses_image_file_name.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Texture_uses_image_file_name */

int Texture_get_number_of_bytes_per_component(struct Texture *texture)
/*******************************************************************************
LAST MODIFIED : 11 March 2002

DESCRIPTION :
Returns the number bytes in each component of the texture: 1 or 2.
==============================================================================*/
{
	int return_code;

	if (texture)
	{
		return_code = texture->number_of_bytes_per_component;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_get_number_of_bytes_per_component.  Missing texture");
		return_code = 0;
	}

	return (return_code);
} /* Texture_get_number_of_bytes_per_component */

int Texture_set_number_of_bytes_per_component(struct Texture *texture,
	int number_of_bytes)
{
	if ((texture && (0 == texture->image_file_name)) &&
			((number_of_bytes == 1) || (number_of_bytes ==2 )))
	{
		texture->number_of_bytes_per_component = number_of_bytes;
		return 1;
	}
	return 0;
}

int Texture_get_number_of_components(struct Texture *texture)
/*******************************************************************************
LAST MODIFIED : 11 March 2002

DESCRIPTION :
Returns the number of components used per texel in the texture: 1, 2, 3 or 4.
==============================================================================*/
{
	int return_code;

	ENTER(Texture_get_number_of_components);
	if (texture)
	{
		return_code =
			Texture_storage_type_get_number_of_components(texture->storage);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_get_number_of_components.  Missing texture");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Texture_get_number_of_components */

int cmzn_texture_get_graphics_storage_size(cmzn_texture_id texture)
/*******************************************************************************
LAST MODIFIED : 26 May 2007

DESCRIPTION :
Returns the amount of graphics memory used to store the texture.
If the texture is compressed then this parameter will be requested directly
from the graphics card, so if it hasn't been rendered yet will be undefined.
If the texture is not compressed then it is calculated from the texture
parameters.
==============================================================================*/
{
	int size;

	ENTER(Texture_get_graphics_storage_size);
	if (texture)
	{
#if defined (GL_ARB_texture_compression)
		if (Graphics_library_check_extension(GL_ARB_texture_compression) &&
			(TEXTURE_COMPRESSED_UNSPECIFIED == texture->compression_mode))
		{
			GLint texture_size;
			GLenum texture_target;

			texture_target = Texture_get_target_enum(texture);
			if (texture_target)
			{
				/* get the compressed texture size */
				glBindTexture(texture_target, texture->texture_id);
				glGetTexLevelParameteriv(texture_target, (GLint)0,
				  GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB, &texture_size);
				size = texture_size;
			}
			else
			{
				size = 0;
			}
		}
		else
		{
#endif /* defined (GL_ARB_texture_compression) */
			size = texture->width_texels * texture->height_texels *
				texture->depth_texels * texture->number_of_bytes_per_component *
				Texture_storage_type_get_number_of_components(texture->storage);
#if defined (GL_ARB_texture_compression)
		}
#endif /* defined (GL_ARB_texture_compression) */
	}
	else
	{
		display_message(ERROR_MESSAGE, "Texture_get_graphics_storage_size.  "
			"Invalid argument(s)");
		size = 0;
	}
	LEAVE;

	return (size);
} /* Texture_get_graphics_storage_size */

int cmzn_texture_get_graphics_parameter(cmzn_texture_id texture,
	cmzn_texture_graphics_parameter_id graphics_parameter)
/*******************************************************************************
LAST MODIFIED : 12 October 2009

DESCRIPTION :
Returns information from the graphics system about the storage.
These will only be defined if they have been rendered.
==============================================================================*/
{
	int return_value;
#if defined (OPENGL_API)
	GLenum texture_target;
#endif

	ENTER(cmzn_texture_get_graphics_parameter);
	if (texture)
	{
#if defined (OPENGL_API)
		texture_target = Texture_get_target_enum(texture);
		if (texture_target && texture->texture_id)
		{
			GLenum gl_parameter;
			GLint gl_value;

			/* get the corresponding value */
			switch (graphics_parameter)
			{
			/* LUMINANCE is deprecated in OpenGL.
			 */
			case CMZN_TEXTURE_LUMINANCE_SIZE:
			{
				gl_parameter = GL_TEXTURE_LUMINANCE_SIZE;
			} break;
			default:
			{
				gl_parameter = 0;
			}
			}
			if (gl_parameter)
			{
				glBindTexture(texture_target, texture->texture_id);
				glGetTexLevelParameteriv(texture_target, (GLint)0,
				  gl_parameter, &gl_value);
				return_value = gl_value;
			}
			else
			{
				return_value = 0;
			}
		}
		else
		{
			return_value = 0;
		}
#else
		USE_PARAMETER(graphics_parameter);
		return_value = 0;
#endif
	}
	else
	{
		display_message(ERROR_MESSAGE, "Texture_get_graphics_storage_size.  "
			"Invalid argument(s)");
		return_value = 0;
	}
	LEAVE;

	return (return_value);
} /* Texture_get_graphics_storage_size */

int Texture_get_original_size(struct Texture *texture,
	int *original_width_texels, int *original_height_texels,
	int *original_depth_texels)
/*******************************************************************************
LAST MODIFIED : 8 February 2002

DESCRIPTION :
Returns the width, height and depth of the image from which the texture was
read. May differ from the dimensions of the texture which is in powers of 2.
==============================================================================*/
{
	int return_code;

	ENTER(Texture_get_original_size);
	if (texture && original_width_texels && original_height_texels &&
		original_depth_texels)
	{
		*original_width_texels = texture->original_width_texels;
		*original_height_texels = texture->original_height_texels;
		*original_depth_texels = texture->original_depth_texels;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_get_original_size.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Texture_get_original_size */

int Texture_get_physical_size(struct Texture *texture,ZnReal *width,
	ZnReal *height, ZnReal *depth)
/*******************************************************************************
LAST MODIFIED : 8 February 2002

DESCRIPTION :
Returns the physical size in model coordinates of the original texture image.
==============================================================================*/
{
	int return_code;

	ENTER(Texture_get_physical_size);
	if (texture && width && height && depth)
	{
		*width = texture->width;
		*height = texture->height;
		*depth = texture->depth;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_get_physical_size.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Texture_get_physical_size */

int Texture_set_physical_size(struct Texture *texture,
	ZnReal width, ZnReal height, ZnReal depth)
/*******************************************************************************
LAST MODIFIED : 8 February 2002

DESCRIPTION :
Sets the physical size in model coordinates of the original texture image. The
default is 1.0, so that texture coordinates in the range from 0 to 1 represent
real image data and not padding to make image sizes up to powers of 2.
==============================================================================*/
{
	int return_code;

	ENTER(Texture_set_physical_size);
	if (texture && (0.0 < width) && (0.0 < height) && (0.0 < depth))
	{
		if ((width != texture->width) || (height != texture->height) ||
			(depth != texture->depth))
		{
			texture->width = width;
			texture->height = height;
			texture->depth = depth;
			/* display list needs to be compiled again */
			texture->display_list_current = TEXTURE_COMPILE_STATE_NOT_COMPILED;
		}
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_set_physical_size.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Texture_set_physical_size */

int cmzn_texture_get_texture_coordinate_sizes(cmzn_texture_id texture,
	int dimension, double *sizesOut)
/*******************************************************************************
Gets the texture coordinates sizes of the texture.
This is the same as the physical size above.  When rendered the
texture will be rendered mapping the texture coordinates [0,0,0] to the bottom
left of the texture and
[textureCoordinateWidth, textureCoordinateHeight, textureCoordinateDepth] to
the top right of the texture.
==============================================================================*/
{
	if (texture && (0 < dimension) && (sizesOut))
	{
		for (int i = 0; i < dimension; i++)
		{
			if (i ==0)
				sizesOut[i] = texture->width;
			else if (i == 1)
				sizesOut[i] = texture->height;
			else if (i == 2)
				sizesOut[i] = texture->depth;
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_texture_set_texture_coordinate_sizes(cmzn_texture_id texture,
   int dimension, const double *sizes)
/*******************************************************************************
LAST MODIFIED : 26 May 2007

DESCRIPTION :
Returns the texture coordinates sizes of the texture.
This is the same as the physical size above.  When rendered the
texture will be rendered mapping the texture coordinates [0,0,0] to the bottom
left of the texture and
[textureCoordinateWidth, textureCoordinateHeight, textureCoordinateDepth] to
the top right of the texture.
==============================================================================*/
{
	if (texture && (dimension > 0) && (sizes))
	{
		for (int i = 0; i < dimension; ++i)
			if (sizes[i] <= 0.0)
				return CMZN_ERROR_ARGUMENT;
		if (texture->width != sizes[0])
		{
			texture->width = sizes[0];
			texture->display_list_current = TEXTURE_COMPILE_STATE_NOT_COMPILED;
		}
		if (dimension > 1)
		{
			if (texture->height != sizes[1])
			{
				texture->height = sizes[1];
				texture->display_list_current = TEXTURE_COMPILE_STATE_NOT_COMPILED;
			}
			if (dimension > 2)
			{
				if (texture->depth != sizes[2])
				{
					texture->depth = sizes[2];
					texture->display_list_current = TEXTURE_COMPILE_STATE_NOT_COMPILED;
				}
			}
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int Texture_get_distortion_info(struct Texture *texture,
	ZnReal *distortion_centre_x,ZnReal *distortion_centre_y,
	ZnReal *distortion_factor_k1)
/*******************************************************************************
LAST MODIFIED : 28 September 1999

DESCRIPTION :
Returns the radial distortion information stored with the texture as
<centre_x,centre_y> with <factor_k1>, all in the physical space of the image,
from (0.0,0.0) to (texture->width,texture->height).
==============================================================================*/
{
	int return_code;

	ENTER(Texture_get_distortion_info);
	if (texture&&distortion_centre_x&&distortion_centre_y&&distortion_factor_k1)
	{
		*distortion_centre_x = texture->distortion_centre_x;
		*distortion_centre_y = texture->distortion_centre_y;
		*distortion_factor_k1 = texture->distortion_factor_k1;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_get_distortion_info.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Texture_get_distortion_info */

int Texture_set_distortion_info(struct Texture *texture,
	ZnReal distortion_centre_x,ZnReal distortion_centre_y,
	ZnReal distortion_factor_k1)
/*******************************************************************************
LAST MODIFIED : 28 September 1999

DESCRIPTION :
Records with the texture that the lens from which it was produced had radial
distortion centred at <centre_x,centre_y> with <factor_k1>, and hence is to be
considered as distorted with such values. These values are relative to the
physical size of the image, from (0.0,0.0) to (texture->width,texture->height).
==============================================================================*/
{
	int return_code;

	ENTER(Texture_set_distortion_info);
	if (texture)
	{
		texture->distortion_centre_x = distortion_centre_x;
		texture->distortion_centre_y = distortion_centre_y;
		texture->distortion_factor_k1 = distortion_factor_k1;
		/* texture display list DOES NOT need to be compiled. Distortion info
			 is merely made available to users of the texture for a consistent look
			 if it decides to correct it */
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_set_distortion_info.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Texture_set_distortion_info */

int Texture_get_size(struct Texture *texture,
	int *width_texels, int *height_texels, int *depth_texels)
/*******************************************************************************
LAST MODIFIED : 8 February 2002

DESCRIPTION :
Returns the dimensions of the texture image as stored, these may have been
modified to match the requirements of the current OpenGL display.
The width, height and depth returned are at least as large as those of the
original image read into the texture.
==============================================================================*/
{
	int return_code;

	ENTER(Texture_get_size);
	if (texture && width_texels && height_texels && depth_texels)
	{
		*width_texels = texture->width_texels;
		*height_texels = texture->height_texels;
		*depth_texels = texture->depth_texels;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE, "Texture_get_size.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Texture_get_size */

int Texture_get_dimension(struct Texture *texture, int *dimension)
/*******************************************************************************
LAST MODIFIED : 24 May 2007

DESCRIPTION :
Returns the dimension of the texture image.
==============================================================================*/
{
	int return_code;
	ENTER(Texture_get_dimension);
	if (texture)
	{
		*dimension = texture->dimension;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_get_dimension.  Missing texture");
		return_code=0;
	}
	LEAVE;

	return (return_code);
}/* Texture_get_dimension */

int cmzn_texture_get_pixel_sizes(struct Texture *texture,
	int dimension, int *sizesOut)
/*******************************************************************************
LAST MODIFIED : 25 May 2007

DESCRIPTION :
Returns the original texel sizes of the texture.  These may have been
subsequently modified by cmgui such as to support platforms which require
each size to be a power of two.
==============================================================================*/
{
	if (texture)
	{
		for (int i = 0; i < dimension; i++)
		{
			if (i ==0)
				sizesOut[i] = texture->original_width_texels;
			else if (i == 1)
				sizesOut[i] = texture->original_height_texels;
			else if (i == 2)
				sizesOut[i] = texture->original_depth_texels;
		}
		return texture->dimension;
	}
	return 0;

}

int cmzn_texture_get_rendered_texel_sizes(struct Texture *texture,
	unsigned int *dimension, unsigned int **sizes)
/*******************************************************************************
LAST MODIFIED : 29 June 2007

DESCRIPTION :
Returns the rendered texel sizes of the texture.  These indicate what sizes
were actually loaded into OpenGL and until the texture is rendered will be
zero.
==============================================================================*/
{
	int return_code;

	ENTER(Texture_get_rendered_texel_sizes);
	if (texture)
	{
		if (ALLOCATE(*sizes, unsigned int, texture->dimension))
		{
			*dimension = texture->dimension;
			if (texture->dimension > 0)
			{
				(*sizes)[0] = texture->rendered_width_texels;
				if (texture->dimension > 1)
				{
					(*sizes)[1] = texture->rendered_height_texels;
					if (texture->dimension > 2)
					{
						(*sizes)[2] = texture->rendered_depth_texels;
					}
				}
			}
			return_code = 1;
		}
		else
		{
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE, "Texture_get_rendered_texel_sizes.  "
			"Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Texture_get_rendered_texel_sizes */

enum Texture_wrap_mode Texture_get_wrap_mode(struct Texture *texture)
/*******************************************************************************
LAST MODIFIED : 11 October 2000

DESCRIPTION :
Returns how textures coordinates outside [0,1] are handled.
==============================================================================*/
{
	enum Texture_wrap_mode wrap_mode = TEXTURE_CLAMP_WRAP;

	ENTER(Texture_get_wrap_mode);
	if (texture)
	{
		wrap_mode = texture->wrap_mode;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_get_wrap_mode.  Invalid argument(s)");
	}
	LEAVE;

	return (wrap_mode);
} /* Texture_get_wrap_mode */

int Texture_set_wrap_mode(struct Texture *texture,
	enum Texture_wrap_mode wrap_mode)
/*******************************************************************************
LAST MODIFIED : 11 October 2000

DESCRIPTION :
Sets how textures coordinates outside [0,1] are handled.
==============================================================================*/
{
	const char *type_string;
	int return_code;

	ENTER(Texture_set_wrap_mode);
	if (texture)
	{
		if((TEXTURE_CLAMP_WRAP==wrap_mode)
			||(TEXTURE_REPEAT_WRAP==wrap_mode)
#if defined (GL_VERSION_1_2)
			/* Cannot do the runtime test here as the graphics may not be
				initialised yet or we may just want to evaluate the texture as a field */
			|| (TEXTURE_CLAMP_EDGE_WRAP==wrap_mode)
#endif /* defined (GL_VERSION_1_2) */
#if defined (GL_VERSION_1_3)
			|| (TEXTURE_CLAMP_BORDER_WRAP==wrap_mode)
#endif /* defined (GL_VERSION_1_3) */
#if defined (GL_VERSION_1_4)
			|| (TEXTURE_MIRRORED_REPEAT_WRAP==wrap_mode)
#endif /* defined (GL_VERSION_1_4) */
			)
		{
			if (wrap_mode != texture->wrap_mode)
			{
				texture->wrap_mode = wrap_mode;
				/* display list needs to be compiled again */
			texture->display_list_current=TEXTURE_COMPILE_STATE_NOT_COMPILED;
			}
			return_code=1;
		}
		else
		{
			type_string = ENUMERATOR_STRING(Texture_wrap_mode)(wrap_mode);
			if (type_string)
			{
				display_message(ERROR_MESSAGE,  "Texture_set_wrap_mode.  "
					"Texture wrap mode %s was not compiled into this executable.", type_string);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Texture_set_wrap_mode.  Invalid wrap type.");
			}
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_set_wrap_mode.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Texture_set_wrap_mode */

int cmzn_texture_write_to_file(cmzn_texture_id texture,
   const char *filename)
/*******************************************************************************
LAST MODIFIED : 27 June 2007

DESCRIPTION :
Writes the <texture> to file <filename>.
I think it is best to write a separate function if you want to write a
3D texture to a file sequence rather than handle it with this function.
==============================================================================*/
{
	struct Cmgui_image *cmgui_image;
	struct Cmgui_image_information *cmgui_image_information;
	int return_code;

	ENTER(cmzn_texture_write_to_file);
	if (texture)
	{
		cmgui_image_information = CREATE(Cmgui_image_information)();
		Cmgui_image_information_add_file_name(cmgui_image_information,
			(char *)filename);
		cmgui_image = Texture_get_image(texture);
		if (cmgui_image)
		{
			if (Cmgui_image_write(cmgui_image, cmgui_image_information))
			{
				return_code = 1;
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"cmzn_texture_write_to_file:  "
					"Error writing image %s", filename);
				return_code = 0;
			}
			DESTROY(Cmgui_image)(&cmgui_image);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"cmzn_texture_write_to_file:  "
				"Could not get image from texture");
			return_code = 0;
		}
		DESTROY(Cmgui_image_information)(&cmgui_image_information);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_texture_write_to_file:  "
			"Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_texture_write_to_file */

static int list_Texture_property(struct Texture_property *texture_property,
	void *dummy)
/*******************************************************************************
LAST MODIFIED : 25 October 2007

DESCRIPTION :
Writes the <texture_property> to the command window.
==============================================================================*/
{
	int return_code;

	ENTER(list_Texture_property);
	USE_PARAMETER(dummy);
	if (texture_property)
	{
		return_code=1;
		display_message(INFORMATION_MESSAGE,"     %s : %s\n",
			texture_property->name, texture_property->value);
	}
	else
	{
		display_message(ERROR_MESSAGE,"list_Texture_property.  Invalid argument");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* list_Texture_property */

int list_Texture(struct Texture *texture,void *dummy)
/*******************************************************************************
LAST MODIFIED : 8 February 2002

DESCRIPTION :
Writes the properties of the <texture> to the command window.
==============================================================================*/
{
	int return_code;

	ENTER(list_Texture);
	USE_PARAMETER(dummy);
	if (texture)
	{
		return_code=1;
		/* write the name */
		display_message(INFORMATION_MESSAGE,"texture : %s\n",texture->name);
		/* write the texture size in model units */
		display_message(INFORMATION_MESSAGE,
			"  size in model units.  width = %g, height = %g, depth = %g\n",
			texture->width, texture->height, texture->depth);
		/* write the radial distortion parameters in physical space */
		display_message(INFORMATION_MESSAGE,"  radial distortion in model space.  "
			"centre = %g,%g  factor_k1 = %g\n",texture->distortion_centre_x,
			texture->distortion_centre_y,texture->distortion_factor_k1);
		/* write the name of the image file */
		if (texture->image_file_name)
		{
			display_message(INFORMATION_MESSAGE, "  image file name : %s\n",
				texture->image_file_name);
		}
		/* write the file_number_series if more than one texel deep */
		if (1 < texture->depth_texels)
		{
			display_message(INFORMATION_MESSAGE,
				"  file number pattern : %s\n", texture->file_number_pattern);
			display_message(INFORMATION_MESSAGE,
				"  file number series : start = %d, stop = %d, increment = %d\n",
				texture->start_file_number,
				texture->stop_file_number,
				texture->file_number_increment);
		}
		/* write the original image size */
		display_message(INFORMATION_MESSAGE,
			"  original width (texels) = %d, original height (texels) = %d, "
			"original depth (texels) = %d\n", texture->original_width_texels,
			texture->original_height_texels, texture->original_depth_texels);
		/* write the image components */
		switch (texture->storage)
		{
			case TEXTURE_LUMINANCE:
			{
				display_message(INFORMATION_MESSAGE,"  components : intensity\n");
			} break;
			case TEXTURE_LUMINANCE_ALPHA:
			{
				display_message(INFORMATION_MESSAGE,
					"  components : intensity, alpha\n");
			} break;
			case TEXTURE_RGB:
			{
				display_message(INFORMATION_MESSAGE,
					"  components : red, green, blue\n");
			} break;
			case TEXTURE_RGBA:
			{
				display_message(INFORMATION_MESSAGE,
					"  components : red, green, blue, alpha\n");
			} break;
			case TEXTURE_ABGR:
			{
				display_message(INFORMATION_MESSAGE,
					"  components : alpha, blue, green, red\n");
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"list_Texture.  Undefined or unknown storage type");
				return_code=0;
			} break;
		}
		display_message(INFORMATION_MESSAGE,
			"  bytes_per_component : %d\n", texture->number_of_bytes_per_component);
		/* write the image size */
		display_message(INFORMATION_MESSAGE,
			"  width (texels) = %d, height (texels) = %d, depth (texels) = %d\n",
			texture->width_texels, texture->height_texels, texture->depth_texels);
		/* write the type of wrapping */
		display_message(INFORMATION_MESSAGE,
			"  wrap : %s\n",ENUMERATOR_STRING(Texture_wrap_mode)(texture->wrap_mode));
		/* write the type of magnification/minification filter */
		display_message(INFORMATION_MESSAGE,
			"  magnification/minification filter : %s\n",
			ENUMERATOR_STRING(Texture_filter_mode)(texture->filter_mode));
		/* write the type of resize filter */
		display_message(INFORMATION_MESSAGE, "  resize filter : %s\n",
			ENUMERATOR_STRING(Texture_resize_filter_mode)(
				texture->resize_filter_mode));
		/* write the combine type */
		display_message(INFORMATION_MESSAGE,"  combine type : %s\n",
			ENUMERATOR_STRING(Texture_combine_mode)(texture->combine_mode));
		/* write the compression type */
		display_message(INFORMATION_MESSAGE,"  compression type : %s\n",
			ENUMERATOR_STRING(Texture_compression_mode)(texture->compression_mode));

		display_message(INFORMATION_MESSAGE,"  storage used in graphics : %d\n",
			 cmzn_texture_get_graphics_storage_size(texture));
		/* write the rendered image size */
		display_message(INFORMATION_MESSAGE,
			"  rendered width (texels) = %d, rendered height (texels) = %d, "
			"rendered depth (texels) = %d\n", texture->rendered_width_texels,
			texture->rendered_height_texels, texture->rendered_depth_texels);

		switch (texture->storage)
		{
		case TEXTURE_LUMINANCE:
		{
			int red_size = cmzn_texture_get_graphics_parameter(texture,
				CMZN_TEXTURE_LUMINANCE_SIZE);
			display_message(INFORMATION_MESSAGE,"  pixel_storage_size %d\n",
				red_size);
		} break;
		default:
		{
		} break;
		}

		/* write the colour */
		display_message(INFORMATION_MESSAGE,
			"  colour : red = %.3g, green = %.3g, blue = %.3g\n",
			(texture->combine_colour).red,(texture->combine_colour).green,
			(texture->combine_colour).blue);
		display_message(INFORMATION_MESSAGE,
			"  alpha = %.3g\n",texture->combine_alpha);

		if (texture->property_list)
		{
			/* write the texture properties */
			display_message(INFORMATION_MESSAGE,
				"  property list :\n");
			FOR_EACH_OBJECT_IN_LIST(Texture_property)(
				list_Texture_property, dummy, texture->property_list);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"list_Texture.  Invalid argument");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* list_Texture */

int list_Texture_commands(struct Texture *texture,void *command_prefix_void)
/*******************************************************************************
LAST MODIFIED : 11 March 2002

DESCRIPTION :
Writes on the command window the command needed to recreate the <texture>.
The command is started with the string pointed to by <command_prefix>.
==============================================================================*/
{
	char *command_prefix,*name;
	int return_code;

	ENTER(list_Texture_commands);
	if (texture&&(command_prefix=(char *)command_prefix_void))
	{
		display_message(INFORMATION_MESSAGE,command_prefix);
		name=duplicate_string(texture->name);
		if (name)
		{
			/* put quotes around name if it contains special characters */
			make_valid_token(&name);
			display_message(INFORMATION_MESSAGE,name);
			DEALLOCATE(name);
		}
		/* write the name of the image file, if any */
		if (texture->image_file_name)
		{
			display_message(INFORMATION_MESSAGE," image %s",texture->image_file_name);
		}
		/* file number_series if more than one texel deep */
		if (1 < texture->depth_texels)
		{
			display_message(INFORMATION_MESSAGE,
				" number_pattern %s number_series %d %d %d",
				texture->file_number_pattern,
				texture->start_file_number,
				texture->stop_file_number,
				texture->file_number_increment);
		}
		/* write the texture size in model units */
		display_message(INFORMATION_MESSAGE," width %g height %g depth %g",
			texture->width, texture->height, texture->depth);
		/* write the radial distortion parameters in physical space */
		display_message(INFORMATION_MESSAGE," distortion %g %g %g",
			texture->distortion_centre_x,texture->distortion_centre_y,
			texture->distortion_factor_k1);
		/* write the colour, alpha */
		display_message(INFORMATION_MESSAGE," colour %g %g %g",
			(texture->combine_colour).red,(texture->combine_colour).green,
			(texture->combine_colour).blue);
		display_message(INFORMATION_MESSAGE," alpha %g",texture->combine_alpha);
		/* write the combine type */
		display_message(INFORMATION_MESSAGE," %s",
			ENUMERATOR_STRING(Texture_combine_mode)(texture->combine_mode));
		/* write the compression type */
		display_message(INFORMATION_MESSAGE," %s",
			ENUMERATOR_STRING(Texture_compression_mode)(texture->compression_mode));
		/* write the type of magnification/minification filter */
		display_message(INFORMATION_MESSAGE," %s",
			ENUMERATOR_STRING(Texture_filter_mode)(texture->filter_mode));
		/* write the type of resize filter */
		display_message(INFORMATION_MESSAGE," %s",
			ENUMERATOR_STRING(Texture_resize_filter_mode)(
				texture->resize_filter_mode));
		/* write the type of wrapping */
		display_message(INFORMATION_MESSAGE," %s",
			ENUMERATOR_STRING(Texture_wrap_mode)(texture->wrap_mode));
		display_message(INFORMATION_MESSAGE,";\n");
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Texture_commands.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* list_Texture_commands */

#if defined (OPENGL_API)
int Texture_compile_opengl_texture_object(struct Texture *texture,
	Render_graphics_opengl *renderer)
{
	int return_code = 0;
	GLenum texture_target;

	ENTER(Texture_compile_opengl_texture_object);
	if (texture)
	{
		if (((texture->display_list_current == TEXTURE_COMPILE_STATE_TEXTURE_OBJECT_COMPILED)
			||(texture->display_list_current == TEXTURE_COMPILE_STATE_DISPLAY_LIST_COMPILED))
			&& texture->texture_id)
		{
			return_code = 1;
			if (renderer->allow_texture_tiling && texture->texture_tiling)
			{
				renderer->texture_tiling = texture->texture_tiling;
			}
		}
		else
		{
			texture_target = Texture_get_target_enum(texture);
			if(texture->display_list_current == TEXTURE_COMPILE_STATE_TEXTURE_OBJECT_UPDATE_REQUIRED)
			{
				glBindTexture(texture_target, texture->texture_id);
				direct_render_Texture(texture, renderer);
				/* The display list does not need to be compiled too. */
				texture->display_list_current= TEXTURE_COMPILE_STATE_DISPLAY_LIST_COMPILED;
			}
			else
			{
				if (texture->texture_id)
				{
					glDeleteTextures(1, &(texture->texture_id));
					texture->texture_id = 0;
				}
				if (!texture->texture_id)
				{
					glGenTextures(1, &(texture->texture_id));
				}
				glBindTexture(texture_target, texture->texture_id);
				direct_render_Texture_environment(texture);
				direct_render_Texture(texture, renderer);
				texture->display_list_current= TEXTURE_COMPILE_STATE_TEXTURE_OBJECT_COMPILED;
			}

			return_code=1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_compile_opengl_texture_object.  Missing texture");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Texture_compile_opengl_texture_object */
#endif /* defined (OPENGL_API) */

#if defined (OPENGL_API)
int Texture_execute_opengl_texture_object(struct Texture *texture,
	Render_graphics_opengl *renderer)
{
	GLenum texture_target;
	int return_code = 0;

	ENTER(Texture_execute_opengl_texture_object);
	USE_PARAMETER(renderer);
	/* Passed graphics_buffer through here as was using it to
		do special compilation for Intel graphics driver, but found
		a more complete work around in the graphics_buffer creation.
		This parameter is still better than the NULL from before. */
	if (texture)
	{
		texture_target = Texture_get_target_enum(texture);
		if (texture->texture_tiling)
		{
			/* We only activate the environment here, the individual
			 * textures will be bound as we activate each driver */
			direct_render_Texture_environment(texture);
			if (renderer->allow_texture_tiling)
			{
				renderer->texture_tiling = texture->texture_tiling;
			}
		}
		else
		{
			glBindTexture(texture_target, texture->texture_id);
			/* As we have bound the texture we only need the
				environment in the display list */
			direct_render_Texture_environment(texture);
		}
	}
	else
	{
#if defined (OPENGL_API)
		glDisable(GL_TEXTURE_1D);
		glDisable(GL_TEXTURE_2D);
#if defined (GL_VERSION_1_2) || defined (GL_EXT_texture3D)
		if (
#  if defined (GL_VERSION_1_2)
			Graphics_library_check_extension(GL_VERSION_1_2)
#    if defined (GL_EXT_texture3D)
			||
#    endif /* defined (GL_EXT_texture3D) */
#  endif /* defined (GL_VERSION_1_2) */
#  if defined (GL_EXT_texture3D)
			Graphics_library_check_extension(GL_EXT_texture3D)
#  endif /* defined (GL_EXT_texture3D) */
			)
		{
			glDisable(GL_TEXTURE_3D);
		}
#endif /* defined (GL_VERSION_1_2) || defined (GL_EXT_texture3D) */
#endif /* defined (OPENGL_API) */
		return_code=1;
	}
	LEAVE;

	return (return_code);
} /* Texture_execute_opengl_texture_object */
#endif /* defined (OPENGL_API) */

#if defined (OPENGL_API)
int Texture_compile_opengl_display_list(struct Texture *texture,
	Callback_base< Texture * > *execute_function,
	Render_graphics_opengl *renderer)
{
	int return_code;

	ENTER(Texture_compile_opengl_dispay_list);
	if (texture)
	{
		if (texture->display_list_current == TEXTURE_COMPILE_STATE_DISPLAY_LIST_COMPILED)
		{
			return_code = 1;
			if (renderer->allow_texture_tiling && texture->texture_tiling)
			{
				renderer->texture_tiling = texture->texture_tiling;
			}
		}
		else
		{
			if (texture->display_list||(texture->display_list=glGenLists(1)))
			{
				return_code = Texture_compile_opengl_texture_object(texture, renderer);
				glNewList(texture->display_list,GL_COMPILE);
				(*execute_function)(texture);
				glEndList();

				texture->display_list_current=TEXTURE_COMPILE_STATE_DISPLAY_LIST_COMPILED;
				return_code=1;
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Texture_execute_opengl_dispay_list.  Could not generate display list");
				return_code=0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_execute_opengl_dispay_list.  Missing texture");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Texture_execute_opengl_dispay_list */
#endif /* defined (OPENGL_API) */

int Texture_execute_opengl_display_list(struct Texture *texture,
	Render_graphics_opengl *renderer)
{
	int return_code;

	ENTER(Texture_execute_opengl_display_list);
	USE_PARAMETER(renderer);
	return_code=0;
	if (texture)
	{
		if (texture->display_list_current == TEXTURE_COMPILE_STATE_DISPLAY_LIST_COMPILED)
		{
#if defined (OPENGL_API)
			glCallList(texture->display_list);
			return_code=1;
#else /* defined (OPENGL_API) */
			display_message(ERROR_MESSAGE,
				"Texture_execute_opengl_display_list.  Not defined for this API");
			return_code=0;
#endif /* defined (OPENGL_API) */
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Texture_execute_opengl_display_list.  Display list not current");
			return_code=0;
		}
	}
	else
	{
#if defined (OPENGL_API)
		glDisable(GL_TEXTURE_1D);
		glDisable(GL_TEXTURE_2D);
#if defined (GL_VERSION_1_2) || defined (GL_EXT_texture3D)
		if (
#  if defined (GL_VERSION_1_2)
			Graphics_library_check_extension(GL_VERSION_1_2)
#    if defined (GL_EXT_texture3D)
			||
#    endif /* defined (GL_EXT_texture3D) */
#  endif /* defined (GL_VERSION_1_2) */
#  if defined (GL_EXT_texture3D)
			Graphics_library_check_extension(GL_EXT_texture3D)
#  endif /* defined (GL_EXT_texture3D) */
			)
		{
			glDisable(GL_TEXTURE_3D);
		}
#endif /* defined (GL_VERSION_1_2) || defined (GL_EXT_texture3D) */
#endif /* defined (OPENGL_API) */
		return_code=1;
	}
	LEAVE;

	return (return_code);
} /* Texture_execute_opengl_display_list */

int Texture_execute_vertex_program_environment(struct Texture *texture, unsigned int program)
/*******************************************************************************
LAST MODIFIED : 14 September 2005

DESCRIPTION :
Sets the texture coordinate scaling into the vertex program environment for use
by vertex programs.
==============================================================================*/
{
	int return_code;
#if (defined GL_ARB_vertex_program && defined GL_ARB_fragment_program) || defined GL_VERSION_2_0
	GLfloat texture_scaling[4];
	GLfloat texel_size[4];
#endif /* defined GL_ARB_vertex_program && defined GL_ARB_fragment_program */
#if defined (GL_VERSION_2_0)
	GLint loc1 = -1;
#endif

	ENTER(Texture_execute_vertex_program_environment);
	return_code=0;
	if (texture)
	{
#if defined GL_ARB_vertex_program && defined GL_ARB_fragment_program || defined (GL_VERSION_2_0)
		if (Graphics_library_check_extension(GL_shading_language) ||
			(Graphics_library_check_extension(GL_ARB_vertex_program) &&
				Graphics_library_check_extension(GL_ARB_fragment_program)))
		{
			if (texture->original_width_texels > 1)
				texture_scaling[0] = texture->original_width_texels/
					(texture->width_texels*texture->width);
			else
				texture_scaling[0] = 1.0;

			if (texture->original_height_texels > 1)
				texture_scaling[1] = texture->original_height_texels/
					(texture->height_texels*texture->height);
			else
				texture_scaling[1] = 1.0;

			if (texture->original_depth_texels > 1)
				texture_scaling[2] = texture->original_depth_texels/
					(texture->depth_texels*texture->depth);
			else
				texture_scaling[2] = 1.0;

			texture_scaling[3] = 1.0;

			if (Graphics_library_check_extension(GL_shading_language) && glIsProgram((GLuint)program))
			{
				GLint flag;
				glGetProgramiv(program,GL_LINK_STATUS, &flag);
				if (flag == GL_TRUE)
				{
					loc1 = glGetUniformLocation((GLuint)program,"texture_scaling");
					if (loc1 > (GLint)-1)
					{
						glUniform4f(loc1, texture_scaling[0], texture_scaling[1],
							texture_scaling[2], texture_scaling[3]);
					}
				}
			}
			else
			{
				 glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, 0,
						texture_scaling);
			}
			if (texture->rendered_width_texels)
			{
				texel_size[0] = 1.0 / texture->rendered_width_texels;
			}
			else
			{
				texel_size[0] = 0.0;
			}
			if (texture->rendered_height_texels)
			{
				texel_size[1] = 1.0 / texture->rendered_height_texels;
			}
			else
			{
				texel_size[1] = 0.0;
			}
			if (texture->rendered_depth_texels)
			{
				texel_size[2] = 1.0 / texture->rendered_depth_texels;
			}
			else
			{
				texel_size[2] = 0.0;
			}
			texel_size[3] = 0.0;
			if (Graphics_library_check_extension(GL_shading_language) && glIsProgram((GLuint)program))
			{
				loc1 = glGetUniformLocation((GLuint)program,"texturesize");
				if (loc1 > (GLint)-1)
					glUniform4f(loc1, texel_size[0], texel_size[1],
						texel_size[2], texel_size[3]);
			}
			else
			{
				 glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 0,
						texel_size);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE, "Texture_execute_vertex_program_environment.  "
				"GL_ARB_vertex_program or GL_ARB_fragment_program extension unavailable.");
			return_code=0;
		}
#else /* defined GL_ARB_vertex_program && defined GL_ARB_fragment_program */
		USE_PARAMETER(program);
#endif
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_execute_vertex_program_environment.  Missing texture.");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Texture_execute_vertex_program_environment */

int Texture_set_property(struct Texture *texture,
	const char *property, const char *value)
/*******************************************************************************
LAST MODIFIED : 25 October 2007

DESCRIPTION :
Sets the <property> and <value> string for the <texture>.
==============================================================================*/
{
	int return_code;
	struct Texture_property *texture_property;

	ENTER(Texture_set_property);
	if (texture && property && value)
	{
		if (!texture->property_list)
		{
			texture->property_list = CREATE(LIST(Texture_property))();
		}
		texture_property = FIND_BY_IDENTIFIER_IN_LIST(Texture_property,name)(
			(char *)property, texture->property_list);
		if (texture_property)
		{
			DEALLOCATE(texture_property->value);
			texture_property->value = duplicate_string(value);
			return_code = 1;
		}
		else
		{
			texture_property = CREATE(Texture_property)
				(property, value);
			return_code = ADD_OBJECT_TO_LIST(Texture_property)(
				texture_property, texture->property_list);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_set_property.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Texture_set_property */

char *Texture_get_property(struct Texture *texture,
	const char *property)
/*******************************************************************************
LAST MODIFIED : 25 October 2007

DESCRIPTION :
If the <property> is defined for the <texture>, then a copy is returned (and
should be DEALLOCATED when finished with).
==============================================================================*/
{
	char *return_value;
	struct Texture_property *texture_property;

	ENTER(Texture_get_property);
	if (texture && property)
	{
		if (texture->property_list &&
			(texture_property = FIND_BY_IDENTIFIER_IN_LIST(Texture_property,name)(
			(char *)property, texture->property_list)))
		{
			return_value = duplicate_string(texture_property->value);
		}
		else
		{
			return_value = (char *)NULL;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_get_property.  Invalid argument(s)");
		return_value = (char *)NULL;
	}
	LEAVE;

	return (return_value);
} /* Texture_get_property */

int Texture_clear_all_properties(struct Texture *texture)
{
	int return_code;

	ENTER(Texture_clear_all_properties);
	if (texture)
	{
		if (texture->property_list)
		{
			return_code = REMOVE_ALL_OBJECTS_FROM_LIST(Texture_property)(texture->property_list);
		}
		else
		{
			return_code = 1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_get_property.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Texture_clear_all_properties */

#if defined (OPENGL_API)
unsigned int Texture_create_float_texture(int width, int height, char* buffer,
	int alpha, int fallback_to_shorts)
{
	GLuint type = GL_TEXTURE_2D;
	GLuint texture_object_id;

	GLuint format, iformat;
	format = 0;
	if (alpha)
	{
		iformat = GL_RGBA;
	}
	else
	{
		iformat = GL_RGB;
	}

#if defined (GL_ARB_texture_float)
	if (Graphics_library_check_extension(GL_ARB_texture_float))
	{
		if (alpha)
		{
			format = GL_RGB32F_ARB;
		}
		else
		{
			format = GL_RGBA32F_ARB;
		}
	}
	else
#endif /* defined (GL_ARB_texture_float) */
#if defined (GL_ATI_texture_float)
	if (Graphics_library_check_extension(GL_ATI_texture_float))
	{
		if (alpha)
		{
			format = GL_RGBA_FLOAT32_ATI;
		}
		else
		{
			format = GL_RGB_FLOAT32_ATI;
		}
	}
	else
#endif /* defined (GL_ATI_texture_float) */
#if defined (GL_NV_float_buffer)
	if (Graphics_library_check_extension(GL_NV_float_buffer))
	{
		if (alpha)
		{
			format = GL_FLOAT_RGBA32_NV;
		}
		else
		{
			format = GL_FLOAT_RGB32_NV;
		}
	}
	else
#endif /* defined (GL_ATI_texture_float) */
	if (fallback_to_shorts)
	{
		/* Fall back to shorts if no GLfloat format available */
		if (alpha)
		{
			format = GL_RGBA16;
		}
		else
		{
			format = GL_RGB16;
		}
		display_message(WARNING_MESSAGE, "Texture_create_float_texture.  "
			"Float texture formats unavailable with this OpenGL implementation, using integer texture as a fallback.");
	}
   if (format)
   {
	glGenTextures (1, &texture_object_id);
	glBindTexture(type,texture_object_id);

	// set texture parameters
	glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(type,0,format,width,height,0,iformat,GL_FLOAT,buffer);

	glBindTexture(type,0);
	}
   else
   {
		texture_object_id = 0;
   }
	return texture_object_id;
}
#endif /* defined (OPENGL_API) */

int Texture_get_texture_tiling_enabled(struct Texture *texture)
{
	int return_value;

	ENTER(Texture_get_texture_tiling_enabled);
	if (texture)
	{
		return_value = texture->allow_texture_tiling;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_get_wrap_mode.  Invalid argument(s)");
		return_value = 0;
	}
	LEAVE;

	return (return_value);
} /* Texture_get_texture_tiling_enabled */

int Texture_set_texture_tiling_enabled(struct Texture *texture, int enable_texture_tiling)
{
	int return_code;

	ENTER(Texture_set_texture_tiling_enabled);
	if (texture)
	{
	  if (enable_texture_tiling != texture->allow_texture_tiling)
		{
			texture->allow_texture_tiling = enable_texture_tiling;
				/* display list needs to be compiled again */
			texture->display_list_current=TEXTURE_COMPILE_STATE_NOT_COMPILED;
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_set_texture_tiling_enabled.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Texture_set_texture_tiling_enabled */

#if defined (OPENGL_API)
int Texture_tiling_activate_tile(struct Texture_tiling *texture_tiling,
	int tile_index)
{
	int return_code;

	ENTER(Texture_tiling_activate_tile);
	if (texture_tiling && texture_tiling->texture_target &&
		(tile_index >= 0) && (tile_index < texture_tiling->total_tiles))
	{
		glBindTexture(texture_tiling->texture_target,
			texture_tiling->texture_ids[tile_index]);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Texture_tiling_activate_tile.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
}
#endif // defined (OPENGL_API)
