/**
 * @file element.hpp
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CMZN_ELEMENT_HPP__
#define CMZN_ELEMENT_HPP__

#include "opencmiss/zinc/element.h"
#include "opencmiss/zinc/elementfieldtemplate.hpp"
#include "opencmiss/zinc/field.hpp"
#include "opencmiss/zinc/differentialoperator.hpp"
#include "opencmiss/zinc/node.hpp"

namespace OpenCMISS
{
namespace Zinc
{

class Fieldmodule;
class Mesh;
class Elementtemplate;

class Element
{
private:

	cmzn_element_id id;

public:

	Element() : id(0)
	{ }

	// takes ownership of C handle, responsibility for destroying it
	explicit Element(cmzn_element_id element_id) :
		id(element_id)
	{ }

	Element(const Element& element) :
		id(cmzn_element_access(element.id))
	{ }

	~Element()
	{
		if (0 != id)
		{
			cmzn_element_destroy(&id);
		}
	}

	Element& operator=(const Element& element)
	{
		cmzn_element_id temp_id = cmzn_element_access(element.id);
		if (0 != id)
		{
			cmzn_element_destroy(&id);
		}
		id = temp_id;
		return *this;
	}

	bool isValid() const
	{
		return (0 != id);
	}

	/**
	 * Element change bit flags returned by Meshchanges query functions.
	 */
	enum ChangeFlag
	{
		CHANGE_FLAG_NONE = CMZN_ELEMENT_CHANGE_FLAG_NONE,
		CHANGE_FLAG_ADD = CMZN_ELEMENT_CHANGE_FLAG_ADD,
		CHANGE_FLAG_REMOVE = CMZN_ELEMENT_CHANGE_FLAG_REMOVE,
		CHANGE_FLAG_IDENTIFIER = CMZN_ELEMENT_CHANGE_FLAG_IDENTIFIER,
		CHANGE_FLAG_DEFINITION = CMZN_ELEMENT_CHANGE_FLAG_DEFINITION,
		CHANGE_FLAG_FIELD = CMZN_ELEMENT_CHANGE_FLAG_FIELD
	};
	
	/**
	 * Type for passing logical OR of #ChangeFlag
	 */
	typedef int ChangeFlags;

	enum FaceType
	{
		FACE_TYPE_INVALID = CMZN_ELEMENT_FACE_TYPE_INVALID,
		FACE_TYPE_ALL = CMZN_ELEMENT_FACE_TYPE_ALL,
		FACE_TYPE_ANY_FACE = CMZN_ELEMENT_FACE_TYPE_ANY_FACE,
		FACE_TYPE_NO_FACE = CMZN_ELEMENT_FACE_TYPE_NO_FACE,
		FACE_TYPE_XI1_0 = CMZN_ELEMENT_FACE_TYPE_XI1_0,
		FACE_TYPE_XI1_1 = CMZN_ELEMENT_FACE_TYPE_XI1_1,
		FACE_TYPE_XI2_0 = CMZN_ELEMENT_FACE_TYPE_XI2_0,
		FACE_TYPE_XI2_1 = CMZN_ELEMENT_FACE_TYPE_XI2_1,
		FACE_TYPE_XI3_0 = CMZN_ELEMENT_FACE_TYPE_XI3_0,
		FACE_TYPE_XI3_1 = CMZN_ELEMENT_FACE_TYPE_XI3_1
	};

	enum ShapeType
	{
		SHAPE_TYPE_INVALID = CMZN_ELEMENT_SHAPE_TYPE_INVALID,        /**< unspecified shape of known dimension */
		SHAPE_TYPE_LINE = CMZN_ELEMENT_SHAPE_TYPE_LINE,              /**< 1-D: 0 <= xi1 <= 1 */
		SHAPE_TYPE_SQUARE = CMZN_ELEMENT_SHAPE_TYPE_SQUARE,          /**< 2-D: 0 <= xi1,xi2 <= 1 */
		SHAPE_TYPE_TRIANGLE = CMZN_ELEMENT_SHAPE_TYPE_TRIANGLE,      /**< 3-D: 0 <= xi1,xi2; xi1+xi2 <= 1 */
		SHAPE_TYPE_CUBE = CMZN_ELEMENT_SHAPE_TYPE_CUBE,              /**< 3-D: 0 <= xi1,xi2,xi3 <= 1 */
		SHAPE_TYPE_TETRAHEDRON = CMZN_ELEMENT_SHAPE_TYPE_TETRAHEDRON,/**< 3-D: 0 <= xi1,xi2,xi3; xi1+xi2+xi3 <= 1 */
		SHAPE_TYPE_WEDGE12 = CMZN_ELEMENT_SHAPE_TYPE_WEDGE12,        /**< 3-D: 0 <= xi1,xi2; xi1+xi2 <= 1; 0 <= xi3 <= 1 */
		SHAPE_TYPE_WEDGE13 = CMZN_ELEMENT_SHAPE_TYPE_WEDGE13,        /**< 3-D: 0 <= xi1,xi3; xi1+xi3 <= 1; 0 <= xi2 <= 1 */
		SHAPE_TYPE_WEDGE23 = CMZN_ELEMENT_SHAPE_TYPE_WEDGE23         /**< 3-D: 0 <= xi2,xi3; xi2+xi3 <= 1; 0 <= xi1 <= 1 */
	};

	enum PointSamplingMode
	{
		POINT_SAMPLING_MODE_INVALID = CMZN_ELEMENT_POINT_SAMPLING_MODE_INVALID,
		POINT_SAMPLING_MODE_CELL_CENTRES = CMZN_ELEMENT_POINT_SAMPLING_MODE_CELL_CENTRES,
		POINT_SAMPLING_MODE_CELL_CORNERS = CMZN_ELEMENT_POINT_SAMPLING_MODE_CELL_CORNERS,
		POINT_SAMPLING_MODE_CELL_POISSON = CMZN_ELEMENT_POINT_SAMPLING_MODE_CELL_POISSON,
		POINT_SAMPLING_MODE_SET_LOCATION = CMZN_ELEMENT_POINT_SAMPLING_MODE_SET_LOCATION,
		POINT_SAMPLING_MODE_GAUSSIAN_QUADRATURE = CMZN_ELEMENT_POINT_SAMPLING_MODE_GAUSSIAN_QUADRATURE
	};

	enum QuadratureRule
	{
		QUADRATURE_RULE_INVALID = CMZN_ELEMENT_QUADRATURE_RULE_INVALID,
		QUADRATURE_RULE_GAUSSIAN = CMZN_ELEMENT_QUADRATURE_RULE_GAUSSIAN,
		QUADRATURE_RULE_MIDPOINT = CMZN_ELEMENT_QUADRATURE_RULE_MIDPOINT
	};

	cmzn_element_id getId() const
	{
		return id;
	}

	int getDimension()
	{
		return cmzn_element_get_dimension(id);
	}

	Elementfieldtemplate getElementfieldtemplate(const Field& field, int componentNumber) const
	{
		return Elementfieldtemplate(cmzn_element_get_elementfieldtemplate(this->id, field.getId(), componentNumber));
	}

	int getIdentifier()
	{
		return cmzn_element_get_identifier(id);
	}

	int setIdentifier(int identifier)
	{
		return cmzn_element_set_identifier(id, identifier);
	}

	inline Mesh getMesh() const;

	Node getNode(const Elementfieldtemplate &eft, int localNodeIndex)
	{
		return Node(cmzn_element_get_node(this->id, eft.getId(), localNodeIndex));
	}

	int setNode(const Elementfieldtemplate &eft, int localNodeIndex, const Node &node)
	{
		return cmzn_element_set_node(this->id, eft.getId(), localNodeIndex, node.getId());
	}

	int setNodesByIdentifier(const Elementfieldtemplate &eft, int identifiersCount,
		const int *identifiersIn)
	{
		return cmzn_element_set_nodes_by_identifier(this->id, eft.getId(), identifiersCount, identifiersIn);
	}

	int getScaleFactor(const Elementfieldtemplate &eft, int scaleFactorIndex, double *valueOut)
	{
		return cmzn_element_get_scale_factor(this->id, eft.getId(), scaleFactorIndex, valueOut);
	}

	int setScaleFactor(const Elementfieldtemplate &eft, int scaleFactorIndex, double value)
	{
		return cmzn_element_set_scale_factor(this->id, eft.getId(), scaleFactorIndex, value);
	}

	int getScaleFactors(const Elementfieldtemplate &eft, int valuesCount,
		double *valuesOut)
	{
		return cmzn_element_get_scale_factors(this->id, eft.getId(), valuesCount, valuesOut);
	}

	int setScaleFactors(const Elementfieldtemplate &eft, int valuesCount,
		const double *valuesIn)
	{
		return cmzn_element_set_scale_factors(this->id, eft.getId(), valuesCount, valuesIn);
	}

	enum ShapeType getShapeType()
	{
		return static_cast<ShapeType>(cmzn_element_get_shape_type(id));
	}

	inline int merge(const Elementtemplate& elementTemplate);

};

inline bool operator==(const Element& a, const Element& b)
{
	return a.getId() == b.getId();
}

class Elementiterator
{
private:

	cmzn_elementiterator_id id;

public:

	Elementiterator() : id(0)
	{ }

	// takes ownership of C handle, responsibility for destroying it
	explicit Elementiterator(cmzn_elementiterator_id element_iterator_id) :
		id(element_iterator_id)
	{ }

	Elementiterator(const Elementiterator& elementIterator) :
		id(cmzn_elementiterator_access(elementIterator.id))
	{ }

	Elementiterator& operator=(const Elementiterator& elementIterator)
	{
		cmzn_elementiterator_id temp_id = cmzn_elementiterator_access(elementIterator.id);
		if (0 != id)
		{
			cmzn_elementiterator_destroy(&id);
		}
		id = temp_id;
		return *this;
	}

	~Elementiterator()
	{
		if (0 != id)
		{
			cmzn_elementiterator_destroy(&id);
		}
	}

	bool isValid() const
	{
		return (0 != id);
	}

	Element next()
	{
		return Element(cmzn_elementiterator_next(id));
	}
};

}  // namespace Zinc
}

#endif /* CMZN_ELEMENT_HPP__ */
