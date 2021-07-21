/***************************************************************************//**
 * shaders.cpp
 *
 * Objects for controlling shaders program used in Zinc.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <list>
#include <cstdlib>
#include "opencmiss/zinc/status.h"
#include "general/debug.h"
#include "general/manager_private.h"
#include "general/mystring.h"
#include "general/enumerator_conversion.hpp"
#include "graphics/shader.hpp"
#include "graphics/shader_uniforms.hpp"
#include "graphics/shader_program.hpp"
#include "general/message.h"

struct ShaderprogramTypeMatch
{
	int type;
	cmzn_shaderprogram_id program;
};


static int shaderprogram_match_type(cmzn_shaderprogram *program,
	void *shaderprogramTypeMatch)
{
	if (program && shaderprogramTypeMatch)
	{
		ShaderprogramTypeMatch *typeMatch = (ShaderprogramTypeMatch *)shaderprogramTypeMatch;
		if (typeMatch->program != 0)
		{
			if ((int)cmzn_shaderprogram_get_type(program) == typeMatch->type)
			{
				typeMatch->program = program;
			}
		}
		return 1;
	}
	return 0;
}

struct cmzn_shadermodule
{

private:

	//void *manager_callback_id;
	struct MANAGER(cmzn_shaderuniforms) *uniformsManager;
	struct MANAGER(cmzn_shaderprogram) *programManager;
	int access_count;

	cmzn_shadermodule() :
		uniformsManager(CREATE(MANAGER(cmzn_shaderuniforms))()),
		programManager(CREATE(MANAGER(cmzn_shaderprogram))()),
		access_count(1)
	{
		//manager_callback_id = MANAGER_REGISTER(cmzn_tessellation)(
		//	cmzn_tessellationmodule_Tessellation_change, (void *)this, tessellationManager);
	}

	~cmzn_shadermodule()
	{
		DESTROY(MANAGER(cmzn_shaderuniforms))(&(this->uniformsManager));
		DESTROY(MANAGER(cmzn_shaderprogram))(&(this->programManager));
	}

public:
	static cmzn_shadermodule *create()
	{
		cmzn_shadermodule *shaderModule = new cmzn_shadermodule();
		return shaderModule;
	}

	cmzn_shadermodule *access()

	{
		++access_count;
		return this;
	}

	static int deaccess(cmzn_shadermodule* &shadermodule)
	{
		if (shadermodule)
		{
			--(shadermodule->access_count);
			if (shadermodule->access_count <= 0)
			{
				delete shadermodule;
			}
			shadermodule = 0;
			return CMZN_OK;
		}
		return CMZN_ERROR_ARGUMENT;
	}

	struct MANAGER(cmzn_shaderuniforms) *getUniformsManager()
	{
		return this->uniformsManager;
	}

	struct MANAGER(cmzn_shaderprogram) *getProgramManager()
	{
		return this->programManager;
	}

	int beginChange()
	{
		return MANAGER_BEGIN_CACHE(cmzn_shaderuniforms)(this->uniformsManager);
	}

	int endChange()
	{
		return MANAGER_END_CACHE(cmzn_shaderuniforms)(this->uniformsManager);
	}

	cmzn_shaderuniforms_id createShaderuniforms()
	{
		cmzn_shaderuniforms_id uniforms = NULL;
		char temp_name[20];
		int i = NUMBER_IN_MANAGER(cmzn_shaderuniforms)(this->uniformsManager);
		do
		{
			i++;
			sprintf(temp_name, "temp%d",i);
		}
		while (FIND_BY_IDENTIFIER_IN_MANAGER(cmzn_shaderuniforms,name)(temp_name,
			this->uniformsManager));
		uniforms = cmzn_shaderuniforms_create_private();
		cmzn_shaderuniforms_set_name(uniforms, temp_name);
		if (!ADD_OBJECT_TO_MANAGER(cmzn_shaderuniforms)(uniforms, this->uniformsManager))
		{
			DEACCESS(cmzn_shaderuniforms)(&uniforms);
		}
		return uniforms;
	}

	cmzn_shaderuniforms_id findShaderuniformsByName(const char *name)
	{
		cmzn_shaderuniforms_id uniforms = FIND_BY_IDENTIFIER_IN_MANAGER(cmzn_shaderuniforms,name)(name,
			this->uniformsManager);
		if (uniforms)
		{
			return ACCESS(cmzn_shaderuniforms)(uniforms);
		}
		return 0;
	}

	cmzn_shaderprogram_id createShaderprogram()
	{
		cmzn_shaderprogram_id program = NULL;
		char temp_name[20];
		int i = NUMBER_IN_MANAGER(cmzn_shaderprogram)(this->programManager);
		do
		{
			i++;
			sprintf(temp_name, "temp%d",i);
		}
		while (FIND_BY_IDENTIFIER_IN_MANAGER(cmzn_shaderprogram,name)(temp_name,
			this->programManager));
		program = cmzn_shaderprogram_create_private();
		cmzn_shaderprogram_set_name(program, temp_name);
		if (!ADD_OBJECT_TO_MANAGER(cmzn_shaderprogram)(program, this->programManager))
		{
			DEACCESS(cmzn_shaderprogram)(&program);
		}
		return program;
	}

	cmzn_shaderprogram_id findShaderprogramByName(const char *name)
	{
		cmzn_shaderprogram_id program = FIND_BY_IDENTIFIER_IN_MANAGER(cmzn_shaderprogram,name)(name,
			this->programManager);
		if (program)
		{
			return ACCESS(cmzn_shaderprogram)(program);
		}
		return 0;
	}

	cmzn_shaderprogram_id findShaderprogramByType(int type)
	{
		ShaderprogramTypeMatch typeMatch;
		typeMatch.type = type;
		typeMatch.program = 0;

		FOR_EACH_OBJECT_IN_MANAGER(cmzn_shaderprogram)(shaderprogram_match_type, (void *)&typeMatch,
			this->programManager);
		if (typeMatch.program)
			return ACCESS(cmzn_shaderprogram)(typeMatch.program);

		return 0;
	}


};

cmzn_shadermodule_id cmzn_shadermodule_create()
{
	return cmzn_shadermodule::create();
}

cmzn_shadermodule_id cmzn_shadermodule_access(
	cmzn_shadermodule_id shadermodule)
{
	if (shadermodule)
		return shadermodule->access();
	return 0;
}

int cmzn_shadermodule_destroy(cmzn_shadermodule_id *shadermodule_address)
{
	if (shadermodule_address)
		return cmzn_shadermodule::deaccess(*shadermodule_address);
	return CMZN_ERROR_ARGUMENT;
}

cmzn_shaderuniforms_id cmzn_shadermodule_create_shaderuniforms(
	cmzn_shadermodule_id shadermodule)
{
	if (shadermodule)
		return shadermodule->createShaderuniforms();
	return 0;
}

struct MANAGER(cmzn_shaderuniforms) *cmzn_shadermodule_get_uniforms_manager(
	cmzn_shadermodule_id shadermodule)
{
	if (shadermodule)
		return shadermodule->getUniformsManager();
	return 0;
}

int cmzn_shadermodule_begin_change(cmzn_shadermodule_id shadermodule)
{
	if (shadermodule)
		return shadermodule->beginChange();
   return CMZN_ERROR_ARGUMENT;
}

int cmzn_shadermodule_end_change(cmzn_shadermodule_id shadermodule)
{
	if (shadermodule)
		return shadermodule->endChange();
   return CMZN_ERROR_ARGUMENT;
}

cmzn_shaderuniforms_id cmzn_shadermodule_find_shaderuniforms_by_name(
	cmzn_shadermodule_id shadermodule, const char *name)
{
	if (shadermodule)
		return shadermodule->findShaderuniformsByName(name);
   return 0;
}

cmzn_shaderprogram_id cmzn_shadermodule_create_shaderprogram(
	cmzn_shadermodule_id shadermodule)
{
	if (shadermodule)
		return shadermodule->createShaderprogram();
	return 0;
}

struct MANAGER(cmzn_shaderprogram) *cmzn_shadermodule_get_program_manager(
	cmzn_shadermodule_id shadermodule)
{
	if (shadermodule)
		return shadermodule->getProgramManager();
	return 0;
}

cmzn_shaderprogram_id cmzn_shadermodule_find_shaderprogram_by_name(
	cmzn_shadermodule_id shadermodule, const char *name)
{
	if (shadermodule)
		return shadermodule->findShaderprogramByName(name);
   return 0;
}

cmzn_shaderprogram_id cmzn_shadermodule_find_shaderprogram_by_type(
		cmzn_shadermodule_id shadermodule, int type)
{
	if (shadermodule)
		return shadermodule->findShaderprogramByType(type);
   return 0;

}

