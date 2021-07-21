/***************************************************************************//**
 * FILE : scene_stream.hpp
 *
 * The private interface to cmzn_streaminformation_scene.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (CMZN_SCENE_STREAM_HPP)
#define CMZN_SCENE_STREAM_HPP

#include "graphics/scene.hpp"
#include "opencmiss/zinc/scenefilter.h"
#include "opencmiss/zinc/scenepicker.h"
#include "stream/stream_private.hpp"

struct cmzn_streaminformation_scene : cmzn_streaminformation
{
public:

	cmzn_streaminformation_scene(cmzn_scene_id scene_in) : scene(scene_in),
		scenefilter(0), numberOfTimeSteps(0), initialTime(0.0), finishTime(0.0),
		format(CMZN_STREAMINFORMATION_SCENE_IO_FORMAT_INVALID),
		data_type(CMZN_STREAMINFORMATION_SCENE_IO_DATA_TYPE_COLOUR),
		overwriteSceneGraphics(0),  outputTimeDependentVertices(1),
		outputTimeDependentColours(0), outputTimeDependentNormals(0),
		outputIsInline(0)
	{
		cmzn_scene_access(scene_in);
	}

	virtual ~cmzn_streaminformation_scene()
	{
		cmzn_scene_destroy(&scene);
		cmzn_scenefilter_destroy(&scenefilter);
	}

	cmzn_scene_id getScene()
	{
		return cmzn_scene_access(scene);
	}

	double getInitialTime()
	{
		return initialTime;
	}

	int setInitialTime(double initialTimeIn)
	{
		initialTime = initialTimeIn;
		return CMZN_OK;
	}

	double getFinishTime()
	{
		return finishTime;
	}

	int setFinishTime(double finishTimeIn)
	{
		finishTime = finishTimeIn;
		return CMZN_OK;
	}

	int getNumberOfResourcesRequired()
	{
		if (format == CMZN_STREAMINFORMATION_SCENE_IO_FORMAT_THREEJS)
		{
			if (outputIsInline)
			{
				return 1;
			}
			/* multiply the following by 2, each glyph export also requires an extra resource
			 * with informations on the transformation.
			 */
			int numberOfResources = Scene_get_number_of_exportable_glyph_resources(scene, scenefilter);
			numberOfResources += Scene_get_number_of_graphics_with_surface_vertices_in_tree(
				scene, scenefilter);
			numberOfResources += Scene_get_number_of_graphics_with_line_vertices_in_tree(
				scene, scenefilter);
			/* An additional resources on new metadata file describing each graphics. */
			if (numberOfResources > 0)
				numberOfResources += 1;
			return numberOfResources;
		}
		else if (format == CMZN_STREAMINFORMATION_SCENE_IO_FORMAT_DESCRIPTION)
			return 1;
		else
			return 0;
	}

	int getNumberOfTimeSteps()
	{
		return numberOfTimeSteps;
	}

	int setNumberOfTimeSteps(int numberOfTimeStepsIn)
	{
		numberOfTimeSteps = numberOfTimeStepsIn;
		return CMZN_OK;
	}

	cmzn_scenefilter_id getScenefilter()
	{
		if (scenefilter)
			return cmzn_scenefilter_access(scenefilter);
		return 0;
	}

	int setScenefilter(cmzn_scenefilter_id scenefilter_in)
	{
		if (scenefilter)
			cmzn_scenefilter_destroy(&scenefilter);
		scenefilter = cmzn_scenefilter_access(scenefilter_in);
		return CMZN_OK;
	}

	cmzn_streaminformation_scene_io_format getIOFormat()
	{
		return format;
	}

	int setIOFormat(cmzn_streaminformation_scene_io_format formatIn)
	{
		format = formatIn;
		return CMZN_OK;
	}

	cmzn_streaminformation_scene_io_data_type getIODataType()
	{
		return data_type;
	}

	int setIODataType(cmzn_streaminformation_scene_io_data_type dataTypeIn)
	{
		data_type = dataTypeIn;
		return CMZN_OK;
	}

	int setOverwriteSceneGraphics(int overwrite)
	{
		overwriteSceneGraphics = overwrite;
		return CMZN_OK;
	}

	int getOverwriteSceneGraphics()
	{
		return overwriteSceneGraphics;
	}

	int getOutputTimeDependentVertices()
	{
		return outputTimeDependentVertices;
	}

	int setOutputTimeDependentVertices(int outputTimeDependentVerticesIn)
	{
		outputTimeDependentVertices = outputTimeDependentVerticesIn;
		return CMZN_OK;
	}

	int getOutputTimeDependentColours()
	{
		return outputTimeDependentColours;
	}

	int setOutputTimeDependentColours(int outputTimeDependentColoursIn)
	{
		outputTimeDependentColours = outputTimeDependentColoursIn;
		return CMZN_OK;
	}

	int getOutputTimeDependentNormals()
	{
		return outputTimeDependentNormals;
	}

	int setOutputTimeDependentNormals(int outputTimeDependentNormalsIn)
	{
		outputTimeDependentNormals = outputTimeDependentNormalsIn;
		return CMZN_OK;
	}

	int getOutputIsInline()
	{
		return outputIsInline;
	}

	int setOutputIsInline(int outputIsInlineIn)
	{
		outputIsInline = outputIsInlineIn;
		return CMZN_OK;
	}

private:
	cmzn_scene_id scene;
	cmzn_scenefilter_id scenefilter;
	int numberOfTimeSteps;
	double initialTime, finishTime;
	enum cmzn_streaminformation_scene_io_format format;
	enum cmzn_streaminformation_scene_io_data_type data_type;
	int overwriteSceneGraphics;
	int outputTimeDependentVertices, outputTimeDependentColours, outputTimeDependentNormals,
		outputIsInline;
};


#endif /* CMZN_REGION_STREAM_HPP */
