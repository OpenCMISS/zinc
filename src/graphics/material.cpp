/*******************************************************************************
FILE : material.c

LAST MODIFIED : 5 July 2007

DESCRIPTION :
The functions for manipulating graphical materials.

???RC Only OpenGL is supported now.

???RC 28 Nov 97.
		Two functions are now used for activating the graphical material:
- compile_Graphical_material puts the material into its own display list.
- execute_Graphical_material calls that display list.
		The separation into two functions was needed because OpenGL cannot start a
new list while one is being written to. As a consequence, you must precompile
all objects that are executed from within another display list. To make this job
easier, compile_Graphical_material is a list/manager iterator function.
		Future updates of OpenGL may overcome this limitation, in which case the
execute function can take over compiling as well. Furthermore, it is easy to
return to direct rendering, as described with these routines.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "opencmiss/zinc/status.h"
#include "opencmiss/zinc/zincconfigure.h"
#include "opencmiss/zinc/fieldmodule.h"
#include "opencmiss/zinc/material.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_image.h"
#include "computed_field/computed_field_private.hpp"
#include "general/compare.h"
#include "general/debug.h"
#include "general/indexed_list_private.h"
#include "general/indexed_list_stl_private.hpp"
#include "general/io_stream.h"
#include "general/manager_private.h"
#include "general/mystring.h"
#include "general/object.h"
#include "graphics/graphics_library.h"
#include "graphics/material.h"
#include "graphics/spectrum.h"
#include "graphics/texture.h"
#include "three_d_drawing/abstract_graphics_buffer.h"
#include "general/message.h"
#include "general/enumerator_conversion.hpp"
#include "graphics/render_gl.h"
#include "graphics/material.hpp"
#include "graphics/spectrum.hpp"
#include "graphics/shader.hpp"
#include "graphics/shader_program.hpp"
#include "graphics/shader_uniforms.hpp"

struct Startup_material_definition
{
	const char *name;
	double ambient[3];
	double diffuse[3];
	double emission[3];
	double specular[3];
	double alpha;
	double shininess;
};

/* Only to be used from FIND_BY_IDENTIFIER_IN_INDEXED_LIST_STL function
 * Creates a pseudo object with name identifier suitable for finding
 * objects by identifier with cmzn_set.
 */
class cmzn_material_identifier : private cmzn_material
{
public:
	cmzn_material_identifier(const char *nameIn)
	{
		cmzn_material::name = nameIn;
	}

	~cmzn_material_identifier()
	{
		cmzn_material::name = 0;
	}

	cmzn_material *getPseudoObject()
	{
		return this;
	}
};

/** functor for ordering cmzn_set<Computed_field> by field name */
struct cmzn_material_compare_name_functor
{
	bool operator() (const cmzn_material* material1, const cmzn_material* material2) const
	{
		return strcmp(material1->name, material2->name) < 0;
	}
};

typedef cmzn_set<cmzn_material *,cmzn_material_compare_name_functor> cmzn_set_cmzn_material;

FULL_DECLARE_MANAGER_TYPE_WITH_OWNER(cmzn_material, cmzn_materialmodule, void *);

struct cmzn_materialiterator : public cmzn_set_cmzn_material::ext_iterator
{
private:
	cmzn_materialiterator(cmzn_set_cmzn_material *container);
	cmzn_materialiterator(const cmzn_materialiterator&);
	~cmzn_materialiterator();

public:

	static cmzn_materialiterator *create(cmzn_set_cmzn_material *container)
	{
		return static_cast<cmzn_materialiterator *>(cmzn_set_cmzn_material::ext_iterator::create(container));
	}

	cmzn_materialiterator *access()
	{
		return static_cast<cmzn_materialiterator *>(this->cmzn_set_cmzn_material::ext_iterator::access());
	}

	static int deaccess(cmzn_materialiterator* &iterator)
	{
		cmzn_set_cmzn_material::ext_iterator* baseIterator = static_cast<cmzn_set_cmzn_material::ext_iterator*>(iterator);
		iterator = 0;
		return cmzn_set_cmzn_material::ext_iterator::deaccess(baseIterator);
	}

};

/*
Module functions
----------------
*/

int cmzn_material::deaccess(cmzn_material **materialAddress)
{
	int return_code;
	struct cmzn_material *material;
	if (materialAddress && (material = *materialAddress))
	{
		--(material->access_count);
		if (material->access_count <= 0)
		{
			DESTROY(cmzn_material)(&material);
			return_code = 1;
		}
		else if ((!material->isManagedFlag) && (material->manager) &&
			((1 == material->access_count) || ((2 == material->access_count) &&
			(MANAGER_CHANGE_NONE(cmzn_material) != material->manager_change_status))))
		{
			return_code = REMOVE_OBJECT_FROM_MANAGER(cmzn_material)(material, material->manager);
		}
		else
		{
			return_code = 1;
		}
		*materialAddress = 0;
	}
	else
	{
		return_code = 0;
	}
	return (return_code);
}

int cmzn_material::setName(const char *newName)
{
	if (!newName)
		return CMZN_ERROR_ARGUMENT;
	if (this->name && (0 == strcmp(this->name, newName)))
		return CMZN_OK;
	cmzn_set_cmzn_material *allMaterials = 0;
	bool restoreObjectToLists = false;
	if (this->manager)
	{
		allMaterials = reinterpret_cast<cmzn_set_cmzn_material *>(this->manager->object_list);
		cmzn_material *existingMaterial = FIND_BY_IDENTIFIER_IN_MANAGER(cmzn_material, name)(newName, this->manager);
		if (existingMaterial)
		{
			display_message(ERROR_MESSAGE, "cmzn_material::setName.  material named '%s' already exists.", newName);
			return CMZN_ERROR_ARGUMENT;
		}
		else
		{
			// this temporarily removes the object from all related lists
			restoreObjectToLists = allMaterials->begin_identifier_change(this);
			if (!restoreObjectToLists)
			{
				display_message(ERROR_MESSAGE, "cmzn_material::setName.  "
					"Could not safely change identifier in manager");
				return CMZN_ERROR_GENERAL;
			}
		}
	}
	if (this->name)
		DEALLOCATE(this->name);
	this->name = duplicate_string(newName);
	if (restoreObjectToLists)
		allMaterials->end_identifier_change();
	if (this->manager)
		MANAGED_OBJECT_CHANGE(cmzn_material)(this, MANAGER_CHANGE_IDENTIFIER(cmzn_material));
	return CMZN_OK;
}

PROTOTYPE_ACCESS_OBJECT_FUNCTION(cmzn_material)
{
	return object->access();
}

PROTOTYPE_DEACCESS_OBJECT_FUNCTION(cmzn_material)
{
	return cmzn_material::deaccess(object_address);
}

PROTOTYPE_REACCESS_OBJECT_FUNCTION(cmzn_material)
{
	if (object_address)
	{
		if (new_object)
			new_object->access();
		if (*object_address)
			cmzn_material::deaccess(object_address);
		*object_address = new_object;
		return 1;
	}
	return 0;
}

DECLARE_DEFAULT_GET_OBJECT_NAME_FUNCTION(cmzn_material)

DECLARE_INDEXED_LIST_STL_FUNCTIONS(cmzn_material)
DECLARE_FIND_BY_IDENTIFIER_IN_INDEXED_LIST_STL_FUNCTION(cmzn_material,name,const char *)
DECLARE_INDEXED_LIST_STL_IDENTIFIER_CHANGE_FUNCTIONS(cmzn_material,name)

DECLARE_LOCAL_MANAGER_FUNCTIONS(cmzn_material)

DECLARE_MANAGER_FUNCTIONS(cmzn_material, manager)

DECLARE_DEFAULT_MANAGED_OBJECT_NOT_IN_USE_FUNCTION(cmzn_material, manager)

DECLARE_MANAGER_IDENTIFIER_WITHOUT_MODIFY_FUNCTIONS(cmzn_material, name, const char *, manager)
DECLARE_MANAGER_MODIFY_NOT_IDENTIFIER_FUNCTION(cmzn_material, name)

DECLARE_CREATE_INDEXED_LIST_STL_ITERATOR_FUNCTION(cmzn_material,cmzn_materialiterator)

DECLARE_MANAGER_OWNER_FUNCTIONS(cmzn_material, struct cmzn_materialmodule)

int Material_manager_set_owner(struct MANAGER(cmzn_material) *manager,
	struct cmzn_materialmodule *materialmodule)
{
	return MANAGER_SET_OWNER(cmzn_material)(manager, materialmodule);
}

#if defined (OPENGL_API)
int direct_render_Graphical_material(cmzn_material *material,
	Render_graphics_opengl *renderer)
/*******************************************************************************
LAST MODIFIED : 8 August 2002

DESCRIPTION :
Directly outputs the graphics library commands for activating <material>.
???RC Only supports OpenGL.
SAB Spectrums make extensive use of this function as they operate by copying the
normal material, modifying it according to the settings and then calling this
routine.  I expected this to make spectrums much slower as we are no longer
using the glColorMaterial function, didn't really seem to change.  However on
some other OPENGL implementations this could have a significant effect.  I was
going to try and get this routine to keep record of the current colour and use
the glColorMaterial function if it can however if this is done you must make
sure that when this routine is compiled into different display list the correct
material results.
==============================================================================*/
{
	GLfloat values[4];
#if defined (GL_VERSION_2_0)
	GLint loc1 = -1;
#endif /* defined (GL_VERSION_2_0) */
	int return_code;

	ENTER(direct_render_Graphical_material);
	if (material)
	{
		values[0]=static_cast<GLfloat>((material->diffuse).red);
		values[1]=static_cast<GLfloat>((material->diffuse).green);
		values[2]=static_cast<GLfloat>((material->diffuse).blue);
		values[3]=static_cast<GLfloat>(material->alpha);
		/* use diffuse colour for lines, which are unlit */
		glColor4fv(values);
		glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,values);
		values[0]=(material->ambient).red;
		values[1]=(material->ambient).green;
		values[2]=(material->ambient).blue;
		glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,values);
		values[0]=(material->emission).red;
		values[1]=(material->emission).green;
		values[2]=(material->emission).blue;
		values[3]=1.0;
		glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,values);
		values[0]=(material->specular).red;
		values[1]=(material->specular).green;
		values[2]=(material->specular).blue;
		glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,values);
		glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,(material->shininess)*128.);

		if (material->image_texture.texture)
		{
			renderer->Texture_execute(material->image_texture.texture);
		}

#if defined (GL_VERSION_1_3)
		if (Graphics_library_check_extension(GL_VERSION_1_3))
		{
			/* I used to test for the GL_VERSION_1_3 when setting the texture
				and not here, but at that point the openGL may not have been
				initialised yet, instead check here at display list compile time. */
			if (material->second_image_texture.texture)
			{
				glActiveTexture(GL_TEXTURE1);
				renderer->Texture_execute(material->second_image_texture.texture);
				glActiveTexture(GL_TEXTURE0);
			}
			else if (material->spectrum)
			{
				glActiveTexture(GL_TEXTURE1);
				Spectrum_execute_colour_lookup(material->spectrum, renderer);
				glActiveTexture(GL_TEXTURE0);
			}
			else
			{
				glActiveTexture(GL_TEXTURE1);
				glDisable(GL_TEXTURE_1D);
				glDisable(GL_TEXTURE_2D);
				glDisable(GL_TEXTURE_3D);
				glActiveTexture(GL_TEXTURE0);
			}
			/* The colour_lookup_spectrum is specified as the third texture
				so far, so can't have both a spectrum and an explicit third texture
				at the moment. */
			if (material->third_image_texture.texture)
			{
				glActiveTexture(GL_TEXTURE2);
				renderer->Texture_execute(material->third_image_texture.texture);
				glActiveTexture(GL_TEXTURE0);
			}
			else
			{
				glActiveTexture(GL_TEXTURE2);
				glDisable(GL_TEXTURE_1D);
				glDisable(GL_TEXTURE_2D);
				glDisable(GL_TEXTURE_3D);
				glActiveTexture(GL_TEXTURE0);
			}
			if (material->fourth_image_texture.texture)
			{
				glActiveTexture(GL_TEXTURE3);
				renderer->Texture_execute(material->fourth_image_texture.texture);
				glActiveTexture(GL_TEXTURE0);
			}
			else
			{
				glActiveTexture(GL_TEXTURE3);
				glDisable(GL_TEXTURE_1D);
				glDisable(GL_TEXTURE_2D);
				glDisable(GL_TEXTURE_3D);
				glActiveTexture(GL_TEXTURE0);
			}
		}
#endif /* defined (GL_VERSION_1_3) */

		if (material->program)
		{
			 cmzn_shaderprogram_execute(material->program, renderer);
			 cmzn_shaderprogram_execute_textures(material->program, material->image_texture.texture,
				material->second_image_texture.texture, material->third_image_texture.texture);
			 cmzn_shaderprogram_execute_uniforms(material->program, material->uniforms);
			if (material->image_texture.texture)
			{
#if defined(GL_VERSION_2_0) || (defined GL_ARB_vertex_program && defined GL_ARB_fragment_program)
				 /* Adjust the scaling by the ratio from the original texel
					size to the actually rendered texture size so that
					we are independent of texture reduction. */
				 GLfloat normal_scaling[4];
				 int original_dimension, original_sizes[3];
				 unsigned int rendered_dimension, *rendered_sizes;
				 if ((original_dimension = cmzn_texture_get_pixel_sizes(material->image_texture.texture,
							 3, &original_sizes[0])) &&
						(cmzn_texture_get_rendered_texel_sizes(material->image_texture.texture,
							 &rendered_dimension, &rendered_sizes)))
				 {
						if ((original_dimension > 0) && (rendered_dimension > 0)
							 && (original_sizes[0] > 0))
						{
							 normal_scaling[0] = (GLfloat)rendered_sizes[0] /
									(GLfloat)original_sizes[0] *
									material->lit_volume_normal_scaling[0];
						}
						else
						{
							 normal_scaling[0] = 1.0;
						}
						if ((original_dimension > 1) && (rendered_dimension > 1)
							 && (original_sizes[1] > 0))
						{
							 normal_scaling[1] = (GLfloat)rendered_sizes[1] /
									(GLfloat)original_sizes[1] *
									material->lit_volume_normal_scaling[1];
						}
						else
						{
							 normal_scaling[1] = 1.0;
						}
						if ((original_dimension > 2) && (rendered_dimension > 2)
							 && (original_sizes[2] > 0))
						{
							 normal_scaling[2] = (GLfloat)rendered_sizes[2] /
									(GLfloat)original_sizes[2] *
									material->lit_volume_normal_scaling[2];
						}
						else
						{
							normal_scaling[2] = 1.0;
						}
						normal_scaling[3] = 0.0;
						enum cmzn_shaderprogram_shader_type shader_type =
							cmzn_shaderprogram_get_shader_type(material->program);
						if (shader_type)
						{
							GLuint currentProgram = (GLuint)cmzn_shaderprogram_get_glslprogram(material->program);
							if (glIsProgram(currentProgram))
							{
								loc1 = glGetUniformLocation(currentProgram,"normal_scaling");
								if (loc1 != (GLint)-1)
									glUniform4f(loc1, normal_scaling[0], normal_scaling[1],
										normal_scaling[2], normal_scaling[3]);
							}
						}
						else if (shader_type)
						{
							glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 3,
								normal_scaling);
						}
						DEALLOCATE(rendered_sizes);
				 }
#endif /* defined(GL_VERSION_2_0) || defined GL_ARB_vertex_program && defined GL_ARB_fragment_program */
			}
		}
		else
		{
#if defined GL_ARB_vertex_program && defined GL_ARB_fragment_program
			 if (Graphics_library_check_extension(GL_ARB_vertex_program) &&
				 Graphics_library_check_extension(GL_ARB_fragment_program))
			 {
				 glDisable(GL_VERTEX_PROGRAM_ARB);
				 glDisable(GL_FRAGMENT_PROGRAM_ARB);
				 glDisable(GL_VERTEX_PROGRAM_TWO_SIDE_ARB);
			 }
#endif
#if defined (GL_VERSION_2_0)
			 if (Graphics_library_check_extension(GL_shading_language))
			 {
						 glUseProgram(0);
						 glDisable(GL_VERTEX_PROGRAM_TWO_SIDE);
			 }
#endif
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"direct_render_Graphical_material.  Missing material");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* direct_render_Graphical_material */
#endif /* defined (OPENGL_API) */

static void Graphical_material_Spectrum_change(
	struct MANAGER_MESSAGE(cmzn_spectrum) *message, void *material_void)
{
	cmzn_material *material;

	ENTER(Graphical_material_Spectrum_change);
	if (message && (material = (cmzn_material *)material_void))
	{
		int change = MANAGER_MESSAGE_GET_OBJECT_CHANGE(cmzn_spectrum)(message, material->spectrum);
		if (change & MANAGER_CHANGE_RESULT(cmzn_spectrum))
		{
			material->compile_status = GRAPHICS_NOT_COMPILED;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphical_material_Spectrum_change.  Invalid argument(s)");
	}
	LEAVE;
}

/*
Global functions
----------------
*/

static int Graphical_material_remove_module_if_matching(cmzn_material *material,
	void *materialmodule_void)
{
	int return_code;

	if (material && materialmodule_void)
	{
		if (material->module == (struct cmzn_materialmodule *)materialmodule_void)
		{
			material->module = (struct cmzn_materialmodule *)NULL;
		}
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphical_material_remove_module_if_matching.  Invalid argument(s)");
		return_code = 0;
	}

	return (return_code);
}

struct cmzn_materialmodule
{

private:

	struct MANAGER(cmzn_material) *materialManager;
	cmzn_material *defaultMaterial;
	cmzn_material *defaultSelectedMaterial;
	cmzn_material *defaultSurfaceMaterial;
	struct MANAGER(cmzn_spectrum) *spectrumManager;
	int access_count;

	cmzn_materialmodule() :
		materialManager(CREATE(MANAGER(cmzn_material))()),
		defaultMaterial(0),
		defaultSelectedMaterial(0),
		defaultSurfaceMaterial(0),
		spectrumManager(0),
		access_count(1)
	{
	}

	~cmzn_materialmodule()
	{
		if (this->defaultMaterial)
			cmzn_material_destroy(&this->defaultMaterial);
		if (this->defaultSelectedMaterial)
			cmzn_material_destroy(&this->defaultSelectedMaterial);
		if (this->defaultSurfaceMaterial)
			cmzn_material_destroy(&this->defaultSurfaceMaterial);
		/* Make sure each material no longer points at this module */
		FOR_EACH_OBJECT_IN_MANAGER(cmzn_material)(
			Graphical_material_remove_module_if_matching, (void *)this,
			materialManager);
		DESTROY(MANAGER(cmzn_material))(&materialManager);
	}

public:

	static cmzn_materialmodule *create()
	{
		return new cmzn_materialmodule();
	}

	cmzn_materialmodule *access()
	{
		++access_count;
		return this;
	}

	static int deaccess(cmzn_materialmodule* &materialmodule)
	{
		if (materialmodule)
		{
			--(materialmodule->access_count);
			if (materialmodule->access_count <= 0)
			{
				delete materialmodule;
			}
			materialmodule = 0;
			return CMZN_OK;
		}
		return CMZN_ERROR_ARGUMENT;
	}


	void setSpectrumManager(struct MANAGER(cmzn_spectrum) *spectrum_manager)
	{
		spectrumManager = spectrum_manager;
	}

	struct MANAGER(cmzn_spectrum) *getSpectrumManager()
	{
		return spectrumManager;
	}

	struct MANAGER(cmzn_material) *getManager()
	{
		return materialManager;
	}

	int beginChange()
	{
		return MANAGER_BEGIN_CACHE(cmzn_material)(this->materialManager);
	}

	int endChange()
	{
		return MANAGER_END_CACHE(cmzn_material)(this->materialManager);
	}

	cmzn_material_id createMaterial()
	{
		cmzn_material_id material = NULL;
		char temp_name[20];
		int i = NUMBER_IN_MANAGER(cmzn_material)(this->materialManager);
		do
		{
			i++;
			sprintf(temp_name, "temp%d",i);
		}
		while (FIND_BY_IDENTIFIER_IN_MANAGER(cmzn_material,name)(temp_name,
			this->materialManager));
		material = cmzn_material_create_private();
		cmzn_material_set_name(material, temp_name);
		if (!ADD_OBJECT_TO_MANAGER(cmzn_material)(material, this->materialManager))
		{
			cmzn_material_destroy(&material);
		}
		material->module = this;
		return material;
	}

	cmzn_materialiterator *createMaterialiterator()
	{
		return CREATE_LIST_ITERATOR(cmzn_material)(this->materialManager->object_list);
	}

	cmzn_material *findMaterialByName(const char *name)
	{
		cmzn_material *material = FIND_BY_IDENTIFIER_IN_MANAGER(cmzn_material,name)(name,
			this->materialManager);
		if (material)
		{
			return cmzn_material_access(material);
		}
		return 0;
	}

	cmzn_material *getDefaultMaterial()
	{
		if (this->defaultMaterial)
			return this->defaultMaterial->access();
		return 0;
	}

	int setDefaultMaterial(cmzn_material *material)
	{
		REACCESS(cmzn_material)(&this->defaultMaterial, material);
		return CMZN_OK;
	}

	cmzn_material *getDefaultSelectedMaterial()
	{
		if (this->defaultSelectedMaterial)
			return this->defaultSelectedMaterial->access();
		return 0;
	}

	int setDefaultSelectedMaterial(cmzn_material *material)
	{
		REACCESS(cmzn_material)(&this->defaultSelectedMaterial, material);
		return CMZN_OK;
	}

	cmzn_material *getDefaultSurfaceMaterial()
	{
		if (this->defaultSurfaceMaterial)
			return this->defaultSurfaceMaterial->access();
		return 0;
	}

	int setDefaultSurfaceMaterial(cmzn_material *material)
	{
		REACCESS(cmzn_material)(&this->defaultSurfaceMaterial, material);
		return CMZN_OK;
	}

};

cmzn_materialmodule_id cmzn_materialmodule_access(
	cmzn_materialmodule_id materialmodule)
{
	if (materialmodule)
		return materialmodule->access();
	return 0;
}

int cmzn_materialmodule_destroy(cmzn_materialmodule_id *materialmodule_address)
{
	if (materialmodule_address)
		return cmzn_materialmodule::deaccess(*materialmodule_address);
	return CMZN_ERROR_ARGUMENT;
}

cmzn_material_id cmzn_materialmodule_create_material(
	cmzn_materialmodule_id materialmodule)
{
	if (materialmodule)
		return materialmodule->createMaterial();
	return 0;
}

cmzn_materialiterator_id cmzn_materialmodule_create_materialiterator(
	cmzn_materialmodule_id materialmodule)
{
	if (materialmodule)
		return materialmodule->createMaterialiterator();
	return 0;
}

struct MANAGER(cmzn_material) *cmzn_materialmodule_get_manager(
	cmzn_materialmodule_id materialmodule)
{
	if (materialmodule)
		return materialmodule->getManager();
	return 0;
}

struct MANAGER(cmzn_spectrum) *cmzn_materialmodule_get_spectrum_manager(
	struct cmzn_materialmodule *materialmodule)
{
	if (materialmodule)
		return materialmodule->getSpectrumManager();
	return 0;
}

int cmzn_materialmodule_define_standard_materials(
	cmzn_materialmodule_id materialmodule)
{
	struct Startup_material_definition
		startup_materials[] =
		{
			{"black",
			 /*ambient*/ { 0.00, 0.00, 0.00},
			 /*diffuse*/ { 0.00, 0.00, 0.00},
			 /*emission*/{ 0.00, 0.00, 0.00},
			 /*specular*/{ 0.30, 0.30, 0.30},
	 /*alpha*/1.0,
			 /*shininess*/0.2},
			{"blue",
			 /*ambient*/ { 0.00, 0.00, 1.00},
			 /*diffuse*/ { 0.00, 0.00, 1.00},
			 /*emission*/{ 0.00, 0.00, 0.00},
			 /*specular*/{ 0.10, 0.10, 0.10},
			 /*alpha*/1.0,
			 /*shininess*/0.2},
			{"bone",
			 /*ambient*/ { 0.70, 0.70, 0.60},
			 /*diffuse*/ { 0.90, 0.90, 0.70},
			 /*emission*/{ 0.00, 0.00, 0.00},
			 /*specular*/{ 0.10, 0.10, 0.10},
			 /*alpha*/1.0,
			 /*shininess*/0.2},
			{"brown",
			 /*ambient*/ { 0.5, 0.25, 0.0},
			 /*diffuse*/ { 0.5, 0.25, 0.0},
			 /*emission*/{ 0.00, 0.00, 0.00},
			 /*specular*/{ 0.10, 0.10, 0.10},
			 /*alpha*/1.0,
			 /*shininess*/0.2},
			{"cyan",
			 /*ambient*/ { 0.00, 1.00, 1.00},
			 /*diffuse*/ { 0.00, 1.00, 1.00},
			 /*emission*/{ 0.00, 0.00, 0.00},
			 /*specular*/{ 0.10, 0.10, 0.10},
			 /*alpha*/1.0,
			 /*shininess*/0.2},
			{"grey25",
			 /*ambient*/ { 0.25, 0.25, 0.25},
			 /*diffuse*/ { 0.25, 0.25, 0.25},
			 /*emission*/{ 0.00, 0.00, 0.00},
			 /*specular*/{ 0.10, 0.10, 0.10},
			 /*alpha*/1.0,
			 /*shininess*/0.2},
			{"grey50",
			 /*ambient*/ { 0.50, 0.50, 0.50},
			 /*diffuse*/ { 0.50, 0.50, 0.50},
			 /*emission*/{ 0.00, 0.00, 0.00},
			 /*specular*/{ 0.10, 0.10, 0.10},
			 /*alpha*/1.0,
			 /*shininess*/0.2},
			{"grey75",
			 /*ambient*/ { 0.75, 0.75, 0.75},
			 /*diffuse*/ { 0.75, 0.75, 0.75},
			 /*emission*/{ 0.00, 0.00, 0.00},
			 /*specular*/{ 0.10, 0.10, 0.10},
			 /*alpha*/1.0,
			 /*shininess*/0.2},
			{"gold",
			 /*ambient*/ { 1.00, 0.40, 0.00},
			 /*diffuse*/ { 1.00, 0.70, 0.00},
			 /*emission*/{ 0.00, 0.00, 0.00},
			 /*specular*/{ 0.50, 0.50, 0.50},
			 /*alpha*/1.0,
			 /*shininess*/0.3},
			{"green",
			 /*ambient*/ { 0.00, 1.00, 0.00},
			 /*diffuse*/ { 0.00, 1.00, 0.00},
			 /*emission*/{ 0.00, 0.00, 0.00},
			 /*specular*/{ 0.10, 0.10, 0.10},
			 /*alpha*/1.0,
			 /*shininess*/0.2},
			{"magenta",
			 /*ambient*/ { 1.00, 0.00, 1.00},
			 /*diffuse*/ { 1.00, 0.00, 1.00},
			 /*emission*/{ 0.00, 0.00, 0.00},
			 /*specular*/{ 0.10, 0.10, 0.10},
			 /*alpha*/1.0,
			 /*shininess*/0.2},
			{"muscle",
			 /*ambient*/ { 0.40, 0.14, 0.11},
			 /*diffuse*/ { 0.50, 0.12, 0.10},
			 /*emission*/{ 0.00, 0.00, 0.00},
			 /*specular*/{ 0.30, 0.50, 0.50},
			 /*alpha*/1.0,
			 /*shininess*/0.2},
			{"orange",
			 /*ambient*/ { 1.00, 0.5, 0.00},
			 /*diffuse*/ { 1.00, 0.5, 0.00},
			 /*emission*/{ 0.00, 0.00, 0.00},
			 /*specular*/{ 0.10, 0.10, 0.10},
			 /*alpha*/1.0,
			 /*shininess*/0.2},
			{"red",
			 /*ambient*/ { 1.00, 0.00, 0.00},
			 /*diffuse*/ { 1.00, 0.00, 0.00},
			 /*emission*/{ 0.00, 0.00, 0.00},
			 /*specular*/{ 0.10, 0.10, 0.10},
			 /*alpha*/1.0,
			 /*shininess*/0.2},
			{"silver",
			 /*ambient*/ { 0.40, 0.40, 0.40},
			 /*diffuse*/ { 0.70, 0.70, 0.70},
			 /*emission*/{ 0.00, 0.00, 0.00},
			 /*specular*/{ 0.50, 0.50, 0.50},
			 /*alpha*/1.0,
			 /*shininess*/0.3},
			{"tissue",
			 /*ambient*/ { 0.90, 0.70, 0.50},
			 /*diffuse*/ { 0.90, 0.70, 0.50},
			 /*emission*/{ 0.00, 0.00, 0.00},
			 /*specular*/{ 0.20, 0.20, 0.30},
			 /*alpha*/1.0,
			 /*shininess*/0.2f},
			{"white",
			 /*ambient*/ { 1.00, 1.00, 1.00},
			 /*diffuse*/ { 1.00, 1.00, 1.00},
			 /*emission*/{ 0.00, 0.00, 0.00},
			 /*specular*/{ 0.00, 0.00, 0.00},
			 /*alpha*/1.0,
			 /*shininess*/0.0},
			{"yellow",
			 /*ambient*/ { 1.00, 1.00, 0.00},
			 /*diffuse*/ { 1.00, 1.00, 0.00},
			 /*emission*/{ 0.00, 0.00, 0.00},
			 /*specular*/{ 0.10, 0.10, 0.10},
			 /*alpha*/1.0,
			 /*shininess*/0.2}
		};
	int i, return_code;
	int number_of_startup_materials = sizeof(startup_materials) /
		sizeof(struct Startup_material_definition);
	cmzn_material *material = 0;

	if (materialmodule)
	{
		for (i = 0; i < number_of_startup_materials; i++)
		{
			material = NULL;
			if (NULL != (material = cmzn_materialmodule_find_material_by_name(
				materialmodule, startup_materials[i].name)))
			{
				cmzn_material_destroy(&material);
			}
			else if ((NULL != (material = cmzn_materialmodule_create_material(materialmodule))) &&
				cmzn_material_set_name(material, startup_materials[i].name))
			{
				cmzn_material_set_attribute_real3(material,
					CMZN_MATERIAL_ATTRIBUTE_AMBIENT, &startup_materials[i].ambient[0]);
				cmzn_material_set_attribute_real3(material,
					CMZN_MATERIAL_ATTRIBUTE_DIFFUSE, &startup_materials[i].diffuse[0]);
				cmzn_material_set_attribute_real3(material,
					CMZN_MATERIAL_ATTRIBUTE_EMISSION, & startup_materials[i].emission[0]);
				cmzn_material_set_attribute_real3(material,
					CMZN_MATERIAL_ATTRIBUTE_SPECULAR, &startup_materials[i].specular[0]);
				cmzn_material_set_attribute_real(material,
					CMZN_MATERIAL_ATTRIBUTE_ALPHA, startup_materials[i].alpha);
				cmzn_material_set_attribute_real(material,
					CMZN_MATERIAL_ATTRIBUTE_SHININESS, startup_materials[i].shininess);
				cmzn_material_set_managed(material, true);
				material->module = materialmodule;
				cmzn_material_destroy(&material);
			}
		}

		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_materialmodule_define_standard_materials.  Invalid argument(s)");
		return_code = 0;
	}

	return return_code;
}

int cmzn_materialmodule_begin_change(cmzn_materialmodule_id materialmodule)
{
	if (materialmodule)
		return materialmodule->beginChange();
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_materialmodule_end_change(cmzn_materialmodule_id materialmodule)
{
	if (materialmodule)
		return materialmodule->endChange();
   return CMZN_ERROR_ARGUMENT;
}

cmzn_material_id cmzn_materialmodule_find_material_by_name(
	cmzn_materialmodule_id materialmodule, const char *name)
{
	if (materialmodule)
		return materialmodule->findMaterialByName(name);
   return 0;
}

cmzn_material_id cmzn_materialmodule_get_default_material(
	cmzn_materialmodule_id materialmodule)
{
	if (materialmodule)
		return materialmodule->getDefaultMaterial();
	return 0;
}

int cmzn_materialmodule_set_default_material(
	cmzn_materialmodule_id materialmodule, cmzn_material_id material)
{
	if (materialmodule)
		return materialmodule->setDefaultMaterial(material);
	return 0;
}

cmzn_material_id cmzn_materialmodule_get_default_selected_material(
	cmzn_materialmodule_id materialmodule)
{
	if (materialmodule)
		return materialmodule->getDefaultSelectedMaterial();
	return 0;
}

int cmzn_materialmodule_set_default_selected_material(
	cmzn_materialmodule_id materialmodule, cmzn_material_id material)
{
	if (materialmodule)
		return materialmodule->setDefaultSelectedMaterial(material);
	return 0;
}

cmzn_material_id cmzn_materialmodule_get_default_surface_material(
	cmzn_materialmodule_id materialmodule)
{
	if (materialmodule)
		return materialmodule->getDefaultSurfaceMaterial();
	return 0;
}

int cmzn_materialmodule_set_default_surface_material(
	cmzn_materialmodule_id materialmodule, cmzn_material_id material)
{
	if (materialmodule)
		return materialmodule->setDefaultSurfaceMaterial(material);
	return 0;
}

cmzn_materialmodule_id cmzn_materialmodule_create(
	struct MANAGER(cmzn_spectrum) *spectrum_manager)
{
	cmzn_materialmodule *materialmodule = cmzn_materialmodule::create();
	cmzn_material *defaultMaterial = 0, *defaultSelectedMaterial = 0;
	materialmodule->setSpectrumManager(spectrum_manager);
	struct Material_definition
	{
		double ambient[3];
		double diffuse[3];
		double emission[3];
		double specular[3];
		double alpha;
		double shininess;
	}
	default_material = {
		/*ambient*/ { 1.00, 1.00, 1.00},
		/*diffuse*/ { 1.00, 1.00, 1.00},
		/*emission*/{ 0.00, 0.00, 0.00},
		/*specular*/{ 0.00, 0.00, 0.00},
		/*alpha*/1.0,
		/*shininess*/0.0},
	default_selected = {
		/*ambient*/ { 1.00, 0.20, 0.00},
		/*diffuse*/ { 1.00, 0.20, 0.00},
		/*emission*/{ 0.00, 0.00, 0.00},
		/*specular*/{ 0.00, 0.00, 0.00},
		/*alpha*/1.0,
		/*shininess*/0.0};
	defaultMaterial = cmzn_materialmodule_create_material(
		materialmodule);
	cmzn_material_set_name(defaultMaterial, "default");
	cmzn_material_set_attribute_real3(defaultMaterial,
		CMZN_MATERIAL_ATTRIBUTE_AMBIENT, &default_material.ambient[0]);
	cmzn_material_set_attribute_real3(defaultMaterial,
		CMZN_MATERIAL_ATTRIBUTE_DIFFUSE, &default_material.diffuse[0]);
	cmzn_material_set_attribute_real3(defaultMaterial,
		CMZN_MATERIAL_ATTRIBUTE_EMISSION, &default_material.emission[0]);
	cmzn_material_set_attribute_real3(defaultMaterial,
		CMZN_MATERIAL_ATTRIBUTE_SPECULAR, &default_material.specular[0]);
	cmzn_material_set_attribute_real(defaultMaterial,
		CMZN_MATERIAL_ATTRIBUTE_ALPHA, default_material.alpha);
	cmzn_material_set_attribute_real(defaultMaterial,
		CMZN_MATERIAL_ATTRIBUTE_SHININESS, default_material.shininess);
	cmzn_material_set_managed(defaultMaterial, true);
	defaultMaterial->module = materialmodule;
	materialmodule->setDefaultMaterial(defaultMaterial);
	cmzn_material_destroy(&defaultMaterial);

	defaultSelectedMaterial = cmzn_materialmodule_create_material(
		materialmodule);
	cmzn_material_set_name(defaultSelectedMaterial, "default_selected");
	cmzn_material_set_attribute_real3(defaultSelectedMaterial,
		CMZN_MATERIAL_ATTRIBUTE_AMBIENT, &default_selected.ambient[0]);
	cmzn_material_set_attribute_real3(defaultSelectedMaterial,
		CMZN_MATERIAL_ATTRIBUTE_DIFFUSE, &default_selected.diffuse[0]);
	cmzn_material_set_attribute_real3(defaultSelectedMaterial,
		CMZN_MATERIAL_ATTRIBUTE_EMISSION, &default_selected.emission[0]);
	cmzn_material_set_attribute_real3(defaultSelectedMaterial,
		CMZN_MATERIAL_ATTRIBUTE_SPECULAR, &default_selected.specular[0]);
	cmzn_material_set_attribute_real(defaultSelectedMaterial,
		CMZN_MATERIAL_ATTRIBUTE_ALPHA, default_selected.alpha);
	cmzn_material_set_attribute_real(defaultSelectedMaterial,
		CMZN_MATERIAL_ATTRIBUTE_SHININESS, default_selected.shininess);
	cmzn_material_set_managed(defaultSelectedMaterial, true);
	defaultSelectedMaterial->module = materialmodule;
	materialmodule->setDefaultSelectedMaterial(defaultSelectedMaterial);
	cmzn_material_destroy(&defaultSelectedMaterial);

	return materialmodule;
}

cmzn_material *cmzn_material_create_private()
{
	cmzn_material *material = 0;

	/* allocate memory for structure */
	if (ALLOCATE(material,cmzn_material,1))
	{
		material->name = 0;
		material->access_count=1;
		material->per_pixel_lighting_flag = 0;
		material->bump_mapping_flag = 0;
		(material->ambient).red=1;
		(material->ambient).green=1;
		(material->ambient).blue=1;
		(material->diffuse).red=1;
		(material->diffuse).green=1;
		(material->diffuse).blue=1;
		material->alpha=1;
		(material->emission).red=0;
		(material->emission).green=0;
		(material->emission).blue=0;
		(material->specular).red=0;
		(material->specular).green=0;
		(material->specular).blue=0;
		material->shininess=0;
		material->spectrum=(struct cmzn_spectrum *)NULL;
		material->spectrum_manager_callback_id=NULL;
		(material->image_texture).texture=(struct Texture *)NULL;
		(material->image_texture).field  = NULL;
		(material->image_texture).callback_id = NULL;
		(material->image_texture).material = material;
		(material->second_image_texture).texture=(struct Texture *)NULL;
		(material->second_image_texture).field  = NULL;
		(material->second_image_texture).callback_id = NULL;
		(material->second_image_texture).material = material;
		(material->third_image_texture).texture=(struct Texture *)NULL;
		(material->third_image_texture).field  = NULL;
		(material->third_image_texture).callback_id = NULL;
		(material->third_image_texture).material = material;
		(material->fourth_image_texture).texture=(struct Texture *)NULL;
		(material->fourth_image_texture).field  = NULL;
		(material->fourth_image_texture).callback_id = NULL;
		(material->fourth_image_texture).material = material;
		material->module = (struct cmzn_materialmodule *)NULL;
		material->lit_volume_normal_scaling[0] = 1.0;
		material->lit_volume_normal_scaling[1] = 1.0;
		material->lit_volume_normal_scaling[2] = 1.0;
		material->lit_volume_normal_scaling[3] = 1.0;
		material->program = (cmzn_shaderprogram_id )NULL;
		material->uniforms = 0;
		material->isManagedFlag = false;
		material->manager = (struct MANAGER(cmzn_material) *)NULL;
		material->manager_change_status = MANAGER_CHANGE_NONE(cmzn_material);
		material->executed_as_order_independent = 0;
		material->order_program = (cmzn_shaderprogram_id )NULL;
#if defined (OPENGL_API)
				material->display_list=0;
				material->brightness_texture_id=0;
#endif /* defined (OPENGL_API) */
				material->compile_status = GRAPHICS_NOT_COMPILED;
	}
	return (material);
}

/*******************************************************************************
 * Reset the material_image_texture to hold NULL object.
 *
 * @param image_texture  Pointer to the image_texture object to be reset.
 * @return  1 if successfully reset, otherwise 0.
 */
int Material_image_texture_reset(struct Material_image_texture *image_texture)
{
	int return_code = 1;
	if (image_texture)
	{
		if (image_texture->texture)
			DEACCESS(Texture)(&(image_texture->texture));
		if (image_texture->field)
		{
			// field manager may be nullptr during clean-up. This means it has been destroyed and callback_id is defunct.
			MANAGER(Computed_field) *field_manager = cmzn_field_image_base_cast(image_texture->field)->getManager();
			if (field_manager && image_texture->callback_id)
			{
				MANAGER_DEREGISTER(Computed_field)(image_texture->callback_id, field_manager);
				image_texture->callback_id = nullptr;
			}
			else
			{
				image_texture->callback_id = nullptr;
			}
			cmzn_field_image_destroy(&(image_texture->field));
		}
	}
	else
	{
		return_code = 0;
		display_message(ERROR_MESSAGE,"Material_image_texture_reset.  Invalid argument");
	}

	return return_code;
}

int DESTROY(cmzn_material)(struct cmzn_material **material_address)
/*******************************************************************************
LAST MODIFIED : 3 August 1998

DESCRIPTION :
Frees the memory for the material and sets <*material_address> to NULL.
==============================================================================*/
{
	int return_code;
	cmzn_material *material;

	if (material_address&&(material= *material_address))
	{
		if (0==material->access_count)
		{
			DEALLOCATE(material->name);
#if defined (OPENGL_API)
			if (material->display_list)
			{
				glDeleteLists(material->display_list, 1);
			}
#endif /* defined (OPENGL_API) */
			if (material->spectrum)
			{
				DEACCESS(cmzn_spectrum)(&(material->spectrum));
			}
			if (material->module &&
				material->spectrum_manager_callback_id)
			{
				MANAGER_DEREGISTER(cmzn_spectrum)(
					material->spectrum_manager_callback_id,
					material->module->getSpectrumManager());
				material->spectrum_manager_callback_id=NULL;
			}
			Material_image_texture_reset(&(material->image_texture));
			Material_image_texture_reset(&(material->second_image_texture));
			Material_image_texture_reset(&(material->third_image_texture));
			Material_image_texture_reset(&(material->fourth_image_texture));
			if (material->program)
			{
				cmzn_shaderprogram_destroy(&(material->program));
			}
			if (material->order_program)
			{
				cmzn_shaderprogram_destroy(&(material->order_program));
			}
			if (material->uniforms)
			{
				cmzn_shaderuniforms_destroy(&material->uniforms);
			}
			DEALLOCATE(*material_address);
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"DESTROY(cmzn_material).  Graphical material %s has non-zero access count",
				material->name);
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(cmzn_material).  Missing material");
		return_code=0;
	}
	return (return_code);
}

/***************************************************************************//**
 * Something has changed in the regional computed field manager.
 * Check if the field being used is changed, if so update the material.
 */
static void Material_image_field_change(
	struct MANAGER_MESSAGE(Computed_field) *message, void *material_image_texture_void)
{
	struct Material_image_texture *image_texture =
		(struct Material_image_texture *)material_image_texture_void;
	if (message && image_texture)
	{
		int change = MANAGER_MESSAGE_GET_OBJECT_CHANGE(Computed_field)(
				message, cmzn_field_image_base_cast(image_texture->field));
		if (change & MANAGER_CHANGE_RESULT(Computed_field))
		{
			if (image_texture->material->compile_status != GRAPHICS_NOT_COMPILED)
			{
				image_texture->material->compile_status = CHILD_GRAPHICS_NOT_COMPILED;
			}
			if (image_texture->material->manager)
				MANAGER_BEGIN_CACHE(cmzn_material)(image_texture->material->manager);
			REACCESS(Texture)(&(image_texture->texture),
			cmzn_field_image_get_texture(image_texture->field));
			MANAGED_OBJECT_CHANGE(cmzn_material)(image_texture->material,
				MANAGER_CHANGE_FULL_RESULT(cmzn_material));
			if (image_texture->material->manager)
				MANAGER_END_CACHE(cmzn_material)(image_texture->material->manager);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Material_image_field_change.  Invalid argument(s)");
	}
}

/***************************************************************************//**
 * Set the field and update all the related objects in material_image_texture.
 * This will also create a callback for computed field.
 */
int Material_image_texture_set_field(struct Material_image_texture *image_texture,
	cmzn_field_image_id field)
{
	int return_code = 0;
	if (image_texture)
	{
		return_code = 1;
		if (image_texture->field)
		{
			MANAGER(Computed_field) *field_manager = cmzn_field_image_base_cast(image_texture->field)->getManager();
			cmzn_field_image_destroy(&(image_texture->field));
			image_texture->field = nullptr;
			if ((field_manager) && (image_texture->callback_id))
			{
				MANAGER_DEREGISTER(Computed_field)(image_texture->callback_id, field_manager);
				image_texture->callback_id = nullptr;
			}
			if (image_texture->texture)
				DEACCESS(Texture)(&(image_texture->texture));
		}
		if (field)
		{
			MANAGER(Computed_field) *field_manager = cmzn_field_image_base_cast(field)->getManager();
			if (field_manager)
			{
				image_texture->callback_id=
					MANAGER_REGISTER(Computed_field)(Material_image_field_change,
						(void *)image_texture, field_manager);
				image_texture->field = field;
				cmzn_field_access(cmzn_field_image_base_cast(image_texture->field));
				image_texture->texture = ACCESS(Texture)(cmzn_field_image_get_texture(image_texture->field));
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			 "Material_image_texture_set_field.  Missing Material_image_texture");
		return_code = 0;
	}
	return return_code;
}

PROTOTYPE_MANAGER_COPY_WITH_IDENTIFIER_FUNCTION(cmzn_material,name)
{
	char *name;
	int return_code;

	ENTER(MANAGER_COPY_WITH_IDENTIFIER(cmzn_material,name));
	/* check arguments */
	if (source&&destination)
	{
		if (source->name)
		{
			if (ALLOCATE(name,char,strlen(source->name)+1))
			{
				strcpy(name,source->name);
				return_code=1;
			}
			else
			{
				display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITH_IDENTIFIER(cmzn_material,name).  Insufficient memory");
				return_code=0;
			}
		}
		else
		{
			name=(char *)NULL;
			return_code=1;
		}
		if (return_code)
		{
			return_code = MANAGER_COPY_WITHOUT_IDENTIFIER(cmzn_material,name)(destination, source);
			if (return_code)
			{
				/* copy values */
				DEALLOCATE(destination->name);
				destination->name=name;
			}
			else
			{
				DEALLOCATE(name);
				display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITH_IDENTIFIER(cmzn_material,name).  Could not copy without identifier");
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITH_IDENTIFIER(cmzn_material,name).  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* MANAGER_COPY_WITH_IDENTIFIER(cmzn_material,name) */

PROTOTYPE_MANAGER_COPY_WITHOUT_IDENTIFIER_FUNCTION(cmzn_material,name)
{
	int return_code;

	ENTER(MANAGER_COPY_WITHOUT_IDENTIFIER(cmzn_material,name));
	/* check arguments */
	if (source&&destination)
	{
		/* copy values */
		(destination->diffuse).red=(source->diffuse).red;
		(destination->diffuse).green=(source->diffuse).green;
		(destination->diffuse).blue=(source->diffuse).blue;
		(destination->ambient).red=(source->ambient).red;
		(destination->ambient).green=(source->ambient).green;
		(destination->ambient).blue=(source->ambient).blue;
		(destination->emission).red=(source->emission).red;
		(destination->emission).green=(source->emission).green;
		(destination->emission).blue=(source->emission).blue;
		(destination->specular).red=(source->specular).red;
		(destination->specular).green=(source->specular).green;
		(destination->specular).blue=(source->specular).blue;
		destination->shininess=source->shininess;
		destination->alpha=source->alpha;
		if (source->module)
		{
			destination->module = source->module;
		}
		else
		{
			destination->module = (struct cmzn_materialmodule *)NULL;
		}
		REACCESS(cmzn_shaderprogram)(&destination->program, source->program);
		destination->lit_volume_normal_scaling[0] =
			source->lit_volume_normal_scaling[0];
		destination->lit_volume_normal_scaling[1] =
			source->lit_volume_normal_scaling[1];
		destination->lit_volume_normal_scaling[2] =
			source->lit_volume_normal_scaling[2];
		destination->lit_volume_normal_scaling[3] =
			source->lit_volume_normal_scaling[3];
		REACCESS(cmzn_spectrum)(&(destination->spectrum), source->spectrum);
		if (destination->spectrum)
		{
			if (destination->module &&
				(!destination->spectrum_manager_callback_id))
			{
				destination->spectrum_manager_callback_id=
					MANAGER_REGISTER(cmzn_spectrum)(Graphical_material_Spectrum_change,
						(void *)destination, destination->module->getSpectrumManager());
			}
		}
		else
		{
			if (destination->module &&
				destination->spectrum_manager_callback_id)
			{
				MANAGER_DEREGISTER(cmzn_spectrum)(
					destination->spectrum_manager_callback_id,
					destination->module->getSpectrumManager());
				destination->spectrum_manager_callback_id=NULL;
			}
		}
		Material_image_texture_set_field(&(destination->image_texture), source->image_texture.field);
		Material_image_texture_set_field(&(destination->second_image_texture), source->second_image_texture.field);
		Material_image_texture_set_field(&(destination->third_image_texture), source->third_image_texture.field);
		Material_image_texture_set_field(&(destination->fourth_image_texture), source->fourth_image_texture.field);
		REACCESS(cmzn_shaderuniforms)(&(destination->uniforms), source->uniforms);
		/* flag destination display list as no longer current */
		destination->compile_status = GRAPHICS_NOT_COMPILED;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
"MANAGER_COPY_WITHOUT_IDENTIFIER(cmzn_material,name).  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* MANAGER_COPY_WITHOUT_IDENTIFIER(cmzn_material,name) */

PROTOTYPE_MANAGER_COPY_IDENTIFIER_FUNCTION(cmzn_material,name,const char *)
{
	char *destination_name = NULL;
	int return_code;

	ENTER(MANAGER_COPY_IDENTIFIER(cmzn_material,name));
	/* check arguments */
	if (name&&destination)
	{
		if (name)
		{
			if (ALLOCATE(destination_name,char,strlen(name)+1))
			{
				strcpy(destination_name,name);
				return_code=1;
			}
			else
			{
				display_message(ERROR_MESSAGE,
			"MANAGER_COPY_IDENTIFIER(cmzn_material,name).  Insufficient memory");
				return_code=0;
			}
		}
		else
		{
			name=(char *)NULL;
			return_code=1;
		}
		if (return_code)
		{
			/* copy name */
			DEALLOCATE(destination->name);
			destination->name=destination_name;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"MANAGER_COPY_IDENTIFIER(cmzn_material,name).  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
}

const char *Graphical_material_name(cmzn_material *material)
/*******************************************************************************
LAST MODIFIED : 29 November 1997

DESCRIPTION :
While the GET_NAME macro returns a copy of the name of an object, this function
has been created for returning just a pointer to the material's name, or some
other string if the name is invalid, suitable for putting in printf statements.
Be careful with the returned value: esp. do not modify or DEALLOCATE it!
==============================================================================*/
{
	const char *return_name;
	static char error_string[]="ERROR";
	static char no_name_string[]="NO NAME";

	ENTER(Graphical_material_name);
	if (material)
	{
		if (material->name)
		{
			return_name=material->name;
		}
		else
		{
			return_name=no_name_string;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphical_material_name.  Missing material");
		return_name=error_string;
	}
	LEAVE;

	return (return_name);
} /* Graphical_material_name */

/***************************************************************************//**
 * Broadcast changes in the graphical material to be propagated to objects that
 * uses it through manager that owns it.
 *
 * @param material  Modified material to be broadcast.
 * @return 1 on success, 0 on failure
 */
int Graphical_material_changed(cmzn_material *material)
{
	int return_code;

	ENTER(Graphical_material_changed);
	if (material)
	{
		return_code = MANAGED_OBJECT_CHANGE(cmzn_material)(material,
			MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER(cmzn_material));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphical_material_changed.  Invalid argument");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
}

int Graphical_material_get_ambient(cmzn_material *material,
	struct Colour *ambient)
/*******************************************************************************
LAST MODIFIED : 28 November 1997

DESCRIPTION :
Returns the ambient colour of the material.
==============================================================================*/
{
	int return_code;

	ENTER(Graphical_material_get_ambient);
	if (material&&ambient)
	{
		ambient->red=material->ambient.red;
		ambient->green=material->ambient.green;
		ambient->blue=material->ambient.blue;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphical_material_get_ambient.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Graphical_material_get_ambient */

int Graphical_material_set_ambient(cmzn_material *material,
	struct Colour *ambient)
/*******************************************************************************
LAST MODIFIED : 15 October 1998

DESCRIPTION :
Sets the ambient colour of the material.
==============================================================================*/
{
	int return_code;

	ENTER(Graphical_material_set_ambient);
	if (material&&ambient)
	{
		material->ambient.red=ambient->red;
		material->ambient.green=ambient->green;
		material->ambient.blue=ambient->blue;
		/* display list needs to be compiled again */
		material->compile_status = GRAPHICS_NOT_COMPILED;
		Graphical_material_changed(material);
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphical_material_set_ambient.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Graphical_material_set_ambient */

int Graphical_material_get_diffuse(cmzn_material *material,
	struct Colour *diffuse)
/*******************************************************************************
LAST MODIFIED : 28 November 1997

DESCRIPTION :
Returns the diffuse colour of the material.
==============================================================================*/
{
	int return_code;

	ENTER(Graphical_material_get_diffuse);
	if (material&&diffuse)
	{
		diffuse->red=material->diffuse.red;
		diffuse->green=material->diffuse.green;
		diffuse->blue=material->diffuse.blue;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphical_material_get_diffuse.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Graphical_material_get_diffuse */

int Graphical_material_set_diffuse(cmzn_material *material,
	struct Colour *diffuse)
/*******************************************************************************
LAST MODIFIED : 15 October 1998

DESCRIPTION :
Sets the diffuse colour of the material.
==============================================================================*/
{
	int return_code;

	ENTER(Graphical_material_set_diffuse);
	if (material&&diffuse)
	{
		material->diffuse.red=diffuse->red;
		material->diffuse.green=diffuse->green;
		material->diffuse.blue=diffuse->blue;
		/* display list needs to be compiled again */
		material->compile_status = GRAPHICS_NOT_COMPILED;
		Graphical_material_changed(material);
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphical_material_set_diffuse.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Graphical_material_set_diffuse */

int Graphical_material_get_emission(cmzn_material *material,
	struct Colour *emission)
/*******************************************************************************
LAST MODIFIED : 28 November 1997

DESCRIPTION :
Returns the emission colour of the material.
==============================================================================*/
{
	int return_code;

	ENTER(Graphical_material_get_emission);
	if (material&&emission)
	{
		emission->red=material->emission.red;
		emission->green=material->emission.green;
		emission->blue=material->emission.blue;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphical_material_get_emission.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Graphical_material_get_emission */

int Graphical_material_set_emission(cmzn_material *material,
	struct Colour *emission)
/*******************************************************************************
LAST MODIFIED : 15 October 1998

DESCRIPTION :
Sets the emission colour of the material.
==============================================================================*/
{
	int return_code;

	ENTER(Graphical_material_set_emission);
	if (material&&emission)
	{
		material->emission.red=emission->red;
		material->emission.green=emission->green;
		material->emission.blue=emission->blue;
		/* display list needs to be compiled again */
		material->compile_status = GRAPHICS_NOT_COMPILED;
		Graphical_material_changed(material);
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphical_material_set_emission.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Graphical_material_set_emission */

int Graphical_material_get_specular(cmzn_material *material,
	struct Colour *specular)
/*******************************************************************************
LAST MODIFIED : 28 November 1997

DESCRIPTION :
Returns the specular colour of the material.
==============================================================================*/
{
	int return_code;

	ENTER(Graphical_material_get_specular);
	if (material&&specular)
	{
		specular->red=material->specular.red;
		specular->green=material->specular.green;
		specular->blue=material->specular.blue;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphical_material_get_specular.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Graphical_material_get_specular */

int Graphical_material_set_specular(cmzn_material *material,
	struct Colour *specular)
/*******************************************************************************
LAST MODIFIED : 15 October 1998

DESCRIPTION :
Sets the specular colour of the material.
==============================================================================*/
{
	int return_code;

	ENTER(Graphical_material_set_specular);
	if (material&&specular)
	{
		material->specular.red=specular->red;
		material->specular.green=specular->green;
		material->specular.blue=specular->blue;
		/* display list needs to be compiled again */
		material->compile_status = GRAPHICS_NOT_COMPILED;
		Graphical_material_changed(material);
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphical_material_set_specular.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Graphical_material_set_specular */

int Graphical_material_get_alpha(cmzn_material *material,
	MATERIAL_PRECISION *alpha)
/*******************************************************************************
LAST MODIFIED : 28 November 1997

DESCRIPTION :
Returns the alpha value of the material.
==============================================================================*/
{
	int return_code;

	ENTER(Graphical_material_get_alpha);
	if (material&&alpha)
	{
		*alpha=material->alpha;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphical_material_get_alpha.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Graphical_material_get_alpha */

int Graphical_material_set_alpha(cmzn_material *material,
	MATERIAL_PRECISION alpha)
/*******************************************************************************
LAST MODIFIED : 15 October 1998

DESCRIPTION :
Sets the alpha value of the material.
==============================================================================*/
{
	int return_code;

	ENTER(Graphical_material_set_alpha);
	if (material&&(0.0 <= alpha)&&(1.0 >= alpha))
	{
		material->alpha=alpha;
		/* display list needs to be compiled again */
		material->compile_status = GRAPHICS_NOT_COMPILED;
		Graphical_material_changed(material);
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphical_material_set_alpha.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Graphical_material_set_alpha */

int Graphical_material_get_shininess(cmzn_material *material,
	MATERIAL_PRECISION *shininess)
/*******************************************************************************
LAST MODIFIED : 28 November 1997

DESCRIPTION :
Returns the shininess value of the material.
==============================================================================*/
{
	int return_code;

	ENTER(Graphical_material_get_shininess);
	if (material&&shininess)
	{
		*shininess=material->shininess;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphical_material_get_shininess.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Graphical_material_get_shininess */

int Graphical_material_set_shininess(cmzn_material *material,
	MATERIAL_PRECISION shininess)
/*******************************************************************************
LAST MODIFIED : 28 November 1997

DESCRIPTION :
Sets the shininess value of the material.
==============================================================================*/
{
	int return_code;

	ENTER(Graphical_material_set_shininess);
	if (material&&(0.0 <= shininess)&&(1.0 >= shininess))
	{
		material->shininess=shininess;
		/* display list needs to be compiled again */
		material->compile_status = GRAPHICS_NOT_COMPILED;
		Graphical_material_changed(material);
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphical_material_set_shininess.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Graphical_material_set_shininess */

int Graphical_material_get_bump_mapping_flag(cmzn_material *material)
/*******************************************************************************
LAST MODIFIED : 5 December 2007

DESCRIPTION :
Returns the flag set for bump_mapping.
==============================================================================*/
{
	 int return_code;

	 ENTER(Graphical_material_get_bump_mapping_flag);
	 if (material)
	 {
			return_code = material->bump_mapping_flag;
	 }
	 else
	 {
			display_message(ERROR_MESSAGE,
				 "Graphical_material_get_bump_mapping_flag.  Invalid argument(s)");
			return_code=0;
	 }
	LEAVE;

	return (return_code);
}

int Graphical_material_get_per_pixel_lighting_flag(cmzn_material *material)
/*******************************************************************************
LAST MODIFIED : 5 December 2007

DESCRIPTION :
Returns the flag set for per_pixel_lighting.
==============================================================================*/
{
	 int return_code;

	 ENTER(Graphical_material_get_per_pixel_lighting_flag);
	 if (material)
	 {
			return_code = material->per_pixel_lighting_flag;
	 }
	 else
	 {
			display_message(ERROR_MESSAGE,
				 "Graphical_material_get_per_pixel_flag.  Invalid argument(s)");
			return_code=0;
	 }
	LEAVE;

	return (return_code);
}

struct Texture *Graphical_material_get_texture(
	cmzn_material *material)
/*******************************************************************************
LAST MODIFIED : 12 February 1998

DESCRIPTION :
Returns the texture member of the material.
==============================================================================*/
{
	struct Texture *texture;

	ENTER(Graphical_material_get_texture);
	if (material)
	{
		texture=material->image_texture.texture;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphical_material_get_texture.  Missing material");
		texture=(struct Texture *)NULL;
	}
	LEAVE;

	return (texture);
} /* Graphical_material_get_texture */

struct Texture *Graphical_material_get_second_texture(
	cmzn_material *material)
/*******************************************************************************
LAST MODIFIED : 5 Dec 2007

DESCRIPTION :
Returns the second texture of the material.
==============================================================================*/
{
	struct Texture *texture;

	ENTER(Graphical_material_get_second_texture);
	if (material)
	{
		texture=material->second_image_texture.texture;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphical_material_get_second_texture.  Missing material");
		texture=(struct Texture *)NULL;
	}
	LEAVE;

	return (texture);
} /* Graphical_material_get_second_texture */

struct Texture *Graphical_material_get_third_texture(
	cmzn_material *material)
/*******************************************************************************
LAST MODIFIED : 5 Dec 2007

DESCRIPTION :
Returns the third texture of the material.
==============================================================================*/
{
	struct Texture *texture;

	ENTER(Graphical_material_get_third_texture);
	if (material)
	{
		texture=material->third_image_texture.texture;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphical_material_get_third_texture.  Missing material");
		texture=(struct Texture *)NULL;
	}
	LEAVE;

	return (texture);
} /* Graphical_material_get_third_texture */

struct Texture *Graphical_material_get_fourth_texture(
	cmzn_material *material)
/*******************************************************************************
LAST MODIFIED : 5 Dec 2007

DESCRIPTION :
Returns the fourth texture of the material.
==============================================================================*/
{
	struct Texture *texture;

	ENTER(Graphical_material_get_fourth_texture);
	if (material)
	{
		texture=material->fourth_image_texture.texture;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphical_material_get_fourth_texture.  Missing material");
		texture=(struct Texture *)NULL;
	}
	LEAVE;

	return (texture);
} /* Graphical_material_get_fourth_texture */

int Graphical_material_set_colour_lookup_spectrum(cmzn_material *material,
	struct cmzn_spectrum *spectrum)
/*******************************************************************************
LAST MODIFIED : 6 October 2006

DESCRIPTION :
Sets the spectrum member of the material.
==============================================================================*/
{
	int return_code;

	ENTER(Graphical_material_set_colour_lookup_spectrum);
	if (material)
	{
#if defined (GL_VERSION_1_3)
		if (Graphics_library_tentative_check_extension(GL_VERSION_1_3))
		{
			REACCESS(cmzn_spectrum)(&material->spectrum, spectrum);
			material->compile_status = GRAPHICS_NOT_COMPILED;
			Graphical_material_changed(material);
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"A colour lookup spectrum requires OpenGL version 1.3 or better which is "
				"not available on this display.");
			return_code = 0;
		}
#else /* defined (GL_VERSION_1_3) */
		USE_PARAMETER(spectrum);
		display_message(ERROR_MESSAGE,
			"Graphical_material_set_colour_lookup_spectrum.  "
			"OpenGL version 1.3 required for colour lookup spectrums and not compiled into this executable.");
		return_code=0;
#endif /* defined (GL_VERSION_1_3) */
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphical_material_set_colour_lookup_spectrum.  Missing material");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Graphical_material_set_colour_lookup_spectrum */

struct cmzn_spectrum *Graphical_material_get_colour_lookup_spectrum(
	cmzn_material *material)
/*******************************************************************************
LAST MODIFIED : 6 October 2006

DESCRIPTION :
Returns the spectrum member of the material.
==============================================================================*/
{
	struct cmzn_spectrum *spectrum;

	ENTER(Graphical_material_get_colour_lookup_spectrum);
	if (material)
	{
		spectrum=material->spectrum;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Graphical_material_get_colour_lookup_spectrum.  Missing material");
		spectrum=(struct cmzn_spectrum *)NULL;
	}
	LEAVE;

	return (spectrum);
} /* Graphical_material_get_colour_lookup_spectrum */

cmzn_field_id cmzn_material_get_texture_field(cmzn_material_id material,
	int texture_number)
{
	if (material)
	{
		cmzn_field_image_id texture_field = 0;
		switch (texture_number)
		{
			case 1:
				texture_field = material->image_texture.field;
				break;
			case 2:
				texture_field = material->second_image_texture.field;
				break;
			case 3:
				texture_field = material->third_image_texture.field;
				break;
			case 4:
				texture_field = material->fourth_image_texture.field;
				break;
			default:
				break;
		}
		if (texture_field)
			return cmzn_field_access(cmzn_field_image_base_cast(texture_field));
	}
	return 0;
}

int cmzn_material_set_texture_field(cmzn_material_id material,
	int texture_number, cmzn_field_id texture_field)
{
	cmzn_field_image_id image_field = cmzn_field_cast_image(texture_field);
	if (material && ((0 == texture_field) || image_field))
	{
		Material_image_texture *image_texture = 0;
		switch (texture_number)
		{
			case 1:
				image_texture = &(material->image_texture);
				break;
			case 2:
				image_texture = &(material->second_image_texture);
				break;
			case 3:
				image_texture = &(material->third_image_texture);
				break;
			case 4:
				image_texture = &(material->fourth_image_texture);
				break;
			default:
				break;
		}
		if (image_texture)
		{
			if (image_texture->field != image_field)
			{
				Material_image_texture_set_field(image_texture, image_field);
				material->compile_status = GRAPHICS_NOT_COMPILED;
				Graphical_material_changed(material);
			}
			return CMZN_OK;
		}
	}
	return CMZN_ERROR_ARGUMENT;
}

int set_shader_program_type_texture_mode(cmzn_material *material_to_be_modified,
	 int *type, int return_code)
{
	 int dimension;

	 ENTER(set_shader_program_type_texture_mode);
	 if (material_to_be_modified->image_texture.texture)
	 {
			Texture_get_dimension(material_to_be_modified->image_texture.texture, &dimension);
			switch (dimension)
			{
				 case 1:
				 {
						*type |= SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_1;
				 } break;
				 case 2:
				 {
						*type |= SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_2;
				 } break;
				 case 3:
				 {
						*type |= SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_1 |
							 SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_2;
				 } break;
				 default:
				 {
						display_message(ERROR_MESSAGE, "Colour texture dimension %d not supported.",
							 dimension);
						return_code = 0;
				 } break;
			}
			switch (Texture_get_number_of_components(material_to_be_modified->image_texture.texture))
			{
				 case 1:
				 {
						*type |= SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_1;
				 } break;
				 case 2:
				 {
						*type |= SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_2;
				 } break;
				 case 3:
				 {
						*type |= SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_1 |
							 SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_2;
				 } break;
				 case 4:
				 {
						/* Do nothing as zero in these bits indicates rgba */
				 } break;
				 default:
				 {
						display_message(ERROR_MESSAGE, "Colour texture output dimension not supported.");
						return_code = 0;
				 } break;
			}
			switch (Texture_get_combine_mode(material_to_be_modified->image_texture.texture))
			{
				 case TEXTURE_DECAL:
				 {
						*type |= SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_DECAL;
				 }
				 default:
				 {
						/* Do nothing as modulate is the default */
				 }
			}
	 }
	 LEAVE;

	 return return_code;
}

int set_shader_program_type_second_texture(cmzn_material *material_to_be_modified,
	 int *type, int return_code)
/******************************************************************************
LAST MODIFIED : 4 Dec 2007

DESCRIPTION : Set up the shader program second texture type
for using the vertex and fragment program. This and following
functions are orginally from the modify_graphical_materil.
==============================================================================*/
{
	 int dimension;

	 ENTER(set_shader_program_type_second_texture);
	 if (material_to_be_modified->second_image_texture.texture)
	 {
			Texture_get_dimension(material_to_be_modified->second_image_texture.texture, &dimension);
			switch (dimension)
			{
				 case 1:
				 {
						*type |= SHADER_PROGRAM_CLASS_SECOND_TEXTURE_1;
				 } break;
				 case 2:
				 {
						*type |= SHADER_PROGRAM_CLASS_SECOND_TEXTURE_2;
				 } break;
				 case 3:
				 {
						*type |= SHADER_PROGRAM_CLASS_SECOND_TEXTURE_1 |
							 SHADER_PROGRAM_CLASS_SECOND_TEXTURE_2;
				 } break;
				 default:
				 {
						display_message(ERROR_MESSAGE, "Second texture dimension %d not supported.",
							 dimension);
						return_code = 0;
				 } break;
			}
	 }
	 LEAVE;

	 return return_code;
}

int set_shader_program_type_bump_mapping(cmzn_material *material_to_be_modified,
	 int *type, int return_code)
/******************************************************************************
LAST MODIFIED : 4 Dec 2007

DESCRIPTION : Set up the shader program bump mapping type
for using the vertex and fragment program. This and following
functions are orginally from the modify_graphical_materil.
==============================================================================*/
{
	 ENTER(set_shader_program_type_bump_mapping);
	 if (material_to_be_modified->second_image_texture.texture)
	 {
			*type |= SHADER_PROGRAM_CLASS_SECOND_TEXTURE_BUMPMAP;
			material_to_be_modified->bump_mapping_flag = 1;
	 }
	 else
	 {
			display_message(ERROR_MESSAGE,
				 "Bump mapping requires specification of a second texture containing a normal map.");
			material_to_be_modified->bump_mapping_flag = 0;
			return_code = 0;
	 }
	 LEAVE;

	 return return_code;
}

int set_shader_program_type_spectrum(cmzn_material *material_to_be_modified,
	 int *type, int red_flag, int green_flag, int blue_flag,
	 int alpha_flag, int return_code)
/******************************************************************************
LAST MODIFIED : 4 Dec 2007

DESCRIPTION : Set up the shader program spectrum type
for using the vertex and fragment program. This and following
functions are orginally from the modify_graphical_materil.
==============================================================================*/
{
	 enum Spectrum_colour_components spectrum_colour_components;
	 int number_of_spectrum_components;

	 ENTER(set_shader_program_type_spectrum);
	 if (material_to_be_modified->spectrum)
	 {
			/* Cannot just rely on the COPY functions as when
				 first created this will be the actual object. */
			if (material_to_be_modified->module &&
				 (!material_to_be_modified->spectrum_manager_callback_id))
			{
				 material_to_be_modified->spectrum_manager_callback_id=
						MANAGER_REGISTER(cmzn_spectrum)(Graphical_material_Spectrum_change,
							 (void *)material_to_be_modified, material_to_be_modified->module->getSpectrumManager());
			}

			/* Specify the input colours */
			if (red_flag)
			{
				 *type |= SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_1;
			}
			if (green_flag)
			{
				 *type |= SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_2;
			}
			if (blue_flag)
			{
				 *type |= SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_3;
			}
			if (alpha_flag)
			{
				 *type |= SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_4;
			}

			number_of_spectrum_components =
				 Spectrum_get_number_of_data_components(material_to_be_modified->spectrum);
			if ((alpha_flag + blue_flag +
						green_flag + red_flag) ==
				 number_of_spectrum_components)
			{
				 spectrum_colour_components = Spectrum_get_colour_components(
						material_to_be_modified->spectrum);
				 if (spectrum_colour_components & SPECTRUM_COMPONENT_ALPHA)
				 {
						if (spectrum_colour_components == SPECTRUM_COMPONENT_ALPHA)
						{
							 /* Alpha only */
							 *type |= SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_ALPHA;
						}
						else
						{
							 /* Colour and alpha */
							 *type |= SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_COLOUR
									| SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_ALPHA;
						}
				 }
				 else
				 {
						/* Colour only */
						*type |= SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_COLOUR;
				 }
			}
			else if (1 == number_of_spectrum_components)
			{
				 /* Lookup each component specified in the 1D spectra only */
				 *type |= SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_1D_COMPONENT_LOOKUP;
			}
	 }
	 else
	 {
			if (material_to_be_modified->module &&
				 material_to_be_modified->spectrum_manager_callback_id)
			{
				 MANAGER_DEREGISTER(cmzn_spectrum)(
						material_to_be_modified->spectrum_manager_callback_id,
						material_to_be_modified->module->getSpectrumManager());
				 material_to_be_modified->spectrum_manager_callback_id=NULL;
			}
	 }
	 LEAVE;

	 return return_code;
}

int material_update_shader_program(cmzn_shadermodule_id shadermodule, cmzn_material *material_to_be_modified,
	 struct cmzn_materialmodule *materialmodule, enum cmzn_shaderprogram_type type, int return_code)
/******************************************************************************
LAST MODIFIED : 4 Dec 2007

DESCRIPTION : Check the shader program and renew it if necessary.
==============================================================================*/
{
	 if (!material_to_be_modified->program ||
			(cmzn_shaderprogram_get_type(material_to_be_modified->program) != type))
	 {
		if (material_to_be_modified->program)
		{
			 cmzn_shaderprogram_destroy(&material_to_be_modified->program);
		}
		material_to_be_modified->program = cmzn_shadermodule_find_shaderprogram_by_type(
			shadermodule, type);
		if (!material_to_be_modified->program)
		{
			material_to_be_modified->program = cmzn_shadermodule_create_shaderprogram(shadermodule);

			 if (material_to_be_modified->program)
			 {
				 cmzn_shaderprogram_set_type(material_to_be_modified->program, type);
			 }
			 else
			 {
					return_code = 0;
			 }
		}
	 }

	 return return_code;
}

int set_shader_program_type(cmzn_shadermodule_id shadermodule, cmzn_material *material_to_be_modified,
	 int bump_mapping_flag, int colour_lookup_red_flag, int colour_lookup_green_flag,
	 int colour_lookup_blue_flag,  int colour_lookup_alpha_flag,
	 int lit_volume_intensity_normal_texture_flag, int lit_volume_finite_difference_normal_flag,
	 int lit_volume_scale_alpha_flag, int return_code)
/******************************************************************************
LAST MODIFIED : 4 Dec 2007

DESCRIPTION : Set up the shader program type for using the vertex
and fragment program. This and following functions are orginally
from the modify_graphical_material.
NOTE: I use the pointer to the materialmodule from the material.
==============================================================================*/
{
	 int type;

	 ENTER(set_shader_program_type);

	 type = SHADER_PROGRAM_PER_PIXEL_LIGHTING;
	 material_to_be_modified->per_pixel_lighting_flag = 1;

	 return_code = set_shader_program_type_texture_mode(material_to_be_modified,
			&type, return_code);
	 return_code = set_shader_program_type_second_texture(material_to_be_modified,
			&type, return_code);
	 if (bump_mapping_flag)
	 {
			return_code = set_shader_program_type_bump_mapping(material_to_be_modified,
				 &type, return_code);
	 }
	 else
	 {
			material_to_be_modified->bump_mapping_flag = 0;
	 }

	 return_code = set_shader_program_type_spectrum(material_to_be_modified,
			&type, colour_lookup_red_flag, colour_lookup_green_flag, colour_lookup_blue_flag,
			colour_lookup_alpha_flag, return_code);

	 if (lit_volume_intensity_normal_texture_flag)
	 {
			type |= SHADER_PROGRAM_CLASS_LIT_VOLUME_INTENSITY_NORMAL_TEXTURE;
	 }
	 if (lit_volume_finite_difference_normal_flag)
	 {
			type |= SHADER_PROGRAM_CLASS_LIT_VOLUME_FINITE_DIFFERENCE_NORMAL;
	 }
	 if (lit_volume_scale_alpha_flag)
	 {
			type |= SHADER_PROGRAM_CLASS_LIT_VOLUME_SCALE_ALPHA;
	 }

	 material_to_be_modified->compile_status = GRAPHICS_NOT_COMPILED;

	 return_code = material_update_shader_program(shadermodule, material_to_be_modified,
			material_to_be_modified->module, (cmzn_shaderprogram_type)type, return_code);

	 return return_code;
}

int material_copy_bump_mapping_and_per_pixel_lighting_flag(cmzn_material *material,
	 cmzn_material *material_to_be_modified)
/******************************************************************************
LAST MODIFIED : 5 Dec 2007

DESCRIPTION : This function will set the bump mapping and per
pixel_lighting_flag of the material_to_be_modified to be the same as
the one in material, it is used for setting up the GUI.
==============================================================================*/
{
	 int return_code;
	 if (material && material_to_be_modified)
	 {
			material_to_be_modified->bump_mapping_flag =
				 material->bump_mapping_flag;
			material_to_be_modified->per_pixel_lighting_flag =
				 material->per_pixel_lighting_flag;
			return_code = 1;
	 }
	 else
	 {
			display_message(ERROR_MESSAGE,
				 "material_copy_bump_mapping_and_per_pixel_lighting_flag.  Missing shader_program");
			return_code = 0;
	 }

	 return return_code;
}

/**************************************************************************//**
 * Sets the material to use a #cmzn_shaderprogram with user specified strings
 * for the vertex_program and fragment_program.
 */
int Material_set_shader_program_strings(cmzn_shadermodule_id shadermodule,
	cmzn_material *material_to_be_modified,	char *vertex_program_string,
	char *fragment_program_string, char *geometry_program_string)
{
	if (shadermodule && material_to_be_modified)
	{
		cmzn_shaderprogram_id old_program;
		old_program = material_to_be_modified->program;
		char *vs = 0, *fs = 0, *gs = 0;
		if (old_program)
		{
	#if defined (OPENGL_API)
			if (!vertex_program_string)
			{
				vs = cmzn_shaderprogram_get_vertex_shader(old_program);
			}
			else
			{
				vs = duplicate_string(vertex_program_string);
			}
			if (!fragment_program_string)
			{
				fs = cmzn_shaderprogram_get_fragment_shader(old_program);
			}
			else
			{
				fs = duplicate_string(fragment_program_string);
			}
			if (!geometry_program_string)
			{
				gs = cmzn_shaderprogram_get_geometry_shader(old_program);
			}
			else
			{
				gs = duplicate_string(geometry_program_string);
			}
	#endif /* defined (OPENGL_API) */
		}
		else
		{
			if (vertex_program_string)
				vs = duplicate_string(vertex_program_string);
			if (fragment_program_string)
			{
				fs = duplicate_string(fragment_program_string);
			}
			if (geometry_program_string)
			{
				gs = duplicate_string(geometry_program_string);
			}
		}

		material_to_be_modified->program = cmzn_shadermodule_create_shaderprogram(shadermodule);
		if (material_to_be_modified->program)
		{
			if (vs)
			{
				cmzn_shaderprogram_set_vertex_shader(material_to_be_modified->program, vs);
				DEALLOCATE(vs);
			}
			if (fs)
			{
				cmzn_shaderprogram_set_fragment_shader(material_to_be_modified->program, fs);
				DEALLOCATE(fs);
			}
			if (gs)
			{
				cmzn_shaderprogram_set_geometry_shader(material_to_be_modified->program, gs);
				DEALLOCATE(gs);
			}
		}
		if (old_program)
		{
			cmzn_shaderprogram_destroy(&old_program);
		}
		if (material_to_be_modified->program)
		{
			material_to_be_modified->compile_status = GRAPHICS_NOT_COMPILED;
			return CMZN_OK;
		}
		return CMZN_ERROR_ARGUMENT;
	}
	return CMZN_ERROR_ARGUMENT;
}

int list_Graphical_material(cmzn_material *material,void *dummy)
/*******************************************************************************
LAST MODIFIED : 24 November 1999

DESCRIPTION :
Writes the properties of the <material> to the command window.
==============================================================================*/
{
	char line[80],*name;
	int return_code;

	ENTER(list_Graphical_material);
	USE_PARAMETER(dummy);
	/* check the arguments */
	if (material)
	{
		display_message(INFORMATION_MESSAGE,"material : ");
		display_message(INFORMATION_MESSAGE,material->name);
		display_message(INFORMATION_MESSAGE,"\n");
		sprintf(line,"  access count = %i\n",material->access_count);
		if (material->program)
		{
			enum cmzn_shaderprogram_type type = cmzn_shaderprogram_get_type(material->program);
			if (SHADER_PROGRAM_CLASS_GOURAUD_SHADING & type)
			{
				display_message(INFORMATION_MESSAGE,"  Standard Gouraud Shading (program)\n");
			}
			else if (SHADER_PROGRAM_CLASS_PER_PIXEL_LIGHTING & type)
			{
				display_message(INFORMATION_MESSAGE,"  Per Pixel Shading\n");
			}
			else if (SHADER_PROGRAM_CLASS_SECOND_TEXTURE_BUMPMAP & type)
			{
				display_message(INFORMATION_MESSAGE,"  Per Pixel Bump map Shading\n");
			}
		}
		else
		{
			display_message(INFORMATION_MESSAGE,"  Standard Gouraud Shading\n");
		}
		display_message(INFORMATION_MESSAGE,line);
		sprintf(line,"  diffuse  red = %.3g, green = %.3g, blue = %.3g\n",
			(material->diffuse).red,(material->diffuse).green,
			(material->diffuse).blue);
		display_message(INFORMATION_MESSAGE,line);
		sprintf(line,"  ambient  red = %.3g, green = %.3g, blue = %.3g\n",
			(material->ambient).red,(material->ambient).green,
			(material->ambient).blue);
		display_message(INFORMATION_MESSAGE,line);
		sprintf(line,"  alpha = %.3g\n",material->alpha);
		display_message(INFORMATION_MESSAGE,line);
		sprintf(line,"  emission  red = %.3g, green = %.3g, blue = %.3g\n",
			(material->emission).red,(material->emission).green,
			(material->emission).blue);
		display_message(INFORMATION_MESSAGE,line);
		sprintf(line,"  specular  red = %.3g, green = %.3g, blue = %.3g\n",
			(material->specular).red,(material->specular).green,
			(material->specular).blue);
		display_message(INFORMATION_MESSAGE,line);
		sprintf(line,"  shininess = %.3g\n",material->shininess);
		display_message(INFORMATION_MESSAGE,line);
		if (material->image_texture.texture&&GET_NAME(Texture)(material->image_texture.texture,&name))
		{
			display_message(INFORMATION_MESSAGE, "  texture : ");
			display_message(INFORMATION_MESSAGE, name);
			display_message(INFORMATION_MESSAGE, "\n");
			DEALLOCATE(name);
		}
		if (material->second_image_texture.texture&&GET_NAME(Texture)(material->second_image_texture.texture,&name))
		{
			display_message(INFORMATION_MESSAGE, "  second texture : ");
			display_message(INFORMATION_MESSAGE, name);
			display_message(INFORMATION_MESSAGE, "\n");
			DEALLOCATE(name);
		}
		if (material->third_image_texture.texture&&GET_NAME(Texture)(material->third_image_texture.texture,&name))
		{
			display_message(INFORMATION_MESSAGE, "  third texture : ");
			display_message(INFORMATION_MESSAGE, name);
			display_message(INFORMATION_MESSAGE, "\n");
			DEALLOCATE(name);
		}
		if (material->fourth_image_texture.texture&&GET_NAME(Texture)(material->fourth_image_texture.texture,&name))
		{
			display_message(INFORMATION_MESSAGE, "  fourth texture : ");
			display_message(INFORMATION_MESSAGE, name);
			display_message(INFORMATION_MESSAGE, "\n");
			DEALLOCATE(name);
		}
		if (material->spectrum&&GET_NAME(cmzn_spectrum)(material->spectrum,&name))
		{
			display_message(INFORMATION_MESSAGE, "  colour lookup spectrum : ");
			display_message(INFORMATION_MESSAGE, name);
			display_message(INFORMATION_MESSAGE, "\n");
			DEALLOCATE(name);
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Graphical_material.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* list_Graphical_material */

int list_Graphical_material_commands(cmzn_material *material,
	 void *command_prefix_void)
/*******************************************************************************
LAST MODIFIED : 15 August 2007

DESCRIPTION :
Writes on the command window the command needed to recreate the <material>.
The command is started with the string pointed to by <command_prefix>.
==============================================================================*/
{
	char *command_prefix,line[100],*name;
	int return_code;

	ENTER(list_Graphical_material_commands);
	if (material&&(command_prefix=(char *)command_prefix_void))
	{
		display_message(INFORMATION_MESSAGE,command_prefix);
		name=duplicate_string(material->name);
		if (name)
		{
			/* put quotes around name if it contains special characters */
			make_valid_token(&name);
			display_message(INFORMATION_MESSAGE,name);
			DEALLOCATE(name);
		}
		if (material->program)
		{
			enum cmzn_shaderprogram_type type = cmzn_shaderprogram_get_type(material->program);
			if (SHADER_PROGRAM_CLASS_GOURAUD_SHADING & type)
			{
				display_message(INFORMATION_MESSAGE," normal_mode");
			}
			else if (SHADER_PROGRAM_CLASS_PER_PIXEL_LIGHTING & type)
			{
				display_message(INFORMATION_MESSAGE," per_pixel_mode");
			}
			else if (SHADER_PROGRAM_CLASS_SECOND_TEXTURE_BUMPMAP & type)
			{
				display_message(INFORMATION_MESSAGE," per_pixel_mode bump_mapping");
			}
		}
		else
		{
			display_message(INFORMATION_MESSAGE," normal_mode");
		}
		sprintf(line," ambient %g %g %g",
			(material->ambient).red,(material->ambient).green,
			(material->ambient).blue);
		display_message(INFORMATION_MESSAGE,line);
		sprintf(line," diffuse %g %g %g",
			(material->diffuse).red,(material->diffuse).green,
			(material->diffuse).blue);
		display_message(INFORMATION_MESSAGE,line);
		sprintf(line," emission %g %g %g",
			(material->emission).red,(material->emission).green,
			(material->emission).blue);
		display_message(INFORMATION_MESSAGE,line);
		sprintf(line," specular %g %g %g",
			(material->specular).red,(material->specular).green,
			(material->specular).blue);
		display_message(INFORMATION_MESSAGE,line);
		sprintf(line," alpha %g",material->alpha);
		display_message(INFORMATION_MESSAGE,line);
		sprintf(line," shininess %g",material->shininess);
		display_message(INFORMATION_MESSAGE,line);
		if (material->image_texture.texture&&GET_NAME(Texture)(material->image_texture.texture,&name))
		{
			/* put quotes around name if it contains special characters */
			make_valid_token(&name);
			display_message(INFORMATION_MESSAGE," texture %s",name);
			DEALLOCATE(name);
		}
		if (material->second_image_texture.texture&&GET_NAME(Texture)(material->second_image_texture.texture,&name))
		{
			/* put quotes around name if it contains special characters */
			make_valid_token(&name);
			display_message(INFORMATION_MESSAGE," secondary_texture %s",name);
			DEALLOCATE(name);
		}
		if (material->third_image_texture.texture&&GET_NAME(Texture)(material->third_image_texture.texture,&name))
		{
			/* put quotes around name if it contains special characters */
			make_valid_token(&name);
			display_message(INFORMATION_MESSAGE," third_texture %s",name);
			DEALLOCATE(name);
		}
		if (material->fourth_image_texture.texture&&GET_NAME(Texture)(material->fourth_image_texture.texture,&name))
		{
			/* put quotes around name if it contains special characters */
			make_valid_token(&name);
			display_message(INFORMATION_MESSAGE," fourth_texture %s",name);
			DEALLOCATE(name);
		}
		if (material->spectrum&&GET_NAME(cmzn_spectrum)(material->spectrum,&name))
		{
			/* put quotes around name if it contains special characters */
			make_valid_token(&name);
			display_message(INFORMATION_MESSAGE," colour_lookup_spectrum %s",name);
			DEALLOCATE(name);
		}
		display_message(INFORMATION_MESSAGE,";\n");
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Graphical_material_commands.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* list_Graphical_material_commands */

int write_Graphical_material_commands_to_comfile(cmzn_material *material,
	 void *command_prefix_void)
/*******************************************************************************
LAST MODIFIED : 15 August 2007

DESCRIPTION :
Writes on the command window the command needed to recreate the <material>.
The command is started with the string pointed to by <command_prefix>.
==============================================================================*/
{
	char *command_prefix,line[100],*name;
	int return_code;

	ENTER(write_Graphical_material_commands_to_comfile);
	if (material&&(command_prefix=(char *)command_prefix_void))
	{
		write_message_to_file(INFORMATION_MESSAGE,command_prefix);
		name=duplicate_string(material->name);
		if (name)
		{
			/* put quotes around name if it contains special characters */
			make_valid_token(&name);
			write_message_to_file(INFORMATION_MESSAGE,name);
			DEALLOCATE(name);
		}
		if (material->program)
		{
			enum cmzn_shaderprogram_type type = cmzn_shaderprogram_get_type(material->program);
			if (SHADER_PROGRAM_CLASS_GOURAUD_SHADING & type)
			{
				 write_message_to_file(INFORMATION_MESSAGE," normal_mode");
			}
			else if (SHADER_PROGRAM_CLASS_PER_PIXEL_LIGHTING & type)
			{
				write_message_to_file(INFORMATION_MESSAGE," per_pixel_mode");
			}
			else if (SHADER_PROGRAM_CLASS_SECOND_TEXTURE_BUMPMAP & type)
			{
				write_message_to_file(INFORMATION_MESSAGE," per_pixel_mode bump_mapping");
			}
		}
		else
		{
			 write_message_to_file(INFORMATION_MESSAGE," normal_mode");
		}
		sprintf(line," ambient %g %g %g",
			(material->ambient).red,(material->ambient).green,
			(material->ambient).blue);
		write_message_to_file(INFORMATION_MESSAGE,line);
		sprintf(line," diffuse %g %g %g",
			(material->diffuse).red,(material->diffuse).green,
			(material->diffuse).blue);
		write_message_to_file(INFORMATION_MESSAGE,line);
		sprintf(line," emission %g %g %g",
			(material->emission).red,(material->emission).green,
			(material->emission).blue);
		write_message_to_file(INFORMATION_MESSAGE,line);
		sprintf(line," specular %g %g %g",
			(material->specular).red,(material->specular).green,
			(material->specular).blue);
		write_message_to_file	(INFORMATION_MESSAGE,line);
		sprintf(line," alpha %g",material->alpha);
		write_message_to_file	(INFORMATION_MESSAGE,line);
		sprintf(line," shininess %g",material->shininess);
		write_message_to_file(INFORMATION_MESSAGE,line);
		if (material->image_texture.texture&&GET_NAME(Texture)(material->image_texture.texture,&name))
		{
			/* put quotes around name if it contains special characters */
			make_valid_token(&name);
			write_message_to_file(INFORMATION_MESSAGE," texture %s",name);
			DEALLOCATE(name);
		}
		if (material->second_image_texture.texture&&GET_NAME(Texture)(material->second_image_texture.texture,&name))
		{
			/* put quotes around name if it contains special characters */
			make_valid_token(&name);
			write_message_to_file(INFORMATION_MESSAGE," secondary_texture %s",name);
			DEALLOCATE(name);
		}
		if (material->third_image_texture.texture&&GET_NAME(Texture)(material->third_image_texture.texture,&name))
		{
			/* put quotes around name if it contains special characters */
			make_valid_token(&name);
			write_message_to_file(INFORMATION_MESSAGE," third_texture %s",name);
			DEALLOCATE(name);
		}
		if (material->fourth_image_texture.texture&&GET_NAME(Texture)(material->fourth_image_texture.texture,&name))
		{
			/* put quotes around name if it contains special characters */
			make_valid_token(&name);
			write_message_to_file(INFORMATION_MESSAGE," fourth_texture %s",name);
			DEALLOCATE(name);
		}
		if (material->spectrum&&GET_NAME(cmzn_spectrum)(material->spectrum,&name))
		{
			/* put quotes around name if it contains special characters */
			make_valid_token(&name);
			write_message_to_file(INFORMATION_MESSAGE," colour_lookup_spectrum %s",name);
			DEALLOCATE(name);
		}
		write_message_to_file(INFORMATION_MESSAGE,";\n");
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Graphical_material_commands.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* write_Graphical_material_commands_to_comfile */


/* int list_Graphical_material_commands(cmzn_material *material, */
/* 	void *command_prefix_void) */
/* ****************************************************************************** */
/* LAST MODIFIED : 15 August 2007 */

/* DESCRIPTION : */
/* List on the command window the command needed to recreate the <material>. */
/* The command is started with the string pointed to by <command_prefix>. */
/* ==============================================================================*/
/* { */
/* 	 int return_code; */

/* 	 ENTER(list_Graphical_material_commands); */
/* 	 Process_list_command_class *list_message = */
/* 			new Process_list_command_class(); */
/* 	 return_code = process_list_or_write_Graphical_material_commands( */
/* 			material,command_prefix_void, list_message); */
/* 	 LEAVE; */

/* 	return (return_code); */
/* }  write_Graphical_material_commands_to_comfile */

/* int write_Graphical_material_commands_to_comfile(cmzn_material *material, */
/* 	void *command_prefix_void) */
/* ****************************************************************************** */
/* LAST MODIFIED : 15 August 2007 */

/* DESCRIPTION : */
/* Writes on the command window the command needed to recreate the <material>. */
/* The command is started with the string pointed to by <command_prefix>. */
/* ==============================================================================*/
/* { */
/* 	 int return_code; */

/* 	 ENTER(write_Graphical_material_commands_to_comfile); */
/* 	 Process_write_command_class *write_message = */
/* 			new Process_write_command_class(); */
/* 	 return_code = process_list_or_write_Graphical_material_commands( */
/* 			material,command_prefix_void, write_message); */
/* 	 LEAVE; */

/* 	 return (return_code); */
/* } write_Graphical_material_commands_to_comfile */

int file_read_Graphical_material_name(struct IO_stream *stream,
	cmzn_material **material_address,
	struct MANAGER(cmzn_material) *graphical_material_manager)
/*******************************************************************************
LAST MODIFIED : 6 December 2004

DESCRIPTION :
Reads a material name from a <file>.  Searchs the list of all materials for one
with the specified name.  If one is not found a new one is created with the
specified name and the default properties.
==============================================================================*/
{
	char *material_name;
	int return_code;
	cmzn_material *material;

	ENTER(file_read_Graphical_material_name);
	/* check the arguments */
	if (stream&&material_address)
	{
		if (IO_stream_read_string(stream,"s",&material_name))
		{
			/*???DB.  Should this read function be in another module ? */
			/* either find an existing material of that name, use no material if the
				 name is "none", or make a material of the given name */
			material=FIND_BY_IDENTIFIER_IN_MANAGER(cmzn_material,name)(
				material_name,graphical_material_manager);
			if (material || fuzzy_string_compare_same_length(material_name,"NONE"))
			{
				*material_address=material;
				return_code=1;
			}
			else
			{
				material=cmzn_material_create_private();
				cmzn_material_set_name(material, material_name);
				if (material)
				{
					cmzn_material_set_managed(material, true);
					if (ADD_OBJECT_TO_MANAGER(cmzn_material)(material,
						graphical_material_manager))
					{
						*material_address=material;
						return_code=1;
					}
					else
					{
						return_code=0;
					}
					cmzn_material_destroy(&material);
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"file_read_Graphical_material_name.  Could not create material");
					return_code=0;
				}
			}
			DEALLOCATE(material_name);
		}
		else
		{
			display_message(ERROR_MESSAGE,
			"file_read_Graphical_material_name.  Error reading material name strin");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"file_read_Graphical_material_name.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* file_read_Graphical_material_name */

#if defined (OPENGL_API)
int Material_compile_members_opengl(cmzn_material *material,
	Render_graphics_opengl *renderer)
{
	int return_code;

	ENTER(Material_compile_members_opengl);
	if (material)
	{
		material->executed_as_order_independent = 0;
		cmzn_shaderprogram_destroy(&material->order_program);
		return_code = 1;
		if (GRAPHICS_COMPILED != material->compile_status)
		{
			/* must compile texture before opening material display list */
			if (material->image_texture.texture)
			{
				renderer->Texture_compile(material->image_texture.texture);
			}
#if defined (GL_VERSION_1_3)
			if (Graphics_library_check_extension(GL_VERSION_1_3))
			{
				/* It probably isn't necessary to activate the texture
					unit only to compile the texture, but seems a discipline
					that may help avoid graphics driver problems? */
				if (material->second_image_texture.texture)
				{
					glActiveTexture(GL_TEXTURE1);
					renderer->allow_texture_tiling = 0;
					renderer->Texture_compile(material->second_image_texture.texture);
					glActiveTexture(GL_TEXTURE0);
				}
				if (material->third_image_texture.texture)
				{
					glActiveTexture(GL_TEXTURE2);
					renderer->allow_texture_tiling = 0;
					renderer->Texture_compile(material->third_image_texture.texture);
					glActiveTexture(GL_TEXTURE0);
				}
				if (material->fourth_image_texture.texture)
				{
					glActiveTexture(GL_TEXTURE3);
					renderer->allow_texture_tiling = 0;
					renderer->Texture_compile(material->fourth_image_texture.texture);
					glActiveTexture(GL_TEXTURE0);
				}
				if (material->spectrum)
				{
					glActiveTexture(GL_TEXTURE1);
					Spectrum_compile_colour_lookup(material->spectrum,
						renderer);
					glActiveTexture(GL_TEXTURE0);
				}
			}
#endif /* defined (GL_VERSION_1_3) */

			if (material->program)
			{
				cmzn_shaderprogram_compile(material->program, renderer);
			}
			material->compile_status = GRAPHICS_COMPILED;
		}
		else
		{
			if (renderer->allow_texture_tiling && material->image_texture.texture)
			{
				renderer->Texture_compile(material->image_texture.texture);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Material_compile_members_opengl.  Missing material");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Material_compile_members_opengl */
#endif /* defined (OPENGL_API) */

#if defined (OPENGL_API)
int Material_compile_opengl_display_list(cmzn_material *material,
	Callback_base< cmzn_material* > *execute_function,
	Render_graphics_opengl *renderer)
{
	int return_code;

	ENTER(Material_compile_opengl_display_list);
	USE_PARAMETER(renderer);
	if (material)
	{
		return_code = 1;
		if (GRAPHICS_NOT_COMPILED == material->compile_status)
		{
			if (material->display_list || (material->display_list = glGenLists(1)))
			{
				glNewList(material->display_list,GL_COMPILE);
				return_code = (*execute_function)(material);
				glEndList();
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"Material_compile_opengl_display_list.  Could not generate display list");
				return_code = 0;
			}
		}
		if (return_code)
		{
			material->compile_status = GRAPHICS_COMPILED;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Material_compile_opengl_display_list.  Missing material");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Material_compile_opengl_display_list */
#endif /* defined (OPENGL_API) */

#if defined (OPENGL_API)
int compile_Graphical_material_for_order_independent_transparency(
	cmzn_material *material,
	void *material_order_independent_data_void)
/*******************************************************************************
LAST MODIFIED : 2 May 2005

DESCRIPTION :
Recompile each of the <materials> which have already been compiled so that they
will work with order_independent_transparency.
==============================================================================*/
{
	int return_code;
	int modified_type;
	int dimension;
	struct Material_order_independent_transparency *data;
	struct cmzn_materialmodule *materialmodule;
	cmzn_shaderprogram_id unmodified_program;

	ENTER(compile_Graphical_material_for_order_independent_transparency);

	if (material && (data = (struct Material_order_independent_transparency *)
			material_order_independent_data_void))
	{
		return_code = 1;
		materialmodule = material->module;
		/* Only do the materials that have been compiled already as the scene
			is compiled so presumably uncompiled materials are not used. */
		if ((GRAPHICS_COMPILED == material->compile_status) &&
			(!data->renderer->use_display_list || material->display_list))
		{
			unmodified_program = material->program;
			if (material->program)
			{
				modified_type = cmzn_shaderprogram_get_type(material->program);
			}
			else
			{
				modified_type = SHADER_PROGRAM_CLASS_GOURAUD_SHADING;
				if (material->image_texture.texture)
				{
					/* Texture should have already been compiled when the
						scene was compiled before rendering. */
					if (material->image_texture.texture)
					{
						data->renderer->Texture_compile(material->image_texture.texture);
					}
					Texture_get_dimension(material->image_texture.texture, &dimension);
					switch (dimension)
					{
						case 1:
						{
							modified_type |= SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_1;
						} break;
						case 2:
						{
							modified_type |= SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_2;
						} break;
						case 3:
						{
							modified_type |= SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_1 |
								SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_2;
						} break;
						default:
						{
							display_message(ERROR_MESSAGE, "Colour texture dimension %d not supported.",
								dimension);
							return_code = 0;
						} break;
					}
					switch (Texture_get_number_of_components(material->image_texture.texture))
					{
						case 1:
						{
							modified_type |= SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_1;
						} break;
						case 2:
						{
							modified_type |= SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_2;
						} break;
						case 3:
						{
							modified_type |= SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_1 |
								SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_2;
						} break;
						case 4:
						{
							/* Do nothing as zero in these bits indicates rgba */
						} break;
						default:
						{
							display_message(ERROR_MESSAGE, "Colour texture output dimension not supported.");
							return_code = 0;
						} break;
					}
					switch (Texture_get_combine_mode(material->image_texture.texture))
					{
						case TEXTURE_DECAL:
						{
							modified_type |= SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_DECAL;
						}
						default:
						{
							/* Do nothing as modulate is the default */
						}
					}
				}
			}

			if (data->layer == 1)
			{
				/* The first layer does not peel */
				modified_type |= SHADER_PROGRAM_CLASS_ORDER_INDEPENDENT_FIRST_LAYER;
			}
			else if (data->layer > 1)
			{
				/* The rest of the layers should peel */
				modified_type |= SHADER_PROGRAM_CLASS_ORDER_INDEPENDENT_PEEL_LAYER;
			}
			/*
			else
			{
				  Reset the material to its original state.  Could
					try to avoid this compile if we are about to render
					with order_independent_transparency again but need more
					compilation states then.
			} */
			cmzn_shaderprogram_id orderProgram = 0;

			if (modified_type != SHADER_PROGRAM_CLASS_GOURAUD_SHADING)
			{
				orderProgram = cmzn_shaderprogram_create_private();
				material->program = orderProgram;
				cmzn_shaderprogram_set_type(material->program, (cmzn_shaderprogram_type)modified_type);
				cmzn_shaderprogram_compile(material->program, data->renderer);
			}

			if (data->renderer->use_display_list)
			{
				glNewList(material->display_list,GL_COMPILE);
				if (material->program &&
						(cmzn_shaderprogram_get_shader_type(material->program) == SHADER_PROGRAM_SHADER_ARB))
				{
					if (material->image_texture.texture)
					{
						Texture_execute_vertex_program_environment(material->image_texture.texture,
							0);
					}
				}
				direct_render_Graphical_material(material,data->renderer);
				if (material->program &&
						(cmzn_shaderprogram_get_shader_type(material->program) == SHADER_PROGRAM_SHADER_GLSL))
				{
					GLint loc1;
					if (data && data->renderer)
					{
						GLuint currentProgram = (GLuint)cmzn_shaderprogram_get_glslprogram(material->program);
						if (glIsProgram(currentProgram))
						{
							loc1 = glGetUniformLocation(currentProgram,"texturesize");
							if (loc1>-1)
							{
								glUniform4f(loc1, static_cast<GLfloat>(data->renderer->viewport_width),
										static_cast<GLfloat>(data->renderer->viewport_height), 1.0, 1.0);
							}
							loc1 = glGetUniformLocation(currentProgram,"samplertex");
							if (loc1 != (GLint)-1)
							{
								glUniform1i(loc1, 3);
							}
						}
					}
				}
				glEndList();
			}
			else
			{
				material->executed_as_order_independent = 1;
				REACCESS(cmzn_shaderprogram)(&material->order_program, material->program);
			}
			material->program = unmodified_program;
			if (orderProgram)
				cmzn_shaderprogram_destroy(&orderProgram);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"compile_Graphical_material_for_order_independent_transparency.  Missing material");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* compile_Graphical_material_for_order_independent_transparency */
#endif /* defined (OPENGL_API) */

#if defined (OPENGL_API)
int Material_render_opengl(cmzn_material *material,
	Render_graphics_opengl *renderer)
/*******************************************************************************
LAST MODIFIED : 19 November 2007

DESCRIPTION :
Rebuilds the display_list for <material> if it is not current. If <material>
does not have a display list, first attempts to give it one. The display list
created here may be called using execute_Graphical_material, below.
If <texture_tiling> is not NULL then if the material uses a primary texture
and this texture is larger than can be compiled into a single texture on
the current graphics hardware, then it can be tiled into several textures
and information about the tile boundaries is returned in Texture_tiling
structure and should be used to compile any graphics objects.
???RC Graphical_materials must be compiled before they are executed since openGL
cannot start writing to a display list when one is currently being written to.
???RC The behaviour of materials is set up to take advantage of pre-computed
display lists. To switch to direct rendering make this routine do nothing and
execute_Graphical_material should just call direct_render_Graphical_material.
==============================================================================*/
{
	int return_code;

	ENTER(Material_render_opengl);
	if (material)
	{
		if (material->executed_as_order_independent && material->order_program)
		{
			if (material->order_program)
			{
				if (material->image_texture.texture)
				{
					if (cmzn_shaderprogram_get_shader_type(material->order_program) == SHADER_PROGRAM_SHADER_GLSL)
					{
						GLuint currentProgram = (GLuint)cmzn_shaderprogram_get_glslprogram(material->order_program);
						Texture_execute_vertex_program_environment(material->image_texture.texture,
							currentProgram);
						GLint loc1 = glGetUniformLocation(currentProgram,"texture0");
						if (loc1 != (GLint)-1)
							 glUniform1i(loc1, 0);
					}
					else
					{
						Texture_execute_vertex_program_environment(material->image_texture.texture,
							0);
					}
				}
			}
			cmzn_shaderprogram_id temp_program = material->program;
			if (material->order_program)
				material->program = material->order_program;
			return_code = direct_render_Graphical_material(material, renderer);
			material->program = temp_program;
			if (material->order_program)
			{
				if (cmzn_shaderprogram_get_shader_type(material->order_program) == SHADER_PROGRAM_SHADER_GLSL)
				{
					GLuint currentProgram = (GLuint)cmzn_shaderprogram_get_glslprogram(material->order_program);
					GLint loc1=-1;
					loc1 = glGetUniformLocation(currentProgram,"texturesize");
					if (loc1>-1)
					{
						glUniform4f(loc1, static_cast<GLfloat>(renderer->viewport_width),
								static_cast<GLfloat>(renderer->viewport_height), 1.0, 1.0);
					}
					loc1 = glGetUniformLocation(currentProgram,"samplertex");
					if (loc1 != (GLint)-1)
					{

						glUniform1i(loc1, 3);
					}
				}
			}
		}
		else
		{
			if (material->program)
			{
				/* Load up the texture scaling into the vertex program
				environment and the texel size into the fragment
				program environment. */
				if (material->image_texture.texture)
				{
					Texture_execute_vertex_program_environment(material->image_texture.texture,
						0);
				}
				else if (material->second_image_texture.texture)
				{
					Texture_execute_vertex_program_environment(material->second_image_texture.texture,
						0);
				}
				else if (material->third_image_texture.texture)
				{
					Texture_execute_vertex_program_environment(material->third_image_texture.texture,
						0);
				}
				else if (material->fourth_image_texture.texture)
				{
					Texture_execute_vertex_program_environment(material->fourth_image_texture.texture,
						0);
				}
#if defined GL_ARB_fragment_program || defined GL_VERSION_2_0
				enum cmzn_shaderprogram_shader_type shader_type =
					cmzn_shaderprogram_get_shader_type(material->program);
				if (material->spectrum && (shader_type != SHADER_PROGRAM_SHADER_NONE))
				{
					int i, lookup_dimensions, *lookup_sizes;
					GLfloat values[4];
					GLuint currentProgram = (GLuint)cmzn_shaderprogram_get_glslprogram(material->program);
					Spectrum_get_colour_lookup_sizes(material->spectrum,
						&lookup_dimensions, &lookup_sizes);
					/* Set the offsets = 0.5 / size */
					for (i = 0 ; i < lookup_dimensions ; i++)
					{
						values[i] = 0.5 / ((double)lookup_sizes[i]);
					}
					for (; i < 4 ; i++)
					{
						values[i] = 0.0;
					}
					if (shader_type == SHADER_PROGRAM_SHADER_ARB)
					{
						glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 1,
							values[0], values[1], values[2], values[3]);
					}
					else
					{
						GLint loc1=-1;
						if (glIsProgram(currentProgram))
						{
							loc1 = glGetUniformLocation(currentProgram,"lookup_offsets");
							if (loc1 != (GLint)-1)
							{
								glUniform4f(loc1, values[0], values[1],
									values[2], values[3]);
							}
						}
					}
					/* Set the scales = (size - 1) / (size) */
					for (i = 0 ; i < lookup_dimensions ; i++)
					{
						values[i] = ((double)(lookup_sizes[i] - 1)) / ((double)lookup_sizes[i]);
					}
					for (; i < 4 ; i++)
					{
						values[i] = 1.0;
					}
					if (shader_type == SHADER_PROGRAM_SHADER_ARB)
					{
						glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 2,
							values[0], values[1], values[2], values[3]);
					}
					else
					{
						GLint loc2=-1;
						if (glIsProgram(currentProgram))
						{
							loc2 = glGetUniformLocation(currentProgram,"lookup_scales");
							if (loc2 != (GLint)-1)
							{
								glUniform4f(loc2, values[0], values[1],
									values[2], values[3]);
							}
						}
					}
					DEALLOCATE(lookup_sizes);
				}
			}
			return_code = direct_render_Graphical_material(material, renderer);
		}
#endif /* defined GL_ARB_fragment_program */
	}
	else
	{
		return_code = renderer->Texture_execute((struct Texture *)NULL);
	}
	LEAVE;

	return (return_code);
} /* Material_render_opengl */
#endif /* defined (OPENGL_API) */

#if defined (OPENGL_API)
int Material_execute_opengl_display_list(cmzn_material *material,
	Render_graphics_opengl *renderer)
/*******************************************************************************
LAST MODIFIED : 13 March 2002

DESCRIPTION :
Activates <material> by calling its display list. If the display list is not
current, an error is reported.
Passing a NULL material will deactivate any textures or material parameters
that get set up with materials.
???RC The behaviour of materials is set up to take advantage of pre-computed
display lists. To switch to direct rendering this routine should just call
direct_render_Graphical_material.
==============================================================================*/
{
	int return_code;

	ENTER(execute_Graphical_material);
	if (material)
	{
		if (GRAPHICS_COMPILED == material->compile_status)
		{
			glCallList(material->display_list);
			return_code = 1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"execute_Graphical_material.  Display list not current");
			return_code = 0;
		}
	}
	else
	{
#if defined (GL_VERSION_2_0)
		if (Graphics_library_check_extension(GL_shading_language))
		{
			glUseProgram(0);
			glDisable(GL_VERTEX_PROGRAM_TWO_SIDE);
		}
#endif
#if defined GL_ARB_vertex_program && defined GL_ARB_fragment_program
		if (Graphics_library_check_extension(GL_ARB_vertex_program) &&
			Graphics_library_check_extension(GL_ARB_fragment_program))
		{
			glDisable(GL_VERTEX_PROGRAM_ARB);
			glDisable(GL_FRAGMENT_PROGRAM_ARB);
			glDisable(GL_VERTEX_PROGRAM_TWO_SIDE_ARB);
		}
#endif /* defined GL_ARB_vertex_program && defined GL_ARB_fragment_program */
		return_code = renderer->Texture_execute((struct Texture *)NULL);
	}
	LEAVE;

	return (return_code);
} /* execute_Graphical_material */
#endif /* defined (OPENGL_API) */

int cmzn_material_shaderprogram_changed(cmzn_material *material, void *message_void)
{
	struct MANAGER_MESSAGE(cmzn_shaderprogram) *message =
		(struct MANAGER_MESSAGE(cmzn_shaderprogram) *)message_void;
	int return_code = 0;

	if (material && message)
	{
		int change = MANAGER_MESSAGE_GET_OBJECT_CHANGE(cmzn_shaderprogram)(message, material->program);
		if (change & MANAGER_CHANGE_RESULT(cmzn_shaderprogram))
		{
			material->compile_status = GRAPHICS_NOT_COMPILED;
			Graphical_material_changed(material);
		}
		return_code = 1;
	}

	return return_code;
}

int cmzn_material_shaderuniforms_changed(cmzn_material *material, void *message_void)
{
	struct MANAGER_MESSAGE(cmzn_shaderuniforms) *message =
		(struct MANAGER_MESSAGE(cmzn_shaderuniforms) *)message_void;
	int return_code = 0;

	if (material && message)
	{
		int change = MANAGER_MESSAGE_GET_OBJECT_CHANGE(cmzn_shaderuniforms)(message, material->uniforms);
		if (change & MANAGER_CHANGE_RESULT(cmzn_shaderuniforms))
		{
			material->compile_status = GRAPHICS_NOT_COMPILED;
			Graphical_material_changed(material);
		}
		return_code = 1;
	}

	return return_code;
}

int material_deaccess_shader_program(cmzn_material *material_to_be_modified)
/******************************************************************************
LAST MODIFIED : 4 Dec 2007

DESCRIPTION : This function is to allow the material editor to
deaccess the shader program from the material.
==============================================================================*/
{
	 int return_code;

	 if (material_to_be_modified && material_to_be_modified->program)
	 {
			cmzn_shaderprogram_destroy(&material_to_be_modified->program);
			material_to_be_modified->compile_status = GRAPHICS_NOT_COMPILED;
			material_to_be_modified->per_pixel_lighting_flag = 0;
			material_to_be_modified->bump_mapping_flag = 0;
			return_code = 1;
	 }
	 else
	 {
			return_code = 0;
	 }

	 return return_code;
}

int cmzn_material_set_texture(
	cmzn_material *material, Texture *texture)
{
	int return_code = 0;

	ENTER(cmzn_material_set_texture);
	if (material)
	{
		REACCESS(Texture)(&(material->image_texture.texture), texture);
		if (material->manager)
		{
			Graphical_material_changed(material);
			return_code = 1;
		}
	}
	LEAVE;

	return return_code;
}

cmzn_material_id cmzn_material_access(cmzn_material_id material)
{
	if (material)
		material->access();
	return material;
}

int cmzn_material_destroy(cmzn_material_id *material_address)
{
	if (material_address)
	{
		cmzn_material::deaccess(material_address);
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

bool cmzn_material_is_managed(cmzn_material_id material)
{
	if (material)
	{
		return material->isManagedFlag;
	}
	return 0;
}

int cmzn_material_set_managed(cmzn_material_id material, bool value)
{
	if (material)
	{
		if (value != material->isManagedFlag)
		{
			material->isManagedFlag = value;
			MANAGED_OBJECT_CHANGE(cmzn_material)(material, MANAGER_CHANGE_DEFINITION(cmzn_material));
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_material_set_name(cmzn_material *material, const char *name)
{
	if (material)
		return material->setName(name);
	return CMZN_ERROR_ARGUMENT;
}

char *cmzn_material_get_name(cmzn_material_id material)
{
	char *name = NULL;
	if (material)
	{
		name = duplicate_string(Graphical_material_name(material));
	}

	return name;
}

double cmzn_material_get_attribute_real(cmzn_material_id material,
	enum cmzn_material_attribute attribute)
{
	ZnReal value = 0.0;
	if (material)
	{
		switch (attribute)
		{
			case CMZN_MATERIAL_ATTRIBUTE_ALPHA:
			{
				Graphical_material_get_alpha(material, &value);
			} break;
			case CMZN_MATERIAL_ATTRIBUTE_SHININESS:
			{
				Graphical_material_get_shininess(material, &value);
			}	break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"cmzn_material_get_attribute_real.  Invalid attribute");
			} break;
		}
	}
	return ((double)value);
}

int cmzn_material_set_attribute_real(cmzn_material_id material,
	enum cmzn_material_attribute attribute, double value)
{
	int return_code = 0;
	if (material)
	{
		return_code = 1;
		switch (attribute)
		{
			case CMZN_MATERIAL_ATTRIBUTE_ALPHA:
			{
				return_code = Graphical_material_set_alpha(material, value);
			} break;
			case CMZN_MATERIAL_ATTRIBUTE_SHININESS:
			{
				return_code = Graphical_material_set_shininess(material, value);
			}	break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"cmzn_material_set_attribute_real.  Invalid attribute");
				return_code = 0;
			} break;
		}
	}
	return return_code;
}

int cmzn_material_get_attribute_real3(cmzn_material_id material,
	enum cmzn_material_attribute attribute, double *values)
{
	struct Colour colour;
	int return_code = 0;
	colour.red = 0.0;
	colour.green = 0.0;
	colour.blue = 0.0;
	if (material)
	{
		return_code = 1;
		switch (attribute)
		{
			case CMZN_MATERIAL_ATTRIBUTE_AMBIENT:
			{
				return_code = Graphical_material_get_ambient(material, &colour);
			} break;
			case CMZN_MATERIAL_ATTRIBUTE_DIFFUSE:
			{
				return_code = Graphical_material_get_diffuse(material, &colour);
			}	break;
			case CMZN_MATERIAL_ATTRIBUTE_EMISSION:
			{
				return_code = Graphical_material_get_emission(material, &colour);
			}	break;
			case CMZN_MATERIAL_ATTRIBUTE_SPECULAR:
			{
				return_code = Graphical_material_get_specular(material, &colour);
			}	break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"cmzn_material_get_attribute_real3.  Invalid attribute");
				return_code = 0;
			} break;
		}
		if (return_code)
		{
			values[0] = (double)colour.red;
			values[1] = (double)colour.green;
			values[2] = (double)colour.blue;
		}
	}
	return (return_code);
}

int cmzn_material_set_attribute_real3(cmzn_material_id material,
	enum cmzn_material_attribute attribute, const double *values)
{
	struct Colour colour;
	int return_code = 0;
	colour.red = (ZnReal)values[0];
	colour.green = (ZnReal)values[1];
	colour.blue = (ZnReal)values[2];
	if (material)
	{
		return_code = 1;
		switch (attribute)
		{
			case CMZN_MATERIAL_ATTRIBUTE_AMBIENT:
			{
				return_code = Graphical_material_set_ambient(material, &colour);
			} break;
			case CMZN_MATERIAL_ATTRIBUTE_DIFFUSE:
			{
				return_code = Graphical_material_set_diffuse(material, &colour);
			}	break;
			case CMZN_MATERIAL_ATTRIBUTE_EMISSION:
			{
				return_code = Graphical_material_set_emission(material, &colour);
			}	break;
			case CMZN_MATERIAL_ATTRIBUTE_SPECULAR:
			{
				return_code = Graphical_material_set_specular(material, &colour);
			}	break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"cmzn_material_set_attribute_real3.  Invalid attribute");
				return_code = 0;
			} break;
		}
	}
	return return_code;
}

cmzn_shaderuniforms_id cmzn_material_get_shaderuniforms(cmzn_material_id material)
{
	if (material)
	{
		if (material->uniforms)
			return cmzn_shaderuniforms_access(material->uniforms);
	}
	return 0;
}

int cmzn_material_set_shaderuniforms(cmzn_material_id material,
	cmzn_shaderuniforms_id shaderuniforms)
{
	if (material)
	{
		REACCESS(cmzn_shaderuniforms)(&(material->uniforms), shaderuniforms);
		material->compile_status = GRAPHICS_NOT_COMPILED;
		if (material->manager)
		{
			Graphical_material_changed(material);
		}
		return CMZN_OK;
	}

	return CMZN_ERROR_ARGUMENT;
}

cmzn_shaderprogram_id cmzn_material_get_shaderprogram(cmzn_material_id material)
{
	if (material)
	{
		if (material->program)
			return cmzn_shaderprogram_access(material->program);
	}
	return 0;
}

int cmzn_material_set_shaderprogram(cmzn_material_id material,
	cmzn_shaderprogram_id shaderprogram)
{
	if (material)
	{
		REACCESS(cmzn_shaderprogram)(&(material->program), shaderprogram);
		material->compile_status = GRAPHICS_NOT_COMPILED;
		if (material->manager)
		{
			Graphical_material_changed(material);
		}
		return CMZN_OK;
	}

	return CMZN_ERROR_ARGUMENT;
}


class cmzn_material_attribute_conversion
{
public:
	static const char *to_string(enum cmzn_material_attribute attribute)
	{
		const char *enum_string = 0;
		switch (attribute)
		{
			case CMZN_MATERIAL_ATTRIBUTE_ALPHA:
				enum_string = "ALPHA";
				break;
			case CMZN_MATERIAL_ATTRIBUTE_AMBIENT:
				enum_string = "AMBIENT";
				break;
			case CMZN_MATERIAL_ATTRIBUTE_DIFFUSE:
				enum_string = "DIFFUSE";
				break;
			case CMZN_MATERIAL_ATTRIBUTE_EMISSION:
				enum_string = "EMISSION";
				break;
			case CMZN_MATERIAL_ATTRIBUTE_SHININESS:
				enum_string = "SHININESS";
				break;
			case CMZN_MATERIAL_ATTRIBUTE_SPECULAR:
				enum_string = "SPECULAR";
				break;
			default:
				break;
		}
		return enum_string;
	}
};

enum cmzn_material_attribute cmzn_material_attribute_enum_from_string(
	const char *string)
{
	return string_to_enum<enum cmzn_material_attribute,
	cmzn_material_attribute_conversion>(string);
}

char *cmzn_material_attribute_enum_to_string(
	enum cmzn_material_attribute attribute)
{
	const char *attribute_string = cmzn_material_attribute_conversion::to_string(attribute);
	return (attribute_string ? duplicate_string(attribute_string) : 0);
}

cmzn_materialiterator_id cmzn_materialiterator_access(cmzn_materialiterator_id iterator)
{
	if (iterator)
		return iterator->access();
	return 0;
}

int cmzn_materialiterator_destroy(cmzn_materialiterator_id *iterator_address)
{
	if (!iterator_address)
		return 0;
	return cmzn_materialiterator::deaccess(*iterator_address);
}

cmzn_material_id cmzn_materialiterator_next(cmzn_materialiterator_id iterator)
{
	if (iterator)
		return iterator->next();
	return 0;
}

cmzn_material_id cmzn_materialiterator_next_non_access(cmzn_materialiterator_id iterator)
{
	if (iterator)
		return iterator->next_non_access();
	return 0;
}
