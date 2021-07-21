/*
 * OpenCMISS-Zinc Library Unit Tests
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <gtest/gtest.h>
#include <cmath>
#include <opencmiss/zinc/context.h>
#include <opencmiss/zinc/core.h>
#include <opencmiss/zinc/differentialoperator.h>
#include <opencmiss/zinc/element.h>
#include <opencmiss/zinc/field.h>
#include <opencmiss/zinc/fieldarithmeticoperators.h>
#include <opencmiss/zinc/fieldcache.h>
#include <opencmiss/zinc/fieldconstant.h>
#include <opencmiss/zinc/fieldderivatives.h>
#include <opencmiss/zinc/fieldmodule.h>
#include <opencmiss/zinc/fieldvectoroperators.h>
#include <opencmiss/zinc/node.h>
#include <opencmiss/zinc/region.h>
#include <opencmiss/zinc/status.h>

#include "zinctestsetupcpp.hpp"
#include <opencmiss/zinc/differentialoperator.hpp>
#include <opencmiss/zinc/element.hpp>
#include <opencmiss/zinc/field.hpp>
#include <opencmiss/zinc/fieldarithmeticoperators.hpp>
#include <opencmiss/zinc/fieldassignment.hpp>
#include <opencmiss/zinc/fieldcache.hpp>
#include <opencmiss/zinc/fieldcomposite.hpp>
#include <opencmiss/zinc/fieldconstant.hpp>
#include <opencmiss/zinc/fieldderivatives.hpp>
#include <opencmiss/zinc/fieldfiniteelement.hpp>
#include <opencmiss/zinc/fieldmatrixoperators.hpp>
#include <opencmiss/zinc/fieldtrigonometry.hpp>

#include "test_resources.h"

#include "zinctestsetup.hpp"

TEST(cmzn_fieldmodule_create_field_derivative, invalid_args)
{
	cmzn_context_id context = cmzn_context_create("test");
	cmzn_region_id root_region = cmzn_context_get_default_region(context);
	cmzn_fieldmodule_id fm = cmzn_region_get_fieldmodule(root_region);

	EXPECT_NE(static_cast<cmzn_fieldmodule *>(0), fm);

	cmzn_field_id f0 = cmzn_fieldmodule_create_field_derivative(0, 0, 0);
	EXPECT_EQ(0, f0);

	cmzn_field_id f1 = cmzn_fieldmodule_create_field_derivative(fm, 0, 2);
	EXPECT_EQ(0, f1);

	double values[] = {6.0, 1.0, 2.5};
	cmzn_field_id f2 = cmzn_fieldmodule_create_field_constant(fm, 3, values);

	cmzn_field_id f3 = cmzn_fieldmodule_create_field_derivative(fm, f2, -1);
	EXPECT_EQ(0, f3);

	cmzn_field_destroy(&f0);
	cmzn_field_destroy(&f1);
	cmzn_field_destroy(&f2);
	cmzn_field_destroy(&f3);
	cmzn_fieldmodule_destroy(&fm);
	cmzn_region_destroy(&root_region);
	cmzn_context_destroy(&context);
}

TEST(cmzn_fieldmodule_create_field_derivative, valid_args)
{
	cmzn_context_id context = cmzn_context_create("test");
	cmzn_region_id root_region = cmzn_context_get_default_region(context);
	cmzn_fieldmodule_id fm = cmzn_region_get_fieldmodule(root_region);

	int result = cmzn_region_read_file(root_region, TestResources::getLocation(TestResources::FIELDMODULE_CUBE_RESOURCE));
	EXPECT_EQ(CMZN_OK, result);

	cmzn_field_id f1 = cmzn_fieldmodule_find_field_by_name(fm, "coordinates");
	EXPECT_NE(static_cast<cmzn_field *>(0), f1);

	cmzn_field_id ft = cmzn_fieldmodule_find_field_by_name(fm, "xi");
	EXPECT_NE(static_cast<cmzn_field *>(0), ft);

	cmzn_field_id f2 = cmzn_fieldmodule_create_field_derivative(fm, f1, 1);
	EXPECT_NE(static_cast<cmzn_field *>(0), f2);

	cmzn_fieldcache_id fc = cmzn_fieldmodule_create_fieldcache(fm);

	cmzn_mesh_id mesh = cmzn_fieldmodule_find_mesh_by_dimension(fm, 3);
	EXPECT_NE(static_cast<cmzn_mesh *>(0), mesh);

	cmzn_element_id el = cmzn_mesh_find_element_by_identifier(mesh, 1);
	EXPECT_NE(static_cast<cmzn_element *>(0), el);

	double xi[] = {0.6, 0.2, 0.45};
	result = cmzn_fieldcache_set_mesh_location(fc, el, 3, xi);
	EXPECT_EQ(CMZN_OK, result);

	double outvalues[3];
	EXPECT_EQ(CMZN_OK, result = cmzn_field_evaluate_real(f1, fc, 3, outvalues));
	EXPECT_DOUBLE_EQ(xi[0], outvalues[0]);
	EXPECT_DOUBLE_EQ(xi[1], outvalues[1]);
	EXPECT_DOUBLE_EQ(xi[2], outvalues[2]);

	EXPECT_EQ(CMZN_OK, result = cmzn_field_evaluate_real(f2, fc, 3, outvalues));
	EXPECT_DOUBLE_EQ(1.0, outvalues[0]);
	EXPECT_DOUBLE_EQ(0.0, outvalues[1]);
	EXPECT_DOUBLE_EQ(0.0, outvalues[2]);

	cmzn_element_destroy(&el);
	cmzn_mesh_destroy(&mesh);
	cmzn_field_destroy(&f1);
	cmzn_field_destroy(&f2);
	cmzn_field_destroy(&ft);
	cmzn_fieldcache_destroy(&fc);
	cmzn_fieldmodule_destroy(&fm);
	cmzn_region_destroy(&root_region);
	cmzn_context_destroy(&context);
}

TEST(cmzn_fieldmodule_create_field_curl, invalid_args)
{
	cmzn_context_id context = cmzn_context_create("test");
	cmzn_region_id root_region = cmzn_context_get_default_region(context);
	cmzn_fieldmodule_id fm = cmzn_region_get_fieldmodule(root_region);

	EXPECT_NE(static_cast<cmzn_fieldmodule *>(0), fm);

	cmzn_field_id f0 = cmzn_fieldmodule_create_field_curl(0, 0, 0);
	EXPECT_EQ(0, f0);

	cmzn_field_id f1 = cmzn_fieldmodule_create_field_curl(fm, 0, 0);
	EXPECT_EQ(0, f1);

	double values[] = {6.0, 1.0, 2.5};
	cmzn_field_id f2 = cmzn_fieldmodule_create_field_constant(fm, 3, values);

	cmzn_field_id f3 = cmzn_fieldmodule_create_field_curl(fm, f2, 0);
	EXPECT_EQ(0, f3);

	cmzn_field_destroy(&f0);
	cmzn_field_destroy(&f1);
	cmzn_field_destroy(&f2);
	cmzn_field_destroy(&f3);
	cmzn_fieldmodule_destroy(&fm);
	cmzn_region_destroy(&root_region);
	cmzn_context_destroy(&context);
}

TEST(cmzn_fieldmodule_create_field_curl, valid_args)
{
	cmzn_context_id context = cmzn_context_create("test");
	cmzn_region_id root_region = cmzn_context_get_default_region(context);
	cmzn_fieldmodule_id fm = cmzn_region_get_fieldmodule(root_region);

	cmzn_region_read_file(root_region, TestResources::getLocation(TestResources::FIELDMODULE_CUBE_RESOURCE));

	cmzn_field_id f1 = cmzn_fieldmodule_find_field_by_name(fm, "coordinates");
	EXPECT_NE(static_cast<cmzn_field *>(0), f1);

	double values[] = {2.0, 3.0, 5.0};
	cmzn_field_id c1 = cmzn_fieldmodule_create_field_constant(fm, 3, values);

	cmzn_field_id f2 = cmzn_fieldmodule_create_field_curl(fm, c1, f1);
	EXPECT_NE(static_cast<cmzn_field *>(0), f2);

	cmzn_fieldcache_id fc = cmzn_fieldmodule_create_fieldcache(fm);

	cmzn_mesh_id mesh = cmzn_fieldmodule_find_mesh_by_dimension(fm, 3);
	EXPECT_NE(static_cast<cmzn_mesh *>(0), mesh);

	cmzn_element_id el = cmzn_mesh_find_element_by_identifier(mesh, 1);
	EXPECT_NE(static_cast<cmzn_element *>(0), el);

	double chart_coordinates[] = {0.6, 0.2, 0.45};
	int result = cmzn_fieldcache_set_mesh_location(fc, el, 3, chart_coordinates);
	EXPECT_EQ(CMZN_OK, result);

	double outvalues[3];
	result = cmzn_field_evaluate_real(f2, fc, 3, outvalues);
	EXPECT_EQ(CMZN_OK, result);
	EXPECT_EQ(0.0, outvalues[0]);
	EXPECT_EQ(0.0, outvalues[1]);
	EXPECT_EQ(0.0, outvalues[2]);

	cmzn_element_destroy(&el);
	cmzn_mesh_destroy(&mesh);
	cmzn_field_destroy(&f1);
	cmzn_field_destroy(&c1);
	cmzn_field_destroy(&f2);
	cmzn_fieldcache_destroy(&fc);
	cmzn_fieldmodule_destroy(&fm);
	cmzn_region_destroy(&root_region);
	cmzn_context_destroy(&context);
}

TEST(cmzn_fieldmodule_create_field_divergence, invalid_args)
{
	cmzn_context_id context = cmzn_context_create("test");
	cmzn_region_id root_region = cmzn_context_get_default_region(context);
	cmzn_fieldmodule_id fm = cmzn_region_get_fieldmodule(root_region);

	EXPECT_NE(static_cast<cmzn_fieldmodule *>(0), fm);

	cmzn_field_id f0 = cmzn_fieldmodule_create_field_divergence(0, 0, 0);
	EXPECT_EQ(0, f0);

	cmzn_field_id f1 = cmzn_fieldmodule_create_field_divergence(fm, 0, 0);
	EXPECT_EQ(0, f1);

	double values[] = {6.0, 1.0, 2.5};
	cmzn_field_id f2 = cmzn_fieldmodule_create_field_constant(fm, 3, values);

	cmzn_field_id f3 = cmzn_fieldmodule_create_field_divergence(fm, f2, 0);
	EXPECT_EQ(0, f3);

	cmzn_field_destroy(&f0);
	cmzn_field_destroy(&f1);
	cmzn_field_destroy(&f2);
	cmzn_field_destroy(&f3);
	cmzn_fieldmodule_destroy(&fm);
	cmzn_region_destroy(&root_region);
	cmzn_context_destroy(&context);
}

TEST(cmzn_fieldmodule_create_field_divergence, valid_args)
{
	cmzn_context_id context = cmzn_context_create("test");
	cmzn_region_id root_region = cmzn_context_get_default_region(context);
	cmzn_fieldmodule_id fm = cmzn_region_get_fieldmodule(root_region);

	cmzn_region_read_file(root_region, TestResources::getLocation(TestResources::FIELDMODULE_CUBE_RESOURCE));

	cmzn_field_id coordinates = cmzn_fieldmodule_find_field_by_name(fm, "coordinates");
	EXPECT_NE(static_cast<cmzn_field *>(0), coordinates);

	double values[] = {2.0, 3.0, 5.0};
	cmzn_field_id c1 = cmzn_fieldmodule_create_field_constant(fm, 3, values);

	cmzn_field_id div_const = cmzn_fieldmodule_create_field_divergence(fm, c1, coordinates);
	EXPECT_NE(static_cast<cmzn_field *>(0), div_const);

	cmzn_fieldcache_id fc = cmzn_fieldmodule_create_fieldcache(fm);

	cmzn_mesh_id mesh = cmzn_fieldmodule_find_mesh_by_dimension(fm, 3);
	EXPECT_NE(static_cast<cmzn_mesh *>(0), mesh);

	cmzn_element_id el = cmzn_mesh_find_element_by_identifier(mesh, 1);
	EXPECT_NE(static_cast<cmzn_element *>(0), el);

	double chart_coordinates[] = {0.6, 0.2, 0.45};
	int result = cmzn_fieldcache_set_mesh_location(fc, el, 3, chart_coordinates);
	EXPECT_EQ(CMZN_OK, result);

	double outvalues[1];
	result = cmzn_field_evaluate_real(div_const, fc, 1, outvalues);
	EXPECT_EQ(CMZN_OK, result);
	EXPECT_EQ(0.0, outvalues[0]);

	cmzn_element_destroy(&el);
	cmzn_mesh_destroy(&mesh);
	cmzn_field_destroy(&coordinates);
	cmzn_field_destroy(&c1);
	cmzn_field_destroy(&div_const);
	cmzn_fieldcache_destroy(&fc);
	cmzn_fieldmodule_destroy(&fm);
	cmzn_region_destroy(&root_region);
	cmzn_context_destroy(&context);
}

TEST(cmzn_fieldmodule_create_field_divergence, grad_mag)
{
	cmzn_context_id context = cmzn_context_create("test");
	cmzn_region_id root_region = cmzn_context_get_default_region(context);
	cmzn_fieldmodule_id fm = cmzn_region_get_fieldmodule(root_region);

	cmzn_region_read_file(root_region, TestResources::getLocation(TestResources::FIELDMODULE_CUBE_RESOURCE));

	cmzn_field_id coordinates = cmzn_fieldmodule_find_field_by_name(fm, "coordinates");
	EXPECT_NE(static_cast<cmzn_field *>(0), coordinates);

	const double scaling[] = { 2.0, 1.5, 0.75 };
	cmzn_field_id constant_scaling = cmzn_fieldmodule_create_field_constant(fm, 3, scaling);
	cmzn_field_id scaled_coordinates = cmzn_fieldmodule_create_field_multiply(fm, coordinates, constant_scaling);
	cmzn_field_id mag = cmzn_fieldmodule_create_field_magnitude(fm, scaled_coordinates);
	cmzn_field_id grad_mag = cmzn_fieldmodule_create_field_gradient(fm, mag, scaled_coordinates);
	cmzn_field_id div_grad_mag = cmzn_fieldmodule_create_field_divergence(fm, grad_mag, scaled_coordinates);
	EXPECT_NE(static_cast<cmzn_field *>(0), div_grad_mag);

	cmzn_fieldcache_id fc = cmzn_fieldmodule_create_fieldcache(fm);

	cmzn_mesh_id mesh = cmzn_fieldmodule_find_mesh_by_dimension(fm, 3);
	EXPECT_NE(static_cast<cmzn_mesh *>(0), mesh);

	cmzn_element_id el = cmzn_mesh_find_element_by_identifier(mesh, 1);
	EXPECT_NE(static_cast<cmzn_element *>(0), el);

	const double xi[3][3] = {
		{ 0.10, 0.90, 0.25 },
		{ 0.50, 0.50, 0.00 },
		{ 0.60, 0.20, 0.45 } };
	double x[3];
	const double fineTol = 1.0E-10;
	const double coarseTol = 1.0E-7;
	int result;
	for (int i = 0; i < 3; ++i)
	{
		for (int c = 0; c < 3; ++c)
			x[c] = scaling[c]*xi[i][c];
		result = cmzn_fieldcache_set_mesh_location(fc, el, 3, xi[i]);
		EXPECT_EQ(CMZN_OK, result);
		double outvalues[3];
		result = cmzn_field_evaluate_real(grad_mag, fc, 3, outvalues);
		// exact math answers:
		const double mag1 = sqrt(x[0]*x[0] + x[1]*x[1] + x[2]*x[2]);
		const double gradMagAnswers[] = { x[0]/mag1, x[1]/mag1, x[2]/mag1 };
		double divGradMagAnswer = 0.0;
		for (int c = 0; c < 3; ++c)
			divGradMagAnswer += (1.0/mag1) - x[c]*x[c]/(mag1*mag1*mag1);
		for (int c = 0; c < 3; ++c)
			EXPECT_NEAR(outvalues[c], gradMagAnswers[c], fineTol);
		// div_grad_mag currently computed with finite differences, hence coarser tolerance:
		double outValue;
		result = cmzn_field_evaluate_real(div_grad_mag, fc, 1, &outValue);
		EXPECT_EQ(CMZN_OK, result);
		EXPECT_NEAR(divGradMagAnswer, outValue, coarseTol);
	}

	cmzn_element_destroy(&el);
	cmzn_mesh_destroy(&mesh);
	cmzn_field_destroy(&coordinates);
	cmzn_field_destroy(&constant_scaling);
	cmzn_field_destroy(&scaled_coordinates);
	cmzn_field_destroy(&mag);
	cmzn_field_destroy(&grad_mag);
	cmzn_field_destroy(&div_grad_mag);
	cmzn_fieldcache_destroy(&fc);
	cmzn_fieldmodule_destroy(&fm);
	cmzn_region_destroy(&root_region);
	cmzn_context_destroy(&context);
}

TEST(cmzn_fieldmodule_create_field_gradient, invalid_args)
{
	cmzn_context_id context = cmzn_context_create("test");
	cmzn_region_id root_region = cmzn_context_get_default_region(context);
	cmzn_fieldmodule_id fm = cmzn_region_get_fieldmodule(root_region);

	EXPECT_NE(static_cast<cmzn_fieldmodule *>(0), fm);

	cmzn_field_id f0 = cmzn_fieldmodule_create_field_gradient(0, 0, 0);
	EXPECT_EQ(0, f0);

	cmzn_field_id f1 = cmzn_fieldmodule_create_field_gradient(fm, 0, 0);
	EXPECT_EQ(0, f1);

	double values[] = {6.0, 1.0, 2.5};
	cmzn_field_id f2 = cmzn_fieldmodule_create_field_constant(fm, 3, values);

	cmzn_field_id f3 = cmzn_fieldmodule_create_field_gradient(fm, f2, 0);
	EXPECT_EQ(0, f3);

	cmzn_field_id f4 = cmzn_fieldmodule_create_field_gradient(fm, 0, f2);
	EXPECT_EQ(0, f4);

	cmzn_field_destroy(&f0);
	cmzn_field_destroy(&f1);
	cmzn_field_destroy(&f2);
	cmzn_field_destroy(&f3);
	cmzn_field_destroy(&f4);
	cmzn_fieldmodule_destroy(&fm);
	cmzn_region_destroy(&root_region);
	cmzn_context_destroy(&context);
}

TEST(cmzn_fieldmodule_create_field_gradient, valid_args)
{
	cmzn_context_id context = cmzn_context_create("test");
	cmzn_region_id root_region = cmzn_context_get_default_region(context);
	cmzn_fieldmodule_id fm = cmzn_region_get_fieldmodule(root_region);

	cmzn_region_read_file(root_region, TestResources::getLocation(TestResources::FIELDMODULE_CUBE_RESOURCE));

	cmzn_field_id f1 = cmzn_fieldmodule_find_field_by_name(fm, "coordinates");
	EXPECT_NE(static_cast<cmzn_field *>(0), f1);

	cmzn_field_id mag = cmzn_fieldmodule_create_field_magnitude(fm, f1);

	cmzn_field_id f2 = cmzn_fieldmodule_create_field_gradient(fm, mag, f1);
	EXPECT_NE(static_cast<cmzn_field *>(0), f2);

	cmzn_fieldcache_id fc = cmzn_fieldmodule_create_fieldcache(fm);

	cmzn_mesh_id mesh = cmzn_fieldmodule_find_mesh_by_dimension(fm, 3);
	EXPECT_NE(static_cast<cmzn_mesh *>(0), mesh);

	cmzn_element_id el = cmzn_mesh_find_element_by_identifier(mesh, 1);
	EXPECT_NE(static_cast<cmzn_element *>(0), el);

	double chart_coordinates[] = {0.0, 1.0, 0.0};
	int result = cmzn_fieldcache_set_mesh_location(fc, el, 3, chart_coordinates);
	EXPECT_EQ(CMZN_OK, result);

	double outvalues[3];
	result = cmzn_field_evaluate_real(f2, fc, 3, outvalues);
	EXPECT_EQ(CMZN_OK, result);
	EXPECT_EQ(0.0, outvalues[0]);
	EXPECT_EQ(1.0, outvalues[1]);
	EXPECT_EQ(0.0, outvalues[2]);

	cmzn_element_destroy(&el);
	cmzn_mesh_destroy(&mesh);
	cmzn_field_destroy(&f1);
	cmzn_field_destroy(&mag);
	cmzn_field_destroy(&f2);
	cmzn_fieldcache_destroy(&fc);
	cmzn_fieldmodule_destroy(&fm);
	cmzn_region_destroy(&root_region);
	cmzn_context_destroy(&context);
}

// Zinc issue #163 numerical derivative use for gradient of gradient indexed
// values incorrectly if elementDimension != componentsCount
TEST(ZincFieldGradient, gradientOfGradient1)
{
	ZincTestSetupCpp zinc;

	int result = zinc.root_region.readFile(TestResources::getLocation(TestResources::FIELDMODULE_CUBE_TRICUBIC_DEFORMED_RESOURCE));
	EXPECT_EQ(CMZN_OK, result);

	Field coordinates = zinc.fm.findFieldByName("coordinates");
	Field deformed = zinc.fm.findFieldByName("deformed");
	Field displacement = deformed - coordinates;
	EXPECT_TRUE(displacement.isValid());
	Field displacementGradient1 = zinc.fm.createFieldGradient(displacement, coordinates);
	EXPECT_TRUE(displacementGradient1.isValid());
	EXPECT_EQ(9, displacementGradient1.getNumberOfComponents());
	Field displacementGradient2 = zinc.fm.createFieldGradient(displacementGradient1, coordinates);
	EXPECT_TRUE(displacementGradient2.isValid());
	EXPECT_EQ(27, displacementGradient2.getNumberOfComponents());

	const double xi[2][3] =
	{
		{ 0.5, 0.5, 0.5 },
		{ 0.1, 0.2, 0.3 }
	};
	const double displGrad1Answer[2][9] = {
		{-0.0132767,-0.0274628, 0.0112656,-0.0299884,-0.0401232,-0.0101377,-0.0005196, 0.0508512,-0.0255976},
		{-0.1066798,-0.0086179, 0.0600045,-0.0210395, 0.1247084, 0.0950958, 0.2936052, 0.0786568, 0.1425584} };
	const double displGrad2Answer[2][27] = {
		{-0.1494242,-0.0611968,-0.1640609,-0.0611968,-0.0046702,-0.1575785,-0.1640609,-0.1575785,-0.1622634,
		 -0.1411490, 0.2206460, 0.2305718, 0.2206460,-0.4751799, 0.2183105, 0.2305718, 0.2183105,-0.0874906,
		 -0.0244105, 0.9511653, 0.7109450, 0.9511653,-0.1729542, 0.8192559, 0.7109450, 0.8192559, 0.3452878},
		{ 0.6763522,-0.0832618, 1.4575007,-0.0832618, 0.2925900,-0.1729627, 1.4575007,-0.1729627,-1.1215458,
		 -0.2739081, 1.0910543, 0.2909908, 1.0910543,-0.7688550, 0.1861709, 0.2909908, 0.1861709,-0.6266895,
		 -2.0214297, 0.6055436,-0.2633694, 0.6055436,-1.1092826,-0.2157290,-0.2633694,-0.2157290,-1.3034022} };
	double displGrad1[9], displGrad2[27];

	Fieldcache fieldcache = zinc.fm.createFieldcache();
	Mesh mesh = zinc.fm.findMeshByDimension(3);
	Element element = mesh.findElementByIdentifier(1);
	EXPECT_TRUE(element.isValid());
	const double TOL = 1.0E-6;
	for (int i = 0; i < 2; ++i)
	{
		EXPECT_EQ(RESULT_OK, fieldcache.setMeshLocation(element, 3, xi[i]));
		EXPECT_EQ(RESULT_OK, displacementGradient1.evaluateReal(fieldcache, 9, displGrad1));
		for (int c = 0; c < 9; ++c)
			EXPECT_NEAR(displGrad1Answer[i][c], displGrad1[c], TOL);
		EXPECT_EQ(RESULT_OK, displacementGradient2.evaluateReal(fieldcache, 27, displGrad2));
		for (int c = 0; c < 27; ++c)
			EXPECT_NEAR(displGrad2Answer[i][c], displGrad2[c], TOL);
	}
}

// Test gradient and gradient of gradient with both deformed and reference coordinates distorted.
// Compare against gradient calculated from differences in coordinates over a span of coordinates,
// and gradient of gradient calculated from differences of gradient
TEST(ZincFieldGradient, gradientOfGradient2)
{
	ZincTestSetupCpp zinc;

	EXPECT_EQ(RESULT_OK, zinc.root_region.readFile(TestResources::getLocation(TestResources::FIELDMODULE_CUBE_TRICUBIC_DEFORMED_RESOURCE)));

	Field coordinates = zinc.fm.findFieldByName("coordinates");
	Field deformed = zinc.fm.findFieldByName("deformed");
	// make coordinates distorted too by adding scaled functions of cos coordinates
	double projectionMatrixValues[16] =
	{
		0.1, 4.1, 0.2, 1.2,
		0.2, 0.2, 1.7, 0.1,
		10.5, 0.5, 0.2, 0.5,
		0.0, 0.0, 0.0, 1.5,
	};
	FieldConstant projectionMatrix = zinc.fm.createFieldConstant(16, projectionMatrixValues);
	FieldProjection projectedCoordinates = zinc.fm.createFieldProjection(coordinates, projectionMatrix);
	const double scaleValues[3] = { 0.1, 0.2, 0.15 };
	FieldConstant scale = zinc.fm.createFieldConstant(3, scaleValues);
	FieldAdd newCoordinates = coordinates + scale*zinc.fm.createFieldCos(projectedCoordinates);
	EXPECT_EQ(RESULT_OK, coordinates.createFieldassignment(newCoordinates).assign());

	Field deformedGradient1 = zinc.fm.createFieldGradient(deformed, coordinates);
	EXPECT_TRUE(deformedGradient1.isValid());
	Field deformedGradient2 = zinc.fm.createFieldGradient(deformedGradient1, coordinates);
	EXPECT_TRUE(deformedGradient2.isValid());

	Mesh mesh = zinc.fm.findMeshByDimension(3);
	EXPECT_TRUE(mesh.isValid());
	const double zero3[3] = { 0.0, 0.0, 0.0 };
	FieldConstant constCoordinates = zinc.fm.createFieldConstant(3, zero3);
	EXPECT_TRUE(constCoordinates.isValid());
	FieldFindMeshLocation findMeshLocation = zinc.fm.createFieldFindMeshLocation(constCoordinates, coordinates, mesh);
	EXPECT_TRUE(findMeshLocation.isValid());
	EXPECT_EQ(RESULT_OK, findMeshLocation.setSearchMode(FieldFindMeshLocation::SEARCH_MODE_EXACT));

	const double xi[4][3] =
	{
		{ 0.5, 0.5, 0.5 },
		{ 0.1, 0.2, 0.3 },
		{ 0.6, 0.9, 0.2 },
		{ 0.3, 0.3, 0.7 }
	};

	Fieldcache fieldcache = zinc.fm.createFieldcache();
	const double TOL0 = 1.0E-12;
	const double TOL1 = 1.0E-8;
	const double TOL2 = 1.0E-7;
	double xCentre[3];
	double xOffset[3];
	double xiOffset[3];
	double deformedCentre[3];
	double deformedOffset[6][3];
	double gradient1Offset[6][9];
	const double delta = 1.0E-5;
	double gradient1[9], gradient2[27];
	double deformedMinus[3], deformedPlus[3];
	double gradient1Minus[9], gradient1Plus[9];
	double expectedGradient1[9], expectedGradient2[27];
	// have some known results to ensure coordinates and deformed are evaluating correctly
	const double expectedXCentre[4][3] =
	{
		0.43686074177676704, 0.64397744904338272, 0.44950266755367074,
		0.11667294587192019, 0.38040664800582774, 0.26357751190891537,
		0.49974195958953088, 1.0762492661274996, 0.23747305597961940,
		0.28350554575983011, 0.41798289896679547, 0.54944658282447456
	};
	const double expectedDeformedCentre[4][3] =
	{
		0.50075510807820856, 0.51143970478732725, 0.51222453477511909,
		0.090562253021671868, 0.19765535751251320, 0.33502917794672082,
		0.60628064212189914, 0.93268142947955301, 0.16913508415768003,
		0.29490813871204041, 0.29339708509827261, 0.70171923555405613
	};
	for (int i = 0; i < 4; ++i)
	{
		Element element = mesh.findElementByIdentifier(1);
		EXPECT_EQ(RESULT_OK, fieldcache.setMeshLocation(element, 3, xi[i]));
		EXPECT_EQ(RESULT_OK, coordinates.evaluateReal(fieldcache, 3, xCentre));
		EXPECT_EQ(RESULT_OK, deformed.evaluateReal(fieldcache, 3, deformedCentre));
		for (int k = 0; k < 3; ++k)
		{
			EXPECT_NEAR(expectedXCentre[i][k], xCentre[k], TOL0);
			EXPECT_NEAR(expectedDeformedCentre[i][k], deformedCentre[k], TOL0);
		}
		EXPECT_EQ(RESULT_OK, deformedGradient1.evaluateReal(fieldcache, 9, gradient1));
		EXPECT_EQ(RESULT_OK, deformedGradient2.evaluateReal(fieldcache, 27, gradient2));
		// now compare with finite difference calculated values
		for (int j = 0; j < 3; ++j)
		{
			xOffset[0] = xCentre[0];
			xOffset[1] = xCentre[1];
			xOffset[2] = xCentre[2];
			xOffset[j] -= delta;
			EXPECT_EQ(RESULT_OK, constCoordinates.assignReal(fieldcache, 3, xOffset));
			element = findMeshLocation.evaluateMeshLocation(fieldcache, 3, xiOffset);
			EXPECT_TRUE(element.isValid());
			EXPECT_EQ(RESULT_OK, fieldcache.setMeshLocation(element, 3, xiOffset));
			EXPECT_EQ(RESULT_OK, deformed.evaluateReal(fieldcache, 3, deformedMinus));
			EXPECT_EQ(RESULT_OK, deformedGradient1.evaluateReal(fieldcache, 9, gradient1Minus));
			xOffset[j] = xCentre[j] + delta;
			EXPECT_EQ(RESULT_OK, constCoordinates.assignReal(fieldcache, 3, xOffset));
			element = findMeshLocation.evaluateMeshLocation(fieldcache, 3, xiOffset);
			EXPECT_TRUE(element.isValid());
			EXPECT_EQ(RESULT_OK, fieldcache.setMeshLocation(element, 3, xiOffset));
			EXPECT_EQ(RESULT_OK, deformed.evaluateReal(fieldcache, 3, deformedPlus));
			EXPECT_EQ(RESULT_OK, deformedGradient1.evaluateReal(fieldcache, 9, gradient1Plus));
			for (int k = 0; k < 3; ++k)
				expectedGradient1[k*3 + j] = (deformedPlus[k] - deformedMinus[k]) / (2.0*delta);
			for (int k = 0; k < 9; ++k)
				expectedGradient2[k*3 + j] = (gradient1Plus[k] - gradient1Minus[k]) / (2.0*delta);
		}
		for (int k = 0; k < 9; ++k)
			EXPECT_NEAR(expectedGradient1[k], gradient1[k], TOL1);
		for (int k = 0; k < 27; ++k)
			EXPECT_NEAR(expectedGradient2[k], gradient2[k], TOL2);
	}
}

// Issue 3317: Gradient field calculations for grid based scalar fields are not
// being scaled by the number of grid points in each xi direction. The resulting
// gradients are smaller than their correct values.
TEST(cmzn_field, issue_3317_grid_derivatives_wrt_xi)
{
	ZincTestSetup zinc;

	int result;
	result = cmzn_region_read_file(zinc.root_region, TestResources::getLocation(TestResources::FIELDMODULE_CUBE_RESOURCE));
	EXPECT_EQ(CMZN_OK, result);
	result = cmzn_region_read_file(zinc.root_region, TestResources::getLocation(TestResources::FIELDMODULE_CUBE_GRID_RESOURCE));
	EXPECT_EQ(CMZN_OK, result);

	cmzn_mesh_id mesh = cmzn_fieldmodule_find_mesh_by_dimension(zinc.fm, 3);
	EXPECT_NE((cmzn_mesh_id)0, mesh);

	cmzn_differentialoperator_id d_dxi1 = cmzn_mesh_get_chart_differentialoperator(mesh, /*order*/1, /*term*/1);
	EXPECT_NE((cmzn_differentialoperator_id)0, d_dxi1);
	cmzn_differentialoperator_id d_dxi2 = cmzn_mesh_get_chart_differentialoperator(mesh, /*order*/1, /*term*/2);
	EXPECT_NE((cmzn_differentialoperator_id)0, d_dxi2);
	cmzn_differentialoperator_id d_dxi3 = cmzn_mesh_get_chart_differentialoperator(mesh, /*order*/1, /*term*/3);
	EXPECT_NE((cmzn_differentialoperator_id)0, d_dxi3);

	cmzn_field_id coordinates = cmzn_fieldmodule_find_field_by_name(zinc.fm, "coordinates");
	EXPECT_NE(static_cast<cmzn_field *>(0), coordinates);
	cmzn_field_id potential = cmzn_fieldmodule_find_field_by_name(zinc.fm, "potential");
	EXPECT_NE(static_cast<cmzn_field *>(0), potential);

	cmzn_fieldcache_id cache = cmzn_fieldmodule_create_fieldcache(zinc.fm);
	cmzn_element_id element = cmzn_mesh_find_element_by_identifier(mesh, 1);
	const double chartLocation[] = { 0.25, 1.0/6.0, 0.125 };
	cmzn_fieldcache_set_mesh_location(cache, element, 3, chartLocation);
	cmzn_element_destroy(&element);

	double outValue;
	EXPECT_EQ(CMZN_OK, result = cmzn_field_evaluate_real(potential, cache, 1, &outValue));
	EXPECT_DOUBLE_EQ(1.75, outValue);
	EXPECT_EQ(CMZN_OK, result = cmzn_field_evaluate_derivative(potential, d_dxi1, cache, 1, &outValue));
	EXPECT_DOUBLE_EQ(2.0, outValue);
	EXPECT_EQ(CMZN_OK, result = cmzn_field_evaluate_derivative(potential, d_dxi2, cache, 1, &outValue));
	EXPECT_DOUBLE_EQ(1.5, outValue);
	EXPECT_EQ(CMZN_OK, result = cmzn_field_evaluate_derivative(potential, d_dxi3, cache, 1, &outValue));
	EXPECT_DOUBLE_EQ(8.0, outValue);

	cmzn_field_id grad_potential = cmzn_fieldmodule_create_field_gradient(zinc.fm, potential, coordinates);
	EXPECT_NE(static_cast<cmzn_field *>(0), grad_potential);

	double outValues[3];
	EXPECT_EQ(CMZN_OK, result = cmzn_field_evaluate_real(grad_potential, cache, 3, outValues));
	EXPECT_DOUBLE_EQ(2.0, outValues[0]);
	EXPECT_DOUBLE_EQ(1.5, outValues[1]);
	EXPECT_DOUBLE_EQ(8.0, outValues[2]);

	cmzn_field_destroy(&grad_potential);
	cmzn_fieldcache_destroy(&cache);
	cmzn_field_destroy(&potential);
	cmzn_field_destroy(&coordinates);
	cmzn_differentialoperator_destroy(&d_dxi1);
	cmzn_differentialoperator_destroy(&d_dxi2);
	cmzn_differentialoperator_destroy(&d_dxi3);
	cmzn_mesh_destroy(&mesh);
}

// Issue 3812 Grid-based field component derivatives only work for first component
// Derivatives for higher grid-based components were overwriting those of first component
TEST(ZincField, issue_3812_grid_derivatives_non_first_component)
{
	ZincTestSetupCpp zinc;
	int result;

	EXPECT_EQ(OK, result = zinc.root_region.readFile(
		TestResources::getLocation(TestResources::FIELDMODULE_CUBE_XYZP_RESOURCE)));

	Field dependent = zinc.fm.findFieldByName("dependent");
	EXPECT_TRUE(dependent.isValid());
	EXPECT_EQ(4, dependent.getNumberOfComponents());

	Mesh mesh = zinc.fm.findMeshByDimension(3);
	Differentialoperator d_dxi1 = mesh.getChartDifferentialoperator(1, 1);
	EXPECT_TRUE(d_dxi1.isValid());
	Differentialoperator d_dxi2 = mesh.getChartDifferentialoperator(1, 2);
	EXPECT_TRUE(d_dxi2.isValid());
	Differentialoperator d_dxi3 = mesh.getChartDifferentialoperator(1, 3);
	EXPECT_TRUE(d_dxi3.isValid());

	Fieldcache cache = zinc.fm.createFieldcache();
	EXPECT_TRUE(cache.isValid());
	Element element = mesh.findElementByIdentifier(1);
	EXPECT_TRUE(element.isValid());
	const double xi[3] = { 0.5, 0.5, 0.5 };
	EXPECT_EQ(OK, result = cache.setMeshLocation(element, 3, xi));

	const double expected_x[4] = { 0.5, 0.5, 0.5, 1.23456789 };
	double x[4];
	EXPECT_EQ(OK, result = dependent.evaluateReal(cache, 4, x));
	for (int i = 0; i < 4; ++i)
		EXPECT_DOUBLE_EQ(expected_x[i], x[i]);

	const double expected_dx_dxi1[4] = { 1.0, 0.0, 0.0, 0.0 };
	double dx_dxi1[4];
	EXPECT_EQ(OK, result = dependent.evaluateDerivative(d_dxi1, cache, 4, dx_dxi1));
	for (int i = 0; i < 4; ++i)
		EXPECT_DOUBLE_EQ(expected_dx_dxi1[i], dx_dxi1[i]);

	const double expected_dx_dxi2[4] = { 0.0, 1.0, 0.0, 0.0 };
	double dx_dxi2[4];
	EXPECT_EQ(OK, result = dependent.evaluateDerivative(d_dxi2, cache, 4, dx_dxi2));
	for (int i = 0; i < 4; ++i)
		EXPECT_DOUBLE_EQ(expected_dx_dxi2[i], dx_dxi2[i]);

	const double expected_dx_dxi3[4] = { 0.0, 0.0, 1.0, 0.0 };
	double dx_dxi3[4];
	EXPECT_EQ(OK, result = dependent.evaluateDerivative(d_dxi3, cache, 4, dx_dxi3));
	for (int i = 0; i < 4; ++i)
		EXPECT_DOUBLE_EQ(expected_dx_dxi3[i], dx_dxi3[i]);
}

