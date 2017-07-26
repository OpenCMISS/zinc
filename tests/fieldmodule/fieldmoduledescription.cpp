/*
 * OpenCMISS-Zinc Library Unit Tests
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <gtest/gtest.h>

#include <opencmiss/zinc/core.h>
#include <opencmiss/zinc/context.h>
#include <opencmiss/zinc/element.h>
#include <opencmiss/zinc/field.h>
#include <opencmiss/zinc/fieldcache.h>
#include <opencmiss/zinc/fieldmodule.h>
#include <opencmiss/zinc/fieldarithmeticoperators.h>
#include <opencmiss/zinc/fieldcomposite.h>
#include <opencmiss/zinc/fieldconstant.h>
#include <opencmiss/zinc/fieldcoordinatetransformation.h>
#include <opencmiss/zinc/fieldderivatives.h>
#include <opencmiss/zinc/fieldfiniteelement.h>
#include <opencmiss/zinc/fieldlogicaloperators.h>
#include <opencmiss/zinc/fieldmatrixoperators.h>
#include <opencmiss/zinc/fieldvectoroperators.h>
#include <opencmiss/zinc/fieldtime.h>
#include <opencmiss/zinc/node.h>
#include <opencmiss/zinc/region.h>
#include <opencmiss/zinc/status.h>
#include <opencmiss/zinc/stream.h>
#include <opencmiss/zinc/streamregion.h>
#include <opencmiss/zinc/timekeeper.h>
#include "zinctestsetup.hpp"

#include <string>       // std::string
#include <iostream>     // std::cout, std::ostream, std::hex
#include <sstream>
#include <fstream>

#include "test_resources.h"

void testFields(cmzn_fieldmodule_id fieldmodule)
{
	cmzn_field_id coordinatesField = cmzn_fieldmodule_find_field_by_name(fieldmodule, "coordinates");
	EXPECT_NE(static_cast<cmzn_field *>(0), coordinatesField);

	cmzn_field_id temperatureField = cmzn_fieldmodule_find_field_by_name(fieldmodule, "temperature");
	EXPECT_NE(static_cast<cmzn_field *>(0), temperatureField);

	cmzn_field_id componentField = cmzn_fieldmodule_find_field_by_name(
		fieldmodule, "component");
	EXPECT_NE(static_cast<cmzn_field *>(0), componentField);
	EXPECT_TRUE(cmzn_field_is_managed(componentField));
	EXPECT_EQ(1, cmzn_field_get_number_of_source_fields(componentField));
	EXPECT_EQ(2, cmzn_field_get_number_of_components(componentField));
	cmzn_field_id temp = cmzn_field_get_source_field(componentField, 1);
	EXPECT_EQ(temp, coordinatesField);
	cmzn_field_destroy(&temp);

	cmzn_field_component_id fieldComponent = cmzn_field_cast_component(componentField);
	EXPECT_NE(static_cast<cmzn_field_component *>(0), fieldComponent);
	EXPECT_EQ(2, cmzn_field_component_get_source_component_index(fieldComponent, 1));
	EXPECT_EQ(1, cmzn_field_component_get_source_component_index(fieldComponent, 2));
	cmzn_field_component_destroy(&fieldComponent);

	cmzn_field_id firstComponentField = cmzn_fieldmodule_find_field_by_name(
		fieldmodule, "firstComponent");
	EXPECT_NE(static_cast<cmzn_field *>(0), firstComponentField);
	EXPECT_TRUE(cmzn_field_is_managed(firstComponentField));
	EXPECT_EQ(1, cmzn_field_get_number_of_source_fields(firstComponentField));
	EXPECT_EQ(1, cmzn_field_get_number_of_components(firstComponentField));
	temp = cmzn_field_get_source_field(firstComponentField, 1);
	EXPECT_EQ(temp, coordinatesField);
	cmzn_field_destroy(&temp);

	cmzn_field_id addField = cmzn_fieldmodule_find_field_by_name(
		fieldmodule, "add");
	EXPECT_NE(static_cast<cmzn_field *>(0), addField);
	EXPECT_TRUE(cmzn_field_is_managed(addField));
	EXPECT_EQ(2, cmzn_field_get_number_of_source_fields(addField));
	EXPECT_EQ(1, cmzn_field_get_number_of_components(addField));
	temp = cmzn_field_get_source_field(addField, 1);
	EXPECT_EQ(temp, firstComponentField);
	cmzn_field_destroy(&temp);
	temp = cmzn_field_get_source_field(addField, 2);
	EXPECT_EQ(temp, temperatureField);
	cmzn_field_destroy(&temp);

	cmzn_field_id concatenateField = cmzn_fieldmodule_find_field_by_name(
		fieldmodule, "concatenate");
	EXPECT_NE(static_cast<cmzn_field *>(0), concatenateField);
	EXPECT_TRUE(cmzn_field_is_managed(concatenateField));
	EXPECT_EQ(2, cmzn_field_get_number_of_source_fields(concatenateField));
	EXPECT_EQ(4, cmzn_field_get_number_of_components(concatenateField));
	temp = cmzn_field_get_source_field(concatenateField, 1);
	EXPECT_EQ(temp, coordinatesField);
	cmzn_field_destroy(&temp);
	temp = cmzn_field_get_source_field(concatenateField, 2);
	EXPECT_EQ(temp, temperatureField);
	cmzn_field_destroy(&temp);

	cmzn_fieldcache_id fieldCache = cmzn_fieldmodule_create_fieldcache(fieldmodule);

	cmzn_field_id constantField = cmzn_fieldmodule_find_field_by_name(
		fieldmodule, "constant");
	EXPECT_NE(static_cast<cmzn_field *>(0), constantField);
	EXPECT_TRUE(cmzn_field_is_managed(constantField));
	EXPECT_EQ(0, cmzn_field_get_number_of_source_fields(constantField));
	EXPECT_EQ(3, cmzn_field_get_number_of_components(constantField));
	double values3[3] = {0, 0, 0};
	EXPECT_EQ(CMZN_OK, cmzn_field_evaluate_real(constantField, fieldCache, 3, &values3[0]));
	EXPECT_EQ(1.0, values3[0]);
	EXPECT_EQ(4.0, values3[1]);
	EXPECT_EQ(2.0, values3[2]);

	cmzn_field_id stringConstantField = cmzn_fieldmodule_find_field_by_name(
		fieldmodule,"stringConstant");
	EXPECT_NE(static_cast<cmzn_field *>(0), stringConstantField);
	EXPECT_TRUE(cmzn_field_is_managed(stringConstantField));
	EXPECT_EQ(0, cmzn_field_get_number_of_source_fields(stringConstantField));
	EXPECT_EQ(1, cmzn_field_get_number_of_components(stringConstantField));
	char *returned_string = cmzn_field_evaluate_string(stringConstantField, fieldCache);
	EXPECT_STREQ(returned_string, "string_constant");

	cmzn_field_id andField =  cmzn_fieldmodule_find_field_by_name(fieldmodule,
		"and");
	EXPECT_NE(static_cast<cmzn_field *>(0), andField);
	EXPECT_TRUE(cmzn_field_is_managed(andField));
	EXPECT_EQ(2, cmzn_field_get_number_of_source_fields(andField));

	cmzn_field_id edgeDiscontinuityField = cmzn_fieldmodule_find_field_by_name(
		fieldmodule, "edgeDiscontinuity");
	EXPECT_NE(static_cast<cmzn_field *>(0), edgeDiscontinuityField);
	EXPECT_TRUE(cmzn_field_is_managed(edgeDiscontinuityField));
	EXPECT_EQ(1, cmzn_field_get_number_of_source_fields(edgeDiscontinuityField));

	cmzn_field_id sumComponentField = cmzn_fieldmodule_find_field_by_name(
		fieldmodule, "sumComponent");
	EXPECT_NE(static_cast<cmzn_field *>(0), sumComponentField);
	EXPECT_TRUE(cmzn_field_is_managed(sumComponentField));
	EXPECT_EQ(1, cmzn_field_get_number_of_source_fields(sumComponentField));
	EXPECT_EQ(1, cmzn_field_get_number_of_components(sumComponentField));

	cmzn_field_id derivativeField = cmzn_fieldmodule_find_field_by_name(fieldmodule,
		"derivative");
	EXPECT_NE(static_cast<cmzn_field *>(0), derivativeField);
	EXPECT_TRUE(cmzn_field_is_managed(derivativeField));
	EXPECT_EQ(1, cmzn_field_get_number_of_source_fields(derivativeField));

	cmzn_field_id coordinateTransformationField =
		cmzn_fieldmodule_find_field_by_name(fieldmodule, "coordinateTransformation");
	EXPECT_NE(static_cast<cmzn_field *>(0), coordinateTransformationField);
	EXPECT_TRUE(cmzn_field_is_managed(coordinateTransformationField));
	EXPECT_EQ(1, cmzn_field_get_number_of_source_fields(coordinateTransformationField));
	EXPECT_EQ(CMZN_FIELD_COORDINATE_SYSTEM_TYPE_PROLATE_SPHEROIDAL,
		cmzn_field_get_coordinate_system_type(coordinateTransformationField));
	EXPECT_EQ(0.7, cmzn_field_get_coordinate_system_focus(coordinateTransformationField));

	cmzn_field_id determinantField =
		cmzn_fieldmodule_find_field_by_name(fieldmodule, "determinant");
	EXPECT_NE(static_cast<cmzn_field *>(0), determinantField);
	EXPECT_EQ(CMZN_OK, cmzn_field_evaluate_real(determinantField, fieldCache, 1, &values3[0]));
	EXPECT_EQ(-3.0, values3[0]);

	cmzn_field_id matrixMultiplyField =
		cmzn_fieldmodule_find_field_by_name(fieldmodule, "matrixMultiply");
	EXPECT_NE(static_cast<cmzn_field *>(0), matrixMultiplyField);
	EXPECT_EQ(9, cmzn_field_get_number_of_components(matrixMultiplyField));

	cmzn_field_id eigenvaluesField =
		cmzn_fieldmodule_find_field_by_name(fieldmodule, "eigenvalues");
	EXPECT_NE(static_cast<cmzn_field *>(0), eigenvaluesField);
	EXPECT_EQ(3, cmzn_field_get_number_of_components(eigenvaluesField));

	cmzn_field_id eigenvectorsField =
		cmzn_fieldmodule_find_field_by_name(fieldmodule, "eigenvectors");
	EXPECT_NE(static_cast<cmzn_field *>(0), eigenvectorsField);
	EXPECT_EQ(9, cmzn_field_get_number_of_components(eigenvectorsField));

	cmzn_field_id crossProductField = cmzn_fieldmodule_find_field_by_name(
		fieldmodule, "crossProduct");
	EXPECT_NE(static_cast<cmzn_field *>(0), crossProductField);

	cmzn_field_id timeValueField = cmzn_fieldmodule_find_field_by_name(
		fieldmodule, "timeValue");
	EXPECT_NE(static_cast<cmzn_field *>(0), timeValueField);

	cmzn_field_id timeLookupField = cmzn_fieldmodule_find_field_by_name(
		fieldmodule, "timeLookup");
	EXPECT_NE(static_cast<cmzn_field *>(0), timeLookupField);

	cmzn_field_id transposeField = cmzn_fieldmodule_find_field_by_name(
		fieldmodule, "transpose");
	EXPECT_NE(static_cast<cmzn_field *>(0), transposeField);

	cmzn_field_id nodeValueField = cmzn_fieldmodule_find_field_by_name(
		fieldmodule, "nodeValue");
	EXPECT_NE(static_cast<cmzn_field *>(0), nodeValueField);

	cmzn_field_id isOnFaceField = cmzn_fieldmodule_find_field_by_name(
		fieldmodule, "isOnFace");
	EXPECT_NE(static_cast<cmzn_field *>(0), isOnFaceField);

	cmzn_field_id storedMeshLocationField = cmzn_fieldmodule_find_field_by_name(
		fieldmodule, "storedMeshLocation");
	EXPECT_NE(static_cast<cmzn_field *>(0), storedMeshLocationField);

	cmzn_field_id findMeshLocationField = cmzn_fieldmodule_find_field_by_name(
		fieldmodule, "findMeshLocation");
	EXPECT_NE(static_cast<cmzn_field *>(0), findMeshLocationField);

	cmzn_deallocate(returned_string);
	cmzn_fieldcache_destroy(&fieldCache);
	cmzn_field_destroy(&findMeshLocationField);
	cmzn_field_destroy(&storedMeshLocationField);
	cmzn_field_destroy(&isOnFaceField);
	cmzn_field_destroy(&nodeValueField);
	cmzn_field_destroy(&transposeField);
	cmzn_field_destroy(&timeLookupField);
	cmzn_field_destroy(&timeValueField);
	cmzn_field_destroy(&crossProductField);
	cmzn_field_destroy(&eigenvectorsField);
	cmzn_field_destroy(&eigenvaluesField);
	cmzn_field_destroy(&determinantField);
	cmzn_field_destroy(&coordinateTransformationField);
	cmzn_field_destroy(&derivativeField);
	cmzn_field_destroy(&sumComponentField);
	cmzn_field_destroy(&edgeDiscontinuityField);
	cmzn_field_destroy(&andField);
	cmzn_field_destroy(&stringConstantField);
	cmzn_field_destroy(&constantField);
	cmzn_field_destroy(&concatenateField);
	cmzn_field_destroy(&addField);
	cmzn_field_destroy(&firstComponentField);
	cmzn_field_destroy(&componentField);
	cmzn_field_destroy(&temperatureField);
	cmzn_field_destroy(&coordinatesField);
}

void TestDescriptionOutput(cmzn_region_id region, char *description)
{
	cmzn_streaminformation_id si = cmzn_region_create_streaminformation_region(
		region);
	EXPECT_NE(static_cast<cmzn_streaminformation *>(0), si);

	cmzn_streamresource_id sr = cmzn_streaminformation_create_streamresource_file(
		si, TestResources::getLocation(TestResources::FIELDMODULE_REGION_INPUT_RESOURCE));
	EXPECT_NE(static_cast<cmzn_streamresource *>(0), sr);

	cmzn_streaminformation_region_id si_region = cmzn_streaminformation_cast_region(
		si);
	EXPECT_NE(static_cast<cmzn_streaminformation_region *>(0), si_region);

	int result = cmzn_region_read(region, si_region);
	EXPECT_EQ(CMZN_OK, result);

	cmzn_streamresource_destroy(&sr);
	cmzn_streaminformation_region_destroy(&si_region);
	cmzn_streaminformation_destroy(&si);

	cmzn_region_id tetrahedron_region = cmzn_region_find_child_by_name(
		region, "tetrahedron");
	EXPECT_NE(static_cast<cmzn_region *>(0), tetrahedron_region);

	cmzn_fieldmodule_id fieldmodule = cmzn_region_get_fieldmodule(tetrahedron_region);
	EXPECT_NE(static_cast<cmzn_fieldmodule *>(0), fieldmodule);

	EXPECT_EQ(CMZN_OK, cmzn_fieldmodule_read_description(fieldmodule, description));

	testFields(fieldmodule);

	cmzn_fieldmodule_destroy(&fieldmodule);

	cmzn_region_destroy(&tetrahedron_region);
}

TEST(fieldmodule_description, write)
{
	cmzn_context_id context = cmzn_context_create("test");
	cmzn_region_id root_region = cmzn_context_get_default_region(context);

	cmzn_streaminformation_id si = cmzn_region_create_streaminformation_region(
		root_region);
	EXPECT_NE(static_cast<cmzn_streaminformation *>(0), si);

	cmzn_streamresource_id sr = cmzn_streaminformation_create_streamresource_file(
		si, TestResources::getLocation(TestResources::FIELDMODULE_REGION_INPUT_RESOURCE));
	EXPECT_NE(static_cast<cmzn_streamresource *>(0), sr);

	cmzn_streaminformation_region_id si_region = cmzn_streaminformation_cast_region(
		si);
	EXPECT_NE(static_cast<cmzn_streaminformation_region *>(0), si_region);

	int result = cmzn_region_read(root_region, si_region);
	EXPECT_EQ(CMZN_OK, result);

	cmzn_streamresource_destroy(&sr);
	cmzn_streaminformation_region_destroy(&si_region);
	cmzn_streaminformation_destroy(&si);

	cmzn_region_id tetrahedron_region = cmzn_region_find_child_by_name(
		root_region, "tetrahedron");
	EXPECT_NE(static_cast<cmzn_region *>(0), tetrahedron_region);

	cmzn_fieldmodule_id fieldmodule = cmzn_region_get_fieldmodule(tetrahedron_region);
	EXPECT_NE(static_cast<cmzn_fieldmodule *>(0), fieldmodule);

	cmzn_field_id coordinatesField = cmzn_fieldmodule_find_field_by_name(fieldmodule, "coordinates");
	EXPECT_NE(static_cast<cmzn_field *>(0), coordinatesField);

	cmzn_field_id temperatureField = cmzn_fieldmodule_find_field_by_name(fieldmodule, "temperature");
	EXPECT_NE(static_cast<cmzn_field *>(0), temperatureField);

	int indexes[2] = {2,1};

	cmzn_field_id componentField = cmzn_fieldmodule_create_field_component_multiple(
		fieldmodule, coordinatesField, 2, indexes);
	cmzn_field_set_managed(componentField, true);
	cmzn_field_set_name(componentField, "component");
	EXPECT_NE(static_cast<cmzn_field *>(0), componentField);

	cmzn_field_id firstComponentField = cmzn_fieldmodule_create_field_component(
		fieldmodule, coordinatesField, 1);
	EXPECT_NE(static_cast<cmzn_field *>(0), firstComponentField);
	cmzn_field_set_managed(firstComponentField, false);
	cmzn_field_set_name(firstComponentField, "firstComponent");

	cmzn_field_id addField = cmzn_fieldmodule_create_field_add(
		fieldmodule, firstComponentField, temperatureField);
	EXPECT_NE(static_cast<cmzn_field *>(0), addField);
	cmzn_field_set_managed(addField, true);
	cmzn_field_set_name(addField, "add");

	cmzn_field_id sourceFields[2] = {coordinatesField, temperatureField};

	cmzn_field_id concatenateField = cmzn_fieldmodule_create_field_concatenate(
		fieldmodule, 2, sourceFields);
	cmzn_field_set_managed(concatenateField, true);
	cmzn_field_set_name(concatenateField, "concatenate");
	EXPECT_NE(static_cast<cmzn_field *>(0), concatenateField);

	double values3[3] = {1.0, 4.0, 2.0};
	cmzn_field_id constantField = cmzn_fieldmodule_create_field_constant(
		fieldmodule, 3, values3);
	cmzn_field_set_managed(constantField, true);
	cmzn_field_set_name(constantField, "constant");
	EXPECT_NE(static_cast<cmzn_field *>(0), constantField);

	cmzn_field_id stringConstantField = cmzn_fieldmodule_create_field_string_constant(
		fieldmodule, "string_constant");
	cmzn_field_set_managed(stringConstantField, true);
	cmzn_field_set_name(stringConstantField, "stringConstant");
	EXPECT_NE(static_cast<cmzn_field *>(0), stringConstantField);

	cmzn_field_id andField =  cmzn_fieldmodule_create_field_and(fieldmodule,
		constantField, temperatureField);
	EXPECT_NE(static_cast<cmzn_field *>(0), andField);
	cmzn_field_set_managed(andField, true);
	cmzn_field_set_name(andField, "and");

	cmzn_field_id edgeDiscontinuityField = cmzn_fieldmodule_create_field_edge_discontinuity(
		fieldmodule, coordinatesField);
	EXPECT_NE(static_cast<cmzn_field *>(0), edgeDiscontinuityField);
	cmzn_field_set_managed(edgeDiscontinuityField, true);
	cmzn_field_set_name(edgeDiscontinuityField, "edgeDiscontinuity");

	cmzn_field_id sumComponentField = cmzn_fieldmodule_create_field_sum_components(
		fieldmodule, coordinatesField);
	EXPECT_NE(static_cast<cmzn_field *>(0), sumComponentField);
	cmzn_field_set_managed(sumComponentField, true);
	cmzn_field_set_name(sumComponentField, "sumComponent");

#if defined (ZINC_USE_ITK)
	cmzn_field_id derivativeField = cmzn_fieldmodule_create_field_derivative(fieldmodule,
		coordinatesField, 1);
	EXPECT_NE(static_cast<cmzn_field *>(0), derivativeField);
	cmzn_field_set_managed(derivativeField, true);
	cmzn_field_set_name(derivativeField, "derivative");
#endif
	cmzn_field_id coordinateTransformationField =
		cmzn_fieldmodule_create_field_coordinate_transformation(
			fieldmodule, coordinatesField);
	EXPECT_NE(static_cast<cmzn_field *>(0), coordinateTransformationField);
	cmzn_field_set_managed(coordinateTransformationField, true);
	cmzn_field_set_name(coordinateTransformationField, "coordinateTransformation");
	cmzn_field_set_coordinate_system_type(coordinateTransformationField,
		CMZN_FIELD_COORDINATE_SYSTEM_TYPE_PROLATE_SPHEROIDAL);
	cmzn_field_set_coordinate_system_focus(coordinateTransformationField, 0.7);

	double values9[9] = {2.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0};
	cmzn_field_id matrixField = cmzn_fieldmodule_create_field_constant(
		fieldmodule, 9, values9);
	EXPECT_NE(static_cast<cmzn_field *>(0), matrixField);
	cmzn_field_set_name(matrixField, "matrix");

	cmzn_field_id determinantField =
		cmzn_fieldmodule_create_field_determinant(fieldmodule, matrixField);
	EXPECT_NE(static_cast<cmzn_field *>(0), determinantField);
	cmzn_field_set_managed(determinantField, true);
	cmzn_field_set_name(determinantField, "determinant");

	cmzn_field_id eigenvaluesField = cmzn_fieldmodule_create_field_eigenvalues(fieldmodule, matrixField);
	EXPECT_NE(static_cast<cmzn_field *>(0), eigenvaluesField);
	cmzn_field_set_managed(eigenvaluesField, true);
	cmzn_field_set_name(eigenvaluesField, "eigenvalues");

	cmzn_field_id eigenvectorsField = cmzn_fieldmodule_create_field_eigenvectors(
		fieldmodule, eigenvaluesField);
	EXPECT_NE(static_cast<cmzn_field *>(0), eigenvectorsField);
	cmzn_field_set_managed(eigenvectorsField, true);
	cmzn_field_set_name(eigenvectorsField, "eigenvectors");

	double values9_2[9] = {9.0, 8.0, 7.0, 6.0, 5.0, 4.0, 3.0, 2.0, 2.0};
	cmzn_field_id matrixField2 = cmzn_fieldmodule_create_field_constant(
		fieldmodule, 9, values9_2);
	EXPECT_NE(static_cast<cmzn_field *>(0), matrixField2);
	cmzn_field_set_name(matrixField2, "matrix2");

	cmzn_field_id matrixMultiplyField = cmzn_fieldmodule_create_field_matrix_multiply(
		fieldmodule, 3, matrixField, matrixField2);
	EXPECT_NE(static_cast<cmzn_field *>(0), matrixMultiplyField);
	cmzn_field_set_managed(matrixMultiplyField, true);
	cmzn_field_set_name(matrixMultiplyField, "matrixMultiply");

	double values3_2[3] = {3.0, 2.0, 1.0};
	cmzn_field_id secondConstantField = cmzn_fieldmodule_create_field_constant(
		fieldmodule, 3, values3_2);
	EXPECT_NE(static_cast<cmzn_field *>(0), secondConstantField);
	cmzn_field_set_name(secondConstantField, "secondConstant");

	cmzn_field_id fieldArray[2] = {constantField, secondConstantField};

	cmzn_field_id crossProductField = cmzn_fieldmodule_create_field_cross_product(
		fieldmodule, 2, fieldArray);
	EXPECT_NE(static_cast<cmzn_field *>(0), crossProductField);
	cmzn_field_set_managed(crossProductField, true);
	cmzn_field_set_name(crossProductField, "crossProduct");

	cmzn_timekeepermodule_id tm = cmzn_context_get_timekeepermodule(context);
	cmzn_timekeeper_id timekeeper = cmzn_timekeepermodule_get_default_timekeeper(tm);
	cmzn_field_id timeValueField = cmzn_fieldmodule_create_field_time_value(
		fieldmodule, timekeeper);
	EXPECT_NE(static_cast<cmzn_field *>(0), timeValueField);
	cmzn_field_set_managed(timeValueField, true);
	cmzn_field_set_name(timeValueField, "timeValue");
	cmzn_timekeepermodule_destroy(&tm);
	cmzn_timekeeper_destroy(&timekeeper);

	cmzn_field_id timeLookupField = cmzn_fieldmodule_create_field_time_lookup(
		fieldmodule, coordinatesField, timeValueField);
	EXPECT_NE(static_cast<cmzn_field *>(0), timeLookupField);
	cmzn_field_set_managed(timeLookupField, true);
	cmzn_field_set_name(timeLookupField, "timeLookup");

	cmzn_field_id transposeField = cmzn_fieldmodule_create_field_transpose(
		fieldmodule, 3, matrixField);
	EXPECT_NE(static_cast<cmzn_field *>(0), transposeField);
	cmzn_field_set_managed(transposeField, true);
	cmzn_field_set_name(transposeField, "transpose");

	cmzn_field_id nodeValueField = cmzn_fieldmodule_create_field_node_value(
		fieldmodule, coordinatesField, CMZN_NODE_VALUE_LABEL_VALUE, 1);
	EXPECT_NE(static_cast<cmzn_field *>(0), nodeValueField);
	cmzn_field_set_managed(nodeValueField, true);
	cmzn_field_set_name(nodeValueField, "nodeValue");

	cmzn_field_id isOnFaceField = cmzn_fieldmodule_create_field_is_on_face(
		fieldmodule, CMZN_ELEMENT_FACE_TYPE_XI1_0);
	EXPECT_NE(static_cast<cmzn_field *>(0), isOnFaceField);
	cmzn_field_set_managed(isOnFaceField, true);
	cmzn_field_set_name(isOnFaceField, "isOnFace");

	cmzn_mesh_id mesh = cmzn_fieldmodule_find_mesh_by_name(fieldmodule, "mesh2d");
	cmzn_field_id storedMeshLocationField = cmzn_fieldmodule_create_field_stored_mesh_location(
		fieldmodule, mesh);
	EXPECT_NE(static_cast<cmzn_field *>(0), storedMeshLocationField);
	cmzn_field_set_managed(storedMeshLocationField, true);
	cmzn_field_set_name(storedMeshLocationField, "storedMeshLocation");

	cmzn_field_id findMeshLocationField = cmzn_fieldmodule_create_field_find_mesh_location(
		fieldmodule, coordinatesField, coordinatesField, mesh);
	EXPECT_NE(static_cast<cmzn_field *>(0), findMeshLocationField);
	cmzn_field_set_managed(findMeshLocationField, true);
	cmzn_field_set_name(findMeshLocationField, "findMeshLocation");
	cmzn_mesh_destroy(&mesh);

	char *description_string = cmzn_fieldmodule_write_description(fieldmodule);
	EXPECT_NE(static_cast<char *>(0), description_string);
	//printf("%s", description_string);

	cmzn_region_id region = cmzn_region_create_child(root_region, "test");
	TestDescriptionOutput(region, description_string);
	cmzn_region_destroy(&region);

	cmzn_deallocate(description_string);

	cmzn_field_destroy(&findMeshLocationField);
	cmzn_field_destroy(&storedMeshLocationField);
	cmzn_field_destroy(&isOnFaceField);
	cmzn_field_destroy(&nodeValueField);
	cmzn_field_destroy(&transposeField);
	cmzn_field_destroy(&timeLookupField);
	cmzn_field_destroy(&timeValueField);
	cmzn_field_destroy(&crossProductField);
	cmzn_field_destroy(&secondConstantField);
	cmzn_field_destroy(&matrixMultiplyField);
	cmzn_field_destroy(&matrixField2);
	cmzn_field_destroy(&eigenvectorsField);
	cmzn_field_destroy(&eigenvaluesField);
	cmzn_field_destroy(&determinantField);
	cmzn_field_destroy(&matrixField);
	cmzn_field_destroy(&coordinateTransformationField);
#if defined (ZINC_USE_ITK)
	cmzn_field_destroy(&derivativeField);
#endif
	cmzn_field_destroy(&sumComponentField);
	cmzn_field_destroy(&edgeDiscontinuityField);
	cmzn_field_destroy(&andField);
	cmzn_field_destroy(&stringConstantField);
	cmzn_field_destroy(&constantField);
	cmzn_field_destroy(&concatenateField);
	cmzn_field_destroy(&addField);
	cmzn_field_destroy(&firstComponentField);
	cmzn_field_destroy(&componentField);
	cmzn_field_destroy(&temperatureField);
	cmzn_field_destroy(&coordinatesField);
	cmzn_fieldmodule_destroy(&fieldmodule);

	cmzn_region_destroy(&tetrahedron_region);
	cmzn_region_destroy(&root_region);
	cmzn_context_destroy(&context);
}

TEST(fieldmodule_description, read)
{
	cmzn_context_id context = cmzn_context_create("test");
	cmzn_region_id root_region = cmzn_context_get_default_region(context);

	void *buffer = 0;
	long length;
	FILE * f = fopen (TestResources::getLocation(TestResources::FIELDMODULE_DESCRIPTION_JSON_RESOURCE), "rb");
	if (f)
	{
		fseek (f, 0, SEEK_END);
		length = ftell (f);
		fseek (f, 0, SEEK_SET);
		buffer = malloc (length);
		if (buffer)
		{
			fread (buffer, 1, length, f);
		}
		fclose (f);
	}

	TestDescriptionOutput(root_region, (char *)buffer);

	free(buffer);

	cmzn_region_destroy(&root_region);
	cmzn_context_destroy(&context);
}
