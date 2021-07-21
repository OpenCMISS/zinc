/***************************************************************************//**
 * FILE : stream_private.cpp
 *
 * The definition to cmzn_stream.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "opencmiss/zinc/stream.h"
#include "general/debug.h"
#include "general/mystring.h"
#include "stream/stream_private.hpp"

cmzn_streamresource_id cmzn_streamresource_access(cmzn_streamresource_id resource)
{
	if (resource)
		return resource->access();
	return 0;
}

int cmzn_streamresource_destroy(cmzn_streamresource_id *resource_address)
{
	if (resource_address && *resource_address)
	{
		(*resource_address)->deaccess();
		*resource_address = NULL;
		return 1;
	}
	return 0;
}

cmzn_streamresource_file_id cmzn_streamresource_cast_file(
	cmzn_streamresource_id resource)
{
	if (resource &&
		(dynamic_cast<cmzn_streamresource_file *>(resource)))
	{
		resource->access();
		return (reinterpret_cast<cmzn_streamresource_file *>(resource));
	}
	else
	{
		return (NULL);
	}
}

int cmzn_streamresource_file_destroy(cmzn_streamresource_file_id *resource_address)
{
	if (resource_address && *resource_address)
	{
		(*resource_address)->deaccess();
		*resource_address = NULL;
		return 1;
	}
	return 0;
}

cmzn_streamresource_memory_id cmzn_streamresource_cast_memory(
	cmzn_streamresource_id resource)
{
	if (resource &&
		(dynamic_cast<cmzn_streamresource_memory *>(resource)))
	{
		resource->access();
		return (reinterpret_cast<cmzn_streamresource_memory *>(resource));
	}
	else
	{
		return (NULL);
	}
}

int cmzn_streamresource_memory_destroy(cmzn_streamresource_memory_id *resource_address)
{
	if (resource_address && *resource_address)
	{
		(*resource_address)->deaccess();
		*resource_address = NULL;
		return 1;
	}
	return 0;
}

int cmzn_streamresource_memory_get_buffer(cmzn_streamresource_memory_id resource,
	const void **buffer_out, unsigned int *buffer_length_out)
{
	if (resource)
	{
		return resource->getBuffer(buffer_out, buffer_length_out);
	}
	return 0;
}

int cmzn_streamresource_memory_get_buffer_copy(cmzn_streamresource_memory_id resource,
	void **buffer_out, unsigned int *buffer_length_out)
{
	if (resource)
	{
		return resource->getBufferCopy(buffer_out, buffer_length_out);
	}
	return 0;
}

char *cmzn_streamresource_file_get_name(cmzn_streamresource_file_id resource)
{
	if (resource)
	{
		return resource->getFileName();
	}
	return 0;
}

cmzn_streaminformation_id cmzn_streaminformation_access(
	cmzn_streaminformation_id streaminformation)
{
	if (streaminformation)
		return streaminformation->access();
	return 0;
}

int cmzn_streaminformation_destroy(cmzn_streaminformation_id *streaminformation_address)
{
	if (streaminformation_address && *streaminformation_address)
	{
		(*streaminformation_address)->deaccess();
		*streaminformation_address = NULL;
		return 1;
	}
	return 0;
}

cmzn_streamresource_id cmzn_streaminformation_create_streamresource_file(
	cmzn_streaminformation_id streaminformation, const char *file_name)
{
	if (streaminformation)
	{
		return streaminformation->createStreamresourceFile(file_name);
	}
	return NULL;
}

cmzn_streamresource_id cmzn_streaminformation_create_streamresource_memory(
	cmzn_streaminformation_id streaminformation)
{
	if (streaminformation)
	{
		return streaminformation->createStreamresourceMemory();
	}
	return NULL;
}

cmzn_streamresource_id cmzn_streaminformation_create_streamresource_memory_buffer(
	cmzn_streaminformation_id streaminformation,
	const void *buffer, unsigned int buffer_length)
{
	if (streaminformation)
	{
		return streaminformation->createStreamresourceMemoryBuffer(buffer,
			buffer_length);
	}
	return NULL;
}

cmzn_streamresource_id cmzn_streaminformation_create_streamresource_memory_buffer_copy(
	cmzn_streaminformation_id streaminformation,
	const void *buffer, unsigned int buffer_length)
{
	if (streaminformation)
	{
		return streaminformation->createStreamresourceMemoryBufferCopy(buffer,
			buffer_length);
	}
	return NULL;
}

enum cmzn_streaminformation_data_compression_type cmzn_streaminformation_get_resource_data_compression_type(
	cmzn_streaminformation_id streaminformation, cmzn_streamresource_id resource)
{
	if (streaminformation && resource)
	{
		return streaminformation->getResourceDataCompression(resource);
	}
	return CMZN_STREAMINFORMATION_DATA_COMPRESSION_TYPE_INVALID;
}

int cmzn_streaminformation_set_resource_data_compression_type(
	cmzn_streaminformation_id streaminformation,	cmzn_streamresource_id resource,
	enum cmzn_streaminformation_data_compression_type data_compression_type)
{
	if (streaminformation && resource)
		return streaminformation->setResourceDataCompression(resource, data_compression_type);
	return CMZN_ERROR_ARGUMENT;
}

enum cmzn_streaminformation_data_compression_type cmzn_streaminformation_get_data_compression_type(
	cmzn_streaminformation_id streaminformation)
{
	if (streaminformation)
	{
		return streaminformation->getDataCompression();
	}
	return CMZN_STREAMINFORMATION_DATA_COMPRESSION_TYPE_INVALID;
}

int cmzn_streaminformation_set_data_compression_type(cmzn_streaminformation_id streaminformation,
	enum cmzn_streaminformation_data_compression_type data_compression_type)
{
	if (streaminformation)
		return streaminformation->setDataCompression(data_compression_type);
	return CMZN_ERROR_ARGUMENT;
}
