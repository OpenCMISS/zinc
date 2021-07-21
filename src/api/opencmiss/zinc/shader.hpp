/**
 * @file shader.hpp
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef CMZN_SHADER_HPP__
#define CMZN_SHADER_HPP__

#include "opencmiss/zinc/shader.h"
#include "opencmiss/zinc/context.hpp"

namespace OpenCMISS
{
namespace Zinc
{

class Shaderprogram
{
protected:
	cmzn_shaderprogram_id id;

public:

	Shaderprogram() : id(0)
	{  }

	// takes ownership of C handle, responsibility for destroying it
	explicit Shaderprogram(cmzn_shaderprogram_id in_shaderprogram_id) :
		id(in_shaderprogram_id)
	{  }

	Shaderprogram(const Shaderprogram& shaderprogram) :
		id(cmzn_shaderprogram_access(shaderprogram.id))
	{  }

	Shaderprogram& operator=(const Shaderprogram& shaderprogram)
	{
		cmzn_shaderprogram_id temp_id = cmzn_shaderprogram_access(shaderprogram.id);
		if (0 != id)
		{
			cmzn_shaderprogram_destroy(&id);
		}
		id = temp_id;
		return *this;
	}

	~Shaderprogram()
	{
		if (0 != id)
		{
			cmzn_shaderprogram_destroy(&id);
		}
	}

	bool isValid() const
	{
		return (0 != id);
	}

	cmzn_shaderprogram_id getId() const
	{
		return id;
	}

	bool isManaged()
	{
		return cmzn_shaderprogram_is_managed(id);
	}

	int setManaged(bool value)
	{
		return cmzn_shaderprogram_set_managed(id, value);
	}

	char *getName()
	{
		return cmzn_shaderprogram_get_name(id);
	}

	int setName(const char *name)
	{
		return cmzn_shaderprogram_set_name(id, name);
	}

	char *getVertexShader()
	{
		return cmzn_shaderprogram_get_vertex_shader(id);
	}

	int setVertexShader(const char *vertex_shader_string)
	{
		return cmzn_shaderprogram_set_vertex_shader(id, vertex_shader_string);
	}

	char *getFragmentShader()
	{
		return cmzn_shaderprogram_get_fragment_shader(id);
	}

	int setFragmentShader(const char *fragment_shader_string)
	{
		return cmzn_shaderprogram_set_fragment_shader(id, fragment_shader_string);
	}

};

inline bool operator==(const Shaderprogram& a, const Shaderprogram& b)
{
	return a.getId() == b.getId();
}

class Shaderuniforms
{
protected:
	cmzn_shaderuniforms_id id;

public:

	Shaderuniforms() : id(0)
	{  }

	// takes ownership of C handle, responsibility for destroying it
	explicit Shaderuniforms(cmzn_shaderuniforms_id in_shaderuniforms_id) :
		id(in_shaderuniforms_id)
	{  }

	Shaderuniforms(const Shaderuniforms& shaderuniforms) :
		id(cmzn_shaderuniforms_access(shaderuniforms.id))
	{  }

	Shaderuniforms& operator=(const Shaderuniforms& shaderuniforms)
	{
		cmzn_shaderuniforms_id temp_id = cmzn_shaderuniforms_access(shaderuniforms.id);
		if (0 != id)
		{
			cmzn_shaderuniforms_destroy(&id);
		}
		id = temp_id;
		return *this;
	}

	~Shaderuniforms()
	{
		if (0 != id)
		{
			cmzn_shaderuniforms_destroy(&id);
		}
	}

	bool isValid() const
	{
		return (0 != id);
	}

	cmzn_shaderuniforms_id getId() const
	{
		return id;
	}

	bool isManaged()
	{
		return cmzn_shaderuniforms_is_managed(id);
	}

	int setManaged(bool value)
	{
		return cmzn_shaderuniforms_set_managed(id, value);
	}

	char *getName()
	{
		return cmzn_shaderuniforms_get_name(id);
	}

	int setName(const char *name)
	{
		return cmzn_shaderuniforms_set_name(id, name);
	}

	int removeUniform(const char *name)
	{
		return cmzn_shaderuniforms_remove_uniform(id, name);
	}

	int getUniformReal(const char *name, int valuesCount, double *valuesOut)
	{
		return cmzn_shaderuniforms_get_uniform_real(id,
			name, valuesCount, valuesOut);
	}

	int addUniformReal(const char *name, int valuesCount, const double *valuesIn)
	{
		return cmzn_shaderuniforms_add_uniform_real(id,
			name, valuesCount, valuesIn);
	}

	int setUniformReal(const char *name, int valuesCount, const double *valuesIn)
	{
		return cmzn_shaderuniforms_set_uniform_real(id,
			name, valuesCount, valuesIn);
	}

	int getUniformInteger(const char *name, int valuesCount, int *valuesOut)
	{
		return cmzn_shaderuniforms_get_uniform_integer(id,
			name, valuesCount, valuesOut);
	}

	int addUniformInteger(const char *name, int valuesCount, const int *valuesIn)
	{
		return cmzn_shaderuniforms_add_uniform_integer(id,
			name, valuesCount, valuesIn);
	}

	int setUniformInteger(const char *name, int valuesCount, const int *valuesIn)
	{
		return cmzn_shaderuniforms_set_uniform_integer(id,
			name, valuesCount, valuesIn);
	}

};

inline bool operator==(const Shaderuniforms& a, const Shaderuniforms& b)
{
	return a.getId() == b.getId();
}

class Shadermodule
{
protected:
	cmzn_shadermodule_id id;

public:

	Shadermodule() : id(0)
	{  }

	// takes ownership of C handle, responsibility for destroying it
	explicit Shadermodule(cmzn_shadermodule_id in_shadermodule_id) :
		id(in_shadermodule_id)
	{  }

	Shadermodule(const Shadermodule& shaderModule) :
		id(cmzn_shadermodule_access(shaderModule.id))
	{  }

	Shadermodule& operator=(const Shadermodule& shaderModule)
	{
		cmzn_shadermodule_id temp_id = cmzn_shadermodule_access(
				shaderModule.id);
		if (0 != id)
		{
			cmzn_shadermodule_destroy(&id);
		}
		id = temp_id;
		return *this;
	}

	~Shadermodule()
	{
		if (0 != id)
		{
			cmzn_shadermodule_destroy(&id);
		}
	}

	bool isValid() const
	{
		return (0 != id);
	}

	cmzn_shadermodule_id getId() const
	{
		return id;
	}

	Shaderprogram createShaderprogram()
	{
		return Shaderprogram(cmzn_shadermodule_create_shaderprogram(id));
	}

	Shaderprogram findShaderprogramByName(const char *name)
	{
		return Shaderprogram(cmzn_shadermodule_find_shaderprogram_by_name(id, name));
	}

	Shaderuniforms createShaderuniforms()
	{
		return Shaderuniforms(cmzn_shadermodule_create_shaderuniforms(id));
	}

	Shaderuniforms findShaderuniformsByName(const char *name)
	{
		return Shaderuniforms(cmzn_shadermodule_find_shaderuniforms_by_name(id, name));
	}

};

inline Shadermodule Context::getShadermodule()
{
	return Shadermodule(cmzn_context_get_shadermodule(id));
}

}  // namespace Zinc
}

#endif
