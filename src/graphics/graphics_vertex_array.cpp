/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/**
 * C++ interfaces for graphics_vertex_array.cpp
 */
#include <iostream>
#include <map>
#include <stdlib.h>
#include <vector>
#include "general/compare.h"
#include "general/debug.h"
#include "graphics/auxiliary_graphics_types.h"
#include "graphics/graphics_vertex_array.hpp"
#include "general/indexed_list_private.h"
#include "general/message.h"
#include "general/mystring.h"

#define GRAPHICS_VERTEX_BUFFER_INITIAL_SIZE (50)

/*****************************************************************************//**
 * Holds the vertex buffer for a particular vertex_type.
*/
struct Graphics_vertex_buffer
{
	/** Number of vertices stored. */
	unsigned int vertex_count;
	/** Type of vertex. */
	Graphics_vertex_array_attribute_type type;
	/** Number of values per vertex */
	unsigned int values_per_vertex;
	/** Maximum number of vertices currently memory is allocated for. */
	unsigned int max_vertex_count;
	/** Vertex buffer memory */
	void *memory;
	/** Cmgui reference count. */
	int access_count;
};

struct Graphics_vertex_string_buffer
{
	/** Number of vertices stored. */
	std::vector<std::string> strings_vectors;
	unsigned int vertex_count;
	/** Number of values per vertex */
	unsigned int values_per_vertex;
};

DECLARE_LIST_TYPES(Graphics_vertex_buffer);

/*
Module functions
----------------
*/

PROTOTYPE_DEFAULT_DESTROY_OBJECT_FUNCTION(Graphics_vertex_buffer);
PROTOTYPE_OBJECT_FUNCTIONS(Graphics_vertex_buffer);
PROTOTYPE_LIST_FUNCTIONS(Graphics_vertex_buffer);
PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(Graphics_vertex_buffer,type,
	Graphics_vertex_array_attribute_type);
FULL_DECLARE_INDEXED_LIST_TYPE(Graphics_vertex_buffer);
DECLARE_OBJECT_FUNCTIONS(Graphics_vertex_buffer)
DECLARE_INDEXED_LIST_MODULE_FUNCTIONS(Graphics_vertex_buffer,type,
	Graphics_vertex_array_attribute_type,compare_int)
DECLARE_INDEXED_LIST_FUNCTIONS(Graphics_vertex_buffer)
DECLARE_FIND_BY_IDENTIFIER_IN_INDEXED_LIST_FUNCTION(Graphics_vertex_buffer,
	type,Graphics_vertex_array_attribute_type,compare_int)


/*****************************************************************************//**
 * Creates a new Graphics_vertex_buffer.  Initially no memory is allocated
 * and no vertices stored.
 *
 * @param type  Determines the format of this vertex buffers.
 * @return Newly created buffer.
 */
struct Graphics_vertex_buffer *CREATE(Graphics_vertex_buffer)(
	Graphics_vertex_array_attribute_type type, unsigned int values_per_vertex)
{
	struct Graphics_vertex_buffer *buffer;

	if (ALLOCATE(buffer, struct Graphics_vertex_buffer, 1))
	{
		buffer->type = type;
		buffer->values_per_vertex = values_per_vertex;
		buffer->max_vertex_count = 0;
		buffer->vertex_count = 0;
		buffer->memory = NULL;
		buffer->access_count = 0;
	}
	else
	{
		display_message(ERROR_MESSAGE,"CREATE(Graphics_vertex_buffer)  "
				"Unable to allocate buffer memory.");
		buffer = (struct Graphics_vertex_buffer *)NULL;
	}
	return (buffer);
}

/*****************************************************************************//**
 * Destroys a Graphics_vertex_buffer.
 *
 * @param buffer_address  Pointer to a buffer to be destroyed.
 * @return return_code. 1 for Success, 0 for failure.
*/
int DESTROY(Graphics_vertex_buffer)(
	struct Graphics_vertex_buffer **buffer_address)
{
	int return_code = 0;
	struct Graphics_vertex_buffer *buffer;
	if (buffer_address && (buffer = *buffer_address))
	{
		if (buffer->max_vertex_count && buffer->memory)
		{
			DEALLOCATE(buffer->memory);
		}
		DEALLOCATE(*buffer_address);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"DESTROY(Graphics_vertex_buffer)  "
			"Invalid object.");
	}
	return (return_code);
}


typedef std::map<Graphics_vertex_array_attribute_type, Graphics_vertex_string_buffer*> String_buffer_map;
typedef std::multimap<int , int> Fast_search_id_map;

class Graphics_vertex_array_internal
{
public:
	Graphics_vertex_array_type type;
	LIST(Graphics_vertex_buffer) *buffer_list;
	String_buffer_map string_buffer_list;
	/* fast search map for locating id for quick modification,
	 * this is implemented as multimap for graphics type that have varying number of primitives */
	Fast_search_id_map id_map;

	Graphics_vertex_array_internal(Graphics_vertex_array_type type)
		: type(type)
	{
		buffer_list = CREATE(LIST(Graphics_vertex_buffer))();
	}

	~Graphics_vertex_array_internal()
	{
		clear_string_buffer();
		DESTROY(LIST(Graphics_vertex_buffer))(&buffer_list);
	}

	void clear_string_buffer()
	{
		String_buffer_map::iterator pos;
		for (pos = string_buffer_list.begin(); pos != string_buffer_list.end(); ++pos)
		{
			Graphics_vertex_string_buffer *string_buffer = pos->second;
			delete string_buffer;
		}
		string_buffer_list.clear();
	}

	int add_fast_search_id(int object_id);

	int find_first_fast_search_id_location(int target_id);

	int get_all_fast_search_id_locations(int target_id,
		int *number_of_locations, int **locations);

	/** Gets the buffer appropriate for storing this vertex data or
	* creates one in this array if it doesn't already exist.
	* If it does exist but the value_per_vertex does not match then
	* the method return NULL.
	*/
	Graphics_vertex_buffer *get_or_create_vertex_buffer(
		Graphics_vertex_array_attribute_type vertex_type,
		unsigned int values_per_vertex);

	Graphics_vertex_string_buffer *get_or_create_string_buffer(
		Graphics_vertex_array_attribute_type vertex_type,
		unsigned int values_per_vertex);

	int get_string_buffer(
		Graphics_vertex_array_attribute_type vertex_buffer_type,
		std::string **string_buffer, unsigned int *values_per_vertex,
		unsigned int *vertex_count);

	/** Gets the buffer appropriate for storing this vertex data or
	* returns NULL.
	*/
	Graphics_vertex_buffer *get_vertex_buffer_for_attribute(
		Graphics_vertex_array_attribute_type vertex_type);

	Graphics_vertex_string_buffer *get_string_buffer_for_attribute(
		Graphics_vertex_array_attribute_type vertex_type);

	template <class value_type> int free_unused_buffer_memory( Graphics_vertex_array_attribute_type vertex_type, const value_type* dummy );

	template <class value_type> int add_attribute(
		Graphics_vertex_array_attribute_type vertex_type,
		const unsigned int values_per_vertex, const unsigned int number_of_values, const value_type *values);

	int add_string_attribute(
		Graphics_vertex_array_attribute_type vertex_type,
		const unsigned int values_per_vertex, const unsigned int number_of_values, std::string *values);

	template <class value_type> int replace_attribute(
		Graphics_vertex_array_attribute_type vertex_type,
		const unsigned int vertex_index,
		const unsigned int values_per_vertex, const unsigned int number_of_values, const value_type *values);

	template <class value_type> int get_attribute(
		Graphics_vertex_array_attribute_type vertex_type,
		unsigned int vertex_index,
		unsigned int number_of_values, value_type *values);

	template <class value_type> int get_vertex_buffer(
		Graphics_vertex_array_attribute_type vertex_buffer_type,
		value_type **vertex_buffer, unsigned int *values_per_vertex,
		unsigned int *vertex_count);

};


Graphics_vertex_array::Graphics_vertex_array(Graphics_vertex_array_type type)
{
	internal = new Graphics_vertex_array_internal(type);
}

int Graphics_vertex_array_internal::add_fast_search_id(int object_id)
{
	const int current_location = static_cast<int>(id_map.size());
	id_map.insert(std::make_pair(object_id, current_location));
	return 1;
}

int Graphics_vertex_array_internal::find_first_fast_search_id_location(int target_id)
{
	int location = -1;
	Fast_search_id_map::iterator pos;
	pos = id_map.find(target_id);
	if (pos != id_map.end())
	{
		location = pos->second;
	}
	return location;
}

int Graphics_vertex_array_internal::get_all_fast_search_id_locations(int target_id,
	int *number_of_locations, int **locations)
{
	*number_of_locations = static_cast<int>(id_map.count(target_id));
	if (*number_of_locations > 0)
	{
		int current_location = 0;
		*locations = new int[*number_of_locations];
		Fast_search_id_map::iterator pos;
		for (pos = id_map.lower_bound(target_id); pos != id_map.upper_bound(target_id); ++pos)
		{
			(*locations)[current_location] = pos->second;
			current_location++;
		}
	}
	return 1;
}

Graphics_vertex_string_buffer *Graphics_vertex_array_internal::get_or_create_string_buffer(
	Graphics_vertex_array_attribute_type vertex_type,
	unsigned int values_per_vertex)
{
	Graphics_vertex_array_attribute_type vertex_buffer_type = GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_LABEL;
	Graphics_vertex_string_buffer *buffer = 0;

	switch (type)
	{
		case GRAPHICS_VERTEX_ARRAY_TYPE_FLOAT_SEPARATE_DRAW_ARRAYS:
		{
			vertex_buffer_type = vertex_type;
		} break;
	}
	String_buffer_map::iterator pos;
	pos = string_buffer_list.find(vertex_buffer_type);
	if (pos != string_buffer_list.end())
	{
		buffer = pos->second;
	}
	if (buffer)
	{
		if (buffer->values_per_vertex != values_per_vertex)
		{
			buffer = (Graphics_vertex_string_buffer *)NULL;
		}
	}
	else
	{
		buffer = new Graphics_vertex_string_buffer;
		buffer->vertex_count = 0;
		buffer->values_per_vertex = values_per_vertex;
		string_buffer_list.insert(std::make_pair(vertex_buffer_type, buffer));
	}
	return (buffer);
}

Graphics_vertex_string_buffer *Graphics_vertex_array_internal::get_string_buffer_for_attribute(
	Graphics_vertex_array_attribute_type vertex_type)
{
	Graphics_vertex_array_attribute_type vertex_buffer_type = GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_LABEL;
	Graphics_vertex_string_buffer *buffer = 0;

	switch (type)
	{
		case GRAPHICS_VERTEX_ARRAY_TYPE_FLOAT_SEPARATE_DRAW_ARRAYS:
		{
			vertex_buffer_type = vertex_type;
		} break;
	}

	String_buffer_map::iterator pos;
	pos = string_buffer_list.find(vertex_buffer_type);
	if (pos != string_buffer_list.end())
	{
		buffer = pos->second;
	}

	return (buffer);
}

int Graphics_vertex_array_internal::get_string_buffer(
		Graphics_vertex_array_attribute_type vertex_buffer_type,
		std::string **string_buffer, unsigned int *values_per_vertex,
		unsigned int *vertex_count)
{
	Graphics_vertex_string_buffer *buffer;
	int return_code;
	buffer = get_string_buffer_for_attribute(vertex_buffer_type);
	if (buffer)
	{
		*string_buffer = &(buffer->strings_vectors[0]);
		*values_per_vertex = buffer->values_per_vertex;
		*vertex_count = buffer->vertex_count;
		return_code = 1;
	}
	else
	{
		*string_buffer = 0;
		*values_per_vertex = 0;
		*vertex_count = 0;
		return_code = 0;
	}

	return return_code;
}

int Graphics_vertex_array_internal::add_string_attribute(
	Graphics_vertex_array_attribute_type vertex_type,
	const unsigned int values_per_vertex, const unsigned int number_of_values, std::string *values)
{
	int return_code = 1;
	Graphics_vertex_string_buffer *buffer = get_or_create_string_buffer(vertex_type, values_per_vertex);
	if (buffer)
	{
		if (buffer->strings_vectors.capacity() <= ( GRAPHICS_VERTEX_BUFFER_INITIAL_SIZE + number_of_values ) * values_per_vertex)
		{
			buffer->strings_vectors.reserve(( GRAPHICS_VERTEX_BUFFER_INITIAL_SIZE + number_of_values ) * values_per_vertex);
		}
		if (buffer->strings_vectors.capacity() <= ( buffer->vertex_count + number_of_values ) * values_per_vertex)
		{
			buffer->strings_vectors.reserve(2 * (buffer->strings_vectors.capacity() + number_of_values * values_per_vertex));
		}
		if (return_code)
		{
			int total_number = values_per_vertex * number_of_values;
			for (int i = 0; i < total_number; i++)
				buffer->strings_vectors.push_back(values[i]);
			buffer->vertex_count += number_of_values;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Graphics_vertex_array::add_attribute.  "
			"Unable to create buffer.");
		return_code = 0;
	}

	return (return_code);
}

Graphics_vertex_buffer *Graphics_vertex_array_internal::get_or_create_vertex_buffer(
	Graphics_vertex_array_attribute_type vertex_type,
	unsigned int values_per_vertex)
{
	Graphics_vertex_array_attribute_type vertex_buffer_type = GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION;
	Graphics_vertex_buffer *buffer;

	switch (type)
	{
		case GRAPHICS_VERTEX_ARRAY_TYPE_FLOAT_SEPARATE_DRAW_ARRAYS:
		{
			vertex_buffer_type = vertex_type;
		} break;
	}
	buffer = FIND_BY_IDENTIFIER_IN_LIST(Graphics_vertex_buffer,type)
		 (vertex_buffer_type, buffer_list);
	if (buffer)
	{
		if (buffer->values_per_vertex != values_per_vertex)
		{
			buffer = (Graphics_vertex_buffer *)NULL;
		}
	}
	else
	{
		buffer = CREATE(Graphics_vertex_buffer)(vertex_buffer_type,
			values_per_vertex);
		if (buffer)
		{
			if (!ADD_OBJECT_TO_LIST(Graphics_vertex_buffer)(buffer,
				buffer_list))
			{
				DESTROY(Graphics_vertex_buffer)(&buffer);
			}
		}
	}
	return (buffer);
}

Graphics_vertex_buffer *Graphics_vertex_array_internal::get_vertex_buffer_for_attribute(
	Graphics_vertex_array_attribute_type vertex_type)
{
	Graphics_vertex_array_attribute_type vertex_buffer_type = GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION;
	Graphics_vertex_buffer *buffer = 0;

   switch (type)
   {
		case GRAPHICS_VERTEX_ARRAY_TYPE_FLOAT_SEPARATE_DRAW_ARRAYS:
		{
			vertex_buffer_type = vertex_type;
		} break;
	}
	buffer = FIND_BY_IDENTIFIER_IN_LIST(Graphics_vertex_buffer,type)
		(vertex_buffer_type, buffer_list);

	return (buffer);
}

template <class value_type> int Graphics_vertex_array_internal::add_attribute(
	Graphics_vertex_array_attribute_type vertex_type,
	const unsigned int values_per_vertex, const unsigned int number_of_values, const value_type *values)
{
	int return_code = 1;
	Graphics_vertex_buffer *buffer;

	buffer = get_or_create_vertex_buffer(vertex_type, values_per_vertex);
	if (buffer)
	{
		Graphics_vertex_array_attribute_type vertex_buffer_type = buffer->type;
		if (!buffer->memory)
		{
		// Allocate enough memory for what I am about to add plus some headroom
			if (ALLOCATE(buffer->memory, value_type,
				( GRAPHICS_VERTEX_BUFFER_INITIAL_SIZE + number_of_values ) * values_per_vertex))
			{
				buffer->max_vertex_count = GRAPHICS_VERTEX_BUFFER_INITIAL_SIZE;
			}
			else
			{
				return_code = 0;
			}
		}
		if (return_code)
		{
			if (buffer->max_vertex_count <= ( buffer->vertex_count + number_of_values ) )
			{
				// Reallocate enough memory for what I am about to add plus some headroom
				if (REALLOCATE(buffer->memory, buffer->memory, value_type,
					( 2 * buffer->max_vertex_count + number_of_values ) * values_per_vertex))
				{
					buffer->max_vertex_count = 2 * buffer->max_vertex_count + number_of_values;
				}
				else
				{
					return_code = 0;
				}
			}
		}
		if (return_code)
		{
			if (vertex_buffer_type == vertex_type)
			{
				memcpy((value_type*)buffer->memory + buffer->vertex_count * values_per_vertex,
					values, values_per_vertex * number_of_values * sizeof(value_type));
				buffer->vertex_count += number_of_values;
			}
			else
			{
				display_message(ERROR_MESSAGE,"Graphics_vertex_array::add_attribute.  "
					"Storage for this combination of vertex_buffer and vertex not implemented yet.");
				return_code = 0;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"Graphics_vertex_array::add_attribute.  "
			"Unable to create buffer.");
		return_code = 0;
	}

	return (return_code);
}

template <class value_type> int Graphics_vertex_array_internal::replace_attribute(
	Graphics_vertex_array_attribute_type vertex_type,
	const unsigned int vertex_index,
	const unsigned int values_per_vertex, const unsigned int number_of_values, const value_type *values)
{
	Graphics_vertex_buffer *buffer;

	buffer = get_or_create_vertex_buffer(vertex_type, values_per_vertex);
	if (buffer)
	{
		Graphics_vertex_array_attribute_type vertex_buffer_type = buffer->type;
		if (!buffer->memory)
		{
			return 0;
		}

		if ((buffer->vertex_count > vertex_index) &&
			((buffer->vertex_count - vertex_index) >= number_of_values) &&
			values_per_vertex == buffer->values_per_vertex &&
			vertex_buffer_type == vertex_type)
		{
			memcpy((value_type*)buffer->memory + vertex_index * values_per_vertex,
				values, values_per_vertex * number_of_values * sizeof(value_type));
			return 1;
		}
	}

	return 0;
}

template <class value_type> int Graphics_vertex_array_internal::get_attribute(
	Graphics_vertex_array_attribute_type vertex_type,
	unsigned int vertex_index,
	unsigned int number_of_values, value_type *values)
{
	Graphics_vertex_buffer *buffer;
	int return_code = 0;

	buffer = get_vertex_buffer_for_attribute(vertex_type);
	if (buffer)
	{
		Graphics_vertex_array_attribute_type vertex_buffer_type = buffer->type;
		if (buffer->values_per_vertex == number_of_values)
		{
			if (vertex_buffer_type == vertex_type)
			{
				memcpy(values, (value_type*) buffer->memory + vertex_index
					* buffer->values_per_vertex, buffer->values_per_vertex
					* sizeof(value_type));
				return_code = 1;
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
}

template <class value_type> int Graphics_vertex_array_internal::get_vertex_buffer(
		Graphics_vertex_array_attribute_type vertex_buffer_type,
		value_type **vertex_buffer, unsigned int *values_per_vertex,
		unsigned int *vertex_count)
{
	Graphics_vertex_buffer *buffer;
	int return_code;

	buffer = get_vertex_buffer_for_attribute(vertex_buffer_type);
	if (buffer)
	{
		*vertex_buffer = static_cast<value_type*>(buffer->memory);
		*values_per_vertex = buffer->values_per_vertex;
		*vertex_count = buffer->vertex_count;
		return_code = 1;
	}
	else
	{
		*vertex_buffer = 0;
		*values_per_vertex = 0;
		*vertex_count = 0;
		return_code = 0;
	}

	return return_code;
}

template <class value_type> int Graphics_vertex_array_internal::free_unused_buffer_memory(
	Graphics_vertex_array_attribute_type vertex_type, const value_type *dummy )
{
	int return_code = 0;
	Graphics_vertex_buffer *buffer = get_vertex_buffer_for_attribute(vertex_type);
	if (buffer)
	{
		if (REALLOCATE(buffer->memory, buffer->memory, value_type,
				(buffer->vertex_count  * buffer->values_per_vertex)))
		{
			return_code = 1;
			buffer->max_vertex_count = buffer->vertex_count;
		}
	}

	return return_code;
}

int Graphics_vertex_array::free_unused_buffer_memory(
	Graphics_vertex_array_attribute_type vertex_type )
{
	USE_PARAMETER(vertex_type);
	return 0;//internal->free_unused_buffer_memory( vertex_type );
}
/*
int Graphics_vertex_array::add_float_attribute(
	Graphics_vertex_array_attribute_type vertex_type,
	unsigned int values_per_vertex, unsigned int number_of_values, ZnReal *values)
{
	return internal->add_attribute(vertex_type, values_per_vertex, number_of_values, values);
}
*/
int Graphics_vertex_array::add_float_attribute(
	Graphics_vertex_array_attribute_type vertex_type,
	const unsigned int values_per_vertex, const unsigned int number_of_values, const GLfloat *values)
{
	return internal->add_attribute(vertex_type, values_per_vertex, number_of_values, values);
}


int Graphics_vertex_array::get_float_vertex_buffer(
		Graphics_vertex_array_attribute_type vertex_type,
		GLfloat **vertex_buffer, unsigned int *values_per_vertex,
		unsigned int *vertex_count)
{
	return internal->get_vertex_buffer(vertex_type,
		vertex_buffer, values_per_vertex, vertex_count);
}

int Graphics_vertex_array::add_string_attribute(Graphics_vertex_array_attribute_type vertex_type,
	const unsigned int values_per_vertex, const unsigned int number_of_values, std::string *values)
{
	return internal->add_string_attribute(vertex_type, values_per_vertex, number_of_values, values);
}

int Graphics_vertex_array::get_string_vertex_buffer(
	Graphics_vertex_array_attribute_type vertex_type,
	std::string **vertex_buffer, unsigned int *values_per_vertex,
	unsigned int *vertex_count)
{
	return internal->get_string_buffer(vertex_type, vertex_buffer, values_per_vertex, vertex_count);
}


int Graphics_vertex_array::replace_float_vertex_buffer_at_position(
	Graphics_vertex_array_attribute_type vertex_type,
	const unsigned int vertex_index,	const unsigned int values_per_vertex,
	const unsigned int number_of_values, const GLfloat *values)
{
	return internal->replace_attribute(vertex_type,
		vertex_index, values_per_vertex, number_of_values, values);
}

int Graphics_vertex_array::add_unsigned_integer_attribute(
		Graphics_vertex_array_attribute_type vertex_type,
		const unsigned int values_per_vertex, const unsigned int number_of_values, const unsigned int *values)
{
	return internal->add_attribute(vertex_type, values_per_vertex, number_of_values, values);
}

int Graphics_vertex_array::get_unsigned_integer_attribute(
		Graphics_vertex_array_attribute_type vertex_type,
		unsigned int vertex_index,
		unsigned int number_of_values, unsigned int *values)
{
	return internal->get_attribute(vertex_type,
		vertex_index, number_of_values, values);
}

int Graphics_vertex_array::get_unsigned_integer_vertex_buffer(
		Graphics_vertex_array_attribute_type vertex_buffer_type,
		unsigned int **vertex_buffer, unsigned int *values_per_vertex,
		unsigned int *vertex_count)
{
	return internal->get_vertex_buffer(vertex_buffer_type,
		vertex_buffer, values_per_vertex, vertex_count);
}

int Graphics_vertex_array::add_integer_attribute(
		Graphics_vertex_array_attribute_type vertex_type,
		const unsigned int values_per_vertex, const unsigned int number_of_values, const int *values)
{
	return internal->add_attribute(vertex_type, values_per_vertex, number_of_values, values);
}

int Graphics_vertex_array::get_integer_attribute(
		Graphics_vertex_array_attribute_type vertex_type,
		unsigned int vertex_index,
		unsigned int number_of_values, int *values)
{
	return internal->get_attribute(vertex_type,
		vertex_index, number_of_values, values);
}

int Graphics_vertex_array::replace_integer_vertex_buffer_at_position(
	Graphics_vertex_array_attribute_type vertex_type,
	const unsigned int vertex_index,	const unsigned int values_per_vertex,
	const unsigned int number_of_values, const int *values)
{
	return internal->replace_attribute(vertex_type,
		vertex_index, values_per_vertex, number_of_values, values);
}

int Graphics_vertex_array::get_integer_vertex_buffer(
		Graphics_vertex_array_attribute_type vertex_buffer_type,
		int **integer_buffer, unsigned int *values_per_vertex,
		unsigned int *vertex_count)
{
	return internal->get_vertex_buffer(vertex_buffer_type,
		integer_buffer, values_per_vertex, vertex_count);
}

unsigned int Graphics_vertex_array::get_number_of_vertices(
	Graphics_vertex_array_attribute_type vertex_buffer_type)
{
	Graphics_vertex_buffer *buffer;
	unsigned int vertex_count;
	buffer = internal->get_vertex_buffer_for_attribute(vertex_buffer_type);
	if (buffer)
	{
		vertex_count = buffer->vertex_count;
	}
	else
	{
		vertex_count = 0;
	}

	return vertex_count;
}

int Graphics_vertex_array::find_first_location_of_integer_value(
	enum Graphics_vertex_array_attribute_type vertex_type, int value)
{
	int *value_buffer = 0;

	unsigned int values_per_vertex = 0, vertex_count = 0;
	if (get_integer_vertex_buffer(vertex_type, &value_buffer, &values_per_vertex,
			&vertex_count) &&  value_buffer && vertex_count)
	{
		for (unsigned int i = 0; i < vertex_count; i++)
		{
			if (value_buffer[i] == value)
			{
				return (int)i;
			}
		}
	}
	return -1;
}

void Graphics_vertex_array::fill_element_index(
	unsigned vertex_start, unsigned int number_of_xi1, unsigned int number_of_xi2,
	enum Graphics_vertex_array_shape_type shape_type)
{
	unsigned int last_entry = get_number_of_vertices(
		GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_STRIP_START);
	unsigned int count_before_last = 0;
	unsigned int last_count = 0;
	unsigned int last_number_of_strips = 0;
	if (last_entry > 0)
	{
		get_unsigned_integer_attribute(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_STRIP_START,
			last_entry - 1,	1, &count_before_last);
		get_unsigned_integer_attribute(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_STRIPS,
			last_entry - 1,	1, &last_number_of_strips);
		last_count = count_before_last + last_number_of_strips;
	}
	add_unsigned_integer_attribute(
		GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_STRIP_START,
		1, 1, &last_count);
	unsigned int number_of_strip_index_entries = get_number_of_vertices(
		GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_STRIP_INDEX_START);
	unsigned int points_per_strip = 0, index_start_for_strip = 0;
	if (number_of_strip_index_entries > 0)
	{
		get_unsigned_integer_attribute(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_STRIP_INDEX_START,
			number_of_strip_index_entries - 1,	1, &index_start_for_strip);
		get_unsigned_integer_attribute(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_POINTS_FOR_STRIP,
			number_of_strip_index_entries - 1,	1, &points_per_strip);
		index_start_for_strip += points_per_strip;
	}
	if (ARRAY_SHAPE_TYPE_SIMPLEX == shape_type)
	{
		unsigned int number_of_strips = number_of_xi1 - 1;
		add_unsigned_integer_attribute(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_STRIPS,
			1, 1, &number_of_strips);
		unsigned int index;
		for (unsigned int i = 0; i < number_of_strips; i++)
		{
			index = i;
			points_per_strip = (number_of_xi1 - i)*2 - 1;
			unsigned int current_index = 0;
			for (unsigned int j = 0; j < points_per_strip; j++)
			{
				current_index = index + vertex_start;
				if (j & 1)
				{
					index += (number_of_strips - (j >> 1));
				}
				else
				{
					index++;
				}
				add_unsigned_integer_attribute(
					GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_STRIP_INDEX_ARRAY,
					1, 1, &current_index);
			}
			add_unsigned_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_STRIP_INDEX_START,
				1, 1, &index_start_for_strip);
			add_unsigned_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_POINTS_FOR_STRIP,
				1, 1, &points_per_strip);
			index_start_for_strip += points_per_strip;
		}
	}
	else
	{
		unsigned int number_of_strips = number_of_xi1 - 1;
		add_unsigned_integer_attribute(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_STRIPS,
			1, 1, &number_of_strips);
		points_per_strip = 2 * number_of_xi2;
		unsigned int index;
		if (points_per_strip > 0)
		{
			for (unsigned int i = 0; i < number_of_strips; i++)
			{
				index = i;
				unsigned int current_index = 0;
				for (unsigned int j = 0; j < points_per_strip; j++)
				{
					current_index = index + vertex_start;
					if (j & 1)
						index += number_of_strips;
					else
						index++;
					add_unsigned_integer_attribute(
						GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_STRIP_INDEX_ARRAY,
						1, 1, &current_index);
				}
				add_unsigned_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_STRIP_INDEX_START,
					1, 1, &index_start_for_strip);
				add_unsigned_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_POINTS_FOR_STRIP,
					1, 1, &points_per_strip);
				index_start_for_strip += points_per_strip;
			}
		}
	}
}


/*****************************************************************************//**
 * Resets the number of vertices defined in the buffer to zero.  Does not actually
 * reset the allocated memory to zero as it is anticipated that the buffer will
 * recreated.
 *
 * @param buffer  Buffer to be cleared.
 * @return return_code. 1 for Success, 0 for failure.
*/
int Graphics_vertex_buffer_clear(
	struct Graphics_vertex_buffer *buffer, void *user_data_dummy)
{
	int return_code;
	USE_PARAMETER(user_data_dummy);

	if (buffer)
	{
		buffer->vertex_count = 0;
	}
	return_code = 1;

	return (return_code);
}

int Graphics_vertex_array::add_fast_search_id(int object_id)
{
	internal->add_fast_search_id(object_id);
	return 1;
}

int Graphics_vertex_array::find_first_fast_search_id_location(
	 int target_id)
{
	return internal->find_first_fast_search_id_location(target_id);
}

int Graphics_vertex_array::get_all_fast_search_id_locations(int target_id,
	int *number_of_locations, int **locations)
{
	return internal->get_all_fast_search_id_locations(target_id, number_of_locations, locations);
}

int Graphics_vertex_array::clear_buffers()
{
	internal->clear_string_buffer();
	return FOR_EACH_OBJECT_IN_LIST(Graphics_vertex_buffer)(
		Graphics_vertex_buffer_clear, NULL, internal->buffer_list);
}

int Graphics_vertex_array::clear_specified_buffer(Graphics_vertex_array_attribute_type vertex_type)
{
	Graphics_vertex_buffer *buffer = FIND_BY_IDENTIFIER_IN_LIST(Graphics_vertex_buffer,type)
		 (vertex_type, internal->buffer_list);
	if (buffer)
		return Graphics_vertex_buffer_clear(buffer, 0);
	return 1;
}

Graphics_vertex_array::~Graphics_vertex_array()
{
	delete internal;
}

int fill_glyph_graphics_vertex_array(struct Graphics_vertex_array *array, int vertex_location,
	unsigned int number_of_points, Triple *point_list, Triple *axis1_list, Triple *axis2_list,
	Triple *axis3_list, Triple *scale_list,	int n_data_components, GLfloat *data,
	Triple *label_density_list, int object_name, int *names, char **labels,
	int label_bounds_values, int label_bounds_components, ZnReal *label_bounds)
{
	if (array)
	{
		if (vertex_location < 0)
		{
			unsigned int vertex_start = array->get_number_of_vertices(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION);
			array->add_unsigned_integer_attribute(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
				1, 1, &number_of_points);
			array->add_unsigned_integer_attribute(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
				1, 1, &vertex_start);
			Triple *points = point_list, *axis1s = axis1_list, *axis2s = axis2_list,
				*axis3s = axis3_list, *scales = scale_list, *label_densities = label_density_list;
			GLfloat floatValue[3];
			GLfloat *labelBoundsFloatValue = 0;
			int label_bounds_per_points = label_bounds_components * label_bounds_values;
			if (label_bounds_per_points > 0)
			{
				labelBoundsFloatValue = new GLfloat[label_bounds_per_points];
			}
			for (unsigned int i=0;i<number_of_points;i++)
			{
				if (points)
				{
					CAST_TO_OTHER(floatValue,(*points),GLfloat,3);
					array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
						3, 1, floatValue);
					points++;
				}
				if (axis1s)
				{
					CAST_TO_OTHER(floatValue,(*axis1s),GLfloat,3);
					array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_AXIS1,
						3, 1, floatValue);
					axis1s++;
				}
				if (axis2s)
				{
					CAST_TO_OTHER(floatValue,(*axis2s),GLfloat,3);
					array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_AXIS2,
						3, 1, floatValue);
					axis2s++;
				}
				if (axis3s)
				{
					CAST_TO_OTHER(floatValue,(*axis3s),GLfloat,3);
					array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_AXIS3,
						3, 1, floatValue);
					axis3s++;
				}
				if (scales)
				{
					CAST_TO_OTHER(floatValue,(*scales),GLfloat,3);
					array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_SCALE,
						3, 1, floatValue);
					scales++;
				}
				if (label_densities)
				{
					CAST_TO_OTHER(floatValue,(*label_densities),GLfloat,3);
					array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_LABEL_DENSITY,
						3, 1, floatValue);
					label_densities++;
				}
				if (label_bounds)
				{
					CAST_TO_OTHER(labelBoundsFloatValue,label_bounds,GLfloat,label_bounds_per_points);
					array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_LABEL_BOUND,
						label_bounds_per_points, 1, labelBoundsFloatValue);
					label_bounds += label_bounds_per_points;
				}
			}
			if (labelBoundsFloatValue)
				delete[] labelBoundsFloatValue;
			if (labels && number_of_points)
			{
				std::string *labels_string = new std::string[number_of_points];
				for (unsigned int i=0;i<number_of_points;i++)
				{
					if (labels[i] == 0)
						labels_string[i] = std::string("");
					else
						labels_string[i] = std::string(labels[i]);
				}
				array->add_string_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_LABEL,
					1, number_of_points, labels_string);
				delete[] labels_string;
			}
			if (names)
			{
				array->add_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_VERTEX_ID,
					1, number_of_points, names);
			}
			array->add_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_OBJECT_ID,
				1, 1, &object_name);
			array->add_fast_search_id(object_name);
			int modificationRequired = 0;
			array->add_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_UPDATE_REQUIRED,
				1, 1, &modificationRequired);
			if (data)
			{
				array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
					n_data_components, number_of_points, data);
			}
		}
		else
		{
			unsigned int vertex_start = array->get_unsigned_integer_attribute(
				GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
				vertex_location, 1, &vertex_start);
			Triple *points = point_list, *axis1s = axis1_list, *axis2s = axis2_list,
				*axis3s = axis3_list, *scales = scale_list, *label_densities = label_density_list;
			GLfloat floatValue[3];
			GLfloat *labelBoundsFloatValue = 0;
			int label_bounds_per_points = label_bounds_components * label_bounds_values;
			if (label_bounds_per_points > 0)
			{
				labelBoundsFloatValue = new GLfloat[label_bounds_per_points];
			}
			for (unsigned int i=0;i<number_of_points;i++)
			{
				if (points)
				{
					CAST_TO_OTHER(floatValue,(*points),GLfloat,3);
					array->replace_float_vertex_buffer_at_position(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
						vertex_start + i, 3, 1, floatValue);
					points++;
				}
				if (axis1s)
				{
					CAST_TO_OTHER(floatValue,(*axis1s),GLfloat,3);
					array->replace_float_vertex_buffer_at_position(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_AXIS1,
						vertex_start + i, 3, 1, floatValue);
					axis1s++;
				}
				if (axis2s)
				{
					CAST_TO_OTHER(floatValue,(*axis2s),GLfloat,3);
					array->replace_float_vertex_buffer_at_position(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_AXIS2,
						vertex_start + i, 3, 1, floatValue);
					axis2s++;
				}
				if (axis3s)
				{
					CAST_TO_OTHER(floatValue,(*axis3s),GLfloat,3);
					array->replace_float_vertex_buffer_at_position(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_AXIS3,
						vertex_start + i, 3, 1, floatValue);
					axis3s++;
				}
				if (scales)
				{
					CAST_TO_OTHER(floatValue,(*scales),GLfloat,3);
					array->replace_float_vertex_buffer_at_position(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_SCALE,
						vertex_start + i, 3, 1, floatValue);
					scales++;
				}
				if (label_densities)
				{
					CAST_TO_OTHER(floatValue,(*label_densities),GLfloat,3);
					array->replace_float_vertex_buffer_at_position(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_LABEL_DENSITY,
						vertex_start + i, 3, 1, floatValue);
					label_densities++;
				}
				if (label_bounds)
				{
					CAST_TO_OTHER(labelBoundsFloatValue,label_bounds,GLfloat,label_bounds_per_points);
					array->replace_float_vertex_buffer_at_position(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_LABEL_BOUND,
						vertex_start + i, label_bounds_per_points, 1, labelBoundsFloatValue);
					label_bounds += label_bounds_per_points;
				}
			}
			if (labelBoundsFloatValue)
				delete[] labelBoundsFloatValue;
			if (names)
			{
				array->replace_integer_vertex_buffer_at_position(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_VERTEX_ID,
					vertex_start, 1, number_of_points, names);
			}
			if (data)
			{
				array->replace_float_vertex_buffer_at_position(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
					vertex_start, n_data_components, number_of_points, data);
			}
		}
		return 1;
	}
	else
	{
		return 0;
	}
}

int fill_line_graphics_vertex_array(struct Graphics_vertex_array *array,
	unsigned int n_pts,Triple *pointlist,Triple *normallist,	int n_data_components, GLfloat *data)
{
	if (array)
	{
		unsigned int vertex_start = array->get_number_of_vertices(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION);
		array->add_unsigned_integer_attribute(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
			1, 1, &n_pts);
		array->add_unsigned_integer_attribute(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
			1, 1, &vertex_start);
		GLfloat floatValue[3];
		Triple *points = pointlist, *normals = normallist;
		for (unsigned int i=0;i < n_pts;i++)
		{
			if (points)
			{
				CAST_TO_OTHER(floatValue,(*points),GLfloat,3);
				array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
					3, 1, floatValue);
				points++;
			}
			if (normals)
			{
				CAST_TO_OTHER(floatValue,(*normals),GLfloat,3);
				array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
					3, 1, floatValue);
				normals++;
			}
		}
		if (data)
		{
			array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
				n_data_components, n_pts, data);
		}
		return 1;
	}
	else
	{
		return 0;
	}
}

int fill_pointset_graphics_vertex_array(struct Graphics_vertex_array *array,
	unsigned int n_pts,Triple *pointlist, char **text, int n_data_components, GLfloat *data)
{
	if (array)
	{
		unsigned int vertex_start = array->get_number_of_vertices(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION);
		array->add_unsigned_integer_attribute(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
			1, 1, &n_pts);
		array->add_unsigned_integer_attribute(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
			1, 1, &vertex_start);
		GLfloat floatValue[3];
		Triple *points = pointlist;
		for (unsigned int i=0;i<n_pts;i++)
		{
			if (points)
			{
				CAST_TO_OTHER(floatValue,(*points),GLfloat,3);
				array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
					3, 1, floatValue);
				points++;
			}
		}
		if (text && n_pts)
		{
			std::string *labels_string = new std::string[n_pts];
			for (unsigned int i=0;i<n_pts;i++)
			{
				if (text[i] == 0)
					labels_string[i] = std::string("");
				else
					labels_string[i] = std::string(text[i]);
			}
			array->add_string_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_LABEL,
				1, n_pts, labels_string);
			delete[] labels_string;
		}
		if (data)
		{
			array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
				n_data_components, n_pts, data);
		}
		return 1;
	}
	else
	{
		return 0;
	}

}

int fill_surface_graphics_vertex_array(struct Graphics_vertex_array *array,
	gtPolygonType polytype, unsigned int n_pts1, unsigned int n_pts2,
	Triple *pointlist, Triple *normallist, Triple *tangentlist,
	Triple *texturelist, int n_data_components,GLfloat *data)
{
	if (array)
	{
		int polygonType = (int)polytype;
		unsigned int vertex_start = array->get_number_of_vertices(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION);
		unsigned int number_of_points = n_pts1 * n_pts2;
		array->add_unsigned_integer_attribute(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
			1, 1, &number_of_points);
		array->add_unsigned_integer_attribute(
			GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
			1, 1, &vertex_start);
		array->add_unsigned_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_XI1,
			1, 1, &n_pts1);
		array->add_unsigned_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_XI2,
			1, 1, &n_pts2);
		GLfloat floatValue[3];
		Triple *points = pointlist, *normals = normallist, *tangents = tangentlist, *textures = texturelist;
		for (unsigned int i=0;i<number_of_points;i++)
		{
			if (points)
			{
				CAST_TO_OTHER(floatValue,(*points),GLfloat,3);
				array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
					3, 1, floatValue);
				points++;
			}
			if (normals)
			{
				CAST_TO_OTHER(floatValue,(*normals),GLfloat,3);
				array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
					3, 1, floatValue);
				normals++;
			}
			if (tangents)
			{
				CAST_TO_OTHER(floatValue,(*tangents),GLfloat,3);
				array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_TANGENT,
					3, 1, floatValue);
				tangents++;
			}
			if (textures)
			{
				CAST_TO_OTHER(floatValue,(*textures),GLfloat,3);
				array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_TEXTURE_COORDINATE_ZERO,
					3, 1, floatValue);
				textures++;
			}
		}
		array->add_integer_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POLYGON,
			1, 1, &polygonType);
		if (data)
		{
			array->add_float_attribute(GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
				n_data_components, number_of_points, data);
		}
		array->fill_element_index(vertex_start, n_pts1, n_pts2, ARRAY_SHAPE_TYPE_UNSPECIFIED);
		return 1;
	}
	else
	{
		return 0;
	}
}
