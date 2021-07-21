/***************************************************************************//**
 * texture.hpp
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (TEXTURE_HPP)
#define TEXTURE_HPP

class Render_graphics_opengl;

#include <general/callback_class.hpp>

/***************************************************************************//**
 * Compile the texture_object for this texture.
 */
int Texture_compile_opengl_texture_object(struct Texture *texture,
	Render_graphics_opengl *renderer);

/***************************************************************************//**
 * Execute the texture_object for this texture.
 */
int Texture_execute_opengl_texture_object(struct Texture *texture,
	Render_graphics_opengl *renderer);

/***************************************************************************//**
 * Compile the display_list for this texture.
 */
int Texture_compile_opengl_display_list(struct Texture *texture,
	Callback_base< Texture * > *execute_function,
	Render_graphics_opengl *renderer);

/***************************************************************************//**
 * Execute the display_list for this texture.
 */
int Texture_execute_opengl_display_list(struct Texture *texture,
	Render_graphics_opengl *renderer);

#endif /* !defined (TEXTURE_HPP) */
