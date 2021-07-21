/***************************************************************************//**
 * FILE : field_image_stream.cpp
 *
 * The definition to cmzn_field_image_stream.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "opencmiss/zinc/fieldimage.h"
#include "opencmiss/zinc/streamimage.h"
#include "computed_field/computed_field_image.h"
#include "general/debug.h"
#include "general/mystring.h"
#include "general/image_utilities.h"
#include "graphics/texture.h"
#include "general/message.h"
#include "general/enumerator_conversion.hpp"
#include "stream/field_image_stream.hpp"
#include "image_io/analyze.h"
#include "image_io/analyze_object_map.hpp"

int cmzn_field_image_read(cmzn_field_image_id image_field,
	cmzn_streaminformation_image_id streaminformation_image)
{
	int return_code = 1;
	struct Cmgui_image_information *image_information = NULL;

	if (image_field && streaminformation_image &&
		(NULL != (image_information = streaminformation_image->getImageInformation())))
	{
		char *field_name = cmzn_field_get_name(cmzn_field_image_base_cast(image_field));
		const cmzn_stream_properties_list streams_list = streaminformation_image->getResourcesList();
		char *texture_file_name = 0;
		if (!(streams_list.empty()))
		{
			cmzn_streaminformation_id streaminformation = cmzn_streaminformation_image_base_cast(
				streaminformation_image);
			enum cmzn_streaminformation_data_compression_type data_compression_type =
				cmzn_streaminformation_get_data_compression_type(streaminformation);
			cmzn_stream_properties_list_const_iterator iter;
			cmzn_resource_properties *stream_properties = NULL;
			cmzn_streamresource_id stream = NULL;
			int fileStream = 0;
			int memoryStream = 0;

			for (iter = streams_list.begin(); iter != streams_list.end(); ++iter)
			{
				stream_properties = *iter;
				stream = stream_properties->getResource();
				cmzn_streamresource_file_id file_resource = cmzn_streamresource_cast_file(stream);
				cmzn_streamresource_memory_id memory_resource = NULL;
				if (file_resource)
				{
					char *file_name = file_resource->getFileName();
					if (file_name)
					{
						if (!texture_file_name)
							texture_file_name = duplicate_string(file_name);
						fileStream = 1;
						if (!memoryStream)
						{
							Cmgui_image_information_add_file_name(image_information, file_name);
						}
						else
						{
							display_message(ERROR_MESSAGE, "cmzn_field_image_read. Cannot read both file and memory in "
								"one stream information");
							return_code = 0;
						}
						DEALLOCATE(file_name);
					}
					cmzn_streamresource_file_destroy(&file_resource);
				}
				else if (NULL != (memory_resource = cmzn_streamresource_cast_memory(stream)))
				{
					if (!texture_file_name)
						texture_file_name = duplicate_string(field_name);
					const void *memory_block = NULL;
					unsigned int buffer_size = 0;
					memory_resource->getBuffer(&memory_block, &buffer_size);
					if (memory_block)
					{
						memoryStream = 1;
						if (!fileStream)
						{
							Cmgui_image_information_add_memory_block(image_information, const_cast<void *>(memory_block),
								buffer_size);
						}
						else
						{
							display_message(ERROR_MESSAGE, "cmzn_field_image_read. Cannot read both file and memory in "
								"one stream information");
							return_code = 0;
						}
					}
					cmzn_streamresource_memory_destroy(&memory_resource);
				}
				else
				{
					return_code = 0;
					display_message(ERROR_MESSAGE, "cmzn_field_image_read. Stream error");
					break;
				}
				if (!return_code)
					break;
			}
			if (return_code)
			{
				struct Cmgui_image *cmgui_image = 0;
				if (Cmgui_image_information_get_image_file_format(image_information) == ANALYZE_FILE_FORMAT)
				{
					cmgui_image = Cmgui_image_read_analyze(image_information, data_compression_type);
				}
				else if (Cmgui_image_information_get_image_file_format(image_information) == ANALYZE_OBJECT_MAP_FORMAT)
				{
					cmgui_image = Cmgui_image_read_analyze_object_map(image_information, data_compression_type);
				}
				else
				{
					cmgui_image = Cmgui_image_read(image_information);
				}
				if (cmgui_image != NULL)
				{
					char *property, *value;
					Texture *texture= CREATE(Texture)(field_name);
					if (texture && Texture_set_image(texture, cmgui_image,
						texture_file_name, /*file_number_pattern*/NULL,
						/*file_number_series_data.start*/0,
						/*file_number_series_data.stop*/0,
						/*file_number_series_data.increment*/1,
						/*image_data.crop_left_margin*/0,
						/*image_data.crop_bottom_margin*/0,
						/*image_data.crop_width*/0, /*image_data.crop_height*/0))
					{
						/* Calling get_property with wildcard ensures they
						will be available to the iterator, as well as
						any other properties */
						Cmgui_image_get_property(cmgui_image,"exif:*");
						Cmgui_image_reset_property_iterator(cmgui_image);
						while ((property = Cmgui_image_get_next_property(
							cmgui_image)) &&
							(value = Cmgui_image_get_property(cmgui_image,
								property)))
						{
							Texture_set_property(texture, property, value);
							DEALLOCATE(property);
							DEALLOCATE(value);
						}
						DESTROY(Cmgui_image)(&cmgui_image);
						return_code = 1;
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"cmzn_field_image_read.  Could not create image for field");
						return_code = 0;
					}
					if (return_code)
					{
						// copy attributes from old texture
						Texture *old_texture = cmzn_field_image_get_texture(image_field);
						if (old_texture)
						{
							Texture_set_combine_mode(texture, Texture_get_combine_mode(old_texture));
							Texture_set_filter_mode(texture, Texture_get_filter_mode(old_texture));
							Texture_set_compression_mode(texture, Texture_get_compression_mode(old_texture));
							Texture_set_wrap_mode(texture, Texture_get_wrap_mode(old_texture));
							double sizes[3];
							cmzn_texture_get_texture_coordinate_sizes(old_texture, 3, sizes);
							cmzn_texture_set_texture_coordinate_sizes(texture, 3, sizes);
						}
						return_code = cmzn_field_image_set_texture(image_field, texture);
						DESTROY(Texture)(&texture);
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"cmzn_field_image_read.  Could not read image file");
					return_code = 0;
				}
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"cmzn_field_image_read.  streaminformation does not contain any stream");
			return_code = 0;
		}
		if (field_name)
			DEALLOCATE(field_name);
		if (texture_file_name)
			DEALLOCATE(texture_file_name);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_field_image_read.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_field_image_read */

int cmzn_field_image_read_file(cmzn_field_image_id image_field, const char *file_name)
{
	int return_code = 0;
	if (image_field && file_name)
	{
		cmzn_streaminformation_id streaminformation =
			cmzn_field_image_create_streaminformation_image(image_field);
		cmzn_streamresource_id resource = cmzn_streaminformation_create_streamresource_file(
			streaminformation, file_name);
		cmzn_streaminformation_image_id streaminformation_image =
			cmzn_streaminformation_cast_image(streaminformation);
		return_code = cmzn_field_image_read(image_field, streaminformation_image);
		cmzn_streamresource_destroy(&resource);
		cmzn_streaminformation_image_destroy(&streaminformation_image);
		cmzn_streaminformation_destroy(&streaminformation);
	}
	return return_code;
}

int cmzn_field_image_write(cmzn_field_image_id image_field,
	cmzn_streaminformation_image_id streaminformation_image)
{
	int return_code = 1;
	struct Cmgui_image_information *image_information = NULL;

	return_code = 1;
	if (image_field && streaminformation_image &&
		(NULL != (image_information = streaminformation_image->getImageInformation())))
	{
		struct Cmgui_image *cmgui_image = Texture_get_image(cmzn_field_image_get_texture(image_field));
		const cmzn_stream_properties_list streams_list = streaminformation_image->getResourcesList();
		const int number_of_streams = static_cast<int>(streams_list.size());
		if ((number_of_streams > 0) && cmgui_image &&
			(Cmgui_image_get_number_of_images(cmgui_image) == number_of_streams))
		{
			cmzn_stream_properties_list_const_iterator iter;
			cmzn_resource_properties *stream_properties = NULL;
			cmzn_streamresource_id stream = NULL;
			int set_write_to_memory = 0;
			int fileStream = 0, memoryStream = 0;
			for (iter = streams_list.begin(); iter != streams_list.end(); ++iter)
			{
				stream_properties = *iter;
				stream = stream_properties->getResource();
				cmzn_streamresource_file_id file_resource = cmzn_streamresource_cast_file(stream);
				cmzn_streamresource_memory_id memory_resource = NULL;
				if (file_resource)
				{
					char *file_name = file_resource->getFileName();
					if (file_name)
					{
						fileStream = 1;
						if (!memoryStream)
						{
							Cmgui_image_information_add_file_name(image_information, file_name);
						}
						else
						{
							return_code = 0;
						}
						DEALLOCATE(file_name);
					}
					cmzn_streamresource_file_destroy(&file_resource);
				}
				else if (NULL != (memory_resource = cmzn_streamresource_cast_memory(stream)))
				{
					if (fileStream)
					{
						return_code = 0;
					}
					else
					{
						memoryStream = 1;
						set_write_to_memory = 1;
					}
					cmzn_streamresource_memory_destroy(&memory_resource);
				}
				else
				{
					return_code = 0;
					display_message(ERROR_MESSAGE, "cmzn_field_image_write. Stream error");
					break;
				}
				if (!return_code)
					break;
			}
			if (return_code)
			{
				if (set_write_to_memory)
				{
					Cmgui_image_information_set_write_to_memory_block(image_information);
				}
				if (!Cmgui_image_write(cmgui_image, image_information))
				{
					display_message(ERROR_MESSAGE, "cmzn_field_image_write.  "
						"Error writing image");
					return_code = 0;
				}
				else
				{
					if (set_write_to_memory)
					{
						void **memory_blocks = NULL;
						unsigned int *memory_block_lengths = NULL;
						int number_of_blocks;
						Cmgui_image_information_get_memory_blocks(image_information, &number_of_blocks,
							&memory_blocks, &memory_block_lengths);
						if ((number_of_blocks == number_of_streams)&& memory_blocks && memory_block_lengths)
						{
							int k = 0;
							for (iter = streams_list.begin(); iter != streams_list.end(); ++iter)
							{
								stream_properties = *iter;
								stream = stream_properties->getResource();
								cmzn_streamresource_memory_id memory_resource =
									cmzn_streamresource_cast_memory(stream);
								if (memory_resource)
								{
									memory_resource->setBuffer(memory_blocks[k], memory_block_lengths[k]);
									cmzn_streamresource_memory_destroy(&memory_resource);
								}
								k++;
							}
							DEALLOCATE(memory_block_lengths);
							DEALLOCATE(memory_blocks);
						}
					}
				}
				DESTROY(Cmgui_image)(&cmgui_image);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"cmzn_field_image_write.  Stream information does not contain the correct"
				"numerb of streams or field does not contain images");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_field_image_write.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_field_image_write */

int cmzn_field_image_write_file(cmzn_field_image_id image_field, const char *file_name)
{
	int return_code = 0;
	if (image_field && file_name)
	{
		cmzn_streaminformation_id streaminformation =
			cmzn_field_image_create_streaminformation_image(image_field);
		cmzn_streamresource_id resource = cmzn_streaminformation_create_streamresource_file(
			streaminformation, file_name);
		cmzn_streaminformation_image_id streaminformation_image =
			cmzn_streaminformation_cast_image(streaminformation);
	  return_code = cmzn_field_image_write(image_field, streaminformation_image);
	cmzn_streamresource_destroy(&resource);
	cmzn_streaminformation_image_destroy(&streaminformation_image);
	cmzn_streaminformation_destroy(&streaminformation);
	}
	return return_code;
}

cmzn_streaminformation_id cmzn_field_image_create_streaminformation_image(
	cmzn_field_image_id image_field)
{
	if (image_field)
	{
		return new cmzn_streaminformation_image(image_field);
	}
	return NULL;
}

int cmzn_streaminformation_image_destroy(
	cmzn_streaminformation_image_id *streaminformation_address)
{
	if (streaminformation_address && *streaminformation_address)
	{
		(*streaminformation_address)->deaccess();
		*streaminformation_address = NULL;
		return 1;
	}
	return 0;
}

cmzn_streaminformation_image_id cmzn_streaminformation_cast_image(
	cmzn_streaminformation_id streaminformation)
{
	if (streaminformation &&
		(dynamic_cast<cmzn_streaminformation_image *>(streaminformation)))
	{
		streaminformation->access();
		return (reinterpret_cast<cmzn_streaminformation_image *>(streaminformation));
	}
	else
	{
		return (NULL);
	}
}

int cmzn_streaminformation_image_set_attribute_integer(
	cmzn_streaminformation_image_id streaminformation,
	enum cmzn_streaminformation_image_attribute attribute, int value)
{
	struct Cmgui_image_information *image_information = NULL;
	if (streaminformation &&
		(NULL != (image_information = streaminformation->getImageInformation())))
	{
		switch (attribute)
		{
			case CMZN_STREAMINFORMATION_IMAGE_ATTRIBUTE_RAW_WIDTH_PIXELS:
			{
				return (Cmgui_image_information_set_width(image_information, value));
			} break;
			case CMZN_STREAMINFORMATION_IMAGE_ATTRIBUTE_RAW_HEIGHT_PIXELS:
			{
				return (Cmgui_image_information_set_height(image_information, value));
			} break;
			case CMZN_STREAMINFORMATION_IMAGE_ATTRIBUTE_BITS_PER_COMPONENT:
			{
				int number_of_bytes = 1;
				if (value == 8)
					number_of_bytes = 1;
				else if (value ==16)
					number_of_bytes = 2;
				return (Cmgui_image_information_set_number_of_bytes_per_component(
					image_information, number_of_bytes));
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"cmzn_streaminformation_image_set_attribute_integer.  "
					"Invalid attribute");
			} break;
		}
	}
	return 0;
}

int cmzn_streaminformation_image_set_attribute_real(
	cmzn_streaminformation_image_id streaminformation,
	enum cmzn_streaminformation_image_attribute attribute, double value)
{
	struct Cmgui_image_information *image_information = NULL;
	if (streaminformation &&
		(NULL != (image_information = streaminformation->getImageInformation())))
	{
		switch (attribute)
		{
			case CMZN_STREAMINFORMATION_IMAGE_ATTRIBUTE_COMPRESSION_QUALITY:
			{
				return (Cmgui_image_information_set_quality(image_information, value));
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"cmzn_streaminformation_image_set_attribute_double.  "
					"Invalid attribute");
			} break;
		}
	}
	return 0;
}

class cmzn_streaminformation_image_attribute_conversion
{
public:
	static const char *to_string(enum cmzn_streaminformation_image_attribute attribute)
	{
		const char *enum_string = 0;
		switch (attribute)
		{
			case CMZN_STREAMINFORMATION_IMAGE_ATTRIBUTE_RAW_WIDTH_PIXELS:
				enum_string = "RAW_WIDTH_PIXELS";
				break;
			case CMZN_STREAMINFORMATION_IMAGE_ATTRIBUTE_RAW_HEIGHT_PIXELS:
				enum_string = "RAW_HEIGHT_PIXELS";
				break;
			case CMZN_STREAMINFORMATION_IMAGE_ATTRIBUTE_BITS_PER_COMPONENT:
				enum_string = "BITS_PER_COMPONENT";
				break;
			case CMZN_STREAMINFORMATION_IMAGE_ATTRIBUTE_COMPRESSION_QUALITY:
				enum_string = "COMPRESSION_QUALITY";
				break;
			default:
				break;
		}
		return enum_string;
	}
};

enum cmzn_streaminformation_image_attribute
	cmzn_streaminformation_image_attribute_enum_from_string(const char *string)
{
	return string_to_enum<enum cmzn_streaminformation_image_attribute,
		cmzn_streaminformation_image_attribute_conversion>(string);
}

char *cmzn_streaminformation_image_attribute_enum_to_string(
	enum cmzn_streaminformation_image_attribute attribute)
{
	const char *attribute_string = cmzn_streaminformation_image_attribute_conversion::to_string(attribute);
	return (attribute_string ? duplicate_string(attribute_string) : 0);
}

int cmzn_streaminformation_image_set_file_format(
	cmzn_streaminformation_image_id streaminformation,
	enum cmzn_streaminformation_image_file_format format)
{
	struct Cmgui_image_information *image_information = NULL;
	enum Image_file_format cmgui_file_format = JPG_FILE_FORMAT;
	int return_code = 0;

	if (streaminformation &&
		(NULL != (image_information = streaminformation->getImageInformation())))
	{
		return_code = 1;
		switch (format)
		{
			case CMZN_STREAMINFORMATION_IMAGE_FILE_FORMAT_BMP:
			{
				cmgui_file_format = BMP_FILE_FORMAT;
			} break;
			case CMZN_STREAMINFORMATION_IMAGE_FILE_FORMAT_DICOM:
			{
				cmgui_file_format = DICOM_FILE_FORMAT;
			} break;
			case CMZN_STREAMINFORMATION_IMAGE_FILE_FORMAT_JPG:
			{
				cmgui_file_format = JPG_FILE_FORMAT;
			} break;
			case CMZN_STREAMINFORMATION_IMAGE_FILE_FORMAT_GIF:
			{
				cmgui_file_format = GIF_FILE_FORMAT;
			} break;
			case CMZN_STREAMINFORMATION_IMAGE_FILE_FORMAT_PNG:
			{
				cmgui_file_format = PNG_FILE_FORMAT;
			} break;
			case CMZN_STREAMINFORMATION_IMAGE_FILE_FORMAT_SGI:
			{
				cmgui_file_format = SGI_FILE_FORMAT;
			} break;
			case CMZN_STREAMINFORMATION_IMAGE_FILE_FORMAT_TIFF:
			{
				cmgui_file_format = TIFF_FILE_FORMAT;
			} break;
			case CMZN_STREAMINFORMATION_IMAGE_FILE_FORMAT_ANALYZE:
			{
				cmgui_file_format = ANALYZE_FILE_FORMAT;
			} break;
			case CMZN_STREAMINFORMATION_IMAGE_FILE_FORMAT_ANALYZE_OBJECT_MAP:
			{
				cmgui_file_format = ANALYZE_OBJECT_MAP_FORMAT;
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"cmzn_streaminformation_image_set_format.  "
					"File format not implemented yet.");
			} break;
		}
		if (return_code)
		{
			return_code = Cmgui_image_information_set_image_file_format(
				image_information, cmgui_file_format);
		}
	}
	return (return_code);
}

class cmzn_streaminformation_image_file_format_conversion
{
public:
	static const char *to_string(enum cmzn_streaminformation_image_file_format format)
	{
		const char *enum_string = 0;
		switch (format)
		{
			case CMZN_STREAMINFORMATION_IMAGE_FILE_FORMAT_BMP:
				enum_string = "BMP";
				break;
			case CMZN_STREAMINFORMATION_IMAGE_FILE_FORMAT_DICOM:
				enum_string = "DICOM";
				break;
			case CMZN_STREAMINFORMATION_IMAGE_FILE_FORMAT_JPG:
				enum_string = "JPG";
				break;
			case CMZN_STREAMINFORMATION_IMAGE_FILE_FORMAT_GIF:
				enum_string = "GIF";
				break;
			case CMZN_STREAMINFORMATION_IMAGE_FILE_FORMAT_PNG:
				enum_string = "PNG";
				break;
			case CMZN_STREAMINFORMATION_IMAGE_FILE_FORMAT_SGI:
				enum_string = "SGI";
				break;
			case CMZN_STREAMINFORMATION_IMAGE_FILE_FORMAT_TIFF:
				enum_string = "TIFF";
				break;
			case CMZN_STREAMINFORMATION_IMAGE_FILE_FORMAT_ANALYZE:
				enum_string = "ANALYZE";
				break;
			case CMZN_STREAMINFORMATION_IMAGE_FILE_FORMAT_ANALYZE_OBJECT_MAP:
				enum_string = "ANALYZE_OBJECT_MAP";
				break;
			default:
				break;
		}
		return enum_string;
	}
};

enum cmzn_streaminformation_image_file_format
	cmzn_streaminformation_image_file_format_enum_from_string(const char *string)
{
	return string_to_enum<enum cmzn_streaminformation_image_file_format,
	cmzn_streaminformation_image_file_format_conversion>(string);
}

char *cmzn_streaminformation_image_file_format_enum_to_string(
	enum cmzn_streaminformation_image_file_format format)
{
	const char *format_string = cmzn_streaminformation_image_file_format_conversion::to_string(format);
	return (format_string ? duplicate_string(format_string) : 0);
}

int cmzn_streaminformation_image_set_pixel_format(
	cmzn_streaminformation_image_id streaminformation,
	enum cmzn_streaminformation_image_pixel_format pixel_format)
{
	struct Cmgui_image_information *image_information = NULL;
	int return_code = 0;
	if (streaminformation &&
		(NULL != (image_information = streaminformation->getImageInformation())))
	{
		int number_of_components;
		switch(pixel_format)
		{
			case CMZN_STREAMINFORMATION_IMAGE_PIXEL_FORMAT_LUMINANCE:
			{
				number_of_components = 1;
			} break;
			case CMZN_STREAMINFORMATION_IMAGE_PIXEL_FORMAT_LUMINANCE_ALPHA:
			{
				number_of_components = 2;
			} break;
			case CMZN_STREAMINFORMATION_IMAGE_PIXEL_FORMAT_RGB:
			{
				number_of_components = 3;
			} break;
			case CMZN_STREAMINFORMATION_IMAGE_PIXEL_FORMAT_RGBA:
			{
				number_of_components = 4;
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"Texture_set_pixel_format.  Pixel format not implemented yet.");
				number_of_components = 0;
			} break;
		}
		if (number_of_components)
		{
			return_code = Cmgui_image_information_set_number_of_components(
				image_information, number_of_components);
		}
	}
	return (return_code);
}

class cmzn_streaminformation_image_pixel_format_conversion
{
public:
	static const char *to_string(enum cmzn_streaminformation_image_pixel_format format)
	{
		const char *enum_string = 0;
		switch (format)
		{
			case CMZN_STREAMINFORMATION_IMAGE_PIXEL_FORMAT_LUMINANCE:
				enum_string = "LUMINANCE";
				break;
			case CMZN_STREAMINFORMATION_IMAGE_PIXEL_FORMAT_RGB:
				enum_string = "RGB";
				break;
			case CMZN_STREAMINFORMATION_IMAGE_PIXEL_FORMAT_RGBA:
				enum_string = "RGBA";
				break;
			case CMZN_STREAMINFORMATION_IMAGE_PIXEL_FORMAT_ABGR:
				enum_string = "ABGR";
				break;
			case CMZN_STREAMINFORMATION_IMAGE_PIXEL_FORMAT_BGR:
				enum_string = "BGR";
				break;
			case CMZN_STREAMINFORMATION_IMAGE_PIXEL_FORMAT_LUMINANCE_ALPHA:
				enum_string = "LUMINANCE_ALPHA";
				break;
			default:
				break;
		}
		return enum_string;
	}
};

enum cmzn_streaminformation_image_pixel_format
	cmzn_streaminformation_image_pixel_format_enum_from_string(
		const char *string)
{
	return string_to_enum<enum cmzn_streaminformation_image_pixel_format,
		cmzn_streaminformation_image_pixel_format_conversion>(string);
}

char *cmzn_streaminformation_image_pixel_format_enum_to_string(
	enum cmzn_streaminformation_image_pixel_format format)
{
	const char *format_string = cmzn_streaminformation_image_pixel_format_conversion::to_string(format);
	return (format_string ? duplicate_string(format_string) : 0);
}
