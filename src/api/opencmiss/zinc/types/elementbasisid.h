/**
 * @file elementbasisid.h
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CMZN_ELEMENTBASISID_H__
#define CMZN_ELEMENTBASISID_H__

/**
 * @brief A set of basis functions that can apply over an element of a given dimension.
 *
 * A set of basis functions that can apply over an element of a given dimension.
 * The element basis can be a tensor product of different basis function types,
 * with the parameters of the resulting basis cycling over lower element 'xi'
 * coordinates fastest.
 */
struct cmzn_elementbasis;
typedef struct cmzn_elementbasis *cmzn_elementbasis_id;

/**
 * Common 1-D or linked-dimension basis function types.
 *
 * @see cmzn_elementbasis_id
 */
enum cmzn_elementbasis_function_type
{
	CMZN_ELEMENTBASIS_FUNCTION_TYPE_INVALID = 0,
	/*!< Invalid or unspecified basis function type */
	CMZN_ELEMENTBASIS_FUNCTION_TYPE_CONSTANT = 1,
	/*!< Constant value, 1 parameter */
	CMZN_ELEMENTBASIS_FUNCTION_TYPE_LINEAR_LAGRANGE = 2,
	/*!< Linear Lagrange interpolation over xi in [0,1] with 2 parameters
	     ordered in increasing xi: xi=0, xi=1 */
	CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_LAGRANGE = 3,
	/*!< Quadratic Lagrange interpolation over xi in [0,1] with 3 parameters
	     ordered in increasing xi: xi=0, xi=0.5, xi=1 */
	CMZN_ELEMENTBASIS_FUNCTION_TYPE_CUBIC_LAGRANGE = 4,
	/*!< Cubic Lagrange interpolation over xi in [0,1] with 4 parameters
	     ordered in increasing xi: xi=0, xi=1/3, xi=2/3, xi=1 */
	CMZN_ELEMENTBASIS_FUNCTION_TYPE_LINEAR_SIMPLEX = 5,
	/*!< Linear Lagrange simplex basis linked on 2 or more dimensions over
	     chart xi >= 0, and sum of linked xi coordinates <= 1.
	     2 linked dimensions gives a linear triangle with 3 parameters with
	     lowest xi cycling fastest: xi (0,0) (1,0) (0,1)
	     3 linked dimensions gives a linear tetrahedron with 4 parameters with
	     lowest xi cycling fastest: xi (0,0,0) (1,0,0) (0,1,0) (0,0,1) */
	CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_SIMPLEX = 6,
	/*!< Quadratic Lagrange simplex basis linked on 2 or more dimensions over
	     chart xi >= 0, and sum of linked xi coordinates <= 1.
	     2 linked dimensions gives a quadratic triangle with 6 parameters with
	     lowest xi cycling fastest: xi (0,0) (0.5,0) (1,0) (0,0.5) (0.5,0.5) (0,1)
	     3 linked dimensions gives a quadratic tetrahedron with 10 parameters with
	     lowest xi cycling fastest: xi (0,0,0) (0.5,0,0) (1,0,0) (0,0.5,0)
	     (0.5,0.5,0) (0,1,0) (0,0,0.5) (0.5,0,0.5) (0,0.5,0.5) (0,0,1) */
	CMZN_ELEMENTBASIS_FUNCTION_TYPE_CUBIC_HERMITE = 7,
	/*!< Cubic Hermite basis over xi in [0,1] with 4 parameters: x at xi=0,
	     dx/dxi at xi=0, x at xi=1. dx/dxi at xi=1. */
	CMZN_ELEMENTBASIS_FUNCTION_TYPE_CUBIC_HERMITE_SERENDIPITY = 8,
	/*!< Cubic Hermite serendipity basis linked on 2 or more dimensions over
	     square or cube element. Parameters are value and first derivatives. */
	CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_HERMITE_LAGRANGE = 9,
	/*!< Quadratic polynomial with Hermite value + derivative at xi=0 node
	     and Lagrange value at xi=1 node. */
	CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_LAGRANGE_HERMITE = 10
	/*!< Quadratic polynomial with Lagrange value at xi=0 node and Hermite
	     value + derivative at xi=1 node. */
};

#endif
