/***************************************************************************//**
 * FILE : region_stream.cpp
 *
 * The definition to cmzn_region_stream.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "opencmiss/zinc/streamregion.h"
#include "opencmiss/zinc/streamregion.h"
#include "field_io/fieldml_common.hpp"
#include "field_io/read_fieldml.hpp"
#include "field_io/write_fieldml.hpp"
#include "finite_element/export_finite_element.h"
#include "finite_element/import_finite_element.h"
#include "general/debug.h"
#include "general/mystring.h"
#include "region/cmiss_region.hpp"
#include "stream/region_stream.hpp"

namespace {

int cmzn_region_read_from_memory(struct cmzn_region *region, const void *memory_buffer,
	const unsigned int memory_buffer_size, struct FE_import_time_index *time_index, int useData,
	enum cmzn_streaminformation_data_compression_type data_compression_type,
	cmzn_streaminformation_region_file_format fileFormatIn)
{
	const char block_name[] = "dataBlock";
	const char block_name_uri[] = "memory:dataBlock";
	int return_code;
	struct IO_stream_package *io_stream_package;
	struct IO_stream *input_stream;

	return_code = CMZN_ERROR_ARGUMENT;
	if (region && memory_buffer && memory_buffer_size && (io_stream_package=CREATE(IO_stream_package)()))
	{
		cmzn_streaminformation_region_file_format fileFormat = fileFormatIn;
		if (fileFormat == CMZN_STREAMINFORMATION_REGION_FILE_FORMAT_AUTOMATIC)
		{
			if (is_FieldML_memory_block(memory_buffer_size, memory_buffer))
				fileFormat = CMZN_STREAMINFORMATION_REGION_FILE_FORMAT_FIELDML;
			else
				fileFormat = CMZN_STREAMINFORMATION_REGION_FILE_FORMAT_EX;
		}
		switch (fileFormat)
		{
			case CMZN_STREAMINFORMATION_REGION_FILE_FORMAT_FIELDML:
				display_message(WARNING_MESSAGE, "cmzn_region_read.  Cannot read FieldML from memory resource");
				break;
			case CMZN_STREAMINFORMATION_REGION_FILE_FORMAT_EX:
			{
				// We should add a way to define a memory block without requiring specifying a name.
				IO_stream_package_define_memory_block(io_stream_package,
					block_name, memory_buffer, memory_buffer_size);
				input_stream = CREATE(IO_stream)(io_stream_package);
				IO_stream_open_for_read_compression_specified(input_stream, block_name_uri,
					data_compression_type);
				if (!useData)
				{
					return_code = read_exregion_file(region, input_stream, time_index) ? CMZN_OK : CMZN_ERROR_GENERAL;
				}
				else
				{
					return_code = read_exdata_file(region, input_stream, time_index) ? CMZN_OK : CMZN_ERROR_GENERAL;
				}
				IO_stream_close(input_stream);
				DESTROY(IO_stream)(&input_stream);
				IO_stream_package_free_memory_block(io_stream_package,
					block_name);
				DESTROY(IO_stream_package)(&io_stream_package);
			} break;
			case CMZN_STREAMINFORMATION_REGION_FILE_FORMAT_AUTOMATIC:
			case CMZN_STREAMINFORMATION_REGION_FILE_FORMAT_INVALID:
				display_message(WARNING_MESSAGE, "cmzn_region_read.  Invalid file format specified for memory resource");
				break;
		}
	}
	return return_code;
}

int cmzn_region_read_field_file_of_name(struct cmzn_region *region, const char *file_name,
	struct IO_stream_package *io_stream_package,
	struct FE_import_time_index *time_index, int useData,
	enum cmzn_streaminformation_data_compression_type data_compression_type,
	cmzn_streaminformation_region_file_format fileFormatIn)
{
	int return_code = CMZN_ERROR_ARGUMENT;
	cmzn_streaminformation_region_file_format fileFormat = fileFormatIn;
	if (fileFormat == CMZN_STREAMINFORMATION_REGION_FILE_FORMAT_AUTOMATIC)
	{
		if (is_FieldML_file(file_name))
			fileFormat = CMZN_STREAMINFORMATION_REGION_FILE_FORMAT_FIELDML;
		else
			fileFormat = CMZN_STREAMINFORMATION_REGION_FILE_FORMAT_EX;
	}
	switch (fileFormat)
	{
		case CMZN_STREAMINFORMATION_REGION_FILE_FORMAT_FIELDML:
			if (time_index)
				display_message(WARNING_MESSAGE, "cmzn_region_read.  Time not supported by FieldML reader");
			return_code = parse_fieldml_file(region, file_name) ? CMZN_OK : CMZN_ERROR_GENERAL;
			break;
		case CMZN_STREAMINFORMATION_REGION_FILE_FORMAT_EX:
			return_code = read_exregion_file_of_name(region, file_name, io_stream_package, time_index,
				useData, data_compression_type) ? CMZN_OK : CMZN_ERROR_GENERAL;
			break;
		case CMZN_STREAMINFORMATION_REGION_FILE_FORMAT_AUTOMATIC:
		case CMZN_STREAMINFORMATION_REGION_FILE_FORMAT_INVALID:
			display_message(WARNING_MESSAGE, "cmzn_region_read.  Invalid file format specified");
			break;
	}
	return return_code;
}

}

int cmzn_region_read(cmzn_region_id region,
	cmzn_streaminformation_region_id streaminformation_region)
{
	struct FE_import_time_index time_index_value, *time_index = NULL,
		stream_time_index_value, *stream_time_index = NULL;
	int return_code = CMZN_OK;
	if (region && streaminformation_region &&
		(cmzn_streaminformation_region_get_region_private(streaminformation_region) == region))
	{
		enum cmzn_streaminformation_data_compression_type data_compression_type =
			CMZN_STREAMINFORMATION_DATA_COMPRESSION_TYPE_NONE;
		const cmzn_stream_properties_list streams_list = streaminformation_region->getResourcesList();
		struct IO_stream_package *io_stream_package = CREATE(IO_stream_package)();
		cmzn_region_begin_hierarchical_change(region);
		struct cmzn_region *temp_region = cmzn_region_create_region(region);
		if (!(streams_list.empty()) && io_stream_package && temp_region)
		{
			cmzn_region_begin_hierarchical_change(temp_region);
			cmzn_stream_properties_list_const_iterator iter;
			cmzn_resource_properties *stream_properties = NULL;
			cmzn_streamresource_id stream = NULL;
			if (cmzn_streaminformation_region_has_attribute(streaminformation_region,
				CMZN_STREAMINFORMATION_REGION_ATTRIBUTE_TIME))
			{
				time_index_value.time = cmzn_streaminformation_region_get_attribute_real(
					streaminformation_region, CMZN_STREAMINFORMATION_REGION_ATTRIBUTE_TIME);
				time_index = &time_index_value;
			}
			for (iter = streams_list.begin(); (iter != streams_list.end()) && (return_code == CMZN_OK); ++iter)
			{
				data_compression_type = CMZN_STREAMINFORMATION_DATA_COMPRESSION_TYPE_NONE;
				stream_properties = *iter;
				stream = stream_properties->getResource();
				if (cmzn_streaminformation_region_has_resource_attribute(
					streaminformation_region, stream, CMZN_STREAMINFORMATION_REGION_ATTRIBUTE_TIME))
				{
					stream_time_index_value.time = cmzn_streaminformation_region_get_resource_attribute_real(
						streaminformation_region, stream, CMZN_STREAMINFORMATION_REGION_ATTRIBUTE_TIME);
					stream_time_index = &stream_time_index_value;
				}
				else
				{
					stream_time_index = time_index;
				}
				cmzn_streaminformation_id streaminformation = cmzn_streaminformation_region_base_cast(
					streaminformation_region);
				data_compression_type = cmzn_streaminformation_get_resource_data_compression_type(streaminformation, stream);
				if (data_compression_type ==	CMZN_STREAMINFORMATION_DATA_COMPRESSION_TYPE_DEFAULT)
				{
					data_compression_type = cmzn_streaminformation_get_data_compression_type(streaminformation);
				}
				cmzn_streaminformation_region_file_format fileFormat =
					cmzn_streaminformation_region_get_file_format(streaminformation_region);
				cmzn_streamresource_file_id file_resource = cmzn_streamresource_cast_file(stream);
				cmzn_streamresource_memory_id memory_resource = NULL;
				const void *memory_block = NULL;
				unsigned int buffer_size = 0;
				int readData = 0;
				int domain_type = cmzn_streaminformation_region_get_resource_domain_types(
					streaminformation_region, stream);
				if ((domain_type & CMZN_FIELD_DOMAIN_TYPE_DATAPOINTS) && (!(domain_type & CMZN_FIELD_DOMAIN_TYPE_NODES)))
				{
					readData = 1;
				}
				if (file_resource)
				{
					char *file_name = file_resource->getFileName();
					if (file_name)
					{
						return_code = cmzn_region_read_field_file_of_name(temp_region, file_name, io_stream_package, stream_time_index,
							readData, data_compression_type, fileFormat);
						if (return_code != CMZN_OK)
							display_message(ERROR_MESSAGE, "cmzn_region_read.  Cannot read file %s", file_name);
						DEALLOCATE(file_name);
					}
					cmzn_streamresource_file_destroy(&file_resource);
				}
				else if (NULL != (memory_resource = cmzn_streamresource_cast_memory(stream)))
				{
					memory_resource->getBuffer(&memory_block, &buffer_size);
					if (memory_block)
					{
						return_code = cmzn_region_read_from_memory(temp_region, memory_block, buffer_size, stream_time_index,
							readData, data_compression_type, fileFormat);
						if (return_code != CMZN_OK)
							display_message(ERROR_MESSAGE, "cmzn_region_read.  Cannot read memory resource");
					}
					cmzn_streamresource_memory_destroy(&memory_resource);
				}
				else
				{
					return_code = CMZN_ERROR_GENERAL;
					display_message(ERROR_MESSAGE, "cmzn_region_read.  Stream error");
				}
			}
			// end change before merge otherwise there will be callbacks for changes
			// to half-temporary, half-global objects, leading to errors
			cmzn_region_end_hierarchical_change(temp_region);
			if (return_code == CMZN_OK)
			{
				if (!cmzn_region_can_merge(region, temp_region))
					return_code = CMZN_ERROR_INCOMPATIBLE_DATA;
				else if (!cmzn_region_merge(region, temp_region))
					return_code = CMZN_ERROR_GENERAL;
			}
		}
		DEACCESS(cmzn_region)(&temp_region);
		cmzn_region_end_hierarchical_change(region);
		DESTROY(IO_stream_package)(&io_stream_package);
	}
	else
	{
		return_code = CMZN_ERROR_ARGUMENT;
	}
	return return_code;
}

int cmzn_region_read_file(cmzn_region_id region, const char *file_name)
{
	if (region && file_name)
	{
		cmzn_streaminformation_id streaminformation =
			cmzn_region_create_streaminformation_region(region);
		cmzn_streamresource_id resource = cmzn_streaminformation_create_streamresource_file(
			streaminformation, file_name);
		cmzn_streaminformation_region_id streaminformation_region =
			cmzn_streaminformation_cast_region(streaminformation);
		int return_code = cmzn_region_read(region, streaminformation_region);
		cmzn_streamresource_destroy(&resource);
		cmzn_streaminformation_region_destroy(&streaminformation_region);
		cmzn_streaminformation_destroy(&streaminformation);
		return return_code;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_region_write(cmzn_region_id region,
	cmzn_streaminformation_region_id streaminformation_region)
{
	int return_code = CMZN_OK;
	double time = 0.0, stream_time = 0.0;

	if (region && streaminformation_region &&
		(cmzn_streaminformation_region_get_region_private(streaminformation_region) == region))
	{
		const cmzn_stream_properties_list streams_list = streaminformation_region->getResourcesList();
		if (streams_list.empty())
			return_code = CMZN_ERROR_ARGUMENT;
		else
		{
			cmzn_stream_properties_list_const_iterator iter;
			cmzn_resource_properties *stream_properties = NULL;
			cmzn_streamresource_id stream = NULL;
			if (cmzn_streaminformation_region_has_attribute(streaminformation_region,
				CMZN_STREAMINFORMATION_REGION_ATTRIBUTE_TIME))
			{
				time = cmzn_streaminformation_region_get_attribute_real(
					streaminformation_region, CMZN_STREAMINFORMATION_REGION_ATTRIBUTE_TIME);
			}

			char **informationFieldNames = 0;
			int informationNumberOfFieldNames = 0;
			informationNumberOfFieldNames = streaminformation_region->getFieldNames(&informationFieldNames);
			enum cmzn_streaminformation_region_recursion_mode information_recursion_mode =
				streaminformation_region->getRecursionMode();

			for (iter = streams_list.begin(); iter != streams_list.end() && (return_code == CMZN_OK); ++iter)
			{
				stream_properties = *iter;
				stream = stream_properties->getResource();
				if (cmzn_streaminformation_region_has_resource_attribute(
					streaminformation_region, stream, CMZN_STREAMINFORMATION_REGION_ATTRIBUTE_TIME))
				{
					stream_time = cmzn_streaminformation_region_get_resource_attribute_real(
						streaminformation_region, stream, CMZN_STREAMINFORMATION_REGION_ATTRIBUTE_TIME);
				}
				else
				{
					stream_time = time;
				}
				enum FE_write_fields_mode write_fields_mode = FE_WRITE_ALL_FIELDS;
				char **resourceFieldNames = 0, **fieldNames = 0;
				int resourceNumberOfFieldNames = 0, numberOfFieldNames = 0;
				resourceNumberOfFieldNames = streaminformation_region->getResourceFieldNames(stream, &resourceFieldNames);
				if (streaminformation_region->getWriteNoField())
				{
					write_fields_mode = FE_WRITE_NO_FIELDS;
				}
				else
				{
					if (resourceNumberOfFieldNames > 0)
					{
						numberOfFieldNames = resourceNumberOfFieldNames;
						fieldNames = resourceFieldNames;
					}
					else
					{
						numberOfFieldNames = informationNumberOfFieldNames;
						fieldNames = informationFieldNames;
					}
					if (numberOfFieldNames > 0 && fieldNames)
					{
						write_fields_mode = FE_WRITE_LISTED_FIELDS;
					}
				}

				enum cmzn_streaminformation_region_recursion_mode local_recursion_mode =
					streaminformation_region->getResourceRecursionMode(stream);
				if (local_recursion_mode == CMZN_STREAMINFORMATION_REGION_RECURSION_MODE_INVALID)
					local_recursion_mode = information_recursion_mode;
				char *group_name = streaminformation_region->getResourceGroupName(stream);

				cmzn_streamresource_file_id file_resource = cmzn_streamresource_cast_file(stream);
				cmzn_streamresource_memory_id memory_resource = NULL;
				void *memory_block = NULL;
				unsigned int buffer_size = 0;
				int writeElements = 0, writeData = 0, writeNodes = 0;
				int domain_type =	cmzn_streaminformation_region_get_resource_domain_types(
					streaminformation_region, stream);
				if (domain_type == CMZN_FIELD_DOMAIN_TYPE_INVALID)
				{
					writeElements = CMZN_FIELD_DOMAIN_TYPE_MESH1D|CMZN_FIELD_DOMAIN_TYPE_MESH2D|
						CMZN_FIELD_DOMAIN_TYPE_MESH3D|CMZN_FIELD_DOMAIN_TYPE_MESH_HIGHEST_DIMENSION;
					writeData = 1;
					writeNodes = 1;
				}
				else
				{
					if (domain_type & CMZN_FIELD_DOMAIN_TYPE_NODES)
					{
						writeNodes = 1;
					}
					if (domain_type & CMZN_FIELD_DOMAIN_TYPE_DATAPOINTS)
					{
						writeData = 1;
					}
					if (domain_type & CMZN_FIELD_DOMAIN_TYPE_MESH1D)
					{
						writeElements = CMZN_FIELD_DOMAIN_TYPE_MESH1D;
					}
					if (domain_type & CMZN_FIELD_DOMAIN_TYPE_MESH2D)
					{
						writeElements |= CMZN_FIELD_DOMAIN_TYPE_MESH2D;
					}
					if (domain_type & CMZN_FIELD_DOMAIN_TYPE_MESH3D)
					{
						writeElements |= CMZN_FIELD_DOMAIN_TYPE_MESH3D;
					}
					if (domain_type & CMZN_FIELD_DOMAIN_TYPE_MESH_HIGHEST_DIMENSION)
					{
						writeElements |= CMZN_FIELD_DOMAIN_TYPE_MESH_HIGHEST_DIMENSION;
					}
				}
				cmzn_streaminformation_region_file_format fileFormat = streaminformation_region->getFileFormat();
				if (file_resource)
				{
					char *file_name = file_resource->getFileName();
					if (file_name)
					{
						if (fileFormat == CMZN_STREAMINFORMATION_REGION_FILE_FORMAT_AUTOMATIC)
						{
							if (filename_has_FieldML_extension(file_name))
								fileFormat = CMZN_STREAMINFORMATION_REGION_FILE_FORMAT_FIELDML;
							else
								fileFormat = CMZN_STREAMINFORMATION_REGION_FILE_FORMAT_EX;
						}
						switch (fileFormat)
						{
							case CMZN_STREAMINFORMATION_REGION_FILE_FORMAT_EX:
								if (!write_exregion_file_of_name(file_name, region, group_name,
									cmzn_streaminformation_region_get_root_region(streaminformation_region),
									writeElements,	writeNodes, writeData,
									write_fields_mode, numberOfFieldNames, fieldNames,
									stream_time,	FE_WRITE_COMPLETE_GROUP, local_recursion_mode))
								{
									return_code = CMZN_ERROR_GENERAL;
									display_message(ERROR_MESSAGE, "cmzn_region_write.  Failed to write EX file %s", file_name);
								}
								break;
							case CMZN_STREAMINFORMATION_REGION_FILE_FORMAT_FIELDML:
								return_code = write_fieldml_file(region, file_name);
								break;
							case CMZN_STREAMINFORMATION_REGION_FILE_FORMAT_AUTOMATIC:
							case CMZN_STREAMINFORMATION_REGION_FILE_FORMAT_INVALID:
								display_message(ERROR_MESSAGE, "cmzn_region_write.  Invalid file format specified for file %s", file_name);
								return_code = CMZN_ERROR_ARGUMENT;
								break;
						}
						DEALLOCATE(file_name);
					}
					cmzn_streamresource_file_destroy(&file_resource);
				}
				else if (NULL != (memory_resource = cmzn_streamresource_cast_memory(stream)))
				{
					// for now, FILE_FORMAT_AUTOMATIC means EX as no data or resource name to go on
					if (fileFormat == CMZN_STREAMINFORMATION_REGION_FILE_FORMAT_AUTOMATIC)
						fileFormat = CMZN_STREAMINFORMATION_REGION_FILE_FORMAT_EX;
					switch (fileFormat)
					{
						case CMZN_STREAMINFORMATION_REGION_FILE_FORMAT_EX:
							if (!write_exregion_file_to_memory_block(region, group_name,
								cmzn_streaminformation_region_get_root_region(streaminformation_region),
								writeElements,	writeNodes, writeData,
								write_fields_mode, numberOfFieldNames, fieldNames,
								stream_time,	FE_WRITE_COMPLETE_GROUP, local_recursion_mode, &memory_block, &buffer_size))
							{
								return_code = CMZN_ERROR_GENERAL;
								display_message(ERROR_MESSAGE, "cmzn_region_write.  Failed to write EX format to memory block");
							}
							else
							{
								memory_resource->setBuffer(memory_block, buffer_size);
							}
							break;
						case CMZN_STREAMINFORMATION_REGION_FILE_FORMAT_FIELDML:
							display_message(ERROR_MESSAGE, "cmzn_region_write.  Cannot write FieldML to memory block.");
							return_code = CMZN_ERROR_ARGUMENT;
							break;
						case CMZN_STREAMINFORMATION_REGION_FILE_FORMAT_AUTOMATIC:
						case CMZN_STREAMINFORMATION_REGION_FILE_FORMAT_INVALID:
							display_message(ERROR_MESSAGE, "cmzn_region_write.  Invalid file format specified for memory block");
							return_code = CMZN_ERROR_ARGUMENT;
							break;
					}
					cmzn_streamresource_memory_destroy(&memory_resource);
				}
				else
				{
					return_code = CMZN_ERROR_ARGUMENT;
					display_message(ERROR_MESSAGE, "cmzn_region_write.  Unknown resource type");
				}
				if (resourceNumberOfFieldNames > 0)
				{
					char *fieldName = 0;
					for (int i = 0; i < resourceNumberOfFieldNames; i++)
					{
						fieldName = resourceFieldNames[i];
						DEALLOCATE(fieldName);
					}
					DEALLOCATE(resourceFieldNames);
				}
				if (group_name)
					DEALLOCATE(group_name);
			}
			informationNumberOfFieldNames = streaminformation_region->getFieldNames(&informationFieldNames);
			if (informationNumberOfFieldNames > 0)
			{
				char *fieldName = 0;
				for (int i = 0; i < informationNumberOfFieldNames; i++)
				{
					fieldName = informationFieldNames[i];
					DEALLOCATE(fieldName);
				}
				DEALLOCATE(informationFieldNames);
			}

		}
	}
	else
		return_code = CMZN_ERROR_ARGUMENT;
	return return_code;
}

int cmzn_region_write_file(cmzn_region_id region, const char *file_name)
{
	int return_code = 0;
	if (region && file_name)
	{
		cmzn_streaminformation_id streaminformation =
			cmzn_region_create_streaminformation_region(region);
		cmzn_streamresource_id resource = cmzn_streaminformation_create_streamresource_file(
			streaminformation, file_name);
		cmzn_streaminformation_region_id streaminformation_region =
			cmzn_streaminformation_cast_region(streaminformation);
		return_code = cmzn_region_write(region, streaminformation_region);
		cmzn_streamresource_destroy(&resource);
		cmzn_streaminformation_region_destroy(&streaminformation_region);
		cmzn_streaminformation_destroy(&streaminformation);
	}
	return return_code;
}

cmzn_streaminformation_id cmzn_region_create_streaminformation_region(struct cmzn_region *region)
{
	if (region)
	{
		return new cmzn_streaminformation_region(region);
	}
	return NULL;
}

cmzn_streaminformation_region_id cmzn_streaminformation_cast_region(
	cmzn_streaminformation_id streaminformation)
{
	if (streaminformation &&
		(dynamic_cast<cmzn_streaminformation_region *>(streaminformation)))
	{
		streaminformation->access();
		return (reinterpret_cast<cmzn_streaminformation_region *>(streaminformation));
	}
	else
	{
		return (NULL);
	}
}

int cmzn_streaminformation_region_destroy(cmzn_streaminformation_region_id *streaminformation_address)
{
	if (streaminformation_address && *streaminformation_address)
	{
		(*streaminformation_address)->deaccess();
		*streaminformation_address = NULL;
		return 1;
	}
	return 0;
}


bool cmzn_streaminformation_region_has_attribute(
	cmzn_streaminformation_region_id streaminformation,
	enum cmzn_streaminformation_region_attribute attribute)
{
	if (streaminformation)
	{
		switch (attribute)
		{
			case CMZN_STREAMINFORMATION_REGION_ATTRIBUTE_TIME:
			{
				return streaminformation->isTimeEnabled();
			} break;
			default:
			{
			} break;
		}
	}
	return false;
}

double cmzn_streaminformation_region_get_attribute_real(
	cmzn_streaminformation_region_id streaminformation,
	enum cmzn_streaminformation_region_attribute attribute)
{
	double return_value = 0.0;
	if (streaminformation)
	{
		switch (attribute)
		{
			case CMZN_STREAMINFORMATION_REGION_ATTRIBUTE_TIME:
			{
				return_value = streaminformation->getTime();
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"cmzn_streaminformation_region_get_attribute_real.  Invalid attribute");
			} break;
		}
	}
	return return_value;
}

int cmzn_streaminformation_region_set_attribute_real(
	cmzn_streaminformation_region_id streaminformation,
	enum cmzn_streaminformation_region_attribute attribute,
	double value)
{
	int return_code = 0;
	if (streaminformation)
	{
		switch (attribute)
		{
			case CMZN_STREAMINFORMATION_REGION_ATTRIBUTE_TIME:
			{
				return_code = streaminformation->setTime(value);
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"cmzn_streaminformation_region_set_attribute_real.  Invalid attribute");
			} break;
		}
	}
	return return_code;
}

bool cmzn_streaminformation_region_has_resource_attribute(
	cmzn_streaminformation_region_id streaminformation,
	cmzn_streamresource_id resource,
	enum cmzn_streaminformation_region_attribute attribute)
{
	if (streaminformation && resource)
	{
		switch (attribute)
		{
			case CMZN_STREAMINFORMATION_REGION_ATTRIBUTE_TIME:
			{
				return streaminformation->isResourceTimeEnabled(resource);
			} break;
			default:
			{
			} break;
		}
	}
	return false;
}

double cmzn_streaminformation_region_get_resource_attribute_real(
	cmzn_streaminformation_region_id streaminformation,
	cmzn_streamresource_id resource,
	enum cmzn_streaminformation_region_attribute attribute)
{
	double return_value = 0.0;
	if (streaminformation && resource)
	{
		switch (attribute)
		{
			case CMZN_STREAMINFORMATION_REGION_ATTRIBUTE_TIME:
			{
				return_value = streaminformation->getResourceTime(resource);
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"cmzn_streaminformation_region_get_resource_attribute_real.  Invalid attribute");
			} break;
		}
	}
	return return_value;
}

int cmzn_streaminformation_region_set_resource_attribute_real(
	cmzn_streaminformation_region_id streaminformation,
	cmzn_streamresource_id resource,
	enum cmzn_streaminformation_region_attribute attribute,
	double value)
{
	int return_code = 0;
	if (streaminformation && resource)
	{
		switch (attribute)
		{
			case CMZN_STREAMINFORMATION_REGION_ATTRIBUTE_TIME:
			{
				return_code = streaminformation->setResourceTime(resource, value);
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"cmzn_streaminformation_region_set_resource_attribute_real.  Invalid attribute");
			} break;
		}
	}
	return return_code;
}

enum cmzn_streaminformation_region_file_format cmzn_streaminformation_region_get_file_format(
	cmzn_streaminformation_region_id streaminformation)
{
	if (streaminformation)
		return streaminformation->getFileFormat();
	return CMZN_STREAMINFORMATION_REGION_FILE_FORMAT_INVALID;
}

int cmzn_streaminformation_region_set_file_format(
	cmzn_streaminformation_region_id streaminformation,
	enum cmzn_streaminformation_region_file_format file_format)
{
	if (streaminformation)
		return streaminformation->setFileFormat(file_format);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_streaminformation_region_set_field_names(
	cmzn_streaminformation_region_id streaminformation,
	int number_of_names, const char **fieldNames)
{
	if (streaminformation)
		return streaminformation->setFieldNames(number_of_names, fieldNames);
	return CMZN_ERROR_ARGUMENT;
}

cmzn_field_domain_types cmzn_streaminformation_region_get_resource_domain_types(
	cmzn_streaminformation_region_id streaminformation,
	cmzn_streamresource_id resource)
{
	if (streaminformation && resource)
	{
		return streaminformation->getResourceDomainType(resource);
	}
	return (int)CMZN_FIELD_DOMAIN_TYPE_INVALID;
}

int cmzn_streaminformation_region_set_resource_domain_types(
	cmzn_streaminformation_region_id streaminformation,
	cmzn_streamresource_id resource, cmzn_field_domain_types domain_types)
{
	if (streaminformation && resource)
		return streaminformation->setResourceDomainType(resource, domain_types);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_streaminformation_region_set_resource_field_names(
	cmzn_streaminformation_region_id streaminformation,
	cmzn_streamresource_id resource, int number_of_names, const char **fieldNames)
{
	if (streaminformation && resource)
		return streaminformation->setResourceFieldNames(resource, number_of_names, fieldNames);
	return CMZN_ERROR_ARGUMENT;
}

cmzn_region_id cmzn_streaminformation_region_get_region_private(
	cmzn_streaminformation_region_id streaminformation)
{
	return streaminformation->getRegion();
}

cmzn_region_id cmzn_streaminformation_region_get_root_region(
	cmzn_streaminformation_region_id streaminformation)
{
	if (streaminformation)
	{
		return streaminformation->getRootRegion();
	}
	return NULL;
}

enum cmzn_streaminformation_region_attribute
	cmzn_streaminformation_region_attribute_enum_from_string(const char *string)
{
	enum cmzn_streaminformation_region_attribute attribute =
		(cmzn_streaminformation_region_attribute)0;
	if (string)
	{
		const char *str[] = {"TIME"};
		for (unsigned int i = 0; i < 1; i ++)
		{
			if (!strcmp(str[i], string))
			{
				attribute = (cmzn_streaminformation_region_attribute)(i + 1);
				break;
			}
		}
	}
	return attribute;
}

char *cmzn_streaminformation_region_attribute_enum_to_string(
	enum cmzn_streaminformation_region_attribute attribute)
{
	char *string = NULL;
	if (0 < attribute && attribute <= 1)
	{
		const char *str[] = {"TIME"};
		string = duplicate_string(str[attribute - 1]);
	}
	return string;
}

int cmzn_streaminformation_region_set_recursion_mode(
	cmzn_streaminformation_region_id streaminformation,
	enum cmzn_streaminformation_region_recursion_mode recursion_mode)
{
	if (streaminformation)
	{
		return streaminformation->setRecursionMode(recursion_mode);
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_streaminformation_region_set_resource_recursion_mode(
	cmzn_streaminformation_region_id streaminformation,
	cmzn_streamresource_id resource,
	enum cmzn_streaminformation_region_recursion_mode recursion_mode)
{
	if (streaminformation)
	{
		return streaminformation->setResourceRecursionMode(resource, recursion_mode);
	}
	return CMZN_ERROR_ARGUMENT;
}

char *cmzn_streaminformation_region_get_resource_group_name(
	cmzn_streaminformation_region_id streaminformation,
	cmzn_streamresource_id resource)
{
	if (streaminformation)
	{
		return streaminformation->getResourceGroupName(resource);
	}
	return 0;
}

int cmzn_streaminformation_region_set_resource_group_name(
	cmzn_streaminformation_region_id streaminformation,
	cmzn_streamresource_id resource, const char *group_name)
{
	if (streaminformation)
	{
		return streaminformation->setResourceGroupName(resource, group_name);
	}
	return CMZN_ERROR_ARGUMENT;
}
