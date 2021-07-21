/***************************************************************************//**
 * spectrum.hpp
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined(SPECTRUM_HPP)
#define SPECTRUM_HPP

class Render_graphics_opengl;

int Spectrum_compile_colour_lookup(struct cmzn_spectrum *spectrum,
	Render_graphics_opengl *renderer);

int Spectrum_execute_colour_lookup(struct cmzn_spectrum *spectrum,
	Render_graphics_opengl *renderer);

#if defined (OPENGL_API)
struct Spectrum_render_data *spectrum_start_renderGL(
	struct cmzn_spectrum *spectrum,struct cmzn_material *material,
	int number_of_data_components);
/*******************************************************************************
LAST MODIFIED : 3 June 1999

DESCRIPTION :
Initialises the graphics state for rendering values on the current material.
==============================================================================*/

int spectrum_renderGL_value(struct cmzn_spectrum *spectrum,
	struct cmzn_material *material,struct Spectrum_render_data *render_data,
	GLfloat *data);
/*******************************************************************************
LAST MODIFIED : 1 June 1999

DESCRIPTION :
Sets the graphics rendering state to represent the value 'data' in
accordance with the spectrum.
==============================================================================*/

int spectrum_end_renderGL(struct cmzn_spectrum *spectrum,
	struct Spectrum_render_data *render_data);
/*******************************************************************************
LAST MODIFIED : 13 March 1998

DESCRIPTION :
Resets the graphics state after rendering values on current material.
==============================================================================*/

#endif /* defined (OPENGL_API) */

#endif /* !defined(SPECTRUM_HPP) */
