/***************************************************************************//**
 * FILE : scene_stream.cpp
 *
 * The definition to cmzn_streaminformation_scene.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "opencmiss/zinc/streamscene.h"
#include "general/debug.h"
#include "general/mystring.h"
#include "general/message.h"
#include "general/enumerator_conversion.hpp"
#include "description_io/scene_json_export.hpp"
#include "description_io/scene_json_import.hpp"
#include "stream/scene_stream.hpp"



int cmzn_scene_write(cmzn_scene_id scene,
	cmzn_streaminformation_scene_id streaminformation_scene)
{
	int return_code = CMZN_OK;

	if (scene && streaminformation_scene &&
		streaminformation_scene->getIOFormat() != CMZN_STREAMINFORMATION_SCENE_IO_FORMAT_INVALID)
	{
		const cmzn_stream_properties_list streams_list = streaminformation_scene->getResourcesList();
		if (!(streams_list.empty()))
		{
			cmzn_stream_properties_list_const_iterator iter;
			cmzn_resource_properties *stream_properties = NULL;
			int number_of_entries = 0;
			std::string *output_string = 0;

			cmzn_scene_id scene = streaminformation_scene->getScene();
			if (streaminformation_scene->getIOFormat() == CMZN_STREAMINFORMATION_SCENE_IO_FORMAT_THREEJS)
			{
				const int size = static_cast<int>(streams_list.size());
				char **resource_names = new char *[size];
				int current_index = 0;
				for (iter = streams_list.begin(); iter != streams_list.end(); ++iter)
				{
					stream_properties = *iter;
					cmzn_streamresource_id stream = stream_properties->getResource();

					cmzn_streamresource_file_id file_resource = cmzn_streamresource_cast_file(stream);
					if (file_resource)
					{
						resource_names[current_index] = duplicate_string(file_resource->getFileName());
					}
					else
					{
						char temp_string[50];
						sprintf(temp_string, "memory_resource_%d", current_index + 1);
						resource_names[current_index] = duplicate_string(temp_string);
					}
					current_index++;
				}
				cmzn_scenefilter_id scenefilter = streaminformation_scene->getScenefilter();
				return_code = Scene_render_threejs(scene,
					scenefilter, /*file_prefix*/"zinc_scene_export",
					streaminformation_scene->getNumberOfTimeSteps(),
					streaminformation_scene->getInitialTime(),
					streaminformation_scene->getFinishTime(),
					streaminformation_scene->getIODataType(),
					&number_of_entries, &output_string,
					streaminformation_scene->getOutputTimeDependentVertices(),
					streaminformation_scene->getOutputTimeDependentColours(),
					streaminformation_scene->getOutputTimeDependentNormals(),
					size,	resource_names,
					streaminformation_scene->getOutputIsInline());
				cmzn_scenefilter_destroy(&scenefilter);
				for (int i = 0; i < size; i++)
				{
					char *temp_name = resource_names[i];
					DEALLOCATE(temp_name);
				}
				delete[] resource_names;
			}
			else if (streaminformation_scene->getIOFormat() == CMZN_STREAMINFORMATION_SCENE_IO_FORMAT_DESCRIPTION)
			{
				number_of_entries = 1;
				SceneJsonExport jsonExport(scene);
				output_string = new std::string[number_of_entries];
				output_string[0] = jsonExport.getExportString();
			}
			cmzn_scene_destroy(&scene);

			if (return_code != CMZN_OK)
				return CMZN_ERROR_GENERAL;

			cmzn_streamresource_id stream = NULL;
			int i = 0;
			for (iter = streams_list.begin(); iter != streams_list.end(); ++iter)
			{
				stream_properties = *iter;
				stream = stream_properties->getResource();

				cmzn_streamresource_file_id file_resource = cmzn_streamresource_cast_file(stream);
				cmzn_streamresource_memory_id memory_resource = NULL;
				if (number_of_entries > i)
				{
					if (file_resource)
					{
						char *file_name = file_resource->getFileName();
						if (file_name)
						{
							FILE *export_file = fopen(file_name,"w");
							fprintf(export_file, "%s", output_string[i].c_str());
							fclose(export_file);
							DEALLOCATE(file_name);
							i++;
						}
						cmzn_streamresource_file_destroy(&file_resource);
					}
					else if (NULL != (memory_resource = cmzn_streamresource_cast_memory(stream)))
					{
						char *buffer_out = duplicate_string(output_string[i].c_str());
						unsigned int buffer_size = static_cast<unsigned int>(strlen(buffer_out));
						memory_resource->setBuffer(buffer_out, buffer_size);
						cmzn_streamresource_memory_destroy(&memory_resource);
						i++;
					}
					else
					{
						return_code = 0;
						display_message(ERROR_MESSAGE, "cmzn_scene_export. Stream error");
					}
				}
				if (!return_code)
				{
					break;
				}
			}
			if (output_string)
				delete[] output_string;
		}
	}
	else
	{
		return_code = CMZN_ERROR_ARGUMENT;
	}
	return return_code;
}

int cmzn_scene_read(cmzn_scene_id scene,
	cmzn_streaminformation_scene_id streaminformation_scene)
{
	int return_code = CMZN_OK;

	if (scene && streaminformation_scene &&
		streaminformation_scene->getIOFormat() == CMZN_STREAMINFORMATION_SCENE_IO_FORMAT_DESCRIPTION)
	{
		const cmzn_stream_properties_list streams_list = streaminformation_scene->getResourcesList();
		if (!(streams_list.empty()))
		{
			int overwrite = streaminformation_scene->getOverwriteSceneGraphics();

			cmzn_stream_properties_list_const_iterator iter;
			cmzn_resource_properties *stream_properties = NULL;

			cmzn_scene_id scene = streaminformation_scene->getScene();

			cmzn_streamresource_id stream = NULL;

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
						FILE *import_file = fopen(file_name,"rb");
						fseek(import_file, 0, SEEK_END);
						long size = ftell(import_file);
						fseek(import_file, 0, SEEK_SET);
						void *source = malloc(size + 1);
						if (source)
						{
							long sizeRead = static_cast<long>(fread(source, size, 1, import_file));
							fclose(import_file);
							if (sizeRead == size)
							{
								char *jsonString = (char *)source;
								jsonString[size] = 0;
								SceneJsonImport sceneImport(scene, overwrite);
								std::string inputString(jsonString);
								return_code = sceneImport.import(inputString);
							}
							else
							{
								return_code = CMZN_ERROR_GENERAL;
							}
							free(source);
						}
						else
						{
							return_code = CMZN_ERROR_GENERAL;
						}
						overwrite = 0;
					}
					cmzn_streamresource_file_destroy(&file_resource);
				}
				else if (NULL != (memory_resource = cmzn_streamresource_cast_memory(stream)))
				{
					const void *memory_block = NULL;
					unsigned int buffer_size = 0;
					memory_resource->getBuffer(&memory_block, &buffer_size);
					cmzn_streamresource_memory_destroy(&memory_resource);
					if (memory_block)
					{
						SceneJsonImport sceneImport(scene, overwrite);
						const char *jsonString = static_cast<const char *>(memory_block);
						std::string inputString(jsonString);
						return_code = sceneImport.import(inputString);
						overwrite = 0;
					}
					else
					{
						return_code = CMZN_ERROR_GENERAL;
					}
				}
			}
			cmzn_scene_destroy(&scene);
		}
	}
	else
	{
		return_code = CMZN_ERROR_ARGUMENT;
	}
	return return_code;
}


cmzn_streaminformation_id cmzn_scene_create_streaminformation_scene(
	cmzn_scene_id scene)
{
	if (scene)
	{
		return new cmzn_streaminformation_scene(scene);
	}
	return 0;
}

int cmzn_streaminformation_scene_destroy(
	cmzn_streaminformation_scene_id *streaminformation_address)
{
	if (streaminformation_address && *streaminformation_address)
	{
		(*streaminformation_address)->deaccess();
		*streaminformation_address = NULL;
		return 1;
	}
	return 0;
}

cmzn_streaminformation_scene_id cmzn_streaminformation_cast_scene(
	cmzn_streaminformation_id streaminformation)
{
	if (streaminformation &&
		(dynamic_cast<cmzn_streaminformation_scene *>(streaminformation)))
	{
		streaminformation->access();
		return (reinterpret_cast<cmzn_streaminformation_scene *>(streaminformation));
	}
	return 0;
}

int cmzn_streaminformation_scene_get_number_of_resources_required(cmzn_streaminformation_scene_id streaminformation)
{
	if (streaminformation)
	{
		return streaminformation->getNumberOfResourcesRequired();
	}
	return 0;
}

double cmzn_streaminformation_scene_get_initial_time(cmzn_streaminformation_scene_id streaminformation)
{
	if (streaminformation)
	{
		return streaminformation->getInitialTime();
	}
	return 0;
}

int cmzn_streaminformation_scene_set_initial_time(cmzn_streaminformation_scene_id streaminformation,
	double initialTime)
{
	if (streaminformation)
	{
		return streaminformation->setInitialTime(initialTime);
	}
	return CMZN_ERROR_ARGUMENT;
}

double cmzn_streaminformation_scene_get_finish_time(cmzn_streaminformation_scene_id streaminformation)
{
	if (streaminformation)
	{
		return streaminformation->getFinishTime();
	}

	return 0;
}

int cmzn_streaminformation_scene_set_finish_time(cmzn_streaminformation_scene_id streaminformation,
	double finishTime)
{
	if (streaminformation)
	{
		return streaminformation->setFinishTime(finishTime);
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_streaminformation_scene_get_number_of_time_steps(cmzn_streaminformation_scene_id streaminformation)
{
	if (streaminformation)
	{
		return streaminformation->getNumberOfTimeSteps();
	}

	return 0;
}

int cmzn_streaminformation_scene_set_number_of_time_steps(cmzn_streaminformation_scene_id streaminformation,
	int numberOfTimeSteps)
{
	if (streaminformation)
	{
		return streaminformation->setNumberOfTimeSteps(numberOfTimeSteps);
	}
	return CMZN_ERROR_ARGUMENT;
}

enum cmzn_streaminformation_scene_io_format
	cmzn_streaminformation_scene_get_io_format(
	cmzn_streaminformation_scene_id streaminformation)
{
	if (streaminformation)
	{
		return streaminformation->getIOFormat();
	}

	return CMZN_STREAMINFORMATION_SCENE_IO_FORMAT_INVALID;
}

int cmzn_streaminformation_scene_set_io_format(
	cmzn_streaminformation_scene_id streaminformation,
	enum cmzn_streaminformation_scene_io_format format)
{
	if (streaminformation)
	{
		streaminformation->setIOFormat(format);
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

enum cmzn_streaminformation_scene_io_data_type
	cmzn_streaminformation_scene_get_io_data_type(
	cmzn_streaminformation_scene_id streaminformation)
{
	if (streaminformation)
	{
		return streaminformation->getIODataType();
	}

	return CMZN_STREAMINFORMATION_SCENE_IO_DATA_TYPE_INVALID;
}

int cmzn_streaminformation_scene_set_io_data_type(
	cmzn_streaminformation_scene_id streaminformation,
	cmzn_streaminformation_scene_io_data_type data_type)
{
	if (streaminformation)
	{
		streaminformation->setIODataType(data_type);
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

class cmzn_streaminformation_scene_io_format_conversion
{
public:
	static const char *to_string(enum cmzn_streaminformation_scene_io_format format)
	{
		const char *enum_string = 0;
		switch (format)
		{
			case CMZN_STREAMINFORMATION_SCENE_IO_FORMAT_THREEJS:
				enum_string = "THREEJS";
				break;
			default:
				break;
		}
		return enum_string;
	}
};

cmzn_scenefilter_id cmzn_streaminformation_scene_get_scenefilter(
	cmzn_streaminformation_scene_id streaminformation)
{
	if (streaminformation)
	{
		return streaminformation->getScenefilter();
	}
	return 0;
}

int cmzn_streaminformation_scene_set_scenefilter(
	cmzn_streaminformation_scene_id streaminformation,
	cmzn_scenefilter_id scenefilter)
{
	if (streaminformation)
	{
		streaminformation->setScenefilter(scenefilter);
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

enum cmzn_streaminformation_scene_io_format
	cmzn_streaminformation_scene_io_format_enum_from_string(
		const char *string)
{
	return string_to_enum<enum cmzn_streaminformation_scene_io_format,
		cmzn_streaminformation_scene_io_format_conversion>(string);
}

char *cmzn_streaminformation_scene_io_format_enum_to_string(
	enum cmzn_streaminformation_scene_io_format format)
{
	const char *format_string = cmzn_streaminformation_scene_io_format_conversion::to_string(format);
	return (format_string ? duplicate_string(format_string) : 0);
}

int cmzn_streaminformation_scene_set_overwrite_scene_graphics(
	cmzn_streaminformation_scene_id streaminformation, int overwrite)
{
	if (streaminformation)
	{
		streaminformation->setOverwriteSceneGraphics(overwrite);
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_streaminformation_scene_get_output_time_dependent_vertices(
	cmzn_streaminformation_scene_id streaminformation)
{
	if (streaminformation)
	{
		return streaminformation->getOutputTimeDependentVertices();
	}
	return 0;
}

int cmzn_streaminformation_scene_set_output_time_dependent_vertices(
	cmzn_streaminformation_scene_id streaminformation,
	int outputTimeDependentVertices)
{
	if (streaminformation)
	{
		streaminformation->setOutputTimeDependentVertices(outputTimeDependentVertices);
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_streaminformation_scene_get_output_time_dependent_colours(
	cmzn_streaminformation_scene_id streaminformation)
{
	if (streaminformation)
	{
		return streaminformation->getOutputTimeDependentColours();
	}
	return 0;
}

int cmzn_streaminformation_scene_set_output_time_dependent_colours(
	cmzn_streaminformation_scene_id streaminformation,
	int outputTimeDependentColours)
{
	if (streaminformation)
	{
		streaminformation->setOutputTimeDependentColours(outputTimeDependentColours);
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_streaminformation_scene_get_output_time_dependent_normals(
	cmzn_streaminformation_scene_id streaminformation)
{
	if (streaminformation)
	{
		return streaminformation->getOutputTimeDependentNormals();
	}
	return 0;
}

int cmzn_streaminformation_scene_set_output_time_dependent_normals(
	cmzn_streaminformation_scene_id streaminformation,
	int outputTimeDependentNormals)
{
	if (streaminformation)
	{
		streaminformation->setOutputTimeDependentNormals(outputTimeDependentNormals);
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_streaminformation_scene_get_output_is_inline(
	cmzn_streaminformation_scene_id streaminformation)
{
	if (streaminformation)
	{
		return streaminformation->getOutputIsInline();
	}
	return 0;
}

int cmzn_streaminformation_scene_set_output_is_inline(
	cmzn_streaminformation_scene_id streaminformation,
	int outputIsInline)
{
	if (streaminformation)
	{
		streaminformation->setOutputIsInline(outputIsInline);
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}
