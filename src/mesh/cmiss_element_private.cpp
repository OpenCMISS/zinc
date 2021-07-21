/***************************************************************************//**
 * FILE : cmiss_element_private.cpp
 *
 * Implementation of public interface to cmzn_element, finite element meshes.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "opencmiss/zinc/elementbasis.h"
#include "opencmiss/zinc/elementtemplate.h"
#include "opencmiss/zinc/fieldfiniteelement.h"
#include "opencmiss/zinc/fieldsubobjectgroup.h"
#include "opencmiss/zinc/fieldmodule.h"
#include "opencmiss/zinc/mesh.h"
#include "opencmiss/zinc/status.h"
#include "element/element_operations.h"
#include "general/debug.h"
#include "general/mystring.h"
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_mesh.hpp"
#include "finite_element/finite_element_region.h"
#include "finite_element/finite_element_region_private.h"
#include "computed_field/computed_field_finite_element.h"
#include "computed_field/computed_field_subobject_group.hpp"
#include "computed_field/differential_operator.hpp"
#include "computed_field/field_module.hpp"
#include "general/enumerator_conversion.hpp"
#include "mesh/cmiss_element_private.hpp"
#include "mesh/cmiss_node_private.hpp"
#include <map>
#include <vector>

namespace {

inline int cmzn_element_shape_type_get_dimension(
	cmzn_element_shape_type shape_type)
{
	switch (shape_type)
	{
		case CMZN_ELEMENT_SHAPE_TYPE_LINE:
			return 1;
			break;
		case CMZN_ELEMENT_SHAPE_TYPE_SQUARE:
		case CMZN_ELEMENT_SHAPE_TYPE_TRIANGLE:
			return 2;
			break;
		case CMZN_ELEMENT_SHAPE_TYPE_CUBE:
		case CMZN_ELEMENT_SHAPE_TYPE_TETRAHEDRON:
		case CMZN_ELEMENT_SHAPE_TYPE_WEDGE12:
		case CMZN_ELEMENT_SHAPE_TYPE_WEDGE13:
		case CMZN_ELEMENT_SHAPE_TYPE_WEDGE23:
			return 3;
			break;
		default:
			// do nothing
			break;
	}
	return 0;
}

}

/*
Global types
------------
*/

/*============================================================================*/

cmzn_elementbasis::cmzn_elementbasis(FE_region *fe_region, int mesh_dimension,
		cmzn_elementbasis_function_type function_type) :
	fe_region(ACCESS(FE_region)(fe_region)),
	dimension(mesh_dimension),
	function_types(new cmzn_elementbasis_function_type[mesh_dimension]),
	access_count(1)
{
	for (int i = 0; i < dimension; i++)
	{
		function_types[i] = function_type;
	}
}

cmzn_elementbasis::~cmzn_elementbasis()
{
	DEACCESS(FE_region)(&fe_region);
	delete[] function_types;
}

cmzn_elementbasis* cmzn_elementbasis::create(FE_region *fe_region, int mesh_dimension,
	cmzn_elementbasis_function_type function_type)
{
	if ((fe_region) && (0 < mesh_dimension) && (mesh_dimension <= MAXIMUM_ELEMENT_XI_DIMENSIONS) &&
		(function_type > CMZN_ELEMENTBASIS_FUNCTION_TYPE_INVALID))
	{
		return new cmzn_elementbasis(fe_region, mesh_dimension, function_type);
	}
	display_message(ERROR_MESSAGE, "Fieldmodule createElementbasis.  Invalid argument(s)");
	return 0;
}

cmzn_elementbasis* cmzn_elementbasis::create(FE_region *fe_region, FE_basis *basis)
{
	if ((fe_region) && (basis))
	{
		int dimension;
		FE_basis_get_dimension(basis, &dimension);
		FE_basis_type feBasisType;
		FE_basis_get_xi_basis_type(basis, /*xi*/0, &feBasisType);
		cmzn_elementbasis_function_type functionType = FE_basis_type_to_cmzn_elementbasis_function_type(feBasisType);
		if (functionType == CMZN_ELEMENTBASIS_FUNCTION_TYPE_INVALID)
		{
			display_message(ERROR_MESSAGE, "cmzn_elementbasis::create.  Internal basis function type not implemented externally");
			return 0;
		}
		cmzn_elementbasis *elementbasis = new cmzn_elementbasis(fe_region, dimension, functionType);
		if (elementbasis)
		{
			for (int d = 1; d < dimension; ++d)
			{
				FE_basis_type feBasisType2;
				FE_basis_get_xi_basis_type(basis, d, &feBasisType2);
				if (feBasisType2 != feBasisType)
				{
					functionType = FE_basis_type_to_cmzn_elementbasis_function_type(feBasisType2);
					if (functionType == CMZN_ELEMENTBASIS_FUNCTION_TYPE_INVALID)
					{
						display_message(ERROR_MESSAGE, "cmzn_elementbasis::create.  Internal basis function type not implemented externally");
						cmzn_elementbasis::deaccess(elementbasis);
						return 0;
					}
					elementbasis->setFunctionType(d + 1, functionType);
				}
			}
			return elementbasis;
		}
	}
	return 0;
}

int cmzn_elementbasis::getDimensionsUsingFunction(cmzn_elementbasis_function_type function_type) const
{
	int count = 0;
	for (int i = 0; i < dimension; i++)
	{
		if (function_types[i] == function_type)
			count++;
	}
	return count;
}

int cmzn_elementbasis::isValid() const
{
	int return_code = 1;
	if (0 < getDimensionsUsingFunction(CMZN_ELEMENTBASIS_FUNCTION_TYPE_INVALID))
	{
		display_message(ERROR_MESSAGE,
			"cmzn_elementbasis::isValid.  Function type not set");
		return_code = 0;
	}
	if ((1 == getDimensionsUsingFunction(CMZN_ELEMENTBASIS_FUNCTION_TYPE_LINEAR_SIMPLEX)) ||
		(1 == getDimensionsUsingFunction(CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_SIMPLEX)))
	{
		display_message(ERROR_MESSAGE, "cmzn_elementbasis::isValid.  "
			"Must be at least 2 linked dimension for simplex basis");
		return_code = 0;
	}
	return return_code;
}

/** @return  Accessed FE_basis, or NULL on error */
FE_basis *cmzn_elementbasis::getFeBasis() const
{
	if (!isValid())
		return 0;
	const int length = dimension*(dimension + 1)/2 + 1;
	int *int_basis_type_array;
	if (!ALLOCATE(int_basis_type_array, int, length))
		return 0;
	*int_basis_type_array = dimension;
	int *temp = int_basis_type_array + 1;
	for (int i = 0; i < dimension; i++)
	{
		FE_basis_type fe_basis_type = cmzn_elementbasis_function_type_to_FE_basis_type(function_types[i]);
		*temp = (int)fe_basis_type;
		++temp;
		for (int j = i + 1; j < dimension; j++)
		{
			if (((fe_basis_type == CUBIC_HERMITE_SERENDIPITY) ||
				(fe_basis_type == LINEAR_SIMPLEX) ||
				(fe_basis_type == QUADRATIC_SIMPLEX)) &&
				(function_types[j] == function_types[i]))
			{
				*temp = 1;
			}
			else
			{
				*temp = 0; // NO_RELATION
			}
			++temp;
		}
	}
	struct FE_basis *fe_basis = FE_region_get_FE_basis_matching_basis_type(
		fe_region, int_basis_type_array);
	DEALLOCATE(int_basis_type_array);
	return fe_basis;
}

enum cmzn_elementbasis_function_type cmzn_elementbasis::getFunctionType(int chart_component) const
{
	if (chart_component == -1) // homogeneous case
	{
		for (int d = 1; d < this->dimension; ++d)
		{
			if (function_types[d] != function_types[d - 1])
				return CMZN_ELEMENTBASIS_FUNCTION_TYPE_INVALID;
		}
		return function_types[0];
	}
	if ((chart_component < 1) || (chart_component > this->dimension))
		return CMZN_ELEMENTBASIS_FUNCTION_TYPE_INVALID;
	return function_types[chart_component - 1];
}

int cmzn_elementbasis::setFunctionType(int chart_component, cmzn_elementbasis_function_type function_type)
{
	if (FE_BASIS_TYPE_INVALID == cmzn_elementbasis_function_type_to_FE_basis_type(function_type))
		return CMZN_ERROR_ARGUMENT;
	if (chart_component == -1) // homogeneous case
	{
		for (int d = 0; d < this->dimension; ++d)
			function_types[d] = function_type;
	}
	else
	{
		if ((chart_component < 1) || (chart_component > dimension))
			return CMZN_ERROR_ARGUMENT;
		function_types[chart_component - 1] = function_type;
	}
	return CMZN_OK;
}

int cmzn_elementbasis::getNumberOfNodes() const
{
	FE_basis *basis = this->getFeBasis();
	int numberOfNodes = FE_basis_get_number_of_nodes(basis);
	return numberOfNodes;
}

int cmzn_elementbasis::getNumberOfFunctions() const
{
	FE_basis *basis = this->getFeBasis();
	int numberOfFunctions = FE_basis_get_number_of_functions(basis);
	return numberOfFunctions;
}

int cmzn_elementbasis::getNumberOfFunctionsPerNode(int basisNodeIndex) const
{
	FE_basis *basis = this->getFeBasis();
	int numberOfFunctionsPerNode = FE_basis_get_number_of_functions_per_node(basis, basisNodeIndex - 1);
	return numberOfFunctionsPerNode;
}

/*============================================================================*/

// stores map of EFT local node to legacy element nodes, per-component
class LegacyElementFieldData
{
public:
	class NodeMap
	{
		int nodeIndexesCount;
		int *nodeIndexes;

	public:
		NodeMap(int nodeIndexesCountIn, const int *nodeIndexesIn) :
			nodeIndexesCount(nodeIndexesCountIn),
			nodeIndexes(new int[nodeIndexesCountIn])
		{
			memcpy(this->nodeIndexes, nodeIndexesIn, nodeIndexesCountIn*sizeof(int));
		}

		~NodeMap()
		{
			delete[] this->nodeIndexes;
		}

		int getNodeIndexesCount() const
		{
			return this->nodeIndexesCount;
		}

		const int *getNodeIndexes() const
		{
			return this->nodeIndexes;
		}
	};

private:

	FE_field *fe_field;
	const int componentCount;
	NodeMap **componentNodeMaps;

	/** Handles sharing of same node map for multiple components.
	  * @param componentNumber  Starting at 0, or negative to clear all components. Not checked. */
	void clearComponentNodeMaps(int componentNumber)
	{
		if (componentNumber < 0)
		{
			for (int i = 0; i < this->componentCount; i++)
			{
				// handle sharing by multiple components
				for (int j = this->componentCount - 1; i < j; --j)
					if (this->componentNodeMaps[j] == this->componentNodeMaps[i])
						this->componentNodeMaps[j] = 0;
				delete this->componentNodeMaps[i];
				this->componentNodeMaps[i] = 0;
			}
		}
		else
		{
			bool unshared = true;
			for (int i = 0; i < this->componentCount; i++)
			{
				if ((i != componentNumber) && (this->componentNodeMaps[i] == this->componentNodeMaps[componentNumber]))
				{
					unshared = false;
					break;
				}
			}
			if (unshared)
				delete[] this->componentNodeMaps[componentNumber];
			this->componentNodeMaps[componentNumber] = 0;
		}
	}

public:

	LegacyElementFieldData(FE_field *fe_field) :
		fe_field(ACCESS(FE_field)(fe_field)),
		componentCount(get_FE_field_number_of_components(fe_field)),
		componentNodeMaps(new NodeMap*[componentCount])
	{
		for (int i = 0; i < this->componentCount; i++)
			componentNodeMaps[i] = 0;
	}

	~LegacyElementFieldData()
	{
		DEACCESS(FE_field)(&fe_field);
		this->clearComponentNodeMaps(/*componentNumber*/-1);
		delete[] this->componentNodeMaps;
	}

	FE_field *getField() const
	{
		return this->fe_field;
	}

	int getComponentCount() const
	{
		return this->componentCount;
	}

	/** @param componentNumber  Starting at 0, or negative to set for all components. Not checked.
	  * @param nodeIndexesCount  Size of nodeIndexes. Must equal number of nodes expected by EFT. */
	int setNodeMap(int componentNumber, int nodeIndexesCount, const int *nodeIndexes)
	{
		NodeMap *nodeMap = new NodeMap(nodeIndexesCount, nodeIndexes);
		if (!nodeMap)
			return CMZN_ERROR_MEMORY;
		clearComponentNodeMaps(componentNumber);
		if (componentNumber >= 0)
		{
			this->componentNodeMaps[componentNumber] = nodeMap;
		}
		else
		{
			for (int i = 0; i < this->componentCount; ++i)
				this->componentNodeMaps[i] = nodeMap;
		}
		return CMZN_OK;
	}

	/** @param componentNumber  From 0 to count - 1, not checked */
	const NodeMap *getComponentNodeMap(int componentNumber) const
	{
		return this->componentNodeMaps[componentNumber];
	}

};


/*============================================================================*/

cmzn_elementtemplate::cmzn_elementtemplate(FE_mesh *fe_mesh_in) :
	fe_element_template(fe_mesh_in->create_FE_element_template()),
	legacyNodesCount(0),
	legacyNodes(0),
	access_count(1)
{
}

cmzn_elementtemplate::~cmzn_elementtemplate()
{
	const size_t legacyFieldDataCount = this->legacyFieldDataList.size();
	for (size_t i = 0; i < legacyFieldDataCount; ++i)
		delete this->legacyFieldDataList[i];
	this->clearLegacyNodes();
	cmzn::Deaccess(this->fe_element_template);
}

void cmzn_elementtemplate::clearLegacyNodes()
{
	if (this->legacyNodes)
	{
		for (int i = 0; i < this->legacyNodesCount; ++i)
			cmzn_node_destroy(&(this->legacyNodes[i]));
		delete[] this->legacyNodes;
		this->legacyNodes = 0;
	}
}

void cmzn_elementtemplate::clearLegacyElementFieldData(FE_field *fe_field)
{
	for (std::vector<LegacyElementFieldData*>::iterator iter = this->legacyFieldDataList.begin();
		iter != this->legacyFieldDataList.end(); ++iter)
	{
		if ((*iter)->getField() == fe_field)
		{
			delete *iter;
			this->legacyFieldDataList.erase(iter);
			break;
		}
	}
}

LegacyElementFieldData *cmzn_elementtemplate::getLegacyElementFieldData(FE_field *fe_field)
{
	const size_t fieldDataCount = this->legacyFieldDataList.size();
	for (size_t i = 0; i < fieldDataCount; ++i)
		if (this->legacyFieldDataList[i]->getField() == fe_field)
			return this->legacyFieldDataList[i];
	return 0;
}

LegacyElementFieldData *cmzn_elementtemplate::getOrCreateLegacyElementFieldData(FE_field *fe_field)
{
	LegacyElementFieldData *legacyFieldData = this->getLegacyElementFieldData(fe_field);
	if (!legacyFieldData)
	{
		legacyFieldData = new LegacyElementFieldData(fe_field);
		this->legacyFieldDataList.push_back(legacyFieldData);
	}
	return legacyFieldData;
}

/** only call if have already checked has legacy node maps
  * Caller's responsibility to mark element as changed; this marks all fields as changed.
  * Expect to be called during FE_region change cache */
int cmzn_elementtemplate::setLegacyNodesInElement(cmzn_element *element)
{
	FE_mesh *mesh = this->getMesh();
	std::vector<DsLabelIndex> workingNodeIndexes(this->legacyNodesCount, DS_LABEL_INDEX_INVALID);
	const size_t fieldCount = this->legacyFieldDataList.size();
	for (size_t f = 0; f < fieldCount; ++f)
	{
		LegacyElementFieldData *legacyData = this->legacyFieldDataList[f];
		FE_field *field = legacyData->getField();
		const int componentCount = legacyData->getComponentCount();
		const LegacyElementFieldData::NodeMap *lastComponentNodeMap = 0; // keep last one so efficient if used by multiple components
		for (int c = 0; c < componentCount; ++c)
		{
			const LegacyElementFieldData::NodeMap *componentNodeMap = legacyData->getComponentNodeMap(c);
			if (componentNodeMap && (componentNodeMap != lastComponentNodeMap))
			{
				lastComponentNodeMap = componentNodeMap;
				FE_element_field_template *eft = this->fe_element_template->getElementfieldtemplate(field, c);
				if (!eft)
				{
					display_message(ERROR_MESSAGE,
						"Elementtemplate  setLegacyNodesInElement.  Have legacy node map without field defined");
					return CMZN_ERROR_NOT_FOUND;
				}
				const int nodeIndexesCount = componentNodeMap->getNodeIndexesCount();
				const int *nodeIndexes = componentNodeMap->getNodeIndexes();
				if (eft->getNumberOfLocalNodes() != nodeIndexesCount)
				{
					display_message(ERROR_MESSAGE,
						"Elementtemplate  setLegacyNodesInElement.  Number of nodes does not match element field template");
					return CMZN_ERROR_GENERAL;
				}
				FE_mesh_element_field_template_data *eftData = mesh->getElementfieldtemplateData(eft);
				if (!eftData)
				{
					display_message(ERROR_MESSAGE,
						"Elementtemplate  setLegacyNodesInElement.  Invalid element field template");
					return CMZN_ERROR_GENERAL;
				}
				if (workingNodeIndexes.capacity() < static_cast<size_t>(nodeIndexesCount))
					workingNodeIndexes.reserve(nodeIndexesCount);
				for (int n = 0; n < nodeIndexesCount; ++n)
				{
					cmzn_node *node = this->legacyNodes[nodeIndexes[n] - 1];
					workingNodeIndexes[n] = (node) ? node->getIndex() : DS_LABEL_INDEX_INVALID;
				}
				const int result = eftData->setElementLocalNodes(element->getIndex(), workingNodeIndexes.data());
				if (result != CMZN_OK)
				{
					display_message(ERROR_MESSAGE,
						"Elementtemplate  setLegacyNodesInElement.  Failed to set element local nodes");
					return result;
				}
			}
		}
	}
	// simplest to mark all fields as changed as they may share local nodes and scale factors
	// can optimise in future
	mesh->get_FE_region()->FE_field_all_change(CHANGE_LOG_RELATED_OBJECT_CHANGED(FE_field));
	return CMZN_OK;
}

int cmzn_elementtemplate::setElementShapeType(cmzn_element_shape_type shapeTypeIn)
{
	FE_element_shape *elementShape = 0;
	if (CMZN_ELEMENT_SHAPE_TYPE_INVALID != shapeTypeIn)
	{
		const int shapeDimension = cmzn_element_shape_type_get_dimension(shapeTypeIn);
		if (shapeDimension != this->getMesh()->getDimension())
		{
			display_message(ERROR_MESSAGE,
				"Elementtemplate  setElementShapeType.  Shape dimension is different from template mesh");
			return CMZN_ERROR_ARGUMENT;
		}
		elementShape = FE_element_shape_create_simple_type(this->getMesh()->get_FE_region(), shapeTypeIn);
		if (!elementShape)
		{
			display_message(ERROR_MESSAGE,
				"Elementtemplate  setElementShapeType.  Failed to create element shape");
			return CMZN_ERROR_GENERAL;
		}
	}
	int return_code = this->fe_element_template->setElementShape(elementShape);
	if (elementShape)
		DEACCESS(FE_element_shape)(&elementShape);
	return return_code;
}

int cmzn_elementtemplate::setNumberOfNodes(int legacyNodesCountIn)
{
	if (legacyNodesCountIn < legacyNodesCount)
	{
		display_message(ERROR_MESSAGE,
			"Elementtemplate setNumberOfNodes.  Cannot reduce number of nodes");
		return CMZN_ERROR_ARGUMENT;
	}
	cmzn_node **newLegacyNodes = new cmzn_node*[legacyNodesCountIn];
	if (!newLegacyNodes)
		return CMZN_ERROR_MEMORY;
	this->clearLegacyNodes();
	for (int i = 0; i < legacyNodesCountIn; ++i)
		newLegacyNodes[i] = 0;
	this->legacyNodes = newLegacyNodes;
	this->legacyNodesCount = legacyNodesCountIn;
	return CMZN_OK;
}

int cmzn_elementtemplate::defineField(FE_field *field, int componentNumber, cmzn_elementfieldtemplate *eft)
{
	if (!((field) && ((-1 == componentNumber) || ((0 < componentNumber) && (componentNumber <= get_FE_field_number_of_components(field))))
		&& (eft)))
	{
		display_message(ERROR_MESSAGE, "Elementtemplate defineField.  Invalid arguments");
		return CMZN_ERROR_ARGUMENT;
	}
	int return_code = this->fe_element_template->defineField(field, componentNumber - 1, eft);
	if (CMZN_OK != return_code)
		display_message(ERROR_MESSAGE, "Elementtemplate defineField.  Failed");
	return return_code;
}

int cmzn_elementtemplate::defineField(cmzn_field_id field, int componentNumber, cmzn_elementfieldtemplate *eft)
{
	FE_field *fe_field = 0;
	Computed_field_get_type_finite_element(field, &fe_field);
	if (!fe_field)
	{
		display_message(ERROR_MESSAGE,
			"Elementtemplate defineField.  Can only define a finite element type field on elements");
		return CMZN_ERROR_ARGUMENT;
	}
	return this->defineField(fe_field, componentNumber, eft);
}

int cmzn_elementtemplate::addLegacyNodeIndexes(FE_field *field, int componentNumber, int nodeIndexesCount,
	const int *nodeIndexes)
{
	const int componentCount = get_FE_field_number_of_components(field);
	if (!((field) && ((-1 == componentNumber) || ((0 < componentNumber) && (componentNumber <= componentCount)))
		&& ((0 == nodeIndexesCount) || (nodeIndexes))))
	{
		display_message(ERROR_MESSAGE, "Elementtemplate addLegacyNodeIndexes.  Invalid arguments");
		return CMZN_ERROR_ARGUMENT;
	}
	const FE_element_field_template *eft = this->fe_element_template->getElementfieldtemplate(field, (componentNumber > 0) ? componentNumber - 1 : 0);
	if (!eft)
	{
		display_message(ERROR_MESSAGE, "Elementtemplate addLegacyNodeIndexes.  Field %s component is not defined", get_FE_field_name(field));
		return CMZN_ERROR_ARGUMENT;
	}
	if (eft->getParameterMappingMode() != CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE)
	{
		display_message(ERROR_MESSAGE, "Elementtemplate addLegacyNodeIndexes.  "
			"Field %s component is not using node-based parameter map", get_FE_field_name(field));
		return CMZN_ERROR_ARGUMENT;
	}
	if (componentNumber < 0)
	{
		// check homogeneous
		for (int c = 1; c < componentCount; ++c)
		{
			if (this->fe_element_template->getElementfieldtemplate(field, c) != eft)
			{
				display_message(ERROR_MESSAGE, "Elementtemplate addLegacyNodeIndexes.  "
					"Field %s must use same element template for all components to use component -1", get_FE_field_name(field));
				return CMZN_ERROR_ARGUMENT;
			}
		}
	}
	int highestNodeIndex = eft->getHighestLocalNodeIndex();
	if (highestNodeIndex > (nodeIndexesCount - 1))
	{
		display_message(ERROR_MESSAGE, "Elementtemplate addLegacyNodeIndexes.  "
			"Node index map does not cover all local nodes in component template for field %s", get_FE_field_name(field));
		return CMZN_ERROR_ARGUMENT;
	}
	for (int i = 0; i < nodeIndexesCount; i++)
	{
		if ((nodeIndexes[i] < 1) || (nodeIndexes[i] > this->legacyNodesCount))
		{
			display_message(ERROR_MESSAGE, "Elementtemplate addLegacyNodeIndexes.  Local node index out of range 1 to number in element (%d)",
				this->legacyNodesCount);
			return CMZN_ERROR_ARGUMENT;
		}
	}
	LegacyElementFieldData *legacyFieldData = getOrCreateLegacyElementFieldData(field);
	if (!legacyFieldData)
		return CMZN_ERROR_GENERAL;
	return legacyFieldData->setNodeMap(componentNumber - 1, nodeIndexesCount, nodeIndexes);
}

int cmzn_elementtemplate::defineFieldSimpleNodal(cmzn_field_id field,
	int componentNumber, cmzn_elementbasis_id elementbasis, int nodeIndexesCount,
	const int *nodeIndexes)
{
	if (!((field) && ((-1 == componentNumber) || ((0 < componentNumber) && (componentNumber <= cmzn_field_get_number_of_components(field))))
		&& (elementbasis) && ((0 == nodeIndexesCount)  || (nodeIndexes))))
	{
		display_message(ERROR_MESSAGE,
			"Elementtemplate defineFieldSimpleNodal.  Invalid arguments");
		return CMZN_ERROR_ARGUMENT;
	}
	FE_basis *fe_basis = elementbasis->getFeBasis();
	if (!fe_basis)
	{
		display_message(ERROR_MESSAGE,
			"Elementtemplate defineFieldSimpleNodal.  Element basis is invalid or incomplete");
		return CMZN_ERROR_ARGUMENT;
	}
	int return_code = CMZN_OK;
	const int expected_basis_number_of_nodes = FE_basis_get_number_of_nodes(fe_basis);
	if (nodeIndexesCount != expected_basis_number_of_nodes)
	{
		display_message(ERROR_MESSAGE,
			"Elementtemplate defineFieldSimpleNodal.  %d node indexes supplied but %d expected for basis",
			nodeIndexesCount, expected_basis_number_of_nodes);
		return_code = CMZN_ERROR_ARGUMENT;
	}
	cmzn_field_finite_element_id finite_element_field = cmzn_field_cast_finite_element(field);
	FE_field *fe_field = 0;
	if (finite_element_field)
	{
		Computed_field_get_type_finite_element(field, &fe_field);
		if (fe_field->get_FE_region() != this->getMesh()->get_FE_region())
		{
			display_message(ERROR_MESSAGE,
				"Elementtemplate defineFieldSimpleNodal.  Field is from another region");
			return_code = CMZN_ERROR_ARGUMENT;
		}
		cmzn_field_finite_element_destroy(&finite_element_field);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Elementtemplate defineFieldSimpleNodal.  "
			"Can only define real finite element field type on elements");
		return_code = CMZN_ERROR_ARGUMENT;
	}
	if (CMZN_OK == return_code)
	{
		cmzn_elementfieldtemplate *eft = cmzn_elementfieldtemplate::create(this->getMesh(), fe_basis);
		if (eft)
		{
			return_code = this->fe_element_template->defineField(fe_field, componentNumber - 1, eft);
			if (CMZN_OK == return_code)
				return_code = this->addLegacyNodeIndexes(fe_field, componentNumber, nodeIndexesCount, nodeIndexes);
			if (CMZN_OK != return_code)
				display_message(ERROR_MESSAGE, "Elementtemplate defineFieldSimpleNodal.  Failed");
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Elementtemplate defineFieldSimpleNodal.  Could not create element field template");
			return_code = CMZN_ERROR_GENERAL;
		}
		cmzn_elementfieldtemplate::deaccess(eft);
	}
	return return_code;
}

int cmzn_elementtemplate::defineFieldElementConstant(cmzn_field_id field, int componentNumber)
{
	if (!((field) && ((-1 == componentNumber) || ((0 < componentNumber) && (componentNumber <= cmzn_field_get_number_of_components(field))))))
	{
		display_message(ERROR_MESSAGE,
			"Elementtemplate defineFieldElementConstant.  Invalid arguments");
		return CMZN_ERROR_ARGUMENT;
	}
	cmzn_elementbasis_id basis = cmzn_elementbasis::create(this->getMesh()->get_FE_region(),
		this->getMesh()->getDimension(), CMZN_ELEMENTBASIS_FUNCTION_TYPE_CONSTANT);
	FE_basis *fe_basis = basis->getFeBasis();
	cmzn_elementbasis_destroy(&basis);
	if (!fe_basis)
	{
		display_message(ERROR_MESSAGE,
			"Elementtemplate defineFieldElementConstant.  Failed to create constant basis");
		return CMZN_ERROR_GENERAL;
	}
	int return_code = CMZN_OK;
	FE_field *fe_field = 0;
	Computed_field_get_type_finite_element(field, &fe_field);
	if ((fe_field) && (GENERAL_FE_FIELD == get_FE_field_FE_field_type(fe_field)) &&
		((FE_VALUE_VALUE == get_FE_field_value_type(fe_field)) || (INT_VALUE == get_FE_field_value_type(fe_field))))
	{
		if (fe_field->get_FE_region() != this->getMesh()->get_FE_region())
		{
			display_message(ERROR_MESSAGE,
				"Elementtemplate defineFieldElementConstant.  Field is from another region");
			return_code = CMZN_ERROR_ARGUMENT;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE, "Elementtemplate defineFieldElementConstant.  "
			"Can only define real or integer finite element field type on elements");
		return_code = CMZN_ERROR_ARGUMENT;
	}
	if (CMZN_OK == return_code)
	{
		cmzn_elementfieldtemplate *eft = cmzn_elementfieldtemplate::create(this->getMesh(), fe_basis);
		if (eft)
		{
			return_code = eft->setParameterMappingMode(CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_ELEMENT);
			if (CMZN_OK == return_code)
				return_code = this->fe_element_template->defineField(fe_field, componentNumber - 1, eft);
			if (CMZN_OK != return_code)
				display_message(ERROR_MESSAGE, "Elementtemplate defineFieldElementConstant.  Failed");
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Elementtemplate defineFieldElementConstant.  Could not create element field template");
			return_code = CMZN_ERROR_GENERAL;
		}
		cmzn_elementfieldtemplate::deaccess(eft);
	}
	return return_code;
}

/** @param componentNumber  -1 for all components, or positive for single component
  * \note DEPRECATED; only expected to work with defineFieldSimpleNodal */
int cmzn_elementtemplate::setMapNodeValueLabel(cmzn_field_id field, int componentNumber,
	int basisNodeIndex, int nodeFunctionIndex, cmzn_node_value_label nodeValueLabel)
{
	if ((!field) || (componentNumber < -1) || (componentNumber == 0) || (componentNumber > cmzn_field_get_number_of_components(field))
		|| (basisNodeIndex < 1) || (nodeFunctionIndex < 1))
		return CMZN_ERROR_ARGUMENT;
	FE_field *fe_field = 0;
	if (Computed_field_get_type_finite_element(field, &fe_field))
	{
		const int firstComponentNumber = (componentNumber < 0) ? 0 : (componentNumber - 1);
		const int limitComponentNumber = (componentNumber < 0) ? cmzn_field_get_number_of_components(field) : componentNumber;
		int return_code = CMZN_OK;
		for (int c = firstComponentNumber; c < limitComponentNumber; ++c)
		{
			FE_element_field_template *eftInt = this->fe_element_template->getElementfieldtemplate(fe_field, c);
			if (!eftInt)
			{
				display_message(ERROR_MESSAGE,
					"Elementtemplate setMapNodeValueLabel.  Field %s is not defined", get_FE_field_name(fe_field));
				return CMZN_ERROR_ARGUMENT;
			}
			cmzn_elementfieldtemplate *eft = cmzn_elementfieldtemplate::create(eftInt);
			if (!eft)
			{
				display_message(ERROR_MESSAGE,
					"Elementtemplate setMapNodeValueLabel.  Failed to get element field template");
				return CMZN_ERROR_ARGUMENT;
			}
			const int functionNumber = FE_basis_get_function_number_from_node_function(eft->getBasis(), basisNodeIndex - 1, nodeFunctionIndex - 1);
			if (functionNumber < 0)
			{
				display_message(ERROR_MESSAGE,
					"Elementtemplate setMapNodeValueLabel.  Invalid node or node function index for basis");
				return_code = CMZN_ERROR_ARGUMENT;
			}
			else
			{
				const int term = 1;
				return_code = eft->setTermNodeParameter(functionNumber + 1, term,
					eft->getTermLocalNodeIndex(functionNumber + 1, term),
					nodeValueLabel,
					eft->getTermNodeVersion(functionNumber + 1, term));
				if (CMZN_OK != return_code)
				{
					display_message(ERROR_MESSAGE,
						"Elementtemplate setMapNodeValueLabel.  Failed; only expected to work with defineFieldSimpleNodal.");
				}
			}
			if (CMZN_OK == return_code)
			{
				// need to swap with the eft used for the component
				return_code = this->fe_element_template->defineField(fe_field, c, eft);
				if (CMZN_OK != return_code)
				{
					display_message(ERROR_MESSAGE, "Elementtemplate setMapNodeValueLabel.  Failed to set in element template");
				}
			}
			cmzn_elementfieldtemplate::deaccess(eft);
			if (CMZN_OK != return_code)
				break;
		}
		return return_code;
	}
	return CMZN_ERROR_ARGUMENT;
}

/** @param componentNumber  -1 for all components, or positive for single component
  * \note DEPRECATED; only expected to work with defineFieldSimpleNodal */
int cmzn_elementtemplate::setMapNodeVersion(cmzn_field_id field, int componentNumber,
	int basisNodeIndex, int nodeFunctionIndex, int versionNumber)
{
	if ((!field) || (componentNumber < -1) || (componentNumber == 0) || (componentNumber > cmzn_field_get_number_of_components(field))
		|| (basisNodeIndex < 1) || (nodeFunctionIndex < 1))
		return CMZN_ERROR_ARGUMENT;
	FE_field *fe_field = 0;
	if (Computed_field_get_type_finite_element(field, &fe_field))
	{
		const int firstComponentNumber = (componentNumber < 0) ? 0 : (componentNumber - 1);
		const int limitComponentNumber = (componentNumber < 0) ? cmzn_field_get_number_of_components(field) : componentNumber;
		int return_code = CMZN_OK;
		for (int c = firstComponentNumber; c < limitComponentNumber; ++c)
		{
			FE_element_field_template *eftInt = this->fe_element_template->getElementfieldtemplate(fe_field, c);
			if (!eftInt)
			{
				display_message(ERROR_MESSAGE,
					"Elementtemplate setMapNodeVersion.  Field %s is not defined", get_FE_field_name(fe_field));
				return CMZN_ERROR_ARGUMENT;
			}
			cmzn_elementfieldtemplate *eft = cmzn_elementfieldtemplate::create(eftInt);
			if (!eft)
			{
				display_message(ERROR_MESSAGE,
					"Elementtemplate setMapNodeVersion.  Failed to get element field template");
				return CMZN_ERROR_ARGUMENT;
			}
			const int functionNumber = FE_basis_get_function_number_from_node_function(eft->getBasis(), basisNodeIndex - 1, nodeFunctionIndex - 1);
			if (functionNumber < 0)
			{
				display_message(ERROR_MESSAGE,
					"Elementtemplate setMapNodeVersion.  Invalid node or node function index for basis");
				return_code = CMZN_ERROR_ARGUMENT;
			}
			else
			{
				const int term = 1;
				return_code = eft->setTermNodeParameter(functionNumber + 1, term,
					eft->getTermLocalNodeIndex(functionNumber + 1, term),
					eft->getTermNodeValueLabel(functionNumber + 1, term),
					versionNumber);
				if (CMZN_OK != return_code)
				{
					display_message(ERROR_MESSAGE,
						"Elementtemplate setMapNodeVersion.  Failed; only expected to work with defineFieldSimpleNodal.");
				}
			}
			if (CMZN_OK == return_code)
			{
				// need to swap with the eft used for the component
				return_code = this->fe_element_template->defineField(fe_field, c, eft);
				if (CMZN_OK != return_code)
				{
					display_message(ERROR_MESSAGE, "Elementtemplate setMapNodeVersion.  Failed to set in element template");
				}
			}
			cmzn_elementfieldtemplate::deaccess(eft);
			if (CMZN_OK != return_code)
				break;
		}
		return return_code;
	}
	return CMZN_ERROR_ARGUMENT;
}

/** @param local_node_index  Index from 1 to legacy nodes count.
  * @return  Non-accessed node, or 0 if invalid index or no node at index. */
cmzn_node_id cmzn_elementtemplate::getNode(int local_node_index)
{
	if ((0 < local_node_index) && (local_node_index <= this->legacyNodesCount))
		return this->legacyNodes[local_node_index - 1];
	return 0;
}

/** @param local_node_index  Index from 1 to legacy nodes count. */
int cmzn_elementtemplate::setNode(int local_node_index, cmzn_node_id node)
{
	FE_nodeset *nodeset;
	if ((0 < local_node_index) && (local_node_index <= this->legacyNodesCount)
		&& ((!node) || ((0 != (nodeset = FE_node_get_FE_nodeset(node)))
			&& (nodeset->getFieldDomainType() == CMZN_FIELD_DOMAIN_TYPE_NODES)
			&& (this->fe_element_template->getMesh()->get_FE_region() == nodeset->get_FE_region()))))
	{
		cmzn_node::reaccess(this->legacyNodes[local_node_index - 1], node);
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_elementtemplate::removeField(cmzn_field_id field)
{
	if (!field)
		return CMZN_ERROR_ARGUMENT;
	FE_field *fe_field = 0;
	Computed_field_get_type_finite_element(field, &fe_field);
	if (!fe_field)
	{
		display_message(ERROR_MESSAGE, "Elementtemplate removeField.  Not a finite element field");
		return CMZN_ERROR_ARGUMENT;
	}
	int return_code = this->fe_element_template->removeField(fe_field);
	if (CMZN_OK == return_code)
		this->clearLegacyElementFieldData(fe_field);
	return return_code;
}

int cmzn_elementtemplate::undefineField(cmzn_field_id field)
{
	if (!field)
		return CMZN_ERROR_ARGUMENT;
	FE_field *fe_field = 0;
	Computed_field_get_type_finite_element(field, &fe_field);
	if (!fe_field)
	{
		display_message(ERROR_MESSAGE, "Elementtemplate undefineField.  Not a finite element field");
		return CMZN_ERROR_ARGUMENT;
	}
	int return_code = this->fe_element_template->undefineField(fe_field);
	this->clearLegacyElementFieldData(fe_field);
	return return_code;
}

cmzn_element *cmzn_elementtemplate::createElement(int identifier)
{
	if (!this->validate())
	{
		display_message(ERROR_MESSAGE, "Mesh createElement.  Element template is not valid");
		return 0;
	}
	if (!this->fe_element_template->getElementShape())
	{
		display_message(ERROR_MESSAGE, "Mesh createElement.  Element template does not have a shape set");
		return 0;
	}
	this->beginChange();
	cmzn_element *element = this->getMesh()->create_FE_element(identifier, this->fe_element_template);
	if (element && (this->legacyNodes) && (this->legacyFieldDataList.size() > 0))
	{
		int return_code = this->setLegacyNodesInElement(element);
		if (CMZN_OK != return_code)
		{
			display_message(ERROR_MESSAGE, "Mesh createElement.  Failed to set legacy nodes (deprecated feature)");
			cmzn_element::deaccess(element);
		}
	}
	this->endChange();
	return element;
}

int cmzn_elementtemplate::mergeIntoElement(cmzn_element *element)
{
	if (this->validate())
	{
		this->beginChange();
		int return_code = this->getMesh()->merge_FE_element_template(element, this->fe_element_template);
		if ((CMZN_OK == return_code) && (this->legacyNodes) && (this->legacyFieldDataList.size() > 0))
		{
			return_code = this->setLegacyNodesInElement(element);
			if (CMZN_OK != return_code)
			{
				display_message(ERROR_MESSAGE, "Element merge.  Failed to set legacy nodes (deprecated feature)");
				cmzn_element::deaccess(element);
			}
		}
		this->endChange();
		return return_code;
	}
	display_message(ERROR_MESSAGE, "Element merge.  Element template is not valid");
	return CMZN_ERROR_ARGUMENT;
}


/*============================================================================*/

struct cmzn_mesh
{
protected:
	FE_mesh *fe_mesh;
	cmzn_field_element_group_id group;
	int access_count;

	cmzn_mesh(FE_mesh *fe_mesh_in) :
		fe_mesh(fe_mesh_in->access()),
		group(0),
		access_count(1)
	{
	}

	cmzn_mesh(cmzn_field_element_group_id group) :
		fe_mesh(Computed_field_element_group_core_cast(group)->get_fe_mesh()->access()),
		group(group),
		access_count(1)
	{
		// cmzn_field_element_group_access missing:
		cmzn_field_access(cmzn_field_element_group_base_cast(group));
	}

	~cmzn_mesh()
	{
		if (group)
			cmzn_field_element_group_destroy(&group);
		FE_mesh::deaccess(this->fe_mesh);
	}

public:
	cmzn_mesh_id access()
	{
		++access_count;
		return this;
	}

	static cmzn_mesh *create(FE_mesh *fe_mesh_in)
	{
		if (fe_mesh_in)
			return new cmzn_mesh(fe_mesh_in);
		return nullptr;
	}

	static int deaccess(cmzn_mesh_id &mesh)
	{
		if (!mesh)
			return CMZN_ERROR_ARGUMENT;
		--(mesh->access_count);
		if (mesh->access_count <= 0)
			delete mesh;
		mesh = 0;
		return CMZN_OK;
	}

	bool containsElement(cmzn_element_id element)
	{
		if (group)
			return Computed_field_element_group_core_cast(group)->containsObject(element);
		return element->getMesh() == this->fe_mesh;
	}

	cmzn_element_id createElement(int identifier,
		cmzn_elementtemplate_id elementtemplate)
	{
		if (!elementtemplate)
			return 0;
		if (this->group)
		{
			FE_region_begin_change(this->fe_mesh->get_FE_region());
		}
		FE_element *element = elementtemplate->createElement(identifier);
		if (this->group)
		{
			if (element)
				Computed_field_element_group_core_cast(group)->addObject(element);
			FE_region_end_change(this->fe_mesh->get_FE_region());
		}
		return element;
	}

	cmzn_elementtemplate_id createElementtemplate()
	{
		return cmzn_elementtemplate::create(this->fe_mesh);
	}

	cmzn_elementiterator_id createElementiterator()
	{
		if (group)
			return Computed_field_element_group_core_cast(group)->createElementiterator();
		return this->fe_mesh->createElementiterator();
	}

	int destroyAllElements()
	{
		if (this->group)
		{
			Computed_field_element_group *element_group = Computed_field_element_group_core_cast(this->group);
			return this->fe_mesh->destroyElementsInGroup(element_group->getLabelsGroup());
		}
		return this->fe_mesh->destroyAllElements();
	}

	int destroyElement(cmzn_element_id element)
	{
		if (this->containsElement(element))
			return this->fe_mesh->destroyElement(element);
		return 0;
	}

	int destroyElementsConditional(cmzn_field_id conditional_field)
	{
		if (!conditional_field)
			return CMZN_ERROR_ARGUMENT;
		DsLabelsGroup *labelsGroup = this->fe_mesh->createLabelsGroup();
		if (labelsGroup)
		{
			cmzn_region_id region = FE_region_get_cmzn_region(this->fe_mesh->get_FE_region());
			cmzn_fieldmodule_id fieldmodule = cmzn_region_get_fieldmodule(region);
			cmzn_fieldcache_id cache = cmzn_fieldmodule_create_fieldcache(fieldmodule);
			cmzn_elementiterator_id iterator = this->createElementiterator();
			cmzn_element_id element = 0;
			while (0 != (element = cmzn_elementiterator_next_non_access(iterator)))
			{
				cmzn_fieldcache_set_element(cache, element);
				if (cmzn_field_evaluate_boolean(conditional_field, cache))
					labelsGroup->setIndex(get_FE_element_index(element), true);
			}
			cmzn::Deaccess(iterator);
			cmzn_fieldcache_destroy(&cache);
			cmzn_fieldmodule_destroy(&fieldmodule);
			int return_code = this->fe_mesh->destroyElementsInGroup(*labelsGroup);
			cmzn::Deaccess(labelsGroup);
			return return_code;
		}
		return CMZN_ERROR_GENERAL;
	}

	cmzn_element_id findElementByIdentifier(int identifier) const
	{
		cmzn_element_id element = 0;
		if (group)
		{
			element = Computed_field_element_group_core_cast(group)->findElementByIdentifier(identifier);
		}
		else
		{
			element = this->fe_mesh->findElementByIdentifier(identifier);
		}
		if (element)
			ACCESS(FE_element)(element);
		return element;
	}

	int getDimension() const { return this->fe_mesh->getDimension(); }

	FE_mesh *get_FE_mesh() const { return this->fe_mesh; }

	/** @return  Allocated name */
	char *getName()
	{
		if (group)
			return cmzn_field_get_name(cmzn_field_element_group_base_cast(group));
		else
			return duplicate_string(this->fe_mesh->getName());
		return 0;
	}

	cmzn_mesh_id getMaster()
	{
		if (!isGroup())
			return access();
		return cmzn_mesh::create(this->fe_mesh);
	}

	int getSize() const
	{
		if (group)
			return Computed_field_element_group_core_cast(group)->getSize();
		return this->fe_mesh->getSize();
	}

	int isGroup()
	{
		return (0 != group);
	}

	// if mesh is a group, return the element group field, otherwise return 0
	cmzn_field_element_group *getGroupField()
	{
		return this->group;
	}

	bool match(cmzn_mesh& other_mesh)
	{
		return ((this->fe_mesh == other_mesh.fe_mesh) &&
			(group == other_mesh.group));
	}

};

struct cmzn_mesh_group : public cmzn_mesh
{
public:

	cmzn_mesh_group(cmzn_field_element_group_id group) :
		cmzn_mesh(group)
	{
	}

	int addElement(cmzn_element_id element)
	{
		return Computed_field_element_group_core_cast(group)->addObject(element);
	}

	int addElementsConditional(cmzn_field_id conditional_field)
	{
		return Computed_field_element_group_core_cast(group)->addElementsConditional(conditional_field);
	}

	int removeAllElements()
	{
		return Computed_field_element_group_core_cast(group)->clear();
	}

	int removeElement(cmzn_element_id element)
	{
		return Computed_field_element_group_core_cast(group)->removeObject(element);
	}

	int removeElementsConditional(cmzn_field_id conditional_field)
	{
		return Computed_field_element_group_core_cast(group)->removeElementsConditional(conditional_field);
	}

	int addElementFaces(cmzn_element_id element)
	{
		return Computed_field_element_group_core_cast(group)->addElementFaces(element);
	}

	int removeElementFaces(cmzn_element_id element)
	{
		return Computed_field_element_group_core_cast(group)->removeElementFaces(element);
	}

};

/*
Global functions
----------------
*/

cmzn_elementbasis_id cmzn_fieldmodule_create_elementbasis(
	cmzn_fieldmodule_id fieldmodule, int dimension,
	enum cmzn_elementbasis_function_type function_type)
{
	if (fieldmodule && (0 < dimension) && (dimension <= MAXIMUM_ELEMENT_XI_DIMENSIONS))
	{
		cmzn_region *region = cmzn_fieldmodule_get_region_internal(fieldmodule);
		FE_region *fe_region = region->get_FE_region();
		if (fe_region)
		{
			return cmzn_elementbasis::create(fe_region, dimension, function_type);
		}
	}
	return 0;
}

cmzn_mesh_id cmzn_fieldmodule_find_mesh_by_dimension(
	cmzn_fieldmodule_id fieldmodule, int dimension)
{
	cmzn_mesh_id mesh = NULL;
	if (fieldmodule && (1 <= dimension) && (dimension <= MAXIMUM_ELEMENT_XI_DIMENSIONS))
	{
		FE_region *fe_region = cmzn_fieldmodule_get_region_internal(fieldmodule)->get_FE_region();
		mesh = cmzn_mesh::create(FE_region_find_FE_mesh_by_dimension(fe_region, dimension));
	}
	return mesh;
}

cmzn_mesh_id cmzn_fieldmodule_find_mesh_by_name(
	cmzn_fieldmodule_id fieldmodule, const char *mesh_name)
{
	cmzn_mesh_id mesh = 0;
	if (fieldmodule && mesh_name)
	{
		cmzn_field_id field = cmzn_fieldmodule_find_field_by_name(fieldmodule, mesh_name);
		if (field)
		{
			cmzn_field_element_group_id element_group_field = cmzn_field_cast_element_group(field);
			if (element_group_field)
			{
				mesh = cmzn_mesh_group_base_cast(cmzn_field_element_group_get_mesh_group(element_group_field));
				cmzn_field_element_group_destroy(&element_group_field);
			}
			cmzn_field_destroy(&field);
		}
		else
		{
			int mesh_dimension = 0;
			if      (0 == strcmp(mesh_name, "mesh3d"))
				mesh_dimension = 3;
			else if (0 == strcmp(mesh_name, "mesh2d"))
				mesh_dimension = 2;
			else if (0 == strcmp(mesh_name, "mesh1d"))
				mesh_dimension = 1;
			if (0 < mesh_dimension)
			{
				mesh = cmzn_fieldmodule_find_mesh_by_dimension(fieldmodule, mesh_dimension);
			}
		}
	}
	return (mesh);
}

cmzn_mesh_id cmzn_mesh_access(cmzn_mesh_id mesh)
{
	if (mesh)
		return mesh->access();
	return 0;
}

int cmzn_mesh_destroy(cmzn_mesh_id *mesh_address)
{
	if (mesh_address)
		return cmzn_mesh::deaccess(*mesh_address);
	return CMZN_ERROR_ARGUMENT;
}

bool cmzn_mesh_contains_element(cmzn_mesh_id mesh, cmzn_element_id element)
{
	if (mesh)
		return mesh->containsElement(element);
	return false;
}

cmzn_elementtemplate_id cmzn_mesh_create_elementtemplate(
	cmzn_mesh_id mesh)
{
	if (mesh)
		return mesh->createElementtemplate();
	return 0;
}

cmzn_element_id cmzn_mesh_create_element(cmzn_mesh_id mesh,
	int identifier, cmzn_elementtemplate_id element_template)
{
	if (mesh)
		return mesh->createElement(identifier, element_template);
	return 0;
}

cmzn_elementiterator_id cmzn_mesh_create_elementiterator(
	cmzn_mesh_id mesh)
{
	if (mesh)
		return mesh->createElementiterator();
	return 0;
}

int cmzn_mesh_define_element(cmzn_mesh_id mesh, int identifier,
	cmzn_elementtemplate_id element_template)
{
	cmzn_element_id element =
		cmzn_mesh_create_element(mesh, identifier, element_template);
	if (element)
	{
		cmzn_element_destroy(&element);
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_mesh_destroy_all_elements(cmzn_mesh_id mesh)
{
	if (mesh)
		return mesh->destroyAllElements();
	return 0;
}

int cmzn_mesh_destroy_element(cmzn_mesh_id mesh, cmzn_element_id element)
{
	if (mesh && element)
		return mesh->destroyElement(element);
	return 0;
}

int cmzn_mesh_destroy_elements_conditional(cmzn_mesh_id mesh,
	cmzn_field_id conditional_field)
{
	if (mesh)
		return mesh->destroyElementsConditional(conditional_field);
	return CMZN_ERROR_ARGUMENT;
}

cmzn_element_id cmzn_mesh_find_element_by_identifier(cmzn_mesh_id mesh,
	int identifier)
{
	if (mesh)
		return mesh->findElementByIdentifier(identifier);
	return 0;
}

cmzn_differentialoperator_id cmzn_mesh_get_chart_differentialoperator(
	cmzn_mesh_id mesh, int order, int term)
{
	if ((mesh) && ((-1 == order) || ((1 <= order) && (order <= MAXIMUM_MESH_DERIVATIVE_ORDER))))
	{
		FieldDerivative *fieldDerivative = mesh->get_FE_mesh()->getFieldDerivative(order);
		if (fieldDerivative)
		{
			return cmzn_differentialoperator::create(fieldDerivative, term - 1);
		}
	}
	display_message(ERROR_MESSAGE, "Mesh getChartDifferentialoperator.  Invalid argument(s)");
	return nullptr;
}

int cmzn_mesh_get_dimension(cmzn_mesh_id mesh)
{
	if (mesh)
		return mesh->getDimension();
	return 0;
}

cmzn_fieldmodule_id cmzn_mesh_get_fieldmodule(cmzn_mesh_id mesh)
{
	if (mesh)
	{
		cmzn_region *region = FE_region_get_cmzn_region(mesh->get_FE_mesh()->get_FE_region());
		return cmzn_fieldmodule_create(region);
	}
	return 0;
}

cmzn_mesh_id cmzn_mesh_get_master_mesh(cmzn_mesh_id mesh)
{
	if (mesh)
		return mesh->getMaster();
	return 0;
}

char *cmzn_mesh_get_name(cmzn_mesh_id mesh)
{
	if (mesh)
		return mesh->getName();
	return 0;
}

int cmzn_mesh_get_size(cmzn_mesh_id mesh)
{
	if (mesh)
		return mesh->getSize();
	return 0;
}

bool cmzn_mesh_match(cmzn_mesh_id mesh1, cmzn_mesh_id mesh2)
{
	return (mesh1 && mesh2 && mesh1->match(*mesh2));
}

cmzn_mesh_group_id cmzn_mesh_cast_group(cmzn_mesh_id mesh)
{
	if (mesh && mesh->isGroup())
		return static_cast<cmzn_mesh_group_id>(mesh->access());
	return 0;
}

int cmzn_mesh_group_destroy(cmzn_mesh_group_id *mesh_group_address)
{
	if (mesh_group_address)
		return cmzn_mesh::deaccess(*(reinterpret_cast<cmzn_mesh_id*>(mesh_group_address)));
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_mesh_group_add_element(cmzn_mesh_group_id mesh_group, cmzn_element_id element)
{
	if (mesh_group)
		return mesh_group->addElement(element);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_mesh_group_add_elements_conditional(cmzn_mesh_group_id mesh_group,
   cmzn_field_id conditional_field)
{
	if (mesh_group)
		return mesh_group->addElementsConditional(conditional_field);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_mesh_group_remove_all_elements(cmzn_mesh_group_id mesh_group)
{
	if (mesh_group)
		return mesh_group->removeAllElements();
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_mesh_group_remove_element(cmzn_mesh_group_id mesh_group, cmzn_element_id element)
{
	if (mesh_group)
		return mesh_group->removeElement(element);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_mesh_group_remove_elements_conditional(cmzn_mesh_group_id mesh_group,
	cmzn_field_id conditional_field)
{
	if (mesh_group)
		return mesh_group->removeElementsConditional(conditional_field);
	return CMZN_ERROR_ARGUMENT;
}

cmzn_mesh *cmzn_mesh_create(FE_mesh *fe_mesh)
{
	return cmzn_mesh::create(fe_mesh);
}

int cmzn_mesh_group_add_element_faces(cmzn_mesh_group_id mesh_group, cmzn_element_id element)
{
	if (mesh_group)
		return mesh_group->addElementFaces(element);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_mesh_group_remove_element_faces(cmzn_mesh_group_id mesh_group, cmzn_element_id element)
{
	if (mesh_group)
		return mesh_group->removeElementFaces(element);
	return CMZN_ERROR_ARGUMENT;
}

cmzn_mesh_group_id cmzn_field_element_group_get_mesh_group(
	cmzn_field_element_group_id element_group)
{
	if (element_group)
		return new cmzn_mesh_group(element_group);
	return 0;
}

FE_mesh *cmzn_mesh_get_FE_mesh_internal(cmzn_mesh_id mesh)
{
	if (mesh)
		return mesh->get_FE_mesh();
	return 0;
}

FE_region *cmzn_mesh_get_FE_region_internal(cmzn_mesh_id mesh)
{
	if (mesh)
		return mesh->get_FE_mesh()->get_FE_region();
	return 0;
}

cmzn_region_id cmzn_mesh_get_region_internal(cmzn_mesh_id mesh)
{
	if (!mesh)
		return 0;
	return FE_region_get_cmzn_region(mesh->get_FE_mesh()->get_FE_region());
}

cmzn_field_element_group *cmzn_mesh_get_element_group_field_internal(cmzn_mesh_id mesh)
{
	if (mesh)
		return mesh->getGroupField();
	return 0;
}

cmzn_elementbasis_id cmzn_elementbasis_access(
	cmzn_elementbasis_id element_basis)
{
	if (element_basis)
		return element_basis->access();
	return 0;
}

int cmzn_elementbasis_destroy(cmzn_elementbasis_id *element_basis_address)
{
	if (element_basis_address)
		return cmzn_elementbasis::deaccess(*element_basis_address);
	return CMZN_ERROR_ARGUMENT;
}

FE_basis *cmzn_elementbasis_get_FE_basis(cmzn_elementbasis_id elementbasis)
{
	if (elementbasis)
		return elementbasis->getFeBasis();
	return 0;
}

int cmzn_elementbasis_get_dimension(cmzn_elementbasis_id element_basis)
{
	if (element_basis)
		return element_basis->getDimension();
	return 0;
}

enum cmzn_elementbasis_function_type cmzn_elementbasis_get_function_type(
	cmzn_elementbasis_id element_basis, int chart_component)
{
	if (element_basis)
		return element_basis->getFunctionType(chart_component);
	return CMZN_ELEMENTBASIS_FUNCTION_TYPE_INVALID;
}

int cmzn_elementbasis_set_function_type(cmzn_elementbasis_id element_basis,
	int chart_component, enum cmzn_elementbasis_function_type function_type)
{
	if (element_basis)
		return element_basis->setFunctionType(chart_component, function_type);
	return 0;
}

int cmzn_elementbasis_get_number_of_nodes(
	cmzn_elementbasis_id element_basis)
{
	if (element_basis)
		return element_basis->getNumberOfNodes();
	return 0;
}

int cmzn_elementbasis_get_number_of_functions(
	cmzn_elementbasis_id element_basis)
{
	if (element_basis)
		return element_basis->getNumberOfFunctions();
	return 0;
}

int cmzn_elementbasis_get_number_of_functions_per_node(
	cmzn_elementbasis_id element_basis, int basis_node_index)
{
	if (element_basis)
		return element_basis->getNumberOfFunctionsPerNode(basis_node_index);
	return 0;
}

cmzn_elementtemplate_id cmzn_elementtemplate_access(
	cmzn_elementtemplate_id element_template)
{
	if (element_template)
		return element_template->access();
	return 0;
}

int cmzn_elementtemplate_destroy(
	cmzn_elementtemplate_id *element_template_address)
{
	if (element_template_address)
		return cmzn_elementtemplate::deaccess(*element_template_address);
	return CMZN_ERROR_ARGUMENT;
}

enum cmzn_element_shape_type cmzn_elementtemplate_get_element_shape_type(
	cmzn_elementtemplate_id element_template)
{
	if (element_template)
		return element_template->getShapeType();
	return CMZN_ELEMENT_SHAPE_TYPE_INVALID;
}

int cmzn_elementtemplate_set_element_shape_type(cmzn_elementtemplate_id element_template,
	enum cmzn_element_shape_type shape_type)
{
	if (element_template)
		return element_template->setElementShapeType(shape_type);
	return 0;
}

int cmzn_elementtemplate_get_number_of_nodes(
	cmzn_elementtemplate_id element_template)
{
	if (element_template)
		return element_template->getNumberOfNodes();
	return 0;
}

int cmzn_elementtemplate_set_number_of_nodes(
	cmzn_elementtemplate_id element_template, int number_of_nodes)
{
	if (element_template)
		return element_template->setNumberOfNodes(number_of_nodes);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_elementtemplate_define_field(
	cmzn_elementtemplate_id elementtemplate, cmzn_field_id field,
	int component_number, cmzn_elementfieldtemplate_id eft)
{
	if (elementtemplate)
		return elementtemplate->defineField(field, component_number, eft);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_elementtemplate_define_field_element_constant(cmzn_elementtemplate_id element_template,
	cmzn_field_id field, int component_number)
{
	if (element_template)
		return element_template->defineFieldElementConstant(field, component_number);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_elementtemplate_define_field_simple_nodal(
	cmzn_elementtemplate_id elementtemplate,
	cmzn_field_id field,  int component_number,
	cmzn_elementbasis_id basis, int number_of_nodes,
	const int *local_node_indexes)
{
	if (elementtemplate)
		return elementtemplate->defineFieldSimpleNodal(
			field, component_number, basis, number_of_nodes, local_node_indexes);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_elementtemplate_set_map_node_value_label(
	cmzn_elementtemplate_id elementtemplate, cmzn_field_id field,
	int component_number, int basis_node_index, int node_function_index,
	enum cmzn_node_value_label node_value_label)
{
	if (elementtemplate)
		return elementtemplate->setMapNodeValueLabel(field, component_number,
			basis_node_index, node_function_index, node_value_label);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_elementtemplate_set_map_node_version(
	cmzn_elementtemplate_id elementtemplate, cmzn_field_id field,
	int component_number, int basis_node_index, int node_function_index,
	int version_number)
{
	if (elementtemplate)
		return elementtemplate->setMapNodeVersion(field, component_number,
			basis_node_index, node_function_index, version_number);
	return CMZN_ERROR_ARGUMENT;
}

cmzn_node_id cmzn_elementtemplate_get_node(
	cmzn_elementtemplate_id element_template, int local_node_index)
{
	if (element_template)
	{
		cmzn_node *node = element_template->getNode(local_node_index);
		if (node)
			return cmzn_node_access(node);
	}
	return 0;
}

int cmzn_elementtemplate_set_node(cmzn_elementtemplate_id element_template,
	int local_node_index, cmzn_node_id node)
{
	if (element_template)
		return element_template->setNode(local_node_index, node);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_elementtemplate_remove_field(
	cmzn_elementtemplate_id elementtemplate, cmzn_field_id field)
{
	if (elementtemplate)
		return elementtemplate->removeField(field);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_elementtemplate_undefine_field(
	cmzn_elementtemplate_id elementtemplate, cmzn_field_id field)
{
	if (elementtemplate)
		return elementtemplate->undefineField(field);
	return CMZN_ERROR_ARGUMENT;
}

cmzn_element_id cmzn_element_access(cmzn_element_id element)
{
	if (element)
		return element->access();
	return 0;
}

int cmzn_element_destroy(cmzn_element_id *element_address)
{
	if (element_address)
		return cmzn_element::deaccess(*element_address);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_element_set_identifier(cmzn_element_id element, int identifier)
{
	return element->setIdentifier(identifier);
}

int cmzn_element_get_dimension(cmzn_element_id element)
{
	if (element)
		return element->getDimension();
	return 0;
}

cmzn_elementfieldtemplate_id cmzn_element_get_elementfieldtemplate(
	cmzn_element_id element, cmzn_field_id field, int componentNumber)
{
	if (!(element && field && ((-1 == componentNumber) ||
		((1 <= componentNumber) && (componentNumber <= cmzn_field_get_number_of_components(field))))))
	{
		display_message(ERROR_MESSAGE, "Element getElementfieldtemplate.  Invalid argument(s)");
		return 0;
	}
	FE_field *fe_field = 0;
	Computed_field_get_type_finite_element(field, &fe_field);
	Value_type valueType = get_FE_field_value_type(fe_field);
	if ((!fe_field)
		|| (get_FE_field_FE_field_type(fe_field) != GENERAL_FE_FIELD)
		|| ((valueType != FE_VALUE_VALUE) && (valueType != INT_VALUE)))
	{
		display_message(ERROR_MESSAGE, "Element getElementfieldtemplate.  Can only query a finite element type field on elements");
		return 0;
	}
	FE_mesh_field_data *meshFieldData = fe_field->getMeshFieldData(element->getMesh());
	if (!meshFieldData)
		return 0;
	FE_element_field_template *eft = 0;
	const int firstComponent = (componentNumber > 0) ? componentNumber - 1 : 0;
	const int limitComponent = (componentNumber > 0) ? componentNumber : get_FE_field_number_of_components(fe_field);
	FE_mesh_field_template *lastMft = 0;
	for (int c = firstComponent; c < limitComponent; ++c)
	{
		FE_mesh_field_template *mft = meshFieldData->getComponentMeshfieldtemplate(c);
		if (!mft)
		{
			display_message(ERROR_MESSAGE, "Element getElementfieldtemplate.  No mesh field template found for component %d", c + 1);
			return 0;
		}
		if (mft != lastMft)
		{
			FE_element_field_template *eftTmp = mft->getElementfieldtemplate(element->getIndex());
			if (!eftTmp)
				return 0; // not defined
			if (eft)
			{
				if (eftTmp != eft)
					return 0; // not homogeneous;
			}
			else
			{
				eft = eftTmp;
			}
			lastMft = mft;
		}
	}
	return cmzn_elementfieldtemplate::create(eft);
}

int cmzn_element_get_identifier(struct cmzn_element *element)
{
	if (element)
		return element->getIdentifier();
	return DS_LABEL_IDENTIFIER_INVALID;
}

cmzn_mesh_id cmzn_element_get_mesh(cmzn_element_id element)
{
	if (element)
		return cmzn_mesh::create(element->getMesh());
	return nullptr;
}

cmzn_node_id cmzn_element_get_node(cmzn_element_id element,
	cmzn_elementfieldtemplate_id eft, int localNodeIndex)
{
	if (!((element) && (eft) && (0 < localNodeIndex) && (localNodeIndex <= eft->getNumberOfLocalNodes())))
	{
		display_message(ERROR_MESSAGE, "Element getNode.  Invalid argument(s)");
		return 0;
	}
	FE_mesh *mesh = element->getMesh();
	if (!mesh)
	{
		display_message(ERROR_MESSAGE, "Element getNode.  Invalid element");
		return 0;
	}
	FE_mesh_element_field_template_data *eftData = mesh->getElementfieldtemplateData(eft->get_FE_element_field_template());
	if (!eftData)
	{
		display_message(ERROR_MESSAGE, "Element getNode.  Element field template is not used by element's mesh");
		return 0;
	}
	const DsLabelIndex nodeIndex = eftData->getElementLocalNode(element->getIndex(), localNodeIndex - 1);
	if (nodeIndex == DS_LABEL_INDEX_INVALID)
		return 0;
	cmzn_node *node = mesh->getNodeset()->getNode(nodeIndex);
	cmzn_node_access(node);
	return node;
}

int cmzn_element_set_node(cmzn_element_id element,
	cmzn_elementfieldtemplate_id eft, int localNodeIndex, cmzn_node_id node)
{
	if (!((element) && (eft) && (0 < localNodeIndex) && (localNodeIndex <= eft->getNumberOfLocalNodes())))
	{
		display_message(ERROR_MESSAGE, "Element setNode.  Invalid argument(s)");
		return CMZN_ERROR_ARGUMENT;
	}
	FE_mesh *mesh = element->getMesh();
	if (!mesh)
	{
		display_message(ERROR_MESSAGE, "Element setNode.  Invalid element");
		return CMZN_ERROR_ARGUMENT;
	}
	FE_mesh_element_field_template_data *eftData = mesh->getElementfieldtemplateData(eft->get_FE_element_field_template());
	if (!eftData)
	{
		display_message(ERROR_MESSAGE, "Element setNode.  Element field template is not used by element's mesh");
		return CMZN_ERROR_ARGUMENT;
	}
	return eftData->setElementLocalNode(element->getIndex(), localNodeIndex - 1, (node) ? node->getIndex() : DS_LABEL_INDEX_INVALID);
}

int cmzn_element_set_nodes_by_identifier(cmzn_element_id element,
	cmzn_elementfieldtemplate_id eft, int identifiersCount,
	const int *identifiersIn)
{
	if (!((element) && (eft) && (identifiersCount == eft->getNumberOfLocalNodes()) && (identifiersIn)))
	{
		display_message(ERROR_MESSAGE, "Element setNodesByIdentifier.  Invalid argument(s)");
		return CMZN_ERROR_ARGUMENT;
	}
	FE_mesh *mesh = element->getMesh();
	if (!mesh)
	{
		display_message(ERROR_MESSAGE, "Element setNodesByIdentifier.  Invalid element");
		return CMZN_ERROR_ARGUMENT;
	}
	FE_mesh_element_field_template_data *eftData = mesh->getElementfieldtemplateData(eft->get_FE_element_field_template());
	if (!eftData)
	{
		display_message(ERROR_MESSAGE, "Element setNodesByIdentifier.  Element field template is not used by element's mesh");
		return CMZN_ERROR_ARGUMENT;
	}
	const int result = eftData->setElementLocalNodesByIdentifier(element->getIndex(), identifiersIn);
	if (result != CMZN_OK)
	{
		display_message(ERROR_MESSAGE, "Element setNodesByIdentifier.  Failed to set nodes for %d-D element %d",
			mesh->getDimension(), element->getIdentifier());
	}
	return result;
}

int cmzn_element_get_scale_factor(cmzn_element_id element,
	cmzn_elementfieldtemplate_id eft, int localScaleFactorIndex, double *valueOut)
{
	if (!((element) && (eft) && (0 < localScaleFactorIndex)
		&& (localScaleFactorIndex <= eft->getNumberOfLocalScaleFactors()) && (valueOut)))
	{
		display_message(ERROR_MESSAGE, "Element getScaleFactor.  Invalid argument(s)");
		return CMZN_ERROR_ARGUMENT;
	}
	FE_mesh *mesh = element->getMesh();
	if (!mesh)
	{
		display_message(ERROR_MESSAGE, "Element getScaleFactor.  Invalid element");
		return CMZN_ERROR_ARGUMENT;
	}
	FE_mesh_element_field_template_data *eftData = mesh->getElementfieldtemplateData(eft->get_FE_element_field_template());
	if (!eftData)
	{
		display_message(ERROR_MESSAGE, "Element getScaleFactor.  Element field template is not used by element's mesh");
		return CMZN_ERROR_ARGUMENT;
	}
	return eftData->getElementScaleFactor(element->getIndex(), localScaleFactorIndex - 1, *valueOut);
}

int cmzn_element_set_scale_factor(cmzn_element_id element,
	cmzn_elementfieldtemplate_id eft, int localScaleFactorIndex, double value)
{
	if (!((element) && (eft) && (0 < localScaleFactorIndex)
		&& (localScaleFactorIndex <= eft->getNumberOfLocalScaleFactors())))
	{
		display_message(ERROR_MESSAGE, "Element setScaleFactor.  Invalid argument(s)");
		return CMZN_ERROR_ARGUMENT;
	}
	FE_mesh *mesh = element->getMesh();
	if (!mesh)
	{
		display_message(ERROR_MESSAGE, "Element setScaleFactor.  Invalid element");
		return CMZN_ERROR_ARGUMENT;
	}
	FE_mesh_element_field_template_data *eftData = mesh->getElementfieldtemplateData(eft->get_FE_element_field_template());
	if (!eftData)
	{
		display_message(ERROR_MESSAGE, "Element setScaleFactor.  Element field template is not used by element's mesh");
		return CMZN_ERROR_ARGUMENT;
	}
	return eftData->setElementScaleFactor(element->getIndex(), localScaleFactorIndex - 1, value);
}

int cmzn_element_get_scale_factors(cmzn_element_id element,
	cmzn_elementfieldtemplate_id eft, int valuesCount, double *valuesOut)
{
	if (!((element) && (eft) && (valuesCount == eft->getNumberOfLocalScaleFactors()) && (valuesOut)))
	{
		display_message(ERROR_MESSAGE, "Element getScaleFactors.  Invalid argument(s)");
		return CMZN_ERROR_ARGUMENT;
	}
	FE_mesh *mesh = element->getMesh();
	if (!mesh)
	{
		display_message(ERROR_MESSAGE, "Element getScaleFactors.  Invalid element");
		return CMZN_ERROR_ARGUMENT;
	}
	FE_mesh_element_field_template_data *eftData = mesh->getElementfieldtemplateData(eft->get_FE_element_field_template());
	if (!eftData)
	{
		display_message(ERROR_MESSAGE, "Element getScaleFactors.  Element field template is not used by element's mesh");
		return CMZN_ERROR_ARGUMENT;
	}
	return eftData->getOrCreateElementScaleFactors(element->getIndex(), valuesOut);
}
	
int cmzn_element_set_scale_factors(cmzn_element_id element,
	cmzn_elementfieldtemplate_id eft, int valuesCount, const double *valuesIn)
{
	if (!((element) && (eft) && (valuesCount == eft->getNumberOfLocalScaleFactors()) && (valuesIn)))
	{
		display_message(ERROR_MESSAGE, "Element setScaleFactors.  Invalid argument(s)");
		return CMZN_ERROR_ARGUMENT;
	}
	FE_mesh *mesh = element->getMesh();
	if (!mesh)
	{
		display_message(ERROR_MESSAGE, "Element setScaleFactors.  Invalid element");
		return CMZN_ERROR_ARGUMENT;
	}
	FE_mesh_element_field_template_data *eftData = mesh->getElementfieldtemplateData(eft->get_FE_element_field_template());
	if (!eftData)
	{
		display_message(ERROR_MESSAGE, "Element setScaleFactors.  Element field template is not used by element's mesh");
		return CMZN_ERROR_ARGUMENT;
	}
	return eftData->setElementScaleFactors(element->getIndex(), valuesIn);
}

enum cmzn_element_shape_type cmzn_element_get_shape_type(
	cmzn_element_id element)
{
	cmzn_element_shape_type shape_type = CMZN_ELEMENT_SHAPE_TYPE_INVALID;
	if (element)
	{
		FE_mesh *mesh = element->getMesh();
		if (!mesh)
		{
			display_message(ERROR_MESSAGE, "Element getShapeType.  Invalid element");
		}
		else
		{
			shape_type = mesh->getElementShapeType(get_FE_element_index(element));
		}
	}
	return shape_type;
}

int cmzn_element_merge(cmzn_element_id element,
	cmzn_elementtemplate_id element_template)
{
	if (element && element_template)
		return element_template->mergeIntoElement(element);
	return CMZN_ERROR_ARGUMENT;
}

class cmzn_element_shape_type_conversion
{
public:
	static const char *to_string(enum cmzn_element_shape_type type)
	{
		const char *enum_string = 0;
		switch (type)
		{
			case CMZN_ELEMENT_SHAPE_TYPE_LINE:
				enum_string = "LINE";
				break;
			case CMZN_ELEMENT_SHAPE_TYPE_SQUARE:
				enum_string = "SQUARE";
				break;
			case CMZN_ELEMENT_SHAPE_TYPE_TRIANGLE:
				enum_string = "TRIANGLE";
				break;
			case CMZN_ELEMENT_SHAPE_TYPE_CUBE:
				enum_string = "CUBE";
				break;
			case CMZN_ELEMENT_SHAPE_TYPE_TETRAHEDRON:
				enum_string = "TETRAHEDRON";
				break;
			case CMZN_ELEMENT_SHAPE_TYPE_WEDGE12:
				enum_string = "WEDGE12";
				break;
			case CMZN_ELEMENT_SHAPE_TYPE_WEDGE13:
				enum_string = "WEDGE13";
				break;
			case CMZN_ELEMENT_SHAPE_TYPE_WEDGE23:
				enum_string = "_WEDGE23";
				break;
			default:
				break;
		}
		return enum_string;
	}
};

enum cmzn_element_shape_type cmzn_element_shape_type_enum_from_string(
	const char *string)
{
	return string_to_enum<enum cmzn_element_shape_type,	cmzn_element_shape_type_conversion>(string);
}

char *cmzn_element_shape_type_enum_to_string(enum cmzn_element_shape_type type)
{
	const char *type_string = cmzn_element_shape_type_conversion::to_string(type);
	return (type_string ? duplicate_string(type_string) : 0);
}

class cmzn_elementbasis_function_type_conversion
{
public:
	static const char *to_string(enum cmzn_elementbasis_function_type type)
	{
		const char *enum_string = 0;
		switch (type)
		{
			case CMZN_ELEMENTBASIS_FUNCTION_TYPE_CONSTANT:
				enum_string = "CONSTANT";
				break;
			case CMZN_ELEMENTBASIS_FUNCTION_TYPE_LINEAR_LAGRANGE:
				enum_string = "LINEAR_LAGRANGE";
				break;
			case CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_LAGRANGE:
				enum_string = "QUADRATIC_LAGRANGE";
				break;
			case CMZN_ELEMENTBASIS_FUNCTION_TYPE_CUBIC_LAGRANGE:
				enum_string = "CUBIC_LAGRANGE";
				break;
			case CMZN_ELEMENTBASIS_FUNCTION_TYPE_LINEAR_SIMPLEX:
				enum_string = "LINEAR_SIMPLEX";
				break;
			case CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_SIMPLEX:
				enum_string = "QUADRATIC_SIMPLEX";
				break;
			case CMZN_ELEMENTBASIS_FUNCTION_TYPE_CUBIC_HERMITE:
				enum_string = "CUBIC_HERMITE";
				break;
			case CMZN_ELEMENTBASIS_FUNCTION_TYPE_CUBIC_HERMITE_SERENDIPITY:
				enum_string = "CUBIC_HERMITE_SERENDIPITY";
				break;
			case CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_HERMITE_LAGRANGE:
				enum_string = "QUADRATIC_HERMITE_LAGRANGE";
				break;
			case CMZN_ELEMENTBASIS_FUNCTION_TYPE_QUADRATIC_LAGRANGE_HERMITE:
				enum_string = "QUADRATIC_LAGRANGE_HERMITE";
				break;
			default:
				break;
		}
		return enum_string;
	}
};

enum cmzn_elementbasis_function_type cmzn_elementbasis_function_type_enum_from_string(
	const char *string)
{
	return string_to_enum<enum cmzn_elementbasis_function_type,	cmzn_elementbasis_function_type_conversion>(string);
}

char *cmzn_elementbasis_function_type_enum_to_string(enum cmzn_elementbasis_function_type type)
{
	const char *type_string = cmzn_elementbasis_function_type_conversion::to_string(type);
	return (type_string ? duplicate_string(type_string) : 0);
}

cmzn_meshchanges::cmzn_meshchanges(cmzn_fieldmoduleevent *eventIn, cmzn_mesh *meshIn) :
	event(eventIn->access()),
	changeLog(cmzn::Access(eventIn->getFeRegionChanges()->getElementChangeLog(meshIn->getDimension()))),
	access_count(1)
{
	// optimial time to do this:
	eventIn->getFeRegionChanges()->propagateToDimension(meshIn->getDimension());
}

cmzn_meshchanges::~cmzn_meshchanges()
{
	cmzn::Deaccess(this->changeLog);
	cmzn_fieldmoduleevent::deaccess(this->event);
}

cmzn_meshchanges *cmzn_meshchanges::create(cmzn_fieldmoduleevent *eventIn, cmzn_mesh *meshIn)
{
	if (eventIn && (eventIn->getFeRegionChanges()) && meshIn && 
		(eventIn->getRegion()->get_FE_region() == cmzn_mesh_get_FE_region_internal(meshIn)))
		return new cmzn_meshchanges(eventIn, meshIn);
	return 0;
}

int cmzn_meshchanges::deaccess(cmzn_meshchanges* &meshchanges)
{
	if (meshchanges)
	{
		--(meshchanges->access_count);
		if (meshchanges->access_count <= 0)
			delete meshchanges;
		meshchanges = 0;
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

// Note: internal and external flags have same numerical values
cmzn_element_change_flags cmzn_meshchanges::getElementChangeFlags(cmzn_element *element)
{
	// relies on FE_region_changes::propagateToDimension call in constructor
	if (element && this->changeLog->isIndexChange(element->getIndex()))
		return this->changeLog->getChangeSummary();
	return CMZN_ELEMENT_CHANGE_FLAG_NONE;
}

cmzn_meshchanges_id cmzn_meshchanges_access(
	cmzn_meshchanges_id meshchanges)
{
	if (meshchanges)
		return meshchanges->access();
	return 0;
}

int cmzn_meshchanges_destroy(cmzn_meshchanges_id *meshchanges_address)
{
	if (meshchanges_address)
		return cmzn_meshchanges::deaccess(*meshchanges_address);
	return CMZN_ERROR_ARGUMENT;
}

cmzn_element_change_flags cmzn_meshchanges_get_element_change_flags(
	cmzn_meshchanges_id meshchanges, cmzn_element_id element)
{
	if (meshchanges)
		return meshchanges->getElementChangeFlags(element);
	return CMZN_ELEMENT_CHANGE_FLAG_NONE;
}

int cmzn_meshchanges_get_number_of_changes(
	cmzn_meshchanges_id meshchanges)
{
	if (meshchanges)
		return meshchanges->getNumberOfChanges();
	return 0;
}

cmzn_element_change_flags cmzn_meshchanges_get_summary_element_change_flags(
	cmzn_meshchanges_id meshchanges)
{
	if (meshchanges)
		return meshchanges->getSummaryElementChangeFlags();
	return CMZN_ELEMENT_CHANGE_FLAG_NONE;
}

cmzn_mesh_group_id cmzn_fieldmodule_create_mesh_group_from_name_internal(
	cmzn_fieldmodule_id fieldmodule, const char *mesh_group_name)
{
	cmzn_mesh_group_id mesh_group = 0;
	if (fieldmodule && mesh_group_name)
	{
		cmzn_field_id existing_field = cmzn_fieldmodule_find_field_by_name(fieldmodule, mesh_group_name);
		if (existing_field)
		{
			cmzn_field_destroy(&existing_field);
		}
		else
		{
			char *group_name = duplicate_string(mesh_group_name);
			char *mesh_name = strrchr(group_name, '.');
			if (mesh_name)
			{
				*mesh_name = '\0';
				++mesh_name;
				cmzn_mesh_id master_mesh = cmzn_fieldmodule_find_mesh_by_name(fieldmodule, mesh_name);
				cmzn_field_id field = cmzn_fieldmodule_find_field_by_name(fieldmodule, group_name);
				cmzn_field_group_id group = cmzn_field_cast_group(field);
				cmzn_field_element_group_id element_group = cmzn_field_group_create_field_element_group(group, master_mesh);
				mesh_group = cmzn_field_element_group_get_mesh_group(element_group);
				cmzn_field_element_group_destroy(&element_group);
				cmzn_field_group_destroy(&group);
				cmzn_field_destroy(&field);
				cmzn_mesh_destroy(&master_mesh);
			}
			DEALLOCATE(group_name);
		}
	}
	return mesh_group;
}
