/***************************************************************************//**
 * FILE : stream_private.hpp
 *
 * The private interface to cmzn_stream.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (CMZN_STREAM_PRIVATE_HPP)
#define CMZN_STREAM_PRIVATE_HPP

#include <list>
#include <stdio.h>
#include <string.h>
#include "opencmiss/zinc/status.h"
#include "opencmiss/zinc/stream.h"
#include "general/debug.h"
#include "general/mystring.h"

struct cmzn_stream_memory_block
{
	const void *memory_buffer;
	unsigned int memory_buffer_size;
	int to_be_deallocated;
};

struct cmzn_streamresource
{
public:

	cmzn_streamresource() :
		access_count(1)
	{
	}

	virtual ~cmzn_streamresource()
	{
	}

	inline cmzn_streamresource *access()
	{
		++access_count;
		return this;
	}

	inline int deaccess()
	{
		--access_count;
		if (1 > access_count)
		{
			delete this;
		}
		return 1;
	}

protected:
	int access_count;
}; /* struct cmzn_io_stream */

struct cmzn_streamresource_file : public cmzn_streamresource
{
public:

	cmzn_streamresource_file(const char *file_name_in) : file_name(duplicate_string(file_name_in))
	{
	}

	virtual ~cmzn_streamresource_file()
	{
		if (file_name)
			DEALLOCATE(file_name);
	}

	char *getFileName()
	{
		return duplicate_string(file_name);
	}

protected:
	char *file_name;
}; /* struct cmzn_input_stream */

struct cmzn_streamresource_memory : public cmzn_streamresource
{
public:

	cmzn_streamresource_memory() : memory_block(new cmzn_stream_memory_block)
	{
		memory_block->to_be_deallocated = 0;
		memory_block->memory_buffer = NULL;
		memory_block->memory_buffer_size = 0;
	}

	cmzn_streamresource_memory(const void *buffer, int buffer_size) :
		memory_block(new cmzn_stream_memory_block)
	{
		memory_block->to_be_deallocated = 0;
		memory_block->memory_buffer = buffer;
		memory_block->memory_buffer_size = buffer_size;
	}

	~cmzn_streamresource_memory()
	{
		if (memory_block)
		{
			if (memory_block->to_be_deallocated && memory_block->memory_buffer)

			{
				DEALLOCATE(memory_block->memory_buffer);
			}
			delete (memory_block);
			memory_block = NULL;
		}
	}

	int getBuffer(const void **buffer_out, unsigned int *buffer_length_out)
	{
		*buffer_out = const_cast<void *>(memory_block->memory_buffer);
		*buffer_length_out = memory_block->memory_buffer_size;
		return 1;
	}

	int getBufferCopy(void **buffer_out, unsigned int *buffer_length_out)
	{
		if (memory_block->memory_buffer && memory_block->memory_buffer_size)
		{
			void *memory_point = NULL;
			ALLOCATE(memory_point, char, memory_block->memory_buffer_size);
			memcpy(memory_point, memory_block->memory_buffer,
				memory_block->memory_buffer_size);
			*buffer_out = memory_point;
			*buffer_length_out = memory_block->memory_buffer_size;
			return 1;
		}
		else
		{
			*buffer_out = NULL;
			*buffer_length_out = 0;
		}
		return 0;
	}

	int setBuffer(void *memory_buffer_reference, unsigned int memory_buffer_sizes)
	{
		memory_block->memory_buffer = memory_buffer_reference;
		memory_block->memory_buffer_size = memory_buffer_sizes;
		memory_block->to_be_deallocated = 1;
		return 1;
	}

protected:

	cmzn_stream_memory_block *memory_block;
};

struct cmzn_streamresource_memory_copy : public cmzn_streamresource_memory
{
public:

	cmzn_streamresource_memory_copy(const void *buffer, int buffer_size) :
		cmzn_streamresource_memory()
	{
		memory_block->to_be_deallocated = 1;
		char *new_block = 0;
		ALLOCATE(new_block, char, buffer_size);
		memcpy (new_block, buffer, buffer_size);
		memory_block->memory_buffer = (void *)new_block;
		memory_block->memory_buffer_size = buffer_size;
	}
};

struct cmzn_resource_properties
{
public:

	cmzn_resource_properties(cmzn_streamresource_id resource_in) : resource(
		cmzn_streamresource_access(resource_in)),
		data_compression_type(CMZN_STREAMINFORMATION_DATA_COMPRESSION_TYPE_DEFAULT)
	{
	}

	~cmzn_resource_properties()
	{
		cmzn_streamresource_destroy(&resource);
	}

	cmzn_streamresource_id getResource()
	{
		if (resource)
			return resource;
		return NULL;
	}

	enum cmzn_streaminformation_data_compression_type getDataCompression()
	{
		return data_compression_type;
	}

	int setDataCompression(enum cmzn_streaminformation_data_compression_type data_compression_type_in)
	{
		data_compression_type = data_compression_type_in;
		return 1;
	}

protected:
	cmzn_streamresource_id resource;
	enum cmzn_streaminformation_data_compression_type data_compression_type;
};

typedef std::list<cmzn_resource_properties *> cmzn_stream_properties_list;
typedef std::list<cmzn_resource_properties *>::const_iterator cmzn_stream_properties_list_const_iterator;

struct cmzn_streaminformation
{
public:

	cmzn_streaminformation() :
		access_count(1),
		data_compression_type(CMZN_STREAMINFORMATION_DATA_COMPRESSION_TYPE_NONE)
	{
	}

	virtual ~cmzn_streaminformation()
	{
		clearResources();
	}

	inline cmzn_streaminformation *access()
	{
		++access_count;
		return this;
	}

	inline int deaccess()
	{
		--access_count;
		if (1 > access_count)
		{
			delete this;
		}
		return 1;
	}

	int appendResourceProperties(cmzn_resource_properties *stream_properties)
	{
		if (stream_properties)
		{
			resources_list.push_back(stream_properties);
			return 1;
		}
		return 0;
	}

	int clearResources()
	{
		std::list<cmzn_resource_properties *>::iterator pos;
		for (pos = resources_list.begin(); pos != resources_list.end(); ++pos)
		{
			cmzn_resource_properties *temp = *pos;
			delete temp;
		}
		resources_list.clear();
		return 1;
	}

	virtual cmzn_resource_properties *createResourceProperties(cmzn_streamresource_id stream)
	{
		if (stream)
			return new cmzn_resource_properties(stream);
		return NULL;
	}

	cmzn_streamresource_id createStreamresourceFile(const char *file_name)
	{
		if (file_name)
		{
			cmzn_streamresource_id stream = new cmzn_streamresource_file(
				file_name);
			appendResourceProperties(createResourceProperties(stream));
			return stream;
		}
		return NULL;
	}

	cmzn_streamresource_id createStreamresourceMemory()
	{
		cmzn_streamresource_id stream = new cmzn_streamresource_memory();
		appendResourceProperties(createResourceProperties(stream));
		return stream;
	}

	cmzn_streamresource_id createStreamresourceMemoryBuffer(const void *memory_block,
		unsigned int block_length)
	{
		if (memory_block && block_length)
		{
			cmzn_streamresource_id stream = new cmzn_streamresource_memory(
				memory_block, block_length);
			appendResourceProperties(createResourceProperties(stream));
			return stream;
		}
		return NULL;
	}

	cmzn_streamresource_id createStreamresourceMemoryBufferCopy(const void *memory_block,
		unsigned int block_length)
	{
		if (memory_block && block_length)
		{
			cmzn_streamresource_id stream = new cmzn_streamresource_memory_copy(
				memory_block, block_length);
			appendResourceProperties(createResourceProperties(stream));
			return stream;
		}
		return NULL;
	}

	inline cmzn_resource_properties *findResourceInList(cmzn_streamresource_id resource_in)
	{
		std::list<cmzn_resource_properties *>::iterator pos;
		for (pos = resources_list.begin(); pos != resources_list.end(); ++pos)
		{
			if ((*pos)->getResource() == resource_in)
			{
				return *pos;
			}
		}
		return NULL;
	}

	const cmzn_stream_properties_list& getResourcesList() const
	{
		return resources_list;
	}

	int removeResource(cmzn_streamresource_id resource_in)
	{
		std::list<cmzn_resource_properties *>::iterator pos;
		for (pos = resources_list.begin(); pos != resources_list.end(); ++pos)
		{
			if ((*pos)->getResource() == resource_in)
			{
				cmzn_resource_properties *properties = *pos;
				resources_list.erase(pos);
				delete properties;
				return 1;
			}
		}
		return 0;
	}

	enum cmzn_streaminformation_data_compression_type getResourceDataCompression(
		cmzn_streamresource_id resource)
	{
		if (resource)
		{
			cmzn_resource_properties *resource_properties =	findResourceInList(resource);
			if (resource_properties)
			{
				return resource_properties->getDataCompression();
			}
		}
		return CMZN_STREAMINFORMATION_DATA_COMPRESSION_TYPE_INVALID;
	}

	int setResourceDataCompression(cmzn_streamresource_id resource,
		enum cmzn_streaminformation_data_compression_type data_compression_type_in)
	{
		if (resource)
		{
			cmzn_resource_properties *resource_properties =	findResourceInList(resource);
			if (resource_properties)
			{
				return resource_properties->setDataCompression(data_compression_type_in);
			}
		}
		return CMZN_ERROR_ARGUMENT;
	}

	enum cmzn_streaminformation_data_compression_type getDataCompression()
	{
		return data_compression_type;
	}

	int setDataCompression(enum cmzn_streaminformation_data_compression_type data_compression_type_in)
	{
		if (data_compression_type_in != CMZN_STREAMINFORMATION_DATA_COMPRESSION_TYPE_DEFAULT)
			data_compression_type = data_compression_type_in;
		else
			return CMZN_ERROR_ARGUMENT;
		return CMZN_OK;
	}

protected:
	int access_count;
	cmzn_stream_properties_list resources_list;
	enum cmzn_streaminformation_data_compression_type data_compression_type;
}; /* struct cmzn_streaminformation */

#endif /* CMZN_STREAM_PRIVATE_HPP */
