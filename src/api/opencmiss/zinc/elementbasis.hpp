/**
 * @file elementbasis.hpp
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CMZN_ELEMENTBASIS_HPP__
#define CMZN_ELEMENTBASIS_HPP__

#include "opencmiss/zinc/elementbasis.h"

namespace OpenCMISS
{
namespace Zinc
{

class Elementbasis
{
private:

	cmzn_elementbasis_id id;

public:

	Elementbasis() : id(0)
	{ }

	// takes ownership of C handle, responsibility for destroying it
	explicit Elementbasis(cmzn_elementbasis_id element_basis_id) :
		id(element_basis_id)
	{ }

	Elementbasis(const Elementbasis& elementBasis) :
		id(cmzn_elementbasis_access(elementBasis.id))
	{ }

	Elementbasis& operator=(const Elementbasis& elementBasis)
	{
		cmzn_elementbasis_id temp_id = cmzn_elementbasis_access(elementBasis.id);
		if (0 != id)
		{
			cmzn_elementbasis_destroy(&id);
		}
		id = temp_id;
		return *this;
	}

	~Elementbasis()
	{
		if (0 != id)
		{
			cmzn_elementbasis_destroy(&id);
		}
	}

	bool isValid() const
	{
		return (0 != id);
	}

	enum FunctionType
	{
		FUNCTION_TYPE_INVALID = CMZN_ELEMENTBASIS_FUNCTION_TYPE_INVALID,
		FUNCTION_TYPE_CONSTANT = CMZN_ELEMENTBASIS_FUNCTION_TYPE_CONSTANT,
		FUNCTION_TYPE_LINEAR_LAGRANGE = CMZN_ELEMENTBASIS_FUNCTION_TYPE_LINEAR_LAGRANGE,
		FUNCTION_TYPE_QUADRATIC_LAGRANGE = CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_LAGRANGE,
		FUNCTION_TYPE_CUBIC_LAGRANGE = CMZN_ELEMENTBASIS_FUNCTION_TYPE_CUBIC_LAGRANGE,
		FUNCTION_TYPE_LINEAR_SIMPLEX = CMZN_ELEMENTBASIS_FUNCTION_TYPE_LINEAR_SIMPLEX,   /**< linked on 2 or more dimensions */
		FUNCTION_TYPE_QUADRATIC_SIMPLEX = CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_SIMPLEX, /**< linked on 2 or more dimensions */
		FUNCTION_TYPE_CUBIC_HERMITE = CMZN_ELEMENTBASIS_FUNCTION_TYPE_CUBIC_HERMITE,
		FUNCTION_TYPE_CUBIC_HERMITE_SERENDIPITY = CMZN_ELEMENTBASIS_FUNCTION_TYPE_CUBIC_HERMITE_SERENDIPITY,
		FUNCTION_TYPE_QUADRATIC_HERMITE_LAGRANGE = CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_HERMITE_LAGRANGE,
		FUNCTION_TYPE_QUADRATIC_LAGRANGE_HERMITE = CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_LAGRANGE_HERMITE
	};

	cmzn_elementbasis_id getId() const
	{
		return id;
	}

	int getDimension()
	{
		return cmzn_elementbasis_get_dimension(id);
	}

	enum FunctionType getFunctionType(int chartComponent)
	{
		return static_cast<FunctionType>(cmzn_elementbasis_get_function_type(id, chartComponent));
	}

	int setFunctionType(int chartComponent, FunctionType functionType)
	{
		return cmzn_elementbasis_set_function_type(id, chartComponent,
			static_cast<cmzn_elementbasis_function_type>(functionType));
	}

	int getNumberOfNodes()
	{
		return cmzn_elementbasis_get_number_of_nodes(id);
	}

	int getNumberOfFunctions()
	{
		return cmzn_elementbasis_get_number_of_functions(id);
	}

	int getNumberOfFunctionsPerNode(int nodeNumber)
	{
		return cmzn_elementbasis_get_number_of_functions_per_node(id, nodeNumber);
	}

};

}  // namespace Zinc
}

#endif /* CMZN_ELEMENTBASIS_HPP__ */
