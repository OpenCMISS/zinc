/**
 * @file shader.h
 *
 * Public interface to shader objects. Shaders provides additional functions to
 * change and add special effects to graphics but it may also provide non-graphics
 * related functions.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CMZN_SHADER_H__
#define CMZN_SHADER_H__

#include "types/shaderid.h"
#include "opencmiss/zinc/zincsharedobject.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Returns a new handle to the shader module with reference count
 * incremented.
 *
 * @param shadermodule  Handle to shader module.
 * @return  New handle to shader module, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_shadermodule_id cmzn_shadermodule_access(
    cmzn_shadermodule_id shadermodule);

/**
 * Destroys this handle to the shader module (and sets it to NULL).
 * Internally this decrements the reference count.
 *
 * @param shadermodule_address  Address of handle to shader module
 *   to destroy.
 * @return  Status CMZN_OK on success, otherwise CMZN_ERROR_ARGUMENT.
 */
ZINC_API int cmzn_shadermodule_destroy(
    cmzn_shadermodule_id *shadermodule_address);

/**
 * Begin caching or increment cache level for this shader module. Call this
 * function before making multiple changes to minimise number of change messages
 * sent to clients. Must remember to end_change after completing changes.
 * Can be nested.
 * @see cmzn_shadermodule_end_change
 *
 * @param shadermodule  The shader module to begin change cache on.
 * @return  Status CMZN_OK on success, otherwise CMZN_ERROR_ARGUMENT.
 */
ZINC_API int cmzn_shadermodule_begin_change(cmzn_shadermodule_id shadermodule);

/**
 * Decrement cache level or end caching of changes for the shader module.
 * Call shader module begin change method before making multiple changes
 * and call this afterwards. When change level is restored to zero,
 * cached change messages are sent out to clients.
 * @see cmzn_shadermodule_begin_change
 *
 * @param shadermodule  The shader module to end change cache on.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
ZINC_API int cmzn_shadermodule_end_change(cmzn_shadermodule_id shadermodule);

/**
 * Create and return shader uniforms.
 *
 * @param shadermodule  The handle to the shader module the
 * shader will belong to.
 * @return  Handle to new shader uniforms, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_shaderuniforms_id cmzn_shadermodule_create_shaderuniforms(
    cmzn_shadermodule_id shadermodule);


/**
 * Find the shader uniforms with the specified name, if any.
 *
 * @param shadermodule  Shader module to search.
 * @param name  The name of the shader uniforms.
 * @return  Handle to shader uniforms, or NULL/invalid handle if not found or failed.
 */
ZINC_API cmzn_shaderuniforms_id cmzn_shadermodule_find_shaderuniforms_by_name(
    cmzn_shadermodule_id shadermodule, const char *name);

/**
 * Returns a new handle to the shader uniforms with reference count incremented.
 *
 * @param shaderuniforms  Handle to shader uniforms.
 * @return  New handle to shader uniforms, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_shaderuniforms_id cmzn_shaderuniforms_access(cmzn_shaderuniforms_id shaderuniforms);

/**
 * Destroys handle to the shaderuniforms (and sets it to NULL).
 * Internally this decrements the reference count.
 *
 * @param shaderuniforms_address  The address to the handle of the shaderuniforms
 *    to be destroyed.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
ZINC_API int cmzn_shaderuniforms_destroy(cmzn_shaderuniforms_id *shaderuniforms_address);

/**
 * Get managed status of shader uniforms in its owning shader module.
 * @see cmzn_shaderuniforms_set_managed
 *
 * @param shaderuniforms  The shader uniforms to query.
 * @return  1 (true) if shader uniforms is managed, otherwise 0 (false).
 */
ZINC_API bool cmzn_shaderuniforms_is_managed(cmzn_shaderuniforms_id shaderuniforms);

/**
 * Set managed status of shader uniforms in its owning shader module.
 * If set (managed) the shader uniforms will remain indefinitely in the shader uniforms
 * module even if no external references are held.
 * If not set (unmanaged) the shader uniforms will be automatically removed from the
 * module when no longer referenced externally, effectively marking it as
 * pending destruction.
 * All new objects are unmanaged unless stated otherwise.
 *
 * @param shaderuniforms  The shader uniforms to modify.
 * @param value  The new value for the managed flag: true or false.
 * @return  Status CMZN_OK on success, otherwise CMZN_ERROR_ARGUMENT.
 */
ZINC_API int cmzn_shaderuniforms_set_managed(cmzn_shaderuniforms_id shaderuniforms,
    bool value);

/**
 * Remove a set of uniform values from the uniforms object,
 *
 * @param shaderuniforms  The shader uniforms to modify.
 * @param name  Name of uniform to be removed.
 * @return  Status CMZN_OK on success, otherwise CMZN_ERROR_ARGUMENT.
 */
ZINC_API int cmzn_shaderuniforms_remove_uniform(cmzn_shaderuniforms_id shaderuniforms,
	const char *name);

/**
 * Return the size and values of the specified uniform if present.
 *
 * @param shaderuniforms  The shader uniforms to query.
 * @param valuesCount  The size of the valuesOut array to fill, from 1 to 4.
 * @param valuesOut  Array to receive uniform values.
 * @return  The actual number of values in the uniform, or 0 on error.
 */
ZINC_API int cmzn_shaderuniforms_get_uniform_real(cmzn_shaderuniforms_id shaderuniforms,
	const char *name, int valuesCount, double *valuesOut);

/**
 * Add a new set of double precision uniform to the uniforms object.
 * This set of values will be written into the shaders at runtime
 * when it is used by the shader program.
 * If an uniform with the same name already exist, nothing will be added to the
 * uniforms object.
 *
 * @param shaderuniforms  The shader uniforms to modify.
 * @param name  name of the uniform to add
 * @param valuesCount  The size of the valuesIn array to fill, from 1 to 4.
 * @param valuesIn  Initial values to be set for the uniform.
 * @return  Status CMZN_OK on success, otherwise CMZN_ERROR_ARGUMENT.
 */
ZINC_API int cmzn_shaderuniforms_add_uniform_real(cmzn_shaderuniforms_id shaderuniforms,
	const char *name, int valuesCount, const double *valuesIn);

/**
 * Set values of the specified uniform if it is found with
 * the specified name. This will not change the size nor type of the uniform
 * values.
 * If valuesCount is not the same as the size of the uniforms value found,
 * it will only set the minimal number of values possible. The operation
 * fails when an uniform with the same name and value type is not found.
 *
 * @param shaderuniforms  The shaderuniforms to modify.
 * @param name  name of the uniform to set.
 * @param valuesCount  The size of the valuesIn array to fill, from 1 to 4.
 * @param values  Array containing values to be set for the uniform.
 * @return  Status CMZN_OK on success, otherwise CMZN_ERROR_ARGUMENT.
 */
ZINC_API int cmzn_shaderuniforms_set_uniform_real(cmzn_shaderuniforms_id shaderuniforms,
	const char *name, int valuesCount, const double *valuesIn);

/**
 * Return the size and values of the specified uniform if present.
 *
 * @param shaderuniforms  The shaderuniforms to query.
 * @param valuesCount  The size of the valuesOut array to fill, from 1 to 4.
 * @param valuesOut  Array to receive uniform values.
 * @return  The actual number of values in the uniform, or 0 on error.
 */
ZINC_API int cmzn_shaderuniforms_get_uniform_integer(cmzn_shaderuniforms_id shaderuniforms,
	const char *name, int valuesCount, int *valuesOut);

/**
 * Add a new set of integer uniform to the uniforms object.
 * This set of values will be written into the shaders at runtime
 * when it is used by the shader program.
 * If an uniform with the same name already exists, nothing will be added to the
 * uniforms object.
 *
 * @param shaderuniforms  The shaderuniforms to modify.
 * @param name  name of the uniform to add
 * @param valuesCount  The size of the valuesIn array to fill, from 1 to 4.
 * @param valuesIn  Initial values to be set for the uniform.
 * @return  Status CMZN_OK on success, otherwise CMZN_ERROR_ARGUMENT.
 */
ZINC_API int cmzn_shaderuniforms_add_uniform_integer(cmzn_shaderuniforms_id shaderuniforms,
	const char *name, int valuesCount, const int *valuesIn);

/**
 * Set values of the specified uniform if it is found with
 * the specified name. This will not change the size nor type of the uniform
 * values.
 * If valuesCount is not the same as the size of the uniforms value found,
 * it will only set the minimal number of values possible. The operation
 * fails when an uniform with the same name and value type is not found.
 *
 * @param shaderuniforms  The shaderuniforms to modify.
 * @param name  name of the uniform to set.
 * @param valuesCount  The size of the valuesIn array to fill, from 1 to 4.
 * @param values  Array containing values to be set for the uniform.
 * @return  Status CMZN_OK on success, otherwise CMZN_ERROR_ARGUMENT.
 */
ZINC_API int cmzn_shaderuniforms_set_uniform_integer(cmzn_shaderuniforms_id shaderuniforms,
	const char *name, int valuesCount, const int *valuesIn);

/**
 * Return an allocated string containing shaderuniforms name.
 *
 * @param shaderuniforms  handle to the zinc shaderuniforms.
 * @return  allocated string containing shaderuniforms name, or NULL on failure.
 * Up to caller to free using cmzn_deallocate().
 */
ZINC_API char *cmzn_shaderuniforms_get_name(cmzn_shaderuniforms_id shaderuniforms);

/**
 * Set/change name for shaderuniforms. Must be unique in the shaderuniforms module.
 *
 * @param shaderuniforms  The handle to zinc shaderuniforms.
 * @param name  name to be set to the shaderuniforms
 * @return  status CMZN_OK if successfully set/change name for shaderuniforms,
 * any other value on failure.
 */
ZINC_API int cmzn_shaderuniforms_set_name(cmzn_shaderuniforms_id shaderuniforms, const char *name);

/**
 * Create and return a new shader program.
 *
 * @param shadermodule  The handle to the shader module the
 * shader will belong to.
 * @return  Handle to new shader program, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_shaderprogram_id cmzn_shadermodule_create_shaderprogram(
    cmzn_shadermodule_id shadermodule);

/**
 * Find the shader program with the specified name, if any.
 *
 * @param shadermodule  Shader module to search.
 * @param name  The name of the shader program.
 * @return  Handle to shader program, or NULL/invalid handle if not found or failed.
 */
ZINC_API cmzn_shaderprogram_id cmzn_shadermodule_find_shaderprogram_by_name(
    cmzn_shadermodule_id shadermodule, const char *name);


/**
 * Create and return a new shader program.
 *
 * @param shadermodule  The handle to the shader module the
 * shader will belong to.
 * @return  Handle to new shader program, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_shaderprogram_id cmzn_shadermodule_create_shaderprogram(
    cmzn_shadermodule_id shadermodule);

/**
 * Find the shader uniforms with the specified name, if any.
 *
 * @param shadermodule  Shader module to search.
 * @param name  The name of the shader uniforms.
 * @return  Handle to shader program, or NULL/invalid handle if not found or failed.
 */
ZINC_API cmzn_shaderprogram_id cmzn_shadermodule_find_shaderprogram_by_name(
    cmzn_shadermodule_id shadermodule, const char *name);

/**
 * Returns a new handle to the shader program with reference count incremented.
 *
 * @param shaderprogram  Handle to shaderprogram.
 * @return  New handle to shader program, or NULL/invalid handle on failure.
 */
ZINC_API cmzn_shaderprogram_id cmzn_shaderprogram_access(
	cmzn_shaderprogram_id shaderprogram);

/**
 * Destroys handle to the shader program (and sets it to NULL).
 * Internally this decrements the reference count.
 *
 * @param shaderprogram_address  The address to the handle of the shader program
 *    to be destroyed.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
ZINC_API int cmzn_shaderprogram_destroy(cmzn_shaderprogram_id *shaderprogram_address);

/**
 * Get managed status of shader program in its owning shader module.
 * @see cmzn_shaderprogram_set_managed
 *
 * @param shaderprogram  The shader program to query.
 * @return  1 (true) if shader program is managed, otherwise 0 (false).
 */
ZINC_API bool cmzn_shaderprogram_is_managed(cmzn_shaderprogram_id shaderprogram);

/**
 * Set managed status of shader program in its owning shader module.
 * If set (managed) the shader program will remain indefinitely in the shader program
 * module even if no external references are held.
 * If not set (unmanaged) the shader program will be automatically removed from the
 * module when no longer referenced externally, effectively marking it as
 * pending destruction.
 * All new objects are unmanaged unless stated otherwise.
 *
 * @param shaderprogram  The shader program to modify.
 * @param value  The new value for the managed flag: true or false.
 * @return  Status CMZN_OK on success, otherwise CMZN_ERROR_ARGUMENT.
 */
ZINC_API int cmzn_shaderprogram_set_managed(cmzn_shaderprogram_id shaderprogram,
    bool value);

/**
 * Return an allocated string containing shader program name.
 *
 * @param shaderprogram  handle to the zinc shader program.
 * @return  allocated string containing shader program name, or NULL on failure.
 * Up to caller to free using cmzn_deallocate().
 */
ZINC_API char *cmzn_shaderprogram_get_name(cmzn_shaderprogram_id shaderprogram);

/**
 * Set/change name for shader program. Must be unique in the shader program module.
 *
 * @param shaderprogram  The handle to zinc shader program.
 * @param name  name to be set to the shader program
 * @return  status CMZN_OK if successfully set/change name for shader program,
 * any other value on failure.
 */
ZINC_API int cmzn_shaderprogram_set_name(cmzn_shaderprogram_id shaderprogram, const char *name);

/**
 * Get the vertex shader that has been set for this program.
 *
 * @param shaderprogram  The handle to zinc shader program.
 * @return  allocated string of the vertex shader this should be freed afterward,
 * 0 on failure.
 */
ZINC_API char *cmzn_shaderprogram_get_vertex_shader(cmzn_shaderprogram_id shaderprogram);

/**
 * Set/change the vertex shader to be compiled for shader program. The provided
 * string will be compiled at run time if no error is encountered.
 * Only GLSL is currently supported.
 *
 * @param shaderprogram  The handle to zinc shader program.
 * @param vertex_program_string  string for the vertex shader to be compiled
 * @return  status CMZN_OK if successfully set/change vertex string for shader program,
 * any other value on failure.
 */
ZINC_API int cmzn_shaderprogram_set_vertex_shader(cmzn_shaderprogram_id shaderprogram,
	const char *vertex_shader_string);

/**
 * Get the fragment shader that has been set for this program.
 *
 * @param shaderprogram  The handle to zinc shader program.
 * @return  allocated string of the fragment shader this should be freed afterward,
 * 0 on failure.
 */
ZINC_API char *cmzn_shaderprogram_get_fragment_shader(cmzn_shaderprogram_id shaderprogram);

/**
 * Set/change the fragment shader to be compiled for shader program. The provided
 * string will be compiled at run time if no error is encountered.
 * Only GLSL is currently supported.
 *
 * @param shaderprogram  The handle to zinc shader program.
 * @param fragment_program_string  string for the fragment shader to be compiled
 * @return  status CMZN_OK if successfully set/change vertex string for shader program,
 * any other value on failure.
 */
ZINC_API int cmzn_shaderprogram_set_fragment_shader(cmzn_shaderprogram_id shaderprogram,
	const char *fragment_shader_string);


#ifdef __cplusplus
}
#endif

#endif
