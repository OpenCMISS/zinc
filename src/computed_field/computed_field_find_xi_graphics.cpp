/*******************************************************************************
FILE : computed_field_find_xi_graphics.cpp

LAST MODIFIED : 12 June 2008

DESCRIPTION :
Implements a special version of find_xi that uses OpenGL to accelerate the
lookup of the element.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stdio.h>
#include <math.h>

#include "opencmiss/zinc/zincconfigure.h"
#include "opencmiss/zinc/element.h"
#include "general/debug.h"
#include "general/image_utilities.h"
#include "general/matrix_vector.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_private.hpp"
#include "computed_field/computed_field_find_xi.h"
#include "computed_field/computed_field_find_xi_graphics.h"
#include "computed_field/computed_field_find_xi_private.hpp"
#include "computed_field/field_cache.hpp"
#include "finite_element/finite_element_discretization.h"
#include "finite_element/finite_element_region.h"
#include "graphics/texture.h"
#include "general/message.h"

#if defined (GRAPHICS_BUFFER_USE_OFFSCREEN_BUFFERS)
#if defined (USE_GLEW)
#include <GL/glew.h>
#else
#include <GL/gl.h>
#endif
#endif /* defined (GRAPHICS_BUFFER_USE_OFFSCREEN_BUFFERS) */

#if defined (GRAPHICS_BUFFER_USE_OFFSCREEN_BUFFERS)
struct Computed_field_find_element_xi_graphics_cache :
	public Computed_field_find_element_xi_base_cache
{
public:
	int bit_shift;
	int minimum_element_number;
	int maximum_element_number;
	struct Graphics_buffer *graphics_buffer;

	Computed_field_find_element_xi_graphics_cache() :
		Computed_field_find_element_xi_base_cache(),
	  graphics_buffer((struct Graphics_buffer *)NULL)
  {
  }

	virtual ~Computed_field_find_element_xi_graphics_cache()
	{
		if (graphics_buffer)
		{
			DEACCESS(Graphics_buffer)(&graphics_buffer);
		}
	}
};
#endif /* defined (GRAPHICS_BUFFER_USE_OFFSCREEN_BUFFERS) */

struct Render_element_data
{
	int bit_shift;
	int minimum_element_number;
	int maximum_element_number;
	cmzn_fieldcache_id field_cache;
	struct Computed_field *field;
	FE_value values[3];
};

#if defined (GRAPHICS_BUFFER_USE_OFFSCREEN_BUFFERS)
int Render_element_as_texture(struct FE_element *element,
	struct Render_element_data *data)
/*******************************************************************************
LAST MODIFIED : 24 August 2006

DESCRIPTION :
Stores cache data for the Computed_field_find_element_xi_special routine.
==============================================================================*/
{
	enum FE_element_shape_type shape_type;
	FE_value xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	ZnReal red, green, blue;
	int return_code;
	struct FE_element_shape *shape;
	unsigned int scaled_number;

	ENTER(Render_element_as_texture);
	if (data)
	{
		/* Code the element number into the colour */
		int element_identifier = get_FE_element_identifier(element);
		int dimension = get_FE_element_dimension(element);
		if (2 == dimension)
		{
			scaled_number = (unsigned int)((double)cm_information.number - data->minimum_element_number + 1);
			blue = ((double)((scaled_number & ((1 << data->bit_shift) - 1)) <<
				(8 - data->bit_shift)) + 0.5) / 255.0;
			scaled_number = scaled_number >> data->bit_shift;
			green = ((double)((scaled_number & ((1 << data->bit_shift) - 1)) <<
				(8 - data->bit_shift)) + 0.5) / 255.0;
			scaled_number = scaled_number >> data->bit_shift;
			red = ((double)((scaled_number & ((1 << data->bit_shift) - 1)) <<
				(8 - data->bit_shift)) + 0.5) / 255.0;

			glColor3f(red, green, blue);

#if defined (DEBUG_CODE)
			printf ( "%d %lf %lf %lf\n",  cm_information.number, red, green, blue);
#endif /* defined (DEBUG_CODE) */

			xi[0] = 0.0;
			xi[1] = 0.0;
			data->field_cache->setMeshLocation(element, xi);
			cmzn_field_evaluate_real(data->field, data->field_cache, /*number_of_values*/3, data->values);
			glVertex2dv(data->values);
			xi[0] = 1.0;
			xi[1] = 0.0;
			data->field_cache->setMeshLocation(element, xi);
			cmzn_field_evaluate_real(data->field, data->field_cache, /*number_of_values*/3, data->values);
			glVertex2dv(data->values);

			/* Only need to check the first dimension as this is only working for 2D */
			shape = get_FE_element_shape(element);
			if ((shape) &&
				get_FE_element_shape_xi_shape_type(shape,
				/*xi_number*/0,  &shape_type) &&
				(SIMPLEX_SHAPE == shape_type))
			{
				xi[0] = 0.0;
				xi[1] = 1.0;
				data->field_cache->setMeshLocation(element, xi);
				cmzn_field_evaluate_real(data->field, data->field_cache, /*number_of_values*/3, data->values);
				glVertex2dv(data->values);
				glVertex2dv(data->values);
			}
			else
			{
				xi[0] = 1.0;
				xi[1] = 1.0;
				data->field_cache->setMeshLocation(element, xi);
				cmzn_field_evaluate_real(data->field, data->field_cache, /*number_of_values*/3, data->values);
				glVertex2dv(data->values);

				xi[0] = 0.0;
				xi[1] = 1.0;
				data->field_cache->setMeshLocation(element, xi);
				cmzn_field_evaluate_real(data->field, data->field_cache, /*number_of_values*/3, data->values);
				glVertex2dv(data->values);
			}
		}
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Render_element_as_texture.  Missing Render_element_data.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Render_element_as_texture */
#endif /* defined (GRAPHICS_BUFFER_USE_OFFSCREEN_BUFFERS) */

int Computed_field_find_element_xi_special(struct Computed_field *field,
	cmzn_fieldcache_id field_cache,
	struct Computed_field_find_element_xi_cache **cache_ptr,
	const FE_value *values, int number_of_values, struct FE_element **element,
	FE_value *xi, cmzn_mesh_id search_mesh,
	Graphics_buffer_package *graphics_buffer_package,
	ZnReal *hint_minimums, ZnReal *hint_maximums, ZnReal *hint_resolution)
{
	int return_code;
#if defined (GRAPHICS_BUFFER_USE_OFFSCREEN_BUFFERS)
#define BLOCK_SIZE (20)
#if defined (DEBUG_CODE)
	int dummy[1024 * 1024];
#endif /* defined (DEBUG_CODE) */
	unsigned char *block_ptr, colour[4], colour_block[BLOCK_SIZE * BLOCK_SIZE *4],
		*next_colour;
	ZnReal ditherx, dithery;
	struct Computed_field_find_element_xi_graphics_cache *cache;
	struct Computed_field_iterative_find_element_xi_data find_element_xi_data;
	struct FE_element *first_element;
	struct Render_element_data data;
	int gl_list, i, nx, ny, px, py, scaled_number;
#endif /* defined (GRAPHICS_BUFFER_USE_OFFSCREEN_BUFFERS) */

	ENTER(Computed_field_find_element_xi_special);
	USE_PARAMETER(number_of_values);
#if !defined (GRAPHICS_BUFFER_USE_OFFSCREEN_BUFFERS)
	USE_PARAMETER(field);
	USE_PARAMETER(field_cache);
	USE_PARAMETER(cache_ptr);
	USE_PARAMETER(values);
	USE_PARAMETER(element);
	USE_PARAMETER(xi);
	USE_PARAMETER(search_mesh);
	USE_PARAMETER(graphics_buffer_package);
	USE_PARAMETER(hint_minimums);
	USE_PARAMETER(hint_maximums);
	USE_PARAMETER(hint_resolution);
#endif /* !defined (DM_BUFFERS) */

	return_code = 0;
#if defined (GRAPHICS_BUFFER_USE_OFFSCREEN_BUFFERS)
	/* If the number of elements in the group is small then there probably isn't
		any benefit to using this method */
	/* This method is adversely affected when displaying on a remote machine as every
		pixel grab requires transfer across the network */
	if (cache_ptr && hint_minimums && hint_maximums && hint_resolution &&
		((2 == cmzn_field_get_number_of_components(field)) ||
		((3 == cmzn_field_get_number_of_components(field)) &&
		(hint_resolution[2] == 1.0f))) && graphics_buffer_package && search_mesh
		/* This special case actually only works for 2D elements */
		&& (cmzn_mesh_get_dimension(search_mesh) == 2)
		&& ((cmzn_mesh_get_size(search_mesh) > 5))
		/*&& (Computed_field_is_find_element_xi_capable(field,NULL))*/)
	{
		find_element_xi_data.field_cache = field_cache;
		find_element_xi_data.field = field;
		find_element_xi_data.values = const_cast<FE_value *>(values);
		find_element_xi_data.number_of_values = number_of_values;
		find_element_xi_data.found_number_of_xi = 0;
		find_element_xi_data.found_derivatives = (FE_value *)NULL;
		find_element_xi_data.xi_tolerance = 1e-06;
		find_element_xi_data.find_nearest_location = 0; /* Find exact location */
		find_element_xi_data.nearest_element = (struct FE_element *)NULL;
		find_element_xi_data.nearest_element_distance_squared = 0.0;
		find_element_xi_data.start_with_data_xi = 0;
		find_element_xi_data.time = 0;
		if (ALLOCATE(find_element_xi_data.found_values, FE_value, number_of_values))
		{
			cache = (Computed_field_find_element_xi_graphics_cache*)NULL;
			if (*cache_ptr)
			{
				cache = dynamic_cast<Computed_field_find_element_xi_graphics_cache*>(
					(*cache_ptr)->cache_data);
			}
			if (!cache)
			{
				if (*cache_ptr)
				{
					DESTROY(Computed_field_find_element_xi_cache)(cache_ptr);
				}
				/* Get the element number range so we can spread the colour range
					as widely as possible */
				cache = new Computed_field_find_element_xi_graphics_cache;
				*cache_ptr = CREATE(Computed_field_find_element_xi_cache)(cache);
				cmzn_elementiterator_id iterator = cmzn_mesh_create_elementiterator(search_mesh);
				cmzn_element_id element = 0;
				element = cmzn_elementiterator_next(iterator);
				int identifier = cmzn_element_get_identifier(element);
				cache->maximum_element_number = identifier;
				cache->minimum_element_number = identifier;
				cmzn_element_destroy(&element);
				while (0 != (element = cmzn_elementiterator_next(iterator)))
				{
					identifier = cmzn_element_get_identifier(element);
					if (identifier > cache->maximum_element_number)
					{
						cache->maximum_element_number = identifier;
					}
					if (identifier < cache->minimum_element_number)
					{
						cache->minimum_element_number = identifier;
					}
					cmzn_element_destroy(&element);
				}
				cmzn_elementiterator_destroy(&iterator);

				cache->bit_shift = (int)ceil(log((double)(cache->maximum_element_number -
					cache->minimum_element_number + 2)) / log (2.0));
				cache->bit_shift = (cache->bit_shift / 3) + 1;
				/* 1024 is the limit on an Octane and the limit incorrectly reported by
					the O2 proxy query.  I didn't want to limit the CREATE(Dm_buffer) to
					1024 on an O2 so it doesn't check or handle limits properly so I do
					it here instead. */
				if (hint_resolution[0] > 1024.0)
				{
					hint_resolution[0] = 1024;
				}
				if (hint_resolution[1] > 1024.0)
				{
					hint_resolution[1] = 1024;
				}
				if ((cache->graphics_buffer = create_Graphics_buffer_offscreen(
						 graphics_buffer_package, (int)hint_resolution[0], (int)hint_resolution[1],
					GRAPHICS_BUFFER_ANY_BUFFERING_MODE, GRAPHICS_BUFFER_ANY_STEREO_MODE,
					/*minimum_colour_buffer_depth*/0, /*minimum_depth_buffer_depth*/0,
						 /*minimum_accumulation_buffer_depth*/0)))
				{
					data.field_cache = field_cache;
					data.field = field;
					data.bit_shift = cache->bit_shift;
					data.minimum_element_number = cache->minimum_element_number;
					data.maximum_element_number = cache->maximum_element_number;
					Graphics_buffer_make_current(cache->graphics_buffer);
					glClearColor(0.0, 0.0, 0.0, 0.0);
					glClear(GL_COLOR_BUFFER_BIT);

					glDisable(GL_ALPHA_TEST);
					glDisable(GL_DEPTH_TEST);
					glDisable(GL_SCISSOR_TEST);

					gl_list = glGenLists(1);
					glNewList(gl_list, GL_COMPILE);
					if (gl_list)
					{
						glBegin(GL_QUADS);
						cmzn_elementiterator_id iterator = cmzn_mesh_create_elementiterator(search_mesh);
						cmzn_element_id element = 0;
						while (0 != (element = cmzn_elementiterator_next(iterator)))
						{
							Render_element_as_texture(element, &data);
							cmzn_element_destroy(&element);
						}
						cmzn_elementiterator_destroy(&iterator);
						glEnd();

						glEndList();

						/* Dither things around a bit so that we get elements around the edges */
						ditherx = (hint_maximums[0] - hint_minimums[0]) / hint_resolution[0];
						dithery = (hint_maximums[1] - hint_minimums[1]) / hint_resolution[1];
						glLoadIdentity();
						glOrtho(hint_minimums[0] - ditherx, hint_maximums[0] - ditherx,
								  hint_minimums[1] - dithery, hint_maximums[1] - dithery,
								  -1.0, 1.0);
						glCallList(gl_list);
						glLoadIdentity();
						glOrtho(hint_minimums[0] + ditherx, hint_maximums[0] + ditherx,
								  hint_minimums[1] - dithery, hint_maximums[1] - dithery,
								  -1.0, 1.0);
						glCallList(gl_list);
						glLoadIdentity();
						glOrtho(hint_minimums[0] - ditherx, hint_maximums[0] - ditherx,
								  hint_minimums[1] + dithery, hint_maximums[1] + dithery,
								  -1.0, 1.0);
						glCallList(gl_list);
						glLoadIdentity();
						glOrtho(hint_minimums[0] + ditherx, hint_maximums[0] + ditherx,
								  hint_minimums[1] + dithery, hint_maximums[1] + dithery,
								  -1.0, 1.0);
						glCallList(gl_list);

						glLoadIdentity();
						glOrtho(hint_minimums[0], hint_maximums[0],
								  hint_minimums[1], hint_maximums[1],
								  -1.0, 1.0);
						glCallList(gl_list);

						glDeleteLists(gl_list, 1);

#if defined (DEBUG_CODE)
						glReadPixels(values[0] * hint_resolution[0],
							values[1] * hint_resolution[1],
							hint_resolution[0], hint_resolution[1], GL_RGBA, GL_UNSIGNED_BYTE,
							dummy);

						write_rgb_image_file("bob.rgb", 4, 1, hint_resolution[1],
							hint_resolution[0], 0, (long unsigned *)dummy);
#endif /* defined (DEBUG_CODE) */
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"CREATE(Computed_field_find_element_xi_special).  Unable to make display list.");
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"CREATE(Computed_field_find_element_xi_special).  Unable to create offscreen buffer.");
				}
			}
			if (cache->graphics_buffer)
			{
				Graphics_buffer_make_current(cache->graphics_buffer);
				px = (int)((values[0] - hint_minimums[0]) * hint_resolution[0] /
					(hint_maximums[0] - hint_minimums[0]));
				py = (int)((values[1] - hint_minimums[1]) * hint_resolution[1] /
					(hint_maximums[1] - hint_minimums[1]));
				glReadPixels(px, py, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, colour);
				scaled_number = (((unsigned int)colour[0] >> (8 - cache->bit_shift))
					<< (2 * cache->bit_shift)) +
					(((unsigned int)colour[1] >> (8 - cache->bit_shift))
						<< cache->bit_shift) +
					((unsigned int)colour[2] >> (8 - cache->bit_shift));
				if (scaled_number)
				{
					int identifier = scaled_number + cache->minimum_element_number - 1;
					if (0 != (*element = cmzn_mesh_find_element_by_identifier(search_mesh, identifier)))
					{
						first_element = *element;
#if defined (DEBUG_CODE)
						printf("First element %d\n", identifier);
#endif /* defined (DEBUG_CODE) */
						find_element_xi_data.xi_tolerance = 1e-06;
						if (Computed_field_iterative_element_conditional(*element, &find_element_xi_data))
						{
							for (i = 0 ; i < find_element_xi_data.found_number_of_xi ; i++)
							{
								xi[i] = find_element_xi_data.xi[i];
							}
						}
						else
						{
							*element = (struct FE_element *)NULL;
							/* Look in all the surrounding pixels for elements */
							if (px > BLOCK_SIZE / 2)
							{
								px -= BLOCK_SIZE / 2;
							}
							else
							{
								px = 0;
							}
							if (py > BLOCK_SIZE / 2)
							{
								py -= BLOCK_SIZE / 2;
							}
							else
							{
								py = 0;
							}
							if (px + BLOCK_SIZE < hint_resolution[0])
							{
								nx = BLOCK_SIZE;
							}
							else
							{
								nx = (int)(hint_resolution[0] - px);
							}
							if (py + BLOCK_SIZE < hint_resolution[1])
							{
								ny = BLOCK_SIZE;
							}
							else
							{
								ny = (int)(hint_resolution[1] - py);
							}
							glReadPixels(px, py, nx, ny, GL_RGBA, GL_UNSIGNED_BYTE,
								colour_block);

							while (scaled_number && (*element == (struct FE_element *)NULL))
							{
								/* each different colour represents an element to search in.
									 Following only checks each colour/value once and clears
									 that colour in the next loop. Continues until either an
									 element is found or all pixels in the block are black */
								block_ptr = colour_block + (nx * ny - 1)*4;
								next_colour = block_ptr;
								while (block_ptr >= colour_block)
								{
									if (block_ptr[0] || block_ptr[1] ||
										block_ptr[2] || block_ptr[3])
									{
										if ((block_ptr[0] == colour[0]) &&
											(block_ptr[1] == colour[1]) &&
											(block_ptr[2] == colour[2]) &&
											(block_ptr[3] == colour[3]))
										{
											block_ptr[0] = 0;
											block_ptr[1] = 0;
											block_ptr[2] = 0;
											block_ptr[3] = 0;
										}
										else
										{
											next_colour = block_ptr;
										}
									}
									block_ptr -= 4;
								}
								colour[0] = next_colour[0];
								colour[1] = next_colour[1];
								colour[2] = next_colour[2];
								colour[3] = next_colour[3];
								scaled_number = (((unsigned int)colour[0] >> (8 - cache->bit_shift))
									<< (2 * cache->bit_shift)) +
									(((unsigned int)colour[1] >> (8 - cache->bit_shift))
										<< cache->bit_shift) +
									((unsigned int)colour[2] >> (8 - cache->bit_shift));
								if (scaled_number)
								{
									identifier = scaled_number + cache->minimum_element_number - 1;
									if (0 != (*element = cmzn_mesh_find_element_by_identifier(search_mesh, identifier)))
									{
										if (Computed_field_iterative_element_conditional(
											*element, &find_element_xi_data))
										{
											for (i = 0 ; i < find_element_xi_data.found_number_of_xi ; i++)
											{
												xi[i] = find_element_xi_data.xi[i];
											}
										}
										else
										{
											*element = (struct FE_element *)NULL;
										}
									}
								}
							}

							if (! *element)
							{
								/* Revert to the originally found element and extrapolate if it close (to allow for boundaries) */
								find_element_xi_data.xi_tolerance = 1.0;
								if (Computed_field_iterative_element_conditional(
									first_element, &find_element_xi_data))
								{
									*element = first_element;
									for (i = 0 ; i < find_element_xi_data.found_number_of_xi ; i++)
									{
										xi[i] = find_element_xi_data.xi[i];
									}
								}
							}
						}
#if defined (DEBUG_CODE)
						if (*element)
						{
							printf("Final found element %d  %f %f %f\n", (*element)->cm.number,
								xi[0], xi[1], xi[2]);
						}
#endif /* defined (DEBUG_CODE) */
					}
				}
				else
				{
					*element = (struct FE_element *)NULL;
				}
				return_code = 1;

			}
			else
			{
				/* Do something else again */
			}
			DEALLOCATE(find_element_xi_data.found_values);
			if (find_element_xi_data.found_derivatives)
			{
				DEALLOCATE(find_element_xi_data.found_derivatives);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Computed_field_find_element_xi_special.  Unable to allocate value memory.");
			return_code = 0;
		}
	}
#endif /* defined (GRAPHICS_BUFFER_USE_OFFSCREEN_BUFFERS) */
	LEAVE;

	return (return_code);
} /* Computed_field_find_element_xi_special */
