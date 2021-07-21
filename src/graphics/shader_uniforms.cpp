/***************************************************************************//**
 * shaders_uniform.cpp
 *
 * Objects for providing shaders uniforms values.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "opencmiss/zinc/zincconfigure.h"
#include "opencmiss/zinc/status.h"
#include "opencmiss/zinc/types/shaderid.h"
#include "general/debug.h"
#include "general/cmiss_set.hpp"
#include "general/indexed_list_stl_private.hpp"
#include "general/manager_private.h"
#include "general/message.h"
#include "general/mystring.h"
#include "general/value.h"
#include "graphics/graphics_library.h"
#include "graphics/shader_uniforms.hpp"


#include <stdio.h>
#include <string>
#include <list>
#include <iterator>

struct cmzn_shaderuniform
{
public:
	std::string name;
	int size;

	cmzn_shaderuniform(const char *nameIn, unsigned int sizeIn) :
		name(std::string(nameIn)), size(sizeIn)
	{
	}

	virtual ~cmzn_shaderuniform()
	{
	}

	virtual int writeToShaders(unsigned int currentProgram) = 0;
};

struct cmzn_shaderuniform_float : public cmzn_shaderuniform
{
private:
	float *values = 0;

public:

	cmzn_shaderuniform_float(const char *nameIn, int sizeIn) :
		cmzn_shaderuniform(nameIn, sizeIn)
	{
		if ((sizeIn > 0) && (sizeIn < 4))
			values = new float[sizeIn];
	}

	virtual ~cmzn_shaderuniform_float()
	{
		delete[] values;
	}

	int setValues(int numberOfComponent, float *valuesIn)
	{
		if (values)
		{
			for (int i = 0; (i < numberOfComponent && i < size); i++)
			{
				values[i] = valuesIn[i];
			}
			return CMZN_OK;
		}
		return CMZN_ERROR_ARGUMENT;
	}

	int getValues(int numberOfComponent, float *valuesOut)
	{
		if (values)
		{
			for (int i = 0; (i < numberOfComponent && i < size); i++)
			{
				valuesOut[i] = values[i];
			}
			return size;
		}
		return 0;
	}

	virtual int writeToShaders(unsigned int currentProgram)
	{
		if ((size > 0) && (values))
		{
	#if defined (GL_VERSION_2_0)
			GLint location = glGetUniformLocation((GLuint)currentProgram,
				name.c_str());
			if (location != (GLint)-1)
			{
				switch(size)
				{
					case 1:
					{
						glUniform1f(location, values[0]);
					} break;
					case 2:
					{
						glUniform2f(location, values[0], values[1]);
					} break;
					case 3:
					{
						glUniform3f(location, values[0], values[1], values[2]);
					} break;
					case 4:
					{
						glUniform4f(location, values[0], values[1], values[2], values[3]);
					} break;
				}
			}
	#endif
			return CMZN_OK;
		}
		return CMZN_ERROR_ARGUMENT;
	}
};

struct cmzn_shaderuniform_integer : public cmzn_shaderuniform
{
private:
	int *values = 0;

public:

	cmzn_shaderuniform_integer(const char *nameIn, int sizeIn) :
		cmzn_shaderuniform(nameIn, sizeIn)
	{
		if ((sizeIn > 0) && (sizeIn < 4))
			values = new int[sizeIn];
	}

	virtual ~cmzn_shaderuniform_integer()
	{
		delete[] values;
	}

	int getValues(int numberOfComponent, int *valuesOut)
	{
		if (values)
		{
			for (int i = 0; (i < numberOfComponent && i < size); i++)
			{
				valuesOut[i] = values[i];
			}
			return size;
		}
		return 0;
	}

	int setValues(int numberOfComponent, const int *valuesIn)
	{
		if (values)
		{
			for (int i = 0; (i < numberOfComponent && i < size); i++)
			{
				values[i] = valuesIn[i];
			}
			return CMZN_OK;
		}
		return CMZN_ERROR_ARGUMENT;
	}

	virtual int writeToShaders(unsigned int currentProgram)
	{
		if ((size > 0) && (values))
		{
	#if defined (GL_VERSION_2_0)
			GLint location = glGetUniformLocation((GLuint)currentProgram,
				name.c_str());
			if (location != (GLint)-1)
			{
				switch(size)
				{
					case 1:
					{
						glUniform1i(location, values[0]);
					} break;
					case 2:
					{
						glUniform2i(location, values[0], values[1]);
					} break;
					case 3:
					{
						glUniform3i(location, values[0], values[1], values[2]);
					} break;
					case 4:
					{
						glUniform4i(location, values[0], values[1], values[2], values[3]);
					} break;
				}
			}
	#endif
			return CMZN_OK;
		}
		return CMZN_ERROR_ARGUMENT;
	}
};

struct cmzn_shaderuniforms
{
private:
	std::list<cmzn_shaderuniform *> uniforms;

public:
	const char *name;
	struct MANAGER(cmzn_shaderuniforms) *manager;
	cmzn_shaderuniforms_change_detail changeDetail;
	bool is_managed_flag;
	int manager_change_status;
	int access_count;

	cmzn_shaderuniforms() :
		name(NULL),
		manager(NULL),
		is_managed_flag(false),

		manager_change_status(MANAGER_CHANGE_NONE(cmzn_shaderuniforms)),
		access_count(1)

	{
	}

	~cmzn_shaderuniforms()
	{
		if (name)
		{
			DEALLOCATE(name);
		}
	}



	/** must construct on the heap with this function */
	static cmzn_shaderuniforms *create()
	{
		return new cmzn_shaderuniforms();
	}

	cmzn_shaderuniforms *access()
	{
		++(this->access_count);
		return this;
	}

	static inline int deaccess(cmzn_shaderuniforms **object_address)
	{
		cmzn_shaderuniforms *shaderuniforms;

		if (object_address && (shaderuniforms = *object_address))
		{
			--(shaderuniforms->access_count);
			if (shaderuniforms->access_count <= 0)
			{
				std::list<cmzn_shaderuniform *>::iterator pos;
				for (pos = shaderuniforms->uniforms.begin(); pos != shaderuniforms->uniforms.end(); ++pos)
				{
					delete (*pos);
				}
				delete shaderuniforms;
			}
			shaderuniforms = 0;
			return CMZN_OK;
		}
		return CMZN_ERROR_ARGUMENT;
	}

	int removeUniform(const char*name)
	{
		if (name)
		{
			std::list<cmzn_shaderuniform *>::iterator pos;
			for (pos = uniforms.begin(); pos != uniforms.end(); ++pos)
			{
				if (strcmp((*pos)->name.c_str(), name) == 0)
				{
					delete (*pos);
					uniforms.remove((*pos));
					return CMZN_OK;
				}
			}
		}

		return CMZN_ERROR_ARGUMENT;
	}

	int getUniformFloat(const char *name, int numberOfComponents, float *valuesOut)
	{
		if (name && ((numberOfComponents > 0) && (numberOfComponents < 4)) && valuesOut)
		{
			std::list<cmzn_shaderuniform *>::iterator pos;
			for (pos = uniforms.begin(); pos != uniforms.end(); ++pos)
			{
				if (strcmp((*pos)->name.c_str(), name) == 0)
				{
					cmzn_shaderuniform_float *uniform = dynamic_cast<cmzn_shaderuniform_float *>(*pos);
					if (uniform)
						return uniform->getValues(numberOfComponents, valuesOut);
					else
						return 0;
				}
			}
		}

		return 0;
	}


	int addUniformFloat(const char *name, int numberOfComponents, float *values)
	{
		if (name && ((numberOfComponents > 0) && (numberOfComponents < 4)) && values)
		{
			std::list<cmzn_shaderuniform *>::iterator pos;
			for (pos = uniforms.begin(); pos != uniforms.end(); ++pos)
			{
				if (strcmp((*pos)->name.c_str(), name) == 0)
				{
					return CMZN_ERROR_ARGUMENT;
				}
			}
			cmzn_shaderuniform_float *uniform = new cmzn_shaderuniform_float(name, numberOfComponents);
			uniform->setValues(numberOfComponents, values);
			uniforms.push_back(uniform);
			this->uniformsChanged();
			return CMZN_OK;
		}

		return CMZN_ERROR_ARGUMENT;
	}

	int setUniformFloat(const char *name, int numberOfComponents, float *values)
	{
		if (name && ((numberOfComponents > 0) && (numberOfComponents < 4)) && values)
		{
			std::list<cmzn_shaderuniform *>::iterator pos;
			for (pos = uniforms.begin(); pos != uniforms.end(); ++pos)
			{
				if (strcmp((*pos)->name.c_str(), name) == 0)
				{
					cmzn_shaderuniform_float *uniform = dynamic_cast<cmzn_shaderuniform_float *>(*pos);
					if (uniform)
					{
						int return_code = uniform->setValues(numberOfComponents, values);
						this->uniformsChanged();
						return return_code;
					}
					else
						return CMZN_ERROR_ARGUMENT;
				}
			}
		}

		return CMZN_ERROR_ARGUMENT;
	}

	int getUniformInteger(const char *name, int numberOfComponents, int *valuesOut)
	{
		if (name && ((numberOfComponents > 0) && (numberOfComponents < 4)) && valuesOut)
		{
			std::list<cmzn_shaderuniform *>::iterator pos;
			for (pos = uniforms.begin(); pos != uniforms.end(); ++pos)
			{
				if (strcmp((*pos)->name.c_str(), name) == 0)
				{
					cmzn_shaderuniform_integer *uniform = dynamic_cast<cmzn_shaderuniform_integer *>(*pos);
					if (uniform)
						return uniform->getValues(numberOfComponents, valuesOut);
					else
						return 0;
				}
			}
		}

		return 0;
	}

	int addUniformInteger(const char *name, int numberOfComponents, const int *values)
	{
		if (name && ((numberOfComponents > 0) && (numberOfComponents < 4)) && values)
		{
			std::list<cmzn_shaderuniform *>::iterator pos;
			for (pos = uniforms.begin(); pos != uniforms.end(); ++pos)
			{
				if (strcmp((*pos)->name.c_str(), name) == 0)
				{
					return CMZN_ERROR_ARGUMENT;
				}
			}
			cmzn_shaderuniform_integer *uniform = new cmzn_shaderuniform_integer(name, numberOfComponents);
			uniform->setValues(numberOfComponents, values);
			uniforms.push_back(uniform);
			this->uniformsChanged();
			return CMZN_OK;
		}

		return CMZN_ERROR_ARGUMENT;
	}

	int setUniformInteger(const char *name, int numberOfComponents, const int *values)
	{
		if (name && ((numberOfComponents > 0) && (numberOfComponents < 4)) && values)
		{
			std::list<cmzn_shaderuniform *>::iterator pos;
			for (pos = uniforms.begin(); pos != uniforms.end(); ++pos)
			{
				if (strcmp((*pos)->name.c_str(), name) == 0)
				{
					cmzn_shaderuniform_integer *uniform = dynamic_cast<cmzn_shaderuniform_integer *>(*pos);
					if (uniform)
					{
						int return_code = uniform->setValues(numberOfComponents, values);
						this->uniformsChanged();
						return return_code;
					}
					else
						return CMZN_ERROR_ARGUMENT;
				}
			}
		}

		return CMZN_ERROR_ARGUMENT;
	}

	int writeToShaders(unsigned int currentProgram)
	{
		std::list<cmzn_shaderuniform *>::iterator pos;
		for (pos = uniforms.begin(); pos != uniforms.end(); ++pos)
		{
			if (*pos)
			{
				(*pos)->writeToShaders(currentProgram);
			}
		}

		return CMZN_OK;
	}

	void uniformsChanged()
	{
		this->changeDetail.setUniformsChanged();
		MANAGED_OBJECT_CHANGE(cmzn_shaderuniforms)(this,
			MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER(cmzn_shaderuniforms));
	}

	/** clones and clears type-specific change detail, if any.
	 * override for classes with type-specific change detail
	 * @return  change detail prior to clearing, or NULL if none.
	 */
	cmzn_shaderuniforms_change_detail *extractChangeDetail()
	{
		cmzn_shaderuniforms_change_detail *change_detail = new cmzn_shaderuniforms_change_detail(this->changeDetail);
		this->changeDetail.clear();
		return change_detail;
	}
};


/* Only to be used from FIND_BY_IDENTIFIER_IN_INDEXED_LIST_STL function
 * Creates a pseudo object with name identifier suitable for finding
 * objects by identifier with cmzn_set.
 */
class cmzn_shaderuniforms_identifier : private cmzn_shaderuniforms
{
public:
	cmzn_shaderuniforms_identifier(const char *name)
	{
		cmzn_shaderuniforms::name = name;
	}

	~cmzn_shaderuniforms_identifier()
	{
		cmzn_shaderuniforms::name = NULL;
	}

	cmzn_shaderuniforms *getPseudoObject()
	{
		return this;
	}
};

/** functor for ordering cmzn_set<cmzn_shaderuniforms> by name */
struct cmzn_shaderuniforms_compare_name
{
	bool operator() (const cmzn_shaderuniforms* shaderuniforms1, const cmzn_shaderuniforms* shaderuniforms2) const
	{
		return strcmp(shaderuniforms1->name, shaderuniforms2->name) < 0;
	}
};

typedef cmzn_set<cmzn_shaderuniforms *,cmzn_shaderuniforms_compare_name> cmzn_set_cmzn_shaderuniforms;

FULL_DECLARE_MANAGER_TYPE_WITH_OWNER(cmzn_shaderuniforms, cmzn_shadermodule, class cmzn_shaderuniforms_change_detail *);

DECLARE_DEFAULT_MANAGER_UPDATE_DEPENDENCIES_FUNCTION(cmzn_shaderuniforms);

inline class cmzn_shaderuniforms_change_detail *MANAGER_EXTRACT_CHANGE_DETAIL(cmzn_shaderuniforms)(
		cmzn_shaderuniforms *uniforms)
{
	return uniforms->extractChangeDetail();
}

inline void MANAGER_CLEANUP_CHANGE_DETAIL(cmzn_shaderuniforms)(
		cmzn_shaderuniforms_change_detail **change_detail_address)
{
	delete *change_detail_address;
}

DECLARE_MANAGER_UPDATE_FUNCTION(cmzn_shaderuniforms)

DECLARE_MANAGER_FIND_CLIENT_FUNCTION(cmzn_shaderuniforms)

DECLARE_MANAGED_OBJECT_NOT_IN_USE_CONDITIONAL_FUNCTION(cmzn_shaderuniforms)

PROTOTYPE_ACCESS_OBJECT_FUNCTION(cmzn_shaderuniforms)
{
	if (object)
		return object->access();
	return 0;
}

PROTOTYPE_DEACCESS_OBJECT_FUNCTION(cmzn_shaderuniforms)
{
	return cmzn_shaderuniforms::deaccess(object_address);
}

PROTOTYPE_REACCESS_OBJECT_FUNCTION(cmzn_shaderuniforms)
{
	if (object_address)
	{
		if (new_object)
		{
			new_object->access();
		}
		if (*object_address)
		{
			cmzn_shaderuniforms::deaccess(object_address);
		}
		*object_address = new_object;
		return 1;
	}
	return 0;
}

DECLARE_DEFAULT_GET_OBJECT_NAME_FUNCTION(cmzn_shaderuniforms)

DECLARE_INDEXED_LIST_STL_FUNCTIONS(cmzn_shaderuniforms)
DECLARE_FIND_BY_IDENTIFIER_IN_INDEXED_LIST_STL_FUNCTION(cmzn_shaderuniforms,name,const char *)
DECLARE_INDEXED_LIST_STL_IDENTIFIER_CHANGE_FUNCTIONS(cmzn_shaderuniforms,name)

DECLARE_MANAGER_FUNCTIONS(cmzn_shaderuniforms,manager)
DECLARE_DEFAULT_MANAGED_OBJECT_NOT_IN_USE_FUNCTION(cmzn_shaderuniforms,manager)
DECLARE_MANAGER_IDENTIFIER_WITHOUT_MODIFY_FUNCTIONS(cmzn_shaderuniforms,name,const char *,manager)
DECLARE_MANAGER_OWNER_FUNCTIONS(cmzn_shaderuniforms, struct cmzn_shadermodule)

int cmzn_shaderuniforms_manager_set_owner_private(struct MANAGER(cmzn_shaderuniforms) *manager,
	struct cmzn_shadermodule *shadermodule)
{
	return MANAGER_SET_OWNER(cmzn_shaderuniforms)(manager, shadermodule);
}

int cmzn_shaderuniforms_manager_message_get_object_change_and_detail(
	struct MANAGER_MESSAGE(cmzn_shaderuniforms) *message, cmzn_shaderuniforms *shaderuniforms,
	const cmzn_shaderuniforms_change_detail **change_detail_address)
{
	if (message)
		return message->getObjectChangeFlagsAndDetail(shaderuniforms, change_detail_address);
	if (change_detail_address)
		*change_detail_address = 0;
	return (MANAGER_CHANGE_NONE(cmzn_shaderuniforms));
}

struct cmzn_shaderuniforms *cmzn_shaderuniforms_create_private()
{
	return cmzn_shaderuniforms::create();
}

cmzn_shaderuniforms_id cmzn_shaderuniforms_access(cmzn_shaderuniforms_id shaderuniforms)
{
	if (shaderuniforms)
		return ACCESS(cmzn_shaderuniforms)(shaderuniforms);
	return 0;
}

int cmzn_shaderuniforms_destroy(cmzn_shaderuniforms_id *shaderuniforms_address)
{
	return DEACCESS(cmzn_shaderuniforms)(shaderuniforms_address);
}

bool cmzn_shaderuniforms_is_managed(cmzn_shaderuniforms_id shaderuniforms)
{
	if (shaderuniforms)
	{
		return shaderuniforms->is_managed_flag;
	}
	return 0;
}

int cmzn_shaderuniforms_set_managed(cmzn_shaderuniforms_id shaderuniforms,
	bool value)
{
	if (shaderuniforms)
	{
		bool old_value = shaderuniforms->is_managed_flag;
		shaderuniforms->is_managed_flag = (value != 0);
		if (value != old_value)
		{
			MANAGED_OBJECT_CHANGE(cmzn_shaderuniforms)(shaderuniforms,
				MANAGER_CHANGE_DEFINITION(cmzn_shaderuniforms));
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_shaderuniforms_remove_uniform(cmzn_shaderuniforms_id shaderuniforms,
	const char *name)
{
	if (shaderuniforms && name)
	{
		return shaderuniforms->removeUniform(name);
	}
	return CMZN_ERROR_ARGUMENT;

}

int cmzn_shaderuniforms_get_uniform_integer(cmzn_shaderuniforms_id shaderuniforms,
	const char *name, int valuesCount, int *valuesOut)
{
	if (shaderuniforms)
	{
		return shaderuniforms->getUniformInteger(name, valuesCount, valuesOut);
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_shaderuniforms_add_uniform_integer(cmzn_shaderuniforms_id shaderuniforms,
	const char *name, int valuesCount, const int *valuesIn)
{
	if (shaderuniforms)
	{
		return shaderuniforms->addUniformInteger(name, valuesCount, valuesIn);

	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_shaderuniforms_set_uniform_integer(cmzn_shaderuniforms_id shaderuniforms,
	const char *name, int valuesCount, const int *valuesIn)
{
	if (shaderuniforms)
	{
		return shaderuniforms->setUniformInteger(name, valuesCount, valuesIn);

	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_shaderuniforms_get_uniform_real(cmzn_shaderuniforms_id shaderuniforms,
	const char *name, int valuesCount, double *valuesOut)
{
	if (shaderuniforms)
	{
		if (valuesCount > 0)
		{
			float *values = new float[valuesCount];
			int return_code = shaderuniforms->addUniformFloat(name, valuesCount, values);
			CAST_TO_OTHER(valuesOut, values, double, valuesCount);
			delete[] values;
			return return_code;
		}
	}
	return 0;
}

int cmzn_shaderuniforms_add_uniform_real(cmzn_shaderuniforms_id shaderuniforms,
	const char *name, int valuesCount, const double *valuesIn)
{
	if (shaderuniforms)
	{
		if (valuesCount > 0)
		{
			float *values = new float[valuesCount];
			CAST_TO_OTHER(values, valuesIn, float, valuesCount);
			int return_code = shaderuniforms->addUniformFloat(name, valuesCount, values);
			delete[] values;
			return return_code;
		}
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_shaderuniforms_set_uniform_real(cmzn_shaderuniforms_id shaderuniforms,
	const char *name, int valuesCount, const double *valuesIn)
{
	if (shaderuniforms)
	{
		if (valuesCount > 0)
		{
			float *values = new float[valuesCount];
			CAST_TO_OTHER(values, valuesIn, float, valuesCount);
			int return_code = shaderuniforms->setUniformFloat(name, valuesCount, values);
			delete[] values;
			return return_code;
		}
	}
	return CMZN_ERROR_ARGUMENT;
}


char *cmzn_shaderuniforms_get_name(cmzn_shaderuniforms_id shaderuniforms)
{
	char *name = NULL;
	if (shaderuniforms && shaderuniforms->name)
	{
		name = duplicate_string(shaderuniforms->name);
	}
	return name;
}

void cmzn_shaderuniforms_write_to_shaders(cmzn_shaderuniforms_id shaderuniforms,
	unsigned int glsl_program)
{
	if (shaderuniforms)
	{
		shaderuniforms->writeToShaders(glsl_program);
	}
}

int cmzn_shaderuniforms_set_name(cmzn_shaderuniforms_id shaderuniforms, const char *name)
{
	int return_code;

	ENTER(cmzn_shaderuniforms_set_name);
	if (shaderuniforms && name)
	{
		return_code = 1;
		cmzn_set_cmzn_shaderuniforms *manager_shaderuniforms_list = 0;
		bool restore_changed_object_to_lists = false;
		if (shaderuniforms->manager)
		{
			cmzn_shaderuniforms *existing_shaderuniforms =
				FIND_BY_IDENTIFIER_IN_MANAGER(cmzn_shaderuniforms, name)(name, shaderuniforms->manager);
			if (existing_shaderuniforms && (existing_shaderuniforms != shaderuniforms))
			{
				display_message(ERROR_MESSAGE, "cmzn_shaderuniforms_set_name.  "
					"shaderuniforms named '%s' already exists.", name);
				return_code = 0;
			}
			if (return_code)
			{
				manager_shaderuniforms_list = reinterpret_cast<cmzn_set_cmzn_shaderuniforms *>(
					shaderuniforms->manager->object_list);
				// this temporarily removes the object from all related lists
				restore_changed_object_to_lists =
					manager_shaderuniforms_list->begin_identifier_change(shaderuniforms);
				if (!restore_changed_object_to_lists)
				{
					display_message(ERROR_MESSAGE, "cmzn_shaderuniforms_set_name.  "
						"Could not safely change identifier in manager");
					return_code = 0;
				}
			}
		}
		if (return_code)
		{
			char *new_name = duplicate_string(name);
			if (new_name)
			{
				DEALLOCATE(shaderuniforms->name);
				shaderuniforms->name = new_name;
			}
			else
			{
				return_code = 0;
			}
		}
		if (restore_changed_object_to_lists)
		{
			manager_shaderuniforms_list->end_identifier_change();
		}
		if (shaderuniforms->manager && return_code)
		{
			MANAGED_OBJECT_CHANGE(cmzn_shaderuniforms)(shaderuniforms,
				MANAGER_CHANGE_IDENTIFIER(cmzn_shaderuniforms));
		}
	}
	else
	{
		if (shaderuniforms)
		{
			display_message(ERROR_MESSAGE,
				"cmzn_shaderuniforms_set_name.  Invalid shaderuniforms name '%s'", name);
		}
		return_code=0;
	}

	return (return_code);
}
