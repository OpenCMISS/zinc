/*
 * OpenCMISS-Zinc Library Unit Tests
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <gtest/gtest.h>

#include <zinc/field.hpp>
#include <zinc/fieldcache.hpp>
#include <zinc/fieldconstant.hpp>
#include <zinc/fieldfiniteelement.hpp>
#include <zinc/fieldmeshoperators.hpp>
#include <zinc/fieldmodule.hpp>
#include <zinc/fieldsubobjectgroup.hpp>
#include <zinc/region.hpp>
#include <zinc/streamregion.hpp>

#include "utilities/zinctestsetupcpp.hpp"
#include "utilities/fileio.hpp"

#include "test_resources.h"

#define FIELDML_OUTPUT_FOLDER "fieldmltest"

ManageOutputFolder manageOutputFolderFieldML(FIELDML_OUTPUT_FOLDER);

namespace {

void check_cube_model(Fieldmodule& fm)
{
	int result;
	Field coordinates = fm.findFieldByName("coordinates");
	EXPECT_TRUE(coordinates.isValid());
	EXPECT_EQ(3, coordinates.getNumberOfComponents());
	EXPECT_TRUE(coordinates.isTypeCoordinate());

	Field pressure = fm.findFieldByName("pressure");
	EXPECT_TRUE(pressure.isValid());
	EXPECT_EQ(1, pressure.getNumberOfComponents());
	EXPECT_FALSE(pressure.isTypeCoordinate());

	EXPECT_EQ(OK, result = fm.defineAllFaces());
	Mesh mesh3d = fm.findMeshByDimension(3);
	EXPECT_EQ(1, mesh3d.getSize());
	Mesh mesh2d = fm.findMeshByDimension(2);
	EXPECT_EQ(6, mesh2d.getSize());
	Mesh mesh1d = fm.findMeshByDimension(1);
	EXPECT_EQ(12, mesh1d.getSize());
	Nodeset nodes = fm.findNodesetByFieldDomainType(Field::DOMAIN_TYPE_NODES);
	EXPECT_EQ(8, nodes.getSize());
	Element element = mesh3d.findElementByIdentifier(1);
	EXPECT_TRUE(element.isValid());
	EXPECT_EQ(Element::SHAPE_TYPE_CUBE, element.getShapeType());

	const double valueOne = 1.0;
	Field one = fm.createFieldConstant(1, &valueOne);
	FieldMeshIntegral volume = fm.createFieldMeshIntegral(one, coordinates, mesh3d);
	const int numberOfPoints = 2;
	EXPECT_EQ(OK, result = volume.setNumbersOfPoints(1, &numberOfPoints));
	FieldMeshIntegral surfacePressureIntegral = fm.createFieldMeshIntegral(pressure, coordinates, mesh2d);
	EXPECT_EQ(OK, result = surfacePressureIntegral.setNumbersOfPoints(1, &numberOfPoints));

	Fieldcache cache = fm.createFieldcache();
	double outVolume;
	EXPECT_EQ(OK, result = volume.evaluateReal(cache, 1, &outVolume));
	ASSERT_DOUBLE_EQ(1.0, outVolume);
	double outSurfacePressureIntegral;
	EXPECT_EQ(OK, result = surfacePressureIntegral.evaluateReal(cache, 1, &outSurfacePressureIntegral));
	ASSERT_DOUBLE_EQ(540000.0, outSurfacePressureIntegral);
}

}

// cube model defines a 3-D RC coordinates field and 1-D pressure field
// using the same trilinear Lagrange scalar template.
// field dofs and mesh nodes connectivity are inline text in the fieldml document
TEST(ZincRegion, read_fieldml_cube)
{
	ZincTestSetupCpp zinc;
	int result;

	EXPECT_EQ(OK, result = zinc.root_region.readFile(
		TestResources::getLocation(TestResources::FIELDIO_FIELDML_CUBE_RESOURCE)));
	check_cube_model(zinc.fm);

	// test writing and re-reading into different region
	EXPECT_EQ(OK, result = zinc.root_region.writeFile(FIELDML_OUTPUT_FOLDER "/cube.fieldml"));
	Region testRegion = zinc.root_region.createChild("test");
	EXPECT_EQ(OK, result = testRegion.readFile(FIELDML_OUTPUT_FOLDER "/cube.fieldml"));
	Fieldmodule testFm = testRegion.getFieldmodule();
	check_cube_model(testFm);
}

// Also reads cube model, but tries to read it as EX format which should fail
TEST(ZincStreaminformationRegion, fileFormat)
{
	ZincTestSetupCpp zinc;
	int result;

	StreaminformationRegion streamInfo = zinc.root_region.createStreaminformationRegion();
	EXPECT_TRUE(streamInfo.isValid());
	StreamresourceFile fileResource = streamInfo.createStreamresourceFile(
		TestResources::getLocation(TestResources::FIELDIO_FIELDML_CUBE_RESOURCE));
	EXPECT_TRUE(fileResource.isValid());
	StreaminformationRegion::FileFormat fileFormat = streamInfo.getFileFormat();
	EXPECT_EQ(StreaminformationRegion::FILE_FORMAT_AUTOMATIC, fileFormat);
	EXPECT_EQ(OK, result = streamInfo.setFileFormat(StreaminformationRegion::FILE_FORMAT_EX));
	EXPECT_EQ(StreaminformationRegion::FILE_FORMAT_EX, fileFormat = streamInfo.getFileFormat());
	result = zinc.root_region.read(streamInfo);
	EXPECT_EQ(ERROR_GENERAL, result); // not in EX format
	EXPECT_EQ(OK, result = streamInfo.setFileFormat(StreaminformationRegion::FILE_FORMAT_FIELDML));
	EXPECT_EQ(StreaminformationRegion::FILE_FORMAT_FIELDML, fileFormat = streamInfo.getFileFormat());
	result = zinc.root_region.read(streamInfo);
	EXPECT_EQ(OK, result); // in FieldML format
	check_cube_model(zinc.fm);
}

namespace {

void check_tetmesh_model(Fieldmodule& fm)
{
	int result;
	Field coordinates = fm.findFieldByName("coordinates");
	EXPECT_TRUE(coordinates.isValid());
	EXPECT_EQ(3, coordinates.getNumberOfComponents());
	EXPECT_TRUE(coordinates.isTypeCoordinate());

	EXPECT_EQ(OK, result = fm.defineAllFaces());
	Mesh mesh3d = fm.findMeshByDimension(3);
	EXPECT_EQ(102, mesh3d.getSize());
	Mesh mesh2d = fm.findMeshByDimension(2);
	EXPECT_EQ(232, mesh2d.getSize());
	Mesh mesh1d = fm.findMeshByDimension(1);
	EXPECT_EQ(167, mesh1d.getSize());
	Nodeset nodes = fm.findNodesetByFieldDomainType(Field::DOMAIN_TYPE_NODES);
	EXPECT_EQ(38, nodes.getSize());
	for (int i = 1; i < 102; ++i)
	{
		Element element = mesh3d.findElementByIdentifier(i);
		EXPECT_TRUE(element.isValid());
		Element::ShapeType shapeType = element.getShapeType();
		EXPECT_EQ(Element::SHAPE_TYPE_TETRAHEDRON, shapeType);
	}

	const double valueOne = 1.0;
	Field one = fm.createFieldConstant(1, &valueOne);
	FieldMeshIntegral volume = fm.createFieldMeshIntegral(one, coordinates, mesh3d);
	EXPECT_TRUE(volume.isValid());

	FieldElementGroup exteriorFacesGroup = fm.createFieldElementGroup(mesh2d);
	EXPECT_TRUE(exteriorFacesGroup.isValid());
	EXPECT_EQ(OK, result = exteriorFacesGroup.setManaged(true));
	MeshGroup exteriorFacesMeshGroup = exteriorFacesGroup.getMeshGroup();
	EXPECT_TRUE(exteriorFacesMeshGroup.isValid());
	FieldIsExterior isExterior = fm.createFieldIsExterior();
	EXPECT_TRUE(isExterior.isValid());
	exteriorFacesMeshGroup.addElementsConditional(isExterior);
	EXPECT_EQ(56, exteriorFacesMeshGroup.getSize());
	FieldMeshIntegral surfaceArea = fm.createFieldMeshIntegral(one, coordinates, exteriorFacesMeshGroup);
	EXPECT_TRUE(surfaceArea.isValid());

	Fieldcache cache = fm.createFieldcache();
	double outVolume;
	EXPECT_EQ(OK, result = volume.evaluateReal(cache, 1, &outVolume));
	ASSERT_DOUBLE_EQ(0.41723178864303812, outVolume);
	double outSurfaceArea;
	EXPECT_EQ(OK, result = surfaceArea.evaluateReal(cache, 1, &outSurfaceArea));
	ASSERT_DOUBLE_EQ(2.7717561493468423, outSurfaceArea);
}

}

// tetmesh model defines a 3-D RC coordinates field over a tetrahedral
// mesh in approximate unit sphere shape with trilinearSimplex basis/
// node coordinates and connectivity are read from separate files
TEST(ZincRegion, read_fieldml_tetmesh)
{
	ZincTestSetupCpp zinc;
	int result;

	EXPECT_EQ(OK, result = zinc.root_region.readFile(
		TestResources::getLocation(TestResources::FIELDIO_FIELDML_TETMESH_RESOURCE)));
	check_tetmesh_model(zinc.fm);

	// check can't merge cube model since it redefines element 1 shape
	result = zinc.root_region.readFile(
		TestResources::getLocation(TestResources::FIELDIO_FIELDML_CUBE_RESOURCE));
	EXPECT_EQ(ERROR_INCOMPATIBLE_DATA, result);

	// test writing and re-reading into different region
	EXPECT_EQ(OK, result = zinc.root_region.writeFile(FIELDML_OUTPUT_FOLDER "/tetmesh.fieldml"));
	Region testRegion = zinc.root_region.createChild("test");
	EXPECT_EQ(OK, result = testRegion.readFile(FIELDML_OUTPUT_FOLDER "/tetmesh.fieldml"));
	Fieldmodule testFm = testRegion.getFieldmodule();
	check_tetmesh_model(testFm);
}

namespace {

void check_wheel_model(Fieldmodule& fm)
{
	int result;
	Field coordinates = fm.findFieldByName("coordinates");
	EXPECT_TRUE(coordinates.isValid());
	EXPECT_EQ(3, coordinates.getNumberOfComponents());
	EXPECT_TRUE(coordinates.isTypeCoordinate());

	EXPECT_EQ(OK, result = fm.defineAllFaces());
	Mesh mesh3d = fm.findMeshByDimension(3);
	EXPECT_EQ(12, mesh3d.getSize());
	Mesh mesh2d = fm.findMeshByDimension(2);
	EXPECT_EQ(48, mesh2d.getSize());
	Mesh mesh1d = fm.findMeshByDimension(1);
	EXPECT_EQ(61, mesh1d.getSize());
	Nodeset nodes = fm.findNodesetByFieldDomainType(Field::DOMAIN_TYPE_NODES);
	EXPECT_EQ(129, nodes.getSize());
	for (int i = 1; i < 12; ++i)
	{
		Element element = mesh3d.findElementByIdentifier(i);
		EXPECT_TRUE(element.isValid());
		Element::ShapeType shapeType = element.getShapeType();
		if (i <= 6)
			EXPECT_EQ(Element::SHAPE_TYPE_WEDGE12, shapeType);
		else
			EXPECT_EQ(Element::SHAPE_TYPE_CUBE, shapeType);
	}

	const double valueOne = 1.0;
	Field one = fm.createFieldConstant(1, &valueOne);
	FieldMeshIntegral volume = fm.createFieldMeshIntegral(one, coordinates, mesh3d);
	EXPECT_TRUE(volume.isValid());
	const int pointCount = 2;
	EXPECT_EQ(OK, result = volume.setNumbersOfPoints(1, &pointCount));

	FieldElementGroup exteriorFacesGroup = fm.createFieldElementGroup(mesh2d);
	EXPECT_TRUE(exteriorFacesGroup.isValid());
	EXPECT_EQ(OK, result = exteriorFacesGroup.setManaged(true));
	MeshGroup exteriorFacesMeshGroup = exteriorFacesGroup.getMeshGroup();
	EXPECT_TRUE(exteriorFacesMeshGroup.isValid());
	FieldIsExterior isExterior = fm.createFieldIsExterior();
	EXPECT_TRUE(isExterior.isValid());
	exteriorFacesMeshGroup.addElementsConditional(isExterior);
	EXPECT_EQ(30, exteriorFacesMeshGroup.getSize());
	FieldMeshIntegral surfaceArea = fm.createFieldMeshIntegral(one, coordinates, exteriorFacesMeshGroup);
	EXPECT_TRUE(surfaceArea.isValid());
	EXPECT_EQ(OK, result = surfaceArea.setNumbersOfPoints(1, &pointCount));

	Fieldcache cache = fm.createFieldcache();
	double outVolume;
	EXPECT_EQ(OK, result = volume.evaluateReal(cache, 1, &outVolume));
	ASSERT_DOUBLE_EQ(100.28718664065387, outVolume);
	double outSurfaceArea;
	EXPECT_EQ(OK, result = surfaceArea.evaluateReal(cache, 1, &outSurfaceArea));
	ASSERT_DOUBLE_EQ(150.53218306379620, outSurfaceArea);
}

}

// wheel_direct model defines a 3-D RC coordinates field over a wheel mesh
// consisting of 6 wedge elements in the centre, and 6 cube elements around
// them, all coordinates interpolated with triquadratic bases.
// This model tests having variant element shapes and a piecewise field
// template which directly maps element to function (basis + parameter map).
// It also reads shapeids, node coordinates and connectivity (for wedge and
// cube connectivity) from separate files, and the connectivity data uses
// dictionary of keys (DOK) format with key data in the first column of the
// same file.
TEST(ZincRegion, read_fieldml_wheel_direct)
{
	ZincTestSetupCpp zinc;
	int result;
	EXPECT_EQ(OK, result = zinc.root_region.readFile(
		TestResources::getLocation(TestResources::FIELDIO_FIELDML_WHEEL_DIRECT_RESOURCE)));
	check_wheel_model(zinc.fm);
}

// wheel_indirect model is the same as the wheel_direct model except that it
// uses a more efficient indirect element-to-function map
TEST(ZincRegion, read_fieldml_wheel_indirect)
{
	ZincTestSetupCpp zinc;
	int result;
	EXPECT_EQ(OK, result = zinc.root_region.readFile(
		TestResources::getLocation(TestResources::FIELDIO_FIELDML_WHEEL_INDIRECT_RESOURCE)));
	check_wheel_model(zinc.fm);

	// test writing and re-reading into different region
	EXPECT_EQ(OK, result = zinc.root_region.writeFile(FIELDML_OUTPUT_FOLDER "/wheel.fieldml"));
	Region testRegion = zinc.root_region.createChild("test");
	EXPECT_EQ(OK, result = testRegion.readFile(FIELDML_OUTPUT_FOLDER "/wheel.fieldml"));
	Fieldmodule testFm = testRegion.getFieldmodule();
	check_wheel_model(testFm);
}
