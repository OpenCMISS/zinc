/*
 * OpenCMISS-Zinc Library Unit Tests
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <gtest/gtest.h>

#include <opencmiss/zinc/element.hpp>
#include <opencmiss/zinc/field.hpp>
#include <opencmiss/zinc/fieldconstant.hpp>
#include <opencmiss/zinc/fieldderivatives.hpp>
#include <opencmiss/zinc/fieldparameters.hpp>
#include <opencmiss/zinc/fieldcache.hpp>
#include <opencmiss/zinc/fieldfiniteelement.hpp>

#include "utilities/zinctestsetupcpp.hpp"
#include "test_resources.h"

TEST(ZincFieldparameters, validAPI)
{
	ZincTestSetupCpp zinc;

	EXPECT_EQ(RESULT_OK, zinc.root_region.readFile(TestResources::getLocation(TestResources::FIELDMODULE_CUBE_RESOURCE)));

	Field coordinates = zinc.fm.findFieldByName("coordinates");
	EXPECT_TRUE(coordinates.isValid());
	const int componentCount = coordinates.getNumberOfComponents();
	EXPECT_EQ(3, componentCount);

	Mesh mesh3d = zinc.fm.findMeshByDimension(3);
	Element element = mesh3d.findElementByIdentifier(1);
	EXPECT_TRUE(element.isValid());

	Fieldparameters fieldparameters = coordinates.getFieldparameters();
	EXPECT_TRUE(fieldparameters.isValid());
	Field fieldOut = fieldparameters.getField();
	EXPECT_TRUE(fieldOut.isValid());
	EXPECT_EQ(coordinates, fieldOut);

	EXPECT_EQ(24, fieldparameters.getNumberOfElementParameters(element));
	EXPECT_EQ(24, fieldparameters.getNumberOfParameters());

	Differentialoperator parameterDerivative1 = fieldparameters.getDerivativeOperator(/*order*/1);
	EXPECT_TRUE(parameterDerivative1.isValid());
	Differentialoperator parameterDerivative2 = fieldparameters.getDerivativeOperator(/*order*/2);
	EXPECT_TRUE(parameterDerivative2.isValid());

	Fieldcache fieldcache = zinc.fm.createFieldcache();
	EXPECT_TRUE(fieldcache.isValid());

	double outDerivativeParameters1[72];
	double outDerivativeParameters2[1728];

	// test that first derivatives of components w.r.t. their parameters equal the basis functions
	// but are zero w.r.t. other components' parameters
	const double xi[4][3] =
	{
		{ 0.2, 0.5, 0.7 },
		{ 0.0, 0.9, 0.2 },
		{ 0.7, 0.2, 0.7 },
		{ 0.9, 0.9, 1.0 }
	};
	const double TOL = 1.0E-12;
	for (int i = 0; i < 4; ++i)
	{
		EXPECT_EQ(RESULT_OK, fieldcache.setMeshLocation(element, 3, xi[i]));
		EXPECT_EQ(RESULT_OK, coordinates.evaluateDerivative(parameterDerivative1, fieldcache, 72, outDerivativeParameters1));
		const double phi12 = xi[i][0];
		const double phi22 = xi[i][1];
		const double phi32 = xi[i][2];
		const double phi11 = 1.0 - phi12;
		const double phi21 = 1.0 - phi22;
		const double phi31 = 1.0 - phi32;
		const double trilinearBasis[8] =
		{
			phi11*phi21*phi31, phi12*phi21*phi31, phi11*phi22*phi31, phi12*phi22*phi31,
			phi11*phi21*phi32, phi12*phi21*phi32, phi11*phi22*phi32, phi12*phi22*phi32
		};
		int v = 0;
		for (int c = 0; c < componentCount; ++c)
		{
			for (int d = 0; d < componentCount; ++d)
			{
				if (d == c)
				{
					for (int p = 0; p < 8; ++p)
						EXPECT_NEAR(trilinearBasis[p], outDerivativeParameters1[v++], TOL);
				}
				else
				{
					for (int p = 0; p < 8; ++p)
						EXPECT_DOUBLE_EQ(0.0, outDerivativeParameters1[v++]);
				}
			}
		}
		EXPECT_EQ(RESULT_OK, coordinates.evaluateDerivative(parameterDerivative2, fieldcache, 1728, outDerivativeParameters2));
		// test that second derivatives w.r.t. parameters are all zero
		for (int j = 0; j < 1728; ++j)
			EXPECT_DOUBLE_EQ(0.0, outDerivativeParameters2[j]);
	}
}

TEST(ZincFieldparameters, invalidAPI)
{
	ZincTestSetupCpp zinc;

	FieldFiniteElement fieldFiniteElement = zinc.fm.createFieldFiniteElement(1);
	EXPECT_TRUE(fieldFiniteElement.isValid());
	Element noElement;
	Mesh mesh3d = zinc.fm.findMeshByDimension(3);
	Elementtemplate elementtemplate = mesh3d.createElementtemplate();
	EXPECT_EQ(RESULT_OK, elementtemplate.setElementShapeType(Element::SHAPE_TYPE_CUBE));
	Element element = mesh3d.createElement(1, elementtemplate);
	EXPECT_TRUE(element.isValid());

	Fieldparameters fieldparameters = fieldFiniteElement.getFieldparameters();
	EXPECT_TRUE(fieldparameters.isValid());
	Field noField;
	EXPECT_FALSE(noField.isValid());
	Fieldparameters noFieldparameters = noField.getFieldparameters();
	EXPECT_FALSE(noFieldparameters.isValid());
	Field fieldOut = noFieldparameters.getField();
	EXPECT_FALSE(fieldOut.isValid());

	EXPECT_EQ(-1, fieldparameters.getNumberOfElementParameters(noElement));
	EXPECT_EQ(-1, noFieldparameters.getNumberOfElementParameters(element));
	EXPECT_EQ(-1, noFieldparameters.getNumberOfElementParameters(noElement));
	EXPECT_EQ(-1, noFieldparameters.getNumberOfParameters());

	Differentialoperator parameterDerivative1 = fieldparameters.getDerivativeOperator(/*order*/0);
	EXPECT_FALSE(parameterDerivative1.isValid());
	Differentialoperator parameterDerivative2 = fieldparameters.getDerivativeOperator(/*order*/3);
	EXPECT_FALSE(parameterDerivative2.isValid());

	const double one = 1.0;
	FieldConstant fieldConstant = zinc.fm.createFieldConstant(1, &one);
	EXPECT_TRUE(fieldConstant.isValid());
	noFieldparameters = fieldConstant.getFieldparameters();
	EXPECT_FALSE(noFieldparameters.isValid());
}

// Test 3 component field on 2d square with mixed quadratic-linear basis
// and scaled general linear maps to prove correct summation of basis
// functions with scaling by multiple parameters in parameter derivatives.
TEST(ZincFieldparameters, mixedBasisMultipleScaledTerms)
{
	ZincTestSetupCpp zinc;

	// create 2D model with 3D coordinates

	EXPECT_EQ(RESULT_OK, zinc.fm.beginChange());

	const int componentCount = 3;
	FieldFiniteElement coordinates = zinc.fm.createFieldFiniteElement(componentCount);
	EXPECT_EQ(RESULT_OK, coordinates.setName("coordinates"));
	EXPECT_EQ(RESULT_OK, coordinates.setTypeCoordinate(true));
	EXPECT_EQ(RESULT_OK, coordinates.setComponentName(1, "x"));
	EXPECT_EQ(RESULT_OK, coordinates.setComponentName(2, "y"));
	EXPECT_EQ(RESULT_OK, coordinates.setComponentName(3, "z"));
	EXPECT_TRUE(coordinates.isValid());

	Nodeset nodes = zinc.fm.findNodesetByFieldDomainType(Field::DOMAIN_TYPE_NODES);
	Nodetemplate nodetemplate = nodes.createNodetemplate();
	EXPECT_EQ(RESULT_OK, nodetemplate.defineField(coordinates));
	const double x[6][3] = {
		{ 0.1, 0.1, 0.1 },
		{ 1.5,-0.0,-0.3 },
		{ 2.9,-0.1, 0.1 },
		{ 0.0, 1.0, 0.0 },
		{ 1.4, 1.1, 0.4 },
		{ 3.0, 1.2, 0.1 }
	};
	Fieldcache fieldcache = zinc.fm.createFieldcache();
	for (int n = 0; n < 6; ++n)
	{
		Node node = nodes.createNode(n + 1, nodetemplate);
		EXPECT_TRUE(node.isValid());
		EXPECT_EQ(RESULT_OK, fieldcache.setNode(node));
		EXPECT_EQ(RESULT_OK, coordinates.setNodeParameters(fieldcache, -1, Node::VALUE_LABEL_VALUE, 1, 3, x[n]));
	}

	Mesh mesh2d = zinc.fm.findMeshByDimension(2);

	Elementbasis elementbasis = zinc.fm.createElementbasis(2, Elementbasis::FUNCTION_TYPE_QUADRATIC_LAGRANGE);
	EXPECT_EQ(RESULT_OK, elementbasis.setFunctionType(2, Elementbasis::FUNCTION_TYPE_LINEAR_LAGRANGE));
	EXPECT_TRUE(elementbasis.isValid());
	EXPECT_EQ(6, elementbasis.getNumberOfFunctions());
	EXPECT_EQ(6, elementbasis.getNumberOfNodes());
	for (int n = 1; n <= 6; ++n)
		EXPECT_EQ(1, elementbasis.getNumberOfFunctionsPerNode(n));

	Elementfieldtemplate eft = mesh2d.createElementfieldtemplate(elementbasis);
	EXPECT_TRUE(eft.isValid());
	EXPECT_EQ(6, eft.getNumberOfFunctions());
	EXPECT_EQ(6, eft.getNumberOfLocalNodes());
	EXPECT_EQ(RESULT_OK, eft.setNumberOfLocalScaleFactors(3));
	for (int i = 1; i <= 3; ++i)
	{
		EXPECT_EQ(RESULT_OK, eft.setScaleFactorType(i, Elementfieldtemplate::SCALE_FACTOR_TYPE_GLOBAL_GENERAL));
		EXPECT_EQ(RESULT_OK, eft.setScaleFactorIdentifier(i, i));
	}

	// add to node 4 coordinates: node 1 coordinates * scale factors 1, 2
	EXPECT_EQ(RESULT_OK, eft.setFunctionNumberOfTerms(4, 2));
	EXPECT_EQ(RESULT_OK, eft.setTermNodeParameter(4, 2, /*localNodeIndex*/1, Node::VALUE_LABEL_VALUE, /*version*/1));
	const int scalingIndexes4[] = { 1, 2 };
	EXPECT_EQ(RESULT_OK, eft.setTermScaling(4, 2, 2, scalingIndexes4));
	// scale node 6 coordinates by scale factor 3
	const int scalingIndexes6[] = { 3 };
	EXPECT_EQ(RESULT_OK, eft.setTermScaling(6, 1, 1, scalingIndexes6));

	Elementtemplate elementtemplate = mesh2d.createElementtemplate();
	elementtemplate.setElementShapeType(Element::SHAPE_TYPE_SQUARE);
	EXPECT_EQ(RESULT_OK, elementtemplate.defineField(coordinates, -1, eft));
	Element element = mesh2d.createElement(1, elementtemplate);
	EXPECT_TRUE(element.isValid());
	const int nodeIdentifiers[] = { 1, 2, 3, 4, 5, 6 };
	EXPECT_EQ(RESULT_OK, element.setNodesByIdentifier(eft, 6, nodeIdentifiers));
	const double scaleFactors[] = { -1.0, 1.5, 1.1 };
	EXPECT_EQ(RESULT_OK, element.setScaleFactors(eft, 3, scaleFactors));

	// get zinc first and second derivative fields
	EXPECT_EQ(RESULT_OK, zinc.fm.beginChange());
	FieldDerivative meshDerivatives1[2], meshDerivatives2[2][2];
	for (int i = 0; i < 2; ++i)
	{
		meshDerivatives1[i] = zinc.fm.createFieldDerivative(coordinates, i + 1);
		EXPECT_TRUE(meshDerivatives1[i].isValid());
		for (int j = 0; j < 2; ++j)
		{
			meshDerivatives2[i][j] = zinc.fm.createFieldDerivative(meshDerivatives1[i], j + 1);
			EXPECT_TRUE(meshDerivatives2[i][j].isValid());
		}
	}
	EXPECT_EQ(RESULT_OK, zinc.fm.endChange());

	Fieldparameters fieldparameters = coordinates.getFieldparameters();
	EXPECT_TRUE(fieldparameters.isValid());
	Field fieldOut = fieldparameters.getField();
	EXPECT_TRUE(fieldOut.isValid());
	EXPECT_EQ(coordinates, fieldOut);

	EXPECT_EQ(18, fieldparameters.getNumberOfElementParameters(element));
	EXPECT_EQ(18, fieldparameters.getNumberOfParameters());

	Differentialoperator parameterDerivative1 = fieldparameters.getDerivativeOperator(/*order*/1);
	EXPECT_TRUE(parameterDerivative1.isValid());

	double outDerivativeParameters1[54];
	double tmpDerivatives[54];
	double outDerivativesMesh1Parameters1[108];
	double outDerivativesMesh2Parameters1[216];
	// test that first derivatives of components w.r.t. their parameters equal the scaled sums
	// of the basis functions they are terms of but are zero w.r.t. other components' parameters
	const double xi[4][2] =
	{
		{ 0.5, 0.5 },
		{ 0.0, 0.9 },
		{ 0.7, 0.2 },
		{ 0.9, 1.0 }
	};
	const double TOL = 1.0E-12;
	for (int i = 0; i < 4; ++i)
	{
		EXPECT_EQ(RESULT_OK, fieldcache.setMeshLocation(element, 2, xi[i]));
		EXPECT_EQ(RESULT_OK, coordinates.evaluateDerivative(parameterDerivative1, fieldcache, 54, outDerivativeParameters1));
		// evaluate mixed parameter derivatives of mesh derivatives, working around limitation
		// of using legacy derivative field for one term at a time
		for (int i = 0; i < 2; ++i)
		{
			EXPECT_EQ(RESULT_OK, meshDerivatives1[i].evaluateDerivative(parameterDerivative1, fieldcache, 54, tmpDerivatives));
			for (int c = 0; c < 3; ++c)
				for (int p = 0; p < 18; ++p)
					outDerivativesMesh1Parameters1[c*36 + i*18 + p] = tmpDerivatives[c*18 + p];
			for (int j = 0; j < 2; ++j)
			{
				EXPECT_EQ(RESULT_OK, meshDerivatives2[i][j].evaluateDerivative(parameterDerivative1, fieldcache, 54, tmpDerivatives));
				for (int c = 0; c < 3; ++c)
					for (int p = 0; p < 18; ++p)
						outDerivativesMesh2Parameters1[c*72 + i*36 + j*18 + p] = tmpDerivatives[c*18 + p];
			}
		}
		const double xi1 = xi[i][0];
		const double xi2 = xi[i][1];
		const double xi1_2 = xi1*xi1;
		//std::cerr << "i " << i << " xi " << xi1 << ", " << xi2 << std::endl;
		// basis phi1[order][function]
		const double phi1[3][3] =
		{
			{ 1.0 - 3.0*xi1 + 2.0*xi1_2, 4.0*xi1 - 4.0*xi1_2, -1.0*xi1 + 2.0*xi1_2 },
			{ -3.0 + 4.0*xi1, 4.0 - 8.0*xi1, -1.0 + 4.0*xi1 },
			{ 4.0, -8.0, 4.0 }
		};
		// basis phi2[order][fucnction]
		const double phi2[3][2] =
		{
			{ 1.0 - xi2, xi2 },
			{ -1.0, 1.0 },
			{ 0.0, 0.0 }
		};
		const double quadraticLinearBasis[6] =
		{
			phi1[0][0]*phi2[0][0], phi1[0][1]*phi2[0][0], phi1[0][2]*phi2[0][0],
			phi1[0][0]*phi2[0][1], phi1[0][1]*phi2[0][1], phi1[0][2]*phi2[0][1]
		};
		int v = 0;
		for (int c = 0; c < componentCount; ++c)
			for (int d = 0; d < componentCount; ++d)
				if (d == c)
				{
					for (int p = 0; p < 6; ++p)
					{
						double expectedValue = quadraticLinearBasis[p];
						if (p == 0)
							expectedValue += quadraticLinearBasis[3]*scaleFactors[0]*scaleFactors[1];
						else if (p == 5)
							expectedValue *= scaleFactors[2];
						//std::cerr << "c " << c << " p " << p << std::endl;
						EXPECT_NEAR(expectedValue, outDerivativeParameters1[v++], TOL);
					}
				}
				else
				{
					for (int p = 0; p < 6; ++p)
						EXPECT_DOUBLE_EQ(0.0, outDerivativeParameters1[v++]);
				}

		const double quadraticLinearBasisDerivative1[2][6] =
		{
			{
				phi1[1][0]*phi2[0][0], phi1[1][1]*phi2[0][0], phi1[1][2]*phi2[0][0],
				phi1[1][0]*phi2[0][1], phi1[1][1]*phi2[0][1], phi1[1][2]*phi2[0][1]
			},
			{
				phi1[0][0]*phi2[1][0], phi1[0][1]*phi2[1][0], phi1[0][2]*phi2[1][0],
				phi1[0][0]*phi2[1][1], phi1[0][1]*phi2[1][1], phi1[0][2]*phi2[1][1]
			}
		};
		v = 0;
		for (int c = 0; c < componentCount; ++c)
			for (int i = 0; i < 2; ++i)
				for (int d = 0; d < componentCount; ++d)
					if (d == c)
					{
						for (int p = 0; p < 6; ++p)
						{
							double expectedValue = quadraticLinearBasisDerivative1[i][p];
							if (p == 0)
								expectedValue += quadraticLinearBasisDerivative1[i][3]*scaleFactors[0]*scaleFactors[1];
							else if (p == 5)
								expectedValue *= scaleFactors[2];
							//std::cerr << "c " << c << " i " << i << " p " << p << std::endl;
							EXPECT_NEAR(expectedValue, outDerivativesMesh1Parameters1[v++], TOL);
						}
					}
					else
					{
						for (int p = 0; p < 6; ++p)
							EXPECT_DOUBLE_EQ(0.0, outDerivativesMesh1Parameters1[v++]);
					}

		const double quadraticLinearBasisDerivative2[2][2][6] =
		{
			{
				{
					phi1[2][0]*phi2[0][0], phi1[2][1]*phi2[0][0], phi1[2][2]*phi2[0][0],
					phi1[2][0]*phi2[0][1], phi1[2][1]*phi2[0][1], phi1[2][2]*phi2[0][1]
				},
				{
					phi1[1][0]*phi2[1][0], phi1[1][1]*phi2[1][0], phi1[1][2]*phi2[1][0],
					phi1[1][0]*phi2[1][1], phi1[1][1]*phi2[1][1], phi1[1][2]*phi2[1][1]
				}
			},
			{
				{
					phi1[1][0]*phi2[1][0], phi1[1][1]*phi2[1][0], phi1[1][2]*phi2[1][0],
					phi1[1][0]*phi2[1][1], phi1[1][1]*phi2[1][1], phi1[1][2]*phi2[1][1]
				},
				{
					phi1[0][0]*phi2[2][0], phi1[0][1]*phi2[2][0], phi1[0][2]*phi2[2][0],
					phi1[0][0]*phi2[2][1], phi1[0][1]*phi2[2][1], phi1[0][2]*phi2[2][1]
				}
			}
		};
		v = 0;
		for (int c = 0; c < componentCount; ++c)
			for (int i = 0; i < 2; ++i)
				for (int j = 0; j < 2; ++j)
					for (int d = 0; d < componentCount; ++d)
						if (d == c)
						{
							for (int p = 0; p < 6; ++p)
							{
								double expectedValue = quadraticLinearBasisDerivative2[i][j][p];
								if (p == 0)
									expectedValue += quadraticLinearBasisDerivative2[i][j][3]*scaleFactors[0]*scaleFactors[1];
								else if (p == 5)
									expectedValue *= scaleFactors[2];
								//std::cerr << "c " << c << " i " << i << " j " << j << " p " << p << std::endl;
								EXPECT_NEAR(expectedValue, outDerivativesMesh2Parameters1[v++], TOL);
							}
						}
						else
						{
							for (int p = 0; p < 6; ++p)
								EXPECT_DOUBLE_EQ(0.0, outDerivativesMesh2Parameters1[v++]);
						}
	}
}