/*
 * OpenCMISS-Zinc Library Unit Tests
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <gtest/gtest.h>

#include "zinctestsetup.hpp"
#include <opencmiss/zinc/core.h>
#include <opencmiss/zinc/field.h>
#include <opencmiss/zinc/fieldconstant.h>
#include <opencmiss/zinc/optimisation.h>

#include "zinctestsetupcpp.hpp"
#include <opencmiss/zinc/field.hpp>
#include <opencmiss/zinc/fieldarithmeticoperators.hpp>
#include <opencmiss/zinc/fieldcache.hpp>
#include <opencmiss/zinc/fieldcomposite.hpp>
#include <opencmiss/zinc/fieldconstant.hpp>
#include <opencmiss/zinc/fieldderivatives.hpp>
#include <opencmiss/zinc/fieldfiniteelement.hpp>
#include <opencmiss/zinc/fieldlogicaloperators.hpp>
#include <opencmiss/zinc/fieldmatrixoperators.hpp>
#include <opencmiss/zinc/fieldmeshoperators.hpp>
#include <opencmiss/zinc/fieldnodesetoperators.hpp>
#include <opencmiss/zinc/fieldsubobjectgroup.hpp>
#include <opencmiss/zinc/optimisation.hpp>

#include "test_resources.h"

TEST(cmzn_optimisation, arguments)
{
	ZincTestSetup zinc;
	int result;

	cmzn_optimisation_id optimisation = cmzn_fieldmodule_create_optimisation(zinc.fm);
	EXPECT_NE(static_cast<cmzn_optimisation_id>(0), optimisation);

	EXPECT_EQ(CMZN_OPTIMISATION_METHOD_QUASI_NEWTON, cmzn_optimisation_get_method(optimisation));
	EXPECT_EQ(OK, result = cmzn_optimisation_set_method(optimisation, CMZN_OPTIMISATION_METHOD_LEAST_SQUARES_QUASI_NEWTON));
	EXPECT_EQ(CMZN_OPTIMISATION_METHOD_LEAST_SQUARES_QUASI_NEWTON, cmzn_optimisation_get_method(optimisation));

	cmzn_optimisation_destroy(&optimisation);
}

TEST(ZincOptimisation, arguments)
{
	ZincTestSetupCpp zinc;
	int result;

	Optimisation optimisation = zinc.fm.createOptimisation();
	EXPECT_TRUE(optimisation.isValid());

	EXPECT_EQ(Optimisation::METHOD_QUASI_NEWTON, optimisation.getMethod());
	EXPECT_EQ(OK, result = optimisation.setMethod(Optimisation::METHOD_LEAST_SQUARES_QUASI_NEWTON));
	EXPECT_EQ(Optimisation::METHOD_LEAST_SQUARES_QUASI_NEWTON, optimisation.getMethod());

	// made-up fields to test objective/independent field APIs
	FieldFiniteElement f1 = zinc.fm.createFieldFiniteElement(3);
	EXPECT_TRUE(f1.isValid());
	FieldFiniteElement f2 = zinc.fm.createFieldFiniteElement(1);
	EXPECT_TRUE(f2.isValid());
	FieldFiniteElement f3 = zinc.fm.createFieldFiniteElement(3);
	EXPECT_TRUE(f3.isValid());
	Field f4 = zinc.fm.createFieldMultiply(f1, f2);
	EXPECT_TRUE(f4.isValid());
	Field f5 = zinc.fm.createFieldMultiply(f4, f3);
	EXPECT_TRUE(f5.isValid());
	const double oneValue = 1.0;
	Field fcond = zinc.fm.createFieldConstant(1, &oneValue);
	EXPECT_TRUE(fcond.isValid());

	Field temp;
	EXPECT_EQ(OK, result = optimisation.addIndependentField(f1));
	EXPECT_EQ(OK, result = optimisation.addIndependentField(f2));
	EXPECT_EQ(OK, result = optimisation.addIndependentField(f3));
	EXPECT_EQ(ERROR_ARGUMENT, result = optimisation.addIndependentField(f1));
	temp = optimisation.getFirstIndependentField();
	EXPECT_EQ(f1, temp);
	temp = optimisation.getNextIndependentField(temp);
	EXPECT_EQ(f2, temp);
	temp = optimisation.getNextIndependentField(temp);
	EXPECT_EQ(f3, temp);
	temp = optimisation.getNextIndependentField(temp);
	EXPECT_FALSE(temp.isValid());
	temp = optimisation.getConditionalField(f1);
	EXPECT_FALSE(temp.isValid());
	temp = optimisation.getConditionalField(f2);
	EXPECT_FALSE(temp.isValid());
	temp = optimisation.getConditionalField(f3);
	EXPECT_FALSE(temp.isValid());
	EXPECT_EQ(ERROR_ARGUMENT, result = optimisation.setConditionalField(f2, f3)); // wrong #components
	EXPECT_EQ(OK, result = optimisation.setConditionalField(f2, fcond));
	temp = optimisation.getConditionalField(f2);
	EXPECT_EQ(fcond, temp);

	EXPECT_EQ(ERROR_ARGUMENT, result = optimisation.removeIndependentField(f4)); // not in use
	EXPECT_EQ(OK, result = optimisation.removeIndependentField(f1));
	temp = optimisation.getFirstIndependentField();
	EXPECT_EQ(f2, temp);
	temp = optimisation.getNextIndependentField(temp);
	EXPECT_EQ(f3, temp);
	temp = optimisation.getNextIndependentField(temp);
	EXPECT_FALSE(temp.isValid());
	temp = optimisation.getConditionalField(f2);
	EXPECT_EQ(fcond, temp);
	EXPECT_EQ(OK, result = optimisation.setConditionalField(f2, Field()));
	temp = optimisation.getConditionalField(f2);
	EXPECT_FALSE(temp.isValid());

	EXPECT_EQ(OK, result = optimisation.addObjectiveField(f4));
	EXPECT_EQ(OK, result = optimisation.addObjectiveField(f5));
	temp = optimisation.getFirstObjectiveField();
	EXPECT_EQ(f4, temp);
	temp = optimisation.getNextObjectiveField(temp);
	EXPECT_EQ(f5, temp);
	temp = optimisation.getNextObjectiveField(temp);
	EXPECT_FALSE(temp.isValid());

	EXPECT_EQ(ERROR_ARGUMENT, result = optimisation.removeObjectiveField(f1));
	EXPECT_EQ(OK, result = optimisation.removeObjectiveField(f5));
	temp = optimisation.getFirstObjectiveField();
	EXPECT_EQ(f4, temp);
	temp = optimisation.getNextObjectiveField(temp);
	EXPECT_FALSE(temp.isValid());
}

// A non-linear optimisation for deformation of a tricubic Lagrange unit cube.
// All nodes on end1 are fixed in x,y,z by being excluded as DOFs
// All nodes on end2 are fixed to x=1.5 by a penalty
// Incompressible and strain penalties are applied at Gauss points.
// The solution proves that the cube gets thinner, symmetrically at end2 as it extends
TEST(ZincOptimisation, tricubicFit)
{
	ZincTestSetupCpp zinc;
	int result;

	EXPECT_EQ(OK, result = zinc.root_region.readFile(
		TestResources::getLocation(TestResources::OPTIMISATION_CUBE_TRICUBIC_LAGRANGE_RESOURCE)));
	Field referenceCoordinates = zinc.fm.findFieldByName("coordinates");
	EXPECT_TRUE(referenceCoordinates.isValid());
	EXPECT_EQ(OK, referenceCoordinates.setName("reference_coordinates"));
	EXPECT_EQ(OK, result = zinc.root_region.readFile(
		TestResources::getLocation(TestResources::OPTIMISATION_CUBE_TRICUBIC_LAGRANGE_RESOURCE)));
	Field coordinates = zinc.fm.findFieldByName("coordinates");
	EXPECT_TRUE(coordinates.isValid());

	zinc.fm.beginChange();
	Nodeset nodeset = zinc.fm.findNodesetByFieldDomainType(Field::DOMAIN_TYPE_NODES);
	EXPECT_TRUE(nodeset.isValid());

	const int end2NodeIdentifiers[] = { 4,8,12,16,20,24,28,32,36,40,44,48,52,56,60,64 };
	const int end2NodeIdentifiersSize = sizeof(end2NodeIdentifiers)/sizeof(int);
	FieldNodeGroup end2NodeGroup = zinc.fm.createFieldNodeGroup(nodeset);
	NodesetGroup end2Nodeset = end2NodeGroup.getNodesetGroup();
	EXPECT_TRUE(end2Nodeset.isValid());
	for (int i = 0; i < end2NodeIdentifiersSize; ++i)
	{
		Node node = nodeset.findNodeByIdentifier(end2NodeIdentifiers[i]);
		EXPECT_TRUE(node.isValid());
		EXPECT_EQ(OK, result = end2Nodeset.addNode(node));
	}
	zinc.fm.endChange();

	Field displacement = zinc.fm.createFieldSubtract(coordinates, referenceCoordinates);

	const double penalty1Value = 10.0;
	FieldConstant penalty1 = zinc.fm.createFieldConstant(1, &penalty1Value);

	FieldComponent x = zinc.fm.createFieldComponent(coordinates, 1);
	const double xEndValue = 1.5;
	FieldConstant xEnd = zinc.fm.createFieldConstant(1, &xEndValue);
	FieldNodesetSumSquares end2Objective = zinc.fm.createFieldNodesetSumSquares(penalty1*(x - xEnd), end2Nodeset);
	EXPECT_TRUE(end2Objective.isValid());

	const double alphaValue = 0.2;
	Field alpha = zinc.fm.createFieldConstant(1, &alphaValue);
#if defined (ZINC_USE_ITK)
	Field displacementGradient = zinc.fm.createFieldGradient(displacement, referenceCoordinates);

	Field F = zinc.fm.createFieldGradient(coordinates, referenceCoordinates);
	Field detF = zinc.fm.createFieldDeterminant(F);
	const double oneValue = 1.0;
	Field one = zinc.fm.createFieldConstant(1, &oneValue);

	Field WComponents[] = { alpha*displacementGradient, penalty1*(detF - one) };
	Field W = zinc.fm.createFieldConcatenate(2, WComponents);

	Mesh mesh3d = zinc.fm.findMeshByDimension(3);
	FieldMeshIntegralSquares WObjective = zinc.fm.createFieldMeshIntegralSquares(W, referenceCoordinates, mesh3d);
	EXPECT_TRUE(WObjective.isValid());
	const int numGaussPoints = 4;
	EXPECT_EQ(OK, result = WObjective.setNumbersOfPoints(1, &numGaussPoints));

	const double xConstValue = 0.1;
	FieldConstant xConst = zinc.fm.createFieldConstant(1, &xConstValue);
	EXPECT_TRUE(xConst.isValid());
	Field xCondition = x > xConst;
	EXPECT_TRUE(xCondition.isValid());

	Optimisation optimisation = zinc.fm.createOptimisation();
	EXPECT_TRUE(optimisation.isValid());

	EXPECT_EQ(OK, result = optimisation.addObjectiveField(end2Objective));
	EXPECT_EQ(OK, result = optimisation.addObjectiveField(WObjective));
	EXPECT_EQ(OK, result = optimisation.addIndependentField(coordinates));
	EXPECT_EQ(OK, result = optimisation.setConditionalField(coordinates, xCondition));
	EXPECT_EQ(OK, result = optimisation.setMethod(Optimisation::METHOD_LEAST_SQUARES_QUASI_NEWTON));
	EXPECT_EQ(OK, result = optimisation.setAttributeInteger(Optimisation::ATTRIBUTE_MAXIMUM_ITERATIONS, 10));

	EXPECT_EQ(OK, result = optimisation.optimise());
	char *solutionReport = optimisation.getSolutionReport();
	EXPECT_NE((char *)0, solutionReport);
	printf("%s", solutionReport);
	const char *dimensionText = strstr(solutionReport,
		"Dimension of the problem  = 144");
	EXPECT_NE((const char *)0, dimensionText);
	const char *convergedText = strstr(solutionReport,
		"Return code               = 2 (Algorithm converged - Difference in successive fcn values is less than tolerance)");
	EXPECT_NE((const char *)0, convergedText);
	const char *iterationsText = strstr(solutionReport,
		"No. iterations taken      = 5");
	EXPECT_NE((const char *)0, iterationsText);

	Fieldcache cache = zinc.fm.createFieldcache();
	EXPECT_TRUE(cache.isValid());
	const double expectedWValues[10] =
	{
		0.011449513901451864,
		1.1196820095409539e-006,
		1.1196845955486078e-006,
		0.00039399120829730822,
		0.0014075399963259518,
		5.6687058307447950e-006,
		0.00039399120475952152,
		5.6687063105913627e-006,
		0.0014075399951463652,
		2.4300383826273377e-006
	};

	double WValues[10];
	const double tolerance = 1.0E-9;
	EXPECT_EQ(OK, result = WObjective.evaluateReal(cache, 10, WValues));
	for (int i = 0; i < 10; ++i)
		EXPECT_NEAR(expectedWValues[i], WValues[i], tolerance);

	FieldMeshIntegral volume = zinc.fm.createFieldMeshIntegral(one, coordinates, mesh3d);
	EXPECT_EQ(OK, volume.setNumbersOfPoints(1, &numGaussPoints));
	const double expectedVolumeValue = 1.0001085833573624;
	double volumeValue;
	EXPECT_EQ(OK, result = volume.evaluateReal(cache, 1, &volumeValue));
	EXPECT_NEAR(expectedVolumeValue, volumeValue, tolerance);

	cmzn_deallocate(solutionReport);
#endif
}
