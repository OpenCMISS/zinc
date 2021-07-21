/***************************************************************************//**
 * FILE : region_stream.hpp
 *
 * The private interface to cmzn_region_stream.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (CMZN_REGION_STREAM_HPP)
#define CMZN_REGION_STREAM_HPP

#include <string>
#include <vector>
#include <stdlib.h>
#include "opencmiss/zinc/types/fieldid.h"
#include "opencmiss/zinc/region.h"
#include "opencmiss/zinc/status.h"
#include "general/debug.h"
#include "stream/stream_private.hpp"

struct cmzn_region_resource_properties : cmzn_resource_properties
{
public:

	cmzn_region_resource_properties(cmzn_streamresource_id resource_in) :
		cmzn_resource_properties(resource_in), time_enabled(false),
		time(0.0), domain_type((int)CMZN_FIELD_DOMAIN_TYPE_INVALID),
		recursion_mode(CMZN_STREAMINFORMATION_REGION_RECURSION_MODE_INVALID),
		groupName(0)
	{
	}

	~cmzn_region_resource_properties()
	{
		if (groupName)
			DEALLOCATE(groupName);
	}

	double getTime()
	{
		return time;
	}

	bool isTimeEnabled()
	{
		return time_enabled;
	}

	int setTime(double time_in)
	{
		time = time_in;
		time_enabled = true;
		return 1;
	}

	int getDomainType()
	{
		return domain_type;
	}

	int setDomainType(int domain_type_in)
	{
		domain_type = domain_type_in;
		return CMZN_OK;
	}

	enum cmzn_streaminformation_region_recursion_mode getRecursionMode()
	{
		return recursion_mode;
	}

	int setRecursionMode(enum cmzn_streaminformation_region_recursion_mode recursionMode)
	{
		recursion_mode = recursionMode;
		return CMZN_OK;
	}

	int clearFieldNames()
	{
		strings_vectors.clear();
		return CMZN_OK;
	}

	int getFieldNames(char ***fieldNames)
	{
		if (fieldNames)
		{
			*fieldNames = 0;
			if (!strings_vectors.empty())
			{
				const size_t size = strings_vectors.size();
				char **names_array = *fieldNames;
				ALLOCATE(names_array, char *, size);
				std::vector<std::string>::iterator pos;
				int location = 0;
				for (pos = strings_vectors.begin(); pos != strings_vectors.end(); ++pos)
				{
					std::string *my_string = &pos[0];
					names_array[location] = duplicate_string(my_string->c_str());
					location++;
				}
				*fieldNames = names_array;
				return static_cast<int>(strings_vectors.size());
			}
		}
		return 0;
	}

	int setFieldNames(int numberOfNames, const char **fieldNames)
	{
		clearFieldNames();
		for (int i = 0; i < numberOfNames; i++)
		{
			strings_vectors.push_back(std::string(fieldNames[i]));
		}
		return CMZN_OK;
	}

	char *getGroupName()
	{
		if (groupName)
			return duplicate_string(groupName);
		return 0;
	}

	int setGroupName(const char *groupNameIn)
	{
		char *tmp = 0;
		if (groupNameIn)
		{
			tmp = duplicate_string(groupNameIn);
			if (!tmp)
				return CMZN_ERROR_MEMORY;
		}
		if (this->groupName)
			DEALLOCATE(this->groupName);
		this->groupName = tmp;
		return CMZN_OK;
	}

private:
	bool time_enabled;
	double time;
	int domain_type;
	std::vector<std::string> strings_vectors;
	cmzn_streaminformation_region_recursion_mode recursion_mode;
	char *groupName;
};

struct cmzn_streaminformation_region : cmzn_streaminformation
{
public:

	cmzn_streaminformation_region(struct cmzn_region *region_in) :
		time(0.0),
		time_enabled(false),
		region(cmzn_region_access(region_in)),
		root_region(cmzn_region_access(region_in)),
		fileFormat(CMZN_STREAMINFORMATION_REGION_FILE_FORMAT_AUTOMATIC),
		recursion_mode(CMZN_STREAMINFORMATION_REGION_RECURSION_MODE_ON),
		write_no_field(0)
	{
	}

	virtual ~cmzn_streaminformation_region()
	{
		if (root_region)
			cmzn_region_destroy(&root_region);
		if (region)
			cmzn_region_destroy(&region);
	}

	virtual int addResource(cmzn_streamresource_id resourec_in)
	{
		if (resourec_in && !findResourceInList(resourec_in))
		{
			cmzn_region_resource_properties *resource_properties =
				new cmzn_region_resource_properties(resourec_in);
			appendResourceProperties(resource_properties);
			return 1;
		}
		else
		{
			return 0;
		}
	}

	virtual cmzn_resource_properties *createResourceProperties(cmzn_streamresource_id resource)
	{
		if (resource)
			return new cmzn_region_resource_properties(resource);
		return NULL;
	}

	cmzn_streaminformation_region_file_format getFileFormat() const
	{
		return this->fileFormat;
	}

	int setFileFormat(cmzn_streaminformation_region_file_format fileFormatIn)
	{
		if (fileFormatIn == CMZN_STREAMINFORMATION_REGION_FILE_FORMAT_INVALID)
			return CMZN_ERROR_ARGUMENT;
		this->fileFormat = fileFormatIn;
		return CMZN_OK;
	}

	double getTime()
	{
		return time;
	}

	bool isTimeEnabled()
	{
		return time_enabled;
	}

	int setTime(double time_in)
	{
		time = time_in;
		return 1;
	}

	double getResourceTime(cmzn_streamresource_id resource)
	{
		if (resource)
		{
			cmzn_region_resource_properties *resource_properties =
				(cmzn_region_resource_properties *)findResourceInList(resource);
			if (resource_properties)
			{
				if (resource_properties->isTimeEnabled())
					return resource_properties->getTime();
				else
					return time;
			}
		}
		return 0.0;
	}

	bool isResourceTimeEnabled(cmzn_streamresource_id resource)
	{
		if (resource)
		{
			cmzn_region_resource_properties *resource_properties =
				(cmzn_region_resource_properties *)findResourceInList(resource);
			if (resource_properties)
			{
				return resource_properties->isTimeEnabled();
			}
		}
		return false;
	}

	int setResourceTime(cmzn_streamresource_id resource, double time_in)
	{
		if (resource)
		{
			cmzn_region_resource_properties *resource_properties =
				(cmzn_region_resource_properties *)findResourceInList(resource);
			if (resource_properties)
			{
				resource_properties->setTime(time_in);
				return 1;
			}
		}
		return 0;
	}

	int getResourceDomainType(cmzn_streamresource_id resource)
	{
		if (resource)
		{
			cmzn_region_resource_properties *resource_properties =
				(cmzn_region_resource_properties *)findResourceInList(resource);
			if (resource_properties)
			{
				return resource_properties->getDomainType();
			}
		}
		return CMZN_FIELD_DOMAIN_TYPE_INVALID;
	}

	int setResourceDomainType(cmzn_streamresource_id resource, int domain_type_in)
	{
		if (resource)
		{
			cmzn_region_resource_properties *resource_properties =
				(cmzn_region_resource_properties *)findResourceInList(resource);
			if (resource_properties)
			{
				return resource_properties->setDomainType(domain_type_in);
			}
		}
		return CMZN_ERROR_ARGUMENT;
	}

	enum cmzn_streaminformation_region_recursion_mode getRecursionMode()
	{
		return recursion_mode;
	}

	int setRecursionMode(enum cmzn_streaminformation_region_recursion_mode recursionMode)
	{
		if (recursionMode != CMZN_STREAMINFORMATION_REGION_RECURSION_MODE_INVALID)
		{
			recursion_mode = recursionMode;
			return CMZN_OK;
		}
		return CMZN_ERROR_ARGUMENT;
	}

	enum cmzn_streaminformation_region_recursion_mode getResourceRecursionMode(
		cmzn_streamresource_id resource)
	{
		if (resource)
		{
			cmzn_region_resource_properties *resource_properties =
				(cmzn_region_resource_properties *)findResourceInList(resource);
			if (resource_properties)
			{
				return resource_properties->getRecursionMode();
			}
		}
		return CMZN_STREAMINFORMATION_REGION_RECURSION_MODE_INVALID;
	}

	int setResourceRecursionMode(cmzn_streamresource_id resource,
		enum cmzn_streaminformation_region_recursion_mode recursionMode)
	{
		if (resource)
		{
			cmzn_region_resource_properties *resource_properties =
				(cmzn_region_resource_properties *)findResourceInList(resource);
			if (resource_properties)
			{
				return resource_properties->setRecursionMode(recursionMode);
			}
		}
		return CMZN_ERROR_ARGUMENT;
	}

	cmzn_region_id getRegion()
	{
		return region;
	}

	cmzn_region_id getRootRegion()
	{
		return root_region;
	}

	int setRootRegion(cmzn_region_id root_region_in)
	{
		if (root_region != root_region_in)
		{
			if (root_region)
			{
				cmzn_region_destroy(&root_region);
			}
			root_region = cmzn_region_access(root_region_in);
		}
		return 1;
	}

	int clearFieldNames()
	{
		strings_vectors.clear();
		return CMZN_OK;
	}

	int getFieldNames(char ***fieldNames)
	{
		if (fieldNames)
		{
			*fieldNames = 0;
			if (!strings_vectors.empty())
			{
				const size_t size = strings_vectors.size();
				char **names_array = *fieldNames;
				ALLOCATE(names_array, char *, size);
				std::vector<std::string>::iterator pos;
				int location = 0;
				for (pos = strings_vectors.begin(); pos != strings_vectors.end(); ++pos)
				{
					std::string *my_string = &pos[0];
					names_array[location] = duplicate_string(my_string->c_str());
					location++;
				}
				*fieldNames = names_array;
				return static_cast<int>(strings_vectors.size());
			}
		}
		return 0;
	}

	int setFieldNames(int numberOfNames, const char **fieldNames)
	{
		clearFieldNames();
		for (int i = 0; i < numberOfNames; i++)
		{
			strings_vectors.push_back(std::string(fieldNames[i]));
		}
		return CMZN_OK;
	}

	int getResourceFieldNames(cmzn_streamresource_id resource, char ***fieldNames)
	{
		if (resource)
		{
			cmzn_region_resource_properties *resource_properties =
				(cmzn_region_resource_properties *)findResourceInList(resource);
			if (resource_properties)
			{
				return resource_properties->getFieldNames(fieldNames);
			}
		}
		return 0;
	}

	int setResourceFieldNames(cmzn_streamresource_id resource, int numberOfNames, const char **fieldNames)
	{
		if (resource)
		{
			cmzn_region_resource_properties *resource_properties =
				(cmzn_region_resource_properties *)findResourceInList(resource);
			if (resource_properties)
			{
				return resource_properties->setFieldNames(numberOfNames, fieldNames);
			}
		}
		return CMZN_ERROR_ARGUMENT;
	}

	char *getResourceGroupName(cmzn_streamresource_id resource)
	{
		if (resource)
		{
			cmzn_region_resource_properties *resource_properties =
				(cmzn_region_resource_properties *)findResourceInList(resource);
			if (resource_properties)
			{
				return resource_properties->getGroupName();
			}
		}
		return 0;
	}

	int setResourceGroupName(cmzn_streamresource_id resource, const char *groupName)
	{
		if (resource)
		{
			cmzn_region_resource_properties *resource_properties =
				(cmzn_region_resource_properties *)findResourceInList(resource);
			if (resource_properties)
			{
				return resource_properties->setGroupName(groupName);
			}
		}
		return CMZN_ERROR_ARGUMENT;
	}

	int getWriteNoField()
	{
		return write_no_field;
	}

	int setWriteNoField(int writeNoField)
	{
		write_no_field = writeNoField;
		return CMZN_OK;
	}

private:
	double time;
	bool time_enabled;
	struct cmzn_region *region, *root_region;
	cmzn_streaminformation_region_file_format fileFormat;
	std::vector<std::string> strings_vectors;
	cmzn_streaminformation_region_recursion_mode recursion_mode;
	int write_no_field;
};

cmzn_region_id cmzn_streaminformation_region_get_region_private(
	cmzn_streaminformation_region_id streaminformation);

cmzn_region_id cmzn_streaminformation_region_get_root_region(
	cmzn_streaminformation_region_id streaminformation);

#endif /* CMZN_REGION_STREAM_HPP */
