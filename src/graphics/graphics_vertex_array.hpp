/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/**
 * C++ interfaces for graphics_vertex_array.hpp
 */
#ifndef GRAPHICS_VERTEX_ARRAY_HPP
#define GRAPHICS_VERTEX_ARRAY_HPP

#include "graphics/graphics_object.h"
#include <string>

enum Graphics_vertex_array_shape_type
{
	ARRAY_SHAPE_TYPE_UNSPECIFIED = 0,
	ARRAY_SHAPE_TYPE_SIMPLEX = 1
};

/*****************************************************************************//**
 * Specifies the type of storage to be used for the vertex buffer array.
 * As vertices are added to the array they will be organised in memory
 * according to this type, so that they are efficiently formatted when the
 * vertex buffer memory pointers are retrieved.
*/
enum Graphics_vertex_array_type
{
	/** Each type of vertex attribute is added to a buffer for vertices of just that type.
	 * All attributes are stored as GLfloat values, except element indices which are
	 * stored by recording the sizes of each array and the first index suitable for
	 * using with draw arrays (thus indices for a given primitive must be consecutive). */
	GRAPHICS_VERTEX_ARRAY_TYPE_FLOAT_SEPARATE_DRAW_ARRAYS
	/* Other types may support interleaved buffer formats, other value types or
	 * the arbitrary ordering of indices used with draw elements. */
}; /* enum Graphics_vertex_array_type */

/*****************************************************************************//**
 * Specifies the type of the vertices being added or retrieved from the vertex buffer.
 * The formats supported for reading or writing depend on the array type.
 * If the array types supports interleaved values then vertex types for those
 * interleaved values may also be specified.
 * The numbers in the enumerations specify the number of values per vertex.
 * @see Graphics_vertex_array_type.
*/
enum Graphics_vertex_array_attribute_type
{
	/** Vertex position values. */
	GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POSITION,
	/** Vertex normal values. */
	GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NORMAL,
	/** Per vertex colour values. */
	GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_COLOUR,
	/** Per vertex data values. */
	GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_DATA,
	/** First texture coordinate values. */
	GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_TEXTURE_COORDINATE_ZERO,
	/** Specifies that the number of vertices for a primitive. */
	GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_COUNT,
	/** Specifies that the index of the first vertex for a primitive. */
	GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_START,
	/** Records the identifier of a particular primitive for selection and editing. */
	GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_VERTEX_ID,
	/** Per vertex tangent values. */
	GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_TANGENT,
	/** Per vertex axis_1 values. */
	GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_AXIS1,
	/** Per vertex axis_2 values. */
	GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_AXIS2,
	/** Per vertex axis_3 values. */
	GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_AXIS3,
	/** Per vertex scale values. */
	GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_SCALE,
	/** Per vertex label density values. */
	GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_LABEL_DENSITY,
	GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_LABEL_BOUND,
	GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_XI1,
	GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_XI2,
	GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_SCALE_OFFSET,
	GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_POLYGON,
	GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_OBJECT_ID,
	/** number of strips for element */
	GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_STRIPS,
	/** number of point for strip */
	GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_NUMBER_OF_POINTS_FOR_STRIP,
	/** Index at which information of strips for this element start */
	GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_STRIP_START,
	/** starting index for strip index array */
	GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_STRIP_INDEX_START,
	/** array for storing the index */
	GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_STRIP_INDEX_ARRAY,
	GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_UPDATE_REQUIRED,
	GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_LABEL,
	GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_PARTIAL_REDRAW,
	GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_PARTIAL_REDRAW_COUNT
	/* Complex types might be like this...
	 * GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_VERTEX3_NORMAL3
	 * and element_array indices might be supported with an DRAW_ELEMENTS set
	 * type where each vertex index can be unrelated.
	 * GRAPHICS_VERTEX_ARRAY_ATTRIBUTE_TYPE_ELEMENT_INDEX_3
	 */
}; /* enum Graphics_vertex_array_attribute_type */

/** Private implementation of Graphics_vertex_array */
class Graphics_vertex_array_internal;

/*****************************************************************************//**
 * Object for storing attributes for arrays of vertices.
*/
struct Graphics_vertex_array
{
private:
	class Graphics_vertex_array_internal *internal;

public:

	/*****************************************************************************//**
	 * Construct a new vertex array of the specified type.
	*/
	Graphics_vertex_array(Graphics_vertex_array_type type);

	/*****************************************************************************//**
	 * Destroys a vertex array.
	*/
	~Graphics_vertex_array();

	/*****************************************************************************//**
	 * Add values to set.
	 *
	 * @param vertex_type  Specifies the format of the supplied vertices.
	 * @param values_per_vertex  The number of values for each vertex.
	 * @param number_of_values  The size of the values array.
	 * @param values  Array of values, length is required to match that expected by
	 * the specified vertex_type.
	 * @return return_code. 1 for Success, 0 for failure.
	*/
/*	int add_float_attribute(
			Graphics_vertex_array_attribute_type vertex_type,
			unsigned int values_per_vertex, unsigned int number_of_values, GLfloat *values);
*/
	int add_float_attribute( Graphics_vertex_array_attribute_type vertex_type,
		const unsigned int values_per_vertex, const unsigned int number_of_values, const GLfloat *values);

	/*****************************************************************************//**
	 * Retrieve pointer to value buffer from set.
	 *
	 * @param vertex_buffer_type  Specifies the format expected of the vertex buffer.  If the
	 * type requested is not supported by the set type then the routine will fail.
	 * (This behaviour could be changed to allow format conversion but the point of this
	 * object is to avoid such conversions and so isn't expected normally.)
	 * @param vertex_buffer  Returns a pointer to the vertex buffer.  It is a reference
	 * to the sets own memory and not a copy and so can not be used once the
	 * set is destroyed or modified and should not be freed.
	 * @param values_per_vertex  Returns the number of values for each vertex.
	 * @param vertex_count  Returns the total number of vertices.  The total number of GLfloat
	 * values is the vertex_count * "values per vertex according to vertex type".
	 * @return return_code. 1 for Success, 0 for failure.
	*/
	int get_float_vertex_buffer(
			Graphics_vertex_array_attribute_type vertex_type,
			GLfloat **vertex_buffer, unsigned int *values_per_vertex,
			unsigned int *vertex_count);

	int add_string_attribute(Graphics_vertex_array_attribute_type vertex_type,
		const unsigned int values_per_vertex, const unsigned int number_of_values, std::string *values);

	int get_string_vertex_buffer(Graphics_vertex_array_attribute_type vertex_type,
		std::string **vertex_buffer, unsigned int *values_per_vertex,
		unsigned int *vertex_count);

	int replace_float_vertex_buffer_at_position(
		Graphics_vertex_array_attribute_type vertex_type,
		const unsigned int vertex_index,	const unsigned int values_per_vertex,
		const unsigned int number_of_values, const GLfloat *values);

	/*****************************************************************************//**
	 * Add values to set.
	 *
	 * @param vertex_type  Specifies the format of the supplied vertices.
	 * @param number_of_values  The size of the values array.
	 * @param values  Array of values, length is required to match that expected by
	 * the specified vertex_type.
	 * @return return_code. 1 for Success, 0 for failure.
	*/
	int add_unsigned_integer_attribute(
			Graphics_vertex_array_attribute_type vertex_type,
			const unsigned int values_per_vertex, const unsigned int number_of_values,
			const unsigned int *values);

	/*****************************************************************************//**
	 * Get values from set.
	 *
	 * @param vertex_type  Specifies the format of the supplied vertices.
	 * @param vertex_index  The index of the vertex that values will be returned for.
	 * @param number_of_values  The expected size of the values array.
	 * @param values  Array of values.
	 * @return return_code. 1 for Success, 0 for failure.
	*/
	int get_unsigned_integer_attribute(
		Graphics_vertex_array_attribute_type vertex_type,
		unsigned int vertex_index,	unsigned int number_of_values, unsigned int *values);

	/*****************************************************************************//**
	 * Retrieve pointer to value buffer from set.
	 *
	 * @param vertex_buffer_type  Specifies the format expected of the vertex buffer.  If the
	 * type requested is not supported by the set type then the routine will fail.
	 * (This behaviour could be changed to allow format conversion but the point of this
	 * object is to avoid such conversions and so isn't expected normally.)
	 * @param vertex_buffer  Returns a pointer to the vertex buffer.  It is a reference
	 * to the sets own memory and not a copy and so can not be used once the
	 * set is destroyed or modified and should not be freed.
	 * @param vertex_count  Returns the total number of vertices.  The total number of GLfloat
	 * values is the vertex_count * "values per vertex according to vertex type".
	 * @return return_code. 1 for Success, 0 for failure.
	*/
	int get_unsigned_integer_vertex_buffer(
		Graphics_vertex_array_attribute_type vertex_buffer_type,
		unsigned int **vertex_buffer, unsigned int *values_per_vertex,
		unsigned int *vertex_count);

	/*****************************************************************************//**
	 * Add values to set.
	 *
	 * @param vertex_type  Specifies the format of the supplied vertices.
	 * @param number_of_values  The size of the values array.
	 * @param values  Array of values.
	 * @return return_code. 1 for Success, 0 for failure.
	*/
	int add_integer_attribute(
		Graphics_vertex_array_attribute_type vertex_type,
		const unsigned int values_per_vertex, const unsigned int number_of_values,
		const int *values);

	/*****************************************************************************//**
	 * Get values from set.
	 *
	 * @param vertex_type  Specifies the format of the supplied vertices.
	 * @param vertex_index  The index of the vertex that values will be returned for.
	 * @param number_of_values  The expected size of the values array.
	 * @param values  Array of values.
	 * @return return_code. 1 for Success, 0 for failure.
	*/
	int get_integer_attribute(
		Graphics_vertex_array_attribute_type vertex_type,
		unsigned int vertex_index,	unsigned int number_of_values, int *values);

	/*****************************************************************************//**
	 * Replace values in set.
	 *
	 * @param vertex_type  Specifies the format of the supplied vertices.
	 * @param vertex_index  values starting from this vertex index will be replaced.
	 * @param values_per_vertex  provide the values per vertex, it must match with the stored one.
	 * @param number_of_values  The number of vertices to be replaced.
	 * @param values  array of values to replace the one in set.
	 * @return return_code. 1 for Success, 0 for failure.
	*/
	int replace_integer_vertex_buffer_at_position(Graphics_vertex_array_attribute_type vertex_type,
		const unsigned int vertex_index,	const unsigned int values_per_vertex,
		const unsigned int number_of_values, const int *values);

	/*****************************************************************************//**
	 * Retrieve pointer to value buffer from set.
	 *
	 * @param vertex_buffer_type  Specifies the format expected of the vertex buffer.  If the
	 * type requested is not supported by the set type then the routine will fail.
	 * (This behaviour could be changed to allow format conversion but the point of this
	 * object is to avoid such conversions and so isn't expected normally.)
	 * @param integer_buffer  Returns a pointer to the integer_buffer.  It is a reference
	 * to the sets own memory and not a copy and so can not be used once the
	 * set is destroyed or modified and should not be freed.
	 * @param values_per_vertex  Returns the number of values for each vertex.
	 * @param vertex_count  Returns the total number of vertices.  The total number of GLfloat
	 * values is the vertex_count * "values per vertex according to vertex type".
	 * @return return_code. 1 for Success, 0 for failure.
	*/
	int get_integer_vertex_buffer(
			Graphics_vertex_array_attribute_type vertex_buffer_type,
			int **integer_buffer, unsigned int *values_per_vertex,
			unsigned int *vertex_count);

	/*****************************************************************************//**
	 * Gets the current size of specified buffer.
	 *
	 * @param vertex_buffer_type  Specifies that the size should be for buffer of this type.
	 * This actual buffers created and used for different attributes depends on the array type.
	 * @return buffer size.
	*/
	unsigned int get_number_of_vertices(
		Graphics_vertex_array_attribute_type vertex_type);

	/**
	 * Free any unused memory at the end of a buffer
	 */
	int free_unused_buffer_memory( Graphics_vertex_array_attribute_type vertex_type );

	/*****************************************************************************//**
	 * Resets the sizes of all the buffers in the set.  Does not actually
	 * release memory in the buffers as it is assumed likely that the same buffers
	 * will be recreated.
	 *
	 * @return return_code.
	*/
	int clear_buffers();

	/*****************************************************************************//**
	 * Resets the sizes of the specified buffers in the set.  Does not actually
	 * release memory in the buffer as it is assumed likely that the same buffer
	 * will be recreated.
	 *
	 * @return return_code.
	*/
	int clear_specified_buffer(Graphics_vertex_array_attribute_type vertex_type);

	/*****************************************************************************//**
	 * Find the first location in the array with the same integer value.
	 *
	 * @return first location, negative integer if none found.
	 */
	int find_first_location_of_integer_value(enum Graphics_vertex_array_attribute_type vertex_type, int value);

	int add_fast_search_id(int object_id);

	/* return the first index if element with same id is found, this is used
	 * with fixed number of vertices per id e.g. elements */
	int find_first_fast_search_id_location(int target_id);

	/* return the all indices if element with same id is found, this is used
	 * with varying number of vertices per id e.g contour */
	int get_all_fast_search_id_locations(int target_id, int *number_of_locations, int **locations);

	void fill_element_index(unsigned vertex_start, unsigned int number_of_xi1, unsigned int number_of_xi2,
		enum Graphics_vertex_array_shape_type shape_type);

};

int fill_glyph_graphics_vertex_array(struct Graphics_vertex_array *array, int vertex_location,
	unsigned int number_of_points, Triple *point_list, Triple *axis1_list, Triple *axis2_list,
	Triple *axis3_list, Triple *scale_list,	int n_data_components, GLfloat *data,
	Triple *label_density_list, int object_name, int *names, char **labels, int label_bounds_values,
	int label_bounds_components, ZnReal *label_bounds);

int fill_line_graphics_vertex_array(struct Graphics_vertex_array *array,
	unsigned int n_pts,Triple *pointlist,Triple *normallist,
	int n_data_components, GLfloat *data);

int fill_pointset_graphics_vertex_array(struct Graphics_vertex_array *array,
	unsigned int n_pts,Triple *pointlist,char **text, int n_data_components, GLfloat *data);

int fill_surface_graphics_vertex_array(struct Graphics_vertex_array *array,
	gtPolygonType polytype, unsigned int n_pts1, unsigned int n_pts2,Triple *pointlist,
	Triple *normallist, Triple *tangentlist, Triple *texturelist,
	int n_data_components,GLfloat *data);

#endif /* GRAPHICS_VERTEX_ARRAY_HPP */
