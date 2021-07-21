#if !defined (SHADER_PROGRAM_H)
#define SHADER_PROGRAM_H

#include "opencmiss/zinc/zincconfigure.h"
#include "opencmiss/zinc/shader.h"
#include "general/list.h"
#include "general/manager.h"
#include "general/object.h"
#include "graphics/auxiliary_graphics_types.h"


class cmzn_shaderprogram_change_detail
{
	bool programChanged;

public:

	cmzn_shaderprogram_change_detail() :
		programChanged(false)
	{ }

	void clear()
	{
		programChanged = false;
	}

	bool isProgramChanged() const
	{
		return programChanged;
	}

	void setProgramChanged()
	{
		programChanged = true;
	}

};

class Render_graphics_opengl;
struct Texture;

enum cmzn_shaderprogram_type
/*****************************************************************************//**
@date LAST MODIFIED : 4 July 2007

Enumerates the main different types of vertex/fragment program for materials
==============================================================================*/
{
	/* This type is for cmzn_shaderprograms which have arbitrary specified strings
	 * rather than the program being generated based on this type value.
	 */
	SHADER_PROGRAM_SPECIFIED_STRINGS = 0,
	/* This first one is a standard Gouraud Shaded material, included here so
		that it can be peeled in order independent transparency */
	SHADER_PROGRAM_GOURAUD_SHADING = 1,
	SHADER_PROGRAM_PER_PIXEL_LIGHTING = 2,
	SHADER_PROGRAM_PER_PIXEL_TEXTURING = 10,
	SHADER_PROGRAM_BUMP_MAPPING = 770,
	SHADER_PROGRAM_BUMP_MAPPING_TEXTURING = 778,

	/* These classes modify the above programs and so must be bit independent */
	SHADER_PROGRAM_CLASS_GOURAUD_SHADING = 1,
	SHADER_PROGRAM_CLASS_PER_PIXEL_LIGHTING = 2,
	/* Use these bits to indicate the presence of and dimension (1, 2 or 3) of a colour texture. */
	SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_1 = (1<<2),
	SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_2 = (1<<3),
	/* If either bit is set then we are using a texture */
	SHADER_PROGRAM_CLASS_COLOUR_TEXTURE = 12,
	/* If this bit is set then a colour texture will replace the lighting calculation for a colour,
		if it is not set then the texture will modulate the lighting calculation colour */
	SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_DECAL = (1<<4),
	/* Specifies the output dimension of the texture and therefore how it is applied.
		OUTPUT1 = grayscale, OUTPUT2 = grayscale and alpha, OUTPUT1 & OUTPUT2 = rgb
		!OUPUT1 & !OUTPUT2 = rgba. */
	SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_1 = (1<<5),
	SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_2 = (1<<6),

	/* Use these bits to indicate the presence of and dimension (1, 2 or 3) of a second or bump map texture. */
	SHADER_PROGRAM_CLASS_SECOND_TEXTURE_1 = (1<<7),
	SHADER_PROGRAM_CLASS_SECOND_TEXTURE_2 = (1<<8),
	SHADER_PROGRAM_CLASS_SECOND_TEXTURE = 384,
   /* Specifies that the second texture is intended to be used as a bump map texture, modulating
		the per pixel value of the normal in the lighting calculation */
	SHADER_PROGRAM_CLASS_SECOND_TEXTURE_BUMPMAP = (1<<9),

	/* The colour value is used as the input, derived from the primary texture or the lighting.
	   Specify which input components are used directly from the input source,
		at most 3 of the input values can be used (a 3D texture lookup) */
	SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_1 = (1<<10),
	SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_2 = (1<<11),
	SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_3 = (1<<12),
	SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_4 = (1<<13),
	SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_INPUTS = (1<<10) + (1<<11) + (1<<12) + (1<<13),

	/* Specify the outputs in the dependent texture lookup, either replacing the colour, alpha or both. */
	/* These modes work by assuming that the dependent_texture inputs form
		the axes of a single 1D, 2D or 3D texture */
	SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_COLOUR = (1<<14),
	SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_ALPHA = (1<<15),
	/* This mode works by looking up each input component independently
		in a common 1D texture */
	SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_1D_COMPONENT_LOOKUP = (1<<16),

	/* Assume that the texture contains an intensity followed by a 3 component
		normal vector.  This vector is used to light volume rendering by
		performing per pixel lighting using this normal. */
	SHADER_PROGRAM_CLASS_LIT_VOLUME_INTENSITY_NORMAL_TEXTURE = (1<<17),
	/* Calculate a normal by using a finite difference operator. */
	SHADER_PROGRAM_CLASS_LIT_VOLUME_FINITE_DIFFERENCE_NORMAL = (1<<18),
	/* Scale the alpha by the magnitude of the normal */
	SHADER_PROGRAM_CLASS_LIT_VOLUME_SCALE_ALPHA = (1<<19),

   /* Order independent transparency passes */
	SHADER_PROGRAM_CLASS_ORDER_INDEPENDENT_FIRST_LAYER = (1<<20),
	SHADER_PROGRAM_CLASS_ORDER_INDEPENDENT_PEEL_LAYER = (1<<21)
}; /* enum cmzn_shaderprogram_type */

enum cmzn_shaderprogram_shader_type
{
	SHADER_PROGRAM_SHADER_NONE = 0,
	SHADER_PROGRAM_SHADER_ARB = 1,
	SHADER_PROGRAM_SHADER_GLSL = 2
};


/**
 * Stores a display list which sets up the correct state for a particular
 * material state.  This allows vertex/fragment programs to be used per material
 * but shared between different materials with the same state.
 */
struct cmzn_shaderprogram;

DECLARE_LIST_TYPES(cmzn_shaderprogram);
DECLARE_MANAGER_TYPES(cmzn_shaderprogram);

PROTOTYPE_OBJECT_FUNCTIONS(cmzn_shaderprogram);
PROTOTYPE_GET_OBJECT_NAME_FUNCTION(cmzn_shaderprogram);

PROTOTYPE_LIST_FUNCTIONS(cmzn_shaderprogram);
PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(cmzn_shaderprogram,type,cmzn_shaderprogram_type);
PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(cmzn_shaderuniforms,name,const char *);

PROTOTYPE_MANAGER_FUNCTIONS(cmzn_shaderprogram);
PROTOTYPE_MANAGER_IDENTIFIER_WITHOUT_MODIFY_FUNCTIONS(cmzn_shaderprogram,name,const char *);

cmzn_shaderprogram_id cmzn_shaderprogram_create_private();

/***************************************************************************//**
 * Private; only to be called from graphics_module.
 */
int cmzn_shaderprogram_manager_set_owner_private(struct MANAGER(cmzn_shaderprogram) *manager,
	struct cmzn_shadernmodule *shadermodule);


/**
 * Same as MANAGER_MESSAGE_GET_OBJECT_CHANGE(cmzn_shaderprogram) but also returns
 * change_detail for shaderprogram, if any.
 *
 * @param message  The shaderprogram manager change message.
 * @param shaderprogram  The shaderprogram to query about.
 * @param change_detail_address  Address to put const change detail in.
 * @return  manager change flags for the object.
 */
int cmzn_shaderprogram_manager_message_get_object_change_and_detail(
	struct MANAGER_MESSAGE(cmzn_shaderprogram) *message, cmzn_shaderprogram *shaderprogram,
	const cmzn_shaderprogram_change_detail **change_detail_address);

#if defined (OPENGL_API)
int cmzn_shaderprogram_compile(cmzn_shaderprogram_id shader_program,
	Render_graphics_opengl *renderer);

int cmzn_shaderprogram_execute(cmzn_shaderprogram_id shader_program,
	Render_graphics_opengl *renderer);

int cmzn_shaderprogram_execute_textures(cmzn_shaderprogram_id shader_program,
	struct Texture *texture, struct Texture *second_texture,
	struct Texture *third_texture);

int cmzn_shaderprogram_execute_uniforms(cmzn_shaderprogram_id program,
	cmzn_shaderuniforms_id uniforms);
#endif

unsigned int cmzn_shaderprogram_get_glslprogram(cmzn_shaderprogram_id program);

enum cmzn_shaderprogram_shader_type cmzn_shaderprogram_get_shader_type(cmzn_shaderprogram_id program);

enum cmzn_shaderprogram_type cmzn_shaderprogram_get_type(cmzn_shaderprogram_id program);

int cmzn_shaderprogram_set_type(cmzn_shaderprogram_id shaderprogram,
	enum cmzn_shaderprogram_type type);

int cmzn_shader_program_destroy(cmzn_shaderprogram_id *shader_program_address);

cmzn_shaderprogram_id cmzn_shader_program_access(cmzn_shaderprogram_id shader_program);

int cmzn_shaderprogram_set_geometry_shader(cmzn_shaderprogram_id shader_program_to_be_modified,
	const char *geometry_shader_string);

char *cmzn_shaderprogram_get_geometry_shader(cmzn_shaderprogram_id program);


#endif
