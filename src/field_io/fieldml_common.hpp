/**
 * FILE : fieldml_common.hpp
 * 
 * Common data and classes for FieldML 0.5 I/O.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (CMZN_FIELDML_COMMON_HPP)
#define CMZN_FIELDML_COMMON_HPP

#include "opencmiss/zinc/types/elementid.h"
#include "opencmiss/zinc/types/elementbasisid.h"
#include "fieldml_api.h"
#include "datastore/labels.hpp"
#include "datastore/map.hpp"
#include "datastore/mapindexing.hpp"
#include <vector>

const FmlObjectHandle FML_INVALID_OBJECT_HANDLE = (const FmlObjectHandle)FML_INVALID_HANDLE;

struct FE_basis;

struct ShapeType
{
	const char *fieldmlName;
	const int dimension;
	enum cmzn_element_shape_type shape_type;
};

const ShapeType libraryShapes[] =
{
	{ "shape.unit.line",        1, CMZN_ELEMENT_SHAPE_TYPE_LINE },
	{ "shape.unit.square",      2, CMZN_ELEMENT_SHAPE_TYPE_SQUARE },
	{ "shape.unit.triangle",    2, CMZN_ELEMENT_SHAPE_TYPE_TRIANGLE },
	{ "shape.unit.cube",        3, CMZN_ELEMENT_SHAPE_TYPE_CUBE },
	{ "shape.unit.tetrahedron", 3, CMZN_ELEMENT_SHAPE_TYPE_TETRAHEDRON },
	{ "shape.unit.wedge12",     3, CMZN_ELEMENT_SHAPE_TYPE_WEDGE12 },
	{ "shape.unit.wedge13",     3, CMZN_ELEMENT_SHAPE_TYPE_WEDGE13 },
	{ "shape.unit.wedge23",     3, CMZN_ELEMENT_SHAPE_TYPE_WEDGE23 }
};
const int numLibraryShapes = sizeof(libraryShapes) / sizeof(ShapeType);

// swizzle array gives the source parameter index (1-based) for the zinc basis parameter index (0-based)
//const int biquadraticSimplex_vtk_swizzle[6] = { 1, 3, 6, 2, 5, 4 };
const int zinc_to_biquadraticSimplex_vtk_swizzle[6] = { 1, 4, 2, 6, 5, 3 }; // same for Zienkiewicz
const int zinc_to_bicubicSimplex_zienkiewicz_swizzle[10] = { 1, 4, 5, 2, 9, 10, 6, 8, 7, 3 };
//const int triquadraticSimplex_zienkiewicz_swizzle[10] = { 1, 3, 6, 10, 2, 4, 7, 5, 9, 8 };
const int zinc_to_triquadraticSimplex_zienkiewicz_swizzle[10] = { 1, 5, 2, 6, 8, 3, 7, 10, 9, 4 };
const int zinc_to_tricubicSimplex_zienkiewicz_swizzle[20] = { 1, 5, 6, 2, 7, 17, 11, 8, 12, 3, 9, 18, 15, 19, 20, 13, 10, 16, 14, 4 };

struct BasisType
{
	int dimension;
	const char *fieldmlBasisEvaluatorName;
	bool homogeneous;
	enum cmzn_elementbasis_function_type functionType[3];
	const int *zinc_to_source_parameter_swizzle;
};

const BasisType libraryBases[] =
{
	{ 1, "interpolator.1d.unit.linearLagrange",      true, { CMZN_ELEMENTBASIS_FUNCTION_TYPE_LINEAR_LAGRANGE, CMZN_ELEMENTBASIS_FUNCTION_TYPE_INVALID, CMZN_ELEMENTBASIS_FUNCTION_TYPE_INVALID }, 0 },
	{ 1, "interpolator.1d.unit.quadraticLagrange",   true, { CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_LAGRANGE, CMZN_ELEMENTBASIS_FUNCTION_TYPE_INVALID, CMZN_ELEMENTBASIS_FUNCTION_TYPE_INVALID }, 0 },
	{ 1, "interpolator.1d.unit.cubicLagrange",       true, { CMZN_ELEMENTBASIS_FUNCTION_TYPE_CUBIC_LAGRANGE, CMZN_ELEMENTBASIS_FUNCTION_TYPE_INVALID, CMZN_ELEMENTBASIS_FUNCTION_TYPE_INVALID }, 0 },
	{ 1, "interpolator.1d.unit.cubicHermite",        true, { CMZN_ELEMENTBASIS_FUNCTION_TYPE_CUBIC_HERMITE, CMZN_ELEMENTBASIS_FUNCTION_TYPE_INVALID, CMZN_ELEMENTBASIS_FUNCTION_TYPE_INVALID }, 0 },
	{ 2, "interpolator.2d.unit.bilinearLagrange",    true, { CMZN_ELEMENTBASIS_FUNCTION_TYPE_LINEAR_LAGRANGE, CMZN_ELEMENTBASIS_FUNCTION_TYPE_LINEAR_LAGRANGE, CMZN_ELEMENTBASIS_FUNCTION_TYPE_INVALID }, 0 },
	{ 2, "interpolator.2d.unit.biquadraticLagrange", true, { CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_LAGRANGE, CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_LAGRANGE, CMZN_ELEMENTBASIS_FUNCTION_TYPE_INVALID }, 0 },
	{ 2, "interpolator.2d.unit.bicubicLagrange",     true, { CMZN_ELEMENTBASIS_FUNCTION_TYPE_CUBIC_LAGRANGE, CMZN_ELEMENTBASIS_FUNCTION_TYPE_CUBIC_LAGRANGE, CMZN_ELEMENTBASIS_FUNCTION_TYPE_INVALID }, 0 },
	{ 2, "interpolator.2d.unit.bilinearSimplex",     true, { CMZN_ELEMENTBASIS_FUNCTION_TYPE_LINEAR_SIMPLEX, CMZN_ELEMENTBASIS_FUNCTION_TYPE_LINEAR_SIMPLEX, CMZN_ELEMENTBASIS_FUNCTION_TYPE_INVALID }, 0 },
	{ 2, "interpolator.2d.unit.biquadraticSimplex",  true, { CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_SIMPLEX, CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_SIMPLEX, CMZN_ELEMENTBASIS_FUNCTION_TYPE_INVALID }, 0 },
	{ 2, "interpolator.2d.unit.biquadraticSimplex.vtk", true, { CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_SIMPLEX, CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_SIMPLEX, CMZN_ELEMENTBASIS_FUNCTION_TYPE_INVALID }, zinc_to_biquadraticSimplex_vtk_swizzle },
	{ 2, "interpolator.2d.unit.bicubicHermite",      true, { CMZN_ELEMENTBASIS_FUNCTION_TYPE_CUBIC_HERMITE, CMZN_ELEMENTBASIS_FUNCTION_TYPE_CUBIC_HERMITE, CMZN_ELEMENTBASIS_FUNCTION_TYPE_INVALID }, 0 },
	{ 3, "interpolator.3d.unit.trilinearLagrange",   true, { CMZN_ELEMENTBASIS_FUNCTION_TYPE_LINEAR_LAGRANGE, CMZN_ELEMENTBASIS_FUNCTION_TYPE_LINEAR_LAGRANGE, CMZN_ELEMENTBASIS_FUNCTION_TYPE_LINEAR_LAGRANGE }, 0 },
	{ 3, "interpolator.3d.unit.triquadraticLagrange",true, { CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_LAGRANGE, CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_LAGRANGE, CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_LAGRANGE }, 0 },
	{ 3, "interpolator.3d.unit.tricubicLagrange",    true, { CMZN_ELEMENTBASIS_FUNCTION_TYPE_CUBIC_LAGRANGE, CMZN_ELEMENTBASIS_FUNCTION_TYPE_CUBIC_LAGRANGE, CMZN_ELEMENTBASIS_FUNCTION_TYPE_CUBIC_LAGRANGE }, 0 },
	{ 3, "interpolator.3d.unit.trilinearSimplex",    true, { CMZN_ELEMENTBASIS_FUNCTION_TYPE_LINEAR_SIMPLEX, CMZN_ELEMENTBASIS_FUNCTION_TYPE_LINEAR_SIMPLEX, CMZN_ELEMENTBASIS_FUNCTION_TYPE_LINEAR_SIMPLEX }, 0 },
	{ 3, "interpolator.3d.unit.triquadraticSimplex", true, { CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_SIMPLEX, CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_SIMPLEX, CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_SIMPLEX }, 0 },
	{ 3, "interpolator.3d.unit.triquadraticSimplex.zienkiewicz", true, { CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_SIMPLEX, CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_SIMPLEX, CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_SIMPLEX }, zinc_to_triquadraticSimplex_zienkiewicz_swizzle },
	{ 3, "interpolator.3d.unit.trilinearWedge12",    false,{ CMZN_ELEMENTBASIS_FUNCTION_TYPE_LINEAR_SIMPLEX, CMZN_ELEMENTBASIS_FUNCTION_TYPE_LINEAR_SIMPLEX, CMZN_ELEMENTBASIS_FUNCTION_TYPE_LINEAR_LAGRANGE }, 0 },
	{ 3, "interpolator.3d.unit.triquadraticWedge12", false,{ CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_SIMPLEX, CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_SIMPLEX, CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_LAGRANGE }, 0 },
	{ 3, "interpolator.3d.unit.tricubicHermite",     true, { CMZN_ELEMENTBASIS_FUNCTION_TYPE_CUBIC_HERMITE, CMZN_ELEMENTBASIS_FUNCTION_TYPE_CUBIC_HERMITE, CMZN_ELEMENTBASIS_FUNCTION_TYPE_CUBIC_HERMITE }, 0 }
};

const int numLibraryBases = sizeof(libraryBases) / sizeof(BasisType);

enum cmzn_element_shape_type getElementShapeFromFieldmlName(const char *shapeName);

const char *getFieldmlNameFromElementShape(enum cmzn_element_shape_type shapeType);

/**
 * @return  True if the file_name has an extension expected for FieldML,
 * e.g. .fieldml.
 */
bool filename_has_FieldML_extension(const char *filename);

#endif /* !defined (CMZN_FIELDML_COMMON_HPP) */
